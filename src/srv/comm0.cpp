/* comm0.c */
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
#include "netapi.h"
#include "nlmtexts.h"
#include <time.h>
#ifndef UNIX
#include <process.h>
#endif
#include <fcntl.h>   /* O_BINARY */

#include "baselib.cpp"     
/*********************** WINS cs *******************************/
sig32 cmp_keys(const char * key1, const char * key2, dd_index * idx_descr)
/* compares the keys described by the idx_descr, returns -1,0,1 in LOWORD. */
/* HIWORD is 1 iff keys are the same except for record number */
/* Cannot return 0. */
{ unsigned total, i;  int res;
  total=0;  i=0;
  const part_desc * apart = idx_descr->parts;
  do
  { int sz=idx_descr->parts[i].partsize;
	  switch (apart->type)
	  {case ATT_BOOLEAN: res=(int)*(signed char*)key1-(int)*(signed char*)key2; break;
		 case ATT_CHAR:    res=(int)(uns8)*key1-(int)(uns8)*key2; break;
     case ATT_INT8:    res=(*(sig8 *)key1< *(sig8 *)key2) ? -1 :
								         ( (*(sig8 *)key1==*(sig8 *)key2) ? 0 : 1);  break;
		 case ATT_INT16:   res=(*(sig16 *)key1< *(sig16 *)key2)     ? -1 :
								         ( (*(sig16 *)key1==*(sig16 *)key2) ? 0 : 1);  break;
		 case ATT_MONEY:   res=cmpmoney((monstr *)key1, (monstr*)key2);  break;
		 case ATT_TIME:  case ATT_DATE:  case ATT_DATIM:  case ATT_TIMESTAMP:
       if        (*(uns32 *)key1==NONEDATE) res = *(uns32 *)key2==NONEDATE? 0 : -1;
       else if   (*(uns32 *)key2==NONEDATE) res = 1;
       else res=((*(uns32 *)key1< *(uns32 *)key2) ? -1 :
		           ( (*(uns32 *)key1==*(uns32 *)key2) ? 0 : 1));  break;
		 case ATT_INT32:   res=((*(sig32 *)key1< *(sig32 *)key2) ? -1 :
								          ( (*(sig32 *)key1==*(sig32 *)key2) ? 0 : 1));  break;
		 case ATT_INT64:   res=((*(sig64 *)key1< *(sig64 *)key2) ? -1 :
								          ( (*(sig64 *)key1==*(sig64 *)key2) ? 0 : 1));  break;
		 case ATT_PTR:    case ATT_BIPTR:
								 res=((*(sig32 *)key1<*(sig32 *)key2)     ? -1 :
								 ( (*(sig32 *)key1==*(sig32 *)key2) ? 0 : 1));  break;
		 case ATT_FLOAT:   res=((*(double*)key1<*(double*)key2) ? -1 :
								 ( (*(double*)key1==*(double*)key2) ? 0 : 1));  break;
		 case ATT_STRING:  
       res=cmp_str(key1, key2, apart->specif);  break;
     case ATT_BINARY:
		 { res=0;  const uns8 * s1=(uns8*)key1, * s2=(uns8*)key2;  int maxlen=sz;
		   while (maxlen--)
         if (*s1!=*s2)
           { res = *s1 < *s2 ? -1 : 1;  break; }
         else
           { s1++;  s2++; }
       break;
     }
     default:
       res=0;  break;
    }
    if (res) return MAKELONG(idx_descr->parts[i].desc ? -res : res, 0);
    key1+=sz;  key2+=sz;  total+=sz;  apart++;
  } while (++i < idx_descr->partnum);
  do
  { if (*(trecnum*)key1 > *(trecnum*)key2) return MAKELONG(1,  1);
    if (*(trecnum*)key1 < *(trecnum*)key2) return MAKELONG(-1, 1);
	  key1+=sizeof(trecnum);  key2+=sizeof(trecnum);  total+=sizeof(trecnum);
  } while (total < idx_descr->keysize);
  return MAKELONG(0, 1);
}

sig32 cmp_keys_partial(const char * key1, const char * key2, dd_index * idx_descr, int compare_parts)
// If HIWORD(returned) == 1 then LOWORD is undefined!
{ if (!compare_parts || compare_parts>=idx_descr->partnum)
    return cmp_keys(key1, key2, idx_descr);
  int orig_partnum = idx_descr->partnum;
  idx_descr->partnum=compare_parts;
  sig32 res = cmp_keys(key1, key2, idx_descr);
  idx_descr->partnum = orig_partnum;
  return res;
}

/************************** date & time *************************************/

#ifndef SRVR
int _fmode=O_BINARY;
#endif

int WINAPI typesize(const attribdef * pdatr)
{ return TYPESIZE(pdatr); }

//////////////////////////////////// info //////////////////////////////////////
BOOL get_info_value(const char * info, const char * section, const char * keyname, int type, void * value)
// Read a value with given keyname from info. If !*section, looks for the value before the 1st section.
// Does not write *value and returns FALSE if section/keyname not found
// Values in info are separated by ; or (CR)LF
// Section name, eyname and value can contain spaces, but spaces on the beginning and the end are ignored.
// Only ATT_INT32 and ATT_STRING types supported.
{ tobjname current_section = "", current_key;  int i;
  if (!info) return FALSE;
  enum { max_value_len=200 };  char value_buffer[max_value_len+1];
  do
  { while (*info <= ' ' || *info == ';') 
    { if (!*info) return FALSE;
      info++;
    }
    if (*info=='[')  // read the name of the new section
    { while (*info == ' ' || *info == '\t') info++;
      i=0;
      while (*info!=']' && *info>=' ' && *info!=';')
      { if (i<OBJ_NAME_LEN) current_section[i++]=*info;
        info++;
      }
      current_section[i]=0;  cutspaces(current_section);
      while (*info == ' ' || *info == '\t') info++;
      if (*info==']') info++;
    }
    else // read keyname/value
    { i=0;  // read the keyname
      while (*info!='=' && *info>=' ' && *info!=';')
      { if (i<OBJ_NAME_LEN) current_key[i++]=*info;
        info++;
      }
      current_key[i]=0;  cutspaces(current_key);
      while (*info == ' ' || *info == '\t') info++;
      if (*info=='=')  // read the value
      { info++;
        while (*info == ' ' || *info == '\t') info++;
        i=0;
        while (*info && *info>'\r' && *info>'\n' && *info!=';')
        { if (i<max_value_len) value_buffer[i++]=*info;
          info++;
        }
        value_buffer[i]=0;
       // check the section/keyname and convert & store the value
        if (!sys_stricmp(section, current_section) && !sys_stricmp(keyname, current_key))
        { if (type<0)
            strmaxcpy((tptr)value, value_buffer, -type);
          else switch (type)
          { case ATT_INT32:
              if (!str2int(value_buffer, (sig32*)value)) *(sig32*)value=NONEINTEGER;  break;
            case ATT_STRING:  
              strcpy((tptr)value, value_buffer);  break;
            default:
              return FALSE;  // other types not supported
          }
          return TRUE;
        }
      }
    }
  } while (TRUE);
}

int d_curs_size(d_table * tdd)
{ int i, maxpref = -1;  d_attr * pattr;
  if (tdd==NULL) return 0;
  for (i=0, pattr=FIRST_ATTR(tdd);  i<tdd->attrcnt;  i++, pattr=NEXT_ATTR(tdd,pattr))
    //if (pattr->needs_prefix) -- all attributes have prefnum
      if (pattr->prefnum > maxpref) maxpref=pattr->prefnum;
  return sizeof(d_table)+sizeof(d_attr)*tdd->attrcnt+(maxpref+1)*2*sizeof(tobjname);
}

d_table * kernel_get_descr(cdp_t cdp, tcateg cat, tobjnum tbnum, unsigned *psize)
/* Can be called from kernel only! */
{ d_table * tdd;  unsigned size;  d_attr * att;  int i;
  if (cat==CATEG_TABLE)   /* create d_table from table_descr */
  { table_descr_auto td(cdp, tbnum);
    if (td->me()==NULL) return NULL;
    *psize=size=sizeof(d_table)+(td->attrcnt-1)*sizeof(d_attr);
    tdd=(d_table*)corealloc(size, 47);
    if (tdd==NULL)
    { request_error(cdp, OUT_OF_KERNEL_MEMORY);  
      return NULL;
    }
   /* moving global table info: */
    tdd->attrcnt=td->attrcnt;
    tdd->tabcnt=0;
    tdd->updatable = QE_IS_UPDATABLE | QE_IS_DELETABLE | QE_IS_INSERTABLE;
    tdd->tabdef_flags=td->tabdef_flags;
    strcpy(tdd->selfname,   td->selfname);
    if (!*td->schemaname) get_table_schema_name(cdp, tbnum, td->schemaname);
    strcpy(tdd->schemaname, td->schemaname);
   /* move attribute info: */
    const attribdef * at;
    for (i=0, att=tdd->attribs, at=td->attrs;  i<td->attrcnt;  i++, att++, at++)
    { strcpy(att->name, at->name);  
      att->type=at->attrtype;  att->multi=at->attrmult;  att->nullable=at->nullable;  att->specif=at->attrspecif;
      att->needs_prefix=wbfalse;
      if (at->defvalexpr==COUNTER_DEFVAL) att->a_flags = NOEDIT_FLAG;
      else att->a_flags = 0;
    }
  }
  else if (cat==CATEG_CURSOR)
  { tptr buf=ker_load_objdef(cdp, objtab_descr, tbnum);
    if (!buf) return NULL;
   // dynamic parameters are not supposed to be in the stored cursor
    WBUUID schema_uuid;  sql_statement * so;
    tb_read_atr(cdp, objtab_descr, tbnum, APPL_ID_ATR, (tptr)schema_uuid);
    { t_short_term_schema_context stsc(cdp, schema_uuid);
      unsigned old_flags = cdp->ker_flags;
      cdp->ker_flags |= KFL_NO_OPTIM;
      so=sql_submitted_comp(cdp, buf);
      cdp->ker_flags = old_flags;
    }
    corefree(buf);
    if (so==NULL) return NULL;  // error reported
    if (so->next_statement != NULL || // multiple statements submitted! */
        so->statement_type != SQL_STAT_SELECT ||  /* not a select statement */
        ((sql_stmt_select*)so)->is_into)
      { delete so;  request_error(cdp, SELECT_EXPECTED);  return NULL; }
    tdd=create_d_curs(cdp, ((sql_stmt_select*)so)->qe);
    delete so;
    *psize=d_curs_size(tdd);
    if (!tdd) { request_error(cdp, OUT_OF_KERNEL_MEMORY);  return NULL; }
  }
  else if (cat==CATEG_DIRCUR) /* direct cursor */
  { cur_descr * cd = get_cursor(cdp, tbnum);
    if (!cd) return NULL; 
    size=*psize=d_curs_size(cd->d_curs);
    tdd=(d_table*)corealloc(size, 47);
    if (tdd==NULL) { request_error(cdp, OUT_OF_KERNEL_MEMORY);  return NULL; }
    memcpy(tdd, cd->d_curs, size);
  }
  else 
  { request_error(cdp, ERROR_IN_FUNCTION_ARG);  // error must be raised!
    tdd=NULL;
  }
 // convert server types to client types:
  if (tdd)
    for (i=0, att=tdd->attribs;  i<tdd->attrcnt;  i++, att++)
      if (att->type==ATT_EXTCLOB) att->type=ATT_TEXT;
      else if (att->type==ATT_EXTBLOB) att->type=ATT_NOSPEC;
  return tdd;
}

//////////////////////////////// schema UUID cache ///////////////////////////////////////////////////
/* This cache makes translations of schema UUIDs into schema names faster. When schemas are created,
deleted, updated, the cache contents may become obsolete. The cache content is therefore used as 
a hint only. Every value taken from the cache is verified in OBJTAB. */

struct t_schema_cache_item
{ tobjname schema_name;  // schema name is in upper case here
  WBUUID   schema_uuid;
  tobjnum  objnum;  // schema object number in OBJTAB
};
 
SPECIF_DYNAR(t_schema_cache_dynar, t_schema_cache_item, 5); 

static t_schema_cache_dynar schema_cache_dynar;
// Cache item is empty when *scheman_name==0.

static bool verify_values_from_cache(cdp_t cdp, const WBUUID schema_uuid, const char * schema_name, tobjnum objnum)
// Must not cause errors when the record is deleted/released!
{ bool res;
 // can cause errors:
 // WBUUID real_uuid;  tobjname real_name;  
 // fast_table_read(cdp, objtab_descr, objnum, OBJ_NAME_ATR, real_name);
 // fast_table_read(cdp, objtab_descr, objnum, APPL_ID_ATR,  real_uuid);
  if (objnum>=objtab_descr->Recnum()) return false;  // prevent the error when a schema is dropped
  t_table_record_rdaccess rda(cdp, objtab_descr);
  const ttrec * dt = (const ttrec*)rda.position_on_record(objnum, 0);
  if (!dt) return false;
  if (dt->del_mark!=NOT_DELETED)
    res=false;
  else  
    res=!memcmp(schema_uuid, dt->apluuid, UUID_SIZE) && !strcmp(schema_name, dt->name);
  return res;
}

static void put_values_to_cache(const WBUUID schema_uuid, const char * schema_name, tobjnum objnum)
// Removes all conflicting items from the cache and inserts the specified values, when objnum!=NOOBJECT.
// Removes all conflicting or identical items from the cache, when objnum==NOOBJECT.
{ bool done = false;
  ProtectedByCriticalSection cs(&cs_short_term);
  for (int i=0;  i<schema_cache_dynar.count() || !done && objnum!=NOOBJECT && i==schema_cache_dynar.count();  i++)
  { t_schema_cache_item * sci = schema_cache_dynar.acc(i);  // must not use acc0 here!
    if (!*sci->schema_name ||  // empty item
        !memcmp(schema_uuid, sci->schema_uuid, UUID_SIZE) || !strcmp(schema_name, sci->schema_name))  // conflicting or identical item
      if (done)  
        *sci->schema_name=0;
      else  
      { if (objnum==NOOBJECT)  // just removing
          *sci->schema_name=0;
        else  
        { strcpy(sci->schema_name, schema_name);
          memcpy(sci->schema_uuid, schema_uuid, UUID_SIZE);
          sci->objnum = objnum;
          done=true;
        }  
      }
  }    
}

bool ker_apl_name2id(cdp_t cdp, const char * name, uns8 * appl_id, tobjnum * aplobj)
// Converts schema name to schema uuid and/or schema object number.
// Returns false iff schema with the name exists.
{ tobjname uc_objname;  tobjnum objnum = NOOBJECT;  WBUUID found_uuid;
  strmaxcpy(uc_objname, name, sizeof(uc_objname));  
  sys_Upcase(uc_objname);  
 // search the cache first:
  { ProtectedByCriticalSection cs(&cs_short_term);
    for (int i=0;  i<schema_cache_dynar.count();  i++)
    { const t_schema_cache_item * sci = schema_cache_dynar.acc0(i);
      if (!strcmp(uc_objname, sci->schema_name))  // item is valid and name found
      { memcpy(found_uuid, sci->schema_uuid, UUID_SIZE);
        objnum = sci->objnum;
        break;
      }
    }    
  }
 // if the schema was found in the cache, verify it:
  if (objnum!=NOOBJECT)
    if (verify_values_from_cache(cdp, found_uuid, uc_objname, objnum))
    { if (appl_id) memcpy(appl_id, found_uuid, UUID_SIZE);
      if (aplobj) *aplobj=objnum;
      return false;
    }  
 // search the OBJTAB:
  objtab_key search_key, found_key;
  strcpy(search_key.objname, uc_objname);  
  search_key.categ=CATEG_APPL;  // must be after writing objname because it does not have the terminator in the search_key
  memset(search_key.appl_id, 0, UUID_SIZE);  // minimal value
  bool found;
  if (!find_closest_key(cdp, objtab_descr, 1, &search_key, &found_key) || found_key.categ!=CATEG_APPL)
    found=false;
  else  
  { found_key.categ=0;  // this appends the terminator to found_key .objname
    found=!sys_stricmp(uc_objname, found_key.objname);  // ignoring case is coherent with the string type, important when schema name is in lower case by mistake
  }
 // put the search result (even negative) into the cache:
  put_values_to_cache(found_key.appl_id, uc_objname, found ? (tobjnum)found_key.recnum : NOOBJECT);
  if (!found) return true;
  if (appl_id) memcpy(appl_id, found_key.appl_id, UUID_SIZE);
  if (aplobj) *aplobj=found_key.recnum;
  return false;
}

bool ker_apl_id2name(cdp_t cdp, const uns8 * appl_id, char * name, trecnum * aplobj)
// Converts schema uuid to schema name and/or schema object number.
// Returns false iff schema with the uuid exists.
{ tobjnum objnum = NOOBJECT;  tobjname found_name;
 // search the cache first:
  { ProtectedByCriticalSection cs(&cs_short_term);
    for (int i=0;  i<schema_cache_dynar.count();  i++)
    { const t_schema_cache_item * sci = schema_cache_dynar.acc0(i);
      if (*sci->schema_name)  // item is valid
        if (!memcmp(appl_id, sci->schema_uuid, UUID_SIZE))  // found
        { strcpy(found_name, sci->schema_name);
          objnum = sci->objnum;
          break;
        }
    }    
  }
 // if the schema was found in the cache, verify it:
  if (objnum!=NOOBJECT)
    if (verify_values_from_cache(cdp, appl_id, found_name, objnum))
    { if (name) strcpy(name, found_name);
      if (aplobj) *aplobj=objnum;
      return false;
    }  
 // search the OBJTAB (slow):
  trecnum rec;  bool found=false;  
  { t_table_record_rdaccess table_record_rdaccess(cdp, objtab_descr);
    for (rec=0;  rec<objtab_descr->Recnum();  rec++)
    { const ttrec * dt = (const ttrec *)table_record_rdaccess.position_on_record(rec, 0);
      if (dt!=NULL)
        if (!dt->del_mark && dt->categ==CATEG_APPL && !memcmp(dt->apluuid, appl_id, UUID_SIZE))
        { memcpy(found_name, dt->name, OBJ_NAME_LEN);  found_name[OBJ_NAME_LEN]=0;
          found=true;
          break;
        }
    }
  }
 // put the search result (even negative) into the cache:
  put_values_to_cache(appl_id, found_name, found ? (tobjnum)rec : NOOBJECT);
  if (!found) return true;
  if (name) strcpy(name, found_name);
  if (aplobj) *aplobj=rec;
  return false;
}

void mark_schema_cache_dynar(void)
{ schema_cache_dynar.mark_core(); }

///////////////////////////////////////////// timer for periodical actions //////////////////////////////////////
/* Fields in the timer table:
ACTION_NAME   - name of the periodical action (used as the thread name when executing)
SQL_STATEMENT - statement executed in the periodical action
PRIORITY      - priority of the thread executing the action
DISABLED      - flag disabling the action (will not be started
LAST_RUN      - timestamp of the last run (date and time)
PLAN_CODE_1   - 0 if not used, -1 for interval planning, week day map otherwise
PLAN_TIME_1   - seconds between runs or day time of the run
...
_5
*/

ttablenum planner_tablenum = NOOBJECT;
#define PLANNER_ATR_LAST_RUN 7
#define PLANNED_SQL_STATEMENT_MAX 100
#define PLANNED_RESULT_MAX 100
#define ACTION_NAME_MAX_LEN (31+31)  // space for application name and object name

void create_timer_table(cdp_t cdp)
{ if (planner_tablenum == NOOBJECT)
    if (!name_find2_obj(cdp, "_TIMERTAB", "_SYSEXT", CATEG_TABLE, &planner_tablenum))
      return;  // table exists
  char stmt[400];
  sprintf(stmt, "CREATE TABLE _SYSEXT._TIMERTAB(ACTION_NAME CHAR(%u) PRIMARY KEY,SQL_STATEMENT CHAR(%u),RESULT CHAR(%u),PRIORITY INT,DISABLED BOOLEAN,TIME_WINDOW TIME,LAST_RUN TIMESTAMP,PLAN_CODE_1 INT,PLAN_TIME_1 TIME,PLAN_CODE_2 INT,PLAN_TIME_2 TIME,PLAN_CODE_3 INT,PLAN_TIME_3 TIME,PLAN_CODE_4 INT,PLAN_TIME_4 TIME,PLAN_CODE_5 INT,PLAN_TIME_5 TIME)", 
    ACTION_NAME_MAX_LEN, PLANNED_SQL_STATEMENT_MAX, PLANNED_RESULT_MAX);
  if (!sql_exec_direct(cdp, stmt))
  { if (!name_find2_obj(cdp, "_TIMERTAB", "_SYSEXT", CATEG_TABLE, &planner_tablenum))
    { table_descr_auto td(cdp, planner_tablenum);
      privils prvs(cdp);  t_privils_flex priv_val;  // will be initialised for every table
      priv_val.init(8+10*2);
     // read privils to the EVERYBODY_GROUP:
      prvs.set_user(EVERYBODY_GROUP, CATEG_GROUP, FALSE);
      priv_val.set_all_r();
      *priv_val.the_map() = RIGHT_READ;
      prvs.set_own_privils(td->me(), NORECNUM, priv_val);
     // all privils to the CONF_ADM_GROUP:
      prvs.set_user(CONF_ADM_GROUP, CATEG_GROUP, FALSE);
      priv_val.set_all_rw();
      *priv_val.the_map() = RIGHT_INSERT | RIGHT_DEL | RIGHT_GRANT | RIGHT_READ | RIGHT_WRITE;
      prvs.set_own_privils(td->me(), NORECNUM, priv_val);
    }
    commit(cdp);
  }  
}

#pragma pack(1)
struct timer_table_record
{ wbbool deleted;
  char action_name[ACTION_NAME_MAX_LEN];  // 1, primary key, name of the periodical action (used with @ as the thread name when executing)        
  char sql_statement[PLANNED_SQL_STATEMENT_MAX];  // 2, statement executed in the periodical action
  char result[PLANNED_RESULT_MAX]; // 3, execution result optionally defined by the execution
  int priority; // 4, priority of the thread executing the action (<0: below normal, >0: above normal)
  wbbool disabled; // 5, action disabled
  uns32 time_window;  // 6, as time
  uns32 last_run;     // 7, timestamp of the last run (date and time), PLANNER_ATR_LAST_RUN
  uns32 plan_code_1;  // 8, 0 or NULL if not used, -1 for interval planning, week day map otherwise
  uns32 plan_time_1;  // 9, time between runs or time when the action is run at
  uns32 plan_code_2;
  uns32 plan_time_2;
  uns32 plan_code_3;
  uns32 plan_time_3;
  uns32 plan_code_4;
  uns32 plan_time_4;
  uns32 plan_code_5;
  uns32 plan_time_5;
};  
#pragma pack()

static bool shall_run(uns32 dtm_now, uns32 time_window, uns32 last_run, uns32 plan_code, uns32 plan_time)
{ bool start=false;
  if (plan_code==NONEINTEGER || plan_time==NONEINTEGER)
    return false;
  if (plan_code==(uns32)-1)
    return last_run+plan_time/1000 <= dtm_now;  // comparing in seconds
  else // when converting 23.59 to the timestamp on 00:01, I must not use today's date but yesterday's!
  { if (time_window==NONEINTEGER) time_window=86400000;  // when window is not defined, the unlimited window
    else if (time_window<60000) time_window=60000;  // 1 minute is a minimum, otherwise some actions would have been skipped
   // testing if plan_time is between last_run and dtm_now and if it is in the time window:
    uns32 last_date=timestamp2date(last_run);
    uns32 last_time=timestamp2time(last_run);
    uns32 now_date =timestamp2date(dtm_now);
    uns32 now_time =timestamp2time(dtm_now);
    if (plan_time<=now_time)  // same date implied
    { if (plan_time+time_window>now_time)  // is in the time window
        if (last_time<plan_time || last_date<now_date)  // not started yet
          if (plan_code & (1<<Day_of_week(now_date)))
            return true;
    }
    else  // planned at a day earlier implied
    { if (plan_time+time_window>now_time+86400000)  // is in the time window
        if (last_date<now_date)  // not run today
          if (last_time<plan_time || last_date<dt_minus(now_date, 1))
          { int y_wd = Day_of_week(now_date)-1;
            if (y_wd<0) y_wd=6;
            if (plan_code & (1<<Day_of_week(y_wd)))
              return true;
          }
    }
  }  
  return false;
}

struct t_start_data
{ char action_name[1+ACTION_NAME_MAX_LEN+1];  // @ is prepended!
  char sql_statement[PLANNED_SQL_STATEMENT_MAX+1];
  int priority;
};

static THREAD_PROC(planned_proc_thread)
{
  t_start_data * start_data = (t_start_data *)data;
 // attach the thread to the server:
  cd_t cd(PT_WORKER);  cdp_t cdp = &cd;
  int code = interf_init(cdp);
  if (code == KSE_OK)
  { detachedThreadCounter++;
    strmaxcpy(cdp->thread_name, start_data->action_name, sizeof(cdp->thread_name));
#ifdef WINS
    if (start_data->priority && start_data->priority!=NONEINTEGER)
      SetThreadPriority(GetCurrentThread(), start_data->priority>0 ? THREAD_PRIORITY_ABOVE_NORMAL : THREAD_PRIORITY_BELOW_NORMAL);
#endif      
   // define user name 
    cdp->prvs.set_user(CONF_ADM_GROUP, CATEG_GROUP, TRUE); 
    cdp->random_login_key=GetTickCount();
    exec_direct(cdp, start_data->sql_statement);
    commit(cdp);
    kernel_cdp_free(cdp);
    cd_interf_close(cdp);
    detachedThreadCounter--;
  }
  else // slightly unsafe, because GetCurrTaskPtr() returns NULL
    client_start_error(cdp, detachedThreadStart, code);
  corefree(start_data);
  integrity_check_planning.thread_operation_stopped();
  THREAD_RETURN;
}

bool thread_is_running(const char * name)
{
  ProtectedByCriticalSection cs(&cs_client_list);
  for (int i=1;  i<=max_task_index;  i++)   /* system process 0 not counted */
    if (cds[i])
    { cdp_t acdp = cds[i];
      if (acdp->in_use==PT_WORKER)
        if (!sys_stricmp(acdp->thread_name, name))
          return true;
    }
  return false;        
}

void start_planned_actions(cdp_t cdp)
// Starts planned actions defined in the planner table. Updates the timestamps of the last run in the table.
// The number of the planner table is put into [planner_tablenum] in kernel_init() or when the table is created by create_timer_table().
{ bool commit_changes_in_planner_table = false;
  if (planner_tablenum!=NOOBJECT)
  { table_descr_auto td(cdp, planner_tablenum);
    t_table_record_rdaccess table_record_rdaccess(cdp, td->me());
    uns32 dtm_now = stamp_now();
    for (trecnum rec = 0;  td->me() && rec<td->Recnum();  rec++)
    { const timer_table_record * dt = (const timer_table_record*)table_record_rdaccess.position_on_record(rec, 0);
      if (dt && !dt->deleted)
        if (dt->disabled!=1) // may be NULL
          if (shall_run(dtm_now, dt->time_window, dt->last_run, dt->plan_code_1, dt->plan_time_1) ||
              shall_run(dtm_now, dt->time_window, dt->last_run, dt->plan_code_2, dt->plan_time_2) ||
              shall_run(dtm_now, dt->time_window, dt->last_run, dt->plan_code_3, dt->plan_time_3) ||
              shall_run(dtm_now, dt->time_window, dt->last_run, dt->plan_code_4, dt->plan_time_4) ||
              shall_run(dtm_now, dt->time_window, dt->last_run, dt->plan_code_5, dt->plan_time_5))
          { char thread_name[1+ACTION_NAME_MAX_LEN+1];
            *thread_name='@';  strmaxcpy(thread_name+1, dt->action_name, sizeof(thread_name)-1); // dt->action_name may not be zero-terminated
            if (!thread_is_running(thread_name))
            { t_start_data * start_data = (t_start_data*)corealloc(sizeof(t_start_data), 95);
              if (start_data)
              { start_data->priority=dt->priority;
                strmaxcpy(start_data->sql_statement, dt->sql_statement, sizeof(start_data->sql_statement));  // dt->sql_statement may not be zero-terminated
                strcpy(start_data->action_name, thread_name);
                integrity_check_planning.starting_client_request();  // registers the new running request, cannot return FALSE because the calling request is running
                if (!THREAD_START(planned_proc_thread, 0, start_data, NULL)) // thread handle closed inside
                { integrity_check_planning.thread_operation_stopped();
                  corefree(start_data);  
                }
                else
                { tb_write_atr(cdp, td->me(), rec, PLANNER_ATR_LAST_RUN, &dtm_now);
                  commit_changes_in_planner_table=true;
                }  
              }
            }
          }    
    }
  } // closing the access to the table
  if (commit_changes_in_planner_table) 
    commit(cdp);        
}

////////////////////////////////////////////////////////////////////////////////////
#include "csmsgs.cpp"
#include "wbmail.cpp"
