-include ../subject.mk

COLM = $(SUBJ_COLM_BIN)
COLM_INC = $(SUBJ_COLM_CPPFLAGS)
COLM_LIB = $(SUBJ_COLM_LDFLAGS)

all: rlparse

OBJ = parse.o actparams.o commit.o parsetree.o parsedata.o common.o fsmap.o \
	fsmbase.o idbase.o load.o if.o reducer.o inputdata.o fsmgraph.o main.o \
	dump.o

RAGEL_LM = \
	ragel.lm \
	ragel-c.lm \
	ragel-ruby.lm \
	ragel-ocaml.lm \
	ragel-crack.lm

if.h: parse.c
if.cc: parse.c
commit.cc: parse.c

parse.c: rlparse.lm reducer.lm $(RAGEL_LM)
	$(COLM) -c -b rlparse_object -o $@ -e if.h -x if.cc -m commit.cc $<

%.o: %.c
	gcc -c -o $@ -Wall $(COLM_INC) $<

%.o: %.cc
	g++ -c -o $@ -Wall $(COLM_INC) $<

rlparse: $(OBJ)
	g++ -o $@ $(OBJ) $(COLM_LIB) -lcolm

clean:
	rm -f *.o

install:
