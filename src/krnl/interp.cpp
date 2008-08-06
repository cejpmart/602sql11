/****************************************************************************/
/* interp.cpp - The S-Pascal interpreter                                    */
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
#include "wprez.h"  // load cache adr etc.
#else
#include "wprez9.h"  // load cache adr etc.
#endif
#ifdef WINS
#include <windowsx.h>  // I_EVENTRET
static void aggr_get(cdp_t cdp, uns16 index, uns8 ag_type, basicval * retval); /* forw. */
#else
#include <ctype.h>
CFNC DllKernel void WINAPI inval_table_d(cdp_t cdp, tobjnum tb, tcateg cat);
#ifndef UNIX
#include <process.h>
#include <advanced.h>
#endif
#endif

static void unt_destruct(cdp_t cdp, void * start, unsigned size);
static void realize_bps(cdp_t cdp);
static void unrealize_bps(cdp_t cdp);
/****************************************************************************/
#define TOS1  (  cdp->RV.stt   ->int8)
#define TOS2  (  cdp->RV.stt   ->int16)
#define TOS4  (  cdp->RV.stt   ->int32)
#define TOS8  (  cdp->RV.stt   ->real)
#define TOS8i (  cdp->RV.stt   ->int64)
#define TOSP  (  cdp->RV.stt   ->ptr)
#define TOSWB (  cdp->RV.stt   ->wbacc)
#define TOSS  (  cdp->RV.stt   ->subr)
#define TOSM  (& cdp->RV.stt  ->moneys)
#define NOS1  ( (cdp->RV.stt-1)->int8)
#define NOS2  ( (cdp->RV.stt-1)->int16)
#define NOS4  ( (cdp->RV.stt-1)->int32)
#define NOS8  ( (cdp->RV.stt-1)->real)
#define NOS8i ( (cdp->RV.stt-1)->int64)
#define NOSP  ( (cdp->RV.stt-1)->ptr)
#define NOSWB ( (cdp->RV.stt-1)->wbacc)
#define NOSM  (&(cdp->RV.stt-1)->moneys)
#define DT1  (*(uns8*)  (cdp->RV.runned_code+cdp->RV.pc))
#define DT2  (*(uns16*) (cdp->RV.runned_code+cdp->RV.pc))
#define DT4  (*(uns32*) (cdp->RV.runned_code+cdp->RV.pc))
#define DT8  (*(double*)(cdp->RV.runned_code+cdp->RV.pc))
#define DT8i (*(sig64 *)(cdp->RV.runned_code+cdp->RV.pc))

CFNC DllKernel BOOL WINAPI is_null(tptr val, uns8 type)
{ switch (type)
  { case ATT_CHAR:    return *val==0x0;  // change from version 4!
    case ATT_BOOLEAN: return *(uns8*)val==0x80;
    case ATT_INT8:    return *(uns8*)val==0x80;
    case ATT_INT16:   return *(uns16*)val==0x8000;
    case ATT_MONEY:   return !*(uns16*)val && (*(sig32*)(val+2)==NONEINTEGER);
    case ATT_INT32: case ATT_DATE: case ATT_TIME:  case ATT_TIMESTAMP:
                      return *(sig32*)val==NONEINTEGER;
    case ATT_INT64:   return *(sig64*)val==NONEBIGINT;
    case ATT_PTR:   case ATT_BIPTR:
                      return *(trecnum*)val==NORECNUM;
    case ATT_FLOAT:   return *(double*)val==NULLREAL;
    case ATT_STRING:  return !*val;
    case ATT_BINARY:  return FALSE;
  }
  return FALSE;
}

CFNC DllKernel int WINAPI close_run(cdp_t cdp)
{ int clres = 0;
  while (cdp->RV.key_registry)
  { key_trap * kt = cdp->RV.key_registry;
    cdp->RV.key_registry=kt->next;
    corefree(kt);
  }
  unt_destruct(cdp, NULL, 0);
  if (close_all_files(cdp)) clres|=1;
//  if (close_cursors()) clres|=2;
  Unlock_user(FALSE);       /* removes all locks except for objects */
#ifdef WINS
#if WBVERS<900
  remove_temp_conns(cdp);   /* removes connection crated by the program */
#endif
#endif
  return clres;
}

CFNC DllKernel void WINAPI run_prep(cdp_t cdp, tptr code_buffer, uns32 start_pc, uns8 wexception)
{ cdp->RV.stt        =cdp->RV.st-1;
  cdp->RV.appended   =cdp->RV.mask=0;
  cdp->RV.auto_vars  =cdp->RV.glob_vars;
  cdp->RV.prep_vars  =NULL;
  cdp->RV.runned_code=(uns8*)code_buffer;  /* base for view definitions */
  cdp->RV.pc         =start_pc;
  cdp->RV.wexception =wexception;
}

static BOOL is_main_level_code(cdp_t cdp)
/* Returns TRUE if executing project-level code not called from other code */
{ scopevars * sc;
  if (!cdp->RV.bpi) return FALSE;
  if (cdp->RV.runned_code!=cdp->RV.bpi->code_addr) return FALSE;
  if (cdp->RV.auto_vars)
  { sc=cdp->RV.auto_vars;
    while ((sc->next_scope!=cdp->RV.glob_vars) && sc->next_scope)
      sc=sc->next_scope;
    if (sc->retcode!=cdp->RV.bpi->code_addr) return FALSE;
  }
  return TRUE;
}

CFNC DllKernel void WINAPI free_run_vars(cdp_t cdp)
{ scopevars * sc, * saved_vars;
 /* unrealize breakpoints unless executing non-main code or a subr. called
    from non-main code. */
  if (cdp->RV.bpi && cdp->RV.bpi->realized && cdp->RV.run_project.common_proj_code)
    if (is_main_level_code(cdp)) unrealize_bps(cdp);
  while (cdp->RV.auto_vars && (cdp->RV.auto_vars!=cdp->RV.glob_vars))
  { if (cdp->RV.prev_run_state)  /* check if not releasing any of saved frames */
    { saved_vars=cdp->RV.prev_run_state->auto_vars;
      while (saved_vars)
      { if (cdp->RV.auto_vars==saved_vars) goto free_exit;
        saved_vars=saved_vars->next_scope;
      }
    }
    sc=cdp->RV.auto_vars->next_scope;
    corefree(cdp->RV.auto_vars);
    cdp->RV.auto_vars=sc;
  }
 free_exit:
  if (cdp->RV.prev_run_state) saved_vars=cdp->RV.prev_run_state->prep_vars;
  else saved_vars=NULL;
  while ((cdp->RV.prep_vars) && (cdp->RV.prep_vars!=saved_vars))
  { sc=cdp->RV.prep_vars->next_scope;
    corefree(cdp->RV.prep_vars);
    cdp->RV.prep_vars=sc;
  }
}

/********************************** dates ***********************************/
sig32 dt2days(sig32 dt)
{ sig16 year; uns16 month, m;
  year=(sig16)(dt / (31*12));
  month=(uns16)(dt / 31 % 12);   m=1;
  dt %= 31;
  if (year>=3000) return 1116000L;
  if (month<=12) /* fuse */ while (m<=month) dt+=c_month(m++,year);
  while (--year>=0) dt+=c_year(year);
  return dt;
}

sig32 days2dt(sig32 days)
{ uns16 year, month;
  if (days>1116000L) days=1116000L;
  year=0; month=1;
  while (days >= (sig32)c_year (year))       days-=c_year (year++);
  while (days >= (sig32)c_month(month,year)) days-=c_month(month++,year);
  return days+31*(uns32)(month-1+12*year);
}

sig32 _ts_minus(uns32 ts1, uns32 ts2)
{ if (ts1==NONEDATE || ts2==NONEDATE) return NONEINTEGER;
  uns32 dt1 = timestamp2date(ts1),  dt2 = timestamp2date(ts2),
        tm1 = timestamp2time(ts1),  tm2 = timestamp2time(ts2);
  int tmdiff, daysdiff = dt2days(dt1) - dt2days(dt2);
  tmdiff=tm1/1000-tm2/1000;
  if (tmdiff < 0) { tmdiff+=86400;  daysdiff--; }
  if (daysdiff < 0) { tmdiff-=86400;  daysdiff++; }
  return 86400*daysdiff + tmdiff;
}

uns32 _ts_plus(uns32 ts, sig32 diff)
{ if (ts==NONEDATE || diff==NONEINTEGER) return ts;
  uns32 dt = timestamp2date(ts),  tm = timestamp2time(ts)/1000;
  if (diff>=0)
  { tm+=diff % 86400;  
    if (tm >= 86400) { tm-=86400;  diff+=86400; }
    dt=days2dt(dt2days(dt) + diff / 86400);
  }
  else
  { diff=-diff;
    unsigned tmdiff = diff % 86400;
    if (tm >= tmdiff) tm-=tmdiff;
    else { tm = tm+86400-tmdiff;  diff+=86400; }
    diff /= 86400;
    int days = dt2days(dt);
    if (diff > days) { days=0;  tm=0; } else days-=diff;
    dt=days2dt(days);
  }
  return datetime2timestamp(dt, 1000*tm);
}
/******************************* database I/O *******************************/
static BOOL db_read(cdp_t cdp, curpos * wbacc)
/* reads database data to TOS */
{ BOOL res;  basicval bval;
  if (wbacc->position==NORECNUM) r_error(NIL_POINTER);
  bval.int32=0;  /* auto-conversion */
  res=cd_Read_ind(cdp, wbacc->cursnum, wbacc->position, wbacc->attr, wbacc->index, &bval);
  *cdp->RV.stt=bval; /* data address replaces kontext on TOS */
  return res;
}

static BOOL db_reads(cdp_t cdp, curpos * wbacc)
/* reads database data somewhere and writes the pointer to them to TOSP (used for strings and binary) */
{ BOOL res;
  if (wbacc->position==NORECNUM) r_error(NIL_POINTER);
  cdp->RV.hyperbuf[0]=0;  // I want to have a defined value on error (error may be masked)
  res=cd_Read_ind(cdp, wbacc->cursnum, wbacc->position, wbacc->attr, wbacc->index, cdp->RV.hyperbuf);
  TOSP=cdp->RV.hyperbuf; /* data address replaces kontext on TOS */
  return res;
}

static BOOL db_read_num(cdp_t cdp, curpos * wbacc) /* reads number of multiattribute items to TOS */
/* on TOS, wbacc will be replaced by num */
{ BOOL res;  
  if (wbacc->position==NORECNUM) r_error(NIL_POINTER);
  int val = 0;  // I want to have a defined value on error (error may be masked)
  res=cd_Read_ind_cnt(cdp, wbacc->cursnum, wbacc->position, wbacc->attr, (uns16*)&val);
  TOS4 = val & 0xffff;
  return res;
}

static BOOL db_read_len(cdp_t cdp, curpos * wbacc) /* reads var-atr length to TOS */
{ /* on TOS, wbacc will be replaced by length */
  if (wbacc->position==NORECNUM) r_error(NIL_POINTER);
  uns32 val = 0;   // I want to have a defined value on error (error may be masked)
  BOOL res = cd_Read_len(cdp, wbacc->cursnum, wbacc->position, wbacc->attr, wbacc->index, &val);
  TOS4 = val;
  return res;
}

static BOOL db_read_int(cdp_t cdp, uns32 start, uns16 size, BOOL strng)
/* Reads interval & stores it to NOSP (0-terminated string or structure with a 16-bit counter (not changed to 32-bits!)) */
/* Stack contents: destination address, wbacc (remove all). */
{ BOOL res;  uns32 rdsize;
  res=cd_Read_var(cdp, TOSWB.cursnum, TOSWB.position, TOSWB.attr, TOSWB.index,
               start, size, strng ? NOSP : NOSP+sizeof(uns16), &rdsize);
  if (!res)
    if (strng) NOSP[(unsigned)rdsize]=0;
    else *(uns16*)NOSP=(uns16)rdsize;
  cdp->RV.stt-=2;    /* the database access & destination address removed */
  return res;
}

static BOOL db_write(cdp_t cdp, curpos * wbacc, uns16 size, tptr dataptr)
/* Writes data (dataptr, size) to database (wbacc). */
{ if (!wbacc->cursnum) return TRUE;
  return cd_Write_ind(cdp, wbacc->cursnum, wbacc->position, wbacc->attr, wbacc->index, dataptr, size);
}

static BOOL db_write_num(cdp_t cdp) /* writes number of multiattribute items TOS2 to NOSWB */
{ BOOL res;  uns16 num = TOS2;
  cdp->RV.stt--;
  if (!TOSWB.cursnum) return TRUE;
  res=cd_Write_ind_cnt(cdp, TOSWB.cursnum, TOSWB.position, TOSWB.attr, num);
  cdp->RV.stt--;
  return res;
}

static BOOL db_write_len(cdp_t cdp) /* writes var-atr length TOS4 to NOSWB */
{ BOOL res;  uns32 len = TOS4;
  cdp->RV.stt--;
  if (!TOSWB.cursnum) return TRUE;
  res=cd_Write_len(cdp, TOSWB.cursnum, TOSWB.position, TOSWB.attr, TOSWB.index, len);
  cdp->RV.stt--;
  return res;
}

static BOOL db_write_int(cdp_t cdp, tptr dataptr, uns32 start, uns16 size)
/* writes interval from TOSP */
/* Stack contents: structure address. */
{ BOOL res;
  if (!TOSWB.cursnum)  /* disabled unless creating link table */
    if ((start!=0) || (size!=20) || (TOSWB.attr!=OBJ_DEF_ATR)) return TRUE;
  res=cd_Write_var(cdp, TOSWB.cursnum, TOSWB.position, TOSWB.attr, TOSWB.index, start, size, dataptr);
  if (TOSWB.cursnum==OBJ_TABLENUM) inval_table_d(cdp, TOSWB.position, CATEG_CURSOR);
  cdp->RV.stt--;   /* the database access removed */
  return res;
}

void unt_register(cdp_t cdp, untstr * uval)
{ int i;  untstr ** pptr = NULL;
  for (i=0;  i<cdp->RV.registry.count();  i++)
  { if (*cdp->RV.registry.acc(i)==uval) return;
    if (*cdp->RV.registry.acc(i)==NULL) pptr=cdp->RV.registry.acc(i);
  }
  if (pptr==NULL)
  { pptr=cdp->RV.registry.next();
    if (pptr==NULL) return;
  }
  *pptr=uval;
}

void unt_unregister(cdp_t cdp, untstr * uval)
{ for (int i=0;  i<cdp->RV.registry.count();  i++)
    if (*cdp->RV.registry.acc(i)==uval) *cdp->RV.registry.acc(i)=NULL;
}

static void unt_destruct(cdp_t cdp, void * start, unsigned size)
/* destruct all iff start==NULL */
{ untstr * ptr;  BOOL rel;
  for (int i=0;  i<cdp->RV.registry.count();  i++)
  { ptr=*cdp->RV.registry.acc(i);
    if (ptr!=NULL)
    { if (!start) rel=TRUE;
      else rel=((tptr)ptr >= (tptr)start) && ((tptr)ptr < (tptr)start+size);
      if (rel)
      { if (is_string(ptr->uany.type))   /* this should be always satisfied */
          { corefree(ptr->uptr.val);  ptr->uptr.val=NULL; }
        *cdp->RV.registry.acc(i)=NULL;
      }
    }
  }
  if (start==NULL) cdp->RV.registry.usptr_dynar::~usptr_dynar();
}

CFNC DllKernel int WINAPI untyped_assignment(cdp_t cdp, basicval * dest, basicval * src, int tp1, int tp2, int flags, int len)
{ const d_table * td;  BOOL is_multi;  BOOL res;  int vallen = 12;  t_specif specif1, specif2;  // default specif is NULL
 /* determining the types from cursor descrs.: */
  if ((tp1==ATT_UNTYPED) && (flags & UNT_DESTDB))
  { td=cd_get_table_d(cdp, dest->wbacc.cursnum, CATEG_TABLE);
    if (!td) return OUT_OF_R_MEMORY;
    tp1     =td->attribs[dest->wbacc.attr].type;
    specif1 =td->attribs[dest->wbacc.attr].specif;
    is_multi=td->attribs[dest->wbacc.attr].multi!=1;
    if (SPECIFLEN(tp1)) len=td->attribs[dest->wbacc.attr].specif.length;
    release_table_d(td);
    if (is_multi != (dest->wbacc.index!=NOINDEX))
      return UNCONVERTIBLE_UNTYPED;   /* multi conflict */
    if (tp1==ATT_ODBC_DATE)      tp1=ATT_DATE;  else
    if (tp1==ATT_ODBC_TIME)      tp1=ATT_TIME;  else
    if (tp1==ATT_ODBC_TIMESTAMP) tp1=ATT_DATIM; else
    if (tp1==ATT_ODBC_DECIMAL || tp1==ATT_ODBC_NUMERIC) tp1=ATT_MONEY;
  }
  if ((tp2==ATT_UNTYPED) && (flags & UNT_SRCDB))
  { td=cd_get_table_d(cdp, src->wbacc.cursnum, CATEG_TABLE);
    if (!td) return OUT_OF_R_MEMORY;
    tp2     =td->attribs[src->wbacc.attr].type;
    specif2 =td->attribs[src->wbacc.attr].specif;
    vallen  =td->attribs[src->wbacc.attr].specif.length;
    is_multi=td->attribs[src->wbacc.attr].multi!=1;
    release_table_d(td);
    if (is_multi != (src->wbacc.index!=NOINDEX))
      return UNCONVERTIBLE_UNTYPED;   /* multi conflict */
    if (tp2==ATT_ODBC_DATE)      tp2=ATT_DATE;  else
    if (tp2==ATT_ODBC_TIME)      tp2=ATT_TIME;  else
    if (tp2==ATT_ODBC_TIMESTAMP) tp2=ATT_DATIM; else
    if (tp2==ATT_ODBC_DECIMAL || tp2==ATT_ODBC_NUMERIC)  tp2=ATT_MONEY;
  }
 /* Now, database objects are typed. */
 /* reading from the database: */
  basicval0 bval;  tptr space;
  BOOL strrepr2 = is_string(tp2);
  if (flags & UNT_SRCDB)
  { if (IS_HEAP_TYPE(tp2))
    { uns32 len32;
      space = cdp->RV.hyperbuf;
      res=cd_Read_var(cdp, src->wbacc.cursnum, src->wbacc.position, src->wbacc.attr, src->wbacc.index, 0, 256, space, &len32);
      if (!res) space[(unsigned)len32]=0;
      tp2=ATT_STRING;  strrepr2 = TRUE;
    }
    else
    { space = strrepr2 || tp2==ATT_BINARY ? cdp->RV.hyperbuf : (tptr)&bval;
      res=cd_Read_ind(cdp, src->wbacc.cursnum, src->wbacc.position, src->wbacc.attr, src->wbacc.index, space);
    }
    if (res) return cd_Sz_error(cdp);
  }
  else if (tp2==ATT_UNTYPED)   /* assigning from an untyped variable */
  { tp2=src->unt->uany.type;
    specif2=src->unt->uany.specif;
    if (!tp2) //return UNCONVERTIBLE_UNTYPED; /* uninitialized */
    { space=NULLSTRING;  tp2=ATT_STRING; /* NULL value */
      strrepr2 = TRUE;
    }
    else
    { strrepr2 = is_string(tp2);
      space=strrepr2 ? src->unt->uptr.val : (tptr)src->unt->uany.val;
    }
  }
  else space=strrepr2 ? src->ptr : (tptr)src;
 /* Now, source value is in space, its type is in tp2, not untyped. */
  if (tp1==ATT_UNTYPED) /* writing into an untyped variable, type will not be changed: */
  { BOOL was_string=is_string(dest->unt->uany.type);
    if (was_string) safefree((void**)&dest->unt->uptr.val);
    if (strrepr2)
    { tptr val = (tptr)corealloc(strlen(space)+1, 88);
      if (val==NULL) return OUT_OF_R_MEMORY;   /* out of memory */
      strcpy(val, space);
      if (!was_string) unt_register(cdp, dest->unt);
      dest->unt->uptr.val=val;
    }
    else
    { if (was_string) unt_unregister(cdp, dest->unt);
      memcpy(dest->unt->uany.val, space, tpsize[tp2]);
    }
    dest->unt->uany.type=tp2;  dest->unt->uany.specif=specif2.opqval;
  }
  else  /* writing into an typed object */
  {/* converting types */
    basicval0 * pval= (basicval0*)space;
    if ((tp2==ATT_PTR) || (tp2==ATT_BIPTR)) tp2=ATT_INT32;
    if (strrepr2)  /* converting from a string */
    { if (!is_string(tp1) && !IS_HEAP_TYPE(tp1))
      { if (!convert_from_string(tp1, space, &bval, specif1)) //return UNCONVERTIBLE_UNTYPED;
          convert_from_string(tp1, NULLSTRING, &bval, specif1);
        space=(tptr)&bval;
      }
    }
    else switch (tp1)  /* destination type */
    { case ATT_BOOLEAN:
      case ATT_CHAR:
      case ATT_INT16:  case ATT_INT8: case ATT_INT64:
      case ATT_INT32:  case ATT_PTR:  case ATT_BIPTR:
      { BOOL is_null;
        switch (tp2)  /* source type */
        { case ATT_BOOLEAN:
            is_null=(sig8)pval->int8==NONETINY;
            bval.int64 = pval->int8 ? 1 : 0;  break;
          case ATT_INT8:
            is_null=(sig8)pval->int8==NONETINY;
            bval.int64 = pval->int8;  break;
          case ATT_CHAR:
            is_null=!pval->uint8;
            bval.int64 = pval->uint8;  break;
          case ATT_INT16:
            is_null=pval->int16==NONESHORT;
            bval.int64 = (sig64)pval->int16;  break;
          case ATT_INT32:  case ATT_PTR:  case ATT_BIPTR:
            is_null=pval->int32==NONEINTEGER;
            bval.int64 = pval->int32;  break;
          case ATT_INT64:
            is_null=pval->int64==NONEBIGINT;
            bval.int64 = pval->int64;  break;
          case ATT_MONEY:
            is_null=IS_NULLMONEY(&pval->moneys);
            bval.int64 = pval->int64;  
            if (pval->moneys.money_hi4 < 0) ((uns16*)&bval.int64)[3]=0xffff;
            else                            ((uns16*)&bval.int64)[3]=0;
            break;
          case ATT_FLOAT:
            is_null=pval->real==NONEREAL;
            bval.int64 = (sig64)pval->real;  break;
          default: return UNCONVERTIBLE_UNTYPED;
        }
        if (tp1==ATT_INT8)
          { if (is_null) bval.int8=NONETINY; }
        else if (tp1==ATT_INT16)
          { if (is_null) bval.int16=NONESHORT; }
        else if (tp1==ATT_INT32)
          { if (is_null) bval.int32=NONEINTEGER; }
        else if (tp1==ATT_INT64)
          { if (is_null) bval.int64=NONEBIGINT; }
        else if (tp1==ATT_CHAR)
          { if (is_null) bval.int32=0; }
        else if (tp1==ATT_BOOLEAN)
          { if (is_null) bval.int8=NONETINY;  else if (bval.int8) bval.int8=1; }
        space=(tptr)&bval;  break;
      }
      case ATT_FLOAT:
      case ATT_MONEY:
        switch (tp2)  /* source type */
        { case ATT_BOOLEAN:  
            bval.real=pval->uint8==NONEBOOLEAN   ? NONEREAL : pval->int8 ? 1.0 : 0.0;  break;
          case ATT_INT8:
            bval.real=pval->int8==NONETINY       ? NONEREAL : (double)pval->int8;  break;
          case ATT_CHAR:
            bval.real=(double)pval->int8;  break;
          case ATT_INT16:
            bval.real=pval->int16==(sig16)0x8000 ? NONEREAL : (double)pval->int16;  break;
          case ATT_INT32:  case ATT_PTR:  case ATT_BIPTR:
            bval.real=pval->int32==NONEINTEGER   ? NONEREAL : (double)pval->int32;  break;
          case ATT_INT64:
            bval.real=pval->int64==NONEBIGINT    ? NONEREAL : (double)pval->int64;  break;
          case ATT_MONEY:
            bval.real=money2real(&pval->moneys);  break;
          case ATT_FLOAT:
            bval.real=pval->real;  break;
          default: return UNCONVERTIBLE_UNTYPED;
        }
        if (tp1==ATT_MONEY) real2money(bval.real, &bval.moneys);
        space=(tptr)&bval;  break;
      case ATT_DATIM: /* dest. type */
      case ATT_DATE:  /* dest. type */
      case ATT_TIME:
      case ATT_TIMESTAMP:
        if (tp1==tp2)
          { bval.int32=pval->int32;  space=(tptr)&bval;  break; }
        /* cont. */
      default:
        if (!IS_HEAP_TYPE(tp1)) return UNCONVERTIBLE_UNTYPED;
        /* cont. */
      case ATT_STRING:  
        if (tp2==ATT_BINARY)
        { basicval0 abval;  abval.ptr=space;
          conv2str(&abval, tp2, cdp->RV.hyperbuf, 0, specif2);
        }
        else
          conv2str(pval, tp2, cdp->RV.hyperbuf, SQL_LITERAL_PREZ, specif2);
        space=cdp->RV.hyperbuf;
        break;
      case ATT_BINARY:
        if (tp2!=ATT_BINARY) return UNCONVERTIBLE_UNTYPED;
        break;
    }

   /* writing the value: */
    if (flags & UNT_DESTDB)  /* write into the database */
    { if (!SPECIFLEN(tp1)) len=tpsize[tp1];
      if (IS_HEAP_TYPE(tp1))
        res=cd_Write_var(cdp, dest->wbacc.cursnum, dest->wbacc.position, dest->wbacc.attr, dest->wbacc.index, 0, (int)strlen(space)+1, space);
      else
        res=cd_Write_ind(cdp, dest->wbacc.cursnum, dest->wbacc.position, dest->wbacc.attr, dest->wbacc.index, space, len);
      if (res) return cd_Sz_error(cdp);
    }
    else /* normal assignment into a type variable, value was converted */
    { if (is_string(tp1))   /* writing into a string variable */
        strmaxcpy(dest->ptr, space, len);
      else if (tp1==ATT_BINARY)
        memcpy(dest->ptr, space, len);
      else memcpy(dest->ptr, space, tpsize[tp1]); /* non-string IPL variable */
    }
  }
  return 0;
}

static void run_db_error(cdp_t cdp, int err)
{ if (!cdp->RV.mask) r_error(err);  /* not masked */ }

static sig32 conv16to32(sig16 val)
{ if (val==(sig16)0x8000) return NONEINTEGER;
  return (sig32)val;
}

CFNC DllKernel BOOL WINAPI wsave_run(cdp_t cdp)
{ saved_run_state * pRV;
  pRV=(saved_run_state *)corealloc(sizeof(saved_run_state), 99);
  if (!pRV) return TRUE;
  memcpy(pRV, &cdp->RV, sizeof(saved_run_state));
  cdp->RV.prev_run_state=pRV;
  cdp->RV.auto_vars=cdp->RV.glob_vars; /* I_LOADADR in the inner run needs this */
  return FALSE;
}

CFNC DllKernel void WINAPI wrestore_run(cdp_t cdp)
{ saved_run_state * pRV = cdp->RV.prev_run_state;
  if (!pRV) return;
  if (cdp->RV.wexception==BREAK_EXCEPTION) pRV->wexception=BREAK_EXCEPTION;
  memcpy(&cdp->RV, pRV, sizeof(saved_run_state));
  corefree(pRV);
}

#ifdef WINS
#if WBVERS<900
void construct_object(cdp_t cdp, tptr addr, tptr objname)
{ char vw[1*sizeof(tobjname)];  *vw='*';  strcpy(vw+1, objname);
  view_dyn * inst = new(addr) basic_vd(cdp);
  inst->destroy_on_close=FALSE;
  if (*objname) inst->construct(vw, NOOBJECT, 0, 0);
}

void destruct_object(cdp_t cdp, tptr addr)
{ view_dyn * inst = (view_dyn*)addr;
  if (!wsave_run(cdp))
  { inst->destruct();  // closes the window which may activate other windows and run a program
    wrestore_run(cdp);
  }
}
#endif
#endif


BOOL testuj = FALSE;

CFNC DllKernel int WINAPI run_pgm(cdp_t cdp)
/* All jumps are relative to the 1st byte of the dest. address */
/* When called from kernel, table_descrs of all tables in kont_c are supposed to be locked. */
{ int run_err;  unsigned len;  tptr ptr;  uns8 tp;
  BOOL bb;                 /* auxiliary variable - result of relations */
 // find the current line & file in order to be able to determine the step to another line
  if (cdp->RV.wexception==TRACE_AND_BP || cdp->RV.wexception==STEP_AND_BP)
    line_from_offset(cdp, cdp->RV.pc, cdp->RV.runned_code, &cdp->RV.bpi->current_line, &cdp->RV.bpi->current_file);
  cdp->RV.break_request=wbfalse;
#ifdef WINS
  __try
#else
  run_err=Catch(cdp->RV.run_jmp);
  if (!run_err)
#endif
  /* else: normal execution */
  {do /* the main instruction cycle */
   {
    if (testuj) heap_test();
    switch (cdp->RV.runned_code[cdp->RV.pc++])
    {
case I_FALSEJUMP4:if ((cdp->RV.stt--)->int16) { cdp->RV.pc += sizeof(sig32);  break; } /* cont. */
case I_JUMP4:     cdp->RV.pc+=(sig32)DT4;  goto jumping;
case I_FALSEJUMP: if ((cdp->RV.stt--)->int16) { cdp->RV.pc += sizeof(sig16);  break; } /* cont. */
case I_JUMP:      cdp->RV.pc+=(sig16)DT2;
 jumping:
#ifdef WINS /////////////////////// WINS only ///////////////////////////////////////////
#if WBVERS<900
  // Must not be called during redrawing the form, click into caption is replaced by double click then!
  if (!cdp->RV.interrupt_disabled)
    if (!cdp->RV.prev_run_state && task_switch_enabled>0)
    { MSG msg;
      wsave_run(cdp);
      while (PeekMessage(&msg, 0, 0, WM_PAINT-1, PM_REMOVE) ||
             PeekMessage(&msg, 0, WM_PAINT+1, 0xffff, PM_REMOVE))
        if (TranslateRegisteredKeys(&cdp->RV, &msg))
        { if (msg.message==WM_QUIT) { PostQuitMessage(0);  break; } // must preserve the message
          else
          { TranslateMessage(&msg);
            DispatchMessage(&msg);
          }
        }
      wrestore_run(cdp);
  /* WM_KEYDOWN removed from the main cycle because of loosing keyboard
     messages in views open from program (Peek_message in a cycle.
     save-restore_run must be called if function called on JUMP from program,
     otherwise the program may be damaged by incomming messages (KALKUL).
  */
    }
    else // task switch disabled
    { MSG msg;
      while (PeekMessage(&msg, 0, WM_KEYDOWN, WM_KEYDOWN, PM_NOREMOVE))
      { if (msg.wParam==VK_CONTROL)
        { TranslateMessage(&msg);
          DispatchMessage(&msg);
          PeekMessage(&msg,0,WM_KEYDOWN, WM_KEYDOWN, PM_REMOVE|PM_NOYIELD);
        }
        else if (msg.wParam==VK_CANCEL)
        { if ((cdp->RV.global_state==PROG_RUNNING) && cdp->RV.bpi && (cdp->RV.wexception!=BREAK_EXCEPTION) && !cdp->RV.prev_run_state)
            cdp->RV.wexception=BREAK_EXCEPTION;
          else
            { cdp->RV.break_request=wbtrue;  cd_Break(cdp); }
          PeekMessage(&msg,0,WM_KEYDOWN, WM_KEYDOWN, PM_REMOVE|PM_NOYIELD);
        }
        break;
      }
    }
#endif
#endif //////////////////////////// WINS only ///////////////////////////////////////////
  break;
case I_INT8CONST: cdp->RV.stt++; TOS4=(sig32)DT1; cdp->RV.pc += 1; break;
case I_INT16CONST:cdp->RV.stt++; TOS4=(sig32)(sig16)DT2; cdp->RV.pc += 2; break;
case I_INT32CONST:cdp->RV.stt++; TOS4=DT4;        cdp->RV.pc += 4; break; /* for ptr as well */
case I_INT64CONST:cdp->RV.stt++; TOS8i=DT8i;      cdp->RV.pc += 8; break;
case I_REALCONST: cdp->RV.stt++; TOS8=DT8;        cdp->RV.pc += 8; break;
case I_MONEYCONST:cdp->RV.stt++;  memcpy(TOSM, cdp->RV.runned_code+cdp->RV.pc, MONEY_SIZE);
                                                cdp->RV.pc += MONEY_SIZE;  break;
case I_STRINGCONST: cdp->RV.stt++;
#ifdef NEVER  // result lost patched in run_code
                  len=strlen(cdp->RV.runned_code+cdp->RV.pc+2);
                  if ((len < STR_BUF_SIZE) /*&& cdp->RV.prev_run_state*/)
                  /* cdp->RV.prev_run is NULL in main program, in which the strings
                     should not be copied, otherwise open_sql_parts does not
                     work. */
                  /* It is necessary to copy the string if it is the result of
                    the program run and must be preserved after the program
                    code is destroyed (e.g. view item def. code copy). */
                  { strcpy(cdp->RV.hyperbuf, cdp->RV.runned_code+cdp->RV.pc+2);
                    TOSP=cdp->RV.hyperbuf;
                  }
                  else
#endif
                  TOSP=(char*)cdp->RV.runned_code+cdp->RV.pc+2;
                  cdp->RV.pc+=DT2+3;  break; /* TOS=addr. of string */
case I_BINCONST:  cdp->RV.stt++;
                  TOSP=(char*)cdp->RV.runned_code+cdp->RV.pc+1;
                  cdp->RV.pc+=DT1+1;  break; /* TOS=addr. of string */
case I_LOADADR:   
{ cdp->RV.stt++;  uns8 level=DT1;  cdp->RV.pc++;
  ptr=level ? cdp->RV.glob_vars->vars : cdp->RV.auto_vars->vars;
  if (!ptr) r_error(EXECUTED_OK);
  TOSP=ptr + DT2;  cdp->RV.pc += 2;
  break;
}
case I_LOADADR4:  
{ cdp->RV.stt++;  uns8 level=DT1;  cdp->RV.pc++;
  ptr=level ? cdp->RV.glob_vars->vars : cdp->RV.auto_vars->vars;
  if (!ptr) r_error(EXECUTED_OK);
  TOSP=ptr + DT4;  cdp->RV.pc += 4;
  break;
}
case I_SUBRADR:   cdp->RV.stt++;  TOSS.offset=DT4;  cdp->RV.pc += 4;
                  cdp->RV.pc++;  /* skips level */ break;
case I_DUP:       //cdp->RV.stt++; cdp->RV.stt[0]=cdp->RV.stt[-1]; break;
                  cdp->RV.stt++;
                  memcpy(cdp->RV.stt, cdp->RV.stt-1, sizeof(basicval)); break;
case I_DROP:      cdp->RV.stt--; break;
case I_BREAK:     cdp->RV.pc--;  cdp->RV.wexception=NO_EXCEPTION;
                  if (cdp->RV.bpi && (cdp->RV.runned_code==cdp->RV.bpi->code_addr))
                  { unrealize_bps(cdp);
                    line_from_offset(cdp, cdp->RV.pc, cdp->RV.runned_code, &cdp->RV.bpi->current_line, &cdp->RV.bpi->current_file);
                  }
                  return PGM_BREAKED;
case I_STOP:      cdp->RV.pc--;  free_run_vars(cdp);  return EXECUTED_OK;
case I_PREPCALL:  
{ cdp->RV.stt++;
  len=DT4;  cdp->RV.pc += 4;
  scopevars * sv=(scopevars*)corealloc(sizeof(scopevars)+len, OW_PGRUN);
  if (!sv) r_error(OUT_OF_R_MEMORY);
  memset(sv->vars, 0, len);  /* inits. auto vars by 0, better for debugger */
  /* 0-init necesary for variable cursors! */
  sv->next_scope=cdp->RV.prep_vars;
  sv->localsize=len;
  cdp->RV.prep_vars=sv;
  TOSP=sv->vars;     /* actual parameters pointer */
  break;
}
case I_REALLOC_FRAME:
{ len=DT4;  cdp->RV.pc += 4;
  scopevars * sv=(scopevars*)corerealloc(cdp->RV.auto_vars, 2*len+sizeof(scopevars));   /* *2 - temp. for 32 bit */
  if (sv) cdp->RV.auto_vars=sv;
  break;
}
#ifdef __ia64__
case I_REAL_PAR_OFFS:
{ uns8  parnum = DT1;  cdp->RV.pc+=1; // real paramater number
  uns32 offset = DT4;  cdp->RV.pc+=4; // parameter value offset in the stack frame
  scopevars * sv=cdp->RV.prep_vars;
  register double parval __asm__("f8");
  parval= *(double*)(sv->vars+offset);
  break;
}
#endif
case I_CALL:
{ cdp->RV.stt--;   /* parameters pointer removed */
  scopevars * sv=cdp->RV.prep_vars; cdp->RV.prep_vars=sv->next_scope;
  sv->next_scope=cdp->RV.auto_vars; cdp->RV.auto_vars=sv;
  sv->retadr=cdp->RV.pc;
  sv->retcode=cdp->RV.runned_code;
  switch (cdp->RV.wexception)
  { case IN_STEP:
           cdp->RV.wexception=NO_EXCEPTION;  sv->wexception=IN_STEP;
           break;
    case STEP_AND_BP:
           cdp->RV.wexception=SET_BP;        sv->wexception=IN_STEP;
           break;
    case IN_TRACE:
    case TRACE_AND_BP:
           sv->wexception=IN_STEP;
           break;
    default:  /* SET_BP or NO_EXCEPTION */
           sv->wexception=NO_EXCEPTION;
           break;
  }
  if (cdp->RV.run_project.common_proj_code==NULL) // e.g. project changed when form editor is opened 
    r_error(PROJECT_CHANGED);
  cdp->RV.runned_code=(uns8*)cdp->RV.run_project.common_proj_code;
  cdp->RV.pc         =TOSS.offset;
  cdp->RV.stt--;
  break;
}
case I_SCALL:
{ unsigned procnum;  scopevars * parptr;
  cdp->RV.stt--;  procnum=TOS2;  cdp->RV.stt--;
  parptr=cdp->RV.prep_vars;  cdp->RV.prep_vars=cdp->RV.prep_vars->next_scope;
  do_call(cdp, procnum, parptr); /* corefree(parptr); inside! */
  break;
}

case I_DLLCALL:
{ if (cdp->RV.wexception && (cdp->RV.runned_code==cdp->RV.bpi->code_addr))
  { switch (cdp->RV.wexception)
    { case STEP_AND_BP:
        realize_bps(cdp);  cdp->RV.wexception=IN_STEP;
        break;
      case TRACE_AND_BP:
        realize_bps(cdp);  cdp->RV.wexception=IN_TRACE;
        break;
      case SET_BP:
        realize_bps(cdp);  cdp->RV.wexception=NO_EXCEPTION;
        break;
    }
  }
  scopevars * sv=cdp->RV.auto_vars; cdp->RV.auto_vars=cdp->RV.auto_vars->next_scope;
  ptr=(char*)cdp->RV.runned_code+cdp->RV.pc;  /* ptr to address & names */
  cdp->RV.pc=sv->retadr;  cdp->RV.runned_code=sv->retcode;
 /* I_CALL actions undone */
  call_dll(cdp, (dllprocdef *)ptr, sv);
  if ((sv->wexception==IN_STEP) &&
      ((cdp->RV.wexception==NO_EXCEPTION) || (cdp->RV.wexception==IN_TRACE)))
    cdp->RV.wexception=sv->wexception;
  corefree(sv);
  break;
}

#ifdef WINS /////////////////////// WINS only ///////////////////////////////////////////
case I_EVENTRET:  wsave_run(cdp);   /* necessary e.g. for WM_TILE */
                  SendMessage(GetParent(cdp->RV.hClient), WM_COMMAND,
                              GET_WM_COMMAND_MPS(cdp->RV.stt->int32, 0, 0));
/* PostMessage sometimes does not work (when clicking owner-draw buttons by ALT-&letter) */
                  wrestore_run(cdp);
                  cdp->RV.stt--; break;
#endif //////////////////////////// WINS only ///////////////////////////////////////////

case I_FUNCTRET:  ptr=cdp->RV.auto_vars->vars + DT2;  cdp->RV.pc+=2;  tp=DT1;  cdp->RV.pc++;
                  cdp->RV.stt++;
                  if (!tp)  /* string returned */
                  { len=(int)strlen(ptr)+1;  if (len>STR_BUF_SIZE) len=STR_BUF_SIZE;
                    strmaxcpy(cdp->RV.hyperbuf,ptr,len);
                    TOSP=cdp->RV.hyperbuf;
                  }
                  else switch (tp)  /* simple value returned */
                  { case 1: TOS4=(sig32)*ptr;               break;
                    case 2: TOS4=conv16to32(*(sig16*)ptr);  break;
                    case 4: TOS4=*(sig32*)ptr;              break;
                    case 6: memcpy(TOSM, ptr, MONEY_SIZE);  break;
                    case 8: TOS8i=*(sig64*)ptr;             break;
                  }
                  /* no break ! */
case I_PROCRET:  /* BP must be set now, return may perform code change */
{/* necessary for RUN on RET of procedure called from menu */
  if (cdp->RV.wexception && (cdp->RV.runned_code==cdp->RV.bpi->code_addr))
  { switch (cdp->RV.wexception)
    { case STEP_AND_BP:
        realize_bps(cdp);  cdp->RV.wexception=IN_STEP;
        break;
      case TRACE_AND_BP:
        realize_bps(cdp);  cdp->RV.wexception=IN_TRACE;
                                                break;
      case SET_BP:
        realize_bps(cdp);  cdp->RV.wexception=NO_EXCEPTION;
                                                break;
    }
  }
  scopevars * sv=cdp->RV.auto_vars; cdp->RV.auto_vars=cdp->RV.auto_vars->next_scope;
  cdp->RV.pc=sv->retadr;
  cdp->RV.runned_code=sv->retcode;
  if ((sv->wexception==IN_STEP) && (cdp->RV.wexception==NO_EXCEPTION))
                                  cdp->RV.wexception=sv->wexception;
  unt_destruct(cdp, sv->vars, sv->localsize);
  corefree(sv);  break;
}
case I_STORE1:               *NOSP=(uns8)TOS2;      cdp->RV.stt-=2;  break;
case I_STORE2:               *(sig16*)NOSP=TOS2;    cdp->RV.stt-=2;  break;
case I_STORE4:               *(sig32*)NOSP=TOS4;    cdp->RV.stt-=2;  break;
case I_STORE6:                      memcpy(NOSP, TOSM, MONEY_SIZE);  cdp->RV.stt-=2;  break;
case I_STORE8:               memcpy(NOSP,&TOS8,8);  cdp->RV.stt-=2;  break;
case I_STORE:                memcpy(NOSP,TOSP,DT4); cdp->RV.stt-=2;  cdp->RV.pc += 4; break;
case I_STORE_STR: 
{ uns16 transsize=DT2;  cdp->RV.pc+=2;
  strmaxcpy(NOSP, TOSP, transsize);
  cdp->RV.stt-=2;  break;
}
#if WBVERS<900
case I_STORE_DB_REF:
                ptr=NOSP;
                *(HWND    *)ptr=cdp->RV.inst ? cdp->RV.inst->hWnd : 0;    ptr+=4;//sizeof(HWND);
                *(tcursnum*)ptr=TOSWB.cursnum;   ptr+=4/*sizeof(tcursnum)*/;
                *(trecnum *)ptr=TOSWB.position;  ptr+=sizeof(trecnum);
                *(uns32   *)ptr=TOSWB.attr;      ptr+=4;
                *(uns32   *)ptr=TOSWB.index;     ptr+=4;
                cdp->RV.stt--;  TOSP=ptr;  break;
#endif
//case I_P_STORE1:*NOSP=(uns8)TOS2;                  NOSP+=1;  cdp->RV.stt--;  break;
case I_P_STORE2:  *(sig32*)NOSP=(uns32)(uns16)TOS2;  NOSP+=sizeof(void*);  cdp->RV.stt--;  break;
case I_P_STORE4:  *(sig32*)NOSP=TOS4;                NOSP+=sizeof(void*);  cdp->RV.stt--;  break;
case I_P_STORE6:  memcpy(NOSP, TOSM, MONEY_SIZE);    NOSP+=8;  cdp->RV.stt--;  break;
case I_P_STORE8:  memcpy(NOSP,&TOS8,8);              NOSP+=8;  cdp->RV.stt--;  break;
case I_P_STORE:   memcpy(NOSP, TOSP, DT4);
                  NOSP+=((DT4-1)/sizeof(void*)+1)*sizeof(void*);  cdp->RV.pc += 4;  cdp->RV.stt--;  break;
case I_P_STORE_STR:
{ uns16 transsize=DT2;  cdp->RV.pc+=2;
  strmaxcpy(NOSP, TOSP, transsize);  NOSP+=((transsize-1)/sizeof(void*)+1)*sizeof(void*);
  cdp->RV.stt-=1;  break;
}
#ifdef WINS
#if WBVERS<900
case I_CURRVIEW:  cdp->RV.stt++;  TOS4=(uns32)(cdp->RV.inst ? cdp->RV.inst->hWnd : (HWND)0);  break;
#endif
#endif
case I_LOAD1:     TOS4=(sig32)*TOSP;              break;
case I_LOAD2:     TOS4=(sig32)*(uns16*)TOSP;      break;  /* no sign extens. */
case I_LOAD4:     TOS4=*(sig32*)TOSP;             break;
case I_LOADOBJNUM:TOS4=*(sig32*)TOSP;             break;
case I_LOAD6:     memcpy(TOSM, TOSP, MONEY_SIZE); break;
case I_LOAD8:     TOS8i=*(sig64*)TOSP;            break;
case I_DEREF:     TOSP=*(tptr*)TOSP;
                  if (TOSP==NULL) r_error(NIL_POINTER);
                  break;

case I_TOSI2F:    TOS8=(TOS4==NONEINTEGER) ? NULLREAL : (double)TOS4;  break;
case I_NOSI2F:    NOS8=(NOS4==NONEINTEGER) ? NULLREAL : (double)NOS4;  break;
case I_TOSM2F:    TOS8=money2real(TOSM);      break;
case I_NOSM2F:    NOS8=money2real(NOSM);      break;
case I_TOSI2M:    int2money(TOS4, TOSM);      break;
case I_NOSI2M:    int2money(NOS4, NOSM);      break;
case I_TOSM2I:    money2int(TOSM, &TOS4);     break;
case I_NOSM2I:    money2int(NOSM, &NOS4);     break;
case I_TOSF2M:    real2money(TOS8, TOSM);     break;
case I_INTNEG:    if (TOS4!=NONEINTEGER)    TOS4=-TOS4; break;
case I_REALNEG:   if (TOS8!=NULLREAL) TOS8=-TOS8; break;
case I_BOOLNEG:   if (TOS2) TOS4=0; else TOS4=1;  break;
case I_MONEYNEG:  money_neg(TOSM);  break;
case I_I64NEG:    if (TOS8i!=NONEBIGINT) TOS8i=-TOS8i; break;
case I_OR:         NOS2=(uns16)(TOS2 || NOS2); cdp->RV.stt--;  break;
case I_AND:        NOS2=(uns16)(TOS2 && NOS2); cdp->RV.stt--;  break;
case I_OR_BITWISE: NOS4=TOS4 |  NOS4; cdp->RV.stt--;  break;
case I_AND_BITWISE:NOS4=TOS4 &  NOS4; cdp->RV.stt--;  break;
case I_NOT_BITWISE:TOS4=~TOS4;                   break;
case I_INTMOD:    if (TOS4) NOS4=NOS4 % TOS4; else
                    cdp->RV.wexception=DIV0_ERROR;
                  cdp->RV.stt--; break;
case I_I64MOD:    if (TOS8i) NOS8i=NOS8i % TOS8i; else
                    cdp->RV.wexception=DIV0_ERROR;
                  cdp->RV.stt--; break;
case I_INTOPER+0: bb=(BOOL)(NOS4<TOS4);
                          bol: cdp->RV.stt--;  TOS4=(bb) ? 1 : 0;  break;
case I_INTOPER+1: bb=(BOOL)(NOS4==TOS4); goto bol;
case I_INTOPER+2: bb=(BOOL)(NOS4> TOS4); goto bol;
case I_INTOPER+3: bb=(BOOL)(NOS4<=TOS4); goto bol;
case I_INTOPER+4: bb=(BOOL)(NOS4!=TOS4); goto bol;
case I_INTOPER+5: bb=(BOOL)(NOS4>=TOS4); goto bol;
case I_INTOPER+6: if (NOS4==NONEINTEGER) NOS4=TOS4;  else if (TOS4!=NONEINTEGER) NOS4+=TOS4;
                  cdp->RV.stt--;  break;
case I_INTOPER+7: if (NOS4==NONEINTEGER) NOS4=-TOS4; else if (TOS4!=NONEINTEGER) NOS4-=TOS4;
                  cdp->RV.stt--;  break;
case I_INTOPER+8: if (TOS4==NONEINTEGER) NOS4=NONEINTEGER; else if (NOS4!=NONEINTEGER) NOS4*=TOS4;
                  cdp->RV.stt--;  break;
case I_INTOPER+9: if (TOS4==NONEINTEGER) NOS4=NONEINTEGER; else if (NOS4!=NONEINTEGER)
                                         if (TOS4) NOS4/=TOS4; else
                    cdp->RV.wexception=DIV0_ERROR;
                  cdp->RV.stt--;  break;
case I_I64OPER+0: bb=(BOOL)(NOS8i< TOS8i); goto bol;
case I_I64OPER+1: bb=(BOOL)(NOS8i==TOS8i); goto bol;
case I_I64OPER+2: bb=(BOOL)(NOS8i> TOS8i); goto bol;
case I_I64OPER+3: bb=(BOOL)(NOS8i<=TOS8i); goto bol;
case I_I64OPER+4: bb=(BOOL)(NOS8i!=TOS8i); goto bol;
case I_I64OPER+5: bb=(BOOL)(NOS8i>=TOS8i); goto bol;
case I_I64OPER+6: if (NOS8i==NONEBIGINT) NOS8i=TOS8i;  else if (TOS8i!=NONEBIGINT) NOS8i+=TOS8i;
                  cdp->RV.stt--;  break;
case I_I64OPER+7: if (NOS8i==NONEBIGINT) NOS8i=-TOS8i; else if (TOS8i!=NONEBIGINT) NOS8i-=TOS8i;
                  cdp->RV.stt--;  break;
case I_I64OPER+8: if (TOS8i==NONEBIGINT) NOS8i=NONEBIGINT; else if (NOS8i!=NONEBIGINT) NOS8i*=TOS8i;
                  cdp->RV.stt--;  break;
case I_I64OPER+9: if (TOS8i==NONEBIGINT) NOS8i=NONEBIGINT; else if (NOS8i!=NONEBIGINT)
                                         if (TOS8i) NOS8i/=TOS8i; else
                    cdp->RV.wexception=DIV0_ERROR;
                  cdp->RV.stt--;  break;
case I_FOPER+0: bb=(BOOL)(NOS8< TOS8); goto bol;
case I_FOPER+1: bb=(BOOL)(NOS8==TOS8); goto bol;
case I_FOPER+2: bb=(BOOL)(NOS8> TOS8); goto bol;
case I_FOPER+3: bb=(BOOL)(NOS8<=TOS8); goto bol;
case I_FOPER+4: bb=(BOOL)(NOS8!=TOS8); goto bol;
case I_FOPER+5: bb=(BOOL)(NOS8>=TOS8); goto bol;
case I_FOPER+6: if (NOS8==NULLREAL) NOS8=TOS8;     else if (TOS8!=NULLREAL) NOS8+=TOS8;
                  cdp->RV.stt--;  break;
case I_FOPER+7: if (NOS8==NULLREAL)
                  if (TOS8==NULLREAL) NOS8=0.0f;  else NOS8=-TOS8;
                else if (TOS8!=NULLREAL) NOS8-=TOS8;
                  cdp->RV.stt--;  break;
case I_FOPER+8: if (TOS8==NULLREAL) NOS8=NULLREAL; else if (NOS8!=NULLREAL) NOS8*=TOS8;
                  cdp->RV.stt--;  break;
case I_FOPER+9: if (TOS8==NULLREAL) NOS8=NULLREAL; else if (NOS8!=NULLREAL)
                if (TOS8==0.0) cdp->RV.wexception=FP_ERROR; else NOS8/=TOS8;
                cdp->RV.stt--;  break;
case I_DATMINUS:NOS4=((uns32)NOS4!=(uns32)NONEDATE && (uns32)TOS4!=(uns32)NONEDATE) ? dt2days(NOS4)-dt2days(TOS4) : 0;
                cdp->RV.stt--; break; /* days between dates */
case I_DATPLUS: if ((uns32)NOS4!=(uns32)NONEDATE && TOS4!=NONEINTEGER) NOS4=days2dt(dt2days(NOS4)+TOS4);
                cdp->RV.stt--; break; /* incr/decr. dates */
case I_TSMINUS: NOS4=_ts_minus(NOS4, TOS4);  cdp->RV.stt--; break;
case I_TSPLUS:  NOS4=_ts_plus (NOS4, TOS4);  cdp->RV.stt--; break;
case I_MOPER+0: bb=cmpmoney(NOSM, TOSM) <  0;  goto bol;
case I_MOPER+1: bb=cmpmoney(NOSM, TOSM) == 0;  goto bol;
case I_MOPER+2: bb=cmpmoney(NOSM, TOSM) >  0;  goto bol;
case I_MOPER+3: bb=cmpmoney(NOSM, TOSM) <= 0;  goto bol;
case I_MOPER+4: bb=cmpmoney(NOSM, TOSM) != 0;  goto bol;
case I_MOPER+5: bb=cmpmoney(NOSM, TOSM) >= 0;  goto bol;
case I_MPLUS:   moneyadd (NOSM, TOSM);  cdp->RV.stt--;  break;
case I_MMINUS:  moneysub (NOSM, TOSM);  cdp->RV.stt--;  break;
case I_MTIMES:  //moneymult(NOSM, TOS4);  cdp->RV.stt--;  break;
                if (TOS8i==NONEBIGINT) 
                  { cdp->RV.stt[-1].moneys.money_hi4=NONEINTEGER;  cdp->RV.stt[-1].moneys.money_lo2=0; }
                else if (!IS_NULLMONEY(NOSM))
                { sig64 val = 0;  memcpy(&val, NOSM, MONEY_SIZE);
                  val*=TOS8i;
                  memcpy(NOSM, &val, MONEY_SIZE);
                }
                cdp->RV.stt--;
                break;
case I_MDIV:    moneydiv (NOSM, TOSM);  cdp->RV.stt--;  break;

case I_STRREL+0:bb=(BOOL)(cmp_str(NOSP,TOSP,t_specif().set_sprops(DT2)) <0);  cdp->RV.pc+=2;  goto bol;
case I_STRREL+1:bb=(BOOL)(cmp_str(NOSP,TOSP,t_specif().set_sprops(DT2))==0);  cdp->RV.pc+=2;  goto bol;
case I_STRREL+2:bb=(BOOL)(cmp_str(NOSP,TOSP,t_specif().set_sprops(DT2)) >0);  cdp->RV.pc+=2;  goto bol;
case I_STRREL+3:bb=(BOOL)(cmp_str(NOSP,TOSP,t_specif().set_sprops(DT2))<=0);  cdp->RV.pc+=2;  goto bol;
case I_STRREL+4:bb=(BOOL)(cmp_str(NOSP,TOSP,t_specif().set_sprops(DT2))!=0);  cdp->RV.pc+=2;  goto bol;
case I_STRREL+5:bb=(BOOL)(cmp_str(NOSP,TOSP,t_specif().set_sprops(DT2))>=0);  cdp->RV.pc+=2;  goto bol;
case I_STRREL+6:NOSP=sp_strcat(NOSP, TOSP);  cdp->RV.stt--;  break;
case I_STRREL+7:bb=Like(NOSP,TOSP);                                           cdp->RV.pc+=2;  goto bol;  /* string variant not used */
case I_STRREL+8:bb=general_Substr(NOSP,TOSP, t_specif().set_sprops(DT2))!=0;  cdp->RV.pc+=2;  goto bol;
case I_STRREL+9:bb=general_Pref  (NOSP,TOSP, t_specif().set_sprops(DT2));     cdp->RV.pc+=2;  goto bol;
case I_BINARYOP  :NOS4=memcmp(NOSP, TOSP, DT1) <0;  cdp->RV.pc++;  cdp->RV.stt--;  break;
case I_BINARYOP+1:NOS4=memcmp(NOSP, TOSP, DT1)==0;  cdp->RV.pc++;  cdp->RV.stt--;  break;
case I_BINARYOP+2:NOS4=memcmp(NOSP, TOSP, DT1) >0;  cdp->RV.pc++;  cdp->RV.stt--;  break;
case I_BINARYOP+3:NOS4=memcmp(NOSP, TOSP, DT1)<=0;  cdp->RV.pc++;  cdp->RV.stt--;  break;
case I_BINARYOP+4:NOS4=memcmp(NOSP, TOSP, DT1)!=0;  cdp->RV.pc++;  cdp->RV.stt--;  break;
case I_BINARYOP+5:NOS4=memcmp(NOSP, TOSP, DT1)>=0;  cdp->RV.pc++;  cdp->RV.stt--;  break;
case I_SWAP:
{ basicval bval;
  memcpy(&bval, cdp->RV.stt-1, sizeof(basicval));
  memcpy(cdp->RV.stt-1, cdp->RV.stt, sizeof(basicval));
  memcpy(cdp->RV.stt, &bval, sizeof(basicval));  break;
}
case I_SAVE:    cdp->RV.testedval=*(cdp->RV.stt--);  break;
case I_RESTORE: memcpy(++cdp->RV.stt, &cdp->RV.testedval, sizeof(basicval));  break;
/*************************** database operations ****************************/
case I_CURKONT: cdp->RV.stt++;
                TOSWB.position=cdp->RV.kont_p[DT1];
                TOSWB.cursnum =cdp->RV.kont_c[DT1];
                TOSWB.index=NOINDEX;   /* no index so far */
                cdp->RV.pc++;
                break;
case I_ADD_CURSNUM:
              { tcursnum cursnum;
                cursnum=NOS4; NOS4=TOS4; cdp->RV.stt--;
                TOSWB.cursnum=cursnum;
                TOSWB.index=NOINDEX;   /* no index so far */
                break;
              }
case I_PUSH_KONTEXT:
                memmov(cdp->RV.kont_c+1, cdp->RV.kont_c, (MAX_KONT_SP-1)*sizeof(tcursnum));
                memmov(cdp->RV.kont_p+1, cdp->RV.kont_p, (MAX_KONT_SP-1)*sizeof(trecnum));
                *cdp->RV.kont_c=TOSWB.cursnum;
                *cdp->RV.kont_p=TOSWB.position;
                cdp->RV.stt--; break;
case I_DROP_KONTEXT:
                memmov(cdp->RV.kont_c, cdp->RV.kont_c+1, (MAX_KONT_SP-1)*sizeof(tcursnum));
                memmov(cdp->RV.kont_p, cdp->RV.kont_p+1, (MAX_KONT_SP-1)*sizeof(trecnum));
                break;
case I_ADD_ATRNUM:
                TOSWB.attr=*(tattrib*)(cdp->RV.runned_code+cdp->RV.pc);  cdp->RV.pc+=sizeof(tattrib);  break;
case I_SAVE_DB_IND:
                cdp->RV.stt--;  TOSWB.index=cdp->RV.stt[1].int16;  break;

case I_DBREAD_NUM:
                if (db_read_num(cdp, &TOSWB))  run_db_error(cdp, KERNEL_CALL_ERROR);  break;
case I_DBREAD_LEN:
                if (db_read_len(cdp, &TOSWB))  run_db_error(cdp, KERNEL_CALL_ERROR);  break;
case I_DBREAD_INT:
 { uns32 start;  uns16 size;
   size=TOS2;  cdp->RV.stt--;  start=TOS4;  cdp->RV.stt--;
   if (db_read_int(cdp, start, size, FALSE)) run_db_error(cdp, KERNEL_CALL_ERROR);
   break;
 }
case I_DBREAD_INTS:
 { uns32 start;  uns16 size;
   size=TOS2;  cdp->RV.stt--;  start=TOS4;  cdp->RV.stt--;
   if (size > DT2) size=DT2;  cdp->RV.pc+=2;
   if (db_read_int(cdp, start, size, TRUE)) run_db_error(cdp, KERNEL_CALL_ERROR);
   break;
 }
case I_DBREAD:  if (db_read (cdp,&TOSWB)) run_db_error(cdp, KERNEL_CALL_ERROR);  break;
case I_DBREADS: if (db_reads(cdp,&TOSWB)) run_db_error(cdp, KERNEL_CALL_ERROR);  break;
case I_DBWRITE_NUM:
                if (db_write_num(cdp))    run_db_error(cdp, KERNEL_CALL_ERROR); break;
case I_DBWRITE_LEN:
                if (db_write_len(cdp))    run_db_error(cdp, KERNEL_CALL_ERROR); break;
case I_DBWRITE_INT:
 { tptr dataptr;  uns32 start;  uns16 size;
   dataptr=TOSP;  cdp->RV.stt--;  size=TOS2;  cdp->RV.stt--;  start=TOS4;  cdp->RV.stt--;
   if (db_write_int(cdp, dataptr+2, start, size)) run_db_error(cdp, KERNEL_CALL_ERROR);
   break;
 }
case I_DBWRITE_INTS:
 { tptr dataptr;  uns32 start;  int size;
        dataptr=TOSP;  cdp->RV.stt--;  size=TOS4;  cdp->RV.stt--;  start=TOS4;  cdp->RV.stt--;
        if (size==-1 || size > (int)strlen(dataptr)) size=(int)strlen(dataptr);
        if (db_write_int(cdp, dataptr, start, (uns16)size)) run_db_error(cdp, KERNEL_CALL_ERROR);
   break;
 }
case I_DBWRITE: if (db_write(cdp, &NOSWB, DT1, (tptr)cdp->RV.stt)) run_db_error(cdp, KERNEL_CALL_ERROR);
                cdp->RV.pc++;  cdp->RV.stt-=2;  break;
case I_DBWRITES:if (db_write(cdp, &NOSWB, DT1, TOSP)) run_db_error(cdp, KERNEL_CALL_ERROR);
                cdp->RV.pc++;  cdp->RV.stt-=2;  break;

case I_FAST_DBREAD:
{ uns8 level=DT1;  cdp->RV.pc++;  
  tattrib atr = *(tattrib*)(cdp->RV.runned_code+cdp->RV.pc);  cdp->RV.pc+=sizeof(tattrib);
  (++cdp->RV.stt)->int32=0;  /* used when shorter value is read */
  if (cd_Read(cdp, cdp->RV.kont_c[level], cdp->RV.kont_p[level], atr,
           NULL, (tptr)cdp->RV.stt))
    run_db_error(cdp, KERNEL_CALL_ERROR);
  break;
}
case I_FAST_STRING_DBREAD:
{ uns8 level=DT1;  cdp->RV.pc++;  
  tattrib atr = *(tattrib*)(cdp->RV.runned_code+cdp->RV.pc);  cdp->RV.pc+=sizeof(tattrib);
  cdp->RV.stt++;
  if (cd_Read(cdp, cdp->RV.kont_c[level], cdp->RV.kont_p[level], atr,
           NULL, cdp->RV.hyperbuf))
    run_db_error(cdp, KERNEL_CALL_ERROR);
  TOSP=cdp->RV.hyperbuf;  break;
}
case I_MAKE_PTR_STEP:    
  { char buf[6];
    if (TOSWB.position==NORECNUM) r_error(NIL_POINTER);
    if (PtrSteps(TOSWB.cursnum, TOSWB.position, TOSWB.attr, TOSWB.index,
                 buf)) run_db_error(cdp, KERNEL_CALL_ERROR);
    TOSWB.cursnum=*(ttablenum*)buf;
    TOSWB.position=*(trecnum*)(buf+sizeof(ttablenum));
    TOSWB.index=NOINDEX;
  }
  break;
case I_SWITCH_KONT:
  cdp->RV.kont_c[2]=cdp->RV.kont_c[1]; cdp->RV.kont_c[1]=cdp->RV.kont_c[0]; cdp->RV.kont_c[0]=cdp->RV.kont_c[2];
  cdp->RV.kont_p[2]=cdp->RV.kont_p[1]; cdp->RV.kont_p[1]=cdp->RV.kont_p[0]; cdp->RV.kont_p[0]=cdp->RV.kont_p[2];
  break;
case I_WRITE:
{ int fl = TOS2;  cdp->RV.stt--;
  win_write(cdp, fl, DT1, &cdp->RV);  cdp->RV.pc++;  break;
}
case I_READ:
{ int fl = TOS2;  cdp->RV.stt--;
  win_read (cdp, fl, DT1, &cdp->RV);  cdp->RV.pc++;  break;
}
case I_SYS_VAR:
  cdp->RV.stt++;
//  if (DT1==3) // destination server
//    cdp->RV.stt->ptr=(tptr)cdp->all_views;
//  else
    cdp->RV.stt->int32 = (DT1==1) ? (sig32)cdp->RV.sys_index :
                             ( (DT1) ? (sig32)cdp->RV.current_page_number : (sig32)cdp->RV.view_position);
  cdp->RV.pc++; break;
case I_TRANSF_A:
  if (NOSWB.position==NORECNUM)
  { if (!cdp->RV.appended)
    { cdp->RV.append_posit=cd_Append(cdp, NOSWB.cursnum);
      if (cdp->RV.append_posit==NORECNUM) run_db_error(cdp, KERNEL_CALL_ERROR);
      cdp->RV.appended=wbtrue;
         }
    NOSWB.position=cdp->RV.append_posit;
  }
  if (!NOSWB.cursnum) 
    { client_error(cdp, NO_RIGHT);  run_db_error(cdp, KERNEL_CALL_ERROR); }
  if (cd_Db_copy(cdp, NOSWB.cursnum, NOSWB.position, NOSWB.attr, NOSWB.index,
                          TOSWB.cursnum, TOSWB.position, TOSWB.attr, TOSWB.index))
       run_db_error(cdp, KERNEL_CALL_ERROR);
  cdp->RV.stt-=2; break;
case I_TEXTSUBSTR:
{ BOOL maybe=FALSE;  uns32 pos;
  if (!cd_Text_substring(cdp, NOSWB.cursnum, NOSWB.position, NOSWB.attr, NOSWB.index,
                      cdp->RV.stt->ptr, (int)strlen(cdp->RV.stt->ptr),
                      0, 0/*SEARCH_CASE_SENSITIVE*/, &pos)) maybe=TRUE;
  cdp->RV.stt--;
  cdp->RV.stt->int16 = (uns16)((maybe && (pos!=(uns32)-1)) ? 1 : 0);
  break;
}
case I_LOAD_OBJECT:
  { char * p;  tobjnum * pobjnum;  tcateg cat;
    cat=DT1;  cdp->RV.pc++;
    p=(char *)cdp->RV.run_project.common_proj_code;
    p+=DT4;  cdp->RV.pc+=4;  cdp->RV.stt++;
    if (cat==CATEG_VIEW) { TOSP=(tptr)p;  break; }
    pobjnum=(tobjnum*)(p+OBJ_NAME_LEN+1);
    if (cat==CATEG_CURSOR) pobjnum++;
    if ((*pobjnum==NOOBJECT) && (*(uns8*)p!=0xdc))
    { if (cd_Find_object(cdp, p, cat, pobjnum))
      { 
#ifdef WINS
#if WBVERS<900
        cd_Signalize(cdp);
#endif
#endif
        r_error(OBJECT_DOESNT_EXIST);
      }
    }
    if (cat==CATEG_TABLE) TOS4=*pobjnum;
    else TOSP=(tptr)(pobjnum-1);  /* cursor access */
    break;
  }
case I_MOVESTRING:  // used by ATT_BINARY too
  ptr=cdp->RV.stt->ptr;
  if ((ptr==cdp->RV.hyperbuf) || (ptr==cdp->RV.hyperbuf+2))
//    if ((len=strlen(ptr)) < STR_BUF_SIZE)
    { //memcpy(cdp->RV.strbuf, ptr, len);  cdp->RV.strbuf[len]=0;
      memcpy(cdp->RV.strbuf, ptr, STR_BUF_SIZE);
      cdp->RV.stt->ptr=(tptr)cdp->RV.strbuf;
    }
  break;
case I_CHAR2STR: cdp->RV.charstr[0]=(char)TOS2;  cdp->RV.charstr[1]=0;
         TOSP=(tptr)cdp->RV.charstr;  break;
case I_READREC:    
{ uns16 transsize=DT2;  cdp->RV.pc+=2;
  if (cd_Read_record(cdp, TOSWB.cursnum, TOSWB.position, NOSP, (uns16)transsize))
    run_db_error(cdp, KERNEL_CALL_ERROR);
  cdp->RV.stt-=2;    break;
}
case I_WRITEREC:    
{ uns16 transsize=DT2;  cdp->RV.pc+=2;
  if (SYSTEM_TABLE(NOSWB.cursnum)) run_db_error(cdp, KERNEL_CALL_ERROR);
    if (cd_Write_record(cdp, NOSWB.cursnum,NOSWB.position,TOSP,(uns16)transsize))
      run_db_error(cdp, KERNEL_CALL_ERROR);
  cdp->RV.stt-=2;    break;
}
case I_JMPTDR: if (!cdp->RV.stt->int16) { cdp->RV.pc += 2;  cdp->RV.stt--; }
               else cdp->RV.pc+=(sig16)DT2;
               break;
case I_JMPFDR: if ( cdp->RV.stt->int16) { cdp->RV.pc += 2;  cdp->RV.stt--; }
               else cdp->RV.pc+=(sig16)DT2;
               break;
// conversions:
case I_T16TO32: if (TOS2==NONESHORT) TOS4=NONEINTEGER;
                else TOS4=(sig32)(sig16)TOS2;  break;
case I_T32TO16: if (TOS4==NONEINTEGER) TOS4=(sig32)(uns16)0x8000;
                                         else TOS4=(sig32)(uns16)TOS2;  break;
case I_N16TO32: if (NOS2==NONESHORT) NOS4=NONEINTEGER;
                else NOS4=(sig32)(sig16)NOS2;  break;
case I_N8TO32:  if (NOS1==NONETINY) NOS4=NONEINTEGER;
                else NOS4=(sig32)(sig8)NOS1;  break;
case I_T8TO32:  if (TOS1==NONETINY) TOS4=NONEINTEGER;
                else TOS4=(sig32)(sig8)TOS1;  break;
case I_T64TOF:  TOS8 =(TOS8i==NONEBIGINT)  ? NULLREAL    : (double)TOS8i;  break;
case I_T64TO32: TOS4 =(TOS8i==NONEBIGINT)  ? NONEINTEGER : (sig32 )TOS8i;  break;
case I_T32TO8:  TOS1 =(TOS4 ==NONEINTEGER) ? NONETINY    : (sig8  )TOS4;   break;
case I_TFTO64:  TOS8i=(TOS8 ==NULLREAL)    ? NONEBIGINT  : (sig64 )TOS8;   break;
case I_T32TO64: TOS8i=(TOS4 ==NONEINTEGER) ? NONEBIGINT  : (sig64 )TOS4;   break;

case I_LOADNULL:
{ uns8 tp=DT1;  cdp->RV.pc++;  cdp->RV.stt++;
  switch (tp)
  { case ATT_BOOLEAN:  case ATT_INT8:
      cdp->RV.stt->int32=NONETINY;  break;
    case ATT_CHAR:
      cdp->RV.stt->int32=0;  break;
    case ATT_INT16:
      cdp->RV.stt->int32=NONESHORT;  break;
    case ATT_INT32:  case ATT_DATE:  case ATT_TIME:  case ATT_TIMESTAMP:
      cdp->RV.stt->int32=NONEINTEGER;  break;
    case ATT_INT64:
      cdp->RV.stt->int64=NONEBIGINT;  break;
    case ATT_FLOAT:
      cdp->RV.stt->real=NONEREAL;  break;
    case ATT_MONEY:
      cdp->RV.stt->moneys.money_hi4=NONEINTEGER;
      cdp->RV.stt->moneys.money_lo2=0;  break;
    case ATT_STRING:
      cdp->RV.hyperbuf[0]=0;
      cdp->RV.stt->ptr=cdp->RV.hyperbuf;  break;
    case ATT_PTR:  case ATT_BIPTR:
      cdp->RV.stt->int32=-1;  break;
  }
  break;
}
case I_UNTASGN:   /* untyped assignment */
{ uns8 tp1, tp2, flags, len;
  tp1  =DT1;  cdp->RV.pc++;  tp2=DT1;  cdp->RV.pc++;
  flags=DT1;  cdp->RV.pc++;  len=DT1;  cdp->RV.pc++;
  int res=untyped_assignment(cdp, cdp->RV.stt-1, cdp->RV.stt, tp1, tp2, flags, len);
  if (res) run_db_error(cdp, res);
  if (flags & UNT_DESTPAR)
  { cdp->RV.stt--;
    TOSP += ((len-1)/sizeof(void*) + 1) * sizeof(void*);
  }
  else cdp->RV.stt-=2;
  break;
}
case I_UNTATTR:   /* unspecified attribute access */
{ d_attr datr;  tptr name = (tptr)cdp->RV.runned_code+cdp->RV.pc;
  cdp->RV.pc+=ATTRNAMELEN+1;
  if (!find_attr(cdp, TOSWB.cursnum, CATEG_TABLE, name, NULL, NULL, &datr))
    r_error(ATTRIBUTE_DOESNT_EXIST);
  TOSWB.attr=datr.get_num();
  break;
}
case I_CHECK_BOUND:
  if (TOS4 >= (sig32)DT4) r_error(ARRAY_BOUND_EXCEEDED);
  cdp->RV.pc+=4;
  break;

case I_LOAD_CACHE_ADDR:
{ tptr p =load_inst_cache_addr(cdp->RV.inst, TOSWB.position, TOSWB.attr, TOSWB.index, TRUE);
  if (p==NULL) r_error(KERNEL_CALL_ERROR);  /* no read right or internal error */
  TOSP=p;
  break;
}
case I_LOAD_CACHE_ADDR_CH:
{ tptr p =load_inst_cache_addr(cdp->RV.inst, TOSWB.position, TOSWB.attr, TOSWB.index, TRUE);
  if (p==NULL) r_error(KERNEL_CALL_ERROR);  /* no read right or internal error */
#if WBVERS<900
  register_edit(cdp->RV.inst, TOSWB.attr);
#endif
  TOSP=p;
  break;
}
case I_LOAD_CACHE_TEST:
{ tptr p =load_inst_cache_addr(cdp->RV.inst, TOSWB.position, TOSWB.attr, TOSWB.index, FALSE);
  if (p==NULL) r_error(INDEX_OUT_OF_RANGE);  /* no read right or internal error possible */
  TOSP=p;
  break;
}
case I_GET_CACHED_MCNT:
{ tptr p =load_inst_cache_addr(cdp->RV.inst, TOSWB.position, TOSWB.attr, NOINDEX, FALSE);
  if (p==NULL) r_error(KERNEL_CALL_ERROR);  /* no read right or internal error */
  TOS4=((t_dynar*)p)->count();
  break;
}
#ifdef WINS /////////////////////// WINS only ///////////////////////////////////////////
#if WBVERS<900
case I_AGGREG:
{ uns16 index = DT2;
  cdp->RV.pc+=2;  cdp->RV.stt++;
  aggr_get(cdp, index, DT1, cdp->RV.stt);  cdp->RV.pc++;  break;
}
case I_CALLPROP:
{ cdp->RV.stt--;  // parameter frame pointer
  unsigned method_number = DT2;  cdp->RV.pc+=2;
  unsigned tp = DT1;             cdp->RV.pc++;
  scopevars * parptr=cdp->RV.prep_vars;  cdp->RV.prep_vars=cdp->RV.prep_vars->next_scope;
 // check the instance pointer:
  view_dyn * inst = *(view_dyn**)parptr->vars;  // stdcall
  if ((inst==NULL || inst->stat==NULL) &&         // non-existing view or non-compiled view 
      method_number!=13 && method_number!=62 && method_number!=84)  // unless Is_open method, Close, Set_source
  { if (tp)
      if (is_string(tp)) { cdp->RV.stt++;  cdp->RV.stt->ptr=NULLSTRING; }
      else               { cdp->RV.stt++;  cdp->RV.stt->real=0; }
    corefree(parptr);
  }
  else call_method(cdp, inst, method_number, parptr, tp); /* corefree(parptr); inside! */
  break;
}
case I_CALLPROP_BYNAME:
{ cdp->RV.stt--;  // parameter frame pointer
  unsigned method_number = DT2;  cdp->RV.pc+=2;
  unsigned tp = DT1;             cdp->RV.pc++;
  scopevars * parptr=cdp->RV.prep_vars;  cdp->RV.prep_vars=cdp->RV.prep_vars->next_scope;
 // find the instance:
  tptr viewname = *(tptr*)parptr->vars;
  view_dyn * inst;
  if (*viewname==1) // THISFORM
  { inst=cdp->RV.inst;
    if (!inst) r_error(INDEX_OUT_OF_RANGE);  // thisform used out of kontext
  }
  else
  { inst = find_inst(cdp, viewname);
    if (inst==NULL &&  // non-opened view 
        method_number!=13 && method_number!=62 && method_number!=84)  // unless Is_open method, Close, Set_source
    { char buf[1+sizeof(tobjname)];  *buf='*';  strcpy(buf+1, viewname);
      if (wsave_run(cdp)) r_error(OUT_OF_R_MEMORY);
      cd_Open_view(cdp, buf, NO_REDIR, 0, 0, NULL, 0, NULL);
      wrestore_run(cdp);
      inst = find_inst(cdp, viewname);
      if (!inst) r_error(INDEX_OUT_OF_RANGE);  // internal error
    }
  }
  *(view_dyn**)parptr->vars = inst; // replace view name by instance pointer in method parameters
  call_method(cdp, inst, method_number, parptr, tp); /* corefree(parptr); inside! */
  break;
}
case I_GET_THISFORM:
    if (!cdp->RV.inst) r_error(INDEX_OUT_OF_RANGE);  // thisform used out of kontext
    cdp->RV.stt++;  TOSP=(tptr)cdp->RV.inst;
    break;

case I_CONSTRUCT:  // called when entering a scope with a local object
{ tptr p = (char*)cdp->RV.runned_code+cdp->RV.pc;  cdp->RV.pc += strlen(p)+1;
  construct_object(cdp, TOSP, p);  cdp->RV.stt--;
  break;
}
case I_DESTRUCT:   // called when leaving a scope with a local object
  destruct_object(cdp, TOSP);  cdp->RV.stt--;  break;
#endif //////////////////////////// WINS only ///////////////////////////////////////////
#endif

default: 
  r_error(UNDEF_INSTR); 
    } /* end of the instruction switch */
    if ((uns16)((tptr)(cdp->RV.stt+1)-(tptr)cdp->RV.st) >= sizeof(basicval)*I_STACK_SIZE)
      r_error(STACK_OVFL);
    if (cdp->RV.break_request) r_error(PGM_BREAK);
    if (cdp->RV.wexception)
    { switch (cdp->RV.wexception)
      { case FP_ERROR:  case DIV0_ERROR:
          cdp->RV.wexception=NO_EXCEPTION;  r_error(MATHERR);
        case STEP_AND_BP:
          if (cdp->RV.runned_code!=cdp->RV.bpi->code_addr) break;
          realize_bps(cdp);  cdp->RV.wexception=IN_STEP;
          goto check_line;
        case TRACE_AND_BP:
          if (cdp->RV.runned_code!=cdp->RV.bpi->code_addr) break;
          realize_bps(cdp);  cdp->RV.wexception=IN_TRACE;
          goto check_line;
        case SET_BP:
          if (cdp->RV.runned_code!=cdp->RV.bpi->code_addr) break;
          realize_bps(cdp);  cdp->RV.wexception=NO_EXCEPTION;
          break;
        case IN_STEP:  case IN_TRACE:
          if (cdp->RV.runned_code!=cdp->RV.bpi->code_addr) break;
         check_line:
          { uns8 a_file;  unsigned a_line;
            line_from_offset(cdp, cdp->RV.pc, cdp->RV.runned_code, &a_line, &a_file);
            if ((a_line!=cdp->RV.bpi->current_line) || (a_file!=cdp->RV.bpi->current_file))
            { unrealize_bps(cdp);  cdp->RV.wexception=NO_EXCEPTION;
              line_from_offset(cdp, cdp->RV.pc, cdp->RV.runned_code, &cdp->RV.bpi->current_line, &cdp->RV.bpi->current_file);
              return PGM_BREAKED;
            }
            break;
          }
        case BREAK_EXCEPTION:
          if (cdp->RV.runned_code!=cdp->RV.bpi->code_addr) break;
          unrealize_bps(cdp);  cdp->RV.wexception=NO_EXCEPTION;
          line_from_offset(cdp, cdp->RV.pc, cdp->RV.runned_code, &cdp->RV.bpi->current_line, &cdp->RV.bpi->current_file);
          return PGM_BREAKED;
      }
    }
   }while (TRUE);
  }
#ifdef WINS
  __except ((GetExceptionCode() < 256 ||
    GetExceptionCode() >= FIRST_RT_ERROR   && GetExceptionCode() <= FIRST_RT_ERROR+50 ||
    GetExceptionCode() >= FIRST_COMP_ERROR && GetExceptionCode() <= LAST_COMP_ERROR)
    ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)
  { run_err=(short)GetExceptionCode();
#else
  else
  {
#endif

    if (is_main_level_code(cdp))
    { unrealize_bps(cdp);
      line_from_offset(cdp, cdp->RV.pc, cdp->RV.runned_code, &cdp->RV.bpi->current_line, &cdp->RV.bpi->current_file);
    }
    free_run_vars(cdp);
    return run_err==KERNEL_CALL_ERROR && cdp->RV.report_number ?
      cdp->RV.report_number : run_err;
  }
}

/******************************* aggeregates ********************************/
void add_val(tptr sum, tptr val, uns8 type)
{ switch (type)
  { case ATT_CHAR:  *(sig32*)sum += *val;                    break;
    case ATT_INT16: *(sig32*)sum += *(sig16*)val;            break;
    case ATT_INT8:  *(sig32*)sum += *(sig8 *)val;            break;
    case ATT_MONEY: moneyadd((monstr *)sum, (monstr *)val);  break;
    case ATT_DATE:  case ATT_TIME:
    case ATT_INT32: *(sig32*)sum += *(sig32*)val;            break;
    case ATT_INT64: *(sig64*)sum += *(sig64*)val;            break;
    case ATT_FLOAT: *(double*)sum += *(double*)val;          break;
  }
}

#if defined(WINS) && (WBVERS<900)
CFNC DllKernel int WINAPI test_grouping(cdp_t cdp, trecnum rec)
/* returns the number of the biggest group that is different for the given
        record than for the stored one. Stores the grouping values for
        the given record. */
{ short group, level;  trecnum rec1;  tptr grp_vals;  uns16 * grp_val_offs;
  tptr code;  int res;
  rec1=cdp->RV.view_position=rec;
  wsave_run(cdp);
  set_top_rec((view_dyn*)cdp->RV.view_ptr, rec);
  wrestore_run(cdp);
  if ((rec1=inter2exter((view_dyn*)cdp->RV.view_ptr, rec))==NORECNUM)
    return 0;  /* full grouping: no record follows */
  grp_vals=cdp->RV.aggr->grp_vals;
  grp_val_offs=cdp->RV.aggr->grp_val_offs;
  *cdp->RV.kont_p=rec1;
  wsave_run(cdp);
  level=GR_LEVEL_CNT;
  for (group=1; group<GR_LEVEL_CNT; group++)
  { code=get_var_value(&((view_dyn*)cdp->RV.view_ptr)->stat->vwCodeSize,
                       (BYTE)(VW_GROUPA+group-1));
    if (code)
    { int tp = code[1];  t_specif specif = *(t_specif*)(code+2);
      run_prep(cdp, code, 1+1+sizeof(t_specif));
      res=run_pgm(cdp);
      if (res==EXECUTED_OK)
      { if (is_string(tp))
        { res=cmp_str(grp_vals+grp_val_offs[group], cdp->RV.stt->ptr, specif);
               strcpy(grp_vals+grp_val_offs[group], cdp->RV.stt->ptr);
        }
        else
        { res=memcmp(grp_vals+grp_val_offs[group], cdp->RV.stt,specif.length);
              memcpy(grp_vals+grp_val_offs[group], cdp->RV.stt,specif.length);
        }
        if (res) if (level==GR_LEVEL_CNT) level=group;
      }
      else
        if (cd_Sz_error(cdp)==OUT_OF_TABLE) { level=0;  break; }
    }
  }
  wrestore_run(cdp);
  return level;
}


CFNC DllKernel void WINAPI free_aggr_info(aggr_info ** paggr)
{ aggr_info * pt;
  while (*paggr)
  { pt=*paggr;
    *paggr=(*paggr)->next_info;
    corefree(pt);
  }
}

CFNC DllKernel void WINAPI aggr_reset(cdp_t cdp, int level)
{ aggr_info * pt;  int i;  tptr ptr1, ptr2, ptr3;
  for (i=level; i<GR_LEVEL_CNT; i++)
  { cdp->RV.aggr->aggr_completed[i]=wbfalse; cdp->RV.aggr->aggr_count[i]=0;
         cdp->RV.aggr->aggr_added[i]=0;   /* limit removed */
  }
  pt=cdp->RV.aggr->aggr_head;
  while (pt)
  { for (i=level; i<GR_LEVEL_CNT; i++)
    { pt->count[i]=0;
      ptr1=pt->vls+pt->codesize+i*3*pt->valsize;
      ptr2=ptr1+pt->valsize;
      ptr3=ptr2+pt->valsize;
      switch (pt->type)
      { case ATT_STRING: *ptr1=(uns8)0xff; *ptr2=0; break;
        case ATT_MONEY:((monstr*)ptr1)->money_hi4=0x7fffffffL;
                       ((monstr*)ptr1)->money_lo2=0xffff;
                       ((monstr*)ptr2)->money_hi4=0x80000000L;
                       ((monstr*)ptr2)->money_lo2=0x0001;
                       ((monstr*)ptr3)->money_hi4=0;
                       ((monstr*)ptr3)->money_lo2=0;
                       break;
        case ATT_INT8: *(sig32*)ptr1= 127;
                       *(sig32*)ptr2=-127;
                       *(sig32*)ptr3= 0L; break;
        case ATT_INT16:*(sig32*)ptr1= 32767;
                       *(sig32*)ptr2=-32767;
                       *(sig32*)ptr3= 0L; break;
        case ATT_INT32:*(sig32*)ptr1= 2147483647L;
                       *(sig32*)ptr2=-2147483647L;
                       *(sig32*)ptr3= 0L; break;
        case ATT_INT64:*(sig64*)ptr1= 0x7fffffffffffffff;
                       *(sig64*)ptr2=-0x7fffffffffffffff;
                       *(sig64*)ptr3= 0L; break;
        case ATT_TIME:  case ATT_DATE: case ATT_CHAR:  case ATT_BOOLEAN:
                       *(sig32*)ptr1= 2147483647L;
                       *(sig32*)ptr2= 0L;
                       *(sig32*)ptr3= 0L; break;
        case ATT_FLOAT:*(double*)ptr1= 1.7E308;
                       *(double*)ptr2=-1.7E308;
                       *(double*)ptr3= 0.0; break;
      }
    }
    pt=pt->next_info;
  }
}

void aggr_compl(cdp_t cdp, int level)
/* marks some levels completed */
{ int i;
  for (i=level; i<GR_LEVEL_CNT; i++) cdp->RV.aggr->aggr_completed[i]=wbtrue;
}


CFNC DllKernel void WINAPI aggr_in(cdp_t cdp, trecnum rec)
/* adds record info into uncompleted aggerg. info */
{ aggr_info * pt;  short level, res;  tptr ptr;   trecnum rec1;
  rec1=rec;
  wsave_run(cdp);
  set_top_rec((view_dyn*)cdp->RV.view_ptr, rec);
  wrestore_run(cdp);
  if ((rec1=inter2exter((view_dyn*)cdp->RV.view_ptr, rec))==NORECNUM) return;
  *cdp->RV.kont_p=rec1;
  pt=cdp->RV.aggr->aggr_head;
  while (pt)
  { wsave_run(cdp);
    run_prep(cdp, pt->vls, 0);
    res=run_pgm(cdp);
    if (res==EXECUTED_OK)
    { for (level=0; level<GR_LEVEL_CNT; level++)
        if (!cdp->RV.aggr->aggr_completed[level] && (rec >= cdp->RV.aggr->aggr_added[level]))
        { ptr=pt->vls+pt->codesize+level*3*pt->valsize;
          if (is_string(pt->type))
          { /* pt->count[level]++;  local count ignored */
            if (cmp_str(cdp->RV.stt->ptr, ptr, pt->specif) < 0) 
              strcpy(ptr, cdp->RV.stt->ptr); /* MIN */
            ptr+=pt->valsize;
            if (cmp_str(cdp->RV.stt->ptr, ptr, pt->specif) > 0) 
              strcpy(ptr, cdp->RV.stt->ptr); /* MAX */
          }
          else /* other types */
          { if (!is_null((tptr)cdp->RV.stt,pt->type))
            { pt->count[level]++;
              if (compare_values(pt->type, pt->specif, (tptr)cdp->RV.stt, ptr) < 0)
                memcpy(ptr, cdp->RV.stt, pt->valsize);
              ptr+=pt->valsize;
              if (compare_values(pt->type, pt->specif, (tptr)cdp->RV.stt, ptr) > 0)
                memcpy(ptr, cdp->RV.stt, pt->valsize);
              ptr+=pt->valsize;
              add_val(ptr, (tptr)cdp->RV.stt, pt->type);
            }
          }
        }
    }
    wrestore_run(cdp);
    pt=pt->next_info;
  }
  for (level=0; level<GR_LEVEL_CNT; level++)
    if (!cdp->RV.aggr->aggr_completed[level])
      if (rec >= cdp->RV.aggr->aggr_added[level])
      { cdp->RV.aggr->aggr_count[level]++;
        cdp->RV.aggr->aggr_added[level]=rec+1;
      }
}

void aggr_catch(cdp_t cdp, short level)
/* makes the "level" of aggr. info complete */
/* the curr_pos is supposed to be already aggr_in'ed */
{ trecnum rec, orig_rec;
  if (cdp->RV.aggr->aggr_completed[level]) return;
  orig_rec=rec=cdp->RV.view_position;
  do
  { rec++;
         aggr_compl(cdp, test_grouping(cdp, rec));
         if (cdp->RV.aggr->aggr_completed[level]) break;
         aggr_in(cdp, rec);
  } while (TRUE);
  test_grouping(cdp, cdp->RV.view_position=orig_rec); /* restore the grouping information */
}

static void aggr_get(cdp_t cdp, uns16 index, uns8 ag_type, basicval * retval)
/* puts into the *retval the aggregate value */
{ aggr_info * pt;   tptr ptr;
  if (!cdp->RV.aggr) 
  { retval->ptr=NULL;  /*return;*/
    r_error(UNSPECIFIED_ERROR);   /* NULL result is bad for strings, must force error */ 
  }
  aggr_catch(cdp, cdp->RV.aggr->glob_level);
  if (ag_type==AG_TYPE_CNT)
         { retval->int32=cdp->RV.aggr->aggr_count[cdp->RV.aggr->glob_level]; return; }
  pt=cdp->RV.aggr->aggr_head;
  while (pt && index--) pt=pt->next_info;
  if (!pt) return; /* error */
  if (ag_type==AG_TYPE_LCN)
         { retval->int32=pt->count[cdp->RV.aggr->glob_level]; return; }
  ptr=pt->vls+pt->codesize+pt->valsize*(3*cdp->RV.aggr->glob_level+ag_type);
  if (is_string(pt->type)) retval->ptr=ptr;
  else memcpy(retval,ptr,pt->valsize);
}

CFNC DllKernel short WINAPI aggr_post(cdp_t cdp, trecnum pos)
{ int gr;
  gr=test_grouping(cdp, pos);
  aggr_compl(cdp, gr); /* mark some aggr. values as completed (incl. report) */
  return gr;
}

#include "dbgint.c"
#else
static void unrealize_bps(cdp_t cdp) { }
static void realize_bps(cdp_t cdp) { }
CFNC DllKernel void WINAPI line_from_offset(cdp_t cdp, unsigned locoffset, uns8 *  loccode, unsigned * linenum, uns8 * filenum) { }
#endif

#if WBVERS>=1000
#include "project9.cpp"
#endif
