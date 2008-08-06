/////////////////////////////////////////////////////////////////////////////
// Name:        LayoutManagerDlg.h
// Purpose:     
// Author:      
// Modified by: 
// Created:     04/14/04 10:00:13
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifndef _LAYOUTMANAGERDLG_H_
#define _LAYOUTMANAGERDLG_H_

#ifdef __GNUG__
//#pragma interface "LayoutManagerDlg.cpp"
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
#define SYMBOL_LAYOUTMANAGERDLG_STYLE wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU|wxCLOSE_BOX
#define SYMBOL_LAYOUTMANAGERDLG_TITLE _("Frame Layout Manager")
#define SYMBOL_LAYOUTMANAGERDLG_IDNAME ID_DIALOG
#define SYMBOL_LAYOUTMANAGERDLG_SIZE wxSize(400, 300)
#define SYMBOL_LAYOUTMANAGERDLG_POSITION wxDefaultPosition
#define CD_FL_LIST 10001
#define CD_FL_TOP 10002
#define CD_FL_LEFT 10003
#define CD_FL_RIGHT 10004
#define CD_FL_BOTTOM 10005
#define CD_FL_FLOATING 10006
#define CD_FL_HIDDEN 10007
#define CD_FL_RESTORE 10008
////@end control identifiers

/*!
 * Compatibility
 */

#ifndef wxCLOSE_BOX
#define wxCLOSE_BOX 0x1000
#endif

/*!
 * LayoutManagerDlg class declaration
 */

class LayoutManagerDlg: public wxDialog
{    
    DECLARE_CLASS( LayoutManagerDlg )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    LayoutManagerDlg( );

    /// Creation
    bool Create( wxWindow* parent, wxWindowID id = -1, const wxString& caption = _("Frame Layout Manager"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU|wxCLOSE_BOX );

    /// Creates the controls and sizers
    void CreateControls();

////@begin LayoutManagerDlg event handler declarations

    /// wxEVT_COMMAND_LISTBOX_SELECTED event handler for CD_FL_LIST
    void OnCdFlListSelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for CD_FL_TOP
    void OnCdFlTopSelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for CD_FL_LEFT
    void OnCdFlLeftSelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for CD_FL_RIGHT
    void OnCdFlRightSelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for CD_FL_BOTTOM
    void OnCdFlBottomSelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for CD_FL_FLOATING
    void OnCdFlFloatingSelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for CD_FL_HIDDEN
    void OnCdFlHiddenSelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CANCEL
    void OnCancelClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_FL_RESTORE
    void OnCdFlRestoreClick( wxCommandEvent& event );

////@end LayoutManagerDlg event handler declarations

////@begin LayoutManagerDlg member function declarations

////@end LayoutManagerDlg member function declarations
  MyFrame * frame;
    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin LayoutManagerDlg member variables
    wxListBox* mList;
    wxRadioButton* mTop;
    wxRadioButton* mLeft;
    wxRadioButton* mRight;
    wxRadioButton* mBottom;
    wxRadioButton* mFloating;
    wxRadioButton* mHidden;
////@end LayoutManagerDlg member variables
};

#endif
    // _LAYOUTMANAGERDLG_H_
