
/****************************************************************************/
/* wbprezex.h - definice pro rozsirenou komunikaci s 602SQL                 */
/* (C) Janus Drozd, 1992-2008                                               */
/* verze: 0.0 (32-bit)                                                      */
/****************************************************************************/
#ifndef __WBPREZEX_H__
#define __WBPREZEX_H__

#include "wbapiex.h"
/* types of controls */
#define MV_LIST 0
#define MV_COMBO 1
#define MV_TREE 2
#define MV_MENU 3
#define MV_SLIST 4
#define OPEN_IN_EDCONT 0x0800
#define AUTO_READ_ONLY 0x1000
enum t_licence_result { ELIC_OK, ELIC_NOTFOUND, ELIC_TAMPERED, ELIC_CANNOT_WRITE, ELIC_BAD_TYPE, ELIC_INVALID, ELIC_DLL_MISSING, ELIC_TOO_MANY_LICS, ELIC_ALREADY_STORED, ELIC_RETRIAL, ELIC_BAD_BASE } ;


class editor_type;
#ifdef __cplusplus
extern "C" {
#endif
    /* object lists */
    DllPrezen void WINAPI move_object_names( cdp_t cdp, tcateg cat, HWND hCtrl, int ctrl_type );
    DllPrezen void WINAPI move_object_names_ex( cdp_t cdp, tcateg cat, HWND wind, int ctrl_type, int mask, int cond );
    /* text editor */
    DllPrezen HWND WINAPI Edit_text( HWND hParent, tcursnum cursnum, trecnum position, tattrib attr, uns16 index, uns16 sizetype, short x, short y, short x1, short y1 );
    DllPrezen editor_type * WINAPI find_editor_by_objnum( HWND hClient, tobjnum objnum, ttablenum tab, HWND * hWndChild, HWND * hMdiChild, BOOL activate_it, unsigned flags );
    /* folders */
    DllPrezen char * WINAPI Current_folder_name( cdp_t cdp );
    DllPrezen void WINAPI Set_object_folder_time( cdp_t cdp, tobjnum objnum, tcateg categ );
    DllPrezen tobjnum WINAPI Add_component( cdp_t cdp, tcateg categ, const char * name, tobjnum objnum, BOOL select_it, BOOL noshow DEFAULT(FALSE), BOOL is_encrypted DEFAULT(FALSE), t_folder_index folder DEFAULT(-1) );
    /* entering object names */
    DllPrezen BOOL WINAPI enter_name( HWND hWndParent, const char * capt, char * name );
    DllPrezen BOOL WINAPI is_ident( char * name );
    DllPrezen BOOL WINAPI append_object( cdp_t cdp, HWND hWndParent, tcateg categ, const char * prompt, tobjnum * objnum, char * edt_name );
    /* message boxes */
    DllPrezen void WINAPI errbox( const char * text );
    DllPrezen void WINAPI infbox( const char * text );
    DllPrezen void WINAPI wrnbox( const char * text );
    DllPrezen void WINAPI unpbox( const char * text );
    DllPrezen void WINAPI box2( const char * text, const char * caption );
    DllPrezen char * WINAPI sigalloc( unsigned size, uns8 owner );
    /* other */
    DllPrezen void WINAPI ReleaseOle( void );
    DllPrezen BOOL WINAPI GKState( WORD key );
    DllKernel void WINAPI inval_table_d( cdp_t cdp, tobjnum tb, tcateg cat );
    DllKernel t_licence_result WINAPI get_licences( int * access, BOOL * internet, BOOL * client, BOOL * server, BOOL * fulltext );

    /* advanced database objects editors */
    DllViewed BOOL WINAPI Edit_table_ex( cdp_t cdp, ttablenum table_num, const char * table_name, HWND hParent, char * buffer, unsigned bufsize, unsigned flags );
    DllViewed BOOL WINAPI Edit_query_ex( cdp_t cdp, tobjnum query_num, const char * query_name, HWND hParent, char * buffer, unsigned bufsize, unsigned flags );

#if WBVERS<=810
    DllPrezen void WINAPI no_memory( void );

#endif

#ifdef __cplusplus
} /* of extern "C" */
#endif       


#endif // __WBPREZEX_H__

