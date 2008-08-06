/////////////////////////////////////////////////////////////////////////////
// Name:        CreateODBCDataSourceDlg.h
// Purpose:     
// Author:      
// Modified by: 
// Created:     06/06/07 16:20:57
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifndef _CREATEODBCDATASOURCEDLG_H_
#define _CREATEODBCDATASOURCEDLG_H_

#if defined(__GNUG__) && !defined(__APPLE__)
//#pragma interface "CreateODBCDataSourceDlg.cpp"
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
#define ID_MYDIALOG12 10154
#define SYMBOL_CREATEODBCDATASOURCEDLG_STYLE wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU|wxCLOSE_BOX
#define SYMBOL_CREATEODBCDATASOURCEDLG_TITLE _("Create ODBC Data Source for the Schema")
#define SYMBOL_CREATEODBCDATASOURCEDLG_IDNAME ID_MYDIALOG12
#define SYMBOL_CREATEODBCDATASOURCEDLG_SIZE wxSize(400, 300)
#define SYMBOL_CREATEODBCDATASOURCEDLG_POSITION wxDefaultPosition
#define CD_CREODBC_TYPE 10155
#define CD_CREODBC_SERVER_NAME 10156
#define CD_CREODBC_SCHEMA_NAME 10157
#define CD_CREODBC_DATA_SOURCE_NAME 10158
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
 * CreateODBCDataSourceDlg class declaration
 */

class CreateODBCDataSourceDlg: public wxDialog
{    
    DECLARE_CLASS( CreateODBCDataSourceDlg )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    CreateODBCDataSourceDlg(wxString server_nameIn, wxString schema_nameIn);
    CreateODBCDataSourceDlg( wxWindow* parent, wxWindowID id = SYMBOL_CREATEODBCDATASOURCEDLG_IDNAME, const wxString& caption = SYMBOL_CREATEODBCDATASOURCEDLG_TITLE, const wxPoint& pos = SYMBOL_CREATEODBCDATASOURCEDLG_POSITION, const wxSize& size = SYMBOL_CREATEODBCDATASOURCEDLG_SIZE, long style = SYMBOL_CREATEODBCDATASOURCEDLG_STYLE );

    /// Creation
    bool Create( wxWindow* parent, wxWindowID id = SYMBOL_CREATEODBCDATASOURCEDLG_IDNAME, const wxString& caption = SYMBOL_CREATEODBCDATASOURCEDLG_TITLE, const wxPoint& pos = SYMBOL_CREATEODBCDATASOURCEDLG_POSITION, const wxSize& size = SYMBOL_CREATEODBCDATASOURCEDLG_SIZE, long style = SYMBOL_CREATEODBCDATASOURCEDLG_STYLE );

    /// Creates the controls and sizers
    void CreateControls();

////@begin CreateODBCDataSourceDlg event handler declarations

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_OK
    void OnOkClick( wxCommandEvent& event );

////@end CreateODBCDataSourceDlg event handler declarations

////@begin CreateODBCDataSourceDlg member function declarations

    /// Retrieves bitmap resources
    wxBitmap GetBitmapResource( const wxString& name );

    /// Retrieves icon resources
    wxIcon GetIconResource( const wxString& name );
////@end CreateODBCDataSourceDlg member function declarations

    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin CreateODBCDataSourceDlg member variables
    wxRadioBox* mSourceType;
    wxStaticText* mServerName;
    wxStaticText* mSchemaName;
    wxTextCtrl* mDataSourceName;
////@end CreateODBCDataSourceDlg member variables
    wxString server_name, schema_name;
};

#endif
    // _CREATEODBCDATASOURCEDLG_H_
