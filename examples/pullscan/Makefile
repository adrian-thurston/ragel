RAGEL = ../../ragel/ragel
RLCODEGEN = ../../rlcodegen/rlcodegen

CFLAGS = -Wall -g -O3

all: pullscan

ps: pullscan.ps

pullscan: pullscan.o
	g++ -g -o $@ $<

pullscan.c: pullscan.rl $(RAGEL) $(RLCODEGEN)
	$(RAGEL) pullscan.rl | $(RLCODEGEN) -G2 -o $@

%.o: %.c
	gcc $(CFLAGS) -c -o $@ $<

pullscan.ps: pullscan.rl $(RAGEL) $(RLCODEGEN)
	$(RAGEL) pullscan.rl | $(RLCODEGEN) -V | dot -Tps > pullscan.ps

distclean clean:
	rm -Rf *.o pullscan.c pullscan pullscan.ps
