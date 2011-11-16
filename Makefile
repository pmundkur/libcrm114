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

LIBHDRS =					\
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

LIBOBJS =					\
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


all: test simple_demo

test: test.o libcrm114.a
	$(CC) -o test $(LDFLAGS) -Wl,-M -Wl,--cref test.o libcrm114.a -ltre -lm >test.map

test.o: texts.h $(LIBHDRS)

simple_demo: simple_demo.o libcrm114.a
	$(CC) -o simple_demo $(LDFLAGS) -Wl,-M -Wl,--cref simple_demo.o libcrm114.a -ltre -lm >simple_demo.map

simple_demo.o: texts.h $(LIBHDRS)

#ar cmd below has no s modifier (like ranlib).  Not needed now, partly
#because LIBOBJS is in the right order.
libcrm114.a: $(LIBOBJS) Makefile
	ar rc libcrm114.a $(LIBOBJS)

$(LIBOBJS): $(LIBHDRS)

clean: clean_test clean_simple_demo clean_lib clean_profiling

clean_test:
	rm -f test test.map test.o

clean_simple_demo:
	rm -f simple_demo simple_demo.map simple_demo.o simple_demo_datablock.txt

clean_lib:
	rm -f libcrm114.a $(LIBOBJS)

clean_profiling:
	rm -f gmon.out *.gcov *.gcno *.gcda
