COLM_BIN    = ../../colm/colm
COLM_LA     = ../../colm/libcolm.la
COLM_LIBDEP = $(COLM_LA)
COLM_BINDEP = $(COLM_BIN)

bin_PROGRAMS = ragel-c

ragel_c_CPPFLAGS = -I$(top_srcdir)/aapl -I$(top_srcdir)/ragel -I$(top_srcdir)/colm/include

dist_ragel_c_SOURCES = main.cc rlhc.c

nodist_ragel_c_SOURCES = \
	rlparse.c rlreduce.cc

ragel_c_LDADD = ../libragel.la ../libfsm.la $(COLM_LA)

BUILT_SOURCES = rlparse.c rlreduce.cc

EXTRA_DIST = rlparse.lm rlhc.lm

rlparse.c: rlparse.lm $(COLM_BINDEP)
	$(COLM_BIN) -I.. -c -b rlparseC -o $@ -m rlreduce.cc $<

rlreduce.cc: rlparse.c

rlhc.c: rlhc.lm ../ril.lm $(COLM_BINDEP)
	$(COLM_BIN) -I.. -c -b rlhcC -o $@ $<
