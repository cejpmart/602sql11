/////////////////////////////////////////////////////////////////////////////
// Name:        XMLRunDlg.cpp
// Purpose:     
// Author:      
// Modified by: 
// Created:     04/07/04 15:43:47
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifdef __GNUG__
//#pragma implementation "XMLRunDlg.h"
#endif

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

////@begin includes
#include "wx/wx.h"
////@end includes

#include "XMLRunDlg.h"

////@begin XPM images
////@end XPM images

/*!
 * XMLRunDlg type definition
 */

IMPLEMENT_CLASS( XMLRunDlg, wxDialog )

/*!
 * XMLRunDlg event table definition
 */

BEGIN_EVENT_TABLE( XMLRunDlg, wxDialog )

////@begin XMLRunDlg event table entries
    EVT_BUTTON( CD_XMLRUN_FILESEL, XMLRunDlg::OnCdXmlrunFileselClick )

    EVT_BUTTON( wxID_OK, XMLRunDlg::OnOkClick )

    EVT_BUTTON( wxID_CANCEL, XMLRunDlg::OnCancelClick )

////@end XMLRunDlg event table entries

END_EVENT_TABLE()

/*!
 * XMLRunDlg constructors
 */

XMLRunDlg::XMLRunDlg(item_info & infoIn) : info(infoIn)
{  }

/*!
 * XMLRunDlg creator
 */

bool XMLRunDlg::Create( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
////@begin XMLRunDlg member initialisation
////@end XMLRunDlg member initialisation

////@begin XMLRunDlg creation
    SetExtraStyle(wxWS_EX_BLOCK_EVENTS);
    wxDialog::Create( parent, id, caption, pos, size, style );

    CreateControls();
    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);
    Centre();
////@end XMLRunDlg creation
    return TRUE;
}

/*!
 * Control creation for XMLRunDlg
 */

void XMLRunDlg::CreateControls()
{    
////@begin XMLRunDlg content construction

    XMLRunDlg* item1 = this;

    wxBoxSizer* item2 = new wxBoxSizer(wxVERTICAL);
    item1->SetSizer(item2);
    item1->SetAutoLayout(TRUE);

    wxString item3Strings[] = {
        _("&Export data to an XML file"),
        _("&Import data from an XML file")
    };
    wxRadioBox* item3 = new wxRadioBox;
    item3->Create( item1, CD_XMLRUN_DIRECTION, _("Select direction"), wxDefaultPosition, wxDefaultSize, 2, item3Strings, 1, wxRA_SPECIFY_COLS );
    mDirection = item3;
    item2->Add(item3, 0, wxGROW|wxALL, 5);

    wxBoxSizer* item4 = new wxBoxSizer(wxHORIZONTAL);
    item2->Add(item4, 0, wxGROW|wxALL, 0);

    wxStaticText* item5 = new wxStaticText;
    item5->Create( item1, wxID_STATIC, _("&XML file name:"), wxDefaultPosition, wxDefaultSize, 0 );
    item4->Add(item5, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    wxTextCtrl* item6 = new wxTextCtrl;
    item6->Create( item1, CD_XMLRUN_FILE, _T(""), wxDefaultPosition, wxSize(200, -1), 0 );
    mFileName = item6;
    item4->Add(item6, 1, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxButton* item7 = new wxButton;
    item7->Create( item1, CD_XMLRUN_FILESEL, _("..."), wxDefaultPosition, wxSize(20, -1), 0 );
    item4->Add(item7, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxTOP|wxBOTTOM, 5);

    wxBoxSizer* item8 = new wxBoxSizer(wxHORIZONTAL);
    item2->Add(item8, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 0);

    wxButton* item9 = new wxButton;
    item9->Create( item1, wxID_OK, _("OK"), wxDefaultPosition, wxDefaultSize, 0 );
    item9->SetDefault();
    item8->Add(item9, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxButton* item10 = new wxButton;
    item10->Create( item1, wxID_CANCEL, _("Cancel"), wxDefaultPosition, wxDefaultSize, 0 );
    item8->Add(item10, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

////@end XMLRunDlg content construction
}

/*!
 * Should we show tooltips?
 */

bool XMLRunDlg::ShowToolTips()
{
    return TRUE;
}
/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_XMLRUN_FILESEL
 */

void XMLRunDlg::OnCdXmlrunFileselClick( wxCommandEvent& event )
{ bool exporting = mDirection->GetSelection()==0;
  wxString str = exporting ?
    GetExportFileName(mFileName->GetValue().c_str(), get_xml_filter(), this) :
    GetImportFileName(mFileName->GetValue().c_str(), get_xml_filter(), this);
  if (!str.IsEmpty())
    mFileName->SetValue(str);
}


/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_OK
 */

void XMLRunDlg::OnOkClick( wxCommandEvent& event )
{ BOOL res;  char dad_ref[1+OBJ_NAME_LEN+1];
  bool exporting = mDirection->GetSelection()==0;
  *dad_ref='*';  strcpy(dad_ref+1, info.object_name);
  wxString fname = mFileName->GetValue();
  fname.Trim(false);  fname.Trim(true);
  if (fname.IsEmpty()) 
    { wxBell();  mFileName->SetFocus();  return; }
  { wxBusyCursor wait;
  if (exporting)
    { if (!can_overwrite_file(fname.mb_str(*wxConvCurrent), NULL)) return; /* file exists, overwrite not confirmed */
      res=link_Export_to_XML(info.cdp, dad_ref, fname.mb_str(*wxConvCurrent), NOCURSOR, NULL, 0);
    }
    else
      res=link_Import_from_XML(info.cdp, dad_ref, fname.mb_str(*wxConvCurrent), NULL, 0);
  }
  if (!res) { cd_Signalize(info.cdp);  return; }
  event.Skip();
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CANCEL
 */

void XMLRunDlg::OnCancelClick( wxCommandEvent& event )
{
    event.Skip();
}


