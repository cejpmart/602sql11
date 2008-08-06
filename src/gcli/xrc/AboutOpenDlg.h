/////////////////////////////////////////////////////////////////////////////
// Name:        AboutOpenDlg.h
// Purpose:     
// Author:      
// Modified by: 
// Created:     06/18/07 11:02:12
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifndef _ABOUTOPENDLG_H_
#define _ABOUTOPENDLG_H_

#if defined(__GNUG__) && !defined(__APPLE__)
//#pragma interface "AboutOpenDlg.cpp"
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
class wxHyperlinkCtrl;
////@end forward declarations

/*!
 * Control identifiers
 */

////@begin control identifiers
#define ID_MYDIALOG13 10160
#define SYMBOL_ABOUTOPENDLG_STYLE wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU|wxCLOSE_BOX
#define SYMBOL_ABOUTOPENDLG_TITLE _("About 602SQL Open Server")
#define SYMBOL_ABOUTOPENDLG_IDNAME ID_MYDIALOG13
#define SYMBOL_ABOUTOPENDLG_SIZE wxSize(400, 300)
#define SYMBOL_ABOUTOPENDLG_POSITION wxDefaultPosition
#define CD_ABOUT_BITMAP 10000
#define CD_ABOUT_BUILD 10002
#define CD_ABOUT_URL 10161
#define CD_ABOUT_LICENC 10001
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
 * AboutOpenDlg class declaration
 */

class AboutOpenDlg: public wxDialog
{    
    DECLARE_DYNAMIC_CLASS( AboutOpenDlg )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    AboutOpenDlg( );
    AboutOpenDlg( wxWindow* parent, wxWindowID id = SYMBOL_ABOUTOPENDLG_IDNAME, const wxString& caption = SYMBOL_ABOUTOPENDLG_TITLE, const wxPoint& pos = SYMBOL_ABOUTOPENDLG_POSITION, const wxSize& size = SYMBOL_ABOUTOPENDLG_SIZE, long style = SYMBOL_ABOUTOPENDLG_STYLE );

    /// Creation
    bool Create( wxWindow* parent, wxWindowID id = SYMBOL_ABOUTOPENDLG_IDNAME, const wxString& caption = SYMBOL_ABOUTOPENDLG_TITLE, const wxPoint& pos = SYMBOL_ABOUTOPENDLG_POSITION, const wxSize& size = SYMBOL_ABOUTOPENDLG_SIZE, long style = SYMBOL_ABOUTOPENDLG_STYLE );

    /// Creates the controls and sizers
    void CreateControls();

////@begin AboutOpenDlg event handler declarations

////@end AboutOpenDlg event handler declarations

////@begin AboutOpenDlg member function declarations

    /// Retrieves bitmap resources
    wxBitmap GetBitmapResource( const wxString& name );

    /// Retrieves icon resources
    wxIcon GetIconResource( const wxString& name );
////@end AboutOpenDlg member function declarations

    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin AboutOpenDlg member variables
    wxStaticBitmap* mBitmap;
    wxBoxSizer* mSizer;
    wxStaticText* mMainName;
    wxStaticText* mBuild;
    wxStaticText* mCopyr1;
    wxStaticText* mCopyr2;
    wxHyperlinkCtrl* mURL;
    wxTextCtrl* mLicenc;
////@end AboutOpenDlg member variables
};

#endif
    // _ABOUTOPENDLG_H_
