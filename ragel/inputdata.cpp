/*
 *  Copyright 2008 Adrian Thurston <thurston@complang.org>
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

#include "ragel.h"
#include "common.h"
#include "inputdata.h"
#include "parsedata.h"
#include "rlparse.h"
#include <iostream>

using std::cout;
using std::cerr;
using std::endl;
using std::ios;

void InputData::generateSpecificReduced()
{
	if ( parserDict.length() > 0 ) {
		/* There is either a machine spec or machine name given. */
		ParseData *parseData = 0;
		GraphDictEl *graphDictEl = 0;

		/* Traverse the sections, break out when we find a section/machine
		 * that matches the one specified. */
		for ( ParserDict::Iter parser = parserDict; parser.lte(); parser++ ) {
			ParseData *checkPd = parser->value->pd;
			if ( machineSpec == 0 || strcmp( checkPd->sectionName, machineSpec ) == 0 ) {
				GraphDictEl *checkGdEl = 0;
				if ( machineName == 0 || (checkGdEl = 
						checkPd->graphDict.find( machineName )) != 0 )
				{
					/* Have a machine spec and/or machine name that matches
					 * the -M/-S options. */
					parseData = checkPd;
					graphDictEl = checkGdEl;
					break;
				}
			}
		}

		if ( parseData == 0 )
			error() << "could not locate machine specified with -S and/or -M" << endl;
		else {
			/* Section/Machine to emit was found. Prepare and emit it. */
			parseData->prepareMachineGen( graphDictEl );
			if ( gblErrorCount == 0 )
				parseData->generateReduced( *this );
		}
	}

	writeOutput();
}


/* Invoked by the parser when the root element is opened. */
void InputData::cdDefaultFileName( const char *inputFile )
{
	/* If the output format is code and no output file name is given, then
	 * make a default. */
	if ( outputFileName == 0 ) {
		const char *ext = findFileExtension( inputFile );
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
}

/* Invoked by the parser when the root element is opened. */
void InputData::javaDefaultFileName( const char *inputFile )
{
	/* If the output format is code and no output file name is given, then
	 * make a default. */
	if ( outputFileName == 0 )
		outputFileName = fileNameFromStem( inputFile, ".java" );
}

/* Invoked by the parser when the root element is opened. */
void InputData::rubyDefaultFileName( const char *inputFile )
{
	/* If the output format is code and no output file name is given, then
	 * make a default. */
	if ( outputFileName == 0 )
		outputFileName = fileNameFromStem( inputFile, ".rb" );
}

/* Invoked by the parser when the root element is opened. */
void InputData::csharpDefaultFileName( const char *inputFile )
{
	/* If the output format is code and no output file name is given, then
	 * make a default. */
	if ( outputFileName == 0 ) {
		const char *ext = findFileExtension( inputFile );
		if ( ext != 0 && strcmp( ext, ".rh" ) == 0 )
			outputFileName = fileNameFromStem( inputFile, ".h" );
		else
			outputFileName = fileNameFromStem( inputFile, ".cs" );
	}
}

void InputData::openOutput()
{
	if ( ! generateDot ) {
		switch ( hostLang->lang ) {
			case HostLang::C:
			case HostLang::D:
				cdDefaultFileName( inputFileName );
				break;
			case HostLang::Java:
				javaDefaultFileName( inputFileName );
				break;
			case HostLang::Ruby:
				rubyDefaultFileName( inputFileName );
				break;
			case HostLang::CSharp:
				csharpDefaultFileName( inputFileName );
				break;
		}
	}

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
		/* Writing out ot std out. */
		outStream = &cout;
	}
}

void InputData::openOutput2()
{
	if ( outFilter != 0 ) {
		outFilter->open( outputFileName, ios::out|ios::trunc );
		if ( !outFilter->is_open() ) {
			error() << "error opening " << outputFileName << " for writing" << endl;
			exit(1);
		}
	}
}

void InputData::prepareMachineGen()
{
	/* No machine spec or machine name given. Generate everything. */
	for ( ParserDict::Iter parser = parserDict; parser.lte(); parser++ ) {
		ParseData *pd = parser->value->pd;
		if ( pd->instanceList.length() > 0 )
			pd->prepareMachineGen( 0 );
	}
}

void InputData::generateReduced()
{
	for ( ParserDict::Iter parser = parserDict; parser.lte(); parser++ ) {
		ParseData *pd = parser->value->pd;
		if ( pd->instanceList.length() > 0 )
			pd->generateReduced( *this );
	}
}

void InputData::writeOutput()
{
	for ( InputItemList::Iter ii = inputItems; ii.lte(); ii++ ) {
		if ( ii->type == InputItem::Write ) {
			CodeGenData *cgd = ii->pd->cgd;
			::keyOps = &cgd->thisKeyOps;

			cgd->writeStatement( ii->loc, ii->writeArgs.length()-1, ii->writeArgs.data );
		}
		else /*if ( /!generateDot )*/ {
			*outStream << '\n';
			lineDirective( *outStream, inputFileName, ii->loc.line );
			*outStream << ii->data.str();
		}
	}
}

