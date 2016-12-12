#
# Copyright 2001-2016 Adrian Thurston <thurston@colm.net>
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to
# deal in the Software without restriction, including without limitation the
# rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
# sell copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#  
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#  
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
#

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
