COLM_BIN    = ../../colm/colm
COLM_LA     = ../../colm/libcolm.la
COLM_LIBDEP = $(COLM_LA)
COLM_BINDEP = $(COLM_BIN)

bin_PROGRAMS = ragel-asm

ragel_asm_CPPFLAGS = -I$(top_srcdir)/aapl -I$(top_srcdir)/ragel -I$(top_srcdir)/colm/include

dist_ragel_asm_SOURCES = main.cc

nodist_ragel_asm_SOURCES = \
	rlparse.c rlreduce.cc

ragel_asm_LDADD = ../libragel.la ../libfsm.la $(COLM_LA)

BUILT_SOURCES = rlparse.c rlreduce.cc

EXTRA_DIST = rlparse.lm

rlparse.c: rlparse.lm $(COLM_BINDEP)
	$(COLM_BIN) -I.. -c -b rlparseAsm -o $@ -m rlreduce.cc $<

rlreduce.cc: rlparse.c

