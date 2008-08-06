/****************************************************************************/
/* common.cpp - object descriptor caches and column information functions   */
/****************************************************************************/
#ifdef WINS
#include <windows.h>
#else
#include "winrepl.h"
#endif
#include "defs.h"
#include "comp.h"
#include "cint.h"
#pragma hdrstop
#if WBVERS<=810
#include "messages.h"
#ifdef CLIENT_GUI
#include "wprez.h"
#else
#include "wbprezen.h"
#include "wbprezex.h"
#endif
#ifdef CLIENT_GUI
#include "wbviewed.h"
#endif
#endif
#ifndef CLIENT_GUI
#include "wbprezex.h"
#endif

#ifdef UNIX
#if WBVERS<1000  // moved to cint.cpp since 10.0
#include <stdlib.h>
#include <locale.h>
#include <malloc.h>
#include "wbmail.cpp"
#endif
#endif
#include "random.cpp"

/////////////////////////////////// Form descriptor support ////////////////////////////
#ifdef CLIENT_GUI

static const char * this_form_source = NULL;  // source of the thisform==1

CFNC DllKernel const char * WINAPI set_this_form_source(cdp_t cdp, const char * source)
{ const char * prev = this_form_source;
  this_form_source=source;
  inval_table_d(cdp, 1, CATEG_VIEW);
  return prev;
}

static t_control_info * get_view_info(cdp_t cdp, tobjnum objnum)
{ BOOL dealloc=TRUE;
  BOOL dflt_view;  tobjnum basenum;  tcateg cat;  tptr basename;
  kont_descr compil_kont;  tptr def;
  def=lnk_get_edited_view_source(objnum); // returns source iff the view "objnum" is bein edited, NULL otherwise
  if (def==NULL) 
    if (objnum==1) { def=(tptr)this_form_source;  dealloc=FALSE; }
    else def=cd_Load_objdef(cdp, objnum, CATEG_VIEW);
  if (def==NULL) return NULL;
  compil_info xCI(cdp, def, MEMORY_OUT, view_info_comp);
  if (analyze_base(cdp, def, &basename, &basenum, &cat, &dflt_view))
  { corefree(basename);
    compil_kont.kontcateg=cat;
    compil_kont.kontobj=basenum;
    compil_kont.next_kont=NULL;
    if (cat==CATEG_CURSOR || cat==CATEG_TABLE || cat==CATEG_DIRCUR)
      xCI.kntx=&compil_kont;
  }
  int res=compile(&xCI);
  if (dealloc) corefree(def);
  if (res) return NULL;
  return (t_control_info*)xCI.univ_code;
}

static wbbool loading_view_info = wbfalse;  // semaphore preventing view descritpor recursion

#else // !CLIENT_GUI -> no form decsriptors

CFNC DllKernel const char * WINAPI set_this_form_source(cdp_t cdp, const char * source)
{ return NULL; }  // placeholder function

#endif
    
#include "baselib.cpp"
#ifndef WINS
#include <iconv.h>
#endif

static void bool2str(wbbool b, char * txt, int prez)
{ if (b==NONEBOOLEAN) *txt=0;
  else
#ifndef ENGLISH
    if (prez!=SQL_LITERAL_PREZ)  // not a SQL_LITERAL_PREZ
      strcpy(txt, b ? "Ano" : "Ne");
    else
#endif
      strcpy(txt, b ? "True" : "False");
}

static BOOL str2bool(const char * txt, wbbool  * b)
{ while (*txt==' ') txt++;
  if (!*txt) { *b=0x80; return TRUE; }
  if      (!stricmp(txt,"TRUE"))  *b=wbtrue;
  else if (!stricmp(txt,"FALSE")) *b=wbfalse;
  else if (!stricmp(txt,"T"))     *b=wbtrue;
  else if (!stricmp(txt,"F"))     *b=wbfalse;
#ifndef ENGLISH
  else if (!stricmp(txt,"ANO"))   *b=wbtrue;
  else if (!stricmp(txt,"NE"))    *b=wbfalse;
  else if (!stricmp(txt,"A"))     *b=wbtrue;
  else if (!stricmp(txt,"N"))     *b=wbfalse;
#endif
  else return FALSE;
  return TRUE;
}

CFNC DllKernel char * WINAPI convert_w2s(const wuchar * wstr, t_specif specif)
// Converts null-terminated wide-character string to char string according to [specif].
// Allocates the output buffer.
{ int len=(int)wuclen(wstr);
  char * buf2 = (char*)corealloc(len+1, 77);
  if (!buf2) return NULL;
  if (conv2to1(wstr, len, buf2, wbcharset_t(specif.charset), 0))
    *buf2=0;
  return buf2;
}

CFNC DllKernel wuchar * WINAPI convert_s2w(const char * str, t_specif specif)
// Converts null-terminated character string to wide-char string according to [specif].
// Allocates the output buffer.
{ int len=(int)strlen(str);
  wuchar * buf2 = (wuchar*)corealloc(sizeof(wuchar)*(len+1), 77);
  if (!buf2) return NULL;
  if (conv1to2(str, len, buf2, wbcharset_t(specif.charset), 0))
    *buf2=0;
  return buf2;
}

#pragma optimize ("w", off)  // binary conversion
CFNC DllKernel void WINAPI convert2string(const void * val, int tp, char * txt, int prez, t_specif specif)
{ switch (tp)
  { case ATT_BOOLEAN:  bool2str(*(wbbool*)val, txt, prez);  break;
    case ATT_CHAR:     *txt=*(char*)val; txt[1]=0;  break;
    case ATT_INT8:     if (*(sig8*)val==NONETINY) *txt=0; 
                       else intspec2str(*(sig8*)val, txt, specif);  break;
    case ATT_INDNUM:
    case ATT_INT16:    if (*(sig16*)val==NONESHORT) *txt=0; 
                       else intspec2str(*(sig16*)val, txt, specif);  break;
    case ATT_FLOAT:    real2str(*(double*)val, txt, prez);     break;
    case ATT_MONEY:    money2str((monstr*)val, txt, prez); break;
    case ATT_DATE:     date2str(*(uns32*)val, txt, prez);    break;
    case ATT_TIME:     if (specif.with_time_zone) timeUTC2str(*(uns32*)val, client_tzd, txt, prez);
                       else time2str(*(uns32*)val, txt, prez);    break;
    case ATT_TIMESTAMP:
    case ATT_DATIM:    if (specif.with_time_zone) timestampUTC2str(*(uns32*)val, client_tzd, txt, prez);
                       else timestamp2str(*(uns32*)val, txt, prez);  break;
    case ATT_PTR:      case ATT_BIPTR:
    case ATT_VARLEN:
    case ATT_INT32:    intspec2str(*(sig32*)val, txt, specif);    break;
    case ATT_INT64:    int64spec2str(*(sig64*)val, txt, specif);  break;
    case ATT_STRING:   
    case ATT_TEXT:    /* used when displaying CLOB in a simple view */
      if (specif.wide_char)
      { char * buf = convert_w2s((const wuchar *)val, specif);
        if (buf) { strcpy(txt, buf);  corefree(buf); }
        else *txt=0;
        break;
      }
      // cont.
    case ATT_ODBC_DECIMAL:  case ATT_ODBC_NUMERIC:
    case ATT_AUTOR:   strcpy(txt, (char*)val);  break;
    case 0:           *txt=0;  break;  // used by the NULL type
    case ATT_BINARY: // converts full size from [specif], must support in-place operation
    { int len = specif.length;  uns8 * src = (uns8*)val;
      txt[2*len]=0;  BOOL nonnull=FALSE;
      while (len-- > 0)
      { int x = src[len];
        if (x) nonnull=TRUE;
        int bt = x >> 4;
        txt[2*len]   = bt >= 10 ? 'A'-10+bt : '0'+bt;
        bt = x & 0xf;
        txt[2*len+1] = bt >= 10 ? 'A'-10+bt : '0'+bt;
      }
      if (!nonnull) *txt=0;
      break;
    }
    case ATT_ODBC_DATE:
      if (((DATE_STRUCT*)val)->day > 31) *txt=0; else
      date2str(Make_date(((DATE_STRUCT*)val)->day,
                         ((DATE_STRUCT*)val)->month,
                         ((DATE_STRUCT*)val)->year), txt, prez);  break;
    case ATT_ODBC_TIME:
      if (((TIME_STRUCT*)val)->hour > 24) *txt=0; else
      time2str(Make_time(((TIME_STRUCT*)val)->hour,
                         ((TIME_STRUCT*)val)->minute,
                         ((TIME_STRUCT*)val)->second, 0), txt, prez);  break;
    case ATT_ODBC_TIMESTAMP:
      odbc_timestamp2str((TIMESTAMP_STRUCT*)val, txt, prez);  break;
    default: *txt=0; break;
  }
}
#pragma optimize ("", on)

CFNC DllKernel void WINAPI conv2str(basicval0 * bas, int tp, tptr txt, int prez, t_specif specif)
// Converts a value from the stack of the interpreter to a string.
{ bool indirect = tp==ATT_STRING || tp==ATT_TEXT || tp==ATT_AUTOR || tp==ATT_BINARY || 
                  tp==ATT_ODBC_DECIMAL || tp==ATT_ODBC_NUMERIC ||
                  tp==ATT_ODBC_DATE || tp==ATT_ODBC_TIME || tp==ATT_ODBC_TIMESTAMP;
  convert2string(indirect ? bas->ptr : (char*)&bas->int8, tp, txt, prez, specif);
}

CFNC DllKernel BOOL WINAPI convert_from_string(int tp, const char * txt, void * dest, t_specif specif)
// dest may be same as txt!
{ uns32 aux32;
  switch (tp)  /* conversion according to the result type */
  { case ATT_BOOLEAN:
    { wbbool b;
      if (!str2bool(txt, &b)) return FALSE;  *(sig16*)dest = (sig16)b;  return TRUE;
    }
    case ATT_CHAR:    *(char*)dest=*txt;  return TRUE;
    case ATT_INT8:
    { sig32 aux32;
      while (*txt==' ') txt++;
      if (!*txt) { *(sig32*)dest=0x80;  return TRUE; }
      if (!str2intspec(txt, &aux32, specif) || aux32>127 || aux32<-127) return FALSE;
      *(sig32*)dest=aux32;  return TRUE;
    }
    case ATT_INDNUM:
    case ATT_INT16:
    { sig32 aux32;
      while (*txt==' ') txt++;
      if (!*txt) { *(sig32*)dest=0x8000;  return TRUE; }
      if (!str2intspec(txt, &aux32, specif) || aux32>32767 || aux32<-32767) return FALSE;
      *(sig32*)dest=aux32;  return TRUE;
    }
    case ATT_PTR:       case ATT_BIPTR:    case ATT_VARLEN:
    case ATT_INT32:   return str2intspec  (txt, (sig32 *)dest, specif);
    case ATT_INT64:   return str2int64spec(txt, (sig64 *)dest, specif);
    case ATT_FLOAT:   return str2real     (txt, (double*)dest);
    case ATT_MONEY:   return str2money    (txt, (monstr*)dest);
    case ATT_DATE:    return str2date     (txt, (uns32 *)dest);
    case ATT_TIME:    if (specif.with_time_zone) return str2timeUTC(txt, (uns32 *)dest, true, client_tzd);
                      return str2time     (txt, (uns32 *)dest);
    case ATT_TIMESTAMP:
    case ATT_DATIM: /* used by untyped assignment to ODBC */
                      if (specif.with_time_zone) return str2timestampUTC(txt, (uns32 *)dest, true, client_tzd);
                      return str2timestamp(txt, (uns32 *)dest);
    case ATT_TEXT:
    case ATT_STRING:  
    { //int num = strlen(txt);
      //while (num && (txt[num-1]==' ')) num--;
      //txt[num]=0;
      if (specif.wide_char)
      { wuchar * buf = convert_s2w(txt, specif);
        if (!buf) return FALSE;
        wuccpy((wuchar*)dest, buf);  
        corefree(buf);
      }
      else strcpy((char*)dest, txt);
      return TRUE;
    }
    case ATT_ODBC_DECIMAL:  case ATT_ODBC_NUMERIC:
    { //int num = strlen(txt);
      //while (num && (txt[num-1]==' ')) num--;
      //txt[num]=0;
      strcpy((char*)dest, txt);
			return TRUE;
    }
    case ATT_BINARY:
    { int src = 0;
      while (txt[src]==' ') src++;
      uns8 * btdest = (uns8*)dest;
      int dst = 0;
      while (txt[src])
      { char c=txt[src++];
        if (c>='0' && c<='9') c-='0';
        else if (c>='A' && c<='F') c-='A'-10;
        else if (c>='a' && c<='f') c-='a'-10;
        else return FALSE;
        btdest[dst]=c<<4;
        c=txt[src++];
        if (!c) { dst++;  break; }
        if (c>='0' && c<='9') c-='0';
        else if (c>='A' && c<='F') c-='A'-10;
        else if (c>='a' && c<='f') c-='a'-10;
        else return FALSE;
        btdest[dst]+=c;
        dst++;
      }
      if (dst<specif.length) memset(btdest+dst, 0, specif.length-dst);
			return TRUE;
    }
    default:   // used by the NULL type
    case ATT_AUTOR:
    case ATT_HIST:    return FALSE;  /* stopped as soon as possible */
    case ATT_ODBC_DATE:
      if (!str2date(txt, &aux32)) return FALSE;
      if (aux32==NONEDATE) ((DATE_STRUCT*)dest)->day=32;
      else
      { ((DATE_STRUCT*)dest)->day=Day(aux32);  ((DATE_STRUCT*)dest)->month=Month(aux32);  ((DATE_STRUCT*)dest)->year=Year(aux32); }
      return TRUE;
    case ATT_ODBC_TIME:
      if (!str2time(txt, &aux32)) return FALSE;
      if (aux32==NONETIME) ((TIME_STRUCT*)dest)->hour=25;
      else
      { ((TIME_STRUCT*)dest)->hour=Hours(aux32);  ((TIME_STRUCT*)dest)->minute=Minutes(aux32);  ((TIME_STRUCT*)dest)->second=Seconds(aux32); }
      return TRUE;
    case ATT_ODBC_TIMESTAMP:
      return str2odbc_timestamp(txt, ((TIMESTAMP_STRUCT*)dest));
  }
}

////////////////////// Loading object definitions and its prefixes ////////////////////////////////////
CFNC DllKernel tptr WINAPI load_blob(cdp_t cdp, tcursnum cursnum, trecnum rec, tattrib attr, t_mult_size index, uns32 * length)
// Loads object definition and returns it or returns NULL and sets the error.
{ tptr def; 
  if (cd_Read_len(cdp, cursnum, rec, attr, index, length)) return NULL;
	def=(tptr)corealloc(*length+1, 84);
	if (!def)
    { client_error(cdp, OUT_OF_MEMORY);  return NULL; }
	if (cd_Read_var(cdp, cursnum, rec, attr, index, 0, *length, def, length))
		{ corefree(def);  return NULL; }
  def[*length]=0;  // terminator fo CLOBs
  return def;
}

CFNC DllKernel tptr WINAPI cd_Load_objdef(cdp_t cdp, tobjnum objnum, tcateg category, uns16 *pFlags)
// Loads object definition and returns it or returns NULL and sets the error.
{ tptr def;  uns32 size;
  ttablenum tb=(ttablenum)((category==CATEG_TABLE) ? TAB_TABLENUM : OBJ_TABLENUM);
  def=load_blob(cdp, tb, objnum, OBJ_DEF_ATR, NOINDEX, &size);
  if (!def) return NULL;
  if (ENCRYPTED_CATEG(category) || pFlags)
  { uns16 flags;
    cd_Read(cdp, tb, objnum, OBJ_FLAGS_ATR, NULL, &flags);
    if (ENCRYPTED_CATEG(category) && (flags & CO_FLAG_ENCRYPTED))
    { enc_buffer_total(cdp, def, size, objnum);
      flags = 0;
    }
    if (pFlags)
      *pFlags = flags;
  }
  return def;
}

CFNC DllKernel BOOL WINAPI cd_Encrypt_object(cdp_t cdp, tobjnum objnum, tcateg category)
{ if (!ENCRYPTED_CATEG(category))
    { client_error(cdp, ERROR_IN_FUNCTION_CALL);  return TRUE; }
  ttablenum tb=(ttablenum)((category==CATEG_TABLE) ? TAB_TABLENUM : OBJ_TABLENUM);
  uns16 flags;
  if (cd_Read(cdp, tb, objnum, OBJ_FLAGS_ATR, NULL, &flags)) return TRUE;
  if (flags & CO_FLAG_ENCRYPTED) return FALSE;  // already encrypted
  tptr def = cd_Load_objdef(cdp, objnum, category, NULL);
  if (!def) return TRUE;
  unsigned defsize = (int)strlen(def);
  enc_buffer_total(cdp, def, defsize, objnum);
	if (cd_Write_var(cdp, tb, objnum, OBJ_DEF_ATR, NOINDEX, 0, defsize, def))
		{ corefree(def);  return TRUE; }
  corefree(def);
  flags |= CO_FLAG_ENCRYPTED | CO_FLAG_PROTECT;
  return cd_Write(cdp, tb, objnum, OBJ_FLAGS_ATR, NULL, &flags, sizeof(flags));
}

CFNC DllKernel BOOL WINAPI Encrypt_object(tobjnum objnum, tcateg category)
{ return cd_Encrypt_object(GetCurrTaskPtr(), objnum, category); }

/************************ object descriptor cache **********************/
/* The logic:
   Descriptors stored in the cache are identified by (tabnum, category, applnum).
   The cache never contains multiple descriptors with the same identification.
   [locked] is the number of external pointer to the descriptor. 
     When taking a descriptor from the cache, [locked] is increased by 1.
     When returning a descriptor, if it is found in the cache, [locked] is decreased by 1.
     Descriptor may be removed from the cache (and corefree-d) when [locked] is <= 1. Otherwise it is shared by multiple users.
   Any descriptor requested by cd_get_table_d is stored in the cache, unless the cache is full.
   Any descriptor returned by release_table_d is stored in the cache, unless the cache is full or it already contains it.
   Not all descriptors are guaranteed to be stored in the cache, when the cache is full.
*/
struct d_table_cch_item   // descriptor cache cell
{ tobjnum  tabnum;      // cached object number
  tcateg   category;    // cached object category
  uns8     applnum;     // identifies the server
  bool     invalidated; // descriptor is invalid, may be set only for locked objects
  int      locked;      // lock counter
  d_table  * tabdes;    // the descriptor, cache cell is free iff this is NULL
};

enum { DTB_CACHE_SIZE=16 };

class d_table_cache
{ d_table_cch_item dtb_cache[DTB_CACHE_SIZE];  // the descriptor cache, NULL initialized! 
  int next_cell;
  CRITICAL_SECTION cs_dtb;
  friend const d_table * WINAPI cd_get_table_d(cdp_t cdp, tobjnum tb, tcateg cat);
  friend void WINAPI inval_table_d(cdp_t cdp, tobjnum tb, tcateg cat);
  friend void WINAPI destroy_cursor_table_d(cdp_t cdp);
  friend void WINAPI release_table_d(const d_table * td);
 public:
  d_table_cache(void);
  ~d_table_cache(void);
};

d_table_cache::d_table_cache(void)
// Must be called before using the cache. Must not be called twice.
{ int i;  d_table_cch_item * pdtb;
  for (i=0, pdtb=dtb_cache;  i<DTB_CACHE_SIZE;  i++, pdtb++)
    pdtb->tabdes=NULL;
  next_cell=0;
  InitializeCriticalSection(&cs_dtb);
}

d_table_cache::~d_table_cache(void)
{ int i;  d_table_cch_item * pdtb;
  for (i=0, pdtb=dtb_cache;  i<DTB_CACHE_SIZE;  i++, pdtb++)
    if (pdtb->tabdes) corefree(pdtb->tabdes);
  DeleteCriticalSection(&cs_dtb);
}

static d_table_cache dtc;

CFNC DllKernel const d_table * WINAPI cd_get_table_d(cdp_t cdp, tobjnum tb, tcateg cat)
// Returns tb/cat descriptor and increases its lock-count, if necessary,
//   creates this descriptor and loads it into the cache. Returns NULL on error. 
// When cat==CATEG_TABLE then tb can be table number, ODBC table number or cursor number.
{ int i, ind1, ind2, ind3;  d_table_cch_item * pdtb;  d_table * desc_new;
  if (IS_CURSOR_NUM(tb) || IS_ODBC_TABLEC(cat, tb)) cat=CATEG_DIRCUR;
  EnterCriticalSection(&dtc.cs_dtb);
 // look for the descriptor in the cache:
  for (i=0, pdtb=dtc.dtb_cache;  i<DTB_CACHE_SIZE;  i++, pdtb++)
    if (pdtb->tabnum==tb && pdtb->category==cat && pdtb->tabdes) // descriptor found
    if (pdtb->applnum==cdp->applnum)  // important when user connected to multiple servers
    { //if (pdtb->invalidated) -- must return even the invalidate objects because must not delete locked object and must not load the same object again
      //  { safefree((void**)&pdtb->tabdes);  break; }
      pdtb->locked++; // increase the lock counter
      LeaveCriticalSection(&dtc.cs_dtb);
      return pdtb->tabdes;
    }
  LeaveCriticalSection(&dtc.cs_dtb);
 // descriptor not found in the cache, create it:
  if (cat==CATEG_VIEW)
  { 
#ifdef CLIENT_GUI
    if (loading_view_info) return NULL;  // prevents recursion 
    loading_view_info=wbtrue;
    desc_new = (d_table*)get_view_info(cdp, tb);
    loading_view_info=wbfalse;
    if (desc_new==NULL) 
#endif
      return NULL;
  }
  else if (cd_Get_descriptor(cdp, tb, cat, &desc_new)) return NULL;
 // Find a cell in the cache for the descriptor
 // The same descriptor may have been added by sone other client: MUST CHECK FOR IT
  ind1=ind2=ind3=-1;
  EnterCriticalSection(&dtc.cs_dtb);
  for (i=0, pdtb=dtc.dtb_cache;  i<DTB_CACHE_SIZE;  i++, pdtb++)
    if (!pdtb->tabdes) ind1=i;   // free cell found
    else if (pdtb->tabnum==tb && pdtb->category==cat && pdtb->applnum==cdp->applnum)  // important when user connected to multiple servers
    { corefree(desc_new);  // drop the created descriptor
      pdtb->locked++;
      LeaveCriticalSection(&dtc.cs_dtb);  
      return pdtb->tabdes;
    } 
    else if (!pdtb->locked)                 // a not locked cell found
      if (i==dtc.next_cell) ind2=i; else ind3=i;
 // Select the cell for storing the descriptor:     
 // Strategy: 1. free cell, 2. next_cell if not locked, 3. any not locked
  if (ind1==-1)   // free cell not found 
  { if (++dtc.next_cell == DTB_CACHE_SIZE) dtc.next_cell=0;
    if (ind2!=-1) ind1=ind2;
    else
      if (ind3!=-1) ind1=ind3;
      // else: all cells locked, the descriptor will not be stored in the cache
  }
 // insert the descriptor into the cache:
  if (ind1!=-1)
  { pdtb=dtc.dtb_cache+ind1;
    if (pdtb->tabdes) corefree(pdtb->tabdes);
    pdtb->tabnum=tb;  pdtb->category=cat;  pdtb->applnum=cdp->applnum;  pdtb->tabdes=desc_new;
    pdtb->locked=1;   pdtb->invalidated=false;
  }
  LeaveCriticalSection(&dtc.cs_dtb);
  return desc_new;
}

CFNC DllKernel void WINAPI release_table_d(const d_table * td)   // releases the lock on tabdes (decreases the reference count)
// Decreases the lock-count for the descriptor stored in the cache. Destroys the descriptor otherwise.
{ int i;  d_table_cch_item * pdtb;  
  EnterCriticalSection(&dtc.cs_dtb);
 // find the descriptor in the cache, decrease [lockled] if found:
  bool found=false;
  for (i=0, pdtb=dtc.dtb_cache;  i<DTB_CACHE_SIZE;  i++, pdtb++)
    if (pdtb->tabdes==td) 
    { if (pdtb->locked) pdtb->locked--;
      if (!pdtb->locked && pdtb->invalidated) safefree((void**)&pdtb->tabdes);
      found=true;  
      break;
    }
  LeaveCriticalSection(&dtc.cs_dtb);       
 // destroy the descriptor, if it is not contained in the cache, cannot insert it because (tabnum, category) is unknown:   
  if (!found)
    corefree(td);
}

CFNC DllKernel void WINAPI inval_table_d(cdp_t cdp, tobjnum tb, tcateg cat)
// Removes the descriptor from the cache or invalidates it (when locked)
// When cat==CATEG_TABLE then tb can be table number, ODBC table number or cursor number.
{ int i;  d_table_cch_item * pdtb;
  if (IS_CURSOR_NUM(tb) || IS_ODBC_TABLEC(cat, tb)) cat=CATEG_DIRCUR;
  EnterCriticalSection(&dtc.cs_dtb);
  for (i=0, pdtb=dtc.dtb_cache;  i<DTB_CACHE_SIZE;  i++, pdtb++)
    if ((pdtb->tabnum==tb || tb==NOOBJECT) && pdtb->category==cat && pdtb->tabdes)
      if (pdtb->applnum==cdp->applnum)  // important when user is connected to multiple servers
      { if (pdtb->locked) pdtb->invalidated=wbtrue;
        else safefree((void**)&pdtb->tabdes);
      }
  LeaveCriticalSection(&dtc.cs_dtb);
#if WBVERS<900
#ifdef CLIENT_GUI
  // This is obsolete and causes strange error messages when calling inval... without privileges to relations.
  if (cat==CATEG_TABLE) // must update attribute numbers and reference info in relations
    if (!cdp->object_cache_disabled)
    { cdp->odbc.apl_relations.relation_dynar::~relation_dynar();
      if (*cdp->sel_appl_name)
        scan_objects(cdp, CATEG_RELATION, TRUE);
    }
#endif
#endif
}

void WINAPI destroy_cursor_table_d(cdp_t cdp)
// Removes the descriptors of own cursors from the cache 
{ int i;  d_table_cch_item * pdtb;
  EnterCriticalSection(&dtc.cs_dtb);
  for (i=0, pdtb=dtc.dtb_cache;  i<DTB_CACHE_SIZE;  i++, pdtb++)
    if (//pdtb->category==CATEG_DIRCUR && -- should remove everything, may connect to a different server later
        pdtb->tabdes && pdtb->applnum==cdp->applnum)
      safefree((void**)&pdtb->tabdes);
  LeaveCriticalSection(&dtc.cs_dtb);
}

/* Improvement:
   Then multiple descriptors of the same object would be possible in the cache -> cache can return
   the new descriptor even if the old one is locked and invalidated. */

/////////////////////////////// Column information functions //////////////////////////////////
CFNC DllKernel BOOL WINAPI find_attr(cdp_t cdp, tobjnum tb, tcateg cat,
   const char * name1, const char * name2, const char * name3, d_attr * info)
// When cat==CATEG_TABLE then tb can be table number, ODBC table number or cursor number.
{ const d_table * td;  const char *p, *schema, *tbname, *attrname;
  int i,j;  const d_attr * pattr;
  if (IS_CURSOR_NUM(tb) || IS_ODBC_TABLEC(cat, tb)) cat=CATEG_DIRCUR;
  if (name3 && *name3)      { schema=name1;  tbname=name2;  attrname=name3; }
  else if (name2 && *name2) { schema=NULL;   tbname=name1;  attrname=name2; }
  else                      { schema=        tbname=NULL;   attrname=name1; }
  td=cd_get_table_d(cdp, tb, cat);  if (!td) return FALSE;
  if (IS_ODBC_TABLEC(cat, tb))
  { tptr src = (tptr)(td->attribs+td->attrcnt);
    for (i=1;  i < td->attrcnt;  src+=strlen(src)+1, i++)
    { p=src;  j=0;
      while (upcase_charA(*p, cdp->sys_charset)==attrname[j++])
        if (!*(p++))
        { if (tbname && strcmp(tbname, td->selfname) ||
              schema && strcmp(schema,  td->schemaname)) i=td->attrcnt;
          goto break2;
        }
    }
  }
  else   /* WB table or cursor: */
    for (i=0, pattr=FIRST_ATTR(td);  i<td->attrcnt;  i++, pattr=NEXT_ATTR(td,pattr))
    { p=pattr->name;  j=0;
      while (*p==attrname[j++]) /* upcase_charA() removed, somebody may need this */
        if (!*(p++))
          if (!td->tabcnt)   /* a table */
          { if (tbname && strcmp(tbname, td->selfname) ||
                schema && strcmp(schema,  td->schemaname)) i=td->attrcnt;
            goto break2;
          }
          else               /* a cursor */
          { const char * prefix=COLNAME_PREFIX(td, pattr->prefnum);
            if ((!tbname || !strcmp(tbname, prefix+(OBJ_NAME_LEN+1))) &&
                (!schema || !strcmp(schema,  prefix)) )
              goto break2;
          }
    }
 break2:
  if (i<td->attrcnt)
  { info->base_copy(ATTR_N(td,i), i);
#if WBVERS<=900   // probably not used any more
    info->name[1]=i - (first_user_attr(td)-1);
#endif
    release_table_d(td);  return TRUE;
  }
  else { release_table_d(td);  return FALSE; }
}

CFNC DllKernel int WINAPI first_user_attr(const d_table * td)
{ for (int i=1;  i<td->attrcnt;  i++)
    if (memcmp(ATTR_N(td,i)->name, "_W5_", 4)) return i;
  return 0;
}

CFNC DllKernel BOOL WINAPI cd_Enum_attributes(cdp_t cdp, ttablenum table, enum_attr * callback)
{ char name[ATTRNAMELEN+1];  const d_table * td;  const d_attr * pdatr;  int i;
  td=cd_get_table_d(cdp, table, CATEG_TABLE);
  if (!td) return FALSE;
  for (i=1, pdatr=ATTR_N(td,1);  i<td->attrcnt;  i++, pdatr=NEXT_ATTR(td,pdatr))
  { strcpy(name, pdatr->name);
    if (!(*callback)(name, pdatr->type, pdatr->multi, pdatr->specif.length)) break;
  }
  release_table_d(td);
  return TRUE;
}

CFNC DllKernel BOOL WINAPI cd_Enum_attributes_ex(cdp_t cdp, ttablenum table, enum_attr_ex * callback, void * param)
{ char name[ATTRNAMELEN+1];  const d_table * td;  const d_attr * pdatr;  int i;
  td=cd_get_table_d(cdp, table, CATEG_TABLE);
  if (!td) return FALSE;
  for (i=1, pdatr=ATTR_N(td,1);  i<td->attrcnt;  i++, pdatr=NEXT_ATTR(td,pdatr))
  { strcpy(name, pdatr->name);
    if (!(*callback)(name, pdatr->type, pdatr->multi, pdatr->specif, param)) break;
  }
  release_table_d(td);
  return TRUE;
}

CFNC DllKernel BOOL WINAPI cd_Attribute_info(cdp_t cdp, ttablenum table, const char * attrname,
       tattrib * attrnum, uns8 * attrtype, uns8 * attrmult, uns16 * attrspecif)
{ d_attr datr;  char name[ATTRNAMELEN+1+1];
  strmaxcpy(name, attrname, ATTRNAMELEN+1+1);
  Upcase9(cdp, name);
  if (find_attr(cdp, table, CATEG_TABLE, name, NULL, NULL, &datr))
  { if (attrnum)    *attrnum   =datr.get_num();   
    if (attrtype)   *attrtype  =datr.type;
    if (attrmult)   *attrmult  =datr.multi;     
    if (attrspecif) *attrspecif=datr.specif.length;
    return TRUE;
  }
  else return FALSE;
}

CFNC DllKernel BOOL WINAPI cd_Attribute_info_ex(cdp_t cdp, ttablenum table, const char * attrname,
       tattrib * attrnum, uns8 * attrtype, uns8 * attrmult, t_specif * attrspecif)
{ d_attr datr;  char name[ATTRNAMELEN+1+1];
  strmaxcpy(name, attrname, ATTRNAMELEN+1+1);
  Upcase9(cdp, name);
  if (find_attr(cdp, table, CATEG_TABLE, name, NULL, NULL, &datr))
  { if (attrnum)    *attrnum   =datr.get_num();   
    if (attrtype)   *attrtype  =datr.type;
    if (attrmult)   *attrmult  =datr.multi;     
    if (attrspecif) *attrspecif=datr.specif;
    return TRUE;
  }
  else return FALSE;
}

CFNC DllKernel BOOL WINAPI Enum_attributes(ttablenum table, enum_attr * callback)
{ return cd_Enum_attributes(GetCurrTaskPtr(), table, callback); }

CFNC DllKernel BOOL WINAPI Enum_attributes_ex(cdp_t cdp, ttablenum table, enum_attr_ex * callback, void * param)
{ return cd_Enum_attributes_ex(GetCurrTaskPtr(), table, callback, param); }

CFNC DllKernel BOOL WINAPI Attribute_info(ttablenum table, const char * attrname,
       tattrib * attrnum, uns8 * attrtype, uns8 * attrmult, uns16 * attrspecif)
{ return cd_Attribute_info(GetCurrTaskPtr(), table, attrname, attrnum, attrtype, attrmult, attrspecif); }

CFNC DllKernel BOOL WINAPI Attribute_info_ex(ttablenum table, const char * attrname,
       tattrib * attrnum, uns8 * attrtype, uns8 * attrmult, t_specif * attrspecif)
{ return cd_Attribute_info_ex(GetCurrTaskPtr(), table, attrname, attrnum, attrtype, attrmult, attrspecif); }

