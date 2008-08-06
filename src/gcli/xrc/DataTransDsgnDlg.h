/////////////////////////////////////////////////////////////////////////////
// Name:        DataTransDsgnDlg.h
// Purpose:     
// Author:      
// Modified by: 
// Created:     04/05/04 11:13:01
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifndef _DATATRANSDSGNDLG_H_
#define _DATATRANSDSGNDLG_H_

#ifdef __GNUG__
//#pragma interface "DataTransDsgnDlg.cpp"
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
class wxGrid;
////@end forward declarations

/*!
 * Control identifiers
 */

////@begin control identifiers
#define ID_DIALOG 10000
#define SYMBOL_DATATRANSDSGNDLG_STYLE wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU|wxCLOSE_BOX
#define SYMBOL_DATATRANSDSGNDLG_TITLE _("Design of a Data Transfer")
#define SYMBOL_DATATRANSDSGNDLG_IDNAME ID_DIALOG
#define SYMBOL_DATATRANSDSGNDLG_SIZE wxDefaultSize
#define SYMBOL_DATATRANSDSGNDLG_POSITION wxDefaultPosition
#define CD_TR_FROM 10002
#define CD_TR_FROM_NAME 10001
#define CD_TR_BACK_FROM 10008
#define CD_TR_TO 10004
#define CD_TR_TO_NAME 10007
#define CD_TR_BACK_TO 10005
#define CD_TR_GRID 10003
////@end control identifiers

/*!
 * Compatibility
 */

#ifndef wxCLOSE_BOX
#define wxCLOSE_BOX 0x1000
#endif

/*!
 * DataTransDsgnDlg class declaration
 */
class DataTransDsgnDlg;

class DelRowHandler : public wxEvtHandler
{ DataTransDsgnDlg * dlg;
 public:
  DelRowHandler(DataTransDsgnDlg * dlgIn)
    { dlg=dlgIn; }
  void OnKeyDown(wxKeyEvent & event);
  void OnRangeSelect(wxGridRangeSelectEvent & event);
  DECLARE_EVENT_TABLE()
};

class DataTransDsgnDlg: public wxPanel, public any_designer
{    
    DECLARE_CLASS( DataTransDsgnDlg )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    DataTransDsgnDlg(cdp_t cdpIn, tobjnum objnumIn, const char * folder_nameIn, const char * schema_nameIn);
    ~DataTransDsgnDlg(void)
      { if (mGrid) mGrid->PopEventHandler(true); }  // deletes the handler

    /// Creation
    bool Create( wxWindow* parent, wxWindowID id = -1, const wxString& caption = _("Design of a data transfer"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU );

    /// Creates the controls and sizers
    void CreateControls();

////@begin DataTransDsgnDlg event handler declarations

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_TR_BACK_FROM
    void OnCdTrBackFromClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_TR_BACK_TO
    void OnCdTrBackToClick( wxCommandEvent& event );

////@end DataTransDsgnDlg event handler declarations

////@begin DataTransDsgnDlg member function declarations

    /// Retrieves bitmap resources
    wxBitmap GetBitmapResource( const wxString& name );

    /// Retrieves icon resources
    wxIcon GetIconResource( const wxString& name );
////@end DataTransDsgnDlg member function declarations
  void OnCdTrCreImplClick(void);
  void CreateSpecificGrid(void);
  void show_source_target_info(void);
  bool back_and_restart(int page);
    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin DataTransDsgnDlg member variables
    wxStaticText* mFromType;
    wxStaticText* mFromName;
    wxBitmapButton* mFromButton;
    wxStaticText* mToType;
    wxStaticText* mToName;
    wxBitmapButton* mToButton;
    wxGrid* mGrid;
////@end DataTransDsgnDlg member variables
  t_ie_run x;
  bool changed;
};

#endif
    // _DATATRANSDSGNDLG_H_
