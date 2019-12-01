COLM = ../../colm/colm
RAGEL = ../../ragel/ragel

all: rust

rust: rust.lm parserust.lm $(COLM)
	$(COLM) -o rust parserust.lm
