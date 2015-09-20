COLM = @COLM@
KELBT = @KELBT@
RAGEL = @RAGEL@

bin_PROGRAMS = ragel

if WITH_COLM
bin_PROGRAMS += rlhc
endif

ragel_CPPFLAGS = -I$(top_srcdir)/aapl -DBINDIR='"@bindir@"'

ragel_SOURCES = \
	buffer.h inputdata.h redfsm.h parsedata.h \
	parsetree.h rlscan.h common.h \
	fsmgraph.h pcheck.h gendata.h ragel.h codegen.h \
	binary.h bingoto.h binloopgoto.h binexpgoto.h binvar.h binloopvar.h binexpvar.h \
	flat.h flatloopgoto.h flatexpgoto.h flatvar.h flatloopvar.h flatexpvar.h \
	goto.h switchloopgoto.h switchexpgoto.h \
	ipgoto.h xml.h dot.h asm.h rlparse.h \
	\
	main.cc parsetree.cc parsedata.cc fsmstate.cc fsmbase.cc \
	fsmattach.cc fsmmin.cc fsmgraph.cc fsmap.cc fsmcond.cc fsmnfa.cc \
	inputdata.cc common.cc redfsm.cc gendata.cc allocgen.cc \
	rlscan.cc rlparse.cc codegen.cc \
	binary.cc binloopgoto.cc binexpgoto.cc binvar.cc binloopvar.cc binexpvar.cc \
	flat.cc flatloopgoto.cc flatexpgoto.cc flatvar.cc flatloopvar.cc flatexpvar.cc \
	goto.cc switchloopgoto.cc switchexpgoto.cc \
	ipgoto.cc xml.cc dot.cc asm.cc

nodist_ragel_SOURCES = version.h

BUILT_SOURCES = version.h

if WITH_COLM
nodist_ragel_SOURCES += \
	parse.c if.h if.cc if.h 

ragel_SOURCES += \
	load.cc load.h

ragel_LDADD = -lcolmd
endif


version.h: Makefile
	echo '#define VERSION "$(PACKAGE_VERSION)"' > version.h
	echo '#define PUBDATE "$(PUBDATE)"' >> version.h

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
	ragel-c.lm \
	ragel-crack.lm \
	ragel-ruby.lm \
	ragel-ocaml.lm \
	rlparse.lm \
	rlscan.rl \
	rlparse.kh \
	rlparse.kl

CLEANFILES = parse.c if.h if.cc

if WITH_COLM

parse.c: ragel-c.lm ragel-ruby.lm ragel-ocaml.lm ragel-crack.lm

parse.c: ragel.lm rlparse.lm
	$(COLM) -c -o parse.c -e if.h -x if.cc rlparse.lm
if.h: parse.c
if.cc: parse.c

ragel-if.$(OBJEXT): if.h if.cc parse.c
ragel-load.$(OBJEXT): if.h if.cc parse.c

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
	rlhc-julia.lm

rlhc_SOURCES =

rlhc$(EXEEXT): rlhc.lm $(RLHC)
	$(COLM) -o $@ $<

endif

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


