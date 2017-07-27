#
#   Copyright 2002-2014 Adrian Thurston <thurston@complang.org>
#

#   This file is part of Ragel.
#
#   Ragel is free software; you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation; either version 2 of the License, or
#   (at your option) any later version.
#
#   Ragel is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with Ragel; if not, write to the Free Software
#   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA 

COLM = @COLM@
RAGEL_SRC = @RAGEL_SRC@

TESTS = runtests

noinst_SCRIPTS = runtests subject.mk subject.sh
noinst_PROGRAMS = trans

EXTRA_DIST = \
	trans.lm trans-c.lm trans-asm.lm runtests.sh \
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
	$(RAGEL_SRC)/ragel.lm \
	trans-c.lm trans-asm.lm 

trans_SOURCES = trans.c main.c
trans_LDADD = -lcolm

trans.c: trans.lm $(TRANS_DEPS)
	$(COLM) -c -b trans_object -I$(RAGEL_SRC) -o $@ $<

runtests: runtests.sh Makefile
	@$(top_srcdir)/sedsubst $< $@ -w,+x $(SED_SUBST)

subject.mk: subject.mk.in Makefile
	@$(top_srcdir)/sedsubst $< $@ -w,+x $(SED_SUBST)

subject.sh: subject.sh.in Makefile
	@$(top_srcdir)/sedsubst $< $@ -w,+x $(SED_SUBST)
