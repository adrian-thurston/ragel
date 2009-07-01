
SUBDIRS = ragel doc
DIST_SUBDIRS = $(SUBDIRS) aapl contrib examples test

dist_doc_DATA = CREDITS ChangeLog
EXTRA_DIST = ragel.vim

# This file is checked for by the configure script and its existence causes the
# parsers and the manual to not be built when the distribution is built.
dist-hook:
	( \
	echo "#!/bin/sh"; \
	echo ""; \
	echo "# Change to yes to enable building of parsers or manual. Reconfigure"; \
	echo "# afterwards."; \
	echo "build_parsers=no;"; \
	echo "build_manual=no;"; \
	) > $(distdir)/DIST

