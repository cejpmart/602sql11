//  mngmain.cpp
#include "wx/wxprec.h"
#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif
#pragma hdrstop
#ifdef UNIX
#include <winrepl.h>
#endif
#include "wbkernel.h"
#include "mngmain.h"
#include "wbvers.h"
#include "ServerManagerPanel.h"
#include "mng16.xpm"
#include "mng32.xpm"

#include "regstr.cpp"
#define KEY_READ_EX KEY_READ //| KEY_WOW64_64KEY
#define KEY_ALL_ACCESS_EX KEY_ALL_ACCESS //| KEY_WOW64_64KEY
#define KEY_QUERY_VALUE_EX KEY_QUERY_VALUE //| KEY_WOW64_64KEY
#define KEY_SET_VALUE_EX KEY_SET_VALUE //| KEY_WOW64_64KEY
#define KEY_WRITE_EX KEY_WRITE //| KEY_WOW64_64KEY
#define KEY_ENUMERATE_SUB_KEYS_EX KEY_ENUMERATE_SUB_KEYS //| KEY_WOW64_64KEY
#include "enumsrv.c"

///////////////////////////////// locale ///////////////////////////////////////
#ifdef LINUX
#include <prefix.h>
#include <uxprofile.c>
#endif

static char * _ini_file_name = NULL;

void drop_ini_file_name(void)
{ delete [] _ini_file_name;  _ini_file_name=NULL; }

#ifdef LINUX

const char * ini_file_name(void)
{ if (!_ini_file_name)
  { const char * home = getenv("HOME");
    if (!home) home = "/tmp";
    _ini_file_name = new char[strlen(home)+10];
    strcpy(_ini_file_name, home);
    strcat(_ini_file_name, "/.602sql");
  }
  return _ini_file_name;
}

#else  // MSW

const char * ini_file_name(void)
{ if (!_ini_file_name)
  { HKEY hKey;  char buf[MAX_PATH];
    *buf=0;
    if (RegOpenKeyExA(HKEY_CURRENT_USER, "Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders", 0, KEY_READ_EX, &hKey)==ERROR_SUCCESS)
    { DWORD key_len=sizeof(buf);
      if (RegQueryValueExA(hKey, "AppData", NULL, NULL, (BYTE*)buf, &key_len)==ERROR_SUCCESS)
      { strcat(buf, "\\Software602");
        CreateDirectoryA(buf, NULL);
        strcat(buf, "\\602SQL");
        CreateDirectoryA(buf, NULL);
        strcat(buf, "\\");
      }
      RegCloseKey(hKey);
    }
    strcat(buf, "602sql.ini");
    _ini_file_name = new char[strlen(buf)+1];
    strcpy(_ini_file_name, buf);
  }
  return _ini_file_name;
}

#endif

bool read_profile_string(const char * section, const char * entry, char * buf, int bufsize)
{ GetPrivateProfileStringA(section, entry, "\1", buf, bufsize, ini_file_name());
  return *buf!='\1';
}

bool read_profile_int(const char * section, const char * entry, int * val)
{ char buf[20];
  if (!read_profile_string(section, entry, buf, sizeof(buf))) 
    return false;
  return sscanf(buf, " %d", val) == 1;
}

bool read_profile_bool(const char * section, const char * entry, bool Default)
{ char buf[20];
  if (!read_profile_string(section, entry, buf, sizeof(buf))) 
    return Default;
  int val;
  if (sscanf(buf, " %d", &val) == 1)
    return val != 0;
  else
    return Default;
}

wxLocale locale;

void set_language_support(void)
{ int client_language;
  if (!read_profile_int("Locale", "Language", &client_language)) client_language=wxLANGUAGE_DEFAULT;
  bool client_load_catalog=read_profile_bool("Locale", "LoadCatalog", true);
#ifdef WINS // lookup prefix is . by default, we must specify an fixed location!
  wxChar path[MAX_PATH];
  int len = GetModuleFileName(NULL, path, MAX_PATH);
  while (len && path[len-1]!='\\') len--;
  path[len]=0;
  locale.AddCatalogLookupPathPrefix(path); 
#else
  char path[MAX_PATH];
  strcpy(path, PREFIX);  // module is located in bin
  strcat(path, "/share/locale");  // looks in path/<language>/LC_MESSAGES
  locale.AddCatalogLookupPathPrefix(wxString(path, *wxConvCurrent));
#endif
  locale.Init(client_language==wxLANGUAGE_DEFAULT ? wxLocale::GetSystemLanguage() : client_language,
              client_load_catalog ? wxLOCALE_LOAD_DEFAULT | wxLOCALE_CONV_ENCODING : 0);
  // Must not pass wxLANGUAGE_DEFAULT as parameter: it is translated to	GetSystemLanguage() but
  // ... locale is not initialized properly then (windows do not get national charactes)		  
  if (client_load_catalog)   // must be before creating windows
    locale.AddCatalog(wxString(NAME_PREFIX "mng" NAME_SUFFIX, *wxConvCurrent));
}

////////////////////////////// main application and main window ///////////////////////////////////////
IMPLEMENT_APP(MngApp)

BEGIN_EVENT_TABLE(MngFrame, wxFrame )
  EVT_ICONIZE(MngFrame::OnIconize) 
END_EVENT_TABLE()

static wxString get_main_title(void) 
{ wxString title;
  title.Printf(_("602SQL Open Server Manager %u.%u"), VERS_1, VERS_2);
  return title;
}

void MngFrame::OnIconize(wxIconizeEvent & event)
{
#ifdef WINS
  if (event.Iconized()) Hide();
  else 
#endif  
    event.Skip();
}

MngFrame::MngFrame(void)
    : wxFrame(NULL, -1, get_main_title(), wxDefaultPosition, wxDefaultSize, 
#ifdef SYSTRAY_NO_TASKBAR
          wxFRAME_NO_TASKBAR |
#endif
          wxCLIP_CHILDREN | wxMINIMIZE_BOX | wxMAXIMIZE_BOX | wxTHICK_FRAME | wxSYSTEM_MENU | wxCAPTION | wxCLOSE_BOX, 
          wxT("602mng_frame") )
#ifdef SYSTRAY_NO_TASKBAR
, tbi(this)
#endif
{
  wxIconBundle main_icons;// = new wxIconBundle();
#ifdef LINUX
  main_icons.AddIcon(wxIcon(mng16_xpm));
#endif
  main_icons.AddIcon(wxIcon(mng32_xpm));
  SetIcons(main_icons);
  wxSizer * main_sizer = new wxBoxSizer(wxVERTICAL);
  SetSizer(main_sizer);
  ServerManagerPanel * panel = new ServerManagerPanel(this);
  main_sizer->Add(panel, 1, wxEXPAND, 0);
  Fit();
}

bool MngApp::OnInit(void)
{
  set_language_support();
  frame = new MngFrame;
	frame->Show(true);
#ifdef SYSTRAY_NO_TASKBAR
	frame->tbi.SetIcon(wxIcon(mng16_xpm), wxString(_("602SQL Open Server Manager")));
#endif
	return true;
}

BEGIN_EVENT_TABLE(MngTaskBarIcon, wxTaskBarIcon )
  EVT_TASKBAR_LEFT_DOWN(MngTaskBarIcon::OnLeftClick) 
  EVT_MENU(1, MngTaskBarIcon::OnRestoreCmd) 
  EVT_MENU(2, MngTaskBarIcon::OnCloseCmd) 
END_EVENT_TABLE()

void MngTaskBarIcon::OnLeftClick(wxTaskBarIconEvent & event)
{
  owner->Show();
  owner->Iconize(false);
  owner->Raise();
}

void MngTaskBarIcon::OnRestoreCmd(wxCommandEvent & event)
{
  owner->Show();
  owner->Iconize(false);
  owner->Raise();
}

void MngTaskBarIcon::OnCloseCmd(wxCommandEvent & event)
{
  owner->Destroy();
}

wxMenu * MngTaskBarIcon::CreatePopupMenu(void)
{ wxMenu * menu = new wxMenu;
  menu->Append(1, _("Restore"));
  menu->Append(2, _("Close"));
  return menu;
}

void t_server_info::get_server_state(void)
// Determines if the server is running
{
  int state = srv_Get_state_local(name);
  running = state==1 || state==2;
  if (!srv_Get_service_name(name, service_name))
    *service_name=0;
}

t_server_info * loc_server_list = NULL;
int server_count = 0;

int get_server_list_index_from_combo_selection(const ServersOwnerDrawnComboBox * combo, int sel)
{ int i;
  if (sel==-1) return -1;
  wxString str = combo->GetString(sel);
  i=0;
  while (str!=wxString(loc_server_list[i].name, *wxConvCurrent))
    if (++i>=server_count) return -1;
  return i;
}    
/////////////////////////////// class ServersOwnerDrawnComboBox //////////////////////////
void ServersOwnerDrawnComboBox::OnDrawItem(wxDC& dc, const wxRect& rect, int item, int flags) const
{ int ind = get_server_list_index_from_combo_selection(this, item);
  if (ind!=-1 && !(flags & wxODCB_PAINTING_CONTROL))
    dc.SetTextForeground(loc_server_list[ind].running ? wxColour(0,128,0) : *wxRED);
  wxOwnerDrawnComboBox::OnDrawItem(dc, rect, item, flags);
}
/////////////////////////////// operations /////////////////////////////////////
void ServerManagerPanel::refresh_server_list(void)
{ wxString selected_name;
 // save name of the selected server and clear the list:
  selected_name = mServers->GetStringSelection();
  mServers->Clear();
 // count registered servers:
  prep_regdb_access();
  int new_server_count = 0;
  tobjname a_server_name;  char a_path[MAX_PATH];
  { t_enum_srv es(t_enum_srv::ENUM_SRV_BOTH, CURRENT_FIL_VERSION_STR);
    while (es.next(a_server_name))
    { es.GetPrimaryPath(a_server_name, a_path);
      if (*a_path) new_server_count++;
    }
  }  
 // allocate a new list of servers:
  t_server_info * new_server_list = new t_server_info[new_server_count];
  if (!new_server_list) return;
 // get server list: 
  int i=0;
  { t_enum_srv es(t_enum_srv::ENUM_SRV_BOTH, CURRENT_FIL_VERSION_STR);
    while (i<new_server_count && es.next(a_server_name))
    { es.GetPrimaryPath(a_server_name, a_path);
      if (*a_path) 
      { strcpy(new_server_list[i].name, a_server_name);
        if (!es.GetDatabaseString(a_server_name, IP_str, new_server_list[i].addr, sizeof(new_server_list[i].addr))) 
          *new_server_list[i].addr=0;
        if (!es.GetDatabaseString(a_server_name, IP_socket_str, &new_server_list[i].port, sizeof(new_server_list[i].port))) 
          new_server_list[i].port=DEFAULT_IP_SOCKET;
        i++;
      }    
    }
  } 
  new_server_count=i;  // the actual number may be smaller than in the previous cout
 // check server's state and add them to the combo:
  int last_running = wxNOT_FOUND;
  i=0;
  while (i<new_server_count)
  { new_server_list[i].get_server_state();
    if (new_server_list[i].running)
      last_running=i;
    mServers->Append(wxString(new_server_list[i].name, *wxConvCurrent));
    i++;
  }  
 // replace lists:
  delete [] loc_server_list;  loc_server_list=new_server_list;
  server_count=new_server_count;
 // re-select the original server:
  int sel = mServers->FindString(selected_name);
  if (sel==wxNOT_FOUND) 
    if (last_running != wxNOT_FOUND)
      sel = mServers->FindString(wxString(new_server_list[last_running].name, *wxConvCurrent));
    else if (new_server_count>0)
      sel=0;
  mServers->SetSelection(sel);  
 // update server info:
  show_selected_server_info();
}

void ServerManagerPanel::show_selected_server_info(void)
{
  int i=mServers->GetSelection();
  if (i==wxNOT_FOUND)
  { mStartTask->Disable();  mPass->Disable();  mDaemon->Disable();
    mStop->Disable();
    mLog->Clear();
    mInfo1->SetLabel(wxEmptyString);
    mInfo2->SetLabel(wxEmptyString);
    mInfo3->SetLabel(wxEmptyString);
    mInfo4->SetLabel(wxEmptyString);
    mInfo5->SetLabel(wxEmptyString);
    return;
  }
  i = get_server_list_index_from_combo_selection(mServers, i);
  if (i<0) return;  // fuse
  loc_server_list[i].get_server_state(); // re-detect the server:
 // show run data:
  if (!loc_server_list[i].running)
  { mStartTask->Enable();
    mPass->Enable();  
#if 0  // enabled only when the service is registered
    mDaemon->Enable(*loc_server_list[i].service_name!=0);
#else  // always enabled, auto-registering the service  
    mDaemon->Enable();
#endif
    mStop->Disable();
    mInfo1->SetLabel(_("Server is not running"));
    mInfo2->SetLabel(wxEmptyString);
    mInfo3->SetLabel(wxEmptyString);
    mInfo4->SetLabel(wxEmptyString);
    mInfo5->SetLabel(wxEmptyString);
    mLog->Clear();
  }
  else  
  { mStartTask->Disable();  mPass->Disable();  mDaemon->Disable();
    mStop->Enable();
    mLog->Clear();
    mInfo1->SetLabel(wxEmptyString);
    mInfo2->SetLabel(wxEmptyString);
   // connect for the other information:
    cd_t cd;
    cdp_t cdp = &cd;
    cdp_init(cdp);
    if (cd_connect(cdp, loc_server_list[i].name, 0) == KSE_OK)
    { uns32 daemon, pid;
      wxString msg, part;
      cd_Get_server_info(cdp, OP_GI_DAEMON,     &daemon,    sizeof(daemon));
      cd_Get_server_info(cdp, OP_GI_PID,        &pid,       sizeof(pid));
      msg.Printf(daemon ?
#ifdef WINS
          _("The server is running as a system service (PID=%u).")
#else
          _("The server is running as a daemon (PID=%u).")
#endif      
        : _("The server is running (PID=%u)."), pid);
      mInfo1->SetLabel(msg);
     // version: 
      unsigned vers1, vers2, vers4;
      cd_Get_server_info(cdp, OP_GI_VERSION_1,  &vers1,     sizeof(vers1));
      cd_Get_server_info(cdp, OP_GI_VERSION_2,  &vers2,     sizeof(vers2));
      cd_Get_server_info(cdp, OP_GI_VERSION_4,  &vers4,     sizeof(vers4));
      msg.Printf(_("Version %u.%u, build %u"), vers1, vers2, vers4);
      mInfo2->SetLabel(msg);
     // clients: 
      unsigned clients, uptime, ipc, tcpip, http, web;
      if (vers1<11 || cd_Get_server_info(cdp, OP_GI_CLIENT_COUNT, &clients, sizeof(clients)))
      { curr_clients=clients=0;
        mInfo3->SetLabel(wxEmptyString);
      }
      else
      { curr_clients=clients-1;
        msg.Printf(_("Connected clients: %u"), curr_clients);  
        mInfo3->SetLabel(msg);
      }  
     // uptime
      if (cd_Get_server_info(cdp, OP_GI_SERVER_UPTIME, &uptime, sizeof(uptime)))
        mInfo4->SetLabel(wxEmptyString);
      else
      { msg.Printf(_("Uptime: %u hours, %u minutes"), uptime/3600, uptime / 60 % 60);  
        mInfo4->SetLabel(msg);
      }  
     // interfaces:
      char port_tcpip[10], port_http[10], port_web[10];
      cd_Get_server_info(cdp, OP_GI_IPC_RUNNING,  &ipc,     sizeof(ipc));
      cd_Get_server_info(cdp, OP_GI_NET_RUNNING,  &tcpip,   sizeof(tcpip));
      cd_Get_server_info(cdp, OP_GI_WEB_RUNNING,  &web,     sizeof(web));
      cd_Get_server_info(cdp, OP_GI_HTTP_RUNNING, &http,    sizeof(http));
      if (tcpip)
        cd_Get_property_value(cdp, "@SQLSERVER", "IPPort",         0, port_tcpip, sizeof(port_tcpip));
      if (http)
        cd_Get_property_value(cdp, "@SQLSERVER", "HTTPTunnelPort", 0, port_http,  sizeof(port_http));
      if (web)
        cd_Get_property_value(cdp, "@SQLSERVER", "WebPort",        0, port_web,   sizeof(port_web));
      msg=_("Interfaces:");
      if (ipc)
        msg=msg+wxT(" ")+wxT("IPC");
      if (tcpip)
        { part.Printf(wxT("TCP/IP(%s)"),          wxString(port_tcpip, *wxConvCurrent).c_str());  msg=msg+wxT(" ")+part; }
      if (http)
        { part.Printf(  _("HTTP tunnelling(%s)"), wxString(port_http,  *wxConvCurrent).c_str());  msg=msg+wxT(" ")+part; }
      if (web)
        { part.Printf(wxT("WWW(%s)"),             wxString(port_web,   *wxConvCurrent).c_str());  msg=msg+wxT(" ")+part; }
      mInfo5->SetLabel(msg);
     // show log:
      uns16 buf[8000];  
      if (!cd_Get_server_info(cdp, OP_GI_RECENT_LOG, buf, sizeof(buf)))
      { uns16 * p = buf;
        while (*p)
        { wxString msg;
          while (*p)
            msg=msg+(wxChar)*(p++);
          mLog->Append(msg);
          p++;
        }  
      }      
      else  // old server
        mLog->Append(_("- Not available -"));
      unsigned cnt = mLog->GetCount();
      if (cnt)
        mLog->SetSelection(cnt-1);
      cd_disconnect(cdp);
    }  
    else
      curr_clients=0;
    Layout();  
  }  
}

