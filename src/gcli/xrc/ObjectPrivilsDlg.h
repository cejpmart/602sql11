/////////////////////////////////////////////////////////////////////////////
// Name:        ObjectPrivilsDlg.h
// Purpose:     
// Author:      
// Modified by: 
// Created:     04/27/04 07:42:36
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifndef _OBJECTPRIVILSDLG_H_
#define _OBJECTPRIVILSDLG_H_

#if defined(__GNUG__) && !defined(__APPLE__)
//#pragma interface "ObjectPrivilsDlg.cpp"
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
class wxOwnerDrawnComboBox;
class wxBoxSizer;
class wxListCtrl;
////@end forward declarations

/*!
 * Control identifiers
 */

////@begin control identifiers
#define ID_DIALOG 10000
#define SYMBOL_OBJECTPRIVILSDLG_STYLE wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU|wxCLOSE_BOX
#define SYMBOL_OBJECTPRIVILSDLG_TITLE _("Object Privileges")
#define SYMBOL_OBJECTPRIVILSDLG_IDNAME ID_DIALOG
#define SYMBOL_OBJECTPRIVILSDLG_SIZE wxSize(400, 300)
#define SYMBOL_OBJECTPRIVILSDLG_POSITION wxDefaultPosition
#define CD_PRIV_USER 10001
#define CD_PRIV_GROUP 10002
#define CD_PRIV_ROLE 10003
#define CD_PRIV_SUBJECT 10004
#define CD_PRIV_MEMB1 10005
#define CD_PRIV_MEMB2 10006
#define CD_PRIV_LIST 10007
#define CD_PRIV_GRANT 10008
#define CD_PRIV_REVOKE 10009
#define CD_PRIV_GRANT_REC 10010
#define CD_PRIV_REVOKE_REC 10011
#define CD_PRIV_SEL_READ 10012
#define CD_PRIV_SEL_WRITE 10013
////@end control identifiers

/*!
 * Compatibility
 */

#ifndef wxCLOSE_BOX
#define wxCLOSE_BOX 0x1000
#endif

/*!
 * ObjectPrivilsDlg class declaration
 */

class ObjectPrivilsDlg: public wxDialog
{    
    DECLARE_CLASS( ObjectPrivilsDlg )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    ObjectPrivilsDlg(t_privil_info & piIn);

    /// Creation
    bool Create( wxWindow* parent, wxWindowID id = SYMBOL_OBJECTPRIVILSDLG_IDNAME, const wxString& caption = SYMBOL_OBJECTPRIVILSDLG_TITLE, const wxPoint& pos = SYMBOL_OBJECTPRIVILSDLG_POSITION, const wxSize& size = SYMBOL_OBJECTPRIVILSDLG_SIZE, long style = SYMBOL_OBJECTPRIVILSDLG_STYLE );

    /// Creates the controls and sizers
    void CreateControls();

////@begin ObjectPrivilsDlg event handler declarations

    /// wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for CD_PRIV_USER
    void OnCdPrivUserSelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for CD_PRIV_GROUP
    void OnCdPrivGroupSelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for CD_PRIV_ROLE
    void OnCdPrivRoleSelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_COMBOBOX_SELECTED event handler for CD_PRIV_SUBJECT
    void OnCdPrivSubjectSelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_PRIV_MEMB1
    void OnCdPrivMemb1Click( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_PRIV_MEMB2
    void OnCdPrivMemb2Click( wxCommandEvent& event );

    /// wxEVT_COMMAND_LIST_ITEM_SELECTED event handler for CD_PRIV_LIST
    void OnCdPrivListSelected( wxListEvent& event );

    /// wxEVT_COMMAND_LIST_ITEM_DESELECTED event handler for CD_PRIV_LIST
    void OnCdPrivListDeselected( wxListEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_PRIV_GRANT
    void OnCdPrivGrantClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_PRIV_REVOKE
    void OnCdPrivRevokeClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_PRIV_GRANT_REC
    void OnCdPrivGrantRecClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_PRIV_REVOKE_REC
    void OnCdPrivRevokeRecClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_PRIV_SEL_READ
    void OnCdPrivSelReadClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_PRIV_SEL_WRITE
    void OnCdPrivSelWriteClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CANCEL
    void OnCancelClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_HELP
    void OnHelpClick( wxCommandEvent& event );

////@end ObjectPrivilsDlg event handler declarations

////@begin ObjectPrivilsDlg member function declarations

    /// Retrieves bitmap resources
    wxBitmap GetBitmapResource( const wxString& name );

    /// Retrieves icon resources
    wxIcon GetIconResource( const wxString& name );
////@end ObjectPrivilsDlg member function declarations

    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin ObjectPrivilsDlg member variables
    wxRadioButton* mUser;
    wxRadioButton* mGroup;
    wxRadioButton* mRole;
    wxOwnerDrawnComboBox* mSubject;
    wxButton* mMemb1;
    wxButton* mMemb2;
    wxBoxSizer* mSizerContainingList;
    wxListCtrl* mList;
    wxButton* mGrant;
    wxButton* mRevoke;
    wxButton* mGrantRec;
    wxButton* mRevokeRec;
    wxButton* mSelRead;
    wxButton* mSelWrite;
////@end ObjectPrivilsDlg member variables
    t_privil_info & pi;
};

#endif
    // _OBJECTPRIVILSDLG_H_
