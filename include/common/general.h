
/****************************************************************************/
/* general.h - zakladni spolecne definice pro praci s 602SQL                */
/* (C) Janus Drozd, 1992-2008                                               */
/* verze: 0.0 (32-bit)                                                      */
/****************************************************************************/
#ifndef __GENERAL_H__
#define __GENERAL_H__

#define SQL602_VERSION_MAJOR 0
#define SQL602_VERSION_MINOR 0
#define SQL602_VERSION_STRING "0.0"

#ifdef  _WIN32  // Windows platform
#ifndef WIN32
#define WIN32   // sqltypes.h need this
#endif
#ifndef WINS
#define WINS
#endif
#endif

#ifdef _WIN32

#define DllImport __declspec(dllimport)
#define DllExport __declspec(dllexport)
#define oexport
#ifndef SRVR
#if defined(KERN)
#define DllKernel __declspec(dllexport)
#else
#define DllKernel __declspec(dllimport)
#endif
#else
#define DllKernel
#endif
#if defined(PREZ)
#define DllPrezen __declspec(dllexport)
#else
#define DllPrezen __declspec(dllimport)
#endif
#ifdef VIEW
#define DllViewed __declspec(dllexport)
#else
#define DllViewed __declspec(dllimport)
#endif
#ifdef INST
#define DllInstserv __declspec(dllexport)
#else
#define DllInstserv __declspec(dllimport)
#endif
#ifdef WBED
#define DllWbed __declspec(dllexport)
#else
#define DllWbed __declspec(dllimport)
#endif
#ifdef XMLEXT
#define DllXMLExt __declspec(dllexport)
#else
#define DllXMLExt __declspec(dllimport)
#endif

#else // !_WIN32

#define DllImport
#define DllExport
#define DllKernel
#define DllPrezen
#define DllViewed
#define DllInstserv
#define DllWbed
#define DllXMLExt
#define oexport
#define __stdcall

#ifndef WINDOWS_REPLAC
typedef unsigned int LRESULT;
typedef unsigned int LPARAM;
typedef unsigned int WPARAM;
typedef unsigned int HMENU;
typedef void * HANDLE; // non-file handle, convertible to any pointer
#define WINDOWS_REPLAC
#endif  // !WINDOWS_REPLAC
#endif // !_WIN32


#ifndef GENERAL_DEF
#define oexport
#endif // GENERAL_DEF neni definovano ////////////////////////////////////////

#define _near
#define __near
#define huge
#define _huge
#define __huge

#ifdef __WATCOMC__
#pragma off (unreferenced)
#define EMPTY_ARRAY_INDEX
#else
#define EMPTY_ARRAY_INDEX   0
#endif /* __WATCOMC__ */

#ifndef NULL
#define NULL  0
#endif

#define CFNC extern "C"

#ifdef LINUX
#include <stdint.h>
typedef int64_t sig64;
typedef uint64_t uns64;
typedef uint32_t               uns32;
typedef int32_t                sig32;
typedef uint16_t               uns16;
typedef int16_t                sig16;
#define i64toa(val, buf) sprintf(buf, "%lld", val)
#define itoa(val, buf, base) sprintf(buf, "%d", val)
#else
typedef __int64           sig64;
typedef unsigned __int64  uns64;
typedef unsigned int      uns32;
typedef signed   int      sig32;
typedef unsigned short    uns16;
typedef signed   short    sig16;
#define i64toa(val, buf) _i64toa(*(sig64*)val, buf, 10)
#endif
typedef unsigned char  uns8;
typedef signed   char  sig8;
typedef unsigned char  byte;

typedef       char *  tptr;
typedef const char * ctptr;

typedef uns8     wbbool;
#define wbfalse  (wbbool)0
#define wbtrue   (wbbool)1  
/* zakladni typy databazoveho jadra */
typedef sig32 ttablenum;
typedef sig32 tobjnum;
#define NOOBJECT ((tobjnum)-1) /* cislo neexistujiciho objektu */
typedef sig32 tcursnum;
#define NOCURSOR ((tcursnum)-1) /* cislo neexistujiciho kurzoru */
typedef sig32 tcurstab;
#define ODBC_TABLE 0xffff4000
#define CURS_USER 0xffff8000
#define CURS_MASK 0x1fff /* table number bits in standard cursors */
typedef uns32 trecnum;
#define NORECNUM ((trecnum)-1)
#if WBVERS<=900
typedef uns8 tattrib;
#endif
#if WBVERS>=950
typedef uns16 tattrib;
#endif
#define NOATTRIB ((tattrib)-1)
typedef uns8 tcateg;
typedef uns32 t_varcol_size;
typedef uns16 t_mult_size;
/* zastaraly typ */
typedef uns8 tright;
/* zastaraly typ */
typedef uns16 tdright;
#define OBJ_NAME_LEN 31 /* max. delka jmena databazoveho objektu */
typedef char tobjname[OBJ_NAME_LEN+1];
#if WBVERS<=900
#define NAMELEN 18 /* max. delka identifikatoru promenne */
#endif
#if WBVERS>=950
#define NAMELEN 31 /* max. delka identifikatoru promenne */
#endif
typedef char tname[NAMELEN+1];
#define UUID_SIZE 12 /* delka identifikace objektu */
typedef uns8 WBUUID[UUID_SIZE];
#if WBVERS<=810
typedef HWND window_id;
#endif
typedef short t_folder_index;
/* typ parametru t_oper */
typedef enum  { OPER_SET=0, OPER_GET=1, OPER_GETEFF=2, OPER_SETREPL=3, OPER_SET_ON_CREATE }  t_oper;
typedef enum  { VT_OBJNUM=0, VT_NAME=1, VT_UUID=2, VT_NAME3=3 }  t_valtype;
typedef enum  { READ_UNCOMMITTED, READ_COMMITTED, REPEATABLE_READ, SERIALIZABLE }  t_isolation;
typedef enum  { DISABLE_NEW_CLIENTS=0, WORKER_STOP=1, WORKER_RESTART=2, ENABLE_NEW_CLIENTS=3 }  t_oper_limits;
#define NOINDEX 0xffff /* hodnota parametru index, nejde-li o multiatribut */
#define OUTER_JOIN_NULL_POSITION ((trecnum)-2) /* neexistujici zaznam pripojeny v operaci OUTER JOIN */
#define MAX_PACKAGED_REQS 25 /* max. pocet pozadavku v baliku */
#define MAX_PASSWORD_LEN 100
#define DEFAULT_IP_SOCKET 5001
#define MAX_FIL_PASSWORD 31

#define IS_CURSOR_NUM(curs)  (((curs) & (ODBC_TABLE|CURS_USER)) == CURS_USER)
#define IS_ODBC_TABLE(tb)    (((tb)   & (ODBC_TABLE|CURS_USER)) == ODBC_TABLE)
#define GET_CURSOR_NUM(curs) ((curs) & 0x1fff)
#define GET_ODBC_TABNUM(tb)  ((tb)   & 0x1fff)
#define IS_ODBC_TABLEC(cat, tb) ((cat == CATEG_TABLE || cat == CATEG_DIRCUR) && IS_ODBC_TABLE(tb))

/* cisla systemovych tabulek */
#define TAB_TABLENUM 0 /* cislo tabulky tabulek */
#define OBJ_TABLENUM 1 /* cislo tabulky objektu */
#define USER_TABLENUM 2 /* cislo tabulky uzivatelu */
#define SRV_TABLENUM 3 /* cislo tabulky serveru */
#define REPL_TABLENUM 4 /* cislo tabulky replikacnich pravidel */
#define KEY_TABLENUM 5 /* cislo tabulky verejnych klicu */
#define PROP_TABLENUM 6 /* cislo tabulky vlastnosti */
#define REL_TABLENUM 7 /* cislo tabulky vztahu */

/* sloupec DELETED */
#define DEL_ATTR_NUM 0 /* cislo sloupce DELETED */
#define NOT_DELETED 0 /* hodnota sloupce: zaznam neni zruseny */
#define DELETED 1 /* hodnota sloupce: zaznam je zruseny */
#define RECORD_EMPTY 2 /* hodnota sloupce: zaznam je uvolneny */

/* sloupce systemovych tabulek TABTAB a OBJTAB */
#define OBJ_NAME_ATR 3 /* Jmeno objektu, STRING delky OBJ_NAME_LEN znaku */
#define OBJ_CATEG_ATR 4 /* Kategorie objektu, atribut typu CHAR */
#define APPL_ID_ATR 5 /* Id aplikace, k niz objekt patri, BINARY delky 12 bajtu */
#define OBJ_DEF_ATR 6 /* Definice objektu, atribut typu TEXT */
#define OBJ_FLAGS_ATR 7 /* Priznaky objektu, atribut typu SHORT */
#define OBJ_FOLDER_ATR 8 /* Jmeno slozky pro objekt, STRING delky OBJ_NAME_LEN znaku */
#define OBJ_MODIF_ATR 9 /* Datum a cas posledni zmeny definice objektu */

/* kategorie */
#define CATEG_TABLE 0 /* tabulka */
#define CATEG_USER 1 /* uzivatel */
#define CATEG_VIEW 2 /* pohled */
#define CATEG_CURSOR 3 /* dotaz */
#define CATEG_PGMSRC 4 /* text programu */
#define CATEG_PGMEXE 5 /* prelozeny program */
#define CATEG_MENU 6 /* menu */
#define CATEG_APPL 7 /* aplikace */
#define CATEG_PICT 8 /* obrazek */
#define CATEG_GROUP 9 /* skupina */
#define CATEG_ROLE 10 /* role */
#define CATEG_CONNECTION 11 /* ODBC spojeni */
#define CATEG_RELATION 12 /* relace */
#define CATEG_DRAWING 13 /* nakres */
#define CATEG_GRAPH 14 /* graf */
#define CATEG_REPLREL 15 /* replikacni vztah */
#define CATEG_PROC 16 /* rutina */
#define CATEG_TRIGGER 17 /* trigger */
#define CATEG_WWW 18 /* WWW objekt */
#define CATEG_FOLDER 19 /* slozka */
#define CATEG_SEQ 20 /* sekvence */
#define CATEG_INFO 21 /* informacni objekt */
#define CATEG_DOMAIN 22 /* domena */
#define CATEG_STSH 23 /* style sheet */
#define CATEG_XMLFORM 24 /* XML form */
#define CATEG_KEY 25 /* verejny klic uzivatele */
#define CATEG_SERVER 26 /* replikacni server */
#define CATEG_COUNT 27
#define CATEG_MASK 0x7f
#define IS_LINK 0x80 /* priznak spojovaciho objektu */
#define CATEG_DIRCUR 27 /* otevreny kurzor */

/* struktura kernel_info */
typedef struct {
    char version[6]; /* ASCIIZ oznaceni verze serveru */
    uns16 logged; /* pocet prihlasenych uzivatelu */
    uns16 blocksize; /* velikost clusteru */
    uns32 freeblocks; /* pocet volnych clusteru v alokacni tabulce */
    uns16 frames; /* pocet pametovych ramu */
    uns8 fixed_pages; /* pocet ramu s fixovanym obsahem */
    uns8 max_users; /* maximalni pocet uzivatelu */
    uns8 unused;
    uns32 diskspace; /* mnozstvi volneho mista na disku */
    tobjname server_name; /* jmeno serveru */
    uns32 local_free_memory; /* volna pamet u klienta */
    uns32 remote_free_memory; /* volna pamet na serveru */
    uns32 networking; /* sitova prace klienta */
    uns16 owned_cursors; /* pocet otevrenych kurzoru klientem */
    uns16 padding; /* oprava zarovnani struktury v Delphi */
} kernel_info;

/* Hodnoty slozky "modtype" v zaznamu "modifrec" */
#define MODSTOP 0
#define MODLEN 2
#define MODIND 3
#define MODINT 4
#define MODPTR 5
#define MODINDPTR 6
typedef struct {
    uns8 modtype;
    union umoddef {
        struct smodstop {
            uns16 dummy;
        } modstop;
        struct smodlen {
            uns16 dummy;
        } modlen;
        struct smodind {
            t_mult_size index;
        } modind;
        struct smodint {
            t_varcol_size start;
            t_varcol_size size;
        } modint;
        struct smodp {
            tattrib attr;
        } modptr;
        struct smodindp {
            t_mult_size index;
            tattrib attr;
        } modindptr;
    } moddef;
} modifrec;

/* hodnota typu money */
#pragma pack(1)
typedef struct {
    uns16 money_lo2;
    sig32 money_hi4;
} monstr;
#pragma pack()

#define MAX_FIXED_STRING_LENGTH 4090

/* t_compoper a t_specif */
/* composed operation */
union t_compoper {
    struct {
        uns8 pure_oper; /* operation, e.g. +, <=, between, substring */
        uns8 arg_type; /* type of operands, e.g. int32, float, string */
        uns8 collation; /* used by string relations only */
uns8 ignore_case : 1; /* valid only in string relations */
uns8 wide_char : 1; /* valid only in string relations */
uns8 negate : 1; /* used by _IS_NULL, _PREF, _BETWEEN, ... */
uns8 all_disp : 1;
uns8 any_disp : 1;
    };
    uns32 opq;

#ifdef __cplusplus
    inline unsigned merged(void) // must be equal to MRG(pure_oper,arg_type)
    { return (unsigned)(uns16)opq; }
    t_compoper(void)
    { opq=0; }
    void set_pure(int pure_operIn)                 // used by simple operations (BOP_AND)
    { pure_oper=(uns8)pure_operIn; }
    void set_num(int pure_operIn, int arg_typeIn)  // used by integer operations
    { pure_oper=(uns8)pure_operIn;  arg_type=(uns8)arg_typeIn; }
    void set_str(int pure_operIn, int arg_typeIn, int collationIn, BOOL wide, BOOL ignore)  // used by string operations
    { pure_oper=(uns8)pure_operIn;  arg_type=(uns8)arg_typeIn;
        collation=collationIn;  wide_char=wide;  ignore_case=ignore;
    }
    inline void add_all_disp(void) { all_disp=1; }
    inline void add_any_disp(void) { any_disp=1; }
    inline t_compoper operator =(t_compoper & oper)
    { opq=oper.opq;  return *this; }
    inline BOOL operator ==(t_compoper & oper)
    { return opq==oper.opq; }
#endif
};
/* type-specific additional information about a data type */
union t_specif {
    ttablenum destination_table; /* used by ATT_PTR and ATT_BIPTR */
    tobjnum domain_num; /* used by domains */
    struct {
        sig8 scale; /* LO byte */
        uns8 precision; /* HI byte of the LO word */
    };
    struct {
        uns16 length; /* LO word, used by (char and bin) strings only */
        uns8 charset;
uns8 ignore_case : 1;
uns8 wide_char : 2;
    }; /* used by strings and CLOBs */
    struct {
        uns16 length_unused;
        uns16 stringprop_pck;
    }; /* used by strings without length info */
uns8 with_time_zone : 1;
    uns32 opqval;

#ifdef __cplusplus
    inline bool operator ==(const t_specif & op) const
        { return opqval==op.opqval; }
    inline void set_num(int scaleIn, int precisionIn)
    { opqval=0;  scale=(sig8)scaleIn;  precision=(sig8)precisionIn; }
    inline void set_string(int lengthIn, int charsetIn, int ignoreIn, int wide_charIn)
    { length=lengthIn;  charset=charsetIn;  ignore_case=ignoreIn;  wide_char=wide_charIn; }
    inline void set_null(void)
    { opqval=0; }
    inline t_specif(void)
    { opqval=0; }
    inline t_specif(uns32 opqvalIn)  // may be used for binary strings
    { opqval=opqvalIn; }
    inline t_specif(int scaleIn, int precisionIn)
    { opqval=0;  scale=(sig8)scaleIn;  precision=(sig8)precisionIn; }
    inline t_specif(uns16 lengthIn, int charsetIn, int ignoreIn, int wideIn)
    { length=lengthIn;  charset=charsetIn;  ignore_case=ignoreIn;  wide_char=wideIn; }
    inline t_specif(t_compoper oper)
    { opqval=0;  length=0xffff;
        charset=oper.collation;  ignore_case=oper.ignore_case;  wide_char=oper.wide_char;
    }
    // string properties without length:
    //inline t_specif(int charsetIn, int ignoreIn, int wideIn)
    //  { length=0xffff;  charset=charsetIn;  ignore_case=ignoreIn;  wide_char=wideIn; }
    inline t_specif & set_sprops(uns16 sprops)
    { length_unused=0xffff;  stringprop_pck=sprops;  return *this; }

    //private:  // disabled
    inline uns32 operator =(uns32 val)
    { return opqval=val; }
#endif
};

/* Pristupova prava */
#define RIGHT_READ 0x01 /* calculated privilege, never granted explicitly */
#define RIGHT_WRITE 0x02 /* calculated privilege, never granted explicitly */
#define RIGHT_APPEND 0x04 /* pravo vkladat zaznamy */
#define RIGHT_INSERT 0x04 /* pravo vkladat zaznamy */
#define RIGHT_DEL 0x08 /* pravo rusit zaznamy */
#define RIGHT_NEW_READ 0x10 /* pridelovat k novym zaznamum pravo cist */
#define RIGHT_NEW_WRITE 0x20 /* pridelovat k novym zaznamum pravo prepsat */
#define RIGHT_NEW_DEL 0x40 /* pridelovat k novym zaznamum pravo zrusit */
#define RIGHT_GRANT 0x80 /* universalni pravo poskytovat sva prava */
#define PRIVIL_DESCR_SIZE (1+64) /* velikost zaznamu o pravech */

#define HAS_READ_PRIVIL(priv_val, atr) (*priv_val & RIGHT_READ  ||\
  priv_val[1+(atr-1) / 4] &  (1 << (2*((atr-1)%4))  ))
#define HAS_WRITE_PRIVIL(priv_val,atr) (*priv_val & RIGHT_WRITE ||\
  priv_val[1+(atr-1) / 4] &  (1 << ((2*((atr-1)%4))+1)))
#define SET_READ_PRIVIL(priv_val, atr)\
  priv_val[1+(atr-1) / 4] |= (1 << (2*((atr-1)%4))  )
#define SET_WRITE_PRIVIL(priv_val,atr)\
  priv_val[1+(atr-1) / 4] |= (1 << ((2*((atr-1)%4))+1))

/* Hodnoty "NONE" ruznych typu */

#define NONEMONEY /* nelze takto definovat, ma hodnotu 0,0,0,0,0,0x80 */

#ifdef WINS
#define NONEBIGINT ((sig64)0x8000000000000000)
#define MAXBIGINT  ((sig64)0x7fffffffffffffff)
#else
#define NONEBIGINT 0x8000000000000000LL
#define MAXBIGINT  0x7fffffffffffffffLL
#endif
      #define NONEBOOLEAN ((uns8)0x80)
#define NONECHAR ((uns8)0)
#define NONEDATE 0x80000000L
#define NONETIME 0x80000000L
#define NONETIMESTAMP 0x80000000L
#define NONEINTEGER ((sig32)0x80000000L)
#define NONESHORT ((sig16)0x8000)
#define NONETINY ((sig8)0x80)
#define NONEREAL ((double)-1.7001e308)
#define NONESTRING ""
#define NONEPTR ((trecnum)-1)

/* Parametry funkce Set_sql_option */
#define SQLOPT_NULLEQNULL 1 /* hodnota NULL se rovna NULL */
#define SQLOPT_NULLCOMP 2 /* NULL se porovnavat s ne-NULL hodnotami */
#define SQLOPT_RD_PRIVIL_VIOL 4 /* hodnota bez prava cteni se jevi jako NULL */
#define SQLOPT_MASK_NUM_RANGE 8 /* preteceni ciselneho typu pri konverzi vrati NULL */
#define SQLOPT_MASK_INV_CHAR 0x10 /* nekonvertovatelny retezec znaku vrati NULL */
#define SQLOPT_MASK_RIGHT_TRUNC 0x20 /* zkraceni retezce pri konverzi se maskuje */
#define SQLOPT_EXPLIC_FREE 0x40 /* zrusene zaznamy se uvolni az pri explicitnim Free_deleted */
#define SQLOPT_OLD_ALTER_TABLE 0x80 /* specificke ALTER TABLE */
#define SQLOPT_DUPLIC_COLNAMES 0x100 /* duplicitni jmena sloupcu v dotazu se neprejmenuji */
#define SQLOPT_USER_AS_SCHEMA 0x200 /* povoli jmeno schematu stejne jako jmeno uzivatele */
#define SQLOPT_DISABLE_SCALED 0x400 /* konvertuje vysledky dotazu s desetinnymi misty na realny typ */
#define SQLOPT_ERROR_STOPS_TRANS 0x800 /* pri kazde chybe se ukonci probihajici transakce */
#define SQLOPT_NO_REFINT_TRIGGERS 0x1000 /* akce vyvolane referencni intehritou nespousteji triggery */
#define SQLOPT_USE_SYS_COLS 0x2000 /* SELECT * vybere i systemove sloupce */
#define SQLOPT_CONSTRS_DEFERRED 0x4000 /* integritni omezeni se implicitne vyhodnoti na konci transakce */
#define SQLOPT_COL_LIST_EQUAL 0x8000 /* UPDATE trigger se spousti pouze pri rovnosti mnozin sloupcu */
#define SQLOPT_QUOTED_IDENT 0x10000 /* uvozovky vymezuji stringovy literal */
#define SQLOPT_GLOBAL_REF_RIGHTS 0x20000 /* pravo odkazovat na sloupec v dotazu se testuje globalne */
#define SQLOPT_REPEATABLE_RETURN 0x40000 /* RETURN nastavi hodnotu funkce ale neukonci jeji provadeni */
#define SQLOPT_OLD_STRING_QUOTES 0x80000 /* Vymezeni stringu funguje podle verze 8 a starsich */

/* Cisla chyb */

#ifndef GENERAL_DEF // neni casti obecnejsich deklaraci //////////////////////
#ifndef NO_ERROR    /* conflict with winerror.h */
#define NO_ERROR                 0
#endif
#undef  IS_ERROR
#define IS_ERROR                 128
#endif
      #define ANS_OK 0
#define NOT_ANSWERED 0xff
#define BAD_MODIF 0x80
#define NO_RIGHT 0x81
#define BAD_ELEM_NUM 0x82
#define OUT_OF_TABLE 0x83
#define THREAD_START_ERROR 0x84 /* C-H */
#define CURSOR_MISUSE 0x85
#define BAD_OPCODE 0x86
#define CANNOT_APPEND 0x87
#define NOT_LOCKED 0x88
#define OBJECT_DOES_NOT_EXIST 0x89 /* C-H */
#define INDEX_OUT_OF_RANGE 0x8a
#define NOT_FOUND_02000 0x8b /* 02000, C-H */
#define EMPTY 0x8c
#define CANNOT_CONVERT_DATA 0x8d
#define BAD_PASSWORD 0x8e
#define PTR_TO_DELETED 0x8f
#define NIL_PTR 0x90
#define OUT_OF_KERNEL_MEMORY 0x91
#define RECORD_DOES_NOT_EXIST 0x92
#define IS_DELETED 0x93
#define INDEX_NOT_FOUND 0x94
#define OBJECT_NOT_FOUND 0x95
#define OUT_OF_APPL_MEMORY 0x96
#define BAD_DATA_SIZE 0x97
#define UNREADABLE_BLOCK 0x98
#define NOT_LOGGED_IN 0x99
#define OUT_OF_BLOCKS 0x9a
#define REQUEST_BREAKED 0x9b
#define CONNECTION_LOST 0x9c
#define OS_FILE_ERROR 0x9d
#define INCOMPATIBLE_VERSION 0x9e
#define REJECTED_BY_KERNEL 0x9f
#define MUST_NOT_BE_NULL 0xa0 /* 40002 */
#define OPERATION_DISABLED 0xa1
#define REFERENCED_BY_OTHER_OBJECT 0xa2
#define IE_OUT_OF_DWORM 0xa3 /* interni chyba */
#define IE_FRAME_OVERRUN 0xa4 /* interni chyba */
#define IE_PAGING 0xa5 /* interni chyba */
#define IE_DOUBLE_PAGE 0xa6 /* interni chyba */
#define IE_OUT_OF_BSTACK 0xa7 /* interni chyba */
#define TABLE_DAMAGED 0xa8
#define CANNOT_LOCK_KERNEL 0xa9
#define ROLLBACK_IN_CURSOR_CREATION 0xaa
#define DEADLOCK 0xab
#define KEY_DUPLICITY 0xac /* 40004 */
#define BAD_VERSION 0xad /* client/server version mismatch */
#define CHECK_CONSTRAIN 0xae /* 40005 */
#define CHECK_CONSTRAINT 0xae /* 40005 */
#define REFERENTIAL_CONSTRAIN 0xaf /* 40006 */
#define REFERENTIAL_CONSTRAINT 0xaf /* 40006 */
#define UNPROPER_TYPE 0xb0
#define TABLE_IS_FULL 0xb1
#define REQUEST_NESTING 0xb2
#define CANNOT_FOR_ODBC 0xb3
#define ERROR_IN_FUNCTION_ARG 0xb4
#define ODBC_CURSOR_NOT_OPEN 0xb5
#define DRIVER_NOT_CAPABLE 0xb6
#define TOO_COMPLEX_TRANS 0xb7
#define INTERNAL_SIGNAL 0xb8
#define STRING_CONV_NOT_AVAIL 0xb9
#define NO_WRITE_TOKEN 0xba
#define WAITING_FOR_ACKN 0xbb
#define REPL_BLOCKED 0xbc
#define BAD_TOKEN_STATE 0xbd
#define BAD_TABLE_PROPERTIES 0xbe
#define INDEX_DAMAGED 0xbf
#define PASSWORD_EXPIRED 0xc0
#define NO_KEY_FOUND 0xc1
#define DIFFERENT_KEY 0xc2
#define ASSERTION_FAILED 0xc3
#define SQ_INVALID_CURSOR_STATE 0xc4 /* 24000, C-H */
#define SQ_SAVEPOINT_INVAL_SPEC 0xc5 /* 3B001  C-H */
#define SQ_SAVEPOINT_TOO_MANY 0xc6 /* 3B002 */
#define SQ_TRANS_STATE_ACTIVE 0xc7 /* 25001, C-H */
#define SQ_INVAL_TRANS_TERM 0xc8 /* 2D000, C-H */
#define SQ_TRANS_STATE_RDONLY 0xc9 /* 25006, C-H */
#define SQ_NUM_VAL_OUT_OF_RANGE 0xca /* 22003, C-H */
#define SQ_INV_CHAR_VAL_FOR_CAST 0xcb /* 22018, C-H */
#define SQ_STRING_DATA_RIGHT_TRU 0xcc /* 22001, C-H */
#define SQ_DIVISION_BY_ZERO 0xcd /* 22012, C-H */
#define SQ_CARDINALITY_VIOLATION 0xce /* 21000, C-H */
#define SQ_INVALID_ESCAPE_CHAR 0xcf /* 22019, C-H */
#define SQ_CASE_NOT_FOUND_STMT 0xd0 /* 20000, C-H */
#define SQ_UNHANDLED_USER_EXCEPT 0xd1 /* 45000 */
#define SQ_RESIGNAL_HND_NOT_ACT 0xd2 /* 0K000, C-H */
#define SQ_EXT_ROUT_NOT_AVAIL 0xd3 /* 38001, C-H */
#define SQ_NO_RETURN_IN_FNC 0xd4 /* 2F001, C-H */
#define COLUMN_NOT_EDITABLE 0xd5
#define SQ_TRIGGERED_ACTION 0xd6 /* 09000 */
#define REPLICATION_NOT_RUNNING 0xd7
#define SQ_INVALID_CURSOR_NAME 0xd9 /* 34000, C-H */
#define ROLE_FROM_DIFF_APPL 0xda
#define SEQUENCE_EXHAUSTED 0xdb
#define NO_CURRENT_VAL 0xdc
#define NO_MORE_INTRANET_LICS 0xde
#define LIBRARY_ACCESS_DISABLED 0xdf /* C-H */
#define LIBRARY_NOT_FOUND 0xe0 /* C-H */
#define LANG_SUPP_NOT_AVAIL 0xe1
#define CONVERSION_NOT_SUPPORTED 0xe2
#define NO_FULLTEXT_LICENCE 0xe3
#define WONT_RUN_AS_ROOT 0xe4 /* C-H */
#define NOTHING_TO_REPL 0xe5
#define OBJECT_VERSION_NOT_AVAILABLE 0xe6
#define NO_REPL_UNIKEY 0xe7
#define FRAMES_EXHAUSTED 0xe8
#define NO_CONF_RIGHT 0xe9
#define NO_SECUR_RIGHT 0xea
#define ACCOUNT_DISABLED 0xeb
#define SCHEMA_IS_OPEN 0xec
#define ERROR_IN_FUNCTION_CALL 0xed
#define NO_XML_LICENCE 0xee
#define LIMIT_EXCEEDED 0xef
#define LAST_DB_ERROR LIMIT_EXCEEDED
#define FIRST_MAIL_ERROR 500 /* client mail error numbers */
#define MAIL_NOT_INITIALIZED 500
#define MAIL_ERROR 501
#define MAIL_NOT_REMOTE 502
#define MAIL_TYPE_INVALID 503
#define MAIL_LOGON_FAILED 504
#define MAIL_BAD_PROFILE 505
#define MAIL_BAD_USERID 506
#define MAIL_NO_ADDRESS 507
#define MAIL_FILE_NOT_FOUND 508
#define MAIL_SYSTEM_ACCOUNT 509
#define MAIL_DIAL_ERROR 510
#define MAIL_ALREADY_INIT 511
#define MAIL_PROFILE_NOTFND 512
#define MAIL_PROFSTR_NOTFND 513
#define MAIL_INVALIDPATH 514
#define MAIL_SOCK_IO_ERROR 515
#define MAIL_UNKNOWN_SERVER 516
#define MAIL_CONNECT_FAILED 517
#define MAIL_NO_MORE_FILES 518
#define MAIL_FILE_DELETED 519
#define MAIL_BOX_LOCKED 520
#define MAIL_MSG_NOT_FOUND 521
#define MAIL_UNK_MSG_FMT 522
#define MAIL_DLL_NOT_FOUND 523
#define MAIL_FUNC_NOT_FOUND 524
#define MAIL_NO_SUPPORT 525
#define MAIL_REQ_QUEUED 526
#define MAIL_PROFILE_EXISTS 527
/* Cisla varovani */
#define NO_WARNING 0
#define WAS_IN_TRANS 1 /* zahajeni transakce provedeno uvnitr transakce  */
#define NOT_IN_TRANS 2 /* ukonceni transakce provedeno mimo transakci  */
#define ERROR_IN_CONSTRS 3 /* integritni omezeni na tabulce je chybne */
#define IS_NOT_DEL 4 /* zaznam nelze obnovit protoze neni zruseny   */
#define ERROR_IN_DEFVAL 5 /* implicitni hodnota sloupce je chybna */
#define WORKING_WITH_DELETED_RECORD 6 /* prace se zrusenym zaznamem */
#define RENAME_IN_DEFIN 7 /* objekt je treba prejmenovat v definici */
#define IS_DEL 8 /* zaznam nelze zrusit protoze neni platny  */
#define WORKING_WITH_EMPTY_RECORD 9 /* the record has been deleted and released, ignoring */
#define NO_BIPTR 32 /* k obousmernemu ukazateli neexistuje protismerny ukazatel */
#define INDEX_OOR 64 /* index prekracuje pocet hodnot multiatributu */
#define internal_IS_ERROR 128 /* hodnota Sz_warning, kdyz doslo k chybe */
/* Hodnoty vracene funkci link_kernel a interf_init */
#define KSE_OK 0 /* Bez chyby */
#define KSE_WINDOWS 1 /* Nelze otevrit okno serveru pod Windows */
#define KSE_FAKE_SERVER 2 /* Falesny SQL server - neproslo overeni identity */
#define KSE_ENCRYPTION_FAILURE 3 /* Neshoda mezi klientem a serverem o sifrovani komunikace */
#define KSE_WINEXEC 4 /* Klientovi se nepodarilo spustit lokalni SQL server */
#define KSE_NO_MEMORY 5 /* Chyba pri alokaci pameti */
#define KSE_CDROM_FIL 6 /* Databazovy soubor byl konvertovan pro CD ROM a nebyl korektne instalovan */
#define KSE_NO_FIL 7 /* Server nelze spustit, jeho jmeno neni registrovano */
#define KSE_DAMAGED 8 /* Databazovy soubor je vazne poskozen */
#define KSE_SYSTEM_CHARSET 9 /* Platforma nepodporuje systemovy jazyk databazoveho souboru */
#define KSE_SERVER_CLOSED 10 /* Pristup na server je docasne uzamcen */
#define KSE_BAD_VERSION 11 /* Verze databazoveho souboru neodpovida verzi serveru */
#define KSE_NETWORK_INIT 12 /* Nelze inicializovat zvoleny sitovy protokol */
#define KSE_CLIENT_SERVER_INCOMP 13 /* Verze serveru neni kompatibilni s verzi klienta */
#define KSE_NOSERVER 14 /* Server nebyl nalezen v siti ani pro nej neni specifikovana platna IP adresa */
#define KSE_CONNECTION 15 /* Nepodarilo se navazat spojeni s SQL serverem */
#define KSE_NOTASK 16 /* Prekroceno maximum spojeni mezi klienty a serverem */
#define KSE_UNSPEC 17 /* Interni chyba spojeni */
#define KSE_NO_NETWORK_LICENCE 18 /* Server nema licenci pro sitovy pristup */
#define KSE_NO_HTTP_TUNNEL 19 /* HTTP tunel neni zapnut na serveru (na stejnem portu bezi web XML interface) */
#define KSE_MAXNCB 22 /* Neni dost ridicich bloku NetBIOS */
#define KSE_DBASE_OPEN 23 /* Nad stejnou databazi jiz pracuje jiny server nebo nedostatek prav k db souboru */
#define KSE_SERVER_NAME_USED 24 /* V siti jiz bezi databazovy server stejneho jmena  */
#define KSE_START_THREAD 25 /* Nelze spustit dalsi vlakno na serveru */
#define KSE_SYNCHRO_OBJ 26 /* Nelze vytvorit synchronizacni objekt */
#define KSE_MAPPING 27 /* Nelze mapovat pametovy soubor */
#define KSE_TIMEOUT 29 /* Lokalni server neodpovedel v casovem limitu */
#define KSE_NO_WINSOCK 30 /* Knihovna TCP/IP socketu nefunguje */
#define KSE_WINSOCK_ERROR 31 /* Chyba pri praci se socketem */
#define KSE_NETBIOS_NAME 34 /* Nelze pridat jmeno pro NetBIOS */
#define KSE_FWNOTFOUND 35 /* Nenalezen SOCKS server (firewall) */
#define KSE_FWCOMM 36 /* Chyba pri komunikaci se SOCKS serverem */
#define KSE_FWDENIED 37 /* SOCKS server (firewall) odmitl vytvorit spojeni na SQL server */
#define KSE_BAD_PASSWORD 38 /* Zadano chybne heslo k databazovemu souboru */
#define KSE_ESCAPED 39 /* Upusteno od startu serveru */
#define KSE_NO_IPX 40 /* Protokol IPX neni instalovan  */
#define KSE_CANNOT_CREATE_FIL 42 /* Server nemuze vytvorit databazovy soubor */
#define KSE_CANNOT_OPEN_FIL 43 /* Server nemuze otevrit databazovy soubor */
#define KSE_CANNOT_OPEN_TRANS 44 /* Server nemuze otevrit transakcni soubor */
#define KSE_IP_FILTER 45 /* Pristup z teto IP adresy na server neni povolen */
#define KSE_INST_KEY 47 /* Instalacni klic pro databazi neni uveden nebo neni platny */
#define KSE_NO_SERVER_LICENCE 48 /* SQL server nema licenci pro provoz */
#define KSE_LAST 49
/* Cisla generickych chyb */
#define GENERR_DAD_DESIGN 6000 /* Chyba v navrhu DAD */
#define GENERR_XERCES_INIT 6001 /* Chyba inicializaci utilit xerces */
#define GENERR_XML_PARSING 6002 /* Chyba pri analyze XML souboru */
#define GENERR_DAD_PARSING 6003 /* Chyba v XML strukture DAD */
#define GENERR_ODBC_DRIVER 6004 /* Chyba vracena ODBC driverem */
#define GENERR_FULLTEXT_CONV 6005 /* Chyba vracena konvertorem dokumentu */
#define GENERR_REGEX_PATTERN 6006 /* Chyba kompilace regularniho vyrazu */
#define GENERR_CONVERTED 6007 /* Konverted SQL server error */
#define GENERR_WCS 6008 /* Error in the  WCS library */
#define FIRST_GENERIC_ERROR GENERR_DAD_DESIGN
#define LAST_GENERIC_ERROR 6999
/* Typy (vyuziji se pri volani funkce Enum_attributes) */
#define ATT_BOOLEAN 1 /* Boolean */
#define ATT_CHAR 2 /* Char */
#define ATT_INT16 3 /* Short */
#define ATT_INT32 4 /* Integer */
#define ATT_MONEY 5 /* Money */
#define ATT_FLOAT 6 /* Real */
#define ATT_STRING 7 /* String + size */
#define ATT_BINARY 10 /* Binary + size */
#define ATT_DATE 11 /* Date */
#define ATT_TIME 12 /* Time */
#define ATT_TIMESTAMP 13 /* Timestamp */
#define ATT_PTR 14 /* Pointer */
#define ATT_BIPTR 15 /* Bipointer */
#define ATT_FIRSTSPEC 16
#define ATT_AUTOR 16 /* sledovaci atribut: Autorizace */
#define ATT_DATIM 17 /* sledovaci atribut: Datumovka */
#define ATT_HIST 18 /* sledovaci atribut: Historie */
#define ATT_LASTSPEC 18
#define ATT_FIRST_HEAPTYPE 18
#define ATT_RASTER 19 /* Raster */
#define ATT_TEXT 20 /* Text */
#define ATT_NOSPEC 21 /* Nospec */
#define ATT_SIGNAT 22 /* Signature */
#define ATT_LAST_HEAPTYPE 22
#define ATT_INT8 45 /* Tiny Integer */
#define ATT_INT64 46 /* Huge Integer */
#define ATT_DOMAIN 47 /* Domain */
#define ATT_EXTCLOB 48 /* External BLOB */
#define ATT_EXTBLOB 49 /* External CLOB */
#define ATT_AFTERLAST 50

#if defined(__ia64__) || defined(__x86_64) || defined(_WIN64)
#define X64ARCH
#define ATT_HANDLE ATT_INT64  // the value of ATT_HANDLE is convertible to pointer and back
#else
      #define ATT_HANDLE ATT_INT32

#endif

/* pristup serveru k promennym klienta */
/* smer predavani hodnoty promennych klienta */
enum t_parmode { MODE_IN=1, MODE_OUT=2, MODE_INOUT=3 } ;
/* popis promenne klienta pristupne pro SQL server */

#ifdef WINS
#pragma pack(push,4)
#endif
struct t_clivar {
    tname name; /* jmeno promenne (velka pismena) */
    enum t_parmode mode; /* zpusob predavani hodnoty promenne */
    int wbtype; /* typ promenne */
    uns32 specif; /* specificke udaje k typu (delka retezce, pocet desetinnych mist,...), t_specif::opqval */
    void * buf; /* ukazatel na buffer s hodnotou promenne */
    int buflen; /* delka bufferu buf */
    int actlen; /* skutecna delka hodnoty - plati pouze pro typy promenne velikosti */
};

#ifdef WINS
#pragma pack(pop)
#endif
typedef struct t_clivar *p_clivar;

/* parametr funkce cd_Get_server_info */
#define OP_GI_SERVER_PLATFORM 0
#define PLATFORM_WINDOWS 0
#define PLATFORM_NETWARE 1
#define PLATFORM_LINUX 2
#define PLATFORM_FREEBSD 3
#define PLATFORM_WINDOWS64 0x10
#define PLATFORM_LINUX64 0x12
#define OP_GI_LICS_CLIENT 2
#define OP_GI_LICS_WWW 3
#define OP_GI_LICS_FULLTEXT 4
#define OP_GI_LICS_SERVER 5
#define OP_GI_INST_KEY 6
#define OP_GI_SERVER_LIC 7
#define OP_GI_TRIAL_ADD_ON 8
#define OP_GI_TRIAL_FULLTEXT 9
#define OP_GI_VERSION_1 10
#define OP_GI_VERSION_2 11
#define OP_GI_VERSION_3 12
#define OP_GI_VERSION_4 13
#define OP_GI_PID 14
#define OP_GI_SERVER_NAME 15
#define OP_GI_DISK_SPACE 16
#define OP_GI_CLUSTER_SIZE 17
#define OP_GI_LICS_USING 18
#define OP_GI_OWNED_CURSORS 19
#define OP_GI_FIXED_PAGES 20
#define OP_GI_FIXES_ON_PAGES 21
#define OP_GI_FRAMES 22
#define OP_GI_FREE_CLUSTERS 23
#define OP_GI_USED_MEMORY 24
#define OP_GI_TRIAL_REM_DAYS 25
#define OP_GI_INSTALLED_TABLES 30
#define OP_GI_LOCKED_TABLES 31
#define OP_GI_TABLE_LOCKS 32
#define OP_GI_TEMP_TABLES 33
#define OP_GI_OPEN_CURSORS 34
#define OP_GI_PAGE_LOCKS 35
#define OP_GI_LOGIN_LOCKS 36
#define OP_GI_CLIENT_NUMBER 37
#define OP_GI_BACKGROUD_OPER 38
#define OP_GI_SYS_CHARSET 39
#define OP_GI_MINIMAL_CLIENT_VERSION 40
#define OP_GI_SERVER_UPTIME 41
#define OP_GI_PROFILING_ALL 42
#define OP_GI_PROFILING_LINES 43
#define OP_GI_SERVER_HOSTNAME 44
#define OP_GI_HTTP_RUNNING 45
#define OP_GI_WEB_RUNNING 46
#define OP_GI_NET_RUNNING 47
#define OP_GI_IPC_RUNNING 48
#define OP_GI_BACKING_UP 49
#define OP_GI_CLIENT_COUNT 50
#define OP_GI_RECENT_LOG 51
#define OP_GI_LICS_XML 52
#define OP_GI_DAEMON 53
#define OP_GI_LEAKED_MEMORY 54

#ifdef WINS
#ifdef X64ARCH
#define CURRENT_PLATFORM  PLATFORM_WINDOWS64
#else
#define CURRENT_PLATFORM  PLATFORM_WINDOWS
#endif
#else // !WINS
#ifdef NETWARE
#define CURRENT_PLATFORM  PLATFORM_NETWARE
#else
#ifdef LINUX
#ifdef __ia64__
#define CURRENT_PLATFORM  PLATFORM_LINUX64
#else
#define CURRENT_PLATFORM  PLATFORM_LINUX
#endif
#else
#define CURRENT_PLATFORM  PLATFORM_FREEBSD
#endif
#endif
#endif

/* typ parametru cd_Insert_record_ex */
struct t_column_val_descr {
    int column_number;
    const void * column_value;
    int value_length;
};

/* situace pro trasovani */
#define TRACE_USER_ERROR 0x00000001 /* E */
#define TRACE_SERVER_FAILURE 0x00000002 /* E user ignored */
#define TRACE_NETWORK_GLOBAL 0x00000004 /* T user ignored - tracing global network events */
#define TRACE_REPLICATION 0x00000008 /* T */
#define TRACE_DIRECT_IP 0x00000010 /* T user ignored */
#define TRACE_USER_MAIL 0x00000020 /* E */
#define TRACE_SQL 0x00000040 /* T */
#define TRACE_LOGIN 0x00000080 /* T */
#define TRACE_CURSOR 0x00000100 /* T */
#define TRACE_IMPL_ROLLBACK 0x00000200 /* T */
#define TRACE_LOG_WRITE 0x00000400 /* C */
#define TRACE_NETWORK_ERROR 0x00000800 /* E */
#define TRACE_REPLIC_MAIL 0x00001000 /* T user ignored */
#define TRACE_REPLIC_COPY 0x00002000 /* T user ignored */
#define TRACE_START_STOP 0x00004000 /* T user ignored */
#define TRACE_REPL_CONFLICT 0x00008000 /* E user ignored */
#define TRACE_SERVER_INFO 0x00010000 /* T user ignored */
#define TRACE_READ 0x00020000 /* object trace */
#define TRACE_WRITE 0x00040000 /* object trace */
#define TRACE_INSERT 0x00080000 /* object trace */
#define TRACE_DELETE 0x00100000 /* object trace */
#define TRACE_BCK_OBJ_ERROR 0x00200000 /* E user ignored */
#define TRACE_PROCEDURE_CALL 0x00400000 /* T */
#define TRACE_TRIGGER_EXEC 0x00800000 /* T */
#define TRACE_USER_WARNING 0x01000000 /* T */
#define TRACE_PIECES 0x02000000 /* T, internal */
#define TRACE_LOCK_ERROR 0x04000000 /* E */
#define TRACE_WEB_REQUEST 0x08000000 /* T */
#define TRACE_CONVERTOR_ERROR 0x10000000 /* E */

#define TRACE_SIT_USER_INDEPENDENT(x) ((x)==TRACE_SERVER_FAILURE || (x)==TRACE_NETWORK_GLOBAL || (x)==TRACE_DIRECT_IP || \
                                       (x)==TRACE_REPLIC_MAIL    || (x)==TRACE_REPLIC_COPY    || (x)==TRACE_START_STOP || \
                                       (x)==TRACE_REPL_CONFLICT  || (x)==TRACE_SERVER_INFO    ||(x)==TRACE_BCK_OBJ_ERROR ||\
									   (x)==TRACE_REPLICATION)
#define TRACE_SIT_TABLE_DEPENDENT(x)  ((x)==TRACE_READ || (x)==TRACE_WRITE || (x)==TRACE_INSERT || (x)==TRACE_DELETE)
/* result of cd_Wait_for_event */
#define WAIT_EVENT_OK 0
#define WAIT_EVENT_TIMEOUT 1
#define WAIT_EVENT_CANCELLED 2
#define WAIT_EVENT_SHUTDOWN 3
#define WAIT_EVENT_ERROR_BUFFER_SIZE 4
#define WAIT_EVENT_ERROR 5
#define WAIT_EVENT_NOT_SIG 6 /* timeout==0 */

/* UNICODE */
typedef struct __locale_struct *charset_t;

/* Parametr Set_translation_callback */
typedef void (t_translation_callback)( void * message );

#if WBVERS>=950 && WBVERS<=950

#define cd_connect(cdp, server_name, show_type)  cd_connect_abi95(cdp, server_name, show_type)  
#endif
#if WBVERS>=1000 && WBVERS<=1099

#define cd_connect(cdp, server_name, show_type)  cd_connect_abi10(cdp, server_name, show_type)  
#endif
#if WBVERS>=1100 && WBVERS<=1100

#define cd_connect(cdp, server_name, show_type)  cd_connect_abi11(cdp, server_name, show_type)  
#endif

#endif   /* !def __GENERAL_H__ */

