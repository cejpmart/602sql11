/////////////////////////////////////////////////////////////////////////////
// Name:        TransportDataDlg.h
// Purpose:     
// Author:      
// Modified by: 
// Created:     04/06/04 16:12:15
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifndef _TRANSPORTDATADLG_H_
#define _TRANSPORTDATADLG_H_

#ifdef __GNUG__
//#pragma interface "TransportDataDlg.cpp"
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
#define SYMBOL_TRANSPORTDATADLG_STYLE wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU|wxCLOSE_BOX
#define SYMBOL_TRANSPORTDATADLG_TITLE _("Transport data")
#define SYMBOL_TRANSPORTDATADLG_IDNAME ID_DIALOG
#define SYMBOL_TRANSPORTDATADLG_SIZE wxSize(400, 300)
#define SYMBOL_TRANSPORTDATADLG_POSITION wxDefaultPosition
#define CD_TR_SOURCE_NAME 10004
#define CD_TR_SOURCE_NAME_SEL 10005
#define CD_TR_DEST_NAME 10015
#define CD_TR_DEST_NAME_SEL 10016
////@end control identifiers

/*!
 * Compatibility
 */

#ifndef wxCLOSE_BOX
#define wxCLOSE_BOX 0x1000
#endif

/*!
 * TransportDataDlg class declaration
 */

class TransportDataDlg: public wxDialog
{    
    DECLARE_CLASS( TransportDataDlg )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    TransportDataDlg(t_ie_run * dsgnIn);

    /// Creation
    bool Create( wxWindow* parent, wxWindowID id = -1, const wxString& caption = _("Transport data"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU|wxCLOSE_BOX );

    /// Creates the controls and sizers
    void CreateControls();

////@begin TransportDataDlg event handler declarations

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_TR_SOURCE_NAME_SEL
    void OnCdTrSourceNameSelClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_TR_DEST_NAME_SEL
    void OnCdTrDestNameSelClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_OK
    void OnOkClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CANCEL
    void OnCancelClick( wxCommandEvent& event );

////@end TransportDataDlg event handler declarations

////@begin TransportDataDlg member function declarations

    /// Retrieves bitmap resources
    wxBitmap GetBitmapResource( const wxString& name );

    /// Retrieves icon resources
    wxIcon GetIconResource( const wxString& name );
////@end TransportDataDlg member function declarations

    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin TransportDataDlg member variables
    wxOwnerDrawnComboBox* mSourceName;
    wxButton* mSourceNameSel;
    wxOwnerDrawnComboBox* mDestName;
    wxButton* mDestNameSel;
////@end TransportDataDlg member variables
    t_ie_run * dsgn;
};

#endif
    // _TRANSPORTDATADLG_H_
