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

#include "rlgen-java.h"
#include "xmlparse.h"
#include "pcheck.h"
#include "vector.h"
#include "version.h"
#include "common.h"
#include "javacodegen.h"

using std::istream;
using std::ifstream;
using std::ostream;
using std::ios;
using std::cin;
using std::cout;
using std::cerr;
using std::endl;

/* Io globals. */
extern istream *inStream;
extern ostream *outStream;
extern output_filter *outFilter;
extern const char *outputFileName;

extern int numSplitPartitions;

/*
 * Callbacks invoked by the XML data parser.
 */

/* Invoked by the parser when the root element is opened. */
ostream *javaOpenOutput( char *inputFile )
{
	if ( hostLang->lang != HostLang::Java ) {
		error() << "this code generator is for Java only" << endl;
		exit(1);
	}

	/* If the output format is code and no output file name is given, then
	 * make a default. */
	if ( outputFileName == 0 ) {
		char *ext = findFileExtension( inputFile );
		if ( ext != 0 && strcmp( ext, ".rh" ) == 0 )
			outputFileName = fileNameFromStem( inputFile, ".h" );
		else
			outputFileName = fileNameFromStem( inputFile, ".java" );
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
CodeGenData *javaMakeCodeGen( char *sourceFileName, char *fsmName, 
		ostream &out, bool wantComplete )
{
	CodeGenData *codeGen = new JavaTabCodeGen(out);

	codeGen->sourceFileName = sourceFileName;
	codeGen->fsmName = fsmName;
	codeGen->wantComplete = wantComplete;

	return codeGen;
}
