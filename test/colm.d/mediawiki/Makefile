#
#   Copyright 2008 Adrian Thurston <thurston@complang.org>
#

SRC = $(wildcard *.lm)
BIN = $(SRC:%.lm=%.bin)
COLM = ../../colm/colm

all: pdump garticle mediawiki.bin

pdump.cpp: pdump.rl
	ragel -G2 -o pdump.cpp pdump.rl

pdump: pdump.cpp
	g++ -O3 -Wall -o pdump pdump.cpp

garticle.cpp: garticle.rl
	ragel -G2 -o garticle.cpp garticle.rl

garticle: garticle.cpp
	g++ -O3 -Wall -o garticle garticle.cpp

mediawiki.bin: mediawiki.lm
	$(COLM) mediawiki.lm

clean:
	rm -f mediawiki.cpp *.bin garticle.cpp garticle pdump.cpp pdump
