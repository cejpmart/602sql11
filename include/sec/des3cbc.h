// des3cbc.h

struct t_DES3CBC_keyIV
{ unsigned char des3key[CryptoPP::DES_EDE3_Encryption::KEYLENGTH];
  unsigned char IV[CryptoPP::DES_EDE3_Encryption::BLOCKSIZE];
};

class DES3CBCEncryption : public CryptoPP::DES_EDE3_Encryption
{ unsigned char cbc[CryptoPP::DES_EDE3_Encryption::BLOCKSIZE];
 public:
	enum { KEYLENGTH=sizeof(t_DES3CBC_keyIV) };

	DES3CBCEncryption(const t_DES3CBC_keyIV & keyIV) : CryptoPP::DES_EDE3_Encryption(keyIV.des3key)
    { memcpy(cbc, keyIV.IV, sizeof(cbc)); }
	void ProcessBlock(const byte *inBlock, byte * outBlock);
	void ProcessBlock(byte * inoutBlock) 
    { ProcessBlock(inoutBlock, inoutBlock); }
};

class DES3CBCDecryption : public CryptoPP::DES_EDE3_Decryption
{ unsigned char cbc[CryptoPP::DES_EDE3_Encryption::BLOCKSIZE];
 public:
	enum { KEYLENGTH=sizeof(t_DES3CBC_keyIV) };

	DES3CBCDecryption(const t_DES3CBC_keyIV & keyIV) : CryptoPP::DES_EDE3_Decryption(keyIV.des3key)
    { memcpy(cbc, keyIV.IV, sizeof(cbc)); }
	void ProcessBlock(const byte *inBlock, byte * outBlock);
	void ProcessBlock(byte * inoutBlock) 
    { ProcessBlock(inoutBlock, inoutBlock); }
};

