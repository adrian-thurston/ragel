include ../subject.mk

COLM = $(SUBJECT_BIN)
COLM_INC = $(SUBJECT_CPPFLAGS)
COLM_LIB = $(SUBJECT_LDFLAGS)

all: loadred

OBJ = parse.o actparams.o commit.o parsetree.o parsedata.o common.o fsmap.o \
	fsmbase.o idbase.o load.o if.o reducer.o inputdata.o fsmgraph.o main.o

%.o: %.c
	gcc -c -o $@ -Wall $(COLM_INC) $<

%.o: %.cc
	g++ -c -o $@ -Wall $(COLM_INC) $<

loadred: $(OBJ)
	g++ -o $@ $(OBJ) $(COLM_LIB) -lcolm

RAGEL_LM = \
	ragel.lm \
	ragel-c.lm \
	ragel-ruby.lm \
	ragel-ocaml.lm \
	ragel-crack.lm

parse.c: rlparse.lm reducer.lm $(RAGEL_LM)
	$(COLM) -c -b rlparse_object -o $@ -e if.h -x if.cc -m commit.cc $<
