/////////////////////////////////////////////////////////////////////////////
// Name:        XMLElementDlg.h
// Purpose:     
// Author:      
// Modified by: 
// Created:     03/23/04 13:47:11
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifndef _XMLELEMENTDLG_H_
#define _XMLELEMENTDLG_H_

#ifdef __GNUG__
//#pragma interface "XMLElementDlg.cpp"
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
#define SYMBOL_XMLELEMENTDLG_STYLE wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU|wxCLOSE_BOX
#define SYMBOL_XMLELEMENTDLG_TITLE _("XML element properties")
#define SYMBOL_XMLELEMENTDLG_IDNAME ID_DIALOG
#define SYMBOL_XMLELEMENTDLG_SIZE wxSize(400, 300)
#define SYMBOL_XMLELEMENTDLG_POSITION wxDefaultPosition
#define CD_XML_NAME 10001
#define CD_XML_RELATION 10002
#define CD_XML_RECUR 10025
#define CD_XML_TABLENAME_C 10003
#define CD_XML_TABLENAME 10004
#define CD_XML_ALIAS_C 10005
#define CD_XML_ALIAS 10006
#define CD_XML_COLUMNNAME_C 10007
#define CD_XML_COLUMNNAME 10008
#define CD_XML_UTABLENAME_C 10009
#define CD_XML_UTABLENAME 10010
#define CD_XML_UCOLUMNNAME_C 10011
#define CD_XML_UCOLUMNNAME 10012
#define CD_XML_ORDERBY_C 10013
#define CD_XML_ORDERBY 10014
#define CD_XML_COND_C 10015
#define CD_XML_COND 10016
#define CD_XML_MULTIOCC 10017
#define CD_XML_INFOCOL 10018
#define CD_XML_ITABLENAME_C 10019
#define CD_XML_ITABLENAME 10020
#define CD_XML_ICOLUMNNAME_C 10021
#define CD_XML_ICOLUMNNAME 10022
#define CD_XML_ELEMCOND_C 10023
#define CD_XML_ELEMCOND 10024
////@end control identifiers

/*!
 * Compatibility
 */

#ifndef wxCLOSE_BOX
#define wxCLOSE_BOX 0x1000
#endif

/*!
 * XMLElementDlg class declaration
 */

class XMLElementDlg: public wxDialog
{    
    DECLARE_CLASS( XMLElementDlg )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    XMLElementDlg(xml_designer * dadeIn, dad_element_node * elIn);

    /// Creation
    bool Create( wxWindow* parent, wxWindowID id = -1, const wxString& caption = _("XML element properties"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU|wxCLOSE_BOX );

    /// Creates the controls and sizers
    void CreateControls();

////@begin XMLElementDlg event handler declarations

    /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for CD_XML_RELATION
    void OnCdXmlRelationClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for CD_XML_RECUR
    void OnCdXmlRecurClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_UPDATED event handler for CD_XML_TABLENAME
    void OnCdXmlTablenameUpdated( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_UPDATED event handler for CD_XML_ALIAS
    void OnCdXmlAliasUpdated( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_UPDATED event handler for CD_XML_UTABLENAME
    void OnCdXmlUtablenameUpdated( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for CD_XML_INFOCOL
    void OnCdXmlInfocolClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_UPDATED event handler for CD_XML_ITABLENAME
    void OnCdXmlItablenameUpdated( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_OK
    void OnOkClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CANCEL
    void OnCancelClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_HELP
    void OnHelpClick( wxCommandEvent& event );

////@end XMLElementDlg event handler declarations

////@begin XMLElementDlg member function declarations

    /// Retrieves bitmap resources
    wxBitmap GetBitmapResource( const wxString& name );

    /// Retrieves icon resources
    wxIcon GetIconResource( const wxString& name );
////@end XMLElementDlg member function declarations
  void enable_rel(bool enable_, bool not_recur);
  void enable_infocol(bool enable);
  void table_dependent_enabling(bool joining);

    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin XMLElementDlg member variables
    wxTextCtrl* mName;
    wxCheckBox* mRelation;
    wxStaticBox* mRelationGroup;
    wxCheckBox* mRecur;
    wxStaticText* mTablenameC;
    wxComboBox* mTablename;
    wxStaticText* mAliasC;
    wxTextCtrl* mAlias;
    wxStaticText* mColumnnameC;
    wxComboBox* mColumnname;
    wxStaticText* mUtablenameC;
    wxComboBox* mUtablename;
    wxStaticText* mUcolumnnameC;
    wxComboBox* mUcolumnname;
    wxStaticText* mOrderbyC;
    wxTextCtrl* mOrderby;
    wxStaticText* mCondC;
    wxTextCtrl* mCond;
    wxCheckBox* mMultiocc;
    wxCheckBox* mInfocol;
    wxStaticBox* iInfoGroup;
    wxStaticText* mItablenameC;
    wxComboBox* mItablename;
    wxStaticText* mIcolumnnameC;
    wxComboBox* mIcolumnname;
    wxStaticText* mElemcondC;
    wxTextCtrl* mElemcond;
////@end XMLElementDlg member variables
  xml_designer * dade;
  cdp_t cdp;
  dad_element_node * el;
};

#endif
    // _XMLELEMENTDLG_H_
