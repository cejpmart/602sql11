// ideaxcbc.cpp - extension to IDEA
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

#if 0
IDEAXCBC::IDEAXCBC(const t_IDEAXCBC_keyIV & ikeyIV, CryptoPP::CipherDir dir) : IDEA(ikeyIV.key.key, dir)
{ 
  memcpy(s1, ikeyIV.key.s1, BLOCKSIZE);  memcpy(s2, ikeyIV.key.s2, BLOCKSIZE);  memcpy(cbc, ikeyIV.IV, BLOCKSIZE); 
}

void IDEAXCBC::ProcessBlock(const byte *inBlock, byte * outBlock)
{ unsigned char buf[BLOCKSIZE];
  if (encrypt)
  {// XOR in CBC mode:
    ((unsigned long *)buf)[0] = ((const unsigned long *)inBlock)[0] ^ ((unsigned long *)cbc)[0];
    ((unsigned long *)buf)[1] = ((const unsigned long *)inBlock)[1] ^ ((unsigned long *)cbc)[1];
   // XEX3:
    ((unsigned long *)buf)[0] = ((const unsigned long *)buf)[0] ^ ((unsigned long *)s1)[0];
    ((unsigned long *)buf)[1] = ((const unsigned long *)buf)[1] ^ ((unsigned long *)s1)[1];
    IDEA::ProcessBlock(buf, buf);
    ((unsigned long *)outBlock)[0] = ((const unsigned long *)buf)[0] ^ ((unsigned long *)s2)[0];
    ((unsigned long *)outBlock)[1] = ((const unsigned long *)buf)[1] ^ ((unsigned long *)s2)[1];
   // store new CBC:
    ((unsigned long *)cbc)[0] = ((unsigned long *)outBlock)[0];
    ((unsigned long *)cbc)[1] = ((unsigned long *)outBlock)[1];
  }
  else  // decrypt
  { unsigned char newcbc[BLOCKSIZE];
   // store new CBC:
    ((unsigned long *)newcbc)[0] = ((unsigned long *)inBlock)[0];
    ((unsigned long *)newcbc)[1] = ((unsigned long *)inBlock)[1];
   // XEX3 de:
    ((unsigned long *)buf)[0] = ((const unsigned long *)inBlock)[0] ^ ((unsigned long *)s2)[0];
    ((unsigned long *)buf)[1] = ((const unsigned long *)inBlock)[1] ^ ((unsigned long *)s2)[1];
    IDEA::ProcessBlock(buf, buf);
   // XEX3 de:
    ((unsigned long *)buf)[0] = ((const unsigned long *)buf)[0] ^ ((unsigned long *)s1)[0];
    ((unsigned long *)buf)[1] = ((const unsigned long *)buf)[1] ^ ((unsigned long *)s1)[1];
   // CBC de:
    ((unsigned long *)outBlock)[0] = ((const unsigned long *)buf)[0] ^ ((unsigned long *)cbc)[0];
    ((unsigned long *)outBlock)[1] = ((const unsigned long *)buf)[1] ^ ((unsigned long *)cbc)[1];
   // update CBC:
    ((unsigned long *)cbc)[0] = ((unsigned long *)newcbc)[0];
    ((unsigned long *)cbc)[1] = ((unsigned long *)newcbc)[1];
  }

}
#else

IDEAXCBCEncryption::IDEAXCBCEncryption(const t_IDEAXCBC_keyIV & ikeyIV) : CryptoPP::IDEAEncryption(ikeyIV.key.key)
{ 
  memcpy(s1, ikeyIV.key.s1, BLOCKSIZE);  memcpy(s2, ikeyIV.key.s2, BLOCKSIZE);  memcpy(cbc, ikeyIV.IV, BLOCKSIZE); 
}

IDEAXCBCDecryption::IDEAXCBCDecryption(const t_IDEAXCBC_keyIV & ikeyIV) : CryptoPP::IDEADecryption(ikeyIV.key.key)
{ 
  memcpy(s1, ikeyIV.key.s1, BLOCKSIZE);  memcpy(s2, ikeyIV.key.s2, BLOCKSIZE);  memcpy(cbc, ikeyIV.IV, BLOCKSIZE); 
}
void IDEAXCBCEncryption::ProcessBlock(const byte *inBlock, byte * outBlock)
{ unsigned char buf[BLOCKSIZE];
  {// XOR in CBC mode:
    ((unsigned long *)buf)[0] = ((const unsigned long *)inBlock)[0] ^ ((unsigned long *)cbc)[0];
    ((unsigned long *)buf)[1] = ((const unsigned long *)inBlock)[1] ^ ((unsigned long *)cbc)[1];
   // XEX3:
    ((unsigned long *)buf)[0] = ((const unsigned long *)buf)[0] ^ ((unsigned long *)s1)[0];
    ((unsigned long *)buf)[1] = ((const unsigned long *)buf)[1] ^ ((unsigned long *)s1)[1];
    CryptoPP::IDEAEncryption::ProcessBlock(buf, buf);
    ((unsigned long *)outBlock)[0] = ((const unsigned long *)buf)[0] ^ ((unsigned long *)s2)[0];
    ((unsigned long *)outBlock)[1] = ((const unsigned long *)buf)[1] ^ ((unsigned long *)s2)[1];
   // store new CBC:
    ((unsigned long *)cbc)[0] = ((unsigned long *)outBlock)[0];
    ((unsigned long *)cbc)[1] = ((unsigned long *)outBlock)[1];
  }
}

void IDEAXCBCDecryption::ProcessBlock(const byte *inBlock, byte * outBlock)
{ unsigned char buf[BLOCKSIZE];
  { unsigned char newcbc[BLOCKSIZE];
   // store new CBC:
    ((unsigned long *)newcbc)[0] = ((unsigned long *)inBlock)[0];
    ((unsigned long *)newcbc)[1] = ((unsigned long *)inBlock)[1];
   // XEX3 de:
    ((unsigned long *)buf)[0] = ((const unsigned long *)inBlock)[0] ^ ((unsigned long *)s2)[0];
    ((unsigned long *)buf)[1] = ((const unsigned long *)inBlock)[1] ^ ((unsigned long *)s2)[1];
    CryptoPP::IDEADecryption::ProcessBlock(buf, buf);
   // XEX3 de:
    ((unsigned long *)buf)[0] = ((const unsigned long *)buf)[0] ^ ((unsigned long *)s1)[0];
    ((unsigned long *)buf)[1] = ((const unsigned long *)buf)[1] ^ ((unsigned long *)s1)[1];
   // CBC de:
    ((unsigned long *)outBlock)[0] = ((const unsigned long *)buf)[0] ^ ((unsigned long *)cbc)[0];
    ((unsigned long *)outBlock)[1] = ((const unsigned long *)buf)[1] ^ ((unsigned long *)cbc)[1];
   // update CBC:
    ((unsigned long *)cbc)[0] = ((unsigned long *)newcbc)[0];
    ((unsigned long *)cbc)[1] = ((unsigned long *)newcbc)[1];
  }

}

#endif

