/*************************************************
*      Perl-Compatible Regular Expressions       *
*************************************************/

/* PCRE is a library of functions to support regular expressions whose syntax
and semantics are as close as possible to those of the Perl 5 language.

                       Written by Philip Hazel
           Copyright (c) 1997-2007 University of Cambridge

-----------------------------------------------------------------------------
Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice,
      this list of conditions and the following disclaimer.

    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.

    * Neither the name of the University of Cambridge nor the names of its
      contributors may be used to endorse or promote products derived from
      this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.
-----------------------------------------------------------------------------
*/


/* This module contains the external function pcre_maketables(), which builds
character tables for PCRE in the current locale. The file is compiled on its
own as part of the PCRE library. However, it is also included in the
compilation of dftables.c, in which case the macro DFTABLES is defined. */


#ifndef DFTABLES
#include "pcre_internal.h"
#endif


/*************************************************
*           Create PCRE character tables         *
*************************************************/

/* This function builds a set of character tables for use by PCRE and returns
a pointer to them. They are build using the ctype functions, and consequently
their contents will depend upon the current locale setting. When compiled as
part of the library, the store is obtained via pcre_malloc(), but when compiled
inside dftables, use malloc().

Arguments:   none
Returns:     pointer to the contiguous block of data
*/

#ifdef DFTABLES // the original version of the function, used only when the pcre library is being build

const unsigned char *
pcre_maketables(void)
{
unsigned char *yield, *p;
int i;

#ifndef DFTABLES
yield = (unsigned char*)(pcre_malloc)(tables_length);
#else
yield = (unsigned char*)malloc(tables_length);
#endif

if (yield == NULL) return NULL;
p = yield;

/* First comes the lower casing table */

for (i = 0; i < 256; i++) *p++ = tolower(i);

/* Next the case-flipping table */

for (i = 0; i < 256; i++) *p++ = islower(i)? toupper(i) : tolower(i);

/* Then the character class tables. Don't try to be clever and save effort on
exclusive ones - in some locales things may be different. Note that the table
for "space" includes everything "isspace" gives, including VT in the default
locale. This makes it work for the POSIX class [:space:]. Note also that it is
possible for a character to be alnum or alpha without being lower or upper,
such as "male and female ordinals" (\xAA and \xBA) in the fr_FR locale (at
least under Debian Linux's locales as of 12/2005). So we must test for alnum
specially. */

memset(p, 0, cbit_length);
for (i = 0; i < 256; i++)
  {
  if (isdigit(i)) p[cbit_digit  + i/8] |= 1 << (i&7);
  if (isupper(i)) p[cbit_upper  + i/8] |= 1 << (i&7);
  if (islower(i)) p[cbit_lower  + i/8] |= 1 << (i&7);
  if (isalnum(i)) p[cbit_word   + i/8] |= 1 << (i&7);
  if (i == '_')   p[cbit_word   + i/8] |= 1 << (i&7);
  if (isspace(i)) p[cbit_space  + i/8] |= 1 << (i&7);
  if (isxdigit(i))p[cbit_xdigit + i/8] |= 1 << (i&7);
  if (isgraph(i)) p[cbit_graph  + i/8] |= 1 << (i&7);
  if (isprint(i)) p[cbit_print  + i/8] |= 1 << (i&7);
  if (ispunct(i)) p[cbit_punct  + i/8] |= 1 << (i&7);
  if (iscntrl(i)) p[cbit_cntrl  + i/8] |= 1 << (i&7);
  }
p += cbit_length;

/* Finally, the character type table. In this, we exclude VT from the white
space chars, because Perl doesn't recognize it as such for \s and for comments
within regexes. */

for (i = 0; i < 256; i++)
  {
  int x = 0;
  if (i != 0x0b && isspace(i)) x += ctype_space;
  if (isalpha(i)) x += ctype_letter;
  if (isdigit(i)) x += ctype_digit;
  if (isxdigit(i)) x += ctype_xdigit;
  if (isalnum(i) || i == '_') x += ctype_word;

  /* Note: strchr includes the terminating zero in the characters it considers.
  In this instance, that is ok because we want binary zero to be flagged as a
  meta-character, which in this sense is any character that terminates a run
  of data characters. */

  if (strchr("\\*+?{^.$|()[", i) != 0) x += ctype_meta;
  *p++ = x;
  }

return yield;
}

#else  ////////////////////////////////////////////////////////////////////////////
// updated version if pcre_maketables() using locales specified by the call to the
// pcre_set_locale() and calling the ..._l functions with locale specified by a parameter.

struct __locale_struct * loc = NULL;

void pcre_set_locale(struct __locale_struct * locIn)
{ loc=locIn; }

#include "apsymbols.h"  // decreases the GLIBC versio number the product is dependent on

const unsigned char *
pcre_maketables(void)
{
unsigned char *yield, *p;
int i;

#ifndef DFTABLES
yield = (unsigned char*)(pcre_malloc)(tables_length);
#else
yield = (unsigned char*)malloc(tables_length);
#endif

if (yield == NULL) return NULL;
p = yield;

/* First comes the lower casing table */

for (i = 0; i < 256; i++) *p++ = __tolower_l(i, loc);

/* Next the case-flipping table */

for (i = 0; i < 256; i++) *p++ = __islower_l(i, loc)? __toupper_l(i, loc) : __tolower_l(i, loc);

/* Then the character class tables. Don't try to be clever and save effort on
exclusive ones - in some locales things may be different. Note that the table
for "space" includes everything "isspace" gives, including VT in the default
locale. This makes it work for the POSIX class [:space:]. Note also that it is
possible for a character to be alnum or alpha without being lower or upper,
such as "male and female ordinals" (\xAA and \xBA) in the fr_FR locale (at
least under Debian Linux's locales as of 12/2005). So we must test for alnum
specially. */

memset(p, 0, cbit_length);
for (i = 0; i < 256; i++)
  {
  if (__isdigit_l(i, loc)) p[cbit_digit  + i/8] |= 1 << (i&7);
  if (__isupper_l(i, loc)) p[cbit_upper  + i/8] |= 1 << (i&7);
  if (__islower_l(i, loc)) p[cbit_lower  + i/8] |= 1 << (i&7);
  if (__isalnum_l(i, loc)) p[cbit_word   + i/8] |= 1 << (i&7);
  if (i == '_')   p[cbit_word   + i/8] |= 1 << (i&7);
  if (__isspace_l(i, loc)) p[cbit_space  + i/8] |= 1 << (i&7);
  if (__isxdigit_l(i, loc))p[cbit_xdigit + i/8] |= 1 << (i&7);
  if (__isgraph_l(i, loc)) p[cbit_graph  + i/8] |= 1 << (i&7);
  if (__isprint_l(i, loc)) p[cbit_print  + i/8] |= 1 << (i&7);
  if (__ispunct_l(i, loc)) p[cbit_punct  + i/8] |= 1 << (i&7);
  if (__iscntrl_l(i, loc)) p[cbit_cntrl  + i/8] |= 1 << (i&7);
  }
p += cbit_length;

/* Finally, the character type table. In this, we exclude VT from the white
space chars, because Perl doesn't recognize it as such for \s and for comments
within regexes. */

for (i = 0; i < 256; i++)
  {
  int x = 0;
  if (i != 0x0b && __isspace_l(i, loc)) x += ctype_space;
  if (__isalpha_l(i, loc)) x += ctype_letter;
  if (__isdigit_l(i, loc)) x += ctype_digit;
  if (__isxdigit_l(i, loc)) x += ctype_xdigit;
  if (__isalnum_l(i, loc) || i == '_') x += ctype_word;

  /* Note: strchr includes the terminating zero in the characters it considers.
  In this instance, that is ok because we want binary zero to be flagged as a
  meta-character, which in this sense is any character that terminates a run
  of data characters. */

  if (strchr("\\*+?{^.$|()[", i) != 0) x += ctype_meta;
  *p++ = x;
  }

return yield;
}

#endif
/* End of pcre_maketables.c */
