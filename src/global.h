/*
 * Copyright 2006-2012 Adrian Thurston <thurston@colm.net>
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

#ifndef _COLM_GLOBAL_H
#define _COLM_GLOBAL_H

#include <stdio.h>

#include <iostream>
#include <fstream>
#include <fstream>
#include <string>

#include <avltree.h>

#include "config.h"
#include "defs.h"
#include "keyops.h"

#define PROGNAME "colm"

/* IO filenames and stream. */
extern bool genGraphviz;
extern int gblErrorCount;

std::ostream &error();

/* IO filenames and stream. */
extern std::ostream *outStream;
extern bool generateGraphviz;
extern bool branchPointInfo;
extern bool verbose, logging;
extern bool addUniqueEmptyProductions;

extern int gblErrorCount;
extern char startDefName[];

/* Error reporting. */
std::ostream &error();
std::ostream &error( int first_line, int first_column );
std::ostream &warning( ); 
std::ostream &warning( int first_line, int first_column );

extern std::ostream *outStream;
extern bool printStatistics;

extern int gblErrorCount;
extern bool gblLibrary;
extern long gblActiveRealm;
extern char machineMain[];
extern const char *exportHeaderFn;

struct colm_location;

/* Location in an input file. */
struct InputLoc
{
	InputLoc( colm_location *pcloc );

	InputLoc() : fileName(0), line(-1), col(-1)  {}

	InputLoc( const InputLoc &loc )
	{
		fileName = loc.fileName;
		line = loc.line;
		col = loc.col;
	}

	const char *fileName;
	int line;
	int col;
};

extern InputLoc internal;

/* Error reporting. */
std::ostream &error();
std::ostream &error( const InputLoc &loc ); 
std::ostream &warning( const InputLoc &loc ); 

void scan( char *fileName, std::istream &input, std::ostream &output );
void terminateAllParsers( );
void checkMachines( );

void xmlEscapeHost( std::ostream &out, char *data, int len );
void openOutput();
void escapeLiteralString( std::ostream &out, const char *data );
bool readCheck( const char *fn );

#endif /* _COLM_GLOBAL_H */

