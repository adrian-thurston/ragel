#
#   Copyright 2007 Adrian Thurston <thurston@complang.org>
#


SRC = $(wildcard *.lm)
BIN = $(SRC:%.lm=%.bin)
COLM = ../../colm/colm

all: $(BIN)

$(BIN): $(COLM)

$(BIN): %.bin: %.lm
	$(COLM) $<

clean:
	rm -f *.cpp *.bin
