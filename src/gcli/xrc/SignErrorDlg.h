/////////////////////////////////////////////////////////////////////////////
// Name:        SignErrorDlg.h
// Purpose:     
// Author:      
// Modified by: 
// Created:     06/22/04 14:51:03
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifndef _SIGNERRORDLG_H_
#define _SIGNERRORDLG_H_

#if defined(__GNUG__) && !defined(__APPLE__)
//#pragma interface "SignErrorDlg.cpp"
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
#define SYMBOL_SIGNERRORDLG_STYLE wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU|wxCLOSE_BOX
#define SYMBOL_SIGNERRORDLG_TITLE _("SignErrorDlg")
#define SYMBOL_SIGNERRORDLG_IDNAME ID_DIALOG
#define SYMBOL_SIGNERRORDLG_SIZE wxSize(400, 300)
#define SYMBOL_SIGNERRORDLG_POSITION wxDefaultPosition
#define CD_ERROR_MESSAGE 10001
////@end control identifiers

/*!
 * Compatibility
 */

#ifndef wxCLOSE_BOX
#define wxCLOSE_BOX 0x1000
#endif

/*!
 * SignErrorDlg class declaration
 */

class SignErrorDlg: public wxDialog
{    
    DECLARE_CLASS( SignErrorDlg )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    SignErrorDlg(wxString captIn, wxString msgIn, int topicIn);

    /// Creation
    bool Create( wxWindow* parent, wxWindowID id = SYMBOL_SIGNERRORDLG_IDNAME, const wxString& caption = SYMBOL_SIGNERRORDLG_TITLE, const wxPoint& pos = SYMBOL_SIGNERRORDLG_POSITION, const wxSize& size = SYMBOL_SIGNERRORDLG_SIZE, long style = SYMBOL_SIGNERRORDLG_STYLE );

    /// Creates the controls and sizers
    void CreateControls();

////@begin SignErrorDlg event handler declarations

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_OK
    void OnOkClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_HELP
    void OnHelpClick( wxCommandEvent& event );

////@end SignErrorDlg event handler declarations

////@begin SignErrorDlg member function declarations

////@end SignErrorDlg member function declarations

    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin SignErrorDlg member variables
    wxStaticText* mMsg;
    wxButton* mHelp;
////@end SignErrorDlg member variables
    wxString capt, msg;
    int topic;
};

#endif
    // _SIGNERRORDLG_H_
