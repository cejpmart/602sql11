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
#include "string.h"
#include "ed.h"
#include "eddesprm.h"
#include "eddes.h"
#include "general.h"

#if defined(WINS) && !defined(X64ARCH)
#define _ED_ASM_
#pragma warning(disable:4996)  // using insecure functions like strcpy, strcat,... (#define _CRT_SECURE_NO_DEPRECATE)
#endif

//
// Key - Buffer for the key, min 128 bytes
//
extern "C" __declspec(dllexport) void EDSlowInit(char *Password, BYTE *Key)
{
    char pw[MAX_PASSWORD];
    strncpy(pw, Password, MAX_PASSWORD);
    int sz = (int)strlen(pw);
    int ps = (sz - 1) / 8;
    while (sz % 8)
        pw[sz++] = 0;

    DESBL *db = (DESBL *)pw;
    db->KeyInit((DESKEY *)Key);
    if (ps)
    {
        CDesEnv de(Key);
        de.Encrypt((BYTE *)pw + 8, ps * 8);
        (db + ps)->KeyInit((DESKEY *)Key);
    }
}
//
// sz - Length of the encryped message, rounded to the multiply of 8 bytes
//
extern "C" __declspec(dllexport) BOOL EDSlowEncrypt(BYTE *b, DWORD sz, BYTE *Key)
{
    if (sz % 8)
        return(TRUE);

    CDesEnv de(Key);
    de.Encrypt(b, sz);
    return(FALSE);
}

extern "C" __declspec(dllexport) BOOL EDSlowDecrypt(BYTE *b, DWORD sz, BYTE *Key)
{
    if (sz % 8)
        return(TRUE);

    CDesEnv de(Key);
    de.Decrypt(b, sz);
    return(FALSE);
}

DWORD DESBLHL :: f(BYTE *k)
{
    DESBLHL b, out;
    b = 0;
    out = 0;
    char a[8];
    register DWORD s;
    register DWORD rc;
    register short int j;
    
    Expand(a);
    for (j = 0; j < 8; j++)
    {
        rc = a[j] ^ k[j];
        rc = (rc & 0x20) | ((rc << 4) & 0x10) | ((rc >> 1) & 0x0F);
        s  = S[j][rc];
        b  = (b << 4) | s;
    }
    out.Perm32(b);
    return(out.m_HL);
}

#define DESBIT1 0x80000000L

void DESBLHL :: Perm32(DESBLHL inb)
{
#ifndef _ED_ASM_

    DWORD DestMask;

    m_HL = 0;
    char *BitPos = P;
    for (DestMask = DESBIT1; DestMask; DestMask >>= 1)
    {
        char  Shift  = *BitPos++ - 1;
        if ((inb << Shift) & DESBIT1)
            m_HL |= DestMask;
    }

#else

    DESBLHL *ths = this;
    _asm
    {
        mov  ecx,ths
        push ecx
        lea  esi,P
        mov  edi,DWORD PTR inb
        xor  ebx,ebx
        mov  edx,DESBIT1
        mov  ch,32
    l0:
        mov  cl,[esi]
        inc  esi
        dec  cl
        mov  eax,edi
        shl  eax,cl
        or   eax,eax
        jns  l1
        or   ebx,edx
    l1:
        shr  edx,1
        dec  ch
        jnz  l0
        pop  ecx
        mov  [ecx],ebx
    }

#endif // _ED_ASM_
}

//#define MASK6 0x3F3F3F3F
void DESBLHL :: Expand(char *a)
{
#ifndef _ED_ASM_

        *a++ = (char)(((m_HL << 5) & 0xE0) | ((m_HL >> 27) & 0x1F));
        *a++ = (char)(m_HL >> 23);
        *a++ = (char)(m_HL >> 19);
        *a++ = (char)(m_HL >> 15);
        *a++ = (char)(m_HL >> 11);
        *a++ = (char)(m_HL >> 07);
        *a++ = (char)(m_HL >> 03);
        *a++ = (char)(((m_HL << 1) & 0xFE) | ((m_HL >> 31) & 0x01));

#else

    DESBLHL *ths = this;
    _asm
    {
        mov     ecx,ths
        mov     edi,a
        mov     eax,[ecx]
        rol     eax,5
        mov     [edi],al
        inc     edi
        rol     eax,4
        mov     [edi],al
        inc     edi
        rol     eax,4
        mov     [edi],al
        inc     edi
        rol     eax,4
        mov     [edi],al
        inc     edi
        rol     eax,4
        mov     [edi],al
        inc     edi
        rol     eax,4
        mov     [edi],al
        inc     edi
        rol     eax,4
        mov     [edi],al
        inc     edi
        rol     eax,4
        mov     [edi],al
        //sub     edi,7
        //and     [edi],MASK6
        //add     edi,4 
        //and     [edi],MASK6
    }
#endif // _ED_ASM_
}

void DESBL :: Perm64(DESBL inb, char *perm)
{
#ifndef _ED_ASM_

    DWORD DestMask;

    for (DESBLHL *hl = &m_LO; hl <= &m_HI; hl++)
    {
        *hl = 0;
        for (DestMask = 0x80000000; DestMask; DestMask >>= 1)
        {
            BYTE  Shift = *perm++ - 1;
            DWORD Src   = Shift < 32 ? inb.m_LO : inb.m_HI;
            if ((Src << Shift) & 0x80000000)
                *hl |= DestMask;
        }
    }

#else

    WORD hl = 2;
    DESBL *ths = this;

    _asm
    {
        mov   edi,ths
        mov   esi,perm
        mov   edx,0x80000000

    bb:
        xor   ebx,ebx
        mov   ch,32
    l0:
        mov   cl,[esi]
        inc   esi
        dec   cl
        mov   eax,DWORD PTR inb
        cmp   cl,32
        jb    l1
        mov   eax,DWORD PTR inb + 4
    l1:
        shl   eax,cl
        or    eax,eax
        jns   l2
        or    ebx,edx
    l2:
        ror   edx,1
        dec   ch
        jnz   l0

        mov   [edi],ebx
        add   edi,4
        dec   hl
        jnz   bb
    }
#endif // _ED_ASM_
}

void DESBL :: KeyInit(DESKEY *Key)
{
    DESBL cd;

    cd.Perm64(*this, PC1);
    cd.Mask56();

    for (int i = 0; i < 16; i++)
    {
        cd.Rot128(keyrot[i]);
        cd.KeyPerm(Key, i);
    }
}

#define KEYBIT1 0x20

void DESBL :: KeyPerm(DESKEY *Key, char i)
{
    int  j, k;
    char mask;
    char *perm = PC2;
    
    for (j = 0; j < 4; j++)
    {
        Key->m_bt[i][j] = 0;
        mask = KEYBIT1;
        for (k = 0; k < 6; k++)
        {
            if (KeyBit(*perm++))
                Key->m_bt[i][j] |= mask;
            mask >>=1;
        }
    }
    for (j = 4; j < 8; j++)
    {
        Key->m_bt[i][j] = 0;
        mask = KEYBIT1;
        for (k = 0; k < 6; k++)
        {
            if (KeyBit(*perm++))
                Key->m_bt[i][j] |= mask;
            mask >>= 1;
        }
    }
}

//////////////////////////////////////////////////////////////////////
// CDesEnv Class
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

void CDesEnv :: Encrypt(BYTE *b, DWORD sz)
{
    DESBL *pb = (DESBL *)b;
    DESBL *pe = pb + sz / 8;
    
    Encrypt(pb++);
    while (pb < pe)
    {
        pb->Xor(*(pb - 1));
        Encrypt(pb++);
    }
}

void CDesEnv :: Encrypt(DESBL *In)
{
    DESBL work;
    work.Perm64(*In, IP);

    for (int i = 0; i <= 15; i++)
        work.Round(m_Key->m_bt[i]);
    work.swap();

    In->Perm64(work, FP);
}

void CDesEnv :: Decrypt(BYTE *b, DWORD sz)
{
    DESBL *pb = (DESBL *)b;
    DESBL *pe = pb + sz / 8;
    DESBL mo, mn;
    
    mn = *pb;
    Decrypt(pb++);
    while (pb < pe)
    {
        mo = mn;
        mn = *pb;
        Decrypt(pb);
        pb->Xor(mo);
        pb++;
    }
}

void CDesEnv :: Decrypt(DESBL *In)
{
    DESBL work;
 
    work.Perm64(*In, IP);
    for (int i = 15; i >= 0; i--)
        work.Round(m_Key->m_bt[i]);
    work.swap();
    In->Perm64(work, FP);
}

