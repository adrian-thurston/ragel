
SUBDIRS = ragel aapl doc test contrib

dist_doc_DATA = ChangeLog

sed_script = 's/build_parsers=yes/build_parsers=no/;s/build_manual=yes/build_manual=no/;'

dist-hook:
	cp $(top_distdir)/configure $(top_distdir)/configure.tmp
	cp $(top_distdir)/configure.in $(top_distdir)/configure.in.tmp
	sed $(sed_script) $(top_distdir)/configure.tmp > $(top_distdir)/configure
	sed $(sed_script) $(top_distdir)/configure.in.tmp > $(top_distdir)/configure.in
	rm $(top_distdir)/configure.tmp $(top_distdir)/configure.in.tmp
