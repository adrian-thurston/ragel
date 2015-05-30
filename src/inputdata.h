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
struct Parser6;
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

/* This exists for ragel-6 parsing. */
typedef AvlMap<const char*, Parser6*, CmpStr> ParserDict;
typedef AvlMapEl<const char*, Parser6*> ParserDictEl;
typedef DList<Parser6> ParserList;

typedef DList<InputItem> InputItemList;
typedef Vector<const char *> ArgsVector;

struct InputData
{
	InputData()
	: 
		inputFileName(0),
		outputFileName(0),
		nextMachineId(0),
		inStream(0),
		outStream(0),
		outFilter(0),
		hostLang(&hostLangC),
		codeStyle(GenBinaryLoop),
		dotGenParser(0),
		machineSpec(0),
		machineName(0),
		minimizeLevel(MinimizePartition2),
		minimizeOpt(MinimizeMostOps),
		generateXML(false),
		generateDot(false),
		printStatistics(false),
		wantDupsRemoved(true),
		noLineDirectives(false),
		displayPrintables(false),
		stringTables(false),
		maxTransitions(LONG_MAX),
		numSplitPartitions(0),
		rubyImpl(MRI),
		rlhcShowCmd(false),
		noIntermediate(false),
		frontendSpecified(false),
		backendSpecified(false),
		saveTemps(false),
		nfaTermCheck(0),
		nfaCondsDepth(-1),
		transSpanDepth(6),
		nfaIntermedStateLimit(10000),
		nfaFinalStateLimit(2000),
		varBackend(false)
	{}

	std::string dirName;

	/* The name of the root section, this does not change during an include. */
	const char *inputFileName;
	const char *outputFileName;

	int nextMachineId;

	std::string origOutputFileName;
	std::string genOutputFileName;

	/* Io globals. */
	std::istream *inStream;
	std::ostream *outStream;
	output_filter *outFilter;

	ParseDataDict parseDataDict;
	ParseDataList parseDataList;
	InputItemList inputItems;

	/* Ragel-6 frontend. */
	ParserDict parserDict;
	ParserList parserList;

	ArgsVector includePaths;

	const HostLang *hostLang;

	/* Target language and output style. */
	CodeStyle codeStyle;

	Parser6 *dotGenParser;

	const char *machineSpec;
	const char *machineName;

	MinimizeLevel minimizeLevel;
	MinimizeOpt minimizeOpt;
	bool generateXML;
	bool generateDot;
	bool printStatistics;

	bool wantDupsRemoved;
	bool noLineDirectives;
	bool displayPrintables;
	bool stringTables;

	long maxTransitions;
	int numSplitPartitions;

	/* Target ruby impl */
	RubyImplEnum rubyImpl;

	bool rlhcShowCmd;
	bool noIntermediate;

	bool frontendSpecified;
	RagelFrontend frontend;

	bool backendSpecified;
	RagelBackend backend;

	bool saveTemps;
	bool nfaTermCheck;
	long nfaCondsDepth;
	long transSpanDepth;
	long nfaIntermedStateLimit;
	long nfaFinalStateLimit;

	bool varBackend;

	void verifyWritesHaveData();

	void makeFirstInputItem();
	void writeOutput();
	void makeDefaultFileName();
	void makeOutputStream();
	void openOutput();
	void closeOutput();
	void generateReduced();
	void prepareSingleMachine();
	void prepareAllMachines();

	void cdDefaultFileName( const char *inputFile );
	void goDefaultFileName( const char *inputFile );
	void javaDefaultFileName( const char *inputFile );
	void rubyDefaultFileName( const char *inputFile );
	void csharpDefaultFileName( const char *inputFile );
	void ocamlDefaultFileName( const char *inputFile );
	void crackDefaultFileName( const char *inputFile );
	void asmDefaultFileName( const char *inputFile );


	void writeLanguage( std::ostream &out );
	void writeXML( std::ostream &out );

	void processXML();
	void processDot();
	void processCode();

	void writeDot( std::ostream &out );

	void parseArgs( int argc, const char **argv );
	void checkArgs();
	void terminateAllParsers();

	void process();
};

#endif
