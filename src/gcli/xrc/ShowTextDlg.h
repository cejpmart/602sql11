/////////////////////////////////////////////////////////////////////////////
// Name:        ShowTextDlg.h
// Purpose:     
// Author:      
// Modified by: 
// Created:     02/03/04 14:39:36
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifndef _SHOWTEXTDLG_H_
#define _SHOWTEXTDLG_H_

#ifdef __GNUG__
//#pragma interface "ShowTextDlg.cpp"
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
#define SYMBOL_SHOWTEXTDLG_STYLE wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU|wxCLOSE_BOX
#define SYMBOL_SHOWTEXTDLG_TITLE _("SQL command")
#define SYMBOL_SHOWTEXTDLG_IDNAME ID_DIALOG
#define SYMBOL_SHOWTEXTDLG_SIZE wxSize(400, 300)
#define SYMBOL_SHOWTEXTDLG_POSITION wxDefaultPosition
#define CD_TD_SQL 10001
////@end control identifiers

/*!
 * Compatibility
 */

#ifndef wxCLOSE_BOX
#define wxCLOSE_BOX 0x1000
#endif

/*!
 * ShowTextDlg class declaration
 */

class ShowTextDlg: public wxDialog
{    
    DECLARE_CLASS( ShowTextDlg )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    ShowTextDlg( );
    ShowTextDlg( wxWindow* parent, wxWindowID id = -1, const wxString& caption = _("SQL command"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = SYMBOL_SHOWTEXTDLG_STYLE );

    /// Creation
    bool Create( wxWindow* parent, wxWindowID id = -1, const wxString& caption = _("SQL command"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = SYMBOL_SHOWTEXTDLG_STYLE );

    /// Creates the controls and sizers
    void CreateControls();

////@begin ShowTextDlg event handler declarations

////@end ShowTextDlg event handler declarations

////@begin ShowTextDlg member function declarations

////@end ShowTextDlg member function declarations

    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin ShowTextDlg member variables
    wxTextCtrl* mText;
////@end ShowTextDlg member variables
};

#endif
    // _SHOWTEXTDLG_H_
