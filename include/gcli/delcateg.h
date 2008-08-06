/////////////////////////////////////////////////////////////////////////////
// Name:        delcateg.h
// Purpose:     
// Author:      
// Modified by: 
// Created:     04/14/04 14:32:32
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifndef _DELCATEG_H_
#define _DELCATEG_H_

#ifdef __GNUG__
//#pragma interface "delcateg.cpp"
#endif

/*!
 * Includes
 */

////@begin includes
#include "objlist9.h"
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
#define SYMBOL_CDELCATEG_STYLE wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU|wxCLOSE_BOX
#define SYMBOL_CDELCATEG_TITLE _("Warning")
#define SYMBOL_CDELCATEG_IDNAME ID_DIALOG
#define SYMBOL_CDELCATEG_SIZE wxSize(400, 300)
#define SYMBOL_CDELCATEG_POSITION wxDefaultPosition
#define ID_RADIO 10001
////@end control identifiers

/*!
 * Compatibility
 */

#ifndef wxCLOSE_BOX
#define wxCLOSE_BOX 0x1000
#endif

/*!
 * CDelCateg class declaration
 */

class CDelCateg: public wxDialog
{    
    DECLARE_CLASS( CDelCateg )
    DECLARE_EVENT_TABLE()

    wxString m_CategObjs;
    wxString m_WholeFolder;

public:
    /// Constructors
    CDelCateg( );
    CDelCateg(cdp_t cdp, CCateg Categ, const wxChar *Folder);
    //CDelCateg( wxWindow* parent, wxWindowID id = -1, const wxString& caption = _("Warning"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU );

    /// Creation
    bool Create( wxWindow* parent, wxWindowID id = -1, const wxString& caption = _("Warning"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU );

    /// Creates the controls and sizers
    void CreateControls();

////@begin CDelCateg event handler declarations

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_OK
    void OnOKClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CANCEL
    void OnCancelClick( wxCommandEvent& event );

////@end CDelCateg event handler declarations

////@begin CDelCateg member function declarations


////@end CDelCateg member function declarations

    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin CDelCateg member variables
    wxRadioBox* RadioBox;
////@end CDelCateg member variables
};

#endif
    // _DELCATEG_H_
