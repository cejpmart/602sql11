/////////////////////////////////////////////////////////////////////////////
// Name:        NewFtxIdCol.h
// Purpose:     
// Author:      
// Modified by: 
// Created:     02/15/05 11:50:05
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifndef _NEWFTXIDCOL_H_
#define _NEWFTXIDCOL_H_

#if defined(__GNUG__) && !defined(__APPLE__)
//#pragma interface "NewFtxIdCol.cpp"
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
class wxOwnerDrawnComboBox;
////@end forward declarations

/*!
 * Control identifiers
 */

////@begin control identifiers
#define ID_DIALOG 10000
#define SYMBOL_NEWFTXIDCOL_STYLE wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU|wxCLOSE_BOX
#define SYMBOL_NEWFTXIDCOL_TITLE _("Adding ID Column")
#define SYMBOL_NEWFTXIDCOL_IDNAME ID_DIALOG
#define SYMBOL_NEWFTXIDCOL_SIZE wxSize(400, 300)
#define SYMBOL_NEWFTXIDCOL_POSITION wxDefaultPosition
#define CD_FTXCOL_COMBO 10001
////@end control identifiers

/*!
 * Compatibility
 */

#ifndef wxCLOSE_BOX
#define wxCLOSE_BOX 0x1000
#endif

/*!
 * NewFtxIdCol class declaration
 */

class NewFtxIdCol: public wxDialog
{    
    DECLARE_CLASS( NewFtxIdCol )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    NewFtxIdCol(cdp_t cdpIn, ttablenum tbnumIn, const char * ftx_nameIn);
    ~NewFtxIdCol(void);

    /// Creation
    bool Create( wxWindow* parent, wxWindowID id = SYMBOL_NEWFTXIDCOL_IDNAME, const wxString& caption = SYMBOL_NEWFTXIDCOL_TITLE, const wxPoint& pos = SYMBOL_NEWFTXIDCOL_POSITION, const wxSize& size = SYMBOL_NEWFTXIDCOL_SIZE, long style = SYMBOL_NEWFTXIDCOL_STYLE );

    /// Creates the controls and sizers
    void CreateControls();

////@begin NewFtxIdCol event handler declarations

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_OK
    void OnOkClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CANCEL
    void OnCancelClick( wxCommandEvent& event );

////@end NewFtxIdCol event handler declarations

////@begin NewFtxIdCol member function declarations

    /// Retrieves bitmap resources
    wxBitmap GetBitmapResource( const wxString& name );

    /// Retrieves icon resources
    wxIcon GetIconResource( const wxString& name );
////@end NewFtxIdCol member function declarations

    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin NewFtxIdCol member variables
    wxOwnerDrawnComboBox* mCombo;
    wxButton* mOK;
////@end NewFtxIdCol member variables
    cdp_t cdp;
    ttablenum tbnum;
    tobjname ftx_name;
    table_all * ta;
    wxString newcolname;
};

#endif
    // _NEWFTXIDCOL_H_
