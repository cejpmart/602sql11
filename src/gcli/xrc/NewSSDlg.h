/////////////////////////////////////////////////////////////////////////////
// Name:        NewSSDlg.h
// Purpose:     
// Author:      
// Modified by: 
// Created:     07/24/06 15:23:08
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifndef _NEWSSDLG_H_
#define _NEWSSDLG_H_

#if defined(__GNUG__) && !defined(__APPLE__)
//#pragma interface "NewSSDlg.cpp"
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
#define ID_MYDIALOG7 10115
#define SYMBOL_NEWSSDLG_STYLE wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU|wxCLOSE_BOX
#define SYMBOL_NEWSSDLG_TITLE _("Creating a New Style Sheet")
#define SYMBOL_NEWSSDLG_IDNAME ID_MYDIALOG7
#define SYMBOL_NEWSSDLG_SIZE wxSize(400, 300)
#define SYMBOL_NEWSSDLG_POSITION wxDefaultPosition
#define CD_NEWSS_TYPE 10116
#define CD_NEWSS_NAME 10122
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
 * NewSSDlg class declaration
 */

class NewSSDlg: public wxDialog
{    
    DECLARE_CLASS( NewSSDlg )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    NewSSDlg(cdp_t cdpIn);

    /// Creation
    bool Create( wxWindow* parent, wxWindowID id = SYMBOL_NEWSSDLG_IDNAME, const wxString& caption = SYMBOL_NEWSSDLG_TITLE, const wxPoint& pos = SYMBOL_NEWSSDLG_POSITION, const wxSize& size = SYMBOL_NEWSSDLG_SIZE, long style = SYMBOL_NEWSSDLG_STYLE );

    /// Creates the controls and sizers
    void CreateControls();

////@begin NewSSDlg event handler declarations

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_OK
    void OnOkClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CANCEL
    void OnCancelClick( wxCommandEvent& event );

////@end NewSSDlg event handler declarations

////@begin NewSSDlg member function declarations

    /// Retrieves bitmap resources
    wxBitmap GetBitmapResource( const wxString& name );

    /// Retrieves icon resources
    wxIcon GetIconResource( const wxString& name );
////@end NewSSDlg member function declarations

    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin NewSSDlg member variables
    wxRadioBox* mType;
    wxTextCtrl* mName;
////@end NewSSDlg member variables
    cdp_t cdp;
    wxString name;  // the entered trimmed verified name
};

#endif
    // _NEWSSDLG_H_
