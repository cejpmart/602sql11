// CONNECT.C 
#include "wbdrv.h"       
#pragma hdrstop
#define SQL_NOUNICODEMAP  // prevents mapping SQLGetPrivateProfileString to ...W
#include  <odbcinst.h>           // ODBC installer prototypes: just SQLGetPrivateProfileString
#ifdef WINS
#include <windowsx.h>
#endif
//      Allocate an environment (ENV) block.
int envi_charset = 3;
#include "enumsrv.h"

#if 0
void logit(const char * msg)
{ FILE * f = fopen("/tmp/mylog.txt", "a");
  fprintf(f, msg);
  fclose(f);
}
#endif

static RETCODE SQL_API local_SQLAllocEnv(SQLHENV *phenv)
/* Called by driver manager on each SQLConnect... */
{ ENV * env;
  if (!phenv) return SQL_ERROR;
  env = new ENV;
  if (env==NULL)
  { *phenv=SQL_NULL_HENV;
    return SQL_ERROR;
  }
  *phenv = (SQLHENV)env;
 // find system charset probably used when DM converts Unicode:
  envi_charset = wbcharset_t::get_host_charset().get_code();
  if (!envi_charset) 
#ifdef LINUX  
    envi_charset=131;
  wbcharset_t::prepare_conversions();  // makes the ANSI conversion available, used by catalog functions
#else
    envi_charset=3;
#endif
  return SQL_SUCCESS;
}

//      -       -       -       -       -       -       -       -       -

//      Allocate a DBC block.

static RETCODE SQL_API local_SQLAllocConnect(SQLHENV henv, SQLHDBC *phdbc)
{ DBC * dbc;  
  //DBC *&hdbc=dbc;  -- this was present for the Linux version, I do not know why
  ENV * env=(ENV *)henv;
  CLEAR_ERR(env);
  dbc = new DBC(env);
  if (!dbc)
    { *phdbc=SQL_NULL_HDBC;  raise_err_env(env, SQL_HY001);  /* Memory allocation failure */  return SQL_ERROR; }
 // set the default values: 
  dbc->access_mode  =SQL_MODE_READ_WRITE;
  dbc->async_enabled=FALSE;
  dbc->autocommit   =1;
  dbc->conn_timeout=0;
  dbc->login_timeout=0;
  dbc->txn_isolation=SQL_TXN_READ_COMMITTED;
  dbc->translate_option=0;
  dbc->server_name[0]=0;  // current catalog {qualifier)
  dbc->appl_name[0]=0;    // current schema (owner)
 // set the default statement options: 
  dbc->stmtopt.async_enabled =FALSE;
  dbc->stmtopt.max_data_len  =0;  /* not limited */
  dbc->stmtopt.no_scan       =FALSE;
  dbc->stmtopt.query_timeout =0;  /* no timeout */
  dbc->stmtopt.max_rows      =0;  /* all rows */
  dbc->stmtopt.retrieve_data =TRUE;
  dbc->stmtopt.sim_cursor    =SQL_SC_UNIQUE;
  dbc->stmtopt.use_bkmarks   =FALSE;
  dbc->stmtopt.crowKeyset    =0;  /* full keyset */
  dbc->stmtopt.cursor_type   =SQL_SO_KEYSET_DRIVEN;//SQL_SCROLL_FORWARD_ONLY;
  dbc->stmtopt.concurrency   =SQL_CONCUR_READ_ONLY;
  dbc->stmtopt.metadata      =FALSE;
 // regiter the DBC in the ENV:
  dbc->next_dbc=env->dbcs;  env->dbcs=dbc;
  env->connection_cnt++;
  *phdbc=(SQLHDBC)dbc;
  return SQL_SUCCESS;
}

//      -       -       -       -       -       -       -       -       -

const char CONFIG_PATH8[] = "ConfigPath";
const char CONFIG_PATH9[] = "SQLServerName";
const char SCHEMANAME8[] = "ApplName";
const char SCHEMANAME9[] = "ApplicationName";
char LAST_USERNAME[] = "LastUserName";
#ifdef LINUX
char ODBC_INI[] = "odbc.ini";  // in fact it may be .odbc.ini in ~ or /etc or /usr/local/etc but the SQLGetPrivateProfileString solves this
#else
char ODBC_INI[] = "ODBC.INI";
#endif
char DEFAULT[] = "Default";

#define MAX_CPATH 170

static BOOL do_connect_0(DBC * dbc, const char * dsn, char * server_name)
{ cdp_t cdp;  int err;
 /* allocate cdp */
  cdp=cdp_alloc();
  cdp_init(cdp);
#ifdef WINS  
  err=cd_connect(cdp, server_name, SW_SHOWMINNOACTIVE);
#else  
  err=cd_connect(cdp, server_name, -1);
#endif
  if (err)
  { cdp_free(cdp);
    raise_err_dbc(dbc, WB_ERROR_BASE+err);  
    return TRUE;
  }
  cd_Enable_task_switch(cdp, FALSE);
  cd_waiting(cdp, 50);   // 5 sec waiting, Cetnura
  strmaxcpy(dbc->dsn, dsn, sizeof(dbc->dsn));    /* SqlGetInfo needs it */
  strmaxcpy(dbc->server_name, server_name, sizeof(dbc->server_name));
  dbc->cdp=cdp;
  dbc->connected=TRUE;
  return FALSE;
}

static void do_disconnect(DBC * dbc)
{ cd_interf_close(dbc->cdp);
  cdp_free(dbc->cdp);   /* must not free dbc->cdp, error if two connections open */
  dbc->cdp=NULL;
  dbc->connected=FALSE;
  dbc->dsn[0]=0;
  dbc->server_name[0]=0;
}

static BOOL do_connect_1(DBC * dbc, char * username, char * password)
// Disconnects when login failed!
{ if (cd_Login(dbc->cdp, username, password)) // primarne logovat se zadanym jmenem!!
    if (cd_Login(dbc->cdp, "*NETWORK", NULLSTRING)) // asi zbytecne, v teto fazi uz mam username i heslo, Klinger to chce v tomto poradi kvuli bezpecnosti
    { do_disconnect(dbc);
      raise_err_dbc(dbc, SQL_28000); /* Invalid authorization specification */
      return TRUE;
    }
 /* Povol jmeno uzivatele jako schema, povol "" pro omezeni identifikatoru */
#if WBVERS>=900
  cd_Set_sql_option(dbc->cdp, SQLOPT_USER_AS_SCHEMA|SQLOPT_QUOTED_IDENT,0);
#else
  cd_Set_sql_option(dbc->cdp, SQLOPT_USER_AS_SCHEMA, 0);  // Vabank compatibility
#endif
  cd_Set_progress_report_modulus(dbc->cdp, 0); // do not need any notifications (added in 9.5.1)
  cd_SQL_execute(dbc->cdp, "CALL _SQP_SET_THREAD_NAME('ODBC client')", NULL);
  return FALSE;
}

static bool do_connect(DBC * dbc, const char * dsn, char * server_name, char * username, char * password)
{ 
  if (!do_connect_0(dbc, dsn, server_name)) 
    if (!do_connect_1(dbc, username, password))
      return false;
  return true;
}

tobjname openning_appl = "";

static BOOL protected_set_appl(cdp_t cdp, char * applname)
// Prevents application nesting by recursion
// Fails to open some applications when web server sends multiple requests at the same time!!!
{ BOOL res;
#ifdef LINUX  
  if (*applname && !my_stricmp(applname, openning_appl)) return FALSE;
  strcpy(openning_appl, applname);
#endif  
  res=cd_Set_application(cdp, applname);
#ifdef LINUX  
  *openning_appl=0;
#endif  
  return res;
}

SQLRETURN SQL_API SQLConnectW(SQLHDBC hdbc, SQLWCHAR *szDSN, SQLSMALLINT cbDSN, SQLWCHAR *szUID, SQLSMALLINT cbUID, 
                                            SQLWCHAR *szAuthStr, SQLSMALLINT cbAuthStr)
{ if (!IsValidHDBC(hdbc)) return SQL_INVALID_HANDLE;
  DBC * dbc=(DBC *)hdbc;
  CLEAR_ERR(dbc);
  if (dbc->connected)
  { raise_err_dbc(dbc, SQL_08002); /* Connection in use */
    return SQL_ERROR;
  }
 /* convering string parameters: */
  bool use_default_dsn = !szDSN || !cbDSN || !*szDSN;
  wide2ansi xdsn(szDSN, cbDSN);
  wide2ansi xUID(szUID, cbUID);
  wide2ansi xAuth(szAuthStr, cbAuthStr);
  const char * pdsn = use_default_dsn ? "DEFAULT" : xdsn.get_string();
 /* locating data source name in the ODBC.INI: */
  char server_name[MAX_CPATH+1];  tobjname schemaname;
  SQLGetPrivateProfileString(pdsn, CONFIG_PATH8, NULLSTRING, server_name, sizeof(server_name),ODBC_INI);
  SQLGetPrivateProfileString(pdsn, SCHEMANAME8,  NULLSTRING, schemaname,  sizeof(schemaname), ODBC_INI);
#if WBVERS>=900
  if (!*server_name)
    SQLGetPrivateProfileString(pdsn, CONFIG_PATH9, NULLSTRING, server_name, sizeof(server_name),ODBC_INI);
  if (!*schemaname)
    SQLGetPrivateProfileString(pdsn, SCHEMANAME9,  NULLSTRING, schemaname,  sizeof(schemaname), ODBC_INI);
#endif
  if (!*server_name) strmaxcpy(server_name, pdsn, sizeof(server_name));  /* from DriverConnect */
  if (!*schemaname)
  { raise_err_dbc(dbc, SQL_W0003);
    return SQL_ERROR;
  }
  if (do_connect(dbc, pdsn, server_name, xUID.get_string(), xAuth.get_string())) 
    return SQL_ERROR;
  protected_set_appl(dbc->cdp, schemaname);
  return SQL_SUCCESS;
}

//      -       -       -       -       -       -       -       -       -

#define CONNECT_KEY_COUNT 6
static char * connect_keynames[CONNECT_KEY_COUNT] =
{ "DSN", "UID", "PWD", "SCHEMA", "DRIVER", "SERVER_ACCESS" };

BOOL parse_string(const char * inp, int inplen, char ** keynames, int keycount,
                  char ** values, int * lenlimits, char delimiter)
{ int i;  unsigned len;  BOOL keyfound;  const char * stop, * eq, * start;
  char keyword[40];  unsigned keylen;  BOOL badval=FALSE;
  start=inp;
  while (start < inp+inplen)
  { while ((start < inp+inplen) && (*start == ' ')) start++;
    stop=start;  eq=NULL;  keylen=0;
    while ((stop < inp+inplen) && (*stop != delimiter))
    { if (eq==NULL)
        if (*stop=='=') eq=stop;
        else if (keylen<40) keyword[keylen++]=*stop & 0xdf;
      stop++;
    }
    if (eq!=NULL)   /* contains '=' */
    { keyfound=FALSE;
      for (i=0;  i<keycount;  i++)
        if (keylen==strlen(keynames[i]))
          if (!memcmp(keynames[i], keyword, keylen))
            { keyfound=TRUE;  break; }
      if (keyfound)
      { eq++;
        len=(unsigned)(stop-eq);
        if (len>lenlimits[i]) len=lenlimits[i];
        if (values[i]!=NULL)
          { memcpy(values[i], eq, len);  values[i][len]=0; }
      }
      else if (stricmp(keyword, "DATABASE")) // Delphi sometimes specify this
        badval=TRUE;
    }
    else if (keylen) badval=TRUE;   /* attribute non-empty */
    start=stop+1;  /* to the next attribute */
  }
  return badval;
}
/////////////////////////////// selecting the server ///////////////////////////
#include "regstr.h"

#ifdef WINS                      
static INT_PTR CALLBACK SelServerDlgProc(HWND hDlg, UINT wMsg, WPARAM wParam, LPARAM lParam)
{ 
  switch (wMsg)
  { case WM_INITDIALOG:
    { tobjname server_name;
      t_enum_srv es(t_enum_srv::ENUM_SRV_BOTH, CURRENT_FIL_VERSION_STR);
      while (es.next(server_name))
        SendDlgItemMessage(hDlg, 100, LB_ADDSTRING, 0, (LPARAM)server_name);
      //SendDlgItemMessage(hDlg, 100, LB_SETCURSEL, 0, 0);
      EnableWindow(GetDlgItem(hDlg, IDOK), FALSE);
      SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)lParam);
      break;
    }
    case WM_COMMAND:
      switch (GET_WM_COMMAND_ID(wParam, lParam))
      { case 100:
          if (GET_WM_COMMAND_CMD(wParam, lParam) != LBN_SELCHANGE) break;
          EnableWindow(GetDlgItem(hDlg, IDOK), SendDlgItemMessage(hDlg, 100, LB_GETCURSEL, 0, 0)!=LB_ERR);
          break;
        case IDOK:
        { int sel=(int)SendDlgItemMessage(hDlg, 100, LB_GETCURSEL, 0, 0);
          if (sel==LB_ERR) break;
          SendDlgItemMessage(hDlg, 100, LB_GETTEXT, sel, GetWindowLong(hDlg, DWLP_USER));
          EndDialog(hDlg, TRUE);   break;
        }
        case IDCANCEL:  
          EndDialog(hDlg, FALSE);  break;
        default: return FALSE;
      }
    default: return FALSE;
  }
  return TRUE;
}

BOOL select_server(HWND hParent, char * server_name)
{ return (int)DialogBoxParam(s_hModule, MAKEINTRESOURCE(DLG_SELECT_SERVER), hParent,
                        SelServerDlgProc, (LPARAM)server_name);
}
///////////////////////////// odbc_move_object_names //////////////////////////////
static void WINAPI odbc_move_object_names(cdp_t cdp, tcateg cat, HWND hCtrl)
{ tptr info, src;  int len;
  HCURSOR oldcursor=SetCursor(LoadCursor(NULL, IDC_WAIT));
  SendMessage(hCtrl, WM_SETREDRAW, FALSE, 0);
  if (!cd_List_objects(cdp, cat, cdp->sel_appl_uuid, &info)) 
  { for (src=info;  *src;  src+=len+sizeof(tobjnum)+sizeof(uns16))
    { len=(int)strlen(src)+1;
      tobjname objname;  strcpy(objname, src);  small_string(objname, TRUE);
      SendMessage(hCtrl, CB_ADDSTRING, 0, (LPARAM)objname);
    }
    corefree(info);
  }
  SetCursor(oldcursor);
  SendMessage(hCtrl, WM_SETREDRAW, TRUE, 0);
}

//////////////////////////////////////////////////////////////////////////////////

static char ** aux_values;
static DBC * aux_dbc;

static INT_PTR WINAPI FDriverConnectProc(HWND hdlg, UINT wMsg, WPARAM wParam, LPARAM lParam)
{ char ** values;
  switch (wMsg)
  { case WM_INITDIALOG:
    { aux_values=values=(char**)lParam;
      SetDlgItemText(hdlg, DD_DSN,        values[0]);
      SetDlgItemText(hdlg, DD_SERVERACC,  values[5]);
      SetDlgItemText(hdlg, DD_SCHEMANAME, values[3]);
     // user names:
      HWND hCombo=GetDlgItem(hdlg, DD_USERNAME);
      odbc_move_object_names(aux_dbc->cdp, CATEG_USER, hCombo);
      if (strcmp(values[1], "."))
      { int sel=(int)SendMessage(hCombo, CB_FINDSTRINGEXACT, -1, (LPARAM)values[1]);
        if (sel!=CB_ERR) SendMessage(hCombo, CB_SETCURSEL, (WPARAM)sel, 0);
      }
//      SendDlgItemMessage(hdlg, DD_DSN,       EM_LIMITTEXT, SQL_MAX_DSN_LENGTH,       0);
//      SendDlgItemMessage(hdlg, DD_SERVERACC, EM_LIMITTEXT, MAX_CPATH,     0);
      SendDlgItemMessage(hdlg, DD_USERNAME,  EM_LIMITTEXT, OBJ_NAME_LEN,  0);
      SendDlgItemMessage(hdlg, DD_PASSWORD,  EM_LIMITTEXT, US_PASSWD_LEN, 0);
      return TRUE;
    }
    case WM_COMMAND:
      switch (GET_WM_COMMAND_ID(wParam, lParam))
      { case IDOK:
        { values=aux_values;
//          GetDlgItemText(hdlg, DD_DSN,        values[0], SQL_MAX_DSN_LENGTH+1);
//          GetDlgItemText(hdlg, DD_SERVERACC,  values[5], MAX_CPATH    +1);
#ifdef STOP
          HWND hCombo=GetDlgItem(hdlg, DD_USERNAME);
          int sel=SendMessage(hCombo, CB_GETCURSEL, 0, 0);
          if (sel==CB_ERR) values[1][0]=0;  // anonymous, if not selected
          else SendMessage(hCombo, CB_GETLBTEXT, (WPARAM)sel, (LPARAM)values[1]);
#endif
          GetDlgItemText(hdlg, DD_USERNAME, values[1], OBJ_NAME_LEN+1);
          GetDlgItemText(hdlg, DD_PASSWORD, values[2], US_PASSWD_LEN+1);
          EndDialog(hdlg, TRUE);
          return TRUE;
        }
        case IDCANCEL:
          EndDialog(hdlg, FALSE);
          return TRUE;
      }
  }
  return FALSE;
}

static INT_PTR CALLBACK SelApplDlgProc(HWND hdlg, UINT wMsg, WPARAM wParam, LPARAM lParam)
{ char ** values;  HWND hCombo;  LRESULT sel;
  switch (wMsg)
  { case WM_INITDIALOG:
      aux_values=values=(char**)lParam;
      hCombo=GetDlgItem(hdlg, DD_SCHEMANAME);
      odbc_move_object_names(aux_dbc->cdp, CATEG_APPL, hCombo);
      sel=SendMessage(hCombo, CB_FINDSTRINGEXACT, -1, (LPARAM)values[3]);
      SendMessage(hCombo, CB_SETCURSEL, (WPARAM)sel, 0);
      return TRUE;

    case WM_COMMAND:
      switch (GET_WM_COMMAND_ID(wParam, lParam))
      { case DD_SCHEMANAME:
          if (GET_WM_COMMAND_CMD(wParam, lParam) != CBN_CLOSEUP) return TRUE;
          SetFocus(GetDlgItem(hdlg, IDOK));
          return TRUE;
        case IDOK:
          values=aux_values;
          hCombo=GetDlgItem(hdlg, DD_SCHEMANAME);
          sel=SendMessage(hCombo, CB_GETCURSEL, 0, 0);
          if (sel==CB_ERR) return TRUE;
          SendMessage(hCombo, CB_GETLBTEXT, (WPARAM)sel, (LPARAM)values[3]);
//          SQLWritePrivateProfileString(values[0], SCHEMANAME, values[3], ODBC_INI);
// do not store this selection any more
          EndDialog(hdlg, TRUE);
          return TRUE;
        case IDCANCEL:
          EndDialog(hdlg, FALSE);
          return TRUE;
      }
  }
  return FALSE;
}
#endif // WINS

static RETCODE finalize_connect(DBC * dbc, RETCODE result, char *values[],
  int lenlimits[], SQLWCHAR *szConnStrOut, SQLSMALLINT cbConnStrOutMax, SQLSMALLINT *pcbConnStrOut, bool ret_bytes)
{ char buf[SQL_MAX_DSN_LENGTH+MAX_CPATH+OBJ_NAME_LEN+US_PASSWD_LEN+OBJ_NAME_LEN+70];
  unsigned i, pos;
  protected_set_appl(dbc->cdp, values[3]);
  strcpy(dbc->appl_name, values[3]);
 /* store the full string: */
  for (pos=i=0;  i<CONNECT_KEY_COUNT;  i++)
    if (i && i!=4 || *values[i])  /* "DRIVER=xyz" only if nonempty */
    { strcpy(buf+pos, connect_keynames[i]);  pos+=(int)strlen(connect_keynames[i]);
      buf[pos++]='=';
      if (strcmp(values[i], "."))
        { strcpy(buf+pos, values[i]);  pos+=(int)strlen(values[i]); }
      buf[pos++]=';';
    }
  buf[pos]=0;
  if (ret_bytes ?
       write_string_wide_b (buf, szConnStrOut, cbConnStrOutMax, pcbConnStrOut, envi_charset) :
       write_string_wide_ch(buf, szConnStrOut, cbConnStrOutMax, pcbConnStrOut, envi_charset))
  { raise_err_dbc(dbc, SQL_01004);  /* Data truncated */
    return result == SQL_SUCCESS ? SQL_SUCCESS_WITH_INFO : result;
  }
  return result;
}

//      -        -       -       -       -       -       -       -       -
                      
SQLRETURN SQL_API SQLDriverConnectW(SQLHDBC hdbc, SQLHWND hwnd, SQLWCHAR *szConnStrIn, SQLSMALLINT cbConnStrIn,
    SQLWCHAR *szConnStrOut, SQLSMALLINT cbConnStrOutMax, SQLSMALLINT *pcbConnStrOut, SQLUSMALLINT fDriverCompletion)
// FILENAME, SAVEFILE removed by the DM.
{ int   iRet;  RETCODE result=SQL_SUCCESS;
  char dsn[SQL_MAX_DSN_LENGTH+1]="", username[OBJ_NAME_LEN+1]=".",
       password[US_PASSWD_LEN+1]=".", schemaname[OBJ_NAME_LEN+1]="";
  char cpath[MAX_CPATH+1]="", driver[50+1]="";
  char * values[CONNECT_KEY_COUNT] = { dsn, username, password, schemaname, driver, cpath };
  int lenlimits[CONNECT_KEY_COUNT] = { SQL_MAX_DSN_LENGTH, OBJ_NAME_LEN, US_PASSWD_LEN, OBJ_NAME_LEN, 50, MAX_CPATH };
  if (!IsValidHDBC(hdbc)) return SQL_INVALID_HANDLE;
  DBC * dbc=(DBC *)hdbc;
  CLEAR_ERR(dbc);
  if (dbc->connected)
  { raise_err_dbc(dbc, SQL_08002);  /* Connection in use */
    return SQL_ERROR;
  }

  wide2ansi xConnStrIn(szConnStrIn, cbConnStrIn==SQL_NTS ? cbConnStrIn : cbConnStrIn/sizeof(SQLWCHAR));
  if (parse_string(xConnStrIn.get_string(), xConnStrIn.get_len(), connect_keynames, CONNECT_KEY_COUNT, values, lenlimits, ';'))
  { raise_err_dbc(dbc, SQL_01S00);  /* Invalid connection string attribute */
    result=SQL_SUCCESS_WITH_INFO;
  }
 /* locating data source name in the ODBC.INI, adding info: */
  if (!*driver)  // DRIVER specified when creating or openning a file data source
  { //SQLSetConfigMode(ODBC_BOTH_DSN);
    if (!*dsn) strcpy(dsn, DEFAULT);   
    if (!*cpath)      SQLGetPrivateProfileString(dsn, CONFIG_PATH8,   NULLSTRING, cpath,      sizeof(cpath),      ODBC_INI);
    if (!*username)   SQLGetPrivateProfileString(dsn, LAST_USERNAME,  NULLSTRING, username,   sizeof(username),   ODBC_INI);
    if (!*schemaname) SQLGetPrivateProfileString(dsn, SCHEMANAME8,    NULLSTRING, schemaname, sizeof(schemaname), ODBC_INI);
#if WBVERS>=900
    if (!*cpath)      SQLGetPrivateProfileString(dsn, CONFIG_PATH9,   NULLSTRING, cpath,      sizeof(cpath),      ODBC_INI);
    if (!*schemaname) SQLGetPrivateProfileString(dsn, SCHEMANAME9,    NULLSTRING, schemaname, sizeof(schemaname), ODBC_INI);
#endif
  }
  if (fDriverCompletion == SQL_DRIVER_NOPROMPT && !*cpath)
  { raise_err_dbc(dbc, SQL_IM007);  /* No data source specified, dialog prohibited */
    result=SQL_SUCCESS_WITH_INFO;
  }
#ifdef WINS
  if ((fDriverCompletion == SQL_DRIVER_COMPLETE || fDriverCompletion == SQL_DRIVER_COMPLETE_REQUIRED) &&
      (!*cpath || *username=='.' || *password=='.') ||
      fDriverCompletion == SQL_DRIVER_PROMPT)
  { if (!*cpath)
    { if (!select_server(hwnd, cpath))
        return SQL_NO_DATA_FOUND;  /* dialog box cancelled */
    }
    if (do_connect_0(dbc, dsn, cpath)) return SQL_ERROR;
    aux_dbc = dbc;  // cdp for dialogs
    iRet = (int)DialogBoxParam(s_hModule, MAKEINTRESOURCE(EDRIVERCONNECT), hwnd,
                        FDriverConnectProc, (LPARAM)values);
    if (!iRet || iRet == -1)
    { do_disconnect(dbc);
      return SQL_NO_DATA_FOUND;  /* dialog box cancelled or not opened */
    }
    if (do_connect_1(dbc, username, password)) return SQL_ERROR;
    if (fDriverCompletion == SQL_DRIVER_PROMPT || !*schemaname)
    { iRet = (int)DialogBoxParam(s_hModule, MAKEINTRESOURCE(DLG_SELAPPL), hwnd, SelApplDlgProc, (LPARAM)values);
      if (!iRet || iRet == -1)
      { do_disconnect(dbc);
        return SQL_NO_DATA_FOUND;  /* dialog box cancelled or not opened */
      }
    }
  }
#else
  if ((fDriverCompletion == SQL_DRIVER_COMPLETE || fDriverCompletion == SQL_DRIVER_COMPLETE_REQUIRED) &&
       !*cpath ||
      fDriverCompletion == SQL_DRIVER_PROMPT)
  {
    //assert(0); // jednak by k tomu nemelo dojit
    return SQL_NO_DATA_FOUND; // a pokud ano, je to chyba.
  }
#endif
  else  /* dialog not necessary or NO_PROMPT: */
  { if (*username=='.') *username=0; // possible for SQL_DRIVER_NOPROMPT
    if (*password=='.') *password=0; // possible for SQL_DRIVER_NOPROMPT
    if (do_connect(dbc, dsn, cpath, username, password))
      return SQL_ERROR;
  }
  return finalize_connect(dbc, result, values, lenlimits, szConnStrOut, cbConnStrOutMax, pcbConnStrOut, false);
}

//      -       -       -       -       -       -       -       -       -
static char BC_dsn[SQL_MAX_DSN_LENGTH+1]="";
static char BC_str[] = "SCHEMA:Aplikace=?;UID:Jmeno uzivatele=?;PWD:Heslo=?";
static char BC_str_c[] = "SERVER_ACCESS:Pristup k serveru=?;SCHEMA:Aplikace=?;UID:Jmeno uzivatele=?;PWD:Heslo=?";

SQLRETURN SQL_API SQLBrowseConnectW(SQLHDBC hdbc, SQLWCHAR *szConnStrIn, SQLSMALLINT cbConnStrIn,
    SQLWCHAR *szConnStrOut, SQLSMALLINT cbConnStrOutMax, SQLSMALLINT *pcbConnStrOut)
{ DBC * dbc;
  char username[OBJ_NAME_LEN+1]=".",  password[US_PASSWD_LEN+1]=".", schemaname[OBJ_NAME_LEN+1]="";
  char cpath[MAX_CPATH+1]="", driver[50+1]="";
  char * values[CONNECT_KEY_COUNT] = { BC_dsn, username, password, schemaname, driver, cpath };
  int lenlimits[CONNECT_KEY_COUNT] = { SQL_MAX_DSN_LENGTH, OBJ_NAME_LEN, US_PASSWD_LEN, OBJ_NAME_LEN, 50, MAX_CPATH };
  if (!IsValidHDBC(hdbc)) return SQL_INVALID_HANDLE;
  dbc=(DBC *)hdbc;  CLEAR_ERR(dbc);
  if (dbc->connected)
  { raise_err_dbc(dbc, SQL_08002);  /* Connection in use */
    return SQL_ERROR;
  }
  if (!dbc->browse_connect_status) BC_dsn[0]=0;
  wide2ansi xConnStrIn(szConnStrIn, cbConnStrIn==SQL_NTS ? cbConnStrIn : cbConnStrIn/sizeof(SQLWCHAR));
  if (parse_string(xConnStrIn.get_string(), xConnStrIn.get_len(), connect_keynames, CONNECT_KEY_COUNT, values, lenlimits, ';'))
  { raise_err_dbc(dbc, SQL_01S00);  /* Invalid connection string attribute */
    return SQL_NEED_DATA;
  }
  if (!*BC_dsn && !*driver)   /* load default dsn */
    strcpy(BC_dsn, DEFAULT);
  if (!*cpath && *BC_dsn)
  { SQLGetPrivateProfileString(BC_dsn, CONFIG_PATH8, NULLSTRING, cpath, sizeof(cpath), ODBC_INI);
#if WBVERS>=900
    if (!*cpath)
      SQLGetPrivateProfileString(BC_dsn, CONFIG_PATH9, NULLSTRING, cpath, sizeof(cpath), ODBC_INI);
#endif
  }
  if (!*cpath || *username=='.' || *password=='.' || !*schemaname)
  { dbc->browse_connect_status=1;
    if (write_string_wide_b(!*cpath ? BC_str_c : BC_str, szConnStrOut, cbConnStrOutMax, pcbConnStrOut, envi_charset))
    { raise_err_dbc(dbc, SQL_01004);  /* Data truncated */
      return SQL_SUCCESS_WITH_INFO;
    }
    else return SQL_NEED_DATA;
  }
  dbc->browse_connect_status=0;
  if (do_connect(dbc, values[0], values[5], values[1], values[2]))
    return SQL_ERROR;
  return finalize_connect(dbc, SQL_SUCCESS, values, lenlimits, szConnStrOut, cbConnStrOutMax, pcbConnStrOut, true);
}

//      -       -       -       -       -       -       -       -       -

//      Allocate a SQL statement

static RETCODE SQL_API local_SQLAllocStmt(SQLHDBC hdbc, SQLHSTMT *phstmt)
{ STMT * stmt;  DBC * dbc;
  if (!IsValidHDBC(hdbc)) return SQL_INVALID_HANDLE;
  dbc=(DBC*)hdbc;
  CLEAR_ERR(dbc);
  *phstmt=SQL_NULL_HSTMT;   /* for error exits */
 // DM checks if the connecton is open, but to be sure:
  if (!dbc->connected)
  { raise_err_dbc(dbc, SQL_08003);  /* Connection not open */
    return SQL_ERROR;
  }
  stmt = new STMT(dbc);
  if (!stmt)
  { raise_err_dbc(dbc, SQL_HY001);  /* Memory allocation failure */
    return SQL_ERROR;
  }
 // alocate descriptors for bookmarks:
  if (stmt->impl_ipd.drecs.acc(0)==NULL || stmt->impl_apd.drecs.acc(0)==NULL ||
      stmt->impl_ird.drecs.acc(0)==NULL || stmt->impl_ard.drecs.acc(0)==NULL)
  { delete stmt;
    raise_err_dbc(dbc, SQL_HY001);  /* Memory allocation failure */
    return SQL_ERROR;
  }

  CLEAR_ERR(stmt);
 /* init statement options by the connection defaults: */
  stmt->async_enabled =dbc->stmtopt.async_enabled;
  stmt->max_data_len  =dbc->stmtopt.max_data_len;
  stmt->no_scan       =dbc->stmtopt.no_scan;
  stmt->query_timeout =dbc->stmtopt.query_timeout;
  stmt->max_rows      =dbc->stmtopt.max_rows;
  stmt->retrieve_data =dbc->stmtopt.retrieve_data;
  stmt->sim_cursor    =dbc->stmtopt.sim_cursor;
  stmt->use_bkmarks   =dbc->stmtopt.use_bkmarks;
  stmt->crowKeyset    =dbc->stmtopt.crowKeyset;
  stmt->cursor_type   =dbc->stmtopt.cursor_type;
  stmt->concurrency   =dbc->stmtopt.concurrency;
  stmt->metadata      =dbc->stmtopt.metadata;
  *phstmt=(SQLHSTMT)stmt;
  return SQL_SUCCESS;
}

static RETCODE SQL_API local_SQLAllocDesc(SQLHDBC hdbc, SQLHDESC *phdesc)
{ if (!IsValidHDBC(hdbc)) return SQL_INVALID_HANDLE;
  DBC * dbc=(DBC*)hdbc;
  CLEAR_ERR(dbc);
  *phdesc=SQL_NULL_HDESC;   /* for error exits */
  DESC * desc = new DESC(dbc, false, FALSE, 0);
  if (desc==NULL)
  { raise_err_dbc(dbc, SQL_HY001);  /* Memory allocation failure */
    return SQL_ERROR;
  }
  *phdesc=(SQLHDESC)desc;
  return SQL_SUCCESS;
}

SQLRETURN SQL_API SQLAllocHandle(SQLSMALLINT HandleType, SQLHANDLE InputHandle, SQLHANDLE *OutputHandle)
{ 
  switch (HandleType) 
  { case SQL_HANDLE_ENV:  return local_SQLAllocEnv    (OutputHandle);
    case SQL_HANDLE_DBC:  return local_SQLAllocConnect(InputHandle, OutputHandle);
    case SQL_HANDLE_STMT: return local_SQLAllocStmt   (InputHandle, OutputHandle);
    case SQL_HANDLE_DESC: return local_SQLAllocDesc   (InputHandle, OutputHandle);
  }
  return SQL_ERROR;  // never performed
}

//      -       -       -       -       -       -       -       -       -

RETCODE SQL_API SQLFreeStmt(SQLHSTMT hstmt, UWORD fOption)
{ if (!IsValidHSTMT(hstmt)) return SQL_INVALID_HANDLE;
  STMT * stmt=(STMT*)hstmt;  int i;
  CLEAR_ERR(stmt);
  if (stmt->AE_mode)
  { raise_err(stmt, SQL_HY010);  /* Function sequence error */
    return SQL_ERROR;
  }
  if (stmt->async_run)
  { raise_err(stmt, SQL_HY010);  /* Function sequence error */
    return SQL_ERROR;
  }
  switch (fOption)
  { case SQL_DROP:
      close_result_set(stmt);
      discard_other_result_sets(stmt); // not checking if any result set exists!
      un_prepare(stmt);  /* releases resources in WB server */
     /* free everything: */
      delete stmt;
      break;
    case SQL_CLOSE:
      close_result_set(stmt);  // no error of result set not open
      discard_other_result_sets(stmt); // must close all!
      break;
    case SQL_UNBIND:
      for (i=0;  i<=stmt->ard->count;  i++)
        stmt->ard->drecs.acc(i)->clear();  // unbinds
      stmt->ard->count=0;
      //stmt->ard->drecs.t_dynar::~t_dynar();  -- must not delete bookmark
      break;
    case SQL_RESET_PARAMS: // should not change IPD!
      drop_params(stmt);   /* releases resources in WB server */
      for (i=0;  i<=stmt->apd->count;  i++)
        stmt->apd->drecs.acc(i)->clear();  // unbinds, releases buffers and dynar 
      stmt->apd->count=0;
      //stmt->apd->drecs.t_dynar::~t_dynar();  -- must not delete bookmark
      break;
    default: // not performed
      return SQL_ERROR;
  }
  return SQL_SUCCESS;
}

//      -       -       -       -       -       -       -       -       -
RETCODE SQL_API SQLDisconnect(HDBC hdbc)
{ if (!IsValidHDBC(hdbc)) return SQL_INVALID_HANDLE;
  DBC * dbc=(DBC *)hdbc;
  CLEAR_ERR(dbc);
  if (dbc->browse_connect_status) /* in SQLBrowseConnect, cannot be connected */
  { /* free browse info */
    dbc->browse_connect_status=0;
    return SQL_SUCCESS;
  }
  if (!dbc->connected)
  { raise_err_dbc(dbc, SQL_08003);  /* Connection not open */
    return SQL_ERROR;
  }
  if (dbc->in_trans)
  { raise_err_dbc(dbc, SQL_25000);  /* Invalid transaction state */
    return SQL_ERROR;
  }
 /* free all statements: */
  while (dbc->stmts)
  { if (dbc->stmts->async_run) /* must not disconnect if any async. oper. running */
    { raise_err_dbc(dbc, SQL_HY010);  /* Function sequence error */
      return SQL_ERROR;
    }
    SQLFreeStmt((SQLHSTMT)dbc->stmts, SQL_DROP);
  }
 // free all descriptors:
  while (dbc->descs!=NULL) delete dbc->descs;  // delete removes descriptor from le list
 // disconnecting:
  do_disconnect(dbc);
  return SQL_SUCCESS;
}

/////////////////////////// FreeHandle ///////////////////////////////////////////

static RETCODE SQL_API FreeConnect(SQLHDBC hdbc)
{ if (!IsValidHDBC(hdbc)) return SQL_INVALID_HANDLE;
  DBC * dbc=(DBC *)hdbc;
  CLEAR_ERR(dbc);
  if (dbc->connected)
  { raise_err_dbc(dbc, SQL_HY010);  /* Function sequence error */
    return SQL_ERROR;
  }
 // unregister the DBC from the ENV:
  DBC ** pdbc = &dbc->my_env->dbcs;
  while (*pdbc!=NULL && *pdbc!=dbc) pdbc=&(*pdbc)->next_dbc;
  *pdbc=dbc->next_dbc;
  dbc->my_env->connection_cnt--;
 // destroy the DBC:
  delete dbc;
  return SQL_SUCCESS;
}

//      -       -       -       -       -       -       -       -       -

static RETCODE SQL_API FreeEnv(SQLHENV henv)
{ if (!IsValidHENV(henv)) return SQL_INVALID_HANDLE;
  ENV * env=(ENV *)henv;
  CLEAR_ERR(env);
  delete env;
  return SQL_SUCCESS;
}


SQLRETURN SQL_API SQLCloseCursor(SQLHSTMT StatementHandle)
// Same as SQLFreeStmt(, SQL_CLOSE), but can return error 24000
{ if (!IsValidHSTMT(StatementHandle)) return SQL_INVALID_HANDLE;
  STMT * stmt=(STMT*)StatementHandle;
  CLEAR_ERR(stmt);
  BOOL any_result_set=stmt->has_result_set;
  close_result_set(stmt);  
  any_result_set|=discard_other_result_sets(stmt); // must close all!
  if (!any_result_set)
  { raise_err(stmt, SQL_24000);
    return SQL_ERROR;
  }
  return SQL_SUCCESS;
}

static RETCODE SQL_API SQLFreeDesc(SQLHDESC hdesc)
{ if (!IsValidHDESC(hdesc)) return SQL_INVALID_HANDLE;
  DESC * desc=(DESC *)hdesc;
  delete desc;  // removes it from dbc and replaces it in all stmts
  return SQL_SUCCESS;
}

SQLRETURN SQL_API SQLFreeHandle(SQLSMALLINT HandleType, SQLHANDLE Handle)
{ switch (HandleType) 
	{ case SQL_HANDLE_ENV:  return FreeEnv    (Handle);
	  case SQL_HANDLE_DBC:  return FreeConnect(Handle);
	  case SQL_HANDLE_STMT: return SQLFreeStmt(Handle, SQL_DROP);
    case SQL_HANDLE_DESC: return SQLFreeDesc(Handle);
  }
  return SQL_ERROR;  // never performed
}
///////////////////////////////// STMT methods //////////////////////////////////
STMT::STMT(DBC * dbc) : t_odbc_obj(TAG_STMT), result_sets(sizeof(uns32)),
  impl_apd(dbc, true, FALSE, 1), impl_ard(dbc, true, FALSE, 2), 
  impl_ipd(dbc, true, TRUE,  1), impl_ird(dbc, true, TRUE,  2) 
{ my_dbc=dbc;  cdp=dbc->cdp;
  if (dbc->stmts)
  { next_stmt=dbc->stmts;
    prev_stmt=dbc->stmts->prev_stmt;
    next_stmt->prev_stmt=prev_stmt->next_stmt=this;
  }
  else
  { next_stmt=prev_stmt=this;
    dbc->stmts=this;
  }
  auto_ipd=TRUE;
  apd=&impl_apd;
  ard=&impl_ard; 
  err.errcnt=0;
  err.return_code=SQL_SUCCESS;
  is_prepared=param_info_retrieved=impl_ipd.populated=FALSE;
  *cursor_name=0;
  preloaded_recnum=NORECNUM;
  preloaded_count=0;
}

STMT::~STMT(void)
{/* unlink statement handle from dbc: */
  if (next_stmt==this)
    my_dbc->stmts=NULL;
  else
  { next_stmt->prev_stmt=prev_stmt;
    prev_stmt->next_stmt=next_stmt;
    if (my_dbc->stmts==this) my_dbc->stmts=next_stmt;
  }
  tag=0;
}

void * STMT::operator new(size_t size) throw ()
{
  STMT * stmt = (STMT*)corealloc(sizeof(STMT), 55);
  if (stmt == NULL) return NULL;
  memset(stmt, 0, sizeof(STMT));
  return stmt;
}

void STMT::operator delete(void * ptr)
{ 
  corefree(ptr);
}
//////////////////////////// DBC methods //////////////////////////////////////////
void DBC::remove_descr_from_list_and_statements(DESC * desc)
// Deleting descriptor belonging to this DBC, remove it from list & all its statements
{// removing from the list of descriptors allocated in the DBC: 
  DESC ** pdesc = &descs;
  while (*pdesc!=NULL && *pdesc!=desc) pdesc=&(*pdesc)->next_desc_in_dbc;
  if (*pdesc!=NULL) *pdesc=desc->next_desc_in_dbc;
 // removing from the statements in the DBC:
  for (STMT * stmt = stmts;  stmt!=NULL && stmt!=stmts;  stmt=stmt->next_stmt)
  { if (stmt->apd==desc) stmt->apd=&stmt->impl_apd;
    if (stmt->ard==desc) stmt->ard=&stmt->impl_ard;
  }
}

void t_desc_rec::init(void)
{ memset(this, 0, sizeof(t_desc_rec));
  concise_type=type=SQL_C_DEFAULT;  
  //data_ptr=NULL;  indicator_ptr=NULL;  octet_length_ptr=NULL;
}

//////////////////////////////////// Unicode conversions ////////////////////////////////////////////////
int odbclen(const SQLWCHAR * str)
{ int len=0;
  while (*str)
    { len++;  str++; }
  return len;  
}

wide2ansi::wide2ansi(const SQLWCHAR * widestr, SQLINTEGER wlen, int charset)
// wlen is the number of characters.
{
  if (!widestr) wlen=alen=0;
  else 
  { if (wlen==SQL_NTS) wlen=odbclen(widestr);
    else if (wlen<0) wlen=0;  // masking the error, probably has already been detected by the DM 
    alen=wlen;  // same number of characters because not converting to UTF-8
  }
  astr=(char*)corealloc(alen+1, 81);
  if (!astr) { astr=NULLSTRING;  alen=0; }  // error, but does not crash
  else 
  { conv2to1(widestr, wlen, astr, wbcharset_t(charset), alen);
#ifdef STOP  
    if (wlen)  // otherwise [alen] is zero
#ifdef WINS
      alen=WideCharToMultiByte(wbcharset_t(charset).cp(), 0, widestr, wlen, astr, alen, NULL, NULL);
#else
    { int i;
     // UCS-4 -> UCS-2:
      for (i=0;  i<wlen;  i++)
        ((wuchar*)widestr)[i] = widestr[i];
      alen=ODBCWideCharToMultiByte(wbcharset_t(charset), (const char*)widestr, wlen, astr);
     // UCS-2 -> UCS-4:
      for (i=wlen-1;  i>=0;  i--)
        ((SQLWCHAR*)widestr)[i] = ((wuchar*)widestr)[i];
    }
#endif
    astr[alen]=0;
#endif    
  }
}

bool write_string_wide_ch(const char * text, SQLWCHAR *szStrOut, SQLSMALLINT cbStrOutMax, SQLSMALLINT *pcbStrOut, int charset)
/* writes null-terminated string in a standard way, return TRUE on trunc. */
// cbStrOutMax is supposed to be in BYTES
// pcbStrOut is returned in CHARACTERS
{ 
  int wlen, len=(int)strlen(text);
  wuchar * wstr = (wuchar*)corealloc(sizeof(wuchar) * (len+1), 81);
  if (!wstr) wlen=0;
  else
#ifdef STOP  
#ifdef WINS		
		wlen = MultiByteToWideChar(wbcharset_t(charset).cp(), 0 /*MB_PRECOMPOSED gives ERROR_INVALID_FLAGS for UTF-8 on XP*/, text, len, wstr, len);
#else
		wlen = ODBCMultiByteToWideChar(wbcharset_t(charset), text, len, wstr);
#endif		
#endif
  conv1to2(text, len, wstr, wbcharset_t(charset), 0);
  wlen=(int)wuclen(wstr);  
  if (pcbStrOut != NULL) *pcbStrOut=wlen;
  if (szStrOut == NULL || cbStrOutMax <= 0) 
    { corefree(wstr);  return false; }  // must not write anything, just returning the length
  wlen *= sizeof(SQLWCHAR);  // converting to bytes
  int cp = wlen < cbStrOutMax-sizeof(SQLWCHAR) ? wlen : cbStrOutMax-sizeof(SQLWCHAR);
  memcpy(szStrOut, wstr, cp);  szStrOut[cp/sizeof(SQLWCHAR)]=0;
  corefree(wstr);
  return wlen >= cbStrOutMax;
}

bool write_string_wide_b(const char * text, SQLWCHAR *szStrOut, SQLSMALLINT cbStrOutMax, SQLSMALLINT *pcbStrOut, int charset)
/* writes null-terminated string in a standard way, return TRUE on trunc. */
// cbStrOutMax is supposed to be in BYTES
// pcbStrOut is returned in BYTES
{ 
  int wlen, len=(int)strlen(text);
  wuchar * wstr = (wuchar*)corealloc(sizeof(wuchar) * (len+1), 81);
  if (!wstr) wlen=0;
  else
#ifdef STOP
#ifdef WINS		
		wlen = MultiByteToWideChar(wbcharset_t(charset).cp(), 0 /*MB_PRECOMPOSED gives ERROR_INVALID_FLAGS for UTF-8 on XP*/, text, len, wstr, len);
#else
		wlen = ODBCMultiByteToWideChar(wbcharset_t(charset), text, len, wstr);
#endif		
#endif  
    conv1to2(text, len, wstr, wbcharset_t(charset), 0);
  wlen=(int)wuclen(wstr);  
  if (pcbStrOut != NULL) *pcbStrOut=wlen*(int)sizeof(SQLWCHAR);
  if (szStrOut == NULL || cbStrOutMax <= 0) 
    { corefree(wstr);  return false; }  // must not write anything, just returning the length
  wlen *= sizeof(SQLWCHAR);  // converting to bytes
  int cp = wlen < cbStrOutMax-sizeof(SQLWCHAR) ? wlen : cbStrOutMax-sizeof(SQLWCHAR);
  memcpy(szStrOut, wstr, cp);  szStrOut[cp/sizeof(SQLWCHAR)]=0;
  corefree(wstr);
  return wlen >= cbStrOutMax;
}

bool write_string_wide_b4(const char * text, SQLWCHAR *szStrOut, SQLINTEGER cbStrOutMax, SQLINTEGER *pcbStrOut, int charset)
// Like the above, but lengths are INTEGERS.
/* writes null-terminated string in a standard way, return TRUE on trunc. */
// cbStrOutMax is supposed to be in BYTES
// pcbStrOut is returned in BYTES
{ 
  int wlen, len=(int)strlen(text);
  wuchar * wstr = (wuchar*)corealloc(sizeof(wuchar) * (len+1), 81);
  if (!wstr) wlen=0;
  else
#ifdef STOP
#ifdef WINS		
		wlen = MultiByteToWideChar(wbcharset_t(charset).cp(), 0 /*MB_PRECOMPOSED gives ERROR_INVALID_FLAGS for UTF-8 on XP*/, text, len, wstr, len);
#else
		wlen = ODBCMultiByteToWideChar(wbcharset_t(charset), text, len, wstr);
#endif		
#endif  
    conv1to2(text, len, wstr, wbcharset_t(charset), 0);
  wlen=(int)wuclen(wstr);  
  if (pcbStrOut != NULL) *pcbStrOut=wlen*sizeof(SQLWCHAR);
  if (szStrOut == NULL || cbStrOutMax <= 0) 
    { corefree(wstr);  return false; }  // must not write anything, just returning the length
  wlen *= sizeof(SQLWCHAR);  // converting to bytes
  int cp = wlen < cbStrOutMax-sizeof(SQLWCHAR) ? wlen : cbStrOutMax-sizeof(SQLWCHAR);
  memcpy(szStrOut, wstr, cp);  szStrOut[cp/sizeof(SQLWCHAR)]=0;
  corefree(wstr);
  return wlen >= cbStrOutMax;
}

#ifdef LINUX
CFNC DllExport char * WINAPI load_inst_cache_addr(view_dyn * inst, trecnum position, tattrib attr, uns16 index, BOOL grow)
{ return NULL; }

#endif
