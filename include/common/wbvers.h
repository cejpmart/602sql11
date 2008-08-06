// wbvers.h - version information
#ifndef WBVERS
#error WBVERS is not defined!
#endif

#if WBVERS>=1100

#ifdef UNIX  // on MSW conf.h is replaced by the command line defines of preprocessor symbols
#include <conf.h>
#else // MSW
#endif

////////////////////// part depending on conf.h (on Unix) ////////////////////
// Name of the subdirectory for SQL server cerificates downloaded by the client (Unix)
#define CERT_CLIENT_DIR  "." NAME_PREFIX "sql" NAME_SUFFIX
// Name of the SQL server service executable (MSW):
#define SERVICE_FNAME    "\\" NAME_PREFIX "svc" NAME_SUFFIX ".exe"
// Name of the ODBC driver file:
#ifdef WINS
#define ODBC_DRIVER_FILE NAME_PREFIX "odbcw" NAME_SUFFIX ".dll"
#define ODBC_SETUP_FILE  NAME_PREFIX "odbcw" NAME_SUFFIX ".dll"
#else
#define ODBC_DRIVER_FILE  "lib" NAME_PREFIX "odbcw" NAME_SUFFIX ".so"
#define ODBC_SETUP_FILE   "lib" NAME_PREFIX "odbc"  NAME_SUFFIX "S.so" // *S.so is a unixODBC naming convention
#endif

#define WB_VERSION_MAJOR     11
#define WB_VERSION_MINOR     0
#define WB_VERSION_STRING    "11.0"   // used by main window caption
#define WB_VERSION_STRING_EX WB_VERSION_STRING  // used in information windows
#define ENGLISH 1 

// version resource defines:
#define VERS_1   11
#define VERS_2   0
#define VERS_3   2
#define VERS_4   806
#define VERS_STR "11.0.1.0806"

// installation ODBC names:
#define ODBC_DRIVER_NAME NAME_PREFIX "SQL" NAME_SUFFIX " " WB_VERSION_STRING " Driver"

// features:
#define EXTFT
#define XMLFORMS

#elif WBVERS>=810  // 8.1 ////////////////////////////////////////////////////////////////////////////////////////

#define NAME_PREFIX          "602"
#define NAME_SUFFIX          "8"
#define WB_VERSION_MAJOR     8
#define WB_VERSION_MINOR     1
#define WB_VERSION_STRING    "8.1"   // used by main window caption
#define WB_VERSION_STRING_EX "8.1c"  // used in information windows
#include "english.h"

// version resource defines:
#define VERS_1   8
#define VERS_2   1
#define VERS_3   4
#define VERS_4   2
#define VERS_STR "8.1.4.2"     

// installation ODBC names:
#define ODBC_DRIVER_NAME "602SQL 8.1 Driver"
#define ODBC_DRIVER_FILE "602odbc81.dll"
#define ODMA_DRIVER_FILE "602odma81.dll"
#define HELP_FILE_NAME   "602sql8.chm"
#define SERVICE_FNAME    "\\602SVC8.EXE"   // must be in upper case, used when searching
#define CERT_CLIENT_DIR  ".602sql9"

// name of /usr/lib/NNN and /usr/share/NNN subdirs
#define WB_LINUX_SUBDIR_NAME "602sql8"

#ifdef WINS
#define MAIL602
#endif

#else // other versions not supported any more

#endif // version-dependent defines ///////////////////////////////////////////////////////////////////////

// Domain name for gettext
#define GETTEXT_DOMAIN   NAME_PREFIX "gcli" NAME_SUFFIX
// Name of the server executable and base name of the help file
#define SERVER_FILE_NAME NAME_PREFIX "sql"  NAME_SUFFIX
// Name of the main client library:
#define KRNL_LIB_NAME    NAME_PREFIX "krnl" NAME_SUFFIX
// Name of the XML extension library
#define XML_EXT_NAME     NAME_PREFIX "xml"  NAME_SUFFIX

///////////////////////////////////////// konfiguration: /////////////////////////////////////////
#ifdef WINS
#ifndef ODBCDRV
#define CLIENT_PGM   // emb. vars, languages
#if WBVERS<900
#define CLIENT_GUI   // object name cache, object def cache, help file,
#define CLIENT_ODBC  // ODBC client
#endif
#endif
#else // !WINS
#define CLIENT_PGM  // allows to access embedded variables from SQL statements in WBC connectors
#endif

#if WBVERS>=900
#define CLIENT_ODBC_9
#endif

////////////////////////////// a very general part ///////////////////////////////////
#define CURRENT_FIL_VERSION_STR "8"
#define CURRENT_FIL_VERSION_NUM 8
#define CURRENT_APX_VERSION     8
#define FIL_NAME     "wb8.fil"

#define SOFT_ID "SQL11"
