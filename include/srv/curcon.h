/****************************************************************************/
/* curcon.h - Construction of cursors                                       */
/****************************************************************************/
#define BITMAPSIZE 300
/************************ cursor definition *********************************/
#define LOWER_BOUND 1  /* values of tab_acc_plan.type */
#define UPPER_BOUND 2
#define PURE_POINT  4
#define POINT_BOUND 7

typedef enum { NS_NORMAL, NS_FULL, NS_HALF } node_state;

class cur_descr;

#include "btree.h"
#include "sql2.h"

extern uns64 global_cursor_inst_counter;

class cur_descr    /* cursor core descriptor */
{public:
  cdp_t cdp;                 // owning client cdp
  d_table * d_curs;          // descriptor of columns in the cursor
  t_client_number owner;     // owning client number 
  bool persistent;           // if [persistent] then cursor is not closed when closing cursor on the end of the client-side program

  unsigned  tabcnt;          // number of record numbers in tables every record in the cursor maps to
  ttablenum * tabnums;       // list of table numbers (for UNION, INTERSECT, EXCEPT only the 1st operand is stored), indexed by kont.ordnum
  BOOL      insensitive;   /* TRUE: cursor data are in a temporary table (may have pointers on the top) */
                           /* FALSE:cursor contains references into tables */
  t_qe_table * underlying_table;// the only table in the cursor (not a cursor temp. table) or NULL
  t_mater_ptrs * pmat;     // pointer materialization owned by the top qe
  ttablenum own_table;     // table materialization, deined only if pmat==NULL, may not be from the top qe
/* holding records (references): */

  BOOL      subcurs;
  tcursnum  super;       /* supercursor number, defined only for subcursors */

  t_query_expr * qe;

  trecnum   recnum;      /* number of records selected in cursor */
  BOOL      complete;    /* FALSE iff can add other records */
  t_optim   * opt;       // used by delayed completion
  tptr      source_copy;
  tobjname  cursor_name; // used by positioned statements
  int       generation;  // distinguishes cursors with the same name and owner opened in recursive procedures
  trecnum   position;    // used by FETCH only
  uns32     hstmt;       // binds the cursor to the host variables and dyn. params
  t_translog_cursor translog;
  const uns64 global_cursor_inst_num;
  bool      fictive_cursor;
  bool      is_saved_position;
  trecnum   saved_position[MAX_OPT_TABLES];

  void curs_seek(trecnum recnum);
  BOOL curs_eseek(trecnum rec);
  BOOL curs_seek(trecnum crec, trecnum * recnums);
  
  inline table_descr * own_table_descr(void) { return tables[own_table]; }  // can be used only on cursors with own_table defined

  int  cursor_record_state(trecnum rec)
  { if (pmat!=NULL) 
    { if (rec>=recnum) enlarge_cursor(rec+1);
      return pmat->cursor_record_state(cdp, rec); // pointers on the top
    }
    return table_record_state(cdp, own_table_descr(), rec);
    // Can be replaced by checking for OUT_OF_CURSOR only if records cannot be deleted in t_mater_table.
  }

  void enlarge_cursor(trecnum limit);
  void release_opt(void)
  { if (opt!=NULL) { opt->Release();  opt=NULL; } }

  cur_descr(cdp_t cdpIn, t_query_expr * qeIn, t_optim * optIn) : 
    translog(cdpIn), global_cursor_inst_num(global_cursor_inst_counter++) // adds itself into the subtrans list in the cdp
  { d_curs =NULL;
    owner=cdpIn->applnum_perm;
    cdp=cdpIn;
    persistent=false;
    qe=qeIn;  opt=optIn;   
    if (qe)  qe->AddRef();  
    if (opt) opt->AddRef();
    subcurs=FALSE;
    tabnums=NULL;
    underlying_table=NULL;
    // insensitive, complete, recnum not initialized, defined in create cursor
    source_copy=NULL;
    pmat=NULL;  // not owning!
    own_table=NOTBNUM;  // not necessary, but to be safe
    cursor_name[0]=0;
    generation=0;
    hstmt=0;
    fictive_cursor=false;
    is_saved_position=false;
  }
  ~cur_descr(void);
  void mark_core(void);  // marks itself too
  void mark_disk_space(cdp_t cdp) const
  { if (pmat) pmat->mark_disk_space(cdp);
    if (opt)  opt ->mark_disk_space(cdp);
    if (qe)   qe  ->mark_disk_space(cdp);
  }
};

class t_fictive_current_cursor
// Constructor sets a new current_cursor in the cdp, destructor returns the original one.
{ cdp_t cdp;
  cur_descr * cd, * upper_current_cursor;
  bool changed;
 public:
  t_fictive_current_cursor(cdp_t cdpIn) : cdp(cdpIn)
  { if (cdp->current_cursor) changed=false;
    else
    { cd = new cur_descr(cdpIn, NULL, NULL);  // get a new global_cursor_inst_num
      cd->fictive_cursor=true;
      upper_current_cursor = cdp->current_cursor;  cdp->current_cursor = cd; 
      changed=true;
    }
  }
  ~t_fictive_current_cursor(void)
    { if (changed) 
      { cdp->current_cursor = upper_current_cursor; 
        delete cd;
      }
    }
};


BOOL cursor_locking(cdp_t cdp, int opc, cur_descr * cd, trecnum recnum);

// tabnums contains used table numbers, -1 for temporary table,
// 0x8000 for UNION flag position, 0x8001 for INTERSEC/EXCEPT placeholders
// (additional tables used in op2 above tables in op1).

// cd->qe contains always top qe, it may be the additional ptrmat ident qe.
// When top qe has ptrmat, cd->pmat points to it, cd->pmap==NULL otherwise.
// cd->qe may not have materialization (non-grp SELECT over grp SELECT).
// cd->own_table is defined only for cd->pmat==NULL and is used only for 
//   checking the status of cursor records.

#define PTRS_CURSOR(cd)         ((cd)->pmat!=NULL)

tptr assemble_subcursor_def(const char * subinfo, char * orig_source, BOOL from_table);
cur_descr * get_cursor(cdp_t cdp, tcursnum cursnum);
tcursnum find_named_cursor(cdp_t cdp, const char * name, cur_descr *& rcd);

SPECIF_DYNAR(crs_dynar,cur_descr*,50);
extern crs_dynar crs;

void free_cursor(cdp_t cdp, tcursnum num);
tcursnum create_cursor(cdp_t cdp, t_query_expr * qe, tptr source, t_optim * opt, cur_descr ** pcd);
unsigned count_cursors(t_client_number applnum);
void make_complete(cur_descr * cd);
void assign_name_to_cursor(const cdp_t cdp, const char * new_cursor_name, cur_descr * cd);
trecnum new_record_in_cursor(cdp_t cdp, trecnum trec, cur_descr * cd, tcursnum crnm);

//short compile_sql_query(tptr query, tptr subcond, cur_descr * cd);
//short indnum(char * object, ttablenum tbnum);
BOOL is_null_key(const char * key, const dd_index * idx_descr);

//select_upart * compile_select(CIp_t CI, select_upart * upper_upart);
//BOOL optimize_query(cdp_t cdp, select_upart * first_upart);
BOOL find_key(cdp_t cdp, tblocknum root, dd_index * idx_descr, tptr key, trecnum * precnum, bool shared_index);
BOOL distinctor_add(cdp_t cdp, dd_index * index, tptr key, t_translog * translog);
//BOOL compare_uparts(select_upart * upart, select_upart * upart0);
tcursnum sql_catalog_query(cdp_t cdp, tptr params);
tcursnum open_working_cursor(cdp_t cdp, char * query, cur_descr ** pcd);
void close_working_cursor(cdp_t cdp, tcursnum cursnum);
BOOL write_insert_rec_ex(cdp_t cdp, BOOL inserting, cur_descr * cd, tcursnum crnm, table_descr * tbdf, trecnum recnum, int col_count, const tattrib * cols, const char * databuf);
BOOL write_one_column(cdp_t cdp, table_descr * tbdf, trecnum recnum, tattrib colnum, 
                      const char * val, uns32 offset, uns32 size);


