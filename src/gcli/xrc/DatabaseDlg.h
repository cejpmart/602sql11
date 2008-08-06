/////////////////////////////////////////////////////////////////////////////
// Name:        DatabaseDlg.h
// Purpose:     
// Author:      
// Modified by: 
// Created:     04/13/04 11:39:21
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifndef _DATABASEDLG_H_
#define _DATABASEDLG_H_

#ifdef __GNUG__
//#pragma interface "DatabaseDlg.cpp"
#endif

/*!
 * Includes
 */

////@begin includes
#include "wx/notebook.h"
////@end includes

/*!
 * Forward declarations
 */

////@begin forward declarations
class wxOwnerDrawnComboBox;
class wxBoxSizer;
////@end forward declarations

/*!
 * Control identifiers
 */

////@begin control identifiers
#define ID_DIALOG 10000
#define SYMBOL_DATABASEDLG_STYLE wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU|wxCLOSE_BOX
#define SYMBOL_DATABASEDLG_TITLE _("Database Management")
#define SYMBOL_DATABASEDLG_IDNAME ID_DIALOG
#define SYMBOL_DATABASEDLG_SIZE wxDefaultSize
#define SYMBOL_DATABASEDLG_POSITION wxDefaultPosition
#define ID_NOTEBOOK 10001
#define ID_PANEL 10002
#define CD_FD_PART1 10004
#define CD_FD_LIM1 10006
#define CD_FD_PART2 10007
#define CD_FD_SEL2 10008
#define CD_FD_LIM2 10009
#define CD_FD_PART3 10010
#define CD_FD_SEL3 10011
#define CD_FD_LIM3 10012
#define CD_FD_PART4 10013
#define CD_FD_SEL4 10014
#define CD_FD_TRANS 10005
#define CD_FD_SELTRANS 10016
#define CD_FD_LOG 10017
#define CD_FD_SELLOG 10018
#define CD_FD_SERVER_LANG 10066
#define ID_PANEL1 10003
#define CD_RECORVERY_DIR 10020
#define CD_RECORVERY_DIRSEL 10021
#define CD_RECORVERY_LIST 10022
#define CD_RECORVERY_START 10023
#define CD_PATCH_START 10019
#define CD_ERASE_START 10015
#define CD_ED_APPLS 10065
#define CD_ED_BACK_FOLDER 10069
#define CD_ED_BUTBACKFOLDER 10070
#define CD_ED_DUPLFIL 10025
#define ID_PANEL3 10030
#define CD_SERVICE_OLD_NAME 10159
#define CD_SERVICE_UNREGISTER 10033
#define CD_SERVICE_NAME 10031
#define CD_SERVICE_REGISTER 10032
#define CD_SERVICE_AUTOSTART 10034
#define CD_SERVICE_STATE 10035
#define CD_SERVICE_START 10036
#define CD_SERVICE_STOP 10037
////@end control identifiers

/*!
 * Compatibility
 */

#ifndef wxCLOSE_BOX
#define wxCLOSE_BOX 0x1000
#endif

/*!
 * DatabaseDlg class declaration
 */

class DatabaseDlg: public wxDialog
{    
    DECLARE_CLASS( DatabaseDlg )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    DatabaseDlg(tobjname server_nameIn);

    /// Creation
    bool Create( wxWindow* parent, wxWindowID id = -1, const wxString& caption = SYMBOL_DATABASEDLG_TITLE, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = SYMBOL_DATABASEDLG_STYLE );

    /// Creates the controls and sizers
    void CreateControls();

////@begin DatabaseDlg event handler declarations

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_FD_SEL2
    void OnCdFdSel2Click( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_FD_SEL3
    void OnCdFdSel3Click( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_FD_SEL4
    void OnCdFdSel4Click( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_FD_SELTRANS
    void OnCdFdSeltransClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_FD_SELLOG
    void OnCdFdSellogClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_UPDATED event handler for CD_RECORVERY_DIR
    void OnCdRecorveryDirUpdated( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_RECORVERY_DIRSEL
    void OnCdRecorveryDirselClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_LISTBOX_SELECTED event handler for CD_RECORVERY_LIST
    void OnCdRecorveryListSelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_RECORVERY_START
    void OnCdRecorveryStartClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_PATCH_START
    void OnCdPatchStartClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_ERASE_START
    void OnCdEraseStartClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_UPDATED event handler for CD_ED_APPLS
    void OnCdEdApplsUpdated( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_ED_APPLS
    void OnCdEdApplsClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_UPDATED event handler for CD_ED_BACK_FOLDER
    void OnCdEdBackFolderUpdated( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_ED_BUTBACKFOLDER
    void OnCdEdButbackfolderClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_ED_DUPLFIL
    void OnCdEdDuplfilClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_SERVICE_UNREGISTER
    void OnCdServiceUnregisterClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_UPDATED event handler for CD_SERVICE_NAME
    void OnCdServiceNameUpdated( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_SERVICE_REGISTER
    void OnCdServiceRegisterClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_SERVICE_START
    void OnCdServiceStartClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_SERVICE_STOP
    void OnCdServiceStopClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_OK
    void OnOkClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CANCEL
    void OnCancelClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_HELP
    void OnHelpClick( wxCommandEvent& event );

////@end DatabaseDlg event handler declarations
    void OnCdEdDuplFilUpdated( wxCommandEvent& event );
    //void OnIdle(wxIdleEvent& event) ; -- obsolete

////@begin DatabaseDlg member function declarations

    /// Retrieves bitmap resources
    wxBitmap GetBitmapResource( const wxString& name );

    /// Retrieves icon resources
    wxIcon GetIconResource( const wxString& name );
////@end DatabaseDlg member function declarations
 // backups:
  void list_backups(void);
 // IK:
  void FillLicencePoolIDs(void);
  void fill_server_group(void);
 // management:
  wxString FindRegisteredServiceForServer(void);
  void show_service_run_state(int running);
  bool get_autostart(wxString service_name);
  bool set_autostart(wxString service_name, bool autostart);
  int  is_service_running(wxString service_name);
  void show_service_register_state(wxString service_name);

    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin DatabaseDlg member variables
    wxTextCtrl* mPart1;
    wxTextCtrl* mLim1;
    wxTextCtrl* mPart2;
    wxButton* mSel2;
    wxTextCtrl* mLim2;
    wxTextCtrl* mPart3;
    wxButton* mSel3;
    wxTextCtrl* mLim3;
    wxTextCtrl* mPart4;
    wxButton* mSel4;
    wxTextCtrl* mTrans;
    wxButton* mSelTrans;
    wxTextCtrl* mLog;
    wxButton* mSelLog;
    wxOwnerDrawnComboBox* mServerLang;
    wxTextCtrl* mRecovDir;
    wxButton* mRecovDirSel;
    wxListBox* mRecovList;
    wxButton* mRestoreNow;
    wxTextCtrl* EDAppls;
    wxButton* Appls;
    wxTextCtrl* EDBackFolder;
    wxButton* Butbackfolder;
    wxButton* Duplfil;
    wxBoxSizer* mPanelSizer;
    wxStaticBox* mServiceBox;
    wxTextCtrl* mOldName;
    wxButton* mUnregister;
    wxTextCtrl* mServiceName;
    wxButton* mRegister;
    wxCheckBox* mAutoStart;
    wxStaticText* mState;
    wxButton* mStart;
    wxButton* mStop;
////@end DatabaseDlg member variables
  tobjname server_name;
  bool mApplsChanged;
  bool private_server;
};

#endif
    // _DATABASEDLG_H_
