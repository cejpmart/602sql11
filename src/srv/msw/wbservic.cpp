// WBServic.cpp - WinBase602 NT server service code
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
#include <windowsx.h>
#include "nlmtexts.h"

#include "enumsrv.c"

#if defined(_DEBUG) && !defined(X64ARCH)
#include "stackwalk.cpp"
#endif

#if WBVERS<=1000
#define ANY_SERVER_GUI
#define SMART_GUI 
#endif

// Stopping the service under the Terminal Server via WM_QUIT does not work.
// "stopping" flag and timer used instead
#ifdef ANY_SERVER_GUI
#include "server.h"
#include "srvctrl.cpp"

void stop_the_server(void)  // stopping request from a connected client
{ PostMessage(hwMain, WM_ENDSESSION, TRUE, 0); }

//////////////////////////////// watching logon ////////////////////////////
#ifdef SMART_GUI 

static BOOL user_logout = FALSE;
static DWORD logout_time;
void open_service_gui(void);

BOOL any_user_logged(void)
{ HWND hTray = FindWindowEx(0, 0, "Shell_TrayWnd", NULL);
  if (hTray) return FindWindowEx(hTray, 0, "TrayNotifyWnd", NULL) != 0;
  return FALSE;
}

static void CALLBACK LoginTimerProc(HWND hwnd, UINT uMsg, UINT idEvent, DWORD dwTime)
{ if (user_logout) // check for login, then open GUI
  { if (any_user_logged())
    { user_logout=FALSE;    // start checking for logout
      open_service_gui();
    }
  }
  else            // check for logout
    if (!any_user_logged())
    { user_logout=TRUE;     // start checking for login
      logout_time=GetTickCount(); 
    }
}

BOOL WINAPI handler_routine(DWORD dwCtrlType)  
{ if (dwCtrlType == CTRL_LOGOFF_EVENT) 
  { user_logout=TRUE;  
    logout_time=GetTickCount(); 
  }
  return TRUE; 
} 
 
static BOOL smart_service_gui = TRUE; // viz KB
#endif // SMART_GUI

////////////////////////////////// GUI ////////////////////////////////////
void open_service_gui(void)
{ //common_prepare(); 
  MyTaskBarAddIcon(hwMain, loc_server_name);
}

void close_service_gui(void)
{ DestroyWindow(hwMain); } // deletes tray icon, posts quit message

volatile bool stopping;
#else // !ANY_SERVER_GUI

tobjname loc_server_name; // used by the shell of the server before the server is initialised
void write_to_server_console(cdp_t cdp, const char * text) { }  // does not have any console

void client_terminating(void) { }                               // terminating client never stops the service

void stop_the_server(void)  // stopping request from a connected client
{ SetLocalAutoEvent(&hKernelShutdownReq); }

#endif // !ANY_SERVER_GUI

///////////////////////////// system log /////////////////////////////
#include "mcmsgs.h"
#define EVENT_SOURCE_NAME "602SQL"

bool AddEventSource(void)
// Add the source name as a subkey under the Application key in the EventLog registry key. 
{ HKEY hk;  DWORD disp;
  if (RegCreateKeyEx(HKEY_LOCAL_MACHINE, "SYSTEM\\CurrentControlSet\\Services\\EventLog\\Application\\" EVENT_SOURCE_NAME, 
         NULL, "", REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS_EX, NULL, &hk, &disp) != ERROR_SUCCESS) 
    return false; 
  //if (disp==REG_CREATED_NEW_KEY)  -- overwrite any previous registration, file location may have changed
  { DWORD dwData;  char szBuf[MAX_PATH];  
    GetModuleFileName(NULL, szBuf, sizeof(szBuf));  // Set the name of the message file. 
   // Add the name to the EventMessageFile subkey. 
    RegSetValueEx(hk, "EventMessageFile", 0, REG_EXPAND_SZ, (LPBYTE)szBuf, (DWORD)strlen(szBuf) + 1);
   // Set the supported event types in the TypesSupported subkey. 
    dwData = EVENTLOG_ERROR_TYPE | EVENTLOG_WARNING_TYPE | EVENTLOG_INFORMATION_TYPE; 
    RegSetValueEx(hk, "TypesSupported",  0, REG_DWORD, (LPBYTE)&dwData, sizeof(DWORD));
  }
  RegCloseKey(hk); 
  return true;
} 

void error_to_system_log(int errnum,  const char * errmsg)
{ HANDLE h = RegisterEventSource(NULL, EVENT_SOURCE_NAME); 
  if (h != NULL) 
  { ReportEvent(h, EVENTLOG_ERROR_TYPE, 0, // category zero 
            MSG_ENVELOPE,         // event identifier 
            NULL,                 // no user security identifier 
            1,                    // one substitution string 
            0,                    // no data 
            (const char**)&errmsg,       // pointer to string array 
            NULL);                // pointer to data 
    DeregisterEventSource(h); 
  }
}
//////////////////////////////// service ///////////////////////////////////
SERVICE_STATUS_HANDLE m_hServiceStatus;
SERVICE_STATUS m_Status;
bool service_is_paused;

#define SERVICE_CONTROL_USER 128

#ifdef LIBINTL_STATIC
extern "C" int  _nl_msg_cat_cntr;
#endif

void SetStatus(DWORD dwState)
{ m_Status.dwCurrentState = dwState;
  ::SetServiceStatus(m_hServiceStatus, &m_Status);
}

#if 0
HANDLE hlogger;

void log_msg(char * msg)
{ DWORD wr;
  WriteFile(hlogger, msg, (int)strlen(msg), &wr, NULL);
}
#endif

int OnInit(const char * service_name)    
{ int err;  char path[MAX_PATH];
  is_service=true;
 /* read the CNF path from the registry: */
  HKEY hkey; 
  strcpy(path, "SYSTEM\\CurrentControlSet\\Services\\");
  strcat(path, service_name);
  if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, path, 0, KEY_QUERY_VALUE_EX, &hkey) == ERROR_SUCCESS)
  { DWORD dwType, dwSize = MAX_PATH;
    RegQueryValueEx(hkey, "CnfPath", NULL, &dwType, (BYTE*)path, &dwSize);
    RegCloseKey(hkey);
  }
  if (!ServerByPath(path, loc_server_name))
    strmaxcpy(loc_server_name, path, sizeof(loc_server_name));
  running_as_service=wbtrue;
  my_cdp->cd_init();

#ifdef ENGLISH  // start gettext() in the international version
#if WBVERS>=900
 // the database is known now, find the language for its messages:
  { char lang[40], buf[50];
    if (GetDatabaseString(loc_server_name, "LanguageOfMessages", lang, sizeof(lang), false))
    { sprintf(buf, "LANGUAGE=%s", lang);
      _putenv(buf);
#ifdef LIBINTL_STATIC
      ++_nl_msg_cat_cntr;
#endif
      char * trans = gettext("Continue?");
    }
  }
#endif
  int i;
  GetModuleFileName(NULL, path, sizeof(path));
  i=(int)strlen(path);
  while (i && path[i-1]!=PATH_SEPARATOR) i--;
  path[i]=0;
  if (*path) bindtextdomain(GETTEXT_DOMAIN, path);
  bind_textdomain_codeset(GETTEXT_DOMAIN, "UTF-8");
  textdomain(GETTEXT_DOMAIN);
#endif

  if (!GetDatabaseString(loc_server_name, t_server_profile::http_tunnel_port.name, (char*)&http_tunnel_port_num, sizeof(http_tunnel_port_num), false)) 
    http_tunnel_port_num=80;
  if (!GetDatabaseString(loc_server_name, t_server_profile::web_port        .name, (char*)&http_web_port_num,    sizeof(http_web_port_num),    false)) 
    http_web_port_num   =80;
#if WBVERS<1100
 // check the licence:
#ifndef _DEBUG_
  if (!check_server_licence()) 
    err=KSE_NO_SERVER_LICENCE;
  else 
#endif
#endif 
#if WBVERS<1000
    err=kernel_init(my_cdp, loc_server_name, true);  // always networking
#else
    err=kernel_init(my_cdp, loc_server_name, false);  // forcing the network communication removed, replaced the the default value of the net_comm property
#endif
  return err;
}

void Run(void)
{ 
#ifdef ANY_SERVER_GUI
  hInst=(HINSTANCE)GetModuleHandle(NULL);  //"602svc8.EXE"
  stopping=false;
  common_prepare(); 
#ifdef SMART_GUI 
  UINT hLoginTimer;
  if (!GetDatabaseString(loc_server_name, "SmartServiceGui", (tptr)&smart_service_gui, sizeof(DWORD), false)) 
    smart_service_gui=TRUE;
  int dividor=0;
  if (smart_service_gui)
  { if (any_user_logged())  // open gui iff user logged
      { user_logout=FALSE;  open_service_gui(); /* MessageBeep(-1);  Sleep(150);*/ }
    else // open later otherwise
      { user_logout=TRUE;  logout_time=GetTickCount(); }
    hLoginTimer = SetTimer(0, 1, 15000, LoginTimerProc);
  }
  else // open gui anyways
#endif
    open_service_gui();
  start_thread_for_periodical_actions();
  MSG msg;  
  while (GetMessage(&msg, 0, 0, 0) && !stopping)
  { if (msg.message==WM_USERCHANGED)
      reset_service_gui();
    else
    { TranslateMessage(&msg);
      DispatchMessage(&msg);
    }
  }
#ifdef SMART_GUI 
  if (smart_service_gui) KillTimer(0, hLoginTimer);
#endif

#else  // !ANY_SERVER_GUI
  start_thread_for_periodical_actions();
  WaitLocalAutoEvent(&hKernelShutdownReq, INFINITE);
  dir_kernel_close();
#endif  // !ANY_SERVER_GUI
}

void OnPause(void)     // called when the service is paused
{ if (service_is_paused) return;
  dir_lock_kernel(my_cdp, false);
  service_is_paused=true;
  m_Status.dwCurrentState = SERVICE_PAUSED;
}

void OnContinue(void)  // called when the service is continued
{ if (!service_is_paused) return;
  dir_unlock_kernel(my_cdp);
  service_is_paused=false;
  m_Status.dwCurrentState = SERVICE_RUNNING;
}

void OnUserControl(DWORD dwOpcode)   // Process user control requests
{ switch (dwOpcode)
  { case SERVICE_CONTROL_USER + 0:
      break;
  }
}

void StopService(t_down_reason dr)
{ 
  down_reason=dr;
  SetStatus(SERVICE_STOP_PENDING);
#ifdef ANY_SERVER_GUI
//  PostThreadMessage(hMainWndThreadId, WM_QUIT, 0, 0);     // WW_QUIT not received by GetMessage
//  PostQuitMessage(0);                                     // WW_QUIT not received by GetMessage
  MyTaskBarDeleteIcon(hwMain);  // deletes if exists (is on WM_DESTROY, but does not work there)
  PostMessage(hwMain, WM_QUIT, 0, 0); // probably posting to another thread, so must use window handle
  close_service_gui();
  dir_kernel_close();
  stopping=true;
#else
  SetLocalAutoEvent(&hKernelShutdownReq);
#endif  
}
     
void WINAPI Handler(DWORD dwOpcode)    // Control request handlers
{ switch (dwOpcode)
  { case SERVICE_CONTROL_STOP:
        StopService(down_system);
        break;
    case SERVICE_CONTROL_PAUSE:
        OnPause();
        break;
    case SERVICE_CONTROL_CONTINUE:
        OnContinue();
        break;
    case SERVICE_CONTROL_INTERROGATE:
        // no action, only SetServiceStatus
        break;
    case SERVICE_CONTROL_SHUTDOWN:
        StopService(down_endsession);
        break;
    default:  // not used
        if (dwOpcode >= SERVICE_CONTROL_USER) OnUserControl(dwOpcode);
        break;
  }
  ::SetServiceStatus(m_hServiceStatus, &m_Status);  // reporting the status defined by the last call to SetStatus()
}

static char cml_password[MAX_FIL_PASSWORD+1];

bool get_database_file_password(char * password)
{ 
  strcpy(password, cml_password); 
  return true;
}

void WINAPI ServiceMain(DWORD dwArgc, LPTSTR* lpszArgv)
{ 
  m_Status.dwServiceType = SERVICE_WIN32_OWN_PROCESS | SERVICE_INTERACTIVE_PROCESS;
  m_Status.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_PAUSE_CONTINUE | SERVICE_ACCEPT_SHUTDOWN;
  m_Status.dwWin32ExitCode = NO_ERROR;
  m_Status.dwServiceSpecificExitCode = 0;
  m_Status.dwCheckPoint = 0;
  m_Status.dwWaitHint = 20000;
  service_is_paused  = false;
 // Register the control request handler
  m_Status.dwCurrentState = SERVICE_START_PENDING;
  m_hServiceStatus = RegisterServiceCtrlHandler(lpszArgv[0], Handler);
  if (m_hServiceStatus == NULL) return;

 // scan parameters for the password:
  *cml_password=0;
  int i=1;  
  bool passcont=false;  // password containing a space may be divided into several parameters!! Double quotes do not help!
  while (i<dwArgc)
  { if (lpszArgv[i])
    { if (lpszArgv[i][0]=='-' || lpszArgv[i][0]=='/')
        if (lpszArgv[i][1]=='p')
          { strmaxcpy(cml_password, lpszArgv[i]+2, sizeof(cml_password));  passcont=true; }
        else  
          passcont=false; // other parameter (not recognized, ignored)
      else 
        if (passcont)  // contunuation of the -p value
        { int len=(int)strlen(cml_password);
          if (len+1<MAX_FIL_PASSWORD)
          { cml_password[len++]=' ';
            strmaxcpy(cml_password+len, lpszArgv[i], sizeof(cml_password)-len);
          }  
        }
    }      
    i++;
  }        
 // Initialize and run
  SetStatus(SERVICE_START_PENDING);
  int res = OnInit(lpszArgv[0]);
  m_Status.dwCheckPoint = 0;
  m_Status.dwWaitHint = 0;
  if (res==KSE_OK)
  { 
#ifdef SMART_GUI
    SetConsoleCtrlHandler(handler_routine, TRUE);  // watching for Logoff
#endif
    SetStatus(SERVICE_RUNNING);
    Run();
  }
  else  /* initialization error */
  {// write to system log:
    AddEventSource();
    error_to_system_log(res, kse_errors[res]);
    m_Status.dwServiceSpecificExitCode = res;
    m_Status.dwWin32ExitCode = ERROR_SERVICE_SPECIFIC_ERROR;
  }
#if defined(_DEBUG) && !defined(X64ARCH)
  stackwalk_unprepare();
#endif
  SetStatus(SERVICE_STOPPED);
}

#define SERVICE_NAME "602SQL Server"  // not used

int main(int argc, char* argv[])
{ SERVICE_TABLE_ENTRY st[] = { { argc>1 ? argv[1] : SERVICE_NAME, ServiceMain}, {NULL, NULL} };
  m_Status.dwWin32ExitCode = NO_ERROR;
  ::StartServiceCtrlDispatcher(st);
  return m_Status.dwWin32ExitCode;
}

#ifdef ANY_SERVER_GUI
void StopServerOrService(void)  // called from GUI
{ StopService(down_console); }
#endif

