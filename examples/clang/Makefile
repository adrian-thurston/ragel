RAGEL = ../../ragel/ragel
RLCODEGEN = ../../rlcodegen/rlcodegen

all: clang

ps: clang.ps

clang: clang.o 
	gcc -g -o clang clang.o

clang.c: clang.rl $(RAGEL) $(RLCODEGEN)
	$(RAGEL) clang.rl | $(RLCODEGEN) -G2 -o clang.c

clang.ps: clang.rl $(RAGEL) $(RLCODEGEN)
	$(RAGEL) clang.rl | $(RLCODEGEN) -V | dot -Tps > clang.ps

%.o: %.c
	gcc -pedantic -Wall -O3 -g -c -o $@ $<

distclean clean:
	rm -Rf *.o clang.c clang clang.ps
