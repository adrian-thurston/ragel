/*
 *  Copyright 2001-2007 Adrian Thurston <thurston@cs.queensu.ca>
 */

/*  This file is part of Ragel.
 *
 *  Ragel is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 * 
 *  Ragel is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 * 
 *  You should have received a copy of the GNU General Public License
 *  along with Ragel; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA 
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <unistd.h>
#include <sstream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

/* Parsing. */
#include "ragel.h"
#include "rlscan.h"

/* Parameters and output. */
#include "pcheck.h"
#include "vector.h"
#include "version.h"
#include "common.h"

using std::istream;
using std::ostream;
using std::ifstream;
using std::ofstream;
using std::cin;
using std::cout;
using std::cerr;
using std::endl;
using std::ios;
using std::streamsize;

/* Controls minimization. */
MinimizeLevel minimizeLevel = MinimizePartition2;
MinimizeOpt minimizeOpt = MinimizeMostOps;

/* Graphviz dot file generation. */
char *machineSpec = 0, *machineName = 0;
bool machineSpecFound = false;

bool printStatistics = false;

typedef Vector<char*> ArgsVector;
ArgsVector backendArgs;

/* Print a summary of the options. */
void usage()
{
	cout <<
"usage: ragel [options] file\n"
"general:\n"
"   -h, -H, -?, --help   Print this usage and exit\n"
"   -v, --version        Print version information and exit\n"
"   -o <file>            Write output to <file>\n"
"   -s                   Print some statistics on stderr\n"
"fsm minimization:\n"
"   -n                   Do not perform minimization\n"
"   -m                   Minimize at the end of the compilation\n"
"   -l                   Minimize after most operations (default)\n"
"   -e                   Minimize after every operation\n"
"machine selection:\n"
"   -S <spec>            FSM specification to output (for rlgen-dot)\n"
"   -M <machine>         Machine definition/instantiation to output (for rlgen-dot)\n"
"host language:\n"
"   -C                   The host language is C, C++, Obj-C or Obj-C++ (default)\n"
"   -D                   The host language is D\n"
"   -J                   The host language is Java\n"
"   -R                   The host language is Ruby\n"
	;	
}

/* Print version information. */
void version()
{
	cout << "Ragel State Machine Compiler version " VERSION << " " PUBDATE << endl <<
			"Copyright (c) 2001-2007 by Adrian Thurston" << endl;
}

/* Total error count. */
int gblErrorCount = 0;

/* Print the opening to a warning in the input, then return the error ostream. */
ostream &warning( const InputLoc &loc )
{
	assert( loc.fileName != 0 );
	cerr << loc.fileName << ":" << loc.line << ":" << 
			loc.col << ": warning: ";
	return cerr;
}

/* Print the opening to a program error, then return the error stream. */
ostream &error()
{
	gblErrorCount += 1;
	cerr << PROGNAME ": ";
	return cerr;
}

ostream &error( const InputLoc &loc )
{
	gblErrorCount += 1;
	assert( loc.fileName != 0 );
	cerr << loc.fileName << ":" << loc.line << ": ";
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

/* If any forward slash is found in argv0 then it is assumed that the path is
 * explicit and the path to the backend executable should be derived from
 * that. If no forward slash is found it is assumed the file is being run from
 * the installed location. The PREFIX supplied during configuration is used.
 * */
char **makePathChecks( const char *argv0, const char *progName )
{
	char **result = new char*[3];
	const char *lastSlash = strrchr( argv0, '/' );
	int numChecks = 0;

	if ( lastSlash != 0 ) {
		char *path = strdup( argv0 );
		int givenPathLen = (lastSlash - argv0) + 1;
		path[givenPathLen] = 0;

		int progNameLen = strlen(progName);
		int length = givenPathLen + progNameLen + 1;
		char *check = new char[length];
		sprintf( check, "%s%s", path, progName );
		result[numChecks++] = check;

		length = givenPathLen + 3 + progNameLen + 1 + progNameLen + 1;
		check = new char[length];
		sprintf( check, "%s../%s/%s", path, progName, progName );
		result[numChecks++] = check;
	}
	else {
		int prefixLen = strlen(PREFIX);
		int progNameLen = strlen(progName);
		int length = prefixLen + 5 + progNameLen + 1;
		char *check = new char[length];

		sprintf( check, PREFIX "/bin/%s", progName );
		result[numChecks++] = check;
	}

	result[numChecks] = 0;
	return result;
}


void execBackend( const char *argv0, costream *intermed )
{
	/* Locate the backend program */
	const char *progName = 0;
	switch ( hostLang->lang ) {
		case HostLang::C:
		case HostLang::D:
			progName = "rlgen-cd";
			break;
		case HostLang::Java:
			progName = "rlgen-java";
			break;
		case HostLang::Ruby:
			progName = "rlgen-ruby";
			break;
	}

	char **pathChecks = makePathChecks( argv0, progName );

	backendArgs.insert( 0, "rlgen-ruby" );
	backendArgs.append( intermed->b->fileName );
	backendArgs.append( 0 );

	pid_t pid = fork();
	if ( pid < 0 ) {
		/* Error, no child created. */
		error() << "failed to fork backend" << endp;
	}
	else if ( pid == 0 ) {
		/* child */
		while ( *pathChecks != 0 ) {
			execv( *pathChecks, backendArgs.data );
			pathChecks += 1;
		}
		error() << "failed to exec backend" << endp;
	}
	else {
		/* parent. */
		wait( 0 );
	}

	unlink( intermed->b->fileName );
}

char *makeIntermedTemplate( char *baseFileName )
{
	char *result;
	char *lastSlash = strrchr( baseFileName, '/' );
	if ( lastSlash == 0 ) {
		result = new char[13];
		strcpy( result, "ragel-XXXXXX.xml" );
	}
	else {
		int baseLen = lastSlash - baseFileName + 1;
		result = new char[baseLen + 13];
		memcpy( result, baseFileName, baseLen );
		strcpy( result+baseLen, "ragel-XXXXXX.xml" );
	}
	return result;
};

char fnChars[] = "abcdefghijklmnopqrstuvwxyz0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";

costream *openIntermed( char *inputFileName, char *outputFileName )
{
	srandom(time(0));
	costream *result = 0;

	/* Which filename do we use as the base? */
	char *baseFileName = outputFileName != 0 ? outputFileName : inputFileName;

	/* The template for the intermediate file name. */
	char *intermedFileName = makeIntermedTemplate( baseFileName );

	/* Randomize the name and try to open. */
	char *firstX = strrchr( intermedFileName, 'X' ) - 5;
	for ( int tries = 0; tries < 20; tries++ ) {
		/* Choose a random name. */
		for ( int x = 0; x < 6; x++ )
			firstX[x] = fnChars[random() % 52];

		/* Try to open the file. */
		int fd = ::open( intermedFileName, O_WRONLY|O_EXCL|O_CREAT, S_IRUSR|S_IWUSR );

		if ( fd > 0 ) {
			/* Success. */
			FILE *file = fdopen( fd, "wt" );
			if ( file == 0 )
				error() << "fdopen(...) on intermediate file failed" << endp;

			cfilebuf *b = new cfilebuf( intermedFileName, file );
			result = new costream( b );
			break;
		}

		if ( errno == EACCES ) {
			error() << "failed to open temp file " << intermedFileName << 
					", access denied" << endp;
		}
	}

	if ( result == 0 )
		error() << "abnormal error: cannot find unique name for temp file" << endp;

	return result;
}

/* Main, process args and call yyparse to start scanning input. */
int main(int argc, char **argv)
{
	ParamCheck pc("o:nmleabjkS:M:CDJRvHh?-:sT:F:G:P:Lp", argc, argv);
	char *inputFileName = 0;
	char *outputFileName = 0;

	while ( pc.check() ) {
		switch ( pc.state ) {
		case ParamCheck::match:
			switch ( pc.parameter ) {
			/* Output. */
			case 'o':
				if ( *pc.parameterArg == 0 )
					error() << "a zero length output file name was given" << endl;
				else if ( outputFileName != 0 )
					error() << "more than one output file name was given" << endl;
				else {
					/* Ok, remember the output file name. */
					outputFileName = pc.parameterArg;
					backendArgs.append( "-o" );
					backendArgs.append( pc.parameterArg );
				}
				break;

			/* Minimization, mostly hidden options. */
			case 'n':
				minimizeOpt = MinimizeNone;
				break;
			case 'm':
				minimizeOpt = MinimizeEnd;
				break;
			case 'l':
				minimizeOpt = MinimizeMostOps;
				break;
			case 'e':
				minimizeOpt = MinimizeEveryOp;
				break;
			case 'a':
				minimizeLevel = MinimizeApprox;
				break;
			case 'b':
				minimizeLevel = MinimizeStable;
				break;
			case 'j':
				minimizeLevel = MinimizePartition1;
				break;
			case 'k':
				minimizeLevel = MinimizePartition2;
				break;

			/* Machine spec. */
			case 'S':
				if ( *pc.parameterArg == 0 )
					error() << "please specify an argument to -S" << endl;
				else if ( machineSpec != 0 )
					error() << "more than one -S argument was given" << endl;
				else {
					/* Ok, remember the path to the machine to generate. */
					machineSpec = pc.parameterArg;
				}
				break;

			/* Machine path. */
			case 'M':
				if ( *pc.parameterArg == 0 )
					error() << "please specify an argument to -M" << endl;
				else if ( machineName != 0 )
					error() << "more than one -M argument was given" << endl;
				else {
					/* Ok, remember the machine name to generate. */
					machineName = pc.parameterArg;
				}
				break;

			/* Host language types. */
			case 'C':
				hostLang = &hostLangC;
				break;
			case 'D':
				hostLang = &hostLangD;
				break;
			case 'J':
				hostLang = &hostLangJava;
				break;
			case 'R':
				hostLang = &hostLangRuby;
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

			/* Passthrough args. */
			case 'T': 
				backendArgs.append( "-T" );
				backendArgs.append( pc.parameterArg );
				break;
			case 'F': 
				backendArgs.append( "-F" );
				backendArgs.append( pc.parameterArg );
				break;
			case 'G': 
				backendArgs.append( "-G" );
				backendArgs.append( pc.parameterArg );
				break;
			case 'P':
				backendArgs.append( "-P" );
				backendArgs.append( pc.parameterArg );
				break;
			case 'p':
				backendArgs.append( "-p" );
				break;
			case 'L':
				backendArgs.append( "-L" );
				break;
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

	/* Bail on above errors. */
	if ( gblErrorCount > 0 )
		exit(1);

	/* Make sure we are not writing to the same file as the input file. */
	if ( inputFileName != 0 && outputFileName != 0 && 
			strcmp( inputFileName, outputFileName  ) == 0 )
	{
		error() << "output file \"" << outputFileName  << 
				"\" is the same as the input file" << endp;
	}

	/* Open the input file for reading. */
	istream *inStream;
	if ( inputFileName != 0 ) {
		/* Open the input file for reading. */
		ifstream *inFile = new ifstream( inputFileName );
		inStream = inFile;
		if ( ! inFile->is_open() )
			error() << "could not open " << inputFileName << " for reading" << endp;
	}
	else {
		inputFileName = "<stdin>";
		inStream = &cin;
	}

	/* Used for just a few things. */
	std::ostringstream hostData;

	if ( machineSpec == 0 && machineName == 0 )
		hostData << "<host line=\"1\" col=\"1\">";

	Scanner scanner( inputFileName, *inStream, hostData, 0, 0, 0, false );
	scanner.do_scan();

	/* Finished, final check for errors.. */
	if ( gblErrorCount > 0 )
		return 1;
	
	/* Now send EOF to all parsers. */
	terminateAllParsers();

	/* Finished, final check for errors.. */
	if ( gblErrorCount > 0 )
		return 1;

	if ( machineSpec == 0 && machineName == 0 )
		hostData << "</host>\n";

	if ( gblErrorCount > 0 )
		return 1;
	
	costream *intermed = openIntermed( inputFileName, outputFileName );

	/* Write the machines, then the surrounding code. */
	writeMachines( *intermed, hostData.str(), inputFileName );

	/* Close the intermediate file. */
	intermed->fclose();

	/* Run the backend process. */
	execBackend( argv[0], intermed );

	return 0;
}
