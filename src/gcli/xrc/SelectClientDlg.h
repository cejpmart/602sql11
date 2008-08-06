/////////////////////////////////////////////////////////////////////////////
// Name:        SelectClientDlg.h
// Purpose:     
// Author:      
// Modified by: 
// Created:     03/02/04 15:01:57
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifndef _SELECTCLIENTDLG_H_
#define _SELECTCLIENTDLG_H_

#ifdef __GNUG__
//#pragma interface "SelectClientDlg.cpp"
#endif

/*!
 * Includes
 */

////@begin includes
#include "wx/listctrl.h"
////@end includes

/*!
 * Forward declarations
 */

////@begin forward declarations
class wxBoxSizer;
class wxListCtrl;
////@end forward declarations

/*!
 * Control identifiers
 */

////@begin control identifiers
#define ID_DIALOG 10000
#define SYMBOL_SELECTCLIENTDLG_STYLE wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU|wxCLOSE_BOX
#define SYMBOL_SELECTCLIENTDLG_TITLE _("Select client to be debugged")
#define SYMBOL_SELECTCLIENTDLG_IDNAME ID_DIALOG
#define SYMBOL_SELECTCLIENTDLG_SIZE wxSize(400, 300)
#define SYMBOL_SELECTCLIENTDLG_POSITION wxDefaultPosition
#define CD_SELPROC_LIST 10001
////@end control identifiers

/*!
 * Compatibility
 */

#ifndef wxCLOSE_BOX
#define wxCLOSE_BOX 0x1000
#endif

/*!
 * SelectClientDlg class declaration
 */

class SelectClientDlg: public wxDialog
{    
    DECLARE_CLASS( SelectClientDlg )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    SelectClientDlg(cdp_t cdpIn);

    /// Creation
    bool Create( wxWindow* parent, wxWindowID id = -1, const wxString& caption = _("Select client to debug"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU|wxCLOSE_BOX );

    /// Creates the controls and sizers
    void CreateControls();

////@begin SelectClientDlg event handler declarations

    /// wxEVT_COMMAND_LIST_ITEM_SELECTED event handler for CD_SELPROC_LIST
    void OnCdSelprocListSelected( wxListEvent& event );

////@end SelectClientDlg event handler declarations

////@begin SelectClientDlg member function declarations

    /// Retrieves bitmap resources
    wxBitmap GetBitmapResource( const wxString& name );

    /// Retrieves icon resources
    wxIcon GetIconResource( const wxString& name );
////@end SelectClientDlg member function declarations

    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin SelectClientDlg member variables
    wxBoxSizer* mSizerContainingGrid;
    wxListCtrl* mList;
    wxButton* mOK;
////@end SelectClientDlg member variables
    cdp_t cdp;
};

#endif
    // _SELECTCLIENTDLG_H_
