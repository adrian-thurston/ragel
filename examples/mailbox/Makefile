RAGEL = ../../ragel/ragel
RLCODEGEN = ../../rlcodegen/rlcodegen

all: mailbox

mailbox: mailbox.o
	g++ -g -o mailbox mailbox.o 

mailbox.cpp: mailbox.rl $(RAGEL) $(RLCODEGEN)
	$(RAGEL) mailbox.rl | $(RLCODEGEN) -G2 -o mailbox.cpp

%.o: %.cpp
	g++ -Wall -g -c -O3 -o $@ $<

distclean clean:
	rm -Rf *.o mailbox.cpp mailbox mailbox.ps
