/////////////////////////////////////////////////////////////////////////////
// Name:        NewRoutineDlg.h
// Purpose:     
// Author:      
// Modified by: 
// Created:     02/16/04 17:11:00
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifndef _NEWROUTINEDLG_H_
#define _NEWROUTINEDLG_H_

#ifdef __GNUG__
//#pragma interface "NewRoutineDlg.cpp"
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
#define SYMBOL_NEWROUTINEDLG_STYLE wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU|wxCLOSE_BOX
#define SYMBOL_NEWROUTINEDLG_TITLE _("Creating a new server object")
#define SYMBOL_NEWROUTINEDLG_IDNAME ID_DIALOG
#define SYMBOL_NEWROUTINEDLG_SIZE wxSize(400, 300)
#define SYMBOL_NEWROUTINEDLG_POSITION wxDefaultPosition
#define CD_CREPROC_TYPE 10001
#define CD_CREPROC_CAPT 10005
#define CD_CREPROC_NAME 10004
////@end control identifiers

/*!
 * Compatibility
 */

#ifndef wxCLOSE_BOX
#define wxCLOSE_BOX 0x1000
#endif

/*!
 * NewRoutineDlg class declaration
 */

class NewRoutineDlg: public wxDialog
{    
    DECLARE_CLASS( NewRoutineDlg )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    NewRoutineDlg(cdp_t cdpIn );
    NewRoutineDlg( wxWindow* parent, wxWindowID id = -1, const wxString& caption = SYMBOL_NEWROUTINEDLG_TITLE, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = SYMBOL_NEWROUTINEDLG_STYLE );

    /// Creation
    bool Create( wxWindow* parent, wxWindowID id = -1, const wxString& caption = SYMBOL_NEWROUTINEDLG_TITLE, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = SYMBOL_NEWROUTINEDLG_STYLE );

    /// Creates the controls and sizers
    void CreateControls();

////@begin NewRoutineDlg event handler declarations

    /// wxEVT_COMMAND_RADIOBOX_SELECTED event handler for CD_CREPROC_TYPE
    void OnCdCreprocTypeSelected( wxCommandEvent& event );

////@end NewRoutineDlg event handler declarations

////@begin NewRoutineDlg member function declarations

    int GetObjtype() const { return objtype ; }
    void SetObjtype(int value) { objtype = value ; }

////@end NewRoutineDlg member function declarations

    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin NewRoutineDlg member variables
    wxRadioBox* mType;
    wxStaticText* mNameCapt;
    wxTextCtrl* mName;
    wxButton* mOK;
    int objtype;
////@end NewRoutineDlg member variables
  cdp_t cdp;
  tobjname objname;
  bool TransferDataToWindow(void);
  bool TransferDataFromWindow(void);
  bool Validate(void);
};

#endif
    // _NEWROUTINEDLG_H_
