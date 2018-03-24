COLM        = @COLM@
COLM_LD     = @COLM_LD@
COLM_LIBDEP = @COLM_LIBDEP@
COLM_BINDEP = @COLM_BINDEP@

noinst_PROGRAMS = ragel-d

ragel_d_CPPFLAGS = -I$(top_srcdir)/aapl

dist_ragel_d_SOURCES = main.cc rlhc.c

nodist_ragel_d_SOURCES = \
	rlparse.c rlreduce.cc

ragel_d_LDADD = ../libragel.la ../libfsm.la $(COLM_LD)

BUILT_SOURCES = rlparse.c rlreduce.cc rlhc.c

rlparse.c: rlparse.lm $(COLM_BINDEP)
	$(COLM) -I.. -c -b rl_parse -o $@ -m rlreduce.cc $<

rlreduce.cc: rlparse.c

rlhc.c: rlhc-d.lm
	$(COLM) -I.. -c -b rlhc -o $@ $<
