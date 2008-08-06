/////////////////////////////////////////////////////////////////////////////
// Name:        RenameServerDlg.h
// Purpose:     
// Author:      
// Modified by: 
// Created:     09/10/04 16:33:43
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifndef _RENAMESERVERDLG_H_
#define _RENAMESERVERDLG_H_

#if defined(__GNUG__) && !defined(__APPLE__)
//#pragma interface "RenameServerDlg.cpp"
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
#define SYMBOL_RENAMESERVERDLG_STYLE wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU|wxCLOSE_BOX
#define SYMBOL_RENAMESERVERDLG_TITLE _("Rename Server")
#define SYMBOL_RENAMESERVERDLG_IDNAME ID_DIALOG
#define SYMBOL_RENAMESERVERDLG_SIZE wxSize(400, 300)
#define SYMBOL_RENAMESERVERDLG_POSITION wxDefaultPosition
#define CD_SRVRENAME_NAME 10001
#define CD_SRVRENAME_ID 10002
#define CD_SRVRENAME_LOCAL 10077
////@end control identifiers

/*!
 * Compatibility
 */

#ifndef wxCLOSE_BOX
#define wxCLOSE_BOX 0x1000
#endif

/*!
 * RenameServerDlg class declaration
 */

class RenameServerDlg: public wxDialog
{    
    DECLARE_CLASS( RenameServerDlg )
    DECLARE_EVENT_TABLE()
    tobjname oldname;

public:
    /// Constructors
    RenameServerDlg(cdp_t cdpIn);

    /// Creation
    bool Create( wxWindow* parent, wxWindowID id = SYMBOL_RENAMESERVERDLG_IDNAME, const wxString& caption = SYMBOL_RENAMESERVERDLG_TITLE, const wxPoint& pos = SYMBOL_RENAMESERVERDLG_POSITION, const wxSize& size = SYMBOL_RENAMESERVERDLG_SIZE, long style = SYMBOL_RENAMESERVERDLG_STYLE );

    /// Creates the controls and sizers
    void CreateControls();

////@begin RenameServerDlg event handler declarations

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_OK
    void OnOkClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CANCEL
    void OnCancelClick( wxCommandEvent& event );

////@end RenameServerDlg event handler declarations

////@begin RenameServerDlg member function declarations

    /// Retrieves bitmap resources
    wxBitmap GetBitmapResource( const wxString& name );

    /// Retrieves icon resources
    wxIcon GetIconResource( const wxString& name );
////@end RenameServerDlg member function declarations

    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin RenameServerDlg member variables
    wxTextCtrl* mName;
    wxTextCtrl* mId;
    wxCheckBox* mLocal;
    wxButton* mOK;
////@end RenameServerDlg member variables
    cdp_t cdp;
};

#endif
    // _RENAMESERVERDLG_H_
