/////////////////////////////////////////////////////////////////////////////
// Name:        FtxSearchDlg.h
// Purpose:     
// Author:      
// Modified by: 
// Created:     09/21/06 16:05:46
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifndef _FTXSEARCHDLG_H_
#define _FTXSEARCHDLG_H_

#if defined(__GNUG__) && !defined(__APPLE__)
//#pragma interface "FtxSearchDlg.cpp"
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
class wxBoxSizer;
class wxOwnerDrawnComboBox;
////@end forward declarations

/*!
 * Control identifiers
 */

////@begin control identifiers
#define ID_MYDIALOG9 10127
#define SYMBOL_FTXSEARCHDLG_STYLE wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU|wxCLOSE_BOX
#define SYMBOL_FTXSEARCHDLG_TITLE _("Search in the `%s` Fulltext System")
#define SYMBOL_FTXSEARCHDLG_IDNAME ID_MYDIALOG9
#define SYMBOL_FTXSEARCHDLG_SIZE wxSize(400, 300)
#define SYMBOL_FTXSEARCHDLG_POSITION wxDefaultPosition
#define CD_FTXSRCH_FIND 10128
#define CD_FTXSRCH_APPLY 10000
#define CD_FTXSRCH_LIST 10002
#define CD_FTXSRCH_TABLE 10170
#define CD_FTXSRCH_TXCOL 10171
#define CD_FTXSRCH_IDCOL 10172
#define CD_FTXSRCH_FORMAT 10173
#define CD_FTXSRCH_INDIRECT 10129
#define CD_FTXSRCH_PANEL 10001
////@end control identifiers

/*!
 * Compatibility
 */

#ifndef wxCLOSE_BOX
#define wxCLOSE_BOX 0x1000
#endif
#ifndef wxFIXED_MINSIZE
#define wxFIXED_MINSIZE 0
#endif

/*!
 * FtxSearchDlg class declaration
 */

class FtxSearchDlg: public wxDialog
{    
    DECLARE_CLASS( FtxSearchDlg )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    FtxSearchDlg(cdp_t cdpIn, tobjnum objnum);
    FtxSearchDlg( wxWindow* parent, wxWindowID id = SYMBOL_FTXSEARCHDLG_IDNAME, const wxString& caption = SYMBOL_FTXSEARCHDLG_TITLE, const wxPoint& pos = SYMBOL_FTXSEARCHDLG_POSITION, const wxSize& size = SYMBOL_FTXSEARCHDLG_SIZE, long style = SYMBOL_FTXSEARCHDLG_STYLE );

    /// Creation
    bool Create( wxWindow* parent, wxWindowID id = SYMBOL_FTXSEARCHDLG_IDNAME, const wxString& caption = SYMBOL_FTXSEARCHDLG_TITLE, const wxPoint& pos = SYMBOL_FTXSEARCHDLG_POSITION, const wxSize& size = SYMBOL_FTXSEARCHDLG_SIZE, long style = SYMBOL_FTXSEARCHDLG_STYLE );

    /// Creates the controls and sizers
    void CreateControls();

////@begin FtxSearchDlg event handler declarations

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_FTXSRCH_APPLY
    void OnCdFtxsrchApplyClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_COMBOBOX_SELECTED event handler for CD_FTXSRCH_TABLE
    void OnCdFtxsrchTableSelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CLOSE
    void OnCloseClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_HELP
    void OnHelpClick( wxCommandEvent& event );

////@end FtxSearchDlg event handler declarations

////@begin FtxSearchDlg member function declarations

    /// Retrieves bitmap resources
    wxBitmap GetBitmapResource( const wxString& name );

    /// Retrieves icon resources
    wxIcon GetIconResource( const wxString& name );
////@end FtxSearchDlg member function declarations

    /// Should we show tooltips?
    static bool ShowToolTips();
    wxString make_select_for_doc_source(const char * doctable_name, const char * id_column, const char * text_expr, const char * format, int mode, int wcnt, wxString fnd);

////@begin FtxSearchDlg member variables
    wxBoxSizer* mVertSizer;
    wxTextCtrl* mFind;
    wxButton* mApply;
    wxBoxSizer* mSizerAuto;
    wxCheckListBox* mList;
    wxBoxSizer* mSizerMan;
    wxOwnerDrawnComboBox* mTable;
    wxOwnerDrawnComboBox* mTxCol;
    wxOwnerDrawnComboBox* mIDCol;
    wxOwnerDrawnComboBox* mFormat;
    wxCheckBox* mIndirect;
    wxPanel* mPanel;
    wxBoxSizer* mSizerResult;
////@end FtxSearchDlg member variables
    cdp_t cdp;
    t_fulltext ftdef;  // the FT definition
    wxWindow * data;
};

#endif
    // _FTXSEARCHDLG_H_
