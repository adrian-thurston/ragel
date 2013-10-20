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

#ifndef _INPUT_DATA
#define _INPUT_DATA

#include "gendata.h"
#include <iostream>
#include <sstream>

struct ParseData;
struct CondSpace;
struct CondAp;
struct ActionTable;

struct InputItem
{
	enum Type {
		HostData,
		Write,
	};

	Type type;
	std::ostringstream data;
	std::string name;
	ParseData *pd;
	Vector<std::string> writeArgs;

	InputLoc loc;

	InputItem *prev, *next;
};

typedef AvlMap<std::string, ParseData*, CmpString> ParseDataDict;
typedef AvlMapEl<std::string, ParseData*> ParseDataDictEl;
typedef DList<ParseData> ParseDataList;

typedef DList<InputItem> InputItemList;
typedef Vector<const char *> ArgsVector;

struct InputData
{
	InputData() : 
		inputFileName(0),
		outputFileName(0),
		inStream(0),
		outStream(0),
		outFilter(0)
	{}

	/* The name of the root section, this does not change during an include. */
	const char *inputFileName;
	const char *outputFileName;

	/* Io globals. */
	std::istream *inStream;
	std::ostream *outStream;
	output_filter *outFilter;

	ParseDataDict parseDataDict;
	ParseDataList parseDataList;
	InputItemList inputItems;

	ArgsVector includePaths;

	void verifyWritesHaveData();

	void makeFirstInputItem();
	void writeOutput( bool generateDot );
	void makeDefaultFileName();
	void makeOutputStream();
	void openOutput();
	void generateReduced( CodeStyle codeStyle, bool printStatistics );
	void prepareSingleMachine();
	void prepareAllMachines();

	void cdDefaultFileName( const char *inputFile );
	void goDefaultFileName( const char *inputFile );
	void javaDefaultFileName( const char *inputFile );
	void rubyDefaultFileName( const char *inputFile );
	void csharpDefaultFileName( const char *inputFile );
	void ocamlDefaultFileName( const char *inputFile );
	void crackDefaultFileName( const char *inputFile );

	void writeLanguage( std::ostream &out );
	void writeXML( std::ostream &out );

	void processXML();
	void processDot();
	void processCode( CodeStyle codeStyle, bool generateDot, bool printStatistics );

	void writeDot( std::ostream &out );

	void parseArgs( int argc, const char **argv );
	void checkArgs();

	void process( CodeStyle codeStyle, bool generateXML, bool generateDot, bool printStatistics );
};

#endif
