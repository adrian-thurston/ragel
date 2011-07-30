SUBDIRS = 

TESTS = \
	ruby.lm

DIFFS = \
	ruby01.rb.diff

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

ruby01.rb.diff: ruby01.rb.out ruby01.rb.exp
	@diff -u ruby01.rb.exp ruby01.rb.out > ruby01.rb.diff || ( cat ruby01.rb.diff; rm ruby01.rb.diff )

ruby01.rb.out: ruby.bin
	./ruby.bin  < ruby01.rb.in > ruby01.rb.out

ruby.bin: ruby.lm ../../colm/colm
	../../colm/colm ruby.lm
