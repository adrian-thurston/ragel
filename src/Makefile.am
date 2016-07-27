COLM        = @COLM@
COLM_LD     = @COLM_LD@
COLM_LIBDEP = @COLM_LIBDEP@
COLM_BINDEP = @COLM_BINDEP@
KELBT       = @KELBT@
RAGEL       = @RAGEL@

# Libfsm contains only the FSM construction code and the backend code
# generators. It is useful for building code generators in programs not
# connected to the ragel language.
lib_LTLIBRARIES = libfsm.la

if BUILD_PROGRAM

bin_PROGRAMS = ragel

# Libragel includes the ragel parsers. It is the programm wrapped up with a
# libragel call in place of main.
lib_LTLIBRARIES += libragel.la

endif

include_HEADERS = \
	libragel.h action.h fsmgraph.h ragel.h common.h \
	gendata.h redfsm.h dot.h

nodist_include_HEADERS = config.h

ragel_CPPFLAGS = -I$(top_srcdir)/aapl -DBINDIR='"@bindir@"'

LIBFSM_SRC = \
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

ragel_SOURCES = \
	parsedata.h parsetree.h inputdata.h pcheck.h reducer.h rlscan.h load.h \
	parsetree.cc parsedata.cc inputdata.cc load.cc reducer.cc main.cc \
	$(LIBFSM_SRC)

nodist_ragel_SOURCES = version.h
BUILT_SOURCES = version.h

if WITH_RAGEL_KELBT
nodist_ragel_SOURCES += rlscan.cc rlparse.cc rlparse.h 
endif

nodist_ragel_SOURCES += \
	parse.c if.h if.cc commit.cc \
	rlhc.c

ragel_LDADD = $(COLM_LD)
ragel_DEPENDENCIES = $(COLM_LIBDEP)

version.h: Makefile
	echo '#define VERSION "$(PACKAGE_VERSION)"' > version.h
	echo '#define PUBDATE "$(PUBDATE)"' >> version.h

nodist_libragel_la_SOURCES = $(nodist_ragel_SOURCES)
libragel_la_SOURCES = $(ragel_SOURCES)
libragel_la_CPPFLAGS = -Dmain=main_lib -I$(top_srcdir)/aapl -DBINDIR='"@bindir@"'

nodist_libfsm_la_SOURCES = version.h
libfsm_la_SOURCES = $(LIBFSM_SRC)
libfsm_la_CPPFLAGS = -I$(top_srcdir)/aapl

EXTRA_DIST = \
	ragel.lm \
	rlhc.lm \
	rlhc-host.lm \
	rlhc-c.lm \
	rlhc-d.lm \
	rlhc-csharp.lm \
	rlhc-go.lm \
	rlhc-ruby.lm \
	rlhc-java.lm \
	rlhc-rust.lm \
	rlhc-ocaml.lm \
	rlhc-crack.lm \
	rlhc-julia.lm \
	rlhc-js.lm \
	ragel-c.lm \
	ragel-crack.lm \
	ragel-ruby.lm \
	ragel-ocaml.lm \
	rlparse.lm \
	reducer.lm \
	rlscan.rl \
	rlparse.kh \
	rlparse.kl

CLEANFILES = parse.c if.h if.cc commit.cc rlhc.c

if WITH_RAGEL_KELBT
CLEANFILES += rlscan.cc rlparse.cc rlparse.h 
endif

RAGEL_LM = \
	ragel.lm \
	ragel-c.lm \
	ragel-ruby.lm \
	ragel-ocaml.lm \
	ragel-crack.lm

parse.c: rlparse.lm reducer.lm $(RAGEL_LM) $(COLM_BINDEP)
	$(COLM) -c -b rlparse_object -o $@ -e if.h -x if.cc -m commit.cc $<

if.cc: parse.c
if.h: parse.c
commit.cc: parse.c

# These are necessary because we are not using built sources. They create the
# dependency on the colm run for the objects that that require its output. Must
# create them for every target.
ragel-if.$(OBJEXT): parse.c
ragel-load.$(OBJEXT): parse.c
ragel-commit.$(OBJEXT): parse.c
ragel-reducer.$(OBJEXT): parse.c

# Use a var for the LO extension otherwise automake will think that we are
# specifying the build rules for the files. This doesn't appear to be a problem
# for o files.
LOEXT = lo

libragel_la-if.$(LOEXT): parse.c
libragel_la-load.$(LOEXT): parse.c
libragel_la-commit.$(LOEXT): parse.c
libragel_la-reducer.$(LOEXT): parse.c

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
