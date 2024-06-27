#ifndef GUARD_GBA_MOBILE_ADAPTER_H
#define GUARD_GBA_MOBILE_ADAPTER_H


//===================================================================
//	Constants
//===================================================================

//-----------------------------
//	Condition Flags
//-----------------------------
#define MA_CONDITION_APIWAIT			((u16)0x0001)	// API Processing in Progress
#define MA_CONDITION_ERROR				((u16)0x0002)	// Error Occurred
#define MA_CONDITION_BUFFER_FULL		((u16)0x0004)	// Receive Buffer Full
#define MA_CONDITION_PTP_GET			((u16)0x0008)	// Interterminal Communication Reception
#define MA_CONDITION_CONNECT			((u16)0x0010)	// Phone Connection in Progress

#define MA_CONDITION_SHIFT				(8)
#define MA_CONDITION_IDLE				(0x00)			// Idle
#define MA_CONDITION_PPP				(0x01)			// PPP Connection in Progress
#define MA_CONDITION_P2P_SEND			(0x02)			// PtoP Connection in Progress (Caller Send)
#define MA_CONDITION_P2P_RECV			(0x03)			// PtoP Connection in Progress (Caller Receive)
#define MA_CONDITION_SMTP				(0x04)			// SMTP Session
#define MA_CONDITION_POP3				(0x05)			// POP3 Session
#define MA_CONDITION_RESERVED			(0x06)			// Reserved
#define MA_CONDITION_LOST				(0x07)			// Terminal not Connected or Terminal Power Off
#define MA_CONDITION_MASK				((u16)0xff00)

//-----------------------------
//	Adapter No.
//-----------------------------
#define MATYPE_PDC						(0x00)			// PDC Adapter
#define MATYPE_CDMA						(0x01)			// CDMA Adapter
#define MATYPE_PHS_DoCoMo				(0x02)			// PHS (DoCoMo､ Aster) Adapter
#define MATYPE_PHS_Pocket				(0x03)			// PHS (DDI-Pocket) Adapter

//-----------------------------
//	Error Codes
//-----------------------------
#define MAAPIE_MA_NOT_FOUND				(0x10)			// MA not Connected
#define MAAPIE_CALL_FAILED				(0x11)			// Cannot Make a Call (Phone not Connected)
#define MAAPIE_BUSY						(0x12)			// On the Line (Phone)
#define MAAPIE_CONNECT					(0x13)			// Line Connection Failure
#define MAAPIE_EEPROM_SUM				(0x14)			// EEPROM Checksum Error
#define MAAPIE_SYSTEM					(0x15)			// System Error
#define MAAPIE_CANNOT_EXECUTE_LOW		(0x16)			// Unprocessable State (Low Level)
#define MAAPIE_ILLEGAL_PARAMETER_LOW	(0x17)			// Parameter Abnormality (Low Level)

#define MAAPIE_ILLEGAL_PARAMETER		(0x20)			// Parameter Anomalies During API Calls
#define MAAPIE_CANNOT_EXECUTE			(0x21)			// Unhandled State
#define MAAPIE_PPP_CONNECT				(0x22)			// PPP Connection Failed
#define MAAPIE_OFFLINE					(0x23)			// Communication Already Disconnected
#define MAAPIE_TCP_CONNECT				(0x24)			// TCP Connection Failed
#define MAAPIE_REGISTRATION				(0x25)			// EEPROM Registration Error
#define MAAPIE_TIMEOUT					(0x26)			// Adapter Response Timeout Error

#define MAAPIE_SMTP						(0x30)			// SMTP Error
#define MAAPIE_POP3						(0x31)			// POP3 Error
#define MAAPIE_HTTP						(0x32)			// HTTP Error
#define MAAPIE_GB_CENTER				(0x33)			// GB Centre Error

// MAAPIE_POP3 Detailed Error Code
#define MAPOP3E_USER_NOT_FOUND			(0x02)			// (Response to User) User ID Invalid
#define MAPOP3E_LOGIN_FAILED			(0x03)			// (Response to Pass) Invalid Password, Mailbox Locked
#define MAPOP3E_NO_SUCH_MESSAGE			(0x04)			// (Response to LIST, RETR, DELE, TOP) No Specified Email Number
#define MAPOP3E_UNKNOWN					(0x00)			// Unknown


//===================================================================
//	Type Definition
//===================================================================
// Received Data
typedef struct {

	char	telNo[17];		// Telephone Number
	char	comment[17];	// Comment

} MA_TELDATA;

//===================================================================
//	External Function Declaration
//===================================================================
// Interrupt Handler
void MA_IntrSerialIO(void);
void MA_IntrTimer(void);

// API Functions
void MA_InitLibrary(
	u8 *pHardwareType
);

void MA_InitLibrary2(
	u8 *pHardwareType
);

void MA_End(void);

void MA_TelServer(
	const char *pTelNo,
	const char *pUserID,
	const char *pPassword
);

void MA_Tel(
	const char *pTelNo
);

void MA_Receive(void);

void MA_SData(
	const u8 *pSendData,
	u8 sendSize,
	u8 *pResult
);

void MA_GData(
	u8 *pRecvData,
	u8 *pRecvSize
);

void MA_Condition
(
	u8 *pCondition
);

void MA_Condition2
(
	u8 *pCondition
);

void MA_Offline(void);

void MA_SMTP_Connect(
	const char *pMailAddress
);

void MA_SMTP_Sender(
	const char * const pRecipients[]	//const char *pRecipients[]を変更	2001.03.12
);

void MA_SMTP_Send(
	const char *pSendData,				//const u8 *pSendData,を変更	2001.03.12
	u16 sendSize,
	int endFlag
);

void MA_SMTP_Quit(void);

void MA_POP3_Connect(
	const char *pUserID,
	const char *pPassword
);

void MA_POP3_Stat(
	u16 *pNum,
	u32 *pSize
);

void MA_POP3_List(
	u16	mailNo,
	u32 *pSize
);

void MA_POP3_Retr(
	u16 mailNo,
	u8	*pRecvData,
	u16 recvBufSize,
	u16 *pRecvSize
);

void MA_POP3_Dele(
	u16 mailNo
);

void MA_POP3_Head(
	u16	mailNo,
	u8	*pRecvData,
	u16	recvBufSize,
	u16	*pRecvSize
);

void MA_POP3_Quit(void);

void MA_HTTP_Get(
	const char	*pURL,
	char		*pHeadBuf,
	u16			headBufSize,
	u8			*pRecvData,
	u16			recvBufSize,
	u16			*pRecvSize,
	const char	*pUserID,
	const char	*pPassword
);

void MA_HTTP_Post(
	const char	*pURL,
	char		*pHeadBuf,
	u16			headBufSize,
	const u8	*pSendData,
	u16			sendSize,
	u8			*pRecvData,
	u16			recvBufSize,
	u16			*pRecvSize,
	const char	*pUserID,
	const char	*pPassword
);

void MA_GetTel(
	MA_TELDATA *pTelData
);

void MA_GetUserID(
	char *pUserIDBuf
);

void MA_GetMailID(
	char *pBufPtr
);

void MA_Stop(void);

void MA_TCP_Cut(void);

// API Status Check
u16 MAAPI_GetConditionFlag(void);

u8 MAAPI_ErrorCheck(
	u16 *pProtocolError
);


bool8 maConnected(void);
int maInitLibrary(void);
int maDisconnect(void);
void maKill(void);
void maEnd(void);
int maGetEEPROMData(MA_TELDATA *maTel, char *maUserID, char *maMailID);
int maConnectServer(const MA_TELDATA *pTelNo, const char *pUserID, const char *pPassword);
int maDownload(const char *pURL, char *pHeadBuf, u16 headBufSize, u8 *pRecvData, u16 recvBufSize, u16 *pRecvSize, const char *pUserID, const char *pPassword);
int maUpload(const char *pURL, char *pHeadBuf, u16 headBufSize, const u8 *pSendData, u16 sendSize, u8 *pRecvData, u16 recvBufSize, u16 *pRecvSize, const char *pUserID, const char *pPassword);
int maCallPhone(const char *pTelNo);
int maReceivePhoneCall(void);
int maSendData(const u8 *pSendData, u8 sendSize, u8 *pResult);
int maReceiveData(u8 *pRecvData, u8 *pRecvSize);
int maWait(void);

#endif // GUARD_GBA_MOBILE_ADAPTER_H