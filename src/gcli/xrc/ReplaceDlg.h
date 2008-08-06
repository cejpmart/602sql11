/////////////////////////////////////////////////////////////////////////////
// Name:        ReplaceDlg.h
// Purpose:     
// Author:      
// Modified by: 
// Created:     03/19/04 08:55:36
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifndef _REPLACEDLG_H_
#define _REPLACEDLG_H_

#ifdef __GNUG__
//#pragma interface "ReplaceDlg.cpp"
#endif

/*!
 * Includes
 */
#include <wx/fdrepdlg.h>

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
#define SYMBOL_REPLACEDLG_STYLE wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU|wxSTAY_ON_TOP|wxCLOSE_BOX|wxWANTS_CHARS
#define SYMBOL_REPLACEDLG_TITLE _("Replace")
#define SYMBOL_REPLACEDLG_IDNAME ID_DIALOG
#define SYMBOL_REPLACEDLG_SIZE wxSize(400, 300)
#define SYMBOL_REPLACEDLG_POSITION wxDefaultPosition
#define CD_REPL_FINDSTR 10003
#define CD_REPL_REPLSTR 10005
#define CD_REPL_WORD 10006
#define CD_REPL_CASE 10007
#define CD_REPL_BACK 10008
#define CD_REPL_SEL 10009
#define CD_REPL_FIND 10001
#define CD_REPL_REPL 10004
#define CD_REPL_REPLALL 10002
////@end control identifiers

/*!
 * Compatibility
 */

#ifndef wxCLOSE_BOX
#define wxCLOSE_BOX 0x1000
#endif

/*!
 * ReplaceDlg class declaration
 */

class ReplaceDlg: public wxDialog
{    
    DECLARE_CLASS( ReplaceDlg )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    ReplaceDlg(editor_type * edIn, wxString & ini_strIn, bool any_selIn, bool multiline_selIn);
    ~ReplaceDlg(void);
    /// Creation
    bool Create( wxWindow* parent, wxWindowID id = -1, const wxString& caption = _("Replace"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU|wxCLOSE_BOX );

    /// Creates the controls and sizers
    void CreateControls();

////@begin ReplaceDlg event handler declarations

    /// wxEVT_COMMAND_RADIOBOX_SELECTED event handler for CD_REPL_SEL
    void OnCdReplSelSelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_REPL_FIND
    void OnCdReplFindClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_REPL_REPL
    void OnCdReplReplClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_REPL_REPLALL
    void OnCdReplReplallClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CANCEL
    void OnCancelClick( wxCommandEvent& event );

////@end ReplaceDlg event handler declarations
  void OnClose( wxCloseEvent& event );

////@begin ReplaceDlg member function declarations

    /// Retrieves bitmap resources
    wxBitmap GetBitmapResource( const wxString& name );

    /// Retrieves icon resources
    wxIcon GetIconResource( const wxString& name );
////@end ReplaceDlg member function declarations
  void SendEvent(const wxEventType& evtType);

    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin ReplaceDlg member variables
    wxOwnerDrawnComboBox* m_textFind;
    wxOwnerDrawnComboBox* m_textRepl;
    wxCheckBox* m_chkWord;
    wxCheckBox* m_chkCase;
    wxCheckBox* m_chkBack;
    wxRadioBox* m_radioExtent;
    wxButton* m_button_find;
    wxButton* m_button_replace;
////@end ReplaceDlg member variables
  editor_type * ed;
  wxString ini_str;
  bool any_sel, multiline_sel;
};

#endif
    // _REPLACEDLG_H_
