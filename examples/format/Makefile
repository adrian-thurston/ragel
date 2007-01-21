RAGEL = ../../ragel/ragel
RLCODEGEN = ../../rlcodegen/rlcodegen

all: format

ps: format.ps

format: format.o 
	gcc -g -o format format.o

format.c: format.rl $(RAGEL) $(RLCODEGEN)
	$(RAGEL) format.rl | $(RLCODEGEN) -G2 -o format.c

format.ps: format.rl $(RAGEL) $(RLCODEGEN)
	$(RAGEL) format.rl | $(RLCODEGEN) -V | dot -Tps > format.ps

%.o: %.c
	gcc -Wall -O3 -g -c -o $@ $<

distclean clean:
	rm -Rf *.o format.c format format.ps
