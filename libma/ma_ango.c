/**
 * @file
 * @brief Authentication algorithm
 *
 * This file exports a single function, that generates a reply to an
 * authentication challenge from the server. This is done by hashing the
 * password with a random key sent by the server, adding the username and
 * scrambling the data with the key in a recoverable manner.
 *
 * The effect of this is twofold: The password is made unrecoverable, and the
 * data is obfuscated and different every time. The generated result is only
 * valid for a single request, as the server will reply with a session token for
 * use during the remainder of the communication.
 *
 * Of course, the chosen hashing algorithm is weak, the key exchange is insecure
 * and obscurity isn't security, but we're talking about in-house authentication
 * for a low-power device in an era where SSL was rare. It should be noted that
 * this authentication challenge doesn't secure the remaining communication.
 *
 * TL Note: "angō-ka" (暗号化) means "encryption"
 */

#include "ma_ango.h"

#include "ma_sub.h"
#include "md5.h"

#define HASH_SIZE 0x10
#define KEY_SIZE_BIN 0x24
#define KEY_SIZE_TEXT 0x30

static void gb_MakeSecretCode(const char *key, const char *userid,
    const char *password, u8 *out);
static void gb_OutSecretCode(int length, const char *key, u8 *out);
static void gb_CreateMD5Hash(u8 *out, const char *key, const char *password);
static void gb_BitHalfMove(u8 *out, const u8 *key);
static void gb_BitChangeAndRotation(u8 *data, const u8 *key);
static void CalcValueMD5(u8 *data, u32 size, u8 *out);
static void Base64_encode(int length, u8 *data, char *out);
static void Base64_decode(int length, const char *string, u8 *out);

static int i, j, k;
static int len;

/**
 * @brief Generate an authentication challenge reply
 *
 * Takes a base64 key, userid and password and combines them to calculate an
 * appropriate reply. Concatenates the input key (after stripping the final 4
 * bytes) with a generated secret code, both in base64, and stores them in the
 * output buffer.
 *
 * The output buffer must be able to hold 92 + 1 bytes, for the terminator.
 *
 * The key must be at least 48 bytes of valid base64 text, without null bytes.
 * The maximum length for userid is 20, and for password it's 51, but they're
 * generally not expected to be longer than 16 bytes.
 *
 * @param[in] key base64 data
 * @param[in] userid userid string
 * @param[in] password password string
 * @param[out] out challenge reply
 */
void MA_MakeAuthorizationCode(const char *key, const char *userid,
    const char *password, char *out)
{
    static u8 seq_bin[KEY_SIZE_BIN];
    static char seq_text[KEY_SIZE_TEXT + 1];
    static u8 onetime_key_bin[KEY_SIZE_BIN];
    static char onetime_key_text[KEY_SIZE_TEXT - 4 + 1];

    seq_text[0] = '\0';
    onetime_key_text[0] = '\0';
    out[0] = '\0';

    // Create secret code
    gb_MakeSecretCode(key, userid, password, seq_bin);

    // Encrypt secret code
    gb_OutSecretCode(KEY_SIZE_TEXT, key, seq_bin);
    Base64_encode(KEY_SIZE_BIN, seq_bin, seq_text);

    // Shave 4 bytes off of key
    Base64_decode(KEY_SIZE_TEXT, key, onetime_key_bin);
    Base64_encode(KEY_SIZE_BIN - 4, onetime_key_bin, onetime_key_text);

    // Concatenate key and secret code
    MAU_strcpy(out, onetime_key_text);
    MAU_strcpy(&out[KEY_SIZE_TEXT - 4], seq_text);
}

/**
 * @brief Calculate the secret code
 *
 * Creates an array of md5(key + password) + userid, with padding.
 *
 * Key buffer must contain at least KEY_SIZE_TEXT bytes, and may not contain
 * null bytes. Output buffer must be able to hold at least KEY_SIZE_BIN bytes.
 *
 * @bug maximum userid size is 20, due to expected output buffer size.
 *
 * @param[in] key key buffer
 * @param[in] userid string
 * @param[in] password string
 * @param[out] out secret code
 */
static void gb_MakeSecretCode(const char *key, const char *userid,
    const char *password, u8 *out)
{
    static u8 hash[HASH_SIZE + 1];
    static int j;

    MAU_memset(hash, 0, sizeof(hash));

    // Hash the key and password, and append the userid
    gb_CreateMD5Hash(hash, key, password);
    MAU_memcpy(out, hash, HASH_SIZE);
    MAU_memcpy(&out[HASH_SIZE], userid, MAU_strlen(userid));

    // Pad the resulting array with 0xff bytes up to KEY_SIZE_BIN
    j = HASH_SIZE + MAU_strlen(userid);
    MAU_memset(&out[j], 0xff, KEY_SIZE_BIN - j);
}

/**
 * @brief Encrypt the secret out
 *
 * Uses the base64 string in key to encrypt the secret out. This process is
 * reversible given the correct key.
 *
 * @bug Obscurity isn't security
 *
 * @param[in] length maximum length of key
 * @param[in] key base64 string
 * @param[out,in] out buffer to encrypt
 */
static void gb_OutSecretCode(int length, const char *key, u8 *out)
{
    static u8 dest[KEY_SIZE_BIN + 1];
    static u8 result[KEY_SIZE_BIN + 1];

    MAU_memset(dest, 0, sizeof(dest));

    Base64_decode(length, key, dest);
    gb_BitHalfMove(result, dest);
    gb_BitChangeAndRotation(out, result);
}

/**
 * @brief Create the MD5 hash of the random key and the password
 *
 * Concatenates exactly KEY_SIZE_TEXT bytes from the key with the terminated
 * string in password. Calculates the md5sum of the resulting string up to but
 * not including the first null byte.
 *
 * @bug Maximum password length is 51, due to the limited size buffer.
 * @bug Key buffer must be at least 48 bytes and may not contain null bytes.
 *
 * @param[out] out output hash
 * @param[in] key key buffer
 * @param[in] password password string
 */
static void gb_CreateMD5Hash(u8 *out, const char *key, const char *password)
{
    static char buf[100];

    MAU_memset(buf, 0, sizeof(buf));

    MAU_memcpy(buf, key, KEY_SIZE_TEXT);
    len = MAU_strlen(password);
    MAU_memcpy(&buf[KEY_SIZE_TEXT], password, len);
    len = MAU_strlen(buf);
    CalcValueMD5(buf, len, out);
}

/**
 * @brief Shuffle the key
 *
 * Reads a KEY_SIZE_BIN-sized array and extracts the even- and odd-numbered bits
 * into two consecutive arrays of KEY_SIZE_BIN / 2, writing them to the output.
 *
 * @param[out] out shuffled array
 * @param[in] key input array
 */
static void gb_BitHalfMove(u8 *out, const u8 *key)
{
    static int half;

    MAU_memset(out, 0, KEY_SIZE_BIN + 1);

    half = KEY_SIZE_BIN / 2;
    for (i = 0; i < half; i++) {
        j = i * 2;
        out[i] |= (key[j] & 0x40) << 1;
        out[i] |= (key[j] & 0x10) << 2;
        out[i] |= (key[j] & 0x04) << 3;
        out[i] |= (key[j] & 0x01) << 4;
        out[i] |= (key[j + 1] & 0x40) >> 3;
        out[i] |= (key[j + 1] & 0x10) >> 2;
        out[i] |= (key[j + 1] & 0x04) >> 1;
        out[i] |= (key[j + 1] & 0x01) >> 0;
    }

    for (; i < KEY_SIZE_BIN; i++) {
        j = (i - half) * 2;
        out[i] |= (key[j] & 0x80) << 0;
        out[i] |= (key[j] & 0x20) << 1;
        out[i] |= (key[j] & 0x08) << 2;
        out[i] |= (key[j] & 0x02) << 3;
        out[i] |= (key[j + 1] & 0x80) >> 4;
        out[i] |= (key[j + 1] & 0x20) >> 3;
        out[i] |= (key[j + 1] & 0x08) >> 2;
        out[i] |= (key[j + 1] & 0x02) >> 1;
    }
}

/**
 * @brief Encrypt an array with a key
 *
 * Performs an XOR operation of two KEY_SIZE_BIN-sized arrays, afterwards
 * rotating bit 0, 3 and 6 in each byte of the resulting data.
 *
 * @param[out,in] data array to encrypt
 * @param[in] key encryption key
 */
static void gb_BitChangeAndRotation(u8 *data, const u8 *key)
{
    static u8 buf;

    for (i = 0; i < KEY_SIZE_BIN; i++) {
        u8 *outp = &data[i];
        const u8 *inp = &key[i];

        u8 c = *outp;
        c ^= *inp;
        buf = c & 0xb6;
        buf |= (c & 0x08) << 3;
        buf |= (c & 0x01) << 3;
        buf |= (c & 0x40) >> 6;
        *outp = buf;
    }
}

/**
 * @brief Calculate MD5 hash
 *
 * Helper using md5c.c to get the MD5 hash of a single continuous block of data.
 * This is the only function that uses the md5 library.
 *
 * Output buffer must be able to hold at least HASH_SIZE bytes.
 *
 * @param[in] data data to hash
 * @param[in] size size of data
 * @param[out] out hash
 */
static void CalcValueMD5(u8 *data, u32 size, u8 *out)
{
    static MD5_CTX context;

    MAU_memset(&context, 0, sizeof(context));
    MD5Init(&context);
    MD5Update(&context, data, size);
    MD5Final(out, &context);
}

static const char xchg[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

/**
 * @brief Encode base64 string
 *
 * Creates a null-terminated base64 string based on the given data.
 *
 * Output string must be able to hold at least ceil(length * 4 / 3) + 1 bytes.
 *
 * @param[in] length size of data
 * @param[in] data input data
 * @param[out] out base64 string
 */
static void Base64_encode(int length, u8 *data, char *out)
{
    static u32 code;

    len = 0;
    while (len < length) {
        // Extract up to 3 bytes (24 bits) into code
        code = 0;
        for (i = 0; i < 3; i++) {
            if (len >= length) {
                code <<= (3 - i) * 8;
                goto brk;
            }
            code = (code << 8) | *data++;
            len++;
        }
    brk:

        // Write four 6-bit symbols (24 bits) into the resulting string
        for (k = 0; k < 4; k++) {
            if (k <= i) {
                *out++ = xchg[(code >> ((3 - k) * 6)) % 64];
            } else {
                *out++ = '=';
            }
        }
    }

    *out = '\0';
}

/**
 * @brief Decode base64 string
 *
 * Parses a base64 string to extract its data. Stops when either the maximum
 * length of the string is reached, when the terminator is reached, or when an
 * invalid character is found. The buffer is null-terminated.
 *
 * Output buffer must be able to hold at least floor(length * 3 / 4) + 1 bytes.
 *
 * @bug Null terminator was likely not intended, overflows some buffers.
 *
 * @param[in] length maximum length of string
 * @param[in] string base64 string
 * @param[out] out output data
 */
static void Base64_decode(int length, const char *string, u8 *out)
{
    static const u8 base64RevTable[] = {
        0xff /*   */, 0xff /* ! */, 0xff /* " */, 0xff /* # */,
        0xff /* $ */, 0xff /* % */, 0xff /* & */, 0xff /* ' */,
        0xff /* ( */, 0xff /* ) */, 0xff /* * */, 0x3e /* + */,
        0xff /* , */, 0xff /* - */, 0xff /* . */, 0x3f /* / */,
        0x34 /* 0 */, 0x35 /* 1 */, 0x36 /* 2 */, 0x37 /* 3 */,
        0x38 /* 4 */, 0x39 /* 5 */, 0x3a /* 6 */, 0x3b /* 7 */,
        0x3c /* 8 */, 0x3d /* 9 */, 0xff /* : */, 0xff /* ; */,
        0xff /* < */, 0xff /* = */, 0xff /* > */, 0xff /* ? */,
        0xff /* @ */, 0x00 /* A */, 0x01 /* B */, 0x02 /* C */,
        0x03 /* D */, 0x04 /* E */, 0x05 /* F */, 0x06 /* G */,
        0x07 /* H */, 0x08 /* I */, 0x09 /* J */, 0x0a /* K */,
        0x0b /* L */, 0x0c /* M */, 0x0d /* N */, 0x0e /* O */,
        0x0f /* P */, 0x10 /* Q */, 0x11 /* R */, 0x12 /* S */,
        0x13 /* T */, 0x14 /* U */, 0x15 /* V */, 0x16 /* W */,
        0x17 /* X */, 0x18 /* Y */, 0x19 /* Z */, 0xff /* [ */,
        0xff /* \ */, 0xff /* ] */, 0xff /* ^ */, 0xff /* _ */,
        0xff /* ` */, 0x1a /* a */, 0x1b /* b */, 0x1c /* c */,
        0x1d /* d */, 0x1e /* e */, 0x1f /* f */, 0x20 /* g */,
        0x21 /* h */, 0x22 /* i */, 0x23 /* j */, 0x24 /* k */,
        0x25 /* l */, 0x26 /* m */, 0x27 /* n */, 0x28 /* o */,
        0x29 /* p */, 0x2a /* q */, 0x2b /* r */, 0x2c /* s */,
        0x2d /* t */, 0x2e /* u */, 0x2f /* v */, 0x30 /* w */,
        0x31 /* x */, 0x32 /* y */, 0x33 /* z */, 0xff /* { */,
        0xff /* | */, 0xff /* } */, 0xff /* ~ */, 0xff /*   */,
    };

    static u32 code;
    static int c;
    static int byte;

    for (byte = 0; byte < length;) {
        code = 0;

        // Extract up to 4 symbols from the string
        // 6 bits per character, 6*4 = 24 bits
        for (i = 0; i < 4; i++) {
            if (*string >= 0x20 && *string < 0x80) {
                c = base64RevTable[*string - 0x20];
            } else {
                c = 0xff;
            }
            string++;
            byte++;

            // If we encountered an invalid character, pad with zero
            if (c == 0xff) {
                code = code << ((4 - i) * 6);
                break;
            }
            code = code << 6 | c;
        }

        // Write the extracted bytes to the output
        // 8 bits per byte, 6*3 = 24 bits
        for (k = 0; k < (i - 1); k++) {
            *out++ = code >> ((2 - k) * 8);
        }
    }

    *out = '\0';
}
