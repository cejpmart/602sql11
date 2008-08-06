/////////////////////////////////////////////////////////////////////////////
// Name:        impobjconfl.h
// Purpose:     
// Author:      
// Modified by: 
// Created:     03/04/04 15:15:36
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifndef _IMPOBJCONFL_H_
#define _IMPOBJCONFL_H_

#ifdef __GNUG__
//#pragma interface "impobjconfl.cpp"
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
#define SYMBOL_CIMPOBJCONFLDLG_STYLE wxCAPTION|wxCLOSE_BOX
#define SYMBOL_CIMPOBJCONFLDLG_TITLE _("Import conflict")
#define SYMBOL_CIMPOBJCONFLDLG_IDNAME ID_DIALOG
#define SYMBOL_CIMPOBJCONFLDLG_SIZE wxSize(400, 300)
#define SYMBOL_CIMPOBJCONFLDLG_POSITION wxDefaultPosition
#define ID_RADIOBOX 10005
#define ID_TEXTCTRL 10002
#define ID_Close 10001
////@end control identifiers

/*!
 * Compatibility
 */

#ifndef wxCLOSE_BOX
#define wxCLOSE_BOX 0x1000
#endif

/*!
 * CImpObjConflDlg class declaration
 */

class CImpObjConflDlg: public wxDialog
{    
    DECLARE_CLASS( CImpObjConflDlg )
    DECLARE_EVENT_TABLE()
    cdp_t    m_cdp;
    wxString m_Prompt;
    char    *m_ObjName;
    tcateg   m_Categ;
    bool     m_Single;
    bool     m_Application;
    int      m_Value;

    wxRadioBox* CreateRadioBox(wxWindow* Parent);
    int GetSelValue();

public:
    /// Constructors
    CImpObjConflDlg( );
    CImpObjConflDlg(cdp_t cdp, const wxString &Prompt, char *ObjName, tcateg Categ, bool Single, bool Application = false);
    CImpObjConflDlg( wxWindow* parent, wxWindowID id = -1, const wxString& caption = _("Import conflict"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU );

    /// Creation
    bool Create( wxWindow* parent, wxWindowID id = -1, const wxString& caption = _("Import conflict"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU );

    /// Creates the controls and sizers
    void CreateControls();

////@begin CImpObjConflDlg event handler declarations

    /// wxEVT_COMMAND_RADIOBOX_SELECTED event handler for ID_RADIOBOX
    void OnRadioboxSelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_Close
    void OnCloseClick( wxCommandEvent& event );

////@end CImpObjConflDlg event handler declarations

////@begin CImpObjConflDlg member function declarations

    /// Retrieves bitmap resources
    wxBitmap GetBitmapResource( const wxString& name );

    /// Retrieves icon resources
    wxIcon GetIconResource( const wxString& name );
////@end CImpObjConflDlg member function declarations

    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin CImpObjConflDlg member variables
    wxStaticText* m_Text;
    wxRadioBox* m_RadioBox;
    wxStaticText* m_Label;
    wxTextCtrl* m_Edit;
////@end CImpObjConflDlg member variables

    void OnClose(wxCloseEvent & event);
};

#endif
    // _IMPOBJCONFL_H_
