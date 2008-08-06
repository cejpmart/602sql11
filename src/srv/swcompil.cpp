/****************************************************************************/
/* The S-Pascal compiler                                                    */
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
#ifndef WINS
#ifndef UNIX
#include <process.h>
#endif
#include <stdlib.h>
#include <limits.h>
#endif
#include "odbc.h"
#ifdef LINUX
#include <prefix.h>
#include <dlfcn.h>
#endif  
/* Teorie: Standarni procedury a funkce (PF) musi mit parametry o sude velikosti,
zatimco uzivatelem definovane PF nikoli. To se zajistuje pouze pro velikost 1,
ktera se pro std. konvertuje na 2. Zadna standardni PF nema jako parametr
string predavany hodnotou nebo strukturu. Neni podporeno predani databazoveho
objektu do parametru strukturovaneho typu.

*/
/*************************** standard objects *******************************/
#include "swstds.c"
char STR_REPL_DESTIN[] = "REPL_DESTIN";

///////////////////////////////// loading extensions ///////////////////////////
#include "xsa.h"

int get_xsa_version(void)
{ return XSA_VERSION; }

bool has_xml_licence(cdp_t cdp);
int get_system_charset(void);
void * get_native_variable_val(cdp_t cdp, const char * variable_name, int & type, t_specif & specif, int & vallen, bool importing);
bool write_data(cdp_t cdp, ttablenum tablenum, trecnum current_extrec, int column_num, const void * val, int vallen);
BOOL request_error_with_msg(cdp_t cdp, int errnum, const char * msg);
void append_native_variable_val(cdp_t cdp, const char * variable_name, const void * wrval, int bytes, bool appending);
bool translate_by_function(cdp_t cdp, const char * funct_name, wuchar * str, int bufsize);
char * get_web_access_params(cdp_t cdp);

xsa the_xsa =
{ get_xsa_version,
  make_scope_from_cursor,
  add_cursor_structure_to_scope,
  load_cursor_record_to_rscope,
  release_cursor_rscope,
  create_and_activate_rscope,
  deactivate_and_drop_rscope,
  make_complete,
  get_recnum_list,
  destruct_scope,
  access_locdecl_struct,
  name_find2_obj,
  ker_load_object,
  ker_free,
  commit,
  roll_back,
  request_error,
  kernel_get_descr,
  open_cursor_in_current_kontext,
  close_working_cursor,
  get_cursor, 
  request_generic_error,
  ker_alloc,

  eius_prepare_orig,
  eius_drop,
  eius_clear,
  eius_store_data,
  eius_get_dataptr,
  eius_insert,
  eius_read_column_value,

  has_xml_licence,
  get_native_variable_val,
  write_data,
  request_error_with_msg,
  append_native_variable_val,
  get_system_charset,
  translate_by_function,
  eius_prepare,
  get_web_access_params
};

t_server_ext * loaded_extensions = NULL;

typedef BOOL WINAPI t_load_ext(xsa * xsapiIn, ext_objdef ** pobjdefs, unsigned * pobjcnt);

void instantiate_generic_name(const char * name, char * myname)
{ strcpy(myname, name);  //bool add_version=false;
  if (!memcmp(name, "602xml", 6) || !memcmp(name, "602XML", 6))
  { const char * p = name+6;
    while (*p>='0' && *p<='9') p++;
    if (!*p) // add_version=true;
      strcpy(myname, XML_EXT_NAME);
  }
}

bool load_server_extension(cdp_t cdp, const char * name)
{ ProtectedByCriticalSection cs(&cs_short_term, cdp, WAIT_CS_SHORT_TERM);
  char myname[sizeof(loaded_extensions->extname)+3];  // space for 602 version number
  instantiate_generic_name(name, myname);
 // search among the loaded extensions:
  t_server_ext * ext = loaded_extensions;
  while (ext)
    if (!strcmp(ext->extname, myname)) return true;
    else ext=ext->next;
 // load it:
  HINSTANCE hInst;
#ifdef LINUX  // try first the name with prefix lib and then without prefix - reducing the error messages
  { char pathname[MAX_PATH];
    sprintf(pathname, "%s/lib/" WB_LINUX_SUBDIR_NAME "/lib%s.so", PREFIX, myname);
    hInst = LoadLibrary(pathname);
    if (!hInst)
    { sprintf(pathname, "%s/lib/" WB_LINUX_SUBDIR_NAME "/%s.so", PREFIX, myname);
      hInst = LoadLibrary(pathname);
    }
  }
#else
  hInst = LoadLibrary(myname);  
#endif  
  if (!hInst) return false;
  t_load_ext * p_load_ext = (t_load_ext *)GetProcAddress(hInst, "Load_602_extension");
  if (!p_load_ext) { FreeLibrary(hInst);  return false; }
 // init the extension and get the description of its objects:
  ext_objdef * objdefs;  unsigned objcnt;
  if (!(*p_load_ext)(&the_xsa, &objdefs, &objcnt))  // init error
    { FreeLibrary(hInst);  return false; }
 // add the extension into the list:
  ext = new t_server_ext;
  if (!ext) { request_error(cdp, OUT_OF_KERNEL_MEMORY);  return false; }
  ext->hInst=hInst;
  strmaxcpy(ext->extname, myname, sizeof(ext->extname));
  ext->objdefs=objdefs;  ext->objcnt=objcnt;
  ext->next=loaded_extensions;  loaded_extensions=ext;
  return true;
}

void mark_extensions(void)
{ for (t_server_ext * ext = loaded_extensions;  ext;  ext=ext->next)
    mark(ext);
}

extension_object * search_obj_in_ext(cdp_t cdp, const char * name, const char * prefix, HINSTANCE * hInst)  
// Searches the object [name] in the table of standard objects, returns the object description or NULL if not found.
{ bool prefix_found = false;
  char myprefix[sizeof(loaded_extensions->extname)+3];  // space for 602 version number
  if (prefix) instantiate_generic_name(prefix, myprefix);  else *myprefix=0;  // both NULL and "" prefixes used!
  for (t_server_ext * ext = loaded_extensions;  ext;  ext=ext->next)
  { if (*myprefix)   
      if (sys_stricmp(myprefix, ext->extname)) continue;
    prefix_found=true;
    int ind;
    if (!search_table(name, ext->objdefs[0].name, ext->objcnt, sizeof(sobjdef), &ind))
    { *hInst = ext->hInst;
      return ext->objdefs[ind].descr;
    }
  }
  if (*myprefix && !prefix_found)  // try to load the extension
    if (load_server_extension(cdp, prefix))  // retry if loaded
      return search_obj_in_ext(cdp, name, prefix, hInst);
  return NULL;
}

bool has_xml_licence(cdp_t cdp)  // not called in the unblocked versions
{ uns32 buf;
#if WBVERS>1000
  get_server_info(cdp, OP_GI_LICS_XML, &buf);
#else  // common licence for fulltext and XML in versions <=10.0
  get_server_info(cdp, OP_GI_LICS_FULLTEXT, &buf);
#endif  
  return buf!=0;
}

int get_system_charset(void)
{ return sys_spec.charset; }

t_locdecl * access_native_variable(cdp_t cdp, const char * variable_name, int & level)
{ if (!cdp->rscope) return NULL;
  t_rscope * rscope = cdp->rscope;  
 // ignore parametres of the XML function, record window scopes:
  while (rscope && (rscope->level==-1 || rscope->level==1000 || rscope->level==1001))
    rscope=rscope->next_rscope;
  t_scope * scope_list = create_scope_list_from_run_scopes(rscope);  // rscope changed by this call, must not use cdp->rscope!
  compil_info xCI(cdp, NULL, NULL);
  t_sql_kont sql_kont;  xCI.sql_kont=&sql_kont;  xCI.sql_kont->active_scope_list=scope_list;
  t_locdecl * locdecl = find_name_in_scopes(&xCI, variable_name, level, LC_VAR);
  if (locdecl==NULL || locdecl->loccat!=LC_VAR) return NULL;
  return locdecl;
}

void * get_native_variable_val(cdp_t cdp, const char * variable_name, int & type, t_specif & specif, int & vallen, bool importing)
{ int level;
  t_locdecl * locdecl = access_native_variable(cdp, variable_name, level);
  if (!locdecl) return NULL;
  type = locdecl->var.type;  specif=locdecl->var.specif;
  vallen = simple_type_size(type, specif);
  void * dataptr = variable_ptr(cdp, level, locdecl->var.offset);
  if (IS_HEAP_TYPE(type)) 
  { t_value * val = (t_value*)dataptr;
    vallen = importing ? (val->vmode==mode_indirect ? val->indir.vspace : MAX_DIRECT_VAL) : val->length;
    dataptr=val->valptr();
  }
  return dataptr;
}

void append_native_variable_val(cdp_t cdp, const char * variable_name, const void * wrval, int bytes, bool appending)
// for var-len variables only
{ int level;
  t_locdecl * locdecl = access_native_variable(cdp, variable_name, level);
  if (!locdecl) return;
  if (!IS_HEAP_TYPE(locdecl->var.type)) return;
  t_value * val = (t_value*)variable_ptr(cdp, level, locdecl->var.offset);
  if (appending)
  { if (val->reallocate(val->length+bytes)) return;
    memcpy(val->valptr()+val->length, wrval, bytes);
    val->length+=bytes;  
  }
  else
  { if (val->allocate(bytes)) return;
    memcpy(val->valptr(), wrval, bytes);
    val->length=bytes;  
  }
  val->valptr()[val->length]=val->valptr()[val->length+1]=0;
}

bool write_data(cdp_t cdp, ttablenum tablenum, trecnum current_extrec, int column_num, const void * val, int vallen)
{ table_descr_auto td(cdp, tablenum);
  if (td->me())
  { if (IS_HEAP_TYPE(td->attrs[column_num].attrtype))
      return !tb_write_var(cdp, td->me(), current_extrec, column_num, 0, vallen, val);
    else
      return !tb_write_atr(cdp, td->me(), current_extrec, column_num, val);
  }
  else return false;
}

BOOL request_error_with_msg(cdp_t cdp, int errnum, const char * msg)
{ SET_ERR_MSG(cdp, msg);  return request_error(cdp, errnum); }

bool translate_by_function(cdp_t cdp, const char * funct_name, wuchar * str, int bufsize)
{ tobjname fnc_name;  int i;  bool res=false;
  strmaxcpy(fnc_name, funct_name, sizeof(fnc_name));
  sys_Upcase(fnc_name);
  tobjnum objnum = find_object(cdp, CATEG_PROC, fnc_name);
  if (objnum==NOOBJECT) return false;
  t_routine * routine = get_stored_routine(cdp, objnum, NULL, TRUE);
  if (!routine) return false;
 // calling:
  const t_scope * pscope;  pscope = routine->param_scope();
  t_rscope * rscope;  rscope = new(pscope->extent) t_rscope(pscope);
  if (rscope==NULL) { request_error(cdp, OUT_OF_KERNEL_MEMORY);  goto errex0; }
  memset(rscope->data, 0, pscope->extent);  // CLOB and BLOB vars must be init.
  rscope_init(rscope);
 // prepare returning a value:
  t_locdecl * retlocdecl;  // space for return value of internal function
  rscope->data[0]=wbfalse;  // return_not_performed_yet flag
 // activate the parameter scope, for parameter assignment only:
  rscope->next_rscope=cdp->rscope;  cdp->rscope=rscope;
 // assign input parameter values or default values:
  for (i=0;  i<pscope->locdecls.count();  i++)
  { t_locdecl * locdecl = pscope->locdecls.acc0(i);
    if (locdecl->loccat==LC_INPAR || locdecl->loccat==LC_INPAR_ || locdecl->loccat==LC_INOUTPAR)
    { int len = (int)wuclen(str);
      if (len>t_specif(locdecl->var.specif).length/sizeof(wuchar)) len=t_specif(locdecl->var.specif).length/sizeof(wuchar);
      memcpy(rscope->data+locdecl->var.offset, str, len*sizeof(wuchar));
      ((wuchar*)(rscope->data+locdecl->var.offset))[len]=0;
    }
    else if (locdecl->loccat==LC_VAR) // return value will be here (internal only)
      retlocdecl=locdecl;
  }
 // execute statements:
  rscope->level=routine->scope_level;  // must be after rscope_init which sets level to 0
  cdp->in_routine++;
 // prepare checking for cursors opened by the procedure:
  { t_exkont_proc ekt(cdp, rscope, routine);
    t_short_term_schema_context stsc(cdp, routine->schema_uuid);
   // execute:
    BOOL admin_mode_routine = is_in_admin_mode(cdp, objnum, routine->schema_uuid);
    if (admin_mode_routine) cdp->in_admin_mode++;
    res = routine->stmt->exec(cdp)==FALSE;
    process_rollback_conditions_by_cont_handler(cdp);
    if (admin_mode_routine) cdp->in_admin_mode--;
  }
  cdp->in_routine--;
  if (!res) goto errex; 
  cdp->stop_leaving_for_name(routine->name);

 // assign the return value:
  if (rscope->data[0])  // return value assigned
  { int len=(int)wuclen((wuchar*)(rscope->data+retlocdecl->var.offset));
    if (len>bufsize/sizeof(wuchar)-1) len=bufsize/sizeof(wuchar)-1;
    memcpy(str, rscope->data+retlocdecl->var.offset, len*sizeof(wuchar));
    str[len]=0;
  }
 errex:
 // destroy variables (must be done even on error!):
  if (cdp->rscope==rscope) cdp->rscope=rscope->next_rscope;  // nothing to be done for detached routine start error
  delete rscope;
 errex0:
  psm_cache.put(routine);
  return res;
}

/*********************** ident. tables **************************************/
object * search_obj(const char * name)  
// Searches the object [name] in the table of standard objects, returns the object description or NULL if not found.
{ int ind;
  if (!search_table(name, standard_table[0].name, STD_NUM, sizeof(sobjdef), &ind))
    return standard_table[ind].descr;
  return NULL;
}

CFNC int WINAPI check_atr_name(tptr name)
{ int i;
  sys_Upcase(name);
  if (!search_table(name, *keynames, KEY_COUNT, sizeof(t_idname), &i))
    return 1;  /* is an IPL keyword */
  if (!search_table(name, *sql_keynames, SQL_KEY_COUNT, sizeof(t_idname), &i))
    return 4;  /* is an IPL keyword */
  if (!search_table(name, standard_table[0].name, STD_NUM, sizeof(sobjdef), &i))
    return 2;  /* is standard identifier */
  if (!strcmp(name, "INDEX") || !strcmp(name, "ATTRIBUTE"))
    return 3;
  return 0;    /* not predefined */
}

uns32 WINAPI next_char(CIp_t CI)
{ switch (CI->src_specif.wide_char)
  { case 0:
     CI->curchar = *CI->compil_ptr;
     break;
    case 1:
     CI->curchar = *(wuchar*)CI->compil_ptr;
     break;
    case 2:
     CI->curchar = *(uns32*)CI->compil_ptr;
     break;
  }
  if (!CI->curchar) return S_SOURCE_END;  /* the delimiter can be read several times */
  CI->compil_ptr += CI->charsize;
  if (CI->curchar=='\n')
    { CI->loc_line++;  CI->loc_col=0; }
  else CI->loc_col++;
  return CI->curchar;
}

uns32 get_unsig(CIp_t CI)
{ uns32 val = 0;
  uns32 c = CI->curchar;
  if (c<'0' || c>'9') c_error(BAD_CHAR);
  do
  { //if ((val==214748364L) && (CI->curchar>'7') || (val>214748364L))
    //  c_error(NUMBER_TOO_BIG);
    val = 10*val + c - '0';  
    c = next_char(CI);
  } while (c>='0' && c<='9');
  return val;
}

static symbol scan_string(CIp_t CI, char terminator)
// Read the string. CI->curchar is its 1st character after the [terminator].
{ int i=0;  tptr p;
  do
  { do /* CI->curchar is the next char after " or ' */
    { char c = CI->curchar;
      if (c=='\n' || !c) c_error(STRING_EXCEEDS_LINE);
      next_char(CI);
#if WBVERS<900  // both " and ' must be doubled inside a string, any of them can terminate the string
      if (c=='"' || c=='\'')
#else           // string can be terminated only by the starting quote and only this character must be doubled when used inside the string
      if ((CI->cdp->sqloptions & SQLOPT_OLD_STRING_QUOTES) ? (c=='"' || c=='\'') : c==terminator)
#endif
        if (CI->curchar==c)
          next_char(CI);
        else 
          break;
     // store the [c] character:
      if ((p=(tptr)CI->string_dynar.acc(i))==NULL) c_error(OUT_OF_MEMORY);
      *p=c;
      i++;
    } while (true);
   // skip whitespaces between parts, process the # notation (obsolete):
    while (CI->curchar=='#' || CI->curchar==' ' || CI->curchar=='\n' || CI->curchar=='\r')
      if (CI->curchar=='#')
      { next_char(CI);
        uns32 intval=get_unsig(CI);
        if (intval>255) c_error(NUMBER_TOO_BIG);  // intval is unsigned. Unicode not supported here
        if ((p=(tptr)CI->string_dynar.acc(i))==NULL) c_error(OUT_OF_MEMORY);
        *p=(char)intval;
        i++;
      }
      else next_char(CI);
   // check for continuation of the string:
#if WBVERS<900  // both " and ' can start the continuation
    if (CI->curchar=='"' || CI->curchar=='\'')
#else           // string can be continued only by the starting quote (unless old comp. is on)
    if ((CI->cdp->sqloptions & SQLOPT_OLD_STRING_QUOTES) ? (CI->curchar=='"' || CI->curchar=='\'') : CI->curchar==terminator)
#endif
      next_char(CI);  // continue with the next part
    else break;  // no next part, stop
  } while (true);
 // add a terminator
  if ((p=(tptr)CI->string_dynar.acc(i))==NULL) c_error(OUT_OF_MEMORY);
  *p=0;  /* a delimiter */
  CI->curr_int=0;  // Uniciode escape character, when non-null
  return (symbol)((i==1 && terminator=='\'') ? S_CHAR : S_STRING);
}

static int hex2int(CIp_t CI, char c)
{ if (c>='0' && c<='9') return c-'0';
  if (c>='A' && c<='F') return c-'A'+10;
  if (c>='a' && c<='f') return c-'a'+10;
  c_error(BAD_CHAR_IN_HEX_LITERAL);
  return 0;
}

static void string_to_binary(CIp_t CI, BOOL hex)
{ tptr src = CI->curr_string(), dest=src;
  int len=0;
  if (hex)
  { while (src[0] && src[1])
    { dest[len++]=(hex2int(CI, src[0])<<4) + hex2int(CI, src[1]);
      src+=2;
    }
    if (*src) dest[len++]=hex2int(CI, *src) << 4;
    dest[len]=0;  // I don't know if this is important
  }
  else  // binary: bits
  { int bit = 128, val = 0;
    while (*src)
    { if (*src=='1') val+=bit;
      else if (*src!='0') c_error(BAD_CHAR_IN_HEX_LITERAL);
      if (bit>1) bit >>= 1;
      else { bit=128;  dest[len++]=val;  val=0; }
      src++;
    }
    if (bit<128) // imcomplete byte in val
      dest[len++]=val;
  }
  CI->curr_int = len;  // the length of the literal
}

bool comp_read_displ(CIp_t CI, int * displ)
// Must not read the sign unless a digit follows
{ uns32 hours, minutes;  
  if ((CI->curchar=='+' || CI->curchar=='-') && *CI->compil_ptr>='0' && *CI->compil_ptr<='9') 
  { bool minus = CI->curchar=='-';  
    next_char(CI);  // skip the sign
    hours=get_unsig(CI);
    if (CI->curchar==':') 
      { next_char(CI);  minutes=get_unsig(CI); }
    else minutes=0;     
    if (hours>12 || minutes>59) c_error(BAD_TIME);
    *displ = 60*(60*hours+minutes);
    if (minus) *displ=-*displ;
    return true;
  }
  else return false;
}


static uns32 read_odbc_time(CIp_t CI, BOOL with_fraction)
{ int hours, minutes, seconds, fract;
  while (CI->curchar==' '||CI->curchar=='\r'||CI->curchar=='\n')
    next_char(CI);
  if (CI->curchar=='\'') next_char(CI);  // not present in datetime constant
  hours  =get_unsig(CI);
  if (CI->curchar!=':') c_error(BAD_TIME);
  next_char(CI);  minutes=get_unsig(CI);
  if (CI->curchar!=':') c_error(BAD_TIME);
  next_char(CI);  seconds=get_unsig(CI);

  fract=0;
  if (CI->curchar=='.')
  { uns32 c=next_char(CI);
    if (c>='0' && c<='9')
    { fract=100*(c-'0');
      c=next_char(CI);
      if (c>='0' && c<='9')
      { fract+=10*(c-'0');
        c=next_char(CI);
        if (c>='0' && c<='9')
        { fract+=c-'0';
          do c=next_char(CI);  // skip other digits
          while (c>='0' && c<='9');
        }
      }
    }
  }

  uns32 tm = Make_time(hours, minutes, seconds, with_fraction ? fract : 0);
  // must ignore fraction for ODBC time, cannot use fraction form ODBC datetime
  if (tm==NONETIME) c_error(BAD_TIME);
  return tm;
}

static uns32 read_odbc_time_with_zone(CIp_t CI, BOOL with_fraction)
{ uns32 tm = read_odbc_time(CI, with_fraction);
  int displ;
  if (comp_read_displ(CI, &displ)) 
  { tm=displace_time(tm, -displ);         // -> UTC
    tm=displace_time(tm,  CI->cdp->tzd);  // UTC -> ZULU
  }
  if (CI->curchar!='\'') c_error(BAD_TIME);
  next_char(CI);
  return tm;
}

static uns32 read_odbc_date(CIp_t CI)
{ char sdate[20];  tptr p, q;  int day, month, year;
  strcpy(sdate, "yyyy-mm-dd");
  while (CI->curchar==' '||CI->curchar=='\r'||CI->curchar=='\n')
    next_char(CI);
  if (CI->curchar!='\'') c_error(BAD_DATE);
  next_char(CI);
  p=sdate;
  do
  { q=p;  while (*p==*q) p++;
    switch (*q)
    { case 'd':  case 'D':
        day  =(int)get_unsig(CI);
        break;
      case 'm':  case 'M':
        month=(int)get_unsig(CI);
        break;
      case 'y':  case 'Y':
        year =(int)get_unsig(CI);
        if (p==q+2) year+=1900;
        break;
      default:
        if (CI->curchar!=*q) c_error(BAD_DATE);
        next_char(CI);  break;
    }
  } while (*p);
  if (CI->curchar=='\'') next_char(CI);
  uns32 dat = Make_date(day, month, year);
  if (dat==NONEDATE) c_error(BAD_DATE);
  return dat;
}

static uns32 read_odbc_timestamp_with_zone(CIp_t CI)
{ uns32 dat=read_odbc_date(CI);
  uns32 tim=read_odbc_time(CI, FALSE);
  uns32 dtm=datetime2timestamp(dat, tim);
  int displ;
  if (comp_read_displ(CI, &displ)) 
  { dtm=displace_timestamp(dtm, -displ);        // -> UTC
    dtm=displace_timestamp(dtm, CI->cdp->tzd);  // UTC -> ZULU
  }
  if (CI->curchar!='\'') c_error(BAD_TIME);
  next_char(CI);
  return dtm;
}

void t_last_comment::add_to_comment(const char * start, const char * stop)
{ if (start>=stop) return;  // ignoring empty comments, preventing possible errors when reading the source per-partes
  int length = (int)(stop-start);
  if (length>1000) return;  // preventing possible errors when reading the source per-partes
  int act_len = comm.count();
  if (act_len) act_len--;  // stipping the null terminator
  if (comm.acc(act_len+length+(act_len ? 2 : 0)+1) == NULL) return;
  memcpy(comm.acc0(act_len), start, length);
  if (act_len) { memcpy(comm.acc0(act_len+length), "\r\n", 2);  length+=2; }
  *(char*)comm.acc0(act_len+length) = 0;
}

bool apostr_follows(CIp_t CI)
{
  if (CI->curchar=='\'') return true;
  if (CI->curchar!=' ') return false;
  const char * ptr = CI->compil_ptr;
  while (*ptr==' ') ptr++;
  return *ptr=='\'';
}

CFNC symbol WINAPI next_sym(CIp_t CI)
/* returns the next input symbol, sets the cur_. variables */
{ uns32 c;  int i;  
  wuchar wide_ident[OBJ_NAME_LEN+1];  // conversion functions for 32-bit character strings are not available
  CI->prev_line=CI->loc_line; 
  CI->last_comment.clear();
commentloop:   // goes here after processing a comment
  /* first skip spaces & delimiters */
  while (CI->curchar==' ' || CI->curchar=='\r' || CI->curchar=='\n' || CI->curchar=='\t')  // skips tabulator too (ODBC staements may contain it)
    next_char(CI);
  CI->sc_col=CI->loc_col;
  if (CI->sc_col) CI->sc_col--; /* the beginning of the current symbol */

  CI->prepos=CI->compil_ptr-CI->charsize;
  c=CI->curchar;
  CI->cursym=(symbol)c;  next_char(CI);
  switch (c)
  { case '+': case '^': case ',': case '*': case '~':
    case ';': case ')': case '[': case ']': case '?': case '\\':
      return (symbol)c;
    case S_SOURCE_END:
      CI->prepos+=CI->charsize;
      return (symbol)c;
    case 0x1a:
      return CI->cursym=0;
    case '#':
#ifndef SRVR
      if (CI->curchar=='#')
        { next_char(CI); return CI->cursym=S_PAGENUM; }
      else 
#endif
      return (symbol)'#';
    case '@':
      if (CI->curchar=='@')
      { next_char(CI); 
       // check for system-reserved identifiers (national characters not allowed in them):
        if (CI->curchar>='A' && CI->curchar<='Z' || CI->curchar>='a' && CI->curchar<='z')  
        { CI->curr_name[0]=CI->curr_name[1]='@';
          CI->curr_name[2]=CI->curchar;  
          i=3;  next_char(CI); 
          do
          { c=CI->curchar;
            if (c>='A' && c<='Z' || c>='a' && c<='z' || c>='0' && c<='9' || c=='_')
            { if (i<OBJ_NAME_LEN)
                CI->curr_name[i++] = (char)c;
            }
            else break;
            next_char(CI);
          } while (true);
          CI->curr_name[i]=0;  /* delimiter */
          sys_Upcase(CI->curr_name);  // in fact, only ASCII charcters can be here
          return CI->cursym=S_IDENT;
        }
       // the @@ symbol:
        return CI->cursym=S_DBL_AT; 
      }
      return (symbol)'@';
    case ':':
      if (CI->curchar=='=')
        { next_char(CI); return CI->cursym=S_ASSIGN; }
      else return (symbol)':';
    case '.':
      if (CI->curchar==')')
        { next_char(CI); return CI->cursym=(symbol)']'; }
      else if (CI->curchar=='.')  // SQL 2003
        { next_char(CI); return CI->cursym=S_DDOT; }
      else if (CI->curchar=='=')
      { if (next_char(CI)=='.')
          { next_char(CI); return CI->cursym=S_SUBSTR; }
        return CI->cursym=S_PREFIX;
      }
      else if (CI->curchar>='0' && CI->curchar<='9')
        goto number;  // ODBC requires .123 and .123E56
      else return (symbol)'.';
    case '<':
      if (CI->curchar=='>')
        { next_char(CI); return CI->cursym=S_NOT_EQ; }
      else if (CI->curchar=='=')
        { next_char(CI); return CI->cursym=S_LESS_OR_EQ; }
      else return (symbol)'<';
    case '>':
      if (CI->curchar=='=')
        { next_char(CI); return CI->cursym=S_GR_OR_EQ; }
      else return (symbol)'>';
    case '!':   /* for C & SQL */
      if (CI->curchar=='=')
        { next_char(CI); return CI->cursym=S_NOT_EQ; }
      else if (CI->curchar=='<')  /* for SQL */
        { next_char(CI); return CI->cursym=S_GR_OR_EQ; }
      else if (CI->curchar=='>')  /* for SQL */
        { next_char(CI); return CI->cursym=S_LESS_OR_EQ; }
      else if (CI->curchar=='!')
        { next_char(CI); return CI->cursym=S_CURRVIEW; }
      else return '!';
    case '=':   /* for C */
      if (CI->curchar=='=') next_char(CI);
      return CI->cursym;  /* no change */
    case '|':   /* for C */
      if (CI->curchar=='|')  // SQL 2003
        { next_char(CI); return CI->cursym=SQ_CONCAT; }
      else return (symbol)'|';
    case '&':   /* for C */
      if (CI->curchar=='&')
        { next_char(CI); return CI->cursym=S_AND; }
      else return (symbol)'&';
    case '/':
      if (CI->curchar=='*')  /* C & SQL comment */
      { next_char(CI);
        do
        { if (CI->curchar=='*')
          { if (next_char(CI)=='/') break; }
          else
          { if (CI->curchar=='\0') c_error(EOF_IN_COMMENT);
            next_char(CI);
          }
        } while (TRUE);
        CI->last_comment.add_to_comment(CI->prepos+2*CI->charsize, CI->compil_ptr-2*CI->charsize);
        next_char(CI);  goto commentloop;
      }
      else if (CI->curchar=='/')  /* C++ line comment, not required by SQL */
      { do
        { next_char(CI);
        } while (CI->curchar!='\r' && CI->curchar!='\n' && CI->curchar);
        CI->last_comment.add_to_comment(CI->prepos+2*CI->charsize, CI->compil_ptr-CI->charsize);
        goto commentloop;
      }
      return (symbol)'/';
    case '-':
      if (CI->curchar=='-')  /* SQL line comment, ODBC 2.x long extension format not supported */
      { do
        { next_char(CI);
        } while (CI->curchar!='\r' && CI->curchar!='\n' && CI->curchar);
        CI->last_comment.add_to_comment(CI->prepos+2*CI->charsize, CI->compil_ptr-CI->charsize);
        goto commentloop;
      }
      return (symbol)'-';
    case '(':
      if (CI->curchar=='.')
        { next_char(CI); return CI->cursym=(symbol)'['; }
      return '(';
    case '}':   /* end of {oj ... } etc */
      goto commentloop;
    case '{':
    {/* check for ODBC extensions */
      while ((CI->curchar==' ')||(CI->curchar=='\r')||(CI->curchar=='\n'))
        next_char(CI);
      if ((CI->curchar & 0xffdf)=='D')    // date literal
      { next_char(CI);
        if (apostr_follows(CI))
        { CI->curr_int=read_odbc_date(CI);
          return CI->cursym=S_DATE;
        }
      }
      else if ((CI->curchar & 0xffdf)=='T')  // time or timestamp literal
      { next_char(CI);
        if ((CI->curchar & 0xffdf)=='S')  // ODBC timestamp value
        { next_char(CI);
          if (apostr_follows(CI))
          { CI->curr_int=read_odbc_timestamp_with_zone(CI);
            return CI->cursym=S_TIMESTAMP;
          }
        }
        else if (apostr_follows(CI))  // ODBC time value
        { CI->curr_int=read_odbc_time_with_zone(CI, FALSE);
          return CI->cursym=S_TIME;
        }
      }
      else if ((CI->curchar & 0xffdf)=='O')  // outer join
      { next_char(CI);
        if ((CI->curchar & 0xffdf)=='J')
        { next_char(CI);
          goto commentloop;
        }
      }
      else if (CI->curchar=='?')  // calling a function
      { //next_char(CI);
        //return (symbol)'?';
        return CI->cursym=(symbol)S_SET;
      }
      else if ((CI->curchar & 0xffdf)=='C')  // calling a routine
      { if ((next_char(CI)&0xffdf)=='A' && (next_char(CI)&0xffdf)=='L' && (next_char(CI)&0xffdf)=='L')
        { next_char(CI);
          return CI->cursym=(symbol)SQ_CALL;
        }
      }
      else if ((CI->curchar & 0xffdf)=='F')  // calling scalar function
      { next_char(CI);
        if ((CI->curchar & 0xffdf)=='N')
        { next_char(CI);
          goto commentloop;
        }
      }
      else if ((CI->curchar & 0xffdf)=='E')  // LIKE - ESCAPE clause
      { if ((next_char(CI)&0xffdf)=='S' && (next_char(CI)&0xffdf)=='C' && (next_char(CI)&0xffdf)=='A' && (next_char(CI)&0xffdf)=='P' && (next_char(CI)&0xffdf)=='E')
        { next_char(CI);
          return CI->cursym=(symbol)SQ_ESCAPE;
        }
      }
     /* if ODBC ext. not recognized, continuing like normal comment */
     // pascal-style comment, skip it:
      do
      { if (CI->curchar=='}') break;
        if (CI->curchar=='\0') c_error(EOF_IN_COMMENT);
        next_char(CI);
      } while (TRUE);
      next_char(CI);  
      CI->last_comment.add_to_comment(CI->prepos+CI->charsize, CI->compil_ptr-2*CI->charsize);
      goto commentloop;
    }
    case '\'':
      return CI->cursym=scan_string(CI, '\'');
    case '"':  
      if (CI->cdp->sqloptions & SQLOPT_QUOTED_IDENT) return CI->cursym=scan_string(CI, '"');
      // cont.
    case '`': /* quouted identifier */
      i=0;
      while (CI->curchar != c)  // must be terminated with the same character
      { if (CI->curchar=='\n' || !CI->curchar) c_error(STRING_EXCEEDS_LINE);
        if (i<OBJ_NAME_LEN)
        { CI->name_with_case[i] = (char)(wide_ident[i] = (wuchar)CI->curchar);
          i++;
        }
        next_char(CI);
      }
      next_char(CI);  // skip the ending quote
      if (!i) c_error(BAD_CHAR);  // empty identifier is not allowed
      CI->name_with_case[i] = (char)(wide_ident[i]=0);  /* delimiter */
      if (CI->src_specif.wide_char)
        if (conv2to1(wide_ident, -1, CI->name_with_case, CI->src_specif.charset, 0)) 
          c_error(BAD_CHAR);  // inconvertible
      convert_to_uppercaseA(CI->name_with_case, CI->curr_name, CI->src_specif.charset);
      return CI->cursym=S_IDENT;
    default:  /* identifiers, numbers, bad chars */
      if ((CI->src_specif.wide_char ? c>='A' && c<='Z' || c>='a' && c<='Z' : is_AlphaA(c, CI->src_specif.charset)) || c=='_')
      { if (CI->curchar=='\'' && (c=='X' || c=='B' || c=='N'))  // literal
        { next_char(CI);
          CI->cursym=scan_string(CI, '\'');
          if (c=='X' || c=='B')
          { string_to_binary(CI, c=='X');
            CI->cursym=S_BINLIT;  // length stored in CI->curr_int
          }
          return CI->cursym;
        }
        if (c=='U' && CI->curchar=='&')
        { uns32 starter = next_char(CI);
          if (starter != '\'' && starter != '`' && starter != '"') c_error(BAD_CHAR);
          next_char(CI);
          CI->cursym=scan_string(CI, starter);
          if (CI->curchar=='U' && !memcmp(CI->compil_ptr, "ESCAPE'", 7) && CI->compil_ptr[8]=='\'')   // UESCAPE uses quote even if the tarter is different
            { CI->curr_int=CI->compil_ptr[7];  CI->compil_ptr+=9;  next_char(CI); }
          else
            CI->curr_int='\\';  // the default escape character
          if (starter=='`' || starter=='"' && !(CI->cdp->sqloptions & SQLOPT_QUOTED_IDENT))  // Unicode identifier, convert
          { char * src = CI->curr_string(); char * dest = src;
            while (*src)
            { if (*src==CI->curr_int)  // escape
                if (src[1]==CI->curr_int)
                  { *dest=*src;  src+=2; }
                else  // Unicode escape value
                { src++;
                  int digits;   uns16 val=0;
                  if (*src=='+')
                    { digits=6;  src++;  c_error(BAD_CHAR); }
                  else digits=4;
                  while (digits--)
                  { val=16*val;
                    if (*src>='0' && *src<='9') val += *src-'0';
                    else if (*src>='A' && *src<='Z') val += *src-'A'+10;
                    else if (*src>='a' && *src<='z') val += *src-'a'+10;
                    else c_error(BAD_CHAR);
                    src++;
                  }
                 // convert to the system charset
                  wuchar str2[2];  str2[0]=val;   str2[1]=0;
                  if (superconv(ATT_STRING, ATT_STRING, str2, dest, t_specif(0, 0, 0, 1), sys_spec, NULL)<0)
                    c_error(BAD_CHAR);
                }
              else
                { *dest=*src;  src++; }
              dest++;  
            }
            *dest=0;
            strmaxcpy(CI->name_with_case, CI->curr_string(), sizeof(CI->name_with_case));
            goto ident_read;
          }
          return CI->cursym;
        }
       // identifier
        CI->name_with_case[0] = (char)(wide_ident[0] = (wuchar)c);
        i=1;
        do
        { c=CI->curchar;
          if ((CI->src_specif.wide_char ? c>='A' && c<='Z' || c>='a' && c<='Z' : is_AlphaA(c, CI->src_specif.charset)) || c>='0' && c<='9' || c=='_')
          { if (i<OBJ_NAME_LEN)
            { CI->name_with_case[i] = (char)(wide_ident[i] = (wuchar)c);
              i++;
            }
          }
          else break;
          next_char(CI);
        } while (true);
        CI->name_with_case[i] = (char)(wide_ident[i]=0);  /* delimiter */
       ident_read:
        if (CI->src_specif.wide_char)
          if (conv2to1(wide_ident, -1, CI->name_with_case, CI->src_specif.charset, 0)) 
            c_error(BAD_CHAR);  // inconvertible
        convert_to_uppercaseA(CI->name_with_case, CI->curr_name, CI->src_specif.charset);
        if (search_table(CI->curr_name, sql_keynames[0], SQL_KEY_COUNT, sizeof(t_idname),&i))
        { //if (CI->curchar=='\'' &&
          //    CI->curr_name[0]=='_')  // national character set name
          //{ next_char(CI);
          //  CI->cursym=scan_string(CI);
          //}  -- not for I-level
          /*else*/ CI->cursym=S_IDENT;
        }
        else  // found in the keyword table
        { CI->cursym=(symbol)(i+0x80);  // keyword symbol value
         // SQL keywords
          if (CI->curchar=='\'')
          { switch (CI->cursym)
            { case SQ_TIME:
                CI->curr_int=read_odbc_time_with_zone(CI, TRUE);
                CI->cursym=S_TIME;  break;
              case SQ_DATE:
                CI->curr_int=read_odbc_date(CI);
                CI->cursym=S_DATE;  break;
              case SQ_TIMESTAMP:
                CI->curr_int=read_odbc_timestamp_with_zone(CI);
                CI->cursym=S_TIMESTAMP;  break;
            }
          } // followed by '
        } // found in the keyword table
        return CI->cursym;
      } // does not start with a letter
     number:  // number starting with the dot
      // scanning a number: integer, real, money, date, time 
      { CI->curr_scale=0;  BOOL in_decpart;
#define MAX_NUM_LEN 30
        char num[MAX_NUM_LEN+4];  num[0]=c;  int numpos=1;  // string copy of the number, user for real conversion
       // process the 1st character already in c:
        if (c>='0' && c<='9') { CI->curr_int = c-'0';  in_decpart=FALSE; }
        else if (c=='.') { CI->curr_int = 0;  in_decpart=TRUE; }
        else c_error(BAD_CHAR);
       // process the other characters nad digits:
        BOOL is_money = FALSE;
        do
        { c=CI->curchar;
          if (numpos>=MAX_NUM_LEN) c_error(NUMBER_TOO_BIG);
          if (c>='0' && c<='9')
          { if (CI->curr_int >= MAXBIGINT / 10)
              if (CI->curr_int > MAXBIGINT / 10 || c > MAXBIGINT % 10)
                c_error(NUMBER_TOO_BIG);
            CI->curr_int = 10*CI->curr_int + (c-'0');
            if (in_decpart) CI->curr_scale++;
          }
          else if ((c=='.' || c=='$') && !in_decpart)
          { in_decpart=TRUE;
            if (c=='$') { is_money=TRUE;  c='.'; }
          }
          else break;
          num[numpos++]=c;
          next_char(CI);
        } while (TRUE);
       // 'E' means the semilogaritmic literal:
        if (c=='E' || c=='e')
        {// read the rest of the real number 
          num[numpos++]=c;
          next_char(CI);
          if (CI->curchar=='-')
            { num[numpos++]=CI->curchar;  next_char(CI); }
          else if (CI->curchar=='+')
            { num[numpos++]=CI->curchar;  next_char(CI); }
          i=0;
          while ((CI->curchar>='0')&&(CI->curchar<='9'))
          { if (numpos>=MAX_NUM_LEN) c_error(NUMBER_TOO_BIG);
            num[numpos++]=CI->curchar;
            i=10*i+CI->curchar-'0';
            if (i>308) c_error(NUMBER_TOO_BIG);
            next_char(CI);
          }
          num[numpos]=0;
         // convert and return the real literal:
          //CI->curr_real=strtod(num, NULL);  -- bad, strtod may need locale-dependent decimal separator
          if (!str2real(num, &CI->curr_real)) c_error(BAD_CHAR);
          return CI->cursym=S_REAL;
        }
       // ':' means the time literal:
        if (c==':')  
        { do
          { if (numpos>=MAX_NUM_LEN) c_error(NUMBER_TOO_BIG);
            num[numpos++]=c;
            c=next_char(CI);
          } while (c>='0' && c<='9' || c==':' || c=='.' || (c=='+' || c=='-') && *CI->compil_ptr>='0' && *CI->compil_ptr<='9');
          num[numpos]=0;
          uns32 tm;
          if (!str2timeUTC(num, &tm, false, CI->cdp->tzd)) c_error(BAD_TIME);
          CI->curr_int=tm;
          return CI->cursym=S_TIME;
        }
       // another dot -> date or timestamp
        if (c=='.')  
        { do
          { if (numpos>=MAX_NUM_LEN) c_error(NUMBER_TOO_BIG);
            num[numpos++]=c;
            c=next_char(CI);
          } while (c>='0' && c<='9' || c=='.');
          while (c==' ') c=next_char(CI);
          if (c>='0' && c<='9')
          { if (numpos>=MAX_NUM_LEN) c_error(NUMBER_TOO_BIG);
            num[numpos++]=' ';
            do
            { if (numpos>=MAX_NUM_LEN) c_error(NUMBER_TOO_BIG);
              num[numpos++]=c;
              c=next_char(CI);
            } while (c>='0' && c<='9' || c==':' || c=='.' || c=='+' || c=='-');
            num[numpos]=0;
            uns32 ts;
            if (!str2timestampUTC(num, &ts, false, CI->cdp->tzd)) c_error(BAD_TIME);
            CI->curr_int=ts;
            return CI->cursym=S_TIMESTAMP;
          }
          else
          { num[numpos]=0;
            uns32 dt;
            if (!str2date(num, &dt)) c_error(BAD_DATE);
            CI->curr_int=dt;
            return CI->cursym=S_DATE;
          }
        }
       // integer (possibly scaled) or money:
        if (is_money) // separator '$' used, convert to scale 2:
        { if (!scale_int64(CI->curr_int, 2-CI->curr_scale) || CI->curr_int>MAXBIGINT || CI->curr_int<-MAXBIGINT)
            c_error(NUMBER_TOO_BIG);
          CI->curr_scale=2;
          return CI->cursym=S_MONEY; 
        }
        return CI->cursym=S_INTEGER; 
      }
  }
}

void WINAPI next_and_test(CIp_t CI, symbol sym, short message)
{ if (next_sym(CI)!=sym) c_error(message); }

void WINAPI test_and_next(CIp_t CI, symbol sym, short message)
{ if (CI->cursym!=sym) c_error(message); next_sym(CI); }

BOOL test_signum(CIp_t CI)
{ if (CI->cursym=='-') { next_sym(CI); return TRUE; }
  else if (CI->cursym=='+') next_sym(CI);
  return FALSE;
}

void c_end_check(CIp_t CI)
{ if (CI->cursym!=S_SOURCE_END) c_error(GARBAGE_AFTER_END); }

/*************************** compiler entry *********************************/
void compiler_init(void)  /* called only once */
{ nullreal_cdesc.value.real=NULLREAL;
  nullsig16_cdesc.value.int16=(sig16)0x8000;
  nullsig32_cdesc.value.int32=
  nulltime_cdesc.value.int32=nulldate_cdesc.value.int32=
  nullts_cdesc.value.int32=
  nullmoney_cdesc.value.moneys.money_hi4=NONEINTEGER;
  nullmoney_cdesc.value.moneys.money_lo2=0;
}

int WINAPI compile_direct(CIp_t CI)
{ 
#ifdef CPPEXCEPTIONS
  try
#else
#ifdef WINS
  __try
#else
  int comp_err=Catch(CI->comp_jmp);
  if (!comp_err)
#endif
#endif
  {/* clean-up after the last compilation */
    CI->loc_line=1;
    CI->sc_col=CI->loc_col=0;  /* position in the source file */
    CI->compil_ptr=CI->univ_source;
    next_char(CI);  next_sym(CI);
    (*CI->startnet)(CI);  /* starting the compilation */
    return COMPILED_OK; /* compiled ok */
  }
#ifdef CPPEXCEPTIONS
  catch (unsigned code)
  { if (VALID_COMP_ERR_CODE(code)) 
      return code;
    else throw;  // re-throws unhandled exceptions
  }
  catch (int code)
  { if (VALID_COMP_ERR_CODE(code)) 
      return code;
    else throw;  // re-throws unhandled exceptions
  }
  catch (short code)
  { if (VALID_COMP_ERR_CODE(code)) 
      return code;
    else throw;  // re-throws unhandled exceptions
  }
  catch (unsigned short code)
  { if (VALID_COMP_ERR_CODE(code)) 
      return code;
    else throw;  // re-throws unhandled exceptions
  }
#else
#ifdef WINS
  __except ((GetExceptionCode() >= FIRST_COMP_ERROR &&
             GetExceptionCode() <= LAST_COMP_ERROR ||
             GetExceptionCode() >= 128 &&
             GetExceptionCode() <= LAST_DB_ERROR ||
             GetExceptionCode()==99)  /* ignored error */
       ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)
  { return (int)GetExceptionCode();
  }
#else
  else
    return comp_err;
#endif
#endif
}

void create_error_contect(CIp_t CI)
// To be called just after compilation in CI returns error
// Stored error context in cdp->comp_err_line, cdp->comp_err_column, CI->cdp->comp_kontext
{ cdp_t cdp = CI->cdp;
 // error position info:
  cdp->comp_err_line   = CI->loc_line;
  cdp->comp_err_column = CI->sc_col;
 // compilation error kontext:  
  if (CI->cdp->comp_kontext==NULL) // not allocated yet
    CI->cdp->comp_kontext=(tptr)corealloc(PRE_KONTEXT+POST_KONTEXT+2, 65);
  if (CI->cdp->comp_kontext!=NULL) // allocated OK
  { int presize, postsize;  BOOL leader=FALSE, trailer=FALSE;
   // calculate size of kontext parts:
    const char * refpos = CI->prepos;
    presize  = (int)(refpos-CI->univ_source);  if (presize<0) presize=0;
    if (*refpos==' ' && presize) { refpos--;  presize--; } // 1 space back when positioned after a space
    postsize = (int)strlen(refpos);
    if (presize <= PRE_KONTEXT)
    { if (postsize > PRE_KONTEXT-presize+POST_KONTEXT)
        { postsize = PRE_KONTEXT-presize+POST_KONTEXT;  trailer=TRUE; }
    }
    else if (postsize <= POST_KONTEXT)
    { if (presize > POST_KONTEXT-postsize+PRE_KONTEXT)
        { presize = POST_KONTEXT-postsize+PRE_KONTEXT;  leader =TRUE; }
    }
    else
    { presize  = PRE_KONTEXT;   leader =TRUE;
      postsize = POST_KONTEXT;  trailer=TRUE;
    }
   // write the kontext:
    tptr p = CI->cdp->comp_kontext;
    if (leader) { strcpy(p, "...");  p+=3;  presize-=3; }
    memcpy(p, refpos-presize, presize);  p+=presize;
    *(p++)=0x7f;
    memcpy(p, refpos,        postsize);  p+=postsize;
    if (trailer) strcpy(p-3, "...");
    else *p=0;
  }
}

int WINAPI compile_masked(CIp_t CI)
{ CI->cdp->mask_compile_error++;
  int result = compile_direct(CI);
  CI->cdp->mask_compile_error--;
  return result;
}
 
int WINAPI compile(CIp_t CI)
{ int result = compile_direct(CI);
  if (result && !CI->cdp->mask_compile_error)
  { create_error_contect(CI);
    { t_exkont_comp exc(CI->cdp);
      request_compilation_error(CI->cdp, result);
    }
  }
  return result;
}

void t_exkont_comp::get_descr(cdp_t cdp, char * buf, int buflen)
{ if (buflen > 12)
  { strcpy(buf, " [Comp: ");  buf+=8;  buflen-=8;
    if (cdp->comp_kontext) strmaxcpy(buf, cdp->comp_kontext, buflen-1);
   // remove control chars from the source: Cr and LF would break the structure of the log
    uns8 * p = (uns8*)buf;
    while (*p) { if (*p<' ') *p=' ';  p++; }
    strcat(buf, "]");
  }
}

sig32 get_num_val(CIp_t CI, sig32 lbd, sig32 hbd)
{ sig32 val;
  if (CI->cursym=='=') next_sym(CI);
  if (CI->cursym!=S_INTEGER) c_error(INTEGER_EXPECTED);
  if ((CI->curr_int<lbd) || (CI->curr_int>hbd)) c_error(INT_OUT_OF_BOUND);
  val=(sig32)CI->curr_int;
  next_sym(CI);
  return val;
}

sig32 num_val(CIp_t CI, sig32 lbd, sig32 hbd)
{ sig32 val=0;  BOOL minus;  
  int oper = '+';
  do
  { if (CI->cursym=='-')
    { minus=TRUE;  next_sym(CI); }
    else
    { if (CI->cursym=='+') next_sym(CI);
      minus=FALSE;
    }
    if (CI->cursym!=S_INTEGER)
    { if (CI->cursym==S_IDENT)
      { object * obj = search_obj(CI->curr_name);
        if (obj)  /* program-defined identifier */
        { if (obj->any.categ==OBT_CONST)
            if (obj->cons.type==ATT_INT32)
              { CI->curr_int=obj->cons.value.int32;  goto is_const; }
            if (obj->cons.type==ATT_INT16)
            { CI->curr_int = (obj->cons.value.int16==(sig16)0x8000) ? NONEINTEGER :
                obj->cons.value.int16;
              goto is_const;
            }
        }
      }
      c_error(INTEGER_EXPECTED);
    }
   is_const:
    if (minus) CI->curr_int=-CI->curr_int;
    if      (oper=='|') val |= (sig32)CI->curr_int;
    else if (oper=='&') val &= (sig32)CI->curr_int;
    else if (oper=='+') val += (sig32)CI->curr_int;
    else                val -= (sig32)CI->curr_int;
    oper=next_sym(CI);
    if ((oper != '|') && (oper != '&') && (oper != '+') && (oper != '-'))
      break;
    next_sym(CI);
  } while (TRUE);
  if (val<lbd || val>hbd) c_error(INT_OUT_OF_BOUND);
  return val;
}

/* Comments:
Compilation error logic: Compilation error is propagated by:
- creating error kontext, 
- storing line and column of the error position into cdp->comp_err_line & _column and 
- by calling request_compilation error 
when compilation is called by "compile" entry. 
The "compile_masked" entry does not set client error on compilation error.
   While "request_error" clears the previous compilation error kontext, "request_compilaton_error"
preserves it.
*/
