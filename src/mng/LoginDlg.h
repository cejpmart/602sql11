/////////////////////////////////////////////////////////////////////////////
// Name:        LoginDlg.h
// Purpose:     
// Author:      
// Modified by: 
// Created:     06/25/07 09:14:56
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifndef _LOGINDLG_H_
#define _LOGINDLG_H_

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma interface "LoginDlg.cpp"
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
#define ID_DIALOG 10016
#define SYMBOL_LOGINDLG_STYLE wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU|wxCLOSE_BOX
#define SYMBOL_LOGINDLG_TITLE _("Login to the SQL server")
#define SYMBOL_LOGINDLG_IDNAME ID_DIALOG
#define SYMBOL_LOGINDLG_SIZE wxSize(400, 300)
#define SYMBOL_LOGINDLG_POSITION wxDefaultPosition
#define CD_LOGIN_USER 10017
#define CD_LOGIN_PASS 10018
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
 * LoginDlg class declaration
 */

class LoginDlg: public wxDialog
{    
    DECLARE_CLASS( LoginDlg )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    LoginDlg(cdp_t cdpIn);
    LoginDlg( wxWindow* parent, wxWindowID id = SYMBOL_LOGINDLG_IDNAME, const wxString& caption = SYMBOL_LOGINDLG_TITLE, const wxPoint& pos = SYMBOL_LOGINDLG_POSITION, const wxSize& size = SYMBOL_LOGINDLG_SIZE, long style = SYMBOL_LOGINDLG_STYLE );

    /// Creation
    bool Create( wxWindow* parent, wxWindowID id = SYMBOL_LOGINDLG_IDNAME, const wxString& caption = SYMBOL_LOGINDLG_TITLE, const wxPoint& pos = SYMBOL_LOGINDLG_POSITION, const wxSize& size = SYMBOL_LOGINDLG_SIZE, long style = SYMBOL_LOGINDLG_STYLE );

    /// Creates the controls and sizers
    void CreateControls();

////@begin LoginDlg event handler declarations

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_OK
    void OnOkClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CANCEL
    void OnCancelClick( wxCommandEvent& event );

////@end LoginDlg event handler declarations

////@begin LoginDlg member function declarations

    /// Retrieves bitmap resources
    wxBitmap GetBitmapResource( const wxString& name );

    /// Retrieves icon resources
    wxIcon GetIconResource( const wxString& name );
////@end LoginDlg member function declarations

    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin LoginDlg member variables
    wxComboBox* mUser;
    wxTextCtrl* mPass;
////@end LoginDlg member variables
    cdp_t cdp;
};

#endif
    // _LOGINDLG_H_
