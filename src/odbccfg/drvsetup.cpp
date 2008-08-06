/**************************************************
 * This code is based on one created by Peter Harvey @ CodeByDesign.
 * It was created by Tomas Zellerin at software602.cz.
 * Updated by Janus Drozd at software602.cz.
 * Released under LGPL 18.FEB.99
 **************************************************/
extern "C" {
#include <odbcinstext.h>
}
#include "wbvers.h"
#include "string.h"  // size_t
#include "general.h"

#ifndef FALSE
#define FALSE 0
#endif
#ifndef TRUE
#define TRUE 1
#endif
#include <stdlib.h>

#include "regstr.h"
#include "regstr.cpp"
#include "uxprofile.c"
#define MAX_PATH 256
#include "enumsrv.c"
#define MAX_SERVERS 50

static char server_names[MAX_SERVERS][31+1];

extern "C" int ODBCINSTGetProperties( HODBCINSTPROPERTY hLastProperty )
{ int srvcnt=0;
  char ** ptrs = (char**)malloc((MAX_SERVERS+1)*sizeof(char*));
  if (ptrs)
  { prep_regdb_access(); 
    t_enum_srv es(t_enum_srv::ENUM_SRV_BOTH, CURRENT_FIL_VERSION_STR);
    while (srvcnt<MAX_SERVERS && es.next(server_names[srvcnt]))
    { ptrs[srvcnt] = server_names[srvcnt];
      srvcnt++;
    }  
    ptrs[srvcnt]=NULL;
  }
  
	hLastProperty->pNext 				= (HODBCINSTPROPERTY)malloc( sizeof(ODBCINSTPROPERTY) );
	hLastProperty 						= hLastProperty->pNext;
	memset( hLastProperty, 0, sizeof(ODBCINSTPROPERTY) );
	hLastProperty->nPromptType			= ODBCINST_PROMPTTYPE_COMBOBOX;
	hLastProperty->aPromptData			= ptrs;
	strncpy( hLastProperty->szName, "SQLServerName", INI_MAX_PROPERTY_NAME );
	strncpy( hLastProperty->szValue, "", INI_MAX_PROPERTY_VALUE );

	hLastProperty->pNext 				= (HODBCINSTPROPERTY)malloc( sizeof(ODBCINSTPROPERTY) );
	hLastProperty 						= hLastProperty->pNext;
	memset( hLastProperty, 0, sizeof(ODBCINSTPROPERTY) );
    hLastProperty->nPromptType			= ODBCINST_PROMPTTYPE_TEXTEDIT;
	strncpy( hLastProperty->szName, "ApplicationName", INI_MAX_PROPERTY_NAME );
	strncpy( hLastProperty->szValue, "", INI_MAX_PROPERTY_VALUE );

	return 1;
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Attribute key indexes (into an array of Attr structs, see below)
#define KEY_DSN                 0
#define KEY_DESC                1
#define KEY_OPT1                2
#define KEY_OPT2                3
#define KEY_TRANSNAME   4
#define KEY_TRANSOPTION 5
#define KEY_TRANSDLL    6
#define NUMOFKEYS               7                               // Number of keys supported

const char INI_KDESC   []="Description";  // Data source description
const char CONFIG_PATH[] = "SQLServerName";
const char SCHEMANAME[] = "ApplicationName";
const char szTranslateName[] = "TranslationName";
const char szTranslateDLL[] = "TranslationDLL";
const char szTranslateOption[] = "TranslationOption";
const char szUnkTrans[] = "Unknown Translator";
const char INI_SDEFAULT[] = "Default";    // Default data source name
#define ODBC_INI "odbc.ini"

#define MAXKEYLEN       (15+1)            // Max keyword length
#define MAXPATHLEN      (255+1)           // Max path length
#define MAXDSNAME       (32+1)            // Max data source name length
#define MIN(x,y)      ((x) < (y) ? (x) : (y))
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
} Attr, * LPAttr;

// NOTE:  All these are used by the dialog procedures
typedef struct tagSETUPDLG {
        HWND    hwndParent;         // Parent window handle
        LPCSTR  lpszDrvr;           // Driver description
        Attr    aAttr[NUMOFKEYS];   // Attribute array
        char    szDSN[MAXDSNAME];   // Original data source name
        BOOL    fNewDSN;            // New data source flag
        BOOL    fDefault;           // Default data source flag

} SETUPDLG, *LPSETUPDLG;



/* ParseAttributes ---------------------------------------------------------
  Description:  Parse attribute string moving values into the aAttr array
  Input      :  lpszAttributes - Pointer to attribute string
  Output     :  None (global aAttr normally updated)
--------------------------------------------------------------------------*/
void ParseAttributes(LPCSTR lpszAttributes, LPSETUPDLG lpsetupdlg)
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

                        memcpy(aszKey, lpszStart, cbKey);
                        aszKey[cbKey] = '\0';
                        for (j = 0; *s_aLookup[j].szKey; j++)
                        {
                                if (!strcasecmp(s_aLookup[j].szKey, aszKey))
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
                        memcpy(lpsetupdlg->aAttr[iElement].szAttr,
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
BOOL SetDSNAttributes(HWND hwndParent, LPSETUPDLG lpsetupdlg)
{
        LPCSTR lpszDSN = lpsetupdlg->aAttr[KEY_DSN].szAttr;
       // Validate arguments
        if (lpsetupdlg->fNewDSN && !*lpszDSN) return FALSE;
       // Write the data source name
        if (!SQLWriteDSNToIni(lpszDSN, lpsetupdlg->lpszDrvr))
          return FALSE;

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
                strcasecmp(lpsetupdlg->szDSN, lpszDSN))
        {
                SQLRemoveDSNFromIni(lpsetupdlg->szDSN);
        }
        return TRUE;
}

extern "C" BOOL ConfigDSN(HWND hwndParent, WORD fRequest, const char * lpszDriver, const char * lpszAttributes)
{       BOOL  fSuccess;                                            // Success/fail flag
 // if user-interface is tested, return FAIL: ODBCConfig will use the property list instead 
  if (hwndParent) return FALSE;
 // non-interactive processing: used when creating a new data source 
  int atrlen=0;
  if (lpszAttributes)
  { while (lpszAttributes[atrlen])
      atrlen += strlen(lpszAttributes+atrlen) + 1;
  }
  SETUPDLG setupdlg, * lpsetupdlg = &setupdlg;
  memset(lpsetupdlg, 0, sizeof(SETUPDLG));

       // Parse attribute string
        if (lpszAttributes) ParseAttributes(lpszAttributes, lpsetupdlg);

       // Save original data source name
        if (lpsetupdlg->aAttr[KEY_DSN].fSupplied)
                strcpy(lpsetupdlg->szDSN, lpsetupdlg->aAttr[KEY_DSN].szAttr);
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
          lpsetupdlg->lpszDrvr     = lpszDriver;
          lpsetupdlg->fNewDSN      = (ODBC_ADD_DSN == fRequest);
          lpsetupdlg->fDefault     =
                  !strcasecmp(lpsetupdlg->aAttr[KEY_DSN].szAttr, INI_SDEFAULT);

          if (lpsetupdlg->aAttr[KEY_DSN].fSupplied)
            fSuccess = SetDSNAttributes(hwndParent, lpsetupdlg);
          else
            fSuccess = FALSE;
        }
        return fSuccess;
}

