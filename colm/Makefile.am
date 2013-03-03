#
#   Copyright 2007-2012 Adrian Thurston <thurston@complang.org>
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

INCLUDES = -I$(top_srcdir)/aapl

bin_PROGRAMS = colm
noinst_PROGRAMS = bootstrap0 bootstrap1

RUNTIME_SRC = \
	map.c pdarun.c list.c input.c debug.c \
	codevect.c pool.c string.c tree.c bytecode.c program.c

RUNTIME_HDR = \
	bytecode.h config.h defs.h debug.h pool.h input.h \
	pdarun.h map.h tree.h program.h colm.h

lib_LIBRARIES = libcolm.a

noinst_LIBRARIES = libprog.a

libcolm_a_SOURCES = $(RUNTIME_SRC)
libcolm_a_CFLAGS = -I..

common_CFLAGS = \
	-Wall \
	-DPREFIX='"$(prefix)"' \
	-I..

libprog_a_SOURCES = \
	buffer.h bytecode.h colm.h debug.h dotgen.h fsmcodegen.h fsmgraph.h \
	input.h keyops.h lmparse.h lmscan.h map.h parsedata.h \
	parsetree.h pcheck.h pdacodegen.h pdagraph.h pdarun.h pool.h redbuild.h \
	redfsm.h rtvector.h tree.h version.h global.h colm.h parser.h \
	\
	resolve.cc synthesis.cc lmparse.cc lmscan.cc parsetree.cc \
	fsmstate.cc fsmbase.cc fsmattach.cc fsmmin.cc \
	fsmgraph.cc pdagraph.cc pdabuild.cc pdacodegen.cc fsmcodegen.cc \
	redfsm.cc fsmexec.cc redbuild.cc closure.cc fsmap.cc \
	dotgen.cc pcheck.cc ctinput.cc declare.cc codegen.cc \
	exports.cc compiler.cc parser.cc

libprog_a_CXXFLAGS = $(common_CFLAGS)

bootstrap0_CXXFLAGS = $(common_CFLAGS) -DBOOTSTRAP0
bootstrap0_SOURCES = bootstrap.cc main.cc
bootstrap0_LDADD = libprog.a libcolm.a

bootstrap1_CXXFLAGS = $(common_CFLAGS)
bootstrap1_CFLAGS = $(common_CFLAGS)
bootstrap1_SOURCES = exports1.h exports1.cc parse1.c main.cc
bootstrap1_LDADD = libprog.a libcolm.a

colm_CXXFLAGS = $(common_CFLAGS) 
colm_CFLAGS = $(common_CFLAGS)
colm_SOURCES = exports2.h exports2.cc parse2.c main.cc
colm_LDADD = libprog.a libcolm.a

colmincdir = $(includedir)/colm

colminc_HEADERS = $(RUNTIME_HDR)

exports1.h: bootstrap0 parse1.c
exports1.cc: bootstrap0 parse1.c

parse1.c: bootstrap0
	$(builddir)/bootstrap0 -L -o parse1.c -e exports1.h -c exports1.cc -B

exports2.h: bootstrap1 parse2.c
exports2.cc: bootstrap1 parse2.c

parse2.c: bootstrap1
	$(builddir)/bootstrap1 -L -o parse2.c -e exports2.h -c exports2.cc colm.lm

BUILT_SOURCES = \
	version.h lmscan.cc lmparse.h lmparse.cc \
	exports1.h exports1.cc parse1.c \
	exports2.h exports2.cc parse2.c

version.h: Makefile
	echo '#define VERSION "$(PACKAGE_VERSION)"' > version.h
	echo '#define PUBDATE "$(PUBDATE)"' >> version.h

if BUILD_PARSERS
 
lmparse.h: lmparse.kh
	$(KELBT) -o $@ $<

lmparse.cc: lmparse.kl lmparse.kh
	$(KELBT) -o $@ $<

lmscan.cc: lmparse.h

lmscan.cc: lmscan.rl
	$(RAGEL) -G2 -o $@ $<

CLEANFILES = $(BUILT_SOURCES)

endif
