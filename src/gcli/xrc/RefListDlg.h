/////////////////////////////////////////////////////////////////////////////
// Name:        RefListDlg.h
// Purpose:     
// Author:      
// Modified by: 
// Created:     02/05/04 11:27:49
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifndef _REFLISTDLG_H_
#define _REFLISTDLG_H_

#ifdef __GNUG__
//#pragma interface "RefListDlg.cpp"
#endif

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
////@end forward declarations

/*!
 * Control identifiers
 */

////@begin control identifiers
#define ID_DIALOG 10000
#define SYMBOL_REFLISTDLG_STYLE wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU|wxCLOSE_BOX
#define SYMBOL_REFLISTDLG_TITLE _T("")
#define SYMBOL_REFLISTDLG_IDNAME ID_DIALOG
#define SYMBOL_REFLISTDLG_SIZE wxSize(400, 300)
#define SYMBOL_REFLISTDLG_POSITION wxDefaultPosition
#define CD_REFL_DROP 10001
#define CD_REFL_GRID 10002
////@end control identifiers

/*!
 * Compatibility
 */

#ifndef wxCLOSE_BOX
#define wxCLOSE_BOX 0x1000
#endif

/*!
 * RefListDlg class declaration
 */

class RefListDlg: public wxPanel
{    
    DECLARE_CLASS( RefListDlg )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    RefListDlg( );
    RefListDlg( wxWindow* parent, wxWindowID id = -1, const wxString& caption = SYMBOL_REFLISTDLG_TITLE, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU );

    /// Creation
    bool Create( wxWindow* parent, wxWindowID id = -1, const wxString& caption = SYMBOL_REFLISTDLG_TITLE, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU );

    /// Creates the controls and sizers
    void CreateControls();

////@begin RefListDlg event handler declarations

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_REFL_DROP
    void OnCdReflDropClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_HELP
    void OnHelpClick( wxCommandEvent& event );

////@end RefListDlg event handler declarations
    void OnRangeSelect(wxGridRangeSelectEvent & event);

////@begin RefListDlg member function declarations

////@end RefListDlg member function declarations

    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin RefListDlg member variables
////@end RefListDlg member variables
};

#endif
    // _REFLISTDLG_H_
