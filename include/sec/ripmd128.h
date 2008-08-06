#ifndef CRYPTOPP_RIPEMD128_H
#define CRYPTOPP_RIPEMD128_H

NAMESPACE_BEGIN(CryptoPP)

#ifdef STOP

class RIPEMD128 : public IteratedHash<word32>
{
public:
	RIPEMD128();
	void Final(byte *hash);
	unsigned int DigestSize() const {return DIGESTSIZE;}

	static void CorrectEndianess(word32 *out, const word32 *in, unsigned int byteCount)
	{
#ifndef IS_LITTLE_ENDIAN
		byteReverse(out, in, byteCount);
#else
		if (in!=out)
			memcpy(out, in, byteCount);
#endif
	}

	static void Transform(word32 *digest, const word32 *data);

	enum {DIGESTSIZE = 16, DATASIZE = 64};

private:
	void Init();
	void HashBlock(const word32 *input);
};

#endif
NAMESPACE_END
#endif
