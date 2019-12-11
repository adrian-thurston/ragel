SUBDIRS = rust pcre dns c++ python go

all: rust pcre dns
	for d in $(SUBDIRS); do ( cd $$d && $(MAKE) ); done
