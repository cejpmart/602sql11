/////////////////////////////////////////////////////////////////////////////
// Name:        ServerCertificateDlg.cpp
// Purpose:     
// Author:      
// Modified by: 
// Created:     05/03/04 17:45:42
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#if defined(__GNUG__) && !defined(__APPLE__)
//#pragma implementation "ServerCertificateDlg.h"
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

#include "ServerCertificateDlg.h"

////@begin XPM images
////@end XPM images

/*!
 * ServerCertificateDlg type definition
 */

IMPLEMENT_CLASS( ServerCertificateDlg, wxDialog )

/*!
 * ServerCertificateDlg event table definition
 */

BEGIN_EVENT_TABLE( ServerCertificateDlg, wxDialog )

////@begin ServerCertificateDlg event table entries
    EVT_BUTTON( CD_CERT_START, ServerCertificateDlg::OnCdCertStartClick )

    EVT_BUTTON( CD_CERT_CANCEL, ServerCertificateDlg::OnCdCertCancelClick )

    EVT_BUTTON( CD_CERT_IMPCERT, ServerCertificateDlg::OnCdCertImpcertClick )

    EVT_BUTTON( CD_CERT_IMPKEY, ServerCertificateDlg::OnCdCertImpkeyClick )

////@end ServerCertificateDlg event table entries
END_EVENT_TABLE()

/*!
 * ServerCertificateDlg constructors
 */

ServerCertificateDlg::ServerCertificateDlg(cdp_t cdpIn)
{ cdp=cdpIn; }

/*!
 * ServerCertificateDlg creator
 */

bool ServerCertificateDlg::Create( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
////@begin ServerCertificateDlg member initialisation
    mState = NULL;
    mStart = NULL;
    mCancel = NULL;
    mField = NULL;
    mGauge = NULL;
    mImpCert = NULL;
    mImpKey = NULL;
////@end ServerCertificateDlg member initialisation

////@begin ServerCertificateDlg creation
    SetExtraStyle(GetExtraStyle()|wxWS_EX_BLOCK_EVENTS);
    wxDialog::Create( parent, id, caption, pos, size, style );

    CreateControls();
    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);
    Centre();
////@end ServerCertificateDlg creation
    return TRUE;
}

/*!
 * Control creation for ServerCertificateDlg
 */

void ServerCertificateDlg::CreateControls()
{    
////@begin ServerCertificateDlg content construction
    ServerCertificateDlg* itemDialog1 = this;

    wxBoxSizer* itemBoxSizer2 = new wxBoxSizer(wxVERTICAL);
    itemDialog1->SetSizer(itemBoxSizer2);

    mState = new wxStaticText;
    mState->Create( itemDialog1, CD_CERT_STATE, _("The server already has a certificate. If you create a new certificate, clients will be required to remove their local copy\nof the certificate before they can establish a new connection."), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer2->Add(mState, 0, wxALIGN_LEFT|wxALL|wxADJUST_MINSIZE, 5);

    wxStaticText* itemStaticText4 = new wxStaticText;
    itemStaticText4->Create( itemDialog1, wxID_STATIC, _("The new certicate can be obtained in one of two ways:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer2->Add(itemStaticText4, 0, wxALIGN_LEFT|wxALL|wxADJUST_MINSIZE, 5);

    wxNotebook* itemNotebook5 = new wxNotebook;
    itemNotebook5->Create( itemDialog1, ID_NOTEBOOK, wxDefaultPosition, wxDefaultSize, wxNB_TOP );
#if !wxCHECK_VERSION(2,5,2)
    wxNotebookSizer* itemNotebook5Sizer = new wxNotebookSizer(itemNotebook5);
#endif

    wxPanel* itemPanel6 = new wxPanel;
    itemPanel6->Create( itemNotebook5, ID_PANEL, wxDefaultPosition, wxSize(100, 80), wxSUNKEN_BORDER|wxTAB_TRAVERSAL );
    wxBoxSizer* itemBoxSizer7 = new wxBoxSizer(wxVERTICAL);
    itemPanel6->SetSizer(itemBoxSizer7);

    wxStaticText* itemStaticText8 = new wxStaticText;
    itemStaticText8->Create( itemPanel6, wxID_STATIC, _("Click the Start button and move the mouse over the rectangle until the gauge at the bottom fills."), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer7->Add(itemStaticText8, 0, wxALIGN_LEFT|wxALL|wxADJUST_MINSIZE, 5);

    wxBoxSizer* itemBoxSizer9 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer7->Add(itemBoxSizer9, 0, wxGROW|wxALL, 0);
    wxBoxSizer* itemBoxSizer10 = new wxBoxSizer(wxVERTICAL);
    itemBoxSizer9->Add(itemBoxSizer10, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);
    mStart = new wxButton;
    mStart->Create( itemPanel6, CD_CERT_START, _("Start"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer10->Add(mStart, 0, wxALIGN_LEFT|wxALL, 5);

    mCancel = new wxButton;
    mCancel->Create( itemPanel6, CD_CERT_CANCEL, _("Cancel"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer10->Add(mCancel, 0, wxALIGN_LEFT|wxALL, 5);

    mField = new wxTextCtrl;
    mField->Create( itemPanel6, CD_CERT_FIELD, _T(""), wxDefaultPosition, wxSize(-1, 80), wxTE_READONLY );
    mField->SetBackgroundColour(wxColour(255, 0, 0));
    itemBoxSizer9->Add(mField, 1, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    mGauge = new wxGauge;
    mGauge->Create( itemPanel6, CD_CERT_GAUGE, 100, wxDefaultPosition, wxDefaultSize, wxGA_HORIZONTAL|wxGA_PROGRESSBAR|wxGA_SMOOTH );
    mGauge->SetValue(1);
    itemBoxSizer7->Add(mGauge, 0, wxGROW|wxALL, 5);

    itemNotebook5->AddPage(itemPanel6, _("Create a self-signed certificate"));

    wxPanel* itemPanel15 = new wxPanel;
    itemPanel15->Create( itemNotebook5, ID_PANEL1, wxDefaultPosition, wxSize(100, 80), wxSUNKEN_BORDER|wxTAB_TRAVERSAL );
    wxFlexGridSizer* itemFlexGridSizer16 = new wxFlexGridSizer(3, 2, 0, 0);
    itemPanel15->SetSizer(itemFlexGridSizer16);

    wxStaticText* itemStaticText17 = new wxStaticText;
    itemStaticText17->Create( itemPanel15, wxID_STATIC, _("1."), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer16->Add(itemStaticText17, 0, wxALIGN_LEFT|wxALIGN_TOP|wxALL|wxADJUST_MINSIZE, 5);

    wxStaticText* itemStaticText18 = new wxStaticText;
    itemStaticText18->Create( itemPanel15, wxID_STATIC, _("Create the certificate in the PEM format and an unencryped private key\nin the DER format using an external program like OpenSSL."), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer16->Add(itemStaticText18, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    wxStaticText* itemStaticText19 = new wxStaticText;
    itemStaticText19->Create( itemPanel15, wxID_STATIC, _("2."), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer16->Add(itemStaticText19, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    mImpCert = new wxButton;
    mImpCert->Create( itemPanel15, CD_CERT_IMPCERT, _("Import the certificate from a file"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer16->Add(mImpCert, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxStaticText* itemStaticText21 = new wxStaticText;
    itemStaticText21->Create( itemPanel15, wxID_STATIC, _("3."), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer16->Add(itemStaticText21, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    mImpKey = new wxButton;
    mImpKey->Create( itemPanel15, CD_CERT_IMPKEY, _("Import the private key from a file"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer16->Add(mImpKey, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL, 5);

    itemNotebook5->AddPage(itemPanel15, _("Import the certificate and secret key from files"));

#if !wxCHECK_VERSION(2,5,2)
    itemBoxSizer2->Add(itemNotebook5Sizer, 0, wxGROW|wxALL, 5);
#else
    itemBoxSizer2->Add(itemNotebook5, 0, wxGROW|wxALL, 5);
#endif

    wxButton* itemButton23 = new wxButton;
    itemButton23->Create( itemDialog1, wxID_OK, _("Close"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer2->Add(itemButton23, 0, wxALIGN_CENTER_HORIZONTAL|wxLEFT|wxRIGHT|wxBOTTOM, 5);

////@end ServerCertificateDlg content construction
    accumulating=false;
    mCancel->Disable();
    state_info();
}

void ServerCertificateDlg::state_info(void)
{ uns32 length;
  if (cd_Read_len(cdp, SRV_TABLENUM, 0, SRV_ATR_PUBKEY, NOINDEX, &length)) length=0;
  bool has_cert=length>0;
  char buf[10];  int len;
  if (cd_Get_property_value(cdp, sqlserver_owner, "ServerKey", 0, buf, sizeof(buf), &len))
    len=0;
  bool has_key=len>0;
  mState->SetLabel(
    has_cert && has_key ?
      _("The server already has a certificate. If you create a new certificate, clients will be required to remove their local copy\nof the certificate before they can establish a new connection.") :
    !has_cert ? 
      _("The server does not have a certificate.\n ") :
      _("The server has a certificate, but it cannot be used since\nthe private key has not been imported yet."));
  mImpKey->Enable(has_cert && !has_key);
 // make space for the new text:
  GetSizer()->Fit(this);
  //GetSizer()->SetSizeHints(this);
}

/*!
 * Should we show tooltips?
 */

bool ServerCertificateDlg::ShowToolTips()
{
    return TRUE;
}
/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_CERT_IMPCERT
 */
#include "wbsecur.h"

void ServerCertificateDlg::OnCdCertImpcertClick( wxCommandEvent& event )
{
  wxString fname = wxFileSelector(_("Choose a certificate file"), wxEmptyString, wxEmptyString, wxEmptyString, wxT("*.*"), wxOPEN | wxFILE_MUST_EXIST, this);
  if (fname.IsEmpty()) return;  // cancelled
  FHANDLE hnd = CreateFileA(fname.mb_str(*wxConvCurrent), GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
  if (hnd == INVALID_FHANDLE_VALUE)
    { error_box(_("Cannot open the file"), this);  return; }
  unsigned size=SetFilePointer(hnd, 0, NULL, FILE_END);
  SetFilePointer(hnd, 0, NULL, FILE_BEGIN);
  unsigned char * certbuf = (unsigned char *)sigalloc(size+1, 48);
  if (!certbuf)
    { CloseFile(hnd);  return; }
  DWORD rd;
  ReadFile(hnd, certbuf, size, &rd, NULL);
  CloseFile(hnd);
  t_Certificate * cert = make_certificate(certbuf, rd);
  if (cert)
  { DeleteCert(cert);  
    if (cd_Write_var(cdp, SRV_TABLENUM, 0, SRV_ATR_PUBKEY, NOINDEX, 0, rd, certbuf))
      { cd_Signalize2(cdp, this);  return; }
    cd_Write_len(cdp, SRV_TABLENUM, 0, SRV_ATR_PUBKEY, NOINDEX, rd);
  }
  else 
    { error_box(_("File does not contain a valid certificate."), this);  return; }
  corefree(certbuf);
  state_info();
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_CERT_IMPKEY
 */

static const char pem_alphabet[64] = {
'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', 
'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', 
'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '+', '/' };

bool pem2der(const unsigned char * ebuf, uns32 elength, char * buf, uns32 * length)
{ bool error=false;
  *length = 0;
 // skip the 1st line of input:
  while (elength && *ebuf>=' ') { ebuf++;  elength--; }
 // decode:
  int padd=0;
  uns32 res = 0;  int chars_in_res=0;
  while (elength)
  {// read from input: 
    char c = *ebuf;  ebuf++;  elength--;
    if (c >= ' ')  // otherwise ignored
      if (c == '-') 
      { if (chars_in_res) error=true;  // incomplete input
        goto inp_end;  // normal end
      }
      else if (c=='=')
        { res <<= 6;  padd++;  chars_in_res++; }
      else
      { const char * p = strchr(pem_alphabet, c);
        if (p==NULL) { error=true;  goto inp_end; }
        res = (res<<6) + (p-pem_alphabet);
        chars_in_res++;
      }
    if (chars_in_res==4)
    {               *buf = ((char*)&res)[2];  buf++;  (*length)++;
      if (padd<2) { *buf = ((char*)&res)[1];  buf++;  (*length)++; }
      if (padd<1) { *buf = ((char*)&res)[0];  buf++;  (*length)++; }
      res=0;  chars_in_res=0;  padd=0;
    }
  }
 inp_end:
  return error;
}

void ServerCertificateDlg::OnCdCertImpkeyClick( wxCommandEvent& event )
{
  wxString fname = wxFileSelector(_("Choose a private key file"), wxEmptyString, wxEmptyString, wxEmptyString, wxT("*.*"), wxOPEN | wxFILE_MUST_EXIST, this);
  if (fname.IsEmpty()) return;  // cancelled
  FHANDLE hnd = CreateFileA(fname.mb_str(*wxConvCurrent), GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
  if (hnd == INVALID_FHANDLE_VALUE)
    { error_box(_("Cannot open the file"), this);  return; }
  char buf[1000]; unsigned char ebuf[1000];  uns32 length;  DWORD elength;
  ReadFile(hnd, ebuf, sizeof(ebuf), &elength, NULL);
  CloseFile(hnd);  
  if (ebuf[0]=='-'  && ebuf[1]=='-')  // PEM encoded key
    pem2der(ebuf, elength, buf, &length);
  else if (ebuf[0]==0x30  && ebuf[1]==0x82 && ebuf[4]==0x2)  // DER encoded key
    { memcpy(buf, ebuf, elength);  length=elength; }
  else
    { error_box(_("File does not contain a valid key or it is encrypted."), this);  return; }
//  else if (!Decrypt_private_key(ebuf, elength, password, buf, &length))
//    { resource_message_box(ctip->ce->hInst, hDlg, CAPT_ERROR, CANNOT_DECRYPT_FILE, MB_ICONSTOP);  break; }
  if (cd_Set_property_value(cdp, sqlserver_owner, "ServerKey", 0, buf, length))
  { if (cd_Sz_error(cdp)==INTERNAL_SIGNAL)
      error_box(_("The private key does not correspond to the certificate."), this);
    else cd_Signalize2(cdp, this);  
    return; 
  }
  state_info();
}

class MouseMoveHandler : public wxEvtHandler
{ ServerCertificateDlg * dlg;
 public:
  MouseMoveHandler(ServerCertificateDlg * dlgIn)
    { dlg=dlgIn; }
  void OnMouseMove(wxMouseEvent & event);
  DECLARE_EVENT_TABLE()
};

void ServerCertificateDlg::make_key_pair(void)
{ wxBusyCursor wait;
 // compress random data:
  unsigned char * cert_str;  unsigned cert_len;  unsigned char * privkey_str;  unsigned privkey_len;
  int i = 0;
  while (i+200 < RANDOM_DATA_SPACE)
  { for (int j=0;  j<200;  j++) random_data[j] ^= random_data[i+j];
    i+=200;
  }
  if (!create_self_certified_keys((char*)random_data, &cert_str, &cert_len, &privkey_str, &privkey_len))
    { wxBell();  return; }
 // save the certificate:
  if (cd_Write_var(cdp, SRV_TABLENUM, 0, SRV_ATR_PUBKEY, NOINDEX, 0, cert_len, cert_str))
    { cd_Signalize(cdp);  return; }
  cd_Write_len(cdp, SRV_TABLENUM, 0, SRV_ATR_PUBKEY, NOINDEX, cert_len);
 // save the private key:
  if (cd_Set_property_value(cdp, sqlserver_owner, "ServerKey", 0, (char*)privkey_str, privkey_len))
    cd_Signalize(cdp);  
  free_unschar_arr(cert_str);
  free_unschar_arr(privkey_str);
  state_info();
}

void MouseMoveHandler::OnMouseMove(wxMouseEvent & event)
{ if (!dlg->accumulating) return;
  if (dlg->accumulated<RANDOM_DATA_SPACE) 
  { dlg->random_data[dlg->accumulated++]=(event.GetX()+37*event.GetY()) % 256; 
    dlg->mGauge->SetValue(100*dlg->accumulated/RANDOM_DATA_SPACE);
  }
  else  // stop it
  { dlg->mCancel->Disable();
    dlg->accumulating=false;
    info_box(_("Finished gathering random data. The certificate will be generated now."), dlg);
    dlg->make_key_pair();
    info_box(_("The certificate and private key has been created."), dlg);
    dlg->mField->PopEventHandler(true);  // deletes this handler!!!
  }
}

BEGIN_EVENT_TABLE( MouseMoveHandler, wxEvtHandler )
  EVT_MOTION(MouseMoveHandler::OnMouseMove)
END_EVENT_TABLE()

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_CERT_START
 */

void ServerCertificateDlg::OnCdCertStartClick( wxCommandEvent& event )
{
  accumulated=0;
  mCancel->Enable();
  mStart->Disable();
  accumulating=true;
  mGauge->SetValue(0);
  MouseMoveHandler * handler = new MouseMoveHandler(this);
  mField->PushEventHandler(handler);
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for CD_CERT_CANCEL
 */

void ServerCertificateDlg::OnCdCertCancelClick( wxCommandEvent& event )
{ if (!accumulating) return;
  mCancel->Disable();
  mStart->Enable();
  accumulating=false;
  mGauge->SetValue(0);
  mField->PopEventHandler(true);  // deletes the handler
}



/*!
 * Get bitmap resources
 */

wxBitmap ServerCertificateDlg::GetBitmapResource( const wxString& name )
{
    // Bitmap retrieval
////@begin ServerCertificateDlg bitmap retrieval
    return wxNullBitmap;
////@end ServerCertificateDlg bitmap retrieval
}

/*!
 * Get icon resources
 */

wxIcon ServerCertificateDlg::GetIconResource( const wxString& name )
{
    // Icon retrieval
////@begin ServerCertificateDlg icon retrieval
    return wxNullIcon;
////@end ServerCertificateDlg icon retrieval
}
