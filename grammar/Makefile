all: rust pcre

rust: rust.lm parserust.lm
	../colm/colm -o rust parserust.lm

pcre: pcre.rl
	../ragel/ragel -G2 pcre.rl
	gcc -g -Wall -o pcre pcre.c
