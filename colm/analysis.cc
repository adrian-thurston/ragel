/*
 *  Copyright 2012 Adrian Thurston <thurston@complang.org>
 */

/*  This file is part of Colm.
 *
 *  Colm is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 * 
 *  Colm is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 * 
 *  You should have received a copy of the GNU General Public License
 *  along with Colm; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA 
 */

#include <iostream>
#include <iomanip>
#include <errno.h>
#include <stdlib.h>
#include <limits.h>
#include <sstream>

#include "global.h"
#include "lmparse.h"
#include "parsedata.h"
#include "parsetree.h"
#include "mergesort.h"
#include "redbuild.h"
#include "pdacodegen.h"
#include "fsmcodegen.h"
#include "fsmrun.h"
#include "pdarun.h"

using namespace std;


void ParseData::prepGrammar()
{
	/* This will create language elements. */
	wrapNonTerminals();

	makeLangElIds();
	makeLangElNames();
	makeDefinitionNames();
	noUndefindLangEls();

	/* Put the language elements in an index by language element id. */
	langElIndex = new LangEl*[nextSymbolId+1];
	memset( langElIndex, 0, sizeof(LangEl*)*(nextSymbolId+1) );
	for ( LelList::Iter lel = langEls; lel.lte(); lel++ )
		langElIndex[lel->id] = lel;

	makeProdFsms();

	/* Allocate the Runtime data now. Every PdaTable that we make 
	 * will reference it, but it will be filled in after all the tables are
	 * built. */
	runtimeData = new RuntimeData;
}

void ParseData::semanticAnalysis()
{
	beginProcessing();
	initKeyOps();

	/* Type declaration. */
	typeDeclaration();

	/* Type resolving. */
	typeResolve();

	makeTerminalWrappers();
	makeEofElements();
	makeIgnoreCollectors();

	/*
	 * Parsers
	 */

	/* Init the longest match data */
	initLongestMatchData();
	FsmGraph *fsmGraph = makeScanner();

	if ( colm_log_compile ) {
		printNameTree( fsmGraph->rootName );
		printNameIndex( fsmGraph->nameIndex );
	}

	prepGrammar();

	/* Compile bytecode. */
	compileByteCode();

	/* Make the reduced fsm. */
	RedFsmBuild reduce( sectionName, this, fsmGraph );
	redFsm = reduce.reduceMachine();

	BstSet<LangEl*> parserEls;
	collectParserEls( parserEls );

	makeParser( parserEls );

	/* Make the scanner tables. */
	fsmTables = redFsm->makeFsmTables();

	/* Now that all parsers are built, make the global runtimeData. */
	makeRuntimeData();

	/* 
	 * All compilation is now complete.
	 */
	
	/* Parse patterns and replacements. */
	parsePatterns();
}

