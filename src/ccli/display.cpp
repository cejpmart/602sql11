#ifdef WINS
#include  <windows.h>
#else 
#include "winrepl.h"
#endif
#include <stdio.h>
#include <string.h>  // memset on Linux in wbkernel.h
// general client headers
#include "cdp.h"
#include "wbkernel.h"
// 602ccli headers
#include "options.h"
#include "wbcl.h"
#include "commands.h"
#pragma hdrstop

#define ATTRNAMELEN 31

int limit_displ_size = 40;  

BOOL WINAPI count_columns(char * attrname, uns8 attrtype, uns8 attrmult, union t_specif attrspecif, void * user_data)
{ 
  int * column_count = (int*)user_data;
  (*column_count)++;
  return TRUE;
}  

struct t_column_descr
{ char name[ATTRNAMELEN+1];
  int type;
  t_specif specif;
  bool disabled;
  int displ_size;
};  

BOOL WINAPI get_column_descr(char * attrname, uns8 attrtype, uns8 attrmult, union t_specif attrspecif, void * user_data)
{ 
  t_column_descr ** pcold = (t_column_descr **)user_data, * cold = *pcold;
  strcpy(cold->name, attrname);
  cold->type = attrtype;
  cold->specif = attrspecif;
  cold->disabled = attrmult!=1 || attrtype==ATT_PTR || attrtype==ATT_BIPTR || attrtype==ATT_SIGNAT ||
                                  attrtype==ATT_AUTOR || attrtype==ATT_DATIM || attrtype==ATT_HIST ||
                   !memcmp(attrname, "_W5_", 4);               
  (*pcold)++;
  return TRUE;
}  

#ifdef WINS
typedef wchar_t wuchar;
#define wuclen(wstr) wcslen(wstr)
#else
typedef uns16 wuchar;
CFNC DllExport size_t WINAPI wuclen(const wuchar * wstr);
#endif

int utf8_strlen(const char * str)
{ int chlen, len = 0;
  while (*str)
  { if      ((*(unsigned char*)str&0x80)==0   ) chlen=1;
    else if ((*(unsigned char*)str&0xe0)==0xc0) chlen=2;
    else if ((*(unsigned char*)str&0xf0)==0xe0) chlen=3;
    else if ((*(unsigned char*)str&0xf8)==0xf0) chlen=4;
    else if ((*(unsigned char*)str&0xfc)==0xf8) chlen=5;
    else if ((*(unsigned char*)str&0xfe)==0xfc) chlen=6;
    else break;  // error
    str+=chlen;  len++;
  }
  return len;
}

void get_text_column_value(cdp_t cdp, tcursnum curs, trecnum rec, tattrib atr, t_column_descr * cold,
                           char * buf, char * rdbuf)
{ t_specif dest_spec(0, CHARSET_NUM_UTF8, 0, 0);
  t_varcol_size rdlimit = limit_displ_size;
  if (cold->type>=ATT_FIRST_HEAPTYPE && cold->type<=ATT_LAST_HEAPTYPE && cold->type!=ATT_TEXT ||
      cold->type==ATT_BINARY) 
    rdlimit /=2;
  if (cold->type==ATT_TEXT || cold->type==ATT_STRING || cold->type==ATT_CHAR)
    if (cold->specif.wide_char) 
      rdlimit *=2;
  if (cold->type>=ATT_FIRST_HEAPTYPE && cold->type<=ATT_LAST_HEAPTYPE)
  { t_varcol_size rd; 
    if (cd_Read_var(cdp, curs, rec, atr, NOINDEX, 0, rdlimit, rdbuf, &rd))
      goto err;
    if (cold->type==ATT_TEXT)
    { rdbuf[rd]=rdbuf[rd+1]=0;
      if (superconv(ATT_TEXT, ATT_TEXT, rdbuf, buf, cold->specif, dest_spec, NULL) < 0)
        goto err;
    }  
    else  // BLOB
      superconv(ATT_NOSPEC, ATT_STRING, rdbuf, buf, t_specif(rd), dest_spec, NULL);  // bin->hex
  }
  else
  { if (cd_Read(cdp, curs, rec, atr, NULL, rdbuf)) 
      goto err;
    if (cold->type==ATT_BINARY && cold->specif.length > rdlimit)  
      cold->specif.length = rdlimit;
    if (cold->type==ATT_STRING)
      if (cold->specif.wide_char)
      { if ((int)wuclen((wuchar*)rdbuf) > limit_displ_size)
          rdbuf[rdlimit]=rdbuf[rdlimit+1]=0;  // rdlimit is in bytes here
      }    
      else
        if ((int)strlen(rdbuf)>limit_displ_size) 
          rdbuf[limit_displ_size]=0;
    const char * format = NULL;
    switch (cold->type)
    { case ATT_DATE: 
        format=date_format.c_str();  break;
      case ATT_TIME: 
        format=time_format.c_str();  break;
      case ATT_TIMESTAMP: 
        format=timestamp_format.c_str();  break;
      case ATT_FLOAT:
        format=real_format.c_str();  break;
    }
    if (superconv(cold->type, ATT_STRING, rdbuf, buf, cold->specif, dest_spec, format) < 0)
      goto err;
    if (cold->type!=ATT_STRING && cold->type!=ATT_CHAR)
      if (!*buf)
        strmaxcpy(buf, disp_null.c_str(), limit_displ_size+1);
  }    
  return;
 err:
  strmaxcpy(buf, "<Error>", limit_displ_size+1);
}

#define FILL_STEP 20

void fill_output(size_t count, char c)
{ char buf[FILL_STEP+1];
  memset(buf, c, FILL_STEP);
  while (count)
  { size_t step = count < FILL_STEP ? count : FILL_STEP;
    buf[step]=0;
    output_message(buf, true, false);
    count-=step;
  }  
}  

void write_delimiter(t_column_descr * coldescr, char * buf)
// Used for non-expanded format only
// called only for aligned format
{
  if (disp_border>=2)
    output_message("+-", true, false);
  bool first = true;  
  for (t_column_descr * cold = coldescr;  cold->name[0];  cold++)
    if (!cold->disabled)
    { if (!first)
        output_message("-+-", true, false);
      else  
        first=false;
      fill_output(cold->displ_size, '-');
    }  
  if (disp_border>=2)
    output_message("-+", true, false);  
  newline();
}

void display_cursor_contents(cdp_t cdp, tcursnum curs)
{ trecnum reccnt;
 // count records:
  if (cd_Rec_cnt(cdp, curs, &reccnt))
    { write_error_info(cdp, "Rec count", cd_Sz_error(cdp));  return; }
 // count the columns (DELETED column not included):
  int column_count = 0;
  if (!cd_Enum_attributes_ex(cdp, curs, count_columns, &column_count))
    return;
 // allocate the descriptor:
  t_column_descr * coldescr = (t_column_descr*)malloc(sizeof(t_column_descr)*(column_count+1));
  if (!coldescr) return;
 // create the description: 
  t_column_descr * cold = coldescr;
  cd_Enum_attributes_ex(cdp, curs, get_column_descr, &cold);
  cold->name[0]=0;
 // calculate display sizes: 
  int max_displ_size = 0, max_name_len = 0;  // used by expanded view
  if (disp_aligned || expanded)
    for (cold = coldescr;  cold->name[0];  cold++)
    { if (cold->disabled) cold->displ_size=0;
      else
      { switch (cold->type)
        { case ATT_BOOLEAN:
            cold->displ_size=5;  break;  // "false"
          case ATT_CHAR:
            cold->displ_size=1;  break;  // "a"
          case ATT_INT16:
            cold->displ_size=6;  break;  // "-32000"
          case ATT_INT32:
            cold->displ_size=11;  break;  // "-2000000000"
          case ATT_MONEY:
            cold->displ_size=12;  break;  // "-2000000.000"
          case ATT_FLOAT:  // size of the value in fixed format depends on the value, not only on format -> reasonable minimum must be specified
            { char buf[100+1];
              double rval = -123456789.123456789012345e-173;
              superconv(ATT_FLOAT, ATT_STRING, &rval, buf, cold->specif, t_specif(100,0,0,0), real_format.c_str());  
              cold->displ_size=(int)strlen(buf);
              if (cold->displ_size<20) cold->displ_size=20;
            }  
            break;
          case ATT_STRING:
            cold->displ_size=cold->specif.wide_char ? cold->specif.length/2 : cold->specif.length;  
            if (cold->displ_size>limit_displ_size) cold->displ_size=limit_displ_size;
            break;
          case ATT_BINARY:
            cold->displ_size=2*cold->specif.length;  
            if (cold->displ_size>limit_displ_size) cold->displ_size=limit_displ_size;
            break;  
          case ATT_DATE:
            if (date_format.empty())
              cold->displ_size=10;  // // "27.04.1962"
            else  // find the max length for the specified date_format
            { char buf[100+1];  uns32 dt;
              dt = Make_date(27, 12, 1956);
              superconv(ATT_DATE, ATT_STRING, &dt, buf, cold->specif, t_specif(100,0,0,0), date_format.c_str());
              cold->displ_size=(int)strlen(buf);
            }  
            break;  
          case ATT_TIME:
            if (time_format.empty())
            { cold->displ_size=12;  // "12:34:56.789"
              if (cold->specif.with_time_zone) cold->displ_size += 6;  // time zone information like '+05:00'
            }  
            else  // find the max length for the specified time_format
            { char buf[100+1];  uns32 tm;
              tm = Make_time(23, 34, 56, 789);
              superconv(ATT_TIME, ATT_STRING, &tm, buf, cold->specif, t_specif(100,0,0,0), time_format.c_str());  // includes the TZ, if required
              cold->displ_size=(int)strlen(buf);
            }  
            break;  
          case ATT_TIMESTAMP:
            if (timestamp_format.empty())
            { cold->displ_size=19;  // "27.04.1962 12:34:56"
              if (cold->specif.with_time_zone) cold->displ_size += 6;  // time zone information like '+05:00'
            }  
            else  // find the max length for the specified timestamp_format
            { char buf[100+1];  uns32 dtm;
              dtm = datetime2timestamp(Make_date(27, 12, 1956), Make_time(23, 34, 56, 789));
              superconv(ATT_TIMESTAMP, ATT_STRING, &dtm, buf, cold->specif, t_specif(100,0,0,0), timestamp_format.c_str());  // includes the TZ, if required
              cold->displ_size=(int)strlen(buf);
            }  
            break;  
          case ATT_RASTER:
            cold->displ_size=limit_displ_size;  break;
          case ATT_TEXT:
            cold->displ_size=limit_displ_size;  break;
          case ATT_NOSPEC:
            cold->displ_size=limit_displ_size;  break;
          case ATT_INT8:
            cold->displ_size=4;  break;  // "-123"
          case ATT_INT64:
            cold->displ_size=21;  break;
        }
        int namelen = (int)strlen(cold->name);
        if (expanded) // calculate the size of the 2 columns:
        { if (max_displ_size < cold->displ_size) max_displ_size = cold->displ_size;
          if (max_name_len < namelen) max_name_len = namelen;
        }  
        else
          if (!tuples_only)  
            if (cold->displ_size < namelen) cold->displ_size = namelen;
      }  
    }
  char * buf = (char*)malloc(2*(limit_displ_size>ATTRNAMELEN ? limit_displ_size:ATTRNAMELEN) + 1);
    // 2*x used when converting output to UTF-8
  char * rdbuf = (char*)malloc((2*limit_displ_size>MAX_FIXED_STRING_LENGTH ? 2*limit_displ_size:MAX_FIXED_STRING_LENGTH) + 2);
   // 2*limit_displ_size used when reading wide character CLOBs, MAX_FIXED_STRING_LENGTH used when reading strings
 // write the header:
  if (disp_aligned && !expanded && disp_border>=2)
    write_delimiter(coldescr, buf);
 // write column names:   
  if (!tuples_only && !expanded)
  { if (disp_border>=2)
      output_message("| ", true, false);
    bool first = true;  
    for (cold = coldescr;  cold->name[0];  cold++)
      if (!cold->disabled)
      { if (!first)
          output_message(disp_aligned ? " | " : field_separator.c_str(), true, false);
        else  
          first=false;
        output_message(cold->name, false, false);  
        if (disp_aligned)
          fill_output(cold->displ_size - strlen(cold->name), ' ');  // never is negative
      }  
    if (disp_border>=2)
      output_message(" |", true, false);  
    output_message(disp_aligned ? "\n" : record_separator.c_str(), true, false);  
    if (disp_aligned && disp_border>=1)
      write_delimiter(coldescr, buf);
  }
 // pass the records:   
  tattrib atr;
  for (trecnum rec = 0;  rec<reccnt && !cancel_long_output;  rec++)  // testing [cancel_long_output] saves long reading the data and outputting the to nul
    if (expanded)
    { if (disp_border>=1)
      { if (disp_border>=2)
          output_message("+-", true, false);
        char buf[100];
        sprintf(buf, _("Record number %u:"), rec);
        output_message(buf, true, false);
        int space = max_name_len+max_displ_size+3;
        if (disp_aligned && space>(int)strlen(buf))
        { fill_output(space-strlen(buf), '-');
          if (disp_border>=2)
            output_message("-+", true, false);
        }
        newline();
      }
      for (atr=1, cold = coldescr;  cold->name[0];  atr++, cold++)
        if (!cold->disabled)
        { if (disp_border>=2)
            output_message("| ", true, false);
         // column name:   
          output_message(cold->name, false, false);  
          if (disp_aligned)
            fill_output(max_name_len - strlen(cold->name), ' ');  // never is negative
          output_message(" | ", true, false);  
         // column value:
          get_text_column_value(cdp, curs, rec, atr, cold, buf, rdbuf);
          bool left_alignment = cold->type!=ATT_INT8 && cold->type!=ATT_INT16 && cold->type!=ATT_INT32 && cold->type!=ATT_INT64 && cold->type!=ATT_MONEY && cold->type!=ATT_FLOAT; 
          int vallen = utf8_strlen(buf);
          if (disp_aligned && vallen>max_displ_size)
            { vallen=max_displ_size;  buf[vallen]=0; }
          if (disp_aligned && !left_alignment)
            fill_output(max_displ_size - vallen, ' ');  // never is negative
          output_message(buf, true, false);  
          if (disp_aligned && left_alignment)
            fill_output(max_displ_size - vallen, ' ');  // never is negative
          if (disp_border>=2)
            output_message(" |", true, false);  
          newline();
        }  
    }
    else
    { if (disp_aligned && disp_border>=2)
        output_message("| ", true, false);
      bool first = true;  
      for (atr=1, cold = coldescr;  cold->name[0];  atr++, cold++)
        if (!cold->disabled)
        { if (!first)
            output_message(disp_aligned ? " | " : field_separator.c_str(), true, false);
          else  
            first=false;
         // output the column value:   
          get_text_column_value(cdp, curs, rec, atr, cold, buf, rdbuf);
          bool left_alignment = cold->type!=ATT_INT8 && cold->type!=ATT_INT16 && cold->type!=ATT_INT32 && cold->type!=ATT_INT64 && cold->type!=ATT_MONEY && cold->type!=ATT_FLOAT; 
          int vallen = utf8_strlen(buf);
          if (disp_aligned && vallen>cold->displ_size)
            { vallen=cold->displ_size;  buf[vallen]=0; }
          if (disp_aligned && !left_alignment)
            fill_output(cold->displ_size - vallen, ' ');  // never is negative
          output_message(buf, true, false);  
          if (disp_aligned && left_alignment)
            fill_output(cold->displ_size - vallen, ' ');  // never is negative
        }  
      if (disp_aligned && disp_border>=2)
        output_message(" |", true, false);  
      output_message(disp_aligned ? "\n" : record_separator.c_str(), true, false);  
    }
 // write the footer:
  if (disp_aligned && disp_border>=2 && reccnt>0)
    if (expanded)
    { output_message("+-", true, false);
      fill_output(max_name_len, '-');
      output_message("-+-", true, false);  
      fill_output(max_displ_size, '-');
      output_message("-+", true, false);  
      newline();
    }  
    else
      write_delimiter(coldescr, buf);
 // record count:
  if (!tuples_only && disp_aligned)
  { sprintf(rdbuf, _("Record count = %u.\n"), reccnt);
    output_message(rdbuf, true, false);
  }  
  free(rdbuf);
  free(buf);
  free(coldescr);
}

