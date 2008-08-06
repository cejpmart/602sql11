/////////////////////////////////////////////////////////////////////////////
// Name:        RenameServerDlg.cpp
// Purpose:     
// Author:      
// Modified by: 
// Created:     09/10/04 16:33:43
// RCS-ID:      
// Copyright:   
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#if defined(__GNUG__) && !defined(__APPLE__)
//#pragma implementation "RenameServerDlg.h"
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

#include "RenameServerDlg.h"

////@begin XPM images
////@end XPM images

/*!
 * RenameServerDlg type definition
 */

IMPLEMENT_CLASS( RenameServerDlg, wxDialog )

/*!
 * RenameServerDlg event table definition
 */

BEGIN_EVENT_TABLE( RenameServerDlg, wxDialog )

////@begin RenameServerDlg event table entries
    EVT_BUTTON( wxID_OK, RenameServerDlg::OnOkClick )

    EVT_BUTTON( wxID_CANCEL, RenameServerDlg::OnCancelClick )

////@end RenameServerDlg event table entries

END_EVENT_TABLE()

/*!
 * RenameServerDlg constructors
 */

RenameServerDlg::RenameServerDlg(cdp_t cdpIn) : cdp(cdpIn)
{ }

/*!
 * RenameServerDlg creator
 */

bool RenameServerDlg::Create( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
////@begin RenameServerDlg member initialisation
    mName = NULL;
    mId = NULL;
    mLocal = NULL;
    mOK = NULL;
////@end RenameServerDlg member initialisation

////@begin RenameServerDlg creation
    SetExtraStyle(GetExtraStyle()|wxWS_EX_BLOCK_EVENTS);
    wxDialog::Create( parent, id, caption, pos, size, style );

    CreateControls();
    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);
    Centre();
////@end RenameServerDlg creation
    return TRUE;
}

/*!
 * Control creation for RenameServerDlg
 */

void RenameServerDlg::CreateControls()
{    
////@begin RenameServerDlg content construction
    RenameServerDlg* itemDialog1 = this;

    wxBoxSizer* itemBoxSizer2 = new wxBoxSizer(wxVERTICAL);
    itemDialog1->SetSizer(itemBoxSizer2);

    wxFlexGridSizer* itemFlexGridSizer3 = new wxFlexGridSizer(2, 2, 0, 0);
    itemFlexGridSizer3->AddGrowableCol(1);
    itemBoxSizer2->Add(itemFlexGridSizer3, 0, wxGROW|wxALL, 0);

    wxStaticText* itemStaticText4 = new wxStaticText;
    itemStaticText4->Create( itemDialog1, wxID_STATIC, _("Server &name:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer3->Add(itemStaticText4, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    mName = new wxTextCtrl;
    mName->Create( itemDialog1, CD_SRVRENAME_NAME, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer3->Add(mName, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxStaticText* itemStaticText6 = new wxStaticText;
    itemStaticText6->Create( itemDialog1, wxID_STATIC, _("Server &identity:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer3->Add(itemStaticText6, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    mId = new wxTextCtrl;
    mId->Create( itemDialog1, CD_SRVRENAME_ID, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer3->Add(mId, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxStaticText* itemStaticText8 = new wxStaticText;
    itemStaticText8->Create( itemDialog1, wxID_STATIC, _("WARNING: Changing the server identity will invalidate all passwords!"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer2->Add(itemStaticText8, 0, wxGROW|wxALL|wxADJUST_MINSIZE, 5);

    wxStaticText* itemStaticText9 = new wxStaticText;
    itemStaticText9->Create( itemDialog1, wxID_STATIC, _("The server must be restarted after the name or identity has changed."), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer2->Add(itemStaticText9, 0, wxGROW|wxALL|wxADJUST_MINSIZE, 5);

    mLocal = new wxCheckBox;
    mLocal->Create( itemDialog1, CD_SRVRENAME_LOCAL, _("Change the local registration of the server to its new name"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    mLocal->SetValue(FALSE);
    itemBoxSizer2->Add(mLocal, 0, wxGROW|wxALL, 5);

    wxBoxSizer* itemBoxSizer11 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer2->Add(itemBoxSizer11, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 0);

    mOK = new wxButton;
    mOK->Create( itemDialog1, wxID_OK, _("OK"), wxDefaultPosition, wxDefaultSize, 0 );
    mOK->SetDefault();
    itemBoxSizer11->Add(mOK, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxButton* itemButton13 = new wxButton;
    itemButton13->Create( itemDialog1, wxID_CANCEL, _("Cancel"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer11->Add(itemButton13, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

////@end RenameServerDlg content construction
    mName->SetMaxLength(OBJ_NAME_LEN);
    mId->SetMaxLength(2*UUID_SIZE);
   // show values:
    WBUUID uuid;  
    cd_Read(cdp, SRV_TABLENUM, 0, SRV_ATR_NAME, NULL, oldname);  // same as cdp->conn_server_name
    cd_Read(cdp, SRV_TABLENUM, 0, SRV_ATR_UUID, NULL, uuid);
    mName->SetValue(wxString(oldname, *wxConvCurrent));
    mId->SetValue(bin2hexWX(uuid, UUID_SIZE));
   // init the "local" checkbox:
    t_avail_server * as;  
    as = find_server_connection_info(cdp->locid_server_name);
    if (as && as->cdp)
    { if (as->notreg || as->local)
        mLocal->Disable();
      else
        mLocal->SetValue(true);
    }
   // enabling OK:
    if (!cd_Am_I_config_admin(cdp))
      mOK->Disable();
}

/*!
 * Should we show tooltips?
 */

bool RenameServerDlg::ShowToolTips()
{
    return TRUE;
}
/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_OK
 */

int hex2bin(wxChar c)
{ if (c>='0' && c<='9') return c-'0';
  if (c>='A' && c<='F') return c-'A'+10;
  if (c>='a' && c<='f') return c-'a'+10;
  return -1;
}

#ifdef WINS
BOOL RegRenameKey(char * key, char * old_name, char * new_name)
{ int len=(int)strlen(key);  HKEY hOldKey, hNewKey;
  strcat(key, old_name);
  LONG err=RegOpenKeyExA(HKEY_LOCAL_MACHINE, key, 0, KEY_READ_EX, &hOldKey);
  if (err != ERROR_SUCCESS) 
    return err==ERROR_FILE_NOT_FOUND || err==ERROR_PATH_NOT_FOUND;  // may have been changed by the server, if local
  BOOL res=FALSE;  DWORD Disposition;  int ind;
  strcpy(key+len, new_name);
  if (RegCreateKeyExA(HKEY_LOCAL_MACHINE, key, 0, "", REG_OPTION_NON_VOLATILE,
    KEY_ALL_ACCESS_EX, NULL, &hNewKey, &Disposition) != ERROR_SUCCESS) goto error1;
  ind=0;
  do
  { DWORD type, namesize, datasize;  char valname[50];  BYTE valdata[300];
    namesize=sizeof(valname);  datasize=sizeof(valdata);
    if (RegEnumValueA(hOldKey, ind++, valname, &namesize, NULL, &type,
      valdata, &datasize) != ERROR_SUCCESS) break;
    if (RegSetValueExA(hNewKey, valname, 0, type, valdata, datasize))
      goto error2;
  } while (TRUE);
  RegCloseKey(hNewKey);
  RegCloseKey(hOldKey);
  strcpy(key+len, old_name);
  RegDeleteKeyA(HKEY_LOCAL_MACHINE, key);
  return TRUE;

 error2:
  RegCloseKey(hNewKey);
  RegDeleteKeyA(HKEY_LOCAL_MACHINE, key);
 error1:
  RegCloseKey(hOldKey);
  return res;
}
#endif

void RenameServerDlg::OnOkClick( wxCommandEvent& event )
{ tobjname name;  WBUUID uuid;
  t_avail_server * as;  
  as = find_server_connection_info(cdp->locid_server_name);
  if (!as || !as->cdp) return;  // fuse
 // get data from the dialog:
  wxString id_buf = mId->GetValue();
  wxString name_buf = mName->GetValue();  name_buf.Trim(false);  name_buf.Trim(true);
  strcpy(name, name_buf.mb_str(*wxConvCurrent));
  int i;
  for (i=0;  i<UUID_SIZE;  i++)
  { int h=hex2bin(id_buf.GetChar(2*i));
    if (h<0) break;
    uuid[i]=h<<4;
    h=hex2bin(id_buf.GetChar(2*i+1));
    if (h<0) break;
    uuid[i]+=h;
  }
  if (i<UUID_SIZE)
    { mId->SetFocus();  wxBell();  return; }
 // converted, check for local name conflict:
  tobjname orig_name;
  cd_Read(cdp, SRV_TABLENUM, 0, SRV_ATR_NAME, NULL, orig_name);
  if (name_buf.CmpNoCase(wxString(orig_name, *wxConvCurrent).c_str()))  // changing the name
  {// new name must not be the same as a registered name unless it is the same as my own registration name: 
    if (IsServerRegistered(name) && stricmp(name, cdp->locid_server_name))
      { error_box(_("A server with this name is already registered"), this);  return; }
   // save new data on the server side:
    if (cd_Set_property_value(cdp, "@SQLSERVER", "ServerName", 0, name))  
      { cd_Signalize(cdp);  return; }
    strcpy(cdp->conn_server_name, name);  // update client data
   // save locally:
    if (mLocal->GetValue() || as->local)
    {
#ifdef WINS
      char key[160];  
      strcpy(key, WB_inst_key);  strcat(key, Database_str);  strcat(key, "\\");
      RegRenameKey(key, orig_name, name);
     // rename the server in the service definition, if any:
      char service_name[MAX_SERVICE_NAME+1];
      if (srv_Get_service_name(orig_name, service_name)) 
        WriteServerNameForService(wxString(service_name, *wxConvCurrent), wxString(name, *wxConvCurrent), false);
#else
      RenameSectionInPrivateProfile(orig_name, name, configfile);
#endif
      MyFrame * frame = wxGetApp().frame;
      if (frame->monitor_window) // remove the old name from the monitor list (and stop monitoring)
        frame->monitor_window->Disconnecting(cdp);
      strcpy(cdp->locid_server_name, name);  // update client data
      strcpy(as->locid_server_name, name);
      if (frame->monitor_window)  // re-add the server to the monitor list:
        frame->monitor_window->Connecting(cdp);
    }
    if (ControlPanel())
      ControlPanel()->RefreshPanel();
  }
 // save new UUID:
  cd_Set_property_value(cdp, "@SQLSERVER", "SrvUUID", 0, (const char*)uuid, UUID_SIZE);  // for new servers
  if (cd_Write(cdp, SRV_TABLENUM, 0, SRV_ATR_UUID, NULL, uuid, UUID_SIZE))      // for old servers
    cd_Signalize(cdp); 
  else  // update client data
    memcpy(cdp->server_uuid, uuid, UUID_SIZE);  // necessary to be able to log in just after importing users
  event.Skip();
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CANCEL
 */

void RenameServerDlg::OnCancelClick( wxCommandEvent& event )
{
    event.Skip();
}



/*!
 * Get bitmap resources
 */

wxBitmap RenameServerDlg::GetBitmapResource( const wxString& name )
{
    // Bitmap retrieval
////@begin RenameServerDlg bitmap retrieval
    return wxNullBitmap;
////@end RenameServerDlg bitmap retrieval
}

/*!
 * Get icon resources
 */

wxIcon RenameServerDlg::GetIconResource( const wxString& name )
{
    // Icon retrieval
////@begin RenameServerDlg icon retrieval
    return wxNullIcon;
////@end RenameServerDlg icon retrieval
}
