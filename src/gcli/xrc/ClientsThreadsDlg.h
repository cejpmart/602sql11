/////////////////////////////////////////////////////////////////////////////
// Name:        ClientsThreadsDlg.h
// Purpose:     
// Author:      
// Modified by: 
// Created:     03/17/04 14:10:52
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifndef _CLIENTSTHREADSDLG_H_
#define _CLIENTSTHREADSDLG_H_

#ifdef __GNUG__
//#pragma interface "ClientsThreadsDlg.cpp"
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
#define SYMBOL_CLIENTSTHREADSDLG_STYLE wxFULL_REPAINT_ON_RESIZE  // wxFULL_REPAINT_ON_RESIZE needed for FL panes since WX 2.6.0
#define SYMBOL_CLIENTSTHREADSDLG_TITLE _T("")
#define SYMBOL_CLIENTSTHREADSDLG_IDNAME ID_DIALOG
#define SYMBOL_CLIENTSTHREADSDLG_SIZE wxSize(400, 300)
#define SYMBOL_CLIENTSTHREADSDLG_POSITION wxDefaultPosition
#define CD_THR_PLACEHOLDER 10003
////@end control identifiers

/*!
 * Compatibility
 */

#ifndef wxCLOSE_BOX
#define wxCLOSE_BOX 0x1000
#endif

/*!
 * ClientsThreadsDlg class declaration
 */

class ClientsThreadsDlg: public wxPanel
{    
    DECLARE_CLASS( ClientsThreadsDlg )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    ClientsThreadsDlg( );
    /// Creation
    bool Create( wxWindow* parent, wxWindowID id = -1, const wxString& caption = SYMBOL_CLIENTSTHREADSDLG_TITLE, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = SYMBOL_CLIENTSTHREADSDLG_STYLE);
    /// Creates the controls and sizers
    void CreateControls();

////@begin ClientsThreadsDlg event handler declarations

////@end ClientsThreadsDlg event handler declarations

////@begin ClientsThreadsDlg member function declarations

////@end ClientsThreadsDlg member function declarations
    void open_subwindow(wxWindow * wnd);
    bool open_monitor_page(cdp_t cdp, DataGrid * grid, const char * query, const char * form);
    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin ClientsThreadsDlg member variables
////@end ClientsThreadsDlg member variables
};

#endif
    // _CLIENTSTHREADSDLG_H_
