#
#   Copyright 2002, 2003 Adrian Thurston <thurston@cs.queensu.ca>
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



# Get the version info.
include ../version.mk

PREFIX = ../../aapl

all: Doxyfile 
	rm -Rf html
	mkdir html
	./makelinks
	./makedummy
	doxygen Doxyfile

Doxyfile: Doxyfile.in ../version.mk
	sed 's/@VERSION@/$(VERSION)/' Doxyfile.in > Doxyfile

clean:
	rm -Rf .deps Doxyfile html dummy
	find . -type l | xargs rm -f

