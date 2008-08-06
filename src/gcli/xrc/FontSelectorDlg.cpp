/////////////////////////////////////////////////////////////////////////////
// Name:        FontSelectorDlg.cpp
// Purpose:     
// Author:      
// Modified by: 
// Created:     06/03/04 16:49:53
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#if defined(__GNUG__) && !defined(__APPLE__)
//#pragma implementation "FontSelectorDlg.h"
#endif

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

////@begin includes
////@end includes

#include "FontSelectorDlg.h"
#include "cptree.h" 
#include "cplist.h" 
////@begin XPM images
////@end XPM images

struct t_font_ref
{ wxChar * font_name;  // untranslated
  wxFont ** global_ptr;
  t_font_ref(wxChar * font_nameIn, wxFont ** global_ptrIn)
    { font_name=font_nameIn;  global_ptr=global_ptrIn; }
};

t_font_ref font_ref[FONT_CNT] = {
 t_font_ref(  wxT("Editor text"),    &text_editor_font ),
 t_font_ref(  wxT("Grid label"),     &grid_label_font ),
 t_font_ref(  wxT("Grid cell text"), &grid_cell_font ),
 t_font_ref(  wxT("Control panel"),  &control_panel_font )
};  
/*!
 * FontSelectorDlg type definition
 */

IMPLEMENT_CLASS( FontSelectorDlg, wxDialog )

/*!
 * FontSelectorDlg event table definition
 */

BEGIN_EVENT_TABLE( FontSelectorDlg, wxDialog )

////@begin FontSelectorDlg event table entries
    EVT_GRID_CELL_LEFT_DCLICK( FontSelectorDlg::OnCdFontGridLeftDClick )

    EVT_BUTTON( wxID_OK, FontSelectorDlg::OnOkClick )

    EVT_BUTTON( wxID_CANCEL, FontSelectorDlg::OnCancelClick )

////@end FontSelectorDlg event table entries

END_EVENT_TABLE()

/*!
 * FontSelectorDlg constructors
 */

FontSelectorDlg::FontSelectorDlg( )
{
}

FontSelectorDlg::FontSelectorDlg( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
    Create(parent, id, caption, pos, size, style);
}

/*!
 * FontSelectorDlg creator
 */

bool FontSelectorDlg::Create( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
////@begin FontSelectorDlg member initialisation
    mSizer = NULL;
    mGrid = NULL;
////@end FontSelectorDlg member initialisation

////@begin FontSelectorDlg creation
    SetExtraStyle(wxWS_EX_BLOCK_EVENTS);
    wxDialog::Create( parent, id, caption, pos, size, style );

    CreateControls();
    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);
    Centre();
////@end FontSelectorDlg creation
    return TRUE;
}

/*!
 * Control creation for FontSelectorDlg
 */

void FontSelectorDlg::CreateControls()
{    
////@begin FontSelectorDlg content construction

    FontSelectorDlg* item1 = this;

    wxBoxSizer* item2 = new wxBoxSizer(wxVERTICAL);
    mSizer = item2;
    item1->SetSizer(item2);
    item1->SetAutoLayout(TRUE);

    wxStaticText* item3 = new wxStaticText;
    item3->Create( item1, wxID_STATIC, _("Double-click the font to edit"), wxDefaultPosition, wxDefaultSize, 0 );
    item2->Add(item3, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT|wxTOP|wxADJUST_MINSIZE, 5);

    wxGrid* item4 = new wxGrid;
    item4->Create( item1, CD_FONT_GRID, wxDefaultPosition, wxSize(330, 150), wxSUNKEN_BORDER );
    mGrid = item4;
    item4->SetDefaultColSize(150);
    item4->SetDefaultRowSize(25);
    item4->SetColLabelSize(20);
    item4->SetRowLabelSize(150);
    item4->CreateGrid(2, 1, wxGrid::wxGridSelectCells);
    item2->Add(item4, 1, wxGROW|wxALL, 5);

    wxBoxSizer* item5 = new wxBoxSizer(wxHORIZONTAL);
    item2->Add(item5, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 0);

    wxButton* item6 = new wxButton;
    item6->Create( item1, wxID_OK, _("OK"), wxDefaultPosition, wxDefaultSize, 0 );
    item6->SetDefault();
    item5->Add(item6, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxButton* item7 = new wxButton;
    item7->Create( item1, wxID_CANCEL, _("Cancel"), wxDefaultPosition, wxDefaultSize, 0 );
    item5->Add(item7, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

////@end FontSelectorDlg content construction
  mGrid->AppendRows(FONT_CNT-2);  // 2 rows on creation
  mGrid->SetColLabelValue(0, _("Example"));
  int i;
  for (i=0;  i<FONT_CNT;  i++)
    mGrid->SetRowLabelValue(i, wxGetTranslation(font_ref[i].font_name));
  wxFont hLabFont = mGrid->GetLabelFont();  // using the system default label font (only here)
  wxClientDC dc(mGrid);
  dc.SetFont(hLabFont);
  wxCoord w, h, wx=0;
  dc.GetTextExtent(_("Grid label"), &w, &h);
  if (w>wx) wx=w;
  dc.GetTextExtent(_("Grid cell text"), &w, &h);
  if (w>wx) wx=w;
  mGrid->SetRowLabelSize(wx+16);
  mGrid->EnableEditing(false);
  for (i=0;  i<FONT_CNT;  i++)
  { font_list[i].existing = *font_ref[i].global_ptr!=NULL;
    if (font_list[i].existing)
    { font_list[i].font = **font_ref[i].global_ptr;  // copy of the font
      mGrid->SetCellValue(i, 0, wxT("AaBbZz"));
      mGrid->SetCellFont(i, 0, font_list[i].font);
    }
  }  
  mGrid->SetColSize(0, wx);
  mSizer->SetItemMinSize(mGrid, 16+2*wx+16, 160);
}

/*!
 * Should we show tooltips?
 */

bool FontSelectorDlg::ShowToolTips()
{
    return TRUE;
}
/*!
 * wxEVT_GRID_CELL_LEFT_DCLICK event handler for CD_FONT_GRID
 */

#include <wx/fontdlg.h>

#ifndef WINS
bool MyIsFixedFont(wxWindow * wnd, wxFont font)
{
  wxClientDC dc(wnd);
  dc.SetFont(font);
  wxCoord w1, w2, h;
  dc.GetTextExtent(wxT("i"), &w1, &h);
  dc.GetTextExtent(wxT("W"), &w2, &h);
  return w1==w2;
}

#endif

void FontSelectorDlg::OnCdFontGridLeftDClick( wxGridEvent& event )
{ int ind = event.GetRow();
  wxFontData data;
  data.EnableEffects(false);  // disables colour, strikeout and underline on MSW
  //data.SetAllowSymbols(false);  // disables symbol fonts on MSW  -- no, it disables Script selection!!!
  if (font_list[ind].existing) 
    data.SetInitialFont(font_list[ind].font);
  //data.SetColour(canvasTextColour);

  wxFontDialog dialog(this, data);
  if (dialog.ShowModal() == wxID_OK)
  {
    wxFontData retData = dialog.GetFontData();
    if (ind==0)
#ifdef WINS  // IsFixedWidth checks for TELETYPE font family but most fixed fonts return the UNKNOWN family on GTK
      if (!retData.GetChosenFont().IsFixedWidth())
#else
      if (!MyIsFixedFont(this, retData.GetChosenFont()))
#endif
      { error_box(_("The Text Editor requires a fixed width font. The selected font does not have a fixed width."), this);
        return;
      }
    font_list[ind].font = retData.GetChosenFont();
    font_list[ind].existing = true;
    mGrid->SetCellFont(ind, 0, font_list[ind].font);
    mGrid->SetCellValue(ind, 0, wxT("AaBbZz"));  // for fonts which were not existing before
    //canvasTextColour = retData.GetColour();
    mGrid->Refresh();
  }
}


/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CANCEL
 */

void FontSelectorDlg::OnCancelClick( wxCommandEvent& event )
{
  event.Skip();
}


/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_OK
 */

void FontSelectorDlg::OnOkClick( wxCommandEvent& event )
{
  for (int i=0;  i<FONT_CNT;  i++)
    if (font_list[i].existing)
    { delete *font_ref[i].global_ptr;  
      *font_ref[i].global_ptr = new wxFont(font_list[i].font);
    }  
 // update CP:
  if (control_panel_font) 
  { wxGetApp().frame->control_panel->objlist->SetFont(*control_panel_font);   
    wxGetApp().frame->control_panel->tree_view->SetFont(*control_panel_font);   
  }  
  event.Skip();
}


