/////////////////////////////////////////////////////////////////////////////
// Name:        SQLConsoleDlg.h
// Purpose:     
// Author:      
// Modified by: 
// Created:     01/23/04 13:37:35
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifndef _SQLCONSOLEDLG_H_
#define _SQLCONSOLEDLG_H_

#ifdef __GNUG__
//#pragma interface "SQLConsoleDlg.cpp"
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
#define SYMBOL_SQLCONSOLEDLG_STYLE wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU|wxCLOSE_BOX
#define SYMBOL_SQLCONSOLEDLG_TITLE _("SQLConsole")
#define SYMBOL_SQLCONSOLEDLG_IDNAME ID_DIALOG
#define SYMBOL_SQLCONSOLEDLG_SIZE wxSize(400, 200)
#define SYMBOL_SQLCONSOLEDLG_POSITION wxDefaultPosition
#define CD_SQLCONS_SCHEMA 10006
#define CD_SQLCONS_STMT 10002
#define CD_SQLCONS_DEBUG 10001
#define CD_SQLCONS_EXEC 10003
#define CD_SQLCONS_PREV 10007
#define CD_SQLCONS_NEXT 10008
////@end control identifiers

/*!
 * Compatibility
 */

#ifndef wxCLOSE_BOX
#define wxCLOSE_BOX 0x1000
#endif

/*!
 * SQLConsoleDlg class declaration
 */

class SQLConsoleDlg: public wxPanel
{    
    DECLARE_CLASS( SQLConsoleDlg )
    DECLARE_EVENT_TABLE()
    tobjname schema;

public:
    /// Constructors
    SQLConsoleDlg( );
    ~SQLConsoleDlg( );

    /// Creation
    bool Create( wxWindow* parent, wxWindowID id = -1, const wxString& caption = _("SQLConsole"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = SYMBOL_SQLCONSOLEDLG_STYLE);

    /// Creates the controls and sizers
    void CreateControls();

////@begin SQLConsoleDlg event handler declarations

    /// wxEVT_COMMAND_TEXT_ENTER event handler for CD_SQLCONS_STMT
    void OnCdSqlconsStmtEnter( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_SQLCONS_EXEC
    void OnCdSqlconsExecClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_SQLCONS_PREV
    void OnCdSqlconsPrevClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_SQLCONS_NEXT
    void OnCdSqlconsNextClick( wxCommandEvent& event );

////@end SQLConsoleDlg event handler declarations

////@begin SQLConsoleDlg member function declarations

    cdp_t GetCdp() const { return cdp ; }
    void SetCdp(cdp_t value) { cdp = value ; }

    /// Retrieves bitmap resources
    wxBitmap GetBitmapResource( const wxString& name );

    /// Retrieves icon resources
    wxIcon GetIconResource( const wxString& name );
////@end SQLConsoleDlg member function declarations

    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin SQLConsoleDlg member variables
    wxStaticText* mSchema;
    wxTextCtrl* mCmd;
    wxCheckBox* mDebug;
    wxButton* mExec;
    wxBitmapButton* mPrev;
    wxBitmapButton* mNext;
    cdp_t cdp;
////@end SQLConsoleDlg member variables
#define MAX_OLD_STMTS      15
#define MAX_SQL_STATEMENTS 30
#define MAX_RESULT_LINES   100

  xcdp_t xcdp;
  wxString old_sql_statements[MAX_OLD_STMTS];
  int stmt_list_pos;
  void Push_cdp(xcdp_t xcdpIn);
  void Disconnecting(xcdp_t dis_xcdp);
  void add_line(wxString buf);
  void add_unbroken(wxString msg);
  void add_error(cdp_t acdp);
  void add_to_stmt_list(wxString & stmt);
  void write_old_stmts(void);
  bool process_stmt(wxString & stmt, bool debugged);
  void display_results(uns32 * results, cdp_t cdp2);
  void EnableControls(bool enable);
};
                       
#endif
    // _SQLCONSOLEDLG_H_
