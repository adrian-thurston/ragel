COLM        = @COLM@
COLM_LD     = @COLM_LD@
COLM_LIBDEP = @COLM_LIBDEP@
COLM_BINDEP = @COLM_BINDEP@

noinst_PROGRAMS = ragel-rust

ragel_rust_CPPFLAGS = -I$(top_srcdir)/aapl

dist_ragel_rust_SOURCES = main.cc rlhc.c

nodist_ragel_rust_SOURCES = \
	rlparse.c rlreduce.cc

ragel_rust_LDADD = ../libragel.la ../libfsm.la $(COLM_LD)

BUILT_SOURCES = rlparse.c rlreduce.cc rlhc.c

rlparse.c: rlparse.lm $(COLM_BINDEP)
	$(COLM) -I.. -c -b rl_parse -o $@ -m rlreduce.cc $<

rlreduce.cc: rlparse.c

rlhc.c: rlhc-rust.lm
	$(COLM) -I.. -c -b rlhc -o $@ $<
