#ifndef _MAILPROFDLG_H_
#define _MAILPROFDLG_H_

#include "objlist9.h"

enum
{
    ID_MAILTYPE,
    ID_POP3SERVER,
    ID_USER,
    ID_PASSWORD,
    ID_SMPTSERVER,
    ID_ADDRESS,
    ID_AUTH,
    ID_SMTPUSER,
    ID_SMTPPASSW,
    ID_DIAL,
    ID_DIALNAME,
    ID_DIALPASSW,
    
    ID_PROFIL602,
    ID_NAME602,
    ID_PASSW602,

    ID_PROFILMAPI,
    ID_PASSWMAPI,

    ID_PATH,
    ID_MAILTABLE,
    ID_FILETABLE,
    ID_MAILSCHEMA
};

enum TSMTPAuth {AUTH_NOTNEEDED, AUTH_SAMEASPOP3, AUTH_SEPARATE};

class CMailProfDlg : public wxDialog
{
protected:

    int        TypeSMTP;
    int        Type602;
    int        TypeMAPI;

    bool             m_Changed;
    bool             m_PasswChanged;
    bool             m_dPasswChanged;
    bool             m_sPasswChanged;
    wxSize           m_LabelSize;
    CMailProfList    m_pl;
    wxString         m_ProfileName;
    wxOwnerDrawnComboBox       m_MailTypeCB;

    wxBoxSizer      *m_MainSizer;
    wxFlexGridSizer *m_SizerAct;
    wxFlexGridSizer *m_SizerSMTP;
    wxTextCtrl       m_POP3ED;
    wxTextCtrl       m_NameED;
    wxTextCtrl       m_PasswordED;
    wxTextCtrl       m_SMTPED;
    wxTextCtrl       m_AddressED;
    wxOwnerDrawnComboBox       m_AuthCB;
    wxStaticText     m_SMTPNameLab;
    wxTextCtrl       m_SMTPNameED;
    wxStaticText     m_SMTPPasswLab;
    wxTextCtrl       m_SMTPPasswED;
    wxTextCtrl       m_PathED;
    wxTextCtrl       m_MailTableED;
    wxTextCtrl       m_FileTableED;
    wxTextCtrl       m_MailSchemaED;

#ifdef WINS

    wxOwnerDrawnComboBox       m_DialCB;
    wxStaticText     m_DialNameLab;
    wxTextCtrl       m_DialNameED;
    wxStaticText     m_DialPasswLab;
    wxTextCtrl       m_DialPasswED;

    wxFlexGridSizer *m_Sizer602;
    wxOwnerDrawnComboBox       m_Prof602CB;
    wxStaticText     m_Name602Lab;
    wxTextCtrl       m_Name602ED;
    wxStaticText     m_Passw602Lab;
    wxTextCtrl       m_Passw602ED;

    wxFlexGridSizer *m_SizerMAPI;
    wxOwnerDrawnComboBox       m_ProfMAPICB;
    wxTextCtrl       m_PasswMAPIED;

    HINSTANCE  m_hMail602;
    HINSTANCE  m_hMAPI;
    
#endif

    wxButton   m_OKBut;
    wxButton   m_CancelBut;

    int  GetMailTypes(wxString *MailTypes);
    bool IsSMTP();
    wxFlexGridSizer *GetSizerSMTP();
    void OnAuthChange(wxCommandEvent &event);
    void OnTextChanged(wxCommandEvent &event);
    void OnPasswordChanged(wxCommandEvent &event);
    void OnsPasswordChanged(wxCommandEvent &event);
    void OnOK(wxCommandEvent &event);
    void AuthChange(TSMTPAuth Auth, const wxString &SMTPName);
    void ShowAuth();

#ifdef WINS

#ifdef MAIL602
    bool Is602();
#endif
    bool IsMAPI();

    void OnMailTypeChange(wxCommandEvent &event);
    void OnComboChanged(wxCommandEvent &event);
    void OndPasswordChanged(wxCommandEvent &event);
    void OnDialChanged(wxCommandEvent &event);
    void OnProf602Changed(wxCommandEvent &event);
    void OnProf602Changed(const wxString &Profile, const wxString &User);
    int  Is602NetLan(const char *Profile);
    wxFlexGridSizer *GetSizer602();
    wxFlexGridSizer *GetSizerMAPI();

#endif

    void OnSize(wxSizeEvent &event);
    void OnHelpClick( wxCommandEvent& event );

public:

    CMailProfDlg()
    {
        TypeSMTP        = -1;
        Type602         = -1;
        TypeMAPI        = -1;
        m_SizerSMTP     = NULL;

#ifdef WINS
        m_Sizer602      = NULL;
        m_SizerMAPI     = NULL;
#endif
    }
    void Create(wxWindow *Parent, const wxString &ProfileName);
    //virtual void OnInternalIdle();

    DECLARE_EVENT_TABLE()
};

#endif  // _MAILPROFDLG_H_
