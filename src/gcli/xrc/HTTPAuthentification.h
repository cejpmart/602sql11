/////////////////////////////////////////////////////////////////////////////
// Name:        HTTPAuthentification.h
// Purpose:     
// Author:      
// Modified by: 
// Created:     03/29/06 12:12:50
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifndef _HTTPAUTHENTIFICATION_H_
#define _HTTPAUTHENTIFICATION_H_

#if defined(__GNUG__) && !defined(__APPLE__)
//#pragma interface "HTTPAuthentification.cpp"
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
#define ID_MYDIALOG4 10100
#define SYMBOL_HTTPAUTHENTIFICATION_STYLE wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU|wxCLOSE_BOX
#define SYMBOL_HTTPAUTHENTIFICATION_TITLE _("HTTP Authentification")
#define SYMBOL_HTTPAUTHENTIFICATION_IDNAME ID_MYDIALOG4
#define SYMBOL_HTTPAUTHENTIFICATION_SIZE wxSize(400, 300)
#define SYMBOL_HTTPAUTHENTIFICATION_POSITION wxDefaultPosition
#define CD_HTTPA_NAME 10101
#define CD_HTTPA_PASSWORD 10102
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
 * HTTPAuthentification class declaration
 */

class HTTPAuthentification: public wxDialog
{    
    DECLARE_CLASS( HTTPAuthentification )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    HTTPAuthentification(bool againIn);
    HTTPAuthentification( wxWindow* parent, wxWindowID id = SYMBOL_HTTPAUTHENTIFICATION_IDNAME, const wxString& caption = SYMBOL_HTTPAUTHENTIFICATION_TITLE, const wxPoint& pos = SYMBOL_HTTPAUTHENTIFICATION_POSITION, const wxSize& size = SYMBOL_HTTPAUTHENTIFICATION_SIZE, long style = SYMBOL_HTTPAUTHENTIFICATION_STYLE );

    /// Creation
    bool Create( wxWindow* parent, wxWindowID id = SYMBOL_HTTPAUTHENTIFICATION_IDNAME, const wxString& caption = SYMBOL_HTTPAUTHENTIFICATION_TITLE, const wxPoint& pos = SYMBOL_HTTPAUTHENTIFICATION_POSITION, const wxSize& size = SYMBOL_HTTPAUTHENTIFICATION_SIZE, long style = SYMBOL_HTTPAUTHENTIFICATION_STYLE );

    /// Creates the controls and sizers
    void CreateControls();

////@begin HTTPAuthentification event handler declarations

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_OK
    void OnOkClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CANCEL
    void OnCancelClick( wxCommandEvent& event );

////@end HTTPAuthentification event handler declarations

////@begin HTTPAuthentification member function declarations

    /// Retrieves bitmap resources
    wxBitmap GetBitmapResource( const wxString& name );

    /// Retrieves icon resources
    wxIcon GetIconResource( const wxString& name );
////@end HTTPAuthentification member function declarations

    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin HTTPAuthentification member variables
    wxTextCtrl* mName;
    wxTextCtrl* mPassword;
////@end HTTPAuthentification member variables
    bool again;
};

#endif
    // _HTTPAUTHENTIFICATION_H_
