/*
 *  Copyright 2008-2016 Adrian Thurston <thurston@complang.org>
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
#include "reducer.h"

#include <iostream>
#include <unistd.h>

using std::istream;
using std::cout;
using std::ifstream;
using std::stringstream;
using std::ostream;
using std::endl;
using std::ios;

InputData::~InputData()
{
	includeDict.empty();
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

void InputData::asmDefaultFileName( const char *inputFile )
{
    if ( outputFileName == 0 )
        outputFileName = fileNameFromStem( inputFile, ".s" );
}

void InputData::rustDefaultFileName( const char *inputFile )
{
	if ( outputFileName == 0 )
		outputFileName = fileNameFromStem( inputFile, ".rs" );
}

void InputData::juliaDefaultFileName( const char *inputFile )
{
	if ( outputFileName == 0 )
		outputFileName = fileNameFromStem( inputFile, ".jl" );
}

void InputData::jsDefaultFileName( const char *inputFile )
{
	/* If the output format is code and no output file name is given, then
	 * make a default. */
	if ( outputFileName == 0 )
		outputFileName = fileNameFromStem( inputFile, ".js" );
}

void InputData::makeDefaultFileName()
{
	switch ( hostLang->lang ) {
		case HostLang::C:
		case HostLang::D:
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
		case HostLang::Asm:
			asmDefaultFileName( inputFileName );
			break;
		case HostLang::Rust:
			rustDefaultFileName( inputFileName );
			break;
		case HostLang::Julia:
			juliaDefaultFileName( inputFileName );
			break;
		case HostLang::JS:
			jsDefaultFileName( inputFileName );
			break;
	}
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

void InputData::verifyWriteHasData( InputItem *ii )
{
}

void InputData::verifyWritesHaveData()
{
	for ( InputItemList::Iter ii = inputItems; ii.lte(); ii++ )
		verifyWriteHasData( ii );
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

void InputData::writeOutput( InputItem *ii )
{
	switch ( ii->type ) {
		case InputItem::Write: {
			cout << "write:";
			for ( size_t i = 0; i < ii->writeArgs.size(); i++ )
				cout << " " << ii->writeArgs[i];
			cout << endl;
			break;
		}
		case InputItem::HostData: {
			cout << ii->data.str() << endl;
			break;
		}
		case InputItem::EndSection: {
			break;
		}
	}
}

/* Send eof to all parsers. */
void InputData::terminateAllParsers( )
{
}

void InputData::flushRemaining()
{
	/* Flush remaining items. */
	while ( lastFlush != 0 ) {
		/* Flush out. */
		writeOutput( lastFlush );
		lastFlush = lastFlush->next;
	}
}

void InputData::makeTranslateOutputFileName()
{
	if ( backend == Translated ) {
		origOutputFileName = outputFileName;
		outputFileName = fileNameFromStem( inputFileName, ".ri" );
		genOutputFileName = outputFileName;
	}
}

void InputData::writeOutput()
{
	for ( InputItemList::Iter ii = inputItems; ii.lte(); ii++ )
		writeOutput( ii );
}

void InputData::processColm()
{
	/*
	 * Colm-based parser introduced in ragel 7. Uses more memory.
	 */

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
	if ( errorCount > 0 )
		abortCompile( 1 );

	assert( errorCount == 0 );

	writeOutput();

	for ( ParseDataList::Iter pd = parseDataList; pd.lte(); pd++ )
		pd->dump();
}

bool InputData::parseReduce()
{
	/*
	 * Colm-based reduction parser introduced in ragel 7. 
	 */

	SectionPass *sectionPass = new SectionPass( this );
	TopLevel *topLevel = new TopLevel( this, sectionPass, hostLang,
			minimizeLevel, minimizeOpt );

	if ( ! inLibRagel ) {
		/* Check input file. File is actually opened by colm code. We don't
		 * need to perform the check if in libragel since it comes in via a
		 * string. */
		if ( input == 0 ) {
			ifstream *inFile = new ifstream( inputFileName );
			if ( ! inFile->is_open() )
				error() << "could not open " << inputFileName << " for reading" << endp;
			delete inFile;
		}
	}

	makeFirstInputItem();

	if ( inLibRagel )
		sectionPass->reduceStr( inputFileName, input );
	else
		sectionPass->reduceFile( inputFileName );
	
	if ( errorCount )
		return false;

	curItem = inputItems.head;
	lastFlush = inputItems.head;

	if ( inLibRagel )
		topLevel->reduceStr( inputFileName, input );
	else
		topLevel->reduceFile( inputFileName );

	if ( errorCount )
		return false;

	bool success = topLevel->success;

	delete topLevel;
	delete sectionPass;

	return success;
}

bool InputData::processReduce()
{
	makeDefaultFileName();
	makeTranslateOutputFileName();
	createOutputStream();

	bool success = parseReduce();
	if ( success )
		flushRemaining();

	for ( ParseDataList::Iter pd = parseDataList; pd.lte(); pd++ )
		pd->dump();

	return success;
}

bool InputData::process()
{
	switch ( frontend ) {
		case ColmBased: {
			processColm();
			return true;
		}
		case ReduceBased: {
			return processReduce();
		}
	}
	

	return false;
}
