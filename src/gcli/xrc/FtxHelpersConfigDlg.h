/////////////////////////////////////////////////////////////////////////////
// Name:        FtxHelpersConfigDlg.h
// Purpose:     
// Author:      
// Modified by: 
// Created:     07/15/05 13:51:40
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifndef _FTXHELPERSCONFIGDLG_H_
#define _FTXHELPERSCONFIGDLG_H_

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma interface "FtxHelpersConfigDlg.cpp"
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
class wxOwnerDrawnComboBox;
////@end forward declarations

/*!
 * Control identifiers
 */

////@begin control identifiers
#define ID_DIALOG 10000
#define SYMBOL_FTXHELPERSCONFIGDLG_STYLE wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU|wxCLOSE_BOX
#define SYMBOL_FTXHELPERSCONFIGDLG_TITLE _("Configure usage of fulltext helpers")
#define SYMBOL_FTXHELPERSCONFIGDLG_IDNAME ID_DIALOG
#define SYMBOL_FTXHELPERSCONFIGDLG_SIZE wxSize(400, 300)
#define SYMBOL_FTXHELPERSCONFIGDLG_POSITION wxDefaultPosition
#define ID_CHOICE4 10010
#define ID_CHOICE5 10011
#define ID_CHOICE6 10012
#define ID_CHOICE7 10013
#define ID_OOO_STANDALONE_RADIOBUTTON 10001
#define ID_OOO_BINARY_TEXTCTRL 10002
#define ID_OOO_BINARY_BROWSE_BUTTON 10003
#define ID_OOO_SERVER_RADIOBUTTON 10004
#define ID_OOO_HOSTNAME_TEXTCTRL 10005
#define ID_OOO_PORT_TEXTCTRL 10006
#define ID_TEXTCTRL 10007
#define ID_BUTTON 10008
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
 * FtxHelpersConfigDlg class declaration
 */

class FtxHelpersConfigDlg: public wxDialog
{    
    DECLARE_DYNAMIC_CLASS( FtxHelpersConfigDlg )
    DECLARE_EVENT_TABLE()

    /* loads parameters of ftx. helpers into dialog controls
       returns true on success, false on error */
    bool LoadParams();
    /* saves parameters of ftx. helpers info config file
     * returns true on success, false on error */
    bool SaveParams();
    /* Tests if OOo macro ftx602.xba is installed when the user wants to use OOo for document conversion.
       Returns true on success (OOo isn't used or macro is installed), false on error. */
    bool Checkftx602macro();
    /* Tests if script.xlb file contains XML tag <library:element library:name="ftx602"/>
       Returns true on success, false on error. */
    bool Checkscriptxlbfile(const wxString &filename);
    /* Test if OOo is selected in some document format convertors combo box.
       Returns true if OOo is selected, false otherwise. */
    bool IsOOoSelected();
#ifdef WINS
    /* Loads tc value from regdb value whose name is regdb_valname.
     * Returns true on success, false on error. */
    bool LoadwxTextCtrlValueFromRegDB(HKEY hKey,wxTextCtrl *tc,const char *regdb_valname);
#endif
public:
    /// Constructors
    FtxHelpersConfigDlg( );
    FtxHelpersConfigDlg( wxWindow* parent, wxWindowID id = SYMBOL_FTXHELPERSCONFIGDLG_IDNAME, const wxString& caption = SYMBOL_FTXHELPERSCONFIGDLG_TITLE, const wxPoint& pos = SYMBOL_FTXHELPERSCONFIGDLG_POSITION, const wxSize& size = SYMBOL_FTXHELPERSCONFIGDLG_SIZE, long style = SYMBOL_FTXHELPERSCONFIGDLG_STYLE );

    /// Creation
    bool Create( wxWindow* parent, wxWindowID id = SYMBOL_FTXHELPERSCONFIGDLG_IDNAME, const wxString& caption = SYMBOL_FTXHELPERSCONFIGDLG_TITLE, const wxPoint& pos = SYMBOL_FTXHELPERSCONFIGDLG_POSITION, const wxSize& size = SYMBOL_FTXHELPERSCONFIGDLG_SIZE, long style = SYMBOL_FTXHELPERSCONFIGDLG_STYLE );

    /// Creates the controls and sizers
    void CreateControls();

////@begin FtxHelpersConfigDlg event handler declarations

    /// wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_OOO_STANDALONE_RADIOBUTTON
    void OnOooStandaloneRadiobuttonSelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_OOO_BINARY_BROWSE_BUTTON
    void OnOooBinaryBrowseButtonClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_OOO_SERVER_RADIOBUTTON
    void OnOooServerRadiobuttonSelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON
    void OnBrowseOOOProgramDirButtonClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_OK
    void OnOkClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_HELP
    void OnHelpClick( wxCommandEvent& event );

////@end FtxHelpersConfigDlg event handler declarations

////@begin FtxHelpersConfigDlg member function declarations

    /// Retrieves bitmap resources
    wxBitmap GetBitmapResource( const wxString& name );

    /// Retrieves icon resources
    wxIcon GetIconResource( const wxString& name );
////@end FtxHelpersConfigDlg member function declarations

    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin FtxHelpersConfigDlg member variables
    wxOwnerDrawnComboBox* mDOC;
    wxOwnerDrawnComboBox* mXLS;
    wxOwnerDrawnComboBox* mPPT;
    wxOwnerDrawnComboBox* mRTF;
    wxRadioButton* mOOOStandalone;
    wxTextCtrl* mSoffice;
    wxButton* mBrowseSoffice;
    wxRadioButton* mOOOServer;
    wxTextCtrl* mHostname;
    wxTextCtrl* mPort;
    wxTextCtrl* mOOOProgramDir;
    wxButton* mBrowseOOOProgramDir;
////@end FtxHelpersConfigDlg member variables
};

#endif
    // _FTXHELPERSCONFIGDLG_H_
