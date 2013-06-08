/*
 *  Copyright 2007-2012 Adrian Thurston <thurston@complang.org>
 */

/*  This file is part of Colm.
 *
 *  Colm is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 * 
 *  Colm is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 * 
 *  You should have received a copy of the GNU General Public License
 *  along with Colm; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA 
 */

#ifndef _RLSCAN_H
#define _RLSCAN_H

#include <iostream>
#include <fstream>
#include <string.h>

#include "global.h"
#include "lmparse.h"
#include "parsedata.h"
#include "avltree.h"
#include "vector.h"
#include "buffer.h"

using std::ifstream;
using std::istream;
using std::ostream;
using std::cout;
using std::cerr;
using std::endl;

extern char *Parser_lelNames[];


struct ColmScanner
{
	ColmScanner( const char *fileName, istream &input, 
			ColmParser *parser, int includeDepth )
	: 
		fileName(fileName), input(input),
		includeDepth(includeDepth),
		line(1), column(1), lastnl(0), 
		parser(parser),
		parserExistsError(false),
		whitespaceOn(true)
	{
	}

	ifstream *tryOpenInclude( char **pathChecks, long &found );
	char **makeIncludePathChecks( const char *thisFileName, const char *fileName );
	bool recursiveInclude( const char *inclFileName );

	void sectionParseInit();
	void token( int type, char *start, char *end );
	void token( int type, char c );
	void token( int type );
	void updateCol();
	void endSection();
	void scan();
	void eof();
	ostream &scan_error();

	const char *fileName;
	istream &input;
	int includeDepth;

	int cs;
	int line;
	char *word, *lit;
	int word_len, lit_len;
	InputLoc sectionLoc;
	char *ts, *te;
	int column;
	char *lastnl;

	/* Set by machine statements, these persist from section to section
	 * allowing for unnamed sections. */
	ColmParser *parser;
	IncludeStack includeStack;

	/* This is set if ragel has already emitted an error stating that
	 * no section name has been seen and thus no parser exists. */
	bool parserExistsError;

	/* This is for inline code. By default it is on. It goes off for
	 * statements and values in inline blocks which are parsed. */
	bool whitespaceOn;

	Buffer litBuf;
};

#endif /* _RLSCAN_H */
