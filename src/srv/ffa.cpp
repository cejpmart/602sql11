/****************************************************************************/
/* ffa.c - fast file access module                                          */
/*                                                                          */
/****************************************************************************/
#ifdef WINS
#include <windows.h>
#else
#include "winrepl.h"
#endif

#include "sdefs.h"
#include "scomp.h"
#include "basic.h"
#include "frame.h"
#include "kurzor.h"
#include "dispatch.h"
#include "table.h"
#include "dheap.h"
#include "curcon.h"
#pragma hdrstop
#include "profiler.h"
#ifndef WINS
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/fcntl.h>
#include <errno.h>
#ifdef UNIX
#include <unistd.h>
#include <stdlib.h>
#include <general.h>
#include <sys/file.h>
#include <sys/stat.h>
#else
#include <io.h>
#include <fcntl.h>
#include <advanced.h>
#include <fileengd.h>
#endif
#endif
#include "enumsrv.h"
#include "wbsecur.h"

#define ZIP602_STATIC
#include "Zip602.h"

#ifdef LINUX
#define NO_ASM
#define NO_MEMORYMAPPEDFILES
#endif

#ifdef FLEXIBLE_BLOCKSIZE
unsigned BLOCKSIZE = 4096;  // used when reading the database file header
#endif

void complete_broken_backup(void);
/************************** creating or restoring the database file ********************************/
inline bool position(FHANDLE hnd, tblocknum blocknum)
{
  return position2(hnd, (uns64)blocknum*BLOCKSIZE);
}

#define SAVE_ABLOCK(pos) { SetFilePointer(hnd, block_size*(pos), NULL, FILE_BEGIN);  WriteFile(hnd, ablock, block_size, &wr, NULL); }
#define LOAD_ABLOCK(pos) { SetFilePointer(hnd, block_size*(pos), NULL, FILE_BEGIN);   ReadFile(hnd, ablock, block_size, &wr, NULL); }

static bool is_piece(const tpiece * pc)
{ return pc->len32 < 32 && pc->offs32<32; }

static bool is_name(const char * name)
{ for (int i=0;  i<OBJ_NAME_LEN && name[i];  i++)
    if (name[i]>0 && name[i]<' ') return false;
  return true;
}

static bool is_tabtab_data(ttrec * rblock)
{ for (int i=0;  i<BLOCKSIZE / sizeof(ttrec);  i++)
  { ttrec * tt = rblock+i;
    if (tt->del_mark>2) return false;
    if (tt->del_mark==0)  // check the contents
    { if (tt->categ!=CATEG_TABLE && tt->categ!=(CATEG_TABLE|IS_LINK))
        return false;
      if (!*tt->name) return false;
      if (!is_name(tt->name)) return false;
      if (!is_name(tt->folder_name)) return false;
      if (!is_piece(&tt->defpiece)) return false;
      if (!is_piece(&tt->drigpiece)) return false;
      if (!is_piece(&tt->rigpiece)) return false;
      if (tt->defsize > 30000 || tt->drigsize>10000 || tt->rigsize>1000) return false;
    }
  }
  return true;
}

#define SETTINGS_FILE_NAME "settings.bin"

void park_server_settings(cdp_t cdp)
// Save server's setting not contained in the proptab into a temporary file.
// Ordering: server UUID, system charset, GCR, LOB counter value, private key, FIL password 
{ char fname[MAX_PATH];  DWORD wr;
  strcpy(fname, ffa.first_fil_path());  append(fname, SETTINGS_FILE_NAME);
  FHANDLE hnd = CreateFile(fname, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, 0);
  if (hnd==INVALID_FHANDLE_VALUE) request_error(cdp, OS_FILE_ERROR);
  else
  {// server UUID:
    char buf[2*UUID_SIZE];
    bin2hex(buf, header.server_uuid, UUID_SIZE);
    WriteFile(hnd, buf, 2*UUID_SIZE, &wr, NULL);
   // system charset:
    *buf = header.sys_spec.charset;
    WriteFile(hnd, buf, 1, &wr, NULL);
   // GCR value: 
    WriteFile(hnd, header.gcr, sizeof(header.gcr), &wr, NULL);
   // LOB counter: 
    WriteFile(hnd, &header.lob_id_gen, sizeof(header.lob_id_gen), &wr, NULL);
    CloseFile(hnd);
  }
}

void restore_server_settings(const char * fil_path)
// Restore server's setting not contained in the proptab from a temporary file.
{ char fname[MAX_PATH];  DWORD rd;
  strcpy(fname, fil_path);  append(fname, SETTINGS_FILE_NAME);
  FHANDLE hnd = CreateFile(fname, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, 0);
  if (hnd!=INVALID_FHANDLE_VALUE) 
  {// server UUID:
    char buf[2*UUID_SIZE];
    if (ReadFile(hnd, buf, 2*UUID_SIZE, &rd, NULL) && rd==2*UUID_SIZE)
    { hex2bin_str(buf, header.server_uuid, UUID_SIZE);
     // system charset:
      if (ReadFile(hnd, buf, 1, &rd, NULL) && rd==1)
      { header.sys_spec.charset=*buf;
       // CGR value:
        if (ReadFile(hnd, buf, sizeof(header.gcr), &rd, NULL) && rd==sizeof(header.gcr))
        { memcpy(header.gcr, buf, sizeof(header.gcr));
         // LOB counter: 
          if (ReadFile(hnd, buf, sizeof(header.lob_id_gen), &rd, NULL) && rd==sizeof(header.lob_id_gen))
            memcpy(&header.lob_id_gen, buf, sizeof(header.lob_id_gen)); 
        }
      }
    }  
    CloseFile(hnd);
    DeleteFile(SETTINGS_FILE_NAME);
  }
}

static BOOL format_database_file(const char * fil_path, BOOL restoring)
// [server_name] and [my_private_server] must be defined (in preinit()).
{ unsigned i;  int disp, val;  tblocknum tabtabdata;  
 // find the block size, verify the value (must be 4096 multiplied by a power of 2):
  unsigned block_size;
  if (!GetDatabaseString(server_name, "BlockSize", &val, sizeof(val), my_private_server) || val<4096) 
    block_size=4096;
  else    
  { block_size=val;
    while (!(val & 1)) val/=2;
    if (val != 1) block_size=4096;
  }
  DWORD wr;
  tptr ablock=(tptr)corealloc(block_size, 5);
  if (ablock==NULL) return FALSE;
 // open or create the database file:
  char fname[MAX_PATH];  strcpy(fname, fil_path);  append(fname, FIL_NAME);
  FHANDLE hnd=CreateFile(fname, GENERIC_READ | GENERIC_WRITE, 0, NULL,
      restoring ? OPEN_EXISTING : OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, (HANDLE)NULL);
  if (!FILE_HANDLE_VALID(hnd)) { corefree(ablock);  return FALSE; }
 // find allocated blocks:
  unsigned alloc_bytes;
  if (restoring)
  { DWORD flen=SetFilePointer(hnd, 0, NULL, FILE_END);
    unsigned alloc_blocks = flen / block_size;
    alloc_bytes = alloc_blocks / 8 + 1;
    if (alloc_bytes<9) alloc_bytes=9;
    if (alloc_bytes>block_size) alloc_bytes=block_size;
  }
 // init header:
  memset(&header, 0, sizeof(header_structure));
  strcpy(header.signature, "WinBase602 Database File\x1a");
  header.fil_version=CURRENT_FIL_VERSION_NUM;
#if CURRENT_FIL_VERSION_NUM==8  
  if (block_size!=4096)
    header.fil_version=8+1;  
#endif    
  header.sezam_open=wbfalse;
  header.blocksize=block_size;
  val = 0;
  if (GetDatabaseString(server_name, "SystemCharset", &val, sizeof(val), my_private_server))
    header.sys_spec.charset=val;
 // journal:
  header.jourfirst= HEADER_RESERVED;
  header.joursize = TRANS_JOURNAL_BYTES / block_size;
  memset(ablock, 0, block_size);
  for (i=0; i<header.joursize; i++) SAVE_ABLOCK(header.jourfirst+i);
 // bpool:
  header.bpool_first=disp=header.jourfirst+header.joursize;
  header.bpool_count=1;
  memset(ablock, 0xff, block_size);
  if (restoring) memset(ablock, 0x0, alloc_bytes);
  else { ablock[0]=0;  ablock[1]=(uns8)0xfe; }  // 9 blocks allocated
  SAVE_ABLOCK(disp);
  tblocknum blocks_used = 1;  // counts allocated blocks
 // remap:
  header.remapnum=0;
  header.remapblocks[0]=blocks_used++;  // 1
  for (i=1; i<REMAPBLOCKSNUM; i++) header.remapblocks[i]=0;
  memset(ablock, 0, block_size);
  SAVE_ABLOCK(disp+header.remapblocks[0]);
 // tabtab:
  header.tabtabbasbl=blocks_used++;  // 2
  if (restoring) LOAD_ABLOCK(disp+header.tabtabbasbl)
  else memset(ablock, 0, block_size);
  bas_tbl_block * bbl = (bas_tbl_block *)ablock;
  bbl->diraddr=wbtrue;
  bbl->nbnums=restoring ? 250 : 1;
  bbl->recnum=restoring ? 5000 : 1;
  bbl->unique_counter=0;
  for (i=0; i<INDX_MAX; i++) bbl->indroot[i]=0;
  bbl->databl[0]=tabtabdata=blocks_used++; // 3
  if (restoring==2)
  { char * rblock=(char *)corealloc(block_size, 5);
    DWORD rd;
    bbl->nbnums=0;
    bbl->recnum=0;
    tblocknum bl = blocks_used-1;
    while (position(hnd, disp+bl) && ReadFile(hnd, rblock, block_size, &rd, NULL) && rd==block_size)
    { if (is_tabtab_data((ttrec*)rblock))
      { bbl->recnum += block_size / sizeof(ttrec);
        bbl->databl[bbl->nbnums++]=bl;
        if (bbl->nbnums == MAX_BAS_ADDRS) break;
      }
      bl++;
    }
    corefree(rblock);
  }
  SAVE_ABLOCK(disp+header.tabtabbasbl);
  ttrec * tabtabrec = (ttrec *)ablock;
  if (restoring) LOAD_ABLOCK(disp+tabtabdata)
  else memset(ablock, 0, block_size);
  strcpy(tabtabrec->name, "TABTAB");
  if (!restoring) tabtabrec->last_modif=datetime2timestamp(Today(), Now());
  SAVE_ABLOCK(disp+tabtabdata);
 // piece store:
  header.koef_k32=block_size >> NUM_OF_PIECE_SIZES;
  memset(ablock, 0, block_size);
  hp_make_null_bg(ablock);
  for (i=0; i<NUM_OF_PIECE_SIZES; i++)
  { header.pcpcs[i].dadr=blocks_used++;  // 4,5,6,7,8
    header.pcpcs[i].offs32=0;
    header.pcpcs[i].len32=(uns8)(block_size/k32);  /* full block piece */
    header.pccnt[i]=0;  /* no free pieces contained */
    SAVE_ABLOCK(disp+header.pcpcs[i].dadr);
  }
  header.closetime=0;
  header.max_blocknum=blocks_used-1;
  memset(header.gcr, 0, ZCR_SIZE);
  header.prealloc_gcr=0;
  header.berle_used=restoring;
  header.creation_date=Today();  // used platforms for TRIAL licences
  if (!restoring) safe_create_uuid(header.server_uuid);
  for (i=0; i<10; i++) header.reserve[i]=0;   /* not used but set to 0 */
  restore_server_settings(fil_path);
 // save header:
  memcpy(ablock, &header, sizeof(header_structure));
  SAVE_ABLOCK(0);
  if (!restoring) SAVE_ABLOCK(disp+250);  // allocates space in the file
  corefree(ablock);
  CloseFile(hnd);
  return TRUE;
}

/////////////////////////////////// CRYPT ////////////////////////////////////
t_crypt_2 acrypt;

#if WBVERS>=1000 // in version 10 the DES encryption is supported but not used
void prep_des_key(const char * password, uns8 * crypt_xor)
{
  memset(crypt_xor, 0, DES_KEY_SIZE);
  strmaxcpy((char*)crypt_xor, password, DES_KEY_SIZE);
  for (int i=0;  DES_BLOCK_SIZE+i <= DES_KEY_SIZE;  i++)
    DesEnc(crypt_xor, crypt_xor+i);
}           
#endif

void t_crypt_1::set_crypt(t_fil_crypt acrypt, const char * password)
{ crypt_type=acrypt;
#if WBVERS>=1000 // in version 10 the DES encryption is supported but not used
  if (acrypt==fil_crypt_des)
    prep_des_key(password, crypt_xor);
  else 
#endif
#ifndef xOPEN602 
  if (acrypt==fil_crypt_slow)
    EDSlowInit((char*)password, crypt_xor);
  else
#endif
  { int i, j, x;
    x=0;
    for (i=0;  password[i];  i++) x=x^password[i];
    x=(x<<2) | (header.blocksize>>1) | 1;
    crypt_step=x;
    crypt_xor[0]=password[0];
    for (i=1, j=0;  i<XOR_SIZE;  i++)
    { crypt_xor[i]=password[j] ^ (crypt_xor[i-1] << 1);
      if (!password[++j]) j=0;
    }
  }
}

void t_crypt_1::i_fil_crypt(uns8 * block)
{ 
#if WBVERS>=1000 // in version 10 the DES encryption is supported but not used
  if (crypt_type==fil_crypt_des)
  { for (int i=0;  i<header.blocksize;  i+=DES_BLOCK_SIZE)
      DesEnc(crypt_xor, block+i);
  }
  else 
#endif
#ifndef xOPEN602 
  if (crypt_type==fil_crypt_slow)
    EDSlowEncrypt(block, header.blocksize, crypt_xor);
  else
#endif
  { uns8 bt, bt0;  unsigned i, j, pos;
    bt=block[0];  pos=crypt_step;  j=0;
    for (i=0;  i<header.blocksize;  i++)
    { bt0=block[pos];  block[pos]=bt ^ crypt_xor[j];
      bt=bt0;
      pos=(pos+crypt_step) & (header.blocksize - 1);
      if (++j == XOR_SIZE) j=0;
    }
  }
}

void t_crypt_1::o_fil_crypt(uns8 * block)
{ 
#if WBVERS>=1000 // in version 10 the DES encryption is supported but not used
  if (crypt_type==fil_crypt_des)
  { for (int i=0;  i<header.blocksize;  i+=DES_BLOCK_SIZE)
      DesDec(crypt_xor, block+i);
  }
  else 
#endif
#ifndef xOPEN602 
  if (crypt_type==fil_crypt_slow)
    EDSlowDecrypt(block, header.blocksize, crypt_xor);
  else
#endif
  { uns8 bt0;  unsigned i, j, pos, pos1;
    bt0=block[0];  pos=0;  j=0;
    for (i=0;  i<header.blocksize-1;  i++)
    { pos1=(pos+crypt_step) & (header.blocksize - 1);
      block[pos]=block[pos1] ^ crypt_xor[j];
      pos=pos1;
      if (++j == XOR_SIZE) j=0;
    }
    block[pos]=bt0 ^ crypt_xor[j];
  }
}

void crypt_changed(cdp_t cdp)
{ tblocknum block=1;
  tblocknum count=ffa.get_fil_size();
  while (!ffa.w_raw_read_block(block, (tptr)acrypt.cbuf))
  { ffa.w_raw_write_block(block, (tptr)acrypt.cbuf);
    if (!(block % 32)) report_state(cdp, block, count);
    block++;
  }
  report_state(cdp, NORECNUM, NORECNUM);
}

//////////////////////////////////// ffa /////////////////////////////////////////
uns8 * corepool = NULL;  /* used in catch_free etc. too */

void make_directory_path(tptr direc)
{ int len=(int)strlen(direc);
  for (int i=1;  i<len;  i++)
    if (direc[i]==PATH_SEPARATOR)
    { direc[i]=0;
      CreateDirectory(direc, NULL);
      direc[i]=PATH_SEPARATOR; 
    }
}
////////////////////////////// analysing ///////////////////////////////////////
#ifdef WINS
BOOL WINAPI ServerByPath(const char * path, char * server_name)
// Returns server name for the specified path of the 1st fil part. Obsolete.
{ LONG res;  HKEY hKey, hSubKey;  char key[160];
  tobjname a_server_name;  char a_path[MAX_PATH];
  if (path[strlen(path)-1]=='\\') ((char*)path)[strlen(path)-1]=0;  // removes '\\' from the end
  *server_name=0;  /* init. result */
  strcpy(key, WB_inst_key);  strcat(key, Database_str);
  res=RegOpenKeyEx(HKEY_LOCAL_MACHINE, key, 0, KEY_READ_EX, &hKey);
  if (res!=ERROR_SUCCESS) return FALSE;
  int ind=0;
  do
  { DWORD key_len=sizeof(a_server_name);
    if (RegEnumKey(hKey, ind++, a_server_name, key_len) != ERROR_SUCCESS)
      break;
    res=RegOpenKeyEx(hKey, a_server_name, 0, KEY_READ_EX, &hSubKey);
    if (res==ERROR_SUCCESS)
    { key_len=sizeof(a_path);
      res=RegQueryValueEx(hSubKey, Path_str, NULL, NULL, (BYTE*)a_path, &key_len);
      if (res!=ERROR_SUCCESS) *a_path=0;
      RegCloseKey(hSubKey);
     /* send server request: */
      if (*a_path && !stricmp(a_path, path))  /* path specified & equal */
		  { strcpy(server_name, a_server_name);
        break;
      }
    }
  } while (TRUE);
  RegCloseKey(hKey);
  return *server_name!=0;
}
#endif

BOOL t_ffa::GetServerPaths(const char * server_name)
// Defines fil_parts and all fil_path[], fil_offset[].
// Returns TRUE if OK, FALSE if trying to use the server_name as path.
{ fil_offset[0]=0;
  fil_path[0][0]=0;
  my_private_server=false;
  if (!GetPrimaryPath(server_name, my_private_server, fil_path[0]))
  { my_private_server=true;
    if (!GetPrimaryPath(server_name, my_private_server, fil_path[0]))
    { strcpy(fil_path[0], server_name);  fil_parts=1;
      append(fil_path[0], "");  // appends '/' or '\\' unless it is the last char
      return FALSE;  // the parameter is not the server name
    }
  }
  append(fil_path[0], "");  // appends '/' or '\\' unless it is the last char
  fil_parts=1;
 // look for the other paths and limits:
  do
  { char key[16];  DWORD lim;
   // read fil part limit:
    strcpy(key, "Limit");  int2str(fil_parts, key+5);
    if (!GetDatabaseString(server_name, key, (tptr)&lim, sizeof(DWORD), my_private_server)) break;
    fil_offset[fil_parts] = fil_offset[fil_parts-1] + lim * 256;  // converted from megatytes to blocks
   // read the FIL path:
    strcpy(key, "Path");  int2str(fil_parts+1, key+4);
    tptr dest = fil_path[fil_parts];  *dest=0;
    if (!GetFilPath(server_name, my_private_server, fil_parts+1, dest) || !*dest) break;
    append(dest, ""); // appends '\\' unless it is the last char
  } while (++fil_parts<MAX_FIL_PARTS);
  return TRUE;
}

void t_ffa::preinit(const char * name_or_path, char * server_name)
// From server name or FIL path obtains the server name and 1st path.
// Reads all other paths.
// No deinitiazation necessary.
{// main preinit:
  backup_support=NULL;
  db_file_size=0;  fil_open_counter = 0;  first_part_read_only=FALSE;
  for (int i=0;  i<MAX_FIL_PARTS;  i++) handle[i]=INVALID_FHANDLE_VALUE;
  j_handle=t_handle=c_handle=hProtocol=INVALID_FHANDLE_VALUE;   /* all handles closed */
 // paths init:
  *server_name=0;
  if (GetServerPaths(name_or_path))
    strcpy(server_name, name_or_path);
#ifdef WINS  // for LINUX always name passed
  else  // specified server name not defined, try direct file path
    ServerByPath(name_or_path, server_name);
#endif
 // remap:
  remap_from = &remap_noinit;
  remap_to   = &remap_noinit;
 // accocation
#ifndef FLUSH_OPTIZATION        
  next_pool=0;  blocks_in_buf=0;
#endif
#ifdef FLEXIBLE_BLOCKSIZE
  header.blocksize=BLOCKSIZE=4096;  // used when reading the header
#else
  header.blocksize=BLOCKSIZE;  // used when reading the header
#endif
}

////////////////////////////// open and close /////////////////////////////////////
char * append(char * dest, const char * src)  // appends with single PATH_SEPARATOR
{ int len=(int)strlen(dest);
  if (!len || dest[len-1]!=PATH_SEPARATOR) dest[len++]=PATH_SEPARATOR;
  if (*src==PATH_SEPARATOR) src++;
  strcpy(dest+len, src);
  return dest;
}

#define DADR_COUNT2BPOOL_COUNT(dadr_count)  (((dadr_count)-1) / (8*BLOCKSIZE) + 1)

void t_ffa::get_database_file_size(void)
// Finds the size of all parts of the database file. BLOCKSIZE must be defined and file opened.
{ db_file_size=0;
  for (int i=0;  i<fil_parts;  i++)
  {
#ifdef WINS
    unsigned _int64 len64;
    DWORD length=GetFileSize(handle[i], (DWORD*)&len64+1);
    if (length!=(DWORD)-1 || GetLastError()==NO_ERROR)
    { *(DWORD*)&len64=length;
      if (len64)  // non-first part of the file may be created, but empty (after compacting)
      { tblocknum blocks = (tblocknum)(len64/BLOCKSIZE);
#elif defined LINUX
    uns64 len64=GetFileSize(handle[i], NULL);
    if (len64!=(uns64)-1 && len64) /* TZ: this causes next if to be always true - weird */
    { if (len64) // non-first part of the file may be created, but empty (after compacting)
      { tblocknum blocks = (tblocknum)(len64/BLOCKSIZE);
#else
    DWORD length=GetFileSize(handle[i], NULL);
    if (length!=(DWORD)-1 && length)
    { if (length) // non-first part of the file may be created, but empty (after compacting)
      { tblocknum blocks = length/BLOCKSIZE;
#endif
        part_blocks[i]=blocks;
        blocks += fil_offset[i];  
        if (blocks > db_file_size) db_file_size=blocks;
      }
    }
  }
}

BOOL t_ffa::open_fil_parts(void)
// Opens all parts of FIL, supposes fil_parts and fil_path[] defined.
// Defines handle[] and db_file_size, part_blocks.
// Returns TRUE on error, all files are closed then.
{ 
  for (int i=0;  i<fil_parts;  i++)
  { char fname[MAX_PATH];  strcpy(fname, fil_path[i]);  append(fname, FIL_NAME);
#ifdef WINS     // READ sharing necessary for backups
    handle[i]=CreateFile(fname, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ,//|FILE_SHARE_WRITE
      NULL, OPEN_EXISTING, FILE_FLAG_RANDOM_ACCESS, NULL);
    if (!FILE_HANDLE_VALID(handle[i]) && (GetLastError()==ERROR_ACCESS_DENIED || GetLastError()==ERROR_NETWORK_ACCESS_DENIED) &&
        !i && fil_parts>=2)  // READ-ONLY 1st part on CD-ROM database
    { handle[i]=CreateFile(fname, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_RANDOM_ACCESS, NULL);
      first_part_read_only=TRUE;
    }
#else
    handle[i]=_lopen(fname, OF_READWRITE|/*OF_SHARE_DENY_NONE*/OF_SHARE_DENY_WRITE);
#endif
    if (!FILE_HANDLE_VALID(handle[i])) 
      { close_fil_parts();  return TRUE; }
#ifdef LINUX
    if (flock(handle[i], LOCK_EX|LOCK_NB)){
	    close_fil_parts();
	    return TRUE;
    }
#endif
  } // cycling on database file parts
  return FALSE;
}

void t_ffa::fil_consistency(void)
{ if (DADR_COUNT2BPOOL_COUNT(ffa.get_fil_size() - header.bpool_first) > header.bpool_count)
    dbg_err("INFO: BPOOL number inconsistency");
}

void t_ffa::close_fil_parts(void)
// Closes all FIL parts, supposes each handle valid or =INVALID_HANDLE_VALUE.
{ for (int i=0;  i<fil_parts;  i++)
    if (FILE_HANDLE_VALID(handle[i])) 
      { CloseFile(handle[i]);  handle[i]=INVALID_FHANDLE_VALUE; }
}

#ifdef WINS
#define create_db_file(fname) CreateFile(fname, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, FILE_FLAG_RANDOM_ACCESS, NULL);
#else
#define create_db_file(fname) _lcreat(fname, 0666);  // less problems iff common access allowed, admin may change it
#endif

int t_ffa::restore(void)
{ if (!format_database_file(fil_path[0], 2)) return KSE_NO_FIL; 
  return KSE_OK;
}

int t_ffa::check_fil(void)
/* Returns error if cannot establish exclusive access to the FIL */
// Check all parts of the database file, creates them if they do not exist.
{ for (int i=0;  i<fil_parts;  i++)
  { FHANDLE hnd;  char fname[MAX_PATH];
    strcpy(fname, fil_path[i]);  append(fname, FIL_NAME);
#ifdef WINS
    hnd=CreateFile(fname, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_FLAG_RANDOM_ACCESS, NULL);
    if (!FILE_HANDLE_VALID(hnd) && (GetLastError()==ERROR_ACCESS_DENIED || GetLastError()==ERROR_NETWORK_ACCESS_DENIED) &&
        !i && fil_parts>=2)  // READ-ONLY 1st part on CD-ROM database
      hnd=CreateFile(fname, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_FLAG_RANDOM_ACCESS, NULL);
#else
    hnd=_lopen(fname, OF_READWRITE|OF_SHARE_EXCLUSIVE);
#endif

    if (!FILE_HANDLE_VALID(hnd))  // cannot open exclusive, check the file existence
    {
#ifdef WINS
      if (GetLastError()==ERROR_SHARING_VIOLATION || GetLastError()==ERROR_ACCESS_DENIED || GetLastError()==ERROR_NETWORK_ACCESS_DENIED)
#else
      if (errno==EACCES)
#endif
        return KSE_DBASE_OPEN;
      { if (!i)  // creating the 1st part:
          { if (!format_database_file(fil_path[0], FALSE)) return KSE_CANNOT_CREATE_FIL; }
        else // creating the other parts
        { hnd=create_db_file(fname);
          if (!FILE_HANDLE_VALID(hnd)) return KSE_CANNOT_CREATE_FIL;
          CloseFile(hnd);
        }
      }
    } // exclusive not opened
    else // filecan be opened
    {
#ifdef LINUX
      if (flock(hnd, LOCK_EX|LOCK_NB))
	      { close(hnd);  return KSE_DBASE_OPEN; }
#endif
	    CloseFile(hnd);  // file opened OK
    }
  } // cycle on parts
  return KSE_OK;
}

void t_ffa::close_fil_access(cdp_t cdp) // LCK1
// Closes all files for the thread
{ 
  { ProtectedByCriticalSection cs(&cs_short_term);
    if ( !fil_open_counter ||   /* this is a strange error, impossible I hope */
        --fil_open_counter)     // files are used by another thread, do not close the files
      return;
  }    
  { ProtectedByCriticalSection cs(&fil_sem, cdp, WAIT_CS_FIL);
    close_fil_parts();
    if (FILE_HANDLE_VALID(j_handle)) CloseFile(j_handle);
    if (FILE_HANDLE_VALID(t_handle)) CloseFile(t_handle);
    if (FILE_HANDLE_VALID(c_handle)) CloseFile(c_handle);
    if (FILE_HANDLE_VALID(hProtocol)) CloseFile(hProtocol);
    j_handle=t_handle=c_handle=hProtocol=INVALID_FHANDLE_VALUE;   /* all handles closed */
  }
}

int t_ffa::open_fil_access(cdp_t cdp, char * server_name)  // LCK1
// Called on interf_init from AddTask, again in dir_kernel_close.
// Opens FIL, journal & transact, creates journal/transact if it does not exist, 
// no error if cannot open journal.
// Uses fil_path. Defines file handles, db_file_size and global_journal_pos.
{ char fname[MAX_PATH];  int res=KSE_OK;
  { ProtectedByCriticalSection cs(&cs_short_term);
    if (0!=fil_open_counter)
    { fil_open_counter++; 
      return res;
    }
  }  

  { ProtectedByCriticalSection cs(&fil_sem, cdp, WAIT_CS_FIL);
    { ProtectedByCriticalSection cs(&cs_short_term);
      if (0!=fil_open_counter)
      { fil_open_counter++; 
        return res;
      }
    }  
   // openning the database file:
    if (open_fil_parts()) return KSE_CANNOT_OPEN_FIL;

   // openning the transaction file:
    if (!GetDatabaseString(server_name, "TransactPath", fname, sizeof(fname), my_private_server) || !*fname)
      strcpy(fname, fil_path[fil_parts-1]);  
    append(fname, TRANS_NAME);
  #ifdef WINS
    t_handle=CreateFile(fname, GENERIC_READ | GENERIC_WRITE, 0, //FILE_SHARE_READ|FILE_SHARE_WRITE
      NULL, OPEN_ALWAYS, FILE_FLAG_RANDOM_ACCESS, NULL);
  #else
    t_handle=_lopen(fname, OF_READWRITE|OF_SHARE_EXCLUSIVE/*OF_SHARE_DENY_NONE*/);
    if (!FILE_HANDLE_VALID(t_handle))
    { t_handle=create_db_file(fname);
      if (FILE_HANDLE_VALID(t_handle))  /* must close and open with the sharing atribute! */
      { _lclose(t_handle);
        t_handle=_lopen(fname, OF_READWRITE|OF_SHARE_EXCLUSIVE/*OF_SHARE_DENY_NONE*/);
      }
    }
  #endif
    if (!FILE_HANDLE_VALID(t_handle)) 
      { close_fil_access(cdp);  return KSE_CANNOT_OPEN_TRANS; }

   // openning the journal file (no error if cannot open):
    if (!GetDatabaseString(server_name, "JournalPath", fname, sizeof(fname), my_private_server) || !*fname)
      strcpy(fname, fil_path[fil_parts-1]);  
    append(fname, JOURNAL_NAME);
  #ifdef WINS
    j_handle=CreateFile(fname, GENERIC_READ | GENERIC_WRITE, 0, //FILE_SHARE_READ|FILE_SHARE_WRITE
      NULL, OPEN_ALWAYS, 0, NULL);
  #else
    j_handle=_lopen(fname, OF_READWRITE|OF_SHARE_EXCLUSIVE/*OF_SHARE_DENY_NONE*/);
    if (!FILE_HANDLE_VALID(j_handle))
    { j_handle=create_db_file(fname);
      if (FILE_HANDLE_VALID(j_handle))  /* must close and open with the sharing atribute! */
      { _lclose(j_handle);
        j_handle=_lopen(fname, OF_READWRITE|OF_SHARE_EXCLUSIVE/*OF_SHARE_DENY_NONE*/);
      }
    }
  #endif
   // find the journal length and set the global_journal_pos:
    if (FILE_HANDLE_VALID(j_handle))
    { DWORD lenght=GetFileSize(j_handle, NULL);
      if (lenght!=(DWORD)-1 && lenght > global_journal_pos) global_journal_pos=lenght;
    }

   // openning the communication journal file (no error if cannot open, no sharing):
    if (GetDatabaseString(server_name, "CommJournalPath", fname, sizeof(fname), my_private_server) && *fname)
    { append(fname, COMM_JOURNAL_NAME);
  #ifdef WINS
      c_handle=CreateFile(fname, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, 0, NULL);
  #else
      c_handle=_lopen(fname, OF_READWRITE);
      if (!FILE_HANDLE_VALID(j_handle))
        c_handle=create_db_file(fname);
  #endif
    }

    protocol2=true;  
   // look for an auxiliary backup file and use/delete it, if found:
    complete_broken_backup();
  }  
 // all files opened:
  { ProtectedByCriticalSection cs(&cs_short_term);
    fil_open_counter=1;
  }  
  return res;
}

////////////////////////////////// backuping and zippig ///////////////////////////////////
struct zipper_data // data used by the offline zipping thread
{ THREAD_HANDLE th;
  bool first_part_read_only;   // do not backup the first part iff true
  bool _disconnect_remote_backup_dir;  // call disconnect_remote_backup_dir() on the end
  int  fil_parts;              // number of fil parts
  char name_pattern[MAX_PATH]; // name for the backup files and zip file

  zipper_data(bool first_part_read_onlyIn, bool disconnect_remote_backup_dirIn, int fil_partsIn, const char * name_patternIn) :
    first_part_read_only(first_part_read_onlyIn), 
    _disconnect_remote_backup_dir(disconnect_remote_backup_dirIn),
    fil_parts(fil_partsIn)
      { strcpy(name_pattern, name_patternIn); }
};

void Save_backup_result(cdp_t cdp, const char *Msg)
{ time_t tm;
  time(&tm);
  struct tm *ltm = localtime(&tm);
  char tmm[32];
  strftime(tmm, sizeof(tmm), "%d.%m.%Y %H:%M:%S", ltm);
  set_property_value(cdp, "@SQLSERVER", "LastBackupTime",  0, tmm, (unsigned)-1);
  set_property_value(cdp, "@SQLSERVER", "LastBackupError", 0, Msg, (unsigned)-1);
}

THREAD_PROC(zipper)
// ZIPped file is created in the backup directory. Then, it may be moved.
// Must deallocate [data].
{ zipper_data * zd = (zipper_data*)data;
  char fil_copy_name[MAX_PATH];
  int len = (int)strlen(zd->name_pattern);
  zd->name_pattern[len+1]=0;
 // append 'z' to the name of the zip file if the name is standard:
  int namestart = len-1;
  while (namestart && zd->name_pattern[namestart-1]!=PATH_SEPARATOR) namestart--;
  if (len==namestart+11)
  { bool standard = true;
    for (int j=0;  j<11;  j++)
      if (j!=8)
        if (zd->name_pattern[namestart+j]<'0' || zd->name_pattern[namestart+j]>'9')
          standard=false;
    if (standard)
      zd->name_pattern[len]='Z';
  }
 // create the ZIP file and add backup files into it:
  INT_PTR hZip = 0;
  int     Err;
  int     i;
  char    msg[256] = "";
  wchar_t wname[MAX_PATH];
  wchar_t wroot[MAX_PATH];
  wchar_t *root = NULL;
  superconv(ATT_STRING, ATT_STRING, zd->name_pattern, (char *)wname, t_specif(0, wbcharset_t::get_host_charset().get_code(), 0, 0), ext_unicode_specif, NULL);
  wcscpy(wroot, wname);
  wchar_t *p = wcsrchr(wroot, PATH_SEPARATOR);
  if (p)
  { *p = 0;
    root = wroot;
  }
  Err = ZipCreate(wname, root, &hZip);
  if (!Err)
  { for (i=0;  i<zd->fil_parts;  i++)
      if (i || !zd->first_part_read_only)
      { memcpy(fil_copy_name, zd->name_pattern, len+2);
        fil_copy_name[len]='A'+i;
        superconv(ATT_STRING, ATT_STRING, fil_copy_name, (char *)wname, t_specif(0, wbcharset_t::get_host_charset().get_code(), 0, 0), ext_unicode_specif, NULL);
        Err = ZipAdd(hZip, wname);
        if (Err)
          break;
      }
  }
  if (Err)
  { if (hZip)
      ZipGetErrMsgA(hZip, msg, sizeof(msg));
    else
      propagate_system_error("Error %s when zipping the backup.", "", msg);
  }
  if (hZip)
    ZipClose(hZip);
  if (Err)
      goto exit;

 // delete the copy of the FIL:
  for (i=0;  i<zd->fil_parts;  i++)
    if (i || !zd->first_part_read_only)
    { memcpy(fil_copy_name, zd->name_pattern, len+2);
      fil_copy_name[len]='A'+i;
      DeleteFile(fil_copy_name);
    }
 // copy the zipped file, if required:
  if (*the_sp.BackupZipMoveTo.val())
  { char dest_zip[MAX_PATH];
    strcpy(dest_zip, the_sp.BackupZipMoveTo.val());
    append(dest_zip, zd->name_pattern+namestart);
    if (CopyFileEx(zd->name_pattern, dest_zip, NULL, NULL, &bCancelCopying, 0))
    { DeleteFile(zd->name_pattern);  // delete source if copied OK
      backup_count_limit(dest_zip, "z");  
    }
    else
      backup_count_limit(zd->name_pattern, "z");  
  }
  else
    backup_count_limit(zd->name_pattern, "z");  
  offline_threads.remove(zd->th);
exit:
 // write the result into the properties:   
  cd_t cd(PT_WORKER);
  int res = interf_init(&cd);
  if (res == KSE_OK)
  { Save_backup_result(&cd, msg); 
    commit(&cd);
    kernel_cdp_free(&cd);
    cd_interf_close(&cd);
  }
  delete zd;
  SetLocalManualEvent(&Zipper_stopped);
  THREAD_RETURN;
}

int t_ffa::do_backup(cdp_t cdp, const char * fname)  // LCK1
// Backups all database file parts (except CD ROM), returns TRUE on success, FALSE on copy error.
// Saves cached pieces and blocks before, tring to reduce lock pieces and blocks in the backup
{ int err;
  if (!kernel_is_init) return INTERNAL_SIGNAL;
  ProtectedByCriticalSection cs  (&commit_sem, cdp, WAIT_CS_COMMIT);  // must be before entering fil_sem
  ProtectedByCriticalSection cs_b(&disksp.bpools_sem, cdp, WAIT_CS_BPOOLS);  // must be before close_block_cache
  commit(cdp);       // before saving pieces must remove any error (it may occur in _on_server_event()!)
  cdp->request_init();
  save_pieces(cdp);  /* must be done before saving blocks! */
  commit(cdp);       // writes saved pieces
  disksp.close_block_cache(cdp);
  ProtectedByCriticalSection cs2(&remap_sem);  // protects the access to the [header]
  header.sezam_open=wbfalse;
  header.closetime=stamp_now();
  write_header();  
  { ProtectedByCriticalSection cs(&fil_sem, cdp, WAIT_CS_FIL);
    err = do_backup_inner(cdp, fname);
  }
  reload_pieces(cdp);
  header.sezam_open=wbtrue;
  write_header();
  return err;
}

void propagate_system_error(const char * pattern, const char * parameter, char * errmsg)
#define SYS_MSG_MAXSIZE 256
// Writes the text of the system error to the log, using [pattern] and optional {parametr].
// If [errmsg] != NULL then it is used form the system message, its size is supposed to be SYS_MSG_MAXSIZE.
{ char err[SYS_MSG_MAXSIZE];
  if (!errmsg) errmsg=err;
  *errmsg=0;
#ifdef UNIX
  strerror_r(errno, errmsg, SYS_MSG_MAXSIZE);
#else
  FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, GetLastError(), MAKELANGID(LANG_ENGLISH, SUBLANG_DEFAULT), errmsg, SYS_MSG_MAXSIZE, NULL);
  // NOTE: The enconding is not specified for MSW so it is more safe to use the english message
  if (!*errmsg) sprintf(errmsg, "%u", GetLastError());
#endif
  char msg[MAX_PATH+100+SYS_MSG_MAXSIZE];
  sprintf(msg, pattern, errmsg, parameter);
  dbg_err(msg);
}

int t_ffa::do_backup_inner(cdp_t cdp, const char * fname)  
// Copies the database file(s) to [fname] 
{ int err=ANS_OK;  char fnames[MAX_PATH];
  strcpy(fnames, fname);
  make_directory_path((char*)fname);  // creates the directory path for fname if it does not exist
#ifdef WINS
  SYSTEMTIME systime;  GetSystemTime(&systime);
  FILETIME filetime;  SystemTimeToFileTime(&systime, &filetime);
#endif
  int len = (int)strlen(fnames);  char filname[MAX_PATH];  
  char errmsg[256] = "";
  for (int i=0;  i<fil_parts;  i++)
  { 
#ifdef STOP   // temporarily removed because of problems in Archibald  
    t_temp_priority_change tpc(backup_support==NULL);  // ABOVE if blocking, BELOW if nonblocking
#endif    
    if (i || !first_part_read_only)
    { fnames[len]='A'+i;  fnames[len+1]=0;
      strcpy(filname, fil_path[i]);  append(filname, FIL_NAME);
      if (!CopyFileEx(filname, fnames, NULL, NULL, &bCancelCopying, 0)) 
      { propagate_system_error("Error %s when creating the database file backup in %s.", fnames, errmsg);
        err=OS_FILE_ERROR;
      }          
#ifdef WINS
      else  // set file date and time
      { FHANDLE hnd=CreateFile(fnames, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
        if (FILE_HANDLE_VALID(hnd))
        { SetFileTime(hnd, &filetime, &filetime, &filetime);
          CloseFile(hnd);
        }
      }
#endif
    }
  }  
  fnames[len]=0;  // used by ZIP thread
  
 // ZIP or delete old backups: 
  if (err==ANS_OK)
   // start zipping thread
    if (the_sp.BackupZip.val())
    { zipper_data * zd = new zipper_data(first_part_read_only!=FALSE, false, fil_parts, fnames);
      if (zd)
      { ResetLocalManualEvent(&Zipper_stopped);
        if (THREAD_START_JOINABLE(::zipper, 10000, zd, &zd->th))  // TRUE if started
          offline_threads.add(zd->th);  // register the offline thread
        else 
        { delete zd;
          SetLocalManualEvent(&Zipper_stopped);
        }
      }  
    }
    else
      backup_count_limit(fname, NULL);  // called by the zipper thread if zipped offline
  if (!the_sp.BackupZip.val() || err!=ANS_OK)
    Save_backup_result(cdp, errmsg);
  return err;
}

//////////////////////////////////////// hot-backup support /////////////////////////////////////

int t_ffa::do_backup2(cdp_t cdp, const char * fname) 
// non-blocking backup, returns error number or 0 if OK.
{ int res = ANS_OK;
  { ProtectedByCriticalSection cs1(&commit_sem);  // the committed state if the database file must be freeded
    ProtectedByCriticalSection cs2(&fil_sem);
    if (backup_support) return INTERNAL_SIGNAL; // another backup in progress
    backup_support = new t_backup_support_fil(&fil_sem);
    if (!backup_support) return OUT_OF_KERNEL_MEMORY;
   // must not exit the CS before successfull prepare()
    if (!backup_support->prepare(false))
      { propagate_system_error("Error %s when openning auxiliary hot-backup file %s.", backup_support->my_auxi_file_name());
        delete backup_support;  backup_support=NULL;  
        return OS_FILE_ERROR;
      }  
  }
 // backup (all write operations are redirected):
  res=do_backup_inner(cdp, fname);
 // synchronize changes:
  backup_support->synchronize(false, (void**)&backup_support);
  return res;
}

void t_ffa::complete_broken_backup(void)
// called before staring other threads
{
  backup_support = new t_backup_support_fil(&fil_sem);
  if (!backup_support) return;
 // look for the auxiliary file:
  if (!backup_support->prepare(true))
    { delete backup_support;  backup_support=NULL;  return; }
 // write the changes form the auxi file and delete it:
  backup_support->synchronize(true, (void**)&backup_support);
}

void full_bpool(uns8 * corepool)
{ memset(corepool, 0xff, BLOCKSIZE);  corepool[0]=0xfe; }

void empty_bpool(uns8 * corepool)
{ memset(corepool, 0, BLOCKSIZE); }

//////////////////////////// raw read and write //////////////////////////////////
BOOL WINAPI t_ffa::w_raw_read_block(uns32 blocknum, void * kam)  // LCK1
{ ProtectedByCriticalSection cs(&fil_sem);  // all must be protected: get_block may temp. close the file and handle[0] is invalid
  if (backup_support)
  {// try reading from the file of changes made during the backup:
    if (backup_support->read_block(blocknum, kam))
      goto decrypt;  // read OK
   // check for reading from the part added during the backup - must not read because the fil has not been expanded yet
    if (blocknum >= db_file_size && blocknum < backup_support->new_db_file_size)
      return FALSE; // the contents is undefined (has not been written)
  }
 // now the blocknum is inside FIL
  DWORD rd;  FHANDLE hnd;
  if (fil_parts>1)
  { int i=0;  
    while (i+1 < fil_parts && blocknum >= fil_offset[i+1]) i++;
    blocknum-=fil_offset[i];
    hnd=handle[i];  
  }
  else hnd=handle[0];
  if (!position(hnd, blocknum) || !ReadFile(hnd, kam, BLOCKSIZE, &rd, NULL) || rd!=BLOCKSIZE)
    return TRUE;
 decrypt:
 // decrypting:
  if (acrypt.i.crypting() && blocknum) acrypt.i.i_fil_crypt((uns8*)kam);
  return FALSE;
}

BOOL WINAPI t_ffa::w_raw_write_block(uns32 blocknum, const void * adr)  // LCK1
{ 
  ProtectedByCriticalSection cs(&fil_sem);  // must be before encrypting which uses shared acrypt.cbuf
  if (acrypt.o.crypting() && blocknum)
  { memcpy(acrypt.cbuf, adr, header.blocksize);
    acrypt.o.o_fil_crypt(acrypt.cbuf);
    adr=(tptr)acrypt.cbuf;
  }

  if (backup_support)
    if (backup_support->is_phase1())
      return !backup_support->write_block(blocknum, adr);
    else
      backup_support->invalidate_block(blocknum);
  return w_enc_write_block(blocknum, adr);  // LCK1
}

BOOL WINAPI t_ffa::w_enc_write_block(uns32 blocknum, const void * adr)  // LCK1
{ DWORD wr;  FHANDLE hnd;  uns32 new_end;  int part;
  new_end=blocknum+1;  
  if (fil_parts>1)
  { if (blocknum < fil_offset[1])
    { if (first_part_read_only) return TRUE; // 1st part converted to ROM, must not write
      hnd=handle[0];  part=0;
    }
    else
    { part=1;
      while (part+1 < fil_parts && blocknum >= fil_offset[part+1]) part++;
      blocknum-=fil_offset[part];
      hnd=handle[part];  
    }
  }
  else { hnd=handle[0];  part=0; }
  if (!position(hnd, blocknum) || !WriteFile(hnd, adr, BLOCKSIZE, &wr, NULL) || wr!=BLOCKSIZE)
    return TRUE; 
  if (new_end  >  db_file_size)      db_file_size      = new_end;
  if (blocknum >= part_blocks[part]) part_blocks[part] = blocknum+1;
  return FALSE;
}

void t_ffa::flush(void)
{ // on Linux it writes data but does not update file update time -> faster
  if (!backup_support)
  { ProtectedByCriticalSection cs(&fil_sem);  // I do not know if it is necessary, probably not
    for (int i=0;  i<fil_parts;  i++) 
      FlushFileBuffers(handle[i]);
  }    
}

int t_ffa::read_header(void)  // called only once when starting server
{ int res=KSE_OK;
  tptr header_frame = (tptr)corealloc(BLOCKSIZE, OW_FRAMES);
  if (header_frame == NULL) return KSE_NO_MEMORY;
  header_blocknum=0;
  if (w_raw_read_block(header_blocknum, header_frame))
    res=KSE_DAMAGED;  
  else 
  { memcpy(&header, header_frame, sizeof(header));  /* header prepared */
#ifdef FLEXIBLE_BLOCKSIZE
   // update BLOCKSIZE according to the database file header:
    BLOCKSIZE=header.blocksize;
    if (BLOCKSIZE!=4096)
    { header_frame=(char*)corerealloc(header_frame, BLOCKSIZE);
      if (header_frame == NULL) return KSE_NO_MEMORY;
    }
#endif
   // BLOCKSIZE is defined now:
    corepool = (uns8*)corealloc(BLOCKSIZE, 5);
    if (!corepool) return KSE_NO_MEMORY;
   // verify the header:
    if      (header.signature[0] == (char)0xdc) res=KSE_BAD_VERSION;
    else if (header.fil_version  != CURRENT_FIL_VERSION_NUM 
#ifdef FLEXIBLE_BLOCKSIZE
          && header.fil_version  != 8+1  // only new servers accept this, independent of version
#endif
            )
      res=KSE_BAD_VERSION;
    else if (header.cd_rom_offset) 
    { if (fil_parts==1)  // FIL converted for CDROM, but not installed
        res=KSE_CDROM_FIL;
      else
      { tblocknum new_base = header.cd_rom_offset*8*BLOCKSIZE+header.bpool_first;
        header_blocknum=new_base-1;  // replacement header before the new bpool
        first_part_read_only=TRUE;
        if (w_raw_read_block(header_blocknum, header_frame))
        { int i;
         // create replacement header:
         // copy the contents of existing remap blocks & update their numbers:
          unsigned blocks_used=1; // bpool
          for (i=0;  i<REMAPBLOCKSNUM;  i++) 
            if (header.remapblocks[i])
            { w_raw_read_block(header.bpool_first+header.remapblocks[i], corepool);
              w_raw_write_block(new_base+blocks_used, corepool);
              header.remapblocks[i]=new_base+blocks_used - header.bpool_first;
              blocks_used++;
            }
         // create new lists of free pieces:
          memset(corepool, 0, BLOCKSIZE);
          hp_make_null_bg(corepool);
          for (i=0; i<NUM_OF_PIECE_SIZES; i++)
          { header.pcpcs[i].dadr=new_base+blocks_used - header.bpool_first;  
            header.pcpcs[i].offs32=0;
            header.pcpcs[i].len32=32;  /* full block piece */
            header.pccnt[i]=0;  /* no free pieces contained */
            w_raw_write_block(new_base+blocks_used, corepool);
            blocks_used++;
          }
         // init the new bpool:
          full_bpool(corepool);
          uns8 * p = corepool;
          while (blocks_used>=8) { *(p++)=0;  blocks_used-=8; }
          *p &= ~((1<<blocks_used)-1);
          w_raw_write_block(new_base, corepool);
          header.bpool_count++;
          write_header();
         // allocate spare space:
          w_raw_write_block(new_base+blocks_used+60, corepool);
        }
        else
        { memcpy(&header, header_frame, sizeof(header));  /* header prepared */
          if      (header.signature[0] == (char)0xdc) res=KSE_BAD_VERSION;
          else if (header.fil_version  != CURRENT_FIL_VERSION_NUM &&
                   header.fil_version  != 8+1)  // only new servers accept this, independent of version
            res=KSE_BAD_VERSION;
        }
      } // FIL has 2 or more parts
    } // CDROM FIL
  } // 1st header read OK
  corefree(header_frame);
  return res;
}

void t_ffa::write_header(void)
// Writes the database file header to the disc.
// Access to the header is protected by the [remap_sem].
{ tptr ptr; 
  if (header.fil_version != CURRENT_FIL_VERSION_NUM &&
      header.fil_version != 8+1)  // only new servers accept this, independent of version
    return;  // just a fuse
  void * handle = aligned_alloc(BLOCKSIZE, &ptr);
  if (!handle) return;
  memcpy(ptr, &header, sizeof(header));
  w_raw_write_block(header_blocknum, ptr);
  aligned_free(handle);
}

void t_ffa::truncate_database_file(tblocknum dadr_count)
{ tblocknum total_blocks = dadr_count + header.bpool_first;
  if (total_blocks < db_file_size)
  { for (int i=0;  i<fil_parts;  i++)
      if (total_blocks < fil_offset[i])  // truncate to zero length
      { SetFileSize(handle[i], 0);
        part_blocks[i]=0;
      }
      else if (i+1 == fil_parts || total_blocks < fil_offset[i+1])  // last part or the new end lies inside the part
      { part_blocks[i] = total_blocks - fil_offset[i];
        SetFileSize(handle[i], part_blocks[i]);
      }
    db_file_size = total_blocks;
   // decrease the number of bpools:
    unsigned new_bpool_count = DADR_COUNT2BPOOL_COUNT(dadr_count);
    { ProtectedByCriticalSection cs(&remap_sem);  // protects the access to the [header]
      if (header.bpool_count!=new_bpool_count)
      { header.bpool_count=new_bpool_count;
        write_header();
        if (disksp.next_pool>=header.bpool_count) disksp.next_pool=0;  // important - otherwise may allocte from non-existing pool!
      }
    }
   // close & open file parts in order to store the new length:
    close_fil_parts();  open_fil_parts();  get_database_file_size();
  }
}

bool t_ffa::set_fil_size(cdp_t cdp, tblocknum newblocks)  // LCK2
// Increases the database file size to total [newblocks] blocks, called inside disksp.bpools_sem
{ int i;  
  if (newblocks > 0xff000000LU) return true;  // bad argument
  if (db_file_size>=newblocks) return false; // must not write the last block if equal
 // add new bpools, if necessary:
  tblocknum dadrlimit= newblocks    - header.bpool_first;
  unsigned new_bpool_count = DADR_COUNT2BPOOL_COUNT(dadrlimit);
  full_bpool(corepool);
  { ProtectedByCriticalSection cs(&remap_sem);  // protects the access to the [header]
    if (header.bpool_count!=new_bpool_count)
    { for (i=header.bpool_count;  i<new_bpool_count;  i++)
        if (disksp.write_bpool(corepool, i)) return true;
      header.bpool_count=new_bpool_count;
      write_header();
    }
  }
 // increase the FILs (sys_write_block redirects, if no disk space!):
  for (i=0;  i<fil_parts-1;  i++)  // extending some parts to their maximal size
  { if (newblocks    < fil_offset[i+1]) break;  // break if this part will not get its maximal size
    if (db_file_size < fil_offset[i+1])         // if it did not have max size so far, expand
      if (w_raw_write_block(fil_offset[i+1]-1, corepool)) return true;
  }
 // now i is the last part or the part which will not get its mximal size:
  if (newblocks > fil_offset[i]) // write not necessary when newblocks==fil_offset[i]
    if (w_raw_write_block(newblocks-1, corepool))  // this may be the bpool, so corepool must be initialized!
      return true;
 // close & open file parts in order to store the new length:
  { ProtectedByCriticalSection cs(&fil_sem, cdp, WAIT_CS_FIL);
    close_fil_parts();  open_fil_parts();  get_database_file_size();
  }
 // check the space on the disk:
  uns32 space;
  get_server_info(cdp, OP_GI_DISK_SPACE, &space);  // space in kilobytes
  if (space/1024 < the_sp.DiskSpaceLimit.val())
    if (find_system_procedure(cdp, "_ON_SYSTEM_EVENT"))
    { t_value res;
      tobjnum orig_user = cdp->prvs.luser();
      cdp->prvs.set_user(CONF_ADM_GROUP, CATEG_GROUP, TRUE);  // execute with CONF_ADMIN privils (hierarchy is necessary, I need to be the Admin os _sysext)
      if (!exec_direct(cdp, "CALL _SYSEXT._ON_SYSTEM_EVENT(3,0,\'\')", &res)) commit(cdp);
      cdp->prvs.set_user(orig_user, CATEG_USER, TRUE);  // return to original privils (may be DB_ADMIN)
      commit(cdp);
    }
  return false;
}

void t_ffa::cdrom(cdp_t cdp)
{
 // fill the bpools:
  empty_bpool(corepool);
  for (unsigned i=0;  i<header.bpool_count;  i++)
    if (disksp.write_bpool(corepool, i))
      { request_error(cdp, OS_FILE_ERROR);  return; }
#ifndef FLUSH_OPTIZATION        
  blocks_in_buf=0;  // prevents releasing blocks on exit
#endif
  header.cd_rom_offset=header.bpool_count;
  write_header();
 // store the limit to the reg. db.:
  DWORD lim = header.bpool_count*8*(BLOCKSIZE/256);  // in MB
  SetDatabaseNum(server_name, "Limit1", lim, my_private_server);
}

////////////////////////// FIL access with redirection ///////////////////////////
#define REMAPS_PER_BLOCK (BLOCKSIZE/(2*sizeof(tblocknum)))
#define REMAP_RESERVE 10    /* number of free remap cells allocated at init. */

void t_ffa::get_remap_to(tblocknum ** to_remap,  unsigned * remap_count)
{ *remap_count=header.remapnum;  *to_remap=remap_to; }

int t_ffa::remap_init(void)
// Loads current remap state from FIL and inits remap structures/
// Uses corepool.
{ remapspace=header.remapnum + REMAP_RESERVE;
  remap_from=(tblocknum *)corealloc(remapspace*sizeof(tblocknum), OW_REMAP);
  remap_to  =(tblocknum *)corealloc(remapspace*sizeof(tblocknum), OW_REMAP);
  if (remap_from==NULL || remap_to==NULL) return KSE_NO_MEMORY;
  unsigned remap_rest=header.remapnum;  unsigned move_pos=0, i=0;
  while (remap_rest)   /* reading remap blocks */
  { if (w_raw_read_block(header.bpool_first+header.remapblocks[i], corepool))
      { header.remapnum-=remap_rest;  return KSE_DAMAGED; } 
    else
	  { unsigned move_amount = (remap_rest < REMAPS_PER_BLOCK) ?
          remap_rest * sizeof(tblocknum) : BLOCKSIZE/2;
      memcpy((tptr)remap_from + move_pos, corepool, move_amount);
      memcpy((tptr)remap_to   + move_pos, corepool+BLOCKSIZE/2, move_amount);
      move_pos += move_amount;
	    remap_rest -= move_amount/sizeof(tblocknum);
    }
  }
  return KSE_OK;
}

void t_ffa::mark_core(void)
{ mark(remap_from);  mark(remap_to); }

int t_ffa::resave_remap(void)
{ tptr rmt=(tptr)corealloc(BLOCKSIZE, OW_REMAP);
  if (rmt==NULL) return OUT_OF_KERNEL_MEMORY;
  int remap_block_num = 0;
  do
  { memcpy(rmt,            remap_from+remap_block_num*REMAPS_PER_BLOCK, BLOCKSIZE/2);
    memcpy(rmt+BLOCKSIZE/2,remap_to  +remap_block_num*REMAPS_PER_BLOCK, BLOCKSIZE/2);
    if (header.remapblocks[remap_block_num]) /* remap block is allocated */
      if (w_raw_write_block(header.bpool_first+header.remapblocks[remap_block_num], rmt))
        return OUT_OF_BLOCKS;
  } while (header.remapnum > ++remap_block_num * REMAPS_PER_BLOCK);
  corefree(rmt);
  return 0;
}

int t_ffa::add_to_remap(cdp_t cdp, tblocknum from_place, tblocknum to_place)
/* adds a new item to the remap table and stores it to the disc */
/* if from_place == -1, add this remap, but it means marking an unallocated
   block as bad */
{ tblocknum * r_from, * r_to;
  unsigned remap_block_num;
  ProtectedByCriticalSection cs(&remap_sem);
  if (header.remapnum+1 >= remapspace)
  { r_from=(tblocknum*)corealloc((remapspace+REMAP_RESERVE)*sizeof(tblocknum), OW_REMAP);
    r_to=  (tblocknum*)corealloc((remapspace+REMAP_RESERVE)*sizeof(tblocknum), OW_REMAP);
    if (r_to && r_from)
    { memcpy(r_from, remap_from, remapspace*sizeof(tblocknum));
      memcpy(r_to  , remap_to,   remapspace*sizeof(tblocknum));
      corefree(remap_to);  corefree(remap_from);
      remap_from=r_from;  remap_to=r_to;
      remapspace += REMAP_RESERVE;
    }
    else return OUT_OF_KERNEL_MEMORY; 
  }
  remap_from[header.remapnum]=from_place;
  remap_to  [header.remapnum]=to_place;
  remap_block_num=header.remapnum / REMAPS_PER_BLOCK;
  if (remap_block_num >= REMAPBLOCKSNUM)  /* bad error */
    return OUT_OF_BLOCKS; 
  header.remapnum++;
 /* save the remap table */
  int res=resave_remap();
  if (res) return res; 
  write_header(); /* changes in remapnum &/or remapblocks */
  return 0;
}

tblocknum t_ffa::translate_blocknum(tblocknum logblocknum)
/* translate the block number */
{ tblocknum phys_block;  
  if (!header.remapnum) return logblocknum;  // this makes the typical situation faster
  { ProtectedByCriticalSection cs(&remap_sem);
    remap_from[header.remapnum]=logblocknum;
    unsigned remap_ind=0;
    while (remap_from[remap_ind]!=logblocknum) remap_ind++;
    if (remap_ind==header.remapnum) phys_block=logblocknum;
    else phys_block=remap_to[remap_ind];
  }
  return phys_block;
}

BOOL t_ffa::sys_write_block_noredir(const void * from_where, tblocknum where_to)  
// Writes a memory block to the disc
{ //if (where_to == 3+header.bpool_first)
  //{ tptr p = from_where;
  //  for (int i = 0;  i<tabtab_descr->Recnum() && i<0x21;  i++, p+=0x7a)
  //    ;
  //}
  return w_raw_write_block(translate_blocknum(where_to), from_where); 
}

BOOL t_ffa::sys_write_block(cdp_t cdp, const void * from_where, tblocknum where_to)  // LCK2
// writes a memory block to the disc in a safe way - redirecting
// when disc errors occur 
// add_to_remap() must be called ouside [fil_sem] because it enters [remap_sem].
{ tblocknum phys_where_to, new_place;  int res;
 /* translate the block number */
  phys_where_to=translate_blocknum(where_to);
  if (!w_raw_write_block((uns32)phys_where_to, from_where)) return FALSE;
  if (!CheckCriticalSection(&disksp.bpools_sem)) return TRUE;
 /* write to another block and make redirection */
  int count=3;
  do
  { new_place=disksp.get_block(cdp); /* system allocation, must not be rolled back */
    if (!new_place) return TRUE;
    new_place+=header.bpool_first;   /* converted to the system block number */
    if (!w_raw_write_block(new_place, from_where))
      break;
    res=add_to_remap(cdp, (tblocknum)-1, new_place);  // mark as bad
    if (res) { request_error(cdp, res);  return TRUE; } 
    if (!count--) { request_error(cdp, OUT_OF_BLOCKS);  return TRUE; }
  } while (TRUE);
 // change existing redirection...
  if (where_to!=phys_where_to)   
  { unsigned remap_ind=0;
	  while (remap_from[remap_ind]!=where_to) remap_ind++;
    remap_to[remap_ind]=new_place;
   // add the old redirection as a bad block to the redirection:
    res=add_to_remap(cdp, (tblocknum)-1, phys_where_to);  /* old redir_block disabled */
  }
 // ... or add a new redirection: 
  else   
    res=add_to_remap(cdp, where_to, new_place);
  if (res) { request_error(cdp, res);  return TRUE; }
  return FALSE;
}

BOOL t_ffa::read_block(cdp_t cdp, tblocknum from_where, tptr where_to)
{ prof_time start;
  if (PROFILING_ON(cdp)) start = get_prof_time();  
  BOOL res = sys_read_block(from_where+header.bpool_first, where_to); 
  if (PROFILING_ON(cdp)) add_hit_and_time(get_prof_time()-start, PROFCLASS_ACTIVITY, PROFACT_DISC_READ, 0, NULL);
  cdp->cnt_pagerd++;
  return res;
}

BOOL t_ffa::write_block(cdp_t cdp, tptr from_where, tblocknum where_to)
{ prof_time start;
  if (PROFILING_ON(cdp)) start = get_prof_time();  
  BOOL res = sys_write_block(cdp, from_where, where_to+header.bpool_first); 
  if (PROFILING_ON(cdp)) add_hit_and_time(get_prof_time()-start, PROFCLASS_ACTIVITY, PROFACT_DISC_WRITE, 0, NULL);
  cdp->cnt_pagewr++;
  return res;
}

////////////////////////////// allocating blocks /////////////////////////////////
#define POOL_FROM_DADR(dadr) ((dadr)/ (8L * BLOCKSIZE))

#ifndef FLUSH_OPTIZATION        
static char DBL_BLOCK2[] = "Releasing a free block 2!";
static char DBL_BLOCK[]  = "Releasing a free block!";

BOOL t_ffa::write_bpool(uns8 * bpool, int poolnum)
{ return sys_write_block_noredir(bpool, header.bpool_first+(tblocknum)poolnum*8*BLOCKSIZE); }

BOOL t_ffa::read_bpool(int poolnum, uns8 * bpool)
{ return sys_read_block(header.bpool_first+(tblocknum)poolnum*8*BLOCKSIZE, bpool); }

void t_ffa::insert_block(cdp_t cdp, tblocknum dadr)
/* inserts dadr-block info free block buffer preserving the ordering for
   max to min. There must be free space there! */
{ unsigned short i;
  blockbuf[blocks_in_buf]=dadr;   /* sentinel */
  i=0;
  while (blockbuf[i] > dadr) i++;
  if (blockbuf[i] == dadr)
  { if (i<blocks_in_buf)  /* dadr is already in the buffer! */
    { char msg[100];
      sprintf(msg, "Releasing a free block %u!", dadr);
      if (the_sp.down_conditions.val() & REL_NONEX_BLOCK)
        fatal_error(cdp, msg);  
      else
        cd_dbg_err(cdp, msg);  
      return; 
    }   /* else dadr is on the right place */
  }
  else /* i is the right index for inserting dadr */
  { memmov(blockbuf+i+1, blockbuf+i, (blocks_in_buf-i)*sizeof(tblocknum));
	  blockbuf[i]=dadr;
  }
  blocks_in_buf++;
}

#define BLOCK_FILL_LIMIT  (BLOCK_BUFFER_SIZE-10)

tblocknum t_ffa::get_block(cdp_t cdp)  /* returns a new block, 0 if cannot */  // LCK2
/* When new blocks are added into the free block cache, FIL is extended to contain it.
   Otherwise if there if no space on the disk, the error would be found
     during commit and an inconsistence in the db would be created.
   W98:
   If FIL is not extended and system crashes, file retains its original size 
     and lost clusters are reported.
   If FIL is extended by SetEndOfFile only and system crashes, difference in file size is reported
     on next startup and it may be corrected.
   Only closing and re-openning the file guarantees the presistent increase in its size.
*/
{ tblocknum theblock, max_phys_block = 0;
  { ProtectedByCriticalSection cs(&disksp.bpools_sem, cdp, WAIT_CS_BPOOLS);
    if (!blocks_in_buf)
    { unsigned int cyc=0;  int i, j;  BOOL change;
      do
      { if (read_bpool(next_pool, corepool)) // unreadable pool: must be after the end of the database file, bad compacting
          full_bpool(corepool);
       // search the corepool for the free blocks:   
        i=0; j=0; change=FALSE;
        do
        { if (!corepool[i]) j=7;  /* byte empty, do not check its bits */
          else
            if (corepool[i] & (1<<j))  /* block not allocated yet */
            { corepool[i] ^= (1<<j);
              change=TRUE;
              insert_block(cdp, ((tblocknum)next_pool * BLOCKSIZE + i) * 8 + j);
              if (blocks_in_buf>=BLOCK_FILL_LIMIT) break;
            }
          if (++j==8)
          { j=0;
            if (++i==BLOCKSIZE) break;
          }
        } while (true);
        if (change)
        { if (write_bpool(corepool, next_pool)) goto error;
          break;
        }
       // not allocated enough from the pool, checking the next one:
        { ProtectedByCriticalSection cs(&remap_sem);  // protects the access to the [header]
          if (++next_pool==header.bpool_count) next_pool=0;  /* when pool empty only! */
          if (++cyc>header.bpool_count)
          { full_bpool(corepool);
            if (write_bpool(corepool, header.bpool_count)) goto error;
	          next_pool=header.bpool_count++;
            write_header();
            continue;
          }
        }
      } while (!blocks_in_buf); /* core buffer empty, allocate from disc pool */
     // find the last of the blocks put into the cache:
      for (i=0;  i<blocks_in_buf;  i++)
      { tblocknum phys_block=translate_blocknum(header.bpool_first+blockbuf[i]);
        if (phys_block>max_phys_block) max_phys_block=phys_block;
      }
    } // cache empty
    theblock = blockbuf[--blocks_in_buf];
   // extending the database file(s), if any added block is outside:
    if (backup_support)  // if the file is frozen for backup
    { if (backup_support->new_db_file_size <= max_phys_block)
        backup_support->new_db_file_size = max_phys_block+1;
    }
    else
    { if (max_phys_block >= db_file_size) 
      { unsigned step = max_phys_block + 1 - db_file_size;
        unsigned minstep = db_file_size < 25600 ? 256 : 2560;
        if (step<minstep) step=minstep;
        if (set_fil_size(cdp, db_file_size+step))
          if (set_fil_size(cdp, max_phys_block+1)) 
            goto error;
      }
    }
  } // CS
  return theblock;  /* takes the minimal block number */

 error: // error processing must be outside the CS:
  request_error(cdp, OUT_OF_BLOCKS);  
  blocks_in_buf=0;  
  return 0;
}

tblocknum t_ffa::get_big_block(cdp_t cdp)  /* returns a new big block, 0 if cannot */  // LCK2
/* When a new block is returned, FIL is extended to contain it.
   Reasons:
   - if there if no space on the disk, the error would be found
     during commit and an inconsistence in the db would be created;
   - reading errors can be checked correctly now. */
{ tblocknum dadr;
  { ProtectedByCriticalSection cs(&disksp.bpools_sem, cdp, WAIT_CS_BPOOLS);
    unsigned int cyc=0;  BOOL found = FALSE;  
    do // cycle on pools
    { if (read_bpool(next_pool, corepool))
        full_bpool(corepool);
      unsigned start = 0, size;
      while (start <= BLOCKSIZE - SMALL_BLOCKS_PER_BIG_BLOCK/8)
      {// find number of ff bytes:
        size=0;
        while (size < SMALL_BLOCKS_PER_BIG_BLOCK/8 && corepool[start+size]==0xff) size++;
        if (size == SMALL_BLOCKS_PER_BIG_BLOCK/8) { found=TRUE;  dadr = ((tblocknum)next_pool * BLOCKSIZE + start) * 8;  break; }
        start+=size+1;
      }
      if (found)
      { memset(corepool+start, 0, size);
        if (write_bpool(corepool, next_pool)) goto error;
        break;
      }
      { ProtectedByCriticalSection cs(&remap_sem);  // protects the access to the [header]
        if (++next_pool==header.bpool_count) next_pool=0;  /* when pool empty only! */
        if (++cyc>header.bpool_count)
        { full_bpool(corepool);
          if (write_bpool(corepool, header.bpool_count))
            { blocks_in_buf=0;  goto error; }
  	      next_pool=header.bpool_count++;
          write_header();
        }
      }
    } while (true); /* core buffer empty, allocate from disc pool */
   // expand FIL, if necessary:
    if (header.bpool_first+dadr+SMALL_BLOCKS_PER_BIG_BLOCK > db_file_size)
      if (set_fil_size(cdp, header.bpool_first+dadr+SMALL_BLOCKS_PER_BIG_BLOCK))
        goto error;
  } // CS
  return dadr;

 error: // error processing must be outside the CS:
  request_error(cdp, OUT_OF_BLOCKS);  
  return 0;
}

void t_ffa::save_blocks(cdp_t cdp, unsigned pool)
// Saves blocks belonging to the given pool. bpools_sem is supposed to be down. 
{ unsigned i, j;  bool mask_doubling=false;
  if (read_bpool(pool, corepool))   // may be a compacting problem, ignore
    { full_bpool(corepool);  mask_doubling=true; }
  /* unreadable pool is considered to be empty */
  for (i=0;  i<blocks_in_buf; )
    if (POOL_FROM_DADR(blockbuf[i]) == pool)
    { j=blockbuf[i] % (8L * BLOCKSIZE);
      if (corepool[j / 8] & (1 << (j%8))) 
        if (!mask_doubling)
        { char msg[100];
          sprintf(msg, "Releasing 2 a free block %u!", blockbuf[i]);
          if (the_sp.down_conditions.val() & REL_NONEX_BLOCK)
            fatal_error(cdp, msg);  
          else
            cd_dbg_err(cdp, msg);  
        }   
		  corepool[j / 8] |= 1 << (j%8);
      memmov(blockbuf+i, blockbuf+i+1, (--blocks_in_buf-i)*sizeof(tblocknum));
      /* ordering preserved */
    }
    else i++;
  write_bpool(corepool, pool);  /* no need to check error */
 // prevent growing the database file when there are free blocks in the [pool]:
  if (pool<next_pool) next_pool=pool;
}

void t_ffa::save_working_blocks(cdp_t cdp)
{ while (blocks_in_buf)   /* save free blocks from core buffer */
    save_blocks(cdp, blockbuf[0] / (8L * BLOCKSIZE));
}

void t_ffa::get_block_buf(tblocknum ** buf,  unsigned * count)
{ *count=blocks_in_buf;  *buf=blockbuf; }

void t_ffa::release_block(cdp_t cdp, tblocknum dadr)   /* frees a disc block */    // LCK2
{ unsigned best_pool, max_pool_blocks, curr_pool, blocks_in_curr, i;
  if (POOL_FROM_DADR(dadr) >= header.bpool_count ||
      dadr % (8L * BLOCKSIZE) == 0)  // releasing bpool, severe error
  { char msg[100];
    sprintf(msg, "Releasing a nonexisting block %u (%u bpools)", dadr, header.bpool_count);
    if (the_sp.down_conditions.val() & REL_NONEX_BLOCK)
      fatal_error(cdp, msg);  
    else
      cd_dbg_err(cdp, msg);  
    return; 
  }
  if (!releasing_the_page(cdp, dadr))  // release any ownership (I may have my private copy)
    return;  // internal error, somebody has a private copy of the page
  { ProtectedByCriticalSection cs(&disksp.bpools_sem, cdp, WAIT_CS_BPOOLS);
    if (blocks_in_buf==BLOCK_BUFFER_SIZE)
    { max_pool_blocks=0;  best_pool=0;  // init. to avoid a warning
      for (curr_pool=0; curr_pool<header.bpool_count; curr_pool++)
      { blocks_in_curr=0;
        for (i=0; i<BLOCK_BUFFER_SIZE; i++)
          if (POOL_FROM_DADR(blockbuf[i]) == curr_pool) blocks_in_curr++;
        if (blocks_in_curr>max_pool_blocks)
        { max_pool_blocks=blocks_in_curr;  best_pool=curr_pool; }
        if (max_pool_blocks > BLOCK_BUFFER_SIZE/3) break;
      }
      save_blocks(cdp, best_pool);
    }
    insert_block(cdp, dadr);   /* preserved ordering of blocks */
  }
}

tblocknum t_ffa::free_blocks(cdp_t cdp)  // LCK2
// Returns the number of free bloks in the FIL (bpools & memory buffer)
{ tblocknum count;  tblocknum dadr, dadrlimit;
  if (!FILE_HANDLE_VALID(handle[0])) return (tblocknum)-1;  // when called from server window, files may be closed
  dadrlimit = db_file_size - header.bpool_first;
  ProtectedByCriticalSection cs(&disksp.bpools_sem, cdp, WAIT_CS_BPOOLS);
  count=blocks_in_buf;  dadr=0;
  for (int poolnum=0;  poolnum<header.bpool_count;  poolnum++)
    if (!read_bpool(poolnum, corepool))
    { for (unsigned cnt=0;  cnt<BLOCKSIZE;  cnt++)
        if (!corepool[cnt]) dadr+=8;
        else
          for (unsigned i=0;  i<8;  i++, dadr++)
            if (corepool[cnt] & (1<<i))
              if (dadr >= dadrlimit) goto limit_reached;
              else count++;
    }
 limit_reached:
  return count;
}
#endif  // !FLUSH_OPTIZATION        

////////////////////////////// init & close ///////////////////////////////////
CRITICAL_SECTION crs_sem, cs_client_list;

char def_password[] = "To je tajne heslo WinBase602 5.0";

void t_ffa::init(void)  // Makes it possible to open/close FIL.
{ InitializeCriticalSection(&remap_sem);  
  InitializeCriticalSection(&fil_sem);  
  InitializeCriticalSection(&c_log_cs);
}

void t_ffa::deinit(void)
{ DeleteCriticalSection(&remap_sem);  
  DeleteCriticalSection(&fil_sem);  
  DeleteCriticalSection(&c_log_cs);
}

#define PROTOCOL_SIZE_LIMIT  200000000

void t_ffa::to_protocol(char * buf, int len)
{ DWORD wr;  char fname[MAX_PATH];  bool open_new = false;
  ProtectedByCriticalSection cs(&c_log_cs);
  if (FILE_HANDLE_VALID(ffa.hProtocol)) 
    wr=SetFilePointer(hProtocol, 0, NULL, FILE_END);
  else open_new=true;
  if (open_new || wr>=PROTOCOL_SIZE_LIMIT)
  { if (!open_new) CloseHandle(hProtocol);
    strcpy(fname, fil_path[fil_parts-1]);  
    if (protocol2)
    { append(fname, "wb8prot1.txt");
      protocol2=false;
    }
    else
    { append(fname, "wb8prot2.txt");
      protocol2=true;
    }
    if (!open_new) DeleteFile(fname);
#ifdef WINS
    hProtocol=CreateFile(fname, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE,
      NULL, OPEN_ALWAYS, 0, NULL);
#else
    hProtocol=_lopen(fname, OF_READWRITE|OF_SHARE_DENY_NONE);
#endif
    if (FILE_HANDLE_VALID(hProtocol))
      SetFilePointer(hProtocol, 0, NULL, FILE_END);
  }
  WriteFile(hProtocol, buf, len, &wr, NULL);
}

int t_ffa::disc_open(cdp_t cdp, BOOL & fil_closed_properly)
// Main initialization of ffa, if succeded, ffa.close must be called later
{ int res;
 /* level 0 initialization: read header */
  res=read_header();
  if (res==KSE_OK)
  { get_database_file_size();
    fil_closed_properly = !header.sezam_open;
    if (header.fil_crypt==fil_crypt_fast || header.fil_crypt==fil_crypt_slow || header.fil_crypt==fil_crypt_des)
    { char fil_password[MAX_FIL_PASSWORD+1];
      if (!get_database_file_password(fil_password)) return KSE_ESCAPED;  
      acrypt.i.set_crypt((t_fil_crypt)header.fil_crypt, fil_password);
      acrypt.o.set_crypt((t_fil_crypt)header.fil_crypt, fil_password);
    }
    else if (header.fil_crypt==fil_crypt_simple)
    { acrypt.i.set_crypt((t_fil_crypt)header.fil_crypt, def_password);
      acrypt.o.set_crypt((t_fil_crypt)header.fil_crypt, def_password);
    }
   /* level 1 initialization: remaps */
    res=remap_init();
    if (res==KSE_OK)
    {// check the proper decrypting:
      ffa.w_raw_read_block(header.bpool_first+3, corepool);
      if (header.fil_crypt!=fil_crypt_no && strcmp(((ttrec*)corepool)->name, "TABTAB"))
        res=KSE_BAD_PASSWORD;
      else
      { if (header.sezam_open) header.prealloc_gcr=0;  
        header.sezam_open=wbtrue;   write_header();
        return KSE_OK;
      }
    } /* remaps allocated */
  } /* header read OK */
  return res;
}

void t_ffa::disc_close(cdp_t cdp)
{// closing & saving ffa structures:
  disksp.close_block_cache(cdp);
  header.sezam_open=wbfalse;
  header.closetime=stamp_now();
  write_header();
 // wait for completing the hot-backup, if in progress (comes here on when the backup is called from the client): 
  if (backup_support)
  { display_server_info("Waiting until the hot-backup is completed...");
    while (backup_support)
     Sleep(100);
  }   
}

t_ffa ffa;

//////////////////////////////////////////////////////////////////////////////////
header_structure header;

void write_gcr(cdp_t cdp, tptr dest)
{ ProtectedByCriticalSection cs(&cs_gcr);  // protects the access to the [header]
  if (!header.prealloc_gcr) 
  { t_zcr store;  memcpy(store, header.gcr, ZCR_SIZE);
    if (!++header.gcr[4]) if (!++header.gcr[3]) if (!++header.gcr[2]) if (!++header.gcr[1]) ++header.gcr[0];
    ffa.write_header();
    header.prealloc_gcr=256;
    memcpy(header.gcr, store, ZCR_SIZE);
  }
  if (!++header.gcr[5]) if (!++header.gcr[4]) if (!++header.gcr[3]) 
  if (!++header.gcr[2]) if (!++header.gcr[1]) ++header.gcr[0];
  memcpy(dest, header.gcr, ZCR_SIZE);
  header.prealloc_gcr--;
}

void update_gcr(cdp_t cdp, const char * zcr_val)
// updates gcr so that it is bigger than zcr_val
{ ProtectedByCriticalSection cs(&ffa.remap_sem);  // protects the access to the [header]
  memcpy(header.gcr, zcr_val, ZCR_SIZE);
  header.gcr[5]=0;
  if (!++header.gcr[4]) if (!++header.gcr[3]) if (!++header.gcr[2]) if (!++header.gcr[1]) ++header.gcr[0];
  header.prealloc_gcr=0;
  ffa.write_header();
}
/********************* transaction level block allocation *******************/
void t_translog::free_block(cdp_t cdp, tblocknum dadr)
// Releases the block on the transaction level 
{ log.drop_page_changes(cdp, dadr);
  if (log.page_deallocated(dadr)) // allocated in this transaction
  { page_unlock(cdp, dadr, ANY_LOCK);  // remove all own locks from the block
    disksp.release_block_safe(cdp, dadr);   // allocated in the same transaction
    trace_alloc(cdp, dadr, false, "alloc in same trans");
  }
}

void t_translog_main::free_index_block(cdp_t cdp, tblocknum root, tblocknum dadr)
// Releases the block on the transaction level 
{ 
  if (index_page_freed(cdp, root, dadr))
  { disksp.release_block_safe(cdp, dadr);   // allocated in the same transaction
    trace_alloc(cdp, dadr, false, "alloc in same trans index");
  }
}

void t_translog_cursor::free_block(cdp_t cdp, tblocknum dadr)
{ log.drop_page_changes(cdp, dadr);
  page_unlock(cdp, dadr, ANY_LOCK);  // remove all own locks from the block
  disksp.release_block_direct(cdp, dadr);
}

void t_translog_cursor::free_index_block(cdp_t cdp, tblocknum root, tblocknum dadr)
{ log.drop_page_changes(cdp, dadr);
  disksp.release_block_direct(cdp, dadr);
}

void t_translog_loctab::free_index_block(cdp_t cdp, tblocknum root, tblocknum dadr)
// Non-index logging for temp. tables.
{ free_block(cdp, dadr); }

tblocknum t_translog::alloc_block(cdp_t cdp)  
// Allocates a free block on the transaction level
{ tblocknum bl = disksp.get_block(cdp);
  if (bl) log.page_allocated(bl); 
  trace_alloc(cdp, bl, true, "trans");
  return bl;
}

tblocknum t_translog_main::alloc_index_block(cdp_t cdp, tblocknum root)  
// Allocates a free block for index with the [root] on the transaction level
// Not used for the root of the index.
{ tblocknum bl = disksp.get_block(cdp);
  if (bl) 
    index_page_allocated(cdp, root, bl);
  trace_alloc(cdp, bl, true, "index");
  return bl;
}


tblocknum t_translog_cursor::alloc_block(cdp_t cdp)  
// Allocates a free block, no logging for cursors
{ return disksp.get_block(cdp); }

tblocknum t_translog_cursor::alloc_index_block(cdp_t cdp, tblocknum root)  
// Allocates a free block, no logging for cursors
{ return alloc_block(cdp); }

tblocknum t_translog_loctab::alloc_index_block(cdp_t cdp, tblocknum root)  
// Allocates a free block, non-index logging for temp. tables
{ return alloc_block(cdp); }

tblocknum t_translog::alloc_big_block(cdp_t cdp)  
// Allocates a big free block on the transaction level
{ tblocknum bl = disksp.get_big_block(cdp);
  if (bl) 
    for (unsigned i = 0;  i<SMALL_BLOCKS_PER_BIG_BLOCK;  i++)
      log.page_allocated(bl+i);
  return bl;
}

tblocknum t_translog_cursor::alloc_big_block(cdp_t cdp)  
// Allocates a big free block, no logging for cursors
{ return disksp.get_big_block(cdp); }

void t_translog::free_big_block(cdp_t cdp, tblocknum dadr)
{ for (unsigned i = 0;  i<SMALL_BLOCKS_PER_BIG_BLOCK;  i++)
    free_block(cdp, dadr+i);  // polymorphic!
}

#ifdef IMAGE_WRITER
///////////////////////////////////// image writer //////////////////////////////
// ATTN: The class works with fcbs, it must be called in cs_frame or changed.

t_image_writer the_image_writer;
 
THREAD_PROC(ImageWriterThread)
{
  the_image_writer.image_writer_thread();
  THREAD_RETURN;
}

bool t_image_writer::wait_for_releasing_frame(void)
{ 
  WaitPulsedManualEvent(&block_written_event, INFINITE);
  return true;
}

bool t_image_writer::read_from_queue(tblocknum dadr, tptr frame)
// Returns true if found and read, false when not found.
{
  { ProtectedByCriticalSection cs(&writer_cs);
    if (head==-1) return false;
    int pos=head;
    while (items[pos].dadr!=dadr)
    { if (pos==tail) return false;
      if (++pos>=MAX_WRITE_BLOCK_COUNT) pos=0;
    }
   // found, read:
    if (items[pos].is_in_core)  // is fixed
      memcpy(frame, FRAMEADR(items[pos].fcbn), BLOCKSIZE);
    else
      transactor.load_block(frame, fcbs[items[pos].fcbn].fc.swapdadr);
  }
  return true;
}

void t_image_writer::add_item(tfcbnum fcbn, bool is_in_core)
{
  do
  { 
    { ProtectedByCriticalSection cs(&writer_cs);
      int new_tail;
      if (head==-1)
        head=new_tail=0;
      else
      { new_tail=tail+1;
        if (new_tail>=MAX_WRITE_BLOCK_COUNT) new_tail=0;
        if (new_tail==head) goto full;  // queue is full
      }
      tail=new_tail;
      items[tail].fcbn=fcbn;  items[tail].is_in_core=is_in_core;  items[tail].dadr=fcbs[fcbn].blockdadr;
      SetLocalAutoEvent(&new_write_request_event);
      break;
    }
   full: 
    WaitPulsedManualEvent(&block_written_event, INFINITE);
  } while (true);
}

void t_image_writer::write_to_queue(tblocknum dadr, tptr frame)
{
}

void t_image_writer::write_item(tfcbnum fcbn, bool is_in_core)
{
  tblocknum dadr = fcbs[fcbn].blockdadr;
  //if (is_in_core)  ;

}

void t_image_writer::write_until_queue_empty(void)
{
  do
  { tfcbnum fcbn;  bool is_in_core;
    { ProtectedByCriticalSection cs(&writer_cs);
      if (head==-1) break;
      fcbn=items[head].fcbn;  is_in_core=items[head].is_in_core;
    }
   // write the block outside [writer_cs] in order not to block the other threads inserting items into the queue
    write_item(fcbn, is_in_core);
   // remove the written info from [items] - must not be done before the disc write, because other thread may read it
   // Note: nobody could have changed items[head] while I was outside [writer_cs].
    { ProtectedByCriticalSection cs(&writer_cs);
      //if (
      if (head==tail) head=tail=-1;
      else if (++head>=MAX_WRITE_BLOCK_COUNT) head=0;
    }
   // return fcbn to the free pool:


    PulseEvent(block_written_event);  // called after the space in [items] is made free and core frame is unfixed
  } while (true);
}

void t_image_writer::image_writer_thread(void)
{
  do
  { WaitLocalAutoEvent(&new_write_request_event, INFINITE);  // will be signalled on deinit()
    write_until_queue_empty();
  } while (!deinit_request);
}

void t_image_writer::init(void)
{ head=tail=-1;
  deinit_request=false;
  InitializeCriticalSection(&writer_cs);
  CreateLocalAutoEvent(FALSE, &new_write_request_event);
  CreateLocalManualEvent(FALSE, &block_written_event);
  THREAD_START_JOINABLE(ImageWriterThread, 0, NULL, &thread_handle);  // thread handle closed inside
}

void t_image_writer::deinit(void)
{
  deinit_request=true;
  SetLocalAutoEvent(&new_write_request_event);   // writes all and terminates the writer thread
  THREAD_JOIN(thread_handle);
  CloseLocalManualEvent(&block_written_event);
  CloseLocalAutoEvent(&new_write_request_event);
  DeleteCriticalSection(&writer_cs);
}
#endif

#ifdef STOP
void trace_alloc(cdp_t cdp, tblocknum dadr, bool alloc, const char * descr)
{ char buf[200];
  sprintf(buf, "%s by cli %d block >%u<: %s", alloc ? "A" : "D", cdp->applnum_perm, dadr, descr);
  dbg_err(buf);
}
#endif

///////////////////////////////////////////////////////////////////////////////////////////////////////
// Alocation of space in the database file


t_disk_space_management disksp;

t_disk_space_management::t_disk_space_management(void)
{ InitializeCriticalSection(&bpools_sem);
  predeallocated_blocks_cnt=free_blocks_cnt=pre_free_pieces_cnt=0;
  next_pool = 0;
  last_fil_flush = 0;
  stop_flushing = 0;
  locked_by = (t_client_number)-1;  
}

t_disk_space_management::~t_disk_space_management(void)
{ DeleteCriticalSection(&bpools_sem);
}

BOOL t_disk_space_management::write_bpool(uns8 * bpool, int poolnum)
{ return ffa.sys_write_block_noredir(bpool, header.bpool_first+(tblocknum)poolnum*8*BLOCKSIZE); }

BOOL t_disk_space_management::read_bpool(int poolnum, uns8 * bpool)
{ return ffa.sys_read_block(header.bpool_first+(tblocknum)poolnum*8*BLOCKSIZE, bpool); }

void t_disk_space_management::get_block_buf(tblocknum ** buf, unsigned * count)
// Returns the list of free blocks
{ 
#ifndef FLUSH_OPTIZATION        
  ffa.get_block_buf(buf, count);
#else  
  *count=free_blocks_cnt;  *buf=free_blocks.acc0(0);
#endif
}

void t_disk_space_management::release_block_safe(cdp_t cdp, tblocknum dadr)
{ 
#ifndef FLUSH_OPTIZATION        
  ffa.release_block(cdp, dadr);
#else  
  if (!the_sp.FlushPeriod.val())
    release_block_direct(cdp, dadr);
  else  
  { assert(dadr!=0);
    ProtectedByCriticalSection cs(&bpools_sem, cdp, WAIT_CS_BPOOLS);
    *pre_free_blocks.acc(predeallocated_blocks_cnt++) = dadr;
  }  
#endif
}

void t_disk_space_management::release_block_direct(cdp_t cdp, tblocknum dadr)
// Deallocation of blocks belongigng to objects which will diappear if server crashes
{ 
#ifndef FLUSH_OPTIZATION        
  ffa.release_block(cdp, dadr);
#else  
  assert(dadr!=0);
  releasing_the_page(cdp, dadr);  // removes private copies of the page
  ProtectedByCriticalSection cs(&bpools_sem, cdp, WAIT_CS_BPOOLS);
  *free_blocks.acc(free_blocks_cnt++) = dadr;
#endif
}

void t_disk_space_management::release_piece_safe(cdp_t cdp, tpiece * pc)
{ 
#ifndef FLUSH_OPTIZATION        
  release_piece(cdp, pc, true);
#else  
  ProtectedByCriticalSection cs(&bpools_sem, cdp, WAIT_CS_BPOOLS);
  if (!the_sp.FlushPeriod.val())
    locked_release_piece(cdp, pc, true);
  else
    *pre_free_pieces.acc(pre_free_pieces_cnt++) = *pc;
#endif
}

void t_disk_space_management::deallocation_move(cdp_t cdp)
// Moves predeallocated blocks among free blocks. Flush must be done before any of these blocks can be allocated.
{ int i;
 // move pieces (do it first, may create new predeallocated blocks): 
  for (i=0;  i<pre_free_pieces_cnt;  i++)
    locked_release_piece(cdp, pre_free_pieces.acc0(i), true);
  if (pre_free_pieces.count() > 100)
    pre_free_pieces.destruct();
  pre_free_pieces_cnt=0;  
 // move blocks:
  while (predeallocated_blocks_cnt)
    *free_blocks.acc(free_blocks_cnt++) = *pre_free_blocks.acc0(--predeallocated_blocks_cnt);
  if (pre_free_blocks.count() > 100)
    pre_free_blocks.destruct();
}

void t_disk_space_management::save_to_pool(unsigned poolnum, unsigned margin)
// Saves all blocks from [free_blocks] which belong to the [poolnum].
{ unsigned i, j;  
  if (read_bpool(poolnum, corepool))   // may be a compacting problem, ignore
    empty_bpool(corepool);  
  /* unreadable pool is considered to be empty */
  for (i=0;  i<free_blocks_cnt; )
  { tblocknum dadr = *free_blocks.acc0(i);
    if (POOL_FROM_DADR(dadr) == poolnum)
    { j = dadr % (8L * BLOCKSIZE);
      if (corepool[j / 8] & (1 << (j%8))) 
      { char msg[100];
        sprintf(msg, "Releasing 2 a free block %u!", dadr);
        if (the_sp.down_conditions.val() & REL_NONEX_BLOCK)
          fatal_error(NULL, msg);  
        else
          cd_dbg_err(NULL, msg);  
      }   
		  corepool[j / 8] |= 1 << (j%8);
		 // remove the block from [free_blocks]: 
		  free_blocks_cnt--;
		  if (i<free_blocks_cnt)
		    *free_blocks.acc0(i) = *free_blocks.acc0(free_blocks_cnt);
		  else
		    i++;  
		  if (free_blocks_cnt <= margin) break;
		}
		else i++;
  }		  
  write_bpool(corepool, poolnum);  /* no need to check error */
 // prevent growing the database file when there are free blocks in the [pool]:
  if (poolnum<next_pool) next_pool=poolnum;
}

void t_disk_space_management::reduce_free_blocks(void)
// Ensures that the number of free blocks does not exceed FREE_LIMIT_UP.
{ int i;
  if (free_blocks_cnt<=FREE_LIMIT_UP) return;
 // create the statistics per pool:
  t_blocknum_dynar in_pool_count;
  for (i=0;  i<free_blocks_cnt;  i++)
    (*in_pool_count.acc(POOL_FROM_DADR(*free_blocks.acc0(i))))++;
  while (free_blocks_cnt>FREE_LIMIT_UP)
  {// find the best pool:
    int best_pos, best_val=0;
    for (i=0;  i<in_pool_count.count();  i++)
      if (*in_pool_count.acc0(i) > best_val)
        { best_pos=i;  best_val=*in_pool_count.acc0(i); }
    save_to_pool(best_pos, FREE_LIMIT_DOWN);  
    *in_pool_count.acc0(best_pos)=0;  
  }    
}

void t_disk_space_management::extflush(cdp_t cdp, bool unconditional)
// Commit flush.
{ 
  if (stop_flushing) return;  // during import, ALTER TABLE
  uns32 tm=clock_now();  // seconds from the start of the day
  if (!unconditional)  // periodic flushing (called only in commit)
    if (!the_sp.FlushPeriod.val() ||  // do not flush
        tm<last_fil_flush+the_sp.FlushPeriod.val() && tm>=last_fil_flush) // flushed recently
      return;
  { last_fil_flush=tm;
#ifdef FLUSH_OPTIZATION        
    ProtectedByCriticalSection cs(&bpools_sem);
    reduce_free_blocks();  // these blocks are really free, can be included into the flush
    ffa.flush();  
    deallocation_move(cdp);  // may cross the limit the number of free blocks, no problem
#else    
    ffa.flush();  
#endif
  }  
}

void t_disk_space_management::flush_and_time(cdp_t cdp)  // called inside bpools_sem
// Called after allocation from the pool, alter expanding the table, after taking the list of free records.
{
  reduce_free_blocks();    // this must be done periodically independent of settings
  if (!stop_flushing &&               // during import, ALTER TABLE
      the_sp.FlushPeriod.val())     // flushing disabled
  { last_fil_flush=clock_now();
    ffa.flush();
  }  
  deallocation_move(cdp);  // this must be done periodically independent of settings
}

void t_disk_space_management::close_block_cache(cdp_t cdp) // called inside bpools_sem
{ 
#ifndef FLUSH_OPTIZATION        
  ffa.save_working_blocks(cdp);
#else
  deallocation_move(cdp);  // moves the predeallocated blocks
 // save all blocks to the pools: 
  while (free_blocks_cnt)
    save_to_pool(POOL_FROM_DADR(*free_blocks.acc0(0)), 0);  
  ffa.flush();  
#endif
}

bool t_disk_space_management::add_new_pool(void)  // called inside bpools_sem        
{ ProtectedByCriticalSection cs(&ffa.remap_sem);  // protects the access to the [header]
  full_bpool(corepool);
  if (write_bpool(corepool, header.bpool_count)) return false;
  next_pool=header.bpool_count++;
  ffa.write_header();
  return true;
}  

bool t_disk_space_management::extend_database_if_outside(cdp_t cdp, tblocknum max_phys_block)
// Called inside bpools_sem!
{ if (ffa.backup_support)  // if the file is frozen for backup
  { if (ffa.backup_support->new_db_file_size <= max_phys_block)
      ffa.backup_support->new_db_file_size = max_phys_block+1;
  }
  else
  { if (max_phys_block >= ffa.db_file_size) 
    { unsigned step = max_phys_block + 1 - ffa.db_file_size;
      unsigned minstep = ffa.db_file_size < 25600 ? 256 : 2560;
      if (step<minstep) step=minstep;
      if (ffa.set_fil_size(cdp, ffa.db_file_size+step))
        if (ffa.set_fil_size(cdp, max_phys_block+1)) 
          return false;
    }
  }
  return true;
}

bool t_disk_space_management::lock_and_alloc(cdp_t cdp, unsigned count)
// Makes sure that the list of free blocks contains at least count blocks.
// Defines [max_phys_block] and [taken_from_pool].
{ 
#ifdef FLUSH_OPTIZATION        
  EnterCriticalSection(&bpools_sem);
  if (free_blocks_cnt<count)
  { // must not call deallocation_move(cdp) here, this can be done only with the immediate flush
    taken_from_pool=true;
    max_phys_block = 0;
    if (count<BLOCK_FILL_LIMIT) count=BLOCK_FILL_LIMIT;
    unsigned int cyc=0;  int i, j;  bool change;
   // cycle on corepools, unless deallocation_move() provided enough blocks: 
    while (free_blocks_cnt<count)
    { if (read_bpool(next_pool, corepool)) // unreadable pool: must be after the end of the database file, bad compacting
        full_bpool(corepool);
     // search the corepool for the free blocks:   
      i=0; j=0; change=false;
      do
      { if (!corepool[i]) j=7;  /* byte empty, do not check its bits */
        else
          if (corepool[i] & (1<<j))  /* block not allocated yet */
          { corepool[i] ^= (1<<j);
            tblocknum dadr = ((tblocknum)next_pool * BLOCKSIZE + i) * 8 + j;
            *free_blocks.acc(free_blocks_cnt++) = dadr;
            change=true;
            tblocknum phys_block=ffa.translate_blocknum(header.bpool_first+dadr);
            if (phys_block>max_phys_block) max_phys_block=phys_block;
            if (free_blocks_cnt>=count) break;
          }
       // go to the next candidate block:
        if (++j==8)
        { j=0;
          if (++i==BLOCKSIZE) break;
        }
      } while (true);
     // save the pool if anything changed in it: 
      if (change)
        if (write_bpool(corepool, next_pool)) goto error;
      if (free_blocks_cnt>=count) break;  // stay in the current pool if allocated enough
     // not allocated enough from the pool, checking the next one:
      if (++next_pool==header.bpool_count) next_pool=0;  /* when pool empty only! */
      if (++cyc>header.bpool_count) // all pools checked
        if (add_new_pool()) next_pool=header.bpool_count-1;
        else goto error;
    } 
  } // cache empty
  else
    taken_from_pool=false; 
  locked_by = cdp->applnum_perm;  
  flush_requested=false;
  return true;  

 error: // error processing must be outside the CS:
  LeaveCriticalSection(&bpools_sem);
  request_error(cdp, OUT_OF_BLOCKS);  
  return false;
#else
  return true;
#endif
}

tblocknum t_disk_space_management::take_block_from_cache(void)
// Called inside the CS.
{// take an existing block from the cache:
  if (free_blocks_cnt)
		return *free_blocks.acc0(--free_blocks_cnt);
  return 0;  // not allocated
}

void t_disk_space_management::unlock_allocation(cdp_t cdp)
{
#ifdef FLUSH_OPTIZATION        
  if (taken_from_pool || flush_requested)
    flush_and_time(cdp);
  locked_by = (t_client_number)-1;  
 // extending the database file(s), if any added block is outside:
  if (taken_from_pool)
    extend_database_if_outside(cdp, max_phys_block);
  LeaveCriticalSection(&bpools_sem);
#endif
}
   
tblocknum t_disk_space_management::get_block(cdp_t cdp)  /* returns a new block, 0 if cannot */  // LCK2
/* When new blocks are added into the free block cache, FIL is extended to contain it.
   Otherwise if there if no space on the disk, the error would be found
     during commit and an inconsistence in the db would be created.
   W98:
   If FIL is not extended and system crashes, file retains its original size 
     and lost clusters are reported.
   If FIL is extended by SetEndOfFile only and system crashes, difference in file size is reported
     on next startup and it may be corrected.
   Only closing and re-openning the file guarantees the presistent increase in its size.
*/
{ 
#ifndef FLUSH_OPTIZATION        
  return ffa.get_block(cdp);
#else  
  tblocknum theblock;
  if (locked_by==cdp->applnum_perm)
    theblock = take_block_from_cache();
  else  
  { if (!lock_and_alloc(cdp, 1))
      return 0;  // not locked
    theblock = take_block_from_cache();
    unlock_allocation(cdp);
  }  
  return theblock;  /* takes the minimal block number */
#endif
}

tblocknum t_disk_space_management::get_big_block(cdp_t cdp)  /* returns a new big block, 0 if cannot */  // LCK2
/* When a new block is returned, FIL is extended to contain it.
   Reasons:
   - if there if no space on the disk, the error would be found
     during commit and an inconsistence in the db would be created;
   - reading errors can be checked correctly now. */
{ 
#ifndef FLUSH_OPTIZATION        
  return ffa.get_big_block(cdp);
#else  
  tblocknum dadr, max_phys_block = 0;
  { ProtectedByCriticalSection cs(&bpools_sem, cdp, WAIT_CS_BPOOLS);
    unsigned int cyc=0;  BOOL found = FALSE;  
    do // cycle on pools
    { if (read_bpool(next_pool, corepool))
        full_bpool(corepool);
      unsigned start = 0, size;
      while (start <= BLOCKSIZE - SMALL_BLOCKS_PER_BIG_BLOCK/8)
      {// find number of ff bytes:
        size=0;
        while (size < SMALL_BLOCKS_PER_BIG_BLOCK/8 && corepool[start+size]==0xff) size++;
        if (size == SMALL_BLOCKS_PER_BIG_BLOCK/8) { found=TRUE;  dadr = ((tblocknum)next_pool * BLOCKSIZE + start) * 8;  break; }
        start+=size+1;
      }
      if (found)
      { memset(corepool+start, 0, size);
        if (write_bpool(corepool, next_pool)) goto error;
        break;
      }
     // search in the next pool:
      if (++next_pool==header.bpool_count) next_pool=0;  /* when pool empty only! */
      if (++cyc>header.bpool_count) // all pools checked
        if (add_new_pool()) next_pool=header.bpool_count-1;
        else goto error;
    } while (true); /* core buffer empty, allocate from disc pool */
    flush_and_time(cdp);
   // expand FIL, if necessary:
    if (!extend_database_if_outside(cdp, header.bpool_first+dadr+SMALL_BLOCKS_PER_BIG_BLOCK-1))
      goto error;
  } // CS
  return dadr;

 error: // error processing must be outside the CS:
  request_error(cdp, OUT_OF_BLOCKS);  
  return 0;
#endif
}


tblocknum t_disk_space_management::free_blocks_count(cdp_t cdp)  
// Returns the number of free bloks in the FIL (bpools & memory buffer)
{ 
#ifndef FLUSH_OPTIZATION        
  return ffa.free_blocks(cdp);
#else  
  tblocknum count;  tblocknum dadr, dadrlimit;
  if (!FILE_HANDLE_VALID(ffa.handle[0])) return (tblocknum)-1;  // when called from server window, files may be closed
  dadrlimit = ffa.db_file_size - header.bpool_first;
  ProtectedByCriticalSection cs(&bpools_sem, cdp, WAIT_CS_BPOOLS);
  count = free_blocks_cnt+predeallocated_blocks_cnt;  
  dadr=0;
  for (int poolnum=0;  poolnum<header.bpool_count;  poolnum++)
    if (!read_bpool(poolnum, corepool))
    { for (unsigned cnt=0;  cnt<BLOCKSIZE;  cnt++)
        if (!corepool[cnt]) dadr+=8;
        else
          for (unsigned i=0;  i<8;  i++, dadr++)
            if (corepool[cnt] & (1<<i))
              if (dadr >= dadrlimit) goto limit_reached;
              else count++;
    }
 limit_reached:
  return count;
#endif
}

