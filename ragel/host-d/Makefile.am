COLM_BIN    = ../../colm/colm
COLM_PACK   = ../../colm/colm-pack
COLM_LA     = ../../colm/libcolm.la
COLM_LIBDEP = $(COLM_LA)
COLM_BINDEP = $(COLM_BIN) $(COLM_PACK)

bin_PROGRAMS = ragel-d

ragel_d_CPPFLAGS = -I$(top_srcdir)/aapl -I$(top_srcdir)/ragel -I$(top_srcdir)/colm/include

dist_ragel_d_SOURCES = main.cc rlhc.c

nodist_ragel_d_SOURCES = \
	rlparse.c rlreduce.cc

ragel_d_LDADD = ../libragel.la ../libfsm.la $(COLM_LA)

BUILT_SOURCES = rlparse.c rlreduce.cc rlhc.c

EXTRA_DIST = rlparse.lm rlhc.lm

rlparse.pack: rlparse.lm $(COLM_BINDEP)
	$(COLM_PACK) colm -p rlparse.pack \
		-I.. -c -b rlparseD -o rlparse.c -m rlreduce.cc $<

rlparse.c: rlparse.pack
	$(COLM_PACK) unpack -o $@ $<

rlreduce.cc: rlparse.pack
	$(COLM_PACK) unpack -o $@ $<

rlhc.c: rlhc.lm ../ril.lm $(COLM_BINDEP)
	$(COLM_BIN) -I.. -c -b rlhcD -o $@ $<
