#
#   Copyright 2007-2013 Adrian Thurston <thurston@complang.org>
#

#   This file is part of Colm.
#
#   Colm is free software; you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation; either version 2 of the License, or
#   (at your option) any later version.
#
#   Colm is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with Colm; if not, write to the Free Software
#   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA 

AM_CPPFLAGS = -I$(top_srcdir)/aapl -Iinclude

AUTOMAKE_OPTIONS = subdir-objects

bin_PROGRAMS = colm
noinst_PROGRAMS = bootstrap0 bootstrap1

RUNTIME_SRC = \
	map.c pdarun.c list.c input.c debug.c \
	codevect.c pool.c string.c tree.c iter.c \
	bytecode.c program.c struct.c commit.c

RUNTIME_HDR = \
	bytecode.h config.h defs.h debug.h pool.h input.h \
	pdarun.h map.h type.h tree.h struct.h program.h colm.h internal.h

lib_LTLIBRARIES = libcolm.la

noinst_LIBRARIES = libprog.a

libcolm_la_SOURCES = $(RUNTIME_SRC)
libcolm_la_LDFLAGS = -release ${PACKAGE_VERSION}

common_CFLAGS = \
	-Wall \
	-DPREFIX='"$(prefix)"' \
	-Iinclude

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

bootstrap0_CXXFLAGS = $(common_CFLAGS) -DCONS_INIT
bootstrap0_SOURCES = consinit.cc consinit.h main.cc
bootstrap0_LDADD = libprog.a libcolm.la

bootstrap1_CXXFLAGS = $(common_CFLAGS) -DLOAD_INIT
bootstrap1_CFLAGS = $(common_CFLAGS)
bootstrap1_SOURCES = \
	loadinit.h loadinit.cc main.cc
nodist_bootstrap1_SOURCES = \
	gen/if1.h gen/if1.cc gen/parse1.c
bootstrap1_LDADD = libprog.a libcolm.la

colm_CXXFLAGS = $(common_CFLAGS) -DLOAD_COLM
colm_CFLAGS = $(common_CFLAGS)
colm_SOURCES = \
	loadcolm.h loadcolm.cc main.cc
nodist_colm_SOURCES = \
	gen/if2.h gen/if2.cc gen/parse2.c
colm_LDADD = libprog.a libcolm.la

colmincdir = $(includedir)/colm

colminc_HEADERS = $(RUNTIME_HDR)

gen/parse1.c: bootstrap0
	mkdir -p gen
	$(builddir)/bootstrap0 -c -o gen/parse1.c -e gen/if1.h -x gen/if1.cc

gen/if1.h: gen/parse1.c
gen/if1.cc: gen/parse1.c

gen/parse2.c: bootstrap1 colm.lm
	mkdir -p gen
	$(builddir)/bootstrap1 -c -o gen/parse2.c -e gen/if2.h -x gen/if2.cc colm.lm

gen/if2.h: gen/parse2.c
gen/if2.cc: gen/parse2.c

gen/bootstrap1-if1.$(OBJEXT): gen/if1.h gen/if1.cc gen/parse1.c
bootstrap1-loadinit.$(OBJEXT): gen/if1.h gen/if1.cc gen/parse1.c

gen/colm-if2.$(OBJEXT): gen/if2.h gen/if2.cc gen/parse2.c
colm-loadcolm.$(OBJEXT): gen/if2.h gen/if2.cc gen/parse2.c

BUILT_SOURCES = version.h include/colm

include/colm:
	mkdir -p include
	ln -s .. include/colm

version.h: Makefile
	echo '#define VERSION "$(PACKAGE_VERSION)"' > version.h
	echo '#define PUBDATE "$(PUBDATE)"' >> version.h

CLEANFILES = $(BUILT_SOURCES)
EXTRA_DIST = colm.lm
