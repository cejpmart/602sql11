/////////////////////////////////////////////////////////////////////////////
// Name:        NetAccDlg.cpp
// Purpose:     
// Author:      
// Modified by: 
// Created:     02/14/04 12:19:04
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifdef __GNUG__
//#pragma implementation "NetAccDlg.h"
#endif

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

////@begin includes
////@end includes

#include "NetAccDlg.h"
#include "ShowTextDlg.h"
////@begin XPM images
////@end XPM images

/*!
 * NetAccDlg type definition
 */

IMPLEMENT_CLASS( NetAccDlg, wxDialog )

/*!
 * NetAccDlg event table definition
 */

BEGIN_EVENT_TABLE( NetAccDlg, wxDialog )

////@begin NetAccDlg event table entries
    EVT_RADIOBUTTON( CD_NETACC_LOCAL, NetAccDlg::OnCdNetaccLocalSelected )

    EVT_RADIOBUTTON( CD_NETACC_NETWORK, NetAccDlg::OnCdNetaccNetworkSelected )

    EVT_BUTTON( CD_NETACC_PING, NetAccDlg::OnCdNetaccPingClick )

    EVT_CHECKBOX( CD_NETACC_VIA_FW, NetAccDlg::OnCdNetaccViaFwClick )

    EVT_BUTTON( CD_NETACC_SHOW_CERT, NetAccDlg::OnCdNetaccShowCertClick )

    EVT_BUTTON( CD_NETACC_ERASE_CERT, NetAccDlg::OnCdNetaccEraseCertClick )

    EVT_BUTTON( wxID_OK, NetAccDlg::OnOkClick )

    EVT_BUTTON( wxID_HELP, NetAccDlg::OnHelpClick )

////@end NetAccDlg event table entries

END_EVENT_TABLE()

/*!
 * NetAccDlg constructors
 */

NetAccDlg::NetAccDlg( )
{
}

NetAccDlg::NetAccDlg( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
    Create(parent, id, caption, pos, size, style);
}

/*!
 * NetAccDlg creator
 */

bool NetAccDlg::Create( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
////@begin NetAccDlg member initialisation
    mPort = NULL;
    mLocal = NULL;
    mNetwork = NULL;
    mAddr = NULL;
    mPing = NULL;
    mTunnel = NULL;
    mViaFW = NULL;
    mFWAddr = NULL;
    mCertMsg = NULL;
    mCertShow = NULL;
    mCertErase = NULL;
////@end NetAccDlg member initialisation

////@begin NetAccDlg creation
    SetExtraStyle(GetExtraStyle()|wxWS_EX_BLOCK_EVENTS);
    wxDialog::Create( parent, id, caption, pos, size, style );

    CreateControls();
    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);
    Centre();
////@end NetAccDlg creation
    return TRUE;
}

/*!
 * Control creation for NetAccDlg
 */

void NetAccDlg::CreateControls()
{    
////@begin NetAccDlg content construction
    NetAccDlg* itemDialog1 = this;

    wxBoxSizer* itemBoxSizer2 = new wxBoxSizer(wxVERTICAL);
    itemDialog1->SetSizer(itemBoxSizer2);

    wxStaticBox* itemStaticBoxSizer3Static = new wxStaticBox(itemDialog1, wxID_ANY, _("Address of the SQL server:"));
    wxStaticBoxSizer* itemStaticBoxSizer3 = new wxStaticBoxSizer(itemStaticBoxSizer3Static, wxVERTICAL);
    itemBoxSizer2->Add(itemStaticBoxSizer3, 0, wxGROW|wxALL, 5);

    wxBoxSizer* itemBoxSizer4 = new wxBoxSizer(wxHORIZONTAL);
    itemStaticBoxSizer3->Add(itemBoxSizer4, 0, wxGROW|wxALL, 0);

    wxStaticText* itemStaticText5 = new wxStaticText;
    itemStaticText5->Create( itemDialog1, wxID_STATIC, _("Port &number (default is 5001):"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer4->Add(itemStaticText5, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    mPort = new wxTextCtrl;
    mPort->Create( itemDialog1, CD_NETACC_PORT, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer4->Add(mPort, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    mLocal = new wxRadioButton;
    mLocal->Create( itemDialog1, CD_NETACC_LOCAL, _("Server runs &locally (on the same computer as client)"), wxDefaultPosition, wxDefaultSize, wxRB_GROUP );
    mLocal->SetValue(FALSE);
    itemStaticBoxSizer3->Add(mLocal, 0, wxALIGN_LEFT|wxALL, 5);

    mNetwork = new wxRadioButton;
    mNetwork->Create( itemDialog1, CD_NETACC_NETWORK, _("Server runs on the &host with specified network address or name:"), wxDefaultPosition, wxDefaultSize, 0 );
    mNetwork->SetValue(FALSE);
    itemStaticBoxSizer3->Add(mNetwork, 0, wxALIGN_LEFT|wxALL, 5);

    wxBoxSizer* itemBoxSizer9 = new wxBoxSizer(wxHORIZONTAL);
    itemStaticBoxSizer3->Add(itemBoxSizer9, 0, wxGROW|wxALL, 0);

    itemBoxSizer9->Add(5, 5, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    mAddr = new wxTextCtrl;
    mAddr->Create( itemDialog1, CD_NETACC_ADDR, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer9->Add(mAddr, 1, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    mPing = new wxButton;
    mPing->Create( itemDialog1, CD_NETACC_PING, _("&Ping"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer9->Add(mPing, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    mTunnel = new wxCheckBox;
    mTunnel->Create( itemDialog1, CD_NETACC_TUNNEL, _("Connect through HTTP"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    mTunnel->SetValue(FALSE);
    if (ShowToolTips())
        mTunnel->SetToolTip(_("Check if the SQL Server allows HTTP connections (usually port 80)"));
    itemStaticBoxSizer3->Add(mTunnel, 0, wxGROW|wxALL, 5);

    wxStaticBox* itemStaticBoxSizer14Static = new wxStaticBox(itemDialog1, wxID_ANY, _("SOCKS server"));
    wxStaticBoxSizer* itemStaticBoxSizer14 = new wxStaticBoxSizer(itemStaticBoxSizer14Static, wxVERTICAL);
    itemBoxSizer2->Add(itemStaticBoxSizer14, 0, wxGROW|wxALL, 5);

    mViaFW = new wxCheckBox;
    mViaFW->Create( itemDialog1, CD_NETACC_VIA_FW, _("Connect &through the Socks server:"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    mViaFW->SetValue(FALSE);
    itemStaticBoxSizer14->Add(mViaFW, 0, wxALIGN_LEFT|wxALL, 5);

    wxBoxSizer* itemBoxSizer16 = new wxBoxSizer(wxHORIZONTAL);
    itemStaticBoxSizer14->Add(itemBoxSizer16, 0, wxGROW|wxALL, 0);

    itemBoxSizer16->Add(5, 5, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    mFWAddr = new wxTextCtrl;
    mFWAddr->Create( itemDialog1, CD_NETACC_FW_ADDR, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer16->Add(mFWAddr, 1, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxStaticBox* itemStaticBoxSizer19Static = new wxStaticBox(itemDialog1, wxID_ANY, _("Server certificate"));
    wxStaticBoxSizer* itemStaticBoxSizer19 = new wxStaticBoxSizer(itemStaticBoxSizer19Static, wxHORIZONTAL);
    itemBoxSizer2->Add(itemStaticBoxSizer19, 1, wxGROW|wxALL, 5);

    mCertMsg = new wxStaticText;
    mCertMsg->Create( itemDialog1, CD_NETACC_CERT_MSG, _("Server certificate has been acquired by the client.\nView the certificate to verify the identity of the server.\nRemove the certificate if the server has a new certificate \nand the client cannot connect."), wxDefaultPosition, wxDefaultSize, 0 );
    itemStaticBoxSizer19->Add(mCertMsg, 1, wxGROW|wxLEFT|wxRIGHT|wxADJUST_MINSIZE, 5);

    wxBoxSizer* itemBoxSizer21 = new wxBoxSizer(wxVERTICAL);
    itemStaticBoxSizer19->Add(itemBoxSizer21, 0, wxALIGN_TOP|wxALL, 0);

    mCertShow = new wxButton;
    mCertShow->Create( itemDialog1, CD_NETACC_SHOW_CERT, _("&Show"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer21->Add(mCertShow, 0, wxGROW|wxLEFT|wxRIGHT|wxBOTTOM, 5);

    mCertErase = new wxButton;
    mCertErase->Create( itemDialog1, CD_NETACC_ERASE_CERT, _("&Erase"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer21->Add(mCertErase, 0, wxGROW|wxLEFT|wxRIGHT|wxBOTTOM, 5);

    wxStaticText* itemStaticText24 = new wxStaticText;
    itemStaticText24->Create( itemDialog1, wxID_STATIC, _("Only servers created/registered by the administrator are visible to all other users."), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer2->Add(itemStaticText24, 0, wxALIGN_LEFT|wxALL|wxADJUST_MINSIZE, 5);

    wxBoxSizer* itemBoxSizer25 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer2->Add(itemBoxSizer25, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 0);

    wxButton* itemButton26 = new wxButton;
    itemButton26->Create( itemDialog1, wxID_OK, _("&OK"), wxDefaultPosition, wxDefaultSize, 0 );
    itemButton26->SetDefault();
    itemBoxSizer25->Add(itemButton26, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxButton* itemButton27 = new wxButton;
    itemButton27->Create( itemDialog1, wxID_CANCEL, _("&Cancel"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer25->Add(itemButton27, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxButton* itemButton28 = new wxButton;
    itemButton28->Create( itemDialog1, wxID_HELP, _("Help"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer25->Add(itemButton28, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    // Set validators
    mPort->SetValidator( wxTextValidator(wxFILTER_NUMERIC, & server_port) );
    mAddr->SetValidator( wxTextValidator(wxFILTER_NONE, & server_address) );
    mTunnel->SetValidator( wxGenericValidator(& via_tunnel) );
    mViaFW->SetValidator( wxGenericValidator(& via_fw) );
    mFWAddr->SetValidator( wxTextValidator(wxFILTER_NONE, & socks_address) );
////@end NetAccDlg content construction
  wxString title;
  title.Printf(_("Server Connection Data for \'%s\'"), server_name.c_str());
  SetTitle(title);
  enabling();
  mFWAddr->Enable(via_fw);
  mAddr->Enable(!is_local);  mPing->Enable(!is_local);  mViaFW->Enable(!is_local);
  if (!is_local) mNetwork->SetValue(true);  // not bound with a variable
  else mLocal->SetValue(true);  // binding with generic bool validator does not work in 2.5.1
}

/*!
 * Should we show tooltips?
 */

bool NetAccDlg::ShowToolTips()
{
    return TRUE;
}

void NetAccDlg::enabling(void)
// Shows the certificate state and controls.
// Chnaging state for NOT HAVING to HAVING not supported.
{ unsigned length;
  unsigned char * cert_data = load_certificate_data(server_name.mb_str(wxConvLocal), &length);
  mCertShow ->Enable(cert_data!=NULL);
  mCertErase->Enable(cert_data!=NULL);
  if (cert_data==NULL)
    mCertMsg->SetLabel(_("The client has not acquired the server certificate."));
  if (cert_data) corefree(cert_data);
}

/*!
 * wxEVT_COMMAND_CHECKBOX_CLICKED event handler for CD_NETACC_VIA_FW
 */

void NetAccDlg::OnCdNetaccViaFwClick( wxCommandEvent& event )
{ mFWAddr->Enable(mViaFW->GetValue()); 
  event.Skip();
}


/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_NETACC_SHOW_CERT
 */

//#define CERT_VIEW_CMD "/usr/lib/602sql9/keytool -printcert -file %s"  // problems with java library and versions
#define CERT_VIEW_CMD "openssl x509 -inform DER -text -certopt no_header,no_version,no_signame,no_extensions,no_pubkey,no_sigdump -noout -fingerprint -in %s"
//#define CERT_VIEW_CMD "openssl x509 -inform DER -subject -issuer -serial -dates -fingerprint -noout -in %s"  -- less nice
 
void NetAccDlg::OnCdNetaccShowCertClick( wxCommandEvent& event )
{ 
  unsigned length;
  unsigned char * certdata = load_certificate_data(server_name.mb_str(wxConvLocal), &length);
  if (!certdata)
    { error_box(_("Cannot read the certificate."));  return; }
#ifdef WINS
  wxChar path[MAX_PATH];  DWORD wr;
  GetTempPath(sizeof(path)/sizeof(wxChar), path);  _tcscat(path, server_name);  _tcscat(path, wxT(".cer"));
  FHANDLE hnd = CreateFile(path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, 0);
  if (hnd==INVALID_FHANDLE_VALUE) { corefree(certdata);  return; }
  WriteFile(hnd, certdata, length, &wr, NULL);
  CloseHandle(hnd);
  corefree(certdata);
  SHELLEXECUTEINFO sei;  memset(&sei, 0, sizeof(sei));
  sei.cbSize=sizeof(sei);
  sei.fMask=SEE_MASK_NOCLOSEPROCESS;
  sei.hwnd=(HWND)GetHandle();
  sei.lpVerb=wxT("open");
  sei.lpFile=path;
  sei.nShow=SW_SHOW;
  if (ShellExecuteEx(&sei))
  { WaitForSingleObject(sei.hProcess, INFINITE);
    CloseHandle(sei.hProcess);
  }
  else wxBell();
  DeleteFile(path);
#else
  char cert_info[10000];
  { FILE * pipein_fp;
	int temp_cert_fd;
	char temp_cert_filename[] = "/tmp/temp_cert_file.XXXXXX";
	wxBusyCursor wait;
	/* Create temporary file */
	if ( (temp_cert_fd = mkstemp(temp_cert_filename)) == -1)
	{
		//perror("mkstemp");
        corefree(certdata);
		return;
	}
		
	write(temp_cert_fd, certdata, length);
	corefree(certdata);
	
    char shell_cmd[1000];
	sprintf(shell_cmd, CERT_VIEW_CMD, temp_cert_filename);
    
	
	/* Create one way pipe line with call to popen() */
	if ((pipein_fp = popen(shell_cmd, "r")) == NULL)
	{
		//perror("popen");
		return;
	}
	
	int len = fread(cert_info, sizeof(char), sizeof(cert_info)-1, pipein_fp);
	cert_info[len]=0;
	/* Close the streams */
	pclose(pipein_fp);
	unlink(temp_cert_filename);
	close(temp_cert_fd);
  }
  ShowTextDlg dlg(this, -1, _("Certificate summary"));    
  dlg.mText->SetValue(wxString(cert_info, *wxConvCurrent));
  dlg.ShowModal();
  //info_box(wxString(cert_info, *wxConvCurrent), this);
#endif
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_NETACC_ERASE_CERT
 */

void NetAccDlg::OnCdNetaccEraseCertClick( wxCommandEvent& event )
{ erase_certificate(server_name.mb_str(wxConvLocal));
  enabling();
}


/*!
 * wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for CD_NETACC_LOCAL
 */

void NetAccDlg::OnCdNetaccLocalSelected( wxCommandEvent& event )
{ is_local=mLocal->GetValue();
  mAddr->Enable(!is_local);  mPing->Enable(!is_local);  mViaFW->Enable(!is_local);
  if (is_local) mViaFW->SetValue(false);
  event.Skip();
}


/*!
 * wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for CD_NETACC_NETWORK
 */

void NetAccDlg::OnCdNetaccNetworkSelected( wxCommandEvent& event )
{ is_local=mLocal->GetValue();
  mAddr->Enable(!is_local);  mPing->Enable(!is_local);  mViaFW->Enable(!is_local);
  event.Skip();
}

#define IP_TEST_CAPTION  _("TCP/IP connection test result (ping)")
#define IP_TEST_OK               _("The TCP/IP connection to the server is OK.")
#define IP_TEST_VIA_FW           _("Cannot check the connection. The target server is protected by a firewall.")
#define IP_TEST_NO_ADDR          _("The IP address of the server is not specified. Enter the address in the Connection Data dialog.")
#define IP_TEST_NOT_INIT         _("Cannot initialize TCP/IP. It may not be installed properly.")
#define IP_TEST_TIMEOUT          _("The TCP/IP connection is not working. Check the IP address.")
#define IP_TEST_ERROR            _("Error %d when using the IP protocol")

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_NETACC_PING
 */

void NetAccDlg::OnCdNetaccPingClick( wxCommandEvent& event )
{ int res;
  if (mViaFW->GetValue()) res=200;
  else  // Tries to ping the specified server and reports the result
  { wxBusyCursor();
    res=tcpip_echo_addr(mAddr->GetValue().mb_str(wxConvLocal));
  }
  wxString msg;
  switch (res)
  { case 0:            msg=IP_TEST_OK;       break;
    case 200:          msg=IP_TEST_VIA_FW;   break;
    case 201:          msg=IP_TEST_NO_ADDR;  break;
    case 202:          msg=IP_TEST_NOT_INIT; break;
    case 11:  case 9:  case 20:
                       msg=IP_TEST_TIMEOUT;  break;
    default:           msg.Printf(IP_TEST_ERROR, res);  break;
  }
  wxMessageBox(msg, IP_TEST_CAPTION);
}


/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_HELP
 */

void NetAccDlg::OnHelpClick( wxCommandEvent& event )
{
  wxGetApp().frame->help_ctrl.DisplaySection(wxT("xml/html/dlgconn_data.html"));
}


/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_OK
 */

void NetAccDlg::OnOkClick( wxCommandEvent& event )
{ unsigned long val;
  if (!mPort->GetValue().ToULong(&val) || val>65535) val=5001;
  if (is_local && !is_port_unique(val, server_name.mb_str(*wxConvCurrent)))
    if (!yesno_box(_("There is another local server with the same port number.\nThese servers will be inaccessible when running simultaneously.\nDo you want to continue anyway?"), this)) 
      return;
  event.Skip();
}



/*!
 * Get bitmap resources
 */

wxBitmap NetAccDlg::GetBitmapResource( const wxString& name )
{
    // Bitmap retrieval
////@begin NetAccDlg bitmap retrieval
    return wxNullBitmap;
////@end NetAccDlg bitmap retrieval
}

/*!
 * Get icon resources
 */

wxIcon NetAccDlg::GetIconResource( const wxString& name )
{
    // Icon retrieval
////@begin NetAccDlg icon retrieval
    return wxNullIcon;
////@end NetAccDlg icon retrieval
}


