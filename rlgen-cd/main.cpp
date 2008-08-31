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

#include "common.h"
#include "rlgen-cd.h"
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


/*
 * Callbacks invoked by the XML data parser.
 */

/* Invoked by the parser when the root element is opened. */
ostream *cdOpenOutput( char *inputFile )
{
	if ( hostLang->lang != HostLang::C && hostLang->lang != HostLang::D ) {
		error() << "this code generator is for C and D only" << endl;
		exit(1);
	}

	/* If the output format is code and no output file name is given, then
	 * make a default. */
	if ( outputFileName == 0 ) {
		char *ext = findFileExtension( inputFile );
		if ( ext != 0 && strcmp( ext, ".rh" ) == 0 )
			outputFileName = fileNameFromStem( inputFile, ".h" );
		else {
			const char *defExtension = 0;
			switch ( hostLang->lang ) {
				case HostLang::C: defExtension = ".c"; break;
				case HostLang::D: defExtension = ".d"; break;
				default: break;
			}
			outputFileName = fileNameFromStem( inputFile, defExtension );
		}
	}

	/* Make sure we are not writing to the same file as the input file. */
	if ( outputFileName != 0 && strcmp( inputFile, outputFileName  ) == 0 ) {
		error() << "output file \"" << outputFileName  << 
				"\" is the same as the input file" << endl;
	}

	if ( outputFileName != 0 ) {
		/* Create the filter on the output and open it. */
		outFilter = new output_filter( outputFileName );
		outFilter->open( outputFileName, ios::out|ios::trunc );
		if ( !outFilter->is_open() ) {
			error() << "error opening " << outputFileName << " for writing" << endl;
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
CodeGenData *cdMakeCodeGen( char *sourceFileName, char *fsmName, 
		ostream &out, bool wantComplete )
{
	CodeGenData *codeGen = 0;
	switch ( hostLang->lang ) {
	case HostLang::C:
		switch ( codeStyle ) {
		case GenTables:
			codeGen = new CTabCodeGen(out);
			break;
		case GenFTables:
			codeGen = new CFTabCodeGen(out);
			break;
		case GenFlat:
			codeGen = new CFlatCodeGen(out);
			break;
		case GenFFlat:
			codeGen = new CFFlatCodeGen(out);
			break;
		case GenGoto:
			codeGen = new CGotoCodeGen(out);
			break;
		case GenFGoto:
			codeGen = new CFGotoCodeGen(out);
			break;
		case GenIpGoto:
			codeGen = new CIpGotoCodeGen(out);
			break;
		case GenSplit:
			codeGen = new CSplitCodeGen(out);
			break;
		}
		break;

	case HostLang::D:
		switch ( codeStyle ) {
		case GenTables:
			codeGen = new DTabCodeGen(out);
			break;
		case GenFTables:
			codeGen = new DFTabCodeGen(out);
			break;
		case GenFlat:
			codeGen = new DFlatCodeGen(out);
			break;
		case GenFFlat:
			codeGen = new DFFlatCodeGen(out);
			break;
		case GenGoto:
			codeGen = new DGotoCodeGen(out);
			break;
		case GenFGoto:
			codeGen = new DFGotoCodeGen(out);
			break;
		case GenIpGoto:
			codeGen = new DIpGotoCodeGen(out);
			break;
		case GenSplit:
			codeGen = new DSplitCodeGen(out);
			break;
		}
		break;

	default: break;
	}

	codeGen->sourceFileName = sourceFileName;
	codeGen->fsmName = fsmName;
	codeGen->wantComplete = wantComplete;

	return codeGen;
}

