#include "wx/wxprec.h"
#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif
#ifndef WINS
#include "winrepl.h"
#endif
#include "cint.h"
#include "flstr.h"
#include "support.h"
#include "topdecl.h"
#pragma hdrstop

#include "mailprofdlg.h"


BEGIN_EVENT_TABLE(CMailProfDlg, wxDialog) 
    EVT_TEXT(ID_POP3SERVER,     CMailProfDlg::OnTextChanged) 
    EVT_TEXT(ID_USER,           CMailProfDlg::OnTextChanged) 
    EVT_TEXT(ID_SMPTSERVER,     CMailProfDlg::OnTextChanged) 
    EVT_TEXT(ID_ADDRESS,        CMailProfDlg::OnTextChanged) 
    EVT_COMBOBOX(ID_AUTH,       CMailProfDlg::OnAuthChange)
    EVT_TEXT(ID_SMTPUSER,       CMailProfDlg::OnTextChanged) 
    EVT_TEXT(ID_PATH,           CMailProfDlg::OnTextChanged) 
    EVT_TEXT(ID_MAILTABLE,      CMailProfDlg::OnTextChanged) 
    EVT_TEXT(ID_FILETABLE,      CMailProfDlg::OnTextChanged) 
    EVT_TEXT(ID_MAILSCHEMA,     CMailProfDlg::OnTextChanged) 
    EVT_TEXT(ID_PASSWORD,       CMailProfDlg::OnPasswordChanged) 
    EVT_TEXT(ID_SMTPPASSW,      CMailProfDlg::OnsPasswordChanged) 
    EVT_BUTTON(wxID_OK,         CMailProfDlg::OnOK) 
    EVT_BUTTON(wxID_HELP,       CMailProfDlg::OnHelpClick)
#ifdef WINS
    EVT_COMBOBOX(ID_MAILTYPE,   CMailProfDlg::OnMailTypeChange)
    EVT_TEXT(ID_DIAL,           CMailProfDlg::OnTextChanged) 
    EVT_TEXT(ID_DIALNAME,       CMailProfDlg::OnTextChanged) 
    EVT_TEXT(ID_NAME602,        CMailProfDlg::OnTextChanged) 
    EVT_TEXT(ID_DIALPASSW,      CMailProfDlg::OndPasswordChanged) 
    EVT_TEXT(ID_PASSW602,       CMailProfDlg::OnPasswordChanged) 
    EVT_TEXT(ID_PASSWMAPI,      CMailProfDlg::OnPasswordChanged) 
    EVT_COMBOBOX(ID_PROFIL602,  CMailProfDlg::OnProf602Changed)
    EVT_COMBOBOX(ID_PROFILMAPI, CMailProfDlg::OnComboChanged)
    EVT_COMBOBOX(ID_DIAL,       CMailProfDlg::OnDialChanged)
#endif
END_EVENT_TABLE()

#define DialUp         _("&Dial-up Connection:")
#define AttachTab      _("&Attachments Table:")
#define SMTPServer     _T("SMTPServer")
#define POP3Server     _T("POP3Server")
#define MyAddress      _T("MyAddress")
#define UserName       _T("UserName")
#define SMTPUserName   _T("SMTPUserName")
#define SMTPPassword   _T("SMTPPassword")
#define DialConn       _T("DialConn")
#define DialUserName   _T("DialUserName")
#define FilePath       _T("FilePath")
#define InBoxAppl      _T("InBoxAppl")
#define InBoxMessages  _T("InBoxMessages")
#define InBoxFiles     _T("InBoxFiles")
#define TpsSMTP        _T("SMTP/POP3")
#define AuthNotNeeded  _("SMTP server does not require authentication")
#define AuthSameAsPOP3 _("For authentication use POP3 settings")
#define AuthSeparate   _("For authentication use separate settings")

#ifdef WINS
#define Tps602        _T("Mail602")
#define TpsMAPI       _T("MAPI")
#define Profile602    _T("Profile602")
#define ProfileMAPI   _T("ProfileMAPI")
#define Options       _T("Options")
#else
#define Password      _T("Password")
#endif
//
// Vytvoreni dialogu
//
void CMailProfDlg::Create(wxWindow *Parent, const wxString &ProfileName)
{
    // Naplnit MailTypes seznamem podporovanych post
    wxString MailTypes[3];
    int MailTypeCnt = GetMailTypes(MailTypes);
    int sz;

    m_ProfileName = ProfileName;

    wxScreenDC dc;
    dc.SetFont(*wxNORMAL_FONT);
    dc.GetTextExtent(DialUp, &m_LabelSize.x, &m_LabelSize.y);
    dc.GetTextExtent(DialUp, &sz,            &m_LabelSize.y);
    if (sz > m_LabelSize.x)
        m_LabelSize.x = sz;
    m_LabelSize.x += 4;
    m_LabelSize.y = -1;

    wxString Title = _("Mail Profile ") + ProfileName;
    wxDialog::Create(Parent, 0, Title, wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER);

    // Defaultni typ = SMTP/POP3
    wxString Type = TpsSMTP;

    // Otevrit profil a nacist typ
    if (m_pl.Open(ProfileName, false))
    {
#ifdef WINS
        if (*m_pl.GetStrValue(Profile602))
            Type = Tps602;
        else if (*m_pl.GetStrValue(ProfileMAPI))
            Type = TpsMAPI;
#endif
    }

    m_MainSizer = new wxBoxSizer(wxVERTICAL);
    SetSizer(m_MainSizer);
    SetAutoLayout(TRUE);

    wxFlexGridSizer *TopSizer = new wxFlexGridSizer(1, 2, 0, 0);
    TopSizer->AddGrowableCol(1);
    m_MainSizer->Add(TopSizer, 0, wxEXPAND | wxADJUST_MINSIZE | wxALL, 8);

    // Combo s typem posty
    wxStaticText *Static = new wxStaticText(this, wxID_STATIC, _("&Mail Type:"), wxDefaultPosition, m_LabelSize);
    TopSizer->Add(Static, 0, wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL | wxADJUST_MINSIZE, 0);
    m_MailTypeCB.Create(this, ID_MAILTYPE, Type, wxDefaultPosition, wxSize(250, -1), MailTypeCnt, MailTypes, wxCB_READONLY); //sizeof(atTypes) / sizeof(wxString), atTypes, wxCB_READONLY))
    TopSizer->Add(&m_MailTypeCB, 0, wxEXPAND | wxTOP | wxBOTTOM, 1);

    // Podle typu posty vytvorit slozky dialogu
#ifdef WINS

    if (Type == Tps602)
        m_SizerAct = GetSizer602();
    else if (Type == TpsMAPI)
        m_SizerAct = GetSizerMAPI();
    else

#endif
        m_SizerAct = GetSizerSMTP();

    // Path a tabulky
    wxFlexGridSizer *CommonSizer = new wxFlexGridSizer(4, 2, 0, 0);
    CommonSizer->AddGrowableCol(1);
    m_MainSizer->Add(CommonSizer, 0, wxEXPAND | wxADJUST_MINSIZE | wxRIGHT | wxLEFT | wxBOTTOM, 8);

    Static = new wxStaticText(this, wxID_STATIC, _("&Path:"), wxDefaultPosition, m_LabelSize);
    CommonSizer->Add(Static, 0, wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL | wxADJUST_MINSIZE, 0);
    m_PathED.Create(this, ID_PATH, m_pl.GetStrValue(FilePath));
    CommonSizer->Add(&m_PathED, 0, wxEXPAND | wxTOP | wxBOTTOM, 1);

    Static = new wxStaticText(this, wxID_STATIC, _("&Inbox Schema:"), wxDefaultPosition, m_LabelSize);
    CommonSizer->Add(Static, 0, wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL | wxADJUST_MINSIZE, 0);
    m_MailSchemaED.Create(this, ID_MAILSCHEMA, m_pl.GetStrValue(InBoxAppl));
    CommonSizer->Add(&m_MailSchemaED, 0, wxEXPAND | wxTOP | wxBOTTOM, 1);

    Static = new wxStaticText(this, wxID_STATIC, _("&Inbox Table:"), wxDefaultPosition, m_LabelSize);
    CommonSizer->Add(Static, 0, wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL | wxADJUST_MINSIZE, 0);
    m_MailTableED.Create(this, ID_MAILTABLE, m_pl.GetStrValue(InBoxMessages));
    CommonSizer->Add(&m_MailTableED, 0, wxEXPAND | wxTOP | wxBOTTOM, 1);

    Static = new wxStaticText(this, wxID_STATIC, AttachTab, wxDefaultPosition, m_LabelSize);
    CommonSizer->Add(Static, 0, wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL | wxADJUST_MINSIZE, 0);
    m_FileTableED.Create(this, ID_FILETABLE, m_pl.GetStrValue(InBoxFiles));
    CommonSizer->Add(&m_FileTableED, 0, wxEXPAND | wxTOP | wxBOTTOM, 1);

    // OK Cancel
    wxBoxSizer *BottomSizer = new wxBoxSizer(wxHORIZONTAL);
    m_MainSizer->Add(BottomSizer, 0, wxALIGN_CENTER_HORIZONTAL | wxLEFT | wxRIGHT | wxBOTTOM | wxADJUST_MINSIZE, 8);
    m_OKBut.Create(this, wxID_OK, _("OK"));
    m_OKBut.SetDefault();
    m_OKBut.Enable(false);
    BottomSizer->Add(&m_OKBut, 0, wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT, 8);

    m_CancelBut.Create(this, wxID_CANCEL, _("Cancel"));
    BottomSizer->Add(&m_CancelBut, 0, wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT, 8);

    wxButton* itemButton16 = new wxButton;
    itemButton16->Create( this, wxID_HELP, _("&Help"), wxDefaultPosition, wxDefaultSize, 0 );
    BottomSizer->Add(itemButton16, 0, wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT, 8);

    m_MainSizer->Fit(this);
    m_MainSizer->SetSizeHints(this);
    m_pl.Close();

    m_Changed       = false;
    m_PasswChanged  = false;
    m_dPasswChanged = false;
#ifdef WINS
    m_dPasswChanged = false;
#endif
}

#ifdef WINS
#include <ras.h>
#include <raserror.h>

typedef DWORD (WINAPI *t_RasEnumEntries)(LPTSTR reserved, LPTSTR lpszPhonebook, LPRASENTRYNAMEA lprasentryname, LPDWORD lpcb, LPDWORD lpcEntries);
#endif
//
// Vraci sizer s polozkami SMTP/POP3 profilu
//
wxFlexGridSizer *CMailProfDlg::GetSizerSMTP()
{
    if (!m_SizerSMTP)
    {
        wxString AuthTypes[3];
        AuthTypes[0] = AuthNotNeeded; 
        AuthTypes[1] = AuthSameAsPOP3;
        AuthTypes[2] = AuthSeparate;  

        m_SizerSMTP = new wxFlexGridSizer(8, 2, 0, 0);
        m_SizerSMTP->AddGrowableCol(1);
        m_MainSizer->Insert(1, m_SizerSMTP, 0, wxEXPAND | wxADJUST_MINSIZE | wxRIGHT | wxLEFT | wxBOTTOM, 8);

        wxStaticText *Static = new wxStaticText(this, wxID_STATIC, _("&POP3 Server:"), wxDefaultPosition, m_LabelSize);
        m_SizerSMTP->Add(Static, 0, wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL | wxADJUST_MINSIZE, 0);
        m_POP3ED.Create(this, ID_POP3SERVER, m_pl.GetStrValue(POP3Server));
        m_SizerSMTP->Add(&m_POP3ED, 0, wxEXPAND | wxTOP | wxBOTTOM, 1);

        Static = new wxStaticText(this, wxID_STATIC, _("&User Name:"), wxDefaultPosition, m_LabelSize);
        m_SizerSMTP->Add(Static, 0, wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL | wxADJUST_MINSIZE, 0);
        wxBoxSizer *Sizer = new wxBoxSizer(wxHORIZONTAL);
        m_SizerSMTP->Add(Sizer, 0, wxEXPAND | wxTOP | wxBOTTOM, 1);
        wxString Val = m_pl.GetStrValue(UserName);
        m_NameED.Create(this, ID_USER, Val);
        Sizer->Add(&m_NameED, 1, wxEXPAND | wxRIGHT, 4);

        Static = new wxStaticText(this, wxID_STATIC, _("&Password:"));
        Sizer->Add(Static, 0, wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL | wxADJUST_MINSIZE | wxLEFT | wxRIGHT, 4);
        m_PasswordED.Create(this, ID_PASSWORD, Val.IsEmpty() ? wxString() : _T("***"), wxDefaultPosition, wxDefaultSize, wxTE_PASSWORD);
        m_PasswordED.SetMaxLength(30);
        Sizer->Add(&m_PasswordED, 1, wxEXPAND | wxTOP | wxBOTTOM, 1);

        Static = new wxStaticText(this, wxID_STATIC, _("&SMTP Server:"), wxDefaultPosition, m_LabelSize);
        m_SizerSMTP->Add(Static, 0, wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL | wxADJUST_MINSIZE, 0);
        m_SMTPED.Create(this, ID_SMPTSERVER, m_pl.GetStrValue(SMTPServer));
        m_SizerSMTP->Add(&m_SMTPED, 0, wxEXPAND | wxTOP | wxBOTTOM, 1);

        Static = new wxStaticText(this, wxID_STATIC, _("&E-Mail Address:"), wxDefaultPosition, m_LabelSize);
        m_SizerSMTP->Add(Static, 0, wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL | wxADJUST_MINSIZE, 0);
        m_AddressED.Create(this, ID_ADDRESS, m_pl.GetStrValue(MyAddress));
        m_SizerSMTP->Add(&m_AddressED, 0, wxEXPAND | wxTOP | wxBOTTOM, 1);

        Static = new wxStaticText(this, wxID_STATIC, _("&Authentication:"), wxDefaultPosition, m_LabelSize);
        m_SizerSMTP->Add(Static, 0, wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL | wxADJUST_MINSIZE, 0);
        m_AuthCB.Create(this, ID_AUTH, AuthSameAsPOP3, wxDefaultPosition, wxSize(250, -1), 3, AuthTypes, wxCB_READONLY);
        m_SizerSMTP->Add(&m_AuthCB, 0, wxEXPAND | wxTOP | wxBOTTOM, 1);

        m_SMTPNameLab.Create(this, wxID_STATIC, _("&User Name:"), wxDefaultPosition, m_LabelSize);
        m_SizerSMTP->Add(&m_SMTPNameLab, 0, wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL | wxADJUST_MINSIZE, 0);
        Sizer = new wxBoxSizer(wxHORIZONTAL);
        m_SizerSMTP->Add(Sizer, 0, wxEXPAND | wxTOP | wxBOTTOM, 1);
        m_SMTPNameED.Create(this, ID_SMTPUSER);
        Sizer->Add(&m_SMTPNameED, 1, wxEXPAND | wxRIGHT, 4);

        m_SMTPPasswLab.Create(this, wxID_STATIC, _("&Password:"));
        Sizer->Add(&m_SMTPPasswLab, 0, wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL | wxADJUST_MINSIZE | wxLEFT | wxRIGHT, 4);
        m_SMTPPasswED.Create(this, ID_SMTPPASSW, wxString(), wxDefaultPosition, wxDefaultSize, wxTE_PASSWORD);
        m_SMTPPasswED.SetMaxLength(30);
        Sizer->Add(&m_SMTPPasswED, 1, wxEXPAND | wxTOP | wxBOTTOM, 1);
        ShowAuth();

#ifdef WINS

        Static = new wxStaticText(this, wxID_STATIC, DialUp, wxDefaultPosition, m_LabelSize);
        m_SizerSMTP->Add(Static, 0, wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL | wxADJUST_MINSIZE, 0);
        m_DialCB.Create(this, ID_DIAL, wxString(), wxDefaultPosition, wxDefaultSize, 0, NULL, wxCB_READONLY);
        m_DialCB.Append(_("- no dial-up connection -"));
        DWORD entries;
        HINSTANCE hRasLib = LoadLibraryA("RASAPI32.DLL");
        if (hRasLib)
        {
            t_RasEnumEntries pRasEnumEntries;
            pRasEnumEntries = (t_RasEnumEntries)GetProcAddress(hRasLib, "RasEnumEntriesA");
            if (pRasEnumEntries)  /* unless Win NT */
            {
                RASENTRYNAMEA ren, *pren;  
                DWORD bufsize;  
                unsigned i;
                ren.dwSize = sizeof(RASENTRYNAMEA);
                bufsize    = sizeof(RASENTRYNAMEA);
                pRasEnumEntries(NULL, NULL, &ren, &bufsize, &entries);
                bufsize   += sizeof(RASENTRYNAMEA) + 1;  // RasEnumEntries returns 3 without this
                pren       = (RASENTRYNAMEA *)malloc(bufsize);
                if (pren)
                {
                    pren->dwSize = sizeof(RASENTRYNAMEA);
                    if (!pRasEnumEntries(NULL, NULL, pren, &bufsize, &entries))
                        for (i = 0; i < entries; i++)
                            m_DialCB.Append(wxString(pren[i].szEntryName, *wxConvCurrent));
                    free(pren);
                }
            }
            FreeLibrary(hRasLib);
        }
        Val = m_pl.GetStrValue(DialConn);
        if (Val.IsEmpty())
            m_DialCB.SetSelection(0);
        else
            m_DialCB.SetValue(Val);
        m_SizerSMTP->Add(&m_DialCB, 0, wxEXPAND | wxTOP | wxBOTTOM, 1);

        m_DialNameLab.Create(this, wxID_STATIC, _("&User Name:"), wxDefaultPosition, m_LabelSize);
        m_DialNameLab.Enable(!Val.IsEmpty());
        m_SizerSMTP->Add(&m_DialNameLab, 0, wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL | wxADJUST_MINSIZE, 0);
        Sizer = new wxBoxSizer(wxHORIZONTAL);
        m_SizerSMTP->Add(Sizer, 0, wxEXPAND | wxTOP | wxBOTTOM, 1);
        wxString User = m_pl.GetStrValue(DialUserName);
        m_DialNameED.Create(this, ID_DIALNAME, User);
        m_DialNameED.Enable(!Val.IsEmpty());
        Sizer->Add(&m_DialNameED, 1, wxEXPAND | wxRIGHT, 4);
        m_DialPasswLab.Create(this, wxID_STATIC, _("&Password:"));
        m_DialPasswLab.Enable(!Val.IsEmpty());
        Sizer->Add(&m_DialPasswLab, 0, wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL | wxADJUST_MINSIZE | wxLEFT | wxRIGHT, 4);
        m_DialPasswED.Create(this, ID_DIALPASSW, Val.IsEmpty() || User.IsEmpty() ? wxString() : _T("***"), wxDefaultPosition, wxDefaultSize, wxTE_PASSWORD);
        m_DialPasswED.Enable(!Val.IsEmpty());
        m_DialPasswED.SetMaxLength(30);
        Sizer->Add(&m_DialPasswED, 1, wxEXPAND | wxTOP | wxBOTTOM, 1);

#endif
    }
    return m_SizerSMTP;
}
//
// Podle parametru profilu nastavi typ autentizace
//
void CMailProfDlg::ShowAuth()
{
    wxString SMTPUser;
    TSMTPAuth Auth = AUTH_SAMEASPOP3; // Defaulne pro stare profily

#ifdef WINS

    char pw[96];
    // IF novy profil
    if (m_pl.GetBinValue(Options, pw, sizeof(pw)) == 96)
    {
        pw[ 0]  ^= pw[17];
        pw[64]  ^= pw[88];
        SMTPUser = m_pl.GetStrValue(SMTPUserName);
        // IF neni SMTP jmeno ani SMTP heslo, server nechce autentizaci
        if (SMTPUser.IsEmpty() && !pw[64])
            Auth = AUTH_NOTNEEDED;
        // IF jine heslo nebo jmeno, separatni autentizace
        else if (memcmp(pw, pw + 64, pw[0] + 1) != 0 || SMTPUser.CmpNoCase(m_pl.GetStrValue(UserName)) != 0)
            Auth = AUTH_SEPARATE;
    }

#else

    // IF novy profil
    if (m_pl.Exists(SMTPUserName) || m_pl.Exists(SMTPPassword))
    {
        SMTPUser           = m_pl.GetStrValue(SMTPUserName);
        wxString SMTPPassw = m_pl.GetStrValue(SMTPPassword);
        // IF neni SMTP jmeno ani SMTP heslo, server nechce autentizaci
        if (SMTPUser.IsEmpty() && SMTPPassw.IsEmpty())
            Auth = AUTH_NOTNEEDED;
        // IF jine heslo nebo jmeno, separatni autentizace
        else if (SMTPUser.CmpNoCase(m_pl.GetStrValue(UserName)) != 0 || SMTPPassw.CmpNoCase(m_pl.GetStrValue(Password)) != 0)
            Auth = AUTH_SEPARATE;
    }

#endif

    m_AuthCB.SetSelection(Auth); 
    AuthChange(Auth, SMTPUser);
}
//
// Podle typu autentizace povoli nebo zakaze prislusne slozky dialogu
//
void CMailProfDlg::AuthChange(TSMTPAuth Auth, const wxString &SMTPName)
{
    if (Auth == AUTH_SEPARATE)
    {
        m_SMTPNameLab.Enable();
        m_SMTPNameED.Enable();
        m_SMTPNameED.SetValue(SMTPName);
        m_SMTPPasswLab.Enable();
        m_SMTPPasswED.Enable();
        m_SMTPPasswED.SetValue(SMTPName.IsEmpty() ? wxEmptyString : _T("***"));
    }
    else
    {
        m_SMTPNameLab.Enable(false);
        m_SMTPNameED.Enable(false);
        m_SMTPNameED.SetValue(wxEmptyString);
        m_SMTPPasswLab.Enable(false);
        m_SMTPPasswED.Enable(false);
        m_SMTPPasswED.SetValue(wxEmptyString);
    }
}

#ifdef WINS
//
// Vraci sizer s polozkami Mail602
//
wxFlexGridSizer *CMailProfDlg::GetSizer602()
{
    if (!m_Sizer602)
    {
        m_Sizer602 = new wxFlexGridSizer(2, 2, 0, 0);
        m_Sizer602->AddGrowableCol(1);
        m_MainSizer->Insert(1, m_Sizer602, 0, wxEXPAND | wxADJUST_MINSIZE | wxRIGHT | wxLEFT | wxBOTTOM, 8);

        wxStaticText *Static = new wxStaticText(this, wxID_STATIC, _("&Mail602 Profile:"), wxDefaultPosition, m_LabelSize);
        m_Sizer602->Add(Static, 0, wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL | wxADJUST_MINSIZE, 0);
        m_Prof602CB.Create(this, ID_PROFIL602, wxString(), wxDefaultPosition, wxDefaultSize, 0, NULL, wxCB_READONLY);
        m_Sizer602->Add(&m_Prof602CB, 0, wxEXPAND | wxTOP | wxBOTTOM, 1);
        for (bool Found = m_pl.FindFirst602(); Found; Found = m_pl.FindNext602())
            m_Prof602CB.Append(m_pl.GetName602());
        wxString Profile = m_pl.GetStrValue(Profile602);
        if (Profile.IsEmpty())
            m_Prof602CB.SetSelection(0);
        else
            m_Prof602CB.SetValue(Profile);

        m_Name602Lab.Create(this, wxID_STATIC, _("&ID/User Name:"), wxDefaultPosition, m_LabelSize);
        m_Sizer602->Add(&m_Name602Lab, 0, wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL | wxADJUST_MINSIZE, 0);
        wxBoxSizer *Sizer = new wxBoxSizer(wxHORIZONTAL);
        m_Sizer602->Add(Sizer, 0, wxEXPAND, 0);
        m_Name602ED.Create(this, ID_NAME602);
        Sizer->Add(&m_Name602ED, 1, wxEXPAND | wxRIGHT, 4);
        m_Passw602Lab.Create(this, wxID_STATIC, _("&Password:"));
        Sizer->Add(&m_Passw602Lab, 0, wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT, 4);
        m_Passw602ED.Create(this, ID_PASSW602, wxString(), wxDefaultPosition, wxDefaultSize, wxTE_PASSWORD);
        Sizer->Add(&m_Passw602ED, 1, wxEXPAND | wxTOP | wxBOTTOM, 1);
        OnProf602Changed(Profile, m_pl.GetStrValue(UserName));
    }
    return m_Sizer602;
}
//
// Vraci sizer s polozkami MAPI
//
wxFlexGridSizer *CMailProfDlg::GetSizerMAPI()
{
    if (!m_SizerMAPI)
    {
        m_SizerMAPI = new wxFlexGridSizer(2, 2, 0, 0);
        m_SizerMAPI->AddGrowableCol(1);
        m_MainSizer->Insert(1, m_SizerMAPI, 0, wxEXPAND | wxADJUST_MINSIZE | wxRIGHT | wxLEFT | wxBOTTOM, 8);

        wxStaticText *Static = new wxStaticText(this, wxID_STATIC, _("&MAPI Profile:"), wxDefaultPosition, m_LabelSize);
        m_SizerMAPI->Add(Static, 0, wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL | wxADJUST_MINSIZE, 0);
        m_ProfMAPICB.Create(this, ID_PROFILMAPI, wxString(), wxDefaultPosition, wxDefaultSize, 0, NULL, wxCB_READONLY);
        m_SizerMAPI->Add(&m_ProfMAPICB, 0, wxEXPAND | wxTOP | wxBOTTOM, 1);
        for (bool Found = m_pl.FindFirstMAPI(); Found; Found = m_pl.FindNextMAPI())
            m_ProfMAPICB.Append(m_pl.GetNameMAPI());
        wxString Profile = m_pl.GetStrValue(ProfileMAPI);
        if (Profile.IsEmpty())
            m_ProfMAPICB.SetSelection(0);
        else
            m_ProfMAPICB.SetValue(Profile);

        Static = new wxStaticText(this, wxID_STATIC, _("&Password:"), wxDefaultPosition, m_LabelSize);
        m_SizerMAPI->Add(Static, 0, wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL | wxADJUST_MINSIZE, 0);
        m_PasswMAPIED.Create(this, ID_PASSWMAPI, _T("***"), wxDefaultPosition, wxDefaultSize, wxTE_PASSWORD);
        m_PasswMAPIED.SetMaxLength(30);
        m_SizerMAPI->Add(&m_PasswMAPIED, 0, wxEXPAND | wxTOP | wxBOTTOM, 1);
    }
    return m_SizerMAPI;
}

#endif
//
// Do MailTypes naplni seznam podporovanych typu posty, vraci pocet podporovanych typu posty
//
int CMailProfDlg::GetMailTypes(wxString *MailTypes)
{
    int i = 0;
    if (IsSMTP())
    {   
        TypeSMTP = i;
        MailTypes[i++] = TpsSMTP;
    }
#ifdef WINS
#ifdef MAIL602
    if (Is602())
    {
        Type602 = i;
        MailTypes[i++] = Tps602;
    }
#endif
    if (IsMAPI())
    {
        TypeMAPI = i;
        MailTypes[i++] = TpsMAPI;
    }

#endif

    return i;
}
//
// Testuje, zda je pouzitelne SMTP/POP3
//
bool CMailProfDlg::IsSMTP()
{
#ifdef WINS

    WSADATA WsaData;
    if (WSAStartup(0x0101, &WsaData) != NOERROR)
        return false;
    PROTOENT *sp = getprotobyname("tcp");
    return sp != NULL;

#else
   
    return true;

#endif       
}


#ifdef WINS

#ifdef MAIL602
static char MultiUser[] = "Software602\\Mail602.Klient\\MultiUser"; 

typedef short (WINAPI *LPJEEMI100)(char *Adresar);

static LPJEEMI100 lpJeEmi100;
//
// Testuje, zda existuje nejaky Mail602 klient
// Zjisti cestu k MAIL602.INI a naloaduje wm602m32.dll
//
bool CMailProfDlg::Is602()
{
    HKEY hKey;
    char Ini[MAX_PATH];
    *Ini = 0;
    if (RegOpenKeyExA(HKEY_CLASSES_ROOT, MultiUser, 0, KEY_QUERY_VALUE_EX, &hKey) == ERROR_SUCCESS)
    {
        DWORD Multion;
        DWORD sz  = sizeof(Multion);
        LONG  Err = RegQueryValueExA(hKey, "multion", NULL, NULL, (LPBYTE)&Multion, &sz);
        RegCloseKey(hKey);
        if (Err == ERROR_SUCCESS && Multion)
        {
            if (RegOpenKeyExA(HKEY_CURRENT_USER, MultiUser, 0, KEY_QUERY_VALUE_EX, &hKey) == ERROR_SUCCESS)
            {
                sz  = sizeof(Ini);
                RegQueryValueExA(hKey, "inipath", NULL, NULL, (LPBYTE)Ini, &sz);
                RegCloseKey(hKey);
            }
        }
    }
    if (!*Ini)
        GetWindowsDirectoryA(Ini, sizeof(Ini));
    int sz = lstrlenA(Ini);
    if (Ini[sz - 1] == '\\')
        sz--;
    lstrcpyA(Ini + sz, "\\MAIL602.INI");
    WIN32_FIND_DATAA ff;
    HANDLE hFF = FindFirstFileA(Ini, &ff);
    if (hFF == INVALID_HANDLE_VALUE)
        return false;
    FindClose(hFF);
    m_hMail602 = LoadLibraryA("wm602m32.dll");
    if (!m_hMail602)
        return false;
    lpJeEmi100 = (LPJEEMI100)GetProcAddress(m_hMail602, "JeEMI100");
    return lpJeEmi100 != NULL;
    return false;
}
#else  // !MAIL602
#endif    
//
// Testuje, zda existuje nejaky MAPI klient
//
bool CMailProfDlg::IsMAPI()
{
    m_hMAPI = LoadLibraryA("mapi32.dll");
    if (!m_hMAPI)
        return false;
    LPMAPIINITIALIZE lpMAPIInitialize = (LPMAPIINITIALIZE)GetProcAddress(m_hMAPI, "MAPIInitialize");
    if (!lpMAPIInitialize)
        return false;
    MAPIINIT_0 MapInit = {MAPI_INIT_VERSION, MAPI_MULTITHREAD_NOTIFICATIONS};
    return lpMAPIInitialize(&MapInit) == NOERROR;
}
//
// Udalost Zmena typu posty, schova polozky pro stary typ, ukaze polozky pro novy typ
//
void CMailProfDlg::OnMailTypeChange(wxCommandEvent &event)
{
    wxFlexGridSizer *Sizer;
    if (event.GetSelection() == TypeSMTP)
        Sizer = GetSizerSMTP();
    else if (event.GetSelection() == Type602)
        Sizer = GetSizer602();
    else
        Sizer = GetSizerMAPI();
    m_MainSizer->Show(m_SizerAct, false);
    m_MainSizer->Show(Sizer, true);
    m_SizerAct = Sizer;
    m_MainSizer->Layout();
    m_MainSizer->SetSizeHints(this);
    m_MainSizer->Fit(this);
    m_Changed = true;
}

#if 0
void CMailProfDlg::OnInternalIdle()
{
    wxSize sSize = GetSizer()->GetMinSize();
    wxSize cSize = GetClientSize();
    if (sSize.y < cSize.y)
        SetClientSize(sSize);
}
#endif

#endif // WINS
//
// Udalost Zmena vlastnosti profilu
//
void CMailProfDlg::OnTextChanged(wxCommandEvent &event)
{
    m_Changed = true;
    m_OKBut.Enable(true);
}
//
// Udalost Zmena typu SMTP autentizace
//
void CMailProfDlg::OnAuthChange(wxCommandEvent &event)
{
    AuthChange((TSMTPAuth)event.GetSelection(), wxEmptyString);
}
//
// Udalost Zmena hesla POP3, Mail602 nebo MAPI
//
void CMailProfDlg::OnPasswordChanged(wxCommandEvent &event)
{
    m_Changed      = true;
    m_PasswChanged = true;
    m_OKBut.Enable(true);
}
//
// Udalost Zmena hesla SMTP
//
void CMailProfDlg::OnsPasswordChanged(wxCommandEvent &event)
{
    m_Changed       = true;
    m_sPasswChanged = true;
    m_OKBut.Enable(true);
}

#ifdef WINS
//
// Udalost Zmena vlastnosti profilu
//
void CMailProfDlg::OnComboChanged(wxCommandEvent &event)
{
    m_Changed      = true;
    m_OKBut.Enable(true);
}
//
// Udalost Zmena DialUp connection
//
void CMailProfDlg::OnDialChanged(wxCommandEvent &event)
{
    m_Changed = true;
    bool Enb  = event.GetSelection() != 0;
    m_DialNameLab.Enable(Enb);
    m_DialNameED.Enable(Enb);
    m_DialPasswLab.Enable(Enb);
    m_DialPasswED.Enable(Enb);

    m_OKBut.Enable(true);
}
//
// Udalost Zmena DialUp hesla
//
void CMailProfDlg::OndPasswordChanged(wxCommandEvent &event)
{
    m_Changed       = true;
    m_dPasswChanged = true;
    m_OKBut.Enable(true);
}
//
// Udalost Zmena profilu Mail602
//
void CMailProfDlg::OnProf602Changed(wxCommandEvent &event)
{
    OnProf602Changed(event.GetString(), wxString());
    m_Changed = true;
    m_OKBut.Enable(true);
}
//
// Podle typu Mail602 povoli slozky v dialogu
//
void CMailProfDlg::OnProf602Changed(const wxString &Profile, const wxString &User)
{
    bool NetLan = Is602NetLan(Profile.mb_str(*wxConvCurrent)) != 0;

    // IF sitovy klient, zobrazit jmeno a heslo
    m_Name602Lab.Enable(NetLan);
    m_Name602ED.SetValue(NetLan ? User : wxString());
    m_Name602ED.Enable(NetLan);
    m_Passw602Lab.Enable(NetLan);
    m_Passw602ED.SetValue(NetLan && !User.IsEmpty() ? _("***") : wxString());
    m_Passw602ED.Enable(NetLan);
}
//
// Sifrovani hesla
//
inline void Prohoz(char *A, char *B)
{
    char C = *A;
    *A = *B;
    *B = C;
}

void Mix(char *Dst)
{
    BYTE  I, K;
    short J;
    K = *Dst;
    for (I = 2; I <= K; I++)
    {
        for (J = 1; J <= K; J++)
            Dst[J] = Dst[J] ^ J;
        for (J = 0; J < K / I; J++)
            Prohoz(&Dst[J * I + 1], &Dst[(J + 1) * I]);
        for (J = 1; J < K; J++)
            Dst[J] = Dst[J] ^ Dst[J + 1];
    }
}

void DeMix(char *Dst)
{
    BYTE I,K;
    short J;

    K = *Dst;
    for (I = K; I >= 2; I--)
    {
        for (J = K - 1; J >= 1; J--)
            Dst[J] = Dst[J] ^ Dst[J + 1];
        for (J = (K / I) - 1; J >= 0; J--)
            Prohoz(&Dst[J * I + 1], &Dst[(J + 1) * I]);
        for (J = 1; J <= K; J++)
            Dst[J] = Dst[J] ^ J;
    }
}

#endif

#define EMAIL_NOPROFILE602  _("No Mail602 profile selected")
#define EMAIL_NOPROFILEMAPI _("No MAPI profile selected")
#define EMAIL_NOSMTP        _("No mail server specified")
#define EMAIL_NOMYADDR      _("No From address specified")
#define EMAIL_NOPOP3USER    _("No POP3 user name specified")
#define EMAIL_INVALIDPROF   _("Invalid profile name")
#define EMAIL_PROFEXIST     _("Profile already exists, enter another name")
//
// Udalost Ulozit zmeny v profilu
// 
void CMailProfDlg::OnOK(wxCommandEvent &event)
{
    if (!m_pl.Open(m_ProfileName, true))
    {
        error_box(SysErrorMsg(_("Failed to save mail profile due to error:\n\n%ErrMsg")));
        return;
    }

    wxString Msg;
    wxString Profile;
#ifndef WINS    
    wxString pw;
    wxString spw;
#endif    

#ifdef WINS

    int NetLan;
    
    int Type = m_MailTypeCB.GetSelection();
    if (Type == Type602)
    {
        // IF neni vybran profil Mail602, chyba
        Profile = m_Prof602CB.GetValue();
        if (Profile.IsEmpty())
        {
            int i = m_Prof602CB.GetSelection();
            if (i == wxNOT_FOUND)
            {
                Msg = EMAIL_NOPROFILE602;  
                goto exit; 
            }
            Profile = m_Prof602CB.GetString(i);
        }
        
        // IF nepodarilo se zjistit typ Mail602 klienta, chyba
        NetLan = Is602NetLan(Profile.mb_str(*wxConvCurrent));
        if (NetLan < 0)
        {
            Msg = EMAIL_INVALIDPROF;
            goto exit;
        }
    }
    else if (Type == TypeMAPI)
    {
        // IF neni vybran profil MAPI, chyba
        Profile = m_ProfMAPICB.GetValue();
        if (Profile.IsEmpty())
        {
            int i = m_ProfMAPICB.GetSelection();
            if (i == wxNOT_FOUND)
            {
                Msg = EMAIL_NOPROFILEMAPI;  
                goto exit; 
            }
            Profile = m_ProfMAPICB.GetString(i);
        }
    }
    else

#endif

    {
        bool bs = m_SMTPED.GetValue().IsEmpty();
        bool ba = m_AddressED.GetValue().IsEmpty();
        bool bp = m_POP3ED.GetValue().IsEmpty();
        bool bu = m_NameED.GetValue().IsEmpty();
        // IF neni nastaven server SMTP ani POP3, chyba
        if (bs && bp)
        {
            Msg = EMAIL_NOSMTP;
            goto exit;
        }
        // IF nastaven server SMTP a neni adresa odesilatele, chyba
        if (!bs && ba)
        {
            Msg = EMAIL_NOMYADDR;  
            goto exit; 
        }
        // IF nastaven server POP3 a neni jmeno uzivatele, chyba
        if (!bp && bu)
        {
            Msg = EMAIL_NOPOP3USER;  
            goto exit; 
        }
    }

#ifdef WINS

    // Nacist a odsifrovat heslo
    char pw[96];
    pw[ 1] = 0;
    pw[33] = 0;
    pw[65] = 0;
    if (m_pl.GetBinValue(Options, pw, sizeof(pw)))
    {
        pw[ 0] ^= pw[17];
        if (pw[0] > 30)     // Ochrana proti preteceni
        {
            pw[0]  = 30;
            pw[31] = 0;
        }
        DeMix(pw);
        pw[32] ^= pw[43];
        if (pw[32] > 30)
        {
            pw[32] = 30;
            pw[63] = 0;
        }
        DeMix(pw + 32);
        pw[64] ^= pw[88];
        if (pw[64] > 30)
        {
            pw[64] = 30;
            pw[95] = 0;
        }
        DeMix(pw + 64);
    }

#else

    pw = m_pl.GetStrValue(_T("Password"));

#endif

    // Smazat stare vlastnosti profilu
    m_pl.DeleteAllValues();

#ifdef WINS

    // Mail602 - Zapsat jmeno profilu Mail602, pro sitoveho klienta jmeno uzivatele a heslo
    if (Type == Type602)
    {
        m_pl.SetStrValue(Profile602, Profile);
        if (NetLan)
        {
            m_pl.SetStrValue(UserName, m_Name602ED.GetValue());
            if (m_PasswChanged)
                strcpy(pw + 1, m_Passw602ED.GetValue().mb_str(*wxConvCurrent));
        }
    }
    // MAPI - Zapsat jmeno profilu MAPI a heslo
    else if (Type == TypeMAPI)
    {
        m_pl.SetStrValue(ProfileMAPI, Profile);
        if (m_PasswChanged)
            strcpy(pw + 1, m_PasswMAPIED.GetValue().mb_str(*wxConvCurrent));
    }
    else

#endif

    // SMTP/POP3 - Zapsat vlastnosti SMTP/POP3
    {
        m_pl.SetStrValue(SMTPServer,    m_SMTPED.GetValue());
        m_pl.SetStrValue(MyAddress,     m_AddressED.GetValue());
        m_pl.SetStrValue(POP3Server,    m_POP3ED.GetValue());
        m_pl.SetStrValue(UserName,      m_NameED.GetValue());

#ifdef WINS

        wxString dc = m_DialCB.GetSelection() ? m_DialCB.GetValue() : wxEmptyString;
        m_pl.SetStrValue(DialConn,      dc);
        m_pl.SetStrValue(DialUserName,  m_DialNameED.GetValue());
        if (m_PasswChanged)
            strcpy(pw + 1, m_PasswordED.GetValue().mb_str(*wxConvCurrent));
        if (m_dPasswChanged)
            strcpy(pw + 33, m_DialPasswED.GetValue().mb_str(*wxConvCurrent));
        if (m_sPasswChanged)
            strcpy(pw + 65, m_SMTPPasswED.GetValue().mb_str(*wxConvCurrent));
        TSMTPAuth Auth = (TSMTPAuth)m_AuthCB.GetSelection();  //GetCurrentSelection();
        if (Auth == AUTH_SAMEASPOP3)
        {
            m_pl.SetStrValue(SMTPUserName, m_NameED.GetValue());
            strcpy(pw + 65, pw + 1);
        }
        else if (Auth == AUTH_SEPARATE)
        {
            m_pl.SetStrValue(SMTPUserName, m_SMTPNameED.GetValue());
        }
        else
        {
            m_pl.SetStrValue(SMTPUserName, wxEmptyString);
            pw[65] = 0;
        }
#else
        if (m_PasswChanged)
            pw = m_PasswordED.GetValue();
        if (m_sPasswChanged)
            spw = m_SMTPPasswED.GetValue();
        TSMTPAuth Auth = (TSMTPAuth)m_AuthCB.GetSelection();  //GetCurrentSelection();
        if (Auth == AUTH_SAMEASPOP3)
        {
            m_pl.SetStrValue(SMTPUserName, m_NameED.GetValue());
            spw = pw;
        }
        else if (Auth == AUTH_SEPARATE)
        {
            m_pl.SetStrValue(SMTPUserName, m_SMTPNameED.GetValue());
        }
        else
        {
            m_pl.SetStrValue(SMTPUserName, wxEmptyString);
            spw.Clear();
        }
#endif
    }
    m_pl.SetStrValue(InBoxAppl,     m_MailSchemaED.GetValue());
    m_pl.SetStrValue(InBoxMessages, m_MailTableED.GetValue());
    m_pl.SetStrValue(InBoxFiles,    m_FileTableED.GetValue());
    m_pl.SetStrValue(FilePath,      m_PathED.GetValue());

    // Zapsat hesla
#ifdef WINS
    pw[ 0] = (int)strlen(pw + 1);
    Mix(pw);
    pw[ 0] ^= pw[17];
    pw[32] = (int)strlen(pw + 33);
    Mix(pw + 32);
    pw[32] ^= pw[43];
    pw[64] = (int)strlen(pw + 65);
    Mix(pw + 64);
    pw[64] ^= pw[88];
    m_pl.SetBinValue(Options, pw, sizeof(pw));
#else
    m_pl.SetStrValue(Password,     pw);
    m_pl.SetStrValue(SMTPPassword, spw);
#endif
    event.Skip();
    return;

exit:
    
    error_box(Msg, this);
    m_pl.Close();
}

#ifdef WINS
//
// Testuje, zda Mail602 klient je sitovy
// Vraci: -1 - chyba 
//         1 - sitovy klient
//         0 - remote nebo SMTP/POP3
//
int CMailProfDlg::Is602NetLan(const char *Profile)
{
#ifdef MAIL602
    char EMI[MAX_PATH];
    // IF Profil602, EMI nacist z profilu
    DWORD Size = GetPrivateProfileStringA("PROFILES", Profile, NULL, EMI, sizeof(EMI), "Mail602.INI");
    if (!Size)
        return -1;
    Size = GetPrivateProfileStringA("SETTINGS", "MAIL602DIR", NULL, EMI + 1, sizeof(EMI) - 1, EMI);
    if (!Size)
        return -1;
    EMI[0] = strlen(EMI + 1);

    short emt = 0;
    if (lpJeEmi100)
        emt = lpJeEmi100(EMI);
    // IF sitovy klient, zobrazit edit pro jmeno a heslo
    return emt != 6 && emt != 7;
#else
    return -2;
#endif    
}

#endif

void CMailProfDlg::OnHelpClick( wxCommandEvent& event )
{
    wxGetApp().frame->help_ctrl.DisplaySection(wxT("html/HE_MAILPROFILES.htm"));
}

