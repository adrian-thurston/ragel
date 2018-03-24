COLM        = @COLM@
COLM_LD     = @COLM_LD@
COLM_LIBDEP = @COLM_LIBDEP@
COLM_BINDEP = @COLM_BINDEP@

noinst_PROGRAMS = ragel-ocaml

ragel_ocaml_CPPFLAGS = -I$(top_srcdir)/aapl

dist_ragel_ocaml_SOURCES = main.cc rlhc.c

nodist_ragel_ocaml_SOURCES = \
	rlparse.c rlreduce.cc

ragel_ocaml_LDADD = ../libragel.la ../libfsm.la $(COLM_LD)

BUILT_SOURCES = rlparse.c rlreduce.cc rlhc.c

rlparse.c: rlparse.lm $(COLM_BINDEP)
	$(COLM) -I.. -c -b rl_parse -o $@ -m rlreduce.cc $<

rlreduce.cc: rlparse.c

rlhc$(EXEEXT): rlhc-ocaml.lm
	$(COLM) -I.. -o $@ $<

rlhc.c: rlhc-ocaml.lm
	$(COLM) -I.. -c -b rlhc -o $@ $<


