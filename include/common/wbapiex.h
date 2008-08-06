
/****************************************************************************/
/* wbapiex.h - definice pro rozsirenou komunikaci s 602SQL                  */
/* (C) Janus Drozd, 1992-2008                                               */
/* verze: 0.0 (32-bit)                                                      */
/****************************************************************************/
#ifndef __WBAPIEX_H__
#define __WBAPIEX_H__
/* object flags */
#define CO_FLAG_ODBC 1
#define CO_FLAG_LINK 0x80
#define CO_FLAG_REPLTAB 2
#define CO_FLAG_SIMPLE_VW 1
#define CO_FLAG_GRAPH_VW 2
#define CO_FLAG_LABEL_VW 3
#define CO_FLAG_REPORT_VW 4
#define CO_FLAG_POPUP_MN 1
#define CO_FLAG_INCLUDE 1
#define CO_FLAG_XML 4
#define CO_FLAG_TRANSPORT 3
#define CO_FLAG_REF12 1
#define CO_FLAG_REF21 2
#define CO_FLAG_WWWCONN 1
#define CO_FLAG_WWWSEL 2
#define CO_FLAG_STS_CASC 1
#define CO_FLAG_FT_SEP 1
#define CO_FLAG_CASC 1
#define CO_FLAG_PROTECT 0x20
#define CO_FLAG_ENCRYPTED 0x8 /* changed in WB 8.0 */
#define CO_FLAG_NOEXPORT 0x40
#define CO_FLAG_NOEXPORTD 0x10
#define CO_FLAGS_OBJECT_TYPE 7
/* get_xml_response options */
#define HTTP_REQOPT_PATH 0
#define HTTP_REQOPT_ACCEPT 1
#define HTTP_REQOPT_CONTTP 2
#define HTTP_REQOPT_HOST 3
#define HTTP_REQOPT_USERNAME 4
#define HTTP_REQOPT_PASSWORD 5
#define HTTP_REQOPT_COUNT 6


#ifdef __cplusplus
extern "C" {
#endif
    /* Memory management */
    DllKernel BOOL WINAPI core_init( int num );
    DllKernel void WINAPI core_release( void );
    DllKernel void WINAPI corefree( const void * x );
    DllKernel void * WINAPI corealloc( size_t x, uns8 owner );
    DllKernel uns32 WINAPI coresize( const void * x );
    /* conversion routines */
    DllKernel char * WINAPI cutspaces( char * txt );
    DllKernel void WINAPI int2str( sig32 val, char * txt );
    DllKernel BOOL WINAPI str2int( const char * txt, sig32 * val );
    DllKernel BOOL WINAPI str2real( const char * txt, double * val );
    DllKernel void WINAPI real2str( double val, char * txt, int param );
    DllKernel void WINAPI money2str( monstr * val, char * txt, int param );
    DllKernel BOOL WINAPI str2money( const char * txt, monstr * val );
    DllKernel void WINAPI int2money( sig32 i, monstr * m );
    DllKernel void WINAPI money2int( monstr * m, sig32 * i );
    DllKernel void WINAPI datim2str( uns32 dtm, char * txt, int param );
    DllKernel BOOL WINAPI str2int64( const char * txt, sig64 * val );
    DllKernel void WINAPI int64tostr( sig64 val, char * txt );
    /* string comparison */
    DllKernel int WINAPI cmp_str( const char * ss1, const char * ss2, union t_specif specif );

    /* object descriptor */
#define OBJECT_DESCRIPTOR_SIZE 46
    DllPrezen BOOL WINAPI get_object_descriptor_data( const char * buf, tobjname folder_name, uns32 * stamp );

    DllKernel BOOL WINAPI cd_List_objects( cdp_t cdp, tcateg category, const WBUUID uuid, char * * buffer );

    typedef short t_folder_index;


#ifdef __cplusplus
} /* of extern "C" */
#endif       


#endif // __WBAPIEX_H__

