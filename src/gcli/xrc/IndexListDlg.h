/////////////////////////////////////////////////////////////////////////////
// Name:        IndexListDlg.h
// Purpose:     
// Author:      
// Modified by: 
// Created:     02/04/04 09:19:12
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifndef _INDEXLISTDLG_H_
#define _INDEXLISTDLG_H_

#ifdef __GNUG__
//#pragma interface "IndexListDlg.cpp"
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
#define SYMBOL_INDEXLISTDLG_STYLE wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU|wxCLOSE_BOX
#define SYMBOL_INDEXLISTDLG_TITLE _T("")
#define SYMBOL_INDEXLISTDLG_IDNAME ID_DIALOG
#define SYMBOL_INDEXLISTDLG_SIZE wxSize(400, 300)
#define SYMBOL_INDEXLISTDLG_POSITION wxDefaultPosition
#define CD_TDCC_DEL 10002
#define CD_TDCC_GRID 10004
////@end control identifiers

/*!
 * Compatibility
 */

#ifndef wxCLOSE_BOX
#define wxCLOSE_BOX 0x1000
#endif

/*!
 * IndexListDlg class declaration
 */

class IndexListDlg: public wxPanel
{    
    DECLARE_CLASS( IndexListDlg )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    IndexListDlg( );
    IndexListDlg( wxWindow* parent, wxWindowID id = -1, const wxString& caption = SYMBOL_INDEXLISTDLG_TITLE, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU );

    /// Creation
    bool Create( wxWindow* parent, wxWindowID id = -1, const wxString& caption = SYMBOL_INDEXLISTDLG_TITLE, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU );

    /// Creates the controls and sizers
    void CreateControls();

////@begin IndexListDlg event handler declarations

////@end IndexListDlg event handler declarations

////@begin IndexListDlg member function declarations

////@end IndexListDlg member function declarations

    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin IndexListDlg member variables
////@end IndexListDlg member variables
    void OnCdTdccDelClick( wxCommandEvent& event );
    void OnHelpButton( wxCommandEvent& event );
    void OnRangeSelect(wxGridRangeSelectEvent & event);
};

#endif
    // _INDEXLISTDLG_H_
