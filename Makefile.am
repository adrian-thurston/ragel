
SUBDIRS = ragel aapl

no_parsers = 's/build_parsers=yes/build_parsers=no/'

dist-hook:
	cp $(top_distdir)/configure $(top_distdir)/configure.tmp
	cp $(top_distdir)/configure.in $(top_distdir)/configure.in.tmp
	sed $(no_parsers) $(top_distdir)/configure.tmp > $(top_distdir)/configure
	sed $(no_parsers) $(top_distdir)/configure.in.tmp > $(top_distdir)/configure.in
	rm $(top_distdir)/configure.tmp $(top_distdir)/configure.in.tmp
