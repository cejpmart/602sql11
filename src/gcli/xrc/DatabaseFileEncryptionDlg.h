/////////////////////////////////////////////////////////////////////////////
// Name:        DatabaseFileEncryptionDlg.h
// Purpose:     
// Author:      
// Modified by: 
// Created:     05/02/04 17:47:35
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifndef _DATABASEFILEENCRYPTIONDLG_H_
#define _DATABASEFILEENCRYPTIONDLG_H_

#if defined(__GNUG__) && !defined(__APPLE__)
//#pragma interface "DatabaseFileEncryptionDlg.cpp"
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
#define SYMBOL_DATABASEFILEENCRYPTIONDLG_STYLE wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU|wxCLOSE_BOX
#define SYMBOL_DATABASEFILEENCRYPTIONDLG_TITLE _("Database File Encryption")
#define SYMBOL_DATABASEFILEENCRYPTIONDLG_IDNAME ID_DIALOG
#define SYMBOL_DATABASEFILEENCRYPTIONDLG_SIZE wxSize(400, 300)
#define SYMBOL_DATABASEFILEENCRYPTIONDLG_POSITION wxDefaultPosition
#define CD_FILENC_TYPE 10001
#define CD_FILENC_PASS1 10002
#define CD_FILENC_PASS2 10003
////@end control identifiers

/*!
 * Compatibility
 */

#ifndef wxCLOSE_BOX
#define wxCLOSE_BOX 0x1000
#endif

/*!
 * DatabaseFileEncryptionDlg class declaration
 */

class DatabaseFileEncryptionDlg: public wxDialog
{    
    DECLARE_CLASS( DatabaseFileEncryptionDlg )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    DatabaseFileEncryptionDlg(cdp_t cdpIn);

    /// Creation
    bool Create( wxWindow* parent, wxWindowID id = SYMBOL_DATABASEFILEENCRYPTIONDLG_IDNAME, const wxString& caption = SYMBOL_DATABASEFILEENCRYPTIONDLG_TITLE, const wxPoint& pos = SYMBOL_DATABASEFILEENCRYPTIONDLG_POSITION, const wxSize& size = SYMBOL_DATABASEFILEENCRYPTIONDLG_SIZE, long style = SYMBOL_DATABASEFILEENCRYPTIONDLG_STYLE );

    /// Creates the controls and sizers
    void CreateControls();

////@begin DatabaseFileEncryptionDlg event handler declarations

    /// wxEVT_COMMAND_RADIOBOX_SELECTED event handler for CD_FILENC_TYPE
    void OnCdFilencTypeSelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_OK
    void OnOkClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CANCEL
    void OnCancelClick( wxCommandEvent& event );

////@end DatabaseFileEncryptionDlg event handler declarations

////@begin DatabaseFileEncryptionDlg member function declarations

    /// Retrieves bitmap resources
    wxBitmap GetBitmapResource( const wxString& name );

    /// Retrieves icon resources
    wxIcon GetIconResource( const wxString& name );
////@end DatabaseFileEncryptionDlg member function declarations

    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin DatabaseFileEncryptionDlg member variables
    wxRadioBox* mType;
    wxTextCtrl* mPass1;
    wxTextCtrl* mPass2;
    wxButton* mOK;
////@end DatabaseFileEncryptionDlg member variables
  cdp_t cdp;
};

#endif
    // _DATABASEFILEENCRYPTIONDLG_H_
