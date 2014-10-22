/*
 *  Copyright 2006-2012 Adrian Thurston <thurston@complang.org>
 */

#ifndef __COLM_GLOBAL_H
#define __COLM_GLOBAL_H

#include <stdio.h>
#include <iostream>
#include <fstream>
#include <fstream>
#include <string>

#include "config.h"
#include "defs.h"
#include "avltree.h"
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

#endif
