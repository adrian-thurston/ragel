#
#   Copyright 2001-2007 Adrian Thurston <thurston@complang.org>
#

#   This file is part of Ragel.
#
#   Ragel is free software; you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation; either version 2 of the License, or
#   (at your option) any later version.
#
#   Ragel is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with Ragel; if not, write to the Free Software
#   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA 

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

dist_doc_DATA = ragel-guide.pdf

.fig.pdf:
	fig2dev -L pdf $< $@

.tex.pdf:
	pdflatex -interaction=nonstopmode $< >/dev/null
	pdflatex -interaction=nonstopmode $< >/dev/null
	pdflatex -interaction=nonstopmode $< >/dev/null

version.tex: Makefile
	echo '|def|version{$(PACKAGE_VERSION)}' | tr '|' '\\' > version.tex
	echo '|def|pubdate{$(PUBDATE)}' | tr '|' '\\' >> version.tex

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
