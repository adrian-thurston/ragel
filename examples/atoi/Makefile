RAGEL = ../../ragel/ragel
RLCODEGEN = ../../rlcodegen/rlcodegen

all: atoi

ps: atoi.ps

atoi: atoi.o
	g++ -g -o atoi atoi.o 

atoi.cpp: atoi.rl $(RAGEL) $(RLCODEGEN)
	$(RAGEL) atoi.rl | $(RLCODEGEN) -G2 -o atoi.cpp

atoi.o: atoi.cpp
	g++ -Wall -g -c -O3 -o $@ $<

atoi.ps: atoi.rl $(RAGEL) $(RLCODEGEN)
	$(RAGEL) atoi.rl | $(RLCODEGEN) -V | dot -Tps > atoi.ps

distclean clean:
	rm -Rf *.o atoi.cpp atoi atoi.ps
