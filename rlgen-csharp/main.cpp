/*
 *  Copyright 2001-2007 Adrian Thurston <thurston@complang.org>
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

#include "common.h"
#include "rlgen-csharp.h"
#include "xmlparse.h"
#include "pcheck.h"
#include "vector.h"
#include "version.h"

/* Code generators. */
#include "tabcodegen.h"
#include "ftabcodegen.h"
#include "flatcodegen.h"
#include "fflatcodegen.h"
#include "gotocodegen.h"
#include "fgotocodegen.h"
#include "ipgotocodegen.h"
#include "splitcodegen.h"

using std::istream;
using std::ifstream;
using std::ostream;
using std::ios;
using std::cin;
using std::cout;
using std::cerr;
using std::endl;

/* Target language and output style. */
extern CodeStyleEnum codeStyle;

/* Io globals. */
extern istream *inStream;
extern ostream *outStream;
extern output_filter *outFilter;
extern const char *outputFileName;

/* Graphviz dot file generation. */
extern bool graphvizDone;

extern int numSplitPartitions;
extern bool noLineDirectives;

/* Print a summary of the options. */
void csharp_usage()
{
	cout <<
"usage: " PROGNAME " [options] file\n"
"general:\n"
"   -h, -H, -?, --help    Print this usage and exit\n"
"   -v, --version         Print version information and exit\n"
"   -o <file>             Write output to <file>\n"
"code generation options:\n"
"   -L                    Inhibit writing of #line directives\n"
"generated code style:\n"
"   -T0                   Table driven FSM (default)\n"
"   -T1                   Faster table driven FSM\n"
"   -F0                   Flat table driven FSM\n"
"   -F1                   Faster flat table-driven FSM\n"
"   -G0                   Goto-driven FSM\n"
"   -G1                   Faster goto-driven FSM\n"
	;	
}

/* Print version information. */
void csharp_version()
{
	cout << "Ragel Code Generator for C#" << endl <<
			"Version " VERSION << ", " PUBDATE << endl <<
			"Copyright (c) 2001-2007 by Adrian Thurston" << endl;
}

ostream &csharp_error()
{
	gblErrorCount += 1;
	cerr << PROGNAME ": ";
	return cerr;
}

/*
 * Callbacks invoked by the XML data parser.
 */

/* Invoked by the parser when the root element is opened. */
ostream *csharpOpenOutput( const char *inputFile )
{
	if ( hostLang->lang != HostLang::CSharp ) {
		csharp_error() << "this code generator is for C# only" << endl;
		exit(1);
	}

	/* If the output format is code and no output file name is given, then
	 * make a default. */
	if ( outputFileName == 0 ) {
		const char *ext = findFileExtension( inputFile );
		if ( ext != 0 && strcmp( ext, ".rh" ) == 0 )
			outputFileName = fileNameFromStem( inputFile, ".h" );
		else
			outputFileName = fileNameFromStem( inputFile, ".cs" );
	}

	/* Make sure we are not writing to the same file as the input file. */
	if ( outputFileName != 0 && strcmp( inputFile, outputFileName  ) == 0 ) {
		csharp_error() << "output file \"" << outputFileName  << 
				"\" is the same as the input file" << endl;
	}

	if ( outputFileName != 0 ) {
		/* Create the filter on the output and open it. */
		outFilter = new output_filter( outputFileName );
		outFilter->open( outputFileName, ios::out|ios::trunc );
		if ( !outFilter->is_open() ) {
			csharp_error() << "error opening " << outputFileName << " for writing" << endl;
			exit(1);
		}

		/* Open the output stream, attaching it to the filter. */
		outStream = new ostream( outFilter );
	}
	else {
		/* Writing out ot std out. */
		outStream = &cout;
	}
	return outStream;
}

/* Invoked by the parser when a ragel definition is opened. */
CodeGenData *csharpMakeCodeGen( const char *sourceFileName, const char *fsmName, 
		ostream &out, bool wantComplete )
{
	CodeGenData *codeGen = 0;

	switch ( codeStyle ) {
	case GenTables:
		codeGen = new CSharpTabCodeGen(out);
		break;
	case GenFTables:
		codeGen = new CSharpFTabCodeGen(out);
		break;
	case GenFlat:
		codeGen = new CSharpFlatCodeGen(out);
		break;
	case GenFFlat:
		codeGen = new CSharpFFlatCodeGen(out);
		break;
	case GenGoto:
		codeGen = new CSharpGotoCodeGen(out);
		break;
	case GenFGoto:
		codeGen = new CSharpFGotoCodeGen(out);
		break;
	case GenIpGoto:
		codeGen = new CSharpIpGotoCodeGen(out);
		break;
	case GenSplit:
		codeGen = new CSharpSplitCodeGen(out);
		break;
	}

	codeGen->sourceFileName = sourceFileName;
	codeGen->fsmName = fsmName;
	codeGen->wantComplete = wantComplete;

	return codeGen;
}
