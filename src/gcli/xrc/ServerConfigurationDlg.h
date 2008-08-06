/////////////////////////////////////////////////////////////////////////////
// Name:        ServerConfigurationDlg.h
// Purpose:     
// Author:      
// Modified by: 
// Created:     05/02/04 08:18:59
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifndef _SERVERCONFIGURATIONDLG_H_
#define _SERVERCONFIGURATIONDLG_H_

#if defined(__GNUG__) && !defined(__APPLE__)
//#pragma interface "ServerConfigurationDlg.cpp"
#endif

/*!
 * Includes
 */

////@begin includes
#include "wx/notebook.h"
#include "wx/grid.h"
////@end includes

/*!
 * Forward declarations
 */

////@begin forward declarations
class wxNotebook;
class wxGrid;
class wxBoxSizer;
////@end forward declarations

/*!
 * Control identifiers
 */

////@begin control identifiers
#define ID_DIALOG 10000
#define SYMBOL_SERVERCONFIGURATIONDLG_STYLE wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU|wxCLOSE_BOX
#define SYMBOL_SERVERCONFIGURATIONDLG_TITLE _("Runtime Parameters of the SQL Server")
#define SYMBOL_SERVERCONFIGURATIONDLG_IDNAME ID_DIALOG
#define SYMBOL_SERVERCONFIGURATIONDLG_SIZE wxDefaultSize
#define SYMBOL_SERVERCONFIGURATIONDLG_POSITION wxDefaultPosition
#define ID_NOTEBOOK 10001
#define CD_CONFTAB_OPER 10004
#define CD_OPER_GRID 10006
#define CD_CONFTAB_SECUR 10002
#define CD_SECUR_GRID 10003
#define CD_CONFTAB_COMPAT 10005
#define CD_COMPAT_GRID 10007
#define CD_CONFTAB_DIRS 10008
#define CD_DIRS_LIST 10009
#define CD_DIRS_EDIT 10016
#define CD_DIRS_SEL 10017
#define CD_DIRS_NEW 10010
#define CD_DIR_MODIF 10011
#define CD_DIRS_REMOVE 10012
#define CD_CONFTAB_IP 10013
#define CD_IP_ENA 10014
#define CD_IP_DIS 10015
#define CD_CONFTAB_IP1 10018
#define CD_CONFTAB_IP2 10021
////@end control identifiers

/*!
 * Compatibility
 */

#ifndef wxCLOSE_BOX
#define wxCLOSE_BOX 0x1000
#endif

/*!
 * ServerConfigurationDlg class declaration
 */

class ServerConfigurationDlg: public wxDialog
{    
    DECLARE_CLASS( ServerConfigurationDlg )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    ServerConfigurationDlg(cdp_t cdpIn, PropertyGrid * pg1In, PropertyGrid * pg2In, PropertyGrid * pg3In);
    /// Creation
    bool Create( wxWindow* parent, wxWindowID id = SYMBOL_SERVERCONFIGURATIONDLG_IDNAME, const wxString& caption = SYMBOL_SERVERCONFIGURATIONDLG_TITLE, const wxPoint& pos = SYMBOL_SERVERCONFIGURATIONDLG_POSITION, const wxSize& size = SYMBOL_SERVERCONFIGURATIONDLG_SIZE, long style = SYMBOL_SERVERCONFIGURATIONDLG_STYLE );
    /// Creates the controls and sizers
    void CreateControls();

////@begin ServerConfigurationDlg event handler declarations

    /// wxEVT_GRID_CELL_CHANGE event handler for CD_OPER_GRID
    void OnCdOperGridCellChange( wxGridEvent& event );

    /// wxEVT_GRID_CELL_CHANGE event handler for CD_SECUR_GRID
    void OnCdSecurGridCellChange( wxGridEvent& event );

    /// wxEVT_GRID_CELL_CHANGE event handler for CD_COMPAT_GRID
    void OnCdCompatGridCellChange( wxGridEvent& event );

    /// wxEVT_COMMAND_LISTBOX_SELECTED event handler for CD_DIRS_LIST
    void OnCdDirsListSelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_LISTBOX_DOUBLECLICKED event handler for CD_DIRS_LIST
    void OnCdDirsListDoubleClicked( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_UPDATED event handler for CD_DIRS_EDIT
    void OnCdDirsEditUpdated( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_DIRS_SEL
    void OnCdDirsSelClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_DIRS_NEW
    void OnCdDirsNewClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_DIR_MODIF
    void OnCdDirModifClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_DIRS_REMOVE
    void OnCdDirsRemoveClick( wxCommandEvent& event );

    /// wxEVT_GRID_CELL_CHANGE event handler for CD_IP_ENA
    void OnCdIpEnaCellChange( wxGridEvent& event );

    /// wxEVT_GRID_CELL_CHANGE event handler for CD_IP_DIS
    void OnCdIpDisCellChange( wxGridEvent& event );

    /// wxEVT_GRID_CELL_CHANGE event handler for CD_IP_ENA
    void OnCellChange( wxGridEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_OK
    void OnOkClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CANCEL
    void OnCancelClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_HELP
    void OnHelpClick( wxCommandEvent& event );

////@end ServerConfigurationDlg event handler declarations

////@begin ServerConfigurationDlg member function declarations

    /// Retrieves bitmap resources
    wxBitmap GetBitmapResource( const wxString& name );

    /// Retrieves icon resources
    wxIcon GetIconResource( const wxString& name );
////@end ServerConfigurationDlg member function declarations

    /// Should we show tooltips?
    static bool ShowToolTips();
    void save_IP(wxGrid * mIPEna, wxGrid * mIPDis, const char * enabled_addr, const char * enabled_mask, const char * disabled_addr, const char * disabled_mask);

////@begin ServerConfigurationDlg member variables
    wxNotebook* mNotebook;
    wxGrid* mOperGrid;
    wxPanel* mConftabSecur;
    wxGrid* mSecurGrid;
    wxGrid* mCompatGrid;
    wxListBox* mDirsList;
    wxTextCtrl* mEdit;
    wxButton* mSel;
    wxButton* mNewDir;
    wxButton* mModifDir;
    wxButton* mRemDir;
    wxBoxSizer* mSizerContainingGrids;
    wxGrid* mIPEna;
    wxGrid* mIPDis;
    wxBoxSizer* mSizerContainingGrids1;
    wxGrid* mIP1Ena;
    wxGrid* mIP1Dis;
    wxBoxSizer* mSizerContainingGrids2;
    wxGrid* mIP2Ena;
    wxGrid* mIP2Dis;
////@end ServerConfigurationDlg member variables
  cdp_t cdp;
  PropertyGrid * pg1, * pg2, * pg3;
  bool conf_adm, dirs_changed, ip_changed;
};

#endif
    // _SERVERCONFIGURATIONDLG_H_
