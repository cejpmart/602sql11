// support.cpp
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
#include "regstr.h"
#include "controlpanel.h"
#include "yesnoalldlg.cpp"
#include "xmldsgn.h"
 
#ifdef _DEBUG
#define new DEBUG_NEW
#endif 

#include "bmps/File_save.xpm"
#include "bmps/_cut.xpm"
#include "bmps/Redo.xpm"
#include "bmps/Undo.xpm"
#include "bmps/run2.xpm"
#include "bmps/compile.xpm"
#include "bmps/cut.xpm"
#include "bmps/validateSQL.xpm"
#include "bmps/help.xpm"
#include "bmps/DeleteCol.xpm"
#include "bmps/InsertCol.xpm"
#include "bmps/tableProperties.xpm"
#include "bmps/showSQLtext.xpm"
#include "bmps/empty.xpm"
#include "bmps/trigwiz.xpm"
#include "bmps/exit.xpm"

#include "xrc/SignErrorDlg.cpp"

wxString int2wxstr(int val)
{ wxString str;
  str.Printf(wxT("%d"), val);
  return str;
}

bool check_xml_library(void)
{
  uns32 ver = link_XMLGetVersion();
  if (ver==0)
  { error_box(_("The 602XML library for 602SQL is not available."));
    return false;
  }
  if (ver!=256*(256*(256*VERS_1+VERS_2)+VERS_3)+VERS_4)
  { error_box(_("The 602XML library for 602SQL has a wrong version."));
    return false;
  }
  return true;
}

void check_open_transaction(cdp_t cdp, wxWindow * parent)
// Checks if the tranaction in cdp is open. If it is, warn and optionally closes it.
{ BOOL transaction_open;
  cd_Transaction_open(cdp, &transaction_open);  // may return TRUE, but [transaction_open] is valid anyway
  if (!transaction_open) return;
 // warn and ask: 
  if (yesno_box(_("The execution left an open transaction. Do you want to close it?"), parent))
    cd_Commit(cdp);
}
///////////////////////////////////// IsCtrlDown //////////////////////////////////////
#ifdef LINUX
#if wxMAJOR_VERSION==2 && (wxMINOR_VERSION<5 || wxMINOR_VERSION==5 && wxRELEASE_NUMBER<=1)
bool wxGetKeyState(wxKeyCode key)
{ return false; }
#endif
#endif

bool IsCtrlDown(void)
{ 
#ifdef WINS  // error in MSW impementation of wxGetKeyState  
  bool bVirtual;
  WORD vkey = wxCharCodeWXToMSW(WXK_CONTROL, &bVirtual);
  SHORT state = GetAsyncKeyState(vkey);
  return (state & 0x8000) != 0;
#else
  return wxGetKeyState(WXK_CONTROL); 
#endif
}

bool IsShiftDown(void)
{ 
#ifdef WINS  // error in MSW impementation of wxGetKeyState  
  bool bVirtual;
  WORD vkey = wxCharCodeWXToMSW(WXK_SHIFT, &bVirtual);
  SHORT state = GetAsyncKeyState(vkey);
  return (state & 0x8000) != 0;
#else
  return wxGetKeyState(WXK_SHIFT); 
#endif
}
////////////////////////////////////// boxes /////////////////////////////////
int wx602MessageBox(const wxString& message, const wxString& caption, long style, wxWindow *parent)
{   long decorated_style = style;
    if ( ( style & ( wxICON_EXCLAMATION | wxICON_HAND | wxICON_INFORMATION | wxICON_QUESTION ) ) == 0 )
        decorated_style |= ( style & wxYES ) ? wxICON_QUESTION : wxICON_INFORMATION ;
    wx602MessageDialog dialog(parent, message, caption, decorated_style);
    if ((style & wxYES) && (style & wxNO) && !(style & wxCANCEL))
      dialog.SetEscapeId(wxID_NO);  // prevents returning wxID_YES when ESC is pressed and Cancel button does not exist
    int ans = dialog.ShowModal();
    switch ( ans )
    {
        case wxID_OK:
            return wxOK;
        case wxID_YES:
            return wxYES;
        case wxID_NO:
            return wxNO;
        case wxID_CANCEL:
            return wxCANCEL;
    }
    return wxCANCEL;
}


void unpos_box(wxString message, wxWindow * parent)
{ wx602MessageBox(message, STR_IMPOSSIBLE_CAPTION, wxICON_ERROR | wxOK, parent); }

void error_box(wxString message, wxWindow * parent)
{ wx602MessageBox(message, STR_ERROR_CAPTION, wxICON_ERROR | wxOK, parent); }

void error_boxp(wxString message, wxWindow * parent, ...)
{ wx602MessageBox(wxString::FormatV(message, (va_list)((char *)&parent + sizeof(wxWindow *))), STR_ERROR_CAPTION, wxICON_ERROR | wxOK, parent); }

void info_box(wxString message, wxWindow * parent)
{ wx602MessageBox(message, STR_INFO_CAPTION, wxICON_INFORMATION | wxOK, parent); }

bool yesno_box(wxString message, wxWindow * parent)
{ return wx602MessageBox(message, STR_WARNING_CAPTION, wxICON_QUESTION | wxNO_DEFAULT | wxYES_NO, parent) == wxYES; }

bool yesno_boxp(wxString message, wxWindow * parent, ...)
{ return wx602MessageBox(wxString::FormatV(message, (va_list)((char *)&parent + sizeof(wxWindow *))), STR_WARNING_CAPTION, wxICON_QUESTION | wxNO_DEFAULT  | wxYES_NO, parent) == wxYES; }

int yesnoall_box(wxString allbut, wxString message, wxWindow * parent)
{ CYesNoAllDlg Dlg(parent, message, allbut);
  return Dlg.ShowModal();
}

int yesnoall_boxp(wxString allbut, wxString message, wxWindow * parent, ...)
{ CYesNoAllDlg Dlg(parent, wxString::FormatV(message, (va_list)((char *)&parent + sizeof(wxWindow *))), allbut);
  return Dlg.ShowModal();
}

void connection_error_box(cdp_t cdp, int errnum)
{ 
  if (errnum!=KSE_OK)
  { if (errnum > KSE_LAST) errnum=KSE_LAST;
    wx602MessageBox(Get_error_num_textWX(cdp, errnum), _("Connection error"), wxICON_ERROR | wxOK, wxGetApp().frame);
  }
}

//
// Vraci chybove hlaseni podle formatu Fmt s parametry ..., navic placeholder %ErrNO nahradi kodem posledni systemove chyby
// a %ErrMsg nahradi odpovidajici textvou zpravou.
//
wxString SysErrorMsg(const wxChar *Fmt, ...)
{
    wxString rFmt(Fmt);
    if (rFmt.Find(wxT("%ErrNO")) != -1)
    {
        wxString ErrNO;
        long Code = wxSysErrorCode();
        if (Code >= 0)
            ErrNO = wxString::Format(wxT("%d"),   Code);
        else
            ErrNO = wxString::Format(wxT("%08X"), Code);
        rFmt.Replace(wxT("%ErrNO"), ErrNO);
    }
    if (rFmt.Find(wxT("%ErrMsg")) != -1)
        rFmt.Replace(wxT("%ErrMsg"), wxSysErrorMsg());
    return wxString::FormatV(rFmt, (va_list)((char *)&Fmt + sizeof(const wxChar *)));
}
//
// To same pro chyby 602SQL
//
wxString SQL602ErrorMsg(cdp_t cdp, const wxChar *Fmt, ...)
{
    long Code = cd_Sz_error(cdp);
    wxString rFmt(Fmt);
    if (rFmt.Find(wxT("%ErrNO")) != -1)
    {
        wxString ErrNO;
        if (Code >= 0)
            ErrNO = wxString::Format(wxT("%d"),   Code);
        else
            ErrNO = wxString::Format(wxT("%08X"), Code);
        rFmt.Replace(wxT("%ErrNO"), ErrNO);
    }
    if (rFmt.Find(wxT("%ErrMsg")) != -1)
      rFmt.Replace(wxT("%ErrMsg"), Get_error_num_textWX(cdp, Code));
    return wxString::FormatV(rFmt, (va_list)((char *)&Fmt + sizeof(const wxChar *)));
}

int errors_with_topics[] = { 128, 129, 133, 136, 137, 160, 162, 169, 172, 174, 175, 178, 180, 213, 227,
                             230, 233, 234, 238, 0 };

CFNC int find_help_topic_for_error(int err)
// Returns the number of the help page describing the error or 0 if such page does not exist
{ int * p = errors_with_topics;
  while (*p && *p!=err) p++;
  if (*p) return 10000+err;
  else return 0;
}

#include "compil.h"
CFNC BOOL WINAPI report_last_error(cdp_t cdp, wxWindow * parent)
{ wxString buf;
  int err=cd_Sz_error(cdp);
  if (err == INTERNAL_SIGNAL) return FALSE;
  if (wxThread::IsMain())  // ignore errors in other threads
  { buf=Get_error_num_textWX(cdp, err);
    if (buf.IsEmpty()) return FALSE;
   /* find the error type: */
    if (err >= FIRST_CLIENT_ERROR) // includes generic error
      wx602MessageBox(buf, STR_ERROR_CAPTION,            wxICON_ERROR | wxOK, parent);
    else if (err >= FIRST_RT_ERROR)
      wx602MessageBox(buf, _("Program Execution Error"), wxICON_ERROR | wxOK, parent);
    else if (err >= FIRST_COMP_ERROR)
      wx602MessageBox(buf, _("Syntax Error"),            wxICON_ERROR | wxOK, parent);
    else
    { SignErrorDlg dlg(_("SQL Server Error"), buf, find_help_topic_for_error(err) ? err : 0);
      dlg.Create(parent);
      dlg.ShowModal();
    }
  }
  return TRUE;
}

CFNC BOOL WINAPI cd_Signalize(cdp_t cdp)
{ return report_last_error(cdp, wxGetActiveWindow()/*wxGetApp().frame*/); }

CFNC bool WINAPI cd_Signalize2(cdp_t cdp, wxWindow * parent)
{ return report_last_error(cdp, parent)==TRUE; }

CFNC void WINAPI syserr(int operat)  // ###
{
}

CFNC void WINAPI no_memory(void)
{ wx602MessageBox(_("Not enough memory."), _("Client error"), wxICON_ERROR | wxOK, wxGetApp().frame); }

void no_memory2(wxWindow * parent)
{ wx602MessageBox(_("Not enough memory."), _("Client error"), wxICON_ERROR | wxOK, parent); }

CFNC tptr WINAPI sigalloc(unsigned size, uns8 owner)
{ tptr p;
  p=(tptr)corealloc(size,owner);
  if (!p) no_memory();
  return p;
}

#if 0  // replaced by ToASCII
uns8 bez_outcode[128]={
' ',' ',' ',' ',' ',' ',' ',' ',' ',' ','S',' ','S','T','Z','Z',
' ',' ',' ',' ',' ',' ',' ',' ',' ',' ','s',' ','s','t','z','z',
' ',' ',' ','L',' ','A',' ',' ',' ',' ','S',' ',' ',' ',' ','Z',
' ',' ',' ','l',' ',' ',' ',' ',' ','a','s',' ','L',' ','l','z',
'R','A','A','A','A','L','C','C','C','E','E','E','E','I','I','D',
'D','N','N','O','O','O','O',' ','R','U','U','U','U','Y','T',' ',
'r','a','a','a','a','l','c','c','c','e','e','e','e','i','i','d',
'd','n','n','o','o','o','o',' ','r','u','u','u','u','y','t',' '};

CFNC void WINAPI remove_diacrit(char * str, int len)
{ if (len<0) len=strlen(str);
  for (int i=0;  i<len;  i++)
    if ((unsigned char)str[i]>=128)
      str[i]=bez_outcode[(unsigned char)str[i]-128];
    else if (str[i]<'0' || str[i]>'9' && str[i]<'A' ||
             str[i]>'Z' && str[i]<'a' || str[i]>'z')
      str[i]='_';
}
#endif

///////////////////////// entering a name ///////////////////////////////////////
#include "xrc/EnterObjectNameDlg.cpp"

bool convertible_name(cdp_t cdp, wxWindow * parent, wxString name)
{ 
  if (cdp)
  { wxCharBuffer cbuf = name.mb_str(*cdp->sys_conv);
    if ((const char *)cbuf && *(const char *)cbuf) 
    { if (cdp->sys_charset!=0) return true;
     // special test for ASCII because sys_conv is ISO-8859-1 in this case:
      int i=0;
      do
      { if ((unsigned char)((const char *)cbuf)[i] >= 0x80) break;  // non-ASCII character
        if (!((const char *)cbuf)[i]) return true;
        i++;
      } while (true);
    }
    error_box(_("This name is not convertible to the SQL Server charset."), parent);
  }
  else
  { wxCharBuffer cbuf = name.mb_str(*wxConvCurrent);
    if ((const char *)cbuf && *(const char *)cbuf) return true;
    error_box(_("This name is not convertible to the local system charset."), parent);
  }
  return false;
}

bool enter_object_name(cdp_t cdp, wxWindow * parent, wxString prompt, char * name, bool renaming, tcateg categ)
// Returns true if user entered a valid object name into [name].
// Returns false if user cancelled entering the name.
// cdp may ne NULL when entering the server name.
{ EnterObjectNameDlg dlg(cdp, prompt, renaming, categ==CATEG_INFO ? 20 : OBJ_NAME_LEN);
  dlg.name = wxString(name, GetConv(cdp));
  dlg.Create(parent);
  if (dlg.ShowModal() != wxID_OK) return false;
  // is_object_name() called inside.
  if (cdp)
    strcpy(name, dlg.name.mb_str(*cdp->sys_conv)); // length tested inside
  else
    strcpy(name, dlg.name.mb_str(*wxConvCurrent)); // length tested inside
  return true;
}

bool is_object_name(cdp_t cdp, const wxChar * name, wxWindow * parent)
// When cdp==NULL the [name] must be universal, otherwise must be convertible to the cdp->sys_conv.
{ if (wcslen((const wchar_t *)name) > OBJ_NAME_LEN)
    { unpos_box(_("The object name is too long (31 characters max)."), parent);  return false; }
  int i=0;
  while (name[i])
  { if (name[i]=='`')  // must not contain this char
      { unpos_box(_("The object name cannot contain the grave accent character (`)."), parent);  return false; }
    i++;
  }
  return convertible_name(cdp, parent, name);
}

tobjnmvldt check_object_name(const char * name)
{ if (!name || !*name)
    return onInvalid;
  int len = (int)strlen(name);
  if (len > OBJ_NAME_LEN)
    return onTooLong;
  if (strchr(name, '`'))
    return onInvalid;
  char c = *name;
  if (c != '_' && (c < 'A' || c > 'Z') && (c < 'a' || c > 'z') && (unsigned char)c < 0x80)
    return onSuspicious;
  while ((c = *(++name)))
  { if (c != '_' && (c < '0' || c > '9') && (c < 'A' || c > 'Z') && (c < 'a' || c > 'z') && (unsigned char)c < 0x80)
      return onSuspicious;
  }
  return onValid;
}

bool is_valid_object_name(cdp_t cdp, const char * name, wxWindow *parent)
{
    tobjnmvldt vldt = check_object_name(name);
    if (vldt == onValid)
        return true;
    if (vldt == onSuspicious) 
        return yesno_boxp(_("The name \"%s\" contains characters that may cause problems in SQL. Are you sure you want to use this name?"), parent, TowxChar(name, &GetConv(cdp)));
    error_boxp(vldt == onInvalid ? _("The identifier \"%s\" is not valid. Please, enter another name.") : 
                                   _("The identifier \"%s\" is too long. Please, enter another name."), parent, TowxChar(name, &GetConv(cdp)));
    return false;
}
//
// Pokud Ident zacina na _ nebo pismeno a obsahuje jen _, pismena a cislice 
// nebo pokud je Ident uzavren do ``,
// vraci true jinak false
//
bool IsProperIdent(const wxChar *Ident)
{
    if (*Ident == '`')
    {
        const wxChar *pe = wxStrchr(Ident + 1, '`');
        return pe && pe[1] == 0;
    }
    wxChar c = *Ident;
    if (c != wxT('_') && (c < wxT('A') || c > wxT('Z')) && (c < wxT('a') || c > wxT('z')) && c < 0x80)
        return false;
    const wxChar *p = Ident;
    while ((c = *(++p)))
    { 
        if (c != wxT('_') && (c < wxT('0') || c > wxT('9')) && (c < wxT('A') || c > wxT('Z')) && (c < wxT('a') || c > wxT('z')) && c < 0x80)
            return false;
    }
    char aIdent[256];
    wxConvCurrent->WC2MB(aIdent, (const wchar_t *)Ident, sizeof(aIdent));
    if (check_atr_name(aIdent))
        return false;
    return true;
}
//
// Je-li treba uzavre ident do ``
//
wxString GetProperIdent(wxString Ident, wchar_t qc)
{
    if (!IsProperIdent(Ident))
        Ident = (wxChar)qc + Ident + (wxChar)qc;
    return Ident;
}

wxString GetNakedIdent(wxString Ident)
{
    const wxChar *ps = Ident.c_str();
    const wxChar *pe;
    if (*ps == '`' && (pe = wxStrchr(ps + 1, '`')) && pe[1] == 0)
        return Ident.Mid(1, pe - ps - 1);
    return Ident;
}

bool get_name_for_new_object(cdp_t cdp, wxWindow * parent, const char * schema_name, tcateg categ, wxString prompt, char * name, bool renaming)
{ tobjnum auxobj;
  while (enter_object_name(cdp, parent, prompt, name, renaming, categ))
  { if (categ==CATEG_CURSOR || categ==CATEG_TABLE)   /* check for table/query with the same name */
    { if (!cd_Find_prefixed_object(cdp, schema_name, name, categ==CATEG_CURSOR ? CATEG_TABLE : CATEG_CURSOR, &auxobj))
        { unpos_box(TABLE_QUERY_CONFLICT);  continue; }
    }
    else if (categ==CATEG_ROLE && *name>='0' && *name<='9')  // silently disable
      continue;  // roles starting with a number would disappear
   // try to find the object:
    if (cd_Find_prefixed_object(cdp, schema_name, name, categ, &auxobj)) return true;  // OK
    unpos_box(OBJ_NAME_USED);
  }
  return false;    /* user cancelled entering name */
}

bool edit_name_and_create_object(cdp_t cdp, wxWindow * parent, const char * schema_name, tcateg categ, 
                                 wxString prompt, tobjnum * objnum, char * name)
// [schema_name] may ne NULL for CATEG_APPL, CATEG_GROUP, CATEG_USER etc.
{ *objnum=NOOBJECT;  
  while (enter_object_name(cdp, parent, prompt, name, false, categ))
  { if (categ==CATEG_CURSOR || categ==CATEG_TABLE)   /* check for table/query with the same name */
    { tobjnum auxobj;
      if (!cd_Find_prefixed_object(cdp, schema_name, name, categ==CATEG_CURSOR ? CATEG_TABLE : CATEG_CURSOR, &auxobj))
        { unpos_box(TABLE_QUERY_CONFLICT);  continue; }
    }
    else if (categ==CATEG_ROLE && *name>='0' && *name<='9')  // silently disable
      continue;  // roles starting with a number would disappear
   // try to insert the object:
    { t_temp_appl_context tac(cdp, schema_name);
      if (!cd_Insert_object(cdp, name, categ, objnum)) return true;  // OK
      if (cd_Sz_error(cdp)==KEY_DUPLICITY) unpos_box(OBJ_NAME_USED);
      else cd_Signalize(cdp);
    }
  }
  return false;    /* user cancelled entering name */
}

bool check_not_encrypted_object(cdp_t cdp, tcateg categ, tobjnum objnum)
{ if (not_encrypted_object(cdp, categ, objnum)) return true;
  client_error(cdp, APPL_IS_LOCKED);  cd_Signalize(cdp);
  return false;
}
///////////////////////////////// connection //////////////////////////////////////
int connected_count = 0;

const wxChar * wx_charset_name(int charset)
{ if (charset & 0x80)
  { charset &= 0x7f;
    return charset==1 || charset==2 ? wxT("iso-8859-2") : wxT("iso-8859-1");
  }	
  else
    return charset==0 ? wxT("iso-8859-1") : // the problem is that "US-ASCII" or "windows-437" are not fully supported by WX
           charset==1 || charset==2 ? wxT("windows-1250") : wxT("windows-1252");
}

bool make_sys_conv(cdp_t cdp)
{ 
  //if (cdp->sys_charset==0)
  //  cdp->sys_conv = new wxCSConv( wxT("windows-1250"));  // trying to prevent crashes of the client on national characters
  //else
  // The above is wrong - when national characters are used in ASCII system charset, they cannot be properly converted into it and errors follow
    cdp->sys_conv = new wxCSConv(wx_charset_name(cdp->sys_charset));
  return cdp->sys_conv!=NULL;
}

void wx_disconnect(cdp_t cdp)
{ if (cdp->sys_conv)
    { delete cdp->sys_conv;  cdp->sys_conv=NULL; }
#ifdef _DEBUG
  char buf[100];
  sprintf(buf, "Disconnecting client %d.", cdp->applnum);
  wxLogDebug(wxString(buf, *wxConvCurrent));
#endif  
  cd_disconnect(cdp);
  if (connected_count)
  { connected_count--;
    if (!connected_count) if (browse_enabled) BrowseStart(0);
  }
}

cdp_t clone_connection(cdp_t cdp)
{ cdp_t cdp2 = new cd_t;
  if (cdp2)
  { cdp_init(cdp2);
    int err = cd_connect(cdp2, cdp->locid_server_name, 0);
    //cdp2->translation_callback=cdp->translation_callback;
    if (err == KSE_OK)
    { make_sys_conv(cdp2);
      if (!connected_count)
        if (browse_enabled) BrowseStop(0);
      connected_count++;
#ifdef _DEBUG
  char buf[100];
  sprintf(buf, "Clone connecting client %d.", cdp2->applnum);
  wxLogDebug(wxString(buf, *wxConvCurrent));
#endif  
      if (!cd_Login(cdp2, "*NETWORK", "") || !cd_Login(cdp2, "", ""))
      {// set connection parameters like in the original connection: reporting, schema, SQL options
        cd_Set_progress_report_modulus(cdp2, notification_modulus);
        if (!cd_Set_application(cdp2, cdp->sel_appl_name))
        { cd_SQL_execute(cdp2, "CALL _SQP_SET_THREAD_NAME('debugged')", NULL);  
          return cdp2;
        }  
        uns32 optval;
        if (!cd_Get_sql_option(cdp, &optval))
          cd_Set_sql_option(cdp2, (uns32)-1, optval);
      }
      cd_Signalize(cdp2);
      wx_disconnect(cdp2);
    }
    else
      connection_error_box(cdp2, err);
    delete cdp2;
  }
  return NULL;
}

/////////////////////////////////// t_temp_appl_context ///////////////////////////////////////////////
t_temp_appl_context::t_temp_appl_context(cdp_t designer_cdp, const char * designer_schema, int extent)
// Sets the designer_schema as the current schema until destruction.
// No action if [designer_schema] is NULL or it is the current schema.
{ err=false;
  if (designer_schema==NULL || !wb_stricmp(designer_cdp, designer_schema, designer_cdp->sel_appl_name))
    schema_changed=false;  // same schema, no action on entry and no action on exit
  else
  { cdp=designer_cdp;
    strcpy(orig_schema, cdp->sel_appl_name);
    if (cd_Set_application_ex(cdp, designer_schema, extent))  // do not load the object lists if [extent]==2
      err=true;
    schema_changed=true;
  }
}

t_temp_appl_context::~t_temp_appl_context(void)
// Restores the original current schmema.
{ if (schema_changed)
    cd_Set_application_ex(cdp, orig_schema, 1);  // full reload of object lists
}

///////////////////////////////// INI ///////////////////////////////////////////////
static char * _ini_file_name = NULL;

void drop_ini_file_name(void)
{ delete [] _ini_file_name;  _ini_file_name=NULL; }

#ifdef LINUX

const char * ini_file_name(void)
{ if (!_ini_file_name)
  { const char * home = getenv("HOME");
    if (!home) home = "/tmp";
    _ini_file_name = new char[strlen(home)+10];
    strcpy(_ini_file_name, home);
    strcat(_ini_file_name, "/.602sql");
  }
  return _ini_file_name;
}

#else  // MSW

const char * ini_file_name(void)
{ if (!_ini_file_name)
  { HKEY hKey;  char buf[MAX_PATH];
    *buf=0;
    if (RegOpenKeyExA(HKEY_CURRENT_USER, "Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders", 0, KEY_READ_EX, &hKey)==ERROR_SUCCESS)
    { DWORD key_len=sizeof(buf);
      if (RegQueryValueExA(hKey, "AppData", NULL, NULL, (BYTE*)buf, &key_len)==ERROR_SUCCESS)
      { strcat(buf, "\\Software602");
        CreateDirectoryA(buf, NULL);
        strcat(buf, "\\602SQL");
        CreateDirectoryA(buf, NULL);
        strcat(buf, "\\");
      }
      RegCloseKey(hKey);
    }
    strcat(buf, "602sql.ini");
    _ini_file_name = new char[strlen(buf)+1];
    strcpy(_ini_file_name, buf);
  }
  return _ini_file_name;
}

#endif

bool read_profile_string(const char * section, const char * entry, char * buf, int bufsize)
{ GetPrivateProfileStringA(section, entry, "\1", buf, bufsize, ini_file_name());
  return *buf!='\1';
}

wxString read_profile_string(const char * section, const char * entry, const char * Default)
{
    char Result[256];
    GetPrivateProfileStringA(section, entry, Default, Result, sizeof(Result), ini_file_name());
    return wxString(Result, *wxConvCurrent);
}

bool write_profile_string(const char * section, const char * entry, const char * buf)
{ return WritePrivateProfileStringA(section, entry, buf, ini_file_name()) != 0; }

bool read_profile_int(const char * section, const char * entry, int * val)
{ char buf[20];
  if (!read_profile_string(section, entry, buf, sizeof(buf))) 
    return false;
  return sscanf(buf, " %d", val) == 1;
}

int read_profile_int(const char * section, const char * entry, int Default)
{ char buf[20];
  if (!read_profile_string(section, entry, buf, sizeof(buf))) 
    return Default;
  int val;
  if (sscanf(buf, " %d", &val) == 1)
    return val;
  else
    return Default;
}

bool read_profile_bool(const char * section, const char * entry, bool Default)
{ char buf[20];
  if (!read_profile_string(section, entry, buf, sizeof(buf))) 
    return Default;
  int val;
  if (sscanf(buf, " %d", &val) == 1)
    return val != 0;
  else
    return Default;
}

bool write_profile_int(const char * section, const char * entry, int val)
{ char buf[20];
  sprintf(buf, "%d", val);
  return write_profile_string(section, entry, buf);
}

void write_profile_bool(const char * section, const char * entry, bool val)
{ 
    write_profile_string(section, entry, val ? "1" : "0");
}

#ifdef LINUX
// Function WritePrivateProfileString seems to be damaged somewhere in some library, using own version instead.
// WritePrivateProfileString does not remove the section contents when removing the section.
#include <errno.h>

BOOL WINAPI xWritePrivateProfileString(const char * lpszSection,  const char * lpszEntry, const char * lpszString, const char * lpszFilename)
// Deletes the entry iff lpszString==NULL
{ char line[1024];  FILE * fi, * fo;  BOOL my_section=FALSE, added=FALSE;
  char newname[160];  int hnd;
#ifdef IN_PLACE_TEMP  // problems when user has no rights to create files in the directory
  strcpy(newname, lpszFilename);
  strcpy(newname+strlen(newname)-3, "NEW");
#else
  strcpy(newname, "/tmp/602sqlXXXXXX");
  hnd=mkstemp(newname);
  if (hnd==-1) { perror(newname); return FALSE; }
  close(hnd);
#endif
  fi=fopen(lpszFilename, "rb");
  if (!fi && errno!=ENOENT)
    { perror(lpszFilename);  return FALSE; }
  fo=fopen(newname, "wb");
  if (!fo) { perror(newname); if (fi) fclose(fi);  return FALSE; }
  my_section=FALSE;
  if (fi)
  { while (fgets(line, sizeof(line), fi))
    { int l;
      cutspaces(line);
      l=strlen(line);
      while (l && ((line[l-1]=='\r') || (line[l-1]=='\n'))) l--;
      line[l]=0;
    // analyse the clear line:
      if (line[0]=='[')
      { if (my_section && !added && lpszEntry)    // end of my section, add it  // V.B. Ruseni cele sekce, kdyz lpszEntry == NULL
        { if (lpszString)
            { fputs(lpszEntry, fo);  fputs("=", fo);  fputs(lpszString, fo);  fputs("\n", fo); }
          added=TRUE;
        }
        if ((l>2) && (line[l-1]==']'))
        { line[l-1]=0;
          my_section=!strcasecmp(line+1, lpszSection);
          line[l-1]=']';
        }
        else my_section=FALSE;
        if (!my_section || lpszEntry)             // V.B.
          fputs(line, fo);  fputs("\n", fo);
      }
      else if (my_section)           // V.B.
      { if (lpszEntry) 
        { char * p=line;  const char * q=lpszEntry;
          while (*p && (upcase_charA(*p, 0) == upcase_charA(*q, 0))) { p++; q++; }
          while (*p==' ') p++;
          if (!*q && *p=='=')  // replace the line
          { if (lpszString)
            { fputs(lpszEntry, fo);  fputs("=", fo);  fputs(lpszString, fo);  fputs("\n", fo); }
            added=TRUE;
          }
          else // other entry from my section
          { fputs(line, fo);  fputs("\n", fo); }
        }
      }
      else  // entry from other section
        { fputs(line, fo);  fputs("\n", fo); }
    }
    fclose(fi);
  }
  if (!added && lpszEntry && lpszString)                     // V.B.
  { if (!my_section)
   	  { fputs("[", fo);   fputs(lpszSection, fo);  fputs("]\n", fo); }
    fputs(lpszEntry, fo);  fputs("=", fo);  fputs(lpszString, fo);
    fputs("\n", fo);
  }
  fclose(fo);
#ifdef IN_PLACE_TEMP
  if (unlink(lpszFilename)) return FALSE;
  if (rename(newname, lpszFilename)) return FALSE;
  return TRUE;
#else
  // works for different filesystems, too
  { int hFrom, hTo;  BOOL ok=TRUE;
    hFrom=open(newname, O_RDONLY);
    if (hFrom==-1) ok=FALSE;
    else
    { hTo=open(lpszFilename, O_WRONLY|O_TRUNC|O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
      if (hTo==-1) { perror(lpszFilename);  ok=FALSE; }
      else
      { char buf[512];  int rd;
        do
        { rd=read(hFrom, buf, sizeof(buf));
          if (rd==-1) { ok=FALSE;  break; }
          if (!rd) break;
          if (write(hTo, buf, rd) != rd) { ok=FALSE;  break; }
        } while (true);
        close(hTo);
      }
      close(hFrom);
      if (ok) unlink(newname);
    }
    return ok;
  }
#endif
}
#endif
//////////////////////////////////// enumerators //////////////////////////////////////
struct t_object_enumerator
{ cdp_t cdp;
  bool enum_roles, extended;
 // for cached objects:
  t_dynar * d_comp;  
  int index;
 // for connections:
  t_connection * curr_conn;
 // for listed categories
  char * start, * curr;
  // current folder
  int curr_folder;
  t_object_enumerator(void)
    { d_comp=NULL;  curr_conn=NULL;  start=curr=NULL; curr_folder = -1;}
  ~t_object_enumerator(void)
    { if (start) corefree(start); }
  void init(cdp_t cdpIn, tcateg categ, const char *schema, int folder = -1);
  BOOL object_enumerator_next(char * name, tobjnum & objnum, int & flags, uns32 & modif_timestamp, int & folder);
};

//////////////////////////////////// enumerators //////////////////////////////////////
void t_object_enumerator::init(cdp_t cdpIn, tcateg categ, const char * schema, int folder)
// !schema or !*schema means the current schema.
{ cdp=cdpIn;
  enum_roles=categ==CATEG_ROLE;
  curr_folder = folder;
  if (!schema || !*schema || !wb_stricmp(cdp, schema, cdp->sel_appl_name))
  { d_comp=get_object_cache(cdp, categ);
    if (d_comp) { index=0;  return; }
#ifdef CLIENT_ODBC
    if (categ==CATEG_CONNECTION)
    { if (!cdp->odbc.hEnv) curr_conn = NULL;
      curr_conn = cdp->odbc.conns;
      return;
    }
#endif
  }
 // not cached, make list (front a nd back-end redirection not supported here!!):
  WBUUID appl_id;
  if (schema && *schema) cd_Apl_name2id(cdp, schema, appl_id);
  extended = !(categ==CATEG_USER || categ==CATEG_GROUP || categ==CATEG_SERVER);
  if (cd_List_objects(cdp, !extended ? categ : categ|IS_LINK, schema && *schema ? appl_id : NULL, &start)) start=NULL;
  else curr=start;
}

BOOL t_object_enumerator::object_enumerator_next(char * name, tobjnum & objnum, int & flags, uns32 & modif_timestamp, int & folder)
{ if (d_comp)
  { if (index>=d_comp->count()) return FALSE;
    for (;;)
    { t_comp * acomp = (t_comp*)d_comp->acc0(index++);
      if (curr_folder != -1 && curr_folder != acomp->folder)
        continue;
      strcpy(name, acomp->name);  objnum=acomp->objnum;  flags=acomp->flags;  modif_timestamp=acomp->modif_timestamp;
      folder=acomp->folder;
      return TRUE;
    }
  } 
#ifdef CLIENT_ODBC
  if (curr_conn)
  { strcpy(name, curr_conn->dsn);  objnum=curr_conn->objnum;  flags=curr_conn->hStmt ? 0 : 1;
    return TRUE;
  } 
#endif
  if (start)
  { do
    { if (!*curr) return FALSE;  
      strcpy(name, curr);  curr+=strlen(curr)+1;
      if (extended)
      { folder = search_or_add_folder(cdp, curr, NOOBJECT);  curr+=strlen(curr)+1;
        modif_timestamp=*(uns32*)curr;  curr+=sizeof(uns32);
      }
      else folder=0;

      objnum=*(tobjnum*)curr;   curr+=sizeof(tobjnum);
      flags=*(uns16*)curr;  curr+=sizeof(uns16);
      if (curr_folder != -1 && curr_folder != folder)
        continue;
      if (*name != TEMP_TABLE_MARK)
        if (!enum_roles || *name<'0' || *name>'9')
          return TRUE;
    } while (true);
  }
  return FALSE;
}

void * get_object_enumerator(cdp_t cdp, tcateg categ, const char * schema)
{ t_object_enumerator * en = new t_object_enumerator;
  if (en) en->init(cdp, categ,  schema);
  return en;
}

void free_object_enumerator(void * en)
{ if (en) delete (t_object_enumerator*)en; } 

bool object_enumerator_next(void * en, char * name, tobjnum & objnum, int & flags, uns32 & modif_timestamp, int & folder)
{ if (!en) return false;
  return ((t_object_enumerator*)en)->object_enumerator_next(name, objnum, flags, modif_timestamp, folder) != 0;
}

void move_object_names_to_combo(wxOwnerDrawnComboBox * combo, cdp_t cdp, tcateg category)
{ void * en = get_object_enumerator(cdp, category, NULL);
  if (en)
  { tobjname name;  tobjnum objnum;  int flags;  uns32 modif_timestamp;  int folder;
    while (object_enumerator_next(en, name, objnum, flags, modif_timestamp, folder))
      combo->Append(wxString(name, *cdp->sys_conv), (void*)(size_t)objnum);
    free_object_enumerator(en);
  }
}
///////////////////////////////// object caches //////////////////////////////////
void Get_obj_folder_name(cdp_t cdp, tobjnum objnum, tcateg categ, char * folder_name)
{ if (cd_Read(cdp, categ==CATEG_TABLE ? TAB_TABLENUM : OBJ_TABLENUM, objnum, OBJ_FOLDER_ATR, NULL, folder_name))
    *folder_name=0;
}

bool Set_object_flags(cdp_t cdp, tobjnum objnum, tcateg categ, uns16 flags)
{ return !cd_Write(cdp, categ==CATEG_TABLE ? TAB_TABLENUM : OBJ_TABLENUM, objnum, OBJ_FLAGS_ATR, NULL, &flags, sizeof(uns16)); }

void Change_component_flag(cdp_t cdp, tcateg categ, const char * schema_name, const char * objname, tobjnum objnum, int mask, int setbit)
{// find and update in the cache:
  tobjname uobjname;  strcpy(uobjname, objname);  Upcase9(cdp, uobjname);
  if (!wb_stricmp(cdp, schema_name, cdp->sel_appl_name))
  { t_dynar * d_comp = get_object_cache(cdp, categ);
    if (d_comp!=NULL)  // cached category
    { unsigned pos;
      if (comp_dynar_search(d_comp, uobjname, &pos))
      { t_comp * acomp = (t_comp*)d_comp->acc(pos);
        if (categ!=CATEG_CONNECTION)
          acomp->flags=(acomp->flags & ~mask) | (setbit & mask);
      }
    }
  }
 // update flags in the database:
  uns16 flags;
  cd_Read(cdp, categ==CATEG_TABLE ? TAB_TABLENUM : OBJ_TABLENUM, objnum, OBJ_FLAGS_ATR, NULL, &flags);
  flags=(flags & ~mask) | (setbit & mask);
  Set_object_flags(cdp, objnum, categ, flags);
}

void Changed_component(cdp_t cdp, tcateg categ, const char * schema_name, const char * objname, tobjnum objnum, uns16 flags)
{ uns32 modtime = stamp_now();
 // find and update in the cache:
  if (!wb_stricmp(cdp, schema_name, cdp->sel_appl_name))
  { t_dynar * d_comp = get_object_cache(cdp, categ);
    if (d_comp!=NULL)  // cached category
    { unsigned pos;
      if (comp_dynar_search(d_comp, objname, &pos))
      { t_comp * acomp = (t_comp*)d_comp->acc(pos);
        if (categ!=CATEG_CONNECTION)
        { acomp->flags=flags;
          acomp->modif_timestamp=modtime=stamp_now();
        }
      }
    }
  }
 // update the panel:
  item_info info;
  info.topcat=TOPCAT_SERVERS;
  info.cdp=cdp;  strcpy(info.server_name, cdp->locid_server_name);
  if (cdp)
  { if (categ==CATEG_APPL)
    { strcpy(info.schema_name, schema_name);
      info.syscat=SYSCAT_APPLTOP;
    }
    else
    { if (categ==CATEG_USER || categ==CATEG_GROUP || categ==CATEG_SERVER)
        *info.schema_name=0;
      else
        strcpy(info.schema_name, schema_name);
      info.syscat=SYSCAT_OBJECT;
      strcpy(info.object_name, objname);  
    }
    info.category=categ;  info.objnum=objnum;
    info.flags=flags;  
  }
  else 
    info.syscat=SYSCAT_APPLS;
  update_cp_item(info, modtime);
}

void Add_new_component(cdp_t cdp, tcateg categ, const char * schema_name, const char * objname, tobjnum objnum, uns16 flags, const char * folder_name, bool select_it)
{ uns32 modtime = stamp_now();
 // add to the cache:
  if (!wb_stricmp(cdp, schema_name, cdp->sel_appl_name))
    AddObjectToCache(cdp, objname, objnum, categ, flags, folder_name);
 // add to the panel:
  item_info info;
  info.topcat=TOPCAT_SERVERS;
  info.cdp=cdp;  strcpy(info.server_name, cdp->locid_server_name);
  if (cdp)
  { if (categ==CATEG_APPL)
    { strcpy(info.schema_name, schema_name);
      info.syscat=SYSCAT_APPLTOP;
    }
    else
    { if (categ==CATEG_USER || categ==CATEG_GROUP || categ==CATEG_SERVER)
        *info.schema_name=0;
      else
        strcpy(info.schema_name, schema_name);
      info.syscat=SYSCAT_OBJECT;
      strcpy(info.object_name, objname);
      strcpy(info.folder_name, folder_name);
    }
    info.category=categ;  info.objnum=objnum;
    info.flags=flags;
  }
  else
    info.syscat=SYSCAT_APPLS;
  insert_cp_item(info, modtime, select_it);
}

///////////////////// registry access //////////////////////////////
#include "enumsrv.c"

bool GetDatabaseNum(const char * server_name, const char * value_name, sig32 & val, bool private_server)
{ bool result=true;
#ifdef WINS
  char key[160];  HKEY hKey;
  strcpy(key, WB_inst_key);  strcat(key, Database_str);  strcat(key, "\\");  strcat(key, server_name);
  if (RegOpenKeyExA(private_server ? HKEY_CURRENT_USER : HKEY_LOCAL_MACHINE, key, 0, KEY_READ_EX, &hKey)!=ERROR_SUCCESS)
    result=false;  //   -- connected to an unregistered server
  else
  { DWORD key_len=sizeof(DWORD);
    if (RegQueryValueExA(hKey, value_name, NULL, NULL, (BYTE*)&val, &key_len)!=ERROR_SUCCESS)
      result=false;
    RegCloseKey(hKey);
  }
#else
  result=GetDatabaseString(server_name, value_name, &val, sizeof(val), private_server);
#endif
  return result;
}

bool DeleteDatabaseValue(const char * server_name, const char * value_name, bool private_server)
{
#ifdef WINS
  char key[160];  HKEY hKey;  
  strcpy(key, WB_inst_key);  strcat(key, Database_str);  strcat(key, "\\");  strcat(key, server_name);
  if (RegOpenKeyExA(private_server ? HKEY_CURRENT_USER : HKEY_LOCAL_MACHINE, key, 0, KEY_ALL_ACCESS_EX, &hKey)!=ERROR_SUCCESS)
    return false;
  LRESULT res = RegDeleteValueA(hKey, value_name);
  if (res!=ERROR_SUCCESS && res!=ERROR_FILE_NOT_FOUND)
    { RegCloseKey(hKey);  return false; }
  RegCloseKey(hKey);
  return true;
#else
  const char * cfile = private_server ? locconffile : configfile;
  return xWritePrivateProfileString(server_name, value_name, NULL, cfile) != FALSE;
#endif
}

#ifdef WINS

bool WriteServerNameForService(wxString service_name, wxString server_name, bool add_default_description)
{ HKEY hkey;	bool ret=false;
	wxString key(wxT("SYSTEM\\CurrentControlSet\\Services\\"));  key=key+service_name;
  if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, key.c_str(), 0, KEY_SET_VALUE_EX, &hkey) == ERROR_SUCCESS)
  { if (RegSetValueEx(hkey, wxT("CnfPath"), NULL, REG_SZ, (BYTE*)server_name.c_str(), (int)sizeof(wchar_t)*((int)server_name.Length()+1)) == ERROR_SUCCESS)
      ret=true;
    RegSetValueEx(hkey, wxT("Description"), NULL, REG_SZ, (BYTE*)wxT("602SQL Server"), sizeof(wchar_t)*(13+1));
    RegCloseKey(hkey);
  }
  return ret;
}

#endif

char * append(char * dest, const char * src)  // appends with single '\'
{ int len=(int)strlen(dest);
  if (!len || dest[len-1]!=PATH_SEPARATOR) dest[len++]=PATH_SEPARATOR;
  if (*src==PATH_SEPARATOR) src++;
  strcpy(dest+len, src);
  return dest;
}
////////////////////////////////////// string history ///////////////////////////////////
void string_history::add_to_history(wxString str)
// Inserts [str] as the most recent string into the history.
// If there is an occurrence of the same string in the history, it is removed.
// Otherwise, is the history has the maximal size, the oldest string is removed.
{ if (!strngs) return;  // allocation error
  if (str.IsEmpty()) return;  // not adding
  int i = 0;
  while (i<count && str!=strngs[i]) i++;  
 // now, i is either the same string or it is after the last string
  if (i==size) i--;
  else if (i>=count) count=i+1;  // count updated
 // now, i is either the same string or it is after the last string if there is some space or is is the index of the last string
 // move strings destroying strngs[i]:
  for (int j=i;  j>0;  j--)
    strngs[j]=strngs[j-1];
  strngs[0]=str;
}

const wxChar * string_history::get_from_history(int ind)
{ if (ind<0 || ind >=count || !strngs) return NULL;
  return strngs[ind].c_str();
}

void string_history::fill(wxOwnerDrawnComboBox * combo)
{ combo->Clear();
  int ind = 0;
  do
  { const wxChar * str = get_from_history(ind++);
    if (!str) break;
    combo->Append(str);
  } while (true);
}

// File need not to be on its beginning
#if 0  // not used
wxString load_and_check_tab_def(cdp_t cdp, FHANDLE hFile, char *tabname, int *flags)
{ wxString Result;
  DWORD flen = GetFileSize(hFile, NULL) - SetFilePointer(hFile, 0, NULL, FILE_CURRENT);
  tptr defin = Result.GetWriteBuf(flen);
  if (!defin) { client_error(cdp, OUT_OF_APPL_MEMORY); return Result; }
  DWORD rd;
  if (!ReadFile(hFile, defin, flen, &rd, NULL))
  { client_error(cdp, OS_FILE_ERROR); return Result; }
  Result.UngetWriteBuf(rd);
  table_all ta;
  int res=compile_table_to_all(cdp, Result, &ta);
  if (res)
  { client_error(cdp, res); Result.Empty(); return Result; }
  strcpy(tabname, ta.selfname);
  *flags = ta.tabdef_flags & TFL_ZCR ? CO_FLAG_REPLTAB : 0;
  return Result;
}
#endif

bool CopyFromFile(cdp_t cdp, ttablenum tb, trecnum recnum, tattrib attr, uns16 index, FHANDLE hFile, uns32 offset, int recode)
{ DWORD Len = GetFileSize(hFile, NULL);
  DWORD rd  = Len;
  if (rd > 0x100000)
    rd = 0x100000;
  CBuffer buf;
  if (!buf.Alloc(rd))
  { client_error(cdp, OUT_OF_APPL_MEMORY);
    return false;
  }
  while (Len > 0)
  { if (!ReadFile(hFile, buf, buf.GetSize(), &rd, NULL))
    { client_error(cdp, OS_FILE_ERROR); return false; }
    if (!rd)
      break;
    //if (recode) encode(buf, rd, TRUE, recode);
    if (cd_Write_var(cdp, tb, recnum, attr, index, offset, rd, buf))
        return false;
    if (rd < buf.GetSize())
      break;
    offset += rd;
    Len    -= rd;
  }
  cd_Write_len(cdp, tb, recnum, attr, index, offset);
  return true;
}

///////////////////////// confirm overwrite file //////////////////////////////////////
bool can_overwrite_file(const char * fname, bool * overwrite_all)
/* Returns TRUE if file does not exist or overwrite confirmed. FALSE if cannot overwrite. */
{// check if the overwriting has been allowed before:
  if (overwrite_all!=NULL && *overwrite_all) return true;
 // find the file:
  { WIN32_FIND_DATAA FindFileData;
    HANDLE fh = FindFirstFileA(fname, &FindFileData);
    if (fh==INVALID_HANDLE_VALUE) return true;  // not found
    FindClose(fh);
  }
 // ask:
  wxString msg = wxString(_("File '")) + wxString(fname, *wxConvCurrent) + _("' already exists. Overwrite?");
  if (!overwrite_all) return yesno_box(msg);
  int res = yesnoall_box(_("Yes to all"), msg);
  if (res==wxID_YESTOALL) *overwrite_all=true;
  return res!=wxID_NO;
}

////////////////////////////// menu with bitmaps //////////////////////////////////////
void AppendXpmItem(wxMenu * menu, int id, const wxString& item, char * bitmap_xpm[], bool enb)
{ 
#ifdef STOP  // menus with bitmaps crash W98! in wx262 (before repaired)
  OSVERSIONINFO VersionInfo;
  VersionInfo.dwOSVersionInfoSize = sizeof(VersionInfo);
  if (GetVersionEx(&VersionInfo))
  { if (VersionInfo.dwMajorVersion<4 || VersionInfo.dwMajorVersion==4 /*&& VersionInfo.dwMinorVersion<=10*/)  // W Me, W98 or older
    { menu->Append(id, item);
      if (!enb)
        menu->Enable(id, false);
      return;
    }
  }
#endif
  wxMenuItem * mi;
  mi = new wxMenuItem(menu, id, item);
  mi->SetBitmap(bitmap_xpm);
  menu->Append(mi);
  if (!enb)
    menu->Enable(id, false); 
}

////////////////////////////// checking password length ////////////////////////////////
bool password_short(cdp_t cdp, const char * password)
{ char buf[13];  sig32 value;
  if (!cd_Get_property_value(cdp, "@SQLSERVER", "MinPasswordLen", 0, buf, sizeof(buf)))
    if (str2int(buf, &value))
      if (strlen(password) < value)
      { wxString msg;
        msg.Printf(_("The password is not long enough. It must have at least %u characters."), value);
        error_box(msg);
        return true;
      }
  return false;
}

////////////////////////////// new combo value in event handler ///////////////
int GetNewComboSelection(wxOwnerDrawnComboBox * combo)
// For read-only combo: should return the NEW value in the wxEVT_COMMAND_COMBOBOX_SELECTED event handler.
// For editable combo on Linux: GetSelection return some valid index even is a different value written.
{ wxString selstr = combo->GetValue(); 
  return combo->FindString(selstr);
}

////////////////////////////// wxDateTime conversions ////////////////////////////////

wxDateTime Date2wxDateTime(uns32 Date)
{
    if (Date == NONEDATE)
        return wxInvalidDateTime;
    else
        return wxDateTime((Date % 31) + 1, (Date / 31 % 12) + 1, Date / (12 * 31));
}

wxDateTime Time2wxDateTime(uns32 Time)
{
    if (Time == NONETIME)
        return wxInvalidDateTime;
    else
        return wxDateTime(Time / (60 * 60 * 1000), (Time / (60 * 1000)) % 60, (Time / 1000) % 60, Time % 1000);
}

wxDateTime timestamp2wxDateTime(uns32 timestamp)
{
    if (timestamp == NONETIME)
        return wxInvalidDateTime;
    else
    {
        unsigned dt   = timestamp / (60 * 60 * 24);
        unsigned tm   = timestamp % (60 * 60 * 24);
        unsigned Day  = (dt % 31) + 1;  dt = dt / 31;
        unsigned Mon  = (dt % 12);
        unsigned Year = (dt / 12) + 1900;
        unsigned Hour =  tm / (60 * 60);
        unsigned Min  = (tm % (60 * 60)) / 60;
        unsigned Sec  =  tm % 60;
        return wxDateTime(Day, wxDateTime::Month(Mon), Year, Hour, Min, Sec, 0);
    }
}

uns32 wxDateTime2Date(wxDateTime DateTime)
{
    if (!DateTime.IsValid())
        return NONEDATE;
    else
        return (DateTime.GetYear() * (12 * 31)) + ((DateTime.GetMonth() - 1) * 31) + (DateTime.GetDay() - 1);
}

uns32 wxDateTime2Time(wxDateTime DateTime)
{
    if (!DateTime.IsValid())
        return NONETIME;
    else
        return (DateTime.GetHour() * 60 * 60 * 1000) + (DateTime.GetMinute() * 60 * 1000) + (DateTime.GetSecond() * 1000) + DateTime.GetMillisecond();
}

uns32 wxDateTime2timestamp(wxDateTime DateTime)
{
    if (!DateTime.IsValid())
        return NONETIMESTAMP;
    else
        return ((DateTime.GetYear() - 1900) * (12 * 31 * 24 * 60 * 60)) + ((DateTime.GetMonth() - 1) * (31 * 24 * 60 * 60)) + ((DateTime.GetDay() - 1) * (24 * 60 * 60)) + (DateTime.GetHour() * 60 * 60) + (DateTime.GetMinute() * 60) + DateTime.GetSecond();
}

///////////////////////////////// default grid data formats ///////////////////////
int format_real = -3;
wxString format_decsep = wxT(".");
wxString format_date = wxT("D.M.CCYY");
wxString format_time = wxT("h:mm:ss.fff");
wxString format_timestamp = wxT("D.M.CCYY h:mm:ss.fff");

void get_real_format_info(signed char precision, int *decim, int *form)
{ if (decim!=NULL)
  { int val=precision<0 ? -precision : precision;
    if (val==127) val=0;
    if (val>=100) val-=100;
    *decim=val;
  }
  if (form!=NULL)
  { if (precision==-127 || precision==127) *form=0;
    else if (precision<=-100 || precision>=100) *form=2;
    else *form = precision<0 ? 0 : 1;
  }
}

////////////////////////// GetMetric patch ///////////////////////////////

int PatchedGetMetric(wxSystemMetric index)
{
#ifdef LINUX
  switch (index)
  { case wxSYS_SMALLICON_X:
    case wxSYS_SMALLICON_Y:
      return 16;  // missing in GTK
  }
#endif
  return wxSystemSettings::GetMetric(index);
}

/////////////////////////////////// numeric SetValue ////////////////////////////////////
void SetIntValue(wxTextCtrl * ctrl, int value)
{ wxString buf;
  buf.Printf(wxT("%d"), value);
  ctrl->SetValue(buf);
}

////////////////////////////////// grid /////////////////////////////////////	
bool wxGridCellChoiceEditorNoKeys::IsAcceptedKey(wxKeyEvent& event)
{
  if (!wxGridCellEditor::IsAcceptedKey(event)) return false;
  int keycode = event.GetKeyCode();
  if (keycode==WXK_SHIFT || keycode>=WXK_F1 && keycode<=WXK_F12)
    return false;
  return false;
}

/*
int GetSysCharSet()
{
    static int SysCharSet = -1;
    if (SysCharSet == -1)
    {
        switch (wxLocale::GetSystemLanguage)
        {
        case wxLANGUAGE_CZECH:
        case wxLANGUAGE_SLOVAK:
            SysCharSet = 1;
            break;
        case wxLANGUAGE_POLISH:
            SysCharSet = 2;
            break;
        case wxLANGUAGE_FRENCH:   
        case wxLANGUAGE_FRENCH_BELGIAN:
        case wxLANGUAGE_FRENCH_CANADIAN:
        case wxLANGUAGE_FRENCH_LUXEMBOURG:   
        case wxLANGUAGE_FRENCH_MONACO:
        case wxLANGUAGE_FRENCH_SWISS:
            SysCharSet = 3;
            break;
        case wxLANGUAGE_GERMAN:
        case wxLANGUAGE_GERMAN_AUSTRIAN:
        case wxLANGUAGE_GERMAN_BELGIUM:
        case wxLANGUAGE_GERMAN_LIECHTENSTEIN:
        case wxLANGUAGE_GERMAN_LUXEMBOURG:
        case wxLANGUAGE_GERMAN_SWISS:
            SysCharSet = 4;
            break;
        case wxLANGUAGE_ITALIAN   
        case wxLANGUAGE_ITALIAN_SWISS  
            SysCharSet = 5;
            break;
        default:
            SysCharSet = 0;
            break;
        }
        int OsVer = wxGetOsVersion();
        if (osVer != wxWIN95 && osVer != wxWINDOWS_NT)
            SysCharSet |= 0x80;
    }
    return SysCharSet;
}

*/

unsigned get_grid_width(wxGrid * mGrid)
{ unsigned width;
  int border = wxSystemSettings::GetMetric(wxSYS_BORDER_X, mGrid);
  if (border<1) border=1;  // is -1 on GTK
  width = mGrid->GetRowLabelSize() + 2*border;
  for (int col=0;  col<mGrid->GetNumberCols();  col++)
    width += mGrid->GetColSize(col) + border;
  return width + wxSystemSettings::GetMetric(wxSYS_VSCROLL_X, mGrid);
}

unsigned get_grid_height(wxGrid * mGrid, int rows)
{ int border = wxSystemSettings::GetMetric(wxSYS_BORDER_Y, mGrid);
  if (border<1) border=1;  // is -1 on GTK
  return 2*border + mGrid->GetColLabelSize() + rows * (border + mGrid->GetDefaultRowSize()) + 
         wxSystemSettings::GetMetric(wxSYS_HSCROLL_Y, mGrid) + 2; // +2 needs GTK
}

///////////////////////////////// warn and ask if connected to a remote network server /////////////////
bool remote_operation_confirmed(cdp_t cdp, wxWindow * parent)
{ char path[MAX_PATH];
  if (cdp->in_use==PT_DIRECT) return true;  // local direct connection, operation OK
  t_avail_server * as = find_server_connection_info(cdp->locid_server_name);
 // check if the path is locally specified:
  if (GetDatabaseString(cdp->locid_server_name, "PATH",  path, sizeof(path), as && as->private_server) || 
      GetDatabaseString(cdp->locid_server_name, "PATH1", path, sizeof(path), as && as->private_server))
    if (*path)
      return true;  // server is local, connected by the network
 // remote server, ask:
  return yesno_box(_("A directory from the remote SQL server must be specified,\nnot a local directory on the client computer.\nDo you still want to browse for the directory?"), parent);
}

////////////////////////////////// case conversions ////////////////////////////////////////////////
wxString GetSmallStringName(const CObjectList & ol)
// cdp in ol my be NULL.
{
  cdp_t cdp = ol.GetCdp();
  wxString name(ol.GetName(), GetConv(cdp));
  if (cdp)
    return CaptFirst(name);
  else  // do not capitalize, some ODBC data sources are case-sensitive in object names
  { if (*ol.GetName() && name.IsEmpty())  // converion error, happens with ODBC data sources with national characters in object names
      name=_("<Error>");
    return name;
  }
}

wxString GetWName(const CObjectList & ol)
{
  return wxString(ol.GetName(), GetConv(ol.GetCdp()));
}

//////////////////////////////////////////////// wxGridCellMyEnumEditor ///////////////////////////////////////////////////
#include "wx/tokenzr.h"

wxGridCellMyEnumEditor::wxGridCellMyEnumEditor(const wxString& choicesIn)
                     :wxGridCellODChoiceEditor()
{
    choices=choicesIn;
    if (!choices.empty())
        SetParameters(choicesIn);
}

wxGridCellEditor *wxGridCellMyEnumEditor::Clone() const
{
    wxGridCellMyEnumEditor *editor = new wxGridCellMyEnumEditor();
    editor->choices = choices;
    return editor;
}

void wxGridCellMyEnumEditor::Reset()
{ 
    Combo()->SetSelection(oldval); 
}

void wxGridCellMyEnumEditor::BeginEdit(int row, int col, wxGrid* grid)
{   long ival;
    wxASSERT_MSG(m_control, wxT("The wxGridCellMyEnumEditor must be Created first!"));
    wxGridTableBase *table = grid->GetTable();
    if ( table->CanGetValueAs(row, col, wxGRID_VALUE_NUMBER) )
    {
        ival = table->GetValueAsLong(row, col);
    }
    else
    {
        wxString startValue = table->GetValue(row, col);
        if (startValue.IsNumber() && !startValue.empty())
        {
            startValue.ToLong(&ival);
        }
        else
        {
            ival=-1;
        }
    }
    oldval = ival;
    Combo()->SetSelection(ival);
    Combo()->SetInsertionPointEnd();
    Combo()->SetFocus();

}

bool wxGridCellMyEnumEditor::EndEdit(int row, int col, wxGrid* grid)
{
    wxString value = Combo()->GetValue();
   // find the value in [choice]:
    int ival=-1, num=0;
    wxStringTokenizer tk(choices, _T(','));
    while ( tk.HasMoreTokens() )
    {   if (tk.GetNextToken() == value)
            { ival=num;  break; }
        num++;
    }
    bool changed = (ival != oldval);
    if (changed)
    {
        if (grid->GetTable()->CanSetValueAs(row, col, wxGRID_VALUE_NUMBER))
            grid->GetTable()->SetValueAsLong(row, col, ival);
        else
            grid->GetTable()->SetValue(row, col, wxString::Format(wxT("%i"), ival));
    }
    return changed;
}

/////////////////////////////////// universal message dialog from WX for MSW version /////////////////////////////////////
#ifdef WINS
#include "../src/generic/msgdlgg.cpp"  // is not included in the ADV library
#endif

/////////////////////////////////// Unicode messages support ///////////////////////////////////////////////
wxString Get_error_num_textWX(xcdp_t xcdp, int err)
{ wxChar _buf[500];
  if (!Get_error_num_textW(xcdp, err, _buf, sizeof(_buf)/sizeof(wxChar))) *_buf=0;
  return wxString(_buf);
}
                                  
wxString bin2hexWX(const uns8 * data, unsigned len)
{ unsigned bt;  wxString res;
  while (len--)
  { bt=*data >> 4;
    res = res + (wxChar)(bt >= 10 ? 'A'-10+bt : '0'+bt);
    bt=*data & 0xf;
    res = res + (wxChar)(bt >= 10 ? 'A'-10+bt : '0'+bt);
    data++;
  }
  return res;
}

void WXUpcase(cdp_t cdp, wxString & s)
// In Unicode version converts the string to the upper case
{ 
#if wxUSE_UNICODE
  wxCharBuffer buf = s.mb_str(*cdp->sys_conv);
  Upcase9(cdp, (char*)(const char*)buf);
  s=wxString((const char*)buf, *cdp->sys_conv);
#else
  Upcase9(cdp, (char*)s.c_str());
#endif
}

///////////////////////////////////////////////// licence message /////////////////////////////////////////////////
wxString get_licence_text(bool server_type)
{ const wxChar * lic_type = server_type ? wxTRANSLATE("Licence Agreement - 602SQL Server") : wxTRANSLATE("Licence Agreement - 602SQL");
  const wxChar * lic_type_trans = wxGetTranslation(lic_type);
  if (lic_type==lic_type_trans)  // english
    return wxString( 
L"    SOFTWARE602 License Agreement\n\n"
    L"This is a legal agreement between you, the end user ('Licensee'), and Software602 Inc. ('Producer')\n\n"
L"I.  The Software602 software program on whatever distribution media including documentation, which is identified on the Cover Sheet (the 'SOFTWARE') is licensed to you for use only on the terms set forth here. Important - The SOFTWARE contains copyright works protected by copyright law and international conventions.\n\n"
L"II. GRANT OF LICENSE.\n\n"
    L"The Producer grants to you:\n"
    L"(a)  The right to use one copy of the SOFTWARE on one computer so long as you comply with the terms of this License.\n"
    L"(b)  Only for duly registered users, the right to receive new versions of SOFTWARE (the 'UPGRADE') for reduced prices, basic consultancy support and optional information about other products of the Producer.\n"
    L"(c)  The right to make a copy of SOFTWARE if it is necessary for the use of the SOFTWARE as mentioned in (a)  above for the purposes of the execution of program functions and for backup purposes.\n\n"
L"III. USE RESTRICTIONS.\n\n"
    L"As the Licensee, you may not particularly:\n"
    L"(a)  Install SOFTWARE on more computers at the same time than it is authorized by this License.\n"
    L"(b)  Make copies of SOFTWARE except as expressly permitted by this License and/or applicable statutes and to distribute them. You should be aware that any breach of this provision by you constitute the infringement of the producer's copyright and trade mark rights.\n"
    L"(c)  Reverse engineer, decompile, disassemble or create derivative works from the SOFTWARE and distribute them and in any other way interfere with the internal structure of SOFTWARE, except as expressly authorized by this License and/or applicable statutes.\n"
    L"(d)  Rent or lease the SOFTWARE, although you may transfer the SOFTWARE on a permanent basis provided you retain no copies and the recipient agrees to the terms of this License.\n\n"
L"IV. LIMITED WARRANTY\n\n")+wxString(
    L"Producer warrants that the SOFTWARE will perform in substantial compliance with the written documentation accompanying SOFTWARE for 180 days after date of delivery of the SOFTWARE. Your sole remedy under the warranty during this 180 day period is that Producer will undertake to correct within a reasonable period of time any reported SOFTWARE errors, correct errors in the documentation and replace any magnetic media which proves defective in materials or workmanship on an exchange basis without a charge. Producer also may, at its sole and exclusive option, either replace the SOFTWARE with a functionally equivalent program at no charge to you. Producer does not warrant that the SOFTWARE will meet your requirements, that operation of the SOFTWARE will be uninterrupted or error-free, or that all SOFTWARE errors will be corrected."
    L"The above warranties are exclusive and are in lieu of all other warranties, expressed or implied, including the implied warranties of merchantibility, fitness for a particular purpose and noninfringement. You may have other rights under applicable statutes.\n\n"
)+wxString(
L"V. LIMITATION OF REMEDIES\n\n"
    L"In no event will Producer be liable to you for any special, consequential, indirect or similar damages, including any lost profits or lost data arising out of the use or inability to use the SOFTWARE or any data supplied therewith even if Producer or anyone else has been advised of the possibility of such damages, or for any claim by any other party. You may have other rights under applicable statutes. In no case shall Producer's liability exceed the purchase price for the SOFTWARE.\n\n"
L"VI. TERMINATION\n\n"
    L"This License is effective until terminated. This License will terminate automatically without notice from Producer if you fail to comply with any provisions of this License. This License is also terminated if and when exchanged for a new License regulating your use of an UPGRADE version of SOFTWARE. Upon termination, you shall stop use and shall destroy all copies of SOFTWARE including written documentation and including modified copies, if any.");
  return lic_type_trans;
}

#ifdef GETTEXT_STRING  // mark these strings for translation - they are missing in WX
// MDI
wxTRANSLATE("&Window") wxTRANSLATE("&Cascade") wxTRANSLATE("&Next") wxTRANSLATE("&Previous")
// Wizard frame:
wxTRANSLATE("&Next >"), wxTRANSLATE("< &Back"), wxTRANSLATE("&Finish"),
#endif

#ifdef WINS
BOOL IsVista(void)
{ OSVERSIONINFO osver;
	osver.dwOSVersionInfoSize = sizeof( OSVERSIONINFO );
	if (GetVersionEx( &osver ) && 
			osver.dwPlatformId == VER_PLATFORM_WIN32_NT && 
			(osver.dwMajorVersion >= 6 ) )
		return TRUE;
	return FALSE;
}

#ifdef IDI_SHIELD  // WINVER >= 0x0600
/* Use GetElevationType() to determine the elevation type of the current process.
Parameters:
ptet
	[out] Pointer to a variable that receives the elevation type of the current process.
	The possible values are:
	TokenElevationTypeDefault - User is not using a "split" token. 
		This value indicates that either UAC is disabled, or the process is started
		by a standard user (not a member of the Administrators group).
	The following two values can be returned only if both the UAC is enabled and
	the user is a member of the Administrator's group (that is, the user has a "split" token):
	TokenElevationTypeFull - the process is running elevated. 
	TokenElevationTypeLimited - the process is not running elevated.
Return Values:
	If the function succeeds, the return value is S_OK. 
	If the function fails, the return value is E_FAIL. To get extended error information, 
	call GetLastError().
*/
HRESULT GetElevationType( TOKEN_ELEVATION_TYPE * ptet )
{ HRESULT hResult = E_FAIL; // assume an error occured
	HANDLE hToken	= NULL;
	if ( !::OpenProcessToken(::GetCurrentProcess(), TOKEN_QUERY, &hToken ) )
		return hResult;
	DWORD dwReturnLength = 0;
	if (::GetTokenInformation(hToken, TokenElevationType, ptet, sizeof( *ptet ), &dwReturnLength ) )
    hResult = S_OK;
	::CloseHandle( hToken );
	return hResult;
}

/* Use IsElevated() to determine whether the current process is elevated or not.
Parameters:
pbElevated
	[out] [optional] Pointer to a BOOL variable that, if non-NULL, receives the result.
	The possible values are:
	TRUE - the current process is elevated.
		This value indicates that either UAC is enabled, and the process was elevated by 
		the administrator, or that UAC is disabled and the process was started by a user 
		who is a member of the Administrators group.
	FALSE - the current process is not elevated (limited).
		This value indicates that either UAC is enabled, and the process was started normally, 
		without the elevation, or that UAC is disabled and the process was started by a standard user. 
Return Values
	If the function succeeds, and the current process is elevated, the return value is S_OK. 
	If the function succeeds, and the current process is not elevated, the return value is S_FALSE. 
	If the function fails, the return value is E_FAIL. To get extended error information, 
	call GetLastError().
*/

HRESULT IsElevated( BOOL * pbElevated )
{ HRESULT hResult = E_FAIL; // assume an error occured
	HANDLE hToken	= NULL;
	if ( !::OpenProcessToken( ::GetCurrentProcess(), TOKEN_QUERY, &hToken ) )
		return hResult;
	TOKEN_ELEVATION te = { 0 };
	DWORD dwReturnLength = 0;
	if (::GetTokenInformation(hToken, TokenElevation, &te, sizeof( te ), &dwReturnLength ) )
	{ hResult = te.TokenIsElevated ? S_OK : S_FALSE; 
		if ( pbElevated)                        
			*pbElevated = (te.TokenIsElevated != 0);
	}
	::CloseHandle( hToken );
	return hResult;
}
#else
#pragma message("The platform SDK for Windows Vista is not installed. Suppressing the Vista-related features.")
HRESULT IsElevated( BOOL * pbElevated )
{ *pbElevated=TRUE; 
  return S_OK;
}  
#endif

#endif

bool AmILocalAdmin(void)
{ bool is_admin=false;
#ifdef WINS
  if (IsVista())
  { BOOL elev;
    if (IsElevated(&elev) == E_FAIL) return false;
    return elev==TRUE;
  }
  HKEY hKey1, hKey2;  DWORD disp;
  if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, "SOFTWARE\\ODBC\\ODBC.INI", 0, KEY_ALL_ACCESS, &hKey1)==ERROR_SUCCESS)
  { if (RegCreateKeyExA(hKey1, "whdgr69", 0, "", REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey2, &disp)==ERROR_SUCCESS)
    { is_admin=true;
      RegCloseKey(hKey2);
      RegDeleteKeyA(HKEY_LOCAL_MACHINE, "whdgr69");
    }  
    RegCloseKey(hKey1);
  }  
#else
  is_admin=true;
#endif  
  return is_admin;
}

#ifdef LINUX
// ----------------------------------------------------------------------------
// wxGridCellODChoiceEditor
// ----------------------------------------------------------------------------

wxGridCellODChoiceEditor::wxGridCellODChoiceEditor(const wxArrayString& choices,
                                               bool allowOthers)
    : m_choices(choices),
      m_allowOthers(allowOthers) { }

wxGridCellODChoiceEditor::wxGridCellODChoiceEditor(size_t count,
                                               const wxString choices[],
                                               bool allowOthers)
                      : m_allowOthers(allowOthers)
{
    if ( count )
    {
        m_choices.Alloc(count);
        for ( size_t n = 0; n < count; n++ )
        {
            m_choices.Add(choices[n]);
        }
    }
}

wxGridCellEditor *wxGridCellODChoiceEditor::Clone() const
{
    wxGridCellODChoiceEditor *editor = new wxGridCellODChoiceEditor;
    editor->m_allowOthers = m_allowOthers;
    editor->m_choices = m_choices;

    return editor;
}

void wxGridCellODChoiceEditor::Create(wxWindow* parent,
                                    wxWindowID id,
                                    wxEvtHandler* evtHandler)
{
    m_control = new wxOwnerDrawnComboBox(parent, id, wxEmptyString,
                               wxDefaultPosition, wxDefaultSize,
                               m_choices,
                               m_allowOthers ? 0 : wxCB_READONLY);

    wxGridCellEditor::Create(parent, id, evtHandler);
}

void wxGridCellODChoiceEditor::PaintBackground(const wxRect& rectCell,
                                             wxGridCellAttr * attr)
{
    // as we fill the entire client area, don't do anything here to minimize
    // flicker

    // TODO: It doesn't actually fill the client area since the height of a
    // combo always defaults to the standard.  Until someone has time to
    // figure out the right rectangle to paint, just do it the normal way.
    wxGridCellEditor::PaintBackground(rectCell, attr);
}

class wxGridCellEditorEvtHandler : public wxEvtHandler  // copy from grid.cpp
{
public:
    wxGridCellEditorEvtHandler(wxGrid* grid, wxGridCellEditor* editor)
        : m_grid(grid),
          m_editor(editor),
          m_inSetFocus(false)
    {
    }

    void OnKillFocus(wxFocusEvent& event);
    void OnKeyDown(wxKeyEvent& event);
    void OnChar(wxKeyEvent& event);

    void SetInSetFocus(bool inSetFocus) { m_inSetFocus = inSetFocus; }

private:
    wxGrid             *m_grid;
    wxGridCellEditor   *m_editor;

    // Work around the fact that a focus kill event can be sent to
    // a combobox within a set focus event.
    bool                m_inSetFocus;

    DECLARE_EVENT_TABLE()
    DECLARE_DYNAMIC_CLASS(wxGridCellEditorEvtHandler)
    DECLARE_NO_COPY_CLASS(wxGridCellEditorEvtHandler)
};

void wxGridCellODChoiceEditor::BeginEdit(int row, int col, wxGrid* grid)
{
    wxASSERT_MSG(m_control,
                 wxT("The wxGridCellEditor must be created first!"));

    wxGridCellEditorEvtHandler* evtHandler = NULL;
    if (m_control)
        evtHandler = wxDynamicCast(m_control->GetEventHandler(), wxGridCellEditorEvtHandler);

    // Don't immediately end if we get a kill focus event within BeginEdit
    if (evtHandler)
        evtHandler->SetInSetFocus(true);

    m_startValue = grid->GetTable()->GetValue(row, col);

    if (m_allowOthers)
    {
        Combo()->SetValue(m_startValue);
    }
    else
    {
        // find the right position, or default to the first if not found
        int pos = Combo()->FindString(m_startValue);
        if (pos == wxNOT_FOUND)
            pos = 0;
        Combo()->SetSelection(pos);
    }

    Combo()->SetInsertionPointEnd();
    Combo()->SetFocus();

    if (evtHandler)
    {
        // When dropping down the menu, a kill focus event
        // happens after this point, so we can't reset the flag yet.
#if !defined(__WXGTK20__)
        evtHandler->SetInSetFocus(false);
#endif
    }
}

bool wxGridCellODChoiceEditor::EndEdit(int row, int col,
                                     wxGrid* grid)
{
    wxString value = Combo()->GetValue();
    if ( value == m_startValue )
        return false;

    grid->GetTable()->SetValue(row, col, value);

    return true;
}

void wxGridCellODChoiceEditor::Reset()
{
    Combo()->SetValue(m_startValue);
    Combo()->SetInsertionPointEnd();
}

void wxGridCellODChoiceEditor::SetParameters(const wxString& params)
{
    if ( !params )
    {
        // what can we do?
        return;
    }

    m_choices.Empty();

    wxStringTokenizer tk(params, _T(','));
    while ( tk.HasMoreTokens() )
    {
        m_choices.Add(tk.GetNextToken());
    }
}

// return the value in the text control
wxString wxGridCellODChoiceEditor::GetValue() const
{
  return Combo()->GetValue();
}

#endif // wxGridCellODChoiceEditor defined on Linux

