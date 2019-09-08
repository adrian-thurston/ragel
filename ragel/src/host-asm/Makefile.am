COLM        = @COLM@
COLM_LD     = @COLM_LD@
COLM_LIBDEP = @COLM_LIBDEP@
COLM_BINDEP = @COLM_BINDEP@

bin_PROGRAMS = ragel-asm

ragel_asm_CPPFLAGS = -I$(top_srcdir)/aapl

dist_ragel_asm_SOURCES = main.cc

nodist_ragel_asm_SOURCES = \
	rlparse.c rlreduce.cc

ragel_asm_LDADD = ../libragel.la ../libfsm.la $(COLM_LD)

BUILT_SOURCES = rlparse.c rlreduce.cc

EXTRA_DIST = rlparse.lm

rlparse.c: rlparse.lm $(COLM_BINDEP)
	$(COLM) -I.. -c -b rlparseAsm -o $@ -m rlreduce.cc $<

rlreduce.cc: rlparse.c

