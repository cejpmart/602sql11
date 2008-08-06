/////////////////////////////////////////////////////////////////////////////
// Name:        AutoBackupDlg.h
// Purpose:     
// Author:      
// Modified by: 
// Created:     05/03/04 08:55:00
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifndef _AUTOBACKUPDLG_H_
#define _AUTOBACKUPDLG_H_

#if defined(__GNUG__) && !defined(__APPLE__)
//#pragma interface "AutoBackupDlg.cpp"
#endif

/*!
 * Includes
 */

////@begin includes
#include "wx/grid.h"
////@end includes

/*!
 * Forward declarations
 */

////@begin forward declarations
class wxBoxSizer;
class wxGrid;
////@end forward declarations

/*!
 * Control identifiers
 */

////@begin control identifiers
#define ID_DIALOG 10000
#define SYMBOL_AUTOBACKUPDLG_STYLE wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU|wxCLOSE_BOX
#define SYMBOL_AUTOBACKUPDLG_TITLE _("Database File Automatic Backups")
#define SYMBOL_AUTOBACKUPDLG_IDNAME ID_DIALOG
#define SYMBOL_AUTOBACKUPDLG_SIZE wxSize(400, 300)
#define SYMBOL_AUTOBACKUPDLG_POSITION wxDefaultPosition
#define CD_BCK_DIR 10007
#define CD_BCK_SELECT 10008
#define CD_BCK_UNC 10187
#define CD_BCK_NO 10001
#define CD_BCK_PERIOD 10002
#define CD_BCK_TIME 10003
#define CD_BCK_HOURS 10004
#define CD_BCK_MINUTES 10005
#define CD_BCK_GRID 10006
#define CD_BCK_NONBLOCKING 10152
#define CD_BCK_IS_LIMIT 10013
#define CD_BCK_LIMIT 10011
#define CD_BCK_ZIP 10012
#define CD_BCK_ZIPMOVE 10009
#define CD_BCK_ZIPMOVE_SEL 10010
#define CD_BCK_NOW 10149
////@end control identifiers

/*!
 * Compatibility
 */

#ifndef wxCLOSE_BOX
#define wxCLOSE_BOX 0x1000
#endif

/*!
 * AutoBackupDlg class declaration
 */

class AutoBackupDlg: public wxDialog
{    
    DECLARE_CLASS( AutoBackupDlg )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    AutoBackupDlg(cdp_t cdpIn);
    /// Creation
    bool Create( wxWindow* parent, wxWindowID id = SYMBOL_AUTOBACKUPDLG_IDNAME, const wxString& caption = SYMBOL_AUTOBACKUPDLG_TITLE, const wxPoint& pos = SYMBOL_AUTOBACKUPDLG_POSITION, const wxSize& size = SYMBOL_AUTOBACKUPDLG_SIZE, long style = SYMBOL_AUTOBACKUPDLG_STYLE );

    /// Creates the controls and sizers
    void CreateControls();

////@begin AutoBackupDlg event handler declarations

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_BCK_SELECT
    void OnCdBckSelectClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_BCK_UNC
    void OnCdBckUncClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for CD_BCK_NO
    void OnCdBckNoSelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for CD_BCK_PERIOD
    void OnCdBckPeriodSelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for CD_BCK_TIME
    void OnCdBckTimeSelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for CD_BCK_IS_LIMIT
    void OnCdBckIsLimitClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for CD_BCK_ZIP
    void OnCdBckZipClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_BCK_ZIPMOVE_SEL
    void OnCdBckZipmoveSelClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_OK
    void OnOkClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CANCEL
    void OnCancelClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_BCK_NOW
    void OnCdBckNowClick( wxCommandEvent& event );

////@end AutoBackupDlg event handler declarations

////@begin AutoBackupDlg member function declarations

    /// Retrieves bitmap resources
    wxBitmap GetBitmapResource( const wxString& name );

    /// Retrieves icon resources
    wxIcon GetIconResource( const wxString& name );
////@end AutoBackupDlg member function declarations
  void backup_enabling(void);

    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin AutoBackupDlg member variables
    wxBoxSizer* mSizerContainingGrid;
    wxTextCtrl* mDir;
    wxButton* mSelect;
    wxButton* mUNC;
    wxRadioButton* mNo;
    wxRadioButton* mPeriod;
    wxRadioButton* mTime;
    wxTextCtrl* mHours;
    wxTextCtrl* mMinutes;
    wxGrid* mGrid;
    wxCheckBox* mNonBlocking;
    wxCheckBox* mIsLimit;
    wxTextCtrl* mLimit;
    wxCheckBox* mZip;
    wxTextCtrl* mZipMove;
    wxButton* mZipMoveSel;
    wxButton* mOK;
    wxButton* mBackupNow;
////@end AutoBackupDlg member variables
  cdp_t cdp;
  bool orig_nonblocking;
};

#endif
    // _AUTOBACKUPDLG_H_
