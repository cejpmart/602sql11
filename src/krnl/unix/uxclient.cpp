/****************************************************************************/
/* nwclient.cpp - client NLM WB serveru v NLM                               */
/****************************************************************************/
#include "winrepl.h"

#include <xlocale.h>
#include "defs.h"
#include "comp.h"
#include "cint.h"
#include <signal.h>

#include "dynar.cpp"

#pragma off(unreferenced)
 
void write_error_info(cdp_t cdp, const char * caller)
{ int err=cd_Sz_error(cdp);
	if (err==0)return ;
	char buffer[1024];
	Get_error_num_text(cdp, err, buffer, sizeof(buffer));
  if (err) printf("\nChyba cislo %d (%s) v %s.", err, buffer, caller);
}

CFNC int WINAPI cd_Signalize(cdp_t cdp)
{ write_error_info(cdp, "nespecifikovanem modulu");
  return cd_Sz_error(cdp);
}

CFNC DllPrezen BOOL WINAPI Signalize(void)
{ return cd_Signalize(GetCurrTaskPtr()); }

CFNC DllPrezen void WINAPI syserr(int operat)
{ printf("\nChyba cislo %d pri praci se souborem.", operat); }

CFNC void WINAPI no_memory(void)
{ printf("\nNeni dost pameti."); }

CFNC tptr WINAPI sigalloc(unsigned size, uns8 owner)
{ tptr p=(tptr) corealloc(size, owner);
  if (p==NULL) no_memory();
  return p;
}

#ifdef STOP  // must use the real functions!!!
CFNC BOOL WINAPI SendToWeb(char *label,char *text_to_send) { return FALSE; }
CFNC BOOL WINAPI SetSTWError(char *errmsg) { return FALSE; }
CFNC BOOL WINAPI GetValue(char *varname,sig16 index,char *value) { return FALSE; }
CFNC sig16 WINAPI GetValueCount(char *varname) { return 0; }
CFNC sig16 WINAPI GetVarCount() { return 0; }
CFNC BOOL WINAPI GetVar(sig16 pos,char *varname) { return FALSE; }
CFNC BOOL WINAPI GetVarValue(sig16 pos,char *varvalue) { return FALSE; }
CFNC void  WINAPI SetUserError(char *errmsg) { }
#endif

/* Is in messages.cpp. It is defined as for console output. */
CFNC DllPrezen void WINAPI wrnbox(const char * text)
{ fprintf(stderr, "602SQL warning: %s\n", text); }
CFNC DllPrezen void WINAPI errbox(const char * text)
{ fprintf(stderr, "602SQL error: %s\n", text); }
CFNC DllPrezen void WINAPI infbox(const char * text)
{ printf("602SQL: %s\n", text); }
CFNC DllPrezen void WINAPI Set_status_nums(trecnum num0, trecnum num1)
{ return; }


/* Is in dirsel.cpp */
CFNC DllPrezen BOOL Get_object_modif_time(cdp_t cdp, tobjnum objnum, tcateg categ, uns32 * ts)
// Reads the time of the last change of the object.
{ return !cd_Read(cdp, categ==CATEG_TABLE ? TAB_TABLENUM : OBJ_TABLENUM, objnum, OBJ_MODIF_ATR, NULL, ts); }

CFNC DllPrezen BOOL WINAPI get_object_descriptor_data(const char * buf, tobjname folder_name, uns32 * stamp)
{ if (strlen(buf) >= 46 && buf[0]=='{' && buf[1]=='$' && buf[2]=='$' && buf[13]==' ' && buf[45]=='}')
  { if (stamp)
    { const char * p=buf+3;  *stamp=0;
      while (*p!=' ')
      { *stamp=10 * *stamp + (*p-'0');
        p++;
      } 
    }
    if (folder_name)
    { memcpy(folder_name, buf+14, OBJ_NAME_LEN); 
      int i=OBJ_NAME_LEN;  while (i && folder_name[i-1]==' ') i--;  folder_name[i]=0;
    }
    return TRUE;
  }
  else
  { if (folder_name) *folder_name=0;
    if (stamp) *stamp=0;
    return FALSE;
  }
}

CFNC DllPrezen tobjnum WINAPI Add_component(cdp_t cdp, tcateg categ, ctptr
	name, tobjnum objnum, BOOL select_it, BOOL noshow, BOOL is_encrypted,
	t_folder_index folder)
{ uns16 flags = is_encrypted;
 // flags to database:
  if (flags & CO_FLAG_LINK || categ==CATEG_TABLE && flags & CO_FLAG_ODBC)
    return objnum; // both objects alread have the proper flags
  if (categ==CATEG_USER || categ==CATEG_GROUP) return FALSE;  // OK, no action
  if (!ENCRYPTED_CATEG(categ))
    return objnum;  // OK, no flags, no action
  ttablenum tb = categ==CATEG_TABLE ? TAB_TABLENUM : OBJ_TABLENUM;
  if (cd_Write(cdp, tb, (trecnum)objnum, OBJ_FLAGS_ATR, NULL, &flags, sizeof(uns16)))
    { cd_Signalize(cdp);  return NOOBJECT; }
  return objnum;
}


/* from fsed.cpp */
CFNC DllPrezen void WINAPI remove_diacr(char * str, unsigned len)
{ for (int i=0;  i<len;  i++)
	if ((unsigned char)str[i]>=128)
		str[i]='_';
	else if (str[i]<'0' || str[i]>'9' && str[i]<'A' ||
		str[i]>'Z' && str[i]<'a' || str[i]>'z')
		str[i]='_';
}

/* from tablesql.cpp */
#ifdef STOP
#include "flstr.h"

static void write_constraint_characteristics(int co_cha, t_flstr * src)
{ src->put(
    co_cha==COCHA_UNSPECIF       ? "" :
    co_cha==COCHA_IMM_DEFERRABLE ? " INITIALLY IMMEDIATE DEFERRABLE " :
    co_cha==COCHA_IMM_NONDEF     ? " INITIALLY IMMEDIATE " :   // NOT DEFERRABLE IMPLIED
    co_cha==COCHA_DEF            ? " INITIALLY DEFERRED " : "");
}


CFNC DllKernel tptr WINAPI table_all_to_source(table_all * ta, BOOL altering)
// Creates the SQL description of the table in ta. Never stores the schema name of the table.
// ta->schemaname must be specified (although not stored).
// Stores schema names of the referenced tables iff they are specified and are different than the ta->schemaname.
{ int i;  t_flstr src(3000,3000);
  src.put(altering ? "ALTER TABLE " : "CREATE TABLE ");  src.putq(ta->selfname);
  if (ta->tabdef_flags & TFL_NO_JOUR    ) src.put(" NO_JOURNAL");
  if (ta->tabdef_flags & TFL_ZCR        ) src.put(" ZCR");
  if (ta->tabdef_flags & TFL_UNIKEY     ) src.put(" UNIKEY");
  if (ta->tabdef_flags & TFL_LUO        ) src.put(" LUO");
  if (ta->tabdef_flags & TFL_TOKEN      ) src.put(" TOKEN");
  if (ta->tabdef_flags & TFL_DETECT     ) src.put(" DETECT");
  if (ta->tabdef_flags & TFL_VARIANT    ) src.put(" VARIANT");
  if (ta->tabdef_flags & TFL_REC_PRIVILS) src.put(" REC_PRIVILS");
  if (ta->tabdef_flags & TFL_DOCFLOW    ) src.put(" DOCFLOW");
  if (ta->tabdef_flags & TFL_REPL_DEL   ) src.put(" REPL_DEL");
  if (altering) src.put("\r\n");  else src.put(" (\r\n");

  int startatr = (ta->attrs.count() && !stricmp(ta->attrs.acc(0)->name, "DELETED")) ? 1 : 0;
  while (ta->attrs.count() > startatr &&
         !memcmp(ta->attrs.acc(startatr)->name, "_W5_", 4))
    startatr++;

  BOOL firstel = TRUE;
  if (altering)   /* dropped attributes first: */
    for (i=startatr; i<ta->attrs.count(); i++)
    { atr_all * att = ta->attrs.acc0(i);
      if (*att->name && att->state==ATR_DEL)
      { if (firstel) firstel=FALSE;  else src.put(",\r\n");
        src.put("DROP ");  src.putq(att->name);
      }
    }
  for (i=startatr; i<ta->attrs.count(); i++)
  { atr_all * att = ta->attrs.acc0(i);
    if (!*att->name || !att->type) continue;
    if (altering)
    { if (att->state==ATR_OLD || att->state==ATR_DEL) continue;
      if (firstel) firstel=FALSE;  else src.put(",\r\n");
      src.put(att->state==ATR_NEW ? "ADD " : /* att->state==ATR_MODIF */ "ALTER ");
    }
    else // CREATE TABLE
      if (firstel) firstel=FALSE;  else src.put(",\r\n");

    if (altering && att->state==ATR_NEW)
      { src.putint(i+1-startatr);  src.putc(' '); }
    src.putq(att->name);  src.putc(' ');
   /* type: */
    switch (att->type)
    { case ATT_BOOLEAN:  src.put("BIT");                  break;
      case ATT_CHAR:     
          src.put(att->specif.wide_char ? "NCHAR" : "CHAR");  
        break; /* differs from string[1] */
      case ATT_INT8:     
        if (att->specif.scale==0) src.put("TINYINT");
        else { src.put("NUMERIC(2,");   src.putint(att->specif.scale);  src.putc(')'); }
        break;
      case ATT_INT64:
        if (att->specif.scale==0) src.put("BIGINT");
        else { src.put("NUMERIC(18,");  src.putint(att->specif.scale);  src.putc(')'); }
        break;
      case ATT_INT16:    
        if (att->specif.scale==0) src.put("SMALLINT");
        else { src.put("NUMERIC(4,");  src.putint(att->specif.scale);  src.putc(')'); }
        break;
      case ATT_INT32:    
        if (att->specif.scale==0) src.put("INTEGER");
        else { src.put("NUMERIC(9,");  src.putint(att->specif.scale);  src.putc(')'); }
        break;
      case ATT_MONEY:    src.put("NUMERIC(14,2)");        break;
      case ATT_FLOAT:    src.put("REAL");                 break;
      case ATT_STRING:  
        if (att->specif.wide_char)
          { src.put("NCHAR(");  src.putint(att->specif.length/2); }
        else
          { src.put( "CHAR(");  src.putint(att->specif.length);   }
        src.putc(')');
        break;
      case ATT_BINARY:
        src.put("BINARY(");  src.putint(att->specif.length);  src.putc(')');  break;
      case ATT_DATE:     src.put("DATE");                 break;
      case ATT_TIME:     src.put("TIME");                 break;
      case ATT_TIMESTAMP:src.put("TIMESTAMP");            break;
      case ATT_PTR:      src.put("POINTER[");  goto ptrs;
      case ATT_BIPTR:    src.put("BIPTR[");
       ptrs:
       { tptr pom=(char*)ta->names.acc(i);
         if (*pom) { src.putq(pom);  src.putc('.'); }
         src.putq(pom+sizeof(tobjname));  src.putc(']');
         break;
       }
      case ATT_AUTOR:   src.put("AUTOR");                break;
      case ATT_DATIM:   src.put("DATIM");                break;
      case ATT_HIST:    src.put("HISTORY");              break;
      case ATT_TEXT:    
        src.put(att->specif.wide_char ? "NCLOB" : "LONG VARCHAR");  break;
      case ATT_RASTER:  src.put("BINARY(65000)");        break;
      case ATT_NOSPEC:  src.put("LONG VARBINARY");       break;
      case ATT_SIGNAT:  src.put("SIGNATURE");            break;
    }
    src.putc(' ');
   /* multiattribute: */
    if (att->multi!=1)
    { src.put("PREALLOC");  src.putint(att->multi & 0x7f);
      if (att->multi & 0x80) src.put(" EXPANDABLE");
    }
   /* default value */
    if (att->defval!=NULL)
      { src.put("DEFAULT ");  src.put(att->defval);  src.putc(' '); }
  /* NOT NULL: */
    if (!att->nullable)  src.put("NOT NULL ");
  /* COLLATE: */
    if (att->type==ATT_STRING || att->type==ATT_CHAR || att->type==ATT_TEXT)
      if (att->specif.charset!=0 || att->specif.ignore_case)
        if (att->specif.charset==1)
          if (att->specif.ignore_case) src.put("COLLATE CSISTRING ");
          else                         src.put("COLLATE CSSTRING ");
        else if (att->specif.ignore_case)
          src.put("COLLATE IGNORE_CASE ");
  /* VALUES: */
    if (altering)
      { src.put("VALUES ");  src.putssx(att->alt_value ? att->alt_value : NULLSTRING); }
  }

  for (i=0; i<ta->indxs.count(); i++)
  { ind_descr * indx=ta->indxs.acc0(i);
    if (indx->state==INDEX_DEL || !indx->text || !*indx->text || !indx->index_type) continue;
    if (!memcmp(indx->constr_name, "_W5_", 4) ||
        !memcmp(indx->constr_name, "@", 2)) continue;
    if (firstel) firstel=FALSE;  else src.put(",\r\n");
    if (*indx->constr_name)
      { src.put("CONSTRAINT "); src.putq(indx->constr_name); }
    if      (indx->index_type==INDEX_PRIMARY)   src.put("PRIMARY KEY (");
    else if (indx->index_type==INDEX_UNIQUE)    src.put("UNIQUE (");
    else if (indx->index_type==INDEX_NONUNIQUE) src.put("INDEX (");
    src.put(indx->text);  src.putc(')');
    write_constraint_characteristics(indx->co_cha, &src);
  }

  for (i=0; i<ta->checks.count(); i++)
  { check_constr * check=ta->checks.acc0(i);
    if (check->state==ATR_DEL || !check->text || !*check->text) continue;
    if (firstel) firstel=FALSE;  else src.put(",\r\n");
    if (*check->constr_name)
      { src.put("CONSTRAINT "); src.putq(check->constr_name); }
    src.put("CHECK (");  src.put(check->text);  src.putc(')');
    write_constraint_characteristics(check->co_cha, &src);
  }

  for (i=0; i<ta->refers.count(); i++)
  { forkey_constr * ref=ta->refers.acc0(i);
    if (ref->state==ATR_DEL || !ref->text || !*ref->text || !*ref->desttab_name) continue;
    if (firstel) firstel=FALSE;  else src.put(",\r\n");
    if (*ref->constr_name)
      { src.put("CONSTRAINT "); src.putq(ref->constr_name); }
    src.put("FOREIGN KEY (");  src.put(ref->text);  src.put(") REFERENCES ");
    if (*ref->desttab_schema && strcmp(ref->desttab_schema, ta->schemaname))
      { src.putq(ref->desttab_schema);  src.putc('.'); }
    src.putq(ref->desttab_name);  
    if (ref->par_text && *ref->par_text) 
      { src.putc('(');  src.put(ref->par_text);  src.putc(')'); }
    if (ref->update_rule!=REFRULE_NO_ACTION)
    { src.put(" ON UPDATE ");
      src.put(ref->update_rule==REFRULE_SET_NULL    ? "SET NULL" :
              ref->update_rule==REFRULE_SET_DEFAULT ? "SET DEFAULT" : "CASCADE");
    }
    if (ref->delete_rule!=REFRULE_NO_ACTION)
    { src.put(" ON DELETE ");
      src.put(ref->delete_rule==REFRULE_SET_NULL    ? "SET NULL" :
              ref->delete_rule==REFRULE_SET_DEFAULT ? "SET DEFAULT" : "CASCADE");
    }
    write_constraint_characteristics(ref->co_cha, &src);
  }
  if (!altering) src.putc(')');
  src.put("\r\n");
  return src.error() ? NULL : src.unbind();
}
#endif

/* Is currently undefined - do we need it? */

SQLRETURN SQL_API SQLExecDirect(
    SQLHSTMT           hstmt,
    SQLCHAR FAR       *szSqlStr,
    SQLINTEGER         cbSqlStr)
{
	fprintf(stderr, "602SQL: %s (%d, \"%s\", %d) undefined on Linux\n", 
		hstmt, szSqlStr, cbSqlStr);
	return -1;
}

SQLRETURN SQL_API SQLGetInfo(
    SQLHDBC            hdbc,
    SQLUSMALLINT       fInfoType,
    SQLPOINTER         rgbInfoValue,
    SQLSMALLINT        cbInfoValueMax,
    SQLSMALLINT FAR   *pcbInfoValue)
{
	fprintf(stderr, "SQLGetInfo undefined on Linux for now\n");
	return -1;
}

CFNC DllExport BOOL WINAPI odbc_synchronize_view(cdp_t cdp, BOOL update,
	t_connection * conn, ltable * dt, tcursnum cursnum, trecnum extrec,
	tptr new_vals, tptr old_vals, BOOL on_current)
{
	fprintf(stderr, "odbc_synchronize_view undefined on Linux for now\n");
	return -1;
}

CFNC DllExport BOOL WINAPI make_ext_odbc_name(odbc_tabcurs * tc, tptr extname,
	uns32 usage_mask)
{
	fprintf(stderr, "make_ext_odbc_name undefined on Linux for now\n");
	return -1;
}

CFNC DllKernel int type_WB_2_ODBC(xtype_info * ti, int wbtype)
	// Returns ODBC type index+1 or 0 if not found.
{
	return 0;
}

CFNC DllKernel tptr WINAPI odbc_table_to_source(table_all * ta, BOOL altering, t_connection * conn)
{
  if (conn==NULL) return table_all_to_source(ta, altering);
  errbox("odbc_table_to_source undefined on Linux for now\n");
  return NULL;
}

CFNC DllKernel void WINAPI odbc_stmt_error(cdp_t cdp, HSTMT hStmt)
{
  errbox("odbc_stmt_error undefined on Linux for now\n");
}

CFNC DllKernel ltable * WINAPI create_cached_access(cdp_t cdp, t_connection *
	conn, tptr source, BOOL source_is_table, tcursnum cursnum)
{
  ltable * ltab = new ltable(cdp, conn);
  if (ltab==NULL) { no_memory();  return NULL; }
  { if (ltab->describe(cursnum))
    { ltab->cursnum=cursnum;  return ltab; }
  }
  delete ltab;
  return NULL;
}

CFNC DllKernel odbc_tabcurs * WINAPI get_odbc_tc(cdp_t cdp, tcursnum curs)
{
  wrnbox("create_cached_access undefined on Linux\n");
  return NULL;
}

#ifndef __WXGTK__
#if WBVERS<900
/* from dataproc.cpp */
CFNC DllExport BOOL WINAPI User_name_by_id(cdp_t cdp, uns8* user_uuid, char * namebuf)
{ for (int i=0;  i<UUID_SIZE;  i++) if (user_uuid[i]) break;
	if (i==UUID_SIZE)  // NULL value in author
		strcpy(namebuf, "Nobody");
	else
	{ tobjnum objnum;
		if (cd_Find_object_by_id(cdp, user_uuid, CATEG_USER, &objnum))
		{
			strcpy(namebuf, "Unknown");
			return FALSE;
		}
		else if (cd_Read(cdp, USER_TABLENUM, (trecnum)objnum, USER_ATR_LOGNAME, NULL, namebuf))
			namebuf[0]=0;
	}
	return TRUE;
}

CFNC DllExport BOOL WINAPI wb_write_cache(cdp_t cdp, ltable * dt, tcursnum cursnum,
	trecnum * erec, tptr cache_rec, BOOL global_save)
{
  return dt->write(cursnum, erec, cache_rec, global_save);
}

CFNC DllPrezen void WINAPI cache_free(ltable * dt, unsigned offset, unsigned
	count)
{
  dt->free(offset, count);
}

CFNC DllExport void WINAPI cache_load(ltable * dt, view_dyn * inst,
	   unsigned offset, unsigned count, trecnum new_top)
{
    dt->load(inst, offset, count, new_top);
}

CFNC DllExport void WINAPI catr_set_null(tptr pp, int type) /* write the WB NULL representation */
{ switch (type)
{   case ATT_BOOLEAN: case ATT_INT8: *pp=(uns8)0x80;  break;
    case ATT_STRING:
    case ATT_CHAR:    case ATT_TEXT:
    case ATT_ODBC_DECIMAL:  case ATT_ODBC_NUMERIC:
      *pp=0;  break;
    case ATT_INT16:    *(uns16*)pp=NONESHORT;  break;
    case ATT_INT32:  case ATT_TIME:  case ATT_DATE:
    case ATT_DATIM:  case ATT_TIMESTAMP:
      *(uns32*)pp=NONEINTEGER;  break;
    case ATT_INT64:
      *(sig64 *)pp=NONEBIGINT; break;
    case ATT_ODBC_DATE:
      ((DATE_STRUCT*)pp)->day=32;  break;
    case ATT_ODBC_TIME:
      ((TIME_STRUCT*)pp)->hour=25;  break;
    case ATT_ODBC_TIMESTAMP:
      ((TIMESTAMP_STRUCT*)pp)->day=32;  break;
    case ATT_MONEY:
      *(uns32*)pp=0;  *(uns16*)(pp+4)=0x8000;  break;
    case ATT_FLOAT:  *(double*)pp=NONEREAL;  break;
    case ATT_PTR:  case ATT_BIPTR:  *(uns32*)pp=(uns32)-1;  break;
    case ATT_AUTOR: memset(pp, 0, UUID_SIZE);  break;
  }
}

CFNC DllPrezen char * WINAPI load_inst_cache_addr(view_dyn * inst, trecnum position, tattrib attr, uns16 index, BOOL grow)
{ return NULL; }
#endif

CFNC DllPrezen BOOL WINAPI register_edit(view_dyn * inst, int attrnum)
{ return TRUE; }

#if 0
typedef char tpiece[6];

uns8 tpsize[ATT_AFTERLAST] =
 {0,                                    /* fictive, NULL */
  1,1,2,4,6,8,0,0,0,0,4,4,4,sizeof(trecnum),sizeof(trecnum), /* normal  */
  UUID_SIZE,4,4+sizeof(tpiece),         /* special */
  4+sizeof(tpiece),4+sizeof(tpiece),    /* raster, text */
  4+sizeof(tpiece),4+sizeof(tpiece),    /* nospec, signature */
  9,0,0,0,0,                            /* UNTYPED, not used & ATT_NIL */
  1,sizeof(ttablenum),sizeof(tobjnum),8,0,3*sizeof(tcursnum), /* file, table, cursor, dbrec, view, statcurs */
  0,0,0,0,0,0,0,0,0,0,0,1,8             /* ..., int8, int64 */
};
#endif

#endif

CFNC DllKernel void WINAPI destroy_cached_access(ltable * dt)
{ delete dt; }

CFNC DllKernel void WINAPI deliver_messages(cdp_t cdp, BOOL waitcurs)
{
  wrnbox("deliver_messages undefined on Linux for now");
}

CFNC DllPrezen int WINAPI SezamBox(HWND hWnd, const char * text, const char * caption, LPCSTR icon, unsigned * buttons)
{
  wrnbox("SezamBox undefined on Linux.");
  return 0;
}



#include "dwordptr.cpp"

//const char *configfile="/etc/602sql";  -- moved to enumsrv.c

void SetCursor(int i) { }
