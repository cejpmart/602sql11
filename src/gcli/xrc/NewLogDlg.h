/////////////////////////////////////////////////////////////////////////////
// Name:        NewLogDlg.h
// Purpose:     
// Author:      
// Modified by: 
// Created:     06/01/04 14:32:15
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifndef _NEWLOGDLG_H_
#define _NEWLOGDLG_H_

#if defined(__GNUG__) && !defined(__APPLE__)
//#pragma interface "NewLogDlg.cpp"
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
#define SYMBOL_NEWLOGDLG_STYLE wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU|wxCLOSE_BOX
#define SYMBOL_NEWLOGDLG_TITLE _("Create New Log")
#define SYMBOL_NEWLOGDLG_IDNAME ID_DIALOG
#define SYMBOL_NEWLOGDLG_SIZE wxSize(400, 300)
#define SYMBOL_NEWLOGDLG_POSITION wxDefaultPosition
#define CD_NEWLOG_NAME 10001
#define CD_NEWLOG_FILE 10002
#define CD_NEWLOG_SELFILE 10003
#define CD_NEWLOG_FORMAT 10004
////@end control identifiers

/*!
 * Compatibility
 */

#ifndef wxCLOSE_BOX
#define wxCLOSE_BOX 0x1000
#endif

/*!
 * NewLogDlg class declaration
 */

class NewLogDlg: public wxDialog
{    
    DECLARE_CLASS( NewLogDlg )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    NewLogDlg(cdp_t cdpIn);

    /// Creation
    bool Create( wxWindow* parent, wxWindowID id = SYMBOL_NEWLOGDLG_IDNAME, const wxString& caption = SYMBOL_NEWLOGDLG_TITLE, const wxPoint& pos = SYMBOL_NEWLOGDLG_POSITION, const wxSize& size = SYMBOL_NEWLOGDLG_SIZE, long style = SYMBOL_NEWLOGDLG_STYLE );

    /// Creates the controls and sizers
    void CreateControls();

////@begin NewLogDlg event handler declarations

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_NEWLOG_SELFILE
    void OnCdNewlogSelfileClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_OK
    void OnOkClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CANCEL
    void OnCancelClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_HELP
    void OnHelpClick( wxCommandEvent& event );

////@end NewLogDlg event handler declarations

////@begin NewLogDlg member function declarations

    /// Retrieves bitmap resources
    wxBitmap GetBitmapResource( const wxString& name );

    /// Retrieves icon resources
    wxIcon GetIconResource( const wxString& name );
////@end NewLogDlg member function declarations

    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin NewLogDlg member variables
    wxTextCtrl* mLogName;
    wxTextCtrl* mLogFile;
    wxTextCtrl* mLogFormat;
////@end NewLogDlg member variables
  cdp_t cdp;
};

#endif
    // _NEWLOGDLG_H_
