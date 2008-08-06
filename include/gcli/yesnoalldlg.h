/////////////////////////////////////////////////////////////////////////////
// Name:        yesnoalldlg.h
// Purpose:     
// Author:      
// Modified by: 
// Created:     03/09/04 13:20:33
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifndef _YESNOALLDLG_H_
#define _YESNOALLDLG_H_

#ifdef __GNUG__
//#pragma interface "yesnoalldlg.cpp"
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
////@end control identifiers

/*!
 * Compatibility
 */

#ifndef wxCLOSE_BOX
#define wxCLOSE_BOX 0x1000
#endif

/*!
 * CYesNoAllDlg class declaration
 */

class CYesNoAllDlg: public wxDialog
{    
    DECLARE_CLASS( CYesNoAllDlg )
    DECLARE_EVENT_TABLE()
    wxString m_Text;
    wxString m_AllBut;

public:
    /// Constructors
    CYesNoAllDlg( );
    CYesNoAllDlg(wxWindow* parent, wxString Text, wxString AllBut);

    CYesNoAllDlg( wxWindow* parent, wxWindowID id = -1, const wxString& caption = _("Warning"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU );

    /// Creation
    bool Create( wxWindow* parent, wxWindowID id = -1, const wxString& caption = _("Warning"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU );

    /// Creates the controls and sizers
    void CreateControls();

////@begin CYesNoAllDlg event handler declarations

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_YES
    void OnClick( wxCommandEvent& event );

////@end CYesNoAllDlg event handler declarations

////@begin CYesNoAllDlg member function declarations

////@end CYesNoAllDlg member function declarations

    void OnClose(wxCloseEvent & event);
    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin CYesNoAllDlg member variables
    wxStaticText* m_Label;
////@end CYesNoAllDlg member variables
};

#endif
    // _YESNOALLDLG_H_
