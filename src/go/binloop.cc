/*
 *  Copyright 2001-2006 Adrian Thurston <thurston@complang.org>
 *            2004 Erich Ocean <eric.ocean@ampede.com>
 *            2005 Alan West <alan@alanz.com>
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
#include "binloop.h"
#include "redfsm.h"
#include "gendata.h"

namespace Go {

BinaryLooped::BinaryLooped( const CodeGenArgs &args )
:
	Binary( args )
{}

/* Determine if we should use indicies or not. */
void BinaryLooped::calcIndexSize()
{
	long long sizeWithInds =
		indicies.size() +
		transCondSpacesWi.size() +
		transOffsetsWi.size() +
		transLengthsWi.size();

	long long sizeWithoutInds =
		transCondSpaces.size() +
		transOffsets.size() +
		transLengths.size();

	std::cerr << "sizes: " << sizeWithInds << " " << sizeWithoutInds << std::endl;

	///* If using indicies reduces the size, use them. */
	//useIndicies = sizeWithInds < sizeWithoutInds;
	useIndicies = false;
}


void BinaryLooped::tableDataPass()
{
	taActions();
	taKeyOffsets();
	taSingleLens();
	taRangeLens();
	taIndexOffsets();
	taIndicies();

	taTransCondSpacesWi();
	taTransOffsetsWi();
	taTransLengthsWi();

	taTransCondSpaces();
	taTransOffsets();
	taTransLengths();

	taCondTargs();
	taCondActions();

	taToStateActions();
	taFromStateActions();
	taEofActions();

	taEofTransDirect();
	taEofTransIndexed();

	taKeys();
	taCondKeys();
}

void BinaryLooped::genAnalysis()
{
	redFsm->sortByStateId();

	/* Choose default transitions and the single transition. */
	redFsm->chooseDefaultSpan();

	/* Choose the singles. */
	redFsm->chooseSingle();

	/* If any errors have occured in the input file then don't write anything. */
	if ( gblErrorCount > 0 )
		return;

	/* Anlayze Machine will find the final action reference counts, among other
	 * things. We will use these in reporting the usage of fsm directives in
	 * action code. */
	analyzeMachine();

	setKeyType();

	/* Run the analysis pass over the table data. */
	setTableState( TableArray::AnalyzePass );
	tableDataPass();

	/* Determine if we should use indicies. */
	calcIndexSize();

	/* Switch the tables over to the code gen mode. */
	setTableState( TableArray::GeneratePass );
}


void BinaryLooped::COND_ACTION( RedCondAp *cond )
{
	int act = 0;
	if ( cond->action != 0 )
		act = cond->action->location+1;
	condActions.value( act );
}

void BinaryLooped::TO_STATE_ACTION( RedStateAp *state )
{
	int act = 0;
	if ( state->toStateAction != 0 )
		act = state->toStateAction->location+1;
	toStateActions.value( act );
}

void BinaryLooped::FROM_STATE_ACTION( RedStateAp *state )
{
	int act = 0;
	if ( state->fromStateAction != 0 )
		act = state->fromStateAction->location+1;
	fromStateActions.value( act );
}

void BinaryLooped::EOF_ACTION( RedStateAp *state )
{
	int act = 0;
	if ( state->eofAction != 0 )
		act = state->eofAction->location+1;
	eofActions.value( act );
}

std::ostream &BinaryLooped::TO_STATE_ACTION_SWITCH()
{
	/* Walk the list of functions, printing the cases. */
	for ( GenActionList::Iter act = actionList; act.lte(); act++ ) {
		/* Write out referenced actions. */
		if ( act->numToStateRefs > 0 ) {
			/* Write the case label, the action and the case break. */
			out << "\tcase " << act->actionId << ":\n";
			ACTION( out, act, 0, false, false );
		}
	}

	genLineDirective( out );
	return out;
}

std::ostream &BinaryLooped::FROM_STATE_ACTION_SWITCH()
{
	/* Walk the list of functions, printing the cases. */
	for ( GenActionList::Iter act = actionList; act.lte(); act++ ) {
		/* Write out referenced actions. */
		if ( act->numFromStateRefs > 0 ) {
			/* Write the case label, the action and the case break. */
			out << "\tcase " << act->actionId << ":\n";
			ACTION( out, act, 0, false, false );
		}
	}

	genLineDirective( out );
	return out;
}

std::ostream &BinaryLooped::EOF_ACTION_SWITCH()
{
	/* Walk the list of functions, printing the cases. */
	for ( GenActionList::Iter act = actionList; act.lte(); act++ ) {
		/* Write out referenced actions. */
		if ( act->numEofRefs > 0 ) {
			/* Write the case label, the action and the case break. */
			out << "\tcase " << act->actionId << ":\n";
			ACTION( out, act, 0, true, false );
		}
	}

	genLineDirective( out );
	return out;
}


std::ostream &BinaryLooped::ACTION_SWITCH()
{
	/* Walk the list of functions, printing the cases. */
	for ( GenActionList::Iter act = actionList; act.lte(); act++ ) {
		/* Write out referenced actions. */
		if ( act->numTransRefs > 0 ) {
			/* Write the case label, the action and the case break. */
			out << "\tcase " << act->actionId << ":\n";
			ACTION( out, act, 0, false, false );
		}
	}

	genLineDirective( out );
	return out;
}


void BinaryLooped::writeData()
{
	/* If there are any transtion functions then output the array. If there
	 * are none, don't bother emitting an empty array that won't be used. */
	if ( redFsm->anyActions() )
		taActions();

	taKeyOffsets();
	taKeys();
	taSingleLens();
	taRangeLens();
	taIndexOffsets();

	if ( useIndicies ) {
		taIndicies();
		taTransCondSpacesWi();
		taTransOffsetsWi();
		taTransLengthsWi();
	}
	else {
		taTransCondSpaces();
		taTransOffsets();
		taTransLengths();
	}

	taCondKeys();

	taCondTargs();
	taCondActions();

	if ( redFsm->anyToStateActions() )
		taToStateActions();

	if ( redFsm->anyFromStateActions() )
		taFromStateActions();

	if ( redFsm->anyEofActions() )
		taEofActions();

	if ( redFsm->anyEofTrans() ) {
		taEofTransIndexed();
		taEofTransDirect();
	}

	STATE_IDS();
}

void BinaryLooped::writeExec()
{
	testEofUsed = false;
	outLabelUsed = false;

	out <<
		"	{\n"
		"	var _klen";

	if ( redFsm->anyRegCurStateRef() )
		out << ", _ps";

	out <<
		" int\n"
		"	var _trans uint\n" <<
		"	var _cond uint\n";

	if ( redFsm->anyToStateActions() || redFsm->anyRegActions()
			|| redFsm->anyFromStateActions() )
	{
		out <<
			"	var _acts uint\n" // actions array index
			"	var _nacts uint\n";
	}

	out <<
		"	var _keys uint\n" // ALPH_TYPE array index
		"	var _ckeys uint\n" // condKeys array index
		"	var _cpc int\n"
		"\n";

	if ( !noEnd ) {
		testEofUsed = true;
		out <<
			"	if " << P() << " == " << PE() << " {\n"
			"		goto _test_eof\n"
			"	}\n";
	}

	if ( redFsm->errState != 0 ) {
		outLabelUsed = true;
		out <<
			"	if " << vCS() << " == " << redFsm->errState->id << " {\n"
			"		goto _out\n"
			"	}\n";
	}

	out << "_resume:\n";

	if ( redFsm->anyFromStateActions() ) {
		out <<
			"	_acts = " << "uint(" << ARR_REF( fromStateActions ) << "[" << vCS() << "]" << ")\n" // actions array index
			"	_nacts = " << "uint(" << ARR_REF( actions ) << "[_acts]); _acts++\n"
			"	for ; _nacts > 0; _nacts-- {\n"
			"		_acts++\n"
			"		switch " << ARR_REF( actions ) << "[_acts - 1]" << " {\n";
			FROM_STATE_ACTION_SWITCH() <<
			"		}\n"
			"	}\n"
			"\n";
	}

	LOCATE_TRANS();

	out << "_match:\n";

	if ( useIndicies )
		out << "	_trans = " << ARR_REF( indicies ) << "[_trans];\n";

	LOCATE_COND();

	out << "_match_cond:\n";

	if ( redFsm->anyEofTrans() )
		out << "_eof_trans:\n";

	if ( redFsm->anyRegCurStateRef() )
		out << "	_ps = " << vCS() << "\n";

	out <<
		"	" << vCS() << " = " << "int(" << ARR_REF( condTargs ) << "[_cond]" << ")\n"
		"\n";

	if ( redFsm->anyRegActions() ) {
		out <<
			"	if " << ARR_REF( condActions ) << "[_cond] == 0 {\n"
			"		goto _again\n"
			"	}\n"
			"\n"
			"	_acts = " << "uint(" << ARR_REF( condActions ) << "[_cond]" << ")\n" // actions array index
			"	_nacts = " << "uint(" << ARR_REF( actions ) << "[_acts]); _acts++\n"
			"	for ; _nacts > 0; _nacts-- {\n"
			"		_acts++\n"
			"		switch " << ARR_REF( actions ) << "[_acts - 1] {\n";
			ACTION_SWITCH() <<
			"		}\n"
			"	}\n"
			"\n";
	}

//	if ( redFsm->anyRegActions() || redFsm->anyActionGotos() ||
//			redFsm->anyActionCalls() || redFsm->anyActionRets() )
		out << "_again:\n";

	if ( redFsm->anyToStateActions() ) {
		out <<
			"	_acts = " << "uint(" << ARR_REF( toStateActions ) << "[" << vCS() << "]" << ")\n" // actions array index
			"	_nacts = " << "uint(" << ARR_REF( actions ) << "[_acts]); _acts++\n"
			"	for ; _nacts > 0; _nacts-- {\n"
			"		_acts++\n"
			"		switch " << ARR_REF( actions ) << "[_acts - 1]" << " {\n";
			TO_STATE_ACTION_SWITCH() <<
			"		}\n"
			"	}\n"
			"\n";
	}

	if ( redFsm->errState != 0 ) {
		outLabelUsed = true;
		out <<
			"	if " << vCS() << " == " << redFsm->errState->id << " {\n"
			"		goto _out\n"
			"	}\n";
	}

	if ( !noEnd ) {
		out <<
			"	if " << P() << "++; " << P() << " != " << PE() << " {\n"
			"		goto _resume\n"
			"	}\n";
	}
	else {
		out <<
			"	" << P() << " += 1\n"
			"	goto _resume\n";
	}

	if ( testEofUsed )
		out << "	_test_eof: {}\n";

	if ( redFsm->anyEofTrans() || redFsm->anyEofActions() ) {
		out <<
			"	if " << P() << " == " << vEOF() << " {\n";

		if ( redFsm->anyEofTrans() ) {
			TableArray &eofTrans = useIndicies ? eofTransIndexed : eofTransDirect;
			out <<
				"	if " << ARR_REF( eofTrans ) << "[" << vCS() << "] > 0 {\n"
				"		_trans = " << "uint(" << ARR_REF( eofTrans ) << "[" << vCS() << "] - 1)\n"
				"		_cond = " << "uint(" << ARR_REF( transOffsets ) << "[_trans]" << ")\n"
				"		goto _eof_trans\n"
				"	}\n";
		}

		if ( redFsm->anyEofActions() ) {
			out <<
				"	var __acts = " << "uint(" << ARR_REF( eofActions ) << "[" << vCS() << "]" << ")\n" // actions array index
				"	var __nacts = " << "uint(" << ARR_REF( actions ) << "[__acts]); __acts++\n"
				"	for ; __nacts > 0; __nacts-- {\n"
				"		__acts++\n"
				"		switch " << ARR_REF( actions ) << "[__acts - 1]" << " {\n";
				EOF_ACTION_SWITCH() <<
				"		}\n"
				"	}\n";
		}

		out <<
			"	}\n"
			"\n";
	}

	if ( outLabelUsed )
		out << "	_out: {}\n";

	out << "	}\n";
}

}
