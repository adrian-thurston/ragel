SUBDIRS = 

TESTS = \
	html.lm

DIFFS = \
	html01.diff

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

html01.diff: html01.out html01.exp
	@diff -u html01.exp html01.out > html01.diff || ( cat html01.diff; rm html01.diff )

html01.out: html.bin
	./html.bin  < html01.in > html01.out

html.bin: html.lm ../../colm/colm
	../../colm/colm html.lm
