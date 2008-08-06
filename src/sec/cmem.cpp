// cmem.cpp
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
#define ENGLISH

const unsigned attr_type_country_name    [] = { 2, 5, 4, 6 };
const unsigned attr_type_organiz_name    [] = { 2, 5, 4, 10 };
const unsigned attr_type_org_unit_name   [] = { 2, 5, 4, 11 };
const unsigned attr_type_common_unit_name[] = { 2, 5, 4, 3 };
const unsigned attr_type_given_name      [] = { 2, 5, 4, 42 };
const unsigned attr_type_initials        [] = { 2, 5, 4, 43 };
const unsigned attr_type_surname         [] = { 2, 5, 4, 4 };
const unsigned attr_type_domain_comp_name[] = { 0, 9, 2342, 19200300, 100, 1, 25 };
const unsigned attr_type_email_name      [] = { 1, 2, 840, 113549, 1, 9, 1 };

const unsigned attr_id_content_type  [] = { 1, 2, 840, 113549, 1, 9, 3 };
const unsigned attr_id_message_digest[] = { 1, 2, 840, 113549, 1, 9, 4 };
const unsigned attr_id_signing_time  [] = { 1, 2, 840, 113549, 1, 9, 5 };
const unsigned attr_id_account_date  [] = { 1, 3, 6, 1, 4, 1, 4511, 1, 2, 2 };

const char cri_attribute_signature[]  = "signature";
const char cri_attribute_encryption[] = "encryption";
const char cri_attribute_CA[]         = "CA";
const char cri_attribute_RCA[]        = "RCA";

const unsigned char tag_universal_boolean[1]              = { 1 };
const unsigned char tag_universal_integer[1]              = { 2 };
const unsigned char tag_universal_objectid[1]             = { 6 };
const unsigned char tag_universal_printable_string[1]     = { 19 };
const unsigned char tag_universal_bit_string[1]           = { 3 };
const unsigned char tag_universal_ia5_string[1]           = { 22 };
const unsigned char tag_universal_octet_string[1]         = { 4 };
const unsigned char tag_universal_null[1]                 = { 5 };
const unsigned char tag_universal_utctime[1]              = { 23 };
const unsigned char tag_universal_strange_string[1]       = { 12 };
const unsigned char tag_universal_utctime2[1]             = { 24 };  // used by 1CA in CRLs

const unsigned char tag_universal_constructed_sequence[1] = { 32+16 };
const unsigned char tag_universal_constructed_set[1]      = { 32+17 };

const unsigned char tag_kontext_primitive_0[1]   = { 128 + 0 };
const unsigned char tag_kontext_primitive_1[1]   = { 128 + 1 };
const unsigned char tag_kontext_primitive_2[1]   = { 128 + 2 };
const unsigned char tag_kontext_constructed_0[1] = { 128 + 32 + 0 };
const unsigned char tag_kontext_constructed_1[1] = { 128 + 32 + 1 };
const unsigned char tag_kontext_constructed_3[1] = { 128 + 32 + 3 };
const unsigned char tag_kontext_constructed_4[1] = { 128 + 32 + 4 };
////////////////////////////////////////// IDs //////////////////////////////////////////////////////
//const unsigned cri_attr_type_unstructure_name   [] = { 1, 2, 840, 113549, 1, 9, 26, 1 };  // ??
//#define cri_attr_type_unstructure_name_length    8
const unsigned cri_attr_type_unstructured_name   [] = { 1, 2, 840, 113549, 1, 9, 2 };  // from CNB example
const unsigned cri_attr_type_extension_req[]        = { 1, 2, 840, 113549, 1, 9, 14 }; // from MS CA request
const unsigned algorithm_id_rsaencryption       [] = { 1, 2, 840, 113549, 1, 1, 1 };  // CNB has the same
const unsigned algorithm_id_rsaSigWithMD160[] = { 1, 3, 36, 3, 3, 1, 2 }; // from CNB docs
const unsigned algorithm_id_rsaSigWithMD128[] = { 1, 3, 36, 3, 3, 1, 3 }; // from CNB docs
const unsigned algorithm_id_rsaSigWithSHA[]   = { 1, 2, 840, 113549, 1, 1, 5 }; // from CNB docs
const unsigned algorithm_id_ripemd160[]       = { 1, 3, 36, 3, 2, 1 };  // from CNB docs
const unsigned algorithm_id_ripemd128[]       = { 1, 3, 36, 3, 2, 2 };  // from CNB docs
const unsigned algorithm_id_sha      []       = { 1, 3, 14, 3, 2, 26 }; // from CNB docs
const unsigned algorithm_id_ideaxex3[]        = { 1, 3, 6, 1, 4, 1, 4511, 1, 1, 1 };  // from CNB docs
const unsigned algorithm_id_des3cbc[]         = { 1, 2, 840, 113549, 3, 7 };  // from PMB

const t_asn_objectid object_id_ripemd160(algorithm_id_ripemd160, algorithm_id_ripemd160_length);
const t_asn_objectid object_id_ripemd128(algorithm_id_ripemd128, algorithm_id_ripemd128_length);
const t_asn_objectid object_id_sha      (algorithm_id_sha,       algorithm_id_sha_length);
const t_asn_objectid object_id_rsaripemd160(algorithm_id_rsaSigWithMD160, algorithm_id_rsaSigWithMD160_length);
const t_asn_objectid object_id_rsaripemd128(algorithm_id_rsaSigWithMD128, algorithm_id_rsaSigWithMD128_length);
const t_asn_objectid object_id_rsasha      (algorithm_id_rsaSigWithSHA,   algorithm_id_rsaSigWithSHA_length  );
const t_asn_objectid object_id_ideaxex3    (algorithm_id_ideaxex3,        algorithm_id_ideaxex3_length  );
const t_asn_objectid object_id_des3cbc     (algorithm_id_des3cbc,         algorithm_id_des3cbc_length  );

// 0x80=dig podpis, 0x40=Neodvolatelnost, 0x20=sifrovani klice
const unsigned char keyUsageSign   [] = { 3, 2, 6, 0xc0 };
//const unsigned char keyUsageEncrypt[] = { 3, 2, 3, 0xe0 };
const unsigned char keyUsageEncrypt[] = { 3, 2, 3, 0x38 };
const unsigned char keyUsageSignEnc[] = { 3, 2, 5, 0xa0 };
const unsigned char keyUsageCA     [] = { 3, 2, 1, 0xc6 };   // incl. Neodvolatelnost
const unsigned char keyUsageCA2    [] = { 3, 2, 1, 0x86 };   // excl. Neodvolatelnost
const unsigned char keyUsageCA3    [] = { 3, 2, 1, 6    };   // used by 1.CA

const unsigned exten_id_AutKeyId[]  = { 2, 5, 29, 35 };
const unsigned exten_id_BasConstr[] = { 2, 5, 29, 19 };
const unsigned exten_id_KeyUsage[]  = { 2, 5, 29, /*37*/15 }; // 37 in the printed example, 15 in the example file 2

const unsigned        data_content_object_id[] = { 1, 2, 840, 113549, 1, 7, 1 };
const unsigned signed_data_content_object_id[] = { 1, 2, 840, 113549, 1, 7, 2 };
const unsigned sigenv_data_content_object_id[] = { 1, 2, 840, 113549, 1, 7, 4 };

const t_asn_objectid sigenv_data_content_object(sigenv_data_content_object_id, sigenv_data_content_object_id_length);
const t_asn_objectid signed_data_content_object(signed_data_content_object_id, signed_data_content_object_id_length);

#ifdef ENGLISH
#define RDNATTR_IDENT      "Identification"
#define RDNATTR_FISTNAME   "First name"
#define RDNATTR_MIDDLENAME "Middle name"
#define RDNATTR_LASTNAME   "Last name"
#else
#define RDNATTR_IDENT      "Identifikace"
#define RDNATTR_FISTNAME   "Jmeno"
#define RDNATTR_MIDDLENAME "Inicialka"
#define RDNATTR_LASTNAME   "Prijmeni"
#endif
////////////////////////////////////////// tStream ////////////////////////////////////////////////////
#ifdef OVERLAPPED_IO
BOOL xwrite(HANDLE hFile, LPVOID buf, DWORD size, DWORD & wr)
{ OVERLAPPED ov;  DWORD bts;  ULARGE_INTEGER pos;
  pos.QuadPart = SetFilePointer(hFile, 0, NULL, FILE_CURRENT);
  ov.Offset = pos.LowPart;  ov.OffsetHigh = pos.HighPart;
  ov.hEvent = 0;
  BOOL pending = !WriteFile(hFile, buf, size, &bts, &ov) && GetLastError()==ERROR_IO_PENDING;
  if (pending) 
    GetOverlappedResult(hFile, &ov, &bts, TRUE);
  pos.QuadPart += bts;
  SetFilePointer(hFile, pos.QuadPart, NULL, FILE_BEGIN);
  wr=bts;
  return TRUE;
}

BOOL xread (HANDLE hFile, LPVOID buf, DWORD size, DWORD & wr) 
{ OVERLAPPED ov;  DWORD bts;  ULARGE_INTEGER pos;
  pos.QuadPart = SetFilePointer(hFile, 0, NULL, FILE_CURRENT);
  ov.Offset = pos.LowPart;  ov.OffsetHigh = pos.HighPart;
  ov.hEvent = 0;
  BOOL pending = !ReadFile(hFile, buf, size, &bts, &ov) && GetLastError()==ERROR_IO_PENDING;
  if (pending) 
    GetOverlappedResult(hFile, &ov, &bts, TRUE);
  pos.QuadPart += bts;
  SetFilePointer(hFile, pos.QuadPart, NULL, FILE_BEGIN);
  wr=bts;
  return TRUE;
}

#endif

tFileStream::~tFileStream(void)  // replaces "close_..." but does not flush the output buffer
{ if (hFile!=INVALID_FHANDLE_VALUE) CloseHandle(hFile);
  if (buf) VirtualFree(buf, 0, MEM_RELEASE);//delete [] buf;
}

bool tFileStream::open_output(const char * pathnameIn, bool fast)
{ if (pathnameIn)
  { strcpy(pathname, pathnameIn);
#ifdef OVERLAPPED_IO
    hFile=CreateFile(pathname, GENERIC_WRITE|GENERIC_READ, 0, NULL, CREATE_ALWAYS, fast ? FILE_FLAG_OVERLAPPED   : FILE_FLAG_SEQUENTIAL_SCAN, NULL);
#else
#ifdef NO_BUFFERING
    hFile=CreateFile(pathname, GENERIC_WRITE|GENERIC_READ, 0, NULL, CREATE_ALWAYS, fast ? FILE_FLAG_NO_BUFFERING : FILE_FLAG_SEQUENTIAL_SCAN, NULL);
#else
    hFile=CreateFile(pathname, GENERIC_WRITE|GENERIC_READ, 0, NULL, CREATE_ALWAYS, FILE_FLAG_SEQUENTIAL_SCAN, NULL);
#endif
#endif
    extpos=0;
  }
  else
  { hFile=CreateFile(pathname, GENERIC_WRITE|GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);
    extpos=SetFilePointer(hFile, 0, NULL, FILE_END);
  }
  if (hFile==INVALID_FHANDLE_VALUE) return false;
  bufsize = fast ? BIG_STREAM_BUF_SIZE : STREAM_BUF_SIZE;
  buf = (unsigned char*)VirtualAlloc(NULL, bufsize, MEM_COMMIT, PAGE_READWRITE);
  //buf = new unsigned char[bufsize];
  if (!buf) { CloseHandle(hFile);  return false; }
  error=false;  struct_error=ERR_NO;  bufcont=0;
  return true;
}

void tFileStream::close_output(void)
{ if (bufcont) save_buf();
  CloseHandle(hFile);  hFile=INVALID_FHANDLE_VALUE;
  //delete [] buf;  
  VirtualFree(buf, 0, MEM_RELEASE);
  buf=NULL;
}

FHANDLE open_input_file(const char * pathname, bool fast)
// -- Requires write access too in order to prevent processing open files - no, this disable access to read-only files!
// Share mode 0 should do the same!
{
#ifdef OVERLAPPED_IO
  return CreateFile(pathname, GENERIC_READ, 0, NULL, OPEN_EXISTING, fast ? FILE_FLAG_OVERLAPPED   : FILE_FLAG_SEQUENTIAL_SCAN, NULL);
#else
#ifdef NO_BUFFERING
  return CreateFile(pathname, GENERIC_READ, 0, NULL, OPEN_EXISTING, fast ? FILE_FLAG_NO_BUFFERING : FILE_FLAG_SEQUENTIAL_SCAN, NULL);
#else
  return CreateFile(pathname, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);
#endif
#endif
}

bool tFileStream::open_input(const char * pathname, bool fast, FHANDLE handle)
{ hFile = handle==INVALID_FHANDLE_VALUE ? open_input_file(pathname, fast) : handle;
  if (hFile==INVALID_FHANDLE_VALUE) return false;
  bufsize = fast ? BIG_STREAM_BUF_SIZE : STREAM_BUF_SIZE;
  buf = (unsigned char*)VirtualAlloc(NULL, bufsize, MEM_COMMIT, PAGE_READWRITE);
  //buf = new unsigned char[bufsize];
  if (!buf) { CloseHandle(hFile);  return false; }
  error=false;  struct_error=ERR_NO;  bufcont=extpos=0;
  bufpos=0;  eof=false;
  return true;
}

void tFileStream::close_input(void)
{ CloseHandle(hFile);  hFile=INVALID_FHANDLE_VALUE;
  //delete [] buf;  
  VirtualFree(buf, 0, MEM_RELEASE);
  buf=NULL;
}

unsigned char tFileStream::get_advance_byte(unsigned offset)
{ if (offset >= bufcont-bufpos) 
  { load_buf();
    if (offset >= bufcont-bufpos) return 0;  // does not match any valid tag
  }
  return buf[bufpos+offset];
}

void tFileStream::load_buf(void)
{ if (eof) error=true;
  else
  { if (bufpos!=bufcont)
    { memmove(buf, buf+bufpos, bufcont-bufpos);
      bufpos=0;  bufcont-=bufpos;
    }
    else bufpos=bufcont=0;
    DWORD rd;
    if (!xread(hFile, buf, bufsize-bufcont, rd)) error=true;
    if (rd<bufsize-bufcont) eof=TRUE;
    bufcont+=rd;
  }
}

unsigned tFileStream::load_val(void * dest, unsigned size) // dest may be NULL
{ unsigned loaded = 0;
  while (TRUE)
  { unsigned step=bufcont-bufpos;
    if (step>size) step=size;
    if (dest) { memcpy(dest, buf+bufpos, step);  dest=(char*)dest+step; }
    size-=step;  bufpos+=step;  extpos+=step;  loaded+=step;
    if (!size) return loaded;
    load_buf();  
    if (bufcont==bufpos) { error=true;  return loaded; }
  }
}

void tFileStream::save_buf(void)
{ DWORD wr;
#ifdef NO_BUFFERING
  if (bufsize == BIG_STREAM_BUF_SIZE && (bufcont % sector_size))
  { unsigned alg = (bufcont / sector_size + 1) * sector_size;
    if (!xwrite(hFile, buf, alg, wr)) error=true;
  }
  else
#endif
    if (!xwrite(hFile, buf, bufcont, wr)) error=true;
  bufcont=0;
}

void tFileStream::save_val(const void * src, unsigned size)
{ while (size)
  { unsigned step = bufsize-bufcont;
    if (step > size) step=size;
    memcpy(buf+bufcont, src, step);
    bufcont += step;  extpos+=step;
    if (bufcont==bufsize) save_buf();
    src = (const char*)src+step;  size -= step;
  }
}

void tFileStream::get_digest(CryptoPP::HashModule &md, unsigned char * digest, unsigned * digest_length) const
{ unsigned length = GetFileSize(hFile, NULL);
  SetFilePointer(hFile, 0, NULL, FILE_BEGIN);
  unsigned file_offset=0;
  if (length>10000) 
  { progress_open(0, length);
    progress_set_text("Prob��digestov��z�ilky...");
  }
  DWORD rd;
  do
  { xread(hFile, buf, bufsize, rd);
    GetLastError();
    md.Update(buf, rd);
    file_offset+=rd;  progress_set_value(file_offset, length);  
  } while (rd==bufsize);
	md.Final(digest);
  *digest_length=md.DigestSize();
  progress_close();
}

void tFileStream::copy(tStream * str) const
{ SetFilePointer(hFile, 0, NULL, FILE_BEGIN);
  DWORD rd;
  do
  { xread(hFile, buf, bufsize, rd);
    GetLastError();
    str->save_val(buf, rd);
  } while (rd==bufsize);
}

#ifdef OVERLAPPED_IO
void tFileStream::copy_encrypting_digesting(tFileStream * str, IDEAXCBC & idea, CryptoPP::HashModule &md, unsigned char * digest, unsigned * digest_length) const
{ unsigned length = GetFileSize(hFile, NULL);
  unsigned file_offset=0;
  if (length>10000) 
  { progress_open(0, length);
    progress_set_text("Prob��ifrov��z�ilky...");
  }
  unsigned char * obuf[3];
  obuf[0] = (unsigned char*)VirtualAlloc(NULL, BIG_STREAM_BUF_SIZE+IDEAXCBC::BLOCKSIZE, MEM_COMMIT, PAGE_READWRITE);
  obuf[1] = (unsigned char*)VirtualAlloc(NULL, BIG_STREAM_BUF_SIZE+IDEAXCBC::BLOCKSIZE, MEM_COMMIT, PAGE_READWRITE);
  obuf[2] = (unsigned char*)VirtualAlloc(NULL, BIG_STREAM_BUF_SIZE+IDEAXCBC::BLOCKSIZE, MEM_COMMIT, PAGE_READWRITE);
  OVERLAPPED ovin, ovout;
  ovin.hEvent =CreateEvent(NULL, TRUE, FALSE, NULL);
  ovout.hEvent=CreateEvent(NULL, TRUE, FALSE, NULL);
  int rdbuf=0, procbuf, wrbuf;

  unsigned rdstep = BIG_STREAM_BUF_SIZE / IDEAXCBC::BLOCKSIZE * IDEAXCBC::BLOCKSIZE;
  DWORD rd, rdd, towr, wr;  bool write_pending=false;
  ULARGE_INTEGER input_pos, output_pos;
  input_pos.QuadPart = 0;
  output_pos.QuadPart = str->external_pos();
  str->close_output();
  HANDLE hOutFile = CreateFile(str->pathname, GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED | FILE_FLAG_SEQUENTIAL_SCAN, NULL);
  ovin.Offset = input_pos.LowPart;  ovin.OffsetHigh = input_pos.HighPart;
  bool read_pending = !ReadFile(hFile, obuf[rdbuf], rdstep, &rd, &ovin) && GetLastError()==ERROR_IO_PENDING;
  do
  {// wait for completing the read: 
    procbuf=rdbuf;
    if (read_pending) GetOverlappedResult(hFile, &ovin, &rd, TRUE);
    rdd=rd;
   // start new reading if not eof
    if (rdd==rdstep)
    { rdbuf = (rdbuf+1) % 3;
      input_pos.QuadPart += rdd;
      ovin.Offset = input_pos.LowPart;  ovin.OffsetHigh = input_pos.HighPart;
      read_pending = !ReadFile(hFile, obuf[rdbuf], rdstep, &rd, &ovin) && GetLastError()==ERROR_IO_PENDING;
    }
   // processing:
    md.Update(obuf[procbuf], rdd);
    unsigned offset, step=idea.BlockSize();
    unsigned char * ptr = obuf[procbuf];
    for (offset=0;  offset+step <= rdd;  offset+=step, ptr+=step)
      idea.ProcessBlock(ptr);
    if (rdd<rdstep)  // add padding
    { unsigned padding = IDEAXCBC::BLOCKSIZE-(rdd-offset);  
      memset(ptr+rdd-offset, padding, padding);
      idea.ProcessBlock(ptr);
      towr=rdd+padding;
    }
    else towr=rdd;
   // wait for completing the previous write:
    if (write_pending) 
    { GetOverlappedResult(hOutFile, &ovout, &wr, TRUE);
      output_pos.QuadPart += wr;
    }
    wrbuf=procbuf;
   // start the new write:
    ovout.Offset = output_pos.LowPart;  ovout.OffsetHigh = output_pos.HighPart;
    write_pending = !WriteFile(hOutFile, obuf[wrbuf], towr, &wr, &ovout) && GetLastError()==ERROR_IO_PENDING;
    if (!write_pending) output_pos.QuadPart += wr;
    file_offset+=rdd;  progress_set_value(file_offset, length);  
  } while (rdd==rdstep);
 // wait for completing the last write:
  if (write_pending) GetOverlappedResult(hOutFile, &ovout, &wr, TRUE);
  CloseHandle(hOutFile);
  str->open_output(NULL); // append, same filename

  CloseHandle(ovin.hEvent);
  CloseHandle(ovout.hEvent);
  VirtualFree(obuf[0], 0, MEM_RELEASE);
  VirtualFree(obuf[1], 0, MEM_RELEASE);
  VirtualFree(obuf[2], 0, MEM_RELEASE);
	md.Final(digest);
  *digest_length=md.DigestSize();
  progress_close();
}

void tFileStream::copy_encrypting_digesting(tFileStream * str, DES3CBCEncryption & idea, CryptoPP::HashModule &md, unsigned char * digest, unsigned * digest_length) const
{ unsigned length = GetFileSize(hFile, NULL);
  unsigned file_offset=0;
  if (length>10000) 
  { progress_open(0, length);
    progress_set_text("Prob��ifrov��z�ilky...");
  }
  unsigned char * obuf[3];
  obuf[0] = (unsigned char*)VirtualAlloc(NULL, BIG_STREAM_BUF_SIZE+DES3CBCEncryption.BLOCKSIZE, MEM_COMMIT, PAGE_READWRITE);
  obuf[1] = (unsigned char*)VirtualAlloc(NULL, BIG_STREAM_BUF_SIZE+DES3CBCEncryption.BLOCKSIZE, MEM_COMMIT, PAGE_READWRITE);
  obuf[2] = (unsigned char*)VirtualAlloc(NULL, BIG_STREAM_BUF_SIZE+DES3CBCEncryption.BLOCKSIZE, MEM_COMMIT, PAGE_READWRITE);
  OVERLAPPED ovin, ovout;
  ovin.hEvent =CreateEvent(NULL, TRUE, FALSE, NULL);
  ovout.hEvent=CreateEvent(NULL, TRUE, FALSE, NULL);
  int rdbuf=0, procbuf, wrbuf;

  unsigned rdstep = BIG_STREAM_BUF_SIZE / DES3CBCEncryption.BLOCKSIZE * DES3CBCEncryption.BLOCKSIZE;
  DWORD rd, rdd, towr, wr;  bool write_pending=false;
  ULARGE_INTEGER input_pos, output_pos;
  input_pos.QuadPart = 0;
  output_pos.QuadPart = str->external_pos();
  str->close_output();
  HANDLE hOutFile = CreateFile(str->pathname, GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED | FILE_FLAG_SEQUENTIAL_SCAN, NULL);
  ovin.Offset = input_pos.LowPart;  ovin.OffsetHigh = input_pos.HighPart;
  bool read_pending = !ReadFile(hFile, obuf[rdbuf], rdstep, &rd, &ovin) && GetLastError()==ERROR_IO_PENDING;
  do
  {// wait for completing the read: 
    procbuf=rdbuf;
    if (read_pending) GetOverlappedResult(hFile, &ovin, &rd, TRUE);
    rdd=rd;
   // start new reading if not eof
    if (rdd==rdstep)
    { rdbuf = (rdbuf+1) % 3;
      input_pos.QuadPart += rdd;
      ovin.Offset = input_pos.LowPart;  ovin.OffsetHigh = input_pos.HighPart;
      read_pending = !ReadFile(hFile, obuf[rdbuf], rdstep, &rd, &ovin) && GetLastError()==ERROR_IO_PENDING;
    }
   // processing:
    md.Update(obuf[procbuf], rdd);
    unsigned offset, step=idea.BlockSize();
    unsigned char * ptr = obuf[procbuf];
    for (offset=0;  offset+step <= rdd;  offset+=step, ptr+=step)
      idea.ProcessBlock(ptr);
    if (rdd<rdstep)  // add padding
    { unsigned padding = DES3CBCEncryption.BLOCKSIZE-(rdd-offset);  
      memset(ptr+rdd-offset, padding, padding);
      idea.ProcessBlock(ptr);
      towr=rdd+padding;
    }
    else towr=rdd;
   // wait for completing the previous write:
    if (write_pending) 
    { GetOverlappedResult(hOutFile, &ovout, &wr, TRUE);
      output_pos.QuadPart += wr;
    }
    wrbuf=procbuf;
   // start the new write:
    ovout.Offset = output_pos.LowPart;  ovout.OffsetHigh = output_pos.HighPart;
    write_pending = !WriteFile(hOutFile, obuf[wrbuf], towr, &wr, &ovout) && GetLastError()==ERROR_IO_PENDING;
    if (!write_pending) output_pos.QuadPart += wr;
    file_offset+=rdd;  progress_set_value(file_offset, length);  
  } while (rdd==rdstep);
 // wait for completing the last write:
  if (write_pending) GetOverlappedResult(hOutFile, &ovout, &wr, TRUE);
  CloseHandle(hOutFile);
  str->open_output(NULL); // append, same filename

  CloseHandle(ovin.hEvent);
  CloseHandle(ovout.hEvent);
  VirtualFree(obuf[0], 0, MEM_RELEASE);
  VirtualFree(obuf[1], 0, MEM_RELEASE);
  VirtualFree(obuf[2], 0, MEM_RELEASE);
	md.Final(digest);
  *digest_length=md.DigestSize();
  progress_close();
}

#else

void tFileStream::copy_encrypting_digesting(tFileStream * str, IDEAXCBCEncryption & idea, CryptoPP::HashModule &md, unsigned char * digest, unsigned * digest_length) const
{ unsigned length = GetFileSize(hFile, NULL);
  SetFilePointer(hFile, 0, NULL, FILE_BEGIN);
  unsigned file_offset=0;
  if (length>10000) 
  { progress_open(0, length);
    progress_set_text("Prob��ifrov��z�ilky...");
  }
  unsigned rdstep = bufsize / IDEAXCBCEncryption::BLOCKSIZE * IDEAXCBCEncryption::BLOCKSIZE;
  DWORD rd;
  do
  { xread(hFile, buf, rdstep, rd);
    md.Update(buf, rd);
    unsigned offset, step=idea.BlockSize();
    for (offset=0;  offset+step <= rd;  offset+=step)
      idea.ProcessBlock(buf+offset);
    if (rd<rdstep)  // add padding
    { unsigned padding = IDEAXCBCEncryption::BLOCKSIZE-(rd-offset);  
      unsigned char auxbuf[IDEAXCBCEncryption::BLOCKSIZE];
      memcpy(auxbuf, buf+offset, rd-offset);
      memset(auxbuf+rd-offset, padding, padding);
      idea.ProcessBlock(auxbuf);
      str->save_val(buf, offset);
      str->save_val(auxbuf, IDEAXCBCEncryption::BLOCKSIZE);
    }
    else
      str->save_val(buf, rd);
    file_offset+=rd;  progress_set_value(file_offset, length);  
  } while (rd==rdstep);
	md.Final(digest);
  *digest_length=md.DigestSize();
  progress_close();
}

#if 0
void tFileStream::copy_encrypting_digesting(tFileStream * str, DES3CBCEncryption & idea, CryptoPP::HashModule &md, unsigned char * digest, unsigned * digest_length) const
{ unsigned length = GetFileSize(hFile, NULL);
  SetFilePointer(hFile, 0, NULL, FILE_BEGIN);
  unsigned file_offset=0;
  if (length>10000) 
  { progress_open(0, length);
    progress_set_text("Prob��ifrov��z�ilky...");
  }
  unsigned rdstep = bufsize / DES3CBCEncryption.BLOCKSIZE * DES3CBCEncryption.BLOCKSIZE;  
  DWORD rd;
  do
  { xread(hFile, buf, rdstep, rd);
    md.Update(buf, rd);
    unsigned offset, step=idea.BlockSize();
    for (offset=0;  offset+step <= rd;  offset+=step)
      idea.ProcessBlock(buf+offset);
    if (rd<rdstep)  // add padding
    { unsigned padding = DES3CBCEncryption.BLOCKSIZE-(rd-offset);  
      unsigned char auxbuf[DES3CBCEncryption.BLOCKSIZE];
      memcpy(auxbuf, buf+offset, rd-offset);
      memset(auxbuf+rd-offset, padding, padding);
      idea.ProcessBlock(auxbuf);
      str->save_val(buf, offset);
      str->save_val(auxbuf, DES3CBCEncryption.BLOCKSIZE);
    }
    else
      str->save_val(buf, rd);
    file_offset+=rd;  progress_set_value(file_offset, length);  
  } while (rd==rdstep);
	md.Final(digest);
  *digest_length=md.DigestSize();
  progress_close();
}
#endif
#endif

unsigned tFileStream::extent(void) const
{ return GetFileSize(hFile, NULL); }
//////////////////////////////////////// memory stream ////////////////////////////////////////
tMemStream::~tMemStream(void)
{ if (allocated_buf) delete [] buf; }

bool tMemStream::open_output(unsigned extent)
{ buf = new unsigned char[extent];
  if (!buf) return false; 
  error=false;  struct_error=ERR_NO;  bufcont=extpos=0;  bufsize=extent;
  allocated_buf=true;
  return true;
}

unsigned char * tMemStream::close_output(void)
{ unsigned char * out = buf;  buf=NULL;  bufsize=0;  allocated_buf=false;
  return out;
}

bool tMemStream::open_input(unsigned char * bufIn, unsigned bufsizeIn)
{ buf = bufIn;  allocated_buf=false;
  bufsize=bufcont=bufsizeIn;
  bufpos=extpos=0;
  return true;
}

void tMemStream::close_input(void)
{ buf=NULL;  bufsize=0; }

unsigned tMemStream::load_val(void * dest, unsigned size) // dest may be NULL
{ unsigned step=bufcont-bufpos;
  if (step>size) step=size;
  if (dest) memcpy(dest, buf+bufpos, step);
  bufpos+=step;  extpos+=step;
  if (step<size) error=true;  
  return step;
}

void tMemStream::save_val(const void * src, unsigned size)
{ unsigned step = bufsize-bufcont;
  if (step > size) step=size;
  memcpy(buf+bufcont, src, step);
  bufcont += step;  extpos+=step;  size -= step;
  if (size) error=true;  
}

unsigned char tMemStream::get_advance_byte(unsigned offset)
{ if (bufpos+offset >= bufcont) return 0;  // does not match any valid tag
  return buf[bufpos+offset];
}

void tMemStream::get_digest(CryptoPP::HashModule &md, unsigned char * digest, unsigned * digest_length) const
{ md.Update(buf, bufcont);
	md.Final(digest);
  *digest_length=md.DigestSize();
}

void tMemStream::copy(tStream * str) const
{ str->save_val(buf, bufcont); }

void tMemStream::copy_encrypting_digesting(tFileStream * str, IDEAXCBCEncryption & idea, CryptoPP::HashModule &md, unsigned char * digest, unsigned * digest_length) const
{ unsigned char auxbuf[IDEAXCBCEncryption::BLOCKSIZE];
  unsigned offset, step=idea.BlockSize();
  for (offset=0;  offset+step <= bufcont;  offset+=step)
  { idea.ProcessBlock(buf+offset, auxbuf);
    str->save_val(auxbuf, step);
  }
  unsigned padding = IDEAXCBCEncryption::BLOCKSIZE-(bufcont-offset);  
  memcpy(auxbuf, buf+offset, bufcont-offset);
  memset(auxbuf+bufcont-offset, padding, padding);
  idea.ProcessBlock(auxbuf);
  str->save_val(auxbuf, IDEAXCBCEncryption::BLOCKSIZE);
  md.Update(buf, bufcont);
	md.Final(digest);
  *digest_length=md.DigestSize();
}

#if 0
void tMemStream::copy_encrypting_digesting(tFileStream * str, DES3CBCEncryption & idea, CryptoPP::HashModule &md, unsigned char * digest, unsigned * digest_length) const
{ unsigned char auxbuf[DES3CBCEncryption.BLOCKSIZE];
  unsigned offset, step=idea.BlockSize();
  for (offset=0;  offset+step <= bufcont;  offset+=step)
  { idea.ProcessBlock(buf+offset, auxbuf);
    str->save_val(auxbuf, step);
  }
  unsigned padding = DES3CBCEncryption.BLOCKSIZE-(bufcont-offset);  
  memcpy(auxbuf, buf+offset, bufcont-offset);
  memset(auxbuf+bufcont-offset, padding, padding);
  idea.ProcessBlock(auxbuf);
  str->save_val(auxbuf, DES3CBCEncryption.BLOCKSIZE);
  md.Update(buf, bufcont);
	md.Final(digest);
  *digest_length=md.DigestSize();
}
#endif
#if 0
void tMemStream::copy(tStream * str, CryptoPP::IDEA & idea) const
{ unsigned char auxbuf[CryptoPP::IDEAEncryption::BLOCKSIZE];
  unsigned offset, step=idea.BlockSize();
  for (offset=0;  offset+step <= bufcont;  offset+=step)
  { idea.ProcessBlock(buf+offset, auxbuf);
    str->save_val(auxbuf, step);
  }
  unsigned padding = CryptoPP::IDEA::BLOCKSIZE-(bufcont-offset);  
  memcpy(auxbuf, buf+offset, bufcont-offset);
  memset(auxbuf+bufcont-offset, padding, padding);
  idea.ProcessBlock(auxbuf);
  str->save_val(auxbuf, CryptoPP::IDEA::BLOCKSIZE);
}
#endif
void tMemStream::copy(tStream * str, CryptoPP::IDEAEncryption & idea) const
{ unsigned char auxbuf[CryptoPP::IDEAEncryption::BLOCKSIZE];
  unsigned offset, step=idea.BlockSize();
  for (offset=0;  offset+step <= bufcont;  offset+=step)
  { idea.ProcessBlock(buf+offset, auxbuf);
    str->save_val(auxbuf, step);
  }
  unsigned padding = CryptoPP::IDEAEncryption::BLOCKSIZE-(bufcont-offset);  
  memcpy(auxbuf, buf+offset, bufcont-offset);
  memset(auxbuf+bufcont-offset, padding, padding);
  idea.ProcessBlock(auxbuf);
  str->save_val(auxbuf, CryptoPP::IDEAEncryption::BLOCKSIZE);
}

unsigned tMemStream::extent(void) const
{ return bufcont; }
///////////////////////////////////////// base stream ////////////////////////////////////////////
void tStream::save_tag(const unsigned char * tag)
{ save_val(tag, 1);
  if ((*tag & 31) == 31) // high-tag-number form
  { do
    { tag++;  save_val(tag, 1);
    } while (*tag & 128); // bit 8 is 0 in the last octet of the tag
  }
}

bool tStream::check_tag(const unsigned char * tag, bool skip)
{ unsigned char bt;
  bt=get_advance_byte(0);
  if (*tag!=bt) return false;
  unsigned pos=1;
  if ((bt & 31) == 31) // high-tag-number form
  { do
    { bt=get_advance_byte(pos++);
      if (bt!=*(++tag)) return false;
    } while (bt & 128); // bit 8 is 0 in the last octet of the tag
  }
  if (skip) load_val(NULL, pos);  // skips the tag in the stream
  return true;
}

unsigned tag_length(const unsigned char * tag)
{ if ((*tag & 31)!=31) return 1;
  unsigned len=1;
  do
  { tag++;  len++;
  } while (*tag & 128);  // bit 8 is 0 in the last octet of the tag
  return len;
}

unsigned tStream::load_length(void) // supposes length<2^32, for indefinite length returns LENGTH_INDEFINITE 0xffffffff
{ unsigned char len1;
  load_val(&len1, 1);
  if (!(len1 & 0x80)) return len1;  // short form of the length
 // long form:
  len1 &= 0x7f;
  if (!len1) return LENGTH_INDEFINITE;
  if (len1>4) set_struct_error(ERR_LENGHT_TOO_BIG);
  unsigned length = 0;
  do
  { unsigned char len_octet;
    load_val(&len_octet, 1);
    length = 256*length + len_octet;
  } while (--len1);
  return length;
}

void tStream::save_length(unsigned length)
{ if (length<=127) save_val(&length, 1);
  else
  { int parts=0; unsigned char part[4];
    while (length)
    { part[parts++]=(unsigned char)(length % 256);
      length /= 256;
    }
    save_dir(128+parts, 1);
    while (parts>0) save_val(&part[--parts], 1);
  }
}

void tStream::load_end_of_contents(void)
{ unsigned char end_of_cont[2];
  load_val(end_of_cont, 2);
  if (end_of_cont[0] || end_of_cont[1])
    set_struct_error(ERR_MISSING_EOC);
}

unsigned length_of_length(unsigned length)
{ if (length<=127) return 1;
  unsigned parts=0; 
  while (length)
    { parts++;  length /= 256; }
  return 1+parts;
}

void tStream::start_constructed(t_bookmark * bookmark)
{ bookmark->length = load_length();
  bookmark->start_pos = external_pos();
}

bool tStream::check_constructed_end(t_bookmark * bookmark)
{ if (bookmark->length==LENGTH_INDEFINITE)
    return is_end_of_contents(); // skips it if present
  else // definite length
    return external_pos() >= bookmark->start_pos+bookmark->length;
}

t_bookmark::t_bookmark(tStream * str)
  { str->start_constructed(this); }

void stream_copy(tStream * source, tStream * dest, unsigned length, CryptoPP::HashModule * md)
{ unsigned char buf[1024];
  while (length)
  { unsigned step = length < sizeof(buf) ? length : sizeof(buf);
    source->load_val(buf, step);
    if (md) md->Update(buf, step);
    dest->save_val(buf, step);
    if (source->any_error() || dest->any_error()) break;
    length -= step;
  }
}

#if 0
bool stream_copy_decrypt(tStream * source, tStream * dest, int length, CryptoPP::HashModule * md, DES3CBCDecryption & ideadec)
// Length is supposed to be the multiply of DES3CBCEncryption.BLOCKSIZE, must truncate the padding
{ unsigned char buf[DES3CBCEncryption.BLOCKSIZE];  bool ok=true;
  unsigned orig_length = length, counter=0;
  if (length>10000) 
  { progress_open(0, length);
    progress_set_text("Prob��deifrov��z�ilky...");
  }
  while (length>0)
  { source->load_val(buf, DES3CBCEncryption.BLOCKSIZE);
    ideadec.ProcessBlock(buf); // decrypt
    length -= DES3CBCEncryption.BLOCKSIZE;
    if (length>0)
    { if (md) md->Update(buf, DES3CBCEncryption.BLOCKSIZE); // digesting the decrypted info
      dest->save_val(buf, DES3CBCEncryption.BLOCKSIZE);
    }
    else // the last block with padding
    { int padding = buf[DES3CBCEncryption.BLOCKSIZE-1];
      if (padding>DES3CBCEncryption.BLOCKSIZE) { padding=DES3CBCEncryption.BLOCKSIZE;  ok=false; } // error: minimizing damages
      if (md) md->Update(buf, DES3CBCEncryption.BLOCKSIZE-padding); // digesting the decrypted info
      dest->save_val(buf, DES3CBCEncryption.BLOCKSIZE-padding);
      for (int i=DES3CBCEncryption.BLOCKSIZE-padding;  i<DES3CBCEncryption.BLOCKSIZE;  i++)
        if (buf[i]!=padding) ok=false;
    }
    if (source->any_error() || dest->any_error()) break;
    if (!(counter++ % 10000)) progress_set_value(orig_length-length, orig_length);
  }
  progress_close();
  return ok;
}
#endif

bool stream_copy_decrypt(tStream * source, tStream * dest, int length, CryptoPP::HashModule * md, IDEAXCBCDecryption & ideadec)
// Length is supposed to be the multiply of IDEAXCBC::BLOCKSIZE, must truncate the padding
{ unsigned char buf[IDEAXCBCEncryption::BLOCKSIZE];  bool ok=true;
  unsigned orig_length = length, counter=0;
  if (length>10000) 
  { progress_open(0, length);
    progress_set_text("Prob��deifrov��z�ilky...");
  }
  while (length>0)
  { source->load_val(buf, IDEAXCBCEncryption::BLOCKSIZE);
    ideadec.ProcessBlock(buf); // decrypt
    length -= IDEAXCBCEncryption::BLOCKSIZE;
    if (length>0)
    { if (md) md->Update(buf, IDEAXCBCEncryption::BLOCKSIZE); // digesting the decrypted info
      dest->save_val(buf, IDEAXCBCEncryption::BLOCKSIZE);
    }
    else // the last block with padding
    { int padding = buf[IDEAXCBCEncryption::BLOCKSIZE-1];
      if (padding>IDEAXCBCEncryption::BLOCKSIZE) { padding=IDEAXCBCEncryption::BLOCKSIZE;  ok=false; } // error: minimizing damages
      if (md) md->Update(buf, IDEAXCBCEncryption::BLOCKSIZE-padding); // digesting the decrypted info
      dest->save_val(buf, IDEAXCBCEncryption::BLOCKSIZE-padding);
      for (int i=IDEAXCBCEncryption::BLOCKSIZE-padding;  i<IDEAXCBCEncryption::BLOCKSIZE;  i++)
        if (buf[i]!=padding) ok=false;
    }
    if (source->any_error() || dest->any_error()) break;
    if (!(counter++ % 10000)) progress_set_value(orig_length-length, orig_length);
  }
  progress_close();
  return ok;
}

bool stream_copy_decrypt(tStream * source, tStream * dest, int length, CryptoPP::HashModule * md, CryptoPP::IDEADecryption & ideadec)
// Length is supposed to be the multiply of IDEAXCBC::BLOCKSIZE, must truncate the padding
{ unsigned char buf[CryptoPP::IDEAEncryption::BLOCKSIZE];  bool ok=true;
  unsigned orig_length = length, counter=0;
  if (length>10000) 
  { progress_open(0, length);
    progress_set_text("Prob��deifrov��z�ilky...");
  }
  while (length>0)
  { source->load_val(buf, CryptoPP::IDEA::BLOCKSIZE);
    ideadec.ProcessBlock(buf); // decrypt
    length -= CryptoPP::IDEAEncryption::BLOCKSIZE;
    if (length>0)
    { if (md) md->Update(buf, CryptoPP::IDEAEncryption::BLOCKSIZE); // digesting the decrypted info
      dest->save_val(buf, CryptoPP::IDEAEncryption::BLOCKSIZE);
    }
    else // the last block with padding
    { int padding = buf[CryptoPP::IDEAEncryption::BLOCKSIZE-1];
      if (padding>CryptoPP::IDEAEncryption::BLOCKSIZE) { padding=CryptoPP::IDEAEncryption::BLOCKSIZE;  ok=false; } // error: minimizing damages
      if (md) md->Update(buf, CryptoPP::IDEAEncryption::BLOCKSIZE-padding); // digesting the decrypted info
      dest->save_val(buf, CryptoPP::IDEAEncryption::BLOCKSIZE-padding);
      for (int i=CryptoPP::IDEAEncryption::BLOCKSIZE-padding;  i<CryptoPP::IDEAEncryption::BLOCKSIZE;  i++)
        if (buf[i]!=padding) ok=false;
    }
    if (source->any_error() || dest->any_error()) break;
    if (!(counter++ % 100)) progress_set_value(orig_length-length, orig_length);
  }
  progress_close();
  return ok;
}

//////////////////////////////////////////// boolean //////////////////////////////////////////
void t_asn_boolean::save(tStream * str, const unsigned char * tag) const
{ str->save_tag(tag);
  unsigned length = der_length(false); // inner
  str->save_val(&length, 1);  // length of the value
  str->save_val((char*)&val, 1); 
}

void t_asn_boolean::load(tStream * str, const unsigned char * tag)
{ if (!str->check_tag(tag))
    str->set_struct_error(ERR_BOOLEAN_NOT_PRESENT);
  else
  { signed char len1;
    str->load_val(&len1, 1);
    if (len1!=1) str->set_struct_error(ERR_BOOLEAN_LENGHT);  // only LENGTH == 1 supported
    else
      str->load_val(&val, 1);
  }
}

unsigned t_asn_boolean::der_length(bool outer, const unsigned char * tag) const
{ return outer ? 1 + tag_length(tag) + length_of_length(1) : 1; }

//////////////////////////////////////////// integer direct //////////////////////////////////////////
unsigned t_asn_integer_direct::der_length(bool outer, const unsigned char * tag) const
{ unsigned length;
  if (value < 0x80)
  { if      (value>=-0x80    ) length=1;
    else if (value>=-0x8000  ) length=2;
    else if (value>=-0x800000) length=3;
    else                       length=4;
  }
  else // >=128
  { if      (value<0x8000  ) length=2;
    else if (value<0x800000) length=3;
    else                     length=4;
  }
  if (outer) length += tag_length(tag) + length_of_length(length);
  return length;
}

void t_asn_integer_direct::save(tStream * str, const unsigned char * tag) const
{ str->save_tag(tag);
  unsigned length = der_length(false); // inner
  str->save_val(&length, 1);  // length of the value
  for (int i=length-1;  i>=0;  i--)  // MSB first
    str->save_val(((char*)&value)+i, 1); 
}


void t_asn_integer_direct::load(tStream * str, const unsigned char * tag)
{ if (str->check_tag(tag))
  { signed char len1;
    str->load_val(&len1, 1);
    if (!len1 || len1>4) str->set_struct_error(ERR_DIRECT_INTEGER_LENGHT);  // cannot be represented in the direct integer form
    else
    { unsigned char val1;
      str->load_val(&val1, 1);
      if (val1>=128) value=-1;  else value=0;
      do
      { value=(value<<8) | val1;
        if (!--len1) break;
        str->load_val(&val1, 1);
      } while (TRUE);
    }
  }
  else str->set_struct_error(ERR_DIRECT_INTEGER_NOT_PRESENT);
}

////////////////////////////////////// indirect integer ////////////////////////////////////
void t_asn_integer_indirect::assign(const unsigned char * strng, unsigned value_length)
{ length=value_length;
  if (length)
  { value=new unsigned char[length];
    if (value==NULL) return;
    memcpy(value, strng, length);
  }
  else value=NULL;
}

void t_asn_integer_indirect::assign_reversed(const unsigned char * strng, unsigned value_length)
{ length=value_length;
  if (length)
  { value=new unsigned char[length+1];
    if (value==NULL) return;
    unsigned char * dest = value;
   // skip trailing 0s:
    while (value_length>1 && !strng[value_length-1]) { value_length--;  length--; }
   // add leading 0 if MSBit is 1:
    if (strng[value_length-1] & 0x80) { *dest=0;  dest++;  length++; }
   // copy reversed:
    while (value_length)
      *(dest++) = strng[--value_length];
  }
  else value=NULL;
}

void t_asn_integer_indirect::load(tStream * str, const unsigned char * tag)
{ if (!str->check_tag(tag))
    { str->set_struct_error(ERR_INDIRECT_INTEGER_NOT_PRESENT);  return; }
  length = str->load_length();
  if (length==LENGTH_INDEFINITE) { str->set_struct_error(ERR_INDIRECT_INTEGER_LENGHT);  return; }
  if (length)
  { value=new unsigned char[length];   
    if (value==NULL) { str->set_struct_error(ERR_NO_MEMORY);  return; }
    str->load_val(value, length);
  }
}

void t_asn_integer_indirect::save(tStream * str, const unsigned char * tag) const
{ str->save_tag(tag);
  str->save_length(length);
  str->save_val(value, length);
}

unsigned t_asn_integer_indirect::der_length(bool outer, const unsigned char * tag) const
{ return outer ? tag_length(tag) + length_of_length(length) + length : length; }

t_asn_integer_indirect::t_asn_integer_indirect(const CryptoPP::Integer & inp) : t_asn_integer(false)
{ tMemStream str;  unsigned char buf[1+3+257];  
  //CryptoPP::StringStore ss(buf, sizeof(buf));
  CryptoPP::ArraySink ss(buf, sizeof(buf));
  inp.DEREncode(ss);
  str.open_input(buf, (unsigned)ss.TotalPutLength());
  load(&str);
  str.close_input();
}

////////////////////////////////////// universal integer ////////////////////////////////////
unsigned t_asn_integer_univ::der_length(bool outer, const unsigned char * tag) const
{ 
  if (is_direct())
  { int dirlen;
    if (dirval < 0x80)
    { if      (dirval>=-0x80    ) dirlen=1;
      else if (dirval>=-0x8000  ) dirlen=2;
      else if (dirval>=-0x800000) dirlen=3;
      else                        dirlen=4;
    }
    else // >=128
    { if      (dirval<0x8000  ) dirlen=2;
      else if (dirval<0x800000) dirlen=3;
      else                      dirlen=4;
    }
    return outer ? tag_length(tag) + length_of_length(dirlen) + dirlen : dirlen; 
  }
  return outer ? tag_length(tag) + length_of_length(length) + length : length; 
}

void t_asn_integer_univ::load(tStream * str, const unsigned char * tag)
{ if (!str->check_tag(tag))
    { str->set_struct_error(ERR_INDIRECT_INTEGER_NOT_PRESENT);  return; }
  length = str->load_length();
  if (length==LENGTH_INDEFINITE) { str->set_struct_error(ERR_INDIRECT_INTEGER_LENGHT);  return; }
  if (length>0 && length<=4)  // direct
  { unsigned char val1;
    int len1 = length;
    str->load_val(&val1, 1);
    if (val1>=128) dirval=-1;  else dirval=0;
    do
    { dirval=(dirval<<8) | val1;
      if (!--len1) break;
      str->load_val(&val1, 1);
    } while (true);
  }
  else if (length)  // indirect
  { value=new unsigned char[length];   
    if (value==NULL) { str->set_struct_error(ERR_NO_MEMORY);  return; }
    str->load_val(value, length);
  }
}

void t_asn_integer_univ::save(tStream * str, const unsigned char * tag) const
{ str->save_tag(tag);
  if (is_direct())
  { unsigned length = der_length(false); // inner
    str->save_val(&length, 1);  // length of the value
    for (int i=length-1;  i>=0;  i--)  // MSB first
      str->save_val(((char*)&dirval)+i, 1); 
  }
  else
  { str->save_length(length);
    str->save_val(value, length);
  }
}

void t_asn_integer_univ::assign(const unsigned char * strng, unsigned value_length)
// Used only for the indirect values
{ length=value_length;
  if (length)
  { value=new unsigned char[length];
    if (value==NULL) return;
    memcpy(value, strng, length);
  }
}

t_asn_integer_univ::t_asn_integer_univ(const CryptoPP::Integer & inp)
{ tMemStream str;  unsigned char buf[1+3+257];  
  //CryptoPP::StringStore ss(buf, sizeof(buf));
  CryptoPP::ArraySink ss(buf, sizeof(buf));
  inp.DEREncode(ss);
  str.open_input(buf, (unsigned)ss.TotalPutLength());
  load(&str);
  str.close_input();
}

char hexdigit(int i)
{ return i<10 ? '0'+i : 'A'-10+i; }

void t_asn_integer_univ::print_hex(char * buf) const
{
  if (is_direct())
    sprintf(buf, "%X", dirval);
  else
  { for (int i=0;  i<length;  i++)
    { *buf=hexdigit(value[i] >> 4);    buf++;
      *buf=hexdigit(value[i] & 0x0f);  buf++;
    }
    *buf=0;
  }
}
////////////////////////////////////// object id ////////////////////////////////////
void t_asn_objectid::assign(const unsigned * vals, unsigned count)
// count must be >=2
{ unsigned i, pos;
  octet_cnt=1;
  for (i=2;  i<count;  i++)
  { unsigned val=vals[i];  
    do { octet_cnt++;  val/=128; }
    while (val);
  }   
  octets=new unsigned char[octet_cnt];
  if (octets==NULL) return; 
  octets[0]=(unsigned char)(40*vals[0]+vals[1]);  pos=1;
  for (i=2;  i<count;  i++)
  { unsigned val=vals[i];  
   // convert val into a sequence of bytes:
    unsigned char bytes[5];  int j=0;
    do { bytes[j++] = (unsigned char)(val%128);  val/=128; }
    while (val);
   // copy bytes to octets in revers order, with continuation flags
    while (j--)
      octets[pos++] = j ? (unsigned char)(bytes[j] | 0x80) : bytes[j];
  }   
}

bool t_asn_objectid::is(const unsigned * vals, unsigned count) const
{ unsigned i, pos;
  unsigned cnt2=1;
  for (i=2;  i<count;  i++)
  { unsigned val=vals[i];  
    do { cnt2++;  val/=128; }
    while (val);
  }   
  if (cnt2!=octet_cnt) return false;
  if (octets[0]!=40*vals[0]+vals[1]) return false;  
  pos=1;
  for (i=2;  i<count;  i++)
  { unsigned val=vals[i];  
   // convert val into a sequence of bytes:
    unsigned char bytes[5];  int j=0;
    do { bytes[j++] = (unsigned char)(val%128);  val/=128; }
    while (val);
   // copy bytes to octets in revers order, with continuation flags
    while (j--)
      if (octets[pos++] != (j ? (bytes[j] | 0x80) : bytes[j])) return false;
  }   
  return true;
}

void t_asn_objectid::load(tStream * str, const unsigned char * tag)
{ if (str->check_tag(tag))
  { octet_cnt = str->load_length();
    if (!octet_cnt || octet_cnt==LENGTH_INDEFINITE) str->set_struct_error(ERR_OBJECT_ID_LENGHT);  // cannot be represented in the direct integer form
    else
    { octets=new unsigned char[octet_cnt];
      if (octets==NULL) str->set_struct_error(ERR_NO_MEMORY);
      else str->load_val(octets, octet_cnt);
    }
  }
  else str->set_struct_error(ERR_OBJECT_ID_NOT_PRESENT);
}

void t_asn_objectid::save(tStream * str, const unsigned char * tag) const
{ str->save_tag(tag);
  str->save_length(octet_cnt);
  str->save_val(octets, octet_cnt);
}

unsigned t_asn_objectid::der_length(bool outer, const unsigned char * tag) const
{ return outer ? tag_length(tag) + length_of_length(octet_cnt) + octet_cnt : octet_cnt; }

t_asn_objectid * t_asn_objectid::copy(void) const
{ return new t_asn_objectid(octets, octet_cnt); }

t_asn_objectid::t_asn_objectid(const unsigned char * octetsIn, unsigned octet_cntIn)
{ octet_cnt=octet_cntIn;
  octets = new unsigned char[octet_cnt]; 
  if (octets) memcpy(octets, octetsIn, octet_cnt);
}

////////////////////////////////////// printable string ////////////////////////////////////
void t_asn_general_string::assign(const char * strng)
{ length=(unsigned)strlen(strng);
  if (length)
  { val=new char[length+1];   // terminating 0 only for debugging purposes
    if (val==NULL) return;
    memcpy(val, strng, length+1);
  }
  else val=NULL;
}

t_asn_general_string::t_asn_general_string(unsigned lengthIn, const char * valIn)
{ length=lengthIn;
  val=new char[length+1];   // terminating 0 only for debugging purposes
  if (val==NULL) return;
  memcpy(val, valIn, length);
  val[length]=0; // terminating 0 only for debugging purposes
}

void t_asn_general_string::get(char * buf) const
{ memcpy(buf, val, length);  buf[length]=0; }

void t_asn_printable_string::load(tStream * str, const unsigned char * tag)
{ if (str->check_tag(tag))
  { length = str->load_length();
    if (length)
      if (length==LENGTH_INDEFINITE) str->set_struct_error(ERR_PRINTABLE_STRING_LENGHT);  // cannot be represented in the direct integer form
      else
      { val=new char[length+1];
        if (val==NULL) { str->set_struct_error(ERR_NO_MEMORY);  return; }
        str->load_val(val, length);
        val[length]=0;  // terminating 0 only for debugging purposes
      }
  }
  else str->set_struct_error(ERR_PRINTABLE_STRING_NOT_PRESENT);
}

void t_asn_printable_string::save(tStream * str, const unsigned char * tag) const
{ str->save_tag(tag);
  str->save_length(length);
  str->save_val(val, length);
}

unsigned t_asn_printable_string::der_length(bool outer, const unsigned char * tag) const
{ return outer ? tag_length(tag) + length_of_length(length) + length : length; }

t_asn_general_string * t_asn_printable_string::copy(void) const
{ return new t_asn_printable_string(length, val); }

////////////////////////////////////// ia5 string ////////////////////////////////////
void t_asn_ia5_string::load(tStream * str, const unsigned char * tag)
{ if (str->check_tag(tag))
  { length = str->load_length();
    if (length)
      if (length==LENGTH_INDEFINITE) str->set_struct_error(ERR_IA5_STRING_LENGHT);  
      else
      { val=new char[length+1];   
        if (val==NULL) { str->set_struct_error(ERR_NO_MEMORY);  return; }
        str->load_val(val, length);
        val[length]=0;  // terminating 0 only for debugging purposes
      }
  }
  else str->set_struct_error(ERR_IA5_STRING_NOT_PRESENT);
}

void t_asn_ia5_string::save(tStream * str, const unsigned char * tag) const
{ str->save_tag(tag);
  str->save_length(length);
  str->save_val(val, length);
}

unsigned t_asn_ia5_string::der_length(bool outer, const unsigned char * tag) const
{ return outer ? tag_length(tag) + length_of_length(length) + length : length; }

t_asn_ia5_string * t_asn_ia5_string::copy(void) const
{ return new t_asn_ia5_string(length, val); }

t_asn_ia5_string::t_asn_ia5_string(unsigned lengthIn, const char * valIn)
{ length=lengthIn;
  val=new char[length+1];   // terminating 0 only for debugging purposes
  if (val==NULL) return;
  memcpy(val, valIn, length);
  val[length]=0; // terminating 0 only for debugging purposes
}

void t_asn_ia5_string::assign(const char * strng)
{ length=(unsigned)strlen(strng);
  if (length)
  { val=new char[length+1];   // terminating 0 only for debugging purposes
    if (val==NULL) return;
    memcpy(val, strng, length+1);
  }
  else val=NULL;
}

////////////////////////////////////// octet string ////////////////////////////////////
void t_asn_octet_string::assign(const unsigned char * strng, unsigned value_length)
{ length=value_length;
  if (length)
  { val=new unsigned char[length];
    if (val==NULL) return;
    memcpy(val, strng, length);
  }
  else val=NULL;
}

void t_asn_octet_string::load(tStream * str, const unsigned char * tag)
{ if (!str->check_tag(tag))
    { str->set_struct_error(ERR_OCTET_STRING_NOT_PRESENT);  return; }
  length = str->load_length();
  if (length==LENGTH_INDEFINITE) { str->set_struct_error(ERR_OCTET_STRING_LENGHT);  return; }
  if (length)
  { val=new unsigned char[length];   
    if (val==NULL) { str->set_struct_error(ERR_NO_MEMORY);  return; }
    str->load_val(val, length);
  }
}

void t_asn_octet_string::save(tStream * str, const unsigned char * tag) const
{ str->save_tag(tag);
  str->save_length(length);
  str->save_val(val, length);
}

unsigned t_asn_octet_string::der_length(bool outer, const unsigned char * tag) const
{ return outer ? tag_length(tag) + length_of_length(length) + length : length; }

t_asn_octet_string * t_asn_octet_string::copy(void) const
{ return new t_asn_octet_string(val, length); }


////////////////////////////////////// UTCTime ////////////////////////////////////
int last_day(int wMonth, int wYear)
{ while (wMonth>12) { wMonth-=12;  wYear++; }
  if (wMonth==4 || wMonth==6 || wMonth==9 || wMonth==11) return 30;
  if (wMonth==2)
  { if (wYear % 4)   return 28;
    if (wYear % 100) return 29;
    if (wYear % 400) return 28;
    return 29;
  }
  return 31;
}

void t_asn_UTCTime::str2systime(bool generalized)
{ int pos;
  if (generalized)
  { systime.wYear   = 1000*(val[0]-'0')+100*(val[1]-'0')+10*(val[2]-'0')+(val[3]-'0');
    pos=4;
  }
  else
  { systime.wYear   = 10*(val[0]-'0')+(val[1]-'0');
    systime.wYear += (systime.wYear >=90) ? 1900 : 2000;
    pos=2;
  }
  systime.wMonth  = 10*(val[pos]-'0')+(val[pos+1]-'0');  pos+=2;
  systime.wDay    = 10*(val[pos]-'0')+(val[pos+1]-'0');  pos+=2;
  systime.wHour   = 10*(val[pos]-'0')+(val[pos+1]-'0');  pos+=2;
  systime.wMinute = 10*(val[pos]-'0')+(val[pos+1]-'0');  pos+=2;
  if (val[pos]>='0' && val[pos]<='9')
  { systime.wSecond = 10*(val[pos]-'0')+(val[pos+1]-'0');
    pos+=2;
  }
  else 
    systime.wSecond = 0;
  systime.wMilliseconds = 0;
  if (generalized)
    if (val[pos]=='.' || val[pos]==',')
    { do
        pos++;  // should read
      while (val[pos]>='0' && val[pos]<='9');
    }
 // GTM offset:
  if (val[pos]=='Z')  // must convert GMT to local system time
  {
#ifdef WINS
	  TIME_ZONE_INFORMATION tzi;
    if (GetTimeZoneInformation(&tzi)==TIME_ZONE_ID_DAYLIGHT) tzi.Bias+=tzi.DaylightBias;
	  int bias=tzi.Bias;
#else
    int bias=systime.bias; /* No seconds; sorry */
#endif
    if (bias > 0)
    { int moff = bias % 60;
      int hoff = bias / 60;
      if (systime.wMinute>=moff) systime.wMinute-=moff;
      else
      { systime.wMinute = systime.wMinute+60-moff;
        hoff++;
      }
      if (systime.wHour>=hoff) systime.wHour-=hoff;
      else
      { systime.wHour = systime.wHour+24-hoff;
        systime.wDay--;
        if (!systime.wDay)
        { systime.wMonth--;
          if (!systime.wMonth)
          { systime.wMonth=12;
            systime.wYear--;
          }
          systime.wDay = last_day(systime.wMonth, systime.wYear);
        }
      }
    }
    else if (bias < 0)
    { int moff = -bias % 60;
      int hoff = -bias / 60;
      systime.wMinute+=moff;
      if (systime.wMinute>=60)
      { systime.wMinute-=60;
        hoff++;
      }
      systime.wHour+=hoff;
      if (systime.wHour>=24)
      { systime.wHour-=24;
        systime.wDay++;
        if (systime.wDay > last_day(systime.wMonth, systime.wYear))
        { systime.wDay=1;
          systime.wMonth++;
          if (systime.wMonth>12)
          { systime.wMonth=1;
            systime.wYear++;
          }
        }
      }
    }
  } 
#ifdef STOP
  if (val[pos]=='+' || val[pos]=='-')
  { int hoff = 10*(val[pos+1]-'0')+(val[pos+2]-'0');
    int moff = 10*(val[pos+3]-'0')+(val[pos+4]-'0');
    if (val[pos]=='+')
    { systime.wMinute+=moff;
      if (systime.wMinute>=60)
      { systime.wMinute-=60;
        hoff++;
      }
      systime.wHour+=hoff;
      if (systime.wHour>=24)
      { systime.wHour-=24;
        systime.wDay++;
        if (systime.wDay > last_day(systime.wMonth, systime.wYear))
        { systime.wDay=1;
          systime.wMonth++;
          if (systime.wMonth>12)
          { systime.wMonth=1;
            systime.wYear++;
          }
        }
      }
    }
    else if (val[pos]=='-')
    { if (systime.wMinute>=moff) systime.wMinute-=moff;
      else
      { systime.wMinute = systime.wMinute+60-moff;
        hoff++;
      }
      if (systime.wHour>=hoff) systime.wHour-=hoff;
      else
      { systime.wHour = systime.wHour+24-hoff;
        systime.wDay--;
        if (!systime.wDay)
        { systime.wMonth--;
          if (!systime.wMonth)
          { systime.wMonth=12;
            systime.wYear--;
          }
          systime.wDay = last_day(systime.wMonth, systime.wYear);
        }
      }
    }
  }
#endif
}

void t_asn_UTCTime::assign(const char * strng)
{ length=(unsigned)strlen(strng);
  if (length)
  { val=new char[length+1];  // terminating 0 for debugging purposes, and for copying!
    if (val==NULL) return;
    memcpy(val, strng, length+1);
    str2systime();
  }
}

void t_asn_UTCTime::load(tStream * str, const unsigned char * tag)
// When tag is tag_universal_utctime then tag_universal_utctime2 is allowed too (1.CA)
{ bool generalized;
  if (str->check_tag(tag))
    generalized=false;
  else if (tag==tag_universal_utctime && str->check_tag(tag_universal_utctime2))
    generalized=true;
  else
    { str->set_struct_error(ERR_UTCTIME_NOT_PRESENT);  return; }
  length = str->load_length();
  if (length<11 || length >17 || length==LENGTH_INDEFINITE)
    { str->set_struct_error(ERR_UTCTIME_LENGHT);  return; }
  val=new char[length+1];   
  if (val==NULL) { str->set_struct_error(ERR_NO_MEMORY);  return; }
  str->load_val(val, length);
  val[length]=0;  // terminating 0 only for debugging purposes
  str2systime(generalized);
}

void t_asn_UTCTime::save(tStream * str, const unsigned char * tag) const
{ str->save_tag(tag);
  str->save_length(length);
  str->save_val(val, length);
}

unsigned t_asn_UTCTime::der_length(bool outer, const unsigned char * tag) const
{ return outer ? tag_length(tag) + length_of_length(length) + length : length; }

t_asn_UTCTime::t_asn_UTCTime(const SYSTEMTIME * st)
{ systime=*st;
  length=17;
  val=new char[length+1];
  if (val)
    systemtime2UTC(systime, val);
}

void t_asn_UTCTime::get_readable_time(char * buf) const
{ sprintf(buf, "%u.%u.%u  %02u:%02u:%02u", systime.wDay, systime.wMonth, systime.wYear, systime.wHour, systime.wMinute, systime.wSecond); }
////////////////////////////////////// bit string ////////////////////////////////////
void t_asn_bit_string::assign(const unsigned char * strng, unsigned bitlengthIn)  // supposes strng is padded by 0s
{ bitlength=bitlengthIn;
  if (bitlength)
  { unsigned octet_cnt = (bitlength+7)/8;
    val=new unsigned char[octet_cnt];
    if (val==NULL) return;
    memcpy(val, strng, octet_cnt);
  }
  else val=NULL;
}

void t_asn_bit_string::load(tStream * str, const unsigned char * tag)
{ if (str->check_tag(tag))
  { unsigned octet_length = str->load_length();
    if (!octet_length || octet_length==LENGTH_INDEFINITE) str->set_struct_error(ERR_BIT_STRING_LENGHT);  // cannot be represented in the direct integer form
    else
    { unsigned char padding; 
      str->load_val(&padding, 1);
      octet_length--;
      if (octet_length)
      { val=new unsigned char[octet_length];
        if (val==NULL) { str->set_struct_error(ERR_NO_MEMORY);  return; }
        str->load_val(val, octet_length);
        bitlength = 8*octet_length - padding;
      }
      else bitlength = 0;
    }
  }
  else str->set_struct_error(ERR_BIT_STRING_NOT_PRESENT);
}

void t_asn_bit_string::save(tStream * str, const unsigned char * tag) const
{ str->save_tag(tag);
  str->save_length(der_length(false));
  unsigned octet_cnt = (bitlength+7)/8;
  str->save_dir(octet_cnt*8-bitlength, 1); // padding bits
  str->save_val(val, octet_cnt);
}

unsigned t_asn_bit_string::der_length(bool outer, const unsigned char * tag) const
{ unsigned total_length = 1+(bitlength+7)/8;
  if (outer) total_length += tag_length(tag) + length_of_length(total_length); 
  return total_length;
}

////////////////////////////////////// Name //////////////////////////////////////////
// t_asn_AVAssert: only single assertion in the set supported!

t_asn_AVAssert::~t_asn_AVAssert(void)
{ if (attribute_type ) delete attribute_type;
  if (attribute_value) 
    if (is_printable)
      delete (t_asn_printable_string *)attribute_value;
    else
      delete (t_asn_ia5_string       *)attribute_value;
  if (next)            delete next;
}

void t_asn_AVAssert::load(tStream * str)
{ if (!str->check_tag(tag_universal_constructed_sequence))
    { str->set_struct_error(ERR_SEQUENCE_NOT_PRESENT);  return; }
  str->load_length();  // ignored
  attribute_type = new t_asn_objectid;
  if (!attribute_type) { str->set_struct_error(ERR_NO_MEMORY);  return; }
  attribute_type->load(str);
  if (str->any_error()) return;
 // load the attribute value
  if (str->check_tag(tag_universal_printable_string, false))
  { t_asn_printable_string * pstrng = new t_asn_printable_string;
    if (!pstrng) { str->set_struct_error(ERR_NO_MEMORY);  return; }
    attribute_value=pstrng;
    pstrng->load(str);
    is_printable=true;
  }
  else if (str->check_tag(tag_universal_strange_string, false))
  { t_asn_printable_string * pstrng = new t_asn_printable_string;
    if (!pstrng) { str->set_struct_error(ERR_NO_MEMORY);  return; }
    attribute_value=pstrng;
    pstrng->load(str, tag_universal_strange_string);
    is_printable=true;
  }
  else if (str->check_tag(tag_universal_ia5_string, false))
  { t_asn_ia5_string * pstrng = new t_asn_ia5_string;
    if (!pstrng) { str->set_struct_error(ERR_NO_MEMORY);  return; }
    attribute_value=pstrng;
    pstrng->load(str);
  }
  else str->set_struct_error(ERR_UNSUPPORTED_ATTRIBUTE);
}

void t_asn_AVAssert::save(tStream * str) const
{ str->save_tag(tag_universal_constructed_sequence);
  str->save_length(der_length(false));
 // save fields:
  attribute_type->save(str);
  if (is_printable)
    ((t_asn_printable_string *)attribute_value)->save(str);
  else
    ((t_asn_ia5_string       *)attribute_value)->save(str);
}

unsigned t_asn_AVAssert::der_length(bool outer) const
{ unsigned total_length;
  total_length = attribute_type->der_length(true) + (is_printable ?
    ((t_asn_printable_string *)attribute_value)->der_length(true) :
    ((t_asn_ia5_string       *)attribute_value)->der_length(true));
  if (outer) total_length += tag_length(tag_universal_constructed_sequence) + length_of_length(total_length);
  return total_length;
}

t_asn_AVAssert * t_asn_AVAssert::copy(void) const
{ return new t_asn_AVAssert(attribute_type->copy(), 
    is_printable ? (void*)((t_asn_printable_string *)attribute_value)->copy() : 
                   (void*)((t_asn_ia5_string       *)attribute_value)->copy(),
    is_printable); 
}

t_asn_Name::~t_asn_Name(void)
{ delete AVAssert;
  if (next) delete next;
}

void t_asn_Name::load(tStream * str)
{ load_set_of(str, &AVAssert); }

void t_asn_Name::save(tStream * str) const
{ save_set_of(str, AVAssert); }

unsigned t_asn_Name::der_length(bool outer) const  // not implemented for outer==false
{ return der_length_set(AVAssert); }

t_asn_Name * t_asn_Name::copy(void) const
{ t_asn_Name * copyName = new t_asn_Name;
  if (!copyName) return NULL;
  copyName->AVAssert = AVAssert->copy();
  return copyName->AVAssert ? copyName : NULL;
}

t_asn_Name * t_asn_Name::copy_seq(void) const
{ t_asn_Name * copyName = copy();
  if (next) 
  { copyName->next = next->copy_seq();
    if (!copyName->next) { delete copyName;  return NULL; }
  }
  return copyName;
}

bool t_asn_Name::get_readableRDN(char * & buf, unsigned & bufsize) const
{ char * local = new char[AVAssert->der_length(false)+10];
 // attribute type:
  if      (AVAssert->attribute_type->is(attr_type_country_name,     attr_type_length))
    strcpy(local, "C");
  else if (AVAssert->attribute_type->is(attr_type_organiz_name,     attr_type_length))
    strcpy(local, "O");
  else if (AVAssert->attribute_type->is(attr_type_org_unit_name,    attr_type_length))
    strcpy(local, "OU");
  else if (AVAssert->attribute_type->is(attr_type_common_unit_name, attr_type_length))
    strcpy(local, RDNATTR_IDENT);
  else if (AVAssert->attribute_type->is(attr_type_given_name,       attr_type_length))
    strcpy(local, RDNATTR_FISTNAME);
  else if (AVAssert->attribute_type->is(attr_type_initials,         attr_type_length))
    strcpy(local, RDNATTR_MIDDLENAME);
  else if (AVAssert->attribute_type->is(attr_type_surname,          attr_type_length))
    strcpy(local, RDNATTR_LASTNAME);
  else if (AVAssert->attribute_type->is(attr_type_domain_comp_name, attr_type_length_DC))
    strcpy(local, "DC");
  else if (AVAssert->attribute_type->is(attr_type_email_name, attr_type_length_email))
    strcpy(local, "E");
  else
    strcpy(local, "?");
 // atribute value:
  unsigned len=(unsigned)strlen(local);  // unsigned because it will be compared to bufsize
  local[len++]='=';  local[len++]='\"';
  if (AVAssert->is_printable)
    ((t_asn_printable_string *)AVAssert->attribute_value)->get(local+len);
  else
    ((t_asn_ia5_string       *)AVAssert->attribute_value)->get(local+len);
  len+=(unsigned)strlen(local+len);
  local[len++]='\"';  local[len]=0;
  if (len>=bufsize)
    { delete [] local;  *buf=0;  return false; }
  strcpy(buf, local);  bufsize-=len;  buf+=len; 
  delete [] local;
  return true;
}

void get_common_name(const t_asn_Name * Name, char * buf)
// Extracts the common name from Name RDN and stores it to buf
{ while (Name && !Name->AVAssert->attribute_type->is(attr_type_common_unit_name, attr_type_length))
    Name=Name->next;
  if (!Name) *buf=0;
  else
    if (Name->AVAssert->is_printable)
      ((t_asn_printable_string *)Name->AVAssert->attribute_value)->get(buf);
    else
      ((t_asn_ia5_string       *)Name->AVAssert->attribute_value)->get(buf);
}

///////////////////////////////////// CRI atributes /////////////////////////////////////////////
// auxiliary type allowing the sequencing of ia5 string type
t_asn_cri_atribute_value::~t_asn_cri_atribute_value(void)
{ if (val)  delete val;
  if (ExtensionReq) delete ExtensionReq;
  if (next) delete next;
}

void t_asn_cri_atribute_value::load(tStream * str)
{ if (str->check_tag(tag_universal_constructed_sequence, false))
    load_sequence_of(str, &ExtensionReq);
  else
  { val = new t_asn_ia5_string;
    if (!val) { str->set_struct_error(ERR_NO_MEMORY);  return; }
    val->load(str);
  }
}

void t_asn_cri_atribute_value::save(tStream * str) const
{ if (val) val->save(str); 
  else if (ExtensionReq) save_sequence_of(str, ExtensionReq);
}

unsigned t_asn_cri_atribute_value::der_length(bool outer) const
// Not exact
{ return val ? val->der_length(outer) : ExtensionReq ? (outer ? der_length_sequence(ExtensionReq) : der_length_sequence(ExtensionReq)-4) : 0; }

///////////////
t_asn_cri_atribute::~t_asn_cri_atribute(void)
{ if (type)             delete type;
  if (attribute_values) delete attribute_values;
  if (next)             delete next;
}

void t_asn_cri_atribute::load(tStream * str)
{ if (!str->check_tag(tag_universal_constructed_sequence))
    { str->set_struct_error(ERR_SEQUENCE_NOT_PRESENT);  return; }
  str->load_length();  // ignored
 // load fields: type
  type = new t_asn_objectid;  if (!type) { str->set_struct_error(ERR_NO_MEMORY);  return; }
  type->load(str);
  if (str->any_error()) return;
 // load the attribute values:
#ifdef CNB_ERROR
  attribute_values = new t_asn_cri_atribute_value;  if (!attribute_values) { str->set_struct_error(ERR_NO_MEMORY);  return; }
  attribute_values->load(str);
#else
  load_set_of(str, &attribute_values);
#endif
}

void t_asn_cri_atribute::save(tStream * str) const
{ str->save_tag(tag_universal_constructed_sequence);
  str->save_length(der_length(false));
 // save fields:
  type->save(str);
#ifdef CNB_ERROR
  attribute_values->save(str);
#else
  save_set_of(str, attribute_values);
#endif
}

unsigned t_asn_cri_atribute::der_length(bool outer) const
{ unsigned total_length = type->der_length(true) + 
#ifdef CNB_ERROR
    attribute_values->der_length(true);
#else
    der_length_set(attribute_values);
#endif
  if (outer) total_length += tag_length(tag_universal_constructed_sequence) + length_of_length(total_length);
  return total_length;
}

////////////////////////////////////// general atributes //////////////////////////////////////////
t_asn_attribute::~t_asn_attribute(void)
{ if (attribute_type ) delete attribute_type;
  if (attribute_value) 
    switch (value_tag)
    { case 6:  delete (t_asn_objectid        *)attribute_value;  break;
      case 4:  delete (t_asn_octet_string    *)attribute_value;  break;
      case 23: delete (t_asn_UTCTime         *)attribute_value;  break;
      case 19: delete (t_asn_printable_string*)attribute_value;  break;
    }
  if (next)            delete next;
}

void t_asn_attribute::load(tStream * str)
{ if (!str->check_tag(tag_universal_constructed_sequence))
    { str->set_struct_error(ERR_SEQUENCE_NOT_PRESENT);  return; }
  str->load_length();  // ignored
  attribute_type = new t_asn_objectid;
  if (!attribute_type) { str->set_struct_error(ERR_NO_MEMORY);  return; }
  attribute_type->load(str);
  if (str->any_error()) return;
 // load the attribute value
  if (str->check_tag(tag_universal_constructed_set))  // SET encapsulation silently removed
    str->load_length();  // ignored
  if (str->check_tag(tag_universal_objectid, false))
  { t_asn_objectid * pstrng = new t_asn_objectid;
    if (!pstrng) { str->set_struct_error(ERR_NO_MEMORY);  return; }
    attribute_value=pstrng;  value_tag=6;
    pstrng->load(str);
  }
  else if (str->check_tag(tag_universal_octet_string, false))
  { t_asn_octet_string * pstrng = new t_asn_octet_string;
    if (!pstrng) { str->set_struct_error(ERR_NO_MEMORY);  return; }
    attribute_value=pstrng;  value_tag=4;
    pstrng->load(str);
  }
  else if (str->check_tag(tag_universal_utctime, false))
  { t_asn_UTCTime * pstrng = new t_asn_UTCTime;
    if (!pstrng) { str->set_struct_error(ERR_NO_MEMORY);  return; }
    attribute_value=pstrng;  value_tag=23;
    pstrng->load(str);
  }
  else if (str->check_tag(tag_universal_printable_string, false))
  { t_asn_printable_string * pstrng = new t_asn_printable_string;
    if (!pstrng) { str->set_struct_error(ERR_NO_MEMORY);  return; }
    attribute_value=pstrng;  value_tag=19;
    pstrng->load(str);
  }
  else str->set_struct_error(ERR_UNSUPPORTED_ATTRIBUTE);
}

void t_asn_attribute::save(tStream * str) const
{ str->save_tag(tag_universal_constructed_sequence);
  str->save_length(der_length(false));
 // save fields:
  attribute_type->save(str);
  str->save_tag(tag_universal_constructed_set);  // SET encapsulation
  unsigned val_length;
  switch (value_tag)
  { case 6:  val_length = ((t_asn_objectid        *)attribute_value)->der_length(true);  break;
    case 4:  val_length = ((t_asn_octet_string    *)attribute_value)->der_length(true);  break;
    case 23: val_length = ((t_asn_UTCTime         *)attribute_value)->der_length(true);  break;
    case 19: val_length = ((t_asn_printable_string*)attribute_value)->der_length(true);  break;
    default: val_length = 0;  break;
  }
  str->save_length(val_length);
  switch (value_tag)
  { case 6:  ((t_asn_objectid        *)attribute_value)->save(str);  break;
    case 4:  ((t_asn_octet_string    *)attribute_value)->save(str);  break;
    case 23: ((t_asn_UTCTime         *)attribute_value)->save(str);  break;
    case 19: ((t_asn_printable_string*)attribute_value)->save(str);  break;
  }
}

unsigned t_asn_attribute::der_length(bool outer) const
{ unsigned total_length = attribute_type->der_length(true);
  unsigned val_length;
  switch (value_tag)
  { case 6:  val_length = ((t_asn_objectid        *)attribute_value)->der_length(true);  break;
    case 4:  val_length = ((t_asn_octet_string    *)attribute_value)->der_length(true);  break;
    case 23: val_length = ((t_asn_UTCTime         *)attribute_value)->der_length(true);  break;
    case 19: val_length = ((t_asn_printable_string*)attribute_value)->der_length(true);  break;
    default: val_length = 0;  break;
  }
 // add the SET encapsulation:
  val_length += tag_length(tag_universal_constructed_set) + length_of_length(val_length);
  total_length += val_length;
  if (outer) total_length += tag_length(tag_universal_constructed_sequence) + length_of_length(total_length);
  return total_length;
}

t_asn_attribute * t_asn_attribute::find(const unsigned * object_id, unsigned object_id_length)
{ if (this==NULL) return NULL;
  if (attribute_type->is(object_id, object_id_length)) return this;
  if (!next) return NULL;
  return next->find(object_id, object_id_length);
}

////////////////////////////////// t_asn_Validity ///////////////////////////////////////////////////////////////
int compare_systime(const SYSTEMTIME & tm1, const SYSTEMTIME & tm2)
{ if (tm1.wYear>tm2.wYear) return  1;
  if (tm1.wYear<tm2.wYear) return -1;
  if (tm1.wMonth>tm2.wMonth) return  1;
  if (tm1.wMonth<tm2.wMonth) return -1;
  if (tm1.wDay>tm2.wDay) return  1;
  if (tm1.wDay<tm2.wDay) return -1;
  if (tm1.wHour>tm2.wHour) return  1;
  if (tm1.wHour<tm2.wHour) return -1;
  if (tm1.wMinute>tm2.wMinute) return  1;
  if (tm1.wMinute<tm2.wMinute) return -1;
  if (tm1.wSecond>tm2.wSecond) return  1;
  if (tm1.wSecond<tm2.wSecond) return -1;
  if (tm1.wMilliseconds>tm2.wMilliseconds) return  1;
  if (tm1.wMilliseconds<tm2.wMilliseconds) return -1;
  return 0;
}

void systemtime_add(SYSTEMTIME & tm, int years, int months, int days, int hours, int minutes, int seconds)
{ tm.wSecond+=seconds;
  while (tm.wSecond>=60) { tm.wSecond-=60;  minutes++; }
  tm.wMinute+=minutes;
  while (tm.wMinute>=60) { tm.wMinute-=60;  hours++;   }
  tm.wHour+=hours;
  while (tm.wHour>=24)   { tm.wHour  -=24;  days++;    }
  tm.wDay+=days;
  while (tm.wDay>last_day(tm.wMonth, tm.wYear)) { tm.wDay-=last_day(tm.wMonth, tm.wYear);  tm.wMonth++; }
  tm.wMonth+=months;
  while (tm.wMonth>12) { tm.wMonth-=12;  tm.wYear++; }
  tm.wYear+=years;
}

void systemtime_sub(SYSTEMTIME & tm, int years, int months, int days, int hours, int minutes, int seconds)
{ while (tm.wSecond<seconds) { tm.wSecond+=60;  minutes++; }
  tm.wSecond-=seconds;
  while (tm.wMinute<minutes) { tm.wMinute+=60;  hours++;   }
  tm.wMinute-=minutes;
  while (tm.wHour  <hours)   { tm.wHour  +=24;  days++;    }
  tm.wHour-=hours;
  while (tm.wDay  <=days)    
  { if (!--tm.wMonth) { tm.wMonth=12;  tm.wYear++; }
    tm.wDay+=last_day(tm.wMonth, tm.wYear);   
  }
  tm.wDay-=days;   
  while (tm.wMonth<=months)  { tm.wMonth +=12;  years++; }
  tm.wMonth-=months;
  tm.wYear-=years;
}

void systemtime2UTC(const SYSTEMTIME & SysTm, char * utc_time)
{
#ifdef WINS
	TIME_ZONE_INFORMATION tzi;
  if (GetTimeZoneInformation(&tzi)==TIME_ZONE_ID_DAYLIGHT) tzi.Bias+=tzi.DaylightBias;
  unsigned absbias = tzi.Bias < 0 ? -tzi.Bias : tzi.Bias;
  char sgn=tzi.Bias <= 0 ? '+' : '-';
#else
  unsigned absbias = abs(SysTm.bias);
  char sgn=SysTm.bias <= 0 ? '+' : '-';
#endif

#ifdef STOP   // either MSIE or Layman's guide is wrong in it
  sprintf(utc_time, "%02u%02u%02u%02u%02u%02u%c%02u%02u", SysTm.wYear % 100, SysTm.wMonth, SysTm.wDay, SysTm.wHour, SysTm.wMinute, SysTm.wSecond, 
    sgn, absbias / 60, absbias % 60);
#else   // Z-time
  SYSTEMTIME gmt = SysTm;
  if (sgn=='+') systemtime_sub(gmt, 0, 0, 0, absbias / 60, absbias % 60, 0);
  else          systemtime_add(gmt, 0, 0, 0, absbias / 60, absbias % 60, 0);
  sprintf(utc_time, "%02u%02u%02u%02u%02u%02uZ", gmt.wYear % 100, gmt.wMonth, gmt.wDay, gmt.wHour, gmt.wMinute, gmt.wSecond);
#endif
}

void systemtime2rdbl(const SYSTEMTIME & SysTm, char * rdbl_time)
{ sprintf(rdbl_time, "%u.%u.%u  %02u:%02u:%02u", SysTm.wDay, SysTm.wMonth, SysTm.wYear, SysTm.wHour, SysTm.wMinute, SysTm.wSecond); }

const char * current_UTC(void)
{ static char utc_time[18];  // must be static, because it is returned
  SYSTEMTIME CurrentTime;  GetLocalTime(&CurrentTime);
  systemtime2UTC(CurrentTime, utc_time);
  return utc_time;
}

void t_asn_Validity::load(tStream * str)
{ if (!str->check_tag(tag_universal_constructed_sequence))
    { str->set_struct_error(ERR_SEQUENCE_NOT_PRESENT);  return; }
  str->load_length();  // ignored
  start = new t_asn_UTCTime;  if (!start) { str->set_struct_error(ERR_NO_MEMORY);  return; }
  start->load(str);           if (str->any_error()) return;
  end   = new t_asn_UTCTime;  if (!end  ) { str->set_struct_error(ERR_NO_MEMORY);  return; }
  end  ->load(str);           
}

void t_asn_Validity::save(tStream * str) const
{ str->save_tag(tag_universal_constructed_sequence);
  str->save_length(der_length(false));
 // save fields:
  start->save(str);
  end->save(str);
}

unsigned t_asn_Validity::der_length(bool outer) const
{ unsigned total_length = start->der_length(true) + end->der_length(true);
  if (outer) total_length += tag_length(tag_universal_constructed_sequence) + length_of_length(total_length);
  return total_length;
}

bool t_asn_Validity::set_dates(const char * start_string, const char * end_string)
{ start = new t_asn_UTCTime(start_string);
  end   = new t_asn_UTCTime(end_string);
  return start && end;
}

bool t_asn_Validity::is_valid_now(const SYSTEMTIME * reference_time) const
// Checks the valitidy in the reference_time, current time used if reference_time not specified
{ SYSTEMTIME SystemTime;  
  if (!reference_time) { GetLocalTime(&SystemTime);  reference_time=&SystemTime; }
  return compare_systime(*start->get_system_time(), *reference_time) <= 0 && compare_systime(*reference_time, *end->get_system_time()) <= 0;
}

t_asn_Validity * t_asn_Validity::copy(void) const
{ t_asn_Validity * val = new t_asn_Validity;
  if (val)
  { if (val->set_dates(start->get_time_string(), end->get_time_string()))
      return val;
    delete val;
  }
  return NULL;
}
///////////////////////////////// t_asn_algorithm_identifier /////////////////////////////////////////////////////
t_algors t_asn_algorithm_identifier::algorithm_type(void)
{ if (algorithm->same(object_id_ripemd160)) return ALG_RIPEMD160;
  if (algorithm->same(object_id_ripemd128)) return ALG_RIPEMD128;
  if (algorithm->same(object_id_sha      )) return ALG_SHA;
  if (algorithm->same(object_id_rsaripemd160)) return ALG_RSARIPEMD160;
  if (algorithm->same(object_id_rsaripemd160)) return ALG_RSARIPEMD128;
  if (algorithm->same(object_id_rsasha      )) return ALG_RSASHA;
  if (algorithm->same(object_id_ideaxex3    )) return ALG_IDEAXEX;
  if (algorithm->same(object_id_des3cbc     )) return ALG_DES3CBC;
  return ALG_UNDEF;
}

const unsigned char * t_asn_algorithm_identifier::get_IDEAXEX_IV(void) const
{ if (!octet_parameters) return NULL;
  if (octet_parameters->get_length() != IDEAXCBCEncryption::BLOCKSIZE) return NULL;
  return octet_parameters->get_val();
}

bool t_asn_algorithm_identifier::set_IV_idea(const unsigned char * IV)
{ octet_parameters = new t_asn_octet_string(IV, IDEAXCBCEncryption::BLOCKSIZE); 
  return octet_parameters!=NULL;
}

#if 0
const unsigned char * t_asn_algorithm_identifier::get_DESXEX_IV(void) const
{ if (!octet_parameters) return NULL;
  if (octet_parameters->get_length() != DES3CBCEncryption.BLOCKSIZE) return NULL;
  return octet_parameters->get_val();
}

bool t_asn_algorithm_identifier::set_IV_des(const unsigned char * IV)
{ octet_parameters = new t_asn_octet_string(IV, DES3CBCEncryption.BLOCKSIZE); 
  return octet_parameters!=NULL;
}
#endif
