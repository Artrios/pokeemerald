/**
 * @file
 * @brief Miscellaneous subroutines
 *
 * Mostly copies of libc functions to avoid relying on it, as well as some
 * string parsing, conversion and socket tracking utility functions.
 */

#include "ma_sub.h"

#include <stddef.h>
#include <AgbDefine.h>
#include "ma_var.h"

/**
 * @brief Calculate the length of a string
 *
 * @param[in] str string
 * @return length of string, excluding null terminator
 */
int MAU_strlen(const char *str)
{
    static const char *eos;

    eos = str;
    while (*eos++ != '\0') {}
    return (u16)(eos - str - 1);
}

/**
 * @brief Copy a string
 *
 * @param[out] dest destination string
 * @param[in] src source string
 */
void MAU_strcpy(char *dest, const char *src)
{
    static const char *pSrc;

    pSrc = src;
    for (;;) {
        *dest++ = *pSrc;
        if (*pSrc == '\0') break;
        pSrc++;
    }
}

/**
 * @brief Copy a string, up to n bytes
 *
 * @param[out] dest destination string
 * @param[in] src source string
 * @param[in] len maximum bytes to copy
 * @return amount of copied bytes
 */
int MAU_strcpy_size(char *dest, const char *src, int len)
{
    static int ret;

    for (ret = 0; len != 0; len--, ret++) {
        *dest = *src;
        if (*src == '\0') break;
        dest++;
        src++;
    }
    if (len != 0) ret++;
    return ret;
}

/**
 * @brief Copy a string, up to n bytes or a newline sequence
 *
 * Copies the src string to the dest buffer up until either a CRLF or LF newline
 * sequence is found, or the maximum length is reached. The newline sequence
 * isn't copied. A pointer to the characters following the newline sequence in
 * the string is returned. If the newline is not found, a NULL pointer will be
 * returned instead.
 *
 * @param[out] dest destination string
 * @param[in] src source string
 * @param[in] len maximum bytes to copy
 * @return pointer following the newline if found, NULL otherwise
 */
const char *MAU_strncpy_CRLF_LF(char *dest, const char *src, int len)
{
    if (len == 0) return NULL;

    while (len != 0) {
        if (src[0] == '\r' && src[1] == '\n') {
            *dest = '\0';
            return src + 2;
        }
        if (src[0] == '\n') {
            *dest = '\0';
            return src + 1;
        }
        *dest++ = *src++;
        len--;
    }
    *dest = '\0';
    return NULL;
}

/**
 * @brief Find the character after the first CRLF sequence in a string
 *
 * Scans a string to find the first CRLF sequence, then returns a pointer to the
 * first character following it. If the sequence is not found, a NULL pointer
 * will be returned instead.
 *
 * @param[in] str string
 * @param[in] len maximum length of string
 * @return pointer following the CRLF sequence if found, NULL otherwise
 */
char *MAU_SearchCRLF(char *str, int len)
{
    if (len < 2) return NULL;

    while (len != 1) {
        if (str[0] == '\r' && str[1] == '\n') {
            *str = '\0';
            return &str[2];
        }
        str++;
        len--;
    }
    return NULL;
}

/**
 * @brief Concatenate two strings
 *
 * @param[out] dest appended string
 * @param[in] src string to append
 */
void MAU_strcat(char *dest, const char *src)
{
    while (*dest != '\0') dest++;
    for (;;) {
        *dest++ = *src;
        if (*src == '\0') break;
        src++;
    }
}

/**
 * @brief Locate first occurrence of character in string
 *
 * @param[in] str string
 * @param[in] c character
 * @return pointer to character if found, NULL otherwise
 */
const char *MAU_strchr(const char *str, int c)
{
    while (*str != '\0' && *str != (char)c) str++;
    if (*str == (char)c) return str;
    return NULL;
}

/**
 * @brief Locate last occurrence of character in string
 *
 * @param[in] str string
 * @param[in] c character
 * @return pointer to character if found, NULL otherwise
 */
const char *MAU_strrchr(const char *str, int c)
{
    static const char *start;
    static const char *string;

    string = str;
    start = str;
    while (*string++ != '\0') {}
    while (--string != start && *string != (char)c) {}
    if (*string == (char)c) return string;
    return NULL;
}

/**
 * @brief Find the character after the first space in a string
 *
 * Scans a string to find the first space character, then returns a pointer to
 * the first character following the space. If no space is found, or the space
 * is the last character in the string, a NULL pointer will be returned instead.
 *
 * @param[in] str string
 * @return pointer to character after first space, NULL if not found
 */
char *MAU_FindPostBlank(char *str)
{
    while (*str != '\0' && *str != ' ') str++;
    if (*str == '\0') return NULL;
    while (*str == ' ') str++;
    if (*str == '\0') return NULL;
    return str;
}

/**
 * @brief Compare two strings
 *
 * @param[in] str1 string to compare
 * @param[in] str2 string to compare
 * @return comparison result, 0 if strings are equal
 */
int MAU_strcmp(const char *str1, const char *str2)
{
    static int ret;

    ret = 0;
    for (;;) {
        ret = *str1 - *str2;
        if (ret) break;
        if (*str2 == '\0') break;
        str1++;
        str2++;
    }
    return ret;
}

/**
 * @brief Compare two strings, up to n bytes
 *
 * @param[in] str1 string to compare
 * @param[in] str2 string to compare
 * @param[in] len maximum bytes to compare
 * @return comparison result, 0 if strings are equal
 */
int MAU_strncmp(const char *str1, const char *str2, int len)
{
    if (len == 0) return 0;
    while (--len != 0) {
        if (*str1 == '\0') break;
        if (*str1 != *str2) break;
        str1++;
        str2++;
    }
    return *str1 - *str2;
}

/**
 * @brief Compare two strings, up to n bytes, case insensitively
 *
 * @param[in] str1 string to compare
 * @param[in] str2 string to compare
 * @param[in] len maximum bytes to compare
 * @return comparison result, 0 if strings are equal
 */
int MAU_strnicmp(const char *str1, const char *str2, int len)
{
    static int f;
    static int l;

    if (len != 0) {
        do {
            f = *str1++;
            if ('A' <= f) if (f <= 'Z') f += 'a' - 'A';
            l = *str2++;
            if ('A' <= l) if (l <= 'Z') l += 'a' - 'A';
        } while (--len != 0 && f != '\0' && f == l);
        return f - l;
    }
    return 0;
}

/**
 * @brief Copy memory area
 *
 * @param[out] dest destination memory
 * @param[in] src source memory
 * @param[in] size size of memory
 */
void MAU_memcpy(void *dest, const void *src, int size)
{
    const char *s = src;
    char *d = dest;

    while (size-- != 0) *d++ = *s++;
}

/**
 * @brief Fill memory with a constant byte
 *
 * @param[out] dest memory
 * @param[in] c fill value
 * @param[in] size size of memory
 */
void MAU_memset(void *dest, u8 c, int size)
{
    u8 *d = dest;

    while (size-- != 0) *d++ = c;
}

/**
 * @brief Convert an integer into a string
 *
 * Used by MAU_itoa() to perform the actual conversion.
 *
 * @param[in] num integer value
 * @param[out] str string
 * @param[in] base number base
 * @param[in] neg number is negative
 */
static void xtoa(int num, char *str, unsigned base, int neg)
{
    static char *p;
    static char *firstdig;
    static char temp;
    static unsigned digval;

    // If the number is negative, prefix with the sign
    p = str;
    if (neg) {
        *p++ = '-';
        num = -num;
    }

    // Write number backwards into the string
    firstdig = p;
    do {
        digval = num % base;
        num = num / base;
        if (digval >= 10) {
            *p++ = digval + 'a' - 10;
        } else {
            *p++ = digval + '0';
        }
    } while (num != 0);

    // Reverse the string
    *p-- = '\0';
    do {
        temp = *p;
        *p = *firstdig;
        *firstdig = temp;
        p--;
        firstdig++;
    } while (firstdig < p);
}

/**
 * @brief Convert an integer number into a string
 *
 * This function converts an integer into a string of an any base. Digits with
 * values bigger than 10 are represented as letters starting from "a".
 *
 * Negative numbers are only handled in base 10.
 *
 * @param[in] num integer value
 * @param[out] str string
 * @param[in] base number base
 * @return string
 */
char *MAU_itoa(int num, char *str, int base)
{
    if (base == 10 && num < 0) {
        xtoa(num, str, 10, TRUE);
    } else {
        xtoa(num, str, base, FALSE);
    }
    return str;
}

/**
 * @brief Convert a string into an integer
 *
 * Converts the initial portion of the string pointed to by str into an integer.
 * The string may optionally start with a '-' or a '+' sign, and any following
 * base 10 digits will be interpreted.
 *
 * @param[in] str string
 * @return integer value
 */
int MAU_atoi(const char *str)
{
    static char c;
    static char sign;
    static int total;

    c = *str++;

    sign = c;
    if (sign == '-' || sign == '+') {
        c = *str++;
    }

    total = 0;
    while (MAU_isdigit(c)) {
        total = total * 10 + (c - '0');
        c = *str++;
    }

    if (sign == '-') return -total;
    return total;
}

static const char telCharTable[] = "0123456789#*";

/**
 * @brief Decode an EEPROM-packed telephone number
 *
 * Converts a given EEPROM phone number into an equivalent string.
 *
 * The function will read up to EEPROM_TELNO_SIZE bytes from the EEPROM buffer
 * unless a terminator is encountered. The string buffer must be able to
 * hold at least EEPROM_TELNO_SIZE * 2 + 1 bytes.
 *
 * The phone number contained in the EEPROM is a binary-coded decimal number,
 * where 0xa and 0xb are '#' and '*', respectively. A value of 0xf indicates the
 * end of the number.
 *
 * @param[in] pNum EEPROM data
 * @param[out] pStr string
 */
void MAU_DecodeEEPROMTelNo(const u8 *pNum, char *pStr)
{
    static int i;
    static u8 hi;
    static u8 lo;

    for (i = 0; i < EEPROM_TELNO_SIZE; i++) {
        hi = (*pNum >> 4) & 0xf;
        lo = (*pNum >> 0) & 0xf;
        pNum++;
        if (hi == 0xf) {
            *pStr = '\0';
            break;
        }
        *pStr++ = telCharTable[hi];
        if (lo == 0xf) {
            *pStr = '\0';
            break;
        }
        *pStr++ = telCharTable[lo];
    }

    if (i == EEPROM_TELNO_SIZE) *pStr = '\0';
}

/**
 * @brief Verify a telephone number string
 *
 * Verifies a given string only contains characters allowed by the mobile
 * adapter to represent a phone number.
 *
 * @param[in] pStr string
 * @return boolean
 */
int MAU_IsValidTelNoStr(const char *pStr)
{
    static unsigned i;

    for (; *pStr != '\0'; pStr++) {
        for (i = 0; i < sizeof(telCharTable); i++) {
            if (*pStr == telCharTable[i]) break;
        }
        if (i == sizeof(telCharTable)) return FALSE;
    }
    return TRUE;
}

/**
 * @brief Check if a string starts with a CRLF terminator
 *
 * @param[in] pStr string
 * @param[in] len length of string
 * @return boolean
 */
int MAU_CheckCRLF(const char *pStr, u16 len)
{
    if (len < 3) return FALSE;
    if (pStr[len - 2] != '\r') return FALSE;
    if (pStr[len - 1] != '\n') return FALSE;
    return TRUE;
}

/**
 * @brief Add a socket ID to the list of used sockets
 *
 * Does not do anything when the list is full.
 *
 * @param[in] sock socket ID
 */
void MAU_Socket_Add(u8 sock)
{
    int i;

    for (i = 0; i < NUM_SOCKETS; i++) {
        if (!gMA.usedSockets[i]) {
            gMA.usedSockets[i] = TRUE;
            gMA.sockets[i] = sock;
            break;
        }
    }
}

/**
 * @brief Delete a socket ID from the list of used sockets
 *
 * Does not do anything if the socket ID doesn't appear in the list.
 *
 * @param[in] sock socket ID
 */
void MAU_Socket_Delete(u8 sock)
{
    int i;

    for (i = 0; i < NUM_SOCKETS; i++) {
        if (gMA.sockets[i] == sock) {
            gMA.usedSockets[i] = FALSE;
            break;
        }
    }
}

/**
 * @brief Check if a socket ID appears in the list of used sockets
 *
 * @bug Doesn't check if the socket is actually used
 *
 * @param[in] sock socket ID
 * @return boolean
 */
int MAU_Socket_Search(u8 sock)
{
    int i;
    int ret;

    ret = FALSE;
    for (i = 0; i < NUM_SOCKETS; i++) {
        if (gMA.sockets[i] == sock) {
            ret = TRUE;
            break;
        }
    }
    return ret;
}

/**
 * @brief Get the number of sockets in use
 *
 * @return count
 */
int MAU_Socket_GetNum(void)
{
    int i;
    int ret;

    ret = 0;
    for (i = 0; i < NUM_SOCKETS; i++) {
        if (gMA.usedSockets[i] == TRUE) ret++;
    }
    return ret;
}

/**
 * @brief Check if there's a free slot in the list of used sockets
 *
 * @return boolean
 */
int MAU_Socket_FreeCheck(void)
{
    int i;
    int ret;

    ret = FALSE;
    for (i = 0; i < NUM_SOCKETS; i++) {
        if (gMA.usedSockets[i] == FALSE) {
            ret = TRUE;
            break;
        }
    }
    return ret;
}

/**
 * @brief Check if an IP address matches the currently connected address
 *
 * @param[in] pAddr IP address
 * @return boolean
 */
int MAU_Socket_IpAddrCheck(const u8 *pAddr)
{
    if (pAddr[0] != gMA.socketAddr[0]) return FALSE;
    if (pAddr[1] != gMA.socketAddr[1]) return FALSE;
    if (pAddr[2] != gMA.socketAddr[2]) return FALSE;
    if (pAddr[3] != gMA.socketAddr[3]) return FALSE;
    return TRUE;
}

/**
 * @brief Clear the list of used sockets
 */
void MAU_Socket_Clear(void)
{
    int i;

    for (i = 0; i < NUM_SOCKETS; i++) {
        gMA.usedSockets[i] = FALSE;
    }
    gMA.socketAddr[0] = 0;
    gMA.socketAddr[1] = 0;
    gMA.socketAddr[2] = 0;
    gMA.socketAddr[3] = 0;
}
