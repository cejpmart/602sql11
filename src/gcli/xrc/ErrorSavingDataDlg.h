/////////////////////////////////////////////////////////////////////////////
// Name:        ErrorSavingDataDlg.h
// Purpose:     
// Author:      
// Modified by: 
// Created:     04/19/04 15:18:36
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifndef _ERRORSAVINGDATADLG_H_
#define _ERRORSAVINGDATADLG_H_

#ifdef __GNUG__
//#pragma interface "ErrorSavingDataDlg.cpp"
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
#define SYMBOL_ERRORSAVINGDATADLG_STYLE wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU|wxCLOSE_BOX
#define SYMBOL_ERRORSAVINGDATADLG_TITLE _("Error Saving Data")
#define SYMBOL_ERRORSAVINGDATADLG_IDNAME ID_DIALOG
#define SYMBOL_ERRORSAVINGDATADLG_SIZE wxSize(400, 300)
#define SYMBOL_ERRORSAVINGDATADLG_POSITION wxDefaultPosition
#define CD_DATAERR_MSG 10001
////@end control identifiers

/*!
 * Compatibility
 */

#ifndef wxCLOSE_BOX
#define wxCLOSE_BOX 0x1000
#endif

/*!
 * ErrorSavingDataDlg class declaration
 */

class ErrorSavingDataDlg: public wxDialog
{    
    DECLARE_CLASS( ErrorSavingDataDlg )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    ErrorSavingDataDlg(xcdp_t xcdpIn);

    /// Creation
    bool Create( wxWindow* parent, wxWindowID id = -1, const wxString& caption = _("Error Saving Data"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU );

    /// Creates the controls and sizers
    void CreateControls();

////@begin ErrorSavingDataDlg event handler declarations

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_OK
    void OnOkClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CANCEL
    void OnCancelClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_HELP
    void OnHelpClick( wxCommandEvent& event );

////@end ErrorSavingDataDlg event handler declarations

////@begin ErrorSavingDataDlg member function declarations

    /// Retrieves bitmap resources
    wxBitmap GetBitmapResource( const wxString& name );

    /// Retrieves icon resources
    wxIcon GetIconResource( const wxString& name );
////@end ErrorSavingDataDlg member function declarations

    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin ErrorSavingDataDlg member variables
    wxTextCtrl* mMsg;
    wxButton* mHelp;
////@end ErrorSavingDataDlg member variables
  xcdp_t xcdp;
  int topic;
};

#endif
    // _ERRORSAVINGDATADLG_H_
