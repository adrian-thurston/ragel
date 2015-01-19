#
#   Copyright 2002-2014 Adrian Thurston <thurston@complang.org>
#

COLM = @COLM@

TESTS = runtests

noinst_SCRIPTS = runtests
noinst_PROGRAMS = trans

ragel-parse: ragel-parse.lm
	$(COLM) $<

EXTRA_DIST = \
	ragel-parse.lm  trans-cs.lm  trans-go.lm    trans.lm \
	trans-c.lm      trans-d.lm   trans-java.lm  trans-ruby.lm \
	runtests.sh \
	atoi1.rl clang2.rl cond7.rl element3.rl erract8.rl forder3.rl java1.rl \
	range.rl scan3.rl xml.rl atoi2.rl clang3.rl cppscan1.rl eofact.rl \
	erract9.rl gotocallret1.rl java2.rl recdescent1.rl scan4.rl atoi3.rl \
	clang4.rl cppscan2.rl erract1.rl export1.rl gotocallret2.rl keller1.rl \
	recdescent2.rl stateact1.rl awkemu.rl cond1.rl cppscan3.rl erract2.rl \
	export2.rl high1.rl lmgoto.rl recdescent3.rl statechart1.rl builtin.rl \
	cond2.rl cppscan4.rl erract3.rl export3.rl high2.rl mailbox1.rl \
	repetition.rl strings1.rl call1.rl cond3.rl cppscan5.rl erract4.rl \
	export4.rl high3.rl mailbox2.rl rlscan.rl strings2.rl call2.rl cond4.rl \
	cppscan6.rl erract5.rl fnext1.rl import1.rl mailbox3.rl ruby1.rl \
	tokstart1.rl call3.rl cond5.rl element1.rl erract6.rl forder1.rl \
	include1.rl minimize1.rl scan1.rl union.rl clang1.rl cond6.rl \
	element2.rl erract7.rl forder2.rl include2.rl patact.rl scan2.rl \
	xmlcommon.rl cppscan1.h eofact.h mailbox1.h strings2.h

CLEANFILES = \
	*.c *.cpp *.m *.d *.java *.bin *.class *.exp \
	*.out *_c.rl *_d.rl *_java.rl *_ruby.rl *_csharp.rl *.cs \
	*_go.rl *.go *.exe

TRANS_DEPS = \
	../src/ragel.lm \
	trans-c.lm \
	trans-asm.lm \
	trans-d.lm \
	trans-cs.lm \
	trans-go.lm \
	trans-java.lm \
	trans-ruby.lm \
	trans-ocaml.lm

trans$(EXEEXT): trans.lm $(TRANS_DEPS)
	$(COLM) -o $@ $<

runtests: runtests.sh Makefile
	@$(top_srcdir)/sedsubst $< $@ -w,+x $(SED_SUBST)
