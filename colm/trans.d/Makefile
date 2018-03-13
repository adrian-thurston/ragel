
include ../subject.mk

all: trans

COLM = $(SUBJECT_BIN)
COLM_INC = $(SUBJECT_CPPFLAGS)
COLM_LIB = $(SUBJECT_LDFLAGS)

TRANS_DEPS = $(wildcard *-*.lm)

OBJ = actparams.o trans.o main.o

trans: $(OBJ)
	g++ -o $@ $(OBJ) $(COLM_LIB) -lcolm

actparams.o: actparams.cc
	g++ -c $(COLM_INC) -o $@ $<

main.o: main.c
	gcc -c $(COLM_INC) -o $@ $<

trans.o: trans.c
	gcc -c $(COLM_INC) -o $@ $<

trans.c: trans.lm $(TRANS_DEPS) 
	$(COLM) -b trans_object -c -o $@ $<
