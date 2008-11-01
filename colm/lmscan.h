/*
 *  Copyright 2007 Adrian Thurston <thurston@complang.org>
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

#include "colm.h"
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

/* This is used for tracking the current stack of include file/machine pairs. It is
 * is used to detect and recursive include structure. */
struct IncludeStackItem
{
	IncludeStackItem( char *fileName, char *sectionName )
		: fileName(fileName), sectionName(sectionName) {}

	char *fileName;
	char *sectionName;
};

typedef Vector<IncludeStackItem> IncludeStack;

struct Scanner
{
	Scanner( const char *fileName, istream &input, ostream &output,
			Parser *inclToParser, char *inclSectionTarg,
			int includeDepth )
	: 
		fileName(fileName), input(input), output(output),
		inclToParser(inclToParser),
		inclSectionTarg(inclSectionTarg),
		includeDepth(includeDepth),
		line(1), column(1), lastnl(0), 
		parserExistsError(false),
		whitespaceOn(true)
	{
		parser = new Parser( fileName, "machine", InputLoc() );
		parser->init();
	}

	bool recursiveInclude( char *inclFileName, char *inclSectionName );

	#if 0
	char *prepareFileName( char *fileName, int len )
	{
		bool caseInsensitive;
		Token tokenFnStr, tokenRes;
		tokenFnStr.data = fileName;
		tokenFnStr.length = len;
		tokenFnStr.prepareLitString( tokenRes, caseInsensitive );
		return tokenRes.data;
	}
	#endif

	void sectionParseInit();
	void token( int type, char *start, char *end );
	void token( int type, char c );
	void token( int type );
	void updateCol();
	void endSection();
	void do_scan();
	ostream &scan_error();

	const char *fileName;
	istream &input;
	ostream &output;
	Parser *inclToParser;
	char *inclSectionTarg;
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
	Parser *parser;
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
