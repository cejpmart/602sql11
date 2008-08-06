/////////////////////////////////////////////////////////////////////////////
// Name:        ObjectPropertiesDlg.h
// Purpose:     
// Author:      
// Modified by: 
// Created:     04/20/04 09:07:54
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifndef _OBJECTPROPERTIESDLG_H_
#define _OBJECTPROPERTIESDLG_H_

#ifdef __GNUG__
//#pragma interface "ObjectPropertiesDlg.cpp"
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
#define SYMBOL_OBJECTPROPERTIESDLG_STYLE wxFULL_REPAINT_ON_RESIZE  // wxFULL_REPAINT_ON_RESIZE needed for FL panes since WX 2.6.0
#define SYMBOL_OBJECTPROPERTIESDLG_TITLE _("Object Properties")
#define SYMBOL_OBJECTPROPERTIESDLG_IDNAME ID_DIALOG
#define SYMBOL_OBJECTPROPERTIESDLG_SIZE wxSize(400, 300)
#define SYMBOL_OBJECTPROPERTIESDLG_POSITION wxDefaultPosition
#define CD_OBJPROP_CAPT 10004
#define CD_OBJPROP_EXPORT 10001
#define CD_OBJPROP_EXPDATA 10002
#define CD_OBJPROP_PROTECT 10003
////@end control identifiers

/*!
 * Compatibility
 */

#ifndef wxCLOSE_BOX
#define wxCLOSE_BOX 0x1000
#endif

/*!
 * ObjectPropertiesDlg class declaration
 */

class ObjectPropertiesDlg: public wxScrolledWindow
{    
    DECLARE_CLASS( ObjectPropertiesDlg )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    ObjectPropertiesDlg( );

    /// Creation
    bool Create( wxWindow* parent, wxWindowID id = -1, const wxString& caption = _("Object Properties"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = SYMBOL_OBJECTPROPERTIESDLG_STYLE);

    /// Creates the controls and sizers
    void CreateControls();

////@begin ObjectPropertiesDlg event handler declarations

    /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for CD_OBJPROP_EXPORT
    void OnCdObjpropExportClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for CD_OBJPROP_EXPDATA
    void OnCdObjpropExpdataClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for CD_OBJPROP_PROTECT
    void OnCdObjpropProtectClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_HELP
    void OnHelpClick( wxCommandEvent& event );

////@end ObjectPropertiesDlg event handler declarations

////@begin ObjectPropertiesDlg member function declarations

////@end ObjectPropertiesDlg member function declarations

    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin ObjectPropertiesDlg member variables
    wxStaticText* mCapt;
    wxCheckBox* mExport;
    wxCheckBox* mExpData;
    wxCheckBox* mProtect;
////@end ObjectPropertiesDlg member variables
};

#endif
    // _OBJECTPROPERTIESDLG_H_
