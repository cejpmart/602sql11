// datagrid.h

#include "advtooltip.h"

class DataGrid;
class DataRowObject;
struct view_dyn;
struct sview_dyn;
struct t_ctrl;
/////////////////////////////////////// data grid table /////////////////////////////////////////
enum t_on_write_error { OWE_FAIL,             // just returns false
                        OWE_CONT_OR_RELOAD,   // asks user if it should continue editing or reload the old values
                        OWE_CONT_OR_IGNORE }; // asks user if it should continue editing or fail (used when closing the grid)

class DataGridTable : public wxGridTableBase
{protected:
  DataGrid * dg;    // not owning
  xcdp_t xcdp;
  view_dyn * inst;  // not owning
 public:
  trecnum edited_row;
  bool conversion_error;
  void set_edited_row(trecnum row);
  bool write_changes(t_on_write_error owe);
    virtual int GetNumberRows();
    virtual void SetValue( int row, int col, const wxString& value );
// private:
    virtual int GetNumberCols();
    virtual bool IsEmptyCell( int row, int col );
    virtual wxString GetValue( int row, int col );

    virtual wxString GetColLabelValue( int col );
    virtual wxString GetRowLabelValue(int row);
    bool AppendRows(size_t numRows = 1);
    bool InsertRows(size_t pos = 0, size_t numRows = 1);
    bool DeleteRows(size_t pos = 0, size_t numRows = 1);
    bool CanGetValueAs( int row, int col, const wxString& typeName );
    bool CanSetValueAs( int row, int col, const wxString& typeName )
      { return CanGetValueAs(row, col, typeName); }
    long GetValueAsLong( int row, int col);
    void SetValueAsLong( int row, int col, long value );
 public:
  DataGridTable(DataGrid * dgIn, xcdp_t xcdpIn, view_dyn * instIn)
    { dg=dgIn;  xcdp=xcdpIn;  inst=instIn;  edited_row=NORECNUM; }
  void prepare();
  void check_cache_top(int row);
  int GetNativeData(int row, int col, void * buf);
  bool PutNativeData(int row, int col, void * buf, int length);
};

class DataGridToolTip : public wxAdvToolTipText
{ DataGrid * grid;
public:
    DataGridToolTip(DataGrid * gridIn, wxWindow * w);
    wxString GetText(const wxPoint& pos, wxRect& boundary);
}; 

class CColLabel : public wxEvtHandler
{
protected:
  wxWindow *m_Label;
  DataGrid *m_Grid; 
  void OnPaint( wxPaintEvent& event );
  const wxBitmap &GetOrderBitmap(bool Desc);
public:
  CColLabel(){m_Label = NULL;}
  ~CColLabel()
  { if (m_Label)
      m_Label->PopEventHandler();
  }
  DECLARE_EVENT_TABLE()
  friend class DataGrid; 
};

enum t_refresh_extent { RESET_CACHE, RESET_REMAP, RESET_CURSOR };

class DataGrid : public wxGrid, public competing_frame  // wx doc says that the event handling ancestor must be the 1st one
{ friend class DataFrame;
 protected:
  xcdp_t xcdp;
  cdp_t cdp;
  sview_dyn * sinst;
  int current_column_selected;
  bool child_grid;
  int cumulative_wheel_delta;
  bool in_closing_window;
  enum { DATA_TOOL_COUNT=17 };
  virtual DataGridTable * create_grid_table(xcdp_t xcdp, view_dyn * inst)
    { return new DataGridTable(this, xcdp, inst); }
  void edit_grid_privils(void);
  DataGridToolTip * m_tooltip;
  int get_column_size(int col, t_ctrl * itm);
  void OnRemFilterOrder();
  void OnFilterOrder();
  void cursor_changed(void);
 public:
  static t_tool_info data_tool_info[DATA_TOOL_COUNT+1];
  view_dyn * inst;  // owning
  DataGridTable * dgt;  // owning
  trecnum last_recnum;
  bool translate_captions;
  CColLabel ColLabel;
  DataGrid(xcdp_t xcdpIn) : wxGrid(), competing_frame()
  { xcdp=xcdpIn;  cdp=xcdp->get_cdp();  
    dgt=NULL;  inst=NULL;  m_tooltip=NULL;  translate_captions=false;  cumulative_wheel_delta=0; 
    in_closing_window=false;  current_column_selected=-1;
    persistent_coords=&tabledsng_coords;  small_icon=_table_xpm;
  }
  ~DataGrid(void);
  wxWindow * open_form(wxWindow * parent, xcdp_t xcdp, const char * source, tcursnum curs, int flags);
  wxWindow * open_data_grid(wxWindow * parent, xcdp_t xcdp, tcateg categ, tcursnum curs, char * objname, bool delrecs, const char * odbc_prefix);
  int create_column_description();
  void OnSize(wxSizeEvent & event);
  void OnKeyDown(wxKeyEvent & event);
  //void OnMouseEvent(wxMouseEvent & event);
  void OnMouseWheel(wxMouseEvent & event);
  void OnLabelDblClick(wxGridEvent & event);
  void StartExternalEditor(int row, int col);
  void OnEditorHidden(wxGridEvent & event);
  void OnSelectCell(wxGridEvent & event);
  void OnRangeSelect(wxGridRangeSelectEvent & event);
  void OnCommand( wxCommandEvent& event );
  void OnxCommand( wxCommandEvent& event );
  void OnDisconnecting(ServerDisconnectingEvent & event);
  void OnRecordUpdate(RecordUpdateEvent & event);
  void refresh(t_refresh_extent extent);
  void make_cursor_persistent(void);
  t_tool_info * get_tool_info(void) const
    { return data_tool_info; }
  wxMenu * get_designer_menu(void);
  void _set_focus(void)
    { if (GetHandle()) SetFocus(); }
  void Activated(void);
  void Deactivated(void);
  void update_designer_menu(void);
  void update_toolbar(void) const;
  void synchronize_rec_count(void);
  wxArrayInt MyGetSelectedRows(void) const;
  wxString GetTooltipText(void);
  bool copy_row_to_data_object(trecnum rec, DataRowObject * rdo);
  bool copy_data_object_to_row(DataRowObject * rdo, trecnum rec);
  DECLARE_EVENT_TABLE()
  virtual void delete_selected_records(void);
  void delete_all_records(void);
  friend class CColLabel;
  void set_designer_caption(void);
  bool open(wxWindow * parent, t_global_style my_style);
 public:
  const void * enum_reverse(const wxChar * enumdef, const wxChar * str);
  void write_current_pos(trecnum row);
};

#if 0
class DataFramePopup : public wxFrame
{
public:
  DataFramePopup(wxWindow * parent, wxWindowID id, const wxString& title, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxDEFAULT_FRAME_STYLE | wxFRAME_FLOAT_ON_PARENT | wxWANTS_CHARS)
  // Warning: in wx 2.5.3 the style wxFRAME_NO_TASKBAR makes it impossible to focus the popup
    : wxFrame(parent, id, title, pos, size, style)
    { grid = NULL;}
  DataGrid * grid;
  void OnCommand( wxCommandEvent& event );
  void OnActivate(wxActivateEvent & event);
  void OnCloseWindow(wxCloseEvent & event);
  void OnFocus(wxFocusEvent& event); 
  void OnDisconnecting(wxEvent & event);
  void OnRecordUpdate(wxEvent & event);
  void OnInternalIdle();

  DECLARE_EVENT_TABLE()
};

class DataFrameMDI : public wxMDIChildFrame
{public:
  DataFrameMDI(wxMDIParentFrame* parent, wxWindowID id, const wxString& title, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxDEFAULT_FRAME_STYLE)
    : wxMDIChildFrame(parent, id, title, pos, size, style)
    { grid = NULL; 
    }
  DataGrid * grid;
  void OnCommand( wxCommandEvent& event );
  void OnActivate(wxActivateEvent & event);
  void OnCloseWindow(wxCloseEvent & event);
  void OnFocus(wxFocusEvent& event); 
  void OnDisconnecting(wxEvent & event);
  void OnRecordUpdate(wxEvent & event);

  DECLARE_EVENT_TABLE()
};
#endif

class wxGridCellMLTextEditor : public wxGridCellEditor
{ wxWindow * parent_grid;
  bool read_only;
public:
    wxGridCellMLTextEditor(bool read_onlyIn);

    virtual void Create(wxWindow* parent,
                        wxWindowID id,
                        wxEvtHandler* evtHandler);
    virtual void SetSize(const wxRect& rect);

    virtual void PaintBackground(const wxRect& rectCell, wxGridCellAttr *attr);

    virtual bool IsAcceptedKey(wxKeyEvent& event);
    virtual void BeginEdit(int row, int col, wxGrid* grid);
    virtual bool EndEdit(int row, int col, wxGrid* grid);

    virtual void Reset();
    virtual void StartingKey(wxKeyEvent& event);
    virtual void HandleReturn(wxKeyEvent& event);

    virtual wxGridCellEditor *Clone() const
        { return new wxGridCellMLTextEditor(read_only); }

    // DJC MAPTEK
    // added GetValue so we can get the value which is in the control
    virtual wxString GetValue() const;
protected:
    wxTextCtrl *Text() const { return (wxTextCtrl *)m_control; }

    // parts of our virtual functions reused by the derived classes
    void DoBeginEdit(const wxString& startValue);
    void DoReset(const wxString& startValue);

private:
    wxString m_startValue;

    DECLARE_NO_COPY_CLASS(wxGridCellMLTextEditor)
};

// the editor for boolean data
class wxGridCellBool3Editor : public wxGridCellEditor
{
public:
    wxGridCellBool3Editor() { }

    virtual void Create(wxWindow* parent,
                        wxWindowID id,
                        wxEvtHandler* evtHandler);

    virtual void SetSize(const wxRect& rect);
    virtual void Show(bool show, wxGridCellAttr *attr = (wxGridCellAttr *)NULL);

    virtual bool IsAcceptedKey(wxKeyEvent& event);
    virtual void BeginEdit(int row, int col, wxGrid* grid);
    virtual bool EndEdit(int row, int col, wxGrid* grid);

    virtual void Reset();
    virtual void StartingClick();

    virtual wxGridCellEditor *Clone() const
        { return new wxGridCellBool3Editor; }
    // DJC MAPTEK
    // added GetValue so we can get the value which is in the control
    virtual wxString GetValue() const;

protected:
    wxCheckBox *CBox() const { return (wxCheckBox *)m_control; }

private:
    wxCheckBoxState m_startValue;

    DECLARE_NO_COPY_CLASS(wxGridCellBool3Editor)
};

// renderer for boolean fields
class wxGridCellBool3Renderer : public wxGridCellRenderer
{
public:
    // draw a check mark or nothing
    virtual void Draw(wxGrid& grid,
                      wxGridCellAttr& attr,
                      wxDC& dc,
                      const wxRect& rect,
                      int row, int col,
                      bool isSelected);

    // return the checkmark size
    virtual wxSize GetBestSize(wxGrid& grid,
                               wxGridCellAttr& attr,
                               wxDC& dc,
                               int row, int col);

    virtual wxGridCellRenderer *Clone() const
        { return new wxGridCellBool3Renderer; }

private:
    static wxSize ms_sizeCheckMark;
};

class DataRowObject : public wxDataObjectSimple
{ size_t data_size;
  void * data;
  size_t internal_offset;
 public:
  DataRowObject(void)
    { data_size=0;  data=NULL; 
      SetFormat(*DF_602SQLDataRow);
    }
  ~DataRowObject(void)
    { if (data) corefree(data); }
  size_t GetDataSize(void) const
    { return data_size; }
  bool GetDataHere(void *buf) const
    { if (!data || !buf) return false;
      memcpy(buf, data, data_size);
      return true;
    }
  bool SetData(size_t len, const void *buf)
  { if (data) corefree(data);  
    data_size=0;
    data=corealloc(len, 41);
    if (!data) return false;
    memcpy(data, buf, len);
    data_size = len;
    return true;
  }
  bool prepare_buffer(size_t extent);
  void * get_ptr(void);
  void reset_ptr(void);
  void advance_ptr(size_t offset);

};

bool close_dependent_editors(cdp_t cdp, tcursnum cursnum, bool unconditional);
