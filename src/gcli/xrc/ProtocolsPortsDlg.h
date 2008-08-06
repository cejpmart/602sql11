/////////////////////////////////////////////////////////////////////////////
// Name:        ProtocolsPortsDlg.h
// Purpose:     
// Author:      
// Modified by: 
// Created:     03/30/06 09:52:45
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifndef _PROTOCOLSPORTSDLG_H_
#define _PROTOCOLSPORTSDLG_H_

#if defined(__GNUG__) && !defined(__APPLE__)
//#pragma interface "ProtocolsPortsDlg.dlg"
#endif

/*!
 * Includes
 */

////@begin includes
////@end includes

/*!
 * Forward declarations
 */

////@begin forward declarations
class wxBoxSizer;
////@end forward declarations

/*!
 * Control identifiers
 */

////@begin control identifiers
#define ID_MYDIALOG5 10103
#define SYMBOL_PROTOCOLSPORTSDLG_STYLE wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU|wxCLOSE_BOX
#define SYMBOL_PROTOCOLSPORTSDLG_TITLE _("Protocols and Ports")
#define SYMBOL_PROTOCOLSPORTSDLG_IDNAME ID_MYDIALOG5
#define SYMBOL_PROTOCOLSPORTSDLG_SIZE wxDefaultSize
#define SYMBOL_PROTOCOLSPORTSDLG_POSITION wxDefaultPosition
#define CD_PP_IPC 10004
#define CD_PP_IPC_RUNS 10096
#define CD_PP_TCPIP 10005
#define CD_PP_NET_RUNS 10006
#define CD_PP_PORT 10106
#define CD_PP_TUNNEL 10000
#define CD_PP_TUNNEL_RUNS 10007
#define CD_PP_TUNNEL_PORT 10108
#define CD_PP_WEB 10001
#define CD_PP_WEB_RUNS 10008
#define ID_STATICTEXT 10002
#define CD_PP_WEB_PORT 10003
////@end control identifiers

/*!
 * Compatibility
 */

#ifndef wxCLOSE_BOX
#define wxCLOSE_BOX 0x1000
#endif
#ifndef wxFIXED_MINSIZE
#define wxFIXED_MINSIZE 0
#endif

/*!
 * ProtocolsPortsDlg class declaration
 */

class ProtocolsPortsDlg: public wxDialog
{    
    DECLARE_CLASS( ProtocolsPortsDlg )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    ProtocolsPortsDlg(cdp_t cdpIn);
    ProtocolsPortsDlg( wxWindow* parent, wxWindowID id = SYMBOL_PROTOCOLSPORTSDLG_IDNAME, const wxString& caption = SYMBOL_PROTOCOLSPORTSDLG_TITLE, const wxPoint& pos = SYMBOL_PROTOCOLSPORTSDLG_POSITION, const wxSize& size = SYMBOL_PROTOCOLSPORTSDLG_SIZE, long style = SYMBOL_PROTOCOLSPORTSDLG_STYLE );

    /// Creation
    bool Create( wxWindow* parent, wxWindowID id = -1/*SYMBOL_PROTOCOLSPORTSDLG_IDNAME*/, const wxString& caption = SYMBOL_PROTOCOLSPORTSDLG_TITLE, const wxPoint& pos = SYMBOL_PROTOCOLSPORTSDLG_POSITION, const wxSize& size = SYMBOL_PROTOCOLSPORTSDLG_SIZE, long style = SYMBOL_PROTOCOLSPORTSDLG_STYLE );

    /// Creates the controls and sizers
    void CreateControls();

////@begin ProtocolsPortsDlg event handler declarations

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_OK
    void OnOkClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CANCEL
    void OnCancelClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_HELP
    void OnHelpClick( wxCommandEvent& event );

////@end ProtocolsPortsDlg event handler declarations

////@begin ProtocolsPortsDlg member function declarations

    /// Retrieves bitmap resources
    wxBitmap GetBitmapResource( const wxString& name );

    /// Retrieves icon resources
    wxIcon GetIconResource( const wxString& name );
////@end ProtocolsPortsDlg member function declarations

    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin ProtocolsPortsDlg member variables
    wxBoxSizer* mTopSizer;
    wxCheckBox* mIPC;
    wxStaticText* mIPCRuns;
    wxCheckBox* mTCPIP;
    wxStaticText* mNetRuns;
    wxTextCtrl* mPort;
    wxCheckBox* mTunnel;
    wxStaticText* mTunnelRuns;
    wxTextCtrl* mTunnelPort;
    wxBoxSizer* mWebSizer1;
    wxCheckBox* mWeb;
    wxStaticText* mWebRuns;
    wxBoxSizer* mWebSizer2;
    wxStaticText* mWebPortCapt;
    wxTextCtrl* mWebPort;
    wxButton* mOK;
////@end ProtocolsPortsDlg member variables
    cdp_t cdp;
    bool ipc,tcpip,tunnel,web;
    char ip_port[10],tunnel_port[10],web_port[10];
};

#endif
    // _PROTOCOLSPORTSDLG_H_
