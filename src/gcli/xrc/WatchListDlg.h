/////////////////////////////////////////////////////////////////////////////
// Name:        WatchListDlg.h
// Purpose:     
// Author:      
// Modified by: 
// Created:     03/24/04 14:25:10
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifndef _WATCHLISTDLG_H_
#define _WATCHLISTDLG_H_

#ifdef __GNUG__
//#pragma interface "WatchListDlg.cpp"
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
class wxListCtrl;
////@end forward declarations

/*!
 * Control identifiers
 */

////@begin control identifiers
#define ID_DIALOG 10000
#define CD_WATCH_LIST 10001
////@end control identifiers

/*!
 * Compatibility
 */

#ifndef wxCLOSE_BOX
#define wxCLOSE_BOX 0x1000
#endif

/*!
 * WatchListDlg class declaration
 */

class WatchListDlg: public wxPanel
{    
    DECLARE_CLASS( WatchListDlg )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    WatchListDlg(server_debugger * my_server_debuggerIn);
    /// Creation
    bool Create( wxWindow* parent, wxWindowID id = -1, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU );

    /// Creates the controls and sizers
    void CreateControls();

////@begin WatchListDlg event handler declarations

    /// wxEVT_COMMAND_LIST_END_LABEL_EDIT event handler for CD_WATCH_LIST
    void OnCdWatchListEndLabelEdit( wxListEvent& event );

    /// wxEVT_COMMAND_LIST_KEY_DOWN event handler for CD_WATCH_LIST
    void OnCdWatchListKeyDown( wxListEvent& event );

////@end WatchListDlg event handler declarations

////@begin WatchListDlg member function declarations

////@end WatchListDlg member function declarations
  void recalc(int ind, wxString name);
  void recalc_all(void);
  void append(const wxString & data, int ind = -1);
  void OnDropText(wxCoord x, wxCoord y, const wxString & data);
    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin WatchListDlg member variables
    wxListCtrl* mList;
////@end WatchListDlg member variables
  server_debugger * my_server_debugger;
};

#endif
    // _WATCHLISTDLG_H_
