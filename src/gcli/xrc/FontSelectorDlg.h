/////////////////////////////////////////////////////////////////////////////
// Name:        FontSelectorDlg.h
// Purpose:     
// Author:      
// Modified by: 
// Created:     06/03/04 16:49:53
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifndef _FONTSELECTORDLG_H_
#define _FONTSELECTORDLG_H_

#if defined(__GNUG__) && !defined(__APPLE__)
//#pragma interface "FontSelectorDlg.cpp"
#endif

#define FONT_CNT 4

struct t_font_state
{ wxFont font;
  bool   existing;
};
  
/*!
 * Includes
 */

////@begin includes
#include "wx/grid.h"
////@end includes

/*!
 * Forward declarations
 */

////@begin forward declarations
class wxBoxSizer;
class wxGrid;
////@end forward declarations

/*!
 * Control identifiers
 */

////@begin control identifiers
#define ID_DIALOG 10000
#define SYMBOL_FONTSELECTORDLG_STYLE wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU|wxCLOSE_BOX
#define SYMBOL_FONTSELECTORDLG_TITLE _("Client Fonts")
#define SYMBOL_FONTSELECTORDLG_IDNAME ID_DIALOG
#define SYMBOL_FONTSELECTORDLG_SIZE wxSize(400, 300)
#define SYMBOL_FONTSELECTORDLG_POSITION wxDefaultPosition
#define CD_FONT_GRID 10001
////@end control identifiers

/*!
 * Compatibility
 */

#ifndef wxCLOSE_BOX
#define wxCLOSE_BOX 0x1000
#endif

/*!
 * FontSelectorDlg class declaration
 */

class FontSelectorDlg: public wxDialog
{    
    DECLARE_CLASS( FontSelectorDlg )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    FontSelectorDlg( );
    FontSelectorDlg( wxWindow* parent, wxWindowID id = SYMBOL_FONTSELECTORDLG_IDNAME, const wxString& caption = SYMBOL_FONTSELECTORDLG_TITLE, const wxPoint& pos = SYMBOL_FONTSELECTORDLG_POSITION, const wxSize& size = SYMBOL_FONTSELECTORDLG_SIZE, long style = SYMBOL_FONTSELECTORDLG_STYLE );

    /// Creation
    bool Create( wxWindow* parent, wxWindowID id = SYMBOL_FONTSELECTORDLG_IDNAME, const wxString& caption = SYMBOL_FONTSELECTORDLG_TITLE, const wxPoint& pos = SYMBOL_FONTSELECTORDLG_POSITION, const wxSize& size = SYMBOL_FONTSELECTORDLG_SIZE, long style = SYMBOL_FONTSELECTORDLG_STYLE );

    /// Creates the controls and sizers
    void CreateControls();

////@begin FontSelectorDlg event handler declarations

    /// wxEVT_GRID_CELL_LEFT_DCLICK event handler for CD_FONT_GRID
    void OnCdFontGridLeftDClick( wxGridEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_OK
    void OnOkClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CANCEL
    void OnCancelClick( wxCommandEvent& event );

////@end FontSelectorDlg event handler declarations

////@begin FontSelectorDlg member function declarations

////@end FontSelectorDlg member function declarations

    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin FontSelectorDlg member variables
    wxBoxSizer* mSizer;
    wxGrid* mGrid;
////@end FontSelectorDlg member variables
  t_font_state font_list[FONT_CNT];
};

#endif
    // _FONTSELECTORDLG_H_
