/////////////////////////////////////////////////////////////////////////////
// Name:        EvalDlg.h
// Purpose:     
// Author:      
// Modified by: 
// Created:     03/03/04 08:57:40
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifndef _EVALDLG_H_
#define _EVALDLG_H_

#ifdef __GNUG__
//#pragma interface "EvalDlg.cpp"
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
class wxOwnerDrawnComboBox;
////@end forward declarations

/*!
 * Control identifiers
 */

////@begin control identifiers
#define ID_DIALOG 10000
#define SYMBOL_EVALDLG_STYLE wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU|wxCLOSE_BOX
#define SYMBOL_EVALDLG_TITLE _("Evaluate the Expression or Set a New Value")
#define SYMBOL_EVALDLG_IDNAME ID_DIALOG
#define SYMBOL_EVALDLG_SIZE wxSize(400, 300)
#define SYMBOL_EVALDLG_POSITION wxDefaultPosition
#define CD_EVAL_EXPR 10001
#define CD_EVAL_VAL 10002
#define CD_EVAL_EVAL 10003
#define CD_EVAL_SET 10004
#define CD_EVAL_WATCH 10005
////@end control identifiers

/*!
 * Compatibility
 */

#ifndef wxCLOSE_BOX
#define wxCLOSE_BOX 0x1000
#endif

/*!
 * EvalDlg class declaration
 */

class EvalDlg: public wxDialog
{    
    DECLARE_CLASS( EvalDlg )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    EvalDlg(cdp_t cdpIn, wxString exprIn);

    /// Creation
    bool Create( wxWindow* parent, wxWindowID id = -1, const wxString& caption = _("Evaluate the expression or set a new value"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU|wxCLOSE_BOX );

    /// Creates the controls and sizers
    void CreateControls();

////@begin EvalDlg event handler declarations

    /// wxEVT_COMMAND_TEXT_UPDATED event handler for CD_EVAL_VAL
    void OnCdEvalValUpdated( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_EVAL_EVAL
    void OnCdEvalEvalClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_EVAL_SET
    void OnCdEvalSetClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_EVAL_WATCH
    void OnCdEvalWatchClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CANCEL
    void OnCancelClick( wxCommandEvent& event );

////@end EvalDlg event handler declarations

////@begin EvalDlg member function declarations

    /// Retrieves bitmap resources
    wxBitmap GetBitmapResource( const wxString& name );

    /// Retrieves icon resources
    wxIcon GetIconResource( const wxString& name );
////@end EvalDlg member function declarations
  void evaluate_and_show(wxString ex);

    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin EvalDlg member variables
    wxOwnerDrawnComboBox* mEval;
////@end EvalDlg member variables
  cdp_t cdp;
  wxString expr;
};

#endif
    // _EVALDLG_H_
