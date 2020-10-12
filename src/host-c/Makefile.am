bin_PROGRAMS = ragel-c

ragel_c_CPPFLAGS = -I$(top_srcdir)/aapl -I$(top_srcdir)/ragel -I$(top_srcdir)/colm/include

dist_ragel_c_SOURCES = main.cc rlhc.c

nodist_ragel_c_SOURCES = \
	rlparse.c rlreduce.cc

ragel_c_LDADD = $(LIBFSM_LA) $(LIBCOLM_LA) ../libragel.la

BUILT_SOURCES = rlparse.c rlreduce.cc

EXTRA_DIST = rlparse.lm

rlparse.pack: rlparse.lm $(COLM) $(COLM_WRAP)
	$(COLM_WRAP) -c -I .. -b rlparseC -o $@ -p rlparse.c -m rlreduce.cc $<

rlparse.c: rlparse.pack
	$(COLM_WRAP) -o $@ $<

rlreduce.cc: rlparse.pack
	$(COLM_WRAP) -o $@ $<

rlhc.c: $(COLM_SHARE)/rlhc-c.lm $(COLM_SHARE)/ril.lm $(COLM_BINDEP)
	$(COLM) -c -I $(COLM_SHARE) -b rlhcC -o $@ $<
