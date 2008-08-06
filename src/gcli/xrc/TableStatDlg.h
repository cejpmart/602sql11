/////////////////////////////////////////////////////////////////////////////
// Name:        TableStatDlg.h
// Purpose:     
// Author:      
// Modified by: 
// Created:     09/27/07 12:40:32
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifndef _TABLESTATDLG_H_
#define _TABLESTATDLG_H_

#if defined(__GNUG__) && !defined(__APPLE__)
//#pragma interface "TableStatDlg.cpp"
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
class wxListCtrl;
////@end forward declarations

/*!
 * Control identifiers
 */

////@begin control identifiers
#define ID_MYDIALOG15 10174
#define SYMBOL_TABLESTATDLG_STYLE wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU|wxCLOSE_BOX
#define SYMBOL_TABLESTATDLG_TITLE _("Table Statistics")
#define SYMBOL_TABLESTATDLG_IDNAME ID_MYDIALOG15
#define SYMBOL_TABLESTATDLG_SIZE wxSize(400, 300)
#define SYMBOL_TABLESTATDLG_POSITION wxDefaultPosition
#define CD_TABSTAT_MAIN_CAPT 10175
#define CD_TABSTAT_LIST1 10176
#define CD_TABSTAT_COMP 10177
#define CD_TABSTAT_EXT 10178
#define CD_TABSTAT_LIST2 10000
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
 * TableStatDlg class declaration
 */

class TableStatDlg: public wxDialog
{    
    DECLARE_CLASS( TableStatDlg )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    TableStatDlg(cdp_t cdp, tobjnum objnum, wxString table_name, wxString schema_name);

    /// Creation
    bool Create( wxWindow* parent, wxWindowID id = SYMBOL_TABLESTATDLG_IDNAME, const wxString& caption = SYMBOL_TABLESTATDLG_TITLE, const wxPoint& pos = SYMBOL_TABLESTATDLG_POSITION, const wxSize& size = SYMBOL_TABLESTATDLG_SIZE, long style = SYMBOL_TABLESTATDLG_STYLE );

    /// Creates the controls and sizers
    void CreateControls();

////@begin TableStatDlg event handler declarations

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_TABSTAT_COMP
    void OnCdTabstatCompClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CLOSE
    void OnCloseClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_HELP
    void OnHelpClick( wxCommandEvent& event );

////@end TableStatDlg event handler declarations

////@begin TableStatDlg member function declarations

    /// Retrieves bitmap resources
    wxBitmap GetBitmapResource( const wxString& name );

    /// Retrieves icon resources
    wxIcon GetIconResource( const wxString& name );
////@end TableStatDlg member function declarations

    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin TableStatDlg member variables
    wxStaticText* mMainCapt;
    wxListCtrl* mList1;
    wxButton* mCompute;
    wxCheckBox* mExt;
    wxListCtrl* mList2;
////@end TableStatDlg member variables
    cdp_t cdp;
    tobjnum objnum;
    wxString table_name, schema_name;
};

#endif
    // _TABLESTATDLG_H_
