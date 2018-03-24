SUBDIRS = . ragel-ruby ragel-asm ragel-julia ragel-ocaml ragel-c \
	ragel-d ragel-csharp ragel-go ragel-java ragel-rust ragel-crack ragel-js

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
	buffer.h codegen.h binary.h bingoto.h bingotoloop.h bingotoexp.h binvar.h \
	binvarloop.h binvarexp.h flat.h flatgotoloop.h flatgotoexp.h flatvar.h \
	flatvarloop.h flatvarexp.h goto.h gotoloop.h gotoexp.h \
	ipgoto.h asm.h \
	idbase.cc fsmstate.cc fsmbase.cc fsmattach.cc fsmmin.cc fsmgraph.cc \
	fsmap.cc fsmcond.cc fsmnfa.cc common.cc redfsm.cc gendata.cc \
	allocgen.cc codegen.cc binary.cc bingotoloop.cc bingotoexp.cc binvar.cc \
	binvarloop.cc binvarexp.cc flat.cc flatgotoloop.cc flatgotoexp.cc \
	flatvar.cc flatvarloop.cc flatvarexp.cc goto.cc gotoloop.cc \
	gotoexp.cc ipgoto.cc dot.cc asm.cc

nodist_libfsm_la_SOURCES = \
	version.h

#
# libragel: ragel program minus host-specific code
#
libragel_la_CPPFLAGS = -I$(top_srcdir)/aapl -DBINDIR='"@bindir@"'

dist_libragel_la_SOURCES = \
	parsedata.h parsetree.h inputdata.h pcheck.h reducer.h rlscan.h load.h \
	parsetree.cc parsedata.cc inputdata.cc load.cc reducer.cc

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
	parse.c rlreduce.cc

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
	rlparse.kl

CLEANFILES = parse.c commit.cc rlhc.c

if WITH_RAGEL_KELBT
CLEANFILES += rlscan.cc rlparse.cc rlparse.h 
endif

RAGEL_LM = \
	rlparse.lm \
	ragel.lm \
	reducer.lm

parse.c: $(RAGEL_LM) $(COLM_BINDEP)
	$(COLM) -c -b rl_parse -o $@ -m rlreduce.cc $<

rlreduce.cc: parse.c

RLHC = \
	rlhc-host.lm \
	rlhc-c.lm \
	rlhc-d.lm \
	rlhc-go.lm \
	rlhc-csharp.lm \
	rlhc-java.lm \
	rlhc-ruby.lm \
	rlhc-rust.lm \
	rlhc-ocaml.lm \
	rlhc-crack.lm \
	rlhc-julia.lm \
	rlhc-js.lm

rlhc.c: rlhc.lm $(RLHC) $(COLM_BINDEP)
	$(COLM) -c -b rlhc_object -o $@ $<

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
