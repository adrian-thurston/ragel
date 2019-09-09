/*
 * Copyright 2007-2018 Adrian Thurston <thurston@colm.net>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef _RLSCAN_H
#define _RLSCAN_H

#include <iostream>
#include <fstream>
#include <string.h>

#include "global.h"
#include "lmparse.h"
#include "compiler.h"
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
