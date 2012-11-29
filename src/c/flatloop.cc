/*
 *  Copyright 2004-2006 Adrian Thurston <thurston@complang.org>
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
#include "flatloop.h"
#include "redfsm.h"
#include "gendata.h"

namespace C {

void FlatLooped::writeData()
{
	setTransPos();

	/* If there are any transtion functions then output the array. If there
	 * are none, don't bother emitting an empty array that won't be used. */
	if ( redFsm->anyActions() ) {
		OPEN_ARRAY( ARRAY_TYPE(redFsm->maxActArrItem), A() );
		ACTIONS_ARRAY();
		CLOSE_ARRAY() <<
		"\n";
	}

	OPEN_ARRAY( WIDE_ALPH_TYPE(), K() );
	KEYS();
	CLOSE_ARRAY() <<
	"\n";

	OPEN_ARRAY( ARRAY_TYPE(redFsm->maxSpan), SP() );
	KEY_SPANS();
	CLOSE_ARRAY() <<
	"\n";

	OPEN_ARRAY( ARRAY_TYPE(redFsm->maxFlatIndexOffset), IO() );
	FLAT_INDEX_OFFSET();
	CLOSE_ARRAY() <<
	"\n";

	OPEN_ARRAY( ARRAY_TYPE(redFsm->maxIndex), I() );
	INDICIES();
	CLOSE_ARRAY() <<
	"\n";

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

void FlatLooped::writeExec()
{
	testEofUsed = false;
	outLabelUsed = false;

	out << 
		"	{\n"
		"	int _slen";

	if ( redFsm->anyRegCurStateRef() )
		out << ", _ps";

	out << 
		";\n"
		"	int _trans, _cond";
	out << ";\n";

	if ( redFsm->anyToStateActions() || 
			redFsm->anyRegActions() || redFsm->anyFromStateActions() )
	{
		out << 
			"	" << PTR_CONST() << ARRAY_TYPE(redFsm->maxActArrItem) << PTR_CONST_END() << POINTER() << "_acts;\n"
			"	" << UINT() << " _nacts;\n"; 
	}

	out <<
		"	" << PTR_CONST() << WIDE_ALPH_TYPE() << PTR_CONST_END() << POINTER() << "_keys;\n"
		"	" << PTR_CONST() << ARRAY_TYPE(redFsm->maxIndex) << PTR_CONST_END() << POINTER() << "_inds;\n"
		"	const char *_ckeys;\n"
		"	int _klen;\n"
		"	int _cpc;\n";

	out << "\n";

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
			"	_acts = " << ARR_OFF( A(), FSA() + "[" + vCS() + "]" ) << ";\n"
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
			"	while ( _nacts-- > 0 ) {\n"
			"		switch ( *(_acts++) )\n		{\n";
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
			"	_acts = " << ARR_OFF( A(),  TSA() + "[" + vCS() + "]" ) << ";\n"
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

}
