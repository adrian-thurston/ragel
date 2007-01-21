RAGEL = ../../ragel/ragel
RLCODEGEN = ../../rlcodegen/rlcodegen
FLEX = flex
RE2C = re2c

CFLAGS = -Wall -g -O3

all: cppscan lex-cppscan re2c-cppscan

ps: cppscan.ps

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

distclean clean:
	rm -Rf *.o cppscan.cpp cppscan cppscan.ps \
		lex-cppscan lex-cppscan.cpp re2c-cppscan re2c-cppscan.cpp
