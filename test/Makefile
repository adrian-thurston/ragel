#
#   Copyright 2001, 2002 Adrian Thurston <thurston@cs.queensu.ca>
#

#   This file is part of Aapl.
#
#   Aapl is free software; you can redistribute it and/or modify it under the
#   terms of the GNU Lesser General Public License as published by the Free
#   Software Foundation; either version 2.1 of the License, or (at your option)
#   any later version.
#
#   Aapl is distributed in the hope that it will be useful, but WITHOUT ANY
#   WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
#   FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
#   more details.
#
#   You should have received a copy of the GNU Lesser General Public License
#   along with Aapl; if not, write to the Free Software Foundation, Inc., 59
#   Temple Place, Suite 330, Boston, MA 02111-1307 USA

LIBS = 

CFLAGS = -g -Wall -I../src

STRESS = \
	stress_avltree.cpp stress_avliter.cpp stress_avlmel.cpp \
	stress_avlmelkey.cpp stress_avlitree.cpp stress_avlmap.cpp \
	stress_avlset.cpp stress_avlimel.cpp stress_avlimelkey.cpp \
	stress_avlimap.cpp stress_avliset.cpp stress_svector.cpp \
	stress_allsort.cpp stress_stblsort.cpp

TEST = \
	test_vector.cpp test_bsttable.cpp test_bstmap.cpp \
	test_bstset.cpp test_doublelist.cpp test_dlistval.cpp \
	test_mergesort.cpp test_allavl.cpp test_avliter.cpp \
	test_quicksort.cpp test_insertsort.cpp test_bubblesort.cpp \
	test_allsort.cpp test_svector.cpp test_sbsttable.cpp \
	test_sbstmap.cpp test_sbstset.cpp test_avlkeyless.cpp \
	test_avlikeyless.cpp test_compare.cpp test_string.cpp


#*************************************

# Programs.
CXX		= g++


# Make targets from sources. 
STRESS_TARGS = $(STRESS:%.cpp=%.tst)
TEST_TARGS = $(TEST:%.cpp=%.tst)
TARGS = $(STRESS_TARGS) $(TEST_TARGS)
DEPS = $(STRESS:%.cpp=.deps/%.d) $(TEST:%.cpp=.deps/%.d) .deps/util.d

# Rules.
main: $(STRESS_TARGS) $(TEST_TARGS)

$(TARGS): %.tst: %.cpp util.o
	@test -d .deps || mkdir .deps
	@$(CXX) -M $(CFLAGS) $< \
		| sed 's/^\([a-zA-Z0-9_-]*\)\.o:/\1.tst:/;' > .deps/$*.d
	$(CXX) $(CFLAGS) -o $@ $< util.o $(LIBS)

distclean: clean
	rm -Rf .deps Makefile

clean:
	rm -Rf .deps *.tst *.o

-include $(DEPS)
