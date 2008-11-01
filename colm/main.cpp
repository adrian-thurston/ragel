/*
 *  Copyright 2001-2007 Adrian Thurston <thurston@cs.queensu.ca>
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

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <unistd.h>
#include <sstream>

#include "colm.h"
#include "lmscan.h"
#include "pcheck.h"
#include "vector.h"
#include "version.h"
#include "common.h"
#include "parsedata.h"
#include "vector.h"
#include "version.h"
#include "fsmcodegen.h"
#include "dotgen.h"

using std::istream;
using std::ifstream;
using std::ostream;
using std::ios;
using std::cin;
using std::cout;
using std::cerr;
using std::endl;

/* Graphviz dot file generation. */
bool graphvizDone = false;

bool printPrintables = false;

using std::ostream;
using std::istream;
using std::ifstream;
using std::ofstream;
using std::ios;
using std::cout;
using std::cerr;
using std::cin;
using std::endl;

/* Target language and output style. */
char defExtension[] = ".cpp";

/* Io globals. */
istream *inStream = 0;
ostream *outStream = 0;
const char *inputFileName = 0;
const char *outputFileName = 0;

bool generateGraphviz = false;
bool branchPointInfo = false;
bool addUniqueEmptyProductions = false;

/* Print version information. */
void version();

/* Total error count. */
int gblErrorCount = 0;

/* Print the opening to an error in the input, then return the error ostream. */
ostream &error( const InputLoc &loc )
{
	/* Keep the error count. */
	gblErrorCount += 1;

	cerr << "error: " << inputFileName << ":" << 
			loc.line << ":" << loc.col << ": ";
	return cerr;
}

/* Print the opening to a program error, then return the error stream. */
ostream &error()
{
	gblErrorCount += 1;
	cerr << "error: " PROGNAME ": ";
	return cerr;
}


/* Print the opening to a warning, then return the error ostream. */
ostream &warning( )
{
	cerr << "warning: " << inputFileName << ": ";
	return cerr;
}

/* Print the opening to a warning in the input, then return the error ostream. */
ostream &warning( const InputLoc &loc )
{
	assert( inputFileName != 0 );
	cerr << "warning: " << inputFileName << ":" << 
			loc.line << ":" << loc.col << ": ";
	return cerr;
}

void escapeLineDirectivePath( std::ostream &out, char *path )
{
	for ( char *pc = path; *pc != 0; pc++ ) {
		if ( *pc == '\\' )
			out << "\\\\";
		else
			out << *pc;
	}
}

void escapeLineDirectivePath( std::ostream &out, char *path );
void scan( char *fileName, istream &input );

bool printStatistics = false;

/* Print a summary of the options. */
void usage()
{
	cout <<
"usage: colm [options] file\n"
"general:\n"
"   -h, -H, -?, --help   print this usage and exit\n"
"   -v, --version        print version information and exit\n"
"   -o <file>            write output to <file>\n"
"   -i                   show conflict information\n"
	;	
}

/* Print version information. */
void version()
{
	cout << "Colm version " VERSION << " " PUBDATE << endl <<
			"Copyright (c) 2007, 2008 by Adrian Thurston" << endl;
}

/* Invoked by the parser when the root element is opened. */
void openOutput( )
{
	/* If the output format is code and no output file name is given, then
	 * make a default. */
	if ( outputFileName == 0 ) {
		const char *ext = findFileExtension( inputFileName );
		if ( ext != 0 && strcmp( ext, ".rh" ) == 0 )
			outputFileName = fileNameFromStem( inputFileName, ".h" );
		else {
			const char *defExtension = ".cpp";
			outputFileName = fileNameFromStem( inputFileName, defExtension );
		}
	}

	#ifdef COLM_LOG_COMPILE
	cerr << "opening output file: " << outputFileName << endl;
	#endif

	/* Make sure we are not writing to the same file as the input file. */
	if ( outputFileName != 0 && strcmp( inputFileName, outputFileName  ) == 0 ) {
		error() << "output file \"" << outputFileName  << 
				"\" is the same as the input file" << endl;
	}

	if ( outputFileName != 0 ) {
		/* Open the output stream, attaching it to the filter. */
		ofstream *outFStream = new ofstream( outputFileName );

		if ( !outFStream->is_open() ) {
			error() << "error opening " << outputFileName << " for writing" << endl;
			exit(1);
		}

		outStream = outFStream;
	}
	else {
		/* Writing out ot std out. */
		outStream = &cout;
	}
}

void compileOutput( const char *argv0 )
{
	/* Find the location of us. */
	char *location = strdup( argv0 );
	char *last = location + strlen(location) - 1;
	while ( true ) {
		if ( last == location ) {
			last[0] = '.';
			last[1] = 0;
			break;
		}
		if ( *last == '/' ) {
			last[0] = 0;
			break;
		}
		last -= 1;
	}

	char *exec = fileNameFromStem( outputFileName, ".bin" );

	int length = 1024 + 3*strlen(location) + strlen(outputFileName) + strlen(exec);
	char command[length];
	sprintf( command, 
		"g++ -Wall -Wwrite-strings"
		" -I%s/../aapl"
		" -I%s/../colm"
		" -I%s/../common"
		" -g"
		" -o %s"
		" %s"
		" %s/../colm/runtime.a",
		location, location, location, exec, outputFileName, location );
	#ifdef COLM_LOG_COMPILE
	cout << "compiling: " << outputFileName << endl;
	#endif
	int res = system( command );
	if ( res != 0 )
		cout << "there was a problem compiling the output" << endl;
}

void process_args( int argc, const char **argv )
{
	ParamCheck pc( "io:S:M:vHh?-:s", argc, argv );

	while ( pc.check() ) {
		switch ( pc.state ) {
		case ParamCheck::match:
			switch ( pc.parameter ) {
			case 'i':
				branchPointInfo = true;
				break;
			/* Output. */
			case 'o':
				if ( *pc.parameterArg == 0 )
					error() << "a zero length output file name was given" << endl;
				else if ( outputFileName != 0 )
					error() << "more than one output file name was given" << endl;
				else {
					/* Ok, remember the output file name. */
					outputFileName = pc.parameterArg;
				}
				break;

			/* Version and help. */
			case 'v':
				version();
				exit(0);
			case 'H': case 'h': case '?':
				usage();
				exit(0);
			case 's':
				printStatistics = true;
				break;
			case '-':
				if ( strcasecmp(pc.parameterArg, "help") == 0 ) {
					usage();
					exit(0);
				}
				else if ( strcasecmp(pc.parameterArg, "version") == 0 ) {
					version();
					exit(0);
				}
				else {
					error() << "--" << pc.parameterArg << 
							" is an invalid argument" << endl;
				}
			}
			break;

		case ParamCheck::invalid:
			error() << "-" << pc.parameter << " is an invalid argument" << endl;
			break;

		case ParamCheck::noparam:
			/* It is interpreted as an input file. */
			if ( *pc.curArg == 0 )
				error() << "a zero length input file name was given" << endl;
			else if ( inputFileName != 0 )
				error() << "more than one input file name was given" << endl;
			else {
				/* OK, Remember the filename. */
				inputFileName = pc.curArg;
			}
			break;
		}
	}
}

/* Main, process args and call yyparse to start scanning input. */
int main(int argc, const char **argv)
{
	process_args( argc, argv );

	/* Bail on above errors. */
	if ( gblErrorCount > 0 )
		exit(1);

	/* Make sure we are not writing to the same file as the input file. */
	if ( inputFileName != 0 && outputFileName != 0 && 
			strcmp( inputFileName, outputFileName  ) == 0 )
	{
		error() << "output file \"" << outputFileName  << 
				"\" is the same as the input file" << endl;
	}

	/* Open the input file for reading. */
	istream *inStream;
	if ( inputFileName != 0 ) {
		/* Open the input file for reading. */
		ifstream *inFile = new ifstream( inputFileName );
		inStream = inFile;
		if ( ! inFile->is_open() )
			error() << "could not open " << inputFileName << " for reading" << endl;
	}
	else {
		inputFileName = "<stdin>";
		inStream = &cin;
	}

	/* Bail on above errors. */
	if ( gblErrorCount > 0 )
		exit(1);

	Scanner scanner( inputFileName, *inStream, cout, 0, 0, 0 );
	scanner.do_scan();

	/* Parsing complete, check for errors.. */
	if ( gblErrorCount > 0 )
		return 1;

	/* Initiate a compile following a parse. */
	scanner.parser->pd->semanticAnalysis();

	if ( outStream != 0 )
		delete outStream;

	compileOutput( argv[0] );

	return 0;
}
