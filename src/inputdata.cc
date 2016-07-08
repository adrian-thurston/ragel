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
#include "rlscan.h"
#include "reducer.h"

#include <iostream>
#include <unistd.h>

using std::istream;
using std::ifstream;
using std::stringstream;
using std::ostream;
using std::endl;
using std::ios;

extern colm_sections rlhc_object;

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
			error( ii->loc ) << "no machine instantiations to write" << endl;
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
			cgd->writeStatement( ii->loc, ii->writeArgs.length(),
					ii->writeArgs.data, generateDot, hostLang );
			break;
		}
		case InputItem::HostData: {
			switch ( backend ) {
				case Direct:
					if ( hostLang->lang == HostLang::C ) {
						if ( ii->loc.fileName != 0 ) {
							// if ( lineDirectives )
							*outStream << "\n#line " << ii->loc.line <<
									" \"" << ii->loc.fileName << "\"\n";
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

void InputData::runRlhc()
{
	if ( backend != Translated || noIntermediate )
		return;

	string rlhc = dirName + "/rlhc " + 
			origOutputFileName + " " +
			genOutputFileName + " " +
			hostLang->rlhcArg;

	if ( rlhcShowCmd )
		info() << rlhc << std::endl;

	const char *argv[5];
	argv[0] = "rlhc";
	argv[1] = origOutputFileName.c_str();
	argv[2] = genOutputFileName.c_str(); 
	argv[3] = hostLang->rlhcArg;
	argv[4] = 0;

	colm_program *program = colm_new_program( &rlhc_object );
	colm_set_debug( program, 0 );
	colm_run_program( program, 4, argv );
	int es = program->exit_status;

	streamFileNames.append( colm_extract_fns( program ) );

	colm_delete_program( program );

	if ( !saveTemps )
		unlink( genOutputFileName.c_str() );

	/* Translation step shouldn't fail, but it can if there is an
	 * internal error. Pass it up.  */
	if ( es != 0 )
		abortCompile( es );
}

void InputData::processCode()
{
	/* Compiles machines. */
	prepareAllMachines();

	if ( errorCount > 0 )
		abortCompile( 1 );

	makeDefaultFileName();

	makeTranslateOutputFileName();

	createOutputStream();

	/* Generates the reduced machine, which we use to write output. */
	generateReduced();

	if ( errorCount > 0 )
		abortCompile( 1 );

	verifyWritesHaveData();

	if ( errorCount > 0 )
		abortCompile( 1 );

	/*
	 * From this point on we should not be reporting any errors.
	 */

	openOutput();
	writeOutput();
	closeOutput();
	runRlhc();
}

bool InputData::checkLastRef( InputItem *ii )
{
	if ( generateDot )
		return true;
		
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

			/* If this is the last reference to a pd, we can now clear the
			 * memory for it. */
			if ( lastFlush->pd != 0 && lastFlush->section->lastReference == lastFlush ) {
				if ( lastFlush->pd->instanceList.length() > 0 ) {
					lastFlush->pd->clear();

#ifdef WITH_RAGEL_KELBT
					if ( lastFlush->parser != 0 )
						lastFlush->parser->clear();
#endif
				}
			}

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

#ifdef WITH_RAGEL_KELBT
void InputData::parseKelbt()
{
	/*
	 * Ragel Parser from ragel 6.
	 */
	ifstream *inFileStream;
	stringstream *inStringStream;
	istream *inStream;

	if ( inLibRagel ) {
		/* Open the input file for reading. */
		assert( inputFileName != 0 );
		inStringStream = new stringstream( string( input ) );
		inStream = inStringStream;
	}
	else {
		/* Open the input file for reading. */
		assert( inputFileName != 0 );
		inFileStream = new ifstream( inputFileName );
		if ( ! inFileStream->is_open() )
			error() << "could not open " << inputFileName << " for reading" << endp;
		inStream = inFileStream;
	}

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
	
	if ( inLibRagel )
		delete inStringStream;
	else
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
		makeDefaultFileName();
		makeTranslateOutputFileName();
		createOutputStream();
		openOutput();
		parseKelbt();
		flushRemaining();
		closeOutput();
		runRlhc();
	}

	assert( errorCount == 0 );
}
#endif

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

	if ( generateDot )
		processDot();
	else 
		processCode();

	assert( errorCount == 0 );
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
	if ( generateDot ) {
		parseReduce();
		processDot();
		return true;
	}
	else {
		makeDefaultFileName();
		makeTranslateOutputFileName();
		createOutputStream();
		openOutput();

		bool success = parseReduce();
		if ( success )
			flushRemaining();

		closeOutput();

		if ( success )
			runRlhc();

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
