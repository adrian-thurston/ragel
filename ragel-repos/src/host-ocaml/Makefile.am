COLM        = @COLM@
COLM_LD     = @COLM_LD@
COLM_LIBDEP = @COLM_LIBDEP@
COLM_BINDEP = @COLM_BINDEP@

bin_PROGRAMS = ragel-ocaml

ragel_ocaml_CPPFLAGS = -I$(top_srcdir)/aapl

dist_ragel_ocaml_SOURCES = main.cc rlhc.c

nodist_ragel_ocaml_SOURCES = \
	rlparse.c rlreduce.cc

ragel_ocaml_LDADD = ../libragel.la ../libfsm.la $(COLM_LD)

BUILT_SOURCES = rlparse.c rlreduce.cc rlhc.c

EXTRA_DIST = rlparse.lm rlhc.lm

rlparse.c: rlparse.lm $(COLM_BINDEP)
	$(COLM) -I.. -c -b rlparseOCaml -o $@ -m rlreduce.cc $<

rlreduce.cc: rlparse.c

rlhc$(EXEEXT): rlhc.lm
	$(COLM) -I.. -o $@ $<

rlhc.c: rlhc.lm
	$(COLM) -I.. -c -b rlhcOCaml -o $@ $<


