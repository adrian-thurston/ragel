COLM = ../../colm/colm
RAGEL = ../../ragel/ragel

all: go insert-semi

go: go.lm parsego.lm utf8.lm $(COLM)
	$(COLM) -o go parsego.lm

insert-semi: insert-semi.lm
	$(COLM) $<

utf8.lm: Makefile utf8pat.rb UnicodeData.txt
	echo 'rl unicode_letter /' >utf8.lm
	ruby utf8pat.rb 'L[ultmo]' 1 UnicodeData.txt >> utf8.lm
	echo '/' >>utf8.lm
	echo 'rl unicode_digit /' >>utf8.lm
	ruby utf8pat.rb 'Nd' 1 UnicodeData.txt >> utf8.lm
	echo '/' >>utf8.lm
