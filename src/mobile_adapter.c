#include "global.h"
#include "gpu_regs.h"
#include "main.h"
#include "mobile_adapter.h"
#include "pokemon.h"
#include "pokedex.h"

extern void MA_IntrSerialIO(void);
extern void MA_IntrTimer(void);
extern u8 CopyMonToPC(struct Pokemon *mon);

//Checks if the MA is connected by starting the library and seeing if there's an errors. Ends the communication as well
bool8 maConnected(void){
    int isError = 0;

    isError = maInitLibrary();
    maEnd();

    if(isError == 0){
        return TRUE;
    } else {
        return FALSE;
    }

}

// Start the MA Library
int maInitLibrary(void){
    gIntrTable[1] = MA_IntrSerialIO;
    gIntrTable[2] = MA_IntrTimer;
    u8 hardware;

    MA_InitLibrary(&hardware);

    return maWait();
}

//Disconnect Telephone line  
int maDisconnect(void)
{
    MA_Offline();
    return maWait();
}

//Forcefully terminate Mobile Adapter Communication 
void maKill(void)
{
    MA_Stop();
    maEnd();
}

//Ends the Library Communication and restore the correct interrupts  
void maEnd(void)
{
    MA_End();
    RestoreSerialTimer3IntrHandlers();
}

//Get data stored in the MA EEPROM
int maGetEEPROMData(MA_TELDATA *maTel, char *maUserID, char *maMailID){
    int isError = 0;

    MA_GetTel(maTel);
    isError = maWait();
    if(isError > 0) {
        return isError;
    }

    MA_GetUserID(maUserID);
    isError = maWait();
    if(isError > 0) {
        return isError;
    }

    MA_GetMailID(maMailID);
    isError = maWait();
    if(isError > 0) {
        return isError;
    }

    return 0;
}

//Connect to a Server (PPP)
int maConnectServer(const MA_TELDATA *pTelNo, const char *pUserID, const char *pPassword){
    MA_TelServer(pTelNo->telNo,pUserID,pPassword);
    return maWait();
}

//Download data
int maDownload(const char *pURL, char *pHeadBuf, u16 headBufSize, u8 *pRecvData, u16 recvBufSize, u16 *pRecvSize, const char *pUserID, const char *pPassword){
    MA_HTTP_Get(pURL, pHeadBuf, headBufSize, pRecvData, recvBufSize, pRecvSize, pUserID, pPassword);

    return maWait();
}

//Upload data (and then also download data)
int maUpload(const char *pURL, char *pHeadBuf, u16 headBufSize, const u8 *pSendData, u16 sendSize, u8 *pRecvData, u16 recvBufSize, u16 *pRecvSize, const char *pUserID, const char *pPassword){
    MA_HTTP_Post(pURL, pHeadBuf, headBufSize, pSendData, sendSize, pRecvData, 4, pRecvSize, pUserID, pPassword);
    return maWait();
}

//Call a Phone (P2P)
int maCallPhone(const char *pTelNo){
    MA_Tel(pTelNo);
    return maWait();
}

//Receive a Phone call (P2P)
int maReceivePhoneCall(void){
    MA_Receive();
    return maWait();
}

//Send Data to Phone
int maSendData(const u8 *pSendData, u8 sendSize, u8 *pResult){
    MA_SData(pSendData, sendSize, pResult);
    return maWait();
}

//Receive Data from Phone
int maReceiveData(u8 *pRecvData, u8 *pRecvSize){
    MA_GData(pRecvData, pRecvSize);
    return maWait();
}


//Waits until action has been completed, check if there's an error and supplies the error code if there is (Nearly all MA functions should use this as their return value, except for maKill and maEnd)
int maWait(void){
    u8 maError=0;
    u16 maErrorProtocol=0;
    u16 maCondition=0;

    do {
        maCondition = MAAPI_GetConditionFlag();

        if (maCondition & MA_CONDITION_BUFFER_FULL)
	        return 0;

        if(maCondition & MA_CONDITION_ERROR){
            maError=MAAPI_ErrorCheck(&maErrorProtocol);
            break;
        }

    } while(maCondition & MA_CONDITION_APIWAIT);

    return (maError << 16) | maErrorProtocol;
}
