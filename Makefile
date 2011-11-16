#  Makefile for CRM114 library
#
# Copyright 2010 Kurt Hackenberg & William S. Yerazunis, each individually
# with full rights to relicense.
#
#   This file is part of the CRM114 Library.
#
#   The CRM114 library is free software: you can redistribute it and/or modify
#   it under the terms of the GNU Lesser General Public License as published by
#   the Free Software Foundation, either version 3 of the License, or
#   (at your option) any later version.
#
#   The CRM114 Library is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU Lesser General Public License for more details.
#
#   You should have received a copy of the GNU Lesser General Public License
#   along with the CRM114 Library.  If not, see <http://www.gnu.org/licenses/>.


#C flags below are for GCC 4.3.2.


#Use one of the following sets of options for generating code, debug
#information, and profiling information.

#GCC flags: no debugging, optimize, inline matrix library
#Defining DO_INLINES turns on use of a GCC extension in the matrix library,
#so don't turn it on for other compilers.
#CFLAGS += -O3 -DDO_INLINES
#
#GCC flags for debugging, no optimization
CFLAGS += -g
#
#GCC and LD flags for debugging, no optimization, and profile for speed
#CFLAGS += -g -pg
#LDFLAGS += -pg
#
#GCC and LD flags for debugging, no optimization, and profile for coverage
#CFLAGS += -g -fprofile-arcs -ftest-coverage
#LDFLAGS += -fprofile-arcs -ftest-coverage

CFLAGS += -Iinclude

#always use this: C99, and check source code carefully
CFLAGS += -std=c99 -pedantic -Wall -Wextra -Wpointer-arith -Wstrict-prototypes
#well, pretty carefully
CFLAGS += -Wno-sign-compare -Wno-overlength-strings

#These are optional.

#warn about any type conversion that could possibly change a value
#CFLAGS += -Wconversion
#warn about variable-length arrays, which Microsoft C (C89 w/ ext) can't handle
#CFLAGS += -Wvla
#warn about undefined macro in #if value
#CFLAGS += -Wundef
#warn about structures marked packed that had no padding anyway
#CFLAGS += -Wpacked
#tell us when padding a structure
#CFLAGS += -Wpadded
#tell us when denying an inline request
#CFLAGS += -Winline

LIBHDR_FILES =					\
crm114_lib.h					\
crm114_sysincludes.h				\
crm114_config.h					\
crm114_structs.h				\
crm114_internal.h				\
crm114_regex.h					\
crm114_svm.h					\
crm114_svm_lib_fncts.h				\
crm114_pca.h					\
crm114_pca_lib_fncts.h				\
crm114_matrix.h					\
crm114_matrix_util.h				\
crm114_svm_quad_prog.h				\
crm114_datalib.h

LIBHDRS = $(foreach hdr, $(LIBHDR_FILES), include/$(hdr))

LIBOBJ_FILES =					\
crm114_base.o					\
crm114_markov.o					\
crm114_markov_microgroom.o			\
crm114_bit_entropy.o				\
crm114_hyperspace.o				\
crm114_svm.o					\
crm114_svm_lib_fncts.o				\
crm114_svm_quad_prog.o				\
crm114_fast_substring_compression.o		\
crm114_pca.o					\
crm114_pca_lib_fncts.o				\
crm114_matrix.o					\
crm114_matrix_util.o				\
crm114_datalib.o				\
crm114_vector_tokenize.o			\
crm114_strnhash.o				\
crm114_util.o					\
crm114_regex_tre.o

LIBOBJS = $(foreach obj, $(LIBOBJ_FILES), lib/$(obj))

LIB = lib/libcrm114.a

all: $(LIB) tests/test tests/simple_demo

$(LIBOBJS): $(LIBHDRS) Makefile

# #ar cmd below has no s modifier (like ranlib).  Not needed now, partly
# #because LIBOBJS is in the right order.
$(LIB): $(LIBOBJS) Makefile
	ar rc $@ $(LIBOBJS)

lib/%.o: lib/%.c
	$(CC) -c $(CFLAGS) -o $@ $<

tests/test: tests/test.c tests/texts.h $(LIBHDRS)
	$(CC) $(CFLAGS) -o $@ $(LDFLAGS) -Wl,-M -Wl,--cref $< $(LIB) -ltre -lm >tests/test.map

tests/simple_demo: tests/simple_demo.c tests/texts.h
	$(CC) $(CFLAGS) -o $@ $(LDFLAGS) -Wl,-M -Wl,--cref $< $(LIB) -ltre -lm >tests/simple_demo.map

clean: clean_test clean_simple_demo clean_lib clean_profiling

clean_test:
	rm -f tests/test tests/test.map tests/test.o

clean_simple_demo:
	rm -f tests/simple_demo tests/simple_demo.map tests/simple_demo.o {,tests/}simple_demo_datablock.txt

clean_lib:
	rm -f $(LIB) $(LIBOBJS)

clean_profiling:
	rm -f gmon.out *.gcov {,lib/}*.gcno {,lib/}*.gcda
