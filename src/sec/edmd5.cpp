#ifdef WINS
#include <windows.h>
#else
#include "winreplsecur.h"
#include <winrepl.h>
#endif
#include <stdlib.h> // NULL
#include <stdio.h>
#include "cmem.h"
#include "pkcsasn.h"
#include "secur.h"
#pragma hdrstop
#include "edtypes.h"
#include "edmd5.h"

HMDCTX EDInitDigest()
{
    return(new CMDCtx());
}

void EDAddDigest(HMDCTX hMD, LPBYTE buf, ULONG len)
{
    hMD->Update(buf, len);
}

int EDGetDigest(HMDCTX hMD, LPBYTE buf, ULONG len, LPBYTE Digest)
{
    if (!hMD)
    {
        hMD = new CMDCtx();
        if (!hMD)
            return(-1);
    }

    if (len)
        hMD->Update(buf, len);

    hMD->Final(Digest);

    delete hMD;
    return(0);
}


CMDCtx :: CMDCtx()
{
    m_Buf[0] = 0x67452301;
    m_Buf[1] = 0xEFCDAB89;
    m_Buf[2] = 0x98BADCFE;
    m_Buf[3] = 0x10325476;

    m_Bytes  = 0;
}

/*
 * Update context to reflect the concatenation of another buffer full
 * of bytes.
 */
void CMDCtx :: Update(LPBYTE buf, ULONG len)
{
    unsigned t;

    /* Update bitcount */
    
    t = m_Bytes & 0x3F;
    m_Bytes += len;

    if (t)
    {
        BYTE *p = m_In + t;
        UINT s = 64 - t;
        if (len < s)
            s = len;
        memcpy(p, buf, s);
        if (s + t < 64)
            return;
        Transform(m_In);
        buf += s;
        len -= s;
    }


    while (len >= 64)
    {
        Transform(buf);
        buf += 64;
        len -= 64;
    }

    memcpy(m_In, buf, len);
}

/*
 * Final wrapup - pad to 64-byte boundary with the bit pattern 
 * 1 0* (64-bit count of bits processed, MSB-first)
 */
void CMDCtx :: Final(LPBYTE digest)
{
    int count;
    BYTE *p;

    count = m_Bytes & 0x3F;

    p = m_In + count;
    *p++ = 0x80;

    count = 64 - 4 - 1 - count;     // Count zbyvalicich bytu - posledni DWORD na delku
    if (count < 4)                  // IF zbyva mene nez DWORD
    {
        memset(p, 0, count + 4);    // Vynulovat cely zbytek vstupniho bufferu
        Transform(m_In);            // Transformovat
        p     = m_In;               // Vynulovat skoro cely vstupni buffer
        count = 60;
    } 
    memset(p, 0, count);            // Vynulovat zbytek vstupniho bufferu

    ((unsigned *) m_In)[15] = m_Bytes; // Do posledniho DWORDU zapsat delku
    Transform(m_In);
    memcpy(digest, m_Buf, 16);
}

/* The four core functions - F1 is optimized somewhat */

/* #define F1(x, y, z) (x & y | ~x & z) */
#define F1(x, y, z) (z ^ (x & (y ^ z)))
#define F2(x, y, z) F1(z, x, y)
#define F3(x, y, z) (x ^ y ^ z)
#define F4(x, y, z) (y ^ (x | ~z))

/* This is the central step in the MD5 algorithm. */
#define MDSTEP(f, w, x, y, z, data, s) (w += f(x, y, z) + data,  w = w<<s | w>>(32-s),  w += x)

/*
 * The core of the MD5 algorithm, this alters an existing MD5 hash to
 * reflect the addition of 16 longwords of new data.  MD5Update blocks
 * the data and converts bytes into longwords for this routine.
 */
void CMDCtx :: Transform(LPBYTE inb)
{
    register unsigned a, b, c, d;
    unsigned *in = (unsigned*)inb;

    a = m_Buf[0];
    b = m_Buf[1];
    c = m_Buf[2];
    d = m_Buf[3];

    MDSTEP(F1, a, b, c, d, in[0] + 0xd76aa478, 7);
    MDSTEP(F1, d, a, b, c, in[1] + 0xe8c7b756, 12);
    MDSTEP(F1, c, d, a, b, in[2] + 0x242070db, 17);
    MDSTEP(F1, b, c, d, a, in[3] + 0xc1bdceee, 22);
    MDSTEP(F1, a, b, c, d, in[4] + 0xf57c0faf, 7);
    MDSTEP(F1, d, a, b, c, in[5] + 0x4787c62a, 12);
    MDSTEP(F1, c, d, a, b, in[6] + 0xa8304613, 17);
    MDSTEP(F1, b, c, d, a, in[7] + 0xfd469501, 22);
    MDSTEP(F1, a, b, c, d, in[8] + 0x698098d8, 7);
    MDSTEP(F1, d, a, b, c, in[9] + 0x8b44f7af, 12);
    MDSTEP(F1, c, d, a, b, in[10] + 0xffff5bb1, 17);
    MDSTEP(F1, b, c, d, a, in[11] + 0x895cd7be, 22);
    MDSTEP(F1, a, b, c, d, in[12] + 0x6b901122, 7);
    MDSTEP(F1, d, a, b, c, in[13] + 0xfd987193, 12);
    MDSTEP(F1, c, d, a, b, in[14] + 0xa679438e, 17);
    MDSTEP(F1, b, c, d, a, in[15] + 0x49b40821, 22);

    MDSTEP(F2, a, b, c, d, in[1] + 0xf61e2562, 5);
    MDSTEP(F2, d, a, b, c, in[6] + 0xc040b340, 9);
    MDSTEP(F2, c, d, a, b, in[11] + 0x265e5a51, 14);
    MDSTEP(F2, b, c, d, a, in[0] + 0xe9b6c7aa, 20);
    MDSTEP(F2, a, b, c, d, in[5] + 0xd62f105d, 5);
    MDSTEP(F2, d, a, b, c, in[10] + 0x02441453, 9);
    MDSTEP(F2, c, d, a, b, in[15] + 0xd8a1e681, 14);
    MDSTEP(F2, b, c, d, a, in[4] + 0xe7d3fbc8, 20);
    MDSTEP(F2, a, b, c, d, in[9] + 0x21e1cde6, 5);
    MDSTEP(F2, d, a, b, c, in[14] + 0xc33707d6, 9);
    MDSTEP(F2, c, d, a, b, in[3] + 0xf4d50d87, 14);
    MDSTEP(F2, b, c, d, a, in[8] + 0x455a14ed, 20);
    MDSTEP(F2, a, b, c, d, in[13] + 0xa9e3e905, 5);
    MDSTEP(F2, d, a, b, c, in[2] + 0xfcefa3f8, 9);
    MDSTEP(F2, c, d, a, b, in[7] + 0x676f02d9, 14);
    MDSTEP(F2, b, c, d, a, in[12] + 0x8d2a4c8a, 20);

    MDSTEP(F3, a, b, c, d, in[5] + 0xfffa3942, 4);
    MDSTEP(F3, d, a, b, c, in[8] + 0x8771f681, 11);
    MDSTEP(F3, c, d, a, b, in[11] + 0x6d9d6122, 16);
    MDSTEP(F3, b, c, d, a, in[14] + 0xfde5380c, 23);
    MDSTEP(F3, a, b, c, d, in[1] + 0xa4beea44, 4);
    MDSTEP(F3, d, a, b, c, in[4] + 0x4bdecfa9, 11);
    MDSTEP(F3, c, d, a, b, in[7] + 0xf6bb4b60, 16);
    MDSTEP(F3, b, c, d, a, in[10] + 0xbebfbc70, 23);
    MDSTEP(F3, a, b, c, d, in[13] + 0x289b7ec6, 4);
    MDSTEP(F3, d, a, b, c, in[0] + 0xeaa127fa, 11);
    MDSTEP(F3, c, d, a, b, in[3] + 0xd4ef3085, 16);
    MDSTEP(F3, b, c, d, a, in[6] + 0x04881d05, 23);
    MDSTEP(F3, a, b, c, d, in[9] + 0xd9d4d039, 4);
    MDSTEP(F3, d, a, b, c, in[12] + 0xe6db99e5, 11);
    MDSTEP(F3, c, d, a, b, in[15] + 0x1fa27cf8, 16);
    MDSTEP(F3, b, c, d, a, in[2] + 0xc4ac5665, 23);

    MDSTEP(F4, a, b, c, d, in[0] + 0xf4292244, 6);
    MDSTEP(F4, d, a, b, c, in[7] + 0x432aff97, 10);
    MDSTEP(F4, c, d, a, b, in[14] + 0xab9423a7, 15);
    MDSTEP(F4, b, c, d, a, in[5] + 0xfc93a039, 21);
    MDSTEP(F4, a, b, c, d, in[12] + 0x655b59c3, 6);
    MDSTEP(F4, d, a, b, c, in[3] + 0x8f0ccc92, 10);
    MDSTEP(F4, c, d, a, b, in[10] + 0xffeff47d, 15);
    MDSTEP(F4, b, c, d, a, in[1] + 0x85845dd1, 21);
    MDSTEP(F4, a, b, c, d, in[8] + 0x6fa87e4f, 6);
    MDSTEP(F4, d, a, b, c, in[15] + 0xfe2ce6e0, 10);
    MDSTEP(F4, c, d, a, b, in[6] + 0xa3014314, 15);
    MDSTEP(F4, b, c, d, a, in[13] + 0x4e0811a1, 21);
    MDSTEP(F4, a, b, c, d, in[4] + 0xf7537e82, 6);
    MDSTEP(F4, d, a, b, c, in[11] + 0xbd3af235, 10);
    MDSTEP(F4, c, d, a, b, in[2] + 0x2ad7d2bb, 15);
    MDSTEP(F4, b, c, d, a, in[9] + 0xeb86d391, 21);

    m_Buf[0] += a;
    m_Buf[1] += b;
    m_Buf[2] += c;
    m_Buf[3] += d;
}
