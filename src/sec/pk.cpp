// pk.cpp
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
// PK structures:

NAMESPACE_BEGIN(CryptoPP)      

#ifdef STOP  // included in cryptoPP 5.2.1
template<> const byte PKCS_DigestDecoration<RIPEMD160>::decoration[] = {0x30,0x21,0x30,0x09,0x06,0x05,0x2B,0x24,0x03,0x02,0x1,0x05,0x00,0x04,0x14};
template<> const unsigned int PKCS_DigestDecoration<RIPEMD160>::length = sizeof(PKCS_DigestDecoration<RIPEMD160>::decoration);
#endif
template<> const byte PKCS_DigestDecoration<RIPEMD128>::decoration[] = {0x30,0x21,0x30,0x09,0x06,0x05,0x2B,0x24,0x03,0x02,0x2,0x05,0x00,0x04,0x10};
template<> const unsigned int PKCS_DigestDecoration<RIPEMD128>::length = sizeof(PKCS_DigestDecoration<RIPEMD128>::decoration);

NAMESPACE_END

t_asn_public_key::t_asn_public_key(void)
{ modulus = NULL;  publicExponent=NULL; }

t_asn_public_key::~t_asn_public_key(void)
{ if (modulus)      delete modulus;
  if (publicExponent) delete publicExponent;
}

unsigned t_asn_public_key::der_length(bool outer) const
{ unsigned total_length = modulus->der_length(true) + publicExponent->der_length(true);
  if (outer) total_length += tag_length(tag_universal_constructed_sequence) + length_of_length(total_length);
  return total_length;
}

void t_asn_public_key::save(tStream * str) const
{ str->save_tag(tag_universal_constructed_sequence);
  str->save_length(der_length(false));  // inner length
 // save fields:
  modulus->save(str);
  publicExponent->save(str);
}

void t_asn_public_key::load(tStream * str)
{ if (!str->check_tag(tag_universal_constructed_sequence))
    { str->set_struct_error(ERR_SEQUENCE_NOT_PRESENT);  return; }
  str->load_length();  // ignored
 // load fields:
  modulus = new t_asn_integer_indirect;  if (!modulus) { str->set_struct_error(ERR_NO_MEMORY);  return; }
  modulus->load(str);  if (str->any_error()) return;

  publicExponent = new t_asn_integer_indirect;  if (!publicExponent) { str->set_struct_error(ERR_NO_MEMORY);  return; }
  publicExponent->load(str);
}

t_asn_public_key * t_asn_public_key::copy(void) const
{ t_asn_public_key * pk = new t_asn_public_key;
  if (pk)
  { pk->modulus        = (t_asn_integer_indirect *)modulus->copy();
    pk->publicExponent = (t_asn_integer_indirect *)publicExponent->copy();
    if (pk->modulus && pk->publicExponent)
      return pk;
    delete pk;
  }
  return NULL;
}

#ifdef PK_BITSTRING
bool t_asn_public_key::verify_signature(const unsigned char * signature, unsigned sign_length, const unsigned char * auth_data, unsigned auth_data_length)
#else
bool t_asn_public_key::verify_signature(const t_asn_integer_indirect * signature)
#endif
{// convert public key and signature value into Integers:
  CryptoPP::Integer modul, pubexp;
  MyInteger2Integer(modulus, modul);
  MyInteger2Integer(publicExponent, pubexp);
  CryptoPP::Integer signat(signature, sign_length, CryptoPP::Integer::UNSIGNED); 

 // decode the signature by the public key and unpad:
  CryptoPP::RSAFunction rsafnc;  rsafnc.Initialize(modul, pubexp);
#ifdef STOP // test, to skutecne vraci padded seq obsahujici hast a alg.
  CryptoPP::Integer decoded = rsafnc.ApplyFunction(signat);
#endif    
  int PaddedBlockBitLength = rsafnc.MaxImage().BitCount()-1;
	CryptoPP::SecByteBlock paddedBlock(CryptoPP::bitsToBytes(PaddedBlockBitLength));
  rsafnc.ApplyFunction(signat).Encode(paddedBlock, paddedBlock.size());
  CryptoPP::PKCS_SignaturePaddingScheme pad;
	CryptoPP::SecByteBlock recoveredDecoratedDigest(pad.MaxUnpaddedLength(PaddedBlockBitLength));
  const unsigned char * rdd = recoveredDecoratedDigest;
	unsigned recoveredDecoratedDigestLen = pad.Unpad(paddedBlock, PaddedBlockBitLength, recoveredDecoratedDigest);
 // undecorate and compare to the real digest:
  unsigned char digest[MAX_DIGEST_LENGTH];
  if (!memcmp(rdd, CryptoPP::PKCS_DigestDecoration<CryptoPP::RIPEMD160>::decoration, CryptoPP::PKCS_DigestDecoration<CryptoPP::RIPEMD160>::length))
  { unsigned decorlen = CryptoPP::PKCS_DigestDecoration<CryptoPP::RIPEMD160>::length;
   // compute the real digest and compare:
    CryptoPP::RIPEMD160 ripemd160;
    ripemd160.Update(auth_data, auth_data_length);
	  ripemd160.Final(digest);
    return ripemd160.DigestSize() == recoveredDecoratedDigestLen-decorlen && !memcmp(rdd+decorlen, digest, recoveredDecoratedDigestLen-decorlen);
  }
  if (!memcmp(recoveredDecoratedDigest, CryptoPP::PKCS_DigestDecoration<CryptoPP::RIPEMD128>::decoration, CryptoPP::PKCS_DigestDecoration<CryptoPP::RIPEMD128>::length))
  { unsigned decorlen = CryptoPP::PKCS_DigestDecoration<CryptoPP::RIPEMD128>::length;
   // compute the real digest and compare:
    CryptoPP::RIPEMD128 ripemd128;
    ripemd128.Update(auth_data, auth_data_length);
	  ripemd128.Final(digest);
    return ripemd128.DigestSize() == recoveredDecoratedDigestLen-decorlen && !memcmp(recoveredDecoratedDigest+decorlen, digest, recoveredDecoratedDigestLen-decorlen);
  }
  if (!memcmp(recoveredDecoratedDigest, CryptoPP::PKCS_DigestDecoration<CryptoPP::SHA      >::decoration, CryptoPP::PKCS_DigestDecoration<CryptoPP::SHA      >::length))
  { unsigned decorlen = CryptoPP::PKCS_DigestDecoration<CryptoPP::SHA>::length;
   // compute the real digest and compare:
    CryptoPP::SHA sha;
    sha      .Update(auth_data, auth_data_length);
	  sha      .Final(digest);
    return sha      .DigestSize() == recoveredDecoratedDigestLen-decorlen && !memcmp(recoveredDecoratedDigest+decorlen, digest, recoveredDecoratedDigestLen-decorlen);
  }
  return false;
}
///////////////////////////////////////////// private key ////////////////////////////////////////////////////////

t_asn_private_key::t_asn_private_key(void)
{ modulus = NULL;  publicExponent=NULL;  d=p=q=dp=dq=u=NULL; }

t_asn_private_key::~t_asn_private_key(void)
{ if (modulus)        delete modulus;
  if (publicExponent) delete publicExponent;
  if (d)  delete d;   if (p)  delete p;   if (q) delete q;
  if (dp) delete dp;  if (dq) delete dq;  if (u) delete u;
}

unsigned t_asn_private_key::der_length(bool outer) const
{ unsigned total_length = modulus->der_length(true) + publicExponent->der_length(true) +
     d->der_length(true) + p->der_length(true) + q->der_length(true) + dp->der_length(true) + dq->der_length(true) + u->der_length(true);
  if (outer) total_length += tag_length(tag_universal_constructed_sequence) + length_of_length(total_length);
  return total_length;
}

void t_asn_private_key::save(tStream * str) const
{ str->save_tag(tag_universal_constructed_sequence);
  str->save_length(der_length(false));  // inner length
 // save fields:
  modulus->save(str);
  publicExponent->save(str);
  d->save(str); p->save(str);  q->save(str);  dp->save(str);  dq->save(str);  u->save(str);
}

void t_asn_private_key::load(tStream * str)
{ if (!str->check_tag(tag_universal_constructed_sequence))
    { str->set_struct_error(ERR_SEQUENCE_NOT_PRESENT);  return; }
  str->load_length();  // ignored
 // load fields:
  modulus = new t_asn_integer_indirect;  if (!modulus) { str->set_struct_error(ERR_NO_MEMORY);  return; }
  modulus->load(str);  if (str->any_error()) return;

 // private keys generated by openSSL have integer 0 on its start - skip it!
  if (modulus->length==1 && modulus->value[0]==0)  
  { delete modulus;
    modulus = new t_asn_integer_indirect;  if (!modulus) { str->set_struct_error(ERR_NO_MEMORY);  return; }
    modulus->load(str);  if (str->any_error()) return;
  }
 // load the other fields:
  publicExponent = new t_asn_integer_indirect;  if (!publicExponent) { str->set_struct_error(ERR_NO_MEMORY);  return; }
  publicExponent->load(str);  if (str->any_error()) return;

  d  = new t_asn_integer_indirect;  if (!d) { str->set_struct_error(ERR_NO_MEMORY);  return; }
  d  ->load(str);  if (str->any_error()) return;
  p  = new t_asn_integer_indirect;  if (!p) { str->set_struct_error(ERR_NO_MEMORY);  return; }
  p  ->load(str);  if (str->any_error()) return;
  q  = new t_asn_integer_indirect;  if (!q) { str->set_struct_error(ERR_NO_MEMORY);  return; }
  q  ->load(str);  if (str->any_error()) return;
  dp = new t_asn_integer_indirect;  if (!dp) { str->set_struct_error(ERR_NO_MEMORY);  return; }
  dp ->load(str);  if (str->any_error()) return;
  dq = new t_asn_integer_indirect;  if (!dq) { str->set_struct_error(ERR_NO_MEMORY);  return; }
  dq ->load(str);  if (str->any_error()) return;
  u  = new t_asn_integer_indirect;  if (!u) { str->set_struct_error(ERR_NO_MEMORY);  return; }
  u  ->load(str);  if (str->any_error()) return;
}

t_asn_private_key::t_asn_private_key(CryptoPP::InvertibleRSAFunction * priv)
{ modulus = NULL;  publicExponent=NULL;  d=p=q=dp=dq=u=NULL; 
  modulus=new t_asn_integer_indirect(priv->GetModulus());
  publicExponent=new t_asn_integer_indirect(priv->GetPublicExponent());
  d=new t_asn_integer_indirect(priv->GetPrivateExponent());
  p=new t_asn_integer_indirect(priv->GetPrime1());
  q=new t_asn_integer_indirect(priv->GetPrime2());
  dp=new t_asn_integer_indirect(priv->GetModPrime1PrivateExponent());
  dq=new t_asn_integer_indirect(priv->GetModPrime2PrivateExponent());
  u=new t_asn_integer_indirect(priv->GetMultiplicativeInverseOfPrime2ModPrime1());
}

//////////////////////////////////// t_asn_SubjectPublicKeyInfo //////////////////////////////////////////////////
t_asn_SubjectPublicKeyInfo::t_asn_SubjectPublicKeyInfo(void)
{ algorithm=NULL;  subjectPublicKey=NULL; }

t_asn_SubjectPublicKeyInfo::t_asn_SubjectPublicKeyInfo(const unsigned * object_id, unsigned object_id_length, t_asn_public_key * subjectPublicKeyIn)
{ algorithm        = new t_asn_algorithm_identifier(object_id, object_id_length);  
  subjectPublicKey = subjectPublicKeyIn;
}

t_asn_SubjectPublicKeyInfo::~t_asn_SubjectPublicKeyInfo(void)
  { if (algorithm) delete algorithm;  if (subjectPublicKey) delete subjectPublicKey; }

unsigned t_asn_SubjectPublicKeyInfo::der_length(bool outer) const
{ unsigned keylength = subjectPublicKey->der_length(true);
#ifndef PK_BITSTRING2  // add bit_string envelope
  keylength += tag_length(tag_universal_bit_string) + length_of_length(keylength) + 1; // +1: space for the number of unused bits in the bit string
#endif
  unsigned total_length = algorithm->der_length(true) + keylength;
  if (outer) total_length += tag_length(tag_universal_constructed_sequence) + length_of_length(total_length);
  return total_length;
}

void t_asn_SubjectPublicKeyInfo::save(tStream * str) const
{ str->save_tag(tag_universal_constructed_sequence);
  str->save_length(der_length(false));  // inner length
 // save fields:
  algorithm->save(str);
#ifndef PK_BITSTRING2
  str->save_tag(tag_universal_bit_string);
  str->save_length(1+subjectPublicKey->der_length(true));
  str->save_dir(0, 1);  // number of unused padding bits
#endif
  subjectPublicKey->save(str);
}

void t_asn_SubjectPublicKeyInfo::load(tStream * str) 
{ if (!str->check_tag(tag_universal_constructed_sequence))
    { str->set_struct_error(ERR_SEQUENCE_NOT_PRESENT);  return; }
  str->load_length();  // ignored
 // load fields:
  algorithm = new t_asn_algorithm_identifier;
  if (!algorithm) { str->set_struct_error(ERR_NO_MEMORY);  return; }
  algorithm->load(str);
  if (str->any_error()) return;

#ifdef PK_BITSTRING2
  subjectPublicKey = new t_asn_bit_string;
#else
  subjectPublicKey = new t_asn_public_key;
  if (!str->check_tag(tag_universal_bit_string)) { str->set_struct_error(ERR_BIT_STRING_NOT_PRESENT);  return; }
  /*unsigned bit_length = -- not used */ str->load_length();
  str->load_val(NULL, 1);  // number of padding bits
#endif
  if (!subjectPublicKey) { str->set_struct_error(ERR_NO_MEMORY);  return; }
  subjectPublicKey->load(str);
}

t_asn_SubjectPublicKeyInfo * t_asn_SubjectPublicKeyInfo::copy(void) const
{ t_asn_SubjectPublicKeyInfo * pki = new t_asn_SubjectPublicKeyInfo;
  if (pki)
  { pki->algorithm = algorithm->copy();
    pki->subjectPublicKey = subjectPublicKey->copy();
    if (pki->algorithm && pki->subjectPublicKey) 
      return pki;
    delete pki;
  }
  return NULL;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////

//typedef DigestVerifierTemplate<PKCS_SignaturePaddingScheme, RSAFunction> xx;

void vefi()
{ 

}
// moved from the old CryptoPP:
NAMESPACE_BEGIN(CryptoPP)

unsigned int PKCS_SignaturePaddingScheme::MaxUnpaddedLength(unsigned int paddedLength) const
{
	return paddedLength/8 > 10 ? paddedLength/8-10 : 0;
}

void PKCS_SignaturePaddingScheme::Pad(RandomNumberGenerator &, const byte *input, unsigned int inputLen, byte *pkcsBlock, unsigned int pkcsBlockLen) const
{
	assert (inputLen <= MaxUnpaddedLength(pkcsBlockLen));

	// convert from bit length to byte length
	if (pkcsBlockLen % 8 != 0)
	{
		pkcsBlock[0] = 0;
		pkcsBlock++;
	}
	pkcsBlockLen /= 8;

	pkcsBlock[0] = 1;   // block type 1

	// padd with 0xff
	memset(pkcsBlock+1, 0xff, pkcsBlockLen-inputLen-2);

	pkcsBlock[pkcsBlockLen-inputLen-1] = 0;               // separator
	memcpy(pkcsBlock+pkcsBlockLen-inputLen, input, inputLen);
}

unsigned int PKCS_SignaturePaddingScheme::Unpad(const byte *pkcsBlock, unsigned int pkcsBlockLen, byte *output) const
{
	unsigned int maxOutputLen = MaxUnpaddedLength(pkcsBlockLen);

	// convert from bit length to byte length
	if (pkcsBlockLen % 8 != 0)
	{
		if (pkcsBlock[0] != 0)
			return 0;
		pkcsBlock++;
	}
	pkcsBlockLen /= 8;

	// Require block type 1.
	if (pkcsBlock[0] != 1)
		return 0;

	// skip past the padding until we find the seperator
	unsigned i=1;
	while (i<pkcsBlockLen && pkcsBlock[i++])
		if (pkcsBlock[i-1] != 0xff)     // not valid padding
			return 0;
	assert(i==pkcsBlockLen || pkcsBlock[i-1]==0);

	unsigned int outputLen = pkcsBlockLen - i;
	if (outputLen > maxOutputLen)
		return 0;

	memcpy (output, pkcsBlock+i, outputLen);
	return outputLen;
}
NAMESPACE_END

