#ifndef _EDMD5_H_
#define _EDMD5_H_

void EDAddDigest(HMDCTX hMD, LPBYTE buf, ULONG len);
int  EDGetDigest(HMDCTX hMD, LPBYTE buf, ULONG len, LPBYTE Digest);

#pragma pack(4)
class CMDCtx 
{
public:

	unsigned m_Buf[4];
	unsigned m_Bytes;
	BYTE  m_In[64];

    CMDCtx();

protected:

    void Update(LPBYTE buf, ULONG len);
    void Final(LPBYTE digest);
    void Transform(BYTE *in);

    friend void EDAddDigest(HMDCTX hMD, LPBYTE buf, ULONG len);
    friend int  EDGetDigest(HMDCTX hMD, LPBYTE buf, ULONG len, LPBYTE Digest);
};

#endif // _EDMD5_H_
