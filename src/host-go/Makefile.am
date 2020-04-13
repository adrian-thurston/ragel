bin_PROGRAMS = ragel-go

data_DATA = out-go.lm

ragel_go_CPPFLAGS = -I$(top_srcdir)/aapl -I$(top_srcdir)/ragel -I$(top_srcdir)/colm/include

dist_ragel_go_SOURCES = main.cc rlhc.c

nodist_ragel_go_SOURCES = \
	rlparse.c rlreduce.cc

ragel_go_LDADD = $(LIBFSM_LA) $(LIBCOLM_LA) ../libragel.la 

BUILT_SOURCES = rlparse.c rlreduce.cc rlhc.c

EXTRA_DIST = rlparse.lm rlhc.lm out-go.lm

rlparse.pack: rlparse.lm $(COLM) $(COLM_WRAP)
	$(COLM_WRAP) -c -I .. -b rlparseGo -o $@ -p rlparse.c -m rlreduce.cc $<

rlparse.c: rlparse.pack
	$(COLM_WRAP) -o $@ $<

rlreduce.cc: rlparse.pack
	$(COLM_WRAP) -o $@ $<

rlhc.c: $(COLM_SHARE)/rlhc-go.lm $(COLM_SHARE)/ril.lm $(COLM) $(COLM_WRAP)
	$(COLM) -c -I $(COLM_SHARE) -b rlhcGo -o $@ $<
