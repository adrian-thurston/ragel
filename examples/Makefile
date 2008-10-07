#
#   Copyright 2002-2007 Adrian Thurston <thurston@complang.org>
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

TARGS = \
	atoi awkemu clang concurrent cppscan format gotocallret mailbox params \
	pullscan rlscan statechart 

#########################################

all: $(TARGS)

distclean clean:
	rm -Rf *.o *.c *.cpp $(TARGS)

%.o: %.c
	gcc $(CFLAGS) -Wall -g -c -O3 -o $@ $<

%.o: %.cpp
	g++ $(CXXFLAGS) -Wall -g -c -O3 -o $@ $<

#########################################

gotocallret: gotocallret.o
	g++ -g -o gotocallret gotocallret.o 

gotocallret.cpp: gotocallret.rl
	$(RAGEL) -G2 -o gotocallret.cpp gotocallret.rl 

gotocallret.o: gotocallret.cpp
	g++ -Wall -g -c -O3 -o $@ $<


pullscan: pullscan.o
	gcc -g -o $@ $<

pullscan.c: pullscan.rl $(RAGEL) 
	$(RAGEL) -G2 -o $@ pullscan.rl

concurrent: concurrent.o
	g++ -g -o concurrent concurrent.o 

concurrent.cpp: concurrent.rl $(RAGEL)
	$(RAGEL) -G2 -o concurrent.cpp concurrent.rl

rlscan: rlscan.o
	g++ -g -o rlscan rlscan.o 

rlscan.cpp: rlscan.rl 
	$(RAGEL) -G2 -o rlscan.cpp rlscan.rl

statechart: statechart.o
	g++ -g -o statechart statechart.o 

statechart.cpp: statechart.rl 
	$(RAGEL) -G2 -o statechart.cpp statechart.rl

statechart.o: statechart.cpp
	g++ -Wall -g -c -O3 -o $@ $<

params: params.o 
	gcc -g -o params params.o

params.c: params.rl
	$(RAGEL) -G2 -o params.c params.rl

clang: clang.o 
	gcc -g -o clang clang.o

clang.c: clang.rl 
	$(RAGEL) -G2 -o clang.c clang.rl

optional: lex-cppscan re2c-cppscan

cppscan: cppscan.o
	g++ -g -o $@ $<

lex-cppscan: lex-cppscan.o
	g++ -g -o $@ $<
	
re2c-cppscan: re2c-cppscan.o
	g++ -g -o $@ $<
	
cppscan.cpp: cppscan.rl 
	$(RAGEL) -G2 -o $@ cppscan.rl

lex-cppscan.cpp: cppscan.lex
	$(FLEX) -f -o $@ $<

re2c-cppscan.cpp: cppscan.rec
	$(RE2C) -s $< > $@

example.cpp: example.rec
	$(RE2C) -s $< > $@

format: format.o 
	gcc -g -o format format.o

format.c: format.rl
	$(RAGEL) -G2 -o format.c format.rl

awkemu: awkemu.o
	gcc -g -o awkemu awkemu.o

awkemu.c: awkemu.rl
	$(RAGEL) -G2 -o awkemu.c awkemu.rl


mailbox: mailbox.o
	g++ -g -o mailbox mailbox.o 

mailbox.cpp: mailbox.rl
	$(RAGEL) -G2 -o mailbox.cpp mailbox.rl

atoi: atoi.o
	g++ -g -o atoi atoi.o 

atoi.cpp: atoi.rl
	$(RAGEL) -G2 -o atoi.cpp atoi.rl

atoi.o: atoi.cpp
	g++ -Wall -g -c -O3 -o $@ $<
