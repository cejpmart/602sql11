/////////////////////////////////////////////////////////////////////////////
// Name:        SubjectRelationshipDlg.h
// Purpose:     
// Author:      
// Modified by: 
// Created:     02/16/04 13:04:04
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifndef _SUBJECTRELATIONSHIPDLG_H_
#define _SUBJECTRELATIONSHIPDLG_H_

#ifdef __GNUG__
//#pragma interface "SubjectRelationshipDlg.cpp"
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
#define SYMBOL_SUBJECTRELATIONSHIPDLG_STYLE wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU|wxCLOSE_BOX
#define SYMBOL_SUBJECTRELATIONSHIPDLG_TITLE _("Relationship among subjects")
#define SYMBOL_SUBJECTRELATIONSHIPDLG_IDNAME ID_DIALOG
#define SYMBOL_SUBJECTRELATIONSHIPDLG_SIZE wxSize(400, 300)
#define SYMBOL_SUBJECTRELATIONSHIPDLG_POSITION wxDefaultPosition
#define CD_SUBJ_REL_CAPT 10002
#define CD_SUBJ_REL_LIST 10001
////@end control identifiers

/*!
 * Compatibility
 */

#ifndef wxCLOSE_BOX
#define wxCLOSE_BOX 0x1000
#endif

/*!
 * SubjectRelationshipDlg class declaration
 */

class SubjectRelationshipDlg: public wxDialog
{    
    DECLARE_CLASS( SubjectRelationshipDlg )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    SubjectRelationshipDlg(cdp_t cdpIn, tcateg subject1In, tobjnum subjnumIn, tcateg subject2In) : 
      cdp(cdpIn), subject1(subject1In), subjnum(subjnumIn), subject2(subject2In) 
        { }

    /// Creation
    bool Create( wxWindow* parent, wxWindowID id = -1, const wxString& caption = _("Relationship among subjects"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU|wxCLOSE_BOX );

    /// Creates the controls and sizers
    void CreateControls();

////@begin SubjectRelationshipDlg event handler declarations

////@end SubjectRelationshipDlg event handler declarations

////@begin SubjectRelationshipDlg member function declarations

////@end SubjectRelationshipDlg member function declarations

    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin SubjectRelationshipDlg member variables
////@end SubjectRelationshipDlg member variables
  cdp_t cdp;
  tcateg subject1;
  tobjnum subjnum;
  tcateg subject2;
  bool rel1in2;
  bool TransferDataToWindow(void);
  bool TransferDataFromWindow(void);
};

#endif
    // _SUBJECTRELATIONSHIPDLG_H_
