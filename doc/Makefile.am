#
#   Copyright 2001-2007 Adrian Thurston <thurston@complang.org>
#

man_MANS = ragel.1 

EXTRA_DIST = ragel-guide.tex \
	bmconcat.fig bmregex.fig dropdown.fig exdoneact.fig \
	exoutact1.fig exstrongsubtr.fig lines2.fig smallscanner.fig bmnull.fig \
	comments1.fig entryguard.fig exinter.fig exoutact2.fig exsubtr.fig \
	lmkleene.fig stembed.fig bmnum.fig comments2.fig exaction.fig \
	exnegate.fig explus.fig finguard.fig opconcat.fig bmor.fig conds1.fig \
	exallact.fig exoption.fig exstact.fig leftguard.fig opor.fig \
	bmrange.fig conds2.fig exconcat.fig exor.fig exstar.fig lines1.fig \
	opstar.fig

if BUILD_MANUAL

doc_DATA = ragel-guide.pdf

.fig.pdf:
	fig2dev -L pdf $< $@

.tex.pdf:
	pdflatex -interaction=nonstopmode $< >/dev/null
	pdflatex -interaction=nonstopmode $< >/dev/null
	pdflatex -interaction=nonstopmode $< >/dev/null

version.tex: Makefile
	echo '|def|version{$(PACKAGE_VERSION)}' | tr '|' '\\' > version.tex
	echo '|def|pubdate{$(PUBDATE)}' | tr '|' '\\' >> version.tex

ragel-guide.tex: generate ragel-guide.txt
	./generate < ragel-guide.txt > ragel-guide.tex
	
generate: generate.lm
	$(COLM) $<

ragel-guide.pdf: version.tex

ragel-guide.pdf: bmconcat.pdf bmregex.pdf dropdown.pdf exdoneact.pdf \
	exoutact1.pdf exstrongsubtr.pdf lines2.pdf smallscanner.pdf bmnull.pdf \
	comments1.pdf entryguard.pdf exinter.pdf exoutact2.pdf exsubtr.pdf \
	lmkleene.pdf stembed.pdf bmnum.pdf comments2.pdf exaction.pdf \
	exnegate.pdf explus.pdf finguard.pdf opconcat.pdf bmor.pdf conds1.pdf \
	exallact.pdf exoption.pdf exstact.pdf leftguard.pdf opor.pdf \
	bmrange.pdf conds2.pdf exconcat.pdf exor.pdf exstar.pdf lines1.pdf \
	opstar.pdf

endif
