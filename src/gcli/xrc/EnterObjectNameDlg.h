/////////////////////////////////////////////////////////////////////////////
// Name:        EnterObjectNameDlg.h
// Purpose:     
// Author:      
// Modified by: 
// Created:     04/10/04 16:51:41
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifndef _ENTEROBJECTNAMEDLG_H_
#define _ENTEROBJECTNAMEDLG_H_

#ifdef __GNUG__
//#pragma interface "EnterObjectNameDlg.cpp"
#endif

/*!
 * Includes
 */

////@begin includes
#include "wx/statline.h"
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
#define SYMBOL_ENTEROBJECTNAMEDLG_STYLE wxCAPTION|wxSYSTEM_MENU|wxCLOSE_BOX
#define SYMBOL_ENTEROBJECTNAMEDLG_TITLE _("Enter object name")
#define SYMBOL_ENTEROBJECTNAMEDLG_IDNAME ID_DIALOG
#define SYMBOL_ENTEROBJECTNAMEDLG_SIZE wxSize(400, 300)
#define SYMBOL_ENTEROBJECTNAMEDLG_POSITION wxDefaultPosition
#define CD_NAME_PROMPT 10001
#define CD_NAME_NAME 10002
////@end control identifiers

/*!
 * Compatibility
 */

#ifndef wxCLOSE_BOX
#define wxCLOSE_BOX 0x1000
#endif

/*!
 * EnterObjectNameDlg class declaration
 */

class EnterObjectNameDlg: public wxDialog
{    
    DECLARE_CLASS( EnterObjectNameDlg )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    EnterObjectNameDlg(cdp_t cdpIn, wxString promptIn, bool renamingIn, int max_name_lenIn);

    /// Creation
    bool Create( wxWindow* parent, wxWindowID id = -1, const wxString& caption = SYMBOL_ENTEROBJECTNAMEDLG_TITLE, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU|wxCLOSE_BOX );

    /// Creates the controls and sizers
    void CreateControls();

////@begin EnterObjectNameDlg event handler declarations

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_OK
    void OnOkClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CANCEL
    void OnCancelClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_HELP
    void OnHelpClick( wxCommandEvent& event );

////@end EnterObjectNameDlg event handler declarations

////@begin EnterObjectNameDlg member function declarations

////@end EnterObjectNameDlg member function declarations

    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin EnterObjectNameDlg member variables
    wxStaticText* mPrompt;
    wxTextCtrl* mName;
////@end EnterObjectNameDlg member variables
    wxString prompt, name;
    bool renaming;
    cdp_t cdp; 
    int max_name_len;
};

#endif
    // _ENTEROBJECTNAMEDLG_H_
