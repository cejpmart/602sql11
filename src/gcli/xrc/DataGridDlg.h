/////////////////////////////////////////////////////////////////////////////
// Name:        DataGridDlg.h
// Purpose:     
// Author:      
// Modified by: 
// Created:     12/15/06 09:38:36
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifndef _DATAGRIDDLG_H_
#define _DATAGRIDDLG_H_

#if defined(__GNUG__) && !defined(__APPLE__)
//#pragma interface "DataGridDlg.cpp"
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
class wxBoxSizer;
////@end forward declarations

/*!
 * Control identifiers
 */

////@begin control identifiers
#define ID_MYDIALOG11 10148
#define SYMBOL_DATAGRIDDLG_STYLE wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU|wxCLOSE_BOX
#define SYMBOL_DATAGRIDDLG_TITLE _T("")
#define SYMBOL_DATAGRIDDLG_IDNAME ID_MYDIALOG11
#define SYMBOL_DATAGRIDDLG_SIZE wxSize(400, 300)
#define SYMBOL_DATAGRIDDLG_POSITION wxDefaultPosition
#define CD_DGD_PLACEHOLDER 10000
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
 * DataGridDlg class declaration
 */

class DataGridDlg: public wxDialog
{    
    DECLARE_DYNAMIC_CLASS( DataGridDlg )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    DataGridDlg( );
    DataGridDlg(cdp_t cdpIn, wxString captIn, const char * queryIn, const char * formdefIn);

    /// Creation
    bool Create( wxWindow* parent, wxWindowID id = SYMBOL_DATAGRIDDLG_IDNAME, const wxString& caption = SYMBOL_DATAGRIDDLG_TITLE, const wxPoint& pos = SYMBOL_DATAGRIDDLG_POSITION, const wxSize& size = SYMBOL_DATAGRIDDLG_SIZE, long style = SYMBOL_DATAGRIDDLG_STYLE );

    /// Creates the controls and sizers
    void CreateControls();

////@begin DataGridDlg event handler declarations

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CLOSE
    void OnCloseClick( wxCommandEvent& event );

////@end DataGridDlg event handler declarations

////@begin DataGridDlg member function declarations

    /// Retrieves bitmap resources
    wxBitmap GetBitmapResource( const wxString& name );

    /// Retrieves icon resources
    wxIcon GetIconResource( const wxString& name );
////@end DataGridDlg member function declarations

    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin DataGridDlg member variables
    wxBoxSizer* mGridSizer;
    wxStaticText* mGrid;
////@end DataGridDlg member variables
    cdp_t cdp;
    wxString capt;
    const char * query;
    const char * formdef;
    DataGrid * grid;
};

#endif
    // _DATAGRIDDLG_H_
