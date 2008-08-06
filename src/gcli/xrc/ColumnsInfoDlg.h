/////////////////////////////////////////////////////////////////////////////
// Name:        ColumnsInfoDlg.h
// Purpose:     
// Author:      
// Modified by: 
// Created:     11/30/07 10:45:38
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifndef _COLUMNSINFODLG_H_
#define _COLUMNSINFODLG_H_

#if defined(__GNUG__) && !defined(__APPLE__)
//#pragma interface "ColumnsInfoDlg.cpp"
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
#define ID_MYDIALOG16 10180
#define SYMBOL_COLUMNSINFODLG_STYLE 0
#define SYMBOL_COLUMNSINFODLG_TITLE _("Columns Info")
#define SYMBOL_COLUMNSINFODLG_IDNAME ID_MYDIALOG16
#define SYMBOL_COLUMNSINFODLG_SIZE wxSize(400, 300)
#define SYMBOL_COLUMNSINFODLG_POSITION wxDefaultPosition
#define CD_COLI_SCHEMA 10181
#define CD_COLI_CATEG 10182
#define CD_COLI_TABLE 10183
#define CD_COLI_COPY 10186
#define D_COLI_LIST 10184
#define CD_COLI_TYPE 10185
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
 * ColumnsInfoDlg class declaration
 */

class ColumnsInfoDlg: public wxPanel
{    
    DECLARE_DYNAMIC_CLASS( ColumnsInfoDlg )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    ColumnsInfoDlg( );
    ColumnsInfoDlg( wxWindow* parent, wxWindowID id = SYMBOL_COLUMNSINFODLG_IDNAME, const wxPoint& pos = SYMBOL_COLUMNSINFODLG_POSITION, const wxSize& size = SYMBOL_COLUMNSINFODLG_SIZE, long style = SYMBOL_COLUMNSINFODLG_STYLE );

    /// Creation
    bool Create( wxWindow* parent, wxWindowID id = SYMBOL_COLUMNSINFODLG_IDNAME, const wxPoint& pos = SYMBOL_COLUMNSINFODLG_POSITION, const wxSize& size = SYMBOL_COLUMNSINFODLG_SIZE, long style = SYMBOL_COLUMNSINFODLG_STYLE );

    /// Creates the controls and sizers
    void CreateControls();

////@begin ColumnsInfoDlg event handler declarations

    /// wxEVT_COMMAND_COMBOBOX_SELECTED event handler for CD_COLI_SCHEMA
    void OnCdColiSchemaSelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_RADIOBOX_SELECTED event handler for CD_COLI_CATEG
    void OnCdColiCategSelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_COMBOBOX_SELECTED event handler for CD_COLI_TABLE
    void OnCdColiTableSelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_COLI_COPY
    void OnCdColiCopyClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_LISTBOX_SELECTED event handler for D_COLI_LIST
    void OnDColiListSelected( wxCommandEvent& event );

////@end ColumnsInfoDlg event handler declarations

////@begin ColumnsInfoDlg member function declarations

    /// Retrieves bitmap resources
    wxBitmap GetBitmapResource( const wxString& name );

    /// Retrieves icon resources
    wxIcon GetIconResource( const wxString& name );
////@end ColumnsInfoDlg member function declarations
    void fill_tables(void);
    void Push_cdp(xcdp_t xcdpIn);
    void Disconnecting(xcdp_t dis_xcdp)
    { if (cdp && cdp==dis_xcdp->get_cdp())
        Push_cdp(NULL);
    }    

    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin ColumnsInfoDlg member variables
    wxComboBox* mSchema;
    wxRadioBox* mCateg;
    wxComboBox* mTable;
    wxButton* mCopy;
    wxListBox* mList;
    wxTextCtrl* mType;
////@end ColumnsInfoDlg member variables
    cdp_t cdp;
};

#endif
    // _COLUMNSINFODLG_H_
