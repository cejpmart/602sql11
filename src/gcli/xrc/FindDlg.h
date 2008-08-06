/////////////////////////////////////////////////////////////////////////////
// Name:        FindDlg.h
// Purpose:     
// Author:      
// Modified by: 
// Created:     03/19/04 14:31:34
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifndef _FINDDLG_H_
#define _FINDDLG_H_

#ifdef __GNUG__
//#pragma interface "FindDlg.cpp"
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
#define SYMBOL_FINDDLG_STYLE wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU|wxCLOSE_BOX
#define SYMBOL_FINDDLG_TITLE _("Find")
#define SYMBOL_FINDDLG_IDNAME ID_DIALOG
#define SYMBOL_FINDDLG_SIZE wxSize(400, 300)
#define SYMBOL_FINDDLG_POSITION wxDefaultPosition
#define CD_FIND_FINDSTR 10003
#define CD_FIND_WORD 10006
#define CD_FIND_CASE 10007
#define CD_FIND_BACK 10008
#define CD_FIND_FIND 10001
////@end control identifiers

/*!
 * Compatibility
 */

#ifndef wxCLOSE_BOX
#define wxCLOSE_BOX 0x1000
#endif

/*!
 * FindDlg class declaration
 */

class FindDlg: public wxDialog
{    
    DECLARE_CLASS( FindDlg )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    FindDlg(editor_type * edIn, wxString & ini_strIn);
    ~FindDlg(void);

    /// Creation
    bool Create( wxWindow* parent, wxWindowID id = -1, const wxString& caption = _("Find"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = SYMBOL_FINDDLG_STYLE );

    /// Creates the controls and sizers
    void CreateControls();

////@begin FindDlg event handler declarations

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_FIND_FIND
    void OnCdFindFindClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CANCEL
    void OnCancelClick( wxCommandEvent& event );

////@end FindDlg event handler declarations
  void OnClose( wxCloseEvent& event );

////@begin FindDlg member function declarations

    /// Retrieves bitmap resources
    wxBitmap GetBitmapResource( const wxString& name );

    /// Retrieves icon resources
    wxIcon GetIconResource( const wxString& name );
////@end FindDlg member function declarations
  void SendEvent(const wxEventType& evtType);

    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin FindDlg member variables
    wxOwnerDrawnComboBox* m_textFind;
    wxCheckBox* m_chkWord;
    wxCheckBox* m_chkCase;
    wxCheckBox* m_chkBack;
    wxButton* m_button_find;
////@end FindDlg member variables
  editor_type * ed;
  wxString ini_str;
};

#endif
    // _FINDDLG_H_
