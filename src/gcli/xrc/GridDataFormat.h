/////////////////////////////////////////////////////////////////////////////
// Name:        GridDataFormat.h
// Purpose:     
// Author:      
// Modified by: 
// Created:     06/16/04 08:52:35
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifndef _GRIDDATAFORMAT_H_
#define _GRIDDATAFORMAT_H_

#if defined(__GNUG__) && !defined(__APPLE__)
//#pragma interface "GridDataFormat.cpp"
#endif

/*!
 * Includes
 */
#include "wx/odcombo.h"
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
#define SYMBOL_GRIDDATAFORMAT_STYLE wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU|wxCLOSE_BOX
#define SYMBOL_GRIDDATAFORMAT_TITLE _("Default Format of Data in Grids")
#define SYMBOL_GRIDDATAFORMAT_IDNAME ID_DIALOG
#define SYMBOL_GRIDDATAFORMAT_SIZE wxSize(400, 300)
#define SYMBOL_GRIDDATAFORMAT_POSITION wxDefaultPosition
#define CD_FORMAT_C1 10001
#define CD_FORMAT_DECSEP 10006
#define CD_FORMAT_C2 10008
#define CD_FORMAT_REAL 10166
#define CD_FORMAT_C3 10009
#define CD_FORMAT_DIGITS 10002
#define CD_FORMAT_C4 10010
#define CD_FORMAT_DATE 10003
#define CD_FORMAT_C5 10011
#define CD_FORMAT_TIME 10004
#define CD_FORMAT_C6 10012
#define CD_FORMAT_TIMESTAMP 10005
////@end control identifiers

/*!
 * Compatibility
 */

#ifndef wxCLOSE_BOX
#define wxCLOSE_BOX 0x1000
#endif

/*!
 * GridDataFormat class declaration
 */

class GridDataFormat: public wxDialog
{    
    DECLARE_CLASS( GridDataFormat )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    GridDataFormat( );
    GridDataFormat( wxWindow* parent, wxWindowID id = SYMBOL_GRIDDATAFORMAT_IDNAME, const wxString& caption = SYMBOL_GRIDDATAFORMAT_TITLE, const wxPoint& pos = SYMBOL_GRIDDATAFORMAT_POSITION, const wxSize& size = SYMBOL_GRIDDATAFORMAT_SIZE, long style = SYMBOL_GRIDDATAFORMAT_STYLE );

    /// Creation
    bool Create( wxWindow* parent, wxWindowID id = SYMBOL_GRIDDATAFORMAT_IDNAME, const wxString& caption = SYMBOL_GRIDDATAFORMAT_TITLE, const wxPoint& pos = SYMBOL_GRIDDATAFORMAT_POSITION, const wxSize& size = SYMBOL_GRIDDATAFORMAT_SIZE, long style = SYMBOL_GRIDDATAFORMAT_STYLE );

    /// Creates the controls and sizers
    void CreateControls();

////@begin GridDataFormat event handler declarations

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_OK
    void OnOkClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CANCEL
    void OnCancelClick( wxCommandEvent& event );

////@end GridDataFormat event handler declarations

////@begin GridDataFormat member function declarations

    /// Retrieves bitmap resources
    wxBitmap GetBitmapResource( const wxString& name );

    /// Retrieves icon resources
    wxIcon GetIconResource( const wxString& name );
////@end GridDataFormat member function declarations

    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin GridDataFormat member variables
    wxOwnerDrawnComboBox* mDecSep;
    wxOwnerDrawnComboBox* mReal;
    wxTextCtrl* mDigits;
    wxOwnerDrawnComboBox* mDate;
    wxOwnerDrawnComboBox* mTime;
    wxOwnerDrawnComboBox* mTimestamp;
////@end GridDataFormat member variables
};

#endif
    // _GRIDDATAFORMAT_H_
