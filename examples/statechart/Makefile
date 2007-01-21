RAGEL = ../../ragel/ragel
RLCODEGEN = ../../rlcodegen/rlcodegen

all: statechart

ps: statechart.ps

statechart: statechart.o
	g++ -g -o statechart statechart.o 

statechart.cpp: statechart.rl $(RAGEL) $(RLCODEGEN)
	$(RAGEL) statechart.rl | $(RLCODEGEN) -G2 -o statechart.cpp

statechart.o: statechart.cpp
	g++ -Wall -g -c -O3 -o $@ $<

statechart.ps: statechart.rl $(RAGEL) $(RLCODEGEN)
	$(RAGEL) statechart.rl | $(RLCODEGEN) -V | dot -Tps > statechart.ps

distclean clean:
	rm -Rf *.o statechart.cpp statechart statechart.ps
