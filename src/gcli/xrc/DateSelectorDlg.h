/////////////////////////////////////////////////////////////////////////////
// Name:        DateSelectorDlg.h
// Purpose:     
// Author:      
// Modified by: 
// Created:     03/24/04 09:42:19
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifndef _DATESELECTORDLG_H_
#define _DATESELECTORDLG_H_

#ifdef __GNUG__
#pragma interface "DateSelectorDlg.cpp"
#endif

/*!
 * Includes
 */

////@begin includes
#include "wx/calctrl.h"
////@end includes

/*!
 * Forward declarations
 */

////@begin forward declarations
class wxBoxSizer;
class wxCalendarCtrl;
////@end forward declarations

/*!
 * Control identifiers
 */

////@begin control identifiers
#define ID_DIALOG 10000
#define SYMBOL_DATESELECTORDLG_STYLE wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU|wxCLOSE_BOX
#define SYMBOL_DATESELECTORDLG_TITLE _("Date Selector")
#define SYMBOL_DATESELECTORDLG_IDNAME ID_DIALOG
#define SYMBOL_DATESELECTORDLG_SIZE wxSize(200, 160)
#define SYMBOL_DATESELECTORDLG_POSITION wxDefaultPosition
#define ID_CALCTRL 10001
////@end control identifiers

/*!
 * Compatibility
 */

#ifndef wxCLOSE_BOX
#define wxCLOSE_BOX 0x1000
#endif

/*!
 * DateSelectorDlg class declaration
 */

class DateSelectorDlg: public wxDialog
{    
    DECLARE_CLASS( DateSelectorDlg )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    DateSelectorDlg(wxDateTime * dtmIn);
    /// Creation
    bool Create( wxWindow* parent, wxWindowID id = -1, const wxString& caption = SYMBOL_DATESELECTORDLG_TITLE, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = SYMBOL_DATESELECTORDLG_STYLE);

    /// Creates the controls and sizers
    void CreateControls();

////@begin DateSelectorDlg event handler declarations

    /// wxEVT_CALENDAR_DOUBLECLICKED event handler for ID_CALCTRL
    void OnCalctrlDoubleClicked( wxCalendarEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CANCEL
    void OnCancelClick( wxCommandEvent& event );

////@end DateSelectorDlg event handler declarations

////@begin DateSelectorDlg member function declarations

////@end DateSelectorDlg member function declarations

    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin DateSelectorDlg member variables
    wxBoxSizer* mSizer;
    wxCalendarCtrl* mCal;
////@end DateSelectorDlg member variables
    wxDateTime * p_dtm;
};

#endif
    // _DATESELECTORDLG_H_
