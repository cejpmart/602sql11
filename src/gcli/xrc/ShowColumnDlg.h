/////////////////////////////////////////////////////////////////////////////
// Name:        ShowColumnDlg.h
// Purpose:     
// Author:      
// Modified by: 
// Created:     08/25/05 09:26:58
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifndef _SHOWCOLUMNDLG_H_
#define _SHOWCOLUMNDLG_H_

#if defined(__GNUG__) && !defined(__APPLE__)
//#pragma interface "ShowColumnDlg.cpp"
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
#define ID_MYDIALOG 10068
#define SYMBOL_SHOWCOLUMNDLG_STYLE wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU|wxCLOSE_BOX
#define SYMBOL_SHOWCOLUMNDLG_TITLE _("Show Column")
#define SYMBOL_SHOWCOLUMNDLG_IDNAME ID_MYDIALOG
#define SYMBOL_SHOWCOLUMNDLG_SIZE wxSize(400, 300)
#define SYMBOL_SHOWCOLUMNDLG_POSITION wxDefaultPosition
#define CD_SHOWCOL_NAME 10071
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
 * ShowColumnDlg class declaration
 */

class ShowColumnDlg: public wxDialog
{    
    DECLARE_CLASS( ShowColumnDlg )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    ShowColumnDlg(wxGrid * gridIn);

    /// Creation
    bool Create( wxWindow* parent, wxWindowID id = SYMBOL_SHOWCOLUMNDLG_IDNAME, const wxString& caption = SYMBOL_SHOWCOLUMNDLG_TITLE, const wxPoint& pos = SYMBOL_SHOWCOLUMNDLG_POSITION, const wxSize& size = SYMBOL_SHOWCOLUMNDLG_SIZE, long style = SYMBOL_SHOWCOLUMNDLG_STYLE );

    /// Creates the controls and sizers
    void CreateControls();

////@begin ShowColumnDlg event handler declarations

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_OK
    void OnOkClick( wxCommandEvent& event );

////@end ShowColumnDlg event handler declarations

////@begin ShowColumnDlg member function declarations

    /// Retrieves bitmap resources
    wxBitmap GetBitmapResource( const wxString& name );

    /// Retrieves icon resources
    wxIcon GetIconResource( const wxString& name );
////@end ShowColumnDlg member function declarations

    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin ShowColumnDlg member variables
    wxOwnerDrawnComboBox* mShowcolName;
////@end ShowColumnDlg member variables
    wxGrid * grid;
};

#endif
    // _SHOWCOLUMNDLG_H_
