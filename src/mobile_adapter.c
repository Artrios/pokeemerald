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
    if(isError = maWait() > 0) {
        return isError;
    }

    MA_GetUserID(maUserID);
    if(isError = maWait() > 0) {
        return isError;
    }

    MA_GetMailID(maMailID);
    if(isError = maWait() > 0) {
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
    int siz;
    int maCondition;

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

//Example function to show how to download a Pokemon
//This function:
//1. Initialises the Mobile Adapter Library
//2. Reads the data on the Mobile Adapter's EEPROM
//3. Makes a PPP connection to a server to gain internet access
//4. An upload function sends a POST request to a domain with the content "GIFT"
//   4.1 The server checks for "GIFT"
//   4.2 The server sends a response packet back with the Pokemon's data as the content
//   4.3 The response data is copied to an array to use
//   4.4 The upload function is called until all data is copied over
//5. The connection is disconnected and the Mobile Adapter Library stopped
//6. The player's party size is calculated
//7. The Pokemon is added to the party or sent to the PC if the party is full (Or is not given at all if the PC is full too)
//8. If everything goes well a 0 is returned. If a Mobile Adapter error is thrown that error code is returned. If the PC is full '2' is returned.
int maExample(void){
    int errNum; //The error code for the error encountered
    const char *pURL;   //URL to query
    char pHeadBuf[100]; //Buffer to hold the HTTP header
    const u8 *pSendData;    //Data to send
    pSendData="GIFT";
    u16 recvBufSize = 100;  //Size of received data
    u8 pRecvData[recvBufSize];  //Buffer to hold received data
    u16 pRecvSize;  //How many bytes were copied to pRecvData after calling maUpload once
    int tracker=0;  //Keep track of No of bytes copied to pRecvData
    char pUserID[17];  //User ID from the MA EEPROM, has max lenght of 17
    char pPassword[17];    //User passwoord from the MA EEPROM, has max lenght of 17
    char *maMailID; //User mail ID from the MA EEPROM
    int i = 0;

    MA_TELDATA maTel;   //MA Telephone struct

    //Initialise MA Library
    errNum = maInitLibrary();

    //If there was an error stop MA Library and return the error code
    if(errNum !=0){
        maKill();
        return errNum;
    }
    
    //Get EEPROM Data
    errNum=maGetEEPROMData(&maTel, pUserID, maMailID);

    //If there was an error stop MA Library and return the error code
    if(errNum !=0){
        maKill();
        return errNum;
    }

    //Set your password, must end in Null byte
    memcpy(pPassword, "Password1\0", 10);

    //Makes a call and establishes a PPP connection 
    errNum=maConnectServer(&maTel,pUserID,pPassword);

    //If there was an error stop MA Library and return the error code
    if(errNum !=0){
        maKill();
        return errNum;
    }

    //Set the domain
    pURL="http://www.PutYourDomainHere.com";
    //Query a domain, send a POST request and store the response data in pRecvData
    errNum = maUpload(pURL, NULL, 0, pSendData, 4, pRecvData, recvBufSize, &pRecvSize, pUserID, pPassword);

    //If there was an error stop MA Library and return the error code
    if(errNum !=0){
        maKill();
        return errNum;
    }
    
    //If pRecvSize<recvBufSize then not all the data that was expected was copied into pRecvData
    if(pRecvSize<recvBufSize){
        tracker=pRecvSize;  //Keep track of the number of bytes copied
        while(tracker<recvBufSize){ //While that is less than the Buffer size
            //Keep calling maUpload until
            errNum = maUpload(pURL, NULL, 0, pSendData, 4, &pRecvData[tracker], recvBufSize-tracker, &pRecvSize, pUserID, pPassword);
            if(errNum !=0){
                maKill();
                return errNum;
            }
            tracker=tracker+pRecvSize;
        }
    }

    //Stop MA Library
    if(maDisconnect() == 0){
        maEnd();
    }
    else{
        maKill();
    }

    //Get number of Pokemon in party
    for (i = 0; i < PARTY_SIZE; i++)
    {
        if (GetMonData(&gPlayerParty[i], MON_DATA_SPECIES, NULL) == SPECIES_NONE)
            break;
    }

    //If no room in party send mon to PC, otherwise put in party
    if(i >= PARTY_SIZE){
        struct Pokemon *pRecvData;
        i = CopyMonToPC(pRecvData); //Checks for full PC
        if(i==2){   //Box is full
            return i;
        }
    }
    else {
        CopyMon(&gPlayerParty[i], pRecvData, 100);
    }

    //Everything went well, return 0
    return 0;
}
