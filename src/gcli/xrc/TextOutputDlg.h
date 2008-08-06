/////////////////////////////////////////////////////////////////////////////
// Name:        TextOutputDlg.h
// Purpose:     
// Author:      
// Modified by: 
// Created:     03/31/04 09:38:36
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifndef _TEXTOUTPUTDLG_H_
#define _TEXTOUTPUTDLG_H_

#ifdef __GNUG__
#pragma interface "TextOutputDlg.cpp"
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
#define SYMBOL_TEXTOUTPUTDLG_STYLE wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU|wxCLOSE_BOX
#define SYMBOL_TEXTOUTPUTDLG_TITLE _("XML output preview")
#define SYMBOL_TEXTOUTPUTDLG_IDNAME ID_DIALOG
#define SYMBOL_TEXTOUTPUTDLG_SIZE wxSize(400, 300)
#define SYMBOL_TEXTOUTPUTDLG_POSITION wxDefaultPosition
#define CD_XML_TEXT 10001
////@end control identifiers

/*!
 * Compatibility
 */

#ifndef wxCLOSE_BOX
#define wxCLOSE_BOX 0x1000
#endif

/*!
 * TextOutputDlg class declaration
 */

class TextOutputDlg: public wxDialog
{    
    DECLARE_CLASS( TextOutputDlg )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    TextOutputDlg( );
    TextOutputDlg( wxWindow* parent, wxWindowID id = -1, const wxString& caption = _("XML output sample"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = SYMBOL_TEXTOUTPUTDLG_STYLE );

    /// Creation
    bool Create( wxWindow* parent, wxWindowID id = -1, const wxString& caption = _("XML output sample"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = SYMBOL_TEXTOUTPUTDLG_STYLE );

    /// Creates the controls and sizers
    void CreateControls();

////@begin TextOutputDlg event handler declarations

////@end TextOutputDlg event handler declarations

////@begin TextOutputDlg member function declarations

////@end TextOutputDlg member function declarations

    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin TextOutputDlg member variables
    wxTextCtrl* mXMLText;
////@end TextOutputDlg member variables
};

#endif
    // _TEXTOUTPUTDLG_H_
