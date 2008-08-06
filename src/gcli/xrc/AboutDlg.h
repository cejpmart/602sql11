/////////////////////////////////////////////////////////////////////////////
// Name:        AboutDlg.h
// Purpose:     
// Author:      
// Modified by: 
// Created:     01/12/04 17:54:10
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifndef _ABOUTDLG_H_
#define _ABOUTDLG_H_

#ifdef __GNUG__
//#pragma interface "AboutDlg.cpp"
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
#define ID_DIALOG 10000
#define SYMBOL_ABOUTDLG_STYLE wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU|wxCLOSE_BOX
#define SYMBOL_ABOUTDLG_TITLE _("About 602sql")
#define SYMBOL_ABOUTDLG_IDNAME ID_DIALOG
#define SYMBOL_ABOUTDLG_SIZE wxSize(400, 300)
#define SYMBOL_ABOUTDLG_POSITION wxDefaultPosition
#define CD_ABOUT_BITMAP 10003
#define CD_ABOUT_BUILD 10002
#define CD_ABOUT_LICENC 10001
////@end control identifiers

/*!
 * Compatibility
 */

#ifndef wxCLOSE_BOX
#define wxCLOSE_BOX 0x1000
#endif

/*!
 * AboutDlg class declaration
 */

class AboutDlg: public wxDialog
{    
    DECLARE_CLASS( AboutDlg )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    AboutDlg( );
    AboutDlg( wxWindow* parent, wxWindowID id = -1, const wxString& caption = SYMBOL_ABOUTDLG_TITLE, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = SYMBOL_ABOUTDLG_STYLE );

    /// Creation
    bool Create( wxWindow* parent, wxWindowID id = -1, const wxString& caption = SYMBOL_ABOUTDLG_TITLE, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = SYMBOL_ABOUTDLG_STYLE );

    /// Creates the controls and sizers
    void CreateControls();

////@begin AboutDlg event handler declarations

////@end AboutDlg event handler declarations

////@begin AboutDlg member function declarations

    /// Retrieves bitmap resources
    wxBitmap GetBitmapResource( const wxString& name );

    /// Retrieves icon resources
    wxIcon GetIconResource( const wxString& name );
////@end AboutDlg member function declarations

    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin AboutDlg member variables
    wxStaticBitmap* mBitmap;
    wxBoxSizer* mSizer;
    wxStaticText* mMainName;
    wxStaticText* mBuild;
    wxStaticText* mCopyr1;
    wxStaticText* mCopyr2;
    wxTextCtrl* mLicenc;
////@end AboutDlg member variables
};

#endif
    // _ABOUTDLG_H_
