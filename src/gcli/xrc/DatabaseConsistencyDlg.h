/////////////////////////////////////////////////////////////////////////////
// Name:        DatabaseConsistencyDlg.h
// Purpose:     
// Author:      
// Modified by: 
// Created:     04/26/04 12:46:48
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifndef _DATABASECONSISTENCYDLG_H_
#define _DATABASECONSISTENCYDLG_H_

#if defined(__GNUG__) && !defined(__APPLE__)
//#pragma interface "DatabaseConsistencyDlg.cpp"
#endif

/*!
 * Includes
 */

////@begin includes
#include "wx/statline.h"
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
#define SYMBOL_DATABASECONSISTENCYDLG_STYLE wxCAPTION|wxSYSTEM_MENU|wxCLOSE_BOX
#define SYMBOL_DATABASECONSISTENCYDLG_TITLE _("Database Consistency Check")
#define SYMBOL_DATABASECONSISTENCYDLG_IDNAME ID_DIALOG
#define SYMBOL_DATABASECONSISTENCYDLG_SIZE wxSize(400, 300)
#define SYMBOL_DATABASECONSISTENCYDLG_POSITION wxDefaultPosition
#define CD_DBINT_ERRCAPT 10002
#define CD_DBINT_LOST 10001
#define CD_DBINT_LOSTERR 10003
#define CD_DBINT_CROSS 10004
#define CD_DBINT_CROSSERR 10009
#define CD_DBINT_DAMAGED 10005
#define CD_DBINT_DAMAGEDERR 10010
#define CD_DBINT_NONEX 10006
#define CD_DBINT_NONEXERR 10011
#define CD_DBINT_INDEX 10007
#define CD_DBINT_INDEXERR 10012
#define CD_DBINT_CACHE 10008
#define CD_DBINT_CACHEERR 10013
#define CD_DBINT_REPAIR 10014
////@end control identifiers

/*!
 * Compatibility
 */

#ifndef wxCLOSE_BOX
#define wxCLOSE_BOX 0x1000
#endif

/*!
 * DatabaseConsistencyDlg class declaration
 */

class DatabaseConsistencyDlg: public wxDialog
{    
    DECLARE_CLASS( DatabaseConsistencyDlg )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    DatabaseConsistencyDlg(cdp_t cdpIn);

    /// Creation
    bool Create( wxWindow* parent, wxWindowID id = SYMBOL_DATABASECONSISTENCYDLG_IDNAME, const wxString& caption = SYMBOL_DATABASECONSISTENCYDLG_TITLE, const wxPoint& pos = SYMBOL_DATABASECONSISTENCYDLG_POSITION, const wxSize& size = SYMBOL_DATABASECONSISTENCYDLG_SIZE, long style = SYMBOL_DATABASECONSISTENCYDLG_STYLE );

    /// Creates the controls and sizers
    void CreateControls();

////@begin DatabaseConsistencyDlg event handler declarations

    /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for CD_DBINT_INDEX
    void OnCdDbintIndexClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for CD_DBINT_CACHE
    void OnCdDbintCacheClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for CD_DBINT_REPAIR
    void OnCdDbintRepairClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_OK
    void OnOkClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CANCEL
    void OnCancelClick( wxCommandEvent& event );

////@end DatabaseConsistencyDlg event handler declarations
    void OnIdle( wxIdleEvent& event );

////@begin DatabaseConsistencyDlg member function declarations

    /// Retrieves bitmap resources
    wxBitmap GetBitmapResource( const wxString& name );

    /// Retrieves icon resources
    wxIcon GetIconResource( const wxString& name );
////@end DatabaseConsistencyDlg member function declarations

    /// Should we show tooltips?
    static bool ShowToolTips();
    void display1(void);
    void display2(void);
    void completed(void);

////@begin DatabaseConsistencyDlg member variables
    wxStaticText* mErrCapt;
    wxCheckBox* mLost;
    wxStaticText* mLostErr;
    wxCheckBox* mCross;
    wxStaticText* mCrossErr;
    wxCheckBox* mDamaged;
    wxStaticText* mDamagedErr;
    wxCheckBox* mNonex;
    wxStaticText* mNonexErr;
    wxCheckBox* mIndex;
    wxStaticText* mIndexErr;
    wxCheckBox* mCache;
    wxStaticText* mCacheErr;
    wxCheckBox* mRepair;
    wxButton* mStart;
    wxButton* mClose;
////@end DatabaseConsistencyDlg member variables
  cdp_t cdp;
  uns32 lost_blocks, lost_dheap, cross_link, damaged_tabdef, nonex_blocks, index_err, cache_err;
  int phase, extent;
};

#endif
    // _DATABASECONSISTENCYDLG_H_
