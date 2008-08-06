// ideaxcbc.h

struct t_IDEA_key
{ unsigned char key[16];
};

struct t_IDEAXCBC_key
{ unsigned char key[16];
  unsigned char s1[8];
  unsigned char s2[8];
};

struct t_IDEAXCBC_keyIV
{ t_IDEAXCBC_key key;
  unsigned char IV[8];
};
#if 0
class IDEAXCBC : public CryptoPP::IDEA
{protected: 
  bool encrypt;
 public:
	IDEAXCBC(const t_IDEAXCBC_keyIV & ikeyIV, CryptoPP::CipherDir dir);
	void ProcessBlock(byte * inoutBlock) 
    {IDEAXCBC::ProcessBlock(inoutBlock, inoutBlock);}
	void ProcessBlock(const byte *inBlock, byte * outBlock);
	enum { BLOCKSIZE=8 };
 private:
	void EnKey(const byte *);
	void DeKey();
  unsigned char s1[BLOCKSIZE], s2[BLOCKSIZE], cbc[BLOCKSIZE];
};

class IDEAXCBCEncryption : public IDEAXCBC
{public:
	IDEAXCBCEncryption(const t_IDEAXCBC_keyIV & ikeyIV)
		: IDEAXCBC (ikeyIV, CryptoPP::ENCRYPTION) { encrypt=true; }
};

class IDEAXCBCDecryption : public IDEAXCBC
{public:
	IDEAXCBCDecryption(const t_IDEAXCBC_keyIV & ikeyIV)
		: IDEAXCBC (ikeyIV, CryptoPP::DECRYPTION) { encrypt=false; }
};
#else

class IDEAXCBCEncryption : public CryptoPP::IDEAEncryption
{public:
	IDEAXCBCEncryption(const t_IDEAXCBC_keyIV & ikeyIV);
	void ProcessBlock(byte * inoutBlock) 
    {IDEAXCBCEncryption::ProcessBlock(inoutBlock, inoutBlock);}
	void ProcessBlock(const byte *inBlock, byte * outBlock);
	enum { BLOCKSIZE=8 };
 private:
	void EnKey(const byte *);
	void DeKey();
  unsigned char s1[BLOCKSIZE], s2[BLOCKSIZE], cbc[BLOCKSIZE];
};

class IDEAXCBCDecryption : public CryptoPP::IDEADecryption
{protected: 
  bool encrypt;
 public:
	IDEAXCBCDecryption(const t_IDEAXCBC_keyIV & ikeyIV);
	void ProcessBlock(byte * inoutBlock) 
    {IDEAXCBCDecryption::ProcessBlock(inoutBlock, inoutBlock);}
	void ProcessBlock(const byte *inBlock, byte * outBlock);
	enum { BLOCKSIZE=8 };
 private:
	void EnKey(const byte *);
	void DeKey();
  unsigned char s1[BLOCKSIZE], s2[BLOCKSIZE], cbc[BLOCKSIZE];
};


#endif

