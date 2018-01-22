COLM        = @COLM@
COLM_LD     = @COLM_LD@
COLM_LIBDEP = @COLM_LIBDEP@
COLM_BINDEP = @COLM_BINDEP@

noinst_PROGRAMS = rbrl

rbrl_CPPFLAGS = -I$(top_srcdir)/aapl

dist_rbrl_SOURCES = main.cc

nodist_rbrl_SOURCES = \
	rlparse.c rlreduce.cc

rbrl_LDADD = ../libragel.la ../libfsm.la $(COLM_LD)

BUILT_SOURCES = rlparse.c rlreduce.cc

rlparse.c: ruby.lm $(COLM_BINDEP)
	$(COLM) -I.. -c -b rl_parse -o $@ -m rlreduce.cc $<

rlreduce.cc: rlparse.c
