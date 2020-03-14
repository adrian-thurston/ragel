COLM_BIN    = @COLM@
COLM_WRAP   = @COLM@-wrap
COLM_LA     = @LIBCOLM@
COLM_LIBDEP = $(COLM_LA)
COLM_BINDEP = $(COLM_BIN) $(COLM_WRAP)

bin_PROGRAMS = ragel-rust

ragel_rust_CPPFLAGS = -I$(top_srcdir)/aapl -I$(top_srcdir)/ragel -I$(top_srcdir)/colm/include

dist_ragel_rust_SOURCES = main.cc rlhc.c

nodist_ragel_rust_SOURCES = \
	rlparse.c rlreduce.cc

ragel_rust_LDADD = ../libragel.la @LIBCOLM@ $(COLM_LA)

BUILT_SOURCES = rlparse.c rlreduce.cc rlhc.c

EXTRA_DIST = rlparse.lm rlhc.lm

rlparse.pack: rlparse.lm $(COLM_BINDEP)
	$(COLM_WRAP) -c -I .. -b rlparseRust -o $@ -p rlparse.c -m rlreduce.cc $<

rlparse.c: rlparse.pack
	$(COLM_WRAP) -o $@ $<

rlreduce.cc: rlparse.pack
	$(COLM_WRAP) -o $@ $<

rlhc.c: $(COLM_SHARE)/rlhc-rust.lm $(COLM_SHARE)/ril.lm $(COLM_BINDEP)
	$(COLM_BIN) -c -I $(COLM_SHARE) -b rlhcRust -o $@ $<
