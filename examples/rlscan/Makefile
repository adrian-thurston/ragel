RAGEL = ../../ragel/ragel
RLCODEGEN = ../../rlcodegen/rlcodegen

all: rlscan

ps: rlscan.ps

rlscan: rlscan.o
	g++ -g -o rlscan rlscan.o 

rlscan.cpp: rlscan.rl $(RAGEL) $(RLCODEGEN)
	$(RAGEL) rlscan.rl | $(RLCODEGEN) -G2 -o rlscan.cpp

%.o: %.cpp
	g++ -Wall -g -c -O3 -o $@ $<

rlscan.ps: rlscan.rl $(RAGEL) $(RLCODEGEN)
	$(RAGEL) rlscan.rl | $(RLCODEGEN) -V | dot -Tps > rlscan.ps

distclean clean:
	rm -Rf *.o rlscan.cpp rlscan rlscan.ps
