/**
 * @file
 * @brief Low level mobile adapter protocol
 *
 * This file implements the low level protocol for the mobile adapter, and all
 * the serial handling and timeouts surrounding it.
 *
 * The two main functions of interest are MA_IntrTimer and MA_IntrSerialIO. Both
 * of these functions should be associated with their respective interrupts
 * within the main program. MA_IntrTimer is responsible for starting serial
 * transmissions at a constant rate, allowing the adapter enough time to breathe
 * between each transmission to process the data. MA_IntrSerialIO triggers when
 * the byte exchange is finished, and is responsible for processing it.
 */

#include "ma_bios.h"

#include <stddef.h>
#include "ma_api.h"
#include "ma_sub.h"

// Serial protocol magic bytes
#define MAPROT_IDLE_SLAVE 0xd2
#define MAPROT_IDLE_MASTER 0x4b
#define MAPROT_MAGIC_1 0x99
#define MAPROT_MAGIC_2 0x66

// Serial protocol errors
/// Command is unsupported by the adapter
#define MAPROT_ERR_ILLEGAL_CMD 0xf0
/// Packet checksum failed, retry
#define MAPROT_ERR_CHECKSUM 0xf1
/// Internal mobile adapter error (e.g. transmit buffer full)
#define MAPROT_ERR_INTERNAL 0xf2

/// MA_SetTimeoutCount() indexes
enum timeouts {
    TIMEOUT_02,
    TIMEOUT_10,
    TIMEOUT_30,
    TIMEOUT_90,
};

/// States of the packet sender
enum iobuf_send_states {
    /// Do nothing
    IOBUF_SEND_IDLE,
    /// Send byte or word in buffer, wait a bit and move to IOBUF_SEND_START
    IOBUF_SEND_PRESTART,
    /// Send MaPacketData_Start
    IOBUF_SEND_START,
    /// Send packet in buffer
    IOBUF_SEND_DATA,
};

/// States of the packet receiver
enum iobuf_recv_states {
    /// Do nothing
    IOBUF_RECV_IDLE,
    /// Receive magic bytes
    IOBUF_RECV_START,
    /// Receive packet header
    IOBUF_RECV_HEADER,
    /// Receive packet data
    IOBUF_RECV_DATA,
    /// Receive and verify packet checksum
    IOBUF_RECV_CHECKSUM,
    /// Receive and verify packet footer
    IOBUF_RECV_FOOTER,
};

static void SetInternalRecvBuffer(void);
static void MA_SetInterval(int index);
static void MA_SetTimeoutCount(int index);
static int MA_PreSend(void);
static void MA_InitIoBuffer(MA_IOBUF *buffer, vu8 *mem, u16 size, u16 state);
static void MA_StartSioTransmit(void);
static void MA_SetTransmitData(MA_IOBUF *pSendIoBuf);
static int MA_IsSupportedHardware(u8 hardware);
static void MABIOS_Data2(MA_BUF *pRecvBuf, const u8 *pSendData, u8 sendSize);
static int MA_CreatePacket(u8 *packet, u8 cmd, u16 size);
static int MA_Create8BitPacket(u8 *packet, u8 cmd, u16 size);
static int MA_Create32BitPacket(u8 *packet, u8 cmd, u16 size);
static u16 MA_CalcCheckSum(u8 *pData, u16 size);
static void MA_IntrTimer_SIOSend(void);
static void MA_IntrTimer_SIORecv(void);
static void MA_IntrTimer_SIOIdle(void);
static void MA_IntrTimer_SIOWaitTime(void);
static void ConvertNegaErrToApiErr(void);
static void MA_ProcessRecvPacket(u8 cmd);
static void MA_IntrSio_Send(void);
static void MA_IntrSio_Recv(u8 byte);

static u8 *tmppPacket;
static MAPROT_FOOTER *tmppPacketLast;
static u16 tmpPacketLen;
static int i;

// Byte (8-bit serial) timer settings
#define COUNTER_BYTE_INTER1 TIMER_MS(0.20)  // 4096.6796875 Hz
#define COUNTER_BYTE_INTER2 TIMER_MS(0.30)  // 3277.34375 Hz
#define COUNTER_BYTE_INTER3 TIMER_MS(0.40)  // 2340.9598214285716 Hz
#define COUNTER_BYTE_INTER4 TIMER_MS(0.50)  // 1820.7465277777778 Hz
#define COUNTER_BYTE_INTER5 TIMER_MS(0.60)  // 1638.671875 Hz

/// Byte timer interrupt intervals
static const u16 gTimerIntByteInter[] = {
    -COUNTER_BYTE_INTER1,
    -COUNTER_BYTE_INTER2,
    -COUNTER_BYTE_INTER3,
    -COUNTER_BYTE_INTER4,
    -COUNTER_BYTE_INTER5,

    -COUNTER_BYTE_INTER1 * 2,
    -COUNTER_BYTE_INTER2 * 2,
    -COUNTER_BYTE_INTER3 * 2,
    -COUNTER_BYTE_INTER4 * 2,
    -COUNTER_BYTE_INTER5 * 2,
};

/// Calculate a timeout counter for each of the byte interrupt intervals
#define COUNTER_BYTE(ms) \
    TIMER_MS((float)(ms) / COUNTER_BYTE_INTER1), \
    TIMER_MS((float)(ms) / COUNTER_BYTE_INTER2), \
    TIMER_MS((float)(ms) / COUNTER_BYTE_INTER3), \
    TIMER_MS((float)(ms) / COUNTER_BYTE_INTER4), \
    TIMER_MS((float)(ms) / COUNTER_BYTE_INTER5), \
\
    TIMER_MS((float)(ms) / COUNTER_BYTE_INTER1) / 2, \
    TIMER_MS((float)(ms) / COUNTER_BYTE_INTER2) / 2, \
    TIMER_MS((float)(ms) / COUNTER_BYTE_INTER3) / 2, \
    TIMER_MS((float)(ms) / COUNTER_BYTE_INTER4) / 2, \
    TIMER_MS((float)(ms) / COUNTER_BYTE_INTER5) / 2,

// Pre-calculated timeout counters for various timeouts
static const u32 gNullCounterByte[] = {COUNTER_BYTE(1000)};
static const u32 gP2PCounterByte[] = {COUNTER_BYTE(125)};
static const u32 gTimeout90CounterByte[] = {COUNTER_BYTE(90000 + 0.05)};
static const u32 gTimeout30CounterByte[] = {COUNTER_BYTE(30000)};
static const u32 gTimeout10CounterByte[] = {COUNTER_BYTE(10000)};
static const u32 gTimeout02CounterByte[] = {COUNTER_BYTE(2000)};
static const u32 gTimeout200msecCounterByte[] = {COUNTER_BYTE(200)};
static const u32 gTimeout250msecCounterByte[] = {COUNTER_BYTE(250)};
static const u32 gTimeout40msecCounterByte[] = {COUNTER_BYTE(40 - 0.05)};

// Word (32-bit serial) timer settings
#define COUNTER_WORD_INTER1 (COUNTER_BYTE_INTER1 * 2)  // 2048.33984375 Hz
#define COUNTER_WORD_INTER2 TIMER_MS(0.40)  // 2340.9598214285716 Hz
#define COUNTER_WORD_INTER3 TIMER_MS(0.50)  // 1820.7465277777778 Hz
#define COUNTER_WORD_INTER4 TIMER_MS(0.60)  // 1638.671875 Hz
#define COUNTER_WORD_INTER5 TIMER_MS(0.70)  // 1365.5598958333333 Hz

/// Word timer interrupt intervals
static const u16 gTimerIntWordInter[] = {
    -COUNTER_WORD_INTER1,
    -COUNTER_WORD_INTER2,
    -COUNTER_WORD_INTER3,
    -COUNTER_WORD_INTER3,
    -COUNTER_WORD_INTER4,

    -COUNTER_WORD_INTER1 * 2,
    -COUNTER_WORD_INTER2 * 2,
    -COUNTER_WORD_INTER3 * 2,
    -COUNTER_WORD_INTER3 * 2,
    -COUNTER_WORD_INTER4 * 2,
};

/// Calculate a timeout counter for each of the word interrupt intervals
#define COUNTER_WORD(ms) \
    TIMER_MS((float)(ms) / COUNTER_WORD_INTER1), \
    TIMER_MS((float)(ms) / COUNTER_WORD_INTER2), \
    TIMER_MS((float)(ms) / COUNTER_WORD_INTER3), \
    TIMER_MS((float)(ms) / COUNTER_WORD_INTER4), \
    TIMER_MS((float)(ms) / COUNTER_WORD_INTER5), \
\
    TIMER_MS((float)(ms) / COUNTER_WORD_INTER1) / 2, \
    TIMER_MS((float)(ms) / COUNTER_WORD_INTER2) / 2, \
    TIMER_MS((float)(ms) / COUNTER_WORD_INTER3) / 2, \
    TIMER_MS((float)(ms) / COUNTER_WORD_INTER4) / 2, \
    TIMER_MS((float)(ms) / COUNTER_WORD_INTER5) / 2,

// Pre-calculated timeout counters for various timeouts
static const u32 gNullCounterWord[] = {COUNTER_WORD(1000)};
static const u32 gP2PCounterWord[] = {COUNTER_WORD(125)};
static const u32 gTimeout90CounterWord[] = {COUNTER_WORD(90000)};
static const u32 gTimeout30CounterWord[] = {COUNTER_WORD(30000)};
static const u32 gTimeout10CounterWord[] = {COUNTER_WORD(10000)};
static const u32 gTimeout02CounterWord[] = {COUNTER_WORD(2000)};
static const u32 gTimeout200msecCounterWord[] = {COUNTER_WORD(200)};
static const u32 gTimeout250msecCounterWord[] = {COUNTER_WORD(250)};
static const u32 gTimeout40msecCounterWord[] = {COUNTER_WORD(40)};

/// Idle bytes, also used to wake the adapter up if it went on standby
static const u8 MaPacketData_PreStart[] = {
    MAPROT_IDLE_MASTER,
    MAPROT_IDLE_MASTER,
    MAPROT_IDLE_MASTER,
    MAPROT_IDLE_MASTER,
};

/// Protocol initialization packet
static const u8 MaPacketData_Start[] = {
    MAPROT_MAGIC_1, MAPROT_MAGIC_2,  // Magic bytes
    MACMD_START, 0x00, 0x00, 0x08,  // Command ID, size
    'N', 'I', 'N', 'T', 'E', 'N', 'D', 'O',  // Data
    0x02, 0x77,  // Checksum
    MAPROT_TYPE_MASTER | MATYPE_GBA, 0x00, 0x00, 0x00,  // Footer
};

/// No-op packet
static const u8 MaPacketData_NULL[] = {
    MAPROT_MAGIC_1, MAPROT_MAGIC_2,  // Magic bytes
    MACMD_NULL, 0x00, 0x00, 0x00,  // Command ID, size
    0x00, 0x0f,  // Checksum
    MAPROT_TYPE_MASTER | MATYPE_GBA, 0x00, 0x00, 0x00,  // Footer
};

/// Check adapter status
__attribute__((unused))
static const u8 MaPacketData_CheckStatus[] = {
    MAPROT_MAGIC_1, MAPROT_MAGIC_2,  // Magic bytes
    MACMD_CHECK_STATUS, 0x00, 0x00, 0x00,  // Command ID, size
    0x00, 0x17,  // Checksum
    MAPROT_TYPE_MASTER | MATYPE_GBA, 0x00, 0x00, 0x00,  // Footer
};

/**
 * @brief Initialize hardware and variables
 */
void MABIOS_Init(void)
{
    *(vu16 *)REG_IME = 0;

    // Initialize serial hardware and interrupts
    *(vu32 *)REG_TM3CNT = 0;
    *(vu16 *)REG_RCNT = R_SIO_MASTER_MODE;
    *(vu16 *)REG_SIOCNT = SIO_8BIT_MODE;
    *(vu16 *)REG_SIOCNT |= SIO_IF_ENABLE | SIO_SCK_IN;
    *(vu16 *)REG_IF = SIO_INTR_FLAG | TIMER3_INTR_FLAG;
    *(vu16 *)REG_IE |= SIO_INTR_FLAG | TIMER3_INTR_FLAG;
    *(vu32 *)REG_TM3CNT = 0;

    // Initialize serial hardware variables
    gMA.condition = 0;
    gMA.error = -1;
    gMA.intrMode = MA_INTR_IDLE;
    gMA.hardwareType = -1;
    MA_ChangeSIOMode(MA_SIO_BYTE);

    // Initialize timeouts
    MA_SetInterval(0);
    gMA.counter = 0;
    gMA.timerInter = 0;
    gMA.retryCount = 0;

    // Initalize protocol variables and buffers
    gMA.status = 0;
    gMA.pSendIoBuf = NULL;
    gMA.sendCmd = 0;
    gMA.recvCmd = 0;
    gMA.checkSum = 0;
    gMA.sendFooter[0] = 0;
    gMA.sendFooter[1] = 0;
    gMA.recvFooter[0] = 0;
    gMA.recvFooter[1] = 0;
    gMA.biosRecvBuf.size = sizeof(gMA.biosRecvPacket);
    gMA.biosRecvBuf.data = gMA.biosRecvPacket;
    gMA.recvBuf.size = sizeof(gMA.recvPacket);
    gMA.recvBuf.data = gMA.recvPacket;

    *(vu16 *)REG_IME = 1;
}

/**
 * @brief Use the internal receive buffer
 *
 * This is used for protocol commands that don't return any useful data, as
 * they will not have a parameter for the reply packet, and they will be
 * handled internally.
 *
 * @bug Buffer isn't big enough, MA_IntrSio_Recv has no boundary checks.
 */
static void SetInternalRecvBuffer(void)
{
    gMA.biosRecvBuf.size = sizeof(gMA.biosRecvPacket);
    gMA.biosRecvBuf.data = gMA.biosRecvPacket;
    gMA.pRecvBuf = &gMA.biosRecvBuf;
}

/**
 * @brief Set serial transaction interval
 *
 * This function initializes the interval between each byte and each word, for
 * both of the serial modes, as well as all the timeouts values.
 *
 * The index parameter lets you choose between one of 5 interval settings, with
 * 0 being the fastest and 4 being the slowest. Only 0 is actually used.
 *
 * @param[in] index interval setting
 */
static void MA_SetInterval(int index)
{
    // The blue adapter needs twice the interval between each serial transaction
    if (gMA.hardwareType == (MAPROT_TYPE_SLAVE | MATYPE_PDC)) {
        index += 5;
    }

    gMA.interval = index;
    gMA.timerIntInter[MA_SIO_BYTE] = gTimerIntByteInter[index];
    gMA.timerIntInter[MA_SIO_WORD] = gTimerIntWordInter[index];
    gMA.nullCounter[MA_SIO_BYTE] = gNullCounterByte[index];
    gMA.nullCounter[MA_SIO_WORD] = gNullCounterWord[index];
    gMA.P2PCounter[MA_SIO_BYTE] = gP2PCounterByte[index];
    gMA.P2PCounter[MA_SIO_WORD] = gP2PCounterWord[index];
    gMA.timeout200msecCounter[MA_SIO_BYTE] = gTimeout200msecCounterByte[index];
    gMA.timeout200msecCounter[MA_SIO_WORD] = gTimeout200msecCounterWord[index];

    switch (gMA.hardwareType) {
    case MAPROT_TYPE_SLAVE | MATYPE_CDMA:
        gMA.tcpDelayCounter[MA_SIO_BYTE] = gTimeout250msecCounterByte[index];
        gMA.tcpDelayCounter[MA_SIO_WORD] = gTimeout250msecCounterWord[index];
        break;

    case MAPROT_TYPE_SLAVE | MATYPE_PDC:
        gMA.tcpDelayCounter[MA_SIO_BYTE] = gTimeout200msecCounterByte[index];
        gMA.tcpDelayCounter[MA_SIO_WORD] = gTimeout200msecCounterWord[index];
        break;

    case MAPROT_TYPE_SLAVE | MATYPE_PHS_Pocket:
        gMA.tcpDelayCounter[MA_SIO_BYTE] = gTimeout40msecCounterByte[index];
        gMA.tcpDelayCounter[MA_SIO_WORD] = gTimeout40msecCounterWord[index];
        break;

    default:
        gMA.tcpDelayCounter[MA_SIO_BYTE] = 0;
        gMA.tcpDelayCounter[MA_SIO_WORD] = 0;
        break;
    }
}

/**
 * @brief Set serial timeout
 *
 * Set the maximum timeout for a protocol transaction. This includes the time it
 * takes to send the requesting command, and how long it takes for the reply
 * to arrive.
 *
 * @param[in] index timeout index
 */
static void MA_SetTimeoutCount(int index)
{
    static const u32 * const counterArrayByte[] = {
        gTimeout02CounterByte,
        gTimeout10CounterByte,
        gTimeout30CounterByte,
        gTimeout90CounterByte,
    };

    static const u32 * const counterArrayWord[] = {
        gTimeout02CounterWord,
        gTimeout10CounterWord,
        gTimeout30CounterWord,
        gTimeout90CounterWord,
    };

    gMA.timeoutCounter[MA_SIO_BYTE] = counterArrayByte[index][gMA.interval];
    gMA.timeoutCounter[MA_SIO_WORD] = counterArrayWord[index][gMA.interval];
}

/**
 * @brief Check if the library is connected to the adapter
 *
 * @return boolean
 */
int MA_GetStatus(void)
{
    if (gMA.status & STATUS_CONNECTED) {
        return TRUE;
    } else {
        return FALSE;
    }
}

/**
 * @brief Get condition flags
 *
 * @return condition flags
 */
u16 MA_GetCondition(void)
{
    return gMA.condition;
}

/**
 * @brief Read out the error variable and clear error status
 *
 * Unused.
 *
 * @return error status
 */
u8 MA_ErrorCheck(void)
{
    gMA.condition &= ~MA_CONDITION_ERROR;
    return gMA.error;
}

/**
 * @brief Set api error
 *
 * Resets all the bios-related variables in doing so.
 *
 * @param[in] error api error code
 */
void MA_SetError(u8 error)
{
    if (error == MAAPIE_MA_NOT_FOUND) {
        MA_Reset();
    }

    gMA.error = error;
    gMA.intrMode = MA_INTR_IDLE;
    gMA.sendIoBuf.state = IOBUF_SEND_IDLE;
    gMA.recvIoBuf.state = IOBUF_RECV_IDLE;
    gMA.condition |= MA_CONDITION_ERROR;
    gMA.condition &= ~MA_CONDITION_BIOS_BUSY;
    gMA.task = TASK_NONE;
    gMA.condition &= ~MA_CONDITION_APIWAIT;
    gMA.condition &= ~MA_CONDITION_BUFFER_FULL;
    gMA.pSendIoBuf = NULL;
}

/**
 * @brief Initialize the library for sending a command
 *
 * Called before sending a command to make sure the bios isn't busy processing
 * a different command before sending. Return value indicates if a command can
 * be sent right now.
 *
 * @return boolean
 */
static int MA_PreSend(void)
{
    if (gMA.condition & MA_CONDITION_BIOS_BUSY) {
        MA_SetError(MAAPIE_CANNOT_EXECUTE);
        return FALSE;
    }

    if (gMA.condition & MA_CONDITION_ERROR) {
        MA_SetError(MAAPIE_CANNOT_EXECUTE);
        return FALSE;
    }

    gMA.condition &= ~MA_CONDITION_ERROR;
    gMA.error = -1;
    gMA.condition &= ~MA_CONDITION_TCPCLOSED;
    gMA.retryCount = 0;
    gMA.status &= ~STATUS_SIO_RETRY;
    gMA.counter = 0;
    gMA.sendCmd = 0;
    gMA.recvCmd = 0;
    return TRUE;
}

/**
 * @brief Initialize a serial IO buffer
 *
 * Initializes a buffer for sending or receiving a command. Depending on the
 * buffer, the memory pointed to by the mem and size pair will be either read
 * or written. The state variable indicates the next operation the interrupts
 * will perform on the buffer, and depends on the buffer.
 *
 * @param[out] pBuffer IO buffer to initialize
 * @param[in] pMem memory to read from/write to
 * @param[in] size size of memory
 * @param[in] state next operation, depending on buffer
 */
static void MA_InitIoBuffer(MA_IOBUF *pBuffer, vu8 *pMem, u16 size, u16 state)
{
    pBuffer->state = state;
    pBuffer->pRead = pMem;
    pBuffer->pWrite = pMem;
    pBuffer->size = size;
    pBuffer->readCnt = 0;
    pBuffer->checkSum = 0;
}

/**
 * @brief Transmit the next data
 *
 * If a transmission buffer is set, start transmission of the inmediate next
 * byte or word (depending on serial mode), and then stop. This will trigger
 * the serial interrupt (which should call MA_IntrSerialIO()) on completion,
 * allowing it to interpret the received byte.
 *
 * The transmission buffer is set by MA_SetTransmitData() before this function
 * is called.
 */
static void MA_StartSioTransmit(void)
{
    static u32 wordData;

    if (!gMA.pSendIoBuf) return;

    while (*(vu16 *)REG_SIOCNT & SIO_START) {}

    if (gMA.sioMode == MA_SIO_WORD) {
        wordData = gMA.pSendIoBuf->pRead[3] << 0
                   | gMA.pSendIoBuf->pRead[2] << 8
                   | gMA.pSendIoBuf->pRead[1] << 16
                   | gMA.pSendIoBuf->pRead[0] << 24;
        *(vu32 *)REG_SIODATA32 = wordData;

        gMA.pSendIoBuf->pRead += 4;
        gMA.pSendIoBuf->readCnt += 4;
    } else {
        *(vu8 *)REG_SIODATA8 = gMA.pSendIoBuf->pRead[0];

        gMA.pSendIoBuf->pRead += 1;
        gMA.pSendIoBuf->readCnt += 1;
    }

    gMA.pSendIoBuf = NULL;
    gMA.status &= ~STATUS_TIMER_ENABLED;
    *(vu16 *)REG_SIOCNT |= SIO_START;
}

/**
 * @brief Set transmission IO buffer
 *
 * Sets the serial IO buffer that will be used by MA_StartSioTransmit() to
 * transmit the next byte or word (depending on serial mode).
 *
 * @param[in] pSendIoBuf data to transmit
 */
static void MA_SetTransmitData(MA_IOBUF *pSendIoBuf)
{
    gMA.pSendIoBuf = pSendIoBuf;
}

/**
 * @brief Set serial mode
 *
 * Changes the serial mode to either MA_SIO_BYTE or MA_SIO_WORD, the 8-bit and
 * 32-bit modes respectively. This affects interrupt timings as well as the
 * exact layout of the low level protocol.
 *
 * @param[in] mode serial mode
 */
void MA_ChangeSIOMode(u8 mode)
{
    gMA.sioMode = mode;
    if (mode == MA_SIO_BYTE) {
        *(vu16 *)REG_SIOCNT &= ~SIO_MODE_MASK;
        //*(vu16 *)REG_SIOCNT |= SIO_8BIT_MODE;
    } else {
        *(vu16 *)REG_SIOCNT &= ~SIO_MODE_MASK;
        *(vu16 *)REG_SIOCNT |= SIO_32BIT_MODE;
    }
}

/**
 * @brief Set the delay between each serial transmission
 *
 * Overrides the internal intervals used for each byte and word transmission.
 * The intervals are specified in timer ticks, which is configured to divide
 * the CPU frequency by 1024.
 *
 * Unused.
 *
 * @bug This function has no inmediate effect.
 * @bug This function's settings are easily overridden by the rest of the code.
 *
 * @param[in] byteInter interval for bytes in timer ticks
 * @param[in] wordInter interval for words in timer ticks
 */
void MA_SetDataInterval(u16 byteInter, u16 wordInter)
{
    gMA.timerIntInter[MA_SIO_BYTE] = byteInter;
    gMA.timerIntInter[MA_SIO_WORD] = wordInter;
}

/**
 * @brief Check if the value represents a valid mobile adapter
 *
 * @param[in] hardware adapter type
 * @return boolean
 */
static int MA_IsSupportedHardware(u8 hardware)
{
    if ((hardware & MAPROT_TYPE_MASK) == MAPROT_REPLY) {
        return TRUE;
    } else {
        return FALSE;
    }
}

/**
 * @brief Get the call type value for a given mobile adapter
 *
 * This function is used when dialing a phone number in MABIOS_Tel(), as the
 * call type is the first parameter of the command, and the adapter expects a
 * valid value. The exact meaning of the call types is unknown.
 *
 * @bug typo
 *
 * @param[in] hardware adapter type
 * @return call type
 */
int MA_GetCallTypeFromHarwareType(u8 harware)
{
    if (harware == (MAPROT_TYPE_SLAVE | MATYPE_PDC)) return 0;
    if (harware == (MAPROT_TYPE_SLAVE | MATYPE_CDMA)) return 2;
    if (harware == (MAPROT_TYPE_SLAVE | MATYPE_PHS_DoCoMo)
        || harware == (MAPROT_TYPE_SLAVE | MATYPE_PHS_Pocket)) {
        return 1;
    }
    return 3;
}

/**
 * @brief Dummy packet
 *
 * Sends a null packet, that incites no reply from the adapter. Due to this, it
 * will always cause a timeout on reception, leading to a MA_BiosStop().
 *
 * Unused.
 */
void MABIOS_Null(void)
{
    // Make sure we're connected
    if (!(gMA.status & STATUS_CONNECTED) || gMA.status & STATUS_CONNTEST) {
        return;
    }

    // Not returning reply to caller, use internal reception buffer
    SetInternalRecvBuffer();

    // Initialize transaction
    gMA.condition &= ~MA_CONDITION_BIOS_BUSY;
    gMA.sendCmd = MACMD_NULL;

    // Fill packet buffer
    tmpPacketLen = sizeof(MaPacketData_NULL);
    if (gMA.sioMode == MA_SIO_BYTE) tmpPacketLen -= 2;
    MA_InitIoBuffer(&gMA.sendIoBuf, (u8 *)MaPacketData_NULL, tmpPacketLen,
        IOBUF_SEND_DATA);

    // Configure timer and start transaction
    gMA.intrMode = MA_INTR_SEND;
    gMA.timerInter = gMA.timerIntInter[gMA.sioMode];
    gMA.status |= STATUS_CONNTEST;
    gMA.status |= STATUS_TIMER_ENABLED;
    MA_SetTimeoutCount(TIMEOUT_30);
}

/**
 * @brief Initialize adapter and start session
 *
 * Sends a dummy byte, before waiting a bit and starting the session. This
 * allows the adapter some time to wake up from sleep mode. The remaining part
 * is handled in MA_IntrTimer_SIOSend().
 *
 * @bug Standard adapter reply packet overflows internal reception buffer.
 */
void MABIOS_Start(void)
{
    *(vu32 *)REG_TM3CNT = 0;
    if (!MA_PreSend()) return;

    // Not returning reply to caller, use internal reception buffer
    SetInternalRecvBuffer();

    // Initialize transaction
    gMA.condition |= MA_CONDITION_BIOS_BUSY;
    gMA.sendCmd = MACMD_START;

    // Fill packet buffer
    MA_InitIoBuffer(&gMA.sendIoBuf, (u8 *)MaPacketData_PreStart, 1,
        IOBUF_SEND_PRESTART);

    // Configure timer and start transaction
    gMA.intrMode = MA_INTR_SEND;
    gMA.timerInter = gMA.timerIntInter[gMA.sioMode];
    MA_SetTimeoutCount(TIMEOUT_30);
    gMA.status |= STATUS_TIMER_ENABLED;
    *(vu32 *)REG_TM3CNT = TIMER_FLAGS | gMA.timerInter;
}

/**
 * @brief Start session
 *
 * Start session without waking up the adapter first, as in MABIOS_Start(). Used
 * to restart the session when we know the adapter hasn't gone to sleep.
 */
void MABIOS_Start2(void)
{
    *(vu32 *)REG_TM3CNT = 0;
    gMA.condition &= ~MA_CONDITION_BIOS_BUSY;
    if (!MA_PreSend()) return;

    // Not returning reply to caller, use internal reception buffer
    SetInternalRecvBuffer();

    // Initialize transaction
    gMA.condition |= MA_CONDITION_BIOS_BUSY;
    gMA.sendCmd = MACMD_START;

    // Fill packet buffer
    if (gMA.sioMode == MA_SIO_BYTE) {
        MA_InitIoBuffer(&gMA.sendIoBuf, (u8 *)MaPacketData_Start,
            sizeof(MaPacketData_Start) - 2, IOBUF_SEND_DATA);
    } else {
        MA_InitIoBuffer(&gMA.sendIoBuf, (u8 *)MaPacketData_Start,
            sizeof(MaPacketData_Start), IOBUF_SEND_DATA);
    }

    // Configure timer and start transaction
    gMA.intrMode = MA_INTR_SEND;
    gMA.timerInter = gMA.timerIntInter[gMA.sioMode];
    MA_SetTimeoutCount(TIMEOUT_02);
    gMA.status |= STATUS_TIMER_ENABLED;
}

/**
 * @brief End session
 *
 * Finalizes the connection with the adapter.
 */
void MABIOS_End(void)
{
    tmppPacket = gMA.sendPacket;
    if (!MA_PreSend()) return;

    // Not returning reply to caller, use internal reception buffer
    SetInternalRecvBuffer();

    // Fill packet buffer
    tmpPacketLen = MA_CreatePacket(tmppPacket, MACMD_END, 0);
    MA_InitIoBuffer(&gMA.sendIoBuf, gMA.sendPacket, tmpPacketLen,
        IOBUF_SEND_DATA);

    // Initialize transaction
    gMA.sendCmd = MACMD_END;
    gMA.condition |= MA_CONDITION_BIOS_BUSY;

    // Configure timer and start transaction
    gMA.intrMode = MA_INTR_SEND;
    gMA.timerInter = gMA.timerIntInter[gMA.sioMode];
    MA_SetTimeoutCount(TIMEOUT_30);
    gMA.status |= STATUS_TIMER_ENABLED;
}

/**
 * @brief Dial a telephone pNumber and connect to it
 *
 * Starts a call with a phone, allowing data to be transmitted between both
 * devices. This can be used to call an ISP or a different adapter.
 *
 * The callType parameter is related to the adapter type, and doesn't specify
 * what kind of connection will be established.
 *
 * @param[in] callType call type
 * @param[in] pNumber phone pNumber
 */
void MABIOS_Tel(u8 callType, const char *pNumber)
{
    static int telNoLen;

    tmppPacket = gMA.sendPacket;
    if (!MA_PreSend()) return;

    // Not returning reply to caller, use internal reception buffer
    SetInternalRecvBuffer();

    // Initialize transaction
    gMA.condition |= MA_CONDITION_BIOS_BUSY;
    gMA.sendCmd = MACMD_TEL;

    // Fill packet buffer
    tmppPacket[MAPROT_HEADER_SIZE + 0] = callType;
    telNoLen = 0;
    while (*pNumber != '\0') {
        // FAKEMATCH
        u8 *p = tmppPacket;
        int n = telNoLen + 1;
        *(p + n + MAPROT_HEADER_SIZE) = *pNumber++;
        telNoLen = n;
    }
    tmpPacketLen = MA_CreatePacket(tmppPacket, MACMD_TEL, telNoLen + 1);
    MA_InitIoBuffer(&gMA.sendIoBuf, gMA.sendPacket, tmpPacketLen,
        IOBUF_SEND_DATA);

    // Configure timer and start transaction
    gMA.intrMode = MA_INTR_SEND;
    gMA.timerInter = gMA.timerIntInter[gMA.sioMode];
    MA_SetTimeoutCount(TIMEOUT_90);
    gMA.status |= STATUS_TIMER_ENABLED;
}

/**
 * @brief Disconnect phone line
 *
 * Ends all communications with the phone connected through MABIOS_Tel() or
 * MABIOS_WaitCall(). This implicitly destroys any ISP and TCP/UDP connections.
 */
void MABIOS_Offline(void)
{
    tmppPacket = gMA.sendPacket;
    if (!MA_PreSend()) return;

    // Not returning reply to caller, use internal reception buffer
    SetInternalRecvBuffer();

    // Fill packet buffer
    tmpPacketLen = MA_CreatePacket(tmppPacket, MACMD_OFFLINE, 0);
    MA_InitIoBuffer(&gMA.sendIoBuf, gMA.sendPacket, tmpPacketLen,
        IOBUF_SEND_DATA);

    // Initialize transaction
    gMA.sendCmd = MACMD_OFFLINE;
    gMA.condition |= MA_CONDITION_BIOS_BUSY;

    // Configure timer and start transaction
    gMA.intrMode = MA_INTR_SEND;
    gMA.timerInter = gMA.timerIntInter[gMA.sioMode];
    MA_SetTimeoutCount(TIMEOUT_30);
    gMA.status |= STATUS_TIMER_ENABLED;
}

/**
 * @brief Wait for phone call and connect to it
 *
 * Checks if a phone call is being received, if so, starts a connection with the
 * calling device.
 */
void MABIOS_WaitCall(void)
{
    tmppPacket = gMA.sendPacket;
    if (!MA_PreSend()) return;

    // Not returning reply to caller, use internal reception buffer
    SetInternalRecvBuffer();

    // Fill packet buffer
    tmpPacketLen = MA_CreatePacket(tmppPacket, MACMD_WAIT_CALL, 0);
    MA_InitIoBuffer(&gMA.sendIoBuf, gMA.sendPacket, tmpPacketLen,
        IOBUF_SEND_DATA);

    // Initialize transaction
    gMA.sendCmd = MACMD_WAIT_CALL;
    gMA.condition |= MA_CONDITION_BIOS_BUSY;

    // Configure timer and start transaction
    gMA.intrMode = MA_INTR_SEND;
    gMA.timerInter = gMA.timerIntInter[gMA.sioMode];
    MA_SetTimeoutCount(TIMEOUT_30);
    gMA.status |= STATUS_TIMER_ENABLED;
}

/**
 * @brief Send/receive data through a socket
 *
 * Sends and receives data to/from the specified socket parameter. This socket
 * may be opened with MABIOS_TCPConnect() or MABIOS_UDPConnect().
 *
 * If the adapter is connected to a phone line, but hasn't initialized a session
 * with the ISP yet, this function may also be used to communicate over the
 * phone line, in which case the socket parameter is ignored, though this
 * function is primarily intended for TCP/UDP connections.
 *
 * @param[out] pRecvBuf received data
 * @param[in] pSendData data to send
 * @param[in] sendSize size of data to send
 * @param[in] socket socket number
 */
void MABIOS_Data(MA_BUF *pRecvBuf, const u8 *pSendData, u8 sendSize, u8 socket)
{
    tmppPacket = gMA.sendPacket;
    if (!MA_PreSend()) return;

    // Return reply data to caller
    gMA.pRecvBuf = pRecvBuf;

    // Initialize transaction
    gMA.condition |= MA_CONDITION_BIOS_BUSY;
    gMA.sendCmd = MACMD_DATA;

    // Fill packet buffer
    tmppPacket[MAPROT_HEADER_SIZE + 0] = socket;
    for (i = 0; i < sendSize; i++) {
        // FAKEMATCH
        u8 *p = tmppPacket;
        int n = i + 1;
        *(p + n + MAPROT_HEADER_SIZE) = *pSendData++;
    }
    tmpPacketLen = MA_CreatePacket(tmppPacket, MACMD_DATA, sendSize + 1);
    MA_InitIoBuffer(&gMA.sendIoBuf, gMA.sendPacket, tmpPacketLen,
        IOBUF_SEND_DATA);

    // Configure timer and start transaction
    gMA.intrMode = MA_INTR_SEND;
    gMA.timerInter = gMA.timerIntInter[gMA.sioMode];
    MA_SetTimeoutCount(TIMEOUT_30);
    gMA.status |= STATUS_TIMER_ENABLED;
}

/**
 * @brief Send/receive data over a phone line
 *
 * Counterpart to MABIOS_Data(), but only for connections to other adapters.
 * Implicitly sets the socket as 0xff and prepends a size byte to the sent data.
 *
 * @param[out] pRecvBuf received data
 * @param[in] pSendData data to send
 * @param[in] sendSize size of data to send
 */
static void MABIOS_Data2(MA_BUF *pRecvBuf, const u8 *pSendData, u8 sendSize)
{
    tmppPacket = gMA.sendPacket;
    if (!MA_PreSend()) return;

    // Return reply data to caller
    gMA.pRecvBuf = pRecvBuf;

    // Initialize transaction
    gMA.condition |= MA_CONDITION_BIOS_BUSY;
    gMA.sendCmd = MACMD_DATA;

    // Fill packet buffer
    tmppPacket[MAPROT_HEADER_SIZE + 0] = 0xff;
    tmppPacket[MAPROT_HEADER_SIZE + 1] = sendSize;
    for (i = 0; i < sendSize; i++) {
        // FAKEMATCH
        u8 *p = tmppPacket;
        int n = i + 2;
        *(p + n + MAPROT_HEADER_SIZE) = *pSendData++;
    }
    tmpPacketLen = MA_CreatePacket(tmppPacket, MACMD_DATA, sendSize + 2);
    MA_InitIoBuffer(&gMA.sendIoBuf, gMA.sendPacket, tmpPacketLen,
        IOBUF_SEND_DATA);

    // Configure timer and start transaction
    gMA.intrMode = MA_INTR_SEND;
    gMA.timerInter = gMA.timerIntInter[gMA.sioMode];
    MA_SetTimeoutCount(TIMEOUT_30);
    gMA.status |= STATUS_TIMER_ENABLED;
}

/**
 * @brief Reinitialize the adapter
 *
 * Equivalent to a MABIOS_End() + MABIOS_Start2(). Closes any connections and
 * returns to 8-bit serial mode.
 *
 * Unused.
 */
void MABIOS_ReInit(void)
{
    tmppPacket = gMA.sendPacket;
    if (!MA_PreSend()) return;

    // Not returning reply to caller, use internal reception buffer
    SetInternalRecvBuffer();

    // Fill packet buffer
    tmpPacketLen = MA_CreatePacket(tmppPacket, MACMD_REINIT, 0);
    MA_InitIoBuffer(&gMA.sendIoBuf, gMA.sendPacket, tmpPacketLen,
        IOBUF_SEND_DATA);

    // Initialize transaction
    gMA.sendCmd = MACMD_REINIT;
    gMA.condition |= MA_CONDITION_BIOS_BUSY;

    // Configure timer and start transaction
    gMA.intrMode = MA_INTR_SEND;
    gMA.timerInter = gMA.timerIntInter[gMA.sioMode];
    MA_SetTimeoutCount(TIMEOUT_30);
    gMA.status |= STATUS_TIMER_ENABLED;
}

/**
 * @brief Get adapter status
 *
 * Query the adapter for its status, returns a buffer of three bytes.
 *
 * @param[out] pRecvBuf received data
 */
void MABIOS_CheckStatus(MA_BUF *pRecvBuf)
{
    tmppPacket = gMA.sendPacket;
    if (!MA_PreSend()) return;

    // Return reply data to caller
    gMA.pRecvBuf = pRecvBuf;

    // Fill packet buffer
    tmpPacketLen = MA_CreatePacket(tmppPacket, MACMD_CHECK_STATUS, 0);
    MA_InitIoBuffer(&gMA.sendIoBuf, gMA.sendPacket, tmpPacketLen,
        IOBUF_SEND_DATA);

    // Initialize transaction
    gMA.sendCmd = MACMD_CHECK_STATUS;
    gMA.condition |= MA_CONDITION_BIOS_BUSY;

    // Configure timer and start transaction
    gMA.intrMode = MA_INTR_SEND;
    gMA.timerInter = gMA.timerIntInter[gMA.sioMode];
    gMA.status |= STATUS_TIMER_ENABLED;
    MA_SetTimeoutCount(TIMEOUT_30);
}

/**
 * @brief Get adapter status
 *
 * Counterpart to MABIOS_CheckStatus() used by the bios functions internally to
 * test whether the adapter is still connected.
 *
 * @param[out] pRecvBuf received data
 */
void MABIOS_CheckStatus2(MA_BUF *pRecvBuf)
{
    tmppPacket = gMA.sendPacket;

    // Make sure we're connected
    if (!(gMA.status & STATUS_CONNECTED) || gMA.status & STATUS_CONNTEST) {
        return;
    }

    // Return reply data to caller
    gMA.pRecvBuf = pRecvBuf;

    // Fill packet buffer
    tmpPacketLen = MA_CreatePacket(tmppPacket, MACMD_CHECK_STATUS, 0);
    MA_InitIoBuffer(&gMA.sendIoBuf, gMA.sendPacket, tmpPacketLen,
        IOBUF_SEND_DATA);

    // Initialize transaction
    gMA.sendCmd = MACMD_CHECK_STATUS;
    gMA.condition |= MA_CONDITION_BIOS_BUSY;

    // Configure timer and start transaction
    gMA.intrMode = MA_INTR_SEND;
    gMA.timerInter = gMA.timerIntInter[gMA.sioMode];
    MA_SetTimeoutCount(TIMEOUT_10);
    gMA.status |= STATUS_CONNTEST;
    gMA.status |= STATUS_TIMER_ENABLED;
}

/**
 * @brief Change serial mode
 *
 * Requests the adapter to change the serial mode, to either MA_SIO_BYTE or
 * MA_SIO_WORD. Usually called right after the session starts to upgrade to
 * WORD (32-bit) mode.
 *
 * The code in MA_ProcessRecvPacket() takes care of the switching of the
 * library's serial mode upon successful completion.
 *
 * @param[in] mode serial mode
 */
void MABIOS_ChangeClock(u8 mode)
{
    tmppPacket = gMA.sendPacket;
    if (!MA_PreSend()) return;

    // Not returning reply to caller, use internal reception buffer
    SetInternalRecvBuffer();

    // Initialize transaction
    gMA.condition |= MA_CONDITION_BIOS_BUSY;
    gMA.sendCmd = MACMD_CHANGE_CLOCK;

    // Fill packet buffer
    tmppPacket[MAPROT_HEADER_SIZE + 0] = mode;
    tmpPacketLen = MA_CreatePacket(tmppPacket, MACMD_CHANGE_CLOCK, 1);
    MA_InitIoBuffer(&gMA.sendIoBuf, gMA.sendPacket, tmpPacketLen,
        IOBUF_SEND_DATA);

    // Configure timer and start transaction
    gMA.intrMode = MA_INTR_SEND;
    gMA.timerInter = gMA.timerIntInter[gMA.sioMode];
    MA_SetTimeoutCount(TIMEOUT_30);
    gMA.status |= STATUS_TIMER_ENABLED;
}

/**
 * @brief Read EEPROM data
 *
 * Read data from the adapter's EEPROM storage. Used to store adapter
 * configuration data, required to establish an ISP connection and connect to
 * mail servers.
 *
 * The received data buffer is prefixed with one byte indicating the offset.
 *
 * @param[out] pRecvBuf received data
 * @param[in] offset eeprom offset
 * @param[in] size size of data to read
 */
void MABIOS_EEPROM_Read(MA_BUF *pRecvBuf, u8 offset, u8 size)
{
    tmppPacket = gMA.sendPacket;
    if (!MA_PreSend()) return;

    // Return reply data to caller
    gMA.pRecvBuf = pRecvBuf;

    // Initialize transaction
    gMA.condition |= MA_CONDITION_BIOS_BUSY;
    gMA.sendCmd = MACMD_EEPROM_READ;

    // Fill packet buffer
    tmppPacket[MAPROT_HEADER_SIZE + 0] = offset;
    tmppPacket[MAPROT_HEADER_SIZE + 1] = size;
    tmpPacketLen = MA_CreatePacket(tmppPacket, MACMD_EEPROM_READ, 2);
    MA_InitIoBuffer(&gMA.sendIoBuf, gMA.sendPacket, tmpPacketLen,
        IOBUF_SEND_DATA);

    // Configure timer and start transaction
    gMA.intrMode = MA_INTR_SEND;
    gMA.timerInter = gMA.timerIntInter[gMA.sioMode];
    MA_SetTimeoutCount(TIMEOUT_30);
    gMA.status |= STATUS_TIMER_ENABLED;
}

/**
 * @brief Write EEPROM data
 *
 * Writes data to the adapter's EEPROM storage.
 *
 * The received data buffer is never read.
 *
 * @param[out] pRecvBuf received data
 * @param[in] offset eeprom offset
 * @param[in] pSendData data to write
 * @param[in] size size of data to write
 */
void MABIOS_EEPROM_Write(MA_BUF *pRecvBuf, u8 offset, const u8 *pSendData,
    u8 size)
{
    tmppPacket = gMA.sendPacket;
    if (!MA_PreSend()) return;

    // Return reply data to caller
    gMA.pRecvBuf = pRecvBuf;

    // Initialize transaction
    gMA.condition |= MA_CONDITION_BIOS_BUSY;
    gMA.sendCmd = MACMD_EEPROM_WRITE;

    // Fill packet buffer
    tmppPacket[MAPROT_HEADER_SIZE + 0] = offset;
    for (i = 0; i < size; i++) {
        // FAKEMATCH
        u8 *p = tmppPacket;
        int n = i + 1;
        *(p + n + MAPROT_HEADER_SIZE) = *pSendData++;
    }
    tmpPacketLen = MA_CreatePacket(tmppPacket, MACMD_EEPROM_WRITE, size + 1);
    MA_InitIoBuffer(&gMA.sendIoBuf, gMA.sendPacket, tmpPacketLen,
        IOBUF_SEND_DATA);

    // Configure timer and start transaction
    gMA.intrMode = MA_INTR_SEND;
    gMA.timerInter = gMA.timerIntInter[gMA.sioMode];
    MA_SetTimeoutCount(TIMEOUT_30);
    gMA.status |= STATUS_TIMER_ENABLED;
}

/**
 * @brief Connect to the ISP
 *
 * Initializes a connection with the ISP, allowing access to the internet. This
 * function requires info that can be obtained from the EEPROM.
 *
 * This function should only be called after dialing the ISP number with
 * MABIOS_Tel().
 *
 * @param[out] pRecvBuf received data
 * @param[in] pUserID user ID
 * @param[in] pPassword user password
 * @param[in] dns1 primary dns address
 * @param[in] dns2 secondary dns address
 */
void MABIOS_PPPConnect(MA_BUF *pRecvBuf, const char *pUserID,
    const char *pPassword, const u8 *dns1, const u8 *dns2)
{
    static u8 *pData;
    static int dataLen;
    static int userIDLength;
    static int passwordLength;

    tmppPacket = gMA.sendPacket;
    pData = &tmppPacket[MAPROT_HEADER_SIZE];
    if (!MA_PreSend()) return;

    // Initialize transaction
    gMA.condition |= MA_CONDITION_BIOS_BUSY;
    gMA.sendCmd = MACMD_PPP_CONNECT;

    // Return reply data to caller
    gMA.pRecvBuf = pRecvBuf;

    // Determine packet data size
    userIDLength = MAU_strlen(pUserID);
    passwordLength = MAU_strlen(pPassword);
    dataLen = userIDLength + passwordLength + 10;

    // Fill packet buffer
    *pData++ = userIDLength;
    for (; userIDLength; userIDLength--) *pData++ = *pUserID++;
    *pData++ = passwordLength;
    for (; passwordLength; passwordLength--) *pData++ = *pPassword++;
    for (i = 0; i < 4; i++) *pData++ = *dns1++;
    for (i = 0; i < 4; i++) *pData++ = *dns2++;
    tmpPacketLen = MA_CreatePacket(tmppPacket, MACMD_PPP_CONNECT, dataLen);
    MA_InitIoBuffer(&gMA.sendIoBuf, gMA.sendPacket, tmpPacketLen,
        IOBUF_SEND_DATA);

    // Configure timer and start transaction
    gMA.intrMode = MA_INTR_SEND;
    gMA.timerInter = gMA.timerIntInter[gMA.sioMode];
    MA_SetTimeoutCount(TIMEOUT_30);
    gMA.status |= STATUS_TIMER_ENABLED;
}

/**
 * @brief Disconnect from the ISP
 */
void MABIOS_PPPDisconnect(void)
{
    tmppPacket = gMA.sendPacket;
    if (!MA_PreSend()) return;

    // Not returning reply to caller, use internal reception buffer
    SetInternalRecvBuffer();

    // Fill packet buffer
    tmpPacketLen = MA_CreatePacket(tmppPacket, MACMD_PPP_DISCONNECT, 0);
    MA_InitIoBuffer(&gMA.sendIoBuf, gMA.sendPacket, tmpPacketLen,
        IOBUF_SEND_DATA);

    // Initialize transaction
    gMA.sendCmd = MACMD_PPP_DISCONNECT;
    gMA.condition |= MA_CONDITION_BIOS_BUSY;

    // Configure timer and start transaction
    gMA.intrMode = MA_INTR_SEND;
    gMA.timerInter = gMA.timerIntInter[gMA.sioMode];
    MA_SetTimeoutCount(TIMEOUT_30);
    gMA.status |= STATUS_TIMER_ENABLED;
}

/**
 * @brief Connect to a TCP server
 *
 * Connects to a TCP server to transmit data. Once the connection is
 * established, MABIOS_Data() may be used to send and receive data.
 * expected to check whether the connection still exists through
 * MA_GetCondition() & MA_CONDITION_TCPCLOSED or similar, as the server may
 * close it at any point in time.
 *
 * The received data buffer contains the socket number assigned to the
 * connection.
 *
 * This function should only be called after connecting to the ISP with
 * MABIOS_PPPConnect().
 *
 * @param[out] pRecvBuf received data
 * @param[in] pAddr IPv4 server address
 * @param[in] port server port
 */
void MABIOS_TCPConnect(MA_BUF *pRecvBuf, u8 *pAddr, u16 port)
{
    tmppPacket = gMA.sendPacket;
    if (!MA_PreSend()) return;

    // Return reply data to caller
    gMA.pRecvBuf = pRecvBuf;

    // Initialize transaction
    gMA.condition |= MA_CONDITION_BIOS_BUSY;
    gMA.sendCmd = MACMD_TCP_CONNECT;

    // Fill packet buffer
    for (i = 0; i < 4; i++) {
        *(tmppPacket + i + MAPROT_HEADER_SIZE) = *pAddr++;
    }
    tmppPacket[MAPROT_HEADER_SIZE + 4 + 0] = ((u8 *)&port)[1];
    tmppPacket[MAPROT_HEADER_SIZE + 4 + 1] = ((u8 *)&port)[0];
    tmpPacketLen = MA_CreatePacket(tmppPacket, MACMD_TCP_CONNECT, 6);
    MA_InitIoBuffer(&gMA.sendIoBuf, gMA.sendPacket, tmpPacketLen,
        IOBUF_SEND_DATA);

    // Configure timer and start transaction
    gMA.intrMode = MA_INTR_SEND;
    gMA.timerInter = gMA.timerIntInter[gMA.sioMode];
    MA_SetTimeoutCount(TIMEOUT_30);
    gMA.tcpConnectRetryCount = 0;
    gMA.status |= STATUS_TIMER_ENABLED;
}

/**
 * @brief Disconnect from a TCP server
 *
 * The received data buffer is never read.
 *
 * @param pRecvBuf received data
 * @param socket socket number
 */
void MABIOS_TCPDisconnect(MA_BUF *pRecvBuf, u8 socket)
{
    tmppPacket = gMA.sendPacket;
    if (!MA_PreSend()) return;

    // Return reply data to caller
    gMA.pRecvBuf = pRecvBuf;

    // Initialize transaction
    gMA.condition |= MA_CONDITION_BIOS_BUSY;
    gMA.sendCmd = MACMD_TCP_DISCONNECT;

    // Fill packet buffer
    tmppPacket[MAPROT_HEADER_SIZE + 0] = socket;
    tmpPacketLen = MA_CreatePacket(tmppPacket, MACMD_TCP_DISCONNECT, 1);
    MA_InitIoBuffer(&gMA.sendIoBuf, gMA.sendPacket, tmpPacketLen,
        IOBUF_SEND_DATA);

    // Configure timer and start transaction
    gMA.intrMode = MA_INTR_SEND;
    gMA.timerInter = gMA.timerIntInter[gMA.sioMode];
    MA_SetTimeoutCount(TIMEOUT_30);
    gMA.status |= STATUS_TIMER_ENABLED;
}

/**
 * @brief Create a UDP socket
 *
 * Initializes a socket as UDP with a given destination address, since UDP
 * doesn't keep track of connection state.
 *
 * The received data buffer contains the socket number assigned to the
 * connection.
 *
 * This function should only be called after connecting to the ISP with
 * MABIOS_PPPConnect().
 *
 * Unused.
 *
 * @param[out] pRecvBuf received data
 * @param[in] pAddr IPv4 destination address
 * @param[in] port destination port
 */
void MABIOS_UDPConnect(MA_BUF *pRecvBuf, u8 *pAddr, u16 port)
{
    tmppPacket = gMA.sendPacket;
    if (!MA_PreSend()) return;

    // Return reply data to caller
    gMA.pRecvBuf = pRecvBuf;

    // Initialize transaction
    gMA.condition |= MA_CONDITION_BIOS_BUSY;
    gMA.sendCmd = MACMD_UDP_CONNECT;

    // Fill packet buffer
    for (i = 0; i < 4; i++) {
        *(tmppPacket + i + MAPROT_HEADER_SIZE) = *pAddr++;
    }
    tmppPacket[MAPROT_HEADER_SIZE + 4 + 0] = ((u8 *)&port)[1];
    tmppPacket[MAPROT_HEADER_SIZE + 4 + 1] = ((u8 *)&port)[0];
    tmpPacketLen = MA_CreatePacket(tmppPacket, MACMD_UDP_CONNECT, 6);
    MA_InitIoBuffer(&gMA.sendIoBuf, gMA.sendPacket, tmpPacketLen,
        IOBUF_SEND_DATA);

    // Configure timer and start transaction
    gMA.intrMode = MA_INTR_SEND;
    gMA.timerInter = gMA.timerIntInter[gMA.sioMode];
    MA_SetTimeoutCount(TIMEOUT_30);
    gMA.status |= STATUS_TIMER_ENABLED;
}

/**
 * @brief Destroy a UDP socket
 *
 * Unused.
 *
 * @param[out] pRecvBuf received data
 * @param[in] socket socket number
 */
void MABIOS_UDPDisconnect(MA_BUF *pRecvBuf, u8 socket)
{
    tmppPacket = gMA.sendPacket;
    if (!MA_PreSend()) return;

    // Return reply data to caller
    gMA.pRecvBuf = pRecvBuf;

    // Initialize transaction
    gMA.condition |= MA_CONDITION_BIOS_BUSY;
    gMA.sendCmd = MACMD_UDP_DISCONNECT;

    // Fill packet buffer
    tmppPacket[MAPROT_HEADER_SIZE + 0] = socket;
    tmpPacketLen = MA_CreatePacket(tmppPacket, MACMD_UDP_DISCONNECT, 1);
    MA_InitIoBuffer(&gMA.sendIoBuf, gMA.sendPacket, tmpPacketLen,
        IOBUF_SEND_DATA);

    // Configure timer and start transaction
    gMA.intrMode = MA_INTR_SEND;
    gMA.timerInter = gMA.timerIntInter[gMA.sioMode];
    MA_SetTimeoutCount(TIMEOUT_30);
    gMA.status |= STATUS_TIMER_ENABLED;
}

/**
 * @brief Query IP address of a server name through DNS
 *
 * Queries the configured DNS servers for the IPv4 address associated with a
 * given server name. This is useful to figure out the address to use in
 * MABIOS_TCPConnect().
 *
 * The received data buffer contains the resulting IP address.
 *
 * This function should only be called after connecting to the ISP with
 * MABIOS_PPPConnect().
 *
 * @param[out] pRecvBuf received data
 * @param[in] pServerName server name to query
 */
void MABIOS_DNSRequest(MA_BUF *pRecvBuf, char *pServerName)
{
    static int serverNameLen;

    tmppPacket = gMA.sendPacket;
    if (!MA_PreSend()) return;

    // Return reply data to caller
    gMA.pRecvBuf = pRecvBuf;

    // Initialize transaction
    gMA.condition |= MA_CONDITION_BIOS_BUSY;
    gMA.sendCmd = MACMD_DNS_REQUEST;

    // Fill packet buffer
    serverNameLen = 0;
    while (*pServerName != '\0') {
        // FAKEMATCH
        u8 *p = tmppPacket;
        int n = serverNameLen;
        *(p + n + MAPROT_HEADER_SIZE) = *pServerName++;
        serverNameLen = n + 1;
    }
    tmpPacketLen = MA_CreatePacket(tmppPacket, MACMD_DNS_REQUEST, serverNameLen);
    MA_InitIoBuffer(&gMA.sendIoBuf, gMA.sendPacket, tmpPacketLen,
        IOBUF_SEND_DATA);

    // Configure timer and start transaction
    gMA.intrMode = MA_INTR_SEND;
    gMA.timerInter = gMA.timerIntInter[gMA.sioMode];
    MA_SetTimeoutCount(TIMEOUT_30);
    gMA.status |= STATUS_TIMER_ENABLED;
}

/**
 * @brief Enter test mode
 *
 * Unused.
 */
void MABIOS_TestMode(void)
{
    tmppPacket = gMA.sendPacket;
    if (!MA_PreSend()) return;

    // Not returning reply to caller, use internal reception buffer
    SetInternalRecvBuffer();

    // Fill packet buffer
    tmpPacketLen = MA_CreatePacket(tmppPacket, MACMD_TEST_MODE, 0);
    MA_InitIoBuffer(&gMA.sendIoBuf, gMA.sendPacket, tmpPacketLen,
        IOBUF_SEND_DATA);

    // Initialize transaction
    gMA.sendCmd = MACMD_TEST_MODE;
    gMA.condition |= MA_CONDITION_BIOS_BUSY;

    // Configure timer and start transaction
    gMA.intrMode = MA_INTR_SEND;
    gMA.timerInter = gMA.timerIntInter[gMA.sioMode];
    MA_SetTimeoutCount(TIMEOUT_30);
    gMA.status |= STATUS_TIMER_ENABLED;
}

/**
 * @brief Finalize packet to send
 *
 * Initializes the header and footer of a packet with a given size, calculating
 * the checksum along the way. Expects a packet buffer already containing data
 * at the correct offset (MAPROT_HEADER_SIZE). The total size of the packet will
 * depend on the serial mode, to account for padding in the 32-bit mode.
 *
 * @param[in,out] packet pre-filled packet
 * @param[in] cmd command to write in the header
 * @param[in] size size of packet data
 * @return total size
 */
static int MA_CreatePacket(u8 *packet, u8 cmd, u16 size)
{
    if (gMA.sioMode == MA_SIO_WORD) {
        return MA_Create32BitPacket(packet, cmd, size);
    } else {
        return MA_Create8BitPacket(packet, cmd, size);
    }
}

/**
 * @brief Finalize an 8-bit packet
 *
 * 8-bit branch of MA_CreatePacket().
 *
 * @param[in,out] packet pre-filled packet
 * @param[in] cmd command to write in the header
 * @param[in] size size of packet data
 * @return total size
 */
static int MA_Create8BitPacket(u8 *packet, u8 cmd, u16 size)
{
    static u16 checkSum;

    // Initialize the header with the appropriate values
    MAPROT_HEADER *p = (MAPROT_HEADER *)packet;
    p->magic = 0x6699;
    p->cmd = cmd;
    p->pad = 0;
    p->size = (size & 0x00ff) << 8 | (size & 0xff00) >> 8;  // big endian

    // Initialize the footer, calculating the checksum
    tmppPacketLast = (MAPROT_FOOTER *)(packet + MAPROT_HEADER_SIZE + size);
    checkSum = MA_CalcCheckSum(packet + 2, size + MAPROT_HEADER_SIZE - 2);
    tmppPacketLast->checkSum_H = checkSum >> 8;
    tmppPacketLast->checkSum_L = checkSum >> 0;
    tmppPacketLast->device = MAPROT_TYPE_MASTER | MATYPE_GBA;
    tmppPacketLast->pad[0] = 0;

    return size + MAPROT_HEADER_SIZE + MAPROT_FOOTER_SIZE - 2;
}

/**
 * @brief Finalize a 32-bit packet
 *
 * 32-bit branch of MA_CreatePacket().
 *
 * @param[in,out] packet pre-filled packet
 * @param[in] cmd command to write in the header
 * @param[in] size size of packet data
 * @return total size
 */
static int MA_Create32BitPacket(u8 *packet, u8 cmd, u16 size)
{
    static u8 *pPadding;
    static int paddingLength;
    static int amari;
    static u16 checkSum;

    // Initialize the header with the appropriate values
    MAPROT_HEADER *p = (MAPROT_HEADER *)packet;
    p->magic = 0x6699;
    p->cmd = cmd;
    p->pad = 0;
    p->size = (size & 0x00ff) << 8 | (size & 0xff00) >> 8;

    // Calculate the padding for the packet data, aligning the footer to 4 bytes
    if (p->size == 0) {
        paddingLength = 0;
    } else {
        amari = size % 4;
        if (amari == 0) {
            paddingLength = 0;
        } else {
            paddingLength = 4 - amari;
        }

        // Zero-fill the padding bytes
        pPadding = packet + MAPROT_HEADER_SIZE + size;
        for (i = 0; i < paddingLength; i++) *pPadding++ = 0;
    }

    // Initialize the footer, calculating the checksum
    tmppPacketLast =
        (MAPROT_FOOTER *)(packet + MAPROT_HEADER_SIZE + size + paddingLength);
    checkSum = MA_CalcCheckSum(packet + 2, size + MAPROT_HEADER_SIZE - 2);
    tmppPacketLast->checkSum_H = checkSum >> 8;
    tmppPacketLast->checkSum_L = checkSum >> 0;
    tmppPacketLast->device = MAPROT_TYPE_MASTER | MATYPE_GBA;
    tmppPacketLast->pad[0] = 0;
    tmppPacketLast->pad[1] = 0;
    tmppPacketLast->pad[2] = 0;

    return size + paddingLength + MAPROT_HEADER_SIZE + MAPROT_FOOTER_SIZE;
}

/**
 * @brief Calculate packet checksum
 *
 * Simple additive checksum. Calculated separately for received packets in
 * MA_IntrSio_Recv().
 *
 * @param pData data to checksum
 * @param size size of data
 * @return checksum
 */
static u16 MA_CalcCheckSum(u8 *pData, u16 size)
{
    static u16 sum;

    sum = 0;
    while (size != 0) {
        sum = sum + *pData++;
        size--;
    }
    return sum;
}

/**
 * @brief Cancel the currently processing request
 *
 * This is implemented by waiting just enough time for the adapter to reset
 * itself, before sending the start command again.
 */
void MA_CancelRequest(void)
{
    gMA.condition |= MA_CONDITION_BIOS_BUSY;
    gMA.status |= STATUS_BIOS_RESTART;
    gMA.timerInter = TIMER_COUNTER_MS(2500);
    gMA.intrMode = MA_INTR_WAIT;
}

/**
 * @brief Stop processing the current command
 *
 * Sets the interrupt into idle mode, to keep the connection alive.
 */
void MA_BiosStop(void)
{
    gMA.condition |= MA_CONDITION_BIOS_BUSY;
    gMA.status |= STATUS_BIOS_STOP;
    gMA.timerInter = 0;
    gMA.intrMode = MA_INTR_WAIT;
    gMA.status &= ~STATUS_BIOS_RESTART;
}

/**
 * @brief Re-initialize the send buffer with the same data
 *
 * Inmediately triggers a send.
 */
void MA_SendRetry(void)
{
    MA_InitIoBuffer(&gMA.sendIoBuf, gMA.sendIoBuf.pWrite, gMA.sendIoBuf.size,
        IOBUF_SEND_DATA);
}

/**
 * @brief Re-initialize the reception buffer with the same pointer
 *
 * Inmediately triggers the reception of a packet.
 */
void MA_RecvRetry(void)
{
    MA_InitIoBuffer(&gMA.recvIoBuf, gMA.recvIoBuf.pWrite, 0, IOBUF_RECV_START);
    gMA.recvRubbishCount = 0;
    gMA.status &= ~STATUS_SIO_RETRY;
    gMA.intrMode = MA_INTR_RECV;
    gMA.sendIoBuf.state = IOBUF_SEND_IDLE;
    gMA.negaResCmd = 0;
    gMA.negaResErr = 0;
}

/**
 * @brief Timer interrupt handler for MA_INTR_SEND
 *
 * Sends the requested data, depending on the iobuf state.
 */
static void MA_IntrTimer_SIOSend(void)
{
    switch (gMA.sendIoBuf.state) {
    // Send byte or word in buffer, wait a bit and move to IOBUF_SEND_START
    case IOBUF_SEND_PRESTART:
        MA_SetTransmitData(&gMA.sendIoBuf);
        gMA.sendIoBuf.state = IOBUF_SEND_START;
        gMA.timerInter = TIMER_COUNTER_MS(120);
        break;

    // Send MaPacketData_Start
    case IOBUF_SEND_START:
        MA_InitIoBuffer(&gMA.sendIoBuf, (u8 *)MaPacketData_Start,
            sizeof(MaPacketData_Start) - 2, IOBUF_SEND_DATA);
        gMA.timerInter = gMA.timerIntInter[gMA.sioMode];
        MA_SetTransmitData(&gMA.sendIoBuf);
        break;

    // Send packet in buffer
    case IOBUF_SEND_DATA:
        MA_SetTransmitData(&gMA.sendIoBuf);
        break;
    }
}

/**
 * @brief Timer interrupt handler for MA_INTR_RECV
 *
 * When at any stage of receiving a packet, sends dummy bytes by initializing
 * the send buffer to the same dummy byte buffer. This triggers
 * MA_IntrSerialIO() which then handles the received bytes.
 *
 * The only exception being the footer, which is expected to be filled by
 * MA_IntrSerialIO() and won't be overridden here.
 */
static void MA_IntrTimer_SIORecv(void)
{
    switch (gMA.recvIoBuf.state) {
    // Do nothing
    case IOBUF_RECV_IDLE:
        break;

    // Receiving any part of a packet
    case IOBUF_RECV_START:
    case IOBUF_RECV_HEADER:
    case IOBUF_RECV_DATA:
    case IOBUF_RECV_CHECKSUM:
        if (gMA.sioMode == MA_SIO_BYTE) {
            MA_InitIoBuffer(&gMA.tempIoBuf, (u8 *)MaPacketData_PreStart, 1, 0);
        } else {
            MA_InitIoBuffer(&gMA.tempIoBuf, (u8 *)MaPacketData_PreStart, 4, 0);
        }
        MA_SetTransmitData(&gMA.tempIoBuf);
        break;

    // Receiving the footer
    case IOBUF_RECV_FOOTER:
        MA_SetTransmitData(&gMA.tempIoBuf);
        break;
    }
}

/**
 * @brief Timer interrupt handler for MA_INTR_IDLE
 *
 * MA_INTR_IDLE is the mode in which the timer interrupt is enabled, but isn't
 * doing anything of note. This mode handles periodically checking whether the
 * adapter is still connected, as well as periodically initiating a p2p transfer
 * when the adapter is in that mode.
 */
static void MA_IntrTimer_SIOIdle(void)
{
    // Don't run if any task is currently running, as this could override what
    // the API is currently doing
    if (gMA.task != TASK_NONE
        && gMA.task != TASK_SDATA
        && gMA.task != TASK_GDATA) {
        return;
    }

    // If the adapter isn't connected, there's nothing to do at all
    if (!(gMA.status & STATUS_CONNECTED)) return;

    gMA.counter++;

    // In p2p mode, try to send and receive data
    if (gMA.status & STATUS_CONN_PTP
        && (!(gMA.condition & STATUS_SIO_RETRY) || gMA.status & STATUS_PTP_SEND)) {
        if (gMA.counter > gMA.P2PCounter[gMA.sioMode]) {
            gMA.counter = 0;
            MA_InitBuffer(&gMA.recvBuf, gMA.recvPacket);
            if (gMA.status & STATUS_PTP_SEND) {
                MABIOS_Data2(&gMA.recvBuf, gMA.param.p2p.pSendData,
                    gMA.param.p2p.sendSize);
                gMA.param.p2p.pSendData = NULL;
                gMA.param.p2p.sendSize = 0;
                gMA.status |= STATUS_PTP_SEND_DONE;
            } else {
                MABIOS_Data(&gMA.recvBuf, NULL, 0, 0xff);
            }
        }
        return;
    }

    // Periodically check if the adapter is still connected
    if (gMA.counter > gMA.nullCounter[gMA.sioMode]) {
        gMA.counter = 0;
        MA_InitBuffer(&gMA.biosRecvBuf, gMA.hwCondition);
        MABIOS_CheckStatus2(&gMA.biosRecvBuf);
    }
}

/**
 * @brief Timer interrupt handler for MA_INTR_WAIT
 *
 * This handler is used to implement delays with the timer. This is useful to
 * give the adapter some time to breathe between actions like retries, startup
 * and such.
 *
 * Example usage:
 * @code
 * // Wait 1 second before returning control to the high-level api
 * gMA.timerInter = TIMER_INTER_MS(1000);
 * gMA.intrMode = MA_INTR_WAIT;
 * @endcode
 */
static void MA_IntrTimer_SIOWaitTime(void)
{
    if (gMA.status & STATUS_BIOS_RESTART) {
        // Restart the connection after the delay
        MABIOS_Start2();
    } else if (gMA.status & STATUS_BIOS_STOP) {
        // Go into idle mode
        gMA.status &= ~STATUS_BIOS_STOP;
        gMA.condition &= ~MA_CONDITION_BIOS_BUSY;
        gMA.intrMode = MA_INTR_IDLE;

        if (gMA.status & STATUS_SIO_SEND_TIMEOUT) {
            MA_ChangeSIOMode(MA_SIO_BYTE);
            MA_Reset();
            MA_TaskSet(TASK_NONE, 0);
            if (gMA.task != TASK_STOP) {
                gMA.status &= ~STATUS_SIO_SEND_TIMEOUT;
                MA_SetError(MAAPIE_TIMEOUT);
                gMA.counter = 0;
                gMA.intrMode = MA_INTR_IDLE;
            }
        }
    } else if (gMA.status & STATUS_SIO_RETRY) {
        // Retry sending after the delay
        gMA.intrMode = MA_INTR_SEND;
        MA_SendRetry();
    } else {
        // If we were waiting without a specific purpose, signal the api it
        // can continue now.
        gMA.intrMode = MA_INTR_IDLE;
        gMA.pRecvBuf->size = gMA.recvIoBuf.size;
        gMA.counter = 0;
        gMA.condition &= ~MA_CONDITION_BIOS_BUSY;
    }

    // Restore the original timer interval
    gMA.timerInter = gMA.timerIntInter[gMA.sioMode];
}

/**
 * @brief Interpret CheckStatus response
 *
 * Checks the status byte reported by MACMD_CHECK_STATUS, and checks the internal
 * state to return a suitable MA_CONDITION value. Handles errors by resetting
 * the library as well.
 *
 * @param[in] response status code
 * @return condition code
 */
int MA_ProcessCheckStatusResponse(u8 response)
{
    int condition = MA_CONDITION_IDLE;

    switch (response) {
    // The phone isn't connected
    case 0xff:
        condition = MA_CONDITION_LOST;
        if (gMA.connMode != CONN_OFFLINE) {
            if (gMA.status & STATUS_CONNTEST) {
                gMA.status = 0;
                MA_SetError(MAAPIE_OFFLINE);
            } else {
                gMA.status = 0;
            }
            MA_Reset();
        }
        break;

    // Phone line is not busy
    case 0:
    case 1:
        // If there was an active connection, this means the user manually hung
        // up. The only thing that can be done here is resetting the library.
        if (gMA.connMode != CONN_OFFLINE) {
            if (gMA.status & STATUS_CONNTEST) {
                gMA.status = 0;
                MA_SetError(MAAPIE_OFFLINE);
            } else {
                gMA.status = 0;
            }
            MA_Reset();
        }

    // Phone line is busy (in a call)
    case 5:
    case 4:
        switch (gMA.connMode) {
        case CONN_PPP: condition = MA_CONDITION_PPP; break;
        case CONN_SMTP: condition = MA_CONDITION_SMTP; break;
        case CONN_POP3: condition = MA_CONDITION_POP3; break;
        case CONN_P2P_SEND: condition = MA_CONDITION_P2P_SEND; break;
        case CONN_P2P_RECV: condition = MA_CONDITION_P2P_RECV; break;
        }
        break;
    }

    MA_SetCondition(condition);
    gMA.intrMode = MA_INTR_IDLE;
    gMA.sendIoBuf.state = IOBUF_SEND_IDLE;
    gMA.recvIoBuf.state = IOBUF_RECV_IDLE;
    gMA.status &= ~STATUS_CONNTEST;

    return condition;
}

/**
 * @brief Create generic API error from low-level protocol error
 *
 * Handles the MACMD_ERROR values for most of the commands, where
 * MA_DefaultNegaResProc() handles the rest of them. The resulting API errors
 * can be too generic compared to their actual meanings, but most of them can't
 * be handled anyway...
 */
static void ConvertNegaErrToApiErr(void)
{
    static const u8 errTable[] = {
        MAAPIE_SYSTEM,
        MAAPIE_CANNOT_EXECUTE_LOW,
        MAAPIE_ILLEGAL_PARAMETER_LOW,
        MAAPIE_CONNECT,
        MAAPIE_CONNECT,
    };

    gMA.taskError = errTable[gMA.negaResErr];
    gMA.taskErrorDetail = 0;
}

/**
 * @brief Create API error from low-level protocol error
 *
 * Handles MACMD_ERROR values by converting them to something understood by the
 * library users.
 */
void MA_DefaultNegaResProc(void)
{
    switch (gMA.negaResCmd) {
    case MACMD_TEL:
        switch (gMA.negaResErr) {
        case 0: gMA.taskError = MAAPIE_BUSY; break;
        case 1: gMA.taskError = MAAPIE_CONNECT; break;
        case 2: gMA.taskError = MAAPIE_ILLEGAL_PARAMETER_LOW; break;
        case 3: gMA.taskError = MAAPIE_CONNECT; break;
        case 4: gMA.taskError = MAAPIE_CONNECT; break;
        }
        gMA.taskErrorDetail = 0;
        break;

    case MACMD_START:
    case MACMD_WAIT_CALL:
    case MACMD_DATA:
    case MACMD_CHECK_STATUS:
    case MACMD_CHANGE_CLOCK:
    case MACMD_EEPROM_READ:
    case MACMD_EEPROM_WRITE:
    case MACMD_PPP_CONNECT:
    case MACMD_TCP_CONNECT:
    case MACMD_DNS_REQUEST:
        ConvertNegaErrToApiErr();
        break;
    }
}

/**
 * @brief Initial handling of received commands
 *
 * Handles the basic details of some received commands inmediately in the timer
 * interrupt routine, allowing things like retrying some commands and changing
 * the serial mode. Commands that actually return information are delegated to
 * the high-level API to handle.
 *
 * @param[in] cmd command to process
 */
static void MA_ProcessRecvPacket(u8 cmd)
{
    static u8 *pPacket;

    i = TRUE;
    pPacket = gMA.sendPacket;
    if (gMA.status & STATUS_BIOS_RESTART) {
        // Go back into idle mode after the session has been restarted
        gMA.status &= ~STATUS_BIOS_RESTART;
        MA_BiosStop();
        i = FALSE;
    } else {
        switch (cmd) {
        case MAPROT_REPLY | MACMD_START:
            // The adapter has been initialized
            MA_SetInterval(0);
            gMA.status |= STATUS_CONNECTED;
            break;

        case MAPROT_REPLY | MACMD_END:
            // The adapter has been deinitialized, all connections closed
            gMA.status &= ~STATUS_CONNTEST;
            gMA.status &= ~STATUS_CONN_PTP;
            gMA.status &= ~STATUS_PTP_SEND_DONE;
            gMA.status &= ~STATUS_PTP_SEND;
            gMA.condition &= ~MA_CONDITION_CONNECT;

            // Wait a little and reset the library
            if (gMA.task != TASK_INITLIBRARY2 && gMA.task != TASK_CONDITION2) {
                gMA.intrMode = MA_INTR_WAIT;
                gMA.timerInter = TIMER_COUNTER_MS(1000);
                i = FALSE;
            }
            break;

        case MAPROT_REPLY | MACMD_TEL:
        case MAPROT_REPLY | MACMD_WAIT_CALL:
            // We're connected to a phone now
            gMA.condition |= MA_CONDITION_CONNECT;
            break;

        case MAPROT_REPLY | MACMD_OFFLINE:
            // We're completely disconnected
            gMA.condition &= ~MA_CONDITION_CONNECT;
            break;

        case MAPROT_REPLY | MACMD_DATA_END:
            // The remote closed the TCP connection
            gMA.pRecvBuf->size = gMA.recvIoBuf.size;
            gMA.condition |= MA_CONDITION_TCPCLOSED;
            break;

        case MAPROT_REPLY | MACMD_REINIT:
            // Return to 8-bit serial mode after resetting the adapter, and give
            // the adapter some time to reset
            MA_ChangeSIOMode(MA_SIO_BYTE);
            gMA.intrMode = MA_INTR_WAIT;
            gMA.timerInter = TIMER_COUNTER_MS(500);
            i = FALSE;
            break;

        case MAPROT_REPLY | MACMD_CHANGE_CLOCK:
            // Change the serial mode based on the byte in the request packet
            MA_ChangeSIOMode(pPacket[6]);
            gMA.intrMode = MA_INTR_WAIT;
            gMA.timerInter = TIMER_COUNTER_MS(60);
            i = FALSE;
            break;

        case MAPROT_REPLY | MACMD_CHECK_STATUS:
            // Make sure to update gMA.condition according to the received
            // status even if the calling function doesn't bother.
            MA_ProcessCheckStatusResponse(gMA.recvIoBuf.pWrite[0]);
            break;

        case MAPROT_REPLY | MACMD_ERROR:
            // Save the error
            gMA.negaResCmd = gMA.recvIoBuf.pWrite[0];
            gMA.negaResErr = gMA.recvIoBuf.pWrite[1];

            // TCP Connect command is retried up to five times
            if (gMA.negaResCmd == MACMD_TCP_CONNECT
                && ++gMA.tcpConnectRetryCount != 5) {
                gMA.timerInter = TIMER_COUNTER_MS(1000);
                gMA.status |= STATUS_SIO_RETRY;
                gMA.intrMode = MA_INTR_WAIT;
                i = FALSE;
                *(vu32 *)REG_TM3CNT = 0;
                *(vu32 *)REG_TM3CNT = TIMER_FLAGS | gMA.timerInter;
            }
            break;

        default:
            gMA.pRecvBuf->size = gMA.recvIoBuf.size;
            break;
        }
    }

    gMA.status &= ~STATUS_SIO_RECV_DONE;

    // Signal the main api to process the received packet
    if (i == TRUE) {
        gMA.intrMode = MA_INTR_IDLE;
        gMA.condition &= ~MA_CONDITION_BIOS_BUSY;
    }

    gMA.counter = 0;
}

/**
 * @brief Timer interrupt entrypoint
 *
 * Must be called by the game upon a timer 3 interrupt. Handles running API
 * tasks as well as initializing the next serial transfer.
 */
void MA_IntrTimer(void)
{
    static u8 saveSioMode;

    gMA.status |= STATUS_INTR_TIMER;
    saveSioMode = gMA.intrMode;
    *(vu32 *)REG_TM3CNT = 0;

    // Avoid running if the interrupt is disabled, or we're waiting for the
    // serial interrupt to run first.
    if (!(gMA.status & STATUS_TIMER_ENABLED) || gMA.status & STATUS_INTR_SIO
        || gMA.status & STATUS_API_CALL || *(vu16 *)REG_SIOCNT & SIO_START) {
        *(vu32 *)REG_TM3CNT = TIMER_FLAGS | gMA.timerInter;
        gMA.status &= ~STATUS_INTR_TIMER;
        return;
    }

    // Execute the current API task
    MAAPI_Main();

    // If the interrupt mode changed, update the timer interval first
    if (saveSioMode != gMA.intrMode) {
        gMA.status &= ~STATUS_INTR_TIMER;
        *(vu32 *)REG_TM3CNT = TIMER_FLAGS | gMA.timerInter;
        return;
    }

    // If we're done receiving a packet, interpret it
    if (gMA.status & STATUS_SIO_RECV_DONE) {
        MA_ProcessRecvPacket(gMA.recvCmd);
        gMA.status &= ~STATUS_INTR_TIMER;
        *(vu32 *)REG_TM3CNT = TIMER_FLAGS | gMA.timerInter;
        return;
    }

    // Handle the different interrupt states
    switch (gMA.intrMode) {
    case MA_INTR_IDLE: MA_IntrTimer_SIOIdle(); break;
    case MA_INTR_WAIT: MA_IntrTimer_SIOWaitTime(); break;
    case MA_INTR_SEND: MA_IntrTimer_SIOSend(); break;
    case MA_INTR_RECV: MA_IntrTimer_SIORecv(); break;
    }
    MA_StartSioTransmit();

    *(vu32 *)REG_TM3CNT = TIMER_FLAGS | gMA.timerInter;
    gMA.status &= ~STATUS_INTR_TIMER;
}

/**
 * @brief Timeout macro for SIO interrupt handlers
 */
#define MA_IntrSio_Timeout() \
{ \
    if (++gMA.counter > gMA.timeoutCounter[gMA.sioMode]) { \
        if (!(gMA.status & STATUS_BIOS_RESTART)) { \
            MA_CancelRequest(); \
            gMA.status |= STATUS_SIO_SEND_TIMEOUT; \
        } else { \
            MA_BiosStop(); \
        } \
        return; \
    } \
}

/**
 * @brief Inline variant of MA_Bios_disconnect()
 */
#define MA_Bios_disconnect_inline() \
{ \
    gMA.intrMode = MA_INTR_IDLE; \
    gMA.sendIoBuf.state = IOBUF_SEND_IDLE; \
    gMA.recvIoBuf.state = IOBUF_RECV_IDLE; \
    MA_SetError(MAAPIE_MA_NOT_FOUND); \
    gMA.status &= ~STATUS_CONNECTED; \
    gMA.condition &= ~MA_CONDITION_BIOS_BUSY; \
    gMA.status &= ~STATUS_CONNTEST; \
    gMA.sendIoBuf.state = IOBUF_SEND_IDLE; \
    *(vu32 *)REG_TM3CNT = 0; \
}

/**
 * @brief End the connection abruptly
 *
 * Used when an invalid or malfunctioning adapter is detected, disables the
 * timer interrupt and raises an error.
 */
void MA_Bios_disconnect(void)
{
    MA_Bios_disconnect_inline();
}

/**
 * @brief Serial interrupt handler for MA_INTR_SEND
 *
 * Handles the received bytes while sending a packet, essentially only used to
 * check the footer bytes and initialize the packet reception once done.
 */
static void MA_IntrSio_Send(void)
{
    static int dataLeft;

    // Only handle the footer if a packet is actually being sent
    switch (gMA.sendIoBuf.state) {
    default:
    case IOBUF_SEND_PRESTART: return;
    case IOBUF_SEND_START: return;
    case IOBUF_SEND_DATA: break;
    }

    MA_IntrSio_Timeout();

    // Read out the footer
    dataLeft = gMA.sendIoBuf.size - gMA.sendIoBuf.readCnt;
    if (gMA.sioMode == MA_SIO_BYTE) {
        if (dataLeft < 2) {
            gMA.sendFooter[1 - dataLeft] = *(vu8 *)(REG_SIODATA8);
        }
    } else {
        if (dataLeft == 0) {
            gMA.sendFooter[0] = *(vu8 *)(REG_SIODATA32 + 3);
            gMA.sendFooter[1] = *(vu8 *)(REG_SIODATA32 + 2);
            gMA.sendFooter[2] = *(vu8 *)(REG_SIODATA32 + 1);
            gMA.sendFooter[3] = *(vu8 *)(REG_SIODATA32 + 0);
        }
    }

    // Don't actually process the footer until everything has been sent
    if (gMA.sendIoBuf.size != gMA.sendIoBuf.readCnt) return;

    // Pull up behavior on SI (pin always high) means nothing is connected
    if (gMA.sendFooter[0] == 0xff && gMA.sendFooter[1] == 0xff) {
        MA_SetError(MAAPIE_MA_NOT_FOUND);
        gMA.status &= ~STATUS_CONNECTED;
        gMA.condition &= ~MA_CONDITION_BIOS_BUSY;
        gMA.status &= ~STATUS_CONNTEST;
        return;
    }

    // Check if the adapter hardware is supported
    if (!MA_IsSupportedHardware(gMA.sendFooter[0])
        && !(gMA.status & STATUS_BIOS_RESTART)) {
        if (gMA.status & STATUS_SIO_RETRY) {
            if (--gMA.retryCount == 0) {
                MA_Bios_disconnect_inline();
                return;
            }
        } else {
            gMA.retryCount = 2;
        }

        gMA.footerError = gMA.sendFooter[1];
        gMA.timerInter = TIMER_COUNTER_MS(120);
        gMA.status |= STATUS_SIO_RETRY;
        gMA.intrMode = MA_INTR_WAIT;
        *(vu32 *)REG_TM3CNT = 0;
        *(vu32 *)REG_TM3CNT = TIMER_FLAGS | gMA.timerInter;
        return;
    }

    // Check if the adapter returned an internal error
    if (gMA.sendFooter[1] == MAPROT_ERR_INTERNAL
        && !(gMA.status & STATUS_BIOS_RESTART)) {
        gMA.footerError = MAPROT_ERR_INTERNAL;

        if (gMA.status & STATUS_SIO_RETRY) {
            if (--gMA.retryCount == 0) {
                MA_SetError(MAAPIE_PROT_INTERNAL);
                gMA.status &= ~STATUS_CONNECTED;
                gMA.condition &= ~MA_CONDITION_BIOS_BUSY;
                gMA.status &= ~STATUS_CONNTEST;
                return;
            }
        } else {
            gMA.retryCount = -2;
        }

        gMA.footerError = gMA.sendFooter[1];
        gMA.timerInter = gMA.timerIntInter[gMA.sioMode];
        gMA.status |= STATUS_SIO_RETRY;
        gMA.intrMode = MA_INTR_WAIT;
        *(vu32 *)REG_TM3CNT = 0;
        *(vu32 *)REG_TM3CNT = TIMER_FLAGS | gMA.timerInter;
        return;
    }

    // Check if the adapter supports the command and didn't fail the checksum
    if ((gMA.sendFooter[1] == MAPROT_ERR_ILLEGAL_CMD
            || gMA.sendFooter[1] == MAPROT_ERR_CHECKSUM)
        && !(gMA.status & STATUS_BIOS_RESTART)) {
        if (gMA.status & STATUS_SIO_RETRY) {
            if (--gMA.retryCount == 0) {
                if (gMA.sendFooter[1] == MAPROT_ERR_ILLEGAL_CMD) {
                    MA_SetError(MAAPIE_PROT_ILLEGAL_CMD);
                } else {
                    MA_SetError(MAAPIE_PROT_CHECKSUM);
                }
                gMA.status &= ~STATUS_CONNECTED;
                gMA.condition &= ~MA_CONDITION_BIOS_BUSY;
                gMA.status &= ~STATUS_CONNTEST;
                return;
            }
        } else {
            gMA.retryCount = 2;
        }

        gMA.footerError = gMA.sendFooter[1];
        gMA.timerInter = TIMER_COUNTER_MS(1000);
        gMA.status |= STATUS_SIO_RETRY;
        gMA.intrMode = MA_INTR_WAIT;
        *(vu32 *)REG_TM3CNT = 0;
        *(vu32 *)REG_TM3CNT = TIMER_FLAGS | gMA.timerInter;
        return;
    }

    // Make sure the adapter received the correct command, or an error command
    if (!(gMA.sendFooter[1] == MAPROT_REPLY + gMA.sendCmd
            || gMA.sendFooter[1] == MAPROT_REPLY + MACMD_ERROR)
        && !(gMA.status & STATUS_BIOS_RESTART)) {
        if (gMA.status & STATUS_SIO_RETRY) {
            if (--gMA.retryCount == 0) {
                MA_Bios_disconnect();
                return;
            }
        } else {
            gMA.retryCount = 2;
        }

        gMA.footerError = gMA.sendFooter[1];
        gMA.timerInter = TIMER_COUNTER_MS(120);
        gMA.status |= STATUS_SIO_RETRY;
        gMA.intrMode = MA_INTR_WAIT;
        *(vu32 *)REG_TM3CNT = 0;
        *(vu32 *)REG_TM3CNT = TIMER_FLAGS | gMA.timerInter;
        return;
    }

    // Initialize the reception of the reply packet
    gMA.hardwareType = gMA.sendFooter[0];
    if (gMA.sioMode == MA_SIO_BYTE) {
        MA_InitIoBuffer(&gMA.tempIoBuf, (u8 *)MaPacketData_PreStart, 1, 0);
    } else {
        MA_InitIoBuffer(&gMA.tempIoBuf, (u8 *)MaPacketData_PreStart, 4, 0);
    }
    MA_InitIoBuffer(&gMA.recvIoBuf, gMA.pRecvBuf->data, 0, IOBUF_RECV_START);
    gMA.recvRubbishCount = 0;
    gMA.status &= ~STATUS_SIO_RETRY;
    gMA.intrMode = MA_INTR_RECV;
    gMA.sendIoBuf.state = IOBUF_SEND_IDLE;
    gMA.counter = 0;
    gMA.negaResCmd = 0;
    gMA.negaResErr = 0;
}

/**
 * @brief Serial interrupt handler for MA_INTR_RECV
 *
 * Handles packet reception, timing out and checksumming.
 *
 * The first parameter indicates which of the four received bytes to process in
 * the WORD (32-bit) mode.
 *
 * @param[in] byte byte index to process
 */
static void MA_IntrSio_Recv(u8 byte)
{
    static u8 recvByte;
    static int amari;

    if (gMA.sioMode == MA_SIO_BYTE) {
        recvByte = ((vu8 *)REG_SIODATA8)[0];
    } else {
        recvByte = ((vu8 *)REG_SIODATA32)[3 - byte];
    }

    switch (gMA.recvIoBuf.state) {
    // Receive magic bytes
    case IOBUF_RECV_START:
        // Wait for handshake
        switch (gMA.recvIoBuf.readCnt) {
        case 0:
            // Handshake may only start from the first byte in a MA_SIO_WORD
            //   serial transaction.
            if (byte != 0) break;

            // If we time out, stop trying to receive
            MA_IntrSio_Timeout();

            // When the first magic byte is received, try to receive the second
            if (recvByte == MAPROT_MAGIC_1) {
                gMA.recvIoBuf.readCnt++;
                break;
            }

            // Adapter is still busy, keep waiting
            if (recvByte == MAPROT_IDLE_SLAVE) break;

            // Allow up to 20 rubbish bytes to be received
            if (++gMA.recvRubbishCount <= 20) break;
            MA_SetError(MAAPIE_MA_NOT_FOUND);
            gMA.recvRubbishCount = 0;
            break;

        case 1:
            // Try to receive the second magic byte
            if (recvByte == MAPROT_MAGIC_2) {
                gMA.recvIoBuf.state = IOBUF_RECV_HEADER;
                gMA.recvIoBuf.readCnt = 0;
            } else {
                MA_SetError(MAAPIE_MA_NOT_FOUND);
            }
            gMA.recvRubbishCount = 0;
            break;
        }
        break;

    // Receive packet header
    case IOBUF_RECV_HEADER:
        switch (gMA.recvIoBuf.readCnt) {
        case 0:  // Command
            gMA.recvCmd = recvByte;
            break;

        case 1:  // Unused
            break;

        case 2:  // Size (high byte, assume 0)
            ((u8 *)&gMA.recvIoBuf.size)[3 - gMA.recvIoBuf.readCnt] = 0;
            break;

        case 3:  // Size (low byte)
            ((u8 *)&gMA.recvIoBuf.size)[3 - gMA.recvIoBuf.readCnt] = recvByte;
            break;
        }

        // Read until we get 4 bytes
        gMA.recvIoBuf.readCnt++;
        gMA.recvIoBuf.checkSum += recvByte;
        if (gMA.recvIoBuf.readCnt != 4) break;

        // If the packet is empty, skip to the end
        if (gMA.recvIoBuf.size == 0) {
            gMA.recvIoBuf.state = IOBUF_RECV_CHECKSUM;
            gMA.recvIoBuf.readCnt = 0;
            break;
        }

        // When using MA_SIO_WORD mode, a multiple of 4 bytes should be read
        if (gMA.sioMode == MA_SIO_BYTE) {
            gMA.recvIoBuf.readCnt = gMA.recvIoBuf.size;
        } else {
            amari = gMA.recvIoBuf.size % 4;
            if (amari == 0) {
                gMA.recvIoBuf.readCnt = gMA.recvIoBuf.size;
            } else {
                gMA.recvIoBuf.readCnt = gMA.recvIoBuf.size + (4 - amari);
            }
        }

        gMA.recvIoBuf.state = IOBUF_RECV_DATA;
        break;

    // Receive packet data
    case IOBUF_RECV_DATA:
        MA_IntrSio_Timeout();

        // Read the packet body
        *gMA.recvIoBuf.pRead++ = recvByte;
        gMA.recvIoBuf.readCnt--;
        gMA.recvIoBuf.checkSum += recvByte;
        if (gMA.recvIoBuf.readCnt != 0) break;

        gMA.recvIoBuf.state = IOBUF_RECV_CHECKSUM;
        gMA.recvIoBuf.readCnt = 0;
        break;

    // Receive and verify packet checksum
    case IOBUF_RECV_CHECKSUM:
        // Read the checksum
        ((u8 *)&gMA.checkSum)[1 - gMA.recvIoBuf.readCnt] = recvByte;
        gMA.recvIoBuf.readCnt++;
        if (gMA.recvIoBuf.readCnt != 2) break;

        // Initialize the recvFooter buffer
        MA_InitIoBuffer(&gMA.tempIoBuf, gMA.replyFooter,
            sizeof(gMA.replyFooter), IOBUF_RECV_IDLE);
        gMA.replyFooter[0] = MAPROT_TYPE_MASTER | MATYPE_GBA;
        if (gMA.checkSum != gMA.recvIoBuf.checkSum
            && !(gMA.status & STATUS_BIOS_RESTART)) {
            gMA.replyFooter[1] = MAPROT_ERR_CHECKSUM;
            gMA.status |= STATUS_SIO_ERR_CHECKSUM;
        } else {
            gMA.replyFooter[1] = gMA.recvCmd;
        }
        gMA.replyFooter[3] = 0;
        gMA.replyFooter[2] = 0;

        gMA.recvIoBuf.state = IOBUF_RECV_FOOTER;
        gMA.recvIoBuf.readCnt = 0;
        break;

    // Receive and verify packet footer
    case IOBUF_RECV_FOOTER:
        gMA.recvFooter[gMA.recvIoBuf.readCnt] = recvByte;
        gMA.recvIoBuf.readCnt++;

        // Wait until enough bytes have been read depending on the mode
        if ((gMA.sioMode == MA_SIO_BYTE && gMA.recvIoBuf.readCnt == 2)
            || (gMA.sioMode == MA_SIO_WORD && gMA.recvIoBuf.readCnt == 4)) {
            // Retry if the checksum failed
            if (gMA.status & STATUS_SIO_ERR_CHECKSUM) {
                gMA.status &= ~STATUS_SIO_ERR_CHECKSUM;
                MA_RecvRetry();
            } else {
                gMA.status |= STATUS_SIO_RECV_DONE;
                gMA.intrMode = MA_INTR_IDLE;
            }
        }
        break;
    }
}

/**
 * @brief Serial interrupt entrypoint
 *
 * Must be called by the game upon a serial interrupt. This is triggered after a
 * serial transmission is complete.
 */
void MA_IntrSerialIO(void)
{
    *(vu32 *)REG_TM3CNT &= ~TMR_IF_ENABLE;
    gMA.status |= STATUS_INTR_SIO;

    // Handle the different interrupt modes
    switch (gMA.intrMode) {
    case MA_INTR_SEND:
        MA_IntrSio_Send();
        gMA.status |= STATUS_TIMER_ENABLED;
        break;

    case MA_INTR_RECV:
        if (gMA.sioMode == MA_SIO_BYTE) {
            MA_IntrSio_Recv(0);
        } else {
            // In WORD (32-bit) mode, process all four received bytes separately
            for (i = 0; i < 4; i++) {
                if (gMA.condition & MA_CONDITION_ERROR) break;
                MA_IntrSio_Recv(i);
            }
        }
        break;
    }

    *(vu32 *)REG_TM3CNT |= TMR_IF_ENABLE;
    gMA.status |= STATUS_TIMER_ENABLED;
    gMA.status &= ~STATUS_INTR_SIO;
}
