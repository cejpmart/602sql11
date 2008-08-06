/////////////////////////////////////////////////////////////////////////////
// Name:        RestoringFileDlg.h
// Purpose:     
// Author:      
// Modified by: 
// Created:     06/18/04 09:34:39
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifndef _RESTORINGFILEDLG_H_
#define _RESTORINGFILEDLG_H_

#if defined(__GNUG__) && !defined(__APPLE__)
//#pragma interface "RestoringFileDlg.cpp"
#endif

/*!
 * Includes
 */

////@begin includes
#include "wx/valgen.h"
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
#define SYMBOL_RESTORINGFILEDLG_STYLE wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU|wxCLOSE_BOX
#define SYMBOL_RESTORINGFILEDLG_TITLE _("Restoring the Database File")
#define SYMBOL_RESTORINGFILEDLG_IDNAME ID_DIALOG
#define SYMBOL_RESTORINGFILEDLG_SIZE wxSize(400, 300)
#define SYMBOL_RESTORINGFILEDLG_POSITION wxDefaultPosition
#define ID_RADIOBOX 10001
////@end control identifiers

/*!
 * Compatibility
 */

#ifndef wxCLOSE_BOX
#define wxCLOSE_BOX 0x1000
#endif

/*!
 * RestoringFileDlg class declaration
 */

class RestoringFileDlg: public wxDialog
{    
    DECLARE_CLASS( RestoringFileDlg )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    RestoringFileDlg( );
    RestoringFileDlg( wxWindow* parent, wxWindowID id = SYMBOL_RESTORINGFILEDLG_IDNAME, const wxString& caption = SYMBOL_RESTORINGFILEDLG_TITLE, const wxPoint& pos = SYMBOL_RESTORINGFILEDLG_POSITION, const wxSize& size = SYMBOL_RESTORINGFILEDLG_SIZE, long style = SYMBOL_RESTORINGFILEDLG_STYLE );

    /// Creation
    bool Create( wxWindow* parent, wxWindowID id = SYMBOL_RESTORINGFILEDLG_IDNAME, const wxString& caption = SYMBOL_RESTORINGFILEDLG_TITLE, const wxPoint& pos = SYMBOL_RESTORINGFILEDLG_POSITION, const wxSize& size = SYMBOL_RESTORINGFILEDLG_SIZE, long style = SYMBOL_RESTORINGFILEDLG_STYLE );

    /// Creates the controls and sizers
    void CreateControls();

////@begin RestoringFileDlg event handler declarations

////@end RestoringFileDlg event handler declarations

////@begin RestoringFileDlg member function declarations

    int GetMode() const { return mode ; }
    void SetMode(int value) { mode = value ; }

    /// Retrieves bitmap resources
    wxBitmap GetBitmapResource( const wxString& name );

    /// Retrieves icon resources
    wxIcon GetIconResource( const wxString& name );
////@end RestoringFileDlg member function declarations

    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin RestoringFileDlg member variables
    int mode;
////@end RestoringFileDlg member variables
};

#endif
    // _RESTORINGFILEDLG_H_
