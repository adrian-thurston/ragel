COLM = ../../colm/colm
RAGEL = ../../ragel/ragel

all: dns

dns: dns.rl
	$(RAGEL) -G2 -o dns.cc dns.rl
	g++ -g -Wall -o dns dns.cc -lpcap

