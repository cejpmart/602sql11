/////////////////////////////////////////////////////////////////////////////
// Name:        NewDataTransfer95Dlg.h
// Purpose:     
// Author:      
// Modified by: 
// Created:     07/22/05 08:36:26
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifndef _NEWDATATRANSFER95DLG_H_
#define _NEWDATATRANSFER95DLG_H_

#if defined(__GNUG__) && !defined(__APPLE__)
//#pragma interface "NewDataTransfer95Dlg.cpp"
#endif

/*!
 * Includes
 */

////@begin includes
#include "wx/wizard.h"
#include "wx/spinctrl.h"
////@end includes

/*!
 * Forward declarations
 */

////@begin forward declarations
class WizardPage2;
class WizardPage;
class wxOwnerDrawnComboBox;
class WizardPage1;
class WizardPage3;
class WizardPage4;
class wxSpinCtrl;
class WizardPageXSD;
class WizardPageDbxml;
////@end forward declarations

/*!
 * Control identifiers
 */

////@begin control identifiers
#define ID_MYWIZARD 10047
#define SYMBOL_NEWDATATRANSFER95DLG_IDNAME ID_MYWIZARD
#define ID_WIZARDPAGE2 10000
#define CD_TR_MAIN_SEL 10001
#define ID_WIZARDPAGE 10010
#define CD_TR_SOURCE_ORIGIN 10002
#define CD_TR_SOURCE_VIEW 10052
#define CD_TR_SOURCE_TYPE_C 10007
#define CD_TR_SOURCE_TYPE 10003
#define CD_TR_SOURCE_ENC_C 10011
#define CD_TR_SOURCE_ENC 10004
#define CD_TR_SOURCE_FNAME_C 10008
#define CD_TR_SOURCE_FNAME 10057
#define CD_TR_SOURCE_NAME_SEL 10005
#define CD_TR_SOURCE_SERVER_C 10058
#define CD_TR_SOURCE_SERVER 10059
#define CD_TR_SOURCE_SCHEMA_C 10056
#define CD_TR_SOURCE_SCHEMA 10055
#define CD_TR_SOURCE_OBJECT_C 10054
#define CD_TR_SOURCE_OBJECT 10053
#define CD_TR_CONDITION_C 10009
#define CD_TR_CONDITION 10006
#define ID_WIZARDPAGE1 10018
#define CD_TR_DEST_ORIGIN 10019
#define CD_TR_DEST_TYPE_C 10020
#define CD_TR_DEST_TYPE 10012
#define CD_TR_DEST_ENC_C 10021
#define CD_TR_DEST_ENC 10013
#define CD_TR_DEST_FNAME_C 10014
#define CD_TR_DEST_FNAME 10015
#define CD_TR_DEST_NAME_SEL 10016
#define CD_TR_DEST_SERVER_C 10022
#define CD_TR_DEST_SERVER 10060
#define CD_TR_DEST_SCHEMA_C 10061
#define CD_TR_DEST_SCHEMA 10062
#define CD_TR_DEST_OBJECT_C 10063
#define CD_TR_DEST_OBJECT 10064
#define CD_TR_INDEXES 10017
#define ID_WIZARDPAGE3 10023
#define CD_XMLSTART_TABLE 10024
#define CD_XMLSTART_EMPTY 10029
#define CD_XMLSTART_XSD 10030
#define ID_TEXTCTRL2 10031
#define CD_XMLSTART_SELSCHEMA 10032
#define CD_XMLSTART_FO 10130
#define ID_TEXTCTRL3 10065
#define CD_XMLSTART_SELFO 10066
#define ID_WIZARDPAGE4 10034
#define CD_TR_CRE_HEADER 10035
#define CD_TR_SKIP_HEADER_C 10036
#define CD_TR_SKIP_HEADER 10037
#define CD_TR_CSV_SEPAR 10038
#define CD_TR_CSV_DELIM 10039
#define CD_TR_DATE_FORMAT 10040
#define CD_TR_TIME_FORMAT 10041
#define CD_TR_TS_FORMAT 10042
#define CD_TR_BOOL_FORMAT 10043
#define CD_TR_DEC_SEPAR 10033
#define CD_TR_SEMILOG 10044
#define CD_TR_CR_LINES 10045
#define ID_WIZARDPAGE_XSD 10046
#define CD_XMLXSD_PREFIX 10048
#define CD_XMLXSD_LENGTH 10049
#define CD_XMLXSD_UNICODE 10050
#define CD_XMLXSD_IDTYPE 10168
#define CD_XMLXSD_DADNAME_CAPT 10132
#define CD_XMLXSD_DADNAME 10133
#define CD_XMLXSD_REFINT 10131
#define ID_WIZARDPAGE_DBXML 10141
#define CD_XMLDB_SERVER 10142
#define CD_XMLDB_BY_TABLE 10145
#define CD_XMLDB_SCHEMA_CAPT 10144
#define CD_XMLDB_SCHEMA 10143
#define CD_XMLDB_TABLENAME 10067
#define CD_XMLDB_TABLEALIAS 10068
#define CD_XMLDB_BY_QUERY 10146
#define CD_XMLDB_QUERY 10147
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
 * NewDataTransfer95Dlg class declaration
 */

class NewDataTransfer95Dlg: public wxWizard
{    
    DECLARE_CLASS( NewDataTransfer95Dlg )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    NewDataTransfer95Dlg(t_ie_run * dsgnIn, bool xmlIn, t_xsd_gen_params * gen_paramsIn);

    /// Creation
    bool Create( wxWindow* parent, wxWindowID id = SYMBOL_NEWDATATRANSFER95DLG_IDNAME, const wxPoint& pos = wxDefaultPosition );

    /// Creates the controls and sizers
    void CreateControls();

////@begin NewDataTransfer95Dlg event handler declarations

    /// wxEVT_WIZARD_HELP event handler for ID_MYWIZARD
    void OnMywizardHelp( wxWizardEvent& event );

////@end NewDataTransfer95Dlg event handler declarations

////@begin NewDataTransfer95Dlg member function declarations

    /// Runs the wizard.
    bool Run();

    /// Retrieves bitmap resources
    wxBitmap GetBitmapResource( const wxString& name );

    /// Retrieves icon resources
    wxIcon GetIconResource( const wxString& name );
////@end NewDataTransfer95Dlg member function declarations
  bool Run2();
  bool RunUpdate(int page);

    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin NewDataTransfer95Dlg member variables
    WizardPage2* mPageMainSel;
    WizardPage* mPageDataSource;
    WizardPage1* mPageDataTarget;
    WizardPage3* mPageXML;
    WizardPage4* mPageText;
    WizardPageXSD* mPageXSD;
    WizardPageDbxml* mPageDbxml;
////@end NewDataTransfer95Dlg member variables
    bool updating;
    bool xml_transport;
    t_ie_run * dsgn;
    t_xsd_gen_params * gen_params;
};

/*!
 * WizardPage2 class declaration
 */

class WizardPage2: public wxWizardPage
{    
    DECLARE_DYNAMIC_CLASS( WizardPage2 )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    WizardPage2( );

    WizardPage2( wxWizard* parent );

    /// Creation
    bool Create( wxWizard* parent );

    /// Creates the controls and sizers
    void CreateControls();

////@begin WizardPage2 event handler declarations

    /// wxEVT_WIZARD_PAGE_CHANGING event handler for ID_WIZARDPAGE2
    void OnWizardpage2PageChanging( wxWizardEvent& event );

////@end WizardPage2 event handler declarations

////@begin WizardPage2 member function declarations

    /// Gets the previous page.
    virtual wxWizardPage* GetPrev() const;

    /// Gets the next page.
    virtual wxWizardPage* GetNext() const;

    /// Retrieves bitmap resources
    wxBitmap GetBitmapResource( const wxString& name );

    /// Retrieves icon resources
    wxIcon GetIconResource( const wxString& name );
////@end WizardPage2 member function declarations

    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin WizardPage2 member variables
    wxRadioBox* mMainSel;
////@end WizardPage2 member variables
};

/*!
 * WizardPage class declaration
 */

class WizardPage: public wxWizardPage
{    
    DECLARE_DYNAMIC_CLASS( WizardPage )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    WizardPage( );

    WizardPage( wxWizard* parent );

    /// Creation
    bool Create( wxWizard* parent );

    /// Creates the controls and sizers
    void CreateControls();

////@begin WizardPage event handler declarations

    /// wxEVT_WIZARD_PAGE_CHANGED event handler for ID_WIZARDPAGE
    void OnWizardpagePageChanged( wxWizardEvent& event );

    /// wxEVT_WIZARD_PAGE_CHANGING event handler for ID_WIZARDPAGE
    void OnWizardpagePageChanging( wxWizardEvent& event );

    /// wxEVT_COMMAND_RADIOBOX_SELECTED event handler for CD_TR_SOURCE_ORIGIN
    void OnCdTrSourceOriginSelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for CD_TR_SOURCE_VIEW
    void OnCdTrSourceViewClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_COMBOBOX_SELECTED event handler for CD_TR_SOURCE_TYPE
    void OnCdTrSourceTypeSelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_TR_SOURCE_NAME_SEL
    void OnCdTrSourceNameSelClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_COMBOBOX_SELECTED event handler for CD_TR_SOURCE_SERVER
    void OnCdTrSourceServerSelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_COMBOBOX_SELECTED event handler for CD_TR_SOURCE_SCHEMA
    void OnCdTrSourceSchemaSelected( wxCommandEvent& event );

////@end WizardPage event handler declarations

////@begin WizardPage member function declarations

    /// Gets the previous page.
    virtual wxWizardPage* GetPrev() const;

    /// Gets the next page.
    virtual wxWizardPage* GetNext() const;

    /// Retrieves bitmap resources
    wxBitmap GetBitmapResource( const wxString& name );

    /// Retrieves icon resources
    wxIcon GetIconResource( const wxString& name );
////@end WizardPage member function declarations

    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin WizardPage member variables
    wxRadioBox* mSourceOrigin;
    wxCheckBox* mSourceView;
    wxStaticText* mSourceTypeC;
    wxOwnerDrawnComboBox* mSourceType;
    wxStaticText* mSrcEncC;
    wxOwnerDrawnComboBox* mSourceEnc;
    wxStaticText* mSourceFnameC;
    wxTextCtrl* mSourceFname;
    wxButton* mSourceNameSel;
    wxStaticText* mSourceServerC;
    wxOwnerDrawnComboBox* mSourceServer;
    wxStaticText* mSourceSchemaC;
    wxOwnerDrawnComboBox* mSourceSchema;
    wxStaticText* mSrcObjectC;
    wxOwnerDrawnComboBox* mSrcObject;
    wxStaticText* mConditionC;
    wxTextCtrl* mCondition;
////@end WizardPage member variables
};

/*!
 * WizardPage1 class declaration
 */

class WizardPage1: public wxWizardPage
{    
    DECLARE_DYNAMIC_CLASS( WizardPage1 )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    WizardPage1( );

    WizardPage1( wxWizard* parent );

    /// Creation
    bool Create( wxWizard* parent );

    /// Creates the controls and sizers
    void CreateControls();

////@begin WizardPage1 event handler declarations

    /// wxEVT_WIZARD_PAGE_CHANGED event handler for ID_WIZARDPAGE1
    void OnWizardpage1PageChanged( wxWizardEvent& event );

    /// wxEVT_WIZARD_PAGE_CHANGING event handler for ID_WIZARDPAGE1
    void OnWizardpage1PageChanging( wxWizardEvent& event );

    /// wxEVT_COMMAND_RADIOBOX_SELECTED event handler for CD_TR_DEST_ORIGIN
    void OnCdTrDestOriginSelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_COMBOBOX_SELECTED event handler for CD_TR_DEST_TYPE
    void OnCdTrDestTypeSelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_TR_DEST_NAME_SEL
    void OnCdTrDestNameSelClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_COMBOBOX_SELECTED event handler for CD_TR_DEST_SERVER
    void OnCdTrDestServerSelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_COMBOBOX_SELECTED event handler for CD_TR_DEST_SCHEMA
    void OnCdTrDestSchemaSelected( wxCommandEvent& event );

////@end WizardPage1 event handler declarations

////@begin WizardPage1 member function declarations

    /// Gets the previous page.
    virtual wxWizardPage* GetPrev() const;

    /// Gets the next page.
    virtual wxWizardPage* GetNext() const;

    /// Retrieves bitmap resources
    wxBitmap GetBitmapResource( const wxString& name );

    /// Retrieves icon resources
    wxIcon GetIconResource( const wxString& name );
////@end WizardPage1 member function declarations

    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin WizardPage1 member variables
    wxRadioBox* mDestOrigin;
    wxStaticText* mDestTypeC;
    wxOwnerDrawnComboBox* mDestType;
    wxStaticText* mDestEncC;
    wxOwnerDrawnComboBox* mDestEnc;
    wxStaticText* mDestFnameC;
    wxTextCtrl* mDestFname;
    wxButton* mDestNameSel;
    wxStaticText* mDestServerC;
    wxOwnerDrawnComboBox* mDestServer;
    wxStaticText* mDestSchemaC;
    wxOwnerDrawnComboBox* mDestSchema;
    wxStaticText* mDestObjectC;
    wxOwnerDrawnComboBox* mDestObject;
    wxRadioBox* mIndexes;
////@end WizardPage1 member variables
};

/*!
 * WizardPage3 class declaration
 */

enum t_xml_start_type { XST_UNDEF, XST_TABLE, XST_QUERY, XST_EMPTY, XST_XSD, XST_FO };

class WizardPage3: public wxWizardPage
{    
    DECLARE_DYNAMIC_CLASS( WizardPage3 )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    WizardPage3( );

    WizardPage3( wxWizard* parent );

    /// Creation
    bool Create( wxWizard* parent );

    /// Creates the controls and sizers
    void CreateControls();

////@begin WizardPage3 event handler declarations

    /// wxEVT_WIZARD_PAGE_CHANGED event handler for ID_WIZARDPAGE3
    void OnWizardpage3PageChanged( wxWizardEvent& event );

    /// wxEVT_WIZARD_PAGE_CHANGING event handler for ID_WIZARDPAGE3
    void OnWizardpage3PageChanging( wxWizardEvent& event );

    /// wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for CD_XMLSTART_TABLE
    void OnCdXmlstartTableSelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for CD_XMLSTART_EMPTY
    void OnCdXmlstartEmptySelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for CD_XMLSTART_XSD
    void OnCdXmlstartXsdSelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_XMLSTART_SELSCHEMA
    void OnCdXmlstartSelschemaClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for CD_XMLSTART_FO
    void OnCdXmlstartFoSelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_XMLSTART_SELFO
    void OnCdXmlstartSelfoClick( wxCommandEvent& event );

////@end WizardPage3 event handler declarations

////@begin WizardPage3 member function declarations

    /// Gets the previous page.
    virtual wxWizardPage* GetPrev() const;

    /// Gets the next page.
    virtual wxWizardPage* GetNext() const;

    /// Retrieves bitmap resources
    wxBitmap GetBitmapResource( const wxString& name );

    /// Retrieves icon resources
    wxIcon GetIconResource( const wxString& name );
////@end WizardPage3 member function declarations
  void enabling(t_xml_start_type sel);

    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin WizardPage3 member variables
    wxRadioButton* mByTable;
    wxRadioButton* mByEmpty;
    wxRadioButton* mBySchema;
    wxTextCtrl* mSchema;
    wxButton* mSelSchema;
    wxRadioButton* mByFO;
    wxTextCtrl* mFO;
    wxButton* mSelFO;
////@end WizardPage3 member variables
  t_xml_start_type xml_init;
  char server[MAX_OBJECT_NAME_LEN+1];
  char schema[MAX_OBJECT_NAME_LEN+1];
  tobjname table_name, table_alias;
  wxString query;
  d_table * new_td;       
  t_ucd_cl * new_ucdp;
  tcursnum new_top_curs;
};

/*!
 * WizardPage4 class declaration
 */

class WizardPage4: public wxWizardPage
{    
    DECLARE_DYNAMIC_CLASS( WizardPage4 )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    WizardPage4( );

    WizardPage4( wxWizard* parent );

    /// Creation
    bool Create( wxWizard* parent );

    /// Creates the controls and sizers
    void CreateControls();

////@begin WizardPage4 event handler declarations

    /// wxEVT_WIZARD_PAGE_CHANGED event handler for ID_WIZARDPAGE4
    void OnWizardpage4PageChanged( wxWizardEvent& event );

    /// wxEVT_WIZARD_PAGE_CHANGING event handler for ID_WIZARDPAGE4
    void OnWizardpage4PageChanging( wxWizardEvent& event );

////@end WizardPage4 event handler declarations

////@begin WizardPage4 member function declarations

    /// Gets the previous page.
    virtual wxWizardPage* GetPrev() const;

    /// Gets the next page.
    virtual wxWizardPage* GetNext() const;

    /// Retrieves bitmap resources
    wxBitmap GetBitmapResource( const wxString& name );

    /// Retrieves icon resources
    wxIcon GetIconResource( const wxString& name );
////@end WizardPage4 member function declarations

    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin WizardPage4 member variables
    wxCheckBox* mCreHeader;
    wxStaticText* mSkipHeaderC;
    wxSpinCtrl* mSkipHeader;
    wxOwnerDrawnComboBox* mCsvSepar;
    wxOwnerDrawnComboBox* mCsvDelim;
    wxOwnerDrawnComboBox* mDateFormat;
    wxOwnerDrawnComboBox* mTimeFormat;
    wxOwnerDrawnComboBox* mTsFormat;
    wxOwnerDrawnComboBox* mBoolFormat;
    wxOwnerDrawnComboBox* mDecSepar;
    wxCheckBox* mSemilog;
    wxCheckBox* mCRLines;
////@end WizardPage4 member variables
};

/*!
 * WizardPageXSD class declaration
 */

class WizardPageXSD: public wxWizardPage
{    
    DECLARE_DYNAMIC_CLASS( WizardPageXSD )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    WizardPageXSD( );

    WizardPageXSD( wxWizard* parent );

    /// Creation
    bool Create( wxWizard* parent );

    /// Creates the controls and sizers
    void CreateControls();

////@begin WizardPageXSD event handler declarations

    /// wxEVT_WIZARD_PAGE_CHANGED event handler for ID_WIZARDPAGE_XSD
    void OnWizardpageXsdPageChanged( wxWizardEvent& event );

    /// wxEVT_WIZARD_PAGE_CHANGING event handler for ID_WIZARDPAGE_XSD
    void OnWizardpageXsdPageChanging( wxWizardEvent& event );

////@end WizardPageXSD event handler declarations

////@begin WizardPageXSD member function declarations

    /// Gets the previous page.
    virtual wxWizardPage* GetPrev() const;

    /// Gets the next page.
    virtual wxWizardPage* GetNext() const;

    /// Retrieves bitmap resources
    wxBitmap GetBitmapResource( const wxString& name );

    /// Retrieves icon resources
    wxIcon GetIconResource( const wxString& name );
////@end WizardPageXSD member function declarations

    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin WizardPageXSD member variables
    wxTextCtrl* mPrefix;
    wxSpinCtrl* mLength;
    wxCheckBox* mUnicode;
    wxOwnerDrawnComboBox* mIDType;
    wxStaticText* mDADNameCapt;
    wxTextCtrl* mDADName;
    wxRadioBox* mRefInt;
////@end WizardPageXSD member variables
};

wxString encoding_name(int recode);

/*!
 * WizardPageDbxml class declaration
 */

class WizardPageDbxml: public wxWizardPage
{    
    DECLARE_DYNAMIC_CLASS( WizardPageDbxml )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    WizardPageDbxml( );

    WizardPageDbxml( wxWizard* parent );

    /// Creation
    bool Create( wxWizard* parent );

    /// Creates the controls and sizers
    void CreateControls();

////@begin WizardPageDbxml event handler declarations

    /// wxEVT_WIZARD_PAGE_CHANGED event handler for ID_WIZARDPAGE_DBXML
    void OnWizardpageDbxmlPageChanged( wxWizardEvent& event );

    /// wxEVT_WIZARD_PAGE_CHANGING event handler for ID_WIZARDPAGE_DBXML
    void OnWizardpageDbxmlPageChanging( wxWizardEvent& event );

    /// wxEVT_COMMAND_COMBOBOX_SELECTED event handler for CD_XMLDB_SERVER
    void OnCdXmldbServerSelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for CD_XMLDB_BY_TABLE
    void OnCdXmldbByTableSelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_COMBOBOX_SELECTED event handler for CD_XMLDB_SCHEMA
    void OnCdXmldbSchemaSelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for CD_XMLDB_BY_QUERY
    void OnCdXmldbByQuerySelected( wxCommandEvent& event );

////@end WizardPageDbxml event handler declarations

////@begin WizardPageDbxml member function declarations

    /// Gets the previous page.
    virtual wxWizardPage* GetPrev() const;

    /// Gets the next page.
    virtual wxWizardPage* GetNext() const;

    /// Retrieves bitmap resources
    wxBitmap GetBitmapResource( const wxString& name );

    /// Retrieves icon resources
    wxIcon GetIconResource( const wxString& name );
////@end WizardPageDbxml member function declarations
    void enabling(t_xml_start_type sel);

    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin WizardPageDbxml member variables
    wxOwnerDrawnComboBox* mServer;
    wxRadioButton* mByTable;
    wxStaticText* mSchemaC;
    wxOwnerDrawnComboBox* mSchema;
    wxStaticText* mTableNameC;
    wxOwnerDrawnComboBox* mTableName;
    wxStaticText* mTableAliasC;
    wxTextCtrl* mTableAlias;
    wxRadioButton* mByQuery;
    wxTextCtrl* mQuery;
////@end WizardPageDbxml member variables
};

#endif
    // _NEWDATATRANSFER95DLG_H_
