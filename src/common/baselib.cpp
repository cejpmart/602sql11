// baselib.cpp - standard conversions etc.
// common for client and server
#include <time.h>
#undef abs
#undef max
#undef min
#include <stdlib.h>  // strtod
#ifdef LINUX
#include <nl_types.h>
#include <langinfo.h>
#include <locale.h>
#include <wchar.h>
#include <wctype.h>
#include <ctype.h>
#include "thlocale.h"
#define _strnicmp strncasecmp
#define SQL_WCHAR		 	(-8)  // copy from sqlucode.h
#define SQL_WVARCHAR	 	(-9)
#define SQL_WLONGVARCHAR 	(-10)
#include <errno.h>
#endif

static void put2dig(unsigned val, tptr txt, BOOL zero)
{ if (val>=10) *(txt++)=(char)('0' + val / 10);
  else if (zero) *(txt++)='0';
  *txt=(char)('0' + val % 10);
  txt[1]=0;
}

CFNC DllKernel void WINAPI int8tostr(sig8 val, tptr txt)
{ if (val==NONETINY) *txt=0;
  else int2str(val, txt);
}


CFNC DllKernel void WINAPI int16tostr(sig16 val, tptr txt)
{ if ((uns16)val==0x8000) *txt=0;
  else int2str(val, txt);
}

bool separators_loaded = false;
char decim_separ[2], ths_separ[2], list_separ[2], 
  explic_decim_separ[2] = { 0 }, // separator explicitly defined by the server or client property
  sysapi_decim_separ[2],         // effective separator used by strtod and sprintf
#ifdef WINS
  i_long_date[30], i_short_date[20],
  t24_separ, tzero_separ, time_separ[2], am_separ[10], pm_separ[10];
#else
  time_format[40], date_format[40];
#endif

// LOCALE_SYSTEM_DEFAULT does not read the actual setting from the control panel when
// the current user is logged to the remote domain.
// LOCALE_USER_DEFAULT works for local & remote users.

void load_separators(void)
{ if (!separators_loaded)   /* not defined yet */
  {
    //strmaxcpy(sysapi_decim_separ, nl_langinfo(RADIXCHAR), sizeof(sysapi_decim_separ)); -- wrong, perhaps loads KDE locales
    char * endp;
    *sysapi_decim_separ='.';  // the deafult (optimization leaves occupied FPU register if this statement is after the last else in MS Visual C++)
    if (strtod("1,5", &endp)==1.5) *sysapi_decim_separ=',';  else
    if (strtod("1;5", &endp)==1.5) *sysapi_decim_separ=';';  else
    if (strtod("1:5", &endp)==1.5) *sysapi_decim_separ=':';  else
    if (strtod("1.5", &endp)==1.5) *sysapi_decim_separ='.';
#ifdef WINS
    GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_SDECIMAL,  decim_separ,  sizeof(decim_separ));
    GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_STHOUSAND, ths_separ,    sizeof(ths_separ));
    GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_SLIST,     list_separ,   sizeof(list_separ));
    GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_SLONGDATE, i_long_date,  sizeof(i_long_date));
    GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_SSHORTDATE,i_short_date, sizeof(i_short_date));
    GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_ITIME,     time_separ,   sizeof(time_separ));
    t24_separ  =*time_separ=='1';
    GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_ITLZERO,   time_separ,   sizeof(time_separ));
    tzero_separ=*time_separ=='1';
    GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_STIME,     time_separ,   sizeof(time_separ));
    GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_S1159,     am_separ,     sizeof(am_separ));
    GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_S2359,     pm_separ,     sizeof(pm_separ));
#else  // !WINS: use system locale, may be overridden in wbkernel.ini (obsolete)
    char wbfile[MAX_PATH];
#ifndef SRVR  // return to the default locale, client process may have changed it
    //const char *oldloc=setlocale(LC_ALL, "");  -- wrong, nothing works then
    strcpy(wbfile, "/etc/");  // for client
#else
    strcpy(wbfile, ffa.first_fil_path());  // for server
#endif
    strcat(wbfile, "wbkernel.ini");
    GetPrivateProfileString("Intl", "sDecimal",   sysapi_decim_separ,  decim_separ, sizeof(decim_separ), wbfile);
    if (!*decim_separ) *decim_separ='.';
    GetPrivateProfileString("Intl", "sThousand",  nl_langinfo(THOUSEP),ths_separ,   sizeof(ths_separ),   wbfile);
    GetPrivateProfileString("Intl", "sList",      ",",                 list_separ,  sizeof(list_separ),  wbfile);
    GetPrivateProfileString("Intl", "TimeFormat", nl_langinfo(T_FMT),  time_format, sizeof(time_format), wbfile);
    GetPrivateProfileString("Intl", "DateFormat", nl_langinfo(D_FMT),  date_format, sizeof(date_format), wbfile);
#ifndef SRVR  // return to the client locale
    //setlocale(LC_ALL, oldloc);
#endif
#endif // !WINS
    separators_loaded=true;
  }
}

CFNC DllKernel tptr WINAPI get_separator(int num)
// Server uses it only for date/time formats.
{ load_separators();
  switch (num)
  { 
#ifndef SRVR  // not used by the server
    case 0: return decim_separ;
    case 1: return ths_separ;
    case 2: return list_separ;
#endif

#ifdef WINS
    case 3: return i_long_date;
    case 4: return i_short_date;
    case 5: return &t24_separ;
    case 6: return &tzero_separ;
    case 7: return time_separ;
    case 8: return am_separ;
    case 9: return pm_separ;
#else  // LINUX
    case 10: return time_format;
    case 11: return date_format;
#endif
    case 12: return sysapi_decim_separ;
  }
  return NULL;
}

CFNC DllKernel void WINAPI time2str(uns32 tm, tptr txt, int param)
{ unsigned hours, minutes, seconds, sec1000;
  if (tm==(uns32)NONEINTEGER) { *txt=0; return; }
  if (param==SQL_LITERAL_PREZ) param=2;
  hours=Hours(tm);  minutes=Minutes(tm);  seconds=Seconds(tm);  sec1000=Sec1000(tm);
  if (param==3)   /* system-defined format */
  {
#ifdef WINS
    BOOL t24=*get_separator(5);
    BOOL zer=*get_separator(6);
    char separ=*get_separator(7);
    put2dig(!t24 && hours >= 12 ? hours-12 : hours, txt, zer);
    txt+=strlen(txt);
    *(txt++)=separ;
    put2dig(minutes, txt, TRUE);  txt+=strlen(txt);
    *(txt++)=separ;
    put2dig(seconds, txt, TRUE);  txt+=strlen(txt);
    if (!t24) strcpy(txt, get_separator(hours >= 12 ? 9 : 8));
#else
    struct tm tmx;
    memset(&tmx, 0, sizeof(tmx));
    tmx.tm_sec=seconds;  tmx.tm_min=minutes;  tmx.tm_hour=hours;
    strftime(txt, 50, get_separator(10), &tmx);
#endif
  }
  else
  { put2dig(hours, txt, TRUE);  txt+=strlen(txt);  // ODBC needs TRUE here
    *txt=':';
    put2dig(minutes, txt+1, TRUE);
    if (!param) return;
    txt+=3;
    *txt=':';
    put2dig(seconds, txt+1, TRUE);
    if (param==1) return;
    txt+=3;
    *txt='.';
    txt[1]=(char)('0'+sec1000 / 100);
    put2dig(sec1000 % 100, txt+2, TRUE);
  }
}

void tzd2str(int tzd, char * txt)
{ if (tzd<0)
    { tzd=-tzd;  *txt='-'; }
  else *txt='+';
  txt++;
  tzd /= 60;  // convert to minutes
  put2dig(tzd/60, txt, TRUE);
  txt+=2;  *txt++=':';
  put2dig(tzd%60, txt, TRUE);
}

CFNC DllKernel void WINAPI timeUTC2str(uns32 tm, int tzd, tptr txt, int param)
{ if (tm==NONETIME) *txt=0;
  else
  { time2str(displace_time(tm, tzd), txt, param);
    tzd2str(tzd, txt+strlen(txt));
  }
}

#if defined(SRVR) && (WBVERS>=900) && defined(ENGLISH)
#ifdef WINS
#if !OPEN602
#define LIBINTL_STATIC
#endif
#include <libintl-msw.h>
const t_specif wide_spec(0,0,0,1);
#else
#include <libintl.h>
const t_specif wide_spec(0,0,0,2);
#endif
#endif
 
#ifdef GETTEXT_STRINGS  // never defined
_("January"), _("February"), _("March"), _("April"), _("May"), _("June"),
_("July"), _("August"), _("September"), _("October"), _("November"), _("December") };
_("Jan"), _("Feb"), _("Mar"), _("Apr"), _("May"), _("Jun"),
_("Jul"), _("Aug"), _("Sep"), _("Oct"), _("Nov"), _("Dec") };
_("Sunday"), _("Monday"), _("Tuesday"), _("Wednesday"), _("Thursday"), _("Friday"), _("Saturday") };
_("Sun"), _("Mon"), _("Tue"), _("Wed"), _("Thu"), _("Fri"), _("Sat") };
#endif

void get_month_name(int month_num, bool abbrev, char * result)
{
#if defined(SRVR) && (WBVERS>=900) && defined(ENGLISH)
  wchar_t wbuf[30];  char ubuf[30];  struct tm tmx;
  memset(&tmx, 0, sizeof(tmx));
  tmx.tm_mday=1;  tmx.tm_mon=month_num;  tmx.tm_year=2000-1900;  tmx.tm_wday=1;
  if (!wcsftime(wbuf, sizeof(wbuf)/sizeof(wchar_t), abbrev ? L"%b" : L"%B", &tmx))
    *wbuf=0;
  superconv(ATT_STRING, ATT_STRING, wbuf, ubuf, wide_spec, t_specif(0,CHARSET_NUM_UTF8,0,0), NULL);
  superconv(ATT_STRING, ATT_STRING, gettext(ubuf), result, t_specif(0,CHARSET_NUM_UTF8,0,0), sys_spec, NULL);
#else
  strcpy(result, abbrev ? mon3_name[month_num] : month_name[month_num]);
#endif
}

void get_day_name(int day_num, bool abbrev, char * result)
{
#if defined(SRVR) && (WBVERS>=900) && defined(ENGLISH)
  wchar_t wbuf[30];  char ubuf[30];  struct tm tmx;
  memset(&tmx, 0, sizeof(tmx));
  tmx.tm_mday=1;  tmx.tm_mon=1;  tmx.tm_year=2000-1900;  tmx.tm_wday=day_num;
  if (!wcsftime(wbuf, sizeof(wbuf)/sizeof(wchar_t), abbrev ? L"%a" : L"%A", &tmx))
    *wbuf=0;
  superconv(ATT_STRING, ATT_STRING, wbuf, ubuf, wide_spec, t_specif(0,CHARSET_NUM_UTF8,0,0), NULL);
  superconv(ATT_STRING, ATT_STRING, gettext(ubuf), result, t_specif(0,CHARSET_NUM_UTF8,0,0), sys_spec, NULL);
#else
  strcpy(result, abbrev ? day3_name[day_num] : day_name[day_num]);
#endif
}

CFNC DllKernel void WINAPI date2str(uns32 dt, tptr txt, int param)
{ unsigned day, month, year, wday;  tptr pp;
  if (dt==(uns32)NONEINTEGER) { *txt=0; return; }
  if (param==SQL_LITERAL_PREZ) param=1;
  day=Day(dt);  month=Month(dt);  year=Year(dt);  wday=Day_of_week(dt);
  switch (param)
  { case 0:  case 1:  /* numeric month */
    default:
      put2dig(day, txt, FALSE);  txt+=strlen(txt);
      *(txt++)='.';
      put2dig(month, txt, FALSE);  txt+=strlen(txt);
      *(txt++)='.';
      break;
    case 2:  case 3:  /* word month */
      put2dig(day, txt, FALSE);  txt+=strlen(txt);
      *(txt++)='.';
      get_month_name(month-1, false, txt);  txt+=strlen(txt);
      if (param & 1) /* add year */ *(txt++)=' ';
      break;
#ifdef WINS
    case 4:  /* long system format */
      pp=get_separator(3);
      goto conv_patt;
    case 5:  /* short system format */
      pp=get_separator(4);
     conv_patt:
      while (*pp)
      { unsigned cnt=1;
        if (*pp=='m' || *pp=='d' || *pp=='y' || *pp=='M' || *pp=='D' || *pp=='Y')
          while (pp[cnt]==*pp) cnt++;
        if (*pp=='m' || *pp=='M')
        { if (cnt==1) put2dig(month, txt, FALSE);  else
          if (cnt==2) put2dig(month, txt, TRUE);   else
          if (cnt==3) get_month_name(month-1, true,  txt); else
                      get_month_name(month-1, false, txt);
          txt+=strlen(txt);
        }
        else if (*pp=='d' || *pp=='D')
        { if (cnt==1) put2dig(day, txt, FALSE);  else
          if (cnt==2) put2dig(day, txt, TRUE);   else
          if (cnt==3) get_day_name(wday, true,  txt); else
                      get_day_name(wday, false, txt);
          txt+=strlen(txt);
        }
        else if (*pp=='y' || *pp=='Y')
        { if (cnt==2) put2dig(year % 100, txt, TRUE);   else
          int2str(year, txt);
          txt+=strlen(txt);
        }
        else if (*pp=='\'')
        { while (pp[cnt] && pp[cnt]!='\'') *(txt++)=pp[cnt++];
          if (pp[cnt]=='\'') cnt++;
        }
        else *(txt++)=*pp;
        pp+=cnt;
      }
      *txt=0;
      return;
#else
    case 4:  case 5:
    { struct tm tmx;
      memset(&tmx, 0, sizeof(tmx));
      tmx.tm_mday=day;  tmx.tm_mon=month-1;  tmx.tm_year=year-1900;  tmx.tm_wday=wday;
      strftime(txt, 50, get_separator(11), &tmx);
      return;
    }
#endif
    case 6:  // YYYY-MM-DD format
      put2dig(year / 100, txt, TRUE);  txt+=2;
      put2dig(year % 100, txt, TRUE);  txt+=2;
      *(txt++)='-';
      put2dig(month,      txt, TRUE);  txt+=2;
      *(txt++)='-';
      put2dig(day,        txt, TRUE);  
      break;
  }
  if (param & 1)  /* add year */
    int2str(year, txt);
  else *txt=0;
}

bool read_displ(const char ** pstr, int * displ)
// Must not skip spaces!
{ uns32 a;
  const char * p = *pstr;
  if ((*p=='+' || *p=='-') && p[1]>='0' && p[1]<='9')
  { bool minus = *p=='-';  
    p++;
    if (!str2uns(&p, &a) || a>12) return FALSE;
    int hours_displ = a;
    if (*p==':') 
    { p++;
      while (*p==' ') p++;
      if (!str2uns(&p, &a) || a>59) return FALSE;
    }
    else a=0;
    *displ = 60*(60*hours_displ+a);
    if (minus) *displ=-*displ;
    *pstr=p;
    return true;
  }
  else return false;
}

CFNC DllKernel BOOL WINAPI str2time(const char * txt, uns32 * tm)
{ return str2timeUTC(txt, tm, false, 0); }

CFNC DllKernel BOOL WINAPI str2timeUTC(const char * txt, uns32 * tm, bool UTC, int default_tzd)
// When tzd==NULL then the time zone information is not allowed.
// tm may be the same as txt, must not overwrite it before exit.
{ uns32 a, restm;
  while (*txt && *(const unsigned char*)txt<=' ') txt++;
  if (*txt=='{')
  { txt++;
    if (*txt=='t') txt++; 
    while (*txt==' ') txt++;
    if (*txt=='\'') txt++;
  }
  while (*txt==' ') txt++;
  if (!*txt) { *tm=NONEINTEGER; return TRUE; }
  if (!str2uns(&txt,&a))
  { const char * p=txt;
    while (*p && (*p==' ' || *p==':' || *p=='.' || *p==',')) p++;
    if (!*p) { *tm=NONEINTEGER;  return TRUE; }  else return FALSE;
  }
  if (a>=24) return FALSE;
  restm=3600000L*a;
  if ((*txt==':') || (*txt=='.') || (*txt=='/') || (*txt==',')) txt++; else return FALSE;
  if (!str2uns(&txt,&a)) return FALSE;
  if (a>=60) return FALSE;
  restm+=60000L*a;
  while (*txt==' ') txt++;
  if ((*txt==':') || (*txt=='.') || (*txt=='/') || (*txt==',')) 
  { txt++; 
    if (!str2uns(&txt,&a)) return FALSE;
    if (a>=60) return FALSE;
    restm+=1000*a;
    //while (*txt==' ') txt++;  -- spaces not allowed in front of a time zone
    if (*txt=='.' || *txt==',')
    { txt++;
      while (*txt==' ') txt++;
      if ((*txt>='0')&&(*txt<='9'))
      { restm+=100*(*txt-'0'); txt++; }
      if ((*txt>='0')&&(*txt<='9'))
      { restm+=10*(*txt-'0'); txt++; }
      if ((*txt>='0')&&(*txt<='9'))
      { restm+=*txt-'0'; txt++; }
      //while (*txt==' ') txt++;  -- spaces not allowed in front of a time zone
    }
  }

  int displ;
  if (read_displ(&txt, &displ)) 
  { restm=displace_time(restm, -displ);  // -> UTC
    if (!UTC) restm=displace_time(restm,  default_tzd);  // UTC -> ZULU
  }
  else 
    if ( UTC) restm=displace_time(restm, -default_tzd);  // ZULU -> UTC

  while (*txt && *(const unsigned char*)txt<=' ') txt++;
  if (*txt=='\'') txt++;
  while (*txt && *(const unsigned char*)txt<=' ') txt++;
  if (*txt=='}') txt++;
  while (*txt && *(const unsigned char*)txt<=' ') txt++;
  if (*txt) return FALSE;   /* end check */
  *tm=restm;
  return TRUE;
}

CFNC DllKernel BOOL WINAPI str2date(const char * txt, uns32 * date)
// date may be the same as txt, must not overwrite it before exit.
{ uns32 a,b,c;
  while (*txt && *(const unsigned char*)txt<=' ') txt++;
  if (!*txt) { *date=NONEINTEGER; return TRUE; }
  if (!str2uns(&txt,&a))
  { const char * p=txt;
    while (*p && (*p==' ' || *p=='.' || *p==',')) p++;
    if (!*p) { *date=NONEINTEGER;  return TRUE; }  else return FALSE;
  }
  if ((*txt==':') || (*txt=='.') || (*txt==',') || (*txt=='/')) txt++; else return FALSE;
  if (!str2uns(&txt,&b)) return FALSE;
  while (*txt==' ') txt++;
  if ((*txt==':') || (*txt=='.') || (*txt==',') || (*txt=='/')) txt++;
  while (*txt==' ') txt++;
  if (*txt)
  { if (!str2uns(&txt,&c)) return FALSE; }
  else
  { uns8 day, month;  int year;
    xgetdate(&day, &month, &year);
    c=year;
  }
  while (*txt && *(const unsigned char*)txt<=' ') txt++;
  if (*txt) return FALSE;
  *date=Make_date((int)a, (int)b, (int)c);
  if (*date==NONEDATE) return FALSE;  // invalid values
  return TRUE;   /* end check */
}

BOOL str2odbc_timestamp_open(const char *& txt, TIMESTAMP_STRUCT * ts)
{ uns32 aux;
  while (*txt && *(const unsigned char*)txt<=' ') txt++;
  if (!*txt) { ts->day=32;  return TRUE; }
  if (!str2uns(&txt,&aux))
  { const char * p = txt;
    while (*p && (*p==' ' || *p==':' || *p=='.' || *p==',')) p++;
    if (!*p) { ts->day=32;  return TRUE; }  else return FALSE;
  }
  ts->day=(unsigned)aux;
  if (*txt=='.' || *txt==',') txt++; else return FALSE;
  if (!str2uns(&txt,&aux)) return FALSE;
  ts->month=(unsigned)aux;
  if (*txt=='.' || *txt==',') txt++; else return FALSE;
  while (*txt==' ') txt++;
  if (*txt)
  { if (!str2uns(&txt,&aux)) return FALSE;
    ts->year=(unsigned)aux;
  }
  else  /* current year */
  { uns8 day, month;  int year;
    xgetdate(&day, &month, &year);
    ts->year=year;
  }
  ts->hour=ts->minute=ts->second=0;
  while (*txt==' ') txt++;
  if (*txt)
  { if (!str2uns(&txt,&aux)) return FALSE;
    ts->hour=(unsigned)aux;
    while (*txt==' ') txt++;
    if (*txt==':')
    { txt++;
      if (!str2uns(&txt,&aux)) return FALSE;
      ts->minute=(unsigned)aux;
      //while (*txt==' ') txt++;
      if (*txt==':')
      { txt++;
        if (!str2uns(&txt,&aux)) return FALSE;
        ts->second=(unsigned)aux;
      }
    }
  }
  if (Make_date(ts->day, ts->month, ts->year) == NONEDATE) return FALSE;
  if (ts->hour > 23 || ts->minute > 59 || ts->second > 59) return FALSE;
  ts->fraction=0;
//  while (*txt==' ') txt++;  -- spaces not allowed in front of a time zone
  return TRUE;   /* no end check */
}

CFNC DllKernel BOOL WINAPI str2odbc_timestamp(const char * txt, TIMESTAMP_STRUCT * ts)
{ if (!str2odbc_timestamp_open(txt, ts)) return FALSE;
  while (*txt && *(const unsigned char*)txt<=' ') txt++;
  return !*txt; 
}

CFNC DllKernel tptr WINAPI cutspaces(tptr txt)
{ int i;
  i=0; while (txt[i]==' ') i++;
  if (i) strcpy(txt,txt+i);
  i=(int)strlen(txt);
  while (i && (txt[i-1]==' ')) i--;
  txt[i]=0;
  return txt;
}

CFNC DllKernel void WINAPI intspec2str(sig32 val, tptr txt, t_specif specif)
{ int i,j;  char t[50+3];
  if (val == NONEINTEGER)
    *txt=0;
  else
  { if (val<0) { val=-val;  *txt='-';  txt++; }
   // create reverted decimal representation:
    i=0;
    do { t[i++]=(char)((val % 10)+'0'); val /= 10; }
    while (val);
   // add decimal separator, if scale>0
    int scale=specif.scale;
    if (scale>0 && scale<=50)
    { if (i<=scale) { memset(t+i, '0', scale-i+1);  i=scale+1; }
      memmov(t+scale+1, t+scale, i-scale);  i++;
      t[scale]='.';
    }
    for (j=0; i; j++) txt[j]=t[--i];
    txt[j]=0;
  }
}

CFNC DllKernel void WINAPI int64spec2str(sig64 val, tptr txt, t_specif specif)
{ int i,j;  char t[50+3];
  if (val == NONEBIGINT)
    *txt=0;
  else
  { if (val<0) { val=-val;  *txt='-';  txt++; }
   // create reverted decimal representation:
    i=0;
    do { t[i++]=(char)((val % 10)+'0'); val /= 10; }
    while (val);
   // add decimal separator, if scale>0
    int scale=specif.scale;
    if (scale>0 && scale<=50)
    { if (i<=scale) { memset(t+i, '0', scale-i+1);  i=scale+1; }
      memmov(t+scale+1, t+scale, i-scale);  i++;
      t[scale]='.';
    }
    for (j=0; i; j++) txt[j]=t[--i];
    txt[j]=0;
  }
}

CFNC DllKernel void WINAPI int2str(sig32 val, tptr txt)
{ intspec2str(val, txt, t_specif()); }

CFNC DllKernel void WINAPI int64tostr(sig64 val, tptr txt)
{ int64spec2str(val, txt, t_specif()); }

CFNC bool WINAPI scale_integer(sig32 & intval, int scale_disp)
{ while (scale_disp > 0)
  { if (intval > 214748364 || intval < - 214748364) return false;
    intval *= 10;  
    scale_disp--;
  }
  while (scale_disp < 0)
  { intval = (scale_disp==-1 ? (intval < 0 ? intval-5 : intval+5) : intval) / 10;
    scale_disp++;
  }
  return true;
}

CFNC bool WINAPI scale_int64(sig64 & intval, int scale_disp)
{ scale_disp = (sig8)scale_disp;  // must ignore upper bytes!!
  while (scale_disp > 0)
  { if (intval > MAXBIGINT/10 || intval < -MAXBIGINT/10) return false;
    intval *= 10;  
    scale_disp--;
  }
  while (scale_disp < 0)
  { intval = (scale_disp==-1 ? (intval < 0 ? intval-5 : intval+5) : intval) / 10;
    scale_disp++;
  }
  return true;
}

CFNC void WINAPI scale_real(double & realval, int scale_disp)
{ while (scale_disp > 0)
  { realval *= 10;  
    scale_disp--;
  }
  while (scale_disp < 0)
  { realval = realval / 10;
    scale_disp++;
  }
}

CFNC DllKernel void WINAPI int32tostr(sig32 val, tptr txt)
{ int2str(val, txt); }

CFNC DllKernel BOOL WINAPI str2uns_scaled(const char ** ptxt, uns32 * val, int * scale)  /* without end-check */
// If scale!=NULL then accepts decimal separator inside the number and returns the number of decimal digits.
{ char c;  uns32 v=0;  bool has_decim_separ;
  while (**ptxt && **(const unsigned char**)ptxt<=' ') (*ptxt)++;
  c=**ptxt;
  if (scale) *scale=0;
 // number may start with the desimal separator:
  if ((c=='.' || c==',') && scale) 
    { has_decim_separ = true;  (*ptxt)++;  c=**ptxt; }
  else
    has_decim_separ = false;
  if (c<'0' || c>'9') return FALSE;
  do
  { if ((v==429496729) && (c>'5') || v>429496729)
      return FALSE;  /* cannot be converted to sig32 */
    v=10*v+(c-'0');
    if (has_decim_separ) (*scale)++;  // has_decim_separ implies scale!=NULL
    (*ptxt)++;  c=**ptxt;
    if ((c=='.' || c==',') && scale && !has_decim_separ) 
      { has_decim_separ = true;  (*ptxt)++;  c=**ptxt; }
  } while (c>='0' && c<='9');
  *val=v;
  while (**ptxt && **(const unsigned char**)ptxt<=' ') (*ptxt)++;
  return TRUE;
}

CFNC DllKernel BOOL WINAPI str2uns(const char ** ptxt, uns32 * val)  /* without end-check */
{ return str2uns_scaled(ptxt, val, NULL); }

CFNC DllKernel BOOL WINAPI str2int(const char * txt, sig32 * val)
{ uns32 v;
  while (*txt && *(const unsigned char*)txt<=' ') txt++;
  if (!*txt) { *val=NONEINTEGER; return TRUE; }
  bool neg=*txt=='-';
  if ((*txt=='-')||(*txt=='+')) txt++;
  if (!str2uns(&txt, &v)) return FALSE;
  if (v>2147483647) return FALSE;  // outside the signed type range
  if (*txt) return FALSE;  /* end check */
  *val=neg ? -(sig32)v : (sig32)v;  
  return TRUE;
}

CFNC DllKernel BOOL WINAPI str2intspec(const char * txt, sig32 * val, t_specif specif)
{ if (specif.scale==0) return str2int(txt, val);
  uns32 v;  int scale;
  while (*txt && *(const unsigned char*)txt<=' ') txt++;
  if (!*txt) { *val=NONEINTEGER; return TRUE; }
  bool neg=*txt=='-';
  if ((*txt=='-')||(*txt=='+')) txt++;
  if (!str2uns_scaled(&txt, &v, &scale)) return FALSE;
  if (v>2147483647) return FALSE;  // outside the signed type range
  if (*txt) return FALSE;  /* end check */
  *val=neg ? -(sig32)v : (sig32)v;  
  return scale_integer(*val, specif.scale-scale);
}

CFNC DllKernel BOOL WINAPI str2uns64_scaled(const char ** ptxt, sig64 * val, int * scale)  /* without end-check */
// If scale!=NULL then accepts decimal separator inside the number and returns the number of decimal digits.
{ char c;  sig64 v=0;  bool has_decim_separ;
  while (**ptxt && **(const unsigned char**)ptxt<=' ') (*ptxt)++;
  c=**ptxt;
  if (scale) *scale=0;
 // number may start with the desimal separator:
  if ((c=='.' || c==',') && scale) 
    { has_decim_separ = true;  (*ptxt)++;  c=**ptxt; }
  else
    has_decim_separ = false;
  if (c<'0' || c>'9') return FALSE;
  do
  { v=10*v+(c-'0');
    if (v & NONEBIGINT) return FALSE;
    if (has_decim_separ) (*scale)++;  // has_decim_separ implies scale!=NULL
    (*ptxt)++;  c=**ptxt;
    if ((c=='.' || c==',') && scale && !has_decim_separ) 
      { has_decim_separ = true;  (*ptxt)++;  c=**ptxt; }
  } while (c>='0' && c<='9');
  *val=v;
  while (**ptxt && **(const unsigned char**)ptxt<=' ') (*ptxt)++;
  return TRUE;
}

CFNC DllKernel BOOL WINAPI str2uns64(const char ** ptxt, sig64 * val)  /* without end-check */
{ return str2uns64_scaled(ptxt, val, NULL); }

CFNC DllKernel BOOL WINAPI str2int64(const char * txt, sig64 * val)
{ while (*txt && *(const unsigned char*)txt<=' ') txt++;
  if (!*txt) { *val=NONEBIGINT;  return TRUE; }
  bool neg = *txt=='-';
  if (*txt=='-' || *txt=='+') txt++;
  if (!str2uns64(&txt, val)) return FALSE;
  if (*txt) return FALSE;  /* end check */
  if (neg) *val=-*val;
  return TRUE;
}

CFNC DllKernel BOOL WINAPI str2int64spec(const char * txt, sig64 * val, t_specif specif)
{ if (specif.scale==0) return str2int64(txt, val);
  int scale;
  while (*txt && *(const unsigned char*)txt<=' ') txt++;
  if (!*txt) { *val=NONEBIGINT;  return TRUE; }
  bool neg = *txt=='-';
  if (*txt=='-' || *txt=='+') txt++;
  if (!str2uns64_scaled(&txt, val, &scale)) return FALSE;
  if (*txt) return FALSE;  /* end check */
  if (neg) *val=-*val;
  return scale_int64(*val, specif.scale-scale);
}

CFNC DllKernel void WINAPI wininichange(char * section)
{ if (section==NULL || !stricmp(section, "Intl")) separators_loaded=false; }

tptr convert(const char * txt, char * auxbuf, unsigned auxbuf_size)
// Converts string so that it can be coverted by stdtod: removes thousand separators, replaces decimal separators.
{ char * p;
  /* must first remove thousand separator and then look for (the last) decimal sep. */
  load_separators();
  p=auxbuf;
  while (*txt && auxbuf_size>1)
  { if (*txt!=*ths_separ) 
      { *p=*txt;  p++;  auxbuf_size--; }
    txt++;
  }
  *p=0;
  int len = (int)strlen(auxbuf);
#if 0

#ifdef WINS  // locale-independent decimal separator required by strtod
  while (len--) if (p[len]==*decim_separ || p[len]==',') { p[len]='.';  break; }
#else        // locale-dependent decimal separator required by strtod
  if (*decim_separ && *decim_separ!='.')   /* convert it, must ignore explic_decim_separ */
    while (len--) if (p[len]=='.') { p[len]=*decim_separ;  break; }
#endif

#else  // new version, because strtod on WINS sometimes needs locale-dependent separator, too
  while (len--)
    if (auxbuf[len]==*decim_separ || auxbuf[len]=='.' || *explic_decim_separ && auxbuf[len]==*explic_decim_separ)
      {auxbuf[len]=*sysapi_decim_separ;  break; }
#endif
  return auxbuf;
}

CFNC DllKernel BOOL WINAPI str2real(const char * txt, double * val)
{ double v;  char * endp;  char auxbuf[60];  BOOL negate = FALSE;
  while (*txt && *(const unsigned char*)txt<=' ') txt++;
  if (!*txt) { *val=NULLREAL; return TRUE; }
  if (*txt=='-') { negate=TRUE;  txt++; } // - may be followed by space, due to the mask
  else if (*txt=='+') txt++;
  while (*txt && *(const unsigned char*)txt<=' ') txt++;
  if (!*txt) { *val=NULLREAL; return TRUE; }
  txt=convert(txt, auxbuf, sizeof(auxbuf));
  v=strtod(txt, &endp);
  while (*endp && *(const unsigned char*)endp<=' ') endp++;
  if (*endp) return FALSE;
  *val=negate ? -v : v;
  return TRUE;
}

CFNC DllKernel void WINAPI real2str(double val, tptr txt, int param)
{ char format[20]; int prec, len;  BOOL listsep, decim;
  if (val==NULLREAL) { *txt=0; return; }
 // analyse the parametrs:
  prec=param;
  if (prec<0) { prec=-prec;  decim=TRUE; }
  else decim=FALSE;
  if (prec==127) { decim=TRUE;  prec=0; }  // special
  if (prec>=100) { prec-=100;  listsep=decim=TRUE; }
  else listsep=FALSE;
 // create the format string:
  if (param==SQL_LITERAL_PREZ) strcpy(format, "%E");
  else
  { *format='%';  format[1]='.';
    int2str(prec, format+2);
    len=(int)strlen(format);
    format[len]=(char)((decim && val < 1e100 && val > -1e100) ? 'f' : 'E');   /* otherwise may overflow */
    format[len+1]=0;
  }
  sprintf(txt, format, val);  // on Windows does not depend on locale! -- not generally true!!
 // convert the decimal separator:
  load_separators();
  int pos=0;  len=(int)strlen(txt);
  const char * separ = *explic_decim_separ ? explic_decim_separ : decim_separ;
#if 0  // originally #ifdef WINS (sprintf uses '.' as the decimal separator - not generally true)
  if (param!=SQL_LITERAL_PREZ)  
  { while (pos<len) if (txt[pos]!='.') pos++; else break;
    if (txt[pos]=='.') txt[pos]=*separ;
  }
#else  // separator is written according to locale, must find it and replace by the required separator
  while (pos<len) if (txt[pos]!=*sysapi_decim_separ) pos++; else break;
  if (pos<len) txt[pos] = param==SQL_LITERAL_PREZ ? '.' : *separ;
#endif
 // add the thousands separator, if required:
  if (param!=SQL_LITERAL_PREZ)
    if (listsep && *ths_separ)
      while (pos>3)
      { pos-=3;
        if (txt[pos-1]<'0' || txt[pos-1]>'9') break;  /* leading sign */
        memmov(txt+pos+1, txt+pos, len-pos+1);
        txt[pos]=*ths_separ;  len++;
      }
}

CFNC void WINAPI money_neg(monstr * m)
{ if (!m->money_lo2)
    m->money_hi4=~m->money_hi4 + 1;
  else
  { m->money_lo2=(uns16)(~m->money_lo2 + 1);
    m->money_hi4=~m->money_hi4;
  }
}

CFNC DllKernel double WINAPI money2real(monstr * m)
{ if ((m->money_hi4==NONEINTEGER) && !m->money_lo2) return NULLREAL;
  if (m->money_hi4 < 0)
  { money_neg(m);
    return -((double)m->money_lo2 + (double)m->money_hi4 * 65536.0)/100.0;
  }
  return ((double)m->money_lo2 + (double)m->money_hi4 * 65536.0)/100.0;
}

unsigned money_mod_10(monstr * m)   /* not for MULLMONEY or negative */
{ uns32 bit16 = 0x10000L;
  return (unsigned)((((uns32)(m->money_hi4 % 10) * bit16) +
                                                 m->money_lo2) % 10);
}

void money_div_10(monstr * m)   /* not for MULLMONEY or negative */
{ uns32 bit16 = 0x10000L;
  m->money_lo2 = (uns16)((m->money_lo2 + (uns32)(m->money_hi4 % 10)*bit16)/10);
  m->money_hi4 /= 10;
}

CFNC DllKernel void WINAPI money2int(monstr * m, sig32 * i)
{ BOOL neg;
  if (IS_NULLMONEY(m)) *i=NONEINTEGER;
  else
  { if (m->money_hi4 < 0) { money_neg(m);  neg=TRUE; }
    else neg=FALSE;
    money_div_10(m);
    money_div_10(m);
    *i=((m->money_hi4 & 0xffff) << 16) + m->money_lo2;
    if (neg) *i=-*i;
  }
}

CFNC DllKernel void WINAPI money2str(monstr * val, tptr txt, int param)
// param==1 or SQL_LITERAL_PREZ: decimal separator is '.'
// param==0: decimal separator is ',' and ",00" is replaced by ",-"
// other: decimal separator taken from system, adding thousands separators
{ unsigned i, digs;  char pom[20];
  if (param==SQL_LITERAL_PREZ) param=1;
  monstr locm = *val;  /* val may be == txt, copy is necessary! */
  if (IS_NULLMONEY(&locm)) { *txt=0; return; }
  if (locm.money_hi4 < 0) { *txt='-';  txt++;  money_neg(&locm); }
  digs=0;
  while (digs < 3 || locm.money_hi4 || locm.money_lo2)
  { pom[digs++]=(char)('0'+money_mod_10(&locm));
    money_div_10(&locm);
  }
  if (!param)
    if ((pom[0]=='0') && (pom[1]=='0'))
      { pom[0]=' ';  pom[1]='-'; }
  load_separators();
  const char * separ = *explic_decim_separ ? explic_decim_separ : decim_separ;
  for (i=0; i<digs; i++)
  { *(txt++)=pom[digs-i-1];
    if (i==digs-3)
      *(txt++)=(char)(param ? (param==1 ? '.' : *separ) : ',');
    else if (param && param!=1 && *ths_separ && i<digs-3 && !((digs-i-3) % 3))
      *(txt++)=*ths_separ;
  }
  *txt=0;
}

CFNC DllKernel BOOL WINAPI real2money(double d, monstr * m)
{ BOOL neg;  double dh;
  if (d==NULLREAL) { m->money_hi4=NONEINTEGER;  m->money_lo2=0;  return TRUE; }
  if (d<0) { neg=TRUE;  d=-d; } else neg=FALSE;
  if (d>1.4E12) { m->money_hi4=NONEINTEGER;  m->money_lo2=0;  return FALSE; }
  d*=100.0;
  d+=0.5;   /* round */
  dh=d / 65536.0;
  m->money_hi4=(uns32)dh;
  m->money_lo2=(uns16)(d-65536.0*(double)m->money_hi4);
  if (neg) money_neg(m);
  return TRUE;
}

CFNC DllKernel void WINAPI int2money(sig32 i, monstr * m)
{ if (i==NONEINTEGER) { m->money_hi4=NONEINTEGER;  m->money_lo2=0; }
  else
  { m->money_lo2=(uns16)i;
    m->money_hi4=(sig32)(sig16)(i>>16);
    moneymult(m, 100);
  }
}

CFNC DllKernel BOOL WINAPI str2money(const char * txt, monstr * val)
{ BOOL neg=FALSE;  monstr dig;  char auxbuf[50];
  while (*txt && *(const unsigned char*)txt<=' ') txt++;
  if (!*txt) { val->money_hi4=NONEINTEGER;  val->money_lo2=0;  return TRUE; }
  if (*txt=='$') txt++;  /* skipping leading $ sign */
  while (*txt && *(const unsigned char*)txt<=' ') txt++;
  if (*txt=='-') { txt++;  neg=TRUE; }
  else if (*txt=='+') txt++;
  while (*txt && *(const unsigned char*)txt<=' ') txt++;
  txt=convert(txt, auxbuf, sizeof(auxbuf));
  val->money_lo2=0;  val->money_hi4=0;  dig.money_hi4=0;
  if (*txt!='.' && *txt!=',' && *txt!=*sysapi_decim_separ)
  { if ((*txt<'0') || (*txt>'9')) return FALSE;
    do
    { moneymult(val, 10);
	   dig.money_lo2=(uns16)(*txt-'0');
      moneyadd (val, &dig);
      txt++;
    } while ((*txt>='0') && (*txt<='9'));
    moneymult(val, 10);
  }
  if (*txt=='.' || *txt==',' || *txt==*sysapi_decim_separ || *txt=='$')
  { txt++;
    if ((*txt>='0') && (*txt<='9'))
	  { dig.money_lo2=(uns16)(*txt-'0');
      moneyadd (val, &dig);
      txt++;
    }
    moneymult(val, 10);
    if ((*txt>='0') && (*txt<='9'))
	  { dig.money_lo2=(uns16)(*txt-'0');
      moneyadd (val, &dig);
      txt++;
    }
  }
  else moneymult(val, 10);
  if (*txt=='-') txt++;
  if (neg) money_neg(val);
  while (*txt && *(const unsigned char*)txt<=' ') txt++;
  return !*txt;
}

static void odbc_timestamp2str(TIMESTAMP_STRUCT * ts, tptr txt, int param)
{ TIMESTAMP_STRUCT tc = *ts;  /* must make a local copy, ts may = txt */
  if (tc.day>31) *txt=0;
  else
  { int dtprez;
    if (param==SQL_LITERAL_PREZ) param=5; // results in (1,1) formats
    dtprez=param % EXT_DATE_FORMATS;
    if (dtprez < 4) dtprez+=2;  else dtprez-=4;
    date2str(Make_date(tc.day, tc.month, tc.year), txt, dtprez);
   /* add time: */
    if (param < EXT_DATE_FORMATS || param >= 2*EXT_DATE_FORMATS)
    { txt+=strlen(txt);
      *(txt++)=' '; *(txt++)=' ';
      time2str(Make_time(tc.hour, tc.minute, tc.second, 0), txt,
               param < EXT_DATE_FORMATS ? 1 : 0);
    }
  }
}

CFNC DllKernel void WINAPI datim2TIMESTAMP(uns32 dtm, TIMESTAMP_STRUCT * ts)
{ if (dtm==(uns32)NONEINTEGER) ts->day=32;
  else
  { uns32 clock = dtm % 86400L;  dtm /= 86400L;
    ts->day   =(int)(dtm % 31 + 1);  dtm /= 31;
    ts->month =(int)(dtm % 12 + 1);
    ts->year  =(int)(dtm/12 + 1900);
    ts->hour  =(int)(clock / 3600);
    ts->minute=(int)(clock % 3600 / 60);
    ts->second=(int)(clock % 60);
    ts->fraction=0;
  }
}

void time2TIME(uns32 tm, TIME_STRUCT * ts)
{ if (tm==(uns32)NONEINTEGER) ts->hour=25;
  else { ts->hour=Hours(tm);  ts->minute=Minutes(tm);  ts->second=Seconds(tm); }
}

void date2DATE(uns32 dt, DATE_STRUCT * ts)
{ if (dt==(uns32)NONEINTEGER) ts->day=32;
  else { ts->day=Day(dt);  ts->month=Month(dt);  ts->year=Year(dt); }
}

CFNC DllKernel void WINAPI TIMESTAMP2datim(TIMESTAMP_STRUCT * ts, uns32 * dtm)
{ int year = ts->year < 1900 ? 0 : ts->year - 1900;
  if (ts->day > 31) *dtm=(uns32)NONEINTEGER;
  else *dtm = ts->second+60L*(ts->minute+60*ts->hour)+
              86400L*((ts->day-1)+31L*((ts->month-1)+12*year));
}

CFNC DllKernel uns32 WINAPI timestamp2date(uns32 dtm)
{ TIMESTAMP_STRUCT ts;
  datim2TIMESTAMP(dtm, &ts);
  if (ts.day > 31) return NONEDATE;
  return Make_date(ts.day, ts.month, ts.year);
}

CFNC DllKernel uns32 WINAPI timestamp2time(uns32 dtm)
{ TIMESTAMP_STRUCT ts;
  datim2TIMESTAMP(dtm, &ts);
  if (ts.day > 31) return NONETIME;
  return Make_time(ts.hour, ts.minute, ts.second, ts.fraction);
}

CFNC DllKernel uns32 WINAPI datetime2timestamp(uns32 dt, uns32 tm)
{ TIMESTAMP_STRUCT ts;  uns32 dtm;
  if (dt==NONEDATE) return NONETIME/*STAMP*/;
  if (tm==NONETIME) tm=0;
  ts.day=Day(dt);  ts.month=Month(dt);  ts.year=Year(dt);
  ts.hour=Hours(tm);  ts.minute=Minutes(tm);  ts.second=Seconds(tm);  ts.fraction=Sec1000(tm);
  TIMESTAMP2datim(&ts, &dtm);
  return dtm;
}

void TIME2time(TIME_STRUCT * ts, uns32 * tm)
{ if (ts->hour > 24) *tm=(uns32)NONEINTEGER;
  else *tm = Make_time(ts->hour, ts->minute, ts->second, 0);
}

void DATE2date(DATE_STRUCT * ts, uns32 * dt)
{ if (ts->day > 31) *dt=(uns32)NONEINTEGER;
  else *dt = Make_date(ts->day, ts->month, ts->year);
}

CFNC DllKernel uns32 WINAPI displace_time(uns32 tm, int add_this)
{ int tmi = tm;
  tmi += 1000*add_this;
  if (tmi < 0)
    return tmi + 86400000;
  if (tmi >= 86400000)
    return tmi - 86400000;
  return tmi;
}

CFNC DllKernel uns32 WINAPI displace_timestamp(uns32 tms, int add_this)
{ int   tm = tms % 86400;
  uns32 dt = tms / 86400;
  tm += add_this;
  if (tm < 0)
    { tm += 86400;  dt_minus(dt, 1); }
  else if (tm >= 86400)
    { tm -= 86400;  dt_plus (dt, 1); }
  tms = 86400*dt+tm;
  return tms;
}

CFNC DllKernel void WINAPI datim2str(uns32 dtm, tptr txt, int param)
{ TIMESTAMP_STRUCT ts;
  datim2TIMESTAMP(dtm, &ts);
  odbc_timestamp2str(&ts, txt, param);
}

CFNC DllKernel void WINAPI timestamp2str(uns32 dtm, tptr txt, int param)
{ datim2str(dtm, txt, param); }

CFNC DllKernel void WINAPI timestampUTC2str(uns32 dtm, int tzd, tptr txt, int param)
{ if (dtm==(uns32)NONEINTEGER) *txt=0;
  else
  { timestamp2str(displace_timestamp(dtm, tzd), txt, param);
    tzd2str(tzd, txt+strlen(txt));
  }
}

CFNC DllKernel BOOL WINAPI str2timestamp(const char * txt, uns32 * dtm)
{ return str2timestampUTC(txt, dtm, false, 0); }

CFNC DllKernel BOOL WINAPI str2timestampUTC(const char * txt, uns32 * dtm, bool UTC, int default_tzd)
// dtm may be the same as txt, must not overwrite it before exit.
{ TIMESTAMP_STRUCT tsst;  uns32 resdtm;
  if (!str2odbc_timestamp_open(txt, &tsst)) return FALSE;
  TIMESTAMP2datim(&tsst, &resdtm);
 // displacement
  if (resdtm!=(uns32)NONEINTEGER)
  { int displ;
    if (read_displ(&txt, &displ))
    { resdtm=displace_timestamp(resdtm, -displ);  // -> UTC
      if (!UTC) resdtm=displace_timestamp(resdtm,  default_tzd);  // UTC -> ZULU
    }
    else 
      if ( UTC) resdtm=displace_timestamp(resdtm, -default_tzd);  // ZULU -> UTC
  }
  while (*txt && *(const unsigned char*)txt<=' ') txt++;
  if (*txt) return FALSE;   /* end check */
  *dtm=resdtm;
  return TRUE;
}

#ifdef WINS

typedef int (__stdcall *tSystemTimeToVariantTime)(LPSYSTEMTIME lpSystemTime, double *pvtime);

static int xSystemTimeToVariantTime(LPSYSTEMTIME lpSystemTime, double *pvtime)
{ 
    static tSystemTimeToVariantTime pSystemTimeToVariantTime;
    if (!pSystemTimeToVariantTime)
    {
        HINSTANCE hOleAut = LoadLibrary("oleaut32.dll");
        pSystemTimeToVariantTime = (tSystemTimeToVariantTime)GetProcAddress(hOleAut, "SystemTimeToVariantTime");
    }
    if (pSystemTimeToVariantTime)
        return(pSystemTimeToVariantTime(lpSystemTime, pvtime));
    else
        return(0);
}

typedef int (__stdcall *tVariantTimeToSystemTime)(double vtime, LPSYSTEMTIME lpSystemTime);

static int xVariantTimeToSystemTime(double vtime, LPSYSTEMTIME lpSystemTime)
{ 
    static tVariantTimeToSystemTime pVariantTimeToSystemTime;
    if (!pVariantTimeToSystemTime)
    {
        HINSTANCE hOleAut = LoadLibrary("oleaut32.dll");
        pVariantTimeToSystemTime = (tVariantTimeToSystemTime)GetProcAddress(hOleAut, "VariantTimeToSystemTime");
    }
    if (pVariantTimeToSystemTime)
        return(pVariantTimeToSystemTime(vtime, lpSystemTime));
    else
        return(0);
}

#else

// Sem je potreba napsat variantu pro KILIX

struct SYSTEMTIME { 
    WORD wYear; 
    WORD wMonth; 
    WORD wDayOfWeek; 
    WORD wDay; 
    WORD wHour; 
    WORD wMinute; 
    WORD wSecond; 
    WORD wMilliseconds; 
};
typedef SYSTEMTIME * LPSYSTEMTIME; 

static int xSystemTimeToVariantTime(LPSYSTEMTIME lpSystemTime, double *pvtime)
{ 
    return(0);
}

static int xVariantTimeToSystemTime(double vtime, LPSYSTEMTIME lpSystemTime)
{ 
    return(0);
}

#endif


CFNC DllKernel double WINAPI timestamp2TDateTime(uns32 dtm)
{
    if (dtm == NONETIMESTAMP)
        return(0);
    uns32 Tm = dtm % 86400L;
    uns32 Dt = dtm /= 86400L;
    SYSTEMTIME st;
    st.wDay          = (WORD)(Dt % 31 + 1);  Dt /= 31; 
    st.wMonth        = (WORD)(Dt % 12 + 1);
    st.wYear         = (WORD)(Dt / 12 + 1900);
    st.wHour         = (WORD)(Tm / 3600);
    st.wMinute       = (WORD)(Tm % 3600 / 60);
    st.wSecond       = (WORD)(Tm % 60);
    st.wMilliseconds = 0;
    double tdt = 0;
    xSystemTimeToVariantTime(&st, &tdt);
    return(tdt);
}

CFNC DllKernel uns32 WINAPI TDateTime2timestamp(double tdt)
{
    if (!tdt)
        return(NONETIMESTAMP);
    SYSTEMTIME st;
    if (!xVariantTimeToSystemTime(tdt, &st))
        return(NONETIMESTAMP);

    uns32 dtm = (st.wYear  - 1900) * 12 * 31 * 86400
              + (st.wMonth - 1)    * 31 * 86400
              + (st.wDay   - 1)    * 86400
              +  st.wHour          * 3600
              +  st.wMinute        * 60
              +  st.wSecond
              +  st.wMilliseconds  / 500;
    return(dtm);
}

CFNC DllKernel double WINAPI date2TDateTime(uns32 date)
{
    if (date == NONEDATE)
        return(0);
    SYSTEMTIME st;
    st.wDay          = (WORD)(date % 31 + 1); 
    st.wMonth        = (WORD)(date / 31 % 12 + 1);
    st.wYear         = (WORD)(date / (12 * 31));
    st.wHour         = 0;
    st.wMinute       = 0;
    st.wSecond       = 0;
    st.wMilliseconds = 0;
    double tdt = 0;
    xSystemTimeToVariantTime(&st, &tdt);
    return(tdt);
}

CFNC DllKernel double WINAPI time2TDateTime(uns32 time)
{
    if (time == NONETIME)
        return(0);
    SYSTEMTIME st;
    st.wDay          = 31;
    st.wMonth        = 12;
    st.wYear         = 1899;
    st.wHour         = (WORD)(time / 3600000);
    st.wMinute       = (WORD)(time / 60000 % 60);
    st.wSecond       = (WORD)(time / 1000 % 60);
    st.wMilliseconds = (WORD)(time % 1000);
    double tdt = 0;
    xSystemTimeToVariantTime(&st, &tdt);
    return(tdt);
}

CFNC DllKernel void WINAPI TDateTime2datetime(double tdt, uns32 *date, uns32 *time)
{
    SYSTEMTIME st;
    if (!tdt || !xVariantTimeToSystemTime(tdt, &st))
    {
        *date = NONEDATE;
        *time = NONETIME;
    }
    else
    {
        *date = Make_date(st.wDay, st.wMonth, st.wYear);
        *time = Make_time(st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
    }
}

/************************** money-type operations ***************************/
int cmpmoney(monstr * m1, monstr * m2)
{ if (m1->money_hi4==m2->money_hi4)
    if (m1->money_lo2==m2->money_lo2) return 0;
    else return (m1->money_lo2 < m2->money_lo2) ? -1 : 1;
  else return (m1->money_hi4 < m2->money_hi4) ? -1 : 1;
}

void moneyadd(monstr * m1, monstr * m2)
{ uns32 pom;
  if (IS_NULLMONEY(m1)) memcpy(m1, m2, MONEY_SIZE);
  else if (!IS_NULLMONEY(m2))
  { pom=(uns32)m1->money_lo2+(uns32)m2->money_lo2;
    m1->money_lo2=(uns16)pom;
    m1->money_hi4+=m2->money_hi4;
    if (pom >> 16) m1->money_hi4++;
  }
}

void moneysub(monstr * m1, monstr * m2)
{ uns32 pom;
  if (IS_NULLMONEY(m1)) { memcpy(m1, m2, MONEY_SIZE);  money_neg(m1); }
  else if (!IS_NULLMONEY(m2))
  { pom=(uns32)m1->money_lo2-(uns32)m2->money_lo2;
    m1->money_lo2=(uns16)pom;
    m1->money_hi4-=m2->money_hi4;
    if (pom >> 16) m1->money_hi4--;
  }
}

void moneymult(monstr * m1, sig32 val)
{ uns32 pom; uns16 res_lo;  uns32 res_hi;  BOOL neg=FALSE;
  if (val==NONEINTEGER) { m1->money_hi4=NONEINTEGER;  m1->money_lo2=0; }
  else if (!IS_NULLMONEY(m1))
  { if (m1->money_hi4 < 0) { money_neg(m1);  neg=TRUE; }
    if (val < 0) { val=-val;  neg=!neg; }
    pom=(uns32)m1->money_lo2*(uns16)val;
    res_lo=(uns16)pom;  res_hi=pom>>16;
    pom=(uns32)m1->money_lo2*(val>>16);
    res_hi+=pom;
    pom=m1->money_hi4*val;
    res_hi+=pom;
    m1->money_hi4=res_hi;  m1->money_lo2=res_lo;
    if (neg) money_neg(m1);
  }
}

void moneydiv(monstr * m1, monstr * m2)
{ if (IS_NULLMONEY(m2)) memcpy(m1, m2, MONEY_SIZE);
  else if (!IS_NULLMONEY(m1))
    real2money(money2real(m1) / money2real(m2), m1);
}

/*************************** date & time ************************************/
CFNC DllKernel uns32 WINAPI Make_date(int day, int month, int year)
{ if ((day <1) || (day >31) || (month<1) || (month>12) ||
      (year<0) || (year>9999) ||
      (day==31) && ((month==4)||(month==6)||(month==9)||(month==11)))
    return NONEDATE;
  if ((month==2) && (day>28))
  { if (day>29) return NONEDATE;
    if (year % 4) return NONEDATE;
    if (!(year % 100) && (year % 400)) return NONEDATE;
  }
  return (sig32)(day-1 + 31 * (month - 1 + 12 * (uns32)year));
}
CFNC DllKernel int   WINAPI Day(uns32 date)     { return date==NONEDATE ? NONEINTEGER : (date % 31) + 1; }
CFNC DllKernel int   WINAPI Month(uns32 date)   { return date==NONEDATE ? NONEINTEGER : (date / 31 % 12) + 1; }
CFNC DllKernel int   WINAPI Year(uns32 date)    { return date==NONEDATE ? NONEINTEGER :  date / (12*31); }
CFNC DllKernel int   WINAPI Quarter(uns32 date) { return date==NONEDATE ? NONEINTEGER : ((date / 31 % 12) / 3) + 1; }
CFNC DllKernel uns32 WINAPI Today(void)
{ uns8 day, month;  int year;
  xgetdate(&day, &month, &year);
  return day-1 + 31 * (month - 1 + 12 * (uns32)year);
}
CFNC DllKernel int   WINAPI Day_of_week(uns32 date)
{ struct tm atm;
  atm.tm_mday = Day(date);
  atm.tm_mon  = Month(date)-1;
  atm.tm_year = Year(date)-1900;
  atm.tm_hour = 0;
  atm.tm_min  = 0;
  atm.tm_sec  = 1;
  atm.tm_isdst= -1;
 /* call mktime to fill in the weekday field of the structure */
  if (date==NONEDATE || mktime(&atm) == -1) return 7;
  return atm.tm_wday;
}

CFNC DllKernel uns32 WINAPI Make_time(int hour, int minute, int second, int sec1000)
{ if (hour<0 || hour>23 || minute<0 || minute>59 || second<0 || second>59 ||
      sec1000<0 || sec1000 > 999)
    return NONETIME;
  return (sig32)(sec1000 + 1000*(second + 60L*(minute + 60*hour)));
}
CFNC DllKernel int   WINAPI Hours(uns32 time)       { return time==NONETIME ? NONEINTEGER : (time / 3600000L); }
CFNC DllKernel int   WINAPI Minutes(uns32 time)     { return time==NONETIME ? NONEINTEGER : (time / 60000L % 60); }
CFNC DllKernel int   WINAPI Seconds(uns32 time)     { return time==NONETIME ? NONEINTEGER : (time / 1000 % 60); }
CFNC DllKernel int   WINAPI Sec1000(uns32 time)     { return time==NONETIME ? NONEINTEGER : (time % 1000); }
CFNC DllKernel uns32 WINAPI Now(void)               { return 1000 * clock_now(); }

/******************************** strings ***********************************/
CFNC DllKernel char WINAPI upcase_charA(char c, wbcharset_t charset)
{ return charset.case_table()[(uns8)c]; }
#ifdef STOP
#ifdef WINS
{ char res;
  LCMapStringA(charset.lcid(), LCMAP_UPPERCASE, &c, 1, &res, 1);
  return res;
}
#else
{ return __toupper_l(c, charset); }
#endif
#endif
                              
CFNC DllKernel wchar_t WINAPI upcase_charW(wchar_t c, wbcharset_t charset)
#ifdef WINS
{ //wuchar res;
  //LCMapStringW(charset.lcid(), LCMAP_UPPERCASE, &c, 1, &res, 1);
  //return res;
  return (wchar_t)CharUpperW((LPWSTR)(c));
}
#else
{
	//return __towupper_l(c, charset.locales16());  -- not working on Suse 9.1
	return towupper(c);
}
#endif 

CFNC DllKernel BOOL WINAPI general_Pref(const char * s1, const char * s2, t_specif specif)
{ if (specif.wide_char)
    if (specif.ignore_case)
      while (*(wuchar*)s1)
      { if (upcase_charW(*(wuchar*)s1, specif.charset)!=upcase_charW(*(wuchar*)s2, specif.charset)) 
          return FALSE;
        s1+=2; s2+=2;
      }
    else // case-sensitive
      while (*(wuchar*)s1)
      { if (*(wuchar*)s1!=*(wuchar*)s2) return FALSE;
        s1+=2; s2+=2;
      }
  else  // narrow char
    if (specif.ignore_case)
      if (specif.charset>1)
        while (*s1)
        { if (upcase_charA(*s1, specif.charset)!=upcase_charA(*s2, specif.charset)) return FALSE;
          s1++; s2++;
        }
      else
        while (*s1)
        { if (mcUPCASE((uns8)*s1)!=mcUPCASE((uns8)*s2)) return FALSE;
          s1++; s2++;
        }
    else // case-sensitive
      while (*s1)
      { if (*s1!=*s2) return FALSE;
        s1++; s2++;
      }
  return TRUE;
}

CFNC DllKernel int WINAPI general_Substr(const char * s1, const char * s2, t_specif specif)
// Search for s1 in s2. Result for SQL 2: 0 if not found or 1-based position.
{ int pos=1, poss;
  if (specif.wide_char)
    if (specif.ignore_case)
    { int poss = (int)wuclen((wuchar*)s2) - (int)wuclen((wuchar*)s1);  // [poss] is the number of (remaining) possible positions
      while (poss>=0)
      { const wuchar * sw1=(wuchar*)s1, * sw2=(wuchar*)s2;
        do // comparing sw1 and sw2
        { if (!*sw1) return pos;
          if (upcase_charW(*sw1, specif.charset)!=upcase_charW(*sw2, specif.charset)) break;
          sw1++; sw2++;
        } while (TRUE);
        s2+=2;  poss--;  pos++;  // try the next position
      }
    }
    else // case-sensitive
    { poss = (int)wuclen((wuchar*)s2) - (int)wuclen((wuchar*)s1);  // [poss] is the number of (remaining) possible positions
      while (poss>=0)
      { const wuchar * sw1=(wuchar*)s1, * sw2=(wuchar*)s2;
        do // comparing sw1 and sw2
        { if (!*sw1) return pos;
          if (*sw1!=*sw2) break;
          sw1++; sw2++;
        } while (TRUE);
        s2+=2;  poss--;  pos++;  // try the next position
      }
    }
  else  // narrow char
    if (specif.ignore_case)
    { poss = (int)strlen(s2) - (int)strlen(s1);  // [poss] is the number of (remaining) possible positions
      while (poss>=0)
      { const char * sw1=s1, * sw2=s2;
        do // comparing sw1 and sw2
        { if (!*sw1) return pos;
          if (specif.charset>1 ? upcase_charA(*sw1, specif.charset)!=upcase_charA(*sw2, specif.charset) :
                                 mcUPCASE((uns8)*sw1) != mcUPCASE((uns8)*sw2)) break;
          sw1++; sw2++;
        } while (TRUE);
        s2++;  poss--;  pos++;  // try the next position
      }
    }
    else // case-sensitive
    { poss = (int)strlen(s2) - (int)strlen(s1);  // [poss] is the number of (remaining) possible positions
      while (poss>=0)
      { const char * sw1=s1, * sw2=s2;
        do // comparing sw1 and sw2
        { if (!*sw1) return pos;
          if (*sw1!=*sw2) break;
          sw1++; sw2++;
        } while (TRUE);
        s2++;  poss--;  pos++;  // try the next position
      }
    }
  return 0;
}

BOOL like_esc(const char * str, const char * patt, char esc, BOOL ignore, wbcharset_t charset)
{ do
  { char pattchar=*(patt++);
    if (esc && pattchar==esc) pattchar=*(patt++);
    else
    { if (pattchar=='%') break;
      if (pattchar=='_')
        if (!*str) return FALSE;
        else goto next;
    }
    if (!ignore   ? pattchar                       !=*str : 
        charset.is_wb8() ? upcase_charA(pattchar, charset)!=upcase_charA(*str, charset) :
                    cUPCASE(pattchar)              !=cUPCASE(*str))
      return FALSE;
    if (!pattchar) return TRUE;
   next:
    str++;
  } while (TRUE);
 // % found, match the rest of patt with str and all its suffixes:
  do
  { if (like_esc(str, patt, esc, ignore, charset)) return TRUE;
    if (!*str) return FALSE;
    str++;
  } while (TRUE);
}

BOOL wlike_esc(const wuchar * str, const wuchar * patt, wuchar esc, BOOL ignore, wbcharset_t charset)
{ do
  { wuchar pattchar=*(patt++);
    if (esc && pattchar==esc) pattchar=*(patt++);
    else
    { if (pattchar=='%') break;
      if (pattchar=='_')
        if (!*str) return FALSE;
        else goto next;
    }
    if (!ignore   ? pattchar                       !=*str : 
                    upcase_charW(pattchar, charset)!=upcase_charW(*str, charset))
      return FALSE;
    if (!pattchar) return TRUE;
   next:
    str++;
  } while (TRUE);
 // % found, match the rest of patt with str and all its suffixes:
  do
  { if (wlike_esc(str, patt, esc, ignore, charset)) return TRUE;
    if (!*str) return FALSE;
    str++;
  } while (TRUE);
}

#ifdef WINS
CFNC DllKernel void WINAPI convert_to_uppercaseW(const wuchar * src, wuchar * dest, wbcharset_t charset)
{ LCMapStringW(charset.lcid(), LCMAP_UPPERCASE, src, -1, dest, 0x7fffffff); }
CFNC DllKernel void WINAPI convert_to_lowercaseW(const wuchar * src, wuchar * dest, wbcharset_t charset)
{ LCMapStringW(charset.lcid(), LCMAP_LOWERCASE, src, -1, dest, 0x7fffffff); }
#else
CFNC DllKernel void WINAPI convert_to_uppercaseW(const wuchar * src, wuchar * dest, wbcharset_t charset)
{ while (0!=*src) 
    //*dest++=__towupper_l(*src++, charset.locales16()); 
    *dest++=towupper(*src++); 
}
CFNC DllKernel void WINAPI convert_to_lowercaseW(const wuchar * src, wuchar * dest, wbcharset_t charset)
{ while (0!=*src) 
    //*dest++=__towlower_l(*src++, charset.locales16()); 
	*dest++=towlower(*src++); 
}

#include <iconv.h>
#endif

CFNC DllKernel bool WINAPI is_AlphaA(char ch, int charset)
{
  return (unsigned char)ch <= '9' || ch==' ' || ch=='.' ? false : wbcharset_t(charset).ascii_table()[(unsigned char)ch] != '_';
#if 0  // __isalpha_l sometimes does not work, the above version is faster
#ifdef WINS
  WORD res;
  //GetStringTypeA(wbcharset_t(charset).lcid(), CT_CTYPE1, &ch, 1, &res); -- must not use this, lcid() does not make difference between ISO and WIN pages
  wchar_t wch;
  MultiByteToWideChar(wbcharset_t(charset).cp(), 0, &ch, 1, &wch, 1);
  GetStringTypeW(CT_CTYPE1, &wch, 1, &res); 
  return (res & C1_ALPHA) != 0;
#else
  if (charset==1) // implemented internally for the case that cp1250 is not installed
    return UPLETTER(mcUPCASE((uns8)ch));
  return __isalpha_l(ch, wbcharset_t(charset).locales8()) != 0;
#endif
#endif
}

CFNC DllKernel int WINAPI conv2to1(const wuchar * source, int slen, char * target, wbcharset_t charset, int dbytelen)
// Terminator byte should not be included in [slen] and is always added on the end (even if exceeds [dbytelen]).
{ int retcode = 0; // OK result
#ifdef WINS
  int dlen=0;
  if (slen==-1) slen=(int)wcslen(source);
  if (slen && !(dlen=WideCharToMultiByte(charset.cp(), 0, source, slen, target, dbytelen ? dbytelen : 2*slen, NULL, NULL)))  // 2*slen for UTF-8
  { if (GetLastError() == ERROR_INSUFFICIENT_BUFFER && dbytelen>0)
    { do  // converting a maximal prefix:
        if (slen>1000) slen=slen/5*4;  else slen--;  // is faster for long strings than slen--;
      while (slen && !(dlen=WideCharToMultiByte(charset.cp(), 0, source, slen, target, dbytelen, NULL, NULL)));
      target+=dlen;
      retcode=SC_STRING_TRIMMED;
    }
    else
      retcode=SC_INCONVERTIBLE_INPUT_STRING;
  }
  else
    target+=dlen;
#else
  if (slen==-1)  // no system function for UCS-2 length in LINUX
  { slen=0;
    while (source[slen]) slen++;
  }
  const wuchar *from=source;
  char *to=target;
  size_t tolen=dbytelen ? dbytelen : 2*slen;  // space for UTF-8 characters
  size_t fromlen=2*slen;
  iconv_t conv=iconv_open(charset, "UCS-2");
  if (conv==(iconv_t)-1) retcode=SC_INCONVERTIBLE_INPUT_STRING;
  else
  { while (fromlen>0)
    { if (!tolen)
        { retcode=SC_STRING_TRIMMED;  break; }
      int nconv=iconv(conv, (char **)&from, &fromlen, &to, &tolen);
      if (nconv==(size_t)-1)  
        { retcode = errno==E2BIG ? SC_STRING_TRIMMED : SC_INCONVERTIBLE_INPUT_STRING;  break; }
    }
    iconv_close(conv);
    /* TODO: handler prezivajici mezi konverzemi? */
    target=to;
  }  
#endif
  *target=0;  // terminates the converted string or part
  return retcode;
}

CFNC DllKernel bool WINAPI is_AlphaW(wchar_t ch, int charset)
{
#ifdef WINS
  WORD res;
  if (!(GetVersion() & 0x80000000))  // NT, GetStringTypeW is supported
  { GetStringTypeW(CT_CTYPE1, &ch, 1, &res);
    return (res & C1_ALPHA) != 0;
  }
  else  // W98, Me etc.: GetStringTypeW not available
  { char ch8;
    if (!WideCharToMultiByte(wbcharset_t(charset).cp(), 0, (wuchar*)&ch, 1, &ch8, 1, NULL, NULL))
      return false;
    return (unsigned char)ch8 <= '9' || ch8==' ' || ch8=='.' ? false : wbcharset_t(charset).ascii_table()[(unsigned char)ch8] != '_';
  }
#else
  //return __iswalpha_l(ch, wbcharset_t(charset).locales16()) != 0; -- does not work, event for LC_ALL_MASK
  //return __iswalpha(ch) != 0;  -- this function is not defined
  char ch1[2];  // will write terminator!
  if (conv2to1((wuchar *)&ch, 1, ch1, charset, 1)) // conversion error
    return false;
  return is_AlphaA(ch1[0], charset);
#endif
}

CFNC DllKernel bool WINAPI is_AlphanumA(char ch, int charset)
{ //return is_AlphaA(ch, charset) || ch>='0' && ch<='9' || ch=='_'; 
  return ch == '_' || wbcharset_t(charset).ascii_table()[(unsigned char)ch] != '_'  && ch!=' ' && ch!='.' && ch;  // fast and accurate
}

CFNC DllKernel bool WINAPI is_AlphanumW(wchar_t ch, int charset)
{ return ch>='0' && ch<='9' || ch=='_' || is_AlphaW(ch, charset); }

#ifdef SRVR  // 8-bit function used for system object names on the server
void sys_Upcase(char * str)
{ const uns8 * casetab = wbcharset_t(sys_spec.charset).case_table();
  while (*str) { *str=casetab[(uns8)*str];  str++; }
}

bool sys_Alpha(char ch)
{ return is_AlphaA(ch, sys_spec.charset); }

bool sys_Alphanum(char ch)
{ return is_AlphanumA(ch, sys_spec.charset); }

int sys_stricmp(const char * str1, const char * str2)
{
#ifdef WINS
  return cmp_str(str1, str2, t_specif(0xffff, sys_spec.charset, true, false));
#else
  if (sys_spec.charset==1) // implemented internally for the case that cp1250 is not installed
    return cmp_str(str1, str2, t_specif(0xffff, 1, true, false));  
  return __strncasecmp_l(str1, str2, 0x7fffffff, wbcharset_t(sys_spec.charset).locales8());
#endif
}
#endif

CFNC DllKernel void WINAPI convert_to_uppercaseA(const char * src, char * dest, wbcharset_t charset)
{ const uns8 * casetab = charset.case_table();
  while (*src) *dest++=casetab[(uns8)*src++];
  *dest=0;
#ifdef STOP
  if (charset.is_wb8())
#ifdef WINS
    LCMapStringA(charset.lcid(), LCMAP_UPPERCASE, src, -1, dest, 0x7fffffff);
#else
    while (0!=*src) *dest++=__toupper_l(*src++, charset);
#endif
  else 
  { strcpy(dest, src);
    Upcase(dest);
  }
#endif
}

CFNC DllKernel void WINAPI convert_to_lowercaseA(const char * src, char * dest, wbcharset_t charset)
// Problems: ISO code pages may not be available on Windows
// __tolower_l does not work on SuSe 9.1
// Preferring manual conversion using the case tables.
{ 
#ifdef WINS
  if (charset.is_wb8() && !charset.is_iso())  
  { LCMapStringA(charset.lcid(), LCMAP_LOWERCASE, src, -1, dest, 0x7fffffff);
	return;
  }	
#endif  
  if (charset.is_ansi())
  { while (*src)
	{ unsigned char c = *src;
	  *dest = (c>='A' && c<='Z') ? c+0x20 : c;
      src++;  dest++;
    }
  }
  else  // converting via case_table when OS may not support the code page
  { const uns8 * casetab = charset.case_table();
    while (*src)
    { unsigned char c = *src;
      if (c<128)
        *dest = (c>='A' && c<='Z') ? c+0x20 : c;
      else
      { *dest = c;
        for (int i=128; i<256; i++)
        if (casetab[i]==c && ((unsigned char)i!=c))
          { *dest = (char)i;  break; }
      }
      dest++;  src++;
    }
  }
  *dest=0;
 // not working on SuSe:   
 // while (0!=*src) *dest++=__tolower_l(*src++, charset.locales8());
}

int compare_values(int valtype, t_specif specif, const char * key1, const char * key2)
{ int res;
	switch (valtype)
	{case ATT_BOOLEAN: res=(int)*(signed char*)key1-(int)*(signed char*)key2; break;
	 case ATT_CHAR:    res=(int)(uns8)*key1-(int)(uns8)*key2; break;
   case ATT_INT8:    res=(*(sig8 *)key1< *(sig8 *)key2) ? -1 :
  						         ( (*(sig8 *)key1==*(sig8 *)key2) ? 0 : 1);  break;
	 case ATT_INT16:   res=(*(sig16 *)key1< *(sig16 *)key2) ? -1 :
							         ( (*(sig16 *)key1==*(sig16 *)key2) ? 0 : 1);  break;
	 case ATT_MONEY:   res=cmpmoney((monstr *)key1, (monstr*)key2);  break;
	 case ATT_TIME:  case ATT_DATE:  case ATT_DATIM:  case ATT_TIMESTAMP:
       if        (*(uns32 *)key1==NONEDATE) res = *(uns32 *)key2==NONEDATE? 0 : -1;
       else if   (*(uns32 *)key2==NONEDATE) res = 1;
       else res=((*(uns32 *)key1< *(uns32 *)key2) ? -1 :
		           ( (*(uns32 *)key1==*(uns32 *)key2) ? 0 : 1));  break;
	 case ATT_INT32:   res=((*(sig32 *)key1< *(sig32 *)key2) ? -1 :
							          ( (*(sig32 *)key1==*(sig32 *)key2) ? 0 : 1));  break;
   case ATT_INT64:   res=((*(sig64 *)key1< *(sig64 *)key2) ? -1 :
							          ( (*(sig64 *)key1==*(sig64 *)key2) ? 0 : 1));  break;
	 case ATT_PTR:    case ATT_BIPTR:
							 res=((*(sig32 *)key1<*(sig32 *)key2)     ? -1 :
							 ( (*(sig32 *)key1==*(sig32 *)key2) ? 0 : 1));  break;
	 case ATT_FLOAT:   res=((*(double*)key1<*(double*)key2) ? -1 :
							 ( (*(double*)key1==*(double*)key2) ? 0 : 1));  break;
	 case ATT_STRING:
     res=cmp_str(key1, key2, specif);  break;
   case ATT_BINARY:
   { res=0;  const uns8 * s1=(uns8*)key1, * s2=(uns8*)key2;  int maxlen=specif.length;
		 while (maxlen--)
       if (*s1!=*s2)
         { res = *s1 < *s2 ? -1 : 1;  break; }
       else
         { s1++;  s2++; }
     break;
   }
   default:
     res=0;  break;
  }
  return res;
}

static monstr auxmon;

int WINAPI sp_strpos(tptr subs, tptr s)
{ unsigned lsu = (unsigned)strlen(subs), ls = (unsigned)strlen(s), pos = 0;
  tptr psu, ps;
  while (pos+lsu <= ls)
  { psu=subs;  ps=s+pos;
    while ((*psu==*ps) && *psu)
      { ps++;  psu++; }
	 if (!*psu) return pos+1;    /* 1 based result */
    pos++;
  }
  return 0;  /* not found */
}

tptr WINAPI sp_strinsert(tptr source, tptr dest, unsigned index)
{ unsigned ls = (unsigned)strlen(source), ld = (unsigned)strlen(dest);
  if (!index) return dest;   /* action not defined */
  index--;  /* 0 based */
  if (ld >= index)
    memmov(dest+index+ls, dest+index, ld-index+1);
  else
  { memset(dest+ld, ' ', index-ld);
    ls++;   /* copy the terminating 0 too */
  }
  memcpy(dest+index, source, ls);  /* inserting */
  return dest;
}

tptr WINAPI sp_strdelete(tptr s, unsigned index, unsigned count)
{ unsigned len = (unsigned)strlen(s);
  if (!index) return s;
  index--;  /* 0 based */
  if (index >= len) return s;  /* nothing to be deleted */
  if (count > len-index) count=len-index;
  memmov(s+index, s+index+count, len-index-count+1);
  return s;
}

sig32 WINAPI sp_str2int(char * txt)
{ sig32 val;
  if (str2int (txt, &val)) return val; else return NONEINTEGER;
}
monstr * WINAPI sp_str2money(char * txt)
{ if (!str2money(txt, &auxmon))
    { auxmon.money_hi4=NONEINTEGER;  auxmon.money_lo2=0; }
  return &auxmon;
}
double WINAPI sp_str2real(char * txt)
{ double val;
  if (str2real(txt, &val)) return val; else return NULLREAL;
}
uns32 WINAPI sp_str2date(char * txt)
{ uns32 val;
  if (str2date(txt, &val)) return val; else return NONEDATE;
}
uns32 WINAPI sp_str2time(char * txt)
{ uns32 val;
  if (str2time(txt, &val)) return val; else return NONETIME;
}
uns32 WINAPI sp_str2timestamp(char * txt)
{ uns32 val;
  if (str2timestamp(txt, &val)) return val; else return NONETIMESTAMP;
}
sig64 WINAPI sp_str2bigint(char * txt)
{ sig64 val;
  if (str2int64 (txt, &val)) return val; else return NONEBIGINT;
}

int    WINAPI sp_ord(uns8 c)   { return (int)c; }
uns8   WINAPI sp_chr(int i)   { return (uns8)i; }
BOOL   WINAPI sp_odd(sig32 i)   { return (i==NONEINTEGER) ? NONEBOOLEAN : (i & 1) != 0; }
double WINAPI sp_abs(double r)  { return (r==NULLREAL   ) || (r>=0) ? r:-r; }
sig32  WINAPI sp_iabs(sig32 i)  { return (i==NONEINTEGER) || (i>=0) ? i:-i; }
sig32  WINAPI sp_round(double r)
 { return (r==NULLREAL) ? NONEINTEGER : ((r<0.0f) ? (sig32)(r-0.5f):(sig32)(r+0.5f)); }
sig32  WINAPI sp_trunc(double r)
 { return (r==NULLREAL) ? NONEINTEGER : (sig32)r; }
double WINAPI sp_sqr(double r)      { return (r==NONEREAL) ? NONEREAL : r*r; }
sig32  WINAPI sp_isqr(sig32 i)      { return (i==NONEINTEGER) ? NONEINTEGER : i*i; }

#ifdef WINS
#undef _MBCS
#include  <tchar.h>
__inline size_t __cdecl _wcsncnt( const wchar_t * _cpc, size_t _sz) { size_t len; len = wcslen(_cpc); return (len>_sz) ? _sz : len; }
#else
#include  <wchar.h>
#endif

CFNC DllKernel void WINAPI strmaxcpy(char * destin, const char * source, size_t maxsize)
{ size_t len;
  if (!maxsize) return;
  maxsize--;
#if 0
  len=strlen(source);
  if (len > maxsize) len=maxsize;
#else  // safe version: [source] may be an unterminated string and [destin] may not be big enough for [maxsize] bytes
#ifdef WINS
  len = _strncnt(source, maxsize);
#else
  len = strnlen(source, maxsize);
#endif
#endif
  memcpy(destin, source, len);
  destin[len]=0;
}

CFNC DllKernel void WINAPI wcsmaxcpy(wchar_t * destin, const wchar_t * source, unsigned maxchars)
{ unsigned len;
  if (!maxchars) return;
  maxchars--;
#if 0
  len=wcslen(source);
  if (len > maxchars) len=maxchars;
#else  // safe version: [source] may be an unterminated string and [destin] may not be big enough for [maxsize] bytes
#ifdef WINS
  len = (unsigned)_wcsncnt(source, maxchars);
#else
  len = (unsigned)wcsnlen(source, maxchars);
#endif
#endif
  memcpy(destin, source, len*sizeof(wchar_t));
  destin[len]=0;
}

CFNC DllKernel unsigned WINAPI wucmaxlen(const wuchar * source, unsigned maxchars)
{ unsigned len;
#if 0
  len=wuclen(source);
  if (len > maxchars) len=maxchars;
#else  // safe version: [source] may be an unterminated string and [destin] may not be big enough for [maxsize] bytes
#ifdef WINS
  len = (unsigned)_wcsncnt(source, maxchars);
#else
  len=0;
  while (source[len] && len<maxchars) len++;
#endif
#endif
  return len;
}

CFNC DllKernel void WINAPI wucmaxcpy(wuchar * destin, const wuchar * source, unsigned maxchars)
{ unsigned len;
  if (!maxchars) return;
  maxchars--;
  len=wucmaxlen(source, maxchars);
  memcpy(destin, source, len*sizeof(wuchar));
  destin[len]=0;
}

CFNC DllKernel unsigned WINAPI strmaxlen(const char * str, unsigned maxsize)
{ if (memchr(str, 0, maxsize)) return (unsigned)strlen(str);
  return maxsize;
}

CFNC DllKernel uns8 WINAPI small_char(unsigned char c)
{ int i;
  if ((c>='A')&&(c<='Z')) return (unsigned char)(c+0x20);
  if (c<128) return c;
  for (i=128; i<256; i++)
    if ((csconv[i]==c) && ((unsigned char)i!=c))
      return (unsigned char)i;
  return c;
}

CFNC DllKernel void WINAPI small_string(tptr s, BOOL captfirst)
{ if (!s || !*s) return;
  if (captfirst) { *s=mcUPCASE((uns8)*s);  s++; }
  while (*s) { *s=small_char(*s);  s++; }
}



CFNC DllKernel void WINAPI Upcase(tptr txt)
// Obsolete - uses cp1250 only. Do not use!
{ while (*txt) { *txt=mcUPCASE((uns8)*txt); txt++; }
}

CFNC DllKernel void WINAPI Upcase9(cdp_t cdp, char * txt)
{ 
#if WBVERS<900
  Upcase(txt);
#else  
#ifdef SRVR
  sys_Upcase(txt);
#else  
  if (cdp) convert_to_uppercaseA(txt, txt, cdp->sys_charset); 
  else Upcase(txt);
#endif  
#endif  
}  

#ifndef SRVR
CFNC DllKernel BOOL WINAPI Search_in_block(const char * block, uns32 blocksize,
                         const char * next, const char * patt, uns16 pattsize,
                         uns16 flags, uns32 * respos)
{ unsigned pos, matched;  char c;
  if (flags & SEARCH_BACKWARDS)  /* "next" not used if searching backwards */
  { if (pattsize>blocksize) return FALSE;
	 pos=blocksize-pattsize;
    do
    { matched=0;
      do
      { if (!(flags & SEARCH_CASE_SENSITIVE)) c=mcUPCASE((uns8)block[pos]);
        else c=block[pos];
        if (c==patt[matched])
        { pos++;
          if (++matched==pattsize)
          { if (flags & SEARCH_WHOLE_WORDS)
            { if (pos>matched)   if (SYMCHAR(block[pos-matched-1])) break;
              if (pos<blocksize) if (SYMCHAR(block[pos])) break;
            }
				*respos=pos-matched;  return TRUE;
          }
        }
        else break;
      } while (TRUE);
		pos=pos-matched;
    } while (pos--);
  }
  else
  { pos=0;
    while (pos < blocksize)
    { matched=0;
      do
      { if (pos<blocksize) c=block[pos];
        else
          if (next) c=next[pos-blocksize];
          else break;
        if (!(flags & SEARCH_CASE_SENSITIVE)) c=mcUPCASE((uns8)c);
        if (c==patt[matched])
        { pos++;
          if (++matched==pattsize)
          { if (flags & SEARCH_WHOLE_WORDS)
            { if (pos>matched)
                if (SYMCHAR(block[pos-matched-1])) break;
                else if (flags & SEARCH_NOCONT) break;
              if (pos<blocksize)
                if (SYMCHAR(block[pos])) break;
                else if (next) if (SYMCHAR(block[pos-blocksize])) break;
            }
				*respos=pos-matched;  return TRUE;
          }
        }
        else break;
      } while (TRUE);
		pos=pos-matched+1;
    }
  }
  return FALSE;
}

CFNC DllKernel bool WINAPI Search_in_blockA(const char * block, uns32 blocksize, const char * next, const char * patt, int pattsize,
                      unsigned flags, int charset, uns32 * respos)
// AXIOM: if (!(flags & SEARCH_CASE_SENSITIVE)) then [patt] is in upper case.
{ unsigned pos, matched;  char c;
  if (flags & SEARCH_BACKWARDS)  /* "next" not used if searching backwards */
  { if (pattsize>blocksize) return FALSE;
	 pos=blocksize-pattsize;
    do
    { matched=0;
      do
      { c=block[pos];
        if (!(flags & SEARCH_CASE_SENSITIVE)) 
          c=upcase_charA(c, charset);
        if (c==patt[matched])
        { pos++;
          if (++matched==pattsize)
          { if (flags & SEARCH_WHOLE_WORDS)
            { if (pos>matched)   if (is_AlphanumA(block[pos-matched-1], charset)) break;
              if (pos<blocksize) if (is_AlphanumA(block[pos], charset)) break;
            }
				    *respos=pos-matched;  return TRUE;
          }
        }
        else break;
      } while (TRUE);
		  pos=pos-matched;
    } while (pos--);
  }
  else
  { pos=0;
    while (pos < blocksize)
    { matched=0;
      do
      { if (pos<blocksize) c=block[pos];
        else
          if (next) c=next[pos-blocksize];
          else break;
        if (!(flags & SEARCH_CASE_SENSITIVE)) c=upcase_charA(c, charset);
        if (c==patt[matched])
        { pos++;
          if (++matched==pattsize)
          { if (flags & SEARCH_WHOLE_WORDS)
            { if (pos>matched)
                if (is_AlphanumA(block[pos-matched-1], charset)) break;
                else if (flags & SEARCH_NOCONT) break;
              if (pos<blocksize)
                if (is_AlphanumA(block[pos], charset)) break;
                else if (next) if (is_AlphanumA(block[pos-blocksize], charset)) break;
            }
				    *respos=pos-matched;  return TRUE;
          }
        }
        else break;
      } while (TRUE);
      pos=pos-matched+1;
    }
  }
  return FALSE;
}

CFNC DllKernel bool WINAPI Search_in_blockW(const wchar_t * block, uns32 blocksize, const wchar_t * next, const wchar_t * patt, int pattsize,
                      unsigned flags, int charset, uns32 * respos)
// AXIOM: if (!(flags & SEARCH_CASE_SENSITIVE)) then [patt] is in upper case.
{ unsigned pos, matched;  wchar_t c;
  if (flags & SEARCH_BACKWARDS)  /* "next" not used if searching backwards */
  { if (pattsize>blocksize) return FALSE;
	 pos=blocksize-pattsize;
    do
    { matched=0;
      do
      { c=block[pos];
        if (!(flags & SEARCH_CASE_SENSITIVE)) 
          c=upcase_charW(c, charset);
        if (c==patt[matched])
        { pos++;
          if (++matched==pattsize)
          { if (flags & SEARCH_WHOLE_WORDS)
            { if (pos>matched)   if (is_AlphanumW(block[pos-matched-1], charset)) break;
              if (pos<blocksize) if (is_AlphanumW(block[pos], charset)) break;
            }
				    *respos=pos-matched;  return TRUE;
          }
        }
        else break;
      } while (TRUE);
		  pos=pos-matched;
    } while (pos--);
  }
  else
  { pos=0;
    while (pos < blocksize)
    { matched=0;
      do
      { if (pos<blocksize) c=block[pos];
        else
          if (next) c=next[pos-blocksize];
          else break;
        if (!(flags & SEARCH_CASE_SENSITIVE)) c=upcase_charW(c, charset);;
        if (c==patt[matched])
        { pos++;
          if (++matched==pattsize)
          { if (flags & SEARCH_WHOLE_WORDS)
            { if (pos>matched)
                if (is_AlphanumW(block[pos-matched-1], charset)) break;
                else if (flags & SEARCH_NOCONT) break;
              if (pos<blocksize)
                if (is_AlphanumW(block[pos], charset)) break;
                else if (next) if (is_AlphanumW(block[pos-blocksize], charset)) break;
            }
				    *respos=pos-matched;  return TRUE;
          }
        }
        else break;
      } while (TRUE);
      pos=pos-matched+1;
    }
  }
  return FALSE;
}

#endif
///////////////////////////////////////////////////////////////////////////////////
CFNC DllKernel BOOL WINAPI is_string(unsigned tp)
{ return IS_STRING(tp); }

CFNC DllKernel BOOL WINAPI speciflen(unsigned tp)
{ return SPECIFLEN(tp); }

CFNC DllKernel void WINAPI xgettime(uns8 * ti_sec, uns8 * ti_min, uns8 * ti_hour)
{ time_t long_time;  struct tm * tmbuf;
  long_time=time(NULL);  tmbuf=localtime(&long_time);
  if (tmbuf==NULL) { *ti_sec=0;  *ti_min=0;  *ti_hour=0; }
  else
  { *ti_sec =(uns8)tmbuf->tm_sec;  *ti_min=(uns8)tmbuf->tm_min;
    *ti_hour=(uns8)tmbuf->tm_hour;
  }
}

CFNC DllKernel void WINAPI xgetdate(uns8 * da_day, uns8 * da_mon, int * da_year)
{ time_t long_time;  struct tm * tmbuf;
  long_time=time(NULL);  tmbuf=localtime(&long_time);
  if (tmbuf==NULL) {  *da_day = 1;  *da_mon=1;  *da_year=2100; }
  else
  { *da_day =(uns8)tmbuf->tm_mday;  *da_mon=(uns8)(tmbuf->tm_mon+1);
    *da_year=1900+tmbuf->tm_year;
  }
}

uns32 clock_now(void)  /* returns the clock value */
{ uns8 sec, min, hour;
  xgettime(&sec, &min, &hour);
  return 60L*(60*hour+min)+sec;
}

CFNC DllKernel timestamp WINAPI stamp_now(void)  /* returns the standard timestamp */
{ time_t long_time;  struct tm * tmbuf;
  long_time=time(NULL);  tmbuf=localtime(&long_time);
  if (tmbuf==NULL) return 0;
  return 60L*(60*tmbuf->tm_hour+tmbuf->tm_min)+tmbuf->tm_sec + 
         86400L*((tmbuf->tm_mday-1)+31L*(tmbuf->tm_mon+12*tmbuf->tm_year));
}

#ifndef SRVR // on server defined in a different way
CFNC DllExport uns32 WINAPI Current_timestamp(void)
{ return stamp_now(); }
#endif
/****************************** typesize ************************************/
#ifndef __WXGTK__
#define PIECESIZE 6  // sizeof(tpiece)
uns8 tpsize[ATT_AFTERLAST] =
 {0,                                   /* fictive, NULL */
  1,1,2,4,6,8,0,0,0,0,4,4,4,sizeof(trecnum),sizeof(trecnum), /* normal  */
  UUID_SIZE,4,4+PIECESIZE,        /* special */
  4+PIECESIZE,4+PIECESIZE,   /* raster, text */
  4+PIECESIZE,4+PIECESIZE,   /* nospec, signature */
#ifdef SRVR
  0,               // UNTYPED is not used on server
#else
  sizeof(untstr),  // UNTYPED
#endif
  0,0,0,0,    /* not used & ATT_NIL */
  1,sizeof(ttablenum),3*sizeof(tobjnum),8,0,3*sizeof(tcursnum), /* file, table, cursor, dbrec, view, statcurs */
  0,0,0, 0,0,0, 0,0,0, 0,0,
  1, 8, 0, EXTLOBSIZE, EXTLOBSIZE };                       // int8, int64, domain, extclob, extblob
/* NOTE: 0 size for string type is used when generating I_LOAD */
// cursor variable must have 3 objnums, they are checked on Close_cursor()!
#endif
DllKernel int WINAPI typesize(const d_attr * pdatr)
{ int tp = pdatr->type & 0x7f;
  return is_string(tp) || tp==ATT_BINARY ? pdatr->specif.length : tpsize[tp]; 
}

CFNC DllKernel int WINAPI type_specif_size(int tp, t_specif specif)
{ tp &= 0x7f;
  if (tp==ATT_CHAR) return specif.wide_char ? 2 : 1;  // probably the specif.length is defined, but I am not sure
  return is_string(tp) || tp==ATT_BINARY ? specif.length : tpsize[tp]; 
}

CFNC DllKernel UDWORD WINAPI WB_type_length(int type) 
/* For strings and binary returns maximal values! Values fixed by ODBC. */
{ switch (type)
  { case ATT_BOOLEAN:  return 1;
    case ATT_CHAR:     return 1;
    case ATT_INT16:    return 2;
    case ATT_INT32:    return 4;
    case ATT_INT8:     return 1;
    case ATT_INT64:    return 8;
    case ATT_MONEY:    return 6;
    case ATT_FLOAT:    return 8;
    case ATT_DATE:    case ATT_TIME:      case ATT_TIMESTAMP:  case ATT_DATIM:
      return 4;
    case ATT_PTR:     case ATT_BIPTR:
                       return 4;
    case ATT_AUTOR:    return OBJ_NAME_LEN;
    case ATT_TEXT:
    case ATT_RASTER:
      return 200000;//SQL_NO_TOTAL;
    case ATT_HIST:        case ATT_NOSPEC:   case ATT_SIGNAT:
      return (UDWORD)2000000;//SQL_NO_TOTAL;
    case ATT_STRING:  
      return MAX_FIXED_STRING_LENGTH;
    case ATT_BINARY:   return MAX_FIXED_STRING_LENGTH;
    case ATT_ODBC_TIME:       return sizeof(TIME_STRUCT);
    case ATT_ODBC_DATE:       return sizeof(DATE_STRUCT);
    case ATT_ODBC_TIMESTAMP:  return sizeof(TIMESTAMP_STRUCT);
  }
  return 0;  /* this is an error */
}

//////////////////////// string compare //////////////////////////////////////////
#define ORD_CH 88
#define ORD_ch 155

static const uns8 ordr[256] = {    /*8*/   /*a*/   /*c*/   /*e*/
0  ,1  ,2  ,3  ,4  ,5  ,6  ,7  ,8  ,9  ,10 ,11 ,12 ,13 ,14 ,15 ,    /*00*/
16 ,17 ,18 ,19 ,20 ,21 ,22 ,23 ,24 ,25 ,26 ,27 ,28 ,29 ,30 ,31 ,    /*10*/
32 ,33 ,34 ,35 ,36 ,37 ,38 ,39 ,40 ,41 ,42 ,43 ,44 ,45 ,46 ,47 ,    /*20*/
48 ,49 ,50 ,51 ,52 ,53 ,54 ,55 ,56 ,57 ,58 ,59 ,60 ,61 ,62 ,63 ,    /*30*/
64 ,66 ,72 ,73 ,77 ,80 ,85 ,86 ,87 ,89 ,92 ,93 ,94 ,98 ,99 ,102,    /*40*/
107,108,109,112,116,119,124,125,126,127,129,200,201,202,203,204,    /*50*/
65 ,133,139,140,144,147,152,153,154,156,159,160,161,165,166,169,    /*60*/
174,175,176,179,183,186,191,192,193,194,196,205,206,207,208,209,    /*70*/
210,211,212,213,214,215,216,217,218,219,114,220,113,117,132,130,    /*80*/
221,222,223,224,225,226,227,227,228,229,181,230,180,184,199,197,    /*90*/
231,232,232,97 ,233,71 ,234,235,236,237,115,238,239,240,241,131,    /*a0*/
242,243,244,164,245,246,247,248,249,138,182,250,95 ,251,162,198,    /*b0*/
110,67 ,68 ,69 ,70 ,96 ,74 ,75 ,76 ,81 ,82 ,84 ,83 ,90 ,91 ,78 ,    /*c0*/
79 ,100,101,103,104,105,106,252,111,121,120,122,123,128,118,253,    /*d0*/
177,134,135,136,137,163,141,142,143,148,149,151,150,157,158,145,    /*e0*/
146,167,168,170,171,172,173,254,178,188,187,189,190,195,185,255     /*f0*/
};

static uns8 grp[256] = {
0  ,1  ,2  ,3  ,4  ,5  ,6  ,7  ,8  ,9  ,10 ,11 ,12 ,13 ,14 ,15 ,    /*00*/
16 ,17 ,18 ,19 ,20 ,21 ,22 ,23 ,24 ,25 ,26 ,27 ,28 ,29 ,30 ,31 ,    /*10*/
32 ,33 ,34 ,35 ,36 ,37 ,38 ,39 ,40 ,41 ,42 ,43 ,44 ,45 ,46 ,47 ,    /*20*/
48 ,49 ,50 ,51 ,52 ,53 ,54 ,55 ,56 ,57 ,58 ,59 ,60 ,61 ,62 ,63 ,    /*30*/
64 ,66 ,72 ,73 ,77 ,80 ,85 ,86 ,87 ,89 ,92 ,93 ,94 ,98 ,99 ,102,    /*40*/
107,108,109,112,116,119,124,125,126,127,129,200,201,202,203,204,    /*50*/
65 ,133,139,140,144,147,152,153,154,156,159,160,161,165,166,169,    /*60*/
174,175,176,179,183,186,191,192,193,194,196,205,206,207,208,209,    /*70*/
210,211,212,213,214,215,216,217,218,219,113,220,112,116,132,130,    /*80*/
221,222,223,224,225,226,227,227,228,229,180,230,179,183,199,197,    /*90*/
231,232,232,94 ,233,66 ,234,235,236,237,112,238,239,240,241,131,    /*a0*/
242,243,244,161,245,246,247,248,249,133,179,250,94 ,251,161,198,    /*b0*/
109,66 ,66 ,66 ,66 ,94 ,73 ,73 ,74 ,80 ,80 ,80 ,80 ,89 ,89 ,77 ,    /*c0*/
77 ,99 ,99 ,102,102,102,102,252,110,119,119,119,119,127,116,253,    /*d0*/
176,133,133,133,133,161,140,140,141,147,147,147,147,156,156,144,    /*e0*/
144,166,166,169,169,169,169,254,177,186,186,186,186,194,183,255     /*f0*/
};
// c^: 74,141;

uns8 letters[128] =
{0,0,0,0,0,0,0,0,0,0,1,0,1,1,1,1,
 0,0,0,0,0,0,0,0,0,0,1,0,1,1,1,1,
 0,0,0,1,0,1,0,0,0,0,1,0,0,0,0,1,
 0,0,0,0,0,0,0,0,0,1,1,0,1,0,1,1,
 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
 1,1,1,1,1,1,1,0,1,1,1,1,1,1,1,0,
 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
 1,1,1,1,1,1,1,0,1,1,1,1,1,1,1,0 };

CFNC DllKernel uns8 WINAPI cUPCASE(uns8 ch)   /* converts CS letters to upper-case */
{ return csconv[ch]; }

CFNC DllKernel BOOL WINAPI SYMCHAR(uns8 ch)
{ if (mUPLETTER(mcUPCASE(ch))) return TRUE;
  return (ch>='0') && (ch<='9');
}

CFNC DllKernel BOOL WINAPI UPLETTER(uns8 ch)
{ return (ch<128) ? ((ch>='A') && (ch<='Z') || (ch=='_')) :
                    letters[ch-128];
}

CFNC DllKernel void WINAPI bin2hex(char * dest, const uns8 * data, unsigned len)
{ unsigned bt;
  while (len--)
  { bt=*data >> 4;
    *(dest++) = bt >= 10 ? 'A'-10+bt : '0'+bt;
    bt=*data & 0xf;
    *(dest++) = bt >= 10 ? 'A'-10+bt : '0'+bt;
    data++;
  }
}

CFNC DllKernel void WINAPI make_apl_query(char * query, const char * tabname, const uns8 * uuid)
{ if (!stricmp(tabname, "OBJTAB"))
    strcpy(query, "select OBJ_NAME,CATEGORY,APL_UUID,DEFIN,FLAGS,FOLDER_NAME,LAST_MODIF from `");
  else
    strcpy(query, "select TAB_NAME,CATEGORY,APL_UUID,DEFIN,FLAGS,FOLDER_NAME,LAST_MODIF from `");
  strcat(query, tabname);
  strcat(query, "` where APL_UUID=X\'");
  int plen=(int)strlen(query);
  bin2hex(query+plen, uuid, UUID_SIZE);  plen+=2*UUID_SIZE;
  query[plen++]='\'';  query[plen]=0;
}

#ifdef LINUX
int comp_wide(const uns16 * ss1, const uns16 * ss2, int maxlength, bool ignore_case, wbcharset_t charset)
#define COMP_STEP 1024
{ wchar_t buf1[COMP_STEP+1], buf2[COMP_STEP+1];  // the LINUX wchar_t (32bit)!
  int len1, len2, res;
  do  // compare per partes: it is not 100% correct, but faster than allocating buffers for 4-byte representation
  { len1=0;
    while (len1<maxlength && len1<COMP_STEP && ss1[len1])
      { buf1[len1]=ss1[len1];  len1++; }
    buf1[len1]=0;
    len2=0;
    while (len2<maxlength && len2<COMP_STEP && ss2[len2])
      { buf2[len2]=ss2[len2];  len2++; }
    buf2[len2]=0;
    if (ignore_case)
      res = __wcscasecmp_l(buf1, buf2, charset.locales16());
    else
      res = __wcscoll_l(buf1, buf2, charset.locales16());
    if (len1<COMP_STEP || len2<COMP_STEP) return res;
    ss1+=COMP_STEP;  ss2+=COMP_STEP;  maxlength-=COMP_STEP;
  } while (true);
}
#else  // WINS
int comp_cp(const unsigned char * ss1, const unsigned char * ss2, int maxlength, bool ignore_case, wbcharset_t charset)
// Comparing strings in code pages not supported by Windows
#define COMP_STEP 1024
{ wuchar buf1[COMP_STEP+1], buf2[COMP_STEP+1];  // buffers for parts converted to Windows Unicode
  const wchar_t * unictab = charset.unic_table();
  int len1, len2, res;
  do  // compare per partes: it is not 100% correct, but faster than allocating buffers and converting all
  { len1=0;
    while (len1<maxlength && len1<COMP_STEP && ss1[len1])
      { buf1[len1]=unictab[ss1[len1]];  len1++; }
    buf1[len1]=0;
    len2=0;
    while (len2<maxlength && len2<COMP_STEP && ss2[len2])
      { buf2[len2]=unictab[ss2[len2]];  len2++; }
    buf2[len2]=0;
    res = CompareStringW(charset.lcid(), ignore_case ? NORM_IGNORECASE : 0, buf1, len1, buf2, len2);
    if (!res)
    { tobjname buf;
      strcpy(buf, charset);
      strcat(buf, "==");
      strcat(buf, charset);
      SET_ERR_MSG(GetCurrTaskPtr(), buf);
#ifdef SRVR
      request_error(GetCurrTaskPtr(), STRING_CONV_NOT_AVAIL);  // may be handled
#else
      client_error(GetCurrTaskPtr(), STRING_CONV_NOT_AVAIL); 
#endif
      res=0;
    }
    else res-=2;
    if (len1<COMP_STEP || len2<COMP_STEP) return res;
    ss1+=COMP_STEP;  ss2+=COMP_STEP;  maxlength-=COMP_STEP;
  } while (true);
}

#endif

#define CH_FOR_GRP 1
// Ch must be in a specific group, otherwise the comparison would continue after comparing CH to a character from the same group!
// Ch must not be in the group 0,  otherwise the comparison would continue after comparing CH to 0!!!

#include "baseconv.cpp"

CFNC DllKernel int WINAPI cmp_str(const char * ss1, const char * ss2, t_specif specif)
{ int res;
  if (!specif.charset)  // ASCII comparison
  { if (specif.wide_char)
    { if (*(uns16*)ss2==0xffff) return MAKELONG(-1, 0);  // ss2 is an unlimited stopkey
      if (specif.ignore_case) res=wucnicmp((wuchar*)ss1, (wuchar*)ss2, specif.length/2);
      else                    res=wucncmp ((wuchar*)ss1, (wuchar*)ss2, specif.length/2);
    }
    else
      if (specif.ignore_case) res=_strnicmp(ss1, ss2, specif.length);
      else                    res= strncmp (ss1, ss2, specif.length);
    return !res ? 0 : res>0 ? 1 : -1;
  }

  if (specif.charset & 0x80)  // ISO charset
  { if (specif.wide_char)
      return general_comp_strW((const wuchar *)ss1, (const wuchar *)ss2, specif.length, specif);
    else
      return general_comp_strA(ss1, ss2, specif.length, specif);
  }
 // WIN charsets:
  if (specif.charset==1 && !specif.wide_char)  // original Czech comparison
  { uns8 c1,c2,cc1,cc2;  const char * s1=ss1, * s2=ss2;
    int smalldif=0;  int maxlen=specif.length;
    while (maxlen--)
    { if ((*s1=='c') && ((s1[1]=='h')||(s1[1]=='H')) && maxlen)
        { c1=(uns8)(specif.ignore_case ? ORD_CH : ORD_ch);  s1+=2;  cc1=CH_FOR_GRP; }
      else if ((*s1=='C') && ((s1[1]=='h')||(s1[1]=='H')) && maxlen)
        { c1=ORD_CH;  s1+=2;  cc1=CH_FOR_GRP; }
      else
        { cc1=specif.ignore_case ? mcUPCASE((uns8)*s1) : *s1;  c1=ordr[(uns8)cc1];  s1++; }

      if ((*s2=='c') && ((s2[1]=='h')||(s2[1]=='H')) && maxlen)
        { c2=(uns8)(specif.ignore_case ? ORD_CH : ORD_ch);  s2+=2;  cc2=CH_FOR_GRP;  maxlen--; }
      else if ((*s2=='C') && ((s2[1]=='h')||(s2[1]=='H')) && maxlen)
        { c2=ORD_CH;  s2+=2;  cc2=CH_FOR_GRP;  maxlen--; }
      else
        { cc2=specif.ignore_case ? mcUPCASE((uns8)*s2) : *s2;  c2=ordr[(uns8)cc2];  s2++; }

      if (c1!=c2)
      { if (grp[cc1]!=grp[cc2])
          return /*c1==0xff && !c2 && smalldif ? smalldif : c2==0xff && !c1 && smalldif ? smalldif :*/ c1>c2 ? 1 : -1;
          // Zakomentovana cast delala problemy pri prefixovem vyberu Ostr�> Ostra\xff a melo by to byt naopak,
          // protoze jinak vyber slov s prefixem Ostra skonci na Ostr�a nenajde Ostrava.
        if (!smalldif) smalldif=c1>c2 ? 1 : -1;
      }
      if (!c1) break;
    }
    return smalldif;
  }
 // general comparison:
#ifdef WINS
  if (specif.wide_char)
  { if (*(uns16*)ss2==0xffff) return MAKELONG(-1, 0);  // ss2 is an unlimited stopkey
    int l1 = (int)wuclen((wuchar*)ss1);  if (l1>specif.length/2) l1=specif.length/2;
    int l2 = (int)wuclen((wuchar*)ss2);  if (l2>specif.length/2) l2=specif.length/2;
    res=CompareStringW(wbcharset_t(specif.charset).lcid(), specif.ignore_case ? NORM_IGNORECASE : 0, (wuchar*)ss1, l1, (wuchar*)ss2, l2);
  }
  else  // narrow characters
  { if (*(uns8*)ss2==0xff) return MAKELONG(-1, 0);  // ss2 is an unlimited stopkey
    if (wbcharset_t(specif.charset).is_iso())
      return comp_cp((const unsigned char *)ss1, (const unsigned char *)ss2, specif.length, specif.ignore_case, wbcharset_t(specif.charset));
    int l1 = (int)strlen(ss1);  if (l1>specif.length) l1=specif.length;
    int l2 = (int)strlen(ss2);  if (l2>specif.length) l2=specif.length;
    res=CompareStringA(wbcharset_t(specif.charset).lcid(), specif.ignore_case ? NORM_IGNORECASE : 0, ss1, l1, ss2, l2);
  }
  if (!res)
  { tobjname buf;
    strcpy(buf, wbcharset_t(specif.charset));
    strcat(buf, "==");
    strcat(buf, wbcharset_t(specif.charset));
    SET_ERR_MSG(GetCurrTaskPtr(), buf);
#ifdef SRVR
    request_error(GetCurrTaskPtr(), STRING_CONV_NOT_AVAIL);  // may be handled
#else
    client_error(GetCurrTaskPtr(), STRING_CONV_NOT_AVAIL);
#endif
    res=0;
  }
  else res-=2;
#else
  if (specif.wide_char)
  { if (*(uns16*)ss2==0xffff) return MAKELONG(-1, 0);  // ss2 is an unlimited stopkey
    int maxlength = specif.length / 2;
    return comp_wide((const uns16*)ss1, (const uns16*)ss2, maxlength, specif.ignore_case, wbcharset_t(specif.charset));
  }
  else  // narrow characters
  { if (*(uns8*)ss2==0xff) return MAKELONG(-1, 0);  // ss2 is an unlimited stopkey
    int maxlength = specif.length;
    if (specif.ignore_case)
      return __strncasecmp_l(ss1, ss2, maxlength, wbcharset_t(specif.charset).locales8());
    else  // case-sensitive
    { char * as1 = (char*)ss1, * as2 = (char*)ss2;  char c1, c2;
      int l1 = strlen(ss1);
      if (l1>maxlength) { c1=ss1[maxlength];  as1[maxlength]=0; }
      int l2 = strlen(ss2);
      if (l2>maxlength) { c2=ss2[maxlength];  as2[maxlength]=0; }
      res = __strcoll_l(ss1, ss2, wbcharset_t(specif.charset).locales8());
      if (l1>maxlength) as1[maxlength]=c1;
      if (l2>maxlength) as2[maxlength]=c2;
    }
  }
#endif
  return res;
}

CFNC DllKernel int WINAPI my_stricmp(const char * str1, const char * str2)
{ unsigned char c1, c2;
  do
  { c1=mcUPCASE((uns8)*str1);  c2=mcUPCASE((uns8)*str2);
    if (c1 != c2) return (c1 > c2) ? 1 : -1;
    if (!c1) return 0;
    str1++;  str2++;
  } while (TRUE);
}

#ifndef SRVR
CFNC DllKernel int WINAPI wb2_stricmp(int charset, const char * str1, const char * str2)
{
#ifdef WINS
  return cmp_str(str1, str2, t_specif(0xffff, charset, true, false));
#else
  if (charset==1) // implemented internally for the case that cp1250 is not installed
    return cmp_str(str1, str2, t_specif(0xffff, 1, TRUE, FALSE));  
  return __strncasecmp_l(str1, str2, 0x7FFFFFFF, wbcharset_t(charset).locales8());
#endif
}

CFNC DllKernel int WINAPI wb_stricmp(cdp_t cdp, const char * str1, const char * str2)
{
  int charset = cdp ? cdp->sys_charset : 1;
  return wb2_stricmp(charset, str1, str2);
}

CFNC DllKernel void WINAPI wb_small_string(cdp_t cdp, char * s, bool captfirst)
{
  int charset = cdp ? cdp->sys_charset : 1;
  char * buf = (char*)corealloc(strlen(s)+1, 77);
  if (buf)
  { convert_to_lowercaseA(s, buf, (wbcharset_t)charset);
    if (captfirst)
      *buf = wbcharset_t(charset).case_table()[(uns8)*s];
	strcpy(s, buf);
	corefree(buf);
  }	  
}
#endif

byte tNONEREAL[8] = { 0x89, 0xcb, 0xe6, 0xd8, 0x45, 0x43, 0xee, 0xff };
char NULLSTRING[2] = "\0";  // used for wide-character strings too
/////////////////////////// ODBC driver & catalog support ///////////////////////////

#ifndef SQL_TYPE_DATE   // NLM with old header files, move it to wbdef.h (is in swbcdp.h)
#define SQL_TYPE_DATE      91
#define SQL_TYPE_TIME      92
#define SQL_TYPE_TIMESTAMP 93

#define SQL_DATETIME        9
#define SQL_CODE_DATE       1
#define SQL_CODE_TIME       2
#define SQL_CODE_TIMESTAMP  3

#define SQL_PRED_NONE     0
#define SQL_PRED_BASIC    2
#endif

SWORD type_sql_to_WB(int type)
{ switch (type)
  { case SQL_BIT:      return ATT_BOOLEAN;
    case SQL_CHAR:     return ATT_CHAR;
    case SQL_SMALLINT: return ATT_INT16;
    case SQL_INTEGER:  return ATT_INT32;
    case SQL_TINYINT:  return ATT_INT8;
    case SQL_BIGINT:   return ATT_INT64;
    case SQL_NUMERIC:  return ATT_MONEY;
    case SQL_DOUBLE:   return ATT_FLOAT;
    case SQL_VARCHAR:  return ATT_STRING;
    case SQL_BINARY:   return ATT_BINARY;
    case SQL_DATE:     return ATT_DATE;
    case SQL_TIME:     return ATT_TIME;
    case SQL_TIMESTAMP:return ATT_TIMESTAMP;
    case SQL_TYPE_DATE:     return ATT_DATE;
    case SQL_TYPE_TIME:     return ATT_TIME;
    case SQL_TYPE_TIMESTAMP:return ATT_TIMESTAMP;
    case SQL_VARBINARY:     return ATT_RASTER;
    case SQL_LONGVARCHAR:   return ATT_TEXT;
    case SQL_LONGVARBINARY: return ATT_NOSPEC;
    case SQL_WCHAR:		 	    return ATT_CHAR;
    case SQL_WVARCHAR:	 	  return ATT_STRING;
    case SQL_WLONGVARCHAR:  return ATT_TEXT;
  }
  return 0;  /* this is an error */
}

UDWORD WB_type_precision(int type)
{ switch (type)
  { case ATT_INT16:    return 5;    // decimal
    case ATT_INT32:    return 10;   // decimal
    case ATT_INT8:     return 3;    // decimal
    case ATT_INT64:    return 19;   // decimal
    case ATT_MONEY:    return 15;   // decimal
    case ATT_FLOAT:    return 53;   // binary
    case ATT_TIME:     return 3;    // seconds
    case ATT_DATIM:   case ATT_TIMESTAMP:  return 0;  // seconds
  }
  return 0;  /* this is an error */
}

void type_WB_to_3(int WBtype, t_specif specif, SQLSMALLINT & type, SQLSMALLINT & concise, SQLSMALLINT & code, BOOL odbc2)
{ code=0;
  switch (WBtype)
  { case ATT_BOOLEAN:  type=concise=SQL_BIT;       break;
    case ATT_CHAR:     type = concise = specif.wide_char ? SQL_WCHAR : SQL_CHAR;   break;
    case ATT_INT16:    type=concise=SQL_SMALLINT;  break;
    case ATT_INT32:    type=concise=SQL_INTEGER;   break;
    case ATT_INT8:     type=concise=SQL_TINYINT;   break;
    case ATT_INT64:    type=concise=SQL_BIGINT;    break;
    case ATT_MONEY:    type=concise=SQL_NUMERIC;   break;
    case ATT_FLOAT:    type=concise=SQL_DOUBLE;    break;
    case ATT_STRING:   type = concise = specif.wide_char ? SQL_WVARCHAR : SQL_VARCHAR;   break;
    case ATT_BINARY:   type=concise=SQL_BINARY;    break;
    case ATT_DATE:     type=SQL_DATETIME;  
      concise=odbc2 ? SQL_DATE : SQL_TYPE_DATE;       
      code=SQL_CODE_DATE;       break;
    case ATT_TIME:     type=SQL_DATETIME;  
      concise=odbc2 ? SQL_TIME : SQL_TYPE_TIME;       
      code=SQL_CODE_TIME;       break;
    case ATT_DATIM:   case ATT_TIMESTAMP:  type=SQL_DATETIME;  
      concise=odbc2 ? SQL_TIMESTAMP : SQL_TYPE_TIMESTAMP;  
      code=SQL_CODE_TIMESTAMP;  break;
    case ATT_PTR:     case ATT_BIPTR:
                       type=concise=SQL_INTEGER;   break;
    case ATT_AUTOR:    type=concise=SQL_VARCHAR;   break;
    case ATT_TEXT:     type = concise = specif.wide_char ? SQL_WLONGVARCHAR : SQL_LONGVARCHAR;    break;
    case ATT_RASTER:   type=concise=SQL_VARBINARY;  break;
    case ATT_HIST:    case ATT_NOSPEC:  case ATT_SIGNAT:
                       type=concise=SQL_LONGVARBINARY;  break;
    default:           type=concise=0; /* error */ break;
  }
}

CFNC DllKernel SWORD WINAPI type_WB_to_sql(int WBtype, t_specif specif, BOOL odbc2)
{ SQLSMALLINT type, concise, code;
  type_WB_to_3(WBtype, specif, type, concise, code, odbc2);
  return concise;
}

int simple_column_size(int concise_type, t_specif specif)
// Precision or length, depending on type.
{ switch (concise_type)
  { case SQL_BIT:            return 1;
    case SQL_TINYINT:        return 3;
    case SQL_SMALLINT:       return 5;
    case SQL_INTEGER:        return 10;
    case SQL_BIGINT:         return 19;
    case SQL_REAL:           return 15;
    case SQL_FLOAT:          return 15;
    case SQL_DOUBLE:         return 15;
    case SQL_TYPE_DATE:      return 10;
    case SQL_TYPE_TIME:      return 12;
    case SQL_TYPE_TIMESTAMP: return 19;
    case SQL_DECIMAL:
    case SQL_NUMERIC:        return 15;
    case SQL_CHAR: 
    case SQL_WCHAR:          return 1;  // used only for converted WB types
    case SQL_VARCHAR:  
    case SQL_BINARY:         return specif.length;
    case SQL_WVARCHAR:       return specif.length/2;  // in chars!
    case SQL_VARBINARY:      return 200000;  // automated test needs something
    case SQL_LONGVARCHAR:  case SQL_WLONGVARCHAR:
    case SQL_LONGVARBINARY:  return 2000000;//SQL_NO_TOTAL;
  }
  return 0; // error
}

/////////////////////////////// encode - used by server mail //////////////////////////////
#define CODES_NUM       6

unsigned char inptabs[CODES_NUM-1][128] = { {    /* do EECS: Kamenicky, CSKOI 8, LATIN 2-602, LATIN 2-852, ANSI
 0   1   2   3   4   5   6   7   8   9   a   b   c   d   e   f        */
200,252,233,239,228,207,141,232,236,204,197,205,190,229,196,193,  /*80 Kam*/
201,158,142,244,246,211,249,218,253,214,220,138,188,221,216,157,  /*90*/
225,237,243,250,242,210,217,212,154,248,224,192, 63,167,171,187,  /*a0*/
 63,199,250,124, 43,223, 63, 65, 79,195, 97,111,227, 63, 63, 43,  /*b0*/
 43, 43, 43, 43, 45, 43,164,243,237, 80, 33,210, 63,242,233, 63,  /*c0*/
 63, 63, 63, 63, 63, 63, 63, 63,225, 43, 43, 42, 42, 63, 63, 42,  /*d0*/
 63,223, 63, 63, 63, 63,181, 63, 63, 63, 63, 63, 63, 63, 63, 63,  /*e0*/
 63,177, 63, 63, 63, 63,247, 63,176,193, 63, 63, 63, 50, 32, 63 }, {

199,250,233,167,223, 63, 65, 79,195, 97,111,227, 63, 63,164,243,  /*80 KOI*/
237, 80, 33,210, 63,242, 42, 42, 42, 63, 63, 63, 63, 63, 63, 63,  /*90*/
 63,177, 63, 63, 63, 63,247, 63,176, 63, 50, 63, 63,167,171,187,  /*a0*/
 63,223, 63, 63, 63, 63,181, 63, 63, 63, 63, 63, 63, 63, 63, 63,  /*b0*/
 43,225, 43,232,239,236,224, 45,252,237,249,229,190,246,242,243,  /*c0*/
244,228,248,154,157,250, 43,233,225,253,158, 43, 63, 63, 63, 43,  /*d0*/
 43,193, 43,200,207,204,192,124,220,205,217,197,188,214,210,211,  /*e0*/
212,196,216,138,141,218, 43,201,193,221,142, 43, 63, 63, 32, 63 }, {

199,252,233,250,228,249,233, 63, 65, 79,195, 97,111, 63,196,227,  /*80 Lat 602*/
201,197,229,244,246,188,190,164,243,214,220,141,157,237, 80,232,  /*90*/
225,237,243,250, 33,210,142,158, 63,242, 63, 63,200,167,171,187,  /*a0*/
 63, 63, 63,124, 43,193, 63,204, 63, 63, 63, 63, 63,177, 63, 43,  /*b0*/
 43, 43, 43, 43, 45, 43, 63, 63, 63, 63,176, 63, 50, 63, 63,164,  /*c0*/
 63,223,207, 63,239,210,205, 63,236, 43, 43, 42, 42,181,217, 42,  /*d0*/
211,223,212, 63, 63,242,138,154,192,218,224, 63,253,221, 63, 63,  /*e0*/
 63, 63, 63, 63, 63,167,247,225, 63, 63,193, 63,216,248, 63, 63 }, {

199,252,233,226,228,249,230,231,179,235,213,245,238,143,196,198,  /*80 Lat 852*/
201,197,229,244,246,188,190,140,156,214,220,141,157,163,215,232,  /*90*/
225,237,243,250,165,185,142,158,202,234, 63,159,200,186,171,187,  /*a0*/
 63, 63, 63,124, 43,193,194,204,170, 43,124, 43, 43,175,191, 43,  /*b0*/
 43, 43, 43, 43, 45, 43,195,227, 43, 43, 43, 43, 43, 45, 43,164,  /*c0*/
240,208,207,203,239,210,205,206,236, 43, 43, 42, 42,222,217, 42,  /*d0*/
211,223,212,209,241,242,138,154,192,218,224,219,253,221,254, 44,  /*e0*/
 45,189,178,161,162,167,247,184,176,168,255,251,216,248, 42, 63 }, {

 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63,  /*80 ANSI*/
 62,145,146, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63,  /*90*/
160, 33, 99, 76,164, 89,166,167,168,169, 97,171,172,150,174, 63,  /*a0*/
176,177, 50, 51,180,181,182,183,184, 49,111,187, 63, 63, 63, 63,  /*b0*/
 65,193,194, 65,196, 65, 65,199, 69,201, 69,203, 73,205,206, 73,  /*c0*/
208, 78, 79,211,212, 79,214,215, 79, 85,218, 85,220,221, 63,223,  /*d0*/
 97,225,226, 97,228, 97, 97,231,101,233,101,235,105,237,238,105,  /*e0*/
240,110,111,243,244,111,246,247,111,117,250,117,252,253, 63,121 } };

unsigned char outtabs[CODES_NUM-1][128] = { { /* z EECS: Kamenicky, CSKOI 8, LATIN 2-602, LATIN 2-852, ANSI
 0   1   2   3   4   5   6   7   8   9   a   b   c   d   e   f        */
 63, 63, 39, 63, 34, 95, 43, 42, 63, 63,155, 60, 83,134,146, 90,  /*80 Kam*/
 63, 96, 39, 34, 34, 42, 45, 45, 63, 63,168, 62,115,159,145,122,  /*90*/
 32, 63, 63, 76,198, 65,124,173, 63, 67, 83,174, 63, 45, 82, 90,  /*a0*/
248,241, 63,108, 63,230, 63, 46, 63, 65,115,175,156, 63,140,122,  /*b0*/
171,143, 65, 65,142,138, 67,177,128,144, 69, 69,137,139, 73,133,  /*c0*/
 68, 78,165,149,167,153,153,120,158,166,151,154,154,157, 84,181,  /*d0*/
170,160, 97, 97,132,141, 99, 99,135,130,101,101,136,161,105,131,  /*e0*/
100,110,164,162,147,148,148,246,169,150,163,129,129,152,116, 63 }, {

 63, 63, 39, 63, 34, 95, 43, 42, 63, 63,243, 60, 83,244,250, 90,  /*80 KOI*/
 63, 96, 39, 34, 34, 42, 45, 45, 63, 63,211, 62,115,212,218,122,  /*90*/
 32, 63, 63, 76,142, 65,124,173, 63, 67, 83,174, 63, 45, 82, 90,  /*a0*/
168,161, 63,108, 63,182, 63, 46, 63, 65,115,175,236, 63,204,122,  /*b0*/
230,225, 65, 65,241,235, 67,128,227,247, 69, 69,229,233, 73,228,  /*c0*/
 68, 78,238,239,240,237,237,120,242,234,245,232,232,249, 84,177,  /*d0*/
198,193, 97, 97,209,203, 99, 99,195,215,101,101,197,201,105,196,  /*e0*/
100,110,206,207,208,205,205,166,210,202,213,200,200,217,116, 63 }, {

 63, 63, 39, 63, 34, 95, 43, 42, 63, 63,230, 60, 83,155,166, 90,  /*80 Lat 602*/
 63, 96, 39, 34, 34, 42, 45, 45, 63, 63,231, 62,115,156,167,122,  /*90*/
 32, 63, 63, 76,151, 65,124,173, 63, 67, 83,174, 63, 45, 82, 90,  /*a0*/
202,189, 63,108, 63,221, 63, 46, 63, 65,115,175,149, 63,150,122,  /*b0*/
232,181, 65, 65,142,145, 67,128,172,144, 69, 69,183,214, 73,210,  /*c0*/
 68, 78,213,224,226,153,153,120,252,222,233,154,154,237, 84,225,  /*d0*/
234,160, 97, 97,132,146, 99, 99,159,130,101,101,216,161,105,212,  /*e0*/
100,110,229,162,147,148,148,246,253,133,163,129,129,236,116, 63 }, {

 63, 63, 39, 63, 34, 95, 43, 42, 63, 63,230, 60,151,155,166,141,  /*80 Lat 852*/
 63, 96, 39, 34, 34,254, 45, 45, 63, 63,231, 62,152,156,167,171,  /*90*/
 32,243,244,157,207,164,124,245,249, 67,184,174, 63, 45, 82,189,  /*a0*/
248, 63,242,136,239, 63, 63, 63,247,165,173,175,149,241,150,190,  /*b0*/
232,181,182,198,142,145,143,128,172,144,168,211,183,214,215,210,  /*c0*/
209,227,213,224,226,138,153,158,252,222,233,235,154,237,221,225,  /*d0*/
234,160,131,199,132,146,134,135,159,130,169,137,216,161,140,212,  /*e0*/
208,228,229,162,147,139,148,246,253,133,163,251,129,236,238,250 }, {

 63, 63, 39, 63, 34, 95, 43, 42, 63, 63, 83, 60, 83, 84, 90, 90,  /*80 ANSI*/
 63,145,146, 34, 34, 42, 45, 45, 63, 63,115, 62,115,116,122,122,  /*90*/
160, 63, 63, 76,164, 65,166,167,168,169, 83,171,172, 45,174, 90,  /*a0*/
176,177, 63,108,180,181,182,183,184, 97,115,187, 76,168,108,122,  /*b0*/
 82,193,194, 65,196, 76, 67,199, 67,201, 69,203, 69,205,206, 68,  /*c0*/
208, 78, 78,211,212,214,214,215, 82, 85,218,220,220,221, 84,223,  /*d0*/
114,225,226, 97,228,108, 99,231, 99,233,101,235,101,237,238,100,  /*e0*/
240,110,110,243,244,246,246,247,114,117,250,252,252,253,116,176 } };

CFNC DllKernel void WINAPI encode(char * tx, unsigned size, BOOL inputing, int recode)
{ unsigned char * tab;  unsigned char c;
  if (!recode || (recode>CODES_NUM)) return;
  tab=inputing ? inptabs[recode-1] : outtabs[recode-1];
  while (size--)
  { c=*tx;
    if (c>=128)
      if ((c!=0x8d) || (tx[1]!=0x0a))
        *tx=tab[c-128];
    tx++;
  }
}

#define MAX_COLL_NAME 10
struct t_collation_names
{ char name[MAX_COLL_NAME+1];
  int charset;
  t_collation_names(const char * nameIn, int charsetIn) : charset(charsetIn)
    { strcpy(name, nameIn); }
};

const t_collation_names collation_names[] = 
// List of collation names terminated by an empty name.
// Preferred names are specified first!
// CSIString not supported here! Empty name (COLLATION Ignore_case) not supported here!
{ t_collation_names( "ANSI",      0       ),
  t_collation_names( "STRING",    0       ),
  t_collation_names( "CSSTRING",  1       ),
  t_collation_names( "CZ",        1       ),
  t_collation_names( "SK",        1       ),
  t_collation_names( "PL",        2       ),
  t_collation_names( "FR",        3       ),
  t_collation_names( "DE",        4       ),
  t_collation_names( "IT",        5       ),
  t_collation_names( "CZ_WIN",    1       ),
  t_collation_names( "SK_WIN",    1       ),
  t_collation_names( "PL_WIN",    2       ),
  t_collation_names( "FR_WIN",    3       ),
  t_collation_names( "DE_WIN",    4       ),
  t_collation_names( "IT_WIN",    5       ),
  t_collation_names( "CZ_ISO",    128+1   ),
  t_collation_names( "SK_ISO",    128+1   ),
  t_collation_names( "PL_ISO",    128+2   ),
  t_collation_names( "FR_ISO",    128+3   ),
  t_collation_names( "DE_ISO",    128+4   ),
  t_collation_names( "IT_ISO",    128+5   ),
  t_collation_names( "",          0       )
};

const char * get_collation_name(int charset)
{ for (int i=0;  collation_names[i].name[0];  i++)
    if (collation_names[i].charset==charset) return collation_names[i].name;
  return ""; // masking errors: charsets not supported in this version
}

int find_collation_name(const char * name)
{ for (int i=0;  collation_names[i].name[0];  i++)
    if (!stricmp(collation_names[i].name, name)) return collation_names[i].charset;
  return -1; // error
}

CFNC DllKernel BOOL WINAPI specname(cdp_t cdp, const char * p)
{ tobjname cpy;  strmaxcpy(cpy, p, sizeof(tobjname));
#ifdef SRVR
  sys_Upcase(cpy);
  if (check_atr_name(cpy)/* & (IDCL_SQL_KEY|IDCL_IPL_KEY)*/)  /* is an SQL or IPJ keyword - used by SQL and default view designer */
    return TRUE;
  if (!sys_Alpha(*p) && *p!='_') return TRUE;
  while (*(++p))
    if (!sys_Alphanum(*p)) return TRUE;
#else
  Upcase9(cdp, cpy);
  if (check_atr_name(cpy)/* & (IDCL_SQL_KEY|IDCL_IPL_KEY)*/)  /* is an SQL or IPJ keyword - used by SQL and default view designer */
    return TRUE;
  if (!is_AlphaA(*p, cdp->sys_charset) && *p!='_') return TRUE;
  while (*(++p))
    if (!is_AlphanumA(*p, cdp->sys_charset)) return TRUE;
#endif
  return FALSE;
}

CFNC DllKernel void WINAPI copy_name(cdp_t cdp, char * dest, const char * name)
{ if (*name!='`' && specname(cdp, name))
  { *dest='`';  dest++;
    strcpy(dest, name);
    dest+=strlen(dest);
    *dest='`';  dest[1]=0;
  }
  else strcpy(dest, name);
}

/////////////////////////// date & timestamp ///////////////////////////////
unsigned c_year(unsigned year)
{ if (year % 4) return 365;
  if (year % 100) return 366;
  return (year % 400) ? 365 : 366;
}

unsigned c_month(unsigned month, unsigned year)
{ if (month==4 || month==6 || month==9 || month==11) return 30;
  if (month!=2) return 31;
  return c_year(year)-337;
}

CFNC DllKernel uns32 dt_plus(uns32 dt, sig32 diff)
{ if (diff<0) return dt_minus(dt, -diff);
  int day=Day(dt),  month=Month(dt), year=Year(dt), left;
  left=c_month(month, year)-day;
  if (diff <= left) day+=diff;  // days added only
  else
  { diff-=left+1;  day=1;  if (++month==13) { month=1;  year++; } // month boundary
    do
    { left=c_month(month, year);
      if (diff < left) break;
      diff-=left;
      if (++month==13)
        { month=1;  year++;  if (year >= 3000) return NONEDATE; }
    } while (TRUE);
    day+=diff;
  }
  return Make_date(day, month, year);
}

uns32 dt_minus(uns32 dt, sig32 diff)
{ if (diff<0) return dt_plus(dt, -diff);
  int day=Day(dt),  month=Month(dt), year=Year(dt), left;
  if (diff < day) day-=diff;  // days subtracted only
  else
  { diff-=day-1;  day=1;
    do
    { left=month==1 ? 31 : c_month(month-1, year);
      if (diff <= left) break;
      diff-=left;
      if (--month==0)
        { month=12;  year--;  if (!year) return NONEDATE; }
    } while (TRUE);
    day=left-diff+1;
    if (month==1) { year--;  month=12; } else month--;
  }
  return Make_date(day, month, year);
}

CFNC DllKernel sig32 dt_diff(uns32 dt1, uns32 dt2)
{ if (dt1<dt2) return -dt_diff(dt2, dt1);
  int day1=Day(dt1),  month1=Month(dt1), year1=Year(dt1), diff=0;
  int day2=Day(dt2),  month2=Month(dt2), year2=Year(dt2);
  if (year1==year2)
    if (month1==month2)
      return day1-day2;
    else
    { diff=day1 + c_month(month2, year2)-day2;
      while (++month2 < month1) diff+=c_month(month2, year2);
    }
  else
  { diff=day1 + c_month(month2, year2)-day2;
    while (++month2 <= 12) diff+=c_month(month2, year2);
    while (--month1 >= 1 ) diff+=c_month(month1, year1);
    while (++year2 < year1) diff+=c_year(year2);
  }
  return diff;
}

uns32 ts_plus(uns32 ts, sig32 diff)
{ if (diff<0) return ts_minus(ts, -diff);
  uns32 dt=timestamp2date(ts), tm=timestamp2time(ts);
  int dtdiff = diff / 86400,  tmdiff = diff % 86400;
  tm = tm / 1000 + tmdiff;
  if (tm >= 86400) { tm -= 86400;  dtdiff++; }
  return datetime2timestamp(dt_plus(dt, dtdiff), 1000*tm);
}

uns32 ts_minus(uns32 ts, sig32 diff)
{ if (diff<0) return ts_plus(ts, -diff);
  uns32 dt=timestamp2date(ts), tm=timestamp2time(ts);
  uns32 dtdiff = diff / 86400,  tmdiff = diff % 86400;
  tm = tm / 1000;
  if (tm < tmdiff) { tm += 86400-tmdiff;  dtdiff++; }
  else tm-=tmdiff;
  return datetime2timestamp(dt_minus(dt, dtdiff), 1000*tm);
}

sig32 ts_diff(uns32 ts1, uns32 ts2)
{ if (ts1 < ts2) return -ts_diff(ts2, ts1);
  uns32 dt1=timestamp2date(ts1), tm1=timestamp2time(ts1);
  uns32 dt2=timestamp2date(ts2), tm2=timestamp2time(ts2);
  return dt_diff(dt1, dt2) * 86400 + (int)(tm1-tm2)/1000;
}

#define TIME_OVER 86400000

uns32 tm_plus(uns32 tm, sig32 diff)
{ if (diff<0) return tm_minus(tm, -diff);
  diff %= TIME_OVER;
  if (tm+diff < tm) return tm+diff-TIME_OVER;
  return tm+diff;
}

uns32 tm_minus(uns32 tm, sig32 diff)
{ if (diff<0) return tm_plus(tm, -diff);
  diff %= TIME_OVER;
  if (tm<(uns32)diff) return tm+TIME_OVER-diff;
  return tm-diff;
}

sig32 tm_diff(uns32 tm1, uns32 tm2)
{ return tm1-tm2; }

/////////////////////////////////////// universal conversions /////////////////////////////////////////
#define MAX_SOURCE_STR_LEN 100
#ifdef SRVR
int client_tzd = 0;   // just a compilation support
#endif

bool x_str2date(const char * tx, uns32 * dt, const char * format)
// Converts string [tx] in [format] into date [dt]. Allows missing format items except for day and month.
// Returns true on success.
{ if (!format || !*format) format="DD.MM.CCYY";
  const char * f=format;  const char * t=tx;
  int day=0, month=0, year=0, century=0;
  bool was_century=false, was_year=false, was_day=false, was_month=false;
  while (*t && *(const unsigned char*)t<=' ') t++;
  if (!*t) *dt=NONEDATE;
  else
  { while (*f)
    { while (*t && *(const unsigned char*)t<=' ') t++;
      char c = *f & 0xdf;
      if (c=='D' || c=='M' || c=='Y' || c=='C')
      { int i=1;
        while ((f[i] & 0xdf) == c) i++;
        char cc=(f[i] & 0xdf);
        bool sep = cc!='D' && cc!='M' && cc!='Y' && cc!='C';
       // read max i digits:
        int val=0, j=0;
        while ((j<i || sep) && t[j]>='0' && t[j]<='9') val=10*val+t[j++]-'0';
        if      (c=='Y') { year=val;     if (j) was_year=true;    }
        else if (c=='M') { month=val;    if (j) was_month=true;   }
        else if (c=='D') { day=val;      if (j) was_day=true;     }
        else             { century=val;  if (j) was_century=true; }
        f+=i;  t+=j;
      }
      else
      { if (*t==*f) t++;
        f++;  // allows missing format items
      }
    }
    if (!was_month || !was_day) return false; // month and day must not be missing
    if (!was_century && year<100) century = Year(Today()) / 100;
    if (!was_year) year = Year(Today()) % 100;
    while (*t && *(const unsigned char*)t<=' ') t++;  // garbage on the end
    if (*t) return false;
    *dt=Make_date(day, month, 100*century + year);
    if (*dt==NONEINTEGER) return false;  // invalid date values
  }
  return true;
}

bool x_str2time(const char * tx, uns32 * tm, const char * format, bool UTC)
// Converts string [tx] in [format] into time [tm]. Allows missing format items except for hours and minutes.
// Returns true on success.
{ if (!format || !*format) format="HH:MM:SS.FFF";
  const char * f=format;  const char * t=tx;
  int hours=0, minutes=0, seconds=0, sec1000=0;
  bool hours_specified=false, minutes_specified=false;
  while (*t && *(const unsigned char*)t<=' ') t++;
  if (!*t) *tm=NONETIME;
  else
  { while (*f)
    { char c = *f & 0xdf;
      if (c=='H' || c=='M' || c=='S' || c=='F')
      { int i=1;
        while ((f[i] & 0xdf) == c) i++;
        char cc=(f[i] & 0xdf);
        bool sep = cc!='H' && cc!='M' && cc!='S' && cc!='F';
       // read max i digits:
        int val=0, j=0;
        while ((j<i || sep) && t[j]>='0' && t[j]<='9') val=10*val+t[j++]-'0';
        if      (c=='H') { hours  =val;  if (j) hours_specified  =true; }
        else if (c=='M') { minutes=val;  if (j) minutes_specified=true; }
        else if (c=='S')   seconds=val;
        else
        { sec1000=val;  int k;
          for (k=3;  k<j;  k++) sec1000/=10;
          for (k=j;  k<3;  k++) sec1000*=10;
        }
        f+=i;  t+=j;
      }
      else
      { if (*t==*f) t++;
        f++;  // allows missing format items
      }
    }
    if (!hours_specified || !minutes_specified) return false; // month and day must not be missing
   // compose the time value:
    uns32 val = Make_time(hours, minutes, seconds, sec1000);
    if (val==NONEINTEGER) return false;  // invalid time values
   // read displacement (not covered by the format):
    int displ;
    if (read_displ(&t, &displ)) 
    { val=displace_time(val, -displ);  // -> UTC
      if (!UTC) val=displace_time(val,  client_tzd);  // UTC -> ZULU
    }
    else 
      if ( UTC) val=displace_time(val, -client_tzd);  // ZULU -> UTC
   // check the end:
    while (*t && *(const unsigned char*)t<=' ') t++;  // garbage on the end
    if (*t) return false;
    *tm = val;
  }
  return true;
}

bool x_str2timestamp(const char * tx, uns32 * dtm, const char * format, bool UTC)
// Converts string [tx] in [format] into timestamp [dt]. Allows missing format items except for month, day, hours and minutes.
// Returns true on success.
{ if (!format || !*format) format="DD.MM.CCYY hh:mm:ss.fff";
  const char * f=format;  const char * t=tx;
  int hours=0, minutes=0, seconds=0, sec1000=0;
  bool hours_specified=false, minutes_specified=false;
  int day=0, month=0, year=0, century=0;
  bool was_century=false, was_year=false, was_day=false, was_month=false;
  while (*t && *(const unsigned char*)t<=' ') t++;
  if (!*t) *dtm=NONETIMESTAMP;
  else
  { while (*f)
    { char c = *f;
      if (c=='D' || c=='M' || c=='Y' || c=='C' || c=='h' || c=='m' || c=='s' || c=='f')
      { int i=1;
        while (f[i] == c) i++;
        char cc=f[i];
        bool sep = cc!='D' && cc!='M' && cc!='Y' && cc!='C' && cc!='h' && cc!='m' && cc!='s' && cc!='f';
       // read max i digits:
        int val=0, j=0;
        while ((j<i || sep) && t[j]>='0' && t[j]<='9') val=10*val+t[j++]-'0';
        if      (c=='Y') { year=val;     if (j) was_year=true;    }
        else if (c=='M') { month=val;    if (j) was_month=true;   }
        else if (c=='D') { day=val;      if (j) was_day=true;     }
        else if (c=='C') { century=val;  if (j) was_century=true; }
        else if (c=='h') { hours  =val;  if (j) hours_specified  =true; }
        else if (c=='m') { minutes=val;  if (j) minutes_specified=true; }
        else if (c=='s')   seconds=val;
        else
        { sec1000=val;  int k;
          for (k=3;  k<j;  k++) sec1000/=10;
          for (k=j;  k<3;  k++) sec1000*=10;
        }
        f+=i;  t+=j;
      }
      else
      { if (*t==*f) t++;
        f++;  // allows missing format items
      }
    }
    if (!was_month || !was_day /*|| !hours_specified || !minutes_specified*/) return false; // not be missing
   // compose the date value:
    if (!was_century && year<100) century = Year(Today()) / 100;
    if (!was_year) year = Year(Today()) % 100;
    uns32 _date = Make_date(day, month, 100*century + year);
    if (_date==NONEINTEGER) return false;  // invalid date values
   // compose the time value:
    uns32 _time = Make_time(hours, minutes, seconds, sec1000);
    if (_time==NONEINTEGER) return false;  // invalid time values
    uns32 ts = datetime2timestamp(_date, _time);
   // read displacement (not covered by the format):
    int displ;
    if (read_displ(&t, &displ)) 
    { ts=displace_timestamp(ts, -displ);  // -> UTC
      if (!UTC) ts=displace_timestamp(ts,  client_tzd);  // UTC -> ZULU
    }
    else 
      if ( UTC) ts=displace_timestamp(ts, -client_tzd);  // ZULU -> UTC
   // check the end:
    while (*t && *(const unsigned char*)t<=' ') t++;  // garbage on the end
    if (*t) return false;
    *dtm = ts;
  }
  return true;
}

int int2str0(int val, char * tx, unsigned len)
{ char buf[10];  int buflen, led;
  int2str(val, buf);  buflen=(int)strlen(buf);
  if (buflen < len)
    { led=len-buflen;  memset(tx, '0', led); }
  else led=0;
  strcpy(tx+led, buf);
  return led+buflen;
}

void x_date2str(uns32 dt, char * tx, const char * format)
{ if (!format || !*format) format="D.M.CCYY";
  const char * f=format;  char * t=tx;
  if (dt!=NONEDATE)
    while (*f)
    { char c = *f & 0xdf;
      if (c=='D' || c=='M' || c=='Y' || c=='C')
      { int i=1;
        while ((f[i] & 0xdf) == c) i++;
        if (c=='Y')
          t+=int2str0(i<=2 ? Year(dt) % 100 : Year(dt), t, i);
        else if (c=='C')
          t+=int2str0(Year(dt) / 100, t, i);
        else if (c=='M')
          t+=int2str0(Month(dt), t, i);
        else
          t+=int2str0(Day(dt), t, i);
        f+=i;
      }
      else *(t++)=*(f++);
    }
  *t=0;
}

void x_time2str(uns32 tm, char * tx, const char * format, bool UTC)
{ 
  if (tm==NONETIME) *tx=0;
  else
  { if (!format || !*format) format="H:MM:SS.FFF";
    const char * f=format;  char * t=tx;
    if (UTC)
      tm = displace_time(tm, client_tzd);
    while (*f)
    { char c = *f & 0xdf;
      if (c=='H' || c=='M' || c=='S' || c=='F')
      { int i=1;
        while ((f[i] & 0xdf) == c) i++;
        if (c=='H')
          t+=int2str0(Hours(tm), t, i);
        else if (c=='M')
          t+=int2str0(Minutes(tm), t, i);
        else if (c=='S')
          t+=int2str0(Seconds(tm), t, i);
        else // F
          if (i==1)
            t+=int2str0(Sec1000(tm)/100, t, 1);
          else if (i==2)
            t+=int2str0(Sec1000(tm)/10,  t, 2);
          else
          { t+=int2str0(Sec1000(tm),     t, 3);
            memset(t, '0', i-3);  t+=i-3;
          }
        f+=i;
      }
      else *(t++)=*(f++);
    }
    if (UTC) tzd2str(client_tzd, t);
    else     *t=0;
  }
}

void x_timestamp2str(uns32 dtm, char * tx, const char * format, bool UTC)
{ 
  if (dtm==NONETIMESTAMP) *tx=0;
  else
  { if (!format || !*format) format="D.M.CCYY h:mm:ss";
    const char * f=format;  char * t=tx;
    if (UTC)
      dtm = displace_timestamp(dtm, client_tzd);
    uns32 tm = timestamp2time(dtm);
    uns32 dt = timestamp2date(dtm);
    while (*f)
    { char c = *f;
      if (c=='D' || c=='M' || c=='Y' || c=='C' || c=='h' || c=='m' || c=='s' || c=='f')
      { int i=1;
        while (f[i] == c) i++;
        if (c=='Y')
          t+=int2str0(i==2 ? Year(dt) % 100 : Year(dt), t, i);
        else if (c=='C')
          t+=int2str0(Year(dt) / 100, t, i);
        else if (c=='M')
          t+=int2str0(Month(dt), t, i);
        else if (c=='D')
          t+=int2str0(Day(dt), t, i);
        else if (c=='h')
          t+=int2str0(Hours(tm), t, i);
        else if (c=='m')
          t+=int2str0(Minutes(tm), t, i);
        else if (c=='s')
          t+=int2str0(Seconds(tm), t, i);
        else // F
          if (i==1)
            t+=int2str0(Sec1000(tm)/100, t, 1);
          else if (i==2)
            t+=int2str0(Sec1000(tm)/10,  t, 2);
          else
          { t+=int2str0(Sec1000(tm),     t, 3);
            memset(t, '0', i-3);  t+=i-3;
          }
        f+=i;
      }
      else *(t++)=*(f++);
    }
    if (UTC) tzd2str(client_tzd, t);
    else     *t=0;
  }
}




#define MAX_BOOL_VAL_NAME 10

bool x_str2bool(const char * tx, uns8 * val, const char * format)
{ if (!format) format="";
  const char *fval = format, * tval=format;
  while (*tval && *tval!=',') tval++; 
  if (*tval) tval++;
  if (!*fval) fval="False";
  if (!*tval) tval="True";
  while (*tx && *(const unsigned char*)tx<=' ') tx++;
  if (!*tx) *val=NONEBOOLEAN;
  else
  { int i=0;  bool match = true;
    while (fval[i] && fval[i]!=',')
      if ((tx[i] & 0xdf) != (fval[i] & 0xdf)) { match=false;  break; }
      else i++;
    if (match) *val=0;
    else
    { i=0;  match = true;
      while (tval[i])
        if ((tx[i] & 0xdf) != (tval[i] & 0xdf)) { match=false;  break; }
        else i++;
      if (match) *val=1;
      else return false;
    }
   // check the end:
    while (*tx && *(const unsigned char*)tx<=' ') tx++;
    if (tx[i]) return false;
  }
  return true;
}

void x_bool2str(uns8 val, char * tx, const char * format)
{ if (val==NONEBOOLEAN) *tx=0;
  else
  { if (!format) format="";
    const char *fval = format, * tval=format;
    while (*tval && *tval!=',') tval++; 
    if (*tval) tval++;
    if (!*fval) fval="False";
    if (!*tval) tval="True";
    if (val) strcpy(tx, tval);
    else
    { while (*fval && *fval!=',')
        *(tx++)=*(fval++);
      *tx=0;
    }
  }
}

void conv_dec_sep(const char * tx, const char * format)
// Converts decimal separator specified on the beginning of [format] in [tx] into '.'.
{ if (format && *format && *format!='.')
  { char * pos = (char*)strchr(tx, *format);
    if (pos!=NULL) *pos='.';
  }
}

void create_dec_sep(const char * tx, const char * format)
// Converts '.' in [tx] into decimal separator specified on the beginning of [format]
{ if (format && *format && *format!='.')
  { char * pos = (char*)strchr(tx, '.');
    if (pos!=NULL) *pos=*format;
  }
}

bool x_str2real(const char * txt, double * val, const char * format)
{ char ths_sep = 0, dec_sep='.';
  while (*txt && *(const unsigned char*)txt<=' ') txt++;
  if (!*txt) { *val=NULLREAL;  return true; }
  if (format && *format)
  { dec_sep=*format;
    if (format[1] && (format[1]<'0' || format[1]>'9'))
      ths_sep=format[1];
  }
 // remove ths_sep, convert decimal_separator:
  char t[MAX_SOURCE_STR_LEN+1];  int i, j;
  i=j=0;
  while (txt[i])
  { if (txt[i]!=ths_sep)
    { 
#if 0

      if (txt[i]==dec_sep || txt[i]==',')
#ifdef WINS  // locale-independent decimal separator required by strtod
        t[j]='.';
#else        // locale-dependent decimal separator required by strtod
        t[j]=*decim_separ;  
#endif

#else  // new version, because strtod on WINS sometimes needs locale-dependent separator, too
      if (txt[i]==dec_sep || txt[i]=='.' || txt[i]==',')
        t[j]=*sysapi_decim_separ;
#endif
      else
        t[j]=txt[i];
      j++;
    }
    i++;
  }
  t[j]=0;
  double v;  char * endp;  
  v=strtod(t, &endp);
  while (*endp && *(const unsigned char*)endp<=' ') endp++;
  if (*endp) return false;
  *val=v;
  return true;
}

void x_real2str(double val, char * txt, const char * format)
{
  if (val==NULLREAL) *txt=0;
  else
  {// analyse the format: 
    char ths_sep = 0, dec_sep='.';  int decnum=3;  char tp = 'f';
    if (format && *format)
    { dec_sep=*format;
      const char * f = format+1;
      if (*f && (*f<'0' || *f>'9'))
        ths_sep=*(f++);
      if (*f>='0' && *f<='9')
      { decnum=0;
        while (*f>='0' && *f<='9')
        { decnum=10*decnum+*f-'0';
          f++;
        }
      }
      if (*f=='e' || *f=='E') tp='e';
    }
   // convert:
    { char form[20];
      *form='%';  form[1]='.';
      int2str(decnum, form+2);
      int len=(int)strlen(form);
      if (val > 1e100 || val < -1e100) tp='e';  /* otherwise may overflow */
      form[len]=tp;   
      form[len+1]=0;
      sprintf(txt, form, val);  // on Windows does not depend on locale!
    }
   // convert the decimal separator:
    load_separators();
#if 0  // originally #ifdef WINS  (sprintf uses '.' as the decimal separator - not generally true)
    char * pos = strchr(txt, '.');
#else  // separator is written according to locale, must find it
    char * pos = strchr(txt, *sysapi_decim_separ);
#endif
    if (pos) *pos=dec_sep;
   // add the thousands separator, if required:
    if (ths_sep)
      while (pos>txt+3)
      { pos-=3;
        if (pos[-1]<'0' || pos[-1]>'9') break;  /* leading sign */
        memmov(pos+1, pos, strlen(pos)+1);
        *pos=ths_sep;  
      }
  }
}

uns32 Make_timestamp(int day, int month, int year, int hour, int minute, int second)
{ if (day > 31) return (uns32)NONEINTEGER;
  year = (year < 1900) ? 0 : year - 1900;
  return second+60L*(minute+60*hour)+
         86400L*((day-1)+31L*((month-1)+12*year));
}

inline char hex(int i)
{ return i<10 ? '0'+i : 'A'-10+i; }

#ifdef WINS
#define MAXMONEY  ((sig64)0x7fffffffffff)
#else
#define MAXMONEY  0x7fffffffffffLL
#endif

bool conv1to1(const char * source, int slen, char * target, wbcharset_t scharset, wbcharset_t dcharset)
{ if (slen==-1) slen=(int)strlen(source);
#ifdef WINS
  if (slen)
  { int len, dlen=0;
    wchar_t * buf = (wchar_t *)malloc(sizeof(wchar_t)*slen);
    if (!buf) return false;
    if (!(len=MultiByteToWideChar(scharset.cp(), 0, source, slen, buf, slen)) ||
        !(dlen=WideCharToMultiByte(dcharset.cp(), 0, (wuchar*)buf, len, target, 2*len, NULL, NULL)))
      { free(buf);  GetLastError();  return false; }
    free(buf);
    target+=dlen;
  }
#else
  char * from=(char*)source;
  char * to=target;
  size_t fromlen=slen;
  size_t tolen=2*slen;
  iconv_t conv=iconv_open(dcharset, scharset);
  if (conv==(iconv_t)-1) return false;
  while (fromlen>0)
  { int nconv=iconv(conv, &from, &fromlen, &to, &tolen);
    if (nconv==(size_t)-1)
      { iconv_close(conv);  return false; }
  }
  iconv_close(conv);
  /* TODO: handler prezivajici mezi konverzemi? */
  target=to;
#endif
  *target=0;
  return true;
}

CFNC DllKernel int WINAPI conv1to2(const char * source, int slen, wuchar * target, wbcharset_t charset, int dbytelen)
{ int retcode = 0; // OK result
  if (slen==-1) slen=(int)strlen(source);
#ifdef WINS
  int dlen=0;
  if (slen && !(dlen=MultiByteToWideChar(charset.cp(), 0, source, slen, target, dbytelen ? dbytelen/2 : slen)))
  { if (GetLastError() == ERROR_INSUFFICIENT_BUFFER && dbytelen>=2)
    { do  // converting a maximal prefix:
        if (slen>1000) slen=slen/5*4;  else slen--;  // is faster for long strings than slen--;
      while (slen && !(dlen=MultiByteToWideChar(charset.cp(), 0, source, slen, target, dbytelen/2)));
      target+=dlen;
      retcode=SC_STRING_TRIMMED;
    }
    else
      retcode=SC_INCONVERTIBLE_INPUT_STRING;
  }
  else
    target+=dlen;
#else
  char * from=(char*)source;
  wuchar * to=target;
  size_t fromlen=slen;
  size_t tolen=dbytelen ? dbytelen : 2*slen;
  iconv_t conv=iconv_open("UCS-2", charset);
  if (conv==(iconv_t)-1) retcode=SC_INCONVERTIBLE_INPUT_STRING;
  else
  { while (fromlen>0)
    { if (tolen<2)
        { retcode=SC_STRING_TRIMMED;  break; }
      int nconv=iconv(conv, &from, &fromlen, (char **)&to, &tolen);
      if (nconv==(size_t)-1)  
        { retcode = errno==E2BIG ? SC_STRING_TRIMMED : SC_INCONVERTIBLE_INPUT_STRING;  break; }
    }
    iconv_close(conv);
    /* TODO: handler prezivajici mezi konverzemi? */
    target=to;
  }  
#endif
  *target=0;  // terminates the converted string or part
  return retcode;
}

CFNC DllKernel int WINAPI conv1to4(const char * source, int slen, uns32 * target, wbcharset_t charset, int dbytelen)
{ 
  int result = conv1to2(source, slen, (wuchar *)target, charset, dbytelen/2);
  if (result==SC_INCONVERTIBLE_INPUT_STRING)
    *target=0;  // 4-byte terminator
  else  // convert 2 to 4, including the terminator 
  { int i=(int)wuclen((wuchar*)target);
    while (i>=0)
    { target[i]=((wuchar*)target)[i];
      i--;
    }     
  }
  return result;
#ifdef STOP
  if (slen==-1) slen=strlen(source);
#ifdef WINS
  int dlen=0;
  if (slen && !(dlen=MultiByteToWideChar(charset.cp(), 0, source, slen, (wuchar*)target, slen)))
    return false;
 // convert UCS-2 to UCS-4:
  for (int i=dlen-1;  i>=0;  i--)
    target[i] = ((wuchar*)target)[i];
  target[dlen]=0;
#else
  char * from=(char*)source;
  uns32 * to=target;
  size_t fromlen=slen;
  size_t tolen=2*slen;
  iconv_t conv=iconv_open("UCS-2", charset);
  if (conv==(iconv_t)-1) return false;
  while (fromlen>0)
  { int nconv=iconv(conv, &from, &fromlen, (char **)&to, &tolen);
    if (nconv==(size_t)-1)
      { iconv_close(conv);  return false; }
  }
  iconv_close(conv);
  /* TODO: handler prezivajici mezi konverzemi? */
  int ind = 2*(to-target);
  target[ind]=0;
  while (--ind >= 0)
    target[ind] = ((wuchar*)target)[ind];
#endif
  return true;
#endif  
}

//#define XY sizeof(wchar_t)
//#if XY==4
//#error Je to 4
//#elif sizeof(wchar_t)==2
//#error Je to 2
//#else
//#error Jinak
//#endif


CFNC DllKernel int WINAPI conv4to1(const uns32 * source, int slen, char * target, wbcharset_t charset, int dbytelen)
// Always writes the terminator on the end. [slen] does not include the terminator.
{ int retcode = 0; // OK result
#ifdef WINS
  int dlen=0;
  if (slen==-1)  // no system function for UCS-4 length on MSW
  { slen=0;
    while (source[slen]) slen++;
  }
 // compress the source: 
  for (int i=0;  i<slen;  i++)
    ((wuchar*)source)[i]=source[i];
 // convert all or a prefix: 
  if (slen && !(dlen=WideCharToMultiByte(charset.cp(), 0, (wuchar*)source, slen, target, dbytelen ? dbytelen : 2*slen, NULL, NULL)))  // 2*slen for UTF-8
  { if (GetLastError() == ERROR_INSUFFICIENT_BUFFER && dbytelen>0)
    { do  // converting a maximal prefix:
        if (slen>1000) slen=slen/5*4;  else slen--;  // is faster for long strings than slen--;
      while (slen && !(dlen=WideCharToMultiByte(charset.cp(), 0, (wuchar*)source, slen, target, dbytelen, NULL, NULL)));
      target+=dlen;
      retcode=SC_STRING_TRIMMED;
    }
    else
      retcode=SC_INCONVERTIBLE_INPUT_STRING;
  }
  else
    target+=dlen;
 // restore the source:
  while (slen>0)
  { slen--;
    ((uns32*)source)[slen]=((wuchar*)source)[slen];
  }
#else  // temp: UCS-4 conversion does not work (but UTF-32 does)
  if (slen==-1) slen=wcslen((wchar_t*)source);
  const uns32 *from=source;
  char *to=target;
  size_t tolen=dbytelen ? dbytelen : 2*slen;  // space for the UTF-8 characters
  size_t fromlen=4*slen;
  iconv_t conv=iconv_open(charset, "UTF-32");
  if (conv==(iconv_t)-1) retcode=SC_INCONVERTIBLE_INPUT_STRING;
  else
  { while (fromlen>0)
    { if (!tolen)
        { retcode=SC_STRING_TRIMMED;  break; }
      int nconv=iconv(conv, (char **)&from, &fromlen, &to, &tolen);
      if (nconv==(size_t)-1)  
        { retcode = errno==E2BIG ? SC_STRING_TRIMMED : SC_INCONVERTIBLE_INPUT_STRING;  break; }
    }
    iconv_close(conv);
    /* TODO: handler prezivajici mezi konverzemi? */
    target=to;
  }  
#endif
  *target=0;
  return retcode;
}

CFNC DllKernel int WINAPI space_estimate(int stype, int dtype, int sbytes, t_specif sspec, t_specif dspec)
// Returns number of bytes needed for the converted value. Space for the string terminator is not included.
{
  int dchars, dbytes;
  switch (stype)
  { case ATT_STRING:  case ATT_TEXT:
      if      (sspec.wide_char==2) dchars=sbytes/4;
      else if (sspec.wide_char==1) dchars=sbytes/2;
      else                         dchars=sbytes;
      break;
    case ATT_BINARY:  case ATT_NOSPEC:  case ATT_RASTER:  case ATT_SIGNAT:  case ATT_HIST:
      dchars=2*sbytes;  break;
    default:
      dchars=30;  break;  // typical space for the string represention of fixed type values
  }
  switch (dtype)
  { case ATT_STRING:  case ATT_TEXT:
      if      (dspec.wide_char==2) dbytes=4*dchars;
      else if (dspec.wide_char==1) dbytes=2*dchars;
      else                         dbytes=2*dchars;  // for UTF-8, usually dbytes=dchars;
      break;
    case ATT_NOSPEC:  case ATT_RASTER:  case ATT_SIGNAT:  case ATT_HIST:
      dbytes=dchars/2;  break;
    case ATT_BINARY:  
      dbytes=dchars/2;  
      if (dbytes<dspec.length) dbytes=dspec.length;  // value will be padded by zeros
      break;
    default:
      dbytes=8;  break;  // small amount, up to 8 bytes for double and int64
  }
  return dbytes;

}

CFNC DllKernel int WINAPI superconv(int stype, int dtype, const void * sbuf, char * dbuf, t_specif sspec, t_specif dspec, const char * string_format)
// Converts data from [sbuf] in [stype] into [dbuf] in [dtype].
// [sspec] and [dspec] contain type-dependent information: scale for numeric types, wtz for date/timestamp, 
//   actual length for binary data (in opqval for BLOBs!), wide-info for strings (0,1,2 for 1, 2, 4 bytes).
// If one of [stype] and [dtype] is string, then [string_format] describes its format.
// Return value is negative for error (error number) length for binary output, 0 for success.
// The function supposes that [dbuf] is big enough for the result.
// ATT_BINARY target values are padded by 0s up to dspec.length.
// Input and output strings are null-terminated.
// When dspec.length == 0 then the output string length is not limited, when dspec.length>0 then is specifies the output buffer length in bytes,
//  the terminator may by written above it. (implemented for 2->1 conversion so far)
{ int i;  
  if (stype==ATT_STRING || stype==ATT_TEXT || stype==ATT_CHAR || stype==ATT_ODBC_NUMERIC || stype==ATT_ODBC_DECIMAL)
  { if (dtype==ATT_STRING || dtype==ATT_TEXT || dtype==ATT_CHAR || dtype==ATT_ODBC_NUMERIC || dtype==ATT_ODBC_DECIMAL)
    { int convres;
      switch (sspec.wide_char)
      { case 0:
          switch (dspec.wide_char)
          { case 0:
              if (sspec.charset!=dspec.charset /*&& (sspec.charset==6 || sspec.charset==7 || dspec.charset==6 || dspec.charset==7)*/)  // I want to recode Win<->ISO
              { if (!conv1to1((const char *)sbuf, stype==ATT_CHAR ? 1 : -1, dbuf, sspec.charset, dspec.charset)) return SC_INCONVERTIBLE_INPUT_STRING;
              }
              else
                if (stype==ATT_CHAR) { *dbuf=*(char*)sbuf;  dbuf[1]=0; }
                else strcpy(dbuf, (const char*)sbuf);
              break;
            case 1:
              convres=conv1to2((const char *)sbuf, stype==ATT_CHAR ? 1 : -1, (wuchar*)dbuf, sspec.charset, dspec.length);
              return convres;
            case 2:
              convres=conv1to4((const char *)sbuf, stype==ATT_CHAR ? 1 : -1, (uns32 *)dbuf, sspec.charset, dspec.length);
              return convres;
          }
          break;
        case 1:
          switch (dspec.wide_char)
          { case 0:
              convres=conv2to1((const wuchar*)sbuf, stype==ATT_CHAR ? 1 : -1, dbuf, dspec.charset, dspec.length);
              return convres;
            case 1:  // just copy
              for (i=0;  stype==ATT_CHAR ? i<1 : ((uns16*)sbuf)[i];  i++)
                ((uns16*)dbuf)[i] = ((uns16*)sbuf)[i];
              ((uns16*)dbuf)[i] = 0;
              break;
            case 2:  // expand UCS-2 to UCS-4:
              for (i=0;  stype==ATT_CHAR ? i<1 : ((uns16*)sbuf)[i];  i++)
                ((uns32*)dbuf)[i] = ((uns16*)sbuf)[i];
              ((uns32*)dbuf)[i] = 0;
              break;
          }
          break;
        case 2:
          switch (dspec.wide_char)
          { case 0:
              convres=conv4to1((const uns32 *)sbuf, stype==ATT_CHAR ? 1 : -1, dbuf, dspec.charset, dspec.length);
              return convres;
            case 1:  // compress UCS-4 to UCS-2:
              for (i=0;  stype==ATT_CHAR ? i<1 : ((uns32*)sbuf)[i];  i++)
                ((uns16*)dbuf)[i] = ((uns32*)sbuf)[i];
              ((uns16*)dbuf)[i] = 0;
              break;
            case 2:  // just copy
              for (i=0;  stype==ATT_CHAR ? i<1 : ((uns32*)sbuf)[i];  i++)
                ((uns32*)dbuf)[i] = ((uns32*)sbuf)[i];
              ((uns32*)dbuf)[i] = 0;
              break;
          }
          break;
      }
    }
    else if (dtype==ATT_BINARY || IS_HEAP_TYPE(dtype))  // dtype cannot be ATT_TEXT, so is BLOB
    { if (stype==ATT_CHAR)  // may be usefull or may not
      { *(dbuf++) = *(const uns8 *)sbuf;
        i=1;
      }
     // convert character string to binary string, 2 characters -> 1 byte
      else if (sspec.wide_char==0)
      { const uns8 * s2 = (const uns8 *)sbuf;  i=0;  int hex1, hex2;
        while (*s2)
        { uns8 c = *s2;
          if (c!=' ')  // must skip spaces, at least on the end
          { if (c>='0' && c<='9') hex1=c-'0'; else if (c>='A' && c<='F') hex1=c-'A'+10; else if (c>='a' && c<='f') hex1=c-'a'+10;
            else return SC_INCONVERTIBLE_INPUT_STRING;
            s2++;
            c = *s2;
            if (c>='0' && c<='9') hex2=c-'0'; else if (c>='A' && c<='F') hex2=c-'A'+10; else if (c>='a' && c<='f') hex2=c-'a'+10;
            else return SC_INCONVERTIBLE_INPUT_STRING;
            *(dbuf++) = (hex1<<4) + hex2;  i++;
          }
          s2++;
        }
      }
      else if (sspec.wide_char==1)
      { const uns16 * s2 = (const uns16 *)sbuf;  i=0;  int hex1, hex2;
        while (*s2)
        { uns16 c = *s2;
          if (c!=' ')  // must skip spaces, at least on the end
          { if (c>='0' && c<='9') hex1=c-'0'; else if (c>='A' && c<='F') hex1=c-'A'+10; else if (c>='a' && c<='f') hex1=c-'a'+10;
            else return SC_INCONVERTIBLE_INPUT_STRING;
            s2++;
            c = *s2;
            if (c>='0' && c<='9') hex2=c-'0'; else if (c>='A' && c<='F') hex2=c-'A'+10; else if (c>='a' && c<='f') hex2=c-'a'+10;
            else return SC_INCONVERTIBLE_INPUT_STRING;
            *(dbuf++) = (hex1<<4) + hex2;  i++;
          }
          s2++;
        }
      }
      else //if (sspec.wide_char==2)
      { const uns32 * s2 = (const uns32 *)sbuf;  i=0;  int hex1, hex2;
        while (*s2)
        { uns32 c = *s2;
          if (c!=' ')  // must skip spaces, at least on the end
          { if (c>='0' && c<='9') hex1=c-'0'; else if (c>='A' && c<='F') hex1=c-'A'+10; else if (c>='a' && c<='f') hex1=c-'a'+10;
            else return SC_INCONVERTIBLE_INPUT_STRING;
            s2++;
            c = *s2;
            if (c>='0' && c<='9') hex2=c-'0'; else if (c>='A' && c<='F') hex2=c-'A'+10; else if (c>='a' && c<='f') hex2=c-'a'+10;
            else return SC_INCONVERTIBLE_INPUT_STRING;
            *(dbuf++) = (hex1<<4) + hex2;  i++;
          }
          s2++;
        }
      }
     // check the length for ATT_BINARY, padd the value by 0s:
      if (dtype==ATT_BINARY)
      { if (i > dspec.length) return SC_BINARY_VALUE_TOO_LONG;
        while (i < dspec.length)
          { *(dbuf++) = 0;  i++; }
      }
      return i;  // binary length returned
    }
    else  // non-binary non-string output
    { char source[MAX_SOURCE_STR_LEN+1];  const char * psource;
     // convert wide input into 8-bit input pointer by [psource]:
      if (stype==ATT_CHAR)
      { if (!sspec.wide_char)
          *source=*(const uns8  *)sbuf;
        else if (sspec.wide_char==1)
        { if (*(const uns16 *)sbuf > 255) return SC_INCONVERTIBLE_WIDE_CHARACTER;
          *source=(uns8)*(const uns16 *)sbuf;
        }
        else // if (sspec.wide_char==2)
        { if (*(const uns32 *)sbuf > 255) return SC_INCONVERTIBLE_WIDE_CHARACTER;
          *source=*(const uns32 *)sbuf;
        }
        source[1]=0;
        psource=source;
      }
      else  // is 0-terminated
        if (!sspec.wide_char)
          psource=(const char *)sbuf;
        else if (sspec.wide_char==1)
        { const uns16 * s2 = (const uns16 *)sbuf;  i=0;
          while (*s2==' ') s2++;
          while (*s2 && i<MAX_SOURCE_STR_LEN)
          { if (*s2 > 255) return SC_INCONVERTIBLE_WIDE_CHARACTER;
            source[i++] = (uns8)*(s2++);
          }
          source[i]=0;
          while (*s2==' ') s2++;
          if (*s2) return SC_INPUT_STRING_TOO_LONG;
          psource=source;
        }
        else  // UCS4
        { const uns32 * s2 = (const uns32 *)sbuf;  i=0;
          while (*s2==' ') s2++;
          while (*s2 && i<MAX_SOURCE_STR_LEN)
          { if (*s2 > 255) return SC_INCONVERTIBLE_WIDE_CHARACTER;
            source[i++] = *(s2++);
          }
          source[i]=0;
          while (*s2==' ') s2++;
          if (*s2) return SC_INPUT_STRING_TOO_LONG;
          psource=source;
        }
     // converting 8-bit string to a simple type, using format
      switch (dtype)
      { case ATT_DATE:
          if (!x_str2date(psource, (uns32*)dbuf, string_format)) return SC_INCONVERTIBLE_INPUT_STRING;
          break;
        case ATT_ODBC_DATE:
        { uns32 dt;
          if (!x_str2date(psource, &dt, string_format)) return SC_INCONVERTIBLE_INPUT_STRING;
          date2DATE(dt, (DATE_STRUCT*)dbuf);
          break;
        }
        case ATT_TIME:
          if (!x_str2time(psource, (uns32*)dbuf, string_format, dspec.with_time_zone!=0)) return SC_INCONVERTIBLE_INPUT_STRING;  
          break;
        case ATT_ODBC_TIME:
        { uns32 tm;
          if (!x_str2time(psource, &tm, string_format, false)) return SC_INCONVERTIBLE_INPUT_STRING;  
          time2TIME(tm, (TIME_STRUCT*)dbuf);
          break;
        }
        case ATT_TIMESTAMP:  case ATT_DATIM:
          if (!x_str2timestamp(psource, (uns32*)dbuf, string_format, dspec.with_time_zone!=0)) return SC_INCONVERTIBLE_INPUT_STRING;  
          break;
        case ATT_ODBC_TIMESTAMP:
        { uns32 ts;
          if (!x_str2timestamp(psource, &ts, string_format, dspec.with_time_zone!=0)) return SC_INCONVERTIBLE_INPUT_STRING;  
          datim2TIMESTAMP(ts, (TIMESTAMP_STRUCT*)dbuf);
          break;
        }
        case ATT_BOOLEAN:
          if (!x_str2bool(psource, (uns8*)dbuf, string_format)) return SC_INCONVERTIBLE_INPUT_STRING;
          break;
        case ATT_INT8:
        { sig32 aux32;
          while (*psource && *(const unsigned char*)psource<=' ') psource++;
          if (!*psource) { *(sig8*)dbuf=NONETINY;  return 0; }
          if (dspec.scale) conv_dec_sep(psource, string_format);
          if (!str2intspec(psource, &aux32, dspec)) return SC_INCONVERTIBLE_INPUT_STRING;
          if (aux32>127 || aux32<-127) return SC_VALUE_OUT_OF_RANGE;
          *(sig8*)dbuf=(sig8)aux32;  
          break;
        }
        case ATT_INDNUM:
        case ATT_INT16:
        { sig32 aux32;
          while (*psource && *(const unsigned char*)psource<=' ') psource++;
          if (!*psource) { *(sig16*)dbuf=NONESHORT;  return 0; }
          if (dspec.scale) conv_dec_sep(psource, string_format);
          if (!str2intspec(psource, &aux32, dspec)) return SC_INCONVERTIBLE_INPUT_STRING; 
          if (aux32>32767 || aux32<-32767) return SC_VALUE_OUT_OF_RANGE;
          *(sig16*)dbuf=(sig16)aux32;  
          break;
        }
        case ATT_PTR:       case ATT_BIPTR:    case ATT_VARLEN:
        case ATT_INT32:   
          if (dspec.scale) conv_dec_sep(psource, string_format);
          if (!str2intspec  (psource, (sig32 *)dbuf, dspec)) return SC_INCONVERTIBLE_INPUT_STRING;
          break;
        case ATT_INT64:   
          if (dspec.scale) conv_dec_sep(psource, string_format);
          if (!str2int64spec(psource, (sig64 *)dbuf, dspec)) return SC_INCONVERTIBLE_INPUT_STRING;
          break;
        case ATT_FLOAT:   
          if (!x_str2real(psource, (double*)dbuf, string_format)) return SC_INCONVERTIBLE_INPUT_STRING;
          break;
        case ATT_MONEY:   
          conv_dec_sep(psource, string_format);
          if (!str2money(psource, (monstr*)dbuf)) return SC_INCONVERTIBLE_INPUT_STRING;
          break;
        default:
          return SC_BAD_TYPE;
      } // type switch
      return 0;
    }
  }
  else // stype in not a character string
  { switch (dtype)
    { case ATT_STRING:  case ATT_TEXT:  case ATT_ODBC_NUMERIC:  case ATT_ODBC_DECIMAL:
      {// first, convert to a 8-bit string: 
        switch (stype)
        { case ATT_DATE:
            x_date2str(*(uns32*)sbuf, dbuf, string_format);
            break;
          case ATT_ODBC_DATE:
          { DATE_STRUCT * dtst = (DATE_STRUCT *)sbuf;
            x_date2str(Make_date(dtst->day, dtst->month, dtst->year), dbuf, string_format);
            break;
          }
          case ATT_TIME:
            x_time2str(*(uns32*)sbuf, dbuf, string_format, sspec.with_time_zone!=0);
            break;
          case ATT_ODBC_TIME:
          { TIME_STRUCT * tmst = (TIME_STRUCT *)sbuf;
            x_time2str(Make_time(tmst->hour, tmst->minute, tmst->second, 0), dbuf, string_format, false);
            break;
          }
          case ATT_TIMESTAMP:  case ATT_DATIM:
            x_timestamp2str(*(uns32*)sbuf, dbuf, string_format, sspec.with_time_zone!=0);
            break;
          case ATT_ODBC_TIMESTAMP:
          { TIMESTAMP_STRUCT * dtmst = (TIMESTAMP_STRUCT *)sbuf;
            x_timestamp2str(Make_timestamp(dtmst->day, dtmst->month, dtmst->year, dtmst->hour, dtmst->minute, dtmst->second), 
              dbuf, string_format, false);
            break;
          }
          case ATT_BOOLEAN:
            x_bool2str(*(uns8*)sbuf, dbuf, string_format);
            break;
          case ATT_INT8:     
            if (*(sig8*)sbuf==NONETINY) *dbuf=0; 
            else 
            { intspec2str(*(sig8*)sbuf, dbuf, sspec);  
              if (sspec.scale) create_dec_sep(dbuf, string_format);
            }
            break;
          case ATT_INT16:  case ATT_INDNUM:  
            if (*(sig16*)sbuf==NONESHORT) *dbuf=0; 
            else 
            { intspec2str(*(sig16*)sbuf, dbuf, sspec);  
              if (sspec.scale) create_dec_sep(dbuf, string_format);
            }
            break;
          case ATT_FLOAT:    
            x_real2str(*(double*)sbuf, dbuf, string_format);     
            break;
          case ATT_MONEY:    
            money2str((monstr*)sbuf, dbuf, 1); 
            create_dec_sep(dbuf, string_format);
            break;
          case ATT_INT32:  case ATT_PTR:      case ATT_BIPTR:      case ATT_VARLEN:  
            intspec2str(*(sig32*)sbuf, dbuf, sspec);    
            if (sspec.scale) create_dec_sep(dbuf, string_format);
            break;
          case ATT_INT64:    
            int64spec2str(*(sig64*)sbuf, dbuf, sspec);  
            if (sspec.scale) create_dec_sep(dbuf, string_format);
            break;

          case ATT_BINARY:   case ATT_NOSPEC:  case ATT_RASTER:  case ATT_HIST:  case ATT_SIGNAT:
          { int len = stype==ATT_BINARY ? sspec.length : sspec.opqval;
            for (i=0;  i<len;  i++)
              if (((uns8*)sbuf)[i]) break;
            if (i<len)  // non-NULL
            { for (i=0;  i<len;  i++)
              { dbuf[2*i  ]=hex(((uns8*)sbuf)[i] >> 4);
                dbuf[2*i+1]=hex(((uns8*)sbuf)[i] & 0xf);
              }
              dbuf[2*i]=0;
            }
            else  // NULL binary
              dbuf[0]=0;
            break;
          }
          default:
            return SC_BAD_TYPE;
        }
       // convert to a wide-string, if required:
        if (dspec.wide_char)
        { int len = (int)strlen(dbuf);
          if (dspec.wide_char==1)
          { uns16 * s2 = (uns16*)dbuf;
            for (i=len;  i>=0;  i--) s2[i]=dbuf[i];
          }
          else
          { uns32 * s2 = (uns32*)dbuf;
            for (i=len;  i>=0;  i--) s2[i]=dbuf[i];
          }
        }
        return 0;
      }
     // numeric conversions:
      case ATT_INT8:  case ATT_INT16:  case ATT_INT32:  case ATT_INT64: case ATT_MONEY: case ATT_FLOAT:
      { sig64 sval64;  double svald;  bool is_null;
       // read and scale:
        switch (stype)
        { case ATT_INT8:
            is_null = *(sig8 *)sbuf==NONETINY;
            sval64 = (sig64)*(sig8 *)sbuf;  
            break;
          case ATT_INT16:
            is_null = *(sig16*)sbuf==NONESHORT;
            sval64 = (sig64)*(sig16*)sbuf;  
            break;
          case ATT_INT32:
            is_null = *(sig32*)sbuf==NONEINTEGER;
            sval64 = (sig64)*(sig32*)sbuf;
            break;  
          case ATT_INT64:
            sval64 =        *(sig64*)sbuf;
            is_null = sval64==NONEBIGINT;   
            break;
          case ATT_MONEY:
            is_null = IS_NULLMONEY((monstr*)sbuf);
            sval64 = ((sig64)((monstr*)sbuf)->money_hi4) << 16 | ((monstr*)sbuf)->money_lo2;  
            break;
          case ATT_FLOAT:
            svald= *(double*)sbuf;
            is_null = svald == NONEREAL;
           // scale:
            if (!is_null && dtype!=ATT_FLOAT)  // scale it before converion to the integer type
            { scale_real(svald, dspec.scale);
              if (svald > MAXBIGINT || svald < -MAXBIGINT)
                return SC_VALUE_OUT_OF_RANGE;
              sval64 = (sig64)svald;
            }
            break;
          default:
            return SC_BAD_TYPE;
        }
       // scale:
        if (!is_null && stype!=ATT_FLOAT)
          if (dtype!=ATT_FLOAT)
          { if (!scale_int64(sval64, dspec.scale-sspec.scale))
              return SC_VALUE_OUT_OF_RANGE;
          }
          else  // dtype==ATT_FLOAT
          { svald=(double)sval64;
            scale_real(svald, -sspec.scale);
          }

       // test value and write:
        switch (dtype)
        { case ATT_INT8:  
            if (is_null) *(sig8 *)dbuf=NONETINY;
            else
            { if (sval64<-(sig8 )0x7f || sval64>(sig8 )0x7f) return SC_VALUE_OUT_OF_RANGE;
              *(sig8 *)dbuf = (sig8 )sval64;
            }
            break;
          case ATT_INT16:
            if (is_null) *(sig16*)dbuf=NONESHORT;
            else
            { if (sval64<-(sig16)0x7fff || sval64>(sig16)0x7fff) return SC_VALUE_OUT_OF_RANGE;
              *(sig16*)dbuf = (sig16)sval64;
            }
            break;
          case ATT_INT32:  
            if (is_null) *(sig32*)dbuf=NONEINTEGER;
            else
            { if (sval64<-(sig32)0x7fffffff || sval64>(sig32)0x7fffffff) return SC_VALUE_OUT_OF_RANGE;
              *(sig32*)dbuf = (sig32)sval64;
            }
            break;
          case ATT_MONEY:  
            if (is_null) 
              { ((monstr*)dbuf)->money_hi4=NONEINTEGER;  ((monstr*)dbuf)->money_lo2=0; }
            else
            { if (sval64<-(sig64)MAXMONEY || sval64>(sig64)MAXMONEY) return SC_VALUE_OUT_OF_RANGE;
              *(monstr*)dbuf = *(monstr*)&sval64;
            }
            break;
          case ATT_INT64:  
            if (is_null) *(sig64*)dbuf=NONEBIGINT;
            else *(sig64*)dbuf = sval64;
            break;
          case ATT_FLOAT:  
            if (is_null) *(double*)dbuf=NONEREAL;
            else *(double*)dbuf = svald;
            break;
        } // switch (dtype)
        break;
      }

      case ATT_DATE:    case ATT_TIME:    case ATT_TIMESTAMP:    case ATT_DATIM:
      case ATT_ODBC_DATE:      case ATT_ODBC_TIME:      case ATT_ODBC_TIMESTAMP:
      { int day, month, year;  int hour, minute, second, fraction;
        bool is_null=false;
        day=month=year=hour=minute=second=fraction=0;
       // analysis:
        switch (stype)
        { case ATT_DATIM:  case ATT_TIMESTAMP:
          { uns32 dtm = *(uns32*)sbuf;
            if (dtm==(uns32)NONEINTEGER) is_null=true;
            else
            { dtm /= 86400L;
              day   =(int)(dtm % 31 + 1);  dtm /= 31;
              month =(int)(dtm % 12 + 1);
              year  =(int)(dtm/12 + 1900);
              dtm = *(uns32*)sbuf % 86400L;
              second=(int)(dtm % 60);  dtm /= 60;
              minute=(int)(dtm % 60);  dtm /= 60;
              hour  =(int)dtm;
            }
            break;
          }
          case ATT_DATE:
          { uns32 dt=*(uns32*)sbuf;
            if (dt==NONEDATE) is_null=true;
            else
              { day=Day(dt);  month=Month(dt);  year=Year(dt); }
            break;
          }
          case ATT_TIME:
          { uns32 tm=*(uns32*)sbuf;
            if (tm==NONETIME) is_null=true;
            else
            { hour=Hours(tm);  minute=Minutes(tm);
              second=Seconds(tm);  fraction=Sec1000(tm);
            }
            break;
          }
          case ATT_ODBC_DATE:
          { DATE_STRUCT * dtst = (DATE_STRUCT *)sbuf;
            if (dtst->day==32) is_null=true;
            else
              { day=dtst->day;  month=dtst->month;  year=dtst->year; }
            break;
          }
          case ATT_ODBC_TIME:
          { TIME_STRUCT * tmst = (TIME_STRUCT *)sbuf;
            if (tmst->hour>=24) is_null=true;
            else
            { hour=tmst->hour;  minute=tmst->minute;
              second=tmst->second;
            }
            break;
          }
          case ATT_ODBC_TIMESTAMP:
          { TIMESTAMP_STRUCT * dtmst = (TIMESTAMP_STRUCT *)sbuf;
            if (dtmst->day>=32) is_null=true;
            else
            { day=dtmst->day;  month=dtmst->month;  year=dtmst->year;
              hour=dtmst->hour;  minute=dtmst->minute;
              second=dtmst->second;  fraction=dtmst->fraction;
            }
            break;
          }
          default: is_null=true;
        } // analysis
       // synthesis:
        switch (dtype)
        { case ATT_DATE:
            *(uns32*)dbuf=is_null ? NONEDATE : Make_date(day, month, year);  break;
          case ATT_TIME:
            *(uns32*)dbuf=is_null ? NONETIME : Make_time(hour, minute, second, fraction);  break;
          case ATT_TIMESTAMP:
          case ATT_DATIM:
            *(uns32*)dbuf=is_null ? NONEINTEGER :
              Make_timestamp(day, month, year, hour, minute, second);
            break;
          case ATT_ODBC_DATE:
          { DATE_STRUCT * dtst = (DATE_STRUCT *)dbuf;
            dtst->day=is_null ? 32 : day;
            dtst->month=month;
            dtst->year=year;
            break;
          }
          case ATT_ODBC_TIME:
          { TIME_STRUCT * tmst = (TIME_STRUCT *)dbuf;
            tmst->hour=is_null ? 25 : hour;
            tmst->minute=minute;
            tmst->second=second;
            break;
          }
          case ATT_ODBC_TIMESTAMP:
          { TIMESTAMP_STRUCT * dtmst = (TIMESTAMP_STRUCT *)dbuf;
            dtmst->day=is_null ? 32 : day;
            dtmst->month=month;
            dtmst->year=year;
            dtmst->hour=hour;
            dtmst->minute=minute;
            dtmst->second=second;
            dtmst->fraction=fraction;
            break;
          }
        }
        break;
      }

      case ATT_BOOLEAN:
        if (stype!=ATT_BOOLEAN) return SC_BAD_TYPE;
        *(uns8*)dbuf = *(uns8*)sbuf;
        break;

      case ATT_BINARY:   // will be padded with 0s
        switch (stype)
        { case ATT_BINARY:
            if (sspec.length > dspec.length) return SC_BINARY_VALUE_TOO_LONG;
            memcpy(dbuf, sbuf, sspec.length);
            memset(dbuf+sspec.length, 0, dspec.length-sspec.length);
            return dspec.length;
          case ATT_NOSPEC:  case ATT_RASTER:  case ATT_HIST:  case ATT_SIGNAT:
            if (sspec.opqval > dspec.length) return SC_BINARY_VALUE_TOO_LONG;
            memcpy(dbuf, sbuf, sspec.opqval);
            memset(dbuf+sspec.opqval, 0, dspec.length-sspec.opqval);
            return dspec.length;
          default:
            return SC_BAD_TYPE;
        }
      case ATT_NOSPEC:  case ATT_RASTER:  case ATT_HIST:  case ATT_SIGNAT:
        switch (stype)
        { case ATT_BINARY:
            memcpy(dbuf, sbuf, sspec.length);
            return sspec.length;
          case ATT_NOSPEC:  case ATT_RASTER:  case ATT_HIST:  case ATT_SIGNAT:
            memcpy(dbuf, sbuf, sspec.opqval);
            return sspec.opqval;
          default:
            return SC_BAD_TYPE;
        }

      default:
        return SC_BAD_TYPE;
    } // switch on dtype
  } // not from string
  return 0;
}

CFNC DllKernel BOOL charset_available(int charset)
{ return wbcharset_t(charset).charset_available(); }

CFNC DllKernel void prepare_charset_conversions(void)
{ 
#ifdef LINUX
  wbcharset_t::prepare_conversions();
#endif  
}
//
// Ponechava cislice a pismena bez diakritiky, pismena s diakritikou 
// konvertuje na pismena bez diakritiky, ostatni znaky na _
//
CFNC DllKernel const char * WINAPI ToASCII(char *Src, int charset)
{ const uns8 * tab = wbcharset_t(charset).ascii_table();
  for (char *p = Src; *p; p++) *p = tab[(unsigned char)*p];
  return Src;
}
//
// Ponechava znaky < 0x80, pismena s diakritikou 
// konvertuje na pismena bez diakritiky, ostatni znaky na _
//
CFNC DllKernel const char * WINAPI ToASCII7(char *Src, int charset)
{ const uns8 * tab = wbcharset_t(charset).ascii_table();
  for (char *p = Src; *p; p++) if ((unsigned char)*p >= 0x80) *p = tab[(unsigned char)*p];
  return Src;
}
// prefix

#ifdef LINUX
#include <prefix.c>
#endif

CFNC DllKernel void get_path_prefix(char * path)
// Returns the path to the directory where the product is installed, without PATH_SEPARATOR in the end
{ int len;
#ifdef WINS
  GetModuleFileName(NULL, path, MAX_PATH);
  len = (int)strlen(path);
  while (len && path[len-1]!=PATH_SEPARATOR) len--;
  if (len) len--;
#else
  const char * pref = PREFIX;  // problem on non-root accounts
  if (pref && strlen(pref) < MAX_PATH-30)
    strcpy(path, pref);
  else
    strcpy(path, "/usr"); 
  len = strlen(path);
  if (len && path[len-1]==PATH_SEPARATOR) len--;
  if (!strcmp(path+len-4, "/lib"))  // client library path is prefix/lib/WB_LINUX_SUBDIR_NAME
    len-=4;
#endif
  path[len]=0;
}

#if WBVERS<=810
CFNC DllKernel void WINAPI md5_digest(uns8 * input, unsigned len, uns8 * digest)     
{ EDGetDigest(0, input, len, digest); }
#endif

#ifdef UNIX
#if WBVERS>1000  // repacement of the "common" library
#include "uxprofile.c"
#endif
#endif
