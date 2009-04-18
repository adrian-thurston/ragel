
SUBDIRS = ragel aapl doc test contrib

dist_doc_DATA = ChangeLog

sed_script = 's/build_parsers=yes/build_parsers=no/;s/build_manual=yes/build_manual=no/;'

configure = $(top_distdir)/configure

dist-hook:
	cp -p $(configure) $(configure).tmp
	cp -p $(configure).in $(configure).in.tmp
	sed $(sed_script) $(configure).tmp > $(configure)
	sed $(sed_script) $(cfgscripcfgscript).in.tmp > $(configure).in
	touch -r $(configure).tmp $(configure)
	touch -r $(configure).in.tmp $(configure)
	rm $(configure).tmp $(configure).in.tmp
