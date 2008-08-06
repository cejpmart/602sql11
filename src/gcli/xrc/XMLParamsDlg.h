/////////////////////////////////////////////////////////////////////////////
// Name:        XMLParamsDlg.h
// Purpose:     
// Author:      
// Modified by: 
// Created:     03/31/04 11:06:44
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifndef _XMLPARAMSDLG_H_
#define _XMLPARAMSDLG_H_

#ifdef __GNUG__
#pragma interface "XMLParamsDlg.cpp"
#endif

/*!
 * Includes
 */

////@begin includes
#include "wx/grid.h"
////@end includes

/*!
 * Forward declarations
 */

////@begin forward declarations
class wxBoxSizer;
class wxGrid;
////@end forward declarations

/*!
 * Control identifiers
 */

////@begin control identifiers
#define ID_DIALOG 10000
#define SYMBOL_XMLPARAMSDLG_STYLE wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU|wxCLOSE_BOX
#define SYMBOL_XMLPARAMSDLG_TITLE _("XML import/export parameters")
#define SYMBOL_XMLPARAMSDLG_IDNAME ID_DIALOG
#define SYMBOL_XMLPARAMSDLG_SIZE wxSize(500, 300)
#define SYMBOL_XMLPARAMSDLG_POSITION wxDefaultPosition
#define CD_XML_PARAM_GRID 10001
////@end control identifiers

/*!
 * Compatibility
 */

#ifndef wxCLOSE_BOX
#define wxCLOSE_BOX 0x1000
#endif

/*!
 * XMLParamsDlg class declaration
 */

class xml_designer;
class CQueryDesigner;

class XMLParamsDlg: public wxDialog
{    
    DECLARE_CLASS( XMLParamsDlg )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    XMLParamsDlg(xml_designer * xmldsgnIn);
    XMLParamsDlg(CQueryDesigner * querydsgn);
    XMLParamsDlg(t_dad_param_dynar * paramsIn, wxCSConv * convIn, bool * changedIn);
    ~XMLParamsDlg(void);

    /// Creation
    bool Create( wxWindow* parent, wxWindowID id = -1, const wxString& caption = SYMBOL_XMLPARAMSDLG_TITLE, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = SYMBOL_XMLPARAMSDLG_STYLE );

    /// Creates the controls and sizers
    void CreateControls();

////@begin XMLParamsDlg event handler declarations

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_OK
    void OnOkClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CANCEL
    void OnCancelClick( wxCommandEvent& event );

////@end XMLParamsDlg event handler declarations
    void OnClose(wxCloseEvent & event);

////@begin XMLParamsDlg member function declarations

    /// Retrieves bitmap resources
    wxBitmap GetBitmapResource( const wxString& name );

    /// Retrieves icon resources
    wxIcon GetIconResource( const wxString& name );
////@end XMLParamsDlg member function declarations

    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin XMLParamsDlg member variables
    wxBoxSizer* mSizer;
    wxGrid* mGrid;
    wxBoxSizer* mButtons;
    wxButton* mOK;
    wxButton* mCancel;
////@end XMLParamsDlg member variables
  t_dad_param_dynar * params;
  wxCSConv * sys_conv;
  bool * changed;

  void convert_types_for_edit(void);
  void closing(void);
};

#endif
    // _XMLPARAMSDLG_H_
