// mpsql.cpp
#include "wx/wxprec.h"
#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif
#ifndef WINS
#include "winrepl.h"
#endif
#include "cint.h"
#include "flstr.h"
#include "support.h"
#include "topdecl.h"
#pragma hdrstop
#include "wprez9.h"

#if !OPEN602  // no splash screen in the open version
#define SPLASH602
#endif

#ifdef LINUX  
#include <dlfcn.h>

void * LoadLibraryA(const char * name)
{
  char buffer[400];
  sprintf(buffer, "lib%s.so", name);
  return dlopen(buffer, RTLD_LAZY);
}
// FreeLibrary and GetProcAddress defined as dlclose in wbdefs.h

#endif // LINUX

#include "xmldsgn.h"
#include "xmllnk.c"

CFNC DllImport int WINAPI get_xml_response(cdp_t cdp, int method, char ** options, const char * body, int bodysize, 
                      char ** resp, const char ** content_type, char * realm);
bool close_contents_from_server(xcdp_t xcdp, cdp_t cdp);
void server_state_changed(const t_avail_server * as);

#ifndef WINS
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>
#endif
#include "regstr.h"
#include "netapi.h"
#include "impexpui.h"
#include "debugger.h"
#include "datagrid.h"
#include "delcateg.h"
#include "controlpanel.h"
#include "cptree.h"
#include "cplist.h"
#include "dsparser.h"
#include "transdsgn.h"
#include "mailprofdlg.h"
#include "queryopt.h"
#include "tabledsgn.h"

#define AUI_NB 1
#ifdef FL
// extra plugins
#include "wx/fl/barhintspl.h"    // bevel for bars with "X"s and grooves
#include "wx/fl/rowdragpl.h"     // NC-look with draggable rows
#include "wx/fl/cbcustom.h"      // customization plugin
#include "wx/fl/hintanimpl.h"

// beauty-care
#include "wx/fl/gcupdatesmgr.h"  // smooth d&d
#include "wx/fl/antiflickpl.h"   // double-buffered repaint of decorations
#endif
#include "wx/artprov.h"

#include "wx/clipbrd.h"
#include <wx/config.h>

#include "wx/mdi.h"
//#include "wx/resource.h"  // is not in 2.5.1
#include "wx/gdicmn.h"
#include "wx/log.h"
#include "wx/panel.h"
#include "wx/tooltip.h"
#include <wx/splash.h>
#ifdef HTMLHELP
#include "wx/mimetype.h"
#include "wx/fs_zip.h"
#endif

#include "regstr.cpp"
#include "flstr.cpp"  // including in every library because exporting this class is ineffective because of inlines
#ifdef WINS
#if wxMAJOR_VERSION>2 || wxMINOR_VERSION>6 || wxRELEASE_NUMBER>=1
#include "helpchm3.cpp"
#else
#include "helpchm2.cpp"
#endif
#endif

#define CLIENT_ODBC_9

#ifdef LINUX
#if WBVERS<1000
#define SQLExecDirectA SQLExecDirect
#define SQLDataSourcesA SQLDataSources
#define SQLDriverConnectA SQLDriverConnect
#define SQLGetInfoA SQLGetInfo
#define SQLErrorA SQLError
#endif
#endif

#ifdef _DEBUG
#define new DEBUG_NEW
#endif 

t_global_style global_style = ST_POPUP;
wxAuiNotebookExt * aui_nb = NULL;
bool restore_default_request = false;
static bool cmndl_login = false;  // true when /L specified on the command line (until the login)
static bool connect_by_network = false;  // true when * specified on start of the server name on the comand line, until connected
bool ODBCConfig_available = false;
bool is_w98 = false;
// variables read from the INI file on client start and saved when changed by a dialog:
bool browse_enabled = true;
bool broadcast_enabled = true;
int  notification_modulus = 1024;
#define DSGNTBSZ 346
wxFont * text_editor_font = NULL, * grid_label_font = NULL, * grid_cell_font=NULL, 
       * control_panel_font = NULL, system_gui_font;
int default_font_height;

t_avail_server * WINAPI GetServerList();

wxCPSplitterWindow *ControlPanel()
{
    return(wxGetApp().frame->control_panel);
}

////////////////////////////////////// layout /////////////////////////////////////
struct bar_layout
{ int alignment;
  int row;
  int row_offset; 
  int state;  // e.g. wxCBAR_DOCKED_HORIZONTALLY;
  int w[3];
  int h[3];
  bool is_toolbar;
  void set(int alignmentIn, int rowIn, int row_offsetIn, int stateIn, int w0, int w1, int w2, int h0, int h1, int h2, bool is_toolbarIn)
  { alignment=alignmentIn;  row=rowIn;  row_offset=row_offsetIn;  state=stateIn;
    w[0]=w0;  w[1]=w1;  w[2]=w2;
    h[0]=h0;  h[1]=h1;  h[2]=h2;  
    is_toolbar=is_toolbarIn;
  }
};

extern const wxChar * bar_name[BAR_COUNT];  // indexed by bar_id
extern const char * bar_name_profile[BAR_COUNT];  // indexed by bar_id

#ifdef FL
class layout_descr
{ bar_layout bl[BAR_COUNT];  // indexed by bar_id
  void apply_for_pane(wxFrameLayout * fl, wxWindow * bars[], int pane);
 public:
  void load(const char * name);
  void save(const char * name);
  void apply(wxWindow * bars[]);
  void get_current_state(wxWindow * bars[]);
  void set_default_normal(void);
  void set_default_debug(void);
};
#endif

#include "bmps/_apl.xpm"
#include "bmps/_sql_top.xpm"
#include "bmps/_srvl.xpm"
#include "bmps/_srvlr.xpm"
#include "bmps/_srvlrc.xpm"
#include "bmps/_srvn.xpm"
#include "bmps/_srvnr.xpm"
#include "bmps/_srvnrc.xpm"
#include "bmps/_srvq.xpm"
#include "bmps/_link.xpm"
#include "bmps/_table.xpm"
//#include "bmps/_formgen.xpm"
//#include "bmps/_formsmpl.xpm"
//#include "bmps/_label.xpm"
//#include "bmps/_graph.xpm"
//#include "bmps/_report.xpm"
//#include "bmps/_menu.xpm"
//#include "bmps/_popup.xpm"
#include "bmps/_query.xpm"
#include "bmps/_program.xpm"
#include "bmps/source.xpm"
#include "bmps/_pict.xpm"
#include "bmps/_role.xpm"
#include "bmps/_relat.xpm"
#include "bmps/_conn.xpm"
#include "bmps/_user.xpm"
#include "bmps/_group.xpm"
#include "bmps/_draw.xpm"
#include "bmps/_replsrv.xpm"
#include "bmps/_repltab.xpm"
#include "bmps/_transp.xpm"
//#include "bmps/_badconn.xpm"
#include "bmps/_proc.xpm"
#include "bmps/_trigger.xpm"
#include "bmps/_folder.xpm"
#include "bmps/_seq.xpm"
#include "bmps/_xml.xpm"
#include "bmps/_domain.xpm"
#include "bmps/_full.xpm"
#include "bmps/_client.xpm"
#include "bmps/licences.xpm"
#include "bmps/clients.xpm"
#include "bmps/locks.xpm"
#include "bmps/trace.xpm"
#include "bmps/logs.xpm"
#include "bmps/alog.xpm"
#include "bmps/extinfo.xpm"
#include "bmps/runprop.xpm"
#include "bmps/network.xpm"
#include "bmps/export_users.xpm"
#include "bmps/security.xpm"
#include "bmps/filenc.xpm"
#include "bmps/import_users.xpm"
#include "bmps/srvcert.xpm"
#include "bmps/backup.xpm"
#include "bmps/replay.xpm"
#include "bmps/consist.xpm"
#include "bmps/server_rename.xpm"
#include "bmps/refrpanel.xpm"
#include "bmps/_systable.xpm"
#include "bmps/_folder_sys.xpm"
#include "bmps/_folder_params.xpm"
#include "bmps/_folder_tools.xpm"
#include "bmps/message.xpm"
#include "bmps/fonts.xpm"
#include "bmps/grid_format.xpm"
#include "bmps/table_sql_run.xpm"
#include "bmps/cre_srv.xpm"
#include "bmps/reg_srv.xpm"
#include "bmps/plug.xpm"
#include "bmps/plug_edit.xpm"
#include "bmps/unreg_srv.xpm"
#include "bmps/server_edit_data.xpm"
#include "bmps/new_appl.xpm"
#include "bmps/import_appl.xpm"
#include "bmps/dis_plug.xpm"

//#include "bmps/Vyjmout.xpm"
#include "bmps/Vlozit.xpm"
#include "bmps/Kopirovat.xpm"
//#include "bmps/_cut.xpm"
//#include "bmps/_copy.xpm"
//#include "bmps/_paste.xpm"
#include "bmps/new.xpm"
#include "bmps/delete.xpm"
#include "bmps/modify.xpm"
#include "bmps/export_obj.xpm"
#include "bmps/import_obj.xpm"
#include "bmps/data_export.xpm"
#include "bmps/layout_manager.xpm"
#include "bmps/compact.xpm"
#include "bmps/indexes.xpm"
#include "bmps/data_import.xpm"
#include "bmps/data_find.xpm"
#include "bmps/folder_add.xpm" 
#include "bmps/sql_console.xpm"
#include "bmps/monitor.xpm"
#include "bmps/privil_dat.xpm"
#include "bmps/privil_obj.xpm"
#include "bmps/profils.xpm"
#include "bmps/profil_mail.xpm"
#include "bmps/profil_new.xpm"
#include "bmps/profil_delete.xpm"
#include "bmps/profil_edit.xpm"
#include "bmps/locales.xpm"
#include "bmps/odbc.xpm"
#include "bmps/odbcs.xpm"
#include "bmps/odbc_active.xpm"
#include "bmps/odbc_nonactive.xpm"
#include "bmps/fulltext.xpm"
#include "bmps/fulltext_set.xpm"
#include "bmps/cliconn.xpm"
#include "bmps/profile.xpm"
#include "bmps/xmlform.xpm"
#include "bmps/http.xpm"
#include "bmps/protocols.xpm"
#include "bmps/xmlfill.xpm"
#include "bmps/xmldsg.xpm"
#include "bmps/explorer.xpm"
#include "bmps/params.xpm"
#include "bmps/stylesheet.xpm"
#include "bmps/sts_pref.xpm"

#include "bmps/client16.xpm"
#include "bmps/client32.xpm"

#if WBVERS>=1100
#include "xrc/AboutOpenDlg.cpp"
#else
#include "xrc/AboutDlg.cpp"
#endif
#include "xrc/ConnSpeedDlg.cpp"
#include "xrc/SynChkDlg.cpp"
#include "xrc/PasswordExpiredDlg.cpp"
#include "xrc/LoginDlg.cpp"
#include "xrc/SQLConsoleDlg.cpp"
#include "xrc/NetAccDlg.cpp"
#include "xrc/SelectClientDlg.cpp"
#include "xrc/SearchObjectsDlg.cpp"
#ifdef FL
#include "xrc/LayoutManagerDlg.cpp"
#endif
#include "xrc/ObjectPropertiesDlg.cpp"
#include "xrc/TableStatDlg.cpp"
#include "xrc/MonitorMainDlg.h"
#include "xrc/QueryOptimizationDlg.cpp"
#include "xrc/CreateODBCDataSourceDlg.cpp"
#include "xrc/ColumnsInfoDlg.cpp"

#define VIEW_MENU_STRING   _("&View")
#define HELP_MENU_STRING   _("&Help")

#define MNST_OPEN        _("&Open")
#define MNST_ALT_OPEN    _("Open in a default browser")
#define MNST_RUN         _("&Run")
#define MNST_ALTER       _("&Edit")
#define MNST_ALTER_SEPAR _("Edit in separate window")
#define MNST_MODIFY      _("&Modify")
#define MNST_SRCEDIT     _("Edit Source")
#define MNST_DELETE      _("&Delete")

#define MNST_NEW      _("&New")
#define MNST_IMPORT   _("&Import object")

#define MNST_EXPORT   _("Export Object")
#define MNST_PRIVILS  _("Object Privileges")
#define MNST_DATAPRIV _("Data Privileges")
#define MNST_DATAEXP  _("Export Data")
#define MNST_DATAIMP  _("Import Data")
#define MNST_CREFLD   _("Create Folder")
#define MNST_NEWPROF  _("Create New Mail Profile")

#define OPERATION_COMPLETED _("Operation completed")
#define SERVER_NAME_DUPLICITY _("A server with this name is already registered")
#define ERROR_WRITING_TO_REGISTRY _("Error writing to the registry: insufficient privileges")
#define ERROR_CMDLINE_SERVER 
#define ERROR_CMDLINE_SCHEMA 

#include "xrc/NewLocalServer.cpp"
#include "xrc/RestoringFileDlg.cpp"
#include "xrc/DatabaseDlg.cpp"
#include "mailprofdlg.cpp"
#include "ClientsConnectedDlg.cpp"  // located in the mng branch
#ifdef GETTEXT_STRING  // mark these strings for translation - they are located int the mng directory which is not scanned
  wxTRANSLATE("&Warn network clients and wait (max. 60 seconds)"),
  wxTRANSLATE("&Shutdown the SQL server immediately"),
  wxTRANSLATE("Do you want to:"),
  wxTRANSLATE("Clients connected to the server"),
#endif

void AppendFolderItems(wxMenu *Menu);
// FL Note: If a row is created and no windows are added into it, FL crashes.
// WX Note: wxListCtrl owns memory associated with items, wxListBox does not.
// Note: Action menu reflects the last selection/deselection in either CP tree or CP list. The action is applied to objects
//       selected in the CP list, if any, otherwise to the CP tree selection. This may be a problem.
CFNC DllKernel void WINAPI Set_caching_apl_objects(bool on);

wxMenu * empty_designer_menu = NULL;
wxConfig * global_conf = NULL;

wxMenu * get_empty_designer_menu(void)
{
#ifndef RECREATE_MENUS
  if (empty_designer_menu) return empty_designer_menu;
#endif
  empty_designer_menu = new wxMenu;
  empty_designer_menu->Append(CPM_EMPTY_DESIGNER, _("No designer is active"));
  empty_designer_menu->Enable(CPM_EMPTY_DESIGNER, false);
  return empty_designer_menu;
}

#ifdef LINUX
IMPLEMENT_APP_NO_MAIN(MyApp)

extern "C" void WINAPI start_wx(int argc, char *argv[])
{ wxEntry(argc, argv);
}

#include <errno.h>

#else
IMPLEMENT_APP(MyApp)

#if 0
wxAppConsole *wxCreateApp()                                           
{                                                                     
  wxAppConsole::CheckBuildOptions(WX_BUILD_OPTIONS_SIGNATURE, "your program");                  
  return new MyApp;                                                 
}                                                                       
wxAppInitializer wxTheAppInitializer((wxAppInitializerFunction) wxCreateApp);
MyApp & wxGetApp() 
{ return *(MyApp *)wxTheApp; }                   

// we need HINSTANCE declaration to define WinMain()
#include "wx/msw/wrapwin.h"

#ifndef SW_SHOWNORMAL
#define SW_SHOWNORMAL 1
#endif

// WinMain() is always ANSI, even in Unicode build, under normal Windows but is always Unicode under CE
    typedef char *wxCmdLineArgType;

extern int wxEntry(HINSTANCE hInstance, HINSTANCE hPrevInstance = NULL, char * pCmdLine = NULL, int nCmdShow = SW_SHOWNORMAL);
extern "C" int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, char * lpCmdLine, int nCmdShow)                           \
{
  return wxEntry(hInstance, hPrevInstance, lpCmdLine, nCmdShow);
}
#endif

#if 0  // DLL
HINSTANCE hDvlpLib;

CFNC BOOL WINAPI _CRT_INIT(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved);

CFNC BOOL WINAPI DllEntryPoint(HINSTANCE hInstance, DWORD fdwReason, LPVOID lpvReserved)
{
  switch (fdwReason)
  { case DLL_PROCESS_ATTACH:
      if (!_CRT_INIT(hInstance, fdwReason, lpvReserved)) return FALSE;
      wxEntry(hInstance, 0, NULL, 0, false);
      hDvlpLib=hInstance;
      break;
    case DLL_THREAD_ATTACH:
      if (!_CRT_INIT(hInstance, fdwReason, lpvReserved)) return FALSE;
      break;
    case DLL_PROCESS_DETACH:
      if (!_CRT_INIT(hInstance, fdwReason, lpvReserved)) return FALSE;
      break;
    case DLL_THREAD_DETACH:
      wxGetApp().OnExit();
		  wxApp::CleanUp();
      if (!_CRT_INIT(hInstance, fdwReason, lpvReserved)) return FALSE;
      break;
  }
  return TRUE;                   
}

CFNC DllExport int CALLBACK WEP(int nExitType)
{ return 1; }  /* MSC requires this */
#endif  // DLL

// Support for Unicode on MSW 98/Me
HMODULE LoadUnicowsProc(void)
{
    return(LoadLibraryA("unicows.dll"));
}

extern "C" FARPROC _PfnLoadUnicows = (FARPROC) &LoadUnicowsProc;

#endif

#include <fcntl.h>

#ifdef SPLASH602
#ifdef LINUX
#include "bmps/splash.xpm"  // big and slow, damaged on MSW
#endif

void make_splash(wxWindow * parent, int miliseconds)
 // splash screen:
{ wxMemoryDC splash_dc;
  if (!miliseconds) return;
  wxBitmap splash_bitmap
#ifdef LINUX
    (splash_xpm);
#else
    (wxT("SPLASH"), wxBITMAP_TYPE_BMP_RESOURCE);
#endif
  splash_dc.SelectObject(splash_bitmap);
  splash_dc.SetBackgroundMode(wxTRANSPARENT);
  splash_dc.SetTextForeground(wxColour(0,0,0));
  wxCoord w, h1, h2;
  wxFont font1(11, wxSWISS, wxNORMAL, wxBOLD, false, wxEmptyString, wxFONTENCODING_SYSTEM);
  splash_dc.SetFont(font1);
  wxString name;
  name.Printf(wxT("602SQL %u.%u"), VERS_1, VERS_2);
  splash_dc.GetTextExtent(name, &w, &h1);
  splash_dc.DrawText(name, 316+(132-w)/2, 27);
  wxFont font2(8, wxSWISS, wxNORMAL, wxNORMAL, false, wxEmptyString, wxFONTENCODING_SYSTEM);
  splash_dc.SetFont(font2);
  wxString build = wxString(_("Build"))+wxT(" ")+wxT(VERS_STR);
  splash_dc.GetTextExtent(build, &w, &h2);
  splash_dc.DrawText(build, 316+(132-w)/2, 27+4*h1/3);
  splash_dc.SelectObject(wxNullBitmap);
  wxSplashScreen * splash = new wxSplashScreen(splash_bitmap, 
        parent ? wxSPLASH_CENTRE_ON_PARENT | wxSPLASH_TIMEOUT | wxFRAME_FLOAT_ON_PARENT 
        : wxSPLASH_CENTRE_ON_SCREEN | wxSPLASH_TIMEOUT | wxFRAME_NO_TASKBAR,
        miliseconds, parent, -1, wxDefaultPosition, wxDefaultSize, wxSIMPLE_BORDER|wxSTAY_ON_TOP);
  wxYield();
} 
#endif

static const int IDM_WINDOWTILEHOR  = 4001;
static const int IDM_WINDOWTILEVERT = 4005;
static const int IDM_WINDOWCASCADE = 4002;
static const int IDM_WINDOWICONS = 4003;
static const int IDM_WINDOWNEXT = 4004;
static const int IDM_WINDOWPREV = 4006;

#if 0
const char * __a = "a",* __A = "A", *__z = "z", *__Z = "Z", *__R = "\xd8";
#include <nl_types.h>
#include <langinfo.h>
#include <locale.h>
#include <wchar.h>
#include <wctype.h>
#include <ctype.h>
#include "thlocale.h"
#endif

#ifdef LINUX
#include <gtk/gtk.h>
#endif

void CALLBACK client_notification_callback(cdp_t cdp, int notification_type, void * parameter)
// [cdp] may be NULL except for NOTIF_MESSAGE!
{
  switch (notification_type)
  { case NOTIF_PROGRESS:
    { trecnum num1, num2;
      num1 = ((trecnum*)parameter)[0];
      num2 = ((trecnum*)parameter)[1];
      wxString msg;
      if (num2!=NORECNUM) msg.Printf(wxT("%d / %d"), num1, num2);
      else msg.Printf(wxT("%d"), num1);
      wxGetApp().frame->SetStatusText(msg, 1);
#ifdef LINUX // otherwise the status message is not shown until the client request is completed      
      wxGetApp().frame->Update();
#endif      
      break;
    }
    case NOTIF_SERVERDOWN:
    {
#if 0
      wxMessageBox(_("The SQL Server will terminate in one minute. Please save your work and disconnect immediately!"),
                   wxString((const char*)parameter, *wxConvCurrent), wxICON_EXCLAMATION | wxOK, wxGetApp().frame);
#else
      wxString msg((const char*)parameter, *wxConvCurrent);
      msg = msg + wxT(": ") + _("The SQL Server will terminate in one minute. Please save your work and disconnect immediately!");
      wxLogWarning(msg);
#endif
      break;
    }
    case NOTIF_MESSAGE:
    {
#if 0
      wxMessageBox(wxString((const char*)parameter, *cdp->sys_conv), _("Message from the SQL server"),
                   wxICON_INFORMATION | wxOK, wxGetApp().frame);
#else
      wxString msg = _("Message from the SQL server");
      msg = msg + wxT(": ") + wxString((const char*)parameter, wxConvUTF8);
      wxLogMessage(msg);
#endif
      break;
    }
  }
}

void message_translation_callback(void * message)
{ 
  const wxChar * msg = wxGetTranslation((wchar_t *)message);
  wcscpy((wchar_t *)message, msg);
}

#ifdef LINUX
bool qtengine_bad_version(void)
{ char output[100];
  FILE * pipein_fp;
  int temp_cert_fd;
  wxBusyCursor wait;
 /* Create one way pipe line with call to popen() */
  if ((pipein_fp = popen("rpm -q gtk-qt-engine", "r")) == NULL)
    return false;  // rpm not started
  int len = fread(output, sizeof(char), sizeof(output)-1, pipein_fp);
  output[len]=0;
  //wxLogMessage(wxString(output, *wxConvCurrent));
 // analyse the output:
  bool bad = false;
  if (!memcmp(output, "gtk-qt-engine-", 14))
    if (output[14]=='0' && output[15]=='.')
      bad = output[16]<'6';
  pclose(pipein_fp);
  return bad;
}  
#endif

wxLocale locale2;  

bool MyApp::OnInit(void)
{ int i;
  core_init(0);  // starts the memory management system
  //GtkIconTheme *icon_theme;
  //gchar * path[];
  //gint n_elements;
  //icon_theme = gtk_icon_theme_get_default();
  //gtk_icon_theme_get_search_path  (icon_theme, path, n_elements);
  Get_client_version();
#if 0
  int i1;
  i1=cmp_str("H", "CH", t_specif(31, 1, 0,0));
  i1=cmp_str("H", "CH", t_specif(31, 1, 1,0));
  int i1, i2;
  i1 = __strncasecmp_l(__Z, __R, 2, wbcharset_t(129).locales8());
  i2 = __strcoll_l    (__Z, __R, wbcharset_t(129).locales8());
  i1 = __strncasecmp_l(__A, __R, 2, wbcharset_t(129).locales8());
  i2 = __strcoll_l    (__A, __R, wbcharset_t(129).locales8());
  i1 = __strncasecmp_l(__a, __R, 2, wbcharset_t(129).locales8());
  i2 = __strcoll_l    (__a, __R, wbcharset_t(129).locales8());
  i1 = __strncasecmp_l(__z, __R, 2, wbcharset_t(129).locales8());
  i2 = __strcoll_l    (__z, __R, wbcharset_t(129).locales8());
  i1 = __strncasecmp_l(__a, __Z, 2, wbcharset_t(129).locales8());
  i2 = __strcoll_l    (__a, __Z, wbcharset_t(129).locales8());
#endif

#if wxMAJOR_VERSION>2 || wxMINOR_VERSION==6
  //wxLog::EnableLogging(false);  // 2.6.0 problems: crashes in libc when writing to stderr in wxDebugLog
#endif
#ifdef SPLASH602
  int splash_screen_duration = read_profile_int("Look", "SplashTime", 22) * 100;  // tenths of seconds converted to miliseconds
#endif
  browse_enabled = read_profile_bool("Network", "SearchServers", true);
  broadcast_enabled = read_profile_bool("Network", "Broadcast", true);
  if (!read_profile_int("Network", "Notifications", &notification_modulus)) notification_modulus=1024;
  Enable_server_search(broadcast_enabled);
#ifdef WINS
  OSVERSIONINFO VersionInfo;
  VersionInfo.dwOSVersionInfoSize = sizeof(VersionInfo);
  if (GetVersionEx(&VersionInfo))
    if (VersionInfo.dwMajorVersion<4 || VersionInfo.dwMajorVersion==4 /*&& VersionInfo.dwMinorVersion<=10*/)  // W Me, W98 or older
      is_w98=true;
#ifdef SPLASH602
  make_splash(NULL, splash_screen_duration);
#endif
#endif
  set_language_support(locale2);
  Set_translation_callback(NULL, &message_translation_callback);
#ifdef LINUX
  signal(SIGPIPE, SIG_IGN);  // SIGPIPE comes when server terminates or disconnects the client - must ignore this
  if (!wbcharset_t::prepare_conversions())
    error_box(wxT("\nConversion between UCS-2 and ISO646 is not available!"));
 // check the gtk version:
  if (gtk_major_version<GTK_MAJOR_VERSION || gtk_major_version==GTK_MAJOR_VERSION && gtk_minor_version<GTK_MINOR_VERSION)
  { wxString msg;
    msg.Printf(_("The product is compiled with GTK+ %u.%u and the the available version of GTK+ is %u.%u.\nPlease install the proper version of GTK+."), GTK_MAJOR_VERSION, GTK_MINOR_VERSION, gtk_major_version, gtk_minor_version);
    error_box(msg);
    return false;
  }	
#endif
  prep_regdb_access();
  wxToolTip::Enable(true);
  wxToolTip::SetDelay(1200);
  DF_602SQLObject  = new wxDataFormat(wxT("602SQL Object"));
  DF_602SQLDataRow = new wxDataFormat(wxT("602SQL Data Row"));

  global_conf = new wxConfig(wxT("602sql9_wx"), wxEmptyString, wxEmptyString, wxEmptyString, wxCONFIG_USE_LOCAL_FILE);
  wxConfig::Set(global_conf);
  frame=NULL;
  frame = new MyFrame(NULL);
  system_gui_font = wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT);  // used in create_layout()
   // create fonts:
    { char font_descr[250];
      text_editor_font = new wxFont(12, wxMODERN, wxNORMAL, wxNORMAL, FALSE);
      if (read_profile_string("Fonts", "TextEditor", font_descr, sizeof(font_descr)) && *font_descr)
        text_editor_font->SetNativeFontInfo(wxString(font_descr, *wxConvCurrent).c_str());  // uses the default font if the description is invalid
      grid_label_font = new wxFont(8, wxSWISS, wxNORMAL, wxNORMAL/*wxBOLD*/, FALSE);
      if (read_profile_string("Fonts", "GridLabel", font_descr, sizeof(font_descr)) && *font_descr)
        grid_label_font->SetNativeFontInfo(wxString(font_descr, *wxConvCurrent).c_str());  // uses the default font if the description is invalid
      grid_cell_font = new wxFont();
      *grid_cell_font = wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT);
      if (read_profile_string("Fonts", "GridCell", font_descr, sizeof(font_descr)) && *font_descr)
        grid_cell_font->SetNativeFontInfo(wxString(font_descr, *wxConvCurrent).c_str());  // uses the default font if the description is invalid
     // control panel font: 
     // create only if specified - no more, the system default font is too big
      control_panel_font = new wxFont(8, wxSWISS, wxNORMAL, wxNORMAL, FALSE);
      if (read_profile_string("Fonts", "ControlPanel", font_descr, sizeof(font_descr)) && *font_descr)
        control_panel_font->SetNativeFontInfo(wxString(font_descr, *wxConvCurrent).c_str());  // uses the default font if the description is invalid
    }
  frame->control_panel=NULL;
  global_style = (t_global_style)read_profile_int("Look", "MDI", 0);  // used by create_layout()
 ///////////////////////////////////// creating the main manu ///////////////////////////////////
 // It is better to create menu before calling create_layout(), otherwize WX 2.6.0 has problems with layout
    frame->connection_menu = new wxMenu;
    AppendXpmItem(frame->connection_menu, CPM_CONNECT, _("&Connect to Server"),      plug_xpm);
    AppendXpmItem(frame->connection_menu, CPM_DISCONN, _("&Disconnect from Server"), dis_plug_xpm);
    AppendXpmItem(frame->connection_menu, CPM_RELOGIN, _("Relogin"));
    frame->connection_menu->AppendSeparator();
    AppendXpmItem(frame->connection_menu, CPM_DISCONN_ALL, _("Disconnect from &All Servers"));
    frame->connection_menu->AppendSeparator();
    AppendXpmItem(frame->connection_menu, CPM_EXIT,    _("E&xit"));

    frame->edit_menu = new wxMenu;
    //AppendXpmItem(frame->edit_menu, CPM_CUT,   _("Cu&t"),   cut_xpm);
    //AppendXpmItem(frame->edit_menu, CPM_COPY,  _("&Copy"),  Kopirovat_xpm);
    //AppendXpmItem(frame->edit_menu, CPM_PASTE, _("&Paste"), Vlozit_xpm);
    //AppendXpmItem(frame->edit_menu, CPM_REFRPANEL, _("&Refresh"), refrpanel_xpm);

    //AppendXpmItem(frame->edit_menu, CPM_ACTION, _("&Action")); // will be replaced immediately

    wxMenu *view_menu = new wxMenu;
    wxMenu *view_submenu = new wxMenu;
	 // add bars to the View menu:
    for (i=BAR_ID_FIRST_TOOLBAR;  i<BAR_COUNT;  i++)
      view_submenu->AppendCheckItem(CPM_SHOW_HIDE+i, wxGetTranslation(bar_name[i]));
    for (i=0;  i<BAR_ID_FIRST_TOOLBAR;  i++)
      view_menu->AppendCheckItem(CPM_SHOW_HIDE+i, wxGetTranslation(bar_name[i]));
    view_menu->Append(CPM_VIEW_TOOLBARS, _("Toolbars"), view_submenu);
#ifdef FL
    view_menu->AppendSeparator();
    AppendXpmItem(view_menu, CPM_LAYOUT_MANAGER, _("Frame Layout Manager..."), layout_manager_xpm);
#endif
   // create private events:
    disconnect_event = wxNewEventType();
    record_update = wxNewEventType();
   // create main menu:
    wxMenu *debug_menu = new wxMenu;
    AppendXpmItem(debug_menu, DBG_RUN,       _("&Continue\tShift+F5"), debugRun_xpm);
    debug_menu->AppendSeparator();
    AppendXpmItem(debug_menu, DBG_STEP_INTO, _("&Trace Into\tF11"), traceInto_xpm);
    AppendXpmItem(debug_menu, DBG_STEP_OVER, _("&Step Over\tF8"), stepOver_xpm);  // originally it was F10 but the editor in a popup window receives only the every other key - collision with invoking the menu
    AppendXpmItem(debug_menu, DBG_GOTO,      _("&Go to Cursor"), runToCursor_xpm);
    AppendXpmItem(debug_menu, DBG_RETURN,    _("&Return from Current Routine"), runUntilReturn_xpm);
    AppendXpmItem(debug_menu, DBG_EVAL,      _("&Evaluate"), evaluate_xpm);
    debug_menu->AppendSeparator();
    AppendXpmItem(debug_menu, DBG_BREAK,     _("&Break"), break_xpm);
    AppendXpmItem(debug_menu, DBG_EXIT,      _("Continue &Without Debugging"), runu_xpm);
    AppendXpmItem(debug_menu, DBG_KILL,      _("&Kill the Debugee"), debugKill_xpm);

    wxMenu *help_menu = new wxMenu;
    AppendXpmItem(help_menu, CPM_CROSSHELP, _("&Help Contents"), help_xpm);
    help_menu->AppendSeparator();
    AppendXpmItem(help_menu, CPM_ABOUT, _("&About..."));

//    wxMenu *action_menu = new wxMenu;

    wxMenuBar *menu_bar = new wxMenuBar;
    menu_bar->Append(frame->connection_menu, _("&Connection"));
    menu_bar->Append(frame->edit_menu, ACTION_MENU_STRING);
    menu_bar->Append(view_menu, VIEW_MENU_STRING);
    menu_bar->Append(get_empty_designer_menu(), DESIGNER_MENU_STRING);
    menu_bar->Append(debug_menu, _("D&ebug"));
    menu_bar->Append(help_menu,  HELP_MENU_STRING);
    frame->SetMenuBar(menu_bar);
    frame->CreateStatusBar(3);

  frame->create_layout();  // should be after creating Menu/Status bars on Windows, otherwise problems with drawing in FL
  frame->SetBackgroundColour( wxColour(192,192,192) );

 // add control panel related view menu items:
  if (frame->control_panel)
  { AppendFolderItems(view_menu);
    view_menu->AppendSeparator();
    AppendXpmItem(view_menu, CPM_REFRPANEL, _("Refresh control panel"), refrpanel_xpm);
    view_menu->AppendSeparator();
    //view_menu->Append(CPM_UP, _("Go One Level &Up"));  -- nobody would use this
    view_menu->AppendRadioItem(CPM_POPUP, _("Popup Style"));
    view_menu->AppendRadioItem(CPM_AUINB, _("Notebook Style"));
    view_menu->AppendRadioItem(CPM_MDI,   _("MDI Style"));
  }
 // calc default font height:
  { wxClientDC dc(frame);
    dc.SetFont(system_gui_font);
    wxCoord w, h, descent, externalLeading;
    dc.GetTextExtent(wxT("QpYg"), &w, &h, &descent, &externalLeading);
#ifdef WINS  
    default_font_height = (h+descent+externalLeading) * 5 / 4;
#else  
    default_font_height = (h+descent+externalLeading) + 3;
#endif  
  }
#ifdef LINUX
  frame->Show(true);  // on GTK must be before creating the splash, otherwise it covers the splash screen
 // CP fonts (on Linux must be set AFTER Show(), otherwise not effective): 
  if (control_panel_font)
  { frame->control_panel->tree_view->SetFont(*control_panel_font);
    frame->control_panel->objlist->SetFont(*control_panel_font);
  }
#ifdef SPLASH602
  make_splash(frame, splash_screen_duration);
#endif
#endif

#ifdef WINS  // on Linux the execl function does not work when the SipThreadProc is running
  NetworkStart(TCPIP);
#endif
  //TcpIpQueryInit();
  connected_count = 0;
  if (browse_enabled) BrowseStart(0);
  refresh_server_list();  // must be after NetworkStart and after creating image lists in create_layout
  CObjectList::SetServerList(GetServerList);
 // look for ODBCConfig:
#ifdef LINUX
  struct stat st;
  ODBCConfig_available = !stat("/usr/bin/ODBCConfig", &st) || !stat("/usr/local/bin/ODBCConfig", &st);  // checking fixed loations
#else
  ODBCConfig_available = true;  
#endif
 ///////////////////////// main icons ///////////////////////////////
  wxIconBundle main_icons;// = new wxIconBundle();
#ifdef LINUX
  main_icons.AddIcon(wxIcon(client16_xpm));
#endif
  main_icons.AddIcon(wxIcon(client32_xpm));
  frame->SetIcons(main_icons);

 // make the initial selection the control panel:
    wxTreeCtrl * tree = frame->control_panel->tree_view;  wxTreeItemIdValue coo;
    tree->SelectItem(tree->GetFirstChild(tree->GetRootItem(), coo));

#ifdef WINS  // on LINUX have been made before
	frame->Show(true);
#endif

  // divide the control panel (should be after frame->Show which slightly changes the size of control panel):
  { int width, height, division;
    frame->control_panel->GetClientSize(&width, &height);
    int split_mode = frame->control_panel->GetSplitMode();
    if (!read_profile_int("Look", split_mode==wxSPLIT_VERTICAL ? "VertCPSplit" : "HorizCPSplit", &division)) division=300;
    if (split_mode==wxSPLIT_VERTICAL)
      frame->control_panel->SetSashPosition(width*division/1000);
    else
      frame->control_panel->SetSashPosition(height*division/1000);
  }

 // MDI etc:
#ifdef WINS   // remove some automatically inserted items from the Window menu
  HMENU main_menu = GetMenu((HWND)frame->GetHandle());
  DeleteMenu(main_menu, IDM_WINDOWTILEHOR,  MF_BYCOMMAND);
  DeleteMenu(main_menu, IDM_WINDOWTILEVERT, MF_BYCOMMAND);
  DeleteMenu(main_menu, IDM_WINDOWICONS,    MF_BYCOMMAND);
#endif

    frame->mpClientWnd->Refresh();
    SetTopWindow(frame);

#ifdef HTMLHELP
   // must specifically initialize MimeManager - without GNOME which causes problems on SuSe 9.1
    // since wx 2.8.0 seems to work file without this, Initialize is very slow and GetFileTypeFrom... is never used
	// wxTheMimeTypesManager->Initialize(wxMAILCAP_STANDARD | wxMAILCAP_NETSCAPE | wxMAILCAP_KDE /*| wxMAILCAP_GNOME */);
	wxFileSystem::AddHandler(new wxZipFSHandler);
#endif
 // locate the help file and start help (locale must be initialized before):
  { char pathname[MAX_PATH];
    const char * helpsuffix;
    get_path_prefix(pathname);
#ifdef WINS
    helpsuffix=".chm";
#else  // form a file name like: /usr/share/doc/602sql95/602sql95.zip
    strcat(pathname, "/share/doc/" WB_LINUX_SUBDIR_NAME);
    helpsuffix=".zip";
#endif
    strcat(pathname, PATH_SEPARATOR_STR SERVER_FILE_NAME);   // same as help file name without suffix
    int pos=(int)strlen(pathname);
    bool help_found;
   // trying long name for the help: 
    strcpy(pathname+pos, locale2.GetCanonicalName().mb_str(*wxConvCurrent));
    strcat(pathname, helpsuffix);
    help_found = frame->help_ctrl.AddBook(wxString(pathname, *wxConvCurrent), false);
   // trying short name for the help: 
    if (!help_found && pathname[pos+2]=='_') 
    { pathname[pos+2]=0;  strcat(pathname, helpsuffix);
      help_found = frame->help_ctrl.AddBook(wxString(pathname, *wxConvCurrent), false);  // do not show the progress bar: it is empty!
    }  
   // trying cs instead of sk: 
    if (!help_found && !memcmp(pathname+pos, "sk", 2)) 
    { strcpy(pathname+pos, "cs");  strcat(pathname, helpsuffix);
      help_found = frame->help_ctrl.AddBook(wxString(pathname, *wxConvCurrent), false);  // do not show the progress bar: it is empty!
    }
   // trying en as the last resort: 
    if (!help_found) 
    { strcpy(pathname+pos, "en");  strcat(pathname, helpsuffix);
      help_found = frame->help_ctrl.AddBook(wxString(pathname, *wxConvCurrent), false);  // do not show the progress bar: it is empty!
    }
   // report the failure to the client log:
    if (!help_found) 
    { wxString msg;
      msg=wxT("Help file not found:") + wxString(wxT(" ")) + wxString(pathname, *wxConvCurrent);
      log_client_error(msg.mb_str(*wxConvCurrent));
    }
  }
  
  //wxLog::SetTraceMask(wxTraceMessages);
  Set_caching_apl_objects(true);
 // load formats:
  { char format_descr[100];  int rl;
    if (read_profile_string("Format", "DecimalSeparator", format_descr, sizeof(format_descr)))
      format_decsep=wxString(format_descr, *wxConvCurrent);
    if (read_profile_string("Format", "Date", format_descr, sizeof(format_descr)))
      format_date=wxString(format_descr, *wxConvCurrent);
    if (read_profile_string("Format", "Time", format_descr, sizeof(format_descr)))
      format_time=wxString(format_descr, *wxConvCurrent);
    if (read_profile_string("Format", "Timestamp", format_descr, sizeof(format_descr)))
      format_timestamp=wxString(format_descr, *wxConvCurrent);
    if (read_profile_int("Format", "Real", &rl))
      format_real=rl;
  }
  Set_notification_callback(&client_notification_callback);
 // analyse the command line:
  const wxChar * cmndl_server = NULL, * cmndl_appl = NULL;  
  for (i=1;  i<argc;  i++)
  { if (argv[i][0]=='-' || argv[i][0]=='/')
      if (argv[i][1]=='s' || argv[i][1]=='n')
      { if (argv[i][2]) cmndl_server=argv[i]+2;
        else if (i+1<argc) cmndl_server=argv[++i];
      }
      else if (argv[i][1]=='a')
      { if (argv[i][2]) cmndl_appl=argv[i]+2;
        else if (i+1<argc) cmndl_appl=argv[++i];
      }
      else if (argv[i][1]=='L')
        cmndl_login = true;
  }
  // IF prikazova radka, precist server a schema z prikazove radky
  if (cmndl_server)
  { if (*cmndl_server == '*')
    { connect_by_network = true;
      cmndl_server++;
    }
      wxString server_name = cmndl_server;
      item_info info(TOPCAT_SERVERS, server_name.mb_str(*wxConvCurrent));
      info.cdp=(cdp_t)1;  // for FindItem cdp must not be NULL
      ControlPanelTree *Tree = frame->control_panel->tree_view;
      wxTreeItemId Item = Tree->FindItem(Tree->GetRootItem(), info);
      if (!Item.IsOk())
      { error_boxp(_("The server \"%s\" specified in command line was not found"), frame, TowxChar(info.server_name, wxConvCurrent));
        cmndl_login = false;
      }
      else
      {
          Tree->SelectItem(Item);
          Tree->Expand(Item);                                        
          if (Tree->IsExpanded(Item) && cmndl_appl)
          {
              //strcpy(info.schema_name, wxString(cmndl_appl).mb_str(*wxConvCurrent));
              Item = Tree->GetItem(Item, cmndl_appl, IND_APPL);
              if (!Item.IsOk())
                  error_boxp(_("The schema \"%s\" specified in the command line was not found"), frame, cmndl_appl);
              else
              {
                  Tree->SelectItem(Item);
                  Tree->Expand(Item);
              }
          }
          Tree->SetFocus();
      }
  }
  else // server not specified, expand the list and select the 1st server
  { ControlPanelTree *Tree = frame->control_panel->tree_view;
    wxTreeItemIdValue cookie;
    wxTreeItemId Item = Tree->GetFirstChild(Tree->GetRootItem(), cookie);
    Tree->SelectItem(Item);  Tree->Expand(Item);
    Item = Tree->GetFirstChild(Item, cookie);  // the 1st server
    if (Item.IsOk()) Tree->SelectItem(Item);
    else  // no servers registered
      info_box(_("There are no SQL Servers registered.\nYou may create a local SQL Server or register a remote server.\n\nClick the right mouse button to display the pop-up menu on the control panel."));
    tree->SetFocus();
  }
#ifdef LINUX
 // check the GTK theme: warn if Qt-engine is used
  const char * home = getenv("GTK2_RC_FILES");
  if (home && strstr(home, "/Qt/gtk-2.0/gtkrc"))
    if (qtengine_bad_version()) 
      error_box(_("WARNING: The gtk-qt-engine theme is selected for GTK applications.\nThis theme engine is very unstable.\nPlease install GNOME and select another theme\nor uninstall the gtk-qt-engine package."));
#endif
  return TRUE;
}

// global variables:
wxEventType disconnect_event;
wxEventType record_update;
/***** Implementation for class MyFrame *****/
BEGIN_EVENT_TABLE( MyFrame, wxMDIParentFrame )
    EVT_CLOSE(MyFrame::OnClose) 
    EVT_MENU_OPEN(MyFrame::OnMenuOpen) 
    EVT_MENU(-1, MyFrame::OnCPCommand)
    EVT_ACTIVATE(MyFrame::OnActivate)
    EVT_SET_FOCUS(MyFrame::OnFocus)
    EVT_KILL_FOCUS(MyFrame::OnKillFocus)
    EVT_CHILD_FOCUS(MyFrame::OnChildFocus)
    EVT_IDLE(MyFrame::OnIdle)
    EVT_SIZE(MyFrame::OnSize)
   // AUI_NB messages:
    EVT_AUINOTEBOOK_PAGE_CHANGED(-1, MyFrame::OnAuiNotebookPageChanged)
    EVT_AUINOTEBOOK_PAGE_CLOSE(-1, MyFrame::OnAuiNotebookPageClose)
END_EVENT_TABLE()

void MyFrame::OnSize(wxSizeEvent & event)
// Prevents bad sizing of the client window in wxMDIParentFrame::OnSize
{
}

void MyFrame::OnActivate(wxActivateEvent & event)
{ //if (event.GetActive()) SetFocus();    // this causes lost on blinking on Windows when a modal dialog is being closed (e.g. optimization info over query editor)
  event.Skip();
}

void MyFrame::RemoveChild(wxWindowBase *child)
{ m_container.HandleOnWindowDestroy(child);
  wxWindow::RemoveChild(child);
}

void MyFrame::SetFocus()
{ if ( !m_container.DoSetFocus() )
    wxWindow::SetFocus();
}

void MyFrame::OnKillFocus(wxFocusEvent& event) 
{ 
  event.Skip();  // just for debugging
}


void MyFrame::OnChildFocus(wxChildFocusEvent& event)
{ m_container.SetLastFocus(event.GetWindow()); }

void MyFrame::OnFocus(wxFocusEvent& event) 
{  m_container.HandleOnFocus(event); }

void MyFrame::OnIdle(wxIdleEvent & event)
{ 
 // set the delayed focus, if any:
  if (delayed_focus)
  { 
#ifdef _DEBUG
    wxLogDebug(wxT("Before DelaeyedSetfocus %08X"), delayed_focus->GetHandle());
#endif	
    delayed_focus->SetFocus();
#ifdef _DEBUG
    wxLogDebug(wxT("After DelaeyedSetfocus"));
#endif	
    delayed_focus=NULL;
  }
 // process notifications from connected servers:
  t_avail_server * as;
  for (as = available_servers;  as;  as=as->next)
    if (as->cdp)
      process_notifications(as->cdp);
}

void MyFrame::SetFocusDelayed(wxWindow * w)
{ delayed_focus = w; }

void MyFrame::OnAuiNotebookPageChanged(wxAuiNotebookEvent & event)
{
  if (!aui_nb) return;  // fuse
  wxWindow * page = aui_nb->GetPage(event.GetSelection());
  if (!page) return;  // fuse
  //competing_frame * cf = wxDynamicCast(wxDynamicCast(page->FindWindow(COMP_FRAME_ID), table_designer), competing_frame);
  //competing_frame * cf = (competing_frame*)(table_designer*)page->FindWindow(COMP_FRAME_ID);
  competing_frame * cf = dynamic_cast<competing_frame*>(page->FindWindow(COMP_FRAME_ID));
  if (!cf) return;  // fuse
  cf->Activated();
}

void MyFrame::OnAuiNotebookPageClose(wxAuiNotebookEvent & event)
{
#if 0  // must not close here because the NB closes the page after sending the message
  if (!aui_nb) return;  // fuse
  wxWindow * page = aui_nb->GetPage(event.GetSelection());
  if (!page) return;  // fuse
  competing_frame * cf = (competing_frame*)(table_designer*)page->FindWindow(COMP_FRAME_ID);
  if (!cf) return;  // fuse
  wxCommandEvent cmd; 
  cmd.SetId(TD_CPM_EXIT_FRAME);  // can veto
  cf->OnCommand(cmd);
#endif
}

// Adding 2 missing features to wxAuiNotebook:
//  Making the new tab visible when a page is added
//  Making any tab visible when a page is removed

bool wxAuiNotebookExt::has_right_button_visible(void)
{ wxAuiTabContainerButton * hit;
  wxSize sz = GetClientSize();
  wxAuiTabCtrl * tab_ctrl = GetActiveTabCtrl();
  if (tab_ctrl)
    if (tab_ctrl->ButtonHitTest(sz.GetX()-8, 10, &hit))
      if (hit->id == wxAUI_BUTTON_RIGHT)
        if (!(hit->cur_state & wxAUI_BUTTON_STATE_HIDDEN))
          return true;
  return false;
}

void wxAuiNotebookExt::increase_offset(void)
{ wxAuiTabCtrl * tab_ctrl = GetActiveTabCtrl();
  if (tab_ctrl)
  { tab_ctrl->SetTabOffset(tab_ctrl->GetTabOffset()+1);
    tab_ctrl->Refresh();
    tab_ctrl->Update();
  }
}

void wxAuiNotebookExt::normalize_offset(void)
{ wxAuiTabCtrl * tab_ctrl = GetActiveTabCtrl();
  if (tab_ctrl)
  { int offset = (int)tab_ctrl->GetTabOffset();
    if (offset>0 && offset >= GetPageCount())
    { tab_ctrl->SetTabOffset(offset-1);
      tab_ctrl->Refresh();
      tab_ctrl->Update();
    }  
  }
}
    
///////////////////////////////////////////////////////////////////////////////
wxControlContainerJD::wxControlContainerJD(wxWindow *winParent)
{ m_winParent = winParent;
  m_winLastFocused = NULL;
}

//wxWindow * focus_command_receiver = NULL;

void wxControlContainerJD::SetLastFocus(wxWindow *win)
{ wxWindow * p1;
  //wxLogTrace(_T("focus"), _T("MyFrame::SetLastFocus(0x%08lx)."), (unsigned long)win->GetHandle());
  p1 = win->GetParent();
#if WBVERS<1000
  if (win == wxGetApp().frame->MainToolBar)  // J.D. added
    return;
  if (p1== wxGetApp().frame->MainToolBar)  // J.D. added
    return;
#endif
  if (win == any_designer::designer_toolbar)  // J.D. added
    return;
  if (p1 == any_designer::designer_toolbar)  // J.D. added
    return;
    // the panel itself should never get the focus at all but if it does happen
    // temporarily (as it seems to do under wxGTK), at the very least don't
    // forget our previous m_winLastFocused
    if ( win != m_winParent )
    {
        // if we're setting the focus
        if ( win )
        {
            // find the last _immediate_ child which got focus
            wxWindow *winParent = win;
            while ( winParent != m_winParent )
            {
                win = winParent;
                winParent = win->GetParent();  
                if (!winParent) wxLogTrace(_T("focus"), _T("MyFrame::SetLastFocus pathology"));

                // Yes, this can happen, though in a totally pathological case.
                // like when detaching a menubar from a frame with a child
                // which has pushed itself as an event handler for the menubar.
                // (under wxGTK)
#if wxMAJOR_VERSION==2 && wxMINOR_VERSION==5
                return;   // to se stava, kdyz se na 2.5.3 otevira navrhar tabulek v plovoucim okne
#else  // Je to spatne, MyFrame si pak nemapamtuje fucusovaneho childa
                if (!winParent) return;
#endif				
                //wxASSERT_MSG( winParent,
                //              _T("Setting last focus for a window that is not our child?") );
            }
        }
       // Plovouci okno designeru ma kupodivu za parenta take hlavni okno, ale nesmi se zde zaznamenat.
       // Jinak by blokovalo hlavni okno, protoze kluknuti na hlavni ono by predalo focus do plovouciho okna:
        if (win->IsTopLevel()) return; 
        //wxLogTrace(_T("focus"), _T("MyFrame::SetLastFocus storing (0x%08lx)."), (unsigned long)win->GetHandle());
        m_winLastFocused = win;

    }
	else wxLogTrace(_T("focus"), _T("MyFrame::SetLastFocus same"));

    // propagate the last focus upwards so that our parent can set focus back
    // to us if it loses it now and regains later
    wxWindow *parent = m_winParent->GetParent();
    if ( parent )
    {
        wxChildFocusEvent eventFocus(m_winParent);
        parent->GetEventHandler()->ProcessEvent(eventFocus);
    }
}

void wxControlContainerJD::HandleOnWindowDestroy(wxWindowBase *child)
{   if ( child == m_winLastFocused )
        m_winLastFocused = NULL;
    //if (child == focus_command_receiver)
    //  focus_command_receiver=NULL;
}

// ----------------------------------------------------------------------------
// focus handling
// ----------------------------------------------------------------------------

bool wxControlContainerJD::DoSetFocus()
// Called when MyFrame is focused
{
    // when the panel gets the focus we move the focus to either the last
    // window that had the focus or the first one that can get it unless the
    // focus had been already set to some other child
    wxLogTrace(_T("focus"), _T("MyFrame::DoSetFocus()" ));

    wxWindow *win = wxWindow::FindFocus();
    while ( win )
    {   
#if WBVERS<1000
        if (win == wxGetApp().frame->MainToolBar)  // J.D. added
          break;
#endif
        if (win == any_designer::designer_toolbar)  // J.D. added
          break;
#if 0  // this prevents returning focus to a floating designer when a menu command in the main menu is issued
        if ( win == m_winParent )
        {
            // our child already has focus, don't take it away from it
            return TRUE;
        }
#endif
        if ( win->IsTopLevel() )
        {
            // don't look beyond the first top level parent - useless and
            // unnecessary
            break;
        }

        win = win->GetParent();
    }

    return SetFocusToChild();
}

void wxControlContainerJD::HandleOnFocus(wxFocusEvent& event)
{   DoSetFocus();
    event.Skip();
}

bool wxSetFocusToChildJD(wxWindow *win, wxWindow **childLastFocused)
{
    if ( *childLastFocused )
    {
        // It might happen that the window got reparented
        if ( (*childLastFocused)->GetParent() == win )
        {
            //wxLogTrace(_T("focus"),_T("SetFocusToChildJD() => focusing last (0x%08lx)."),(unsigned long)(*childLastFocused)->GetHandle());
            // not SetFocusFromKbd(): we're restoring focus back to the old
            // window and not setting it as the result of a kbd action
            (*childLastFocused)->SetFocus();
            return TRUE;
        }
        else
        {
            // it doesn't count as such any more
            *childLastFocused = (wxWindow *)NULL;
        }
    }

    // set the focus to the first child who wants it
    wxWindowList::Node *node = win->GetChildren().GetFirst();
    while ( node )
    {
        wxWindow *child = node->GetData();

        if ( child->AcceptsFocusFromKeyboard() && !child->IsTopLevel() )
        {
#ifdef _DEBUG
            //wxLogTrace(_T("focus"),_T("SetFocusToChildJD() => first child (0x%08lx)."),(size_t)child->GetHandle());
#endif
            *childLastFocused = child;  // should be redundant, but it is not
            child->SetFocusFromKbd();
            return TRUE;
        }

        node = node->GetNext();
    }

    return FALSE;
}

bool wxControlContainerJD::SetFocusToChild()
{ 
  wxLogTrace(_T("focus"),  _T("JD::SetFocusToChild() => last focused is (0x%08lx)."),
                       m_winLastFocused ? (size_t)m_winLastFocused->GetHandle() : 0);
  return wxSetFocusToChild(m_winParent, &m_winLastFocused); // ???
}


///////////////////////////////////////////////////////////////////////////////
void MyFrame::OnAbout( wxCommandEvent& event )
{ 
#if WBVERS>=1100
  AboutOpenDlg dlg(this);
#else
  AboutDlg dlg(this);
#endif  
  dlg.ShowModal();
  dlg.Destroy();
}

void MyFrame::OnMenuOpen(wxMenuEvent & event)
{ wxMenuBar * menu = GetMenuBar();
#ifdef _DEBUG
  wxLogDebug(wxT("OnMenuOpen %08X %s"), event.GetMenuId(), event.IsPopup() ? wxT("true") : wxT("false"));
#endif
 // update information about hidden/shown bars:
  for (int i=0;  i<BAR_COUNT;  i++)
#ifdef FL
    if (bars[i]) 
    { cbBarInfo * bar = mpLayout->FindBarByWindow(bars[i]);
      if (bar) menu->Check(CPM_SHOW_HIDE+i, bar->mState != wxCBAR_HIDDEN);
    }
#else
    menu->Check(CPM_SHOW_HIDE+i, bars[i] && m_mgr.GetPane(bar_name[i]).IsShown());
#endif
  menu->Enable(CPM_SHOW_HIDE+BAR_ID_CALLS,       the_server_debugger!=NULL);
  menu->Enable(CPM_SHOW_HIDE+BAR_ID_WATCH,       the_server_debugger!=NULL);
  menu->Enable(CPM_SHOW_HIDE+BAR_ID_DBG_TOOLBAR, the_server_debugger!=NULL);
  menu->Check(CPM_POPUP, global_style==ST_POPUP);  
  menu->Check(CPM_AUINB, global_style==ST_AUINB);  
  menu->Check(CPM_MDI,   global_style==ST_MDI);  
 // look for open managed windows:
  bool any_managed = false;
  for (competing_frame * fr = competing_frame_list;  fr;  fr=fr->next)
    if (fr->cf_style!=ST_CHILD) 
      any_managed=true;
  bool can_change_style = !any_managed /*&& GetActiveChild()==NULL && aui_nb->GetPageCount()==0*/;
  menu->Enable(CPM_POPUP, can_change_style);
  menu->Enable(CPM_AUINB, can_change_style);
  menu->Enable(CPM_MDI,   can_change_style);
#ifdef WINS   // disable commands from Window menu in the non-mdi mode
  HMENU main_menu = GetMenu((HWND)GetHandle());
  EnableMenuItem(main_menu, IDM_WINDOWCASCADE, global_style==ST_MDI ? MF_BYCOMMAND|MF_ENABLED:MF_BYCOMMAND|MF_DISABLED|MF_GRAYED);
  EnableMenuItem(main_menu, IDM_WINDOWNEXT,    global_style==ST_MDI ? MF_BYCOMMAND|MF_ENABLED:MF_BYCOMMAND|MF_DISABLED|MF_GRAYED);
  EnableMenuItem(main_menu, IDM_WINDOWPREV,    global_style==ST_MDI ? MF_BYCOMMAND|MF_ENABLED:MF_BYCOMMAND|MF_DISABLED|MF_GRAYED);
#endif
 // debugger menu items:
  bool is_breaked = the_server_debugger!=NULL && the_server_debugger->current_state==DBGST_BREAKED;
  menu->Enable(DBG_RUN,       is_breaked);
  menu->Enable(DBG_STEP_INTO, is_breaked);
  menu->Enable(DBG_STEP_OVER, is_breaked);
  menu->Enable(DBG_GOTO,      is_breaked);
  menu->Enable(DBG_RETURN,    is_breaked);
  menu->Enable(DBG_EVAL,      is_breaked);
  menu->Enable(DBG_BREAK,     the_server_debugger!=NULL && the_server_debugger->current_state==DBGST_EXECUTING);
  menu->Enable(DBG_EXIT,      the_server_debugger!=NULL);
  menu->Enable(DBG_KILL,      is_breaked);
  bool Enb = true;
  wxWindow * focwin = wxWindow::FindFocus();
  if (focwin) // may be NULL!
  { int Id = focwin->GetId();
    if (Id == TREE_ID || Id == LIST_ID)
    { if (wxTheClipboard->Open())
      { Enb = wxTheClipboard->IsSupported(*DF_602SQLObject);
        wxTheClipboard->Close();
      }
    }
  }
  //edit_menu->Enable(CPM_PASTE, Enb);
 // designer menu items updated by the active designer:
  if (competing_frame::active_tool_owner!=NULL)
    competing_frame::active_tool_owner->update_designer_menu();
}

bool find_server_image(t_avail_server *srv, bool packet_received)
// Returns true iff image has been changed.
{ int new_server_image;
  if (srv->odbc)
    new_server_image = srv->conn ? IND_SRVODBCCON : IND_SRVODBC;
  else 
  { if (srv->cdp)
      if (cd_is_dead_connection(srv->cdp) && !packet_received)
        try_disconnect(srv);
    if (srv->local)
      new_server_image = srv->cdp ? IND_SRVLOCCON : // server connected
        srv_Get_state_local(srv->locid_server_name)>=1 ? IND_SRVLOCRUN : IND_SRVLOCIDL;  // not running
    else
      new_server_image = srv->cdp ? IND_SRVREMCON : // server connected
        packet_received ? IND_SRVREMRUN : IND_SRVREMIDL;  // IND_SRVREMIDL will be changed when info from the server arrives
  }
  if (new_server_image!=srv->server_image)
  { srv->server_image=new_server_image;
    return true;
  }
  return false;
}

enum t_add_server_type { add_server_local, add_server_registered, add_server_running, add_server_odbc };

int add_server(const char * server_name, t_add_server_type server_type, bool private_server, uns32 ip=0, uns16 port=0, t_avail_server ** pas = NULL)
// [server_type]: 0=local, 1=remote registerd, 2=remote running, 3=ODBC DSN
// Returns 2 iff added, 1 if image changed, 0 if no change.
// ip and port specified when server_type==add_server_running
// Adds a new server to the list
{ t_avail_server * as;  int chng = 0;
  if (pas) *pas = NULL;  // used only on memory error
 // search for the server in the list:
  as = find_server_connection_info(server_name);
 // try to find the server based on the IP/port (will not work when the server is registered with hostNAME instead of IP address):
  if (!as && server_type==add_server_running)
  { for (as = available_servers;  as;  as=as->next)
      if (!as->local && as->ip == ip && as->port == port)
        break;
  }
 // create a new server if not found:
  if (!as)
  { as = new t_avail_server;
    if (!as) return 0;
    if (server_type==add_server_running || server_type==add_server_registered)
      { as->ip=ip;  as->port=port; }
    if (server_type==add_server_running)
    { as->notreg=true;  
      sprintf(as->locid_server_name, "%u.%u.%u.%u:%u %s", ip & 255, (ip>>8)&255, (ip>>16)&255, ip>>24, port, server_name);
    }
    else
    { as->notreg=false;
      strcpy(as->locid_server_name, server_name);
    }
    as->odbc = server_type==add_server_odbc;
    as->local = server_type==add_server_local;  // originally "local" may have been changed here for existing servers, but starting locally a network server changed its state from local to network, which was wrong
    as->private_server=private_server;
    as->next=available_servers;  available_servers=as;
    chng=2;
  }
  as->committed=true;
 // set state:
  if (find_server_image(as, server_type==add_server_running) && !chng)
    chng=1;
  if (pas) *pas = as;
  return chng;
}

void server_state_changed(const t_avail_server * as)
{ item_info info;
  info.topcat=TOPCAT_SERVERS;
  info.syscat=SYSCAT_NONE;
  strcpy(info.server_name, as->locid_server_name);
  update_cp_item(info, 0);
}

void CALLBACK Server_scan_callback(const char * server_name, uns32 ip, uns16 port)
{ t_avail_server * as;
 // change the server state image to "running": 
  int chng = add_server(server_name, add_server_running, false, ip, port, &as);
  if (chng==2)
  {// add to the tree: 
    wxGetApp().frame->control_panel->tree_view->add_running_server(as->locid_server_name);
   // add to the list:
    ControlPanelList * cpl = wxGetApp().frame->control_panel->objlist;
    if (!cpl->m_BranchInfo->cdp && !*cpl->m_BranchInfo->server_name)  // servers listed in the list
      cpl->Refresh();
  }
  else if (chng==1)
   // update CP item(s):
    server_state_changed(as);
}

#ifdef LINUX
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
 
static int linux_server_scan_callback(const char *name, struct sockaddr_in* addr, void * data)
{
  Server_scan_callback(name, addr->sin_addr.s_addr, ntohs(addr->sin_port)-1);
  return 1;
}
#endif

void refresh_server_list(void)
{ t_avail_server * as, **pas;
 // mark all server as non-committed (except if connected):
  for (as = available_servers;  as;  as=as->next)
    as->committed = as->cdp!=NULL;
 // relist servers from the registry:
  list_available_servers();
 // remove servers not found in the registry and not connected:
  for (pas = &available_servers;  *pas;  )
    if (!(*pas)->committed && (*pas)->cdp==NULL)
    { as=*pas;
      *pas=as->next;  // removing from the list
      delete as;
    }
    else
      pas=&(*pas)->next;
 // ask all servers to send status info:
#ifdef LINUX  // synchronous (because anync. updating the control panel by another thread crashes the WX 
  enum_servers(400, linux_server_scan_callback, NULL);
#else  // anynchronous
  set_server_scan_callback(Server_scan_callback);
  TcpIpQueryForServer();
#endif
}

bool edit_conn_data(wxWindow * parent, item_info & info, bool register_new_nw_server)
// Edits connection data for the server.
{ NetAccDlg dlg;
  t_avail_server * as;  
  as = find_server_connection_info(info.server_name);
  bool private_server = as && as->private_server;
  dlg.SetServerName(wxString(info.server_name, *wxConvCurrent));
  char buf[IP_ADDR_MAX+1];  unsigned long port, val;
  if (!GetDatabaseString(info.server_name, IP_str, buf, sizeof(buf), private_server)) *buf=0;
  dlg.SetServerAddress(wxString(buf, *wxConvCurrent));
  dlg.SetLocal(!*buf && !register_new_nw_server);
  if (!GetDatabaseString(info.server_name, IP_socket_str, &port, sizeof(port), private_server)) *buf=0;
  else sprintf(buf, "%u", port);
  dlg.SetServerPort(wxString(buf, *wxConvCurrent));
  if (!GetDatabaseString(info.server_name, FWIP_str, buf, sizeof(buf), private_server)) *buf=0;
  dlg.SetSocksAddress(wxString(buf, *wxConvCurrent));
  if (!GetDatabaseString(info.server_name, viaFW_str, &val, sizeof(val), private_server)) val=0;
  dlg.SetFw(val!=0);
  if (!GetDatabaseString(info.server_name, useTunnel_str, &val, sizeof(val), private_server)) val=0;
  dlg.SetTunnel(val!=0);
  dlg.Create(parent);
  if (dlg.ShowModal()==wxID_OK)
  { bool err = false;
    err=!SetDatabaseString(info.server_name, IP_str, dlg.GetLocal() ? "" : dlg.GetServerAddress().mb_str(*wxConvCurrent), private_server);
    if (dlg.GetServerPort().ToULong(&port) && port<=65535)
      err|=!SetDatabaseNum(info.server_name, IP_socket_str, port, private_server);
    else // empty or invalid port number
      err|=!DeleteDatabaseValue(info.server_name, IP_socket_str, private_server);
    err|=!SetDatabaseString(info.server_name, FWIP_str, dlg.GetSocksAddress().mb_str(*wxConvCurrent), private_server); 
    err|=!SetDatabaseNum(info.server_name, viaFW_str, dlg.GetFw() ? 1 : 0, private_server);
    err|=!SetDatabaseNum(info.server_name, useTunnel_str, dlg.GetTunnel() ? 1 : 0, private_server);
    if (err) error_box(ERROR_WRITING_TO_REGISTRY);
    update_cp_item(info, 0); // redraw data in the list
   // update ip/port in the available server list
    if (as)
    { as->port = port;
      as->ip = inet_addr(dlg.GetServerAddress().mb_str(*wxConvCurrent));
    }
    return !err;
  }
  return false;
}

static bool command_disabled = false;

void MyFrame::OnCPCommand( wxCommandEvent& event )
// Send the command to the control panel
{ selection_iterator * si = NULL;
  int comm_id = event.GetId();
 // restore the focus from the toolbar button to the last active child:
  //char msg[100];  sprintf(msg, "%x", wxWindow::FindFocus()->GetHandle());  client_error_message(NULL, msg);
//  SetFocus();  // returns focus to the last selected child
  //sprintf(msg, ">>%x", wxWindow::FindFocus()->GetHandle());  client_error_message(NULL, msg);

#ifdef STOP  // does not work
 // focus-driven commands:
   if (comm_id==CPM_CUT || comm_id==CPM_COPY || comm_id==CPM_PASTE || comm_id==CPM_REFRPANEL || comm_id==CPM_DUPLICATE)
  { if (!command_disabled)
    { command_disabled=true;
      wxWindow::FindFocus()->ProcessEvent(event);
      //wxWindow::FindFocus()->AddPendingEvent(event);
      command_disabled=false;
    }
    return;
  }  
#endif

 // MDI child commands:
  if (comm_id>=FIRST_MDICHILD_MSG && comm_id<=LAST_MDICHILD_MSG)
  { if (competing_frame::active_tool_owner==NULL) { wxBell();  return; }
#ifdef STOP
    if (!global_mdi_style)  // ?? on GTK MDI the designer frame does not receive the EVT_SET_FOCUS and does not pass the focus
      if (competing_frame::active_tool_owner->dsgn_mng_wnd)  // this cannot work for editor in the container (dsgn_mng_wnd==NULL)
        competing_frame::active_tool_owner->dsgn_mng_wnd->SetFocus();
#else
    // sending the focus directly to the designer (works fine on MSW and for GTK auto-accelerators (MDI):
    if (competing_frame::active_tool_owner->focus_receiver)
      //competing_frame::active_tool_owner->focus_receiver->SetFocus();  // this version does not work for menu/toolbar commands on GTK
      competing_frame::active_tool_owner->/*focus_receiver->*/_set_focus();  // this version works for MDI menu/toolbar, but not for popup (on GTK)
#endif
    competing_frame::active_tool_owner->OnCommand(event);
    return;   
  }

  SetFocus();  // focuses the last focused child
 // debugger commands:
  if (comm_id>=FIRST_DBG_MSG && comm_id<=LAST_DBG_MSG)
    if (the_server_debugger)
      the_server_debugger->OnCommand(comm_id);

 // selection-independent commands:
  switch (comm_id)
  { 
    case CPM_ABOUT:
      OnAbout(event);  
      return;
    case CPM_FLDBELCAT:
      control_panel->fld_bel_cat=!control_panel->fld_bel_cat;
      GetMenuBar()->Check(CPM_FLDBELCAT, control_panel->fld_bel_cat);
      control_panel->tree_view->Refresh();
      return;
    case CPM_SHOWFOLDERS:
      control_panel->ShowFolders=!control_panel->ShowFolders;
      GetMenuBar()->Check( CPM_SHOWFOLDERS,    control_panel->ShowFolders);
      GetMenuBar()->Enable(CPM_SHOWEMPTYFLDRS, control_panel->ShowFolders);
      GetMenuBar()->Enable(CPM_FLDBELCAT,      control_panel->ShowFolders);
      control_panel->tree_view->Refresh();
      return;
    case CPM_SHOWEMPTYFLDRS:
      control_panel->ShowEmptyFolders=!control_panel->ShowEmptyFolders;
      GetMenuBar()->Check(CPM_SHOWEMPTYFLDRS, control_panel->ShowEmptyFolders);
      control_panel->tree_view->Refresh();
      return;
    case CPM_UP:
      control_panel->tree_view->select_parent();
      return;
    case CPM_MDI:
    { global_style=ST_MDI;  
#ifdef FL
#if WBVERS>=1000   // show the designer toolbar for the MDI style
      cbBarInfo * bar = mpLayout->FindBarByWindow(bars[BAR_ID_DESIGNER_TOOLBAR]);
      if (!bar) return;
      if (global_style==ST_MDI)
      { if (bar->mState == wxCBAR_HIDDEN || bar->mState == wxCBAR_FLOATING)
          mpLayout->SetBarState(bar, wxCBAR_DOCKED_HORIZONTALLY, TRUE );
        //mpLayout->RedockBar(bar, bars[BAR_ID_DESIGNER_TOOLBAR]->GetRect(), mpLayout->GetPane(FL_ALIGN_TOP), true);
        // is not necessary and removes the fixed size appearance!
      }
      else
        mpLayout->SetBarState(bar, wxCBAR_HIDDEN, TRUE);
#endif
#else  // !FL
      set_style(ST_MDI);  // calls m_mgr.Update()
#endif
      return;
    }
    case CPM_POPUP:
      set_style(ST_POPUP);  // calls m_mgr.Update()
      return;
    case CPM_AUINB:
      set_style(ST_AUINB);  // calls m_mgr.Update()
      return;
    case CPM_CROSSHELP:
      help_ctrl.DisplaySection(wxT("xml/html/clienttop.html"));
      return;
    case CPM_SHOW_HIDE:    case CPM_SHOW_HIDE+1:  case CPM_SHOW_HIDE+2:  case CPM_SHOW_HIDE+3:  case CPM_SHOW_HIDE+4:
    case CPM_SHOW_HIDE+5:  case CPM_SHOW_HIDE+6:  case CPM_SHOW_HIDE+7:  case CPM_SHOW_HIDE+8:  case CPM_SHOW_HIDE+9:
    // a copy from FL:
	  { if (comm_id-CPM_SHOW_HIDE >= BAR_COUNT) return;
#ifdef FL
      cbBarInfo * pBar = mpLayout->FindBarByWindow(bars[comm_id-CPM_SHOW_HIDE]);
      if (!pBar) return;
		  int newState = 0;
		  if ( pBar->mState == wxCBAR_HIDDEN )
		  {	if ( pBar->mAlignment == -1 )
			  {	pBar->mAlignment = 0;       // just remove "-1" marking
				  newState = wxCBAR_FLOATING;
			  }
			  else
			  if ( pBar->mAlignment == FL_ALIGN_TOP || pBar->mAlignment == FL_ALIGN_BOTTOM )
        { pBar->mAlignment = comm_id-CPM_SHOW_HIDE<BAR_ID_FIRST_TOOLBAR ? FL_ALIGN_BOTTOM : FL_ALIGN_TOP;  // preferred location
				  newState = wxCBAR_DOCKED_HORIZONTALLY;
        }
			  else
				  newState = wxCBAR_DOCKED_VERTICALLY;
		  }
		  else
		  { newState = wxCBAR_HIDDEN;
			  if ( pBar->mState == wxCBAR_FLOATING )
				  pBar->mAlignment = -1;
		  }
		  mpLayout->SetBarState( pBar, newState, TRUE );
		  if ( newState == wxCBAR_FLOATING )
			  mpLayout->RepositionFloatedBar( pBar ); 
#else
      int barnum = comm_id-CPM_SHOW_HIDE;
      if (bars[barnum])
        m_mgr.GetPane(bar_name[barnum]).Show(!m_mgr.GetPane(bar_name[barnum]).IsShown());
      m_mgr.Update();  // "commit" all changes made to wxAuiManager
#endif
      return;
    }
#ifdef FL
    case CPM_LAYOUT_MANAGER:
    { LayoutManagerDlg dlg;
      dlg.Create(this);
      if (dlg.ShowModal() == 123)
        info_box(_("Please close the client now. The default frame layout will be restored on startup."), this);
      return;
    }
#endif
    case CPM_DISCONN_ALL:
    { for (t_avail_server * as = available_servers;  as;  as=as->next)
        if (as->cdp)
          try_disconnect(as);  // does not stop if any disconnection is cancelled
      control_panel->RefreshItem(control_panel->tree_view->rootId);
      update_object_info(output_window->GetSelection());
      update_action_menu_based_on_selection();  // update the Connect/Disconnect items
      return;
    }
    case CPM_EXIT:
      Close();
      return;
  }
 // find the selection:
  if (control_panel->objlist->GetSelectedItemCount() > 0)
  {  si = new list_selection_iterator(control_panel->objlist);
     control_panel->objlist->add_list_item_info(control_panel->objlist->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED), si->info);
  }
  else
  { wxTreeItemId selitem = control_panel->tree_view->GetSelection();
    if (selitem.IsOk())
      si = new single_selection_iterator(control_panel->tree_view->m_SelInfo);
  }
 // selection-dependent commands:
  if (si)
  { switch (comm_id)
    { case CPM_CUT:  case CPM_COPY:  case CPM_PASTE:  case CPM_DUPLICATE:  case CPM_RENAME:
        if (!command_disabled)
        { command_disabled=true;
          if (control_panel->objlist->GetSelectedItemCount() > 0)
            control_panel->objlist->ProcessEvent(event);
          else
            control_panel->tree_view->ProcessEvent(event);
          command_disabled=false;
        }
        break;
      case CPM_REFRPANEL:
        control_panel->RefreshPanel();
        break;
      case CPM_DELETE:
        control_panel->OnDelete(*si);  break;
      case CPM_NEW:  
        control_panel->OnNew(*si);  break;
      case CPM_NEW_TEXT:  // used only for CATEG_CURSOR and CATEG_VIEW
      { if (!si->next()) return;  // supposed to be impossible
       // check the privilege to insert records to system tables:
        uns8 priv_val[PRIVIL_DESCR_SIZE];
        if (cd_GetSet_privils(si->info.cdp, NOOBJECT, CATEG_USER, OBJ_TABLENUM, NORECNUM, OPER_GETEFF, priv_val)
            || !(*priv_val & RIGHT_INSERT))
        { client_error(si->info.cdp, NO_RIGHT);  cd_Signalize(si->info.cdp);
          break;
        }
        open_text_object_editor(si->info.cdp, OBJ_TABLENUM, NOOBJECT, OBJ_DEF_ATR, si->info.category, si->info.folder_name, OPEN_IN_EDCONT);
        break;
      }
      case CPM_OPEN:
        while (si->next()) open_object(this, si->info);
        break;
#ifdef WINS
#ifdef XMLFORMS
      case CPM_ALT_OPEN:
        while (si->next()) open_object_alt(this, si->info);
        break;
#endif
#endif
      case CPM_MODIFY:
        control_panel->OnModify(*si, false);  break;
      case CPM_MODIFY_ALT:
#if 0
      { char * resp = NULL;  const char * content_type;  tobjname realm;
        char * options[10];
        memset(options, 0, sizeof(options));
        options[HTTP_REQOPT_PATH] = "/manycols/filler/SHOWQUERY/table odds?mez=2";  // http://empo.software602.cz/php/602.php/manycols/Filler
        get_xml_response(si->info.cdp, HTTP_METHOD_GET, options, NULL, 0, &resp, &content_type, realm);
      }
#endif
        control_panel->OnModify(*si, true);  break;
      case CPM_EDIT_SOURCE:  // available only for queries, XML transports, style sheets, no need to check topcat and syscat
        while (si->next()) 
          open_text_object_editor(si->info.cdp, OBJ_TABLENUM, si->info.objnum, OBJ_DEF_ATR, si->info.category, si->info.folder_name, OPEN_IN_EDCONT);
        break;
      case CPM_RUN:
        control_panel->OnRun(*si);  break;
#ifdef WINS
#ifdef XMLFORMS
      case CPM_NEW_XMLFORM:
        control_panel->new_XML_form(si->info); break;
#endif
#endif
      case CPM_PRIVILS:
        control_panel->OnObjectPrivils(*si);  break;
      case CPM_DATAPRIV:
        control_panel->OnDataPrivils(*si);  break;
      case CPM_DATAIMP:
      { if (!si->next()) break;
        t_ie_run * dsgn = new t_ie_run(si->info.cdp, false);  if (!dsgn) break;
        dsgn->source_type=IETYPE_CSV;  dsgn->dest_type=IETYPE_WB;
        *dsgn->inpath=0;
        strcpy(dsgn->outpath, si->info.object_name);
        *dsgn->cond=0;
        dsgn->index_past=false;
        start_transport_wizard(si->info, dsgn, false);
        break;
      }
      case CPM_DATAEXP:
      { if (!si->next()) break;
        t_ie_run * dsgn = new t_ie_run(si->info.cdp, false);  if (!dsgn) break;
        dsgn->source_type=si->info.category==CATEG_TABLE ? IETYPE_WB : IETYPE_CURSOR;  
        dsgn->dest_type=IETYPE_CSV;
        strcpy(dsgn->inpath, si->info.object_name);
        *dsgn->outpath=0;
        *dsgn->cond=0;
        dsgn->index_past=false;
        start_transport_wizard(si->info, dsgn, false);
        break;
      }
      case CPM_TABLESTAT:
      { if (!si->next()) break;
        TableStatDlg dlg(si->info.cdp, si->info.objnum, wxString(si->info.object_name, *si->info.cdp->sys_conv), wxString(si->info.schema_name, *si->info.cdp->sys_conv));
        dlg.Create(this);
        dlg.ShowModal();
#if 0
{        
   t_clivar cli[2];
   strcpy(cli[0].name, "TX");
   cli[0].mode=MODE_OUT;
   cli[0].buf=(char*)corealloc(5000011, 55);
   memset(cli[0].buf, 'B', 5000010);  ((char*)cli[0].buf)[5000010]=0;
   cli[0].buflen=5000011;
   cli[0].actlen=5000010;
   cli[0].specif=0;
   cli[0].wbtype=ATT_TEXT;
   for (int i=0;  i<25; i++)
     cd_SQL_host_execute(si->info.cdp, /*"UPDATE EXTCLOBS set EXC=:TX WHERE ID=1"*/
       "SELECT EXC INTO :TX FROM EXTCLOBS WHERE ID=1", NULL, cli, 1);
}        
#endif
        break;
      }
      case CPM_OPTIM:
      { if (!si->next()) break;
        char * source = cd_Load_objdef(si->info.cdp, si->info.objnum, CATEG_CURSOR);
        QueryOptimizationDlg dlg(si->info.cdp, source);
        dlg.Create(this);
        dlg.ShowModal();
        corefree(source);
        break;
      }
      case CPM_SYNTAX:
        control_panel->OnSyntaxValidation(*si);  break;
      case CPM_CREATE_ODBC_DS:
      { if (!si->next()) break;
        CreateODBCDataSourceDlg dlg(wxString(si->info.cdp->locid_server_name, *wxConvCurrent), 
                                    wxString(si->info.schema_name, *si->info.cdp->sys_conv));
        dlg.Create(this);
        if (dlg.ShowModal()==wxID_OK)
          control_panel->RefreshPanel();  // updates the list of data sources
        break;
      }  
      case CPM_COMPACT: 
        control_panel->OnCompacting(*si);  break;
      case CPM_FREEDEL:
        control_panel->OnFreeDeleted(*si);  break;
      case CPM_REINDEX:
        control_panel->OnReindex(*si);  break;
      case CPM_RESTART: // sequence
        control_panel->OnSequenceRestart(*si);  break;
      case CPM_REG_SRV:  // register a new network server
      { item_info info;
        info.topcat=TOPCAT_SERVERS;
        *info.server_name=0;
        while (enter_object_name(NULL, this, _("The name of the remote SQL server:"), info.server_name, false, CATEG_SERVER)) 
        { if (IsServerRegistered(info.server_name))
          { error_box(SERVER_NAME_DUPLICITY);
            continue;
          }
          bool private_server = false;
          if (!srv_Register_server_local(info.server_name, NULL, private_server))
          { private_server=true;
            if (!srv_Register_server_local(info.server_name, NULL, private_server))
              { error_box(ERROR_WRITING_TO_REGISTRY);  break; }
          }
          add_server(info.server_name, add_server_registered, private_server);  // must be before insert_cp_item
          insert_cp_item(info, 0, true);
          edit_conn_data(this, info, true);
          break;  // stop entering the name
        }
        break;
      }
      case CPM_ODBC_MANAGE:
      { item_info item;  item.cti=IND_ODBCTAB;
        client_action(control_panel, item);
        break;
      }  
#ifdef STOP  // moved to the dialog
      case CPM_PING:
        control_panel->OnPing(*si);  break;
#endif
      case CPM_RELOGIN:
        control_panel->OnRelogin(*si);  break;
      case CPM_CONNSPD:
        control_panel->OnConnSpeed(*si);  break;
      case CPM_CONNECT:
        control_panel->OnConnect(*si);  break;
      case CPM_DISCONN:
        control_panel->OnDisconnect(*si);  break;
      case CPM_START_SERVER:
      { if (!si->next()) return;  // supposed to be impossible
        int res;
        { wxBusyCursor wait;
#ifdef UNIX
          res=srv_Start_server_local(si->info.server_name, 0, NULL);
#else        
          res=srv_Start_server_local(si->info.server_name, SW_SHOWNORMAL, NULL);
#endif    
        }
        if (res != KSE_OK)
          { error_box(_("Cannot start the SQL server"));  break; }
        t_avail_server *as = find_server_connection_info(si->info.server_name);
        if (as)
        { find_server_image(as, false);
          server_state_changed(as);
        }
        break;
      }
      case CPM_STOP_SERVER:
      { if (!si->next()) return;  // supposed to be impossible
        int wait_time=0;
        if (si->info.cdp)  // connected
        { //if (IsShiftDown() && IsCtrlDown())
          //  { cd_Crash_server(si->info.cdp);  break; }
          unsigned clients;
          if (!close_contents_from_server(si->info.cdp, si->info.cdp))
            return;
          if (!cd_Get_server_info(si->info.cdp, OP_GI_CLIENT_COUNT, &clients, sizeof(clients)) && clients>1)
          { ClientsConnectedDlg dlg(this);
            wxString msg;
            msg.Printf(_("%u clients are connected to the SQL server at the momment."), clients-1);
            dlg.mMsg->SetLabel(msg);
            dlg.Fit();
            if (dlg.ShowModal() == wxID_CANCEL) 
              return;
            if (dlg.mOpt->GetSelection()==0) wait_time=60;
          }
        }
        int err=srv_Stop_server_local(si->info.server_name, wait_time, si->info.cdp);
        if (err>=128) cd_Signalize(si->info.cdp);
#ifdef STOP  // these errors are not returned, stopping the server continues by other method
        if      (err==EPERM) error_box(_("You do not have the permission to stop the server."));
        else if (err==ESRCH) error_box(_("The server process was not found."));
#endif
        else if (err!=0) error_box(_("Error stopping the server.")); 
        wxThread::Sleep(800);
        t_avail_server *as = find_server_connection_info(si->info.server_name);
        if (as)
        { find_server_image(as, false);
          server_state_changed(as);
        }
        break;
      }
      case CPM_DB_MN:
      { if (!si->next()) break;  // impossible
        DatabaseDlg dlg(si->info.server_name);
        dlg.Create(this);
        if (dlg.ShowModal() == wxID_CLEAR)  // remove the 
        { remove_server(si->info.server_name);
         // remove from the control panel:  ###

        }
        break;
      }
      case CPM_SEARCH:
      { if (!si->next()) break;
        SearchObjectsDlg dlg(si->info.cdp, wxEmptyString, si->info.schema_name, *si->info.folder_name ? si->info.folder_name : NULL);
        dlg.Create(this);
        dlg.ShowModal();
        break;
      }
      case CPM_DBGATT:
      { if (the_server_debugger)
          { unpos_box(_("A debug session is already in progress."));  break; }
        SelectClientDlg dlg(si->info.cdp);
        dlg.Create(this);
        if (dlg.ShowModal() == wxID_OK)
        { int sel = dlg.mList->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
          if (sel!=-1) attach_debugging(si->info.cdp, (int)dlg.mList->GetItemData(sel));
        }
        break;
      }
      case CPM_MONITOR:  // attaches the server to the monitor
      { if (!monitor_window) return;  // internal error
       // show the monitor pane, if hidden:
#ifdef FL
        cbBarInfo * bar = mpLayout->FindBarByWindow(bars[BAR_ID_MONITOR]);
        if (bar && bar->mState == wxCBAR_HIDDEN)
          mpLayout->SetBarState(bar, wxCBAR_DOCKED_HORIZONTALLY, true);
#else
        m_mgr.GetPane(bar_name[BAR_ID_MONITOR]).Show();
        m_mgr.Update();  // "commit" all changes made to wxAuiManager
#endif
        monitor_window->AttachToServer(si->info.cdp);
        break;
      }	
      case CPM_SEL_USERS:
        if (si->next())  //  single selection supposed
          Edit_relation(si->info.cdp, this, si->info.category, si->info.objnum, CATEG_USER);
        break;
      case CPM_SEL_GROUPS:
        if (si->next())  //  single selection supposed
          Edit_relation(si->info.cdp, this, si->info.category, si->info.objnum, CATEG_GROUP);
        break;
      case CPM_SEL_ROLES:
        if (si->next())  //  single selection supposed
          Edit_relation(si->info.cdp, this, si->info.category, si->info.objnum, CATEG_ROLE);
        break;
      case CPM_LIST_USERS:
        if (si->next())  //  single selection supposed
          List_users(si->info.cdp, this, si->info.object_name, si->info.objnum);
        break;
      case CPM_LIST_TRIGGERS:
        if (si->next())  //  single selection supposed
          List_triggers(si->info.cdp, this, si->info.object_name, si->info.objnum);
        break;
      case CPM_CREATE_TRIGGER:
        if (si->next())  //  single selection supposed
          if (create_trigger(this, si->info, si->info.object_name))  // will overwrite the contents of si->info
          { Set_object_folder_name(si->info.cdp, si->info.objnum, CATEG_TRIGGER, si->info.folder_name);
            Add_new_component(si->info.cdp, CATEG_TRIGGER, si->info.schema_name, si->info.object_name, 
                              si->info.objnum, 0, si->info.folder_name, true);
          }
        break;
      case CPM_IMPORT:
        if (si->info.IsServer())
        { if (ImportSchema(si->info.cdp))
          { ControlPanelTree *Tree = control_panel->tree_view;
            wxTreeItemId Item;
            if (Tree->GetSelInfo()->IsServer())
              Item = Tree->GetSelItem();
            else
              Item = Tree->FindItem(Tree->rootId, si->info, true);
            if (Item.IsOk())
            { strcpy(si->info.schema_name, si->info.cdp->sel_appl_name);
              Item = Tree->FindItem(Item, si->info, true);
              if (Item.IsOk())
                Tree->SelectItem(Item);
            }
          }
        }
        else if (ImportSelObjects(si->info.cdp, si->info.category, si->info.folder_name) > 0)
          control_panel->RefreshSelItem();
        break;
      case CPM_EXPORT:
        control_panel->OnExport(*si);
        break;
      case CPM_CREFLD:
      {
        if (control_panel->m_ActiveChild == (wxControl *)control_panel->tree_view)
          control_panel->tree_view->NewFolder();
        else
          control_panel->objlist->NewFolder();
        break;
      }
    }
    delete si;
  }
  else wxBell(); // unusuall, perhaps imposible: nothing selected in the tree nor in the list
}

void MyFrame::update_action_menu_based_on_selection(bool can_paste)
{
 redo: 
  item_info info = control_panel->tree_view->m_SelInfo;
  if (control_panel->objlist->GetSelectedItemCount() > 0)
  { control_panel->objlist->add_list_item_info(control_panel->objlist->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED), info);
    info.object_count=control_panel->objlist->GetSelectedItemCount();
  }
  else
  { wxTreeItemId selitem = control_panel->tree_view->GetSelection();
    if (!selitem.IsOk())
      return;
  }
 // check the server connection: 
  if (info.topcat==TOPCAT_SERVERS)
    if (info.syscat==SYSCAT_APPLS && info.object_count==1 ||  // this is the top of the server subtree
        info.syscat==SYSCAT_APPLTOP || info.syscat==SYSCAT_CATEGORY || info.syscat==SYSCAT_OBJECT)  // inside a server
    // check for dead connection:
      if (info.cdp)
        if (cd_is_dead_connection(info.cdp))
        { t_avail_server * as = find_server_connection_info(info.server_name);
          find_server_image(as, false);  // disconnects
          server_state_changed(as);
          control_panel->RefreshPanel();
          goto redo;
        }
        
 // update menus:
  wxMenuBar *MenuBar = GetMenuBar();
  if (MenuBar)
  {// Connection menu:
    if ((info.topcat==TOPCAT_SERVERS || info.topcat==TOPCAT_ODBCDSN) && *info.server_name)
    { wxString label;  
      label.Printf(_("&Connect to %s"), wxString(info.server_name, *wxConvCurrent).c_str());
      connection_menu->SetLabel(CPM_CONNECT, label);
      connection_menu->Enable(CPM_CONNECT, info.xcdp()==NULL);
      label.Printf(_("&Disconnect from %s"), wxString(info.server_name, *wxConvCurrent).c_str());
      connection_menu->SetLabel(CPM_DISCONN, label);
      connection_menu->Enable(CPM_DISCONN, info.xcdp()!=NULL);
      label.Printf(_("&Relogin on %s"), wxString(info.server_name, *wxConvCurrent).c_str());
      connection_menu->SetLabel(CPM_RELOGIN, label);
      connection_menu->Enable(CPM_RELOGIN, info.cdp!=NULL);
    }
    else
    { connection_menu->SetLabel(CPM_CONNECT, _("&Connect to Selected Server"));
      connection_menu->Enable(CPM_CONNECT, false);
      connection_menu->SetLabel(CPM_DISCONN, _("&Disconnect from Selected Server"));
      connection_menu->Enable(CPM_DISCONN, false);
      connection_menu->Enable(CPM_RELOGIN, false);
    }
    bool any_server_connected = false;
    for (t_avail_server * as = available_servers;  as;  as=as->next)
      if (as->cdp || as->conn)
        { any_server_connected = true;  break; }
    connection_menu->Enable(CPM_DISCONN_ALL, any_server_connected);

   // Object menu:
    int index  = MenuBar->FindMenu(ACTION_MENU_STRING);  
#ifdef STOP  // index depends on the maximization of MDI children - before 2.5.1 only
    int is_max; // MSW BOOL
    if (SendMessage((HWND)mpClientWnd->GetHandle(), WM_MDIGETACTIVE, 0, (LPARAM)&is_max) && is_max)
      index++;
#endif
    if (index!=wxNOT_FOUND)
    { wxMenu * object_menu = MenuBar->GetMenu(index);
     // remove the old items: 
      while (object_menu->GetMenuItemCount() > 0)      
#ifdef LINUX
        object_menu->Remove(object_menu->FindItemByPosition(0));
#else
        object_menu->Destroy(object_menu->FindItemByPosition(0));
#endif
      info.make_control_panel_popup_menu(object_menu, true, can_paste);
      if (object_menu->FindItem(CPM_CUT)  !=NULL) object_menu->Enable(CPM_CUT,   info.object_count != 0 && info.category!=CATEG_INFO);
      if (object_menu->FindItem(CPM_COPY) !=NULL) object_menu->Enable(CPM_COPY,  info.object_count != 0 && info.category!=CATEG_INFO);
      if (object_menu->FindItem(CPM_PASTE)!=NULL) object_menu->Enable(CPM_PASTE, can_paste && info.category!=CATEG_INFO);
    }
  }
}


void MyFrame::OnExit( wxCommandEvent& event )
{
    Destroy();
}

///////////////////////////////////// available servers //////////////////////////////
t_avail_server * available_servers = NULL;

t_avail_server * WINAPI GetServerList(){return available_servers;}

void list_available_servers(void)
// Listing all registered 602SQL servers.
{
  tobjname a_server_name;  char a_path[MAX_PATH];
  t_enum_srv es(t_enum_srv::ENUM_SRV_BOTH, CURRENT_FIL_VERSION_STR);
  while (es.next(a_server_name))
  { es.GetPrimaryPath(a_server_name, a_path);
    char buf[IP_ADDR_MAX+1];  unsigned long val;
    if (!es.GetDatabaseString(a_server_name, IP_str, buf, sizeof(buf))) *buf=0;
    if (!es.GetDatabaseString(a_server_name, IP_socket_str, &val, sizeof(val))) val=DEFAULT_IP_SOCKET;
   // adding the server:
    add_server(a_server_name, *a_path ? add_server_local : add_server_registered, es.is_private_server(), inet_addr(buf), val);
  }

#ifdef CLIENT_ODBC_9
 // listing ODBC servers
  char dsn[SQL_MAX_DSN_LENGTH+1], descr[512];  
 // list registered data sources:
  BOOL success = ODBC_data_sources(true, dsn, sizeof(dsn), descr, sizeof(descr));
  while (success)  // stops with SQL_NO_DATA returned
  { t_avail_server * as;
    add_server(strlen(dsn) <= OBJ_NAME_LEN ? dsn : "<DSN too long>", add_server_odbc, false, 0, 0, &as);
    if (as && *descr)
    { if (as->description) { delete [] as->description;  as->description=NULL; }
      as->description = new char[strlen(descr)+1];
      if (as->description) strcpy(as->description, descr);
    }
    success = ODBC_data_sources(false, dsn, sizeof(dsn), descr, sizeof(descr));
  }
#endif
}

t_avail_server * find_server_connection_info(const char * locid_server_name)
{ t_avail_server * as;
  // no more searching by the database name!
  for (as = available_servers;  as;  as=as->next)
    if (!my_stricmp(as->locid_server_name, locid_server_name))
      return as;
  return NULL;
}

void remove_server(const char * server_name)
{ t_avail_server ** pas = &available_servers;
  while (*pas)
  { t_avail_server * as = *pas;
    if (!my_stricmp(as->locid_server_name, server_name))
    { *pas=as->next;  delete as;
      wxGetApp().frame->control_panel->RefreshPanel();
      break;
    }
    pas=&as->next;
  }
}

bool is_port_unique(uns16 port, const char * except_server_name)
// Returns false if there is a local server with [port] port number other than [except_server_name].
{ for (t_avail_server * as = available_servers;  as;  as=as->next)
    if (!as->odbc && my_stricmp(as->locid_server_name, except_server_name))
    { unsigned long aport;
      if (!GetDatabaseString(as->locid_server_name, IP_socket_str, &aport, sizeof(aport), as->private_server)) aport=5001;
      if (aport==port) return false;
    }
  return true;
}
////////////////////////////////////// control panel //////////////////////////////////////
void image2categ_flags(int image, tcateg & category, int & flags)
{ flags=0;  
  switch (image)
  { case IND_TABLE: 
      category=CATEG_TABLE;  break;
    case IND_ODBCTAB:
      category=CATEG_TABLE;  flags=CO_FLAG_ODBC;  break;
    case IND_REPLTAB:
      category=CATEG_TABLE;  flags=CO_FLAG_REPLTAB;  break;
/*  case IND_VIEW:  
      category=CATEG_VIEW;  break;
    case IND_STVIEW: 
      category=CATEG_VIEW;  flags=CO_FLAG_SIMPLE_VW;  break;
    case IND_LABEL: 
      category=CATEG_VIEW;  flags=CO_FLAG_LABEL_VW;  break;
    case IND_GRAPH: 
      category=CATEG_VIEW;  flags=CO_FLAG_GRAPH_VW;  break;
    case IND_REPORT:
      category=CATEG_VIEW;  flags=CO_FLAG_REPORT_VW;  break;
    case IND_MENU:  
      category=CATEG_MENU;  break;
    case IND_POPUP:
      category=CATEG_MENU;  flags=CO_FLAG_POPUP_MN;  break;
*/
    case IND_CURSOR:
      category=CATEG_CURSOR;  break;
    case IND_PROGR: 
      category=CATEG_PGMSRC;  break;
    case IND_INCL: 
      category=CATEG_PGMSRC;  flags=CO_FLAG_INCLUDE;  break;
    case IND_TRANSP:
      category=CATEG_PGMSRC;  flags=CO_FLAG_TRANSPORT;  break;
    case IND_XML:
      category=CATEG_PGMSRC;  flags=CO_FLAG_XML;  break;
    case IND_PICT:
      category=CATEG_PICT;  break;
    case IND_ROLE:
      category=CATEG_ROLE;  break;
    case IND_RELAT:
      category=CATEG_RELATION;  break;
    case IND_CONN:
      category=CATEG_CONNECTION;  break;
    case IND_USER:
      category=CATEG_USER;  break;
    case IND_GROUP:
      category=CATEG_GROUP;  break;
    case IND_DRAW:
      category=CATEG_DRAWING;  break;
    case IND_REPLSRV:
      category=CATEG_SERVER;  break;
/*    case IND_BADCONN:
      category=CATEG_CONNECTION;  break;
*/
    case IND_PROC:
      category=CATEG_PROC;  break;
    case IND_TRIGGER:
      category=CATEG_TRIGGER;  break;
    case IND_FOLDER:
      category=CATEG_FOLDER;  break;
    case IND_SEQ:
      category=CATEG_SEQ;  break;
    case IND_DOM:
      category=CATEG_DOMAIN;  break;
    case IND_FULLTEXT:
      category=CATEG_INFO;  break;
    case IND_FULLTEXT_SEP:
      category=CATEG_INFO;  flags=CO_FLAG_FT_SEP;  break;
#ifdef WINS
#ifdef XMLFORMS
    case IND_XMLFORM:
      category=CATEG_XMLFORM;  flags=0;  break;
#endif
#endif
    case IND_STSH:
      category=CATEG_STSH;  flags=0;  break;
    case IND_STSHCASC:
      category=CATEG_STSH;  flags=CO_FLAG_CASC;  break;
  }
}

#ifdef WINS
#ifdef XMLFORMS

const wxString &XMLFormDesignerPath()
{ static wxString Path = wxEmptyString;
  static bool First = false;
  if (!First)
  { First = true;
    HKEY hKey;
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, wxT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\{571584E0-B7F4-4B80-A4D9-04D94783C155}"), 0, KEY_READ, &hKey) == ERROR_SUCCESS)
    { RegCloseKey(hKey);
      if (RegOpenKeyEx(HKEY_CLASSES_ROOT, wxT("602XML.Document.FO_D\\shell\\open\\command"), 0, KEY_READ, &hKey) == ERROR_SUCCESS)
      { wchar_t Buf[MAX_PATH];
        DWORD Size = sizeof(Buf);
        if (RegQueryValueEx(hKey, NULL, NULL, NULL, (LPBYTE)Buf, &Size) == ERROR_SUCCESS)
          Path = Buf;
        RegCloseKey(hKey);
      }
    }
  }
  return Path;
}

const wxString &XMLFormFillerPath()
{ static wxString Path = wxEmptyString;
  static bool First = false;
  if (!First)
  { First = true;
    HKEY hKey;
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, wxT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\{E61CAE2E-6D6E-43C1-941B-17A69BC144C5}"), 0, KEY_READ, &hKey) == ERROR_SUCCESS)
    { RegCloseKey(hKey);
      if (RegOpenKeyEx(HKEY_CLASSES_ROOT, wxT("602XML.Document.FO_F\\shell\\open\\command"), 0, KEY_READ, &hKey) == ERROR_SUCCESS)
      { wchar_t Buf[MAX_PATH];
        DWORD Size = sizeof(Buf);
        if (RegQueryValueEx(hKey, NULL, NULL, NULL, (LPBYTE)Buf, &Size) == ERROR_SUCCESS)
          Path = Buf;
        RegCloseKey(hKey);
      }
    }
  }
  return Path;
}

#endif
#endif

wxString get_stsh_editor_path(bool for_cascaded_style)
{ char buf[256];
  if (!read_profile_string("Editors", for_cascaded_style ? "CSS" : "StyleSheet", buf, sizeof(buf)))
    *buf=0;
  return wxString(buf, *wxConvCurrent);
}

void item_info::make_control_panel_popup_menu(wxMenu * popup_menu, bool formain, bool can_paste) // bool from_tree
// If [from_tree] then the menu contains both items form manipulating the selected object and for adding children.
// Otherwise the menu contains items for manipulating the selected object(s) and possibly for adding new objects
//   (especially when no objects are selected).
// When multiple objects are selected then [this] contains information about the 1st object, but [object_count]>1.
// [object_count]==0 when the popup menu is created for the empty part of the list view. [this] describes the object selected in the tree.
{ 
  switch (topcat)
  { case TOPCAT_SERVERS:
    case TOPCAT_ODBCDSN:
      if (!*server_name)
      { if (object_count>1)  // multiple servers selected in the list
          ;  // multiple unregistering etc. not supported!
        else  // parent of all servers selected in the tree
        { if (topcat==TOPCAT_SERVERS)
          { AppendXpmItem(popup_menu, CPM_NEW, _("Create a New Local SQL Server"), cre_srv_xpm);
            AppendXpmItem(popup_menu, CPM_REG_SRV, _("Register a Remote SQL Server"), reg_srv_xpm);
          }
          else if (ODBCConfig_available) 
            AppendXpmItem(popup_menu, CPM_ODBC_MANAGE, _("ODBC Manager"), odbcs_xpm);
        }
        break;
      }
     // server is specified
      if (syscat==SYSCAT_APPLS)  // this is the top of the server subtree
      {// check for dead connection:
        if (object_count==1)
          if (cdp)
            if (cd_is_dead_connection(cdp))
            { t_avail_server * as = find_server_connection_info(server_name);
              find_server_image(as, false);  // disconnects
              server_state_changed(as);
              cdp=NULL;  // remove invalid pointer from this
            }
        if (!xcdp())  // server not connected
        { if (object_count==1)
          { AppendXpmItem(popup_menu, CPM_CONNECT, _("Connect"), plug_xpm);
            t_avail_server * as = find_server_connection_info(server_name);
            if (as && !as->notreg && !as->odbc)
            { popup_menu->AppendSeparator();
              AppendXpmItem(popup_menu, CPM_MODIFY, _("Server Connection Data"), plug_edit_xpm);  // not "Network connection" because the dialog containt the port number which is important for Linux local connections too
              int st;
              if (as->local)
              { st = srv_Get_state_local(server_name);
                if (st==0)  // server is local and not running
                  AppendXpmItem(popup_menu, CPM_START_SERVER, _("Start Server"));
                if (st==1 || st==2) // server is local and running
                  AppendXpmItem(popup_menu, CPM_STOP_SERVER, _("Stop Server"));
              }
              else st=-1;
              popup_menu->AppendSeparator();
              if (flags)
                AppendXpmItem(popup_menu, CPM_DELETE, _("Unregister the SQL Server"), unreg_srv_xpm);
              else
              { AppendXpmItem(popup_menu, CPM_DB_MN, _("Database Management"), server_edit_data_xpm);  // erasing the database is inside
                if (st!=0)  // unless local and NOT running (must not allow erasing the database, unregistering the service etc when running)
                  popup_menu->Enable(CPM_DB_MN, false);
              }
            } // registered server
          }
        }
        else // server connected
        { if (object_count==1)  // server selected in the tree
            AppendXpmItem(popup_menu, CPM_DISCONN, _("Disconnect"), dis_plug_xpm);
          if (topcat==TOPCAT_SERVERS)
          { if (object_count==1)  // server selected in the tree
            { AppendXpmItem(popup_menu, CPM_STOP_SERVER, _("Stop Server"));
              AppendXpmItem(popup_menu, CPM_RELOGIN, _("Relogin"));
              popup_menu->AppendSeparator();
              AppendXpmItem(popup_menu, CPM_MONITOR, _("&Monitor the server"));
              //if (cdp->pRemAddr)
                AppendXpmItem(popup_menu, CPM_CONNSPD, _("Connection Speed"));
              AppendXpmItem(popup_menu, CPM_SEARCH, _("&Find within Objects on the Server..."), data_find_xpm);
              AppendXpmItem(popup_menu, CPM_DBGATT, _("&Attach the Debugger to a Running Client..."));
              popup_menu->AppendSeparator();
            }
            if (object_count<=1)  // server selected or no selection in the list
            { AppendXpmItem(popup_menu, CPM_NEW, _("New Application"), new_appl_xpm);
              AppendXpmItem(popup_menu, CPM_IMPORT, _("Import Application"), import_appl_xpm);
            }
            if (object_count>1)  // multiple schemas selected
            { AppendXpmItem(popup_menu, CPM_EXPORT, _("Export application"), export_obj_xpm);
              AppendXpmItem(popup_menu, CPM_DELETE, MNST_DELETE, delete_xpm);
            }
          }
        }
      }
      else if (syscat==SYSCAT_APPLTOP)  // schema or schemas specified
      { if (topcat==TOPCAT_ODBCDSN) break;
        if (!*folder_name)
        { if (object_count==1)
            AppendXpmItem(popup_menu, CPM_EXPORT, _("Export application"), export_obj_xpm);
          AppendXpmItem(popup_menu, CPM_DELETE, _("Delete application"), delete_xpm, object_count != 0);
          if (object_count==1)
          { AppendXpmItem(popup_menu, CPM_PASTE, _("Paste object(s)"), Vlozit_xpm, can_paste);
#ifndef CONTROL_PANEL_INPLECE_EDIT
            AppendXpmItem(popup_menu, CPM_RENAME, _("Rename application"));
#endif
          }
        }
        else
        { AppendXpmItem(popup_menu, CPM_DELETE, MNST_DELETE, delete_xpm, object_count != 0);
          AppendXpmItem(popup_menu, CPM_CUT,   _("Cut object(s)"),   cut_xpm, object_count != 0 && category!=CATEG_INFO);
          AppendXpmItem(popup_menu, CPM_COPY,  _("Copy object(s)"),  Kopirovat_xpm, object_count != 0 && category!=CATEG_INFO);
          AppendXpmItem(popup_menu, CPM_PASTE, _("Paste object(s)"), Vlozit_xpm, can_paste && category!=CATEG_INFO);
#ifndef CONTROL_PANEL_INPLECE_EDIT
          if (IsFolder())
            AppendXpmItem(popup_menu, CPM_RENAME, _("Rename folder"));
#endif
        }
        if (object_count==1)
        { popup_menu->AppendSeparator();
          AppendXpmItem(popup_menu, CPM_SEARCH, *folder_name ? _("&Find within Objects in the Folder...") : _("&Find within Objects in the Application..."), data_find_xpm);
          if (!*folder_name) // unless selected a folder in the schema
          { AppendXpmItem(popup_menu, CPM_SYNTAX, _("Validate Syntax"), validateSQL_xpm); 
            AppendXpmItem(popup_menu, CPM_CREATE_ODBC_DS, _("Create ODBC Data Source"), _conn_xpm);
          }  
        }
        if (object_count<=1)
        { if (!ControlPanel()->fld_bel_cat && (tree_info || object_count == 0))
          { popup_menu->AppendSeparator();
            AppendXpmItem(popup_menu, CPM_CREFLD, MNST_CREFLD, folder_add_xpm);
          }
        }
      }
      else if (syscat==SYSCAT_CATEGORY)  // cannot select multiple categories because categories are never listed in the list view
      { if (topcat==TOPCAT_ODBCDSN) break;
       // one or more folders may be selected if folders are below categories
        if (*schema_name || category==CATEG_USER || category==CATEG_GROUP || category==CATEG_SERVER)
          if (category!=CATEG_VIEW)
            AppendXpmItem(popup_menu, CPM_NEW, MNST_NEW, new_xpm);
          else
            AppendXpmItem(popup_menu, CPM_NEW_TEXT, MNST_NEW, new_xpm);
        if (category==CATEG_USER || category==CATEG_GROUP)
          AppendXpmItem(popup_menu, CPM_DATAPRIV, _("Privilege to Create"));
        if (category==CATEG_CURSOR)
          AppendXpmItem(popup_menu, CPM_NEW_TEXT, _("New Query in SQL"));
        if (category!=CATEG_ROLE && *schema_name) 
          AppendXpmItem(popup_menu, CPM_IMPORT, MNST_IMPORT, import_obj_xpm);
        if (CCategList::HasFolders(category) && (ControlPanel()->fld_bel_cat || IsFolder()) && (cti != IND_SYSTABLE))  // not for roles and system objects
        { popup_menu->AppendSeparator();
          AppendXpmItem(popup_menu, CPM_CREFLD, MNST_CREFLD, folder_add_xpm);
        }
        if (*schema_name)
        { //if (formain)
          //  AppendXpmItem(popup_menu, CPM_DELETE, MNST_DELETE, delete_xpm, object_count != 0);
          //else
          {
            popup_menu->AppendSeparator();
            AppendXpmItem(popup_menu, CPM_DELETE, MNST_DELETE, delete_xpm, object_count != 0);
            if (tree_info || object_count)
            { if (category != CATEG_TABLE)
                AppendXpmItem(popup_menu, CPM_CUT,  _("Cut object(s)"),  cut_xpm, object_count != 0 && /* category!=CATEG_INFO && */ CanCopy());
              AppendXpmItem(popup_menu, CPM_COPY, _("Copy object(s)"), Kopirovat_xpm, object_count != 0 && /* category!=CATEG_INFO && */ CanCopy());
            }
            AppendXpmItem(popup_menu, CPM_PASTE, _("Paste object(s)"), Vlozit_xpm, can_paste /*&& category!=CATEG_INFO*/);
          }
#ifndef CONTROL_PANEL_INPLECE_EDIT
          if (IsFolder())
            AppendXpmItem(popup_menu, CPM_RENAME, _("Rename folder"));
#endif
        }
      }
      else if (syscat==SYSCAT_OBJECT)
      { switch (category)
        { case CATEG_TABLE:
            if (object_count>=1)
            { AppendXpmItem(popup_menu, CPM_OPEN,   MNST_OPEN, table_sql_run_xpm);
              if (*schema_name && topcat==TOPCAT_SERVERS) 
                AppendXpmItem(popup_menu, CPM_MODIFY, MNST_ALTER, modify_xpm);
            }
            if (topcat==TOPCAT_ODBCDSN) break;
            if (object_count>=1)
              if (*schema_name) AppendXpmItem(popup_menu, CPM_DELETE, MNST_DELETE, delete_xpm);
            if (object_count>=1)
            { popup_menu->AppendSeparator();
              if (*schema_name)
              { AppendXpmItem(popup_menu, CPM_EXPORT, MNST_EXPORT, export_obj_xpm);
                AppendXpmItem(popup_menu, CPM_PRIVILS,MNST_PRIVILS, privil_obj_xpm);
                popup_menu->AppendSeparator();
                if (object_count==1)
                { if (*schema_name) AppendXpmItem(popup_menu, CPM_DATAEXP, MNST_DATAEXP, data_export_xpm);
                  if (*schema_name) AppendXpmItem(popup_menu, CPM_DATAIMP, MNST_DATAIMP, data_import_xpm);
                }
                AppendXpmItem(popup_menu, CPM_DATAPRIV, MNST_DATAPRIV, privil_dat_xpm);
              }
              else if (objnum<=USER_TABLENUM && object_count==1) // system table
                AppendXpmItem(popup_menu, CPM_DATAPRIV, _("Administrative privileges"));
              if (*schema_name) 
                AppendXpmItem(popup_menu, CPM_COMPACT, _("Compress Records"), compact_xpm);
              AppendXpmItem(popup_menu, CPM_FREEDEL, _("Release Deleted Records"));
              AppendXpmItem(popup_menu, CPM_REINDEX, _("Rebuild Indexes"), indexes_xpm);
              if (object_count==1)
              { AppendXpmItem(popup_menu, CPM_TABLESTAT, _("Table Statistics"));
                if (*schema_name) 
                { AppendXpmItem(popup_menu, CPM_CREATE_TRIGGER, _("Create Trigger"));
#ifdef NEVER // it is more efficient to the the privileges AFTER the command is selected                
                  if (!cd_Am_I_appl_author(cdp))
                    popup_menu->Enable(CPM_CREATE_TRIGGER, false);
#endif                    
                  AppendXpmItem(popup_menu, CPM_LIST_TRIGGERS, _("List Triggers"));
                }  
              }
            }
            break;
          case CATEG_CURSOR:
            if (object_count>=1)
            { AppendXpmItem(popup_menu, CPM_OPEN,   MNST_OPEN, table_sql_run_xpm);
              if (topcat==TOPCAT_ODBCDSN) break;
              AppendXpmItem(popup_menu, CPM_MODIFY, MNST_ALTER, modify_xpm);
              AppendXpmItem(popup_menu, CPM_EDIT_SOURCE, MNST_SRCEDIT, editSQL_xpm);
              AppendXpmItem(popup_menu, CPM_MODIFY_ALT, MNST_ALTER_SEPAR);
              popup_menu->Enable(CPM_MODIFY_ALT, global_style!=ST_AUINB);

            }
            if (topcat==TOPCAT_ODBCDSN) break;
            if (object_count>=1)
              AppendXpmItem(popup_menu, CPM_DELETE, MNST_DELETE, delete_xpm);
            if (object_count>=1)
            { popup_menu->AppendSeparator();
              AppendXpmItem(popup_menu, CPM_EXPORT, MNST_EXPORT, export_obj_xpm);
              AppendXpmItem(popup_menu, CPM_PRIVILS,MNST_PRIVILS, privil_obj_xpm);
              if (object_count==1)
              { AppendXpmItem(popup_menu, CPM_DATAEXP, MNST_DATAEXP, data_export_xpm);
                AppendXpmItem(popup_menu, CPM_OPTIM, _("Optimization"));
              }
            }
            break;
          case CATEG_VIEW:
            if (object_count>=1)
            { AppendXpmItem(popup_menu, CPM_MODIFY, MNST_ALTER, modify_xpm);
              AppendXpmItem(popup_menu, CPM_DELETE, MNST_DELETE, delete_xpm);
              popup_menu->AppendSeparator();
              AppendXpmItem(popup_menu, CPM_EXPORT, MNST_EXPORT, export_obj_xpm);
              AppendXpmItem(popup_menu, CPM_PRIVILS,MNST_PRIVILS, privil_obj_xpm);
            }
            break;
          case CATEG_PGMSRC:
            if (object_count==1)
            { AppendXpmItem(popup_menu, CPM_RUN,    MNST_RUN, run2_xpm);
              AppendXpmItem(popup_menu, CPM_MODIFY, MNST_ALTER, modify_xpm);
              if ((flags & CO_FLAGS_OBJECT_TYPE) == CO_FLAG_XML || (flags & CO_FLAGS_OBJECT_TYPE) == CO_FLAG_TRANSPORT) // CO_FLAG_TRANSPORT added ainly for debugging purposes
                AppendXpmItem(popup_menu, CPM_EDIT_SOURCE, MNST_SRCEDIT, editSQL_xpm);
#ifdef WINS
#ifdef XMLFORMS
              if ((flags & CO_FLAGS_OBJECT_TYPE) == CO_FLAG_XML && XMLFormDesignerPath() != wxEmptyString)
                AppendXpmItem(popup_menu, CPM_NEW_XMLFORM, _("Create new XML form"), xmlform_xpm);
#endif
#endif
            }
            if (object_count>=1)
              AppendXpmItem(popup_menu, CPM_DELETE, MNST_DELETE, delete_xpm);
            if (object_count>=1)
            { popup_menu->AppendSeparator();
              AppendXpmItem(popup_menu, CPM_EXPORT, MNST_EXPORT, export_obj_xpm);
              AppendXpmItem(popup_menu, CPM_PRIVILS,MNST_PRIVILS, privil_obj_xpm);
            }
            break;
          case CATEG_DOMAIN:  case CATEG_TRIGGER:  case CATEG_PROC:  case CATEG_SEQ:  
            if (object_count==1)
              if (category==CATEG_PROC)
                AppendXpmItem(popup_menu, CPM_RUN, _("Run"), run2_xpm);
            if (object_count>=1)
            { AppendXpmItem(popup_menu, CPM_MODIFY, MNST_ALTER, modify_xpm);
              if (category==CATEG_PROC || category==CATEG_TRIGGER)
              { AppendXpmItem(popup_menu, CPM_MODIFY_ALT, MNST_ALTER_SEPAR);
                popup_menu->Enable(CPM_MODIFY_ALT, global_style!=ST_AUINB);
              }
            }
            if (object_count>=1)
              AppendXpmItem(popup_menu, CPM_DELETE, MNST_DELETE, delete_xpm);
            if (object_count>=1)
            { popup_menu->AppendSeparator();
              AppendXpmItem(popup_menu, CPM_EXPORT, MNST_EXPORT, export_obj_xpm);
              AppendXpmItem(popup_menu, CPM_PRIVILS,MNST_PRIVILS, privil_obj_xpm);
              if (category==CATEG_SEQ)
              { popup_menu->AppendSeparator();
                AppendXpmItem(popup_menu, CPM_RESTART, _("Restart"));
              }
            }
            break;
          case CATEG_ROLE:
            if (object_count==1)
            { AppendXpmItem(popup_menu, CPM_SEL_USERS,  _("Select Users"), _user_xpm);
              AppendXpmItem(popup_menu, CPM_SEL_GROUPS, _("Select Groups"), _group_xpm);
              AppendXpmItem(popup_menu, CPM_LIST_USERS, _("List Users (transitive)"));
            }
            if (object_count>=1)
              AppendXpmItem(popup_menu, CPM_DELETE, MNST_DELETE, delete_xpm);
            break;
          case CATEG_USER:
            if (object_count==1)
              AppendXpmItem(popup_menu, CPM_MODIFY, _("&Properties"), modify_xpm);
              AppendXpmItem(popup_menu, CPM_SEL_GROUPS, _("Select &Groups"), _group_xpm);
            if (object_count>=1)
              AppendXpmItem(popup_menu, CPM_DELETE, MNST_DELETE, delete_xpm);
#ifdef STOP  // account management is in the properties dialog
            if (object_count>=1)
            { popup_menu->AppendSeparator();
              popup_menu->Append(CPM_ENABLE_ACCOUNT,  _("Enable account"));
              popup_menu->Append(CPM_DISABLE_ACCOUNT, _("Disable account"));
            }
#endif
            break;
          case CATEG_GROUP:
            if (object_count==1)
            { AppendXpmItem(popup_menu, CPM_SEL_USERS,  _("Select Users"), _user_xpm);
            }
            if (object_count>=1)
              AppendXpmItem(popup_menu, CPM_DELETE, MNST_DELETE, delete_xpm);
            break;
            
          case CATEG_INFO:
            if (object_count==1)
              AppendXpmItem(popup_menu, CPM_RUN,    MNST_RUN, run2_xpm);
            // cont.  
          case CATEG_DRAWING:      
            if (object_count>=1)
              AppendXpmItem(popup_menu, CPM_MODIFY, MNST_ALTER, modify_xpm);
            if (object_count>=1)
            { AppendXpmItem(popup_menu, CPM_DELETE, MNST_DELETE, delete_xpm);
              popup_menu->AppendSeparator();
              AppendXpmItem(popup_menu, CPM_EXPORT, MNST_EXPORT, export_obj_xpm);
              AppendXpmItem(popup_menu, CPM_PRIVILS,MNST_PRIVILS, privil_obj_xpm);
            }
            break;
#ifdef XMLFORMS
#ifdef WINS
          case CATEG_XMLFORM:
            if (object_count==1)
            { if (XMLFormFillerPath() != wxEmptyString)
                AppendXpmItem(popup_menu, CPM_OPEN, MNST_OPEN, xmlfill_xpm);
            }
            if (object_count>=1)
              AppendXpmItem(popup_menu, CPM_ALT_OPEN, MNST_ALT_OPEN, explorer_xpm);
            if (object_count==1)
            { if (XMLFormDesignerPath() != wxEmptyString)
                AppendXpmItem(popup_menu, CPM_MODIFY, MNST_ALTER, xmldsg_xpm);
              AppendXpmItem(popup_menu, CPM_EDIT_SOURCE, MNST_SRCEDIT, editSQL_xpm);
            }
            if (object_count>=1)
            { AppendXpmItem(popup_menu, CPM_DELETE, MNST_DELETE, delete_xpm);
              popup_menu->AppendSeparator();
              AppendXpmItem(popup_menu, CPM_EXPORT, MNST_EXPORT,  export_obj_xpm);
              AppendXpmItem(popup_menu, CPM_PRIVILS,MNST_PRIVILS, privil_obj_xpm);
            }
            break;
#endif  // WINS
          case CATEG_STSH:
            if (object_count==1)
            { AppendXpmItem(popup_menu, CPM_MODIFY,   MNST_ALTER,   modify_xpm);
              if (get_stsh_editor_path((flags & CO_FLAGS_OBJECT_TYPE) == CO_FLAG_STS_CASC).IsEmpty())
                popup_menu->Enable(CPM_MODIFY, false);
            }
            if (object_count>=1)
            { AppendXpmItem(popup_menu, CPM_EDIT_SOURCE,MNST_SRCEDIT, editSQL_xpm);
              AppendXpmItem(popup_menu, CPM_MODIFY_ALT, MNST_ALTER_SEPAR);
              popup_menu->Enable(CPM_MODIFY_ALT, global_style!=ST_AUINB);
              AppendXpmItem(popup_menu, CPM_DELETE,     MNST_DELETE,  delete_xpm);
              popup_menu->AppendSeparator();
              AppendXpmItem(popup_menu, CPM_EXPORT,     MNST_EXPORT,  export_obj_xpm);
              AppendXpmItem(popup_menu, CPM_PRIVILS,    MNST_PRIVILS, privil_obj_xpm);
            }
            break;
#endif // XMLFORMS
        }
        if (topcat!=TOPCAT_ODBCDSN)
        { if (/*!formain &&*/ *schema_name)
          { popup_menu->AppendSeparator();
            if (object_count)
            { if (category != CATEG_TABLE)
                AppendXpmItem(popup_menu, CPM_CUT,  _("Cut object(s)"),  cut_xpm, /* category!=CATEG_INFO && */ CanCopy());
              AppendXpmItem(popup_menu, CPM_COPY, _("Copy object(s)"), Kopirovat_xpm, /* category!=CATEG_INFO && */ CanCopy());
            }
            if (object_count == 1 && IsFolder())
              AppendXpmItem(popup_menu, CPM_PASTE, _("Paste object(s)"), Vlozit_xpm, can_paste && category!=CATEG_INFO);
            if (object_count)
              AppendXpmItem(popup_menu, CPM_DUPLICATE, _("Duplicate object(s)"), empty_xpm, category!=CATEG_INFO && category!=CATEG_ROLE && CanCopy());
          }
#ifndef CONTROL_PANEL_INPLECE_EDIT
          if (object_count == 1)
          {
              bool Renamable = true;
              if 
              (
                  // Systemova nebo ODBC tabulka
                  (IsObject(CATEG_TABLE) && (objnum <= REL_TABLENUM || (flags & CO_FLAG_ODBC))) ||
                  // Anonymous nebo systemova skupina
                  ((IsObject(CATEG_USER) || IsObject(CATEG_GROUP)) && objnum < FIXED_USER_TABLE_OBJECTS) ||
                  // ODBC connection, replikacni server, fulltext
                  (IsObject(CATEG_CONNECTION) || IsObject(CATEG_SERVER) || IsObject(CATEG_INFO) || IsObject(CATEG_ROLE)
                  /*|| IsObject(CATEG_TRIGGER) || IsObject(CATEG_DOMAIN)*/) ||
                  // Zasifrovany objekt
                  (flags & CO_FLAG_ENCRYPTED)
              )
                  Renamable = false;
              tobjnum lobjnum = objnum;
              if (Renamable && (flags & CO_FLAG_LINK) && cd_Find_prefixed_object(cdp, schema_name, object_name,  category | IS_LINK, &lobjnum))
                  Renamable = false;
              if (Renamable && !check_object_access(cdp, category, lobjnum, 1))
                  Renamable = false;
              if (Renamable)
                  AppendXpmItem(popup_menu, CPM_RENAME, _("Rename object"));
          }
#endif
        }
      }
      else if ((syscat==SYSCAT_APPLTOP || syscat==SYSCAT_CATEGORY || syscat==SYSCAT_OBJECT) && cdp->odbc.apl_folders.count() > 1)
        AppendFolderItems(popup_menu);
      break;
    case TOPCAT_CLIENT:
        if (cti == IND_MAIL)
            AppendXpmItem(popup_menu, CPM_NEW, MNST_NEWPROF, profil_new_xpm);
        else if (cti == IND_MAILPROF)
        {
            AppendXpmItem(popup_menu, CPM_MODIFY, MNST_ALTER,  profil_edit_xpm);
            AppendXpmItem(popup_menu, CPM_DELETE, MNST_DELETE, profil_delete_xpm);
        }
        break;
  } // switch topcat
}

wxDragResult item_info::CanPaste(const C602SQLObjects *objs, bool DragDrop)
{
    if (!cdp || !objs)        
        return(wxDragNone); 

    //wxLogDebug("CanPaste SrcFolder = %s DstFolder = %s", objs->m_Folder, info.folder_name);
    // IF Item je aplikace
    if (IsSchema())
    {
        //wxLogDebug("    Schema = %s", info.schema_name);
		// IF objekty z jine aplikace
		if (wb_stricmp(cdp, objs->m_Schema, schema_name))
        {
            if (objs->ContainsOnly(CATEG_INFO))
                return wxDragNone;
			return(wxDragCopy);
        }
		// IF objekty nejsou z ROOTu
		if (*objs->m_Folder)
			return(wxDragMove);
    }
    // ELSE IF hItem je folder
    else if (IsFolder())
    {
        //wxLogDebug("    Folder = %s", info.folder_name);
        // IF Objekty, ktere jsou jen v rootu (Obrazky, Connection, ...)
        if (objs->RootOnlyObjects())
			return(wxDragNone);
        // IF jina aplikace
        if (wb_stricmp(cdp, objs->m_Schema, schema_name))
        {
            if (objs->ContainsOnly(CATEG_INFO))
                return wxDragNone;
			return wxDragCopy;
        }
        // IF Cilovy folder neni potomkem zdrojoveho, Pastovat muzeme i do stejneho
        if (!objs->ContainsFolder(cdp, folder_name, DragDrop))
        	return(wxDragMove);
    }
    else if (IsCategory())
    {
        //wxLogDebug("    Categ = %s", CCategList::CaptFirst(info.category));

        if (objs->Contains(category))
        {
            // IF jina aplikace
            if (wb_stricmp(cdp, objs->m_Schema, schema_name))
            {
                if (objs->ContainsOnly(CATEG_INFO))
                    return wxDragNone;
    			return(wxDragCopy);
            }
            // IF Cilovy folder neni potomkem zdrojoveho, Pastovat muzeme i do stejneho
            if (!objs->ContainsFolder(cdp, folder_name, DragDrop))
        	    return(wxDragMove);
        }
    }
    return(wxDragNone);
}

wxImageList * make_image_list(void)  // exactly as in tcpitemid
{ wxImageList * il = new wxImageList(16, 16, TRUE, 95);
  { wxBitmap bmp(_sql_top_xpm); il->Add(bmp); }  // IND_SERVERS
  { wxBitmap bmp(_client_xpm);  il->Add(bmp); }  // IND_CLIENT
  { wxBitmap bmp(odbcs_xpm);    il->Add(bmp); }  // IND_ODBCDSNS

  { wxBitmap bmp(_srvl_xpm);    il->Add(bmp); }  // IND_SRVLOCIDL
  { wxBitmap bmp(_srvlr_xpm);   il->Add(bmp); }  // IND_SRVLOCRUN
  { wxBitmap bmp(_srvlrc_xpm);  il->Add(bmp); }  // IND_SRVLOCCON
  { wxBitmap bmp(_srvn_xpm);    il->Add(bmp); }  // IND_SRVREMIDL
  { wxBitmap bmp(_srvnr_xpm);   il->Add(bmp); }  // IND_SRVREMRUN
  { wxBitmap bmp(_srvnrc_xpm);  il->Add(bmp); }  // IND_SRVREMCON
  { wxBitmap bmp(_srvq_xpm);    il->Add(bmp); }  // IND_SRVREMUNK
  { wxBitmap bmp(odbc_nonactive_xpm);    il->Add(bmp); }  // IND_SRVODBC
  { wxBitmap bmp(odbc_active_xpm);    il->Add(bmp); }  // IND_SRVODBCCON

  { wxBitmap bmp(_apl_xpm);     il->Add(bmp); }  // IND_APPL
  { wxBitmap bmp(_table_xpm);   il->Add(bmp); }  // IND_TABLE
  { wxBitmap bmp(_repltab_xpm); il->Add(bmp); }  // IND_REPLTAB
  { wxBitmap bmp(_link_xpm);    il->Add(bmp); }  // IND_LINK
  { wxBitmap bmp(_conn_xpm);    il->Add(bmp); }  // IND_ODBC

  { wxBitmap bmp(source_xpm);   il->Add(bmp); }  // IND_VIEW  temp. image##
/*  { wxBitmap bmp(_formsmpl_xpm);il->Add(bmp); }  // IND_STVIEW
  { wxBitmap bmp(_label_xpm);   il->Add(bmp); }  // IND_LABEL
  { wxBitmap bmp(_graph_xpm);   il->Add(bmp); }  // IND_GRAPH
  { wxBitmap bmp(_report_xpm);  il->Add(bmp); }  // IND_REPORT
  { wxBitmap bmp(_menu_xpm);    il->Add(bmp); }  // IND_MENU
  { wxBitmap bmp(_popup_xpm);   il->Add(bmp); }  // IND_POPUP
*/
  { wxBitmap bmp(_query_xpm);   il->Add(bmp); }  // IND_CURSOR
  { wxBitmap bmp(_program_xpm); il->Add(bmp); }  // IND_PROGR
  { wxBitmap bmp(source_xpm);   il->Add(bmp); }  // IND_INCL
  { wxBitmap bmp(_transp_xpm);  il->Add(bmp); }  // IND_TRANSP
  { wxBitmap bmp(_xml_xpm);     il->Add(bmp); }  // IND_XML
  { wxBitmap bmp(_pict_xpm);    il->Add(bmp); }  // IND_PICT
  { wxBitmap bmp(_draw_xpm);    il->Add(bmp); }  // IND_DRAW
  { wxBitmap bmp(_relat_xpm);   il->Add(bmp); }  // IND_RELAT
  { wxBitmap bmp(_role_xpm);    il->Add(bmp); }  // IND_ROLE
  { wxBitmap bmp(_conn_xpm);    il->Add(bmp); }  // IND_CONN
//  { wxBitmap bmp(_badconn_xpm); il->Add(bmp); }  // IND_BADCONN
  { wxBitmap bmp(_replsrv_xpm); il->Add(bmp); }  // IND_REPLREL
  { wxBitmap bmp(_proc_xpm);    il->Add(bmp); }  // IND_PROC
  { wxBitmap bmp(_trigger_xpm); il->Add(bmp); }  // IND_TRIGGER
//  { wxBitmap bmp(_draw_xpm);    il->Add(bmp); }  // IND_WWWTEMPL
//  { wxBitmap bmp(_draw_xpm);    il->Add(bmp); }  // IND_WWWCONN
//  { wxBitmap bmp(_draw_xpm);    il->Add(bmp); }  // IND_WWWSEL
  { wxBitmap bmp(_seq_xpm);     il->Add(bmp); }  // IND_SEQ
  { wxBitmap bmp(_domain_xpm);  il->Add(bmp); }  // IND_DOM
  { wxBitmap bmp(fulltext_xpm);    il->Add(bmp); }  // IND_FULLTEXT
  { wxBitmap bmp(fulltext_xpm);    il->Add(bmp); }  // IND_FULLTEXT_SEP
  { wxBitmap bmp(_user_xpm);    il->Add(bmp); }  // IND_USER
  { wxBitmap bmp(_group_xpm);   il->Add(bmp); }  // IND_GROUP
  { wxBitmap bmp(_systable_xpm);il->Add(bmp); }  // IND_SYSTABLE
  { wxBitmap bmp(_replsrv_xpm); il->Add(bmp); }  // IND_REPLSRV
  { wxBitmap bmp(_folder_xpm);  il->Add(bmp); }  // IND_FOLDER
  { wxBitmap bmp(_folder_xpm);  il->Add(bmp); }  // IND_UPFOLDER
  { wxBitmap bmp(_folder_xpm);  il->Add(bmp); }  // IND_SYSTEM_FOLDER
  { wxBitmap bmp(_folder_xpm);  il->Add(bmp); }  // IND_FOLDER_MONITOR
  { wxBitmap bmp(_folder_sys_xpm);    il->Add(bmp); }  // IND_FOLDER_SYSOBJS
  { wxBitmap bmp(_folder_params_xpm); il->Add(bmp); }  // IND_FOLDER_PARAMS
  { wxBitmap bmp(_folder_tools_xpm);  il->Add(bmp); }  // IND_FOLDER_TOOLS
  { wxBitmap bmp(licences_xpm); il->Add(bmp); }  // IND_LICENCES
  { wxBitmap bmp(clients_xpm);  il->Add(bmp); }  // IND_CLIENTS
  { wxBitmap bmp(locks_xpm);    il->Add(bmp); }  // IND_LOCKS
  { wxBitmap bmp(trace_xpm);    il->Add(bmp); }  // IND_TRACE
  { wxBitmap bmp(logs_xpm);     il->Add(bmp); }  // IND_LOGS
  { wxBitmap bmp(alog_xpm);     il->Add(bmp); }  // IND_ALOG
  { wxBitmap bmp(extinfo_xpm);  il->Add(bmp); }  // IND_EXTINFO
  { wxBitmap bmp(runprop_xpm);  il->Add(bmp); }  // IND_RUNPROP
  { wxBitmap bmp(protocols_xpm);il->Add(bmp); }  // IND_PROTOCOLS
  { wxBitmap bmp(network_xpm);  il->Add(bmp); }  // IND_NETWORK
  { wxBitmap bmp(export_users_xpm); il->Add(bmp); }  // IND_EXP_USERS
  { wxBitmap bmp(security_xpm); il->Add(bmp); }  // IND_SECURITY
  { wxBitmap bmp(filenc_xpm);   il->Add(bmp); }  // IND_FILENC
  { wxBitmap bmp(import_users_xpm);  il->Add(bmp); }  // IND_IMP_USERS
  { wxBitmap bmp(srvcert_xpm);  il->Add(bmp); }  // IND_SRVCERT
  { wxBitmap bmp(backup_xpm);   il->Add(bmp); }  // IND_BACKUP
  { wxBitmap bmp(replay_xpm);   il->Add(bmp); }  // IND_REPLAY
  { wxBitmap bmp(consist_xpm);  il->Add(bmp); }  // IND_CONSISTENCY
  { wxBitmap bmp(compact_xpm);  il->Add(bmp); }  // IND_COMPACT
  { wxBitmap bmp(server_rename_xpm); il->Add(bmp); }  // IND_SERV_RENAME
  { wxBitmap bmp(message_xpm);  il->Add(bmp); }  // IND_MESSAGE
  { wxBitmap bmp(profile_xpm);  il->Add(bmp); }  // IND_PROFILER
  { wxBitmap bmp(http_xpm);     il->Add(bmp); }  // IND_HTTP
  { wxBitmap bmp(fonts_xpm);    il->Add(bmp); }  // IND_FONTS
  { wxBitmap bmp(grid_format_xpm); il->Add(bmp); }  // IND_FORMATS
  { wxBitmap bmp(profils_xpm);     il->Add(bmp); }  // IND_MAIL
  { wxBitmap bmp(profil_mail_xpm); il->Add(bmp); }  // IND_MAILPROF
  { wxBitmap bmp(locales_xpm);  il->Add(bmp); }  // IND_LOCALE
  { wxBitmap bmp(fulltext_set_xpm); il->Add(bmp); }  // IND_FTXHELPERS
  { wxBitmap bmp(cliconn_xpm);  il->Add(bmp); }  // IND_COMM
  { wxBitmap bmp(sts_pref_xpm);  il->Add(bmp); }  // IND_EDITORS
  { wxBitmap bmp(validateSQL_xpm); il->Add(bmp); }  // IND_SYNTAX
  { wxBitmap bmp(data_find_xpm);   il->Add(bmp); }  // IND_SEARCH
  { wxBitmap bmp(tableProperties_xpm);   il->Add(bmp); }  // IND_PROPS
  { wxBitmap bmp(sql_console_xpm);  il->Add(bmp); }  // IND_SQL_CONSOLE
  { wxBitmap bmp(xmlform_xpm);  il->Add(bmp); }  // IND_XMLFORM
  { wxBitmap bmp(stylesheet_xpm);  il->Add(bmp); }  // IND_STSH
  { wxBitmap bmp(stylesheet_xpm);    il->Add(bmp); }  // IND_STSHCASC
  return il;
}

#if WBVERS>=1000
class t_cp_toolbar : public wxToolBar
{ 
 public:
  t_cp_toolbar(wxWindow * parent, int id) : wxToolBar(parent, id, wxDefaultPosition, wxDefaultSize, wxTB_HORIZONTAL | wxNO_BORDER | wxTB_NODIVIDER | wxTB_FLAT)
    { }
  void OnTool(wxCommandEvent & event);
  DECLARE_EVENT_TABLE()
};

void t_cp_toolbar::OnTool(wxCommandEvent & event)
{
  wxGetApp().frame->OnCPCommand(event);
}

BEGIN_EVENT_TABLE( t_cp_toolbar, wxToolBar )
    EVT_TOOL(-1, t_cp_toolbar::OnTool)
END_EVENT_TABLE()

#endif

wxWindow * create_control_panel(MyFrame * parent)
{
#if WBVERS>=1000
  wxPanel * top_panel = new wxPanel(parent, -1, wxDefaultPosition, wxDefaultSize, 0);
  top_panel->SetSizer(new wxBoxSizer(wxVERTICAL));
  t_cp_toolbar * cp_toolbar = new t_cp_toolbar(top_panel, 1);
  cp_toolbar->AddTool( CPM_CUT,    wxEmptyString, wxBitmap(cut_xpm),       _("Cut object(s)") );
  cp_toolbar->AddTool( CPM_COPY,   wxEmptyString, wxBitmap(Kopirovat_xpm), _("Copy object(s)") );
  cp_toolbar->AddTool( CPM_PASTE,  wxEmptyString, wxBitmap(Vlozit_xpm),    _("Paste object(s)") );
  cp_toolbar->AddTool( CPM_REFRPANEL, wxEmptyString, wxBitmap(refrpanel_xpm), _("Refresh control panel") );
  cp_toolbar->AddSeparator();
  cp_toolbar->AddTool( CPM_NEW,    wxEmptyString, wxBitmap(new_xpm),    _("New object") );   // "object" - because the tool works with CP objects only
  cp_toolbar->AddTool( CPM_MODIFY, wxEmptyString, wxBitmap(modify_xpm), _("Modify object") );
  cp_toolbar->AddTool( CPM_DELETE, wxEmptyString, wxBitmap(delete_xpm), _("Delete object(s)") );
  cp_toolbar->Realize();
  top_panel->GetSizer()->Add(cp_toolbar, 0, wxGROW | wxBOTTOM, 4);  // exblicit border on the bottom
#endif
  wxCPSplitterWindow * splitter1 = new wxCPSplitterWindow;
  splitter1->ShowFolders         = read_profile_bool("Look", "Show folders", true);
  splitter1->ShowEmptyFolders    = read_profile_bool("Look", "Show empty folders", false);
  splitter1->fld_bel_cat         = read_profile_bool("Look", "Folders below categories", true);
  splitter1->frame=parent;
#if WBVERS>=1000
  splitter1->Create(top_panel, 2, wxPoint(0, 0), wxSize(200, 200), wxSP_3D, wxT("control_panel_splitter"));
  top_panel->GetSizer()->Add(splitter1, 1, wxGROW, 0);
  top_panel->Layout();
#else
  splitter1->Create(parent, -1, wxPoint(0, 0), wxSize(200, 200), wxSP_3D, wxT("control_panel_splitter"));
#endif

  ControlPanelTree * cpt = new ControlPanelTree(splitter1);
#ifdef CONTROL_PANEL_INPLECE_EDIT
  cpt->Create(splitter1, TREE_ID, wxDefaultPosition, wxDefaultSize, wxTR_EDIT_LABELS | wxTR_HAS_BUTTONS | wxTR_HIDE_ROOT | wxTR_LINES_AT_ROOT | wxWANTS_CHARS | wxSUNKEN_BORDER, wxDefaultValidator, wxT("control_panel_tree"));
#else
  cpt->Create(splitter1, TREE_ID, wxDefaultPosition, wxDefaultSize, wxTR_HAS_BUTTONS | wxTR_HIDE_ROOT | wxTR_LINES_AT_ROOT | wxWANTS_CHARS | wxSUNKEN_BORDER, wxDefaultValidator, wxT("control_panel_tree"));
#endif
  // wxSUNKEN_BORDER looks somewhat strange on old Windows but wxSIMPLE_BORDER look strange on old and new Windows and NO_BORDER makes impossible to locate the sash on new (XP) Windows.
  cpt->SetImageList(parent->main_image_list);  // does not take the ownership
#ifdef LINUX
  cpt->SetOwnBackgroundColour(parent->GetDefaultAttributes().colBg/*wxColour(255,255,255)*/);   // disables the ugly gray backgroud in some themes
#endif
  cpt->SetName(CPTreeName);
  cpt->SetDropTarget(new CCPTreeDropTarget(cpt));
  cpt->insert_top_items();

  ControlPanelList::LoadLayout();
  ControlPanelList * cpl = new ControlPanelList(splitter1, cpt);
#ifdef CONTROL_PANEL_INPLECE_EDIT
  cpl->Create(splitter1, LIST_ID, wxDefaultPosition, wxDefaultSize, wxLC_REPORT | wxLC_EDIT_LABELS | wxWANTS_CHARS | wxSUNKEN_BORDER, wxDefaultValidator, wxT("control_panel_list"));
#else
  cpl->Create(splitter1, LIST_ID, wxDefaultPosition, wxDefaultSize, wxLC_REPORT | wxWANTS_CHARS | wxSUNKEN_BORDER, wxDefaultValidator, wxT("control_panel_list"));
#endif
#ifdef LINUX
  cpl->SetOwnBackgroundColour(parent->GetDefaultAttributes().colBg/*wxColour(255,255,255)*/);   // disables the ugly gray backgroud in some themes
#endif
  cpl->SetImageList(parent->main_image_list, wxIMAGE_LIST_SMALL);  // takes ownership
  cpl->SetName(CPListName);
  cpl->SetDropTarget(new CCPListDropTarget(cpl));
  if (control_panel_font)
  { cpl->SetFont(*control_panel_font);
    cpt->SetFont(*control_panel_font);
  }
 // coupling the tree view and the list view:
  splitter1->objlist   = cpt->objlist = cpl;
  splitter1->tree_view = cpt;
  splitter1->SplitHorizontally(cpt, cpl);
 // Set this to prevent unsplitting
  splitter1->SetMinimumPaneSize(14);
  SetControlPanelInterf(splitter1);
  parent->control_panel = splitter1;
#if WBVERS>=1000
  parent->MainToolBar = cp_toolbar;
  return top_panel;
#else
  return splitter1;
#endif
}

int item_info::get_folder_index()
{// return index of the current folder
  int current_folder = 0;  // root folder index
  if (*folder_name)
    for (current_folder = 1;  current_folder < cdp->odbc.apl_folders.count();  current_folder++)
      if (!wb_stricmp(cdp, cdp->odbc.apl_folders.acc0(current_folder)->name, folder_name)) break;
  return current_folder;
}

void insert_cp_item(item_info & info, uns32 modtime, bool select_it)
// First, tries to insert into the list and select it.
// Then, inserts to the tree. If inserted and selected, replaces the contents of the list.
{ wxCPSplitterWindow * control_panel = wxGetApp().frame->control_panel;
  control_panel->AddObject(info.server_name, info.schema_name, info.folder_name, info.object_name, info.category, info.flags, modtime, select_it);
}

void update_cp_item(item_info & info, uns32 modtime)
{ wxCPSplitterWindow * control_panel = wxGetApp().frame->control_panel;
 // update in the list:
  control_panel->objlist->update_cp_item(info, modtime);
 // update in the tree:
  if ((info.topcat==TOPCAT_SERVERS || info.topcat==TOPCAT_ODBCDSN) && info.syscat==SYSCAT_NONE)
  { wxTreeItemId serv_item = control_panel->tree_view->FindServer(info.server_name_str());
    if (serv_item.IsOk()) control_panel->RefreshItem(serv_item, !control_panel->tree_view->IsChild(control_panel->tree_view->GetSelItem(), serv_item));
  }
}


#ifdef STOP  // replaced by selecting inside inserting a new item
void select_cp_item(item_info & info)
{ wxCPSplitterWindow * control_panel = wxGetApp().frame->control_panel;
  control_panel->objlist->select_cp_item(info);
}
#endif

wxString syslang_name(int sys_charset)
{ int lang = sys_charset & 0x7f;
  wxString s;
  switch (lang)
  { case 0:  s=_("English");       break;
    case 1:  s=_("Czech+Slovak");  break;
    case 2:  s=_("Polish");        break;
    case 3:  s=_("French");        break;
    case 4:  s=_("German");        break;
    case 5:  s=_("Italian");       break;
    default: s=_("Unknown");       break; 
  }
  if (lang)
    if ((sys_charset & 0x80)!=0)
      s=s+wxT(" ")+_("(ISO encoding)");
    else
      s=s+wxT(" ")+_("(MSW code page)");
  return s;
}

void show_source(item_info & info, wxTextCtrl * source_window)
// Dispays the source of the selected object to the [source_window].
{ wxBusyCursor wait;
  if ((info.topcat==TOPCAT_SERVERS || info.topcat==TOPCAT_ODBCDSN) && info.syscat==SYSCAT_APPLS)
  { wxString descr;  
    if (info.cdp)
    { uns32 vers1=0, vers2=0, vers3=0, vers4=0, platform=PLATFORM_WINDOWS;
      tobjname internal_server_name = "";
      if (!cd_Get_server_info(info.cdp, OP_GI_VERSION_1,     &vers1,                sizeof(vers1)) &&
          !cd_Get_server_info(info.cdp, OP_GI_VERSION_2,     &vers2,                sizeof(vers2)) &&
          !cd_Get_server_info(info.cdp, OP_GI_VERSION_3,     &vers3,                sizeof(vers3)) &&
          !cd_Get_server_info(info.cdp, OP_GI_VERSION_4,     &vers4,                sizeof(vers4)) &&
          !cd_Get_server_info(info.cdp, OP_GI_SERVER_NAME,   &internal_server_name, sizeof(internal_server_name)) &&
          !cd_Get_server_info(info.cdp, OP_GI_SERVER_PLATFORM, &platform, sizeof(platform)))
        descr.Printf(_("602SQL server %s, version %d.%d.%02d.%04d\nRunning on %s\nSystem language is %s"), 
          wxString(internal_server_name, *wxConvCurrent).c_str(), 
          vers1, vers2, vers3, vers4, 
          platform==PLATFORM_WINDOWS ? _("Microsoft Windows") : platform==PLATFORM_WINDOWS64 ? _("Microsoft Windows 64") : _("Linux"),
          syslang_name(info.cdp->sys_charset).c_str());
      t_avail_server * as = find_server_connection_info(info.server_name);
      if (as)
      { if (as->notreg) descr=descr+_("\nTemporary name for the unregistered server:");
        else            descr=descr+_("\nServer is locally registered as:");
        descr=descr + wxT(" ") + wxString(info.cdp->locid_server_name, *wxConvCurrent);
      }
      uns32 blocksize;
      if (!cd_Get_server_info(info.cdp, OP_GI_CLUSTER_SIZE, &blocksize, sizeof(blocksize)))
      { wxString msg;  
        msg.Printf(_("\nCluster size in the database file is %u bytes."), blocksize);
        descr=descr + msg;
      }  
    }
    else if (info.conn)
    { descr.Printf(_("ODBC Data Source %s\nDatabase Management System: %s, version %s"),
        wxString(info.conn->dsn, *wxConvCurrent).c_str(), 
        wxString(info.conn->dbms_name, *wxConvCurrent).c_str(), 
        wxString(info.conn->dbms_ver, *wxConvCurrent).c_str());
      if (*info.conn->database_name)
        descr=descr+_("\nDatabase name:")+wxT(" ")+wxString(info.conn->database_name, *wxConvCurrent).c_str();
      char user_name[100];
      if (SQL_SUCCEEDED(SQLGetInfoA(info.conn->hDbc, SQL_USER_NAME, user_name, sizeof(user_name), NULL)) && *user_name)
        descr=descr+_("\nLogged in as:")+wxT(" ")+wxString(user_name, *wxConvCurrent).c_str();
    }
    source_window->SetValue(descr);  // will be empty if server is not connected or if the connection is broken
  }
  else if (!info.cdp)  // ODBC
    source_window->SetValue(wxEmptyString);  
  else if (info.syscat==SYSCAT_APPLTOP)
  { wxString descr;  tobjname aplname;  WBUUID uuid;  char buf[2*UUID_SIZE+1];  apx_header apx;  
    cd_Read(info.cdp, OBJ_TABLENUM, info.objnum, OBJ_NAME_ATR, NULL, aplname);
    cd_Read(info.cdp, OBJ_TABLENUM, info.objnum, APPL_ID_ATR,  NULL, uuid);
    bin2hex(buf, uuid, UUID_SIZE);  buf[2*UUID_SIZE]=0;
    memset(&apx, 0, sizeof(apx));
    if (!cd_Read_var(info.cdp, OBJ_TABLENUM, info.objnum, OBJ_DEF_ATR, NOINDEX, 0, sizeof(apx_header), &apx, NULL))  // unless connection lost
      descr.Printf(_("Application %s:\nID = %s\n%s"), wxString(aplname, *info.cdp->sys_conv).c_str(), 
        UUID2str(uuid).c_str(),  
        apx.version>=6 && apx.appl_locked ?
          _("The application is locked and cannot be changed.") :
          _("The application is not locked and may be changed."));
    source_window->SetValue(descr);
  }
  else if (info.category==CATEG_ROLE)
  { char query[200];  wxString msg, list;  tcursnum cursnum;  
    sprintf(query, "SELECT subject_name, subject_categ FROM _iv_subject_membership WHERE container_categ=CATEG_ROLE and container_objnum=%u", info.objnum);
    if (!cd_Open_cursor_direct(info.cdp, query, &cursnum))
    { msg.Printf(_("Casting of role %s.%s:\n"), info.schema_name_str().c_str(), info.object_name_str().c_str());
      list=msg;
      trecnum cnt, r;  tobjname subjname;  uns16 subjcateg;
      cd_Rec_cnt(info.cdp, cursnum, &cnt);
      for (r=0;  r<cnt;  r++)
      { cd_Read(info.cdp, cursnum, r, 1, NULL, subjname);
        cd_Read(info.cdp, cursnum, r, 2, NULL, &subjcateg);
        msg.Printf(wxT("%s %s\n"), subjcateg==CATEG_USER ? _("User") : _("Group"), wxString(subjname, *info.cdp->sys_conv).c_str());
        list=list+msg;
      }
      cd_Close_cursor(info.cdp, cursnum);
    }
    source_window->SetValue(list);
  }
  else if (info.category==CATEG_USER)
  { char query[200];  wxString list, msg;  tcursnum cursnum;  
    sprintf(query, "SELECT container_name, container_schema, container_categ FROM _iv_subject_membership WHERE subject_categ=CATEG_USER and subject_objnum=%u", info.objnum);
    if (!cd_Open_cursor_direct(info.cdp, query, &cursnum))
    { msg.Printf(_("Groups and roles for user %s:\n"), info.object_name_str().c_str());
      list=msg;
      trecnum cnt, r;  tobjname name, schema;  uns16 categ;
      cd_Rec_cnt(info.cdp, cursnum, &cnt);
      for (r=0;  r<cnt;  r++)
      { cd_Read(info.cdp, cursnum, r, 1, NULL, name);
        cd_Read(info.cdp, cursnum, r, 3, NULL, &categ);
        if (categ==CATEG_ROLE)
        { cd_Read(info.cdp, cursnum, r, 2, NULL, schema);
          msg.Printf(wxT("%s %s.%s\n"), _("Role"), wxString(schema, *info.cdp->sys_conv).c_str(), wxString(name, *info.cdp->sys_conv).c_str());
        }
        else
          msg.Printf(wxT("%s %s\n"), _("Group"), wxString(name, *info.cdp->sys_conv).c_str());
        list=list+msg;
      }
      cd_Close_cursor(info.cdp, cursnum);
    }
    source_window->SetValue(list);
  }
  else if (info.category==CATEG_GROUP)
  { char query[200];  wxString list, msg;  tcursnum cursnum;  
    sprintf(query, "SELECT subject_name, subject_categ FROM _iv_subject_membership WHERE container_categ=CATEG_GROUP and container_objnum=%u", info.objnum);
    if (!cd_Open_cursor_direct(info.cdp, query, &cursnum))
    { msg.Printf(_("Members of group %s:\n"), info.object_name_str().c_str());
      list=msg;
      trecnum cnt, r;  tobjname subjname;  uns16 subjcateg;
      cd_Rec_cnt(info.cdp, cursnum, &cnt);
      for (r=0;  r<cnt;  r++)
      { cd_Read(info.cdp, cursnum, r, 1, NULL, subjname);
        cd_Read(info.cdp, cursnum, r, 2, NULL, &subjcateg);
        msg.Printf(wxT("%s %s\n"), subjcateg==CATEG_USER ? _("User") : _("Group"), wxString(subjname, *info.cdp->sys_conv).c_str());
        list=list+msg;
      }
      cd_Close_cursor(info.cdp, cursnum);
    }
    sprintf(query, "SELECT container_name, container_schema, container_categ FROM _iv_subject_membership WHERE subject_categ=CATEG_GROUP and subject_objnum=%u", info.objnum);
    if (!cd_Open_cursor_direct(info.cdp, query, &cursnum))
    { msg.Printf(_("Roles of group %s:\n"), info.object_name_str().c_str());
      list=list+msg;
      trecnum cnt, r;  tobjname name, schema;  uns16 categ;
      cd_Rec_cnt(info.cdp, cursnum, &cnt);
      for (r=0;  r<cnt;  r++)
      { cd_Read(info.cdp, cursnum, r, 1, NULL, name);
        cd_Read(info.cdp, cursnum, r, 3, NULL, &categ);
        if (categ==CATEG_ROLE)
        { cd_Read(info.cdp, cursnum, r, 2, NULL, schema);
          msg.Printf(wxT("%s %s.%s\n"), _("Role"), wxString(schema, *info.cdp->sys_conv).c_str(), wxString(name, *info.cdp->sys_conv).c_str());
        }
        list=list+msg;
      }
      cd_Close_cursor(info.cdp, cursnum);
    }
    source_window->SetValue(list.c_str());
  }
  else if (info.category==CATEG_XMLFORM)
  { wxString src;
#ifdef WINS
#ifdef XMLFORMS
    tobjname DAD;  *DAD=0;
    if (!link_GetDadFromXMLForm(info.cdp, info.objnum, DAD, false) && *DAD)
    { src  = _("Output DAD:");
      src += L'\t';
      src += wxString(DAD, *info.cdp->sys_conv);
    }
    *DAD=0;
    if (!link_GetDadFromXMLForm(info.cdp, info.objnum, DAD, true) && *DAD)
    { if (!src.IsEmpty())
        src += L'\n';
      src += _("Input DAD:");
      src += L'\t';
      src += wxString(DAD, *info.cdp->sys_conv);
    }
#endif
#endif  
    source_window->SetValue(src);
  }
  else  // normal categories
  { if (info.category==CATEG_TABLE && SYSTEM_TABLE(info.objnum))
      source_window->SetValue(wxEmptyString);
    else if (not_encrypted_object(info.cdp, info.category, info.objnum))
    { char * def = cd_Load_objdef(info.cdp, info.objnum, info.category);
      if (def)  // user may not have rights
      { source_window->SetValue(wxString(def, *info.cdp->sys_conv));
        corefree(def);
      }
    }
    else
      source_window->SetValue(_("This object is encrypted. The source code is not available."));
  }
}

void show_data(item_info & info, wxWindow * parent_page)
{ static bool show_data_lock = false;
  if (show_data_lock) return;  // on GTK during open_data_grid in OnInternalIdle the routine is sometimes called recursively
  show_data_lock=true;
  parent_page->DestroyChildren();
  if (info.category!=CATEG_TABLE || !SYSTEM_TABLE(info.objnum))
  {// check for the write lock on the table definition:
    if (info.category==CATEG_TABLE)
    { if (cd_Read_lock_record(info.cdp, TAB_TABLENUM, info.objnum))
        goto ex;  // locked, do not open the form!
      cd_Read_unlock_record(info.cdp, TAB_TABLENUM, info.objnum);
    }
   // open the grid:
    wxWindow * view = open_data_grid(parent_page, info.xcdp(), info.category, info.objnum, info.object_name, false, info.schema_name);
   // make the cursor persistent:
    if (info.category==CATEG_CURSOR && view)
    { DataGrid * grid = (DataGrid*)view;
      grid->make_cursor_persistent();
    }
   // insert it into the page:
    if (view) parent_page->GetSizer()->Add(view, 1, wxGROW, 0);
    parent_page->Layout();
  }
 ex:
  show_data_lock=false;
}

void MyFrame::update_object_info(int pagenum)
{ if (output_window && output_window->IsShown())
  { int sel_cnt = control_panel->objlist->GetSelectedItemCount();
    item_info info = *control_panel->objlist->m_BranchInfo;
    if (sel_cnt>0)
      control_panel->objlist->add_list_item_info(control_panel->objlist->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED), info);
    if (pagenum == PROP_PAGE_NUM)
    { t_property_panel_type req_property_panel;
      if (info.topcat==TOPCAT_SERVERS && info.syscat==SYSCAT_OBJECT && info.category!=CATEG_ROLE && info.category!=CATEG_USER && info.category!=CATEG_GROUP)
        req_property_panel = info.category==CATEG_TABLE && SYSTEM_TABLE(info.objnum) ? PROP_NONE : PROP_BASIC;
      else
        req_property_panel = PROP_NONE;
      if (req_property_panel != property_panel_type) // replacing pages does not work OK
      { wxWindow * page = output_window->GetPage(PROP_PAGE_NUM);
        wxWindow * panel = page->FindWindow(1);
        if (panel) panel->Destroy();
        if (req_property_panel==PROP_BASIC)
        { ObjectPropertiesDlg * dlg = new ObjectPropertiesDlg;
          dlg->Create(page, 1, wxEmptyString, wxPoint(0,0), wxSize(500, 500));
          page->GetSizer()->Add(dlg, 1, wxGROW, 0);
          page->Layout();
          dlg->Show();
        }
        property_panel_type = req_property_panel;
      }
      if (property_panel_type==PROP_BASIC)
      { ObjectPropertiesDlg * dlg = (ObjectPropertiesDlg *)output_window->GetPage(PROP_PAGE_NUM)->FindWindow(1);
        wxString str;
        str.Printf(_("%u object(s) selected"), sel_cnt);
        dlg->mCapt->SetLabel(str);
        dlg->mExport->Enable();
       // label and enabling 2nd checkbox:
        if (info.category==CATEG_TABLE)
          { dlg->mExpData->SetLabel(_("Export data with application"));  dlg->mExpData->Enable(); }
        else if (info.category==CATEG_PROC || info.category==CATEG_CURSOR)
          { dlg->mExpData->SetLabel(_("Run object in Admin mode"));  dlg->mExpData->Enable(); }
        else dlg->mExpData->Disable();
       // enabling 3rd checkbox:
        dlg->mProtect->Enable(ENCRYPTED_CATEG(info.category));
       // pass the states of selected objects:
        int index = -1;  bool first = true;  
        wxCheckBoxState state1, sumstate1 = wxCHK_UNDETERMINED, 
                        state2, sumstate2 = wxCHK_UNDETERMINED,
                        state3, sumstate3 = wxCHK_UNDETERMINED;
        do
        { index=control_panel->objlist->GetNextItem(index, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
          if (index==-1) break;
          info = *control_panel->objlist->m_BranchInfo;
          control_panel->objlist->add_list_item_info(index, info);
         // get object states:
          state1 = (info.flags & CO_FLAG_NOEXPORT) ? wxCHK_UNCHECKED : wxCHK_CHECKED;
          if (info.category==CATEG_TABLE)
            state2 = (info.flags & (CO_FLAG_NOEXPORTD | CO_FLAG_LINK | CO_FLAG_ODBC)) ? wxCHK_UNCHECKED : wxCHK_CHECKED;
          else if (info.category==CATEG_PROC || info.category==CATEG_CURSOR)
            state2 = cd_Admin_mode(info.cdp, info.objnum, NONEBOOLEAN) ? wxCHK_CHECKED : wxCHK_UNCHECKED;
          else 
            state2 = wxCHK_UNDETERMINED; 
          if (ENCRYPTED_CATEG(info.category))
            state3 = info.flags & CO_FLAG_PROTECT ? wxCHK_CHECKED : wxCHK_UNCHECKED;
          else 
            state3 = wxCHK_UNDETERMINED; 
         // state addition:
          if (first)
            { first=false;  sumstate1 = state1;  sumstate2 = state2;  sumstate3 = state3; }
          else
          { if (sumstate1 != state1) sumstate1=wxCHK_UNDETERMINED;
            if (sumstate2 != state2) sumstate2=wxCHK_UNDETERMINED;
            if (sumstate3 != state3) sumstate3=wxCHK_UNDETERMINED; 
          }
        } while (true);
       // show results:
#ifdef WINS
        dlg->mExport ->Set3StateValue(sumstate1);
        dlg->mExpData->Set3StateValue(sumstate2);
        dlg->mProtect->Set3StateValue(sumstate3);
#else  // wxGTK does not support 3-state checkbox in 2.5.1
        if (sumstate1==wxCHK_UNDETERMINED) dlg->mExport ->Disable();
	else dlg->mExport ->SetValue(sumstate1==wxCHK_CHECKED);
        if (sumstate2==wxCHK_UNDETERMINED) dlg->mExpData->Disable();
	else dlg->mExpData->SetValue(sumstate2==wxCHK_CHECKED);
        if (sumstate3==wxCHK_UNDETERMINED) dlg->mProtect->Disable();
	else dlg->mProtect->SetValue(sumstate3==wxCHK_CHECKED);
#endif
      }

    }
    else if (info.topcat==TOPCAT_SERVERS)
    { if (info.syscat==SYSCAT_OBJECT)
      { if (sel_cnt == 1)
        { if (pagenum == SOURCE_PAGE_NUM)
          { if (CATEGORY_WITH_SOURCE(info.category))
              show_source(info, (wxTextCtrl *)output_window->GetPage(SOURCE_PAGE_NUM));
          }
          else if (pagenum == DATA_PAGE_NUM)
          { if (info.category==CATEG_TABLE || info.category==CATEG_CURSOR)
              show_data(info, output_window->GetPage(DATA_PAGE_NUM));
          }
        }
        else // should clear the contents because the object may have been deleted
        { if (pagenum == SOURCE_PAGE_NUM)
          { if (CATEGORY_WITH_SOURCE(info.category))
              ((wxTextCtrl *)output_window->GetPage(SOURCE_PAGE_NUM))->SetValue(wxEmptyString);
          }
          else if (pagenum == DATA_PAGE_NUM)
          { if (info.category==CATEG_TABLE || info.category==CATEG_CURSOR)
              output_window->GetPage(DATA_PAGE_NUM)->DestroyChildren();
          }
        }
      }
      else if ((info.syscat==SYSCAT_APPLTOP || info.syscat==SYSCAT_APPLS) && pagenum == SOURCE_PAGE_NUM)
      { //if (sel_cnt == 1) -- 0 when selected in the tree
          show_source(info, (wxTextCtrl *)output_window->GetPage(SOURCE_PAGE_NUM));
        //else
        //  ((wxTextCtrl *)output_window->GetPage(SOURCE_PAGE_NUM))->SetValue("");
      }
      else if (pagenum == SOURCE_PAGE_NUM)
        ((wxTextCtrl *)output_window->GetPage(SOURCE_PAGE_NUM))->Clear();
    }
    else if (info.topcat==TOPCAT_ODBCDSN)
    { if (info.syscat==SYSCAT_APPLS && pagenum == SOURCE_PAGE_NUM)
        show_source(info, (wxTextCtrl *)output_window->GetPage(SOURCE_PAGE_NUM));
    }
  } // output_window bar is shown
}

void wxOutputNotebook::OnPageChanged(wxNotebookEvent& event)
// When "Source" page is activated displays the source of the selected object.
// When "Data" page is activated displays data from the selected object.
{// first, remove grid from the data page (must not lock table definition(s) when not visible):
  if (GetPageCount() > DATA_PAGE_NUM)  // unless creating the output notebook pages
    GetPage(DATA_PAGE_NUM)->DestroyChildren();
 // create the new contents:
  if (event.GetSelection() == SOURCE_PAGE_NUM || event.GetSelection() == DATA_PAGE_NUM || event.GetSelection() == PROP_PAGE_NUM)
    wxGetApp().frame->update_object_info(event.GetSelection());
  event.Skip();
}

wxNotebook * MyFrame::show_output_page(int page)
// Makes the output window visible and selects its page. Returns NULL on internal error.
{ 
  if (output_window) 
 // show the output window, if hidden, and select the proper page:
  { //if (!output_window->IsShown())  -- does not work, window is said to be shown when its bar is hidden
#ifdef FL
    cbBarInfo * bar = mpLayout->FindBarByWindow(bars[BAR_ID_OUTPUT]);
    if (bar->mState == wxCBAR_HIDDEN)
    { if (bar->mAlignment==-1)  // fixing error in FL
        bar->mAlignment = bar->mDimInfo.mLRUPane;
      mpLayout->SetBarState(bar, wxCBAR_DOCKED_HORIZONTALLY, true);
    }  
#else
    m_mgr.GetPane(bar_name[BAR_ID_OUTPUT]).Show();
    m_mgr.Update();  // "commit" all changes made to wxAuiManager
#endif
    output_window->SetSelection(page);  // selects the page for SQL output
  }
  return output_window;
}
  
////////////////////////////// common actions for the tree and the list //////////////////
void wxCPSplitterWindow::OnSize(wxSizeEvent & event)
{ MyFrame * frame = wxGetApp().frame;
  if (!frame) return;
  if (!tree_view || !objlist) return;
  bool chng=false;
#ifdef FL
  wxFrameLayout *  Layout = frame->mpLayout;
  cbBarInfo * CP_bar = Layout->FindBarByWindow(frame->bars[BAR_ID_CP]);
  cbDockPane* CP_pane = Layout->GetBarPane(CP_bar);
  int docking_state = CP_pane ? CP_pane->GetDockingState() : wxCBAR_DOCKED_HORIZONTALLY; // floating like wxCBAR_DOCKED_HORIZONTALLY
  int split_mode = GetSplitMode();
  
  if (docking_state == wxCBAR_DOCKED_HORIZONTALLY && split_mode==wxSPLIT_HORIZONTAL)
    { Unsplit();  objlist->Show();  SplitVertically(tree_view, objlist);   }
  else if (docking_state == wxCBAR_DOCKED_VERTICALLY && split_mode==wxSPLIT_VERTICAL)
    { Unsplit();  objlist->Show();  SplitHorizontally(tree_view, objlist); }
#else
 // ignore messages with sizes (1,0) sent by wxAUI manager when docking a pane: must not pass it to wxSplitterWindow
  if (event.GetSize().GetWidth() <= 1 || event.GetSize().GetHeight() <= 1)
    return;
  int vert_docking_state = frame->m_mgr.GetPane(bar_name[BAR_ID_CP]).IsDocked() &&
    (frame->m_mgr.GetPane(bar_name[BAR_ID_CP]).dock_direction==wxAUI_DOCK_LEFT ||
     frame->m_mgr.GetPane(bar_name[BAR_ID_CP]).dock_direction==wxAUI_DOCK_RIGHT);
  int split_mode = GetSplitMode();
  if (!vert_docking_state && split_mode==wxSPLIT_HORIZONTAL)
    { Unsplit();  objlist->Show();  SplitVertically(tree_view, objlist);   chng=true; }
  else if (vert_docking_state && split_mode==wxSPLIT_VERTICAL)
    { Unsplit();  objlist->Show();  SplitHorizontally(tree_view, objlist); chng=true; }
#endif
 // check for bad sash positions, repair:
  int pos = GetSashPosition();
  int cw, ch, ext;
  GetClientSize( &cw, &ch );
  ext = GetSplitMode()==wxSPLIT_HORIZONTAL ? ch : cw;
  if (pos<0 || pos>ext-10)
    if (ext>20) SetSashPosition(ext/2);
 // call the original procedure:
  wxSplitterWindow::OnSize(event);
}

void wxCPSplitterWindow::OnConnect(selection_iterator & si)
{ if (!si.next()) return;  // supposed to be impossible
 // find the tree item of the server: either the selected item or its child (supposed!)
  wxTreeItemId serv_item, selitem = tree_view->GetSelection();
  item_info tinfo;
  tree_view->get_item_info(selitem, tinfo);
  if (si.info==tinfo) serv_item=selitem;  // selection from the tree
  else 
  { tree_view->Expand(selitem);  // may not have been expanded
    serv_item = tree_view->FindChild(selitem, si.info);  // server has been selected in the list view, must find the child here
  }
 // connecting:
  t_avail_server * as = find_server_connection_info(si.info.server_name);
  if (!as || as->cdp || as->conn) return;  // internal error
  if (::Connect(si.info.server_name))
  { tree_view->SelectItem(serv_item);  // synchornizes the tree and the list: expanding will fill the list with children of this item
    tree_view->Expand(serv_item);  // feedback is supposed -> connects
    tree_view->SelectItem(serv_item);  // re-creates the action-menu for the connected state
  }
}

t_avail_server * Connect(const char *ServerName)
{ MyFrame * frame = wxGetApp().frame;
  t_avail_server * as = find_server_connection_info(ServerName);
  if (!as) return NULL; // internal error?
  if (as->odbc)
  { as->conn = ODBC_connect(as->locid_server_name, NULL, NULL, (void*)frame->GetHandle());
    if (as->conn) 
    { find_server_image(as, false);
      server_state_changed(as);
     // 
      return as;
    }
    frame->show_output_page(LOG_PAGE_NUM);  // the connection error is in the log
    wxBell();
    return NULL;
  }
  else
  { as->cdp = new cd_t;
    if (!as->cdp)
    { no_memory();
      return NULL;
    }
    cdp_init(as->cdp);
   // if not registered, prepare IP:port:
    char conn_string[1+OBJ_NAME_LEN+1], *pconn;  int err;
    if (connect_by_network && !as->notreg)
    { sprintf(conn_string, "*%s", as->locid_server_name);  
      pconn=conn_string;
    }
    else
      pconn=as->locid_server_name;
    { wxBusyCursor wait;
#ifdef WINS
      err = cd_connect(as->cdp, pconn, SW_SHOWMINNOACTIVE);
#else
      err = cd_connect(as->cdp, pconn, 0);
#endif
    }
    connect_by_network=false;
    frame->Raise();
    //Set_translation_callback(as->cdp, &message_translation_callback);
    if (!err)
    { bool ok;
#ifdef _DEBUG
  char buf[100];
  sprintf(buf, "Connecting client %d.", as->cdp->applnum);
  wxLogDebug(wxString(buf, *wxConvCurrent));
#endif  
      if (!connected_count)
        if (browse_enabled) BrowseStop(0);
      connected_count++;
      if (!make_sys_conv(as->cdp)) ok=false;
      else
        if (cmndl_login && !cd_Login(as->cdp, "", ""))
            ok = true;
        else if (!cd_Login(as->cdp, "*NETWORK", ""))
            ok = true;
        else
        { LoginDlg ld(frame);
          ld.SetCdp(as->cdp);
          ok = ld.ShowModal() == wxID_OK;
        }
        if (ok)
        { find_server_image(as, false);
          server_state_changed(as);
          if (frame->monitor_window)
            frame->monitor_window->Connecting(as->cdp);
          cmndl_login = false;  // command-line logind performed
          cd_Set_progress_report_modulus(as->cdp, notification_modulus);
          cd_SQL_execute(as->cdp, "CALL _SQP_SET_THREAD_NAME('GUI client')", NULL);  
          return as;
        }
        if (as->cdp) wx_disconnect(as->cdp);  // NULL is server crashed on start
    }
    else
	    connection_error_box(as->cdp, err);
    if (as->cdp)
      { delete as->cdp;  as->cdp = NULL; }
    cmndl_login = false;  // do not retry to login
    return NULL;
  }
}

bool close_contents_from_server(xcdp_t xcdp, cdp_t cdp)
{ 
  MyFrame * frame = wxGetApp().frame;
 // pass MDI children:
  wxMDIChildFrame * mdichild = frame->GetActiveChild(); 
  wxMDIChildFrame * mdistart = mdichild;
  bool any_cancelled = false;
  while (mdichild)
  { ServerDisconnectingEvent disev(xcdp);
    mdichild->ProcessEvent(disev);
    if (disev.cancelled) 
    { any_cancelled=true;
      break;  // vetoed, can stop the process
    }
    if (disev.ignored) // window not closed, go to the next
    { frame->ActivateNext();
      mdichild = frame->GetActiveChild();
      if (mdichild==mdistart) break;  // all windows passed
    }
    else // otherwise the active child must have been closed, new start
    {
#if wxMAJOR_VERSION>2 || wxMINOR_VERSION>=5
      wxIdleEvent ev;
      wxGetApp().OnIdle(ev);
#else
      wxGetApp().Yield();  // must be before DeletePendingObjects(), I think
      wxGetApp().DeletePendingObjects();  // whithout this the GetActiveChild() returns the deleted window
#endif
      mdistart = mdichild = frame->GetActiveChild();
    }
  }
  if (any_cancelled)
    return false;

 // close popup designers:
  bool any_closed;
  do // pass ignored frames up to the 1st to be deleted
  { competing_frame * curr_frame = competing_frame_list;
    any_closed = false;
    while (curr_frame) 
    { ServerDisconnectingEvent disev(xcdp);
      curr_frame->OnDisconnecting(disev);
      if (disev.cancelled) return false;  // vetoed, can stop the process
      if (disev.ignored ||             // window not closed, go to the next
          curr_frame->cf_style==ST_CHILD || curr_frame->cf_style==ST_EDCONT)   // skipping editor in the conatiner and child grids, processed on other places
        curr_frame=curr_frame->next;
      else 
      { if (curr_frame->cf_style==ST_AUINB)
        { wxCommandEvent cmd;
          cmd.SetId(TD_CPM_EXIT_UNCOND);
          curr_frame->OnCommand(cmd);
        }
        else if (curr_frame->dsgn_mng_wnd)
          curr_frame->dsgn_mng_wnd->Destroy();
        wxIdleEvent ev;
        wxGetApp().OnIdle(ev);
        any_closed = true;
        break;
      }
    } 
  } while (any_closed);

 // close windows coupled with the server (unconditionally):
  if (frame->output_window)
  { wxPanel * data_panel = (wxPanel*)frame->output_window->GetPage(DATA_PAGE_NUM);
    if (data_panel)
    { wxWindowList & chlist = data_panel->GetChildren();
      if (!chlist.IsEmpty())
      { DataGrid * grid = (DataGrid*)chlist.GetFirst()->GetData();
        ServerDisconnectingEvent disev(xcdp);
        grid->OnDisconnecting(disev);
        if (!disev.ignored) grid->Destroy();
      }
    }
  }
  if (frame->SQLConsole)
    frame->SQLConsole->Disconnecting(xcdp); 
  if (frame->ColumnList)
    frame->ColumnList->Disconnecting(xcdp); 
  if (profiler_dlg && cdp)
    profiler_dlg->disconnecting(cdp);
 // 602SQL specific contents:
  if (cdp)
  { if (frame->monitor_window)
      frame->monitor_window->Disconnecting(cdp);
    if (CallRoutineDlg::global_inst && CallRoutineDlg::global_inst->cdp==cdp)
    { if (the_server_debugger) the_server_debugger->OnCommand(DBG_EXIT);
      CallRoutineDlg::global_inst->Destroy();
    }
  }
  return true;
}

bool try_disconnect(t_avail_server * as)
// Saves updated objects, closes windows coupled with the server and disconnects.
// May be cancelled by the user.
// Does not update the control panel;
{ 
  if (!as || !as->cdp && !as->conn) return true;  // internal error
  xcdp_t xcdp = as->cdp ? (xcdp_t)as->cdp : (xcdp_t)as->conn;
 // breaking the recorsion:
  cdp_t cdp = as->cdp;  as->cdp=NULL;
  t_connection * conn = as->conn;  as->conn=NULL;
 if (!close_contents_from_server(xcdp, cdp))
    return false;
 // clear the clipboard contents if it referes server object:
  C602SQLDataObject Data;
  C602SQLObjects *objs = Data.GetDataFromClipboard();
  if (objs && stricmp(objs->m_Server, as->locid_server_name) == 0)
      wxTheClipboard->Clear();
  MyFrame * frame = wxGetApp().frame;
 // disconnecting:
  if (cdp)
  { wx_disconnect(cdp);
    if (frame->control_panel->tree_view->GetSelInfo()->cdp == cdp)
        frame->control_panel->tree_view->GetSelInfo()->cdp = NULL;
    delete cdp;
  }
#ifdef CLIENT_ODBC_9
  else if (conn)
  { if (frame->control_panel->tree_view->GetSelInfo()->conn == conn)
        frame->control_panel->tree_view->GetSelInfo()->conn = NULL;
    ODBC_disconnect(conn);
  }
#endif
 // update server image:
  find_server_image(as, false);
  return true;
}

void wxCPSplitterWindow::OnDisconnect(selection_iterator & si)
{ while (si.next())
  { t_avail_server * as = find_server_connection_info(si.info.server_name);
    if (!try_disconnect(as)) return;
    server_state_changed(as);
    // IF neni vybrany server, odpojit aktualni a konec, jinak cyklit
    if (!si.info.IsServer())
      break;
  }
  frame->update_action_menu_based_on_selection();  // update the Connect/Disconnect items
  frame->update_object_info(frame->output_window->GetSelection());
}

void wxCPSplitterWindow::OnSyntaxValidation(selection_iterator & si)
{ if (!si.next()) return;  // supposed to be impossible
  wxNotebook * output = frame->show_output_page(SYNTAX_CHK_PAGE_NUM);
  if (!output) return;  // impossible
  wxWindow * syn_page = output->GetPage(SYNTAX_CHK_PAGE_NUM);
  SynChkDlg * dlg = new SynChkDlg(frame);
  dlg->Show();
  Check_component_syntax(si.info.cdp, si.info.schema_name, (SearchResultList *)syn_page, (wxTextCtrl*)dlg->FindWindow(CD_SYNCHK_OBJECT), (wxTextCtrl*)dlg->FindWindow(CD_SYNCHK_COUNT));
  dlg->Destroy();
}

void wxCPSplitterWindow::OnNew(selection_iterator & si)
// Never iterates: iterator may contain multiple folders, uses the 1st of them and silently ignores the others.
{ if (!si.next()) return;  // supposed to be impossible
  if (si.info.topcat==TOPCAT_SERVERS)
  { if (si.info.syscat==SYSCAT_NONE)
    { NewLocalServer dlg;
      dlg.Create(wxGetApp().frame);
      if (dlg.ShowModal()==wxID_OK)
      { item_info info;
        info.topcat=TOPCAT_SERVERS;
        strcpy(info.server_name, dlg.server_name.mb_str(*wxConvCurrent));
        bool private_server = false;
        if (!srv_Register_server_local(info.server_name, dlg.path.mb_str(*wxConvCurrent), private_server))
        { private_server=true;
          if (!srv_Register_server_local(info.server_name, dlg.path.mb_str(*wxConvCurrent), private_server))
            { error_box(ERROR_WRITING_TO_REGISTRY);  return; }
        }
        if (!dlg.default_port) 
          SetDatabaseNum(info.server_name, IP_socket_str, dlg.port_number, private_server);
        sig32 val;
        val = (int)(size_t)dlg.mCharset->GetClientData(dlg.mCharset->GetSelection());
        SetDatabaseNum(info.server_name, "SystemCharset", val, private_server);
        if (dlg.mBig->GetValue())
          SetDatabaseNum(info.server_name, "BlockSize", 8192, private_server);
        add_server(info.server_name, add_server_local, private_server);  // must be before insert_cp_item
        insert_cp_item(info, 0, true);
      }
    }
    else if (si.info.cdp && si.info.syscat!=SYSCAT_CONSOLE)
    { if (create_object(wxGetApp().frame, si.info))
      { Set_object_folder_name(si.info.cdp, si.info.objnum, si.info.category, si.info.folder_name);
        Add_new_component(si.info.cdp, si.info.category, si.info.schema_name, si.info.object_name, 
                          si.info.objnum, si.info.flags, si.info.folder_name, true);
      }
    }
    else
      wxBell();
  }
  else if (si.info.topcat==TOPCAT_CLIENT && si.info.cti==IND_MAIL)
  { tobjname Name;
    CMailProfList ml;
    for (;;)
    { *Name = 0;
      if (!enter_object_name(NULL, this, _("New mail profile name:"), Name, false, 0))
        return;
      if (!ml.Open(TowxChar(Name, wxConvCurrent), false))
        break;
      error_boxp(_("The mail profile named %s already exists. Please use a different name."), this, (const wxChar *)TowxChar(Name, wxConvCurrent));
    }
    CMailProfDlg Dlg;
    Dlg.Create(wxGetApp().frame, wxString(Name, *wxConvCurrent));
    if (Dlg.ShowModal() == wxID_OK)
      objlist->Refresh();
  }
  else
    wxBell();
}

void wxCPSplitterWindow::OnModify(selection_iterator & si, bool alternative)
// Never iterates: iterator may contain multiple objects, uses the 1st of them and silently ignores the others.
{ 
  while (si.next()) 
    if (si.info.topcat==TOPCAT_SERVERS && si.info.syscat!=SYSCAT_NONE && si.info.syscat!=SYSCAT_CONSOLE)
    { if (si.info.syscat==SYSCAT_APPLS && !si.info.cdp)
        edit_conn_data(wxGetApp().frame, si.info, false);
      else if (si.info.cdp)
        modify_object(this, si.info, alternative);
      else
        { wxBell();  break; }
    }
    else if (si.info.topcat==TOPCAT_CLIENT && si.info.cti==IND_MAILPROF)
    { CMailProfDlg Dlg;
      Dlg.Create(wxGetApp().frame, wxString(si.info.object_name, *wxConvCurrent));
      Dlg.ShowModal();
    }
    else
      { wxBell();  break; }
}

void wxCPSplitterWindow::OnRun(selection_iterator & si)
// Never iterates: iterator may contain multiple objects, uses the 1st of them and silently ignores the others.
{ if (!si.next()) return;  // supposed to be impossible
  run_object(wxGetApp().frame, si.info);
}

int ext_process_couner = 0;

#ifdef XMLFORMS

bool save_XML_form(cdp_t cdp, const wxString &fName, tobjnum obj)
{ FHANDLE hFile = CreateFileA(fName.mb_str(*wxConvCurrent), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
  if (hFile == INVALID_FHANDLE_VALUE)
  { client_error(cdp, OS_FILE_ERROR);
    return false;
  }
  bool  OK  = false;
  DWORD Len = GetFileSize(hFile, NULL);
  char *buf = (char *)corealloc(Len+1, 33);
  if (!buf)
    client_error(cdp, OUT_OF_APPL_MEMORY);
  else
  { DWORD rd;
    if (!ReadFile(hFile, buf, Len, &rd, NULL))
      client_error(cdp, OS_FILE_ERROR); 
    else 
    { buf[rd]=0;
      Len = (int)strlen(buf);
      OK = !cd_Write_var(cdp, OBJ_TABLENUM, obj, OBJ_DEF_ATR, NOINDEX, 0, Len, buf);
      if (OK)
        cd_Write_len(cdp, OBJ_TABLENUM, obj, OBJ_DEF_ATR, NOINDEX, Len);
    }
    corefree(buf);
  }
  CloseHandle(hFile);
  return OK;
}

bool load_XML_form(cdp_t cdp, const wxString &fName, tobjnum obj)
{ FHANDLE hFile = CreateFileA(fName.mb_str(*wxConvCurrent), GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
  if (hFile == INVALID_FHANDLE_VALUE)
  { client_error(cdp, OS_FILE_ERROR);
    return false;
  }
  bool OK = true;
  char * def = link_get_xml_form(cdp, obj, true, NULL);
  if (!def) OK=false;
  else
  { DWORD wr;
    if (!WriteFile(hFile, def, (int)strlen(def), &wr, NULL)) OK=false;
    corefree(def);
    CloseHandle(hFile);
  }
  return OK;
}

#ifdef WINS

#include "xmlformwizard.cpp"

void wxCPSplitterWindow::new_XML_form(const item_info &info)
{ 
  if (!check_xml_library())
    return;
  wxString Path = XMLFormFillerPath();
  if (Path == wxEmptyString)
  { if (!yesno_boxp(_("602XML Filler is not installed, created form will not be able to be open.\n\nDo You want to continue?"), this))
      return;
  }
  Path = XMLFormDesignerPath();
  if (Path == wxEmptyString)
  { if (!yesno_boxp(_("602XML Designer is not installed, created default form will not be able to be modified.\n\nDo You want to continue?"), this))
      return;
  }
  const char *DAD = NULL;
  if (info.category == CATEG_PGMSRC)
      DAD = info.object_name; 

  const CXMLFormStruct *FormStruct = NULL;
  wxSetCursor(*wxHOURGLASS_CURSOR);
  CXMLFormWizard Wizard(wxGetApp().frame, info.cdp, DAD);
  bool OK = Wizard.RunWizard(Wizard.LayoutPage);
  wxSetCursor(*wxSTANDARD_CURSOR);
  if (!OK)
      return;

  FormStruct = Wizard.GetStruct(); 

  uns8 priv_val[PRIVIL_DESCR_SIZE];
  if (cd_GetSet_privils(info.cdp, NOOBJECT, CATEG_USER, OBJ_TABLENUM, NORECNUM, OPER_GETEFF, priv_val) || !(*priv_val & RIGHT_INSERT))
  { client_error(info.cdp, NO_RIGHT);  cd_Signalize2(info.cdp, wxGetApp().frame);
    return;
  }
  wxBeginBusyCursor();
  wxString fName     = wxFileName::CreateTempFileName(wxEmptyString);
  wxCharBuffer aName = fName.mb_str(*wxConvCurrent);
  OK = link_NewXMLForm(info.cdp, FormStruct, aName, NOCURSOR, NULL, 0) != FALSE;
  wxEndBusyCursor();
  if (OK)
  { if (Path != wxEmptyString)
    { Path.Replace(wxT("%1"), fName.c_str(), false);
      Path += L"/alias \""; 
      Path += wxString(FormStruct->FormName, *info.cdp->sys_conv);
      Path += L"\"";
      wxExecute(Path, wxEXEC_SYNC);
    }
    tobjname fo;
    strcpy(fo, FormStruct->FormName);
    while (!IsObjNameFree(info.cdp, fo, CATEG_XMLFORM, NULL, NULL))
    { error_box(_("Form named %ls already exists"), wxGetApp().frame);
      if (!GetFreeObjName(info.cdp, fo, CATEG_XMLFORM))
       *fo = 0;
      if (!enter_object_name(info.cdp, wxGetApp().frame, _("Enter the name of the new form:"), fo, false, CATEG_XMLFORM))
        OK = false;
    }
    if (OK)
    { tobjnum obj;
      if (cd_Insert_object(info.cdp, fo, CATEG_XMLFORM, &obj))
      { if (cd_Sz_error(info.cdp) == KEY_DUPLICITY)
        { unpos_box(OBJ_NAME_USED);
          DeleteFileA(aName);
          return;
        }
        OK = false;
      }
      if (OK)
      { OK = save_XML_form(info.cdp, fName, obj);
        if (!OK)
          cd_Delete(info.cdp, OBJ_TABLENUM, obj);
        else
        { uns16 Flags = 0; 
          OK = !cd_Write_ind(info.cdp, OBJ_TABLENUM, (trecnum)obj, OBJ_FLAGS_ATR, NOINDEX, &Flags, sizeof(uns16));
          if (OK)
          { AddObjectToCache(info.cdp, fo, obj, CATEG_XMLFORM, Flags, NULL);
            objlist->Refresh();
          }
        }
      }
    }
  }
  DeleteFileA(aName);
  if (!OK)
    cd_Signalize2(info.cdp, wxGetApp().frame);
}


#include <wx/process.h>

class ExtProcess : public wxProcess
{protected:
  void OnTerminate(int pid, int status)
  { ext_process_couner--;
    delete this;  // calling the virtual destructor
  }
 public:
  ExtProcess(void) : wxProcess(wxPROCESS_DEFAULT)
    { }
  virtual ~ExtProcess(void)  // common actions for "not started" and "terminated"
    { }
  bool start(wxString cmd_line)
  { if (!wxExecute(cmd_line, wxEXEC_ASYNC, this))  // not started
      { delete this;  return false; }  // calling the virtual destructor
    ext_process_couner++;  // external process started, OnTerminate will be called
    return true;
  }
};

class FillerProcess : public ExtProcess
{ wxString file_name;
 public:
  FillerProcess(wxString file_nameIn)
    { file_name=file_nameIn; }
  virtual ~FillerProcess(void)  // common actions for "not started" and "terminated"
    { DeleteFileA(file_name.mb_str(*wxConvCurrent)); }
};

class DesignerProcess : public ExtProcess
{ wxString file_name;
  cdp_t cdp;
  tobjnum objnum;
  uns16 flags;
  tobjname object_name, schema_name;
  time_t CreTime;
  void OnTerminate(int pid, int status);
 public:
  DesignerProcess(wxString file_nameIn, const item_info &info)
  { file_name=file_nameIn; 
    cdp=info.cdp;  objnum=info.objnum;   flags=info.flags;
    strcpy(object_name, info.object_name);
    strcpy(schema_name, info.schema_name);
    CreTime = wxFileModificationTime(file_name); 
  }
  virtual ~DesignerProcess(void)  // common actions for "not started" and "terminated"
    { DeleteFileA(file_name.mb_str(*wxConvCurrent)); }
};

void DesignerProcess::OnTerminate(int pid, int status)
{
  time_t ModTime = wxFileModificationTime(file_name);   
  if (ModTime != 0 && ModTime > CreTime)
  { if (save_XML_form(cdp, file_name, objnum))
      component_change_notif(cdp, CATEG_XMLFORM, object_name, schema_name, flags, objnum);
  }
  ExtProcess::OnTerminate(pid, status);  // deletes file na this, descreases the counter
}

void fill_XML_form(cdp_t cdp, char * final_form)
{ wxString Path = XMLFormFillerPath();
  if (Path == wxEmptyString)
  { error_box(_("602XML Form Filler is not installed."));  // the menu item is not added, but the action may be activated by the double-click on the form
    return;
  }
  wxString fName = wxFileName::CreateTempFileName(wxEmptyString);
  FHANDLE hFile = CreateFileA(fName.mb_str(*wxConvCurrent), GENERIC_WRITE, 
    FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, 
    FILE_ATTRIBUTE_NORMAL, 0);   //  | DELETE,  | FILE_SHARE_DELETE, FILE_FLAG_DELETE_ON_CLOSE
  if (hFile == INVALID_FHANDLE_VALUE)
  { client_error(cdp, OS_FILE_ERROR);
    cd_Signalize2(cdp, wxGetApp().frame);
    return;
  }
  DWORD wr;
  WriteFile(hFile, final_form, (int)strlen(final_form), &wr, NULL);
  CloseHandle(hFile);
  Path.Replace(wxT("%1"), fName.c_str(), false);
  FillerProcess * fp = new FillerProcess(fName);
  //wxExecute(Path, wxEXEC_SYNC);
  fp->start(Path);  // fp will be deleted in failing start() or in OnTerminate() 
}
 
void modi_XML_form(const item_info &info)
{ 
  wxString Path = XMLFormDesignerPath();
  if (Path == wxEmptyString)
    return;
  wxString fName = wxFileName::CreateTempFileName(wxEmptyString);
  bool OK = load_XML_form(info.cdp, fName, info.objnum);
  if (!OK)
  { DeleteFileA(fName.mb_str(*wxConvCurrent));
    cd_Signalize2(info.cdp, wxGetApp().frame);
  } 
  Path.Replace(wxT("%1"), fName.c_str(), false);
  Path += L"/alias \""; 
  Path += wxString(info.object_name, *info.cdp->sys_conv);
  Path += L"\"";
  //wxExecute(Path, wxEXEC_SYNC);
  DesignerProcess * dp = new DesignerProcess(fName, info);
  dp->start(Path);  // dp will be deleted in failing start() or in OnTerminate() 
}

#endif
#endif

void wxCPSplitterWindow::OnDataPrivils(selection_iterator & si)
{ if (si.info.category==CATEG_USER || si.info.category==CATEG_GROUP)
  { ttablenum tbnum = USER_TABLENUM;
    Edit_privileges(si.info.cdp, NULL, &tbnum, NULL, 0);
  }
  else
  { ttablenum * tabnums = (ttablenum*)sigalloc(sizeof(ttablenum) * (si.total_count()+1), 45);
    if (tabnums==NULL) return;
    int n = 0;  tobjnum lobjnum;
    do
    { if (cd_Find_prefixed_object(si.info.cdp, si.info.schema_name, si.info.object_name, CATEG_TABLE|IS_LINK, &lobjnum))  // must find link object
        { cd_Signalize(si.info.cdp);  return; }
      tabnums[n++]=lobjnum;
    } while (si.next());
    tabnums[n]=-1;
    Edit_privileges(si.info.cdp, NULL, tabnums, NULL, si.total_count()>1 ? MULTITAB : 0);
    corefree(tabnums);
  }
}

void wxCPSplitterWindow::OnObjectPrivils(selection_iterator & si)
{ ttablenum tb;  trecnum rec;  tobjnum lobjnum;
  tb = si.info.category==CATEG_TABLE ? TAB_TABLENUM : OBJ_TABLENUM;
  if (!si.next()) return;  // supposed to be impossible
  if (si.total_count()==1)
  { 
    if (cd_Find_prefixed_object(si.info.cdp, si.info.schema_name, si.info.object_name, si.info.category|IS_LINK, &lobjnum))  // must find link object
      { cd_Signalize(si.info.cdp);  return; }
    rec = lobjnum;
    Edit_privileges(si.info.cdp, NULL, &tb, &rec, 0);
  }
  else
  { trecnum * objnums = (trecnum*)sigalloc(sizeof(trecnum) * (si.total_count()+1), 45);
    if (objnums==NULL) return;
    int n = 0;
    do
    { if (cd_Find_prefixed_object(si.info.cdp, si.info.schema_name, si.info.object_name, si.info.category|IS_LINK, &lobjnum))  // must find link object
        { cd_Signalize(si.info.cdp);  return; }
      objnums[n++]=lobjnum;
    } while (si.next());
    objnums[n]=NORECNUM;
    Edit_privileges(si.info.cdp, NULL, &tb, objnums, MULTIREC);
    corefree(objnums);
  }
}

void wxCPSplitterWindow::OnFreeDeleted(selection_iterator & si)
{ while (si.next())
  { wxBusyCursor wait;
    frame->SetStatusText(si.info.object_name_str(), 0);
    if (cd_Free_deleted(si.info.cdp, si.info.objnum))
      { cd_Signalize(si.info.cdp);  return; }  // on error the table name remains on the status bar
  }
  frame->SetStatusText(OPERATION_COMPLETED, 0);
}

void wxCPSplitterWindow::OnCompacting(selection_iterator & si)
{ while (si.next())
  { wxBusyCursor wait;
    frame->SetStatusText(si.info.object_name_str(), 0);
    if (cd_Compact_table(si.info.cdp, si.info.objnum))
      { cd_Signalize(si.info.cdp);  return; }  // on error the table name remains on the status bar
  }
  frame->SetStatusText(OPERATION_COMPLETED, 0);
}

void wxCPSplitterWindow::OnReindex(selection_iterator & si)
{ bool alternative = IsCtrlDown();  wxString msg;  bool any_error = false;
  while (si.next())
  { wxBusyCursor wait;
    frame->SetStatusText(si.info.object_name_str(), 0);
    if (alternative)
    { sig32 result, index_number;
      if (cd_Check_indices(si.info.cdp, si.info.objnum, &result, &index_number))
        { cd_Signalize(si.info.cdp);  return; }  // on error the table name remains on the status bar
     // show the fail result:
      if (result!=1)
      { if (!result) msg.Printf(_("Index %d damaged on table %ls."), index_number, si.info.object_name_str().c_str());
        else msg.Printf(_("The cache of free records damaged on table %ls."), si.info.object_name_str().c_str());
        error_box(msg);
        any_error=true;
      }
    }
    else if (IsShiftDown())   // re-creating the index without releasing the old one
    { if (cd_Enable_index(si.info.cdp, si.info.objnum, -1, TRUE))
        { cd_Signalize(si.info.cdp);  return; }  // on error the table name remains on the status bar
    }    
    else
    { if (cd_Enable_index(si.info.cdp, si.info.objnum, -1, FALSE) || cd_Enable_index(si.info.cdp, si.info.objnum, -1, TRUE))
        { cd_Signalize(si.info.cdp);  return; }  // on error the table name remains on the status bar
    }    
  }
  frame->SetStatusText(OPERATION_COMPLETED, 0);
  if (alternative)
    if (!any_error)
      info_box(_("Test completed, no index errors found."));
}

void wxCPSplitterWindow::OnSequenceRestart(selection_iterator & si)
{ 
  if (wxMessageBox(_("A restarted sequence will generate values that have already been generated before.\nThis may cause key duplicity errors if the sequence value was used as a key in the table.\nAre you sure you want to restart the sequence?"), 
      _("Confirm"), wxYES_NO | wxICON_QUESTION, wxGetApp().frame) != wxYES)
    return;
  while (si.next())
  { frame->SetStatusText(si.info.object_name_str(), 0);
    if (cd_Restart_sequence(si.info.cdp, si.info.objnum, si.info.object_name, si.info.schema_name))
      { cd_Signalize(si.info.cdp);  return; }  // on error the sequence name remains on the status bar
  }
  frame->SetStatusText(OPERATION_COMPLETED, 0);
  frame->update_object_info(frame->output_window->GetSelection());
}

bool WINAPI get_next_sel_obj(char *ObjName, CCateg *Categ, void *Param)
{ char  *objname;
  selection_iterator *si = (selection_iterator *)Param;
  if (!si->next())
    return false;
  // Folder above category
  if (si->info.syscat == SYSCAT_APPLTOP)
  { objname = si->info.folder_name;
    *Categ  = CATEG_FOLDER;
  }
  // Category or folder below category
  else if (si->info.syscat == SYSCAT_CATEGORY)
  { objname = si->info.folder_name;
    *Categ  = si->whole_folder ? CCateg(CATEG_FOLDER) : CCateg(si->info.category, si->info.IsFolder() ? CO_ALLINFOLDER | CO_FOLDERTOO : CO_ALLINFOLDER);
  }
  // Object
  else
  { objname = si->info.object_name;
    *Categ  = si->info.category;
  }
  strcpy(ObjName, objname);
  return true;
}

tiocresult CALLBACK DelWarn(unsigned Stat, const wchar_t *Msg, const char *ObjName, tcateg Categ, bool Single, void *Param)
{
    if (Stat == DOW_CONFIRM)
        return(iocYes);

    if (Single)
    {
        if (!yesno_box(Msg))
            return(iocCancel);
        return(iocYes);
    }
    int Res = yesnoall_box(_("Delete &All"), Msg);
    if (Res == wxID_YES)
        return(iocYes);
    if (Res == wxID_YESTOALL)
        return(iocYesToAll);
    return(iocNo);
}

bool CALLBACK DelState(unsigned Stat, const wxChar *Msg, const char *ObjName, tcateg Categ, bool Single, void *Param)
{
    return ImpExpState(((selection_iterator *)Param)->info.cdp, Stat == 0, _("Failed to remove %s \"%s\""), Msg, ObjName, Categ, Single);
}

void wxCPSplitterWindow::OnDelete(selection_iterator & si)
{ wxWindow *Wnd = wxWindow::FindFocus();
  if (si.info.topcat == TOPCAT_SERVERS && *si.info.server_name)
  { // Server(y) / Aplikace(e)
    if (si.info.syscat == SYSCAT_APPLS || (si.info.syscat == SYSCAT_APPLTOP && !*si.info.folder_name) || si.info.syscat == SYSCAT_CONSOLE)
    { bool any_deleted=false;
      while (si.next())
      { if (si.info.syscat == SYSCAT_APPLS)
        { if (!delete_server(wxGetApp().frame, si.info, true))
            break;
        }
        else if (si.info.syscat == SYSCAT_APPLTOP)
        { if (!delete_appl(wxGetApp().frame, si.info, true))
           break;
        }
        else
          continue;
        any_deleted=true;
      }
      // update the control panel: one or more items deleted. In most cases only the list view changes.
      if (any_deleted)
      { wxTreeItemId selitem=tree_view->GetSelection();
        if (si.info.tree_info)
        { selitem = tree_view->GetItemParent(selitem);
          if (selitem==tree_view->GetRootItem())  // happens when unregistering the only network server
            goto exit;
          tree_view->SelectItem(selitem);
        }
        RefreshItem(selitem);
      }
      else if (si.info.syscat==SYSCAT_APPLTOP)  // may have been partially deleted, must relist the objects
        objlist->Refresh();
    }
    else
    { // Zkontrolovat systemove objekty
      if (si.info.IsSysCategory())
      { if (si.info.syscat == SYSCAT_CATEGORY)
          goto exit;
        if (si.info.category == CATEG_TABLE)
        { error_box(_("Cannot delete the system table"));
          goto exit;
        }
#if 0  // server will report error to the user: better than silently doing nothing
        else if (si.info.category == CATEG_USER)
        { while (si.next())
            if (stricmp(si.info.object_name, "ANONYMOUS") == 0)
              goto exit;
        }
        else if (si.info.category == CATEG_GROUP)
        { while (si.next())
            if (stricmp(si.info.object_name, "EVERYBODY") == 0 || stricmp(si.info.object_name, "DB_ADMIN") == 0 || stricmp(si.info.object_name, "CONFIG_ADMIN") == 0 || stricmp(si.info.object_name, "SECURITY_ADMIN") == 0)
              goto exit;
        }
#endif
      }
      // Spocitat vybrane foldery a zjistit jestli jsou prazdne
      CObjectList ol(si.info.cdp);
      tobjname Folder;
      int  fc     = 0;
      bool fEmpty = true;
      while (si.next())
      { if (si.info.IsFolder())
        { strcpy(Folder, si.info.folder_name);
          fc++;
          for (int c = 0; c < CATEG_COUNT; c++)
          { if (ol.FindFirst(c, Folder))
            { fEmpty = false;
              break;
            }
          }
        }
      }
      // Spocitat vybrane kategorie
      int cc = 0;
      while (si.next())
      { if (si.info.IsCategory())
          cc++;
      }
      // IF Foldery pod kategoriemi && je vybrany aspon jeden folder, zeptat se, zda mazat jen objekty nebo cely folder
      if (fld_bel_cat && fc && !fEmpty)
      {         
        CDelCateg Dlg(si.info.cdp, si.info.category, fc == 1 ? TowxChar(Folder, si.info.cdp->sys_conv) : NULL);
        int Res = Dlg.ShowModal();
        if (Res == wxID_CANCEL)
          goto exit;
        si.whole_folder = Res != 0;
      }
      else 
      { wxString Msg;
        wchar_t wc[64];
        if (fc == si.total_count() && !fEmpty)
        { if (fc > 1)
            Msg = _("Do you really want to delete all objects in the selected folders?");
          else if (ol.FindFirst(CATEG_FOLDER, si.info.folder_name))
            Msg  = wxString::Format(_("Do you really want to delete all objects in the folder \"%s\" (including subfolders)?"), (const wxChar *)TowxChar(si.info.folder_name, si.info.cdp->sys_conv));
          else
            Msg  = wxString::Format(_("Do you really want to delete all objects in the folder \"%s\"?"), (const wxChar *)TowxChar(si.info.folder_name, si.info.cdp->sys_conv));
        }
        else if (cc == si.total_count() && cc == 1)
        { if (!*si.info.folder_name)
        Msg = wxString::Format(_("Do you really want to delete all %s?"), CCategList::Plural(si.info.cdp, si.info.category, wc));
          else if (ol.FindFirst(CATEG_FOLDER, si.info.folder_name))
              Msg = wxString::Format(_("Do you really want to delete all %s in the folder \"%s\" (including subfolders)?"), CCategList::Plural(si.info.cdp, si.info.category, wc), (const wxChar *)TowxChar(si.info.folder_name, si.info.cdp->sys_conv));
          else
            Msg = wxString::Format(_("Do you really want to delete all %s in the folder \"%s\"?"), CCategList::Plural(si.info.cdp, si.info.category, wc), (const wxChar *)TowxChar(si.info.folder_name, si.info.cdp->sys_conv));
        }
        else if (si.total_count() == 1)
        { if (fc)
          { Msg = wxString::Format(_("Do you really want to delete the folder \"%s\"?"), (const wxChar *)TowxChar(Folder, si.info.cdp->sys_conv));
            si.whole_folder = true;
          }
          else
            Msg = wxString::Format(_("Do you really want to delete %s \"%s\"?"), CCategList::Lower(si.info.cdp, si.info.category, wc), (const wxChar *)TowxChar(si.info.object_name, si.info.cdp->sys_conv));
        }
        else
          Msg = _("Do you really want to delete all selected objects?");
        if (!yesno_box(Msg))
          goto exit;
      }
      t_delobjctx delctx;
      memset(&delctx, 0, sizeof(t_delobjctx));
      delctx.strsize  = sizeof(t_delobjctx);
      delctx.flags    = DOPF_NOCONFIRM;
      delctx.count    = si.total_count();
      delctx.param    = &si;
      delctx.nextfunc = get_next_sel_obj;
      //delctx.warnfunc = DelWarn;
      delctx.statfunc = DelState;
      Delete_objects(si.info.cdp, &delctx);
     // a deleted table may be open on the Data Page in the output window -> close it
      frame->output_window->GetPage(DATA_PAGE_NUM)->DestroyChildren();
    }
  }
  else if (si.info.topcat==TOPCAT_CLIENT && si.info.cti==IND_MAILPROF)
  { wxString Msg;
    if (si.total_count() == 1)
      Msg = wxString::Format(_("Do you really want to delete the mail profile \"%s\"?"), (const wxChar *)TowxChar(si.info.object_name, wxConvCurrent));
    else
      Msg = _("Do you really want to delete all selected mail profiles?");
    if (!yesno_box(Msg, this))
      goto exit;
    CMailProfList ml;
    while (si.next())
    { wxString Prof(si.info.object_name, *wxConvCurrent);
      if (!ml.Delete(Prof))
        SysErrorMsg(_("Failed to delete mail profile %s:\n\n%ErrMsg"));
    }
    objlist->Refresh();
  }
exit:
  if (Wnd)
    Wnd->SetFocus();
}

void wxCPSplitterWindow::OnExport(selection_iterator & si)
{ if (si.total_count() == 1)
  { si.next();
    if (si.info.category == CATEG_APPL)
        ExportSchema(si.info.cdp, si.info.schema_name);
    else
        ExportSelObject(si.info.cdp, si.info.object_name, si.info.category);
  }
  else
  { wxString fname = GetExportPath();
    if (fname.IsEmpty())
        return;

    tccharptr aPath   = fname.mb_str(*wxConvCurrent);    
    t_expobjctx expctx;
    memset(&expctx, 0, sizeof(t_expobjctx));
    expctx.strsize   = sizeof(t_expobjctx);
    expctx.count     = si.total_count();
    expctx.flags     = EOF_WITHDESCRIPTOR;
    expctx.file_name = aPath;
    expctx.param     = &si;
    expctx.nextfunc  = get_next_sel_obj;
    expctx.warnfunc  = FileExistsWarning;
    expctx.statfunc  = ExpState;
    Export_objects(si.info.cdp, &expctx);
  }
}

void wxCPSplitterWindow::OnRelogin(selection_iterator & si)
// Single selection is supposed.
{ if (!si.next()) return;  // supposed to be impossible
  if (!close_contents_from_server(si.info.xcdp(), si.info.cdp))
    return;  // cancelled
  LoginDlg ld(this);
  ld.SetCdp(si.info.cdp);
  if (ld.ShowModal()==wxID_OK)  // refill the list (login name is in the 2nd column
    //objlist->Fill(&si.info);  -- wrong, unsynchronizes tree and list!
  { item_info info; 
    tree_view->get_item_info(tree_view->GetSelection(), info);
    objlist->Fill();
    frame->update_action_menu_based_on_selection();  // because the selection in the list is lost
   // re-add the server to the monitor list:
    if (frame->monitor_window)
      frame->monitor_window->Connecting(si.info.cdp);
  }
}

void wxCPSplitterWindow::OnConnSpeed(selection_iterator & si)
// Single selection is supposed.
{ int speed[2];
  if (!si.next()) return;  // supposed to be impossible
  if (cd_Connection_speed_test(si.info.cdp, &speed[0], &speed[1])) cd_Signalize(si.info.cdp);
  else
  { ConnSpeedDlg x(speed[0], speed[1]);
    x.Create(this);  x.ShowModal();  x.Destroy();
  }
}

void wxCPSplitterWindow::RefreshItem(const wxTreeItemId &Item, bool ListNo)
{
    if (Item.IsOk())
    {
        wxBusyCursor Busy;
        tree_view->Freeze();
        tree_view->RefreshItem(Item);
        if (!ListNo)
            objlist->Refresh();
        tree_view->Thaw();
    }
}

void wxCPSplitterWindow::OnRefresh(wxCommandEvent & event)
{ RefreshPanel();
}

BEGIN_EVENT_TABLE( wxCPSplitterWindow, wxSplitterWindow )
    EVT_SIZE(wxCPSplitterWindow::OnSize)
    EVT_MENU(CPM_REFRPANEL, wxCPSplitterWindow::OnRefresh)
END_EVENT_TABLE()

BEGIN_EVENT_TABLE( wxOutputNotebook, wxNotebook)
    EVT_NOTEBOOK_PAGE_CHANGED(-1, wxOutputNotebook::OnPageChanged)
    EVT_SET_FOCUS(wxOutputNotebook::OnFocus)
END_EVENT_TABLE()

void wxOutputNotebook::OnFocus(wxFocusEvent& event)
{ 
// Without this the toolbar buttons cannot return focus to the proper page
#ifdef WINS
  if (GetSelection()!=-1) GetPage(GetSelection())->SetFocus();
#else
  // Direct SetFocus makes the navigation among pages by clicking arrows by mouse (usually) impossible on GTK!
  if (GetSelection()!=-1) wxGetApp().frame->SetFocusDelayed(GetPage(GetSelection()));  
#endif  
  event.Skip();
}

///////////////////////// client error log window /////////////////////////
#define CLIENT_LOG_ITEMS_LIMIT 200

void CALLBACK log_client_error(const char * text)
{ MyFrame * frame = wxGetApp().frame;
  if (!frame) return;  // used on client exit
  wxOutputNotebook * output_window = frame->output_window;
  if (!output_window) return;  // important when disconnecting from servers when client terminates and a server is down
#ifdef LIST_BOX_SQL_OUTPUT
  wxListBox * LogList = (wxListBox *)output_window->GetPage(LOG_PAGE_NUM);
  if (!LogList) return;
  if (LogList->GetCount() > CLIENT_LOG_ITEMS_LIMIT) LogList->Delete(0);
  LogList->Append(wxString(text, *wxConvCurrent));
  LogList->SetSelection(LogList->GetCount()-1);
#else
  wxListCtrl * LogList = (wxListCtrl*)output_window->GetPage(LOG_PAGE_NUM);
  if (LogList->GetItemCount() > CLIENT_LOG_ITEMS_LIMIT) LogList->DeleteItem(0);  // if there are more lines than CLIENT_LOG_ITEMS_LIMIT in the list box, delete the oldest
  long item = LogList->InsertItem(LogList->GetItemCount(), wxString(text, *wxConvCurrent));
  LogList->EnsureVisible(item);
#endif
}
///////////////////////////////////////////////////////////////////////////////////////////////
#ifdef FL
void MyFrame::restore_rows_in_pane(int alignment, const char * section_name)
{ 
  char buf[20], buf2[10];  int r, rh;
  cbDockPane* pane = mpLayout->GetPane(alignment);
  for (r=0;  true;  r++)
  { sprintf(buf2, "R%u", r);  
    if (!read_profile_string(section_name, buf2, buf, sizeof(buf))) break;
    if (sscanf(buf, "%d", &rh)!=1) break;
    cbRowInfo * row = new cbRowInfo;
    row->mRowHeight=rh;
    pane->InsertRow(row, NULL);
  } 
}
#endif

static wxString get_main_title(void) 
{ wxString title;
#if WBVERS>=1100
  title.Printf(_("602SQL Open Server %u.%u Client"), VERS_1, VERS_2);
#else
  title.Printf(_("602SQL %u.%u Client"), VERS_1, VERS_2);
#endif  
  return title;
}

MyFrame::MyFrame(wxFrame *frame)
    : wxMDIParentFrame( frame, -1, get_main_title(), /*_("602SQL 9.0 Database Designer and Management")*/
          wxDefaultPosition, wxDefaultSize, 
          wxCLIP_CHILDREN | wxMINIMIZE_BOX | wxMAXIMIZE_BOX | wxTHICK_FRAME | wxSYSTEM_MENU | wxCAPTION | wxCLOSE_BOX, 
          wxT("602sql_frame") )
{ closing_all=false;
  // set frame size and position
  {// prepare defaults:
    wxSize size(wxSystemSettings::GetMetric(wxSYS_SCREEN_X), wxSystemSettings::GetMetric(wxSYS_SCREEN_Y));  
    if (size.x>1600) size.x=1600;
    if (size.y>1200) size.y=1200;
    if (size.x>800) size.x=size.x/6*5;
    if (size.y>600) size.y=size.y/6*5;
    wxPoint pos((wxSystemSettings::GetMetric(wxSYS_SCREEN_X)-size.x)/2, (wxSystemSettings::GetMetric(wxSYS_SCREEN_Y)-size.y)/2);  
   // read the stored data:
    char buf[40];
    if (read_profile_string("Look", "MainFrame", buf, sizeof(buf)))
      sscanf(buf, " %d %d %d %d", &pos.x, &pos.y, &size.x, &size.y);
    SetSize(pos.x, pos.y, size.x, size.y);
  }
  main_image_list=NULL;
  output_window=NULL;  SQLConsole = NULL;  MainToolBar=NULL;  monitor_window=NULL;  ColumnList=NULL;
  m_container.SetContainerWindow(this);
  delayed_focus = NULL;
#ifndef FL
    // tell wxAuiManager to manage this frame
    m_mgr.SetManagedWindow(this);
#endif
}

#define LINE_SIZE 3
void wxMySeparatorLine::DoSetSize( int x, int y, int width, int height, int sizeFlags)
{ if (width < height)
  { x += (width - LINE_SIZE) / 2;
    width = LINE_SIZE;
  }
  else
  { y += (height - LINE_SIZE) / 2;
    height = LINE_SIZE;
  }
  wxStaticLine::DoSetSize(x, y, width, height, sizeFlags);
}

/////////////////////////////////// wxPanelCons //////////////////////////////////
// wxPanelCons is a control container which contains one control.
// It sizes the control according to the own size but respects its minimal size.

class wxPanelCons : public wxPanel
{public: 
  wxPanelCons(wxWindow * parent) : wxPanel(parent)
    { }
  void OnSize(wxSizeEvent & event);
  DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE( wxPanelCons, wxPanel )
    EVT_SIZE(wxPanelCons::OnSize)
END_EVENT_TABLE()

void wxPanelCons::OnSize(wxSizeEvent & event)
{ int width, height;
  wxWindow * child = FindWindow(1);
  if (!child) return;
  wxSize sz = child->GetSizer()->GetMinSize();
  GetClientSize(&width, &height);
  if (width <sz.x) width =sz.x;
  if (height<sz.y) height=sz.y;
  child->SetSize(0, 0, width, height);
  child->Layout();
}

class wxListCtrlClear : public wxListCtrl
{public:
  wxListCtrlClear(wxWindow * parent, unsigned id, unsigned style) : 
    wxListCtrl(parent, id, wxDefaultPosition, wxDefaultSize, style)
    { }
  void OnRightClick(wxListEvent & event);
  void OnxCommand( wxCommandEvent& event );
  DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE( wxListCtrlClear, wxListCtrl )
  EVT_LIST_COL_RIGHT_CLICK(-1, wxListCtrlClear::OnRightClick) 
  EVT_LIST_ITEM_RIGHT_CLICK(-1, wxListCtrlClear::OnRightClick) 
  EVT_MENU(-1, wxListCtrlClear::OnxCommand)
END_EVENT_TABLE()

void wxListCtrlClear::OnRightClick(wxListEvent & event)
{
  wxMenu * popup_menu = new wxMenu;
  popup_menu->Append(CPM_DELETE, _("Clear"));
  PopupMenu(popup_menu, ScreenToClient(wxGetMousePosition()));
  delete popup_menu;
}

void wxListCtrlClear::OnxCommand( wxCommandEvent& event )
{ 
  switch (event.GetId())
  { case CPM_DELETE:
      if (IsCtrlDown())
        PrintHeapState(log_client_error);
      else DeleteAllItems();  
      break;
    default:
      event.Skip();  break;
  }
}


void MyFrame::create_layout(void)
{   mpClientWnd = GetClientWindow();
    mpClientWnd->SetWindowStyleFlag(mpClientWnd->GetWindowStyleFlag() | wxCLIP_CHILDREN);
    main_image_list = make_image_list();
#ifdef FL
    mpLayout = new wxFrameLayout( this, mpClientWnd );
    cbCommonPaneProperties props;
    mpLayout->GetPaneProperties( props );
    props.mRealTimeUpdatesOn = FALSE; // real-time OFF!!!
    mpLayout->SetPaneProperties( props, wxALL_PANES );
    mpLayout->SetUpdatesManager( new cbGCUpdatesMgr() );
    // this is now default...
    //mpLayout->SetMargins( 1,1,1,1 ); // gaps for vertical/horizontal/right/left panes
    // setup plugins for testing
    mpLayout->PushDefaultPlugins();
    mpLayout->AddPlugin( CLASSINFO( cbBarHintsPlugin ) ); // fancy "X"es and bevel for bars
    //mpLayout->AddPlugin( CLASSINFO( cbHintAnimationPlugin ) );
    mpLayout->AddPlugin( CLASSINFO( cbRowDragPlugin  ) );
    //mpLayout->AddPlugin( CLASSINFO( cbAntiflickerPlugin ) );  //-- flicks! ASSERT errors on GTK!
    //mpLayout->AddPlugin( CLASSINFO( cbSimpleCustomizationPlugin ) ); -- useless, I have a better one
    mpLayout->EnableFloating(true);
#endif  // FL
   //////////////////////////////////////////////// create bars:///////////////////////////////////////////////
   // creating the control panel
    bars[BAR_ID_CP] = create_control_panel(this);
   // creating output windows:
#if 1   // this prevents the fallback of the doubleclick from the monitor notebook page headers into the FL
    wxPanel * output_panel = new wxPanel(this, ID_OUTPUT_NOTEBOOK, wxDefaultPosition, wxDefaultSize, 0);
    output_panel->SetSizer(new wxBoxSizer(wxVERTICAL));
    output_window = new wxOutputNotebook();
    output_window->Create(output_panel, ID_OUTPUT_NOTEBOOK);
    output_panel->GetSizer()->Add(output_window, 1, wxGROW, 0);
    output_panel->Layout();
    bars[BAR_ID_OUTPUT] = output_panel;
#else
    bars[BAR_ID_OUTPUT] = output_window = new wxOutputNotebook();
    output_window->Create(this, ID_OUTPUT_NOTEBOOK);
#endif
    output_window->SetImageList(main_image_list);
    // add pages in the order of their numbers:
    //  SOURCE_PAGE_NUM, DATA_PAGE_NUM, PROP_PAGE_NUM, SQL_OUTPUT_PAGE_NUM, SEARCH_PAGE_NUM, SYNTAX_CHK_PAGE_NUM, LOG_PAGE_NUM,
    wxTextCtrl * source = new wxTextCtrl(output_window, 4, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE | wxTE_READONLY | wxTE_DONTWRAP);
    output_window->AddPage(source, _("Source"), false, IND_INCL);
    wxPanel * data_panel = new wxPanel(output_window, 5, wxDefaultPosition, wxDefaultSize, 0);
    data_panel->SetSizer(new wxBoxSizer(wxVERTICAL));
    output_window->AddPage(data_panel, _("Data"), false, IND_TABLE);
    wxPanel * prop_panel = new wxPanel(output_window, 7, wxDefaultPosition, wxDefaultSize, 0);
    output_window->AddPage(prop_panel, _("Properties"), false, IND_PROPS);
    prop_panel->SetSizer(new wxBoxSizer(wxVERTICAL));
    property_panel_type = PROP_NONE;
#ifdef LIST_BOX_SQL_OUTPUT
    output_window->AddPage(new wxListBox(output_window, 1, wxDefaultPosition, wxDefaultSize, 0, NULL, wxLB_SINGLE | wxLB_NEEDED_SB | wxLB_HSCROLL), _("SQL Output"), false, IND_SQL_CONSOLE);
#else
    //wxListCtrl * lc = new wxListCtrl(output_window, 1, wxDefaultPosition, wxDefaultSize, wxLC_REPORT | wxLC_NO_HEADER | wxLC_SINGLE_SEL | wxVSCROLL | wxHSCROLL);
    wxListCtrl * lc = new wxListCtrlClear(output_window, 1, wxLC_REPORT | wxLC_NO_HEADER | wxLC_SINGLE_SEL | wxVSCROLL | wxHSCROLL);
    lc->InsertColumn(0, wxEmptyString, wxLIST_FORMAT_LEFT, 1500);
    output_window->AddPage(lc, _("SQL Output"), false, IND_SQL_CONSOLE);
#endif
    SearchResultList * srch_list = new SearchResultList;
    srch_list->Create(output_window, 2, wxDefaultPosition, wxDefaultSize, 0, NULL, wxLB_SINGLE | wxLB_NEEDED_SB | wxLB_HSCROLL);
    output_window->AddPage(srch_list, _("Search Results"), false, IND_SEARCH);
    SearchResultList * synchkbox = new SearchResultList;
    synchkbox->Create(output_window, 3, wxDefaultPosition, wxDefaultSize, 0, NULL, wxLB_SINGLE | wxLB_NEEDED_SB | wxLB_HSCROLL);
    output_window->AddPage(synchkbox, _("Syntax Check Results"), false, IND_SYNTAX);
#ifdef LIST_BOX_SQL_OUTPUT
    output_window->AddPage(new wxListBox(output_window, 6, wxDefaultPosition, wxDefaultSize, 0, NULL, wxLB_SINGLE | wxLB_NEEDED_SB | wxLB_HSCROLL), _("Client Log"), false, IND_ALOG);
#else
    //lc = new wxListCtrl(output_window, 6, wxDefaultPosition, wxDefaultSize, wxLC_REPORT | wxLC_NO_HEADER | wxLC_SINGLE_SEL | wxVSCROLL | wxHSCROLL);
    lc = new wxListCtrlClear(output_window, 6, wxLC_REPORT | wxLC_NO_HEADER | wxLC_SINGLE_SEL | wxVSCROLL | wxHSCROLL);
    lc->InsertColumn(0, wxEmptyString, wxLIST_FORMAT_LEFT, 1500);
    output_window->AddPage(lc, _("Client Log"), false, IND_ALOG);
#endif
    set_client_error_callback(log_client_error);
   // create SQL console window:
    wxPanelCons * console_panel = new wxPanelCons(this);
    SQLConsoleDlg * console_inner = SQLConsole = new SQLConsoleDlg();
    console_inner->Create(console_panel, 1);
    console_panel->SetWindowStyleFlag(console_panel->GetWindowStyleFlag() & ~wxTAB_TRAVERSAL);  // since 2.8 necessary to obtain the Ctrl-Return
    bars[BAR_ID_SQL] = console_panel;

    //bars[BAR_ID_SQL] = SQLConsole = new SQLConsoleDlg();
    //SQLConsole->Create(this);
    SQLConsole->Push_cdp(NULL);
   // create server monitor:
#if 1   // this prevents the fallback of the doubleclick from the monitor notebook page headers into the FL
    wxPanel * monitor_panel = new wxPanel(this, ID_MONITOR_NOTEBOOK, wxDefaultPosition, wxDefaultSize, 0);
    monitor_panel->SetSizer(new wxBoxSizer(wxVERTICAL));
    monitor_window = new MonitorNotebook();
    monitor_window->Create(monitor_panel, ID_MONITOR_NOTEBOOK);
    monitor_panel->GetSizer()->Add(monitor_window, 1, wxGROW, 0);
    monitor_panel->Layout();
    bars[BAR_ID_MONITOR] = monitor_panel;
#else
    bars[BAR_ID_MONITOR] = monitor_window = new MonitorNotebook();
    monitor_window->Create(this, ID_MONITOR_NOTEBOOK);
#endif
    wxImageList * monitor_il = new wxImageList(16, 16, true, 6);
    { wxBitmap bmp(monitor_xpm);  monitor_il->Add(bmp); }  
    { wxBitmap bmp(clients_xpm);  monitor_il->Add(bmp); }  
    { wxBitmap bmp(locks_xpm);    monitor_il->Add(bmp); }  
    { wxBitmap bmp(trace_xpm);    monitor_il->Add(bmp); }  
    { wxBitmap bmp(extinfo_xpm);  monitor_il->Add(bmp); }  
    { wxBitmap bmp(alog_xpm);     monitor_il->Add(bmp); }  
    monitor_window->AssignImageList(monitor_il);  // takes ownership
    MonitorMainDlg * mon_main = new MonitorMainDlg;
    mon_main->Create(monitor_window);
    monitor_window->AddPage(mon_main, _("Monitor"), false, 0);
    monitor_window->reset_main_info();  // clars the contents
   // create the toolbars:  
#if WBVERS<1000
    wxDynamicToolBar * dtb;  // must use pointer with the exact type (cannot use MainToolBar)!
    bars[BAR_ID_MAIN_TOOLBAR] = MainToolBar = dtb = new wxDynamicToolBar();
    dtb->Create( this, -1 );
    dtb->AddTool( CPM_CUT,    wxBitmap(cut_xpm), wxNullBitmap, false, -1, -1, NULL, _("Cut object(s)") );
    dtb->AddTool( CPM_COPY,   wxBitmap(Kopirovat_xpm), wxNullBitmap, false, -1, -1, NULL, _("Copy object(s)") );
    dtb->AddTool( CPM_PASTE,  wxBitmap(Vlozit_xpm), wxNullBitmap, false, -1, -1, NULL, _("Paste object(s)") );
    dtb->AddTool( CPM_REFRPANEL, wxBitmap(refrpanel_xpm), wxNullBitmap, false, -1, -1, NULL, _("Refresh control panel") );
    dtb->AddSeparator(new wxMySeparatorLine(dtb, -1));
    dtb->AddTool( CPM_NEW,    wxBitmap(new_xpm),    wxNullBitmap, false, -1, -1, NULL, _("New object") );   // "object" - because the tool works with CP objects only
    dtb->AddTool( CPM_MODIFY, wxBitmap(modify_xpm), wxNullBitmap, false, -1, -1, NULL, _("Modify object") );
    dtb->AddTool( CPM_DELETE, wxBitmap(delete_xpm), wxNullBitmap, false, -1, -1, NULL, _("Delete object(s)") );
#endif
#ifdef FL
    bars[BAR_ID_DESIGNER_TOOLBAR] = competing_frame::designer_toolbar = new wxDynamicToolBar();
    competing_frame::designer_toolbar->Create(this, -1 );
#else
    bars[BAR_ID_DESIGNER_TOOLBAR] = competing_frame::designer_toolbar = new wxToolBar(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTB_FLAT | wxTB_NODIVIDER | wxNO_BORDER);
    competing_frame::designer_toolbar->SetToolBitmapSize(wxSize(16,16));
    competing_frame::designer_toolbar->Realize();
#endif
   // column info panel:
    wxPanel * column_info_panel = new wxPanel(this, ID_COLUMN_INFO, wxDefaultPosition, wxDefaultSize, 0);
    column_info_panel->SetSizer(new wxBoxSizer(wxVERTICAL));
    ColumnList = new ColumnsInfoDlg(column_info_panel);
    column_info_panel->GetSizer()->Add(ColumnList, 1, wxGROW, 0);
    column_info_panel->Layout();
    bars[BAR_ID_COLUMN_LIST] = column_info_panel;
    
   // create debugger windows:
    bars[BAR_ID_WATCH]=create_persistent_watch_view(this, BAR_ID_WATCH);
    bars[BAR_ID_CALLS]=create_persistent_call_stack(this, BAR_ID_CALLS);
    bars[BAR_ID_DBG_TOOLBAR]=create_persistent_dbg_toolbar(this);

#ifdef FL
    mpLayout->EnableFloating( TRUE ); 
    { layout_descr ld;
      ld.set_default_normal();
      ld.load("LayoutNormal");
      ld.apply(bars);
    }
#else
    m_mgr.AddPane(bars[BAR_ID_CP],               wxAuiPaneInfo().Name(bar_name[BAR_ID_CP]).     Caption(wxGetTranslation(bar_name[BAR_ID_CP])). CaptionVisible(false)./*CloseButton(false).*/Gripper().GripperTop().Left()./*MinSize(80,80).*/FloatingSize(450,300).BestSize(220,230));
    m_mgr.AddPane(bars[BAR_ID_OUTPUT],           wxAuiPaneInfo().Name(bar_name[BAR_ID_OUTPUT]). Caption(wxGetTranslation(bar_name[BAR_ID_OUTPUT])). Bottom().Position(2).FloatingSize(400,220).BestSize(300,220));
    m_mgr.AddPane(bars[BAR_ID_SQL],              wxAuiPaneInfo().Name(bar_name[BAR_ID_SQL]).    Caption(wxGetTranslation(bar_name[BAR_ID_SQL])).    Bottom().Position(1).FloatingSize(200,220).BestSize(200,220));
    m_mgr.AddPane(bars[BAR_ID_MONITOR],          wxAuiPaneInfo().Name(bar_name[BAR_ID_MONITOR]).Caption(wxGetTranslation(bar_name[BAR_ID_MONITOR])).Bottom().Position(0).FloatingSize(400,220).BestSize(300,220));
    m_mgr.AddPane(bars[BAR_ID_CALLS],            wxAuiPaneInfo().Name(bar_name[BAR_ID_CALLS]).  Caption(wxGetTranslation(bar_name[BAR_ID_CALLS])).Bottom().Row(1).Hide().FloatingSize(300,200).BestSize(300,200));
    m_mgr.AddPane(bars[BAR_ID_WATCH],            wxAuiPaneInfo().Name(bar_name[BAR_ID_WATCH]).  Caption(wxGetTranslation(bar_name[BAR_ID_WATCH])).Bottom().Row(1).Hide().FloatingSize(300,200).BestSize(300,200));
    m_mgr.AddPane(bars[BAR_ID_COLUMN_LIST],      wxAuiPaneInfo().Name(bar_name[BAR_ID_COLUMN_LIST]).     Caption(wxGetTranslation(bar_name[BAR_ID_COLUMN_LIST])).Float().Hide().FloatingSize(100,400).BestSize(100,400));
    m_mgr.AddPane(bars[BAR_ID_DESIGNER_TOOLBAR], wxAuiPaneInfo().Name(bar_name[BAR_ID_DESIGNER_TOOLBAR]).Caption(wxGetTranslation(bar_name[BAR_ID_DESIGNER_TOOLBAR]))./*CaptionVisible(false).Layer(10).Fixed().*/ToolbarPane().Top()./*Position(0).*/LeftDockable(false).RightDockable(false)/*.Gripper().GripperTop(false).Floatable(false).MinSize(300,26).BestSize(300,26)*/);
    m_mgr.AddPane(bars[BAR_ID_DBG_TOOLBAR],      wxAuiPaneInfo().Name(bar_name[BAR_ID_DBG_TOOLBAR]).     Caption(wxGetTranslation(bar_name[BAR_ID_DBG_TOOLBAR])).     /*CaptionVisible(false).Layer(10).Fixed().*/ToolbarPane().Top()./*Position(0).*/LeftDockable(false).RightDockable(false)/*.Gripper().GripperTop(false).Floatable(false).MinSize(300,26).MaxSize(300,26).BestSize(300,26).FloatingSize(300,26)*/.Hide());
    wxSize client_size = GetClientSize();
    aui_nb = new wxAuiNotebookExt(this, wxID_ANY,
                                    wxPoint(client_size.x, client_size.y),
                                    wxSize(430,200),
                                    wxAUI_NB_TOP | wxAUI_NB_TAB_MOVE | wxAUI_NB_SCROLL_BUTTONS | /*wxAUI_NB_CLOSE_ON_ACTIVE_TAB |*/ wxNO_BORDER);
    m_mgr.AddPane(aui_nb, wxAuiPaneInfo().Name(wxT("notebook_content")).CenterPane().PaneBorder(false));
    load_layout("LayoutNormal"); 
    set_style(global_style);  // calls m_mgr.Update()
#endif
}

void MyFrame::set_style(t_global_style new_global_style)
{
  global_style=new_global_style;
  m_mgr.GetPane(wxT("mdiclient")).       Show(global_style==ST_MDI);
  m_mgr.GetPane(wxT("notebook_content")).Show(global_style==ST_AUINB);
  if (bars[BAR_ID_DESIGNER_TOOLBAR])
    m_mgr.GetPane(bar_name[BAR_ID_DESIGNER_TOOLBAR]).Show(global_style==ST_MDI);
  m_mgr.Update();  // "commit" all changes made to wxAuiManager
}

void wxCPSplitterWindow::AddObject(const char *ServerName, const char *SchemaName, const char *FolderName, const char *ObjName, tcateg Categ, uns16 Flags, uns32 Modif, bool SelectIt)
{
    wxTreeItemId tItem = tree_view->AddObject(ServerName, SchemaName, FolderName, ObjName, Categ);
    // IF Vybrat a vlozena nova polozka a (strom ma fokus nebo novy objekt v novem folderu)
    if (SelectIt && tItem.IsOk() && (m_ActiveChild == (wxControl *)tree_view || (Categ != NONECATEG && Categ != XCATEG_SERVER && Categ != CATEG_APPL && Categ != CATEG_FOLDER)))
        tree_view->SelectItem(tItem);
    long lItem = objlist->AddObject(ServerName, SchemaName, FolderName, ObjName, Categ, Flags, Modif);
    if (SelectIt && lItem != -1)
        objlist->SelectItem(lItem);
    /*
    if (SelectIt)
    {
        if (FocusId == TREE_ID)
        {
            if (tItem.IsOk())
                tree_view->SelectItem(tItem);
        }
        else if (FocusId == LIST_ID)
        {
            if (lItem != -1)
                objlist->SelectItem(lItem);
        }
        else if (Categ == NONECATEG || Categ == XCATEG_SERVER || Categ == CATEG_APPL || Categ == CATEG_FOLDER)
        {
            if (tItem.IsOk())
                tree_view->SelectItem(tItem);
        }
        else if (lItem != -1)
            objlist->SelectItem(lItem);
    }
    */
}

#ifdef STOP  // not necessary, using the local names everywhere
tccharptr GetLocalServerName(const char *ServerName)
{
    for (t_avail_server *as = available_servers; as; as = as->next)
    {
        if (as->cdp && !my_stricmp(as->cdp->conn_server_name, ServerName))
        {
            ServerName = as->locid_server_name;
            break;
        }
    }
    tccharptr Result(strlen(ServerName) + 1);
    Result = ServerName;
    return Result;
}
#endif

void wxCPSplitterWindow::DeleteObject(const char *ServerName, const char *SchemaName, const char *ObjName, tcateg Categ)
{
    //tccharptr sName = GetLocalServerName(ServerName);
    tree_view->DeleteObject(ServerName, SchemaName, ObjName, Categ);
    objlist->DeleteObject(ServerName, SchemaName, ObjName, Categ);
}

void wxCPSplitterWindow::RefreshSchema(const char *ServerName, const char *SchemaName)
{
    wxBusyCursor Busy;
    //tccharptr sName = GetLocalServerName(ServerName);
    tree_view->RefreshSchema(ServerName, SchemaName);
    objlist->RefreshSchema(ServerName, SchemaName);
}

void wxCPSplitterWindow::RefreshServer(const char *ServerName)
{
    wxBusyCursor Busy;
    //tccharptr sName = GetLocalServerName(ServerName);
    tree_view->RefreshServer(ServerName);
    objlist->RefreshServer(ServerName);
}

void wxCPSplitterWindow::RefreshPanel()
{   
    refresh_server_list();
    for (t_avail_server *as = available_servers; as; as = as->next)
        if (as->cdp)
            cd_Relist_objects_ex(as->cdp, true);
    RefreshItem(tree_view->rootId);
}

wxCPSplitterWindow::~wxCPSplitterWindow(void)
{ int width, height, division;
 // save division:
  GetClientSize(&width, &height);
  int split_mode = GetSplitMode();
  if (split_mode==wxSPLIT_VERTICAL)
    division = 1000*GetSashPosition() / (!width ? 1 : width);
  else
    division = 1000*GetSashPosition() / (!height ? 1 : height);
  if (restore_default_request)
  { write_profile_string("Look", "VertCPSplit", "");
    write_profile_string("Look", "HorizCPSplit", "");
  }
  else
    write_profile_int("Look", split_mode==wxSPLIT_VERTICAL ? "VertCPSplit" : "HorizCPSplit", division);
  write_profile_bool("Look", "Show folders", ShowFolders);
  write_profile_bool("Look", "Show empty folders", ShowEmptyFolders);
  write_profile_bool("Look", "Folders below categories", fld_bel_cat);
}

// BAR_ID_CP, BAR_ID_OUTPUT, BAR_ID_SQL, BAR_ID_MONITOR, BAR_ID_CALLS, BAR_ID_WATCH, BAR_COLUMN_LIST
// BAR_ID_MAIN_TOOLBAR, BAR_ID_DESIGNER_TOOLBAR, BAR_ID_DBG_TOOLBAR
const wxChar * bar_name[BAR_COUNT] =
{ wxTRANSLATE("Control Panel"), wxTRANSLATE("Output"), wxTRANSLATE("SQL Console"), wxTRANSLATE("Server Monitor"), wxTRANSLATE("Column List"), 
  wxTRANSLATE("Call Stack"), wxTRANSLATE("Watch List"), 
#if WBVERS<1000
  wxTRANSLATE("Main ToolBar"),  
#endif
  wxTRANSLATE("Designer ToolBar"), wxTRANSLATE("Debug Toolbar") 
};

const char * bar_name_profile[BAR_COUNT] =
{ "Control Panel", "Output", "SQL Console", "Server Monitor", "Column List", "Call Stack", "Watch List"
#if WBVERS<1000
  "Main ToolBar", 
#endif
  "Designer ToolBar", "Debug Toolbar" };

#define DYN_TOOL_HI 30

#ifdef FL
void layout_descr::load(const char * name)
{ char buf[200];
  for (int i=0;  i</*BAR_COUNT*/BAR_ID_FIRST_TOOLBAR;  i++)
  { bar_layout * abl = &bl[i];
    if (read_profile_string(name, bar_name_profile[i], buf, sizeof(buf)))
      sscanf(buf, " %d %d %d %d %d %d  %d %d %d %d", &abl->w[0], &abl->h[0], &abl->w[1], &abl->h[1], &abl->w[2], &abl->h[2], &abl->state, &abl->alignment, &abl->row, &abl->row_offset);
    if (i>=BAR_ID_FIRST_TOOLBAR)
      abl->h[0]=abl->w[1]=abl->h[2]=DYN_TOOL_HI;
   // repair errors:
    if (abl->w[0]>32000) abl->w[0]=200;
    if (abl->h[0]>32000) abl->h[0]=200;
    if (abl->w[1]>32000) abl->w[1]=200;
    if (abl->h[1]>32000) abl->h[1]=200;
    if (abl->w[2]>32000) abl->w[2]=200;
    if (abl->h[2]>32000) abl->h[2]=200;
  }
 // repair possible errors:
  if (!strcmp(name, "LayoutNormal"))
  { bl[BAR_ID_CALLS].state = bl[BAR_ID_WATCH].state = bl[BAR_ID_DBG_TOOLBAR].state = 
      wxCBAR_HIDDEN;
  }
#if WBVERS>=1000  // if !global_mdi_style hide the designer toolbar and move the gebug toolbar left
  bl[BAR_ID_DESIGNER_TOOLBAR].state = global_mdi_style ? wxCBAR_DOCKED_HORIZONTALLY  : wxCBAR_HIDDEN;
  if (!strcmp(name, "LayoutDebug"))
    bl[BAR_ID_DBG_TOOLBAR].row_offset = global_mdi_style ? DSGNTBSZ : 0;
#endif
#ifdef LINUX  // 205 is the minimum reasonable size, bad display on Linux if smaller
  if (bl[BAR_ID_SQL].w[0]<215) bl[BAR_ID_SQL].w[0]=215;
#endif
}

void layout_descr::save(const char * name)
{ char buf[200];
  for (int i=0;  i<BAR_COUNT;  i++)
  { bar_layout * abl = &bl[i];
    sprintf(buf, "%d %d %d %d %d %d  %d %d %d %d", abl->w[0], abl->h[0], abl->w[1], abl->h[1], abl->w[2], abl->h[2], abl->state, abl->alignment, abl->row, abl->row_offset);
    write_profile_string(name, bar_name_profile[i], restore_default_request ? "" : buf);
  }
}

void layout_descr::apply_for_pane(wxFrameLayout * fl, wxWindow * bars[], int pane)
{ int row_searched=0, row_created=0, i, extent_sum;
  bool found, found_next;
  cbDockPane* Pane = fl->GetPane(pane);
  do  // cycles on rows
  { found=found_next=false;  extent_sum=0; 
   // find if there is any bar in [row_searched] (sets [found]) or any row with a bigger number (sets [found_next])
    for (i=0;  i<BAR_COUNT;  i++)
      if (bars[i]) 
        if (bl[i].state==wxCBAR_DOCKED_VERTICALLY || bl[i].state==wxCBAR_DOCKED_HORIZONTALLY)
          if (bl[i].alignment==pane) 
            if (bl[i].row==row_searched)
            { if (bl[i].state==wxCBAR_DOCKED_VERTICALLY)
                extent_sum+=bl[i].h[bl[i].state];  // dimensions interchanged
              else
                extent_sum+=bl[i].w[bl[i].state];  
              found=true; 
            }
            else if (bl[i].row>row_searched) found_next=true;
   // [row_searched] found:
    if (found)
    { wxList mRowShapeData;
      int ro_limit = 100000;
      do  // must pass in the order of decreasing [row_offset]
      {// Find a bar docked in [pane] and [row_searched] with biggest [row_offset], but <[ro_limit]. Write its index to [ind].
        int ind = -1;
        for (i=0;  i<BAR_COUNT;  i++)
          if (bars[i]) 
            if (bl[i].state==wxCBAR_DOCKED_VERTICALLY || bl[i].state==wxCBAR_DOCKED_HORIZONTALLY)
              if (bl[i].alignment==pane) 
                if (bl[i].row==row_searched)
                  if (bl[i].row_offset<ro_limit)
                    if (ind==-1 || bl[i].row_offset>bl[ind].row_offset)
                      ind=i;
        if (ind==-1) break;  // no more bars docked in [pane] and [row_searched]
        ro_limit=bl[ind].row_offset;  // preents finding the bar again
       // find bar info, add if not added:
        bar_layout * abl = &bl[ind];
        cbBarInfo * bar = fl->FindBarByWindow(bars[ind]);
        if (!bar)
        { cbDimInfo sizes(abl->w[0],abl->h[0], // when docked horizontally (e.g. left margin)     
                          abl->w[1],abl->h[1], // when docked vertically        
                          abl->w[2],abl->h[2], // when floated                  
                          abl->is_toolbar, 4, 4, // fixed-size, vertical gap (bar border), horizontal gap (bar border)
                          abl->is_toolbar ? new cbDynToolBarDimHandler() : NULL);
          fl->AddBar(bars[ind], sizes, pane, row_created, 0/*abl->row_offset*/, wxGetTranslation(bar_name[ind]), !abl->is_toolbar, abl->state);
          // row offset must be 0, otherwise the ordering is indeterminate
          bar = fl->FindBarByWindow(bars[ind]);
        }
        bar->mDimInfo.mLRUPane=0;  // error patch
        fl->SetBarState(bar, abl->state, false);
        cbBarShapeData* pData = new cbBarShapeData();
        pData->mBounds.x=abl->row_offset;  pData->mBounds.y=4;
        if (abl->state==wxCBAR_DOCKED_VERTICALLY)
          { pData->mBounds.width=abl->h[abl->state];  pData->mBounds.height=abl->w[abl->state]; }
        else
          { pData->mBounds.width=abl->w[abl->state];  pData->mBounds.height=abl->h[abl->state]; }
        pData->mLenRatio = (double)pData->mBounds.width / (double)extent_sum;
        mRowShapeData.Insert((wxObject*)pData);
      } while (true);
      Pane->SetRowShapeData(Pane->GetRow(row_created), &mRowShapeData);
     // delete the mRowShapeData list contents:
      for (wxNode *node = mRowShapeData.GetFirst();  node;  node = node->GetNext())
        delete  (cbBarShapeData*)node->GetData();
      row_created++;
    }
    row_searched++;
  } while (found_next);
}

void layout_descr::apply(wxWindow * bars[])
{ int i;
  wxFrameLayout * fl = wxGetApp().frame->mpLayout;
 // hide all bars:
  for (i=0;  i<BAR_COUNT;  i++)
    if (bars[i])  // window is open
    { cbBarInfo * bar = fl->FindBarByWindow(bars[i]);
      if (bar) 
      { bar->mDimInfo.mLRUPane=0;  // error patch
        fl->SetBarState(bar, wxCBAR_HIDDEN, false);
        fl->RemoveBar(bar);
      }
    }
  // rows destroyed, I hope
 // show floating bars, add hidden bars:
  for (i=0;  i<BAR_COUNT;  i++)
    if (bars[i])  // window is open
      if (bl[i].state==wxCBAR_FLOATING || bl[i].state==wxCBAR_HIDDEN)
      { cbBarInfo * bar = fl->FindBarByWindow(bars[i]);
        if (!bar)
        { cbDimInfo sizes( bl[i].w[0],bl[i].h[0], // when docked horizontally (e.g. left margin)     
                        bl[i].w[1],bl[i].h[1], // when docked vertically        
                        bl[i].w[2],bl[i].h[2], // when floated                  
                        bl[i].is_toolbar, 4, 4, // fixed-size, vertical gap (bar border), horizontal gap (bar border)
                        bl[i].is_toolbar ? new cbDynToolBarDimHandler() : NULL);
          fl->AddBar(bars[i], sizes, bl[i].alignment, 0,0,wxGetTranslation(bar_name[i]), !bl[i].is_toolbar, bl[i].state==wxCBAR_FLOATING ? 0 : bl[i].state);
          bar = fl->FindBarByWindow(bars[i]);
        }
        bar->mDimInfo.mLRUPane=0;  // error patch
        if (bl[i].state==wxCBAR_FLOATING)
        { fl->SetBarState(bar, wxCBAR_FLOATING, false);
          fl->RepositionFloatedBar(bar);
        }
      }
 // docked panes:
  apply_for_pane(fl, bars, FL_ALIGN_BOTTOM);
  apply_for_pane(fl, bars, FL_ALIGN_TOP);
  apply_for_pane(fl, bars, FL_ALIGN_LEFT);
  apply_for_pane(fl, bars, FL_ALIGN_RIGHT);
}

void layout_descr::get_current_state(wxWindow * bars[])
{ int i;
  wxFrameLayout * fl = wxGetApp().frame->mpLayout;
  for (i=0;  i<BAR_COUNT;  i++)
    if (bars[i])  // window is open
    { cbBarInfo * bar = fl->FindBarByWindow(bars[i]);
      if (!bar) bl[i].state = wxCBAR_HIDDEN;
      else
      { bl[i].alignment=0;  bl[i].row=0;
        if (bar->mState != wxCBAR_HIDDEN && bar->mState != wxCBAR_FLOATING)
        { cbDockPane* pPane;  cbRowInfo*  pRow;
          fl->LocateBar( bar, &pRow, &pPane );
          bl[i].alignment = pPane->GetAlignment();
          while (pRow->mpPrev)
            { bl[i].row++;  pRow=pRow->mpPrev; }
          //bar.mRowNo;  -- is 0 for all rows
        }
        bl[i].w[0]=bar->mDimInfo.mBounds[0].width;  bl[i].h[0]=bar->mDimInfo.mBounds[0].height;
        bl[i].w[1]=bar->mDimInfo.mBounds[1].width;  bl[i].h[1]=bar->mDimInfo.mBounds[1].height; 
        bl[i].w[2]=bar->mDimInfo.mBounds[2].width;  bl[i].h[2]=bar->mDimInfo.mBounds[2].height;
        if (bar->mState == wxCBAR_FLOATING)
        { bar->mpBarWnd->GetSize(&bl[i].w[2], &bl[i].h[2]);   // dimensions are stored nowhere, must find them by itself
          bl[i].w[2] -= 0x30; // empirical
        }
        else if (bar->mState != wxCBAR_HIDDEN)
          if (bar->mState == wxCBAR_DOCKED_VERTICALLY)  // switched
            { bl[i].w[bar->mState]=bar->mBounds.height;  bl[i].h[bar->mState]=bar->mBounds.width; }
          else
            { bl[i].w[bar->mState]=bar->mBounds.width;  bl[i].h[bar->mState]=bar->mBounds.height; }
        if (bl[i].w[0]==-1)
          { bl[i].w[0]=bar->mDimInfo.mSizes[0].x;  bl[i].h[0]=bar->mDimInfo.mSizes[0].y; }
        if (bl[i].w[1]==-1)
          { bl[i].w[1]=bar->mDimInfo.mSizes[1].x;  bl[i].h[1]=bar->mDimInfo.mSizes[1].y; }
        if (bl[i].w[2]==-1)
          { bl[i].w[2]=bar->mDimInfo.mSizes[2].x;  bl[i].h[2]=bar->mDimInfo.mSizes[2].y; }
        bl[i].state=bar->mState;
        bl[i].row_offset=bar->mBounds.x;
      }
    }
}

// Dimensions: LEFT-RIGHT docked, TOP-BOTTOM docked, floating
void layout_descr::set_default_normal(void)
{
  bl[BAR_ID_CP].     set(FL_ALIGN_LEFT, 0, 0, wxCBAR_DOCKED_VERTICALLY, 230, 230, 230, 200, 200, 200, false);
  bl[BAR_ID_OUTPUT]. set(FL_ALIGN_BOTTOM, 0, 250, wxCBAR_DOCKED_HORIZONTALLY, 270, 90, 270, 90, 270, 90, false);
  bl[BAR_ID_SQL].    set(FL_ALIGN_BOTTOM, 0, 0, wxCBAR_DOCKED_HORIZONTALLY, 260, 90, 260, 90, 260, 90, false);
  bl[BAR_ID_CALLS].  set(FL_ALIGN_BOTTOM, 0, 0, wxCBAR_HIDDEN, 80, 80, 80, 250, 250, 250, false);
  bl[BAR_ID_WATCH].  set(FL_ALIGN_BOTTOM, 0, 0, wxCBAR_HIDDEN, 80, 80, 80, 250, 250, 250, false);
#if WBVERS<1000
  bl[BAR_ID_MAIN_TOOLBAR    ].set(FL_ALIGN_TOP, 0, 0,   wxCBAR_DOCKED_HORIZONTALLY, 168, DYN_TOOL_HI+10, 168, DYN_TOOL_HI, 168, DYN_TOOL_HI, true);
  bl[BAR_ID_DESIGNER_TOOLBAR].set(FL_ALIGN_TOP, 0, 168, wxCBAR_DOCKED_HORIZONTALLY, DSGNTBSZ, DYN_TOOL_HI+10, DSGNTBSZ, DYN_TOOL_HI, DSGNTBSZ, DYN_TOOL_HI, true);
#else
  bl[BAR_ID_DESIGNER_TOOLBAR].set(FL_ALIGN_TOP, 0, 0,   wxCBAR_DOCKED_HORIZONTALLY, DSGNTBSZ, DYN_TOOL_HI+10, DSGNTBSZ, DYN_TOOL_HI, DSGNTBSZ, DYN_TOOL_HI, true);
#endif
  bl[BAR_ID_DBG_TOOLBAR     ].set(FL_ALIGN_TOP, 0, 168+DSGNTBSZ, wxCBAR_HIDDEN,              310, DYN_TOOL_HI+10, 310, DYN_TOOL_HI, 310, DYN_TOOL_HI, true);
  bl[BAR_ID_MONITOR].set(FL_ALIGN_RIGHT, 0, 0, wxCBAR_HIDDEN, 300, 300, 300, 300, 300, 300, false);
}                                                   

void layout_descr::set_default_debug(void)
{
  bl[BAR_ID_CP].     set(FL_ALIGN_LEFT, 0, 0, wxCBAR_DOCKED_VERTICALLY, 230, 230, 230, 200, 200, 200, false);
  bl[BAR_ID_OUTPUT]. set(FL_ALIGN_BOTTOM, 0, 250, wxCBAR_HIDDEN, 270, 90, 270, 90, 270, 90, false);
  bl[BAR_ID_SQL].    set(FL_ALIGN_BOTTOM, 0, 0, wxCBAR_DOCKED_HORIZONTALLY, 260, 90, 260, 90, 260, 90, false);
  bl[BAR_ID_CALLS].  set(FL_ALIGN_BOTTOM, 0, 1, wxCBAR_DOCKED_VERTICALLY, 80, 80, 80, 250, 250, 250, false);
  bl[BAR_ID_WATCH].  set(FL_ALIGN_BOTTOM, 0, 2, wxCBAR_DOCKED_VERTICALLY, 80, 80, 80, 250, 250, 250, false);
#if WBVERS<1000
  bl[BAR_ID_MAIN_TOOLBAR    ].set(FL_ALIGN_TOP, 0, 0,   wxCBAR_DOCKED_HORIZONTALLY, 168, DYN_TOOL_HI+10, 168, DYN_TOOL_HI, 168, DYN_TOOL_HI, true);
  bl[BAR_ID_DESIGNER_TOOLBAR].set(FL_ALIGN_TOP, 0, 168, wxCBAR_DOCKED_HORIZONTALLY, DSGNTBSZ, DYN_TOOL_HI+10, DSGNTBSZ, DYN_TOOL_HI, DSGNTBSZ, DYN_TOOL_HI, true);
  bl[BAR_ID_DBG_TOOLBAR     ].set(FL_ALIGN_TOP, 0, 168+DSGNTBSZ, wxCBAR_DOCKED_HORIZONTALLY, 310, DYN_TOOL_HI+10, 310, DYN_TOOL_HI, 310, DYN_TOOL_HI, true);
#else
  bl[BAR_ID_DESIGNER_TOOLBAR].set(FL_ALIGN_TOP, 0, 0,   wxCBAR_DOCKED_HORIZONTALLY, DSGNTBSZ, DYN_TOOL_HI+10, DSGNTBSZ, DYN_TOOL_HI, DSGNTBSZ, DYN_TOOL_HI, true);
  bl[BAR_ID_DBG_TOOLBAR     ].set(FL_ALIGN_TOP, 0, DSGNTBSZ, wxCBAR_DOCKED_HORIZONTALLY, 310, DYN_TOOL_HI+10, 310, DYN_TOOL_HI, 310, DYN_TOOL_HI, true);
#endif
  bl[BAR_ID_MONITOR].set(FL_ALIGN_RIGHT, 0, 0, wxCBAR_HIDDEN, 300, 300, 300, 300, 300, 300, false);
}

void switch_layouts(bool to_debug)
{ wxFrameLayout * fl = wxGetApp().frame->mpLayout;
  { layout_descr ld;
    if (to_debug) ld.set_default_normal();  else ld.set_default_debug();
    ld.get_current_state(wxGetApp().frame->bars);
    ld.save(to_debug ? "LayoutNormal" : "LayoutDebug");
    if (to_debug) ld.set_default_debug();   else ld.set_default_normal();
    ld.load(to_debug ? "LayoutDebug" : "LayoutNormal");
    ld.apply(wxGetApp().frame->bars);
  }
  fl->RefreshNow(true);  // without this the toolbar is not removed
}

#else  // !FL

#define PERSP_PART_LEN 100

void MyFrame::save_layout(const char * name)
{ char partname[10];  int partnum=1;
  wxString perspective = m_mgr.SavePerspective();  wxString part;
  do
  { if (perspective.Length() > PERSP_PART_LEN) 
    { part = perspective.Left(PERSP_PART_LEN);
      perspective = perspective.Mid(PERSP_PART_LEN);
    }
    else 
    { part = perspective;
      perspective = wxEmptyString;
    }
    part=wxT("[")+part+wxT("]");
    sprintf(partname, "Part%u", partnum++);
    write_profile_string(name, partname, part.mb_str(wxConvUTF8));
  } while (part.Length()>2);
}

bool MyFrame::load_layout(const char * name)
// Call m_mgr.Update(); after this!
{ char partname[10];  int partnum=1;
  wxString perspective;
  do
  { char buf[2*PERSP_PART_LEN+2+1];  // may be UTF-8 encoded
    sprintf(partname, "Part%u", partnum++);
    if (!read_profile_string(name, partname, buf, sizeof(buf))) break;
    int len=(int)strlen(buf);
    if (len<=2) break;  // "><" or error
    bool utf8 = *buf=='[';
    buf[strlen(buf)-1]=0;  // remove "<"
    if (utf8) // must not use the conditional parameter to wxString constructor
      perspective = perspective + wxString(buf+1, wxConvUTF8);
    else  
      perspective = perspective + wxString(buf+1, *wxConvCurrent);
  } while (true);
  if (!perspective.IsEmpty())
  { m_mgr.LoadPerspective(perspective);
   // captions of panes may be translated to a wrong language, must re-translate:
    for (int i=0;  i<BAR_COUNT;  i++)
      m_mgr.GetPane(bar_name[i]).Caption(wxGetTranslation(bar_name[i]));
    return true;
  }
  return false;
}

void switch_layouts(bool to_debug)
{ MyFrame * frame = wxGetApp().frame;
  frame->save_layout(to_debug ? "LayoutNormal" : "LayoutDebug");
  bool loaded = frame->load_layout(to_debug ? "LayoutDebug" : "LayoutNormal");
 // show the debug bars if the debug layout has not been stored yet:
  if (to_debug)
  { if (!loaded)
    { frame->m_mgr.GetPane(bar_name[BAR_ID_WATCH]).Show();
      frame->m_mgr.GetPane(bar_name[BAR_ID_CALLS]).Show();
    }  
   // show the debug toolbar always: 
    frame->m_mgr.GetPane(bar_name[BAR_ID_DBG_TOOLBAR]).Show();
  }
  frame->set_style(global_style);  // calls m_mgr.Update()
}
#endif  // FL

void delete_bitmaps(t_tool_info * ti)
{ while (ti->command)
  { if (ti->command!=-1)
      if (ti->bitmap_created) 
        { delete ti->bmp;  ti->bmp=NULL;  ti->bitmap_created=false; }
    ti++;
  }
}

MyFrame::~MyFrame()
{ closing_all=true;
  Set_notification_callback(NULL);  // stop writing to the status bar
#ifdef _DEBUG
    ::wxLogDebug(wxT("MyFrame::~MyFrame"));
#endif
#ifdef FL
  { layout_descr ld;
    ld.set_default_normal();
    ld.get_current_state(bars);
    ld.save("LayoutNormal");
  }
#else
  save_layout("LayoutNormal");
#endif
  if (!IsIconized())
  { wxPoint pos, size;  char buf[40];
    GetSize(&size.x, &size.y);  GetPosition(&pos.x, &pos.y);
    sprintf(buf, " %d %d %d %d", pos.x, pos.y, size.x, size.y);
    write_profile_string("Look", "MainFrame", restore_default_request ? "" : buf);
  }
  write_profile_int("Look", "MDI", (int)global_style);
 // save last positions and sizes of designers:
  tabledsng_coords.save();
  querydsng_coords.save();
  transdsng_coords.save();
  xmldsng_coords.save();
  diagram_coords.save();
  editor_cont_coords.save();
  editor_sep_coords.save();
  seqdsng_coords.save();
  domdsng_coords.save();
  datagrid_coords.save();
  fulltext_coords.save();
 // stop browsing for servers (before closing windows because the browse callback writes to them!):
  //TcpIpQueryDeinit();
  set_server_scan_callback(NULL);
  if (browse_enabled && !connected_count) BrowseStop(0);
  browse_enabled=false;  // prevents restarting the browsing when wx_disconnect() is called (the value must not be saved!)
#ifdef WINS
  NetworkStop(NULL);
#endif
  help_ctrl.Quit();
 // must unlink the status bar before destroying childres, otherwise the frame destroys it as a child and uses it then in canculations
  SetStatusBar(NULL);
 // must destroy the layout before destroying chldren, otherwise crashes then resizing the non-existing children
#ifdef FL
  if ( mpLayout) 
    { delete mpLayout;  mpLayout=NULL; } // should be destroyed manually
#else
  m_mgr.UnInit();
#endif
 // destroy all children BEFORE disconnecting from servers: designers will unlock the edited objects!
  destroy_persistent_debug_windows();   // must be after destroying the layout
  output_window=NULL;  // some windows close own cursors and produce errors, but the server is disconnected and the output of error messages to the non-existing log windows must be disabled
  DestroyChildren();
  SQLConsole = NULL;  MainToolBar=NULL;  monitor_window=NULL;
  if (main_image_list) delete main_image_list; // must be after destroying the children which use it
  main_image_list=NULL;
 // disconnect from all servers:
  while (available_servers)
  { t_avail_server * as = available_servers;
    available_servers = as->next;
    if (as->cdp)
    { wx_disconnect(as->cdp);
      delete as->cdp;
    }
#ifdef CLIENT_ODBC_9
    if (as->conn)
      { ODBC_disconnect(as->conn);  as->conn=NULL; }
#endif
    delete as;
  }
  ODBC_release_env();
 // delete bitmaps in tool lists:
  dom_delete_bitmaps();
  seq_delete_bitmaps();
  table_delete_bitmaps();
  xml_delete_bitmaps();
  diagram_delete_bitmaps();
  query_delete_bitmaps();
  trans_delete_bitmaps();
  fsed_delete_bitmaps();
  grid_delete_bitmaps();
  fulltext_delete_bitmaps();
 // save fonts:
  wxString font_descr;
  font_descr = text_editor_font->GetNativeFontInfoDesc();
  write_profile_string("Fonts", "TextEditor", font_descr.mb_str(*wxConvCurrent));
  delete text_editor_font;  text_editor_font=NULL;
  font_descr = grid_label_font->GetNativeFontInfoDesc();
  write_profile_string("Fonts", "GridLabel", font_descr.mb_str(*wxConvCurrent));
  delete grid_label_font;  grid_label_font=NULL;
  font_descr = grid_cell_font->GetNativeFontInfoDesc();
  write_profile_string("Fonts", "GridCell", font_descr.mb_str(*wxConvCurrent));
  delete grid_cell_font;  grid_cell_font=NULL;
  if (control_panel_font)
  { font_descr = control_panel_font->GetNativeFontInfoDesc();
    write_profile_string("Fonts", "ControlPanel", font_descr.mb_str(*wxConvCurrent));
    delete control_panel_font;  control_panel_font=NULL;
  }  
 // save formats:
  write_profile_string("Format", "DecimalSeparator", format_decsep.mb_str(*wxConvCurrent));
  write_profile_string("Format", "Date",             format_date.mb_str(*wxConvCurrent));
  write_profile_string("Format", "Time",             format_time.mb_str(*wxConvCurrent));
  write_profile_string("Format", "Timestamp",        format_timestamp.mb_str(*wxConvCurrent));
  write_profile_int   ("Format", "Real",    format_real);
  ControlPanelList::SaveLayout();

  delete DF_602SQLObject;   DF_602SQLObject=NULL;
  delete DF_602SQLDataRow;  DF_602SQLDataRow=NULL;
  drop_ini_file_name();  // just returning the memory
  if (global_conf)
  { global_conf->Flush();
    wxConfig::Set(NULL);
    delete global_conf;
  }	
  set_client_error_callback(NULL);  // must not report network errors when frame does not exist
  wxGetApp().frame=NULL;  // stops logging client errors
}

void MyFrame::OnClose(wxCloseEvent & event)
{
 // veto if waiting for external processes:
  if (event.CanVeto() && ext_process_couner>0)
  { info_box(_("Terminate the external processes first (602XML Filler, Designer,...)."));
    event.Veto();
    return;
  }
 // if the debugging is in progress, stop it:
  if (the_server_debugger)
    the_server_debugger->OnCommand(DBG_EXIT);
 // close designers:
  wxWindowList & chlist = GetClientWindow()->GetChildren();
  for (int i=0;  i<chlist.GetCount();  i++)
  { wxWindow * ch = chlist.Item(i)->GetData();
    ch->Close(!event.CanVeto());
  }
  Destroy();
}

void AppendFolderItems(wxMenu *Menu)
{
    Menu->AppendSeparator();
    Menu->AppendCheckItem(CPM_SHOWFOLDERS,    _("&Show Folders"));   
    Menu->Check(CPM_SHOWFOLDERS,     ControlPanel()->ShowFolders);
    Menu->AppendCheckItem(CPM_SHOWEMPTYFLDRS, _("Show &Empty Folders"));
    Menu->Check(CPM_SHOWEMPTYFLDRS,  ControlPanel()->ShowEmptyFolders);
    Menu->Enable(CPM_SHOWEMPTYFLDRS, ControlPanel()->ShowFolders);
    Menu->AppendCheckItem(CPM_FLDBELCAT,      _("&Folders Below Categories"));
    Menu->Check(CPM_FLDBELCAT,       ControlPanel()->fld_bel_cat);
    Menu->Enable(CPM_FLDBELCAT,      ControlPanel()->ShowFolders);
}

///////////////////////////////// stored positions and sizes of designers ///////////////////////
/* Coordinates are loaded from the profile when the designer is first opened.
   Coordinates are save in the profile when the main window is closed.
   Coordinates are stored in [t_designer_window_coords] when the designer is closed.
   Saving coordinates from the designer to [t_designer_window_coords] does not work when the main window 
     is being closed. 
*/

t_designer_window_coords tabledsng_coords  ("Table Designer",     wxSize(620,420));
t_designer_window_coords querydsng_coords  ("Query Designer",     wxSize(620,420));
t_designer_window_coords transdsng_coords  ("Transport Designer", wxSize(420,320));
t_designer_window_coords xmldsng_coords    ("XML Designer",       wxSize(420,320));
t_designer_window_coords diagram_coords    ("Diagram Designer",   wxSize(520,460));
t_designer_window_coords editor_cont_coords("Editor Container",   wxSize(420,500));
t_designer_window_coords editor_sep_coords ("Editor Window",      wxSize(420,400));
t_designer_window_coords seqdsng_coords    ("Sequence Designer",  wxSize(0,0));
t_designer_window_coords domdsng_coords    ("Domain Designer",    wxSize(0,0));
t_designer_window_coords datagrid_coords   ("Data Grid",          wxSize(400,300));
t_designer_window_coords fulltext_coords   ("Fulltext Designer",  wxSize(420,320));

void t_designer_window_coords::load(void)
{ pos = get_best_designer_pos();
  char buf[50];
  if (read_profile_string("Coords", designer_name, buf, sizeof(buf)))
    sscanf(buf, " %d %d %d %d", &pos.x, &pos.y, &size.x, &size.y);
  coords_loaded=true;
}

void t_designer_window_coords::save(void)
{ if (!coords_updated) return;
  char buf[50];
  sprintf(buf, "%d %d %d %d", pos.x, pos.y, size.x, size.y);
  write_profile_string("Coords", designer_name, restore_default_request ? "" : buf);
}
///////////////////// validators ////////////////////////////////////////////
#if 0  // not using
class MyIntValidator : public wxValidator
{ int * intptr;
 public:
  MyIntValidator(int * intptrIn) : intptr(intptrIn)
    { }
  MyIntValidator(const MyIntValidator & patt)
    { intptr = patt.intptr; }
  wxObject* Clone(void) const
    { return new MyIntValidator(*this); }
  bool TransferFromWindow(void);
  bool TransferToWindow(void);
  bool Validate(wxWindow * parent);
};

bool MyIntValidator::TransferFromWindow(void)
{ return true; }
bool MyIntValidator::TransferToWindow(void)
{ char buf[20];
  sprintf(buf, "%d", *intptr);
  ((wxTextCtrl*)GetWindow())->SetValue(buf);
  return true;
}
bool MyIntValidator::Validate(wxWindow * parent)
{ return true; }
#endif

char NULLSTRING[2] = "";

#ifdef LINUX
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <net/if.h>

unsigned char mac_address[6];

bool get_mac_address(void)
/* Get the hardware address to interface "eth0" (this should be the ethernet
 * adress of the first card), and store it to mac_address. Return true on success. */
{ struct ifreq ifr;  int sockfd;
  if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) return false;
  strcpy(ifr.ifr_name, "eth0");
  if (-1==ioctl(sockfd, SIOCGIFHWADDR, &ifr))
    memset(ifr.ifr_hwaddr.sa_data, 0, 6); //{ close(sockfd);  return false; }
  close(sockfd);
  memcpy(mac_address, ifr.ifr_hwaddr.sa_data, 6);
  return true;
}
#endif
