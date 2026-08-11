#ifndef _MA_SUB
#define _MA_SUB

#include <AgbTypes.h>

/// Check if a character is a digit
#define MAU_isdigit(c) ((c) >= '0' && (c) <= '9')

int MAU_strlen(const char *str);
void MAU_strcpy(char *dest, const char *src);
int MAU_strcpy_size(char *dest, const char *src, int len);
const char *MAU_strncpy_CRLF_LF(char *dest, const char *src, int len);
char *MAU_SearchCRLF(char *str, int len);
void MAU_strcat(char *dest, const char *src);
const char *MAU_strchr(const char *str, int c);
const char *MAU_strrchr(const char *str, int c);
char *MAU_FindPostBlank(char *str);
int MAU_strcmp(const char *str1, const char *str2);
int MAU_strncmp(const char *str1, const char *str2, int len);
int MAU_strnicmp(const char *str1, const char *str2, int len);
void MAU_memcpy(void *dest, const void *src, int size);
void MAU_memset(void *dest, u8 c, int size);
char *MAU_itoa(int num, char *str, int base);
int MAU_atoi(const char *str);
void MAU_DecodeEEPROMTelNo(const u8 *pNum, char *pStr);
int MAU_IsValidTelNoStr(const char *pStr);
int MAU_CheckCRLF(const char *pStr, u16 len);
void MAU_Socket_Add(u8 sock);
void MAU_Socket_Delete(u8 sock);
int MAU_Socket_Search(u8 sock);
int MAU_Socket_GetNum(void);
int MAU_Socket_FreeCheck(void);
int MAU_Socket_IpAddrCheck(const u8 *pAddr);
void MAU_Socket_Clear(void);

#endif // _MA_SUB
