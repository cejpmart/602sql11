/////////////////////////////////////////////////////////////////////////////
// Name:        ExtEditorsDlg.h
// Purpose:     
// Author:      
// Modified by: 
// Created:     07/25/06 15:34:27
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifndef _EXTEDITORSDLG_H_
#define _EXTEDITORSDLG_H_

#if defined(__GNUG__) && !defined(__APPLE__)
//#pragma interface "ExtEditorsDlg.cpp"
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
#define ID_MYDIALOG8 10117
#define SYMBOL_EXTEDITORSDLG_STYLE wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU|wxCLOSE_BOX
#define SYMBOL_EXTEDITORSDLG_TITLE _("External Object Editors")
#define SYMBOL_EXTEDITORSDLG_IDNAME ID_MYDIALOG8
#define SYMBOL_EXTEDITORSDLG_SIZE wxSize(400, 300)
#define SYMBOL_EXTEDITORSDLG_POSITION wxDefaultPosition
#define CD_EE_STSH 10118
#define CD_EE_STSH_BROWSE 10119
#define CD_EE_CSS 10120
#define CD_EE_CSS_BROWSE 10121
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
 * ExtEditorsDlg class declaration
 */

class ExtEditorsDlg: public wxDialog
{    
    DECLARE_DYNAMIC_CLASS( ExtEditorsDlg )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    ExtEditorsDlg( );
    ExtEditorsDlg( wxWindow* parent, wxWindowID id = SYMBOL_EXTEDITORSDLG_IDNAME, const wxString& caption = SYMBOL_EXTEDITORSDLG_TITLE, const wxPoint& pos = SYMBOL_EXTEDITORSDLG_POSITION, const wxSize& size = SYMBOL_EXTEDITORSDLG_SIZE, long style = SYMBOL_EXTEDITORSDLG_STYLE );

    /// Creation
    bool Create( wxWindow* parent, wxWindowID id = SYMBOL_EXTEDITORSDLG_IDNAME, const wxString& caption = SYMBOL_EXTEDITORSDLG_TITLE, const wxPoint& pos = SYMBOL_EXTEDITORSDLG_POSITION, const wxSize& size = SYMBOL_EXTEDITORSDLG_SIZE, long style = SYMBOL_EXTEDITORSDLG_STYLE );

    /// Creates the controls and sizers
    void CreateControls();

////@begin ExtEditorsDlg event handler declarations

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_EE_STSH_BROWSE
    void OnCdEeStshBrowseClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_EE_CSS_BROWSE
    void OnCdEeCssBrowseClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_OK
    void OnOkClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CANCEL
    void OnCancelClick( wxCommandEvent& event );

////@end ExtEditorsDlg event handler declarations

////@begin ExtEditorsDlg member function declarations

    /// Retrieves bitmap resources
    wxBitmap GetBitmapResource( const wxString& name );

    /// Retrieves icon resources
    wxIcon GetIconResource( const wxString& name );
////@end ExtEditorsDlg member function declarations

    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin ExtEditorsDlg member variables
    wxTextCtrl* mStsh;
    wxTextCtrl* mCss;
////@end ExtEditorsDlg member variables
};

#endif
    // _EXTEDITORSDLG_H_
