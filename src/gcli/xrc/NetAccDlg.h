/////////////////////////////////////////////////////////////////////////////
// Name:        NetAccDlg.h
// Purpose:     
// Author:      
// Modified by: 
// Created:     02/14/04 12:19:04
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifndef _NETACCDLG_H_
#define _NETACCDLG_H_

#ifdef __GNUG__
//#pragma interface "NetAccDlg.cpp"
#endif

/*!
 * Includes
 */

////@begin includes
#include "wx/valtext.h"
#include "wx/valgen.h"
////@end includes

/*!
 * Forward declarations
 */

////@begin forward declarations
////@end forward declarations

/*!
 * Control identifiers
 */

////@begin control identifiers
#define ID_DIALOG 10000
#define SYMBOL_NETACCDLG_STYLE wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU|wxCLOSE_BOX
#define SYMBOL_NETACCDLG_TITLE _("Network access to the SQL server")
#define SYMBOL_NETACCDLG_IDNAME ID_DIALOG
#define SYMBOL_NETACCDLG_SIZE wxSize(400, 300)
#define SYMBOL_NETACCDLG_POSITION wxDefaultPosition
#define CD_NETACC_PORT 10005
#define CD_NETACC_LOCAL 10001
#define CD_NETACC_NETWORK 10008
#define CD_NETACC_ADDR 10002
#define CD_NETACC_PING 10009
#define CD_NETACC_TUNNEL 10073
#define CD_NETACC_VIA_FW 10003
#define CD_NETACC_FW_ADDR 10004
#define CD_NETACC_CERT_MSG 10010
#define CD_NETACC_SHOW_CERT 10006
#define CD_NETACC_ERASE_CERT 10007
////@end control identifiers

/*!
 * Compatibility
 */

#ifndef wxCLOSE_BOX
#define wxCLOSE_BOX 0x1000
#endif

/*!
 * NetAccDlg class declaration
 */

class NetAccDlg: public wxDialog
{    
    DECLARE_CLASS( NetAccDlg )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    NetAccDlg( );
    NetAccDlg( wxWindow* parent, wxWindowID id = -1, const wxString& caption = SYMBOL_NETACCDLG_TITLE, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = SYMBOL_NETACCDLG_STYLE );

    /// Creation
    bool Create( wxWindow* parent, wxWindowID id = -1, const wxString& caption = SYMBOL_NETACCDLG_TITLE, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = SYMBOL_NETACCDLG_STYLE );

    /// Creates the controls and sizers
    void CreateControls();

////@begin NetAccDlg event handler declarations

    /// wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for CD_NETACC_LOCAL
    void OnCdNetaccLocalSelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for CD_NETACC_NETWORK
    void OnCdNetaccNetworkSelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_NETACC_PING
    void OnCdNetaccPingClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for CD_NETACC_VIA_FW
    void OnCdNetaccViaFwClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_NETACC_SHOW_CERT
    void OnCdNetaccShowCertClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_NETACC_ERASE_CERT
    void OnCdNetaccEraseCertClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_OK
    void OnOkClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_HELP
    void OnHelpClick( wxCommandEvent& event );

////@end NetAccDlg event handler declarations

////@begin NetAccDlg member function declarations

    wxString GetServerName() const { return server_name ; }
    void SetServerName(wxString value) { server_name = value ; }

    wxString GetServerAddress() const { return server_address ; }
    void SetServerAddress(wxString value) { server_address = value ; }

    wxString GetServerPort() const { return server_port ; }
    void SetServerPort(wxString value) { server_port = value ; }

    bool GetFw() const { return via_fw ; }
    void SetFw(bool value) { via_fw = value ; }

    wxString GetSocksAddress() const { return socks_address ; }
    void SetSocksAddress(wxString value) { socks_address = value ; }

    bool GetLocal() const { return is_local ; }
    void SetLocal(bool value) { is_local = value ; }

    bool GetTunnel() const { return via_tunnel ; }
    void SetTunnel(bool value) { via_tunnel = value ; }

    /// Retrieves bitmap resources
    wxBitmap GetBitmapResource( const wxString& name );

    /// Retrieves icon resources
    wxIcon GetIconResource( const wxString& name );
////@end NetAccDlg member function declarations
  void enabling(void);

    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin NetAccDlg member variables
    wxTextCtrl* mPort;
    wxRadioButton* mLocal;
    wxRadioButton* mNetwork;
    wxTextCtrl* mAddr;
    wxButton* mPing;
    wxCheckBox* mTunnel;
    wxCheckBox* mViaFW;
    wxTextCtrl* mFWAddr;
    wxStaticText* mCertMsg;
    wxButton* mCertShow;
    wxButton* mCertErase;
    wxString server_name;
    wxString server_address;
    wxString server_port;
    bool via_fw;
    wxString socks_address;
    bool is_local;
    bool via_tunnel;
////@end NetAccDlg member variables
};

#endif
    // _NETACCDLG_H_
