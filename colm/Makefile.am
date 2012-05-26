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

RUNTIME_SRC = \
	map.c pdarun.c list.c input.c debug.c \
	codevect.c pool.c string.c tree.c bytecode.c program.c

RUNTIME_HDR = \
	bytecode.h config.h defs.h debug.h pool.h input.h \
	fsmrun.h pdarun.h map.h tree.h program.h colm.h

lib_LIBRARIES = libcolmp.a libcolmd.a

libcolmp_a_SOURCES = $(RUNTIME_SRC)
libcolmp_a_CFLAGS = -I..

libcolmd_a_SOURCES = $(RUNTIME_SRC)
libcolmd_a_CFLAGS = -I..

colm_CXXFLAGS = \
	-Wall \
	-DCOLM_LOG \
	-DPREFIX='"$(prefix)"' \
	-I..

colm_LDADD = libcolmp.a

# ADT DEFS_COLM += -DCOLM_LOG -DPREFIX='"$(prefix)"'

colm_SOURCES = \
	buffer.h bytecode.h colm.h debug.h dotgen.h fsmcodegen.h fsmgraph.h \
	fsmrun.h input.h keyops.h lmparse.h lmscan.h map.h parsedata.h \
	parsetree.h pcheck.h pdacodegen.h pdagraph.h pdarun.h pool.h redbuild.h \
	redfsm.h rtvector.h tree.h version.h global.h colm.h \
	\
	resolve.cc compile.cc lmparse.cc lmscan.cc parsetree.cc \
	fsmstate.cc fsmbase.cc fsmattach.cc fsmmin.cc \
	fsmgraph.cc pdagraph.cc pdabuild.cc pdacodegen.cc fsmcodegen.cc \
	redfsm.cc fsmexec.cc main.cc redbuild.cc closure.cc fsmap.cc \
	dotgen.cc pcheck.cc ctinput.cc declare.cc codegen.cc colm.cc

colmincdir = $(includedir)/colm

colminc_HEADERS = $(RUNTIME_HDR)

BUILT_SOURCES = \
	version.h lmscan.cc lmparse.h lmparse.cc 

version.h: Makefile
	echo '#define VERSION "$(PACKAGE_VERSION)"' > version.h
	echo '#define PUBDATE "$(PUBDATE)"' >> version.h

if BUILD_PARSERS
 
lmparse.h: lmparse.kh
	kelbt -o $@ $<

lmparse.cc: lmparse.kl lmparse.kh
	kelbt -o $@ $<

lmscan.cc: lmparse.h

lmscan.cc: lmscan.rl
	ragel -G2 -o $@ $<
 
endif

# ADT 
# ADT # Logging:
# ADT # colm: rt on/off
# ADT # rt_prd: off
# ADT # rt_db: on
# ADT # rt_clm: rt on/off
# ADT 
# ADT INCS += -I../aapl
# ADT 
# ADT DEFS_COLM += -DCOLM_LOG -DPREFIX='"$(prefix)"'
# ADT DEFS_RT_P +=
# ADT DEFS_RT_D += -DCOLM_LOG
# ADT 
# ADT CFLAGS += -g -Wall -Wwrite-strings
# ADT LDFLAGS +=
# ADT 
# ADT # Files in ALL_SRC that are generated.
# ADT GEN_SRC = version.h lmscan.cc lmparse.h lmparse.cc 
# ADT 
# ADT RUNTIME_P = libcolmp.a
# ADT RUNTIME_D = libcolmd.a
# ADT 
# ADT LIBS = 
# ADT 
# ADT #*************************************
# ADT 
# ADT # Get the version info.
# ADT include ../version.mk
# ADT 
# ADT prefix = @prefix@
# ADT 
# ADT BUILD_PARSERS = @BUILD_PARSERS@
# ADT 
# ADT # Programs
# ADT CXX = @CXX@
# ADT CC = @CC@
# ADT 
# ADT # Get objects and dependencies from sources.
# ADT COLM_OBJ = $(COLM_SRC:%.cc=%.o)
# ADT RUNTIME_OBJ_P = $(RUNTIME_SRC:%.c=%_p.o)
# ADT RUNTIME_OBJ_D = $(RUNTIME_SRC:%.c=%_d.o)
# ADT 
# ADT DEPS = $(COLM_SRC:%.cc=.%.d) $(RUNTIME_SRC:%.c=.%_p.d) $(RUNTIME_SRC:%.c=.%_d.d)
# ADT 
# ADT # Rules.
# ADT all: colm $(RUNTIME_P) $(RUNTIME_D)
# ADT 
# ADT colm: $(GEN_SRC) $(COLM_OBJ) $(RUNTIME_OBJ_D) $(RUNTIME_OBJ_C_D) $(LIBS)
# ADT 	$(CXX) $(LDFLAGS) -o $@ $(COLM_OBJ) $(RUNTIME_OBJ_D) $(RUNTIME_OBJ_C_D) $(LIBS)
# ADT 
# ADT $(RUNTIME_P): $(RUNTIME_OBJ_P) $(RUNTIME_OBJ_C_P)
# ADT 	ar -cr $@ $^
# ADT 
# ADT $(RUNTIME_D): $(RUNTIME_OBJ_D) $(RUNTIME_OBJ_C_D)
# ADT 	ar -cr $@ $^
# ADT 
# ADT version.h: ../version.mk
# ADT 	echo '#define VERSION "$(VERSION)"' > version.h
# ADT 	echo '#define PUBDATE "$(PUBDATE)"' >> version.h
# ADT 
# ADT 
# ADT $(COLM_OBJ): %.o: %.cc
# ADT 	@$(CXX) -M $(DEFS_COLM) $(INCS) $< > .$*.d
# ADT 	$(CXX) -c $(CFLAGS) $(DEFS_COLM) $(INCS) -o $@ $<
# ADT 
# ADT $(RUNTIME_OBJ_P): %_p.o: %.c
# ADT 	@$(CC) -M -MT $@ $(DEFS_RT_P) $< > .$*_p.d
# ADT 	$(CC) -c $(CFLAGS) $(DEFS_RT_P) -o $@ $<
# ADT 
# ADT $(RUNTIME_OBJ_D): %_d.o: %.c
# ADT 	@$(CC) -M -MT $@ $(DEFS_RT_D) $< > .$*_d.d
# ADT 	$(CC) -c $(CFLAGS) $(DEFS_RT_D) -o $@ $<
# ADT 
# ADT distclean: clean
# ADT 	rm -f Makefile config.h
# ADT 
# ADT ifeq ($(BUILD_PARSERS),true)
# ADT EXTRA_CLEAN = $(GEN_SRC)
# ADT endif
# ADT 
# ADT clean:
# ADT 	rm -f tags .*.d *.o colm $(EXTRA_CLEAN) $(RUNTIME_P) $(RUNTIME_D)
# ADT 
# ADT install: all
# ADT 	install -d $(prefix)/bin
# ADT 	install -d $(prefix)/include
# ADT 	install -d $(prefix)/include/colm
# ADT 	install -d $(prefix)/lib
# ADT 	install -s colm $(prefix)/bin/colm
# ADT 	install libcolmp.a libcolmd.a $(prefix)/lib
# ADT 	install $(RUNTIME_HDR) $(prefix)/include/colm
# ADT 
# ADT -include $(DEPS)

