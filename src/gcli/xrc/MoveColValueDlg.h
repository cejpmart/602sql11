/////////////////////////////////////////////////////////////////////////////
// Name:        MoveColValueDlg.h
// Purpose:     
// Author:      
// Modified by: 
// Created:     02/03/04 10:30:07
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifndef _MOVECOLVALUEDLG_H_
#define _MOVECOLVALUEDLG_H_

#ifdef __GNUG__
//#pragma interface "MoveColValueDlg.cpp"
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
#define SYMBOL_MOVECOLVALUEDLG_STYLE wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU|wxCLOSE_BOX
#define SYMBOL_MOVECOLVALUEDLG_TITLE _T("")
#define SYMBOL_MOVECOLVALUEDLG_IDNAME ID_DIALOG
#define SYMBOL_MOVECOLVALUEDLG_SIZE wxSize(400, 300)
#define SYMBOL_MOVECOLVALUEDLG_POSITION wxDefaultPosition
#define CD_TBUP_CAPT 10004
#define CD_TBUP_RADIO 10008
#define CD_TBUP_VAL 10003
#define CD_TBUP_LIST_CAPT 10006
#define CD_TBUP_LIST 10005
////@end control identifiers

/*!
 * Compatibility
 */

#ifndef wxCLOSE_BOX
#define wxCLOSE_BOX 0x1000
#endif

/*!
 * MoveColValueDlg class declaration
 */

class MoveColValueDlg: public wxScrolledWindow
{    
    DECLARE_CLASS( MoveColValueDlg )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    MoveColValueDlg( );
    MoveColValueDlg( wxWindow* parent, wxWindowID id = -1, const wxString& caption = SYMBOL_MOVECOLVALUEDLG_TITLE, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU );

    /// Creation
    bool Create( wxWindow* parent, wxWindowID id = -1, const wxString& caption = SYMBOL_MOVECOLVALUEDLG_TITLE, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU );

    /// Creates the controls and sizers
    void CreateControls();

////@begin MoveColValueDlg event handler declarations

    /// wxEVT_COMMAND_RADIOBOX_SELECTED event handler for CD_TBUP_RADIO
    void OnCdTbupRadioSelected( wxCommandEvent& event );

////@end MoveColValueDlg event handler declarations

////@begin MoveColValueDlg member function declarations

////@end MoveColValueDlg member function declarations

    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin MoveColValueDlg member variables
    wxStaticText* mCapt;
    wxRadioBox* mRadio;
    wxStaticText* mValCapt;
    wxTextCtrl* mVal;
    wxStaticText* mListCapt;
    wxListBox* mList;
////@end MoveColValueDlg member variables
};

#endif
    // _MOVECOLVALUEDLG_H_
