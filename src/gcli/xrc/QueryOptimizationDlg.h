/////////////////////////////////////////////////////////////////////////////
// Name:        QueryOptimizationDlg.h
// Purpose:     
// Author:      
// Modified by: 
// Created:     05/03/04 12:49:06
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifndef _QUERYOPTIMIZATIONDLG_H_
#define _QUERYOPTIMIZATIONDLG_H_

#if defined(__GNUG__) && !defined(__APPLE__)
//#pragma interface "QueryOptimizationDlg.cpp"
#endif

/*!
 * Includes
 */

////@begin includes
#include "wx/treectrl.h"
////@end includes

/*!
 * Forward declarations
 */

////@begin forward declarations
class wxBoxSizer;
class wxTreeCtrl;
////@end forward declarations

/*!
 * Control identifiers
 */

////@begin control identifiers
#define ID_DIALOG 10000
#define SYMBOL_QUERYOPTIMIZATIONDLG_STYLE wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU|wxCLOSE_BOX
#define SYMBOL_QUERYOPTIMIZATIONDLG_TITLE _("Query Optimization Analysis")
#define SYMBOL_QUERYOPTIMIZATIONDLG_IDNAME ID_DIALOG
#define SYMBOL_QUERYOPTIMIZATIONDLG_SIZE wxSize(400, 300)
#define SYMBOL_QUERYOPTIMIZATIONDLG_POSITION wxDefaultPosition
#define CD_OPTIM_TREE 10002
#define CD_OPTIM_EVAL 10001
////@end control identifiers

/*!
 * Compatibility
 */

#ifndef wxCLOSE_BOX
#define wxCLOSE_BOX 0x1000
#endif

/*!
 * QueryOptimizationDlg class declaration
 */

class QueryOptimizationDlg: public wxDialog
{    
    DECLARE_CLASS( QueryOptimizationDlg )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    QueryOptimizationDlg(cdp_t cdpIn, const char * sourceIn);

    /// Creation
    bool Create( wxWindow* parent, wxWindowID id = SYMBOL_QUERYOPTIMIZATIONDLG_IDNAME, const wxString& caption = SYMBOL_QUERYOPTIMIZATIONDLG_TITLE, const wxPoint& pos = SYMBOL_QUERYOPTIMIZATIONDLG_POSITION, const wxSize& size = SYMBOL_QUERYOPTIMIZATIONDLG_SIZE, long style = SYMBOL_QUERYOPTIMIZATIONDLG_STYLE );

    /// Creates the controls and sizers
    void CreateControls();

////@begin QueryOptimizationDlg event handler declarations

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_OPTIM_EVAL
    void OnCdOptimEvalClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CANCEL
    void OnCancelClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_HELP
    void OnHelpClick( wxCommandEvent& event );

////@end QueryOptimizationDlg event handler declarations

////@begin QueryOptimizationDlg member function declarations

    /// Retrieves bitmap resources
    wxBitmap GetBitmapResource( const wxString& name );

    /// Retrieves icon resources
    wxIcon GetIconResource( const wxString& name );
////@end QueryOptimizationDlg member function declarations
  void show_opt(bool evaluate);

    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin QueryOptimizationDlg member variables
    wxBoxSizer* mSizer;
    wxTreeCtrl* mTree;
    wxButton* mEval;
////@end QueryOptimizationDlg member variables
    cdp_t cdp;
    const char * source;
};

#endif
    // _QUERYOPTIMIZATIONDLG_H_
