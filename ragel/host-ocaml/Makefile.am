COLM_BIN    = @COLM_BIN@
COLM_LA     = @COLM_LA@
COLM_LIBDEP = @COLM_LIBDEP@
COLM_BINDEP = @COLM_BINDEP@

bin_PROGRAMS = ragel-ocaml

ragel_ocaml_CPPFLAGS = -I$(top_srcdir)/aapl -I$(top_srcdir)/ragel -I$(top_srcdir)/colm/include

dist_ragel_ocaml_SOURCES = main.cc rlhc.c

nodist_ragel_ocaml_SOURCES = \
	rlparse.c rlreduce.cc

ragel_ocaml_LDADD = ../libragel.la ../libfsm.la $(COLM_LA)

BUILT_SOURCES = rlparse.c rlreduce.cc rlhc.c

EXTRA_DIST = rlparse.lm rlhc.lm

rlparse.c: rlparse.lm $(COLM_BINDEP)
	$(COLM_BIN) -I.. -c -b rlparseOCaml -o $@ -m rlreduce.cc $<

rlreduce.cc: rlparse.c

rlhc$(EXEEXT): rlhc.lm
	$(COLM_BIN) -I.. -o $@ $<

rlhc.c: rlhc.lm
	$(COLM_BIN) -I.. -c -b rlhcOCaml -o $@ $<


