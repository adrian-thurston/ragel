COLM        = @COLM@
COLM_LD     = @COLM_LD@
COLM_LIBDEP = @COLM_LIBDEP@
COLM_BINDEP = @COLM_BINDEP@

bin_PROGRAMS = ragel-c

ragel_c_CPPFLAGS = -I$(top_srcdir)/aapl

dist_ragel_c_SOURCES = main.cc rlhc.c

nodist_ragel_c_SOURCES = \
	rlparse.c rlreduce.cc

ragel_c_LDADD = ../libragel.la ../libfsm.la $(COLM_LD)

BUILT_SOURCES = rlparse.c rlreduce.cc

EXTRA_DIST = rlparse.lm rlhc-c.lm

rlparse.c: rlparse.lm $(COLM_BINDEP)
	$(COLM) -I.. -c -b rl_parse -o $@ -m rlreduce.cc $<

rlreduce.cc: rlparse.c

rlhc.c: rlhc-c.lm ../ril.lm
	$(COLM) -I.. -c -b rlhc -o $@ $<
