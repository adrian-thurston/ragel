SUBDIRS = . host-ruby host-asm host-julia host-ocaml host-c \
	host-d host-csharp host-go host-java host-rust host-crack host-js

COLM        = @COLM@
COLM_LD     = @COLM_LD@
COLM_LIBDEP = @COLM_LIBDEP@
COLM_BINDEP = @COLM_BINDEP@
KELBT       = @KELBT@
RAGEL       = @RAGEL@

# libfsm contains only the FSM construction code and the backend code
# generators. It is useful for building code generators in programs not
# connected to the ragel language.
#
# libragel contains the parse tree and other parsing support code. Everything
# except the reducers, which are specific to the frontends.
lib_LTLIBRARIES = libfsm.la libragel.la

if BUILD_PROGRAM

bin_PROGRAMS = ragel

endif

pkginclude_HEADERS = \
	action.h fsmgraph.h ragel.h common.h \
	gendata.h redfsm.h dot.h

nodist_pkginclude_HEADERS = config.h

data_DATA = ragel.lm

#
# libfsm: state machine construction and direct code generation.
#
libfsm_la_CPPFLAGS = -I$(top_srcdir)/aapl

dist_libfsm_la_SOURCES = \
	buffer.h codegen.h \
	actloop.h actexp.h \
	tables.h \
	binary.h bingoto.h binbreak.h binvar.h \
	flat.h flatgoto.h flatbreak.h flatvar.h \
	switch.h switchgoto.h switchbreak.h switchvar.h \
	goto.h gotoloop.h gotoexp.h \
	ipgoto.h asm.h \
	idbase.cc fsmstate.cc fsmbase.cc fsmattach.cc fsmmin.cc fsmgraph.cc \
	fsmap.cc fsmcond.cc fsmnfa.cc common.cc redfsm.cc gendata.cc \
	allocgen.cc codegen.cc \
	actexp.cc binvar.cc \
	tables.cc tabgoto.cc tabbreak.cc tabvar.cc \
	binary.cc bingoto.cc binbreak.cc actloop.cc \
	flat.cc flatgoto.cc flatbreak.cc flatvar.cc \
	switch.cc switchgoto.cc switchbreak.cc switchvar.cc \
	goto.cc gotoloop.cc gotoexp.cc ipgoto.cc \
	dot.cc asm.cc

nodist_libfsm_la_SOURCES = \
	version.h

libfsm_la_LDFLAGS = -no-undefined

#
# libragel: ragel program minus host-specific code
#
libragel_la_CPPFLAGS = -I$(top_srcdir)/aapl -DBINDIR='"@bindir@"'

dist_libragel_la_SOURCES = \
	parsedata.h parsetree.h inputdata.h pcheck.h reducer.h rlscan.h load.h \
	parsetree.cc longest.cc parsedata.cc inputdata.cc load.cc reducer.cc

libragel_la_LDFLAGS = -no-undefined
libragel_la_LIBADD = libfsm.la $(COLM_LD)

if LINKER_NO_UNDEFINED
libfsm_la_LDFLAGS += -Wl,--no-undefined
libragel_la_LDFLAGS += -Wl,--no-undefined
endif

#
# ragel program.
#
ragel_CPPFLAGS = -I$(top_srcdir)/aapl

dist_ragel_SOURCES = \
	main.cc

nodist_ragel_SOURCES = \
	parse.c rlreduce.cc

ragel_LDADD = libragel.la libfsm.la $(COLM_LD)
ragel_DEPENDENCIES = libragel.la libfsm.la $(COLM_LIBDEP)

BUILT_SOURCES = \
	version.h \
	parse.c rlreduce.cc \
	include/ragel

include/ragel:
	mkdir -p include
	ln -s .. include/ragel

if WITH_RAGEL_KELBT
nodist_ragel_SOURCES += \
	rlscan.cc rlparse.cc rlparse.h 

BUILT_SOURCES += \
	rlscan.cc rlparse.cc rlparse.h 
endif

version.h: Makefile
	echo '#define VERSION "$(PACKAGE_VERSION)"' > version.h
	echo '#define PUBDATE "$(PUBDATE)"' >> version.h

EXTRA_DIST = \
	$(RAGEL_LM) \
	rlscan.rl \
	rlparse.kh \
	rlparse.kl \
	ril.lm  \
	rlhc-main.lm

CLEANFILES = parse.c commit.cc rlhc.c

if WITH_RAGEL_KELBT
CLEANFILES += rlscan.cc rlparse.cc rlparse.h 
endif

RAGEL_LM = \
	rlparse.lm \
	ragel.lm \
	rlreduce.lm

parse.c: $(RAGEL_LM) $(COLM_BINDEP)
	$(COLM) -c -b rlparseC -o $@ -m rlreduce.cc $<

rlreduce.cc: parse.c


if WITH_RAGEL_KELBT

# This dependency comes from the import of the parser defines into the scanner.
rlscan.cc: rlparse.h
inputdata.cc: rlparse.h

ragel-rlscan.$(OBJEXT): rlscan.cc

rlscan.cc: rlscan.rl
	$(RAGEL) -G2 -I$(builddir) -o $@ $<

rlparse.h: rlparse.kh
	$(KELBT) -o $@ $<

rlparse.cc: rlparse.kl rlparse.kh
	$(KELBT) -o $@ $<

endif
