/////////////////////////////////////////////////////////////////////////////
// Name:        XMLXSDDlg.h
// Purpose:     
// Author:      
// Modified by: 
// Created:     05/09/05 14:33:08
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifndef _XMLXSDDLG_H_
#define _XMLXSDDLG_H_

#if defined(__GNUG__) && !defined(__APPLE__)
//#pragma interface "XMLXSDDlg.cpp"
#endif

/*!
 * Includes
 */

////@begin includes
#include "wx/spinctrl.h"
////@end includes

/*!
 * Forward declarations
 */

////@begin forward declarations
class wxSpinCtrl;
class wxOwnerDrawnComboBox;
////@end forward declarations

/*!
 * Control identifiers
 */

////@begin control identifiers
#define ID_DIALOG 10000
#define SYMBOL_XMLXSDDLG_STYLE wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU|wxCLOSE_BOX
#define SYMBOL_XMLXSDDLG_TITLE _("Generating DAD from XSD")
#define SYMBOL_XMLXSDDLG_IDNAME ID_DIALOG
#define SYMBOL_XMLXSDDLG_SIZE wxSize(400, 300)
#define SYMBOL_XMLXSDDLG_POSITION wxDefaultPosition
#define CD_XMLXSD_PREFIX 10001
#define CD_XMLXSD_LENGTH 10003
#define CD_XMLXSD_UNICODE 10002
#define CD_XMLXSD_IDTYPE 10169
////@end control identifiers

/*!
 * Compatibility
 */

#ifndef wxCLOSE_BOX
#define wxCLOSE_BOX 0x1000
#endif

/*!
 * XMLXSDDlg class declaration
 */

class XMLXSDDlg: public wxDialog
{    
    DECLARE_CLASS( XMLXSDDlg )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    XMLXSDDlg(t_xsd_link * xsdIn);

    /// Creation
    bool Create( wxWindow* parent, wxWindowID id = SYMBOL_XMLXSDDLG_IDNAME, const wxString& caption = SYMBOL_XMLXSDDLG_TITLE, const wxPoint& pos = SYMBOL_XMLXSDDLG_POSITION, const wxSize& size = SYMBOL_XMLXSDDLG_SIZE, long style = SYMBOL_XMLXSDDLG_STYLE );

    /// Creates the controls and sizers
    void CreateControls();

////@begin XMLXSDDlg event handler declarations

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_OK
    void OnOkClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CANCEL
    void OnCancelClick( wxCommandEvent& event );

////@end XMLXSDDlg event handler declarations

////@begin XMLXSDDlg member function declarations

    /// Retrieves bitmap resources
    wxBitmap GetBitmapResource( const wxString& name );

    /// Retrieves icon resources
    wxIcon GetIconResource( const wxString& name );
////@end XMLXSDDlg member function declarations

    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin XMLXSDDlg member variables
    wxTextCtrl* mPrefix;
    wxSpinCtrl* mLength;
    wxCheckBox* mUnicode;
    wxOwnerDrawnComboBox* mIDType;
////@end XMLXSDDlg member variables
    t_xsd_link * xsd;
};

#endif
    // _XMLXSDDLG_H_
