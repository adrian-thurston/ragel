all: rust pcre pcre-colm dns

RAGEL = ../ragel/ragel
COLM = ../colm/colm

rust: rust.lm parserust.lm $(COLM)
	$(COLM) -o rust parserust.lm

pcre: pcre.rl $(RAGEL)
	$(RAGEL) -G2 pcre.rl
	gcc -g -Wall -o pcre pcre.c

pcre-colm: pcre.lm
	$(COLM) -o pcre-colm pcre.lm

dns: dns.rl
	$(RAGEL) -G2 -o dns.cc dns.rl
	g++ -g -Wall -o dns dns.cc -lpcap
