/////////////////////////////////////////////////////////////////////////////
// Name:        LicenceWizardDlg.h
// Purpose:     
// Author:      
// Modified by: 
// Created:     10/04/04 15:29:27
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifndef _LICENCEWIZARDDLG_H_
#define _LICENCEWIZARDDLG_H_

#if defined(__GNUG__) && !defined(__APPLE__)
//#pragma interface "LicenceWizardDlg.cpp"
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
class LicWizardPage1;
class LicWizardPage2;
////@end forward declarations

/*!
 * Control identifiers
 */

////@begin control identifiers
#define ID_WIZARD 10000
#define SYMBOL_LICENCEWIZARDDLG_IDNAME ID_WIZARD
#define ID_WIZARDPAGE 10001
#define CD_LICWIZ_BUY_DN 10003
#define CD_LICWIZ_DEVEL 10004
#define ID_WIZARDPAGE1 10002
#define CD_LICWIZ_REGISTER 10005
#define CD_LICWIZ_LN 10006
////@end control identifiers

/*!
 * Compatibility
 */

#ifndef wxCLOSE_BOX
#define wxCLOSE_BOX 0x1000
#endif

/*!
 * LicenceWizardDlg class declaration
 */

class LicenceWizardDlg: public wxWizard
{    
    DECLARE_CLASS( LicenceWizardDlg )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    LicenceWizardDlg(cdp_t cdpIn);

    /// Creation
    bool Create( wxWindow* parent, wxWindowID id = ID_WIZARD, const wxPoint& pos = wxDefaultPosition );

    /// Creates the controls and sizers
    void CreateControls();

////@begin LicenceWizardDlg event handler declarations

    /// wxEVT_WIZARD_HELP event handler for ID_WIZARD
    void OnWizardHelp( wxWizardEvent& event );

////@end LicenceWizardDlg event handler declarations

////@begin LicenceWizardDlg member function declarations

    /// Runs the wizard.
    bool Run();

    /// Retrieves bitmap resources
    wxBitmap GetBitmapResource( const wxString& name );

    /// Retrieves icon resources
    wxIcon GetIconResource( const wxString& name );
////@end LicenceWizardDlg member function declarations

    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin LicenceWizardDlg member variables
    LicWizardPage1* mPage1;
    LicWizardPage2* mPage2;
////@end LicenceWizardDlg member variables
  cdp_t cdp;
  char ik[MAX_LICENCE_LENGTH+1];
};

/*!
 * LicWizardPage1 class declaration
 */

class LicWizardPage1: public wxWizardPageSimple
{    
    DECLARE_DYNAMIC_CLASS( LicWizardPage1 )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    LicWizardPage1( );

    LicWizardPage1( wxWizard* parent );

    /// Creation
    bool Create( wxWizard* parent );

    /// Creates the controls and sizers
    void CreateControls();

////@begin LicWizardPage1 event handler declarations

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_LICWIZ_BUY_DN
    void OnCdLicwizBuyDnClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_LICWIZ_DEVEL
    void OnCdLicwizDevelClick( wxCommandEvent& event );

////@end LicWizardPage1 event handler declarations

////@begin LicWizardPage1 member function declarations

    /// Retrieves bitmap resources
    wxBitmap GetBitmapResource( const wxString& name );

    /// Retrieves icon resources
    wxIcon GetIconResource( const wxString& name );
////@end LicWizardPage1 member function declarations

    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin LicWizardPage1 member variables
    wxButton* mBuy;
    wxButton* mDevel;
////@end LicWizardPage1 member variables
};

/*!
 * LicWizardPage2 class declaration
 */

class LicWizardPage2: public wxWizardPageSimple
{    
    DECLARE_DYNAMIC_CLASS( LicWizardPage2 )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    LicWizardPage2( );

    LicWizardPage2( wxWizard* parent );

    /// Creation
    bool Create( wxWizard* parent );

    /// Creates the controls and sizers
    void CreateControls();

////@begin LicWizardPage11 event handler declarations

    /// wxEVT_WIZARD_PAGE_CHANGING event handler for ID_WIZARDPAGE1
    void OnWizardpage1PageChanging( wxWizardEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_LICWIZ_REGISTER
    void OnCdLicwizRegisterClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_UPDATED event handler for CD_LICWIZ_LN
    void OnCdLicwizLnUpdated( wxCommandEvent& event );

////@end LicWizardPage11 event handler declarations

////@begin LicWizardPage11 member function declarations

////@end LicWizardPage11 member function declarations

    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin LicWizardPage11 member variables
    wxButton* mRegister;
    wxTextCtrl* mLicNum;
////@end LicWizardPage11 member variables
};

#endif
    // _LICENCEWIZARDDLG_H_
