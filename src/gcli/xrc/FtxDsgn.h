/////////////////////////////////////////////////////////////////////////////
// Name:        FtxDsgn.h
// Purpose:     
// Author:      
// Modified by: 
// Created:     02/07/05 09:59:19
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifndef _FTXDSGN_H_
#define _FTXDSGN_H_

#if defined(__GNUG__) && !defined(__APPLE__)
//#pragma interface "FtxDsgn.cpp"
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
#define SYMBOL_FTXDSGN_STYLE wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU|wxCLOSE_BOX
#define SYMBOL_FTXDSGN_TITLE _("Fulltext Designer")
#define SYMBOL_FTXDSGN_IDNAME ID_DIALOG
#define SYMBOL_FTXDSGN_SIZE wxSize(400, 300)
#define SYMBOL_FTXDSGN_POSITION wxDefaultPosition
#define CD_FTD_LANG 10005
#define CD_FTD_LEMMA 10006
#define CD_FTD_SUBSTRUCT 10072
#define CD_FTD_EXTERNAL 10075
#define CD_FTD_GRID 10004
#define CD_FTD_LIMITS 10090
#define CD_FTD_REINDEX 10003
////@end control identifiers

/*!
 * Compatibility
 */

#ifndef wxCLOSE_BOX
#define wxCLOSE_BOX 0x1000
#endif

/*!
 * FtxDsgn class declaration
 */

class FtxDsgn: public wxPanel
{    
    DECLARE_CLASS( FtxDsgn )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    FtxDsgn(class fulltext_designer * ftdsgnIn);

    /// Creation
    bool Create( wxWindow* parent, wxWindowID id = SYMBOL_FTXDSGN_IDNAME, const wxString& caption = SYMBOL_FTXDSGN_TITLE, const wxPoint& pos = SYMBOL_FTXDSGN_POSITION, const wxSize& size = SYMBOL_FTXDSGN_SIZE, long style = SYMBOL_FTXDSGN_STYLE );

    /// Creates the controls and sizers
    void CreateControls();

////@begin FtxDsgn event handler declarations

    /// wxEVT_COMMAND_TEXT_UPDATED event handler for CD_FTD_LIMITS
    void OnCdFtdLimitsUpdated( wxCommandEvent& event );

////@end FtxDsgn event handler declarations

////@begin FtxDsgn member function declarations

    /// Retrieves bitmap resources
    wxBitmap GetBitmapResource( const wxString& name );

    /// Retrieves icon resources
    wxIcon GetIconResource( const wxString& name );
////@end FtxDsgn member function declarations

    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin FtxDsgn member variables
    wxStaticText* mLang;
    wxStaticText* mLemma;
    wxStaticText* mSubstruct;
    wxStaticText* mExternal;
    wxGrid* mGrid;
    wxTextCtrl* mLimits;
    wxButton* mReindex;
////@end FtxDsgn member variables
   class fulltext_designer * ftdsgn;
};

#endif
    // _FTXDSGN_H_
