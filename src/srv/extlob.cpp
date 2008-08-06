// extlob.cpp
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

#ifndef INVALID_SET_FILE_POINTER
#define INVALID_SET_FILE_POINTER ((DWORD)-1)
#endif

const char * extlob_directory(void)
{ const char * extlobdir = the_sp.extlob_directory.val();
  if (!*extlobdir) extlobdir = ffa.last_fil_path();
  return extlobdir;
}
  
void lob_id2pathname(uns64 lob_id, char * pathname, bool new_id)
{
  strcpy(pathname, extlob_directory());
  int pos = (int)strlen(pathname);  // pos>0
  if (pathname[pos-1]!=PATH_SEPARATOR) pathname[pos++]=PATH_SEPARATOR;
  bool any_notnull = false;
  for (int btnum = 7;  btnum>=0;  btnum--)
  { int hexval1 = (int)(lob_id >> (8*btnum+4)) & 0xf;
    int hexval2 = (int)(lob_id >> (8*btnum  )) & 0xf;
    if (!any_notnull)
      if (hexval1 || hexval2)
        any_notnull=true;
    if (any_notnull)
      if (btnum<7)
      { if (new_id)
        { pathname[pos]=0;
          CreateDirectory(pathname, NULL);
        }  
        pathname[pos++] = PATH_SEPARATOR;
      }    
    pathname[pos++] = hexval1>=10 ? 'A'+(hexval1-10) : '0'+hexval1;
    pathname[pos++] = hexval2>=10 ? 'A'+(hexval2-10) : '0'+hexval2;
  }  
  strcpy(pathname+pos, ".lob");
}  

void read_ext_lob(cdp_t cdp, uns64 lob_id, uns32 offset, uns32 size, char * buf_or_null)
// reads nothing if: lob_id is NULL, file does not exist or cannot open it, file is too short, cannot allocate the buffer, read fails
{ FHANDLE hnd = INVALID_FHANDLE_VALUE;  long length = 0;
  if (lob_id)
  { char pathname[MAX_PATH];
    lob_id2pathname(lob_id, pathname, false);
    hnd = CreateFile(pathname, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, 0); 
    if (hnd!=INVALID_FHANDLE_VALUE)
    { length = SetFilePointer(hnd, 0, NULL, FILE_END);
      if (offset>=length)
        length=0;
      else  
        if (SetFilePointer(hnd, offset, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
          length=0;
    }
  }  
 // use the [length] and return the value:
  if (offset>=length) size=0;
  else if (offset+size>length) size = length-offset;
  if (buf_or_null==NULL) // write the result to answer
  { buf_or_null=cdp->get_ans_ptr(sizeof(uns32)+size);
    if (buf_or_null==NULL) 
      size=0;
    else
    { *(uns32*)buf_or_null=(uns32)size;
      buf_or_null+=sizeof(uns32);
    }
  }
  else // buf_or_null provided externally
    cdp->tb_read_var_result=size;
  DWORD rd;  
  ReadFile(hnd, buf_or_null, size, &rd, NULL);
 // close the file, if it is open:
  if (hnd!=INVALID_FHANDLE_VALUE)
    CloseFile(hnd);
}

bool delete_extlob_file(uns64 lob_id)
// Returns true on error
{ char pathname[MAX_PATH];
  lob_id2pathname(lob_id, pathname, false);
  return !DeleteFile(pathname);
}

bool ext_lob_free(cdp_t cdp, uns64 lob_id)
// Returns true on error
{
  if (lob_id)
    return delete_extlob_file(lob_id);
  return false;  
}

bool ext_lob_set_length(cdp_t cdp, uns64 lob_id, uns32 len)
{ char pathname[MAX_PATH];  bool res = true;
  lob_id2pathname(lob_id, pathname, false);
  FHANDLE hnd = CreateFile(pathname, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, 0, 0); 
  if (hnd!=INVALID_FHANDLE_VALUE)
  { 
#ifdef WINS
    if (SetFilePointer(hnd, len, NULL, FILE_BEGIN) != INVALID_SET_FILE_POINTER)
      if (SetEndOfFile(hnd)) res=false;
#else
    if (!ftruncate(hnd, len))
      res=false;
#endif
    CloseFile(hnd);
  }  
  return res;  
}

uns64 get_new_lob_id(void)
{ ProtectedByCriticalSection cs(&ffa.remap_sem);  // protects the access to the [header]
  header.lob_id_gen++;
  ffa.write_header();
  return header.lob_id_gen;
}

bool write_ext_lob(cdp_t cdp, uns64 * lob_id, uns32 offset, uns32 size, const char * data, bool setsize, bool * dbdata_changed)
// If [dbdata_changed!=NULL] writes *dbdata_changed=true iff created a new lob_id.
// Returns true on error.
{ bool res=true, new_id_created=false;  char pathname[MAX_PATH];
 // delete the extlob, if setting size 0:
  if (!size && !offset && setsize)
  { if (*lob_id)
    { delete_extlob_file(*lob_id);
      *lob_id=0;
      if (dbdata_changed) *dbdata_changed=true;
    }
    return false;  
  }    
 // create the extlob id, if it does not exist:
  if (!*lob_id)
  { *lob_id=get_new_lob_id();
    if (dbdata_changed) *dbdata_changed=true;
    lob_id2pathname(*lob_id, pathname, true);
    new_id_created=true;
  }  
 // write to the extlob file:
  else 
    lob_id2pathname(*lob_id, pathname, false);
 // write to the extlob file:
  FHANDLE hnd = CreateFile(pathname, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, 0, 0); 
  if (hnd!=INVALID_FHANDLE_VALUE)
  { if (SetFilePointer(hnd, offset, NULL, FILE_BEGIN)!=INVALID_SET_FILE_POINTER)
    { DWORD wr;  
      if (WriteFile(hnd, data, size, &wr, NULL))
        res=false; // all OK
    }  
    if (new_id_created || setsize)  // the file may already exist (when the database has been regenerated), must truncate it
#ifdef WINS
      if (SetFilePointer(hnd, offset+size, NULL, FILE_BEGIN) != INVALID_SET_FILE_POINTER)
        SetEndOfFile(hnd);
#else
      ftruncate(hnd, offset+size);
#endif
    CloseFile(hnd);
  }
  if (res)
    request_error(cdp, OS_FILE_ERROR);
  return res;
}

//////////////
FHANDLE open_extlob(uns64 lob_id)
{ char pathname[MAX_PATH];  
  if (!lob_id) return INVALID_FHANDLE_VALUE;
  lob_id2pathname(lob_id, pathname, false);
  return CreateFile(pathname, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, 0); 
}

void close_extlob(FHANDLE hnd)
{ CloseFile(hnd); }

uns32 read_extlob_part(FHANDLE hnd, uns32 start, uns32 length, void * buf)
{
  if (SetFilePointer(hnd, start, NULL, FILE_BEGIN) != start) return 0;
  DWORD rd;  
  if (!ReadFile(hnd, buf, length, &rd, NULL)) return 0;
  return rd;
}

uns32 read_extlob_length(FHANDLE hnd)
{ 
  return SetFilePointer(hnd, 0, NULL, FILE_END);
}

uns32 read_ext_lob_length(cdp_t cdp, uns64 lob_id)
{ 
  FHANDLE hnd = open_extlob(lob_id);
  if (hnd==INVALID_FHANDLE_VALUE) return 0;
  uns32 length = read_extlob_length(hnd);
  close_extlob(hnd);
  return length;
}  
  
