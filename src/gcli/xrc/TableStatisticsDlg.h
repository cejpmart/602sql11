/////////////////////////////////////////////////////////////////////////////
// Name:        TableStatisticsDlg.h
// Purpose:     
// Author:      
// Modified by: 
// Created:     05/03/04 12:34:30
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifndef _TABLESTATISTICSDLG_H_
#define _TABLESTATISTICSDLG_H_

#if defined(__GNUG__) && !defined(__APPLE__)
//#pragma interface "TableStatisticsDlg.cpp"
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
#define SYMBOL_TABLESTATISTICSDLG_STYLE wxCAPTION|wxSYSTEM_MENU|wxCLOSE_BOX
#define SYMBOL_TABLESTATISTICSDLG_TITLE _("Table Statistics")
#define SYMBOL_TABLESTATISTICSDLG_IDNAME ID_DIALOG
#define SYMBOL_TABLESTATISTICSDLG_SIZE wxSize(400, 300)
#define SYMBOL_TABLESTATISTICSDLG_POSITION wxDefaultPosition
#define CD_TABSTAT_NAME 10005
#define CD_TABSTAT_VALID 10001
#define CD_TABSTAT_DEL 10002
#define CD_TABSTAT_FREE 10003
#define CD_TABSTAT_TOTAL 10004
////@end control identifiers

/*!
 * Compatibility
 */

#ifndef wxCLOSE_BOX
#define wxCLOSE_BOX 0x1000
#endif

/*!
 * TableStatisticsDlg class declaration
 */

class TableStatisticsDlg: public wxDialog
{    
    DECLARE_CLASS( TableStatisticsDlg )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    TableStatisticsDlg( );
    TableStatisticsDlg( wxWindow* parent, wxWindowID id = SYMBOL_TABLESTATISTICSDLG_IDNAME, const wxString& caption = SYMBOL_TABLESTATISTICSDLG_TITLE, const wxPoint& pos = SYMBOL_TABLESTATISTICSDLG_POSITION, const wxSize& size = SYMBOL_TABLESTATISTICSDLG_SIZE, long style = SYMBOL_TABLESTATISTICSDLG_STYLE );

    /// Creation
    bool Create( wxWindow* parent, wxWindowID id = SYMBOL_TABLESTATISTICSDLG_IDNAME, const wxString& caption = SYMBOL_TABLESTATISTICSDLG_TITLE, const wxPoint& pos = SYMBOL_TABLESTATISTICSDLG_POSITION, const wxSize& size = SYMBOL_TABLESTATISTICSDLG_SIZE, long style = SYMBOL_TABLESTATISTICSDLG_STYLE );

    /// Creates the controls and sizers
    void CreateControls();

////@begin TableStatisticsDlg event handler declarations

////@end TableStatisticsDlg event handler declarations

////@begin TableStatisticsDlg member function declarations

////@end TableStatisticsDlg member function declarations

    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin TableStatisticsDlg member variables
    wxTextCtrl* mName;
    wxTextCtrl* mValid;
    wxTextCtrl* mDel;
    wxTextCtrl* mFree;
    wxTextCtrl* mTotal;
////@end TableStatisticsDlg member variables
};

#endif
    // _TABLESTATISTICSDLG_H_
