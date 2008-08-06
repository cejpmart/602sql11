/****************************************************************************/
/* Kurzor.h - implements the kurzor operations (header)                     */
/****************************************************************************/
#define JOURMARK 0xdc
struct jourheader
{ uns32   spec_size;
  uns32   timestamp;
  tobjnum usernum;
};
extern tptr jourbuf;
BOOL jour_init(void);
void jour_close(void);
extern uns32 global_journal_pos;
extern int  task_switch_enabled;

////////////////////////// reporting progress of long operations /////////////////////////////////
void report_state(cdp_t cdp, trecnum rec1, trecnum rec2);
inline void report_step(cdp_t cdp, trecnum current, trecnum total = NORECNUM)
{ if (cdp->report_modulus && (current % cdp->report_modulus) == cdp->report_modulus-1)
    report_state(cdp, current, total);
}
inline void report_total(cdp_t cdp, trecnum total, trecnum total2 = NORECNUM)
{ if (cdp->report_modulus) report_state(cdp, total, total2); }
inline void report_step_reversed(cdp_t cdp, trecnum num1, trecnum num2)
{ if (cdp->report_modulus && (num2 % cdp->report_modulus) == cdp->report_modulus-1)
    report_state(cdp, num1, num2);
}
inline void report_step_sum(cdp_t cdp, trecnum num1, trecnum num2)
{ if (cdp->report_modulus && ((num1+num2) % cdp->report_modulus) == cdp->report_modulus-1)
    report_state(cdp, num1, num2);
}

////////////////////////////////////////// errors /////////////////////////////////////////////
char * form_message(char * buf, int bufsize, const char * msg, ...);
char * form_terminal_message(char * buf, int bufsize, const char * msg, ...);
char * trans_message(const char * msg);
void WINAPI request_compilation_error(cdp_t cdp, int i);
BOOL WINAPI request_error(cdp_t cdp, int i);
void WINAPI warn(cdp_t cdp, int i);
void WINAPI err_msg(cdp_t cdp, int i);  // creates server error message but does not rollback
inline void request_error_no_context(cdp_t cdp, int errnum)
{ request_compilation_error(cdp, errnum);
 // if there is a compilation context from previous errors, clear it:
  if (cdp->comp_kontext!=NULL) /* already allocated */ cdp->comp_kontext[0]=0; // clear compilation context
}
void request_generic_error(cdp_t cdp, int errnum, const char * msg);
void make_exception_from_error(cdp_t cdp, int errnum);

void std_nd_seek(ttablenum tb, trecnum recnum, trecnum * recnums);
BOOL commit(cdp_t cdp);
void roll_back(cdp_t cdp);
BOOL kernel_cdp_init(cdp_t cdp);
void kernel_cdp_free(cdp_t cdp);
BOOL fast_kernel_read(cdp_t cdp, tcursnum tbnum, trecnum position, uns8 attr,
                  tptr databuf);
tptr ker_load_objdef(cdp_t cdp, table_descr * tbdf, trecnum recnum);
tptr ker_load_blob(cdp_t cdp, table_descr * tbdf, trecnum recnum, tattrib attr, uns32 * size);
void logged_user_name(cdp_t cdp, tobjnum luser, char * user_name);
BOOL set_new_password(cdp_t cdp, tptr username, tptr password);
BOOL set_new_password2(cdp_t cdp, tobjnum userobj, tptr password, BOOL is_sysname);

void var_jour(cdp_t cdp, ttablenum tb, trecnum recnum, tattrib attrib, uns16 index,
              uns32 start, uns32 size, const void * data);
extern bool replaying;

/* functions from other modules: */
void cursor_init(void);
BOOL restoring_actions(cdp_t cdp);  /* no need to export */
void compact_database(cdp_t cdp, sig32 margin);

tobjnum find2_object(cdp_t cdp, tcateg categ, const uns8 * appl_id, const char * objname);
#define find_object(cdp,categ,objname) find2_object(cdp,categ,NULL,objname)
bool ker_apl_name2id(cdp_t cdp, const char * name, uns8 * appl_id, tobjnum * aplobj);
bool ker_apl_id2name(cdp_t cdp, const uns8 * appl_id, char * name, trecnum * aplobj);
void mark_schema_cache_dynar(void);
void ker_set_application(cdp_t cdp, const char * appl_name, tobjnum & aplobj);
void clear_all_passwords(cdp_t cdp);
void close_sequence_cache(cdp_t cdp);
BOOL get_own_sequence_value(cdp_t cdp, tobjnum seq_obj, int nextval, t_seq_value * val);
void reset_sequence(cdp_t cdp, tobjnum seq_obj); // removes the sequence from the cache, called on DROP and ALTER
void mark_sequence_cache(void);
void save_sequence(cdp_t cdp, tobjnum seq_obj); // stores the sequence values

#include "rights.h"

void write_to_log(cdp_t cdp, const char * text);

///////////////////////// monitoring
BOOL define_trace_log(cdp_t cdp, tobjname log_name, const char * pathname, const char * format);
void close_trace_logs(void);
void return_recent_log(cdp_t cdp, uns32 size_limit);
BOOL trace_def(cdp_t cdp, uns32 situation, tobjname username, char * objname, tobjname log_name, int kontext_extent);
void WINAPI trace_msg_general(cdp_t cdp, unsigned trace_type, const char * err_text, tobjnum objnum);  // creates server trace message 
void log_write_ex(cdp_t cdp, const char * err_text, const char * log_name, int kontext_extent);
BOOL is_trace_enabled_global(unsigned trace_type);
uns32 trace_map(tobjnum user_objnum);
void reduce_schema_name(cdp_t cdp, char * schema);

const char * enum_server_messages(int & pos);
void write_to_server_console(cdp_t cdp, const char * text);

// replication
void repl_start(cdp_t cdp);
void repl_stop();
void repl_operation(cdp_t cdp, int optype, int opparsize, char * opparam);
void mark_replication(void);
table_descr *GetReplDelRecsTable(cdp_t cdp);
void ResetDelRecsTable();

#define RDR_SCHEMAUUID 1
#define RDR_TABLENAME  2
extern tattrib RDR_W5_UNIKEY;
extern tattrib RDR_UNIKEY;
extern tattrib RDR_ZCR;
extern tattrib RDR_DATETIME;
bool web_login(cdp_t cdp, char * username, char * pwd);

