SUBDIRS = "rust pcre dns"

all: rust pcre dns
	for d in $(SUBDIRS); do cd $$d && $(MAKE); done


