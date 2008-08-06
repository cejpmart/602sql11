/////////////////////////////////////////////////////////////////////////////
// Name:        ConnSpeedDlg.h
// Purpose:     
// Author:      
// Modified by: 
// Created:     01/15/04 10:31:33
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifndef _CONNSPEEDDLG_H_
#define _CONNSPEEDDLG_H_

#ifdef __GNUG__
//#pragma interface "ConnSpeedDlg.cpp"
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
#define SYMBOL_CONNSPEEDDLG_STYLE wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU|wxCLOSE_BOX
#define SYMBOL_CONNSPEEDDLG_TITLE _("Client-server Connection Speed")
#define SYMBOL_CONNSPEEDDLG_IDNAME ID_DIALOG
#define SYMBOL_CONNSPEEDDLG_SIZE wxSize(400, 300)
#define SYMBOL_CONNSPEEDDLG_POSITION wxDefaultPosition
#define ID_TEXTCTRL 10001
#define ID_TEXTCTRL1 10002
////@end control identifiers

/*!
 * Compatibility
 */

#ifndef wxCLOSE_BOX
#define wxCLOSE_BOX 0x1000
#endif

/*!
 * ConnSpeedDlg class declaration
 */

class ConnSpeedDlg: public wxDialog
{    
    DECLARE_CLASS( ConnSpeedDlg )
    DECLARE_EVENT_TABLE()
public:
    /// Constructors
    ConnSpeedDlg( int speed1In, int speed2In);
    ConnSpeedDlg( wxWindow* parent, wxWindowID id = -1, const wxString& caption = SYMBOL_CONNSPEEDDLG_TITLE, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = SYMBOL_CONNSPEEDDLG_STYLE );

    /// Creation
    bool Create( wxWindow* parent, wxWindowID id = -1, const wxString& caption = SYMBOL_CONNSPEEDDLG_TITLE, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = SYMBOL_CONNSPEEDDLG_STYLE );

    /// Creates the controls and sizers
    void CreateControls();

////@begin ConnSpeedDlg event handler declarations

////@end ConnSpeedDlg event handler declarations

////@begin ConnSpeedDlg member function declarations

    int GetSpeed1() const { return speed1 ; }
    void SetSpeed1(int value) { speed1 = value ; }

    int GetSpeed2() const { return speed2 ; }
    void SetSpeed2(int value) { speed2 = value ; }

    /// Retrieves bitmap resources
    wxBitmap GetBitmapResource( const wxString& name );

    /// Retrieves icon resources
    wxIcon GetIconResource( const wxString& name );
////@end ConnSpeedDlg member function declarations

    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin ConnSpeedDlg member variables
    wxTextCtrl* mSpeed1;
    wxTextCtrl* mSpeed2;
    int speed1;
    int speed2;
////@end ConnSpeedDlg member variables
};

#endif
    // _CONNSPEEDDLG_H_
