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

COLM_BIN = @COLM@
RAGEL_LM = ../../ragel

COLM_xCPPFLAGS = # -I../../colm/include
COLM_xLDFLAGS = # -L../../colm

TESTS = gentests

noinst_SCRIPTS = gentests
noinst_PROGRAMS = trans

EXTRA_DIST = \
	gentests.sh trans.lm \
	trans-asm.lm     trans-d.lm      trans-ocaml.lm \
	trans-c.lm       trans-go.lm     trans-ruby.lm \
	trans-crack.lm   trans-java.lm   trans-rust.lm \
	trans-csharp.lm  trans-julia.lm \
	any1.rl args1.rl args2.rl argsinc.rl atoi1.rl atoi2.rl atoi3.rl \
	atoi4.rl atoi5.rl awkemu.rl buffer.h builtin.rl call1.rl call2.rl \
	call3.rl call4.rl caseindep.rl clang1.rl clang2.rl clang3.rl \
	clang4.rl clang5.rl cond10.rl cond11.rl cond1.rl cond2.rl cond3.rl \
	cond4.rl cond5.rl cond6.rl cond7.rl cond8.rl cond9.rl conderr1.rl \
	conderr2.rl condrep1.rl condrep2.rl condrep3.rl condrep4.rl condrep5.rl \
	cppscan1.h cppscan1.rl cppscan2.rl cppscan3.rl cppscan4.rl cppscan5.rl \
	cppscan6.rl crack1.rl curs1.rl element1.rl element2.rl element3.rl \
	empty1.rl eofact.h eofact.rl eofcall1.rl eofcall2.rl eofgoto1.rl \
	eofgoto2.rl eofret1.rl erract1.rl erract2.rl erract3.rl erract4.rl \
	erract5.rl erract6.rl erract7.rl erract8.rl erract9.rl export1.rl \
	export2.rl export3.rl export4.rl fnext1.rl fnext2.rl fnext3.rl forder1.rl \
	forder2.rl forder3.rl genrep1.rl genrep2.rl genrep3.rl genrep4.rl \
	genrep5.rl genrep6.rl genrep7.rl genrep8.rl goto1.rl gotocallret1.rl \
	gotocallret2.rl gotocallret3.rl high1.rl high2.rl high3.rl import1.rl \
	import2.h import2.rl include1.rl include2.rl include3.rl \
	include3/smtp_address.rl include3/smtp_addr_parser.rl \
	include3/smtp_ip.rl include3/smtp_whitespace.rl \
	java1.rl java2.rl julia1.rl keller1.rl lmgoto.rl lmnfa1.rl mailbox1.h \
	mailbox1.rl mailbox2.rl mailbox3.rl minimize1.rl ncall1.rl next1.rl \
	next2.rl nfa1.rl nfa2.rl nfa3.rl noignore.rl patact.rl rangei.rl \
	range.rl recdescent1.rl recdescent2.rl recdescent4.rl recdescent5.rl \
	repetition.rl rlscan.rl rpn1.rl ruby1.rl rust1.rl scan1.rl scan2.rl \
	scan3.rl scan4.rl scan5.rl scan6.rl scan7.rl stateact1.rl \
	statechart1.rl strings1.rl strings2.h strings2.rl strings3.rl targs1.rl \
	tofrom1.rl tofrom2.rl tokstart1.rl union.rl url1.rl xmlcommon.rl xml.rl \
	zlen1.rl

CLEANFILES = working

TRANS_DEPS = \
	$(RAGEL_LM)/ragel.lm \
	trans-c.lm trans-asm.lm \
	trans-crack.lm trans-csharp.lm \
	trans-d.lm trans-go.lm \
	trans-java.lm trans-julia.lm \
	trans-ocaml.lm trans-ruby.lm \
	trans-rust.lm

trans_CPPFLAGS = $(COLM_xCPPFLAGS)
trans_SOURCES = trans.c main.c
trans_LDADD = -lcolm
trans_LDFLAGS = $(COLM_xLDFLAGS)

trans.c: trans.lm $(TRANS_DEPS) $(COLM_BIN)
	$(COLM_BIN) -c -b trans_object -I$(RAGEL_LM) -I../../ragel/host-go -o $@ $<

gentests: gentests.sh Makefile
	@$(top_srcdir)/sedsubst $< $@ -w,+x $(SED_SUBST)

