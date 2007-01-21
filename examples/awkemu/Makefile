RAGEL = ../../ragel/ragel
RLCODEGEN = ../../rlcodegen/rlcodegen

all: awkemu

ps: awkemu.ps

awkemu: awkemu.o
	gcc -g -o awkemu awkemu.o

awkemu.c: awkemu.rl $(RAGEL) $(RLCODEGEN)
	$(RAGEL) awkemu.rl | $(RLCODEGEN) -G2 -o awkemu.c

awkemu.ps: awkemu.rl $(RAGEL) $(RLCODEGEN)
	$(RAGEL) awkemu.rl | $(RLCODEGEN) -V | dot -Tps > awkemu.ps

%.o: %.c
	gcc -pedantic -Wall -g -c -O3 -o $@ $<

distclean clean:
	rm -Rf *.o awkemu.c awkemu awkemu.ps
