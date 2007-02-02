#
#   Copyright 2002-2007 Adrian Thurston <thurston@cs.queensu.ca>
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

RAGEL = ../../ragel/ragel
RLCODEGEN = ../../rlcodegen/rlcodegen
FLEX = flex
RE2C = re2c
CFLAGS = -Wall -g -O3


TARGS = \
	atoi awkemu clang concurrent cppscan format gotocallret mailbox params \
	pullscan rlscan statechart 

all: $(TARS)

gotocallret: gotocallret.o
	g++ -g -o gotocallret gotocallret.o 

gotocallret.cpp: gotocallret.rl $(RAGEL) $(RLCODEGEN)
	$(RAGEL) gotocallret.rl | $(RLCODEGEN) -G2 -o gotocallret.cpp

gotocallret.o: gotocallret.cpp
	g++ -Wall -g -c -O3 -o $@ $<


pullscan: pullscan.o
	g++ -g -o $@ $<

pullscan.c: pullscan.rl $(RAGEL) $(RLCODEGEN)
	$(RAGEL) pullscan.rl | $(RLCODEGEN) -G2 -o $@

%.o: %.c
	gcc $(CFLAGS) -c -o $@ $<

pullscan.ps: pullscan.rl $(RAGEL) $(RLCODEGEN)
	$(RAGEL) pullscan.rl | $(RLCODEGEN) -V | dot -Tps > pullscan.ps

distclean clean:
	rm -Rf *.o pullscan.c pullscan pullscan.ps


concurrent: concurrent.o
	g++ -g -o concurrent concurrent.o 

concurrent.cpp: concurrent.rl $(RAGEL) $(RLCODEGEN)
	$(RAGEL) concurrent.rl | $(RLCODEGEN) -G2 -o concurrent.cpp

concurrent.ps: concurrent.rl $(RAGEL) $(RLCODEGEN)
	$(RAGEL) concurrent.rl | $(RLCODEGEN) -V | dot -Tps > concurrent.ps

%.o: %.cpp
	g++ -Wall -g -c -O3 -o $@ $<

rlscan: rlscan.o
	g++ -g -o rlscan rlscan.o 

rlscan.cpp: rlscan.rl $(RAGEL) $(RLCODEGEN)
	$(RAGEL) rlscan.rl | $(RLCODEGEN) -G2 -o rlscan.cpp

%.o: %.cpp
	g++ -Wall -g -c -O3 -o $@ $<

rlscan.ps: rlscan.rl $(RAGEL) $(RLCODEGEN)
	$(RAGEL) rlscan.rl | $(RLCODEGEN) -V | dot -Tps > rlscan.ps

statechart: statechart.o
	g++ -g -o statechart statechart.o 

statechart.cpp: statechart.rl $(RAGEL) $(RLCODEGEN)
	$(RAGEL) statechart.rl | $(RLCODEGEN) -G2 -o statechart.cpp

statechart.o: statechart.cpp
	g++ -Wall -g -c -O3 -o $@ $<

statechart.ps: statechart.rl $(RAGEL) $(RLCODEGEN)
	$(RAGEL) statechart.rl | $(RLCODEGEN) -V | dot -Tps > statechart.ps

params: params.o 
	gcc -g -o params params.o

params.c: params.rl $(RAGEL) $(RLCODEGEN)
	$(RAGEL) params.rl | $(RLCODEGEN) -G2 -o params.c

params.ps: params.rl $(RAGEL) $(RLCODEGEN)
	$(RAGEL) params.rl | $(RLCODEGEN) -V | dot -Tps > params.ps

%.o: %.c
	gcc -Wall -O3 -g -c -o $@ $<

clang: clang.o 
	gcc -g -o clang clang.o

clang.c: clang.rl $(RAGEL) $(RLCODEGEN)
	$(RAGEL) clang.rl | $(RLCODEGEN) -G2 -o clang.c

clang.ps: clang.rl $(RAGEL) $(RLCODEGEN)
	$(RAGEL) clang.rl | $(RLCODEGEN) -V | dot -Tps > clang.ps

%.o: %.c
	gcc -pedantic -Wall -O3 -g -c -o $@ $<

optional: lex-cppscan re2c-cppscan

cppscan: cppscan.o
	g++ -g -o $@ $<

lex-cppscan: lex-cppscan.o
	g++ -g -o $@ $<
	
re2c-cppscan: re2c-cppscan.o
	g++ -g -o $@ $<
	
cppscan.cpp: cppscan.rl $(RAGEL) $(RLCODEGEN)
	$(RAGEL) cppscan.rl | $(RLCODEGEN) -G2 -o $@

lex-cppscan.cpp: cppscan.lex
	$(FLEX) -f -o $@ $<

re2c-cppscan.cpp: cppscan.rec
	$(RE2C) -s $< > $@

example.cpp: example.rec
	$(RE2C) -s $< > $@

%.o: %.cpp
	g++ $(CFLAGS) -c -o $@ $<

cppscan.ps: cppscan.rl $(RAGEL) $(RLCODEGEN)
	$(RAGEL) cppscan.rl | $(RLCODEGEN) -V | dot -Tps > cppscan.ps


format: format.o 
	gcc -g -o format format.o

format.c: format.rl $(RAGEL) $(RLCODEGEN)
	$(RAGEL) format.rl | $(RLCODEGEN) -G2 -o format.c

format.ps: format.rl $(RAGEL) $(RLCODEGEN)
	$(RAGEL) format.rl | $(RLCODEGEN) -V | dot -Tps > format.ps

%.o: %.c
	gcc -Wall -O3 -g -c -o $@ $<

awkemu: awkemu.o
	gcc -g -o awkemu awkemu.o

awkemu.c: awkemu.rl $(RAGEL) $(RLCODEGEN)
	$(RAGEL) awkemu.rl | $(RLCODEGEN) -G2 -o awkemu.c

%.o: %.c
	gcc -pedantic -Wall -g -c -O3 -o $@ $<


mailbox: mailbox.o
	g++ -g -o mailbox mailbox.o 

mailbox.cpp: mailbox.rl $(RAGEL) $(RLCODEGEN)
	$(RAGEL) mailbox.rl | $(RLCODEGEN) -G2 -o mailbox.cpp

%.o: %.cpp
	g++ -Wall -g -c -O3 -o $@ $<

atoi: atoi.o
	g++ -g -o atoi atoi.o 

atoi.cpp: atoi.rl $(RAGEL) $(RLCODEGEN)
	$(RAGEL) atoi.rl | $(RLCODEGEN) -G2 -o atoi.cpp

atoi.o: atoi.cpp
	g++ -Wall -g -c -O3 -o $@ $<
