#ifndef _DES_H_
#define _DES_H_

#define MAX_PASSWORD 32
#define MASK28       0xFFFFFFF0L

class DESBLHL
{
protected:

    DWORD m_HL;

public:

    inline DESBLHL& operator=(DWORD nVal){m_HL = nVal; return(*this);}
    inline operator DWORD(){return(*(DWORD *)this);}
    inline DESBLHL& operator|=(DWORD nVal){m_HL |= nVal; return(*this);}
    inline DESBLHL& operator&=(DWORD nVal){m_HL &= nVal; return(*this);}
//    inline DWORD&   operator&(DWORD nVal){return(m_HL &= nVal);}
    inline DWORD Bit(char b){return(m_HL & (1 << (32 - b)));}
    inline void  Rot128(char pos){m_HL = ((m_HL << pos) | (m_HL >> (28 - pos))) & MASK28;}

    void   Perm32(DESBLHL in);
    void   Expand(char *a);
    DWORD  f(BYTE *k);

    friend class DESBL;
};

class DESKEY
{
public:

    BYTE m_bt[16][8];
};

class DESBL
{
protected:

    DESBLHL m_LO;
    DESBLHL m_HI;

    void KeyPerm(DESKEY *Key, char i);

public:

    inline DESBL& operator=(DWORD nVal){m_LO = nVal; m_HI = 0; return(*this);}
    inline DWORD KeyBit(char b){return(b > 28 ? (m_HI & (1 << (60 - b))) : (m_LO & (1 << (32 - b))));}
    inline DWORD Bit(char b){return(b > 32 ? (m_HI & (1 << (64 - b))) : (m_LO & (1 << (32 - b))));}
    inline void  Xor(DESBL Other){m_LO.m_HL ^= Other.m_LO.m_HL; m_HI.m_HL ^= Other.m_HI.m_HL;}
    //inline DWORD BitHI(char b){return(m_HI & (1 << (64 - b)));}
    inline void  SetLO(DWORD Mask){m_LO |= Mask;}
    inline void  SetHI(DWORD Mask){m_HI |= Mask;}
    inline void  Rot128(char i){m_LO.Rot128(i); m_HI.Rot128(i);}
    inline void  swap(){DWORD tmp = m_LO; m_LO = m_HI; m_HI = tmp;}
    inline void Mask56(){m_LO &= MASK28; m_HI &= MASK28;}
    inline void Round(BYTE *k)
    {
        DWORD t = (DWORD)m_LO ^ m_HI.f(k);
        m_LO    = m_HI;
        m_HI    = t;
    }
    
    void Perm64(DESBL in, char *perm);
    void KeyInit(DESKEY *Key);
};

void KeyInit(DESBL key);

class CDesEnv  
{
protected:

    DESKEY *m_Key;
    void Encrypt(DESBL *In);
    void Decrypt(DESBL *In);

public:

    CDesEnv(BYTE *Key){m_Key = (DESKEY *)Key;}
    void Encrypt(BYTE *b, DWORD sz);
    void Decrypt(BYTE *b, DWORD sz);
};

#endif  // _DES_H_
