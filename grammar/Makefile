all: rust pcre pcre-colm

RAGEL = ../ragel/ragel
COLM = ../colm/colm

rust: rust.lm parserust.lm $(COLM)
	$(COLM) -o rust parserust.lm

pcre: pcre.rl $(RAGEL)
	$(RAGEL) -G2 pcre.rl
	gcc -g -Wall -o pcre pcre.c

pcre-colm: pcre.lm
	$(COLM) -o pcre-colm pcre.lm
