bin_PROGRAMS = ragel-java

ragel_java_CPPFLAGS = -I$(top_srcdir)/aapl -I$(top_srcdir)/ragel -I$(top_srcdir)/colm/include

dist_ragel_java_SOURCES = main.cc rlhc.c

nodist_ragel_java_SOURCES = \
	rlparse.c rlreduce.cc

ragel_java_LDADD = $(LIBFSM_LA) $(LIBCOLM_LA) ../libragel.la

BUILT_SOURCES = rlparse.c rlreduce.cc rlhc.c

EXTRA_DIST = rlparse.lm rlhc.lm

rlparse.pack: rlparse.lm $(COLM) $(COLM_WRAP)
	$(COLM_WRAP) -c -I .. -b rlparseJava -o $@ -p rlparse.c -m rlreduce.cc $<

rlparse.c: rlparse.pack
	$(COLM_WRAP) -o $@ $<

rlreduce.cc: rlparse.pack
	$(COLM_WRAP) -o $@ $<

rlhc.c: $(COLM_SHARE)/rlhc-java.lm $(COLM_SHARE)/ril.lm $(COLM) $(COLM_WRAP)
	$(COLM_BIN) -c -I $(COLM_SHARE) -b rlhcJava -o $@ $<
