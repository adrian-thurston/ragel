
.SUFFIXES:

RAGEL = ../../ragel/ragel
MODE = -G0

all: url scan2

url: url.rl
	$(RAGEL) $(MODE) -O url.rl -o url.ml
	$(RAGEL) $(MODE) -O url_authority.rl -o url_authority.ml
	ocamlopt -g unix.cmxa url_authority.ml url.ml -o url

cond: cond.rl
	$(RAGEL) $(MODE) -O -o cond.ml cond.rl
	ocamlopt -g unix.cmxa -o cond cond.ml

rpn: rpn.rl
	$(RAGEL) $(MODE) -O -o rpn.ml rpn.rl
	ocamlopt -g unix.cmxa -o rpn rpn.ml

atoi: atoi.rl
	$(RAGEL) $(MODE) -O -o atoi.ml atoi.rl
	ocamlopt -g unix.cmxa -o atoi atoi.ml

clang: clang.rl
	$(RAGEL) $(MODE) -O -o clang.ml clang.rl
	ocamlopt -g unix.cmxa -o clang clang.ml

gotocallret: gotocallret.rl
	$(RAGEL) $(MODE) -O -o gotocallret.ml gotocallret.rl
	ocamlopt -g unix.cmxa -o gotocallret gotocallret.ml

scan1: scan1.rl
	$(RAGEL) $(MODE) -O -o scan1.ml scan1.rl
	ocamlopt -g unix.cmxa -o scan1 scan1.ml

scan2: scan2.rl
	$(RAGEL) $(MODE) -O -o scan2.ml scan2.rl
	ocamlopt -g unix.cmxa -o scan2 scan2.ml

.PHONY: clean
clean:
	rm -f *.cm* *.o *.annot
	rm -f $(subst .rl,.ml,$(wildcard *.rl))
	rm -f $(subst .rl,,$(wildcard *.rl))
