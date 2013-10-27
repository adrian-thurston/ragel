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
#include "load.h"

#include <iostream>

using std::istream;
using std::ifstream;
using std::ostream;
using std::cout;
using std::cerr;
using std::endl;
using std::ios;

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
				case HostLang::D2: defExtension = ".d"; break;
				default: break;
			}
			outputFileName = fileNameFromStem( inputFile, defExtension );
		}
	}
}

/* Invoked by the parser when the root element is opened. */
void InputData::goDefaultFileName( const char *inputFile )
{
	/* If the output format is code and no output file name is given, then
	 * make a default. */
	if ( outputFileName == 0 )
		outputFileName = fileNameFromStem( inputFile, ".go" );
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

/* Invoked by the parser when the root element is opened. */
void InputData::ocamlDefaultFileName( const char *inputFile )
{
	/* If the output format is code and no output file name is given, then
	 * make a default. */
	if ( outputFileName == 0 )
		outputFileName = fileNameFromStem( inputFile, ".ml" );
}

/* Invoked by the parser when the root element is opened. */
void InputData::crackDefaultFileName( const char *inputFile )
{
	/* If the output format is code and no output file name is given, then
	 * make a default. */
	if ( outputFileName == 0 )
		outputFileName = fileNameFromStem( inputFile, ".crk" );
}


void InputData::makeDefaultFileName()
{
	switch ( hostLang->lang ) {
		case HostLang::C:
		case HostLang::D:
		case HostLang::D2:
			cdDefaultFileName( inputFileName );
			break;
		case HostLang::Java:
			javaDefaultFileName( inputFileName );
			break;
		case HostLang::Go:
			goDefaultFileName( inputFileName );
			break;
		case HostLang::Ruby:
			rubyDefaultFileName( inputFileName );
			break;
		case HostLang::CSharp:
			csharpDefaultFileName( inputFileName );
			break;
		case HostLang::OCaml:
			ocamlDefaultFileName( inputFileName );
			break;
		case HostLang::Crack:
			crackDefaultFileName( inputFileName );
			break;

	}
}

void InputData::makeOutputStream()
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
		/* Writing out ot std out. */
		outStream = &cout;
	}
}

void InputData::openOutput()
{
	if ( outFilter != 0 ) {
		outFilter->open( outputFileName, ios::out|ios::trunc );
		if ( !outFilter->is_open() ) {
			error() << "error opening " << outputFileName << " for writing" << endl;
			exit(1);
		}
	}
}

void InputData::prepareSingleMachine()
{
#ifdef KELBT_PARSER
	/* Locate a machine spec to generate dot output for. We can only emit.
	 * Dot takes one graph at a time. */
	if ( machineSpec != 0 ) {
		/* Machine specified. */
		ParserDictEl *pdEl = parserDict.find( machineSpec );
		if ( pdEl == 0 )
			error() << "could not locate machine specified with -S and/or -M" << endp;
		dotGenParser = pdEl->value;
	}
	else { 
		/* No machine spec given, generate the first one. */
		if ( parserList.length() == 0 )
			error() << "no machine specification to generate graphviz output" << endp;

		dotGenParser = parserList.head;
	}

	GraphDictEl *gdEl = 0;

	if ( machineName != 0 ) {
		gdEl = dotGenParser->pd->graphDict.find( machineName );
		if ( gdEl == 0 )
			error() << "machine definition/instantiation not found" << endp;
	}
	else {
		/* We are using the whole machine spec. Need to make sure there
		 * are instances in the spec. */
		if ( dotGenParser->pd->instanceList.length() == 0 )
			error() << "no machine instantiations to generate graphviz output" << endp;
	}

	dotGenParser->pd->prepareMachineGen( gdEl );
#endif
}

void InputData::prepareAllMachines()
{
	for ( ParseDataDict::Iter pdel = parseDataDict; pdel.lte(); pdel++ ) {
		ParseData *pd = pdel->value;
		if ( pd->instanceList.length() > 0 )
			pd->prepareMachineGen( 0, hostLang );
	}
}


void InputData::generateReduced( CodeStyle codeStyle, bool printStatistics )
{
	for ( ParseDataDict::Iter pdel = parseDataDict; pdel.lte(); pdel++ ) {
		ParseData *pd = pdel->value;
		if ( pd->instanceList.length() > 0 )
			pd->generateReduced( inputFileName, codeStyle, *outStream, printStatistics, hostLang );
	}
}

void InputData::verifyWritesHaveData()
{
	for ( InputItemList::Iter ii = inputItems; ii.lte(); ii++ ) {
		if ( ii->type == InputItem::Write ) {
			if ( ii->pd->cgd == 0 )
				error( ii->loc ) << "no machine instantiations to write" << endl;
		}
	}
}

void InputData::writeOutput( bool generateDot )
{
	for ( InputItemList::Iter ii = inputItems; ii.lte(); ii++ ) {
		if ( ii->type == InputItem::Write ) {
			CodeGenData *cgd = ii->pd->cgd;
			cgd->writeStatement( ii->loc, ii->writeArgs.length(), ii->writeArgs.data, generateDot, hostLang );
		}
		else {
			*outStream << '\n';
			lineDirective( this, *outStream, inputFileName, ii->loc.line, generateDot, hostLang );
			*outStream << ii->data.str();
		}
	}
}

void InputData::processXML()
{
	/* Compiles machines. */
	prepareAllMachines();

	if ( gblErrorCount > 0 )
		exit(1);

	makeOutputStream();

	if ( gblErrorCount > 0 )
		exit(1);

	/*
	 * From this point on we should not be reporting any errors.
	 */

	openOutput();
	writeXML( *outStream );
}

void InputData::processDot()
{
	/* Compiles the DOT machines. */
	prepareSingleMachine();

	if ( gblErrorCount > 0 )
		exit(1);

	makeOutputStream();

	if ( gblErrorCount > 0 )
		exit(1);

	/*
	 * From this point on we should not be reporting any errors.
	 */

	openOutput();
	writeDot( *outStream );
}

void InputData::processCode( CodeStyle codeStyle, bool generateDot, bool printStatistics )
{
	/* Compiles machines. */
	prepareAllMachines();

	if ( gblErrorCount > 0 )
		exit(1);

	makeDefaultFileName();
	makeOutputStream();

	/* Generates the reduced machine, which we use to write output. */
	generateReduced( codeStyle, printStatistics );

	if ( gblErrorCount > 0 )
		exit(1);

	verifyWritesHaveData();

	if ( gblErrorCount > 0 )
		exit(1);

	/*
	 * From this point on we should not be reporting any errors.
	 */

	openOutput();
	writeOutput( generateDot );
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

void InputData::process( CodeStyle codeStyle, bool generateXML, bool generateDot,
		bool printStatistics, MinimizeLevel minimizeLevel, MinimizeOpt minimizeOpt )
{
	/* Check input file. */
	ifstream *inFile = new ifstream( inputFileName );
	if ( ! inFile->is_open() )
		error() << "could not open " << inputFileName << " for reading" << endp;
	delete inFile;

	makeFirstInputItem();

	LoadRagel *lr = newLoadRagel( *this, hostLang, minimizeLevel, minimizeOpt );
	loadRagel( lr, inputFileName );
	deleteLoadRagel( lr );

	/* Bail on above error. */
	if ( gblErrorCount > 0 )
		exit(1);

	if ( generateXML )
		processXML();
	else if ( generateDot )
		processDot();
	else 
		processCode( codeStyle, generateDot, printStatistics );

	/* If writing to a file, delete the ostream, causing it to flush.
	 * Standard out is flushed automatically. */
	if ( outputFileName != 0 ) {
		delete outStream;
		delete outFilter;
	}

	assert( gblErrorCount == 0 );
}

