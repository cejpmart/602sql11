/////////////////////////////////////////////////////////////////////////////
// Name:        CheckListDlg.h
// Purpose:     
// Author:      
// Modified by: 
// Created:     02/04/04 16:48:36
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifndef _CHECKLISTDLG_H_
#define _CHECKLISTDLG_H_

#ifdef __GNUG__
//#pragma interface "CheckListDlg.cpp"
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
#define SYMBOL_CHECKLISTDLG_STYLE wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU|wxCLOSE_BOX
#define SYMBOL_CHECKLISTDLG_TITLE _T("")
#define SYMBOL_CHECKLISTDLG_IDNAME ID_DIALOG
#define SYMBOL_CHECKLISTDLG_SIZE wxSize(400, 300)
#define SYMBOL_CHECKLISTDLG_POSITION wxDefaultPosition
#define CD_CHKL_DROP 10001
#define CD_CHKL_GRID 10002
////@end control identifiers

/*!
 * Compatibility
 */

#ifndef wxCLOSE_BOX
#define wxCLOSE_BOX 0x1000
#endif

/*!
 * CheckListDlg class declaration
 */

class CheckListDlg: public wxPanel
{    
    DECLARE_CLASS( CheckListDlg )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    CheckListDlg( );
    CheckListDlg( wxWindow* parent, wxWindowID id = -1, const wxString& caption = SYMBOL_CHECKLISTDLG_TITLE, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU );

    /// Creation
    bool Create( wxWindow* parent, wxWindowID id = -1, const wxString& caption = SYMBOL_CHECKLISTDLG_TITLE, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU );

    /// Creates the controls and sizers
    void CreateControls();

////@begin CheckListDlg event handler declarations

////@end CheckListDlg event handler declarations

////@begin CheckListDlg member function declarations

////@end CheckListDlg member function declarations

    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin CheckListDlg member variables
////@end CheckListDlg member variables
    void OnCdChklDropClick( wxCommandEvent& event );
    void OnHelpButton( wxCommandEvent& event );
    void OnRangeSelect(wxGridRangeSelectEvent & event);
};

#endif
    // _CHECKLISTDLG_H_
