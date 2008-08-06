/////////////////////////////////////////////////////////////////////////////
// Name:        SequenceDesignerDlg.h
// Purpose:     
// Author:      
// Modified by: 
// Created:     02/21/04 11:47:52
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifndef _SEQUENCEDESIGNERDLG_H_
#define _SEQUENCEDESIGNERDLG_H_

#ifdef __GNUG__
//#pragma interface "SequenceDesignerDlg.cpp"
#endif

/*!
 * Includes
 */

////@begin includes
#include "wx/toolbar.h"
#include "wx/statline.h"
#include "wx/spinctrl.h"
////@end includes
#include "wx/spinctrl.h"

/*!
 * Forward declarations
 */

////@begin forward declarations
class wxBoxSizer;
////@end forward declarations

/*!
 * Control identifiers
 */

////@begin control identifiers
#define ID_DIALOG 10000
#define SYMBOL_SEQUENCEDESIGNERDLG_STYLE wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU|wxCLOSE_BOX|wxWANTS_CHARS
#define SYMBOL_SEQUENCEDESIGNERDLG_TITLE _("Design of a sequence")
#define SYMBOL_SEQUENCEDESIGNERDLG_IDNAME ID_DIALOG
#define SYMBOL_SEQUENCEDESIGNERDLG_SIZE wxDefaultSize
#define SYMBOL_SEQUENCEDESIGNERDLG_POSITION wxDefaultPosition
#define ID_TOOLBAR 10010
#define CD_SEQ_START 10001
#define CD_SEQ_STEP 10002
#define CD_SEQ_MAX 10003
#define CD_SEQ_MAXVAL 10004
#define CD_SEQ_MIN 10005
#define CD_SEQ_MINVAL 10006
#define CD_SEQ_CYCLE 10007
#define CD_SEQ_CACHE 10008
#define CD_SEQ_CACHEVAL 10009
#define CD_SEQ_VALIDATE 10012
#define CD_SEQ_SQL 10013
////@end control identifiers

/*!
 * Compatibility
 */

#ifndef wxCLOSE_BOX
#define wxCLOSE_BOX 0x1000
#endif

/*!
 * SequenceDesignerDlg class declaration
 */

#define SEQ_TOOL_COUNT 7

class SequenceDesignerDlg: public wxDialog, public any_designer
{    
    DECLARE_CLASS( SequenceDesignerDlg )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    SequenceDesignerDlg(cdp_t cdpIn, tobjnum objnumIn, const char * folder_nameIn, const char * schema_nameIn);
    ~SequenceDesignerDlg(void);
    /// Creation
    bool Create( wxWindow* parent, wxWindowID id = SYMBOL_SEQUENCEDESIGNERDLG_IDNAME,  const wxString& caption=SYMBOL_SEQUENCEDESIGNERDLG_TITLE, const wxPoint& pos = SYMBOL_SEQUENCEDESIGNERDLG_POSITION, const wxSize& size = SYMBOL_SEQUENCEDESIGNERDLG_SIZE, long style = SYMBOL_SEQUENCEDESIGNERDLG_STYLE );
    /// Creates the controls and sizers
    void CreateControls();

////@begin SequenceDesignerDlg event handler declarations

    /// wxEVT_COMMAND_TEXT_UPDATED event handler for CD_SEQ_START
    void OnCdSeqStartUpdated( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_UPDATED event handler for CD_SEQ_STEP
    void OnCdSeqStepUpdated( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for CD_SEQ_MAX
    void OnCdSeqMaxClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_UPDATED event handler for CD_SEQ_MAXVAL
    void OnCdSeqMaxvalUpdated( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for CD_SEQ_MIN
    void OnCdSeqMinClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_UPDATED event handler for CD_SEQ_MINVAL
    void OnCdSeqMinvalUpdated( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for CD_SEQ_CYCLE
    void OnCdSeqCycleClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for CD_SEQ_CACHE
    void OnCdSeqCacheClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_SPINCTRL_UPDATED event handler for CD_SEQ_CACHEVAL
    void OnCdSeqCachevalUpdated( wxSpinEvent& event );

    /// wxEVT_COMMAND_TEXT_UPDATED event handler for CD_SEQ_CACHEVAL
    void OnCdSeqCachevalTextUpdated( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_OK
    void OnOkClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CANCEL
    void OnCancelClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_SEQ_VALIDATE
    void OnCdSeqValidateClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_SEQ_SQL
    void OnCdSeqSqlClick( wxCommandEvent& event );

////@end SequenceDesignerDlg event handler declarations

////@begin SequenceDesignerDlg member function declarations

    /// Retrieves bitmap resources
    wxBitmap GetBitmapResource( const wxString& name );

    /// Retrieves icon resources
    wxIcon GetIconResource( const wxString& name );
////@end SequenceDesignerDlg member function declarations
  void set_designer_caption(void);
  bool TransferDataToWindow(void);
  bool TransferDataFromWindow(void);
  bool Validate(void);
  bool open(wxWindow * parent, t_global_style my_style);
  void OnKeyDown(wxKeyEvent & event);
  static t_tool_info seq_tool_info[SEQ_TOOL_COUNT+1];
  void destroy_designer(void);
 // virtual methods (commented in class any_designer):
  char * make_source(bool alter);
  bool IsChanged(void) const;  
  wxString SaveQuestion(void) const;  
  bool save_design(bool save_as);
  void OnCommand(wxCommandEvent & event);
  void OnxCommand(wxCommandEvent & event);
  t_tool_info * get_tool_info(void) const
    { return seq_tool_info; }
  void _set_focus(void)
    { SetFocus(); }
  wxMenu * get_designer_menu(void);
  void OnActivate(wxActivateEvent & event);
  void OnCloseWindow(wxCloseEvent& event);
  void Activated(void);
    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin SequenceDesignerDlg member variables
    wxBoxSizer* mTopSizer;
    wxToolBar* mTB;
    wxBoxSizer* mButtons;
////@end SequenceDesignerDlg member variables
  bool feedback_disabled;
  bool changed;
  t_sequence seq;
};

#endif
    // _SEQUENCEDESIGNERDLG_H_
