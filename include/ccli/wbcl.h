#include "wbvers.h"
#pragma warning(disable:4996)  // using insecure functions like strcpy, strcat,... (#define _CRT_SECURE_NO_DEPRECATE)

extern int libreadline_is_present;
extern "C" void prepare_charset_conversions(void);

// connection variables:
extern bool connected;
extern cdp_t cdp;
extern int sys_charset;
extern const char * sys_charset_name;
// console environment variables:
#ifdef WINS
extern HANDLE hStdin, hStdout, hStderr;
#else
extern FILE * hStdin, * hStdout;
#endif
extern int lines_on_page;
extern bool cancel_long_output;

void write_error_info(cdp_t cdp, const char * caller,int err);
void Display_Cursor(cdp_t cdp, int cursor);
void Do_SQL(void);
void output_message(const char * msg, int encoding, bool to_stderr);
extern bool input_terminated;
void newline(void);

void set_application(const char *appl);

void display_cursor_contents(cdp_t cdp, tcursnum curs);

void strmaxcpy(char * dest, const char * src, size_t size);

// internationalization:
#ifdef WINS
#if !OPEN602
#define LIBINTL_STATIC
#endif
#include <libintl-msw.h>
#else // UNIX
#include <libintl.h>
#endif
#define _(string) gettext(string)
#define gettext_noop(string) string

#ifdef WINS
#define snprintf _snprintf
#ifndef ENABLE_EXTENDED_FLAGS  // using old SDK
#define ENABLE_INSERT_MODE      0x0020
#define ENABLE_QUICK_EDIT_MODE  0x0040
#define ENABLE_EXTENDED_FLAGS   0x0080
#endif
#endif  // WINS

#define CHARSET_NUM_UTF8 7
CFNC DllKernel int WINAPI superconv(int stype, int dtype, const void * sbuf, char * dbuf, t_specif sspec, t_specif dspec, const char * string_format);
CFNC DllKernel int GetHostCharSet(void);
CFNC DllKernel BOOL WINAPI cd_is_dead_connection(cdp_t cdp);
typedef void CALLBACK t_client_error_callback(const char * text);
CFNC DllKernel void WINAPI set_client_error_callback(t_client_error_callback * client_error_callbackIn);

