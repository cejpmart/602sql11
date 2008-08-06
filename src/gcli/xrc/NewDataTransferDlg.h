/////////////////////////////////////////////////////////////////////////////
// Name:        NewDataTransferDlg.h
// Purpose:     
// Author:      
// Modified by: 
// Created:     04/02/04 14:05:23
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifndef _NEWDATATRANSFERDLG_H_
#define _NEWDATATRANSFERDLG_H_

#ifdef __GNUG__
//#pragma interface "NewDataTransferDlg.cpp"
#endif

/*!
 * Includes
 */

////@begin includes
#include "wx/wizard.h"
#include "wx/spinctrl.h"
////@end includes
#include <wx/wizard.h>

/*!
 * Forward declarations
 */

////@begin forward declarations
class WizardPage2;
class WizardPage;
class WizardPage1;
class WizardPage3;
class WizardPage4;
class wxSpinCtrl;
class WizardPageXSD;
////@end forward declarations

/*!
 * Control identifiers
 */

////@begin control identifiers
#define ID_WIZARD 10000
#define SYMBOL_NEWDATATRANSFERDLG_IDNAME ID_WIZARD
#define ID_WIZARDPAGE2 10018
#define CD_TR_MAIN_SEL 10019
#define ID_WIZARDPAGE 10001
#define CD_TR_SOURCE_ORIGIN 10002
#define CD_TR_SOURCE_TYPE_C 10007
#define CD_TR_SOURCE_TYPE 10003
#define CD_TR_SOURCE_ENC_C 10036
#define CD_TR_SOURCE_ENC 10037
#define CD_TR_SOURCE_NAME_C 10008
#define CD_TR_SOURCE_NAME 10004
#define CD_TR_SOURCE_NAME_SEL 10005
#define CD_TR_CONDITION_C 10009
#define CD_TR_CONDITION 10006
#define ID_WIZARDPAGE1 10010
#define CD_TR_DEST_ORIGIN 10011
#define CD_TR_DEST_TYPE_C 10012
#define CD_TR_DEST_TYPE 10013
#define CD_TR_DEST_ENC_C 10035
#define CD_TR_DEST_ENC 10034
#define CD_TR_DEST_NAME_C 10014
#define CD_TR_DEST_NAME 10015
#define CD_TR_DEST_NAME_SEL 10016
#define CD_TR_INDEXES 10017
#define ID_WIZARDPAGE3 10020
#define CD_XMLSTART_TABLE 10001
#define ID_COMBOBOX 10004
#define ID_TEXTCTRL 10005
#define CD_XMLSTART_QUERY 10002
#define ID_TEXTCTRL1 10006
#define CD_XMLSTART_EMPTY 10003
#define CD_XMLSTART_SCHEMA 10038
#define CD_XMLSTART_SELSCHEMA 10040
#define ID_WIZARDPAGE4 10021
#define CD_TR_CRE_HEADER 10022
#define CD_TR_SKIP_HEADER_C 10030
#define CD_TR_SKIP_HEADER 10023
#define CD_TR_CSV_SEPAR 10024
#define CD_TR_CSV_DELIM 10025
#define CD_TR_DATE_FORMAT 10026
#define CD_TR_TIME_FORMAT 10027
#define CD_TR_TS_FORMAT 10031
#define CD_TR_BOOL_FORMAT 10032
#define CD_TR_DEC_SEPAR 10033
#define CD_TR_SEMILOG 10028
#define CD_TR_CR_LINES 10029
#define ID_WIZARDPAGE_XSD 10041
#define CD_XMLXSD_PREFIX 10042
#define CD_XMLXSD_LENGTH 10043
#define CD_XMLXSD_UNICODE 10044
#define CD_XMLXSD_IDTYPE 10045
////@end control identifiers

/*!
 * Compatibility
 */

#ifndef wxCLOSE_BOX
#define wxCLOSE_BOX 0x1000
#endif

/*!
 * NewDataTransferDlg class declaration
 */

class NewDataTransferDlg: public wxWizard
{    
    DECLARE_CLASS( NewDataTransferDlg )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    NewDataTransferDlg(t_ie_run * dsgnIn, bool xmlIn, t_xsd_gen_params * gen_paramsIn);

    /// Creation
    bool Create( wxWindow* parent, wxWindowID id = ID_WIZARD, const wxPoint& pos = wxDefaultPosition );

    /// Creates the controls and sizers
    void CreateControls();

////@begin NewDataTransferDlg event handler declarations

    /// wxEVT_WIZARD_HELP event handler for ID_WIZARD
    void OnWizardHelp( wxWizardEvent& event );

////@end NewDataTransferDlg event handler declarations

////@begin NewDataTransferDlg member function declarations

    /// Runs the wizard.
    bool Run();

    /// Retrieves bitmap resources
    wxBitmap GetBitmapResource( const wxString& name );

    /// Retrieves icon resources
    wxIcon GetIconResource( const wxString& name );
////@end NewDataTransferDlg member function declarations
  bool Run2();
  bool RunUpdate(int page);

    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin NewDataTransferDlg member variables
    WizardPage2* mPageMainSel;
    WizardPage* mPageDataSource;
    WizardPage1* mPageDataTarget;
    WizardPage3* mPageXML;
    WizardPage4* mPageText;
    WizardPageXSD* mPageXSD;
////@end NewDataTransferDlg member variables
    bool updating;
    bool xml_transport;
    t_ie_run * dsgn;
    t_xsd_gen_params * gen_params;
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

    /// wxEVT_COMMAND_COMBOBOX_SELECTED event handler for CD_TR_SOURCE_TYPE
    void OnCdTrSourceTypeSelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_TR_SOURCE_NAME_SEL
    void OnCdTrSourceNameSelClick( wxCommandEvent& event );

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
  void fill_source_types(int selection);
  void fill_source_objects(t_ie_run * dsgn);

    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin WizardPage member variables
    wxRadioBox* mSourceOrigin;
    wxStaticText* mSourceTypeC;
    wxComboBox* mSourceType;
    wxStaticText* mSrcEncC;
    wxComboBox* mSourceEnc;
    wxStaticText* mSourceNameC;
    wxComboBox* mSourceName;
    wxButton* mSourceNameSel;
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

    /// wxEVT_COMMAND_COMBOBOX_SELECTED event handler for CD_TR_DEST_NAME
    void OnCdTrDestNameSelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_TR_DEST_NAME_SEL
    void OnCdTrDestNameSelClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_RADIOBOX_SELECTED event handler for CD_TR_INDEXES
    void OnCdTrIndexesSelected( wxCommandEvent& event );

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
  void fill_dest_types(int selection);
  void fill_dest_objects(t_ie_run * dsgn);

    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin WizardPage1 member variables
    wxRadioBox* mDestOrigin;
    wxStaticText* mDestTypeC;
    wxComboBox* mDestType;
    wxStaticText* mDestEncC;
    wxComboBox* mDestEnc;
    wxStaticText* mDestNameC;
    wxComboBox* mDestName;
    wxButton* mDestNameSel;
    wxRadioBox* mIndexes;
////@end WizardPage1 member variables
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

    /// wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for CD_XMLSTART_QUERY
    void OnCdXmlstartQuerySelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for CD_XMLSTART_EMPTY
    void OnCdXmlstartEmptySelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for CD_XMLSTART_SCHEMA
    void OnCdXmlstartSchemaSelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_XMLSTART_SELSCHEMA
    void OnCdXmlstartSelschemaClick( wxCommandEvent& event );

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
  void enabling(int sel);

    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin WizardPage3 member variables
    wxRadioButton* mByTable;
    wxStaticText* mTableNameC;
    wxComboBox* mTableName;
    wxStaticText* mTableAliasC;
    wxTextCtrl* mTableAlias;
    wxRadioButton* mByQuery;
    wxTextCtrl* mQuery;
    wxRadioButton* mByEmpty;
    wxRadioButton* mBySchema;
    wxTextCtrl* mSchema;
    wxButton* mSelSchema;
////@end WizardPage3 member variables
  int xml_init;
  tobjname table_name, table_alias;
  wxString query;
  d_table * new_td;
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
    wxComboBox* mCsvSepar;
    wxComboBox* mCsvDelim;
    wxComboBox* mDateFormat;
    wxComboBox* mTimeFormat;
    wxComboBox* mTsFormat;
    wxComboBox* mBoolFormat;
    wxComboBox* mDecSepar;
    wxCheckBox* mSemilog;
    wxCheckBox* mCRLines;
////@end WizardPage4 member variables
};

wxString encoding_name(int recode);

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
    wxChoice* mIDType;
////@end WizardPageXSD member variables
};

#endif
    // _NEWDATATRANSFERDLG_H_
