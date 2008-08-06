/////////////////////////////////////////////////////////////////////////////
// Name:        NewFtx.h
// Purpose:     
// Author:      
// Modified by: 
// Created:     02/07/05 16:22:02
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifndef _NEWFTX_H_
#define _NEWFTX_H_

#if defined(__GNUG__) && !defined(__APPLE__)
//#pragma interface "NewFtx.cpp"
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
#define SYMBOL_NEWFTX_STYLE wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU|wxCLOSE_BOX
#define SYMBOL_NEWFTX_TITLE _("New Fulltext System")
#define SYMBOL_NEWFTX_IDNAME ID_DIALOG
#define SYMBOL_NEWFTX_SIZE wxSize(400, 300)
#define SYMBOL_NEWFTX_POSITION wxDefaultPosition
#define CD_FTD_NAME 10001
#define CD_FTD_LANG 10002
#define CD_FTD_BASIC 10003
#define CD_FTD_SUBSTRUCT 10046
#define CD_FTD_EXTERN 10076
#define CD_FTD_BIGINT_ID 10153
////@end control identifiers

/*!
 * Compatibility
 */

#ifndef wxCLOSE_BOX
#define wxCLOSE_BOX 0x1000
#endif

/*!
 * NewFtx class declaration
 */

class NewFtx: public wxDialog
{    
    DECLARE_CLASS( NewFtx )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    NewFtx(cdp_t cdpIn, t_fulltext * ftdefIn);

    /// Creation
    bool Create( wxWindow* parent, wxWindowID id = SYMBOL_NEWFTX_IDNAME, const wxString& caption = SYMBOL_NEWFTX_TITLE, const wxPoint& pos = SYMBOL_NEWFTX_POSITION, const wxSize& size = SYMBOL_NEWFTX_SIZE, long style = SYMBOL_NEWFTX_STYLE );

    /// Creates the controls and sizers
    void CreateControls();

////@begin NewFtx event handler declarations

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_OK
    void OnOkClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CANCEL
    void OnCancelClick( wxCommandEvent& event );

////@end NewFtx event handler declarations

////@begin NewFtx member function declarations

    /// Retrieves bitmap resources
    wxBitmap GetBitmapResource( const wxString& name );

    /// Retrieves icon resources
    wxIcon GetIconResource( const wxString& name );
////@end NewFtx member function declarations

    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin NewFtx member variables
    wxTextCtrl* mName;
    wxOwnerDrawnComboBox* mLanguage;
    wxCheckBox* mBasic;
    wxCheckBox* mSubstruct;
    wxCheckBox* mExternal;
    wxCheckBox* mBigIntId;
////@end NewFtx member variables
    cdp_t cdp;
    t_fulltext * ftdef;
};

#endif
    // _NEWFTX_H_
