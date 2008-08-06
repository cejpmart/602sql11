/****************************************************************************/
/* cint.cpp - The interface between C language programs and the SQL server  */
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
#include "regstr.h"
#include "opstr.h"
#include "netapi.h"
#if WBVERS<=810
#ifdef WINS                  
#include "wbcapi.h"
#include <ole2.h>
#endif
#include "../secur/wbsecur.h"
#else
#include "wbsecur.h"
#endif
#include "replic.h"
#include "wchar.h"
                           
#if WBVERS<900
#include "kernelrc.h"
#ifdef CLIENT_GUI
#include "wprez.h"
#else
#include "wbprezen.h"
#include "wbprezex.h"
#endif
#endif

#ifdef WINS
#if WBVERS<900
#include <windowsx.h>
#include <shellapi.h>
#include "ansiapi.h"
#endif
#else
#include "stdarg.h"
#endif

#ifdef UNIX
#include <sys/time.h>
#include <unistd.h>
#include <stdlib.h>
#endif
#include "odbc.h"
#include "regstr.cpp"
#include "enumsrv.c"

///////////////////////////// login key administration /////////////////////////////////////////
/* Purpose: Make easier for multiple processes to log in a SQL server. When one process logs in,
   other processes are allowed access with the same user name. */
#define MAX_LOGIN_KEYS 7
//#define TRACE_LOG 1  // traces all login key operations

struct t_server_login_key
{ tobjname server_name;    // valid only if login_key!=0
  uns32    login_key;      // empty iff 0
  t_server_login_key(void) : login_key(0) { }
};

static t_server_login_key process_login_keys[MAX_LOGIN_KEYS];  // pool of valid login keys
static bool site_login_keys = false;
static t_server_login_key * login_keys = process_login_keys;

#ifdef WINS
PSECURITY_DESCRIPTOR pSD;
SECURITY_ATTRIBUTES  SA;
HANDLE hMutex;
char shrlgbuf[150];

/* Initialize a security descriptor. */
static void make_security_descr(void)
{ if (pSD!=NULL) return;
  SA.nLength=sizeof(SECURITY_ATTRIBUTES);
  SA.lpSecurityDescriptor=pSD;
  SA.bInheritHandle=FALSE;
  pSD = (PSECURITY_DESCRIPTOR) LocalAlloc(LPTR, SECURITY_DESCRIPTOR_MIN_LENGTH);   /* defined in WINNT.H */
  SA.nLength=sizeof(SECURITY_ATTRIBUTES);
  SA.lpSecurityDescriptor=pSD;
  SA.bInheritHandle=FALSE;
  if (pSD != NULL)
  { if (InitializeSecurityDescriptor(pSD, SECURITY_DESCRIPTOR_REVISION))  /* defined in WINNT.H */
    /* Add a NULL disc. ACL to the security descriptor. */
 	  if (SetSecurityDescriptorDacl(pSD, TRUE,     /* specifying a disc. ACL  */
          (PACL) NULL, FALSE))  /* not a default disc. ACL */
      /* Add the security descriptor to the file. */
 	    /*if (SetFileSecurity(lpszTestFile, DACL_SECURITY_INFORMATION, pSD)) */
        return; 	/* OK */
    LocalFree((HLOCAL) pSD);  pSD=NULL;
  }
#ifdef TRACE_LOG
  hMutex=CreateMutex(&SA, FALSE, "SharedLogin602SQLMutex");
#endif
}

static void free_security_descr(void)
{ if (pSD!=NULL) { LocalFree((HLOCAL) pSD);  pSD=NULL; } }

void log_it(const char * txt)
{ 
#ifdef TRACE_LOG
  WaitForSingleObject(hMutex, INFINITE);
  FHANDLE hnd=CreateFile("c:\\logshr.txt", GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, FILE_FLAG_RANDOM_ACCESS, (HANDLE)NULL);
  if (FILE_HANDLE_VALID(hnd)) 
  { DWORD wr;  
    SetFilePointer(hnd, 0, NULL, FILE_END);  
    WriteFile(hnd, txt, strlen(txt), &wr, NULL);
    WriteFile(hnd, "\r\n", 2, &wr, NULL);
    CloseFile(hnd);
  }
  ReleaseMutex(hMutex);
#endif
}

#else  // LINUX

#include <sys/ipc.h>
#include <sys/shm.h>
void log_it(const char * txt) { } 

#endif

CFNC DllExport void WINAPI start_lk_sharing(void)
// This version allows the per-process sharing, even among clients using the static and dynamic version of the kernel library.
{ if (site_login_keys) return;
  bool new_file_mapping;
#ifdef WINS
  char mapping_name[50];
  sprintf(mapping_name, "602SQLClientLKPool%u", GetCurrentProcessId());  
  HANDLE hFileMapping = OpenFileMapping(FILE_MAP_WRITE, FALSE, mapping_name);
  if (hFileMapping)
    new_file_mapping=false;
  else
  { new_file_mapping=true;
    make_security_descr();
    hFileMapping = CreateFileMapping(INVALID_HANDLE_VALUE, &SA, PAGE_READWRITE, 0, sizeof(process_login_keys), mapping_name);
    free_security_descr();
    if (!hFileMapping) return;
  }
  login_keys = (t_server_login_key *)MapViewOfFile(hFileMapping, FILE_MAP_WRITE, 0, 0, 0);
  if (login_keys==NULL) { CloseHandle(hFileMapping);  login_keys = process_login_keys;  return; }
#else
  static const key_t wbkey=0xaaacb604 ^ getpid();
  int id=shmget(wbkey, 1000, 0666);
  if (-1==id) id=shmget(wbkey, 1000, IPC_CREAT + 0666);
  if (-1==id){
	  perror("shmget"); // problems on RH 7.1
	  return;
  }
  login_keys = (t_server_login_key *)shmat(id, NULL, 0);
  if (login_keys == (t_server_login_key *)-1){
	  perror("shmat");
	  login_keys = process_login_keys;
	  return;
  }
  shmid_ds shmid;
  shmctl(id, IPC_STAT, &shmid);
  new_file_mapping = (shmid.shm_nattch==1);
#endif
 // init the shared data, if created:
  if (new_file_mapping) 
    memcpy(login_keys, process_login_keys, sizeof(process_login_keys));
  site_login_keys=true;
}

CFNC DllExport void WINAPI store_login_key8(const char * server_name, uns32 key) // adds a login key to the pool
{ unsigned i;
  for (i=0;  i<MAX_LOGIN_KEYS;  i++)
    if (!login_keys[i].login_key) break;
  if (i==MAX_LOGIN_KEYS) // if there is no empty cell, overwrite a key of the same server
    for (i=0;  i<MAX_LOGIN_KEYS;  i++)
      if (!strcmp(login_keys[i].server_name, server_name))
      { 
#ifdef TRACE_LOG
        sprintf(shrlgbuf, "%u: Replacing key %u for server %s", GetCurrentThreadId(), login_keys[i].login_key, server_name);
        log_it(shrlgbuf);
#endif
        break;
      }
  if (i<MAX_LOGIN_KEYS)
    { login_keys[i].login_key=key;  strcpy(login_keys[i].server_name, server_name); }
#ifdef TRACE_LOG
  if (i<MAX_LOGIN_KEYS)
  { sprintf(shrlgbuf, "%u: Storing   key %u for server %s", GetCurrentThreadId(), key, server_name);
    log_it(shrlgbuf);
  }
  else 
  { sprintf(shrlgbuf, "%u: Lost      key %u for server %s", GetCurrentThreadId(), key, server_name);
    log_it(shrlgbuf);
  }
#endif
}

CFNC DllExport void WINAPI discard_login_key8(uns32 key)  // removes the login key from the pool
{ for (unsigned i=0;  i<MAX_LOGIN_KEYS;  i++)
    if (login_keys[i].login_key==key)
    { login_keys[i].login_key=0;  
#ifdef TRACE_LOG
      sprintf(shrlgbuf, "%u: Removed   key %u for server %s", GetCurrentThreadId(), key, login_keys[i].server_name);
      log_it(shrlgbuf);
#endif
      break; 
    }
}

CFNC DllExport uns32 WINAPI get_login_key8(const char * server_name, int & i)  // retrieves a login key from the pool
{ i++;  // i was before the start index on entry
  for (;  i<MAX_LOGIN_KEYS;  i++)
    if (login_keys[i].login_key) 
      if (!strcmp(login_keys[i].server_name, server_name))
      {
#ifdef TRACE_LOG
        sprintf(shrlgbuf, "%u: Retrieved   key %u for server %s", GetCurrentThreadId(), login_keys[i].login_key, server_name);
        log_it(shrlgbuf);
#endif
        return login_keys[i].login_key;
      }
#ifdef TRACE_LOG
  sprintf(shrlgbuf, "%u: Key not found for server %s", GetCurrentThreadId(), server_name);
  log_it(shrlgbuf);
#endif
  return 0;
}

//////////////////////////////////////////// server management interface ///////////////////////////////////////
static BOOL WINAPI cond_send(cdp_t cdp);
#ifdef LINUX
#include <errno.h>
#include <sys/file.h>
#include <prefix.h>
#endif

void qstrcpy(char * dest, const char * src)
{ do
  { *dest=*src;  dest++;
    if (*src=='\'' || *src=='"') *(dest++)=*src;
  } while (*(src++));
}

char * append(char * dest, const char * src)  // appends with single PATH_SEPARATOR
{ int len=(int)strlen(dest);
  if (!len || dest[len-1]!=PATH_SEPARATOR) dest[len++]=PATH_SEPARATOR;
  if (*src==PATH_SEPARATOR) src++;
  strcpy(dest+len, src);
  return dest;
}

#ifdef WINS
BOOL start_process_detached(char * command, int show_type)
{ STARTUPINFO si;  PROCESS_INFORMATION pi;
  memset(&si, 0, sizeof STARTUPINFO);
  memset(&pi, 0, sizeof PROCESS_INFORMATION);
  si.cb = sizeof(STARTUPINFO);
  si.dwFlags = STARTF_USESHOWWINDOW;
  si.wShowWindow = show_type; 
  if (!CreateProcess(NULL, // No module name (use command line). 
        command, // Command line. 
        NULL,             // Process handle not inheritable. 
        NULL,             // Thread handle not inheritable. 
        FALSE,            // Set handle inheritance to FALSE. 
#if WBVERS>=1100        
        CREATE_NEW_CONSOLE|
#endif        
        CREATE_DEFAULT_ERROR_MODE|CREATE_NEW_PROCESS_GROUP, // creation flags. 
        NULL,             // Use parent's environment block.
        NULL,             // Use parent's starting directory. 
        &si,              // Pointer to STARTUPINFO structure.
        &pi))             // Pointer to PROCESS_INFORMATION structure.
     return FALSE; // failed
 // Close process and thread handles. 
  CloseHandle(pi.hProcess);
  CloseHandle(pi.hThread);
  return TRUE;
}
#endif

CFNC DllKernel BOOL WINAPI get_server_path(char * pathname)
// Returns the server path from the same directory as client or with the same version.
#ifdef WINS
// Encloses the pathname with "..."
{ *pathname='\"';
  GetModuleFileName(hKernelLibInst, pathname+1, MAX_PATH);
  size_t i=strlen(pathname);  while (i && pathname[i-1]!=PATH_SEPARATOR) i--;
  strcpy(pathname+i, SERVER_FILE_NAME ".exe");
  WIN32_FIND_DATA data;
  HANDLE hnd=FindFirstFile(pathname+1, &data);
  if (hnd!=INVALID_HANDLE_VALUE) goto found;
 // server not found in the same directory
  LONG res;  char key[150];  HKEY hKey;
  strcpy(key, WB_inst_key);  strcat(key, "\\" WB_VERSION_STRING);
  res=RegOpenKeyEx(HKEY_LOCAL_MACHINE, key, 0, KEY_READ_EX, &hKey);
  if (res==ERROR_SUCCESS)
  { DWORD key_len=MAX_PATH-1;
    res=RegQueryValueEx(hKey, Path_str, NULL, NULL, (BYTE*)pathname+1, &key_len);
    if (res==ERROR_SUCCESS) 
    { strcat(pathname, PATH_SEPARATOR_STR SERVER_FILE_NAME ".exe");
      hnd=FindFirstFile(pathname+1, &data);
      if (hnd!=INVALID_HANDLE_VALUE)
        { RegCloseKey(hKey);  goto found; }
    }
    RegCloseKey(hKey);
  }
  return FALSE;
 found:
  FindClose(hnd);  
  strcat(pathname, "\"");  
  return TRUE;
}
#else
// The PREFIX is the module directory and /../
// This module is in a library located in the prefix/lib/602sqlVERS directory
// Thus the corresponding binaries are in the PREFIX/../bin directory
{ struct stat sb;
  sprintf(pathname, "%s/../bin/%s", PREFIX, SERVER_FILE_NAME);
  return stat(pathname, &sb) != -1;
}
#endif


CFNC DllKernel int WINAPI srv_Get_state_local(const char * server_name)
/* Return values: -1 remote server (not returned on MSW), 0 local & can start,               1 local & running, 
                   2 local, runnig, no IPC (MSW only),    3 local & cannot start (Unix only) */
{ 
#ifdef WINS
  return DetectLocalServer(server_name);
#else
  char path[MAX_PATH];
  if (!GetDatabaseString(server_name, "PATH",  path, sizeof(path), false) &&
      !GetDatabaseString(server_name, "PATH1", path, sizeof(path), false) &&
      !GetDatabaseString(server_name, "PATH",  path, sizeof(path), true ) &&
      !GetDatabaseString(server_name, "PATH1", path, sizeof(path), true ))
    return -1;  // server is not local
  if (!*path)
    return -1;  // server is not local
  append(path, FIL_NAME);
  int fh = open(path, O_RDWR);
  if (fh == -1)  // file opens even if server is running (RH 7.3)
  { fh=errno;
    return errno==ENOENT ? 0 /* before 1st run*/ : errno==EACCES ? 3 /*no right to open*/ : 0;  // try it
  }
  if (flock(fh, LOCK_EX|LOCK_NB))
  { close(fh);  
    return 1;   // running, because opened and cannot lock
  }
  close(fh);
  return 0; // not running
#endif
}

CFNC DllKernel int WINAPI srv_Start_server_local(const char * server_name, int mode, const char * password)
// mode<=0: run as service/daemon (must find the name of the service)
// mode>=1000: dependent server, otherwise mode is the ShowWindow parameter value
{ int result = KSE_WINEXEC;
  bool start_dependent_server=false;
  if (mode>=1000)
  { start_dependent_server=true;
    mode-=1000;
  }  
#ifdef LINUX
  // problem: when the server started in this way is stopped, 
  //  the xterm proces is invisible but continues to exists.
  char cmd[50+MAX_PATH+OBJ_NAME_LEN+MAX_FIL_PASSWORD];
  const char * term = getenv("TERM");
  if (!term) term="xterm";
  strcpy(cmd, term);
  strcat(cmd, " -e ");
  get_server_path(cmd+strlen(cmd));
  if (mode<0) strcat(cmd, " -d");  // daemon
  strcat(cmd, " -n\"");
  strcat(cmd, server_name);
  strcat(cmd, "\"");
  if (password && *password)  // cannot pass empty password on Linux, command line analysis does not allow this
  { strcat(cmd, " -p\"");
    strcat(cmd, password);
    strcat(cmd, "\"");
  }
  if (!fork())
  { //get_server_path(cmd);
    //execl(cmd, "-n", server_name, NULL);  -- bad, does not have own termina window
    system(cmd);
    //get_server_path(cmd);  
    //execlp("xterm", "-e", cmd, "-n", server_name, NULL); -- does not work with /../ in the path!
    // execl does not return on success, I do not know if system returns
    exit(1);
  }
  result=KSE_OK;
#else 
  if (mode<0)
  { char service_name[MAX_SERVICE_NAME+1];
    if (!srv_Get_service_name(server_name, service_name))
      return KSE_WINEXEC;
    SERVICE_STATUS ssStatus;  
    SC_HANDLE hSCM = ::OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (hSCM)
    { SC_HANDLE hService = ::OpenService(hSCM, service_name, SERVICE_ALL_ACCESS);
      if (hService)
      { BOOL startres;
        if (password)
        { char paspar[2+MAX_FIL_PASSWORD+1];
          sprintf(paspar, "-p%s", password);  // do not use "..", the service does not recognize it!!
          const char * pars[2];
          pars[0]=service_name;
          pars[1]=paspar;
          startres=StartService(hService, 2, pars);
        }  
        else
          startres=StartService(hService, 0, NULL);
        if (startres)
        { do // Check the status until the service status is no longer "start pending".
          { Sleep(200);
            if (!QueryServiceStatus(hService, &ssStatus)) break;
          } while (ssStatus.dwCurrentState == SERVICE_START_PENDING);
          if (ssStatus.dwCurrentState == SERVICE_RUNNING) 
            result=KSE_OK;
        }
        ::CloseServiceHandle(hService);
      }
      ::CloseServiceHandle(hSCM);
    }
  }  
  else
  { char cmdline[MAX_PATH+OBJ_NAME_LEN+100+20];  
    if (!get_server_path(cmdline)) 
      return KSE_WINEXEC;  // no local server installation found
    if (server_name && *server_name)
    { 
#if WBVERS<900
      strcat(cmdline, " &\"");
#else
      strcat(cmdline, " -n\"");
#endif
      qstrcpy(cmdline+strlen(cmdline), server_name);
      if (server_name[1]==':' && server_name[strlen(server_name)-1]!='\\')
        strcat(cmdline, "\\");
      strcat(cmdline, "\"");
    }
    if (password)
    { strcat(cmdline, " -p\"");
      qstrcpy(cmdline+strlen(cmdline), password);
      strcat(cmdline, "\"");
    }
    if (start_dependent_server)
      strcat(cmdline, " /!");  /* necessary to distinguish the "dependent server" and to return the error condition */
    if (mode>16 || mode<0) mode=SW_MINIMIZE;
   // start server:
#if WBVERS<900
#ifdef CLIENT_GUI
    status_text(STBR_LINK_START);
#endif
#endif
    if (start_process_detached(cmdline, mode)) result = KSE_OK;
  }
#endif

 // if no error occurred so far, try to connect: this will return server start error, if any:
 // must not do this if starting a dependent server: this would down it!
  if (result == KSE_OK && !start_dependent_server)
  { 
   // server probably started, wait for a defined server state:
    DWORD dwTickStart;  dwTickStart = GetTickCount();  int server_state;
    do
      server_state = srv_Get_state_local(server_name);
    while (!server_state && WinWait (dwTickStart, 15000));
    if (!server_state) return KSE_WINEXEC;  // may be other unreported server problem
    cd_t cd;
    cdp_init(&cd);
#ifdef UNIX  // on Unix detecting the server does not mean that the server already accepts connections, must retry
    dwTickStart = GetTickCount();
    do
      result=cd_connect(&cd, server_name, 2000);
    while (result != KSE_OK && WinWait (dwTickStart, 7000));
#else    
    result=cd_connect(&cd, server_name, 2000);
#endif    
    if (result == KSE_OK) cd_disconnect(&cd);
  }  
  return result;
}

CFNC DllKernel int WINAPI srv_Stop_server_local(const char * server_name, int wait_seconds, cdp_t cdp)
// returns KSE_OK on success, error number if failed.
// Must not be called if WBVERS<900.
{ 
#ifdef LINUX
  if (!wait_seconds && !cdp)
  { char path[MAX_PATH], buf[12];  int pid;
    if (!GetDatabaseString(server_name, "PATH",  path, sizeof(path), false) &&
        !GetDatabaseString(server_name, "PATH1", path, sizeof(path), false) &&
        !GetDatabaseString(server_name, "PATH",  path, sizeof(path), true ) &&
        !GetDatabaseString(server_name, "PATH1", path, sizeof(path), true ))
      return 1;  // server is not local
    if (!*path)
      return 1;  // server is not local
    strcat(path, "/pid.txt");
    int fh = open(path, O_RDONLY);
    if (fh == -1) return 1;
    int rd = read(fh, buf, sizeof(buf)-1);
    close(fh);
    if (rd<=0) return 1;
    buf[rd]=0;
    if (sscanf(buf, " %u", &pid) != 1) return 1;
    if (!kill(pid, SIGTERM)) return 0;
  }  
#else
 // if service running, stop it (may not be possible because of missing privileges)
  bool service_stopped = false;  char service_name[MAX_SERVICE_NAME+1];
  if (!cdp && !wait_seconds && srv_Get_service_name(server_name, service_name))
  { SERVICE_STATUS ssStatus;  
    SC_HANDLE hSCM = ::OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (hSCM)
    { SC_HANDLE hService = ::OpenService(hSCM, service_name, SERVICE_ALL_ACCESS);
      if (hService)
      { if (ControlService(hService, SERVICE_CONTROL_STOP, &ssStatus)) 
        {// Check the status until the service status is no longer "start pending". 
          while (ssStatus.dwCurrentState == SERVICE_STOP_PENDING)
          { Sleep(200);
            if (!QueryServiceStatus(hService, &ssStatus)) break;
          }
          if (ssStatus.dwCurrentState == SERVICE_STOPPED) service_stopped=true; 
        }
        ::CloseServiceHandle(hService);
      }
      ::CloseServiceHandle(hSCM);
    }
  }  
  if (service_stopped) return 0;  // service not stopped if not called under an administrator account
#endif
 // try to connect (unless the external cdp provided) and stop:
  cd_t cd;  int res;
  cdp_t down_cdp;
  if (cdp) down_cdp=cdp;
  else
  { down_cdp=&cd;  
    cdp_init(down_cdp);
    res=cd_connect(down_cdp, server_name, 2000);
    if (res!=KSE_OK)
      return res;  // not running locally
  }
  tptr p=get_space_op(down_cdp, 1+1+sizeof(uns32), OP_CNTRL);
  *p=SUBOP_STOP;  p++;
  *(uns32*)p=wait_seconds;
  if (cond_send(down_cdp))
    res=cd_Sz_error(down_cdp);
  else 
    res=0;
  if (!cdp) 
    cd_disconnect(down_cdp);  
  return res;
}

CFNC DllKernel BOOL WINAPI srv_Get_service_name(const char * server_name, char * service_name)
{ 
#ifdef WINS
  *service_name=0;
  LONG res;  HKEY hKey, hSubKey;  bool found=false;
  res=RegOpenKeyEx(HKEY_LOCAL_MACHINE, "SYSTEM\\CurrentControlSet\\Services", 0, KEY_READ_EX, &hKey);
  DWORD key_len;  DWORD ind=0;  char a_service_name[MAX_SERVICE_NAME+1];
  while (!found && RegEnumKey(hKey, ind++, a_service_name, MAX_SERVICE_NAME+1) == ERROR_SUCCESS)
  { if (RegOpenKeyEx(hKey, a_service_name, 0, KEY_READ_EX, &hSubKey)==ERROR_SUCCESS)
    { BYTE path[MAX_PATH];  key_len=sizeof(path);  // BYTE type used by toupper
      if (RegQueryValueEx(hSubKey, "ImagePath", NULL, NULL, path, &key_len) == ERROR_SUCCESS)
      { int i;  BYTE service_fname[MAX_PATH];
        for (i=0;  path[i];  i++)  path[i]=(BYTE)toupper(path[i]);
        strcpy((char*)service_fname, SERVICE_FNAME);  // BYTE type used by toupper
        for (i=0;  service_fname[i];  i++)  service_fname[i]=(BYTE)toupper(service_fname[i]);
        if (strstr((char*)path, (char*)service_fname))
        { key_len=sizeof(path);
          if (RegQueryValueEx(hSubKey, "CnfPath", NULL, NULL, path, &key_len) == ERROR_SUCCESS)
            if (!stricmp(server_name, (char*)path)) 
              { found=true;  strcpy(service_name, a_service_name); }
        }
      }
      RegCloseKey(hSubKey);
    }
  }
  RegCloseKey(hKey);
  return found;
#else
  strcpy(service_name, server_name);
  return TRUE;
#endif  
}

CFNC DllKernel BOOL WINAPI srv_Register_server_local(const char * server_name, const char * path, BOOL private_server)
// [path] is NULL for network servers.
{ bool res=true;
#ifdef WINS
  char key[160];  HKEY hKey;  DWORD Disposition;
  strcpy(key, WB_inst_key);  strcat(key, Database_str);  strcat(key, "\\");  strcat(key, server_name);
  if (RegCreateKeyEx(private_server ? HKEY_CURRENT_USER : HKEY_LOCAL_MACHINE, key, 0, "", REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS_EX, NULL, &hKey, &Disposition)
      !=ERROR_SUCCESS) res=false;
  else
  { if (RegSetValueEx(hKey, Version_str, 0, REG_SZ, (BYTE*)CURRENT_FIL_VERSION_STR, (int)strlen(CURRENT_FIL_VERSION_STR)+1)!=ERROR_SUCCESS)
      res=false;
    if (path)  // local server
    { if (RegSetValueEx(hKey, Path_str, 0, REG_SZ, (BYTE*)path, (int)strlen(path)+1)!=ERROR_SUCCESS)
        res=false;
    }    
    RegCloseKey(hKey);
  }
#else
  const char * cfile = private_server ? locconffile : configfile;
  res = WritePrivateProfileString(server_name, Version_str, CURRENT_FIL_VERSION_STR, cfile) != FALSE &&
        (path==NULL || WritePrivateProfileString(server_name, Path_str, path, cfile) != FALSE);
#endif
  return res;
}

#ifdef WINS
CFNC DllKernel BOOL WINAPI srv_Register_service(const char * server_name, const char * service_name)
{ bool ok=false;
 // prepare the path:
  char pathname[MAX_PATH];
  get_server_path(pathname);
  int i=(int)strlen(pathname);  while (i && pathname[i]!=PATH_SEPARATOR) i--;
  strcpy(pathname+i, SERVICE_FNAME);
 // register it:
  SC_HANDLE hSCM = ::OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
  if (hSCM) 
  { SC_HANDLE hService = ::CreateService(hSCM, service_name, service_name, SERVICE_ALL_ACCESS,
                                       SERVICE_WIN32_OWN_PROCESS|SERVICE_INTERACTIVE_PROCESS,
                                       SERVICE_DEMAND_START, SERVICE_ERROR_NORMAL,
                                       pathname, NULL, NULL, NULL, NULL, NULL);
    if (hService)
    {// write the server name into the service description: 
      HKEY hkey;  char key[150];
	    strcpy(key, "SYSTEM\\CurrentControlSet\\Services\\");  strcat(key, service_name);
      if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, key, 0, KEY_SET_VALUE_EX, &hkey) == ERROR_SUCCESS)
      { if (RegSetValueEx(hkey, "CnfPath", NULL, REG_SZ, (BYTE*)server_name, (int)strlen(server_name)+1) == ERROR_SUCCESS)
          ok=true;
        const char * descr = "602SQL Server";
        RegSetValueEx(hkey, "Description", NULL, REG_SZ, (BYTE*)descr, (int)strlen(descr)+1);
        RegCloseKey(hkey);
      }
      CloseServiceHandle(hService);
    }
    CloseServiceHandle(hSCM);
  }
  return ok;
}  

CFNC DllKernel BOOL WINAPI srv_Unregister_service(const char * service_name)
// Returns false on error.
{ bool ok=false;
  SC_HANDLE hSCM = ::OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
  if (hSCM)
  { SC_HANDLE hService = ::OpenService(hSCM, service_name, SERVICE_ALL_ACCESS);
    if (hService)
    { if (DeleteService(hService)) ok=true;
      ::CloseServiceHandle(hService);
    }
    ::CloseServiceHandle(hSCM);
  }
  return ok;
}
#endif

CFNC DllKernel BOOL WINAPI srv_Unregister_server(const char * server_name, BOOL private_server)
// Used for local databases and remote servers.
{
#ifdef WINS
 // unregister the service name is registered for the server (supposed that it is not running)
  char service_name[MAX_SERVICE_NAME+1];
  if (srv_Get_service_name(server_name, service_name))
    if (!srv_Unregister_service(service_name))
      return FALSE;
 // unregrister the database:
  char key[160];
  strcpy(key, WB_inst_key);  strcat(key, Database_str);  strcat(key, "\\");  strcat(key, server_name);
  return RegDeleteKey(private_server ? HKEY_CURRENT_USER : HKEY_LOCAL_MACHINE, key)==ERROR_SUCCESS;
#else  // LINUX
  const char * cfile = private_server ? locconffile : configfile;
  return WritePrivateProfileString(server_name, NULL, NULL, cfile)!=FALSE;
#endif
}

///////////////////////////////////////////////////////////////////////////////////////////////////
#define REGISTER_ANSWER(address,size) {\
  cdp->ans_array[cdp->ans_ind]=address;\
  cdp->ans_type [cdp->ans_ind]=size;   \
  cdp->ans_ind++;             }
// RV.holding: bit 1: messages blocked, bit 2: waiting for answer, messages must be blocked even just after answer delivered
#if WBVERS<950
#define ANS_SIZE_DYNALLOC  0xfffe
#define ANS_SIZE_PREVITEM  0xfffd
#define ANS_SIZE_VARIABLE  0xffff
#else
#define ANS_SIZE_DYNALLOC  0xfffffffe
#define ANS_SIZE_PREVITEM  0xfffffffd
#define ANS_SIZE_VARIABLE  0xffffffff
#endif
/* Zpusob registrace bufferu pro odpoved na pozadavek klienta na server:
Pri zadani pozadavku klienta na server se zaregistruji vsechny casti ocekavane odpovedi, aby po jejim 
(pripadne asynchronnim) prichodu bylo mozno je zapsat na spravna mista. K registraci slouzi makro
REGISTER_ANSWER(address,size), kde [address] je adresa bufferu pro odpoved a [size] je velikost odpovedi nebo zpusob
zjisteni velikosti a manipulace s odpovedi.
[size] muze byt:
cislo mensi nez 0x(ffff)fffd - odpoved ma pevnou velikost [size], zkopiruje se do bufferu [address]
ANS_SIZE_DYNALLOC      - odpoved zacina ctyrbajtovym udajem o sve skutecne velikosti, pro odpoved se alokuje novy buffer, 
                         do nej se odpoved zkopiruje a adresa tohoto bufferu se zapise do [address], je/li odpoved prazdna, 
                         zapise se tam NULL;
ANS_SIZE_PREVITEM      - velikost odpovedi je zapsana v predchozi odpovedi na stejny pozadavek, ta predchozi odpoved musi 
                         byt ctyrbajtova. Odpoved se zkopiruje na [address]
ANS_SIZE_VARIABLE      - odpoved zacina ctyrbajtovym udajem o velikosti. Za nim nasleduje odpoved, ktera se zkopiruje.
                         Velikost odpovedi se nekopiruje.
Odpoved se zapisuje pouze tehdy, pokud pozadavek skoncil uspesne, jinak je pripadna odpoved ignoruje.
*/
void drop_all_statements(cdp_t cdp);  // Drops all prepared statements
tptr get_space_op(cd_t * cdp, unsigned size, uns8 op);
/////////////////////////////////////// client version /////////////////////////////////
CFNC DllKernel uns32 WINAPI Get_client_version1(void)
{ return VERS_1; }

CFNC DllKernel uns32 WINAPI Get_client_version2(void)
{ return VERS_2; }

CFNC DllKernel uns32 WINAPI Get_client_version3(void)
{ return VERS_3; }

CFNC DllKernel uns32 WINAPI Get_client_version4(void)
{ return VERS_4; }

CFNC DllKernel uns32 WINAPI Get_client_version(void)
{ uns32 my_vers = 256*(256*(256*VERS_1+VERS_2)+VERS_3)+VERS_4;
#ifdef WINS
  if (SecGetVersion()  != my_vers) return 0;
#if WBVERS<900
#ifndef ODBCDRV
  uns32 lib_vers;
  if (PrezGetVersion() != my_vers) return 0;
  lib_vers=lnk_DvlpGetVersion();
  if (lib_vers && lib_vers != my_vers) return 0;
  //lib_vers=lnk_WedGetVersion();  // versions of WED are independent
  //if (lib_vers && lib_vers != my_vers) return 0;
#if WBVERS>=810
  //lib_vers=lnk_XMLGetVersion();  // this loads XML extender - may be beter not to do this
  //if (lib_vers && lib_vers != my_vers) return 0;
#endif
#endif
#endif
#endif
  return my_vers;
}
///////////////////////////////// caching apl objects //////////////////////////////
#ifdef WINS
#ifndef ODBCDRV
bool caching_apl_objects = true;
#else // ODBCDRV
bool caching_apl_objects = false;
#endif
#else // LINUX
bool caching_apl_objects = false;
#endif

CFNC DllKernel void WINAPI Set_caching_apl_objects(bool on)
{ caching_apl_objects=on; }
/////////////////////////////// cdp & cds //////////////////////////////////////////
cdp_t cds[MAX_MAX_TASKS];   
int client_tzd = 0;  // time zone displacement in seconds
DWORD keep_alive_seconds = 0;  // since 8.1.2.6 not sending keep-alive packets unless required
  
CFNC DllKernel cdp_t WINAPI GetCurrTaskPtr(void)
{ t_thread task = GetClientId();
  for (int i = 0;  i < MAX_MAX_TASKS;  i++)
    if (cds[i] && cds[i]->threadID == task) return cds[i];
  return NULL;
}

static run_state auxRV;

CFNC DllKernel run_state * WINAPI get_RV(void)
{ cdp_t cdp = GetCurrTaskPtr();
  return cdp ? (run_state *)&cdp->RV : (run_state *)&auxRV; 
}

CFNC void WINAPI cdp_init(cdp_t cdp)
{ core_init(0);  /* early error messages and initializing locDevNames need it */
  memset(cdp, 0, sizeof(cd_t));
  cdp->connection_type = CONN_TP_602;
  cdp->in_use=PT_EMPTY;
  cdp->RV.registry.init();
  cdp->RV.synchros.init();
  cdp->prep_stmts.init();
  cdp->odbc.apl_tables.init();
  cdp->odbc.apl_views.init();
  cdp->odbc.apl_menus.init();
  cdp->odbc.apl_queries.init();
  cdp->odbc.apl_programs.init();
  cdp->odbc.apl_pictures.init();
  cdp->odbc.apl_drawings.init();
  cdp->odbc.apl_replrels.init();
  cdp->odbc.apl_wwws.init();
  cdp->odbc.apl_relations.init();
  cdp->odbc.apl_procs.init();
  cdp->odbc.apl_triggers.init();
  cdp->odbc.apl_seqs.init();
  cdp->odbc.apl_folders.init();
  cdp->odbc.apl_domains.init();
  cdp->applnum=0xff; 
  cdp->selected_lang=-1;  // important for non-gui client
 // no application opened:
  cdp->admin_role=cdp->senior_role=cdp->junior_role=cdp->author_role=NOOBJECT;
  cdp->applobj=NOOBJECT;
 // not logged:
  cdp->logged_as_user=NOOBJECT;
  cdp->client_error_callback2=NULL;
  cdp->client_error_callbackW=NULL;
}

//#ifndef ODBCDRV  // !CLIENT_GUI    -- no more in mpsql
// now, cache is present in all versions, but sometimes it is switched off
#include "objcache.cpp"
//#endif 

/**************************** interf init and close *************************/
typedef struct
{ BYTE  server_addr[6];
  uns16 ref_cnt;
} t_server_list;
#define SERVER_LIST_SIZE 3
t_server_list server_list[SERVER_LIST_SIZE] = { {"",0}, {"",0}, {"",0} };

static uns8 address_to_server_index(BYTE * addr)
{ int i;
  for (i=0;  i<SERVER_LIST_SIZE;  i++)
    if (server_list[i].ref_cnt && !memcmp(addr, server_list[i].server_addr, 6))
      { server_list[i].ref_cnt++;  return i+1; }
  for (i=0;  i<SERVER_LIST_SIZE;  i++)
    if (!server_list[i].ref_cnt)
    { memcpy(server_list[i].server_addr, addr, 6);
      server_list[i].ref_cnt=1;  return i+1;
    }
  return 0;
}

#define FREE_SERVER_INDEX(cdp) if (cdp->server_index) \
 if (server_list[cdp->server_index-1].ref_cnt) \
     server_list[cdp->server_index-1].ref_cnt--;

void RemoveTask(cdp_t cdp)
{ int i=0;
  while (cds[i]!=cdp)   /* should be always OK */
//  while (cds[i]==NULL || cds[i]->threadID!=cdp->threadID)
// -- bad, with ODBC several tasks can have the same threadID
    if (++i >= MAX_MAX_TASKS) return;  /* task not found */
  cds[i]=NULL;
  cdp->in_use=PT_EMPTY;
  cdp->applnum=0xff;
}

void cd_t::init_client_server_comm(void)
{ in_package=in_concurrent=wbfalse;
  default_frame_busy = has_persistent_part = false;
  req_ind=0;  ans_ind=0;
#if WBVERS<=810
  priv_key_file=NULL;  // done in cdp_init, to be sure
#endif  
 /* odbc: */
#ifdef CLIENT_ODBC
  SQLAllocEnv(&odbc.hEnv);
#endif
  odbc.mapping_on=FALSE;
  RV.holding=0;
}

#ifdef CLIENT_ODBC
static void free_odbc_tables(cdp_t cdp)
{ if (cdp->odbc.hEnv)
  { while (cdp->odbc.conns != NULL)
    { t_connection * conn = cdp->odbc.conns;
      cdp->odbc.conns=conn->next_conn;
      free_connection(cdp, conn);
    }
    inval_table_d(cdp, NOOBJECT, CATEG_DIRCUR);  /* must remove odbc table info from the cache */
  }
}
#endif

#ifdef WINS
static DWORD GetLinkId(cdp_t cdp)
// I need a unique identification of the connection to the server. A thread may have multiple conections.
{ return GetCurrentProcessId() + (DWORD)(size_t)cdp; }

void CALLBACK ClientTimerProc(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime);
#endif

#ifdef WINS
CFNC DllKernel unsigned char * WINAPI load_certificate_data(const tobjname server_name, unsigned * length)
{ LONG res;  char key[150];  HKEY hKey;
  strcpy(key, WB_inst_key);  strcat(key, Database_str);  strcat(key, "\\");
  strcat(key, server_name);
  res=RegOpenKeyEx(HKEY_LOCAL_MACHINE, key, 0, KEY_READ_EX, &hKey);
  if (res!=ERROR_SUCCESS) return NULL;
  DWORD key_len;
  res=RegQueryValueEx(hKey, "ServerCertificate", NULL, NULL, NULL, &key_len);
  if (res!=ERROR_SUCCESS) { RegCloseKey(hKey);  return NULL; }
  unsigned char * buf = (unsigned char *)corealloc(key_len, 58);
  if (!buf) { RegCloseKey(hKey);  return NULL; }
  res=RegQueryValueEx(hKey, "ServerCertificate", NULL, NULL, buf, &key_len);
  if (res!=ERROR_SUCCESS) { corefree(buf);  RegCloseKey(hKey);  return NULL; }
  RegCloseKey(hKey);
  *length = key_len;
  return buf;
}

void save_certificate_data(const tobjname server_name, unsigned char * buf, unsigned length)
{ LONG res;  char key[150];  HKEY hKey;  
  strcpy(key, WB_inst_key);  strcat(key, Database_str);  strcat(key, "\\");
  strcat(key, server_name);
#if 0  // do not create imcomplete server records in the registry, do not save certificate when the server is not registered
  DWORD disp;
  res=RegCreateKeyEx(HKEY_LOCAL_MACHINE, key, 0, "", 0, KEY_ALL_ACCESS_EX, NULL, &hKey, &disp);
#else
  res=RegOpenKeyEx(HKEY_LOCAL_MACHINE, key, 0, KEY_ALL_ACCESS_EX, &hKey);
#endif
  if (res!=ERROR_SUCCESS) return;
  DWORD key_len = length;
  res=RegSetValueEx(hKey, "ServerCertificate", 0, REG_BINARY, buf, key_len);
  RegCloseKey(hKey);
}

CFNC DllKernel BOOL WINAPI erase_certificate(const tobjname server_name)
{ char key[150];  HKEY hKey;  bool ok;
  strcpy(key, WB_inst_key);  strcat(key, Database_str);  strcat(key, "\\");  strcat(key, server_name);
  if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, key, 0, KEY_ALL_ACCESS_EX, &hKey)!=ERROR_SUCCESS) return FALSE;
  ok = RegDeleteValue(hKey, "ServerCertificate")==ERROR_SUCCESS;
  RegCloseKey(hKey);
  return ok;
}

#else  // LINUX

#include <sys/stat.h> /* using chmod(), mkdir()*/
#include <ctype.h>
#include <string.h>
#include <errno.h>

unsigned char * load_certificate_data(const tobjname server_name, unsigned * length)
{ 
  const int STRLEN = 1024;
  
  FILE * cert_file;
  char cert_filename[STRLEN];
  struct stat file_stat;
  unsigned key_len;
  tobjname server_name_lc;
  
  strcpy(server_name_lc, server_name);
  int _i;
  for (_i = 0; server_name_lc[_i]; _i++ )
  {
    server_name_lc[_i] = tolower(server_name_lc[_i]);
  }
  
  sprintf(cert_filename, "%s/%s/servcerts/%s.cer", getenv("HOME"), CERT_CLIENT_DIR, (char *)server_name_lc );
  
  if ( (cert_file = fopen(cert_filename,"r")) == NULL )
  {
    //perror("fopen");
    return NULL;
  }
  
  stat(cert_filename, &file_stat);
  
  int cert_filesize = file_stat.st_size;
  
  unsigned char * buf = (unsigned char *)corealloc(cert_filesize+1, 58);
  if (!buf) 
  { fclose(cert_file);
    return NULL; 
  }

  *length = fread(buf, sizeof(unsigned char), cert_filesize, cert_file);
  fclose(cert_file);
  return buf;
}

void save_certificate_data(const tobjname server_name, unsigned char * buf, unsigned length)
{
  FILE * new_cert_file;
  char cert_dirname[MAX_PATH];
  char new_cert_filename[MAX_PATH];
  tobjname server_name_lc;
  
  strcpy(server_name_lc, server_name);
  int _i;
  for (_i = 0; server_name_lc[_i]; _i++ )
  {
    server_name_lc[_i] = tolower(server_name_lc[_i]);
  }

  sprintf(cert_dirname, "%s/%s/" , getenv("HOME"), CERT_CLIENT_DIR);
  if (mkdir(cert_dirname, S_IRWXU) == -1)
    if (errno != EEXIST) return;
  strcat(cert_dirname, "servcerts/");
  if (mkdir(cert_dirname, S_IRWXU) == -1)
    if (errno != EEXIST) return;

  sprintf(new_cert_filename, "%s%s.cer", cert_dirname, server_name_lc );

  if ( (new_cert_file = fopen(new_cert_filename,"wb")) == NULL )
    return;

  fwrite(buf, sizeof(unsigned char), length, new_cert_file);
  fclose(new_cert_file);

  chmod(new_cert_filename, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

  return;
}

CFNC DllKernel BOOL WINAPI erase_certificate(const tobjname server_name)
{
  char cert_filename[MAX_PATH];
  tobjname server_name_lc;
  
  strcpy(server_name_lc, server_name);
  int _i;
  for (_i = 0; server_name_lc[_i]; _i++ )
  {
    server_name_lc[_i] = tolower(server_name_lc[_i]);
  }

  sprintf(cert_filename, "%s/%s/servcerts/%s.cer" , getenv("HOME"), CERT_CLIENT_DIR, server_name_lc );

  if (remove(cert_filename) == -1)
  {
    //perror("remove");
    return FALSE;
  }

  return TRUE;
}
#endif

static int start_communication(cdp_t cdp, const tobjname server_name)
{// try loading existing certificate:
  unsigned length;  t_Certificate * cert = NULL;
  unsigned char * data = load_certificate_data(server_name, &length);
  if (data) 
  { cert = make_certificate(data, length);
    corefree(data);
  }
 // if I do no have the certificate, load it from the server:
  if (!cert)
  { t_varcol_size size;
    if (cd_Read_len(cdp, SRV_TABLENUM, 0, SRV_ATR_PUBKEY, NOINDEX, &size)) return KSE_UNSPEC;
    if (size)
    { data = (unsigned char *)corealloc(size, 58);
      if (!data) return KSE_NO_MEMORY;
      if (cd_Read_var(cdp, SRV_TABLENUM, 0, SRV_ATR_PUBKEY, NOINDEX, 0, size, (tptr)data, &size))
        { corefree(data);  return KSE_UNSPEC; }
      save_certificate_data(server_name, data, size);
      cert = make_certificate(data, size);  // independent of the sucess of save_certificate_data
      corefree(data);
    }
  }
  if (!cert) return KSE_OK;  // server does not have the certificate
 // send challenge to the server and encoding proposal:
  unsigned char challenge[CHALLENGE_SIZE], enc_challenge[256], dec_challenge[CHALLENGE_SIZE];  unsigned enc_length;
  get_random_string(CHALLENGE_SIZE, challenge);
  encrypt_string_by_public_key(cert, challenge, CHALLENGE_SIZE, enc_challenge, &enc_length);
  uns32 prop_encryption=COMM_ENC_NONE | COMM_ENC_BASIC, sel_encryption;
  tptr p=get_space_op(cdp, 1+1+2*sizeof(uns32)+enc_length, OP_LOGIN);
  *p=LOGIN_CHALLENGE;          p++;
  *(uns32*)p=prop_encryption;  p+=sizeof(uns32);
  *(uns32*)p=enc_length;       p+=sizeof(uns32);
  memcpy(p, enc_challenge, enc_length);
  REGISTER_ANSWER(&sel_encryption, sizeof(uns32));
  REGISTER_ANSWER(dec_challenge, CHALLENGE_SIZE);
  if (cond_send(cdp))  // server does not have private key 
    { DeleteCert(cert);  return KSE_OK; }  // this should NOT to be error, user may have failed to import private key!
  if (memcmp(challenge, dec_challenge, CHALLENGE_SIZE))
    { DeleteCert(cert);  return KSE_FAKE_SERVER; }
  if (!(sel_encryption & prop_encryption))  // server refused proposed encryption
    { DeleteCert(cert);  return KSE_ENCRYPTION_FAILURE; }
 // prepare encryption:
  if (sel_encryption==COMM_ENC_BASIC)
  { unsigned char key[COMM_ENC_KEY_SIZE];
    get_random_string(COMM_ENC_KEY_SIZE, key);
    encrypt_string_by_public_key(cert, key, COMM_ENC_KEY_SIZE, enc_challenge, &enc_length);
    DeleteCert(cert);
    p=get_space_op(cdp, 1+1+sizeof(uns32)+enc_length, OP_LOGIN);
    *p=LOGIN_ENC_KEY;       p++;
    *(uns32*)p=enc_length;  p+=sizeof(uns32);
    memcpy(p, enc_challenge, enc_length);
    if (cond_send(cdp)) return KSE_UNSPEC;
    cdp->commenc = new t_commenc_pseudo(false, key); // start encrypting from the next request
  }
  else DeleteCert(cert);  
  return KSE_OK;
}

int direct_connection_timeout = 80000; // pozor> pri timeoutu kvuli zaneprazdnenosti serveru zustane pripojen an server

CFNC DllKernel void WINAPI cd_assign_to_thread(cdp_t cdp)
{ cdp->threadID = GetClientId(); }

CFNC DllKernel void WINAPI cd_unassign(cdp_t cdp)
// Deletes the assignment, but checks if it is really assigned (otherwise it could unassign another thread)
{ if (cdp->threadID==GetClientId()) cdp->threadID = 0; }

CFNC DllKernel int WINAPI get_time_zone_info(void)
{
#ifdef WINS
  TIME_ZONE_INFORMATION tzi;  
  DWORD res=GetTimeZoneInformation(&tzi); 
  client_tzd=-(60*(res==TIME_ZONE_ID_DAYLIGHT ? tzi.Bias+tzi.DaylightBias : tzi.Bias));
#else
  struct timeval tv;  struct timezone tz;
  gettimeofday(&tv, &tz);
  client_tzd=-(60*tz.tz_minuteswest);
#endif
  return client_tzd;
}
////////////////////////////////////////// connection list //////////////////////////////////////////
xcd_t * connected_xcds = NULL;

void add_to_list(xcd_t * xcdp)
{
#if WBVERS>=900
  EnterCriticalSection(&cs_short_term);
  xcdp->thread_id = GetClientId();
  xcdp->next_xcd = connected_xcds;
  connected_xcds = xcdp;
  LeaveCriticalSection(&cs_short_term);
#endif
}

void remove_from_list(xcd_t * xcdp)
{ 
#if WBVERS>=900
  EnterCriticalSection(&cs_short_term);
  xcd_t ** xcdpp = &connected_xcds;
  while (*xcdpp != NULL)
    if (*xcdpp == xcdp)
    { *xcdpp = xcdp->next_xcd;
      break;
    }
    else
      xcdpp = &(*xcdpp)->next_xcd;
  LeaveCriticalSection(&cs_short_term);
#endif
}

CFNC DllKernel xcd_t * WINAPI find_connection(const char * dsn)
{ xcd_t * xcdp = NULL;
#if WBVERS>=900
  t_thread my_thread_id = GetClientId();
  EnterCriticalSection(&cs_short_term);
  for (xcdp = connected_xcds;  xcdp;  xcdp=xcdp->next_xcd)
    if (xcdp->thread_id==my_thread_id || !xcdp->thread_id)
      if (!stricmp(dsn, xcdp->conn_server_name))
        break;  // found, is in [xcdp]
  LeaveCriticalSection(&cs_short_term);
#endif
  return xcdp;  // NULL iff not found
}


CFNC DllKernel int WINAPI interf_init_ex(cdp_t cdp, int user, cAddress * pServerAddr, const char * server_name)
{ start_lk_sharing(); // starts sharing licences among processes on the computer
#ifdef WINS
  HWND hClient = get_RV()->hClient; 
#endif
  get_time_zone_info();
  if (user==PT_REMOTE && !pServerAddr)
    return KSE_CONNECTION; // no successfull link_kernel performed
  strcpy(cdp->conn_server_name, server_name);  // may have the form IP:port
#if WBVERS>=900  
  strcpy(cdp->locid_server_name, server_name); // unique local readable identification of the server, is the local registration name or "IP database_name" for unregistered servers
#endif

 // insert cdp to the cds list:
  int new_ind=1;  // find space if the cds:
  while (cds[new_ind] != NULL)
    if (++new_ind >= MAX_MAX_TASKS)  /* no free cell in the cds[] */
    { 
#ifdef WINS
      if (user==PT_REMOTE) NetworkStop(cdp);
#endif
      cdp->conn_server_name[0]=0;
      return KSE_NOTASK;
    }
  cds[new_ind]    = cdp;
  cdp->in_use     = (byte)user; /* PT_DIRECT or PT_REMOTE */
  cdp->threadID   = GetClientId();
  cdp->applnum    = new_ind;
#ifdef WINS
  cdp->RV.hClient  = hClient;
  cdp->non_mdi_client = get_RV()->non_mdi_client;
#endif

  if (user==PT_REMOTE)
  { cdp->pRemAddr = pServerAddr;  pServerAddr=NULL;
    int retCode = Link(cdp);
    if (retCode != KSE_OK)
      { RemoveTask(cdp);  NetworkStop(cdp);  cdp->conn_server_name[0]=0;  return retCode; }
    cdp->server_index=address_to_server_index(cdp->pRemAddr->GetRemIdent());
#ifndef PEEKING_DURING_WAIT
#ifndef SINGLE_THREAD_CLIENT
    CreateLocalAutoEvent(0, &cdp->hSlaveSem);
#endif
#endif
  }

#ifdef WINS
  if (user==PT_DIRECT) 
  { HANDLE hMakerEvent, hMakeDoneEvent, hMemFile;  DWORD * clientId, serverId=0;  bool mapped_long;
    bool use_global_namespace = is_server_in_global_namespace(cdp->conn_server_name);
    // server may have global or local names, try both
    if (!CreateIPComm0(cdp->conn_server_name, NULL, &hMakerEvent, &hMakeDoneEvent, &hMemFile, &clientId, mapped_long, use_global_namespace))
      { RemoveTask(cdp);  cdp->conn_server_name[0]=0;  return KSE_SYNCHRO_OBJ; }
    int res;
   // strazeni public casti prihlasovani pomoci mutexu
    HANDLE hDirectLoginMutex=CreateMutex(NULL,FALSE,"WB_DIRECT_LOGIN_MUTEX");
    if (WaitForSingleObject(hDirectLoginMutex, direct_connection_timeout)==WAIT_TIMEOUT)
      res=KSE_TIMEOUT;
    else
    {/* Synchro objects must be created before start, otherwise start errors are lost. */
		  clientId[0] = cdp->LinkIdNumber = GetLinkId(cdp);
//#if WBVERS>=900
      if (mapped_long) clientId[1] = GetCurrentProcessId();
//#endif
      ResetEvent(hMakeDoneEvent);  // may be signalled if the previous connection attempt ended on timeout
		  SetEvent(hMakerEvent);
		  if (WaitForSingleObject(hMakeDoneEvent, direct_connection_timeout) == WAIT_TIMEOUT) // must wait for entering the password
        res=KSE_TIMEOUT;
      else 
      { res=(int)clientId[0];   /* slave thread creation result */
//#if WBVERS>=900
        if (mapped_long) 
        { serverId=clientId[1];
          if (serverId == GetCurrentProcessId()) serverId=0;  // server did not return its ID
        }
//#endif
      }
      ReleaseMutex(hDirectLoginMutex);
    }
    CloseHandle(hDirectLoginMutex);
    CloseIPComm0(hMakerEvent, hMakeDoneEvent, hMemFile, clientId);
    if (res==KSE_OK)
		  // server may have global or local names, try both
      if (!cdp->dcm.OpenClientSlaveComm(cdp->LinkIdNumber, cdp->conn_server_name, NULL, use_global_namespace))
        res=KSE_SYNCHRO_OBJ;
    if (res!=KSE_OK)
      { RemoveTask(cdp);  cdp->conn_server_name[0]=0;  return res; }
    cdp->dcm.PrepareSynchroByClient(serverId ? OpenProcess(SYNCHRONIZE, FALSE, serverId) : 0);  // 0 if serverId not obtained from the server
    cdp->dcm.ClearServerDownFlag();  // init
  } // PT_DIRECT
#ifdef CLIENT_GUI
#if WBVERS<900
  cd_Help_file(cdp, NULLSTRING);  // sets the EXE-path for help
  open_object_cache(cdp);
#endif
#endif
#endif // WINS PT_DIRECT

#ifdef NETWARE  // NLM direct client
  int res=wb_dir_interf_init(&cdp->dircomm);
  if (res!=KSE_OK) { cdp->conn_server_name[0]=0;  return res; }
#endif

 // COMMON: start communication with the server:
  cdp->init_client_server_comm(); 
  uns32 vers1, vers2;
  if (cd_Get_server_info(cdp, OP_GI_VERSION_1, &vers1, sizeof(vers1)))
  { int errnum = cd_Sz_error(cdp);
    cd_interf_close(cdp);  cdp->conn_server_name[0]=0;  
    return user==PT_REMOTE && errnum>500 ? errnum-500   // HTTP tunnel initialization errors
      : errnum==CONNECTION_LOST ? KSE_NO_HTTP_TUNNEL    // only web interface is running on the HTTP tunnel port
      : KSE_SERVER_CLOSED;   // server returns REJECTED_BY_KERNEL
  }
  else
    if (!cd_Get_server_info(cdp, OP_GI_VERSION_2, &vers2, sizeof(vers2)))
      cdp->server_version = MAKELONG(vers2, vers1);
  cd_Read(cdp, SRV_TABLENUM, 0, SRV_ATR_UUID, NULL, cdp->server_uuid);   // using the property would be better but some old servers do not have it
  cd_Read(cdp, SRV_TABLENUM, 0, SRV_ATR_NAME, NULL, cdp->conn_server_name);  // the real server name (replaces IP:port etc.)
 // send time zone information:
  if (!cd_Get_server_info(cdp, OP_GI_VERSION_3, &cdp->server_version3, sizeof(cdp->server_version3)))
    if (vers1>8 || vers1==8 && (vers2>0 || vers2==0 && cdp->server_version3>=4))  // time zone supported
      cd_Send_client_time_zone(cdp);
 // check the versions of the client and the server (must be before start_communication):
#if WBVERS>=950
  //if (vers1<9 || vers1==9 && vers2<5)
  if (vers1<5)
  { cd_interf_close(cdp);  cdp->conn_server_name[0]=0;
    return KSE_CLIENT_SERVER_INCOMP;
  }
#endif
  uns32 min_client_version;
  if (!cd_Get_server_info(cdp, OP_GI_MINIMAL_CLIENT_VERSION, &min_client_version, sizeof(min_client_version)))
    if (min_client_version > WBVERS)
    { cd_interf_close(cdp);  cdp->conn_server_name[0]=0;
      return KSE_CLIENT_SERVER_INCOMP;
    }
 // verify the server identity and start encrypting the communication:
  if (user==PT_REMOTE)
  { int res=start_communication(cdp, /*cdp->conn_server_name*/ server_name); // must save certificates under the local name of the server
    if (res!=KSE_OK)
    { cd_interf_close(cdp);  cdp->conn_server_name[0]=0;
      return res;
    }
  }
 // get system charset information (user by the basic client library too):
  cdp->sys_charset=1;  // the default for old servers
  if (vers1>=9)
    cd_Get_server_info(cdp, OP_GI_SYS_CHARSET, &cdp->sys_charset, sizeof(cdp->sys_charset));
#ifdef WINS  // keep-alive timer
  if (keep_alive_seconds) cdp->timer=SetTimer(0, 0, keep_alive_seconds*1000, ClientTimerProc);
#endif
 // register the connection:
  add_to_list(cdp);
  return KSE_OK;
}

static BOOL cd_send_interf_close(cdp_t cdp);

void cdp_release(cdp_t cdp)
{ 
#ifdef CLIENT_PGM
  Select_language(cdp, -1); // releeses the language cache
#endif
 // clear object caches:
  cdp->odbc.apl_tables.comp_dynar::~comp_dynar();
  cdp->odbc.apl_views.comp_dynar::~comp_dynar();
  cdp->odbc.apl_menus.comp_dynar::~comp_dynar();
  cdp->odbc.apl_queries.comp_dynar::~comp_dynar();
  cdp->odbc.apl_programs.comp_dynar::~comp_dynar();
  cdp->odbc.apl_pictures.comp_dynar::~comp_dynar();
  cdp->odbc.apl_drawings.comp_dynar::~comp_dynar();
  cdp->odbc.apl_replrels.comp_dynar::~comp_dynar();
  cdp->odbc.apl_wwws.comp_dynar::~comp_dynar();
  cdp->odbc.apl_relations.relation_dynar::~relation_dynar();
  cdp->odbc.apl_procs.comp_dynar::~comp_dynar();
  cdp->odbc.apl_triggers.comp_dynar::~comp_dynar();
  cdp->odbc.apl_seqs.comp_dynar::~comp_dynar();
  cdp->odbc.apl_folders.folder_dynar::~folder_dynar();
  cdp->odbc.apl_domains.comp_dynar::~comp_dynar();
 // clear other allocations owned by cdp:
#if WBVERS<=810
  safefree((void**)&cdp->priv_key_file); // clears secure key in memory
#endif
  drop_all_statements(cdp);
  if (cdp->commenc) delete cdp->commenc;
 // back to the clear state
  cdp_init(cdp);                   
}
                          
CFNC DllKernel void WINAPI cd_interf_close(cdp_t cdp)
{ if (!cdp || cdp->in_use==PT_EMPTY) return;  // closed or non-existing
  remove_from_list(cdp);
#ifdef WINS  // keep-alive timer
  if (cdp->timer) KillTimer(0, cdp->timer);
#endif
#ifdef CLIENT_PGM
  free_project(cdp, TRUE);
#endif
#ifdef CLIENT_GUI
  close_object_cache(cdp);
#endif

  destroy_cursor_table_d(cdp);  // below is not sufficient when cursor was opened from Exec_statements and not closed
  if (cdp->obtained_login_key) { discard_login_key8(cdp->obtained_login_key);  cdp->obtained_login_key=0; }
#ifdef NETWARE  // NLM direct client
  inval_table_d(cdp, NOOBJECT, CATEG_CURSOR); // cursor will be closed by server, info must be removed from the cache!
  cd_send_interf_close(cdp);
#else
 // remote client for WINS and UNIX
  if (cdp->in_use==PT_REMOTE)  
  { *cdp->sel_appl_name=0;  // prevents reloading relations in inval_table_d
    inval_table_d(cdp, NOOBJECT, CATEG_TABLE);   /* server index duplicity */
    inval_table_d(cdp, NOOBJECT, CATEG_CURSOR);
    cdp->RV.hClient=0;  // prevents openning dialog in StopReceiveProcess in Unlink
    Unlink(cdp);
    NetworkStop(cdp);
    FREE_SERVER_INDEX(cdp);
#ifndef PEEKING_DURING_WAIT
#ifndef SINGLE_THREAD_CLIENT
    CloseLocalAutoEvent(&cdp->hSlaveSem);
#endif
#endif
  }
#ifdef WINS
 // direct client on Windows
  else if (cdp->in_use==PT_DIRECT)
  { cd_send_interf_close(cdp);
    cdp->dcm.CloseClientSlaveComm();
    //cd_unlink_kernel(cdp);  // added in 5.0, removed when this functionality implemented on the server in 9.5.1
  }
#endif
#endif  // !NETWARE

#ifdef CLIENT_ODBC /* odbc client */
  free_odbc_tables(cdp);
  if (cdp->odbc.hEnv) { SQLFreeEnv(cdp->odbc.hEnv);  cdp->odbc.hEnv=0; }
#endif
  RemoveTask(cdp);
  cdp_release(cdp);  // clears conn_server_name
}

/***************************** server start & stop **************************/
int get_client_protocol(const char * serv_name)
{
#ifdef WINS
  int reqProtocol = 0;
  char key[MAX_PATH];  HKEY hKey;
  strcpy(key, WB_inst_key);  
  if (serv_name && *serv_name)
    { strcat(key, Database_str);  strcat(key, "\\");  strcat(key, serv_name); }
  else strcat(key, "\\" WB_VERSION_STRING);
  if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, key, 0, KEY_READ_EX, &hKey)==ERROR_SUCCESS)
  { char protBuff[40];  DWORD key_len=sizeof(protBuff);
    if (RegQueryValueEx(hKey, ClientProt_str, NULL, NULL, (BYTE*)protBuff, &key_len)==ERROR_SUCCESS)
    { Upcase(protBuff);  
      if      (strstr(protBuff, "IPX"))     reqProtocol = IPXSPX;
      else if (strstr(protBuff, "NETBIOS")) reqProtocol = NETBIOS;
      else if (strstr(protBuff, "TCP/IP"))  reqProtocol = TCPIP;
    }
   // keep alive: should be stored in cdp, because is specified for every server separately
    key_len=sizeof(keep_alive_seconds);
    if (RegQueryValueEx(hKey, KeepAlive_str, NULL, NULL, (BYTE*)&keep_alive_seconds, &key_len)!=ERROR_SUCCESS)
      keep_alive_seconds=0;
    RegCloseKey(hKey);
  }
  return reqProtocol;
#else  // UNIX client
  return TCPIP;
#endif
}

static unsigned get_client_memory(void)
// Reads client memory size specification from the registry.
// Returns 0 if not specified or the number of 64 KB segments.
{ DWORD val = 5;  
#ifdef WINS
  char key[MAX_PATH];  HKEY hKey;
  strcpy(key, WB_inst_key);  strcat(key, "\\" WB_VERSION_STRING);
  if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, key, 0, KEY_READ_EX, &hKey)==ERROR_SUCCESS)
  { DWORD key_len=sizeof(val);
    if (RegQueryValueEx(hKey, ClientMem_str, NULL, NULL, (BYTE*)&val, &key_len)!=ERROR_SUCCESS)
      val=5;
    RegCloseKey(hKey);
  }
#endif
  return val;
}

#ifdef WINS
HWND WINAPI FindServerWindow(const char * fnd_server_name)
// Looks for the main (invisible) window.
// Window of the service is not found because it has no caption
{
#ifdef STOP  // this version cycles when stopping a direct slave after client crash
  HWND hWnd = GetWindow(GetDesktopWindow(), GW_CHILD);
  while (hWnd)
  { char buf[80];
    GetClassName(hWnd, buf, sizeof(buf));
    if (!stricmp(buf, "WBServerClass32"))
    { GetWindowText(hWnd, buf, sizeof(buf));
      char * p = buf;
      while (*p && *p!=':') p++;
      if (*p && p[1] && !my_stricmp(p+2, fnd_server_name))
        return hWnd;
    }
    hWnd=GetWindow(hWnd, GW_HWNDNEXT);
  }
  return 0;
#else
  char caption[30+OBJ_NAME_LEN];
  strcpy(caption, "602SQL Server: ");  strcat(caption, fnd_server_name);
  return FindWindow("WBServerClass32", caption);
#endif
}

#ifdef CLIENT_GUI
#if WBVERS<900
void status_text(unsigned txid)
{ char buf[80];
  LoadString(hKernelLibInst, txid, buf, sizeof(buf));
  Set_status_text(buf);
}
#endif
#endif
                                                             
static int WINAPI link_kernel_ex(const char * serv_name, int show_type, char * server_name, int & linkUser, cAddress * & pServerAddr)
{ int server_state;
  //client_error_message(NULL, "Server_name=%s.", serv_name);
 /* load client memory size specification and init memory management: */
  core_init(get_client_memory());
 // analyse serv_name, try local connection to the server unless server name starts with '*':
  if (serv_name==NULL) serv_name=NULLSTRING;
  while (*serv_name==' ') serv_name++;
  if (*serv_name=='*') goto network_link; 
#ifdef WINS
 // look for local server task:
  server_state = DetectLocalServer(serv_name);
  if (server_state)
  { if (server_state==2)  // runs locally, but without the IPC interface
      goto network_link; 
   // server task running locally
#if WBVERS<900
#ifdef CLIENT_GUI
    status_text(STBR_LINK_TASK);  
#endif
#endif
    goto local_server_link; 
  } 
 // check if the server name is registered and its path specified (and it can be started locally):
  if (*serv_name && serv_name[1]!=':')  /* try to translate the server name */
  { char alt_path[MAX_PATH];
    if (!GetPrimaryPath(serv_name, false, alt_path) &&
        !GetPrimaryPath(serv_name, true,  alt_path))
    { //client_error_message(NULL, "Primary path not found");
      goto network_link;  // not registered, but may be found on the net
    }
    if (!*alt_path) /* registered without path */ goto network_link;  
  }
  if (show_type < 0) goto network_link;  // used by the web client, which must not start the server
  if (show_type==2000) return 1;  // connecing disabled
 // prepare the command line for starting server: 
  { char cmdline[MAX_PATH];  
    if (!get_server_path(cmdline)) 
    { //client_error_message(NULL, "server path not found");
      goto network_link;  // no local server installation found
    }
    if (srv_Start_server_local(serv_name, 1000+show_type, NULL)!=KSE_OK) 
    { //client_error_message(NULL, "server start failed");
      return KSE_WINEXEC;
    }
  }
 // server started but I do not know if the IPC is enabled on it, wait for a defined server state:
  DWORD dwTickStart;  dwTickStart = GetTickCount();
  do
    server_state = DetectLocalServer(serv_name);
  while (!server_state && WinWait (dwTickStart, 15000));
  //client_error_message(NULL, "Detect2=%d", server_state);
  if (!server_state) return KSE_WINEXEC;
  if (server_state==2)  // runs locally, but without the IPC interface
    goto network_link; 

 local_server_link:
  strmaxcpy(server_name, serv_name, sizeof(tobjname));
  linkUser = PT_DIRECT;  /* info for interf_init */
  return KSE_OK;
#endif  // WINS

 network_link:
  HWND hLocClient=get_RV()->hClient;  /* hClient saved from auxRV */
 /* init networking, look for server: */
  if    (*serv_name == '*') serv_name++;   // may have jumped here from non-* branch
  while (*serv_name == ' ') serv_name++;
  int ccode = NetworkStart(get_client_protocol(serv_name));
  if (ccode != KSE_OK) return ccode;
  ccode = GetServerAddress(serv_name, &pServerAddr);
  if (ccode != KSE_OK) { NetworkStop(cds[0]);  return ccode; }
 // network server found
  get_RV()->hClient=hLocClient;
  strmaxcpy(server_name, serv_name, sizeof(tobjname));
  linkUser = PT_REMOTE;   /* info for interf_init */
  return KSE_OK;
}

#endif // WINS

#ifdef UNIX
static int WINAPI link_kernel_ex(const char * serv_name, int show_type, char * server_name, int & linkUser, cAddress * & pServerAddr)
// Only the network access supported
{ int ccode = NetworkStart(TCPIP);
  if (ccode != KSE_OK) return ccode;
  ccode = GetServerAddress(serv_name, &pServerAddr);
  if (ccode != KSE_OK) { NetworkStop(NULL);  return ccode; }
  strmaxcpy(server_name, serv_name, sizeof(tobjname));
  linkUser = PT_REMOTE;   /* info for interf_init */
  return KSE_OK;
}

#endif

#ifdef NETWARE
static int WINAPI link_kernel_ex(const char * serv_name, int show_type, char * server_name, int & linkUser, cAddress * & pServerAddr)
// Only the direct access supported
{ pServerAddr = NULL;
  linkUser = PT_DIRECT;
  return KSE_OK; 
}  
#endif
/////////////////////// client connection interface ///////////////////////////////////
#if WBVERS>=1100
CFNC DllKernel int WINAPI cd_connect_abi11(cdp_t cdp, const char * serv_name, int show_type)
#elif WBVERS>=1000
CFNC DllKernel int WINAPI cd_connect_abi10(cdp_t cdp, const char * serv_name, int show_type)
#elif WBVERS==950
CFNC DllKernel int WINAPI cd_connect_abi95(cdp_t cdp, const char * serv_name, int show_type)
#else  // < 9.5
CFNC DllKernel int WINAPI cd_connect(cdp_t cdp, const char * serv_name, int show_type)
#endif
{ int        linkUser;         // server access
  cAddress * pServerAddr;      // server address
  tobjname   server_name = ""; // server name (remains empty on netware)
#ifdef WINS
  HCURSOR oldcurs = SetCursor(LoadCursor(NULL, IDC_WAIT));
#endif
  cdp_init(cdp);
  int res = link_kernel_ex(serv_name, show_type, server_name, linkUser, pServerAddr); 
  if (res==KSE_OK) res=interf_init_ex(cdp, linkUser, pServerAddr, server_name); 
#ifdef WINS
  SetCursor(oldcurs);
#endif
  return res;
}

#if WBVERS>=1100 // .def file compatibility
CFNC DllKernel int WINAPI cd_connect_abi10(cdp_t cdp, const char * serv_name, int show_type)
{ return KSE_BAD_VERSION; }
#endif

#if WBVERS==1000 // .def file compatibility
CFNC DllKernel int WINAPI cd_connect_abi11(cdp_t cdp, const char * serv_name, int show_type)
{ return KSE_BAD_VERSION; }
#endif

CFNC DllKernel void WINAPI cd_disconnect(cdp_t cdp)
{ cd_interf_close(cdp); }

/////////////////////// obsolete connection interface /////////////////////////////////
// Uses static variables, may have problems when many threads try to connect at the same time
static int        stat_linkUser;         // server access defined in link_kernel, used by interf_init
static cAddress * stat_pServerAddr;      // server address found in link_kernel, used by interf_init
static tobjname   stat_server_name = ""; // server selected by link_kernel, used by interf_init (remains empty on netware)

CFNC DllKernel int WINAPI interf_init(cdp_t cdp, int user)
{ return interf_init_ex(cdp, user ? user : stat_linkUser, stat_pServerAddr, stat_server_name); }

CFNC DllKernel int WINAPI link_kernel(const char * serv_name, int show_type)
{ return link_kernel_ex(serv_name, show_type, stat_server_name, stat_linkUser, stat_pServerAddr); }

CFNC DllKernel void  WINAPI interf_close(void)
{ cd_interf_close(GetCurrTaskPtr()); }

CFNC DllKernel void WINAPI unlink_kernel(void)
{ } // empty

#if 0 //def WINS

#if WBVERS>=900
#define SZM_UNLINK WM_USER+500+211
#endif

void cd_unlink_kernel(cdp_t cdp)  // called by interf_close
{ /* must be before closing kernel, otherwise kernel in closed immediately
     and following corefree will crash. */
	HWND hServerWnd = FindServerWindow(cdp->conn_server_name);   // window of the service is not found because it has no caption
	if (hServerWnd) PostMessage(hServerWnd, SZM_UNLINK, 0, 0);
}
#endif

/////////////////////// transmission control functions ////////////////////////////////
#define IS_AN_ERROR(num) ((num) >= 128)

CFNC DllKernel int WINAPI cd_Sz_error(cdp_t cdp)
{ if (cdp)
  { if (cdp->RV.holding & 2) return NOT_ANSWERED;
    if (IS_AN_ERROR(cdp->RV.report_number)) return cdp->RV.report_number;
  }  
  return ANS_OK;
}

CFNC DllKernel int WINAPI cd_Sz_warning(cdp_t cdp)
{ if (cdp->RV.holding & 2) return NOT_ANSWERED;
  if (IS_AN_ERROR(cdp->RV.report_number)) return internal_IS_ERROR;
  return cdp->RV.report_number;
}

CFNC DllKernel BOOL WINAPI cd_answered(cdp_t cdp)
{ return !(cdp->RV.holding & 3); }

CFNC DllKernel BOOL WINAPI cd_concurrent(cdp_t cdp, BOOL state)
// concurrency not supported on Linux and for direct connections
// Does not wait for answer when turning wbtrue->wbfalse.
{ 
#ifndef LINUX
  if (cdp->in_use==PT_DIRECT)
#endif
    return TRUE;  // error
  cdp->in_concurrent = state!=0; 
  return FALSE;
}

CFNC DllKernel void WINAPI cd_start_package(cdp_t cdp)
{ 
  if (!cdp->in_package)
    cdp->in_package=wbtrue;
}

void write_ans_proc(cdp_t cdp, answer_cb * ans)
// Called when receiving the answer.
// When communicating with the local server, it is called by the transmit_req.
{ cdp->RV.report_number=ans->return_code;
  if (IS_AN_ERROR(cdp->RV.report_number)) /* must not write answer when error occured */
    cdp->last_error_is_from_client=wbfalse;
  else   // copying the answer data into the buffers provided by the request:
  { tptr p=ans->return_data;
    for (unsigned i=0;  i<cdp->ans_ind;  i++)
    { uns32 size = cdp->ans_type[i];
      if (size == ANS_SIZE_VARIABLE) 
      { size=*(uns32*)p;  p+=sizeof(uns32);
        memcpy(cdp->ans_array[i], p, size);             
      }
      else if (size==ANS_SIZE_DYNALLOC)
      { tptr block;
        size=*(uns32*)p;  p+=sizeof(uns32);
        if (size)
        { block=(tptr)corealloc(size, 1);
          if (!block) { client_error(cdp, OUT_OF_APPL_MEMORY); }
          else memcpy(block, p, size);
          *(tptr*)cdp->ans_array[i]=block;
        }
        else *(tptr*)cdp->ans_array[i]=NULL;
      }
      else if (size==ANS_SIZE_PREVITEM)
      { size = *(uns32*)(p-sizeof(uns32));
        memcpy(cdp->ans_array[i], p, size);
      }
      else  /* fixed size */
        memcpy(cdp->ans_array[i], p, size);
      p+=size;
    }
  }
  cdp->ans_ind=0;  // clear the answer registration
  if (cdp->in_concurrent)
    cdp->RV.holding &= ~3;
  else
    cdp->RV.holding &= ~2; 
  // answered, but unless cdp->in_concurrent messages are still blocked
#ifndef PEEKING_DURING_WAIT
#ifndef SINGLE_THREAD_CLIENT
  if (cdp->in_use==PT_REMOTE)
    SetLocalAutoEvent(&cdp->hSlaveSem);
#endif
#endif
}

#if WBVERS>=900
CFNC DllKernel void WINAPI client_status_nums(trecnum num1, trecnum num2)
{ if (notification_callback)
  { trecnum nums[2];
    nums[0]=num1;  nums[1]=num2;
    (*notification_callback)(NULL, NOTIF_PROGRESS, nums);
  }
}
#endif

#ifdef WINS
#include "dircomm.cpp"

static void transmit_req(cdp_t cdp)
// Transmits the request to the local server via shared memory. Then waits for the answer and processes it.
{ t_fragmented_buffer frbu(cdp->get_request_fragments(), cdp->get_request_fragments_count());
 // check the connection state:
	if (cdp->dcm.IsServerDownNotReported() || cdp->dcm.IsServerDownReported())
  // Connection to server not created or closed
	{	client_error(cdp, CONNECTION_LOST);
    if (cdp->dcm.IsServerDownNotReported())
    { cdp->dcm.SetServerDownReported();
#if WBVERS<900
      if (cdp->RV.hClient)
      { char str[80];
        LoadString(hKernelLibInst, SERVER_TERM, str,  sizeof(str));
        MessageBox(GetParent (cdp->RV.hClient), str, "", MB_OK | MB_ICONSTOP | MB_APPLMODAL);
      }
#endif
    }
		goto cancel_request;
	}
 // copy and release the (complete) request data:
  frbu.append_byte(OP_END);
  frbu.reset_pointer();
  int total_length;
  total_length = frbu.total_length();
  
  if (total_length <= cdp->dcm.request_part_size)   // single part
  { frbu.copy((char*)&cdp->dcm.rqst->req, total_length);
    cdp->dcm.SetPartialReq(false);
   // transact with the server:
    if (!cdp->dcm.SendAndWait())  // server process terminated suddenly
      {	client_error(cdp, CONNECTION_LOST);  cdp->dcm.SetServerDownReported();  goto cancel_request; }
  }   // multipart 
  else if (cdp->improved_multipart_comm())  // new multipart request
  { bool initial_part = true;
    do
    { int send_step = total_length < cdp->dcm.request_part_size ? total_length : cdp->dcm.request_part_size;
      char * dest = (char*)&cdp->dcm.rqst->req;
      if (initial_part)
      { *(uns32*)dest = total_length;
        dest += sizeof(uns32);
        send_step -= sizeof(uns32);
        initial_part = false;
      }  
      frbu.copy(dest, send_step);
      cdp->dcm.SetMultipartReq(true);
     // transact with the server:
      if (!cdp->dcm.SendAndWait())  // server process terminated suddenly
        { client_error(cdp, CONNECTION_LOST);  cdp->dcm.SetServerDownReported();  goto cancel_request; }
      total_length -= send_step;
    } while (total_length);
  }
  else  // old partial request
  { do
    { int send_step = total_length < cdp->dcm.request_part_size ? total_length : cdp->dcm.request_part_size;
      frbu.copy((char*)&cdp->dcm.rqst->req, send_step);
      cdp->dcm.SetPartialReq(send_step < total_length);  // non-last part
     // transact with the server:
      if (!cdp->dcm.SendAndWait())  // server process terminated suddenly
        {	client_error(cdp, CONNECTION_LOST);  cdp->dcm.SetServerDownReported();  goto cancel_request; }
      total_length -= send_step;
    } while (total_length);
  }
  cdp->free_request();
 // moving intra-request data:
  while (cdp->dcm.dir_answer->flags==17)   
  { BOOL reading=cdp->dcm.dir_answer->return_data[0];
    if (reading==2)   /* sending status nums */
    {
#if WBVERS<900
#ifdef CLIENT_GUI
      trecnum num1=*(trecnum*)(cdp->dcm.dir_answer->return_data+1),
              num2=*(trecnum*)(cdp->dcm.dir_answer->return_data+1+sizeof(trecnum));
      Set_client_status_nums(cdp, num1, num2);
#endif
#else  // since 9.0
      client_status_nums(((trecnum*)(cdp->dcm.dir_answer->return_data+1))[0], ((trecnum*)(cdp->dcm.dir_answer->return_data+1))[1]);
#endif
    }
#if WBVERS>=900
    else if (reading==3)   // more general communication
    { if (notification_callback)
      { int notif_type = cdp->dcm.dir_answer->return_data[1];
        (*notification_callback)(cdp, notif_type, 
                  notif_type==NOTIF_SERVERDOWN ? cdp->locid_server_name : cdp->dcm.dir_answer->return_data+2);
      }
    }
#endif
    else if (cdp->auxptr)
    { sig32   insize  = *(sig32*)(cdp->dcm.dir_answer->return_data+1);
      char *  indata  = cdp->dcm.dir_answer->return_data+1+sizeof(sig32);
      sig32 * outsize = (sig32*)cdp->dcm.rqst->req.areq;
      char *  outdata = cdp->dcm.rqst->req.areq+sizeof(sig32);
      if (reading) *outsize=((t_container *)cdp->auxptr)->read(outdata, insize);
      else         *outsize=((t_container *)cdp->auxptr)->write(indata, insize);
    }
    else break;
   // transact with the server:
    if (!cdp->dcm.SendAndWait())  // server process terminated suddenly
	  {	client_error(cdp, CONNECTION_LOST);
      cdp->dcm.SetServerDownReported();
		  goto cancel_request;
	  }
  }
 // complete the answer:
  char * answer_copy;
  answer_copy = NULL;  
  if (cdp->dcm.IsPartialAns())
  { answer_copy = (char*)corealloc(ANSWER_PREFIX_SIZE, 188);
    if (!answer_copy) { client_error(cdp, OUT_OF_MEMORY);  goto cancel_request; }
    memcpy(answer_copy, cdp->dcm.dir_answer, ANSWER_PREFIX_SIZE);
    unsigned answer_copy_len = ANSWER_PREFIX_SIZE;
    do 
    { answer_copy = (char*)corerealloc(answer_copy, answer_copy_len+cdp->dcm.answer_part_size);
      if (!answer_copy) { client_error(cdp, OUT_OF_MEMORY);  goto cancel_request; }
      memcpy(answer_copy+answer_copy_len, cdp->dcm.dir_answer->return_data, cdp->dcm.answer_part_size);
      answer_copy_len+=cdp->dcm.answer_part_size;
      if (!cdp->dcm.IsPartialAns()) break;
     // transact with the server:
      if (!cdp->dcm.SendAndWait())  // server process terminated suddenly
	    {	client_error(cdp, CONNECTION_LOST);
        cdp->dcm.SetServerDownReported();
        corefree(answer_copy);
		    goto cancel_request;
	    }
    } while (true);
  }
  else if (cdp->dcm.IsMultipartAns())
  {// allocate space for the full answer:
    uns32 total_length = *(uns32*)cdp->dcm.dir_answer->return_data;  // net data including single instance of the header
    answer_copy = (char*)corealloc(total_length, 188);
    if (!answer_copy) { client_error(cdp, OUT_OF_MEMORY);  goto cancel_request; }
   // copy the header and the 1st part of data, skipping the inserted total length info:
    uns32 partsize = cdp->dcm.answer_part_size-sizeof(uns32);
    memcpy(answer_copy, cdp->dcm.dir_answer, ANSWER_PREFIX_SIZE);
    memcpy(answer_copy+ANSWER_PREFIX_SIZE, cdp->dcm.dir_answer->return_data+sizeof(uns32), partsize);
    uns32 copied = ANSWER_PREFIX_SIZE+partsize;  
    total_length -= ANSWER_PREFIX_SIZE+partsize;
    do
    {// transact with the server:
      if (!cdp->dcm.SendAndWait())  // server process terminated suddenly
	    {	client_error(cdp, CONNECTION_LOST);
        cdp->dcm.SetServerDownReported();
        corefree(answer_copy);
		    goto cancel_request;
	    }
     // append the next part:
      partsize = cdp->dcm.answer_part_size;  // max data in the part (header not included)
      if (partsize > total_length) partsize=total_length;
      memcpy(answer_copy+copied, cdp->dcm.dir_answer->return_data, partsize);
      copied += partsize;  total_length -= partsize;
    } while (total_length);
  }
 // process the answer:
  write_ans_proc(cdp, answer_copy ? (answer_cb*)answer_copy : cdp->dcm.dir_answer);
  corefree(answer_copy);
  cdp->RV.holding=0;  // answered, not blocked
  return;
 cancel_request:
  cdp->free_request();  // may have already been called (if error ocurrend when receiving the answer), no problem
  cdp->ans_ind=0;
  cdp->RV.holding=0;  // answered, not blocked
}
#endif

int task_switch_enabled = 1;

CFNC DllKernel void WINAPI process_notifications(cdp_t cdp)
{
#if WBVERS>=900
  if (cdp->RV.holding & 4)  // progress report from server delivered
  { if (notification_callback)
    { (*notification_callback)(cdp, cdp->notification_type, 
                cdp->notification_type==NOTIF_SERVERDOWN ? cdp->locid_server_name : cdp->notification_parameter);
      if (cdp->notification_parameter) // allocated by the receiver
      { corefree(cdp->notification_parameter);  
        cdp->notification_parameter=NULL;
      }
    }
    cdp->RV.holding &= ~4;
  }
#endif
}

CFNC DllKernel void WINAPI cd_wait_for_answer(cdp_t cdp)
{// waiting for the answer:
  if (!(cdp->RV.holding & 3)) 
    return;  // no request is pending
#if WBVERS<900
#ifdef PEEKING_DURING_WAIT
#ifdef ORIGINAL_WAIT
  HCURSOR origcurs;  origcurs=GetCursor();
  do
  { MSG msg;
    if (PeekMessage(&msg, 0, 0, WM_PAINT-1, PM_REMOVE) ||
        PeekMessage(&msg, 0, WM_PAINT+1, 0xffff, PM_REMOVE))
    { if (cdp->RV.holding & 2)
      { if (task_switch_enabled>0)
        { TranslateMessage(&msg);       /* Translates virtual key codes         */
          DispatchMessage(&msg);
        }
        // RCS!!:
        else if (msg.message>=WM_USER+100)
          PostMessage(msg.hwnd, msg.message, msg.wParam, msg.lParam);
        SetCursor(origcurs);  // without this the WAIT-cursor diappears
      }
      else PostMessage(msg.hwnd, msg.message, msg.wParam, msg.lParam);
      /* without this if ... else fast clicking the scroll bar would cause
         the stack to overflow! */
    }
/* save-restore_run must NOT be called if function called when remote client
   waits for answer. The answer from kernel would otherwise be written into
   the hyperbuf which would be overwritten on restore_run.
*/
  }
  while (cdp->RV.holding & 2);
#else
  while (cdp->RV.holding & 2)
  { MSG msg;
    if (PeekMessage(&msg, 0, WM_KEYDOWN, WM_KEYDOWN, PM_NOREMOVE))
    { if (msg.wParam==VK_CONTROL)
      { TranslateMessage(&msg);
        DispatchMessage(&msg);
        PeekMessage(&msg,0,WM_KEYDOWN, WM_KEYDOWN, PM_REMOVE|PM_NOYIELD);
      }
      else if (msg.wParam==VK_CANCEL)
      { cdp->RV.break_request=wbtrue;
        cd_Break(cdp);
        PeekMessage(&msg,0,WM_KEYDOWN, WM_KEYDOWN, PM_REMOVE|PM_NOYIELD);
      }
    }
  }
#endif
#else  // client without PeekMessage
#ifdef SINGLE_THREAD_CLIENT
  do 
  { if (!cdp->pRemAddr->ReceiveWork(cdp, -1)) 
      ReceiveUnprepare(cdp);
  } while (cdp->RV.holding & 2);
#else
  do
  { WaitLocalAutoEvent(&cdp->hSlaveSem, INFINITE);
    if (cdp->RV.holding & 4)  // progress report from server delivered
    { 
#ifdef CLIENT_GUI      
      MSG msg;
      Set_client_status_nums(cdp, cdp->server_progress_num1, cdp->server_progress_num2);
      while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
      { TranslateMessage(&msg);
        DispatchMessage(&msg);
      }
#endif
      cdp->RV.holding &= ~4;
    }
  } while (cdp->RV.holding & 2);
#endif
#endif

#else  // since 9.0
  do 
  { 
#ifdef SINGLE_THREAD_CLIENT
    if (!cdp->pRemAddr->ReceiveWork(cdp, -1)) 
      ReceiveUnprepare(cdp);
#else
    WaitLocalAutoEvent(&cdp->hSlaveSem, INFINITE);
#endif
    if (!cdp->pRemAddr)  // connection broken when waiting
      { client_error(cdp, CONNECTION_LOST);  break; }
    process_notifications(cdp);
  } while (cdp->RV.holding & 2);
#endif  // since 9.0
  cdp->RV.holding=0;  // answered and not blocked
}

static void transmit_req_rem(cdp_t cdp)
// Transmits the request to the server via a network connection.
// Then waits for the answer being received and processed.
{ BOOL fSend;  unsigned i;
  t_commenc_pseudo commenc_cpy;  // for decrypting 
  t_fragmented_buffer frbu(cdp->get_request_fragments(), cdp->get_request_fragments_count());
  frbu.append_byte(OP_END);
  if (!cdp->pRemAddr) // client has been disconnected by the UNIX server via DT_CLOSE_CONN (must not send the request)
  { client_error(cdp, CONNECTION_LOST);  client_log("connection lost in transmit_req_rem"); 
    goto cancel_request; 
  }
 // copy and release the request data:
  if (cdp->commenc)  // encrypt
  { if (cdp->get_persistent_part())  // must decrypt after sending
      commenc_cpy = *cdp->commenc;
    for (i=0;  i<cdp->get_request_fragments_count();  i++)
      cdp->commenc->encrypt((unsigned char *)cdp->get_request_fragments()[i].contents, cdp->get_request_fragments()[i].length);
  }
  fSend = SendRequest(cdp->pRemAddr, &frbu);
  if (cdp->commenc && cdp->get_persistent_part())  // decrypt (get_persistent_part() must be called before free_request() )
    for (i=0;  i<cdp->get_request_fragments_count();  i++)
      commenc_cpy.encrypt((unsigned char *)cdp->get_request_fragments()[i].contents, cdp->get_request_fragments()[i].length);
  cdp->free_request();
  if (!fSend) // send error, occurs when client has been disconnected by the Win32 server
  { client_error(cdp, CONNECTION_LOST);  client_log("not sended in transmit_req_rem");  
    goto cancel_request; 
  }
  if (!cdp->in_concurrent)  // holding==3, will be cleared by write_ans_proc
    cd_wait_for_answer(cdp);
  return;
 cancel_request:
  cdp->free_request();  // may have already been called, no problem
  cdp->ans_ind=0;
  cdp->RV.holding=0;  // answered and not blocked
}

#if WBVERS>=900
t_notification_callback * notification_callback = NULL;

CFNC DllKernel void WINAPI Set_notification_callback(t_notification_callback *CallBack)
{ notification_callback=CallBack;
}
#endif

CFNC DllKernel BOOL WINAPI cd_is_dead_connection(cdp_t cdp)
{
#ifndef LINUX
  if (cdp->in_use==PT_DIRECT)
  	return cdp->dcm.IsServerDownNotReported() || cdp->dcm.IsServerDownReported();
#endif
  if (cdp->in_use==PT_REMOTE)
    return cdp->pRemAddr==NULL;
  return TRUE;
}

CFNC DllKernel void WINAPI cd_send_package(cdp_t cdp)
/* called explicitly in the in_package state or automat. after each request */
{ cdp->in_package=wbfalse;
  if (!cdp->get_request_fragments_count())   /* empty package, do not send */
  { cdp->RV.report_number=ANS_OK;
    return;
  }
 /* transmitting */
  cdp->RV.holding|=3;   /* only write_ans_proc will reset it, not answered & blocked */
#ifndef LINUX
  if (cdp->in_use==PT_DIRECT)
  	transmit_req(cdp);
  else 
#endif
  if (cdp->in_use==PT_REMOTE)
    transmit_req_rem(cdp);
  if (cdp->in_concurrent) return;
  if (IS_AN_ERROR(cdp->RV.report_number))
    if (ERR_WITH_MSG(cdp->RV.report_number))
      Get_server_error_suppl(cdp, cdp->errmsg);
}

void cd_t::add_request_fragment(char * data, uns32 length, bool dealloc)
{
  req_array[req_ind].contents = data;
  req_array[req_ind].length   = length;
  req_array[req_ind].deallocate_contents = dealloc;
  req_ind++;
}

tptr get_space_op(cd_t * cdp, unsigned size, uns8 op)
// Allocate space for a request or a fragment of a request.
// Each fragment has at its end space for OP_END (not included in [length])
// Stores [op] in the 1st byte of the request, returns pointer after it.
{ tptr p;
  if (!cdp || cdp->in_use==PT_EMPTY) return NULL;
  if (cdp->RV.holding & 3)
    { client_error(cdp, REQUEST_NESTING);  return NULL; }
  if (!cdp->default_frame_busy && (size <= DEFAULT_RQ_FRAME_SIZE))
  { p = cdp->default_rq_frame;  cdp->default_frame_busy=true; 
    cdp->add_request_fragment(p, size, false);
  }
  else
  { p=(tptr)corealloc(size + 1, 15);  // always allocating 1 byte more for terminating OP_END
    if (!p) { client_error(cdp, OUT_OF_APPL_MEMORY); return NULL; }
    cdp->add_request_fragment(p, size, true);
  }
  *p = op;
  return p + 1;
}

void cd_t::free_request(void)
{
  for (int i=0;  i<req_ind;  i++)
    if (req_array[i].deallocate_contents)
       corefree(req_array[i].contents);
  req_ind=0;     
  default_frame_busy=has_persistent_part=false;
}
  
static BOOL WINAPI cond_send(cdp_t cdp)
{// ignore, if [in_package]: 
  if (cdp->in_package && cdp->get_request_fragments_count()<MAX_PACKAGED_REQS-1 && cdp->ans_ind<MAX_PACKAGED_REQS-1) // space for double req and double-ans
    return FALSE;
 /* copy of cd_send_package: */
 /* transmitting */
  cdp->RV.holding|=3;   /* only write_ans_proc will reset it, not answered & blocked */
#ifndef LINUX
  if (cdp->in_use==PT_DIRECT)
  	transmit_req(cdp);
  else 
#endif
  if (cdp->in_use==PT_REMOTE)
    transmit_req_rem(cdp);
 /* end of cd_send_package copy */
  if (cdp->in_concurrent) return FALSE;
  if (IS_AN_ERROR(cdp->RV.report_number))
  { if (ERR_WITH_MSG(cdp->RV.report_number))
      Get_server_error_suppl(cdp, cdp->errmsg);
    return TRUE;
  }
  return FALSE;
}

#ifdef WINS
void CALLBACK ClientTimerProc(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime)
{ for (int i=0;  i<MAX_MAX_TASKS;  i++) 
    if (cds[i] && cds[i]->timer == idEvent)
      if (cds[i]->in_use==PT_REMOTE)
      { if (!cds[i]->RV.holding)
          if (get_space_op(cds[i], 1, OP_END))
            cond_send(cds[i]);
        break;            
      }
}
#endif

/******************************* kernel services ****************************/
static BOOL kernel_control(cdp_t cdp, uns8 command, uns8 data)
{ tptr p=get_space_op(cdp, 1+2, OP_CNTRL);
  if (p==NULL) return TRUE;
  *p=command; p[1]=data;
  return cond_send(cdp);
}

CFNC DllKernel BOOL WINAPI cd_GetSet_fil_size(cdp_t cdp, t_oper operation, uns32 * size)
{ tptr p=get_space_op(cdp, 1+1+1+sizeof(uns32), OP_CNTRL);
  if (p==NULL) return TRUE;
  *p=SUBOP_SET_FILSIZE;  p++;
  *p=(uns8)operation;    p++;
  if (operation==OPER_SET)
    *(uns32*)p=*size;
  else
    REGISTER_ANSWER(size, sizeof(uns32));
  return cond_send(cdp);
}

CFNC DllKernel BOOL WINAPI cd_Restart_server(cdp_t cdp)
{ tptr p=get_space_op(cdp, 1+1, OP_CNTRL);
  if (p==NULL) return TRUE;
  *p=SUBOP_RESTART;
  return cond_send(cdp);
}

CFNC DllKernel BOOL WINAPI cd_Crash_server(cdp_t cdp)
{ tptr p=get_space_op(cdp, 1+1, OP_CNTRL);
  if (p==NULL) return TRUE;
  *p=SUBOP_CRASH;
  return cond_send(cdp);
}

#if WBVERS<900
CFNC DllKernel BOOL WINAPI cd_Stop_server(cdp_t cdp)
{ tptr p=get_space_op(cdp, 1+1, OP_CNTRL);
  if (p==NULL) return TRUE;
  *p=SUBOP_STOP;
  return cond_send(cdp);
}
#endif

CFNC DllKernel BOOL WINAPI cd_Check_indexes(cdp_t cdp, int extent, BOOL repair, uns32 * index_err, uns32 * cache_err)
{ tptr p=get_space_op(cdp, 1+1+2*sizeof(uns32), OP_CNTRL);
  if (p==NULL) return TRUE;
  *p=SUBOP_TABLE_INTEGRITY;  p++;
  *(uns32*)p=extent;  p+=sizeof(uns32);
  *(uns32*)p=repair;
  REGISTER_ANSWER(index_err, sizeof(uns32));
  REGISTER_ANSWER(cache_err, sizeof(uns32));
  return cond_send(cdp);
}

CFNC DllKernel BOOL WINAPI cd_GetSet_fil_blocks(cdp_t cdp, t_oper operation, uns32 * size)
{ tptr p=get_space_op(cdp, 1+1+1+sizeof(uns32), OP_CNTRL);
  if (p==NULL) return TRUE;
  *p=SUBOP_SET_FILSIZE64;  p++;
  *p=(uns8)operation;      p++;
  if (operation==OPER_SET)
    *(uns32*)p=*size;
  else
    REGISTER_ANSWER(size, sizeof(uns32));
  return cond_send(cdp);
}

CFNC DllKernel BOOL WINAPI Unlock_user(BOOL objects)
{ return kernel_control(GetCurrTaskPtr(), SUBOP_USER_UNLOCK, (uns8)objects); }

CFNC DllKernel BOOL WINAPI cd_Lock_server(cdp_t cdp)
{ return kernel_control(cdp, SUBOP_LOCK, 0); }

CFNC DllKernel void WINAPI cd_Unlock_server(cdp_t cdp)
{ kernel_control(cdp, SUBOP_UNLOCK, 0); }

CFNC DllKernel BOOL WINAPI cd_Operation_limits(cdp_t cdp, t_oper_limits data)
{ return kernel_control(cdp, SUBOP_WORKER_TH, (uns8)data); }

CFNC DllKernel BOOL WINAPI cd_Reset_replication(cdp_t cdp)
{ return kernel_control(cdp, SUBOP_REPLIC, 0); }

CFNC DllKernel void WINAPI cd_Enable_task_switch(cdp_t cdp, BOOL enable)
{ if (enable) task_switch_enabled++;
  else task_switch_enabled--;
  kernel_control(cdp, SUBOP_TSE, (uns8)enable);
}

CFNC DllKernel BOOL WINAPI Back_count(uns32 * count)
{ tptr p;  cdp_t cdp = GetCurrTaskPtr();
  p=get_space_op(cdp, 1+2, OP_CNTRL);
  if (p==NULL) return TRUE;
  *p=SUBOP_BACK_COUNT;
  *count=0;
  cdp->ans_array[cdp->ans_ind]=count;
  cdp->ans_type [cdp->ans_ind]=sizeof(count);
  cdp->ans_ind++;
  return cond_send(cdp);
}

CFNC DllKernel BOOL WINAPI cd_Internal_1(cdp_t cdp, uns8 * buf)
{ tptr p=get_space_op(cdp, 1+1, OP_CNTRL);
  if (p==NULL) return TRUE;
  *p=SUBOP_ADD_LIC_1;
  cdp->ans_array[cdp->ans_ind]=buf;
  cdp->ans_type [cdp->ans_ind]=4*sizeof(uns32);
  cdp->ans_ind++;
  return cond_send(cdp);
}

CFNC DllKernel BOOL WINAPI cd_Internal_2(cdp_t cdp, uns32 count, const uns8 * buf)
{ tptr p=get_space_op(cdp, 1+1+sizeof(uns32)+MD5_SIZE, OP_CNTRL);
  if (p==NULL) return TRUE;
  *p=SUBOP_ADD_LIC_2;  p++;
  *(uns32*)p=count;  p+=sizeof(uns32);
  memcpy(p, buf, MD5_SIZE);
  return cond_send(cdp);
}

CFNC DllKernel BOOL WINAPI Get_server_error_suppl(cdp_t cdp, tptr buf)
{ tptr p=get_space_op(cdp, 1+1, OP_CNTRL);
  int saved_rep = cdp->RV.report_number;
  if (p==NULL) return TRUE;
  *p=SUBOP_GET_ERR_MSG;
  cdp->ans_array[cdp->ans_ind]=buf;
  cdp->ans_type [cdp->ans_ind]=sizeof(tobjname);
  cdp->ans_ind++;
  BOOL res= cond_send(cdp);
  cdp->RV.report_number=saved_rep;
  return res;
}

CFNC DllKernel BOOL WINAPI Get_server_error_context(cdp_t cdp, int level, int * type, int * par1, int * par2, int * par3, int * par4)
{ tptr p=get_space_op(cdp, 1+1+sizeof(uns32), OP_CNTRL);
  int saved_rep = cdp->RV.report_number;
  if (p==NULL) return TRUE;
  *p=SUBOP_GET_ERR_CONTEXT;  p++;
  *(uns32*)p=level;
  REGISTER_ANSWER(type, sizeof(int));
  REGISTER_ANSWER(par1, sizeof(int));
  REGISTER_ANSWER(par2, sizeof(int));
  REGISTER_ANSWER(par3, sizeof(int));
  REGISTER_ANSWER(par4, sizeof(int));
  BOOL res= cond_send(cdp);
  cdp->RV.report_number=saved_rep;
  return res;
}

CFNC DllKernel BOOL WINAPI Get_server_error_context_text(cdp_t cdp, int level, char * buffer, int buflen)
{ tptr p=get_space_op(cdp, 1+1+sizeof(uns32), OP_CNTRL);
  int saved_rep = cdp->RV.report_number;
  if (p==NULL) return TRUE;
  *p=SUBOP_GET_ERR_CONTEXT_TEXT;  p++;
  *(uns32*)p=level;
  char * answer;
  REGISTER_ANSWER(&answer, ANS_SIZE_DYNALLOC);
  BOOL res = cond_send(cdp);
  cdp->RV.report_number=saved_rep;
  if (res) return TRUE;
  strmaxcpy(buffer, answer, buflen);
  corefree(answer);
  return res;
}

CFNC DllKernel BOOL WINAPI cd_Kill_user(cdp_t cdp, int client_number)
{ tptr p=get_space_op(cdp, 1+1+sizeof(uns16), OP_CNTRL);
  if (p==NULL) return TRUE;
  *(p++)=SUBOP_KILL_USER;
  *(uns16*)p=(uns16)client_number;
  return cond_send(cdp);
}

CFNC DllKernel BOOL WINAPI cd_Send_client_time_zone(cdp_t cdp)
{ get_time_zone_info();  // reload the current state
  tptr p=get_space_op(cdp, 1+1+sizeof(sig32), OP_CNTRL);
  if (p==NULL) return TRUE;
  *(p++)=SUBOP_SET_TZD;
  *(sig32*)p=client_tzd;
  return cond_send(cdp);
}

CFNC DllKernel BOOL WINAPI cd_Break_user(cdp_t cdp, int client_number)
{ tptr p=get_space_op(cdp, 1+1+sizeof(sig32), OP_CNTRL);
  if (p==NULL) return TRUE;
  *(p++)=SUBOP_BREAK_USER;
  *(sig32*)p=client_number;
  return cond_send(cdp);
}

CFNC DllKernel BOOL WINAPI cd_waiting(cdp_t cdp, sig32 timeout)
{ tptr p=get_space_op(cdp, 1+1+sizeof(sig32), OP_CNTRL);
  if (p==NULL) return TRUE;
  *(p++)=SUBOP_SET_WAITING;
  *(sig32*)p=timeout;
  return cond_send(cdp);
}

CFNC DllKernel BOOL WINAPI cd_Weak_link(cdp_t cdp)
{ tptr p=get_space_op(cdp, 1+1, OP_CNTRL);
  if (p==NULL) return TRUE;
  *p=SUBOP_SET_WEAK_LINK;
  return cond_send(cdp);
}

CFNC DllKernel BOOL WINAPI cd_Check_indices(cdp_t cdp, ttablenum tbnum, sig32 * result, sig32 * index_number)
{ tptr p=get_space_op(cdp, 1+1+sizeof(ttablenum), OP_CNTRL);
  if (p==NULL) return TRUE;
  *(p++)=SUBOP_CHECK_INDECES;
  *(ttablenum*)p=tbnum;
  *result=-1;  *index_number=-1;  // used when server returns error
  REGISTER_ANSWER(result,       sizeof(sig32));
  REGISTER_ANSWER(index_number, sizeof(sig32));
  return cond_send(cdp);
}

CFNC DllKernel BOOL WINAPI cd_Remove_lock(cdp_t cdp, lockinfo * llock)
{ tptr p=get_space_op(cdp, 1+1+sizeof(lockinfo), OP_CNTRL);
  if (p==NULL) return TRUE;
  *(p++)=SUBOP_REMOVE_LOCK;
  *(lockinfo *)p=*llock;
  return cond_send(cdp);
}

CFNC DllKernel BOOL WINAPI cd_Set_sql_option(cdp_t cdp, uns32 optmask, uns32 optval)
{ tptr p=get_space_op(cdp, 1+1+2*sizeof(uns32), OP_CNTRL);
  if (p==NULL) return TRUE;
  *p=SUBOP_SQLOPT;  p++;
  *(uns32*)p=optmask;  p+=sizeof(uns32);
  *(uns32*)p=optval;   
  return cond_send(cdp);
}

CFNC DllKernel BOOL WINAPI cd_Get_sql_option(cdp_t cdp, uns32 * optval)
{ tptr p=get_space_op(cdp, 1+1, OP_CNTRL);
  if (p==NULL) return TRUE;
  *p=SUBOP_SQLOPT_GET; 
  REGISTER_ANSWER(optval, sizeof(uns32));
  return cond_send(cdp);
}

CFNC DllKernel BOOL WINAPI cd_Backup_database_file(cdp_t cdp, const char * file_name)
{ tptr p;  
  p=get_space_op(cdp, 1+1+(unsigned)strlen(file_name)+1, OP_CNTRL);
  if (p==NULL) return TRUE;
  *(p++)=SUBOP_BACKUP;
  strcpy(p, file_name);
  return cond_send(cdp);
}

CFNC DllKernel BOOL WINAPI cd_Mask_duplicity_error(cdp_t cdp, BOOL mask)   
{ tptr p=get_space_op(cdp, 1+1+1, OP_CNTRL);
  if (p==NULL) return TRUE;
  *p=SUBOP_MASK_DUPLICITY;  p++;
  *(uns8*)p=(uns8)mask;  
  return cond_send(cdp);
}

#ifdef NEVER
CFNC DllKernel BOOL WINAPI Start_backup(const char * pathname)
{ tptr p;  int ln;  cdp_t cdp = GetCurrTaskPtr();
  ln=strlen(pathname);
  p=get_space_op(cdp, 1+ln+1, OP_BACKUP);
  memcpy(p, pathname, ln+1);
  return cond_send(cdp);
}

CFNC DllKernel BOOL WINAPI Continue_backup()
{ return kernel_control(GetCurrTaskPtr(), SUBOP_BACK_CONTIN, 0); }
#endif

static BOOL get_password_digest(cdp_t cdp, const char * password, unsigned count, uns8 * digest)
{ int len=(int)strlen(password);
  uns8 * mess = (uns8*)corealloc(UUID_SIZE+len, 47);
  if (mess==NULL) { client_error(cdp, OUT_OF_KERNEL_MEMORY);  return TRUE; }
  strcpy((tptr)mess, password);  
  Upcase9(cdp, (char*)mess);
  memcpy(mess+len, cdp->server_uuid, UUID_SIZE);
#ifdef WINS
  HCURSOR hOldCurs=SetCursor(LoadCursor(NULL, IDC_WAIT));
#endif
  md5_digest(mess, UUID_SIZE+len, digest);
  for (int i=1;  i<count;  i++)
    md5_digest(digest, MD5_SIZE, digest);
  corefree(mess);
#ifdef WINS
  SetCursor(hOldCurs);
#endif
  return FALSE;
}

CFNC DllKernel BOOL WINAPI cd_Login_par(cdp_t cdp, const char * usernm, const char * password, unsigned flags)
/* Parameteres are <username, password> or <*NETWORK, anything>. The 2nd
   case is converted to <*NETWORK, networkname> if networkname is known.
   Stores passwors or *NETWORK in cdp->password.
   Two direct clients or 2 network clients from the same computer are
   recognized by previous_login in server. Direct and network client from
   the same computer are not recognized in this way.
   Then, if client is logged in the Novell network and its login name is
   known in WB, it is logged directly. */
{ tptr p;
  if (!cdp) return TRUE;  // fuse
 // clear old login-related data:
  if (!(flags & LOGIN_FLAG_SIMULATED))
  { drop_all_statements(cdp);  // in simulated login the prepared statements must be preserved
#if WBVERS<=810
    if (cdp->priv_key_file!=NULL) safefree((void**)&cdp->priv_key_file); // clears secure key in memory
#endif
    remove_retained_private_key();
    if (cdp->obtained_login_key) { discard_login_key8(cdp->obtained_login_key);  cdp->obtained_login_key=0; }
  }
  if (!*usernm || !stricmp(usernm, "Anonymous")) return cd_Logout_par(cdp, flags);
  if (strlen(usernm) > OBJ_NAME_LEN)
    { SET_ERR_MSG(cdp, usernm);  client_error(cdp, OBJECT_NOT_FOUND);  return TRUE; }
 // HTTP login
  if (flags & LOGIN_FLAG_HTTP)
  { p=get_space_op(cdp, 1+1+OBJ_NAME_LEN+1+(unsigned)strlen(password)+1, OP_LOGIN);
    *p=LOGIN_PAR_HTTP;  p++;
    strcpy(p, usernm);  p+=OBJ_NAME_LEN+1;  // length of [usernm] verified above
    strcpy(p, password);
    if (cond_send(cdp)) return TRUE;
    goto success;
  }
  BOOL netlogin;  netlogin = !stricmp(usernm, "*NETWORK");
  int login_opcode_flags;  login_opcode_flags = 0;
  if (flags & LOGIN_FLAG_LIMITED)   login_opcode_flags|=LOGIN_LIMITED_FLAG;
  if (flags & LOGIN_FLAG_SIMULATED) login_opcode_flags|=LOGIN_SIMULATED_FLAG;

#ifdef WINS
 // try impersonation
  if (netlogin)
  { p=get_space_op(cdp, 1+1, OP_LOGIN);
    *p=LOGIN_PAR_GET_COMP_NAME;  if (flags & LOGIN_FLAG_LIMITED) *p|=LOGIN_LIMITED_FLAG;
    char server_comp_name[70];
    REGISTER_ANSWER(server_comp_name, 70);
    if (!cond_send(cdp) && *server_comp_name) 
    {// make server listening for connection:
      int iter = 10;
      while (iter) // iterating until server not busy with another impersonation
      { p=get_space_op(cdp, 1+1, OP_LOGIN);
        *p=LOGIN_PAR_IMPERSONATE_1 | login_opcode_flags; // connects and starts reading
        if (!cond_send(cdp)) break;
        iter--;
      }
      if (iter)  // phase _1 OK
      {// connect to the pipe:
        char pipe_name[100+OBJ_NAME_LEN];  FHANDLE hPipe;
        sprintf(pipe_name, "\\\\%s\\pipe\\WinBaseImpersonationPipe_", server_comp_name);
        size_t i=strlen(pipe_name);
        strcat(pipe_name, cdp->conn_server_name);
        while (pipe_name[i])
        { if (pipe_name[i]=='\\' || pipe_name[i]=='/' || pipe_name[i]==':' || pipe_name[i]=='"' || pipe_name[i]=='|' || pipe_name[i]=='<' || pipe_name[i]=='>')
            pipe_name[i]='_';
          i++;
        }
        hPipe = CreateFile(pipe_name, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL); 
        if (hPipe != INVALID_FHANDLE_VALUE)
        { p=get_space_op(cdp, 1+1, OP_LOGIN);
          *p=LOGIN_PAR_IMPERSONATE_2 | login_opcode_flags; // connects and starts reading
          if (!cond_send(cdp)) 
          { DWORD cbWritten;
            if (WriteFile(hPipe, "#", 1, &cbWritten, NULL))
            { p=get_space_op(cdp, 1+1, OP_LOGIN);
              *p=LOGIN_PAR_IMPERSONATE_3 | login_opcode_flags; // impersonates and disconnects
              wbbool result;
              REGISTER_ANSWER(&result, 1);
              if (!cond_send(cdp) && result) 
                { CloseHandle(hPipe);  goto success; }
            }
          }
          CloseHandle(hPipe);
        }
        else 
        { GetLastError();
          p=get_space_op(cdp, 1+1, OP_LOGIN);
          *p=LOGIN_PAR_IMPERSONATE_STOP | login_opcode_flags;
          cond_send(cdp);
        }
      }
    }
  }
#endif
#ifdef USE_KERBEROS
  if (netlogin)
  {
    char *p=get_space_op(cdp, 1+1, OP_LOGIN);
    *p=LOGIN_PAR_KERBEROS_ASK | login_opcode_flags;
    char server_kerberos_name[50];
    REGISTER_ANSWER(server_kerberos_name, 50);
    if (!cond_send(cdp) && *server_kerberos_name){
      cd_concurrent(cdp, TRUE);
      char *p=get_space_op(cdp, 1+1, OP_LOGIN);
      *p=LOGIN_PAR_KERBEROS_DO | login_opcode_flags;
      char use_kerberos[1];
      REGISTER_ANSWER(use_kerberos, 1);
      cond_send(cdp);
      int res=cdp->pRemAddr->krb5_sendauth(server_kerberos_name);
      do {
	      if (!cdp->pRemAddr->ReceiveWork(cdp, -1)) 
		      ReceiveUnprepare(cdp);
      } while (cdp->RV.holding & 2);
      cdp->RV.holding=0;  // answered and not blocked
      cd_concurrent(cdp, FALSE);
      if (res && *use_kerberos)
	      goto success;  /* povedlo se */
    }
  }
#endif

 // try to use the another login from the same process:
  if (!(flags & LOGIN_FLAG_SIMULATED) && netlogin) // using the login key not enabled for the simulated login
  { int lk_ind = -1;
    do
    { 
#if WBVERS>=900
      uns32 login_key = get_login_key8(cdp->locid_server_name, lk_ind);
#else
      uns32 login_key = get_login_key8(cdp->conn_server_name, lk_ind);
#endif
      if (!login_key) break;
      p=get_space_op(cdp, 1+1+sizeof(uns32), OP_LOGIN);
      *p=LOGIN_PAR_SAME | login_opcode_flags;
      *(uns32*)(p+1)=login_key;
      if (!cond_send(cdp)) goto success;
#ifdef TRACE_LOG
      sprintf(shrlgbuf, "%u: Failed 2 key %u for server %s", GetCurrentThreadId(), login_key, cdp->conn_server_name);
      log_it(shrlgbuf);
#endif
      discard_login_key8(login_key);  // this key does not work, remove it
    } while (true);
  }
 // phase 1: send name, receive RFC1938 count, comuter name, objnum:
  p=get_space_op(cdp, 1+1+OBJ_NAME_LEN+1, OP_LOGIN);
  if (p==NULL) return TRUE;
  *p=LOGIN_PAR_PREP | login_opcode_flags;  // phase number
  strcpy(p+1, usernm);    // length of [usernm] verified above
  t_login_info login_info;
  REGISTER_ANSWER(&login_info, sizeof(t_login_info));
  if (cond_send(cdp)) return TRUE;
 // logged by previous login from the same station:
  if (!login_info.just_logged) 
  { if (netlogin) return TRUE;
   // normal login, phase 2:
    uns8 digest[MD5_SIZE];
    if (get_password_digest(cdp, password, login_info.count, digest))
      return TRUE;
   // trying no-license login with own username:
    if (!(flags & LOGIN_FLAG_SIMULATED))
    { int lk_ind = -1;
      do
      { 
#if WBVERS>=900
        uns32 login_key = get_login_key8(cdp->locid_server_name, lk_ind);
#else
        uns32 login_key = get_login_key8(cdp->conn_server_name, lk_ind);
#endif
        if (!login_key) break;
        p=get_space_op(cdp, 1+1+sizeof(tobjnum)+MD5_SIZE+sizeof(uns32), OP_LOGIN);
        if (p==NULL) return TRUE;
        *p=LOGIN_PAR_SAME_DIFF | login_opcode_flags;  // phase number
        p++;
        *(tobjnum*)p=login_info.userobj;
        memcpy(p+sizeof(tobjnum), digest, MD5_SIZE);  p+=sizeof(tobjnum)+MD5_SIZE;
        *(uns32*)p=login_key;
        if (!cond_send(cdp)) goto success;
#ifdef TRACE_LOG
        sprintf(shrlgbuf, "%u: Failed 2 key %u for server %s", GetCurrentThreadId(), login_key, cdp->conn_server_name);
        log_it(shrlgbuf);
#endif
        // discard_login_key8(login_key);  -- do not remove the key, it may be OK but the password was wrong
      } while (true);
    }
   // normal login:
    p=get_space_op(cdp, 1+1+sizeof(tobjnum)+MD5_SIZE, OP_LOGIN);
    if (p==NULL) return TRUE;
    *p=LOGIN_PAR_PASS | login_opcode_flags;  // phase number
    p++;
    *(tobjnum*)p=login_info.userobj;
    memcpy(p+sizeof(tobjnum), digest, MD5_SIZE);
    if (cond_send(cdp)) return TRUE;
  }

 success:
  if (!(flags & LOGIN_FLAG_SIMULATED))
  { p=get_space_op(cdp, 1+1, OP_CNTRL);
    *p=SUBOP_GET_LOGIN_KEY;
    REGISTER_ANSWER(&cdp->obtained_login_key, sizeof(uns32));
    REGISTER_ANSWER(&cdp->logged_as_user, sizeof(tobjnum));
    cond_send(cdp);
#if WBVERS>=900
    store_login_key8(cdp->locid_server_name, cdp->obtained_login_key);
#else
    store_login_key8(cdp->conn_server_name, cdp->obtained_login_key);
#endif
  }
  return FALSE;
}

CFNC DllKernel BOOL WINAPI cd_Login(cdp_t cdp, const char * usernm, const char * password)
{ return cd_Login_par(cdp, usernm, password, 0); }

CFNC DllKernel BOOL WINAPI cd_Logout_par(cdp_t cdp, unsigned flags)
{ if (!cdp) return TRUE;  // fuse
 // clear old login-related data:
  if (!(flags & LOGIN_FLAG_SIMULATED))
  {
#ifdef CLIENT_GUI
#if WBVERS<900
    RemDBContainer();
#endif
#endif
#ifdef CLIENT_PGM
    Select_language(cdp, -1);
#endif
    inval_table_d(cdp, NOOBJECT, CATEG_TABLE);   // bylo podmineno CLIENT_GUI, ale asi by melo byt vzdy
    inval_table_d(cdp, NOOBJECT, CATEG_CURSOR);
    drop_all_statements(cdp);    // VisualBasic create multiple connections, must preserver prepared statements
#if WBVERS<=810
    if (cdp->priv_key_file!=NULL) safefree((void**)&cdp->priv_key_file); // clears secure key in memory
#endif
    remove_retained_private_key();
    if (cdp->obtained_login_key) { discard_login_key8(cdp->obtained_login_key);  cdp->obtained_login_key=0; }
  }

  int login_opcode_flags = 0;
  if (flags & LOGIN_FLAG_LIMITED)   login_opcode_flags|=LOGIN_LIMITED_FLAG;
  if (flags & LOGIN_FLAG_SIMULATED) login_opcode_flags|=LOGIN_SIMULATED_FLAG;
  tptr p=get_space_op(cdp, 1+1, OP_LOGIN);
  if (p==NULL) return TRUE;
  *p=LOGIN_PAR_LOGOUT | login_opcode_flags; // phase
  if (cond_send(cdp)) return TRUE;
  cdp->logged_as_user=ANONYMOUS_USER;  // must not change logged_as_user when anonymous anount is disabled
  return FALSE;
}

CFNC DllKernel BOOL WINAPI cd_Logout(cdp_t cdp)
{ return cd_Logout_par(cdp, 0); }

CFNC DllKernel void WINAPI cd_Invalidate_cached_table_info(cdp_t cdp)
// To be called after modifying structure of a table by ALTER TABLE
{ 
  inval_table_d(cdp, NOOBJECT, CATEG_TABLE);  // this is most important
  inval_table_d(cdp, NOOBJECT, CATEG_DIRCUR); 
  inval_table_d(cdp, NOOBJECT, CATEG_CURSOR); 
}

CFNC DllKernel BOOL WINAPI erase_replica(cdp_t cdp)
{ if (!cdp) return TRUE;  // fuse
#ifdef CLIENT_GUI
  RemDBContainer();
#endif
#ifdef CLIENT_PGM
  Select_language(cdp, -1);
#endif
  drop_all_statements(cdp);    // VisualBasic create multiple connections, must preserver prepared statements
#if WBVERS<=810
  if (cdp->priv_key_file!=NULL) safefree((void**)&cdp->priv_key_file); // clears secure key in memory
#endif
  remove_retained_private_key();
  tptr p=get_space_op(cdp, 1+1, OP_LOGIN);
  if (p==NULL) return TRUE;
  *p=LOGIN_PAR_WWW;
  return cond_send(cdp);
}

static BOOL cd_send_interf_close(cdp_t cdp)
{ 
#ifdef CLIENT_PGM
  Select_language(cdp, -1);
#endif
  tptr p=get_space_op(cdp, 1, OP_ACCCLOSE);
  if (p==NULL) return TRUE;
  return cond_send(cdp);
}

static tobjname user_name;   /* used by Who_am_I only */

CFNC DllKernel const char * WINAPI cd_Who_am_I(cdp_t cdp)
// Returns non-empty name even for ANONYMOUS
{ tptr p=get_space_op(cdp, 1+1, OP_CNTRL);
  if (!p) return NULLSTRING;
  *p=SUBOP_WHO_AM_I;
  cdp->ans_array[cdp->ans_ind]=user_name;
  cdp->ans_type [cdp->ans_ind]=sizeof(tobjname);
  cdp->ans_ind++;
  if (cond_send(cdp)) *user_name=0;
  return user_name;
}

CFNC DllKernel BOOL WINAPI cd_Start_transaction(cdp_t cdp)
{ tptr p=get_space_op(cdp, 1,OP_START_TRA);
  if (p==NULL) return TRUE;
  return cond_send(cdp);
}

CFNC DllKernel BOOL WINAPI cd_Commit(cdp_t cdp)
{ tptr p=get_space_op(cdp, 1,OP_COMMIT);
  if (p==NULL) return TRUE;
  return cond_send(cdp);
}

CFNC DllKernel BOOL WINAPI cd_Roll_back(cdp_t cdp)
{ tptr p=get_space_op(cdp, 1,OP_ROLL_BACK);
  if (p==NULL) return TRUE;
  return cond_send(cdp);
}

#define BACK_END_CATEGORY(cat) ((cat)==CATEG_TABLE || (cat)==CATEG_CURSOR || (cat)==CATEG_TRIGGER || (cat)==CATEG_PROC || \
                                (cat)==CATEG_SEQ || (cat)==CATEG_REPLREL  || (cat)==CATEG_DOMAIN)

CFNC DllKernel BOOL WINAPI cd_List_objects(cdp_t cdp, tcateg category, const uns8 * appl_id, tptr * buffer)
{ tptr p=get_space_op(cdp, 1+sizeof(tcateg)+UUID_SIZE, OP_LIST);
  if (p==NULL) return TRUE;
  *(tcateg *)p=category;  p+=sizeof(tcateg);
  if (appl_id==NULL)
  { tcateg cat = category & CATEG_MASK;
    appl_id = cat==CATEG_ROLE ? cdp->sel_appl_uuid : BACK_END_CATEGORY(cat) ?
      cdp->back_end_uuid : cdp->front_end_uuid;
  }
  memcpy(p, appl_id, UUID_SIZE);
  cdp->ans_array[cdp->ans_ind]=buffer;
  cdp->ans_type [cdp->ans_ind]=ANS_SIZE_DYNALLOC;
  cdp->ans_ind++;
  return cond_send(cdp);
}

CFNC DllKernel BOOL WINAPI cd_Find2_object(cdp_t cdp, const char * name, const uns8 * appl_id, tcateg category, tobjnum * position)
{ tptr p;
  if (appl_id==NULL)  /* via Find_object */
  { tcateg cat = category & CATEG_MASK;
    appl_id = cat==CATEG_ROLE ? cdp->sel_appl_uuid : BACK_END_CATEGORY(cat) ?
      cdp->back_end_uuid : cdp->front_end_uuid;
  }
  if (*name!=1 && strlen(name) > OBJ_NAME_LEN)
    { client_error(cdp, ERROR_IN_FUNCTION_CALL);  return TRUE; }
  p=get_space_op(cdp, 1+sizeof(tcateg)+OBJ_NAME_LEN+UUID_SIZE, OP_FINDOBJ);
  if (p==NULL) return TRUE;
  *(tcateg *)p=category;  p+=sizeof(tcateg);
  memset(p, 0, OBJ_NAME_LEN);
  if (*name==1) memcpy(p, name, UUID_SIZE+1);
  else strmaxcpy(p, name, OBJ_NAME_LEN+1);
  memcpy(p+OBJ_NAME_LEN, appl_id, UUID_SIZE);
  cdp->ans_array[cdp->ans_ind]=position;
  cdp->ans_type [cdp->ans_ind]=sizeof(tobjnum);
  cdp->ans_ind++;
  *position=NOOBJECT;
  return cond_send(cdp);
}

int scan_name(const char * & src, char * name, int maxsize)
{ int len = 0;
  while (*src==' ') src++;
  if (*src=='`')
  { *src++;
    while (*src && *src!='`')
    { if (maxsize>1) 
        { *name=*src;  name++;  len++; maxsize--; }
      src++;  
    }
    if (*src) src++;  // skipping '`'
    while (*src==' ') src++;
  }
  else  // not delimited - accept all except dot
  { while (*src && *src!='.')
    { if (maxsize>1) 
        { *name=*src;  name++;  len++; maxsize--; }
      src++;  
    }
  }
  *name=0;
  return len;    
}

CFNC DllKernel BOOL WINAPI cd_Find_object(cdp_t cdp, const char * name, tcateg category, tobjnum * position)
// Accepts multipart names and names in ``. 
{ char name1[256];  tobjname name2, name3;  int len1, len2, len3;
 /* analyse name: */
  len1=scan_name(name, name1, sizeof(name1));
  if (*name=='.')
  { name++;
    len2=scan_name(name, name2, sizeof(name2));
    if (*name=='.')
    { name++;
      len3=scan_name(name, name3, sizeof(name3));
    }
    else len3=0;
  }
  else len2=len3=0;

 /* search among loaded objects: */
  if (caching_apl_objects)
    if (!len2&& !len3 && (!cdp->odbc.mapping_on || (category & CATEG_MASK)!=CATEG_TABLE))
    { t_dynar * d_comp=get_object_cache(cdp, category);
      if (d_comp!=NULL)
      { tobjname objname;  strmaxcpy(objname, name1, sizeof(tobjname));    unsigned pos;
        Upcase9(cdp, objname);
        if (comp_dynar_search(d_comp, objname, &pos)) /* found */
        { t_comp * acomp = (t_comp*)d_comp->acc(pos);
          if (!(category & IS_LINK) ||          // unless link source searched
              !(acomp->flags & CO_FLAG_LINK) && // or this is not a link
              (category != (CATEG_TABLE | IS_LINK) || !(acomp->flags & CO_FLAG_ODBC)))
                                                // nor ODBC link
          /* cache contains linked object number only, must continue search for link object */
          { *position=acomp->objnum;
            return FALSE;
          }
        }
      }
    }
#ifdef CLIENT_ODBC
 /* search among ODBC-linked tables: */
 /* must ignore case: when compiling view its base is in upcase */
 // not for category & IS_LINK!
  if (category == CATEG_TABLE && cdp->odbc.hEnv && cdp->odbc.conns!=NULL)
  { t_connection * conn = cdp->odbc.conns;  int i;
    do
    { for (i=0;  i<conn->ltabs.count();  i++)
        if (conn->ltabs.acc(i)->is_table)
        { tptr lqualif, lowner, ltable;  int llenqualif, llenowner, llentable;
          ltable = conn->ltabs.acc(i)->s.odbc_link->tabname;
          llentable =strlen(ltable);
          lowner = conn->ltabs.acc(i)->s.odbc_link->owner;
          llenowner =strlen(lowner);
          lqualif= conn->ltabs.acc(i)->s.odbc_link->qualifier;
          llenqualif=strlen(lqualif);
          if (len3)   /* 3-part comparison: */
          { if (llentable==len3 && !strnicmp(ltable, name3, len3) &&
                llenowner ==len2 && !strnicmp(lowner, name2, len2) &&
                llenqualif==len1 && !strnicmp(lqualif, name1, len1))
              { *position=conn->ltabs.acc(i)->odbc_num;  return FALSE; }
           /* dot inside prefix: */
            if (llentable==len3 && !strnicmp(ltable, name3, len3) &&
                !llenowner && llenqualif==len1+1+len2 &&
                !strnicmp(lqualif, name1, len1) && lqualif[len1]=='.' &&
                !strnicmp(lqualif+len1+1, name2, len2))
              { *position=conn->ltabs.acc(i)->odbc_num;  return FALSE; }

          }
          else if (len2!=NULL)  /* 2-part comparision: */
          { if (llentable==len2 && !strnicmp(ltable, name2, len2) &&
                (llenowner ==len1 && !strnicmp(lowner,  name1, len1) ||
                 llenqualif==len1 && !strnicmp(lqualif, name1, len1)))
              { *position=conn->ltabs.acc(i)->odbc_num;  return FALSE; }
          }
          else  /* simple name comparison: */
            if (llentable==len1 && !strnicmp(ltable, name1, len1))
              { *position=conn->ltabs.acc(i)->odbc_num;  return FALSE; }
        }
      conn=conn->next_conn;
    }
    while (conn!=NULL);
  }
  if (cdp->odbc.mapping_on && (category & CATEG_MASK)==CATEG_TABLE)
    { SET_ERR_MSG(cdp, len3 ? name3 : name1);  client_error(cdp, OBJECT_NOT_FOUND);  return TRUE; }
#endif
 /* search in WB: */
  if (len3!=0)
    { SET_ERR_MSG(cdp, name3);  client_error(cdp, OBJECT_NOT_FOUND);  return TRUE; }
  if (len2!=0)
  { if (len2>OBJ_NAME_LEN || len1>OBJ_NAME_LEN)
      { client_error(cdp, ERROR_IN_FUNCTION_CALL);  return TRUE; }
    return cd_Find_prefixed_object(cdp, name1, name2, category, position);
  }
  // dot may be contained in the name
  return cd_Find2_object(cdp, name1, NULL, category, position);
}

CFNC DllKernel BOOL WINAPI cd_Find_prefixed_object(cdp_t cdp, const char * schema, const char * objname, tcateg category, tobjnum * position)
{ WBUUID appl_id;  uns8 * p_uuid;
  if (!schema || !*schema) p_uuid=NULL;
  else 
  { if (cd_Apl_name2id(cdp, schema, appl_id)) return TRUE;
    p_uuid=appl_id;
  }
  return cd_Find2_object(cdp, objname, p_uuid, category, position);
}

CFNC DllKernel BOOL WINAPI cd_Find_object_by_id(cdp_t cdp, const WBUUID uuid, tcateg category, tobjnum * position)
{ char buf[OBJ_NAME_LEN+1];  // supposed OBJ_NAME_LEN > UUID_SIZE
  buf[0]=1;  memcpy(buf+1, uuid, UUID_SIZE);
  return cd_Find2_object(cdp, buf, NULL, category, position);
}

CFNC DllKernel BOOL WINAPI cd_Rec_cnt(cdp_t cdp, tcursnum cursor, trecnum * position)
{ tptr p;
  *position=NORECNUM;   /* may be used by bad programs after error */
#ifdef CLIENT_ODBC  
  if (IS_ODBC_TABLE(cursor))
  { trecnum r, r_ex, r_no;
    ltable * dt = get_ltable(cdp, cursor);
    if (dt==NULL)
      { client_error(cdp, ODBC_CURSOR_NOT_OPEN);  return TRUE; }
    r=64;
    if (odbc_rec_status(cdp, dt, r)==SQL_ROW_SUCCESS)
    { r_ex=r;
      do  /* looking for nonexisting record: */
      { r=2*r_ex;
        if (odbc_rec_status(cdp, dt, r)==SQL_ROW_SUCCESS)
          r_ex=r;
        else { r_no=r;  break; }
      } while (TRUE);
    }
    else
    { r_no=r;  r_ex=NORECNUM;
      do  /* looking for existing record: */
      { r=r_no/2;
        if (odbc_rec_status(cdp, dt, r)!=SQL_ROW_SUCCESS)
          r_no=r;
        else { r_ex=r;  break; }
      } while (r_no);
    }
    while (r_ex+1 < r_no)   /* TRUE even if r_no==0 */
    { r=(r_ex+r_no)/2;
      if (odbc_rec_status(cdp, dt, r)!=SQL_ROW_SUCCESS)
        r_no=r;
      else r_ex=r;
    }
    *position=r_no;
    return FALSE;
  }
#endif
  p=get_space_op(cdp, 1+sizeof(tcursnum), OP_RECCNT);
  if (p==NULL) return TRUE;
  *(tcursnum*)p=cursor;
  cdp->ans_array[cdp->ans_ind]=position;
  cdp->ans_type [cdp->ans_ind]=sizeof(trecnum);
  cdp->ans_ind++;
  return cond_send(cdp);
}

inline void write_attrib(char * & dest, const void * src, unsigned size)
{ if (size==sizeof(uns16))
  {
#if WBVERS<950
    *(uns16*)dest=*(uns8 *)src;  
#else
    *(uns16*)dest=*(uns16*)src;  
#endif    
    dest+=sizeof(uns16); 
  }
  else
    { *(uns8 *)dest=*(uns8 *)src;  dest+=sizeof(uns8 ); }
}

CFNC DllKernel BOOL WINAPI cd_Db_copy(cdp_t cdp, ttablenum t1, trecnum r1, tattrib a1, uns16 ind1,
             ttablenum t2, trecnum r2, tattrib a2, uns16 ind2)
{ unsigned sizeof_tattrib = cdp->sizeof_tattrib();
  tptr p=get_space_op(cdp, 1+2*(sizeof(ttablenum)+sizeof(trecnum)+sizeof_tattrib+sizeof(uns16)),
                 OP_TRANSF);
  if (p==NULL) return TRUE;
  *(ttablenum*)p=t1;    p+=sizeof(ttablenum);      
  *(trecnum  *)p=r1;    p+=sizeof(trecnum);
  write_attrib(p, &a1, sizeof_tattrib);
  *(uns16    *)p=ind1;  p+=sizeof(uns16);
  *(ttablenum*)p=t2;    p+=sizeof(ttablenum);
  *(trecnum  *)p=r2;    p+=sizeof(trecnum);
  write_attrib(p, &a2, sizeof_tattrib);
  *(uns16    *)p=ind2;
  return cond_send(cdp);
}

CFNC DllKernel BOOL WINAPI cd_Get_info(cdp_t cdp, kernel_info * kinf)
// Obsolete function, implemented only for compatibility
{ uns32 vers1, vers2, vers3;
  if (cd_Get_server_info(cdp, OP_GI_VERSION_1,     &vers1,              sizeof(vers1              ))) return TRUE;
  if (cd_Get_server_info(cdp, OP_GI_VERSION_2,     &vers2,              sizeof(vers2              ))) return TRUE;
  if (cd_Get_server_info(cdp, OP_GI_VERSION_3,     &vers3,              sizeof(vers3              ))) return TRUE;
  sprintf(kinf->version, "%u.%u%c", vers1, vers2, vers3==0 ? '*' : vers3==1 ? ' ' : 'a'+vers3-2);
  if (cd_Get_server_info(cdp, OP_GI_LICS_USING,    &kinf->logged,       sizeof(kinf->logged       ))) return TRUE;
  if (cd_Get_server_info(cdp, OP_GI_CLUSTER_SIZE,  &kinf->blocksize,    sizeof(kinf->blocksize    ))) return TRUE;
  if (cd_Get_server_info(cdp, OP_GI_FREE_CLUSTERS, &kinf->freeblocks,   sizeof(kinf->freeblocks   ))) return TRUE;
  if (cd_Get_server_info(cdp, OP_GI_FRAMES,        &kinf->frames,       sizeof(kinf->frames       ))) return TRUE;
  if (cd_Get_server_info(cdp, OP_GI_FIXED_PAGES,   &kinf->fixed_pages,  sizeof(kinf->fixed_pages  ))) return TRUE;
  if (cd_Get_server_info(cdp, OP_GI_LICS_CLIENT,   &kinf->max_users,    sizeof(kinf->max_users    ))) return TRUE;
  if (cd_Get_server_info(cdp, OP_GI_DISK_SPACE,    &kinf->diskspace,    sizeof(kinf->diskspace    ))) return TRUE;
  if (cd_Get_server_info(cdp, OP_GI_SERVER_NAME,   &kinf->server_name,  sizeof(kinf->server_name  ))) return TRUE;
  if (cd_Get_server_info(cdp, OP_GI_OWNED_CURSORS, &kinf->owned_cursors,sizeof(kinf->owned_cursors))) return TRUE;
  kinf->local_free_memory =free_sum();
  kinf->remote_free_memory=(uns32)-1;
  kinf->networking        =cdp->in_use==PT_REMOTE;
  return FALSE;
}

CFNC DllKernel BOOL WINAPI Get_info(kernel_info * kinf)
{ return cd_Get_info(GetCurrTaskPtr(), kinf); }

CFNC DllKernel sig32 WINAPI cd_Available_memory(cdp_t cdp, BOOL local)
{ return local ? (sig32)free_sum() : -1; }

CFNC DllKernel sig32 WINAPI Available_memory(BOOL local)
{ return cd_Available_memory(GetCurrTaskPtr(), local); }

CFNC DllKernel sig32 WINAPI cd_Used_memory(cdp_t cdp, BOOL local)
{ if (local) return total_used_memory();
  sig32 val;
  if (cd_Get_server_info(cdp, OP_GI_USED_MEMORY, &val, sizeof(val))) return -1;
  return val;
}

CFNC DllKernel sig32 WINAPI cd_Client_number(cdp_t cdp)
{ sig32 val;
  if (cd_Get_server_info(cdp, OP_GI_CLIENT_NUMBER, &val, sizeof(val))) -1;
  return val;
}

CFNC DllKernel int WINAPI cd_Owned_cursors(cdp_t cdp)
{ int num;
  return cd_Get_server_info(cdp, OP_GI_OWNED_CURSORS, &num, sizeof(num)) ? 0 : num;
}

static tptr get_space_cat(cdp_t cdp, uns8 op, tcurstab cursor,
        trecnum recnum, tattrib attr, const modifrec * access, unsigned size)
{ tptr p;  int modsize=0;  const modifrec * acc;
  unsigned sizeof_tattrib = cdp->sizeof_tattrib();
  if (access)
  { acc=access;
    while (TRUE)
    { if (acc->modtype==MODIND)           modsize+=3;
           else if (acc->modtype==MODINT)    { modsize+=9;  break; }
           else if (acc->modtype==MODPTR)      modsize+=1+sizeof_tattrib;
           else if (acc->modtype==MODINDPTR)   modsize+=3+sizeof_tattrib;
           else if (acc->modtype==MODLEN)    { modsize+=1;  break; }
           else break;  /* MODSTOP or error */
           acc++;
    }
  }
  p=get_space_op(cdp, 1+sizeof(tcurstab)+sizeof(trecnum)+sizeof_tattrib+1+size+modsize, op);
  if (!p) return NULL;
  *(tcurstab*)p=cursor;  p+=sizeof(tcurstab);
  *(trecnum *)p=recnum;  p+=sizeof(trecnum);
  write_attrib(p, &attr, sizeof_tattrib);
  if (access)
  { while (access->modtype!=MODSTOP)
    { switch (access->modtype)
      { case MODIND:
          *p=MODIND;                                 p++;
          *(uns16*)p=access->moddef.modind.index;    p+=2; break;
        case MODINT:
          *p=MODINT;                                 p++;
          *(uns32*)p=access->moddef.modint.start;    p+=4;
          *(uns32*)p=access->moddef.modint.size;     p+=4; goto len2stop;
        case MODPTR:
          *p=MODPTR;                                 p++;
          write_attrib(p, &access->moddef.modptr.attr, sizeof_tattrib);
          break;
        case MODINDPTR:
          *p=MODINDPTR;                              p++;
          *(uns16*)p=access->moddef.modindptr.index; p+=2;
          write_attrib(p, &access->moddef.modindptr.attr, sizeof_tattrib);
          break;
        case MODLEN:
          *p=MODLEN;                                 p++;  goto len2stop;
        default:                                           goto len2stop;
      }
      access++;
    }
  len2stop:;
  }
  *p=MODSTOP;
  return p+1;
}

#ifdef CLIENT_ODBC
static BOOL odbc_rw(cdp_t cdp, tcursnum cursor, trecnum  position,
  tattrib attr, const modifrec * access, void * buffer, BOOL is_write, t_varcol_size * bnrd)
{ t_varcol_size size, start;  BOOL is_len;
  int i;  atr_info * catr;
 /* analyse modifiers: */
  start=(t_varcol_size )-1;  is_len=FALSE;
  if (access!=NULL)
  { if (access->modtype==MODIND)
      { client_error(cdp, BAD_MODIF);  return TRUE; }  /* multiattrs not supported */
    if (access->modtype==MODINT)
    { start=access->moddef.modint.start;
      size =access->moddef.modint.size;
    }
    else if (access->modtype==MODLEN) is_len=TRUE;
    else if (access->modtype!=MODSTOP)
      { client_error(cdp, BAD_MODIF);  return TRUE; }  /* bad modifier */
  }
 /* position on the destination record: */
  ltable * ltab=get_ltable(cdp, cursor);
  if (ltab==NULL)
    { client_error(cdp, ODBC_CURSOR_NOT_OPEN);  return TRUE; }
#ifdef OLDSETPOS
  if (!ltab->conn->SetPos_support)    /* not supported */
    { client_error(cdp, DRIVER_NOT_CAPABLE);  return TRUE; }
#endif
  if (position!=ltab->cache_top)
  { if (ltab->cache_top!=NORECNUM) cache_free(ltab, 0, 1);
    if (odbc_load_record(cdp, ltab, position, (tptr)ltab->cache_ptr))
    { ltab->cache_top=NORECNUM;
      client_error(cdp, OUT_OF_TABLE);
      return TRUE;
    }  /* record does not exist or cannot load */
    ltab->cache_top=position;
    for (i=1, catr=ltab->attrs+1;  i<(int)ltab->attrcnt;  i++, catr++)
      catr->changed=FALSE;
  }
 /* find the attribute: */
  if (attr >= ltab->attrcnt)
    { client_error(cdp, BAD_ELEM_NUM);  return TRUE; } /* bad attribute number */
  catr=ltab->attrs+attr;
  tptr p=(tptr)ltab->cache_ptr+catr->offset;

  if (catr->flags & CA_INDIRECT)
  { indir_obj * iobj = (indir_obj*)p;
    if (is_len)
      if (is_write)
      { if (iobj->actsize > *(t_varcol_size *)buffer)
          iobj->actsize = *(t_varcol_size *)buffer;
      }
      else /* read */ *(t_varcol_size*)buffer=iobj->actsize;
    else if (start!=(t_varcol_size )-1)   /* var-len operation */
      if (is_write)
      {/* object update size: */
        if (!iobj->obj || start+size > iobj->obj->maxsize)
        { HGLOBAL hnd = GlobalAlloc(GMEM_MOVEABLE, HPCNTRSZ+start+size);
          if (!hnd) { client_error(cdp, OUT_OF_KERNEL_MEMORY);  return TRUE; }
          cached_longobj * lobj = (cached_longobj*)GlobalLock(hnd);
          lobj->maxsize=start+size;
          if (iobj->obj != NULL)
          { memcpy(lobj->data, iobj->obj->data, iobj->actsize);
            GlobalUnlock(GlobalPtrHandle(iobj->obj));
            GlobalFree  (GlobalPtrHandle(iobj->obj));
          }
          iobj->obj=lobj;
          iobj->actsize=start+size;
        }
       /* write: */
        memcpy(iobj->obj->data+start, buffer, size);
      }
      else /* read */
      { if (start>=iobj->actsize)
          { if (bnrd!=NULL) *bnrd=0; }
        else
        { if (start+size > iobj->actsize) size=iobj->actsize-start;
          if (bnrd!=NULL) *bnrd=size;
          memcpy(buffer, iobj->obj->data+start, size);
        }
      }
    else { client_error(cdp, BAD_MODIF);  return TRUE; }
  }
  else  /* fixed-len data */
    if (is_len) { client_error(cdp, BAD_MODIF);  return TRUE; }
    else if (is_string(catr->type))
      if (is_write) strmaxcpy(p, (tptr)buffer, catr->size);
      else /* read */ strcpy((tptr)buffer, p);
    else switch (catr->type)
    { case ATT_ODBC_DATE:
        if (is_write) date2DATE(*(uns32*)buffer, (DATE_STRUCT*)p);
        else DATE2date((DATE_STRUCT*)p, (uns32*)buffer);
        break;
      case ATT_ODBC_TIME:
        if (is_write) time2TIME(*(uns32*)buffer, (TIME_STRUCT*)p);
        else TIME2time((TIME_STRUCT*)p, (uns32*)buffer);
        break;
      case ATT_ODBC_TIMESTAMP:
        if (is_write) datim2TIMESTAMP(*(uns32*)buffer, (TIMESTAMP_STRUCT*)p);
        else TIMESTAMP2datim((TIMESTAMP_STRUCT*)p, (uns32*)buffer);
        break;
      case ATT_ODBC_NUMERIC:
      case ATT_ODBC_DECIMAL:
        if (is_write) money2str((monstr*)buffer, p, 1);
        else str2money(p, (monstr*)buffer);
        break;
      default:
        if (is_write) memcpy(p, buffer, catr->size);
        else /* read */ memcpy(buffer, p, catr->size);  break;
    }

  if (is_write)
  { catr->changed=TRUE;
    BOOL res=setpos_synchronize_view(cdp, TRUE, ltab, (tptr)ltab->cache_ptr, position, FALSE);
    catr->changed=FALSE;
    return res;
  }
  else return FALSE;
}
#endif  // CLIENT_ODBC

CFNC DllKernel BOOL WINAPI cd_Read(cdp_t cdp, tcursnum cursor, trecnum position, tattrib attr,
                      const modifrec * access, void * buffer)
{ 
#ifdef CLIENT_ODBC
  if (IS_ODBC_TABLE(cursor))
    return access && access->modtype==MODINT ?
      odbc_rw(cdp, cursor, position, attr, access, (tptr)buffer+sizeof(t_varcol_size), FALSE, (t_varcol_size *)buffer) :
      odbc_rw(cdp, cursor, position, attr, access, buffer, FALSE, NULL);
  if (cursor==OBJ_TABLENUM && cdp->IStorageCache && attr!=DEL_ATTR_NUM && attr!=SYSTAB_PRIVILS_ATR && !cdp->in_package)
    return !read_pcached(cdp, position, attr, buffer, 0, 0, NULL, RW_OPER_FIXED);
#endif
  tptr p=get_space_cat(cdp, OP_READ, cursor, position, attr, access, 0);
  if (p==NULL) return TRUE;
  cdp->ans_array[cdp->ans_ind]=buffer;
  cdp->ans_type [cdp->ans_ind]=ANS_SIZE_VARIABLE;
  cdp->ans_ind++;
  return cond_send(cdp);
}

CFNC DllKernel BOOL WINAPI PtrSteps(tcursnum cursor, trecnum   position, tattrib attr,
                            t_mult_size index, void * buffer)
{ tptr p;  cdp_t cdp = GetCurrTaskPtr();
  unsigned sizeof_tattrib = cdp->sizeof_tattrib();
  p=get_space_op(cdp, 1+sizeof(tcursnum)+sizeof(trecnum)+sizeof_tattrib+sizeof(t_mult_size), OP_PTRSTEPS);
  if (p==NULL) return TRUE;
  *(tcursnum   *)p=cursor;    p+=sizeof(tcursnum);
  *(trecnum    *)p=position;  p+=sizeof(trecnum);
  write_attrib(p, &attr, sizeof_tattrib);
  *(t_mult_size*)p=index;
  cdp->ans_array[cdp->ans_ind]=buffer;
  cdp->ans_type [cdp->ans_ind]=sizeof(ttablenum)+sizeof(trecnum);
  cdp->ans_ind++;
  return cond_send(cdp);
}

CFNC DllKernel BOOL WINAPI cd_Write(cdp_t cdp, tcursnum cursor, trecnum   position, tattrib attr,
                      const modifrec * access, const void * buffer, uns32 datasize)
{ 
#ifdef CLIENT_ODBC
  if (IS_ODBC_TABLE(cursor))
    return odbc_rw(cdp, cursor, position, attr, access, (void*)buffer, TRUE, NULL);
#endif
  tptr p=get_space_cat(cdp, OP_WRITE, cursor, position, attr, access, datasize+1);
  if (p==NULL) return TRUE;
  memcpy(p,buffer,datasize);
  p[datasize]=(char)DATA_END_MARK;
  BOOL res=cond_send(cdp);
#ifdef CLIENT_GUI
  if (cursor==OBJ_TABLENUM && cdp->IStorageCache && attr!=DEL_ATTR_NUM && attr!=SYSTAB_PRIVILS_ATR && !res)
    write_pcache(cdp, position, attr, buffer, 0, 0, RW_OPER_FIXED);
#endif
  return res;
}

#ifdef CLIENT_ODBC
trecnum odbc_insert(cdp_t cdp, tcursnum curs)
{ trecnum reccnt;
  ltable * dt = get_ltable(cdp, curs);
  if (dt==NULL)
    { client_error(cdp, ODBC_CURSOR_NOT_OPEN);  return TRUE; }
  if (!dt->conn->SetPos_support)   /* not supported */
    { client_error(cdp, DRIVER_NOT_CAPABLE);  return TRUE; }
  if (cd_Rec_cnt(cdp, curs, &reccnt)) return TRUE;
  if (dt->cache_top!=NORECNUM)
    { cache_free(dt, 0, 1);  dt->cache_top=NORECNUM; }
  dt->cache_ptr[0]=REC_NONEX;
  if (setpos_synchronize_view(cdp, FALSE, dt, (tptr)dt->cache_ptr, NORECNUM, FALSE)) return NORECNUM;
  return odbc_rec_status(cdp, dt, reccnt)==SQL_ROW_SUCCESS ? reccnt : NORECNUM;
}
#endif

CFNC DllKernel trecnum WINAPI cd_Append(cdp_t cdp, tcursnum curs)
{ tptr p;  trecnum rec;
#ifdef CLIENT_ODBC
  if (IS_ODBC_TABLE(curs)) return odbc_insert(cdp, curs);
#endif
  p=get_space_op(cdp, 1+sizeof(tcursnum), OP_APPEND);
  if (p==NULL) return TRUE;
  *(tcursnum*)p=curs;
  cdp->ans_array[cdp->ans_ind]=&rec;
  cdp->ans_type [cdp->ans_ind]=sizeof(trecnum);
  cdp->ans_ind++;
  cd_send_package(cdp);
  return IS_AN_ERROR(cdp->RV.report_number) ? NORECNUM : rec;
}

CFNC DllKernel trecnum WINAPI cd_Insert(cdp_t cdp, tcursnum curs)
{ tptr p;  trecnum rec;
#ifdef CLIENT_ODBC
  if (IS_ODBC_TABLE(curs)) return odbc_insert(cdp, curs);
#endif
  p=get_space_op(cdp, 1+sizeof(tcursnum), OP_INSERT);
  if (p==NULL) return TRUE;
  *(tcursnum*)p=curs;
  cdp->ans_array[cdp->ans_ind]=&rec;
  cdp->ans_type [cdp->ans_ind]=sizeof(trecnum);
  cdp->ans_ind++;
  cd_send_package(cdp);
  return IS_AN_ERROR(cdp->RV.report_number) ? NORECNUM : rec;
}

static BOOL recoper(cdp_t cdp, tcursnum cursor, trecnum position, uns8 opcode)
{ tptr p;
#ifdef CLIENT_ODBC
  if (IS_ODBC_TABLE(cursor))
  { int i;  atr_info * catr;  RETCODE retcode;
    if (position==NORECNUM)  /* full-table locking */
      { client_error(cdp, DRIVER_NOT_CAPABLE);  return TRUE; }
    ltable * ltab=get_ltable(cdp, cursor);
    if (ltab==NULL)
      { client_error(cdp, ODBC_CURSOR_NOT_OPEN);  return TRUE; }
    if (!ltab->conn->can_lock)    /* not supported */
      { client_error(cdp, DRIVER_NOT_CAPABLE);  return TRUE; }
    if (position!=ltab->cache_top)
    { if (ltab->cache_top!=NORECNUM) cache_free(ltab, 0, 1);
      ltab->cache_top=NORECNUM;
      if (odbc_load_record(cdp, ltab, position, (tptr)ltab->cache_ptr))
        { client_error(cdp, OUT_OF_TABLE);  return TRUE; }  /* record does not exist or cannot load */
      ltab->cache_top=position;
      for (i=1, catr=ltab->attrs+1;  i<(int)ltab->attrcnt;  i++, catr++)
        catr->changed=FALSE;
    }
    switch (opcode)
    { case OP_WLOCK:    case OP_RLOCK:
        retcode=SQLSetPos(ltab->hStmt, 1, SQL_POSITION, SQL_LOCK_EXCLUSIVE);
        if (retcode!=SQL_SUCCESS && retcode!=SQL_SUCCESS_WITH_INFO)
          { client_error(cdp, NOT_LOCKED);  return TRUE; }
        break;
      case OP_UNWLOCK:  case OP_UNRLOCK:
        retcode=SQLSetPos(ltab->hStmt, 1, SQL_POSITION, SQL_LOCK_UNLOCK);
        if (retcode!=SQL_SUCCESS && retcode!=SQL_SUCCESS_WITH_INFO)
          { client_error(cdp, NOT_LOCKED);  return TRUE; }
        break;
      default:  /* Undelete */
        client_error(cdp, DRIVER_NOT_CAPABLE);  return TRUE;
    }
    return FALSE;
  }
  else
#endif
  { if (cursor==TAB_TABLENUM && IS_ODBC_TABLE(position)) return FALSE;   /* no action */
    p=get_space_op(cdp, 1+sizeof(tcurstab)+sizeof(trecnum), opcode);
    if (p==NULL) return TRUE;
    *(tcurstab*)p=cursor;    p+=sizeof(tcurstab);
    *(trecnum *)p=position;  p+=sizeof(trecnum);
    return cond_send(cdp);
  }
}

CFNC DllKernel BOOL WINAPI cd_Delete_all_records(cdp_t cdp, tcursnum curs)
{ 
#ifdef CLIENT_ODBC
  if (IS_ODBC_TABLE(curs))
  { odbc_tabcurs * tc = get_odbc_tc(cdp, curs);
    return odbc_delete_all(cdp, tc->conn->hStmt, tc->s.source) != 2;
  }
  else
#endif
  { if (!IS_CURSOR_NUM(curs)) // a table
    { tobjname tabname, aplname;  WBUUID tabuuid;  tobjnum apl_objnum;  char comm[2*OBJ_NAME_LEN+20];
      if (cd_Read(cdp, TAB_TABLENUM, curs, OBJ_NAME_ATR, NULL, tabname)) return TRUE;
      if (cd_Read(cdp, TAB_TABLENUM, curs, APPL_ID_ATR, NULL, tabuuid)) return TRUE;
      if (cd_Find_object_by_id(cdp, tabuuid, CATEG_APPL, &apl_objnum)) return TRUE;
      if (cd_Read(cdp, OBJ_TABLENUM, apl_objnum, OBJ_NAME_ATR, NULL, aplname)) return TRUE;
      sprintf(comm, "DELETE FROM `%s`.`%s`", aplname, tabname);
      if (cd_SQL_execute(cdp, comm, NULL)) return TRUE;
    }
    else  // a cursor
    { trecnum cnt, r;  int num;
      if (cd_Rec_cnt(cdp, curs, &cnt)) return TRUE;
      cd_Start_transaction(cdp);  num=0;
      BOOL do_commit = cd_Sz_warning(cdp) != WAS_IN_TRANS;
      for (r=0; r<cnt; r++)
      { if (cd_Delete(cdp, curs, r)) goto err;
        if (++num >= 200)
        { num=0;
#ifdef CLIENT_GUI
#if WBVERS<900
          Set_status_nums(r, NORECNUM);
#endif
#endif
        }
      }
      if (do_commit) if (cd_Commit(cdp)) goto err;
#ifdef CLIENT_GUI
      Set_status_nums(NORECNUM, NORECNUM);
#endif
    }
    return FALSE;
   err:
    int errnum=cd_Sz_error(cdp);
#ifdef CLIENT_GUI
    Set_status_nums(NORECNUM, NORECNUM);
#endif
    client_error(cdp, errnum);
    return TRUE;
  }
}

CFNC DllKernel BOOL WINAPI cd_Delete(cdp_t cdp, tcursnum cursor, trecnum position)
{ 
#ifdef CLIENT_ODBC
  if (IS_ODBC_TABLE(cursor))
  { ltable * dt = get_ltable(cdp, cursor);
    if (dt==NULL)
      { client_error(cdp, ODBC_CURSOR_NOT_OPEN);  return TRUE; }
    if (!dt->conn->SetPos_support)  /* not supported */
      { client_error(cdp, DRIVER_NOT_CAPABLE);  return TRUE; }
    if (dt->cache_top==position)
      { cache_free(dt, 0, 1);  dt->cache_top=NORECNUM; }
    return setpos_delete_record(cdp, dt, position);
  }
#endif
  if (cursor==TAB_TABLENUM)
  { char stat[12+OBJ_NAME_LEN+3+OBJ_NAME_LEN+2];  WBUUID appl_id;
    strcpy(stat, "DROP TABLE `");
    if (cd_Read(cdp, TAB_TABLENUM, position, APPL_ID_ATR, NULL, appl_id))
      return TRUE;
    cd_Apl_id2name(cdp, appl_id, stat+12);
    strcat(stat, "`.`");
    if (cd_Read(cdp, TAB_TABLENUM, position, OBJ_NAME_ATR, NULL, stat+strlen(stat)))
      return TRUE;
    strcat(stat, "`");
    return cd_SQL_execute(cdp, stat, NULL);
  }
  else 
  { BOOL res=recoper(cdp, cursor, position, OP_DELETE);
#ifdef CLIENT_GUI
    if (cursor==OBJ_TABLENUM && cdp->IStorageCache && !res)
      delete_pcache(cdp, position);
#endif
    return res;
  }
}

CFNC DllKernel BOOL WINAPI cd_Undelete(cdp_t cdp, tcursnum cursor, trecnum position)
{ return recoper(cdp, cursor, position, OP_UNDEL); }

CFNC DllKernel BOOL WINAPI cd_Read_lock_record   (cdp_t cdp, tcursnum cursor, trecnum position)
{ return recoper(cdp, cursor, position, OP_RLOCK); }

CFNC DllKernel BOOL WINAPI cd_Read_unlock_record (cdp_t cdp, tcursnum cursor, trecnum position)
{ return recoper(cdp, cursor, position, OP_UNRLOCK); }

CFNC DllKernel BOOL WINAPI cd_Write_lock_record  (cdp_t cdp, tcursnum cursor, trecnum position)
{ return recoper(cdp, cursor, position, OP_WLOCK); }

CFNC DllKernel BOOL WINAPI cd_Write_unlock_record(cdp_t cdp, tcursnum cursor, trecnum position)
{ return recoper(cdp, cursor, position, OP_UNWLOCK); }

CFNC DllKernel BOOL WINAPI cd_Read_lock_table    (cdp_t cdp, tcursnum cursor)
{ return recoper(cdp, cursor, NORECNUM, OP_RLOCK); }

CFNC DllKernel BOOL WINAPI cd_Read_unlock_table  (cdp_t cdp, tcursnum cursor)
{ return recoper(cdp, cursor, NORECNUM, OP_UNRLOCK); }

CFNC DllKernel BOOL WINAPI cd_Write_lock_table   (cdp_t cdp, tcursnum cursor)
{ return recoper(cdp, cursor, NORECNUM, OP_WLOCK); }

CFNC DllKernel BOOL WINAPI cd_Write_unlock_table (cdp_t cdp, tcursnum cursor)
{ return recoper(cdp, cursor, NORECNUM, OP_UNWLOCK); }

CFNC DllKernel BOOL WINAPI cd_Who_prevents_locking(cdp_t cdp, ttablenum tabnum, trecnum position, BOOL write_lock, uns32 * client_number, char * user_name)
{ tptr p=get_space_op(cdp, 1+sizeof(ttablenum)+sizeof(trecnum)+1, OP_WHO_LOCKED);
  if (p==NULL) return TRUE;
  *(ttablenum*)p=tabnum;    p+=sizeof(ttablenum);
  *(trecnum  *)p=position;  p+=sizeof(trecnum);
  *p=(char)write_lock;
  REGISTER_ANSWER(client_number, sizeof(uns32));
  REGISTER_ANSWER(user_name,     sizeof(tobjname));
  return cond_send(cdp);
}

#ifdef CLIENT_PGM
object * find_variable(cdp_t cdp,  const char * name)
{ objtable * objtab = get_proj_decls_table(cdp);
  if (objtab==NULL) return NULL;
  object * obj = search1(name, objtab);
  if (obj==NULL) return NULL;
  if (obj->any.categ!=O_VAR) return NULL;
  return obj;
}

CFNC DllKernel void * WINAPI get_native_variable(cdp_t cdp, const char * variable_name, int & type, t_specif & specif, int & vallen)
{ object * obj = find_variable(cdp,  variable_name);
  if (obj==NULL) return NULL;
	if (DIRECT(obj->var.type))
  { type=ATT_TYPE(obj->var.type);
    vallen=tpsize[type];
    specif = type==ATT_MONEY ? t_specif(2) : type==ATT_CHAR ? t_specif(1) : t_specif(0);  // char or binary strings not used here
  }
  else
  { vallen=obj->var.type->type.anyt.valsize;
    if      (obj->var.type->type.anyt.typecateg==T_STR)
    { type=ATT_STRING;
      specif=obj->var.type->type.string.specif;
    }
    else if (obj->var.type->type.anyt.typecateg==T_BINARY) 
    { type=ATT_BINARY;
      specif=t_specif(vallen);
    }
    else  // type not suitable for the external access
    { type=0;
      specif=t_specif(0);
    }
  }
  return !cdp->RV.glob_vars ? (void*)1 :  // signal: the variable is from just compiled program
          cdp->RV.glob_vars->vars + obj->var.offset;
}

void parse_statement(cdp_t cdp, const char * stmt, t_clivar_dynar * clivars)
{ const char * p = stmt;  BOOL in_comm=FALSE, in_string=FALSE;
  while (*p)
  { if ((*p=='\'' || *p=='\"') && !in_comm) in_string=!in_string;
    else if (*p=='{' && !in_string) in_comm=TRUE;
    else if (*p=='}' && !in_string) in_comm=FALSE;
    else if (*p==':' && !in_string && !in_comm)
    { int l=1;  t_parmode mode;
      while (p[l]==' ') l++;
      if      (p[l]=='<')
        if (p[l+1]=='>') { mode=MODE_INOUT;  l+=2; }
        else             { mode=MODE_IN;     l++;  }
      else if (p[l]=='>')
        if (p[l+1]=='<') { mode=MODE_INOUT;  l+=2; }
        else             { mode=MODE_OUT;    l++;  }
      else               { mode=MODE_INOUT;        }
      while (p[l]==' ') l++;
      int start=l;  tname name;
#if WBVERS<900
      while (SYMCHAR(p[l])) l++;
#else
      while (is_AlphanumA(p[l], cdp->sys_charset)) l++;
#endif
      if (l-start < NAMELEN && l>start)
      { memcpy(name, p+start, l-start);  name[l-start]=0;
        Upcase9(cdp, name);
        int type;  t_specif specif;  int buflen;  void * valptr;
        valptr=get_native_variable(cdp, name, type, specif, buflen);
        if (valptr!=NULL)
        {// find if name registered:
          t_clivar * clivar = NULL;
          int i=0;
          while (i<clivars->count())
            if (!strcmp(clivars->acc0(i)->name, name))
              { clivar=clivars->acc0(i);  break; }
            else i++;
          if (clivar!=NULL)  // found, add mode
            clivar->mode = (t_parmode)(clivar->mode | (int)mode);
          else
          { clivar=clivars->next();
            if (clivar!=NULL)
            { strcpy(clivar->name, name);
              clivar->mode=mode;
              clivar->buf = valptr==(void*)1 ? NULLSTRING : valptr;  // for "1" send anything, just compiling
              clivar->wbtype=type;
              clivar->buflen=buflen;
              clivar->specif=specif.opqval;
              // clivar->actlen not defined, var-len types not used
            }
          }
        } // variable found
      } // symbol length OK
    } // ':' found
    p++;
  } // cycle on statement
}
#endif

CFNC DllKernel BOOL WINAPI cd_Open_subcursor(cdp_t cdp, tcursnum super, const char * subcurdef, tcursnum * curnum)
{ tptr p;  unsigned len;
#ifdef CLIENT_ODBC
  if (IS_ODBC_TABLE(super))
  { ltable * dt = get_ltable(cdp, super);
    if (dt==NULL) return TRUE;
    tptr sdef=sigalloc(strlen(dt->select_st)+20+strlen(subcurdef), 89);
    if (sdef==NULL) { client_error(cdp, OUT_OF_KERNEL_MEMORY);  return TRUE; }
    merge_condition(sdef, dt->select_st, (tptr)subcurdef);
    BOOL res=cd_ODBC_open_cursor(cdp, (uns32)dt->conn, curnum, sdef);
    corefree(sdef);
    return res;
  }
#endif
  len=(unsigned)strlen(subcurdef)+1;
  p=get_space_op(cdp, 1+sizeof(tcursnum)+len, OP_OPEN_SUBCURSOR);
  if (p==NULL) return TRUE;
  *(tcursnum*)p=super;  p+=sizeof(tcursnum);
  memcpy(p, subcurdef, len);
  cdp->ans_array[cdp->ans_ind]=curnum;
  cdp->ans_type [cdp->ans_ind]=sizeof(tcursnum);
  cdp->ans_ind++;
  return cond_send(cdp);
}

CFNC DllKernel BOOL WINAPI cd_Close_cursor(cdp_t cdp, tcursnum curnum)
{ BOOL closed=FALSE;
#ifdef CLIENT_ODBC
  if (IS_ODBC_TABLE(curnum) || curnum==-2)  // closing ODBC cursors
  { for (t_connection * conn = cdp->odbc.conns;  conn;  conn=conn->next_conn)
    { for (int i=0;  i<conn->ltabs.count(); )
      { odbc_tabcurs * tc = conn->ltabs.acc0(i);
        if (!tc->is_table && (curnum==-2 || tc->odbc_num == curnum))
        { corefree(tc->s.source);
          delete tc->ltab;
          conn->ltabs.delet(i);
          closed=TRUE;
        }
        else i++;
      }
    }
    if (IS_ODBC_TABLE(curnum) && !closed) client_error(cdp, ODBC_CURSOR_NOT_OPEN);
  }
#endif
  if (curnum==-2)
  { 
    inval_table_d(cdp, NOOBJECT, CATEG_DIRCUR);
    tptr p=get_space_op(cdp, 1+1, OP_CNTRL);
    if (p==NULL) return TRUE;
    *p=SUBOP_CLOSE_CURSORS;
    return cond_send(cdp) && !closed;  // nothing closed: not a normal cursor nor a ODBC cursor
  }
  else  // closing individual cursor on the SQL server
  { 
    inval_table_d(cdp, curnum, CATEG_DIRCUR);
    if (!IS_ODBC_TABLE(curnum))
    { tptr p=get_space_op(cdp, 1+sizeof(tcursnum), OP_CLOSE_CURSOR);
      if (p==NULL) return TRUE;
      *(tcursnum*)p=curnum;
      if (!cond_send(cdp)) closed=TRUE;
    }
    return !closed;
  }
}

CFNC DllKernel BOOL WINAPI cd_Free_deleted(cdp_t cdp, ttablenum tb)
{ tptr p=get_space_op(cdp, 1+sizeof(ttablenum)+1, OP_INDEX);
  if (p==NULL) return TRUE;
  *(ttablenum*)p=tb;  p[sizeof(ttablenum)]=OP_OP_FREE; /* suboper. */
  return cond_send(cdp);
}

CFNC DllKernel BOOL WINAPI cd_Truncate_table(cdp_t cdp, ttablenum tb)
{ tptr p=get_space_op(cdp, 1+sizeof(ttablenum)+1, OP_INDEX);
  if (p==NULL) return TRUE;
  *(ttablenum*)p=tb;  p[sizeof(ttablenum)]=OP_OP_TRUNCATE; /* suboper. */
  return cond_send(cdp);
}

CFNC DllKernel BOOL WINAPI cd_Make_persistent(cdp_t cdp, tcursnum cursnum)
{ tptr p=get_space_op(cdp, 1+sizeof(tcursnum)+1, OP_INDEX);
  if (p==NULL) return TRUE;
  *(tcursnum*)p=cursnum;  p+=sizeof(tcursnum);
  *p=OP_OP_MAKE_PERSIS; /* suboper. */
  return cond_send(cdp);
}

CFNC DllKernel BOOL WINAPI cd_List_triggers(cdp_t cdp, ttablenum tb, t_trig_info ** trgs)
{ tptr p=get_space_op(cdp, 1+sizeof(ttablenum)+1, OP_INDEX);
  if (p==NULL) return TRUE;
  *(ttablenum*)p=tb;  p[sizeof(ttablenum)]=OP_OP_TRIGGERS; /* suboper. */
  cdp->ans_array[cdp->ans_ind]=trgs;
  cdp->ans_type [cdp->ans_ind]=ANS_SIZE_DYNALLOC;
  cdp->ans_ind++;
  return cond_send(cdp);
}

CFNC DllKernel BOOL WINAPI Delete_history(ttablenum tb, BOOL datim_type, timestamp dtm)
{ tptr p;  cdp_t cdp = GetCurrTaskPtr();
  p=get_space_op(cdp, 1+sizeof(ttablenum)+1+sizeof(wbbool)+sizeof(timestamp), OP_INDEX);
  if (p==NULL) return TRUE;
  *(ttablenum*)p=tb;          p+=sizeof(ttablenum);
  *(p++)=OP_OP_DELHIST; /* suboper. */
  *(wbbool   *)p=(wbbool)datim_type;  p+=sizeof(wbbool);
  *(timestamp*)p=dtm;
  return cond_send(cdp);
}

CFNC DllKernel BOOL WINAPI cd_Enable_index(cdp_t cdp, ttablenum tb, int which, BOOL enable)
{ tptr p=get_space_op(cdp, 1+sizeof(ttablenum)+1+1+sizeof(wbbool), OP_INDEX);
  if (p==NULL) return TRUE;
  *(ttablenum*)p=tb; p+=sizeof(ttablenum);
  *(p++)=OP_OP_ENABLE; /* suboper. */
  *(p++)=(uns8)which;
  *(wbbool*)p=(wbbool)enable;
  return cond_send(cdp);
}

CFNC DllKernel BOOL WINAPI cd_Uninst_table(cdp_t cdp, ttablenum tb)
{ tptr p=get_space_op(cdp, 1+sizeof(ttablenum)+1, OP_INDEX);
  if (p==NULL) return TRUE;
  *(ttablenum*)p=tb; p+=sizeof(ttablenum);
  *p=OP_OP_UNINST; /* suboper. */
  return cond_send(cdp);
}

static BOOL send_params(cdp_t cdp, int hStmt, const t_clivar * clivars, int clivar_count)
// Sends the values of host variables registered in clivars to the server
// Performs some consistency tests on clivars
{ int i, size=2*sizeof(sig16);  bool err=false;
 // normalize decription of clivars: very long strings are not supported
  for (i=0;  i<clivar_count;  i++)
  { const t_clivar * clivar = clivars+i;
    if (!IS_HEAP_TYPE(clivar->wbtype))
      if (clivar->buflen>0xfffc)
        *(uns32*)&clivar->buflen=0xfffc;
  }
 // calc. the size: 
  for (i=0;  i<clivar_count;  i++)
  { const t_clivar * clivar = clivars+i;
    if (!*clivar->name || !clivar->wbtype || clivar->wbtype>=ATT_AFTERLAST) err=true;
    size+=cdp->sizeof_tname()+2*sizeof(sig16)+sizeof(uns32);
    if (clivar->mode==MODE_IN || clivar->mode==MODE_INOUT)
      if (!IS_HEAP_TYPE(clivar->wbtype))
        size += sizeof(sig16)+clivar->buflen;
      else if (clivar->actlen<=SEND_PAR_COMMON_LIMIT)  // short var-len values
        size += sizeof(sig32)+(int)clivar->actlen;
      else                                             // long var-len values
        size += sizeof(sig32);
    else if (clivar->mode==MODE_OUT)
    { if (!IS_HEAP_TYPE(clivar->wbtype)) size += sizeof(sig16); // buffer size sended even for OUT parameters
    }
    else err=true;
    //if (clivar->wbtype==ATT_STRING || clivar->wbtype==ATT_BINARY)
      //if (t_specif(clivar->specif).length>MAX_FIXED_STRING_LENGTH) // error in the client var descriptor -- no, long string is OK!
      //if (t_specif(clivar->specif).length == 0)  -- CGI clients use this
      //  err=true;
  }
  if (err) { client_error(cdp, ERROR_IN_FUNCTION_CALL);  return TRUE; }
  tptr pars=(tptr)corealloc(size, 85);
  if (pars==NULL) { client_error(cdp, OUT_OF_MEMORY);  return TRUE; }
  *(sig16*)pars=SEND_EMB_TYPED;  int pos=sizeof(sig16);
  for (i=0;  i<clivar_count;  i++)
  { const t_clivar * clivar = clivars+i;
    *(sig16*)(pars+pos)=(sig16)clivar->mode;    pos+=sizeof(sig16);
    strmaxcpy(pars+pos, clivar->name, cdp->sizeof_tname());  pos+=cdp->sizeof_tname();
    *(sig16*)(pars+pos)=(sig16)clivar->wbtype;  pos+=sizeof(sig16);
    *(uns32*)(pars+pos)=       clivar->specif;  pos+=sizeof(uns32);
    
    if (clivar->mode==MODE_IN || clivar->mode==MODE_INOUT)
    { if (!IS_HEAP_TYPE(clivar->wbtype))
      { *(uns16*)(pars+pos)=(uns16)clivar->buflen; pos+=sizeof(uns16);
        if (clivar->buf) memcpy(pars+pos, clivar->buf, clivar->buflen); // unless a variable from just compiled program
        pos+=clivar->buflen;
      }
      else if (clivar->actlen<=SEND_PAR_COMMON_LIMIT)
      { *(uns32*)(pars+pos)=(uns32)clivar->actlen;  pos+=sizeof(uns32);
        if (clivar->buf) memcpy(pars+pos, clivar->buf, clivar->actlen); // unless a variable from just compiled program
        pos+=(int)clivar->actlen;
      }
      else // no parameters may be skipped, order is important in pull
      { *(uns32*)(pars+pos)=0;  pos+=sizeof(uns32); }
    }
    else  // MODE_OUT
      if (!IS_HEAP_TYPE(clivar->wbtype))
        { *(uns16*)(pars+pos)=(uns16)clivar->buflen; pos+=sizeof(uns16); }
  }
  *(sig16*)(pars+pos)=SEND_PAR_DELIMITER;
  BOOL res=cd_Send_params(cdp, hStmt, size, pars);
  corefree(pars); 
  if (res) return res;
 // send big values per partes:
  pars=NULL;
  for (i=0;  i<clivar_count;  i++)
  { const t_clivar * clivar = clivars+i;
    if (clivar->mode==MODE_IN || clivar->mode==MODE_INOUT)
      if (IS_HEAP_TYPE(clivar->wbtype) && clivar->actlen>SEND_PAR_COMMON_LIMIT)
      { 
        pars=(tptr)corealloc(sizeof(sig16)+cdp->sizeof_tname()+3*sizeof(uns32)+(clivar->actlen), 85);
        if (pars==NULL) { client_error(cdp, OUT_OF_MEMORY);  return TRUE; }
        uns32 offset=0;
        do
        { unsigned partsize = (int)clivar->actlen-offset;
          //if (partsize > SEND_PAR_PART_SIZE) partsize=SEND_PAR_PART_SIZE;  -- removed, makes things slower
          *(sig16*)pars=SEND_EMB_TYPED_EXT;           pos =sizeof(sig16);
          strmaxcpy(pars+pos, clivar->name, cdp->sizeof_tname());  pos+=cdp->sizeof_tname();
          *(uns32*)(pars+pos)=(uns32)clivar->actlen;  pos+=sizeof(uns32);
          *(uns32*)(pars+pos)=(uns32)offset;          pos+=sizeof(uns32);
          *(uns32*)(pars+pos)=(uns32)partsize;        pos+=sizeof(uns32);
          if (clivar->buf) memcpy(pars+pos, (tptr)clivar->buf+offset, partsize); // unless a variable from just compiled program
          pos+=partsize;
          res=cd_Send_params(cdp, hStmt, pos, pars);
          if (res) break;
          offset+=partsize;
        } while (offset<clivar->actlen);
        corefree(pars); 
        if (res) break;
      }
  }
  return res;
}

static void normalize_string_specifs(d_table * td)
// Servers older tham 8.1 may return undefined upper bits of wide_char in string specif
{ int i;  d_attr * pattr;
  for (i=0, pattr=FIRST_ATTR(td);  i<td->attrcnt;  i++, pattr=NEXT_ATTR(td,pattr))
  { int tp = pattr->type;
    if (tp==ATT_STRING || tp==ATT_TEXT || tp==ATT_CHAR)
      pattr->specif.wide_char &= 1;  // value 2 should never be returned from any SQL server
  }
}

#if WBVERS>=950
d_table * convert_protocol_in_td(d_table_0 * td0)
{ int newsize = sizeof(d_table)+sizeof(d_attr)*td0->attrcnt;
  d_table * td = (d_table *)corealloc(newsize+2*sizeof(tobjname), 88);
  td->attrcnt=td0->attrcnt;
  strcpy(td->schemaname, td0->schemaname);
  strcpy(td->selfname, td0->selfname);
  td->tabcnt=td0->tabcnt;
  td->tabdef_flags=td0->tabdef_flags;
  td->updatable=td0->updatable;
  for (int i=0;  i<td0->attrcnt;  i++)
  { td->attribs[i].a_flags=td0->attribs[i].a_flags;
    td->attribs[i].multi=td0->attribs[i].multi;
    strcpy(td->attribs[i].name, td0->attribs[i].name);
    td->attribs[i].needs_prefix=false;
    td->attribs[i].nullable=td0->attribs[i].nullable;
    td->attribs[i].prefnum=0;
    td->attribs[i].specif=td0->attribs[i].specif;
    td->attribs[i].type=td0->attribs[i].type;
  }
  ((char*)td)[newsize]=((char*)td)[sizeof(tobjname)+newsize]=0;  // empty prefix name
  corefree(td0);
  return td;
}
#else
d_table * convert_protocol_in_td(d_table_1 * td1)
{ int newsize = sizeof(d_table)+sizeof(d_attr)*td1->attrcnt;
  d_table * td = (d_table *)corealloc(newsize+2*sizeof(tobjname), 88);
  td->attrcnt=(tattrib)td1->attrcnt;
  strcpy(td->schemaname, td1->schemaname);
  strcpy(td->selfname, td1->selfname);
  td->tabcnt=td1->tabcnt;
  td->tabdef_flags=td1->tabdef_flags;
  td->updatable=td1->updatable;
  for (int i=0;  i<td1->attrcnt;  i++)
  { td->attribs[i].a_flags=td1->attribs[i].a_flags;
    td->attribs[i].multi=td1->attribs[i].multi;
    strcpy(td->attribs[i].name, td1->attribs[i].name);
    td->attribs[i].needs_prefix=false;
    td->attribs[i].nullable=td1->attribs[i].nullable;
    td->attribs[i].prefnum=0;
    td->attribs[i].specif=td1->attribs[i].specif;
    td->attribs[i].type=td1->attribs[i].type;
  }
  ((char*)td)[newsize]=((char*)td)[sizeof(tobjname)+newsize]=0;  // empty prefix name
  corefree(td1);
  return td;
}
#endif

CFNC DllKernel BOOL WINAPI cd_Get_descriptor(cdp_t cdp, tobjnum curs, tcateg cat, d_table ** td)
{ 
#ifdef CLIENT_ODBC
  if (IS_ODBC_TABLEC(cat, curs) && cdp->odbc.hEnv)
  { odbc_tabcurs * tc = get_odbc_tc(cdp, curs);
    if (tc==NULL)
      { *td=NULL;  client_error(cdp, ODBC_CURSOR_NOT_OPEN);  return TRUE; }
    if (tc->is_table)
      *td=make_odbc_descriptor(tc->conn, tc->s.odbc_link, TRUE);
    else
      *td=make_result_descriptor(tc->ltab);
    return *td==NULL;
  }
  else
#endif
  { *td=NULL;
   // must look for embedded variables and send them:
    if (cat==CATEG_CURSOR)
    { tptr buf = cd_Load_objdef(cdp, curs, CATEG_CURSOR);
      if (buf==NULL) return TRUE; 
      t_clivar_dynar clivars;
#ifdef CLIENT_PGM
      parse_statement(cdp, buf, &clivars);
      if (clivars.count())
        if (send_params(cdp, 0, clivars.acc0(0), clivars.count())) { corefree(buf);  return TRUE; }
#endif
      corefree(buf);
    }
   // normal operation:
    tptr p=get_space_op(cdp, 1+sizeof(tobjnum)+sizeof(tcateg)+1, OP_INDEX);
    if (p==NULL) return TRUE;
    *(tobjnum*)p=curs;  p+=sizeof(tobjnum);
    *(p++)=OP_OP_COMPAT;  /* suboper. */
    *(tcateg*)p=cat;
    cdp->ans_array[cdp->ans_ind]=td;
    cdp->ans_type [cdp->ans_ind]=ANS_SIZE_DYNALLOC;
    cdp->ans_ind++;
    if (cond_send(cdp)) return TRUE;
#if WBVERS>=950
    if (cdp->protocol_version() == 0)
      *td=convert_protocol_in_td((d_table_0*)*td);
#else      
    if (cdp->protocol_version() == 1)
      *td=convert_protocol_in_td((d_table_1*)*td);
#endif
    if (!cdp->in_package) normalize_string_specifs(*td);
    return FALSE;
  }
}

CFNC DllKernel BOOL WINAPI cd_xSave_table(cdp_t cdp, ttablenum table, FHANDLE hFile)
{ tptr p;  BOOL res;  t_container * contr;
  if (hFile!=INVALID_FHANDLE_VALUE)
  { cdp->auxptr = contr = new t_container;
    if (!contr)
      { client_error(cdp, OUT_OF_KERNEL_MEMORY);  return TRUE; }
    if (!contr->wopen(hFile))
      { delete contr;  client_error(cdp, OS_FILE_ERROR);  return TRUE; }
  }
  else contr=NULL;
  p=get_space_op(cdp, 1+sizeof(ttablenum), OP_SAVE_TABLE);
  if (p==NULL) { if (hFile!=INVALID_FHANDLE_VALUE) delete contr;  return TRUE; }
  *(ttablenum*)p=table;
  res=cond_send(cdp);
  if (contr!=NULL) delete contr;
  cdp->auxptr=NULL;
  return res;
}

CFNC DllKernel BOOL WINAPI cd_Restore_table(cdp_t cdp, ttablenum table, FHANDLE hFile, BOOL flag)
{ t_container * contr;  BOOL res;
 // prepare the container for the transport of the file contents:
  if (hFile!=INVALID_FHANDLE_VALUE)
  { cdp->auxptr = contr = new t_container;
    if (!contr)
      { client_error(cdp, OUT_OF_KERNEL_MEMORY);  return TRUE; }
    if (!contr->ropen(hFile))
      { delete contr;  client_error(cdp, OS_FILE_ERROR);  return TRUE; }
  }
  else contr=NULL;  // cdp->auxptr already points to the container
 // request to the server:
  tptr p=get_space_op(cdp, 1+sizeof(ttablenum)+1, OP_REST_TABLE);
  if (p==NULL) res=TRUE;
  else
  { *(ttablenum*)p = table;  p+=sizeof(ttablenum);
    *p=(char)flag;
    res=cond_send(cdp);
  }
 // delete the containter:
  if (contr!=NULL) delete contr;
  cdp->auxptr=NULL;
  return res;
}

CFNC DllKernel BOOL WINAPI cd_Translate(cdp_t cdp, tcursnum curs, trecnum crec, int tbord, trecnum * trec)
{ if (IS_ODBC_TABLE(curs)) { client_error(cdp, CANNOT_FOR_ODBC);  return TRUE; }
  *trec=NORECNUM;
  tptr p=get_space_op(cdp, 1+sizeof(tcursnum)+sizeof(trecnum)+1, OP_TRANSL);
  if (p==NULL) return TRUE;
  *(tcursnum*)p=curs;  p+=sizeof(tcursnum);
  *(trecnum *)p=crec;  p+=sizeof(trecnum);
  *           p=(uns8)tbord;
  cdp->ans_array[cdp->ans_ind]=trec;
  cdp->ans_type [cdp->ans_ind]=sizeof(trecnum);
  cdp->ans_ind++;
  return cond_send(cdp);
}

CFNC DllKernel BOOL WINAPI cd_Get_base_tables(cdp_t cdp, tcursnum curs, uns16 * cnt, ttablenum * tabs)
{ tptr p;
  if (IS_ODBC_TABLE(curs)) { client_error(cdp, CANNOT_FOR_ODBC);  return TRUE; }
  p=get_space_op(cdp, 1+sizeof(tcursnum)+sizeof(uns16), OP_BASETABS);
  if (p==NULL) return TRUE;
  *(tcursnum*)p=curs;  p+=sizeof(tcursnum);
  *(uns16   *)p=*cnt;
  cdp->ans_array[cdp->ans_ind]=cnt;
  cdp->ans_type [cdp->ans_ind]=sizeof(uns16);
  cdp->ans_ind++;
  cdp->ans_array[cdp->ans_ind]=tabs;
  cdp->ans_type [cdp->ans_ind]=*cnt*sizeof(ttablenum);
  cdp->ans_ind++;
  return cond_send(cdp);
}

CFNC DllKernel BOOL WINAPI Recstate(cdp_t cdp, tcursnum cursor, trecnum rec, uns8 * state)
{ tptr p;
  p=get_space_op(cdp, 1+sizeof(tcursnum)+sizeof(trecnum), OP_RECSTATE);
  if (p==NULL) return TRUE;
  *(tcursnum*)p=cursor;  p+=sizeof(tcursnum);
  *(trecnum *)p=rec;
  cdp->ans_array[cdp->ans_ind]=state;
  cdp->ans_type [cdp->ans_ind]=sizeof(uns8);
  cdp->ans_ind++;
  return cond_send(cdp);
}

CFNC DllKernel BOOL WINAPI cd_Table_stat(cdp_t cdp, ttablenum table, trecnum * stat)
{ tptr p=get_space_op(cdp, 1+sizeof(ttablenum), OP_TABINFO);
  if (p==NULL) return TRUE;
  *(ttablenum*)p=table;
  cdp->ans_array[cdp->ans_ind]=stat;
  cdp->ans_type [cdp->ans_ind]=3*sizeof(trecnum);
  cdp->ans_ind++;
  return cond_send(cdp);
}

CFNC DllKernel BOOL WINAPI cd_Read_record(cdp_t cdp, tcursnum cursor, trecnum position, void * buf, uns32 datasize)
{ 
#ifdef CLIENT_ODBC
  if (IS_ODBC_TABLE(cursor))
  { ltable * ltab=get_ltable(cdp, cursor);  int i;  atr_info * catr;
    if (ltab==NULL)
      { client_error(cdp, ODBC_CURSOR_NOT_OPEN);  return TRUE; }
    if (!ltab->conn->SetPos_support)  /* not supported */
      { client_error(cdp, DRIVER_NOT_CAPABLE);  return TRUE; }
    if (position!=ltab->cache_top)
    { if (ltab->cache_top!=NORECNUM) cache_free(ltab, 0, 1);
      ltab->cache_top=NORECNUM;
      if (odbc_load_record(cdp, ltab, position, (tptr)ltab->cache_ptr))
        { client_error(cdp, OUT_OF_TABLE);  return TRUE; }  /* record does not exist or cannot load */
      ltab->cache_top=position;
      for (i=1, catr=ltab->attrs+1;  i<(int)ltab->attrcnt;  i++, catr++)
        catr->changed=FALSE;
    }
    memcpy(buf, (tptr)ltab->cache_ptr+1, ltab->rr_datasize-1 < datasize ? ltab->rr_datasize-1 : datasize);
    return FALSE;
  }
  else
#endif
  { tptr p=get_space_op(cdp, 1+sizeof(tcursnum)+sizeof(trecnum)+sizeof(uns32), OP_READREC);
    if (p==NULL) return TRUE;
    *(tcursnum*)p=cursor;    p+=sizeof(tcursnum);
    *(trecnum *)p=position;  p+=sizeof(trecnum);
    *(uns32   *)p=datasize;
    cdp->ans_array[cdp->ans_ind]=buf;
    cdp->ans_type [cdp->ans_ind]=datasize;
    cdp->ans_ind++;
    return cond_send(cdp);
  }
}

CFNC DllKernel BOOL WINAPI cd_Write_record(cdp_t cdp, tcursnum cursor, trecnum position, const void * buf, uns32 datasize)
{ 
#ifdef CLIENT_ODBC
  if (IS_ODBC_TABLE(cursor))
  { ltable * ltab=get_ltable(cdp, cursor);  int i;  atr_info * catr;
    if (ltab==NULL)
      { client_error(cdp, ODBC_CURSOR_NOT_OPEN);  return TRUE; }
    if (!ltab->conn->SetPos_support)   /* not supported */
      { client_error(cdp, DRIVER_NOT_CAPABLE);  return TRUE; }
    if (position!=ltab->cache_top)
    { if (ltab->cache_top!=NORECNUM) cache_free(ltab, 0, 1);
      ltab->cache_top=NORECNUM;
      if (odbc_load_record(cdp, ltab, position, (tptr)ltab->cache_ptr))
        { client_error(cdp, OUT_OF_TABLE);  return TRUE; }  /* record does not exist or cannot load */
      ltab->cache_top=position;
    }
    memcpy((tptr)ltab->cache_ptr+1, buf, ltab->rr_datasize-1 < datasize ? ltab->rr_datasize-1 : datasize);
    for (i=1, catr=ltab->attrs+1;  i<(int)ltab->attrcnt;  i++, catr++)
      catr->changed=TRUE;
    BOOL res=setpos_synchronize_view(cdp, TRUE, ltab, (tptr)ltab->cache_ptr, position, FALSE);
    for (i=1, catr=ltab->attrs+1;  i<(int)ltab->attrcnt;  i++, catr++)
      catr->changed=FALSE;
    return res;
  }
  else
#endif
  { tptr p=get_space_op(cdp, 1+sizeof(tcursnum)+sizeof(trecnum)+sizeof(uns32)+datasize, OP_WRITEREC);
    if (p==NULL) return TRUE;
    *(tcursnum*)p=cursor;    p+=sizeof(tcursnum);
    *(trecnum *)p=position;  p+=sizeof(trecnum);
    *(uns32   *)p=datasize;  p+=sizeof(uns32);
    memcpy(p, buf, datasize);
    return cond_send(cdp);
  }
}

unsigned get_column_extent(const d_attr * pdatr)
{ if (IS_HEAP_TYPE(pdatr->type)) return 0;  // not used
  if (is_string(pdatr->type))  return pdatr->specif.length+1;
  if (pdatr->type==ATT_BINARY) return pdatr->specif.length;
  return tpsize[pdatr->type];
}

static unsigned get_data_extent(const t_column_val_descr * colvaldescr, unsigned colcount, const d_table * td)
{ unsigned total = 0;  
  for (;  colcount--;  colvaldescr++)
  { const d_attr * pdatr = ATTR_N(td,colvaldescr->column_number);
    if (IS_HEAP_TYPE(pdatr->type))
      if (colvaldescr->column_value) total+=sizeof(uns32)+colvaldescr->value_length;
      else total+=sizeof(uns32);
    else total+=get_column_extent(pdatr);
  }
  return total;
}

static void pack_data(unsigned sizeof_tattrib, const t_column_val_descr * colvaldescr, unsigned colcount, const d_table * td, tptr databuf)
{ int i;
  *(uns16*)databuf = colcount;  databuf+=sizeof(uns16);
  for (i=0;  i<colcount;  i++)
    write_attrib(databuf, &colvaldescr[i].column_number, sizeof_tattrib);
  for (i=0;  i<colcount;  i++, colvaldescr++)
  { const d_attr * pdatr = ATTR_N(td,colvaldescr->column_number);
    if (IS_HEAP_TYPE(pdatr->type))
      if (colvaldescr->column_value) 
      { *(uns32*)databuf = colvaldescr->value_length;  databuf+=sizeof(uns32); 
        memcpy(databuf, colvaldescr->column_value, colvaldescr->value_length);
        databuf+=colvaldescr->value_length;
      }
      else 
      { *(uns32*)databuf = 0;  databuf+=sizeof(uns32); }
    else 
    { unsigned size = get_column_extent(pdatr);
      if (colvaldescr->column_value) 
        memcpy(databuf, colvaldescr->column_value, size);
      else  // value not specified -> NULL implies
        if (is_string(pdatr->type))  
          *databuf=0;
        else if (pdatr->type==ATT_BINARY) 
          memset(databuf, 0, pdatr->specif.length);
#ifdef CLIENT_GUI
#if WBVERS<900
        else 
          catr_set_null(databuf, pdatr->type);
#endif
#endif
      databuf+=size;
    }
  }
}

CFNC DllKernel BOOL WINAPI cd_Write_record_ex(cdp_t cdp, tcurstab curs, trecnum position, unsigned colcount, const t_column_val_descr * colvaldescr)
{ if (IS_ODBC_TABLE(curs)) { client_error(cdp, CANNOT_FOR_ODBC);  return TRUE; }
  const d_table * td = cd_get_table_d(cdp, curs, CATEG_TABLE);
  if (!td) return TRUE;
  unsigned sizeof_tattrib = cdp->sizeof_tattrib();
  unsigned datasize=get_data_extent(colvaldescr, colcount, td);
  tptr p=get_space_op(cdp, 1+sizeof(tcursnum)+sizeof(trecnum)+sizeof(uns32)+sizeof(uns16)+colcount*sizeof_tattrib+datasize, OP_WRITERECEX);
  if (p==NULL) { release_table_d(td);  return TRUE; }
  *(tcursnum*)p=curs;      p+=sizeof(tcursnum);
  *(trecnum *)p=position;  p+=sizeof(trecnum);
  *(uns32   *)p=datasize;  p+=sizeof(uns32);
  pack_data(sizeof_tattrib, colvaldescr, colcount, td, p);
  release_table_d(td);
  return cond_send(cdp);
}

CFNC DllKernel BOOL WINAPI cd_Insert_record_ex(cdp_t cdp, tcurstab curs, trecnum * position, unsigned colcount, const t_column_val_descr * colvaldescr)
{ if (IS_ODBC_TABLE(curs)) { client_error(cdp, CANNOT_FOR_ODBC);  return TRUE; }
  const d_table * td = cd_get_table_d(cdp, curs, CATEG_TABLE);
  if (!td) return TRUE;
  unsigned sizeof_tattrib = cdp->sizeof_tattrib();
  unsigned datasize=get_data_extent(colvaldescr, colcount, td);
  tptr p=get_space_op(cdp, 1+sizeof(tcursnum)+sizeof(uns32)+sizeof(uns16)+colcount*sizeof_tattrib+datasize, OP_INSERTRECEX);
  if (p==NULL) { release_table_d(td);  return TRUE; }
  *(tcursnum*)p=curs;      p+=sizeof(tcursnum);
  *(uns32   *)p=datasize;  p+=sizeof(uns32);
  pack_data(sizeof_tattrib, colvaldescr, colcount, td, p);
  release_table_d(td);
  REGISTER_ANSWER(position, sizeof(trecnum));
  return cond_send(cdp);
}

CFNC DllKernel BOOL WINAPI cd_Append_record_ex(cdp_t cdp, tcurstab curs, trecnum * position, unsigned colcount, const t_column_val_descr * colvaldescr)
{ if (IS_ODBC_TABLE(curs)) { client_error(cdp, CANNOT_FOR_ODBC);  return TRUE; }
  const d_table * td = cd_get_table_d(cdp, curs, CATEG_TABLE);
  if (!td) return TRUE;
  unsigned sizeof_tattrib = cdp->sizeof_tattrib();
  unsigned datasize=get_data_extent(colvaldescr, colcount, td);
  tptr p=get_space_op(cdp, 1+sizeof(tcursnum)+sizeof(uns32)+sizeof(uns16)+colcount*sizeof_tattrib+datasize, OP_APPENDRECEX);
  if (p==NULL) { release_table_d(td);  return TRUE; }
  *(tcursnum*)p=curs;      p+=sizeof(tcursnum);
  *(uns32   *)p=datasize;  p+=sizeof(uns32);
  pack_data(sizeof_tattrib, colvaldescr, colcount, td, p);
  release_table_d(td);
  REGISTER_ANSWER(position, sizeof(trecnum));
  return cond_send(cdp);
}

CFNC DllKernel BOOL WINAPI Add_value(tcursnum cursor, trecnum position, tattrib attr, void * buf, uns16 datasize)
{ tptr p;  cdp_t cdp = GetCurrTaskPtr();
  unsigned sizeof_tattrib = cdp->sizeof_tattrib();
  p=get_space_op(cdp, 1+sizeof(tcursnum)+sizeof(trecnum)+sizeof_tattrib+
                                          /*sizeof(uns16)+*/datasize, OP_ADDVALUE);
  if (p==NULL) return TRUE;
  *(tcursnum*)p=cursor;    p+=sizeof(tcursnum);
  *(trecnum *)p=position;  p+=sizeof(trecnum);
  write_attrib(p, &attr, sizeof_tattrib);
  memcpy(p, buf, datasize);
  return cond_send(cdp);
}

CFNC DllKernel BOOL WINAPI Del_value(tcursnum cursor, trecnum position, tattrib attr, void * buf, uns16 datasize)
{ tptr p;  cdp_t cdp = GetCurrTaskPtr();
  unsigned sizeof_tattrib = cdp->sizeof_tattrib();
  p=get_space_op(cdp, 1+sizeof(tcursnum)+sizeof(trecnum)+sizeof_tattrib+
                                          /*sizeof(uns16)+*/datasize, OP_DELVALUE);
  if (p==NULL) return TRUE;
  *(tcursnum*)p=cursor;    p+=sizeof(tcursnum);
  *(trecnum *)p=position;  p+=sizeof(trecnum);
  write_attrib(p, &attr, sizeof_tattrib);
  memcpy(p, buf, datasize);
  return cond_send(cdp);
}

CFNC DllKernel BOOL WINAPI Remove_invalid(tcursnum cursor, trecnum position, tattrib attr)
{ tptr p;  cdp_t cdp = GetCurrTaskPtr();
  unsigned sizeof_tattrib = cdp->sizeof_tattrib();
  p=get_space_op(cdp, 1+sizeof(tcursnum)+sizeof(trecnum)+sizeof_tattrib, OP_REMINVAL);
  if (p==NULL) return TRUE;
  *(tcursnum*)p=cursor;    p+=sizeof(tcursnum);
  *(trecnum *)p=position;  p+=sizeof(trecnum);
  write_attrib(p, &attr, sizeof_tattrib);
  return cond_send(cdp);
}

static void SendDirectBreak(cdp_t cdp)
{ 
#ifdef WINS
  UINT uMsg = RegisterWindowMessage("WBBreak_message");
	HWND hServerWnd = FindWindow("WBServerClass", NULL);
	if (hServerWnd)
  	PostMessage(hServerWnd, uMsg, 0, cdp->LinkIdNumber);
#endif
}

CFNC DllKernel BOOL WINAPI cd_Break(cdp_t cdp)
{ if (!cdp) return TRUE;
  if (!(cdp->RV.holding & 2)) return FALSE;
  if (cdp->in_use==PT_DIRECT)
	  SendDirectBreak(cdp);
  else if (cdp->in_use==PT_REMOTE)
    return !cdp->pRemAddr->SendBreak(cdp);  // error for HTTP tunnelling or when the server port is not accessible
  return FALSE;
}

CFNC DllKernel BOOL WINAPI cd_Set_password(cdp_t cdp, const char * username, const char * password)
// Sets the new password. If setting own password in a conditional attachment, must set [logged_as_user].
// When *password==1 then the password start at password+1 and user must change it.
{// replace NULL username by the caller name:
  const char * caller_name = cd_Who_am_I(cdp);
  if (!username || !*username) username=caller_name;
  bool must_change_pass;
  if (*password==1) { must_change_pass=true;  password++; }
  else must_change_pass=false;
 // check the password length:
  char buf[13];  sig32 value;
  if (!cd_Get_property_value(cdp, "@SQLSERVER", "MinPasswordLen", 0, buf, sizeof(buf)))
    if (str2int(buf, &value))
      if (strlen(password) < value)
        { SET_ERR_MSG(cdp, username);  client_error(cdp, BAD_PASSWORD);  return TRUE; }
 // find the user:
  bool sysname;  tobjnum objnum;
  if (!username || !*username) username=caller_name;
  if (cd_Find2_object(cdp, username, NULL, CATEG_USER, &objnum))  // logged_as_user may not be defined now
    return TRUE;  // user not found
  sysname = *username=='_';
 // set cycles and compute digest:
  uns8 passwd[sizeof(uns32)+MD5_SIZE];
  unsigned cycles = sysname || must_change_pass ? 1 : PASSWORD_CYCLES;
  *(uns32*)passwd=cycles;
  if (get_password_digest(cdp, password, cycles+1, passwd+sizeof(uns32)))
    { client_error(cdp, OUT_OF_KERNEL_MEMORY);  return TRUE; }
 // write the password:
  tptr p=get_space_op(cdp, 1+1+sizeof(tobjnum)+sizeof(passwd), OP_CNTRL);
  if (p==NULL) return TRUE;
  *(p++)=SUBOP_SET_PASSWORD;
  *(tobjnum*)p=objnum;  p+=sizeof(tobjnum);
  memcpy(p, passwd, sizeof(passwd));
  if (cond_send(cdp)) return TRUE;
 // define logged_as_user:
  if (!my_stricmp(caller_name, username))
    cdp->logged_as_user=objnum;
  return FALSE;
}

#ifdef WINS
#include <rpcdce.h>
#endif

CFNC DllExport void WINAPI create_uuid(WBUUID appl_id)
{ 
#ifdef WINS
  UUID Uuid;
  if (UuidCreate(&Uuid) == RPC_S_OK)
    memcpy(appl_id, Uuid.Data4+2, 6);        
#endif
  uns32 dt=Today();
  int year=Year(dt), month=Month(dt), day=Day(dt);
  dt=Now()/1000 + 86400L*((day-1)+31L*((month-1)+12*(year-1996)));
  *(uns32*)(appl_id+6)=dt;
#ifdef WINS
  SYSTEMTIME st;
  GetSystemTime(&st);
  *(uns16*)(appl_id+10)=st.wMilliseconds;
#endif
}

static void diff_uuid(const WBUUID patt_uuid)
{ WBUUID uuid;
  do create_uuid(uuid);
  while (!memcmp(patt_uuid, uuid, UUID_SIZE));
}

CFNC DllExport BOOL WINAPI cd_Insert_object(cdp_t cdp, const char * name, tcateg category, tobjnum * objnum)
{ t_op_insobj * pars = (t_op_insobj*)get_space_op(cdp, 1+sizeof(t_op_insobj), OP_INSOBJ);
  if (pars==NULL) return TRUE;
  strmaxcpy(pars->objname, name, sizeof(pars->objname));
  pars->categ=category;
  pars->limited=FALSE;
  REGISTER_ANSWER(objnum, sizeof(tobjnum));
  return cond_send(cdp);
}

CFNC DllExport BOOL WINAPI cd_Insert_object_limited(cdp_t cdp, const char * name, tcateg category, tobjnum * objnum)
// Creates object with only "Author" role privils. No privils for the creator or other std. roles.
{ t_op_insobj * pars = (t_op_insobj*)get_space_op(cdp, 1+sizeof(t_op_insobj), OP_INSOBJ);
  if (pars==NULL) return TRUE;
  strmaxcpy(pars->objname, name, sizeof(pars->objname));
  pars->categ=category;
  pars->limited=TRUE;
  REGISTER_ANSWER(objnum, sizeof(tobjnum));
  return cond_send(cdp);
}

CFNC DllKernel BOOL WINAPI cd_Create_user(cdp_t cdp, const char * logname,
  const char * name1, const char * name2, const char * name3,
  const char * identif,  const WBUUID homesrv, const char * password, tobjnum * objnum)
// Old compatibility: password==NULL says password is an empty string and user must change it.
// When *password==1 then the password start at password+1 and user must change it.
{// check name duplicity
 // if (!cd_Find_object(cdp, logname, CATEG_USER, objnum))
 //   { SET_ERR_MSG(cdp, "USERTAB");  client_error(cdp, KEY_DUPLICITY);  return TRUE; }
  *objnum=NOOBJECT;
  bool must_change_pass;
  if (password==NULL) 
    must_change_pass=true;
  else if (*password==1)
    { must_change_pass=true;  password++; }
  else must_change_pass=false;
   
 // store user in the database:
  tptr p = get_space_op(cdp, 1+1+sizeof(t_op_create_user), OP_CNTRL);
  if (p==NULL) return TRUE;
  *p=SUBOP_CREATE_USER;
  t_op_create_user * uinfo = (t_op_create_user *)(p+1);
  strcpy(uinfo->logname, logname);  Upcase9(cdp, uinfo->logname);
  memset(&uinfo->info, 0, sizeof(t_user_ident));
  if (name1  !=NULL) strmaxcpy(uinfo->info.name1,   name1,   sizeof(uinfo->info.name1));
  if (name2  !=NULL) strmaxcpy(uinfo->info.name2,   name2,   sizeof(uinfo->info.name2));
  if (name3  !=NULL) strmaxcpy(uinfo->info.name3,   name3,   sizeof(uinfo->info.name3));
  if (identif!=NULL) strmaxcpy(uinfo->info.identif, identif, sizeof(uinfo->info.identif));
  memcpy(uinfo->home_srv, homesrv!=NULL ? homesrv : cdp->server_uuid, UUID_SIZE);
  create_uuid(uinfo->user_uuid);  diff_uuid(uinfo->user_uuid);
  unsigned cycles = *logname=='_' || must_change_pass ? 1 : PASSWORD_CYCLES;
  *(uns32*)uinfo->enc_password = cycles;
  if (get_password_digest(cdp, password ? password : NULLSTRING, cycles+1, uinfo->enc_password+sizeof(uns32)))
    { client_error(cdp, OUT_OF_KERNEL_MEMORY);  return TRUE; }
  REGISTER_ANSWER(objnum, sizeof(tobjnum));
  return cond_send(cdp);
}

CFNC DllKernel BOOL WINAPI cd_GetSet_privils(cdp_t cdp,
      tobjnum user_group_role, tcateg subject_categ, ttablenum table,
      trecnum record, t_oper operation, uns8 * privils)
#if 0
{ op_privils * p=(op_privils *)get_space_op(cdp, 1+sizeof(op_privils), OP_PRIVILS);
  if (p==NULL) return TRUE;
  p->user_group_role=user_group_role; // may be NOOBJECT, logged user used then
  p->subject_categ  =(uns8)subject_categ;
  p->table          =table;
  p->record         =record;
  p->operation      =(uns8)operation;
  memcpy(p->privils, privils, PRIVIL_DESCR_SIZE);
  if (operation!=OPER_SET)  // get, get eff
  { memset(privils, 0, PRIVIL_DESCR_SIZE);
    REGISTER_ANSWER(privils, PRIVIL_DESCR_SIZE);
  }
  return cond_send(cdp);
}

CFNC DllKernel BOOL WINAPI cd_GetSet_privils_ex(cdp_t cdp,
      tobjnum user_group_role, tcateg subject_categ, ttablenum table,
      trecnum record, t_oper operation, uns8 * privils)
#endif
{ unsigned extension = 0;
  const d_table * td = cd_get_table_d(cdp, table, CATEG_TABLE);  // may be open cursor
  if (td) 
  { unsigned privil_descr_size = SIZE_OF_PRIVIL_DESCR(td->attrcnt);
    extension = privil_descr_size>PRIVIL_DESCR_SIZE ? privil_descr_size-PRIVIL_DESCR_SIZE : 0;
    release_table_d(td);
  }
  op_privils * p=(op_privils *)get_space_op(cdp, 1+sizeof(op_privils)+(operation==OPER_SET ? extension : 0), OP_PRIVILS);
  if (p==NULL) return TRUE;
  p->user_group_role=user_group_role; // may be NOOBJECT, logged user used then
  p->subject_categ  =(uns8)subject_categ;
  p->table          =table;
  p->record         =record;
  p->operation      =(uns8)operation;
  if (operation==OPER_SET)
    memcpy(p->privils, privils, PRIVIL_DESCR_SIZE+extension);
  if (operation!=OPER_SET)  // get, get eff
    REGISTER_ANSWER(privils, PRIVIL_DESCR_SIZE+extension);
  return cond_send(cdp);
}

CFNC DllKernel BOOL WINAPI cd_GetSet_group_role(cdp_t cdp, tobjnum user_or_group,
  tobjnum group_or_role, tcateg subject2, t_oper operation, uns32 * relation)
// Read or writes relation between user-group, user-role or group_role.
// If subject2==CATEG_APPL, removes all roles of applicaton group_or_role
//  from all users & groups.
{ if (subject2==CATEG_GROUP)
  { if (group_or_role==EVERYBODY_GROUP && user_or_group!=ANONYMOUS_USER)
      if (operation==OPER_GET || operation==OPER_GETEFF)   // get
        { *relation=1;  return FALSE; }
      else if (*relation) return FALSE;   // inserting into EVERYBODY_GROUP group: no action
      else { client_error(cdp, NO_RIGHT);  return TRUE; }  // cannot remove from EVERYBODY group
  }
  op_grouprole * p=(op_grouprole *)get_space_op(cdp, 1+sizeof(op_grouprole), OP_GROUPROLE);
  if (p==NULL) return TRUE;
  p->user_or_group=user_or_group;
  p->group_or_role=group_or_role;
  p->subject2     =(uns8)subject2;
  p->operation    =(uns8)operation;
  p->relation     =(uns32)*relation;
  if (operation==OPER_GET || operation==OPER_GETEFF)  // get
    REGISTER_ANSWER(relation, sizeof(uns32));
  return cond_send(cdp);
}

CFNC DllKernel BOOL WINAPI cd_Am_I_db_admin(cdp_t cdp)
{ uns32 relation=0;
  cd_GetSet_group_role(cdp, cdp->logged_as_user, DB_ADMIN_GROUP, CATEG_GROUP, OPER_GET, &relation);
  return relation!=0;
}

CFNC DllKernel BOOL WINAPI cd_Am_I_config_admin(cdp_t cdp)
{ uns32 relation=0;
  cd_GetSet_group_role(cdp, cdp->logged_as_user, CONF_ADM_GROUP, CATEG_GROUP, OPER_GET, &relation);
  return relation!=0;
}
                                                  
CFNC DllKernel BOOL WINAPI cd_Am_I_security_admin(cdp_t cdp)
{ uns32 relation=0;
  cd_GetSet_group_role(cdp, cdp->logged_as_user, SECUR_ADM_GROUP, CATEG_GROUP, OPER_GET, &relation);
  return relation!=0;
}

CFNC DllKernel BOOL WINAPI cd_Am_I_appl_admin(cdp_t cdp)
{ tobjnum rolenum;  uns32 relation=0;
  if (cd_Find2_object(cdp, "ADMINISTRATOR", NULL, CATEG_ROLE, &rolenum)) return FALSE;
  cd_GetSet_group_role(cdp, cdp->logged_as_user, rolenum, CATEG_ROLE, OPER_GET, &relation);
  return relation!=0;
}

CFNC DllKernel BOOL WINAPI cd_Am_I_appl_author(cdp_t cdp)
{ tobjnum rolenum;  uns32 relation=0;
  if (cd_Find2_object(cdp, "AUTHOR", NULL, CATEG_ROLE, &rolenum)) return FALSE;
  cd_GetSet_group_role(cdp, cdp->logged_as_user, rolenum, CATEG_ROLE, OPER_GET, &relation);
  return relation!=0;
}

CFNC DllKernel BOOL WINAPI cd_GetSet_next_user(cdp_t cdp, tcurstab curs,
  trecnum position, tattrib attr, t_oper operation, t_valtype valtype, void * value)
#if WBVERS>=1000   // support terminated, not converting the [attr] parameter between protocols
{ client_error(cdp, ERROR_IN_FUNCTION_CALL);  return TRUE; }
#else
{ int valsize;
  if      (valtype==VT_OBJNUM) valsize=sizeof(tobjnum);
  else if (valtype==VT_NAME)   valsize=OBJ_NAME_LEN+1;
  else if (valtype==VT_UUID)   valsize=UUID_SIZE;
  else if (valtype==VT_NAME3 && operation==OPER_GET)
                               valsize=sizeof(t_user_ident)+5;
  else { client_error(cdp, ERROR_IN_FUNCTION_CALL);  return TRUE; }
  op_nextuser * p=(op_nextuser *)get_space_op(cdp, 1+sizeof(op_nextuser)+(operation==OPER_GET ? 0 : valsize), OP_NEXTUSER);
  if (p==NULL) return TRUE;
  p->curs=curs;
  p->position=position;
  p->attr=attr;
  p->operation=(uns8)operation;
  p->valtype  =(uns8)valtype;
  if (operation==OPER_GET)
    REGISTER_ANSWER(value, valsize)
  else memcpy(p+1, value, valsize);
  return cond_send(cdp);
}

CFNC DllKernel BOOL WINAPI GetSet_next_user(tcurstab curs,
  trecnum position, tattrib attr, t_oper operation, t_valtype valtype, void * value)
{ return cd_GetSet_next_user(GetCurrTaskPtr(), curs, position,
  attr, operation, valtype, value);
}

#endif

// old functions for compatibility:
CFNC DllKernel BOOL WINAPI Get_object_rights(const char * object, tcateg category, const char * username,
                                        tright * rights)
{ cdp_t cdp = GetCurrTaskPtr();
  tobjnum objnum, subjnum;  uns8 priv_val[PRIVIL_DESCR_SIZE];  tcateg subject_categ;
  if (!*username) { subjnum=NOOBJECT;  subject_categ=CATEG_USER; }
  else if (!cd_Find2_object(cdp, username, NULL, CATEG_USER, &subjnum)) subject_categ=CATEG_USER;
  else if (!cd_Find2_object(cdp, username, NULL, CATEG_GROUP,&subjnum)) subject_categ=CATEG_GROUP;
  else return TRUE;
  if (cd_Find2_object(cdp, object, NULL, category, &objnum)) return TRUE;
  if (cd_GetSet_privils(cdp, subjnum, subject_categ, CATEG2SYSTAB(category), objnum, OPER_GETEFF, priv_val))
    return TRUE;
  if (HAS_READ_PRIVIL( priv_val, OBJ_DEF_ATR)) *priv_val |= RIGHT_READ;
  else *priv_val &= ~RIGHT_READ;
  if (HAS_WRITE_PRIVIL(priv_val, OBJ_DEF_ATR)) *priv_val |= RIGHT_WRITE;
  else *priv_val &= ~RIGHT_WRITE;
  *rights=*priv_val;
  return FALSE;
}

CFNC DllKernel BOOL WINAPI Set_object_rights(const char * object, tcateg category, const char * username,
                                        tright rights)
{ cdp_t cdp = GetCurrTaskPtr();
  tobjnum objnum, subjnum;  uns8 priv_val[PRIVIL_DESCR_SIZE];  tcateg subject_categ;
  if (!*username) { subjnum=NOOBJECT;  subject_categ=CATEG_USER; }
  else if (!cd_Find2_object(cdp, username, NULL, CATEG_USER, &subjnum)) subject_categ=CATEG_USER;
  else if (!cd_Find2_object(cdp, username, NULL, CATEG_GROUP,&subjnum)) subject_categ=CATEG_GROUP;
  else return TRUE;
  if (cd_Find2_object(cdp, object, NULL, category, &objnum)) return TRUE;
  *priv_val=rights;
  if (rights & RIGHT_READ)
    if (rights & RIGHT_WRITE)
      memset(priv_val+1, 0xff, PRIVIL_DESCR_SIZE-1);
    else
      memset(priv_val+1, 0x55, PRIVIL_DESCR_SIZE-1);
  else
    if (rights & RIGHT_WRITE)
      memset(priv_val+1, 0xaa, PRIVIL_DESCR_SIZE-1);
    else
      memset(priv_val+1, 0,    PRIVIL_DESCR_SIZE-1);
  return cd_GetSet_privils(cdp, subjnum, subject_categ, CATEG2SYSTAB(category), objnum, OPER_SET, priv_val);
}

CFNC DllKernel BOOL WINAPI Get_data_rights(ttablenum table, const char * username,
                tright * rights, tdright * rd_ri, tdright * wr_ri)
{ cdp_t cdp = GetCurrTaskPtr();
  tobjnum subjnum;  uns8 priv_val[PRIVIL_DESCR_SIZE];  tcateg subject_categ;
  if (!*username) { subjnum=NOOBJECT;  subject_categ=CATEG_USER; }
  else if (!cd_Find2_object(cdp, username, NULL, CATEG_USER, &subjnum)) subject_categ=CATEG_USER;
  else if (!cd_Find2_object(cdp, username, NULL, CATEG_GROUP,&subjnum)) subject_categ=CATEG_GROUP;
  else return TRUE;
  if (cd_GetSet_privils(cdp, subjnum, subject_categ, table, NORECNUM, OPER_GETEFF, priv_val))
    return TRUE;
  *rights = *priv_val;  // RIGHT_READ & RIGHT_WRITE should be preserved
  *rd_ri=*wr_ri=0;
  for (int i=0;  i<16;  i++)
  { if (HAS_READ_PRIVIL( priv_val, i+1)) *rd_ri |= (1<<i);
    if (HAS_WRITE_PRIVIL(priv_val, i+1)) *wr_ri |= (1<<i);
  }
  return FALSE;
}

CFNC DllKernel BOOL WINAPI Set_data_rights(ttablenum table, const char * username,
                tright rights, tdright rd_ri, tdright wr_ri)
{ cdp_t cdp = GetCurrTaskPtr();
  tobjnum subjnum;  uns8 priv_val[PRIVIL_DESCR_SIZE];  tcateg subject_categ;
  if (!*username) { subjnum=NOOBJECT;  subject_categ=CATEG_USER; }
  else if (!cd_Find2_object(cdp, username, NULL, CATEG_USER, &subjnum)) subject_categ=CATEG_USER;
  else if (!cd_Find2_object(cdp, username, NULL, CATEG_GROUP,&subjnum)) subject_categ=CATEG_GROUP;
  else return TRUE;
  *priv_val=rights;   // RIGHT_READ & RIGHT_WRITE can be preserved
  if (rights & RIGHT_READ)
    if (rights & RIGHT_WRITE)
      memset(priv_val+1, 0xff, PRIVIL_DESCR_SIZE-1);
    else
      memset(priv_val+1, 0x55, PRIVIL_DESCR_SIZE-1);
  else
    if (rights & RIGHT_WRITE)
      memset(priv_val+1, 0xaa, PRIVIL_DESCR_SIZE-1);
    else
      memset(priv_val+1, 0,    PRIVIL_DESCR_SIZE-1);
  for (int i=0;  i<16;  i++)
  { if (rd_ri & (1<<i)) SET_READ_PRIVIL( priv_val, i+1);
    if (wr_ri & (1<<i)) SET_WRITE_PRIVIL(priv_val, i+1);
  }
  return cd_GetSet_privils(cdp, subjnum, subject_categ, table, NORECNUM, OPER_SET, priv_val);
}

// obsolete
CFNC DllKernel BOOL WINAPI User_to_group(tobjnum user, tobjnum group, BOOL state)
{ cdp_t cdp = GetCurrTaskPtr();  uns32 relation = state;
  return cd_GetSet_group_role(cdp, user, group, CATEG_GROUP, OPER_SET, &relation);
}

// obsolete
CFNC DllKernel BOOL WINAPI User_in_group(tobjnum user, tobjnum group, BOOL * state)
{ cdp_t cdp = GetCurrTaskPtr();  uns32 relation;
  if (cd_GetSet_group_role(cdp, user, group, CATEG_GROUP, OPER_GET, &relation))
    return TRUE;
  *state = relation!=0;
  return FALSE;
}
//////////////////////////////////////////////////////////////////////////////

CFNC DllKernel BOOL WINAPI cd_Get_valid_recs(cdp_t cdp, tcursnum cursnum, trecnum startrec,
    trecnum limit, void * map, trecnum * totrec, trecnum * valrec)
{ tptr p;  unsigned mapsize;  
  mapsize=(unsigned)(((limit-1)/BITSPERPART + 1) * (BITSPERPART/8) + 2);
  /* +2 due to 16-bit alignment (map need not start at the mapspace beginning) */
  p=get_space_op(cdp, 1+sizeof(tcursnum)+2*sizeof(trecnum), OP_VALRECS);
  if (p==NULL) return TRUE;
  *(tcursnum *)p=cursnum;   p+=sizeof(tcursnum);
  *(trecnum  *)p=startrec;  p+=sizeof(trecnum);
  *(trecnum  *)p=limit;
  cdp->ans_array[cdp->ans_ind]=totrec;
  cdp->ans_type [cdp->ans_ind]=sizeof(trecnum);
  cdp->ans_ind++;
  cdp->ans_array[cdp->ans_ind]=valrec;
  cdp->ans_type [cdp->ans_ind]=sizeof(trecnum);
  cdp->ans_ind++;
  cdp->ans_array[cdp->ans_ind]=map;
  cdp->ans_type [cdp->ans_ind]=mapsize;
  cdp->ans_ind++;
  return cond_send(cdp);
}

CFNC DllKernel BOOL WINAPI cd_Text_substring(cdp_t cdp, tcursnum cursor, trecnum position, tattrib attr, uns16 index,
              tptr pattern, uns16 pattlen, uns32 start, uns16 flags, uns32 * stringpos)
{ unsigned sizeof_tattrib = cdp->sizeof_tattrib();
  tptr p=get_space_op(cdp, 1+sizeof(tcursnum)+sizeof(trecnum)+sizeof_tattrib+
            (unsigned)strlen(pattern)+1+sizeof(uns32)+3*sizeof(uns16), OP_TEXTSUB);
  if (p==NULL) return TRUE;
  *(tcursnum*)p=cursor;    p+=sizeof(tcursnum);
  *(trecnum *)p=position;  p+=sizeof(trecnum);
  write_attrib(p, &attr, sizeof_tattrib);
  *(uns16   *)p=index;     p+=sizeof(uns16);
  *(uns32   *)p=start;     p+=sizeof(uns32);
  *(uns16   *)p=flags;     p+=sizeof(uns16);
  *(uns16   *)p=pattlen;   p+=sizeof(uns16);
  strcpy(p, pattern);
  cdp->ans_array[cdp->ans_ind]=stringpos;
  cdp->ans_type [cdp->ans_ind]=sizeof(uns32);
  cdp->ans_ind++;
  return cond_send(cdp);
}

CFNC DllKernel BOOL WINAPI cd_Add_record(cdp_t cdp, tcursnum curs, trecnum * recs, int numofrecs)
{ if (IS_ODBC_TABLE(curs)) { client_error(cdp, CANNOT_FOR_ODBC);  return TRUE; }
  tptr p=get_space_op(cdp, 1+sizeof(tcursnum)+sizeof(uns16)+(uns8)numofrecs*sizeof(trecnum), OP_ADDREC);
  if (p==NULL) return TRUE;
  *(tcursnum*)p=curs;                    p+=sizeof(tcursnum);
  *(uns16   *)p=(uns16)(uns8)numofrecs;  p+=sizeof(uns16);
  memcpy(p, recs, (uns8)numofrecs*sizeof(trecnum));
  return cond_send(cdp);
}

CFNC DllKernel BOOL WINAPI cd_Super_recnum(cdp_t cdp, tcursnum subcursor, tcursnum supercursor,
              trecnum subrecnum, trecnum * superrecnum)
{ if (IS_ODBC_TABLE(subcursor))   { client_error(cdp, CANNOT_FOR_ODBC);  return TRUE; }
  if (IS_ODBC_TABLE(supercursor)) { client_error(cdp, CANNOT_FOR_ODBC);  return TRUE; }
  tptr p=get_space_op(cdp, 1+2*sizeof(tcursnum)+sizeof(trecnum), OP_SUPERNUM);
  if (p==NULL) return TRUE;
  *(tcursnum*)p=subcursor;  p+=sizeof(tcursnum);
  *(trecnum *)p=subrecnum;  p+=sizeof(trecnum);
  *(tcursnum*)p=supercursor;
  cdp->ans_array[cdp->ans_ind]=superrecnum;
  cdp->ans_type [cdp->ans_ind]=sizeof(trecnum);
  cdp->ans_ind++;
  return cond_send(cdp);
}

CFNC DllKernel BOOL WINAPI cd_Relist_objects(cdp_t cdp)
{ return cd_Relist_objects_ex(cdp, TRUE); }

CFNC DllKernel BOOL WINAPI cd_Relist_objects_ex(cdp_t cdp, BOOL extended)
/* Reloads object names from TABTAB/OBJTAB. Does not read nor
   modify application description. Does not affect ODBC linked tables. */
{ 
  if (!caching_apl_objects) return FALSE;
#ifdef WINS
  HCURSOR hOldCurs = SetCursor(LoadCursor(NULL, IDC_WAIT));
#endif
  cdp->odbc.apl_tables   .comp_dynar::~comp_dynar();
  cdp->odbc.apl_views    .comp_dynar::~comp_dynar();
  cdp->odbc.apl_menus    .comp_dynar::~comp_dynar();
  cdp->odbc.apl_queries  .comp_dynar::~comp_dynar();
  cdp->odbc.apl_programs .comp_dynar::~comp_dynar();
  cdp->odbc.apl_pictures .comp_dynar::~comp_dynar();
  cdp->odbc.apl_drawings .comp_dynar::~comp_dynar();
  cdp->odbc.apl_replrels .comp_dynar::~comp_dynar();
  cdp->odbc.apl_wwws     .comp_dynar::~comp_dynar();
  cdp->odbc.apl_relations.relation_dynar::~relation_dynar();
  cdp->odbc.apl_procs    .comp_dynar::~comp_dynar();
  cdp->odbc.apl_triggers .comp_dynar::~comp_dynar();
  cdp->odbc.apl_seqs     .comp_dynar::~comp_dynar();
  cdp->odbc.apl_folders  .folder_dynar::~folder_dynar();
  cdp->odbc.apl_domains  .comp_dynar::~comp_dynar();
  if (!*cdp->sel_appl_name)
  {
#ifdef CLIENT_ODBC
    free_odbc_tables(cdp);
#endif
  }
  else
	{// create root folder:
    t_folder * fld = cdp->odbc.apl_folders.next();
    if (fld)
      { strcpy(fld->name, ".");  fld->objnum=0;  fld->flags=0; }
#ifdef CLIENT_GUI
    Set_current_folder(cdp, 0);  // set root as the current folder
#endif    
    if (!cdp->object_cache_disabled)
    { scan_objects(cdp, CATEG_CONNECTION, FALSE);  // must be before tables, because contains connection strings
      scan_objects(cdp, CATEG_TABLE,    extended);
      scan_objects(cdp, CATEG_MENU,     extended);
      scan_objects(cdp, CATEG_CURSOR,   extended);
      scan_objects(cdp, CATEG_VIEW,     extended);
      scan_objects(cdp, CATEG_PGMSRC,   extended);
      scan_objects(cdp, CATEG_PICT,     FALSE);
      scan_objects(cdp, CATEG_DRAWING,  extended);
      scan_objects(cdp, CATEG_REPLREL,  extended);
      scan_objects(cdp, CATEG_WWW,      extended);
      scan_objects(cdp, CATEG_RELATION, extended);
      scan_objects(cdp, CATEG_PROC,     extended);
      scan_objects(cdp, CATEG_TRIGGER,  extended);
      scan_objects(cdp, CATEG_SEQ,      extended);
      scan_objects(cdp, CATEG_FOLDER,   extended);
      scan_objects(cdp, CATEG_DOMAIN,   extended);
#ifdef WINS
#if WBVERS<900
      SendMessage(GetParent(cdp->RV.hClient), SZM_RELISTOBJ, 0, 0);
#endif
#endif
    }
  }
#ifdef WINS
  SetCursor(hOldCurs);
#endif
  return FALSE;
}

CFNC DllKernel BOOL WINAPI cd_Set_application(cdp_t cdp, const char * applname)
{ return cd_Set_application_ex(cdp, applname, FALSE); }

CFNC DllKernel BOOL WINAPI cd_Set_application_ex(cdp_t cdp, const char * applname, BOOL extended)
{ if (extended == 2)
  { cdp->object_cache_disabled = true;
    extended=FALSE;
  }
  else cdp->object_cache_disabled = !caching_apl_objects;
 // send the schema name to the server:
  tptr p=get_space_op(cdp, 1+sizeof(tobjname), OP_SETAPL);
  if (p==NULL) return TRUE;
  strmaxcpy(p, applname, sizeof(tobjname));
  Upcase9(cdp, p);
  REGISTER_ANSWER(cdp->sel_appl_uuid, UUID_SIZE);        // NULL UUID for NULL application
  REGISTER_ANSWER(&cdp->author_role,  sizeof(tobjnum));
  REGISTER_ANSWER(&cdp->admin_role,   sizeof(tobjnum));
  REGISTER_ANSWER(&cdp->senior_role,  sizeof(tobjnum));
  REGISTER_ANSWER(&cdp->junior_role,  sizeof(tobjnum));
  REGISTER_ANSWER(&cdp->applobj,      sizeof(tobjnum));  // NOOBJECT for NULL application
  if (cond_send(cdp)) return TRUE;
 // server side actions OK, continue on client side:
#ifdef CLIENT_GUI
  RemDBContainer();
#endif
 // store the name in the client:
  strmaxcpy(cdp->sel_appl_name, applname, sizeof(tobjname));
  Upcase9(cdp, cdp->sel_appl_name);
 // set application data valid for NULL application and for applications without an APX:
  memcpy(cdp->front_end_uuid, cdp->sel_appl_uuid, UUID_SIZE);  // NULL UUID for NULL application
  memcpy(cdp->back_end_uuid,  cdp->sel_appl_uuid, UUID_SIZE);  // NULL UUID for NULL application
  cdp->locked_appl=wbfalse;
  *cdp->appl_starter=0;
#ifdef CLIENT_PGM
  int lang=-1;
  Select_language(cdp, -1);
#endif
 // read data from the APX if the APX exists:
  { apx_header apx;  
    memset(&apx, 0, sizeof(apx));
    if (cdp->applobj!=NOOBJECT && !cd_Read_var(cdp, OBJ_TABLENUM, (trecnum)cdp->applobj, OBJ_DEF_ATR, NOINDEX, 0, sizeof(apx_header), &apx, NULL))
    { if (apx.version>=6)  // link front-back end, find out if locked:
      { tobjnum objnum;  
        if (*apx.front_end_name || *apx.back_end_name) // if back_end_name specified then back_end_uuid returned as sel_appl_uuid
        { if (!cd_Find2_object(cdp, *apx.front_end_name ? apx.front_end_name : applname, NULL, CATEG_APPL, &objnum))
            cd_Read(cdp, OBJ_TABLENUM, objnum, APPL_ID_ATR, NULL, cdp->front_end_uuid);
          if (!cd_Find2_object(cdp, applname, NULL, CATEG_APPL, &objnum))
            cd_Read(cdp, OBJ_TABLENUM, objnum, APPL_ID_ATR, NULL, cdp->sel_appl_uuid);
        }
        if (*apx.back_end_name  && !cd_Find2_object(cdp, apx.back_end_name, NULL, CATEG_APPL, &objnum))
          cd_Read(cdp, OBJ_TABLENUM, objnum, APPL_ID_ATR, NULL, cdp->back_end_uuid);
        cdp->locked_appl=apx.appl_locked;
#ifdef CLIENT_PGM
        lang=apx.sel_language;
#endif
      } // verion >= 6
      strmaxcpy(cdp->appl_starter, apx.start_name, sizeof(cdp->appl_starter));
      cdp->appl_starter_categ = apx.start_categ;
    } // APX read
  }
 // release prepared statements:
  drop_all_statements(cdp);  // VisualBasic creates multiple connections, must preserve prepared statements
 // clear and reload all application-related info on the client side:
#ifdef CLIENT_ODBC
  free_odbc_tables(cdp);
  cdp->odbc.mapping_on=FALSE;
#endif
  cd_Relist_objects_ex(cdp, extended); // must be called after setting front and back end link
#ifdef CLIENT_PGM
  if (cdp->applobj!=NOOBJECT) Select_language(cdp, lang); // setting the default language
#endif
  return FALSE;
}

CFNC DllKernel BOOL WINAPI cd_Apl_name2id(cdp_t cdp, const char * name, uns8 * appl_id)
{ memset(appl_id, 0, UUID_SIZE);
  if (!*name) return FALSE;
  tptr p=get_space_op(cdp, 1+1+OBJ_NAME_LEN+1, OP_IDCONV);
  if (p==NULL) return TRUE;
  *(p++)=0;
  strmaxcpy(p, name, OBJ_NAME_LEN+1);  Upcase9(cdp, p);
  cdp->ans_array[cdp->ans_ind]=appl_id;
  cdp->ans_type [cdp->ans_ind]=UUID_SIZE;
  cdp->ans_ind++;
  return cond_send(cdp);
}

CFNC DllKernel BOOL WINAPI cd_Apl_id2name(cdp_t cdp, const uns8 * appl_id, char * name)
{ tptr p=get_space_op(cdp, 1+1+UUID_SIZE, OP_IDCONV);
  if (p==NULL) return TRUE;
  *(p++)=1;
  memcpy(p, appl_id, UUID_SIZE);
  cdp->ans_array[cdp->ans_ind]=name;
  cdp->ans_type [cdp->ans_ind]=OBJ_NAME_LEN+1;
  cdp->ans_ind++;
  *name=0;
  return cond_send(cdp);
}

CFNC DllKernel BOOL WINAPI cd_Bck_replay(cdp_t cdp, tobjnum bad_user, timestamp dttm, uns32 * stat)
{ tptr p=get_space_op(cdp, 1+sizeof(tobjnum)+sizeof(timestamp), OP_REPLAY);
  if (p==NULL) return TRUE;
  *(tobjnum  *)p=bad_user;  p+=sizeof(tobjnum);
  *(timestamp*)p=dttm;
  cdp->ans_array[cdp->ans_ind]=stat;
  cdp->ans_type [cdp->ans_ind]=4*sizeof(uns32);
  cdp->ans_ind++;
  return cond_send(cdp);
}

CFNC DllKernel BOOL WINAPI Bck_get_time(timestamp * dttm)
{ tptr p;  cdp_t cdp = GetCurrTaskPtr();
  p=get_space_op(cdp, 1, OP_GETDBTIME);
  if (p==NULL) return TRUE;
  cdp->ans_array[cdp->ans_ind]=dttm;
  cdp->ans_type [cdp->ans_ind]=sizeof(timestamp);
  cdp->ans_ind++;
  return cond_send(cdp);
}

CFNC DllKernel BOOL WINAPI cd_Write_len(cdp_t cdp, tcursnum cursnum, trecnum position, tattrib attr, t_mult_size index, t_varcol_size size)
{ modifrec md[2];
  if (index==NOINDEX) md[0].modtype=MODLEN;
  else
  { md[0].modtype=MODIND;  md[0].moddef.modind.index=index;
    md[1].modtype=MODLEN;
  }
  BOOL res=cd_Write(cdp, cursnum, position, attr, md, (tptr)&size, HPCNTRSZ);
#ifdef CLIENT_GUI
  if (cursnum==OBJ_TABLENUM && cdp->IStorageCache && attr!=DEL_ATTR_NUM && attr!=SYSTAB_PRIVILS_ATR && !res)
    write_pcache(cdp, position, attr, &size, 0, 0, RW_OPER_LEN);
#endif
  return res;
}

CFNC DllKernel BOOL WINAPI cd_Read_len(cdp_t cdp, tcursnum cursnum, trecnum position, tattrib attr, t_mult_size index, t_varcol_size * size)
{ modifrec md[2];
#ifdef CLIENT_GUI
  if (cursnum==OBJ_TABLENUM && cdp->IStorageCache && attr!=DEL_ATTR_NUM && attr!=SYSTAB_PRIVILS_ATR && !cdp->in_package)
    return !read_pcached(cdp, position, attr, size, 0, 0, NULL, RW_OPER_LEN);
#endif
  if (index==NOINDEX) md[0].modtype=MODLEN;
  else
  { md[0].modtype=MODIND;  md[0].moddef.modind.index=index;
    md[1].modtype=MODLEN;
  }
  return cd_Read(cdp, cursnum, position, attr, md, (tptr)size);
}

CFNC DllKernel BOOL WINAPI cd_Write_ind(cdp_t cdp, tcursnum cursnum, trecnum position, tattrib attr, t_mult_size index, void * data, uns32 datasize)
{ if (index==NOINDEX)
    return cd_Write(cdp, cursnum, position, attr, NULL, data, datasize);
  else
  { modifrec myacc[2] = { {MODIND, {0}}, {MODSTOP, {0}} };
    myacc[0].moddef.modind.index=index;
    return cd_Write(cdp, cursnum, position, attr, myacc, data, datasize);
  }
}

CFNC DllKernel BOOL WINAPI cd_Read_ind(cdp_t cdp, tcursnum cursnum, trecnum position, tattrib attr, t_mult_size index, void * data)
{ if (index==NOINDEX)
    return cd_Read(cdp, cursnum, position, attr, NULL, data);
  else
  { modifrec myacc[2] = { {MODIND, {0}}, {MODSTOP, {0}} };
    myacc[0].moddef.modind.index=index;
    return cd_Read(cdp, cursnum, position, attr, myacc, data);
  }
}

CFNC DllKernel BOOL WINAPI cd_Write_ind_cnt(cdp_t cdp, tcursnum cursnum, trecnum position, tattrib attr, t_mult_size count)
{ modifrec myacc[2] = { {MODLEN, {0}}, {MODSTOP, {0}} };
  return cd_Write(cdp, cursnum, position, attr, myacc, &count, sizeof(uns16));
}

CFNC DllKernel BOOL WINAPI cd_Read_ind_cnt(cdp_t cdp, tcursnum cursnum, trecnum position, tattrib attr, t_mult_size * count)            
{ modifrec myacc[2] = { {MODLEN, {0}}, {MODSTOP, {0}} };
//  uns32 cnt;
//  BOOL res=cd_Read(cdp, cursnum, position, attr, myacc, &cnt);
//  *count=(uns16)cnt;
//  return res;
// I do not understand why a separate variable was used, but it conflicts with fast_cache_load!
  return cd_Read(cdp, cursnum, position, attr, myacc, count);
}

CFNC DllKernel sig32 WINAPI WinBase602_version(void)
{ return MAKELONG(WB_VERSION_MINOR,WB_VERSION_MAJOR); }
CFNC DllKernel sig32 WINAPI cd_WinBase602_version(cdp_t cdp)
{ return MAKELONG(WB_VERSION_MINOR,WB_VERSION_MAJOR); }

/////////////////////////////// dynamic params /////////////////////////////
CFNC DllKernel BOOL WINAPI cd_Pull_params(cdp_t cdp, uns32 handle, sig16 code, void ** answer)
{ *answer=NULL;
 // almost copy of cd_Send_params:
  tptr p=get_space_op(cdp, 1+sizeof(uns32)+sizeof(sig16), OP_SENDPAR);
  if (p==NULL) return TRUE;
  *(uns32*)p=handle;  p+=sizeof(uns32);
  *(sig16*)p=code;
  REGISTER_ANSWER(answer, ANS_SIZE_DYNALLOC);
  return cond_send(cdp);
}

CFNC DllKernel BOOL WINAPI cd_Pull_parameter(cdp_t cdp, uns32 handle, int parnum, int start, int length, char * buf)
{ tptr p=get_space_op(cdp, 1+sizeof(uns32)+2*sizeof(sig16)+2*sizeof(sig32), OP_SENDPAR);
  if (p==NULL) return TRUE;
  *(uns32*)p=handle;           p+=sizeof(uns32);
  *(sig16*)p=SEND_PAR_PULL_1;  p+=sizeof(sig16);
  *(uns16*)p=parnum;           p+=sizeof(uns16);
  *(sig32*)p=start;            p+=sizeof(sig32);
  *(sig32*)p=length;           p+=sizeof(sig32);
  REGISTER_ANSWER(buf, length);
  return cond_send(cdp);
}

static BOOL pull_clivars(cdp_t cdp, uns32 handle, t_clivar * clivars, int clivar_count)
// First, values of all vars are pulled, but from the long values max. SEND_PAR_COMMON_LIMIT bytes.
// Then, the rest of long values is pulled per partes.
// If clivar->buf is NULL for a long value, it is allocated. When the long value exceeds SEND_PAR_COMMON_LIMIT bytes,
//  the allocation is postponed to the per-partes phase (otherwise the ownership of the buffer would be unclear the per partes phase).
{ tptr buffer;
  BOOL res=cd_Pull_params(cdp, handle, SEND_EMB_PULL, (void**)&buffer);
  if (!res)
  { for (int i=0, pos=0;  i<clivar_count;  i++)
    { t_clivar * clivar = clivars+i;
      if (clivar->mode==MODE_OUT || clivar->mode==MODE_INOUT)
        if (IS_HEAP_TYPE(clivar->wbtype))
        { clivar->actlen = *(uns32*)(buffer+pos);  pos+=sizeof(uns32);
          if (!clivar->buf && clivar->actlen<SEND_PAR_COMMON_LIMIT) // allocating now and copying the full value
          { clivar->buf=corealloc(clivar->actlen, 55);
            if (!clivar->buf) client_error(cdp, OUT_OF_MEMORY);
            else clivar->buflen=(int)clivar->actlen;
          }
          if (clivar->buf)
            memcpy(clivar->buf, buffer+pos, clivar->buflen < clivar->actlen ? clivar->buflen : clivar->actlen);
          pos+=(int)clivar->actlen;
        }
        else
        { memcpy(clivar->buf, buffer+pos, clivar->buflen); 
          pos+=clivar->buflen;
        }
    }
    corefree(buffer);
  }
  if (res) return res;
 // pull long values (1st short part has been already moved):
  { for (int i=0, pos=0;  i<clivar_count;  i++)
    { t_clivar * clivar = clivars+i;
      if (clivar->mode==MODE_OUT || clivar->mode==MODE_INOUT)
        if (IS_HEAP_TYPE(clivar->wbtype) && (clivar->actlen==SEND_PAR_COMMON_LIMIT || clivar->buf==NULL))
        { bool alloc_buf = clivar->buf==NULL;
          if (alloc_buf) clivar->actlen=clivar->buflen=0;
          unsigned offset = (unsigned)clivar->actlen, partsize;
          do
          { tptr p=get_space_op(cdp, 1+sizeof(uns32)+sizeof(sig16)+cdp->sizeof_tname()+sizeof(uns32), OP_SENDPAR);
            if (p==NULL) return TRUE;
            *(uns32*)p=handle;  p+=sizeof(uns32);
            *(sig16*)p=SEND_EMB_PULL_EXT;  p+=sizeof(sig16);
            strmaxcpy(p, clivar->name, cdp->sizeof_tname());  p+=cdp->sizeof_tname();
            *(uns32*)p=offset;  p+=sizeof(uns32);
            tptr answer;
            REGISTER_ANSWER(&answer, ANS_SIZE_DYNALLOC);
            if (cond_send(cdp)) return TRUE;
            partsize=*(uns32*)answer;  
            if (alloc_buf) 
            { if (clivar->buflen+partsize)
              { clivar->buf=corerealloc(clivar->buf, clivar->buflen+partsize);
                if (clivar->buf)
                { memcpy((tptr)clivar->buf+offset, answer+sizeof(uns32), partsize);
                  clivar->buflen+=partsize;
                }
                else { client_error(cdp, OUT_OF_MEMORY);  return TRUE; }
              }
            }
            else if (clivar->buflen > offset)
              memcpy((tptr)clivar->buf+offset, answer+sizeof(uns32), clivar->buflen-offset < partsize ? clivar->buflen-offset : partsize);
            corefree(answer);
            offset+=partsize;  clivar->actlen=offset;
          } while (false);  //(partsize == SEND_PAR_PART_SIZE);  -- not dividing any more
        }
    }
  }
  return res;
}

static BOOL sql_exec_direct(cdp_t cdp, const char * statement, uns32 * results, t_clivar * clivars, int clivar_count)
// Executes the SQL statement with handle 0
{
//#ifdef CLIENT_PGM   - emb vars used in fulltext.dll
  if (clivar_count)
    if (send_params(cdp, 0, clivars, clivar_count)) return TRUE;
//#endif
  tobjnum tabobj;
 /* check for DROP TABLE: */
  BOOL del_tab=FALSE;  tobjname name;
  const char * pp=statement;  while (*pp==' ') pp++;
  if (!strnicmp(pp, "DROP", 4))
  { pp+=4;  while (*pp==' ') pp++;
    if (!strnicmp(pp, "TABLE", 5))
    { pp+=5;  while (*pp==' ') pp++;
      int i=0;
      if (*pp=='`')
      { pp++;
        while (*pp && *pp!='`' && i<OBJ_NAME_LEN) name[i++]=*(pp++);
      }
      else
#if WBVERS<900
        while (i<OBJ_NAME_LEN && SYMCHAR(*pp)) name[i++]=*(pp++);
      name[i]=0;
      if (!SYMCHAR(*pp))
#else
        while (i<OBJ_NAME_LEN && is_AlphanumA(*pp, cdp->sys_charset)) name[i++]=*(pp++);
      name[i]=0;
      if (!is_AlphanumA(*pp, cdp->sys_charset))
#endif
        if (!cd_Find_object(cdp, name, CATEG_TABLE, &tabobj))
          del_tab=TRUE;
    }
  }
 /* execute: */
#ifdef CLIENT_ODBC
  if (cdp->odbc.mapping_on)
  { t_connection * conn = cdp->odbc.conns;
    while (conn!=NULL && conn->selected != 2) conn=conn->next_conn;
    if (conn!=NULL)
    { if (SQLExecDirect(conn->hStmt, (UCHAR*)statement, SQL_NTS) != SQL_ERROR)
        return FALSE;
      SDWORD native;  char sqlstate[SQL_SQLSTATE_SIZE+1];
      SQLError(SQL_NULL_HENV, SQL_NULL_HDBC, conn->hStmt, (UCHAR*)sqlstate, &native,
        NULL, 0, NULL);
      client_error(cdp, (int)native);   /* propagate native error */
    }
    return TRUE;
  }
#endif
  static uns32 glob_psize; 
  uns32 loc_psize;
  t_varcol_size * p_psize = cdp->in_package ? &glob_psize : &loc_psize;
  static uns32 default_result[MAX_SQL_RESULTS+2];  // must be static due to packages
  tptr p=get_space_op(cdp, 1+sizeof(uns32)+(unsigned)strlen(statement)+1, OP_SUBMIT);
  if (p==NULL) return TRUE;
  *(uns32*)p=0;  p+=sizeof(uns32);  // handle is always 0, not reading the handle from *results any more!
  strcpy(p, statement);
  cdp->ans_array[cdp->ans_ind]=p_psize;
  cdp->ans_type [cdp->ans_ind]=sizeof(uns32);
  cdp->ans_ind++;
  cdp->ans_array[cdp->ans_ind]=results==NULL ? default_result : results;
  cdp->ans_type [cdp->ans_ind]=ANS_SIZE_PREVITEM;
  cdp->ans_ind++;
  if (cond_send(cdp)) return TRUE;
  if (del_tab)
  { 
#ifdef CLIENT_GUI
    Delete_component(cdp, CATEG_TABLE, name, tabobj);
#endif
    inval_table_d(cdp, tabobj,   CATEG_TABLE);
    inval_table_d(cdp, NOOBJECT, CATEG_CURSOR); // cursors may depend on it
  }
  if (clivar_count)
    if (pull_clivars(cdp, 0, clivars, clivar_count)) return TRUE;
  return FALSE;
}

CFNC DllKernel BOOL WINAPI cd_Open_cursor(cdp_t cdp, tobjnum cursor, tcursnum * curnum)
{ 
#ifdef CLIENT_ODBC
  if (cdp->odbc.mapping_on)
  { tptr def=cd_Load_objdef(cdp, cursor, CATEG_CURSOR);
    if (def==NULL) return TRUE;  /* client_error called inside */
    tptr pp,p=def;
    while (*p==' ') p++;
    if (*p=='{') while (*(p++)!='}') ;
    while (*p=='\r' || *p=='\n') p++;
    pp=p;
    while (*pp)
    { if (*pp=='\r' || *pp=='\n') *pp=' ';
      pp++;
    }
    t_connection * conn = cdp->odbc.conns;
    while (conn!=NULL && conn->selected != 2) conn=conn->next_conn;
    if (conn==NULL) { corefree(def);  return TRUE; }
    BOOL res=cd_ODBC_open_cursor(cdp, (uns32)conn, curnum, p);
    corefree(def);
    return res;
  }
  else
#endif
  { *curnum=NOOBJECT;  
    t_clivar_dynar clivars;
   // must look for embedded variables:
#ifdef CLIENT_PGM
    if (!IS_CURSOR_NUM(cursor)) /* unless restoring cursor */
    { tptr buf = cd_Load_objdef(cdp, cursor, CATEG_CURSOR);
      if (buf==NULL) return TRUE; 
     // parse for variables:
      parse_statement(cdp, buf, &clivars);
      corefree(buf);
      if (clivars.count())
        if (send_params(cdp, 0, clivars.acc0(0), clivars.count())) return TRUE;
    }
#endif
   // ignoring possible embedded variables:
    tptr p=get_space_op(cdp, 1+sizeof(tobjnum), OP_OPEN_CURSOR);
    if (p==NULL) return TRUE;
    *(tobjnum*)p=cursor;
    REGISTER_ANSWER(curnum, sizeof(tcursnum));
    if (cond_send(cdp)) return TRUE; 
    if (IS_CURSOR_NUM(cursor)) /* restoring cursor, original cursor has been closed */
      inval_table_d(cdp, cursor, CATEG_DIRCUR);  /* !! */
#ifdef CLIENT_PGM
    else
      if (clivars.count())
        if (pull_clivars(cdp, 0, clivars.acc0(0), clivars.count())) return TRUE;
#endif
    return FALSE;
  }
}

CFNC DllKernel BOOL WINAPI cd_SQL_execute(cdp_t cdp, const char * statement, uns32 * results)
// Executes the SQL statement with handle 0
{// parse for variables:
  t_clivar_dynar clivars;
#ifdef CLIENT_PGM
  parse_statement(cdp, statement, &clivars);
#endif
  return clivars.count() ? 
    sql_exec_direct(cdp, statement, results, clivars.acc0(0), clivars.count()) :
    sql_exec_direct(cdp, statement, results, NULL, 0);
}

CFNC DllKernel BOOL WINAPI cd_SQL_host_execute(cdp_t cdp, const char * statement, uns32 * results, struct t_clivar * hostvars, unsigned hostvars_count)
// Executes the SQL statement with handle 0
{ return sql_exec_direct(cdp, statement, results, (t_clivar *)hostvars, hostvars_count); }

#define RETVAL_VAR_NAME " \5 "

CFNC DllKernel BOOL WINAPI cd_SQL_value(cdp_t cdp, const char * sql_expr, char * outbuf, int out_bufsize)
{ t_clivar hostvar;
  if (out_bufsize<2)
    { client_error(cdp, ERROR_IN_FUNCTION_CALL);  return TRUE; }
 // prepare paremeter:
  hostvar.actlen=0;
  hostvar.buf=outbuf;
  hostvar.buflen=out_bufsize;
  hostvar.mode=MODE_OUT;
  strcpy(hostvar.name, RETVAL_VAR_NAME); 
  hostvar.wbtype=ATT_STRING;
  hostvar.specif=t_specif(out_bufsize-1, 0, FALSE, FALSE).opqval;
 // prepare stement:
  char * stmt = (char*)corealloc(strlen(sql_expr)+40, 74);
  if (!stmt)
    { client_error(cdp, OUT_OF_MEMORY);  return TRUE; }
  sprintf(stmt, "SET :>`%s`=CAST(%s AS char(%d))", RETVAL_VAR_NAME, sql_expr, out_bufsize-1);
  uns32 result;
  BOOL res = sql_exec_direct(cdp, stmt, &result, &hostvar, 1);
  corefree(stmt);
  return res;
}
////////////////////////////// prepared statements //////////////////////////
// Prepared statement handle is dynar index+1.

t_prep_stmt * find_prep_stmt(cdp_t cdp, uns32 handle)
// Returns prepared statement info for a given handle or NULL iff the handle is invalid
{ if (handle && (int)handle-1 < cdp->prep_stmts.count() &&
      cdp->prep_stmts.acc0(handle-1)->is_prepared)
    return cdp->prep_stmts.acc0(handle-1);
  return NULL;
}

BOOL drop_prep_stmt(cdp_t cdp, uns32 handle)
// Drops prepared statement info on the client side for a given handle 
{ t_prep_stmt * prep = find_prep_stmt(cdp, handle);
  if (prep==NULL) return TRUE;
  prep->clivars.t_clivar_dynar::~t_clivar_dynar();
  prep->is_prepared=FALSE;  // mark the cell as "unused"
  return FALSE;
}

t_prep_stmt * alloc_prep_stmt(cdp_t cdp, uns32 * handle)
// Allocates a new prepared statement on the client side
{ t_prep_stmt * prep;  int i;
  for (i=0;  i<cdp->prep_stmts.count();  i++)
    if (!cdp->prep_stmts.acc0(i)->is_prepared) break;
  if (i<cdp->prep_stmts.count()) prep=cdp->prep_stmts.acc0(i);
  else
  { prep=cdp->prep_stmts.next();
    if (prep==NULL) { *handle=0;  return NULL; }
    prep->clivars.init();
  }
  prep->is_prepared=TRUE; // mark the cell as "used"
  *handle=i+1;
  return prep;
}

void drop_all_statements(cdp_t cdp)  
// Drops all prepared statements on the client side
{ int i=0;
  while (i<cdp->prep_stmts.count())
  { t_prep_stmt * prep = cdp->prep_stmts.acc0(i);
    prep->clivars.t_clivar_dynar::~t_clivar_dynar();
    i++;
  }
  cdp->prep_stmts.t_prep_dynar::~t_prep_dynar();
}

static BOOL send_statement(cdp_t cdp, t_prep_stmt * prep, const char * statement, uns32 * handle)
{ if (prep->hostvar_count)   
    if (send_params(cdp, *handle, prep->hostvars, prep->hostvar_count)) goto exit;

  { int len = (int)strlen(statement);
    tptr p = get_space_op(cdp, 1+sizeof(uns32)+sizeof(sig16)+len+1, OP_SENDPAR);
    if (p==NULL) goto exit;
    *(uns32*)p=*handle;           p+=sizeof(uns32);
    *(sig16*)p=SEND_PAR_PREPARE;  p+=sizeof(sig16);
    memcpy(p, statement, len+1);
    if (!cond_send(cdp)) return FALSE;
  }
 exit:  // drop the handle on error:
  drop_prep_stmt(cdp, *handle);
  *handle=0;
  return TRUE;
}

CFNC DllKernel BOOL WINAPI cd_SQL_prepare(cdp_t cdp, const char * statement, uns32 * handle)
// Prepares a new statement and alocates (and returns) a new handle for it
// Sends the values of host varinables found in the statement
{ t_prep_stmt * prep = alloc_prep_stmt(cdp, handle);
  if (prep==NULL) { client_error(cdp, OUT_OF_MEMORY);  return TRUE; }
 // parse:
  prep->hostvar_count=0;
  prep->hostvars = NULL;  // not necessary
#ifdef CLIENT_PGM
  parse_statement(cdp, statement, &prep->clivars);
  if (prep->clivars.count())
  { prep->hostvar_count = prep->clivars.count();
    prep->hostvars = prep->clivars.acc0(0);
  }
#endif
 // send to prepare:
  return send_statement(cdp, prep, statement, handle);
}

CFNC DllKernel BOOL WINAPI cd_SQL_host_prepare(cdp_t cdp, const char * statement, uns32 * handle, struct t_clivar * hostvars, unsigned hostvars_count)
// Prepares a new statement and alocates (and returns) a new handle for it
// Sends the values of specified host varinables
{ t_prep_stmt * prep = alloc_prep_stmt(cdp, handle);
  if (prep==NULL) { client_error(cdp, OUT_OF_MEMORY);  return TRUE; }
  prep->hostvars=(t_clivar*)hostvars;
  prep->hostvar_count = hostvars_count;
 // send the statement to prepare it:
 // send to prepare:
  return send_statement(cdp, prep, statement, handle);
}

CFNC DllKernel BOOL WINAPI cd_SQL_exec_prepared(cdp_t cdp, uns32 handle, uns32 * results, int * count)
// Join into a single packet!
{ t_prep_stmt * prep = find_prep_stmt(cdp, handle);
  if (prep==NULL)
    { client_error(cdp, ERROR_IN_FUNCTION_CALL);  return TRUE; }
  if (prep->hostvar_count)
    if (send_params(cdp, handle, prep->hostvars, prep->hostvar_count)) return TRUE;

  static uns32 glob_psize; 
  uns32 loc_psize;
  t_varcol_size * p_psize = cdp->in_package ? &glob_psize : &loc_psize;
  static uns32 default_results[3];  // must be static due to packages
  tptr p=get_space_op(cdp, 1+sizeof(uns32)+sizeof(sig16), OP_SENDPAR);
  if (p==NULL) return TRUE;
  *(uns32*)p=handle;  p+=sizeof(uns32);
  *(sig16*)p=SEND_PAR_EXEC_PREP;
  REGISTER_ANSWER(p_psize, sizeof(uns32));
  REGISTER_ANSWER(results==NULL ? default_results : results, ANS_SIZE_PREVITEM);
  if (cond_send(cdp)) return TRUE;
  if (count!=NULL) *count=*p_psize/sizeof(uns32);
   
  if (prep->hostvar_count)
    if (pull_clivars(cdp, handle, prep->hostvars, prep->hostvar_count)) return TRUE;
  return FALSE;
}

CFNC DllKernel BOOL WINAPI cd_SQL_drop(cdp_t cdp, uns32 handle)
{// drop local info: 
  if (drop_prep_stmt(cdp, handle))
    { client_error(cdp, ERROR_IN_FUNCTION_CALL);  return TRUE; }
 // drop server info:
  tptr p=get_space_op(cdp, 1+sizeof(uns32)+sizeof(sig16), OP_SENDPAR);
  if (p==NULL) return TRUE;
  *(uns32*)p=handle;  p+=sizeof(uns32);
  *(sig16*)p=SEND_PAR_DROP_STMT;
  return cond_send(cdp);
}

CFNC DllKernel BOOL WINAPI cd_SQL_get_result_set_info(cdp_t cdp, uns32 handle, unsigned rs_order, d_table ** td)
{ t_prep_stmt * prep = find_prep_stmt(cdp, handle);
  if (prep==NULL) { client_error(cdp, ERROR_IN_FUNCTION_CALL);  return TRUE; }
  tptr p=get_space_op(cdp, 1+4+2*sizeof(sig16), OP_SENDPAR);
  if (p==NULL) return TRUE;
  *(uns32*)p=handle;            p+=sizeof(uns32);
  *(sig16*)p=SEND_PAR_RS_INFO;  p+=sizeof(sig16);
  *(uns16*)p=rs_order;
  REGISTER_ANSWER(td, ANS_SIZE_DYNALLOC);
  if (cond_send(cdp)) return TRUE;
#if WBVERS>=950
    if (cdp->protocol_version() == 0)
      *td=convert_protocol_in_td((d_table_0*)*td);
#else      
    if (cdp->protocol_version() == 1)
      *td=convert_protocol_in_td((d_table_1*)*td);
#endif
  if (!cdp->in_package) normalize_string_specifs(*td);
  return FALSE;
}

CFNC DllKernel BOOL WINAPI cd_SQL_get_param_info(cdp_t cdp, uns32 handle, void ** pars)
{ t_prep_stmt * prep = find_prep_stmt(cdp, handle);
  if (prep==NULL) { client_error(cdp, ERROR_IN_FUNCTION_CALL);  return TRUE; }
  tptr p=get_space_op(cdp, 1+4+sizeof(sig16), OP_SENDPAR);
  if (p==NULL) return TRUE;
  *(uns32*)p=handle;             p+=sizeof(uns32);
  *(sig16*)p=SEND_PAR_PAR_INFO;  
  REGISTER_ANSWER(pars, ANS_SIZE_DYNALLOC);
  return cond_send(cdp);
}


CFNC DllKernel BOOL WINAPI cd_Open_cursor_direct(cdp_t cdp, const char * query, tcursnum * curnum)
{ uns32 submit_result[MAX_SQL_RESULTS];
  submit_result[0]=0;  // statement handle!
  if (cd_SQL_execute(cdp, query, submit_result)) return TRUE;
  if (!IS_CURSOR_NUM(submit_result[0]))
    { client_error(cdp, SELECT_EXPECTED);  return TRUE; }  // statement did not generate a result set
  *curnum=(tcursnum)submit_result[0];
  return FALSE;
}

CFNC DllKernel BOOL WINAPI cd_Send_parameter(cdp_t cdp, uns32 stmt, sig16 parnum, uns32 len, uns32 offset, void * value)
// Sends the value of a single parameter from client to server
{ tptr p;  tptr hptr = (tptr)value;
  do  // not breaking the data into parts since 11.0.1.804
  { uns32 loc_len = len;  //> 0x4000 ? 0x4000 : (unsigned)len;  -- faster
    p=get_space_op(cdp, 1+4+10+loc_len+2, OP_SENDPAR);
    if (p==NULL) return TRUE;
    *(uns32*)p=stmt;    p+=sizeof(uns32);
    *(sig16*)p=parnum;  p+=sizeof(sig16);
    *(DWORD*)p=loc_len; p+=sizeof(DWORD);
    *(DWORD*)p=offset;  p+=sizeof(DWORD);
    memcpy(p, hptr, loc_len);
    *(sig16*)(p+loc_len)=-1;
    if (cond_send(cdp)) return TRUE;
    len-=loc_len;  hptr+=loc_len;  offset+=loc_len;
  } while (len);
  return FALSE;
}

CFNC DllKernel BOOL WINAPI cd_Send_params(cdp_t cdp, uns32 stmt, DWORD len, void * value)
/* Optimize this later! Too much copying! */
{ tptr p=get_space_op(cdp, 1+sizeof(uns32)+len, OP_SENDPAR);
  if (p==NULL) return TRUE;
  *(uns32*)p=stmt;  p+=sizeof(uns32);
  memcpy(p, value, len);
  return cond_send(cdp);
}

#define RD_STEP 0x5000  /* TCP/IP traffic optimization */

CFNC DllKernel BOOL WINAPI cd_Read_var(cdp_t cdp, tcursnum cursor,
  trecnum position, tattrib attr, t_mult_size index, t_varcol_size start, t_varcol_size size,
  void * buf, t_varcol_size * psize)
{ tptr p;  uns32 loc_size;  tptr hbuf = (tptr)buf;
#ifdef CLIENT_ODBC
  if (IS_ODBC_TABLE(cursor))
  { modifrec loc_access[2], * acc=loc_access;
    if (index!=NOINDEX)
      { client_error(cdp, BAD_MODIF);  return TRUE; }  /* multiattrs not supported */
    loc_access[0].modtype=MODINT;
    loc_access[0].moddef.modint.start=start;
    loc_access[0].moddef.modint.size =size;
    loc_access[1].modtype=MODSTOP;
    if (psize!=NULL) *psize=0;
    return odbc_rw(cdp, cursor, position, attr, loc_access, buf, FALSE, (t_varcol_size *)psize);
  }
  if (cursor==OBJ_TABLENUM && cdp->IStorageCache && attr!=DEL_ATTR_NUM && attr!=SYSTAB_PRIVILS_ATR && !cdp->in_package)
  { t_io_oper_size iosize;
    if (!read_pcached(cdp, position, attr, buf, start, size, &iosize, RW_OPER_VAR)) return TRUE;
    if (psize!=NULL) *psize=iosize;
    return FALSE;
  }
#endif
  unsigned sizeof_tattrib = cdp->sizeof_tattrib();
  static t_varcol_size glob_rd_size; /* packaging needs this variable after leaving Read_var */
  t_varcol_size loc_rd_size;
  t_varcol_size * p_rd_size = cdp->in_package ? &glob_rd_size : &loc_rd_size;
  if (psize) *psize=0;
  do
  { loc_size = cdp->in_package ?
      (size > 0xf000  ? 0xf000  : size) :  /* must be bigger than MAX_PACKAGE */
      (size > RD_STEP ? RD_STEP : size);
    p=get_space_op(cdp, 1+sizeof(tcursnum)+sizeof(trecnum)+sizeof_tattrib+((index==NOINDEX) ? 10 : 13), OP_READ);
    if (p==NULL) return TRUE;
    *(tcursnum*)p=cursor;   p+=sizeof(tcursnum);
    *(trecnum*)p=position;  p+=sizeof(trecnum);
    write_attrib(p, &attr, sizeof_tattrib);
    if (index != NOINDEX)
    { *(p++)=MODIND;
      *(t_mult_size *)p=index;       p+=sizeof(t_mult_size);
    }
    *(p++)=MODINT;
    *(t_varcol_size *)p=start;       p+=sizeof(t_varcol_size);
    *(t_varcol_size *)p=loc_size;    p+=sizeof(t_varcol_size);
    *(p++)=MODSTOP;
    cdp->ans_array[cdp->ans_ind]=p_rd_size;
    cdp->ans_type [cdp->ans_ind]=sizeof(t_varcol_size);
    cdp->ans_ind++;
    cdp->ans_array[cdp->ans_ind]=hbuf;
    cdp->ans_type [cdp->ans_ind]=ANS_SIZE_PREVITEM;
    cdp->ans_ind++;
    if (cond_send(cdp)) return TRUE;
    hbuf+=loc_size;  start+=loc_size;  size-=loc_size;
    if (!cdp->in_package) if (psize) *psize+=*p_rd_size;
  } while (size);
  return FALSE;
}

CFNC DllKernel BOOL WINAPI cd_Write_var(cdp_t cdp, tcursnum cursor,
  trecnum position, tattrib attr, t_mult_size index, t_varcol_size start, t_varcol_size size,
  const void * buf)
{ tptr p;  unsigned loc_size;
#ifdef CLIENT_ODBC
  if (IS_ODBC_TABLE(cursor))
  { modifrec loc_access[2], * acc=loc_access;
    if (index!=NOINDEX)
      { client_error(cdp, BAD_MODIF);  return TRUE; }  /* multiattrs not supported */
    loc_access[0].modtype=MODINT;
    loc_access[0].moddef.modint.start=start;
    loc_access[0].moddef.modint.size =size;
    loc_access[1].modtype=MODSTOP;
    return odbc_rw(cdp, cursor, position, attr, loc_access, (void*)buf, TRUE, NULL);
  }
#endif
  unsigned sizeof_tattrib = cdp->sizeof_tattrib();
  uns32 orig_start=start, orig_size=size;  const void * orig_buf=buf;
  do
  { loc_size = (size > RD_STEP) ? RD_STEP : (unsigned)size;
    p=get_space_op(cdp, 1+sizeof(tcursnum)+sizeof(trecnum)+sizeof_tattrib+
         ((index==NOINDEX) ? 10 : 13)+loc_size+1, OP_WRITE);
    if (p==NULL) return TRUE;
    *(tcursnum*)p=cursor;   p+=sizeof(tcursnum);
    *(trecnum*)p=position;  p+=sizeof(trecnum);
    write_attrib(p, &attr, sizeof_tattrib);
    if (index != NOINDEX)
    { *(p++)=MODIND;
      *(t_mult_size *)p=index;      p+=sizeof(t_mult_size);
    }
    *(p++)=MODINT;
    *(t_varcol_size*)p=start;       p+=sizeof(t_varcol_size);
    *(t_varcol_size*)p=loc_size;    p+=sizeof(t_varcol_size);
    *(p++)=MODSTOP;
    memcpy(p, buf, loc_size);
    p[loc_size]=(char)DATA_END_MARK;
    if (cond_send(cdp)) return TRUE;
    buf=(tptr)buf+loc_size;  start+=loc_size;  size-=loc_size;
  } while (size);
#ifdef CLIENT_GUI
  if (cursor==OBJ_TABLENUM && cdp->IStorageCache && attr!=DEL_ATTR_NUM && attr!=SYSTAB_PRIVILS_ATR)
    write_pcache(cdp, position, attr, orig_buf, orig_start, orig_size, RW_OPER_VAR);
#endif
  return FALSE;
}

CFNC DllKernel BOOL WINAPI cd_Write_lob(cdp_t cdp, tcursnum cursor, trecnum position, tattrib attr, t_varcol_size size, const void * buf)
{ tptr p;  
  p=get_space_op(cdp, 1+sizeof(tcursnum)+sizeof(trecnum)+cdp->sizeof_tattrib()+
                      2+2*sizeof(t_varcol_size)/*+size+1*/, OP_WRITE);
  if (p==NULL) return TRUE;
  *(tcursnum*)p=cursor;   p+=sizeof(tcursnum);
  *(trecnum*)p=position;  p+=sizeof(trecnum);
  write_attrib(p, &attr, cdp->sizeof_tattrib());
  *(p++)=MODINT;
  *(t_varcol_size*)p=(t_varcol_size)-1;  p+=sizeof(t_varcol_size);  // -1 is a signal that the total size should be set
  *(t_varcol_size*)p=size;               p+=sizeof(t_varcol_size);
  *(p++)=MODSTOP;
  /*memcpy(p, buf, size);
  p[size]=(char)DATA_END_MARK;*/
 // space allocation optimized:
  cdp->add_request_fragment((char*)buf, size, false);
  cdp->set_persistent_part();
  get_space_op(cdp, 1, DATA_END_MARK);
  return cond_send(cdp);
}

CFNC DllKernel BOOL WINAPI cd_Get_error_info(cdp_t cdp, uns32 * line, uns16 * column, tptr buf)
{ tptr p=get_space_op(cdp, 1, OP_GETERROR);
  if (p==NULL) return TRUE;
  cdp->ans_array[cdp->ans_ind]=line;
  cdp->ans_type [cdp->ans_ind]=sizeof(uns32);
  cdp->ans_ind++;
  cdp->ans_array[cdp->ans_ind]=column;
  cdp->ans_type [cdp->ans_ind]=sizeof(uns16);
  cdp->ans_ind++;
  cdp->ans_array[cdp->ans_ind]=buf;
  cdp->ans_type [cdp->ans_ind]=PRE_KONTEXT+POST_KONTEXT+2;
  cdp->ans_ind++;
  return cond_send(cdp);
}

static BOOL C_aggr(cdp_t cdp, tcursnum curs, const char * attrname, const char * condition,
            void * result, int aggr_type)
{ tcursnum subcurs;  d_attr datr;  tptr p;  BOOL res;
  char aname[ATTRNAMELEN+1];
  if (IS_ODBC_TABLE(curs)) { client_error(cdp, CANNOT_FOR_ODBC);  return TRUE; }
  strmaxcpy(aname, attrname, ATTRNAMELEN+1);
  Upcase9(cdp, aname);
  BOOL sub=condition && *condition;
  if (!find_attr(cdp, curs, CATEG_TABLE, aname, NULL, NULL, &datr))
  { SET_ERR_MSG(cdp, aname);  client_error(cdp, ATTRIBUTE_NOT_FOUND);
    return TRUE;
  }
  if (sub)
  { if (cd_Open_subcursor(cdp, curs, condition, &subcurs)) return TRUE;
    curs=subcurs;
  }
  unsigned sizeof_tattrib = cdp->sizeof_tattrib();
  p=get_space_op(cdp, 1+sizeof(tcursnum)+sizeof_tattrib+1, OP_AGGR);
  if (!p) res=TRUE;
  else
  { *(tcursnum*)p=curs;  p+=sizeof(tcursnum);
    tattrib attr = datr.get_num();
    write_attrib(p, &attr, sizeof_tattrib);
    *p=(uns8)aggr_type;
    cdp->ans_array[cdp->ans_ind]=result;
    cdp->ans_type [cdp->ans_ind]=ANS_SIZE_VARIABLE;
    cdp->ans_ind++;
    res=cond_send(cdp);
  }
  if (sub) cd_Close_cursor(cdp, subcurs);
  return res;
}

CFNC DllKernel BOOL WINAPI cd_C_sum(cdp_t cdp, tcursnum curs, const char * attrname,
  const char * condition, void * result)
{ return C_aggr(cdp, curs, attrname, condition, result, C_SUM); }

CFNC DllKernel BOOL WINAPI cd_C_avg(cdp_t cdp, tcursnum curs, const char * attrname,
  const char * condition, void * result)
{ return C_aggr(cdp, curs, attrname, condition, result, C_AVG); }

CFNC DllKernel BOOL WINAPI cd_C_max(cdp_t cdp, tcursnum curs, const char * attrname,
  const char * condition, void * result)
{ return C_aggr(cdp, curs, attrname, condition, result, C_MAX); }

CFNC DllKernel BOOL WINAPI cd_C_min(cdp_t cdp, tcursnum curs, const char * attrname,
  const char * condition, void * result)
{ return C_aggr(cdp, curs, attrname, condition, result, C_MIN); }

CFNC DllKernel BOOL WINAPI cd_C_count(cdp_t cdp, tcursnum curs, const char * attrname,
  const char * condition, trecnum * result)
{ return C_aggr(cdp, curs, attrname, condition, result, C_COUNT); }

static uns32 bb32;  // for unused results from the server

CFNC DllKernel BOOL WINAPI cd_Database_integrity(cdp_t cdp, BOOL repair,
           uns32 * lost_blocks, uns32 * lost_dheap,
           uns32 * nonex_blocks, uns32 * cross_link, uns32 * damaged_tabdef)
{ tptr p;  
  p=get_space_op(cdp, 1+sizeof(uns32)+5*sizeof(uns32), OP_INTEGRITY);
  if (p==NULL) return TRUE;
  *(uns32*)p=repair;                  p+=sizeof(uns32);
  *(uns32*)p=lost_blocks    != NULL;  p+=sizeof(uns32);
  *(uns32*)p=lost_dheap     != NULL;  p+=sizeof(uns32);
  *(uns32*)p=nonex_blocks   != NULL;  p+=sizeof(uns32);
  *(uns32*)p=cross_link     != NULL;  p+=sizeof(uns32);
  *(uns32*)p=damaged_tabdef != NULL;
#if 0
  uns32 buf[5];  BOOL res;
  cdp->ans_array[cdp->ans_ind]=buf;
  cdp->ans_type [cdp->ans_ind]=5*sizeof(uns32);
  cdp->ans_ind++;
  res=cond_send(cdp);
  if (!res)
  { if (lost_blocks   !=NULL) *lost_blocks   =buf[0];
    if (lost_dheap    !=NULL) *lost_dheap    =buf[1];
    if (nonex_blocks  !=NULL) *nonex_blocks  =buf[2];
    if (cross_link    !=NULL) *cross_link    =buf[3];
    if (damaged_tabdef!=NULL) *damaged_tabdef=buf[4];
  }
  return res;
#else
  REGISTER_ANSWER(lost_blocks    ? lost_blocks    : &bb32, sizeof(uns32));
  REGISTER_ANSWER(lost_dheap     ? lost_dheap     : &bb32, sizeof(uns32));
  REGISTER_ANSWER(nonex_blocks   ? nonex_blocks   : &bb32, sizeof(uns32));
  REGISTER_ANSWER(cross_link     ? cross_link     : &bb32, sizeof(uns32));
  REGISTER_ANSWER(damaged_tabdef ? damaged_tabdef : &bb32, sizeof(uns32));
  return cond_send(cdp);
#endif
}

CFNC DllKernel BOOL WINAPI cd_SQL_catalog(cdp_t cdp, BOOL metadata, unsigned ctlg_type,
                             unsigned parsize, tptr inpars, uns32 * result)
{ tptr p=get_space_op(cdp, 1+1+2*sizeof(uns16)+parsize, OP_SQL_CATALOG);
  if (p==NULL) return TRUE;
  *p=(char)metadata;     p++;
  *(uns16*)p=ctlg_type;  p+=sizeof(uns16);
  *(uns16*)p=parsize;    p+=sizeof(uns16);
  memcpy(p, inpars, parsize);
  cdp->ans_array[cdp->ans_ind]=result;
  cdp->ans_type [cdp->ans_ind]=sizeof(uns32);
  cdp->ans_ind++;
  return cond_send(cdp);
}

CFNC DllKernel BOOL WINAPI Server_access(cdp_t cdp, char * buf)
{ //if (cdp->in_use==PT_REMOTE) *(buf++)='*';  -- no more, was not not documented and is useless now
  return cd_Get_server_info(cdp, OP_GI_SERVER_NAME, buf, sizeof(tobjname));
}

CFNC DllKernel BOOL WINAPI cd_Query_optimization(cdp_t cdp, const char * query, BOOL evaluate, tptr * result)
{ int len=(int)strlen(query)+1;  *result=NULL;
  tptr p=get_space_op(cdp, 1+1+1+len, OP_CNTRL);
  if (p==NULL) return TRUE;
  *(p++)=SUBOP_OPTIMIZATION; /* suboper. */
  *(wbbool*)p = evaluate!=0;  p+=sizeof(wbbool);
  memcpy(p, query, len);
  REGISTER_ANSWER(result, ANS_SIZE_DYNALLOC);
  return cond_send(cdp);
}

CFNC DllKernel BOOL WINAPI cd_Message_to_clients(cdp_t cdp, const char * msg)
{ tptr p=get_space_op(cdp, 1+1+(unsigned)strlen(msg)+1, OP_CNTRL);
  if (p==NULL) return TRUE;
  *(p++)=SUBOP_MESSAGE; /* suboper. */
  strcpy(p, msg);
  return cond_send(cdp);
}

CFNC DllKernel BOOL WINAPI cd_Trigger_changed(cdp_t cdp, tobjnum trigobj, BOOL after)
// Registers or unregisters triggers, registers fulltexts.
{ tptr p=get_space_op(cdp, 1+1+sizeof(tobjnum), OP_CNTRL);
  if (p==NULL) return TRUE;
  *(p++)=after ? SUBOP_TRIG_REGISTER : SUBOP_TRIG_UNREGISTER; /* suboper. */
  *(tobjnum*)p=trigobj;
  return cond_send(cdp);
}

CFNC DllKernel BOOL WINAPI cd_Procedure_changed(cdp_t cdp)
{ tptr p=get_space_op(cdp, 1+1, OP_CNTRL);
  if (p==NULL) return TRUE;
  *p=SUBOP_PROC_CHANGED; /* suboper. */
  return cond_send(cdp);
}

CFNC DllKernel BOOL WINAPI cd_Set_temporary_authoring_mode(cdp_t cdp, BOOL on_off)
{ tptr p=get_space_op(cdp, 1+1+1, OP_CNTRL);
  if (p==NULL) return TRUE;
  *(p++)=SUBOP_SET_AUTHORING; /* suboper. */
  *p=on_off!=0;
  return cond_send(cdp);
}

CFNC DllKernel int WINAPI program_flags(cdp_t cdp, tptr src)  // used on UNIX in the application import, too
{ tptr p=src;
  while (*p==' ' || *p=='\r' || *p=='\n') p++;
  if (*p=='*')                    return CO_FLAG_TRANSPORT;
  if (!strnicmp(p, "INCLUDE", 7)) return CO_FLAG_INCLUDE;
  if (!memcmp(p, "<?", 2))        return CO_FLAG_XML;
  return 0;
}

CFNC DllKernel BOOL WINAPI cd_Rolled_back(cdp_t cdp, BOOL * rolled_back)
{ uns8 _rolled_back;
  if (cdp->last_error_is_from_client || !IS_AN_ERROR(cdp->RV.report_number))
    { *rolled_back=FALSE;  return FALSE; }
  *rolled_back=TRUE;  // when a transaction with error is in progress, answer is not returned!
  tptr p=get_space_op(cdp, 1+1+sizeof(uns16), OP_CNTRL);
  if (p==NULL) return TRUE;
  *(p++)=SUBOP_ROLLED_BACK; /* suboper. */
  *(uns16*)p=cdp->RV.report_number;
  REGISTER_ANSWER(&_rolled_back, sizeof(uns8));
  if (cond_send(cdp)) return TRUE;
  *rolled_back=_rolled_back;
  return FALSE;
}

CFNC DllKernel BOOL WINAPI cd_Transaction_open(cdp_t cdp, BOOL * transaction_open)
{ *transaction_open=TRUE;  // when a transaction with error is in progress, answer is not returned!
  tptr p=get_space_op(cdp, 1+1, OP_CNTRL);
  if (p==NULL) return TRUE;
  *p=SUBOP_TRANS_OPEN; /* suboper. */
  REGISTER_ANSWER(transaction_open, sizeof(uns8));  // returns only 1 byte, must be init!
  return cond_send(cdp);
}


#ifdef STOP
// on LINUX used only for category==CATEG_PGMSRC
CFNC DllKernel unsigned WINAPI get_object_flags(cdp_t cdp, tcateg category, ctptr name)
{ unsigned flags;  tobjnum objnum, lobjnum;  tcateg cat;
  if (cd_Find_object(cdp, name, (tcateg)(category|IS_LINK), &lobjnum)) return 0;
  if (cd_Read(cdp, CATEG2SYSTAB(category), lobjnum, OBJ_CATEG_ATR, NULL, &cat)) return 0;
  if (cat & IS_LINK)
  { flags=CO_FLAG_LINK;
    if (cd_Find_object(cdp, name, category, &objnum)) return 0;
  }
  else
    { flags=0;  objnum=lobjnum; }
  tptr src=cd_Load_objdef(cdp, objnum, CATEG_PGMSRC);
  if (src)
  { flags|=program_flags(cdp, src);
    corefree(src);
  }
  return flags;
}
#endif

CFNC DllKernel BOOL WINAPI cd_Create3_link(cdp_t cdp, const char * sourcename,
             const uns8 * sourceapplid, tcateg category, const char * linkname, BOOL limited)
{ tobjnum objnum;  t_wb_link wb_link;  ttablenum tb;
  category &= CATEG_MASK;
  if (category==CATEG_USER || category==CATEG_GROUP || category==CATEG_APPL)
    return TRUE;  // links not allowed
  if (limited ? cd_Insert_object_limited(cdp, linkname, category | IS_LINK, &objnum) :
                cd_Insert_object        (cdp, linkname, category | IS_LINK, &objnum)) return TRUE;
  wb_link.link_type=LINKTYPE_WB;
  strmaxcpy(wb_link.destobjname, sourcename,   OBJ_NAME_LEN+1);
  memcpy   (wb_link.dest_uuid,   sourceapplid, UUID_SIZE);
  Upcase9(cdp, wb_link.destobjname);
  tb=CATEG2SYSTAB(category);
  if (cd_Write_var(cdp, tb, (trecnum)objnum, OBJ_DEF_ATR, NOINDEX, 0, sizeof(t_wb_link), &wb_link))
    { cd_Delete(cdp, tb, objnum);  return TRUE; }
  uns16 flags = CO_FLAG_LINK;
  cd_Write(cdp, tb, (trecnum)objnum, OBJ_FLAGS_ATR, NULL, &flags, sizeof(uns16));
#ifdef CLIENT_GUI
 // add link component if destination exists:
  tobjnum destobj;
  if (!cd_Find2_object(cdp, sourcename, sourceapplid, category, &destobj))
  // unless importing appl with not-resolved link -> dest object does not exists
    Add_component(cdp, category, linkname, destobj, FALSE);  // must use destination object number!
#endif
  return FALSE;
}

#ifdef CLIENT_GUI
CFNC DllKernel BOOL WINAPI cd_Create2_link(cdp_t cdp, const char * sourcename,
             const uns8 * sourceapplid, tcateg category, const char * linkname)
{ return cd_Create3_link(cdp, sourcename, sourceapplid, category, linkname, FALSE); }

CFNC DllKernel BOOL WINAPI cd_Create_link(cdp_t cdp, const char * sourcename,
             const char * sourceappl, tcateg category, const char * linkname)
{ WBUUID sourceapplid;
  if (cd_Apl_name2id(cdp, sourceappl, sourceapplid)) return TRUE;
  return cd_Create2_link(cdp, sourcename, sourceapplid, category, linkname);
}

#include "../secur/wbsecur.h"

CFNC DllKernel BOOL WINAPI cd_Signature(cdp_t cdp, HWND hParent, tcursnum cursor, trecnum recnum, tattrib attr, int oper, void * param)
{ uns32 nic;
  unsigned sizeof_tattrib = cdp->sizeof_tattrib();
  if (oper==1) 
  { WBUUID own_uuid;  
   // find own UUID:
    cd_Read(cdp, USER_TABLENUM, cdp->logged_as_user, USER_ATR_UUID, NULL, own_uuid);
   // find a valid certificate:
    trecnum cert_recnum;
    t_Certificate * cert = get_best_own_certificate(cdp, own_uuid, false, &cert_recnum);
    if (cert) 
    {// get private key:
      CryptoPP::InvertibleRSAFunction * private_key = get_private_key_for_certificate(hParent, cd_Who_am_I(cdp), cert);
      DeleteCert(cert);
      if (!private_key) return TRUE;
     // prepare signing: compute the hash of the record:
      unsigned parsize=1+sizeof(tcursnum)+sizeof(trecnum)+sizeof_tattrib+1+sizeof(trecnum);
      tptr p=get_space_op(cdp, parsize, OP_SIGNATURE);
      if (p==NULL) return TRUE;
      *(tcursnum*)p=cursor;  p+=sizeof(tcursnum);
      *(trecnum *)p=recnum;  p+=sizeof(trecnum);
      write_attrib(p, &attr, sizeof_tattrib);
      *p=(char)14;           p++;
      *(trecnum *)p=cert_recnum;
      t_hash_ex hsh;  uns32 stamp;
      REGISTER_ANSWER(&hsh,   sizeof(hsh));
      REGISTER_ANSWER(&stamp, sizeof(stamp));
      BOOL res=cond_send(cdp);
      if (res) { DeletePrivKey(private_key);  return TRUE; }
     // encrypt:
      t_enc_hash enc_hsh;
      digest2signature(hsh, sizeof(hsh), private_key, enc_hsh);
      DeletePrivKey(private_key);
     // store the signature:
      p=get_space_op(cdp, 1+sizeof(tcursnum)+sizeof(trecnum)+sizeof_tattrib+1+sizeof(t_enc_hash)+sizeof(trecnum)+sizeof(uns32), OP_SIGNATURE);
      if (p==NULL) return TRUE;
      *(tcursnum*)p=cursor;                     p+=sizeof(tcursnum);
      *(trecnum *)p=recnum;                     p+=sizeof(trecnum);
      write_attrib(p, &attr, sizeof_tattrib);
      *p=(char)15;                              p++;
      memcpy(p, &enc_hsh, sizeof(t_enc_hash));  p+=sizeof(t_enc_hash);
      *(trecnum *)p=cert_recnum;                p+=sizeof(trecnum);
      *(uns32   *)p=stamp;
      return cond_send(cdp);
    }
   // own certificate not found, try old keys:
    oper=4;  param=&nic; 
  }
 // original version:
  t_hash hsh;  uns32 newstate;  
  t_priv_key_file temp_priv_key_file;  BOOL store_key = FALSE;
  unsigned parsize=1+sizeof(tcursnum)+sizeof(trecnum)+sizeof_tattrib+1;
  if (oper==4)
  { parsize+=UUID_SIZE+sizeof(uns32);
    if (cdp->priv_key_file==NULL)  // must read the key
    { BOOL res=get_private_key(hParent, &temp_priv_key_file, &store_key);
      if (!res) return TRUE;
      if (res==2) { SET_ERR_MSG(cdp, "KeyOwner");  client_error(cdp, BAD_PASSWORD);  return TRUE; }
    }
    else memcpy(&temp_priv_key_file, cdp->priv_key_file, sizeof(t_priv_key_file));
  }
  else if (oper==6)  // storing a public key
    parsize+=UUID_SIZE+sizeof(CPubKey);
  tptr p=get_space_op(cdp, parsize, OP_SIGNATURE);
  if (p==NULL) return TRUE;
  *(tcursnum*)p=cursor;  p+=sizeof(tcursnum);
  *(trecnum *)p=recnum;  p+=sizeof(trecnum);
  write_attrib(p, &attr, sizeof_tattrib);
  *p=(char)oper;         p++;
  if (oper==4)
  { memcpy(p, temp_priv_key_file.key_uuid, UUID_SIZE);  p+=UUID_SIZE;
    *(uns32*)p=*(uns32*)param;  // expiration date
    REGISTER_ANSWER(&hsh, sizeof(t_hash));
  }
  else if (oper==6)
    memcpy(p, param, UUID_SIZE+sizeof(CPubKey));
  else if (!oper)  // check
    REGISTER_ANSWER(&newstate, sizeof(uns32))
  else if (oper==7)  // trace
    REGISTER_ANSWER(param, ANS_SIZE_DYNALLOC);
  BOOL res=cond_send(cdp);
  if (!oper && param) *(t_sigstate*)param=(t_sigstate)newstate;
  if (oper!=4 || res) return res;
  if (store_key)
  { cdp->priv_key_file = (t_priv_key_file*)corealloc(sizeof(t_priv_key_file), 63);
    if (cdp->priv_key_file!=NULL) // no error if cannot store the key
      memcpy(cdp->priv_key_file, &temp_priv_key_file, sizeof(t_priv_key_file));
  }
 // encrypt:
  t_enc_hash enc_hsh;
//  memcpy(&enc_hsh, &hsh, MD5_SIZE);
  if (EDPrivateEncrypt(enc_hsh, hsh, sizeof(hsh), &temp_priv_key_file.KeyPair) < 0)
    return TRUE;
  p=get_space_op(cdp, 1+sizeof(tcursnum)+sizeof(trecnum)+sizeof_tattrib+1+sizeof(t_enc_hash), OP_SIGNATURE);
  if (p==NULL) return TRUE;
  *(tcursnum*)p=cursor;  p+=sizeof(tcursnum);
  *(trecnum *)p=recnum;  p+=sizeof(trecnum);
  write_attrib(p, &attr, sizeof_tattrib);
  *p=(char)5;            p++;
  memcpy(p, &enc_hsh, sizeof(t_enc_hash));
  return cond_send(cdp);
}

CFNC DllKernel BOOL WINAPI Create_link(const char * sourcename, const char * sourceappl, tcateg category, const char * linkname)
{ return cd_Create_link(GetCurrTaskPtr(), sourcename, sourceappl, category, linkname); }

CFNC DllKernel BOOL WINAPI Create2_link(const char * sourcename, const uns8 * sourceapplid, tcateg category, const char * linkname)
{ return cd_Create2_link(GetCurrTaskPtr(), sourcename, sourceapplid, category, linkname); }

CFNC DllKernel BOOL WINAPI Signature(HWND hParent, tcursnum cursor, trecnum recnum, tattrib attr, int oper, void * param)
{ return cd_Signature(GetCurrTaskPtr(), hParent, cursor, recnum, attr, oper, param); }

#endif

CFNC DllKernel BOOL WINAPI cd_Repl_control(cdp_t cdp, int optype, int opparsize, void * opparam)
{ tptr p=get_space_op(cdp, 1+1+sizeof(uns16)+opparsize, OP_REPLCONTROL);
  if (p==NULL) return TRUE;
  *p=optype;  p++;
  *(uns16*)p=(uns16)opparsize;  p+=sizeof(uns16);
  memcpy(p, opparam, opparsize);
  return cond_send(cdp);
}

static BOOL SrvrAppl2ObjUUID(cdp_t cdp, const char *Srvr, const char *Appl, tobjnum *SrvrObj, unsigned char *SrvrUUID, tobjnum *ApplObj, unsigned char *ApplUUID)
{ if (cd_Find2_object(cdp, Srvr, NULL, CATEG_SERVER, SrvrObj)) return TRUE;
  if (   cd_Read(cdp, SRV_TABLENUM, *SrvrObj, SRV_ATR_UUID, NULL, SrvrUUID)) return TRUE;
  if (cd_Find2_object(cdp, Appl, NULL, CATEG_APPL,   ApplObj)) return TRUE;
  return cd_Read(cdp, OBJ_TABLENUM, *ApplObj, APPL_ID_ATR,  NULL, ApplUUID);
}

CFNC DllKernel BOOL WINAPI cd_Replicate(cdp_t cdp, const char * ServerName, const char * ApplName, BOOL pull)
{ tobjnum SrvrObj, ApplObj;  t_repl_param param;
  if (SrvrAppl2ObjUUID(cdp, ServerName, ApplName ? ApplName : cdp->sel_appl_name, &SrvrObj, param.server_id, &ApplObj, param.appl_id))
    return TRUE;
  param.token_req=wbfalse;  param.spectab[0]=0;  param.len16=0;
  return cd_Repl_control(cdp, pull ? PKTTP_REPL_REQ : PKTTP_REPL_DATA, sizeof(param), (char*)&param);
}

CFNC DllKernel BOOL WINAPI cd_Skip_repl(cdp_t cdp, const char * ServerName, const char * ApplName)
{ tobjnum SrvrObj, ApplObj;  t_repl_param param;
  if (SrvrAppl2ObjUUID(cdp, ServerName, ApplName ? ApplName : cdp->sel_appl_name, &SrvrObj, param.server_id, &ApplObj, param.appl_id))
    return TRUE;
  param.token_req=wbfalse;  param.spectab[0]=0;  param.len16=0;
  return cd_Repl_control(cdp, OPER_SKIP_REPL, sizeof(param), (char*)&param);
}

CFNC DllKernel bool WINAPI convert_to_SQL_literal(tptr dest, tptr source, int type, t_specif specif, bool add_eq)
{ return cd_convert_to_SQL_literal(GetCurrTaskPtr(), dest, source, type, specif, add_eq); }

CFNC DllKernel bool WINAPI cd_convert_to_SQL_literal(cdp_t cdp, tptr dest, tptr source, int type, t_specif specif, bool add_eq)
// Converts native value pointer by [source] of type [type, specif] into a SQL literal and writes it to [dest]
// If [add_eq] adds a = or IS in front of the literal.
// Returns true if the value is NULL.
{ int i=0;
  if (add_eq) { dest[0]='=';  i++; }
  if (is_string(type) || type==ATT_TEXT)
  { bool oldqt = true;
#if WBVERS>=900
    uns32 opt = 0;
    cd_Get_sql_option(cdp, &opt);
    oldqt = (opt & SQLOPT_OLD_STRING_QUOTES) != 0;
#endif
    if (specif.wide_char)
    { if (!*(wuchar*)source) goto null_value;
      bool has_wide_char = false;
      for (int j=0;  ((wuchar*)source)[j];  j++)
        if (((wuchar*)source)[j] >= 128 || ((wuchar*)source)[j] < ' ')
          { has_wide_char = true;  break; }
      if (has_wide_char)
      { strcpy(dest+i, "U&'");  i+=3; 
        while (*(wuchar*)source)
        { if (*(wuchar*)source >= 128 || *(wuchar*)source < ' ')
          { dest[i++]='\\';
            bin2hex(dest+i, (uns8*)source+1, 1);  i+=2;
            bin2hex(dest+i, (uns8*)source,   1);  i+=2;
          }
          else
          { dest[i++]=*source;
            if (*source=='\'') dest[i++]='\'';
            else if (*source=='\\') dest[i++]='\\';
          }
          source+=sizeof(wuchar);
        }
      }
      else
      { dest[i++]='\'';
        while (*(wuchar*)source)
        { dest[i++]=(char)*source;
          if (*source=='\'') dest[i++]='\'';
          if (*source=='"' && oldqt) dest[i++]='"';
          source+=sizeof(wuchar);
        }
      }
    }
    else   // 8-bit character
    { if (!*source) goto null_value;
      bool has_ctrl_char = false;
      for (int j=0;  source[j];  j++)
        if (((uns8*)source)[j] < ' ')
        { has_ctrl_char = true;  break; }
      if (has_ctrl_char || cdp->sys_charset!=specif.charset)
      { wuchar * wstr = (wuchar*)corealloc(sizeof(wuchar)*(strlen(source)+1), 57);
        conv1to2(source, -1, wstr, specif.charset, 0);
        strcpy(dest+i, "U&'");  i+=3; 
        for (int j=0;  source[j];  j++)
        { if (((uns8*)source)[j] < ' ' || ((uns8*)source)[j] >= 128 )
          { dest[i++]='\\';
            bin2hex(dest+i, (uns8*)(wstr+j)+1, 1);  i+=2;
            bin2hex(dest+i, (uns8*)(wstr+j),   1);  i+=2;
          }
          else // direct copy
          { dest[i++]=source[j];
           // some characters have to doubled:
            if      (source[j]=='\'') dest[i++]='\'';
            else if (source[j]=='\\') dest[i++]='\\';
          }
        }
        corefree(wstr);
      }
      else
      { dest[i++]='\'';
        while (*source)
        { dest[i++]=*source;
          if (*source=='\'') dest[i++]='\'';
          else if (*source=='"' && oldqt) dest[i++]='"';
          source++;
        }
      }
    }
    dest[i++]='\'';  dest[i]=0;
  }
  else if (type==ATT_CHAR)
  { if (!*source) goto null_value;
    dest[i++]='\'';
    dest[i++]=*source;
    if (*source=='\'') dest[i++]='\'';
    dest[i++]='\'';  dest[i]=0;
  }
  else if (type==ATT_BINARY || type==ATT_NOSPEC || type==ATT_RASTER)
  { bool is_null=true;
    dest[i++]='X';  dest[i++]='\'';
    for (int j=0;  j<specif.length;  j++)
    { unsigned bt = source[j];
      if (bt) is_null=false;
      bt >>= 4;
      dest[i++] = bt >= 10 ? 'A'-10+bt : '0'+bt;
      bt = source[j] & 0xf;
      dest[i++] = bt >= 10 ? 'A'-10+bt : '0'+bt;
    }
    if (is_null) goto null_value;
    dest[i++]='\'';  dest[i]=0;
  }
  else
  { dest[i]=0;
    conv2str((basicval0*)source, type, dest+i, SQL_LITERAL_PREZ, specif);
    if (!dest[i]) goto null_value;
  }
  return false;
 null_value:
  strcpy(dest, add_eq ? " IS NULL" : " NULL");
  return true;
}

CFNC DllKernel trecnum WINAPI cd_Look_up(cdp_t cdp, tcursnum curs, const char * attrname, void * res)
{ char cond[530];  tcursnum subcurs;  trecnum rec;  d_attr info;
  char aname[ATTRNAMELEN+1];
  if (IS_ODBC_TABLE(curs)) { client_error(cdp, CANNOT_FOR_ODBC);  return NORECNUM; }
  strmaxcpy(aname, attrname, ATTRNAMELEN+1);
  Upcase9(cdp, aname);
  if (!find_attr(cdp, curs, IS_CURSOR_NUM(curs) ? CATEG_DIRCUR : CATEG_TABLE, aname, NULL, NULL, &info))
  { SET_ERR_MSG(cdp, aname);  client_error(cdp, ATTRIBUTE_NOT_FOUND);
    return NORECNUM;
  }
  strcpy(cond, attrname);
  cd_convert_to_SQL_literal(cdp, cond+strlen(cond), (tptr)res, info.type, info.specif, true);
  if (cd_Open_subcursor(cdp, curs, cond, &subcurs)) return NORECNUM;
  if (cd_Super_recnum(cdp, subcurs, curs, 0, &rec)) rec=NORECNUM;
  cd_Close_cursor(cdp, subcurs);
  return rec;
}

CFNC DllKernel BOOL WINAPI cd_Export_user(cdp_t cdp, tobjnum userobj, FHANDLE hFile)
{ t_container * contr;  tptr p;  BOOL res;
  cdp->auxptr = contr = new t_container;
  if (!contr)
    { client_error(cdp, OUT_OF_KERNEL_MEMORY);  return TRUE; }
  if (!contr->wopen(hFile))
    { delete contr;  client_error(cdp, OS_FILE_ERROR);  return TRUE; }

 // export user record:
  p=get_space_op(cdp, 1+sizeof(ttablenum)+sizeof(tobjnum), OP_SAVE_TABLE);
  if (p==NULL) { res=TRUE;  goto ex; }
  *(ttablenum*)p=USER_TABLENUM;  p+=sizeof(ttablenum);
  *(tobjnum  *)p=userobj;
  res=cond_send(cdp);
  if (res) goto ex;

 // export keys:
  WBUUID user_uuid;  char query[160];  size_t len;  tcursnum curs;
  if (cd_Read(cdp, USER_TABLENUM, userobj, USER_ATR_UUID, NULL, &user_uuid))
    goto ex;
  strcpy(query, "SELECT KEY_UUID,USER_UUID,PUBKEYVAL,CREDATE,EXPDATE,IDENTIF,CERTIFIC,CERTIFICS FROM KEYTAB WHERE USER_UUID=X\'");
  len=strlen(query);
  bin2hex(query+len, user_uuid, UUID_SIZE);  len+=2*UUID_SIZE;
  query[len++]='\'';  query[len]=0;
  if (cd_Open_cursor_direct(cdp, query, &curs)) goto ex;
  p=get_space_op(cdp, 1+sizeof(ttablenum), OP_SAVE_TABLE);
  if (p==NULL) { cd_Close_cursor(cdp, curs);  res=TRUE;  goto ex; }
  *(ttablenum*)p=curs;
  res=cond_send(cdp);
  cd_Close_cursor(cdp, curs);

 ex:
  delete contr;  cdp->auxptr=NULL;
  return res;
}

CFNC DllKernel BOOL WINAPI cd_Import_user(cdp_t cdp, FHANDLE hFile)
{ t_container * contr;
  cdp->auxptr = contr = new t_container;
  if (!contr)
    { client_error(cdp, OUT_OF_KERNEL_MEMORY);  return TRUE; }
  if (!contr->ropen(hFile))
    { delete contr;  client_error(cdp, OS_FILE_ERROR);  return TRUE; }
  tptr p=get_space_op(cdp, 1+sizeof(ttablenum)+1, OP_REST_TABLE);
  if (p==NULL) return TRUE;
  *(ttablenum*)p=USER_TABLENUM; // the next byte in the request is unused
  BOOL res=cond_send(cdp);
  delete contr;  cdp->auxptr=NULL;
  return res;
}

CFNC DllExport BOOL WINAPI Compare_tables(ttablenum tb1, ttablenum tb2,
             trecnum * diffrec, int * diffattr, int * result);

CFNC DllKernel BOOL WINAPI Compare_tables(ttablenum tb1, ttablenum tb2,
             trecnum * diffrec, int * diffattr, int * result)
{ cdp_t cdp = GetCurrTaskPtr();
  tptr p=get_space_op(cdp, 1+1+2*sizeof(ttablenum), OP_INDEX);
  if (p==NULL) return TRUE;
  *(ttablenum*)p=tb1;  p+=sizeof(ttablenum);
  *p=OP_OP_COMPARE;    p++;
  *(ttablenum*)p=tb2;  p+=sizeof(ttablenum);
  REGISTER_ANSWER(diffrec,  sizeof(int));
  REGISTER_ANSWER(diffattr, sizeof(int));
  REGISTER_ANSWER(result,   sizeof(int));
  return cond_send(cdp);
}

CFNC DllKernel BOOL WINAPI cd_Compact_table(cdp_t cdp, ttablenum table)
{ if (SYSTEM_TABLE(table))
    { client_error(cdp, ERROR_IN_FUNCTION_CALL);  return TRUE; }
  tptr p=get_space_op(cdp, 1+1+sizeof(ttablenum), OP_INDEX);
  if (p==NULL) return TRUE;
  *(ttablenum*)p=table;  p+=sizeof(ttablenum);
  *p=OP_OP_COMPACT; 
#ifdef WINS
  HCURSOR hOldCurs = SetCursor(LoadCursor(NULL, IDC_WAIT));
#endif
  BOOL res=cond_send(cdp);
#ifdef WINS
  SetCursor(hOldCurs);
#endif
  return res;
}

CFNC DllKernel BOOL WINAPI cd_Compact_database(cdp_t cdp, int margin)
{ tptr p=get_space_op(cdp, 1+1+sizeof(sig32), OP_CNTRL);
  if (p==NULL) return TRUE;
  *p=SUBOP_COMPACT;  p++;
  *(sig32*)p=margin;  
#ifdef WINS
  HCURSOR hOldCurs = SetCursor(LoadCursor(NULL, IDC_WAIT));
#endif
  BOOL res=cond_send(cdp);
#ifdef WINS
  SetCursor(hOldCurs);
#endif
  return res;
}

CFNC DllKernel BOOL WINAPI cd_CD_ROM(cdp_t cdp)
{ tptr p=get_space_op(cdp, 1+1, OP_CNTRL);
  if (p==NULL) return TRUE;
  *p=SUBOP_CDROM;
#ifdef WINS
  HCURSOR hOldCurs = SetCursor(LoadCursor(NULL, IDC_WAIT));
#endif
  BOOL res=cond_send(cdp);
#ifdef WINS
  SetCursor(hOldCurs);
#endif
  return res;
}

CFNC DllKernel BOOL WINAPI cd_Enable_refint(cdp_t cdp, BOOL enable)
{ tptr p=get_space_op(cdp, 1+1+1, OP_CNTRL);
  if (p==NULL) return TRUE;
  *p=SUBOP_REFINT;  p++;
  *p=(wbbool)enable;
  return cond_send(cdp);
}

CFNC DllKernel BOOL WINAPI cd_Enable_integrity(cdp_t cdp, BOOL enable)
{ tptr p=get_space_op(cdp, 1+1+1, OP_CNTRL);
  if (p==NULL) return TRUE;
  *p=SUBOP_INTEGRITY;  p++;
  *p=(wbbool)enable;
  return cond_send(cdp);
}

CFNC DllKernel BOOL WINAPI cd_Log_write(cdp_t cdp, const char * text)
{ tptr p=get_space_op(cdp, 1+1+(unsigned)strlen(text)+1, OP_CNTRL);
  if (p==NULL) return TRUE;
  *p=SUBOP_LOGWRITE;
  strcpy(p+1, text);
  return cond_send(cdp);
}

#ifdef STOP  // not used any more
CFNC DllKernel BOOL WINAPI cd_Clear_passwords(cdp_t cdp)
{ tptr p=get_space_op(cdp, 1+1, OP_CNTRL);
  if (p==NULL) return TRUE;
  *p=SUBOP_CLEARPASS;
  return cond_send(cdp);
}
#endif

CFNC DllKernel BOOL WINAPI cd_Set_transaction_isolation_level(cdp_t cdp, t_isolation level)
{ tptr p=get_space_op(cdp, 1+1+1, OP_CNTRL);
  if (p==NULL) return TRUE;
  *p=SUBOP_ISOLATION;
  p[1]=(char)level;
  return cond_send(cdp);
}

CFNC DllKernel BOOL WINAPI cd_Set_progress_report_modulus(cdp_t cdp, unsigned modulus)
{ tptr p=get_space_op(cdp, 1+1+sizeof(uns32), OP_CNTRL);
  if (p==NULL) return TRUE;
  *p=SUBOP_REPORT_MODULUS;  p++;
  *(uns32*)p = modulus;
  return cond_send(cdp);
}

#ifdef WINS
CFNC DllKernel BOOL WINAPI cd_Connection_speed_test(cdp_t cdp, int * requests, int * kbytes)
#define SPEED_BUFSIZE 20000
#define SPEED_CNT1     2000
#define SPEED_CNT2      600
{ SYSTEMTIME systime;  unsigned start1, stop1, start2, stop2;  BOOL err=FALSE;  int i;
  *requests=*kbytes=0;
  tptr buf = (tptr)corealloc(SPEED_BUFSIZE, 77);
  if (!buf) { client_error(cdp, OUT_OF_MEMORY);  return TRUE; }
  memset(buf, ' ', SPEED_BUFSIZE-1);  buf[SPEED_BUFSIZE-1]=0;
  HCURSOR hOldCurs = SetCursor(LoadCursor(NULL, IDC_WAIT));
  GetSystemTime(&systime);  
  start1=systime.wMilliseconds+1000*(systime.wSecond+60*systime.wMinute);
  BOOL transaction_open;
  for (i=0;  i<SPEED_CNT1;  i++) 
    //if (cd_Start_transaction(cdp)) { err=TRUE;  break; } -- this warns since 8.1
    if (cd_Transaction_open(cdp, &transaction_open)) { err=TRUE;  break; }
  GetSystemTime(&systime);  
  stop1=systime.wMilliseconds+1000*(systime.wSecond+60*systime.wMinute);
  cd_Roll_back(cdp);
 // cd_SQL_execute(cdp, buf, NULL);
  GetSystemTime(&systime);  
  start2=systime.wMilliseconds+1000*(systime.wSecond+60*systime.wMinute);
  for (i=0;  i<SPEED_CNT2;  i++)
  { uns32 result;  uns32 psize;
    tptr p=get_space_op(cdp, 1+sizeof(uns32)+SPEED_BUFSIZE, OP_SUBMIT);
    if (p==NULL) { err=TRUE;  break; }
    *(uns32*)p=0;  p+=sizeof(uns32);
    memcpy(p, buf, SPEED_BUFSIZE);
    cdp->ans_array[cdp->ans_ind]=&psize;
    cdp->ans_type [cdp->ans_ind]=sizeof(psize);
    cdp->ans_ind++;
    cdp->ans_array[cdp->ans_ind]=&result;
    cdp->ans_type [cdp->ans_ind]=ANS_SIZE_PREVITEM;
    cdp->ans_ind++;
    if (cond_send(cdp)) { err=TRUE;  break; }
  }
  GetSystemTime(&systime);  
  stop2=systime.wMilliseconds+1000*(systime.wSecond+60*systime.wMinute);
  corefree(buf);
  if (!err)
  { if (stop1<start1) stop1+=3600000;  if (stop1==start1) stop1=start1+1;
    *requests=1000*SPEED_CNT1 / (stop1-start1);
    if (stop2<start2) stop2+=3600000;  if (stop2==start2) stop2=start2+1;
    *kbytes = SPEED_BUFSIZE * SPEED_CNT2 / (stop2-start2);
  }
  SetCursor(hOldCurs);
  return err;
}
#endif
#ifdef UNIX
CFNC DllKernel BOOL WINAPI cd_Connection_speed_test(cdp_t cdp, int * requests, int * kbytes)
#define SPEED_BUFSIZE 20000
#define SPEED_CNT1     2000
#define SPEED_CNT2      600
{ 
  timeval start1, stop1, start2, stop2;  BOOL err=FALSE;  int i;
  *requests=*kbytes=0;
  tptr buf = (tptr)corealloc(SPEED_BUFSIZE, 77);
  if (!buf) { client_error(cdp, OUT_OF_MEMORY);  return TRUE; }
  memset(buf, ' ', SPEED_BUFSIZE-1);  buf[SPEED_BUFSIZE-1]=0;
  gettimeofday(&start1,NULL);  
  BOOL transaction_open;
  for (i=0;  i<SPEED_CNT1;  i++) 
    //if (cd_Start_transaction(cdp)) { err=TRUE;  break; } -- this warns since 8.1
    if (cd_Transaction_open(cdp, &transaction_open)) { err=TRUE;  break; }
  gettimeofday(&stop1,NULL);  
  cd_Roll_back(cdp);
 // cd_SQL_execute(cdp, buf, NULL);
  gettimeofday(&start2,NULL);  
  for (i=0;  i<SPEED_CNT2;  i++)
  { uns32 result;  uns32 psize;
    tptr p=get_space_op(cdp, 1+sizeof(uns32)+SPEED_BUFSIZE, OP_SUBMIT);
    if (p==NULL) { err=TRUE;  break; }
    *(uns32*)p=0;  p+=sizeof(uns32);
    memcpy(p, buf, SPEED_BUFSIZE);
    cdp->ans_array[cdp->ans_ind]=&psize;
    cdp->ans_type [cdp->ans_ind]=sizeof(psize);
    cdp->ans_ind++;
    cdp->ans_array[cdp->ans_ind]=&result;
    cdp->ans_type [cdp->ans_ind]=ANS_SIZE_PREVITEM;
    cdp->ans_ind++;
    if (cond_send(cdp)) { err=TRUE;  break; }
  }
  gettimeofday(&stop2,NULL);  
  corefree(buf);
  if (!err)
  { 
    *requests=((stop1.tv_sec-start1.tv_sec)*1000000+(stop1.tv_usec-start1.tv_usec));
    *kbytes = ((stop2.tv_sec-start2.tv_sec)*1000000+(stop2.tv_usec-start2.tv_usec));
  }
  return err;
}
#endif

CFNC DllKernel BOOL WINAPI cd_Appl_inst_count(cdp_t cdp, uns32 * count)
{ tptr p=get_space_op(cdp, 1+1, OP_CNTRL);
  if (p==NULL) return TRUE;
  *p=SUBOP_APPL_INSTS;
  *count=0;
  cdp->ans_array[cdp->ans_ind]=count;
  cdp->ans_type [cdp->ans_ind]=sizeof(count);
  cdp->ans_ind++;
  return cond_send(cdp);
}

CFNC DllKernel BOOL WINAPI cd_Set_appl_starter(cdp_t cdp, tcateg categ, const char * objname)
{ apx_header apx;
  if (!*cdp->sel_appl_name) return TRUE;
  if (!objname) objname=NULLSTRING;
  memset(&apx, 0, sizeof(apx_header));  apx.version=CURRENT_APX_VERSION;
  if (cd_Read_var(cdp, OBJ_TABLENUM, (trecnum)cdp->applobj, OBJ_DEF_ATR, NOINDEX, 0, sizeof(apx_header), &apx, NULL)) 
    return TRUE;
  strmaxcpy(apx.start_name, objname, sizeof(tobjname));  Upcase9(cdp, apx.start_name);
  apx.start_categ=categ;
  if (cd_Write_var(cdp, OBJ_TABLENUM, (trecnum)cdp->applobj, OBJ_DEF_ATR, NOINDEX, 0, sizeof(apx_header), &apx)) 
    return TRUE;
  strmaxcpy(cdp->appl_starter, objname, sizeof(cdp->appl_starter));  Upcase9(cdp, cdp->appl_starter);
  cdp->appl_starter_categ = categ;
  return FALSE;
}

CFNC DllKernel BOOL WINAPI cd_Get_logged_user(cdp_t cdp, int index, char * username, char * applname, int * state)
{ tptr p=get_space_op(cdp, 1+1+sizeof(tobjnum)+sizeof(uns32), OP_DEBUG);
  if (p==NULL) return TRUE;
  *p=DBGOP_GET_USER;  p++;  *(tobjnum*)p=index;
  REGISTER_ANSWER(username, sizeof(tobjname));
  REGISTER_ANSWER(applname, sizeof(tobjname));
  REGISTER_ANSWER(state,   sizeof(uns32));
  return cond_send(cdp);
}

CFNC DllKernel BOOL WINAPI cd_Server_debug(cdp_t cdp, int oper, tobjnum objnum, int line, int * state, tobjnum * pobjnum, int * pline)
{ tptr p=get_space_op(cdp, 1+1+sizeof(tobjnum)+sizeof(uns32), OP_DEBUG);
  if (p==NULL) return TRUE;
  *p=oper;  p++;  
  *(tobjnum*)p=objnum;  p+=sizeof(tobjnum);
  *(uns32  *)p=line;    p+=sizeof(uns32);
  tobjnum a1;  uns32 a2, a3;
  REGISTER_ANSWER(pobjnum ? pobjnum : &a1, sizeof(tobjnum));
  REGISTER_ANSWER(&a2, sizeof(uns32));
  REGISTER_ANSWER(&a3, sizeof(uns32));
  BOOL res=cond_send(cdp);
  if (pline) *pline=a2;  if (state) *state=a3;
  return res;
}

CFNC DllKernel BOOL WINAPI cd_Server_eval(cdp_t cdp, int frame, const char * expr, void * value)
{ if (!expr || !value)
    { client_error(cdp, ERROR_IN_FUNCTION_CALL);  return TRUE; }
  tptr p=get_space_op(cdp, 1+1+sizeof(uns32)+(unsigned)strlen(expr)+1, OP_DEBUG);
  if (p==NULL) return TRUE;
  *p=DBGOP_EVAL;  p++;  
  *(uns32  *)p=frame;    p+=sizeof(uns32);
  strcpy(p, expr);
  REGISTER_ANSWER(value, ANS_SIZE_VARIABLE);
  return cond_send(cdp);
}

CFNC DllKernel BOOL WINAPI cd_Server_eval9(cdp_t cdp, int frame, const char * expr, wuchar * value)
{ if (!expr || !value)
    { client_error(cdp, ERROR_IN_FUNCTION_CALL);  return TRUE; }
  tptr p=get_space_op(cdp, 1+1+sizeof(uns32)+(unsigned)strlen(expr)+1, OP_DEBUG);
  if (p==NULL) return TRUE;
  *p=DBGOP_EVAL9;  p++;  
  *(uns32  *)p=frame;    p+=sizeof(uns32);
  strcpy(p, expr);
  REGISTER_ANSWER(value, ANS_SIZE_VARIABLE);
  return cond_send(cdp);
}

CFNC DllKernel BOOL WINAPI cd_Server_assign(cdp_t cdp, int frame, const char * dest, const char * value)
{ tptr p=get_space_op(cdp, 1+1+sizeof(uns32)+(unsigned)strlen(dest)+1+(unsigned)strlen(value)+1, OP_DEBUG);
  if (p==NULL) return TRUE;
  *p=DBGOP_ASSIGN;  p++;  
  *(uns32  *)p=frame;    p+=sizeof(uns32);
  strcpy(p, dest);  p+=strlen(p)+1;
  strcpy(p, value);
  return cond_send(cdp);
}

CFNC DllKernel BOOL WINAPI cd_Server_stack(cdp_t cdp, t_stack_info ** value)
{ tptr p=get_space_op(cdp, 1+1, OP_DEBUG);
  if (p==NULL) return TRUE;
  *p=DBGOP_STACK;
  REGISTER_ANSWER(value, ANS_SIZE_DYNALLOC);
  return cond_send(cdp);
}

CFNC DllKernel BOOL WINAPI cd_Server_stack9(cdp_t cdp, t_stack_info9 ** value)
{ tptr p=get_space_op(cdp, 1+1, OP_DEBUG);
  if (p==NULL) return TRUE;
  *p=DBGOP_STACK9;
  REGISTER_ANSWER(value, ANS_SIZE_DYNALLOC);
  return cond_send(cdp);
}

CFNC DllKernel BOOL WINAPI cd_Get_server_info(cdp_t cdp, int info_type, void * buffer, unsigned buffer_size)
{ tptr p=get_space_op(cdp, 1+sizeof(uns32)+1, OP_GENER_INFO);
  if (p==NULL) return TRUE;
  *(uns32*)p=buffer_size;  p+=sizeof(uns32);
  *p=info_type;
  REGISTER_ANSWER(buffer, ANS_SIZE_VARIABLE);
  return cond_send(cdp);  
}

CFNC DllKernel void WINAPI cd_Get_appl_info(cdp_t cdp, WBUUID appl_uuid, char * appl_name, char * server_name)
{ memcpy(appl_uuid,   cdp->sel_appl_uuid, UUID_SIZE);
  strcpy(appl_name,   cdp->sel_appl_name);
  strcpy(server_name, cdp->conn_server_name);
}

CFNC DllKernel BOOL WINAPI cd_Send_fulltext_words(cdp_t cdp, const char * ft_label, t_docid32 docid, unsigned startpos, const char * buf, unsigned bufsize)
{ tptr p=get_space_op(cdp, 1+2*sizeof(OBJ_NAME_LEN)+1+3*sizeof(uns32)+bufsize, OP_FULLTEXT);
  if (p==NULL) return TRUE;
  strcpy(p, ft_label);  p+=2*sizeof(OBJ_NAME_LEN)+1;
  *(uns32*)p=docid;     p+=sizeof(uns32);
  *(uns32*)p=startpos;  p+=sizeof(uns32);
  *(uns32*)p=bufsize;   p+=sizeof(uns32);
  memcpy(p, buf, bufsize);  
  return cond_send(cdp);  
}

CFNC DllKernel BOOL WINAPI cd_Send_fulltext_words64(cdp_t cdp, const char * ft_label, t_docid64 docid, unsigned startpos, const char * buf, unsigned bufsize)
{ uns32 size = bufsize;
  if (bufsize==(unsigned)-1)
    size=(uns32)strlen(buf)+1;
  tptr p=get_space_op(cdp, 1+2*OBJ_NAME_LEN+2+3*sizeof(uns32)+sizeof(uns64)+size, OP_FULLTEXT);
  if (p==NULL) return TRUE;
  strcpy(p, ft_label);  p+=2*OBJ_NAME_LEN+2;
  *(uns32*)p=0xffffffff;  p+=sizeof(uns32);
  *(uns64*)p=docid;     p+=sizeof(uns64);
  *(uns32*)p=startpos;  p+=sizeof(uns32);
  *(uns32*)p=bufsize;   p+=sizeof(uns32);
  memcpy(p, buf, size);  
  return cond_send(cdp);  
}

CFNC DllKernel BOOL WINAPI cd_Get_property_value(cdp_t cdp, const char * owner, const char * name, int num, char * buffer, unsigned buffer_size, int * valsize)
{ int len1=(int)strlen(owner)+1, len2=(int)strlen(name)+1;
  tptr p=get_space_op(cdp, 1+1+len1+len2+2*sizeof(uns32), OP_PROPERTIES);
  if (p==NULL) return TRUE;
  *p=OPER_GET;  p++;
  strcpy(p, owner);  p+=len1;
  strcpy(p, name);   p+=len2;
  *(uns32*)p=num;  p+=sizeof(uns32);
  *(uns32*)p=buffer_size;
  static int psize;  // static, because this API call may be contained in a packet!
  if (!valsize) valsize=&psize;
  else *valsize=0;   // for future clients with 64-bit int and for clients not testing the result
  REGISTER_ANSWER(valsize, sizeof(uns32));
  REGISTER_ANSWER(buffer, ANS_SIZE_PREVITEM);
  return cond_send(cdp);  
}

CFNC DllKernel BOOL WINAPI cd_Set_property_value(cdp_t cdp, const char * owner, const char * name, int num, const char * value, sig32 valsize)
{ int len1=(int)strlen(owner)+1, len2=(int)strlen(name)+1;
  if (valsize==-1) valsize=(int)strlen(value)+1;
  tptr p=get_space_op(cdp, 1+1+len1+len2+2*sizeof(uns32)+valsize, OP_PROPERTIES);
  if (p==NULL) return TRUE;
  *p=OPER_SET;  p++;
  strcpy(p, owner);  p+=len1;
  strcpy(p, name);   p+=len2;
  *(uns32*)p=num;      p+=sizeof(uns32);
  *(uns32*)p=valsize;  p+=sizeof(uns32);
  memcpy(p, value, valsize);  
  return cond_send(cdp);  
}

CFNC DllKernel BOOL WINAPI cd_Admin_mode(cdp_t cdp, tobjnum objnum, BOOL mode)
{ tptr p=get_space_op(cdp, 1+sizeof(ttablenum)+1+1, OP_INDEX);
  if (p==NULL) return TRUE;
  *(ttablenum*)p=objnum;  p+=sizeof(ttablenum);
  *p=OP_OP_ADMIN_MODE; /* suboper. */  p++;
  *p=(uns8)mode;
  uns32 result;
  REGISTER_ANSWER(&result, sizeof(uns32));
  if (cond_send(cdp)) return NONEBOOLEAN;
  return result;
}
///////////////////////////////////////// client events //////////////////////////////////////////////////
CFNC DllKernel BOOL WINAPI cd_Register_event(cdp_t cdp, const char * event_name, const char * param_str, BOOL param_exact, uns32 * event_handle)
{ int len1=(int)strlen(event_name)+1, len2=(int)strlen(param_str)+1;
  tptr p=get_space_op(cdp, 1+1+len1+len2+1, OP_SYNCHRO);
  if (p==NULL) return TRUE;
  *p=OP_SYN_REG;  p++;
  strcpy(p, event_name);  p+=len1;
  strcpy(p, param_str);   p+=len2;
  *(wbbool*)p=(wbbool)param_exact;  
  REGISTER_ANSWER(event_handle, sizeof(uns32));
  return cond_send(cdp);  
}

CFNC DllKernel BOOL WINAPI cd_Unregister_event(cdp_t cdp, uns32 event_handle)
{ tptr p=get_space_op(cdp, 1+1+sizeof(uns32), OP_SYNCHRO);
  if (p==NULL) return TRUE;
  *p=OP_SYN_UNREG;  p++;
  *(uns32*)p=event_handle;  
  return cond_send(cdp);  
}

CFNC DllKernel BOOL WINAPI cd_Cancel_event_wait(cdp_t cdp, uns32 event_handle)
{ tptr p=get_space_op(cdp, 1+1+sizeof(uns32), OP_SYNCHRO);
  if (p==NULL) return TRUE;
  *p=OP_SYN_CANCEL;  p++;
  *(uns32*)p=event_handle;  
  return cond_send(cdp);  
}

CFNC DllKernel BOOL WINAPI cd_Wait_for_event(cdp_t cdp, sig32 timeout, uns32 * event_handle, uns32 * event_count, char * param_str, sig32 param_size, sig32 * result)
{ tptr p=get_space_op(cdp, 1+1+2*sizeof(sig32), OP_SYNCHRO);
  if (p==NULL) return TRUE;
  *p=OP_SYN_WAIT;  p++;
  *(sig32*)p=timeout;  p+=sizeof(sig32);
  *(sig32*)p=param_size;
  *event_handle=0;  *event_count=0;  if (param_size) *param_str='\0';  
  *result=WAIT_EVENT_ERROR;  // this is used e.g. when waiting is cancelled by a break
  REGISTER_ANSWER(event_handle,  sizeof(uns32));
  REGISTER_ANSWER(event_count, sizeof(uns32));
  REGISTER_ANSWER(param_str,   param_size);
  REGISTER_ANSWER(result,      sizeof(sig32));
  return cond_send(cdp);  
}
////////////////////////////////////////// licences for external products ///////////////////////
CFNC DllKernel BOOL WINAPI cd_Store_lic_num(cdp_t cdp, const char * lic, const char * company, const char * computer, char ** request)
{ int len = (int)strlen(lic)+(int)strlen(company)+(int)strlen(computer)+3;
  tptr p=get_space_op(cdp, 1+1+len, OP_CNTRL);
  if (p==NULL) return TRUE;
  *p=SUBOP_LIC1;  p++;
  strcpy(p, lic);  p+=strlen(p)+1;
  strcpy(p, company);  p+=strlen(p)+1;
  strcpy(p, computer);
  REGISTER_ANSWER(request, ANS_SIZE_DYNALLOC);
  return cond_send(cdp);  
}

CFNC DllKernel BOOL WINAPI cd_Store_activation(cdp_t cdp, char * response)
{ tptr p=get_space_op(cdp, 1+1+(unsigned)strlen(response)+1, OP_CNTRL);
  if (p==NULL) return TRUE;
  *p=SUBOP_LIC2;  p++;
  strcpy(p, response);
  return cond_send(cdp);  
}

CFNC DllKernel BOOL WINAPI cd_Park_server_settings(cdp_t cdp)
{ tptr p=get_space_op(cdp, 1+1, OP_CNTRL);
  if (p==NULL) return TRUE;
  *p=SUBOP_PARK_SETTINGS;
  return cond_send(cdp);  
}

CFNC DllKernel BOOL WINAPI cd_Set_lobrefs_export(cdp_t cdp, BOOL refs_only)   
{ tptr p=get_space_op(cdp, 1+1+1, OP_CNTRL);
  if (p==NULL) return TRUE;
  *p=SUBOP_EXPORT_LOBREFS;  p++;
  *(uns8*)p=(uns8)refs_only;  
  return cond_send(cdp);
}

/////////////////////////////// client errors and error translation ////////////////////
#include "csmsgs.cpp"

t_specif wx_string_spec(0, 0, 0,
#ifdef LINUX
  2);
#else  // MSW
  1);
#endif

CFNC DllKernel BOOL WINAPI Get_error_num_text(xcdp_t xcdp, int err, char * buf, unsigned buflen)
// !cdp: local client error, independent of connection.
/* Returns TRUE if any error, FALSE if no error. */
{ 
  if (err==ANS_OK) { *buf=0;  return FALSE; }
 // generic errors: translated and completed on creation 
  if (xcdp)
    if (err >= FIRST_GENERIC_ERROR && err <= LAST_GENERIC_ERROR)
    { if (xcdp->connection_type==CONN_TP_ODBC || xcdp->get_cdp()->last_error_is_from_client)  // message text in available locally
        if (xcdp->generic_error_message)
          strmaxcpy(buf, xcdp->generic_error_message, buflen);
        else if (xcdp->generic_error_messageW)
        { t_specif string_spec_limited(buflen-1, xcdp->get_cdp()->sys_charset, 0, 0);
          if (superconv(ATT_STRING, ATT_STRING, xcdp->generic_error_messageW, buf, wx_string_spec, string_spec_limited, NULL)<0)
            strmaxcpy(buf, "?", buflen);
        }
        else
          strmaxcpy(buf, "?", buflen);
      else // retrieve the message via the "context" concept
      { uns32 line;  uns16 column;
        int saved_err = xcdp->get_cdp()->RV.report_number;
        if (cd_Get_error_info(xcdp->get_cdp(), &line, &column, buf)) *buf=0;
        xcdp->get_cdp()->RV.report_number = saved_err;
      }
      return *buf!=0;
    }
 // get message pattern:
  const char * p;  bool try_kontext=false, add_number=false;
  if      (err >= FIRST_CLIENT_ERROR)
    p=client_errors[err - FIRST_CLIENT_ERROR];
  else if (err >= FIRST_RT_ERROR)
    p=ipjrt_errors [err-FIRST_RT_ERROR];
  else if (err >= FIRST_COMP_ERROR)
  { p=compil_errors[err - FIRST_COMP_ERROR];  try_kontext=true; }
  else if (err >= FIRST_MAIL_ERROR)
    p=mail_errors  [err-FIRST_MAIL_ERROR];
  else if (err==NOT_ANSWERED)
    p=not_answered;
  else if (err > LAST_DB_ERROR)
    p=db_errors    [LAST_DB_ERROR-0x80+1];
  else if (err >= 0x80)
  { p=db_errors    [err-0x80];  add_number=true; }
  else
    p=kse_errors   [err];
#if defined(ENGLISH) || (WBVERS>=900)
 // translate the message (before adding the number, context and substiututing parameters):
  if (global_translation_callback)
  { wchar_t * wbuf = new wchar_t[buflen+1];
    if (wbuf)
    {// conversion to unicode (messages from ODBC drivers may be in the platform language);
      superconv(ATT_STRING, ATT_STRING, p, (char*)wbuf, t_specif(0,1,true,false), wx_string_spec, NULL);
      (*global_translation_callback)(wbuf);
     // convert to system charset:
      superconv(ATT_STRING, ATT_STRING, wbuf, buf, wx_string_spec, t_specif(buflen-1,xcdp ? xcdp->get_cdp()->sys_charset : CHARSET_NUM_UTF8,true,false), NULL);
      delete [] wbuf;
    }
  }
  else
#endif
    strmaxcpy(buf, p, buflen);
 // add message paremeters, if any:
  if (xcdp)
    if (ERR_WITH_MSG(err) || CLIENT_ERR_WITH_MSG(err) || err==FUNCTION_NOT_FOUND || err==DLL_NOT_FOUND)
    { char auxs[200];
      sprintf(auxs, buf, xcdp->get_cdp()->errmsg);
      strmaxcpy(buf, auxs, buflen);
    }
 // adding the error number:
#if WBVERS>=810
  if (add_number)
  { char numstr[20];
    sprintf(numstr, " {%u}", err);
    if (strlen(buf) + strlen(numstr) < buflen) strcat(buf, numstr);
  }
#endif
 // append compilation kontext for compilation errors on the 602SQL server:
  if (try_kontext && xcdp && !xcdp->get_cdp()->last_error_is_from_client)
    if (strlen(buf) + 2+PRE_KONTEXT+POST_KONTEXT+1 < buflen)
    { uns32 line;  uns16 column;
      tptr p = buf+strlen(buf);
      *p='\n';  p++;  *p='\n';  p++;      
      int saved_err = xcdp->get_cdp()->RV.report_number;
      if (cd_Get_error_info(xcdp->get_cdp(), &line, &column, p) || !*p) p[-2]=0;
      xcdp->get_cdp()->RV.report_number = saved_err;
    }
  return *buf != 0;     
}

t_translation_callback * global_translation_callback = NULL;

CFNC DllKernel void WINAPI Set_translation_callback(cdp_t cdp, t_translation_callback * translation_callback)
{ global_translation_callback = translation_callback; }

CFNC DllKernel const wchar_t *Get_transl(cdp_t cdp, const wchar_t *msg, wchar_t *buf)
{ 
  wcscpy(buf, msg);
  if (global_translation_callback)
      (*global_translation_callback)(buf);
  return buf;
}
CFNC DllKernel BOOL WINAPI Get_error_num_textW(xcd_t * xcdp, int err, wchar_t * buf, unsigned buflen)
/* Returns TRUE if any error, FALSE if no error. buflen is in wide characters */
{ 
  if (err==ANS_OK) { *buf=0;  return FALSE; }
 // generic errors: translated and completed on creation 
  if (err >= FIRST_GENERIC_ERROR && err <= LAST_GENERIC_ERROR)
  { if (!xcdp) { *buf=0;  return FALSE; }
    if (xcdp->connection_type==CONN_TP_ODBC || xcdp->get_cdp()->last_error_is_from_client)  // message text in available locally
    { if (xcdp->generic_error_messageW)
	      wcsmaxcpy(buf, xcdp->generic_error_messageW, buflen);
      else if (xcdp->generic_error_message)
      { t_specif wx_string_spec_limited((uns16)((buflen-1)*sizeof(wchar_t)), 0, 0,
#ifdef LINUX
           2);
#else  // MSW
           1);
#endif
        superconv(ATT_STRING, ATT_STRING, xcdp->generic_error_message, (char*)buf, t_specif(0,xcdp && xcdp->get_cdp() ? xcdp->get_cdp()->sys_charset:1,true,false), wx_string_spec_limited, NULL);
      }
      else
        wcsmaxcpy(buf, L"?", buflen);
    }
    else // retrieve the message via the "context" concept
    { uns32 line;  uns16 column;  char buf8[PRE_KONTEXT+1+POST_KONTEXT+1];
      int saved_err = xcdp->get_cdp()->RV.report_number;
      if (cd_Get_error_info(xcdp->get_cdp(), &line, &column, buf8)) *buf8=0;  // in system charset
      xcdp->get_cdp()->RV.report_number = saved_err;
      superconv(ATT_STRING, ATT_STRING, buf8, (char*)buf, t_specif(0,xcdp->get_cdp()->sys_charset,true,false), wx_string_spec, NULL);
    }
    return *buf!=0;
  }	
 // get message pattern (in english, 8-bit characters):
  const char * p;  bool try_kontext=false, add_number=false;
  if      (err >= FIRST_CLIENT_ERROR)
    p=client_errors[err - FIRST_CLIENT_ERROR];
  else if (err >= FIRST_RT_ERROR)
    p=ipjrt_errors [err-FIRST_RT_ERROR];
  else if (err >= FIRST_COMP_ERROR)
  { p=compil_errors[err - FIRST_COMP_ERROR];  try_kontext=true; }
  else if (err >= FIRST_MAIL_ERROR)
    p=mail_errors  [err-FIRST_MAIL_ERROR];
  else if (err==NOT_ANSWERED)
    p=not_answered;
  else if (err > LAST_DB_ERROR)
    p=db_errors    [LAST_DB_ERROR-0x80+1];
  else if (err >= 0x80)
  { p=db_errors    [err-0x80];  add_number=true; }
  else
    p=kse_errors   [err];
 // simple conversion to unicode:	
  int i=0;
  while (p[i] && i+1<buflen)
    { buf[i]=p[i];  i++; }
  buf[i]=0;	
 // translate the message (before adding the number, context and substituting parameters):
  if (global_translation_callback)
    (*global_translation_callback)(buf);
 // replace %s by %ls (must be after translation):
  i=0;  
  while (buf[i])
  { if (buf[i]=='%' && buf[i+1]=='s')
    { memmov(buf+i+2, buf+i+1, sizeof(wchar_t)*(wcslen(buf+i+1)+1));
      buf[i+1]='l';  
    }
    i++;
  }
 // add message paremeters, if any:
  if (ERR_WITH_MSG(err) || CLIENT_ERR_WITH_MSG(err) || err==FUNCTION_NOT_FOUND || err==DLL_NOT_FOUND)
    if (xcdp)
    { wchar_t auxs[200];  wchar_t wparam[OBJ_NAME_LEN+1];
      superconv(ATT_STRING, ATT_STRING, xcdp->get_cdp()->errmsg, (char*)wparam, t_specif(0,xcdp->get_cdp()->sys_charset,true,false), wx_string_spec, NULL);
      swprintf(auxs, 
#ifdef LINUX
               200,
#endif			    	
	           buf, wparam);  // message pattern contains %s and errmsg is 8-bit
      wcsmaxcpy(buf, auxs, buflen);
    }
 // adding the error number:
#if WBVERS>=810
  if (add_number)
  { wchar_t numstr[20];
    swprintf(numstr, 
#ifdef LINUX
             20,
#endif			    	
	         L" {%u}", err);
    if (wcslen(buf) + wcslen(numstr) < buflen) wcscat(buf, numstr);
  }
#endif
 // kontext:
  if (try_kontext && xcdp && !xcdp->get_cdp()->last_error_is_from_client)
  { uns32 line;  uns16 column;  char buf8[PRE_KONTEXT+1+POST_KONTEXT+1];
    int saved_err = xcdp->get_cdp()->RV.report_number;
    if (cd_Get_error_info(xcdp->get_cdp(), &line, &column, buf8)) *buf8=0;  // in system charset
    xcdp->get_cdp()->RV.report_number = saved_err;
    if (*buf8 && wcslen(buf) + 2 + strlen(buf8) < buflen)
    { wchar_t * p = buf+wcslen(buf);
      *p='\n';  p++;  *p='\n';  p++;      
      superconv(ATT_STRING, ATT_STRING, buf8, (char*)p, t_specif(0,xcdp->get_cdp()->sys_charset,true,false), wx_string_spec, NULL);
    }
  }
  return *buf != 0;     
}

CFNC DllKernel void WINAPI Convert_error_to_generic(cdp_t cdp)
{ int err = cd_Sz_error(cdp);
  if (err!=ANS_OK)
  { wchar_t buf[500];
    Get_error_num_textW(cdp, err, buf, sizeof(buf) / sizeof(wchar_t));
    client_error_genericW(cdp, GENERR_CONVERTED, buf);
  }
}
    
CFNC DllKernel BOOL WINAPI Get_warning_num_text(cdp_t cdp, int wrnnum, char * buf, unsigned buflen)
{ 
  int ind = 0;
  while (warning_messages[ind]!=wrnnum && warning_messages[ind]) 
  { ind++;
    if (!warning_messages[ind]) { *buf=0;  return FALSE; }
  }
  strmaxcpy(buf, db_warnings[ind], buflen);
  return TRUE;
}

CFNC DllKernel BOOL WINAPI Get_warning_num_textW(cdp_t cdp, int wrnnum, wchar_t * buf, unsigned buflen)
{ 
  int ind = 0;
  while (warning_messages[ind]!=wrnnum && warning_messages[ind]) 
  { ind++;
    if (!warning_messages[ind]) { *buf=0;  return FALSE; }
  }
  const char * p=db_warnings[ind];
 // convert to wchar_t
  int i=0;
  while (p[i] && i+1<buflen)
    { buf[i]=p[i];  i++; }
  buf[i]=0;	
 // translate the message:
  if (global_translation_callback)
    (*global_translation_callback)(buf);
  return TRUE;
}

t_client_error_callback * client_error_callback = NULL;  // global, used only by single-thread clients for global reporting

CFNC DllKernel void WINAPI set_client_error_callback(t_client_error_callback * client_error_callbackIn)
{ client_error_callback=client_error_callbackIn; }

CFNC DllKernel void WINAPI set_client_error_callback2(cdp_t cdp, t_client_error_callback2 * client_error_callback2In)
{ cdp->client_error_callback2=client_error_callback2In; }

CFNC DllKernel void WINAPI set_client_error_callbackW(cdp_t cdp, t_client_error_callbackW * client_error_callbackWIn)
{ cdp->client_error_callbackW=client_error_callbackWIn; }

CFNC DllKernel void WINAPI client_error_generic(xcd_t * xcdp, int errnum, const char * msg)
{ if (!xcdp) return;  // must not be NULL, client_error_param() or client_error_message() should be used instead!
  if (xcdp->generic_error_message) corefree(xcdp->generic_error_message);
  xcdp->generic_error_message=(char*)corealloc(strlen(msg)+1, 72);
  if (xcdp->generic_error_messageW) { corefree(xcdp->generic_error_messageW);  xcdp->generic_error_messageW=NULL; }
  if (!xcdp->generic_error_message) return;  // error lost
  strcpy(xcdp->generic_error_message, msg);
  client_error(xcdp, errnum);                             
}

CFNC DllKernel void WINAPI client_error_genericW(xcd_t * xcdp, int errnum, const wchar_t * msg)
{ if (!xcdp) return;  // must not be NULL, client_error_param() or client_error_message() should be used instead!
  if (xcdp->generic_error_messageW) corefree(xcdp->generic_error_messageW);
  xcdp->generic_error_messageW=(wchar_t*)corealloc((wcslen(msg)+1)*sizeof(wchar_t), 72);
  if (xcdp->generic_error_message) { corefree(xcdp->generic_error_message);  xcdp->generic_error_message=NULL; }
  if (!xcdp->generic_error_messageW) return;  // error lost
  wcscpy(xcdp->generic_error_messageW, msg);
  client_error(xcdp, errnum);
}

CFNC DllKernel void WINAPI client_error(xcd_t * xcdp, int num)
// [cdp] may be NULL for client environment errors.
{// store the error number 
  if (xcdp && xcdp->connection_type==CONN_TP_602)  // this function can be called before connecting to the server
  { xcdp->get_cdp()->RV.report_number=num;  
    xcdp->get_cdp()->last_error_is_from_client=wbtrue; 
  }
 // write error text to the log via the callback:
  if (num && num!=GENERR_CONVERTED) // unless clearing a previous error (wprint)
  { 
    if (xcdp && xcdp->connection_type==CONN_TP_602 && xcdp->get_cdp()->client_error_callback2 || client_error_callback)
    { char buf[256];
      Get_error_num_text(xcdp, num, buf, sizeof(buf));
      if (xcdp && xcdp->connection_type==CONN_TP_602 && xcdp->get_cdp()->client_error_callback2)
        (*xcdp->get_cdp()->client_error_callback2)(xcdp->get_cdp(), buf);
      else
        (*client_error_callback)(buf);
    }
    if (xcdp && xcdp->connection_type==CONN_TP_602 && xcdp->get_cdp()->client_error_callbackW)
    { wchar_t buf[256];
      Get_error_num_textW(xcdp, num, buf, sizeof(buf)/sizeof(wchar_t));
      (*xcdp->get_cdp()->client_error_callbackW)(xcdp->get_cdp(), buf);
    }
  }	
}

CFNC DllKernel void client_error_param(cdp_t cdp, int num, ...)
{ cdp->RV.report_number=num;  
  cdp->last_error_is_from_client=wbtrue; 
  if (cdp && cdp->client_error_callback2 || client_error_callback)
  { char buf1[256], buf2[300];   va_list vl;
    Get_error_num_text(cdp, num, buf1, sizeof(buf1));
    va_start(vl, num);
    tptr p1= va_arg(vl, tptr);
    tptr p2= va_arg(vl, tptr);
    tptr p3= va_arg(vl, tptr);
    va_end(vl);
    snprintf (buf2, sizeof(buf2), buf1, p1, p2, p3);
    buf2[sizeof(buf2)-1]=0;  // on overflow the terminator is NOT written!
    if (cdp && cdp->client_error_callback2)
      (*cdp->client_error_callback2)(cdp, buf2);
    else
      (*client_error_callback)(buf2);
  }
}

CFNC DllKernel void client_error_message(cdp_t cdp, const char * msg, ...)
// Writes a parameterized message to the client error log
{ char buf[300];   va_list vl;
  va_start(vl, msg);
  tptr p1= va_arg(vl, tptr);
  tptr p2= va_arg(vl, tptr);
  tptr p3= va_arg(vl, tptr);
  va_end(vl);
  snprintf(buf, sizeof(buf), msg, p1, p2, p3);
  buf[sizeof(buf)-1]=0;  // on overflow the terminator is NOT written!
  if (cdp && cdp->client_error_callback2)
    (*cdp->client_error_callback2)(cdp, buf);
  else if (client_error_callback) 
    (*client_error_callback)(buf);
#ifdef WINS
  else
  { char fname[MAX_PATH];
    strcpy(fname /*+ GetTempPath(sizeof(fname), fname)*/, "c:\\wbclient.log");
    FHANDLE hnd=CreateFile(fname, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, FILE_FLAG_RANDOM_ACCESS, (HANDLE)NULL);
    if (FILE_HANDLE_VALID(hnd)) 
    { DWORD wr;  
      SetFilePointer(hnd, 0, NULL, FILE_END);  
      strcat(buf, "\r\n");
      WriteFile(hnd, buf, (int)strlen(buf), &wr, NULL);
      CloseFile(hnd);
    }
  }
#endif
}

/////////////////////////////// cd support /////////////////////////////////////////////
CFNC DllKernel cdp_t WINAPI cdp_alloc(void)
{ return (cdp_t)malloc(sizeof(cd_t)); }
CFNC DllKernel void  WINAPI cdp_free(cdp_t cdp)
{ free(cdp); }
CFNC DllKernel sig32 WINAPI cdp_size(void)
{ return sizeof(cd_t); }


/********************** functions without cdp parameter *******************/

CFNC DllKernel int WINAPI Sz_error(void)
{ return cd_Sz_error(GetCurrTaskPtr()); }

CFNC DllKernel int WINAPI Sz_warning(void)
{ return cd_Sz_warning(GetCurrTaskPtr()); }

CFNC DllKernel BOOL WINAPI Login(const char * username, const char * password)
{ return cd_Login(GetCurrTaskPtr(), username, password); }

CFNC DllKernel BOOL WINAPI Logout(void)
{ return cd_Logout(GetCurrTaskPtr()); }

CFNC DllKernel const char * WINAPI Who_am_I(void)
{ return cd_Who_am_I(GetCurrTaskPtr()); }

CFNC DllKernel BOOL WINAPI Start_transaction(void)
{ return cd_Start_transaction(GetCurrTaskPtr()); }

CFNC DllKernel BOOL WINAPI Commit(void)
{ return cd_Commit(GetCurrTaskPtr()); }

CFNC DllKernel BOOL WINAPI Roll_back(void)
{ return cd_Roll_back(GetCurrTaskPtr()); }

CFNC DllKernel BOOL WINAPI Close_cursor    (tcursnum curs)
{ return cd_Close_cursor(GetCurrTaskPtr(), curs); }

CFNC DllKernel BOOL WINAPI Read_lock_record  (tcurstab curs, trecnum position)
{ return cd_Read_lock_record(GetCurrTaskPtr(), curs, position); }
CFNC DllKernel BOOL WINAPI Read_unlock_record(tcurstab curs, trecnum position)
{ return cd_Read_unlock_record(GetCurrTaskPtr(), curs, position); }
CFNC DllKernel BOOL WINAPI Read_lock_table  (tcurstab curs)
{ return cd_Read_lock_table(GetCurrTaskPtr(), curs); }
CFNC DllKernel BOOL WINAPI Read_unlock_table(tcurstab curs)
{ return cd_Read_unlock_table(GetCurrTaskPtr(), curs); }
CFNC DllKernel BOOL WINAPI Write_lock_record  (tcurstab curs, trecnum position)
{ return cd_Write_lock_record(GetCurrTaskPtr(), curs, position); }
CFNC DllKernel BOOL WINAPI Write_unlock_record(tcurstab curs, trecnum position)
{ return cd_Write_unlock_record(GetCurrTaskPtr(), curs, position); }
CFNC DllKernel BOOL WINAPI Write_lock_table  (tcurstab curs)
{ return cd_Write_lock_table(GetCurrTaskPtr(), curs); }
CFNC DllKernel BOOL WINAPI Write_unlock_table(tcurstab curs)
{ return cd_Write_unlock_table(GetCurrTaskPtr(), curs); }

CFNC DllKernel BOOL WINAPI Delete(tcurstab  curs,  trecnum position)
{ return cd_Delete(GetCurrTaskPtr(), curs, position); }

CFNC DllKernel BOOL WINAPI Undelete(tcursnum cursor, trecnum position)
{ return cd_Undelete(GetCurrTaskPtr(), cursor, position); }

CFNC DllKernel BOOL WINAPI Read  (tcurstab curs, trecnum position, tattrib attr, const modifrec * access, void * buffer)
{ return cd_Read(GetCurrTaskPtr(), curs, position, attr, access, buffer); }

CFNC DllKernel BOOL WINAPI Write (tcurstab curs, trecnum position, tattrib attr,
                          const modifrec * access, const void * buffer, uns32 datasize)
{ return cd_Write(GetCurrTaskPtr(), curs, position, attr, access, buffer, datasize);  }

CFNC DllKernel trecnum WINAPI Insert(tcurstab curs)
{ return cd_Insert(GetCurrTaskPtr(), curs); }

CFNC DllKernel BOOL WINAPI Rec_cnt(tcurstab curs, trecnum * recnum)
{ return cd_Rec_cnt(GetCurrTaskPtr(), curs, recnum); }

CFNC DllKernel BOOL WINAPI Set_application(const char * applname)
{ return cd_Set_application(GetCurrTaskPtr(), applname); }

CFNC DllKernel BOOL WINAPI Relist_objects(void)
{ return cd_Relist_objects(GetCurrTaskPtr()); }

CFNC DllKernel BOOL WINAPI Read_record (tcursnum cursor, trecnum position, void * buf, uns32 datasize)
{ return cd_Read_record (GetCurrTaskPtr(), cursor, position, buf, datasize); }

CFNC DllKernel BOOL WINAPI Write_record(tcursnum cursor, trecnum position, const void * buf, uns32 datasize)
{ return cd_Write_record(GetCurrTaskPtr(), cursor, position, buf, datasize); }

CFNC DllKernel BOOL WINAPI SQL_execute(const char * statement, uns32 * results)
{ return cd_SQL_execute(GetCurrTaskPtr(), statement, results); }
CFNC DllKernel BOOL WINAPI SQL_prepare(const char * statement, uns32 * handle)
{ return cd_SQL_prepare(GetCurrTaskPtr(), statement, handle); }
CFNC DllKernel BOOL WINAPI SQL_exec_prepared(uns32 handle, uns32 * results, int * count)
{ return cd_SQL_exec_prepared(GetCurrTaskPtr(), handle, results, count); }
CFNC DllKernel BOOL WINAPI SQL_drop(uns32 handle)
{ return cd_SQL_drop(GetCurrTaskPtr(), handle); }

CFNC DllKernel BOOL WINAPI Write_len(tcursnum cursnum, trecnum position, tattrib attr, t_mult_size index, t_varcol_size size)
{ return cd_Write_len(GetCurrTaskPtr(), cursnum, position, attr, index, size); }
CFNC DllKernel BOOL WINAPI Read_len (tcursnum cursnum, trecnum position, tattrib attr, t_mult_size index, t_varcol_size *size)
{ return cd_Read_len(GetCurrTaskPtr(), cursnum, position, attr, index, size); }
CFNC DllKernel BOOL WINAPI Read_ind(tcursnum cursnum, trecnum position,
  tattrib attr, t_mult_size index, void * data)
{ return cd_Read_ind(GetCurrTaskPtr(), cursnum, position, attr, index, data); }
CFNC DllKernel BOOL WINAPI Write_ind(tcursnum cursnum, trecnum position,
  tattrib attr, t_mult_size index, void * data, uns32 datasize)
{ return cd_Write_ind(GetCurrTaskPtr(), cursnum, position, attr, index, data, datasize); }
CFNC DllKernel BOOL WINAPI Read_ind_cnt(tcursnum cursnum, trecnum position,
  tattrib attr, t_mult_size * count)
{ return cd_Read_ind_cnt(GetCurrTaskPtr(), cursnum, position, attr, count); }
CFNC DllKernel BOOL WINAPI Write_ind_cnt(tcursnum cursnum, trecnum position,
  tattrib attr, t_mult_size count)
{ return cd_Write_ind_cnt(GetCurrTaskPtr(), cursnum, position, attr, count); }

CFNC DllKernel BOOL WINAPI Read_var(tcursnum cursnum,
  trecnum position, tattrib attr, t_mult_size index, t_varcol_size start, t_varcol_size size,
  void * buf, t_varcol_size * psize)
{ return cd_Read_var(GetCurrTaskPtr(), cursnum, position, attr, index, start, size, buf, psize); }

CFNC DllKernel BOOL WINAPI Write_var(tcursnum cursnum,
  trecnum position, tattrib attr, t_mult_size index, t_varcol_size start, t_varcol_size size,
  const void * buf)
{ return cd_Write_var(GetCurrTaskPtr(), cursnum, position, attr, index, start, size, buf); }

CFNC DllKernel BOOL WINAPI Get_error_info(uns32 * line, uns16 * column, tptr buf)
{ return cd_Get_error_info(GetCurrTaskPtr(), line, column, buf); }

CFNC DllKernel BOOL WINAPI Break(void)
{ return cd_Break(GetCurrTaskPtr()); }

CFNC DllKernel BOOL WINAPI Find2_object(const char * name, const uns8 * appl_id, tcateg category, tobjnum * position)
{ return cd_Find2_object(GetCurrTaskPtr(), name, appl_id, category, position); }

CFNC DllKernel BOOL WINAPI Find_object(const char * name, tcateg category, tobjnum * position)
{ return cd_Find_object(GetCurrTaskPtr(), name, category, position); }

CFNC DllKernel void WINAPI send_package(void)
{ cd_send_package(GetCurrTaskPtr()); }

CFNC DllKernel trecnum WINAPI Append(tcursnum curs)
{ return cd_Append(GetCurrTaskPtr(), curs); }

CFNC DllKernel BOOL WINAPI answered(void)
{ return cd_answered(GetCurrTaskPtr()); }

CFNC DllKernel void WINAPI concurrent(BOOL state)
{ cd_concurrent(GetCurrTaskPtr(), state); }

CFNC DllKernel BOOL WINAPI waiting(sig32 timeout)
{ return cd_waiting(GetCurrTaskPtr(), timeout); }

CFNC DllKernel void WINAPI start_package(void)
{ cd_start_package(GetCurrTaskPtr()); }

CFNC DllKernel BOOL WINAPI Find_object_by_id(const WBUUID uuid, tcateg category, tobjnum * position)
{ return cd_Find_object_by_id(GetCurrTaskPtr(), uuid, category, position); }

CFNC DllKernel sig32 WINAPI Used_memory(BOOL local)
{ return cd_Used_memory(GetCurrTaskPtr(), local); }

CFNC DllKernel int WINAPI Owned_cursors(void)
{ return cd_Owned_cursors(GetCurrTaskPtr()); }

CFNC DllKernel BOOL WINAPI Open_cursor(tobjnum cursor, tcursnum * curnum)
{ return cd_Open_cursor(GetCurrTaskPtr(), cursor, curnum); }

CFNC DllKernel BOOL WINAPI Open_subcursor(tcursnum super, const char * subcurdef, tcursnum * curnum)
{ return cd_Open_subcursor(GetCurrTaskPtr(), super, subcurdef, curnum); }

CFNC DllKernel BOOL WINAPI Free_deleted(ttablenum tb)
{ return cd_Free_deleted(GetCurrTaskPtr(), tb); }

CFNC DllKernel BOOL WINAPI Enable_index(ttablenum tb, int which, BOOL enable)
{ return cd_Enable_index(GetCurrTaskPtr(), tb, which, enable); }

CFNC DllKernel BOOL WINAPI Translate(tcursnum curs, trecnum crec, int tbord, trecnum * trec)
{ return cd_Translate(GetCurrTaskPtr(), curs, crec, tbord, trec); }

CFNC DllKernel BOOL WINAPI Set_password(const char * username, const char * password)
{ return cd_Set_password(GetCurrTaskPtr(), username, password); }

CFNC DllKernel BOOL WINAPI Insert_object(const char * name, tcateg category, tobjnum * objnum)
{ return cd_Insert_object(GetCurrTaskPtr(), name, category, objnum); }

CFNC DllKernel BOOL WINAPI Create_group(const char * name) // obsolete
{ tobjnum objnum;
  if (!Find_object(name, CATEG_GROUP, &objnum)) return TRUE;
  return Insert_object(name, CATEG_GROUP, &objnum);
}

CFNC DllKernel BOOL WINAPI Create_user(const char * logname,
  const char * name1, const char * name2, const char * name3,
  const char * identif,  const WBUUID homesrv,
  const char * password, tobjnum * objnum)
{ return cd_Create_user(GetCurrTaskPtr(), logname, name1, name2, name3,
               identif, homesrv, password, objnum);
}

CFNC DllKernel BOOL WINAPI GetSet_privils(
      tobjnum user_group_role, tcateg subject_categ, ttablenum table,
      trecnum record, t_oper operation, uns8 * privils)
{ return cd_GetSet_privils(GetCurrTaskPtr(),
      user_group_role, subject_categ, table, record, operation, privils);
}

CFNC DllKernel BOOL WINAPI GetSet_group_role(tobjnum user_or_group,
  tobjnum group_or_role, tcateg subject2, t_oper operation, uns32 * relation)
{ return cd_GetSet_group_role(GetCurrTaskPtr(), user_or_group,
  group_or_role, subject2, operation, relation);
}

CFNC DllKernel BOOL WINAPI Add_record(tcursnum curs, trecnum * recs, int numofrecs)
{ return cd_Add_record(GetCurrTaskPtr(), curs, recs, numofrecs); }

CFNC DllKernel BOOL WINAPI Super_recnum(tcursnum subcursor, tcursnum supercursor,
              trecnum subrecnum, trecnum * superrecnum)
{ return cd_Super_recnum(GetCurrTaskPtr(), subcursor, supercursor,
              subrecnum, superrecnum);
}

CFNC DllKernel BOOL WINAPI Open_cursor_direct(const char * query, tcursnum * curnum)
{ return cd_Open_cursor_direct(GetCurrTaskPtr(), query, curnum); }

CFNC DllKernel BOOL WINAPI Database_integrity(BOOL repair,
           uns32 * lost_blocks, uns32 * lost_dheap,
           uns32 * nonex_blocks, uns32 * cross_link, uns32 * damaged_tabdef)
{ return cd_Database_integrity(GetCurrTaskPtr(), repair,
       lost_blocks, lost_dheap, nonex_blocks, cross_link, damaged_tabdef);
}

CFNC DllKernel BOOL WINAPI Repl_control(int optype, int opparsize, void * opparam)
{ return cd_Repl_control(GetCurrTaskPtr(), optype, opparsize, opparam); }

CFNC DllKernel BOOL WINAPI Replicate(const char * ServerName, const char * ApplName, BOOL pull)
{ return cd_Replicate(GetCurrTaskPtr(), ServerName, ApplName, pull); }

CFNC DllKernel BOOL WINAPI Skip_repl(const char * ServerName, const char * ApplName)
{ return cd_Skip_repl(GetCurrTaskPtr(), ServerName, ApplName); }

CFNC DllKernel BOOL WINAPI GetSet_fil_size(t_oper operation, uns32 * size)
{ return cd_GetSet_fil_size(GetCurrTaskPtr(), operation, size); }

CFNC DllKernel BOOL WINAPI GetSet_fil_blocks(t_oper operation, uns32 * size)
{ return cd_GetSet_fil_blocks(GetCurrTaskPtr(), operation, size); }

CFNC DllKernel BOOL WINAPI Reset_replication(void)
{ return cd_Reset_replication(GetCurrTaskPtr()); }

CFNC DllKernel trecnum WINAPI Look_up(tcursnum curs, const char * attrname, void * res)
{ return cd_Look_up(GetCurrTaskPtr(), curs, attrname, res); }

CFNC DllKernel BOOL WINAPI C_sum(tcursnum curs, const char * attrname,
  const char * condition, void * result)
{ return cd_C_sum(GetCurrTaskPtr(), curs, attrname, condition, result); }

CFNC DllKernel BOOL WINAPI C_avg(tcursnum curs, const char * attrname,
  const char * condition, void * result)
{ return cd_C_avg(GetCurrTaskPtr(), curs, attrname, condition, result); }

CFNC DllKernel BOOL WINAPI C_max(tcursnum curs, const char * attrname,
  const char * condition, void * result)
{ return cd_C_max(GetCurrTaskPtr(), curs, attrname, condition, result); }

CFNC DllKernel BOOL WINAPI C_min(tcursnum curs, const char * attrname,
  const char * condition, void * result)
{ return cd_C_min(GetCurrTaskPtr(), curs, attrname, condition, result); }

CFNC DllKernel BOOL WINAPI C_count(tcursnum curs, const char * attrname,
  const char * condition, trecnum * result)
{ return cd_C_count(GetCurrTaskPtr(), curs, attrname, condition, result); }

CFNC DllKernel BOOL WINAPI Delete_all_records(tcursnum curs)
{ return cd_Delete_all_records(GetCurrTaskPtr(), curs); }

CFNC DllKernel BOOL WINAPI Am_I_db_admin(void)
{ return cd_Am_I_db_admin(GetCurrTaskPtr()); }

CFNC DllKernel void WINAPI Enable_task_switch(BOOL enable)
{ cd_Enable_task_switch(GetCurrTaskPtr(), enable); }

CFNC DllKernel BOOL WINAPI Set_sql_option(uns32 optmask, uns32 optval)
{ return cd_Set_sql_option(GetCurrTaskPtr(), optmask, optval); }

CFNC DllKernel BOOL WINAPI Get_sql_option(uns32 * optval)
{ return cd_Get_sql_option(GetCurrTaskPtr(), optval); }

CFNC DllKernel BOOL WINAPI Compact_table(ttablenum table)
{ return cd_Compact_table(GetCurrTaskPtr(), table); }

CFNC DllKernel BOOL WINAPI Compact_database(int margin)
{ return cd_Compact_database(GetCurrTaskPtr(), margin); }

CFNC DllKernel BOOL WINAPI Log_write(const char * text)
{ return cd_Log_write(GetCurrTaskPtr(), text); }

CFNC DllKernel BOOL WINAPI Set_transaction_isolation_level(t_isolation level)
{ return cd_Set_transaction_isolation_level(GetCurrTaskPtr(), level); }

CFNC DllKernel BOOL WINAPI Appl_inst_count(uns32 * count)
{ return cd_Appl_inst_count(GetCurrTaskPtr(), count); }

#ifdef WINS
CFNC DllKernel BOOL WINAPI Connection_speed_test(int * requests, int * kbytes)
{ return cd_Connection_speed_test(GetCurrTaskPtr(), requests, kbytes); }
#endif

CFNC DllKernel BOOL WINAPI Set_appl_starter(tcateg categ, const char * objname)
{ return cd_Set_appl_starter(GetCurrTaskPtr(), categ, objname); }

CFNC DllKernel BOOL WINAPI Get_logged_user(int index, char * username, char * applname, int * state)
{ return cd_Get_logged_user(GetCurrTaskPtr(), index, username, applname, state); }

CFNC DllKernel BOOL WINAPI Set_progress_report_modulus(unsigned modulus)
{ return cd_Set_progress_report_modulus(GetCurrTaskPtr(), modulus); }

CFNC DllKernel BOOL WINAPI Message_to_clients(const char * msg)
{ return cd_Message_to_clients(GetCurrTaskPtr(), msg); }

CFNC DllKernel BOOL WINAPI Get_server_info(int info_type, void * buffer, unsigned buffer_size)
{ return cd_Get_server_info(GetCurrTaskPtr(), info_type, buffer, buffer_size); }

CFNC DllKernel BOOL WINAPI Write_record_ex(tcurstab curs, trecnum position, unsigned colcount, const t_column_val_descr * colvaldescr)
{ return cd_Write_record_ex(GetCurrTaskPtr(), curs, position, colcount, colvaldescr); }

CFNC DllKernel BOOL WINAPI Insert_record_ex(tcurstab curs, trecnum * position, unsigned colcount, const t_column_val_descr * colvaldescr)
{ return cd_Insert_record_ex(GetCurrTaskPtr(), curs, position, colcount, colvaldescr); }

CFNC DllKernel BOOL WINAPI Append_record_ex(tcurstab curs, trecnum * position, unsigned colcount, const t_column_val_descr * colvaldescr)
{ return cd_Append_record_ex(GetCurrTaskPtr(), curs, position, colcount, colvaldescr); }

CFNC DllKernel BOOL WINAPI Get_property_value(const char * owner, const char * name, int num, char * buffer, unsigned buffer_size, int * valsize)
{ return cd_Get_property_value(GetCurrTaskPtr(), owner, name, num, buffer, buffer_size, valsize); }

CFNC DllKernel BOOL WINAPI Set_property_value(const char * owner, const char * name, int num, const char * value, sig32 valsize)
{ return cd_Set_property_value(GetCurrTaskPtr(), owner, name, num, value, valsize); }

CFNC DllKernel void WINAPI Invalidate_cached_table_info(void)
{ cd_Invalidate_cached_table_info(GetCurrTaskPtr()); }

CFNC DllKernel BOOL WINAPI Uninst_table(ttablenum tb)
{ return cd_Uninst_table(GetCurrTaskPtr(), tb); }

CFNC DllKernel BOOL WINAPI Lock_kernel(void)
{ return cd_Lock_server(GetCurrTaskPtr()); }

CFNC DllKernel void WINAPI Unlock_kernel(void)
{ cd_Unlock_server(GetCurrTaskPtr()); }

CFNC DllKernel BOOL WINAPI Kill_user(uns16 usernum)
{ return cd_Kill_user(GetCurrTaskPtr(), usernum); }

CFNC DllKernel BOOL WINAPI Remove_lock(lockinfo * llock)
{ return cd_Remove_lock(GetCurrTaskPtr(), llock); }

static int HostCharSet = -1;

CFNC DllKernel int GetHostCharSet(void)
{
    if (HostCharSet == -1)
        HostCharSet = wbcharset_t::get_host_charset().get_code();
    return HostCharSet;
}

const char *FromUnicode(const wchar_t *Src, char *Buf, int Size)
{
    superconv(ATT_STRING, ATT_STRING, Src, Buf, wx_string_spec, t_specif(Size - 1, GetHostCharSet(), false, false), NULL);
    return Buf;
}

const char *FromUnicode(cdp_t cdp, const wchar_t *Src, char *Buf, int Size)
{
    superconv(ATT_STRING, ATT_STRING, Src, Buf, wx_string_spec, t_specif(Size - 1, cdp ? cdp->sys_charset :  GetHostCharSet(), false, false), NULL);
    return Buf;
}

const wchar_t *ToUnicode(const char *Src, wchar_t *Buf, int Size)
{
    superconv(ATT_STRING, ATT_STRING, Src, (char *)Buf, t_specif(0, GetHostCharSet(), false, false), wx_string_spec, NULL);
    return Buf;
}

const wchar_t *ToUnicode(cdp_t cdp, const char *Src, wchar_t *Buf, int Size)
{
    superconv(ATT_STRING, ATT_STRING, Src, (char *)Buf, t_specif(0, cdp ? cdp->sys_charset :  GetHostCharSet(), false, false), wx_string_spec, NULL);
    return Buf;
}

//#ifndef WINS  // client needs only the Linux replacements
#include "win32ux.cpp"
//#endif

#if WBVERS>=1000  // moved to cint in 10.0 from kerbase/other
#include <stdlib.h>
#include <locale.h>
#include <malloc.h>
#include "wbmail.cpp"
#include "dynar.cpp"
#include "flstr.h"
#include "flstr.cpp"
#endif
#include "dynarbse.cpp"

/////////////////////////////////// library initialisation ////////////////////////////////////////
CRITICAL_SECTION cs_short_term;

void global_objects_init(void)
{
  InitializeCriticalSection(&cs_short_term);
}

void global_objects_deinit(void)
{
  DeleteCriticalSection(&cs_short_term);
}

class t_global_init
{ int is_init;
 public:
  t_global_init(void);
  ~t_global_init(void);
};

t_global_init::t_global_init(void)
{
  global_objects_init();
  compiler_init();
  is_init=1;  // just for debugging purposes
}

t_global_init::~t_global_init(void)
{
  global_objects_deinit();
  is_init=0;  // just for debugging purposes
}

t_global_init global_init;  // static constructor of this variable performs the once-per-library initialization
