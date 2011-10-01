#!/usr/bin/make -f 

SUBDIRS =  python html xml http cxx ruby

TESTS = \
	backtrack1.lm \
	backtrack2.lm \
	backtrack3.lm \
	dns.lm \
	accum1.lm \
	accum2.lm \
	accum3.lm \
	accumbt.lm \
	mutualrec.lm \
	argv1.lm \
	argv2.lm \
	exit.lm \
	rubyhere.lm \
	translate1.lm \
	translate2.lm \
	construct1.lm \
	construct2.lm \
	construct3.lm \
	treecmp1.lm \
	context1.lm \
	context2.lm \
	context3.lm \
	undofrag1.lm \
	undofrag2.lm \
	undofrag3.lm \
	nestedcomm.lm \
	reparse.lm \
	btscan1.lm \
	btscan2.lm \
	island.lm \
	func.lm \
	travs1.lm \
	constructex.lm \
	rediv.lm \
	liftattrs.lm \
	mailbox.lm \
	string.lm \
	repeat.lm \
	ragelambig1.lm \
	ragelambig2.lm \
	ragelambig3.lm \
	ragelambig4.lm \
	counting1.lm \
	counting2.lm \
	counting3.lm \
	counting4.lm \
	til.lm \
	matchex.lm \
	maxlen.lm \
	superid.lm \
	tags.lm \
	heredoc.lm \
	commitbt.lm \
	sprintf.lm \
	div.lm \
	scope1.lm

DIFFS = \
	backtrack1.diff \
	backtrack2.diff \
	backtrack3.diff \
	dns.diff \
	accum1.diff \
	accum2.diff \
	accum3.diff \
	accumbt.diff \
	mutualrec.diff \
	argv1.diff \
	argv2.diff \
	exit.diff \
	rubyhere.diff \
	translate1.diff \
	translate2.diff \
	construct1.diff \
	construct2.diff \
	construct3.diff \
	treecmp1.diff \
	context1.diff \
	context2.diff \
	context3.diff \
	undofrag1.diff \
	undofrag2.diff \
	undofrag3.diff \
	nestedcomm.diff \
	reparse.diff \
	btscan1.diff \
	btscan2.diff \
	island.diff \
	func.diff \
	travs1.diff \
	constructex.diff \
	rediv.diff \
	liftattrs.diff \
	mailbox.diff \
	string.diff \
	repeat.diff \
	ragelambig1.diff \
	ragelambig2.diff \
	ragelambig3.diff \
	ragelambig4.diff \
	counting1.diff \
	counting2.diff \
	counting3.diff \
	counting4.diff \
	til.diff \
	matchex.diff \
	maxlen.diff \
	superid.diff \
	tags.diff \
	heredoc.diff \
	commitbt.diff \
	sprintf.diff \
	div.diff \
	scope1.diff

all: runtests.mk $(DIFFS) $(SUBDIRS)

.PHONY: clean $(SUBDIRS:%=%-clean)
clean: $(SUBDIRS:%=%-clean)
	rm -f *.bin
$(SUBDIRS:%=%-clean):
	cd $(@:%-clean=%) && $(MAKE) -f runtests.mk clean

.PHONY: $(SUBDIRS)
$(SUBDIRS):
	cd $@ && $(MAKE) -f runtests.mk

runtests.mk: ./genmf TESTS
	./genmf > runtests.mk

backtrack1.diff: backtrack1.out backtrack1.exp
	@diff -u backtrack1.exp backtrack1.out > backtrack1.diff || ( cat backtrack1.diff; rm backtrack1.diff )

backtrack1.out: backtrack1.bin
	./backtrack1.bin  < backtrack1.in > backtrack1.out

backtrack1.bin: backtrack1.lm ./../colm/colm
	./../colm/colm backtrack1.lm
backtrack2.diff: backtrack2.out backtrack2.exp
	@diff -u backtrack2.exp backtrack2.out > backtrack2.diff || ( cat backtrack2.diff; rm backtrack2.diff )

backtrack2.out: backtrack2.bin
	./backtrack2.bin  < backtrack2.in > backtrack2.out

backtrack2.bin: backtrack2.lm ./../colm/colm
	./../colm/colm backtrack2.lm
backtrack3.diff: backtrack3.out backtrack3.exp
	@diff -u backtrack3.exp backtrack3.out > backtrack3.diff || ( cat backtrack3.diff; rm backtrack3.diff )

backtrack3.out: backtrack3.bin
	./backtrack3.bin  < backtrack3.in > backtrack3.out

backtrack3.bin: backtrack3.lm ./../colm/colm
	./../colm/colm backtrack3.lm
dns.diff: dns.out dns.exp
	@diff -u dns.exp dns.out > dns.diff || ( cat dns.diff; rm dns.diff )

dns.out: dns.bin
	./dns.bin  < dns.in > dns.out

dns.bin: dns.lm ./../colm/colm
	./../colm/colm dns.lm
accum1.diff: accum1.out accum1.exp
	@diff -u accum1.exp accum1.out > accum1.diff || ( cat accum1.diff; rm accum1.diff )

accum1.out: accum1.bin
	./accum1.bin  < accum1.in > accum1.out

accum1.bin: accum1.lm ./../colm/colm
	./../colm/colm accum1.lm
accum2.diff: accum2.out accum2.exp
	@diff -u accum2.exp accum2.out > accum2.diff || ( cat accum2.diff; rm accum2.diff )

accum2.out: accum2.bin
	./accum2.bin  < accum2.in > accum2.out

accum2.bin: accum2.lm ./../colm/colm
	./../colm/colm accum2.lm
accum3.diff: accum3.out accum3.exp
	@diff -u accum3.exp accum3.out > accum3.diff || ( cat accum3.diff; rm accum3.diff )

accum3.out: accum3.bin
	./accum3.bin -qv -h -o output sdf -i eth0 file > accum3.out

accum3.bin: accum3.lm ./../colm/colm
	./../colm/colm accum3.lm
accumbt.diff: accumbt.out accumbt.exp
	@diff -u accumbt.exp accumbt.out > accumbt.diff || ( cat accumbt.diff; rm accumbt.diff )

accumbt.out: accumbt.bin
	./accumbt.bin  > accumbt.out

accumbt.bin: accumbt.lm ./../colm/colm
	./../colm/colm accumbt.lm
mutualrec.diff: mutualrec.out mutualrec.exp
	@diff -u mutualrec.exp mutualrec.out > mutualrec.diff || ( cat mutualrec.diff; rm mutualrec.diff )

mutualrec.out: mutualrec.bin
	./mutualrec.bin  > mutualrec.out

mutualrec.bin: mutualrec.lm ./../colm/colm
	./../colm/colm mutualrec.lm
argv1.diff: argv1.out argv1.exp
	@diff -u argv1.exp argv1.out > argv1.diff || ( cat argv1.diff; rm argv1.diff )

argv1.out: argv1.bin
	./argv1.bin a b c 1 2 3 > argv1.out

argv1.bin: argv1.lm ./../colm/colm
	./../colm/colm argv1.lm
argv2.diff: argv2.out argv2.exp
	@diff -u argv2.exp argv2.out > argv2.diff || ( cat argv2.diff; rm argv2.diff )

argv2.out: argv2.bin
	./argv2.bin -qv -h -o output -iinput file --input=foo --input bar --help --verbose > argv2.out

argv2.bin: argv2.lm ./../colm/colm
	./../colm/colm argv2.lm
exit.diff: exit.out exit.exp
	@diff -u exit.exp exit.out > exit.diff || ( cat exit.diff; rm exit.diff )

exit.out: exit.bin
	./exit.bin  > exit.out

exit.bin: exit.lm ./../colm/colm
	./../colm/colm exit.lm
rubyhere.diff: rubyhere.out rubyhere.exp
	@diff -u rubyhere.exp rubyhere.out > rubyhere.diff || ( cat rubyhere.diff; rm rubyhere.diff )

rubyhere.out: rubyhere.bin
	./rubyhere.bin  < rubyhere.in > rubyhere.out

rubyhere.bin: rubyhere.lm ./../colm/colm
	./../colm/colm rubyhere.lm
translate1.diff: translate1.out translate1.exp
	@diff -u translate1.exp translate1.out > translate1.diff || ( cat translate1.diff; rm translate1.diff )

translate1.out: translate1.bin
	./translate1.bin  < translate1.in > translate1.out

translate1.bin: translate1.lm ./../colm/colm
	./../colm/colm translate1.lm
translate2.diff: translate2.out translate2.exp
	@diff -u translate2.exp translate2.out > translate2.diff || ( cat translate2.diff; rm translate2.diff )

translate2.out: translate2.bin
	./translate2.bin  < translate2.in > translate2.out

translate2.bin: translate2.lm ./../colm/colm
	./../colm/colm translate2.lm
construct1.diff: construct1.out construct1.exp
	@diff -u construct1.exp construct1.out > construct1.diff || ( cat construct1.diff; rm construct1.diff )

construct1.out: construct1.bin
	./construct1.bin  > construct1.out

construct1.bin: construct1.lm ./../colm/colm
	./../colm/colm construct1.lm
construct2.diff: construct2.out construct2.exp
	@diff -u construct2.exp construct2.out > construct2.diff || ( cat construct2.diff; rm construct2.diff )

construct2.out: construct2.bin
	./construct2.bin  > construct2.out

construct2.bin: construct2.lm ./../colm/colm
	./../colm/colm construct2.lm
construct3.diff: construct3.out construct3.exp
	@diff -u construct3.exp construct3.out > construct3.diff || ( cat construct3.diff; rm construct3.diff )

construct3.out: construct3.bin
	./construct3.bin  > construct3.out

construct3.bin: construct3.lm ./../colm/colm
	./../colm/colm construct3.lm
treecmp1.diff: treecmp1.out treecmp1.exp
	@diff -u treecmp1.exp treecmp1.out > treecmp1.diff || ( cat treecmp1.diff; rm treecmp1.diff )

treecmp1.out: treecmp1.bin
	./treecmp1.bin  < treecmp1.in > treecmp1.out

treecmp1.bin: treecmp1.lm ./../colm/colm
	./../colm/colm treecmp1.lm
context1.diff: context1.out context1.exp
	@diff -u context1.exp context1.out > context1.diff || ( cat context1.diff; rm context1.diff )

context1.out: context1.bin
	./context1.bin  < context1.in > context1.out

context1.bin: context1.lm ./../colm/colm
	./../colm/colm context1.lm
context2.diff: context2.out context2.exp
	@diff -u context2.exp context2.out > context2.diff || ( cat context2.diff; rm context2.diff )

context2.out: context2.bin
	./context2.bin  < context2.in > context2.out

context2.bin: context2.lm ./../colm/colm
	./../colm/colm context2.lm
context3.diff: context3.out context3.exp
	@diff -u context3.exp context3.out > context3.diff || ( cat context3.diff; rm context3.diff )

context3.out: context3.bin
	./context3.bin  < context3.in > context3.out

context3.bin: context3.lm ./../colm/colm
	./../colm/colm context3.lm
undofrag1.diff: undofrag1.out undofrag1.exp
	@diff -u undofrag1.exp undofrag1.out > undofrag1.diff || ( cat undofrag1.diff; rm undofrag1.diff )

undofrag1.out: undofrag1.bin
	./undofrag1.bin  < undofrag1.in > undofrag1.out

undofrag1.bin: undofrag1.lm ./../colm/colm
	./../colm/colm undofrag1.lm
undofrag2.diff: undofrag2.out undofrag2.exp
	@diff -u undofrag2.exp undofrag2.out > undofrag2.diff || ( cat undofrag2.diff; rm undofrag2.diff )

undofrag2.out: undofrag2.bin
	./undofrag2.bin  < undofrag2.in > undofrag2.out

undofrag2.bin: undofrag2.lm ./../colm/colm
	./../colm/colm undofrag2.lm
undofrag3.diff: undofrag3.out undofrag3.exp
	@diff -u undofrag3.exp undofrag3.out > undofrag3.diff || ( cat undofrag3.diff; rm undofrag3.diff )

undofrag3.out: undofrag3.bin
	./undofrag3.bin  < undofrag3.in > undofrag3.out

undofrag3.bin: undofrag3.lm ./../colm/colm
	./../colm/colm undofrag3.lm
nestedcomm.diff: nestedcomm.out nestedcomm.exp
	@diff -u nestedcomm.exp nestedcomm.out > nestedcomm.diff || ( cat nestedcomm.diff; rm nestedcomm.diff )

nestedcomm.out: nestedcomm.bin
	./nestedcomm.bin  < nestedcomm.in > nestedcomm.out

nestedcomm.bin: nestedcomm.lm ./../colm/colm
	./../colm/colm nestedcomm.lm
reparse.diff: reparse.out reparse.exp
	@diff -u reparse.exp reparse.out > reparse.diff || ( cat reparse.diff; rm reparse.diff )

reparse.out: reparse.bin
	./reparse.bin  < reparse.in > reparse.out

reparse.bin: reparse.lm ./../colm/colm
	./../colm/colm reparse.lm
btscan1.diff: btscan1.out btscan1.exp
	@diff -u btscan1.exp btscan1.out > btscan1.diff || ( cat btscan1.diff; rm btscan1.diff )

btscan1.out: btscan1.bin
	./btscan1.bin  < btscan1.in > btscan1.out

btscan1.bin: btscan1.lm ./../colm/colm
	./../colm/colm btscan1.lm
btscan2.diff: btscan2.out btscan2.exp
	@diff -u btscan2.exp btscan2.out > btscan2.diff || ( cat btscan2.diff; rm btscan2.diff )

btscan2.out: btscan2.bin
	./btscan2.bin  > btscan2.out

btscan2.bin: btscan2.lm ./../colm/colm
	./../colm/colm btscan2.lm
island.diff: island.out island.exp
	@diff -u island.exp island.out > island.diff || ( cat island.diff; rm island.diff )

island.out: island.bin
	./island.bin  < island.in > island.out

island.bin: island.lm ./../colm/colm
	./../colm/colm island.lm
func.diff: func.out func.exp
	@diff -u func.exp func.out > func.diff || ( cat func.diff; rm func.diff )

func.out: func.bin
	./func.bin  < func.in > func.out

func.bin: func.lm ./../colm/colm
	./../colm/colm func.lm
travs1.diff: travs1.out travs1.exp
	@diff -u travs1.exp travs1.out > travs1.diff || ( cat travs1.diff; rm travs1.diff )

travs1.out: travs1.bin
	./travs1.bin  < travs1.in > travs1.out

travs1.bin: travs1.lm ./../colm/colm
	./../colm/colm travs1.lm
constructex.diff: constructex.out constructex.exp
	@diff -u constructex.exp constructex.out > constructex.diff || ( cat constructex.diff; rm constructex.diff )

constructex.out: constructex.bin
	./constructex.bin  < constructex.in > constructex.out

constructex.bin: constructex.lm ./../colm/colm
	./../colm/colm constructex.lm
rediv.diff: rediv.out rediv.exp
	@diff -u rediv.exp rediv.out > rediv.diff || ( cat rediv.diff; rm rediv.diff )

rediv.out: rediv.bin
	./rediv.bin  < rediv.in > rediv.out

rediv.bin: rediv.lm ./../colm/colm
	./../colm/colm rediv.lm
liftattrs.diff: liftattrs.out liftattrs.exp
	@diff -u liftattrs.exp liftattrs.out > liftattrs.diff || ( cat liftattrs.diff; rm liftattrs.diff )

liftattrs.out: liftattrs.bin
	./liftattrs.bin  < liftattrs.in > liftattrs.out

liftattrs.bin: liftattrs.lm ./../colm/colm
	./../colm/colm liftattrs.lm
mailbox.diff: mailbox.out mailbox.exp
	@diff -u mailbox.exp mailbox.out > mailbox.diff || ( cat mailbox.diff; rm mailbox.diff )

mailbox.out: mailbox.bin
	./mailbox.bin  < mailbox.in > mailbox.out

mailbox.bin: mailbox.lm ./../colm/colm
	./../colm/colm mailbox.lm
string.diff: string.out string.exp
	@diff -u string.exp string.out > string.diff || ( cat string.diff; rm string.diff )

string.out: string.bin
	./string.bin  < string.in > string.out

string.bin: string.lm ./../colm/colm
	./../colm/colm string.lm
repeat.diff: repeat.out repeat.exp
	@diff -u repeat.exp repeat.out > repeat.diff || ( cat repeat.diff; rm repeat.diff )

repeat.out: repeat.bin
	./repeat.bin  < repeat.in > repeat.out

repeat.bin: repeat.lm ./../colm/colm
	./../colm/colm repeat.lm
ragelambig1.diff: ragelambig1.out ragelambig1.exp
	@diff -u ragelambig1.exp ragelambig1.out > ragelambig1.diff || ( cat ragelambig1.diff; rm ragelambig1.diff )

ragelambig1.out: ragelambig1.bin
	./ragelambig1.bin  < ragelambig1.in > ragelambig1.out

ragelambig1.bin: ragelambig1.lm ./../colm/colm
	./../colm/colm ragelambig1.lm
ragelambig2.diff: ragelambig2.out ragelambig2.exp
	@diff -u ragelambig2.exp ragelambig2.out > ragelambig2.diff || ( cat ragelambig2.diff; rm ragelambig2.diff )

ragelambig2.out: ragelambig2.bin
	./ragelambig2.bin  < ragelambig2.in > ragelambig2.out

ragelambig2.bin: ragelambig2.lm ./../colm/colm
	./../colm/colm ragelambig2.lm
ragelambig3.diff: ragelambig3.out ragelambig3.exp
	@diff -u ragelambig3.exp ragelambig3.out > ragelambig3.diff || ( cat ragelambig3.diff; rm ragelambig3.diff )

ragelambig3.out: ragelambig3.bin
	./ragelambig3.bin  < ragelambig3.in > ragelambig3.out

ragelambig3.bin: ragelambig3.lm ./../colm/colm
	./../colm/colm ragelambig3.lm
ragelambig4.diff: ragelambig4.out ragelambig4.exp
	@diff -u ragelambig4.exp ragelambig4.out > ragelambig4.diff || ( cat ragelambig4.diff; rm ragelambig4.diff )

ragelambig4.out: ragelambig4.bin
	./ragelambig4.bin  < ragelambig4.in > ragelambig4.out

ragelambig4.bin: ragelambig4.lm ./../colm/colm
	./../colm/colm ragelambig4.lm
counting1.diff: counting1.out counting1.exp
	@diff -u counting1.exp counting1.out > counting1.diff || ( cat counting1.diff; rm counting1.diff )

counting1.out: counting1.bin
	./counting1.bin  < counting1.in > counting1.out

counting1.bin: counting1.lm ./../colm/colm
	./../colm/colm counting1.lm
counting2.diff: counting2.out counting2.exp
	@diff -u counting2.exp counting2.out > counting2.diff || ( cat counting2.diff; rm counting2.diff )

counting2.out: counting2.bin
	./counting2.bin  < counting2.in > counting2.out

counting2.bin: counting2.lm ./../colm/colm
	./../colm/colm counting2.lm
counting3.diff: counting3.out counting3.exp
	@diff -u counting3.exp counting3.out > counting3.diff || ( cat counting3.diff; rm counting3.diff )

counting3.out: counting3.bin
	./counting3.bin  < counting3.in > counting3.out

counting3.bin: counting3.lm ./../colm/colm
	./../colm/colm counting3.lm
counting4.diff: counting4.out counting4.exp
	@diff -u counting4.exp counting4.out > counting4.diff || ( cat counting4.diff; rm counting4.diff )

counting4.out: counting4.bin
	./counting4.bin  < counting4.in > counting4.out

counting4.bin: counting4.lm ./../colm/colm
	./../colm/colm counting4.lm
til.diff: til.out til.exp
	@diff -u til.exp til.out > til.diff || ( cat til.diff; rm til.diff )

til.out: til.bin
	./til.bin  < til.in > til.out

til.bin: til.lm ./../colm/colm
	./../colm/colm til.lm
matchex.diff: matchex.out matchex.exp
	@diff -u matchex.exp matchex.out > matchex.diff || ( cat matchex.diff; rm matchex.diff )

matchex.out: matchex.bin
	./matchex.bin  < matchex.in > matchex.out

matchex.bin: matchex.lm ./../colm/colm
	./../colm/colm matchex.lm
maxlen.diff: maxlen.out maxlen.exp
	@diff -u maxlen.exp maxlen.out > maxlen.diff || ( cat maxlen.diff; rm maxlen.diff )

maxlen.out: maxlen.bin
	./maxlen.bin  < maxlen.in > maxlen.out

maxlen.bin: maxlen.lm ./../colm/colm
	./../colm/colm maxlen.lm
superid.diff: superid.out superid.exp
	@diff -u superid.exp superid.out > superid.diff || ( cat superid.diff; rm superid.diff )

superid.out: superid.bin
	./superid.bin  < superid.in > superid.out

superid.bin: superid.lm ./../colm/colm
	./../colm/colm superid.lm
tags.diff: tags.out tags.exp
	@diff -u tags.exp tags.out > tags.diff || ( cat tags.diff; rm tags.diff )

tags.out: tags.bin
	./tags.bin  < tags.in > tags.out

tags.bin: tags.lm ./../colm/colm
	./../colm/colm tags.lm
heredoc.diff: heredoc.out heredoc.exp
	@diff -u heredoc.exp heredoc.out > heredoc.diff || ( cat heredoc.diff; rm heredoc.diff )

heredoc.out: heredoc.bin
	./heredoc.bin  < heredoc.in > heredoc.out

heredoc.bin: heredoc.lm ./../colm/colm
	./../colm/colm heredoc.lm
commitbt.diff: commitbt.out commitbt.exp
	@diff -u commitbt.exp commitbt.out > commitbt.diff || ( cat commitbt.diff; rm commitbt.diff )

commitbt.out: commitbt.bin
	./commitbt.bin  < commitbt.in > commitbt.out

commitbt.bin: commitbt.lm ./../colm/colm
	./../colm/colm commitbt.lm
sprintf.diff: sprintf.out sprintf.exp
	@diff -u sprintf.exp sprintf.out > sprintf.diff || ( cat sprintf.diff; rm sprintf.diff )

sprintf.out: sprintf.bin
	./sprintf.bin  > sprintf.out

sprintf.bin: sprintf.lm ./../colm/colm
	./../colm/colm sprintf.lm
div.diff: div.out div.exp
	@diff -u div.exp div.out > div.diff || ( cat div.diff; rm div.diff )

div.out: div.bin
	./div.bin  > div.out

div.bin: div.lm ./../colm/colm
	./../colm/colm div.lm
scope1.diff: scope1.out scope1.exp
	@diff -u scope1.exp scope1.out > scope1.diff || ( cat scope1.diff; rm scope1.diff )

scope1.out: scope1.bin
	./scope1.bin  > scope1.out

scope1.bin: scope1.lm ./../colm/colm
	./../colm/colm scope1.lm
