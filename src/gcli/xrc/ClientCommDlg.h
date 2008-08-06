/////////////////////////////////////////////////////////////////////////////
// Name:        ClientCommDlg.h
// Purpose:     
// Author:      
// Modified by: 
// Created:     02/02/06 15:15:16
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifndef _CLIENTCOMMDLG_H_
#define _CLIENTCOMMDLG_H_

#if defined(__GNUG__) && !defined(__APPLE__)
//#pragma interface "ClientCommDlg.cpp"
#endif

/*!
 * Includes
 */

////@begin includes
#include "wx/spinctrl.h"
////@end includes

/*!
 * Forward declarations
 */

////@begin forward declarations
class wxSpinCtrl;
////@end forward declarations

/*!
 * Control identifiers
 */

////@begin control identifiers
#define ID_MYDIALOG1 10078
#define SYMBOL_CLIENTCOMMDLG_STYLE wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU|wxCLOSE_BOX
#define SYMBOL_CLIENTCOMMDLG_TITLE _("Client Communication Properties")
#define SYMBOL_CLIENTCOMMDLG_IDNAME ID_MYDIALOG1
#define SYMBOL_CLIENTCOMMDLG_SIZE wxSize(400, 300)
#define SYMBOL_CLIENTCOMMDLG_POSITION wxDefaultPosition
#define CD_COMM_SCAN 10079
#define CD_COMM_BROADCAST 10080
#define CD_COMM_NOTIFS 10081
#define CD_COMM_MODULUS 10082
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
 * ClientCommDlg class declaration
 */

class ClientCommDlg: public wxDialog
{    
    DECLARE_DYNAMIC_CLASS( ClientCommDlg )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    ClientCommDlg( );
    ClientCommDlg( wxWindow* parent, wxWindowID id = SYMBOL_CLIENTCOMMDLG_IDNAME, const wxString& caption = SYMBOL_CLIENTCOMMDLG_TITLE, const wxPoint& pos = SYMBOL_CLIENTCOMMDLG_POSITION, const wxSize& size = SYMBOL_CLIENTCOMMDLG_SIZE, long style = SYMBOL_CLIENTCOMMDLG_STYLE );

    /// Creation
    bool Create( wxWindow* parent, wxWindowID id = SYMBOL_CLIENTCOMMDLG_IDNAME, const wxString& caption = SYMBOL_CLIENTCOMMDLG_TITLE, const wxPoint& pos = SYMBOL_CLIENTCOMMDLG_POSITION, const wxSize& size = SYMBOL_CLIENTCOMMDLG_SIZE, long style = SYMBOL_CLIENTCOMMDLG_STYLE );

    /// Creates the controls and sizers
    void CreateControls();

////@begin ClientCommDlg event handler declarations

    /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for CD_COMM_SCAN
    void OnCdCommScanClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for CD_COMM_NOTIFS
    void OnCdCommNotifsClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_OK
    void OnOkClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_HELP
    void OnHelpClick( wxCommandEvent& event );

////@end ClientCommDlg event handler declarations

////@begin ClientCommDlg member function declarations

    /// Retrieves bitmap resources
    wxBitmap GetBitmapResource( const wxString& name );

    /// Retrieves icon resources
    wxIcon GetIconResource( const wxString& name );
////@end ClientCommDlg member function declarations

    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin ClientCommDlg member variables
    wxCheckBox* mScan;
    wxCheckBox* mBroadcast;
    wxCheckBox* mNotifs;
    wxSpinCtrl* mModulus;
////@end ClientCommDlg member variables
};

#endif
    // _CLIENTCOMMDLG_H_
