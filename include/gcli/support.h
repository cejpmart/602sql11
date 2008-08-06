#ifndef _SUPPORT_H_
#define _SUPPORT_H_

// top-level defines:
#if wxMAJOR_VERSION==2 && wxMINOR_VERSION<8
#define FL
#endif

#include "wx/datetime.h"
#include "wx/grid.h"
#include "wx/odcombo.h"
#include "unicsupp.h"
#include "exceptions.h"
#include "enumsrv.h"

#ifdef _DEBUG
#ifdef WINS
#include <crtdbg.h>
#define DEBUG_NEW new(_NORMAL_BLOCK ,__FILE__, __LINE__)
#else
#define DEBUG_NEW new  // LINUX
#endif 
#else
#define DEBUG_NEW new  // non-debug
#endif 

//#define WXK_APPS 0x5d

extern char *File_save_xpm[];
extern char *_cut_xpm[];
extern char *Undo_xpm[];
extern char *Redo_xpm[];
extern char *run2_xpm[];
extern char *traceInto_xpm[];
extern char *stepOver_xpm[];
extern char *runToCursor_xpm[];
extern char *runUntilReturn_xpm[];
extern char *evaluate_xpm[];
extern char *break_xpm[];
extern char *runu_xpm[];
extern char *debugKill_xpm[];
extern char *debugRun_xpm[];
extern char *compile_xpm[];
extern char *cut_xpm[];
extern char *validateSQL_xpm[];
extern char *help_xpm[];
extern char *DeleteCol_xpm[];
extern char *InsertCol_xpm[];
extern char *tableProperties_xpm[];
extern char *showSQLtext_xpm[];
extern char *modify_xpm[];
extern char *delete_xpm[];
extern char *Kopirovat_xpm[];
extern char *Vlozit_xpm[];
extern char *empty_xpm[];
extern char *_transp_xpm[];
extern char *_table_xpm[];
extern char *_query_xpm[];
extern char *_program_xpm[];
extern char *_xml_xpm[];
extern char *_seq_xpm[];
extern char *_domain_xpm[];
extern char *_full_xpm[];
extern char *_draw_xpm[];
extern char *_proc_xpm[];
extern char *_trigger_xpm[];
extern char *trigwiz_xpm[];
extern char *refrpanel_xpm[];
extern char *exit_xpm[];
extern char *editSQL_xpm[];
extern char *source_xpm[];
extern char *openselector_xpm[];
extern char *privil_dat_xpm[];
extern char *question_xpm[];
extern char *params_xpm[];
extern char *profile_xpm[];

#define TABLE_QUERY_CONFLICT  _("Never use the same name for a table and a query!")
#define OBJ_NAME_USED         _("An object with the same name and category already exists.")


#define WCTRLA   1
#define WCTRLB   2  
#define WCTRLC   3
#define WCTRLF   6
#define WCTRLH   8
#define WCTRLL  12
#define WCTRLK  11
#define WCTRLM  13
#define WCTRLN  14
#define WCTRLO  15
#define WCTRLP  16
#define WCTRLQ  17
#define WCTRLS  19 
#define WCTRLV  22
#define WCTRLW  23
#define WCTRLX  24
#define WCTRLY  25
#define WCTRLZ  26

bool IsCtrlDown(void);
bool IsShiftDown(void);
////////////////////////// global functions ////////////////////////////
wxString int2wxstr(int val);
bool enter_object_name(cdp_t cdp, wxWindow * parent, wxString prompt, char * name, bool renaming = false, tcateg categ = 0);
bool get_name_for_new_object(cdp_t cdp, wxWindow * parent, const char * schema_name, tcateg categ, wxString prompt, char * name, bool renaming = false);
bool edit_name_and_create_object(cdp_t cdp, wxWindow * parent, const char * schema_name, tcateg categ, wxString prompt, tobjnum * objnum, char * name);
bool convertible_name(cdp_t cdp, wxWindow * parent, wxString name);
void check_open_transaction(cdp_t cdp, wxWindow * parent);

CFNC BOOL WINAPI report_last_error(cdp_t cdp, wxWindow * parent);
void connection_error_box(cdp_t cdp, int errnum);
bool can_overwrite_file(const char * fname, bool * overwrite_all);
CFNC void WINAPI no_memory(void);
void no_memory2(wxWindow * parent);

wxString SysErrorMsg(const wxChar *Fmt, ...);
wxString SQL602ErrorMsg(cdp_t cdp, const wxChar *Fmt, ...);

CFNC bool WINAPI cd_Signalize2(cdp_t cdp, wxWindow * parent);
CFNC BOOL WINAPI cd_Signalize(cdp_t cdp);
CFNC void WINAPI remove_diacrit(char * str, int len);

bool check_not_encrypted_object(cdp_t cdp, tcateg categ, tobjnum objnum);
bool is_object_name(cdp_t cdp, const wxChar * name, wxWindow * parent);
bool is_valid_object_name(cdp_t cdp, const char * name, wxWindow *parent);
enum tobjnmvldt {onInvalid, onTooLong, onSuspicious, onValid};
tobjnmvldt check_object_name(const char * name);
bool IsProperIdent(const wxChar *Ident);
wxString GetProperIdent(wxString Ident, wchar_t qc = wxT('`'));
wxString GetNakedIdent(wxString Ident);

bool read_profile_string(const char * section, const char * entry, char * buf, int bufsize);
bool write_profile_string(const char * section, const char * entry, const char * buf);
bool read_profile_int(const char * section, const char * entry, int * val);
bool write_profile_int(const char * section, const char * entry, int val);
bool read_profile_bool(const char * section, const char * entry, bool Default = false);
void write_profile_bool(const char * section, const char * entry, bool val);
int  read_profile_int(const char * section, const char * entry, int Default = 0);
wxString read_profile_string(const char * section, const char * entry, const char * Default = "");
void drop_ini_file_name(void);

void * get_object_enumerator(cdp_t cdp, tcateg categ, const char * schema);
void free_object_enumerator(void * en);
bool object_enumerator_next(void * en, char * name, tobjnum & objnum, int & flags, uns32 & modif_timestamp, int & folder);
void move_object_names_to_combo(wxOwnerDrawnComboBox * combo, cdp_t cdp, tcateg category);
void fill_table_names(cdp_t cdp, wxOwnerDrawnComboBox * combo, bool add_base_cursor, const char * sel_table_name);
class xml_designer;
class t_ucd_cl;
d_table * get_expl_descriptor(cdp_t cdp, const char * sql_stmt, const char * dsn, xml_designer * dsgn, t_ucd_cl ** p_stmt_ucdp, tcursnum * top_curs);

void Get_obj_folder_name(cdp_t cdp, tobjnum objnum, tcateg categ, char * folder_name);
bool Set_object_flags(cdp_t cdp, tobjnum objnum, tcateg categ, uns16 flags);
void Add_new_component(cdp_t cdp, tcateg categ, const char * schema_name, const char * objname, tobjnum objnum, uns16 flags, const char * folder_name, bool select_it);
void Changed_component(cdp_t cdp, tcateg categ, const char * schema_name, const char * objname, tobjnum objnum, uns16 flags);
void Change_component_flag(cdp_t cdp, tcateg categ, const char * schema_name, const char * objname, tobjnum objnum, int mask, int setbit);

wxString load_and_check_tab_def(cdp_t cdp, FHANDLE hFile, char *tabname, int *flags);
bool CopyFromFile(cdp_t cdp, ttablenum tb, trecnum recnum, tattrib attr, uns16 index, FHANDLE hFile, uns32 offset, int recode);

extern int connected_count;
bool make_sys_conv(cdp_t cdp);
cdp_t clone_connection(cdp_t cdp);
void wx_disconnect(cdp_t cdp);


BOOL can_modify(tobjnum objnum, tcateg categ);
bool can_modify(cdp_t cdp);
bool password_short(cdp_t cdp, const char * password);

///////////////////// registry access //////////////////////////////
bool GetDatabaseString(const char * server_name, const char * value_name, void * dest, int bufsize, bool private_server);
bool SetDatabaseString(const char * server_name, const char * value_name, const char * val, bool private_server);
bool GetDatabaseNum(const char * server_name, const char * value_name, sig32 & val, bool private_server);
bool SetDatabaseNum(const char * server_name, const char * value_name, sig32 val, bool private_server);
bool DeleteDatabaseValue(const char * server_name, const char * value_name, bool private_server);
bool IsServerRegistered(const char * server_name);
#ifdef WINS
bool WriteServerNameForService(wxString service_name, wxString server_name, bool add_default_description);
#endif

char * append(char * dest, const char * src);  // appends with single PATH_SEPARATOR

////////////////////////////////////// string history ///////////////////////////////////
class string_history
// The most recent string is strngs[0], the oldest is strngs[count-1].
{ int size, count;
  wxString * strngs;
 public:
  string_history(int sizeIn)
    { size=sizeIn;  count=0;  strngs=new wxString[size]; }
  ~string_history(void)
    { delete [] strngs; }
  void add_to_history(wxString str);
  const wxChar * get_from_history(int ind);
  void fill(wxOwnerDrawnComboBox * combo);
};
///////////////////////////////// t_temp_appl_context ///////////////////////////////////////
class t_temp_appl_context 
// Supporting class for temporary changes of application context inside any_designer
{ tobjname orig_schema;
  bool schema_changed;
  cdp_t cdp;
  bool err;
 public:
  t_temp_appl_context(cdp_t designer_cdp, const char * designer_schema, int extent=2);
  ~t_temp_appl_context(void);
  bool is_error(void) const
    { return err; }
};

////////////////////////////////////// OLE /////////////////////////////////////////////////////
#define STR_IMPOSSIBLE_CAPTION  _("Impossible")
#define STR_ERROR_CAPTION       _("Error")
#define STR_WARNING_CAPTION     _("Warning")
#define STR_INFO_CAPTION        _("Information")
#define TEXT_TOO_LONG           _("Text in the control is too long")
#define ContinueMsg             _("Continue?")

////////////////////////// result list (search, validation) ///////////////////////////////
struct search_result
{ tcateg categ;
  tobjnum objnum;
  uns32 respos;
  search_result(tcateg categIn, tobjnum objnumIn, uns32 resposIn)
    { categ=categIn;  objnum=objnumIn;  respos=resposIn; }
};

class SearchResultList : public wxListBox
{public: 
  tobjname server_name;  // server containing objects from the list (locid_server_name)
  void ClearAll(void);
  void OnDblClk(wxCommandEvent & event);
  DECLARE_EVENT_TABLE()
};
////////////////////////////// menu with bitmaps //////////////////////////////////////
void AppendXpmItem(wxMenu * menu, int id, const wxString& item, char * bitmap_xpm[] = empty_xpm, bool enb = true);
int GetNewComboSelection(wxOwnerDrawnComboBox * combo);

inline wxString UUID2str(WBUUID uuid)
{ return bin2hexWX(uuid, UUID_SIZE); }

inline wxString SelSchemaUUID2str(cdp_t cdp)
{ return bin2hexWX(cdp->sel_appl_uuid, UUID_SIZE); }

#if wxUSE_UNICODE

typedef wxCharBuffer tccharptr;

inline wxChar *CaptFirst(wxChar *Text)
{
    if (*Text)
    {
        *Text = wxToupper(*Text);
        for (wxChar *p = Text + 1; *p; *p++)
            *p = wxTolower(*p);
    }
    return Text;
}

#define TowxChar(Text, conv) (const wxChar *)(conv)->cMB2WC(Text)
#define ToChar(Text, conv)   (conv)->cWC2MB(Text)

#else

typedef const char *tccharptr;

inline wxChar *CaptFirst(wxChar *Text)
{
    small_string(Text, TRUE);
}

#define TowxChar(Text, conv) Text
#define ToChar(Text, conv) Text

#endif

inline wxString &CaptFirst(wxString &Text)
{
  if (!Text.IsEmpty())  
  { Text.LowerCase();
    Text.GetWritableChar(0) = wxToupper(Text.GetChar(0));
  }
  return Text;
}

inline wxMBConv &GetConv(cdp_t cdp)
{
    if (cdp)
        return *cdp->sys_conv;
    else
        return *wxConvCurrent;
}

inline wxMBConv &GetConv(xcdp_t xcdp)
{
    if (xcdp && xcdp->get_cdp())
        return *xcdp->get_cdp()->sys_conv;
    else
        return *wxConvCurrent;
}

////////////////////////////// wxDateTime conversions ////////////////////////////////

wxDateTime Date2wxDateTime(uns32 Date);
wxDateTime Time2wxDateTime(uns32 Time);
wxDateTime timestamp2wxDateTime(uns32 timestamp);
uns32 wxDateTime2Date(wxDateTime DateTime);
uns32 wxDateTime2Time(wxDateTime DateTime);
uns32 wxDateTime2timestamp(wxDateTime DateTime);

///////////////////////////////// default grid data formats ///////////////////////
extern int format_real;
extern wxString format_decsep;
extern wxString format_date;
extern wxString format_time;
extern wxString format_timestamp;

void get_real_format_info(signed char precision, int *decim, int *form);
extern t_specif wx_string_spec;
wxString get_grid_default_string_format(int tp);
wxString get_grid_default_string_format_checkbool(int tp);
unsigned get_grid_width(wxGrid * mGrid);
unsigned get_grid_height(wxGrid * mGrid, int rows);

////////////////////////////////////////////////////////////////
int PatchedGetMetric(wxSystemMetric index);

void SetIntValue(wxTextCtrl * ctrl, int value);
void CALLBACK log_client_error(const char * text);

#if wxUSE_UNICODE
#define Search_in_blockT(block, blocksize, next, patt, pattsize, flags, charset, respos) \
        Search_in_blockW(block, blocksize, next, patt, pattsize, flags, charset, respos)
#else
#define Search_in_blockT(block, blocksize, next, patt, pattsize, flags, charset, respos) \
        Search_in_blockA(block, blocksize, next, patt, pattsize, flags, charset, respos)
#endif


wxArrayInt UnivGetSelectedRows(wxGrid * grid);
//////////////////////////////////////////////////////////
#ifdef WINS
#include <wx/generic/msgdlgg.h>
#define wx602MessageDialog wxGenericMessageDialog
#else
#define wx602MessageDialog wxMessageDialog  // is wxGenericMessageDialog but its name is wxMessageDialog
#endif

bool remote_operation_confirmed(cdp_t cdp, wxWindow * parent);

#ifdef CLIENT_ODBC_9
extern HENV hEnv;
void odbc_stmt_error(t_connection * conn, HSTMT hStmt);  // like cd_Signalize for ODBC
//void odbc_stmt_error(HSTMT hStmt);
//void odbc_dbc_error(HDBC hDbc);
class t_ie_run;
bool find_servers(t_ie_run * ier, wxWindow * parent);
#endif

extern const char sqlserver_owner[];
wxString get_xml_filter(void);  //  from xmldsgn.cpp, but only for gcli
bool check_xml_library(void);
bool AmILocalAdmin(void);

#endif // _SUPPORT_H_

