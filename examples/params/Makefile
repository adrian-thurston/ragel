RAGEL = ../../ragel/ragel
RLCODEGEN = ../../rlcodegen/rlcodegen

all: params

ps: params.ps

params: params.o 
	gcc -g -o params params.o

params.c: params.rl $(RAGEL) $(RLCODEGEN)
	$(RAGEL) params.rl | $(RLCODEGEN) -G2 -o params.c

params.ps: params.rl $(RAGEL) $(RLCODEGEN)
	$(RAGEL) params.rl | $(RLCODEGEN) -V | dot -Tps > params.ps

%.o: %.c
	gcc -Wall -O3 -g -c -o $@ $<

distclean clean:
	rm -Rf *.o params.c params params.ps
