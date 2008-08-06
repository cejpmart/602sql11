/////////////////////////////////////////////////////////////////////////////
// Name:        XMLRunDlg.h
// Purpose:     
// Author:      
// Modified by: 
// Created:     04/07/04 15:43:48
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifndef _XMLRUNDLG_H_
#define _XMLRUNDLG_H_

#ifdef __GNUG__
//#pragma interface "XMLRunDlg.cpp"
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
#define CD_XMLRUN_DIRECTION 10001
#define CD_XMLRUN_FILE 10002
#define CD_XMLRUN_FILESEL 10003
////@end control identifiers

/*!
 * Compatibility
 */

#ifndef wxCLOSE_BOX
#define wxCLOSE_BOX 0x1000
#endif

/*!
 * XMLRunDlg class declaration
 */

class XMLRunDlg: public wxDialog
{    
    DECLARE_CLASS( XMLRunDlg )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    XMLRunDlg(item_info & infoIn);

    /// Creation
    bool Create( wxWindow* parent, wxWindowID id = -1, const wxString& caption = _("Start XML import/export"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU|wxCLOSE_BOX );

    /// Creates the controls and sizers
    void CreateControls();

////@begin XMLRunDlg event handler declarations

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_XMLRUN_FILESEL
    void OnCdXmlrunFileselClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_OK
    void OnOkClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CANCEL
    void OnCancelClick( wxCommandEvent& event );

////@end XMLRunDlg event handler declarations

////@begin XMLRunDlg member function declarations

////@end XMLRunDlg member function declarations

    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin XMLRunDlg member variables
    wxRadioBox* mDirection;
    wxTextCtrl* mFileName;
////@end XMLRunDlg member variables
  item_info & info;
};

#endif
    // _XMLRUNDLG_H_
