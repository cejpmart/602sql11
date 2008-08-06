/////////////////////////////////////////////////////////////////////////////
// Name:        ColumnBasicsDlg.h
// Purpose:     
// Author:      
// Modified by: 
// Created:     01/28/04 17:01:59
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifndef _COLUMNBASICSDLG_H_
#define _COLUMNBASICSDLG_H_

#ifdef __GNUG__
//#pragma interface "ColumnBasicsDlg.cpp"
#endif

/*!
 * Includes
 */

////@begin includes
#include "wx/spinctrl.h"
////@end includes

/*!
 * Forward declarations
 */

////@begin forward declarations
class wxOwnerDrawnComboBox;
class wxSpinCtrl;
////@end forward declarations

/*!
 * Control identifiers
 */

////@begin control identifiers
#define ID_DIALOG 10000
#define SYMBOL_COLUMNBASICSDLG_STYLE wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU|wxCLOSE_BOX
#define SYMBOL_COLUMNBASICSDLG_TITLE _T("")
#define SYMBOL_COLUMNBASICSDLG_IDNAME ID_DIALOG
#define SYMBOL_COLUMNBASICSDLG_SIZE wxSize(400, 300)
#define SYMBOL_COLUMNBASICSDLG_POSITION wxDefaultPosition
#define CD_TDCB_UNICODE 10002
#define CD_TDCB_IGNORE 10003
#define CD_TDCB_LANG_CAPT 10007
#define CD_TDCB_LANG 10004
#define CD_TDCB_SCALE_CAPT 10008
#define CD_TDCB_SCALE 10001
#define CD_TDCB_COMMENT 10006
////@end control identifiers

/*!
 * Compatibility
 */

#ifndef wxCLOSE_BOX
#define wxCLOSE_BOX 0x1000
#endif

/*!
 * ColumnBasicsDlg class declaration
 */

class ColumnBasicsDlg: public wxScrolledWindow
{    
    DECLARE_CLASS( ColumnBasicsDlg )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    ColumnBasicsDlg( );
    ColumnBasicsDlg( wxWindow* parent, wxWindowID id = -1, const wxString& caption = SYMBOL_COLUMNBASICSDLG_TITLE, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU );

    /// Creation
    bool Create( wxWindow* parent, wxWindowID id = -1, const wxString& caption = SYMBOL_COLUMNBASICSDLG_TITLE, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU );

    /// Creates the controls and sizers
    void CreateControls();

////@begin ColumnBasicsDlg event handler declarations

    /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for CD_TDCB_UNICODE
    void OnCdTdcbUnicodeClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for CD_TDCB_IGNORE
    void OnCdTdcbIgnoreClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_COMBOBOX_SELECTED event handler for CD_TDCB_LANG
    void OnCdTdcbLangSelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_UPDATED event handler for CD_TDCB_LANG
    void OnCdTdcbLangUpdated( wxCommandEvent& event );

    /// wxEVT_COMMAND_SPINCTRL_UPDATED event handler for CD_TDCB_SCALE
    void OnCdTdcbScaleUpdated( wxSpinEvent& event );

    /// wxEVT_COMMAND_TEXT_UPDATED event handler for CD_TDCB_SCALE
    void OnCdTdcbScaleTextUpdated( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_UPDATED event handler for CD_TDCB_COMMENT
    void OnCdTdcbCommentUpdated( wxCommandEvent& event );

////@end ColumnBasicsDlg event handler declarations

////@begin ColumnBasicsDlg member function declarations

    /// Retrieves bitmap resources
    wxBitmap GetBitmapResource( const wxString& name );

    /// Retrieves icon resources
    wxIcon GetIconResource( const wxString& name );
////@end ColumnBasicsDlg member function declarations

    /// Should we show tooltips?
    static bool ShowToolTips();
    void changed(bool major_change);

////@begin ColumnBasicsDlg member variables
    wxCheckBox* mUnicode;
    wxCheckBox* mIgnore;
    wxStaticText* mLangCapt;
    wxOwnerDrawnComboBox* mLang;
    wxStaticText* mScaleCapt;
    wxSpinCtrl* mScale;
    wxStaticText* mCommentCapt;
    wxTextCtrl* mComment;
////@end ColumnBasicsDlg member variables
};

#endif
    // _COLUMNBASICSDLG_H_
