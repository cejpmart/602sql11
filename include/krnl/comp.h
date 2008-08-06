/****************************************************************************/
/* The S-Pascal internal header file                                        */
/****************************************************************************/
/* compiler errors making the compiler stop: */
#include "compil.h"

/* suberrors on view compilation */
#define ERR_VIEW_HEADER 0
#define ERR_GROUP_DEF   1
#define ERR_VIEW_ITEM   2       /* copy in viewedit */
#define ERR_X_LAST      2

#define ERR_CLASS       0
#define ERR_X           1
#define ERR_Y           2
#define ERR_CX          3
#define ERR_CY          4
#define ERR_ID          5
#define ERR_STYLE       6
#define ERR_FORECOL     7
#define ERR_BACKCOL     8
#define ERR_FONT        9
#define ERR_HELP        10
#define ERR_CAPTION     11
#define ERR_NOGROUP     12
#define ERR_GROUPA      13 /* 14,15,16,17 */
#define ERR_ACCESS      18
#define ERR_VALUE       19
#define ERR_INTEGRITY   20
#define ERR_ENABLED     21
#define ERR_VISIBILITY  22
#define ERR_ONERROR     23
#define ERR_ACTION      24
#define ERR_ENUMER      25
#define ERR_PICTURE     26
#define ERR_DEFAULT     27
#define ERR_PRECISION   28
#define ERR_PANE        29
#define ERR_LABEL       30
#define ERR_COMBOSEL    31
#define ERR_TOOLBAR     32
#define ERR_STATUSTEXT  33
#define ERR_QUERYCOND   34
#define ERR_OPENACTION  35
#define ERR_CLOSEACTION 36
#define ERR_MASK        37
#define ERR_SPECIF      38
#define ERR_KEY_FLAGS   39
#define ERR_RCLICK      40
#define ERR_NAME        41
#define ERR_NEWRECACT   42
#define ERR_PROP        43
#define ERR_EVENT       44
#define ERR_DBLCLICK    45
#define ERR_UNDEF       46
#define ERR_Y_LAST      46
/*********************** menu compilation error subcodes ********************/
#define ERR_MENU_HEADING      0
#define ERR_MENU_ITEMNAME     1
#define ERR_MENU_RETVAL       2
#define ERR_MENU_STATEM       3
#define ERR_MENU_CHECKED      4
#define ERR_MENU_ACTIV        5
#define ERR_MENU_STATUSTEXT   6
#define ERR_MENU_COUNT        7  /* copy in messages.c */

/************************* opcodes of the internal code *********************/
#define I_JUMP         1
#define I_FALSEJUMP    2
#define I_INT8CONST    3
#define I_INT16CONST   4
#define I_INT32CONST   5
#define I_REALCONST    6
#define I_STRINGCONST  18
#define I_LOADADR      7
#define I_DEREF        8
#define I_SUBRADR      9
#define I_DUP          10
#define I_DROP         11

#define I_TOSI2F       12
#define I_TOSM2F       13
#define I_NOSI2F       14
#define I_NOSM2F       15
#define I_TOSI2M       16
#define I_NOSI2M       17
#define I_TOSM2I       61
#define I_NOSM2I       62

#define I_ACCESS       19
#define I_INTNEG       20
#define I_REALNEG      21
#define I_BOOLNEG      22
#define I_OR           23
#define I_AND          24
#define I_INTMOD       25
#define I_JUMP4        26
#define I_SCALL        27
#define I_BREAK        28
#define I_STOP         29
#define I_PREPCALL     30
#define I_CALL         31
#define I_FUNCTRET     32
#define I_PROCRET      33
#define I_STORE_STR    34
#define I_STORE1       35
#define I_STORE2       36
#define I_STORE4       37
#define I_STORE8       38
#define I_STORE        39
#define I_LOAD1        40
#define I_LOAD2        41
#define I_LOAD4        42
#define I_LOAD8        43

#define I_CURKONT      44
#define I_DBREAD       45
#define I_DBWRITE      46
#define I_FALSEJUMP4   47
#define I_SAVE         48
#define I_RESTORE      49
#define I_SAVE_DB_IND  50
#define I_DBREAD_INT   51
#define I_DBWRITE_INT  52
#define I_SWITCH_KONT  53
#define I_TRANSF_L     54
#define I_TRANSF_A     55
#define I_READ         56
#define I_WRITE        57
#define I_ADD_CURSNUM  58
#define I_PUSH_KONTEXT 59
#define I_DROP_KONTEXT 60  /* 61, 62 used above */
/* 63 is unused */
#define I_AGGREG       64  
#define I_MAKE_PTR_STEP 65
#define I_SYS_VAR      66
#define I_SWAP         67
#define I_DBREAD_LEN   68
#define I_DBWRITE_LEN  69
#define I_LOAD_OBJECT  70  /* 71 is forw */
#define I_MOVESTRING   72
#define I_CHAR2STR     73
#define I_TOSF2M       74  /* 75 is forw */

#define I_LOADNULL     77
#define I_TSMINUS      78
#define I_TSPLUS       79

#define I_EVENTRET     81
#define I_REALLOC_FRAME 82
#define I_READREC      83
#define I_WRITEREC     84
#define I_JMPTDR       85
#define I_JMPFDR       86
#define I_FAST_DBREAD  87
#define I_FAST_STRING_DBREAD  88
#define I_P_STORE_STR  89
#define I_P_STORE      90
#define I_P_STORE1     91
#define I_P_STORE2     92
#define I_P_STORE4     93
#define I_P_STORE8     94
#define I_P_DBREAD     95  /* not supported yet */
#ifdef __ia64__
#define I_P_STOREPTR   I_P_STORE8
#define I_STOREPTR     I_STORE8
#else
#define I_P_STOREPTR   I_P_STORE4
#define I_STOREPTR     I_STORE4
#endif

#define I_STORE_DB_REF 71
#define I_T16TO32      75
#define I_N16TO32      96
#define I_TEXTSUBSTR   97
#define I_MONEYNEG     98
#define I_MONEYCONST   99
#define I_STORE6       100
#define I_P_STORE6     101
#define I_LOAD6        102
#define I_CONTAINS     103
#define I_LOADOBJNUM   104
#define I_DBREAD_NUM   105
#define I_DBWRITE_NUM  106
#define I_ADD_ATRNUM   107
#define I_REAL_PAR_OFFS 108
#define I_DBREADS      109
#define I_DBWRITES     110
#define I_DBREAD_INTS  111
#define I_DBWRITE_INTS 112
#define I_DLLCALL      113
#define I_CURRVIEW     114
#define I_AND_BITWISE  115
#define I_OR_BITWISE   116
#define I_NOT_BITWISE  117
#define I_T32TO16      118
#define I_LOADADR4     119  /* WIN32 only */
#define I_INSERT       120
#define I_CONSTRUCT    121
#define I_DESTRUCT     122
#define I_UNTASGN      123
#define I_UNTATTR      124
#define I_CHECK_BOUND  125
#define I_LOAD_CACHE_ADDR 126
#define I_GET_CACHED_MCNT 127
#define I_LOAD_CACHE_TEST 128
#define I_LOAD_CACHE_ADDR_CH 129
#define I_BINCONST     130
#define I_BINARYOP     131  /* 6 ops */
#define I_CALLPROP         137
#define I_CALLPROP_BYNAME  138
#define I_GET_THISFORM     139
#define I_N8TO32        140
#define I_T8TO32        141
#define I_T64TOF        142
#define I_T64TO32       143
#define I_T32TO8        144
#define I_TFTO64        145
#define I_T32TO64       146
#define I_I64NEG        147
#define I_INT64CONST    148
#define I_I64MOD        149

#define I_STRREL       170  /* 7+3 instructions */
#define I_INTOPER      (I_STRREL+10)
#define I_INTPLUS      (I_INTOPER+6)
#define I_INTMINUS     (I_INTOPER+7)
#define I_INTTIMES     (I_INTOPER+8)
#define I_INTDIV       (I_INTOPER+9)

#define I_FOPER        (I_INTOPER+10)
#define I_FPLUS        (I_FOPER+6)
#define I_FMINUS       (I_FOPER+7)
#define I_FTIMES       (I_FOPER+8)
#define I_FDIV         (I_FOPER+9)

#define I_DATMINUS     (I_FOPER+10)
#define I_DATPLUS      (I_DATMINUS+1)

#define I_MOPER        (I_DATMINUS+2)
#define I_MPLUS        (I_MOPER+6)
#define I_MMINUS       (I_MOPER+7)
#define I_MTIMES       (I_MOPER+8)
#define I_MDIV         (I_MOPER+9)

#define I_I64OPER      (I_MOPER+10)
#define I_I64PLUS      (I_I64OPER+6)
#define I_I64MINUS     (I_I64OPER+7)
#define I_I64TIMES     (I_I64OPER+8)
#define I_I64DIV       (I_I64OPER+9)
/****************************************************************************/

//#define COMPOSED(x)    (((char*)(x)-(char*)0)>0xffff)
#define DB_OBJ         0x8000
#define MULT_OBJ       0x4000
#define PROJ_VAR       0x2000
#define CACHED_ATR     0x1000
//#define NOT_DB_MASK    0x7fff
//#define IS_DB_OBJ(x)     (DIRTYPE(x) & DB_OBJ)
//#define IS_MULT_OBJ(x)   (DIRTYPE(x) & MULT_OBJ)
//#define IS_PROJ_VAR(x)   (DIRTYPE(x) & PROJ_VAR)
//#define IS_CACHED_ATR(x) (DIRTYPE(x) & CACHED_ATR)
//#define OBJ_FLAGS(x)     (DIRTYPE(x) & 0xff00)
#define IS_STRING_OBJ(x) (DIRECT(x) && is_string(DIRTYPE(x)))

#define PROJECT_LEVEL       0xfe
#ifdef WINS
#define c_error(mess) RaiseException(mess,0,0,NULL)
#else
#define c_error(mess) MyThrow(CI->comp_jmp,mess)
#endif

#define ADDRESS_MARK   0xec  /* must differ from any type or object category */

typedef enum sel_type { SEL_ADDRESS=0, SEL_FAKTOR=1, SEL_REFPAR=2 } sel_type;

typedef struct
{ tobjname name;
  uns8  att_type;
} t_integrity;
/*************************** common vars ************************************/
#ifdef WINS
#define r_error(mess)  RaiseException(mess,0,0,NULL)
#define rr_error(mess) RaiseException(mess,0,0,NULL)
#else
#define r_error(mess)  MyThrow(cdp->RV.run_jmp,  mess)
#define rr_error(mess) MyThrow(get_RV()->run_jmp,mess)
#endif
 /* longjump must not be used in the presentation procedures! After each
	 save_run must follow the restore_run! Nested r_error's are OK. */
extern wbbool empty_index_inside;
#ifndef WINS
extern volatile short num_err;
#endif

tptr comp_alloc(CIp_t CI, unsigned size);
void add_source_to_list(CIp_t CI);
BOOL test_signum(CIp_t CI);
typeobj * faktor(CIp_t CI, int * aflag);
//typeobj * term(CIp_t CI);
//typeobj * simple_expr(CIp_t CI);
CFNC DllKernel typeobj * WINAPI expression(CIp_t CI, int * aflag = NULL);
CFNC DllKernel void WINAPI bool_expr_end(CIp_t CI);
CFNC DllKernel void WINAPI bool_expr(CIp_t CI);
void compound_statement(CIp_t CI);
void if_statement(CIp_t CI);
void repeat_statement(CIp_t CI);
void while_statement(CIp_t CI);
void access_statement(CIp_t CI);
void int_check(CIp_t CI, typeobj * tobj);
void int_expression(CIp_t CI);
void read_statement(CIp_t CI);
CFNC DllKernel void WINAPI statement(CIp_t CI);
void stat_seq(CIp_t CI);
void cbuf_test(CIp_t CI);
void free_table(CIp_t CI, objtable * tb);
typeobj * binary_op(CIp_t CI, typeobj * arg1, int aflag1, typeobj * arg2, int aflag2, symbol oper);
//typeobj * unary_op(CIp_t CI, typeobj * arg, symbol oper);
CFNC DllKernel void WINAPI code_out(CIp_t CI, ctptr code, uns32 addr, unsigned size); /* code output proc. */
objtable * adjust_table(CIp_t CI, objtable * tab, unsigned elems);

#define gen(CI,code) { CI->genbuf=(uns8)(code); \
                       code_out(CI,&CI->genbuf,CI->code_offset++,1); }
#define GEN(CI,code) { CI->genbuf=(uns8)(code); \
                       code_out(CI,&CI->genbuf,CI->code_offset++,1); }
/*void   gen(CIp_t CI, uns8 code);*/
CFNC DllKernel void WINAPI gen2(CIp_t CI, uns16 code);
CFNC DllKernel void WINAPI gen4(CIp_t CI, uns32 code);
void   gen8(CIp_t CI, double code);
CFNC DllKernel void WINAPI genstr(CIp_t CI, tptr str);
void  setcadr(CIp_t CI, code_addr place);
code_addr gen_forward_jump(CIp_t CI, uns8 instr);
void setcadr4(CIp_t CI, code_addr place);
code_addr gen_forward_jump4(CIp_t CI, uns8 instr);
void geniconst(CIp_t CI, sig32 val);   /* generates loading a constant */
int  assignment(CIp_t CI, typeobj * t1, int aflag, typeobj * t2, int eflag, BOOL is_parameter);

BOOL search_dbobj(CIp_t CI, char * name1, char * name2, char * name3, kont_descr * kd, d_attr * pdatr, uns8 * obj_level, typeobj ** ptobj);
BOOL relation(symbol oper);
void convtype(CIp_t CI, uns8 at1, uns8 at2);
int opindex(symbol op);
typeobj * selector(CIp_t CI, sel_type seltype, int & aflag);
BOOL CHAR_ARR(typeobj * tobj);
BOOL STR_ARR (typeobj * tobj);
int TtoATT(typeobj * tobj);
t_specif T2specif(typeobj * tobj);
sig32 num_val(CIp_t CI, sig32 lbd, sig32 hbd);
BOOL is_any_key(symbol cursym, tptr curr_name);
void put_string_char(CIp_t CI, unsigned pos, char c);

void supply_marker_type(CIp_t CI, int index, typeobj * tobj);
/************************ special decls. from compil.c **********************/

extern objtable * id_tables, * comp_project_decls;
extern uns8 sfnc_table[];
extern wbbool has_cdp_param[];
extern void (*sproc_table[])(void);
extern void (*vwmtd_table[])(void);

void win_write(cdp_t cdp, int fl, uns8 type, run_state * RV);
void win_read (cdp_t cdp, int fl, uns8 type, run_state * RV);
BOOL close_all_files(cdp_t cdp);
void unt_register(cdp_t cdp, untstr * uval);
void unt_unregister(cdp_t cdp, untstr * uval);

#ifdef __cplusplus
extern "C" {
#endif
DllKernel int WINAPI untyped_assignment(cdp_t cdp, basicval * dest, basicval * src, int tp1, int tp2, int flags, int len);
DllKernel tptr WINAPI get_separator(int num);
void WINAPI dynmem_dispose(void * ptr);
DllKernel void WINAPI dynmem_stop(void);
DllKernel void WINAPI dynmem_start(void);
DllKernel object * WINAPI search1(ctptr name, objtable * tb);
DllKernel BOOL WINAPI valid_method_property(int ClassNum, int method_num);
#ifdef __cplusplus
}
#endif

void next_character(uns8 * c, int tp);
BOOL next_val(short tp, void FAR* val, short size);
BOOL prev_val(short tp, void FAR* val, short size);
tptr WINAPI sp_strcat(tptr s1, tptr s2);
sig32 get_num_val(CIp_t CI, sig32 lbd, sig32 hbd);
unsigned char WINAPI next_char(CIp_t CI);
CFNC DllKernel unsigned WINAPI ipj_typesize(typeobj * tobj);
void construct_object(cdp_t cdp, tptr addr, tptr objname);
void destruct_object(cdp_t cdp, tptr addr);
void register_program_compilation(cdp_t cdp, CIp_t CI);
objtable * get_proj_decls_table(cdp_t cdp);
void translate_string(CIp_t CI);
BOOL WINAPI unload_library(cdp_t cdp, const char * libname);
void do_call(cdp_t cdp, unsigned procnum, scopevars * parptr);  /* calling a standard procedure */
#ifdef WINS
void call_method(cdp_t cdp, view_dyn * inst, unsigned procnum, scopevars * parptr, int tp);
#endif
void call_dll(cdp_t cdp, dllprocdef * def, scopevars * parptr);

struct t_control_info
{ tname    name;
  uns8     classnum;
  int      ID;
  unsigned type;
  uns8     clsid[16]; // valid for NO_ACTX only
};

#define SUBINST_METHOD_NUMBER 56
#define PARINST_METHOD_NUMBER 57
#define CONTROL_VALUE_PROPNUM 59
#define GENERIC_METHOD_NUMBER 63
#define VALUE_INTERVAL_PROPNUM 78
#define VALUE_LENGTH_PROPNUM   80

#define NUM_SQLEXECUTE 204

#define PART_BYREF  0x0100
#define PART_INREF  0x0200

class CWBAXParType
{
public:

    WORD m_WBType;
    WORD m_VarType;

    inline operator long(){return *(long *)this;}
};

