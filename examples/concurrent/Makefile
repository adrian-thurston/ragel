RAGEL = ../../ragel/ragel
RLCODEGEN = ../../rlcodegen/rlcodegen

all: concurrent

ps: concurrent.ps

concurrent: concurrent.o
	g++ -g -o concurrent concurrent.o 

concurrent.cpp: concurrent.rl $(RAGEL) $(RLCODEGEN)
	$(RAGEL) concurrent.rl | $(RLCODEGEN) -G2 -o concurrent.cpp

concurrent.ps: concurrent.rl $(RAGEL) $(RLCODEGEN)
	$(RAGEL) concurrent.rl | $(RLCODEGEN) -V | dot -Tps > concurrent.ps

%.o: %.cpp
	g++ -Wall -g -c -O3 -o $@ $<

distclean clean:
	rm -Rf *.o concurrent.cpp concurrent concurrent.ps
