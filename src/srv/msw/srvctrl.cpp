
#include "htmlhelp.h"
////////////////////////////// globals for server & service /////////////////
HINSTANCE hKernelLibInst;
HINSTANCE hInst;
void StopServerOrService(void);
bool iconic_main_window = true;
UINT uBreakMessage, uRestartMessage;
HWND hwMain = 0;
tobjname loc_server_name; // used by the shell of the server before the server is initialised

// copy from wprez.h:
#define WM_SZM_BASE          WM_USER+500
#define SZM_UNLINK           WM_SZM_BASE+211

#if WBVERS<900
typedef bool WINAPI t_ServerLicWizard(HWND hParent, char * buf);
static HINSTANCE hVeLib = 0;
#endif

bool check_server_licence(void)
// Returns true iff can start the server. Always true since 9.0
{ 
#if 0
  while (!check_server_registration())
//    if (DialogBox(hInst, MAKEINTRESOURCE(DLG_REGISTER_SERVER), 0, LicenceDlgProc) != TRUE)
//      return false;
  { if (!hVeLib) 
    { hVeLib = LoadLibrary("602dvlp8.DLL");
      if (!hVeLib) { MessageBeep(-1);  return false; }
    }
    t_ServerLicWizard * p_ServerLicWizard = (t_ServerLicWizard*)GetProcAddress(hVeLib, "ServerLicWizard");
    if (!p_ServerLicWizard) { MessageBeep(-1);  return false; }
    char buf[MAX_LICENCE_LENGTH+1];
    return (*p_ServerLicWizard)(0, buf);
  }
#endif
  return true;
}

/////////////////////////////// info window //////////////////////////////////
void reset_service_gui(void);

HWND    hInfoWnd = 0;
#define LIST_LIMIT 100

void write_to_server_console(cdp_t cdp, const char * text)
// [text] is translated and is in SQL system charset as specified in sys_spec. ATTN: sys_spec changes during the server startup!
{ 
  if (back_end_operation_on || fShutDown)
    if (cdp && cdp!=cds[0]) return;  // when the main thread is not in the message cycle, writing from other thread may deadlock the server
  while (*text=='\n') text++;
 // make space for the string in the list:
  if (!hInfoWnd) return;
  HWND hList = GetDlgItem(hInfoWnd, CD_SI_LOG);
  int count = (int)SendMessage(hList, LB_GETCOUNT, 0, 0);
  if (count >= LIST_LIMIT) SendMessage(hList, LB_DELETESTRING, 0, 0);
  int ind;
 // convert it from the SQL system charset to the window charset, then display:
  if (!(GetVersion() & 0x80000000U))
  { wchar_t * wbuf = new wchar_t[strlen(text)+1];
    superconv(ATT_STRING, ATT_STRING, text, (char*)wbuf, sys_spec.charset ? sys_spec : t_specif(0, CHARSET_NUM_UTF8, 0, 0), t_specif(0, 0, 0, 1), NULL);
    ind = (int)SendMessageW(hList, LB_ADDSTRING, 0, (LPARAM)wbuf);
    delete [] wbuf;
  }
  else
  { UINT acp = GetACP();  t_specif win_specif;
    switch (acp)
    { case 1250: win_specif.charset=1;  break;  // EE
      case 1252: win_specif.charset=3;  break;  // western Europe
      default:   win_specif.charset=1;  break;
    }
    if (sys_spec.charset!=win_specif.charset)
    { char * cbuf = new char[strlen(text)+1];
      superconv(ATT_STRING, ATT_STRING, text, (char*)cbuf, sys_spec.charset ? sys_spec : t_specif(0, CHARSET_NUM_UTF8, 0, 0), win_specif, NULL);
      ind = (int)SendMessage(hList, LB_ADDSTRING, 0, (LPARAM)cbuf);
      delete [] cbuf;
    }
    else
      ind = (int)SendMessage(hList, LB_ADDSTRING, 0, (LPARAM)text);
  }
 // select it:
  SendMessage(hList, LB_SETCURSEL, ind, 0);
}

typedef struct tagHH_LAST_ERROR
{ int      cbStruct ;
  HRESULT  hr ;            
  BSTR     description ;      
} HH_LAST_ERROR ;

void server_help(HWND hWnd, int htype, DWORD data)
// Supports only file-independent htypes, like HH_DISPLAY_TEXT_POPUP
{ 
  if (!HtmlHelp(hWnd, NULL, htype, data))
  { HH_LAST_ERROR lasterror;
    lasterror.cbStruct=sizeof(lasterror);
    HtmlHelp(hWnd, NULL, HH_GET_LAST_ERROR, (DWORD)&lasterror);
  }
}

void server_help_popup(HWND hItem, DWORD context_id)
{ HH_POPUP popup;  char text[1000];  
  if (!context_id) return;  // help not specified for the control, displays error message if HtmlHelp called
  LoadString(hInst, POP_BASE+context_id, text, sizeof(text));
  popup.cbStruct = sizeof(popup);
  popup.hinst=0; //hPrezInst;  the length of a directly loaded text is limited by Win32 to 255 characters
  popup.idString=0; // /*POP_BASE+*/context_id;
  popup.pszText=text;
  GetCursorPos(&popup.pt);
  popup.clrForeground=popup.clrBackground=-1;
  popup.rcMargins.top=popup.rcMargins.left=popup.rcMargins.bottom=popup.rcMargins.right=-1;
#ifdef ENGLISH
  popup.pszFont="MS Sans Serif, 8";
#else
  popup.pszFont="MS Sans Serif, 8, 238";
#endif
  server_help(hItem, HH_DISPLAY_TEXT_POPUP, (DWORD)&popup);
}

BOOL local_process_context_menu(HWND hDlg, WPARAM wP, LPARAM lP)
{ HWND hItem=(HWND)wP;
  if (hItem==hDlg)  // static controls not found by Windows nor by WindowFromPoint!
  { RECT Rect;
    HWND hChild = GetWindow(hDlg, GW_CHILD);
    while (hChild)
    { GetWindowRect(hChild, &Rect);
      if (LOWORD(lP)>=Rect.left && LOWORD(lP)<Rect.right &&
          HIWORD(lP)>=Rect.top  && HIWORD(lP)<Rect.bottom) break;
      hChild = GetWindow(hChild, GW_HWNDNEXT);
    }
    hItem = hChild ? hChild : hDlg;
  }
  server_help_popup(hItem, GetWindowContextHelpId(hItem));
  return TRUE;
}

BOOL WINAPI local_process_context_popup(const HELPINFO * hi)
{ if ((sig16)hi->iCtrlId<0) return FALSE; // does not work for negative values, even if sign extended
  server_help_popup((HWND)hi->hItemHandle, hi->dwContextId);
  return TRUE;
}



void WriteSlaveThreadCounter(void)
// Updates various information on the server console.
{ 
  if (hInfoWnd && !fShutDown)  // must not write during ShutDown, it would lock the thread (because the server's message loop is already closed)
  { char buf[50];
   // client count:
    client_count_string(buf);
    SetDlgItemText(hInfoWnd, CD_SI_USERS, buf);
   // replication:
    int ip_transport, repl_in, repl_out;
    get_repl_state(&ip_transport, &repl_in, &repl_out);
    ShowWindow(GetDlgItem(hInfoWnd, CD_SI_TRANSPORT), ip_transport ? SW_SHOWNA : SW_HIDE);
    ShowWindow(GetDlgItem(hInfoWnd, CD_SI_REPLIN),    repl_in      ? SW_SHOWNA : SW_HIDE);
    ShowWindow(GetDlgItem(hInfoWnd, CD_SI_REPLOUT),   repl_out     ? SW_SHOWNA : SW_HIDE);
   // protocols and HTTP tunnelling:
    *buf=0;
    if (hThreadMakerThread) strcpy(buf, "ipc ");
    if (wProtocol & IPXSPX)  strcat(buf, "IPX/SPX ");
    if (wProtocol & NETBIOS) strcat(buf, "NetBIOS ");
    if (wProtocol & TCPIP_NET)  
      sprintf(buf+strlen(buf), "TCP/IP(%u) ", the_sp.wb_tcpip_port.val());
    if (wProtocol & TCPIP_HTTP)
      sprintf(buf+strlen(buf), "http(%u) ", http_tunnel_port_num);
    if (wProtocol & TCPIP_WEB)
      sprintf(buf+strlen(buf), "www(%u) ", http_web_port_num);
    SetDlgItemText(hInfoWnd, CD_SI_PROT, buf);
  }
}

static RECT OldRect ={ 0, 0, 150, 150} ;

HWND GetItemRect(HWND hDlg, UINT ID, LPRECT Rect)
{ HWND hCtrl = GetDlgItem(hDlg, ID);
  GetWindowRect(hCtrl, Rect);
  ScreenToClient(hDlg, (LPPOINT)&Rect->left);
  ScreenToClient(hDlg, (LPPOINT)&Rect->right);
  Rect->right  -= Rect->left;
  Rect->bottom -= Rect->top;
  return hCtrl;
}

///////////////////////////////// Server management console thread ////////////////////////////////
/* The server managenement cosole must be opened from a separate thread, otherwise server locks when
   the console client attaches to the server, the client count is being updated and two threads 
   are trying to be inside the dialog procedure of the server window.

   If the console is open then hSrvCons!=0 and it cannot be reopened.
   When console is closed then hSrvCons becomes 0 and ConsoleThread terminates.
   When server terminates, it posts WM_QUIT to ConsoleThread and terminates it.
*/
#if WBVERS<900

static HWND hSrvCons = 0;

typedef void WINAPI t_Control_server(HWND hParent, const char * server_name, HWND * hSrvCons);

THREAD_PROC(ConsoleThread)
{
  t_Control_server * p_Control_server = (t_Control_server*)GetProcAddress(hVeLib, "Control_server");
  if (p_Control_server)
  { (*p_Control_server)((HWND)data, loc_server_name, &hSrvCons);
    if (hSrvCons)
    { MSG msg;
      while (hSrvCons && GetMessage(&msg, NULL, 0, 0))
      { TranslateMessage(&msg);
        DispatchMessage(&msg);
      }
      if (hSrvCons) DestroyWindow(hSrvCons);  // WM_QUIT posted from the main server thread 
    }
  }
  THREAD_RETURN;
}
#endif

////////////////////////////// checking clients on server stop /////////////////////////////////
static BOOL nobody_connected(void)
{ if (!slaveThreadCounter /*|| slaveThreadCounter==1 && hSrvCons*/) return TRUE;  
/* The 2nd part of the condition causes a problem: 
When a client and the console are connected to th server
and the console disconnects (without being closed), server terminates! */
 // checking for weak clients:
  return get_external_client_connections()==0;
}

#if WBVERS>=900

static BOOL CALLBACK ServerStopDlgProc(HWND hDlg, UINT msg, WPARAM wP, LPARAM lP)
{ switch (msg)
  { case WM_INITDIALOG:
    { 
#ifdef ENGLISH
      char transbuf[80];
      SetDlgItemText(hDlg, 32000,               form_terminal_message(transbuf, sizeof(transbuf), _("Clients are connected to the server. What do you want to do:")));
      SetDlgItemText(hDlg, CD_STOP_SERVER_WARN, form_terminal_message(transbuf, sizeof(transbuf), _("Send a message to all interactive clients and shutdown the server in 1 minute")));
      SetDlgItemText(hDlg, CD_STOP_SERVER_IMM,  form_terminal_message(transbuf, sizeof(transbuf), _("Stop the server immediately")));
      SetDlgItemText(hDlg, IDOK,                form_terminal_message(transbuf, sizeof(transbuf), _("&OK")));
      SetDlgItemText(hDlg, IDCANCEL,            form_terminal_message(transbuf, sizeof(transbuf), _("&Cancel")));
      SetWindowText(hDlg,                       form_terminal_message(transbuf, sizeof(transbuf), _("Stopping the SQL server")));
#endif
      CheckRadioButton(hDlg, CD_STOP_SERVER_WARN, CD_STOP_SERVER_IMM, CD_STOP_SERVER_WARN);
      return TRUE;
    }
    case WM_COMMAND:
      switch (GET_WM_COMMAND_ID(wP,lP))
      { case IDOK:
          if (IsDlgButtonChecked(hDlg, CD_STOP_SERVER_WARN))
          { client_logout_timeout=60;
            EndDialog(hDlg, 2);
          }
          else
          { client_logout_timeout=0;
            EndDialog(hDlg, 1);
          }
          return TRUE;
        case IDCANCEL:
          EndDialog(hDlg, 0);
          return TRUE;
      }
      break;
  }
  return FALSE;
}
#endif

static int can_stop_server(HWND hParent)
{ 
  if (back_end_operation_on)
  { MessageBox(hParent, "Information", "Cannot stop the server now, maintenance operations are in progress.", MB_OK | MB_APPLMODAL | MB_ICONHAND);
    return FALSE;
  }
  if (nobody_connected()) return TRUE;
 // anybody connected, ask the user:
#if WBVERS<900
  return MessageBox(hParent, CLOSE_WARNING, SERVER_WARNING_CAPT, MB_YESNO | MB_APPLMODAL | MB_ICONHAND) == IDYES;
#else
  return (int)DialogBox(hInst, MAKEINTRESOURCE(DLG_STOP_USERS), hParent, ServerStopDlgProc);
#endif
}

////////////////////////////// server information dialog //////////////////////////////////////
static BOOL CALLBACK ServerInfoDlgProc(HWND hDlg, UINT msg, WPARAM wP, LPARAM lP)
{ switch (msg)
  { case WM_INITDIALOG:
    { SendDlgItemMessage(hDlg, CD_SI_LOG, LB_SETHORIZONTALEXTENT, 3000, 0);
      //EnableWindow(GetDlgItem(hDlg, CD_SI_DOWN), GetWindow(hDlg, GW_OWNER)!=0);
     // server version:
#if WBVERS<900
      char buf[60];
      sprintf(buf, "%s   (build %u)", WB_VERSION_STRING_EX, VERS_4);
      SetDlgItemText(hDlg, CD_SI_VERSION, buf);
#else
      SetDlgItemText(hDlg, CD_SI_VERSION, VERS_STR);
#endif
     // bitmaps:
      SendDlgItemMessage(hDlg, CD_SI_TRANSPORT, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)LoadBitmap(hInst, MAKEINTRESOURCE(BITMAP_TRANSPORT)));
      SendDlgItemMessage(hDlg, CD_SI_REPLIN,    STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)LoadBitmap(hInst, MAKEINTRESOURCE(BITMAP_REPLIN)));
      SendDlgItemMessage(hDlg, CD_SI_REPLOUT,   STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)LoadBitmap(hInst, MAKEINTRESOURCE(BITMAP_REPLOUT)));
     // write old server messages:
      int pos = -1;  const char * msg;
      do
      { msg = enum_server_messages(pos);
        if (msg) SendDlgItemMessage(hDlg, CD_SI_LOG, LB_ADDSTRING, 0, (LPARAM)msg);
      } while (msg);
      hInfoWnd=hDlg;
      WriteSlaveThreadCounter();  // init
#if WBVERS<900
      OldRect.top=OldRect.right=0;  OldRect.right=330;  OldRect.bottom=285;
#else
      OldRect.top=OldRect.right=0;  OldRect.right=335;  OldRect.bottom=265;
#endif
     // translate texts:
#ifdef ENGLISH
      char transbuf[80];
      SetDlgItemText(hDlg, 32000,          form_terminal_message(transbuf, sizeof(transbuf), _("602SQL Server version:")));
      SetDlgItemText(hDlg, 32001,          form_terminal_message(transbuf, sizeof(transbuf), _("Network protocol(s):")));
      SetDlgItemText(hDlg, 32002,          form_terminal_message(transbuf, sizeof(transbuf), _("Clients connected:")));
      SetDlgItemText(hDlg, 32003,          form_terminal_message(transbuf, sizeof(transbuf), _("Client licenses:")));
      SetDlgItemText(hDlg, CD_SI_LOG,      form_terminal_message(transbuf, sizeof(transbuf), _("Event log")));
#if WBVERS<1000
      SetDlgItemText(hDlg, CD_SI_ERRORS,   form_terminal_message(transbuf, sizeof(transbuf), _("&Log all client errors")));
#endif
#if WBVERS<900
      SetDlgItemText(hDlg, CD_SI_REPLIMON, form_terminal_message(transbuf, sizeof(transbuf), _("Lo&g replication")));
      SetDlgItemText(hDlg, CD_SI_CONSOLE,  form_terminal_message(transbuf, sizeof(transbuf), _("Co&ntrol console")));
#endif
      SetDlgItemText(hDlg, CD_SI_CLEAR,    form_terminal_message(transbuf, sizeof(transbuf), _("Cle&ar list")));
      SetDlgItemText(hDlg, CD_SI_DOWN,     form_terminal_message(transbuf, sizeof(transbuf), _("&Stop server")));
#endif
      return TRUE;
    }
    case WM_SIZE:
    { if (wP == SIZE_MINIMIZED) break;
      if (!OldRect.right) break;
      int ncx = LOWORD(lP), ncy = HIWORD(lP);
      if (!ncx || !ncy) break;
      RECT cRect;
      HWND hCtrl = GetItemRect(hDlg, CD_SI_LOG, &cRect);
      MoveWindow(hCtrl, cRect.left, cRect.top, cRect.right + ncx - OldRect.right, cRect.bottom + ncy - OldRect.bottom, FALSE);
#if WBVERS<1000
      hCtrl = GetItemRect(hDlg, CD_SI_ERRORS, &cRect);
      MoveWindow(hCtrl, cRect.left, cRect.top + ncy - OldRect.bottom, cRect.right, cRect.bottom, FALSE);  
#endif
#if WBVERS<900
      hCtrl = GetItemRect(hDlg, CD_SI_REPLIMON, &cRect);
      MoveWindow(hCtrl, cRect.left, cRect.top + ncy - OldRect.bottom, cRect.right, cRect.bottom, FALSE);  
      hCtrl = GetItemRect(hDlg, CD_SI_CONSOLE, &cRect);
      MoveWindow(hCtrl, cRect.left, cRect.top + ncy - OldRect.bottom, cRect.right, cRect.bottom, FALSE);  
#endif
      hCtrl = GetItemRect(hDlg, CD_SI_CLEAR, &cRect);
      MoveWindow(hCtrl, cRect.left, cRect.top + ncy - OldRect.bottom, cRect.right, cRect.bottom, FALSE);  
      hCtrl = GetItemRect(hDlg, CD_SI_DOWN, &cRect);
      MoveWindow(hCtrl, cRect.left, cRect.top + ncy - OldRect.bottom, cRect.right, cRect.bottom, FALSE);  
      OldRect.right  = ncx;
      OldRect.bottom = ncy;
      InvalidateRect(hDlg, NULL, TRUE);
      break;
    }
    case WM_HELP:
      return local_process_context_popup((HELPINFO*)lP);
    case WM_CONTEXTMENU:
      return local_process_context_menu(hDlg, wP, lP);
    case WM_SHOWWINDOW:  // protocols may not be defined when dialog is opened!
      if (wP)  // showing
      { char buf[260];
       // user limit:
        unsigned total, used;
        client_licences.get_info(&total, &used);
        if (total>=999) form_terminal_message(buf, sizeof(buf), unlimitedLics);
        else int2str(total, buf);
#if WBVERS>=810
        if (fulltext_allowed) strcat(buf, " + FullText + XML");
#else
        if (fulltext_allowed) strcat(buf, " + FullText");
#endif
        SetDlgItemText(hDlg, CD_SI_MAXUSERS, buf);
       // checks:
#if WBVERS<1000
        CheckDlgButton(hDlg, CD_SI_ERRORS,   the_sp.trace_user_error.val());
#endif
#if WBVERS<900
        CheckDlgButton(hDlg, CD_SI_REPLIMON, the_sp.trace_replication.val());
#endif
        WriteSlaveThreadCounter();
      }
      return TRUE;
    case WM_DESTROY:
      DeleteObject((HGDIOBJ)SendDlgItemMessage(hDlg, CD_SI_TRANSPORT, STM_GETIMAGE, IMAGE_BITMAP, 0));
      DeleteObject((HGDIOBJ)SendDlgItemMessage(hDlg, CD_SI_REPLIN,    STM_GETIMAGE, IMAGE_BITMAP, 0));
      DeleteObject((HGDIOBJ)SendDlgItemMessage(hDlg, CD_SI_REPLOUT,   STM_GETIMAGE, IMAGE_BITMAP, 0));
      HtmlHelp(NULL, NULL, HH_CLOSE_ALL, 0);
      hInfoWnd=0;
      break;
    case WM_COMMAND:
      switch (GET_WM_COMMAND_ID(wP,lP))
      { 
#if WBVERS<900
        case CD_SI_CONSOLE:
          if (GetKeyState(VK_CONTROL) & 0x8000)
            { PrintAttachedUsers();  break; }
          if (hSrvCons)
            BringWindowToTop(hSrvCons);
          else
          { if (!hVeLib) 
            { hVeLib = LoadLibrary("602dvlp8.DLL");
              if (!hVeLib) { MessageBeep(-1);  break; }
            }
            THREAD_START(ConsoleThread, 0, hDlg, NULL);  // thread handle closed inside
          }
          break;
#endif
        case CD_SI_CLEAR:
          if (GetKeyState(VK_CONTROL) & 0x8000)
            if (GetKeyState(VK_SHIFT) & 0x8000)
            { ffa.open_fil_access(my_cdp, server_name);  // important when (the_sp.CloseFileWhenIdle.val()!=0)
              HCURSOR hPrevCursor = SetCursor(LoadCursor(NULL, IDC_WAIT)); 
              ffa.fil_consistency();
              int cache_error, index_error;
              check_all_indicies(my_cdp, false, INTEGR_CHECK_INDEXES|INTEGR_CHECK_CACHE, false, cache_error, index_error);
              SetCursor(hPrevCursor);
              display_server_info("Index check completed.");
              ffa.close_fil_access(my_cdp);
            }
            else
            { ffa.open_fil_access(my_cdp, server_name);  // important when (the_sp.CloseFileWhenIdle.val()!=0)
              psm_cache.destroy();
              uninst_tables(my_cdp);
              psm_cache.destroy();
              ffa.close_fil_access(my_cdp);
            }
          else SendDlgItemMessage(hDlg, CD_SI_LOG, LB_RESETCONTENT, 0, 0);
          break;
#if WBVERS<1000
        case CD_SI_ERRORS:
          ffa.open_fil_access(my_cdp, server_name);  // important when (the_sp.CloseFileWhenIdle.val()!=0)
          trace_def(my_cdp, TRACE_USER_ERROR, NULLSTRING, NULLSTRING, NULLSTRING, IsDlgButtonChecked(hDlg, CD_SI_ERRORS));
          commit(my_cdp);
          ffa.close_fil_access(my_cdp);
          break;
#endif
#if WBVERS<900
        case CD_SI_REPLIMON:
          ffa.open_fil_access(my_cdp, server_name);  // important when (the_sp.CloseFileWhenIdle.val()!=0)
          trace_def(my_cdp, TRACE_REPL_CONFLICT, NULLSTRING, NULLSTRING, NULLSTRING, IsDlgButtonChecked(hDlg, CD_SI_REPLIMON));
          trace_def(my_cdp, TRACE_REPLICATION,   NULLSTRING, NULLSTRING, NULLSTRING, IsDlgButtonChecked(hDlg, CD_SI_REPLIMON));
          commit(my_cdp);
          ffa.close_fil_access(my_cdp);
          break;
#endif
        case CD_SI_DOWN:
		      if (GetKeyState(VK_CONTROL) & 0x8000)
          { char line[50];
            unsigned leak = leak_test();
			      PrintHeapState(display_server_info);
            if (leak)
			        display_server_info(form_message(line, sizeof(line), SERVER_LOST_BYTES, 16*leak));
            sprintf(line, "PID=%lu", GetCurrentProcessId());
            display_server_info(line);
            unsigned total, used;
            client_licences.get_info(&total, &used);
            sprintf(line, "Consumed %u licences from %u in the pool.", used, total);
            display_server_info(form_message(line, sizeof(line), LICS_CONSUMED, used, total));
          }
          else if (can_stop_server(hDlg))
            StopServerOrService();
          break;
        default: return FALSE;
      }
      break;
    case WM_USERCHANGED:
      reset_service_gui();
      break;
    default:
      return FALSE; // not processed
  }
  return TRUE;  // processed
}

void open_server_info(void)
{ ShowWindow(hwMain, SW_SHOW);
  OpenIcon(hwMain);
  SetForegroundWindow(hwMain);
  SetActiveWindow(hwMain);
  BringWindowToTop(hwMain);
}

//////////////////////////// taskbar icon ///////////////////////////////////
typedef BOOL WINAPI t_sni(DWORD dwMessage, NOTIFYICONDATA* lpData);

static BOOL MyNotifyIcon(DWORD code, NOTIFYICONDATA * tnid)
// Utility for adding and deleting the taskbar icon
{ BOOL res;  t_sni * proc;
  HINSTANCE hLibInst=LoadLibrary("SHELL32.DLL");
  if (hLibInst < (HINSTANCE)HINSTANCE_ERROR) return FALSE;
  proc=(t_sni*)GetProcAddress(hLibInst, "Shell_NotifyIconA");
  if (proc==NULL) { FreeLibrary(hLibInst);  return FALSE; }
  res = proc(code, tnid);
  FreeLibrary(hLibInst);
  return res;
}

UINT uTaskBarIconMessage = 0;
#define WBSERVER_TBICON_ID 1

// MyTaskBarAddIcon - adds an icon to the taskbar status area.
// Returns TRUE if successful or FALSE otherwise.
// hwnd - handle of the window to receive callback messages
// lpszTip - ToolTip text
BOOL MyTaskBarAddIcon(HWND hwnd, LPSTR lpszTip)
{ NOTIFYICONDATA tnid;
  uTaskBarIconMessage = RegisterWindowMessage("WBTaskBarMessage");
  tnid.cbSize = sizeof(NOTIFYICONDATA);
  tnid.hWnd = hwnd;
  tnid.uID = WBSERVER_TBICON_ID;
  tnid.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
  tnid.uCallbackMessage = uTaskBarIconMessage;
  if (lpszTip) strmaxcpy(tnid.szTip, lpszTip, sizeof(tnid.szTip));
  else tnid.szTip[0] = '\0';
  HICON hIcon=0;  int i;
  if (!hInst) { MessageBeep(-1);  Sleep(100);  MessageBeep(-1);  Sleep(100); }
  for (i=0;  i<20;  i++)
  { if (!hIcon) hIcon=LoadIcon(hInst, "SERVER_ICON"); 
    tnid.hIcon = hIcon;
    if (hIcon)
      if (MyNotifyIcon(NIM_ADD, &tnid)) break;
    Sleep(1500);
  }
  if (hIcon) DestroyIcon(hIcon);
  return i<20;
}

// MyTaskBarDeleteIcon - deletes an icon from the taskbar status area.
// Returns TRUE if successful or FALSE otherwise.
// hwnd - handle of the window that added the icon
void MyTaskBarDeleteIcon(HWND hwnd)
{ if (!uTaskBarIconMessage) return;  // deleted before
  NOTIFYICONDATA tnid;
  tnid.cbSize = sizeof(NOTIFYICONDATA);
  tnid.hWnd = hwnd;
  tnid.uID = WBSERVER_TBICON_ID;
  MyNotifyIcon(NIM_DELETE, &tnid);
  uTaskBarIconMessage=0;  // says that tray icon does not exist
}

// Processes callback messages for taskbar icon
// wParam - first message parameter of the callback message
// lParam - second message parameter of the callback message
void On_TBICON_NOTIFYICON(WPARAM wParam, LPARAM lParam, char * name)
{ UINT uID = (UINT)wParam;
  UINT uMouseMsg  = (UINT)lParam;
  if (uID!=WBSERVER_TBICON_ID) return;
  if (uMouseMsg == WM_LBUTTONDOWN)  // does not work when the window is open, BUTTONUP does not work either
    open_server_info();
  else if (uMouseMsg == WM_LBUTTONDBLCLK)  // does not work when the window is open
    open_server_info();
  else if (uMouseMsg == WM_RBUTTONDOWN)
  { POINT pt;  GetCursorPos(&pt);
    SetForegroundWindow(hwMain);
    HMENU hMenu = CreatePopupMenu();
    char itemtext[30];
    AppendMenu(hMenu, MF_STRING,    1, form_terminal_message(itemtext, sizeof(itemtext), TASKBAR_MENU_ITEM_OPEN));
    AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);
    AppendMenu(hMenu, MF_STRING,    2, form_terminal_message(itemtext, sizeof(itemtext), TASKBAR_MENU_ITEM_DOWN));
    int cmd = TrackPopupMenu(hMenu, TPM_RIGHTALIGN | TPM_RIGHTBUTTON | TPM_NONOTIFY | TPM_RETURNCMD, pt.x, pt.y, 0, hwMain, NULL);
    DestroyMenu(hMenu);
    if      (cmd==1) open_server_info();
    else if (cmd==2) 
    { if (can_stop_server(hwMain))
        StopServerOrService();
    }
  }
}

void reset_service_gui(void) // is not called
{ //MessageBox(0, "RESET GUI", " ", MB_OK | MB_APPLMODAL);
  MessageBeep(-1);
  MyTaskBarAddIcon(hwMain, loc_server_name);
}

CFNC LONG WINAPI ServerMainWndProc(HWND hWnd,UINT msg,WPARAM wParam,LPARAM lParam)
{ if (msg == uBreakMessage)
		send_break_req(lParam);
  else if (msg == uRestartMessage)
  { dir_kernel_close();
    kernel_init(my_cdp, server_name, network_server);
  }
	else if (msg == uTaskBarIconMessage && uTaskBarIconMessage)
    On_TBICON_NOTIFYICON(wParam, lParam, loc_server_name);
	else
  switch (msg)
  { case WM_CREATE:
#if WBVERS>=950
      EnableMenuItem(GetSystemMenu(hWnd, FALSE), SC_CLOSE, MF_BYCOMMAND|MF_DISABLED|MF_GRAYED);
#endif
      hInfoWnd=CreateDialog(hInst, MAKEINTRESOURCE(DLG_SERVER_INFO), hWnd, ServerInfoDlgProc); // must be created here, not by other thread!
      // openning server info dialog may fail if called before creating the desktop (when service started on server startup)
      break;
    case WM_SIZE:
      if (wParam==SIZE_MINIMIZED)  // from v. 9.5
        ShowWindow(hWnd, iconic_main_window ? SW_HIDE : SW_SHOWMINNOACTIVE);
      else if (wParam==SIZE_RESTORED)
      { RECT Rect;  GetClientRect(hWnd, &Rect);
        SetWindowPos(hInfoWnd, 0, 0, 0, Rect.right, Rect.bottom, SWP_NOZORDER | SWP_NOACTIVATE);
      }
      break;
    case WM_GETMINMAXINFO:
    { LPMINMAXINFO mmi = (LPMINMAXINFO)lParam;
      RECT dRect, lRect;
      if (hInfoWnd)
      { GetWindowRect(hInfoWnd, &dRect);
        GetWindowRect(GetDlgItem(hInfoWnd, CD_SI_LOG), &lRect);
        mmi->ptMinTrackSize.y = dRect.bottom - dRect.top - lRect.bottom + lRect.top + 2*GetSystemMetrics(SM_CYSIZEFRAME)+GetSystemMetrics(SM_CYCAPTION);
      }
      break;
    }
    case WM_SETFOCUS:
      if (hInfoWnd) { SetFocus(hInfoWnd);  break; }
      return DefWindowProc(hWnd, msg, wParam, lParam);
    case WM_ACTIVATE:
      if (wParam==WA_ACTIVE || wParam==WA_CLICKACTIVE)  // update check boxes (may have been changed by the API):
      { 
#if WBVERS<1000
        CheckDlgButton(hInfoWnd, CD_SI_ERRORS,   the_sp.trace_user_error.val());
#endif
#if WBVERS<900
        CheckDlgButton(hInfoWnd, CD_SI_REPLIMON, the_sp.trace_replication.val());
#endif
      }
      break;
    case WM_QUERYENDSESSION:
      if (is_service) return DefWindowProc(hWnd, msg, wParam, lParam);
      return can_stop_server(hWnd);
    case WM_ENDSESSION:  /* WM_DESTROY is not sended on ENDSESSION! */
      if (is_service) return DefWindowProc(hWnd, msg, wParam, lParam);
      if (wParam) 
      { down_reason=down_endsession;
        DestroyWindow(hWnd);
      }  
      return 0;
    case SZM_UNLINK: // signal from disconnecting local client
      if (is_dependent_server && nobody_connected())
      { down_reason=down_from_client;
        DestroyWindow(hWnd);
      }  
      break;                    
    case WM_CLOSE:  // used when task closed from the system menu on the task list
      ShowWindow(hWnd, iconic_main_window ? SW_HIDE : SW_SHOWMINNOACTIVE);  // does not stop the server, minimizes the window only
      break;
    case WM_DESTROY:  // the message is not delivered when the service is stopped from outside
#if WBVERS>=900  // closing server before closing the window
      if (!is_service) dir_kernel_close();
#endif
      MyTaskBarDeleteIcon(hWnd);  // deletes if exists
      PostQuitMessage(0);
      break;
    default: return DefWindowProc(hWnd, msg, wParam, lParam);
  }
  return 0;
}

void client_terminating(void)
{ PostMessage(hwMain, SZM_UNLINK, 0, 0);  // replaces cd_unlink_kernel, effective for both direct and network clients
}

static const char server_class_name[] = "WBServerClass32";

int common_prepare(void)
// hInst, loc_server_name must be defined before
{	uBreakMessage = RegisterWindowMessage("WBBreak_message"); // used by locally connected clients to send an async. break
 	uRestartMessage = RegisterWindowMessage("602SQLRestart_message");
 // register main window class
  WNDCLASS MainWndClass;                    // Main window class
  MainWndClass.lpszClassName = server_class_name;        // Window class
  MainWndClass.hInstance     = hInst;                    // Instance in in the global variable
  MainWndClass.lpfnWndProc   = ServerMainWndProc;        // Window function
  MainWndClass.style         = CS_HREDRAW | CS_VREDRAW;  // Horizontal and vertical
  MainWndClass.lpszMenuName  = NULL;                     // No menu
  MainWndClass.hCursor       = LoadCursor(NULL, IDC_ARROW);     // Mouse cursor
  MainWndClass.hIcon         = LoadIcon(hInst, "SERVER_ICON");
  MainWndClass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
  MainWndClass.cbClsExtra    = 0;                        // No extra bytes
  MainWndClass.cbWndExtra    = 0;                        // No extra bytes
  if (!RegisterClass(&MainWndClass)) return KSE_WINDOWS;  
 // prepare the caption:
  char server_caption[30+OBJ_NAME_LEN];
  form_terminal_message(server_caption, sizeof(server_caption), SERVER_CAPTION, loc_server_name);
  hwMain = CreateWindowEx(
#if WBVERS<950
  WS_EX_CONTEXTHELP, 
                        server_class_name, server_caption,         
                        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME, 
#else
  0,
                        server_class_name, server_caption,         
                        WS_OVERLAPPED | WS_MINIMIZEBOX | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME, 
#endif
                        CW_USEDEFAULT, CW_USEDEFAULT, 344, 340,
                        NULL,                   // No parent window
                        0,                      // No menu
                        hInst,              // Application instance
                        NULL);                  // No creation parameters
  if (!hwMain) return KSE_WINDOWS; 
  return 0;
}

