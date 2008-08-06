/////////////////////////////////////////////////////////////////////////////
// Name:        AddOnLicencesDlg.h
// Purpose:     
// Author:      
// Modified by: 
// Created:     06/27/07 16:48:58
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifndef _ADDONLICENCESDLG_H_
#define _ADDONLICENCESDLG_H_

#if defined(__GNUG__) && !defined(__APPLE__)
//#pragma interface "AddOnLicencesDlg.cpp"
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
#define ID_MYDIALOG14 10162
#define SYMBOL_ADDONLICENCESDLG_STYLE wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU|wxCLOSE_BOX
#define SYMBOL_ADDONLICENCESDLG_TITLE _("Server Extensions Manager")
#define SYMBOL_ADDONLICENCESDLG_IDNAME ID_MYDIALOG14
#define SYMBOL_ADDONLICENCESDLG_SIZE wxSize(400, 300)
#define SYMBOL_ADDONLICENCESDLG_POSITION wxDefaultPosition
#define CD_LIC_LIST 10163
#define CD_LIC_BUY 10164
#define CD_LIC_LIC 10000
#define CD_LIC_SAVE 10001
#define CD_LIC_PREP 10165
#define CD_LIC_PROC 10002
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
 * AddOnLicencesDlg class declaration
 */

class AddOnLicencesDlg: public wxDialog
{    
    DECLARE_CLASS( AddOnLicencesDlg )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    AddOnLicencesDlg(cdp_t cdpIn);
    AddOnLicencesDlg( wxWindow* parent, wxWindowID id = SYMBOL_ADDONLICENCESDLG_IDNAME, const wxString& caption = SYMBOL_ADDONLICENCESDLG_TITLE, const wxPoint& pos = SYMBOL_ADDONLICENCESDLG_POSITION, const wxSize& size = SYMBOL_ADDONLICENCESDLG_SIZE, long style = SYMBOL_ADDONLICENCESDLG_STYLE );

    /// Creation
    bool Create( wxWindow* parent, wxWindowID id = SYMBOL_ADDONLICENCESDLG_IDNAME, const wxString& caption = SYMBOL_ADDONLICENCESDLG_TITLE, const wxPoint& pos = SYMBOL_ADDONLICENCESDLG_POSITION, const wxSize& size = SYMBOL_ADDONLICENCESDLG_SIZE, long style = SYMBOL_ADDONLICENCESDLG_STYLE );

    /// Creates the controls and sizers
    void CreateControls();

////@begin AddOnLicencesDlg event handler declarations

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_LIC_BUY
    void OnCdLicBuyClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_LIC_SAVE
    void OnCdLicSaveClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_LIC_PREP
    void OnCdLicPrepClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_LIC_PROC
    void OnCdLicProcClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_HELP
    void OnHelpClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CLOSE
    void OnCloseClick( wxCommandEvent& event );

////@end AddOnLicencesDlg event handler declarations

////@begin AddOnLicencesDlg member function declarations

    /// Retrieves bitmap resources
    wxBitmap GetBitmapResource( const wxString& name );

    /// Retrieves icon resources
    wxIcon GetIconResource( const wxString& name );
////@end AddOnLicencesDlg member function declarations

    /// Should we show tooltips?
    static bool ShowToolTips();
    void show_lics(void);
    bool activation(char * lic, char * company, char * computer);
    bool proc_lic_num(char lic8[25+1]);

////@begin AddOnLicencesDlg member variables
    wxStaticText* mExtCapt;
    wxListBox* mList;
    wxButton* mBuy;
    wxTextCtrl* mLic;
    wxButton* mSave;
    wxButton* mPrep;
    wxButton* mProc;
////@end AddOnLicencesDlg member variables
    cdp_t cdp;
};

#endif
    // _ADDONLICENCESDLG_H_
