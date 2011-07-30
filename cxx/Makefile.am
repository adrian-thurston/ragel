SUBDIRS = 

TESTS = \
	cxx.lm

DIFFS = \
	cxx01.cpp.diff \
	cxx02.cpp.diff \
	cxx03.cpp.diff \
	cxx04.cpp.diff \
	cxx05.cpp.diff \
	cxx06.cpp.diff \
	cxx07.cpp.diff \
	cxx08.cpp.diff \
	cxx09.cpp.diff \
	cxx10.cpp.diff \
	cxx11.cpp.diff \
	cxx12.cpp.diff \
	cxx13.cpp.diff

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

cxx01.cpp.diff: cxx01.cpp.out cxx01.cpp.exp
	@diff -u cxx01.cpp.exp cxx01.cpp.out > cxx01.cpp.diff || ( cat cxx01.cpp.diff; rm cxx01.cpp.diff )

cxx01.cpp.out: cxx.bin
	./cxx.bin  < cxx01.cpp.in > cxx01.cpp.out

cxx02.cpp.diff: cxx02.cpp.out cxx02.cpp.exp
	@diff -u cxx02.cpp.exp cxx02.cpp.out > cxx02.cpp.diff || ( cat cxx02.cpp.diff; rm cxx02.cpp.diff )

cxx02.cpp.out: cxx.bin
	./cxx.bin  < cxx02.cpp.in > cxx02.cpp.out

cxx03.cpp.diff: cxx03.cpp.out cxx03.cpp.exp
	@diff -u cxx03.cpp.exp cxx03.cpp.out > cxx03.cpp.diff || ( cat cxx03.cpp.diff; rm cxx03.cpp.diff )

cxx03.cpp.out: cxx.bin
	./cxx.bin  < cxx03.cpp.in > cxx03.cpp.out

cxx04.cpp.diff: cxx04.cpp.out cxx04.cpp.exp
	@diff -u cxx04.cpp.exp cxx04.cpp.out > cxx04.cpp.diff || ( cat cxx04.cpp.diff; rm cxx04.cpp.diff )

cxx04.cpp.out: cxx.bin
	./cxx.bin  < cxx04.cpp.in > cxx04.cpp.out

cxx05.cpp.diff: cxx05.cpp.out cxx05.cpp.exp
	@diff -u cxx05.cpp.exp cxx05.cpp.out > cxx05.cpp.diff || ( cat cxx05.cpp.diff; rm cxx05.cpp.diff )

cxx05.cpp.out: cxx.bin
	./cxx.bin  < cxx05.cpp.in > cxx05.cpp.out

cxx06.cpp.diff: cxx06.cpp.out cxx06.cpp.exp
	@diff -u cxx06.cpp.exp cxx06.cpp.out > cxx06.cpp.diff || ( cat cxx06.cpp.diff; rm cxx06.cpp.diff )

cxx06.cpp.out: cxx.bin
	./cxx.bin  < cxx06.cpp.in > cxx06.cpp.out

cxx07.cpp.diff: cxx07.cpp.out cxx07.cpp.exp
	@diff -u cxx07.cpp.exp cxx07.cpp.out > cxx07.cpp.diff || ( cat cxx07.cpp.diff; rm cxx07.cpp.diff )

cxx07.cpp.out: cxx.bin
	./cxx.bin  < cxx07.cpp.in > cxx07.cpp.out

cxx08.cpp.diff: cxx08.cpp.out cxx08.cpp.exp
	@diff -u cxx08.cpp.exp cxx08.cpp.out > cxx08.cpp.diff || ( cat cxx08.cpp.diff; rm cxx08.cpp.diff )

cxx08.cpp.out: cxx.bin
	./cxx.bin  < cxx08.cpp.in > cxx08.cpp.out

cxx09.cpp.diff: cxx09.cpp.out cxx09.cpp.exp
	@diff -u cxx09.cpp.exp cxx09.cpp.out > cxx09.cpp.diff || ( cat cxx09.cpp.diff; rm cxx09.cpp.diff )

cxx09.cpp.out: cxx.bin
	./cxx.bin  < cxx09.cpp.in > cxx09.cpp.out

cxx10.cpp.diff: cxx10.cpp.out cxx10.cpp.exp
	@diff -u cxx10.cpp.exp cxx10.cpp.out > cxx10.cpp.diff || ( cat cxx10.cpp.diff; rm cxx10.cpp.diff )

cxx10.cpp.out: cxx.bin
	./cxx.bin  < cxx10.cpp.in > cxx10.cpp.out

cxx11.cpp.diff: cxx11.cpp.out cxx11.cpp.exp
	@diff -u cxx11.cpp.exp cxx11.cpp.out > cxx11.cpp.diff || ( cat cxx11.cpp.diff; rm cxx11.cpp.diff )

cxx11.cpp.out: cxx.bin
	./cxx.bin  < cxx11.cpp.in > cxx11.cpp.out

cxx12.cpp.diff: cxx12.cpp.out cxx12.cpp.exp
	@diff -u cxx12.cpp.exp cxx12.cpp.out > cxx12.cpp.diff || ( cat cxx12.cpp.diff; rm cxx12.cpp.diff )

cxx12.cpp.out: cxx.bin
	./cxx.bin  < cxx12.cpp.in > cxx12.cpp.out

cxx13.cpp.diff: cxx13.cpp.out cxx13.cpp.exp
	@diff -u cxx13.cpp.exp cxx13.cpp.out > cxx13.cpp.diff || ( cat cxx13.cpp.diff; rm cxx13.cpp.diff )

cxx13.cpp.out: cxx.bin
	./cxx.bin  < cxx13.cpp.in > cxx13.cpp.out

cxx.bin: cxx.lm ../../colm/colm
	../../colm/colm cxx.lm
