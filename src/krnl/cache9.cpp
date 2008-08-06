#ifdef WINS
#include <windows.h>
#else
#include "winrepl.h"
#endif
#include "defs.h"
#include "comp.h"
#include "cint.h"
#pragma hdrstop
#include "wprez9.h"
#include "compil.h"
#include "flstr.h"

#ifdef CLIENT_ODBC_9
#define INDIR_MIN_ALLOC   256
#define INDIR_ALLOC_STEP  4096

static UWORD row_status;  /* must not be automatic,
  SQLExtendedFetch stores the address, SQLSetPos writes there. */

int odbc_rec_status(ltable * dt, trecnum extrec)
{ RETCODE retcode;  SQLULEN fetched;
  if (dt->conn->SetPos_support)   /* indirect objects are bound! */
  { //unsigned i;  atr_info * catr;
    //for (i=1, catr=dt->attrs+1;  i<dt->attrcnt;  i++, catr++)
    //  if ((catr->flags & (CA_MULTIATTR | CA_INDIRECT)) == CA_INDIRECT)
    //  { indir_obj * iobj = (indir_obj*)(dt->odbc_buf+catr->offset);
    //    iobj->obj=NULL;  iobj->actsize=0;
    //  }
    memset(dt->odbc_buf, 0, dt->rec_cache_real);
  }
  retcode=SQLExtendedFetch(dt->hStmt, SQL_FETCH_ABSOLUTE, 1+extrec,
                           &fetched, &row_status);
  if (retcode!=SQL_SUCCESS && retcode!=SQL_SUCCESS_WITH_INFO)
  { if (retcode!=SQL_NO_DATA_FOUND) odbc_error(dt->conn, dt->hStmt);
    return SQL_ROW_NOROW;
  }
  if (!fetched)
    return SQL_ROW_NOROW;
  return row_status==SQL_ROW_UPDATED || row_status==SQL_ROW_ADDED ||
         row_status==SQL_ROW_ERROR ? // added in 5.1c, VUKROM, reading Excel data sometimes returns Error in row, Numeric Field Overflow
    SQL_ROW_SUCCESS : row_status;
}

static BOOL odbc_load_indir_obj(t_connection * conn, HSTMT hStmt, UWORD column, indir_obj * iobj, bool char_data, bool wide)
{ HGLOBAL hnd;  cached_longobj * obj;  uns32 loaded = 0;
  RETCODE retcode;  SQLLEN pcbValue;  uns32 allocstep, rdstep;
  int terminator_size = wide ? 2 : 1;
  hnd=GlobalAlloc(GMEM_MOVEABLE, sizeof(cached_longobj)+INDIR_MIN_ALLOC+terminator_size);
  /* allocation includes 1 byte for terminator (in cached_longobj) */
  if (!hnd) return FALSE;
  obj=(cached_longobj*)GlobalLock(hnd);
  obj->maxsize=INDIR_MIN_ALLOC;
  do
  { rdstep=obj->maxsize-loaded;
    pcbValue=SQL_IGNORE;
    //retcode=SQLGetData(hStmt, column, /*SQL_C_BINARY*/SQL_C_DEFAULT, (char *)obj->data+loaded, rdstep, &pcbValue);
    retcode=SQLGetData(hStmt, column, char_data ? (wide ? SQL_C_WCHAR : SQL_C_CHAR) : SQL_C_BINARY, (char *)obj->data+loaded, rdstep, &pcbValue);  // must use the type of the bound cache
    // pcbValue gets the length of the current rest of the data (or SQL_NO_TOTAL), not the full length and not the read length!!!!!
    if (retcode==SQL_ERROR) odbc_error(conn, hStmt);
    /* for SQL_C_BINARY Access with cursor lib does not write into pcbValue! */
    if (pcbValue==SQL_IGNORE) retcode=SQL_ERROR;
    if (retcode==SQL_SUCCESS_WITH_INFO)
      if (pcbValue<=rdstep)  // otherwise the truncation is obvious, no need to check the error code (not reliable on MySQL)
      /* replace by SQL_SUCCESS if this is not the data truncation: */
      { char szSqlState[SQL_SQLSTATE_SIZE+1];
        do
        { retcode=SQLError(SQL_NULL_HENV, SQL_NULL_HDBC, hStmt, (UCHAR*)szSqlState, NULL, NULL, 0, NULL);
          if (retcode==SQL_SUCCESS || retcode==SQL_SUCCESS_WITH_INFO)
          { if (!strcmp(szSqlState, "01004"))  // data truncated, reading should continue
              { retcode=SQL_SUCCESS_WITH_INFO;  break; }
          }
          else /* no more infos found */
            { retcode=SQL_SUCCESS;  break; }
        } while (TRUE);
      }
    switch (retcode)
    { case SQL_ERROR:  case SQL_INVALID_HANDLE:
        GlobalUnlock(hnd);  GlobalFree(hnd);  return FALSE;
      case SQL_NO_DATA_FOUND:  /* this should never occur */
        ((char *)obj->data)[loaded]=0;   /* terminator */
        iobj->obj=obj;  iobj->actsize=loaded;  return TRUE;
      case SQL_SUCCESS:  /* reading completed */
        if (pcbValue==SQL_NULL_DATA)
        { GlobalUnlock(hnd);  GlobalFree(hnd);
          iobj->obj=NULL;  iobj->actsize=0;  return TRUE;
        }
        if (pcbValue==SQL_NO_TOTAL) /* SQL_NO_TOTAL is an error */
        { GlobalUnlock(hnd);  GlobalFree(hnd);
          iobj->obj=NULL;  iobj->actsize=0;  return FALSE;
        }
        else
        { loaded+=(uns32)pcbValue;
          ((char *)obj->data)[loaded]=0;   /* terminator */
          iobj->obj=obj;  iobj->actsize=loaded;
          return TRUE;
        }
      case SQL_SUCCESS_WITH_INFO:  /* data have been truncated */
        if (pcbValue==SQL_NULL_DATA)  /* this should never occur */
        { GlobalUnlock(hnd);  GlobalFree(hnd);
          iobj->obj=NULL;  iobj->actsize=0;  return TRUE;
        }
        if (pcbValue==SQL_NO_TOTAL)  /* total size unknown, read on */
        { loaded+=char_data ? rdstep-terminator_size : rdstep;
          allocstep=INDIR_ALLOC_STEP;
        }
        else if (pcbValue >= rdstep)
        { int thispart = char_data ? rdstep-terminator_size : rdstep;
          allocstep=(uns32)(pcbValue-loaded+terminator_size);
          loaded+=thispart;
        }
        else  /* this should never occur */
        { ((char *)obj->data)[loaded]=0;   /* terminator */
          iobj->obj=obj;  iobj->actsize=loaded;
          return TRUE;
        }
        break;
    }
   /* allocstep defined, realloc the buffer: */
    if (loaded+allocstep > obj->maxsize)
    { HGLOBAL hnd1;  cached_longobj * obj1;
      hnd1=GlobalAlloc(GMEM_MOVEABLE, sizeof(cached_longobj)+loaded+allocstep);
      if (!hnd1) { GlobalUnlock(hnd);  GlobalFree(hnd);  return FALSE; }
      obj1=(cached_longobj*)GlobalLock(hnd1);
      obj1->maxsize=loaded+allocstep;
      memcpy(obj1->data, obj->data, loaded);
      GlobalUnlock(hnd);  GlobalFree(hnd);  hnd=hnd1;  obj=obj1;
    }
  } while (TRUE);
}

static BOOL odbc_load_record(ltable * dt, trecnum extrec, tptr dest)
/* On error return TRUE and *dest==0xff. */
{ RETCODE retcode;  SQLULEN fetched;  SQLLEN * pcbValue;
  BOOL res=FALSE;  int i;  atr_info * catr;
  memset(dt->odbc_buf, 0, dt->rec_cache_real);
  fetched=0;

  if (!dt->conn->absolute_fetch)
  { if ((int)extrec < (int)dt->cache_top) res=TRUE;
    else
    { while ((int)extrec > (int)dt->cache_top && !res)
      { retcode=SQLExtendedFetch(dt->hStmt, SQL_FETCH_NEXT, 0,
          &fetched, &row_status);
        if (retcode==SQL_ERROR)
        { retcode=SQLFetch(dt->hStmt);
          fetched=1;  row_status=SQL_ROW_SUCCESS;
        }
        if (retcode!=SQL_SUCCESS && retcode!=SQL_SUCCESS_WITH_INFO)
          { odbc_error(dt->conn, dt->hStmt);  res=TRUE; }
        else if (fetched!=1 || (row_status!=SQL_ROW_SUCCESS &&
          row_status!=SQL_ROW_UPDATED && row_status!=SQL_ROW_ADDED)) res=TRUE;
        dt->cache_top++;
      }
    }
  }
  else
  { retcode=SQLExtendedFetch(dt->hStmt, SQL_FETCH_ABSOLUTE, 1+extrec, &fetched, &row_status);
    if (retcode!=SQL_SUCCESS) odbc_error(dt->conn, dt->hStmt);  
    if (retcode!=SQL_SUCCESS && retcode!=SQL_SUCCESS_WITH_INFO)
      res=TRUE;
    else if (fetched!=1 || (row_status!=SQL_ROW_SUCCESS && row_status!=SQL_ROW_UPDATED && row_status!=SQL_ROW_ADDED)) 
      res=TRUE;
  }

  if (!res)
  {/* convert the NULL-value representation: */
    pcbValue=(SQLLEN*)(dt->odbc_buf+dt->rr_datasize);
    for (i=1, catr=dt->attrs+1;  i<dt->attrcnt;  i++, catr++)
      if (!(catr->flags & (CA_INDIRECT|CA_MULTIATTR)))
        if (*(pcbValue++) == SQL_NULL_DATA || !(catr->flags & CA_RI_SELECT))
        // no record privils on ODBC table
          catr_set_null(dt->odbc_buf+catr->offset, catr->type);
    memcpy(dest, dt->odbc_buf, dt->rec_cache_real);
   /* load long values: */
    for (i=1, catr=dt->attrs+1;  i<dt->attrcnt;  i++, catr++)
      if (catr->flags & CA_INDIRECT && (catr->flags & CA_RI_SELECT))
      // no record privils on ODBC table
      { indir_obj * iobj = (indir_obj*)(dest+catr->offset);
        if (!odbc_load_indir_obj(dt->conn, dt->hStmt, i, iobj, is_string(catr->type) || catr->type==ATT_TEXT, catr->specif.wide_char!=0))
          res=TRUE;
      }
  }
  else  // must null the dest
    memcpy(dest, dt->odbc_buf, dt->rec_cache_real);
  *dest = res ? 0xff : REC_EXISTS;
  return res;
}
#endif

/////////////////////////////////////// rempap //////////////////////////////////////
CFNC DllExport BOOL remap_init(view_dyn * inst, BOOL remaping)
// Allocates initial map and sets record numbers to 0.
// Returns TRUE iff cannot init (no memory)/
{ ltable * ldt = inst->dt;
  ldt->remap_a=NULL;  ldt->remap_size=0;
  if (remaping)
  { ldt->remap_size = BMAP_ALLOC_STEP;  // size is in bytes
    ldt->remap_a=(RPART *)corealloc(ldt->remap_size, 88);
    if (!ldt->remap_a) return TRUE;
    memset(ldt->remap_a, 0, ldt->remap_size);
  }
  ldt->rm_completed=false;
  ldt->rm_int_recnum=ldt->rm_ext_recnum=ldt->rm_curr_int=ldt->rm_curr_ext=0;
  SET_FI_INT_RECNUM(inst);
  return FALSE;
}

static bool remap_newsize(ltable * ldt, trecnum bitnum)
// Grows remap so that in contains at least bitnum bits. Returns true on error.
{ uns32 newsize;  RPART * newptr;
  newsize=(bitnum / (8L*BMAP_ALLOC_STEP) + 1) * BMAP_ALLOC_STEP + 2;
  newptr = (RPART*)corerealloc(ldt->remap_a, newsize);
  if (!newptr) return true;
  memset((tptr)newptr + ldt->remap_size, 0, newsize-ldt->remap_size);  /* remap_insert needs it */
  ldt->remap_a=newptr;
  ldt->remap_size=newsize;
  return false;
}

static wbbool remap_sem = 0;

CFNC DllExport BOOL remap_expand(view_dyn * inst, trecnum extstep)
/* reading new parts of remap, increases rm_ext_recnum by extstep */
{ RPART frontier;  trecnum newext, newint;
  ltable * ldt = inst->dt;
  if (inst->xcdp->connection_type!=CONN_TP_ODBC)  // prevent creating rthe remap in grids containing the deleted records
    if (!inst->dt->remap_a) return TRUE;
  if (ldt->rm_ext_recnum+extstep >= 8L*ldt->remap_size)
    if (remap_newsize(inst->dt, ldt->rm_ext_recnum+extstep+1)) return TRUE;

  if (inst->xcdp->connection_type==CONN_TP_ODBC)
  { newext=newint=0;
    while (newext<extstep)
    { trecnum rec=ldt->rm_ext_recnum+newext;
      int status=odbc_rec_status(ldt, rec);
      if (status==SQL_ROW_SUCCESS)
      { newint++;
        ldt->remap_a[(size_t)(rec / BITSPERPART)] |= 1 << (rec % BITSPERPART);
      }
      else if (status!=SQL_ROW_DELETED) break;
      newext++;
    }
  }
  else   /* direct WB access: */
  { if (ldt->rm_ext_recnum < 8L*ldt->remap_size)
      frontier=ldt->remap_a[(size_t)(ldt->rm_ext_recnum / BITSPERPART)];  /* part containing 1st unused bit */
    else frontier=0;   /* otherwise may overflow the segment boundary */
    if (extstep < 512) extstep=512;
    if (remap_sem)
      { return TRUE; }
    remap_sem=wbtrue;
    if (cd_Get_valid_recs(ldt->cdp, ldt->cursnum, ldt->rm_ext_recnum,
          extstep, ldt->remap_a+(size_t)(ldt->rm_ext_recnum / BITSPERPART),
          &newext, &newint))
      { remap_sem=wbfalse;  return TRUE; }
    remap_sem=wbfalse;
    ldt->remap_a[(size_t)(ldt->rm_ext_recnum / BITSPERPART)] |= frontier;
  }
  trecnum orig_fi_recnum = ldt->fi_recnum;
  ldt->rm_ext_recnum+=newext;
  ldt->rm_int_recnum+=newint;
  SET_FI_INT_RECNUM(inst);
  if (newext<extstep) ldt->rm_completed=wbtrue;
  return FALSE;
}

CFNC DllExport void WINAPI remap_reset(view_dyn * inst) /* may be called even if no remap exists */
// Must not change anything for variable-only views, recnum values would not be restored!
{ ltable * ldt = inst->dt;  unsigned rows;
  if (ldt->cursnum==NOOBJECT) return;
  if (inst->hWnd && ldt->remap_a) 
    rows = inst->Remapping() ? ldt->rm_int_recnum : ldt->rm_ext_recnum;
  ldt->rm_int_recnum=ldt->rm_ext_recnum=ldt->rm_curr_int=ldt->rm_curr_ext=0;
  ldt->rm_completed=false;  /* used even if no remap */
  if (ldt->remap_a)
  { SET_FI_INT_RECNUM(inst);
    memset(ldt->remap_a, 0, (size_t)ldt->remap_size);
  }
  else SET_FI_EXT_RECNUM(inst);
}

CFNC DllExport trecnum WINAPI inter2exter(view_dyn * inst, trecnum inter)
/* Returns NORECNUM iff inter record does not exist */
{ uns8 del;  trecnum ext_start, int_start, int_dist, r;  BOOL upwards;
  ltable * ldt = inst->dt;
  if (inter==NORECNUM) return inter;
  if (inst->stat->vwHdFt & FORM_OPERATION) return NORECNUM;
  if (!ldt->remap_a)   /* no remap */
  { if (inter < ldt->rm_ext_recnum) return inter;  /* existing record */
    if ((ldt->cursnum==NOOBJECT) || ldt->rm_completed) return NORECNUM;
    if (inst->xcdp->connection_type==CONN_TP_ODBC)
    { while (inter+100 >= ldt->rm_ext_recnum)  // look for [inter] and the next 100 records
      { int status=odbc_rec_status(ldt, ldt->rm_ext_recnum);
        if (status!=SQL_ROW_SUCCESS && status!=SQL_ROW_DELETED) 
          { ldt->rm_completed=true;  break; }
        ldt->rm_ext_recnum++;  /* increase the number of existing records */
      }
      SET_FI_EXT_RECNUM(inst);
      return inter < ldt->rm_ext_recnum ? inter : NORECNUM;
    }
    else  /* WB cursor */
    { del=OUT_OF_CURSOR;
      Recstate(ldt->cdp, ldt->cursnum, inter, &del);
      if (del==OUT_OF_CURSOR || del && !(inst->vwOptions & VIEW_DEL_RECS))
        return NORECNUM;  /* deleted or empty or outside */
      ldt->rm_ext_recnum=inter+1;  /* increase the number of existing records */
      SET_FI_EXT_RECNUM(inst);
    }
    return inter;
  }
  else  /* there is a remap */
  { while (inter >= ldt->rm_int_recnum)  /* must increase the remap */
    { if (ldt->rm_completed) return NORECNUM; /* record does not exist */
      if (remap_expand(inst, 512)) return NORECNUM;
    }
   /* searching the reference point: */
    ext_start=int_start=0;  int_dist=inter;  upwards=TRUE;
    if (ldt->rm_curr_int)
    { if (ldt->rm_curr_int <= inter)
      { ext_start=ldt->rm_curr_ext;  int_start=ldt->rm_curr_int;
        int_dist=inter-ldt->rm_curr_int;
      }
      else if ((ldt->rm_curr_int > inter) && (ldt->rm_curr_int < 2*inter))
      { ext_start=ldt->rm_curr_ext;  int_start=ldt->rm_curr_int;
        int_dist=ldt->rm_curr_int-inter;
        upwards=FALSE;
      }
    }
    /* inter cannot be >= ldt->rm_int_recnum, tested above */
    if (ldt->rm_int_recnum-inter < int_dist)  /* better approx. found */
    { ext_start=ldt->rm_ext_recnum;  int_start=ldt->rm_int_recnum;
      upwards=FALSE;
    }
   /* counting the non-deleted records from ext_start */
    if (upwards)
    { for (r=ext_start; TRUE; r++)
      { if (r >= 8L*ldt->remap_size) return NORECNUM;
        if (ldt->remap_a[(size_t)(r/BITSPERPART)] & (1 << (r%BITSPERPART)))
          if (int_start++==inter)
          { ldt->rm_curr_ext=r;  ldt->rm_curr_int=inter; /* new "current" */
            return r;
          }
      }
    }
    else
    { for (r=ext_start-1; TRUE; r--)
        if (ldt->remap_a[(size_t)(r/BITSPERPART)] & (1 << (r%BITSPERPART)))
          if (--int_start==inter)
          { ldt->rm_curr_ext=r;  ldt->rm_curr_int=inter; /* new "current" */
            return r;
          }
    }
//    return NORECNUM;  /* this point cannot be reached */
  }
}

CFNC DllExport trecnum WINAPI exter2inter(view_dyn * inst, trecnum exter)
/* Returns NORECNUM iff exter record is deleted or outside existing
   records range */
{ uns8 del;  trecnum ext_start, int_start, ext_dist, r;  BOOL upwards;
  ltable * ldt = inst->dt;
//  if (inst->stat->vwHdFt & FORM_OPERATION) return 0; // ??
  if (exter==NORECNUM) return exter;
  if (!ldt->remap_a)   /* no remap */
  { if (exter < ldt->rm_ext_recnum) return exter;  /* existing record */
    if ((ldt->cursnum==NOOBJECT) || ldt->rm_completed ||
        inst->stat->vwHdFt & FORM_OPERATION) return NORECNUM;
    if (inst->xcdp->connection_type==CONN_TP_ODBC)
    { int status=odbc_rec_status(ldt, exter);
      if (status!=SQL_ROW_SUCCESS && status!=SQL_ROW_DELETED) return NORECNUM;
    }
    else  /* WB cursor */
    { del=OUT_OF_CURSOR;
      Recstate(ldt->cdp, ldt->cursnum, exter, &del);
      if (del==OUT_OF_CURSOR || del && !(inst->vwOptions & VIEW_DEL_RECS))
        return NORECNUM;  /* deleted or empty or outside */
    }
    ldt->rm_ext_recnum=exter+1;  /* increase the number of existing records */
    SET_FI_EXT_RECNUM(inst);
    return exter;
  }
  else  /* there is a remap */
  { if (exter >= ldt->rm_ext_recnum)  /* must increase the remap */
    { if (ldt->rm_completed) return NORECNUM; /* outside */
      remap_expand(inst, exter-ldt->rm_ext_recnum+1);
      if (exter >= ldt->rm_ext_recnum) return NORECNUM;  /* outside */
    }
   /* checking the status of the exter record */
    if (!(ldt->remap_a[(size_t)(exter/BITSPERPART)] & (1 << (exter%BITSPERPART))))
      return NORECNUM;  /* deleted or empty record */
   /* searching the reference point: */
    ext_start=int_start=0;  ext_dist=exter;  upwards=TRUE;
    if (ldt->rm_curr_ext)
    { if (ldt->rm_curr_ext <= exter)
      { ext_start=ldt->rm_curr_ext;  int_start=ldt->rm_curr_int;
        ext_dist=exter-ldt->rm_curr_ext;
      }
      else if ((ldt->rm_curr_ext > exter) && (ldt->rm_curr_ext < 2*exter))
      { ext_start=ldt->rm_curr_ext;  int_start=ldt->rm_curr_int;
        ext_dist=ldt->rm_curr_ext-exter;
        upwards=FALSE;
      }
    }
    /* exter cannot be >= ldt->rm_ext_recnum, tested above */
    if (ldt->rm_ext_recnum-exter < ext_dist)  /* better approx. found */
    { ext_start=ldt->rm_ext_recnum;  int_start=ldt->rm_int_recnum;
      upwards=FALSE;
    }
   /* counting the non-deleted records between ext_start & exter */
    if (upwards)
    { for (r=ext_start; r<exter; r++)  /* TRUE on exter, but 1 must be subtracted */
        if (ldt->remap_a[(size_t)(r/BITSPERPART)] & (1 << (r%BITSPERPART)))
          int_start++;
    }
    else
    { for (r=exter; r<ext_start; r++)
        if (ldt->remap_a[(size_t)(r/BITSPERPART)] & (1 << (r%BITSPERPART)))
          int_start--;
    }
    ldt->rm_curr_ext=exter;  ldt->rm_curr_int=int_start;  /* new "current" */
    return int_start;
  }
}

CFNC DllKernel BOOL remap_delete(view_dyn * inst, trecnum inter)
/* Updates remap structures after deleting "inter" record */
{ trecnum exter;
  ltable * ldt = inst->dt;
  if (ldt->remap_a)
  { exter=inter2exter(inst, inter);
    if (exter==NORECNUM) return FALSE;  /* record does not exist */
         ldt->remap_a[(size_t)(exter/BITSPERPART)] ^= 1 << (exter%BITSPERPART);
   /* updating remap info */
    ldt->rm_int_recnum--;
    SET_FI_INT_RECNUM(inst);
    if (ldt->rm_curr_ext > exter)
      ldt->rm_curr_int--;
    else if (ldt->rm_curr_ext == exter)
      if (ldt->rm_curr_int)   /* then rm_curr_ext > 0 */
      { ldt->rm_curr_int--;  ldt->rm_curr_ext--;
        while (!(ldt->remap_a[(size_t)(ldt->rm_curr_ext / BITSPERPART)] &
                                 (1 << (ldt->rm_curr_ext % BITSPERPART))))
          if (ldt->rm_curr_ext) ldt->rm_curr_ext--;
          else break;
      }
      else ldt->rm_curr_ext=0;
  }
  else SET_FI_EXT_RECNUM(inst);
  return TRUE;
}

CFNC DllKernel void remap_insert(view_dyn * inst, trecnum exter)
/* Updates remap information AFTER the "exter" record has been inserted. */
{ ltable * ldt = inst->dt;
  trecnum orig_fi_recnum = ldt->fi_recnum;
  if (ldt->remap_a)
  { if (exter >= ldt->rm_ext_recnum)
    {// create the new fictive row when going UP from the edited fictive: 
      trecnum orig_fi_recnum = ldt->fi_recnum;
      remap_expand(inst, exter-ldt->rm_ext_recnum+1);  
      //if (orig_fi_recnum < ldt->fi_recnum)  // add records to the remap
      //  ((DataGrid*)inst->hWnd)->AppendRows(ldt->fi_recnum - orig_fi_recnum);
      return;   
    }
    else
    { ldt->remap_a[(size_t)(exter/BITSPERPART)] |= 1 << (exter%BITSPERPART);
     /* updating remap info */
      if (ldt->rm_curr_ext > exter) ldt->rm_curr_int++;
      ldt->rm_int_recnum++;
    }
    SET_FI_INT_RECNUM(inst);
  }
  else SET_FI_EXT_RECNUM(inst);
 // add the row to the grid:
  //trecnum irec=exter2inter(inst, exter);
  //((DataGrid*)inst->hWnd)->InsertRows(irec, 1);
}


////////////////////////////
CFNC DllExport void WINAPI catr_set_null(tptr pp, int type) /* write the WB NULL representation */
{ switch (type)
  { case ATT_BOOLEAN: case ATT_INT8: 
      *pp=(uns8)0x80;  break;
    case ATT_STRING:  
    case ATT_CHAR:    case ATT_TEXT:
    case ATT_ODBC_DECIMAL:  case ATT_ODBC_NUMERIC:
      *(wuchar*)pp = 0;  // just to be safe for Unicode
      break;
    case ATT_INT16:    
       *(uns16 *)pp=NONESHORT;  break;
    case ATT_INT32:  case ATT_TIME:  case ATT_DATE:
    case ATT_DATIM:  case ATT_TIMESTAMP:
      *(uns32 *)pp=NONEINTEGER;  break;
    case ATT_INT64:  
      *(sig64 *)pp=NONEBIGINT; break;
    case ATT_ODBC_DATE:
      ((DATE_STRUCT     *)pp)->day =32;  break;
    case ATT_ODBC_TIME:
      ((TIME_STRUCT     *)pp)->hour=25;  break;
    case ATT_ODBC_TIMESTAMP:
      ((TIMESTAMP_STRUCT*)pp)->day =32;  break;
    case ATT_MONEY:
      *(uns32*)pp=0;  *(uns16*)(pp+4)=0x8000;  break;
    case ATT_FLOAT:  *(double*)pp=NONEREAL;  break;
    case ATT_PTR:  case ATT_BIPTR:  *(uns32*)pp=(uns32)-1;  break;
    case ATT_AUTOR: memset(pp, 0, UUID_SIZE);  break;
  }
}

CFNC DllExport BOOL WINAPI catr_is_null(tptr vals, atr_info * catr)
{ tptr val=vals+catr->offset;
  if (catr->flags & CA_INDIRECT)
    return ((indir_obj*)val)->actsize == 0;
  switch (catr->type)
  { 
    case ATT_BOOLEAN: case ATT_INT8:
                      return *(uns8 *)val==0x80;
    case ATT_INT16:   return *(uns16*)val==0x8000;
    case ATT_MONEY:   return !*(uns16*)val && (*(sig32*)(val+2)==NONEINTEGER);
    case ATT_INT32:  case ATT_DATE:  case ATT_TIME:
    case ATT_DATIM:  case ATT_TIMESTAMP:
                      return *(sig32 *)val==NONEINTEGER;
    case ATT_INT64:  
                      return *(sig64 *)val==NONEBIGINT; 
    case ATT_PTR:    case ATT_BIPTR:
                      return *(sig32*)val==NORECNUM;
    case ATT_FLOAT:   return *(double*)val==NONEREAL;
    case ATT_ODBC_NUMERIC:  case ATT_ODBC_DECIMAL:
    case ATT_STRING:        case ATT_TEXT:  case ATT_CHAR:
      return catr->specif.wide_char ? !*(wuchar*)val : !*val;
    case ATT_ODBC_DATE:      return ((DATE_STRUCT     *)val)->day > 31;
    case ATT_ODBC_TIME:      return ((TIME_STRUCT     *)val)->hour > 24;
    case ATT_ODBC_TIMESTAMP: return ((TIMESTAMP_STRUCT*)val)->day > 31;
    default:          return FALSE;  // incluides ATT_BINARY
  }
}

//////////////////////////////// ltable /////////////////////////////////////
ltable::ltable(cdp_t cdpIn, struct t_connection * connIn)
{ memset(this, 0, sizeof(ltable));
  cdp=cdpIn;  conn=connIn;  m_cRef=1;
//  cache_hnd=0;  cache_ptr=odbc_buf=NULL;  attrs=NULL;  conn=NULL;  -- above
  close_cursor=FALSE;
  hStmt=SQL_NULL_HSTMT;       /* used by both ODBC and WB */
  cache_top=edt_rec=NORECNUM;  /* used by both ODBC and WB */
  cursnum=(tcursnum)NOOBJECT;  supercurs=(uns32)NOOBJECT;
  remap_a=NULL;
  recprivs=false;
  colvaldescr=NULL;
  fulltablename=NULL;  
#ifdef CLIENT_ODBC_9
  hInsertStmt=SQL_NULL_HSTMT;
  co_owned_td=NULL;
#endif
}

ltable::~ltable(void)
{
  corefree(select_st);
 // go to supercursor, if exists:
  if (supercurs != NOOBJECT)
#ifdef CLIENT_ODBC_9
    if (conn!=NULL)
    { SQLFreeStmt(hStmt, SQL_CLOSE);
      SQLFreeStmt(hStmt, SQL_DROP);
      hStmt=(HSTMT)(size_t)supercurs;
    }
    else
#endif
    { cd_Close_cursor(cdp, cursnum);
      cursnum=supercurs;
    }
 // close data access (cursor or statement):
  if (conn==NULL)
    { if (close_cursor && cursnum!=NOOBJECT) cd_Close_cursor(cdp, cursnum); }
#ifdef CLIENT_ODBC_9
  else
  { if (hStmt != SQL_NULL_HSTMT)
    { SQLFreeStmt(hStmt, SQL_CLOSE);
      SQLFreeStmt(hStmt, SQL_DROP);
    }
    if (hInsertStmt != SQL_NULL_HSTMT)
    { SQLFreeStmt(hInsertStmt, SQL_CLOSE);
      SQLFreeStmt(hInsertStmt, SQL_DROP);
    }
    if (co_owned_td) corefree(co_owned_td);  // owned, not co-owned!
  }
#endif
 // free remap structures:
  safefree((void**)&remap_a);
 // free the cache contents & the cache buffer:
  if (cache_top!=NORECNUM) free(0, cache_reccnt);
  corefree(attrs);  // used by free() above!
  corefree(odbc_buf);
  if (cache_hnd)  { GlobalUnlock(cache_hnd);  GlobalFree(cache_hnd); }
  corefree(colvaldescr);
  corefree(fulltablename);
}

void ltable::free( unsigned offset, unsigned count)
{ unsigned i;  int j;  atr_info * catr;  tptr p;
  if (cache_top==NORECNUM) return;  // cache is empty, fuse
  for (p= cache_ptr+(LONG)offset*rec_cache_size;  count--;
       p+=rec_cache_size)
  { for (i=0, catr=attrs;  i<attrcnt;  i++, catr++)
    { if (catr->flags & CA_MULTIATTR)
      { t_dynar * dy = (t_dynar*)(p+catr->offset);
        if (catr->flags & CA_INDIRECT)
          for (j=0;  j<dy->count();  j++)
          { indir_obj * iobj = (indir_obj*)dy->acc(j);
            if (iobj->obj)
            { GlobalUnlock(GlobalPtrHandle(iobj->obj));
              GlobalFree  (GlobalPtrHandle(iobj->obj));
            }
          }
        dy->t_dynar::~t_dynar();
      }
      else if (catr->flags & CA_INDIRECT)
      { indir_obj * iobj = (indir_obj*)(p+catr->offset);
        if (iobj->obj)
        { GlobalUnlock(GlobalPtrHandle(iobj->obj));
          GlobalFree  (GlobalPtrHandle(iobj->obj));
        }
        iobj->obj=NULL;
      }
    }
    memset(p+1, 0, rec_cache_real-1);  // fuse against double cache_free
    // used by data import
  }
}

#define MAX_PACK_COUNT (MAX_PACKAGED_REQS-1)
#define MAX_PACK_SIZE  40000L
#define MAX_REC_COUNT  80  // maximum "count" value in fast_cache_load call.

void ltable::load(void * inst, unsigned offset, unsigned count, trecnum new_top)
{
    while (count) // count is big when loading _multiling
    { 
        unsigned step = count < MAX_REC_COUNT ? count : MAX_REC_COUNT;
        loadstep(inst, offset, step, new_top);
        offset+=step;  count-=step;
    }
}

void ltable::loadstep(void * inst, unsigned offset, unsigned count, trecnum new_top)
/* Loads cached data using request packages. Load is performed in 3 phases,
   because # of multiattribute items must be loaded before their values and
   legth before the value of var-len attribute. If sending the package returns
   an error, the same requests are sent individually. */
{ trecnum extrec[MAX_REC_COUNT];  int status;  atr_info * catr;
  unsigned rec, atr, index;  int last_rec;

 /* determine record status, initialize fictive and non-existing records: */
  tptr p = (tptr)(cache_ptr+(LONG)offset * rec_cache_size);
  status = /*inst && (inst->stat->vwHdFt & FORM_OPERATION) ? REC_FICTIVE : */REC_EXISTS;
  last_rec=-1;
  for (rec=0;  rec<count;  rec++, p+=rec_cache_size)
  { memset(p, 0, rec_cache_real);
    if (status == REC_EXISTS)  /* status of the previous record */
    {
      if (inst!=NULL)
      { extrec[rec]=inter2exter((view_dyn*)inst, new_top+offset+rec);
        if (extrec[rec]==NORECNUM)
          status=/*((view_dyn *)inst)->stat->vwHdFt & NO_VIEW_FIC ? REC_NONEX :*/ REC_FICTIVE;  // this may not be OK
      }
      else
        extrec[rec]=new_top+offset+rec;
    }
    else status=REC_NONEX;
    *p=(char)status;
    if (status==REC_EXISTS)
    { last_rec=rec;  // must be before detecting the released record!
      if (!IS_CURSOR_NUM(cursnum) && !remap_a) // table with deleted records
      { uns8 dltd;
        cd_Read(cdp, cursnum, extrec[rec], DEL_ATTR_NUM, NULL, &dltd);
        if (dltd==RECORD_EMPTY) *p=REC_RELEASED;  // temporary mark, not status!
      }
    }
    if (*p==REC_EXISTS) // not status!
    { if (recprivs)
        cd_GetSet_privils(cdp, NOOBJECT, CATEG_USER, cursnum, extrec[rec], OPER_GETEFF, (uns8*)p+rec_priv_off);
    }
    else
    { for (atr=1, catr=attrs+1;  atr<attrcnt;  atr++, catr++)
        if (catr->flags & CA_MULTIATTR)
        { t_dynar * dy = (t_dynar*)(p+catr->offset);
          dy->init(catr->flags & CA_INDIRECT ? sizeof(indir_obj) : catr->size);
        }
        else if (!(catr->flags & CA_INDIRECT))
          catr_set_null(p+catr->offset, catr->type);
      if (status==REC_FICTIVE && recprivs)
        memset(p+rec_priv_off, 0xff, rec_priv_size);
    }
  }
  if (last_rec<0) return;
  LONG pack_len, exp_size;
  unsigned pack_count, cp_atr, cp_rec, cp_index;

 /* PHASE 1: load independent info: */
  rec=0;  atr=0;
  while (rec<=last_rec)
  { cp_rec=rec;  cp_atr=atr;
    cd_start_package(cdp);  pack_len=0;  pack_count=0;
    while (pack_count<MAX_PACK_COUNT && rec<=last_rec)
    { catr=attrs+atr;
      p = (tptr)(cache_ptr + (LONG)(offset+rec) * rec_cache_size);
      if (*p==REC_EXISTS)
      { if (!atr) exp_size=rr_datasize-1;
        else if (privils_ok(atr, p, FALSE))
          if (catr->flags & CA_MULTIATTR) exp_size=2; /* must be before CA_INDIRECT test */
          else if (catr->flags & CA_INDIRECT)  exp_size=4;
        if (pack_len+exp_size > MAX_PACK_SIZE && pack_count)
          break;   /* max package length reached */
       /* add request to the package: */
        if (!atr)
        { cd_Read_record(cdp, cursnum, extrec[rec], p+1, rr_datasize-1);
          pack_count++;  pack_len+=exp_size;
        }
        else if (privils_ok(atr, p, FALSE))
          if (catr->flags & CA_MULTIATTR)  /* must be before CA_INDIRECT test */
          { cd_Read_ind_cnt(cdp, cursnum, extrec[rec], atr, (uns16*)(p+catr->offset));
            pack_count++;  pack_len+=exp_size;
          }
          else if (catr->flags & CA_INDIRECT)
          { cd_Read_len(cdp, cursnum, extrec[rec], atr, NOINDEX, (uns32*)(p+catr->offset));
            pack_count++;  pack_len+=exp_size;
          }
      }
     /* to the next item: */
      if (++atr >= attrcnt) 
      { rec++;  atr=0; 
        if (!IS_CURSOR_NUM(cursnum) && !remap_a) break; // no batch across record boudaries
      }
    }
    cd_send_package(cdp);
    if (cd_Sz_error(cdp))   /* error in the package, must repost requests */
    { rec=cp_rec;  atr=cp_atr;
      while (rec<=last_rec)
      { catr=attrs+atr;
        p = (tptr)(cache_ptr+(LONG)(offset+rec) * rec_cache_size);
        if (*p==REC_EXISTS)
        { BOOL res=FALSE;
          if (!atr) res=cd_Read_record(cdp, cursnum, extrec[rec], p+1, rr_datasize-1);
          else if (privils_ok(atr, p, FALSE))
            if (catr->flags & CA_MULTIATTR)  /* must be before CA_INDIRECT test */
              res=cd_Read_ind_cnt(cdp, cursnum, extrec[rec], atr, (uns16*)(p+catr->offset));
            else if (catr->flags & CA_INDIRECT)
              res=cd_Read_len(cdp, cursnum, extrec[rec], atr, NOINDEX, (uns32*)(p+catr->offset));
          if (res) *p=(unsigned char)0xff;
        }
       /* to the next item: */
        if (++atr >= attrcnt) { rec++;  atr=0; }
      }
    }
  }

 /* Interphase 1-2: allocate indir objs and dynars or clear info from prev. phase: */
    p = (tptr)(cache_ptr + (LONG)offset * rec_cache_size);
    for (rec=0;  rec<=last_rec;  rec++, p+=rec_cache_size)
    if (*(uns8*)p==0xff)
      memset(p+1, 0, rec_cache_real-1);
    else for (atr=1, catr=attrs+1;  atr<attrcnt;  atr++, catr++)
      if (catr->flags & CA_MULTIATTR)
      { t_dynar * dy = (t_dynar*)(p+catr->offset);
        int count=*(uns16*)(p+catr->offset);
        dy->init(catr->flags & CA_INDIRECT ? sizeof(indir_obj) : catr->size);
        if (count)
          if (dy->acc(count-1)==NULL) *p=(unsigned char)0xff;
      }
      else if (catr->flags & CA_INDIRECT)
      { indir_obj * iobj = (indir_obj*)(p+catr->offset);
        uns32 len = *(uns32*)(p+catr->offset);  *(uns32*)(p+catr->offset)=0;
        HGLOBAL hnd = GlobalAlloc(GMEM_MOVEABLE, HPCNTRSZ+len+1);
        if (!hnd) { *p=(unsigned char)0xff;  iobj->obj=NULL;  iobj->actsize=0; }
        else
        { iobj->obj=(cached_longobj*)GlobalLock(hnd);
          iobj->actsize=iobj->obj->maxsize=len;
        }
      }

 /* PHASE 2: load fixed-len multiattribute values and non-multi var-len data: */
  rec=0;  atr=1;  index=0;  t_dynar * dy;
  while (rec<=last_rec)
  { cp_rec=rec;  cp_atr=atr;  cp_index=index;
    cd_start_package(cdp);  pack_len=0;  pack_count=0;  exp_size=0;
    while (pack_count<MAX_PACK_COUNT && rec<=last_rec)
    { catr=attrs+atr;
      p = (tptr)(cache_ptr+(LONG)(offset+rec) * rec_cache_size);
      if (*p==REC_EXISTS)
      { if (catr->flags & CA_MULTIATTR) dy = (t_dynar*)(p+catr->offset);
        if (*(uns8*)p!=0xff && privils_ok(atr, p, FALSE))
          if (catr->flags & CA_MULTIATTR)
            if (catr->flags & CA_INDIRECT) exp_size=4;
            else exp_size=catr->size;
          else if (catr->flags & CA_INDIRECT)  exp_size=*(uns32*)(p+catr->offset);
        if (pack_len+exp_size > MAX_PACK_SIZE && pack_count || exp_size > MAX_PACK_SIZE)
          break;   /* max package length reached */
       /* add request to the package: */
        pack_len+=exp_size;
        if (*(uns8*)p!=0xff && privils_ok(atr, p, FALSE))
          if (catr->flags & CA_MULTIATTR)  /* must be before CA_INDIRECT test */
          { if (index >= dy->count()) goto ph2_nx;
            if (catr->flags & CA_INDIRECT)
              cd_Read_len(cdp, cursnum, extrec[rec], atr, index, (uns32*)dy->acc(index));
            else
              cd_Read_ind(cdp, cursnum, extrec[rec], atr, index, (tptr)dy->acc(index));
            pack_count++;
          }
          else if (catr->flags & CA_INDIRECT)
          { indir_obj * iobj = (indir_obj*)(p+catr->offset);
            cd_Read_var(cdp, cursnum, extrec[rec], atr, NOINDEX, 0, iobj->obj->maxsize, iobj->obj->data, NULL);
            iobj->obj->data[iobj->obj->maxsize]=0;
            pack_count+=2;  /* additional item, Read_var must not divide this! */
          }
      }
      else { rec++;  atr=1;  index=0;  continue; }
     /* to the next item: */
     ph2_nx:
      if (!(catr->flags & CA_MULTIATTR) || ++index >= dy->count())
      { index=0;
        if (++atr >= attrcnt) { atr=1;  rec++; }
      }
    }
    cd_send_package(cdp);
    if (cd_Sz_error(cdp) ||  /* error in the package, must repost requests */
        exp_size && !pack_count) /* breaked on long var-len attribute */
    { rec=cp_rec;  atr=cp_atr;  index=cp_index;
      while (rec<=last_rec)
      { catr=attrs+atr;
        p = (tptr)(cache_ptr + (LONG)(offset+rec) * rec_cache_size);
        if (*p==REC_EXISTS)
        { BOOL res=FALSE;
          if (catr->flags & CA_MULTIATTR) dy = (t_dynar*)(p+catr->offset);
          if (*(uns8*)p!=0xff && privils_ok(atr, p, FALSE))
            if (catr->flags & CA_MULTIATTR)  /* must be before CA_INDIRECT test */
            { if (index >= dy->count()) goto ph2_nx2;
              if (catr->flags & CA_INDIRECT)
                res=cd_Read_len(cdp, cursnum, extrec[rec], atr, index, (uns32*)dy->acc(index));
              else
                res=cd_Read_ind(cdp, cursnum, extrec[rec], atr, index, (tptr)dy->acc(index));
            }
            else if (catr->flags & CA_INDIRECT)
            { indir_obj * iobj = (indir_obj*)(p+catr->offset);
              iobj->actsize=0;  // may have 64 bits
              res=cd_Read_var(cdp, cursnum, extrec[rec], atr, NOINDEX, 0, iobj->obj->maxsize, iobj->obj->data, (t_varcol_size*)&iobj->actsize);
              if (!res) iobj->obj->data[iobj->actsize]=0;
            }
          if (res) *p=(unsigned char)0xff;
        }
       /* to the next item: */
       ph2_nx2:
        if (!(catr->flags & CA_MULTIATTR) || ++index >= dy->count())
        { index=0;
          if (++atr >= attrcnt) { atr=1;  rec++; }
        }
      }
    }
  }

 /* Interphase 2-3: indir objs for muliattributes or clear info from prev. phase: */
  p = (tptr)(cache_ptr + (LONG)offset * rec_cache_size);
  for (rec=0;  rec<=last_rec;  rec++, p+=rec_cache_size)
    for (atr=1, catr=attrs+1;  atr<attrcnt;  atr++, catr++)
      if (IS_CA_MULTVAR(catr->flags))
      { dy = (t_dynar*)(p+catr->offset);
        for (index=0;  index < dy->count();  index++)
        { indir_obj * iobj = (indir_obj*)dy->acc(index);
          uns32 len = *(uns32*)dy->acc(index);  *(uns32*)dy->acc(index)=0;
          HGLOBAL hnd = GlobalAlloc(GMEM_MOVEABLE, HPCNTRSZ+len+1);
          if (!hnd) *p=(unsigned char)0xff;
          else
          { iobj->obj=(cached_longobj*)GlobalLock(hnd);
            iobj->actsize=iobj->obj->maxsize=len;
          }
        }
      }

 /* PHASE 3: load var-len multiattribute values: */
  rec=0;  atr=1;  index=0;
  while (rec<=last_rec)
  { cp_rec=rec;  cp_atr=atr;  cp_index=index;
    cd_start_package(cdp);  pack_len=0;  pack_count=0;  exp_size=0;
    while (pack_count<MAX_PACK_COUNT && rec<=last_rec)
    { catr=attrs+atr;
      p = (tptr)(cache_ptr+(LONG)(offset+rec) * rec_cache_size);
      if (*p==REC_EXISTS)
      { if (catr->flags & CA_MULTIATTR) dy = (t_dynar*)(p+catr->offset);
        if (*(uns8*)p!=0xff && privils_ok(atr, p, FALSE))
          if (IS_CA_MULTVAR(catr->flags))
          { if (index >= dy->count()) goto ph3_nx;
            indir_obj * iobj = (indir_obj*)dy->acc(index);
            exp_size=iobj->obj->maxsize;
          }
        if (pack_len+exp_size > MAX_PACK_SIZE && pack_count || exp_size > MAX_PACK_SIZE)
          break;   /* max package length reached */
       /* add request to the package: */
        pack_len+=exp_size;
        if (*(uns8*)p!=0xff && privils_ok(atr, p, FALSE))
          if (IS_CA_MULTVAR(catr->flags))
          { dy = (t_dynar*)(p+catr->offset);
            indir_obj * iobj = (indir_obj*)dy->acc(index);
            cd_Read_var(cdp, cursnum, extrec[rec], atr, index, 0, iobj->obj->maxsize, iobj->obj->data, NULL);
            iobj->obj->data[iobj->obj->maxsize]=0;
            pack_count+=2;  /* additional item */
          }
      }
      else { rec++;  atr=1;  index=0;  continue; }
       /* to the next item: */
     ph3_nx:
      if (!(catr->flags & CA_MULTIATTR) || ++index >= dy->count())
      { index=0;
        if (++atr >= attrcnt) { atr=1;  rec++; }
      }
    }
    cd_send_package(cdp);
    if (cd_Sz_error(cdp) ||   /* error in the package, must repost requests */
        exp_size && !pack_count) /* breaked on long var-len attribute */
    { rec=cp_rec;  atr=cp_atr;  index=cp_index;
      while (rec<=last_rec)
      { catr=attrs+atr;
        p = (tptr)(cache_ptr + (LONG)(offset+rec) * rec_cache_size);
        if (*p==REC_EXISTS)
        { BOOL res=FALSE;
          if (catr->flags & CA_MULTIATTR) dy = (t_dynar*)(p+catr->offset);
          if (*(uns8*)p!=0xff && privils_ok(atr, p, FALSE))
            if (IS_CA_MULTVAR(catr->flags))
            { if (index >= dy->count()) goto ph3_nx2;
              indir_obj * iobj = (indir_obj*)dy->acc(index);
              iobj->actsize=0;  // may have 64 bits
              res=cd_Read_var(cdp, cursnum, extrec[rec], atr, index, 0, iobj->obj->maxsize, iobj->obj->data, (t_varcol_size*)&iobj->actsize);
              if (!res) iobj->obj->data[iobj->actsize]=0;
            }
          if (res) *p=(unsigned char)0xff;
        }
       /* to the next item: */
       ph3_nx2:
        if (!(catr->flags & CA_MULTIATTR) || ++index >= dy->count())
        { index=0;
          if (++atr >= attrcnt) { atr=1;  rec++; }
        }
      }
    }
  }
 // released records as normal:
  p = (tptr)(cache_ptr + (LONG)offset * rec_cache_size);
  for (rec=0;  rec<=last_rec;  rec++, p+=rec_cache_size)
    if (*p==REC_RELEASED) *p=REC_EXISTS;
}

BOOL ltable::write(tcursnum cursnum, trecnum * erec, tptr cache_rec, BOOL global_save)
{ unsigned colcount = 0;  BOOL res = FALSE;
  // delete or un-delete the column:
  if (*erec!=(trecnum)-2 && *erec!=(trecnum)-1)  // unless Appending or Inserting a record
    if (attrs[0].changed)
    { if (cache_rec[attrs[0].offset])
        res=cd_Delete(cdp, cursnum, *erec);
      else
        res=cd_Undelete(cdp, cursnum, *erec);
      if (res) return TRUE; 
    }
 // prepare description of changed columns:
  int i, j;  atr_info * catr;
  for (i=1, catr=attrs+1;  i<attrcnt;  i++, catr++)
    if (catr->changed || global_save)
      if (!(catr->flags & CA_MULTIATTR))
      { t_column_val_descr * acoldescr = colvaldescr + colcount++;
        acoldescr->column_number=i;
        if (catr->flags & CA_INDIRECT)
        { indir_obj * iobj = (indir_obj*)(cache_rec+catr->offset);
          acoldescr->value_length = (int)iobj->actsize; // store length first: prevent indexing invalid part on the end
          if (iobj->actsize && iobj->obj)
            acoldescr->column_value=iobj->obj->data;
          else acoldescr->column_value=NULL;
        }
        else 
          acoldescr->column_value=cache_rec+catr->offset;
      }
 // store batched values:
  if (*erec==(trecnum)-2)  // Append record
  { if (cd_Append_record_ex(cdp, cursnum, erec, colcount, colvaldescr)) return TRUE;
    if (*erec==NORECNUM) return TRUE;
  }
  else if (*erec==(trecnum)-1)  // Insert record
  { if (cd_Insert_record_ex(cdp, cursnum, erec, colcount, colvaldescr)) return TRUE;
    if (*erec==NORECNUM) return TRUE;
  }
  else
    if (cd_Write_record_ex(cdp, cursnum, *erec, colcount, colvaldescr)) return TRUE;

 // storing multiattributes:
  for (i=1, catr=attrs+1;  i<attrcnt;  i++, catr++)
    if (catr->changed)
      if (catr->flags & CA_MULTIATTR)
      { t_dynar * dy = (t_dynar*)(cache_rec+catr->offset);
        res=cd_Write_ind_cnt(cdp, cursnum, *erec, i, 0); // bipointers use this to be before Write_ind
        if (dy->count())
        { tptr pp=(tptr)dy->acc(0);
          for (j=0;  j<dy->count() && !res;  j++)
          { if (catr->flags & CA_INDIRECT)
            { res=cd_Write_len(cdp, cursnum, *erec, i, j, (t_varcol_size)((indir_obj*)pp)->actsize);  // store length first: prevent indexing invalid part on the end
              if (((indir_obj*)pp)->actsize)
                res=cd_Write_var(cdp, cursnum, *erec, i, j, 0, (t_varcol_size)((indir_obj*)pp)->actsize, ((indir_obj*)pp)->obj->data);
              pp+=sizeof(indir_obj);
            }
            else
            { unsigned size = is_string(catr->type) ? catr->size-1 : catr->size;
              res=cd_Write_ind(cdp, cursnum, *erec, i, j, pp, size);
              pp+=catr->size;
            }
          }
          res|=cd_Write_ind_cnt(cdp, cursnum, *erec, i, dy->count());
        }
      }
  return res;
}

#ifdef CLIENT_ODBC_9

CFNC DllExport void WINAPI cpy_full_attr_name(char quote_char, tptr dest, const d_table * td, unsigned num)
/* num > 0 befause DELETED attribute cannot be used. */
{ const char * src = (const char *)(td->attribs+td->attrcnt);
  while (--num) src+=strlen(src)+1;
  if (quote_char!=' ')
  { dest[0]=quote_char;  strcpy(dest+1, src);
    size_t i=strlen(dest);
    dest[i]=quote_char;  dest[i+1]=0;
  }
  else strcpy(dest, src);
}

SWORD catr_to_c_type(atr_info * catr);

#define MAX_COMMAND_LEN  5000      

SQLLEN nullvalue = SQL_NULL_DATA;  // used for input parametres only, constant

DllExport BOOL WINAPI odbc_synchronize_view9(BOOL update, t_connection * conn,
  ltable * dt, trecnum extrec, tptr new_vals, tptr old_vals, BOOL on_current)
/* ODBC cursor is positioned on the updated record, current values are in odbc_buf. */
{ unsigned i;  atr_info * catr;  unsigned parnum;  RETCODE retcode;
  BOOL firstattr;
 /* write the pcbValues (important when a NULL value in cursor library cache should be replaced by non-NULL */
  SQLLEN * pcbValue = (SQLLEN*)(new_vals+dt->rr_datasize);
  for (i=1, catr=dt->attrs+1;  i<dt->attrcnt;  i++, catr++)
    if (!(catr->flags & (CA_INDIRECT | CA_MULTIATTR)))
    { if (catr_is_null(new_vals, catr)) *pcbValue=SQL_NULL_DATA;
      else if (is_string(catr->type) || catr->type==ATT_TEXT || catr->type==ATT_ODBC_NUMERIC || catr->type==ATT_ODBC_DECIMAL)
        *pcbValue=(SQLLEN)(catr->specif.wide_char ? sizeof(wuchar)*wuclen((wuchar*)(new_vals+catr->offset)) : strlen(new_vals+catr->offset));
      else if (catr->type==ATT_BINARY) *pcbValue=catr->size;
      else *pcbValue=WB_type_length(catr->type);
      pcbValue++;
    }

  if (!update && dt->hInsertStmt!=SQL_NULL_HSTMT)
  {// bind the indirect values (cannot be bound before, because the buffer is allocated dynamically and its address changes)
    parnum=1;
    for (i=1, catr=dt->attrs+1;  i<dt->attrcnt;  i++, catr++)
    { if (catr->flags & CA_INDIRECT)
      { indir_obj * iobj = (indir_obj*)(new_vals+catr->offset);
        if (iobj->obj==NULL)
        { if (SQLBindParameter(dt->hInsertStmt, (UWORD)parnum, SQL_PARAM_INPUT, catr_to_c_type(catr),  catr->odbc_sqltype,
            0, 0, NULLSTRING, 0, &nullvalue) == SQL_ERROR)
              odbc_error(dt->conn, dt->hInsertStmt);
        }
        else
        { if (SQLBindParameter(dt->hInsertStmt, (UWORD)parnum, SQL_PARAM_INPUT, catr_to_c_type(catr),  catr->odbc_sqltype,
            iobj->actsize, 0, iobj->obj->data, 0, &iobj->actsize) == SQL_ERROR)
              odbc_error(dt->conn, dt->hInsertStmt);
        }
      }
      parnum++;
    }
   // execute the prepated insert statement:
    retcode = SQLExecute(dt->hInsertStmt);
    if (retcode!=SQL_SUCCESS) odbc_error(dt->conn, dt->hInsertStmt);
    return !SUCCEEDED(retcode);
  }

 /* create the command: */
  char quote_char;
  if (conn!=NULL) quote_char=conn->identifier_quote_char;
  else quote_char=' ';
  tptr comm=(tptr)corealloc(MAX_COMMAND_LEN, 88);
  if (comm==NULL) return TRUE; 
  sprintf(comm, update ? "UPDATE %s SET " : "INSERT INTO %s(", dt->fulltablename);
  firstattr=TRUE;
  for (i=1, catr=dt->attrs+1;  i<dt->attrcnt;  i++, catr++)
    if (catr->changed)
    { if (firstattr) firstattr=FALSE; else strcat(comm, ",");
      cpy_full_attr_name(quote_char, comm+strlen(comm), dt->co_owned_td, i);
      if (update)
        strcat(comm, catr_is_null(new_vals, catr) ? "=NULL" : "=?");
    }
  firstattr=TRUE;
  if (update)
  { strcat(comm, " WHERE ");
    if (on_current)
    { strcat(comm, "CURRENT OF ");
      SQLGetCursorName(dt->hStmt, (UCHAR*)comm+strlen(comm), 40, NULL);
    }
    else
    { for (i=1, catr=dt->attrs+1;  i<dt->attrcnt;  i++, catr++)
        if (catr->flags & CA_ROWID)
        { if (firstattr) firstattr=FALSE; else strcat(comm, " AND ");
          cpy_full_attr_name(quote_char, comm+strlen(comm), dt->co_owned_td, i);
          strcat(comm, catr_is_null(old_vals, catr) ? " IS NULL" : "=?");
        }
      if (firstattr)
      { //error_box(_("Cannot edit this table"));
        corefree(comm);
        return TRUE;
      }
    }
  }
  else  /* INSERT */
  { on_current=FALSE;
    strcat(comm, ")VALUES(");
    firstattr=TRUE;
    for (i=1, catr=dt->attrs+1;  i<dt->attrcnt;  i++, catr++)
      if (catr->changed)
      { if (firstattr) firstattr=FALSE; else strcat(comm, ",");
        strcat(comm, catr_is_null(new_vals, catr) ? "NULL" : "?");
      }
    strcat(comm, ")");
  }
 /* allow updating the contents of the cursor library cache: (!) */
  if (update && on_current)
    memcpy(dt->odbc_buf, new_vals, dt->rec_cache_real);
#ifdef STOP
 /* position: */
  if (on_current)
  { odbc_rec_status(cdp, dt, extrec); /* ExtFetch */
    retcode=SQLSetPos(dt->hStmt, 1, SQL_POSITION, SQL_LOCK_NO_CHANGE);
    if (retcode!=SQL_SUCCESS && retcode!=SQL_SUCCESS_WITH_INFO)
      { odbc_stmt_error(dt->hStmt);  return TRUE; }
  }
#endif
 /* prepare: */
  retcode=SQLPrepare(conn->hStmt, (UCHAR*)comm, SQL_NTS);
  corefree(comm);
  if (retcode!=SQL_SUCCESS && retcode!=SQL_SUCCESS_WITH_INFO)
    { odbc_error(conn, conn->hStmt);  return TRUE; }
 /* bind the new values and the identifying values: */
  parnum=1;  //SDWORD pcb = SQL_NTS;
  pcbValue=(SQLLEN*)(new_vals+dt->rr_datasize);
  for (i=1, catr=dt->attrs+1;  i<dt->attrcnt;  i++, catr++)
  { if (catr->changed && !catr_is_null(new_vals, catr))
      if (catr->flags & CA_INDIRECT)
      { indir_obj * iobj = (indir_obj*)(new_vals+catr->offset);
        if (SQLBindParameter(conn->hStmt, (UWORD)parnum++, SQL_PARAM_INPUT,
          catr_to_c_type(catr),  catr->odbc_sqltype,
          iobj->actsize, 0, iobj->obj->data, 0, &iobj->actsize) == SQL_ERROR)
            odbc_error(conn, conn->hStmt);
      }
      else
        if (SQLBindParameter(conn->hStmt, (UWORD)parnum++, SQL_PARAM_INPUT,
          catr_to_c_type(catr),  catr->odbc_sqltype,
          catr->odbc_precision, catr->odbc_scale,
          new_vals+catr->offset, 0, pcbValue) == SQL_ERROR)
           odbc_error(conn, conn->hStmt);
    if (!(catr->flags & (CA_INDIRECT | CA_MULTIATTR))) pcbValue++;
  }
  if (update && !on_current)
  { pcbValue=(SQLLEN*)(new_vals+dt->rr_datasize);
    for (i=1, catr=dt->attrs+1;  i<dt->attrcnt;  i++, catr++)
    { if (catr->flags & CA_ROWID && !catr_is_null(old_vals, catr))
      { if (SQLBindParameter(conn->hStmt, (UWORD)parnum++, SQL_PARAM_INPUT,
          catr_to_c_type(catr),  catr->odbc_sqltype,
          catr->odbc_precision, catr->odbc_scale,
          old_vals+catr->offset, 0, pcbValue) == SQL_ERROR)
            odbc_error(conn, conn->hStmt);
      }
      if (!(catr->flags & (CA_INDIRECT | CA_MULTIATTR))) pcbValue++;
    }
  }
 /* execute: */
  if (conn->can_transact)
    retcode=SQLSetConnectOption(conn->hDbc, SQL_AUTOCOMMIT, SQL_AUTOCOMMIT_OFF);
  retcode=SQLExecute(conn->hStmt);
  if (retcode==SQL_SUCCESS || retcode==SQL_SUCCESS_WITH_INFO)
  { if (retcode==SQL_SUCCESS_WITH_INFO)  /* reports "no rows updated" */
      odbc_error(conn, conn->hStmt);
    SQLLEN rowcnt=0;
    SQLRowCount(conn->hStmt, &rowcnt);
    if (conn->can_transact)
      SQLTransact(SQL_NULL_HENV, conn->hDbc, rowcnt==1 ? SQL_COMMIT : SQL_ROLLBACK);
  }
  else odbc_error(conn, conn->hStmt);
  // The error must be reported later, via special dialog box.
  return retcode!=SQL_SUCCESS && retcode!=SQL_SUCCESS_WITH_INFO;
}

CFNC DllExport void WINAPI prepare_insert_statement(ltable * dt, const d_table * dest_odbc_td)
{ t_flstr cmd;  int i;  atr_info * catr;  SQLLEN * pcbValue;
 /* create the command: */
  cmd.put("INSERT INTO ");
  cmd.put(dt->fulltablename);
  cmd.putc('(');
  bool firstcol = true;
  for (i=1, catr=dt->attrs+1;  i<dt->attrcnt;  i++, catr++)
  { if (firstcol) firstcol=false; else cmd.putc(',');
    const char * src = (const char *)(dest_odbc_td->attribs+dest_odbc_td->attrcnt);
    int num = i;
    while (--num) src+=strlen(src)+1;
    if (dt->conn->identifier_quote_char!=' ') cmd.putc(dt->conn->identifier_quote_char);
    cmd.put(src);
    if (dt->conn->identifier_quote_char!=' ') cmd.putc(dt->conn->identifier_quote_char);
  }
  firstcol=true;
  cmd.put(")VALUES(");
  for (i=1, catr=dt->attrs+1;  i<dt->attrcnt;  i++, catr++)
  { if (firstcol) firstcol=false; else cmd.putc(',');
    cmd.putc('?');
  }
  cmd.putc(')');
  if (!cmd.error())
  { HSTMT hStmt;
    RETCODE retcode = create_odbc_statement(dt->conn, &hStmt);
    if (SUCCEEDED(retcode))
    { retcode=SQLPrepare(hStmt, (UCHAR*)cmd.get(), SQL_NTS);
      if (SUCCEEDED(retcode))
      {// bind the cache values (except for the indirect values):
        int parnum=1;  //SDWORD pcb = SQL_NTS;
        pcbValue=(SQLLEN*)(dt->cache_ptr+dt->rr_datasize);  
        for (i=1, catr=dt->attrs+1;  i<dt->attrcnt;  i++, catr++)
        { if (!(catr->flags & (CA_INDIRECT | CA_MULTIATTR))) 
          { if (SQLBindParameter(hStmt, (UWORD)parnum, SQL_PARAM_INPUT, catr_to_c_type(catr),  catr->odbc_sqltype,
                  catr->odbc_precision, catr->odbc_scale, dt->cache_ptr+catr->offset, 0, pcbValue) == SQL_ERROR)
              odbc_error(dt->conn, hStmt);
            pcbValue++;
          }
          parnum++;
        }
        dt->hInsertStmt = hStmt;
        return;
      }
      SQLFreeStmt(hStmt, SQL_DROP);
    }
  }
}

#endif

ULONG ltable::Release(void)
{ if (--m_cRef) return m_cRef; 
  destroy_cached_access(this);  // must not use delete this, ltable must be deallocated in the module which allocated it
  return 0; 
}

static SWORD catr_to_sql_type(atr_info * catr)
{ switch (catr->type)
  { case ATT_INT16:   return SQL_SMALLINT;
    case ATT_INT8:    return SQL_TINYINT;
    case ATT_INT32:   return SQL_INTEGER;
    case ATT_INT64:   return SQL_BIGINT;
    case ATT_FLOAT:
      if (catr->odbc_precision<=7) return SQL_REAL;
      if (catr->odbc_scale==1)
        { catr->odbc_scale=0;  return SQL_FLOAT; /* special flag */ }
      return SQL_DOUBLE;
    case ATT_CHAR:  case ATT_STRING:
      return SQL_CHAR;
    case ATT_BINARY:          return SQL_BINARY;
    case ATT_BOOLEAN:         return SQL_BIT;
    case ATT_MONEY:
    case ATT_ODBC_NUMERIC:    return SQL_NUMERIC;
    case ATT_ODBC_DECIMAL:    return SQL_DECIMAL;
    case ATT_ODBC_DATE:       return SQL_DATE;//SQL_TYPE_DATE;
    case ATT_ODBC_TIME:       return SQL_TIME;//SQL_TYPE_TIME;
    case ATT_ODBC_TIMESTAMP:  return SQL_TIMESTAMP;//SQL_TYPE_TIMESTAMP;
    case ATT_TEXT:            return SQL_LONGVARCHAR;
    case ATT_RASTER:  case ATT_NOSPEC:  case ATT_SIGNAT:
                              return SQL_LONGVARBINARY;
  }
  return 0;
}

CFNC DllKernel unsigned WINAPI odbc_tpsize(int type, t_specif specif)  // space size including 1 byte terminator for DEC/NUM
{ switch (type)
  { case ATT_ODBC_DATE:      return sizeof(DATE_STRUCT);
    case ATT_ODBC_TIME:      return sizeof(TIME_STRUCT);
    case ATT_ODBC_TIMESTAMP: return sizeof(TIMESTAMP_STRUCT);
    case ATT_ODBC_DECIMAL:
    case ATT_ODBC_NUMERIC:
      return specif.precision + specif.scale + 2;
  }
  return 0;
}

BOOL ltable::describe(const d_table * td, bool is_odbc)
// Makes td owned by [this] or releases it.
{ BOOL ok = TRUE;  unsigned i;  atr_info * catr;
  if (td->tabdef_flags & TFL_REC_PRIVILS) recprivs=true;
  attrs=(atr_info*)corealloc(sizeof(atr_info)*td->attrcnt, 69);
  if (attrs==NULL) ok=FALSE;
  else
  { attrcnt=td->attrcnt;
   /* set basic attribute flags & compute the size of the simple part: */
    catr=attrs;  catr->flags=0;
    unsigned spec_start=1, simple_attrs=0;
    for (i=1;  i<td->attrcnt;  i++)
    { const d_attr * pdatr=ATTR_N(td, i);
      catr=attrs+i;
      if (pdatr->multi != 1) catr->flags=CA_MULTIATTR;
      else catr->flags=0;
      if (is_odbc)
      { if (IS_HEAP_TYPE(pdatr->type)   || is_string(pdatr->type) &&
            (pdatr->specif.length==(uns16)SQL_NO_TOTAL || pdatr->specif.length > MAX_FIXED_STRING_LENGTH))
        { catr->flags |= CA_INDIRECT;
          catr->flags &= ~CA_ROWID;  /* crash otherwise */
          catr->type = pdatr->type==ATT_STRING ? ATT_TEXT : ATT_NOSPEC;
        }
      }
      else
        if (IS_HEAP_TYPE(pdatr->type)) catr->flags |= CA_INDIRECT;
      if (!catr->flags)  /* compute the simple attribute size */
      { if (is_string(pdatr->type) || is_odbc && pdatr->type==ATT_CHAR) 
          if (pdatr->specif.wide_char) spec_start+=pdatr->specif.length+2;
          else                         spec_start+=pdatr->specif.length+1;
        else if (pdatr->type==ATT_BINARY) spec_start+=pdatr->specif.length;
        else if (IS_ODBC_TYPE(pdatr->type))
          spec_start+=odbc_tpsize(pdatr->type, pdatr->specif);
        else spec_start+=tpsize[pdatr->type];
        simple_attrs++;
      }
    }
    rr_datasize=spec_start;
    if (is_odbc)  /* reserve space for lengths */
      spec_start+=simple_attrs * sizeof(SQLLEN);
   /* set all the other attribute information: */
    catr=attrs;  catr->changed=wbfalse;  catr->type=0;  catr->offset=0;
    unsigned offset=1;   /* simple attributes offset */
    for (i=1;  i<td->attrcnt;  i++)
    { const d_attr * pdatr=ATTR_N(td, i);
      catr=attrs+i;
      catr->changed=wbfalse;
      catr->type=pdatr->type;
      catr->multi=pdatr->multi; 
      catr->specif=pdatr->specif;
     /* size: */
      if (catr->flags & CA_INDIRECT)      catr->size=(unsigned)SQL_NO_TOTAL;
      else if (IS_ODBC_TYPE(pdatr->type)) catr->size=odbc_tpsize(pdatr->type, pdatr->specif);
      else if (pdatr->type==ATT_BINARY)   catr->size=pdatr->specif.length;
      else if (is_string(pdatr->type) || IS_HEAP_TYPE(pdatr->type) || is_odbc && pdatr->type==ATT_CHAR)
        catr->size = pdatr->specif.wide_char ? pdatr->specif.length+2 : pdatr->specif.length+1;
      else catr->size = tpsize[pdatr->type];  // no space for Unicode terminator
     /* offsets in the cache: */
      if (catr->flags & CA_MULTIATTR)
      { catr->offset=spec_start;
        spec_start+=sizeof(t_dynar);
      }
      else if (catr->flags & CA_INDIRECT)
      { catr->offset=spec_start;
        spec_start+=sizeof(indir_obj);
      }
      else /* normal fixed size attribute, may be odbc type */
      { catr->offset=offset;
        offset+=catr->size;
      }
      catr->flags |= (pdatr->a_flags << 4);  /* privileges and ID info */
      if (catr->type==ATT_AUTOR || catr->type==ATT_DATIM)
        catr->flags |= CA_ROWVER;
     /* used when binding parametrs: */
      if (catr->type==ATT_ODBC_NUMERIC || catr->type==ATT_ODBC_DECIMAL)
      { catr->odbc_precision=pdatr->specif.precision;
        catr->odbc_scale    =pdatr->specif.scale;
      }
      else if (is_string(catr->type) || catr->flags & CA_INDIRECT)
      { if (pdatr->specif.wide_char)  // WIDE_ODBC supposed
          catr->odbc_precision=pdatr->specif.length/2;
        else
          catr->odbc_precision=pdatr->specif.length;
        catr->odbc_scale    =0;
      }
      else  /* REAL/DOUBLE need both, TINYINT needs precision */
      { catr->odbc_precision=pdatr->specif.precision;
        catr->odbc_scale    =pdatr->specif.scale;
      }
      catr->odbc_sqltype=catr_to_sql_type(catr); /* uses & updates odbc_scale! */
    }
    rec_priv_off=spec_start;
    if (recprivs) 
    { rec_priv_size=SIZE_OF_PRIVIL_DESCR(td->attrcnt);
      if (rec_priv_size<PRIVIL_DESCR_SIZE) rec_priv_size=PRIVIL_DESCR_SIZE;
      spec_start+=rec_priv_size;
    }
    rec_cache_size=rec_cache_real=spec_start;
  }
  
 /* creating and binding the ODBC buffer (used for comparing values in WB views): */
  odbc_buf=(tptr)corealloc(rec_cache_size+1, 49);
   /* +1 for the terminator of the last CHAR attribute */
  if (odbc_buf==NULL)
    { release_table_d(td);  safefree((void**)&attrs);  return FALSE; }
  if (is_odbc) 
  { bind_cache(cdp, this);
    co_owned_td = td;  // in fact it is OWNED and corefree must be called
  }
  else release_table_d(td);
 // allocating space for write_ex descriptor:
  colvaldescr=(t_column_val_descr*)corealloc(sizeof(t_column_val_descr)*td->attrcnt, 88);
  if (!colvaldescr) ok=FALSE;
  return ok;
}



BOOL ltable::privils_ok(int attr, tptr buf, BOOL write_priv)
{ 
  if (attrs[attr].flags &
       (write_priv ? CA_RI_UPDATE : CA_RI_SELECT)) return TRUE;
  if (!recprivs) return FALSE;
  return write_priv ? HAS_WRITE_PRIVIL((buf+rec_priv_off), attr) :
                      HAS_READ_PRIVIL ((buf+rec_priv_off), attr);
}

//////////////////////////////////// data cache interface ////////////////////////////////////
CFNC DllExport BOOL WINAPI alloc_data_cache(ltable * dt, trecnum reccount)
{ dt->cache_hnd=GlobalAlloc(GMEM_MOVEABLE, (reccount+1)*dt->rec_cache_size);
  if (!dt->cache_hnd) return FALSE;
  dt->cache_ptr = (tptr)GlobalLock(dt->cache_hnd);
  dt->cache_reccnt = reccount;
  memset(dt->cache_ptr, 0, (reccount+1)*dt->rec_cache_size);
  return TRUE;
}

CFNC DllExport ltable * WINAPI create_data_cache(cdp_t cdp, tcursnum curs, trecnum reccount)
// Creates and returns data cache for cursor [curs] and [reccount] records.
{ 
  ltable * dt=create_cached_access9(cdp, NULL, NULL, NULL, NULL, curs);
  if (!dt) return NULL;
  if (dt->cache_hnd)  { GlobalUnlock(dt->cache_hnd);  GlobalFree(dt->cache_hnd); }
  if (!alloc_data_cache(dt, reccount))
    { delete dt;  return NULL; }
  return dt;
}

CFNC DllExport const atr_info * WINAPI access_data_cache(ltable * dt, int column_num, trecnum rec_offset, void * & valptr)
{ const atr_info * catr = &dt->attrs[column_num];
  valptr = dt->cache_ptr + rec_offset*dt->rec_cache_size + catr->offset;
  return catr;
}

CFNC DllExport void WINAPI cache_free(ltable * dt, unsigned offset, unsigned count)
/* Frees the cache contents (indirect values, dynars). */
{ dt->free(offset, count);
}

DllExport void WINAPI cache_load(ltable * dt, view_dyn * inst,
   unsigned offset, unsigned count, trecnum new_top)
/* Loads the cache contents (cache uninitialized on entry). */
{ 
  if (dt->conn==NULL) // WB access
  { dt->load(inst, offset, count, new_top);
  }
#ifdef CLIENT_ODBC_9
  else // ODBC access
  { trecnum extrec;  int status;
    tptr p = (tptr)(dt->cache_ptr+(LONG)offset * dt->rec_cache_size);
    status = /*inst && (inst->stat->vwHdFt & FORM_OPERATION) ? REC_FICTIVE : */REC_EXISTS;
    for (;  count--;  offset++, p+=dt->rec_cache_size)
    {/* determine the new record status: */
      if (status == REC_EXISTS)  /* status of the previous record */
      { if (inst!=NULL)
        { extrec=inter2exter(inst, new_top+offset);
          if (extrec==NORECNUM)
            status=inst->stat->vwHdFt & NO_VIEW_FIC ? REC_NONEX : REC_FICTIVE;
        }
        else extrec=new_top+offset;
      }
      else if (status == REC_FICTIVE) status=REC_NONEX;
     /* load or null the record: */
      if (status == REC_EXISTS)
        odbc_load_record(dt, extrec, p);
      else
      { *p=status;
        memset(p+1, 0, dt->rec_cache_real-1);
        unsigned i;  atr_info * catr;
        for (i=1, catr=dt->attrs+1;  i<dt->attrcnt;  i++, catr++)
          if (catr->flags & CA_MULTIATTR)
          { t_dynar * dy = (t_dynar*)(p+catr->offset);
            dy->init(catr->flags & CA_INDIRECT ? sizeof(indir_obj) : catr->size);
          }
          else if (!(catr->flags & CA_INDIRECT))
            catr_set_null(p+catr->offset, catr->type);
      }
    }
  }
#endif
}

#ifdef STOP
DllExport BOOL WINAPI wb_write_cache(cdp_t cdp, ltable * dt,
           tcursnum cursnum, trecnum * erec, tptr cache_rec, BOOL global_save)
// Writes changes in the cache record to the database, return TRUE on error.
{ int i, j;  atr_info * catr;
 /* append a new record if synchronizing the fictive one: */
  if (*erec==(trecnum)-2)  // Append record
  { *erec=cd_Append(cdp, cursnum);
    if (*erec==NORECNUM) return TRUE;
  }
  if (*erec==(trecnum)-1)  // Insert record
  { *erec=cd_Insert(cdp, cursnum);
    if (*erec==NORECNUM) return TRUE;
  }
 /* write the new values: */
  if (global_save) 
    if (cd_Write_record(cdp, cursnum, *erec, cache_rec+1, dt->rr_datasize-1))
      return TRUE;
  BOOL res=FALSE;  tptr pp;
  for (i=0, catr=dt->attrs;  i<dt->attrcnt;  i++, catr++)
    if (catr->changed)
    { if (catr->flags & CA_MULTIATTR)
      { t_dynar * dy = (t_dynar*)(cache_rec+catr->offset);
        res=cd_Write_ind_cnt(cdp, cursnum, *erec, i, 0); // bipointers use this to be before Write_ind
        if (dy->count())
        { pp=(tptr)dy->acc(0);
          for (j=0;  j<dy->count() && !res;  j++)
          { if (catr->flags & CA_INDIRECT)
            { res=cd_Write_len(cdp, cursnum, *erec, i, j, ((indir_obj*)pp)->actsize);  // store length first: prevent indexing invalid part on the end
              if (((indir_obj*)pp)->actsize)
                res=cd_Write_var(cdp, cursnum, *erec, i, j, 0, ((indir_obj*)pp)->actsize, ((indir_obj*)pp)->obj->data);
              pp+=sizeof(indir_obj);
            }
            else
            { unsigned size = is_string(catr->type) ? catr->size-1 : catr->size;
              res=cd_Write_ind(cdp, cursnum, *erec, i, j, pp, size);
              pp+=catr->size;
            }
          }
          res|=cd_Write_ind_cnt(cdp, cursnum, *erec, i, dy->count());
        }
      }
      else  /* not a multiattribute */
        if (catr->flags & CA_INDIRECT)
        { indir_obj * iobj = (indir_obj*)(cache_rec+catr->offset);
          res=cd_Write_len(cdp, cursnum, *erec, i, NOINDEX, iobj->actsize); // store length first: prevent indexing invalid part on the end
          if (iobj->actsize)
            res=cd_Write_var(cdp, cursnum, *erec, i, NOINDEX, 0, iobj->actsize, iobj->obj->data);
        }
        else if (!global_save)
        { pp=cache_rec+catr->offset;
          unsigned size = is_string(catr->type) ? catr->size-1 : catr->size;
          res=cd_Write(cdp, cursnum, *erec, i, NULL, pp, size);
        }
      if (res) break;
    }
  return res;
}
#else
DllExport BOOL WINAPI wb_write_cache(cdp_t cdp, ltable * dt,
           tcursnum cursnum, trecnum * erec, tptr cache_rec, BOOL global_save)
// Writes changes in the cache record to the database, return TRUE on error.
{ return dt->write(cursnum, erec, cache_rec, global_save);
}
#endif


DllExport BOOL WINAPI User_name_by_id(cdp_t cdp, uns8* user_uuid, char * namebuf)
{ int i;
  for (i=0;  i<UUID_SIZE;  i++) if (user_uuid[i]) break;
  if (i==UUID_SIZE)  // NULL value in author
    strcpy(namebuf, "- Nobody -");
  else
  { tobjnum objnum;
    if (cd_Find_object_by_id(cdp, user_uuid, CATEG_USER, &objnum))
    { strcpy(namebuf, "- Unknown -");
      return FALSE;
    }
    else if (cd_Read(cdp, USER_TABLENUM, (trecnum)objnum, USER_ATR_LOGNAME, NULL, namebuf))
      namebuf[0]=0;
  }
  return TRUE;
}

////////////////////// supporting functions used on UNIX platoform ///////////////////////////
#if 0
CFNC DllPrezen tobjnum WINAPI store_odbc_link(cdp_t cdp, tptr objname, t_odbc_link * olnk, BOOL select, BOOL limited)
{ tobjnum objnum;
  if (limited ? cd_Insert_object_limited(cdp, objname, CATEG_TABLE|IS_LINK, &objnum) :
                cd_Insert_object        (cdp, objname, CATEG_TABLE|IS_LINK, &objnum))
    return NOOBJECT;
  if (cd_Write_var(cdp, TAB_TABLENUM, objnum, OBJ_DEF_ATR, NOINDEX, 0, sizeof(t_odbc_link), olnk))
    return NOOBJECT;
  uns16 flags = CO_FLAG_LINK | CO_FLAG_ODBC;
  cd_Write(cdp, TAB_TABLENUM, objnum, OBJ_FLAGS_ATR, NULL, &flags, sizeof(uns16));
#ifdef LINUX
  return objnum; // returns link object, differs from the Windows version
#else
  return Add_component(cdp, CATEG_TABLE, objname, objnum, select);
#endif
}
#endif

CFNC DllExport void WINAPI write_dest_ds(cdp_t cdp, tptr dsn)
{ 
#ifdef STOP
  apx_header apx;
  memset(&apx, 0, sizeof(apx_header));  apx.version=CURRENT_APX_VERSION;
  cd_Read_var(cdp, OBJ_TABLENUM, (trecnum)cdp->applobj, OBJ_DEF_ATR, NOINDEX,
        0, sizeof(apx_header), &apx, NULL);
  strmaxcpy(apx.dest_dsn, dsn, SQL_MAX_DSN_LENGTH+1);
  if (cd_Write_var(cdp, OBJ_TABLENUM, (trecnum)cdp->applobj, OBJ_DEF_ATR, NOINDEX,
        0, sizeof(apx_header), &apx)) cd_Signalize(cdp);
#endif
}

#define COPY_BUF_SIZE 4096

CFNC DllExport BOOL WINAPI copy_to_file(cdp_t cdp, ttablenum tb, trecnum recnum, tattrib attr,
                     uns16 index, FHANDLE hFile, int recode, uns32 offset)
/* copies var-len attribute & closes the file, returns FALSE on error */
{ tptr buf;  uns32 size32;  BOOL err=FALSE;  DWORD wr;
  if (!(buf=(char*)corealloc(COPY_BUF_SIZE, 99))) return FALSE;
  do
  { if (cd_Read_var(cdp, tb, recnum, attr, index, offset, COPY_BUF_SIZE, buf, &size32))
      { /*cd_Signalize(cdp);*/  err=TRUE;  break; }
    if (recode) encode(buf, size32, FALSE, recode);
    if (!WriteFile(hFile, buf, size32, &wr, NULL))
      { /*syserr(FILE_WRITE_ERROR); */ err=TRUE;  break; }
    offset+=COPY_BUF_SIZE;
  } while (size32==COPY_BUF_SIZE);
  corefree(buf);
  CloseHandle(hFile);
  return !err;
}

CFNC DllExport void WINAPI fname2objname(const char * fname, char * objname)
// Removes path and suffix (last dot from 6.1a)
{ size_t i, length_limit = 0;
  i=strlen(fname);        /* stripping path & suffix */
  while (i--)
  { if (fname[i]=='.' && !length_limit) length_limit=i;
    else if (fname[i]=='\\' || fname[i]=='/' || fname[i]==':') break;
  }
  i++;
  length_limit-=i;
  if (length_limit>OBJ_NAME_LEN) length_limit=OBJ_NAME_LEN;
  strmaxcpy(objname, fname+i, length_limit+1);  // now, file name starts from filename+i; non-alfa chars allowed
}

CFNC DllExport int WINAPI make_object_descriptor(cdp_t cdp, tcateg categ, tobjnum objnum, char * buf)
// Makes "{$$1234567890 Folder_name_and_spaces_to_31}"
{ tobjname folder_name;  uns32 stamp;
  if (cd_Read(cdp, categ==CATEG_TABLE ? TAB_TABLENUM : OBJ_TABLENUM, objnum, OBJ_FOLDER_ATR, NULL, folder_name))
    *folder_name=0;
  if (cd_Read(cdp, categ==CATEG_TABLE ? TAB_TABLENUM : OBJ_TABLENUM, objnum, OBJ_MODIF_ATR, NULL, &stamp))
    stamp=0;
  sprintf(buf, "{$$%lu          ", stamp);
  superconv(ATT_STRING, ATT_STRING, folder_name, buf + 14, t_specif(OBJ_NAME_LEN, cdp->sys_charset, 0, 0), t_specif(2 * OBJ_NAME_LEN, 7, 0, 0), NULL);
  size_t len=strlen(buf);
  if (len < 45)
  { memset(buf+len, ' ', 45-len);  
    len = 45;
  }
  buf[len]='}';  buf[len+1]=0;
  return (int)(len + 1);
}

#if 0
CFNC DllPrezen BOOL WINAPI Add_relation(cdp_t cdp, char * relname, char * tab1name, char * tab2name,
                  const char * atr1name, const char * atr2name, BOOL select)
// atr1name, atr2name are supposed to be Upcase.
{ tobjnum relobj;
  if (cd_Insert_object(cdp, relname, CATEG_RELATION, &relobj))
    { cd_Signalize(cdp);  return FALSE; }
  Upcase(tab1name);  Upcase(tab2name);  // Upcase(atr1name);  Upcase(atr2name); 
  int pos=0, len=OBJ_NAME_LEN+1;
  cd_Write_var(cdp, OBJ_TABLENUM, relobj, OBJ_DEF_ATR, NOINDEX, pos, len, tab1name);
  pos+=len;
  cd_Write_var(cdp, OBJ_TABLENUM, relobj, OBJ_DEF_ATR, NOINDEX, pos, len, tab2name);
  pos+=len;  len=ATTRNAMELEN+1;
  cd_Write_var(cdp, OBJ_TABLENUM, relobj, OBJ_DEF_ATR, NOINDEX, pos, len, atr1name);
  pos+=len;
  cd_Write_var(cdp, OBJ_TABLENUM, relobj, OBJ_DEF_ATR, NOINDEX, pos, len, atr2name);
  Add_component(cdp, CATEG_RELATION, relname, relobj, select);
  return TRUE;
}
#endif

CFNC DllExport void Set_object_time_and_folder(cdp_t cdp, tobjnum objnum, tcateg categ, const char * folder_name, uns32 stamp)
{ ttablenum tb = categ==CATEG_TABLE ? TAB_TABLENUM : OBJ_TABLENUM;
  cd_Write(cdp, tb, objnum, OBJ_FOLDER_ATR, NULL, folder_name, OBJ_NAME_LEN);
  cd_Write(cdp, tb, objnum, OBJ_MODIF_ATR,  NULL, &stamp, sizeof(uns32));
}

CFNC DllKernel BOOL WINAPI Chng_component_flag(cdp_t cdp, tcateg cat, ctptr name, int mask, int setbit)
// Sets the specified flags for the component (in database, cache)
{ tobjnum objnum;  uns16 flags;  tobjname objname;
  strcpy(objname, name);  Upcase(objname);
  if (cat==CATEG_USER || cat==CATEG_GROUP) return FALSE;  // OK, no action
  ttablenum tb = cat==CATEG_TABLE ? TAB_TABLENUM : OBJ_TABLENUM;
 // find object in the cache:
  t_comp * acomp;  unsigned pos;  t_dynar * d_comp = get_object_cache(cdp, cat);  bool found_in_cache=false;
  if (d_comp!=NULL) // cached category
  { if (comp_dynar_search(d_comp, objname, &pos))
    { acomp = (t_comp*)d_comp->acc(pos);
      objnum=acomp->objnum;  flags=acomp->flags;
    }
    found_in_cache=true;
  }
  if (!found_in_cache)
  { if (cd_Find_object(cdp, objname, cat, &objnum)) return TRUE;  // object not found
    cd_Read(cdp, tb, (trecnum)objnum, OBJ_FLAGS_ATR, NULL, &flags);
  }
 // compute new flags:
  flags=(flags & ~mask) | (setbit & mask);
  uns16 dest_flags = flags & ~CO_FLAG_LINK;  // may be in the cache but must not be saved
 // Write flags to the database:
  if (cd_Write(cdp, tb, (trecnum)objnum, OBJ_FLAGS_ATR, NULL, &dest_flags, sizeof(uns16)))
    return TRUE;
 // change flags in the cache:
  if (found_in_cache) // cached category, acomp defined!
    acomp->flags=flags;
  return FALSE;
}


CFNC DllExport void Remove_from_cache(cdp_t cdp, tcateg cat, ctptr name)
{ char objname[OBJ_NAME_LEN+1];  strcpy(objname, name);
  Upcase9(cdp, objname);
 // remove from cache:
  { unsigned pos;
    t_dynar * d_comp=get_object_cache(cdp, cat);
    if (d_comp!=NULL)
    { // Foldery nejsou podle abecedy, nejde pouzit comp_dynar_search 
      if (cat==CATEG_FOLDER)
      { for (int i = 0; i < d_comp->count(); i++)
        { t_comp *comp = (t_comp *)d_comp->acc0(i);
          if (strcmp(comp->name, objname) == 0)
          { *comp->name = 0;
             break;
          }
        }
      }
      else if (comp_dynar_search(d_comp, objname, &pos))
      { tobjnum objn = ((t_comp*)d_comp->acc(pos))->objnum;
        d_comp->delet(pos);  // must first delete the table from the cache and the call inval_table_d which scans relations which needs descriptors of existing tables
        if (cat==CATEG_TABLE || cat==CATEG_CURSOR)
          inval_table_d(cdp, objn, cat);
      }
    }
  }
}

#ifdef LINUX
CFNC DllPrezen void WINAPI Delete_component(cdp_t cdp, tcateg cat, ctptr name, tobjnum objnum)
{ }

CFNC DllPrezen void WINAPI strip_diacr(char * dest, const char * src, const char * app)
// Not called on LINUX, but referenced in impapl.cpp
/* forms fname in dest from src (without diacritics) & app */
{ strmaxcpy(dest, src, 8+1);
//  for (int i=0;  dest[i];  i++)
//    if ((unsigned char)dest[i]>=128)
//      dest[i]=bez_outcode[(unsigned char)dest[i]-128];
//    else if (dest[i]<'0' || dest[i]>'Z' && dest[i]<'a' ||
//             dest[i]>'z' && dest[i]<128 || dest[i]>'9' && dest[i]<'A')
//      if (dest[i]!='*') dest[i]='_';
  strcat(dest, app);
}

#endif

CFNC DllExport char * WINAPI load_inst_cache_addr(view_dyn * inst, trecnum position, tattrib attr, uns16 index, BOOL grow)
{ trecnum intrec;
  if (inst==NULL) return NULL;
  if (inst->dt->cursnum==NOCURSOR) return NULL;  // used by visibility conditions in QBE viewvs
  if (position==NORECNUM)
  { if (inst->stat->vwHdFt & NO_VIEW_FIC) return NULL;
    intrec=inst->Remapping() ? inst->dt->rm_int_recnum : inst->dt->rm_ext_recnum;
  }
  else intrec = exter2inter(inst, position);
  if (intrec <  inst->dt->cache_top ||
      intrec >= inst->dt->cache_top+inst->dt->cache_reccnt) return NULL;

  tptr p = (tptr)(inst->dt->cache_ptr +
                  inst->dt->rec_cache_size * (unsigned)(intrec-inst->dt->cache_top));
  if (*(uns8*)p==0xff) return NULL;   /* error reading this record */
  if (!inst->dt->privils_ok(attr, p, FALSE))
    { client_error(inst->cdp, NO_RIGHT);  return NULL; }
  p+=inst->dt->attrs[attr].offset;
//  if (inst->dt->attrs[attr].type == ATT_BINARY)
//    inst->cdp->RV.result_size=inst->dt->attrs[attr].size; // used by GET_TEXT_VAL when accessing value
  if (index == NOINDEX)
  { if (inst->dt->attrs[attr].type == ATT_AUTOR)
    { User_name_by_id(inst->cdp, (uns8*)p, inst->formlinebuf);
      return inst->formlinebuf;
    }
  }
  else 
  { if (index < ((t_dynar*)p)->count() || grow &&
           (inst->dt->attrs[attr].multi & 0x80 || index<(inst->dt->attrs[attr].multi & 0x7f)))
      p=(tptr)((t_dynar*)p)->acc(index);
    else p=NULL;  /* reading the non-existing multiattribute element */
    if (p==NULL) client_error(inst->cdp, INDEX_OUT_OF_RANGE);  /* message for report generator, cycles otherwise! */
  }
  return p;   
}

