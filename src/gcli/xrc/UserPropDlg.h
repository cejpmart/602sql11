/////////////////////////////////////////////////////////////////////////////
// Name:        UserPropDlg.h
// Purpose:     
// Author:      
// Modified by: 
// Created:     04/09/04 16:25:40
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifndef _USERPROPDLG_H_
#define _USERPROPDLG_H_

#ifdef __GNUG__
//#pragma interface "UserPropDlg.cpp"
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
#define SYMBOL_USERPROPDLG_STYLE wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU|wxCLOSE_BOX
#define SYMBOL_USERPROPDLG_TITLE _("User Properties")
#define SYMBOL_USERPROPDLG_IDNAME ID_DIALOG
#define SYMBOL_USERPROPDLG_SIZE wxSize(400, 300)
#define SYMBOL_USERPROPDLG_POSITION wxDefaultPosition
#define CD_USR_LNAME 10001
#define CD_USR_DISABLED 10009
#define CD_USR_NAME1 10002
#define CD_USR_NAME2 10003
#define CD_USR_NAME3 10004
#define CD_USR_IDENT 10005
#define CD_USR_SPECPASS 10010
#define CD_USR_PASS1 10006
#define CD_USR_PASS2 10007
#define CD_USR_MUSTCHANGE 10008
////@end control identifiers

/*!
 * Compatibility
 */

#ifndef wxCLOSE_BOX
#define wxCLOSE_BOX 0x1000
#endif

/*!
 * UserPropDlg class declaration
 */

class UserPropDlg: public wxDialog
{    
    DECLARE_CLASS( UserPropDlg )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    UserPropDlg(cdp_t cdpIn, tobjnum objnumIn);
    /// Creation
    bool Create( wxWindow* parent, wxWindowID id = -1, const wxString& caption = _("User Properties"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU|wxCLOSE_BOX );
    /// Creates the controls and sizers
    void CreateControls();

////@begin UserPropDlg event handler declarations

    /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for CD_USR_SPECPASS
    void OnCdUsrSpecpassClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_UPDATED event handler for CD_USR_PASS1
    void OnCdUsrPass1Updated( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for CD_USR_MUSTCHANGE
    void OnCdUsrMustchangeClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_OK
    void OnOkClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CANCEL
    void OnCancelClick( wxCommandEvent& event );

////@end UserPropDlg event handler declarations

////@begin UserPropDlg member function declarations

    /// Retrieves bitmap resources
    wxBitmap GetBitmapResource( const wxString& name );

    /// Retrieves icon resources
    wxIcon GetIconResource( const wxString& name );
////@end UserPropDlg member function declarations

    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin UserPropDlg member variables
    wxTextCtrl* mLoginName;
    wxCheckBox* mDisabled;
    wxTextCtrl* mFirstName;
    wxTextCtrl* mMiddleName;
    wxTextCtrl* mLastName;
    wxTextCtrl* mIdent;
    wxCheckBox* mSpecPass;
    wxStaticText* mPass1C;
    wxTextCtrl* mPass1;
    wxStaticText* mPass2C;
    wxTextCtrl* mPass2;
    wxCheckBox* mMustChange;
////@end UserPropDlg member variables
  cdp_t cdp;
  tobjnum objnum;
  bool editable, password_editable, any_password_change;
  tobjname orig_logname;
  wbbool orig_account_disabled;
};

#endif
    // _USERPROPDLG_H_
