#ifndef _MA_BIOS
#define _MA_BIOS

#include "ma_var.h"

void MABIOS_Init(void);
int MA_GetStatus(void);
u16 MA_GetCondition(void);
u8 MA_ErrorCheck(void);
void MA_SetError(u8 error);
void MA_ChangeSIOMode(u8 mode);
void MA_SetDataInterval(u16 byteInter, u16 wordInter);
int MA_GetCallTypeFromHarwareType(u8 harware);
void MABIOS_Null(void);
void MABIOS_Start(void);
void MABIOS_Start2(void);
void MABIOS_End(void);
void MABIOS_Tel(u8 callType, const char *pNumber);
void MABIOS_Offline(void);
void MABIOS_WaitCall(void);
void MABIOS_Data(MA_BUF *pRecvBuf, const u8 *pSendData, u8 sendSize, u8 socket);
void MABIOS_ReInit(void);
void MABIOS_CheckStatus(MA_BUF *pRecvBuf);
void MABIOS_CheckStatus2(MA_BUF *pRecvBuf);
void MABIOS_ChangeClock(u8 mode);
void MABIOS_EEPROM_Read(MA_BUF *pRecvBuf, u8 offset, u8 size);
void MABIOS_EEPROM_Write(MA_BUF *pRecvBuf, u8 offset, const u8 *pSendData,
    u8 size);
void MABIOS_PPPConnect(MA_BUF *pRecvBuf, const char *pUserID,
    const char *pPassword, const u8 *dns1, const u8 *dns2);
void MABIOS_PPPDisconnect(void);
void MABIOS_TCPConnect(MA_BUF *pRecvBuf, u8 *pAddr, u16 port);
void MABIOS_TCPDisconnect(MA_BUF *pRecvBuf, u8 socket);
void MABIOS_UDPConnect(MA_BUF *pRecvBuf, u8 *pAddr, u16 port);
void MABIOS_UDPDisconnect(MA_BUF *pRecvBuf, u8 socket);
void MABIOS_DNSRequest(MA_BUF *pRecvBuf, char *pServerName);
void MABIOS_TestMode(void);
void MA_CancelRequest(void);
void MA_BiosStop(void);
void MA_SendRetry(void);
void MA_RecvRetry(void);
int MA_ProcessCheckStatusResponse(u8 response);
void MA_DefaultNegaResProc(void);
void MA_IntrTimer(void);
void MA_Bios_disconnect(void);
void MA_IntrSerialIO(void);

#endif // _MA_BIOS
