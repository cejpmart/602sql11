// profiler.h
#ifndef PROFILER_INCLUDED
#define PROFILER_INCLUDED

enum t_profclass { PROFCLASS_NONE=0, PROFCLASS_CS, PROFCLASS_REC_LOCK, PROFCLASS_PAGE_LOCK, 
                   PROFCLASS_ROUTINE, PROFCLASS_SQL, PROFCLASS_THREAD, PROFCLASS_ACTIVITY, 
                   PROFCLASS_TRIGGER, PROFCLASS_LINE, PROFCLASS_SELECT };   

enum t_prof_activity { PROFACT_DISC_READ, PROFACT_DISC_WRITE, PROFACT_COMMIT, 
                       PROFACT_INDEX_LOCK_RETRY, PROFACT_UPDATE_INDEX };
// activity names must be defined for all activities!

struct t_prof_rec
{// object identification:
  t_profclass profclass;  // always defined
  tobjnum objnum;         // 0 when not used (used for PROFCLASS_CS, PROFCLASS_REC_LOCK, PROFCLASS_PAGE_LOCK, PROFCLASS_ROUTINE, PROFCLASS_TRIGGER, PROFCLASS_LINE)
  int line;               // 0 when not used (user for PROFCLASS_LINE only)
  char * source;          // NULL when not used (used only for PROFCLASS_SQL, PROFCLASS_SELECT and for PROFCLASS_THREAD for named threads)
 // profile data:
  unsigned hits;
  sig64 brutto_time;   // time spent in the object
  sig64 subtr_time;    // time spent in nested objects
  sig64 wait_time;     // time spent waiting for locks

  bool same(t_profclass profclass2, tobjnum objnum2, int line2, const char * source2) const;
  inline bool valid(void) const
    { return profclass!=PROFCLASS_NONE; }  // "invalid" entry is not necessary
};

extern bool profiling;       // structures created, but nobody may be profiled in the momment
extern bool profiling_all;   // all threads being profiled
extern bool profiling_lines; // creating line profiles in routines

bool prof_init(void);

#define PROFILING_ON(cdp)    (profiling &&                    (profiling_all || cdp->profiling_thread))
#define PROFILING_LINES(cdp) (profiling && profiling_lines && (profiling_all || cdp->profiling_thread))

void add_hit_and_time(prof_time time_int, t_profclass profclass, tobjnum objnum, int line, const char * source);
void add_prof_time(prof_time time_int, t_profclass profclass, tobjnum objnum, int line, const char * source);
void add_hit_and_time2(prof_time time_int, prof_time time_sub, prof_time wait_time, t_profclass profclass, tobjnum objnum, int line, const char * source);
void add_prof_time2(prof_time time_int, prof_time time_sub, prof_time wait_time, t_profclass profclass, tobjnum objnum, int line, const char * source);
prof_time get_prof_time(void);
sig64 get_prof_koef(void);
void profile_line(cdp_t cdp, uns32 line);
// must set     cdp->last_line_num=line;  after the call!!

inline void PROFILE_AND_DEBUG(cdp_t cdp, uns32 source_line)
{ if (PROFILING_LINES(cdp)) profile_line(cdp, source_line);
  cdp->last_line_num=source_line;
  if (cdp->dbginfo) cdp->dbginfo->check_break(source_line);
  cdp->last_line_num = source_line;
}

inline void CHANGING_SCOPE(cdp_t cdp)
{ if (PROFILING_LINES(cdp)) profile_line(cdp, (uns32)-1);
  cdp->last_line_num=(uns32)-1;
}
#endif  // PROFILER_INCLUDED

