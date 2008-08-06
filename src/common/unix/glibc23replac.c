// Removes references fo GLIBC_2.3 from a binary when included into its source.
// These references are created by libstdc++ when it is linked statically.

#include "apsymbols.h" 
#include "string.h" 
#include "wchar.h" 
#include "locale.h" 
//#include "stdio.h" 

extern "C" __locale_t __uselocale (__locale_t __dataset)
{ 
  //fputs("uselocale replacement", stderr);
  return __dataset;
}

extern "C" size_t __wcsftime_l (wchar_t *__restrict __s, size_t __maxsize,
			__const wchar_t *__restrict __format,
			__const struct tm *__restrict __tp, __locale_t __loc)
{ 
  //fputs("wcsftime replacement", stderr);
  wcscpy(__s, L"*");
  return 1;
}

extern "C" size_t __strftime_l (char *__restrict __s, size_t __maxsize,
 __const char *__restrict __format,
 __const struct tm *__restrict __tp, __locale_t __loc) 
{ 
  //fputs("strftime replacement", stderr);
  strcpy(__s, "*");
  return 1;
}
