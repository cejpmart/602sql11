/////////////////////////////////////////////////////////////////////////////
// Name:        impexpprog.h
// Purpose:     
// Author:      
// Modified by: 
// Created:     04/26/04 15:17:22
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////
       
#ifndef _IMPEXPPROG_H_
#define _IMPEXPPROG_H_
       
#ifdef __GNUG__
//#pragma interface "impexpprog.cpp"
#endif

/*!
 * Includes
 */

////@begin includes
#include "wx/listctrl.h"
////@end includes

/*!
 * Forward declarations
 */

////@begin forward declarations
class wxListCtrl;
////@end forward declarations

/*!
 * Control identifiers
 */

////@begin control identifiers
#define ID_DIALOG 10000
#define SYMBOL_CIMPEXPPROGDLG_STYLE wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU|wxCLOSE_BOX
#define SYMBOL_CIMPEXPPROGDLG_TITLE _("Dialog")
#define SYMBOL_CIMPEXPPROGDLG_IDNAME ID_DIALOG
#define SYMBOL_CIMPEXPPROGDLG_SIZE wxSize(400, 300)
#define SYMBOL_CIMPEXPPROGDLG_POSITION wxDefaultPosition
#define ID_CATEGED 10001
#define ID_NAMEED 10002
#define ID_LISTCTRL 10003
#define ID_GAUGE 10004
////@end control identifiers
/*!
 * Compatibility
 */

#ifndef wxCLOSE_BOX
#define wxCLOSE_BOX 0x1000
#endif

/*!
 * CImpExpProgDlg class declaration
 */

class CImpExpProgDlg: public wxDialog
{    
    DECLARE_CLASS( CImpExpProgDlg )
    DECLARE_EVENT_TABLE()

    cdp_t m_cdp;
    int counter;
    void Append(const char *Value);
    void OnSize(wxSizeEvent &event);

public:
    /// Constructors
    CImpExpProgDlg( );
    CImpExpProgDlg(cdp_t cdp, wxWindow* parent, wxString captionIn, bool dupldbIn);
    //CImpExpProgDlg( wxWindow* parent, wxWindowID id = -1, const wxString& caption = _("Dialog"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU );

    /// Creation
    bool Create( wxWindow* parent, wxWindowID id = -1, const wxString& caption = _("Dialog"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU );

    /// Creates the controls and sizers
    void CreateControls();

////@begin CImpExpProgDlg event handler declarations

////@end CImpExpProgDlg event handler declarations

////@begin CImpExpProgDlg member function declarations

    bool GetExport() const { return m_Export ; }
    void SetExport(bool value) { m_Export = value ; }


    /// Retrieves bitmap resources
    wxBitmap GetBitmapResource( const wxString& name );

    /// Retrieves icon resources
    wxIcon GetIconResource( const wxString& name );
////@end CImpExpProgDlg member function declarations

    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin CImpExpProgDlg member variables
    wxTextCtrl* CategED;
    wxTextCtrl* NameED;
    wxListCtrl* List;
    wxGauge* ProgBar;
    wxButton* OKBut;
    bool m_Export;
////@end CImpExpProgDlg member variables
    bool dupldb;
    wxString caption;
    static void WINAPI Progress(int Categ, const void *Value, void *Param);
    void Progress(int Categ, const char *Value);
};

#endif
    // _IMPEXPPROG_H_
