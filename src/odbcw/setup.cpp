/*
** SETUP.C - This is the ODBC driver code for setup.
*/
/*--------------------------------------------------------------------------
  setup.c -- Sample ODBC setup

  This code demonstrates how to interact with the ODBC Installer.  These
  functions may be part of your ODBC driver or in a separate DLL.

  The ODBC Installer allows a driver to control the management of
  data sources by calling the ConfigDSN entry point in the appropriate
  DLL.  When called, ConfigDSN receives four parameters:

    hwndParent ---- Handle of the parent window for any dialogs which
                    may need to be created.  If this handle is NULL,
                    then no dialogs should be displayed (that is, the
                    request should be processed silently).

    fRequest ------ Flag indicating the type of request (add, configure
                    (edit), or remove).

    lpszDriver ---- Far pointer to a null-terminated string containing
                    the name of your driver.  This is the same string you
                    supply in the ODBC.INF file as your section header
                    and which ODBC Setup displays to the user in lieu
                    of the actual driver filename.  This string needs to
                    be passed back to the ODBC Installer when adding a
                    new data source name.

    lpszAttributes- Far pointer to a list of null-terminated attribute
                    keywords.  This list is similar to the list passed
                    to SQLDriverConnect, except that each key-value
                    pair is separated by a null-byte rather than a
                    semicolon.  The entire list is then terminated with
                    a null-byte (that is, two consecutive null-bytes
                    mark the end of the list).  The keywords accepted
                    should be those for SQLDriverConnect which are
                    applicable, any new keywords you define for ODBC.INI,
                    and any additional keywords you decide to document.

  ConfigDSN should return TRUE if the requested operation succeeds and
  FALSE otherwise.  The complete prototype for ConfigDSN is:

  BOOL FAR PASCAL ConfigDSN(HWND    hwndParent,
                            UINT    fRequest,
                            LPSTR   lpszDriver,
                            LPCSTR  lpszAttributes)

  Your setup code should not write to ODBC.INI directly to add or remove
  data source names.  Instead, link with ODBCINST.LIB (the ODBC Installer
  library) and call SQLWriteDSNToIni and SQLRemoveDSNFromIni.
  Use SQLWriteDSNToIni to add data source names.  If the data source name
  already exists, SQLWriteDSNToIni will delete it (removing all of its
  associated keys) and rewrite it.  SQLRemoveDSNToIni removes a data
  source name and all of its associated keys.

  For NT compatibility, the driver code should not use the
  Get/WritePrivateProfileString windows functions for ODBC.INI, but instead,
  use SQLGet/SQLWritePrivateProfileString functions that are macros (16 bit) or
  calls to the odbcinst.dll (32 bit).

--------------------------------------------------------------------------*/


// Includes ----------------------------------------------------------------
#include  "wbdrv.h"                                    // Local include files
#pragma hdrstop
#include  "odbcinst.h"                  // ODBC installer prototypes
#include  <string.h>                                    // C include files
#include  <windowsx.h>                                    // C include files
#include  "regstr.h"

#ifdef __cplusplus
extern "C" {
#endif
long   atol(const char *__s);
char * ltoa(long the_value, char *__string, int __radix);
#ifdef __cplusplus
}
#endif


// Constants ---------------------------------------------------------------
#define MIN(x,y)      ((x) < (y) ? (x) : (y))

#define MAXPATHLEN      (255+1)           // Max path length
#define MAXKEYLEN       (15+1)            // Max keyword length
#define MAXDESC         (255+1)           // Max description length
#define MAXDSNAME       (32+1)            // Max data source name length

const char EMPTYSTR  []= "";

// ODBC.INI keywords
const char INI_KDESC   []="Description";  // Data source description
const char CONFIG_PATH8[]="ConfigPath";   // First option
const char SCHEMANAME8[]="ApplName";     // Second option
const char CONFIG_PATH9[] = "SQLServerName";
const char SCHEMANAME9[] = "ApplicationName";
const char INI_SDEFAULT[] = "Default";    // Default data source name
const char szTranslateName[] = "TranslationName";
const char szTranslateDLL[] = "TranslationDLL";
const char szTranslateOption[] = "TranslationOption";
const char szUnkTrans[] = "Unknown Translator";
#if WBVERS>=900
#define CONFIG_PATH CONFIG_PATH9
#define SCHEMANAME  SCHEMANAME9
#else
#define CONFIG_PATH CONFIG_PATH8
#define SCHEMANAME  SCHEMANAME8
#endif

// Attribute key indexes (into an array of Attr structs, see below)
#define KEY_DSN                 0
#define KEY_DESC                1
#define KEY_OPT1                2
#define KEY_OPT2                3
#define KEY_TRANSNAME   4
#define KEY_TRANSOPTION 5
#define KEY_TRANSDLL    6
#define NUMOFKEYS               7                               // Number of keys supported

// Attribute string look-up table (maps keys to associated indexes)
static struct {
  char  szKey[MAXKEYLEN];
  int    iKey;
} s_aLookup[] = { 
    {"DSN",               KEY_DSN            },
    {"DESC",              KEY_DESC                   },
    {"Description",       KEY_DESC               },
    {"ConfigPath",        KEY_OPT1             },
    {"ApplName",          KEY_OPT2               },
    {"SQLServerName",     KEY_OPT1             },
    {"ApplicationName",   KEY_OPT2               },
    {"TranslateName",     KEY_TRANSNAME        },
    {"TranslateDLL",      KEY_TRANSDLL         },
    {"TranslateOption",   KEY_TRANSOPTION },
    {"",                  0               } };

// Types -------------------------------------------------------------------
typedef struct tagAttr {
        BOOL  fSupplied;
        char  szAttr[MAXPATHLEN];
} Attr, FAR * LPAttr;


// Globals -----------------------------------------------------------------
// NOTE:  All these are used by the dialog procedures
typedef struct tagSETUPDLG {
        HWND    hwndParent;         // Parent window handle
        LPCSTR  lpszDrvr;           // Driver description
        Attr    aAttr[NUMOFKEYS];   // Attribute array
        char    szDSN[MAXDSNAME];   // Original data source name
        BOOL    fNewDSN;            // New data source flag
        BOOL    fDefault;           // Default data source flag

} SETUPDLG, FAR *LPSETUPDLG;


// Prototypes --------------------------------------------------------------
void WINAPI CenterDialog         (HWND    hdlg);
static INT_PTR CALLBACK ConfigDlgProc(HWND hdlg,
                                           UINT    wMsg,
                                           WPARAM  wParam,
                                           LPARAM  lParam);
void WINAPI ParseAttributes (LPCSTR    lpszAttributes, LPSETUPDLG lpsetupdlg);
BOOL CALLBACK SetDSNAttributes(HWND     hwnd, LPSETUPDLG lpsetupdlg);

/* ConfigDSN ---------------------------------------------------------------
  Description:  ODBC Setup entry point
                This entry point is called by the ODBC Installer
                (see file header for more details)
  Input      :  hwnd ----------- Parent window handle
                fRequest ------- Request type (i.e., add, config, or remove)
                lpszDriver ----- Driver name
                lpszAttributes - data source attribute string
  Output     :  TRUE success, FALSE otherwise
--------------------------------------------------------------------------*/
extern "C" BOOL INSTAPI ConfigDSNW(HWND hwndParent, WORD fRequest, LPCWSTR lpszDriver, LPCWSTR lpszAttributes)
{       BOOL  fSuccess;                                            // Success/fail flag
        GLOBALHANDLE hglbAttr;
        LPSETUPDLG lpsetupdlg;
  int atrlen=0;
  if (lpszAttributes)
  { while (lpszAttributes[atrlen])
      atrlen += (int)wcslen(lpszAttributes+atrlen) + 1;
  }
  wide2ansi xDriver(lpszDriver, SQL_NTS);
  wide2ansi xAttributes(lpszAttributes, atrlen);

       // Allocate attribute array
        hglbAttr = GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT, sizeof(SETUPDLG));
        if (!hglbAttr) return FALSE;
        lpsetupdlg = (LPSETUPDLG)GlobalLock(hglbAttr);

       // Parse attribute string
        if (lpszAttributes) ParseAttributes(xAttributes.get_string(), lpsetupdlg);

       // Save original data source name
        if (lpsetupdlg->aAttr[KEY_DSN].fSupplied)
                lstrcpy(lpsetupdlg->szDSN, lpsetupdlg->aAttr[KEY_DSN].szAttr);
        else
                lpsetupdlg->szDSN[0] = '\0';

       // Remove data source
        if (ODBC_REMOVE_DSN == fRequest)
         // Fail if no data source name was supplied
          if (!lpsetupdlg->aAttr[KEY_DSN].fSupplied) fSuccess = FALSE;
          else // Otherwise remove data source from ODBC.INI
            fSuccess = SQLRemoveDSNFromIni(lpsetupdlg->aAttr[KEY_DSN].szAttr);
       // Add or Configure data source
        else
        {// Save passed variables for global access (e.g., dialog access)
          lpsetupdlg->hwndParent = hwndParent;
          lpsetupdlg->lpszDrvr     = xDriver.get_string();
          lpsetupdlg->fNewDSN      = (ODBC_ADD_DSN == fRequest);
          lpsetupdlg->fDefault     =
                  !lstrcmpi(lpsetupdlg->aAttr[KEY_DSN].szAttr, INI_SDEFAULT);

          // Display the appropriate dialog (if parent window handle supplied)
          if (hwndParent)  // Display dialog(s)
            fSuccess = (IDOK == DialogBoxParam(s_hModule,
                                 MAKEINTRESOURCE(CONFIGDSN),
                                 hwndParent,
                                 ConfigDlgProc,
                                 (LPARAM)lpsetupdlg));
          else if (lpsetupdlg->aAttr[KEY_DSN].fSupplied)
            fSuccess = SetDSNAttributes(hwndParent, lpsetupdlg);
          else
            fSuccess = FALSE;
        }
        GlobalUnlock(hglbAttr);
        GlobalFree(hglbAttr);
        return fSuccess;
}


/* CenterDialog ------------------------------------------------------------
        Description:  Center the dialog over the frame window
        Input      :  hdlg -- Dialog window handle
        Output     :  None
--------------------------------------------------------------------------*/
void WINAPI CenterDialog(HWND hdlg)
{
        HWND    hwndFrame;
        RECT    rcDlg, rcScr, rcFrame;
        int             cx, cy;

        hwndFrame = GetParent(hdlg);

        GetWindowRect(hdlg, &rcDlg);
        cx = rcDlg.right  - rcDlg.left;
        cy = rcDlg.bottom - rcDlg.top;

        GetClientRect(hwndFrame, &rcFrame);
        ClientToScreen(hwndFrame, (LPPOINT)(&rcFrame.left));
        ClientToScreen(hwndFrame, (LPPOINT)(&rcFrame.right));
        rcDlg.top    = rcFrame.top  + (((rcFrame.bottom - rcFrame.top) - cy) >> 1);
        rcDlg.left   = rcFrame.left + (((rcFrame.right - rcFrame.left) - cx) >> 1);
        rcDlg.bottom = rcDlg.top  + cy;
        rcDlg.right  = rcDlg.left + cx;

        GetWindowRect(GetDesktopWindow(), &rcScr);
        if (rcDlg.bottom > rcScr.bottom)
        {
                rcDlg.bottom = rcScr.bottom;
                rcDlg.top    = rcDlg.bottom - cy;
        }
        if (rcDlg.right  > rcScr.right)
        {
                rcDlg.right = rcScr.right;
                rcDlg.left  = rcDlg.right - cx;
        }

        if (rcDlg.left < 0) rcDlg.left = 0;
        if (rcDlg.top  < 0) rcDlg.top  = 0;

        MoveWindow(hdlg, rcDlg.left, rcDlg.top, cx, cy, TRUE);
        return;
}

static char WB_inst_key[] = "SOFTWARE\\Software602\\602SQL";
static char Database_str[] = "\\Database";       

#include "enumsrv.h"

static void list_server_names(HWND hCombo)
{ tobjname a_server_name;  
  t_enum_srv es(t_enum_srv::ENUM_SRV_BOTH, CURRENT_FIL_VERSION_STR);
  while (es.next(a_server_name))
    SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM)a_server_name);
}

typedef SQLRETURN SQL_API t_SQLAllocHandle(SQLSMALLINT HandleType, SQLHANDLE InputHandle, SQLHANDLE *OutputHandle);
typedef SQLRETURN SQL_API t_SQLFreeHandle(SQLSMALLINT HandleType, SQLHANDLE Handle);
typedef SQLRETURN SQL_API t_SQLDriverConnect(SQLHDBC ConnectionHandle,SQLHWND WindowHandle,SQLCHAR * InConnectionString,SQLSMALLINT     StringLength1,SQLCHAR *     OutConnectionString,SQLSMALLINT     BufferLength,SQLSMALLINT *     StringLength2Ptr,SQLUSMALLINT     DriverCompletion);
typedef SQLRETURN SQL_API t_SQLDisconnect(SQLHDBC ConnectionHandle);
typedef SQLRETURN SQL_API t_SQLError(SQLHENV EnvironmentHandle,SQLHDBC ConnectionHandle, SQLHSTMT StatementHandle,SQLCHAR *Sqlstate, SQLINTEGER *NativeError,SQLCHAR *MessageText, SQLSMALLINT BufferLength,SQLSMALLINT *TextLength);
typedef SQLRETURN SQL_API t_SQLSetEnvAttr(SQLHENV	EnvironmentHandle,SQLINTEGER Attribute, SQLPOINTER ValuePtr, SQLINTEGER StringLength);

void test_conn(HWND hdlg, const char * dsn)
{ HENV hEnv;  HDBC hDbc = SQL_NULL_HDBC;
  SDWORD native;  char text[SQL_MAX_MESSAGE_LENGTH+1];  unsigned char state[SQL_SQLSTATE_SIZE+1];  SQLRETURN retcode;
  *text=0;
  HINSTANCE hInst = LoadLibrary("odbc32.dll");
  if (hInst)
  { t_SQLAllocHandle   * p_SQLAllocHandle   = (t_SQLAllocHandle  *)GetProcAddress(hInst, "SQLAllocHandle");
    t_SQLFreeHandle    * p_SQLFreeHandle    = (t_SQLFreeHandle   *)GetProcAddress(hInst, "SQLFreeHandle");
    t_SQLDriverConnect * p_SQLDriverConnect = (t_SQLDriverConnect*)GetProcAddress(hInst, "SQLDriverConnect");
    t_SQLDisconnect    * p_SQLDisconnect    = (t_SQLDisconnect   *)GetProcAddress(hInst, "SQLDisconnect");
    t_SQLError         * p_SQLError         = (t_SQLError        *)GetProcAddress(hInst, "SQLError");         
    t_SQLSetEnvAttr    * p_SQLSetEnvAttr    = (t_SQLSetEnvAttr   *)GetProcAddress(hInst, "SQLSetEnvAttr");
    if (SQL_SUCCEEDED((*p_SQLAllocHandle)(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &hEnv)))
    { (*p_SQLSetEnvAttr)(hEnv, SQL_ATTR_ODBC_VERSION, (SQLPOINTER)SQL_OV_ODBC3, 0);
      retcode=(*p_SQLAllocHandle)(SQL_HANDLE_DBC, hEnv, &hDbc);
      if (SQL_SUCCEEDED(retcode))
      { char inconn[4+SQL_MAX_DSN_LENGTH+1];  char outconn[300];  SQLSMALLINT outlength;
        strcpy(inconn, "DSN=");  strcat(inconn, dsn);
        retcode=(*p_SQLDriverConnect)(hDbc, (SQLHWND)hdlg, (SQLCHAR *)inconn, SQL_NTS, (SQLCHAR *)outconn, sizeof(outconn), &outlength, SQL_DRIVER_COMPLETE);  // SQL_DRIVER_COMPLETE_REQUIRED disables some usefull fields
	      if (SQL_SUCCEEDED(retcode))
          (*p_SQLDisconnect)(hDbc);
        else if (retcode==SQL_NO_DATA)
          strcpy(text, "Connection cancelled.");
        else
          (*p_SQLError)(hEnv, hDbc, SQL_NULL_HSTMT, state, &native, (SQLCHAR*)text, sizeof(text), NULL);
        (*p_SQLFreeHandle)(SQL_HANDLE_DBC, hDbc);
      }
      else
        (*p_SQLError)(hEnv, hDbc, SQL_NULL_HSTMT, state, &native, (SQLCHAR*)text, sizeof(text), NULL);
      (*p_SQLFreeHandle)(SQL_HANDLE_ENV, hEnv);
      if (*text)
        MessageBox(hdlg, text, "Error", MB_OK | MB_ICONEXCLAMATION);
      else
        MessageBox(hdlg, "Connection OK.", "Message", MB_OK | MB_ICONINFORMATION);
    }
    else MessageBox(hdlg, "Environment handle not allocated.", "Error", MB_OK | MB_ICONEXCLAMATION);
  }
  FreeLibrary(hInst);
}

/* ConfigDlgProc -----------------------------------------------------------
  Description:  Manage add data source name dialog
  Input      :  hdlg --- Dialog window handle
                wMsg --- Message
                wParam - Message parameter
                lParam - Message parameter
  Output     :  TRUE if message processed, FALSE otherwise
--------------------------------------------------------------------------*/
static INT_PTR CALLBACK ConfigDlgProc
    (HWND hdlg, UINT wMsg, WPARAM wParam, LPARAM lParam)
{ switch (wMsg) {
    case WM_INITDIALOG:
    { LPSETUPDLG lpsetupdlg;  LPCSTR lpszDSN;
      SetWindowLongPtr(hdlg, DWLP_USER, lParam);
      CenterDialog(hdlg);             
     // make and show the version info: 
      { char buf[100];
        LoadString(s_hModule, DRIVER_VERSION_STR, buf, sizeof(buf)-50);
        strcat(buf, " " SERVER_FILE_NAME " " WB_VERSION_STRING);
        SetDlgItemText(hdlg, IDC_DRIVER_VERS, buf);
      }
      lpsetupdlg = (LPSETUPDLG)lParam;
      lpszDSN    = lpsetupdlg->aAttr[KEY_DSN].szAttr;
      // Initialize dialog fields
      // NOTE: Values supplied in the attribute string will always
      //               override settings in ODBC.INI
      SetDlgItemText(hdlg, IDC_DSNAME, lpszDSN);
     // description:
      if (!lpsetupdlg->aAttr[KEY_DESC].fSupplied)
        SQLGetPrivateProfileString(lpszDSN, INI_KDESC,
              EMPTYSTR, lpsetupdlg->aAttr[KEY_DESC].szAttr,
              sizeof(lpsetupdlg->aAttr[KEY_DESC].szAttr), ODBC_INI);
      SetDlgItemText(hdlg, IDC_DESC, lpsetupdlg->aAttr[KEY_DESC].szAttr);
     // server name:
      list_server_names(GetDlgItem(hdlg, IDC_OPTION1));
      if (!lpsetupdlg->aAttr[KEY_OPT1].fSupplied)
      { SQLGetPrivateProfileString(lpszDSN, CONFIG_PATH8, EMPTYSTR, lpsetupdlg->aAttr[KEY_OPT1].szAttr, sizeof(lpsetupdlg->aAttr[KEY_OPT1].szAttr), ODBC_INI);
#if WBVERS>=900
        if (!*lpsetupdlg->aAttr[KEY_OPT1].szAttr)
          SQLGetPrivateProfileString(lpszDSN, CONFIG_PATH9, EMPTYSTR, lpsetupdlg->aAttr[KEY_OPT1].szAttr, sizeof(lpsetupdlg->aAttr[KEY_OPT1].szAttr), ODBC_INI);
#endif
      }
      int sel = (int)SendDlgItemMessage(hdlg, IDC_OPTION1, CB_FINDSTRINGEXACT, -1, (LPARAM)lpsetupdlg->aAttr[KEY_OPT1].szAttr);
      if (sel!=CB_ERR)
        SendDlgItemMessage(hdlg, IDC_OPTION1, CB_SETCURSEL, sel, 0);
      else SetDlgItemText(hdlg, IDC_OPTION1, lpsetupdlg->aAttr[KEY_OPT1].szAttr);
     // application name:
      if (!lpsetupdlg->aAttr[KEY_OPT2].fSupplied)
      { SQLGetPrivateProfileString(lpszDSN, SCHEMANAME8, EMPTYSTR, lpsetupdlg->aAttr[KEY_OPT2].szAttr, sizeof(lpsetupdlg->aAttr[KEY_OPT2].szAttr), ODBC_INI);
#if WBVERS>=900
        if (!*lpsetupdlg->aAttr[KEY_OPT2].szAttr)
          SQLGetPrivateProfileString(lpszDSN, SCHEMANAME9, EMPTYSTR, lpsetupdlg->aAttr[KEY_OPT2].szAttr, sizeof(lpsetupdlg->aAttr[KEY_OPT2].szAttr), ODBC_INI);
#endif
      }
      SetDlgItemText(hdlg, IDC_OPTION2, lpsetupdlg->aAttr[KEY_OPT2].szAttr);

      // Set Translation fields
      if (!lpsetupdlg->aAttr[KEY_TRANSDLL].fSupplied)
              SQLGetPrivateProfileString(lpsetupdlg->aAttr[KEY_DSN].szAttr,
                      szTranslateDLL, EMPTYSTR,
                      lpsetupdlg->aAttr[KEY_TRANSDLL].szAttr,
                      sizeof(lpsetupdlg->aAttr[KEY_TRANSDLL].szAttr), ODBC_INI);
      if (!lpsetupdlg->aAttr[KEY_TRANSOPTION].fSupplied)
              SQLGetPrivateProfileString(lpsetupdlg->aAttr[KEY_DSN].szAttr,
                      szTranslateOption, EMPTYSTR,
                      lpsetupdlg->aAttr[KEY_TRANSOPTION].szAttr,
                      sizeof(lpsetupdlg->aAttr[KEY_TRANSOPTION].szAttr), ODBC_INI);
      if (!lpsetupdlg->aAttr[KEY_TRANSNAME].fSupplied)
        if (!SQLGetPrivateProfileString(lpsetupdlg->aAttr[KEY_DSN].szAttr,
                szTranslateName, EMPTYSTR,
                lpsetupdlg->aAttr[KEY_TRANSNAME].szAttr,
                sizeof(lpsetupdlg->aAttr[KEY_TRANSNAME].szAttr), ODBC_INI))
          if (*lpsetupdlg->aAttr[KEY_TRANSDLL].szAttr)
            lstrcpy (lpsetupdlg->aAttr[KEY_TRANSNAME].szAttr, szUnkTrans);

#if WBVERS<810
      SetDlgItemText(hdlg, IDC_TRANS_NAME, lpsetupdlg->aAttr[KEY_TRANSNAME].szAttr);
#endif
     // DSN state & limits:
      if (lpsetupdlg->fDefault)
      { EnableWindow(GetDlgItem(hdlg, IDC_DSNAME), FALSE);
        EnableWindow(GetDlgItem(hdlg, IDC_DSNAMETEXT), FALSE);
      }
      else
        SendDlgItemMessage(hdlg, IDC_DSNAME,
                          EM_LIMITTEXT, (WPARAM)(MAXDSNAME-1), 0L);
      SendDlgItemMessage(hdlg, IDC_DESC,
              EM_LIMITTEXT, (WPARAM)(MAXDESC-1), 0L);
      return TRUE;                                            // Focus was not set
  }

  case WM_COMMAND:
    switch (GET_WM_COMMAND_ID(wParam, lParam)) 
    { case IDC_DSNAME:  // Enable/disable the OK button
        if (GET_WM_COMMAND_CMD(wParam, lParam) == EN_CHANGE)
        { char    szItem[MAXDSNAME];              // Edit control text
          EnableWindow(GetDlgItem(hdlg, IDOK),
            GetDlgItemText(hdlg, IDC_DSNAME, szItem, sizeof(szItem)));
          return TRUE;
        }
        break;
#if WBVERS<810
      // Translator selection
      case IDC_SELECT:
      { WORD cbNameOut, cbPathOut;
        unsigned long vOption;
        LPSETUPDLG lpsetupdlg;

        lpsetupdlg = (LPSETUPDLG)GetWindowLong(hdlg, DWL_USER);
        GetDlgItemText (hdlg, IDC_TRANS_NAME,
                lpsetupdlg->aAttr[KEY_TRANSNAME].szAttr,
                sizeof (lpsetupdlg->aAttr[KEY_TRANSNAME].szAttr));
        vOption = atol (lpsetupdlg->aAttr[KEY_TRANSOPTION].szAttr);
        if (SQLGetTranslator (hdlg,
                lpsetupdlg->aAttr[KEY_TRANSNAME].szAttr,
                sizeof (lpsetupdlg->aAttr[KEY_TRANSNAME].szAttr)-1,
                &cbNameOut,
                lpsetupdlg->aAttr[KEY_TRANSDLL].szAttr,
                sizeof (lpsetupdlg->aAttr[KEY_TRANSDLL].szAttr)-1,
                &cbPathOut, &vOption))
        {
                if (cbNameOut == 0)
                {       //      No translator selected
                        *lpsetupdlg->aAttr[KEY_TRANSNAME].szAttr = 0;
                        *lpsetupdlg->aAttr[KEY_TRANSDLL].szAttr = 0;
                        vOption = 0;
                }
                SetDlgItemText (hdlg, IDC_TRANS_NAME,
                        lpsetupdlg->aAttr[KEY_TRANSNAME].szAttr);
                ltoa (vOption, lpsetupdlg->aAttr[KEY_TRANSOPTION].szAttr,
                        10);
        }
        return TRUE;
      }
#endif

      case IDOK:
      case IDTEST:
      { LPSETUPDLG lpsetupdlg;
        lpsetupdlg = (LPSETUPDLG)GetWindowLongPtr(hdlg, DWLP_USER);
       // Retrieve dialog values
        if (!lpsetupdlg->fDefault)
          GetDlgItemText(hdlg, IDC_DSNAME, lpsetupdlg->aAttr[KEY_DSN].szAttr,
                         sizeof(lpsetupdlg->aAttr[KEY_DSN].szAttr));
        GetDlgItemText(hdlg, IDC_DESC,
                lpsetupdlg->aAttr[KEY_DESC].szAttr,
                sizeof(lpsetupdlg->aAttr[KEY_DESC].szAttr));
        GetDlgItemText(hdlg, IDC_OPTION1,
                lpsetupdlg->aAttr[KEY_OPT1].szAttr,
                sizeof(lpsetupdlg->aAttr[KEY_OPT1].szAttr));
        GetDlgItemText(hdlg, IDC_OPTION2,
                lpsetupdlg->aAttr[KEY_OPT2].szAttr,
                sizeof(lpsetupdlg->aAttr[KEY_OPT2].szAttr));
        SetDSNAttributes(hdlg, lpsetupdlg);  // Update ODBC.INI
        if (GET_WM_COMMAND_ID(wParam, lParam) == IDTEST) 
        { test_conn(hdlg, lpsetupdlg->aAttr[KEY_DSN].szAttr);
          return TRUE;
        }
        EndDialog(hdlg, TRUE);  return TRUE;
      }
      case IDCANCEL:       
        EndDialog(hdlg, TRUE);  return TRUE;
    }
    break;
  }
  return FALSE;
}


/* ParseAttributes ---------------------------------------------------------
  Description:  Parse attribute string moving values into the aAttr array
  Input      :  lpszAttributes - Pointer to attribute string
  Output     :  None (global aAttr normally updated)
--------------------------------------------------------------------------*/
void WINAPI ParseAttributes(LPCSTR lpszAttributes, LPSETUPDLG lpsetupdlg)
{
        LPCSTR  lpsz;
        LPCSTR  lpszStart;
        char    aszKey[MAXKEYLEN];
        int             iElement;
        int             cbKey;

        for (lpsz=lpszAttributes; *lpsz; lpsz++)
        {  //  Extract key name (e.g., DSN), it must be terminated by an equals
                lpszStart = lpsz;
                for (;; lpsz++)
                {
                        if (!*lpsz)
                                return;         // No key was found
                        else if (*lpsz == '=')
                                break;          // Valid key found
                }
                // Determine the key's index in the key table (-1 if not found)
                iElement = -1;
                cbKey    = (int)(lpsz - lpszStart);
                if (cbKey < sizeof(aszKey))
                {
                        register int j;

                        _fmemcpy(aszKey, lpszStart, cbKey);
                        aszKey[cbKey] = '\0';
                        for (j = 0; *s_aLookup[j].szKey; j++)
                        {
                                if (!lstrcmpi(s_aLookup[j].szKey, aszKey))
                                {
                                        iElement = s_aLookup[j].iKey;
                                        break;
                                }
                        }
                }

                // Locate end of key value
                lpszStart = ++lpsz;
                for (; *lpsz; lpsz++);

                // Save value if key is known
                // NOTE: This code assumes the szAttr buffers in aAttr have been
                //         zero initialized
                if (iElement >= 0)
                {
                        lpsetupdlg->aAttr[iElement].fSupplied = TRUE;
                        _fmemcpy(lpsetupdlg->aAttr[iElement].szAttr,
                                lpszStart,
                                MIN(lpsz-lpszStart+1, sizeof(lpsetupdlg->aAttr[0].szAttr)-1));
                }
        }
        return;
}


/* SetDSNAttributes --------------------------------------------------------
  Description:  Write data source attributes to ODBC.INI
  Input      :  hwnd - Parent window handle (plus globals)
  Output     :  TRUE if successful, FALSE otherwise
--------------------------------------------------------------------------*/
BOOL WINAPI SetDSNAttributes(HWND hwndParent, LPSETUPDLG lpsetupdlg)
{
        LPCSTR lpszDSN = lpsetupdlg->aAttr[KEY_DSN].szAttr;
       // Validate arguments
        if (lpsetupdlg->fNewDSN && !*lpszDSN) return FALSE;
       // Write the data source name
        if (!SQLWriteDSNToIni(lpszDSN, lpsetupdlg->lpszDrvr))
        { if (hwndParent)   /* report error to the user */
          { char  szBuf[MAXPATHLEN];  char  szMsg[MAXPATHLEN];
            LoadString(s_hModule, IDS_BADDSN, szBuf, sizeof(szBuf));
            sprintf(szMsg, szBuf, lpszDSN);
            LoadString(s_hModule, IDS_MSGTITLE, szBuf, sizeof(szBuf));
            MessageBox(hwndParent, szMsg, szBuf, MB_ICONEXCLAMATION | MB_OK);
          }
          return FALSE;
        }

        // Update ODBC.INI
        // Save the value if the data source is new, if it was edited, or if
        // it was explicitly supplied
        if (hwndParent || lpsetupdlg->aAttr[KEY_DESC].fSupplied )
                SQLWritePrivateProfileString(lpszDSN,
                        INI_KDESC,
                        lpsetupdlg->aAttr[KEY_DESC].szAttr,
                        ODBC_INI);

        if (hwndParent || lpsetupdlg->aAttr[KEY_OPT1].fSupplied )
                SQLWritePrivateProfileString(lpszDSN, CONFIG_PATH, lpsetupdlg->aAttr[KEY_OPT1].szAttr, ODBC_INI);

        if (hwndParent || lpsetupdlg->aAttr[KEY_OPT2].fSupplied )
                SQLWritePrivateProfileString(lpszDSN, SCHEMANAME,lpsetupdlg->aAttr[KEY_OPT2].szAttr, ODBC_INI);

        if (hwndParent || lpsetupdlg->aAttr[KEY_TRANSNAME].fSupplied )
        {
                SQLWritePrivateProfileString(lpszDSN, szTranslateName, *lpsetupdlg->aAttr[KEY_TRANSNAME].szAttr ? lpsetupdlg->aAttr[KEY_TRANSNAME].szAttr: NULL, ODBC_INI);
                SQLWritePrivateProfileString(lpszDSN, szTranslateDLL, *lpsetupdlg->aAttr[KEY_TRANSNAME].szAttr ? lpsetupdlg->aAttr[KEY_TRANSDLL].szAttr: NULL, ODBC_INI);
                SQLWritePrivateProfileString(lpszDSN, szTranslateOption, *lpsetupdlg->aAttr[KEY_TRANSNAME].szAttr ? lpsetupdlg->aAttr[KEY_TRANSOPTION].szAttr: NULL, ODBC_INI);
        }

        // If the data source name has changed, remove the old name
        if (lpsetupdlg->aAttr[KEY_DSN].fSupplied &&
                lstrcmpi(lpsetupdlg->szDSN, lpszDSN))
        {
                SQLRemoveDSNFromIni(lpsetupdlg->szDSN);
        }
        return TRUE;
}

///////////////////////////////////////////////////////////////////////////
#ifdef STOP
BOOL ConfigDriver (HWND	hwndParent,	WORD fRequest, LPCSTR	lpszDriver,
	LPCSTR lpszArgs, LPSTR	lpszMsg, WORD	cbMsgMax,	WORD * pcbMsgOut)
{
  switch (fRequest)
  { case ODBC_INSTALL_DRIVER:
    { char key[200];
      strcpy(key, 
      break;
    }
    case ODBC_REMOVE_DRIVER:
      break;
  }
  if (pcbMsgOut!=NULL) *pcbMsgOut=0;
  if (lpszMsg!=NULL && cbMsgMax) *lpszMsg=0;
  return TRUE;
}
#endif

