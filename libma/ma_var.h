/**
 * @file
 * @brief Common private variables
 */

#ifndef _MA_VAR_H
#define _MA_VAR_H

#include <AgbTypes.h>

#define MAAPI_MAGIC ('M' << 0 | 'A' << 8 | 'G' << 16 | 'B' << 24)

#define MATYPE_GBC 0
#define MATYPE_GBA 1

#define MAAPIE_PROT_ILLEGAL_CMD 0x83
#define MAAPIE_PROT_CHECKSUM 0x84
#define MAAPIE_PROT_INTERNAL 0x85
#define MAAPIE_PROT_UNUSED3 0x87

/// Communicating with the adapter
#define MA_CONDITION_BIOS_BUSY ((u16)0x0020)
/// Remote closed the connection
#define MA_CONDITION_TCPCLOSED ((u16)0x0040)

#define MA_CONDITION_UNMETERED_F (0x80)  // No call fees apply (MA_Condition only)

#define STATUS_CONNECTED (1 << 0)  // Connected to the adapter
#define STATUS_TIMER_ENABLED (1 << 1)  // Execute the timer interrupt
#define STATUS_CONNTEST (1 << 2)  // If the command fails the adapter is disconnected
#define STATUS_SIO_RETRY (1 << 3)  // Retrying the last packet
#define STATUS_SIO_ERR_CHECKSUM (1 << 4)  // Checksum error encountered
#define STATUS_API_CALL (1 << 5)  // Pause interrupts
#define STATUS_SIO_RECV_DONE (1 << 6)  // Received a packet correctly
#define STATUS_INTR_TIMER (1 << 7)  // Pause api calls
#define STATUS_INTR_SIO (1 << 8)  // Pause api calls
#define STATUS_CONN_PTP (1 << 9)  // Connected with a p2p connection
#define STATUS_PTP_SEND_DONE (1 << 10)  // Triggered p2p data send
#define STATUS_BIOS_STOP (1 << 11)  // MA_BiosStop called
#define STATUS_BIOS_RESTART (1 << 12)  // MA_CancelRequest called, (re)start connection
#define STATUS_PTP_SEND (1 << 13)  // Send p2p data in the next interrupt
#define STATUS_SIO_SEND_TIMEOUT (1 << 14)  // Timeout during MA_IntrSio_Send
#define STATUS_BUFFER_EMPTY (1 << 15)  // gMA.prevBuf has just been emptied
#define STATUS_GBCENTER_ERR_101 (1 << 16)  // GB-Status: 101 (fees not paid)

#define CONN_OFFLINE 0
#define CONN_PPP 3
#define CONN_SMTP 4
#define CONN_POP3 5
#define CONN_HTTP 6
#define CONN_P2P_SEND 7
#define CONN_P2P_RECV 8

#define MACMD_NULL 0x0f
#define MACMD_START 0x10
#define MACMD_END 0x11
#define MACMD_TEL 0x12
#define MACMD_OFFLINE 0x13
#define MACMD_WAIT_CALL 0x14
#define MACMD_DATA 0x15
#define MACMD_REINIT 0x16
#define MACMD_CHECK_STATUS 0x17
#define MACMD_CHANGE_CLOCK 0x18
#define MACMD_EEPROM_READ 0x19
#define MACMD_EEPROM_WRITE 0x1a
#define MACMD_DATA_END 0x1f
#define MACMD_PPP_CONNECT 0x21
#define MACMD_PPP_DISCONNECT 0x22
#define MACMD_TCP_CONNECT 0x23
#define MACMD_TCP_DISCONNECT 0x24
#define MACMD_UDP_CONNECT 0x25
#define MACMD_UDP_DISCONNECT 0x26
#define MACMD_DNS_REQUEST 0x28
#define MACMD_TEST_MODE 0x3f
#define MACMD_ERROR 0x6e

#define NUM_SOCKETS 2

#define MAPROT_REPLY 0x80
#define MAPROT_TYPE_MASK 0xf0
#define MAPROT_TYPE_MASTER (MAPROT_REPLY | 0x0)
#define MAPROT_TYPE_SLAVE (MAPROT_REPLY | 0x8)

#define MAPROT_BODY_SIZE 0x100
#define MAPROT_HEADER_SIZE 6
#define MAPROT_FOOTER_SIZE 6

#define EEPROM_DNS_OFFSET 4
#define EEPROM_DNS_SIZE 8
#define EEPROM_USERID_OFFSET 12
#define EEPROM_USERID_SIZE 32
#define EEPROM_MAILID_OFFSET 44
#define EEPROM_MAILID_SIZE 30
#define EEPROM_SERVER_OFFSET 74
#define EEPROM_SERVER_SIZE 44
#define EEPROM_TELNO1_OFFSET 118
#define EEPROM_TELNO_SIZE 8
#define EEPROM_COMMENT1_OFFSET 126
#define EEPROM_COMMENT_SIZE 16

enum tasks {
    TASK_NONE,
    TASK_INITLIBRARY,
    TASK_INITLIBRARY2,
    TASK_TELSERVER,
    TASK_TEL,
    TASK_RECEIVE,
    TASK_SDATA,
    TASK_GDATA,
    TASK_CONDITION,
    TASK_CONDITION2,
    TASK_OFFLINE,
    TASK_SMTP_CONNECT,
    TASK_SMTP_SENDER,
    TASK_SMTP_SEND,
    TASK_SMTP_QUIT,
    TASK_POP3_CONNECT,
    TASK_POP3_STAT,
    TASK_POP3_LIST,
    TASK_POP3_RETR,
    TASK_POP3_DELE,
    TASK_POP3_HEAD,
    TASK_POP3_QUIT,
    TASK_HTTP_GET,
    TASK_HTTP_POST,
    TASK_GETTEL,
    TASK_GETUSERID,
    TASK_GETMAILID,
    TASK_GETEEPROMDATA,
    TASK_EEPROM_READ,
    TASK_EEPROM_WRITE,
    TASK_STOP,
    TASK_TCP_CUT,
    TASK_TCP_CONNECT,
    TASK_TCP_DISCONNECT,
    TASK_TCP_SENDRECV,
    TASK_GETHOSTADDRESS,
    TASK_GETLOCALADDRESS,
};

enum ma_sio_modes {
    MA_SIO_BYTE,
    MA_SIO_WORD,
    MA_NUM_SIO_MODES,
};

enum ma_intr_modes {
    MA_INTR_IDLE,
    MA_INTR_SEND,
    MA_INTR_RECV,
    MA_INTR_WAIT,
};

typedef struct {
    vu16 state;
    vu16 size;
    vu16 readCnt;
    vu16 checkSum;
    vu8 *pRead;
    vu8 *pWrite;
} MA_IOBUF;

typedef struct {
    u16 size;
    u8 *data;
} MA_BUF;

typedef struct {
    u16 magic;
    u8 cmd;
    u8 pad;
    u16 size;
} MAPROT_HEADER;

typedef struct {
    u8 checkSum_H;
    u8 checkSum_L;
    u8 device;
    u8 pad[3];
} MAPROT_FOOTER;

typedef struct {
    u32 cmd;
} PARAM_TCP_CUT;

typedef struct {
    u8 *pHardwareType;
} PARAM_INITLIBRARY;

typedef struct {
    u8 *pSocket;
    u8 *pAddr;
    u32 port;
} PARAM_TCP_CONNECT;

typedef struct {
    u32 socket;
} PARAM_TCP_DISCONNECT;

typedef struct {
    u32 socket;
    u8 *pSendData;
    u32 sendSize;
    u8 *pRecvSize;
} PARAM_TCP_SENDRECV;

typedef struct {
    u8 *pHost;
    char *pServerName;
} PARAM_GETHOSTADDRESS;

typedef struct {
    const char *pTelNo;
    const char *pUserID;
    const char *pPassword;
} PARAM_TELSERVER;

typedef struct {
    const char *pTelNo;
} PARAM_TEL;

typedef struct {
    const u8 *pSendData;
    u32 sendSize;
} PARAM_P2P;

typedef struct {
    u8 *pCondition;
    u32 offline;
} PARAM_CONDITION;

typedef struct {
    u32 timeout;
} PARAM_OFFLINE;

typedef struct {
    const char *pMailAddress;
} PARAM_SMTP_CONNECT;

typedef struct {
    const char * const *pRecipients;
} PARAM_SMTP_SENDER;

typedef struct {
    const char *pSendData;
    u32 sendSize;
    u32 endFlag;
    u32 totalSize;  // unused
    u32 timeout;
    u32 nextStep;
} PARAM_SMTP_SEND;

typedef struct {
    u32 timeout;
} PARAM_SMTP_POP3_QUIT;

typedef struct {
    char *pServerName;
    char *pUserID;
    char *pPassword;
} PARAM_POP3_CONNECT;

typedef struct {
    u16 *pNum;
    u32 *pSize;
} PARAM_POP3_STAT;

typedef struct {
    u32 *pSize;
} PARAM_POP3_LIST;

typedef struct {
    u8 *pRecvData;
    u32 recvBufSize;
    u16 *pRecvSize;
    u32 _pad[4];
    u32 respFound;
} PARAM_POP3_RETR;

typedef struct {
    u32 task;
    void *pData;
} PARAM_GETEEPROMDATA;

typedef struct {
    u8 *pData;
} PARAM_EEPROM_READ;

typedef struct {
    const u8 *pData;
} PARAM_EEPROM_WRITE;

typedef struct {
    u32 task;
    u32 unused4;
    u8 *pRecvData;
    u32 recvBufSize;
    u16 *pRecvSize;
    const u8 *pSendData;
    u32 sendSize;
    const char *pServerPath;
    u32 serverPathLen;
    const char *pServerPathBkp;
    u32 serverPathLenBkp;
    u32 headBufSize;
    u32 serverAuth;
    u32 authStep;
    char *pHeadBuf;
    u32 headEnd;
    u32 headError;
    const char *pUserID;
    const char *pPassword;
    u32 headFlags;
    u32 headFound;
    u32 counter;
    u32 nextStep;
} PARAM_HTTP_GETPOST;

typedef struct {
    vu8 error;
    vu16 condition;
    vu8 intrMode;
    vu8 sioMode;
    vu8 hardwareType;
    vu8 footerError;  // unused
    vu16 timerIntInter[MA_NUM_SIO_MODES];
    vu16 timerInter;
    vu16 retryCount;
    vu8 interval;
    u32 nullCounter[MA_NUM_SIO_MODES];
    u32 timeoutCounter[MA_NUM_SIO_MODES];
    u32 P2PCounter[MA_NUM_SIO_MODES];
    u32 timeout200msecCounter[MA_NUM_SIO_MODES];
    u32 tcpDelayCounter[MA_NUM_SIO_MODES];
    vu32 counter;
    vu32 status;
    vu8 sendCmd;
    vu8 recvCmd;
    u16 checkSum;
    vu8 sendFooter[4];
    vu8 recvFooter[4];  // unused
    vu8 negaResCmd;
    vu8 negaResErr;
    u8 recvRubbishCount;
    u8 tcpConnectRetryCount;
    u8 hwCondition[3];
    u32 apiMagic;
    vu8 connMode;
    vu16 errorDetail;
    vu8 smtpSender;
    vu8 task;
    vu8 taskStep;
    u8 sockets[NUM_SOCKETS];
    u8 prevBufHasEEPROMData;
    u8 taskError;
    u16 taskErrorDetail;
    u8 socketAddr[4];
    union {
        PARAM_TCP_CUT tcp_cut;
        PARAM_INITLIBRARY initlibrary;
        PARAM_TCP_CONNECT tcp_connect;
        PARAM_TCP_DISCONNECT tcp_disconnect;
        PARAM_TCP_SENDRECV tcp_sendrecv;
        PARAM_GETHOSTADDRESS gethostaddress;
        PARAM_TELSERVER telserver;
        PARAM_TEL tel;
        PARAM_P2P p2p;
        PARAM_CONDITION condition;
        PARAM_OFFLINE offline;
        PARAM_SMTP_CONNECT smtp_connect;
        PARAM_SMTP_SENDER smtp_sender;
        PARAM_SMTP_SEND smtp_send;
        PARAM_SMTP_POP3_QUIT smtp_pop3_quit;
        PARAM_POP3_CONNECT pop3_connect;
        PARAM_POP3_STAT pop3_stat;
        PARAM_POP3_LIST pop3_list;
        PARAM_POP3_RETR pop3_retr;
        PARAM_HTTP_GETPOST http_getpost;
        PARAM_GETEEPROMDATA geteepromdata;
        PARAM_EEPROM_READ eeprom_read;
        PARAM_EEPROM_WRITE eeprom_write;
    } param;
    u8 usedSockets[NUM_SOCKETS];
    u8 localAddr[4];
    u16 _unused210;
    u8 recvPacket[MAPROT_HEADER_SIZE + MAPROT_BODY_SIZE + MAPROT_FOOTER_SIZE];
    MA_BUF recvBuf;
    MA_IOBUF sendIoBuf;
    MA_IOBUF recvIoBuf;
    MA_IOBUF tempIoBuf;
    u8 sendPacket[MAPROT_HEADER_SIZE + MAPROT_BODY_SIZE + MAPROT_FOOTER_SIZE];
    u8 biosRecvPacket[4];
    u8 replyFooter[4];
    MA_BUF biosRecvBuf;
    MA_BUF *pRecvBuf;
    MA_IOBUF *pSendIoBuf;
    struct {
        u8 dns1[4];
        u8 dns2[4];
    } dnsConf;
    struct {
        char smtp[20];
        char pop3[20];
        u32 http;
    } serverConf;
    char strBuf[269];
    u8 prevBuf[637];
    u16 prevBufSize;
    char endLineBuf[6];
    u16 httpRes;
    u16 gbCenterRes;
    u8 authCode[94];
} MA_VAR;

extern MA_VAR gMA;

#define CPU_F 16780000
#define TIMER_FLAGS (TMR_ENABLE | TMR_IF_ENABLE | TMR_PRESCALER_1024CK)
#define TIMER_MS(ms) ((int)(((long long)CPU_F * (ms)) / (1024 * 1000) + 1))
#define TIMER_COUNTER_MS(ms) (0x10000 - TIMER_MS(ms))

#define MA_Reset() \
{ \
    gMA.connMode = CONN_OFFLINE; \
    MA_ChangeSIOMode(MA_SIO_BYTE); \
    gMA.timerInter = gMA.timerIntInter[gMA.sioMode]; \
    gMA.counter = 0; \
    gMA.intrMode = MA_INTR_IDLE; \
    gMA.status &= ~STATUS_CONNECTED; \
    gMA.status &= ~STATUS_CONN_PTP; \
    gMA.status &= ~STATUS_PTP_SEND_DONE; \
    gMA.status &= ~STATUS_PTP_SEND; \
    gMA.status &= ~STATUS_CONNTEST; \
    gMA.condition &= ~MA_CONDITION_PTP_GET; \
    gMA.condition &= ~MA_CONDITION_CONNECT; \
    MA_SetCondition(MA_CONDITION_IDLE); \
    MAU_Socket_Clear(); \
    MA_SetCondition(MA_CONDITION_IDLE); \
}

#define MA_SetCondition(cond) \
{ \
    gMA.condition &= ~MA_CONDITION_MASK; \
    gMA.condition |= (cond) << MA_CONDITION_SHIFT; \
}

#define MA_InitBuffer(pBuf, pData) \
{ \
    (pBuf)->size = 0; \
    (pBuf)->data = (pData); \
}

#endif // _MA_VAR_H
