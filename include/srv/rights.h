#include <assert.h>

////////////////////////////////////////////// t_privil_version ///////////////////////////////////////////
typedef unsigned t_priv_version_number;

class t_privil_version
// The counter of priviledge state, increased on every change in global priviledges
// Used to invalidate cached priviledges after any change.
{ t_priv_version_number version_number;
 public:
  t_privil_version(void)
    { version_number=0; }
  inline void new_privil_version(void)
    { version_number++; }
  inline t_priv_version_number get_current_privil_version(void) const
    { return version_number; }
  inline BOOL check_privil_version(const t_priv_version_number & stored_version) const
    { return stored_version==version_number; }
};

extern t_privil_version privil_version;

//////////////////////////////////////////// UUIDs of system subjects /////////////////////////////////////
extern const WBUUID data_adm_uuid;
extern const WBUUID anonymous_uuid;
extern const WBUUID everybody_uuid;
extern const WBUUID conf_adm_uuid;
extern const WBUUID secur_adm_uuid;

typedef struct
{ WBUUID uuid;
  trecnum recnum;
} user_key;

//////////////////////////////////////////// class privils ///////////////////////////////////////////////
////////////////////////////////////////// various chaches of privils ///////////////////////////////////////////////////
class t_privil_test_result_cache 
// Caches the TRUE, FALSE or UNKNOWN value related to the subject and then privil version.
// Used in the INSERT, CALL and DELETE statements for caching global privils (privil testing result).
// Must not be used in admin mode!!
{ tobjnum               priv_user;     // NOOBJECT if privils not checked yet (does not have the positive nor the negative result)
  t_priv_version_number priv_version;  // defined only if priv_user!=NOOBJECT
  BOOL                  result_is_positive;  // defined only if priv_user!=NOOBJECT
 public:
  inline t_privil_test_result_cache(void)
   { priv_user=NOOBJECT; }
  inline BOOL has_valid_positive_test_result(cdp_t cdp)
   { return (priv_user==cdp->prvs.luser() && result_is_positive && privil_version.check_privil_version(priv_version)); }
  inline void store_positive_test_result(cdp_t cdp)
   { priv_user    = cdp->prvs.luser();
     priv_version = privil_version.get_current_privil_version();
     result_is_positive = TRUE;
   }
  inline BOOL has_valid_negative_test_result(cdp_t cdp)
   { return (priv_user==cdp->prvs.luser() && !result_is_positive && privil_version.check_privil_version(priv_version)); }
  inline void store_negative_test_result(cdp_t cdp)
   { priv_user    = cdp->prvs.luser();
     priv_version = privil_version.get_current_privil_version();
     result_is_positive = FALSE;
   }
};
 
class t_privil_cache // used in kontexts of permanent tables for caching column global priviledges
{protected: 
  tobjnum               priv_user;     // NOOBJECT if privils not checked yet
  t_priv_version_number priv_version;  // defined only if priv_user!=NOOBJECT
  t_privils_flex        global_privils;
 public:
  inline t_privil_cache(void)
   { priv_user=NOOBJECT; }
  inline void load_global_privils(cdp_t cdp, table_descr * tbdf)
  { cdp->prvs.get_effective_privils(tbdf, NORECNUM, global_privils);
    priv_user    = cdp->prvs.luser();
    priv_version = privil_version.get_current_privil_version();
  }
  inline virtual BOOL can_read_column(cdp_t cdp, tattrib column, table_descr * tbdf, trecnum recnum)
  { if (priv_user!=cdp->prvs.luser() || !privil_version.check_privil_version(priv_version))
      load_global_privils(cdp, tbdf);
    return global_privils.has_read(column);
  }
  virtual bool have_local_rights_to_columns(cdp_t cdp, table_descr * tbdf, trecnum recnum, t_atrmap_flex & map)
    { return false; }
  inline virtual BOOL can_write_column(cdp_t cdp, tattrib column, table_descr * tbdf, trecnum recnum)
  { if (priv_user!=cdp->prvs.luser() || !privil_version.check_privil_version(priv_version))
      load_global_privils(cdp, tbdf);
    return global_privils.has_write(column);
  }
  bool can_reference_columns(cdp_t cdp, t_atrmap_flex & map, table_descr * tbdf, t_atrmap_flex * problem_map);
  virtual void mark_core(void)
    { global_privils.mark_core(); }
};

class t_record_privil_cache : public t_privil_cache
// used in kontexts of permanent tables with record privils for caching column priviledges
{ trecnum positioned_on_record;  // NORECNUM if privils not loaded
  t_privils_flex record_privils;
 public:
  inline t_record_privil_cache(void)
    { positioned_on_record = NORECNUM; }
  inline void load_record_privils(cdp_t cdp, table_descr * tbdf, trecnum recnum)
  { cdp->prvs.get_effective_privils(tbdf, recnum, record_privils);
    priv_user    = cdp->prvs.luser();
    priv_version = privil_version.get_current_privil_version();
    positioned_on_record = recnum;
  }
  virtual BOOL can_read_column(cdp_t cdp, tattrib column, table_descr * tbdf, trecnum recnum)
  { if (priv_user!=cdp->prvs.luser() || !privil_version.check_privil_version(priv_version))
    { load_global_privils(cdp, tbdf);
      positioned_on_record = NORECNUM;
    }
    if (global_privils.has_read(column)) return TRUE;
    if (positioned_on_record != recnum)
      load_record_privils(cdp, tbdf, recnum);
    return record_privils.has_read(column);
  }
  virtual bool have_local_rights_to_columns(cdp_t cdp, table_descr * tbdf, trecnum recnum, t_atrmap_flex & map);
  virtual BOOL can_write_column(cdp_t cdp, tattrib column, table_descr * tbdf, trecnum recnum)
  { if (priv_user!=cdp->prvs.luser() || !privil_version.check_privil_version(priv_version))
    { load_global_privils(cdp, tbdf);
      positioned_on_record = NORECNUM;
    }
    if (global_privils.has_write(column)) return TRUE;
    if (positioned_on_record != recnum)
      load_record_privils(cdp, tbdf, recnum);
    return record_privils.has_write(column);
  }
  virtual void mark_core(void)
    { t_privil_cache::mark_core();  record_privils.mark_core(); }
};

tptr load_var_val(cdp_t cdp, table_descr * tbdf, trecnum recnum, tattrib atr, unsigned * size);

BOOL user_and_group(cdp_t cdp, tobjnum user_or_group, tobjnum group_or_role, tcateg subject2, t_oper oper, uns8 * state);
BOOL remove_app_roles(cdp_t cdp, tobjnum applobj);
void remove_refs_to_all_roles(cdp_t cdp, tobjnum user_or_group);
BOOL check_all_right(cdp_t cdp, tcursnum curs, BOOL write_rg);
tobjnum role_from_uuid(const WBUUID p_uuid);
bool get_effective_membership(cdp_t cdp, tobjnum subject, tcateg subject_categ, tobjnum container, tcateg cont_categ);
bool can_delete_record(cdp_t cdp, table_descr * tbdf, trecnum recnum);
bool can_insert_record(cdp_t cdp, table_descr * tbdf);
bool can_read_objdef(cdp_t cdp, table_descr * tbdf, trecnum recnum);
bool can_write_objdef(cdp_t cdp, table_descr * tbdf, trecnum recnum);

