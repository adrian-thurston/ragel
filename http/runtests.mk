SUBDIRS = 

TESTS = \
	http.lm

DIFFS = \
	http1.diff \
	http2.diff \
	http3.diff

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

http1.diff: http1.out http1.exp
	@diff -u http1.exp http1.out > http1.diff || ( cat http1.diff; rm http1.diff )

http1.out: http.bin
	./http.bin  < http1.in > http1.out

http2.diff: http2.out http2.exp
	@diff -u http2.exp http2.out > http2.diff || ( cat http2.diff; rm http2.diff )

http2.out: http.bin
	./http.bin  < http2.in > http2.out

http3.diff: http3.out http3.exp
	@diff -u http3.exp http3.out > http3.diff || ( cat http3.diff; rm http3.diff )

http3.out: http.bin
	./http.bin  < http3.in > http3.out

http.bin: http.lm ../../colm/colm
	../../colm/colm http.lm
