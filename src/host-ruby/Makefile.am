COLM        = @COLM@
COLM_LD     = @COLM_LD@
COLM_LIBDEP = @COLM_LIBDEP@
COLM_BINDEP = @COLM_BINDEP@

noinst_PROGRAMS = rlrb rlhc

rlrb_CPPFLAGS = -I$(top_srcdir)/aapl

dist_rlrb_SOURCES = main.cc

nodist_rlrb_SOURCES = \
	rlparse.c rlreduce.cc

rlrb_LDADD = ../libragel.la ../libfsm.la $(COLM_LD)

BUILT_SOURCES = rlparse.c rlreduce.cc

rlparse.c: rlrb.lm $(COLM_BINDEP)
	$(COLM) -I.. -c -b rl_parse -o $@ -m rlreduce.cc $<

rlreduce.cc: rlparse.c

rlhc$(EXEEXT): rlhc.lm
	$(COLM) -I .. -o $@ $<

