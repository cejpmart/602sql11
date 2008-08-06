/////////////////////////////////////////////////////////////////////////////
// Name:        ClientsConnectedDlg.h
// Purpose:     
// Author:      
// Modified by: 
// Created:     06/21/07 15:20:10
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifndef _CLIENTSCONNECTEDDLG_H_
#define _CLIENTSCONNECTEDDLG_H_

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma interface "ClientsConnectedDlg.cpp"
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
////@end forward declarations

/*!
 * Control identifiers
 */

////@begin control identifiers
#define ID_CLICONN_DLG 10009
#define SYMBOL_CLIENTSCONNECTEDDLG_STYLE wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU|wxCLOSE_BOX
#define SYMBOL_CLIENTSCONNECTEDDLG_TITLE _("Clients connected to the server")
#define SYMBOL_CLIENTSCONNECTEDDLG_IDNAME ID_CLICONN_DLG
#define SYMBOL_CLIENTSCONNECTEDDLG_SIZE wxSize(400, 300)
#define SYMBOL_CLIENTSCONNECTEDDLG_POSITION wxDefaultPosition
#define CD_DOWN_MSG 10011
#define CD_DOWN_OPT 10010
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
 * ClientsConnectedDlg class declaration
 */

class ClientsConnectedDlg: public wxDialog
{    
    DECLARE_DYNAMIC_CLASS( ClientsConnectedDlg )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    ClientsConnectedDlg( );
    ClientsConnectedDlg( wxWindow* parent, wxWindowID id = SYMBOL_CLIENTSCONNECTEDDLG_IDNAME, const wxString& caption = SYMBOL_CLIENTSCONNECTEDDLG_TITLE, const wxPoint& pos = SYMBOL_CLIENTSCONNECTEDDLG_POSITION, const wxSize& size = SYMBOL_CLIENTSCONNECTEDDLG_SIZE, long style = SYMBOL_CLIENTSCONNECTEDDLG_STYLE );

    /// Creation
    bool Create( wxWindow* parent, wxWindowID id = SYMBOL_CLIENTSCONNECTEDDLG_IDNAME, const wxString& caption = SYMBOL_CLIENTSCONNECTEDDLG_TITLE, const wxPoint& pos = SYMBOL_CLIENTSCONNECTEDDLG_POSITION, const wxSize& size = SYMBOL_CLIENTSCONNECTEDDLG_SIZE, long style = SYMBOL_CLIENTSCONNECTEDDLG_STYLE );

    /// Creates the controls and sizers
    void CreateControls();

////@begin ClientsConnectedDlg event handler declarations

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_OK
    void OnOkClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CANCEL
    void OnCancelClick( wxCommandEvent& event );

////@end ClientsConnectedDlg event handler declarations

////@begin ClientsConnectedDlg member function declarations

    /// Retrieves bitmap resources
    wxBitmap GetBitmapResource( const wxString& name );

    /// Retrieves icon resources
    wxIcon GetIconResource( const wxString& name );
////@end ClientsConnectedDlg member function declarations

    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin ClientsConnectedDlg member variables
    wxStaticText* mMsg;
    wxRadioBox* mOpt;
////@end ClientsConnectedDlg member variables
};

#endif
    // _CLIENTSCONNECTEDDLG_H_
