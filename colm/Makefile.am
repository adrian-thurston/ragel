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

bin_PROGRAMS = acolm colm

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

acolm_CXXFLAGS = $(common_CFLAGS) -DBOOTSTRAP
acolm_SOURCES = bootstrap.cc main.cc
acolm_LDADD = libprog.a libcolm.a

colm_CXXFLAGS = $(common_CFLAGS) 
colm_CFLAGS = $(common_CFLAGS)
colm_SOURCES = aiface.h aiface.cc aparse.c main.cc
colm_LDADD = libprog.a libcolm.a

colmincdir = $(includedir)/colm

colminc_HEADERS = $(RUNTIME_HDR)

aiface.h: acolm
aiface.cc: acolm
aparse.c: acolm
	./acolm -L -B -o aparse.c -e aiface.h -c aiface.cc

BUILT_SOURCES = \
	version.h lmscan.cc lmparse.h lmparse.cc aiface.h aiface.cc aparse.c

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
