// Copyright 2001-2010 William S. Yerazunis.
//
//   This file is part of the CRM114 Library.
//
//   The CRM114 Library is free software: you can redistribute it and/or modify
//   it under the terms of the GNU Lesser General Public License as published by
//   the Free Software Foundation, either version 3 of the License, or
//   (at your option) any later version.
//
//   The CRM114 Library is distributed in the hope that it will be useful,
//   but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//   GNU Lesser General Public License for more details.
//
//   You should have received a copy of the GNU Lesser General Public License
//   along with the CRM114 Library.  If not, see <http://www.gnu.org/licenses/>.

#define _GNU_SOURCE
#include <dlfcn.h>
#include <string.h>

#include "crm114_regex.h"

int crm114__regncomp(regex_t *preg, const char *regex, long regex_len, int cflags)
{
  return tre_regncomp(preg, regex, regex_len, cflags);
}

int crm114__regnexec(const regex_t *preg, const char *string, long string_len,
		 size_t nmatch, regmatch_t pmatch[], int eflags)
{
  return tre_regnexec(preg, string, string_len, nmatch, pmatch, eflags);
}

size_t crm114__regerror(int errcode, const regex_t *preg, char *errbuf,
		       size_t errbuf_size)
{
  return tre_regerror(errcode, preg, errbuf, errbuf_size);
}

void tre_free(regex_t *preg);
void crm114__regfree(regex_t *preg)
{
  /* The following bug occurs in the 0.8.0 version of tre:

     When tre is configured to use the system regex interface, it
     #defines 'tre_regfree' to be 'regfree'.  This usually works fine,
     except in situations where 'regfree' has already been resolved by
     the linker to point to libc's version (e.g. when loaded from
     within the Python interpreter).  In this case, our use of
     'tre_regncomp' (which is not #defined away) initializes the regex
     context, but it is freed by libc's 'regfree', which causes a
     segfault.

     Luckily, we can still access libtre's 'tre_free' function to free
     up context state allocated by tre.
   */

  Dl_info info;
  memset(&info, 0, sizeof(info));
  if (dladdr((void *)tre_regfree, &info)) {
    if (info.dli_fname && strstr(info.dli_fname, "libtre") == NULL) {
      tre_free(preg);
      return;
    }
  }

  tre_regfree(preg);
}
