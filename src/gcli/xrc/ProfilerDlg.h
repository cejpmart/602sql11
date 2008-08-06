/////////////////////////////////////////////////////////////////////////////
// Name:        ProfilerDlg.h
// Purpose:     
// Author:      
// Modified by: 
// Created:     02/20/06 11:41:13
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifndef _PROFILERDLG_H_
#define _PROFILERDLG_H_

#if defined(__GNUG__) && !defined(__APPLE__)
//#pragma interface "ProfilerDlg.cpp"
#endif

/*!
 * Includes
 */

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
#define ID_MYDIALOG2 10083
#define SYMBOL_PROFILERDLG_STYLE wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU|wxCLOSE_BOX|wxMINIMIZE_BOX
#define SYMBOL_PROFILERDLG_TITLE _("Profiler")
#define SYMBOL_PROFILERDLG_IDNAME ID_MYDIALOG2
#define SYMBOL_PROFILERDLG_SIZE wxSize(400, 300)
#define SYMBOL_PROFILERDLG_POSITION wxDefaultPosition
#define CD_PROFILE_ALL 10084
#define CD_PROFILE_SPECIF 10086
#define CD_PROFILE_REFRESH 10085
#define CD_PROFILE_LIST 10000
#define CD_PROFILE_LINES 10001
#define CD_PROFILE_CLEAR 10087
#define CD_PROFILE_SHOW 10088
#define CD_PROFILE_SHOW_LINES 10089
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
 * ProfilerDlg class declaration
 */

class ProfilerDlg: public wxDialog
{    
    DECLARE_CLASS( ProfilerDlg )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    ProfilerDlg(cdp_t cdpIn);
    ~ProfilerDlg(void);

    /// Creation
    bool Create( wxWindow* parent, wxWindowID id = SYMBOL_PROFILERDLG_IDNAME, const wxString& caption = SYMBOL_PROFILERDLG_TITLE, const wxPoint& pos = SYMBOL_PROFILERDLG_POSITION, const wxSize& size = SYMBOL_PROFILERDLG_SIZE, long style = SYMBOL_PROFILERDLG_STYLE );

    /// Creates the controls and sizers
    void CreateControls();

////@begin ProfilerDlg event handler declarations

    /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for CD_PROFILE_ALL
    void OnCdProfileAllClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_PROFILE_REFRESH
    void OnCdProfileRefreshClick( wxCommandEvent& event );

    /// wxEVT_GRID_CMD_CELL_CHANGE event handler for CD_PROFILE_LIST
    void OnCdProfileListCellChange( wxGridEvent& event );

    /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for CD_PROFILE_LINES
    void OnCdProfileLinesClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_PROFILE_CLEAR
    void OnCdProfileClearClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_PROFILE_SHOW
    void OnCdProfileShowClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_PROFILE_SHOW_LINES
    void OnCdProfileShowLinesClick( wxCommandEvent& event );

////@end ProfilerDlg event handler declarations
    void OnCdProfileCancel( wxCommandEvent& event );

////@begin ProfilerDlg member function declarations

    /// Retrieves bitmap resources
    wxBitmap GetBitmapResource( const wxString& name );

    /// Retrieves icon resources
    wxIcon GetIconResource( const wxString& name );
////@end ProfilerDlg member function declarations
    void CreateGrid(void);
    void RefreshGrid(void);
    void disconnecting(cdp_t dis_cdp);

    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin ProfilerDlg member variables
    wxCheckBox* mAll;
    wxStaticText* mSpecif;
    wxButton* mRefresh;
    wxBoxSizer* mGridSizer;
    wxGrid* mGrid;
    wxCheckBox* mLines;
    wxButton* mClear;
    wxButton* mShow;
    wxButton* mShowLines;
////@end ProfilerDlg member variables
  cdp_t cdp;
  int lock;
};

extern ProfilerDlg * profiler_dlg;

#endif
    // _PROFILERDLG_H_
