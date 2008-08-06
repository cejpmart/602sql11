/////////////////////////////////////////////////////////////////////////////
// Name:        MonitorLogDlg.h
// Purpose:     
// Author:      
// Modified by: 
// Created:     04/30/04 12:31:07
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifndef _MONITORLOGDLG_H_
#define _MONITORLOGDLG_H_

#if defined(__GNUG__) && !defined(__APPLE__)
//#pragma interface "MonitorLogDlg.cpp"
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
#define SYMBOL_MONITORLOGDLG_STYLE wxFULL_REPAINT_ON_RESIZE  // wxFULL_REPAINT_ON_RESIZE needed for FL panes since WX 2.6.0
#define SYMBOL_MONITORLOGDLG_TITLE _T("")
#define SYMBOL_MONITORLOGDLG_IDNAME ID_DIALOG
#define SYMBOL_MONITORLOGDLG_SIZE wxSize(400, 300)
#define SYMBOL_MONITORLOGDLG_POSITION wxDefaultPosition
#define CD_LOG_LIST 10001
#define CD_LOG_LINE 10002
#define CD_LOG_FILE 10003
////@end control identifiers

/*!
 * Compatibility
 */

#ifndef wxCLOSE_BOX
#define wxCLOSE_BOX 0x1000
#endif

/*!
 * MonitorLogDlg class declaration
 */

class MonitorLogDlg;

class LogRefreshHandler : public wxEvtHandler
{ MonitorLogDlg * log;
 public:
  LogRefreshHandler(MonitorLogDlg * dlgIn)
    { log=dlgIn; }
  void OnRightClick(wxMouseEvent & event);
  DECLARE_EVENT_TABLE()
};

class MonitorLogDlg: public wxPanel
{    
    DECLARE_CLASS( MonitorLogDlg )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    MonitorLogDlg(cdp_t cdpIn, const char * log_nameIn);
    MonitorLogDlg( wxWindow* parent, wxWindowID id = SYMBOL_MONITORLOGDLG_IDNAME, const wxString& caption = SYMBOL_MONITORLOGDLG_TITLE, const wxPoint& pos = SYMBOL_MONITORLOGDLG_POSITION, const wxSize& size = SYMBOL_MONITORLOGDLG_SIZE, long style = SYMBOL_MONITORLOGDLG_STYLE );
    ~MonitorLogDlg(void)
      { if (mList) mList->PopEventHandler(true); }  // deletes the handler
    /// Creation
    bool Create( wxWindow* parent, wxWindowID id = SYMBOL_MONITORLOGDLG_IDNAME, const wxString& caption = SYMBOL_MONITORLOGDLG_TITLE, const wxPoint& pos = SYMBOL_MONITORLOGDLG_POSITION, const wxSize& size = SYMBOL_MONITORLOGDLG_SIZE, long style = SYMBOL_MONITORLOGDLG_STYLE );

    /// Creates the controls and sizers
    void CreateControls();

////@begin MonitorLogDlg event handler declarations

    /// wxEVT_COMMAND_LISTBOX_SELECTED event handler for CD_LOG_LIST
    void OnCdLogListSelected( wxCommandEvent& event );

////@end MonitorLogDlg event handler declarations

////@begin MonitorLogDlg member function declarations

////@end MonitorLogDlg member function declarations
  void refresh_log(void);
  void OnCommand(wxCommandEvent & event);

    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin MonitorLogDlg member variables
    wxListBox* mList;
    wxTextCtrl* mLine;
    wxStaticText* mFile;
////@end MonitorLogDlg member variables
  cdp_t cdp;
  tobjname log_name;
};

#endif
    // _MONITORLOGDLG_H_
