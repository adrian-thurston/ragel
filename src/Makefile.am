COLM        = @COLM@
COLM_LD     = @COLM_LD@
COLM_LIBDEP = @COLM_LIBDEP@
COLM_BINDEP = @COLM_BINDEP@
KELBT       = @KELBT@
RAGEL       = @RAGEL@

bin_PROGRAMS = ragel

lib_LIBRARIES = libragel.a

ragel_CPPFLAGS = -I$(top_srcdir)/aapl -DBINDIR='"@bindir@"'

ragel_SOURCES = \
	buffer.h inputdata.h redfsm.h parsedata.h \
	parsetree.h rlscan.h common.h \
	fsmgraph.h pcheck.h gendata.h ragel.h codegen.h reducer.h \
	binary.h bingoto.h binloopgoto.h binexpgoto.h binvar.h binloopvar.h binexpvar.h \
	flat.h flatloopgoto.h flatexpgoto.h flatvar.h flatloopvar.h flatexpvar.h \
	goto.h switchloopgoto.h switchexpgoto.h \
	ipgoto.h xml.h dot.h asm.h \
	\
	main.cc parsetree.cc parsedata.cc fsmstate.cc fsmbase.cc \
	fsmattach.cc fsmmin.cc fsmgraph.cc fsmap.cc fsmcond.cc fsmnfa.cc \
	inputdata.cc common.cc redfsm.cc gendata.cc allocgen.cc \
	codegen.cc reducer.cc \
	binary.cc binloopgoto.cc binexpgoto.cc binvar.cc binloopvar.cc binexpvar.cc \
	flat.cc flatloopgoto.cc flatexpgoto.cc flatvar.cc flatloopvar.cc flatexpvar.cc \
	goto.cc switchloopgoto.cc switchexpgoto.cc \
	ipgoto.cc xml.cc dot.cc asm.cc

if WITH_RAGEL_KELBT
ragel_SOURCES += rlscan.cc rlparse.cc rlparse.h 
endif

nodist_ragel_SOURCES = version.h

BUILT_SOURCES = version.h

nodist_ragel_SOURCES += \
	parse.c if.h if.cc commit.cc \
	rlhc.c

ragel_SOURCES += \
	load.cc load.h

ragel_LDADD = $(COLM_LD)
ragel_DEPENDENCIES = $(COLM_LIBDEP)

version.h: Makefile
	echo '#define VERSION "$(PACKAGE_VERSION)"' > version.h
	echo '#define PUBDATE "$(PUBDATE)"' >> version.h

libragel_a_SOURCES = $(ragel_SOURCES) $(nodist_ragel_SOURCES) lib.cc
libragel_a_CPPFLAGS = -Dmain=main_lib -I$(top_srcdir)/aapl -DBINDIR='"@bindir@"'

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

CLEANFILES = parse.c if.h if.cc commit.cc

RAGEL_LM = \
	ragel.lm \
	ragel-c.lm \
	ragel-ruby.lm \
	ragel-ocaml.lm \
	ragel-crack.lm

parse.c: rlparse.lm reducer.lm $(RAGEL_LM) $(COLM_BINDEP)
	$(COLM) -c -b rlparse_object -o $@ -e if.h -x if.cc -m commit.cc $<

# These are necessary because we are not using built sources. They create the
# dependency on the colm run for the objects that that require its output. Must
# create them for every target.
ragel-if.$(OBJEXT): parse.c
ragel-load.$(OBJEXT): parse.c
ragel-commit.$(OBJEXT): parse.c
ragel-reducer.$(OBJEXT): parse.c

libragel_a-if.$(OBJEXT): parse.c
libragel_a-load.$(OBJEXT): parse.c
libragel_a-commit.$(OBJEXT): parse.c
libragel_a-reducer.$(OBJEXT): parse.c

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
