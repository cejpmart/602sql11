/////////////////////////////////////////////////////////////////////////////
// Name:        ProtocolsPortsDlg.cpp
// Purpose:     
// Author:      
// Modified by: 
// Created:     03/30/06 10:23:41
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#if defined(__GNUG__) && !defined(__APPLE__)
//#pragma implementation "ProtocolsPortsDlg.h"
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

#include "ProtocolsPortsDlg.h"

////@begin XPM images
////@end XPM images

/*!
 * ProtocolsPortsDlg type definition
 */

IMPLEMENT_CLASS( ProtocolsPortsDlg, wxDialog )

/*!
 * ProtocolsPortsDlg event table definition
 */

BEGIN_EVENT_TABLE( ProtocolsPortsDlg, wxDialog )

////@begin ProtocolsPortsDlg event table entries
    EVT_BUTTON( wxID_OK, ProtocolsPortsDlg::OnOkClick )

    EVT_BUTTON( wxID_CANCEL, ProtocolsPortsDlg::OnCancelClick )

    EVT_BUTTON( wxID_HELP, ProtocolsPortsDlg::OnHelpClick )

////@end ProtocolsPortsDlg event table entries

END_EVENT_TABLE()

/*!
 * ProtocolsPortsDlg constructors
 */

ProtocolsPortsDlg::ProtocolsPortsDlg(cdp_t cdpIn)
{ cdp=cdpIn;
}

ProtocolsPortsDlg::ProtocolsPortsDlg( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
    Create(parent, id, caption, pos, size, style);
}

/*!
 * ProtocolsPortsDlg creator
 */

bool ProtocolsPortsDlg::Create( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
////@begin ProtocolsPortsDlg member initialisation
    mTopSizer = NULL;
    mIPC = NULL;
    mIPCRuns = NULL;
    mTCPIP = NULL;
    mNetRuns = NULL;
    mPort = NULL;
    mTunnel = NULL;
    mTunnelRuns = NULL;
    mTunnelPort = NULL;
    mWebSizer1 = NULL;
    mWeb = NULL;
    mWebRuns = NULL;
    mWebSizer2 = NULL;
    mWebPortCapt = NULL;
    mWebPort = NULL;
    mOK = NULL;
////@end ProtocolsPortsDlg member initialisation

////@begin ProtocolsPortsDlg creation
    SetExtraStyle(/*GetExtraStyle()|*/wxWS_EX_BLOCK_EVENTS);
    wxDialog::Create( parent, id, caption, pos, size, style );

    CreateControls();
    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);
    Centre();
////@end ProtocolsPortsDlg creation
    return TRUE;
}

/*!
 * Control creation for ProtocolsPortsDlg
 */

void ProtocolsPortsDlg::CreateControls()
{    
////@begin ProtocolsPortsDlg content construction
    ProtocolsPortsDlg* itemDialog1 = this;

    mTopSizer = new wxBoxSizer(wxVERTICAL);
    itemDialog1->SetSizer(mTopSizer);
    itemDialog1->SetAutoLayout(TRUE);

    wxStaticText* itemStaticText3 = new wxStaticText;
    itemStaticText3->Create( itemDialog1, wxID_STATIC, _("Communication protocols enabled on the SQL server:"), wxDefaultPosition, wxDefaultSize, 0 );
    mTopSizer->Add(itemStaticText3, 0, wxALIGN_LEFT|wxALL|wxADJUST_MINSIZE, 5);

    wxBoxSizer* itemBoxSizer4 = new wxBoxSizer(wxHORIZONTAL);
    mTopSizer->Add(itemBoxSizer4, 0, wxGROW|wxALL, 0);

    mIPC = new wxCheckBox;
    mIPC->Create( itemDialog1, CD_PP_IPC, _("Direct interprocess communication"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    mIPC->SetValue(FALSE);
    if (ShowToolTips())
        mIPC->SetToolTip(_("This protocol may be used only on Windows by clients running on the same computer as the SQL server."));
    itemBoxSizer4->Add(mIPC, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    itemBoxSizer4->Add(5, 5, 1, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    mIPCRuns = new wxStaticText;
    mIPCRuns->Create( itemDialog1, CD_PP_IPC_RUNS, _("(Currently running)"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer4->Add(mIPCRuns, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    wxBoxSizer* itemBoxSizer8 = new wxBoxSizer(wxHORIZONTAL);
    mTopSizer->Add(itemBoxSizer8, 0, wxGROW|wxALL, 0);

    mTCPIP = new wxCheckBox;
    mTCPIP->Create( itemDialog1, CD_PP_TCPIP, _("Network TCP/IP protocol"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    mTCPIP->SetValue(FALSE);
    if (ShowToolTips())
        mTCPIP->SetToolTip(_("The main communication protocol between the SQL server and its clients."));
    itemBoxSizer8->Add(mTCPIP, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxTOP, 5);

    itemBoxSizer8->Add(5, 5, 1, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    mNetRuns = new wxStaticText;
    mNetRuns->Create( itemDialog1, CD_PP_NET_RUNS, _("(Currently running)"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer8->Add(mNetRuns, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxTOP|wxADJUST_MINSIZE, 5);

    wxBoxSizer* itemBoxSizer12 = new wxBoxSizer(wxHORIZONTAL);
    mTopSizer->Add(itemBoxSizer12, 0, wxGROW|wxALL, 0);

    itemBoxSizer12->Add(5, 5, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    itemBoxSizer12->Add(5, 5, 1, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxStaticText* itemStaticText15 = new wxStaticText;
    itemStaticText15->Create( itemDialog1, wxID_STATIC, _("Port number base (default id 5001):"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer12->Add(itemStaticText15, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxTOP|wxBOTTOM|wxADJUST_MINSIZE, 5);

    mPort = new wxTextCtrl;
    mPort->Create( itemDialog1, CD_PP_PORT, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer12->Add(mPort, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxBoxSizer* itemBoxSizer17 = new wxBoxSizer(wxHORIZONTAL);
    mTopSizer->Add(itemBoxSizer17, 0, wxGROW|wxALL, 0);

    mTunnel = new wxCheckBox;
    mTunnel->Create( itemDialog1, CD_PP_TUNNEL, _("HTTP tunnelling of the network protocol"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    mTunnel->SetValue(FALSE);
    if (ShowToolTips())
        mTunnel->SetToolTip(_("Encapsulates the network communication into the standard HTTP protocol."));
    itemBoxSizer17->Add(mTunnel, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxTOP, 5);

    itemBoxSizer17->Add(5, 5, 1, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    mTunnelRuns = new wxStaticText;
    mTunnelRuns->Create( itemDialog1, CD_PP_TUNNEL_RUNS, _("(Currently running)"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer17->Add(mTunnelRuns, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxTOP|wxADJUST_MINSIZE, 5);

    wxBoxSizer* itemBoxSizer21 = new wxBoxSizer(wxHORIZONTAL);
    mTopSizer->Add(itemBoxSizer21, 0, wxGROW|wxALL, 0);

    itemBoxSizer21->Add(5, 5, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    itemBoxSizer21->Add(5, 5, 1, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxStaticText* itemStaticText24 = new wxStaticText;
    itemStaticText24->Create( itemDialog1, wxID_ANY, _("HTTP tunnelling port number (default is 80]:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer21->Add(itemStaticText24, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxTOP|wxBOTTOM|wxADJUST_MINSIZE, 5);

    mTunnelPort = new wxTextCtrl;
    mTunnelPort->Create( itemDialog1, CD_PP_TUNNEL_PORT, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer21->Add(mTunnelPort, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    mWebSizer1 = new wxBoxSizer(wxHORIZONTAL);
    mTopSizer->Add(mWebSizer1, 0, wxGROW|wxALL, 0);

    mWeb = new wxCheckBox;
    mWeb->Create( itemDialog1, CD_PP_WEB, _("Web server emulation"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    mWeb->SetValue(FALSE);
    if (ShowToolTips())
        mWeb->SetToolTip(_("When checked the SQL server provides some elementary web services."));
    mWebSizer1->Add(mWeb, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxTOP, 5);

    mWebSizer1->Add(5, 5, 1, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    mWebRuns = new wxStaticText;
    mWebRuns->Create( itemDialog1, CD_PP_WEB_RUNS, _("(Currently running)"), wxDefaultPosition, wxDefaultSize, 0 );
    mWebSizer1->Add(mWebRuns, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxTOP|wxADJUST_MINSIZE, 5);

    mWebSizer2 = new wxBoxSizer(wxHORIZONTAL);
    mTopSizer->Add(mWebSizer2, 0, wxGROW|wxALL, 0);

    mWebSizer2->Add(5, 5, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    mWebSizer2->Add(5, 5, 1, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    mWebPortCapt = new wxStaticText;
    mWebPortCapt->Create( itemDialog1, ID_STATICTEXT, _("Web port number (default is 80]:"), wxDefaultPosition, wxDefaultSize, 0 );
    mWebSizer2->Add(mWebPortCapt, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxTOP|wxBOTTOM|wxADJUST_MINSIZE, 5);

    mWebPort = new wxTextCtrl;
    mWebPort->Create( itemDialog1, CD_PP_WEB_PORT, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    mWebSizer2->Add(mWebPort, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxBoxSizer* itemBoxSizer35 = new wxBoxSizer(wxHORIZONTAL);
    mTopSizer->Add(itemBoxSizer35, 0, wxALIGN_CENTER_HORIZONTAL|wxLEFT|wxRIGHT|wxTOP, 5);

    mOK = new wxButton;
    mOK->Create( itemDialog1, wxID_OK, _("&OK"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer35->Add(mOK, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxButton* itemButton37 = new wxButton;
    itemButton37->Create( itemDialog1, wxID_CANCEL, _("&Cancel"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer35->Add(itemButton37, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxButton* itemButton38 = new wxButton;
    itemButton38->Create( itemDialog1, wxID_HELP, _("&Help"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer35->Add(itemButton38, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);
////@end ProtocolsPortsDlg content construction
    char buf[20];
#ifdef WINS
    cd_Get_property_value(cdp, sqlserver_owner, "IPCCommunication", 0, buf, sizeof(buf), NULL);
    ipc = *buf=='1';
#else  // LINUX                       
    ipc=false;
    mIPC->Disable();
#endif
    cd_Get_property_value(cdp, sqlserver_owner, "TCPIPCommunication", 0, buf, sizeof(buf), NULL);
    tcpip = *buf=='1';
    cd_Get_property_value(cdp, sqlserver_owner, "HTTPTunnel", 0, buf, sizeof(buf), NULL);
    tunnel = *buf=='1';
    cd_Get_property_value(cdp, sqlserver_owner, "WebServerEmulation", 0, buf, sizeof(buf), NULL);
    web = *buf=='1';
    cd_Get_property_value(cdp, sqlserver_owner, "IPPort", 0, ip_port, sizeof(ip_port), NULL);
    cd_Get_property_value(cdp, sqlserver_owner, "HTTPTunnelPort", 0, tunnel_port, sizeof(tunnel_port), NULL);
    cd_Get_property_value(cdp, sqlserver_owner, "WebPort", 0, web_port, sizeof(web_port), NULL);

    mPort      ->SetMaxLength(sizeof(ip_port    )-1);
    mTunnelPort->SetMaxLength(sizeof(tunnel_port)-1);
    mWebPort   ->SetMaxLength(sizeof(web_port   )-1);

    mIPC   ->SetValue(ipc);
    mTCPIP ->SetValue(tcpip);
    mTunnel->SetValue(tunnel);
    mWeb   ->SetValue(web);
    mPort      ->SetValue(wxString(ip_port,     *wxConvCurrent));
    mTunnelPort->SetValue(wxString(tunnel_port, *wxConvCurrent));
    mWebPort   ->SetValue(wxString(web_port,    *wxConvCurrent));
   // run state:
    sig32 val;
    cd_Get_server_info(cdp, OP_GI_IPC_RUNNING,  &val, sizeof(val));
    if (!val) mIPCRuns->SetLabel(_("(Currently NOT running)"));
    cd_Get_server_info(cdp, OP_GI_NET_RUNNING,  &val, sizeof(val));
    if (!val) mNetRuns->SetLabel(_("(Currently NOT running)"));
    cd_Get_server_info(cdp, OP_GI_HTTP_RUNNING, &val, sizeof(val));
    if (!val) mTunnelRuns->SetLabel(_("(Currently NOT running)"));
    cd_Get_server_info(cdp, OP_GI_WEB_RUNNING,  &val, sizeof(val));
    if (!val) mWebRuns->SetLabel(_("(Currently NOT running)"));
#ifndef XMLFORMS  // hiding the windows is not sufficient, must call sizer's Hide()!
    mWeb->Hide();  mWebRuns->Hide();  mWebPortCapt->Hide();  mWebPort->Hide();
    mTopSizer->Show(mWebSizer1, false, true);
    mTopSizer->Show(mWebSizer2, false, true);
    Layout();
#endif
    if (!cd_Am_I_config_admin(cdp))
      mOK->Disable();
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_OK
 */

void ProtocolsPortsDlg::OnOkClick( wxCommandEvent& event )
{ 
  bool bigchange = false;
  char buf[20];  unsigned long numval;  bool bval;
 // validate:
  if (!mPort->GetValue().IsEmpty() && (!mPort->GetValue().ToULong(&numval) || numval<=0 || numval>65535))
    { mPort->SetFocus();  wxBell();  return; }
  if (!mTunnelPort->GetValue().IsEmpty() && (!mTunnelPort->GetValue().ToULong(&numval) || numval<=0 || numval>65535))
    { mTunnelPort->SetFocus();  wxBell();  return; }
  if (!mWebPort->GetValue().IsEmpty() && (!mWebPort->GetValue().ToULong(&numval) || numval<=0 || numval>65535))
    { mWebPort->SetFocus();  wxBell();  return; }
 // save (must not write the unchanged values, preserving the server's defaults):
#ifdef WINS
  bval = mIPC->GetValue();
  if (bval!=ipc)
    if (cd_Set_property_value(cdp, sqlserver_owner, "IPCCommunication", 0, bval ? "1" : "0"))
      { cd_Signalize2(cdp, this);  return; }
    else bigchange=true;
#endif
  bval = mTCPIP->GetValue();
  if (bval!=tcpip)
    if (cd_Set_property_value(cdp, sqlserver_owner, "TCPIPCommunication", 0, bval ? "1" : "0"))
      { cd_Signalize2(cdp, this);  return; }
    else bigchange=true;
  bval = mTunnel->GetValue();
  if (bval!=tunnel)
    if (cd_Set_property_value(cdp, sqlserver_owner, "HTTPTunnel", 0, bval ? "1" : "0"))
      { cd_Signalize2(cdp, this);  return; }
    else bigchange=true;
  bval = mWeb->GetValue();
  if (bval!=web)
    if (cd_Set_property_value(cdp, sqlserver_owner, "WebServerEmulation", 0, bval ? "1" : "0"))
      { cd_Signalize2(cdp, this);  return; }
    else bigchange=true;

  strcpy(buf, mPort->GetValue().mb_str(*wxConvCurrent));
  if (strcmp(buf, ip_port))
    if (cd_Set_property_value(cdp, sqlserver_owner, "IPPort", 0, buf))
      { cd_Signalize2(cdp, this);  return; }
    else bigchange=true;
  strcpy(buf, mTunnelPort->GetValue().mb_str(*wxConvCurrent));
  if (strcmp(buf, tunnel_port))
    if (cd_Set_property_value(cdp, sqlserver_owner, "HTTPTunnelPort", 0, buf))
      { cd_Signalize2(cdp, this);  return; }
    else bigchange=true;
  strcpy(buf, mWebPort->GetValue().mb_str(*wxConvCurrent));
  if (strcmp(buf, web_port))
    if (cd_Set_property_value(cdp, sqlserver_owner, "WebPort", 0, buf))
      { cd_Signalize2(cdp, this);  return; }
    else bigchange=true;

  if (bigchange)
    info_box(_("The changes will be effective after the SQL server is restarted."), this);
  event.Skip();
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CANCEL
 */

void ProtocolsPortsDlg::OnCancelClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CANCEL in ProtocolsPortsDlg.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CANCEL in ProtocolsPortsDlg. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_HELP
 */

void ProtocolsPortsDlg::OnHelpClick( wxCommandEvent& event )
{
    wxGetApp().frame->help_ctrl.DisplaySection(wxT("xml/html/protocols_ports.html"));
}

/*!
 * Should we show tooltips?
 */

bool ProtocolsPortsDlg::ShowToolTips()
{
    return TRUE;
}

/*!
 * Get bitmap resources
 */

wxBitmap ProtocolsPortsDlg::GetBitmapResource( const wxString& name )
{
    // Bitmap retrieval
////@begin ProtocolsPortsDlg bitmap retrieval
    return wxNullBitmap;
////@end ProtocolsPortsDlg bitmap retrieval
}

/*!
 * Get icon resources
 */

wxIcon ProtocolsPortsDlg::GetIconResource( const wxString& name )
{
    // Icon retrieval
////@begin ProtocolsPortsDlg icon retrieval
    return wxNullIcon;
////@end ProtocolsPortsDlg icon retrieval
}
