#
# Copyright 2007-2018 Adrian Thurston <thurston@colm.net>
#

# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to
# deal in the Software without restriction, including without limitation the
# rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
# sell copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
# 
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

AM_CPPFLAGS = -I$(top_srcdir)/aapl

AUTOMAKE_OPTIONS = subdir-objects

bin_PROGRAMS = colm
bin_SCRIPTS = colm-wrap

RUNTIME_SRC = \
	map.c pdarun.c list.c input.c stream.c debug.c \
	codevect.c pool.c string.c tree.c iter.c \
	bytecode.c program.c struct.c commit.c \
	print.c

RUNTIME_HDR = \
	config.h bytecode.h defs.h debug.h pool.h input.h \
	pdarun.h map.h type.h tree.h struct.h program.h colm.h \
	internal.h colmex.h

lib_LTLIBRARIES = libcolm.la
noinst_LIBRARIES = libprog.a

libcolm_la_SOURCES = $(RUNTIME_SRC)
libcolm_la_LDFLAGS = -release ${COLM_VERSION} -no-undefined

if LINKER_NO_UNDEFINED
libcolm_la_LDFLAGS += -Wl,--no-undefined
endif

common_CFLAGS = \
	-Wall \
	-DPREFIX='"$(prefix)"'

libprog_a_SOURCES = \
	buffer.h bytecode.h colm.h debug.h dotgen.h fsmcodegen.h fsmgraph.h \
	input.h keyops.h map.h compiler.h \
	parsetree.h pcheck.h pdacodegen.h pdagraph.h pdarun.h pool.h redbuild.h \
	redfsm.h rtvector.h tree.h version.h global.h colm.h parser.h cstring.h \
	internal.h \
	\
	resolve.cc lookup.cc synthesis.cc parsetree.cc \
	fsmstate.cc fsmbase.cc fsmattach.cc fsmmin.cc \
	fsmgraph.cc pdagraph.cc pdabuild.cc pdacodegen.cc fsmcodegen.cc \
	redfsm.cc fsmexec.cc redbuild.cc closure.cc fsmap.cc \
	dotgen.cc pcheck.cc ctinput.cc declare.cc codegen.cc \
	exports.cc compiler.cc parser.cc reduce.cc

libprog_a_CXXFLAGS = $(common_CFLAGS)

colmincdir = $(includedir)/colm

colminc_HEADERS = $(RUNTIME_HDR)

BUILT_SOURCES = version.h include/colm

if EXTERNAL_COLM

#
# Generate the parser using a single run with an external colm program.
#
BUILD_PARSE_3_WITH = $(EXTERNAL_COLM)/bin/colm$(EXEEXT)
AM_CPPFLAGS += $(EXTERNAL_INC)
AM_LDFLAGS = $(EXTERNAL_LIBS)

else

noinst_PROGRAMS = bootstrap0 bootstrap1 bootstrap2

BUILD_PARSE_3_WITH = $(builddir)/bootstrap2$(EXEEXT)
AM_CPPFLAGS += -Iinclude
AM_LDFLAGS = -L.

#
# bootstrap0: The input program for bootstrap0 is construced using internal
# data structure constructors. It produces a program that can parse a grammar
# using limited features. No code is supported. 
#
bootstrap0_CXXFLAGS = $(common_CFLAGS) -DCONS_INIT
bootstrap0_SOURCES = consinit.cc consinit.h main.cc version.h
bootstrap0_LDADD = libprog.a libcolm.la

#
# bootstrap1: The input program is specified using a stripped down colm syntax.
# It produces a program that can parse most colm syntax, with the exception of
# the colm syntax/semantics that is implemented in colm itself.
#

gen/bootstrap1.pack: colm-wrap bootstrap0$(EXEEXT)
	mkdir -p gen
	$(builddir)/colm-wrap -w bootstrap0 -o $@ \
		-c -p gen/parse1.c -e gen/if1.h -x gen/if1.cc no-input

gen/parse1.c: gen/bootstrap1.pack
	$(builddir)/colm-wrap -o $@ $<

gen/if1.h: gen/bootstrap1.pack
	$(builddir)/colm-wrap -o $@ $<

gen/if1.cc: gen/bootstrap1.pack
	$(builddir)/colm-wrap -o $@ $<

# Listing if1.h in BUILT_SOURCES isn't sufficient because it depends on the
# building of bootstrap0. Automake wants to put all built sources into a list
# of files built before ANYTHING else (which includes bootstrap0). Not sure if
# it is rejected by automake or make. But in any case, it doesn't work. Fixed
# with the following additional dependency.
BUILT_SOURCES += gen/parse1.c gen/if1.h gen/if1.cc
loadinit.cc: gen/if1.h

bootstrap1_CXXFLAGS = $(common_CFLAGS) -DLOAD_INIT
bootstrap1_CFLAGS = $(common_CFLAGS)
bootstrap1_SOURCES = loadinit.h loadinit.cc main.cc version.h
nodist_bootstrap1_SOURCES = gen/if1.h gen/if1.cc gen/parse1.c
bootstrap1_LDADD = libprog.a libcolm.la

#
# bootstrap2: The input program is specified using the colm grammar used in
# bootstrap1, plus some rewrite rules that implement the final parts of
# syntax/semantics. It produces a program that can parse full colm programs,
# and thus generates the sources used in the colm binary.
#

gen/bootstrap2.pack: colm-wrap bootstrap1$(EXEEXT) colm.lm
	mkdir -p gen
	$(builddir)/colm-wrap -w bootstrap1 -o $@ \
		-c -p gen/parse2.c -e gen/if2.h -x gen/if2.cc colm.lm

gen/parse2.c: gen/bootstrap2.pack
	$(builddir)/colm-wrap -o $@ $<

gen/if2.h: gen/bootstrap2.pack
	$(builddir)/colm-wrap -o $@ $<

gen/if2.cc: gen/bootstrap2.pack
	$(builddir)/colm-wrap -o $@ $<

BUILT_SOURCES += gen/parse2.c gen/if2.h gen/if2.cc
loadboot2.cc: gen/if2.h

bootstrap2_CXXFLAGS = $(common_CFLAGS) -DLOAD_COLM
bootstrap2_CFLAGS = $(common_CFLAGS)
bootstrap2_SOURCES = main.cc loadboot2.cc loadfinal.h version.h
nodist_bootstrap2_SOURCES = gen/if2.h gen/if2.cc gen/parse2.c
bootstrap2_LDADD = libprog.a libcolm.la

endif

gen/bootstrap3.pack: colm-wrap $(BUILD_PARSE_3_WITH) prog.lm colm.lm 
	mkdir -p gen
	$(builddir)/colm-wrap -w $(BUILD_PARSE_3_WITH) -o $@ \
		-c -p gen/parse3.c -e gen/if3.h -x gen/if3.cc prog.lm

gen/parse3.c: gen/bootstrap3.pack
	$(builddir)/colm-wrap -o $@ $<

gen/if3.h: gen/bootstrap3.pack
	$(builddir)/colm-wrap -o $@ $<

gen/if3.cc: gen/bootstrap3.pack
	$(builddir)/colm-wrap -o $@ $<

BUILT_SOURCES += gen/parse3.c gen/if3.h gen/if3.cc
loadcolm.cc: gen/if3.h

colm_CXXFLAGS = $(common_CFLAGS) -DLOAD_COLM
colm_CFLAGS = $(common_CFLAGS)
colm_SOURCES = main.cc loadcolm.cc loadfinal.h version.h
nodist_colm_SOURCES = gen/if3.h gen/if3.cc gen/parse3.c
colm_LDADD = libprog.a -lcolm

include/colm:
	mkdir -p include
	ln -s .. include/colm

version.h: Makefile
	echo '#define COLM_VERSION "$(COLM_VERSION)"' > version.h
	echo '#define COLM_PUBDATE "$(COLM_PUBDATE)"' >> version.h

config.h: ../src/config.h
	cp ../src/config.h ./

CLEANFILES = $(BUILT_SOURCES) gen/bootstrap1.pack gen/bootstrap2.pack gen/bootstrap3.pack
EXTRA_DIST = prog.lm colm.lm loadfinal.cc colm-wrap.sh

colm-wrap: colm-wrap.sh
	@$(top_srcdir)/sedsubst $< $@ -w,+x $(SED_SUBST)
