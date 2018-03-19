COLM        = @COLM@
COLM_LD     = @COLM_LD@
COLM_LIBDEP = @COLM_LIBDEP@
COLM_BINDEP = @COLM_BINDEP@

noinst_PROGRAMS = ragel-julia

ragel_julia_CPPFLAGS = -I$(top_srcdir)/aapl

dist_ragel_julia_SOURCES = main.cc rlhc.c

nodist_ragel_julia_SOURCES = \
	rlparse.c rlreduce.cc

ragel_julia_LDADD = ../libragel.la ../libfsm.la $(COLM_LD)

BUILT_SOURCES = rlparse.c rlreduce.cc rlhc.c

rlparse.c: rlparse.lm $(COLM_BINDEP)
	$(COLM) -I.. -c -b rl_parse -o $@ -m rlreduce.cc $<

rlreduce.cc: rlparse.c

rlhc$(EXEEXT): rlhc-julia.lm
	$(COLM) -I.. -o $@ $<

rlhc.c: rlhc-julia.lm
	$(COLM) -I.. -c -b rlhc -o $@ $<

