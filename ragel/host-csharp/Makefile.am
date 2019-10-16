COLM_BIN    = ../../colm/colm
COLM_LA     = ../../colm/libcolm.la
COLM_LIBDEP = $(COLM_LA)
COLM_BINDEP = $(COLM_BIN)

bin_PROGRAMS = ragel-csharp

ragel_csharp_CPPFLAGS = -I$(top_srcdir)/aapl -I$(top_srcdir)/ragel -I$(top_srcdir)/colm/include

dist_ragel_csharp_SOURCES = main.cc rlhc.c

nodist_ragel_csharp_SOURCES = \
	rlparse.c rlreduce.cc

ragel_csharp_LDADD = ../libragel.la ../libfsm.la $(COLM_LA)

BUILT_SOURCES = rlparse.c rlreduce.cc rlhc.c

EXTRA_DIST = rlparse.lm rlhc.lm

rlparse.c: rlparse.lm $(COLM_BINDEP)
	$(COLM_BIN) -I.. -c -b rlparseCSharp -o $@ -m rlreduce.cc $<

rlreduce.cc: rlparse.c

rlhc.c: rlhc.lm ../ril.lm $(COLM_BINDEP)
	$(COLM_BIN) -I.. -c -b rlhcCSharp -o $@ $<
