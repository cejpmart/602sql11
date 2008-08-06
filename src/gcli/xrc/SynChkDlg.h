/////////////////////////////////////////////////////////////////////////////
// Name:        SynChkDlg.h
// Purpose:     
// Author:      
// Modified by: 
// Created:     01/21/04 15:50:58
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifndef _SYNCHKDLG_H_
#define _SYNCHKDLG_H_

#ifdef __GNUG__
//#pragma interface "SynChkDlg.cpp"
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
#define CD_SYNCHK_OBJECT 10001
#define CD_SYNCHK_COUNT 10002
////@end control identifiers

/*!
 * Compatibility
 */

#ifndef wxCLOSE_BOX
#define wxCLOSE_BOX 0x1000
#endif

/*!
 * SynChkDlg class declaration
 */

class SynChkDlg: public wxDialog
{    
    DECLARE_CLASS( SynChkDlg )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    SynChkDlg( );
    SynChkDlg( wxWindow* parent, wxWindowID id = -1, const wxString& caption = _("Checking syntax of application components"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU );

    /// Creation
    bool Create( wxWindow* parent, wxWindowID id = -1, const wxString& caption = _("Checking syntax of application components"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU );

    /// Creates the controls and sizers
    void CreateControls();

////@begin SynChkDlg event handler declarations

////@end SynChkDlg event handler declarations

////@begin SynChkDlg member function declarations

////@end SynChkDlg member function declarations

    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin SynChkDlg member variables
////@end SynChkDlg member variables
};

#endif
    // _SYNCHKDLG_H_
