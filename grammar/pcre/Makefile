COLM = ../../colm/colm
RAGEL = ../../ragel/ragel

all: pcre-colm pcre-ragel

pcre-colm: pcre.lm
	$(COLM) -o $@ pcre.lm

pcre-ragel: pcre.rl $(RAGEL)
	$(RAGEL) -G2 pcre.rl
	gcc -g -Wall -o $@ pcre.c
