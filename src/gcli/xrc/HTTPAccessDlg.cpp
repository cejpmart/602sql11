/////////////////////////////////////////////////////////////////////////////
// Name:        HTTPAccessDlg.cpp
// Purpose:     
// Author:      
// Modified by: 
// Created:     03/27/06 12:33:45
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#if defined(__GNUG__) && !defined(__APPLE__)
//#pragma implementation "HTTPAccessDlg.h"
#endif

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

////@begin includes
////@end includes

#include "HTTPAccessDlg.h"
#include "DataGridDlg.cpp"

////@begin XPM images
////@end XPM images

/*!
 * HTTPAccessDlg type definition
 */

IMPLEMENT_CLASS( HTTPAccessDlg, wxDialog )

/*!
 * HTTPAccessDlg event table definition
 */

BEGIN_EVENT_TABLE( HTTPAccessDlg, wxDialog )

////@begin HTTPAccessDlg event table entries
    EVT_TEXT( CD_HTTP_HOSTNAME, HTTPAccessDlg::OnCdHttpHostnameUpdated )

    EVT_BUTTON( CD_HTTP_SQLHOST, HTTPAccessDlg::OnCdHttpSqlhostClick )

    EVT_RADIOBUTTON( CD_HTTP_STANDALONE, HTTPAccessDlg::OnCdHttpStandaloneSelected )

    EVT_RADIOBUTTON( CD_HTTP_EMULATED, HTTPAccessDlg::OnCdHttpEmulatedSelected )

    EVT_BUTTON( CD_HTTP_START_EMULATION, HTTPAccessDlg::OnCdHttpStartEmulationClick )

    EVT_TEXT( CD_HTTP_PORT, HTTPAccessDlg::OnCdHttpPortUpdated )

    EVT_TEXT( CD_HTTP_PATH, HTTPAccessDlg::OnCdHttpPathUpdated )

    EVT_RADIOBUTTON( CD_HTTP_ANONYMOUS, HTTPAccessDlg::OnCdHttpAnonymousSelected )

    EVT_RADIOBUTTON( CD_HTTP_AUTHENTIFICATE, HTTPAccessDlg::OnCdHttpAuthentificateSelected )

    EVT_BUTTON( CD_HTTP_PASSWORD, HTTPAccessDlg::OnCdHttpPasswordClick )

    EVT_BUTTON( wxID_OK, HTTPAccessDlg::OnOkClick )

    EVT_BUTTON( wxID_APPLY, HTTPAccessDlg::OnApplyClick )

    EVT_BUTTON( wxID_CANCEL, HTTPAccessDlg::OnCancelClick )

    EVT_BUTTON( wxID_HELP, HTTPAccessDlg::OnHelpClick )

////@end HTTPAccessDlg event table entries
  //  EVT_CLOSE(HTTPAccessDlg::OnClose )
END_EVENT_TABLE()

/*!
 * HTTPAccessDlg constructors
 */

HTTPAccessDlg::HTTPAccessDlg(cdp_t cdpIn)
{ cdp=cdpIn;
}

//void HTTPAccessDlg::OnClose(wxCloseEvent & event)
//{
//  cdp=cdp;
//}
/*!
 * HTTPAccessDlg creator
 */

bool HTTPAccessDlg::Create( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
////@begin HTTPAccessDlg member initialisation
    mHostSizer = NULL;
    mHostNameLabel = NULL;
    mHostName = NULL;
    mSQLHost = NULL;
    mStandalone = NULL;
    mEmulated = NULL;
    mStartEmulation = NULL;
    mPort = NULL;
    mApache = NULL;
    mPath = NULL;
    mURL = NULL;
    mAnonymous = NULL;
    mAuthentificate = NULL;
    mPasswords = NULL;
    mOK = NULL;
    mApply = NULL;
////@end HTTPAccessDlg member initialisation

////@begin HTTPAccessDlg creation
    SetExtraStyle(GetExtraStyle()|wxWS_EX_BLOCK_EVENTS);
    wxDialog::Create( parent, id, caption, pos, size, style );

    CreateControls();
    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);
    Centre();
////@end HTTPAccessDlg creation
    return TRUE;
}

/*!
 * Control creation for HTTPAccessDlg
 */

void HTTPAccessDlg::CreateControls()
{    
////@begin HTTPAccessDlg content construction
    HTTPAccessDlg* itemDialog1 = this;

    wxBoxSizer* itemBoxSizer2 = new wxBoxSizer(wxVERTICAL);
    itemDialog1->SetSizer(itemBoxSizer2);

    mHostSizer = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer2->Add(mHostSizer, 0, wxGROW|wxALL, 0);

    mHostNameLabel = new wxStaticText;
    mHostNameLabel->Create( itemDialog1, CD_HOST_NAME_LABEL, _("Host name:"), wxDefaultPosition, wxDefaultSize, 0 );
    mHostSizer->Add(mHostNameLabel, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxTOP|wxBOTTOM|wxADJUST_MINSIZE, 5);

    mHostName = new wxTextCtrl;
    mHostName->Create( itemDialog1, CD_HTTP_HOSTNAME, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    if (ShowToolTips())
        mHostName->SetToolTip(_("Host name of the computer providing HTTP access to the SQL server, e.g. www.mycompany.com"));
    mHostSizer->Add(mHostName, 1, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    mSQLHost = new wxButton;
    mSQLHost->Create( itemDialog1, CD_HTTP_SQLHOST, _("Use SQL server's name"), wxDefaultPosition, wxDefaultSize, 0 );
    if (ShowToolTips())
        mSQLHost->SetToolTip(_("Use this when the SQL server emulates a web server or when the web server runs on the same computer as the SQL server. Otherwise enter the host name of the web server."));
    mHostSizer->Add(mSQLHost, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxTOP|wxBOTTOM, 5);

    mStandalone = new wxRadioButton;
    mStandalone->Create( itemDialog1, CD_HTTP_STANDALONE, _("Use a stand-alone web server connected to the SQL server by a PHP module"), wxDefaultPosition, wxDefaultSize, wxRB_GROUP );
    mStandalone->SetValue(FALSE);
    itemBoxSizer2->Add(mStandalone, 0, wxALIGN_LEFT|wxALL, 5);

    wxBoxSizer* itemBoxSizer8 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer2->Add(itemBoxSizer8, 0, wxGROW|wxALL, 0);

    mEmulated = new wxRadioButton;
    mEmulated->Create( itemDialog1, CD_HTTP_EMULATED, _("Use a web server emulation by the SQL server"), wxDefaultPosition, wxDefaultSize, 0 );
    mEmulated->SetValue(FALSE);
    itemBoxSizer8->Add(mEmulated, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxBOTTOM, 5);

    itemBoxSizer8->Add(5, 5, 1, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    mStartEmulation = new wxButton;
    mStartEmulation->Create( itemDialog1, CD_HTTP_START_EMULATION, _("Start emulation"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer8->Add(mStartEmulation, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxBOTTOM, 5);

    wxBoxSizer* itemBoxSizer12 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer2->Add(itemBoxSizer12, 0, wxGROW|wxTOP|wxBOTTOM, 5);

    wxStaticText* itemStaticText13 = new wxStaticText;
    itemStaticText13->Create( itemDialog1, wxID_STATIC, _("Port number for the web XML interface (80 is the default):"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer12->Add(itemStaticText13, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxBOTTOM|wxADJUST_MINSIZE, 5);

    mPort = new wxTextCtrl;
    mPort->Create( itemDialog1, CD_HTTP_PORT, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    if (ShowToolTips())
        mPort->SetToolTip(_("When emulating the web server by an SQL server, the port number in defined in the Protocols and Ports dialog"));
    itemBoxSizer12->Add(mPort, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxBOTTOM, 5);

    wxStaticBox* itemStaticBoxSizer15Static = new wxStaticBox(itemDialog1, wxID_ANY, _("Stand-alone web server"));
    mApache = new wxStaticBoxSizer(itemStaticBoxSizer15Static, wxHORIZONTAL);
    itemBoxSizer2->Add(mApache, 0, wxGROW|wxLEFT|wxRIGHT|wxBOTTOM, 5);

    wxStaticText* itemStaticText16 = new wxStaticText;
    itemStaticText16->Create( itemDialog1, wxID_STATIC, _("Path to the 602.php file in the web document tree:"), wxDefaultPosition, wxDefaultSize, 0 );
    mApache->Add(itemStaticText16, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    mPath = new wxTextCtrl;
    mPath->Create( itemDialog1, CD_HTTP_PATH, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    mApache->Add(mPath, 1, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxBoxSizer* itemBoxSizer18 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer2->Add(itemBoxSizer18, 0, wxGROW|wxLEFT|wxRIGHT|wxBOTTOM, 5);

    wxStaticText* itemStaticText19 = new wxStaticText;
    itemStaticText19->Create( itemDialog1, wxID_STATIC, _("URL form:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer18->Add(itemStaticText19, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    mURL = new wxStaticText;
    mURL->Create( itemDialog1, CD_HTTP_URL, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer18->Add(mURL, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    wxStaticBox* itemStaticBoxSizer21Static = new wxStaticBox(itemDialog1, wxID_ANY, _("Web user authentication"));
    wxStaticBoxSizer* itemStaticBoxSizer21 = new wxStaticBoxSizer(itemStaticBoxSizer21Static, wxVERTICAL);
    itemBoxSizer2->Add(itemStaticBoxSizer21, 0, wxGROW|wxLEFT|wxRIGHT|wxBOTTOM, 5);

    mAnonymous = new wxRadioButton;
    mAnonymous->Create( itemDialog1, CD_HTTP_ANONYMOUS, _("User of the web XML interface is anonymous on the SQL server"), wxDefaultPosition, wxDefaultSize, wxRB_GROUP );
    mAnonymous->SetValue(FALSE);
    itemStaticBoxSizer21->Add(mAnonymous, 0, wxGROW|wxLEFT|wxRIGHT|wxTOP, 5);

    mAuthentificate = new wxRadioButton;
    mAuthentificate->Create( itemDialog1, CD_HTTP_AUTHENTIFICATE, _("The basic HTTP authentication (username and password) is required"), wxDefaultPosition, wxDefaultSize, 0 );
    mAuthentificate->SetValue(FALSE);
    itemStaticBoxSizer21->Add(mAuthentificate, 0, wxGROW|wxALL, 5);

    wxBoxSizer* itemBoxSizer24 = new wxBoxSizer(wxHORIZONTAL);
    itemStaticBoxSizer21->Add(itemBoxSizer24, 0, wxGROW|wxBOTTOM, 5);

    itemBoxSizer24->Add(5, 5, 1, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    mPasswords = new wxButton;
    mPasswords->Create( itemDialog1, CD_HTTP_PASSWORD, _("Edit web passwords"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer24->Add(mPasswords, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 5);

    wxBoxSizer* itemBoxSizer27 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer2->Add(itemBoxSizer27, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 0);

    mOK = new wxButton;
    mOK->Create( itemDialog1, wxID_OK, _("&OK"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer27->Add(mOK, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    mApply = new wxButton;
    mApply->Create( itemDialog1, wxID_APPLY, _("&Apply and Test"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer27->Add(mApply, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxButton* itemButton30 = new wxButton;
    itemButton30->Create( itemDialog1, wxID_CANCEL, _("&Cancel"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer27->Add(itemButton30, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxButton* itemButton31 = new wxButton;
    itemButton31->Create( itemDialog1, wxID_HELP, _("&Help"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer27->Add(itemButton31, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

////@end HTTPAccessDlg content construction
    mHostName->SetMaxLength(70);
    mPath->SetMaxLength(70);
   // show values:
    char buf[70+1];  sig32 numval;
    cd_Get_property_value(cdp, sqlserver_owner, "HostName", 0, buf, sizeof(buf), NULL);
    mHostName->SetValue(wxString(buf, *wxConvCurrent));
    cd_Get_property_value(cdp, sqlserver_owner, "HostPathPrefix", 0, buf, sizeof(buf), NULL);
    mPath->SetValue(wxString(buf, *wxConvCurrent));
    cd_Get_property_num(cdp, sqlserver_owner, "DisableHTTPAnonymous", &numval);
    if (numval)
      { mAnonymous->SetValue(false);   mAuthentificate->SetValue(true); }
    else
      { mAnonymous->SetValue(true);   mAuthentificate->SetValue(false); }
   // enabling:
    cd_Get_property_num(cdp, sqlserver_owner, "WebServerEmulation", &numval);
    mStartEmulation->Enable(!numval);  // button enabled when the property is false, independent of the real emulation status
    cd_Get_server_info(cdp, OP_GI_WEB_RUNNING, &numval, sizeof(numval));
    mEmulated->Enable(numval!=0);      // radiobutton enbled when the emulation runs
    if (!numval)
      {   emulation_mode(false);  mStandalone->SetValue(true);   mEmulated->SetValue(false); }  // cannot select the disabled radio button
    else
    { cd_Get_property_num(cdp, sqlserver_owner, "UsingEmulation", &numval);
      if (numval)
        { emulation_mode(true);   mStandalone->SetValue(false);  mEmulated->SetValue(true); }
      else
        { emulation_mode(false);  mStandalone->SetValue(true);   mEmulated->SetValue(false); }
    }
    mPasswords->Enable(mAuthentificate->GetValue());

    show_url();
    if (!cd_Am_I_config_admin(cdp))
      { mOK->Disable();  mApply->Disable();  mPasswords->Disable(); }
}

wxString HTTPAccessDlg::get_normalized_path(void)
{ wxString path = mPath->GetValue();
  path.Trim(false);  path.Trim(true);
  if (path.Length()>0 && path.Last() != '/') path=path+wxT("/");  // make sure it ends with / unless empty
  if (path.Right(8) == wxT("602.php/")) path=path.Left(path.Length()-8);  // remove 602.php/ when ends with is
  if (path[0] == '/') path=path.Right(path.Length()-1);  // remove starting /
  return path;
}

wxString HTTPAccessDlg::get_host_info(void)
{ wxString host, hostinfo;
  unsigned long portval;
  if (!mPort->GetValue().ToULong(&portval)) 
    portval=80;
  host = mHostName->GetValue();
  host.Trim(false);  host.Trim(true);
  if (!host.IsEmpty())
    if (portval!=80)
      hostinfo.Printf(wxT("%s:%d"), host.c_str(), portval);
    else
      hostinfo=host;
  return hostinfo;
}
  
wxString HTTPAccessDlg::make_URL_pattern(void)
{ wxString url;
  url.Printf(wxT("http://%s/%s602.php/%s/%%s/%%s/%%s"), get_host_info().c_str(), get_normalized_path().c_str(), 
              wxString(cdp->conn_server_name, *wxConvCurrent).c_str());
  return url;
}

bool HTTPAccessDlg::verify(void)
// Common verification for OK and Test operations.
{ wxString port, host;
 // port must be empty or a good number: 
  if (mStandalone->GetValue())
  { unsigned long portval;
    port=mPort->GetValue();
    port.Trim(false);  port.Trim(true);
    if (!port.IsEmpty())
      if (!port.ToULong(&portval) || portval==0 || portval>=65535) 
        { mPort->SetFocus();  error_box(_("Port number is invalid."), this);  return false; }
  }
 // host must not be empty:
  host = mHostName->GetValue();
  host.Trim(false);  host.Trim(true);
  if (host.IsEmpty())
    { mHostName->SetFocus();  error_box(_("Host name is empty."), this);  return false; }
  return true;
}

bool HTTPAccessDlg::save(void)
// Called only when verify() returned true
{ wxString port, host, path, url;
 // using emulation:
  if (mEmulated->IsEnabled())
    if (cd_Set_property_value(cdp, sqlserver_owner, "UsingEmulation", 0, mEmulated->GetValue() ? "1" : "0"))
      { cd_Signalize2(cdp, this);  return false; }
 // port:
  if (mStandalone->GetValue())
  { port=mPort->GetValue();
    port.Trim(false);  port.Trim(true);
    if (cd_Set_property_value(cdp, sqlserver_owner, "ExtWebServerPort", 0, port.mb_str(*wxConvCurrent)))
      { cd_Signalize2(cdp, this);  return false; }
  }
 // host:
  host = mHostName->GetValue();
  host.Trim(false);  host.Trim(true);
  if (cd_Set_property_value(cdp, sqlserver_owner, "HostName", 0, host.mb_str(*wxConvCurrent)))
    { cd_Signalize2(cdp, this);  return false; }
 // path:
  path = get_normalized_path();
  if (cd_Set_property_value(cdp, sqlserver_owner, "HostPathPrefix", 0, path.mb_str(*wxConvCurrent)))
    { cd_Signalize2(cdp, this);  return false; }
 // authentification:
  if (cd_Set_property_value(cdp, sqlserver_owner, "DisableHTTPAnonymous", 0, mAuthentificate->GetValue() ? "1" : "0"))
    { cd_Signalize2(cdp, this);  return false; }
  return true;
}

void HTTPAccessDlg::show_url(void)
{
  wxString patt = make_URL_pattern();
  wxString url;
  url.Printf(patt, wxT("<schema>"), wxT("DAD"), wxT("<DAD object>"));
  mURL->SetLabel(url);
}

/*!
 * Should we show tooltips?
 */

bool HTTPAccessDlg::ShowToolTips()
{
    return TRUE;
}

/*!
 * Get bitmap resources
 */

wxBitmap HTTPAccessDlg::GetBitmapResource( const wxString& name )
{
    // Bitmap retrieval
////@begin HTTPAccessDlg bitmap retrieval
    return wxNullBitmap;
////@end HTTPAccessDlg bitmap retrieval
}

/*!
 * Get icon resources
 */

wxIcon HTTPAccessDlg::GetIconResource( const wxString& name )
{
    // Icon retrieval
////@begin HTTPAccessDlg icon retrieval
    return wxNullIcon;
////@end HTTPAccessDlg icon retrieval
}
/*!
 * wxEVT_COMMAND_TEXT_UPDATED event handler for CD_HTTP_HOSTNAME
 */

void HTTPAccessDlg::OnCdHttpHostnameUpdated( wxCommandEvent& event )
{
    show_url();
    event.Skip();
}

/*!
 * wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for CD_HTTP_ANONYMOUS
 */

void HTTPAccessDlg::OnCdHttpAnonymousSelected( wxCommandEvent& event )
{
    mPasswords->Enable(mAuthentificate->GetValue());
    event.Skip();
}

/*!
 * wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for CD_HTTP_AUTHENTIFICATE
 */

void HTTPAccessDlg::OnCdHttpAuthentificateSelected( wxCommandEvent& event )
{
    mPasswords->Enable(mAuthentificate->GetValue());
    event.Skip();
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_HTTP_PASSWORD
 */

static const char edit_http_passwd_form[] =
// REC_SYNCHRO not set in HDFTSTYLE in order to save the changes when dialog is closed by the X button
"x CURSORVIEW 0 0 468 299 "
"STYLE 32841 HDFTSTYLE 0 _STYLE3 0 FONT 8 238 34 'MS Sans Serif' CAPTION 'Web Passwords' "
"BEGIN "
 "LTEXT 'User name' 3 0 0 164 20 0 0 PANE PAGEHEADER "
 "COMBOBOX 4 0 0 164 68 10551362 1 ACCESS _ username _ COMBOSELECT _ select logname from usertab where category=chr(categ_user) _ "
 "LTEXT 'Web password' 5 164 0 260 20 0 0 PANE PAGEHEADER "
 "EDITTEXT 6 164 0 260 20 65664 1 ACCESS _ webpassword _ "
"END";

#ifdef TRANSLATION_SUPPORT
_("Web Passwords")
_("User name")
_("Web password")
#endif

void HTTPAccessDlg::OnCdHttpPasswordClick( wxCommandEvent& event )
{ 
 // if the passwords table does not exist, create it:
  tobjnum objnum;
  if (cd_Find_prefixed_object(cdp, "_SYSEXT", "HTTP_PASSWORDS", CATEG_TABLE, &objnum))
  { if (cd_SQL_execute(cdp, "CREATE IF NOT EXISTS TABLE _SYSEXT.HTTP_PASSWORDS(USERNAME CHAR(31),WEBPASSWORD CHAR(50))", NULL))
      { cd_Signalize(cdp);  return; }
    cd_SQL_execute(cdp, "REVOKE ALL PRIVILEGES ON _SYSEXT.HTTP_PASSWORDS FROM ROLE _SYSEXT.JUNIOR_USER,ROLE _SYSEXT.SENIOR_USER", NULL);
  }
  DataGridDlg pass_grid_dlg(cdp, _("Web Passwords"), "select * from _sysext.http_passwords", edit_http_passwd_form);
  // parameters must exists until the dialog is closed!
  pass_grid_dlg.Create(this);
  pass_grid_dlg.ShowModal();
}

/*!
 * wxEVT_COMMAND_TEXT_UPDATED event handler for CD_HTTP_PATH
 */

void HTTPAccessDlg::OnCdHttpPathUpdated( wxCommandEvent& event )
{
    show_url();
    event.Skip();
}

/*!
 * wxEVT_COMMAND_TEXT_UPDATED event handler for CD_HTTP_PORT
 */

void HTTPAccessDlg::OnCdHttpPortUpdated( wxCommandEvent& event )
{
    show_url();
    event.Skip();
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_OK
 */

void HTTPAccessDlg::OnOkClick( wxCommandEvent& event )
{
  if (!verify()) return;
  if (!save()) return;
  event.Skip();  // closes the dialog
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CANCEL
 */

void HTTPAccessDlg::OnCancelClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CANCEL in HTTPAccessDlg.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CANCEL in HTTPAccessDlg. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_HELP
 */

void HTTPAccessDlg::OnHelpClick( wxCommandEvent& event )
{
    wxGetApp().frame->help_ctrl.DisplaySection(wxT("xml/html/webinterfconfig.html"));
}


/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_HTTP_SQLHOST
 */

void HTTPAccessDlg::OnCdHttpSqlhostClick( wxCommandEvent& event )     
{ char hostname[70+1];
  if (cd_Get_server_info(cdp, OP_GI_SERVER_HOSTNAME, hostname, sizeof(hostname)))
    cd_Signalize2(cdp, this);
  else 
    mHostName->SetValue(wxString(hostname, *wxConvCurrent));
}

#ifdef WINS
#include <winsock.h>
#else
#include <netdb.h>
typedef int SOCKET;
typedef sockaddr_in SOCKADDR_IN;
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
int WSAGetLastError(void)
{ return errno; }
#define WSAEWOULDBLOCK EINPROGRESS 
#define closesocket(s) close(s)
#endif
/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_APPLY
 */

inline int e64(int c)
{ return c<26 ? 'A'+c : c<52 ? 'a'+(c-26) : c<62 ? '0'+(c-52) : c==62 ? '+' : '/'; }


wxString base64_encode(wxString inp)
{
  wxString res;
  int pos=0, len = (int)inp.Length();
  while (pos < len)
  { unsigned char c1, c2, c3;
    c1 = inp[pos++];
    c2 = pos<len ? inp[pos] : 0;  
    c3 = pos+1<len ? inp[pos+1] : 0;  
    res.Append((char)e64(c1>>2));
    res.Append((char)e64(((c1 & 0x3) << 4) | ((c2 & 0xf0)>>4)));
    if (pos < len)
      res.Append((char)e64(((c2 & 0xf) << 2) | ((c3 & 0xc0)>>6)));
    else
      res.Append('=');
    if (pos+1 < len)
      res.Append((char)e64(c3 & 0x3f));
    else
      res.Append('=');  
    pos+=2;
  }
  return res;
}


void HTTPAccessDlg::OnApplyClick( wxCommandEvent& event )
{ wxString msg, fields;  bool restarted=false;
  if (!verify()) return;
  if (!save()) return;
 // resolve the host name:
  wxString host = mHostName->GetValue();
  host.Trim(false);  host.Trim(true);
  hostent * he = gethostbyname(host.mb_str(*wxConvCurrent));
  if (!he || !he->h_addr_list || !he->h_addr_list[0])
  { msg.Printf(_("Cannot resolve the host name %s."), host.c_str());
    error_box(msg, this);
  }
  else
  {// connecting the host:
    unsigned long portval;
    wxString port=mPort->GetValue();
    port.Trim(false);  port.Trim(true);
    if (port.IsEmpty()) portval=80;
    else port.ToULong(&portval);
   restart:
    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET)	
      error_box(_("Socket creation error"), this);
    else
    { SOCKADDR_IN sin;  
      sin.sin_family = he->h_addrtype;
      sin.sin_port = htons(portval);
      memcpy(&sin.sin_addr, he->h_addr_list[0], he->h_length);
#ifdef LINUX
      long arg;
      arg = fcntl(sock, F_GETFL, NULL);
      arg |= O_NONBLOCK; 
      fcntl(sock, F_SETFL, arg);
#else
      unsigned long arg = 1;
      ioctlsocket(sock, FIONBIO, &arg);
#endif
      int err = connect(sock, (sockaddr*)&sin, sizeof(sin));
      if (err == SOCKET_ERROR)
          if (WSAGetLastError()!=WSAEWOULDBLOCK)
          { msg.Printf(_("Cannot connect host on port %u because of error %d."), portval, WSAGetLastError());
            error_box(msg, this);
          }
          else
          { fd_set mySet;  timeval tv;  
            tv.tv_sec = 5;
            tv.tv_usec = 0;
            FD_ZERO(&mySet);
            FD_SET(sock, &mySet);
            wxBeginBusyCursor();
            if (select(sock+1, NULL, &mySet, NULL, &tv) <= 0/* || !FD_ISSET(sock, &mySet)*/)
            { wxEndBusyCursor();
              msg.Printf(_("Cannot connect host on port %u. No server listens on this port"), portval);
              error_box(msg, this);
            }
            else 
            { err=0;  // PRB: On Linux it connects successfully to a non-existing port number
              wxEndBusyCursor();
            }  
          }
      if (err != SOCKET_ERROR)
      {
#ifdef LINUX
        arg = fcntl(sock, F_GETFL, NULL);  // the [arg] value does not say if connected or not
        arg &= ~O_NONBLOCK; 
        fcntl(sock, F_SETFL, arg);
#else
        arg=0;
        ioctlsocket(sock, FIONBIO, &arg);
#endif
       // prepare ULR:
        wxString request;
        request.Printf(wxT("GET /%s602.php/%s/system/STATUS/* HTTP/1.1\r\nHost: %s\r\n%s\r\n"), 
                       get_normalized_path().c_str(), wxString(cdp->conn_server_name, *wxConvCurrent).c_str(), 
                       get_host_info().c_str(), fields.c_str());    
        if (send(sock, request.mb_str(*wxConvCurrent), (int)strlen(request.mb_str(*wxConvCurrent)), 0) == SOCKET_ERROR)
          error_box(_("Cannot communicate with the web server. The port number may be wrong."), this);          
        else  
        { char buf[5000];  int total = 0;
          do
          { int len = recv(sock, buf+total, sizeof(buf)-total-1, 0);
            if (len==-1) break;
            total+=len;
          } while (false && total < sizeof(buf)-1);
          if (!total)
            error_box(mEmulated->GetValue() ? _("No answer. The web server emulation may be OFF.") : _("No answer from the web server."), this);
          else
          { buf[total]=0;
            int status_code, minor_ver, conv = sscanf(buf, "HTTP/1.%u %u ", &minor_ver, &status_code);
            if (conv!=1)
              error_box(_("Wrong answer from the web server."), this);
            else switch (status_code)
            { case 200:
               // when PHP is not installed, the script is returned:
                if (strstr(buf, "Copyright (c) 2006 Software602, a.s")!=NULL)
                  error_box(_("The web server seems not to have the PHP installed."), this);
                else if (strstr(buf, "Call to undefined function:  wb_get_realm()")!=NULL)
                  error_box(_("602 module is not installed in the PHP on the web server."), this);
                else
                  info_box(_("Connection tested OK."), this);  
                break;
              case 301:  case 302:  
                error_box(_("The web server responded by a redirection information."), this);  break;
              case 404:
	              msg.Printf(_("File '%s602.php' not found on the web server. Check the path and the presence of the file."), get_normalized_path().c_str());
                error_box(msg, this);
                break;
              case 401:
                if (strstr(buf, " Unauthorized") || strstr(buf, " Authorization Required"))
                { HTTPAuthentification dlg(restarted);
                  dlg.Create(this);
                  if (dlg.ShowModal() == wxID_OK)
                  { closesocket(sock);
                    wxString auth = dlg.mName->GetValue() + wxT(":") + dlg.mPassword->GetValue();
                    wxString enc = base64_encode(auth);
                    fields = wxT("Authorization: Basic ")+enc+wxT("\r\n");
                    restarted=true;
                    goto restart;
                  }
                  break; 
                } // cont.
              case 400:
              default:
	              msg.Printf(_("Error %u received from the web server"), status_code);
                error_box(msg, this);
                break;
            }
          }
        }  
      }
      closesocket(sock);  
    }
  }
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_HTTP_START_EMULATION
 */

void HTTPAccessDlg::OnCdHttpStartEmulationClick( wxCommandEvent& event )
{
  ProtocolsPortsDlg dlg(cdp);
  dlg.Create(this);
  dlg.ShowModal();
 // disable the button when emulation switched on:
  sig32 numval;
  cd_Get_property_num(cdp, sqlserver_owner, "WebServerEmulation", &numval);
  mStartEmulation->Enable(!numval);  // button enabled when the property is false, independent of the real emulation status
}


void HTTPAccessDlg::emulation_mode(bool emulating)
{
  mHostNameLabel->SetLabel(emulating ? _("Host name (SQL server):") : _("Host name (WWW server):"));
  mHostSizer->Layout();
  mPort->SetEditable(!emulating);
  char buf[10];
  cd_Get_property_value(cdp, sqlserver_owner, emulating ? "WebPort" : "ExtWebServerPort", 0, buf, sizeof(buf), NULL);
  mPort->SetValue(wxString(buf, *wxConvCurrent));

}

/*!
 * wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for CD_HTTP_EMULATED
 */

void HTTPAccessDlg::OnCdHttpEmulatedSelected( wxCommandEvent& event )
{
  emulation_mode(true);
  event.Skip();
}


/*!
 * wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for CD_HTTP_STANDALONE
 */

void HTTPAccessDlg::OnCdHttpStandaloneSelected( wxCommandEvent& event )
{
  emulation_mode(false);
  event.Skip();
}


