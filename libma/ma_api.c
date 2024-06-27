#include "ma_api.h"

#include <stddef.h>
#include "ma_ango.h"
#include "ma_bios.h"
#include "ma_sub.h"

#define CASSETTE_INITIAL_CODE (ROM_BANK0 + 0xac)
#define CASSETTE_VERSION_NO (ROM_BANK0 + 0xbc)

#define MAX_P2P_DATA_SIZE 0x80
#define MAX_TCP_DATA_SIZE (MAPROT_BODY_SIZE - 2)
#define MAX_TELNO_LEN 20
#define MAX_USERID_LEN 0x20
#define MAX_PASSWORD_LEN 0x10
#define MAX_EMAIL_LEN 30
#define MAX_URL_LEN 0x400
#define MAX_RECIPIENT_LEN 0x80
#define MAX_RECIPIENTS 0x100

// TCP port numbers
#define PORT_SMTP 25
#define PORT_HTTP 80
#define PORT_POP3 110

enum server_types {
    SERVER_UNKNOWN,
    SERVER_DOWNLOAD,
    SERVER_UPLOAD,
    SERVER_UTILITY,
    SERVER_RANKING,
};

static void MA_SetApiError(u8 error, u16 errorDetail);
static int ApiValisStatusCheck(u8 task);
static int MA_ApiPreExe(u8 task);
static void MakeEndLineBuffer(u8 *end, int size);
static int IsEndMultiLine(void);
static void InitPrevBuf(void);
static void ConcatPrevBuf(u8 *data, u16 size);
static void MATASK_Stop(void);
static void MATASK_TCP_Cut(void);
static void MA_InitLibraryMain(u8 *pHardwareType, int task);
static void MATASK_InitLibrary(void);
static void MATASK_TCP_Connect(void);
static void MATASK_TCP_Disconnect(void);
static void MATASK_TCP_SendRecv(void);
static void MATASK_GetHostAddress(void);
static int EEPROMSumCheck(const u8 *data);
static int EEPROMRegistrationCheck(const u8 *data);
static void MATASK_TelServer(void);
static void MATASK_Tel(void);
static void MATASK_Receive(void);
static void MATASK_P2P(void);
static void MA_ConditionMain(u8 *pCondition, int task);
static void MATASK_Condition(void);
static void MATASK_Offline(void);
static int CheckSMTPResponse(const char *response);
static void MATASK_SMTP_Connect(void);
static void MATASK_SMTP_Sender(void);
static void MATASK_SMTP_Send(void);
static void MATASK_SMTP_POP3_Quit(void);
static int CheckPOP3Response(const char *response);
static void MATASK_POP3_Connect(void);
static void MATASK_POP3_Stat(void);
static void MATASK_POP3_List(void);
static void MATASK_POP3_Retr(void);
static void MATASK_POP3_Dele(void);
static void MATASK_POP3_Head(void);
static const char *ExtractServerName(char *pServerName, const char *pURL,
    u8 *pServerAuth, u8 *pServerType);
static void MA_HTTP_GetPost(const char *pURL, char *pHeadBuf, u16 headBufSize,
    const u8 *pSendData, u16 sendSize, u8 *pRecvData, u16 recvBufSize,
    u16 *pRecvSize, const char *pUserID, const char *pPassword, u8 task);
static void ConcatUserAgent(char *pUserAgent);
static int GetRequestType(void);
static void CreateHttpRequestHeader(void);
static int HttpGetNextStep(int index);
static void MATASK_HTTP_GetPost(void);
static void CopyEEPROMString(char *pDest, const char *pSrc, int size);
static void CopyEEPROMData(int task, void *pDest);
static void MATASK_GetEEPROMData(void);
static void MATASK_EEPROM_Read(void);
static void MATASK_EEPROM_Write(void);
static u16 ErrDetailHexConv(u16 err);

#define CheckResponse(buf) ( \
    (MAU_isdigit((buf)[0])) && \
    (MAU_isdigit((buf)[1])) && \
    (MAU_isdigit((buf)[2])))

#define GetResponse(buf) ( \
    ((buf)[0] - '0') * 100 + \
    ((buf)[1] - '0') * 10 + \
    ((buf)[2] - '0'))

void SetApiCallFlag(void)
{
    *(vu16 *)REG_IE &= ~SIO_INTR_FLAG & ~TIMER3_INTR_FLAG;
    gMA.status |= STATUS_API_CALL;
}

void ResetApiCallFlag(void)
{
    gMA.status &= ~STATUS_API_CALL;
    *(vu16 *)REG_IE |= SIO_INTR_FLAG | TIMER3_INTR_FLAG;
}

void MA_TaskSet(u8 task, u8 step)
{
    gMA.timerInter = gMA.timerIntInter[gMA.sioMode];
    gMA.task = task;
    gMA.taskStep = step;
    if (gMA.task == 0) gMA.condition &= ~MA_CONDITION_APIWAIT;
}

static void MA_SetApiError(u8 error, u16 errorDetail)
{
    gMA.smtpSender = FALSE;
    gMA.errorDetail = errorDetail;
    MA_SetError(error);
}

static int ApiValisStatusCheck(u8 task)
{
    static int ret;

    ret = TRUE;
    switch (task) {
    case TASK_TELSERVER:
        if (gMA.connMode == CONN_OFFLINE) break;
        ret = FALSE;
        break;

    case TASK_TEL:
        if (gMA.connMode == CONN_OFFLINE) break;
        ret = FALSE;
        break;

    case TASK_RECEIVE:
        if (gMA.connMode == CONN_OFFLINE) break;
        ret = FALSE;
        break;

    case TASK_SDATA:
        if (gMA.connMode == CONN_P2P_SEND) break;
        if (gMA.connMode == CONN_P2P_RECV) break;
        ret = FALSE;
        break;

    case TASK_GDATA:
        if (gMA.connMode == CONN_P2P_SEND) break;
        if (gMA.connMode == CONN_P2P_RECV) break;
        ret = FALSE;
        break;

    case TASK_OFFLINE:
        if (gMA.connMode == CONN_PPP) break;
        if (gMA.connMode == CONN_P2P_SEND) break;
        if (gMA.connMode == CONN_P2P_RECV) break;
        if (gMA.connMode == CONN_SMTP) break;
        if (gMA.connMode == CONN_POP3) break;
        ret = FALSE;
        break;

    case TASK_SMTP_SENDER:
    case TASK_SMTP_SEND:
    case TASK_SMTP_QUIT:
        if (gMA.connMode == CONN_SMTP) break;
        ret = FALSE;
        break;

    case TASK_POP3_STAT:
    case TASK_POP3_LIST:
    case TASK_POP3_RETR:
    case TASK_POP3_DELE:
    case TASK_POP3_HEAD:
    case TASK_POP3_QUIT:
        if (gMA.connMode == CONN_POP3) break;
        ret = FALSE;
        break;

    case TASK_HTTP_GET:
    case TASK_HTTP_POST:
        if (gMA.connMode == CONN_PPP) break;
        if (gMA.connMode == CONN_HTTP) break;
        ret = FALSE;
        break;

    case TASK_GETTEL:
    case TASK_GETUSERID:
    case TASK_GETMAILID:
        if (gMA.connMode == CONN_OFFLINE) break;
        ret = FALSE;
        break;

    case TASK_EEPROM_READ:
    case TASK_EEPROM_WRITE:
        if (gMA.connMode == CONN_OFFLINE) break;
        ret = FALSE;
        break;

    case TASK_SMTP_CONNECT:
    case TASK_POP3_CONNECT:
    case TASK_TCP_CONNECT:
    case TASK_TCP_DISCONNECT:
    case TASK_TCP_SENDRECV:
    case TASK_GETHOSTADDRESS:
    case TASK_GETLOCALADDRESS:
        if (gMA.connMode == CONN_PPP) break;
        ret = FALSE;
        break;
    }

    return ret;
}

static int MA_ApiPreExe(u8 task)
{
    gMA.error = 0;
    gMA.errorDetail = 0;

    if (gMA.apiMagic != MAAPI_MAGIC) {
        MA_SetApiError(MAAPIE_CANNOT_EXECUTE, 0);
        return FALSE;
    }
    if (gMA.status & STATUS_INTR_TIMER || gMA.status & STATUS_INTR_SIO) {
        MA_SetApiError(MAAPIE_CANNOT_EXECUTE, 0);
        return FALSE;
    }
    if (gMA.condition & MA_CONDITION_ERROR) {
        MA_SetApiError(MAAPIE_CANNOT_EXECUTE, 0);
        return FALSE;
    }
    if (gMA.task != TASK_NONE) {
        MA_SetApiError(MAAPIE_CANNOT_EXECUTE, 0);
        return FALSE;
    }
    if (!ApiValisStatusCheck(task)) {
        MA_SetApiError(MAAPIE_CANNOT_EXECUTE, 0);
        return FALSE;
    }

    if (task != TASK_HTTP_GET && task != TASK_HTTP_POST) {
        gMA.condition &= ~MA_CONDITION_TCPCLOSED;
    }
    gMA.condition &= ~MA_CONDITION_ERROR;
    gMA.error = -1;
    gMA.taskError = 0;
    gMA.taskErrorDetail = 0;
    gMA.httpRes = 0;
    gMA.gbCenterRes = 0;
    gMA.condition |= MA_CONDITION_APIWAIT;
    return TRUE;
}

static void MakeEndLineBuffer(u8 *end, int size)
{
    gMA.endLineBuf[5] = '\0';
    if (size == 0) return;
    if (end == NULL) return;

    if (size >= 5) {
        MAU_memcpy(gMA.endLineBuf, end, 5);
    } else {
        MAU_memcpy(gMA.endLineBuf, &gMA.endLineBuf[size], 5 - size);
        MAU_memcpy(&gMA.endLineBuf[5 - size], end, size);
    }
}

static int IsEndMultiLine(void)
{
    static const char strEndMultiLine[] = "\r\n.\r\n";

    if (MAU_strcmp(gMA.endLineBuf, strEndMultiLine) == 0) {
        return TRUE;
    } else {
        return FALSE;
    }
}

static void InitPrevBuf(void)
{
    gMA.prevBuf[0] = 0;
    gMA.prevBufSize = 0;
    gMA.prevBufHasEEPROMData = FALSE;
}

static void ConcatPrevBuf(u8 *data, u16 size)
{
    MAU_memcpy(&gMA.prevBuf[gMA.prevBufSize], data, size);
    gMA.prevBufSize += size;
}

void MA_End(void)
{
    int zero;

    if (gMA.connMode != CONN_OFFLINE) {
        MA_SetApiError(MAAPIE_CANNOT_EXECUTE, 0);
        return;
    }

    *(vu32 *)REG_TM3CNT = 0;
    *(vu16 *)REG_SIOCNT = 0;
    *(vu16 *)REG_IE &= ~SIO_INTR_FLAG & ~TIMER3_INTR_FLAG;
    zero = 0;
    CpuSet(&zero, &gMA, DMA_SRC_FIX | DMA_32BIT_BUS | (sizeof(gMA) / 4));
}

void MA_Stop(void)
{
    SetApiCallFlag();
    if (gMA.apiMagic != MAAPI_MAGIC) {
        MA_SetApiError(MAAPIE_CANNOT_EXECUTE, 0);
        return;
    }

    if (gMA.condition & MA_CONDITION_ERROR) {
        MA_SetApiError(MAAPIE_CANNOT_EXECUTE, 0);
        ResetApiCallFlag();
        return;
    }

    if (gMA.task == TASK_STOP
        || (gMA.connMode == CONN_OFFLINE
            && !(gMA.condition & MA_CONDITION_APIWAIT))) {
        ResetApiCallFlag();
        return;
    }

    gMA.condition &= ~MA_CONDITION_BUFFER_FULL;

    if (gMA.condition & MA_CONDITION_BIOS_BUSY && gMA.sendCmd == MACMD_TEL) {
        gMA.condition |= MA_CONDITION_APIWAIT;
        MA_TaskSet(TASK_STOP, 0);
        MA_CancelRequest();
        ResetApiCallFlag();
        return;
    }

    gMA.condition |= MA_CONDITION_APIWAIT;
    MA_TaskSet(TASK_STOP, 1);
    ResetApiCallFlag();
}

static void MATASK_Stop(void)
{
    switch (gMA.taskStep) {
    case 0:
        gMA.taskStep++;

    case 1:
        MA_BiosStop();
        gMA.taskStep++;
        break;

    case 2:
        MA_ChangeSIOMode(MA_SIO_BYTE);
        MA_Reset();
        gMA.recvCmd = 0;
        MA_TaskSet(TASK_NONE, 0);
        break;
    }
}

void MA_TCP_Cut(void)
{
#define param gMA.param.tcp_cut
    SetApiCallFlag();
    if (gMA.apiMagic != MAAPI_MAGIC) {
        MA_SetApiError(MAAPIE_CANNOT_EXECUTE, 0);
        return;
    }

    if (gMA.connMode == CONN_PPP && gMA.task == TASK_NONE) {
        ResetApiCallFlag();
        return;
    }

    if (gMA.connMode != CONN_HTTP
        && gMA.connMode != CONN_SMTP
        && gMA.connMode != CONN_POP3
        && !(TASK_SMTP_CONNECT <= gMA.task && gMA.task <= TASK_HTTP_POST
             && gMA.condition & MA_CONDITION_PPP)) {
        MA_SetApiError(MAAPIE_CANNOT_EXECUTE, 0);
        ResetApiCallFlag();
        return;
    }

    if (gMA.condition & MA_CONDITION_ERROR) {
        MA_SetApiError(MAAPIE_CANNOT_EXECUTE, 0);
        ResetApiCallFlag();
        return;
    }

    if (gMA.task == TASK_TCP_CUT) {
        ResetApiCallFlag();
        return;
    }

    if (gMA.task == TASK_STOP) {
        MA_SetApiError(MAAPIE_CANNOT_EXECUTE, 0);
        ResetApiCallFlag();
        return;
    }

    param.cmd = gMA.sendCmd;
    gMA.condition &= ~MA_CONDITION_BUFFER_FULL;
    if (!(gMA.condition & MA_CONDITION_BIOS_BUSY) && gMA.connMode != CONN_PPP) {
        gMA.condition |= MA_CONDITION_APIWAIT;
        MA_TaskSet(TASK_TCP_CUT, 2);
        ResetApiCallFlag();
    } else {
        gMA.condition |= MA_CONDITION_APIWAIT;
        MA_TaskSet(TASK_TCP_CUT, 0);
        ResetApiCallFlag();
    }
#undef param
}

static void MATASK_TCP_Cut(void)
{
#define param gMA.param.tcp_cut
    switch (gMA.taskStep) {
    case 0:
        gMA.taskStep++;

    case 1:
        if (param.cmd == MACMD_TCP_CONNECT
            && gMA.recvCmd != (MAPROT_REPLY | MACMD_ERROR)) {
            gMA.sockets[0] = gMA.recvBuf.data[0];
            gMA.taskStep++;
        } else if (gMA.usedSockets[0] != TRUE) {
            gMA.taskStep = 3;
            break;
        } else {
            gMA.taskStep++;
        }

    case 2:
        MABIOS_TCPDisconnect(&gMA.recvBuf, gMA.sockets[0]);
        gMA.taskStep++;
        break;

    case 3:
        gMA.connMode = CONN_PPP;
        MA_SetCondition(MA_CONDITION_PPP);
        gMA.sockets[0] = 0;
        gMA.usedSockets[0] = FALSE;
        gMA.recvCmd = 0;
        MA_TaskSet(TASK_NONE, 0);
        break;
    }
#undef param
}

void MA_InitLibrary(u8 *pHardwareType)
{
    MA_InitLibraryMain(pHardwareType, TASK_INITLIBRARY);
}

void MA_InitLibrary2(u8 *pHardwareType)
{
    MA_InitLibraryMain(pHardwareType, TASK_INITLIBRARY2);
}

void MA_InitLibraryMain(u8 *pHardwareType, int task)
{
#define param gMA.param.initlibrary
    int zero;

    zero = 0;
    CpuSet(&zero, &gMA, DMA_SRC_FIX | DMA_32BIT_BUS | (sizeof(gMA) / 4));

    SetApiCallFlag();

    gMA.apiMagic = MAAPI_MAGIC;
    MA_Reset();
    InitPrevBuf();
    MABIOS_Init();

    gMA.prevBufHasEEPROMData = FALSE;
    param.pHardwareType = pHardwareType;
    MA_TaskSet(task, 0);

    gMA.condition |= MA_CONDITION_APIWAIT;
    ResetApiCallFlag();

    if (!(*(vu32 *)REG_TM3CNT & TMR_IF_ENABLE)
        || !(*(vu32 *)REG_TM3CNT & TMR_ENABLE)) {
        MAAPI_Main();
    }
#undef param
}

static void MATASK_InitLibrary(void)
{
#define param gMA.param.initlibrary
    if (gMA.recvCmd == (MAPROT_REPLY | MACMD_ERROR)) {
        switch (gMA.negaResCmd) {
        case MACMD_END:
            break;

        default:
            MA_DefaultNegaResProc();
            gMA.taskStep = 0xfb;
            break;
        }
    }

    switch (gMA.taskStep) {
    case 0:
        MABIOS_Start();
        gMA.taskStep++;
        break;

    case 1:
        gMA.taskStep++;

    case 2:
        MABIOS_End();
        gMA.taskStep++;
        break;

    case 3:
        *param.pHardwareType = gMA.hardwareType - MAPROT_TYPE_SLAVE;
        MA_ChangeSIOMode(MA_SIO_BYTE);
        MA_Reset();
        MA_TaskSet(TASK_NONE, 0);
        break;

    case 0xfa:
        MABIOS_End();
        gMA.taskStep++;
        break;

    case 0xfb:
        MA_Reset();
        MA_SetApiError(gMA.taskError, gMA.taskErrorDetail);
        MA_TaskSet(TASK_NONE, 0);
        break;
    }
#undef param
}

void MA_TCP_Connect(u8 *pSocket, u8 *pAddr, u16 port)
{
#define param gMA.param.tcp_connect
    SetApiCallFlag();
    if (!MA_ApiPreExe(TASK_TCP_CONNECT)) {
        ResetApiCallFlag();
        return;
    }

    if (!MAU_Socket_FreeCheck()) {
        MA_SetApiError(MAAPIE_CANNOT_EXECUTE, 0);
        ResetApiCallFlag();
        return;
    }

    if (MAU_Socket_GetNum() == 0) {
        MAU_memcpy(gMA.socketAddr, pAddr, sizeof(gMA.socketAddr));
    } else if (!MAU_Socket_IpAddrCheck(pAddr)) {
        MA_SetApiError(MAAPIE_ILLEGAL_PARAMETER, 0);
        ResetApiCallFlag();
        return;
    }

    param.pSocket = pSocket;
    param.pAddr = pAddr;
    param.port = port;
    MA_TaskSet(TASK_TCP_CONNECT, 0);
    ResetApiCallFlag();
#undef param
}

static void MATASK_TCP_Connect(void)
{
#define param gMA.param.tcp_connect
    if (gMA.recvCmd == (MAPROT_REPLY | MACMD_ERROR)) {
        switch (gMA.negaResCmd) {
        case MACMD_TCP_CONNECT:
            gMA.taskError = MAAPIE_TCP_CONNECT;
            gMA.taskErrorDetail = 0;
            gMA.taskStep = 0xf0;
            break;

        default:
            MA_DefaultNegaResProc();
            break;
        }
    }

    switch (gMA.taskStep) {
    case 0:
        MA_InitBuffer(&gMA.recvBuf, gMA.recvPacket);
        MABIOS_TCPConnect(&gMA.recvBuf, param.pAddr, param.port);
        gMA.taskStep++;
        break;

    case 1:
        *param.pSocket = gMA.recvBuf.data[0];
        MAU_Socket_Add(gMA.recvBuf.data[0]);
        MA_TaskSet(TASK_NONE, 0);
        break;

    case 0xf0:
        gMA.connMode = CONN_PPP;
        MA_SetCondition(MA_CONDITION_PPP);
        MA_SetApiError(gMA.taskError, gMA.taskErrorDetail);
        MA_TaskSet(TASK_NONE, 0);
    }
#undef param
}

void MA_TCP_Disconnect(u8 socket)
{
#define param gMA.param.tcp_disconnect
    SetApiCallFlag();
    if (!MA_ApiPreExe(TASK_TCP_DISCONNECT)) {
        ResetApiCallFlag();
        return;
    }

    if (MAU_Socket_GetNum() == 0) {
        MA_SetApiError(MAAPIE_CANNOT_EXECUTE, 0);
        ResetApiCallFlag();
        return;
    }

    if (!MAU_Socket_Search(socket)) {
        MA_SetApiError(MAAPIE_ILLEGAL_PARAMETER, 0);
        ResetApiCallFlag();
        return;
    }

    param.socket = socket;
    MA_TaskSet(TASK_TCP_DISCONNECT, 0);
    ResetApiCallFlag();
#undef param
}

static void MATASK_TCP_Disconnect(void)
{
#define param gMA.param.tcp_disconnect
    if (gMA.recvCmd == (MAPROT_REPLY | MACMD_ERROR)) {
        switch (gMA.negaResCmd) {
        case MACMD_TCP_DISCONNECT:
            break;

        default:
            MA_DefaultNegaResProc();
            break;
        }
    }

    switch (gMA.taskStep) {
    case 0:
        MABIOS_TCPDisconnect(&gMA.recvBuf, param.socket);
        gMA.taskStep++;
        break;

    case 1:
        MAU_Socket_Delete(param.socket);
        gMA.connMode = CONN_PPP;
        MA_SetCondition(MA_CONDITION_PPP);
        gMA.sockets[0] = 0;
        gMA.usedSockets[0] = FALSE;
        MA_TaskSet(TASK_NONE, 0);
        break;
    }
#undef param
}

void MA_TCP_SendRecv(u8 socket, u8 *pSendData, u8 sendSize, u8 *pRecvSize)
{
#define param gMA.param.tcp_sendrecv
    SetApiCallFlag();
    if (!MA_ApiPreExe(TASK_TCP_SENDRECV)) {
        ResetApiCallFlag();
        return;
    }

    if (MAU_Socket_GetNum() == 0) {
        MA_SetApiError(MAAPIE_CANNOT_EXECUTE, 0);
        ResetApiCallFlag();
        return;
    }

    if (!MAU_Socket_Search(socket)) {
        MA_SetApiError(MAAPIE_ILLEGAL_PARAMETER, 0);
        ResetApiCallFlag();
        return;
    }

    if (sendSize > MAX_TCP_DATA_SIZE) {
        MA_SetApiError(MAAPIE_ILLEGAL_PARAMETER, 0);
        ResetApiCallFlag();
        return;
    }

    param.socket = socket;
    param.pSendData = pSendData;
    param.sendSize = sendSize;
    param.pRecvSize = pRecvSize;
    MA_TaskSet(TASK_TCP_SENDRECV, 0);
    ResetApiCallFlag();
#undef param
}

static void MATASK_TCP_SendRecv(void)
{
#define param gMA.param.tcp_sendrecv
    if (gMA.recvCmd == (MAPROT_REPLY | MACMD_ERROR)) {
        switch (gMA.negaResCmd) {
        case MACMD_DATA:
            MA_DefaultNegaResProc();
            gMA.taskStep = 0xf0;
            MAU_Socket_Delete(param.socket);
            break;

        default:
            MA_DefaultNegaResProc();
            break;
        }
    }

    if (MA_GetCondition() & MA_CONDITION_TCPCLOSED) {
        gMA.taskError = MAAPIE_OFFLINE;
        gMA.taskErrorDetail = 0;
        gMA.taskStep = 0xf0;
        MAU_Socket_Delete(param.socket);
    }

    switch (gMA.taskStep) {
    case 0:
        MABIOS_Data(&gMA.recvBuf, param.pSendData, param.sendSize, param.socket);
        gMA.taskStep++;
        break;

    case 1:
        MAU_memcpy(param.pSendData, &gMA.recvBuf.data[1], gMA.recvBuf.size - 1);
        *param.pRecvSize = gMA.recvBuf.size - 1;
        MA_TaskSet(TASK_NONE, 0);
        break;

    case 0xf0:
        MA_SetApiError(gMA.taskError, gMA.taskErrorDetail);
        MA_TaskSet(TASK_NONE, 0);
        break;
    }
#undef param
}

void MA_GetHostAddress(u8 *pAddr, char *pServerName)
{
#define param gMA.param.gethostaddress
    SetApiCallFlag();
    if (!MA_ApiPreExe(TASK_GETHOSTADDRESS)) {
        ResetApiCallFlag();
        return;
    }

    if (MAU_strlen(pServerName) >= MAPROT_BODY_SIZE) {
        MA_SetApiError(MAAPIE_ILLEGAL_PARAMETER, 0);
        ResetApiCallFlag();
        return;
    }

    param.pHost = pAddr;
    param.pServerName = pServerName;
    MA_TaskSet(TASK_GETHOSTADDRESS, 0);
    ResetApiCallFlag();
#undef param
}

static void MATASK_GetHostAddress(void)
{
#define param gMA.param.gethostaddress
    if (gMA.recvCmd == (MAPROT_REPLY | MACMD_ERROR)) {
        switch (gMA.negaResCmd) {
        case MACMD_DNS_REQUEST:
            gMA.taskError = MAAPIE_TCP_CONNECT;
            gMA.taskErrorDetail = 0;
            gMA.taskStep = 0xf0;
            break;

        default:
            MA_DefaultNegaResProc();
            break;
        }
    }

    switch (gMA.taskStep) {
    case 0:
        MA_InitBuffer(&gMA.recvBuf, gMA.recvPacket);
        MABIOS_DNSRequest(&gMA.recvBuf, param.pServerName);
        gMA.taskStep++;
        break;

    case 1:
        MAU_memcpy(param.pHost, gMA.recvBuf.data, 4);
        MA_TaskSet(TASK_NONE, 0);
        break;

    case 0xf0:
        MA_SetApiError(gMA.taskError, gMA.taskErrorDetail);
        MA_TaskSet(TASK_NONE, 0);
        break;
    }
#undef param
}

void MA_GetLocalAddress(u8 *pAddr)
{
    SetApiCallFlag();
    if (!MA_ApiPreExe(TASK_GETLOCALADDRESS)) {
        ResetApiCallFlag();
        return;
    }

    pAddr[0] = gMA.localAddr[0];
    pAddr[1] = gMA.localAddr[1];
    pAddr[2] = gMA.localAddr[2];
    pAddr[3] = gMA.localAddr[3];

    ResetApiCallFlag();
    gMA.condition &= ~MA_CONDITION_APIWAIT;
}

static int EEPROMSumCheck(const u8 *data)
{
    static u16 tmp;
    static u16 sum;
    static int i;

    sum = 0;
    for (i = 0; i < 0xbe; i++) {
        sum += data[i];
    }

    tmp = (data[0xbe] << 8) + data[0xbf];
    if (tmp == sum) {
        return TRUE;
    } else {
        return FALSE;
    }
}

static int EEPROMRegistrationCheck(const u8 *data)
{
    if (data[0] == 'M' && data[1] == 'A') {
        return TRUE;
    } else {
        return FALSE;
    }
}

void MA_TelServer(const char *pTelNo, const char *pUserID, const char *pPassword)
{
#define param gMA.param.telserver
    int len_telno, len_userid, len_password;

    SetApiCallFlag();
    if (!MA_ApiPreExe(TASK_TELSERVER)) {
        ResetApiCallFlag();
        return;
    }

    len_telno = MAU_strlen(pTelNo);
    len_userid = MAU_strlen(pUserID);
    len_password = MAU_strlen(pPassword);
    if (len_telno > MAX_TELNO_LEN
        || len_userid > MAX_USERID_LEN
        || len_password > MAX_PASSWORD_LEN
        || len_telno == 0
        || len_userid == 0
        || len_password == 0) {
        MA_SetApiError(MAAPIE_ILLEGAL_PARAMETER, 0);
        ResetApiCallFlag();
        return;
    }

    if (!MAU_IsValidTelNoStr(pTelNo)) {
        MA_SetApiError(MAAPIE_ILLEGAL_PARAMETER, 0);
        ResetApiCallFlag();
        return;
    }

    param.pTelNo = pTelNo;
    param.pUserID = pUserID;
    param.pPassword = pPassword;
    *(vu32 *)REG_TM3CNT = 0;
    MA_TaskSet(TASK_TELSERVER, 0);

    gMA.condition |= MA_CONDITION_APIWAIT;
    ResetApiCallFlag();

    if (!(*(vu32 *)REG_TM3CNT & TMR_IF_ENABLE)
        || !(*(vu32 *)REG_TM3CNT & TMR_ENABLE)) {
        MAAPI_Main();
    }
#undef param
}

static void MATASK_TelServer(void)
{
#define param gMA.param.telserver
    if (gMA.recvCmd == (MAPROT_REPLY | MACMD_ERROR)) {
        switch (gMA.negaResCmd) {
        case MACMD_END:
        case MACMD_OFFLINE:
            break;

        case MACMD_TEL:
        case MACMD_CHECK_STATUS:
        case MACMD_CHANGE_CLOCK:
            MA_DefaultNegaResProc();
            gMA.taskStep = 0xfa;
            break;

        case MACMD_EEPROM_READ:
            gMA.taskError = MAAPIE_EEPROM_SUM;
            gMA.taskErrorDetail = 0;
            gMA.taskStep = 0xfa;
            break;

        case MACMD_PPP_CONNECT:
            gMA.taskError = MAAPIE_PPP_CONNECT;
            gMA.taskErrorDetail = 0;
            gMA.taskStep = 0xf9;
            break;

        default:
            MA_DefaultNegaResProc();
            gMA.taskStep = 0xfb;
            break;
        }
    }

    switch (gMA.taskStep) {
    case 0:
        MABIOS_Start();
        gMA.taskStep++;
        break;

    case 1:
        gMA.taskStep++;
        MABIOS_ChangeClock(MA_SIO_WORD);
        break;

    case 2:
        MA_InitBuffer(&gMA.recvBuf, gMA.hwCondition);
        MABIOS_CheckStatus(&gMA.recvBuf);
        gMA.taskStep++;
        break;

    case 3:
        if (gMA.recvBuf.data[0] == 0xff) {
            gMA.taskError = MAAPIE_CALL_FAILED;
            gMA.taskErrorDetail = 0;
            gMA.taskStep = 0xfa;
            break;
        }
        if (gMA.prevBufHasEEPROMData == FALSE) {
            MA_InitBuffer(&gMA.recvBuf, gMA.recvPacket);
            MABIOS_EEPROM_Read(&gMA.recvBuf, 0, 0x80);
            gMA.taskStep++;
            break;
        }
        gMA.taskStep = 6;
        break;

    case 4:
        if (!EEPROMRegistrationCheck(&gMA.recvPacket[1])) {
            gMA.taskError = MAAPIE_REGISTRATION;
            gMA.taskErrorDetail = 0;
            gMA.taskStep = 0xfa;
            break;
        }
        MAU_memcpy(gMA.prevBuf, &gMA.recvBuf.data[1], 0x80);
        MA_InitBuffer(&gMA.recvBuf, gMA.recvPacket);
        MABIOS_EEPROM_Read(&gMA.recvBuf, 0x80, 0x40);
        gMA.taskStep++;
        break;

    case 5:
        MAU_memcpy(&gMA.prevBuf[0x80], &gMA.recvBuf.data[1], 0x40);
        if (!EEPROMSumCheck(gMA.prevBuf)) {
            gMA.taskError = MAAPIE_EEPROM_SUM;
            gMA.taskErrorDetail = 0;
            gMA.taskStep = 0xfa;
            break;
        }
        gMA.prevBufHasEEPROMData = TRUE;
        gMA.taskStep++;
        break;

    case 6:
        MAU_memcpy(&gMA.dnsConf, &gMA.prevBuf[EEPROM_DNS_OFFSET],
            EEPROM_DNS_SIZE);
        MAU_memcpy(&gMA.serverConf, &gMA.prevBuf[EEPROM_SERVER_OFFSET],
            EEPROM_SERVER_SIZE);
        gMA.taskStep++;
        break;

    case 7:
        MABIOS_Tel(MA_GetCallTypeFromHarwareType(gMA.hardwareType), param.pTelNo);
        gMA.taskStep++;
        break;

    case 8:
        MA_InitBuffer(&gMA.recvBuf, gMA.recvPacket);
        MABIOS_PPPConnect(&gMA.recvBuf, param.pUserID, param.pPassword,
            gMA.dnsConf.dns1, gMA.dnsConf.dns2);
        gMA.taskStep++;
        break;

    case 9:
        gMA.localAddr[0] = gMA.recvBuf.data[0];
        gMA.localAddr[1] = gMA.recvBuf.data[1];
        gMA.localAddr[2] = gMA.recvBuf.data[2];
        gMA.localAddr[3] = gMA.recvBuf.data[3];
        gMA.connMode = CONN_PPP;
        MA_SetCondition(MA_CONDITION_PPP);
        MA_TaskSet(TASK_NONE, 0);
        break;

    case 0xf9:
        MABIOS_Offline();
        gMA.taskStep++;
        break;

    case 0xfa:
        MABIOS_End();
        gMA.taskStep++;
        break;

    case 0xfb:
        MA_Reset();
        MA_SetApiError(gMA.taskError, gMA.taskErrorDetail);
        MA_TaskSet(TASK_NONE, 0);
        break;
    }
#undef param
}

void MA_Tel(const char *pTelNo)
{
#define param gMA.param.tel
    int len;

    SetApiCallFlag();
    if (!MA_ApiPreExe(TASK_TEL)) {
        ResetApiCallFlag();
        return;
    }

    len = MAU_strlen(pTelNo);
    if (len > MAX_TELNO_LEN || len == 0) {
        MA_SetApiError(MAAPIE_ILLEGAL_PARAMETER, 0);
        ResetApiCallFlag();
        return;
    }

    param.pTelNo = pTelNo;
    *(vu32 *)REG_TM3CNT = 0;
    MA_TaskSet(TASK_TEL, 0);

    ResetApiCallFlag();

    if (!(*(vu32 *)REG_TM3CNT & TMR_IF_ENABLE)
        || !(*(vu32 *)REG_TM3CNT & TMR_ENABLE)) {
        MAAPI_Main();
    }
#undef param
}

static void MATASK_Tel(void)
{
#define param gMA.param.tel
    if (gMA.recvCmd == (MAPROT_REPLY | MACMD_ERROR)) {
        switch (gMA.negaResCmd) {
        case MACMD_END:
            break;

        case MACMD_TEL:
        case MACMD_CHECK_STATUS:
        case MACMD_CHANGE_CLOCK:
        case MACMD_EEPROM_READ:
            MA_DefaultNegaResProc();
            gMA.taskStep = 0xfa;
            break;

        default:
            MA_DefaultNegaResProc();
            gMA.taskStep = 0xfb;
            break;
        }
    }

    switch (gMA.taskStep) {
    case 0:
        MABIOS_Start();
        gMA.taskStep++;
        break;

    case 1:
        gMA.taskStep++;
        MABIOS_ChangeClock(MA_SIO_WORD);
        break;

    case 2:
        MA_InitBuffer(&gMA.recvBuf, gMA.hwCondition);
        MABIOS_CheckStatus(&gMA.recvBuf);
        gMA.taskStep++;
        break;

    case 3:
        if (gMA.recvBuf.data[0] == 0xff) {
            gMA.taskError = MAAPIE_CALL_FAILED;
            gMA.taskErrorDetail = 0;
            gMA.taskStep = 0xfa;
            break;
        }

        MABIOS_Tel(MA_GetCallTypeFromHarwareType(gMA.hardwareType), param.pTelNo);
        gMA.taskStep++;
        break;

    case 4:
        gMA.status |= STATUS_CONN_PTP;
        gMA.connMode = CONN_P2P_SEND;
        MA_SetCondition(MA_CONDITION_P2P_SEND);
        param.pTelNo = NULL;
        gMA.recvBuf.size = 0;
        InitPrevBuf();
        MA_TaskSet(TASK_NONE, 0);
        break;

    case 0xfa:
        MABIOS_End();
        gMA.taskStep++;
        break;

    case 0xfb:
        MA_Reset();
        MA_SetApiError(gMA.taskError, gMA.errorDetail);
        MA_TaskSet(TASK_NONE, 0);
        break;
    }
#undef param
}

void MA_Receive(void)
{
    SetApiCallFlag();
    if (!MA_ApiPreExe(TASK_RECEIVE)) {
        ResetApiCallFlag();
        return;
    }

    *(vu32 *)REG_TM3CNT = 0;
    MA_TaskSet(TASK_RECEIVE, 0);

    ResetApiCallFlag();

    if (!(*(vu32 *)REG_TM3CNT & TMR_IF_ENABLE)
        || !(*(vu32 *)REG_TM3CNT & TMR_ENABLE)) {
        MAAPI_Main();
    }
}

static void MATASK_Receive(void)
{
#define param gMA.param.tel
    if (gMA.recvCmd == (MAPROT_REPLY | MACMD_ERROR)) {
        switch (gMA.negaResCmd) {
        case MACMD_END:
            break;

        case MACMD_WAIT_CALL:
            if (gMA.negaResErr != 0) {
                MA_DefaultNegaResProc();
                gMA.taskStep = 0xfa;
            }
            break;

        case MACMD_CHANGE_CLOCK:
        case MACMD_EEPROM_READ:
            MA_DefaultNegaResProc();
            gMA.taskStep = 0xfa;
            break;

        default:
            MA_DefaultNegaResProc();
            gMA.taskStep = 0xfb;
            break;
        }
    }

    switch (gMA.taskStep) {
    case 0:
        MABIOS_Start();
        gMA.taskStep++;
        break;

    case 1:
        gMA.taskStep++;
        MABIOS_ChangeClock(MA_SIO_WORD);
        break;

    case 2:
        MABIOS_WaitCall();
        gMA.taskStep++;
        break;

    case 3:
        if (gMA.recvCmd != (MAPROT_REPLY | MACMD_WAIT_CALL)) {
            gMA.taskStep--;
        } else {
            gMA.taskStep++;
        }
        break;

    case 4:
        gMA.status |= STATUS_CONN_PTP;
        gMA.connMode = CONN_P2P_RECV;
        MA_SetCondition(MA_CONDITION_P2P_RECV);
        param.pTelNo = NULL;
        gMA.recvBuf.size = 0;
        InitPrevBuf();
        MA_TaskSet(TASK_NONE, 0);
        break;

    case 0xfa:
        MABIOS_End();
        gMA.taskStep++;
        break;

    case 0xfb:
        MA_Reset();
        MA_SetApiError(gMA.taskError, gMA.errorDetail);
        MA_TaskSet(TASK_NONE, 0);
        break;
    }
#undef param
}

void MA_SData(const u8 *pSendData, u8 sendSize, u8 *pResult)
{
#define param gMA.param.p2p
    SetApiCallFlag();
    *pResult = FALSE;
    if (!MA_ApiPreExe(TASK_SDATA)) {
        ResetApiCallFlag();
        return;
    }

    if (sendSize == 0 || sendSize > MAX_P2P_DATA_SIZE) {
        MA_SetApiError(MAAPIE_ILLEGAL_PARAMETER, 0);
        ResetApiCallFlag();
        return;
    }

    if (gMA.condition & MA_CONDITION_PTP_GET) {
        *pResult = TRUE;
        gMA.condition &= ~MA_CONDITION_APIWAIT;
        ResetApiCallFlag();
        return;
    }

    param.pSendData = pSendData;
    param.sendSize = sendSize;
    gMA.status |= STATUS_PTP_SEND;
    ResetApiCallFlag();
#undef param
}

void MA_GData(u8 *pRecvData, u8 *pRecvSize)
{
    int i;
    u8 bufSize;

    SetApiCallFlag();
    if (!MA_ApiPreExe(TASK_GDATA)) {
        ResetApiCallFlag();
        return;
    }

    if (!(gMA.condition & MA_CONDITION_PTP_GET)) {
        MA_SetApiError(MAAPIE_CANNOT_EXECUTE, 0);
        ResetApiCallFlag();
        return;
    }

    MAU_memcpy(pRecvData, &gMA.prevBuf[1], gMA.prevBuf[0]);
    *pRecvSize = gMA.prevBuf[0];

    bufSize = gMA.prevBuf[0] + 1;
    for (i = 0; i < gMA.prevBufSize - bufSize; i++) {
        gMA.prevBuf[i] = gMA.prevBuf[bufSize + i];
    }
    gMA.prevBufSize -= bufSize;

    if (gMA.prevBufSize != 0) {
        if (gMA.prevBuf[0] == 0 || gMA.prevBuf[0] > MAX_P2P_DATA_SIZE) {
            gMA.prevBuf[0] = MAX_P2P_DATA_SIZE;
        }

        if (gMA.prevBufSize >= gMA.prevBuf[0] + 1) {
            gMA.condition |= MA_CONDITION_PTP_GET;
        } else {
            gMA.condition &= ~MA_CONDITION_PTP_GET;
        }
    } else {
        gMA.condition &= ~MA_CONDITION_PTP_GET;
    }
    gMA.condition &= ~MA_CONDITION_APIWAIT;
    ResetApiCallFlag();
}

static void MATASK_P2P(void)
{
    if (gMA.status & STATUS_PTP_SEND_DONE) {
        gMA.status &= ~STATUS_PTP_SEND_DONE;
        gMA.status &= ~STATUS_PTP_SEND;
        gMA.condition &= ~MA_CONDITION_APIWAIT;
    }

    if (gMA.recvCmd == (MAPROT_REPLY | MACMD_ERROR)) {
        switch (gMA.negaResCmd) {
        case MACMD_DATA:
            MA_ChangeSIOMode(MA_SIO_BYTE);
            MA_Reset();
            MA_SetApiError(MAAPIE_OFFLINE, 0);
            MA_TaskSet(TASK_NONE, 0);
            return;
        }
    }

    if (gMA.recvBuf.size <= 1) return;

    ConcatPrevBuf(&gMA.recvBuf.data[1], gMA.recvBuf.size - 1);

    gMA.recvBuf.data = NULL;
    gMA.recvBuf.size = 0;
    if (gMA.prevBuf[0] == 0 || gMA.prevBuf[0] > MAX_P2P_DATA_SIZE) {
        gMA.prevBuf[0] = MAX_P2P_DATA_SIZE;
    }

    if (gMA.prevBufSize >= gMA.prevBuf[0] + 1) {
        gMA.condition |= MA_CONDITION_PTP_GET;
    }
}

void MA_Condition(u8 *pCondition)
{
    MA_ConditionMain(pCondition, TASK_CONDITION);
}

void MA_Condition2(u8 *pCondition)
{
    MA_ConditionMain(pCondition, TASK_CONDITION2);
}

void MA_ConditionMain(u8 *pCondition, int task)
{
#define param gMA.param.condition
    SetApiCallFlag();
    if (!MA_ApiPreExe(task)) {
        ResetApiCallFlag();
        return;
    }

    param.pCondition = pCondition;
    if (gMA.connMode == CONN_OFFLINE) {
        param.offline = TRUE;
        *(vu32 *)REG_TM3CNT = 0;
        MA_TaskSet(task, 0);
    } else {
        param.offline = FALSE;
        MA_TaskSet(task, 1);
    }

    ResetApiCallFlag();
    if (gMA.connMode != CONN_OFFLINE) return;

    if (!(*(vu32 *)REG_TM3CNT & TMR_IF_ENABLE)
        || !(*(vu32 *)REG_TM3CNT & TMR_ENABLE)) {
        MAAPI_Main();
    }
#undef param
}

static void MATASK_Condition(void)
{
#define param gMA.param.condition
    if (gMA.recvCmd == (MAPROT_REPLY | MACMD_ERROR)) {
        switch (gMA.negaResCmd) {
        case MACMD_END:
            break;

        case MACMD_CHECK_STATUS:
            MA_DefaultNegaResProc();
            if (param.offline == TRUE) {
                gMA.taskStep = 0xfa;
            } else {
                gMA.taskStep = 0xfc;
            }
            break;

        default:
            MA_DefaultNegaResProc();
            gMA.taskStep = 0xfb;
            break;
        }
    }

    switch (gMA.taskStep) {
    case 0:
        MABIOS_Start();
        gMA.taskStep++;
        break;

    case 1:
        MA_InitBuffer(&gMA.recvBuf, gMA.hwCondition);
        MABIOS_CheckStatus(&gMA.recvBuf);
        gMA.taskStep++;
        break;

    case 2:
        *param.pCondition = MA_ProcessCheckStatusResponse(gMA.recvBuf.data[0]);
        if (gMA.hwCondition[2] >= 0xf0) {
            *param.pCondition |= MA_CONDITION_UNMETERED_F;
        }

        if (param.offline == TRUE) {
            gMA.taskStep = 3;
        } else {
            MA_TaskSet(TASK_NONE, 0);
        }
        break;

    case 3:
        MABIOS_End();
        gMA.taskStep++;
        break;

    case 4:
        MA_Reset();
        MA_TaskSet(TASK_NONE, 0);
        break;

    case 0xfa:
        MABIOS_End();
        gMA.taskStep++;
        break;

    case 0xfb:
        MA_Reset();
        MA_SetApiError(gMA.taskError, gMA.taskErrorDetail);
        MA_TaskSet(TASK_NONE, 0);
        break;

    case 0xfc:
        MA_SetApiError(gMA.taskError, gMA.taskErrorDetail);
        MA_TaskSet(TASK_NONE, 0);
        break;
    }
#undef param
}

void MA_Offline(void)
{
    SetApiCallFlag();
    if (gMA.connMode == CONN_OFFLINE || !MA_ApiPreExe(TASK_OFFLINE)) {
        ResetApiCallFlag();
        return;
    }

    if (gMA.connMode == CONN_P2P_SEND || gMA.connMode == CONN_P2P_RECV) {
        MA_TaskSet(TASK_OFFLINE, 1);
    } else if (gMA.connMode == CONN_SMTP || gMA.connMode == CONN_POP3) {
        MA_TaskSet(TASK_OFFLINE, 100);
    } else {
        MA_TaskSet(TASK_OFFLINE, 0);
    }
    ResetApiCallFlag();
}

static void MATASK_Offline(void)
{
#define param gMA.param.offline
    if (gMA.recvCmd == (MAPROT_REPLY | MACMD_ERROR)) {
        switch (gMA.negaResCmd) {
        case MACMD_DATA:
            gMA.taskStep = 0;
            break;
        }
    }

    switch (gMA.taskStep) {
    case 0:
        if (gMA.connMode != CONN_P2P_SEND && gMA.connMode != CONN_P2P_RECV) {
            MABIOS_PPPDisconnect();
        }
        gMA.taskStep++;
        break;

    case 1:
        MABIOS_Offline();
        gMA.taskStep++;
        break;

    case 2:
        MABIOS_End();
        gMA.taskStep++;
        break;

    case 3:
        gMA.condition &= ~MA_CONDITION_PTP_GET;
        gMA.status &= ~STATUS_CONN_PTP;
        MA_Reset();
        MA_TaskSet(TASK_NONE, 0);
        MA_ChangeSIOMode(MA_SIO_BYTE);
        break;

    case 100:
        InitPrevBuf();
        MAU_strcpy(gMA.strBuf, "QUIT\r\n");
        MA_InitBuffer(&gMA.recvBuf, gMA.recvPacket);
        MABIOS_Data(&gMA.recvBuf, gMA.strBuf, MAU_strlen(gMA.strBuf),
            gMA.sockets[0]);
        gMA.taskStep++;
        break;

    case 101:
        ConcatPrevBuf(gMA.recvBuf.data + 1, gMA.recvBuf.size - 1);
        if (!MAU_CheckCRLF(gMA.prevBuf, gMA.prevBufSize)) {
            MA_InitBuffer(&gMA.recvBuf, gMA.recvPacket);
            MABIOS_Data(&gMA.recvBuf, NULL, 0, gMA.sockets[0]);
            break;
        }
        param.timeout = gMA.timeout200msecCounter[gMA.sioMode];
        gMA.taskStep++;
        break;

    case 102:
        if (--param.timeout != 0) break;
        MABIOS_TCPDisconnect(&gMA.recvBuf, gMA.sockets[0]);
        gMA.taskStep++;
        break;

    case 103:
        gMA.connMode = CONN_PPP;
        MA_SetCondition(MA_CONDITION_PPP);
        gMA.sockets[0] = 0;
        gMA.usedSockets[0] = FALSE;
        gMA.taskStep = 0;
        break;
    }
#undef param
}

static int CheckSMTPResponse(const char *response)
{
    if (CheckResponse(response)) {
        return 0;
    } else {
        return 1;
    }
}

void MA_SMTP_Connect(const char *pMailAddress)
{
#define param gMA.param.smtp_connect
    int len;

    SetApiCallFlag();
    if (!MA_ApiPreExe(TASK_SMTP_CONNECT)) {
        ResetApiCallFlag();
        return;
    }

    len = MAU_strlen(pMailAddress);
    if (len > MAX_EMAIL_LEN || len == 0) {
        MA_SetApiError(MAAPIE_ILLEGAL_PARAMETER, 0);
        ResetApiCallFlag();
        return;
    }

    MA_GetSMTPServerName(gMA.strBuf);
    param.pMailAddress = pMailAddress;
    MA_TaskSet(TASK_SMTP_CONNECT, 0);
    ResetApiCallFlag();
#undef param
}

static void MATASK_SMTP_Connect(void)
{
#define param gMA.param.smtp_connect
    static char *cp1;
    static const char *cp2;
    static int smtpRes;

    if (gMA.recvCmd == (MAPROT_REPLY | MACMD_ERROR)) {
        switch (gMA.negaResCmd) {
        case MACMD_DATA:
            gMA.taskError = MAAPIE_TCP_CONNECT;
            gMA.taskErrorDetail = 0;
            gMA.taskStep = 0xf0;
            break;

        case MACMD_TCP_CONNECT:
        case MACMD_DNS_REQUEST:
            gMA.taskError = MAAPIE_TCP_CONNECT;
            gMA.taskErrorDetail = 0;
            gMA.taskStep = 0xf1;
            break;

        case MACMD_TCP_DISCONNECT:
            break;

        default:
            MA_DefaultNegaResProc();
            gMA.taskStep = 0xf1;
            break;
        }
    }

    if (MA_GetCondition() & MA_CONDITION_TCPCLOSED) {
        gMA.connMode = CONN_PPP;
        MA_SetCondition(MA_CONDITION_PPP);
        gMA.sockets[0] = 0;
        gMA.usedSockets[0] = FALSE;
        MA_SetApiError(MAAPIE_SMTP, 0);
        MA_TaskSet(TASK_NONE, 0);
        return;
    }

    switch (gMA.taskStep) {
    case 0:
        MA_InitBuffer(&gMA.recvBuf, gMA.recvPacket);
        MABIOS_DNSRequest(&gMA.recvBuf, gMA.strBuf);
        gMA.taskStep++;
        break;

    case 1:
        MAU_memcpy(gMA.socketAddr, gMA.recvBuf.data, sizeof(gMA.socketAddr));
        MA_InitBuffer(&gMA.recvBuf, gMA.recvPacket);
        MABIOS_TCPConnect(&gMA.recvBuf, gMA.socketAddr, PORT_SMTP);
        gMA.taskStep++;
        break;

    case 2:
        gMA.sockets[0] = gMA.recvBuf.data[0];
        gMA.usedSockets[0] = TRUE;
        InitPrevBuf();
        MA_InitBuffer(&gMA.recvBuf, gMA.recvPacket);
        MABIOS_Data(&gMA.recvBuf, NULL, 0, gMA.sockets[0]);
        gMA.taskStep++;
        break;

    case 3:
        ConcatPrevBuf(&gMA.recvBuf.data[1], gMA.recvBuf.size - 1);
        if (!MAU_CheckCRLF(gMA.prevBuf, gMA.prevBufSize)) {
            MA_InitBuffer(&gMA.recvBuf, gMA.recvPacket);
            MABIOS_Data(&gMA.recvBuf, NULL, 0, gMA.sockets[0]);
            break;
        }

        smtpRes = CheckSMTPResponse(gMA.prevBuf);
        if (smtpRes == 0) {
            gMA.errorDetail = GetResponse(gMA.prevBuf);
            if (gMA.errorDetail == 220) {
                MAU_strcpy(gMA.strBuf, "HELO ");
                cp1 = &gMA.strBuf[5];
                cp2 = param.pMailAddress;
                while (*cp2 && *cp2 != '@') *cp1++ = *cp2++;
                *cp1 = '\0';
                MAU_strcat(gMA.strBuf, "\r\n");

                InitPrevBuf();
                MA_InitBuffer(&gMA.recvBuf, gMA.recvPacket);
                MABIOS_Data(&gMA.recvBuf, gMA.strBuf, MAU_strlen(gMA.strBuf),
                    gMA.sockets[0]);
                gMA.taskStep++;
                break;
            }

            gMA.taskError = MAAPIE_SMTP;
            gMA.taskErrorDetail = gMA.errorDetail;
            gMA.taskStep = 0xf0;
            break;
        }

        gMA.taskError = MAAPIE_SMTP;
        gMA.taskErrorDetail = 0;
        gMA.taskStep = 0xf0;
        break;

    case 4:
        ConcatPrevBuf(&gMA.recvBuf.data[1], gMA.recvBuf.size - 1);
        if (!MAU_CheckCRLF(gMA.prevBuf, gMA.prevBufSize)) {
            (&gMA.recvBuf)->size = 0;
            (&gMA.recvBuf)->data = gMA.recvPacket;
            MABIOS_Data(&gMA.recvBuf, NULL, 0, gMA.sockets[0]);
            break;
        }

        smtpRes = CheckSMTPResponse(gMA.prevBuf);
        if (smtpRes == 0) {
            gMA.errorDetail = GetResponse(gMA.prevBuf);
            if (gMA.errorDetail == 250) {
                gMA.taskStep++;
                break;
            }

            gMA.taskError = MAAPIE_SMTP;
            gMA.taskErrorDetail = gMA.errorDetail;
            gMA.taskStep = 0xf0;
            break;
        }

        gMA.taskError = MAAPIE_SMTP;
        gMA.taskErrorDetail = 0;
        gMA.taskStep = 0xf0;
        break;

    case 5:
        gMA.connMode = CONN_SMTP;
        MA_SetCondition(MA_CONDITION_SMTP);
        MA_TaskSet(TASK_NONE, 0);
        break;

    case 0xf0:
        MA_InitBuffer(&gMA.recvBuf, gMA.recvPacket);
        MABIOS_TCPDisconnect(&gMA.recvBuf, gMA.sockets[0]);
        gMA.taskStep++;
        break;

    case 0xf1:
        gMA.connMode = CONN_PPP;
        MA_SetCondition(MA_CONDITION_PPP);
        MA_SetApiError(gMA.taskError, gMA.taskErrorDetail);
        MA_TaskSet(TASK_NONE, 0);
        gMA.usedSockets[0] = FALSE;
        break;
    }
#undef param
}

void MA_SMTP_Sender(const char * const pRecipients[])
{
#define param gMA.param.smtp_sender
    int i;
    int len;

    SetApiCallFlag();
    if (!MA_ApiPreExe(TASK_SMTP_SENDER)) {
        ResetApiCallFlag();
        return;
    }

    if (pRecipients[0] == NULL || pRecipients[1] == NULL) {
        MA_SetApiError(MAAPIE_ILLEGAL_PARAMETER, 0);
        ResetApiCallFlag();
        return;
    }

    len = MAU_strlen(pRecipients[0]);
    if (len > MAX_EMAIL_LEN || len == 0) {
        MA_SetApiError(MAAPIE_ILLEGAL_PARAMETER, 0);
        ResetApiCallFlag();
        return;
    }

    for (i = 1; pRecipients[i] != NULL; i++) {
        len = MAU_strlen(pRecipients[i]);
        if (len == 0 || len >= MAX_RECIPIENT_LEN || i > MAX_RECIPIENTS) {
            MA_SetApiError(MAAPIE_ILLEGAL_PARAMETER, 0);
            ResetApiCallFlag();
            return;
        }
    }

    param.pRecipients = pRecipients;
    MA_TaskSet(TASK_SMTP_SENDER, 0);
    ResetApiCallFlag();
#undef param
}

static void MATASK_SMTP_Sender(void)
{
#define param gMA.param.smtp_sender
    static int smtpRes;

    if (gMA.recvCmd == (MAPROT_REPLY | MACMD_ERROR)) {
        switch (gMA.negaResCmd) {
        case MACMD_DATA:
            gMA.taskError = MAAPIE_TCP_CONNECT;
            gMA.taskErrorDetail = 0;
            gMA.taskStep = 0xf0;
            break;

        case MACMD_TCP_DISCONNECT:
            break;

        default:
            MA_DefaultNegaResProc();
            gMA.taskStep = 0xf1;
            break;
        }
    }

    if (MA_GetCondition() & MA_CONDITION_TCPCLOSED) {
        gMA.connMode = CONN_PPP;
        MA_SetCondition(MA_CONDITION_PPP);
        gMA.sockets[0] = 0;
        gMA.usedSockets[0] = FALSE;
        MA_SetApiError(MAAPIE_SMTP, 0);
        MA_TaskSet(TASK_NONE, 0);
        return;
    }

    switch (gMA.taskStep) {
    case 0:
        MAU_strcpy(gMA.strBuf, "MAIL FROM:<");
        MAU_strcat(gMA.strBuf, *param.pRecipients);
        MAU_strcat(gMA.strBuf, ">\r\n");
        param.pRecipients++;
        InitPrevBuf();
        MA_InitBuffer(&gMA.recvBuf, gMA.recvPacket);
        MABIOS_Data(&gMA.recvBuf, gMA.strBuf, MAU_strlen(gMA.strBuf),
            gMA.sockets[0]);
        gMA.taskStep++;
        break;

    case 1:
        ConcatPrevBuf(gMA.recvBuf.data + 1, gMA.recvBuf.size - 1);
        if (!MAU_CheckCRLF(gMA.prevBuf, gMA.prevBufSize)) {
            MA_InitBuffer(&gMA.recvBuf, gMA.recvPacket);
            MABIOS_Data(&gMA.recvBuf, NULL, 0, gMA.sockets[0]);
            break;
        }

        smtpRes = CheckSMTPResponse(gMA.prevBuf);
        if (smtpRes == 0) {
            gMA.errorDetail = GetResponse(gMA.prevBuf);
            if (gMA.errorDetail == 250) {
                gMA.taskStep++;
                break;
            }

            gMA.taskError = MAAPIE_SMTP;
            gMA.taskErrorDetail = gMA.errorDetail;
            gMA.taskStep = 0xf0;
            break;
        }

        gMA.taskError = MAAPIE_SMTP;
        gMA.taskErrorDetail = 0;
        gMA.taskStep = 0xf0;
        break;

    case 2:
        if (*param.pRecipients != NULL) {
            MAU_strcpy(gMA.strBuf, "RCPT TO:<");
            MAU_strcat(gMA.strBuf, *param.pRecipients);
            MAU_strcat(gMA.strBuf, ">\r\n");
            param.pRecipients++;
            InitPrevBuf();
            MA_InitBuffer(&gMA.recvBuf, gMA.recvPacket);
            MABIOS_Data(&gMA.recvBuf, gMA.strBuf, MAU_strlen(gMA.strBuf),
                gMA.sockets[0]);
            gMA.taskStep++;
            break;
        }
        gMA.taskStep = 4;
        break;

    case 3:
        ConcatPrevBuf(gMA.recvBuf.data + 1, gMA.recvBuf.size - 1);
        if (!MAU_CheckCRLF(gMA.prevBuf, gMA.prevBufSize)) {
            MA_InitBuffer(&gMA.recvBuf, gMA.recvPacket);
            MABIOS_Data(&gMA.recvBuf, NULL, 0, gMA.sockets[0]);
            break;
        }

        smtpRes = CheckSMTPResponse(gMA.prevBuf);
        if (smtpRes == 0) {
            gMA.errorDetail = GetResponse(gMA.prevBuf);
            if (gMA.errorDetail == 250) {
                gMA.taskStep = 2;
                break;
            }

            gMA.taskError = MAAPIE_SMTP;
            gMA.taskErrorDetail = gMA.errorDetail;
            gMA.taskStep = 0xf0;
            break;
        }

        gMA.taskError = MAAPIE_SMTP;
        gMA.taskErrorDetail = 0;
        gMA.taskStep = 0xf0;
        break;

    case 4:
        MA_TaskSet(TASK_NONE, 0);
        gMA.smtpSender = TRUE;
        break;

    case 0xf0:
        MA_InitBuffer(&gMA.recvBuf, gMA.recvPacket);
        MABIOS_TCPDisconnect(&gMA.recvBuf, gMA.sockets[0]);
        gMA.taskStep++;
        break;

    case 0xf1:
        gMA.connMode = CONN_PPP;
        MA_SetCondition(MA_CONDITION_PPP);
        MA_SetApiError(gMA.taskError, gMA.taskErrorDetail);
        MA_TaskSet(TASK_NONE, 0);
        gMA.usedSockets[0] = FALSE;
        break;
    }
#undef param
}

void MA_SMTP_Send(const char *pSendData, u16 sendSize, int endFlag)
{
#define param gMA.param.smtp_send
    SetApiCallFlag();
    if (!MA_ApiPreExe(TASK_SMTP_SEND)) {
        ResetApiCallFlag();
        return;
    }

    if (sendSize == 0) {
        MA_SetApiError(MAAPIE_ILLEGAL_PARAMETER, 0);
        ResetApiCallFlag();
        return;
    }

    if (gMA.smtpSender == TRUE) param.totalSize = 0;
    param.pSendData = pSendData;
    param.sendSize = sendSize;
    param.endFlag = endFlag;
    param.totalSize += sendSize;
    MA_TaskSet(TASK_SMTP_SEND, 0);
    ResetApiCallFlag();
#undef param
}

static void MATASK_SMTP_Send(void)
{
#define param gMA.param.smtp_send
    static int smtpRes;

    if (gMA.recvCmd == (MAPROT_REPLY | MACMD_ERROR)) {
        switch (gMA.negaResCmd) {
        case MACMD_DATA:
            gMA.taskError = MAAPIE_TCP_CONNECT;
            gMA.taskErrorDetail = 0;
            gMA.taskStep = 0xf0;
            break;

        case MACMD_TCP_DISCONNECT:
            break;

        default:
            MA_DefaultNegaResProc();
            gMA.taskStep = 0xf1;
            break;
        }
    }

    if (MA_GetCondition() & MA_CONDITION_TCPCLOSED) {
        gMA.connMode = CONN_PPP;
        MA_SetCondition(MA_CONDITION_PPP);
        gMA.sockets[0] = 0;
        gMA.usedSockets[0] = FALSE;
        MA_SetApiError(MAAPIE_SMTP, 0);
        MA_TaskSet(TASK_NONE, 0);
        return;
    }

    switch (gMA.taskStep) {
    case 0:
        if (gMA.smtpSender == TRUE) {
            gMA.smtpSender = FALSE;
            gMA.taskStep = 100;
            break;
        }
        gMA.taskStep++;

    case 1:
        if (param.sendSize > MAX_TCP_DATA_SIZE) {
            MA_InitBuffer(&gMA.recvBuf, gMA.recvPacket);
            MABIOS_Data(&gMA.recvBuf, param.pSendData, MAX_TCP_DATA_SIZE,
                gMA.sockets[0]);
            param.pSendData += MAX_TCP_DATA_SIZE;
            param.sendSize -= MAX_TCP_DATA_SIZE;
            param.nextStep = gMA.taskStep;
            gMA.taskStep = 50;
            break;
        }
        InitPrevBuf();
        MA_InitBuffer(&gMA.recvBuf, gMA.recvPacket);
        MABIOS_Data(&gMA.recvBuf, param.pSendData, param.sendSize,
            gMA.sockets[0]);
        if (param.endFlag == TRUE) {
            param.nextStep = gMA.taskStep + 1;
            gMA.taskStep = 50;
            break;
        }

        param.nextStep = 3;
        gMA.taskStep = 50;
        break;

    case 2:
        ConcatPrevBuf(&gMA.recvBuf.data[1], gMA.recvBuf.size - 1);
        if (!MAU_CheckCRLF(gMA.prevBuf, gMA.prevBufSize)) {
            MA_InitBuffer(&gMA.recvBuf, gMA.recvPacket);
            MABIOS_Data(&gMA.recvBuf, NULL, 0, gMA.sockets[0]);
            break;
        }

        smtpRes = CheckSMTPResponse(gMA.prevBuf);
        if (smtpRes == 0) {
            gMA.errorDetail = GetResponse(gMA.prevBuf);
            if (gMA.errorDetail == 250) {
                gMA.taskStep++;
                break;
            }

            gMA.taskError = MAAPIE_SMTP;
            gMA.taskErrorDetail = gMA.errorDetail;
            gMA.taskStep = 0xf0;
            break;
        }

        gMA.taskError = MAAPIE_SMTP;
        gMA.taskErrorDetail = 0;
        gMA.taskStep = 0xf0;
        break;

    case 3:
        MA_TaskSet(TASK_NONE, 0);
        break;

    case 50:
        param.timeout = gMA.tcpDelayCounter[gMA.sioMode];
        gMA.taskStep++;

    case 51:
        if (param.timeout-- == 0) gMA.taskStep = param.nextStep;
        break;

    case 100:
        MAU_strcpy(gMA.strBuf, "DATA\r\n");
        InitPrevBuf();
        MA_InitBuffer(&gMA.recvBuf, gMA.recvPacket);
        MABIOS_Data(&gMA.recvBuf, gMA.strBuf, MAU_strlen(gMA.strBuf),
            gMA.sockets[0]);
        gMA.taskStep++;
        param.nextStep = gMA.taskStep;
        gMA.taskStep = 50;
        break;

    case 101:
        ConcatPrevBuf(&gMA.recvBuf.data[1], gMA.recvBuf.size - 1);
        if (!MAU_CheckCRLF(gMA.prevBuf, gMA.prevBufSize)) {
            MA_InitBuffer(&gMA.recvBuf, gMA.recvPacket);
            MABIOS_Data(&gMA.recvBuf, NULL, 0, gMA.sockets[0]);
            break;
        }

        smtpRes = CheckSMTPResponse(gMA.prevBuf);
        if (smtpRes == 0) {
            gMA.errorDetail = GetResponse(gMA.prevBuf);
            if (gMA.errorDetail == 354) {
                gMA.taskStep = 1;
                break;
            }

            gMA.taskError = MAAPIE_SMTP;
            gMA.taskErrorDetail = gMA.errorDetail;
            gMA.taskStep = 0xf0;
            break;
        }

        gMA.taskError = MAAPIE_SMTP;
        gMA.taskErrorDetail = 0;
        gMA.taskStep = 0xf0;
        break;

    case 0xf0:
        MA_InitBuffer(&gMA.recvBuf, gMA.recvPacket);
        MABIOS_TCPDisconnect(&gMA.recvBuf, gMA.sockets[0]);
        gMA.taskStep++;
        break;

    case 0xf1:
        gMA.connMode = CONN_PPP;
        MA_SetCondition(MA_CONDITION_PPP);
        MA_SetApiError(gMA.taskError, gMA.taskErrorDetail);
        MA_TaskSet(TASK_NONE, 0);
        gMA.usedSockets[0] = FALSE;
        break;
    }
#undef param
}

void MA_POP3_Quit(void)
{
    SetApiCallFlag();
    if (!MA_ApiPreExe(TASK_POP3_QUIT)) {
        ResetApiCallFlag();
        return;
    }

    MA_TaskSet(TASK_POP3_QUIT, 0);
    ResetApiCallFlag();
}

void MA_SMTP_Quit(void)
{
    SetApiCallFlag();
    if (!MA_ApiPreExe(TASK_SMTP_QUIT)) {
        ResetApiCallFlag();
        return;
    }

    MA_TaskSet(TASK_SMTP_QUIT, 0);
    ResetApiCallFlag();
}

static void MATASK_SMTP_POP3_Quit(void)
{
#define param gMA.param.smtp_pop3_quit
    if (gMA.recvCmd == (MAPROT_REPLY | MACMD_ERROR)) {
        switch (gMA.negaResCmd) {
        case MACMD_DATA:
            gMA.taskError = MAAPIE_TCP_CONNECT;
            gMA.taskErrorDetail = 0;
            gMA.taskStep = 0xf0;
            break;

        case MACMD_TCP_DISCONNECT:
            break;

        default:
            MA_DefaultNegaResProc();
            gMA.taskStep = 0xf1;
            break;
        }
    }

    if (MA_GetCondition() & MA_CONDITION_TCPCLOSED) {
        if (gMA.connMode == CONN_SMTP) {
            MA_SetApiError(MAAPIE_SMTP, 0);
        } else {
            MA_SetApiError(MAAPIE_POP3, 0);
        }
        gMA.connMode = CONN_PPP;
        MA_SetCondition(MA_CONDITION_PPP);
        gMA.sockets[0] = 0;
        gMA.usedSockets[0] = FALSE;
        MA_TaskSet(TASK_NONE, 0);
        return;
    }

    switch (gMA.taskStep) {
    case 0:
        InitPrevBuf();
        MAU_strcpy(gMA.strBuf, "QUIT\r\n");
        MA_InitBuffer(&gMA.recvBuf, gMA.recvPacket);
        MABIOS_Data(&gMA.recvBuf, gMA.strBuf, MAU_strlen(gMA.strBuf),
            gMA.sockets[0]);
        gMA.taskStep++;
        break;

    case 1:
        ConcatPrevBuf(gMA.recvBuf.data + 1, gMA.recvBuf.size - 1);
        if (!MAU_CheckCRLF(gMA.prevBuf, gMA.prevBufSize)) {
            MA_InitBuffer(&gMA.recvBuf, gMA.recvPacket);
            MABIOS_Data(&gMA.recvBuf, NULL, 0, gMA.sockets[0]);
            break;
        }

        param.timeout = gMA.timeout200msecCounter[gMA.sioMode];
        gMA.taskStep++;
        break;

    case 2:
        param.timeout--;
        if (param.timeout != 0) break;
        MABIOS_TCPDisconnect(&gMA.recvBuf, gMA.sockets[0]);
        gMA.taskStep++;
        break;

    case 3:
        gMA.connMode = CONN_PPP;
        MA_SetCondition(MA_CONDITION_PPP);
        gMA.sockets[0] = 0;
        gMA.usedSockets[0] = FALSE;
        MA_TaskSet(TASK_NONE, 0);
        break;

    case 0xf0:
        MA_InitBuffer(&gMA.recvBuf, gMA.recvPacket);
        MABIOS_TCPDisconnect(&gMA.recvBuf, gMA.sockets[0]);
        gMA.taskStep++;
        break;

    case 0xf1:
        gMA.connMode = CONN_PPP;
        MA_SetCondition(MA_CONDITION_PPP);
        MA_SetApiError(gMA.taskError, gMA.taskErrorDetail);
        MA_TaskSet(TASK_NONE, 0);
        gMA.usedSockets[0] = FALSE;
        break;
    }
#undef param
}

static int CheckPOP3Response(const char *response)
{
    if (response[0] == '+'
        && response[1] == 'O'
        && response[2] == 'K') {
        return 0;
    }

    if (response[0] == '-'
        && response[1] == 'E'
        && response[2] == 'R'
        && response[3] == 'R') {
        return 1;
    }

    return 2;
}

void MA_POP3_Connect(const char *pUserID, const char *pPassword)
{
#define param gMA.param.pop3_connect
    int len_userid, len_password;
    char *end;

    SetApiCallFlag();
    if (!MA_ApiPreExe(TASK_POP3_CONNECT)) {
        ResetApiCallFlag();
        return;
    }

    len_userid = MAU_strlen(pUserID);
    if (len_userid > MAX_EMAIL_LEN || len_userid == 0) {
        MA_SetApiError(MAAPIE_ILLEGAL_PARAMETER, 0);
        ResetApiCallFlag();
        return;
    }

    len_password = MAU_strlen(pPassword);
    if (len_password > MAX_PASSWORD_LEN || len_password == 0) {
        MA_SetApiError(MAAPIE_ILLEGAL_PARAMETER, 0);
        ResetApiCallFlag();
        return;
    }

    param.pServerName = gMA.strBuf;
    MA_GetPOP3ServerName(param.pServerName);

    param.pUserID = param.pServerName + MAU_strlen(param.pServerName) + 1;
    MAU_strcpy(param.pUserID, "USER ");
    MAU_strcat(param.pUserID, pUserID);

    end = param.pUserID;
    while (*end != '\0' && *end != '@') end++;
    *end++ = '\r';
    *end++ = '\n';
    *end++ = '\0';

    param.pPassword = end;
    MAU_strcpy(param.pPassword, "PASS ");
    MAU_strcat(param.pPassword, pPassword);
    MAU_strcat(param.pPassword, "\r\n");

    MA_TaskSet(TASK_POP3_CONNECT, 0);
    ResetApiCallFlag();
#undef param
}

static void MATASK_POP3_Connect(void)
{
#define param gMA.param.pop3_connect
    static int pop3res;

    if (gMA.recvCmd == (MAPROT_REPLY | MACMD_ERROR)) {
        switch (gMA.negaResCmd) {
        case MACMD_DATA:
            gMA.taskError = MAAPIE_TCP_CONNECT;
            gMA.taskErrorDetail = 0;
            gMA.taskStep = 0xf0;
            break;

        case MACMD_TCP_CONNECT:
        case MACMD_DNS_REQUEST:
            gMA.taskError = MAAPIE_TCP_CONNECT;
            gMA.taskErrorDetail = 0;
            gMA.taskStep = 0xf1;
            break;

        case MACMD_TCP_DISCONNECT:
            break;

        default:
            MA_DefaultNegaResProc();
            gMA.taskStep = 0xf1;
            break;
        }
    }

    if (MA_GetCondition() & MA_CONDITION_TCPCLOSED) {
        gMA.connMode = CONN_PPP;
        MA_SetCondition(MA_CONDITION_PPP);
        gMA.sockets[0] = 0;
        gMA.usedSockets[0] = FALSE;
        MA_SetApiError(MAAPIE_POP3, 0);
        MA_TaskSet(TASK_NONE, 0);
        return;
    }

    switch (gMA.taskStep) {
    case 0:
        MA_InitBuffer(&gMA.recvBuf, gMA.recvPacket);
        MABIOS_DNSRequest(&gMA.recvBuf, param.pServerName);
        gMA.taskStep++;
        break;

    case 1:
        MAU_memcpy(gMA.socketAddr, gMA.recvBuf.data, sizeof(gMA.socketAddr));
        MA_InitBuffer(&gMA.recvBuf, gMA.recvPacket);
        MABIOS_TCPConnect(&gMA.recvBuf, gMA.socketAddr, PORT_POP3);
        gMA.taskStep++;
        break;

    case 2:
        gMA.sockets[0] = gMA.recvBuf.data[0];
        gMA.usedSockets[0] = TRUE;
        InitPrevBuf();
        MA_InitBuffer(&gMA.recvBuf, gMA.recvPacket);
        MABIOS_Data(&gMA.recvBuf, NULL, 0, gMA.sockets[0]);
        gMA.taskStep++;
        break;

    case 3:
        ConcatPrevBuf(&gMA.recvBuf.data[1], gMA.recvBuf.size - 1);
        if (!MAU_CheckCRLF(gMA.prevBuf, gMA.prevBufSize)) {
            MA_InitBuffer(&gMA.recvBuf, gMA.recvPacket);
            MABIOS_Data(&gMA.recvBuf, NULL, 0, gMA.sockets[0]);
            break;
        }

        pop3res = CheckPOP3Response(gMA.prevBuf);
        if (pop3res == 0) {
            InitPrevBuf();
            MA_InitBuffer(&gMA.recvBuf, gMA.recvPacket);
            MABIOS_Data(&gMA.recvBuf, param.pUserID, MAU_strlen(param.pUserID),
                gMA.sockets[0]);
            gMA.taskStep++;
        } else if (pop3res == 1) {
            gMA.taskError = MAAPIE_POP3;
            gMA.taskErrorDetail = 0;
            gMA.taskStep = 0xf0;
        } else {
            gMA.taskError = MAAPIE_POP3;
            gMA.taskErrorDetail = 0;
            gMA.taskStep = 0xf0;
        }
        break;

    case 4:
        ConcatPrevBuf(&gMA.recvBuf.data[1], gMA.recvBuf.size - 1);
        if (!MAU_CheckCRLF(gMA.prevBuf, gMA.prevBufSize)) {
            MA_InitBuffer(&gMA.recvBuf, gMA.recvPacket);
            MABIOS_Data(&gMA.recvBuf, NULL, 0, gMA.sockets[0]);
            break;
        }

        pop3res = CheckPOP3Response(gMA.prevBuf);
        if (pop3res == 0) {
            InitPrevBuf();
            MA_InitBuffer(&gMA.recvBuf, gMA.recvPacket);
            MABIOS_Data(&gMA.recvBuf, param.pPassword,
                MAU_strlen(param.pPassword), gMA.sockets[0]);
            gMA.taskStep++;
        } else if (pop3res == 1) {
            gMA.taskError = MAAPIE_POP3;
            gMA.taskErrorDetail = 2;
            gMA.taskStep = 0xf0;
        } else {
            gMA.taskError = MAAPIE_POP3;
            gMA.taskErrorDetail = 0;
            gMA.taskStep = 0xf0;
        }
        break;

    case 5:
        ConcatPrevBuf(&gMA.recvBuf.data[1], gMA.recvBuf.size - 1);
        if (!MAU_CheckCRLF(gMA.prevBuf, gMA.prevBufSize)) {
            MA_InitBuffer(&gMA.recvBuf, gMA.recvPacket);
            MABIOS_Data(&gMA.recvBuf, NULL, 0, gMA.sockets[0]);
            break;
        }

        pop3res = CheckPOP3Response(gMA.prevBuf);
        if (pop3res == 0) {
            gMA.taskStep++;
        } else if (pop3res == 1) {
            gMA.taskError = MAAPIE_POP3;
            gMA.taskErrorDetail = 3;
            gMA.taskStep = 0xf0;
        } else {
            gMA.taskError = MAAPIE_POP3;
            gMA.taskErrorDetail = 0;
            gMA.taskStep = 0xf0;
        }
        break;

    case 6:
        gMA.connMode = CONN_POP3;
        MA_SetCondition(MA_CONDITION_POP3);
        MA_TaskSet(TASK_NONE, 0);
        break;

    case 0xf0:
        MA_InitBuffer(&gMA.recvBuf, gMA.recvPacket);
        MABIOS_TCPDisconnect(&gMA.recvBuf, gMA.sockets[0]);
        gMA.taskStep++;
        break;

    case 0xf1:
        gMA.connMode = CONN_PPP;
        MA_SetCondition(MA_CONDITION_PPP);
        MA_SetApiError(gMA.taskError, gMA.taskErrorDetail);
        MA_TaskSet(TASK_NONE, 0);
        gMA.usedSockets[0] = FALSE;
        break;
    }
#undef param
}

void MA_POP3_Stat(u16 *pNum, u32 *pSize)
{
#define param gMA.param.pop3_stat
    SetApiCallFlag();
    if (!MA_ApiPreExe(TASK_POP3_STAT)) {
        ResetApiCallFlag();
        return;
    }

    param.pNum = pNum;
    param.pSize = pSize;
    MA_TaskSet(TASK_POP3_STAT, 0);
    ResetApiCallFlag();
#undef param
}

static void MATASK_POP3_Stat(void)
{
#define param gMA.param.pop3_stat
    static const char *cp;
    static int pop3res;

    if (gMA.recvCmd == (MAPROT_REPLY | MACMD_ERROR)) {
        switch (gMA.negaResCmd) {
        case MACMD_DATA:
            gMA.taskError = MAAPIE_TCP_CONNECT;
            gMA.taskErrorDetail = 0;
            gMA.taskStep = 0xf0;
            break;

        case MACMD_TCP_DISCONNECT:
            break;

        default:
            MA_DefaultNegaResProc();
            gMA.taskStep = 0xf1;
            break;
        }
    }

    if (MA_GetCondition() & MA_CONDITION_TCPCLOSED) {
        gMA.connMode = CONN_PPP;
        MA_SetCondition(MA_CONDITION_PPP);
        gMA.sockets[0] = 0;
        gMA.usedSockets[0] = FALSE;
        MA_SetApiError(MAAPIE_POP3, 0);
        MA_TaskSet(TASK_NONE, 0);
        return;
    }

    switch (gMA.taskStep) {
    case 0:
        MAU_strcpy(gMA.strBuf, "STAT\r\n");
        InitPrevBuf();
        MA_InitBuffer(&gMA.recvBuf, gMA.recvPacket);
        MABIOS_Data(&gMA.recvBuf, gMA.strBuf, MAU_strlen(gMA.strBuf),
            gMA.sockets[0]);
        gMA.taskStep++;
        break;

    case 1:
        ConcatPrevBuf(gMA.recvBuf.data + 1, gMA.recvBuf.size - 1);
        if (!MAU_CheckCRLF(gMA.prevBuf, gMA.prevBufSize)) {
            MA_InitBuffer(&gMA.recvBuf, gMA.recvPacket);
            MABIOS_Data(&gMA.recvBuf, NULL, 0, gMA.sockets[0]);
            return;
        }

        pop3res = CheckPOP3Response(gMA.prevBuf);
        if (pop3res == 0) {
            cp = &gMA.prevBuf[4];
            *param.pNum = MAU_atoi(cp);
            cp = MAU_FindPostBlank((char *)cp);
            *param.pSize = MAU_atoi(cp);
            gMA.taskStep++;
        } else if (pop3res == 1) {
            gMA.taskError = MAAPIE_POP3;
            gMA.taskErrorDetail = 0;
            gMA.taskStep = 0xf0;
        } else {
            gMA.taskError = MAAPIE_POP3;
            gMA.taskErrorDetail = 0;
            gMA.taskStep = 0xf0;
        }
        break;

    case 2:
        MA_TaskSet(TASK_NONE, 0);
        break;

    case 0xf0:
        MA_InitBuffer(&gMA.recvBuf, gMA.recvPacket);
        MABIOS_TCPDisconnect(&gMA.recvBuf, gMA.sockets[0]);
        gMA.taskStep++;
        break;

    case 0xf1:
        gMA.connMode = CONN_PPP;
        MA_SetCondition(MA_CONDITION_PPP);
        MA_SetApiError(gMA.taskError, gMA.taskErrorDetail);
        MA_TaskSet(TASK_NONE, 0);
        gMA.usedSockets[0] = FALSE;
        break;
    }
#undef param
}

void MA_POP3_List(u16 mailNo, u32 *pSize)
{
#define param gMA.param.pop3_list
    SetApiCallFlag();
    if (!MA_ApiPreExe(TASK_POP3_LIST)) {
        ResetApiCallFlag();
        return;
    }

    if (mailNo == 0) {
        MA_SetApiError(MAAPIE_ILLEGAL_PARAMETER, 0);
        ResetApiCallFlag();
        return;
    }

    param.pSize = pSize;
    MAU_strcpy(gMA.strBuf, "LIST ");
    MAU_itoa(mailNo, &gMA.strBuf[MAU_strlen(gMA.strBuf)], 10);
    MAU_strcat(gMA.strBuf, "\r\n");
    MA_TaskSet(TASK_POP3_LIST, 0);

    ResetApiCallFlag();
#undef param
}

static void MATASK_POP3_List(void)
{
#define param gMA.param.pop3_list
    static const char *cp;
    static int pop3res;

    if (gMA.recvCmd == (MAPROT_REPLY | MACMD_ERROR)) {
        switch (gMA.negaResCmd) {
        case MACMD_DATA:
            gMA.taskError = MAAPIE_TCP_CONNECT;
            gMA.taskErrorDetail = 0;
            gMA.taskStep = 0xf0;
            break;

        case MACMD_TCP_DISCONNECT:
            break;

        default:
            MA_DefaultNegaResProc();
            gMA.taskStep = 0xf1;
            break;
        }
    }

    if (MA_GetCondition() & MA_CONDITION_TCPCLOSED) {
        gMA.connMode = CONN_PPP;
        MA_SetCondition(MA_CONDITION_PPP);
        gMA.sockets[0] = 0;
        gMA.usedSockets[0] = FALSE;
        MA_SetApiError(MAAPIE_POP3, 0);
        MA_TaskSet(TASK_NONE, 0);
        return;
    }

    switch (gMA.taskStep) {
    case 0:
        InitPrevBuf();
        MA_InitBuffer(&gMA.recvBuf, gMA.recvPacket);
        MABIOS_Data(&gMA.recvBuf, gMA.strBuf, MAU_strlen(gMA.strBuf),
            gMA.sockets[0]);
        gMA.taskStep++;
        break;

    case 1:
        ConcatPrevBuf(gMA.recvBuf.data + 1, gMA.recvBuf.size - 1);

        if (!MAU_CheckCRLF(gMA.prevBuf, gMA.prevBufSize)) {
            MA_InitBuffer(&gMA.recvBuf, gMA.recvPacket);
            MABIOS_Data(&gMA.recvBuf, NULL, 0, gMA.sockets[0]);
            return;
        }

        pop3res = CheckPOP3Response(gMA.prevBuf);
        if (pop3res == 0) {
            cp = MAU_FindPostBlank(&gMA.prevBuf[4]);
            *param.pSize = MAU_atoi(cp);
            gMA.taskStep = 100;
        } else if (pop3res == 1) {
            gMA.taskError = MAAPIE_POP3;
            gMA.taskErrorDetail = 4;
            gMA.taskStep = 0xf0;
        } else {
            gMA.taskError = MAAPIE_POP3;
            gMA.taskErrorDetail = 0;
            gMA.taskStep = 0xf0;
        }
        break;

    case 100:
        MA_TaskSet(TASK_NONE, 0);
        break;

    case 0xf0:
        MA_InitBuffer(&gMA.recvBuf, gMA.recvPacket);
        MABIOS_TCPDisconnect(&gMA.recvBuf, gMA.sockets[0]);
        gMA.taskStep++;
        break;

    case 0xf1:
        gMA.connMode = CONN_PPP;
        MA_SetCondition(MA_CONDITION_PPP);
        MA_SetApiError(gMA.taskError, gMA.taskErrorDetail);
        MA_TaskSet(TASK_NONE, 0);
        gMA.usedSockets[0] = FALSE;
        break;
    }
#undef param
}

void MA_POP3_Retr(u16 mailNo, u8 *pRecvData, u16 recvBufSize, u16 *pRecvSize)
{
#define param gMA.param.pop3_retr
    SetApiCallFlag();
    if (!MA_ApiPreExe(TASK_POP3_RETR)) {
        ResetApiCallFlag();
        return;
    }

    *pRecvSize = 0;
    param.pRecvData = pRecvData;
    param.recvBufSize = recvBufSize;
    param.pRecvSize = pRecvSize;

    if (gMA.condition & MA_CONDITION_BUFFER_FULL) {
        gMA.condition &= ~MA_CONDITION_BUFFER_FULL;
        gMA.status &= ~STATUS_BUFFER_EMPTY;

        if (recvBufSize == 0) {
            param.pRecvData = NULL;
            param.pRecvSize = NULL;
            MA_TaskSet(TASK_POP3_RETR, 1);
        } else if (gMA.prevBufSize != 0 && gMA.prevBufSize <= recvBufSize) {
            MAU_memcpy(pRecvData, gMA.prevBuf, gMA.prevBufSize);
            *pRecvSize = gMA.prevBufSize + *pRecvSize;
            gMA.prevBufSize = 0;

            gMA.status |= STATUS_BUFFER_EMPTY;
            MA_TaskSet(TASK_POP3_RETR, 1);
        } else {
            MAU_memcpy(pRecvData, gMA.prevBuf, recvBufSize);
            gMA.prevBufSize -= recvBufSize;
            MAU_memcpy(gMA.prevBuf, &gMA.prevBuf[recvBufSize], gMA.prevBufSize);
            *pRecvSize = recvBufSize;

            gMA.condition |= MA_CONDITION_BUFFER_FULL;
            ResetApiCallFlag();
            return;
        }
    } else {
        if (recvBufSize == 0) {
            MA_SetApiError(MAAPIE_ILLEGAL_PARAMETER, 0);
            ResetApiCallFlag();
            return;
        }

        param.respFound = FALSE;
        InitPrevBuf();
        gMA.condition &= ~MA_CONDITION_BUFFER_FULL;
        gMA.status &= ~STATUS_BUFFER_EMPTY;

        MAU_strcpy(gMA.strBuf, "RETR ");
        MAU_itoa(mailNo, &gMA.strBuf[MAU_strlen(gMA.strBuf)], 10);
        MAU_strcat(gMA.strBuf, "\r\n");

        MA_TaskSet(TASK_POP3_RETR, 0);
    }

    ResetApiCallFlag();
#undef param
}

static void MATASK_POP3_Retr(void)
{
#define param gMA.param.pop3_retr
    static const char *cp;
    static int dataLen;
    static int pop3res;

    if (gMA.recvCmd == (MAPROT_REPLY | MACMD_ERROR)) {
        switch (gMA.negaResCmd) {
        case MACMD_DATA:
            gMA.taskError = MAAPIE_TCP_CONNECT;
            gMA.taskErrorDetail = 0;
            gMA.taskStep = 0xf0;
            break;

        case MACMD_TCP_DISCONNECT:
            break;

        default:
            MA_DefaultNegaResProc();
            gMA.taskStep = 0xf1;
            break;
        }
    }

    if (MA_GetCondition() & MA_CONDITION_TCPCLOSED) {
        gMA.connMode = CONN_PPP;
        MA_SetCondition(MA_CONDITION_PPP);
        gMA.sockets[0] = 0;
        gMA.usedSockets[0] = FALSE;
        MA_SetApiError(MAAPIE_POP3, 0);
        MA_TaskSet(TASK_NONE, 0);
        return;
    }

    switch (gMA.taskStep) {
    case 0:
        InitPrevBuf();
        MA_InitBuffer(&gMA.recvBuf, gMA.recvPacket);
        MABIOS_Data(&gMA.recvBuf, gMA.strBuf, MAU_strlen(gMA.strBuf),
            gMA.sockets[0]);
        gMA.taskStep++;
        break;

    case 1:
        dataLen = gMA.recvBuf.size - 1;
        if (!(gMA.status & STATUS_BUFFER_EMPTY)) {
            if (param.pRecvData && dataLen > 1) {
                if (!param.respFound) {
                    cp = MAU_SearchCRLF(&gMA.recvBuf.data[1], dataLen);
                    if (cp == NULL) {
                        ConcatPrevBuf(&gMA.recvBuf.data[1], dataLen);
                        MA_InitBuffer(&gMA.recvBuf, gMA.recvPacket);
                        MABIOS_Data(&gMA.recvBuf, NULL, 0, gMA.sockets[0]);
                        break;
                    }
                    ConcatPrevBuf(&gMA.recvBuf.data[1],
                        cp - (char *)&gMA.recvBuf.data[1]);

                    pop3res = CheckPOP3Response(gMA.prevBuf);
                    if (pop3res == 0) {
                        dataLen -= (u8 *)cp - &gMA.recvBuf.data[1];
                        gMA.recvBuf.data = (u8 *)&cp[-1];
                        param.respFound = TRUE;
                    } else if (pop3res == 1) {
                        gMA.taskError = MAAPIE_POP3;
                        gMA.taskErrorDetail = 4;
                        gMA.taskStep = 0xf0;
                        break;
                    } else {
                        gMA.taskError = MAAPIE_POP3;
                        gMA.taskErrorDetail = 0;
                        gMA.taskStep = 0xf0;
                        break;
                    }
                }

                if (param.recvBufSize >= dataLen) {
                    MAU_memcpy(param.pRecvData, &gMA.recvBuf.data[1], dataLen);
                    param.pRecvData += dataLen;
                    param.recvBufSize -= dataLen;
                    gMA.condition &= ~MA_CONDITION_BUFFER_FULL;
                    *param.pRecvSize += dataLen;
                } else {
                    MAU_memcpy(param.pRecvData, &gMA.recvBuf.data[1],
                        param.recvBufSize);
                    gMA.prevBufSize = dataLen - param.recvBufSize;
                    MAU_memcpy(gMA.prevBuf,
                        &gMA.recvBuf.data[param.recvBufSize + 1],
                        gMA.prevBufSize);
                    *param.pRecvSize += param.recvBufSize;
                    param.recvBufSize = 0;
                    gMA.condition |= MA_CONDITION_BUFFER_FULL;
                    MA_TaskSet(TASK_NONE, 0);
                    gMA.condition |= MA_CONDITION_APIWAIT;
                    break;
                }
            }
        } else {
            gMA.status &= ~STATUS_BUFFER_EMPTY;
        }

        if (dataLen >= 5) {
            MakeEndLineBuffer(gMA.recvBuf.data + gMA.recvBuf.size - 5, 5);
            if (IsEndMultiLine() == TRUE) {
                gMA.taskStep = 100;
                break;
            }
        } else if (dataLen != 0) {
            MakeEndLineBuffer(&gMA.recvBuf.data[1], dataLen);
            if (IsEndMultiLine() == TRUE) {
                gMA.taskStep = 100;
                break;
            }
        }

        MA_InitBuffer(&gMA.recvBuf, gMA.recvPacket);
        MABIOS_Data(&gMA.recvBuf, NULL, 0, gMA.sockets[0]);
        break;

    case 100:
        MA_TaskSet(TASK_NONE, 0);
        break;

    case 0xf0:
        MA_InitBuffer(&gMA.recvBuf, gMA.recvPacket);
        MABIOS_TCPDisconnect(&gMA.recvBuf, gMA.sockets[0]);
        gMA.taskStep++;
        break;

    case 0xf1:
        gMA.connMode = CONN_PPP;
        MA_SetCondition(MA_CONDITION_PPP);
        MA_SetApiError(gMA.taskError, gMA.taskErrorDetail);
        MA_TaskSet(TASK_NONE, 0);
        gMA.usedSockets[0] = FALSE;
        break;
    }
#undef param
}

void MA_POP3_Dele(u16 mailNo)
{
    SetApiCallFlag();
    if (!MA_ApiPreExe(TASK_POP3_DELE)) {
        ResetApiCallFlag();
        return;
    }

    MAU_strcpy(gMA.strBuf, "DELE ");
    MAU_itoa(mailNo, &gMA.strBuf[MAU_strlen(gMA.strBuf)], 10);
    MAU_strcat(gMA.strBuf, "\r\n");

    MA_TaskSet(TASK_POP3_DELE, 0);
    ResetApiCallFlag();
}

static void MATASK_POP3_Dele(void)
{
    static int pop3res;

    if (gMA.recvCmd == (MAPROT_REPLY | MACMD_ERROR)) {
        switch (gMA.negaResCmd) {
        case MACMD_DATA:
            gMA.taskError = MAAPIE_TCP_CONNECT;
            gMA.taskErrorDetail = 0;
            gMA.taskStep = 0xf0;
            break;

        case MACMD_TCP_DISCONNECT:
            break;

        default:
            MA_DefaultNegaResProc();
            gMA.taskStep = 0xf1;
            break;
        }
    }

    if (MA_GetCondition() & MA_CONDITION_TCPCLOSED) {
        gMA.connMode = CONN_PPP;
        MA_SetCondition(MA_CONDITION_PPP);
        gMA.sockets[0] = 0;
        gMA.usedSockets[0] = FALSE;
        MA_SetApiError(MAAPIE_POP3, 0);
        MA_TaskSet(TASK_NONE, 0);
        return;
    }

    switch (gMA.taskStep) {
    case 0:
        InitPrevBuf();
        MA_InitBuffer(&gMA.recvBuf, gMA.recvPacket);
        MABIOS_Data(&gMA.recvBuf, gMA.strBuf, MAU_strlen(gMA.strBuf),
            gMA.sockets[0]);
        gMA.taskStep++;
        break;

    case 1:
        ConcatPrevBuf(gMA.recvBuf.data + 1, gMA.recvBuf.size - 1);
        if (!MAU_CheckCRLF(gMA.prevBuf, gMA.prevBufSize)) {
            MA_InitBuffer(&gMA.recvBuf, gMA.recvPacket);
            MABIOS_Data(&gMA.recvBuf, NULL, 0, gMA.sockets[0]);
            return;
        }

        pop3res = CheckPOP3Response(gMA.prevBuf);
        if (pop3res == 0) {
            gMA.taskStep++;
        } else if (pop3res == 1) {
            gMA.taskError = MAAPIE_POP3;
            gMA.taskErrorDetail = 4;
            gMA.taskStep = 0xf0;
        } else {
            gMA.taskError = MAAPIE_POP3;
            gMA.taskErrorDetail = 0;
            gMA.taskStep = 0xf0;
        }
        break;

    case 2:
        MA_TaskSet(TASK_NONE, 0);
        break;

    case 0xf0:
        MA_InitBuffer(&gMA.recvBuf, gMA.recvPacket);
        MABIOS_TCPDisconnect(&gMA.recvBuf, gMA.sockets[0]);
        gMA.taskStep++;
        break;

    case 0xf1:
        gMA.connMode = CONN_PPP;
        MA_SetCondition(MA_CONDITION_PPP);
        MA_SetApiError(gMA.taskError, gMA.taskErrorDetail);
        MA_TaskSet(TASK_NONE, 0);
        gMA.usedSockets[0] = FALSE;
        break;
    }
}

void MA_POP3_Head(u16 mailNo, u8 *pRecvData, u16 recvBufSize, u16 *pRecvSize)
{
#define param gMA.param.pop3_retr
    SetApiCallFlag();
    if (!MA_ApiPreExe(TASK_POP3_RETR)) {
        ResetApiCallFlag();
        return;
    }

    *pRecvSize = 0;
    param.pRecvData = pRecvData;
    param.recvBufSize = recvBufSize;
    param.pRecvSize = pRecvSize;

    if (gMA.condition & MA_CONDITION_BUFFER_FULL) {
        gMA.condition &= ~MA_CONDITION_BUFFER_FULL;
        gMA.status &= ~STATUS_BUFFER_EMPTY;

        if (recvBufSize == 0) {
            param.pRecvData = NULL;
            param.pRecvSize = NULL;
            MA_TaskSet(TASK_POP3_RETR, 1);
        } else if (gMA.prevBufSize != 0 && gMA.prevBufSize <= recvBufSize) {
            MAU_memcpy(pRecvData, gMA.prevBuf, gMA.prevBufSize);
            *pRecvSize = gMA.prevBufSize + *pRecvSize;
            gMA.prevBufSize = 0;

            gMA.status |= STATUS_BUFFER_EMPTY;
            MA_TaskSet(TASK_POP3_RETR, 1);
        } else {
            MAU_memcpy(pRecvData, gMA.prevBuf, recvBufSize);
            gMA.prevBufSize -= recvBufSize;
            MAU_memcpy(gMA.prevBuf, &gMA.prevBuf[recvBufSize], gMA.prevBufSize);
            *pRecvSize = recvBufSize;

            gMA.condition |= MA_CONDITION_BUFFER_FULL;
            ResetApiCallFlag();
            return;
        }
    } else {
        if (recvBufSize == 0) {
            MA_SetApiError(MAAPIE_ILLEGAL_PARAMETER, 0);
            ResetApiCallFlag();
            return;
        }

        param.respFound = FALSE;
        InitPrevBuf();
        gMA.condition &= ~MA_CONDITION_BUFFER_FULL;

        MAU_strcpy(gMA.strBuf, "TOP ");
        MAU_itoa(mailNo, &gMA.strBuf[MAU_strlen(gMA.strBuf)], 10);
        MAU_strcat(gMA.strBuf, " 0\r\n");

        MA_TaskSet(TASK_POP3_RETR, 0);
    }

    ResetApiCallFlag();
#undef param
}

static void MATASK_POP3_Head(void)
{
#define param gMA.param.pop3_retr
    static const char *cp;
    static int dataLen;
    static int pop3res;

    if (gMA.recvCmd == (MAPROT_REPLY | MACMD_ERROR)) {
        switch (gMA.negaResCmd) {
        case MACMD_DATA:
            gMA.taskError = MAAPIE_TCP_CONNECT;
            gMA.taskErrorDetail = 0;
            gMA.taskStep = 0xf0;
            break;

        case MACMD_TCP_DISCONNECT:
            break;

        default:
            MA_DefaultNegaResProc();
            gMA.taskStep = 0xf1;
            break;
        }
    }

    if (MA_GetCondition() & MA_CONDITION_TCPCLOSED) {
        gMA.connMode = CONN_PPP;
        MA_SetCondition(MA_CONDITION_PPP);
        gMA.sockets[0] = 0;
        gMA.usedSockets[0] = FALSE;
        MA_SetApiError(MAAPIE_POP3, 0);
        MA_TaskSet(TASK_NONE, 0);
        return;
    }

    switch (gMA.taskStep) {
    case 0:
        InitPrevBuf();
        MA_InitBuffer(&gMA.recvBuf, gMA.recvPacket);
        MABIOS_Data(&gMA.recvBuf, gMA.strBuf, MAU_strlen(gMA.strBuf),
            gMA.sockets[0]);
        gMA.taskStep++;
        break;

    case 1:
        dataLen = gMA.recvBuf.size - 1;
        if (!(gMA.status & STATUS_BUFFER_EMPTY)) {
            if (param.pRecvData && dataLen > 1) {
                if (!param.respFound) {
                    cp = MAU_SearchCRLF(&gMA.recvBuf.data[1], dataLen);
                    if (cp == NULL) {
                        ConcatPrevBuf(&gMA.recvBuf.data[1], dataLen);
                        (&gMA.recvBuf)->size = 0;
                        (&gMA.recvBuf)->data = gMA.recvPacket;
                        MABIOS_Data(&gMA.recvBuf, NULL, 0, gMA.sockets[0]);
                        break;
                    }
                    ConcatPrevBuf(&gMA.recvBuf.data[1],
                        cp - (char *)&gMA.recvBuf.data[1]);

                    pop3res = CheckPOP3Response(gMA.prevBuf);
                    if (pop3res == 0) {
                        dataLen -= (u8 *)cp - &gMA.recvBuf.data[1];
                        gMA.recvBuf.data = (u8 *)&cp[-1];
                        param.respFound = TRUE;
                    } else if (pop3res == 1) {
                        gMA.taskError = MAAPIE_POP3;
                        gMA.taskErrorDetail = 4;
                        gMA.taskStep = 0xf0;
                        break;
                    } else {
                        gMA.taskError = MAAPIE_POP3;
                        gMA.taskErrorDetail = 0;
                        gMA.taskStep = 0xf0;
                        break;
                    }
                }

                if (param.recvBufSize >= dataLen) {
                    MAU_memcpy(param.pRecvData, &gMA.recvBuf.data[1], dataLen);
                    param.pRecvData += dataLen;
                    param.recvBufSize -= dataLen;
                    gMA.condition &= ~MA_CONDITION_BUFFER_FULL;
                    *param.pRecvSize += dataLen;
                } else {
                    MAU_memcpy(param.pRecvData, &gMA.recvBuf.data[1],
                        param.recvBufSize);
                    gMA.prevBufSize = dataLen - param.recvBufSize;
                    MAU_memcpy(gMA.prevBuf,
                        &gMA.recvBuf.data[param.recvBufSize + 1],
                        gMA.prevBufSize);
                    *param.pRecvSize += param.recvBufSize;
                    param.recvBufSize = 0;
                    gMA.condition |= MA_CONDITION_BUFFER_FULL;
                    MA_TaskSet(TASK_NONE, 0);
                    gMA.condition |= MA_CONDITION_APIWAIT;
                    break;
                }
            }
        } else {
            gMA.status &= ~STATUS_BUFFER_EMPTY;
        }

        if (dataLen >= 5) {
            MakeEndLineBuffer(gMA.recvBuf.data + gMA.recvBuf.size - 5, 5);
            if (IsEndMultiLine() == TRUE) {
                gMA.taskStep = 100;
                break;
            }
        } else {
            if (dataLen != 0) {
                MakeEndLineBuffer(&gMA.recvBuf.data[1], dataLen);
                if (IsEndMultiLine() == TRUE) {
                    gMA.taskStep = 100;
                    break;
                }
            }
        }
        MA_InitBuffer(&gMA.recvBuf, gMA.recvPacket);
        MABIOS_Data(&gMA.recvBuf, NULL, 0, gMA.sockets[0]);
        break;

    case 100:
        MA_TaskSet(TASK_NONE, 0);
        break;

    case 0xf0:
        MA_InitBuffer(&gMA.recvBuf, gMA.recvPacket);
        MABIOS_TCPDisconnect(&gMA.recvBuf, gMA.sockets[0]);
        gMA.taskStep++;
        break;

    case 0xf1:
        gMA.connMode = CONN_PPP;
        MA_SetCondition(MA_CONDITION_PPP);
        MA_SetApiError(gMA.taskError, gMA.taskErrorDetail);
        MA_TaskSet(TASK_NONE, 0);
        gMA.usedSockets[0] = FALSE;
        break;
    }
#undef param
}

static const char *ExtractServerName(char *pServerName, const char *pURL,
    u8 *pServerAuth, u8 *pServerType)
{
    static const char strDownload[] = "gameboy.datacenter.ne.jp/cgb/download";
    static const char strUpload[] = "gameboy.datacenter.ne.jp/cgb/upload";
    static const char strUtility[] = "gameboy.datacenter.ne.jp/cgb/utility";
    static const char strRanking[] = "gameboy.datacenter.ne.jp/cgb/ranking";
    static const char strHttp[] = "http://";

    static const char *cp;
    static const char *tmpp;
    static int len;

    if (MAU_strnicmp(pURL, strHttp, sizeof(strHttp) - 1) == 0) {
        pURL += sizeof(strHttp) - 1;
    }

    cp = MAU_strchr(pURL, '/');
    if (!cp) {
        if (MAU_strchr(pURL, '\0') - pURL >= MAPROT_BODY_SIZE) {
            *pServerName = '\0';
            return NULL;
        }
        MAU_strcpy(pServerName, pURL);
        *pServerAuth = SERVER_UNKNOWN;
    } else {
        len = cp - pURL;
        if (len >= MAPROT_BODY_SIZE) {
            *pServerName = '\0';
            return NULL;
        }

        MAU_memcpy(pServerName, pURL, len);
        pServerName[len] = '\0';

        if (MAU_strnicmp(pURL, strDownload, sizeof(strDownload) - 1) == 0) {
            *pServerType = SERVER_DOWNLOAD;
            tmpp = MAU_strrchr(pURL, '/');
            tmpp++;
            if (MAU_isdigit(tmpp[0])) {
                *pServerAuth = SERVER_DOWNLOAD;
            } else {
                *pServerAuth = SERVER_UNKNOWN;
            }
        } else if (MAU_strnicmp(pURL, strUpload, sizeof(strUpload) - 1) == 0) {
            *pServerType = SERVER_UPLOAD;
            tmpp = MAU_strrchr(pURL, '/');
            tmpp++;
            if (MAU_isdigit(tmpp[0])) {
                *pServerAuth = SERVER_UPLOAD;
            } else {
                *pServerAuth = SERVER_UNKNOWN;
            }
        } else if (MAU_strnicmp(pURL, strUtility, sizeof(strUtility) - 1) == 0) {
            *pServerType = SERVER_UTILITY;
            *pServerAuth = SERVER_UTILITY;
        } else if (MAU_strnicmp(pURL, strRanking, sizeof(strRanking) - 1) == 0) {
            *pServerType = SERVER_RANKING;
            tmpp = MAU_strrchr(pURL, '/');
            tmpp++;
            if (MAU_isdigit(tmpp[0])) {
                *pServerAuth = SERVER_RANKING;
            } else {
                *pServerAuth = SERVER_UNKNOWN;
            }
        } else {
            *pServerAuth = SERVER_UNKNOWN;
        }
    }

    return cp;
}

static const char strHttpGet[] = "GET ";
static const char strHttpPost[] = "POST ";
static const char strHttpReqSuffix[] = " HTTP/1.0\r\n";
static const char strHttpContentLength[] = "Content-Length: 0\r\n";
static const char strHttpAuthenticate[] = "WWW-Authenticate: GB00 name=\"";
static const char strHttpAuthorization[] = "Authorization: GB00 name=\"";
static const char strHttpContentType[] = "Content-Type: application/x-cgb\r\n";
static const char strHttpGbStatus[] = "Gb-Status: ";
static const char strHttpGbAuthID[] = "Gb-Auth-ID: ";
static const char strHttpDate[] = "Date: ";
static const char strHttpLocation[] = "Location: ";
static const char strHttpUserAgent[] = "User-Agent: AGB-";
static const char strServerRoot[] = "/";

void MA_HTTP_Get(const char *pURL, char *pHeadBuf, u16 headBufSize, u8 *pRecvData,
    u16 recvBufSize, u16 *pRecvSize, const char *pUserID, const char *pPassword)
{
    MA_HTTP_GetPost(pURL, pHeadBuf, headBufSize, NULL, 0, pRecvData,
        recvBufSize, pRecvSize, pUserID, pPassword, TASK_HTTP_GET);
}

void MA_HTTP_Post(const char *pURL, char *pHeadBuf, u16 headBufSize,
    const u8 *pSendData, u16 sendSize, u8 *pRecvData, u16 recvBufSize,
    u16 *pRecvSize, const char *pUserID, const char *pPassword)
{
    MA_HTTP_GetPost(pURL, pHeadBuf, headBufSize, pSendData, sendSize, pRecvData,
        recvBufSize, pRecvSize, pUserID, pPassword, TASK_HTTP_POST);
}

void MA_HTTP_GetPost(const char *pURL, char *pHeadBuf, u16 headBufSize,
    const u8 *pSendData, u16 sendSize, u8 *pRecvData, u16 recvBufSize,
    u16 *pRecvSize, const char *pUserID, const char *pPassword, u8 task)
{
#define param gMA.param.http_getpost
    int len;
    const char *pServerPath;
    u8 serverAuth;
    u8 serverType;
    int useDNS;

    SetApiCallFlag();
    if (!MA_ApiPreExe(task)) {
        ResetApiCallFlag();
        return;
    }

    param.task = task;
    param.pRecvData = pRecvData;
    param.recvBufSize = recvBufSize;
    param.pRecvSize = pRecvSize;
    param.pSendData = pSendData;
    param.sendSize = sendSize;
    param.pUserID = pUserID;
    param.pPassword = pPassword;
    param.headBufSize = headBufSize;
    param.pHeadBuf = pHeadBuf;

    if (!(gMA.condition & MA_CONDITION_BUFFER_FULL)) {
        if (recvBufSize == 0) {
            param.pRecvData = NULL;
            if (pRecvSize != NULL) *pRecvSize = 0;
        } else if (pRecvData == NULL) {
            MA_SetApiError(MAAPIE_ILLEGAL_PARAMETER, 0);
            ResetApiCallFlag();
            return;
        } else {
            *pRecvSize = 0;
        }

        if (param.headBufSize != 0 && param.pHeadBuf != NULL) {
            param.pHeadBuf[0] = '\0';
        }

        len = MAU_strlen(pURL);
        if (len > MAX_URL_LEN || len == 0) {
            MA_SetApiError(MAAPIE_ILLEGAL_PARAMETER, 0);
            ResetApiCallFlag();
            return;
        }

        serverType = SERVER_UNKNOWN;
        pServerPath = ExtractServerName(gMA.strBuf, pURL, &serverAuth,
            &serverType);
        if (gMA.strBuf[0] == '\0') {
            MA_SetApiError(MAAPIE_ILLEGAL_PARAMETER, 0);
            ResetApiCallFlag();
            return;
        }

        param.serverAuth = serverAuth;
        if (serverType == SERVER_UNKNOWN) {
            param.pUserID = "";
            param.pPassword = "";
        } else {
            len = MAU_strlen(pUserID);
            if (len > MAX_PASSWORD_LEN) {
                MA_SetApiError(MAAPIE_ILLEGAL_PARAMETER, 0);
                ResetApiCallFlag();
                return;
            }
            len = MAU_strlen(pPassword);
            if (len > MAX_PASSWORD_LEN) {
                MA_SetApiError(MAAPIE_ILLEGAL_PARAMETER, 0);
                ResetApiCallFlag();
                return;
            }
        }

        if ((serverType == SERVER_DOWNLOAD && param.task == TASK_HTTP_POST)
            || (serverType == SERVER_UPLOAD && param.task == TASK_HTTP_GET)
            || (serverType == SERVER_UTILITY && param.task == TASK_HTTP_POST)
            || (serverType == SERVER_RANKING && param.task == TASK_HTTP_GET)) {
            MA_SetApiError(MAAPIE_ILLEGAL_PARAMETER, 0);
            ResetApiCallFlag();
            return;
        }

        if (gMA.serverConf.http == 0) {
            useDNS = TRUE;
        } else {
            useDNS = FALSE;
        }

        if (useDNS == TRUE) {
            if (pServerPath) {
                param.pServerPath = pServerPath;
            } else {
                param.pServerPath = strServerRoot;
            }
        } else {
            param.pServerPath = pURL;
        }

        param.serverPathLen = MAU_strlen(param.pServerPath);
        param.pServerPathBkp = param.pServerPath;
        param.serverPathLenBkp = param.serverPathLen;
        param.unused4 = 7;

        InitPrevBuf();
        gMA.condition &= ~MA_CONDITION_BUFFER_FULL;
        gMA.status &= ~STATUS_BUFFER_EMPTY;
        gMA.gbCenterRes = 0;
        gMA.httpRes = 0;
        gMA.status &= ~STATUS_GBCENTER_ERR_101;
        gMA.authCode[0] = '\0';
        param.authStep = 0;
        param.headFlags = 0;
        gMA.connMode = CONN_HTTP;

        if (!useDNS) {
            MAU_memcpy(gMA.socketAddr, &gMA.serverConf.http,
                sizeof(gMA.socketAddr));
            MA_TaskSet(task, 2);
        } else {
            MA_TaskSet(task, 0);
        }
    } else {
        gMA.condition &= ~MA_CONDITION_BUFFER_FULL;
        gMA.status &= ~STATUS_BUFFER_EMPTY;

        if (recvBufSize == 0) {
            param.pRecvData = 0;
            if (pRecvSize != NULL) *pRecvSize = 0;
            gMA.status |= STATUS_BUFFER_EMPTY;
            MA_TaskSet(task, 110);
        } else {
            if (pRecvData == NULL) {
                MA_SetApiError(MAAPIE_ILLEGAL_PARAMETER, 0);
                ResetApiCallFlag();
                return;
            }

            *pRecvSize = 0;
            if (gMA.prevBufSize != 0 && gMA.prevBufSize <= recvBufSize) {
                MAU_memcpy(pRecvData, gMA.prevBuf, gMA.prevBufSize);
                pRecvData += gMA.prevBufSize;
                recvBufSize -= gMA.prevBufSize;
                *pRecvSize += gMA.prevBufSize;
                gMA.prevBufSize = 0;

                param.pRecvData = pRecvData;
                param.recvBufSize = recvBufSize;
                param.pRecvSize = pRecvSize;

                gMA.status |= STATUS_BUFFER_EMPTY;
                MA_TaskSet(task, 110);
            } else {
                MAU_memcpy(pRecvData, gMA.prevBuf, recvBufSize);
                gMA.prevBufSize -= recvBufSize;
                MAU_memcpy(gMA.prevBuf, &gMA.prevBuf[recvBufSize],
                    gMA.prevBufSize);
                *pRecvSize = recvBufSize;
                gMA.condition |= MA_CONDITION_BUFFER_FULL;
                ResetApiCallFlag();
                return;
            }
        }
    }

    ResetApiCallFlag();
#undef param
}

#define ConcatUserAgent_WriteAscii(dest, code) \
{ \
    if ((code) < 0x20 || (code) >= 0x7f) { \
        *(dest) = '0'; \
    } else { \
        *(dest) = (code); \
    } \
}

static void ConcatUserAgent(char *pUserAgent)
{
    static int tmpLen;
    static u8 *tmpp;
    static u8 tmpNum;
    static const char hexChar[] = "0123456789ABCDEF";

    tmpLen = MAU_strlen(pUserAgent);
    tmpp = &pUserAgent[tmpLen];

    ConcatUserAgent_WriteAscii(tmpp++, *(char *)(CASSETTE_INITIAL_CODE + 0));
    ConcatUserAgent_WriteAscii(tmpp++, *(char *)(CASSETTE_INITIAL_CODE + 1));
    ConcatUserAgent_WriteAscii(tmpp++, *(char *)(CASSETTE_INITIAL_CODE + 2));
    ConcatUserAgent_WriteAscii(tmpp++, *(char *)(CASSETTE_INITIAL_CODE + 3));

    *tmpp++ = '-';
    tmpNum = *(u8 *)CASSETTE_VERSION_NO;
    *tmpp++ = hexChar[(tmpNum >> 4) & 0xf];
    *tmpp++ = hexChar[(tmpNum >> 0) & 0xf];

    *tmpp++ = '\r';
    *tmpp++ = '\n';
    *tmpp++ = '\r';
    *tmpp++ = '\n';
    *tmpp = '\0';
}

static int GetRequestType(void)
{
#define param gMA.param.http_getpost
    static int ret;

    ret = 0;
    switch (param.task) {
    case TASK_HTTP_GET:
        switch (param.serverAuth) {
        case SERVER_DOWNLOAD:
            switch (param.authStep) {
            case 0:
            case 1:
                break;

            case 2:
                ret = 1;
                break;
            }
            break;
        }
        break;

    case TASK_HTTP_POST:
        switch (param.serverAuth) {
        case SERVER_UNKNOWN:
            ret = 1;
            break;

        case SERVER_UPLOAD:
            switch (param.authStep) {
            case 0:
            case 1:
                break;

            case 2:
                ret = 1;
                break;
            }
            break;

        case SERVER_RANKING:
            switch (param.authStep) {
            case 0:
                break;

            case 1:
            case 2:
                ret = 1;
                break;
            }
            break;
        }
    }
    return ret;
#undef param
}

static void CreateHttpRequestHeader(void)
{
#define param gMA.param.http_getpost
    static int tmpLen;
    static char bAddContentLength;
    static char bAddContentLengthZero;
    static char bAddAuthorization;
    static char bAddAuthID;

    bAddContentLength = FALSE;
    bAddContentLengthZero = FALSE;
    bAddAuthorization = FALSE;
    bAddAuthID = FALSE;

    switch (param.task) {
    case TASK_HTTP_GET:
        switch (param.serverAuth) {
        case SERVER_UNKNOWN:
            break;

        case SERVER_UTILITY:
            switch (param.authStep) {
            case 0:
                break;

            case 1:
                bAddAuthorization = TRUE;
                break;

            case 2:
                break;
            }
            break;

        case SERVER_DOWNLOAD:
            switch (param.authStep) {
            case 0:
                break;

            case 1:
                bAddAuthorization = TRUE;
                break;

            case 2:
                bAddAuthID = TRUE;
                break;
            }
            break;
        }
        break;

    case TASK_HTTP_POST:
        switch (param.serverAuth) {
        case SERVER_UNKNOWN:
            bAddContentLength = TRUE;
            break;

        case SERVER_UPLOAD:
            switch (param.authStep) {
            case 0:
                break;

            case 1:
                bAddAuthorization = TRUE;
                break;

            case 2:
                bAddAuthID = TRUE;
                bAddContentLength = TRUE;
                break;
            }
            break;

        case SERVER_RANKING:
            switch (param.authStep) {
            case 0:
                break;

            case 1:
                bAddContentLength = TRUE;
                bAddAuthorization = TRUE;
                break;

            case 2:
                bAddAuthID = TRUE;
                break;
            }
            break;
        }
        break;
    }

    if (bAddContentLength == TRUE) {
        MAU_strcat(gMA.strBuf, strHttpContentType);
        MAU_strcat(gMA.strBuf, strHttpContentLength);
        tmpLen = MAU_strlen(gMA.strBuf);
        MAU_itoa(param.sendSize, &gMA.strBuf[tmpLen - 3], 10);
        MAU_strcat(gMA.strBuf, "\r\n");
    }

    if (bAddContentLengthZero == TRUE) {
        MAU_strcat(gMA.strBuf, strHttpContentType);
        MAU_strcat(gMA.strBuf, strHttpContentLength);
    }

    if (bAddAuthorization == TRUE) {
        MAU_strcat(gMA.strBuf, strHttpAuthorization);
        MAU_strcat(gMA.strBuf, gMA.authCode);
        MAU_strcat(gMA.strBuf, "\"\r\n");
    }

    if (bAddAuthID == TRUE) {
        MAU_strcat(gMA.strBuf, strHttpGbAuthID);
        MAU_strcat(gMA.strBuf, gMA.authCode);
        MAU_strcat(gMA.strBuf, "\r\n");
    }

    MAU_strcat(gMA.strBuf, strHttpUserAgent);
    ConcatUserAgent(gMA.strBuf);
#undef param
}

static int HttpGetNextStep(int index)
{
#define param gMA.param.http_getpost
    static int step;

    switch (index) {
    case 0:
        switch (param.task) {
        case TASK_HTTP_GET:
            switch (param.serverAuth) {
            case SERVER_UNKNOWN:
                step = 8;
                break;

            case SERVER_DOWNLOAD:
                step = 8;
                break;

            case SERVER_UTILITY:
                step = 8;
                break;
            }
            break;

        case TASK_HTTP_POST:
            switch (param.serverAuth) {
            case SERVER_UNKNOWN:
                step = 100;
                break;

            case SERVER_UPLOAD:
                switch (param.authStep) {
                case 0:
                    step = 8;
                    break;

                case 1:
                    step = 8;
                    break;

                case 2:
                    step = 100;
                    break;
                }
                break;

            case SERVER_RANKING:
                switch (param.authStep) {
                case 0:
                    step = 8;
                    break;

                case 1:
                    step = 100;
                    break;

                case 2:
                    step = 8;
                    break;
                }
                break;
            }
            break;
        }
        break;

    case 1:
        switch (param.task) {
        case TASK_HTTP_GET:
            switch (param.serverAuth) {
            case SERVER_UNKNOWN:
                step = 110;
                break;

            case SERVER_UTILITY:
                switch (param.authStep) {
                case 0:
                case 1:
                    step = 110;
                    break;
                }
                break;

            case SERVER_DOWNLOAD:
                switch (param.authStep) {
                case 0:
                case 1:
                    step = 110;
                    break;

                case 2:
                    step = 0xff;
                    break;
                }
                break;
            }
            break;

        case TASK_HTTP_POST:
            switch (param.serverAuth) {
            case SERVER_UNKNOWN:
                step = 110;
                break;

            case SERVER_UPLOAD:
                switch (param.authStep) {
                case 0:
                case 1:
                    step = 110;
                    break;

                case 2:
                    step = 0xff;
                    break;
                }
                break;

            case SERVER_RANKING:
                switch (param.authStep) {
                case 0:
                case 1:
                    step = 110;
                    break;

                case 2:
                    step = 0xff;
                    break;
                }
                break;
            }
            break;
        }
        break;

    case 2:
        switch (param.task) {
        case TASK_HTTP_GET:
            switch (param.serverAuth) {
            case SERVER_UNKNOWN:
                step = 0xff;
                break;

            case SERVER_DOWNLOAD:
                switch (param.authStep) {
                case 1:
                case 2:
                    step = 2;
                    break;
                }
                break;

            case SERVER_UTILITY:
                switch (param.authStep) {
                case 1:
                    step = 2;
                    break;

                case 2:
                    step = 0xff;
                    break;
                }
                break;
            }
            break;

        case TASK_HTTP_POST:
            switch (param.serverAuth) {
            case SERVER_UNKNOWN:
                step = 0xff;
                break;

            case SERVER_UPLOAD:
                switch (param.authStep) {
                case 1:
                case 2:
                    step = 2;
                    break;
                }
                break;

            case SERVER_RANKING:
                switch (param.authStep) {
                case 1:
                case 2:
                    step = 2;
                    break;
                }
                break;
            }
            break;
        }
        break;
    }
    return step;
#undef param
}

static void MATASK_HTTP_GetPost(void)
{
#define param gMA.param.http_getpost
    static const char *curCp;
    static const char *nextCp;
    static char *lineCp;
    static int tmpLen;
    static int dataLen;
    static int headerLineLen;

    curCp = nextCp = lineCp = NULL;
    tmpLen = dataLen = headerLineLen = 0;

    if (gMA.recvCmd == (MAPROT_REPLY | MACMD_ERROR)) {
        switch (gMA.negaResCmd) {
        case MACMD_DATA:
            gMA.taskError = MAAPIE_TCP_CONNECT;
            gMA.taskErrorDetail = 0;
            gMA.taskStep = 0xf0;
            break;

        case MACMD_TCP_CONNECT:
        case MACMD_DNS_REQUEST:
            gMA.taskError = MAAPIE_TCP_CONNECT;
            gMA.taskErrorDetail = 0;
            gMA.taskStep = 0xf1;
            break;

        case MACMD_TCP_DISCONNECT:
            break;

        default:
            MA_DefaultNegaResProc();
            gMA.taskStep = 0xf1;
            break;
        }
    }

    switch (gMA.taskStep) {
    case 0:
        MA_InitBuffer(&gMA.recvBuf, gMA.recvPacket);
        MABIOS_DNSRequest(&gMA.recvBuf, gMA.strBuf);
        gMA.taskStep++;
        break;

    case 1:
        MAU_memcpy(gMA.socketAddr, gMA.recvBuf.data, sizeof(gMA.socketAddr));
        gMA.taskStep++;

    case 2:
        MA_InitBuffer(&gMA.recvBuf, gMA.recvPacket);
        if (gMA.serverConf.http == 0) {
            MABIOS_TCPConnect(&gMA.recvBuf, gMA.socketAddr, PORT_HTTP);
        } else {
            MABIOS_TCPConnect(&gMA.recvBuf, gMA.socketAddr, PORT_HTTP);
        }
        gMA.taskStep++;
        break;

    case 3:
        gMA.sockets[0] = gMA.recvBuf.data[0];
        gMA.usedSockets[0] = TRUE;
        gMA.taskStep++;

    case 4:
        MA_InitBuffer(&gMA.recvBuf, gMA.recvPacket);
        if (GetRequestType() == 0) {
            MABIOS_Data(&gMA.recvBuf, strHttpGet, sizeof(strHttpGet) - 1,
                gMA.sockets[0]);
        } else {
            MABIOS_Data(&gMA.recvBuf, strHttpPost, sizeof(strHttpPost) - 1,
                gMA.sockets[0]);
        }
        param.nextStep = gMA.taskStep + 1;
        gMA.taskStep = 50;
        break;

    case 5:
        param.pServerPath = param.pServerPathBkp;
        param.serverPathLen = param.serverPathLenBkp;
        gMA.taskStep++;

    case 6:
        MA_InitBuffer(&gMA.recvBuf, gMA.recvPacket);
        if (param.serverPathLen > MAX_TCP_DATA_SIZE) {
            MABIOS_Data(&gMA.recvBuf, param.pServerPath, MAX_TCP_DATA_SIZE,
                gMA.sockets[0]);
            param.pServerPath += MAX_TCP_DATA_SIZE;
            param.serverPathLen -= MAX_TCP_DATA_SIZE;
            param.nextStep = gMA.taskStep;
            gMA.taskStep = 50;
            break;
        }
        MABIOS_Data(&gMA.recvBuf, param.pServerPath, param.serverPathLen,
            gMA.sockets[0]);
        param.nextStep = gMA.taskStep + 1;
        gMA.taskStep = 50;
        break;

    case 7:
        MAU_strcpy(gMA.strBuf, strHttpReqSuffix);
        CreateHttpRequestHeader();
        InitPrevBuf();
        param.headEnd = FALSE;
        param.headError = FALSE;
        param.headFound = FALSE;
        curCp = nextCp = NULL;
        gMA.taskStep = HttpGetNextStep(0);
        MA_InitBuffer(&gMA.recvBuf, gMA.recvPacket);
        MABIOS_Data(&gMA.recvBuf, gMA.strBuf, MAU_strlen(gMA.strBuf),
            gMA.sockets[0]);
        break;

    case 8:
        if (param.headEnd == TRUE) {
            if (param.headFound == FALSE) {
                gMA.taskError = MAAPIE_HTTP;
                gMA.taskErrorDetail = 0;
                gMA.taskStep = 0xf0;
                break;
            }
            if (param.headError == TRUE) {
                gMA.taskStep = 0xf0;
                break;
            }
            gMA.taskStep = HttpGetNextStep(1);
            param.authStep++;
            break;
        }

        if (MA_GetCondition() & MA_CONDITION_TCPCLOSED) {
            if (param.headFound == FALSE) {
                gMA.taskError = MAAPIE_HTTP;
                gMA.taskErrorDetail = 0;
                gMA.taskStep = 0xf0;
                break;
            }
            if (param.headError == TRUE) {
                gMA.taskStep = 0xf0;
                break;
            }
            gMA.taskError = MAAPIE_HTTP;
            gMA.taskErrorDetail = 0;
            gMA.taskStep = 0xf0;
            break;
        }

        if (gMA.recvBuf.size != 1) {
            tmpLen = gMA.recvBuf.size - 1;
            curCp = &gMA.recvBuf.data[1];
            lineCp = gMA.prevBuf;
            if (gMA.prevBufSize != 0) lineCp += gMA.prevBufSize;

            for (;;) {
                nextCp = MAU_strncpy_CRLF_LF(lineCp, curCp, tmpLen);
                if (nextCp == NULL) break;

                tmpLen -= nextCp - curCp;
                if (gMA.prevBufSize != 0) {
                    lineCp = gMA.prevBuf;
                    gMA.prevBufSize = 0;
                }

                if (MAU_strncmp(lineCp, "HTTP", 4) == 0) {
                    if (CheckResponse(&lineCp[9])) {
                        gMA.httpRes = GetResponse(&lineCp[9]);
                        param.headFound = TRUE;
                        if ((param.serverAuth == SERVER_DOWNLOAD
                                || param.serverAuth == SERVER_UPLOAD
                                || param.serverAuth == SERVER_UTILITY
                                || param.serverAuth == SERVER_RANKING)
                            && param.authStep == 0) {
                            if (gMA.httpRes != 401) {
                                gMA.taskError = MAAPIE_HTTP;
                                if (gMA.httpRes == 200) {
                                    gMA.taskErrorDetail = 0;
                                } else {
                                    gMA.taskErrorDetail = gMA.httpRes;
                                }
                                param.headError = TRUE;
                            }
                        } else if (gMA.httpRes != 200) {
                            gMA.taskError = MAAPIE_HTTP;
                            gMA.taskErrorDetail = gMA.httpRes;
                            param.headError = TRUE;
                        }
                    } else {
                        gMA.taskError = MAAPIE_HTTP;
                        gMA.taskErrorDetail = 0;
                        param.headError = TRUE;
                    }
                } else if (param.authStep == 0 && (
                            (MAU_strncmp(lineCp, strHttpDate, sizeof(strHttpDate) - 1) == 0 &&
                             !(param.headFlags & 1)) ||
                            (MAU_strncmp(lineCp, strHttpLocation, sizeof(strHttpLocation) - 1) == 0 &&
                             !(param.headFlags & 2))
                            )) {
                    if (MAU_strncmp(lineCp, strHttpDate, sizeof(strHttpDate) - 1) == 0) {
                        param.headFlags |= 1;
                    } else if (MAU_strncmp(lineCp, strHttpLocation, sizeof(strHttpLocation) - 1) == 0) {
                        param.headFlags |= 2;
                    }

                    if (param.headBufSize != 0 && param.pHeadBuf) {
                        headerLineLen = MAU_strcpy_size(param.pHeadBuf, lineCp,
                            param.headBufSize);
                        param.headBufSize -= headerLineLen;
                        param.pHeadBuf += headerLineLen;
                        if (param.pHeadBuf[-1] == '\0' && param.headBufSize != 0) {
                            param.pHeadBuf[0] = '\0';
                        }
                    }
                } else if (MAU_strncmp(lineCp, strHttpAuthenticate, sizeof(strHttpAuthenticate) - 1) == 0) {
                    MA_MakeAuthorizationCode(
                        &lineCp[sizeof(strHttpAuthenticate) - 1], param.pUserID,
                        param.pPassword, gMA.authCode);
                } else if (MAU_strncmp(lineCp, strHttpGbStatus, sizeof(strHttpGbStatus) - 1) == 0) {
                    gMA.gbCenterRes =
                        GetResponse(&lineCp[sizeof(strHttpGbStatus) - 1]);
                    if (gMA.gbCenterRes == 101) {
                        gMA.status |= STATUS_GBCENTER_ERR_101;
                    } else if (gMA.gbCenterRes != 0) {
                        gMA.taskError = MAAPIE_GB_CENTER;
                        gMA.taskErrorDetail = gMA.gbCenterRes;
                        param.headError = TRUE;
                    }
                } else if (MAU_strncmp(lineCp, strHttpGbAuthID, sizeof(strHttpGbAuthID) - 1) == 0) {
                    MAU_strcpy(gMA.authCode,
                        &lineCp[sizeof(strHttpGbAuthID) - 1]);
                }

                if ((nextCp[0] == '\r' && nextCp[1] == '\n')
                    || nextCp[0] == '\n') {
                    if (nextCp[0] == '\r') {
                        tmpLen -= 2;
                    } else {
                        tmpLen -= 1;
                    }
                    param.headEnd = TRUE;
                    if (tmpLen != 0) {
                        if (nextCp[0] == '\r') {
                            MAU_memcpy(&gMA.recvBuf.data[1], &nextCp[2], tmpLen);
                        } else {
                            MAU_memcpy(&gMA.recvBuf.data[1], &nextCp[1], tmpLen);
                        }
                        gMA.recvBuf.size = tmpLen + 1;
                    }
                    break;
                }

                curCp = nextCp;
                lineCp = gMA.prevBuf;
            }
        }

        if (!param.headEnd && tmpLen != 0) {
            MAU_memcpy(gMA.prevBuf, curCp, tmpLen);
            gMA.prevBufSize = tmpLen;
        }
        if (param.headEnd && tmpLen != 0) break;

        MA_InitBuffer(&gMA.recvBuf, gMA.recvPacket);
        MABIOS_Data(&gMA.recvBuf, NULL, 0, gMA.sockets[0]);
        break;

    case 50:
        param.counter = gMA.tcpDelayCounter[gMA.sioMode];
        gMA.taskStep++;

    case 51:
        if (param.counter-- == 0) {
            gMA.taskStep = param.nextStep;
        }
        break;

    case 100:
        MA_InitBuffer(&gMA.recvBuf, gMA.recvPacket);
        if (param.sendSize > MAX_TCP_DATA_SIZE) {
            MABIOS_Data(&gMA.recvBuf, param.pSendData, MAX_TCP_DATA_SIZE,
                gMA.sockets[0]);
            param.pSendData += MAX_TCP_DATA_SIZE;
            param.sendSize -= MAX_TCP_DATA_SIZE;
            param.nextStep = gMA.taskStep;
            gMA.taskStep = 50;
            break;
        }
        MABIOS_Data(&gMA.recvBuf, param.pSendData, param.sendSize,
            gMA.sockets[0]);
        param.headEnd = FALSE;
        param.headError = FALSE;
        param.headFound = FALSE;
        curCp = nextCp = NULL;
        InitPrevBuf();
        param.nextStep = 8;
        gMA.taskStep = 50;
        break;

    case 110:
        if (!(gMA.status & STATUS_BUFFER_EMPTY)) {
            if (param.pRecvData && gMA.recvBuf.size > 1) {
                dataLen = gMA.recvBuf.size - 1;
                if (param.recvBufSize >= dataLen) {
                    MAU_memcpy(param.pRecvData, &gMA.recvBuf.data[1], dataLen);
                    param.pRecvData += dataLen;
                    param.recvBufSize -= dataLen;
                    gMA.condition &= ~MA_CONDITION_BUFFER_FULL;
                    *param.pRecvSize += dataLen;
                } else {
                    MAU_memcpy(param.pRecvData, &gMA.recvBuf.data[1],
                        param.recvBufSize);
                    gMA.prevBufSize = dataLen - param.recvBufSize;
                    MAU_memcpy(gMA.prevBuf,
                        &gMA.recvBuf.data[1 + param.recvBufSize],
                        gMA.prevBufSize);
                    *param.pRecvSize += param.recvBufSize;
                    param.recvBufSize = 0;
                    gMA.condition |= MA_CONDITION_BUFFER_FULL;
                    MA_TaskSet(TASK_NONE, 0);
                    gMA.condition |= MA_CONDITION_APIWAIT;
                    break;
                }
            }
        } else {
            gMA.status &= ~STATUS_BUFFER_EMPTY;
        }

        if (MA_GetCondition() & MA_CONDITION_TCPCLOSED) {
            gMA.condition &= ~MA_CONDITION_BUFFER_FULL;
            gMA.taskStep = HttpGetNextStep(2);
            break;
        }
        MA_InitBuffer(&gMA.recvBuf, gMA.recvPacket);
        MABIOS_Data(&gMA.recvBuf, NULL, 0, gMA.sockets[0]);
        break;

    case 0xf0:
        MA_InitBuffer(&gMA.recvBuf, gMA.recvPacket);
        MABIOS_TCPDisconnect(&gMA.recvBuf, gMA.sockets[0]);
        gMA.taskStep++;
        break;

    case 0xf1:
        gMA.connMode = CONN_PPP;
        MA_SetCondition(MA_CONDITION_PPP);
        MA_SetApiError(gMA.taskError, gMA.taskErrorDetail);
        MA_TaskSet(TASK_NONE, 0);
        gMA.usedSockets[0] = FALSE;
        param.headFlags = 0;
        break;

    case 0xff:
        gMA.connMode = CONN_PPP;
        MA_SetCondition(MA_CONDITION_PPP);
        MA_TaskSet(TASK_NONE, 0);
        gMA.usedSockets[0] = FALSE;
        param.headFlags = 0;
        if (gMA.status & STATUS_GBCENTER_ERR_101) {
            MA_SetApiError(MAAPIE_GB_CENTER, 101);
            gMA.status &= ~STATUS_GBCENTER_ERR_101;
        }
        break;
    }
#undef param
}

static void CopyEEPROMString(char *pDest, const char *pSrc, int size)
{
    static int i;
    for (i = 0; i < size; i++) if ((*pDest++ = pSrc[i]) == '\0') break;
    if (i == size) *pDest = '\0';
}

static void CopyEEPROMData(int task, void *pDest)
{
    MA_TELDATA *pTelData = (MA_TELDATA *)pDest;
    char *pUserID = pDest;
    char *pMailID = pDest;

    switch (task) {
    case TASK_GETTEL:
        MAU_DecodeEEPROMTelNo(&gMA.prevBuf[EEPROM_TELNO1_OFFSET],
            pTelData->telNo);
        CopyEEPROMString(pTelData->comment,
            &gMA.prevBuf[EEPROM_COMMENT1_OFFSET], EEPROM_COMMENT_SIZE);
        break;

    case TASK_GETUSERID:
        CopyEEPROMString(pUserID, &gMA.prevBuf[EEPROM_USERID_OFFSET],
            EEPROM_USERID_SIZE);
        break;

    case TASK_GETMAILID:
        CopyEEPROMString(pMailID, &gMA.prevBuf[EEPROM_MAILID_OFFSET],
            EEPROM_MAILID_SIZE);
        break;
    }
}

static void MATASK_GetEEPROMData(void)
{
#define param gMA.param.geteepromdata
    if (gMA.recvCmd == (MAPROT_REPLY | MACMD_ERROR)) {
        switch (gMA.negaResCmd) {
        case MACMD_END:
            break;

        case MACMD_CHANGE_CLOCK:
            MA_DefaultNegaResProc();
            gMA.taskStep = 0xfa;
            break;

        case MACMD_EEPROM_READ:
            gMA.taskError = MAAPIE_EEPROM_SUM;
            gMA.taskErrorDetail = 0;
            gMA.taskStep = 0xfa;
            break;

        default:
            MA_DefaultNegaResProc();
            gMA.taskStep = 0xfb;
            break;
        }
    }

    switch (gMA.taskStep) {
    case 0:
        MABIOS_Start();
        gMA.taskStep++;
        break;

    case 1:
        gMA.taskStep++;
        MABIOS_ChangeClock(MA_SIO_WORD);
        break;

    case 2:
        MA_InitBuffer(&gMA.recvBuf, gMA.recvPacket);
        MABIOS_EEPROM_Read(&gMA.recvBuf, 0, 0x80);
        gMA.taskStep++;
        break;

    case 3:
        if (!EEPROMRegistrationCheck(&gMA.recvPacket[1])) {
            gMA.taskError = MAAPIE_REGISTRATION;
            gMA.taskErrorDetail = 0;
            gMA.taskStep = 0xfa;
            break;
        }
        MAU_memcpy(gMA.prevBuf, &gMA.recvBuf.data[1], 0x80);
        MA_InitBuffer(&gMA.recvBuf, gMA.recvPacket);
        MABIOS_EEPROM_Read(&gMA.recvBuf, 0x80, 0x40);
        gMA.taskStep++;
        break;

    case 4:
        MAU_memcpy(&gMA.prevBuf[0x80], &gMA.recvBuf.data[1], 0x40);
        if (!EEPROMSumCheck(gMA.prevBuf)) {
            gMA.taskError = MAAPIE_EEPROM_SUM;
            gMA.taskErrorDetail = 0;
            gMA.taskStep = 0xfa;
            break;
        }
        gMA.prevBufHasEEPROMData = TRUE;
        MABIOS_End();
        gMA.taskStep++;
        break;

    case 5:
        CopyEEPROMData(param.task, param.pData);
        gMA.taskStep++;
        break;

    case 6:
        MA_ChangeSIOMode(MA_SIO_BYTE);
        MA_Reset();
        MA_TaskSet(TASK_NONE, 0);
        break;

    case 0xfa:
        MABIOS_End();
        gMA.taskStep++;
        break;

    case 0xfb:
        MA_Reset();
        MA_SetApiError(gMA.taskError, gMA.errorDetail);
        MA_TaskSet(TASK_NONE, 0);
        break;
    }
#undef param
}

void MA_EEPROMRead(u8 *pData)
{
#define param gMA.param.eeprom_read
    SetApiCallFlag();
    if (!MA_ApiPreExe(TASK_EEPROM_READ)) {
        ResetApiCallFlag();
        return;
    }

    param.pData = pData;
    *(vu32 *)REG_TM3CNT = 0;
    MA_TaskSet(TASK_EEPROM_READ, 0);

    ResetApiCallFlag();

    if (!(*(vu32 *)REG_TM3CNT & TMR_IF_ENABLE)
        || !(*(vu32 *)REG_TM3CNT & TMR_ENABLE)) {
        MAAPI_Main();
    }
#undef param
}

static void MATASK_EEPROM_Read(void)
{
#define param gMA.param.eeprom_read
    if (gMA.recvCmd == (MAPROT_REPLY | MACMD_ERROR)) {
        switch (gMA.negaResCmd) {
        case MACMD_END:
            break;

        case MACMD_CHANGE_CLOCK:
        case MACMD_EEPROM_READ:
            MA_DefaultNegaResProc();
            gMA.taskStep = 0xfa;
            break;

        default:
            MA_DefaultNegaResProc();
            gMA.taskStep = 0xfb;
            break;
        }
    }

    switch (gMA.taskStep) {
    case 0:
        MABIOS_Start();
        gMA.taskStep++;
        break;

    case 1:
        gMA.taskStep++;
        MABIOS_ChangeClock(MA_SIO_WORD);
        break;

    case 2:
        MA_InitBuffer(&gMA.recvBuf, gMA.recvPacket);
        MABIOS_EEPROM_Read(&gMA.recvBuf, 0, 0x80);
        gMA.taskStep++;
        break;

    case 3:
        MAU_memcpy(param.pData, &gMA.recvBuf.data[1], 0x80);
        MA_InitBuffer(&gMA.recvBuf, gMA.recvPacket);
        MABIOS_EEPROM_Read(&gMA.recvBuf, 0x80, 0x40);
        gMA.taskStep++;
        break;

    case 4:
        MAU_memcpy(&param.pData[0x80], &gMA.recvBuf.data[1], 0x40);
        MABIOS_End();
        gMA.taskStep++;
        break;

    case 5:
        MA_ChangeSIOMode(MA_SIO_BYTE);
        MA_Reset();
        MA_TaskSet(TASK_NONE, 0);
        break;

    case 0xfa:
        MABIOS_End();
        gMA.taskStep++;
        break;

    case 0xfb:
        MA_Reset();
        MA_SetApiError(gMA.taskError, gMA.taskErrorDetail);
        MA_TaskSet(TASK_NONE, 0);
        break;
    }
#undef param
}

void MA_EEPROMWrite(const u8 *pData)
{
#define param gMA.param.eeprom_write
    SetApiCallFlag();
    if (!MA_ApiPreExe(TASK_EEPROM_WRITE)) {
        ResetApiCallFlag();
        return;
    }

    param.pData = pData;
    *(vu32 *)REG_TM3CNT = 0;
    MA_TaskSet(TASK_EEPROM_WRITE, 0);

    ResetApiCallFlag();

    if (!(*(vu32 *)REG_TM3CNT & TMR_IF_ENABLE)
        || !(*(vu32 *)REG_TM3CNT & TMR_ENABLE)) {
        MAAPI_Main();
    }
#undef param
}

static void MATASK_EEPROM_Write(void)
{
#define param gMA.param.eeprom_write
    if (gMA.recvCmd == (MAPROT_REPLY | MACMD_ERROR)) {
        switch (gMA.negaResCmd) {
        case MACMD_END:
            break;

        case MACMD_CHANGE_CLOCK:
        case MACMD_EEPROM_WRITE:
            MA_DefaultNegaResProc();
            gMA.taskStep = 0xfa;
            break;

        default:
            MA_DefaultNegaResProc();
            gMA.taskStep = 0xfb;
            break;
        }
    }

    switch (gMA.taskStep) {
    case 0:
        MABIOS_Start();
        gMA.taskStep++;
        break;

    case 1:
        gMA.taskStep++;
        MABIOS_ChangeClock(MA_SIO_WORD);
        break;

    case 2:
        MA_InitBuffer(&gMA.recvBuf, gMA.recvPacket);
        MABIOS_EEPROM_Write(&gMA.recvBuf, 0, param.pData, 0x80);
        gMA.taskStep++;
        break;

    case 3:
        MA_InitBuffer(&gMA.recvBuf, gMA.recvPacket);
        MABIOS_EEPROM_Write(&gMA.recvBuf, 0x80, &param.pData[0x80], 0x40);
        gMA.taskStep++;
        break;

    case 4:
        MABIOS_End();
        gMA.taskStep++;
        break;

    case 5:
        MA_ChangeSIOMode(MA_SIO_BYTE);
        MA_Reset();
        MA_TaskSet(TASK_NONE, 0);
        break;

    case 0xfa:
        MABIOS_End();
        gMA.taskStep++;
        break;

    case 0xfb:
        MA_Reset();
        MA_SetApiError(gMA.taskError, gMA.taskErrorDetail);
        MA_TaskSet(TASK_NONE, 0);
        break;
    }
#undef param
}

void MA_GetTel(MA_TELDATA *pTelData)
{
#define param gMA.param.geteepromdata
    SetApiCallFlag();
    if (!MA_ApiPreExe(TASK_GETTEL)) {
        ResetApiCallFlag();
        return;
    }

    MAU_memset(pTelData, sizeof(*pTelData), 0);
    if (gMA.prevBufHasEEPROMData == TRUE) {
        CopyEEPROMData(TASK_GETTEL, pTelData);
        ResetApiCallFlag();
        gMA.condition &= ~MA_CONDITION_APIWAIT;
        return;
    }

    param.task = TASK_GETTEL;
    param.pData = pTelData;
    *(vu32 *)REG_TM3CNT = 0;
    MA_TaskSet(TASK_GETEEPROMDATA, 0);
    gMA.condition |= MA_CONDITION_APIWAIT;
    ResetApiCallFlag();

    if (!(*(vu32 *)REG_TM3CNT & TMR_IF_ENABLE)
        || !(*(vu32 *)REG_TM3CNT & TMR_ENABLE)) {
        MAAPI_Main();
    }
#undef param
}

void MA_GetUserID(char *pUserIDBuf)
{
#define param gMA.param.geteepromdata
    SetApiCallFlag();
    if (!MA_ApiPreExe(TASK_GETUSERID)) {
        ResetApiCallFlag();
        return;
    }

    if (gMA.prevBufHasEEPROMData == TRUE) {
        CopyEEPROMData(TASK_GETUSERID, pUserIDBuf);
        ResetApiCallFlag();
        gMA.condition &= ~MA_CONDITION_APIWAIT;
        return;
    }

    MAU_memset(pUserIDBuf, EEPROM_USERID_SIZE + 1, 0);
    param.task = TASK_GETUSERID;
    param.pData = pUserIDBuf;
    *(vu32 *)REG_TM3CNT = 0;
    MA_TaskSet(TASK_GETEEPROMDATA, 0);
    gMA.condition |= MA_CONDITION_APIWAIT;
    ResetApiCallFlag();

    if (!(*(vu32 *)REG_TM3CNT & TMR_IF_ENABLE)
        || !(*(vu32 *)REG_TM3CNT & TMR_ENABLE)) {
        MAAPI_Main();
    }
#undef param
}

void MA_GetMailID(char *pBufPtr)
{
#define param gMA.param.geteepromdata
    SetApiCallFlag();
    if (!MA_ApiPreExe(TASK_GETMAILID)) {
        ResetApiCallFlag();
        return;
    }

    MAU_memset(pBufPtr, EEPROM_MAILID_SIZE + 1, 0);
    if (gMA.prevBufHasEEPROMData == TRUE) {
        CopyEEPROMData(TASK_GETMAILID, pBufPtr);
        ResetApiCallFlag();
        gMA.condition &= ~MA_CONDITION_APIWAIT;
        return;
    }

    param.task = TASK_GETMAILID;
    param.pData = pBufPtr;
    *(vu32 *)REG_TM3CNT = 0;
    MA_TaskSet(TASK_GETEEPROMDATA, 0);
    gMA.condition |= MA_CONDITION_APIWAIT;
    ResetApiCallFlag();

    if (!(*(vu32 *)REG_TM3CNT & TMR_IF_ENABLE)
        || !(*(vu32 *)REG_TM3CNT & TMR_ENABLE)) {
        MAAPI_Main();
    }
#undef param
}

void MA_GetSMTPServerName(char *pDest)
{
    CopyEEPROMString(pDest, gMA.serverConf.smtp, sizeof(gMA.serverConf.smtp));
}

void MA_GetPOP3ServerName(char *pDest)
{
    CopyEEPROMString(pDest, gMA.serverConf.pop3, sizeof(gMA.serverConf.pop3));
}

static void (* const taskProcTable[])(void) = {
    NULL,
    MATASK_InitLibrary,
    MATASK_InitLibrary,
    MATASK_TelServer,
    MATASK_Tel,
    MATASK_Receive,
    NULL,
    NULL,
    MATASK_Condition,
    MATASK_Condition,
    MATASK_Offline,
    MATASK_SMTP_Connect,
    MATASK_SMTP_Sender,
    MATASK_SMTP_Send,
    MATASK_SMTP_POP3_Quit,
    MATASK_POP3_Connect,
    MATASK_POP3_Stat,
    MATASK_POP3_List,
    MATASK_POP3_Retr,
    MATASK_POP3_Dele,
    MATASK_POP3_Head,
    MATASK_SMTP_POP3_Quit,
    MATASK_HTTP_GetPost,
    MATASK_HTTP_GetPost,
    NULL,
    NULL,
    NULL,
    MATASK_GetEEPROMData,
    MATASK_EEPROM_Read,
    MATASK_EEPROM_Write,
    MATASK_Stop,
    MATASK_TCP_Cut,
    MATASK_TCP_Connect,
    MATASK_TCP_Disconnect,
    MATASK_TCP_SendRecv,
    MATASK_GetHostAddress,
    NULL,
};

void MAAPI_Main(void)
{
    if (gMA.task != TASK_INITLIBRARY
        && gMA.task != TASK_INITLIBRARY2
        && gMA.task != TASK_TELSERVER
        && gMA.task != TASK_TEL
        && gMA.task != TASK_CONDITION
        && gMA.task != TASK_CONDITION2
        && gMA.task != TASK_RECEIVE
        && gMA.task != TASK_GETEEPROMDATA
        && gMA.task != TASK_EEPROM_READ
        && gMA.task != TASK_EEPROM_WRITE
        && gMA.task != TASK_STOP
        && !(gMA.status & STATUS_CONNECTED)) {
        return;
    }

    if (gMA.status & STATUS_CONNTEST
        || gMA.condition & MA_CONDITION_ERROR
        || gMA.condition & MA_CONDITION_BIOS_BUSY) {
        return;
    }

    if (gMA.status & STATUS_CONN_PTP && gMA.sendCmd == MACMD_DATA) {
        MATASK_P2P();
    }

    if (gMA.task) taskProcTable[gMA.task]();
}

u16 MAAPI_GetConditionFlag(void)
{
    return gMA.condition;
}

static u16 ErrDetailHexConv(u16 err)
{
    u16 hex;

    hex = 0;
    while (err >= 1000) {
        err -= 1000;
        hex += 0x1000;
    }
    while (err >= 100) {
        err -= 100;
        hex += 0x100;
    }
    while (err >= 10) {
        err -= 10;
        hex += 0x10;
    }
    hex += err;

    return hex;
}

u8 MAAPI_ErrorCheck(u16 *pProtocolError)
{
    if (!(gMA.condition & MA_CONDITION_ERROR)) {
        *pProtocolError = 0;
        return 0;
    }

    switch (gMA.error) {
    case MAAPIE_PROT_ILLEGAL_CMD:
        gMA.error = MAAPIE_SYSTEM;
        gMA.errorDetail = 0;
        break;

    case MAAPIE_PROT_CHECKSUM:
        gMA.error = MAAPIE_SYSTEM;
        gMA.errorDetail = 1;
        break;

    case MAAPIE_PROT_UNUSED3:
        gMA.error = MAAPIE_SYSTEM;
        gMA.errorDetail = 2;
        break;

    case MAAPIE_PROT_INTERNAL:
        gMA.error = MAAPIE_SYSTEM;
        gMA.errorDetail = 3;
        break;
    }

    gMA.condition &= ~MA_CONDITION_ERROR;

    if (pProtocolError != NULL) {
        switch (gMA.error) {
        case MAAPIE_SYSTEM:
        case MAAPIE_SMTP:
        case MAAPIE_POP3:
        case MAAPIE_HTTP:
        case MAAPIE_GB_CENTER:
            *pProtocolError = ErrDetailHexConv(gMA.errorDetail);
            break;

        default:
            *pProtocolError = 0;
            break;
        }
    }

    if (!(gMA.condition & MA_CONDITION_BIOS_BUSY)) {
        gMA.sendCmd = 0;
        gMA.recvCmd = 0;
    }
    return gMA.error;
}
