/****************************************************************************/
/* wprocs.c - standard procedures for internal language                     */
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
#ifdef WINS
#ifndef _MSC_VER
#include <windowsx.h>
#endif
#endif
#ifdef UNIX
#include <unistd.h>
#include <signal.h>
#include <sys/time.h>
#ifndef LINUX
#include <sys/types.h>
#endif
#endif
//#ifndef SRVR
//#include "prezrcdf.h"
//#include <windowsx.h>
//#ifdef __TURBOC__
//#include <dir.h>
//#else
//#include <direct.h>
//#endif
//#endif
#include <math.h>
#include <time.h>
#include <stdlib.h>
#undef putc
/******************** basic values & conversions ****************************/
tpiece NULLPIECE     = {0,0,0};

/***************************** database io **********************************/
CFNC BOOL WINAPI Waits_for_me(cdp_t cdp, val2uuid uuids)
{ return !memcmp(uuids.id_user, cdp->prvs.user_uuid, UUID_SIZE); }

static void get_date_time(cdp_t cdp)
{ 
  cdp->curr_date=Today();
#ifdef WINS
  SYSTEMTIME systime;
  GetLocalTime(&systime);
  cdp->curr_time=Make_time(systime.wHour, systime.wMinute, systime.wSecond, systime.wMilliseconds);
#else
  struct timeval TP;  struct tm time;
  if (0==gettimeofday(&TP, NULL) && localtime_r(&TP.tv_sec, &time))
    cdp->curr_time=Make_time(time.tm_hour, time.tm_min, time.tm_sec,TP.tv_usec/1000);
  else cdp->curr_time=Now();
#endif
  cdp->date_time_statement_cnt_val = cdp->statement_counter;
}

uns32 WINAPI Current_date(cdp_t cdp)
{ 
  if (cdp->statement_counter > cdp->date_time_statement_cnt_val) 
    get_date_time(cdp);
  return cdp->curr_date;
}

uns32 WINAPI Current_time(cdp_t cdp)
{ 
  if (cdp->statement_counter > cdp->date_time_statement_cnt_val) 
    get_date_time(cdp);
  return cdp->curr_time;
}

uns32 WINAPI Current_timestamp(cdp_t cdp)
{ 
  if (cdp->statement_counter > cdp->date_time_statement_cnt_val) 
    get_date_time(cdp);
  return datetime2timestamp(cdp->curr_date, cdp->curr_time);
}

double WINAPI sp_sin (double arg)  { return arg==NULLREAL ? NULLREAL : sin (arg); }
double WINAPI sp_cos (double arg)  { return arg==NULLREAL ? NULLREAL : cos (arg); }
double WINAPI sp_atan(double arg)  { return arg==NULLREAL ? NULLREAL : atan(arg); }
double WINAPI sp_exp (double arg)  { return arg==NULLREAL ? NULLREAL : exp (arg); }
double WINAPI sp_log (double arg)  { return arg==NULLREAL ? NULLREAL : log (arg); }
double WINAPI sp_sqrt(double arg)  { return arg==NULLREAL ? NULLREAL : sqrt(arg); }

/////////////////////////////// server support //////////////////////////////
//////////////////////////// strings /////////////////////////////////////
BOOL is_null(const char * val, int type, t_specif specif)
{ return IS_NULL(val, type, specif); }

void add_val(tptr sum, const char * val, uns8 type)
{ switch (type)
  { case ATT_CHAR:  *(sig32*)sum += *val;                    break;
    case ATT_INT8:  *(sig32*)sum += *(const sig8*)val;             break;
    case ATT_INT16: *(sig32*)sum += *(const sig16*)val;            break;
    case ATT_MONEY: moneyadd((monstr *)sum, (monstr *)val);  break;
    case ATT_INT64: *(sig64*)sum += *(const sig64*)val;            break;
    case ATT_DATE:  case ATT_TIME:
    case ATT_INT32: *(sig32*)sum += *(const sig32*)val;            break;
    case ATT_FLOAT: *(double*)sum += *(const double*)val;          break;
  }
}

#ifdef WINS
#include <rpcdce.h>
CFNC void WINAPI create_uuid(WBUUID appl_id)
{ UUID Uuid;
  if (UuidCreate(&Uuid) == RPC_S_OK)
    memcpy(appl_id, Uuid.Data4+2, 6);
  uns32 dt=Today();
  int year=Year(dt), month=Month(dt), day=Day(dt);
  dt=Now()/1000 + 86400L*((day-1)+31L*((month-1)+12*(year-1996)));
  *(uns32*)(appl_id+6)=dt;
  SYSTEMTIME st;
  GetSystemTime(&st);
  *(uns16*)(appl_id+10)=st.wMilliseconds;
}
#endif

static uns16 last_fast_time_counter_value = 0;
   
void WINAPI safe_create_uuid(WBUUID uuid)
// Creates an UUID different from the previous one
{ uns16 fast_time_counter;
  if (init_counter >= SERVER_INIT_OBJECTS)
    EnterCriticalSection(&cs_client_list);  
  do
  { create_uuid(uuid);
    fast_time_counter = *(uns16*)(uuid+10);
    if (last_fast_time_counter_value!=fast_time_counter) break;
    Sleep(0);
  } while (TRUE);
  if (init_counter >= SERVER_INIT_OBJECTS)
    LeaveCriticalSection(&cs_client_list);  
  last_fast_time_counter_value=fast_time_counter;
}

CFNC sig32 WINAPI WinBase602_version(void)
{ return WB_VERSION_MINOR+(WB_VERSION_MAJOR<<16); }
CFNC sig32 WINAPI cd_WinBase602_version(cdp_t cdp)
{ return WB_VERSION_MINOR+(WB_VERSION_MAJOR<<16); }

BOOL WINAPI exec_process(cdp_t cdp, char * module, char * params, BOOL wait)
{ 
#ifdef WINS
  tptr command = (tptr)corealloc(strlen(module)+1+strlen(params)+1, 48);
  if (!command) { request_error(cdp, OUT_OF_KERNEL_MEMORY);  return FALSE; }
  strcpy(command, module);  strcat(command, " ");  strcat(command, params);
  STARTUPINFO si;  PROCESS_INFORMATION pi;
  ZeroMemory(&si, sizeof(si));
  si.cb = sizeof(si);
  if (!CreateProcess(NULL, // No module name (use command line) -- necessary for 16bit processes
        command,          // Command line. 
        NULL,             // Process handle not inheritable. 
        NULL,             // Thread handle not inheritable. 
        FALSE,            // Set handle inheritance to FALSE. 
        0,                // No creation flags. 
        NULL,             // Use parent's environment block. 
        NULL,             // Use parent's starting directory. 
        &si,              // Pointer to STARTUPINFO structure.
        &pi))             // Pointer to PROCESS_INFORMATION structure.
     { corefree(command);  return FALSE; }// failed
   corefree(command);
 // Wait until child process exits.
   if (wait)
     WaitForSingleObject(pi.hProcess, INFINITE);
 // Close process and thread handles. 
  CloseHandle(pi.hProcess);
  CloseHandle(pi.hThread);
  return TRUE;
#endif
#ifdef UNIX
  tptr * argv, pars;  int argc;  BOOL result;
 // make a local copy of parameters:
  pars=(tptr)corealloc(strlen(params)+1, 48);
  if (!pars) { request_error(cdp, OUT_OF_KERNEL_MEMORY);  return FALSE; }
  strcpy(pars, params);
 // 1st pass: count params, 2nd pass: separate params and store pointers to them
  for (int pass=0;  pass<2;  pass++)
  { tptr p=pars;  argc=1;
    do
    { while (*p && (*p==' ' || *p=='\t' || *p=='\n')) p++;
      if (!*p) break;
      tptr start=p, stop;
      if (*p=='\"')  // delimited parameter
      { while (*p && *p!='\"') p++;
        stop=p;  if (*p) p++;
      }
      else
      { while (*p && *p!=' ' && *p!='\t' && *p!='\n') p++;
        stop=p;
      }
      if (!pass) argc++;
      else 
        { argv[argc++]=start;  *stop=0; }
    } while (TRUE);
    if (!pass)
    { argv=(tptr*)corealloc(sizeof(tptr)*(argc+1), 48);
      if (!argv) { corefree(pars);  request_error(cdp, OUT_OF_KERNEL_MEMORY);  return FALSE; }
      argv[0]=module;
    }
  }
  argv[argc]=NULL; // parameter terminator
  pid_t pid=vfork();
  if (pid==-1) result=FALSE;
  else if (!pid) // the child
  { if (execv(module, argv)==-1)
      kill(9,getpid());
  }
 // parent:
  corefree(argv);  corefree(pars);
  if (result && wait)  // wait for the child process
  {
  }
  return result;
#endif
#ifdef NETWARE
  return FALSE;  // fails always, not implemented
#endif
}

#ifdef STOP
uns32 Make_timestamp(int day, int month, int year, int hour, int minute, int second)
{ if (day > 31) return (uns32)NONEINTEGER;
  year = (year < 1900) ? 0 : year - 1900;
  return second+60L*(minute+60*hour)+
         86400L*((day-1)+31L*((month-1)+12*year));
}

BOOL convert_value(tptr source, int stype, tptr dest, int dtype)
{ if (!stype) stype=dtype;
  else if (!dtype) dtype=stype;
  if (IS_STRING(stype) || stype==ATT_TEXT)
  { if (IS_STRING(dtype) || dtype==ATT_TEXT)
      strcpy(dest, source);
    else
      switch (dtype)
      { case ATT_DATE:
          str2date(source, (uns32*)dest);
          break;
        case ATT_TIME:
          str2time(source, (uns32*)dest);
          break;
        case ATT_TIMESTAMP:
        { tptr p;  uns32 dt, tm;
          p=source;  while (*p && *p!=' ') p++;
          char c=*p;  *p=0;
          str2date(source, &dt);
          *p=c; while (*p==' ') p++;
          str2time(p, &tm);
          TIMESTAMP_STRUCT ts;
          if (dt==NONEDATE) ts.day=32;
          else
          { ts.day=Day(dt);  ts.month=Month(dt);  ts.year=Year(dt);
            if (tm==NONETIME) tm=0;
            ts.hour=Hours(tm);  ts.minute=Minutes(tm);
            ts.second=Seconds(tm);  ts.fraction=Sec1000(tm);
          }
          TIMESTAMP2datim(&ts, (uns32*)dest);
          break;
        }
        default:
          if (!strconv(dtype, source, (basicval0*)dest)) return FALSE;
          break;
      }  // switch
  }
  else  // not from string
  { if (stype==dtype)
      memcpy(dest, source, stype==ATT_BINARY ? 256 : 8);
    else switch (dtype)
    { case ATT_STRING:   // to string
      case ATT_TEXT:
      { int prez=5;
        basicval0 bval, * theval = (basicval0*)source;
        switch (stype)
        { case ATT_ODBC_DATE:  case ATT_ODBC_TIME:  case ATT_ODBC_TIMESTAMP:
          case ATT_AUTOR:
            theval=&bval;  bval.ptr=source;  break;
          case ATT_BINARY:
            theval=&bval;  bval.ptr=source;  prez=UUID_SIZE;  break;
          case ATT_FLOAT:
            prez=15;  break;
          case ATT_DATE:
            prez=1;  break;
          case ATT_TIME:
            prez=2;  break;
          case ATT_TIMESTAMP:  case ATT_DATIM:
          { TIMESTAMP_STRUCT ts;
            datim2TIMESTAMP(*(uns32*)source, &ts);
            if (ts.day==32) *dest=0;
            else
            { date2str(Make_date(ts.day, ts.month, ts.year), dest, 1);
              strcat(dest, "  ");
              time2str(Make_time(ts.hour, ts.minute, ts.second, ts.fraction), dest+strlen(dest), 1);
            }
            return TRUE;
          }
          case ATT_MONEY:
            prez=1;  break;
        }
        conv2str(theval, stype, dest, (sig8)prez);
        break;
      }

      case ATT_INT16:
        switch (stype)
        { case ATT_INT32:
            *(sig16*)dest = *(sig32*)source>0x7fff || *(sig32*)source<-0x7fff ? NONESHORT : *(sig16*)source;
            break;
          case ATT_MONEY:
          { sig32 i;
            money2int((monstr*)source, &i);
            *(sig16*)dest = i>0x7fff || i<-0x7fff ? NONESHORT : (sig16)i;
            break;
          }
          case ATT_FLOAT:
          { double rval=*(double*)source;
            if (rval>0) rval+=0.5;  else rval-=0.5;
            *(sig16*)dest = rval==NONEREAL || rval>0x7fff || rval<-0x7fff ? NONESHORT : (sig16)rval;
            break;
          }
          default: return FALSE;
        }
        break;

      case ATT_INT32:
        switch (stype)
        { case ATT_INT16:
            *(sig32*)dest = *(sig16*)source==NONESHORT ? NONEINTEGER : (sig32)*(sig16*)source;
            break;
          case ATT_MONEY:
            money2int((monstr*)source, (sig32*)dest);
            break;
          case ATT_FLOAT:
          { double rval=*(double*)source;
            if (rval>0) rval+=0.5;  else rval-=0.5;
            *(sig32*)dest = rval==NONEREAL || rval>0x7fffffff || rval<-0x7fffffff ? NONEINTEGER : (sig32)rval;
            break;
          }
          default: return FALSE;
        }
        break;

      case ATT_MONEY:
        switch (stype)
        { case ATT_INT16:
            int2money(*(sig16*)source==NONESHORT ? NONEINTEGER : (sig32)*(sig16*)source, (monstr*)dest);
            break;
          case ATT_INT32:
            int2money(*(sig32*)source, (monstr*)dest);
            break;
          case ATT_FLOAT:
            real2money(*(double*)source, (monstr*)dest);
            break;
          default: return FALSE;
        }
        break;

      case ATT_FLOAT:
        switch (stype)
        { case ATT_INT16:
            *(double*)dest=*(sig16*)source==NONESHORT ? NONEREAL : (double)*(sig16*)source;
            break;
          case ATT_INT32:
            *(double*)dest=*(sig32*)source==NONEINTEGER ? NONEREAL : (double)*(sig32*)source;
            break;
          case ATT_MONEY:
            *(double*)dest=money2real((monstr*)source);
            break;
          default: return FALSE;
        }
        break;

      case ATT_DATE:
      case ATT_TIME:
      case ATT_TIMESTAMP:
      case ATT_DATIM:
      case ATT_ODBC_DATE:
      case ATT_ODBC_TIME:
      case ATT_ODBC_TIMESTAMP:
      { int day, month, year;  int hour, minute, second, fraction;
        BOOL is_null=FALSE;
        day=month=year=hour=minute=second=fraction=0;
       // analysis:
        switch (stype)
        { case ATT_DATIM:  case ATT_TIMESTAMP:
          { uns32 dtm = *(uns32*)source;
            if (dtm==(uns32)NONEINTEGER) is_null=TRUE;
            else
            { dtm /= 86400L;
              day   =(int)(dtm % 31 + 1);  dtm /= 31;
              month =(int)(dtm % 12 + 1);
              year  =(int)(dtm/12 + 1900);
              dtm = *(uns32*)source % 86400L;
              second=(int)(dtm % 60);  dtm /= 60;
              minute=(int)(dtm % 60);  dtm /= 60;
              hour  =(int)dtm;
            }
            break;
          }
          case ATT_DATE:
          { uns32 dt=*(uns32*)source;
            if (dt==NONEDATE) is_null=TRUE;
            else
              { day=Day(dt);  month=Month(dt);  year=Year(dt); }
            break;
          }
          case ATT_TIME:
          { uns32 tm=*(uns32*)source;
            if (tm==NONETIME) is_null=TRUE;
            else
            { hour=Hours(tm);  minute=Minutes(tm);
              second=Seconds(tm);  fraction=Sec1000(tm);
            }
            break;
          }
          case ATT_ODBC_DATE:
          { DATE_STRUCT * dtst = (DATE_STRUCT *)source;
            if (dtst->day==32) is_null=TRUE;
            else
              { day=dtst->day;  month=dtst->month;  year=dtst->year; }
            break;
          }
          case ATT_ODBC_TIME:
          { TIME_STRUCT * tmst = (TIME_STRUCT *)source;
            if (tmst->hour>=24) is_null=TRUE;
            else
            { hour=tmst->hour;  minute=tmst->minute;
              second=tmst->second;
            }
            break;
          }
          case ATT_ODBC_TIMESTAMP:
          { TIMESTAMP_STRUCT * dtmst = (TIMESTAMP_STRUCT *)source;
            if (dtmst->day>=32) is_null=TRUE;
            else
            { day=dtmst->day;  month=dtmst->month;  year=dtmst->year;
              hour=dtmst->hour;  minute=dtmst->minute;
              second=dtmst->second;  fraction=dtmst->fraction;
            }
            break;
          }
          default: return FALSE;
        } // analysis switch
       // synthesis:
        switch (dtype)
        { case ATT_DATE:
            *(uns32*)dest=is_null ? NONEDATE : Make_date(day, month, year);  break;
          case ATT_TIME:
            *(uns32*)dest=is_null ? NONETIME : Make_time(hour, minute, second, fraction);  break;
          case ATT_TIMESTAMP:
          case ATT_DATIM:
            *(uns32*)dest=is_null ? NONEINTEGER :
              Make_timestamp(day, month, year, hour, minute, second);
            break;
          case ATT_ODBC_DATE:
          { DATE_STRUCT * dtst = (DATE_STRUCT *)dest;
            dtst->day=is_null ? 32 : day;
            dtst->month=month;
            dtst->year=year;
            break;
          }
          case ATT_ODBC_TIME:
          { TIME_STRUCT * tmst = (TIME_STRUCT *)dest;
            tmst->hour=is_null ? 25 : hour;
            tmst->minute=minute;
            tmst->second=second;
            break;
          }
          case ATT_ODBC_TIMESTAMP:
          { TIMESTAMP_STRUCT * dtmst = (TIMESTAMP_STRUCT *)dest;
            dtmst->day=is_null ? 32 : day;
            dtmst->month=month;
            dtmst->year=year;
            dtmst->hour=hour;
            dtmst->minute=minute;
            dtmst->second=second;
            dtmst->fraction=fraction;
            break;
          }
        } // synthesis switch
        break;
      } // date and time comversions
      default:
        return FALSE;
    } // switch dtype
  }
  return TRUE;
}
#endif

void get_dp_value(cdp_t cdp, int dpnum, t_value * res, int desttype, t_specif destspecif)
{ if (cdp->sel_stmt==NULL || dpnum >= cdp->sel_stmt->dparams.count()) 
    res->set_null();
  else
  { WBPARAM * wbpar = cdp->sel_stmt->dparams.acc0(dpnum);
    if (!wbpar->val) res->set_null();
    else
    { int size = wbpar->len;  
      if (res->allocate(size)) 
        { request_error(cdp, OUT_OF_KERNEL_MEMORY);  return; }
      char * destptr = res->valptr();
      memcpy(destptr, wbpar->val, size);  
      if (IS_HEAP_TYPE(desttype) || IS_EXTLOB_TYPE(desttype)) 
      { res->length=size;
        if (desttype==ATT_TEXT || desttype==ATT_EXTCLOB) destptr[size]=destptr[size+1]=0; // add unicode terminator
      }
      else 
      { if (is_string(desttype))
          destptr[size]=destptr[size+1]=0;  // add unicode terminator
        Fil2Mem_convert(res, destptr, desttype, destspecif);
      }
    }
  }
}

void set_dp_value(cdp_t cdp, int dpnum, t_value * val, int srctype)
// Assigns the value in val to the DP with the dpnum number.
{ if (dpnum >= cdp->sel_stmt->markers.count()) return; // fuse only, should not be possible
  MARKER * marker = cdp->sel_stmt->markers.acc0(dpnum);
  WBPARAM * wbpar = cdp->sel_stmt->dparams.acc(dpnum);
  if (wbpar==NULL) { request_error(cdp, OUT_OF_KERNEL_MEMORY);  return; }
  if (val->is_null()) wbpar->free();
  else
  { unsigned len = is_string(marker->type) || marker->type==ATT_BINARY || IS_HEAP_TYPE(marker->type)
                   ? val->length : tpsize[marker->type];
    if (wbpar->len < len)
    { wbpar->free();
      wbpar->realloc(len);
      if (wbpar->val==NULL) { request_error(cdp, OUT_OF_KERNEL_MEMORY);  return; }
    }
    memcpy(wbpar->val, val->valptr(), len);
    wbpar->len=len;
  }
}

BOOL Repl_appl_shared(cdp_t cdp, const char *Appl)
{
    BOOL Err = TRUE;
    tcursnum Curs = NOCURSOR;
    cur_descr *cd;
    t_replapl replapl;
    replapl.state = REPLAPL_NOTHING;
    table_descr *srvtab_descr = install_table(cdp, SRV_TABLENUM);
    if (!srvtab_descr)
        return(NONEBOOLEAN);
    table_descr *rpltab_descr = install_table(cdp, REPL_TABLENUM);
    if (!rpltab_descr)
        goto exit;
    WBUUID ApplUUID;
    if (!Appl || !*Appl)
    {
        Appl = cdp->sel_appl_name;
        memcpy(ApplUUID, cdp->sel_appl_uuid, UUID_SIZE);
    }
    else
    {
        tobjnum ApplObj = find_object(cdp, CATEG_APPL, Appl);
        if (ApplObj == NOOBJECT)
            goto exit;
        if (tb_read_atr(cdp, objtab_descr, ApplObj, APPL_ID_ATR, (tptr)ApplUUID))
            goto exit;
    }
    WBUUID my_server_id; 
    if (tb_read_atr(cdp, srvtab_descr, 0, SRV_ATR_UUID, (tptr)my_server_id))
        goto exit;
    char Query[128];
    lstrcpy(Query, "SELECT * FROM REPLTAB WHERE SOURCE_UUID=X\'");
    bin2hex(Query + 42, my_server_id, UUID_SIZE);
    lstrcpy(Query + 42 + 2 * UUID_SIZE, "\' AND APPL_UUID=X\'");
    bin2hex(Query + 42 + 2 * UUID_SIZE + 18, ApplUUID, UUID_SIZE);
    lstrcpy(Query + 42 + 2 * UUID_SIZE + 18 + 2 * UUID_SIZE, "\' AND TABNAME=\'\'");
    Curs = open_working_cursor(cdp, Query, &cd);
    if (Curs == NOCURSOR)
        goto exit;
    trecnum r;
    for (r = 0; r < cd->recnum; r++)
    {
        trecnum tr;
        cd->curs_seek(r, &tr);
        if (tb_read_var(cdp, rpltab_descr, tr, REPL_ATR_REPLCOND, 0, sizeof(replapl), (tptr)&replapl))
            goto exit;
        if (replapl.state == REPLAPL_IS)
            break;
    }
    Err = FALSE;

exit:

    if (Curs != NOCURSOR)
        free_cursor(cdp, Curs);
    if (rpltab_descr)
        unlock_tabdescr(rpltab_descr);
    if (srvtab_descr)
        unlock_tabdescr(srvtab_descr);
    if (Err)
        return(NONEBOOLEAN);
    return(replapl.state == REPLAPL_IS);
}

/////////////////////////// flexible string /////////////////////////////////
// Accumulated error if cannot allocate enough memory
// Contains space for terminating null, added on "get"
#include "flstr.h"
#include "flstr.cpp"

