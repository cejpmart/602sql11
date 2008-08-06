/////////////////////////////////////////////////////////////////////////////
// Name:        NewTriggerWiz.h
// Purpose:     
// Author:      
// Modified by: 
// Created:     02/17/04 09:26:01
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifndef _NEWTRIGGERWIZ_H_
#define _NEWTRIGGERWIZ_H_

#ifdef __GNUG__
//#pragma interface "NewTriggerWiz.cpp"
#endif

/*!
 * Includes
 */

////@begin includes
#include "wx/wizard.h"
////@end includes

/*!
 * Forward declarations
 */

////@begin forward declarations
class TriggerWizardPage1;
class TriggerWizardPage2;
class TriggerWizardPage3;
////@end forward declarations

/*!
 * Control identifiers
 */

////@begin control identifiers
#define ID_WIZARD 10000
#define TRIG_WIZ_PAGE1 10001
#define CD_TWZ1_NAME 10003
#define CD_TWZ1_TABLE 10004
#define TRIG_WIZ_PAGE2 10002
#define CD_TWZ2_OPER 10006
#define CD_TWZ2_EXPLIC 10007
#define CD_TWZ2_LIST 10008
#define TRIG_WIZ_PAGE3 10005
#define CD_TWZ3_TIME 10009
#define CD_TWZ3_STAT 10010
#define CD_TWZ3_OLD_CAPT 10013
#define CD_TWZ3_OLD 10011
#define CD_TWZ3_NEW_CAPT 10014
#define CD_TWZ3_NEW 10012
////@end control identifiers

/*!
 * Compatibility
 */

#ifndef wxCLOSE_BOX
#define wxCLOSE_BOX 0x1000
#endif

/*!
 * NewTriggerWiz class declaration
 */

enum t_trigger_oper { TO_INSERT, TO_DELETE, TO_UPDATE };

class NewTriggerWiz: public wxWizard
{    
    DECLARE_CLASS( NewTriggerWiz )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    NewTriggerWiz(cdp_t cdpIn, const char * tabnameIn );
    NewTriggerWiz( wxWindow* parent, wxWindowID id = -1, const wxPoint& pos = wxDefaultPosition );

    /// Creation
    bool Create( wxWindow* parent, wxWindowID id = -1, const wxPoint& pos = wxDefaultPosition );

    /// Creates the controls and sizers
    void CreateControls();

////@begin NewTriggerWiz event handler declarations

////@end NewTriggerWiz event handler declarations

////@begin NewTriggerWiz member function declarations

    /// Runs the wizard.
    bool Run();

    t_trigger_oper GetTriggerOper() const { return trigger_oper ; }
    void SetTriggerOper(t_trigger_oper value) { trigger_oper = value ; }

    bool GetExplicitAttrs() const { return explicit_attrs ; }
    void SetExplicitAttrs(bool value) { explicit_attrs = value ; }

    bool GetBefore() const { return before ; }
    void SetBefore(bool value) { before = value ; }

    bool GetStatementGranularity() const { return statement_granularity ; }
    void SetStatementGranularity(bool value) { statement_granularity = value ; }

    wxString GetTriggerName() const { return trigger_name ; }
    void SetTriggerName(wxString value) { trigger_name = value ; }

    wxString GetTriggerTable() const { return trigger_table ; }
    void SetTriggerTable(wxString value) { trigger_table = value ; }

    wxString GetNewname() const { return newname ; }
    void SetNewname(wxString value) { newname = value ; }

    wxString GetOldname() const { return oldname ; }
    void SetOldname(wxString value) { oldname = value ; }

////@end NewTriggerWiz member function declarations
    bool Run2();

    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin NewTriggerWiz member variables
    TriggerWizardPage1* mPage1;
    TriggerWizardPage2* mPage2;
    TriggerWizardPage3* mPage3;
    t_trigger_oper trigger_oper;
    bool explicit_attrs;
    bool before;
    bool statement_granularity;
    wxString trigger_name;
    wxString trigger_table;
    wxString newname;
    wxString oldname;
////@end NewTriggerWiz member variables
  cdp_t cdp;
  ttablenum tabobj;
  t_atrmap_flex attrmap;  // bitmap contains only user columns and non-multiattributes
};

/*!
 * TriggerWizardPage1 class declaration
 */

class TriggerWizardPage1: public wxWizardPageSimple
{    
    DECLARE_DYNAMIC_CLASS( TriggerWizardPage1 )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    TriggerWizardPage1( );

    TriggerWizardPage1( wxWizard* parent );

    /// Creation
    bool Create( wxWizard* parent );

    /// Creates the controls and sizers
    void CreateControls();

////@begin TriggerWizardPage1 event handler declarations

    /// wxEVT_WIZARD_PAGE_CHANGED event handler for TRIG_WIZ_PAGE1
    void OnTrigWizPage1PageChanged( wxWizardEvent& event );

    /// wxEVT_WIZARD_PAGE_CHANGING event handler for TRIG_WIZ_PAGE1
    void OnTrigWizPage1PageChanging( wxWizardEvent& event );

    /// wxEVT_WIZARD_HELP event handler for TRIG_WIZ_PAGE1
    void OnTrigWizPage1Help( wxWizardEvent& event );

    /// wxEVT_COMMAND_TEXT_UPDATED event handler for CD_TWZ1_NAME
    void OnCdTwz1NameUpdated( wxCommandEvent& event );

    /// wxEVT_COMMAND_LISTBOX_SELECTED event handler for CD_TWZ1_TABLE
    void OnCdTwz1TableSelected( wxCommandEvent& event );

////@end TriggerWizardPage1 event handler declarations

////@begin TriggerWizardPage1 member function declarations

////@end TriggerWizardPage1 member function declarations

    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin TriggerWizardPage1 member variables
    wxTextCtrl* mName;
    wxListBox* mTable;
////@end TriggerWizardPage1 member variables
};

/*!
 * TriggerWizardPage2 class declaration
 */

class TriggerWizardPage2: public wxWizardPageSimple
{    
    DECLARE_DYNAMIC_CLASS( TriggerWizardPage2 )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    TriggerWizardPage2( );

    TriggerWizardPage2( wxWizard* parent );

    /// Creation
    bool Create( wxWizard* parent );

    /// Creates the controls and sizers
    void CreateControls();

////@begin TriggerWizardPage2 event handler declarations

    /// wxEVT_WIZARD_PAGE_CHANGED event handler for TRIG_WIZ_PAGE2
    void OnTrigWizPage2PageChanged( wxWizardEvent& event );

    /// wxEVT_WIZARD_PAGE_CHANGING event handler for TRIG_WIZ_PAGE2
    void OnTrigWizPage2PageChanging( wxWizardEvent& event );

    /// wxEVT_WIZARD_HELP event handler for TRIG_WIZ_PAGE2
    void OnTrigWizPage2Help( wxWizardEvent& event );

    /// wxEVT_COMMAND_RADIOBOX_SELECTED event handler for CD_TWZ2_OPER
    void OnCdTwz2OperSelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for CD_TWZ2_EXPLIC
    void OnCdTwz2ExplicClick( wxCommandEvent& event );

////@end TriggerWizardPage2 event handler declarations

////@begin TriggerWizardPage2 member function declarations

////@end TriggerWizardPage2 member function declarations
    void enabling(void);

    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin TriggerWizardPage2 member variables
    wxRadioBox* mOper;
    wxCheckBox* mExplic;
    wxCheckListBox* mList;
////@end TriggerWizardPage2 member variables
};

/*!
 * TriggerWizardPage3 class declaration
 */

class TriggerWizardPage3: public wxWizardPageSimple
{    
    DECLARE_DYNAMIC_CLASS( TriggerWizardPage3 )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    TriggerWizardPage3( );

    TriggerWizardPage3( wxWizard* parent );

    /// Creation
    bool Create( wxWizard* parent );

    /// Creates the controls and sizers
    void CreateControls();

////@begin TriggerWizardPage3 event handler declarations

    /// wxEVT_WIZARD_PAGE_CHANGED event handler for TRIG_WIZ_PAGE3
    void OnTrigWizPage3PageChanged( wxWizardEvent& event );

    /// wxEVT_WIZARD_PAGE_CHANGING event handler for TRIG_WIZ_PAGE3
    void OnTrigWizPage3PageChanging( wxWizardEvent& event );

    /// wxEVT_WIZARD_HELP event handler for TRIG_WIZ_PAGE3
    void OnTrigWizPage3Help( wxWizardEvent& event );

    /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for CD_TWZ3_STAT
    void OnCdTwz3StatClick( wxCommandEvent& event );

////@end TriggerWizardPage3 event handler declarations

////@begin TriggerWizardPage3 member function declarations

////@end TriggerWizardPage3 member function declarations
    void enabling(void);

    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin TriggerWizardPage3 member variables
    wxRadioBox* mTime;
    wxCheckBox* mStat;
    wxStaticText* mOldCapt;
    wxTextCtrl* mOld;
    wxStaticText* mNewCapt;
    wxTextCtrl* mNew;
////@end TriggerWizardPage3 member variables
};

#endif
    // _NEWTRIGGERWIZ_H_
