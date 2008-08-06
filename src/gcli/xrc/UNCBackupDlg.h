/////////////////////////////////////////////////////////////////////////////
// Name:        UNCBackupDlg.h
// Purpose:     
// Author:      
// Modified by: 
// Created:     04/24/08 11:48:04
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifndef _UNCBACKUPDLG_H_
#define _UNCBACKUPDLG_H_

#if defined(__GNUG__) && !defined(__APPLE__)
//#pragma interface "UNCBackupDlg.cpp"
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
#define ID_MYDIALOG17 10188
#define SYMBOL_UNCBACKUPDLG_STYLE wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU|wxCLOSE_BOX
#define SYMBOL_UNCBACKUPDLG_TITLE _("Backup over Network")
#define SYMBOL_UNCBACKUPDLG_IDNAME ID_MYDIALOG17
#define SYMBOL_UNCBACKUPDLG_SIZE wxSize(400, 300)
#define SYMBOL_UNCBACKUPDLG_POSITION wxDefaultPosition
#define CD_UNC_NAME 10189
#define CD_UNC_PASS1 10190
#define CD_UNC_PASS2 10191
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
 * UNCBackupDlg class declaration
 */

class UNCBackupDlg: public wxDialog
{    
    DECLARE_CLASS( UNCBackupDlg )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    UNCBackupDlg(cdp_t cdpIn);

    /// Creation
    bool Create( wxWindow* parent, wxWindowID id = SYMBOL_UNCBACKUPDLG_IDNAME, const wxString& caption = SYMBOL_UNCBACKUPDLG_TITLE, const wxPoint& pos = SYMBOL_UNCBACKUPDLG_POSITION, const wxSize& size = SYMBOL_UNCBACKUPDLG_SIZE, long style = SYMBOL_UNCBACKUPDLG_STYLE );

    /// Creates the controls and sizers
    void CreateControls();

////@begin UNCBackupDlg event handler declarations

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_OK
    void OnOkClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CANCEL
    void OnCancelClick( wxCommandEvent& event );

////@end UNCBackupDlg event handler declarations

////@begin UNCBackupDlg member function declarations

    /// Retrieves bitmap resources
    wxBitmap GetBitmapResource( const wxString& name );

    /// Retrieves icon resources
    wxIcon GetIconResource( const wxString& name );
////@end UNCBackupDlg member function declarations

    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin UNCBackupDlg member variables
    wxTextCtrl* mUserName;
    wxTextCtrl* mPass1;
    wxTextCtrl* mPass2;
////@end UNCBackupDlg member variables
    cdp_t cdp;
};

#endif
    // _UNCBACKUPDLG_H_
