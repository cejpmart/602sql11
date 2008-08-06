/////////////////////////////////////////////////////////////////////////////
// Name:        CallStackDlg.h
// Purpose:     
// Author:      
// Modified by: 
// Created:     03/20/04 08:09:59
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifndef _CALLSTACKDLG_H_
#define _CALLSTACKDLG_H_

#ifdef __GNUG__
//#pragma interface "CallStackDlg.cpp"
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
#define CD_CALLSTACK_LIST 10001
////@end control identifiers

/*!
 * Compatibility
 */

#ifndef wxCLOSE_BOX
#define wxCLOSE_BOX 0x1000
#endif

/*!
 * CallStackDlg class declaration
 */

class CallStackDlg: public wxPanel
{    
    DECLARE_CLASS( CallStackDlg )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    CallStackDlg(server_debugger * my_server_debuggerIn);
    ~CallStackDlg(void);
    /// Creation
    bool Create( wxWindow* parent, wxWindowID id = -1, const wxString& caption = _("Call Stack"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU );

    /// Creates the controls and sizers
    void CreateControls();

////@begin CallStackDlg event handler declarations

    /// wxEVT_COMMAND_LISTBOX_DOUBLECLICKED event handler for CD_CALLSTACK_LIST
    void OnCdCallstackListDoubleClicked( wxCommandEvent& event );

////@end CallStackDlg event handler declarations

////@begin CallStackDlg member function declarations

////@end CallStackDlg member function declarations
  void update(bool breaked);

    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin CallStackDlg member variables
    wxListBox* mListBox;
////@end CallStackDlg member variables
  bool running_info;   // the text that the debugged process is running is displayed in the list box
  t_stack_info9 * stk;  // the most recent description of the frames on the stack, owned by the dialog
  server_debugger * my_server_debugger;
};

#endif
    // _CALLSTACKDLG_H_
