/////////////////////////////////////////////////////////////////////////////
// Name:        PasswordExpiredDlg.h
// Purpose:     
// Author:      
// Modified by: 
// Created:     04/10/04 12:25:52
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifndef _PASSWORDEXPIREDDLG_H_
#define _PASSWORDEXPIREDDLG_H_

#ifdef __GNUG__
//#pragma interface "PasswordExpiredDlg.cpp"
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
#define ID_DIALOG 10000
#define SYMBOL_PASSWORDEXPIREDDLG_STYLE wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU|wxCLOSE_BOX
#define SYMBOL_PASSWORDEXPIREDDLG_TITLE _("Enter New Password")
#define SYMBOL_PASSWORDEXPIREDDLG_IDNAME ID_DIALOG
#define SYMBOL_PASSWORDEXPIREDDLG_SIZE wxSize(400, 300)
#define SYMBOL_PASSWORDEXPIREDDLG_POSITION wxDefaultPosition
#define CD_EXP_PASS1 10006
#define CD_EXP_PASS2 10007
////@end control identifiers

/*!
 * Compatibility
 */

#ifndef wxCLOSE_BOX
#define wxCLOSE_BOX 0x1000
#endif

/*!
 * PasswordExpiredDlg class declaration
 */

class PasswordExpiredDlg: public wxDialog
{    
    DECLARE_CLASS( PasswordExpiredDlg )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    PasswordExpiredDlg(cdp_t cdpIn);
    /// Creation
    bool Create( wxWindow* parent, wxWindowID id = -1, const wxString& caption = _("Enter New Password"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU );
    /// Creates the controls and sizers
    void CreateControls();

////@begin PasswordExpiredDlg event handler declarations

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_OK
    void OnOkClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CANCEL
    void OnCancelClick( wxCommandEvent& event );

////@end PasswordExpiredDlg event handler declarations

////@begin PasswordExpiredDlg member function declarations

    /// Retrieves bitmap resources
    wxBitmap GetBitmapResource( const wxString& name );

    /// Retrieves icon resources
    wxIcon GetIconResource( const wxString& name );
////@end PasswordExpiredDlg member function declarations

    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin PasswordExpiredDlg member variables
    wxStaticText* mPass1C;
    wxTextCtrl* mPass1;
    wxStaticText* mPass2C;
    wxTextCtrl* mPass2;
////@end PasswordExpiredDlg member variables
  cdp_t cdp;
  char password[MAX_PASSWORD_LEN+1];
};

#endif
    // _PASSWORDEXPIREDDLG_H_
