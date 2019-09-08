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
#include "rlscan.h"
#include "vector.h"
#ifdef WITH_RAGEL_KELBT
#include "rlparse.h"
#endif
#include "parsedata.h"
#include "avltree.h"
#include "vector.h"

using std::istream;
using std::ostream;

extern char *Parser6_lelNames[];
struct Section;

struct Scanner
{
	Scanner( InputData *id, const char *fileName, istream &input,
			Parser6 *inclToParser, char *inclSectionTarg,
			int includeDepth, bool importMachines )
	: 
		id(id), fileName(fileName), 
		input(input),
		inclToParser(inclToParser),
		inclSectionTarg(inclSectionTarg),
		includeDepth(includeDepth),
		importMachines(importMachines),
		cur_token(0),
		line(1), column(1), lastnl(0), 
		parser(0), ignoreSection(false), 
		parserExistsError(false),
		whitespaceOn(true),
		lastToken(0),
		section(0),
		sectionPass(false)
	{}

	void handleMachine();
	void handleInclude();
	void handleImport();

	void init();
	void token( int type, char *start, char *end );
	void token( int type, char c );
	void token( int type );
	void processToken( int type, char *tokdata, int toklen );
	void directToParser( Parser6 *toParser, const char *tokFileName, int tokLine, 
		int tokColumn, int type, char *tokdata, int toklen );
	void flushImport( );
	void importToken( int type, char *start, char *end );
	void pass( int token, char *start, char *end );
	void pass();
	void updateCol();
	void startSection();
	void endSection();
	void do_scan();
	bool active();
	InputLoc scan_loc();

	InputData *id;
	const char *fileName;
	istream &input;
	Parser6 *inclToParser;
	char *inclSectionTarg;
	int includeDepth;
	bool importMachines;

	/* For import parsing. */
	int tok_cs, tok_act;
	int *tok_ts, *tok_te;
	int cur_token;
	static const int max_tokens = 32;
	int token_data[max_tokens];
	char *token_strings[max_tokens];
	int token_lens[max_tokens];

	/* For section processing. */
	int cs;
	char *word, *lit;
	int word_len, lit_len;

	/* For character scanning. */
	int line;
	InputLoc sectionLoc;
	char *ts, *te;
	int column;
	char *lastnl;

	/* Set by machine statements, these persist from section to section
	 * allowing for unnamed sections. */
	Parser6 *parser;
	bool ignoreSection;

	/* This is set if ragel has already emitted an error stating that
	 * no section name has been seen and thus no parser exists. */
	bool parserExistsError;

	/* This is for inline code. By default it is on. It goes off for
	 * statements and values in inline blocks which are parsed. */
	bool whitespaceOn;

	/* Keeps a record of the previous token sent to the section parser. */
	int lastToken;

	Section *section;
	bool sectionPass;

};

#endif
