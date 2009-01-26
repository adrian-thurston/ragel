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


void InputData::openOutput()
{
	if ( generateDot )
		outStream = dotOpenOutput( inputFileName );
	else if ( hostLang->lang == HostLang::C )
		outStream = cdOpenOutput( inputFileName );
	else if ( hostLang->lang == HostLang::D )
		outStream = cdOpenOutput( inputFileName );
	else if ( hostLang->lang == HostLang::Java )
		outStream = javaOpenOutput( inputFileName );
	else if ( hostLang->lang == HostLang::Ruby )
		outStream = rubyOpenOutput( inputFileName );
	else if ( hostLang->lang == HostLang::CSharp )
		outStream = csharpOpenOutput( inputFileName );
	else {
		assert( false );
	}
}

void InputData::generateReduced()
{
	/* No machine spec or machine name given. Generate everything. */
	for ( ParserDict::Iter parser = parserDict; parser.lte(); parser++ ) {
		ParseData *pd = parser->value->pd;
		if ( pd->instanceList.length() > 0 )
			pd->prepareMachineGen( 0 );
	}

	openOutput();

	if ( gblErrorCount > 0 )
		return;

	for ( ParserDict::Iter parser = parserDict; parser.lte(); parser++ ) {
		ParseData *pd = parser->value->pd;
		if ( pd->instanceList.length() > 0 )
			pd->generateReduced( *this );
	}

	writeOutput();
}

void InputData::writeOutput()
{
	for ( InputItemList::Iter ii = inputItems; ii.lte(); ii++ ) {
		if ( ii->type == InputItem::Write ) {
			CodeGenMapEl *mapEl = codeGenMap.find( (char*)ii->name.c_str() );
			CodeGenData *cgd = mapEl->value;
			::keyOps = &cgd->thisKeyOps;

			cgd->writeStatement( ii->loc, ii->writeArgs.length()-1, ii->writeArgs.data );
		}
		else {
			*outStream << '\n';
			lineDirective( *outStream, inputFileName, ii->loc.line );
			*outStream << ii->data.str();
		}
	}
}

