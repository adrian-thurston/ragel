#
#   Copyright 2002-2006 Adrian Thurston <thurston@cs.queensu.ca>
#

#   This file is part of Ragel.
#
#   Ragel is free software; you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation; either version 2 of the License, or
#   (at your option) any later version.
#
#   Ragel is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with Ragel; if not, write to the Free Software
#   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA 


SRC = $(wildcard *.lm)
BIN = $(SRC:%.lm=%.bin)
COLM = ../../colm/colm

all: $(BIN)

$(BIN): $(COLM)

$(BIN): %.bin: %.lm
	$(COLM) $<

clean:
	rm -f *.cpp *.bin
