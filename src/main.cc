/*
 *  Copyright 2001-2015 Adrian Thurston <thurston@complang.org>
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
#include <iomanip>
#include <fstream>
#include <unistd.h>
#include <sstream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#ifdef _WIN32
#include <windows.h>
#include <psapi.h>
#include <time.h>
#include <io.h>
#include <process.h>

#if _MSC_VER
#define S_IRUSR _S_IREAD
#define S_IWUSR _S_IWRITE
#endif
#endif

/* Parsing. */
#include "ragel.h"

/* Parameters and output. */
#include "pcheck.h"
#include "vector.h"
#include "version.h"
#include "common.h"
#include "inputdata.h"

using std::istream;
using std::ostream;
using std::ifstream;
using std::ofstream;
using std::cin;
using std::endl;
using std::ios;
using std::streamsize;

/* Print a summary of the options. */
void InputData::usage()
{
	info() <<
"usage: ragel [options] file\n"
"general:\n"
"   -h, -H, -?, --help   Print this usage and exit\n"
"   -v, --version        Print version information and exit\n"
"   -o <file>            Write output to <file>\n"
"   -s                   Print some statistics and compilation info to stderr\n"
"   -d                   Do not remove duplicates from action lists\n"
"   -I <dir>             Add <dir> to the list of directories to search\n"
"                        for included an imported files\n"
"   --rlhc               Show the rlhc command used to compile\n"
"   --save-temps         Do not delete intermediate file during compilation\n"
"   --no-intermediate    Disable call to rlhc, leave behind intermediate\n"
"error reporting format:\n"
"   --error-format=gnu   file:line:column: message (default)\n"
"   --error-format=msvc  file(line,column): message\n"
"fsm minimization:\n"
"   -n                   Do not perform minimization\n"
"   -m                   Minimize at the end of the compilation\n"
"   -l                   Minimize after most operations (default)\n"
"   -e                   Minimize after every operation\n"
"visualization:\n"
"   -V                   Generate a dot file for Graphviz\n"
"   -p                   Display printable characters on labels\n"
"   -S <spec>            FSM specification to output (for graphviz output)\n"
"   -M <machine>         Machine definition/instantiation to output (for\n"
"                        graphviz output)\n"
"host language:\n"
"   -C                   C, C++, Obj-C or Obj-C++ (default)\n"
"                        All code styles supported.\n"
"   --asm --gas-x86-64-sys-v\n"
"                        GNU AS, x86_64, System V ABI.\n"
"                        Generated in a code style equivalent to -G2\n"
"line directives:\n"
"   -L                   Inhibit writing of #line directives\n"
"code style:\n"
"   -T0                  Binary search (default)\n"
"   -T1                  Binary search with expanded actions \n"
"   -F0                  Flat table\n"
"   -F1                  Flat table with expanded actions\n"
"   -G0                  Switch-driven\n"
"   -G1                  Switch-driven with expanded actions\n"
"   -G2                  Goto-driven with expanded actions\n"
"large machines:\n"
"   --integral-tables    Use integers for table data (default)\n"
"   --string-tables      Encode table data into strings for faster host lang\n"
"                        compilation (C)\n"
"analysis:\n"
"   --prior-interaction          Search for condition-based general repetitions\n"
"                                that will not function properly due to state mod\n"
"                                overlap and must be NFA reps. \n"
"   --conds-depth=D              Search for high-cost conditions inside a prefix\n"
"                                of the machine (depth D from start state).\n"
"   --state-limit=L              Report fail if number of states exceeds this\n"
"                                during compilation.\n"
"   --breadth-check=E1,E2,..     Report breadth cost of named entry points and\n"
"                                the start state.\n"
"   --input-histogram=FN         Input char histogram for breadth check. If\n"
"                                unspecified a flat histogram is used.\n"
"testing:\n"
"   --kelbt-frontend        Compile using original ragel + kelbt frontend\n"
"                           Requires ragel be built with ragel + kelbt support\n"
"   --colm-frontend         Compile using a colm-based recursive descent\n"
"                           frontend\n"
"   --reduce-frontend       Compile using a colm-based reducer (default)\n"
"   --direct-backend        Use the direct backend for supported langs (default)\n"
"   --colm-backend          Use the translation backed for C\n"
"   --var-backend           Use the variable-based backend for langs that\n"
"                           support goto-based\n"
"   --goto-backend          Use the goto-based backend for supported langs\n"
"                           (default)\n"
"   --supported-host-langs  Show supported host languages by command line arg\n"
"   --supported-frontends   Show supported frontends\n"
"   --supported-backends    Show supported backends\n"
"   --force-libragel        Cause mainline to behave like libragel\n"
	;	

	abortCompile( 0 );
}

/* Print version information and exit. */
void InputData::version()
{
	info() << "Ragel State Machine Compiler version " VERSION << " " PUBDATE << endl <<
			"Copyright (c) 2001-2015 by Adrian Thurston" << endl;
	abortCompile( 0 );
}

void InputData::showHostLangNames()
{
	ostream &out = info();
	for ( int i = 0; i < numHostLangs; i++ ) {
		if ( i > 0 )
			out  << " ";
		out << hostLangs[i]->name;
	}
	out << endl;
	abortCompile( 0 );
}

void InputData::showHostLangArgs()
{
	ostream &out = info();
	for ( int i = 0; i < numHostLangs; i++ ) {
		if ( i > 0 )
			out << " ";
		out << hostLangs[i]->arg;
	}
	out << endl;
	abortCompile( 0 );
}

void InputData::showFrontends()
{
	ostream &out = info();
	out << "--colm-frontend";
	out << " --reduce-frontend";
#ifdef WITH_RAGEL_KELBT
	out << " --kelbt-frontend";
#endif
	out << endl;
	abortCompile( 0 );
}

void InputData::showBackends()
{
	info() << 
		"--direct-backend --colm-backend" << endl;
	abortCompile( 0 );
}

void InputData::showStyles()
{
	switch ( hostLang->lang ) {
	case HostLang::C:
		info() << "-T0 -T1 -F0 -F1 -G0 -G1 -G2" << endl;
		break;
	case HostLang::Asm:
		info() << "-G2" << endl;
		break;
	}

	abortCompile( 0 );
}

InputLoc makeInputLoc( const char *fileName, int line, int col )
{
	InputLoc loc( fileName, line, col );
	return loc;
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

void InputData::parseArgs( int argc, const char **argv )
{
	ParamCheck pc( "r:o:dnmleabjkS:M:I:CEvHh?-:sT:F:G:LpV", argc, argv );

	bool showStylesOpt = false;

	/* Decide if we were invoked using a path variable, or with an explicit path. */
	const char *lastSlash = strrchr( argv[0], '/' );
	if ( lastSlash == 0 ) {
		/* Defualt to the the binary install location. */
		dirName = BINDIR;
	}
	else {
		/* Compute dirName from argv0. */
		dirName = string( argv[0], lastSlash - argv[0] );
	}

	/* FIXME: Need to check code styles VS langauge. */

	while ( pc.check() ) {
		switch ( pc.state ) {
		case ParamCheck::match:
			switch ( pc.parameter ) {
			case 'V':
				generateDot = true;
				break;

			/* Output. */
			case 'o':
				if ( *pc.paramArg == 0 )
					error() << "a zero length output file name was given" << endl;
				else if ( outputFileName != 0 )
					error() << "more than one output file name was given" << endl;
				else {
					/* Ok, remember the output file name. */
					outputFileName = new char[strlen(pc.paramArg)+1];
					strcpy( (char*)outputFileName, pc.paramArg );
				}
				break;

			case 'r':
				commFileName = pc.paramArg;
				break;

			/* Flag for turning off duplicate action removal. */
			case 'd':
				wantDupsRemoved = false;
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
			#ifdef TO_UPGRADE_CONDS
				minimizeLevel = MinimizeApprox;
			#else
				error() << "minimize approx (-a) unsupported in this version" << endp;
			#endif
				break;
			case 'b':
			#ifdef TO_UPGRADE_CONDS
				minimizeLevel = MinimizeStable;
			#else
				error() << "minimize stable (-b) unsupported in this version" << endp;
			#endif
				break;
			case 'j':
				minimizeLevel = MinimizePartition1;
				break;
			case 'k':
				minimizeLevel = MinimizePartition2;
				break;

			/* Machine spec. */
			case 'S':
				if ( *pc.paramArg == 0 )
					error() << "please specify an argument to -S" << endl;
				else if ( machineSpec != 0 )
					error() << "more than one -S argument was given" << endl;
				else {
					/* Ok, remember the path to the machine to generate. */
					machineSpec = pc.paramArg;
				}
				break;

			/* Machine path. */
			case 'M':
				if ( *pc.paramArg == 0 )
					error() << "please specify an argument to -M" << endl;
				else if ( machineName != 0 )
					error() << "more than one -M argument was given" << endl;
				else {
					/* Ok, remember the machine name to generate. */
					machineName = pc.paramArg;
				}
				break;

			case 'I':
				if ( *pc.paramArg == 0 )
					error() << "please specify an argument to -I" << endl;
				else {
					includePaths.append( pc.paramArg );
				}
				break;

			/* Host language types. */
			case 'C':
				hostLang = &hostLangC;
				break;

			/* Version and help. */
			case 'v':
				version();
				break;
			case 'H': case 'h': case '?':
				usage();
				break;
			case 's':
				printStatistics = true;
				break;
			case '-': {
				char *arg = strdup( pc.paramArg );
				char *eq = strchr( arg, '=' );

				if ( eq != 0 )
					*eq++ = 0;

				if ( strcmp( arg, "help" ) == 0 )
					usage();
				else if ( strcmp( arg, "version" ) == 0 )
					version();
				else if ( strcmp( arg, "error-format" ) == 0 ) {
					if ( eq == 0 )
						error() << "expecting '=value' for error-format" << endl;
					else if ( strcmp( eq, "gnu" ) == 0 )
						errorFormat = ErrorFormatGNU;
					else if ( strcmp( eq, "msvc" ) == 0 )
						errorFormat = ErrorFormatMSVC;
					else
						error() << "invalid value for error-format" << endl;
				}
				else if ( strcmp( arg, "rbx" ) == 0 )
					rubyImpl = Rubinius;
				else if ( strcmp( arg, "rlhc" ) == 0 )
					rlhcShowCmd = true;
				else if ( strcmp( arg, "no-intermediate" ) == 0 )
					noIntermediate = true;
#ifdef WITH_RAGEL_KELBT
				else if ( strcmp( arg, "kelbt-frontend" ) == 0 ) {
					frontend = KelbtBased;
					frontendSpecified = true;
				}
#else
				else if ( strcmp( arg, "kelbt-frontend" ) == 0 ) {
					error() << "--kelbt-frontend specified but, "
							"ragel not built with ragel+kelbt support" << endp;
				}
#endif
				else if ( strcmp( arg, "reduce-frontend" ) == 0 ) {
					frontend = ReduceBased;
					frontendSpecified = true;
				}
				else if ( strcmp( arg, "asm" ) == 0 )
					hostLang = &hostLangAsm;
				else if ( strcmp( arg, "gnu-asm-x86-64-sys-v" ) == 0 )
					hostLang = &hostLangAsm;
				else if ( strcmp( arg, "direct" ) == 0 ) {
					backend = Direct;
					backendSpecified = true;
				}
				else if ( strcmp( arg, "direct-backend" ) == 0 ) {
					backend = Direct;
					backendSpecified = true;
				}
				else if ( strcmp( arg, "colm-backend" ) == 0 ) {
					backend = Translated;
					backendSpecified = true;
				}
				else if ( strcmp( arg, "var-backend" ) == 0 ) {
					/* Forces variable-based backend, even if the target
					 * language supports the goto-based backend. May require
					 * --colm-backend depending on the target language (C for
					 * example is direct by default). */
					backendFeature = VarFeature;
					featureSpecified = true;
				}
				else if ( strcmp( arg, "goto-backend" ) == 0 ) {
					/* Forces goto-based based backend. */
					backendFeature = GotoFeature;
					featureSpecified = true;
				}
				else if ( strcmp( arg, "string-tables" ) == 0 )
					stringTables = true;
				else if ( strcmp( arg, "integral-tables" ) == 0 )
					stringTables = false;
				else if ( strcmp( arg, "host-lang-names" ) == 0 )
					showHostLangNames();
				else if ( strcmp( arg, "host-lang-args" ) == 0 || 
						strcmp( arg, "supported-host-langs" ) == 0 )
					showHostLangArgs();
				else if ( strcmp( arg, "supported-frontends" ) == 0 )
					showFrontends();
				else if ( strcmp( arg, "supported-backends" ) == 0 )
					showBackends();
				else if ( strcmp( arg, "supported-styles" ) == 0 )
					showStylesOpt = true;
				else if ( strcmp( arg, "save-temps" ) == 0 )
					saveTemps = true;
				else if ( strcmp( arg, "prior-interaction" ) == 0 )
					checkPriorInteraction = true;
				else if ( strcmp( arg, "conds-depth" ) == 0 )
					condsCheckDepth = strtol( eq, 0, 10 );
				else if ( strcmp( arg, "state-limit" ) == 0 )
					stateLimit = strtol( eq, 0, 10 );
				else if ( strcmp( arg, "breadth-check" ) == 0 ) {
					char *ptr = 0;
					while ( true ) {
						char *label = strtok_r( eq, ",", &ptr );
						eq = NULL;
						if ( label == NULL )
							break;
						breadthLabels.append( strdup( label ) );
					}
					checkBreadth = true;
				}
				else if ( strcmp( arg, "input-histogram" ) == 0 )
					histogramFn = strdup(eq);
				else if ( strcmp( arg, "force-libragel" ) == 0 )
					forceLibRagel = true;
				else {
					error() << "--" << pc.paramArg << 
							" is an invalid argument" << endl;
				}
				free( arg );
				break;
			}

			/* Passthrough args. */
			case 'T': 
				if ( pc.paramArg[0] == '0' )
					codeStyle = GenBinaryLoop;
				else if ( pc.paramArg[0] == '1' )
					codeStyle = GenBinaryExp;
				else {
					error() << "-T" << pc.paramArg[0] << 
							" is an invalid argument" << endl;
					abortCompile( 1 );
				}
				break;
			case 'F': 
				if ( pc.paramArg[0] == '0' )
					codeStyle = GenFlatLoop;
				else if ( pc.paramArg[0] == '1' )
					codeStyle = GenFlatExp;
				else {
					error() << "-F" << pc.paramArg[0] << 
							" is an invalid argument" << endl;
					abortCompile( 1 );
				}
				break;
			case 'G': 
				if ( pc.paramArg[0] == '0' )
					codeStyle = GenSwitchLoop;
				else if ( pc.paramArg[0] == '1' )
					codeStyle = GenSwitchExp;
				else if ( pc.paramArg[0] == '2' )
					codeStyle = GenIpGoto;
				else if ( pc.paramArg[0] == 'T' && pc.paramArg[1] == '2' ) {
					codeStyle = GenIpGoto;
					maxTransitions = 32;
				} else {
					error() << "-G" << pc.paramArg[0] << 
							" is an invalid argument" << endl;
					abortCompile( 1 );
				}
				break;

			case 'p':
				displayPrintables = true;
				break;

			case 'L':
				noLineDirectives = true;
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

	if ( showStylesOpt )
		showStyles();
}

bool langSupportsGoto( const HostLang *hostLang )
{
	return true;
}

void InputData::loadHistogram()
{
	const int alphsize = 256;

	/* Init a default. */
	histogram = new double[alphsize];
	ifstream h( histogramFn );
	if ( !h.is_open() )
		error() << "histogram read: failed to open file: " << histogramFn << endp;

	int i = 0;
	double value;
	while ( true ) {
		if ( h >> value ) {
			if ( i >= alphsize ) {
				/* Too many items. */
				error() << "histogram read: too many histogram values,"
						" expecting " << alphsize << " (for char alphabet)" << endp;
			}
			histogram[i] = value;
			i++;
		}
		else {
			/* Read failure. */
			if ( h.eof() ) {
				if ( i < alphsize ) {
					error() << "histogram read: fell short of " <<
							alphsize << " items" << endp;
				}
				break;
			}
			else {
				error() << "histogram read: error at item " << i << endp;
			}
		}
	}
}

void InputData::defaultHistogram()
{
	/* Flat histogram. */
	const int alphsize = 256;
	histogram = new double[alphsize];
	for ( int i = 0; i < alphsize; i++ ) {
		histogram[i] = 1.0 / (double)alphsize;
	}
}

void InputData::checkArgs()
{
	/* Require an input file. If we use standard in then we won't have a file
	 * name on which to base the output. */
	if ( inputFileName == 0 )
		error() << "no input file given" << endl;

	/* Bail on argument processing errors. */
	if ( errorCount > 0 )
		abortCompile( 1 );

	/* Make sure we are not writing to the same file as the input file. */
	if ( inputFileName != 0 && outputFileName != 0 && 
			strcmp( inputFileName, outputFileName  ) == 0 )
	{
		error() << "output file \"" << outputFileName  << 
				"\" is the same as the input file" << endp;
	}

	if ( !frontendSpecified )
		frontend = ReduceBased;

	if ( !backendSpecified ) {
		if ( hostLang->lang == HostLang::C || hostLang->lang == HostLang::Asm )
			backend = Direct;
		else
			backend = Translated;
	}

	if ( !featureSpecified ) {
		if ( langSupportsGoto( hostLang ) )
			backendFeature = GotoFeature;
		else
			backendFeature = VarFeature;
	}

	if ( checkBreadth ) {
		if ( histogramFn != 0 )
			loadHistogram();
		else
			defaultHistogram();
	}
}

/*
 * result: analysis output
 * out: contents of stdout
 * err: contents of stderr
 * argc, argv: ragel arguments, input file ignored
 * input: input file contents
 */
extern "C"
int libragel( char **result, char **out, char **err, int argc, const char **argv, const char *input )
{
	InputData id;

	try {
		id.inLibRagel = true;

		id.parseArgs( argc, argv );
		id.checkArgs();
		id.input = input;
		id.frontend = ReduceBased;
		id.process();
	}
	catch ( const AbortCompile &ac ) {
		*result = strdup( "" );
	}

	if ( result != 0 )
		*result = strdup( id.comm.c_str() );
	if ( out != 0 )
		*out = strdup( id.libcout.str().c_str() );
	if ( err != 0 )
		*err = strdup( id.libcerr.str().c_str() );
	return 0;
}

char *InputData::readInput( const char *inputFileName )
{
	struct stat st;
	int res = stat( inputFileName, &st );
	if ( res != 0 ) {
		error() << inputFileName << ": stat failed: " << strerror(errno) << endl;
		return 0;
	}

	std::ifstream in( inputFileName );
	if ( !in.is_open() ) {
		error() << inputFileName << ": could not open in force-libragel mode";
		return 0;
	}

	char *input = new char[st.st_size+1];
	in.read( input, st.st_size );
	if ( in.gcount() != st.st_size ) {
		error() << inputFileName << ": could not read in force-libragel mode";
		delete[] input;
		return 0;
	}
	input[st.st_size] = 0;

	return input;
}

/* Main, process args and call yyparse to start scanning input. */
int main( int argc, const char **argv )
{
	int code = 0;
	InputData id;
	try {
		id.parseArgs( argc, argv );
		id.checkArgs();

		if ( id.forceLibRagel ) {
			id.inLibRagel = true;
			id.input = id.readInput( id.inputFileName );
			if ( id.input != 0 ) {
				id.frontend = ReduceBased;
				if ( !id.process() )
					id.abortCompile( 1 );
			}

		}
		else {
			if ( !id.process() )
				id.abortCompile( 1 );
		}
	}
	catch ( const AbortCompile &ac ) {
		code = ac.code;

	}

	if ( id.comm.size() > 0 ) {
		if ( id.commFileName == 0 ) {
			std::cout << id.comm;
		}
		else {
			ofstream ofs( id.commFileName, std::fstream::app );
			ofs << id.comm;
			ofs.close();
		}
	}

	if ( id.forceLibRagel ) {
		std::cerr << id.libcerr.str();
		std::cout << id.libcout.str();
		std::cerr.flush();
		std::cout.flush();
	}
	return code;
}

