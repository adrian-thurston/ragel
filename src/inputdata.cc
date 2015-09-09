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
#include "rlscan.h"

#include <iostream>
#include <unistd.h>

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
	}
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

	dotGenParser->pd->prepareMachineGen( gdEl, hostLang );
}

void InputData::prepareAllMachines()
{
	for ( ParseDataDict::Iter pdel = parseDataDict; pdel.lte(); pdel++ ) {
		ParseData *pd = pdel->value;
		if ( pd->instanceList.length() > 0 )
			pd->prepareMachineGen( 0, hostLang );
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
			error( ii->loc ) << "no machine instantiations to write" << endl;
	}
}

void InputData::verifyWritesHaveData()
{
	for ( InputItemList::Iter ii = inputItems; ii.lte(); ii++ )
		verifyWriteHasData( ii );
}

void translatedHostData( ostream &out, const string &data )
{
	const char *p = data.c_str();
	for ( const char *c = p; *c != 0; ) {
		if ( c[0] == '}' && ( c[1] == '@' || c[1] == '$' || c[1] == '=' ) ) {
			out << "@}@" << c[1];
			c += 2;
		}
		else if ( c[0] == '@' ) {
			out << "@@";
			c += 1;
		}
		else {
			out << c[0];
			c += 1;
		}
	}
}

void InputData::writeOutput( InputItem *ii )
{
	switch ( ii->type ) {
		case InputItem::Write: {
			CodeGenData *cgd = ii->pd->cgd;
			cgd->writeStatement( ii->loc, ii->writeArgs.length(),
					ii->writeArgs.data, generateDot, hostLang );
			break;
		}
		case InputItem::HostData: {
			switch ( backend ) {
				case Direct:
					if ( hostLang->lang == HostLang::C ) {
						*outStream << "\n#line " << ii->loc.line <<
								" \"" << ii->loc.fileName << "\"\n";
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

void InputData::processXML()
{
	/* Compiles machines. */
	prepareAllMachines();

	if ( gblErrorCount > 0 )
		exit(1);

	createOutputStream();

	if ( gblErrorCount > 0 )
		exit(1);

	/*
	 * From this point on we should not be reporting any errors.
	 */

	openOutput();
	writeXML( *outStream );
	closeOutput();
}

void InputData::processDot()
{
	/* Compiles the DOT machines. */
	prepareSingleMachine();

	if ( gblErrorCount > 0 )
		exit(1);

	createOutputStream();

	if ( gblErrorCount > 0 )
		exit(1);

	/*
	 * From this point on we should not be reporting any errors.
	 */

	openOutput();
	writeDot( *outStream );
	closeOutput();
}

void InputData::runRlhc()
{
	if ( backend == Translated ) {
#ifdef WITH_COLM
		if ( !noIntermediate ) {
			string rlhc = dirName + "/rlhc " + 
					origOutputFileName + " " +
					genOutputFileName + " " +
					hostLang->rlhcArg;

			if ( rlhcShowCmd )
				std::cout << rlhc << std::endl;

			int res = system( rlhc.c_str() );
			if ( res != 0 )
				exit( 1 );

			if ( !saveTemps )
				unlink( genOutputFileName.c_str() );
		}
#else
		error() << "colm-based codegen not available" << endp;
#endif
	}
}

void InputData::processCode()
{
	/* Compiles machines. */
	prepareAllMachines();

	if ( gblErrorCount > 0 )
		exit(1);

	makeDefaultFileName();

	makeTranslateOutputFileName();

	createOutputStream();

	/* Generates the reduced machine, which we use to write output. */
	generateReduced();

	if ( gblErrorCount > 0 )
		exit(1);

	verifyWritesHaveData();

	if ( gblErrorCount > 0 )
		exit(1);

	/*
	 * From this point on we should not be reporting any errors.
	 */

	openOutput();
	writeOutput();
	closeOutput();
	runRlhc();
}

void InputData::checkLastRef( InputItem *ii )
{
	if ( generateXML || generateDot )
		return;
		
	/*
	 * 1. Go forward to next last reference.
	 * 2. Fully process that machine, mark as processed.
	 * 3. Move forward through input items until no longer 
	 */
	if ( ii->section != 0 && ii->section->lastReference == ii ) {
		/* Fully Process. */
		ParseData *pd = ii->pd;

		if ( pd->instanceList.length() > 0 ) {
			terminateParser( ii->parser );

			pd->prepareMachineGen( 0, hostLang );

			if ( gblErrorCount > 0 )
				exit(1);

			pd->generateReduced( inputFileName, codeStyle, *outStream, hostLang );

			if ( gblErrorCount > 0 )
				exit(1);
		}

		/* Mark all input items referencing the machine as processed. */
		InputItem *toMark = lastFlush;
		while ( true ) {
			toMark->processed = true;

			if ( toMark == ii )
				break;
			toMark = toMark->next;
		}

		/* Move forward, flusing input items until we get to an unprocessed
		 * input item. */
		while ( lastFlush != 0 && lastFlush->processed ) {
			verifyWriteHasData( lastFlush );

			if ( gblErrorCount > 0 )
				exit(1);

			/* Flush out. */
			writeOutput( lastFlush );

			/* If this is the last reference to a pd, we can now clear the
			 * memory for it. */
			if ( lastFlush->pd != 0 && lastFlush->section->lastReference == lastFlush ) {
				if ( lastFlush->pd->instanceList.length() > 0 ) {
					lastFlush->pd->clear();
					if ( lastFlush->parser != 0 )
						lastFlush->parser->clear();
				}
			}

			lastFlush = lastFlush->next;
		}
	}
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
void InputData::terminateParser( Parser6 *parser )
{
	/* FIXME: a proper token is needed here. Suppose we should use the
	 * location of EOF in the last file that the parser was referenced in. */
	InputLoc loc;
	loc.fileName = "<EOF>";
	loc.line = 0;
	loc.col = 0;

	parser->token( loc, Parser6_tk_eof, 0, 0 );
}

/* Send eof to all parsers. */
void InputData::terminateAllParsers( )
{
	/* FIXME: a proper token is needed here. Suppose we should use the
	 * location of EOF in the last file that the parser was referenced in. */
	InputLoc loc;
	loc.fileName = "<EOF>";
	loc.line = 0;
	loc.col = 0;
	for ( ParserDict::Iter pdel = parserDict; pdel.lte(); pdel++ )
		pdel->value->token( loc, Parser6_tk_eof, 0, 0 );
}

void InputData::parseKelbt()
{
	/*
	 * Ragel Parser from ragel 6.
	 */

	/* Open the input file for reading. */
	assert( inputFileName != 0 );
	ifstream *inFile = new ifstream( inputFileName );
	if ( ! inFile->is_open() )
		error() << "could not open " << inputFileName << " for reading" << endp;

	makeFirstInputItem();

	Scanner scanner( *this, inputFileName, *inFile, 0, 0, 0, false );

	scanner.sectionPass = true;
	scanner.do_scan();

	inFile->clear();
	inFile->seekg( 0, std::ios::beg );
	curItem = inputItems.head;
	lastFlush = inputItems.head;

	scanner.sectionPass = false;
	scanner.do_scan();

	/* Finished, final check for errors.. */
	if ( gblErrorCount > 0 )
		exit(1);


	/* Bail on above error. */
	if ( gblErrorCount > 0 )
		exit(1);
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
		genOutputFileName = fileNameFromStem( inputFileName, ".ri" );
		outputFileName = genOutputFileName.c_str();
	}
}

void InputData::processKelbt()
{
	/* With the kelbt version we implement two parse passes. The first is used
	 * to identify the last time that any given machine is referenced by a
	 * ragel section. In the second pass we parse, compile, and emit as far
	 * forward as possible when we encounter the last reference to a machine.
	 * */
	
	if ( generateXML ) {
		/* When generating XML or dot the inline features are disabled */
		parseKelbt();
		terminateAllParsers();
		processXML();
	}
	else if ( generateDot ) {
		parseKelbt();
		terminateAllParsers();
		processDot();
	}
	else {
		makeDefaultFileName();
		makeTranslateOutputFileName();
		createOutputStream();
		openOutput();
		parseKelbt();
		flushRemaining();
		closeOutput();
		runRlhc();
	}


	assert( gblErrorCount == 0 );
}

void InputData::processColm()
{
#ifdef WITH_COLM

	/* With this version we parse, load, process. It is very memory intensive,
	 * but allows use of colm grammars for parsing. */

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
	if ( gblErrorCount > 0 )
		exit(1);

	if ( generateXML )
		processXML();
	else if ( generateDot )
		processDot();
	else 
		processCode();

	assert( gblErrorCount == 0 );
#endif
}

void InputData::process()
{
	switch ( frontend ) {
		case KelbtBased: {
			processKelbt();
			break;
		}
		case ColmBased: {
			processColm();
			break;
		}
	}
}
