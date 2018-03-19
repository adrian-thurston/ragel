/*
 * Copyright 2008-2018 Adrian Thurston <thurston@colm.net>
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

#include "ragel.h"
#include "common.h"
#include "inputdata.h"
#include "parsedata.h"
#include "load.h"
#include "rlscan.h"
#include "reducer.h"
#include "version.h"
#include "pcheck.h"

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

using std::istream;
using std::ifstream;
using std::ofstream;
using std::stringstream;
using std::ostream;
using std::endl;
using std::ios;

InputData::~InputData()
{
	inputItems.empty();
	parseDataList.empty();
	sectionList.empty();

	for ( Vector<const char**>::Iter fns = streamFileNames; fns.lte(); fns++ ) {
		const char **ptr = *fns;
		while ( *ptr != 0 ) {
			::free( (void*)*ptr );
			ptr += 1;
		}
		free( (void*) *fns );
	}

	if ( outputFileName != 0 )
		delete[] outputFileName;

	if ( histogramFn != 0 )
		::free( (void*)histogramFn );

	if ( histogram != 0 )
		delete[] histogram;

	for ( ArgsVector::Iter bl = breadthLabels; bl.lte(); bl++ )
		free( (void*) *bl );
}

void InputData::makeDefaultFileName()
{
	if ( outputFileName == 0 )
		outputFileName = (hostLang->defaultOutFn)( inputFileName );
}

bool InputData::isBreadthLabel( const string &label )
{
	for ( ArgsVector::Iter bl = breadthLabels; bl.lte(); bl++ ) {
		if ( label == *bl )
			return true;
	}
	return false;
}

void InputData::createOutputStream()
{
	/* Make sure we are not writing to the same file as the input file. */
	if ( outputFileName != 0 ) {
		if ( strcmp( inputFileName, outputFileName  ) == 0 ) {
			error() << "output file \"" << outputFileName  << 
					"\" is the same as the input file" << endl;
		}

		/* Create the filter on the output and open it. */
		outFilter = new output_filter( outputFileName );

		/* Open the output stream, attaching it to the filter. */
		outStream = new ostream( outFilter );
	}
	else {
		/* Writing out to std out. */
		outStream = &std::cout;
	}
}

void InputData::openOutput()
{
	if ( outFilter != 0 ) {
		outFilter->open( outputFileName, ios::out|ios::trunc );
		if ( !outFilter->is_open() ) {
			error() << "error opening " << outputFileName << " for writing" << endl;
			abortCompile( 1 );
		}
	}
}

void InputData::prepareSingleMachine()
{
	ParseData *pd = 0;
	GraphDictEl *gdEl = 0;

	/* Locate a machine spec to generate dot output for. We can only emit.
	 * Dot takes one graph at a time. */
	if ( machineSpec != 0 ) {
		/* Machine specified. */
		ParseDataDictEl *pdEl = parseDataDict.find( machineSpec );
		if ( pdEl == 0 )
			error() << "could not locate machine specified with -S and/or -M" << endp;
		pd = pdEl->value;
	}
	else { 
		/* No machine spec given, generate the first one. */
		if ( parseDataList.length() == 0 )
			error() << "no machine specification to generate graphviz output" << endp;

		pd = parseDataList.head;
	}

	if ( machineName != 0 ) {
		gdEl = pd->graphDict.find( machineName );
		if ( gdEl == 0 )
			error() << "machine definition/instantiation not found" << endp;
	}
	else {
		/* We are using the whole machine spec. Need to make sure there
		 * are instances in the spec. */
		if ( pd->instanceList.length() == 0 )
			error() << "no machine instantiations to generate graphviz output" << endp;
	}

	pd->prepareMachineGen( gdEl, hostLang );
	dotGenPd = pd;
}

void InputData::prepareAllMachines()
{
	for ( ParseDataDict::Iter pdel = parseDataDict; pdel.lte(); pdel++ ) {
		ParseData *pd = pdel->value;
		if ( pd->instanceList.length() > 0 ) {
			pd->prepareMachineGen( 0, hostLang );

			pd->makeExports();
		}

	}
}

void InputData::generateReduced()
{
	for ( ParseDataDict::Iter pdel = parseDataDict; pdel.lte(); pdel++ ) {
		ParseData *pd = pdel->value;
		if ( pd->instanceList.length() > 0 )
			pd->generateReduced( inputFileName, codeStyle, *outStream, hostLang );
	}
}

void InputData::verifyWriteHasData( InputItem *ii )
{
	if ( ii->type == InputItem::Write ) {
		if ( ii->pd->cgd == 0 )
			error( ii->loc ) << ii->pd->sectionName << ": no machine instantiations to write" << endl;
	}
}

void InputData::verifyWritesHaveData()
{
	for ( InputItemList::Iter ii = inputItems; ii.lte(); ii++ )
		verifyWriteHasData( ii );
}

void InputData::writeOutput( InputItem *ii )
{
	switch ( ii->type ) {
		case InputItem::Write: {
			CodeGenData *cgd = ii->pd->cgd;
			cgd->writeStatement( ii->loc, ii->writeArgs.size(),
					ii->writeArgs, generateDot, hostLang );
			break;
		}
		case InputItem::HostData: {
			switch ( hostLang->backend ) {
				case Direct:
					if ( hostLang->_lang == HostLang::C ) {
						if ( ii->loc.fileName != 0 ) {
							if ( !noLineDirectives ) {
								*outStream << "\n#line " << ii->loc.line <<
										" \"" << ii->loc.fileName << "\"\n";
							}
						}
					}
						
					*outStream << ii->data.str();
					break;
				case Translated:
					openHostBlock( '@', this, *outStream, inputFileName, ii->loc.line );
					translatedHostData( *outStream, ii->data.str() );
					*outStream << "}@";
					break;
			}
			break;
		}
		case InputItem::EndSection: {
			break;
		}
	}
}

void InputData::writeOutput()
{
	for ( InputItemList::Iter ii = inputItems; ii.lte(); ii++ )
		writeOutput( ii );
}

void InputData::closeOutput()
{
	/* If writing to a file, delete the ostream, causing it to flush.
	 * Standard out is flushed automatically. */
	if ( outputFileName != 0 ) {
		delete outStream;
		delete outFilter;
	}
}

void InputData::processDot()
{
	/* Compiles the DOT machines. */
	prepareSingleMachine();

	if ( errorCount > 0 )
		abortCompile( 1 );

	createOutputStream();

	if ( errorCount > 0 )
		abortCompile( 1 );

	/*
	 * From this point on we should not be reporting any errors.
	 */

	openOutput();
	writeDot( *outStream );
	closeOutput();
}

bool InputData::checkLastRef( InputItem *ii )
{
	if ( generateDot )
		return true;
	
	if ( errorCount > 0 )
		return false;
		
	/*
	 * 1. Go forward to next last reference.
	 * 2. Fully process that machine, mark as processed.
	 * 3. Move forward through input items until no longer 
	 */
	if ( ii->section != 0 && ii->section->lastReference == ii ) {
		/* Fully Process. */
		ParseData *pd = ii->pd;

		if ( pd->instanceList.length() > 0 ) {
#ifdef WITH_RAGEL_KELBT
			if ( ii->parser != 0 ) 
				ii->parser->terminateParser();
#endif

			FsmRes res = pd->prepareMachineGen( 0, hostLang );

			/* Compute exports from the export definitions. */
			pd->makeExports();

			if ( !res.success() )
				return false;

			if ( errorCount > 0 )
				return false;

			pd->generateReduced( inputFileName, codeStyle, *outStream, hostLang );

			if ( errorCount > 0 )
				return false;
		}

		/* Mark all input items referencing the machine as processed. */
		InputItem *toMark = lastFlush;
		while ( true ) {
			toMark->processed = true;

			if ( toMark == ii )
				break;

			toMark = toMark->next;
		}

		/* Move forward, flushing input items until we get to an unprocessed
		 * input item. */
		while ( lastFlush != 0 && lastFlush->processed ) {
			verifyWriteHasData( lastFlush );

			if ( errorCount > 0 )
				return false;

			/* Flush out. */
			writeOutput( lastFlush );

			lastFlush = lastFlush->next;
		}
	}
	return true;
}

void InputData::makeFirstInputItem()
{
	/* Make the first input item. */
	InputItem *firstInputItem = new InputItem;
	firstInputItem->type = InputItem::HostData;
	firstInputItem->loc.fileName = inputFileName;
	firstInputItem->loc.line = 1;
	firstInputItem->loc.col = 1;
	inputItems.append( firstInputItem );
}

/* Send eof to all parsers. */
void InputData::terminateAllParsers( )
{
#ifdef WITH_RAGEL_KELBT
	for ( ParserDict::Iter pdel = parserDict; pdel.lte(); pdel++ )
		pdel->value->terminateParser();
#endif
}

void InputData::flushRemaining()
{
	InputItem *item = inputItems.head;

	while ( item != 0 ) {
		checkLastRef( item );
		item = item->next;
	}

	/* Flush remaining items. */
	while ( lastFlush != 0 ) {
		/* Flush out. */
		writeOutput( lastFlush );

		lastFlush = lastFlush->next;
	}
}

void InputData::makeTranslateOutputFileName()
{
	origOutputFileName = outputFileName;
	outputFileName = fileNameFromStem( outputFileName, ".ri" );
	genOutputFileName = outputFileName;
}

#ifdef WITH_RAGEL_KELBT
void InputData::parseKelbt()
{
	/*
	 * Ragel Parser from ragel 6.
	 */
	ifstream *inFileStream;
	istream *inStream;

	/* Open the input file for reading. */
	assert( inputFileName != 0 );
	inFileStream = new ifstream( inputFileName );
	if ( ! inFileStream->is_open() )
		error() << "could not open " << inputFileName << " for reading" << endp;
	inStream = inFileStream;

	makeFirstInputItem();

	Scanner scanner( this, inputFileName, *inStream, 0, 0, 0, false );

	scanner.sectionPass = true;
	scanner.do_scan();

	inStream->clear();
	inStream->seekg( 0, std::ios::beg );
	curItem = inputItems.head;
	lastFlush = inputItems.head;

	scanner.sectionPass = false;
	scanner.do_scan();

	/* Finished, final check for errors.. */
	if ( errorCount > 0 )
		abortCompile( 1 );

	/* Bail on above error. */
	if ( errorCount > 0 )
		abortCompile( 1 );
	
	delete inFileStream;
}

void InputData::processKelbt()
{
	/* With the kelbt version we implement two parse passes. The first is used
	 * to identify the last time that any given machine is referenced by a
	 * ragel section. In the second pass we parse, compile, and emit as far
	 * forward as possible when we encounter the last reference to a machine.
	 * */
	
	if ( generateDot ) {
		parseKelbt();
		terminateAllParsers();
		processDot();
	}
	else {
		createOutputStream();
		openOutput();
		parseKelbt();
		flushRemaining();
		closeOutput();
	}

	assert( errorCount == 0 );
}
#endif

bool InputData::parseReduce()
{
	/*
	 * Colm-based reduction parser introduced in ragel 7. 
	 */

	TopLevel *topLevel = new TopLevel( this, hostLang,
			minimizeLevel, minimizeOpt );

	/* Check input file. File is actually opened by colm code. We don't
	 * need to perform the check if in libragel since it comes in via a
	 * string. */
	if ( input == 0 ) {
		ifstream *inFile = new ifstream( inputFileName );
		if ( ! inFile->is_open() )
			error() << "could not open " << inputFileName << " for reading" << endp;
		delete inFile;
	}

	if ( errorCount )
		return false;

	makeFirstInputItem();
	
	curItem = inputItems.head;
	lastFlush = inputItems.head;


	topLevel->reduceFile( "rlparse", inputFileName );

	if ( errorCount )
		return false;

	bool success = topLevel->success;

	delete topLevel;
	return success;
}

bool InputData::processReduce()
{
	if ( generateDot ) {
		parseReduce();
		processDot();
		return true;
	}
	else {
		createOutputStream();
		openOutput();

		bool success = parseReduce();
		if ( success )
			flushRemaining();

		closeOutput();

		if ( !success && outputFileName != 0 )
			unlink( outputFileName );

		return success;
	}
}

bool InputData::process()
{
	switch ( frontend ) {
		case KelbtBased: {
#ifdef WITH_RAGEL_KELBT
			processKelbt();
#endif
			return true;
		}
		case ReduceBased: {
			return processReduce();
		}
	}
	return false;
}

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
"   -D                   D           All code styles supported\n"
"   -Z                   Go          All code styles supported\n"
"   -A                   C#          -T0 -T1 -F0 -F1 -G0 -G1\n"
"   -J                   Java        -T0 -T1 -F0 -F1\n"
"   -R                   Ruby        -T0 -T1 -F0 -F1\n"
"   -O                   OCaml       -T0 -T1 -F0 -F1\n"
"   -U                   Rust        -T0 -T1 -F0 -F1\n"
"   -Y                   Julia       -T0 -T1 -F0 -F1\n"
"   -K                   Crack       -T0 -T1 -F0 -F1\n"
"   -P                   JavaScript  -T0 -T1 -F0 -F1\n"
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
"                        compilation\n"
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
			"Copyright (c) 2001-2018 by Adrian Thurston" << endl;
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
	ParamCheck pc( "r:o:dnmleabjkS:M:I:CEJZRAOKUYvHh?-:sT:F:G:LpV", argc, argv );

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
//			case 'D':
//				hostLang = &hostLangD;
//				break;
//			case 'Z':
//				hostLang = &hostLangGo;
//				break;
//			case 'J':
//				hostLang = &hostLangJava;
//				break;
//			case 'R':
//				hostLang = &hostLangRuby;
//				break;
//			case 'A':
//				hostLang = &hostLangCSharp;
//				break;
//			case 'O':
//				hostLang = &hostLangOCaml;
//				break;
//			case 'K':
//				hostLang = &hostLangCrack;
//				break;
//			case 'U':
//				hostLang = &hostLangRust;
//				break;
//			case 'Y':
//				hostLang = &hostLangJulia;
//				break;
//			case 'P':
//				hostLang = &hostLangJS;
//				break;

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
					rlhc = true;
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

	if ( checkBreadth ) {
		if ( histogramFn != 0 )
			loadHistogram();
		else
			defaultHistogram();
	}
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

int InputData::main( int argc, const char **argv )
{
	int code = 0;
	try {
		parseArgs( argc, argv );
		checkArgs();
		makeDefaultFileName();

		if ( !process() )
			abortCompile( 1 );
	}
	catch ( const AbortCompile &ac ) {
		code = ac.code;
	}

	return code;
}
