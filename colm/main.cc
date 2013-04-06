/*
 *  Copyright 2006-2012 Adrian Thurston <thurston@complang.org>
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
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "global.h"
#include "debug.h"
#include "pcheck.h"
#include "vector.h"
#include "version.h"
#include "keyops.h"
#include "parsedata.h"
#include "vector.h"
#include "version.h"
#include "fsmcodegen.h"
#include "colm.h"

#if defined(CONS_INIT)
#include "consinit.h"
#elif defined(LOAD_COLM)
#include "conscolm.h"
#else
#include "load.h"
#endif

using std::istream;
using std::ifstream;
using std::ostream;
using std::ios;
using std::cin;
using std::cout;
using std::cerr;
using std::endl;

/* Graphviz dot file generation. */
bool genGraphviz = false;

using std::ostream;
using std::istream;
using std::ifstream;
using std::ofstream;
using std::ios;
using std::cout;
using std::cerr;
using std::cin;
using std::endl;

InputLoc internal;

/* Io globals. */
istream *inStream = 0;
ostream *outStream = 0;
const char *inputFileName = 0;
const char *outputFileName = 0;
const char *gblExportTo = 0;
const char *gblExpImplTo = 0;
bool exportCode = false;

bool generateGraphviz = false;
bool verbose = false;
bool logging = false;
bool branchPointInfo = false;
bool addUniqueEmptyProductions = false;
bool gblLibrary = false;
long gblActiveRealm = 0;

ArgsVector includePaths;

/* Print version information. */
void version();

/* Total error count. */
int gblErrorCount = 0;

HostType hostTypesC[] =
{
	{ "char", 0, CHAR_MIN, CHAR_MAX, sizeof(char) },
};

HostLang hostLangC = { hostTypesC, 8, hostTypesC+0, true };
HostLang *hostLang = &hostLangC;

/* Print the opening to an error in the input, then return the error ostream. */
ostream &error( const InputLoc &loc )
{
	/* Keep the error count. */
	gblErrorCount += 1;

	if ( loc.fileName != 0 )
		cerr << loc.fileName << ":";
	else
		cerr << "<input>:";

	if ( loc.line == -1 ) {
		cerr << "INT: ";
	}
	else {
		cerr << loc.line << ":" << loc.col << ": ";
	}
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
"   -v --version         print version information and exit\n"
"   -o <file>            write output to <file>\n"
"   -i                   show conflict information\n"
"   -d                   make colm verbose\n"
"   -l                   compile logging into the output executable\n"
	;	
}

/* Print version information. */
void version()
{
	cout << "Colm version " VERSION << " " PUBDATE << endl <<
			"Copyright (c) 2007-2012 by Adrian D. Thurston" << endl;
}

/* Scans a string looking for the file extension. If there is a file
 * extension then pointer returned points to inside the string
 * passed in. Otherwise returns null. */
const char *findFileExtension( const char *stemFile )
{
	const char *ppos = stemFile + strlen(stemFile) - 1;

	/* Scan backwards from the end looking for the first dot.
	 * If we encounter a '/' before the first dot, then stop the scan. */
	while ( 1 ) {
		/* If we found a dot or got to the beginning of the string then
		 * we are done. */
		if ( ppos == stemFile || *ppos == '.' )
			break;

		/* If we hit a / then there is no extension. Done. */
		if ( *ppos == '/' ) {
			ppos = stemFile;
			break;
		}
		ppos--;
	} 

	/* If we got to the front of the string then bail we 
	 * did not find an extension  */
	if ( ppos == stemFile )
		ppos = 0;

	return ppos;
}

/* Make a file name from a stem. Removes the old filename suffix and
 * replaces it with a new one. Returns a newed up string. */
char *fileNameFromStem( const char *stemFile, const char *suffix )
{
	int len = strlen( stemFile );
	assert( len > 0 );

	/* Get the extension. */
	const char *ppos = findFileExtension( stemFile );

	/* If an extension was found, then shorten what we think the len is. */
	if ( ppos != 0 )
		len = ppos - stemFile;

	/* Make the return string from the stem and the suffix. */
	char *retVal = new char[ len + strlen( suffix ) + 1 ];
	strncpy( retVal, stemFile, len );
	strcpy( retVal + len, suffix );

	return retVal;
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
			const char *defExtension = ".c";
			outputFileName = fileNameFromStem( inputFileName, defExtension );
		}
	}

	//debug( REALM_PARSE, "opening output file: %s\n", outputFileName );

	/* Make sure we are not writing to the same file as the input file. */
	if ( outputFileName != 0 && inputFileName != 0 &&
			strcmp( inputFileName, outputFileName  ) == 0 )
	{
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

void openExports( )
{
	/* Make sure we are not writing to the same file as the input file. */
	if ( inputFileName != 0 && gblExportTo != 0 && strcmp( inputFileName, gblExportTo  ) == 0 ) {
		error() << "output file \"" << gblExportTo  << 
				"\" is the same as the input file" << endl;
	}

	if ( gblExportTo != 0 ) {
		/* Open the output stream, attaching it to the filter. */
		ofstream *outFStream = new ofstream( gblExportTo );

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

void openExportsImpl( )
{
	/* Make sure we are not writing to the same file as the input file. */
	if ( inputFileName != 0 && gblExpImplTo != 0 && strcmp( inputFileName, gblExpImplTo  ) == 0 ) {
		error() << "output file \"" << gblExpImplTo  << 
				"\" is the same as the input file" << endl;
	}

	if ( gblExpImplTo != 0 ) {
		/* Open the output stream, attaching it to the filter. */
		ofstream *outFStream = new ofstream( gblExpImplTo );

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

void compileOutputCommand( const char *command )
{
	//cout << "compiling with: " << command << endl;
	int res = system( command );
	if ( res != 0 )
		error() << "there was a problem compiling the output" << endl;
}

void compileOutputInstalled( const char *argv0 )
{
	/* Find the location of the colm program that is executing. */
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
		"gcc -Wall -Wwrite-strings"
		" -I" PREFIX "/include"
		" -g"
		" -o %s"
		" %s"
		" -L" PREFIX "/lib"
		" -lcolmd",
		exec, outputFileName );

	compileOutputCommand( command );
}

void compileOutputInSource( const char *argv0 )
{
	/* Find the location of the colm program that is executing. */
	char *location = strdup( argv0 );
	char *last = strrchr( location, '/' );
	assert( last != 0 );
	last[1] = 0;

	char *exec = fileNameFromStem( outputFileName, ".bin" );

	int length = 1024 + 3*strlen(location) + strlen(outputFileName) + strlen(exec);
	char command[length];
	sprintf( command, 
		"gcc -Wall -Wwrite-strings"
		" -I%s.."
		" -I%s../aapl"
		" -g"
		" -o %s"
		" %s"
		" -L%s"
		" -lcolmd",
		location, location,
		exec, outputFileName, location );
	
	compileOutputCommand( command );
}

bool inSourceTree( const char *argv0 )
{
	const char *lastSlash = strrchr( argv0, '/' );
	if ( lastSlash != 0 ) {
		int rootLen = lastSlash - argv0 + 1;
		char *mainPath = new char[rootLen + 16];
		memcpy( mainPath, argv0, rootLen );
		strcpy( mainPath + rootLen, "main.cc" );

		struct stat sb;
		int res = stat( mainPath, &sb );
		delete[] mainPath;

		if ( res == 0 && S_ISREG( sb.st_mode ) )
			return true;
	}

	return false;
}

void processArgs( int argc, const char **argv )
{
	ParamCheck pc( "D:e:c:LI:vdlio:S:M:vHh?-:sV", argc, argv );

	while ( pc.check() ) {
		switch ( pc.state ) {
		case ParamCheck::match:
			switch ( pc.parameter ) {
			case 'I':
				includePaths.append( pc.parameterArg );
				break;
			case 'v':
				version();
				exit(0);
				break;
			case 'd':
				verbose = true;
				break;
			case 'l':
				logging = true;
				break;
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

			case 'H': case 'h': case '?':
				usage();
				exit(0);
			case 's':
				printStatistics = true;
				break;
			case 'V':
				generateGraphviz = true;
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
				break;
			case 'L':
				gblLibrary = true;
				break;
			case 'e':
				gblExportTo = pc.parameterArg;
				break;
			case 'c':
				gblExpImplTo = pc.parameterArg;
				break;
			case 'D':
#if DEBUG
				if ( strcmp( pc.parameterArg, "BYTECODE" ) == 0 )
					gblActiveRealm |= REALM_BYTECODE;
				else if ( strcmp( pc.parameterArg, "PARSE" ) == 0 )
					gblActiveRealm |= REALM_PARSE;
				else if ( strcmp( pc.parameterArg, "MATCH" ) == 0 )
					gblActiveRealm |= REALM_MATCH;
				else if ( strcmp( pc.parameterArg, "COMPILE" ) == 0 )
					gblActiveRealm |= REALM_COMPILE;
				else if ( strcmp( pc.parameterArg, "POOL" ) == 0 )
					gblActiveRealm |= REALM_POOL;
				else if ( strcmp( pc.parameterArg, "PRINT" ) == 0 )
					gblActiveRealm |= REALM_PRINT;
				else if ( strcmp( pc.parameterArg, "INPUT" ) == 0 )
					gblActiveRealm |= REALM_INPUT;
				else if ( strcmp( pc.parameterArg, "SCAN" ) == 0 )
					gblActiveRealm |= REALM_SCAN;
				else
					fatal( "unknown argument to -D %s\n", pc.parameterArg );
#else
				fatal( "-D option specified but debugging messsages not compiled in\n" );
#endif
				
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
	processArgs( argc, argv );

	if ( verbose )
		gblActiveRealm = 0xffffffff;

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


#if defined(CONS_COLM) || defined(LOAD_SRC)
	/* Open the input file for reading. */
	if ( inputFileName == 0 ) {
		error() << "colm: no input file given" << endl;
	}
	else {
		/* Open the input file for reading. */
		ifstream *inFile = new ifstream( inputFileName );
		if ( ! inFile->is_open() )
			error() << "could not open " << inputFileName << " for reading" << endl;
		delete inFile;
	}
#endif

	/* Bail on above errors. */
	if ( gblErrorCount > 0 )
		exit(1);

	Compiler *pd = new Compiler;

#if defined(CONS_INIT)
	BaseParser *parser = new ConsInit( pd );
#elif defined(LOAD_COLM)
	BaseParser *parser = new LoadColm( pd, inputFileName );
#else
	BaseParser *parser = consLoadSource( pd, inputFileName );
#endif

	parser->go( gblActiveRealm );

	/* Parsing complete, check for errors.. */
	if ( gblErrorCount > 0 )
		return 1;

	/* Initiate a compile following a parse. */
	pd->compile();

	/*
	 * Write output.
	 */
	if ( generateGraphviz ) {
		outStream = &cout;
		pd->writeDotFile();
	}
	else {
		openOutput();
		pd->generateOutput( gblActiveRealm );
		if ( outStream != 0 )
			delete outStream;

		if ( !gblLibrary ) {
			if ( inSourceTree( argv[0] ) )
				compileOutputInSource( argv[0] );
			else
				compileOutputInstalled( argv[0] );
		}

		if ( gblExportTo != 0 )  {
			openExports();
			pd->generateExports();
			delete outStream;
		}
		if ( gblExpImplTo != 0 )  {
			openExportsImpl();
			pd->generateExportsImpl();
			delete outStream;
		}
	}

	delete parser;
	delete pd;

	/* Bail on above errors. */
	if ( gblErrorCount > 0 )
		exit(1);

	return 0;
}
