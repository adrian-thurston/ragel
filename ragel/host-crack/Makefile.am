COLM        = @COLM@
COLM_LD     = @COLM_LD@
COLM_LIBDEP = @COLM_LIBDEP@
COLM_BINDEP = @COLM_BINDEP@

bin_PROGRAMS = ragel-crack

ragel_crack_CPPFLAGS = -I$(top_srcdir)/aapl

dist_ragel_crack_SOURCES = main.cc rlhc.c

nodist_ragel_crack_SOURCES = \
	rlparse.c rlreduce.cc

ragel_crack_LDADD = ../libragel.la ../libfsm.la $(COLM_LD)

BUILT_SOURCES = rlparse.c rlreduce.cc rlhc.c

EXTRA_DIST = rlparse.lm rlhc.lm

rlparse.c: rlparse.lm $(COLM_BINDEP)
	$(COLM) -I.. -c -b rlparseCrack -o $@ -m rlreduce.cc $<

rlreduce.cc: rlparse.c

rlhc.c: rlhc.lm
	$(COLM) -I.. -c -b rlhcCrack -o $@ $<
