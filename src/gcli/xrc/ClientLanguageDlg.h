/////////////////////////////////////////////////////////////////////////////
// Name:        ClientLanguageDlg.h
// Purpose:     
// Author:      
// Modified by: 
// Created:     10/01/04 09:08:36
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifndef _CLIENTLANGUAGEDLG_H_
#define _CLIENTLANGUAGEDLG_H_

#if defined(__GNUG__) && !defined(__APPLE__)
//#pragma interface "ClientLanguageDlg.cpp"
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
#define SYMBOL_CLIENTLANGUAGEDLG_STYLE wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU|wxCLOSE_BOX
#define SYMBOL_CLIENTLANGUAGEDLG_TITLE _("Client Locales and Language")
#define SYMBOL_CLIENTLANGUAGEDLG_IDNAME ID_DIALOG
#define SYMBOL_CLIENTLANGUAGEDLG_SIZE wxSize(400, 300)
#define SYMBOL_CLIENTLANGUAGEDLG_POSITION wxDefaultPosition
#define CD_LOCALE_CHOICE 10001
#define CD_LOCALE_LOAD_CATALOG 10002
////@end control identifiers

/*!
 * Compatibility
 */

#ifndef wxCLOSE_BOX
#define wxCLOSE_BOX 0x1000
#endif

/*!
 * ClientLanguageDlg class declaration
 */

class ClientLanguageDlg: public wxDialog
{    
    DECLARE_CLASS( ClientLanguageDlg )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    ClientLanguageDlg( );
    ClientLanguageDlg( wxWindow* parent, wxWindowID id = SYMBOL_CLIENTLANGUAGEDLG_IDNAME, const wxString& caption = SYMBOL_CLIENTLANGUAGEDLG_TITLE, const wxPoint& pos = SYMBOL_CLIENTLANGUAGEDLG_POSITION, const wxSize& size = SYMBOL_CLIENTLANGUAGEDLG_SIZE, long style = SYMBOL_CLIENTLANGUAGEDLG_STYLE );

    /// Creation
    bool Create( wxWindow* parent, wxWindowID id = SYMBOL_CLIENTLANGUAGEDLG_IDNAME, const wxString& caption = SYMBOL_CLIENTLANGUAGEDLG_TITLE, const wxPoint& pos = SYMBOL_CLIENTLANGUAGEDLG_POSITION, const wxSize& size = SYMBOL_CLIENTLANGUAGEDLG_SIZE, long style = SYMBOL_CLIENTLANGUAGEDLG_STYLE );

    /// Creates the controls and sizers
    void CreateControls();

////@begin ClientLanguageDlg event handler declarations

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_OK
    void OnOkClick( wxCommandEvent& event );

////@end ClientLanguageDlg event handler declarations

////@begin ClientLanguageDlg member function declarations

    /// Retrieves bitmap resources
    wxBitmap GetBitmapResource( const wxString& name );

    /// Retrieves icon resources
    wxIcon GetIconResource( const wxString& name );
////@end ClientLanguageDlg member function declarations

    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin ClientLanguageDlg member variables
    wxOwnerDrawnComboBox* mLocale;
    wxCheckBox* mLoadCatalog;
////@end ClientLanguageDlg member variables
};

#endif
    // _CLIENTLANGUAGEDLG_H_
