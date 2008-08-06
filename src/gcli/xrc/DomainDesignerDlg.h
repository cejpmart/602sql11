/////////////////////////////////////////////////////////////////////////////
// Name:        DomainDesignerDlg.h
// Purpose:     
// Author:      
// Modified by: 
// Created:     02/18/04 09:30:18
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifndef _DOMAINDESIGNERDLG_H_
#define _DOMAINDESIGNERDLG_H_

#ifdef __GNUG__
//#pragma interface "DomainDesignerDlg.cpp"
#endif

/*!
 * Includes
 */

////@begin includes
#include "wx/toolbar.h"
#include "wx/statline.h"
#include "wx/spinctrl.h"
////@end includes

/*!
 * Forward declarations
 */

////@begin forward declarations
class wxBoxSizer;
class wxOwnerDrawnComboBox;
class wxSpinCtrl;
////@end forward declarations

/*!
 * Control identifiers
 */

////@begin control identifiers
#define ID_DIALOG 10000
#define SYMBOL_DOMAINDESIGNERDLG_STYLE wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU|wxCLOSE_BOX|wxWANTS_CHARS
#define SYMBOL_DOMAINDESIGNERDLG_TITLE _("Desing of the domain")
#define SYMBOL_DOMAINDESIGNERDLG_IDNAME ID_DIALOG
#define SYMBOL_DOMAINDESIGNERDLG_SIZE wxSize(400, 300)
#define SYMBOL_DOMAINDESIGNERDLG_POSITION wxDefaultPosition
#define CD_DOM_TB 10015
#define CD_DOM_TYPE 10167
#define CD_DOM_NULL 10002
#define CD_DOM_BYTELENGTH 10051
#define CD_DOM_LENGTH_CAPT 10011
#define CD_DOM_LENGTH 10003
#define CD_DOM_UNICODE 10005
#define CD_DOM_IGNORE 10006
#define CD_DOM_COLL_CAPT 10012
#define CD_DOM_COLL 10004
#define CD_DOM_DEFAULT_CAPT 10013
#define CD_DOM_DEFAULT 10010
#define CD_DOM_CHECK 10008
#define CD_DOM_DEFER_CAPT 10014
#define CD_DOM_DEFER 10009
#define CD_DOM_VALIDATE 10012
#define CD_DOM_SQL 10013
////@end control identifiers

/*!
 * Compatibility
 */

#ifndef wxCLOSE_BOX
#define wxCLOSE_BOX 0x1000
#endif

/*!
 * DomainDesignerDlg class declaration
 */
#define DOM_TOOL_COUNT 7

class DomainDesignerDlg: public wxDialog, public any_designer
{    
    DECLARE_CLASS( DomainDesignerDlg )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    DomainDesignerDlg(cdp_t cdpIn, tobjnum objnumIn, const char * folder_nameIn, const char * schema_nameIn);
    ~DomainDesignerDlg(void);
    /// Creation
    bool Create( wxWindow* parent, wxWindowID id = -1, const wxString& caption = _("Desing of the domain"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = SYMBOL_DOMAINDESIGNERDLG_STYLE );
    /// Creates the controls and sizers
    void CreateControls();

////@begin DomainDesignerDlg event handler declarations

    /// wxEVT_COMMAND_COMBOBOX_SELECTED event handler for CD_DOM_TYPE
    void OnCdDomTypeSelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for CD_DOM_NULL
    void OnCdDomNullClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_COMBOBOX_SELECTED event handler for CD_DOM_BYTELENGTH
    void OnCdDomBytelengthSelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_SPINCTRL_UPDATED event handler for CD_DOM_LENGTH
    void OnCdDomLengthUpdated( wxSpinEvent& event );

    /// wxEVT_COMMAND_TEXT_UPDATED event handler for CD_DOM_LENGTH
    void OnCdDomLengthTextUpdated( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for CD_DOM_UNICODE
    void OnCdDomUnicodeClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for CD_DOM_IGNORE
    void OnCdDomIgnoreClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_COMBOBOX_SELECTED event handler for CD_DOM_COLL
    void OnCdDomCollSelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_UPDATED event handler for CD_DOM_DEFAULT
    void OnCdDomDefaultUpdated( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_UPDATED event handler for CD_DOM_CHECK
    void OnCdDomCheckUpdated( wxCommandEvent& event );

    /// wxEVT_COMMAND_COMBOBOX_SELECTED event handler for CD_DOM_DEFER
    void OnCdDomDeferSelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_OK
    void OnOkClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CANCEL
    void OnCancelClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_DOM_VALIDATE
    void OnCdDomValidateClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_DOM_SQL
    void OnCdDomSqlClick( wxCommandEvent& event );

////@end DomainDesignerDlg event handler declarations

////@begin DomainDesignerDlg member function declarations

    bool GetChanged() const { return changed ; }
    void SetChanged(bool value) { changed = value ; }

    /// Retrieves bitmap resources
    wxBitmap GetBitmapResource( const wxString& name );

    /// Retrieves icon resources
    wxIcon GetIconResource( const wxString& name );
////@end DomainDesignerDlg member function declarations
  void show_type_specif(void);
  void set_designer_caption(void);
  char * make_source(bool alter);
  bool TransferDataToWindow(void);
  bool TransferDataFromWindow(void);
  bool Validate(void);
  bool open(wxWindow * parent, t_global_style my_style);
  void OnKeyDown(wxKeyEvent & event);
  void OnActivate(wxActivateEvent & event);
  void OnCloseWindow(wxCloseEvent& event);
  void Activated(void);
  static t_tool_info dom_tool_info[DOM_TOOL_COUNT+1];
  void destroy_designer(void);

 // virtual methods (commented in class any_designer):
  bool IsChanged(void) const;  
  wxString SaveQuestion(void) const;  
  bool save_design(bool save_as);
  void OnCommand(wxCommandEvent & event);
  void OnxCommand(wxCommandEvent & event);
  t_tool_info * get_tool_info(void) const
    { return dom_tool_info; }
  void _set_focus(void)
    { SetFocus(); }
  wxMenu * get_designer_menu(void);

    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin DomainDesignerDlg member variables
    wxBoxSizer* mTopSizer;
    wxToolBar* mTB;
    wxOwnerDrawnComboBox* mType;
    wxCheckBox* mNull;
    wxStaticText* mByteLengthCapt;
    wxOwnerDrawnComboBox* mByteLength;
    wxStaticText* mLengthCapt;
    wxSpinCtrl* mLength;
    wxCheckBox* mUnicode;
    wxCheckBox* mIgnore;
    wxStaticText* mCollCapt;
    wxOwnerDrawnComboBox* mColl;
    wxStaticText* mDefaultCapt;
    wxTextCtrl* mDefault;
    wxTextCtrl* mCheck;
    wxStaticText* mDeferCapt;
    wxOwnerDrawnComboBox* mDefer;
    wxBoxSizer* mButtons;
    bool changed;
////@end DomainDesignerDlg member variables
    t_type_specif_info dom;
    bool feedback_disabled;
};

#endif
    // _DOMAINDESIGNERDLG_H_
