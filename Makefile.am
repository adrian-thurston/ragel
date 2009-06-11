
SUBDIRS = ragel aapl
DIST_SUBDIRS = $(SUBDIRS) contrib examples test doc

dist_doc_DATA = CREDITS ChangeLog
EXTRA_DIST = ragel.vim

# This file is checked for by the configure script and its existence causes the
# parsers and the manual to not be built when the distribution is built.
dist-hook:
	echo "This is a distribution of Ragel $(VERSION)." > $(distdir)/DIST

