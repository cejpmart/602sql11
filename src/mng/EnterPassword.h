/////////////////////////////////////////////////////////////////////////////
// Name:        EnterPassword.h
// Purpose:     
// Author:      
// Modified by: 
// Created:     06/21/07 15:26:40
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifndef _ENTERPASSWORD_H_
#define _ENTERPASSWORD_H_

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma interface "EnterPassword.dlg"
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
#define ID_ENT_PASS_DLG 10012
#define SYMBOL_ENTERPASSWORD_STYLE wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU|wxCLOSE_BOX
#define SYMBOL_ENTERPASSWORD_TITLE _("Enter password")
#define SYMBOL_ENTERPASSWORD_IDNAME ID_ENT_PASS_DLG
#define SYMBOL_ENTERPASSWORD_SIZE wxSize(400, 300)
#define SYMBOL_ENTERPASSWORD_POSITION wxDefaultPosition
#define CD_PASS_PASS 10013
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
 * EnterPassword class declaration
 */

class EnterPassword: public wxDialog
{    
    DECLARE_DYNAMIC_CLASS( EnterPassword )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    EnterPassword( );
    EnterPassword( wxWindow* parent, wxWindowID id = SYMBOL_ENTERPASSWORD_IDNAME, const wxString& caption = SYMBOL_ENTERPASSWORD_TITLE, const wxPoint& pos = SYMBOL_ENTERPASSWORD_POSITION, const wxSize& size = SYMBOL_ENTERPASSWORD_SIZE, long style = SYMBOL_ENTERPASSWORD_STYLE );

    /// Creation
    bool Create( wxWindow* parent, wxWindowID id = SYMBOL_ENTERPASSWORD_IDNAME, const wxString& caption = SYMBOL_ENTERPASSWORD_TITLE, const wxPoint& pos = SYMBOL_ENTERPASSWORD_POSITION, const wxSize& size = SYMBOL_ENTERPASSWORD_SIZE, long style = SYMBOL_ENTERPASSWORD_STYLE );

    /// Creates the controls and sizers
    void CreateControls();

////@begin EnterPassword event handler declarations

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_OK
    void OnOkClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CANCEL
    void OnCancelClick( wxCommandEvent& event );

////@end EnterPassword event handler declarations

////@begin EnterPassword member function declarations

    /// Retrieves bitmap resources
    wxBitmap GetBitmapResource( const wxString& name );

    /// Retrieves icon resources
    wxIcon GetIconResource( const wxString& name );
////@end EnterPassword member function declarations

    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin EnterPassword member variables
    wxTextCtrl* mPass;
////@end EnterPassword member variables
};

#endif
    // _ENTERPASSWORD_H_
