SUBDIRS = 

TESTS = \
	python.lm

DIFFS = \
	python1.py.diff \
	python2.py.diff \
	python3.py.diff \
	python4.py.diff

all: Makefile $(DIFFS) $(SUBDIRS)

.PHONY: clean $(SUBDIRS:%=%-clean)
clean: $(SUBDIRS:%=%-clean)
	rm -f *.bin
$(SUBDIRS:%=%-clean):
	cd $(@:%-clean=%) && $(MAKE) clean

.PHONY: $(SUBDIRS)
$(SUBDIRS):
	cd $@ && $(MAKE)

Makefile: ../genmf TESTS
	../genmf > Makefile

python1.py.diff: python1.py.out python1.py.exp
	@diff -u python1.py.exp python1.py.out > python1.py.diff || ( cat python1.py.diff; rm python1.py.diff )

python1.py.out: python.bin
	./python.bin  < python1.py.in > python1.py.out

python2.py.diff: python2.py.out python2.py.exp
	@diff -u python2.py.exp python2.py.out > python2.py.diff || ( cat python2.py.diff; rm python2.py.diff )

python2.py.out: python.bin
	./python.bin  < python2.py.in > python2.py.out

python3.py.diff: python3.py.out python3.py.exp
	@diff -u python3.py.exp python3.py.out > python3.py.diff || ( cat python3.py.diff; rm python3.py.diff )

python3.py.out: python.bin
	./python.bin  < python3.py.in > python3.py.out

python4.py.diff: python4.py.out python4.py.exp
	@diff -u python4.py.exp python4.py.out > python4.py.diff || ( cat python4.py.diff; rm python4.py.diff )

python4.py.out: python.bin
	./python.bin  < python4.py.in > python4.py.out

python.bin: python.lm ../../colm/colm
	../../colm/colm python.lm
