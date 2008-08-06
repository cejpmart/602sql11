/////////////////////////////////////////////////////////////////////////////
// Name:        LoginDlg.h
// Purpose:     
// Author:      
// Modified by: 
// Created:     01/23/04 09:38:05
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifndef _LOGINDLG_H_
#define _LOGINDLG_H_

#ifdef __GNUG__
//#pragma interface "LoginDlg.cpp"
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
class wxOwnerDrawnComboBox;
////@end forward declarations

/*!
 * Control identifiers
 */

////@begin control identifiers
#define ID_DIALOG 10000
#define SYMBOL_LOGINDLG_STYLE wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU|wxCLOSE_BOX
#define SYMBOL_LOGINDLG_TITLE _T("")
#define SYMBOL_LOGINDLG_IDNAME ID_DIALOG
#define SYMBOL_LOGINDLG_SIZE wxSize(400, 300)
#define SYMBOL_LOGINDLG_POSITION wxDefaultPosition
#define CD_LOGIN_ANONYMOUS 10005
#define CD_LOGIN_NAME_C 10004
#define CD_LOGIN_NAME 10001
#define CD_LOGIN_PASSWORD_C 10003
#define CD_LOGIN_PASSWORD 10002
////@end control identifiers

/*!
 * Compatibility
 */

#ifndef wxCLOSE_BOX
#define wxCLOSE_BOX 0x1000
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
    LoginDlg( );
    LoginDlg( wxWindow* parent, wxWindowID id = -1, const wxString& caption = SYMBOL_LOGINDLG_TITLE, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = SYMBOL_LOGINDLG_STYLE );

    /// Creation
    bool Create( wxWindow* parent, wxWindowID id = -1, const wxString& caption = SYMBOL_LOGINDLG_TITLE, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = SYMBOL_LOGINDLG_STYLE );

    /// Creates the controls and sizers
    void CreateControls();

////@begin LoginDlg event handler declarations

    /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for CD_LOGIN_ANONYMOUS
    void OnCdLoginAnonymousClick( wxCommandEvent& event );

////@end LoginDlg event handler declarations

////@begin LoginDlg member function declarations

    cdp_t GetCdp() const { return cdp ; }
    void SetCdp(cdp_t value) { cdp = value ; }

    /// Retrieves bitmap resources
    wxBitmap GetBitmapResource( const wxString& name );

    /// Retrieves icon resources
    wxIcon GetIconResource( const wxString& name );
////@end LoginDlg member function declarations
    bool TransferDataToWindow(void);
    bool TransferDataFromWindow(void);
    bool Validate(void);

    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin LoginDlg member variables
    wxStaticBitmap* mBitmap;
    wxCheckBox* mAnonym;
    wxStaticText* mNameCapt;
    wxOwnerDrawnComboBox* mName;
    wxStaticText* mPassCapt;
    wxTextCtrl* mPass;
    cdp_t cdp;
////@end LoginDlg member variables
};

#endif
    // _LOGINDLG_H_
