// Structures for remote procedure call between CINT and KURZOR

#pragma pack(1)
typedef struct
{ tobjnum user_group_role;
  uns8 subject_categ;
  ttablenum table;
  trecnum record;
  uns8 operation;
  uns8 privils[PRIVIL_DESCR_SIZE];  // will be longer when setting privileges for tables with many columns
} op_privils;

typedef struct
{ tobjnum user_or_group;
  tobjnum group_or_role;
  uns8 subject2;
  uns8 operation;
  uns32 relation;
} op_grouprole;

typedef struct
{ tcurstab curs;
  trecnum position;
  tattrib attr;
  uns8 operation;
  uns8 valtype;
} op_nextuser;

typedef struct     // login phase 1 returned structure
{ tobjnum userobj;
  uns32   count;
  uns32   just_logged;
} t_login_info;

struct t_op_send_par_pos
{ sig16 id;  
  trecnum recnum; 
};
struct t_op_send_par_name
{ sig16 id;  
  tcursnum cursnum;
  tobjname name;
  inline size_t size(void) 
    { return sizeof(t_op_send_par_name)-sizeof(tobjname)+strlen(name)+1; }
};
struct t_op_insobj  // parameter frame for OP_INSOBJ
{ tobjname objname;
  tcateg categ;
  wbbool limited;
};

struct t_op_create_user
{ tobjname     logname;
  t_user_ident info;
  WBUUID       user_uuid;
  WBUUID       home_srv;
  uns8         enc_password[sizeof(uns32)+MD5_SIZE];
};
#pragma pack()
/************************* The Sezam opcodes ********************************/
#define OP_READ        1
#define OP_WRITE       2
#define OP_APPEND      3
#define OP_RLOCK       4
#define OP_UNRLOCK     5
#define OP_WLOCK       6
#define OP_UNWLOCK     7
#define OP_DELETE      8
#define OP_UNDEL       9
#define OP_INSERT     10
#define OP_COMMIT     11
#define OP_START_TRA  12
#define OP_ROLL_BACK  13
#define OP_OPEN_SUBCURSOR 14
#define OP_RECSTATE   15
#define OP_LOGIN      16
#define OP_SIGNATURE  17
#define OP_SQL_CATALOG 18
#define OP_PRIVILS    19
#define OP_GROUPROLE  20
#define OP_REPLCONTROL      21
#define OP_WRITERECEX  22
#define OP_OPEN_CURSOR    23
#define OP_CLOSE_CURSOR   24
#define OP_LIST       25
#define OP_SYNCHRO    26
#define OP_FINDOBJ    27
#define OP_CNTRL      28
#define OP_TRANSF     29
#define OP_GENER_INFO       30
#define OP_RECCNT     31
#define OP_INDEX      32
#define OP_SAVE_TABLE 33
#define OP_REST_TABLE 34
#define OP_TRANSL     35
#define OP_BASETABS   36
#define OP_TABINFO    37
#define OP_INSDEL     38  // used only internally on the server
#define OP_INTEGRITY  39
#define OP_ACCCLOSE   40
#define OP_READREC    41
#define OP_WRITEREC   42
#define OP_ADDVALUE   43
#define OP_DELVALUE   44
#define OP_REMINVAL   45
#define OP_PTRSTEPS   46
#define OP_SETAPL     47
#define OP_VALRECS    48
#define OP_IDCONV     49
#define OP_SUPERNUM   50
#define OP_ADDREC     51
#define OP_TEXTSUB    52
#define OP_END        53
#define OP_BACKUP     54
#define OP_GETDBTIME  55
#define OP_REPLAY     56
#define OP_DEBUG      57 
#define OP_SUBMIT     58
#define OP_SENDPAR    59
#define OP_GETERROR   60
#define OP_AGGR       61
#define OP_NEXTUSER   62
#define OP_FULLTEXT   63
#define OP_INSERTRECEX 64
#define OP_PROPERTIES  65
#define OP_APPENDRECEX 66
#define OP_INSOBJ      67
#define OP_WHO_LOCKED  68

//#define OP_OP_INDEX        0
#define OP_OP_FREE         1
#define OP_OP_DELHIST      2
#define OP_OP_COMPAT       3
#define OP_OP_ENABLE       4
#define OP_OP_UNINST       5
#define OP_OP_COMPARE      7 
#define OP_OP_COMPACT      8
#define OP_OP_TRIGGERS     9
#define OP_OP_MAKE_PERSIS  10
#define OP_OP_TRUNCATE     11
#define OP_OP_ADMIN_MODE   12

/* suboperations of OP_CNTRL */
#define SUBOP_SET_FILSIZE   2
#define SUBOP_SAVE          3
#define SUBOP_RELOAD        4
#define SUBOP_LOCK          5
#define SUBOP_UNLOCK        6
#define SUBOP_BACK_COUNT    7
#define SUBOP_CATCH_FREE    8
#define SUBOP_SET_PASSWORD  9
#define SUBOP_CREATE_USER   10
#define SUBOP_USER_UNLOCK   11
#define SUBOP_SET_AUTHORING 12
#define SUBOP_KILL_USER     13
#define SUBOP_ADD_LIC_1     14
#define SUBOP_REMOVE_LOCK   15
#define SUBOP_SET_WAITING   16
#define SUBOP_GET_ERR_MSG   17
#define SUBOP_WHO_AM_I      18
#define SUBOP_TSE           19
#define SUBOP_REPLIC        20
#define SUBOP_SQLOPT        21
#define SUBOP_OPTIMIZATION  22
#define SUBOP_COMPACT       23
#define SUBOP_CDROM         24
#define SUBOP_REFINT        25
#define SUBOP_LOGWRITE      26
#define SUBOP_ADD_LIC_2     27
#define SUBOP_ISOLATION     28
#define SUBOP_SET_FILSIZE64 29
#define SUBOP_GET_LOGIN_KEY 30
#define SUBOP_APPL_INSTS    31
#define SUBOP_PARK_SETTINGS 32
#define SUBOP_MESSAGE       33
#define SUBOP_TRIG_UNREGISTER 34
#define SUBOP_PROC_CHANGED  35
#define SUBOP_REPORT_MODULUS 36
#define SUBOP_BACKUP        37
#define SUBOP_INTEGRITY     38
#define SUBOP_GET_ERR_CONTEXT 39
#define SUBOP_BREAK_USER    40
#define SUBOP_SQLOPT_GET    41
#define SUBOP_TRIG_REGISTER 42
#define SUBOP_CLOSE_CURSORS 43
#define SUBOP_SET_TZD       44
#define SUBOP_ROLLED_BACK   45
#define SUBOP_TRANS_OPEN    46
#define SUBOP_SET_WEAK_LINK 47
#define SUBOP_CHECK_INDECES 48
#define SUBOP_RESTART       49
#define SUBOP_STOP          50
#define SUBOP_WORKER_TH     51
#define SUBOP_TABLE_INTEGRITY     52
#define SUBOP_GET_ERR_CONTEXT_TEXT 53
#define SUBOP_MASK_DUPLICITY 54
#define SUBOP_LIC1          55
#define SUBOP_LIC2          56
#define SUBOP_CRASH         57
#define SUBOP_EXPORT_LOBREFS 58

// subops of OP_SYNCHRO:
#define OP_SYN_REG        0
#define OP_SYN_UNREG      1
#define OP_SYN_CANCEL     2
#define OP_SYN_WAIT       3

#define OP_GI_REPLICATION     1
// other defined in general.h

#define DATA_END_MARK  0xa9

#define C_SUM    208
#define C_MAX    182
#define C_MIN    183
#define C_AVG    136
#define C_COUNT  149

#define LOGIN_PAR_PREP    1
#define LOGIN_PAR_PASS    2
#define LOGIN_PAR_LOGOUT  3

#define LOGIN_PAR_SAME    5
#define LOGIN_PAR_WWW     6
#define LOGIN_PAR_SAME_DIFF 7
#define LOGIN_PAR_GET_COMP_NAME    10
#define LOGIN_PAR_IMPERSONATE_1    11
#define LOGIN_PAR_IMPERSONATE_2    12
#define LOGIN_PAR_IMPERSONATE_3    13
#define LOGIN_PAR_IMPERSONATE_STOP 14
#define LOGIN_PAR_KERBEROS_ASK     15
#define LOGIN_PAR_KERBEROS_DO      16
#define LOGIN_PAR_HTTP             17
#define CHALLENGE_SIZE      32
#define COMM_ENC_NONE   1
#define COMM_ENC_BASIC  2
#define LOGIN_CHALLENGE            20
#define LOGIN_ENC_KEY              21
#define LOGIN_LIMITED_FLAG       0x80
#define LOGIN_SIMULATED_FLAG     0x40
