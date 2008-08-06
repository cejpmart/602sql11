/////////////////////////////////////////////////////////////////////////////
// Name:        expschemadlg.h
// Purpose:     
// Author:      
// Modified by: 
// Created:     04/29/04 15:55:18
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifndef _EXPSCHEMADLG_H_
#define _EXPSCHEMADLG_H_

#ifdef __GNUG__
//#pragma interface "expschemadlg.cpp"
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
#define SYMBOL_CEXPSCHEMADLG_STYLE wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU|wxCLOSE_BOX
#define SYMBOL_CEXPSCHEMADLG_TITLE _("Schema export")
#define SYMBOL_CEXPSCHEMADLG_IDNAME ID_DIALOG
#define SYMBOL_CEXPSCHEMADLG_SIZE wxSize(400, 300)
#define SYMBOL_CEXPSCHEMADLG_POSITION wxDefaultPosition
#define ID_COMPLET 10001
#define ID_CHANGEDSINCE 10002
#define ID_CHANGEDSINCEED 10003
#define ID_BITMAPBUTTON 10013
#define ID_CHECKBOX 10004
#define ID_CHECKBOX1 10005
#define ID_CHECKBOX2 10006
#define ID_CHECKBOX3 10007
#define ID_FOLDERED 10016
#define ID_SELECT 10015
////@end control identifiers

/*!
 * Compatibility
 */

#ifndef wxCLOSE_BOX
#define wxCLOSE_BOX 0x1000
#endif

/*!
 * CExpSchemaDlg class declaration
 */

class CExpSchemaDlg: public wxDialog
{    
    DECLARE_CLASS( CExpSchemaDlg )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    CExpSchemaDlg( );
    CExpSchemaDlg(wxString Schema, wxString ApplName, bool Locked);
    CExpSchemaDlg( wxWindow* parent, wxWindowID id = -1, const wxString& caption = SYMBOL_CEXPSCHEMADLG_TITLE, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU|wxCLOSE_BOX );

    /// Creation
    bool Create( wxWindow* parent, wxWindowID id = -1, const wxString& caption = SYMBOL_CEXPSCHEMADLG_TITLE, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU|wxCLOSE_BOX );

    /// Creates the controls and sizers
    void CreateControls();

////@begin CExpSchemaDlg event handler declarations

    /// wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_COMPLET
    void OnRadioSelected( wxCommandEvent& event );
    void OnFolderChanged( wxCommandEvent& event );
    void OnSelectFolder( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BITMAPBUTTON
    void OnDateTimeClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_OK
    void OnOkClick( wxCommandEvent& event );
    void OnHelp( wxCommandEvent& event );

////@end CExpSchemaDlg event handler declarations

////@begin CExpSchemaDlg member function declarations

    bool GetData() const { return m_Data ; }
    void SetData(bool value) { m_Data = value ; }

    bool GetRolePrivils() const { return m_RolePrivils ; }
    void SetRolePrivils(bool value) { m_RolePrivils = value ; }

    bool GetUserPrivils() const { return m_UserPrivils ; }
    void SetUserPrivils(bool value) { m_UserPrivils = value ; }

    bool GetEncrypt() const { return m_Encrypt ; }
    void SetEncrypt(bool value) { m_Encrypt = value ; }

    long GetChangedSince() const { return m_ChangedSince ; }
    void SetChangedSince(long value) { m_ChangedSince = value ; }
    wxString GetPath();


    /// Retrieves bitmap resources
    wxBitmap GetBitmapResource( const wxString& name );

    /// Retrieves icon resources
    wxIcon GetIconResource( const wxString& name );
////@end CExpSchemaDlg member function declarations

    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin CExpSchemaDlg member variables
    wxRadioButton* CompletRB;
    wxRadioButton* ChangedSinceRB;
    wxTextCtrl* ChangedSinceED;
    wxBitmapButton* ChangedSinceBut;
    wxCheckBox* DataCHB;
    wxCheckBox* RolePrivilsCHB;
    wxCheckBox* UserPrivilsCHB;
    wxCheckBox* EncryptCHB;
    wxTextCtrl* FolderED;
    wxButton*   OkBut;  
    bool m_Data;
    bool m_RolePrivils;
    bool m_UserPrivils;
    bool m_Encrypt;
    long m_ChangedSince;
    wxString m_ApplName;
    wxString m_Schema;
////@end CExpSchemaDlg member variables
};

#endif
    // _EXPSCHEMADLG_H_
