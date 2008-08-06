// tablepanel.h
#include <wx/dragimag.h>
#include "impexpui.h" // clipboard objects support
#define WD 4
class table_panel;
enum diagram_hit_type { DG_HIT_NONE, DG_HIT_TABLE, DG_HIT_TABLE_END, DG_HIT_COLUMN,
  DG_HIT_LINE_NS, DG_HIT_LINE_WE, 
  DG_HIT_LINEEND_NESW, DG_HIT_LINEEND_NWSE };

enum t_ending_type { ET_UNSICIF, ET_ONE, ET_MANY };

struct index_in_table
{ char * def;
  uns16 index_type;
  int pos_in_win;  // user-column pos or composite index pos
  index_in_table(void)
    { def=NULL; }
  ~index_in_table(void)
    { if (def) corefree(def); }
};

WX_DECLARE_LIST(index_in_table, DiagrIndexList);

struct table_in_diagram
{ wxPoint pos;
  wxSize size;
  ttablenum tbnum;
  tobjname table_name, schema_name;
  int col_cnt;    // number of user-defined columns
  DiagrIndexList indexes;
  int maxsizey;
  void draw(cdp_t cdp, wxDC & dc, table_panel * panel);
  void write(cdp_t cdp, const char * base_schema_name, char * buf);
  bool read(cdp_t cdp, const char * base_schema_name, const char *& pstr);
  bool get_table_info(cdp_t cdp, table_panel * panel);
  const char * index_text(int posit);
  int is_index(int posit);
  int find_key(const char * key);
  table_in_diagram(void)
    { indexes.DeleteContents(true); }
};

WX_DECLARE_LIST(table_in_diagram, DiagrTableList);

WX_DECLARE_LIST(wxPoint, PointList);

struct line_in_diagram
{ table_in_diagram * endtable[2]; // not owning
  t_ending_type ending_type[2];
  int pos1, pos2;  // used only when re-creating the line;
  PointList points;  // the 1st point is at endtable[0], the last at endtable[1]
  bool displaced;
  diagram_hit_type line_hit_test(int x, int y, int & itempart);
  void draw(wxDC & dc, wxBitmap * bitmap1, wxBitmap * bitmap2);
  line_in_diagram(void)
    { points.DeleteContents(true);  displaced=false; }
};
WX_DECLARE_LIST(line_in_diagram, DiagrLineList);

class table_panel : public wxScrolledWindow
{ friend struct table_in_diagram;
  friend class TablePanelDropTarget;
  friend class DiagramPrintout;
  wxPen * TrackerPen, * LinePen;
  wxStockCursor current_cursor;  // code of the current cursor
  wxBitmap *key_bitmap, *multi_bitmap, *index_bitmap;
  cdp_t pcdp;
  bool dragging;
  diagram_hit_type drag_dght;
  int m_oldX, m_oldY;  // the last tracker position
  int m_diffx, m_diffy;  // table dragging support
  int table_caption_height, table_column_height;
  wxDragImage * drag_key;
  diagram_hit_type HitTest(int x, int y, int & item, int & itempart);
  void DrawLineTracker0(wxDC & screenDC, int x1, int y1, int x2, int y2, int x, int y, bool two);
  void DrawLineTracker(int x, int y);
  void DrawTableTracker(int x, int y);
  void DrawTableBottomTracker(int x, int y);
  void MoveDraggedLine(int x, int y);
  void MoveDraggedTable(int x, int y);
  void SizeDraggedTable(int x, int y);
  void SetDragCursor(diagram_hit_type dght);
  int find_free_ypos(table_in_diagram * table1, int pos, bool right_side);
  line_in_diagram * make_z_line(table_in_diagram * table1, int pos1, table_in_diagram * table2, int pos2, int x1, int x2);
  line_in_diagram * make_c_line(table_in_diagram * table1, int pos1, table_in_diagram * table2, int pos2, int x1, int x2);
  bool contains_table(ttablenum tbnum, int * index = NULL);
  line_in_diagram * create_connecting_line(table_in_diagram * table1, int pos1, table_in_diagram * table2, int pos2, t_ending_type et1, t_ending_type et2);
  void connect_tables(ttablenum tbnum1, const char * key1, ttablenum tbnum2, const char * key2, t_ending_type et1, t_ending_type et2);
 protected:
  DiagrTableList tables;
  DiagrLineList lines;
  int drag_item, drag_itempart; 
  bool designer_opened;
  void add_table_lines(table_in_diagram * tid);
  bool PasteTables(int x, int y, bool real_paste);
  void RemoveTable(int item);
  void init(void);
 public:
  void OnMouseEvent(wxMouseEvent& event);
 protected:
  //void OnPaint(wxPaintEvent& event);
  void OnDraw(wxDC& dc);
  virtual void set_changed(void) = 0;
  char * make_source(void);
  bool compile(const char * src);
  void clear_all(void);
  void Refresh_tables_and_lines(void);
  void set_virtual_size_by_contents(void);
  tobjname base_schema_name;  // this name is replaced by `` in the source form and subsituted for `` in compilation

 public:
  table_panel(cdp_t pcdpIn, const char * base_schema_nameIn = "");
  ~table_panel(void);
  bool insert_tables(int x, int y, C602SQLObjects * m_Objs, bool real_paste);
  void update_virtual_size(table_in_diagram * tid);
  static wxPrintData * g_printData;
  static wxPageSetupDialogData * g_pageSetupData;
};

bool is_end(const char *& pstr);
bool read_int(const char *& pstr, int & val);
bool read_ident(const char *& pstr, char * ident);

#include <wx/print.h>

class DiagramPrintout : public wxPrintout
{ int width_pages, height_pages;
  int pagepxx, pagepxy;
  int prippix, prippiy, scrppix, scrppiy;  // PPI for screen and the printer, used for scaling dimensions in pixels
  int margin_l, margin_r, margin_t, margin_b;
  table_panel * panel;
 public:
  wxPrintPreview * my_preview;  // not owning
  DiagramPrintout(wxString title, table_panel * panelIn) : wxPrintout(title), panel(panelIn)
    { my_preview=NULL; }
  bool HasPage(int pageNum)
    { return pageNum <= width_pages * height_pages; }
  void OnPreparePrinting(void);
  void GetPageInfo(int *minPage, int *maxPage, int *pageFrom, int *pageTo);
  bool OnPrintPage(int pageNum);
};

