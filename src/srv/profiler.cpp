// profiler.cpp
#ifdef WINS
#include <windows.h>
#else
#include "winrepl.h"
#endif
#include "sdefs.h"
#include "scomp.h"
#include "basic.h"
#include "frame.h"
#include "kurzor.h"
#include "dispatch.h"
#include "table.h"
#include "dheap.h"
#include "curcon.h"
#pragma hdrstop
#include "profiler.h"
#ifdef LINUX
#include <sys/time.h>
#endif
/* The logic:
During execution of every request the lock_waiting_time if counted when waiting in wait_record_lock_error() and lock_all_index_roots() functions.
This time is used for calculaton of the thread execution netto time only.
*/
bool profiling = false;       // structures created, but nobody may be profiled in the momment
bool profiling_all = false;   // all threads being profiled
bool profiling_lines = true;  // creating line profiles in routines

enum { PROF_INITIAL_TABLE_SIZE = 100, PROF_TABLE_SIZE_LIMIT = 10000 };
unsigned prof_table_size;
unsigned prof_table_entries;
t_prof_rec * prof_hash_table = NULL;

unsigned prof_hash(t_profclass profclass, tobjnum objnum, int line, const char * source)
{ 
  if (source)
  { int i=0, hash=0;
    while (i<50 && source[i]) 
    { if (source[i]>=' ') hash+=source[i];
      i++;
    }
    return hash % prof_table_size;
  }
  else
    return (7*profclass+131*objnum+1501*line) % prof_table_size;
}

bool t_prof_rec::same(t_profclass profclass2, tobjnum objnum2, int line2, const char * source2) const
{ if (profclass!=profclass2 || objnum!=objnum2 || line!=line2) return false;
 // either both source and source2 are NULL or none is NULL:
  if (!source) return !source2;  // I want to be safe...
  if (!source2) return false;
  const char * p = source;
  while (*source2)
    if (*source2<' ') source2++;
    else if (*p==*source2) { p++;  source2++; }
    else return false;
  return !*p;
}

bool prof_init(void)
{ ProtectedByCriticalSection cs(&cs_profile);
  if (prof_hash_table)  // deallocate all [source]s
  { for (int i=0;  i<prof_table_size;  i++)
      corefree(prof_hash_table[i].source);
  }
  else
  { prof_hash_table = (t_prof_rec*)corealloc(sizeof(t_prof_rec) * PROF_INITIAL_TABLE_SIZE, 49);
    if (!prof_hash_table) return false;
    prof_table_size = PROF_INITIAL_TABLE_SIZE;
  }
  memset(prof_hash_table, 0, sizeof(t_prof_rec) * prof_table_size);  // writes PROFCLASS_NONE into every [profclass] and NULL into every [source]
  prof_table_entries=0;
  profiling=true;
  return true;
}

void prof_mark_core(void)
{
  if (prof_hash_table)  // deallocate all [source]s
  { mark(prof_hash_table);
    for (int i=0;  i<prof_table_size;  i++)
      if (prof_hash_table[i].source)
        mark(prof_hash_table[i].source);
  }
}

bool prof_rehash(void)
{ if (prof_table_size >= PROF_TABLE_SIZE_LIMIT) return false;
 // allocate the new table
  t_prof_rec * prof_hash_table_2 = (t_prof_rec*)corealloc(sizeof(t_prof_rec) * 2*prof_table_size, 49);
  if (!prof_hash_table_2) return false;
  memset(prof_hash_table_2, 0, sizeof(t_prof_rec) * 2*prof_table_size);  // writes PROFCLASS_NONE into every [profclass] and NULL into every [source]
  unsigned orig_prof_table_size = prof_table_size;
  prof_table_size *= 2;  // used by the prof_hash() for the new table
 // move the contents:
  for (int i = 0;  i<orig_prof_table_size;  i++)
    if (prof_hash_table[i].valid())
    { unsigned hash = prof_hash(prof_hash_table[i].profclass, prof_hash_table[i].objnum, prof_hash_table[i].line, prof_hash_table[i].source); 
     // research for an empty cell:
      while (prof_hash_table_2[hash].valid())
        if (++hash >= prof_table_size) hash=0;
     // copy (incl. pointer):
      prof_hash_table_2[hash]=prof_hash_table[i];
    }
 // replace tables:
  corefree(prof_hash_table);
  prof_hash_table = prof_hash_table_2;
  return true;
}

t_prof_rec * prof_get(t_profclass profclass, tobjnum objnum, int line, const char * source)
// Returns the profile record for the given parameters (creates it if it does not exist).
// [profiling]==true supposed. Returns NULL on memory error.
{// search the item: 
  unsigned hash = prof_hash(profclass, objnum, line, source);
  while (prof_hash_table[hash].valid())
  { if (prof_hash_table[hash].same(profclass, objnum, line, source))
      return prof_hash_table+hash;
    if (++hash >= prof_table_size) hash=0;
  }
 // entry not found, insert it:
  if (prof_table_entries > prof_table_size*4/5)
  { if (!prof_rehash())
    { profiling_all = false;  // reduces similiar errors
      return NULL;  // memory allocation error
    }  
   // research for an empty cell:
    hash = prof_hash(profclass, objnum, line, source);
    while (prof_hash_table[hash].valid())
      if (++hash >= prof_table_size) hash=0;
  }
 // write to [hash] position:
  if (source)
  { prof_hash_table[hash].source = (char*)corealloc(strlen(source)+1, 49);
    if (!prof_hash_table[hash].source) return NULL;
    int i=0, j=0;
    while (source[i])
    { if (source[i]>=' ') prof_hash_table[hash].source[j++]=source[i];
      i++;
    }
    prof_hash_table[hash].source[j]=0;
  }
  else
    prof_hash_table[hash].source = NULL;
  prof_hash_table[hash].profclass=profclass;
  prof_hash_table[hash].objnum=objnum;
  prof_hash_table[hash].line=line;
  prof_hash_table[hash].hits=0;
  prof_hash_table[hash].brutto_time=0;
  prof_hash_table[hash].subtr_time=0;
  prof_hash_table[hash].wait_time=0;
  prof_table_entries++;
  return prof_hash_table+hash;
}

void add_hit_and_time(prof_time time_int, t_profclass profclass, tobjnum objnum, int line, const char * source)
// [line] must be 0 when line number does not apply to the [profclass].
// [source] must be NULL when source line does not apply to the [profclass].
{ ProtectedByCriticalSection cs(&cs_profile);
  t_prof_rec * pr = prof_get(profclass, objnum, line, source);
  if (!pr) return;
  pr->hits++;
  if (time_int>0)  // unless overflow
    pr->brutto_time += time_int;
}

void add_hit_and_time2(prof_time time_int, prof_time time_sub, prof_time wait_time, t_profclass profclass, tobjnum objnum, int line, const char * source)
// [line] must be 0 when line number does not apply to the [profclass].
// [source] must be NULL when source line does not apply to the [profclass].
{ ProtectedByCriticalSection cs(&cs_profile);
  t_prof_rec * pr = prof_get(profclass, objnum, line, source);
  if (!pr) return;
  pr->hits++;
  if (time_int>0 && time_sub>=0)  // unless overflow
  { pr->brutto_time += time_int;
    pr->subtr_time += time_sub;
    pr->wait_time += wait_time;
  }
}

void add_prof_time(prof_time time_int, t_profclass profclass, tobjnum objnum, int line, const char * source)
{ ProtectedByCriticalSection cs(&cs_profile);
  t_prof_rec * pr = prof_get(profclass, objnum, line, source);
  if (!pr) return;
  if (time_int>0)  // unless overflow
    pr->brutto_time += time_int;
}

void add_prof_time2(prof_time time_int, prof_time time_sub, prof_time wait_time, t_profclass profclass, tobjnum objnum, int line, const char * source)
// [line] must be 0 when line number does not apply to the [profclass].
// [source] must be NULL when source line does not apply to the [profclass].
{ ProtectedByCriticalSection cs(&cs_profile);
  t_prof_rec * pr = prof_get(profclass, objnum, line, source);
  if (!pr) return;
  if (time_int>0 && time_sub>=0)  // unless overflow
  { pr->brutto_time += time_int;
    pr->subtr_time += time_sub;
    pr->wait_time += wait_time;
  }
}

prof_time get_prof_time(void)
{ 
#ifdef WINS
  LARGE_INTEGER tm;
  if (QueryPerformanceCounter(&tm))   
    return tm.QuadPart;
  return GetTickCount();
#else
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return 1000*(uns64)tv.tv_sec + (uns64)(tv.tv_usec/1000);
#endif
}

void profile_line(cdp_t cdp, uns32 line)
{
  if (line!=cdp->last_line_num && cdp->top_objnum!=NOOBJECT)
  { prof_time curr_time = get_prof_time();
    if (cdp->last_line_num != (uns32)-1)  // unless the 1st break after the execution started
      add_hit_and_time(curr_time-cdp->time_since_last_line, PROFCLASS_LINE, cdp->top_objnum, cdp->last_line_num, NULL);
    cdp->time_since_last_line = curr_time;
  }
}

/////////////////////////////////////////// profiling results ////////////////////////////////////////////////
static const char * profile_category_names[] =  // indexed by t_profclass
{ "Critical section", "Record lock", "Index page lock", "Routine", "SQL statement", "Thread", "Activity", "Trigger", "Source line", "SELECT expression" };

static const char * cs_names[] = // indexed by t_wait from WAIT_CS_SEQUENCES
{ "Sequences", "Pieces", "Client list", "Trace logs", "Cursors", "Database file", "Commit", "Block pools", "Frames",
  "Transactor", "Privils", "Memory", "Tables", "PSM cache", "Locks", "Short term", "Deadlock", "Ext. fulltext",
  "Dadr lock", "Impossible" };

static const char * activity_names[] =  // indexed by t_prof_activity
{ "Disc read", "Disc write", "Commit", "Index lock retry", "Update index" };

sig64 get_prof_koef(void)
{ // Returns the number of profiler ticks per second
#ifdef WINS
  LARGE_INTEGER tm;
  if (QueryPerformanceFrequency(&tm))   
    return tm.QuadPart;
#endif
  return 1000;  // fixed in Linux, fallback in Windows
}

void fill_profile(cdp_t cdp, table_descr * tbdf)
// must use dynamic structores because of variable-size [log_message]
{ tobjnum objnum;  tobjname objname; int line;  unsigned hit, brutto, netto, active;
  uns32 sql_length;
  bool saved_profiling = profiling;  profiling=false;
 // find koeficient for the frequency:
  sig64 koef = get_prof_koef();
 // prepare the vector:
  t_colval profile_coldescr[10];
  memset(profile_coldescr, 0, sizeof(profile_coldescr));
  profile_coldescr[0].attr=1;
  profile_coldescr[0].dataptr=NULL;
  profile_coldescr[1].attr=2;
  profile_coldescr[1].dataptr=&objnum;
  profile_coldescr[2].attr=3;
  profile_coldescr[2].dataptr=objname;
  profile_coldescr[3].attr=4;
  profile_coldescr[3].dataptr=&line;
  profile_coldescr[4].attr=5;
  profile_coldescr[4].lengthptr=&sql_length;
  profile_coldescr[4].offsetptr=NULL;
  profile_coldescr[5].attr=6;
  profile_coldescr[5].dataptr=&hit;
  profile_coldescr[6].attr=7;
  profile_coldescr[6].dataptr=&brutto;
  profile_coldescr[7].attr=8;
  profile_coldescr[7].dataptr=&netto;
  profile_coldescr[8].attr=9;
  profile_coldescr[8].dataptr=&active;
  profile_coldescr[9].attr=NOATTRIB;
  t_vector_descr vector(profile_coldescr);
 // insert records:
  for (int i = 0;  i<prof_table_size;  i++)
    if (prof_hash_table[i].valid())
    { const t_prof_rec * pr = prof_hash_table+i;
     // describe the object:
      profile_coldescr[0].dataptr=profile_category_names[pr->profclass-1];
      if (pr->profclass==PROFCLASS_SQL || pr->profclass==PROFCLASS_SELECT)
      { objnum=NONEINTEGER;  objname[0]=0;  line=NONEINTEGER;
        profile_coldescr[4].dataptr = pr->source;
        sql_length=(int)strlen(pr->source);
      }
      else if (pr->profclass==PROFCLASS_THREAD)
      { if (pr->source && *pr->source)
          { strcpy(objname, pr->source);  objnum=NONEINTEGER; } 
        else
          { objname[0]=0;  objnum = pr->objnum; }
        line=NONEINTEGER;
        profile_coldescr[4].dataptr = NULLSTRING;
        sql_length=0;
      }
      else
      { objnum = pr->objnum;
        if (pr->profclass==PROFCLASS_CS)
          strcpy(objname, cs_names[objnum]);
        else if (pr->profclass==PROFCLASS_ACTIVITY)
          strcpy(objname, activity_names[objnum]);
        else if (objnum==NOOBJECT)
          strcpy(objname, "Distinctor");
        else
          tb_read_atr(cdp, pr->profclass==PROFCLASS_LINE || pr->profclass==PROFCLASS_ROUTINE || pr->profclass==PROFCLASS_TRIGGER ? 
                           objtab_descr : tabtab_descr, objnum, OBJ_NAME_ATR, objname);
        line = !pr->line ? NONEINTEGER : pr->line;
        profile_coldescr[4].dataptr = NULLSTRING;
        sql_length=0;
      }
     // add profile data:
      hit=pr->hits;
      brutto=(unsigned)( pr->brutto_time                *1000/koef);
      if (pr->profclass==PROFCLASS_SQL || pr->profclass==PROFCLASS_SELECT || pr->profclass==PROFCLASS_ROUTINE || pr->profclass==PROFCLASS_TRIGGER)
        netto = (unsigned)((pr->brutto_time-pr->subtr_time)*1000/koef);
      else  
        netto = NONEINTEGER;
      if (pr->profclass==PROFCLASS_SQL || pr->profclass==PROFCLASS_SELECT || pr->profclass==PROFCLASS_ROUTINE || pr->profclass==PROFCLASS_TRIGGER || pr->profclass==PROFCLASS_THREAD)
        active=(unsigned)((pr->brutto_time-pr->wait_time )*1000/koef);
      else
        active = NONEINTEGER;  
      tb_new(cdp, tbdf, FALSE, &vector);
    }
  profiling=saved_profiling;
}

