// tabledsgn.h
#include "wx/splitter.h"
#include "wx/notebook.h"
#include "wx/grid.h"
#ifdef FL
#include "wx/fl/dyntbar.h"       // auto-layout toolbar
#include "wx/fl/dyntbarhnd.h"    // control-bar dimension handler for it
#endif
#include "xrc/ColumnBasicsDlg.h"
#include "xrc/ColumnHintsDlg.h"
#include "xrc/MoveColValueDlg.h"
#include "xrc/ShowTextDlg.h"
#include "xrc/IndexListDlg.h"
#include "xrc/CheckListDlg.h"
#include "xrc/RefListDlg.h"
#include "xrc/TablePropertiesDlg.h"

#define DSGN_FLAG_DEVENV   1  // should be public! ###

class table_designer;

class wxTableDesignerGridTableBase : public wxGridTableBase
{ table_designer * tde;
    virtual int GetNumberRows();
    virtual int GetNumberCols();
    virtual bool IsEmptyCell( int row, int col );
    virtual wxString GetValue( int row, int col );
    virtual void SetValue( int row, int col, const wxString& value );

    virtual wxString GetColLabelValue( int col );
    bool AppendRows(size_t numRows = 1);
    bool InsertRows(size_t pos = 0, size_t numRows = 1);  // used only for registering columns in an existing table
    bool CanGetValueAs( int row, int col, const wxString& typeName );
    long GetValueAsLong( int row, int col);
    void SetValueAsLong( int row, int col, long value );
 public:
  wxTableDesignerGridTableBase(table_designer * tdeIn)
    { tde=tdeIn; }

};

class wxIndexListGridTable : public wxGridTableBase
{ table_designer * tde;
 public:
    virtual int GetNumberRows();
 private:
    virtual int GetNumberCols();
    virtual bool IsEmptyCell( int row, int col );
    virtual wxString GetValue( int row, int col );
    virtual void SetValue( int row, int col, const wxString& value );

    virtual wxString GetColLabelValue( int col );
    bool AppendRows(size_t numRows = 1);
    bool InsertRows(size_t pos = 0, size_t numRows = 1);  // used only for registering an existing table
    bool DeleteRows(size_t pos = 0, size_t numRows = 1);
    bool CanGetValueAs( int row, int col, const wxString& typeName );
    long GetValueAsLong( int row, int col);
    void SetValueAsLong( int row, int col, long value );
 public:
  wxIndexListGridTable(table_designer * tdeIn)
    { tde=tdeIn; }

};

class wxCheckListGridTable : public wxGridTableBase
{ table_designer * tde;
 public:
    virtual int GetNumberRows();
 private:
    virtual int GetNumberCols();
    virtual bool IsEmptyCell( int row, int col );
    virtual wxString GetValue( int row, int col );
    virtual void SetValue( int row, int col, const wxString& value );

    virtual wxString GetColLabelValue( int col );
    bool AppendRows(size_t numRows = 1);
    bool InsertRows(size_t pos = 0, size_t numRows = 1);  // used only for registering an existing table
    bool DeleteRows(size_t pos = 0, size_t numRows = 1);
    bool CanGetValueAs( int row, int col, const wxString& typeName );
    long GetValueAsLong( int row, int col);
    void SetValueAsLong( int row, int col, long value );
 public:
  wxCheckListGridTable(table_designer * tdeIn)
    { tde=tdeIn; }
};

class wxRefListGridTable : public wxGridTableBase
{ table_designer * tde;
 public:
    virtual int GetNumberRows();
 private:
    virtual int GetNumberCols();
    virtual bool IsEmptyCell( int row, int col );
    virtual wxString GetValue( int row, int col );
    virtual void SetValue( int row, int col, const wxString& value );

    virtual wxString GetColLabelValue( int col );
    bool AppendRows(size_t numRows = 1);
    bool InsertRows(size_t pos = 0, size_t numRows = 1);  // used only for registering an existing table
    bool DeleteRows(size_t pos = 0, size_t numRows = 1);
    bool CanGetValueAs( int row, int col, const wxString& typeName );
    long GetValueAsLong( int row, int col);
    void SetValueAsLong( int row, int col, long value );
 public:
  wxRefListGridTable(table_designer * tdeIn)
    { tde=tdeIn; }
};

#define TD_TOOL_COUNT 11

class table_designer : public wxSplitterWindow, public any_designer
{ 
  //DECLARE_DYNAMIC_CLASS(table_designer);
  friend class wxTableDesignerGridTableBase;
  friend class wxIndexListGridTable;
  friend class IndexListDlg;
  friend class wxCheckListGridTable;
  friend class CheckListDlg;
  friend class wxRefListGridTable;
  friend class RefListDlg;
  friend class ColumnBasicsDlg;
  friend class ColumnHintsDlg;
  char * ce_buffer;
  int ce_bufsize;
  unsigned ce_flags;       // table editor flags
  bool editing_new_table, feedback_disabled;
  bool WB_table;
  t_connection * tbdf_conn;
  bool changed, combo_changed;
  table_all edta;        // edited table structure
  //const xtype_info * ti;
  const t_type_list * type_list;
  int current_row_selected;

  wxGrid * column_grid, * index_list_grid, * check_list_grid, * ref_list_grid;
  wxNotebook * property_notebook;
  wxTableDesignerGridTableBase* grid_table;
  wxIndexListGridTable * index_list_grid_table;
  wxCheckListGridTable * check_list_grid_table;
  wxRefListGridTable * ref_list_grid_table;
  ColumnBasicsDlg * cbd_page;
  ColumnHintsDlg  * hint_page;
  MoveColValueDlg * upd_page;

  wxGrid * create_grid(wxWindow * parent);
  void init_new_table(const char * table_name);
  void prepare_table_def_editing(void);
  bool compile_WB_table_structure(void);
  void convert_for_edit(void);
  void convert_for_sql(void);

  atr_all * find_column_descr(int colnum, char ** p_cur_name = NULL);
  ind_descr * get_index(int row);
  bool new_column_name_ok(const char * new_col_name, atr_all * for_atr);
  const char * type_param_capt(int wbtype) const;
  const char * short_type_name(int tp);
  void set_default(atr_all * atr);
  wxString get_default_conversion(atr_all * cur_atr, const char * names);
  void show_default_conversion(atr_all * cur_atr, const char * cur_name);
  void is_modified(atr_all * cur_atr, const char * names, int row);
  void update_value_page(bool moved_col, atr_all * cur_atr);
  trecnum create_new_check(void);
  int check_consistency(bool explic);
  void enable_by_type(int row, const atr_all * cur_atr);
  check_constr * get_check(int row);
  forkey_constr * get_forkey(int row);
  void update_local_key_choice(void);
  void update_remote_key_choice(const char * tabname, int row);
  void update_default_choice(int row, int wbtype);
  void set_row_colour(int row, bool inserting_new);
  bool search_column_name(const char * name, int mode);
  void replace_column_name(const char * oldname, const char * newname);
  void disable_cells_in_fictive_record(int row);

  char * make_source(bool alter);
 public:
  static t_tool_info td_tool_info[TD_TOOL_COUNT+1];
  table_designer(cdp_t cdpIn, tobjnum objnumIn, const char * folder_nameIn, const char * schema_nameIn) : 
    any_designer(cdpIn, objnumIn, folder_nameIn, schema_nameIn, CATEG_TABLE, _table_xpm, &tabledsng_coords)
  { ce_buffer=NULL;  ce_bufsize=0;  ce_flags=DSGN_FLAG_DEVENV;  feedback_disabled=false;  tbdf_conn=NULL; 
    cursnum=TAB_TABLENUM; 
    column_grid=NULL;
  }
  table_designer(void)  {} // never used, just for dynamic cast
  ~table_designer(void);
  void set_designer_caption(void);
  void show_column_parameters(atr_all * cur_atr, char * cur_name);                       
  bool save_column_parameters(atr_all * cur_atr, char * cur_name);
  wxGrid * create_index_list_grid(wxWindow * parent);
  wxGrid * create_check_list_grid(wxWindow * parent);
  wxGrid * create_ref_list_grid(wxWindow * parent);

  void OnSelectCell( wxGridEvent & event);
  void OnRangeSelect(wxGridRangeSelectEvent & event);
  void OnKeyDown( wxKeyEvent& event );
  void OnSize(wxSizeEvent & event);
  void OnFocus(wxFocusEvent& event) ;
  bool open(wxWindow * parent, t_global_style my_style);

 // virtual methods (commented in class any_designer):
  bool IsChanged(void) const;
  wxString SaveQuestion(void) const;
  bool save_design(bool save_as);
  void OnCommand(wxCommandEvent & event);
  t_tool_info * get_tool_info(void) const
    { return td_tool_info; }
  void _set_focus(void)
  { if (GetHandle()) // designer created (is not created on locking errors)
      SetFocus(); 
  }
  wxMenu * get_designer_menu(void);

  DECLARE_EVENT_TABLE()
};

ind_descr * get_index_by_column(cdp_t cdp, table_all * ta, const char * unq_colname);
