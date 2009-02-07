/*
 *  Copyright 2001-2007 Adrian Thurston <thurston@complang.org>
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

#ifndef _COLM_H
#define _COLM_H

#include <stdio.h>
#include <iostream>
#include <fstream>
#include <fstream>
#include <string>

#include "config.h"
#include "avltree.h"
#include "keyops.h"

#define PROGNAME "colm"

/* IO filenames and stream. */
extern bool printPrintables;
extern bool graphvizDone;

extern int gblErrorCount;

std::ostream &error();

/* IO filenames and stream. */
extern const char *outputFileName;
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

struct exit_object { };
extern exit_object endp;
void operator<<( std::ostream &out, exit_object & );

extern std::ostream *outStream;
extern bool printStatistics;

extern int gblErrorCount;
extern char machineMain[];

/* Location in an input file. */
struct InputLoc
{
	const char *fileName;
	int line;
	int col;
};

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

#endif /* _COLM_H */
