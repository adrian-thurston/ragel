#
#   Copyright 2001-2007 Adrian Thurston <thurston@complang.org>
#

#   This file is part of Colm.
#
#   Colm is free software; you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation; either version 2 of the License, or
#   (at your option) any later version.
#
#   Colm is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with Colm; if not, write to the Free Software
#   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA 


# ADT BUILD_SUBDIRS = colm
# ADT ALL_SUBDIRS = $(BUILD_SUBDIRS) test
# ADT 
# ADT #*************************************
# ADT 
# ADT # Programs
# ADT CXX = @CXX@
# ADT 
# ADT # Get the version info.
# ADT include version.mk
# ADT 
# ADT # build targets
# ADT all: $(BUILD_SUBDIRS)
# ADT 
# ADT .PHONY: $(BUILD_SUBDIRS)
# ADT 
# ADT $(BUILD_SUBDIRS):
# ADT 	@cd $@ && $(MAKE)
# ADT 
# ADT # clean targets.
# ADT 
# ADT CLEAN_SUBDIRS = $(ALL_SUBDIRS:%=%-clean)
# ADT 
# ADT .PHONY: $(CLEAN_SUBDIRS)
# ADT 
# ADT $(CLEAN_SUBDIRS):
# ADT 	@cd $(@:%-clean=%) && $(MAKE) clean
# ADT 
# ADT clean: $(CLEAN_SUBDIRS)
# ADT 	rm -f tags
# ADT 
# ADT # distcleaan targets
# ADT 
# ADT DISTCLEAN_SUBDIRS = $(ALL_SUBDIRS:%=%-distclean)
# ADT 
# ADT .PHONY: $(DISTCLEAN_SUBDIRS)
# ADT 
# ADT $(DISTCLEAN_SUBDIRS):
# ADT 	@cd $(@:%-distclean=%) && $(MAKE) clean
# ADT 
# ADT distclean: $(DISTCLEAN_SUBDIRS)
# ADT 	rm -f Makefile config.cache config.status config.log
# ADT 
# ADT #install targets
# ADT 
# ADT INSTALL_SUBDIRS = $(BUILD_SUBDIRS:%=%-install)
# ADT 
# ADT .PHONY: $(INSTALL_SUBDIRS)
# ADT 
# ADT $(INSTALL_SUBDIRS):
# ADT 	@cd $(@:%-install=%) && $(MAKE) install
# ADT 
# ADT install: $(INSTALL_SUBDIRS)
# ADT 

SUBDIRS = colm
DIST_SUBDIRS = $(SUBDIRS) aapl test

dist_doc_DATA =  colm.vim
EXTRA_DIST = colm.vim

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

