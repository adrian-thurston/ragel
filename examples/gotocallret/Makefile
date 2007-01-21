RAGEL = ../../ragel/ragel
RLCODEGEN = ../../rlcodegen/rlcodegen

all: gotocallret

ps: gotocallret.ps

gotocallret: gotocallret.o
	g++ -g -o gotocallret gotocallret.o 

gotocallret.cpp: gotocallret.rl $(RAGEL) $(RLCODEGEN)
	$(RAGEL) gotocallret.rl | $(RLCODEGEN) -G2 -o gotocallret.cpp

gotocallret.o: gotocallret.cpp
	g++ -Wall -g -c -O3 -o $@ $<

gotocallret.ps: gotocallret.rl $(RAGEL) $(RLCODEGEN)
	$(RAGEL) gotocallret.rl | $(RLCODEGEN) -V | dot -Tps > gotocallret.ps

distclean clean:
	rm -Rf *.o gotocallret.cpp gotocallret gotocallret.ps
