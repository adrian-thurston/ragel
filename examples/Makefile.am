#
#   Copyright 2002-2009 Adrian Thurston <thurston@complang.org>
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

RAGEL = ../ragel/ragel
FLEX = flex
RE2C = re2c

noinst_PROGRAMS = \
	atoi awkemu clang concurrent cppscan format gotocallret mailbox params \
	pullscan rlscan statechart 

EXTRA_DIST = \
	gotocallret.rl pullscan.rl concurrent.rl rlscan.rl statechart.rl \
	params.rl clang.rl cppscan.rl format.rl awkemu.rl mailbox.rl atoi.rl

gotocallret_SOURCES = gotocallret.cpp
pullscan_SOURCES = pullscan.c
concurrent_SOURCES = concurrent.cpp
rlscan_SOURCES = rlscan.cpp
statechart_SOURCES = statechart.cpp 
params_SOURCES = params.c
clang_SOURCES = clang.c 
cppscan_SOURCES = cppscan.cpp 
format_SOURCES = format.c
awkemu_SOURCES = awkemu.c
mailbox_SOURCES = mailbox.cpp
atoi_SOURCES = atoi.cpp

gotocallret.cpp: gotocallret.rl
	$(RAGEL) -G2 -o gotocallret.cpp gotocallret.rl 

pullscan.c: pullscan.rl $(RAGEL) 
	$(RAGEL) -G2 -o $@ pullscan.rl

concurrent.cpp: concurrent.rl $(RAGEL)
	$(RAGEL) -G2 -o concurrent.cpp concurrent.rl

rlscan.cpp: rlscan.rl 
	$(RAGEL) -G2 -o rlscan.cpp rlscan.rl

statechart.cpp: statechart.rl 
	$(RAGEL) -G2 -o statechart.cpp statechart.rl

params.c: params.rl
	$(RAGEL) -G2 -o params.c params.rl

clang.c: clang.rl 
	$(RAGEL) -G2 -o clang.c clang.rl

cppscan.cpp: cppscan.rl 
	$(RAGEL) -G2 -o $@ cppscan.rl

format.c: format.rl
	$(RAGEL) -G2 -o format.c format.rl

awkemu.c: awkemu.rl
	$(RAGEL) -G2 -o awkemu.c awkemu.rl

mailbox.cpp: mailbox.rl
	$(RAGEL) -G2 -o mailbox.cpp mailbox.rl

atoi.cpp: atoi.rl
	$(RAGEL) -G2 -o atoi.cpp atoi.rl

###

lex-cppscan.cpp: cppscan.lex
	$(FLEX) -f -o $@ $<

re2c-cppscan.cpp: cppscan.rec
	$(RE2C) -s $< > $@

example.cpp: example.rec
	$(RE2C) -s $< > $@
