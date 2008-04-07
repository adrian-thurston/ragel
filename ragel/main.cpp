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
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#ifndef WIN32
#include <sys/wait.h>
#else
#include <windows.h>
#include <psapi.h>
#endif

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
bool wantDupsRemoved = true;

bool printStatistics = false;
bool frontendOnly = false;
bool generateDot = false;

ArgsVector frontendArgs;
ArgsVector backendArgs;
ArgsVector includePaths;

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
"   -d                   Do not remove duplicates from action lists\n"
"   -I <dir>             Add <dir> to the list of directories to search\n"
"                        for included an imported files\n"
"fsm minimization:\n"
"   -n                   Do not perform minimization\n"
"   -m                   Minimize at the end of the compilation\n"
"   -l                   Minimize after most operations (default)\n"
"   -e                   Minimize after every operation\n"
"visualization:\n"
"   -x                   Run the frontend only: emit XML intermediate format\n"
"   -V                   Generate a dot file for Graphviz\n"
"   -p                   Display printable characters on labels\n"
"   -S <spec>            FSM specification to output (for rlgen-dot)\n"
"   -M <machine>         Machine definition/instantiation to output (for rlgen-dot)\n"
"host language:\n"
"   -C                   The host language is C, C++, Obj-C or Obj-C++ (default)\n"
"   -D                   The host language is D\n"
"   -J                   The host language is Java\n"
"   -R                   The host language is Ruby\n"
"   -A                   The host language is C#\n"
"line direcives: (C/D/C# only)\n"
"   -L                   Inhibit writing of #line directives\n"
"code style: (C/Ruby/C# only)\n"
"   -T0                  Table driven FSM (default)\n"
"   -T1                  Faster table driven FSM\n"
"   -F0                  Flat table driven FSM\n"
"   -F1                  Faster flat table-driven FSM\n"
"code style: (C/C# only)\n"
"   -G0                  Goto-driven FSM\n"
"   -G1                  Faster goto-driven FSM\n"
"code style: (C only)\n"
"   -G2                  Really fast goto-driven FSM\n"
"   -P<N>                N-Way Split really fast goto-driven FSM\n"
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

void processArgs( int argc, char **argv, char *&inputFileName, char *&outputFileName )
{
	ParamCheck pc("xo:dnmleabjkS:M:I:CDJRAvHh?-:sT:F:G:P:LpV", argc, argv);

	while ( pc.check() ) {
		switch ( pc.state ) {
		case ParamCheck::match:
			switch ( pc.parameter ) {
			case 'V':
				generateDot = true;
				break;

			case 'x':
				frontendOnly = true;
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

			/* Minimization, mostly hidden options. */
			case 'd':
				wantDupsRemoved = false;
				frontendArgs.append( "-d" );
				break;

			/* Minimization, mostly hidden options. */
			case 'n':
				minimizeOpt = MinimizeNone;
				frontendArgs.append( "-n" );
				break;
			case 'm':
				minimizeOpt = MinimizeEnd;
				frontendArgs.append( "-m" );
				break;
			case 'l':
				minimizeOpt = MinimizeMostOps;
				frontendArgs.append( "-l" );
				break;
			case 'e':
				minimizeOpt = MinimizeEveryOp;
				frontendArgs.append( "-e" );
				break;
			case 'a':
				minimizeLevel = MinimizeApprox;
				frontendArgs.append( "-a" );
				break;
			case 'b':
				minimizeLevel = MinimizeStable;
				frontendArgs.append( "-b" );
				break;
			case 'j':
				minimizeLevel = MinimizePartition1;
				frontendArgs.append( "-j" );
				break;
			case 'k':
				minimizeLevel = MinimizePartition2;
				frontendArgs.append( "-k" );
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
					frontendArgs.append( "-S" );
					frontendArgs.append( pc.parameterArg );
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
					frontendArgs.append( "-M" );
					frontendArgs.append( pc.parameterArg );
				}
				break;

			case 'I':
				if ( *pc.parameterArg == 0 )
					error() << "please specify an argument to -I" << endl;
				else {
					includePaths.append( pc.parameterArg );
					frontendArgs.append( "-I" );
					frontendArgs.append( pc.parameterArg );
				}
				break;

			/* Host language types. */
			case 'C':
				hostLang = &hostLangC;
				frontendArgs.append( "-C" );
				break;
			case 'D':
				hostLang = &hostLangD;
				frontendArgs.append( "-D" );
				break;
			case 'J':
				hostLang = &hostLangJava;
				frontendArgs.append( "-J" );
				break;
			case 'R':
				hostLang = &hostLangRuby;
				frontendArgs.append( "-R" );
				break;
			case 'A':
				hostLang = &hostLangCSharp;
				frontendArgs.append( "-A" );
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
				frontendArgs.append( "-s" );
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
}

int frontend( char *inputFileName, char *outputFileName )
{
	/* Open the input file for reading. */
	assert( inputFileName != 0 );
	ifstream *inFile = new ifstream( inputFileName );
	istream *inStream = inFile;
	if ( ! inFile->is_open() )
		error() << "could not open " << inputFileName << " for reading" << endp;

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
	
	ostream *outputFile = 0;
	if ( outputFileName != 0 )
		outputFile = new ofstream( outputFileName );
	else
		outputFile = &cout;

	/* Write the machines, then the surrounding code. */
	writeMachines( *outputFile, hostData.str(), inputFileName );

	/* Close the intermediate file. */
	if ( outputFileName != 0 )
		delete outputFile;

	return gblErrorCount > 0;
}

char *makeIntermedTemplate( char *baseFileName )
{
	char *result = 0;
	const char *templ = "ragel-XXXXXX.xml";
	char *lastSlash = strrchr( baseFileName, '/' );
	if ( lastSlash == 0 ) {
		result = new char[strlen(templ)+1];
		strcpy( result, templ );
	}
	else {
		int baseLen = lastSlash - baseFileName + 1;
		result = new char[baseLen + strlen(templ) + 1];
		memcpy( result, baseFileName, baseLen );
		strcpy( result+baseLen, templ );
	}
	return result;
};

char *openIntermed( char *inputFileName, char *outputFileName )
{
	srand(time(0));
	char *result = 0;

	/* Which filename do we use as the base? */
	char *baseFileName = outputFileName != 0 ? outputFileName : inputFileName;

	/* The template for the intermediate file name. */
	char *intermedFileName = makeIntermedTemplate( baseFileName );

	/* Randomize the name and try to open. */
	char fnChars[] = "abcdefghijklmnopqrstuvwxyz0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
	char *firstX = strrchr( intermedFileName, 'X' ) - 5;
	for ( int tries = 0; tries < 20; tries++ ) {
		/* Choose a random name. */
		for ( int x = 0; x < 6; x++ )
			firstX[x] = fnChars[rand() % 52];

		/* Try to open the file. */
		int fd = ::open( intermedFileName, O_WRONLY|O_EXCL|O_CREAT, S_IRUSR|S_IWUSR );

		if ( fd > 0 ) {
			/* Success. Close the file immediately and return the name for use
			 * by the child processes. */
			::close( fd );
			result = intermedFileName;
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


void cleanExit( char *intermed, int status )
{
	unlink( intermed );
	exit( status );
}

#ifndef WIN32

/* If any forward slash is found in argv0 then it is assumed that the path is
 * explicit and the path to the backend executable should be derived from
 * that. Whe check that location and also go up one then inside a directory of
 * the same name in case we are executing from the source tree. If no forward
 * slash is found it is assumed the file is being run from the installed
 * location. The PREFIX supplied during configuration is used. */
char **makePathChecksUnix( const char *argv0, const char *progName )
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


void forkAndExec( const char *progName, char **pathChecks, 
		ArgsVector &args, char *intermed )
{
	pid_t pid = fork();
	if ( pid < 0 ) {
		/* Error, no child created. */
		error() << "failed to fork for " << progName << endl;
		cleanExit( intermed, 1 );
	}
	else if ( pid == 0 ) {
		/* child */
		while ( *pathChecks != 0 ) {
			/* Execv does not modify argv, it just uses the const form that is
			 * compatible with the most code. Ours not included. */
			execv( *pathChecks, (char *const*) args.data );
			pathChecks += 1;
		}
		error() << "failed to exec " << progName << endl;
		cleanExit( intermed, 1 );
	}

	/* Parent process, wait for the child. */
	int status;
	wait( &status );

	/* What happened with the child. */
	if ( ! WIFEXITED( status ) ) {
		error() << progName << " did not exit normally" << endl;
		cleanExit( intermed, 1 );
	}
	
	if ( WEXITSTATUS(status) != 0 )
		cleanExit( intermed, WEXITSTATUS(status) );
}

#else

/* GetModuleFileNameEx is used to find out where the the current process's
 * binary is. That location is searched first. If that fails then we go up one
 * directory and look for the executable inside a directory of the same name
 * in case we are executing from the source tree.
 * */
char **makePathChecksWin( const char *progName )
{
	int len = 1024;
	char *imageFileName = new char[len];
	HANDLE h = GetCurrentProcess();
	len = GetModuleFileNameEx( h, NULL, imageFileName, len );
	imageFileName[len] = 0;

	char **result = new char*[3];
	const char *lastSlash = strrchr( imageFileName, '\\' );
	int numChecks = 0;

	assert( lastSlash != 0 );
	char *path = strdup( imageFileName );
	int givenPathLen = (lastSlash - imageFileName) + 1;
	path[givenPathLen] = 0;

	int progNameLen = strlen(progName);
	int length = givenPathLen + progNameLen + 1;
	char *check = new char[length];
	sprintf( check, "%s%s", path, progName );
	result[numChecks++] = check;

	length = givenPathLen + 3 + progNameLen + 1 + progNameLen + 1;
	check = new char[length];
	sprintf( check, "%s..\\%s\\%s", path, progName, progName );
	result[numChecks++] = check;

	result[numChecks] = 0;
	return result;
}

void spawn( const char *progName, char **pathChecks, 
		ArgsVector &args, char *intermed )
{
	int result = 0;
	while ( *pathChecks != 0 ) {
		//cerr << "trying to execute " << *pathChecks << endl;
		result = _spawnv( _P_WAIT, *pathChecks, args.data );
		if ( result >= 0 || errno != ENOENT )
			break;
		pathChecks += 1;
	}

	if ( result < 0 ) {
		error() << "failed to spawn " << progName << endl;
		cleanExit( intermed, 1 );
	}

	if ( result > 0 )
		cleanExit( intermed, 1 );
}

#endif

void execFrontend( const char *argv0, char *inputFileName, char *intermed )
{
	/* The frontend program name. */
	const char *progName = "ragel";

	frontendArgs.insert( 0, progName );
	frontendArgs.insert( 1, "-x" );
	frontendArgs.append( "-o" );
	frontendArgs.append( intermed );
	frontendArgs.append( inputFileName );
	frontendArgs.append( 0 );

#ifndef WIN32
	char **pathChecks = makePathChecksUnix( argv0, progName );
	forkAndExec( progName, pathChecks, frontendArgs, intermed );
#else
	char **pathChecks = makePathChecksWin( progName );
	spawn( progName, pathChecks, frontendArgs, intermed );
#endif
}

void execBackend( const char *argv0, char *intermed, char *outputFileName )
{
	/* Locate the backend program */
	const char *progName = 0;
	if ( generateDot )
		progName = "rlgen-dot";
	else {
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
			case HostLang::CSharp:
				progName = "rlgen-csharp";
		}
	}

	backendArgs.insert( 0, progName );
	if ( outputFileName != 0 ) {
		backendArgs.append( "-o" );
		backendArgs.append( outputFileName );
	}
	backendArgs.append( intermed );
	backendArgs.append( 0 );

#ifndef WIN32
	char **pathChecks = makePathChecksUnix( argv0, progName );
	forkAndExec( progName, pathChecks, backendArgs, intermed );
#else
	char **pathChecks = makePathChecksWin( progName );
	spawn( progName, pathChecks, backendArgs, intermed );
#endif
}

/* Main, process args and call yyparse to start scanning input. */
int main(int argc, char **argv)
{
	char *inputFileName = 0;
	char *outputFileName = 0;

	processArgs( argc, argv, inputFileName, outputFileName );

	/* If -M or -S are given and we're not generating a dot file then invoke
	 * the frontend. These options are not useful with code generators. */
	if ( machineName != 0 || machineSpec != 0 ) {
		if ( !generateDot )
			frontendOnly = true;
	}

	/* Require an input file. If we use standard in then we won't have a file
	 * name on which to base the output. */
	if ( inputFileName == 0 )
		error() << "no input file given" << endl;

	/* Bail on argument processing errors. */
	if ( gblErrorCount > 0 )
		exit(1);

	/* Make sure we are not writing to the same file as the input file. */
	if ( inputFileName != 0 && outputFileName != 0 && 
			strcmp( inputFileName, outputFileName  ) == 0 )
	{
		error() << "output file \"" << outputFileName  << 
				"\" is the same as the input file" << endp;
	}

	if ( frontendOnly )
		return frontend( inputFileName, outputFileName );

	char *intermed = openIntermed( inputFileName, outputFileName );

	/* From here on in the cleanExit function should be used to exit. */

	/* Run the frontend, then the backend processes. */
	execFrontend( argv[0], inputFileName, intermed );
	execBackend( argv[0], intermed, outputFileName );

	/* Clean up the intermediate. */
	cleanExit( intermed, 0 );

	return 0;
}
