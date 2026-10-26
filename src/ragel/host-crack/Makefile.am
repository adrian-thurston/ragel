if INSTALL_RAGEL
bin_PROGRAMS = ragel-crack
else
noinst_PROGRAMS = ragel-crack
endif

AUTOMAKE_OPTIONS = nostdinc

ragel_crack_CPPFLAGS = -I$(top_srcdir)/src/aapl -I$(top_srcdir)/src/ragel \
	$(LIBFSM_CPPFLAGS) $(LIBCOLM_CPPFLAGS)

dist_ragel_crack_SOURCES = main.cc rlhc.c

nodist_ragel_crack_SOURCES = \
	rlparse.c rlreduce.cc

ragel_crack_LDADD = $(LIBFSM_LA) $(LIBCOLM_LA) ../libragel.la 

BUILT_SOURCES = rlparse.c rlreduce.cc rlhc.c

CLEANFILES = rlparse.pack rlparse.c rlreduce.cc rlhc.c

EXTRA_DIST = rlparse.lm CMakeLists.txt

LM_DEPS = ../ragel.lm ../rlreduce.lm

rlparse.pack: rlparse.lm $(COLM) $(COLM_WRAP) $(LM_DEPS)
	$(COLM_WRAP) -c -I$(top_srcdir)/src/ragel -b rlparseCrack -o $@ -p rlparse.c -m rlreduce.cc $<

rlparse.c: rlparse.pack
	$(COLM_WRAP) -o $@ $<

rlreduce.cc: rlparse.pack
	$(COLM_WRAP) -o $@ $<

rlhc.c: $(COLM_SHARE)/rlhc-crack.lm $(COLM_SHARE)/ril.lm $(COLM_SHARE)/rlhc-main.lm \
	$(COLM) $(COLM_WRAP)
	$(COLM) -c -I $(COLM_SHARE) -b rlhcCrack -o $@ $<
