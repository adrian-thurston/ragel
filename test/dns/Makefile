#
#   Copyright 2007 Adrian Thurston <thurston@cs.queensu.ca>
#

SRC = $(wildcard *.lm)
BIN = $(SRC:%.lm=%.bin)
COLM = ../../colm/colm

all: $(BIN)

extract: extract.c
	gcc -Wall -o extract extract.c -lpcap

$(BIN): $(COLM)

$(BIN): %.bin: %.lm
	$(COLM) $<

clean:
	rm -f dns.cpp dns.bin extract
