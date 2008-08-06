#include <clocale>

#ifdef __UCLIBC_MJN3_ONLY__
#warning clean this up
#endif

#ifdef __UCLIBC_HAS_XLOCALE__

extern "C" __typeof(iswctype_l) __iswctype_l;
extern "C" __typeof(iswalpha_l) __iswalpha_l;
extern "C" __typeof(nl_langinfo_l) __nl_langinfo_l;
extern "C" __typeof(strcoll_l) __strcoll_l;
extern "C" __typeof(strftime_l) __strftime_l;
extern "C" __typeof(strtod_l) __strtod_l;
extern "C" __typeof(strtof_l) __strtof_l;
extern "C" __typeof(strtold_l) __strtold_l;
extern "C" __typeof(strtol_l) __strtol_l;
extern "C" __typeof(strtoll_l) __strtoll_l;
extern "C" __typeof(strtoul_l) __strtoul_l;
extern "C" __typeof(strtoull_l) __strtoull_l;
extern "C" __typeof(strxfrm_l) __strxfrm_l;
extern "C" __typeof(towlower_l) __towlower_l;
extern "C" __typeof(towupper_l) __towupper_l;
extern "C" __typeof(wcscoll_l) __wcscoll_l;
extern "C" __typeof(wcsftime_l) __wcsftime_l;
extern "C" __typeof(wcsxfrm_l) __wcsxfrm_l;
extern "C" __typeof(wctype_l) __wctype_l;
extern "C" __typeof(newlocale) __newlocale;
extern "C" __typeof(freelocale) __freelocale;
extern "C" __typeof(duplocale) __duplocale;
extern "C" __typeof(uselocale) __uselocale;
extern "C" __typeof(strncasecmp_l) __strncasecmp_l;
extern "C" __typeof(wcscasecmp_l) __wcscasecmp_l;

#endif // GLIBC 2.3 and later

