// cmem.h
#pragma warning(disable:4996)  // disables the deprecation warnings for itoa, sscanf, strcpy, strcat,...
#define PK_BITSTRING 1
#define CNB 1
#define CRYPTOPP_MAINTAIN_BACKWARDS_COMPATIBILITY 1

//#define OVERLAPPED_IO

#include "crypto/cryptlib.h"
#include "crypto/sha.h"
#include "crypto/ripemd.h"
//#include "crypto\iterhash.h"
//#include "crypto\pkcspad.h"
#include "crypto/rsa.h"
#include "crypto/idea.h"
#include "crypto/des.h"
#include "crypto/integer.h"
//#include "crypto\misc.h"
#include "crypto/pubkey.h"
#include "crypto/rng.h"
#include "crypto/randpool.h"
#include "ripmd128.h"
#include "ideaxcbc.h"
//#include "des3cbc.h"

NAMESPACE_BEGIN(CryptoPP)
typedef HashTransformation HashModule;
inline unsigned int bitsToBytes(unsigned int bitCount)
{
	return ((bitCount+7)/(8));
}
NAMESPACE_END

class temp_rand_pool
{ CryptoPP::RandomPool randPool;
 public:
  temp_rand_pool(byte * seed, int seed_len)
  { randPool.Put(seed, seed_len); }
  temp_rand_pool(void)
  { byte seed[2];  seed[0]=23;  seed[1]=56;
    randPool.Put(seed, sizeof(seed)); 
  }
  operator CryptoPP::RandomPool & ()
  { return randPool; }
};

#ifdef WINS  // for other platforms defined in winrepl.h
typedef HANDLE FHANDLE;   // file handle
#define INVALID_FHANDLE_VALUE  INVALID_HANDLE_VALUE
#endif
#define FILE_HANDLE_VALID(h) ((h)!=INVALID_FHANDLE_VALUE)  // argument should be of FHANDLE type

#define MAX_DIGEST_LENGTH 20 // in bytes
#define MAX_DECOR_LENGTH  16 // 15 in fact

#define LENGTH_INDEFINITE 0xffffffff

unsigned tag_length(const unsigned char * tag);
unsigned length_of_length(unsigned length);

class tStream;

void stream_copy(tStream * source, tStream * dest, unsigned length, CryptoPP::HashModule * md);
bool stream_copy_decrypt(tStream * source, tStream * dest, int length, CryptoPP::HashModule * md, IDEAXCBCDecryption & ideadec);
//bool stream_copy_decrypt(tStream * source, tStream * dest, int length, CryptoPP::HashModule * md, DES3CBCDecryption & ideadec);
bool stream_copy_decrypt(tStream * source, tStream * dest, int length, CryptoPP::HashModule * md, CryptoPP::IDEADecryption & ideadec);

struct t_bookmark
{ unsigned length;
  unsigned start_pos;
  t_bookmark(tStream * str);
};
////////////////////////////////////// object ids //////////////////////////////////////////////////
extern const unsigned attr_type_country_name    [];
extern const unsigned attr_type_organiz_name    [];
extern const unsigned attr_type_org_unit_name   [];
extern const unsigned attr_type_common_unit_name[];
extern const unsigned attr_type_given_name      [];
extern const unsigned attr_type_initials        [];
extern const unsigned attr_type_surname         [];
extern const unsigned attr_type_domain_comp_name[];
extern const unsigned attr_type_email_name[];
#define attr_type_length    4
#define attr_type_length_DC 7
#define attr_type_length_email 7

struct t_name_part
{ const unsigned * object_id; 
  unsigned object_id_length;
  const char * value;
};

class t_asn_Name;
t_asn_Name * create_name_list(const t_name_part * parts);
///////////////////////////////////////////// tags /////////////////////////////////////////////////////
extern const unsigned char tag_universal_boolean[];
extern const unsigned char tag_universal_integer[];
extern const unsigned char tag_universal_objectid[];
extern const unsigned char tag_universal_printable_string[];
extern const unsigned char tag_universal_ia5_string[];
extern const unsigned char tag_universal_octet_string[];
extern const unsigned char tag_universal_bit_string[];
extern const unsigned char tag_universal_null[];
extern const unsigned char tag_universal_utctime[];
extern const unsigned char tag_universal_strange_string[];
extern const unsigned char tag_universal_utctime2[];

extern const unsigned char tag_universal_constructed_sequence[];
extern const unsigned char tag_universal_constructed_set[];

extern const unsigned char tag_kontext_primitive_0[];
extern const unsigned char tag_kontext_primitive_1[];
extern const unsigned char tag_kontext_primitive_2[];
extern const unsigned char tag_kontext_constructed_0[];
extern const unsigned char tag_kontext_constructed_1[];
extern const unsigned char tag_kontext_constructed_3[];
extern const unsigned char tag_kontext_constructed_4[];

enum t_asn_error { ERR_NO=0, ERR_NO_MEMORY, ERR_LENGHT_TOO_BIG,
    ERR_DIRECT_INTEGER_LENGHT,      ERR_OBJECT_ID_LENGHT,      ERR_PRINTABLE_STRING_LENGHT,      ERR_BIT_STRING_LENGHT,      ERR_IA5_STRING_LENGHT,
    ERR_NULL_LENGHT,                ERR_UTCTIME_LENGHT,        ERR_OCTET_STRING_LENGHT,          ERR_INDIRECT_INTEGER_LENGHT,      
    ERR_DIRECT_INTEGER_NOT_PRESENT, ERR_OBJECT_ID_NOT_PRESENT, ERR_PRINTABLE_STRING_NOT_PRESENT, ERR_BIT_STRING_NOT_PRESENT, ERR_IA5_STRING_NOT_PRESENT,
    ERR_NULL_NOT_PRESENT,           ERR_UTCTIME_NOT_PRESENT,   ERR_OCTET_STRING_NOT_PRESENT,     ERR_INDIRECT_INTEGER_NOT_PRESENT, 
    ERR_BOOLEAN_NOT_PRESENT,        ERR_BOOLEAN_LENGHT,
    ERR_SEQUENCE_NOT_PRESENT,       ERR_SET_NOT_PRESENT,
    ERR_UNSUPPORTED_ATTRIBUTE,      ERR_UNSUPPORTED,           ERR_MISSING_EOC,                  ERR_BAD_ALGORITHM,
    ERR_WRONG_CONTENT_TYPE,         ERR_ECNRYPT_KET_NOT_ACTIVE,ERR_CANNOT_DECRYPT,               
    ERR_FILE /* general file error */ };

enum t_VerifyResult { VR_NOT_SIGNED, VR_OK, VR_MISSING_CERTIF, VR_CERTIF_TAMPERED, VR_TAMPERED, VR_TAMPERED_AA, VR_BAD_CERTIF_TYPE, 
                      VR_EXPIRED, VR_STRUCT_ERROR, VR_CANNOT_DECRYPT_SIGNATURE, VR_MISSING_CRL, VR_REVOKED };

extern const unsigned char keyUsageSign    [];
extern const unsigned char keyUsageSignEnc [];
extern const unsigned char keyUsageEncrypt [];
extern const unsigned char keyUsageCA      [];
extern const unsigned char keyUsageCA2     [];
extern const unsigned char keyUsageCA3     [];
#define keyUsageOctetLength 4

extern const unsigned exten_id_AutKeyId [];
extern const unsigned exten_id_BasConstr[];
extern const unsigned exten_id_KeyUsage [];
#define exten_id_length 4

extern const unsigned attr_id_content_type  [];
#define attr_id_content_type_length   7
extern const unsigned attr_id_message_digest[];
#define attr_id_message_digest_length 7
extern const unsigned attr_id_signing_time  [];
#define attr_id_signing_time_length   7
extern const unsigned attr_id_account_date  [];
#define attr_id_account_date_length   10

extern const char cri_attribute_signature[];
extern const char cri_attribute_encryption[];
extern const char cri_attribute_CA[];
extern const char cri_attribute_RCA[];

extern const unsigned        data_content_object_id[];
#define        data_content_object_id_length 7
extern const unsigned signed_data_content_object_id[];
#define signed_data_content_object_id_length 7
extern const unsigned sigenv_data_content_object_id[];
#define sigenv_data_content_object_id_length 7
///////////////////////////////////////////// IDs ////////////////////////////////////////////////////////
//const unsigned cri_attr_type_unstructure_name   [];
//#define cri_attr_type_unstructure_name_length    8
extern const unsigned cri_attr_type_unstructured_name   [];
#define cri_attr_type_unstructured_name_length    7
extern const unsigned cri_attr_type_extension_req       [];
#define cri_attr_type_extension_req_length        7
extern const unsigned algorithm_id_rsaencryption       [];
#define algorithm_id_rsaencryption_length        7
extern const unsigned algorithm_id_rsaSigWithMD160[];
#define algorithm_id_rsaSigWithMD160_length   7
extern const unsigned algorithm_id_rsaSigWithMD128[];
#define algorithm_id_rsaSigWithMD128_length   7
extern const unsigned algorithm_id_rsaSigWithSHA[];
#define algorithm_id_rsaSigWithSHA_length   7
extern const unsigned algorithm_id_ripemd160[];
#define algorithm_id_ripemd160_length         6
extern const unsigned algorithm_id_ripemd128[];
#define algorithm_id_ripemd128_length         6
extern const unsigned algorithm_id_sha      [];
#define algorithm_id_sha_length               6
extern const unsigned algorithm_id_ideaxex3[];
#define algorithm_id_ideaxex3_length         10
extern const unsigned algorithm_id_des3cbc[];
#define algorithm_id_des3cbc_length           6

enum t_algors { ALG_UNDEF, ALG_RIPEMD160, ALG_RIPEMD128, ALG_SHA, ALG_RSARIPEMD160, ALG_RSARIPEMD128, ALG_RSASHA, ALG_IDEAXEX, ALG_DES3CBC };
enum t_keytypes { KEYTYPE_UNDEF, KEYTYPE_ENCRYPT, KEYTYPE_SIGN, KEYTYPE_CERTIFY, KEYTYPE_RCA, KEYTYPE_ENCSIG };

///////////////////////////////////////////// tStream //////////////////////////////////////////////////////
#ifdef OVERLAPPED_IO
BOOL xwrite(FHANDLE hFile, LPVOID buf, DWORD size, DWORD & wr); 
BOOL xread (FHANDLE hFile, LPVOID buf, DWORD size, DWORD & wr); 
#else
#define xwrite(hFile, buf, size, wr) WriteFile(hFile, buf, size, &wr, NULL)
#define xread(hFile, buf, size, rd) ReadFile(hFile, buf, size, &rd, NULL)
#endif

class tFileStream;
class tStream
{ 
 protected:
  bool error;  // file error
  t_asn_error struct_error;
  bool eof;                  // eof not used in output streams
  unsigned char * buf;
  unsigned bufpos, bufcont;  // bufpos not used in output streams
  unsigned extpos;           // current position in the file
  void load_buf(void);
  void save_buf(void);
 public:
  tStream(void)
    { error=false;  struct_error=ERR_NO;  bufcont=extpos=0;  buf=NULL; }
  virtual ~tStream(void) { }
  virtual unsigned load_val(void * dest, unsigned size) = 0;
  virtual void save_val(const void * src, unsigned size) = 0;
  virtual unsigned char get_advance_byte(unsigned offset) = 0;
  virtual void get_digest(CryptoPP::HashModule &md, unsigned char * digest, unsigned * digest_length) const = 0;
  virtual void copy(tStream * str) const = 0;
  virtual void copy_encrypting_digesting(tFileStream * str, IDEAXCBCEncryption & idea, CryptoPP::HashModule &md, unsigned char * digest, unsigned * digest_length) const = 0;
  //virtual void copy_encrypting_digesting(tFileStream * str, DES3CBCEncryption  & idea, CryptoPP::HashModule &md, unsigned char * digest, unsigned * digest_length) const = 0;
  virtual unsigned extent(void) const = 0;
  void reset(void)
    { error=false;  struct_error=ERR_NO;  bufpos=extpos=0; }
 // upper level functions:
  void save_dir(unsigned value, unsigned bytes)
    { save_val(&value, bytes); }
  void save_tag(const unsigned char * tag);
  bool check_tag(const unsigned char * tag, bool skip = true);
  bool is_end_of_contents(void) // when end-of-contents is present, skips it and returns true, otherwise returns false
  { if (get_advance_byte(0) || get_advance_byte(1)) return false;
    load_val(NULL, 2);
    return true;
  }
  void set_struct_error(t_asn_error errIn) 
    { struct_error=errIn; }
  t_asn_error any_error(void) 
    { return struct_error!=ERR_NO ? struct_error : error ? ERR_FILE : ERR_NO; }
  unsigned load_length(void);
  void save_length(unsigned length);
  void load_end_of_contents(void);
  unsigned external_pos(void)     // current position in the file
    { return extpos; }
  void start_constructed(t_bookmark * bookmark);
  bool check_constructed_end(t_bookmark * bookmark);
};

class tFileStream : public tStream
{ FHANDLE hFile;
  enum { STREAM_BUF_SIZE=256, BIG_STREAM_BUF_SIZE=(128*1024) };
  unsigned bufsize;
  void load_buf(void);
  void save_buf(void);
 public:
  char pathname[MAX_PATH];
  tFileStream(void) : tStream() { hFile=INVALID_FHANDLE_VALUE; }
  virtual ~tFileStream(void);
  virtual unsigned load_val(void * dest, unsigned size);
  virtual void save_val(const void * src, unsigned size);
  virtual unsigned char get_advance_byte(unsigned offset);
  virtual void get_digest(CryptoPP::HashModule &md, unsigned char * digest, unsigned * digest_length) const;
  virtual void copy(tStream * str) const;
  virtual void copy_encrypting_digesting(tFileStream * str, IDEAXCBCEncryption & idea, CryptoPP::HashModule &md, unsigned char * digest, unsigned * digest_length) const;
  //virtual void copy_encrypting_digesting(tFileStream * str, DES3CBCEncryption  & idea, CryptoPP::HashModule &md, unsigned char * digest, unsigned * digest_length) const;
  virtual unsigned extent(void) const;
 // file_specific functions:
  bool open_output(const char * pathnameIn, bool fast = false);
  void close_output(void);
  bool open_input(const char * pathname, bool fast = false, FHANDLE handle = INVALID_FHANDLE_VALUE);
  void close_input(void);
};

class tMemStream : public tStream
{ unsigned bufsize;
  bool allocated_buf;
 public:
  tMemStream(void) : tStream() { bufsize=0;  allocated_buf=false; }
  virtual ~tMemStream(void);
  virtual unsigned load_val(void * dest, unsigned size);
  virtual void save_val(const void * src, unsigned size);
  virtual unsigned char get_advance_byte(unsigned offset);
  virtual void get_digest(CryptoPP::HashModule &md, unsigned char * digest, unsigned * digest_length) const;
  virtual void copy(tStream * str) const;
  virtual void copy_encrypting_digesting(tFileStream * str, IDEAXCBCEncryption & idea, CryptoPP::HashModule &md, unsigned char * digest, unsigned * digest_length) const;
  //virtual void copy_encrypting_digesting(tFileStream * str, DES3CBCEncryption  & idea, CryptoPP::HashModule &md, unsigned char * digest, unsigned * digest_length) const;
          void copy(tStream * str, CryptoPP::IDEA & idea) const;
		  void copy(tStream * str, CryptoPP::IDEAEncryption & idea) const;
  virtual unsigned extent(void) const;
 // memory-specific functions:
  bool open_output(unsigned extent);
  unsigned char * close_output(void);
  bool open_input(unsigned char * bufIn, unsigned bufsizeIn);
  void close_input(void);
};

//////////////////////////////////////////// boolean //////////////////////////////////////////
class t_asn_boolean // implements boolean (1-octet length)
{public:
  unsigned char val;
 public:
  t_asn_boolean(unsigned char valIn = 0) 
    { val=valIn; }
  virtual ~t_asn_boolean(void) {}
  virtual void save(tStream * str, const unsigned char * tag = tag_universal_boolean) const;
  virtual void load(tStream * str, const unsigned char * tag = tag_universal_boolean);
  virtual unsigned der_length(bool outer, const unsigned char * tag = tag_universal_boolean) const;
  virtual t_asn_boolean * copy(void) const
  { t_asn_boolean * cpy = new t_asn_boolean(); 
    if (cpy) cpy->val=val;
    return cpy;
  }
//  virtual operator ==(const tag_universal_boolean & op2) const = 0;
};
        

//////////////////////////////////////////// integer //////////////////////////////////////////
class t_asn_integer // implements universal integer with single-byte length
{public:
  bool is_direct;
 public:
  t_asn_integer(bool isDirectIn) : is_direct(isDirectIn) {}
  virtual ~t_asn_integer(void) {}
  virtual void save(tStream * str, const unsigned char * tag = tag_universal_integer) const = 0;
  virtual void load(tStream * str, const unsigned char * tag = tag_universal_integer) = 0;
  virtual unsigned der_length(bool outer, const unsigned char * tag = tag_universal_integer) const = 0;
  virtual bool operator ==(const t_asn_integer & op2) const = 0;
  virtual t_asn_integer * copy(void) const = 0;
};
        
class t_asn_integer_direct : public t_asn_integer
{public: 
  int value;
 public:
  t_asn_integer_direct(void) : t_asn_integer(true) { }
  t_asn_integer_direct(int valueIn) : t_asn_integer(true)
    { value=valueIn; }
  virtual void save(tStream * str, const unsigned char * tag = tag_universal_integer) const;
  virtual void load(tStream * str, const unsigned char * tag = tag_universal_integer);
  virtual unsigned der_length(bool outer, const unsigned char * tag = tag_universal_integer) const;
  virtual bool operator ==(const t_asn_integer & op2) const
  { if (!op2.is_direct) return false;
    int val2 = ((t_asn_integer_direct&)op2).value;
    return value==val2; 
  }
  virtual t_asn_integer * copy(void) const
  { return new t_asn_integer_direct(value); }
};

class t_asn_integer_indirect : public t_asn_integer
{public:
  unsigned length;
  unsigned char * value;
 public:
  t_asn_integer_indirect(void) : t_asn_integer(false)
   { value=NULL;  }
  virtual ~t_asn_integer_indirect(void) 
   { if (value) delete [] value; }
  void assign(const unsigned char * strng, unsigned value_length);
  virtual void save(tStream * str, const unsigned char * tag = tag_universal_integer) const;
  virtual void load(tStream * str, const unsigned char * tag = tag_universal_integer);
  t_asn_integer_indirect(const CryptoPP::Integer & inp);
  virtual unsigned der_length(bool outer, const unsigned char * tag = tag_universal_integer) const;
  virtual bool operator ==(const t_asn_integer & op2) const
  { if (op2.is_direct) return false;
    return length==((t_asn_integer_indirect&)op2).length && !memcmp(value, ((t_asn_integer_indirect&)op2).value, length); 
  }
  virtual t_asn_integer * copy(void) const
  { t_asn_integer_indirect * res = new t_asn_integer_indirect(); 
    if (!res) return NULL;
    res->assign(value, length);
    return res;
  }
  void assign_reversed(const unsigned char * strng, unsigned value_length);
};

class t_asn_integer_univ// : public t_asn_integer
{public:
  int dirval;
  unsigned length;
  unsigned char * value;
  bool is_direct(void) const
    { return value==NULL; }
 public:
  t_asn_integer_univ(void) //: t_asn_integer(true)  // unknown if it is direct
   { value=NULL;  }
  t_asn_integer_univ(int dirvalIn)
   { value=NULL;  dirval=dirvalIn; }
  virtual ~t_asn_integer_univ(void) 
   { if (value) delete value; }
  void assign(const unsigned char * strng, unsigned value_length);
  virtual void save(tStream * str, const unsigned char * tag = tag_universal_integer) const;
  virtual void load(tStream * str, const unsigned char * tag = tag_universal_integer);
  t_asn_integer_univ(const CryptoPP::Integer & inp);
  virtual unsigned der_length(bool outer, const unsigned char * tag = tag_universal_integer) const;
  virtual bool operator ==(const t_asn_integer_univ & op2) const
  { if (op2.is_direct()!=is_direct()) return false;
    if (is_direct()) return dirval==op2.dirval;
    return length==op2.length && !memcmp(value, op2.value, length); 
  }
  virtual t_asn_integer_univ * copy(void) const
  { t_asn_integer_univ * res = new t_asn_integer_univ(); 
    if (!res) return NULL;
    if (is_direct()) res->dirval=dirval;
    else res->assign(value, length);
    return res;
  }
  t_asn_integer_univ & operator =(const t_asn_integer_univ & op)
  { if (value) { delete value;  value=NULL; }
    if (op.is_direct()) dirval=op.dirval;
    else assign(op.value, op.length);
    return *this;
  }
  void print_hex(char * buf) const;
};
////////////////////////////////////// object id ////////////////////////////////////
class t_asn_objectid
{ unsigned octet_cnt;
  unsigned char * octets;
 public:
  void assign(const unsigned * vals, unsigned count);
  void save(tStream * str, const unsigned char * tag = tag_universal_objectid) const;
  void load(tStream * str, const unsigned char * tag = tag_universal_objectid);
  unsigned der_length(bool outer, const unsigned char * tag = tag_universal_objectid) const;
  t_asn_objectid(void) 
    { octets=NULL; }
  t_asn_objectid(const unsigned * vals, unsigned count)
    { assign(vals, count); }
  t_asn_objectid(const unsigned char * octetsIn, unsigned octet_cntIn);
  ~t_asn_objectid(void)
    { if (octets) { delete [] octets;  octets=NULL; } }
  t_asn_objectid * copy(void) const;
  bool same(const t_asn_objectid & id) const
    { return octet_cnt==id.octet_cnt && !memcmp(octets, id.octets, octet_cnt); }
  bool is(const unsigned * vals, unsigned count) const;
  const void * get_alloc(void) const { return octets; }
};

////////////////////////////////////// printable string ////////////////////////////////////
class t_asn_general_string
{protected: 
  unsigned length;
  char * val;
 public:
  void assign(const char * strng);
  t_asn_general_string(void)
    { val=NULL; }
  t_asn_general_string(const char * strng)
    { assign(strng); }
  t_asn_general_string(unsigned lengthIn, const char * valIn);
  virtual ~t_asn_general_string(void)
    { if (val) delete [] val; }
  bool same(t_asn_general_string & another) const
    { return length==another.length && !memcmp(val, another.val, length); }
  void get(char * buf) const;

  virtual void save(tStream * str, const unsigned char * tag = tag_universal_printable_string) const = 0;
  virtual void load(tStream * str, const unsigned char * tag = tag_universal_printable_string) = 0;
  virtual unsigned der_length(bool outer, const unsigned char * tag = tag_universal_printable_string) const = 0;
  virtual t_asn_general_string * copy(void) const = 0;
};

class t_asn_printable_string : public t_asn_general_string
{ 
 public:
  t_asn_printable_string(void) : t_asn_general_string() {}
  t_asn_printable_string(const char * strng) : t_asn_general_string(strng) {}
  t_asn_printable_string(unsigned lengthIn, const char * valIn) : t_asn_general_string(lengthIn, valIn) {}
  virtual void save(tStream * str, const unsigned char * tag = tag_universal_printable_string) const;
  virtual void load(tStream * str, const unsigned char * tag = tag_universal_printable_string);
  virtual unsigned der_length(bool outer, const unsigned char * tag = tag_universal_printable_string) const;
  virtual t_asn_general_string * copy(void) const;
};

////////////////////////////////////// ia5 string ////////////////////////////////////
class t_asn_ia5_string
{ unsigned length;
  char * val;
 public:
  void assign(const char * strng);
  void save(tStream * str, const unsigned char * tag = tag_universal_ia5_string) const;
  void load(tStream * str, const unsigned char * tag = tag_universal_ia5_string);
  unsigned der_length(bool outer, const unsigned char * tag = tag_universal_ia5_string) const;
  t_asn_ia5_string(void)
    { val=NULL; }
  t_asn_ia5_string(const char * strng)
    { assign(strng); }
  t_asn_ia5_string(unsigned lengthIn, const char * valIn);
  ~t_asn_ia5_string(void)
    { if (val) delete [] val; }
  bool equal(const char * val2, unsigned length2)
    { return length==length2 && !memcmp(val, val2, length); }
  t_asn_ia5_string * copy(void) const;
  bool same(t_asn_ia5_string & another) const
    { return length==another.length && !memcmp(val, another.val, length); }
  void get(char * buf) const
    { memcpy(buf, val, length);  buf[length]=0; }
};

////////////////////////////////////// octet string ////////////////////////////////////
class t_asn_octet_string
{public: 
  unsigned length;
  unsigned char * val;
 public:
  t_asn_octet_string * next;
  void assign(const unsigned char * strng, unsigned value_length);
  void save(tStream * str, const unsigned char * tag = tag_universal_octet_string) const;
  void load(tStream * str, const unsigned char * tag = tag_universal_octet_string);
  unsigned der_length(bool outer, const unsigned char * tag = tag_universal_octet_string) const;
  t_asn_octet_string(void)
    { val=NULL;  next=NULL; }
  t_asn_octet_string(const unsigned char * strng, unsigned value_length)
    { assign(strng, value_length);  next=NULL; }
  ~t_asn_octet_string(void)
    { if (val) delete [] val;  if (next) delete next; }
  const unsigned char * get_val(void) const { return val; }
  unsigned get_length(void) const { return length; }
  t_asn_octet_string * copy(void) const;
};

////////////////////////////////////// t_asn_UTCTime ////////////////////////////////////
class t_asn_UTCTime
{ unsigned length;
  char * val;
  SYSTEMTIME systime;
  void str2systime(bool generalized = false);
 public:
  void assign(const char * strng);
  void save(tStream * str, const unsigned char * tag = tag_universal_utctime) const;
  void load(tStream * str, const unsigned char * tag = tag_universal_utctime);
  unsigned der_length(bool outer, const unsigned char * tag = tag_universal_utctime) const;
  t_asn_UTCTime(void)
    { val=NULL; }
  t_asn_UTCTime(const char * strng)
    { assign(strng); }
  t_asn_UTCTime(const SYSTEMTIME * st);
  ~t_asn_UTCTime(void)
    { if (val) delete [] val; }
  const SYSTEMTIME * get_system_time(void) const
    { return &systime; }
  const char * get_time_string(void) const
    { return val; }
  void get_readable_time(char * buf) const;
};

////////////////////////////////////// bit string ////////////////////////////////////

class t_asn_bit_string
{ unsigned bitlength;  // exact length in bits
  unsigned char * val; // padded with 0s
 public:
  void assign(const unsigned char * strng, unsigned bitlengthIn);
  void save(tStream * str, const unsigned char * tag = tag_universal_bit_string) const;
  void load(tStream * str, const unsigned char * tag = tag_universal_bit_string);
  unsigned der_length(bool outer, const unsigned char * tag = tag_universal_bit_string) const;
  t_asn_bit_string(void)
    { val=NULL; }
  t_asn_bit_string(const unsigned char * strng, unsigned bitlengthIn)
    { assign(strng, bitlengthIn); }
  ~t_asn_bit_string(void)
    { if (val) delete [] val; }
  const unsigned char * get_val(void) const { return val; }
  unsigned get_bitlength(void) const { return bitlength; }
  t_asn_bit_string * copy(void) const
    { return new t_asn_bit_string(val, bitlength); }
};

/////////////////////////////////////// NULL //////////////////////////////////
class t_asn_null
{public:
  static void save(tStream * str, const unsigned char * tag = tag_universal_null) 
  { str->save_tag(tag);
    str->save_length(0);
  }
  static void load(tStream * str, const unsigned char * tag = tag_universal_null)
  { if (!str->check_tag(tag)) { str->set_struct_error(ERR_NULL_NOT_PRESENT);  return; }
    if (str->load_length()) str->set_struct_error(ERR_NULL_LENGHT);
  }
  static unsigned der_length(bool outer, const unsigned char * tag = tag_universal_null) 
    { return outer ? tag_length(tag)+1 : 0; }
  t_asn_null(void) { }
  ~t_asn_null(void) { }
};

/////////////////////////////////////// set of, sequence of //////////////////////////////////////////
template <class element> void load_set_of(tStream * str, element ** pelem, const unsigned char * tag = tag_universal_constructed_set)
{ *pelem = NULL;
  if (str->check_tag(tag))
  {// read the length of the sequence:
    t_bookmark bookmark(str);  
    do
    {// check the end of the sequence:
      if (str->check_constructed_end(&bookmark)) break;
     // read the new part
      element * anelem = new element;
      if (!anelem) { str->set_struct_error(ERR_NO_MEMORY);  return; }
      *pelem = anelem;  pelem = &anelem->next;
      anelem->load(str);
    } while (!str->any_error());
  }
  else str->set_struct_error(ERR_SET_NOT_PRESENT);
}

template <class element> void save_set_of(tStream * str, const element * elem, const unsigned char * tag = tag_universal_constructed_set) 
{ str->save_tag(tag);
 // calculate and save the length:
  unsigned total_length = 0;  const element * anelem;
  for (anelem = elem;  anelem;  anelem=anelem->next)
    total_length+=anelem->der_length(true);
  str->save_length(total_length);
 // save the elements:
  for (anelem = elem;  anelem;  anelem=anelem->next)
    anelem->save(str);
}

template <class element> unsigned der_length_set(const element * elem, const unsigned char * tag = tag_universal_constructed_set) // outer
{ unsigned total_length = 0;
  for (const element * anelem = elem;  anelem;  anelem=anelem->next)
    total_length+=anelem->der_length(true);
  return tag_length(tag) + length_of_length(total_length) + total_length;
}

template <class element> void load_sequence_of(tStream * str, element ** pelem, const unsigned char * tag = tag_universal_constructed_sequence)
{ *pelem = NULL;
  if (str->check_tag(tag))
  {// read the length of the sequence:
    t_bookmark bookmark(str);  
    do
    {// check the end of the sequence:
      if (str->check_constructed_end(&bookmark)) break;
     // read the new part
      element * anelem = new element;
      if (!anelem) { str->set_struct_error(ERR_NO_MEMORY);  return; }
      *pelem = anelem;  pelem = &anelem->next;
      anelem->load(str);
    } while (!str->any_error());
  }
  else str->set_struct_error(ERR_SEQUENCE_NOT_PRESENT);
}

template <class element> void save_sequence_of(tStream * str, const element * elem, const unsigned char * tag = tag_universal_constructed_sequence) 
{ str->save_tag(tag);
 // calculate and save the length:
  unsigned total_length = 0;  const element * anelem;
  for (anelem = elem;  anelem;  anelem=anelem->next)
    total_length+=anelem->der_length(true);
  str->save_length(total_length);
 // save the elements:
  for (anelem = elem;  anelem;  anelem=anelem->next)
    anelem->save(str);
}

template <class element> unsigned der_length_sequence(const element * elem, const unsigned char * tag = tag_universal_constructed_sequence) // outer
{ unsigned total_length = 0;
  for (const element * anelem = elem;  anelem;  anelem=anelem->next)
    total_length+=anelem->der_length(true);
  return tag_length(tag) + length_of_length(total_length) + total_length;
}
////////////////////////////////////// Name //////////////////////////////////////////
class t_asn_Name;

class t_asn_AVAssert
{ friend class t_asn_Name;  
  friend void get_common_name(const t_asn_Name * Name, char * buf);
  t_asn_objectid * attribute_type;
  void * attribute_value;
  bool is_printable;  // otherwise IA5 supposed
 public:
  t_asn_AVAssert * next;
  void load(tStream * str); // loads an element
  void save(tStream * str) const;  // saves an element
  unsigned der_length(bool outer) const;
  t_asn_AVAssert(void)
    { is_printable=false;  attribute_type  = NULL;  attribute_value = NULL;  next=NULL; }
  t_asn_AVAssert(t_asn_objectid * attribute_typeIn, void * attribute_valueIn, bool is_printableIn)
    { is_printable=is_printableIn;  attribute_type  = attribute_typeIn;  attribute_value = attribute_valueIn;  next=NULL; }
  t_asn_AVAssert(const unsigned * object_id, unsigned object_id_length, const char * value, bool printable)
  { attribute_type  = new t_asn_objectid(object_id, object_id_length);
    if (printable)
      attribute_value = new t_asn_printable_string(value);
    else
      attribute_value = new t_asn_ia5_string(value);
    is_printable=printable;  next=NULL;
  }
  ~t_asn_AVAssert(void);
  t_asn_AVAssert * copy(void) const;
  bool same(t_asn_AVAssert & anAssert) const
  { if (!attribute_type->same(*anAssert.attribute_type)) return false;
    if (is_printable)
      return ((t_asn_printable_string*)attribute_value)->same(*(t_asn_printable_string*)anAssert.attribute_value);
    else
      return ((t_asn_ia5_string      *)attribute_value)->same(*(t_asn_ia5_string      *)anAssert.attribute_value);
  }
};

class t_asn_Name
{ t_asn_AVAssert * AVAssert;
  friend void get_common_name(const t_asn_Name * Name, char * buf);
 public: 
  t_asn_Name * next;  // next in the sequence of RDNames
  void load(tStream * str); // loads an element
  void save(tStream * str) const;  // saves an element
  unsigned der_length(bool outer) const;
  t_asn_Name(void)
    { AVAssert=NULL;  next=NULL; }
  t_asn_Name(const unsigned * object_id, unsigned object_id_length, const char * value, bool printable)
  { AVAssert = new t_asn_AVAssert(object_id, object_id_length, value, printable);
    next=NULL;
  }
  ~t_asn_Name(void);
  t_asn_Name * copy(void) const;
  t_asn_Name * copy_seq(void) const;
  bool get_readableRDN(char * & buf, unsigned & bufsize) const;
  bool same(const t_asn_Name & aName) const 
  { if (!AVAssert->same(*aName.AVAssert)) return false;
    if (next==NULL) return aName.next==NULL;
    if (aName.next==NULL) return false;
    return next->same(*aName.next);
  }
  void get_strval(char * buf) const
  { if (AVAssert->is_printable) ((t_asn_printable_string*)AVAssert->attribute_value)->get(buf); 
    else                        ((t_asn_ia5_string      *)AVAssert->attribute_value)->get(buf); 
  }
};

void get_common_name(const t_asn_Name * Name, char * buf);

///////////////////////////////////// CRI atributes /////////////////////////////////////////////
class t_asn_Extensions;

class t_asn_cri_atribute_value  // auxiliary type allowing the sequencing of ia5 string type
{ t_asn_ia5_string * val;  // single value
 public:
  t_asn_Extensions * ExtensionReq;  // used by the extensionReq attribute only
 public:
  t_asn_cri_atribute_value * next;
  void load(tStream * str); // loads an element
  void save(tStream * str) const;  // saves an element
  unsigned der_length(bool outer) const;
  t_asn_cri_atribute_value(void)
    { val = NULL;  ExtensionReq=NULL;  next=NULL; }
  t_asn_cri_atribute_value(const char * value)
    { val = new t_asn_ia5_string(value);  ExtensionReq=NULL;  next=NULL; }
  t_asn_cri_atribute_value(t_asn_Extensions * ExtensionReqIn)
    { val = NULL;  ExtensionReq=ExtensionReqIn;  next=NULL; }
  ~t_asn_cri_atribute_value(void);
  bool equal(const char * val2)
    { return val && val->equal(val2, (int)strlen(val2)); }
};

class t_asn_cri_atribute
{ t_asn_objectid * type;
 public:
  t_asn_cri_atribute_value * attribute_values;
 public:
  t_asn_cri_atribute * next;
  void load(tStream * str); // loads an element
  void save(tStream * str) const;  // saves an element
  unsigned der_length(bool outer) const;
  t_asn_cri_atribute(void)
    { type  = NULL;  attribute_values = NULL;  next=NULL; }
  t_asn_cri_atribute(const unsigned * object_id, unsigned object_id_length, const char * value)
  { type  = new t_asn_objectid(object_id, object_id_length);
    attribute_values = new t_asn_cri_atribute_value(value);
    next=NULL;
  }
  t_asn_cri_atribute(t_asn_Extensions * ExtensionReqIn)
  { type  = new t_asn_objectid(cri_attr_type_extension_req, cri_attr_type_extension_req_length);
    attribute_values = new t_asn_cri_atribute_value(ExtensionReqIn);
    next=NULL;
  }
  ~t_asn_cri_atribute(void);
  bool equal(const char * val2)
    { return attribute_values && attribute_values->equal(val2); }
  bool is_type(const unsigned * vals, unsigned count) const
    { return type && type->is(vals, count); }
};

////////////////////////////////////// general atributes //////////////////////////////////////////
class t_asn_attribute
{ t_asn_objectid * attribute_type;
  unsigned char value_tag; 
  void * attribute_value;
 public:
  t_asn_attribute * next;   // usually not used
  void load(tStream * str); // loads an element
  void save(tStream * str) const;  // saves an element
  unsigned der_length(bool outer) const;
  t_asn_attribute(void)
    { attribute_type  = NULL;  value_tag=0;  attribute_value = NULL;  next=NULL; }
  t_asn_attribute(const unsigned * object_id, unsigned object_id_length, unsigned char tag, void * value)
  { attribute_type  = new t_asn_objectid(object_id, object_id_length);
    value_tag=tag;
    attribute_value = value; // takes the ownership of "value" !!
    next=NULL;
  }
  ~t_asn_attribute(void);
  t_asn_attribute * find(const unsigned * object_id, unsigned object_id_length);
  const void * get_attribute_value(void) { return attribute_value; }
};

///////////////////////////////// t_asn_algorithm_identifier /////////////////////////////////////////////////////
class t_asn_algorithm_identifier
{ t_asn_objectid * algorithm;
  t_asn_octet_string * octet_parameters; // optional, supported only NULL, OCTET STRING and INTEGER
  t_asn_integer_direct * int_parameters;
 public:
  t_asn_algorithm_identifier * next;  // makes this class sequencable
  t_asn_algorithm_identifier(void)
    { algorithm=NULL;  octet_parameters=NULL;  int_parameters=NULL;  next=NULL; }
  t_asn_algorithm_identifier(const unsigned * object_id, unsigned object_id_length)
  { algorithm  = new t_asn_objectid(object_id, object_id_length);  
    octet_parameters = NULL;  int_parameters=NULL;  next=NULL;
  }
  ~t_asn_algorithm_identifier(void)
  { if (algorithm) delete algorithm;  if (octet_parameters) delete octet_parameters;  
    if (int_parameters) delete int_parameters;  if (next) delete next; 
  }
  unsigned der_length(bool outer) const
  { unsigned total_length = algorithm->der_length(true); 
    if      (octet_parameters)  total_length += octet_parameters->der_length(true);
    else if (int_parameters)    total_length += int_parameters->der_length(true);
    else                        total_length += t_asn_null::der_length(true);  
    if (outer) total_length += tag_length(tag_universal_constructed_sequence) + length_of_length(total_length);
    return total_length;
  }
  void save(tStream * str) const
  { str->save_tag(tag_universal_constructed_sequence);
    str->save_length(der_length(false));  // inner length
   // save fields:
    algorithm->save(str);
    if      (octet_parameters) octet_parameters->save(str);
    else if (int_parameters)   int_parameters->save(str);  
    else                       t_asn_null::save(str); 
  }
  void load(tStream * str) 
  { if (!str->check_tag(tag_universal_constructed_sequence))
      { str->set_struct_error(ERR_SEQUENCE_NOT_PRESENT);  return; }
    unsigned length = str->load_length();
   // load fields:
    algorithm = new t_asn_objectid;
    if (!algorithm) { str->set_struct_error(ERR_NO_MEMORY);  return; }
    algorithm->load(str);
    if (str->any_error()) return;
    if (str->check_tag(tag_universal_null, false))
    { t_asn_null::load(str);   
      octet_parameters=NULL;
    } 
    else if (str->check_tag(tag_universal_octet_string, false))
    { octet_parameters = new t_asn_octet_string;
      if (!octet_parameters) { str->set_struct_error(ERR_NO_MEMORY);  return; }
      octet_parameters->load(str);
    }
    else if (str->check_tag(tag_universal_integer, false))
    { int_parameters = new t_asn_integer_direct;
      if (!int_parameters) { str->set_struct_error(ERR_NO_MEMORY);  return; }
      int_parameters->load(str);
    }
    else 
    { if (length > algorithm->der_length(true))
        str->set_struct_error(ERR_UNSUPPORTED);  // not supported
      // otherwise paramaters not specified
    }
  }

  t_asn_algorithm_identifier * copy(void) const
  { t_asn_algorithm_identifier * ai = new t_asn_algorithm_identifier;
    if (ai)
    { ai->algorithm = algorithm->copy();
      if (octet_parameters) ai->octet_parameters = octet_parameters->copy();
      if (int_parameters)   ai->int_parameters   = (t_asn_integer_direct *)int_parameters->copy();
      if (ai->algorithm && (!octet_parameters || ai->octet_parameters) && (!int_parameters || ai->int_parameters)) 
        return ai;
      delete ai;
    }
    return NULL;
  }
  t_algors algorithm_type(void);
  const unsigned char * get_IDEAXEX_IV(void) const;
  const unsigned char * get_DESXEX_IV (void) const;
  bool set_IV_idea(const unsigned char * IV);
  bool set_IV_des (const unsigned char * IV);
};

////////////////////////////////// t_asn_Validity ///////////////////////////////////////////////////////////////
int compare_systime(const SYSTEMTIME & tm1, const SYSTEMTIME & tm2);
void systemtime_add(SYSTEMTIME & tm, int years, int months, int days, int hours, int minutes, int seconds);
void systemtime2UTC(const SYSTEMTIME & SysTm, char * utc_time);
void systemtime2rdbl(const SYSTEMTIME & SysTm, char * rdbl_time);
const char * current_UTC(void);

class t_asn_Validity
{ t_asn_UTCTime * start, * end;
 public:
  void load(tStream * str); 
  void save(tStream * str) const;
  unsigned der_length(bool outer) const;
  t_asn_Validity(void)
    { start = end = NULL; }
  ~t_asn_Validity(void)
    { if (start) delete start;  if (end) delete end; }
  bool set_dates(const char * start_string, const char * end_string);
  bool is_valid_now(const SYSTEMTIME * reference_time) const;
  const t_asn_UTCTime * get_start(void) { return start; }
  const t_asn_UTCTime * get_end  (void) { return end;   }
  t_asn_Validity * copy(void) const;
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////
bool add_disting_name(t_asn_Name ** pName, const unsigned * object_id, unsigned object_id_length, const char * value);
t_asn_Name * create_name_list(const t_name_part * parts);
t_asn_Name * copy_disting_name(const t_asn_Name * sourceName);

class t_hash
{ CryptoPP::RIPEMD160 ripemd160;  
  CryptoPP::RIPEMD128 ripemd128;  
  CryptoPP::SHA       sha; 
  int sel; 
  t_hash(void) { } // default constructor disabled
 public:
  t_hash(const char * hash_name);
  t_hash(t_algors alg);
  CryptoPP::HashModule * get_hash_module(void) const;
  const unsigned char * get_hash_decoration(void);
  unsigned get_hash_decoration_length(void);
  const unsigned * get_hash_algorithm_id(void);
  unsigned get_hash_algorithm_id_length(void);
  const unsigned * get_hash_algorithmRSA_id(void);
  unsigned get_hash_algorithmRSA_id_length(void);
  bool is_error(void);
  const char * hash_name(void);
};

extern const t_asn_objectid object_id_ripemd160;
extern const t_asn_objectid object_id_ripemd128;
extern const t_asn_objectid object_id_sha      ;
extern const t_asn_objectid object_id_rsaripemd160;
extern const t_asn_objectid object_id_rsaripemd128;
extern const t_asn_objectid object_id_rsasha      ;
extern const t_asn_objectid object_id_ideaxex3    ;
extern const t_asn_objectid object_id_des3cbc     ;

extern const t_asn_objectid sigenv_data_content_object;
extern const t_asn_objectid signed_data_content_object;
