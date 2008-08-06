/////////////////////////////////////////////////////////////////////////////
// Name:        ColumnHintsDlg.h
// Purpose:     
// Author:      
// Modified by: 
// Created:     01/30/04 10:50:28
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifndef _COLUMNHINTSDLG_H_
#define _COLUMNHINTSDLG_H_

#ifdef __GNUG__
//#pragma interface "ColumnHintsDlg.cpp"
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
////@end forward declarations

/*!
 * Control identifiers
 */

////@begin control identifiers
#define ID_DIALOG 10000
#define SYMBOL_COLUMNHINTSDLG_STYLE wxRESIZE_BORDER|wxCLOSE_BOX
#define SYMBOL_COLUMNHINTSDLG_TITLE _T("")
#define SYMBOL_COLUMNHINTSDLG_IDNAME ID_DIALOG
#define SYMBOL_COLUMNHINTSDLG_SIZE wxSize(400, 300)
#define SYMBOL_COLUMNHINTSDLG_POSITION wxDefaultPosition
#define CD_TDHI_CAPTION_C 10009
#define CD_TDHI_CAPTION 10001
#define CD_TDHI_HELPTEXT_C 10010
#define CD_TDHI_HELPTEXT 10002
#define CD_TDHI_BUBBLE 10003
#define CD_TDHI_COMBO_QUERY_C 10011
#define CD_TDHI_COMBO_QUERY 10004
#define CD_TDHI_COMBO_TEXT_C 10012
#define CD_TDHI_COMBO_TEXT 10005
#define CD_TDHI_COMBO_VAL_C 10013
#define CD_TDHI_COMBO_VAL 10006
////@end control identifiers

/*!
 * Compatibility
 */

#ifndef wxCLOSE_BOX
#define wxCLOSE_BOX 0x1000
#endif

/*!
 * ColumnHintsDlg class declaration
 */

class ColumnHintsDlg: public wxScrolledWindow
{    
    DECLARE_CLASS( ColumnHintsDlg )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    ColumnHintsDlg( );
    ColumnHintsDlg( wxWindow* parent, wxWindowID id = -1, const wxString& caption = SYMBOL_COLUMNHINTSDLG_TITLE, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU );

    /// Creation
    bool Create( wxWindow* parent, wxWindowID id = -1, const wxString& caption = SYMBOL_COLUMNHINTSDLG_TITLE, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU );

    /// Creates the controls and sizers
    void CreateControls();

////@begin ColumnHintsDlg event handler declarations

    /// wxEVT_COMMAND_TEXT_UPDATED event handler for CD_TDHI_CAPTION
    void OnCdTdhiCaptionUpdated( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_UPDATED event handler for CD_TDHI_HELPTEXT
    void OnCdTdhiHelptextUpdated( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for CD_TDHI_BUBBLE
    void OnCdTdhiBubbleClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_UPDATED event handler for CD_TDHI_COMBO_QUERY
    void OnCdTdhiComboQueryUpdated( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_UPDATED event handler for CD_TDHI_COMBO_TEXT
    void OnCdTdhiComboTextUpdated( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_UPDATED event handler for CD_TDHI_COMBO_VAL
    void OnCdTdhiComboValUpdated( wxCommandEvent& event );

////@end ColumnHintsDlg event handler declarations

////@begin ColumnHintsDlg member function declarations

    /// Retrieves bitmap resources
    wxBitmap GetBitmapResource( const wxString& name );

    /// Retrieves icon resources
    wxIcon GetIconResource( const wxString& name );
////@end ColumnHintsDlg member function declarations

    /// Should we show tooltips?
    static bool ShowToolTips();
    void changed(bool major_change);

////@begin ColumnHintsDlg member variables
////@end ColumnHintsDlg member variables
};

#endif
    // _COLUMNHINTSDLG_H_
