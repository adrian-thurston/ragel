#
#   Copyright 2001-2016 Adrian Thurston <thurston@complang.org>
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

EXTRA_DIST = ragel-guide.txt \
	bmconcat.fig bmregex.fig dropdown.fig exdoneact.fig \
	exoutact1.fig exstrongsubtr.fig lines2.fig smallscanner.fig bmnull.fig \
	comments1.fig entryguard.fig exinter.fig exoutact2.fig exsubtr.fig \
	lmkleene.fig stembed.fig bmnum.fig comments2.fig exaction.fig \
	exnegate.fig explus.fig finguard.fig opconcat.fig bmor.fig conds1.fig \
	exallact.fig exoption.fig exstact.fig leftguard.fig opor.fig \
	bmrange.fig conds2.fig exconcat.fig exor.fig exstar.fig lines1.fig \
	opstar.fig

if BUILD_MANUAL

doc_DATA = ragel-guide.html

.fig.png:
	fig2dev -L png -S 4 $< $@

ragel-guide.html: bmconcat.png bmregex.png dropdown.png exdoneact.png \
	exoutact1.png exstrongsubtr.png lines2.png smallscanner.png bmnull.png \
	comments1.png entryguard.png exinter.png exoutact2.png exsubtr.png \
	lmkleene.png stembed.png bmnum.png comments2.png exaction.png \
	exnegate.png explus.png finguard.png opconcat.png bmor.png conds1.png \
	exallact.png exoption.png exstact.png leftguard.png opor.png \
	bmrange.png conds2.png exconcat.png exor.png exstar.png lines1.png \
	opstar.png

endif
