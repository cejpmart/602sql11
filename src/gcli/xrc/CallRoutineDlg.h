/////////////////////////////////////////////////////////////////////////////
// Name:        CallRoutineDlg.h
// Purpose:     
// Author:      
// Modified by: 
// Created:     02/24/04 12:33:23
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifndef _CALLROUTINEDLG_H_
#define _CALLROUTINEDLG_H_

#ifdef __GNUG__
//#pragma interface "CallRoutineDlg.cpp"
#endif

/*!
 * Includes
 */
#include <wx/grid.h>

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
#define SYMBOL_CALLROUTINEDLG_STYLE wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU|wxCLOSE_BOX
#define SYMBOL_CALLROUTINEDLG_TITLE _("Calling Routine")
#define SYMBOL_CALLROUTINEDLG_IDNAME ID_DIALOG
#define SYMBOL_CALLROUTINEDLG_SIZE wxSize(400, 250)
#define SYMBOL_CALLROUTINEDLG_POSITION wxDefaultPosition
#define ID_CALL_GRID 10001
#define ID_CALL_DEBUG 10002
#define ID_CALL_CALL 10150
#define ID_CALL_CANCEL 10151
////@end control identifiers

/*!
 * Compatibility
 */

#ifndef wxCLOSE_BOX
#define wxCLOSE_BOX 0x1000
#endif

struct t_run_sql_param
{ t_idname parname;
  int data_type, length, precision, direction;
  t_specif specif;
  wxString parval;
};

/*!
 * CallRoutineDlg class declaration
 */

class CallRoutineDlg: public wxDialog
{    
    DECLARE_CLASS( CallRoutineDlg )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    CallRoutineDlg(cdp_t cdpIn, tobjnum objnumIn);
    /// Creation
    bool Create( wxWindow* parent, wxWindowID id = -1, const wxString& caption = _("Calling routine"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = SYMBOL_CALLROUTINEDLG_STYLE);
    /// Creates the controls and sizers
    void CreateControls();
    ~CallRoutineDlg(void);

////@begin CallRoutineDlg event handler declarations

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_CALL_CALL
    void OnCallCallClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_CALL_CANCEL
    void OnCallCancelClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_HELP
    void OnHelpClick( wxCommandEvent& event );

////@end CallRoutineDlg event handler declarations

////@begin CallRoutineDlg member function declarations

    /// Retrieves bitmap resources
    wxBitmap GetBitmapResource( const wxString& name );

    /// Retrieves icon resources
    wxIcon GetIconResource( const wxString& name );
////@end CallRoutineDlg member function declarations
    void EnableControls(bool enable);
    void prepare_grid(void);
    bool init(void);
    bool call_sql_proc(bool debugged);
    void clear_results(void);
    void display_results(void);
    void OnClose(wxCloseEvent & event);

    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin CallRoutineDlg member variables
    wxBoxSizer* mSizerContainingGrid;
    wxGrid* mGrid;
    wxCheckBox* mDebug;
    wxButton* mOK;
    wxButton* mCancel;
////@end CallRoutineDlg member variables
  static int counter;
  static bool debug_mode;
  static CallRoutineDlg * global_inst;
  cdp_t cdp; 
  tobjnum objnum;
  tobjname name, schema_name;
  trecnum par_cnt;
  bool is_fnc;
  wxString fncval;
  t_run_sql_param * pars;
  t_clivar * hvars;
};

#endif
    // _CALLROUTINEDLG_H_
