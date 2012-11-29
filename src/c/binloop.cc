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

namespace C {

BinaryLooped::BinaryLooped( const CodeGenArgs &args )
:
	Binary( args ),

	condOffsets(        "cond_offsets",         *this ),
	condLens(           "cond_lengths",         *this ),
	condKeysV1(         "cond_keys_v1",         *this ),
	condSpacesV1(       "cond_spaces_v1",       *this ),
	keys(               "keys",                 *this ),
	indexOffsets(       "index_offsets",        *this ),
	indicies(           "indicies",             *this ),
	transTargsWi(       "trans_targs_wi",       *this ),
	transActionsWi(     "trans_targs_wi",       *this ),
	transCondSpacesWi(  "trans_cond_spaces_wi", *this ),
	transOffsetsWi(     "trans_offsets_wi",     *this ),
	transLengthsWi(     "trans_lengths_wi",     *this ),
	transTargs(         "trans_targs",          *this ),
	transActions(       "trans_actions",        *this ),
	transCondSpaces(    "trans_cond_spaces",    *this ),
	transOffsets(       "trans_offsets",        *this ),
	transLengths(       "trans_lenghts",        *this ),
	condKeys(           "cond_keys",            *this ),
	condTargs(          "cond_targs",           *this ),
	condActions(        "cond_actions",         *this ),
	toStateActions(     "to_state_actions",     *this ),
	fromStateActions(   "from_state_actions",   *this ),
	eofActions(         "eof_actions",          *this ),
	eofTrans(           "eof_trans",            *this )
{}

void BinaryLooped::setTableState( TableArray::State state )
{
	for ( ArrayVector::Iter i = arrayVector; i.lte(); i++ ) {
		TableArray *tableArray = *i;
		tableArray->setState( state );
	}
}

void BinaryLooped::tableDataPass()
{
	taActions();
	taKeyOffsets();
	taSingleLens();
	taRangeLens();
}

void BinaryLooped::writeData()
{
	/* Run the analysis pass over the table data. */
	setTableState( TableArray::GeneratePass );

	if ( useIndicies )
		setTransPosWi();
	else
		setTransPos();

	/* If there are any transtion functions then output the array. If there
	 * are none, don't bother emitting an empty array that won't be used. */
	if ( redFsm->anyActions() )
		taActions();

	taKeyOffsets();

	OPEN_ARRAY( WIDE_ALPH_TYPE(), K() );
	KEYS();
	CLOSE_ARRAY() <<
	"\n";

	taSingleLens();

	taRangeLens();

	OPEN_ARRAY( ARRAY_TYPE(redFsm->maxIndexOffset), IO() );
	INDEX_OFFSETS();
	CLOSE_ARRAY() <<
	"\n";

	if ( useIndicies ) {

		OPEN_ARRAY( ARRAY_TYPE(redFsm->maxIndex), I() );
		INDICIES();
		CLOSE_ARRAY() <<
		"\n";


		OPEN_ARRAY( ARRAY_TYPE(redFsm->maxState), TCS() );
		TRANS_COND_SPACES_WI();
		CLOSE_ARRAY() <<
		"\n";

		OPEN_ARRAY( "int", TO() );
		TRANS_OFFSETS_WI();
		CLOSE_ARRAY() <<
		"\n";

		OPEN_ARRAY( ARRAY_TYPE(redFsm->maxState), TL() );
		TRANS_LENGTHS_WI();
		CLOSE_ARRAY() <<
		"\n";
	}
	else {

		OPEN_ARRAY( ARRAY_TYPE(redFsm->maxState), TCS() );
		TRANS_COND_SPACES();
		CLOSE_ARRAY() <<
		"\n";

		OPEN_ARRAY( "int", TO() );
		TRANS_OFFSETS();
		CLOSE_ARRAY() <<
		"\n";

		OPEN_ARRAY( ARRAY_TYPE(redFsm->maxState), TL() );
		TRANS_LENGTHS();
		CLOSE_ARRAY() <<
		"\n";
	}

	OPEN_ARRAY( "char", CK() );
	COND_KEYS();
	CLOSE_ARRAY() <<
	"\n";

	OPEN_ARRAY( ARRAY_TYPE(redFsm->maxState), CT() );
	COND_TARGS();
	CLOSE_ARRAY() <<
	"\n";

	OPEN_ARRAY( ARRAY_TYPE(redFsm->maxActionLoc), CA() );
	COND_ACTIONS();
	CLOSE_ARRAY() <<
	"\n";

	if ( redFsm->anyToStateActions() ) {
		OPEN_ARRAY( ARRAY_TYPE(redFsm->maxActionLoc), TSA() );
		TO_STATE_ACTIONS();
		CLOSE_ARRAY() <<
		"\n";
	}

	if ( redFsm->anyFromStateActions() ) {
		OPEN_ARRAY( ARRAY_TYPE(redFsm->maxActionLoc), FSA() );
		FROM_STATE_ACTIONS();
		CLOSE_ARRAY() <<
		"\n";
	}

	if ( redFsm->anyEofActions() ) {
		OPEN_ARRAY( ARRAY_TYPE(redFsm->maxActionLoc), EA() );
		EOF_ACTIONS();
		CLOSE_ARRAY() <<
		"\n";
	}

	if ( redFsm->anyEofTrans() ) {
		OPEN_ARRAY( ARRAY_TYPE(redFsm->maxIndexOffset+1), ET() );
		EOF_TRANS();
		CLOSE_ARRAY() <<
		"\n";
	}

	STATE_IDS();
}

void BinaryLooped::writeExec()
{
	testEofUsed = false;
	outLabelUsed = false;

	out <<
		"	{\n"
		"	int _klen";

	if ( redFsm->anyRegCurStateRef() )
		out << ", _ps";

	out <<
		";\n"
		"	" << UINT() << " _trans;\n" <<
		"	" << UINT() << " _cond;\n";

	if ( redFsm->anyToStateActions() || redFsm->anyRegActions() 
			|| redFsm->anyFromStateActions() )
	{
		out << 
			"	" << PTR_CONST() << ARRAY_TYPE(redFsm->maxActArrItem) << PTR_CONST_END() << 
					POINTER() << "_acts;\n"
			"	" << UINT() << " _nacts;\n";
	}

	out <<
		"	" << PTR_CONST() << WIDE_ALPH_TYPE() << PTR_CONST_END() << POINTER() << "_keys;\n"
		"	const char *_ckeys;\n"
		"	int _cpc;\n"
		"\n";

	if ( !noEnd ) {
		testEofUsed = true;
		out << 
			"	if ( " << P() << " == " << PE() << " )\n"
			"		goto _test_eof;\n";
	}

	if ( redFsm->errState != 0 ) {
		outLabelUsed = true;
		out << 
			"	if ( " << vCS() << " == " << redFsm->errState->id << " )\n"
			"		goto _out;\n";
	}

	out << "_resume:\n";

	if ( redFsm->anyFromStateActions() ) {
		out <<
			"	_acts = " << ARR_OFF( A(),  FSA() + "[" + vCS() + "]" ) << ";\n"
			"	_nacts = " << CAST(UINT()) << " *_acts++;\n"
			"	while ( _nacts-- > 0 ) {\n"
			"		switch ( *_acts++ ) {\n";
			FROM_STATE_ACTION_SWITCH();
			SWITCH_DEFAULT() <<
			"		}\n"
			"	}\n"
			"\n";
	}

	LOCATE_TRANS();

	out << "_match:\n";

	if ( useIndicies )
		out << "	_trans = " << I() << "[_trans];\n";

	LOCATE_COND();

	out << "_match_cond:\n";
	
	if ( redFsm->anyEofTrans() )
		out << "_eof_trans:\n";

	if ( redFsm->anyRegCurStateRef() )
		out << "	_ps = " << vCS() << ";\n";

	out <<
		"	" << vCS() << " = " << CT() << "[_cond];\n"
		"\n";

	if ( redFsm->anyRegActions() ) {
		out <<
			"	if ( " << CA() << "[_cond] == 0 )\n"
			"		goto _again;\n"
			"\n"
			"	_acts = " << ARR_OFF( A(), CA() + "[_cond]" ) << ";\n"
			"	_nacts = " << CAST(UINT()) << " *_acts++;\n"
			"	while ( _nacts-- > 0 )\n	{\n"
			"		switch ( *_acts++ )\n		{\n";
			ACTION_SWITCH();
			SWITCH_DEFAULT() <<
			"		}\n"
			"	}\n"
			"\n";
	}

//	if ( redFsm->anyRegActions() || redFsm->anyActionGotos() || 
//			redFsm->anyActionCalls() || redFsm->anyActionRets() )
		out << "_again:\n";

	if ( redFsm->anyToStateActions() ) {
		out <<
			"	_acts = " << ARR_OFF( A(), TSA() + "[" + vCS() + "]" ) << ";\n"
			"	_nacts = " << CAST(UINT()) << " *_acts++;\n"
			"	while ( _nacts-- > 0 ) {\n"
			"		switch ( *_acts++ ) {\n";
			TO_STATE_ACTION_SWITCH();
			SWITCH_DEFAULT() <<
			"		}\n"
			"	}\n"
			"\n";
	}

	if ( redFsm->errState != 0 ) {
		outLabelUsed = true;
		out << 
			"	if ( " << vCS() << " == " << redFsm->errState->id << " )\n"
			"		goto _out;\n";
	}

	if ( !noEnd ) {
		out << 
			"	if ( ++" << P() << " != " << PE() << " )\n"
			"		goto _resume;\n";
	}
	else {
		out << 
			"	" << P() << " += 1;\n"
			"	goto _resume;\n";
	}
	
	if ( testEofUsed )
		out << "	_test_eof: {}\n";
	
	if ( redFsm->anyEofTrans() || redFsm->anyEofActions() ) {
		out << 
			"	if ( " << P() << " == " << vEOF() << " )\n"
			"	{\n";

		if ( redFsm->anyEofTrans() ) {
			out <<
				"	if ( " << ET() << "[" << vCS() << "] > 0 ) {\n"
				"		_trans = " << ET() << "[" << vCS() << "] - 1;\n"
				"		_cond = " << TO() << "[_trans];\n"
				"		goto _eof_trans;\n"
				"	}\n";
		}

		if ( redFsm->anyEofActions() ) {
			out <<
				"	" << PTR_CONST() << ARRAY_TYPE(redFsm->maxActArrItem) << PTR_CONST_END() << 
						POINTER() << "__acts = " << 
						ARR_OFF( A(), EA() + "[" + vCS() + "]" ) << ";\n"
				"	" << UINT() << " __nacts = " << CAST(UINT()) << " *__acts++;\n"
				"	while ( __nacts-- > 0 ) {\n"
				"		switch ( *__acts++ ) {\n";
				EOF_ACTION_SWITCH();
				SWITCH_DEFAULT() <<
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

/* Determine if we should use indicies or not. */
void BinaryLooped::calcIndexSize()
{
	useIndicies = false;
	return;

	int sizeWithInds = 0, sizeWithoutInds = 0;

	/* Calculate cost of using with indicies. */
	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		int totalIndex = st->outSingle.length() + st->outRange.length() + 
				(st->defTrans == 0 ? 0 : 1);
		sizeWithInds += arrayTypeSize(redFsm->maxIndex) * totalIndex;
	}
	sizeWithInds += arrayTypeSize(redFsm->maxState) * redFsm->transSet.length();
	if ( redFsm->anyActions() )
		sizeWithInds += arrayTypeSize(redFsm->maxActionLoc) * redFsm->transSet.length();

	/* Calculate the cost of not using indicies. */
	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		int totalIndex = st->outSingle.length() + st->outRange.length() + 
				(st->defTrans == 0 ? 0 : 1);
		sizeWithoutInds += arrayTypeSize(redFsm->maxState) * totalIndex;
		if ( redFsm->anyActions() )
			sizeWithoutInds += arrayTypeSize(redFsm->maxActionLoc) * totalIndex;
	}

	/* If using indicies reduces the size, use them. */
	useIndicies = sizeWithInds < sizeWithoutInds;
}

}
