/*
 *  Copyright 2001-2014 Adrian Thurston <thurston@complang.org>
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
#include "binbasic.h"
#include "redfsm.h"
#include "gendata.h"

namespace C {

BinaryBasic::BinaryBasic( const CodeGenArgs &args )
:
	Binary( args )
{}

/* Determine if we should use indicies or not. */
void BinaryBasic::calcIndexSize()
{
//	long long sizeWithInds =
//		indicies.size() +
//		transCondSpacesWi.size() +
//		transOffsetsWi.size() +
//		transLengthsWi.size();

//	long long sizeWithoutInds =
//		transCondSpaces.size() +
//		transOffsets.size() +
//		transLengths.size();

	//std::cerr << "sizes: " << sizeWithInds << " " << sizeWithoutInds << std::endl;

	///* If using indicies reduces the size, use them. */
	//useIndicies = sizeWithInds < sizeWithoutInds;
	useIndicies = false;
}


void BinaryBasic::tableDataPass()
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

void BinaryBasic::genAnalysis()
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


void BinaryBasic::COND_ACTION( RedCondAp *cond )
{
	int act = 0;
	if ( cond->action != 0 )
		act = cond->action->location+1;
	condActions.value( act );
}

void BinaryBasic::TO_STATE_ACTION( RedStateAp *state )
{
	int act = 0;
	if ( state->toStateAction != 0 )
		act = state->toStateAction->location+1;
	toStateActions.value( act );
}

void BinaryBasic::FROM_STATE_ACTION( RedStateAp *state )
{
	int act = 0;
	if ( state->fromStateAction != 0 )
		act = state->fromStateAction->location+1;
	fromStateActions.value( act );
}

void BinaryBasic::EOF_ACTION( RedStateAp *state )
{
	int act = 0;
	if ( state->eofAction != 0 )
		act = state->eofAction->location+1;
	eofActions.value( act );
}

std::ostream &BinaryBasic::TO_STATE_ACTION_SWITCH()
{
	/* Walk the list of functions, printing the cases. */
	for ( GenActionList::Iter act = actionList; act.lte(); act++ ) {
		/* Write out referenced actions. */
		if ( act->numToStateRefs > 0 ) {
			/* Write the case label, the action and the case break. */
			out << "\t case " << act->actionId << " {\n";
			ACTION( out, act, IlOpts( 0, false, false ) );
			out << "\n\t}\n";
		}
	}

	return out;
}

std::ostream &BinaryBasic::FROM_STATE_ACTION_SWITCH()
{
	/* Walk the list of functions, printing the cases. */
	for ( GenActionList::Iter act = actionList; act.lte(); act++ ) {
		/* Write out referenced actions. */
		if ( act->numFromStateRefs > 0 ) {
			/* Write the case label, the action and the case break. */
			out << "\t case " << act->actionId << " {\n";
			ACTION( out, act, IlOpts( 0, false, false ) );
			out << "\n\t}\n";
		}
	}

	return out;
}

std::ostream &BinaryBasic::EOF_ACTION_SWITCH()
{
	/* Walk the list of functions, printing the cases. */
	for ( GenActionList::Iter act = actionList; act.lte(); act++ ) {
		/* Write out referenced actions. */
		if ( act->numEofRefs > 0 ) {
			/* Write the case label, the action and the case break. */
			out << "\t case " << act->actionId << " {\n";
			ACTION( out, act, IlOpts( 0, true, false ) );
			out << "\n\t}\n";
		}
	}

	return out;
}


std::ostream &BinaryBasic::ACTION_SWITCH()
{
	/* Walk the list of functions, printing the cases. */
	for ( GenActionList::Iter act = actionList; act.lte(); act++ ) {
		/* Write out referenced actions. */
		if ( act->numTransRefs > 0 ) {
			/* Write the case label, the action and the case break. */
			out << "\t case " << act->actionId << " {\n";
			ACTION( out, act, IlOpts( 0, false, false ) );
			out << "\n\t}\n";
		}
	}

	return out;
}


void BinaryBasic::writeData()
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

void BinaryBasic::LOCATE_TRANS()
{
	out <<
		"	_keys = offset( " << ARR_REF( keys ) << ", " << ARR_REF( keyOffsets ) << "[" << vCS() << "]" << " );\n"
		"	_trans = (uint)" << ARR_REF( indexOffsets ) << "[" << vCS() << "];\n"
		"	_have = 0;\n"
		"\n"
		"	_klen = (int)" << ARR_REF( singleLens ) << "[" << vCS() << "];\n"
		"	if ( _klen > 0 ) {\n"
		"		index " << ALPH_TYPE() << " _lower;\n"
		"		index " << ALPH_TYPE() << " _mid;\n"
		"		index " << ALPH_TYPE() << " _upper;\n"
		"		_lower = _keys;\n"
		"		_upper = _keys + _klen - 1;\n"
		"		while ( _upper >= _lower && _have == 0 ) {\n"
		"			_mid = _lower + ((_upper-_lower) >> 1);\n"
		"			if ( " << GET_KEY() << " < deref( " << ARR_REF( keys ) << ", _mid ) )\n"
		"				_upper = _mid - 1;\n"
		"			else if ( " << GET_KEY() << " > deref( " << ARR_REF( keys ) << ", _mid ) )\n"
		"				_lower = _mid + 1;\n"
		"			else {\n"
		"				_trans += " << "(uint)" << "(_mid - _keys);\n"
		"				_have = 1;\n"
		"			}\n"
		"		}\n"
		"		if ( _have == 0 ) {\n"
		"			_keys += _klen;\n"
		"			_trans += (uint)_klen;\n"
		"		}\n"
		"	}\n"
		"\n"
		"	if ( _have == 0 ) {\n"
		"	_klen = (int)" << ARR_REF( rangeLens ) << "[" << vCS() << "];\n"
		"	if ( _klen > 0 ) {\n"
		"		index " << ALPH_TYPE() << " _lower;\n"
		"		index " << ALPH_TYPE() << " _mid;\n"
		"		index " << ALPH_TYPE() << " _upper;\n"
		"		_lower = _keys;\n"
		"		_upper = _keys + (_klen<<1) - 2;\n"
		"		while ( _have == 0 && _lower <= _upper ) {\n"
		"			_mid = _lower + (((_upper-_lower) >> 1) & ~1);\n"
		"			if ( " << GET_KEY() << " < deref( " << ARR_REF( keys ) << ", _mid ) )\n"
		"				_upper = _mid - 2;\n"
		"			else if ( " << GET_KEY() << " > deref( " << ARR_REF( keys ) << ", _mid + 1 ) )\n"
		"				_lower = _mid + 2;\n"
		"			else {\n"
		"				_trans += " << "(uint)" << "((_mid - _keys)>>1);\n"
		"				_have = 1;\n"
		"			}\n"
		"		}\n"
		"		if ( _have == 0 )\n"
		"			_trans += (uint)_klen;\n"
		"	}\n"
		"	}\n"
		"\n";
}

void BinaryBasic::LOCATE_COND()
{
	out <<
		"	_ckeys = offset( " << ARR_REF( condKeys ) << ",  " << ARR_REF( transOffsets ) << "[_trans] );\n"
		"	_klen = (int) " << ARR_REF( transLengths ) << "[_trans];\n"
		"	_cond = (uint) " << ARR_REF( transOffsets ) << "[_trans];\n"
		"\n";

	out <<
		"	_cpc = 0;\n"
		"	switch ( " << ARR_REF( transCondSpaces ) << "[_trans] ) {\n"
		"\n";

	for ( CondSpaceList::Iter csi = condSpaceList; csi.lte(); csi++ ) {
		GenCondSpace *condSpace = csi;
		out << "	case " << condSpace->condSpaceId << " {\n";
		for ( GenCondSet::Iter csi = condSpace->condSet; csi.lte(); csi++ ) {
			out << TABS(2) << "if ( ";
			CONDITION( out, *csi );
			Size condValOffset = (1 << csi.pos());
			out << " ) _cpc += " << condValOffset << ";\n";
		}

		out << 
			"	}\n";
	}

	out << 
		"	}\n";
	
	out <<
		"	{\n"
		"		index " << ARR_TYPE( condKeys ) << " _lower;\n"
		"		index " << ARR_TYPE( condKeys ) << " _mid;\n"
		"		index " << ARR_TYPE( condKeys ) << " _upper;\n"
		"		_lower = _ckeys;\n"
		"		_upper = _ckeys + _klen - 1;\n"
		"		while ( TRUE ) {\n"
		"			if ( _upper < _lower )\n"
		"				break;\n"
		"\n"
		"			_mid = _lower + ((_upper-_lower) >> 1);\n"
		"			if ( _cpc < (int)deref( " << ARR_REF( condKeys ) << ", _mid ) )\n"
		"				_upper = _mid - 1;\n"
		"			else if ( _cpc > (int)deref( " << ARR_REF( condKeys ) << ", _mid ) )\n"
		"				_lower = _mid + 1;\n"
		"			else {\n"
		"				_cond += (uint)(_mid - _ckeys);\n"
		"				goto _match_cond;\n"
		"			}\n"
		"		}\n"
		"		" << vCS() << " = " << ERROR_STATE() << ";\n"
		"		goto _out;\n"
		"	}\n"
	;
	outLabelUsed = true;
}


void BinaryBasic::writeExec()
{
	testEofUsed = false;
	outLabelUsed = false;

	out <<
		"	{\n"
		"	int _klen;\n";

	if ( redFsm->anyRegCurStateRef() )
		out << "	int _ps;\n";

	out <<
		"	uint _trans = 0;\n" <<
		"	uint _cond = 0;\n"
		"	uint _have = 0;\n";

	if ( redFsm->anyToStateActions() || redFsm->anyRegActions() 
			|| redFsm->anyFromStateActions() )
	{
		out << 
			"	index " << ARR_TYPE( actions ) << " _acts;\n"
			"	uint _nacts;\n";
	}

	out <<
		"	index " << ALPH_TYPE() << " _keys;\n"
		"	index " << ARR_TYPE( condKeys ) << " _ckeys;\n"
		"	int _cpc;\n"
		"	entry {\n"
		"\n";

	if ( redFsm->errState != 0 ) {
		outLabelUsed = true;
		out << 
			"	if ( " << vCS() << " == " << redFsm->errState->id << " )\n"
			"		goto _out;\n";
	}

	out << "label _resume {\n";

	if ( !noEnd ) {
		out << 
			"	if ( " << P() << " == " << PE() << " ) {\n";

		if ( redFsm->anyEofTrans() || redFsm->anyEofActions() ) {
			out << 
				"	if ( " << P() << " == " << vEOF() << " )\n"
				"	{\n";

			if ( redFsm->anyEofTrans() ) {
				TableArray &eofTrans = useIndicies ? eofTransIndexed : eofTransDirect;
				out <<
					"	if ( " << ARR_REF( eofTrans ) << "[" << vCS() << "] > 0 ) {\n"
					"		_trans = (uint)" << ARR_REF( eofTrans ) << "[" << vCS() << "] - 1;\n"
					"		_cond = (uint)" << ARR_REF( transOffsets ) << "[_trans];\n"
					"		goto _match_cond;\n"
					"	}\n";
			}

			if ( redFsm->anyEofActions() ) {
				out <<
					"	index " << ARR_TYPE( actions ) << " __acts;\n"
					"	uint __nacts;\n"
					"	__acts = offset( " << ARR_REF( actions ) << ", " <<
							ARR_REF( eofActions ) << "[" << vCS() << "]" << " );\n"
					"	__nacts = (uint) deref( " << ARR_REF( actions ) << ", __acts );\n"
					"	__acts += 1;\n"
					"	while ( __nacts > 0 ) {\n"
					"		switch ( deref( " << ARR_REF( actions ) << ", __acts ) ) {\n";
					EOF_ACTION_SWITCH() <<
					"		}\n"
					"		__nacts -= 1;\n"
					"		__acts += 1;\n"
					"	}\n";
			}
			
			out << 
				"	}\n"
				"\n";
		}
		out << 
			"	goto _out;\n"
			"	}\n";

	}

	if ( redFsm->anyFromStateActions() ) {
		out <<
			"	_acts = offset( " << ARR_REF( actions ) << ", " << ARR_REF( fromStateActions ) <<
					"[" << vCS() << "]" << " );\n"
			"	_nacts = (uint) deref( " << ARR_REF( actions ) << ", _acts );\n"
			"	_acts += 1;\n"
			"	while ( _nacts > 0 ) {\n"
			"		switch ( deref( " << ARR_REF( actions ) << ", _acts ) ) {\n";
			FROM_STATE_ACTION_SWITCH() <<
			"		}\n"
			"		_nacts -= 1;\n"
			"		_acts += 1;\n"
			"	}\n"
			"\n";
	}

	LOCATE_TRANS();

	if ( useIndicies )
		out << "	_trans = " << ARR_REF( indicies ) << "[_trans];\n";

	LOCATE_COND();

	out << "}\n";
	out << "label _match_cond {\n";
	
	if ( redFsm->anyRegCurStateRef() )
		out << "	_ps = " << vCS() << ";\n";

	out <<
		"	" << vCS() << " = (int)" << ARR_REF( condTargs ) << "[_cond];\n"
		"\n";

	if ( redFsm->anyRegActions() ) {
		out <<
			"	if ( " << ARR_REF( condActions ) << "[_cond] != 0 ) {\n"
			"		_acts = offset( " << ARR_REF( actions ) << ", " << ARR_REF( condActions ) << "[_cond]" << " );\n"
			"		_nacts = (uint) deref( " << ARR_REF( actions ) << ", _acts );\n"
			"		_acts += 1;\n"
			"		while ( _nacts > 0 )\n	{\n"
			"			switch ( deref( " << ARR_REF( actions ) << ", _acts ) )\n"
			"			{\n";
			ACTION_SWITCH() <<
			"			}\n"
			"			_nacts -= 1;\n"
			"			_acts += 1;\n"
			"		}\n"
			"	}\n"
			"\n";
	}

	if ( /*redFsm->anyRegActions() || */ redFsm->anyActionGotos() || 
			redFsm->anyActionCalls() || redFsm->anyActionRets() )
	{
		out << "}\n";
		out << "label _again {\n";
	}

	if ( redFsm->anyToStateActions() ) {
		out <<
			"	_acts = offset( " << ARR_REF( actions ) << ", " << ARR_REF( toStateActions ) <<
					"[" << vCS() << "]" << " );\n"
			"	_nacts = (uint) deref( " << ARR_REF( actions ) << ", _acts );\n"
			"	_acts += 1;\n"
			"	while ( _nacts > 0 ) {\n"
			"		switch ( deref( " << ARR_REF( actions ) << ", _acts ) ) {\n";
			TO_STATE_ACTION_SWITCH() <<
			"		}\n"
			"		_nacts -= 1;\n"
			"		_acts += 1;\n"
			"	}\n"
			"\n";
	}

	if ( redFsm->errState != 0 ) {
		outLabelUsed = true;
		out << 
			"	if ( " << vCS() << " == " << redFsm->errState->id << " )\n"
			"		goto _out;\n";
	}

	out << 
		"	" << P() << " += 1;\n"
		"	goto _resume;\n";

	if ( outLabelUsed )
		out << "} label _out { {}\n";

	/* The entry loop. */
	out << "}}\n";

	/* The execute block. */
	out << "	}\n";
}

}
