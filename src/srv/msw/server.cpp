/****************************************************************************/
/* server.cpp - database server application for Windows                     */
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
#include "netapi.h"
#include <windowsx.h>
#ifndef ANY_SERVER_GUI
#define CONSOLE_INPUT_ENABLED
#ifdef WINS
#include <clocale>
#endif
#endif
#include "nlmtexts.h"

#include "enumsrv.c"

#if defined(_DEBUG) && !defined(X64ARCH)
#include "stackwalk.cpp"
#endif

static void report_error_to_client(const char * server_name, int errnum)
 // do not use global shared objects on Vista unless
{ HANDLE hMakerEvent, hMakeDoneEvent, hMemFile;  DWORD * clientId;  bool mapped_long;
 // do not use global shared objects on Vista unless
  bool try_global = true;
  { OSVERSIONINFO osv;
    osv.dwOSVersionInfoSize = sizeof(osv);
    GetVersionEx(&osv);
    if (osv.dwMajorVersion >= 6)
      if (!is_service)
        try_global=false;
  }
  if (CreateIPComm0(server_name, NULL, &hMakerEvent, &hMakeDoneEvent, &hMemFile, &clientId, mapped_long, try_global))
  { DWORD res=WaitForSingleObject(hMakerEvent, 5000);  /* must wait, otherwise client will overwrite error num by client ID */
    if (res==WAIT_OBJECT_0)  /* time-out if client calls link_kernel, but not interf_init */
    { clientId[0]=errnum;     /* write error */
		  SetEvent(hMakeDoneEvent);
    }
    CloseIPComm0(hMakerEvent, hMakeDoneEvent, hMemFile, clientId);
  }
}

static BOOL networking; // networking request at the server start, not the actual state

#ifdef ANY_SERVER_GUI
#include "server.h"
#include "srvctrl.cpp"

void StopServerOrService(void)
{ down_reason=down_console;
  DestroyWindow(hwMain); 
}

void stop_the_server(void)  // stopping request from a connected client
{ PostMessage(hwMain, WM_ENDSESSION, TRUE, 0); }
////////////////////////////// dialog for kernel password ////////////////////
typedef struct
{ char * text;
  int   maxlen;
} ib_t;

// Kdyz chci mit dialog na heslo v popredi na serveru spustenem z klienta,
// musim odlozit presunuti dialogu do popredi na pozdeji, az dobehnou akce 
// klienta. Sleep nestaci, protoze ve skutecnosti neodevzda rizeni klientovi.
// Funguje pouze Timer.

static BOOL CALLBACK InputBoxDlgProc(HWND hDlg, UINT msg, WPARAM wP, LPARAM lP)
{ HWND hEdit;  int len;  ib_t * ib;
  switch (msg)
  { case WM_INITDIALOG:
    { ib=(ib_t*)lP;
      SetWindowLong(hDlg, DWL_USER, lP);
      hEdit=GetDlgItem(hDlg, CD_KERPASS_EDIT);
      SetWindowText(hEdit, ib->text);
      SendMessage(hEdit, EM_LIMITTEXT, ib->maxlen, 0);
      SetTimer(hDlg, 1, 700, NULL);
#ifdef ENGLISH
      char transbuf[80];
      SetWindowText(hDlg,            form_terminal_message(transbuf, sizeof(transbuf), _("Enter the server password:")));
      SetDlgItemText(hDlg, IDOK,     form_terminal_message(transbuf, sizeof(transbuf), _("&OK")));
      SetDlgItemText(hDlg, IDCANCEL, form_terminal_message(transbuf, sizeof(transbuf), _("&Cancel")));
#endif
      break;
    }
    case WM_TIMER:
      KillTimer(hDlg, 1);
      SetForegroundWindow(hDlg);
      BringWindowToTop(hDlg);
      SetActiveWindow(hDlg);
      SetFocus(hDlg); 
      break;
    case WM_COMMAND:
      ib=(ib_t*)GetWindowLong(hDlg, DWL_USER);
      switch (GET_WM_COMMAND_ID(wP,lP))
      { case IDOK:
          *(WORD*)ib->text=ib->maxlen;  /* size limit */
          len=(int)SendDlgItemMessage(hDlg, CD_KERPASS_EDIT, EM_GETLINE,
                                      0, (LPARAM)ib->text);
          ib->text[len]=0;
          EndDialog(hDlg, 1);  break;
        case IDCANCEL:
          EndDialog(hDlg, 0);  break;
        default: return FALSE;
      }
      break;
    default: return FALSE;
  }
  return TRUE;
}

bool get_database_file_password(char * password)
// FALSE returned if dialog cancelled
{ ib_t ib; *password=0;  ib.text=password;  ib.maxlen=MAX_FIL_PASSWORD;
  return DialogBoxParam(hInst, MAKEINTRESOURCE(DLG_KERNEL_PASSWORD), hwMain, InputBoxDlgProc, (LPARAM)&ib) == 1;
  // Dialog must have the WS_EX_TOPMOST style in order not to be covered by the splash screen of the client.
}

//////////////////////////////////////// selecting the database ///////////////////////////////////////
static BOOL CALLBACK SelDbDlgProc(HWND hDlg, UINT msg, WPARAM wP, LPARAM lP)
{ switch (msg)
  { case WM_INITDIALOG:
    { tobjname a_server_name;  char a_path[MAX_PATH];
      t_enum_srv es(t_enum_srv::ENUM_SRV_BOTH, CURRENT_FIL_VERSION_STR);
      while (es.next(a_server_name))
        if (es.GetPrimaryPath(a_server_name, a_path) && *a_path)
          SendDlgItemMessage(hDlg, CD_DATABASE_LIST, LB_ADDSTRING, 0, (LPARAM)a_server_name);
      SendDlgItemMessage(hDlg, CD_DATABASE_LIST, LB_SETCURSEL, 0, 0);
      CheckDlgButton(hDlg, CD_NETWORK_SERVER, networking);  // check box not shown since version 10
#ifdef ENGLISH
      char transbuf[80];
      SetDlgItemText(hDlg, 32001,             form_terminal_message(transbuf, sizeof(transbuf), _("Select the database for the server:")));
      SetDlgItemText(hDlg, 32002,             form_terminal_message(transbuf, sizeof(transbuf), _("(database is not specified on the command line)")));
      SetDlgItemText(hDlg, CD_NETWORK_SERVER, form_terminal_message(transbuf, sizeof(transbuf), _("&Enable network operation of the server")));
      SetDlgItemText(hDlg, IDOK,              form_terminal_message(transbuf, sizeof(transbuf), _("&OK")));
      SetDlgItemText(hDlg, IDCANCEL,          form_terminal_message(transbuf, sizeof(transbuf), _("&Cancel")));
      SetWindowText(hDlg,                     form_terminal_message(transbuf, sizeof(transbuf), _("Select the database")));
#endif
      break;
    }
    case WM_HELP:
      return local_process_context_popup((HELPINFO*)lP);
    case WM_CONTEXTMENU:
      return local_process_context_menu(hDlg, wP, lP);
    case WM_DESTROY:
      HtmlHelp(NULL, NULL, HH_CLOSE_ALL, 0);
      return FALSE;
    case WM_COMMAND:
      switch (GET_WM_COMMAND_ID(wP,lP))
      { case CD_DATABASE_LIST:
          if (GET_WM_COMMAND_CMD(wP,lP) != LBN_DBLCLK) break;
          // cont.
        case IDOK:
        { int sel=SendDlgItemMessage(hDlg, CD_DATABASE_LIST, LB_GETCURSEL, 0, 0);
          if (sel==LB_ERR) break;
          SendDlgItemMessage(hDlg, CD_DATABASE_LIST, LB_GETTEXT, sel, (LPARAM)loc_server_name);
          networking=IsDlgButtonChecked(hDlg, CD_NETWORK_SERVER);  // check box not shown since version 10
          EndDialog(hDlg, 1);  break;
        }
        case IDCANCEL:
          EndDialog(hDlg, 0);  break;
        default: return FALSE;
      }
      break;
    default: return FALSE;
  }
  return TRUE;
}

static void Kernel_error_box(HWND hParent, int errnum)
// Reports errors when starting server
{ if (errnum!=KSE_OK)
  { if (errnum > KSE_LAST) errnum=KSE_LAST;
    char caption[50], msg[100];
    form_terminal_message(caption, sizeof(caption), kse_errors[0]);
    form_terminal_message(msg, sizeof(msg), kse_errors[errnum]);
    MessageBox(hParent, msg, caption, MB_OK | MB_ICONSTOP | MB_APPLMODAL);
  }
}

char * my_strtok(char * input, char * delims)
{ static char * cont_input;  char * output, * op;
  if (input==NULL) input=cont_input;
 /* skip delimiters: */
  if (!*input) return NULL;
  while (*input && strchr(delims, *input)!=NULL) input++;
 /* scan token: */
  output=op=input;
  while (*input && strchr(delims, *input)==NULL)
  { if (*input=='\"')
    { input++;
      while (*input)
      { if (*input=='\"' || *input=='\'')
          if (input[1]==*input) input++;
          else break;
        *(op++)=*(input++);
      }
      if (*input) input++;
    }
    else *(op++)=*(input++);
  }
  if (*input) cont_input=input+1;
  else cont_input=input;
  *op=0;
  return output;
}

typedef BOOL WINAPI t_AllowSetForegroundWindow(DWORD dwProcessId);
#define ASFW_ANY    ((DWORD)-1)
#ifdef LIBINTL_STATIC
extern "C" int  _nl_msg_cat_cntr;
#endif

bool create_opl_key(char * ik, int opl_num, unsigned opl_lic_cnt);

#include "wbsecur.h"

int WINAPI WinMain(HINSTANCE hInstance,  HINSTANCE hPrevInstance, LPSTR lpszCmdLine, int nCmdShow)
{ int err = 0;  hwMain = 0;  char * kernelpath;
  is_service=false;

#ifdef ENGLISH  // start gettext() in the international version
  //setlocale( LC_ALL, "" );  // seems not to be necessary on Windows, but the locale must not be "C"
  //_putenv("LANGUAGE=sk_SK");  -- this works
  //setlocale( LC_MESSAGES, "cs_CZ" ); -- does nothing on Windows
  { char path[MAX_PATH];  int i;
    GetModuleFileName(NULL, path, sizeof(path));
    i=strlen(path);
    while (i && path[i-1]!=PATH_SEPARATOR) i--;
    path[i]=0;
    if (*path) bindtextdomain(GETTEXT_DOMAIN, path);
    bind_textdomain_codeset(GETTEXT_DOMAIN, "UTF-8");
    textdomain(GETTEXT_DOMAIN);
    char * trans = gettext("Continue?");
  }
#endif

  hInst  = hKernelLibInst = hInstance;
  if (nCmdShow!=SW_SHOWMINNOACTIVE) nCmdShow=SW_SHOWMINIMIZED;
 // analyse the command line:
  BOOL restore_db_file = FALSE;
  kernelpath=NULL;  networking=comm_log_replay=FALSE;
  is_dependent_server=false;
  while (*lpszCmdLine==' ') lpszCmdLine++;
  while ((*lpszCmdLine!=' ') && (*lpszCmdLine!='/') && (*lpszCmdLine!='&') &&
         (*lpszCmdLine!='-') && *lpszCmdLine) lpszCmdLine++;
  while (*lpszCmdLine==' ') lpszCmdLine++;
#if WBVERS<900
  char * tok=my_strtok(lpszCmdLine, " ,;\r\n");
  while (tok)
  { if (*tok=='&') kernelpath=tok+1;
    else if (!stricmp(tok, "/N")) networking=TRUE;
    else if (!stricmp(tok, "/R")) comm_log_replay=TRUE;
    else if (!stricmp(tok, "/Q")) { disable_system_triggers=TRUE;  minimal_start=true; }
    else if (!stricmp(tok, "/Berle")) restore_db_file=TRUE;
    else if (tok[0]=='/' && tok[1]=='!') is_dependent_server=true;
    tok=my_strtok(NULL, " ,;\r\n");
  }
#else // new parameters
  char * p = lpszCmdLine;
  while (*p)
  { while (*p && *p<=' ') p++;
    if (*p=='-' || *p=='/' || *p=='&')  // parameter start
    { if (*p!='&') p++;
      switch (*p)
      { case 'N':
          if (p[1]<=' ') networking=TRUE;  break;
        case 'R':  
          if (p[1]<=' ') comm_log_replay=TRUE;  break;
        case 'Q':  case 'q':
          if (p[1]<=' ') { disable_system_triggers=TRUE;  minimal_start=true; }  break;
        case '!':
          if (p[1]<=' ') is_dependent_server=true;  break;
        default:
          if (!_memicmp(p+1, "Berle", 5)) restore_db_file=TRUE;  break;
        case 's':  case 'n':  case '&':
        { p++;
          while (*p && *p<=' ') p++;
          int i=0;  
         // read the server name:
          if (*p=='\"')
          { p++;
            do 
            { if (*p=='\"')
                if (p[1]=='\"')
                  p++;  // doubled "
                else 
                  { p++;  break; }
              if (i<OBJ_NAME_LEN) loc_server_name[i++]=*p;
              p++;
            } while (true);
          }
          else
            while (*p>' ')
            { if (i<OBJ_NAME_LEN) loc_server_name[i++]=*p;
              p++;
            }
          loc_server_name[i]=0;
          kernelpath=loc_server_name;
          p--;  // preare to skip
          break;    
        }
      }
    }
   // skip:
    while (*p > ' ') p++;
  }
#endif
  prep_regdb_access();

 // check the licence:
#ifndef _DEBUG
  if (!check_server_licence()) 
    { err=KSE_NO_SERVER_LICENCE;  goto onerr; }
#endif
 // dialog to select the database, if not specified
  if (kernelpath==NULL)
  { *loc_server_name=0;  unsigned cnt=0;;  /* init. result */
    t_enum_srv es(t_enum_srv::ENUM_SRV_BOTH, CURRENT_FIL_VERSION_STR);   tobjname a_server_name;  char a_path[MAX_PATH];
    while (es.next(a_server_name))
      if (es.GetPrimaryPath(a_server_name, a_path) && *a_path)
        { strcpy(loc_server_name, a_server_name);  cnt++; }
   // report error, if no databases registered:
    if (cnt==0)
    { char text[50], caption[50];
      form_terminal_message(text, sizeof(text), NO_DATABASES_REG);
      form_terminal_message(caption, sizeof(caption), kse_errors[0]);
      MessageBox(0, text, caption, MB_OK | MB_APPLMODAL | MB_ICONSTOP);
      return 2;
    }
   // open dialog, if multiple:
    if (cnt>1)
    { if (DialogBox(hInst, MAKEINTRESOURCE(DLG_SELECT_DATABASE), 0, SelDbDlgProc) != 1)
        return 3;
    }
#if WBVERS<1000
    else networking=TRUE;  // default if database name not specified (inconsistent behaviour, removed inside version 10)
#endif
    kernelpath=loc_server_name;
  }
 
  else  /* translate path to server name if path specified: */
    if (*kernelpath && kernelpath[1]==':')
    { if (ServerByPath(kernelpath, loc_server_name))
        kernelpath=loc_server_name;
    }
    else strcpy(loc_server_name, kernelpath);

 // the database is known now, find the language for its messages:
#if WBVERS>=900
#ifdef ENGLISH  // start gettext() in the international version
  { char lang[40], buf[50];
    if (GetDatabaseString(loc_server_name, "LanguageOfMessages", lang, sizeof(lang), my_private_server))
    { sprintf(buf, "LANGUAGE=%s", lang);
      _putenv(buf);
#ifdef LIBINTL_STATIC
      ++_nl_msg_cat_cntr;
#endif
      char * trans = gettext("Continue?");
    }
  }
#endif
#endif
 // start the user inerface:
  err=common_prepare();
  if (err) goto onerr;
   if (is_dependent_server)
   { HINSTANCE hLibInst = LoadLibrary("USER32.DLL");
     if (hLibInst)
     { t_AllowSetForegroundWindow * p_proc = (t_AllowSetForegroundWindow *)GetProcAddress(hLibInst, "AllowSetForegroundWindow");
       if (!p_proc) p_proc = (t_AllowSetForegroundWindow *)GetProcAddress(hLibInst, "AllowSetForegroundWindowA");
       if (p_proc) 
       { (*p_proc)(ASFW_ANY);
         //MessageBeep(-1);
       }
       FreeLibrary(hLibInst);
     }
   }

   if (restore_db_file)
     err=restore_database_file(kernelpath);
   else  // start server
   { SetCursor(LoadCursor(NULL, IDC_WAIT));
     back_end_operation_on=TRUE;  // disables console output made by detached threads (_on_server_start)
     if (!GetDatabaseString(loc_server_name, t_server_profile::http_tunnel_port.name, (char*)&http_tunnel_port_num, sizeof(http_tunnel_port_num), my_private_server)) 
       http_tunnel_port_num=80;
     if (!GetDatabaseString(loc_server_name, t_server_profile::web_port        .name, (char*)&http_web_port_num,    sizeof(http_web_port_num),    my_private_server)) 
       http_web_port_num=80;
     err=kernel_init(my_cdp, kernelpath, networking!=0);
     back_end_operation_on=FALSE;
     SetCursor(LoadCursor(NULL, IDC_ARROW));
     iconic_main_window = the_sp.show_as_tray_icon.val() != 0;
     if (iconic_main_window) MyTaskBarAddIcon(hwMain, loc_server_name);
     else ShowWindow(hwMain, nCmdShow);      // Make window visible in the task bar
#if WBVERS<900
     if (!the_sp.EnableConsoleButton.val()) EnableWindow(GetDlgItem(hInfoWnd, CD_SI_CONSOLE), FALSE);
#endif
     SendMessage(hInfoWnd, WM_SHOWWINDOW, TRUE, 0);  // update the contents
   }

 onerr:
  if (err)
  { if (is_dependent_server) report_error_to_client(kernelpath, err); // kernelpath is defined from the beginning
    if (err!=KSE_NO_SERVER_LICENCE)
      if (!is_dependent_server /*|| err==KSE_BAD_PASSWORD*/) // display error box - unless it will be displayed by the client
        Kernel_error_box(hwMain, err);
    if (hwMain) DestroyWindow(hwMain);
#if defined(_DEBUG) && !defined(X64ARCH)
    stackwalk_unprepare();
#endif
    return 1;
  }

  if (restore_db_file) // just restored the database file
    ;
  else if (comm_log_replay)  // replay and terminate
    communic_replay();
  else                       // normal server opertion
  { ffa.log_client_restart(); // write restart info to the client communication log
   // start backup timer:
    start_thread_for_periodical_actions();
   // message cycle:
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
      if (!hInfoWnd || !IsDialogMessage(hInfoWnd, &msg))
      { TranslateMessage(&msg);
        DispatchMessage(&msg);
      }
#if WBVERS<900  // since 9.0 closing the window after closing the server
    if (hSrvCons) 
    { PostMessage(hSrvCons, WM_QUIT, 0, 0); 
      for (int i=0;  hSrvCons && i<30;  i++)  // wait for disconnecting the console
        Sleep(100);
    }
    dir_kernel_close();
#endif
  }

  UnregisterClass(server_class_name, hInstance);
#if defined(_DEBUG) && !defined(X64ARCH)
  stackwalk_unprepare();
#endif
  return 0;
} 

#else  // !ANY_SERVER_GUI
tobjname loc_server_name; // used by the shell of the server before the server is initialised
HANDLE hStdin, hStdout;

bool cml_password_specified = false;
static char cml_password[MAX_FIL_PASSWORD+1+1] = "";

#ifndef ENABLE_EXTENDED_FLAGS  // old Platform SDK ###
#define ENABLE_EXTENDED_FLAGS   0
#define ENABLE_INSERT_MODE      0
#define ENABLE_QUICK_EDIT_MODE  0
#endif

void conv_to_server_console(cdp_t cdp, const char * text, int charset)
{ DWORD wr;
  while (*text=='\n') text++;
  { wchar_t * wbuf = new wchar_t[strlen(text)+1];
    superconv(ATT_STRING, ATT_STRING, text, (char*)wbuf, t_specif(0, charset, 0, 0), t_specif(0, 0, 0, 1), NULL);
    WriteConsoleW(hStdout, wbuf, (int)wcslen(wbuf), &wr, NULL);
    delete [] wbuf;
  }
  WriteConsole(hStdout, "\n", 1, &wr, NULL);
}

void write_to_server_console(cdp_t cdp, const char * text)
// [text] is translated and is in SQL system charset as specified in sys_spec. ATTN: sys_spec changes during the server startup!
{ 
  conv_to_server_console(cdp, text, sys_spec.charset);
}

void write_to_console(cdp_t cdp, const char * text)
// [text] is translated and is in UTF-8 charset.
{ 
  conv_to_server_console(cdp, text, CHARSET_NUM_UTF8);
}

bool get_database_file_password(char * password)
{ 
  if (cml_password_specified)
    strcpy(password, cml_password); 
  else
  { char msg[100];  DWORD wr;
    form_terminal_message(msg, sizeof(msg), _("Enter the password of the database:"));
    write_to_console(NULL, msg);
   // read the password: 
    // no echo: ReadConsole(hStdin, cml_password, sizeof(cml_password), &wr, NULL);
    //int plen = strlen(cml_password);
    //while (plen && cml_password[plen-1]<' ') cml_password[--plen]=0;
    //strmaxcpy(password, cml_password, MAX_FIL_PASSWORD+1);
    SetConsoleMode(hStdin, 0);
    int pos=0;
    do
    { unsigned char c;
      ReadConsole(hStdin, &c, 1, &wr, NULL);
      if (c>=' ')
      { if (pos<MAX_FIL_PASSWORD)
        { password[pos++]=c;
          c='*';
          WriteConsole(hStdout, &c, 1, &wr, NULL);
        } // more characters: ignored 
      }
      else if (c=='\n' || c=='\r' || !c || c==0xff)
        break;
      else if (c=='\b')
      { if (pos)
        { pos--;  
          WriteConsole(hStdout, "\b \b", 3, &wr, NULL);
        }  
      }
    } while (true);
    password[pos]=0;  
    WriteConsole(hStdout, "\n", 1, &wr, NULL);
  }  
 // restore the normal input mode: 
  SetConsoleMode(hStdin, ENABLE_ECHO_INPUT | ENABLE_EXTENDED_FLAGS | ENABLE_INSERT_MODE | ENABLE_LINE_INPUT | ENABLE_MOUSE_INPUT | ENABLE_PROCESSED_INPUT | ENABLE_QUICK_EDIT_MODE);
  return true;
}

void pres_enter_to_exit(void)
{ char msg[100];  DWORD wr;
  form_terminal_message(msg, sizeof(msg), _("Press <Enter> to close the server console..."));
  write_to_console(NULL, msg);
  ReadConsole(hStdin, msg, sizeof(msg), &wr, NULL);
}

void stop_the_server(void)  // stopping request from a connected client
{ SetLocalAutoEvent(&hKernelShutdownReq); }

void client_terminating(void)
{
  if (is_dependent_server && get_external_client_connections()==0)
  { down_reason=down_from_client;
    SetLocalAutoEvent(&hKernelShutdownReq);
  }  
}
  
BOOL WINAPI HandlerRoutine(DWORD dwCtrlType)
{
  switch (dwCtrlType)
  { case CTRL_C_EVENT:
    case CTRL_BREAK_EVENT:
      return TRUE;  // ignore
    case CTRL_CLOSE_EVENT:
      down_reason=down_console;
      SetLocalAutoEvent(&hKernelShutdownReq);
      Sleep(7000);  // time for the main thread to end
      return TRUE;  // display the system dialog (cannot prevent stopping the process)
    case CTRL_LOGOFF_EVENT:
    case CTRL_SHUTDOWN_EVENT:
      down_reason=down_endsession;
      SetLocalAutoEvent(&hKernelShutdownReq);
      Sleep(7000);  // time for the main thread to end
      ExitProcess(0);
  }
  return FALSE;
}

bool YesNoPrompt(bool default_answer)
{ char text[10];  DWORD rd;
  FlushConsoleInputBuffer(hStdin);  // on W98 the character read by ReadConsoleInput is returned again by ReadConsole
  const char * cYes = _("Yes"), * cNo = _("No");
  ReadConsole(hStdin, text, sizeof(text)-1, &rd, NULL);
  text[rd]=0;
  char cAns = toupper(*text);
  if (cAns==toupper(*cYes) || cAns=='A') return true; // problem with slovak A'no - must explicitly allow A.
  if (cAns==toupper(*cNo )) return false;
  return default_answer;
}

int input_proc_mode = 0;

void process_input(char inpchar)
{ char text[100];  
  if (inpchar=='Q' || inpchar=='q')
  { if (back_end_operation_on)
    { form_terminal_message(text, sizeof(text), _("Cannot stop the server now, maintenance operations are in progress."));
      write_to_console(NULL, text);
    }  
    else if (get_external_client_connections()==0)
    { form_terminal_message(text, sizeof(text), _("Do you really want to stop the SQL server? (y/n):"));
      write_to_console(NULL, text);
      if (YesNoPrompt(false))
      { client_logout_timeout=0;
        down_reason=down_console;
        SetLocalAutoEvent(&hKernelShutdownReq);
      }
      else
      { form_terminal_message(text, sizeof(text), _("SQL server not stopped"));
        write_to_console(NULL, text);
      }  
    }  
    else if (input_proc_mode==1)
    { client_logout_timeout=0;
      down_reason=down_console;
      SetLocalAutoEvent(&hKernelShutdownReq);
    }
    else
    { form_terminal_message(text, sizeof(text), _("Clients are connected to the server."));
      write_to_console(NULL, text);
      form_terminal_message(text, sizeof(text), _("Press Q again to terminate immediately."));
      write_to_console(NULL, text);
      form_terminal_message(text, sizeof(text), _("Press W to warn interactive clients and wait max. 60 seconds until they terminate."));
      write_to_console(NULL, text);
      form_terminal_message(text, sizeof(text), _("Press any other key to cancel the shutdown."));
      write_to_console(NULL, text);
      input_proc_mode=1;
    }
  }  
  else if (input_proc_mode==1 && (inpchar=='W' || inpchar=='w'))
  { client_logout_timeout=60;
    down_reason=down_console;
    SetLocalAutoEvent(&hKernelShutdownReq);
  }  
  else if (input_proc_mode)
  { if (input_proc_mode==1)
    { form_terminal_message(text, sizeof(text), _("SQL server not stopped"));
      write_to_console(NULL, text);
    }  
    input_proc_mode=0;
  }  
  else if (inpchar>' ') 
  { form_terminal_message(text, sizeof(text)-1, _("Press Q to terminate the SQL server"));
    write_to_console(NULL, text);
    MessageBeep(-1);
  }  
}

typedef HWND WINAPI t_GSM(void);
HWND hConsoleWindow = 0;  // will not be defined on W95/98

#define ZIP602_STATIC
#include "Zip602.h"

int main(int argc, char * argv[])
{
  is_service=false;
  hStdin  = GetStdHandle(STD_INPUT_HANDLE); 
  hStdout = GetStdHandle(STD_OUTPUT_HANDLE); 

 // disable the Edit item in the system menu because Edit/Mark freezes the server, remove the Close command: 
  HINSTANCE hInst = LoadLibrary("kernel32.dll");
  t_GSM * p_GSM = (t_GSM*)GetProcAddress(hInst, "GetConsoleWindow");
  if (p_GSM)
  { hConsoleWindow=(*p_GSM)();
    if (hConsoleWindow)
    { HMENU hMenu = GetSystemMenu(hConsoleWindow, FALSE);
      EnableMenuItem(hMenu, 7, MF_BYPOSITION | MF_GRAYED | MF_DISABLED);  // disables the Edit item in the system menu.
      //EnableMenuItem(hMenu, 6, MF_BYPOSITION | MF_GRAYED | MF_DISABLED);  // disables Close button, but openning the system menu reenables it
      DeleteMenu(hMenu, SC_CLOSE, MF_BYCOMMAND);
    }  
  }
  FreeLibrary(hInst);

 // analyse the command line:
  BOOL restore_db_file = FALSE;
  networking=FALSE;
  is_dependent_server=false;
  *loc_server_name=0;
  char prevpar=0;
 // parse the command line ("s are removed):
  for (int i=1;  i<argc;  i++)
  { char * p = argv[i];
    if (*p=='-' || *p=='/' || *p=='&')  // parameter start
    { prevpar=0;
      if (*p!='&') p++;
      switch (*p)
      { case 'N':
          if (!p[1]) networking=TRUE;  break;
        case 'Q':  case 'q':
          if (!p[1]) { disable_system_triggers=TRUE;  minimal_start=true; }  break;
        case '!':
          if (!p[1]) is_dependent_server=true;  break;
        default:
          if (!_memicmp(p+1, "Berle", 5)) restore_db_file=TRUE;  break;
        case 's':  case 'n':  case '&':
          if (p[1]) strmaxcpy(loc_server_name, p+1, sizeof(loc_server_name));  
          else prevpar='n';
          break;    
        case 'p':
          if (p[1]) 
            strmaxcpy(cml_password, p+1, sizeof(cml_password));  
          else prevpar='p';
          cml_password_specified=true; // empty password used by the server manager
          break;
      }
    }
    else  // parameter not starting with - nor /
    { if (prevpar=='n')
        strmaxcpy(loc_server_name, p, sizeof(loc_server_name));
      else if (prevpar=='p')
        strmaxcpy(cml_password, p, sizeof(cml_password));  
      prevpar=0;  
    }    
  } 
 // determine the server name:
  if (!*loc_server_name)  //  server name not specified on the command line
  { char text[100];
   // count servers: 
    unsigned cnt=0;  tobjname a_server_name;  char a_path[MAX_PATH];  DWORD wr;
    t_enum_srv es(t_enum_srv::ENUM_SRV_BOTH, CURRENT_FIL_VERSION_STR);   
    while (es.next(a_server_name))
      if (es.GetPrimaryPath(a_server_name, a_path) && *a_path)
        { strcpy(loc_server_name, a_server_name);  cnt++; }
   // report error, if no databases registered:
    if (cnt==0)
    { form_terminal_message(text, sizeof(text), gettext(kse_errors[0]));
      write_to_console(NULL, text);
      form_terminal_message(text, sizeof(text), _("Local database not registered"));
      write_to_console(NULL, text);
      pres_enter_to_exit();
    }
   // select the database, if multiple exist:
    if (cnt>1)
    { int num=1;  char numbuf[16];  sig32 anum;
      form_terminal_message(text, sizeof(text), _("Registered local databases:"));
      write_to_console(NULL, text);
      t_enum_srv es(t_enum_srv::ENUM_SRV_BOTH, CURRENT_FIL_VERSION_STR);   
      while (es.next(a_server_name))
        if (es.GetPrimaryPath(a_server_name, a_path) && *a_path)
        { sprintf(numbuf, "%u. ", num++);
          WriteConsole(hStdout, numbuf, (DWORD)strlen(numbuf), &wr, NULL);
          write_to_console(NULL, a_server_name);
        }  
     // let the operator to enter the name or number: 
      form_terminal_message(text, sizeof(text), _("Enter the name or number of the database for the server:"));
      write_to_console(NULL, text);
      unsigned char line[OBJ_NAME_LEN+3];
      ReadConsole(hStdin, line, sizeof(line), &wr, NULL);
      line[wr]=0;
      while (wr && line[wr-1]<=' ') line[--wr]=0;
     // take the input and ind the database name: 
      if (str2int((char*)line, &anum) && anum>0 && anum<num)  // database number specified
      { num=1;
        t_enum_srv es(t_enum_srv::ENUM_SRV_BOTH, CURRENT_FIL_VERSION_STR);   
        while (es.next(a_server_name))
          if (GetPrimaryPath(a_server_name, false, a_path) && *a_path)
            if (num++==anum)
              strcpy(loc_server_name, a_server_name);
      }
      else  // name specified
        strmaxcpy(loc_server_name, (char*)line, sizeof(loc_server_name));
    }
  }
  
 // prepare the gettext domain (the loc_server_name must be known):
  { char path[MAX_PATH];  int i;  char lang[40], buf[50];
    if (GetDatabaseString(loc_server_name, "LanguageOfMessages", lang, sizeof(lang), my_private_server))
    { sprintf(buf, "LANGUAGE=%s", lang);
      _putenv(buf);                     
      // setlocale(LC_MESSAGES, lang);  // if locale is "C", the value of LANGUAGE is ignored -- LC_MESSAGES is not allowed on MSW!
    }  
    GetModuleFileName(NULL, path, sizeof(path));
    i=(int)strlen(path);
    while (i && path[i-1]!=PATH_SEPARATOR) i--;
    path[i]=0;
    if (*path) bindtextdomain(GETTEXT_DOMAIN, path);
    bind_textdomain_codeset(GETTEXT_DOMAIN, "UTF-8");
    textdomain(GETTEXT_DOMAIN);
    char * trans = gettext("Continue?");
  }
  
  if (restore_db_file)
    return restore_database_file(loc_server_name);
  else  // start server
  {// prepare the console environment: 
    char server_caption[40+OBJ_NAME_LEN];
    form_terminal_message(server_caption, sizeof(server_caption), SERVER_CAPTION, loc_server_name);
    SetConsoleTitle(server_caption);
    SetConsoleCtrlHandler(HandlerRoutine, TRUE);

   if (!GetDatabaseString(loc_server_name, t_server_profile::http_tunnel_port.name, (char*)&http_tunnel_port_num, sizeof(http_tunnel_port_num), my_private_server)) 
     http_tunnel_port_num=80;
   if (!GetDatabaseString(loc_server_name, t_server_profile::web_port        .name, (char*)&http_web_port_num,    sizeof(http_web_port_num),    my_private_server)) 
     http_web_port_num=80;
   int err=kernel_init(my_cdp, loc_server_name, networking!=0);
   if (err!=KSE_OK)
   { //if (is_dependent_server) report_error_to_client(loc_server_name, err); -- reporting to mng, too
     // show error in the console:
      char msg[100];
      form_terminal_message(msg, sizeof(msg)-1, gettext(kse_errors[0]));
      write_to_console(NULL, msg);
      form_terminal_message(msg, sizeof(msg)-1, gettext(kse_errors[err]));
      write_to_console(NULL, msg);
      report_error_to_client(loc_server_name, err);
      pres_enter_to_exit();
#if defined(_DEBUG) && !defined(X64ARCH)
      stackwalk_unprepare();
#endif
      return 1;
    } 
  }

  //if (hConsoleWindow) ShowWindow(hConsoleWindow, SW_HIDE); -- the console window can be hidden but it cannot receive message from the tray icon

  start_thread_for_periodical_actions();
#ifdef CONSOLE_INPUT_ENABLED
  char msg[100];
  form_terminal_message(msg, sizeof(msg)-1, _("Press Q to terminate the SQL server"));
  write_to_console(NULL, msg);
  SetConsoleMode(hStdin, 0);
  HANDLE hndls[2];
  hndls[0]=hKernelShutdownReq;
  hndls[1]=hStdin;
  do
  { DWORD res=WaitForMultipleObjects(2, hndls, FALSE, INFINITE);
    if (res==WAIT_OBJECT_0)  // hKernelShutdownReq
      break;
    if (res==WAIT_OBJECT_0+1)  // input event
    { INPUT_RECORD irBuffer;  DWORD dwCount;
      ReadConsoleInput(hStdin, &irBuffer, 1, &dwCount);
      if (dwCount == 1 && irBuffer.EventType == KEY_EVENT && irBuffer.Event.KeyEvent.bKeyDown)
        process_input(irBuffer.Event.KeyEvent.uChar.AsciiChar);
    }
  } while (true);
#else
  WaitLocalAutoEvent(&hKernelShutdownReq, INFINITE);  // [down_reason] is defined when the event becomes signalled
#endif  
  if (down_reason==down_from_client) 
    Sleep(100);  // this prevents the DOWN request to return the REQUEST_BREAKED error message 
  dir_kernel_close();
#if defined(_DEBUG) && !defined(X64ARCH)
  stackwalk_unprepare();
#endif
  return 0;  
}
#endif  // !ANY_SERVER_GUI

