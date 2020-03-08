COLM_BIN    = @COLM@
COLM_WRAP   = @COLM_WRAP@
COLM_LA     = @LIBCOLM@
COLM_LIBDEP = $(COLM_LA)
COLM_BINDEP = $(COLM_BIN) $(COLM_WRAP)

bin_PROGRAMS = ragel-ocaml

ragel_ocaml_CPPFLAGS = -I$(top_srcdir)/aapl -I$(top_srcdir)/ragel -I$(top_srcdir)/colm/include

dist_ragel_ocaml_SOURCES = main.cc rlhc.c

nodist_ragel_ocaml_SOURCES = \
	rlparse.c rlreduce.cc

ragel_ocaml_LDADD = ../libragel.la @LIBCOLM@ $(COLM_LA)

BUILT_SOURCES = rlparse.c rlreduce.cc rlhc.c

EXTRA_DIST = rlparse.lm rlhc.lm

rlparse.pack: rlparse.lm $(COLM_BINDEP)
	$(COLM_WRAP) -c -I .. -b rlparseOCaml -o $@ -p rlparse.c -m rlreduce.cc $<

rlparse.c: rlparse.pack
	$(COLM_WRAP) -o $@ $<

rlreduce.cc: rlparse.pack
	$(COLM_WRAP) -o $@ $<

rlhc.c: $(COLM_SHARE)/rlhc-ocaml.lm $(COLM_SHARE)/ril.lm $(COLM_BINDEP)
	$(COLM_BIN) -c -I $(COLM_SHARE) -b rlhcOCaml -o $@ $<
