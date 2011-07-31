SUBDIRS = 

TESTS = \
	xml.lm

DIFFS = \
	xml.diff

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

xml.diff: xml.out xml.exp
	@diff -u xml.exp xml.out > xml.diff || ( cat xml.diff; rm xml.diff )

xml.out: xml.bin
	./xml.bin  < xml.in > xml.out

xml.bin: xml.lm ../../colm/colm
	../../colm/colm xml.lm
