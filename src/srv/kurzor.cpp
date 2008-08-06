/****************************************************************************/
/* kurzor.cpp - cursor layer                                                */
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
#include "nlmtexts.h"
#include "worm.h"
#include "opstr.h"
#include "odbc.h"
#include "netapi.h"
#include "profiler.h"
#ifndef WINS
struct SYSTEMTIME;
#endif
#include "wbsecur.h"
#include "flstr.h"
#ifndef WINS
#ifdef UNIX
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <sys/mount.h>
#include <unistd.h>
#ifdef LINUX
#include <sys/vfs.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif
#endif
#endif

#include "updtrig.cpp"
#include "random.cpp"
#include <time.h>

BOOL compare_tables(cdp_t cdp, tcursnum tb1, tcursnum tb2,
                    trecnum * diffrec, int * diffattr, int * result);
BOOL find_break_line(cdp_t cdp, tobjnum objnum, uns32 & line);
void debug_command(cdp_t cdp, tptr & request);
void remove_user_rlocks(t_client_number user, t_locktable * lb);

static const uns8 mod_stop = MODSTOP, opcode_write =OP_WRITE,
                  submit_opcode = OP_SUBMIT;

bool replaying = wbfalse;
static uns32 add_lic_key[4+4+1] = { 0, 0, 0, 0 };  // temp. random key for adding licenses

tptr ker_load_blob(cdp_t cdp, table_descr * tbdf, trecnum recnum, tattrib attr, uns32 * size)
{ uns32 len;  tptr buf;
  if (tb_read_atr_len(cdp, tbdf, recnum, attr, &len)) return NULL;
  buf=(tptr)corealloc(len+1, 94);
  if (!buf) { request_error(cdp, OUT_OF_KERNEL_MEMORY);  return NULL; }
  if (tb_read_var(cdp, tbdf, recnum, attr, 0, len, buf))
    { corefree(buf);  return NULL; }
  *size=len=cdp->tb_read_var_result;
  buf[len]=0;
  return buf;
}

tptr ker_load_objdef(cdp_t cdp, table_descr * tbdf, trecnum recnum)
{ uns32 len;  tptr buf;
  buf = ker_load_blob(cdp, tbdf, recnum, OBJ_DEF_ATR, &len);
  if (!buf) return NULL;
 // decrypting:
  if (tbdf->tbnum==TAB_TABLENUM || tbdf->tbnum==OBJ_TABLENUM)  // allways true, I think
  { uns16 flags;
    tb_read_atr(cdp, tbdf, recnum, OBJ_FLAGS_ATR, (tptr)&flags);
    if (flags & CO_FLAG_ENCRYPTED) enc_buffer_total(cdp, buf, len, (tobjnum)recnum);
  }
  return buf;
}

tptr ker_load_objdef2(cdp_t cdp, table_descr * tbdf, trecnum recnum)
{ uns32 len;  tptr buf;
  buf = ker_load_blob(cdp, tbdf, recnum, OBJ_DEF_ATR, &len);
  if (!buf) return NULL;
 // decrypting:
  if (tbdf->tbnum==TAB_TABLENUM || tbdf->tbnum==OBJ_TABLENUM)  // allways true, I think
  { uns16 flags;
    tb_read_atr(cdp, tbdf, recnum, OBJ_FLAGS_ATR, (tptr)&flags);
    if (flags & CO_FLAG_ENCRYPTED) 
    { char * copy = (char*)corealloc(len+1, 94);
      memcpy(copy, buf, len+1);
      enc_buffer_total(cdp, buf, len, (tobjnum)recnum);
      if (!(flags & CO_FLAG_LINK))
      { tobjnum objnum = recnum % (BLOCKSIZE / sizeof(ttrec));
        while (memcmp(buf, "CREATE TABLE", 12) && objnum < tables[tbdf->tbnum]->Recnum())
        { memcpy(buf, copy, len+1);
          enc_buffer_total(cdp, buf, len, objnum);
          objnum += BLOCKSIZE / sizeof(ttrec);
        }
      }
      corefree(copy);
    }
  }
  return buf;
}

tptr ker_load_object(cdp_t cdp, trecnum recnum)
{ return ker_load_objdef(cdp, objtab_descr, recnum); }

void ker_free(void * ptr)
{ corefree(ptr); }

void * ker_alloc(int size)
{ return corealloc(size, 58); }

void reduce_schema_name(cdp_t cdp, char * schema)
// Removes schema name if equal to the logged user's name
{ if (!(cdp->sqloptions & SQLOPT_USER_AS_SCHEMA))
    if (!stricmp(schema, cdp->prvs.luser_name()))
      *schema=0;
}

void random(uns8 * dest, unsigned size)
{ }

static BOOL register_dynpars(cdp_t cdp, tptr request)
// Registers dynamic parameter markers in the statement
{ if (cdp->sel_stmt==NULL) return FALSE;
  tptr p=request;
  cdp->sel_stmt->markers.marker_dynar::~marker_dynar();
  cdp->sel_stmt->disp=0;  // no displacement
  BOOL in_comm=FALSE, in_string=FALSE;
  while (*p)
  { if ((*p=='\'' || *p=='\"') && !in_comm) in_string=!in_string;
    else if (*p=='{' && !in_string) 
    { while (p[1]==' ') p++;
      if (p[1]!='?' && memicmp(p+1, "CALL", 4))
        in_comm=TRUE;
    }
    else if (*p=='}' && !in_string) in_comm=FALSE;
    else if (*p=='?' && !in_string && !in_comm)
    { MARKER * marker = cdp->sel_stmt->markers.next();
      if (marker==NULL) { request_error(cdp, OUT_OF_KERNEL_MEMORY);  return TRUE; }
      marker->position=(int)(p-request);
      //marker->type=0; -- implicit, input_output is 0 too
    }
    p++;
  }
  return FALSE;
}

void report_slow_statement(cdp_t cdp, const char * stmt, unsigned netto_exec_time)
{ t_flstr s;
  s.put("Exec time");
  s.putint(netto_exec_time/10);
  s.putc('.');
  s.putc((char)('0'+(netto_exec_time%10)));
  s.put("s: ");
  s.put(stmt);
  trace_msg_general(cdp, TRACE_USER_WARNING, s.get(), NOOBJECT);
}

/***************************** journal sevices ******************************/
/* jourbuf is used both for transaction journal & external update journal */
// Journal access protected by commit critical section
uns32 global_journal_pos = 0;

tptr jourbuf=NULL;

BOOL jour_init(void)
{ if (!jourbuf) jourbuf=(tptr)corealloc(BLOCKSIZE, 98);
  return jourbuf==NULL;
}
void jour_close(void)
{ corefree(jourbuf);  jourbuf=NULL; }

static unsigned jour_pos;

static void jour_write(cdp_t cdp, void * object, uns32 size)
// Called inside [commit_sem].
// Output to journal file, buffered in jourbuf.
{ DWORD wr;  
  if (!FILE_HANDLE_VALID(ffa.j_handle)) return;
  while (size)
  { unsigned tomove=BLOCKSIZE-jour_pos;
    if (size<tomove) tomove=size;
    memcpy(jourbuf+jour_pos, object, tomove);
    jour_pos += tomove;
    size -= tomove;
    object = (tptr)object + tomove;
    if (jour_pos==BLOCKSIZE)
    { SetFilePointer(ffa.j_handle, global_journal_pos, NULL, FILE_BEGIN);
      WriteFile(ffa.j_handle, jourbuf, BLOCKSIZE, &wr, NULL);
      global_journal_pos+=BLOCKSIZE;
      jour_pos=0;
    }
  }
}

static void jour_copy(cdp_t cdp)
// Called inside [commit_sem].
{ if (!FILE_HANDLE_VALID(ffa.j_handle)) return;
  worm_reset(cdp, &cdp->jourworm);
  while (!worm_end(&cdp->jourworm))
  { jour_write(cdp, cdp->jourworm.curr_coreptr->items,
                    cdp->jourworm.curr_coreptr->bytesused);
    if (worm_step(cdp, &cdp->jourworm, FALSE)) break;  /* error */
  }
}

static void jour_flush(cdp_t cdp)
// Called inside [commit_sem].
{ DWORD wr;  
  if (!FILE_HANDLE_VALID(ffa.j_handle)) return;
  if (jour_pos)
  { SetFilePointer(ffa.j_handle, global_journal_pos, NULL, FILE_BEGIN);
    WriteFile(ffa.j_handle, jourbuf, jour_pos, &wr, NULL);
    global_journal_pos+=jour_pos;
    jour_pos=0;
  }
}

static uns32 journal_start_time(cdp_t cdp)
// Returns time of the first record in the journal (position 4, 4 bytes)
{ uns32 starttime;  DWORD rd;  
  if (!FILE_HANDLE_VALID(ffa.j_handle)) return 0;
  ProtectedByCriticalSection cs(&commit_sem, cdp, WAIT_CS_COMMIT);
  SetFilePointer(ffa.j_handle, 4, NULL, FILE_BEGIN);
  ReadFile(ffa.j_handle, &starttime, 4, &rd, NULL);
  return rd==4 ? starttime : 0;
}

/*************************** users management *******************************/
int task_switch_enabled = 1;

static void local_Enable_task_switch(BOOL enable)
/* Works locally, does not deliver anything to the remote server */
{ if (enable) task_switch_enabled++;
  else task_switch_enabled--;
}

tobjnum find2_object(cdp_t cdp, tcateg categ, const uns8 * appl_id, const char * objname)
// Searches object of given [categ]ory in system tables.
// [objname] is either object name or byte 1 and object uuid (possible for CATEG_USER, GROUP, SERVER, KEY, APPL)
// Objects which can belong to a schema are searched in schema [appl_id] only.
// When [appl_id] is NULL then the current schema is used }either front- or back-end).
{ trecnum recnum;
  tcateg cat = categ & CATEG_MASK;
  switch (cat)
  { case CATEG_SERVER:  // exhaustive search, number of server is small
    { tobjname name;  WBUUID uuid;
      table_descr_auto tbdf(cdp, SRV_TABLENUM);
      if (tbdf->me()!=NULL)
      { for (recnum=0;  recnum<tbdf->Recnum();  recnum++)
          if (!table_record_state(cdp, tbdf->me(), recnum))
            if (*objname==1)
            { if (!tb_read_atr(cdp, tbdf->me(), recnum, SRV_ATR_UUID, (tptr)uuid))
                if (!memcmp(uuid, objname+1, UUID_SIZE))
                  return (tobjnum)recnum;
            }
            else
            { if (!tb_read_atr(cdp, tbdf->me(), recnum, SRV_ATR_NAME, name))
                if (!sys_stricmp(name, objname))
                  return (tobjnum)recnum;
            }
      }
      break;
    }
    case CATEG_USER: case CATEG_GROUP:
    { if (*objname==1)
      { uuid_key key;
        dd_index * indx = &usertab_descr->indxs[1];
        memcpy(key.uuid, objname+1, UUID_SIZE);  key.recnum=0;
        cdp->index_owner = usertab_descr->tbnum;
        if (find_key(cdp, indx->root, indx, (tptr)&key, &recnum, true))
          return (tobjnum)recnum;
      }
      else
      { usertab_key key;
        dd_index * indx = &usertab_descr->indxs[2];
        memcpy(key.objname, objname, OBJ_NAME_LEN);  // not null-delimited!
        key.categ=cat;  key.recnum=0;
        cdp->index_owner = usertab_descr->tbnum;
        if (find_key(cdp, indx->root, indx, (tptr)&key, &recnum, true))
          return (tobjnum)recnum;
      }
      break;
    }
    case CATEG_KEY:
    { table_descr_auto keytab_descr(cdp, KEY_TABLENUM);
      if (keytab_descr->me())
      { if (*objname==1)
        { uuid_key key;
          dd_index * indx = &keytab_descr->indxs[0];
          memcpy(key.uuid, objname+1, UUID_SIZE);  key.recnum=0;
          cdp->index_owner = keytab_descr->tbnum;
          if (find_key(cdp, indx->root, indx, (tptr)&key, &recnum, true))
            return (tobjnum)recnum; 
        }
      }
      break;
    }
    case CATEG_APPL:
      if (*objname==1)
      { if (!ker_apl_id2name(cdp, (uns8*)(objname+1), NULL, &recnum)) 
          return (tobjnum)recnum;
      }
      else  // must search independently of uuid
      { tobjnum objnum;
        if (!ker_apl_name2id(cdp, objname, NULL, &objnum)) 
          return objnum;
      }
      break;
    case CATEG_TABLE: 
     // system tables cannot be found in the index due to undefined appl_id:
      if (!stricmp(objname, "OBJTAB" ))   return OBJ_TABLENUM;
      if (!stricmp(objname, "TABTAB" ))   return TAB_TABLENUM;
      if (!stricmp(objname, "USERTAB"))   return USER_TABLENUM;
      if (!stricmp(objname, "SRVTAB" ))   return SRV_TABLENUM;
      if (!stricmp(objname, "REPLTAB"))   return REPL_TABLENUM;
      if (!stricmp(objname, "KEYTAB"))    return KEY_TABLENUM;
      if (!stricmp(objname, "__PROPTAB")) return PROP_TABLENUM;
      if (!stricmp(objname, "__RELTAB"))  return REL_TABLENUM;
      //cont.
    default: // search in TABTAB or OBJTAB
    { ttablenum tb;  table_descr * tbdf;
      if (cat==CATEG_TABLE)
      { tb=TAB_TABLENUM;  tbdf=tabtab_descr;
        if (appl_id==NULL) appl_id = cdp->current_schema_uuid;
      }
      else 
      { tb=OBJ_TABLENUM;  tbdf=objtab_descr;
        if (appl_id==NULL) 
          appl_id = IS_BACK_END_CATEGORY(cat) || cat==CATEG_ROLE ?
          // role: must use current_schema_uuid, used in set_membership!
            cdp->current_schema_uuid : cdp->front_end_uuid;
      }
      if (!tbdf) break;  // possible when creating system tables
     // preparing the key for the index search:
      dd_index * indx = &tbdf->indxs[1];
      cdp->index_owner = tbdf->tbnum;
      objtab_key key, saved_key;
      memcpy(key.objname, objname, OBJ_NAME_LEN);  // not null-delimited!
      key.categ=categ;
      memcpy(key.appl_id, appl_id, UUID_SIZE);
      do  // follow the possible links
      { key.recnum=0;
        saved_key=key;
        if (find_key(cdp, indx->root, indx, (tptr)&key, &recnum, true)) goto found;
        key=saved_key;
        key.categ ^= IS_LINK;
        if (find_key(cdp, indx->root, indx, (tptr)&key, &recnum, true)) goto found;
        return NOOBJECT;
       found:
        if (!(key.categ & IS_LINK) || (categ & IS_LINK))
          return (tobjnum)recnum;
       // go via link:
        t_wb_link link_buf;
        if (tb_read_var(cdp, tbdf, recnum, OBJ_DEF_ATR, 0, sizeof(t_wb_link), (tptr)&link_buf) ||
            cdp->tb_read_var_result != sizeof(t_wb_link)) return NOOBJECT;
        if (link_buf.link_type!=LINKTYPE_WB) return NOOBJECT;
        strcpy(key.objname, link_buf.destobjname);
        memcpy(key.appl_id, link_buf.dest_uuid, UUID_SIZE);
        key.categ=saved_key.categ;
      } while (TRUE);
      break;
    } // default: search in TABTAB or OBJTAB
  } // switch
 // common "not found" exit:
  return NOOBJECT;
}

BOOL convert_from_string(cdp_t cdp, int tp, char * txt, void * dest, t_specif specif)
// dest may be same as txt!
{ switch (tp)  /* conversion according to the result type */
  { case ATT_BOOLEAN:
      cutspaces(txt);
      if (!*txt)                       *((sig8*)dest)=NONEBOOLEAN;
      else if (!stricmp(txt, "TRUE"))  *((sig8*)dest)=1;
      else if (!stricmp(txt, "FALSE")) *((sig8*)dest)=0;
      else return FALSE;
      break;
    case ATT_CHAR:    
      *(char*)dest=*txt;  
      if (*txt && txt[1]) return FALSE;
      break;
    case ATT_INT8:    case ATT_INT16:    case ATT_INT32:   case ATT_INT64:   
    { sig64 i64val;
      if (whitespace_string(txt))
      { switch (tp)
        { case ATT_INT8:
            *(sig8*)dest=NONETINY;  break;
          case ATT_INT16:
            *(sig16*)dest=NONESHORT;  break;
          case ATT_INT32:
            *(sig32*)dest=NONEINTEGER;  break;
          case ATT_INT64:
            *(sig64*)dest=NONEBIGINT;  break;
        }
      }
      else
      { if (!str2numeric(txt, &i64val, specif.scale)) return FALSE;
        switch (tp)
        { case ATT_INT8:
            if (i64val>127 || i64val<-127) return FALSE;  *(sig8*)dest=(sig8)i64val;  break;
          case ATT_INT16:
            if (i64val>32767 || i64val<-32767) return FALSE;  *(sig16*)dest=(sig16)i64val;  break;
          case ATT_INT32:
            if (i64val>0x7fffffff || i64val<-0x7fffffff) return FALSE;  *(sig32*)dest=(sig32)i64val;  break;
          case ATT_INT64:
            *(sig64*)dest=i64val;  break;
        }
      }
      break;
    }
    case ATT_FLOAT:   return str2real     (txt, (double*)dest);
    case ATT_MONEY:   return str2money    (txt, (monstr*)dest);
    case ATT_DATE:    return str2date     (txt, (uns32 *)dest);
    { uns32 dt;  
      if (whitespace_string(txt)) dt=NONEDATE;
      else if (str2date(txt, &dt) || sql92_str2date(txt, &dt)) ;
      else if (str2timestampUTC(txt, &dt, false, cdp->tzd) || sql92_str2timestamp(txt, &dt, false, cdp->tzd)) 
        dt=timestamp2date(dt);
      else return FALSE;  // the string is inconvertible
      *(uns32*)dest = dt;
      break;
    }
    case ATT_TIME:    
    { uns32 tm; 
      if (whitespace_string(txt)) tm=NONETIME;
      else if (str2timeUTC(txt, &tm, specif.with_time_zone, cdp->tzd) || sql92_str2time(txt, &tm, specif.with_time_zone, cdp->tzd)) ;
      else if (str2timestampUTC(txt, &tm, specif.with_time_zone, cdp->tzd) || sql92_str2timestamp(txt, &tm, specif.with_time_zone, cdp->tzd))
        tm=timestamp2time(tm); 
      else return FALSE;  // the string is inconvertible
      *(uns32*)dest = tm;
      break;
    }
    case ATT_TIMESTAMP:
    case ATT_DATIM: 
    { uns32 tms;
      if (whitespace_string(txt)) tms=NONEINTEGER;
      else if (str2timestampUTC(txt, &tms, specif.with_time_zone, cdp->tzd) || sql92_str2timestamp(txt, &tms, specif.with_time_zone, cdp->tzd)) ;
      else return FALSE;  // the string is inconvertible
      *(uns32*)dest = tms;
      break;
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
    default:
      return FALSE;  /* stopped as soon as possible */
  }
  return TRUE;
}

trecnum find_record_by_primary_key(cdp_t cdp, table_descr * tbdf, char * strval)
{ trecnum recnum;
 // find primary index:
  int ind = 0;
  do 
  { if (ind>=tbdf->indx_cnt) return NORECNUM;  // no primary key defined for the table
    if (tbdf->indxs[ind].ccateg==INDEX_PRIMARY) break;
    ind++;
  } while (true);
  dd_index * indx = &tbdf->indxs[ind];
  if (indx->partnum!=1) return NORECNUM;
 // allocate the key buffer:
  char * key = (char*)corealloc(indx->keysize, 85);
  if (!key) { request_error(cdp, OUT_OF_KERNEL_MEMORY);  return NORECNUM; }
 // convert the key value:
  if (indx->parts[0].type==ATT_STRING)
    strmaxcpy(key, strval, indx->parts[0].specif.length+1);
  else if (!convert_from_string(cdp, indx->parts[0].type, strval, key, indx->parts[0].specif))
    { corefree(key);  return FALSE; }
 // search the key value:
  *(trecnum*)(key+indx->keysize-sizeof(trecnum)) = 0;
  cdp->index_owner = tbdf->tbnum;
  if (!find_key(cdp, indx->root, indx, key, &recnum, true)) 
    recnum=NORECNUM;  
  corefree(key);
  return recnum;
}

tobjnum find_object_by_id(cdp_t cdp, tcateg categ, const WBUUID uuid)
{ char buf[OBJ_NAME_LEN+1];
  *buf=1;  memcpy(buf+1, uuid, UUID_SIZE);
  return find_object(cdp, categ, buf);
}

BOOL name_find2_obj(cdp_t cdp, const char * name, const char * schemaname, tcateg categ, tobjnum * pobjnum)
// Looks for the object [schemaname].[name] of [categ] category, returns its object number.
// Returns FALSE iff found, TRUE otherwise.
{ WBUUID appl_id;
  if (schemaname==NULL || !*schemaname) // look in the current schema
    return (*pobjnum=find2_object(cdp, categ, NULL, name)) == NOOBJECT;
  if (ker_apl_name2id(cdp, schemaname, appl_id, NULL)) 
    return TRUE;
  return (*pobjnum=find2_object(cdp, categ, appl_id, name)) == NOOBJECT;
}

BOOL find_obj(cdp_t cdp, const char * name, tcateg categ, tobjnum * pobjnum)
// Looks for the object [name] of [categ] category, returns its object number.
// [name] may be prefixed with the schema name.
// Returns FALSE iff found, TRUE otherwise.
{ int i;  tobjname schema, objname;
  while (*name==' ') name++;
 // read the 1st name to [schema]:
  i=0;
  if (*name=='`')
  { do
    { name++;
      if (*name=='`') break;
      if (i>=OBJ_NAME_LEN || !*name) return TRUE;  // misformed name
      schema[i++]=*name;  
    } while (TRUE);
    name++;
    while (*name==' ') name++;
  }
  else
    while (*name && (*name!='.')) 
    { if (i>=OBJ_NAME_LEN) return TRUE;  // misformed name
      schema[i++]=*(name++); 
    }
  schema[i]=0;  
 // check the schema and object name delimiter:
  if (*name=='.')
  { name++;
    while (*name==' ') name++;
   // read the 2nd name to [objname]:
    i=0;       
    if (*name=='`')
    { do
      { name++;
        if (*name=='`') break;
        if (i>=OBJ_NAME_LEN || !*name) return TRUE;  // misformed name
        objname[i++]=*name;  
      } while (TRUE);
      name++;
    }
    else
      while (*name) 
      { if (i>=OBJ_NAME_LEN) return TRUE;  // misformed name
        objname[i++]=*(name++); 
      }                                 
    objname[i]=0;  
    reduce_schema_name(cdp, schema);
    return name_find2_obj(cdp, objname, schema, categ, pobjnum);
  }
  else // [schema] is the object name:
    return name_find2_obj(cdp, schema, NULL, categ, pobjnum);
}

tcursnum open_working_cursor(cdp_t cdp, char * query, cur_descr ** pcd)
{ tcursnum cursnum;
  if (sql_open_cursor(cdp, query, cursnum, pcd, false)) return NOCURSOR;
  unregister_cursor_creation(cdp, *pcd);
  make_complete(*pcd);
  return cursnum;
}

void close_working_cursor(cdp_t cdp, tcursnum cursnum) // cursnum may have CURS_USER flag
{ free_cursor(cdp, cursnum); }

#ifdef DUMP
void dump_addr(int entry, BYTE * addr)
{ char buf[13];  char ent[10];
  ent[0]='X';  int2str(entry, ent+1);
  for (int i=0;  i<6;  i++)
   { buf[2*i]='0'+addr[i]/16;  buf[2*i+1]='0'+addr[i]%16; }
  buf[12]=0;
//  WriteProfileString("DUMP", ent, buf);
  display_server_info(buf);
}
#endif

CryptoPP::InvertibleRSAFunction * get_own_private_key(cdp_t cdp)
{ unsigned len=header.srvkeylen;
  tptr buf = (tptr)corealloc(len, 78);
  if (!buf) return NULL;
  if (hp_read(cdp, &header.srvkey, 1, 0, len, buf))
    { corefree(buf);  return NULL; }
  CryptoPP::InvertibleRSAFunction * priv = make_privkey((const unsigned char *)buf, len);
  corefree(buf);
  return priv;
}

static int previous_login(cdp_t cdp, char *pcName)
// Searches for a logged in process with the same network identification
// returns	0	no name found
//          1	unique login name found and copied to pcName
//		      2 more login names found
{ BOOL fFound = FALSE, fFoundElse = FALSE;
  tobjname acLoginName;

  BYTE * pMyNetIdent = cdp->in_use == PT_SLAVE ? cdp->pRemAddr->GetRemIdent() : abKernelNetIdent;
  ProtectedByCriticalSection cs(&cs_client_list, cdp, WAIT_CS_CLIENT_LIST);  
  for (int i=1;  i <= max_task_index;  i++)
    if (cds[i] && cds[i]->applnum_perm!=cdp->applnum_perm && cds[i]->in_use!=PT_WORKER)
    { BYTE * pHisNetIdent = cds[i]->in_use == PT_SLAVE ? cds[i]->pRemAddr->GetRemIdent() : abKernelNetIdent;
      if (!memcmp (pHisNetIdent, pMyNetIdent, 6))
      { if (!fFound)
        { strcpy(acLoginName, cds[i]->prvs.luser_name());
          fFound = TRUE;
        }
        else if (stricmp(acLoginName, cds[i]->prvs.luser_name()))
          { fFoundElse = TRUE;  break; }
      }
    }
  if (!fFound) return 0;
  if (!fFoundElse) { strcpy(pcName, acLoginName);  return 1; }
  return 2;
}

static void login_trigger(cdp_t cdp, const char * oldname, const char * newname)
{ 
  if (find_system_procedure(cdp, "_ON_LOGIN_CHANGE"))
  { char buf[40+2*OBJ_NAME_LEN];
    sprintf(buf, "CALL _SYSEXT._ON_LOGIN_CHANGE(\'%s\',\'%s\')", *oldname ? oldname : "ANONYMOUS", *newname ? newname : "ANONYMOUS");
    BOOL was_error = cdp->is_an_error();
    exec_direct(cdp, buf);
   // remove the error, login performed: 
    if (cdp->is_an_error() && !was_error) 
    { roll_back(cdp);
      request_compilation_error(cdp, ANS_OK);
    }  
  }
}

static BOOL do_login_change(cdp_t cdp, uns32 key, const char * new_username, tobjnum new_userobj, const char * suffix, int flags)
// Called on every login/logout
// When key==0 then tries to login without licence and can use the user name of the previous login with the same key.
// When key!=0 then tries to login with the specified user name. Takes a licence if doesnot have one.
// If the client is already logged, does not change its login dependency.
// Calls the login trigger.
{ tobjname old_username;  
  unsigned total=0, used;
  if (flags & LOGIN_SIMULATED_FLAG) return TRUE;  // no action
 // check for dependent login iff key specified:
  if (key)
  { bool found = false;
    { ProtectedByCriticalSection cs(&cs_client_list, cdp, WAIT_CS_CLIENT_LIST);  
      for (int i=1;  i<=max_task_index;  i++) if (cds[i])
      { cdp_t acdp = cds[i];
        if (acdp->random_login_key==key) 
        {// use data from the parent login: 
          if (!new_username)         new_username = acdp->prvs.luser_name(); 
          if (new_userobj==NOOBJECT) new_userobj  = acdp->prvs.luser();
          if (cdp->conditional_attachment)
            cdp->dep_login_of = acdp->dep_login_of ? acdp->dep_login_of : i;
          cdp->limited_login = acdp->limited_login;
          found=true;
          break;
        }
      }
    } // end of CS
    if (!found)
    { Sleep(10);  
      int flgs=cdp->ker_flags;  cdp->ker_flags |= KFL_HIDE_ERROR;
	  if (new_username)
		SET_ERR_MSG(cdp, new_username);
	  request_error(cdp, BAD_PASSWORD);  
      cdp->ker_flags=flgs;
      return FALSE; 
    }
  }
  else if (cdp->conditional_attachment) // if does not have the licence, gets it
  { if (flags & LOGIN_LIMITED_FLAG)
      cdp->limited_login = true;
    else
    { if (!client_licences.get_access_licence(cdp))
        { request_error(cdp, NO_MORE_INTRANET_LICS);  return FALSE; }
      cdp->limited_login = false;
    }
    cdp->dep_login_of = 0;
  }
 // client is logged now:
  if (cdp->conditional_attachment)
  { if      (cdp->in_use==PT_SLAVE)  client_licences.network_clients++;
    else if (cdp->in_use==PT_KERDIR) client_licences.direct_clients++;
    cdp->conditional_attachment=false;
    cdp->random_login_key=GetTickCount();
    cdp->www_client=false;   // will be changed for _web login
  }
 // preserve old username and set the new user:
  strcpy(old_username, cdp->prvs.luser_name());
  BOOL same = new_userobj == cdp->prvs.luser();
  cdp->prvs.set_user(new_userobj, CATEG_USER, TRUE);
#ifdef NETWARE
  char thname[18];  // thread name, max 18
  strcpy(thname, "WB ");
  strmaxcpy(thname+3, new_username, sizeof(thname)-3);
  RenameThread(GetThreadID(), thname);
#endif
 // trace the login and call login trigger:
  client_licences.get_info(&total, &used);
  if (!same)
  { char msg[100*2*OBJ_NAME_LEN+5+30];
    form_message(msg, sizeof(msg), msg_login_trace, cdp->applnum_perm, old_username, new_username, suffix, used, total);
    trace_msg_general(cdp, TRACE_LOGIN, msg, NOOBJECT);
    login_trigger(cdp, old_username, new_username);
  }
#if WBVERS<900
  if (the_sp.ReportLowLicences.val())
    if (total>2)  // prevents messaging in single-user eDock with 1+1 licences
     if ((int)used>0)  // used==-1 as a result of a strange error reported by Candy
      if (total<10 ? used >= total-2 : used >= total * 8 / 10)
      { char msg[100];
        form_message(msg, sizeof(msg), msg_licences_margin, used, total);
        display_server_info(msg);
      }
#endif
  ffa.fil_consistency();
  return TRUE;
}

inline bool VersionOK(sig32 version) 
{
#if 0 //WBVERS>=950
  return version>=5+(9<<16);
#else
  return HIWORD(version)>=5;
#endif
}

#ifdef USE_KERBEROS
#include <krb5.h>
#include <sys/poll.h>
#include <fcntl.h>
/* log_by_kerberos()
 * Logov��skrz Kerbera
 * 
 * servername: jm�o serveru podle Kerbera
 * socket: oteven (a �st) socket pro komunikaci s klientem
 * name: vstupn�parametr, dostane jm�o klienta. Je teba jej pozd�i
 * uvolnit pomoc�free() nebo kerberovsk�o ekvivalentu.
 * 
 */

static bool log_by_kerberos(const char * servername, int socket, char **name)
{
  char *version="test"; /* et�ec, pou�an pro ov�en�kompatibility
				 serveru a klienta */
  static krb5_context context; /* Me bt glob�n�.. */
  static krb5_principal my_principal; /* ditto */
  static krb5_keytab keytab; /* ditto, asi */
  static bool initialised=false;
  krb5_auth_context auth_context;
  krb5_ticket *ticket;
  krb5_error_code res=-1;
  const char * cache=the_sp.kerberos_cache.val(); /* cache */

  /* Inicializace */
  if (!initialised) do {
    res=krb5_init_context(&context); if (res) return false;
    if (!*cache) res=krb5_kt_default(context, &keytab);
    else res=krb5_kt_resolve(context, cache, &keytab);
    if (0==res){
      res=krb5_parse_name(context, servername, &my_principal);
      if (0==res){
	initialised=true;
	break;
      }
      krb5_free_principal(context, my_principal);
    }
    krb5_free_context(context);
    error_message(res);
    return false;
  } while (0);
  res=krb5_auth_con_init(context, &auth_context);
  if (res) return false;

  /* Akce: */
  res=krb5_recvauth(
    context, &auth_context,
    &socket, /* komunika��kan� */
    version, 
    NULL, //my_principal,
    //NULL, /* rc_type - v dokumentaci je, v .h neni */
    0, /* flags */
    keytab, /* default keytab */
    &ticket);

  if (res==0){
    krb5_unparse_name(context,
      ticket->enc_part2->client,
      name);
    krb5_free_ticket(context, ticket);
  }
  krb5_auth_con_free(context, auth_context);
  return (res==0);
}
#endif

bool web_login(cdp_t cdp, char * username, char * pwd)
{
 // check if already logged:
  sys_Upcase(username);
  if (!strcmp(cdp->prvs.luser_name(), username))
    return true;  // already logged under this name
 // must authentificate, find the user
  tobjnum userobj=find_object(cdp, CATEG_USER, username);
  if (userobj==NOOBJECT) return false;
 // find the password table:
  ttablenum tabobj;
  if (name_find2_obj(cdp, "HTTP_PASSWORDS", "_SYSEXT", CATEG_TABLE, &tabobj))
    return false;
  table_descr_auto td(cdp, tabobj);
  if (td->me())
  { tattrib ausr, apwd;
    ausr=1;
    while (ausr<td->attrcnt && strcmp(td->attrs[ausr].name, "USERNAME"))
      ausr++;
    if (ausr==td->attrcnt || td->attrs[ausr].attrtype!=ATT_STRING || td->attrs[ausr].attrspecif.length>OBJ_NAME_LEN) 
      return false;  // column not found or it has a wrong type
    apwd=ausr+1;
    while (apwd<td->attrcnt && strcmp(td->attrs[apwd].name, "WEBPASSWORD"))
      apwd++;
    if (apwd==td->attrcnt || td->attrs[apwd].attrtype!=ATT_STRING || td->attrs[apwd].attrspecif.length>MAX_PASSWORD_LEN) 
      return false;  // column not found or it has a wrong type
   // search:
    for (trecnum rec = 0;  rec < td->Recnum();  rec++)
    { uns8 del=table_record_state(cdp, td->me(), rec);
      if (!del)
      { tobjname name;  char pass[MAX_PASSWORD_LEN+1];
        if (!tb_read_atr(cdp, td->me(), rec, ausr, name))
          if (!stricmp(username, name))
          { if (!tb_read_atr(cdp, td->me(), rec, apwd, pass))
              if (!stricmp(pwd, pass))
              { if (!do_login_change(cdp, 0, name, userobj, msg_login_web, false))
                  return false;
                return true;
              }
            break;
          }
      }
    }
  }
  return false;
}

static void save_unlocked_iter_count(cdp_t cdp, tobjnum userobj, char * passwd)
{ *(uns32*)passwd = *(uns32*)passwd & 0x7fffffff;
  tb_write_atr(cdp, usertab_descr, userobj, USER_ATR_PASSWD, passwd);
  commit(cdp);  // must commit because request_error may follow!
}

static BOOL op_login(cdp_t cdp, uns8 * * params)
{ int opcode = *((*params)++);
  int flags = opcode & (LOGIN_LIMITED_FLAG | LOGIN_SIMULATED_FLAG);
  opcode &= ~(LOGIN_LIMITED_FLAG | LOGIN_SIMULATED_FLAG);
  switch (opcode)
  { case LOGIN_PAR_HTTP:
    { char * username = (char*)(*params);
      *params+=OBJ_NAME_LEN+1;
      char * pwd = (char*)(*params);
      *params+=strlen(pwd)+1;
      if (!web_login(cdp, username, pwd))
        { SET_ERR_MSG(cdp, username);  request_error(cdp, BAD_PASSWORD);  return TRUE; }
      break;
    }
    case LOGIN_PAR_PREP: // get RFC1938 count, network access computer name
    {// find the user by its name
      t_login_info * login_info = (t_login_info*)cdp->get_ans_ptr(sizeof(t_login_info));
      login_info->just_logged=FALSE; // used when fast login disabled or prevoius login does not exist
      if (strcmp((tptr)*params, "*NETWORK"))
      { login_info->userobj=find_object(cdp, CATEG_USER, (tptr)*params);
        if (login_info->userobj==NOOBJECT)
          { SET_ERR_MSG(cdp, (tptr)*params);  request_error(cdp, OBJECT_NOT_FOUND);  return TRUE; }
       // check for a disabled account:
        if (login_info->userobj!=ANONYMOUS_USER) 
        { wbbool stopped;
          tb_read_atr(cdp, usertab_descr, login_info->userobj, USER_ATR_STOPPED, (tptr)&stopped);
          if (stopped==wbtrue) { request_error(cdp, ACCOUNT_DISABLED);  return TRUE; }
        }
       // check the HTTP access limitation:
        if (cdp->pRemAddr && cdp->pRemAddr->is_http)  // using HTTP tunelling
          if (*the_sp.http_user.val())  // limitation specified
            if (sys_stricmp(the_sp.http_user.val(), (tptr)*params))  // different username
              { request_error(cdp, ACCOUNT_DISABLED);  return TRUE; }
       // read and lock the iteration counter:
        char passwd[sizeof(uns32)+MD5_SIZE];
        int lock_attempt_counter = 0;
        do
        { BOOL tmplocked = !wait_record_lock(cdp, usertab_descr, login_info->userobj, TMPWLOCK, 100);
          if (tb_read_atr(cdp, usertab_descr, login_info->userobj, USER_ATR_PASSWD, passwd))
            return TRUE;
          login_info->count=*(uns32*)passwd;
          if (tmplocked && !(login_info->count & 0x80000000)) break;
          if (tmplocked) record_unlock(cdp, usertab_descr, login_info->userobj, TMPWLOCK);
         // is locked, wait
          if (lock_attempt_counter++ > 15) goto exiting;  // trying to login without the lock
          Sleep(300);
        } while (TRUE);
       // lock it:
        if (*(tptr)*params!='_' && !the_sp.UnlimitedPasswords.val())  // unless sysname
        { *(uns32*)passwd |= 0x80000000;
          tb_write_atr(cdp, usertab_descr, login_info->userobj, USER_ATR_PASSWD, passwd);  // already has tmp lock
        }
       exiting: ;
        login_info->count &= 0x7fffffff;
      }
      else if (!the_sp.DisableFastLogin.val())  // look for previous login from the same workstation
      { tobjname prevname;
        if (previous_login(cdp, prevname) == 1)
        { login_info->userobj=find_object(cdp, CATEG_USER, prevname);
          if (login_info->userobj==NOOBJECT)
            { SET_ERR_MSG(cdp, prevname);  request_error(cdp, OBJECT_NOT_FOUND);  return TRUE; }
          if (!do_login_change(cdp, 0, prevname, login_info->userobj, msg_login_fast, flags))
            return TRUE;
          login_info->just_logged=TRUE;
        }
      }
      *params+=OBJ_NAME_LEN+1;
      break;
    }
    case LOGIN_PAR_PASS: 
    case LOGIN_PAR_SAME_DIFF: // another login from the same task, no need to check licences, possibly own username
    { tobjnum userobj=*(tobjnum*)*params;  *params+=sizeof(tobjnum);
      char passwd[sizeof(uns32)+MD5_SIZE];  tobjname logname;  wbbool stopped;
      if (tb_read_atr(cdp, usertab_descr, userobj, USER_ATR_PASSWD,  passwd)  ||
          tb_read_atr(cdp, usertab_descr, userobj, USER_ATR_LOGNAME, logname) ||
          tb_read_atr(cdp, usertab_descr, userobj, USER_ATR_STOPPED, (tptr)&stopped))
        return TRUE;
      BOOL sysname = *logname=='_';
      if (userobj!=ANONYMOUS_USER) 
        if (stopped==wbtrue) 
          { save_unlocked_iter_count(cdp, userobj, passwd);  request_error(cdp, ACCOUNT_DISABLED);  return TRUE; }
      { uns8 digest[MD5_SIZE];
        md5_digest(*params, MD5_SIZE, digest);
        if (memcmp(digest, passwd+sizeof(uns32), MD5_SIZE))
        { save_unlocked_iter_count(cdp, userobj, passwd);  
          char param[OBJ_NAME_LEN+1+30+1];
          strcpy(param, logname);
          if (cdp->pRemAddr) 
          { strcat(param, " ");
            cdp->pRemAddr->GetAddressString(param+strlen(param));
            param[OBJ_NAME_LEN]=0;
          }
          SET_ERR_MSG(cdp, param);  
          request_error(cdp, BAD_PASSWORD);  
          return TRUE; 
        }  // heslo
        if (!sysname)
        {// check to see if password has expired:
          uns32 credate;
          if (tb_read_atr(cdp, usertab_descr, userobj, USER_ATR_PASSCREDATE, (tptr)&credate))
            return TRUE;
          if (the_sp.PasswordExpiration.val() && Today() >= credate+the_sp.PasswordExpiration.val() ||
              (*(uns32*)passwd & 0x7fffffff) == 1)
          { cdp->cond_login=userobj;
            save_unlocked_iter_count(cdp, userobj, passwd);  
            request_error(cdp, PASSWORD_EXPIRED);  return TRUE;
          }
         // save the last password & decreased count:
          if (!the_sp.UnlimitedPasswords.val())
          { if (opcode!=LOGIN_PAR_SAME_DIFF)
            { memcpy(passwd+sizeof(uns32), *params, MD5_SIZE);
              *(uns32*)passwd = (*(uns32*)passwd & 0x7fffffff) - 1;
            }
            save_unlocked_iter_count(cdp, userobj, passwd);  
          }
        }
      }
      *params+=MD5_SIZE;
     // do login:
      if (opcode==LOGIN_PAR_SAME_DIFF)
      { uns32 key = *(uns32*)*params;  *params+=sizeof(uns32);  
        if (!do_login_change(cdp, key, logname, userobj, msg_login_normal, flags))
          return TRUE;
      }
      else
        if (!do_login_change(cdp, 0,   logname, userobj, msg_login_normal, flags))
          return TRUE;
      break;
    }
    case LOGIN_PAR_LOGOUT: // logout
      if (the_sp.DisableAnonymous.val())  // must return error in order not to consume any licence
      { cdp->ker_flags |=  KFL_HIDE_ERROR;  // if ANONYMOUS account is disabled, Logout returns this error
        request_error(cdp, ACCOUNT_DISABLED);  
        cdp->ker_flags &= ~KFL_HIDE_ERROR;
        return TRUE; 
      }
      if (!do_login_change(cdp, 0, "ANONYMOUS", ANONYMOUS_USER, msg_login_logout, flags))
        return TRUE;
      break;
    case LOGIN_PAR_SAME: // another login from the same task, no need to check licences
    { uns32 key = *(uns32*)*params;  *params+=sizeof(uns32);
      if (!key) { SET_ERR_MSG(cdp, "<same>");  request_error(cdp, BAD_PASSWORD);  return TRUE; }
      if (!do_login_change(cdp, key, NULL, NOOBJECT, msg_login_process, flags)) return TRUE;
      break;
    }
    case LOGIN_PAR_WWW:
    { tobjnum userobj=find_object(cdp, CATEG_USER, "__WEB");
      if (userobj==NOOBJECT)
        { SET_ERR_MSG(cdp, "__WEB");  request_error(cdp, OBJECT_NOT_FOUND);  return TRUE; }
     // check for a disabled account:
      wbbool stopped;
      tb_read_atr(cdp, usertab_descr, userobj, USER_ATR_STOPPED, (tptr)&stopped);
      if (stopped==wbtrue) 
        { request_error(cdp, ACCOUNT_DISABLED);  return TRUE; }
      if (!do_login_change(cdp, 0, "__WEB", userobj, msg_login_web, false))
        return TRUE;
      if (!cdp->www_client)
      { cdp->www_client=true;  // has been defined as false in do_login_change
        client_licences.www_clients++;
      }
      break;
    }

#ifdef WINS
    case LOGIN_PAR_GET_COMP_NAME:
    { tptr p = cdp->get_ans_ptr(70);
      if (p) strmaxcpy(p, Impersonation.hImpersonationPipe == INVALID_HANDLE_VALUE || !*the_sp.ThrustedDomain.val() ? "" : local_computer_name, 70);
      break;
    }
    case LOGIN_PAR_IMPERSONATE_1:  // starts connecting
    { if (Impersonation.dwState!=DISCONNECTED_STATE) // other impersonation in progress, must retry later
        { request_error(cdp, INTERNAL_SIGNAL);  return TRUE; }  // must not reset the other impersonation
      if (!Impersonation.ConnectToNewClient())
        { request_error(cdp, INTERNAL_SIGNAL);  Impersonation.reset();  return TRUE; }
      Impersonation.dwState = Impersonation.fPendingIO ? CONNECTING_STATE : READING_STATE;
      break;
    } 
    case LOGIN_PAR_IMPERSONATE_STOP:
      Impersonation.reset();  break;
    case LOGIN_PAR_IMPERSONATE_2:  // waits for connecting and starts reading
    { if (Impersonation.dwState == CONNECTING_STATE)
      { WaitForSingleObject(Impersonation.hImpersonationEvent, INFINITE);  // will not wait
        DWORD bytes;
        BOOL fSuccess = GetOverlappedResult(Impersonation.hImpersonationPipe, &Impersonation.hImpersonationOverlap, &bytes, FALSE);
        if (!fSuccess)
          { request_error(cdp, INTERNAL_SIGNAL);  Impersonation.reset();  return TRUE; }
        Impersonation.dwState=READING_STATE;   
      }
      else if (Impersonation.dwState != READING_STATE)
        { request_error(cdp, INTERNAL_SIGNAL);  Impersonation.reset();  return TRUE; }
      BOOL fSuccess = ReadFile(Impersonation.hImpersonationPipe, Impersonation.buf, 1, &Impersonation.cbBytes, &Impersonation.hImpersonationOverlap);
      if (fSuccess && Impersonation.cbBytes != 0) 
      { Impersonation.fPendingIO = FALSE; 
        Impersonation.dwState = WRITING_STATE; 
      } 
      else if (GetLastError() == ERROR_IO_PENDING)  // The read operation is still pending. 
        Impersonation.fPendingIO = TRUE; 
      else
        { request_error(cdp, INTERNAL_SIGNAL);  Impersonation.reset();  return TRUE; }
      break;
    } 
    case LOGIN_PAR_IMPERSONATE_3:  // waits for input, impersonates and disconnects
    { if (Impersonation.dwState == READING_STATE)
      { WaitForSingleObject(Impersonation.hImpersonationEvent, INFINITE);  // will not wait
        DWORD bytes;
        BOOL fSuccess = GetOverlappedResult(Impersonation.hImpersonationPipe, &Impersonation.hImpersonationOverlap, &bytes, FALSE);
        if (!fSuccess)
          { request_error(cdp, INTERNAL_SIGNAL);  Impersonation.reset();  return TRUE; }
        Impersonation.dwState=WRITING_STATE;   
      }
      else if (Impersonation.dwState != WRITING_STATE)        
        { request_error(cdp, INTERNAL_SIGNAL);  Impersonation.reset();  return TRUE; }
      tptr p = cdp->get_ans_ptr(1);  *p=wbfalse;
      if (ImpersonateNamedPipeClient(Impersonation.hImpersonationPipe))
      { HANDLE TokenHandle;
        if (OpenThreadToken(GetCurrentThread(), TOKEN_READ, TRUE, &TokenHandle))
        { /*TOKEN_USER token_user*/ char token_user[100];  DWORD ret;
          if (GetTokenInformation(TokenHandle, TokenUser, token_user, sizeof(token_user), &ret))
          { tobjname UserName;  DWORD UserNameSize = sizeof(UserName);
            char DomainName[MAX_LOGIN_SERVER+1];  DWORD DomainNameSize = sizeof(DomainName);
            SID_NAME_USE use;
            if (LookupAccountSid(NULL, // LPCTSTR lpSystemName, // address of string for system name
                 ((TOKEN_USER*)token_user)->User.Sid, UserName, 
                 &UserNameSize, DomainName, &DomainNameSize, &use))
            { if (*DomainName && !stricmp(DomainName, the_sp.ThrustedDomain.val()))  // temporary version
              { sys_Upcase(UserName);
                tobjnum userobj = find_object(cdp, CATEG_USER, UserName);
                if (userobj!=NOOBJECT)
                {// check for a disabled account:
                  wbbool stopped;
                  tb_read_atr(cdp, usertab_descr, userobj, USER_ATR_STOPPED, (tptr)&stopped);
                  if (stopped==wbtrue) 
                    request_error(cdp, ACCOUNT_DISABLED);  
                 // check the HTTP access limitation:
                  else if (cdp->pRemAddr && cdp->pRemAddr->is_http && // using HTTP tunelling
                           *the_sp.http_user.val() &&  // limitation specified
                           sys_stricmp(the_sp.http_user.val(), UserName))  // different username
                    request_error(cdp, ACCOUNT_DISABLED);  
                  //else if (the_sp.required_comm_enc.val() && !cdp->commenc) // encryption required, but not started
                  //  request_error(cdp, ENCRYPTION_FAILURE);  
                  // not tested, must disable setting required_comm_enc when server does not have the certificate and the key
                  else if (do_login_change(cdp, 0, UserName, userobj, msg_login_imperson, flags))
                    *p=wbtrue; // logged
                }
                else if (the_sp.CreateDomainUsers.val())
                { WBUUID subj_uuid;  safe_create_uuid(subj_uuid);
                  userobj=new_user_or_group(cdp, UserName, CATEG_USER, subj_uuid, true);  // privileges not checked
                  if (userobj!=NORECNUM) 
                  { char password[4+1];
                    srand( (unsigned)time( NULL ) );
                    *(uns32*)password=rand();  password[4]=0;
                    set_new_password2(cdp, userobj, password, false);
                    if (do_login_change(cdp, 0, UserName, userobj, msg_login_imperson, flags))
                      *p=wbtrue; // logged
                  }
                }
              }
            }
          }
          CloseHandle(TokenHandle);
        }
        RevertToSelf();
      }
      Impersonation.reset();
      break;
    } 
#else /* neni impersonace */
    case LOGIN_PAR_GET_COMP_NAME: {
      tptr p = cdp->get_ans_ptr(70);
      if (p) strmaxcpy(p, "", 70);
      break;
    }
#endif

    case LOGIN_PAR_KERBEROS_ASK: { /* M� zkusit pou� kerbera? */
      tptr p = cdp->get_ans_ptr(50);
#ifdef USE_KERBEROS
      strncpy(p, the_sp.kerberos_name.val(), 50); /* pokud se jmenujeme, tak to asi pjde */
#else
      *p=0;
#endif
      break;
    }
    case LOGIN_PAR_KERBEROS_DO: { /* Spout� kerberovskou autentifikaci */
      tptr p = cdp->get_ans_ptr(1);  *p=wbfalse;
#ifdef USE_KERBEROS
      int sock=cdp->pRemAddr->get_sock(); /* FIXME */
      char *UserName;
      if (!log_by_kerberos(the_sp.kerberos_name.val(), sock, &UserName)) break;
      tobjnum userobj = find_object(cdp, CATEG_USER, UserName);
      if (userobj==NOOBJECT) break;
      wbbool stopped;
      tb_read_atr(cdp, usertab_descr, userobj, USER_ATR_STOPPED, (tptr)&stopped);
      if (stopped==wbtrue) 
	      request_error(cdp, ACCOUNT_DISABLED);  
     // check the HTTP access limitation:
      else if (cdp->pRemAddr->is_http &&        // using HTTP tunelling
               *the_sp.http_user.val() &&  // limitation specified
               sys_stricmp(the_sp.http_user.val(), UserName))  // different username
        request_error(cdp, ACCOUNT_DISABLED);  
      else if (do_login_change(cdp, 0, UserName, userobj, msg_login_imperson, flags))
	      *p=wbtrue; // logged
#else
	    request_error(cdp, ACCOUNT_DISABLED);  
#endif
      break;
    }

    case LOGIN_CHALLENGE:
    { uns32 prop_encryption, enc_length;
      prop_encryption = *(uns32*)*params;  *params+=sizeof(uns32);
      enc_length      = *(uns32*)*params;  *params+=sizeof(uns32);
      uns32 my_encryption = the_sp.required_comm_enc.val() ? COMM_ENC_BASIC : COMM_ENC_NONE;
      *(uns32*)cdp->get_ans_ptr(sizeof(uns32)) = prop_encryption & my_encryption;
      CryptoPP::InvertibleRSAFunction * priv = get_own_private_key(cdp);
      if (!priv) { request_error(cdp, INTERNAL_SIGNAL);  return TRUE; }
      unsigned char dec_challenge[CHALLENGE_SIZE];  unsigned dec_length;
      decrypt_string_by_private_key(priv, *params, enc_length, dec_challenge, &dec_length);
      *params+=enc_length;
      memcpy(cdp->get_ans_ptr(CHALLENGE_SIZE), dec_challenge, CHALLENGE_SIZE);
      DeletePrivKey(priv);
      break;
    }
    case LOGIN_ENC_KEY:
    { uns32 enc_length;
      enc_length = *(uns32*)*params;  *params+=sizeof(uns32);
      CryptoPP::InvertibleRSAFunction * priv = get_own_private_key(cdp);
      if (!priv) { request_error(cdp, INTERNAL_SIGNAL);  return TRUE; }
      unsigned char key[COMM_ENC_KEY_SIZE];  unsigned dec_length;
      decrypt_string_by_private_key(priv, *params, enc_length, key, &dec_length);
      *params+=enc_length;
      DeletePrivKey(priv);
      cdp->commenc = new t_commenc_pseudo(true, key);  // do not encrypt this answer!
      break;
    }
  }
  if (!(flags & LOGIN_SIMULATED_FLAG)) WriteSlaveThreadCounter();  // must be after login because conditional_attachment is ignored
  return FALSE;
}

void logged_user_name(cdp_t cdp, tobjnum luser, char * user_name)
{ *user_name=0; // empty name on any error
  table_descr_auto usertab_descr(cdp, USER_TABLENUM);
  if (usertab_descr->me())
    tb_read_atr(cdp, usertab_descr->me(), luser, OBJ_NAME_ATR, user_name);
}


static BOOL write_password(cdp_t cdp, tobjnum userobj, uns8 * passwd)
// Stores the binary encoded password (for user [userobj]) and its creation date.
{ uns32 dt = Today();
  return tb_write_atr(cdp, usertab_descr, userobj, USER_ATR_PASSWD, passwd) ||
         tb_write_atr(cdp, usertab_descr, userobj, USER_ATR_PASSCREDATE, &dt);
}

BOOL set_new_password(cdp_t cdp, tptr username, tptr password)
// Defines the new password for the user. Called from SQL. 
{ if (strlen(password) < the_sp.MinPasswordLen.val())
    return TRUE;
  tobjnum userobj = find2_object(cdp, CATEG_USER, NULL, username);
  if (userobj==NOOBJECT) return TRUE;
 // check privils: user can change own password, security admin can change any password:
  if (cdp->prvs.luser()!=userobj)
    if (!cdp->prvs.is_secur_admin) 
      return TRUE;
  return set_new_password2(cdp, userobj, password, *username=='_');
}

BOOL set_new_password2(cdp_t cdp, tobjnum userobj, tptr password, BOOL is_sysname)
// Defines the new password for the user.
// Privils not checked here, nor the password length - used when the new user is being created
{ uns8 passwd[sizeof(uns32)+MD5_SIZE];
  unsigned cycles = is_sysname ? 1 : PASSWORD_CYCLES;
  *(uns32*)passwd=cycles;
 // compute password digest:
  uns8 * digest = passwd+sizeof(uns32);
  int len=(int)strlen(password);
  uns8 * mess = (uns8*)corealloc(UUID_SIZE+len, 47);
  if (mess==NULL) return TRUE; 
  strcpy((tptr)mess, password);  
  memcpy(mess+len, header.server_uuid, UUID_SIZE);
  md5_digest(mess, UUID_SIZE+len, digest); // one more digest than specified by [cycles]
  corefree(mess);
  while (cycles-- > 0)
    md5_digest(digest, MD5_SIZE, digest);
  return write_password(cdp, userobj, passwd);
}

tptr WBPARAM::realloc(unsigned size)  // zero size is legal 
{ tptr ptr = (tptr)corealloc(size, 48);
  if (ptr && val && len<size) memcpy(ptr, val, len);
  free();
  val=ptr;
  len=size;  
  return val;
}

void WBPARAM::free(void)  // deletes the old value and sets the NULL value
{ if (val!=NULL) corefree(val);
  val=NULL;  len=0;
}

void WBPARAM::mark_core(void)  
{ if (val!=NULL) mark(val); }

static void free_epars(wbstmt * sstmt)
// Free embedded params
{ for (int i=0;  i<sstmt->eparams.count();  i++)
    sstmt->eparams.acc0(i)->free();
  sstmt->eparams.emparam_dynar::~emparam_dynar();
}

t_emparam * wbstmt::find_eparam_by_name(const char * name) const
{ for (int par=0;  par<eparams.count();  par++)
  { t_emparam * empar=eparams.acc0(par);
    if (!stricmp(empar->name, name))
      return empar;
  }
  return NULL;
}

tptr t_emparam::realloc(unsigned size)  // zero size is legal 
{ tptr ptr = (tptr)corealloc(size+2, 48);  // wide terminator added for CLOB, string, CHAR
  if (ptr && val && length<size) memcpy(ptr, val, length+2);
  free();  // calls corefree only when !val_is_indep
  val=ptr;
  length=size;  
  val_is_indep=false;
  return val;
}

void t_emparam::free(void)  // deletes the old value and sets the NULL value
{ if (val!=NULL && !val_is_indep) corefree(val);
  val=NULL;  length=0;
}

void t_emparam::mark_core(void)  
{ if (val!=NULL && !val_is_indep) mark(val); }

static void free_dpars(wbstmt * sstmt)
// Release dyn. parametrs of the statement. Must preserve the statement itself.
{ for (int i=0;  i<sstmt->dparams.count();  i++) 
    sstmt->dparams.acc0(i)->free();
  sstmt->dparams.wbparam_dynar::~wbparam_dynar();
}

wbstmt::wbstmt(cdp_t cdp, uns32 handle) : hstmt(handle)
{ cursor_num=NOCURSOR;
  so=NULL;
  source=NULL;
  next_stmt=cdp->stmts;  cdp->stmts=this;
}

wbstmt::~wbstmt(void)
{ free_dpars(this);
  free_epars(this);
  markers.marker_dynar ::~marker_dynar();
  corefree(source);
  delete so;  // delete prepared statement:
}

void wbstmt::mark_core(void)  // marks itself, too
{ int i;
  mark(this);
  markers.mark_core();
  for (i=0;  i<dparams.count();  i++)  dparams.acc0(i)->mark_core();
  for (i=0;  i<eparams.count();  i++)  eparams.acc0(i)->mark_core();
  dparams.mark_core();  eparams.mark_core();
  if (so) so->mark_core();
  if (source) mark(source);
  if (next_stmt) next_stmt->mark_core();
}

void cd_t::drop_statement(uns32 handle)
// Remove statement kontext with the handle from the kontext list and delete it
{ wbstmt ** psstmt = &stmts;
  while (*psstmt && (*psstmt)->hstmt!=handle) psstmt=&(*psstmt)->next_stmt;
  wbstmt * stmt = *psstmt;
  if (stmt!=NULL)
  { if (sel_stmt==stmt) sel_stmt=NULL; // very important, ovewrites memory when assigning to ->disp otherwise
    *psstmt=stmt->next_stmt;  
    delete stmt;
  }
}

uns32 cd_t::get_new_stmt_handle(void)
{ uns32 handle = 1;
  for (const wbstmt * astmt = stmts;  astmt!=NULL;  astmt=astmt->next_stmt)
    if (handle <= astmt->hstmt) handle=astmt->hstmt+1;
  return handle;
}

void cd_t::free_all_stmts(void)
// Deletes all statement kontexts and sets stmts and sel_stmt to NULL
{ while (stmts != NULL)
  { wbstmt * stmt = stmts;
    stmts=stmt->next_stmt;
    delete stmt;
  }
  sel_stmt=NULL;
}


void cd_t::drop_global_scopes(void)
{ global_sscope=NULL;  global_rscope=NULL;  // non-owning selection
  while (created_global_rscopes) // list of owned global rscopes
  { t_rscope * rscope = created_global_rscopes;
    created_global_rscopes = rscope->next_rscope;
    rscope_clear(rscope);
    delete rscope;
  }
}

static bool close_owned_cursors(cdp_t cdp, bool including_persistent)
// Returns true if any owned cursor found and closed.
{ bool any_closed = false;
  for (tcursnum crnm=0;  crnm<crs.count();  crnm++)
  { cur_descr * cd;
    { ProtectedByCriticalSection cs(&crs_sem);
      cd = *crs.acc0(crnm);
    }
    if (cd!=NULL && cd->owner == cdp->applnum_perm)
      if (!cd->persistent || including_persistent)
        { free_cursor(cdp, crnm);  any_closed = true; }
  }
  return any_closed;
}

void kernel_cdp_free(cdp_t cdp)
// Unlocking the server moved to RemoveTask().
{ if (!cdp->initialized) return;
  if (kernel_is_init)  // not satisfied when called on global_init error
  { if (cdp->expl_trans!=TRANS_NO) roll_back(cdp);
    close_owned_cursors(cdp, true);
    commit(cdp);   /* must commit block freed from cursors */
    unlock_user(cdp, cdp->applnum_perm, true);
    corefree(cdp->comp_kontext);
  }
  cdp->free_all_stmts();
  worm_close(cdp, &(cdp->jourworm));
  delete cdp->dbginfo;  cdp->dbginfo=NULL;
  delete cdp->seq_list;  cdp->seq_list=NULL;
  if (cdp->exkont_info) { corefree(cdp->exkont_info);  cdp->exkont_info=NULL; }
  cdp->tl.destruct();
 // destruct subtransactions (list should be empty, but to be sure):
  while (cdp->subtrans)
  { t_translog * subtl = cdp->subtrans;
    cdp->subtrans = subtl->next;
    subtl->destroying_subtrans(cdp);
    if (subtl->tl_type==TL_LOCTAB) delete subtl;
  }
 // delete global scopes:
  cdp->drop_global_scopes();
  cdp->initialized=wbfalse;
  if (cdp->commenc) { delete cdp->commenc;  cdp->commenc=NULL; }
  cdp->cevs.close();
  if (cdp->current_generic_error_message) 
    { corefree(cdp->current_generic_error_message);  cdp->current_generic_error_message=NULL; }
  while (cdp->stk)
    cdp->stk_drop();
}

BOOL kernel_cdp_init(cdp_t cdp)  /* returns TRUE on "no memory" error */
// Must be called after AddTask!
{ cdp->initialized=wbfalse;
  cdp->expl_trans = TRANS_NO;  /* these vars are affected only by trans. */
  cdp->ker_flags  = 0;
 // part used by kernel working threads only (when calling request_error etc.)
  cdp->request_init();
 // other init.
  if (worm_init(cdp, &cdp->jourworm,120,3)) return TRUE;  /* no memory */
  cdp->prvs.set_user(cdp->in_use==PT_SERVER ? DB_ADMIN_GROUP : ANONYMOUS_USER, CATEG_USER, TRUE);
  cdp->admin_role=cdp->senior_role=cdp->junior_role=NOOBJECT;  // PT_SERVER uses this
  cdp->wait_type=WAIT_NO;
  cdp->trans_rw=TRUE;
  if (cdp->in_use!=PT_SERVER)
  { table_descr_auto srv_descr(cdp, SRV_TABLENUM);
    if (srv_descr->me())
      tb_read_atr(cdp, srv_descr->me(), 0, SRV_ATR_UUID, (tptr)cdp->luo);
  }
  cdp->cevs.preinit();
  cdp->initialized=wbtrue;
  return FALSE;
}

/***************************** object list **********************************/
static void list_objects(cdp_t cdp, tcateg category, const uns8 * appl_id)
{ ttablenum tb;  table_descr * tbdf;  tptr p;  BOOL no_flags=FALSE;
  trecnum rec=0;
  BOOL extended = (category & IS_LINK)!=0;  category &= CATEG_MASK;
  uns8 * own_uuid = category==CATEG_TABLE || category==CATEG_CURSOR || category==CATEG_TRIGGER || category==CATEG_PROC || category==CATEG_REPLREL ?
                    cdp->sel_appl_uuid : cdp->front_end_uuid;
  BOOL own_appl = !memcmp(appl_id, own_uuid, UUID_SIZE);
  if      (category==CATEG_TABLE)
     { tb=TAB_TABLENUM;   tbdf=tabtab_descr;  rec=2;  }
  else if (category==CATEG_USER || category==CATEG_GROUP)
     { tb=USER_TABLENUM;  tbdf=install_table(cdp, tb);  no_flags=TRUE;  extended=FALSE; }
  else if (category==CATEG_SERVER)
     { tb=SRV_TABLENUM;   tbdf=install_table(cdp, tb);  no_flags=TRUE;  extended=FALSE;  rec=1; }
  else 
     { tb=OBJ_TABLENUM;   tbdf=objtab_descr; }

  BOOL any_appl = TRUE;
  for (int i=0;  i<UUID_SIZE;  i++) if (appl_id[i]) { any_appl=FALSE;  break; }
  if (category==CATEG_USER || category==CATEG_GROUP || category==CATEG_APPL)
    any_appl = TRUE;

  tobjnum resobjnum;  uns16 resflags;  const char * pname;
  cdp->answer.sized_answer_start();
  { t_table_record_rdaccess rda(cdp, tbdf);
    while (rec < tbdf->Recnum())
    { const ttrec * dt = (const ttrec*)rda.position_on_record(rec, 0);
      if (dt!=NULL)
      { if (!dt->del_mark)
        { if (category==CATEG_SERVER)
            { pname=(tptr)dt+1;  resobjnum=(tobjnum)rec;  resflags=0; }
          else if ((dt->categ & CATEG_MASK)==category &&
                   (any_appl || !memcmp(dt->apluuid, appl_id, UUID_SIZE)) &&
                   (!(dt->categ & IS_LINK) || own_appl || any_appl))
          { pname=dt->name;  resobjnum=(tobjnum)rec;
            resflags=no_flags ? 0 : dt->flags;
           // objnum & flags: if link, copy from linked object
            if (dt->categ & IS_LINK)
            { t_wb_link link_buf;
              if (category==CATEG_APPL) goto nextrec;  // impossible, used only in crashed databases
              if (tb_read_var(cdp, tbdf, rec, OBJ_DEF_ATR, 0, sizeof(t_wb_link), (tptr)&link_buf) ||
                  cdp->tb_read_var_result != sizeof(t_wb_link))
                goto nextrec;
              if (link_buf.link_type==LINKTYPE_WB)
              { resobjnum=find2_object(cdp, category, link_buf.dest_uuid, link_buf.destobjname); // find the linked object
                if (resobjnum==NOOBJECT) goto nextrec;
                uns16 flags;  tb_read_atr(cdp, tb, resobjnum, OBJ_FLAGS_ATR, (tptr)&flags); // flags og the linked object
                resflags=flags | CO_FLAG_LINK;
              }
              else if (link_buf.link_type==LINKTYPE_ODBC)
                resflags |= CO_FLAG_ODBC;
            }
          }
          else goto nextrec;
         // write the object to the result:
          if (*pname)  /* must not return empty names! */
          { int len=strmaxlen(pname, OBJ_NAME_LEN);  // length of the object name
            if (extended) // return folder_name and last_modif too
            { int fldlen=strmaxlen(dt->folder_name, OBJ_NAME_LEN);
              p=cdp->get_ans_ptr(len+1+fldlen+1+sizeof(uns32)+sizeof(tobjnum)+sizeof(uns16));
              if (p==NULL) break;
              memcpy(p, pname, len);               p+=len;     *(p++)=0;
              memcpy(p, dt->folder_name, fldlen);  p+=fldlen;  *(p++)=0;
              *(uns32*)p=dt->last_modif;           p+=sizeof(uns32);
            }
            else          // returning only object name, object number and flags
            { p=cdp->get_ans_ptr(len+1+sizeof(tobjnum)+sizeof(uns16));
              if (p==NULL) break;
              memcpy(p, pname, len);               p+=len;     *(p++)=0;
            }
            *(tobjnum*)p=resobjnum;                p+=sizeof(tobjnum);
            *(uns16  *)p=resflags;
          }
        }
      }
      else if (cdp->get_return_code()==RECORD_DOES_NOT_EXIST)   /* possible after using berle, must mask it */
        { request_error(cdp, ANS_OK);  break; }
     nextrec:
      rec++;
    }
  } // unfix record access
  if (category==CATEG_USER || category==CATEG_GROUP || category==CATEG_SERVER)
    unlock_tabdescr(tbdf);
  p=cdp->get_ans_ptr(1);  if (p!=NULL) *p=0;   /* terminator */
  cdp->answer.sized_answer_stop();
}

void ker_set_application(cdp_t cdp, const char * appl_name, tobjnum & aplobj)  // appl_name is case-OK
// Defines long-term schema context.
// Sets sel_appl_name, sel_appl_uuid, front_end_uuid and role object numbers.
// No change in application kontext if appl_name does not exist.
// If back-end or front-end name is not found then it is ignored.
{ aplobj=NOOBJECT;  WBUUID uuid;
  cdp->prvs.set_temp_author(NOOBJECT);  // stops the temporary authoring mode
  if (*appl_name && ker_apl_name2id(cdp, appl_name, uuid, &aplobj))
    { SET_ERR_MSG(cdp, appl_name);  request_error(cdp, OBJECT_NOT_FOUND);  return; }
  strmaxcpy(cdp->sel_appl_name, appl_name, sizeof(cdp->sel_appl_name));
  cdp->admin_role=cdp->senior_role=cdp->junior_role=cdp->author_role=NOOBJECT;
  
  if (*appl_name)
  { memcpy(cdp->sel_appl_uuid,  uuid, UUID_SIZE);  // default
    memcpy(cdp->front_end_uuid, uuid, UUID_SIZE);  // default
    memcpy(cdp->top_appl_uuid,  uuid, UUID_SIZE);
   // application selected, can search in its (own) kontext:
    cdp->author_role=find2_object(cdp, CATEG_ROLE, cdp->top_appl_uuid, "AUTHOR");
    cdp->admin_role =find2_object(cdp, CATEG_ROLE, cdp->top_appl_uuid, "ADMINISTRATOR");
    cdp->senior_role=find2_object(cdp, CATEG_ROLE, cdp->top_appl_uuid, "SENIOR_USER");
    cdp->junior_role=find2_object(cdp, CATEG_ROLE, cdp->top_appl_uuid, "JUNIOR_USER");
   // analyse the redirection of the front/ or back-end:
    apx_header apx;  memset(&apx, 0, sizeof(apx));
    tb_read_var(cdp, objtab_descr, aplobj, OBJ_DEF_ATR, 0, sizeof(apx_header), (tptr)&apx);
    if (*apx.back_end_name)
    { tobjnum aobj=find_object(cdp, CATEG_APPL, apx.back_end_name);
      if (aobj!=NOOBJECT) 
        tb_read_atr(cdp, objtab_descr, aobj, APPL_ID_ATR, (tptr)cdp->sel_appl_uuid);
    }
    if (*apx.front_end_name)
    { tobjnum aobj=find_object(cdp, CATEG_APPL, apx.front_end_name);
      if (aobj!=NOOBJECT) 
        tb_read_atr(cdp, objtab_descr, aobj, APPL_ID_ATR, (tptr)cdp->front_end_uuid);
    }
    cdp->global_sscope=find_global_sscope(cdp, cdp->sel_appl_uuid);
  }
  else 
  { memset(cdp->sel_appl_uuid, 0, UUID_SIZE);  memset(cdp->front_end_uuid, 0, UUID_SIZE);
    memset(cdp->top_appl_uuid, 0, UUID_SIZE);
    cdp->global_sscope=NULL;  
  }
  cdp->global_rscope=find_global_rscope(cdp);
}
/***************************** find non-deleted *****************************/
static void get_valid_records(cdp_t cdp, tcursnum cursnum, trecnum startrec, trecnum limit)
/* Will not check for deleted record in a post sorted table - deleted records
   (by HAVING) are not used in the post-sorting. */
/* For table supposes that tab_descr is locked. */
{ trecnum totrec, valrec, * prec;  uns16 * map;  unsigned mapsize;
  trecnum mappos;  cur_descr * cd;  BOOL is_table;
  mapsize=(unsigned)(((limit-1) / 16 + 1) * 2 + 2);
  /* +2 due to 16-bit alignment (map need not start at the mapspace beginning) */
  prec=(trecnum*)cdp->get_ans_ptr(2*sizeof(trecnum) + mapsize);
  if (!prec) return;
  map=(uns16*)(prec+2);
  memset(map, 0, mapsize);
  totrec=valrec=0;  mappos=startrec-(startrec/16)*16;

  if (IS_CURSOR_NUM(cursnum))
  { { ProtectedByCriticalSection cs(&crs_sem, cdp, WAIT_CS_CURSORS);
      cd = *crs.acc(GET_CURSOR_NUM(cursnum));
    }
    if (PTRS_CURSOR(cd))           is_table=FALSE;
    else { cursnum=cd->own_table;  is_table=TRUE; }
  }
  else is_table=TRUE;
  if (!is_table)
  { t_mater_ptrs * pmat = cd->pmat;
    while (limit--)
    { while (startrec >= cd->recnum)
        if (cd->complete) goto wri_ans;
        else cd->enlarge_cursor(cd->recnum+256);
      if (pmat->cursor_record_state(cdp, startrec)==NOT_DELETED)
      { map[mappos/16] |= 1 << (mappos % 16);
        valrec++;
      }
      totrec++;  startrec++;  mappos++;
      report_step(cdp, startrec);
      if (cdp->break_request) { request_error(cdp, REQUEST_BREAKED);  break; }
    }
  }
  else   /* searching in a table */
  { uns8 dltd;  table_descr * tbdf = tables[cursnum];
    if (tbdf->Recnum() < startrec+limit)
    { if (tbdf->Recnum() <= startrec) goto wri_ans;
      limit = tbdf->Recnum()-startrec;
    }
    while (limit--)
    { dltd=table_record_state(cdp, tbdf, startrec);
      if (!dltd)
      { map[mappos/16] |= 1 << (mappos % 16);
        valrec++;
      }
      totrec++;  startrec++;  mappos++;
      report_step(cdp, startrec);
      if (cdp->break_request) { request_error(cdp, REQUEST_BREAKED);  break; }
    }
  }

 wri_ans:
  prec[0]=totrec;  prec[1]=valrec;   /* resulting counts */
}
/***************************** request entry ********************************/

BOOL ptrsteps(cdp_t cdp, table_descr ** ptbdf, ttablenum * ptbnum, trecnum * precnum, tattrib * pattr,
                     char ** prequest)
/* Supposed (**prequest==MODPTR || **prequest==MODINDPTR) ! */
// *ptbdf locked on entry and on exit, but the table changes!
{ attribdef * att;  uns16 limit, index;  uns8 modif;  const char * dt;  tfcbnum fcbn;
  modif=**prequest;  
  do
  { (*prequest)++;
    att=(*ptbdf)->attrs+*pattr;
    if (att->attrtype!=ATT_PTR && att->attrtype!=ATT_BIPTR)
      { request_error(cdp, BAD_MODIF);  return TRUE; }
   // read the pointer value, changed *precnum:
    if (modif==MODPTR)
    { if (att->attrmult!=1)
        { request_error(cdp, BAD_MODIF);  return TRUE; }
      if (fast_table_read(cdp, *ptbdf, *precnum, *pattr, precnum)) return TRUE;
    }
    else /* modif==MODINDPTR */
    { if (att->attrmult<=1)
        { request_error(cdp, BAD_MODIF);  return TRUE; }
      if (tb_read_ind_num(cdp, *ptbdf, *precnum, *pattr, &limit)) return TRUE;
      index=*(t_mult_size*)(*prequest);  (*prequest)+=sizeof(t_mult_size);
      if (index>=limit)
        { request_error(cdp, INDEX_OUT_OF_RANGE);  return TRUE; }
      if (!(dt=fix_attr_ind_read(cdp, *ptbdf, *precnum, *pattr, index, &fcbn))) return TRUE;
      *precnum=*(const trecnum*)dt;
      UNFIX_PAGE(cdp, fcbn);
    }
   /* recnum changed, *prequest points to attribute */
    if (*precnum==NORECNUM) { request_error(cdp, NIL_PTR);  return TRUE; }
    if (att->attrspecif.destination_table==NOT_LINKED)
    { link_table_in(cdp, *ptbdf, *ptbnum);
      if (att->attrspecif.destination_table==NOT_LINKED) // destination table not found
        { request_error(cdp, NIL_PTR);  return TRUE; }
    }
    table_descr * new_tbdf = install_table(cdp, att->attrspecif.destination_table);
    if (!new_tbdf) return TRUE;
    unlock_tabdescr(*ptbdf);  *ptbdf=new_tbdf;
    *ptbnum=att->attrspecif.destination_table;  /* destination table */
    *pattr=*(tattrib*)*prequest;  (*prequest)+=sizeof(tattrib);
    if (table_record_state(cdp, *ptbdf, *precnum))
      { request_error(cdp, PTR_TO_DELETED);  return TRUE; }
    modif=**prequest;  /* next step modifier */
  } while (modif==MODPTR || modif==MODINDPTR);
  return FALSE;
}

static void tra_worms_free(cdp_t cdp)
// Clears the transaction-related data, called after commit & rollback.
{ if (cdp->initialized)
  { worm_free(cdp, &(cdp->jourworm));
    cdp->savepoints.destroy(cdp, 0, TRUE);  // destroys all savepoints
    cdp->expl_trans=TRANS_NO;
  }
}

BOOL map_cur2tab(cdp_t cdp, tcursnum * cursnum, trecnum * position,
                 tattrib * attr, elem_descr * & attracc)
/* called when cursnum & CURS_USER, return TRUE on error */
{ cur_descr * cd = get_cursor(cdp, *cursnum);
  if (!cd) return TRUE; 
  if (cd->curs_eseek(*position)) return TRUE;
  if (!attr_access(&cd->qe->kont, *attr, *cursnum, *attr, *position, attracc))
    { request_error(cdp, BAD_ELEM_NUM);   return TRUE; }
  return FALSE;
}

static BOOL is_apl(cdp_t cdp, ttablenum tb, trecnum recnum)
/* Additional privileges: read, write, delete EXX, read or write APL info. */
{ tcateg cat;
  if (fast_table_read(cdp, objtab_descr, recnum, OBJ_CATEG_ATR, &cat)) return FALSE;
  return cat==CATEG_APPL;
}

BOOL cursor_locking(cdp_t cdp, int opc, cur_descr * cd, trecnum recnum)
// Returns TRUE if locking nonexisting or deleted curcor record or on error
// or if cannot lock.
{ if (cd->insensitive) return FALSE; // no action
 // find position in tables:
  const trecnum * recs;
  if (recnum==NORECNUM) recs=NULL;     // causes locking all records in tables
  else if (recnum >= cd->recnum)
    { request_error(cdp, RECORD_DOES_NOT_EXIST);  return TRUE; }
  else
  { recs = cd->pmat->recnums_pos_const(cdp, recnum);
    if (recs==NULL) return TRUE; // recnum checked, must be reported frame error
    if (*recs==NORECNUM) // deleted cursor record
      { request_error(cdp, IS_DELETED);  return TRUE; }
  }
 // lock or unlock it:
  int limit=1000;
  if (cd->qe->locking(cdp, recs, opc, limit))
  {// unlock locked tables, up to limit:
    if (opc!=OP_READ && opc!=OP_WRITE)
      cd->qe->locking(cdp, recs, opc==OP_RLOCK ? OP_UNRLOCK : OP_UNWLOCK, limit);
    return TRUE;
  }
  return FALSE;
}

#if WBVERS>=1100
#include <wcs.h>
void request_wcs_error(cdp_t cdp, int errnum, void * licence_handle)
{ 
  const char * msg = WCGetLastErrorA(errnum, licence_handle, "en-US");
  request_generic_error(cdp, GENERR_WCS, msg);
}
#endif
///////////////////////////// answer management //////////////////////////////
tptr t_answer::get_ans_ptr(unsigned size)
{ unsigned needs;  answer_cb * p;
  needs=anssize+size;  // currents answer size + additional size
  if (needs > ansbufsize) /* bigger buffer needed */
  { if (size<=10) needs+=40;
    if (!dyn_alloc)  // big buffer not allocated yet
    { p=(answer_cb*)corealloc(needs, OW_ANSW);
      if (!p) return NULL;
      memcpy(p, &min_answer, anssize);
      dyn_alloc=TRUE;
    }
    else
    { p=(answer_cb*)corerealloc(ans, needs);
      if (!p) return NULL;
    }
    ans=p;
    ansbufsize=needs;
  }
  tptr res=(tptr)ans+anssize;
  anssize+=size;
  return res;
}

//////////////////////////// kernel_entry //////////////////////////////////////
static const uns8 opc_types[] = {
/* unused            */ 0,
/* OP_READ           */ 3,
/* OP_WRITE          */ 3,
/* OP_APPEND         */ 1,
/* OP_RLOCK          */ 1,
/* OP_UNRLOCK        */ 1,
/* OP_WLOCK          */ 1,
/* OP_UNWLOCK        */ 1,
/* OP_DELETE         */ 2,
/* OP_UNDEL          */ 2,
/* OP_INSERT         */ 1,
/* OP_COMMIT         */ 0,
/* OP_START_TRA      */ 0,
/* OP_ROLL_BACK      */ 0,
/* OP_OPEN_SUBCURSOR */ 1,
/* OP_RECSTATE       */ 1,
/* OP_LOGIN          */ 0,
/* OP_SIGNATURE      */ 3,
/* OP_SQL_CATALOG    */ 0,
/* OP_PRIVILS        */ 0,
/* OP_GROUPROLE      */ 0,
/* OP_REPLCONTROL    */ 0,
/* OP_WRITERECEX     */ 2,
/* OP_OPEN_CURSOR    */ 0,
/* OP_CLOSE_CURSOR   */ 1,
/* OP_LIST           */ 0,
/*                   */ 0,
/* OP_FINDOBJ        */ 0,
/* OP_CNTRL          */ 0,
/* OP_TRANSF         */ 0,
/* OP_GENER_INFO     */ 0,
/* OP_RECCNT         */ 1,
/* OP_INDEX          */ 0,
/* OP_SAVE_TABLE     */ 1,
/* OP_REST_TABLE     */ 1,
/* OP_TRANSL         */ 1,
/* OP_BASETABS       */ 1,
/* OP_TABINFO        */ 1,
/* OP_MONITOR        */ 0,
/* OP_INTEGRITY      */ 0,
/* OP_ACCCLOSE       */ 0,
/* OP_READREC        */ 2,
/* OP_WRITEREC       */ 2,
/* OP_ADDVALUE       */ 3,
/* OP_DELVALUE       */ 3,
/* OP_REMINVAL       */ 3,
/* OP_PTRSTEPS       */ 3,
/* OP_SETAPL         */ 0,
/* OP_VALRECS        */ 1,
/* OP_IDCONV         */ 0,
/* OP_SUPERNUM       */ 2,
/* OP_ADDREC         */ 1,
/* OP_TEXTSUB        */ 3,
/* OP_END            */ 0,
/* OP_BACKUP         */ 0,
/* OP_GETDBTIME      */ 0,
/* OP_REPLAY         */ 0,
/* OP_DEBUG          */ 0,
/* OP_SUBMIT         */ 0,
/* OP_SENDPAR        */ 0,
/* OP_GETERROR       */ 0,
/* OP_AGGR           */ 1,
/* OP_NEXTUSER       */ 3,
/* OP_FULLTEXT       */ 0,
/* OP_INSERTRECEX    */ 1,
/* OP_PROPERTIES     */ 0,
/* OP_APPENDRECEX    */ 1,
/* OP_INSOBJ         */ 0,
/* OP_WHO_LOCKED     */ 1
};

#include "bckup.c"

static void kernel_entry(cdp_t cdp, tptr request)
/* Special requests (LOGOUT, bad LOGIN) have own returns */
{ uns8 opc;  ttablenum tbnum;  table_descr * tbdf, * locked_tabdescr;  
  tcursnum cursnum, crnm;  cur_descr * cd;
  trecnum recnum;  tattrib attr, curselem;
  unsigned /*!!*/ std; uns8 opc_type;
  tptr p;
  elem_descr * attracc;  // defined for opc_type==3, !=NULL for calculated attrs
  t_value exprval;
  cdp->request_init();
  locked_tabdescr=NULL;
  do // cycle on requests in the packet and execute them
  { cdp->statement_counter++;  // used by triggered statements and inidividual SQL statements
    opc=*(request++);
   // if Login is incomplete or invalid, most operations are disabled:
    if (the_sp.DisableAnonymous.val() && cdp->prvs.luser()==ANONYMOUS_USER && (recnum=ACCOUNT_DISABLED) ||
        cdp->conditional_attachment && (recnum=NOT_LOGGED_IN))
      if (!(opc==OP_LOGIN || opc==OP_GENER_INFO || opc==OP_END || // OP_GENER_INFO used when determining the server version
            opc==OP_READ && (*(tcursnum*)request==SRV_TABLENUM || *(tcursnum*)request==USER_TABLENUM) ||  // used before Login
            opc==OP_FINDOBJ || 
            opc==OP_PRIVILS && (((op_privils*)request)->operation==OPER_GET || ((op_privils*)request)->operation==OPER_GETEFF) || // setting new password after expiration it requires
            opc==OP_CNTRL ||
            opc==OP_LIST && (*(tcateg*)request==CATEG_USER && !the_sp.DisableAnonymous.val() || *(tcateg*)request==CATEG_APPL) || // listing users in the login dialog
            opc==OP_PROPERTIES && *request==OPER_GET
         ))
      { if (opc==OP_LIST) cdp->ker_flags |=  KFL_HIDE_ERROR;
        request_error(cdp, recnum);  
        if (opc==OP_LIST) cdp->ker_flags &= ~KFL_HIDE_ERROR;
        goto err_goto1; 
      }
   /* decoding on request types:
      0 - nothing to do
      1 - fetch cursnum, set crnm, cd & std, check table or cursor number, install table
      2 - fetch record number, translate it for cursor, error if deleted
      3 - fetch attribute, translate it for cursor, re-set crnm, recnum
      4 - go through pointers
   */
    opc_type=opc_types[opc];
    if (opc_type)  /* take the table/cursor number, set cursnum, crnm, std */
    { 
#ifndef __ia64__   // alignment
	    cursnum=*(tcursnum*)request;  
#else
      memcpy(&cursnum, request, sizeof(tcursnum));
#endif
      request+=sizeof(tcursnum);
      std=!IS_CURSOR_NUM(cursnum);
      if (std)
      { tbdf=locked_tabdescr=install_table(cdp, tbnum=cursnum);
        if (!tbdf) goto err_goto1;  /* error inside */
        if (opc_type > 1)  /* take the record number, translate it */
        { 
#ifndef __ia64__  // alignment
	        recnum=*(trecnum*)request;
#else
	        memcpy(&recnum, request, sizeof(trecnum));
#endif
          request+=sizeof(trecnum);
          if (recnum >= tbdf->Recnum())
            { request_error(cdp, OUT_OF_TABLE);  goto err_goto; }
          if (opc_type > 2)
          { attr=*(tattrib*)request;  request+=sizeof(tattrib);
            if (attr>=tbdf->attrcnt)
              { request_error(cdp, BAD_ELEM_NUM);  goto err_goto; }
            attracc=NULL;
          }
        }
      }
      else  /* cursor: all tables already installed */
      { crnm=GET_CURSOR_NUM(cursnum);
        cd = get_cursor(cdp, crnm);
        if (!cd) goto err_goto;
       // activate embedded vars and dyn pars for the cursor:
        if (cd->hstmt) cdp->sel_stmt=cdp->get_statement(cd->hstmt);
        if (opc_type > 1)  /* take the record number, translate it */
        { recnum=*(trecnum*)request;  request+=sizeof(trecnum);
          if (cd->curs_eseek(recnum)) goto err_goto;
          if (opc_type > 2)
          { curselem=*(tattrib*)request;  request+=sizeof(tattrib);
            if (!attr_access(&cd->qe->kont, curselem, tbnum, attr, recnum, attracc))
              { request_error(cdp, BAD_ELEM_NUM);  goto err_goto; }
            if (attracc==NULL) tbdf=tables[tbnum];  // tabdescr is locked in the cursor
          }
        }
      }
    }
    switch (opc)
    {
    case OP_LOGIN:   /*0*/
      if (!(*request & LOGIN_SIMULATED_FLAG))
      { cdp->free_all_stmts();
        if (cdp->in_use==PT_SLAVE && !VersionOK(cdp->clientVersion))
          { request_error(cdp, BAD_VERSION);  goto err_goto; }
      }
      if (op_login(cdp, (uns8**)&request)) goto err_goto;
      break;
    case OP_LIST:  /* category */
      list_objects(cdp, *(tcateg*)request, (uns8*)(request+sizeof(tcateg)));
      request+=sizeof(tcateg)+UUID_SIZE;
      break;
    case OP_CNTRL:
      switch (*(request++))
      { case SUBOP_SET_WAITING:
          cdp->waiting=*(sig32*)request;  request+=sizeof(sig32);
          break;
        case SUBOP_GET_ERR_MSG:
          strcpy(cdp->get_ans_ptr(sizeof(tobjname)), cdp->errmsg);
          break;
        case SUBOP_GET_ERR_CONTEXT:
        { unsigned num = *(uns32*)request;  request+=sizeof(uns32);
          sig32 * result = (sig32*)cdp->get_ans_ptr(5*sizeof(sig32));
          if (!result) break;
          if (num >= cdp->exkont_len || !cdp->exkont_info)
            *result=EKT_NO;
          else
          { *(result++) = cdp->exkont_info[num].type;
            *(result++) = cdp->exkont_info[num].par1;
            *(result++) = cdp->exkont_info[num].par2;
            *(result++) = cdp->exkont_info[num].par3;
            *(result  ) = cdp->exkont_info[num].par4;
          }
          break;
        }
        case SUBOP_GET_ERR_CONTEXT_TEXT:  
        { cdp->answer.sized_answer_start();  
          unsigned num = *(uns32*)request;  request+=sizeof(uns32);
          if (num >= cdp->exkont_len || !cdp->exkont_info)
            *cdp->get_ans_ptr(sizeof(char)) = 0;
          else
          { int len = (int)strlen(cdp->exkont_info[num].text);
            char * p = cdp->get_ans_ptr(len+1);
            if (p) strcpy(p, cdp->exkont_info[num].text);
          }
          cdp->answer.sized_answer_stop();
          break;
        }
        case SUBOP_SET_FILSIZE: // size in bytes
        { BOOL set_it = (t_oper)*request==OPER_SET;
          if (set_it)
          { uns32 newsize=*(uns32*)(request+1);
            if (!cdp->prvs.is_conf_admin)
              { request_error(cdp, NO_CONF_RIGHT);  goto err_goto; }
            bool res;  
            { ProtectedByCriticalSection cs(&disksp.bpools_sem, cdp, WAIT_CS_BPOOLS); 
              res=ffa.set_fil_size(cdp, newsize / BLOCKSIZE);
            }
            if (res)  
              { request_error(cdp, OS_FILE_ERROR);  goto err_goto; }
          }
          else *(uns32*)cdp->get_ans_ptr(sizeof(uns32)) = ffa.get_fil_size()*BLOCKSIZE;
          request+=1+sizeof(uns32);
          break;
        }
        case SUBOP_SET_FILSIZE64: // size in blocks 
        { BOOL set_it = (t_oper)*request==OPER_SET;
          if (set_it)
          { tblocknum newblocks=*(tblocknum*)(request+1);
            if (!cdp->prvs.is_conf_admin)
              { request_error(cdp, NO_CONF_RIGHT);  goto err_goto; }
            bool res;  
            { ProtectedByCriticalSection cs(&disksp.bpools_sem, cdp, WAIT_CS_BPOOLS); 
              res=ffa.set_fil_size(cdp, newblocks);
            }  
            if (res)
              { request_error(cdp, OS_FILE_ERROR);  goto err_goto; }
          }
          else *(tblocknum*)cdp->get_ans_ptr(sizeof(tblocknum)) = ffa.get_fil_size();
          request+=1+sizeof(tblocknum);
          break;
        }
        case SUBOP_LOCK:
          if (!cdp->prvs.is_conf_admin)  // only admin can do this
            { request_error(cdp, NO_CONF_RIGHT);  goto err_goto; }
          if (dir_lock_kernel(cdp, true)) goto err_goto;
          ResetDelRecsTable();
          request++;
          break;
        case SUBOP_UNLOCK:
          if (!cdp->prvs.is_conf_admin)  // only admin can do this
            { request_error(cdp, NO_CONF_RIGHT);  goto err_goto; }
          dir_unlock_kernel(cdp);
          request++;
          break;
        case SUBOP_WORKER_TH:
          if (!cdp->prvs.is_conf_admin)  // only admin can do this
            { request_error(cdp, NO_CONF_RIGHT);  goto err_goto; }
          switch (*request++)
          { case DISABLE_NEW_CLIENTS:
              dir_lock_kernel(cdp, false);  break;  // disable connecting of new users (no errors)
            case WORKER_STOP:  
              stop_working_threads(cdp);  break;
            case WORKER_RESTART:  
              restart_working_threads(cdp);  break;
            case ENABLE_NEW_CLIENTS:
              dir_unlock_kernel(cdp);  break;
          }
          break;
        case SUBOP_BACK_COUNT:
          *(uns32*)cdp->get_ans_ptr(sizeof(uns32))=journal_start_time(cdp);
          request++;
          break;
        case SUBOP_USER_UNLOCK:
          unlock_user(cdp, cdp->applnum_perm, *request!=0);
          request++;
          break;
        case SUBOP_KILL_USER:  // not completely safe
        { int linknum=*(uns16*)request;  request+=sizeof(uns16);
          if (kill_the_user(cdp, linknum)) goto err_goto;
          break;
        }
        case SUBOP_BREAK_USER:
        { int clinum=*(sig32*)request;  request+=sizeof(sig32);
          if (break_the_user(cdp, clinum)) goto err_goto;
          break;
        }
        case SUBOP_REMOVE_LOCK:
          if (!cdp->prvs.is_conf_admin) { request_error(cdp, NO_CONF_RIGHT);  goto err_goto; }
          remove_the_lock(cdp, (lockinfo *)request);
          request+=sizeof(lockinfo);
          break;
        case SUBOP_WHO_AM_I:  // called even during conditional login
        { tptr p=cdp->get_ans_ptr(sizeof(tobjname));  /* anonymous user */
          tobjnum userobj = (tobjnum)(cdp->cond_login ? cdp->cond_login : cdp->prvs.luser());
          logged_user_name(cdp, userobj, p);
          break;
        }
        case SUBOP_TSE:
          local_Enable_task_switch(*request);
          request++;
          break;
        case SUBOP_REPLIC:
          if (!cdp->prvs.is_conf_admin) { request_error(cdp, NO_CONF_RIGHT);  goto err_goto; }
          repl_stop();         // stops iff running
          repl_start(cdp);     // starts only iff allowed
          request++;
          break;
        case SUBOP_SET_PASSWORD:
        { tobjnum userobj;
          userobj=*(tobjnum*)request;  request+=sizeof(tobjnum);
          tobjnum me = cdp->cond_login ? cdp->cond_login : cdp->prvs.luser();
          if (userobj==NOOBJECT)
            userobj=me;
          else if (userobj!=me && !cdp->prvs.is_secur_admin) 
            { request_error(cdp, NO_RIGHT);  goto err_goto; }
          write_password(cdp, userobj, (uns8 *)request);  request+=sizeof(uns32)+MD5_SIZE;
          if (cdp->cond_login)
          { cdp->prvs.set_user(cdp->cond_login, CATEG_USER, TRUE);
            cdp->cond_login=0;
          }
          if (cdp->conditional_attachment)  // must complete the login
          { tobjname logname;
            tb_read_atr(cdp, usertab_descr, userobj, USER_ATR_LOGNAME, logname);
            do_login_change(cdp, 0, logname, userobj, msg_login_normal, false);
            WriteSlaveThreadCounter();  // must be after login because conditional_attachment is ignored
          }
          break;
        }                           
        case SUBOP_CREATE_USER:
        { t_op_create_user * uinfo = (t_op_create_user *)request;  request+=sizeof(t_op_create_user);
         // create user:
          trecnum trec=new_user_or_group(cdp, uinfo->logname, CATEG_USER, uinfo->user_uuid);  // privileges checked inside
          if (trec==NORECNUM) break;  // error, e.g. name duplicity or not allowed to create users
          tb_write_atr(cdp, usertab_descr->me(), trec, USER_ATR_INFO,   (tptr)&uinfo->info);
          tb_write_atr(cdp, usertab_descr->me(), trec, USER_ATR_SERVER, (tptr)uinfo->home_srv);
         // store its password:
          write_password(cdp, trec, uinfo->enc_password);  
          *(tobjnum*)cdp->get_ans_ptr(sizeof(tobjnum)) = trec;
          break;
        }
        case SUBOP_SQLOPT:
        { uns32 mask = *(uns32*)request, opts=((uns32*)request)[1];
          cdp->sqloptions = (cdp->sqloptions & ~mask) | (opts & mask);  
          request+=2*sizeof(uns32);
          break;
        }
        case SUBOP_SQLOPT_GET:
          *(uns32*)cdp->get_ans_ptr(sizeof(uns32))=cdp->sqloptions;
          break;
        case SUBOP_OPTIMIZATION:
        { wbbool eval = *(wbbool*)request;  request+=sizeof(wbbool);
          t_sql_kont sql_kont;  t_query_expr * qe;  t_optim * opt;  cur_descr * cd;  tcursnum cursnum;
          if (compile_query(cdp, request, &sql_kont, &qe, &opt)) goto err_goto;
          request+=strlen(request)+1;
          if (eval)
          { cursnum=create_cursor(cdp, qe, NULL, opt, &cd);
            if (cursnum!=NOCURSOR) make_complete(cd);
          }
          cdp->answer.sized_answer_start();
          { t_flstr dmp;
            opt->dump(cdp, &dmp, eval!=0);
            tptr src=dmp.get();  
            int len=(int)strlen(src);
            tptr dest=cdp->get_ans_ptr(len+1);
            if (dest!=NULL) 
              { memcpy(dest, src, len);  dest[len]=0; }
          }
          if (eval) free_cursor(cdp, cursnum);
          cdp->answer.sized_answer_stop();
          delete qe;  delete opt;
          break;
        }
        case SUBOP_COMPACT:
          if (!cdp->prvs.is_conf_admin) { request_error(cdp, NO_CONF_RIGHT);  goto err_goto; }
          if (dir_lock_kernel(cdp, true)) goto err_goto;
          compact_database(cdp, *(sig32*)request);
          dir_unlock_kernel(cdp);
          request+=sizeof(sig32);
          break;
        case SUBOP_CDROM:
          if (!cdp->prvs.is_conf_admin) { request_error(cdp, NO_CONF_RIGHT);  goto err_goto; }
          if (dir_lock_kernel(cdp, true)) goto err_goto;
          ffa.cdrom(cdp);
          dir_unlock_kernel(cdp);
          break;
        case SUBOP_REFINT:
          if (*request) cdp->ker_flags &= ~KFL_DISABLE_REFINT;
          else          cdp->ker_flags |=  KFL_DISABLE_REFINT;
          request++;
          break;
        case SUBOP_INTEGRITY:
          if (*request) cdp->ker_flags &= ~KFL_DISABLE_INTEGRITY;
          else          cdp->ker_flags |=  KFL_DISABLE_INTEGRITY;
          request++;
          break;
        case SUBOP_LOGWRITE:
          write_to_log(cdp, request);  request+=strlen(request)+1;
          break;
#ifdef STOP  // not used any more
        case SUBOP_CLEARPASS:
          if (!cdp->prvs.is_conf_admin) { request_error(cdp, NO_CONF_RIGHT);  goto err_goto; }
          clear_all_passwords(cdp);
          break;
#endif
        case SUBOP_ISOLATION:
          cdp->isolation_level=(t_isolation)*request;
          request++;  break;
        case SUBOP_GET_LOGIN_KEY:
          *(uns32  *)cdp->get_ans_ptr(sizeof(uns32  ))=cdp->random_login_key;
          *(tobjnum*)cdp->get_ans_ptr(sizeof(tobjnum))=cdp->prvs.luser();
          break;
        case SUBOP_APPL_INSTS:
        { int count=0;
          { ProtectedByCriticalSection cs(&cs_client_list, cdp, WAIT_CS_CLIENT_LIST);  
            for (int i=1;  i<=max_task_index;  i++) 
              if (cds[i]) // counts itself, too
                if (!memcmp(cds[i]->front_end_uuid, cdp->front_end_uuid, UUID_SIZE))
                  count++;
          }
          *(uns32*)cdp->get_ans_ptr(sizeof(uns32)) = count;
          break;
        }
        case SUBOP_MESSAGE:
          MessageToClients(cdp, request);  request+=strlen(request)+1;
          break;
        case SUBOP_TRIG_UNREGISTER: // unregister the trigger before it is changed or deleted
        { tobjnum trigobj = *(tobjnum*)request;  request+=sizeof(tobjnum);
          psm_cache.invalidate(trigobj, CATEG_TRIGGER);
          register_unregister_trigger(cdp, trigobj, FALSE);
          break;
        }
        case SUBOP_TRIG_REGISTER: // register the trigger after it is changed or created, fulltext system alter importing it
        { tobjnum trigobj = *(tobjnum*)request;  request+=sizeof(tobjnum);  tcateg categ;
          fast_table_read(cdp, objtab_descr, trigobj, OBJ_CATEG_ATR, &categ);
          if (categ==CATEG_TRIGGER)
            register_unregister_trigger(cdp, trigobj, TRUE);
          else if (categ==CATEG_INFO)
            register_tables_in_ftx(cdp, trigobj);
          break;
        }
        case SUBOP_PROC_CHANGED:
          psm_cache.invalidate(NOOBJECT, 0);  // must invalidate all calling routines and triggers
          refresh_global_scopes(cdp);
          uninst_tables(cdp);  // CHECK constrains, DEFAULT values etc. may call the changed function!
          break;
        case SUBOP_REPORT_MODULUS:
          cdp->report_modulus=*(uns32*)request;  request+=sizeof(uns32);  break;
        case SUBOP_BACKUP:
        { if (!cdp->prvs.is_conf_admin)
            { request_error(cdp, NO_CONF_RIGHT);  goto err_goto; }
          int res=backup_entry(cdp, request);  // replaces empty file name by the default one
          request+=strlen(request)+1;
          if (res!=ANS_OK)
          { request_error(cdp, res);  // report the error to the client (after calling the trigegr)
            goto err_goto;
          }  
          break;
        }
        case SUBOP_CLOSE_CURSORS:
          if (!close_owned_cursors(cdp, false))
            { request_error(cdp, INTERNAL_SIGNAL);  goto err_goto; } // no open cursor found
          break;
        case SUBOP_SET_AUTHORING:
          cdp->prvs.set_temp_author(*request++ ? cdp->author_role : NOOBJECT);  break;
        case SUBOP_ADD_LIC_1:
        { uns32 * ptr = (uns32*)cdp->get_ans_ptr(4*sizeof(uns32));
          ptr[0]=GetTickCount() ^ 0x7a5cd01f;
          ptr[1]=GetTickCount() ^ 0xf4ca75a5;
          ptr[2]=GetTickCount() ^ 0x2c73d60f;
          ptr[3]=GetTickCount() ^ 0x4b195df0;
          memcpy(add_lic_key, ptr, 4*sizeof(uns32));
          break;
        }
        case SUBOP_ADD_LIC_2:
        { if (!add_lic_key[0] && !add_lic_key[2] && !add_lic_key[2] && !add_lic_key[3]) 
            { request_error(cdp, ERROR_IN_FUNCTION_ARG);  goto err_goto; }
          add_lic_key[4]=0x5a5cd0bf;
          add_lic_key[5]=0xf743c5a8;
          add_lic_key[6]=0x2e7ad7c1;
          add_lic_key[7]=0x4b191df0;
          add_lic_key[8]=*(uns32*)request;  request+=sizeof(uns32);
          uns8 digest[MD5_SIZE];
          md5_digest((uns8*)add_lic_key, sizeof(add_lic_key), digest);
          if (!memcmp(digest, request, MD5_SIZE))
          { unsigned total, used;
            client_licences.get_info(&total, &used);
            client_licences.update_total_lics(total+add_lic_key[8]);
          }
          else { request_error(cdp, ERROR_IN_FUNCTION_ARG);  goto err_goto; }
          request+=MD5_SIZE;
          add_lic_key[0]=add_lic_key[1]=add_lic_key[2]=add_lic_key[3]=0;  // disables reusing SUBOP_ADD_LIC_2 without a new SUBOP_ADD_LIC_1
          break;
        }
        case SUBOP_SET_TZD:
          cdp->tzd = *(sig32*)request;  request+=sizeof(sig32);
          break;
        case SUBOP_ROLLED_BACK:
        { uns16 error_code = *(uns16*)request;  request+=sizeof(uns16);
          *(uns8*)cdp->get_ans_ptr(sizeof(uns8)) = condition_category(error_code)!=CONDITION_COMPLETION;
          break;
        }
        case SUBOP_TRANS_OPEN:
          *(uns8*)cdp->get_ans_ptr(sizeof(uns8)) = cdp->expl_trans!=TRANS_NO;
          break;
        case SUBOP_SET_WEAK_LINK:
          cdp->weak_link=true;  break;
        case SUBOP_CHECK_INDECES:   // undocumented function
        { ttablenum tbnum = *(ttablenum*)request;  request += sizeof(ttablenum);
          sig32 * result = (sig32*)cdp->get_ans_ptr(sizeof(sig32));
          sig32 * indnum = (sig32*)cdp->get_ans_ptr(sizeof(sig32));
          *indnum=-1;
          tcateg categ;
          if (tbnum<0 || tbnum>=tabtab_descr->Recnum())
            { *result=-3;  break; }  // tbnum is outside the interval of table numbers
          if (table_record_state(cdp, tabtab_descr, tbnum))
            { *result=-2;  break; }  // tbnum is not a number of a valid table
          if (fast_table_read(cdp, tabtab_descr, tbnum, OBJ_CATEG_ATR, &categ))
            { *result=-1;  break; }  // error
          if (categ & IS_LINK)
            { *result=-2;  break; }  // tbnum is not a number of a valid table
          { table_descr_auto tbdf(cdp, tbnum);
            if (!tbdf->me())
              { *result=-1;  break; }  // error
            else
            { *result=1;  // OK
              if (wait_record_lock_error(cdp, tbdf->me(), FULLTABLE, TMPRLOCK)) 
                goto err_goto;
              for (int i = 0;  i<tbdf->indx_cnt;  i++)
              { int res=check_index(cdp, tbdf->me(), i);
                if (res!=1)
                  { *result=res;  *indnum=i;  break; }  // index or free rec map damaged
              }
              record_unlock(cdp, tbdf->me(), FULLTABLE, TMPRLOCK);
            }
          }
          break;
        }
#ifdef ANY_SERVER_GUI
        case SUBOP_RESTART:
          if (!cdp->prvs.is_conf_admin)
            { request_error(cdp, NO_CONF_RIGHT);  break; }
          PostMessage(hwMain, uRestartMessage, 0, 0);
          break;
#endif
        case SUBOP_STOP:
        { 
#if WBVERS<900
          if (!cdp->prvs.is_conf_admin)
            { request_error(cdp, NO_CONF_RIGHT);  break; }
#else
          client_logout_timeout = *(uns32*)request;  request+=sizeof(uns32);
#endif           
          if (!cdp->prvs.is_conf_admin)
            { request_error(cdp, NO_CONF_RIGHT);  break; }
          down_reason=down_from_client;
          stop_the_server();
          break;
        }  
        case SUBOP_TABLE_INTEGRITY:
        { int cache_error, index_error;
          uns32 extent = *(uns32*)request;  request+=sizeof(uns32);
          uns32 repair = *(uns32*)request;  request+=sizeof(uns32);
          check_all_indicies(cdp, false, INTEGR_CHECK_INDEXES|INTEGR_CHECK_CACHE, repair!=0, cache_error, index_error);
          *(uns32*)cdp->get_ans_ptr(sizeof(uns32)) = index_error;
          *(uns32*)cdp->get_ans_ptr(sizeof(uns32)) = cache_error;
          break;
        }
        case SUBOP_MASK_DUPLICITY:
          if (*(request++))
            cdp->ker_flags |= KFL_MASK_DUPLICITY_ERR;
          else
            cdp->ker_flags &= ~KFL_MASK_DUPLICITY_ERR;
          break;
#if WBVERS>=1100
        case SUBOP_LIC1:
        { const char * lic, *company, * computer;  char * req;  void * licence_handle;
          lic     =request;  request+=strlen(request)+1;
          company =request;  request+=strlen(request)+1;
          computer=request;  request+=strlen(request)+1;
          char licdir[MAX_PATH];  
          get_path_prefix(licdir);
#ifdef LINUX
          strcat(licdir, "/lib/" WB_LINUX_SUBDIR_NAME); 
#endif
          int res=licence_init(&licence_handle, SOFT_ID, licdir, "en-US", 1, NULL, NULL);
          if (licence_handle)
          { saveIKs(licence_handle, &lic, 1);
            saveNames(licence_handle, company, computer);
            res=generateActivationRequest(licence_handle, &req);
            if (res==LICENCE_OK)
            { cdp->answer.sized_answer_start();
              strcpy(cdp->get_ans_ptr((int)strlen(req)+1), req);
              cdp->answer.sized_answer_stop();
            }
            else
              request_wcs_error(cdp, res, licence_handle);
            licence_close(licence_handle);
          }
          else
            request_wcs_error(cdp, res, licence_handle);
          break;
        }  
        case SUBOP_LIC2:
        { char * response;  void * licence_handle;
          response=request;  request+=strlen(request)+1;
          char licdir[MAX_PATH];  
          get_path_prefix(licdir);
#ifdef LINUX
          strcat(licdir, "/lib/" WB_LINUX_SUBDIR_NAME); 
#endif
          int res=licence_init(&licence_handle, SOFT_ID, licdir, "en-US", 1, NULL, NULL);
          if (licence_handle)
          { saveProp(licence_handle, WCP_BETA, "");  // remove the activation made by product with embedded 602SQL
            res=acceptActivationResponse(&licence_handle, response);  // can change the licence_handle!
            if (res!=LICENCE_OK)
              request_wcs_error(cdp, res, licence_handle);
            licence_close(licence_handle);
            read_addon_licences();  // loads the new state
          }
          else
            request_wcs_error(cdp, res, licence_handle);
          break;
        }  
#endif
        case SUBOP_CRASH:
        { if (!cdp->prvs.is_conf_admin)
            { request_error(cdp, NO_CONF_RIGHT);  goto err_goto; }
          char * p = NULL;
          *p = 1;
          break;
        }
        case SUBOP_PARK_SETTINGS:
          if (!cdp->prvs.is_conf_admin)
            { request_error(cdp, NO_CONF_RIGHT);  goto err_goto; }
          park_server_settings(cdp);  
          break;
        case SUBOP_EXPORT_LOBREFS:
          if (*request) cdp->ker_flags |=  KFL_EXPORT_LOBREFS;
          else          cdp->ker_flags &= ~KFL_EXPORT_LOBREFS;
          request++;
          break;
        default:
          request_error(cdp, BAD_OPCODE);  goto err_goto;
      } // SUBOP_ switch
      break;
    case OP_FINDOBJ:
    { tobjname objname;  memcpy(objname, request+sizeof(tcateg), OBJ_NAME_LEN);  objname[OBJ_NAME_LEN]=0;  // is not 0-terminated in the request packet
      if (*objname!=1) sys_Upcase(objname);  // not necessary but I want to remove this conversion from find2_object in the future
      tobjnum objnum=find2_object(cdp, *request, (uns8*)(request+sizeof(tcateg)+OBJ_NAME_LEN), objname);
      request+=sizeof(tcateg)+OBJ_NAME_LEN+UUID_SIZE;
      if (objnum==NOOBJECT) 
      { if (*objname==1)
        { char buf[2*UUID_SIZE+1];
          bin2hex(buf, (uns8*)objname+1, UUID_SIZE);  buf[2*UUID_SIZE]=0;
          SET_ERR_MSG(cdp, buf);  
        }
        else SET_ERR_MSG(cdp, objname);  
        request_error(cdp, OBJECT_NOT_FOUND);  goto err_goto; 
      }
      *(tobjnum*)cdp->get_ans_ptr(sizeof(tobjnum))=objnum;
      break;
    }

    case OP_PRIVILS:  /*0*/
    { op_privils * p = (op_privils*)request;
      privils prvs(cdp);
      tobjnum subject = p->user_group_role;
      if (subject==NOOBJECT)  // NOOBJECT denotes the current user
        { subject=cdp->prvs.luser();  p->subject_categ=CATEG_USER; }
      t_exkont_table ekt(cdp, p->table, p->record, NOATTRIB, NOINDEX, OP_PRIVILS, NULL, 0);
      prvs.set_user(subject, (tcateg)p->subject_categ, p->operation==OPER_GETEFF);
      std=!IS_CURSOR_NUM(p->table);
      if (std)
      { tbdf=locked_tabdescr=install_table(cdp, p->table);
        if (!tbdf) goto err_goto;  /* error inside */
      }
      else
      { cd = get_cursor(cdp, p->table);
        if (!cd) goto err_goto;
      }
      switch (p->operation)
      { case OPER_SET: /* set own */
        { if (!std)
            { request_error(cdp, ERROR_IN_FUNCTION_ARG);  goto err_goto; }
          t_privils_flex priv_val(tbdf->attrcnt);
          unsigned length = 1+priv_val.colbytes();
          memcpy(priv_val.the_map(), p->privils, length);
          if (!cdp->prvs.can_grant_privils(cdp, &prvs, tbdf, p->record, priv_val))
            { request_error(cdp, NO_RIGHT);  goto err_goto; }
         // check role owner against object owner:
          if (p->subject_categ==CATEG_ROLE)
          { WBUUID role_uuid, object_uuid;  table_descr * uuid_tab;  trecnum rec;
            tb_read_atr(cdp, objtab_descr, subject, APPL_ID_ATR, (tptr)role_uuid);
            if (p->table > OBJ_TABLENUM) { uuid_tab=tabtab_descr;  rec=p->table;  }
            else                         { uuid_tab=tbdf;          rec=p->record; }
            tb_read_atr(cdp, uuid_tab, rec, APPL_ID_ATR, (tptr)object_uuid);
            if (memcmp(role_uuid, object_uuid, UUID_SIZE))
              { request_error(cdp, ROLE_FROM_DIFF_APPL);  goto err_goto; }
          }
          prvs.set_own_privils(tbdf, p->record, priv_val);
          if (length>PRIVIL_DESCR_SIZE) request+=(length-PRIVIL_DESCR_SIZE);
          break;
        }
        case OPER_GET: /* get own: returns PRIVIL_DESCR_SIZE bytes or more if necessary */
        { t_privils_flex priv_val;
          if (!std)
               prvs.get_cursor_privils(cd,   p->record, priv_val, false);
          else prvs.get_own_privils   (tbdf, p->record, priv_val);
          unsigned length = 1+priv_val.colbytes();
          uns8 * respriv = (uns8*)cdp->get_ans_ptr(length>PRIVIL_DESCR_SIZE ? length : PRIVIL_DESCR_SIZE);
          if (respriv==NULL) { request_error(cdp, OUT_OF_KERNEL_MEMORY);  goto err_goto; }
          memcpy(respriv, priv_val.the_map(), length); 
          break;
        }
        case OPER_GETEFF: /* get effective: returns PRIVIL_DESCR_SIZE bytes or more if necessary */
        { t_privils_flex priv_val;
          if (!std)
               prvs.get_cursor_privils   (cd,   p->record, priv_val, true);
          else prvs.get_effective_privils(tbdf, p->record, priv_val);
          unsigned length = 1+priv_val.colbytes();
          uns8 * respriv = (uns8*)cdp->get_ans_ptr(length>PRIVIL_DESCR_SIZE ? length : PRIVIL_DESCR_SIZE);
          if (respriv==NULL) { request_error(cdp, OUT_OF_KERNEL_MEMORY);  goto err_goto; }
          memcpy(respriv, priv_val.the_map(), length); 
          break;
        }
      }
      request+=sizeof(op_privils);   // OPER_SET with many columns: [request] partially increased above
      break;
    }
    case OP_GROUPROLE:
    { op_grouprole * p = (op_grouprole*)request;  uns8 res_state;
      switch (p->operation)
      { case OPER_SET: // set
          user_and_group(cdp, p->user_or_group, p->group_or_role,
                 (tcateg)p->subject2, (t_oper)p->operation, (uns8*)&p->relation);
          break;
        case OPER_GET: // get
          user_and_group(cdp, p->user_or_group, p->group_or_role,
                 (tcateg)p->subject2, (t_oper)p->operation, &res_state);
          *(uns32*)cdp->get_ans_ptr(sizeof(uns32))=res_state;
          break;
        case OPER_GETEFF:
        { tcateg subject_categ;
          tb_read_atr(cdp, usertab_descr, p->user_or_group, USER_ATR_CATEG, (tptr)&subject_categ);
          res_state = get_effective_membership(cdp, p->user_or_group, subject_categ, p->group_or_role, (tcateg)p->subject2);
          *(uns32*)cdp->get_ans_ptr(sizeof(uns32))=res_state;
          break;
        }
        default:
          request_error(cdp, ERROR_IN_FUNCTION_ARG);  goto err_goto;
      }
      request+=sizeof(op_grouprole);
      break;
    }

    case OP_START_TRA:
      if (cdp->expl_trans!=TRANS_NO) warn(cdp, WAS_IN_TRANS);
      else
      { commit(cdp);  /* finalize previous parts of the package */
        cdp->expl_trans=TRANS_EXPL;
        cdp->trans_rw=TRUE;
      }
      break;
    case OP_COMMIT:
      ///if (cdp->expl_trans==TRANS_NO) { warn(cdp, NOT_IN_TRANS); break; }
      trace_msg_general(cdp, TRACE_SQL, "Commit (explicit)", NOOBJECT);
      if (is_trace_enabled_global(TRACE_PIECES)) 
        trace_msg_general(cdp, TRACE_PIECES, "commit piece allocs", NOOBJECT);
      commit(cdp);     /* sets expl_trans to TRANS_NO */
      break;
    case OP_ROLL_BACK:
      //if (cdp->expl_trans==TRANS_NO) { warn(cdp, NOT_IN_TRANS); break; }
      trace_msg_general(cdp, TRACE_SQL, "RollBack (explicit)", NOOBJECT);
      if (is_trace_enabled_global(TRACE_PIECES)) 
        trace_msg_general(cdp, TRACE_PIECES, "rollback piece allocs", NOOBJECT);
      roll_back(cdp);  /* sets expl_trans to TRANS_NO */
      break;

    case OP_RECCNT: /*1: returns the number of records in a cursor or table */
      if (std) recnum=tbdf->Recnum();
      else { make_complete(cd);  recnum=cd->recnum; }
      *(trecnum*)cdp->get_ans_ptr(sizeof(trecnum))=recnum;
      break;
    case OP_VALRECS: /*1: seeks for non-deleted records */
      get_valid_records(cdp, cursnum, *(trecnum*)request,
        *(trecnum*)(request+sizeof(trecnum)));
      request+=2*sizeof(trecnum);
      break;
    case OP_DELETE: /*2*/
    { trecnum curs_recnum;
      if (!std)  // deleting in a cursor
      { if (cd->underlying_table==NULL)
        // must not delete, remove from cursor only
        {// delete the non-translated recnum:
          if (!PTRS_CURSOR(cd))
            { request_error(cdp, CANNOT_APPEND);  goto err_goto; }
          cd->pmat->curs_del(cdp, recnum);
          break;
        }
        else
        { curs_recnum=recnum;
          recnum=cd->underlying_table->kont.position;
          tbnum =cd->underlying_table->tabnum;
          tbdf=tables[tbnum];
        }
        if (!(cd->qe->qe_oper & QE_IS_DELETABLE))
          { request_error(cdp, CANNOT_DELETE_IN_THIS_VIEW);  goto err_goto; }
      }
     // check rights - common for table & sigle-table cursor:
      if (SYSTEM_TABLE(tbnum))  // prevent deleting standard objects
      { if (tbnum==TAB_TABLENUM)  /* must use "DROP TABLE xyz" statement */
          { request_error(cdp, NO_RIGHT);  goto err_goto; }
        else if (tbnum==USER_TABLENUM)  // must prevent deleting standard users & groups
        { if (recnum<FIXED_USER_TABLE_OBJECTS) { request_error(cdp, NO_RIGHT);  break; }
        }
      }
      if (!can_delete_record(cdp, tbdf, recnum))
        { request_error(cdp, NO_RIGHT);  break; }

      t_tab_trigger ttr;
      if (!ttr.prepare_rscopes(cdp, tbdf, TGA_DELETE)) break;
      if (ttr.pre_stmt(cdp, tbdf)) break;
      if (ttr.pre_row(cdp, tbdf, recnum, NULL)) break;

      cdp->except_type=EXC_NO;  cdp->except_name[0]=0;  cdp->processed_except_name[0]=0; // ???
      if (tb_del(cdp, tbdf, recnum)) break; /* don't remove from cursor on error */

     // remove the record from cursor:
      if (!std)
      {// delete the non-translated recnum:
        if (!PTRS_CURSOR(cd))
          { request_error(cdp, CANNOT_APPEND);  goto err_goto; }
        cd->pmat->curs_del(cdp, curs_recnum);
        while (cd->subcurs && IS_CURSOR_NUM(cd->super)) /* deleting from supercursors */
        { trecnum crec;
          { ProtectedByCriticalSection cs(&crs_sem, cdp, WAIT_CS_CURSORS);
            cd=*crs.acc(GET_CURSOR_NUM(cd->super));
          }
          if (cd==NULL) break;
          if (!PTRS_CURSOR(cd))
            { request_error(cdp, CANNOT_APPEND);  goto err_goto; }
          if (cd->pmat->cursor_search(cdp, &recnum, &crec, cd->tabcnt))
            cd->pmat->curs_del(cdp, crec);
        }
      }
      if (ttr.post_row(cdp, tbdf, recnum)) break;
      ttr.post_stmt(cdp, tbdf);
      break;
    }
    case OP_UNDEL: /*2 undelete allowed only for tables, not for cursors */
      if (!std) { request_error(cdp, CANNOT_APPEND);  break; }
      if (!can_delete_record(cdp, tbdf, recnum))
        { request_error(cdp, NO_RIGHT);  break; }
      tb_undel(cdp, tbdf, recnum);
      break;

    case OP_READREC: /*2*/
      read_record(cdp, std, tbdf, recnum, cd, request);
      if (cdp->get_return_code()==WORKING_WITH_EMPTY_RECORD)
        { request_error(cdp, EMPTY);  break; }
      break;
    case OP_WRITEREC:/*2*/
      write_record(cdp, std, tbdf, recnum, cd, request);
      if (cdp->get_return_code()==WORKING_WITH_EMPTY_RECORD)
        { request_error(cdp, EMPTY);  break; }
      break;
    case OP_INSERTRECEX: /*1*/
    case OP_APPENDRECEX: /*1*/
    case OP_WRITERECEX:  /*2*/     
    { uns32 datasize = *(uns32*)request;   request+=sizeof(uns32);
      int   count    = *(uns16*)request;   request+=sizeof(uns16);
      tattrib * cols = (tattrib*)request;  request+=count*sizeof(tattrib); 
      char * databuf = request;            request+=datasize;
      write_insert_rec_ex(cdp, opc==OP_WRITERECEX ? 0 : opc==OP_INSERTRECEX ? 1 : 2, 
        std ? NULL : cd, crnm, std ? tbdf : NULL, recnum, count, cols, databuf);
      break;
    }
    case OP_APPEND:  /*1*/
    case OP_INSERT:  /*1*/
    { trecnum rec, trec;
      if (!std)
      { if (cd->underlying_table==NULL)
          { request_error(cdp, CANNOT_APPEND);  break; }
        if (!(cd->qe->qe_oper & QE_IS_INSERTABLE))
          { request_error(cdp, CANNOT_INSERT_INTO_THIS_VIEW);  break; }
        tbnum=cd->underlying_table->tabnum;   /* table installed */
        tbdf=tables[tbnum];  // tabdescr locked in the cursor
      }
      if (tbnum==TAB_TABLENUM || tbnum==OBJ_TABLENUM)  // this is not allowed, must use CREATE statement or cd_Isert_object
        { request_error(cdp, NO_RIGHT);  break; }
      if (!can_insert_record(cdp, tbdf))
      { t_exkont_table ekt(cdp, tbdf->tbnum, NORECNUM, NOATTRIB, NOINDEX, OP_INSERT, NULL, 0);
        request_error(cdp, NO_RIGHT);  break; 
      }
     // inserting the new record into the table:
      trec=tb_new(cdp, tbdf, opc==OP_APPEND && std, NULL);
      if (trec == NORECNUM) break;  /* error inside */
      if (!std) /* add to the 1-table cursor */
      { rec=new_record_in_cursor(cdp, trec, cd, crnm);
        if (rec==NORECNUM) goto err_goto;
      }
      else rec=trec;
      *(trecnum*)cdp->get_ans_ptr(sizeof(trecnum)) = rec;
     // triggers:
      tbdf->prepare_trigger_info(cdp);
      cdp->except_type=EXC_NO;  cdp->except_name[0]=0;  cdp->processed_except_name[0]=0;
      uns16 map = tbdf->trigger_map;
      if (map & TRG_AFT_INS_ROW) 
        if (execute_triggers(cdp, tbdf, TRG_AFT_INS_ROW, NULL, trec)) break;
      if (map & TRG_AFT_INS_STM) 
        if (execute_triggers(cdp, tbdf, TRG_AFT_INS_STM, NULL, trec)) break;
      break;
    }
    case OP_RECSTATE: /*1*/
    { recnum=*(trecnum*)request;  /* read position from request */
      request+=sizeof(trecnum);
      tptr result = cdp->get_ans_ptr(sizeof(uns8));
      if (std)
        *result = table_record_state(cdp, tbdf, recnum);
      else   /* cursor */
        *result = (uns8)cd->cursor_record_state(recnum);
      break;
    }

    case OP_RLOCK:    /*1*/ /* read lock to table definition needed for locking records */
      recnum=*(trecnum*)request;  request+=sizeof(trecnum);
      if (std) 
      { t_exkont_table ekt(cdp, tbnum, recnum, NOATTRIB, NOINDEX, OP_READ, NULL, 0);
        tb_rlock(cdp, tbdf, recnum);
      }
      else cursor_locking(cdp, opc, cd, recnum);
      break;
    case OP_UNRLOCK:  /*1*/
      recnum=*(trecnum*)request;  request+=sizeof(trecnum);
      if (std) tb_unrlock(cdp, tbdf, recnum);
      else cursor_locking(cdp, opc, cd, recnum);
      break;
    case OP_WLOCK:    /*1*/
      recnum=*(trecnum*)request;  request+=sizeof(trecnum);
      if (std) 
      { t_exkont_table ekt(cdp, tbnum, recnum, NOATTRIB, NOINDEX, OP_WRITE, NULL, 0);
        tb_wlock(cdp, tbdf, recnum);
      }
      else cursor_locking(cdp, opc, cd, recnum);
      break;
    case OP_UNWLOCK:  /*1*/
      recnum=*(trecnum*)request;  request+=sizeof(trecnum);
      if (std) tb_unwlock(cdp, tbdf, recnum);
      else cursor_locking(cdp, opc, cd, recnum);
      break;
    case OP_WHO_LOCKED: /*1*/
    { recnum=*(trecnum*)request;  request+=sizeof(trecnum);
      bool write_lock = *request != 0;  request++;
      if (!std) { request_error(cdp, CANNOT_APPEND);  break; }
      int result = who_locked(cdp, tbdf, recnum, write_lock);
      if (result<=0)  // can lock
        { request_error(cdp, INTERNAL_SIGNAL);  break; }
      tptr p = cdp->get_ans_ptr(sizeof(uns32)+sizeof(tobjname));
      *(uns32*)p = result-1;  p+=sizeof(uns32);
      { ProtectedByCriticalSection cs(&cs_client_list, cdp, WAIT_CS_CLIENT_LIST);  
        if (cds[result-1]!=NULL)
          strcpy(p, cds[result-1]->prvs.luser_name());
        else *p=0;
      }
      break;
    }

    case OP_ADDVALUE: /*3*/
      if (attracc!=NULL)   // non-editable (calculated) attribute
        { SET_ERR_MSG(cdp, attracc->name);  request_error(cdp, COLUMN_NOT_EDITABLE);  break; }
      tb_add_value(cdp, tbdf, recnum, attr, &request);
      break;
    case OP_DELVALUE: /*3*/
      if (attracc!=NULL)   // non-editable (calculated) attribute
        { SET_ERR_MSG(cdp, attracc->name);  request_error(cdp, COLUMN_NOT_EDITABLE);  break; }
      tb_del_value(cdp, tbdf, recnum, attr, &request);
      break;
    case OP_REMINVAL: /*3*/
      if (attracc!=NULL)   // non-editable (calculated) attribute
        { SET_ERR_MSG(cdp, attracc->name);  request_error(cdp, COLUMN_NOT_EDITABLE);  break; }
      tb_rem_inval(cdp, tbdf, recnum, attr);
      break;

    case OP_WRITE: /*3*/
    { uns16 index;  uns32 varstart, varsize;  t_privils_flex priv_val;
      if (*request==MODPTR || *request==MODINDPTR)
      { if (ptrsteps(cdp, &tbdf, &tbnum, &recnum, &attr, &request)) goto err_goto;
        std=TRUE;
      }
     /**************** checking access rights ***************************/
      if (std)  // table rights
      { cdp->prvs.get_effective_privils(tbdf, recnum, priv_val);
        if (attr ? !priv_val.has_write(attr) : !(*priv_val.the_map() & RIGHT_DEL))
          if (tbnum!=OBJ_TABLENUM || !(is_apl(cdp, tbnum, (tobjnum)recnum) && attr != OBJ_CATEG_ATR))
          { t_exkont_table ekt(cdp, tbdf->tbnum, recnum, attr, NOINDEX, OP_WRITE, NULL, 0);
            request_error(cdp, NO_RIGHT);  goto err_goto; 
          }
      }
      else      // cursor access rights:
      { if (attracc!=NULL)      // expression, not editable
        //    || cd->has_own_table)    // aggregation result
        { t_exkont_table ekt(cdp, cursnum, recnum, curselem, NOINDEX, OP_WRITE, NULL, 0);
          SET_ERR_MSG(cdp, attracc->name);  request_error(cdp, COLUMN_NOT_EDITABLE);  goto err_goto; 
        }
        if (!(cd->qe->qe_oper & QE_IS_UPDATABLE))
          { request_error(cdp, CANNOT_UPDATE_IN_THIS_VIEW);  goto err_goto; }
        if (!(cd->d_curs->attribs[curselem].a_flags & RI_UPDATE_FLAG))
          if (!(cd->d_curs->tabdef_flags & TFL_REC_PRIVILS))
            { request_error(cdp, NO_RIGHT);  goto err_goto; }
          else
          { cdp->prvs.get_effective_privils(tbdf, recnum, priv_val);
            if (!priv_val.has_write(attr))
            { t_exkont_table ekt(cdp, tbdf->tbnum, recnum, attr, NOINDEX, OP_WRITE, NULL, 0);
              request_error(cdp, NO_RIGHT);  goto err_goto; 
            }
          }
      }
     // writing to the DELETED column: special handling
      if (attr==DEL_ATTR_NUM)
      { if (*request!=MODSTOP) { request_error(cdp, BAD_MODIF);  goto err_goto; }
        request++;
        if (tbdf->tbnum==TAB_TABLENUM)
          { request_error(cdp, NO_RIGHT);  goto err_goto; }
        switch (*(request++))
        { case DELETED:     tb_del  (cdp, tbdf, recnum);  break;
          case NOT_DELETED: tb_undel(cdp, tbdf, recnum);  break;
          default:          request_error(cdp, NO_RIGHT);  goto err_goto;
        }
      }
     // normal WRITE operation:
      else
      { attribdef * att = tbdf->attrs + attr;
        switch (*request++)
        { case MODSTOP:
          { if (IS_HEAP_TYPE(att->attrtype) || att->attrmult != 1)
              { request_error(cdp, BAD_MODIF);  goto err_goto; }
            if (att->attrtype >= ATT_FIRSTSPEC && att->attrtype <= ATT_LASTSPEC)
              { request_error(cdp, NO_RIGHT);  goto err_goto; }
            if (att->attrtype == ATT_BIPTR)
            { if (tb_write_atr(cdp, tbdf, recnum, attr, request)) goto err_goto;
              request+=sizeof(trecnum);
            }
            else
            { write_one_column(cdp, tbdf, recnum, attr, request, 0, 0);
              request+=TYPESIZE(att);
            }
            break;
          }
          case MODIND:
            index=*(uns16*)request;  request+=sizeof(uns16);
            if (*request==MODSTOP)
            { request++;
              if (IS_HEAP_TYPE(att->attrtype) || att->attrmult <= 1)
                { request_error(cdp, BAD_MODIF);  goto err_goto; }
              if (tb_write_ind(cdp, tbdf, recnum, attr, index, request)) goto err_goto;
              request+=TYPESIZE(att);
            }
            else if (*request==MODINT)
            { request++;
              if (!IS_HEAP_TYPE(att->attrtype) || att->attrmult <= 1)
                { request_error(cdp, BAD_MODIF);  goto err_goto; }
              varstart=*(uns32*)request;  request+=sizeof(uns32);
              varsize =*(uns32*)request;  request+=sizeof(uns32);
              request++;   /* MOD_STOP not checked */
              if (tb_write_ind_var(cdp, tbdf, recnum, attr, index, varstart, varsize, request)) goto err_goto;
              request+=varsize;
            }
            else if (*request==MODLEN)
            { request+=2;   /* MOD_STOP not checked */
              varstart=*(uns32*)request;  request+=sizeof(uns32);
              if (tb_write_ind_len(cdp, tbdf, recnum, attr, index, varstart)) goto err_goto;
            }
            else { request_error(cdp,BAD_MODIF);  goto err_goto; }
            break;
          case MODINT:
            if (!IS_HEAP_TYPE(att->attrtype) && !IS_EXTLOB_TYPE(att->attrtype) || att->attrmult != 1)
              { request_error(cdp, BAD_MODIF);  goto err_goto; }
            if (att->attrtype==ATT_HIST)
              { request_error(cdp, NO_RIGHT);  goto err_goto; }
            varstart=*(uns32*)request;  request+=sizeof(uns32);
            varsize =*(uns32*)request;  request+=sizeof(uns32);
            request++;   /* MOD_STOP not checked */
            write_one_column(cdp, tbdf, recnum, attr, request, varstart, varsize);
            request+=varsize;
#ifdef STOP
            tbdf->prepare_trigger_info(cdp);  // must be before the constructor of t_tab_atr_upd_trig!
            t_tab_atr_upd_trig trg(tbdf, attr);
            if (trg.any_trigger()) 
            { if (trg.pre(cdp, recnum, request, varsize)) goto err_goto;
              if (trg.updated_val!=NULL)
              { t_value * val = (t_value*)trg.updated_val;  tptr p = val->indir.val;
                if (tb_write_var(cdp, tbdf, recnum, attr, varstart, val->length, &p, STD_OPT)) goto err_goto;
                request+=varsize;
              }
              else if (tb_write_var(cdp, tbdf, recnum, attr, varstart, varsize, &request, STD_OPT)) goto err_goto;
              if (trg.post(cdp, recnum)) goto err_goto;
            }
            else if (tb_write_var(cdp, tbdf, recnum, attr, varstart, varsize, &request, STD_OPT)) goto err_goto;
#endif
            break;
          case MODLEN:
            request++;   /* MOD_STOP not checked */
            if (att->attrmult != 1) /* #of multiattr. values */
            { index=*(t_mult_size*)request;  request+=sizeof(t_mult_size);
              if (tb_write_ind_num(cdp, tbdf, recnum, attr, index)) goto err_goto;
            }
            else if (IS_HEAP_TYPE(att->attrtype) || IS_EXTLOB_TYPE(att->attrtype))
            { varstart=*(uns32*)request;  request+=sizeof(uns32);
              if (tb_write_atr_len(cdp, tbdf, recnum, attr, varstart)) goto err_goto;
            }
            else { request_error(cdp, BAD_MODIF);  goto err_goto; }
            break;
          default:
            request_error(cdp, BAD_MODIF);  goto err_goto;
        } // switch
      } // WRITE
      if (*(request++) != (char)DATA_END_MARK) request_error(cdp, BAD_DATA_SIZE);
      break;
    }

    case OP_READ: /*3*/
    { uns16 index;  uns32 varstart, varsize;  t_privils_flex priv_val;
      if (*request==MODPTR || *request==MODINDPTR)
      { if (ptrsteps(cdp, &tbdf, &tbnum, &recnum, &attr, &request)) break;
        std=TRUE;
      }

     /************* checking access rights: ************************/
      if (std)  // table rights
      { cdp->prvs.get_effective_privils(tbdf, recnum, priv_val);
        if (attr && !priv_val.has_read(attr))
          if (!(tbnum==OBJ_TABLENUM && is_apl(cdp, tbnum, (tobjnum)recnum)) &&
              !(tbnum==SRV_TABLENUM && recnum==0)) // used when logging!
          { t_exkont_table ekt(cdp, tbdf->tbnum, recnum, attr, NOINDEX, OP_READ, NULL, 0);
            request_error(cdp, NO_RIGHT);  goto err_goto; 
          }
      }
      else
      { if (attracc==NULL) 
        { if (!cd->insensitive)  /* check cursor access rights, unless checked when filling own table */
            if (!(cd->d_curs->attribs[curselem].a_flags & RI_SELECT_FLAG))
              if (!(cd->d_curs->tabdef_flags & TFL_REC_PRIVILS))
                { request_error(cdp, NO_RIGHT);  goto err_goto; }
              else
              { cdp->prvs.get_effective_privils(tbdf, recnum, priv_val); // tbdf defined only is attracc==NULL
                if (!priv_val.has_read(attr))
                { t_exkont_table ekt(cdp, tbdf->tbnum, recnum, attr, NOINDEX, OP_READ, NULL, 0);
                  request_error(cdp, NO_RIGHT);  goto err_goto; 
                }
              }
        }
        else // attracc!=NULL  -- not indexed
        { exprval.set_null();
          prof_time start;
          if (cd->source_copy) start = get_prof_time();
          { t_temporary_current_cursor tcc(cdp, cd);  // sets the temporary current_cursor in the cdp until destructed
            expr_evaluate(cdp, attracc->expr.eval, &exprval);
          }
          exprval.load(cdp);
          if (exprval.is_null()) exprval.length=0;
          if (PROFILING_ON(cdp) && cd->source_copy) add_prof_time(get_prof_time()-start, PROFCLASS_SELECT, 0, 0, cd->source_copy);
         // write the calculated value to the output: 
          uns8 tp=attracc->type;  //multi=attracc->multi;
          if (*request==MODINT)  // Indexed in bytes here, not chars. Is it OK?        
          { request++;
            varstart=*(uns32*)request;  request+=sizeof(uns32);
            varsize =*(uns32*)request;  request+=sizeof(uns32);
            request++;   /* MODSTOP not checked */
            if (varstart > (uns32)exprval.length) varstart = exprval.length;
            if (varsize > exprval.length-varstart) varsize = exprval.length-varstart;
            tptr p = cdp->get_ans_ptr(sizeof(uns32)+varsize);
            if (p)
              { *(uns32*)p = varsize;  memcpy(p+sizeof(uns32), exprval.valptr()+varstart, varsize); }
          }
          else if (*request==MODLEN)          
          { request+=2;   /* MODSTOP not checked */
            tptr p=cdp->get_ans_ptr(sizeof(uns32)+sizeof(uns32));
            *(uns32*)p=sizeof(uns32);  *(uns32*)(p+sizeof(uns32)) = exprval.length;  // returns length in bytes, not chars. Changed.
          }
          else
          { request++;
            if (IS_STRING(tp))
            { int sz = (attracc->specif.length > MAX_FIXED_STRING_LENGTH ? MAX_FIXED_STRING_LENGTH+1 : attracc->specif.length+1);
              tptr p = cdp->get_ans_ptr(sizeof(uns32)+sz);
              if (p)
              { *(uns32*)p = sz;  p+=sizeof(uns32);
                if (exprval.is_null()) p[0]=p[1]=0;
                else if (attracc->specif.wide_char)
                { int len=2*(int)wuclen(exprval.wchptr());
                  if (len>sz) memcpy(p, exprval.valptr(), sz);
                  else wuccpy((wuchar*)p, exprval.wchptr());
                }
                else
                  strmaxcpy(p, exprval.valptr(), sz);
              }
            }
            else
            { int sz = tp==ATT_BINARY ? attracc->specif.length : tpsize[tp];
              tptr p = cdp->get_ans_ptr(sizeof(uns32)+sz);
              if (p)
              { *(uns32*)p = sz;  p+=sizeof(uns32);
                if (exprval.is_null()) qset_null(p, tp, sz);
                else memcpy(p, exprval.valptr(), sz);
              }
            }
          }
          break;
        }
      }

      switch (*request++)
      { case MODSTOP:
         nullres:
          if (recnum==OUTER_JOIN_NULL_POSITION)
          { attribdef * att = tbdf->attrs + attr;
            int attsz=typesize(att);
            if (IS_STRING(att->attrtype)) attsz++;
            tptr p = cdp->get_ans_ptr(sizeof(uns32)+attsz);
            *(uns32*)p=attsz;
            qset_null(p+sizeof(uns32), att->attrtype, att->attrspecif.length);
            break;
          }
          tb_read_atr(cdp, tbdf, recnum, attr, NULL);
          break;
        case MODIND:
          index=*(uns16*)request;  request+=sizeof(uns16);
          if (*request==MODSTOP)
          { request++;
            if (recnum==OUTER_JOIN_NULL_POSITION) goto nullres;
            tb_read_ind(cdp, tbdf, recnum, attr, index, NULL);
          }
          else if (*request==MODINT)
          { request++;
            varstart=*(uns32*)request;  request+=sizeof(uns32);
            varsize =*(uns32*)request;  request+=sizeof(uns32);
            request++;   /* MODSTOP not checked */
            if (recnum==OUTER_JOIN_NULL_POSITION) goto nulllong;
            tb_read_ind_var(cdp, tbdf, recnum, attr, index, varstart, varsize, NULL);
          }
          else if (*request==MODLEN)
          { request+=2;   /* MOD_STOP not checked */
            tptr p=cdp->get_ans_ptr(sizeof(uns32)+sizeof(uns32));
            *(uns32*)p=sizeof(uns32);  p+=sizeof(uns32);
            if (recnum==OUTER_JOIN_NULL_POSITION) { *(uns32*)p=0; break; }
            tb_read_ind_len(cdp, tbdf, recnum, attr, index, (uns32*)p);
          }
          else request_error(cdp, BAD_MODIF);
          break;
        case MODINT:
          varstart=*(uns32*)request;  request+=sizeof(uns32);
          varsize =*(uns32*)request;  request+=sizeof(uns32);
          request++;   /* MODSTOP not checked */
         nulllong:
          if (recnum==OUTER_JOIN_NULL_POSITION)
          { *(uns32*)cdp->get_ans_ptr(sizeof(uns32)) = 0;
            break;
          }
          tb_read_var(cdp, tbdf, recnum, attr, varstart, varsize, NULL);
          break;
        case MODLEN:
          request++;   /* MODSTOP not checked */
          if (tbdf->attrs[attr].attrmult != 1) /* #of multiattr. values */
          { tptr p=cdp->get_ans_ptr(sizeof(uns32)+sizeof(uns16));
            *(uns32*)p=sizeof(uns16);  p+=sizeof(uns32);
            if (recnum==OUTER_JOIN_NULL_POSITION) *(uns16*)p=0;
            else tb_read_ind_num(cdp, tbdf, recnum, attr, (uns16*)p);
          }
          else if (IS_HEAP_TYPE(tbdf->attrs[attr].attrtype) || IS_EXTLOB_TYPE(tbdf->attrs[attr].attrtype))
          { tptr p=cdp->get_ans_ptr(sizeof(uns32)+sizeof(uns32));
#ifndef __ia64__
            *(uns32*)p=sizeof(uns32);
#else
    	      const int four=sizeof(uns32);
	          memcpy(p, &four, sizeof(uns32));
#endif
      	    p+=sizeof(uns32);
            if (recnum==OUTER_JOIN_NULL_POSITION) *(uns32*)p=0;
            else tb_read_atr_len(cdp, tbdf, recnum, attr, (uns32*)p);
          }
          else request_error(cdp, BAD_MODIF);
          break;
        default:
          request_error(cdp, BAD_MODIF);  break;
      }
      if (cdp->get_return_code()==WORKING_WITH_EMPTY_RECORD)
        { request_error(cdp, EMPTY);  break; }
      break;
    }

    case OP_PTRSTEPS: /*3*/
    { tptr p, ptr;  uns16 index;  char buf[3+sizeof(tattrib)+1];
      if (attracc!=NULL)   // non-editable (calculated) attribute
        { SET_ERR_MSG(cdp, attracc->name);  request_error(cdp, COLUMN_NOT_EDITABLE);  break; }
      index=*(uns16*)request;  request+=sizeof(uns16);
      if (index==NOINDEX)
      { buf[0]=MODPTR;                             *(tattrib*)(buf+1)=0;  buf[1+sizeof(tattrib)]=MODSTOP; }
      else
      { buf[0]=MODINDPTR; *(uns16*)(buf+1)=index;  *(tattrib*)(buf+3)=0;  buf[3+sizeof(tattrib)]=MODSTOP; }
      ptr=buf;
      if (ptrsteps(cdp, &tbdf, &tbnum, &recnum, &attr, &ptr)) break;
      p=cdp->get_ans_ptr(sizeof(ttablenum)+sizeof(trecnum));
      *(ttablenum*)p=tbnum;
      *(trecnum*)(p+sizeof(ttablenum))=recnum;
      break;
    }

    case OP_OPEN_SUBCURSOR: /*1*/
    { tptr def;  BOOL testing = *request==(char)QUERY_TEST_MARK;
      if (testing) request++;
      if (std)
      { char tabname[2*(2+OBJ_NAME_LEN)+2];  WBUUID table_uuid;  tptr p=tabname;
        tb_read_atr(cdp, tabtab_descr, tbnum, APPL_ID_ATR, (tptr)table_uuid);
        if (memcmp(table_uuid, cdp->sel_appl_uuid, UUID_SIZE))
        { *p='`'; 
          if (!ker_apl_id2name(cdp, table_uuid, p+1, NULL))
          { strcat(p, "`.");
            p+=strlen(tabname);
          }
        }
        *p='`';
        tb_read_atr(cdp, tabtab_descr, tbnum, OBJ_NAME_ATR, p+1);
        strcat(p, "`");
        def=assemble_subcursor_def(request, tabname, TRUE);
      }
      else def=assemble_subcursor_def(request, cd->source_copy, FALSE);
      if (!def) { request_error(cdp, OUT_OF_KERNEL_MEMORY);  break; }

      if (testing)
      { sql_statement * so = sql_submitted_comp(cdp, def); // no DPs supposed
        if (so != NULL)
        { *(tcursnum*)cdp->get_ans_ptr(sizeof(tcursnum)) = (tcursnum)0xfffe;  /* must differ from NOOBJECT */
          delete so;
        } // else error raised, result not checked
      }
      else
      { tcursnum crnm2;  cur_descr * cd2;
        t_exkont_sqlstmt ekt(cdp, cdp->sel_stmt, def);
        cdp->cnt_sqlstmts++;
        if (!sql_open_cursor(cdp, def, crnm2, &cd2, true))
          if (!std)
            { cd2->subcurs=wbtrue;  cd2->super=cursnum; }
        *(uns32*)cdp->get_ans_ptr(sizeof(uns32))=crnm2;
      }
      corefree(def);
      request+=strlen(request)+1;
      break;
    }
    case OP_OPEN_CURSOR:    /*0*/
    { cursnum=*(tobjnum*)request;  request+=sizeof(tobjnum);
     // access embedded vars in db cursor def.
      cdp->sel_stmt=cdp->get_statement(0);
      cur_descr * cd;
      cdp->cnt_sqlstmts++;
      if (IS_CURSOR_NUM(cursnum))   /* re-create the cursor */
      { crnm=GET_CURSOR_NUM(cursnum);
        cd = get_cursor(cdp, crnm);
        if (!cd) goto err_goto;        
        tcursnum crnm2;  cur_descr * cd2;
        if (!sql_open_cursor(cdp, cd->source_copy, crnm2, &cd2, true))
        { cd2->persistent=cd->persistent;
         // replace and close the original cursor: 
          { ProtectedByCriticalSection cs(&crs_sem, cdp, WAIT_CS_CURSORS);
            *crs.acc0(crnm) = cd2;  // new cursor gets the original num
            *crs.acc0(GET_CURSOR_NUM(crnm2)) = cd;  // original cursor stored under the new number
          }
          free_cursor(cdp, crnm2);  // the original cursor closed and the new number removed from the log
        }
      }
      else crnm=open_stored_cursor(cdp, cursnum, &cd, true);
      *(tcursnum*)cdp->get_ans_ptr(sizeof(tcursnum)) = CURS_USER | crnm;
      break;
    }
    case OP_CLOSE_CURSOR: /*1*/
      if (!std) free_cursor(cdp, crnm);
      else request_error(cdp, ERROR_IN_FUNCTION_ARG);
      break;

    case OP_INDEX:  /*0*/
      tbnum=cursnum=*(ttablenum*)request;  request+=sizeof(ttablenum);
      switch (*(request++))
      { case OP_OP_UNINST: /* must not call try_uninst_table for tbnum<0! */
          if (tbnum==NOOBJECT) uninst_tables(cdp);
          else if (tbnum>=0) try_uninst_table(cdp, tbnum);
          break;
        case OP_OP_FREE:
          if      (tbnum== TAB_TABLENUM) free_deleted(cdp, tabtab_descr);
          else if (tbnum== OBJ_TABLENUM) free_deleted(cdp, objtab_descr);
          else if (tbnum==USER_TABLENUM) free_deleted(cdp, usertab_descr);
          else
          { table_descr_auto tbdf(cdp, tbnum);
            if (!tbdf->me()) goto err_goto;
            free_deleted(cdp, tbdf->me());
          }
          break;
        case OP_OP_DELHIST:
        { table_descr_auto tbdf(cdp, tbnum);
          if (!tbdf->me()) goto err_goto;
          delete_history(cdp, tbdf->me(), (BOOL)*(wbbool*)request, *(timestamp*)(request+sizeof(wbbool)));
          request+=sizeof(wbbool)+sizeof(timestamp);  break;
        }
        case OP_OP_COMPAT:
        { unsigned size;  tcateg categ=*(request++);
         // access embedded vars in db cursor def.
          cdp->sel_stmt=cdp->get_statement(0);
         // create descriptor:
          d_table * tdd=kernel_get_descr(cdp, categ, tbnum, &size);
          //cdp->sel_stmt=NULL; -- used when completing a cursor 
          if (tdd==NULL) break;  // error reported
          if (categ==CATEG_TABLE)
          { table_descr_auto tbdf(cdp, tbnum);
            if (tbdf->me())
            { t_privils_flex priv_val;  d_attr * att;  int i;
              cdp->prvs.get_effective_privils(tbdf->me(), NORECNUM, priv_val);
              for (i=1, att=tdd->attribs+1;  i<tdd->attrcnt;  i++, att++)
              { if (priv_val.has_read(i))// || (tdd->tabdef_flags & TFL_REC_PRIVILS))
                  att->a_flags|=RI_SELECT_FLAG | RI_REFERS_FLAG;
                if (priv_val.has_write(i))// || (tdd->tabdef_flags & TFL_REC_PRIVILS))
                  att->a_flags|=RI_INSERT_FLAG | RI_UPDATE_FLAG;
              }
            }
          }
          tptr p=cdp->get_ans_ptr(sizeof(uns32)+size);
          if (p==NULL) { corefree(tdd);  break; }
          *(uns32*)p=size;  memcpy(p+sizeof(uns32), tdd, size);
          corefree(tdd);
          break;
        }
        case OP_OP_ENABLE:
        { table_descr_auto tbdf(cdp, tbnum);
          if (!tbdf->me()) goto err_goto;
          enable_index(cdp, tbdf->me(), (int)*(sig8*)request, *(bool*)(request+1));
          request+=1+sizeof(wbbool);  break;
        }
        case OP_OP_COMPARE:
        { ttablenum tbnum2=*(ttablenum*)request;  request+=sizeof(ttablenum);
          int * pans=(int*)cdp->get_ans_ptr(3*sizeof(int));
          if (!IS_CURSOR_NUM(tbnum)) 
            if (install_table(cdp, tbnum)==NULL) break;
          if (!IS_CURSOR_NUM(tbnum2)) 
            if (install_table(cdp, tbnum2)==NULL)
            { if (!IS_CURSOR_NUM(tbnum)) unlock_tabdescr(tbnum);
              break;
            }
          compare_tables(cdp, tbnum, tbnum2, (trecnum*)pans, pans+1, pans+2);
          if (!IS_CURSOR_NUM(tbnum )) unlock_tabdescr(tbnum);
          if (!IS_CURSOR_NUM(tbnum2)) unlock_tabdescr(tbnum2);
          break;
        }
        case OP_OP_COMPACT:
        { table_descr_auto tbdf(cdp, tbnum);
          if (tbdf->me() == NULL) break;
          enable_index(cdp, tbdf->me(), -1, false);
          compact_table(cdp, tbdf->me());
          enable_index(cdp, tbdf->me(), -1, true);
          break;
        }
        case OP_OP_TRIGGERS: // get the list of triggers for a table
        { table_descr_auto tbdf(cdp, tbnum);  if (!tbdf->me()) goto err_goto;
          tbdf->prepare_trigger_info(cdp);
          cdp->answer.sized_answer_start();  uns32 * trigtp;
          for (int i=0;  i<tbdf->triggers.count();  i++)
          { trigtp = (uns32*)cdp->get_ans_ptr(sizeof(uns32));  if (!trigtp) break;  // no memory error
            *trigtp=tbdf->triggers.acc0(i)->trigger_type;
            tptr p=cdp->get_ans_ptr(sizeof(tobjname));  if (!p) break;
            tb_read_atr(cdp, objtab_descr, tbdf->triggers.acc0(i)->trigger_objnum, OBJ_NAME_ATR, p); 
          }
          trigtp = (uns32*)cdp->get_ans_ptr(sizeof(uns32));  
          if (trigtp) *trigtp=0; // the delimiter
          cdp->answer.sized_answer_stop();
          break;
        }
        case OP_OP_MAKE_PERSIS:  // marks a cursor as "persitent"
          if (!IS_CURSOR_NUM(cursnum)) { request_error(cdp, ERROR_IN_FUNCTION_ARG);  goto err_goto; }
          cd = get_cursor(cdp, cursnum);
          if (!cd) goto err_goto;
          cd->persistent=true;
          break;
        case OP_OP_TRUNCATE:
          { table_descr_auto tbdf(cdp, tbnum);
            if (!tbdf->me()) goto err_goto;
            truncate_table(cdp, tbdf->me());
          }
          break;
        case OP_OP_ADMIN_MODE:
        { uns8 mode = *request++;
          WBUUID uuid;  tcateg categ;  BOOL result;
          if (tb_read_atr(cdp, objtab_descr, tbnum, APPL_ID_ATR,   (tptr)uuid) ||
              tb_read_atr(cdp, objtab_descr, tbnum, OBJ_CATEG_ATR, (tptr)&categ)) goto err_goto;
          if (categ!=CATEG_CURSOR && categ!=CATEG_PROC) { request_error(cdp, ERROR_IN_FUNCTION_ARG);  goto err_goto; }
          if (mode && mode!=1)  // only asking
            result = is_in_admin_mode(cdp, tbnum, uuid);
          else // setting 
            if (!cdp->prvs.am_I_appl_author(cdp, uuid))  // test author role:
              { request_error(cdp, NO_RIGHT);  goto err_goto; }
          else if (mode) // set or reset the admin mode
            result = set_admin_mode(cdp, tbnum, uuid);
          else
            result = clear_admin_mode(cdp, tbnum, uuid);
          *(uns32*)cdp->get_ans_ptr(sizeof(uns32)) = result;
          break;
        }
        default:
          request_error(cdp, BAD_OPCODE);  break;
      } /* OP_OP_ switch */
      break;
    case OP_TRANSF: /*0*/
    { ttablenum t1, t2;  trecnum r1, r2;  tattrib a1, a2;  uns16 i1, i2;
     // build attribute access:
      t1=*(ttablenum*)(request);
      r1=*(trecnum*)  (request+  sizeof(ttablenum));
      a1=*(tattrib*)  (request+  sizeof(ttablenum)+  sizeof(trecnum));
      i1=*(uns16*)    (request+  sizeof(ttablenum)+  sizeof(trecnum)+  sizeof(tattrib));
      t2=*(ttablenum*)(request+  sizeof(ttablenum)+  sizeof(trecnum)+  sizeof(tattrib)+sizeof(uns16));
      r2=*(trecnum*)  (request+2*sizeof(ttablenum)+  sizeof(trecnum)+  sizeof(tattrib)+sizeof(uns16));
      a2=*(tattrib*)  (request+2*sizeof(ttablenum)+2*sizeof(trecnum)+  sizeof(tattrib)+sizeof(uns16));
      i2=*(uns16*)    (request+2*sizeof(ttablenum)+2*sizeof(trecnum)+2*sizeof(tattrib)+sizeof(uns16));
      request+=2*(sizeof(ttablenum)+sizeof(trecnum)+sizeof(tattrib)+sizeof(uns16));
      if (IS_CURSOR_NUM(t1)) 
      { if (map_cur2tab(cdp, &t1, &r1, &a1, attracc)) break;
        if (attracc!=NULL)   // non-editable (calculated) attribute
          { SET_ERR_MSG(cdp, attracc->name);  request_error(cdp, COLUMN_NOT_EDITABLE);  break; }
      }
      if (IS_CURSOR_NUM(t2)) 
      { if (map_cur2tab(cdp, &t2, &r2, &a2, attracc)) break; }
      else attracc=NULL;
     // check privils & exec:
      table_descr_auto tbdf1(cdp, t1), tbdf2(cdp, t2);  t_privils_flex priv_val;
      if (tbdf1->me() && tbdf2->me())
      { cdp->prvs.get_effective_privils(tbdf1->me(), r1, priv_val);
        if (a1 ? !priv_val.has_write(a1) : !(*priv_val.the_map() & RIGHT_DEL))
          request_error(cdp, NO_RIGHT);
        else
        { cdp->prvs.get_effective_privils(tbdf2->me(), r2, priv_val);
          if (a2 && !priv_val.has_read(a2))
            request_error(cdp, NO_RIGHT);
          else  // can perform the operation
            copy_var_len(cdp, tbdf1->me(), r1, a1, i1, tbdf2->me(), r2, a2, i2, attracc);
        }
      }
      break;
    }
    case OP_SAVE_TABLE:  /*1*/
    { trecnum userobj;
      if (cursnum==USER_TABLENUM)
      { userobj = *(tobjnum*)request==NOOBJECT ? NORECNUM : *(tobjnum*)request;  
        request+=sizeof(tobjnum); 
      }
      else
      { userobj=NORECNUM;
        if (check_all_right(cdp, cursnum, FALSE))
          { request_error(cdp, NO_RIGHT);  break; }
      }
      os_save_table(cdp, cursnum, userobj);
      break;
    }
    case OP_REST_TABLE: /*1*/
    {/* check write and append rights: */
      uns8 flag = *(request++);
      if (!std)
      { if (cd->underlying_table==NULL)
          { request_error(cdp, CANNOT_APPEND);  break; }
        tbnum=cd->underlying_table->tabnum;  tbdf=tables[tbnum];
      }
      if (cursnum==TAB_TABLENUM || cursnum==KEY_TABLENUM)
      { if (!cdp->prvs.is_conf_admin)
          { request_error(cdp, NO_CONF_RIGHT);  break; }
        if (cursnum==TAB_TABLENUM) cursnum=USER_TABLENUM;  // replacement for storing all users
        os_load_table(cdp, cursnum);
        if (flag && cursnum==USER_TABLENUM)
          clear_all_passwords(cdp);
      }
      else
      { if (!can_insert_record(cdp, tbdf)) 
          { request_error(cdp, NO_RIGHT);  break; }
        if (cursnum==USER_TABLENUM) os_load_user(cdp);
        else
        { if (check_all_right(cdp, cursnum, TRUE))
            { request_error(cdp, NO_RIGHT);  break; }
          os_load_table(cdp, cursnum);
        }
      }
      break;
    }
    case OP_TRANSL:  /*1 - must not use 2, must not raise error on deleted records */
    { trecnum recnums[MAX_CURS_TABLES];
      if (std)                    { request_error(cdp, ERROR_IN_FUNCTION_ARG);  goto err_goto; }
      if (cd->insensitive)        { request_error(cdp, CANNOT_APPEND);  goto err_goto; }
      if (cd->curs_seek(*(trecnum*)request, recnums)) goto err_goto;
      request+=sizeof(trecnum);
      if (*request >= cd->tabcnt) { request_error(cdp, ERROR_IN_FUNCTION_ARG);  goto err_goto; }
      if (*recnums==NORECNUM)  /* deleted record in the cursor */
        recnum=NORECNUM;
      else
        recnum=recnums[*request];
      request++;
      *(trecnum*)cdp->get_ans_ptr(sizeof(trecnum))=recnum;
      break;
    }
    case OP_BASETABS:  /*1*/
    { int num = *(uns16*)request;  request+=sizeof(uns16);
      if (std)
      { *(uns16    *)cdp->get_ans_ptr(sizeof(uns16    ))=1;
        *(ttablenum*)cdp->get_ans_ptr(sizeof(ttablenum))=tbnum;
        num--;
      }
      else
      { *(uns16    *)cdp->get_ans_ptr(sizeof(uns16    ))=cd->tabcnt;
        for (unsigned i=0;  i<cd->tabcnt;  i++)
        { if (num) num--; else break;
          *(ttablenum*)cdp->get_ans_ptr(sizeof(ttablenum))=cd->tabnums[i];
        }
      }
      while (num--) *(ttablenum*)cdp->get_ans_ptr(sizeof(ttablenum))=NOOBJECT;
      break;
    }
    case OP_TABINFO:   /*1*/ /* get table statistics */
      table_info(cdp, tbdf, (trecnum*)cdp->get_ans_ptr(3*sizeof(trecnum)));
      break;
    case OP_SUPERNUM:  /*2*/
    { trecnum recnums[MAX_OPT_TABLES];
      if (std) *recnums=recnum;
      else 
#if SQL3
      { int ordnum = 0;  
        cd->qe->get_position_indep(recnums, ordnum, true); 
      }
#else
        cd->qe->get_position(recnums);
#endif
      tbnum=*(tcursnum*)request;   /* supercursor or a table */
      request+=sizeof(tcursnum);
      if (IS_CURSOR_NUM(tbnum))  /* searching in the supercursor tbnum */
      { cd=get_cursor(cdp, tbnum);
        if (cd==NULL) break;
        if (!PTRS_CURSOR(cd))
        { cdp->ker_flags |=  KFL_HIDE_ERROR;
          request_error(cdp, CANNOT_APPEND); // should not be logged, used to synchronise unrelated cursors
          cdp->ker_flags &=  ~KFL_HIDE_ERROR;  
          goto err_goto; 
        }
        make_complete(cd);
        if (!cd->pmat->cursor_search(cdp, recnums, &recnum, cd->tabcnt))
          request_error(cdp, OUT_OF_TABLE);
      }
      else   /* like Translate */
        recnum=recnums[0];   /* returns records number in the 1st table */
      *(trecnum*)cdp->get_ans_ptr(sizeof(trecnum))=recnum;
      break;
    }
    case OP_ADDREC:   /*1*/
    { if (std) { request_error(cdp, ERROR_IN_FUNCTION_ARG);  break; }    /* table supplied */
      unsigned size=*(uns16*)request;  request+=sizeof(uns16);
      if (!PTRS_CURSOR(cd)) { request_error(cdp, CANNOT_APPEND);  break; }
      if (!cd->pmat->put_recnums(cdp, (trecnum*)request))
        { request_error(cdp, CURSOR_MISUSE);  break; }
      cd->recnum++;
      request+=size*sizeof(trecnum);
      break;
    }
    case OP_TEXTSUB: /*3*/
    { if (attracc!=NULL)   // non-editable (calculated) attribute
        { SET_ERR_MSG(cdp, attracc->name);  request_error(cdp, COLUMN_NOT_EDITABLE);  break; }
      t_mult_size index = *(uns16 *)request;  request+=sizeof(uns16);
      if (tbdf->attrs[attr].attrtype != ATT_TEXT)
        { request_error(cdp, BAD_MODIF);  break; }
      if ((tbdf->attrs[attr].attrmult==1) != (index==NOINDEX))
        { request_error(cdp, BAD_MODIF);  break; }
      p=request+2*sizeof(uns16)+sizeof(uns32);
      var_search(cdp, tbdf, recnum, attr, index,
                *(uns32   *)(request),
                *(uns16   *)(request+sizeof(uns32)), charset_ansi,
                *(uns16   *)(request+sizeof(uns16)+sizeof(uns32)),
                p, (uns32*)cdp->get_ans_ptr(sizeof(uns32)));
                request=p+strlen(p)+1;
      break;
    }
    case OP_SETAPL:
    { tobjnum aplobj;
      sys_Upcase(request); // before 6.1a there was no Upcase on the client side
      ker_set_application(cdp, request, aplobj);
      request+=OBJ_NAME_LEN+1;
      memcpy(cdp->get_ans_ptr(UUID_SIZE), cdp->sel_appl_uuid, UUID_SIZE);
      *(tobjnum*)cdp->get_ans_ptr(sizeof(tobjnum)) = cdp->author_role;
      *(tobjnum*)cdp->get_ans_ptr(sizeof(tobjnum)) = cdp->admin_role;
      *(tobjnum*)cdp->get_ans_ptr(sizeof(tobjnum)) = cdp->senior_role;
      *(tobjnum*)cdp->get_ans_ptr(sizeof(tobjnum)) = cdp->junior_role;
      *(tobjnum*)cdp->get_ans_ptr(sizeof(tobjnum)) = aplobj;
      break;
    }
    case OP_REPLAY:    /*0*/
      replaying=true;
      replay(cdp, *(tobjnum *)request, *(timestamp *)(request+sizeof(tobjnum)));
      replaying=false;
      request+=sizeof(tobjnum)+sizeof(timestamp);
      break;
    case OP_GETDBTIME: /*0*/
      *(timestamp *)cdp->get_ans_ptr(sizeof(timestamp)) =
        header.closetime ? header.closetime : stamp_now();
      break;
    case OP_SUBMIT:    /*0*/
    {// store the selected statement kontext to cdp->sel_stmt:
      cdp->sel_stmt=cdp->get_statement(*(uns32*)request);  request+=sizeof(uns32);
     // skip if testing the connection speed, prevent writing to the log 
      while (*request==' ') request++;
      if (!*request)  // empty statement
      { request++;  
        *(uns32*)cdp->get_ans_ptr(sizeof(uns32)) = 0; // must generate empty answer 
        break; 
      }  
      unsigned len = (unsigned)strlen(request)+1;  /* compilation may add 0 into statement */
      if (*request==(char)(QUERY_TEST_MARK+1)) // testing global declarations
      { compil_info xCI(cdp, request+1, compile_declarations_entry);
        t_sql_kont sql_kont;  xCI.sql_kont=&sql_kont;  
        t_global_scope gssc(cdp->sel_appl_uuid);  xCI.univ_ptr=&gssc;
        compile(&xCI);
        *(uns32*)cdp->get_ans_ptr(sizeof(uns32)) = 0; // must generate empty answer 
      }
      else
      { BOOL testing = *request==(char)QUERY_TEST_MARK;
        if (testing) { request++;  len--; }

        if (register_dynpars(cdp, request)) { cdp->sel_stmt=NULL;  break; }
        sql_statement * so = sql_submitted_comp(cdp, request);
        if (testing || so==NULL) 
        { *(uns32*)cdp->get_ans_ptr(sizeof(uns32)) = 0; // must generate empty answer for the empty statement (e.g. PROCEDURE... without CREATE)
         // for some statements make further analysis/validation:
          if (so)
            if (so->statement_type==SQL_STAT_ALTER_TABLE)
            { table_all ta_old, ta_new;
              compil_info xCI(cdp, ((sql_stmt_alter_table*)so)->source, table_def_comp_link);
              xCI.univ_ptr=&ta_new;  xCI.stmt_ptr=&ta_old;
              if (!compile(&xCI)) 
              { table_descr * tbdf = make_td_from_ta(cdp, &ta_new, -1);
                if (tbdf)                                                 
                { tbdf->deflock_counter=1;
                  memcpy(tbdf->schema_uuid, cdp->sel_appl_uuid, UUID_SIZE);  // used by compile_constrains
                  compile_constrains(cdp, &ta_new, tbdf);
                  tbdf->deflock_counter=0;  tbdf->free_descr(cdp);
                }
              }
            }
            else if (so->statement_type==SQL_STAT_CREATE_TABLE)
            { table_descr * tbdf = make_td_from_ta(cdp, ((sql_stmt_create_table*)so)->ta, -1);
              if (tbdf)                                                 
              { tbdf->deflock_counter=1;
                memcpy(tbdf->schema_uuid, cdp->sel_appl_uuid, UUID_SIZE);  // used by compile_constrains
                compile_constrains(cdp, ((sql_stmt_create_table*)so)->ta, tbdf);
                tbdf->deflock_counter=0;  tbdf->free_descr(cdp);
              }
            }
        }
        else
        { if (the_sp.WriteJournal.val() &&
              (so->statement_type==SQL_STAT_ALTER_TABLE || so->statement_type==SQL_STAT_CREATE_TABLE ||
               so->statement_type==SQL_STAT_DROP ||
               so->statement_type==SQL_STAT_CREATE_INDEX|| so->statement_type==SQL_STAT_DROP_INDEX))
          /* must be before sql_exec, because it contains a commit and the following commit will not write into journal as it does not see any changes */
          { worm_add(cdp, &cdp->jourworm, &submit_opcode, 1);
            worm_add(cdp, &cdp->jourworm, cdp->sel_appl_name, sizeof(tobjname));
            worm_add(cdp, &cdp->jourworm, cdp->sel_appl_uuid, UUID_SIZE);
            worm_add(cdp, &cdp->jourworm, request, len);
          }
         // trace and execute:
          t_exkont_sqlstmt ekt(cdp, cdp->sel_stmt, request);
          cdp->cnt_sqlstmts++;
          if (cdp->trace_enabled & TRACE_SQL) 
          { char buf[81];
            trace_msg_general(cdp, TRACE_SQL, form_message(buf, sizeof(buf), msg_trace_sql), NOOBJECT); // SQL statement source text is in the kontext
          }
         // prepare measuring the execution time (for profiling and reporting the slow statements): 
          prof_time start = get_prof_time();  // important to do it always: the procedure may start profiling!
          cdp->lower_level_time = 0;  cdp->last_line_num = (uns32)-1;
          prof_time orig_lock_waiting_time = cdp->lock_waiting_time;
          sql_exec_top_list_res2client(cdp, so);
          if (PROFILING_ON(cdp)) 
            add_hit_and_time2(get_prof_time()-start, cdp->lower_level_time, cdp->lock_waiting_time-orig_lock_waiting_time, PROFCLASS_SQL, 0, 0, request);
          if (the_sp.report_slow.val())
          { prof_time netto_exec_time = (get_prof_time()-start) - (cdp->lock_waiting_time-orig_lock_waiting_time);
            unsigned netto_exec_time10 = (unsigned)(10*netto_exec_time / get_prof_koef()); // converted to tenths of second to miliseconds
            if (netto_exec_time10 >= the_sp.report_slow.val())  
              report_slow_statement(cdp, request, netto_exec_time10);
          }  
        }
        if (so) delete so;
        //cdp->sel_stmt=NULL; -- used when completing a cursor 
      }
      request+=len;
      break;
    }
    case OP_SENDPAR:   /*0*/
    {/* search statement stream: */
      uns32 hstmt  = *(uns32*)request;  request+=sizeof(uns32);
      sig16 parnum = *(sig16*)request;  
     // deleting and creation:
      if (parnum==SEND_PAR_DROP_STMT)
      { request+=sizeof(sig16);  cdp->drop_statement(hstmt);
        break;
      }
      wbstmt * sstmt=cdp->get_statement(hstmt);
      if (sstmt==NULL)  // create the statement, if it does not exist
      { sstmt=new wbstmt(cdp, hstmt);
        if (sstmt==NULL) { request_error(cdp, OUT_OF_KERNEL_MEMORY);  break; }
      }
     // operations on sstmt:
      switch (parnum)
      { case SEND_PAR_CREATE_STMT:
          request+=sizeof(sig16);  break; // just created
        case SEND_PAR_DROP:
          request+=sizeof(sig16);  free_dpars(sstmt);  break;
        case SEND_PAR_NAME: // assigns cursor number to the statement and name to the cursor
        { t_op_send_par_name * pars = (t_op_send_par_name*)request;  request+=pars->size();
          cur_descr * cd = get_cursor(cdp, pars->cursnum);
          if (cd!=NULL) 
          { assign_name_to_cursor(cdp, pars->name, cd);
            sstmt->cursor_num=pars->cursnum;
          }
          break;
        }
        case SEND_PAR_POS: // positions the cursor associated with the statement
        { t_op_send_par_pos * pars = (t_op_send_par_pos*)request;  request+=sizeof(t_op_send_par_pos);
          if (sstmt->cursor_num!=NOCURSOR)
          { cur_descr * cd = get_cursor(cdp, sstmt->cursor_num);
            if (cd!=NULL) cd->position=pars->recnum;
          }
          break;
        }
        case SEND_PAR_PAR_INFO: // returns description of all DPS from markers
        { request+=sizeof(sig16);
          int size = sstmt->markers.count() * sizeof(MARKER);
          tptr p=cdp->get_ans_ptr(sizeof(uns32)+sizeof(int)+size);
          if (p==NULL) break; 
          *(uns32*)p=size+sizeof(int);  p+=sizeof(uns32);
          *(int*)p=sstmt->markers.count();  p+=sizeof(int);
          if (size) // "if" is necesasary, otherwise phantom marker created
            memcpy(p, sstmt->markers.acc0(0), size);
          break;
        }
        case SEND_PAR_RS_INFO: // returns d_curs iff prepared statement is a cursor, nothing otherwise
        { request+=sizeof(sig16);
          uns16 rs_order = *(uns16*)request;  request+=sizeof(uns16);
          sql_statement * so = sstmt->so;
          while (rs_order && so!=NULL) so=so->next_statement;
          d_table * td = NULL;
          if (so!=NULL)
            if (so->statement_type==SQL_STAT_SELECT)    // direct SELECT
            { sql_stmt_select * selso = (sql_stmt_select*)so;  
              if (!selso->is_into) 
              { td = create_d_curs(cdp, selso->qe);
                if (td==NULL) { request_error(cdp, OUT_OF_KERNEL_MEMORY);  break; }
              }
            }
            else if (so->statement_type==SQL_STAT_CALL) // SELECT in procedure, used by MS ODBC
            { t_routine * routine = ((sql_stmt_call*)so)->get_called_routine(cdp, NULL, TRUE);
              if (routine)
              { if (routine->stmt)
                { sql_stmt_select * selso = NULL;  
                  if (routine->stmt->statement_type==SQL_STAT_SELECT)
                    selso = (sql_stmt_select*)routine->stmt;
                  else if (routine->stmt->statement_type==SQL_STAT_BLOCK)
                  { sql_stmt_block * blso = (sql_stmt_block*)routine->stmt;
                    if (blso->body)
                      if (blso->body->statement_type==SQL_STAT_SELECT)
                        selso = (sql_stmt_select*)blso->body;
                      else if (blso->body->statement_type==SQL_STAT_NO && blso->body->next_statement && blso->body->next_statement->statement_type==SQL_STAT_SELECT)
                        selso = (sql_stmt_select*)blso->body->next_statement;
                  }
                  if (selso && !selso->is_into) 
                  { td = create_d_curs(cdp, selso->qe);
                    if (td==NULL) { request_error(cdp, OUT_OF_KERNEL_MEMORY);  break; }
                  }
                }
                ((sql_stmt_call*)so)->release_called_routine(routine);
              }
            }
          if (td)
          { int size = d_curs_size(td);
            tptr p=cdp->get_ans_ptr(sizeof(uns32)+size);
            if (p==NULL) { corefree(td);  break; }
            *(uns32*)p=size;
            memcpy(p+sizeof(uns32), td, size);
            corefree(td);
          }
          else
            *(uns32*)cdp->get_ans_ptr(sizeof(uns32)) = 0;  // no result set
          break;
        }
        case SEND_PAR_PULL:  // copies selected parameter values to ans
        { request+=sizeof(sig16);
          WBPARAM * wbpar;  MARKER * marker;  int i;
         // calculate the answer size:
          int totsize = sizeof(sig16);  // delimiter space
          for (i=0;  i<sstmt->markers.count();  i++)
          { marker = sstmt->markers.acc0(i);
            wbpar=sstmt->dparams.acc(i);
            if (wbpar==NULL) continue;
            if (marker->input_output & MODE_OUT)
            { totsize+=sizeof(sig16)+sizeof(uns32);
              if (wbpar->len<=256) totsize+=wbpar->len;
            }
          }
         // write size and the parameter values:
          *(uns32*)cdp->get_ans_ptr(sizeof(uns32)) = totsize;
          for (i=0;  i<sstmt->markers.count();  i++)
          { marker = sstmt->markers.acc0(i);
            wbpar=sstmt->dparams.acc(i);
            if (wbpar==NULL) continue;
            if (marker->input_output & MODE_OUT)
            { p = cdp->get_ans_ptr(sizeof(sig16)+sizeof(uns32));
              if (p==NULL) { request_error(cdp, OUT_OF_KERNEL_MEMORY);  goto err_goto; }
              *(sig16*)p=i;       p+=sizeof(sig16);
              *(uns32*)p=wbpar->len;  //p+=sizeof(uns32);
              if (wbpar->len<=256) // condition added in 7.0f
              { p = cdp->get_ans_ptr(wbpar->len);
                if (p==NULL) { request_error(cdp, OUT_OF_KERNEL_MEMORY);  goto err_goto; }
                memcpy(p, wbpar->val, wbpar->len);
              }
            }
          }
          *(sig16*)cdp->get_ans_ptr(sizeof(sig16)) = SEND_PAR_DELIMITER;
          break;
        }
        case SEND_PAR_PULL_1:
        { request+=sizeof(sig16);
          uns16 num;  uns32 start, size;  WBPARAM * wbpar;  MARKER * marker;
          num = *(uns16*)request;  request+=sizeof(uns16);
          start = *(uns32*)request;  request+=sizeof(uns32);
          size  = *(uns32*)request;  request+=sizeof(uns32);
          if (num>=sstmt->markers.count() || num>=sstmt->dparams.count())
            { request_error(cdp, ERROR_IN_FUNCTION_ARG);  goto err_goto; }
          marker = sstmt->markers.acc0(num);
          wbpar  = sstmt->dparams.acc0(num);
          tptr p=cdp->get_ans_ptr(size);
          if (start>=wbpar->len) size=0;
          else if (start+size>wbpar->len) size=wbpar->len-start;
          memcpy(p, wbpar->val+start, size);
          break;
        }
        case SEND_EMB_PULL:  // copies all parameter values to ans
        { request+=sizeof(sig16);
         // calculate the answer size:
          int par, totsize = 0;
          for (par=0;  par<sstmt->eparams.count();  par++)
          { t_emparam * empar=sstmt->eparams.acc0(par);
            if (empar->mode==MODE_OUT || empar->mode==MODE_INOUT)
              if (IS_HEAP_TYPE(empar->type) || IS_EXTLOB_TYPE(empar->type))
              { unsigned vallen = empar->length;
                if (vallen>SEND_PAR_COMMON_LIMIT) vallen=SEND_PAR_COMMON_LIMIT;
                totsize += sizeof(uns32)+vallen;
              }
              else
                totsize += empar->length;
          }
         // write size and the parameter values:
          *(uns32*)cdp->get_ans_ptr(sizeof(uns32)) = totsize;
          for (par=0;  par<sstmt->eparams.count();  par++)
          { t_emparam * empar=sstmt->eparams.acc0(par);
            if (empar->mode==MODE_OUT || empar->mode==MODE_INOUT)
              if (IS_HEAP_TYPE(empar->type) || IS_EXTLOB_TYPE(empar->type))
              { unsigned vallen = empar->length;
                if (vallen>SEND_PAR_COMMON_LIMIT) vallen=SEND_PAR_COMMON_LIMIT;
                p = cdp->get_ans_ptr(sizeof(uns32)+vallen);
                if (p==NULL) { request_error(cdp, OUT_OF_KERNEL_MEMORY);  goto err_goto; }
                *(uns32*)p = vallen;  p+=sizeof(uns32);
                memcpy(p, empar->val, vallen);
              }
              else
              { p = cdp->get_ans_ptr(empar->length);
                if (p==NULL) { request_error(cdp, OUT_OF_KERNEL_MEMORY);  goto err_goto; }
                memcpy(p, empar->val, empar->length);
              }
          }
          //free_epars(sstmt);  // clear the embedded parametrs (otherwise they may be used in the next statement without emb. vars)
          // -- no, used when completing a cursor 
          break;
        }
        case SEND_EMB_PULL_EXT:
        { request+=sizeof(sig16);
          t_emparam * empar = sstmt->find_eparam_by_name(request);  
          if (!empar) { SET_ERR_MSG(cdp, request);  request_error(cdp, OBJECT_NOT_FOUND);  goto err_goto; }
          request+=sizeof(tname);
          uns32 offset    = *(uns32*)request;  request+=sizeof(uns32);
          unsigned partsize = empar->length>offset ? empar->length-offset : 0;
          //if (partsize > SEND_PAR_PART_SIZE) partsize = SEND_PAR_PART_SIZE;  // not dividing is faster
          *(uns32*)cdp->get_ans_ptr(sizeof(uns32)) = sizeof(uns32)+partsize;
          *(uns32*)cdp->get_ans_ptr(sizeof(uns32)) = partsize;
          tptr p = cdp->get_ans_ptr(partsize);
          if (p==NULL) { request_error(cdp, OUT_OF_KERNEL_MEMORY);  goto err_goto; }
          memcpy(p, empar->val+offset, partsize);
          break;
        }
        case SEND_PAR_EXEC_PREP:
          request+=sizeof(sig16);
          if (sstmt->so!=NULL)
          { cdp->sel_stmt=sstmt;
            t_exkont_sqlstmt ekt(cdp, sstmt, sstmt->source);
            cdp->cnt_sqlstmts++;
            if (cdp->trace_enabled & TRACE_SQL) 
            { char buf[81];
              trace_msg_general(cdp, TRACE_SQL, form_message(buf, sizeof(buf), msg_trace_sql), NOOBJECT); // SQl statement source text is in the kontext
            }
            prof_time start = get_prof_time();  // important when the procedure starts profiling!
            cdp->lower_level_time = 0;  cdp->last_line_num = (uns32)-1;
            prof_time orig_lock_waiting_time=cdp->lock_waiting_time;
            sql_exec_top_list_res2client(cdp, sstmt->so);
            if (PROFILING_ON(cdp)) 
              add_hit_and_time2(get_prof_time()-start, cdp->lower_level_time, cdp->lock_waiting_time-orig_lock_waiting_time, PROFCLASS_SQL, 0, 0, sstmt->source);
            if (the_sp.report_slow.val())
            { prof_time netto_exec_time = (get_prof_time()-start) - (cdp->lock_waiting_time-orig_lock_waiting_time);
              unsigned netto_exec_time10 = (unsigned)(10*netto_exec_time / get_prof_koef()); // converted to tenths of second to miliseconds
              if (netto_exec_time10 >= the_sp.report_slow.val())  
                report_slow_statement(cdp, request, netto_exec_time10);
            }  
            //cdp->sel_stmt=NULL; -- used when completing a cursor 
          }
          break;
        case SEND_PAR_PREPARE:
        { request+=sizeof(sig16);
          if (sstmt->so!=NULL) { delete sstmt->so;  sstmt->so=NULL; }
          corefree(sstmt->source);  sstmt->source=NULL;
          cdp->sel_stmt=sstmt;
          if (register_dynpars(cdp, request)) { cdp->sel_stmt=NULL;  break; }
          sstmt->so=sql_submitted_comp(cdp, request);
          int len = (int)strlen(request)+1;
          tptr p = (tptr)corealloc(len, 47);
          sstmt->source=p;
          if (p!=NULL) strcpy(p, request);
          cdp->sel_stmt=NULL;
          request+=len;
          break;
        }
        case SEND_EMB_TYPED:
        { request+=sizeof(sig16);
          free_epars(sstmt);  sig16 mode;
          mode = *(sig16*)request;  request+=sizeof(sig16);
          while (mode!=SEND_PAR_DELIMITER)
          { t_emparam * empar=sstmt->eparams.next();
            if (empar==NULL) { request_error(cdp, OUT_OF_KERNEL_MEMORY);  goto err_goto; }
            empar->mode  = (t_parmode)mode;
            strcpy(empar->name, request);             request+=sizeof(tname);
            empar->type  = *(sig16*)request;          request+=sizeof(sig16);
            empar->specif.opqval = *(uns32*)request;  request+=sizeof(uns32);
            if (mode==MODE_IN || mode==MODE_INOUT || !IS_HEAP_TYPE(empar->type)) // read length, allocate buffer
            { if (IS_HEAP_TYPE(empar->type))
                { empar->length= *(uns32*)request;  request+=sizeof(uns32); }
              else
                { empar->length= *(uns16*)request;  request+=sizeof(uns16); }
              if (empar->length)
              { empar->realloc(empar->length);
                if (empar->val==NULL) { request_error(cdp, OUT_OF_KERNEL_MEMORY);  goto err_goto; }
                if (mode==MODE_IN || mode==MODE_INOUT)
                { memcpy(empar->val, request, empar->length);
                  request+=empar->length;
                }
                else qset_null(empar->val, empar->type, empar->length);
              }
              else empar->val=NULL;  // var-len empty input value
            }
            else // var-len output value
              { empar->val=NULL;  empar->length=0; }
           // next item:
            mode         = *(sig16*)request;  request+=sizeof(sig16);
          }
          break;
        }
        case SEND_EMB_TYPED_EXT:  // moves long values per partes, thy are supposed to be stored as empty before
        { request+=sizeof(sig16);
          t_emparam * empar = sstmt->find_eparam_by_name(request);  
          if (!empar) { SET_ERR_MSG(cdp, request);  request_error(cdp, OBJECT_NOT_FOUND);  goto err_goto; }
          request+=sizeof(tname);
          uns32 totalsize = *(uns32*)request;  request+=sizeof(uns32);
          uns32 offset    = *(uns32*)request;  request+=sizeof(uns32);
          uns32 partsize  = *(uns32*)request;  request+=sizeof(uns32);
          if (!empar->val) 
          { empar->realloc(totalsize);
            if (!empar->val) { request_error(cdp, OUT_OF_KERNEL_MEMORY);  goto err_goto; }
          }
          memcpy(empar->val+offset, request, partsize);  request+=partsize;
          empar->val[offset+partsize]=0;  // may be used by CLOB?
          break;
        }

        default: /* copy parameters: */
          request+=sizeof(sig16);
          while (parnum!=SEND_PAR_DELIMITER)
          { WBPARAM * wbpar = sstmt->dparams.acc(parnum);
            if (!wbpar) { request_error(cdp, OUT_OF_KERNEL_MEMORY);  goto err_goto; }
            uns32 len = *(uns32*)request;  request+=sizeof(uns32);
            if (len==(uns32)SQL_NULL_DATA || len==(uns32)SQL_DEFAULT_PARAM) wbpar->free();
            else
            { uns32 offset=*(uns32*)request;  request+=sizeof(uns32);
              unsigned newsize=offset ? wbpar->len+len : len;
              wbpar->realloc(newsize);
              if (newsize && !wbpar->len)
                { request_error(cdp, OUT_OF_KERNEL_MEMORY);  goto err_goto; }
              memcpy(wbpar->val+offset, request, len);  request+=len;
            }
           /* next parameter: */
            parnum=*(sig16*)request;  request+=sizeof(sig16);
          }
          break; // end of the default branch
      } // switch parnum
      break;
    }
    case OP_GETERROR:  /*0*/
    { *(uns32*)cdp->get_ans_ptr(sizeof(uns32)) = cdp->comp_err_line;
      *(uns16*)cdp->get_ans_ptr(sizeof(uns16)) = cdp->comp_err_column;
      tptr p=cdp->get_ans_ptr(PRE_KONTEXT+POST_KONTEXT+2);
      if (p==NULL) { request_error(cdp, OUT_OF_KERNEL_MEMORY);  break; }
      if (cdp->comp_kontext==NULL || !*cdp->comp_kontext)
        if (cdp->current_generic_error_message)
        { strmaxcpy(p, cdp->current_generic_error_message, PRE_KONTEXT+POST_KONTEXT+2);
          break;
        }
      if (cdp->comp_kontext==NULL) *p=0; // NULL when comp_kontext allocation error occured
      else strcpy(p, cdp->comp_kontext);
      break;
    }
    case OP_AGGR:  /*1*/
    { tptr p;  trecnum reccnt;
      tattrib attr=*(tattrib*)request;  request+=sizeof(tattrib);  symbol ag_type = *(request++);
      int agg_type, agg_size, inisize, res_type, multi;  t_specif specif;  
      BOOL is_a_string;
      if (std)
      { if (attr >= tbdf->attrcnt)
          { request_error(cdp, BAD_ELEM_NUM);  break; }
        agg_type=tbdf->attrs[attr].attrtype;
        multi   =tbdf->attrs[attr].attrmult;
        specif  =tbdf->attrs[attr].attrspecif;
        reccnt=tbdf->Recnum();
      }
      else
      { if (attr >= cd->d_curs->attrcnt)
          { request_error(cdp, BAD_ELEM_NUM);  break; }
        agg_type=cd->d_curs->attribs[attr].type;
        multi   =cd->d_curs->attribs[attr].multi;
        specif  =cd->d_curs->attribs[attr].specif;
        make_complete(cd);
        reccnt=cd->recnum;
      }
      if (multi!=1) { request_error(cdp, UNPROPER_TYPE);  break; }
     /* analyse the attribute: */
      res_type=agg_type;
      is_a_string=is_string(agg_type);
      if ((agg_type != ATT_INT16) && (agg_type != ATT_INT32) && (agg_type != ATT_INT8) && (agg_type != ATT_INT64) &&
          (agg_type != ATT_MONEY) && (agg_type != ATT_FLOAT))
        if (ag_type == C_SUM || ag_type == C_AVG || agg_type >= ATT_PTR)
        { request_error(cdp, UNPROPER_TYPE);  break; }
      if (ag_type==C_COUNT)
      { p=cdp->get_ans_ptr(sizeof(uns32)+sizeof(trecnum));
        *(uns32*)p=sizeof(recnum);  p+=sizeof(uns32);
      }
      else
      { if (agg_type==ATT_BINARY)
          { request_error(cdp, UNPROPER_TYPE);  break; }
        int res_size;
        if (is_a_string)
        { agg_size=specif.length;
          res_size=agg_size+1;
          inisize=1;
        }
        else
        { res_size=agg_size=tpsize[agg_type];
          if ((agg_type==ATT_INT8 || agg_type==ATT_INT16) && (ag_type==C_AVG || ag_type==C_SUM))
            { res_size=sizeof(sig32);  res_type=ATT_INT32; }
          inisize=res_size;
        }
        p=cdp->get_ans_ptr(sizeof(uns32)+res_size);  *(uns32*)p=res_size;  p+=sizeof(uns32);
       /* init the aggregate */
        switch (ag_type)
        { case C_MIN: memcpy(p, maxval(res_type),     inisize);  break;
          case C_MAX: memcpy(p, null_value(res_type), inisize);  break;
          default:    memset(p, 0,                    inisize);  break;
        }
      }

     // cycle on records:
      const char * dt;  trecnum r, cnt=0;  tfcbnum fcbn;
      for (r=0;  r<reccnt;  r++)
      { if (std)
        { if (table_record_state(cdp, tbdf, r)) continue;
          dt=fix_attr_read(cdp, tbdf, r, attr, &fcbn);
          if (dt==NULL) goto err_goto;
        }
        else
        { if (cd->curs_eseek(r)) goto err_goto;
          attr_value(cdp, &cd->qe->kont, attr, &exprval, agg_type, specif, NULL);
          dt=exprval.valptr();
        }
       /* aggregate the dt value into p: */
        switch (ag_type)
        { case C_COUNT:
            if (std ? !is_null(dt, agg_type, specif) : !exprval.is_null())  cnt++;
            break;
          case C_SUM:  case C_AVG:
            if (std ? !is_null(dt, agg_type, specif) : !exprval.is_null())
            { cnt++;
              add_val(p, dt, agg_type);
            }
            break;
          case C_MAX:
            if (!std  && exprval.is_null()) break;
            if (compare_values(agg_type, specif, dt, p) > 0)
              memcpy(p, dt, agg_size);
            break;
          case C_MIN:
            if (!std  && exprval.is_null()) 
              if (is_a_string) *(tptr)dt=0;  else break;
            if (compare_values(agg_type, specif, dt, p) < 0)
              memcpy(p, dt, agg_size);
            break;
        } // switch (ag_type)
        if (std) UNFIX_PAGE(cdp, fcbn);
        else exprval.set_null();
      } // cycle on records
     // calculate & store the result for AVG & COUNT:
      if (ag_type==C_AVG)
      { if (cnt)
          switch (agg_type)
          { case ATT_INT16: case ATT_INT8:
            case ATT_INT32: *(sig32 *)p /= (int)cnt;  break;
            case ATT_INT64: *(sig64 *)p /= (int)cnt;  break;
            case ATT_FLOAT: *(double*)p /= (int)cnt;  break;
            case ATT_MONEY:
              real2money(money2real((monstr*)p) / (int)cnt, (monstr*)p);  break;
          }
        else memcpy(p, null_value(res_type), inisize);  // avg on no items
      }
      else if (ag_type==C_COUNT) *(trecnum*)p=cnt;
      break;
    }
    case OP_INTEGRITY:  /*0*/
    { di_params di;
      BOOL repair = *(BOOL*)request;  request+=sizeof(uns32);
      if (repair && !cdp->prvs.is_conf_admin)
        { request_error(cdp, NO_CONF_RIGHT);  goto err_goto; }
      if (repair)
      { if (dir_lock_kernel(cdp, true)) goto err_goto;
      }
      else
        if (!integrity_check_planning.try_starting_check(true, true))
          { request_error(cdp, CANNOT_LOCK_KERNEL);  goto err_goto; }
      uns32 * counts = (uns32*)cdp->get_ans_ptr(5*sizeof(uns32));
      di.lost_blocks   =((uns32*)request)[0] ? counts+0 : NULL;
      di.lost_dheap    =((uns32*)request)[1] ? counts+1 : NULL;
      di.nonex_blocks  =((uns32*)request)[2] ? counts+2 : NULL;
      di.cross_link    =((uns32*)request)[3] ? counts+3 : NULL;
      di.damaged_tabdef=((uns32*)request)[4] ? counts+4 : NULL;
      request+=5*sizeof(uns32);
      db_integrity(cdp, repair, &di);  // sets error when breaked
      if (repair) dir_unlock_kernel(cdp);
      else integrity_check_planning.check_completed();
      break;
    }
    case OP_SQL_CATALOG: /*0*/
    { *(uns32*)cdp->get_ans_ptr(sizeof(uns32)) = sql_catalog_query(cdp, request);
      request+=1+sizeof(uns16);
      uns16 size = *(uns16*)request;  request+=sizeof(uns16);
      request+=size;
      break;
    }
    case OP_IDCONV:     /*0*/
      switch (*(request++))
      { case 0: /* apl->id */
          if (ker_apl_name2id(cdp, request, (uns8*)cdp->get_ans_ptr(UUID_SIZE), NULL))
            { SET_ERR_MSG(cdp, request);  request_error(cdp, OBJECT_NOT_FOUND); }
          request+=OBJ_NAME_LEN+1;
          break;
        case 1: /* id->apl */
          if (ker_apl_id2name(cdp, (uns8*)request, (tptr)cdp->get_ans_ptr(OBJ_NAME_LEN+1), NULL))
            { SET_ERR_MSG(cdp, "Appl-Id");  request_error(cdp, OBJECT_NOT_FOUND); }
          request+=UUID_SIZE;
          break;
      }
      break;
    case OP_SIGNATURE:  /*3*/
    { if (attracc!=NULL)   // non-editable (calculated) attribute
        { SET_ERR_MSG(cdp, attracc->name);  request_error(cdp, COLUMN_NOT_EDITABLE);  break; }
      t_privils_flex priv_val;
      uns8 oper=*(request++);
      if (oper==2 || oper==4)
        if (std)  // table rights
        { if (tbnum!=KEY_TABLENUM)
          { cdp->prvs.get_effective_privils(tbdf, recnum, priv_val);
            if (!priv_val.has_write(attr))
              { request_error(cdp, NO_RIGHT);  break; }
          }
        }
        else if (cd->insensitive)
          { request_error(cdp, CANNOT_APPEND);  goto err_goto; }
        else   /* cursor access rights */
        { if (!(cd->d_curs->attribs[curselem].a_flags & RI_UPDATE_FLAG))
            if (!(cd->d_curs->tabdef_flags & TFL_REC_PRIVILS))
              { request_error(cdp, NO_RIGHT);  break; }
            else
            { cdp->prvs.get_effective_privils(tbdf, recnum, priv_val);
              if (!priv_val.has_write(attr))
                { request_error(cdp, NO_RIGHT);  break; }
            }
         }
      table_descr_auto keytab_descr(cdp, KEY_TABLENUM);
      if (keytab_descr->me() == NULL) break;
      switch (oper)
      { case 1:  // create
//          if (cdp->prvs.luser()==ANONYMOUS_USER) request_error(cdp, NO_RIGHT);
//          else sign_it(cdp, tbnum, recnum, attr);
          break;
        case 0:  // check
        { uns32 * result = (uns32*)cdp->get_ans_ptr(sizeof(uns32));
          *result=(uns32)check_signature_state(cdp, tbnum, recnum, attr, FALSE);
          break;
        }
        case 7:  // get full certifying path
        { cdp->answer.sized_answer_start();
          check_signature_state(cdp, tbnum, recnum, attr, TRUE);
          tptr p=cdp->get_ans_ptr(1);  if (p!=NULL) *p=0;   /* terminator */
          cdp->answer.sized_answer_stop();
          break;
        }
        case 2:  // clear
          if (cdp->prvs.luser()==ANONYMOUS_USER) request_error(cdp, NO_RIGHT);
          else tb_write_atr_len(cdp, tbdf, recnum, attr, 0);
          break;
        case 4:  // create and return non-encrypted short signature
        { uns8 * hsh = (uns8*)cdp->get_ans_ptr(sizeof(t_hash));
          prepare_signing(cdp, keytab_descr->me(), tbnum, recnum, attr, hsh,
            (uns8*)request, *(uns32*)(request+UUID_SIZE));
          request+=UUID_SIZE+sizeof(uns32);
          break;
        }
        case 14:  // compute and return the digest
        { uns8 * hsh = (uns8*)cdp->get_ans_ptr(sizeof(t_hash_ex));
          prepare_signing_ex(cdp, keytab_descr->me(), tbnum, recnum, attr, *(trecnum*)request, hsh,
            (uns32*)cdp->get_ans_ptr(sizeof(uns32)));
          request+=sizeof(trecnum);
          break;
        }
        case 5:  // store signature
        { t_enc_hash * ehsh = (t_enc_hash*)request; request+=sizeof(t_enc_hash);
          write_signature(cdp, keytab_descr->me(), tbnum, recnum, attr, ehsh);
          break;
        }
        case 15:  // store signature ex
        { t_enc_hash * ehsh = (t_enc_hash*)request;  request+=sizeof(t_enc_hash);
          trecnum keyrec = *(trecnum*)request;  request+=sizeof(trecnum);
          uns32   stamp  = *(uns32  *)request;  request+=sizeof(uns32);  
          write_signature_ex(cdp, keytab_descr->me(), tbnum, recnum, attr, ehsh, keyrec, stamp);
          break;
        }

        case 6:  // store the public key:
        { trecnum trec=tb_new(cdp, keytab_descr->me(), TRUE, NULL);
          if (trec == NORECNUM) break;  /* error inside */
         // store key uuid:
          tb_write_atr(cdp, keytab_descr->me(), trec, KEY_ATR_UUID, request);
          request+=UUID_SIZE;
         // store logged user uuid:
          WBUUID user_uuid;
          tb_read_atr(cdp, usertab_descr->me(), cdp->prvs.luser(), USER_ATR_UUID, (tptr)user_uuid);
          tb_write_atr(cdp, keytab_descr->me(), trec, KEY_ATR_USER, user_uuid);
         // write the creation date:
          uns32 dtm=stamp_now();
          tb_write_atr(cdp, keytab_descr->me(), trec, KEY_ATR_CREDATE, &dtm);
         // write the public key:
          tb_write_var(cdp, keytab_descr->me(), trec, KEY_ATR_PUBKEYVAL, 0, 2*RSA_KEY_SIZE, request);
          request+=2*RSA_KEY_SIZE;
          break;
        }
      }
      break;
    }
    case OP_REPLCONTROL: /*0*/
    { int optype    = *(request++);
      int opparsize = *(uns16*)request;  request+=sizeof(uns16);
      repl_operation(cdp, optype, opparsize, request);
      request+=opparsize;
      break;
    }
    case OP_NEXTUSER:    /*3*/
    { int valsize;
      op_nextuser * p=(op_nextuser *)(request-sizeof(tcurstab)-sizeof(trecnum)-sizeof(tattrib));
      if       ((t_valtype)p->valtype==VT_OBJNUM) valsize=sizeof(tobjnum);
      else  if ((t_valtype)p->valtype==VT_NAME)   valsize=OBJ_NAME_LEN+1;
      else  if ((t_valtype)p->valtype==VT_NAME3)  valsize=sizeof(t_user_ident)+5;
      else/*if ((t_valtype)p->valtype==VT_UUID)*/ valsize=UUID_SIZE;
      request+=2*sizeof(uns8);
      if (attracc!=NULL)   // non-editable (calculated) attribute
        { SET_ERR_MSG(cdp, attracc->name);  request_error(cdp, COLUMN_NOT_EDITABLE);  break; }
     // check or search the _W5_DOCFLOW attribute:
      if (strcmp(tbdf->attrs[attr].name, "_W5_DOCFLOW"))
      { BOOL found=FALSE;
        if (std)
        { for (int i=1;  i<tbdf->attrcnt;  i++)
            if (!strcmp(tbdf->attrs[i].name, "_W5_DOCFLOW"))
              { attr=i;  found=TRUE;  break; }
        }
        else
        { for (int i=1;  i<cd->d_curs->attrcnt;  i++)
            if (!strcmp(cd->d_curs->attribs[i].name, "_W5_DOCFLOW"))
            { if (!attr_access(&cd->qe->kont, i, tbnum, attr, recnum, attracc))
                { request_error(cdp, BAD_ELEM_NUM);  goto err_goto; }
              if (attracc!=NULL)   // non-editable (calculated) attribute
                { SET_ERR_MSG(cdp, attracc->name);  request_error(cdp, COLUMN_NOT_EDITABLE);  goto err_goto; }
              found=TRUE;  break;
            }
        }
        if (!found) { request_error(cdp, BAD_ELEM_NUM);  break; }
      }
     // read or write data:
      if ((t_oper)p->operation==OPER_GET)
      { char xuuid[1+UUID_SIZE+UUID_SIZE];
        if (tb_read_atr(cdp, tbdf, recnum, attr, xuuid+1)) break;
        tptr ans=(tptr)cdp->get_ans_ptr(valsize);
        if ((t_valtype)p->valtype==VT_UUID)
          memcpy(ans, xuuid+1, UUID_SIZE);
        else
        { xuuid[0]=1;
          tobjnum objnum = find_object(cdp, CATEG_USER, xuuid);
          if ((t_valtype)p->valtype==VT_OBJNUM)
            *(tobjnum*)ans=objnum;
          else if (objnum==NOOBJECT)
            strcpy(ans, "-");  //*ans=0;
          else if ((t_valtype)p->valtype==VT_NAME)
            tb_read_atr(cdp, usertab_descr, objnum, USER_ATR_LOGNAME, ans);
          else
          { t_user_ident usid;
            tb_read_atr(cdp, usertab_descr, objnum, USER_ATR_INFO, (tptr)&usid);
            trace_sig(cdp, &usid, ans, FALSE);
          }
        }
      }
      else // set next user
      { uns8 xuser_id[UUID_SIZE+UUID_SIZE];  tobjnum objnum;
       // get user_id:
        if ((t_valtype)p->valtype==VT_UUID)
        { if ((t_oper)p->operation==OPER_SETREPL)
          { objnum=find_object_by_id(cdp, CATEG_USER, (uns8*)request);
            if (objnum == NOOBJECT) 
              { SET_ERR_MSG(cdp, "USER");  request_error(cdp, OBJECT_NOT_FOUND);  break; }
          }
          memcpy(xuser_id, request, UUID_SIZE);
        }
        else
        { if ((t_valtype)p->valtype==VT_OBJNUM) objnum=*(tobjnum*)request;
          else if (*request) // VT_NAME
          { objnum = find_object(cdp, CATEG_USER, request);
            if (objnum == NOOBJECT) 
              { SET_ERR_MSG(cdp, request);  request_error(cdp, OBJECT_NOT_FOUND);  break; }
          }
          else objnum=NOOBJECT; // empty name
          if (objnum == NOOBJECT) memset(xuser_id, 0, UUID_SIZE);
          else if (tb_read_atr(cdp, usertab_descr, objnum, USER_ATR_UUID, (tptr)xuser_id))
            break;
        }
       // get home server id:
        if ((t_oper)p->operation==OPER_SETREPL && objnum!=NOOBJECT)
        { if (tb_read_atr(cdp, usertab_descr, objnum, USER_ATR_SERVER, (tptr)xuser_id+UUID_SIZE))
            break;
        }
        else memset(xuser_id+UUID_SIZE, 0, sizeof(UUID_SIZE));
       // write:
        tb_write_atr(cdp, tbdf, recnum, attr, xuser_id);  // no integrity checks, writing to DOCFLOW column
        request+=valsize;
      }
      break;
    }
    case OP_DEBUG:
      debug_command(cdp, request);
      break;
    case OP_GENER_INFO: /*0*/
    { uns32 size_limit = *(uns32*)request;  request+=sizeof(uns32);
      if (!size_limit) { request_error(cdp, ERROR_IN_FUNCTION_ARG);  goto err_goto; }
      if (*request==OP_GI_SERVER_NAME)
      { int len=(int)strlen(server_name)+1;
        if (len>size_limit) len=size_limit;
        tptr pans = cdp->get_ans_ptr(sizeof(uns32)+len);
        *(uns32*)pans=len;
        strmaxcpy(pans+sizeof(uns32), server_name, len);
      }
      else if (*request==OP_GI_SERVER_HOSTNAME)  // this is not the HostName property value, this is the own host name!
      { char hostname[100];
        if (!get_own_host_name(hostname, sizeof(hostname))) *hostname=0;
        int len=(int)strlen(hostname)+1;
        if (len>size_limit) len=size_limit;
        tptr pans = cdp->get_ans_ptr(sizeof(uns32)+len);
        *(uns32*)pans=len;
        strmaxcpy(pans+sizeof(uns32), hostname, len);
      }
      else if (*request==OP_GI_INST_KEY || *request==OP_GI_SERVER_LIC)
      { if (size_limit>MAX_LICENCE_LENGTH+1) size_limit=MAX_LICENCE_LENGTH+1;
        tptr pans = cdp->get_ans_ptr(sizeof(uns32)+size_limit);
        *(uns32*)pans=size_limit;
        strmaxcpy(pans+sizeof(uns32), *request==OP_GI_INST_KEY ? installation_key : ServerLicenceNumber, size_limit);
      }
      else if (*request==OP_GI_RECENT_LOG)
        return_recent_log(cdp, size_limit);
      else  // numeric result
      { uns32 result;
        if (!get_server_info(cdp, *request, &result))
          { request_error(cdp, ERROR_IN_FUNCTION_ARG);  goto err_goto; }
        if (size_limit>sizeof(uns32)) size_limit=sizeof(uns32);
        tptr pans = cdp->get_ans_ptr(sizeof(uns32)+size_limit);
#ifndef __ia64__  // alignment
        *(uns32*)pans=size_limit;
#else
      	memcpy(pans, &size_limit, sizeof(uns32));
#endif
        memcpy(pans+sizeof(uns32), &result, size_limit);
      }
      request++;
      break;
    }

    case OP_FULLTEXT:
    { t_docid docid;  uns32 startpos, bufsize;  tptr ft_label=request;  request+=2*OBJ_NAME_LEN+2;
      docid   =*(uns32*)request;  request+=sizeof(uns32);  // old compatibility
      if (docid==0xffffffff)
        { docid   =*(uns64*)request;  request+=sizeof(uns64); }
      startpos=*(uns32*)request;  request+=sizeof(uns32);
      bufsize =*(uns32*)request;  request+=sizeof(uns32);
      if (bufsize==(uns32)-1)
      { bufsize=(uns32)strlen(request)+1;
        fulltext_docpart_start(cdp, ft_label, docid, startpos, request);
      }
      else
        fulltext_insert(cdp, ft_label, docid, startpos, bufsize, request);
      request+=bufsize;
      break;
    }
    case OP_PROPERTIES:
    { uns8 oper = *request;  request++;
      tptr owner = request;  request+=strlen(request)+1;  
      tptr name  = request;  request+=strlen(request)+1;  
      uns32 num = *(uns32*)request;  request+=sizeof(uns32);
      if (oper==OPER_GET)
      { unsigned buffer_length = *(uns32*)request;  request+=sizeof(uns32);
        get_nad_return_property_value(cdp, owner, name, num, buffer_length);
      }
      else // OPER_SET
      { uns32 valsize = *(uns32*)request;  request+=sizeof(uns32);
        tptr value = request;  request+=valsize;  
        int err = set_property_value(cdp, owner, name, num, value, valsize);
        if (err) request_error(cdp, err);
      }
      break;
    }
    case OP_INSOBJ: /*0*/
    { t_op_insobj * pars = (t_op_insobj*)request;
      create_application_object(cdp, pars->objname, pars->categ, pars->limited, (tobjnum*)cdp->get_ans_ptr(sizeof(tobjnum)));
      request+=sizeof(t_op_insobj);
      break;
    }
    case OP_SYNCHRO: /*0*/
      switch (*request++)
      { case OP_SYN_REG:
        { const char * EventName = request;  request+=strlen(request)+1;
          const char * ParamStr  = request;  request+=strlen(request)+1;
          cdp->cevs.register_event(cdp, EventName, ParamStr, *request++, (uns32*)cdp->get_ans_ptr(sizeof(uns32)));
          break;
        }
        case OP_SYN_UNREG:
          cdp->cevs.unregister_event(*(uns32*)request);  request+=sizeof(uns32);  break;
        case OP_SYN_CANCEL:
          cancel_event_wait(cdp, *(uns32*)request);  request+=sizeof(uns32);  break;
        case OP_SYN_WAIT:
        { sig32 timeout    = *(sig32*)request;  request+=sizeof(sig32);
          sig32 param_size = *(sig32*)request;  request+=sizeof(sig32);
          uns32 * EventHandle = (uns32*)cdp->get_ans_ptr(sizeof(uns32)+sizeof(uns32)+param_size+sizeof(sig32));
          if (!EventHandle) break;  // error reported
          uns32 * count    = EventHandle+1;
          char * ParamStr  = (tptr)count + sizeof(uns32);
          sig32 * result   = (sig32*)(ParamStr+param_size);
          *result = cdp->cevs.wait_for_event(cdp, timeout, EventHandle, count, ParamStr, param_size);
          break;
        }
        default:
          request_error(cdp, BAD_OPCODE);  break;
      }
      break;
    default:
      request_error(cdp, BAD_OPCODE);  break;
    case OP_END:
     if (cdp->expl_trans!=TRANS_EXPL)
       commit(cdp);  /* automatic commit if not inside transaction */
     return;  // normal exit
    }  /* end of switch */
    if (locked_tabdescr!=NULL) { unlock_tabdescr(locked_tabdescr);  locked_tabdescr=NULL; }
    if (cdp->break_request) 
      { request_error(cdp, REQUEST_BREAKED);  goto err_goto1; }
    if (cdp->roll_back_error && (cdp->sqloptions & SQLOPT_ERROR_STOPS_TRANS)) goto err_goto1;
  } while (TRUE);
  //!cdp->is_an_error();   /* up to the end of the packet */
  // must not stop processing unless on normal packed end or after rolled back!!
  // transaction remains open otherwise after OUT_OF_TABLE error!!
 err_goto:    /* exiting the request processing */
  if (locked_tabdescr!=NULL) { unlock_tabdescr(locked_tabdescr);  locked_tabdescr=NULL; }
 err_goto1:   /* tabinfo not locked */
  if (cdp->roll_back_error && (cdp->sqloptions & SQLOPT_ERROR_STOPS_TRANS))
  { if (cdp->nonempty_transaction())
    { char buf[81];
      trace_msg_general(cdp, TRACE_IMPL_ROLLBACK, form_message(buf, sizeof(buf), msg_implicit_rollback), NOOBJECT);
    }  
    roll_back(cdp);  /* must remove TMPlocks even if no data changes performed */
  }
}

uns64 last_sys_time=0, last_proc_kernel_time, last_proc_user_time;
FHANDLE hProtocol = INVALID_FHANDLE_VALUE;

void new_request(cdp_t cdp, request_frame * req_data)
// Called asynchronously by slave thread from network receiver or MF comm.
{ if (!kernel_is_init || cdp->appl_state!=WAITING_FOR_REQ || cdp->fixnum>=60) // 50 reports in get_frame()
    cdp->answer.ans_error(REJECTED_BY_KERNEL);
  else if (req_data->areq[0]!=OP_END) // keep-alive request must not update the last_request_time nor cancel the integrity checks
    if (!integrity_check_planning.starting_client_request())
      cdp->answer.ans_error(REJECTED_BY_KERNEL);
    else
    { cdp->cnt_requests++;
      cdp->last_request_time=stamp_now();
#if 0//def WINS
     // check for the server overloading:
      FILETIME CreationTime, ExitTime, KernelTime, UserTime;  LARGE_INTEGER tm;
      if (GetProcessTimes(GetCurrentProcess(), &CreationTime, &ExitTime, &KernelTime, &UserTime))
        if (QueryPerformanceCounter(&tm))   
        { uns64 sys_time, proc_kernel_time, proc_user_time;
          LARGE_INTEGER tmf;
          QueryPerformanceFrequency(&tmf);
          sys_time=tm.QuadPart * (10000000 / tmf.QuadPart);  // convert to 100ns
          memcpy(&proc_kernel_time, &KernelTime,   sizeof(proc_kernel_time));
          memcpy(&proc_user_time,   &UserTime,     sizeof(proc_user_time));
          if (sys_time>last_sys_time+1000000)  // min 100 miliseconds elapsed
          { if (last_sys_time)
            { uns64 proc = 100*((proc_kernel_time-last_proc_kernel_time) + (proc_user_time-last_proc_user_time))
                           /(sys_time-last_sys_time);
              if (proc>80)
                Sleep(50);
            }
            last_sys_time=sys_time;
            last_proc_kernel_time=proc_kernel_time;
            last_proc_user_time=proc_user_time;
          }
        }
#endif
     // profiling the thread:
      prof_time request_start_time;
      if (PROFILING_ON(cdp)) 
        request_start_time = get_prof_time();
      else
        request_start_time = 0;  // prevents counting the requsts's time if profiling started in this request
      cdp->lock_waiting_time=0;    
      cdp->lock_cycles=cdp->ilock_cycles=cdp->dlock_cycles=cdp->ft_cycles=0;
     // execute the request:
      cdp->appl_state=REQ_RUNS;
      kernel_entry(cdp, req_data->areq);
      remove_user_rlocks(cdp->applnum_perm, &pg_lockbase);  // just for debugging purposes
      if (cdp->fixnum)
        cd_dbg_err(cdp, "Fixed page at the end of the request");
      if (cdp->appl_state!=SLAVESHUTDOWN) cdp->appl_state=WAITING_FOR_REQ;
     // profiling the thread:
      if (PROFILING_ON(cdp) && request_start_time)  // if profiling now && profiled when request commenced
        add_hit_and_time2(get_prof_time()-request_start_time, 0, cdp->lock_waiting_time, PROFCLASS_THREAD, 
          *cdp->thread_name ? 0 : cdp->session_number, 0, *cdp->thread_name ? cdp->thread_name : NULL);
     // protocol:
      uns8 opcode = req_data->areq[0];
      if (the_sp.FullProtocol.val() && opcode!=OP_READ)
      { char buf[150+100];   
        uns32 tm=timestamp2time(cdp->last_request_time), tm2=Now();
        uns32 ans32 = *(uns32*)(((char*)cdp->answer.get_ans())+ANSWER_PREFIX_SIZE);
        sprintf(buf, "%02u:%02u:%02u,%u,%u,%s,%u,%u,%x,%u,%u,%u,%u,%u", 
          Hours(tm),Minutes(tm),Seconds(tm),//Hours(tm2),Minutes(tm2),Seconds(tm2),
          (tm2-tm+500)/1000,
          cdp->session_number,cdp->thread_name, opcode, cdp->get_return_code(),ans32,
          10*cdp->lock_waiting_time/get_prof_koef(), cdp->lock_cycles, cdp->ilock_cycles, cdp->dlock_cycles, cdp->ft_cycles);
        int pos=(int)strlen(buf);
        uns8 opc_type=opc_types[opcode];
        if (opc_type)
        { sprintf(buf+pos, ",%04hx", *(uns16*)(req_data->areq+1));  pos+=5;
          if (opc_type>1)
          { sprintf(buf+pos, ",%10u", *(uns32*)(req_data->areq+5));  pos+=11;
            if (opc_type>2)
            { sprintf(buf+pos, ",%5u", *(uns16*)(req_data->areq+9));  pos+=6;
            }
            else buf[pos++]=',';
          }
          else {buf[pos++]=','; buf[pos++]=','; }
        }      
        else { buf[pos++]=',';  buf[pos++]=',';  buf[pos++]=','; }
        if (opcode==OP_SUBMIT)
        { buf[pos++]=',';
          int len=5;  unsigned char c;
          c=req_data->areq[len];
          while (c && len<100)
          { if (c==',') buf[pos]='#';
            else if (c<' ') buf[pos]=' ';
            else buf[pos]=c;
            len++;  pos++;
            c=req_data->areq[len];
          }  
        }  
        buf[pos++]='\r';  buf[pos++]='\n';
        ffa.to_protocol(buf, pos);
      }    
#ifdef STOP          
     // detecting long requests:     
      uns32 end_time = clock_now();
      if (end_time > cdp->last_request_time+15)
        end_time=0;
#endif        
      integrity_check_planning.thread_operation_stopped();
    }
}

////////////////////////// reporting server errors ////////////////////////////
#include <stdarg.h>

char * trans_message(const char * msg)
{ char * sys_msg;
#ifdef ENGLISH  
  msg = gettext(msg);
  int len = (int)strlen(msg);
  sys_msg = new char[len+1];
  strcpy(sys_msg, msg);  // used when sys_spec.charset==0 and on conversion error
  if (sys_spec.charset != 0)  // for charset 0 the message remains in UTF-8
    superconv(ATT_STRING, ATT_STRING, msg, sys_msg, t_specif(0, CHARSET_NUM_UTF8, 0, 0), sys_spec, NULL);
#else  // messages are in the national character set
  int len = strlen(msg);
  sys_msg = new char[2*len+1];  // sys_spec may be UTF-8
  strcpy(sys_msg, msg);  // used when sys_spec.charset==1 and on conversion error
  if (sys_spec.charset != 1)
    superconv(ATT_STRING, ATT_STRING, msg, sys_msg, t_specif(0, 1, 0, 0), sys_spec.charset ? sys_spec : t_specif(0, CHARSET_NUM_UTF8, 0, 0), NULL);
#endif
  return sys_msg;
}  
  
char * form_message(char * buf, int bufsize, const char * msg, ...)
// Translates [msg] to the system language, adds its parameters and returns it as the function value and in [buf].
{
  char * sys_msg = NULL;
#ifdef ENGLISH  
  msg = gettext(msg);
  if (sys_spec.charset != 0)  // for charset 0 the message remains in UTF-8
  { int len = (int)strlen(msg);
    sys_msg = new char[len+1];
    if (superconv(ATT_STRING, ATT_STRING, msg, sys_msg, t_specif(0, CHARSET_NUM_UTF8, 0, 0), sys_spec, NULL)>=0)
      msg=sys_msg;
  }
#else  // messages are in the national character set
  if (sys_spec.charset != 1)
  { int len = strlen(msg);
    sys_msg = new char[2*len+1];  // sys_spec may be UTF-8
    if (superconv(ATT_STRING, ATT_STRING, msg, sys_msg, t_specif(0, 1, 0, 0), sys_spec.charset ? sys_spec : t_specif(0, CHARSET_NUM_UTF8, 0, 0), NULL)>=0)
      msg=sys_msg;
  }
#endif
  va_list ap;
  va_start(ap, msg);
  vsnprintf(buf, bufsize, msg, ap);
  buf[bufsize-1]=0;  // on overflow the terminator is NOT written!
  va_end(ap);
  if (sys_msg) delete [] sys_msg;
  return buf;
}

#ifdef WINS
char * form_terminal_message(char * buf, int bufsize, const char * msg, ...)
// Translates [msg] to the terminal charset, adds its parameters and returns it as the function value and in [buf].
// ATTN: Parameters must be in the GetACP() charset!
{
  char * sys_msg = NULL;
#if WBVERS<=1000  // console uses UTF-8 since 1001
  UINT acp = GetACP();  t_specif win_specif;
  switch (acp)
  { case 1250: win_specif.charset=1;  break;  // EE
    case 1252: win_specif.charset=3;  break;  // western Europe
    default:   win_specif.charset=1;  break;
  }
#ifdef ENGLISH  
  msg = gettext(msg);
  int len = (int)strlen(msg);
  sys_msg = new char[len+1];
  if (superconv(ATT_STRING, ATT_STRING, msg, sys_msg, t_specif(0, CHARSET_NUM_UTF8, 0, 0), win_specif, NULL)>=0)
    msg=sys_msg;
#else  // messages are in the national character set
  if (win_specif.charset!=1)
  { int len = strlen(msg);
    sys_msg = new char[len+1];
    if (superconv(ATT_STRING, ATT_STRING, msg, sys_msg, t_specif(0, 1, 0, 0), win_specif, NULL)>=0)
      msg=sys_msg;
  }
#endif
#endif
  va_list ap;
  va_start(ap, msg);
  vsnprintf(buf, bufsize, msg, ap);
  buf[bufsize-1]=0;  // on overflow the terminator is NOT written!
  va_end(ap);
  if (sys_msg) delete [] sys_msg;
  return buf;
}
#endif

BOOL WINAPI Get_server_error_text(cdp_t cdp, int err, char * buf, unsigned buflen)
/* Returns TRUE if any error, FALSE if no error. */
{ const char * p;
  if (err==ANS_OK) { *buf=0;  return FALSE; }
 // get message pattern:
  if (err >= FIRST_GENERIC_ERROR && err <= LAST_GENERIC_ERROR)
    p=cdp->current_generic_error_message ? cdp->current_generic_error_message : "?";
  else if (err >= FIRST_COMP_ERROR)
    p=compil_errors[err - FIRST_COMP_ERROR];
  else if (err >= FIRST_MAIL_ERROR)
    p=mail_errors[err - FIRST_MAIL_ERROR];
  else if (err > LAST_DB_ERROR)
    p=db_errors[LAST_DB_ERROR-0x80+1];
  else if (err<0x80)
  { int ind = 0;
    while (warning_messages[ind]!=err && warning_messages[ind]) ind++;
    p=db_warnings[ind];
  }
  else
    p=db_errors[err-0x80];
 // convert the message to the system charset:
  form_message(buf, buflen, p, cdp->errmsg);
  return *buf != 0;
}

void WINAPI cd_dbg_err(cdp_t cdp, const char * text)  
// Reports server errors to the log & console - with client reference
{ char buf[201];
#ifdef WINS
  MessageBeep(-1);
#endif
  if (is_trace_enabled_global(TRACE_SERVER_FAILURE))
    trace_msg_general(cdp ? cdp : cds[0], TRACE_SERVER_FAILURE, form_message(buf, sizeof(buf), text), NOOBJECT);
}

void WINAPI dbg_err(const char * text)  
// Reports server errors to the log & console
{ cd_dbg_err(NULL, text); }

void WINAPI trace_info(const char * text)  // tracing global network events (used for english messages only)
{ char buf[81];
  if (is_trace_enabled_global(TRACE_NETWORK_GLOBAL))
    trace_msg_general(cds[0], TRACE_NETWORK_GLOBAL, form_message(buf, sizeof(buf), text), NOOBJECT);
}

void write_to_log(cdp_t cdp, const char * text)
// [text] is supposed to be in the system charset.
{ trace_msg_general(cdp, TRACE_LOG_WRITE, text, NOOBJECT); }

void WINAPI display_server_info(const char * text)
// [text] is translated, in the system charset. Must use form_message() to create it.
{ trace_msg_general(cds[0], TRACE_SERVER_INFO, text, NOOBJECT); }

void WINAPI err_msg(cdp_t cdp, int errnum)  // creates server error message but does not rollback
#define MAX_ERR_MSG_LEN 350
{ tptr xbuf = (tptr)corealloc(MAX_ERR_MSG_LEN+8, 66);
  if (xbuf==NULL) return;
  Get_server_error_text(cdp, errnum, xbuf, MAX_ERR_MSG_LEN);  // returns the message in the system charset
  sprintf(xbuf+strlen(xbuf), " {%u}", errnum);
  trace_msg_general(cdp, errnum==GENERR_FULLTEXT_CONV ? TRACE_CONVERTOR_ERROR : TRACE_USER_ERROR, xbuf, NOOBJECT);
  corefree(xbuf);
}


// 3 - client API critical error
// 4 - client API critical error outside SQL
// 7 - error code unused
// 9 - no rollback exception condition

// INTERNAL_SIGNAL moved to the CONDITION_COMPLETION category in 8.0 (no rollback)
/* Logika rozdelni chyb:
 CONDITION_COMPLETION (9) - neni teba nijak reagovat, klient dostane informaci v �sle chyby a nedostane jin�data
   Sem mono pesunout chyby, kter�se projev�jinak, teba hodnotou funkce, ale pozor na nevr�en�dat klientovi.
 CONDITION_HANDLABLE (0) - default reaction is defined, rollback is not necessaary
   handlers (including exit) will be called
 CONDITION_ROLLBACK (1) - always rollback, then can continue, 
   handlers (including exit) will be called
 CONDITION_CRITICAL (2) - rollback cannot be avoided, not calling the handler because of server internal probles
   or because the exception failed to be handled before
 3 - Error of communication with the client, not calling the handler
   Always rollback (sometimes it may not be necessary).
 4 - Error on the client side, not processed by the server.
 5 - CONDITION_HANDLABLE_NO_RB - reports error, handlable, but never causes rollback
 
Pozor:
 OBJECT_DOES_NOT_EXIST - CONDITION_HANDLABLE, the SQL statement will not be executed
 OBJECT_NOT_FOUND      - client communication error

 Problematick� Chyby, kter�vznikaj�na klientsk�i serverov�stran� neikaj� zda prob�l rollback!
*/

static const uns8 cond_cats[] = {
//0 1 2 3 4  5 6 7 8 9
                   3,3,
  3,9,0,3,3, 3,1,0,3,0,  // 130
  3,0,3,3,3, 2,3,3,3,3,  // 140   // EMPTY not raised in SQL! using warning instead
  4,3,2,3,2, 3,4,3,3,4,  // 150
  1,3,1,2,2, 2,2,2,2,3,  // 160
  3,1,1,4,1, 1,3,1,4,4,  // 170
  3,4,4,1,9, 0,3,3,3,4,  // 180
  4,2,3,3,3, 1,0,0,1,0,  // 190
  0,0,0,0,0, 0,0,0,0,2,  // 200
  0,0,0,3,2, 3,7,0,3,1,  // 210
#if WBVERS<950
  1,3,3,0,0, 3,0,3,0,9,  // 220   
#elif WBVERS<1100
  1,3,3,0,0, 3,9,3,0,9,  // 220   // 226 CONVERSION_NOT_SUPPORTED changed to 9 in 9.5
#else
  1,3,3,0,0, 3,5,3,0,9,  // 220   // 226 CONVERSION_NOT_SUPPORTED changed to 5 in v. 11
#endif
  1,3,2,3,3, 3,0,9,3,0   // 230
};

t_condition_category condition_category(int sqlcode)
{ if (sqlcode<128) return CONDITION_COMPLETION;
  if (sqlcode==GENERR_FULLTEXT_CONV) 
#if WBVERS<1100
    return CONDITION_COMPLETION;
#else    
    return CONDITION_HANDLABLE_NO_RB;
#endif    
  if (sqlcode>255) return CONDITION_HANDLABLE;
  uns8 tp = cond_cats[sqlcode-128];
  return tp==9 ? CONDITION_COMPLETION : !tp ? CONDITION_HANDLABLE : tp==1 ? CONDITION_ROLLBACK : 
         tp==5 ? CONDITION_HANDLABLE_NO_RB : CONDITION_CRITICAL;
}

void make_kontext_snapshot(cdp_t cdp)
{// drop the old snapshot:
  if (cdp->exkont_info) { corefree(cdp->exkont_info);  cdp->exkont_info=NULL; }
  if (cdp->execution_kontext)
  {// allocate the new snapshop and copy the context into it: 
    cdp->exkont_len=cdp->execution_kontext->count();
    cdp->exkont_info=(t_exkont_info*)corealloc(cdp->exkont_len * sizeof(t_exkont_info), 58);
    if (cdp->exkont_info)
    { t_exkont * exkont = cdp->execution_kontext;  t_exkont_info * exkont_info = cdp->exkont_info;
      do
      { exkont->get_info(cdp, exkont_info);  
        exkont->get_descr(cdp, exkont_info->text, sizeof(exkont_info->text));
        exkont_info++;  exkont=exkont->_next();
      } while (exkont);
    }
  }
}

void WINAPI request_compilation_error(cdp_t cdp, int i)
// Preserves compilation error kontext, called only from "compile". Clients call "request_error" instead.
{ t_condition_category cc=condition_category(i);
  if (i==ANS_OK) // clears the previous error 
  { cdp->set_return_code(i);  
    cdp->roll_back_error=false;
    *cdp->except_name=0;
    cdp->except_type=EXC_NO;
  }
  else
  { if (cc==CONDITION_COMPLETION)  // no rollback exception condition: just store the return code
      { if (!cdp->is_an_error()) cdp->set_return_code(i); }
    else  // unhandled and unhadlable errors causing rollback
    { cdp->set_return_code(i);  // clears the error when i==0
      cdp->roll_back_error = cc!=CONDITION_HANDLABLE_NO_RB;
     // copy the context:
      make_kontext_snapshot(cdp);
    }
   // log the event:
    if (i!=OUT_OF_TABLE && i!=OBJECT_NOT_FOUND && i!=INTERNAL_SIGNAL && i!=NOT_FOUND_02000) 
      if (!(cdp->ker_flags &  KFL_HIDE_ERROR))
        err_msg(cdp, i);
  }      
}

BOOL may_have_handler(int errnum, int in_trigger)
{ t_condition_category cat = condition_category(errnum);
  return cat==CONDITION_HANDLABLE || cat==CONDITION_HANDLABLE_NO_RB || cat==CONDITION_ROLLBACK && !in_trigger;
}

void make_exception_from_error(cdp_t cdp, int errnum)
{ wb_error_to_sqlstate(errnum, cdp->except_name);  // never called for SQ_UNHANDLED_USER_EXCEPT, except_name has the name of the exception
  cdp->except_type=EXC_EXC;
  if (errnum!=SQ_TRIGGERED_ACTION) strcpy(cdp->last_exception_raised, cdp->except_name+1);
}
  
extern "C" BOOL WINAPI request_error(cdp_t cdp, int errnum)  // must not be called for compilation errors because they must not be handled
// Client error other than compilation error (clears the old compilation error kontext)
{ make_exception_from_error(cdp, errnum);
  if (may_have_handler(errnum, cdp->in_trigger))
    if (condition_category(errnum)!=CONDITION_ROLLBACK)  // not a rollback condition!!
      if (try_handling(cdp)) return FALSE;  // returns iff error handled
 // the error number may have been changed by the handler or replaced by an user exception:
  int error_code;
  if (cdp->except_name[0]==SQLSTATE_EXCEPTION_MARK && sqlstate_to_wb_error(cdp->except_name+1, &error_code))
    request_error_no_context(cdp, error_code);
  else
    request_error_no_context(cdp, SQ_UNHANDLED_USER_EXCEPT);
  return TRUE;
}

void request_generic_error(cdp_t cdp, int errnum, const char * msg)
{ int len = (int)strlen(msg);
  if (cdp->current_generic_error_message) corefree(cdp->current_generic_error_message);
  cdp->current_generic_error_message = (char*)corealloc(len+1, 85);
  if (cdp->current_generic_error_message) strcpy(cdp->current_generic_error_message, msg);
  request_error(cdp, errnum);
}

void WINAPI warn(cdp_t cdp, int i)
#if WBVERS>=810
{ char msg[120];
  Get_server_error_text(cdp, i, msg, sizeof(msg));
  trace_msg_general(cdp, TRACE_USER_WARNING, msg, NOOBJECT);  // msg is in the system charset
  if (!cdp->is_an_error()) 
  { make_kontext_snapshot(cdp);
    cdp->set_return_code(i); // | replaced by = in 8.1
  }  
}
#else
{ if (!cdp->is_an_error()) cdp->set_return_code(cdp->get_return_code() | i); }
#endif

void report_state(cdp_t cdp, trecnum rec1, trecnum rec2)
// Sends progress notification to the client.
// Does nothing for detached threads, worker threads, system thread etc.
{ cdp->send_status_nums(rec1, rec2); }
/*************************** commit & roll_back *****************************/
BOOL check_fcb(void);

void restore_local_table(cdp_t cdp, table_descr * tbdf)
{ tblocknum basblockn=create_table_data(cdp, tbdf->indx_cnt, tbdf->translog);
  tbdf->prepare(cdp, basblockn, tbdf->translog); // this stores the new basblockn in the tbdf, loads new index ...
  //...root numbers from the basblock, and updates the basblock copy (recnum == 0)
 // emptying the lists of free records:
  tbdf->free_rec_cache.drop_free_rec_list(cdp);
  tbdf->free_rec_cache.drop_caches();
}

void t_translog::rollback(cdp_t cdp)
{/* the changed pages in core are made invalid */
  log.remove_all_changes(cdp);
 // remove allocations and deallocations:
  log.rollback_allocs(cdp);   // stores numbers of inserted records to the cache, allocates containter blocks
  rollback_transaction_objects_post(cdp);
  clear_changes();
}

void t_translog_main::rollback(cdp_t cdp)
{ t_index_uad * iuad;
 // remove index changes, rollback index allocs:
  for (iuad = index_uad;  iuad;  iuad=iuad->next)
    iuad->remove_changes_and_rollback_allocs(cdp, cdp->applnum_perm);
 // drop index info:
  clear_index_info(cdp);
 // remove page changes:
  t_translog::rollback(cdp);
}

//int check_node_num(cdp_t cdp, dd_index * indx, tblocknum root);

void roll_back(cdp_t cdp)
{ t_translog * subtl;
#if 0
  if (tables[8] && tables[8]->indxs)
  { t_dumper dumper1(cdp, "c:\\pre0.log");
    dumper1.dump_tree(cdp, &tables[8]->indxs[0], tables[8]->indxs[0].root);
    t_dumper dumper2(cdp, "c:\\pre3.log");
    dumper2.dump_tree(cdp, &tables[8]->indxs[3], tables[8]->indxs[3].root);
  }
#endif
  for (subtl = cdp->subtrans;  subtl;  subtl=subtl->next)
    if (subtl->tl_type!=TL_CURSOR) subtl->rollback(cdp);  
  lock_all_index_roots(cdp);  // Locking because I must prevent access of committing client to my index_uad. May fail on server shutdown
  cdp->tl.rollback(cdp);
 // remove records added into cursors (not created in this transaction): 
  for (int i=0;  i<cdp->d_ins_curs.count();  i++)
  { ins_curs * ic = cdp->d_ins_curs.acc0(i);
    cur_descr * cd = *crs.acc(GET_CURSOR_NUM(ic->cursnum));
    if (cd!=NULL && cd->owner==cdp->applnum_perm)  // may have been closed and/or opened by somebody else, NULL iff created in this trans and removed in this rollback
    { if (ic->recnum < cd->recnum) cd->recnum=ic->recnum;
      if (cd->pmat)
        if (ic->recnum < cd->pmat->rec_cnt) cd->pmat->rec_cnt=ic->recnum;
    }
  }
  cdp->d_ins_curs.ins_curs_dynar::~ins_curs_dynar();
#if 0
  if (tables[8] && tables[8]->indxs)
  { if (check_node_num(cdp, &tables[8]->indxs[0], tables[8]->indxs[0].root)!=8899)
    { t_dumper dumper1(cdp, "c:\\post0.log");
      dumper1.dump_tree(cdp, &tables[8]->indxs[0], tables[8]->indxs[0].root);
    }
    if (check_node_num(cdp, &tables[8]->indxs[3], tables[8]->indxs[3].root)!=8899)
    { t_dumper dumper2(cdp, "c:\\post3.log");
      dumper2.dump_tree(cdp, &tables[8]->indxs[3], tables[8]->indxs[3].root);
    }
  }
#endif
 // end the transaction
  tra_worms_free(cdp);
  cdp->cevs.cancel_events();
 // restore not-committed local tables:
  for (subtl = cdp->subtrans;  subtl;  subtl=subtl->next)
    if (subtl->tl_type==TL_LOCTAB)
    { t_translog_loctab * translog = (t_translog_loctab*)subtl;
      if (!translog->table_comitted)
        restore_local_table(cdp, translog->tbdf);
      else  // must update recnum and drop the caches, if descreasing it
      { t_versatile_block bas_acc(cdp, false);
        bas_tbl_block * basbl = (bas_tbl_block*)bas_acc.access_block(translog->tbdf->basblockn, translog);
        if (basbl->recnum!=translog->tbdf->Recnum())
        { translog->tbdf->update_basblock_copy(basbl);  // should not return error because new opy is smaller than the old one
         // emptying the lists of free records:
          translog->tbdf->free_rec_cache.drop_free_rec_list(cdp);
          translog->tbdf->free_rec_cache.drop_caches();
        }
      }
    }
}

#ifdef STOP
  rollback_transaction_objects_pre(cdp); // destroys created cursors and their temp. tables
#endif

CRITICAL_SECTION commit_sem;

static uns8 op_end = OP_END;

int check_constrains(cdp_t cdp, table_descr * tbdf, trecnum check_recnum, const t_atrmap_flex * ch_map, int immediate_check, BOOL * contains_other_check, char * err_constr_name)
// Checks the integrity constrains defined for table [tbdf] which may be affected by changing columns
//   listed in [ch_map] in record [check_recnum]. If [ch_map]==NULL, all columns may have been changed.
// If [immediate_check]==0, check the deferred constrains, if [immediate_check]==1, check the immediate c., if ==2 checks both.
// Returns error number if any constrain is not satisfied, and the name of the constrain in err_constr_name.
// for [immediate_check]==2 does not call request_error().
{ int err=0;  
  if (err_constr_name) *err_constr_name=0;
  *contains_other_check=FALSE;
  if (!(cdp->ker_flags & KFL_DISABLE_INTEGRITY))
  {// cycle on check constrains:
    int i;  dd_check * cc;  
    for (i=0, cc=tbdf->checks;  i<tbdf->check_cnt && !err;  i++, cc++)
      if ((cc->deferred || (cdp->ker_flags & KSE_FORCE_DEFERRED_CHECK)!=0) != immediate_check || immediate_check==2)
      {// match the list of changed columns with the list of columns used in the constrain: 
        BOOL match = ch_map==NULL || ch_map->intersects(&cc->atrmap);
        if (match && cc->expr!=NULL)  // NULL means error in constraint, not compiled
        { t_value res;
          cdp->kont_tbdf=tbdf;  cdp->kont_recnum=check_recnum;
          cdp->check_rights=false;
          uns32 saved_opts = cdp->sqloptions;  
          cdp->sqloptions &= ~SQLOPT_CONSTRS_DEFERRED;  // must not affect user-defined options, the execution of CHECK may use them
          expr_evaluate(cdp, (t_express *)cc->expr, &res);
          cdp->sqloptions = saved_opts;
          cdp->check_rights=true;
          if (/*cdp->is_an_error() ||*/ res.is_false())  // UNKNOWN is OK
          // Error in the CHECK constrain should be interpreted as FALSE -- but the problem is that an older error would be interpreted as CHECK error
          { t_exkont_record ekt(cdp, tbdf->tbnum, check_recnum);
            SET_ERR_MSG(cdp, cc->constr_name);  if (err_constr_name) strcpy(err_constr_name, cc->constr_name);
            if (immediate_check!=2) request_error(cdp, CHECK_CONSTRAINT);  
            err=CHECK_CONSTRAINT;  
          }
        } // match
      }
      else // IMMEDIATE vs. DEFERRED
        *contains_other_check=TRUE;

   // cycle on foreign key constrains:
    dd_forkey * fcc;  
    for (i=0, fcc=tbdf->forkeys;  i<tbdf->forkey_cnt && !err;  i++, fcc++)
      if ((fcc->deferred || (cdp->ker_flags & KSE_FORCE_DEFERRED_CHECK)!=0) != immediate_check || immediate_check==2)
      { if (!(cdp->in_active_ri>0 && immediate_check)) // disable immediate ref int inside CASCADE (not exact, may disable too much, but I must not check the REF INT inside cascaded REF INT)
        {// match the list of changed columns with the list of columns used in the constrain: 
          BOOL match = ch_map==NULL || ch_map->intersects(&fcc->atrmap);
          if (match && !(cdp->ker_flags & KFL_DISABLE_REFINT) && fcc->desttabnum!=NOOBJECT)
          { char keyval[MAX_KEY_LEN];  dd_index * par_ind;
           /* compute local key: */
            dd_index * indx = &tbdf->indxs[fcc->loc_index_num];
            if (!compute_key(cdp, tbdf, check_recnum, indx, keyval, false))
              { /*request_error(cdp, REFERENTIAL_CONSTRAINT);*/  err=TRUE;  continue; }
            *(trecnum*)(keyval+indx->keysize-sizeof(trecnum))=0;
            if (!is_null_key(keyval, indx))
            {/* find foreign table & index: */
              table_descr_auto par_td(cdp, fcc->desttabnum);
              if (!par_td->me())
                { /*request_error(cdp, REFERENTIAL_CONSTRAINT);*/  err=TRUE;  continue; }
             // checking if the parent index is valid:
              if (fcc->par_index_num==-1 || par_td->indx_cnt <= fcc->par_index_num ||
                  par_td->indxs[fcc->par_index_num].ccateg==INDEX_NONUNIQUE)
              { SET_ERR_MSG(cdp, fcc->constr_name);  if (err_constr_name) strcpy(err_constr_name, fcc->constr_name);
                if (immediate_check!=2) request_error(cdp, REFERENTIAL_CONSTRAINT);  
                err=REFERENTIAL_CONSTRAINT;  continue; 
              }
              par_ind = &par_td->indxs[fcc->par_index_num];
             /* search in the index: */
              cdp->index_owner = par_td->tbnum;
              if (!find_key(cdp, par_ind->root, par_ind, keyval, NULL, true))
              { t_exkont_table ekt(cdp, tbdf->tbnum, check_recnum, NOATTRIB, NOINDEX, OP_PTRSTEPS, NULL, 0);
                { t_exkont_key ekt(cdp, indx, keyval);
                  SET_ERR_MSG(cdp, fcc->constr_name);  if (err_constr_name) strcpy(err_constr_name, fcc->constr_name);
                  if (immediate_check!=2) request_error(cdp, REFERENTIAL_CONSTRAINT);  
                }
                err=REFERENTIAL_CONSTRAINT;  
              }
            }
          } // match
        } 
      }
      else // IMMEDIATE vs. DEFERRED
        *contains_other_check=TRUE;

   // check NOT NULL constrains:
    if (tbdf->notnulls != NULL)   /* any NOT NULL column present */
      if ((!(cdp->sqloptions & SQLOPT_CONSTRS_DEFERRED) && !(cdp->ker_flags & KSE_FORCE_DEFERRED_CHECK)) == immediate_check || immediate_check==2)
      { for (tattrib * patr = tbdf->notnulls;  *patr;  patr++)  // list may be empty, cycle on NOT NULL attributes 
          if (ch_map==NULL || ch_map->has(*patr))
          { const attribdef * att = tbdf->attrs+*patr;
            if (!IS_HEAP_TYPE(att->attrtype) && att->attrmult==1)
            { tfcbnum fcbn;
              const char * dt = fix_attr_read(cdp, tbdf, check_recnum, *patr, &fcbn);
              if (dt!=NULL)
              { if (IS_NULL(dt, att->attrtype, att->attrspecif))
                { unfix_page(cdp, fcbn);  
                  t_exkont_table ekt(cdp, tbdf->tbnum, check_recnum, *patr, NOINDEX, 0, NULL, 0);
                  SET_ERR_MSG(cdp, att->name);  if (err_constr_name) strcpy(err_constr_name, att->name);
                  if (immediate_check!=2) request_error(cdp, MUST_NOT_BE_NULL);  
                  err=MUST_NOT_BE_NULL;  break;
                }
                UNFIX_PAGE(cdp, fcbn);
              }
            }
          } 
      }
      else // IMMEDIATE vs. DEFERRED
        *contains_other_check=TRUE;
  } // integrity checks not disabled
  return err;
}

bool t_translog::pre_commit(cdp_t cdp)
{ if (log.is_error())  // checking for accumulated error
    { log.clear_error();  request_error(cdp, OUT_OF_KERNEL_MEMORY);  return true; }
 // check the deferred integrity constrains:
  for (int chi=0;  chi<ch_s.count();  chi++)
  { t_delayed_change * ch = ch_s.acc0(chi);  BOOL other;
    table_descr_auto tbdf(cdp, ch->ch_tabnum);
    if (!tbdf->me()) return true;
    int ind=0;  trecnum rec;
    while (ch->ch_recnums.get_next(ind, rec))
      if (rec!=NORECNUM && rec!=INVALID_HASHED_RECNUM)
       // ch_s contains deleted records, too
       if (table_record_state(cdp, tbdf->me(), rec) == NOT_DELETED)
        if (check_constrains(cdp, tbdf->me(), rec, &ch->ch_map, FALSE, &other, NULL))
          return true; 
  } 
  return false;
}

BOOL commit(cdp_t cdp)
{ t_translog * subtl;

 // check for errors, check the deferred constrains:
  if (cdp->roll_back_error)  // posible when error does not stop the transaction
    goto err0;
  for (subtl = cdp->subtrans;  subtl;  subtl=subtl->next)
    if (subtl->pre_commit(cdp)) goto err0;
  if (cdp->tl.pre_commit(cdp)) goto err0;

 // lock all indexes (atomically):
  if (!lock_all_index_roots(cdp))
    { request_error(cdp, REQUEST_BREAKED);  goto err0; }
  if (!cdp->tl.update_all_indexes(cdp) || cdp->roll_back_error) 
    goto err;  // e.g. key duplicity
  make_conflicting_indexes_invalid(cdp);

 // clear changes just after checking/replying them (on error, cleared by the rollback)
  for (subtl = cdp->subtrans;  subtl;  subtl=subtl->next)
    subtl->clear_changes();  
  cdp->tl.clear_changes();  

  if (cdp->nonempty_transaction())  // checks the subtransactions, too
  {//////////////////////////// CS ////////////////////////////////////////////
    ProtectedByCriticalSection cs(&commit_sem, cdp, WAIT_CS_COMMIT);  /* due to multiple changes in a page */
    prof_time start;
    if (PROFILING_ON(cdp)) start = get_prof_time();  
   /* write the changes to the journal */
    if (the_sp.WriteJournal.val())
    { jourheader jh;  
      jh.spec_size=sizeof(jourheader)+worm_len(cdp, &cdp->jourworm)+1+
        ((uns32)JOURMARK << 24);
      jh.timestamp=stamp_now();
      jh.usernum=cdp->prvs.luser();
      jour_write(cdp, &jh, sizeof(jourheader));
      jour_copy (cdp);
      jour_write(cdp, &op_end, 1);
      jour_flush(cdp);
    }
   // secured transaction: swap all and create tjcs: */
    bool tjc_is_valid = false;
    int tjc_num, tjc_block_link;  tblocknum * core_tjc, * tjc_all;
    if (the_sp.SecureTransactions.val())
    { int i, chpages, tjc_max, tjc_pos;  
     // count the changed pages:
      chpages=0;
      for (i=0;  i < cdp->tl.log.tablesize;  i++)
        if (cdp->tl.log.htable[i].is_change()) chpages++;
      if (chpages) 
      {// allocate core structures:
        core_tjc = (tblocknum*)corealloc(BLOCKSIZE, 88);
        if (core_tjc==NULL) { request_error(cdp, OUT_OF_KERNEL_MEMORY);  goto err; }
        int tjc_count = chpages / ((BLOCKSIZE / (2*sizeof(tblocknum))) - 1) + 1;
        tjc_all = (tblocknum*)corealloc(tjc_count*sizeof(tblocknum), 88);
        if (tjc_all==NULL) { corefree(core_tjc);  request_error(cdp, OUT_OF_KERNEL_MEMORY);  goto err; }
        tjc_num=0;  tjc_max=BLOCKSIZE / sizeof(tblocknum);  tjc_pos=2;
        tjc_block_link=-1;  // the prevoius tjc block, or -1 if none exists
       // save blocks:
        for (i=0;  i < cdp->tl.log.tablesize;  i++)
          if (cdp->tl.log.htable[i].is_change())
          { if (tjc_pos==tjc_max)
            { core_tjc[0]=VALID_TJC_DEPENDENT;
              core_tjc[1]=tjc_block_link;
              tjc_block_link=transactor.save_tjc_block(core_tjc);
              if (tjc_block_link==-1) 
                { corefree(core_tjc);  corefree(tjc_all);  request_error(cdp, TOO_COMPLEX_TRANS);  goto err; }
              tjc_all[tjc_num++]=tjc_block_link;
              tjc_pos=2;
            }
            if (!make_swapped(cdp, cdp->tl.log.htable[i].block, core_tjc+tjc_pos+1))
            { core_tjc[tjc_pos]=cdp->tl.log.htable[i].block;
              tjc_pos+=2;
            }
          }
       // write delimiter & save the leader block:
        if (tjc_pos<tjc_max)  core_tjc[tjc_pos]=core_tjc[tjc_pos+1]=(tblocknum)-1;
        core_tjc[0]=VALID_TJC_LEADER;
        core_tjc[1]=tjc_block_link;
        tjc_block_link=transactor.save_tjc_block(core_tjc);
        if (tjc_block_link==-1) { corefree(core_tjc);  corefree(tjc_all);  request_error(cdp, TOO_COMPLEX_TRANS);  goto err; }
        tjc_all[tjc_num++]=tjc_block_link;
        tjc_is_valid=true;
      }  
    }
   // writing the new values of changed pages: on disc, other frames & swaps:
    bool written = false;
    for (subtl = cdp->subtrans;  subtl;  subtl=subtl->next)
      if (subtl->tl_type!=TL_CURSOR)  // not saving any more to the database file
      { subtl->log.write_all_changes(cdp, written);
       // mark all local tables as "committed":
        if (subtl->tl_type==TL_LOCTAB)
        { t_translog_loctab * translog = (t_translog_loctab*)subtl;
          translog->table_comitted=true;
        }
      }
    cdp->tl.write_all_changes(cdp, written);
   /* mark the transaction as completed in journal */
    if (tjc_is_valid)  /* write transaction info */
    { transactor.invalidate_tjc_block(tjc_block_link);
      transactor.free_tjc_blocks(tjc_all, tjc_num);
      corefree(core_tjc);  corefree(tjc_all);
    }
    if (written) disksp.extflush(cdp, the_sp.FlushOnCommit.val()!=0);
    if (PROFILING_ON(cdp)) add_hit_and_time(get_prof_time()-start, PROFCLASS_ACTIVITY, PROFACT_COMMIT, 0, NULL);
  } // end of CS
 /* remove info about records added into cursors: */
  cdp->d_ins_curs.ins_curs_dynar::~ins_curs_dynar();
 /* make the freed pages free (close cursor makes no changes but frees pages) */
  cdp->tl.commit_index_allocs(cdp);  // roots must not be unlocked before this - otherwise another client may start working with iuad data structures which are deleted here!
  for (subtl = cdp->subtrans;  subtl;  subtl=subtl->next)
    if (subtl->tl_type!=TL_CURSOR)  // not saved any more to the database file
      subtl->log.commit_allocs(cdp);
  cdp->tl.log.commit_allocs(cdp);  // removes temp. locks, unlocks the index roots
  cdp->tl.commit_transaction_objects_post(cdp);  // no trans objects in the subtranlogs so far
  cdp->cevs.commit_events(cdp);
  tra_worms_free(cdp);
  return FALSE;

 err:
  { ProtectedByCriticalSection cs(&pg_lockbase.cs_locks, cdp, WAIT_CS_LOCKS);
    cdp->tl.lock_all_index_roots(cdp, false);  // unlocks locked index roots, data will be removed in rollback
  }  
 err0:
  if (cdp->nonempty_transaction())
  { char buf[81];
    trace_msg_general(cdp, TRACE_IMPL_ROLLBACK, form_message(buf, sizeof(buf), msg_implicit_rollback), NOOBJECT);
  }
  roll_back(cdp);
  return TRUE;
}

////////////////////////////////////// bps ////////////////////////////////
/* Debugging problem: When a routine evaluates a stored query (e.g. in a SELECT INTO statement) ans the query calls a stored function,
then the caller position in the function is not shown properly. The line number is the line containing the function call in the definition
of the query, but the current object is the original routine. 
As the query does not have its rscope the current object number cannot be set properly.
Dalsi projev: Kdyz se nejaka funkce zavola z ulozeneho dotazu, pak natrvalo poskodi informaci o pozici ve volajicim scope.

Stav: Ukazuje spravne i triggery a handlery na stacku, a to i tehdy, kdyz se provedl attachment
      Pri attachmentu vsak nemusi ukazat spravne misto volani triggeru a handleru.
*/

void cd_t::stk_drop(void)
// Supposes the non-empty stack
{ t_stack_item * s = stk;
  stk=s->next;
  if (!stk) 
    last_linechngstop = s->linechngstop;
  else  // when stopped on BP in subroutine and stepping out, must step in the outer routine too
    if (stk->linechngstop==LCHNGSTOP_NEVER && s->linechngstop!=LCHNGSTOP_NEVER)
      stk->linechngstop=LCHNGSTOP_ALWAYS;
  delete s;
  top_objnum = stk ? stk->objnum : NOOBJECT;  // cdp->stk==NULL when trigger not called from a procedure
}

void cd_t::stk_add(tobjnum objnumIn, char * name)
// name==NULL for handlers.
{
  t_stack_item * s = new t_stack_item;
  s->objnum=objnumIn;
  s->line=(uns32)-1;  // line not defined yet, will be assigned on the break check
  strcpy(s->rout_name, name ? name : "<Handler>");
  if (stk)
    if (stk->linechngstop==LCHNGSTOP_SAME_LEVEL) 
      s->linechngstop=LCHNGSTOP_NEVER;  // stepping over -> do not single-step in the inner level
    else
      s->linechngstop=stk->linechngstop;  // either stepping into or running: preserve the current state
  else
    if (name!=NULL)
      s->linechngstop=last_linechngstop;  // initial state: stop in the 1st line
    else
      s->linechngstop=LCHNGSTOP_NEVER;   // entering global handler called after the operation has been completed: do not stop unless BP found
  s->next=stk;
  stk=s;
}

void t_dbginfo::bp_add(tobjnum objnum, uns32 line, BOOL temp)
{ int pos=-1;  t_bp * bp;
 // look for the breakpoint or for a free space:
  for (int i=0;  i<bps.count();  i++)
  { bp = bps.acc0(i);
    if (bp->objnum==NOOBJECT) pos=i; // free space found
    if (bp->objnum==objnum && bp->line==line && bp->temporary==temp) return;  // this BP has already been set
  }
  if (pos==-1) bp=bps.next();  else bp=bps.acc0(pos);
  if (!bp) return;
  bp->objnum=objnum;  bp->line=line;  bp->temporary=temp;
}

BOOL t_dbginfo::bp_remove(tobjnum objnum, uns32 line, BOOL temp)
{ BOOL removed=FALSE;
  for (int i=0;  i<bps.count();  i++)
  { t_bp * bp = bps.acc0(i);
    if (bp->objnum==objnum && bp->line==line && bp->temporary==temp) 
      { bp->objnum=NOOBJECT;  removed=TRUE;  break; }
  }
  return removed;
}

void t_dbginfo::bp_remove_bps(BOOL temp_only)
{ for (int i=0;  i<bps.count();  i++)
  { t_bp * bp = bps.acc0(i);
    if (!temp_only || bp->temporary) bp->objnum=NOOBJECT;
  }
}

void t_dbginfo::check_break(uns32 line)
{ if (!cdp->stk || disabled) return;  // debuggable object not entered yet (e.g. when executing a line from the SQL console)
 // look for a breakpoint on the executed [line] in cdp->top_objnum: 
  bool bp_found = false;
  for (int i=0;  i<bps.count();  i++)
  { t_bp * bp = bps.acc0(i);
    if (bp->objnum==cdp->top_objnum && bp->line==line) 
      { bp_found=true;  break; }
  }
 // check to see if the line number has changed from the last break check:
  bool is_linechange = cdp->stk->line!=line || cdp->stk->objnum!=cdp->top_objnum;
  if (cdp->stk->linechngstop!=LCHNGSTOP_NEVER && cdp->top_objnum != NOOBJECT && is_linechange || // do not stop outside objects
      bp_found && (is_linechange || was_linechange))
  { cdp->stk->objnum=cdp->top_objnum;
    cdp->stk->line  =line;
    breaked=true;
    bp_remove_bps(TRUE);
    prof_time pre_wait_time = get_prof_time();
    WaitSemaph(&hDebugSem, INFINITE);
    breaked=false;  was_linechange=false;
    if (PROFILING_ON(cdp)) 
    { if (profiling_lines) cdp->time_since_last_line = get_prof_time();  // updating the start time after the waiting
      cdp->lower_level_time += cdp->time_since_last_line - pre_wait_time;
    }
  }
  else
  { if (is_linechange) was_linechange=true;  // allows stopping on a BP even on the same line
    cdp->stk->line=line;  // will be used for proper showing the line from which a trigger or handler has been entered
  }
}

void t_dbginfo::go(void)    
{ ReleaseSemaph(&hDebugSem, 1);  Sleep(250); }

void t_dbginfo::close(void)
{ bp_remove_bps(FALSE);
#if 0  //  cdp->dbginfo=NULL; should be sufficient
  linechngstop=LCHNGSTOP_NEVER;  
  for (t_rscope * rscope = cdp->rscope;  rscope;  rscope=rscope->next_rscope)
    // must remove upper_linechngstop from rscopes too!
    rscope->upper_linechngstop=LCHNGSTOP_NEVER;
#endif
  cdp->dbginfo=NULL;  // prevents next stopping the debugged thread
  if (breaked) go(); 
  else Sleep(250);
  delete this; // upper statement sleeps - so the debugged thread has time to stop working with dbginfo before it is deleted
}

void t_dbginfo::kill(void)    
// Will not terminate waiting for a semaphore or sleeping.
{ 
  //cdp->appl_state = SLAVESHUTDOWN; -- no, the client will disconnect, this would produce an error message on the client side
  cdp->break_request = wbtrue;           // terminates waiting for locks and normal operation
  cdp->cevs.cancel_event_if_waits(cdp);  // terminates waiting for events
  close(); 
}

void WINAPI sql_expression_entry(CIp_t CI)
{ sql_expression(CI, (t_express**)&CI->stmt_ptr); }


t_scope * create_scope_list_from_run_scopes(t_rscope *& rscope)
// Sestavi seznam statickych scopes z dynamickych scopes, cisla urovni musi byt sestupna
{ t_scope * sc, ** psc = &sc;
  int level_limit=1000;
  BOOL stop=FALSE;
  while (rscope && !stop)
  { if (rscope->level < level_limit) // use it
    { *psc = (t_scope*)rscope->sscope;  psc=&(*psc)->next_scope; 
      level_limit=rscope->level;
    }
    stop = !rscope->level;
    rscope=rscope->next_rscope; // rscope must be moved even for rscope->level==0
  } 
  *psc=NULL;
  return sc;
}

void t_dbginfo::eval(cdp_t rep_cdp, int level_limit, tptr source, cdp_t dbg_cdp, int dbgop)
{ tptr src = (tptr)corealloc(strlen(source)+30, 77);
  if (!src) { request_error(rep_cdp, OUT_OF_KERNEL_MEMORY);  return; }
  strcpy(src, "CAST(");  strcat(src, source);  strcat(src, dbgop==DBGOP_EVAL9 ? " AS nchar(4090))" : " AS char(254))");
#ifdef STOP
  compil_info xCI(cdp, src, sql_expression_entry);
  t_sql_kont sql_kont;  xCI.sql_kont=&sql_kont;
 // prepare static scopes acording to the run scopes:
  int level=0;
  do
  { t_rscope * rscope = dbg_cdp->rscope;
    while (rscope && rscope->level!=level) rscope=rscope->next_rscope;
    if (!rscope) break;
    rscope->sscope->activate(&xCI);  level++;
  } while (TRUE);
///////////////
  for (t_rscope * rscope = dbg_cdp->rscope;  rscope;  rscope=rscope->next_rscope)
  { // search for the static scope in the list:
     t_scope * sc = xCI.sql_kont->active_scope_list;
     while (sc && sc!=rscope->sscope) sc=sc->next_scope;
     if (sc) continue;  // already exists
     rscope->sscope->activate(&xCI);
  }
  int err=compile(&xCI);
  corefree(src);
  t_express * ex = (t_express*)xCI.stmt_ptr;
  if (err) { request_error(cdp, err);  delete ex;  return; }
#endif
 // trying to compile in various scopes
  t_rscope * rscope = dbg_cdp->rscope;  int err;  t_express * ex;
  t_rscope * start_rscope;
  do
  { compil_info xCI(rep_cdp, src, sql_expression_entry);
    t_sql_kont sql_kont;  xCI.sql_kont=&sql_kont;  sql_kont.from_debugger=true;
   // prepare compilation kontext with the database record access from the last active trigger:
    if (dbg_cdp->last_active_trigger_tabkont)
    { sql_kont.active_kontext_list=dbg_cdp->last_active_trigger_tabkont;  
      sql_kont.active_kontext_list->next=NULL; // proper correlation name defined in the kontext
    }
    start_rscope=rscope;
    sql_kont.active_scope_list = create_scope_list_from_run_scopes(rscope);
    err=compile_masked(&xCI);
    ex = (t_express*)xCI.stmt_ptr;
    if (err) delete ex;  else break;
  }
  while (rscope);
  corefree(src);
  if (err) { rep_cdp->ker_flags |=  KFL_HIDE_ERROR;  request_error(rep_cdp, err);  rep_cdp->ker_flags &= ~KFL_HIDE_ERROR;  return; }
 // evaluate:
  t_value res;
  rscope = dbg_cdp->rscope;  dbg_cdp->rscope=start_rscope;  // set the rscope accoring to the successfull sscope
  dbg_cdp->ker_flags |= KFL_HIDE_ERROR;
  expr_evaluate(dbg_cdp, ex, &res);
  dbg_cdp->ker_flags &= ~KFL_HIDE_ERROR;
  dbg_cdp->rscope = rscope;  // restore
  tptr output = res.is_null() ? NULLSTRING : res.valptr();
 // decorate:
  t_expr_cast * cex = (t_expr_cast *)ex;
  t_conv_type conv = cex->arg->convert.conv;
  if (dbgop==DBGOP_EVAL9)
  { wuchar * ptr;
    int len = (int)wuclen((wuchar*)output);
    if (len>MAX_FIXED_STRING_LENGTH) len=MAX_FIXED_STRING_LENGTH;
    if (conv==CONV_S2SEXPL || conv==CONV_S2S || conv==CONV_W2SEXPL || conv==CONV_W2S)
    { *(uns32*)rep_cdp->get_ans_ptr(sizeof(uns32)) = (2+len+1)*sizeof(wuchar);
      ptr=(wuchar*)rep_cdp->get_ans_ptr(sizeof(wuchar));  *ptr='\'';  
    }
    else if (conv==CONV_B2S)
    { *(uns32*)rep_cdp->get_ans_ptr(sizeof(uns32)) = (3+len+1)*sizeof(wuchar);
      ptr=(wuchar*)rep_cdp->get_ans_ptr(sizeof(wuchar));  *ptr='X';  
      ptr=(wuchar*)rep_cdp->get_ans_ptr(sizeof(wuchar));  *ptr='\'';  
    }
    else
      *(uns32*)rep_cdp->get_ans_ptr(sizeof(uns32)) = (len+1)*sizeof(wuchar);
    ptr=(wuchar*)rep_cdp->get_ans_ptr(len*sizeof(wuchar));  
    memcpy(ptr, output, sizeof(wuchar)*len);
    if (conv==CONV_S2SEXPL || conv==CONV_S2S || conv==CONV_W2SEXPL || conv==CONV_W2S || conv==CONV_B2S)
    { ptr=(wuchar*)rep_cdp->get_ans_ptr(sizeof(wuchar));  *ptr='\'';  
    }
    ptr=(wuchar*)rep_cdp->get_ans_ptr(sizeof(wuchar));  *ptr=0;  
  }
  else
  { char buf[3+256+1];
    if (conv==CONV_S2SEXPL || conv==CONV_S2S || conv==CONV_W2SEXPL || conv==CONV_W2S)
    { *buf='\'';  
      if (strlen(output)>=254)
        { memcpy(buf+1, output, 254-3);  strcpy(buf+1+254-3, "..."); }
      else strcpy(buf+1, output);  
      strcat(buf, "\'"); 
    }
    else if (conv==CONV_B2S)
    { strcpy(buf, "X\'");  
      if (strlen(output)>=254)
        { memcpy(buf+2, output, 254-3);  strcpy(buf+2+254-3, "..."); }
      else strcpy(buf+2, output);  
      strcat(buf, "\'"); 
    }
    else
      strcpy(buf, output);
    int len = (int)strlen(buf)+1;  
    tptr ans=rep_cdp->get_ans_ptr(sizeof(uns32)+len);
    if (ans) 
      { *(uns32*)ans=len;  strcpy(ans+sizeof(uns32), buf); }
  }
  delete ex;
}

void t_dbginfo::assign(cdp_t rep_cdp, int level_limit, tptr source, tptr value, cdp_t dbg_cdp)
{ tptr src = (tptr)corealloc(strlen(source)+strlen(value)+10, 77);
  if (!src) { request_error(rep_cdp, OUT_OF_KERNEL_MEMORY);  return; }
  strcpy(src, "SET ");  strcat(src, source);  strcat(src, "=");  strcat(src, value);
 // trying to compile in various scopes
  t_rscope * rscope = dbg_cdp->rscope;  int err;  sql_statement * so;
  t_rscope * start_rscope;
  do
  { compil_info xCI(rep_cdp, src, sql_statement_1);
    t_sql_kont sql_kont;  xCI.sql_kont=&sql_kont;
   // prepare compilation kontext with the database record access from the last active trigger:
    if (dbg_cdp->last_active_trigger_tabkont)
    { sql_kont.active_kontext_list=dbg_cdp->last_active_trigger_tabkont;  
      sql_kont.active_kontext_list->next=NULL; // proper correlation name defined in the kontext
    }
   // stack scope kontext:
    start_rscope=rscope;
    sql_kont.active_scope_list = create_scope_list_from_run_scopes(rscope);
    err=compile_masked(&xCI);
    so = (sql_statement*)xCI.stmt_ptr;
    if (err) delete so;  else break;
  }
  while (rscope);
  corefree(src);
  if (err) { request_error(rep_cdp, err);  return; }
  so->exec(dbg_cdp);
  delete so;
}

void t_dbginfo::stack(cdp_t cdp, cdp_t dbg_cdp)
{ t_stack_info * sti;
  cdp->answer.sized_answer_start();
  for (t_stack_item * s = cdp->stk;  s;  s=s->next)
  { sti = (t_stack_info*)cdp->get_ans_ptr(sizeof(t_stack_info));
#if 0
    sti->objnum=stk->objnum;
    sti->line  =stk->line;
    t_rscope * rscope = dbg_cdp->rscope;
    while (rscope)
    { if (rscope->sscope->params==TRUE && rscope->upper_objnum!=NOOBJECT &&   // routines only, no triggers
          rscope->level!=-1)               // prevents displaying the routine on the stack when the values of its input parametrs are being calculated (and functions may be called)
      { sti = (t_stack_info*)cdp->get_ans_ptr(sizeof(t_stack_info));
        sti->objnum=rscope->upper_objnum;
        sti->line  =rscope->upper_line;
      }
      rscope=rscope->next_rscope;
    }
#else
    sti->objnum = s->objnum;
    sti->line = s->line;
#endif
  }
  sti = (t_stack_info*)cdp->get_ans_ptr(sizeof(t_stack_info));
  sti->objnum=0;
  cdp->answer.sized_answer_stop();
}

void t_dbginfo::stack9(cdp_t rep_cdp)
{ t_stack_info9 * sti;
  rep_cdp->answer.sized_answer_start();
  for (t_stack_item * s = cdp->stk;  s;  s=s->next)
  { sti = (t_stack_info9*)rep_cdp->get_ans_ptr(sizeof(t_stack_info9));
#if 0
    sti->objnum=stk->objnum;
    sti->line  =stk->line;
    sti->rout_name[0]=0;
    t_rscope * rscope = dbg_cdp->rscope;
    while (rscope)
    { if (rscope->sscope->params &&  // params!=0 for outmost scopes of routines and triggers
          rscope->level!=-1)         // prevents displaying the routine on the stack when the values of its input parametrs are being calculated (and functions may be called)
      {// find last routine name:
        if (rscope->sscope->params==2) strcpy(sti->rout_name, "<Trigger>");
        else get_routine_name(rscope->sscope, sti->rout_name);
       // get next pos:
        if (rscope->upper_objnum!=NOOBJECT)
        { sti = (t_stack_info9*)rep_cdp->get_ans_ptr(sizeof(t_stack_info9));
          sti->objnum=rscope->upper_objnum;
          sti->line  =rscope->upper_line;
          sti->rout_name[0]=0;
        }
     
      }
      rscope=rscope->next_rscope;
    }
#else
    sti->objnum = s->objnum;
    sti->line = s->line;
    strcpy(sti->rout_name, s->rout_name);
#endif
  }
  sti = (t_stack_info9*)rep_cdp->get_ans_ptr(sizeof(t_stack_info9));
  sti->objnum=0;
  rep_cdp->answer.sized_answer_stop();
}

BOOL find_in_list(sql_statement * stmts, uns32 & line);

static BOOL find_in_decls(t_locdecl_dynar * locdecls, uns32 & line)
{ for (int i=0;  i<locdecls->count();  i++)
  { t_locdecl * locdecl = locdecls->acc0(i);
    switch (locdecl->loccat)
    { case LC_ROUTINE:
        if (find_in_list(locdecl->rout.routine->stmt, line)) return TRUE;
        break;
      case LC_HANDLER:
        if (find_in_list(locdecl->handler.stmt, line)) return TRUE;
        break;
    }
  }
  return FALSE;
}

BOOL find_in_list(sql_statement * stmts, uns32 & line)
{ if (!stmts) return FALSE;
  do
  { if (stmts->source_line==line) return TRUE;
    if (stmts->source_line >line) { line=stmts->source_line;  return TRUE; }
    if (!stmts->next_statement || stmts->next_statement->source_line > line) break;
    stmts=stmts->next_statement;
  } while (TRUE);
 // the next statement does not exist or has a bigger line number then the searched one
  switch (stmts->statement_type)
  { case SQL_STAT_BLOCK:
    { sql_stmt_block * blst = (sql_stmt_block*)stmts;
     // search in local objects:
      if (find_in_decls(&blst->scope.locdecls, line)) return TRUE;
     // search in body:
      if (find_in_list(blst->body, line)) return TRUE;
      if (blst->source_line2 >= line)
        { line=blst->source_line2;  return TRUE; }
      break;
    }
    case SQL_STAT_IF:
    { sql_stmt_if * ifst = (sql_stmt_if*)stmts;
      if (find_in_list(ifst->then_stmt, line)) return TRUE;
      if (find_in_list(ifst->else_stmt, line)) return TRUE;
      break;
    }
    case SQL_STAT_LOOP:
    { sql_stmt_loop * loopst = (sql_stmt_loop*)stmts;
      if (find_in_list(loopst->stmt, line)) return TRUE;
      if (loopst->source_line2 >= line)
        { line=loopst->source_line2;  return TRUE; }
      break;
    }
    case SQL_STAT_FOR:
    { sql_stmt_for * forst = (sql_stmt_for*)stmts;
      if (find_in_list(forst->body, line)) return TRUE;
      if (forst->source_line2 >= line)
        { line=forst->source_line2;  return TRUE; }
      break;
    }
    case SQL_STAT_CASE:
    { sql_stmt_case * casest = (sql_stmt_case*)stmts;
      if (find_in_list(casest->branch, line)) return TRUE;
      if (find_in_list(casest->contin, line)) return TRUE;
      break;
    }
  }
 // line not found inside the statement, return the next or fail:
  if (stmts->next_statement)
  { line=stmts->next_statement->source_line;
    return TRUE;
  }
  return FALSE;
}

BOOL find_break_line(cdp_t cdp, tobjnum objnum, uns32 & line)
{ tcateg cat;  tobjname name;  BOOL found;
  if (fast_table_read(cdp, objtab_descr, objnum, OBJ_CATEG_ATR, &cat)) return FALSE;
  if (cat==CATEG_PROC)
  { fast_table_read(cdp, objtab_descr, objnum, OBJ_NAME_ATR, name);
    if (!stricmp(name, MODULE_GLOBAL_DECLS))
    { WBUUID uuid;
      fast_table_read(cdp, objtab_descr, objnum, APPL_ID_ATR, uuid);
      t_global_scope * gs = find_global_sscope(cdp, uuid);
      if (!gs) return false;
      found=find_in_decls(&gs->locdecls, line);
      //delete gs;  -- shared!
    }
    else 
    { t_routine * routine = get_stored_routine(cdp, objnum, NULL, TRUE);
      if (!routine) return FALSE;
      found=find_in_list(routine->stmt, line);
      psm_cache.put(routine); // return the routine to the cache
    }
  }
  else if (cat==CATEG_TRIGGER)
  { t_trigger * trig = obtain_trigger(cdp, objnum);
    if (!trig) return FALSE;
    found=find_in_list(trig->stmt, line);
    psm_cache.put(trig);    // return the trigger to the cache
  }
  else found=FALSE;
  return found;
}

void debug_command(cdp_t cdp, tptr & request)
{ uns8 dbgop;  tobjnum objnum;  uns32 line;  cdp_t dbg_cdp;
  dbgop=*request;  request++;  
  if (dbgop!=DBGOP_EVAL && dbgop!=DBGOP_EVAL9 && dbgop!=DBGOP_ASSIGN && dbgop!=DBGOP_STACK && dbgop!=DBGOP_STACK9)
  { objnum=*(tobjnum*)request;  request+=sizeof(tobjnum);  
    line=*(uns32*)request;  request+=sizeof(uns32);
  }
  dbg_cdp=NULL;
  if (dbgop==DBGOP_GET_USER)
  { tptr p=cdp->get_ans_ptr(2*sizeof(tobjname)+sizeof(uns32));
    { ProtectedByCriticalSection cs(&cs_client_list, cdp, WAIT_CS_CLIENT_LIST);  
      for (int i=1;  i <= max_task_index;  i++) 
        if (cds[i] && cds[i]->in_use!=PT_WORKER)
          if (!objnum--) { dbg_cdp=cds[i];  break; }
      if (!dbg_cdp) *p=0;
      else 
      { strcpy(p, dbg_cdp->prvs.luser_name());  p+=sizeof(tobjname);
        strcpy(p, dbg_cdp->sel_appl_name);       p+=sizeof(tobjname);
        *(uns32*)p = dbg_cdp==cdp ? (uns32)WAIT_ME : dbg_cdp->wait_type;
      }
    }
  }
  else if (dbgop==DBGOP_INIT) // search by position (up to version 8)
  { { ProtectedByCriticalSection cs(&cs_client_list, cdp, WAIT_CS_CLIENT_LIST);  
      for (int i=1;  i <= max_task_index;  i++) 
        if (cds[i] && cds[i]!=cdp && cds[i]->in_use!=PT_WORKER) 
          if (!objnum--) { dbg_cdp=cds[i];  break; }
      if (dbg_cdp) 
      { if (!dbg_cdp->dbginfo) dbg_cdp->dbginfo=new t_dbginfo(dbg_cdp);
        //if (dbg_cdp->dbginfo)
        //  dbg_cdp->dbginfo->linechngstop=LCHNGSTOP_NEVER;
      }
    }
    cdp->get_ans_ptr(sizeof(uns32));  // result not used
    if (!dbg_cdp) { request_error(cdp, OBJECT_NOT_FOUND);  return; }  // outside the CS!
  }
  else if (dbgop==DBGOP_INIT2)  // search by client number (since version 9)
  {// check for other debugging in progress:
    dbg_cdp=NULL;
    { ProtectedByCriticalSection cs(&cs_client_list, cdp, WAIT_CS_CLIENT_LIST);  
      for (int i=1;  i <= max_task_index;  i++) if (cds[i] && cds[i]->dbginfo) 
          { dbg_cdp=cds[i];  break; }
    }
    if (dbg_cdp)      
      { request_error(cdp, INTERNAL_SIGNAL);  return; }
   // find the debuggee:   
    { ProtectedByCriticalSection cs(&cs_client_list, cdp, WAIT_CS_CLIENT_LIST);  
      for (int i=1;  i <= max_task_index;  i++) 
        if (cds[i] && cds[i]->applnum_perm==objnum) 
          { dbg_cdp=cds[i];  break; }
      if (dbg_cdp) 
      { if (!dbg_cdp->dbginfo) dbg_cdp->dbginfo=new t_dbginfo(dbg_cdp);
        //if (dbg_cdp->dbginfo)
        //{ dbg_cdp->dbginfo->linechngstop=LCHNGSTOP_ALWAYS;  
        //  dbg_cdp->dbginfo->curr_line=(uns32)-1;  // will stop on the 1st executed line
        //}
      }
    }
    cdp->get_ans_ptr(sizeof(uns32));  // result not used
    if (!dbg_cdp) { request_error(cdp, OBJECT_NOT_FOUND);  return; }  // outside the CS!
  }
  else // all others
  { { ProtectedByCriticalSection cs(&cs_client_list, cdp, WAIT_CS_CLIENT_LIST);  
      for (int i=1;  i <= max_task_index;  i++) if (cds[i] && cds[i]->dbginfo) 
        { dbg_cdp=cds[i];  break; }
    } // not completly safe, not protected againts disconnecting dbg_cdp in the following operations
    if (!dbg_cdp) 
    { if (dbgop==DBGOP_GET_STATE)
      { tptr p=cdp->get_ans_ptr(sizeof(tobjnum)+2*sizeof(uns32));
        p+=sizeof(tobjnum)+sizeof(uns32);  *(uns32*)p=DBGSTATE_NOT_FOUND; 
      }
      else 
        request_error(cdp, OBJECT_NOT_FOUND);  
      return; 
    }
    t_dbginfo * dbginfo = dbg_cdp->dbginfo;
    if (dbgop==DBGOP_EVAL || dbgop==DBGOP_EVAL9)
    { int level = *(uns32*)request;  request+=sizeof(uns32);
      tptr source=request;  request+=strlen(request)+1;
      dbginfo->disable();  // prevents stopping on the BREAK semaphore
      dbginfo->eval(cdp, level, source, dbg_cdp, dbgop);
      dbginfo->enable();
    }
    else if (dbgop==DBGOP_ASSIGN)
    { int level = *(uns32*)request;  request+=sizeof(uns32);
      tptr dest=request;  request+=strlen(request)+1;
      tptr value=request; request+=strlen(request)+1;
      dbginfo->disable();  // prevents stopping on the BREAK semaphore
      dbginfo->assign(cdp, level, dest, value, dbg_cdp);
      dbginfo->enable();
    }
    else if (dbgop==DBGOP_STACK)
      dbginfo->stack(cdp, dbg_cdp);
    else if (dbgop==DBGOP_STACK9)
      dbginfo->stack9(cdp);
    else
    { tptr p=cdp->get_ans_ptr(sizeof(tobjnum)+2*sizeof(uns32));
      switch (dbgop)
      { case DBGOP_ADD_BP:
          if (!find_break_line(cdp, objnum, line)) 
            { request_error(cdp, OBJECT_NOT_FOUND);  break; }
          dbginfo->bp_add(objnum, line, FALSE);
          *(tobjnum*)p = objnum;  p+=sizeof(tobjnum);
          *(uns32  *)p = line;    p+=sizeof(uns32);
          break;
        case DBGOP_REMOVE_BP:
          if (!find_break_line(cdp, objnum, line)) 
            { request_error(cdp, OBJECT_NOT_FOUND);  break; }
          if (!dbginfo->bp_remove(objnum, line, FALSE))
            { request_error(cdp, OBJECT_NOT_FOUND);  break; } // this error is necessary to implement "toggle"
          *(tobjnum*)p = objnum;  p+=sizeof(tobjnum);
          *(uns32  *)p = line;    p+=sizeof(uns32);
          break;
        case DBGOP_GO:
        { dbginfo->bp_remove_bps(TRUE);  // removes all temp. BPs
#if 0
          dbginfo->linechngstop=LCHNGSTOP_NEVER;  
         // remove upper_linechngstop from rscopes too! (added in 9.0)
          for (t_rscope * rscope = dbg_cdp->rscope;  rscope;  rscope=rscope->next_rscope)
            rscope->upper_linechngstop=LCHNGSTOP_NEVER;
#else
          for (t_stack_item * s = dbg_cdp->stk;  s;  s=s->next)
            s->linechngstop=LCHNGSTOP_NEVER;
#endif
          dbginfo->go();  break;
        }
        case DBGOP_STEP:
          dbginfo->bp_remove_bps(TRUE);  // removes all temp. BPs
          dbg_cdp->stk->linechngstop=LCHNGSTOP_SAME_LEVEL; // will be changed to LCHNGSTOP_NEVER when going down and restored on return
          dbginfo->go();  break;
        case DBGOP_TRACE:
          dbginfo->bp_remove_bps(TRUE);  // removes all temp. BPs
          dbg_cdp->stk->linechngstop=LCHNGSTOP_ALWAYS;  
          dbginfo->go();  break;
        case DBGOP_OUTER:
          dbginfo->bp_remove_bps(TRUE);  // removes all temp. BPs
          if (!dbg_cdp->stk->next) break;
          dbg_cdp->stk->next->linechngstop=LCHNGSTOP_ALWAYS;  dbg_cdp->stk->line=0;
          dbg_cdp->stk->linechngstop=LCHNGSTOP_NEVER;  
          dbginfo->go();  break;
          break;
        case DBGOP_GOTO:
          dbginfo->bp_remove_bps(TRUE);  // removes all temp. BPs
          dbginfo->bp_add(objnum, line, TRUE);  
          dbg_cdp->stk->linechngstop=LCHNGSTOP_NEVER;  
          dbginfo->go();  break;
        case DBGOP_CLOSE:
          dbginfo->close();
          break;
        case DBGOP_KILL:
          dbginfo->kill();
          //break;  -- no, dbg_cdp becomes invalid
          *(uns32  *)p = DBGSTATE_NOT_FOUND;
          return;
        case DBGOP_BREAK:
          if (dbg_cdp->stk)
          { dbg_cdp->stk->linechngstop=LCHNGSTOP_ALWAYS;  
            dbg_cdp->stk->line=(uns32)-1;  // will stop on the next executed line
          }
          break;
      }
      if (dbgop!=DBGOP_ADD_BP && dbgop!=DBGOP_REMOVE_BP)
      { *(tobjnum*)p = dbg_cdp->stk ? dbg_cdp->stk->objnum : NOOBJECT;  p+=sizeof(tobjnum);
        *(uns32  *)p = dbg_cdp->stk ? dbg_cdp->stk->line   : 0;         p+=sizeof(uns32);
      }
      *(uns32  *)p = dbginfo->breaked ? DBGSTATE_BREAKED : dbg_cdp->appl_state==WAITING_FOR_REQ ? DBGSTATE_NO_REQUEST : DBGSTATE_EXECUTING;
    } // !DBGOP_EVAL, ASSIGN, STACK
  }
}
