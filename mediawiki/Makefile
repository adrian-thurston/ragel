#
#   Copyright 2008 Adrian Thurston <thurston@complang.org>
#

#   This file is part of Colm.
#
#   Colm is free software; you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation; either version 2 of the License, or
#   (at your option) any later version.
#
#   Colm is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with Colm; if not, write to the Free Software
#   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA 

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
