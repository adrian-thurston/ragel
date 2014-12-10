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
#include "cdflat.h"
#include "redfsm.h"
#include "gendata.h"

std::ostream &FlatCodeGen::TO_STATE_ACTION( TableArray &taTSA, RedStateAp *state )
{
	int act = 0;
	if ( state->toStateAction != 0 )
		act = state->toStateAction->location+1;
	taTSA.VAL( act );
	return out;
}

std::ostream &FlatCodeGen::FROM_STATE_ACTION( TableArray &taFSA, RedStateAp *state )
{
	int act = 0;
	if ( state->fromStateAction != 0 )
		act = state->fromStateAction->location+1;
	taFSA.VAL( act );
	return out;
}

std::ostream &FlatCodeGen::EOF_ACTION( TableArray &taEA, RedStateAp *state )
{
	int act = 0;
	if ( state->eofAction != 0 )
		act = state->eofAction->location+1;
	taEA.VAL( act );
	return out;
}

std::ostream &FlatCodeGen::TRANS_ACTION( TableArray &taTA, RedTransAp *trans )
{
	/* If there are actions, emit them. Otherwise emit zero. */
	int act = 0;
	if ( trans->action != 0 )
		act = trans->action->location+1;
	taTA.VAL( act );
	return out;
}

std::ostream &FlatCodeGen::TO_STATE_ACTION_SWITCH()
{
	/* Walk the list of functions, printing the cases. */
	for ( GenActionList::Iter act = actionList; act.lte(); act++ ) {
		/* Write out referenced actions. */
		if ( act->numToStateRefs > 0 ) {
			/* Write the case label, the action and the case break */
			out << "\tcase " << act->actionId << ":\n";
			ACTION( out, act, 0, false, false );
			out << "\tbreak;\n";
		}
	}

	genLineDirective( out );
	return out;
}

std::ostream &FlatCodeGen::FROM_STATE_ACTION_SWITCH()
{
	/* Walk the list of functions, printing the cases. */
	for ( GenActionList::Iter act = actionList; act.lte(); act++ ) {
		/* Write out referenced actions. */
		if ( act->numFromStateRefs > 0 ) {
			/* Write the case label, the action and the case break */
			out << "\tcase " << act->actionId << ":\n";
			ACTION( out, act, 0, false, false );
			out << "\tbreak;\n";
		}
	}

	genLineDirective( out );
	return out;
}

std::ostream &FlatCodeGen::EOF_ACTION_SWITCH()
{
	/* Walk the list of functions, printing the cases. */
	for ( GenActionList::Iter act = actionList; act.lte(); act++ ) {
		/* Write out referenced actions. */
		if ( act->numEofRefs > 0 ) {
			/* Write the case label, the action and the case break */
			out << "\tcase " << act->actionId << ":\n";
			ACTION( out, act, 0, true, false );
			out << "\tbreak;\n";
		}
	}

	genLineDirective( out );
	return out;
}


std::ostream &FlatCodeGen::ACTION_SWITCH()
{
	/* Walk the list of functions, printing the cases. */
	for ( GenActionList::Iter act = actionList; act.lte(); act++ ) {
		/* Write out referenced actions. */
		if ( act->numTransRefs > 0 ) {
			/* Write the case label, the action and the case break */
			out << "\tcase " << act->actionId << ":\n";
			ACTION( out, act, 0, false, false );
			out << "\tbreak;\n";
		}
	}

	genLineDirective( out );
	return out;
}


std::ostream &FlatCodeGen::FLAT_INDEX_OFFSET()
{
	TableArray taIO( *this, arrayType(redFsm->maxFlatIndexOffset), IO() );

	taIO.OPEN();

	int curIndOffset = 0;
	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		/* Write the index offset. */
		taIO.VAL( curIndOffset );
		
		/* Move the index offset ahead. */
		if ( st->transList != 0 ) {
			long long span = st->high - st->low + 1;
			for ( long long pos = 0; pos < span; pos++ )
				curIndOffset += 1;
		}
	}

	taIO.CLOSE();

	return out;
}

std::ostream &FlatCodeGen::KEY_SPANS()
{
	TableArray taSP( *this, arrayType(redFsm->maxSpan), SP() );

	taSP.OPEN();

	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		/* Write singles length. */
		unsigned long long span = 0;
		if ( st->transList != 0 )
			span = keyOps->span( st->lowKey, st->highKey );
		taSP.VAL( span );
	}

	taSP.CLOSE();

	return out;
}

std::ostream &FlatCodeGen::TO_STATE_ACTIONS()
{
	TableArray taTSA( *this, arrayType(redFsm->maxActionLoc), TSA() );

	taTSA.OPEN();

	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		/* Write any eof action. */
		TO_STATE_ACTION( taTSA, st );
	}

	taTSA.CLOSE();

	return out;
}

std::ostream &FlatCodeGen::FROM_STATE_ACTIONS()
{
	TableArray taFSA( *this, arrayType(redFsm->maxActionLoc), FSA() );

	taFSA.OPEN();

	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		/* Write any eof action. */
		FROM_STATE_ACTION( taFSA, st );
	}

	taFSA.CLOSE();

	return out;
}

std::ostream &FlatCodeGen::EOF_ACTIONS()
{
	TableArray taEA( *this, arrayType(redFsm->maxActionLoc), EA() );

	taEA.OPEN();

	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		/* Write any eof action. */
		EOF_ACTION( taEA, st );
	}

	taEA.CLOSE();

	return out;
}

std::ostream &FlatCodeGen::EOF_TRANS()
{
	TableArray taET( *this, arrayType(redFsm->maxIndexOffset+1), ET() );

	taET.OPEN();

	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		/* Write any eof action. */
		long trans = 0;
		if ( st->eofTrans != 0 ) {
			assert( st->eofTrans->pos >= 0 );
			trans = st->eofTrans->pos+1;
		}
		taET.VAL( trans );
	}

	taET.CLOSE();

	return out;
}


std::ostream &FlatCodeGen::COND_KEYS()
{
	TableArray taCK( *this, wideAlphType(), CK() );

	taCK.OPEN();

	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		/* Emit just cond low key and cond high key. */
		taCK.KEY( st->condLowKey );
		taCK.KEY( st->condHighKey );
	}

	/* Output one last number so we don't have to figure out when the last
	 * entry is and avoid writing a comma. */
	taCK.KEY ( 0 );

	taCK.CLOSE();
	return out;
}

std::ostream &FlatCodeGen::COND_KEY_SPANS()
{
	TableArray taCSP( *this, arrayType(redFsm->maxCondSpan), CSP() );

	taCSP.OPEN();

	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		/* Write singles length. */
		unsigned long long span = 0;
		if ( st->condList != 0 )
			span = keyOps->span( st->condLowKey, st->condHighKey );
		taCSP.VAL( span );
	}

	taCSP.CLOSE();
	return out;
}

std::ostream &FlatCodeGen::CONDS()
{
	TableArray taC( *this, arrayType(redFsm->maxCond), C() );

	taC.OPEN();

	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		if ( st->condList != 0 ) {
			/* Walk the singles. */
			unsigned long long span = keyOps->span( st->condLowKey, st->condHighKey );
			for ( unsigned long long pos = 0; pos < span; pos++ ) {
				if ( st->condList[pos] != 0 ) {
					taC.VAL( st->condList[pos]->condSpaceId + 1 );
				}
				else {
					taC.VAL( 0 );
				}
			}
		}
	}

	/* Output one last number so we don't have to figure out when the last
	 * entry is and avoid writing a comma. */
	taC.VAL( 0 );

	taC.CLOSE();
	return out;
}

std::ostream &FlatCodeGen::COND_INDEX_OFFSET()
{
	TableArray taCO( *this, arrayType(redFsm->maxCondIndexOffset), CO() );

	taCO.OPEN();

	int curIndOffset = 0;
	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		/* Write the index offset. */
		taCO.VAL( curIndOffset );
		
		/* Move the index offset ahead. */
		if ( st->condList != 0 )
			curIndOffset += keyOps->span( st->condLowKey, st->condHighKey );
	}

	taCO.CLOSE();
	return out;
}


std::ostream &FlatCodeGen::KEYS()
{
	TableArray taK( *this, wideAlphType(), K() );

	taK.OPEN();

	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		if ( st->transList ) {
			/* Emit just low key and high key. */
			taK.KEY( st->low );
			taK.KEY( st->high );
		}
		else {
			taK.KEY( 0 );
			taK.KEY( 0 );
		}
	}

	/* Output one last number so we don't have to figure out when the last
	 * entry is and avoid writing a comma. */
	taK.KEY( 0 );

	taK.CLOSE();

	return out;
}

std::ostream &FlatCodeGen::ALPH_CLASS()
{
	TableArray taAC( *this, arrayType(redFsm->maxSpan), AC() );
	taAC.OPEN();

	long long maxSpan = keyOps->span( redFsm->lowKey, redFsm->highKey );

	for ( long long pos = 0; pos < maxSpan; pos++ )
		taAC.VAL( redFsm->classMap[pos] );

	taAC.CLOSE();
	return out;
}

std::ostream &FlatCodeGen::INDICIES()
{
	TableArray taI( *this, arrayType(redFsm->maxIndex), I() );

	taI.OPEN();

	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		if ( st->transList != 0 ) {
			long long span = st->high - st->low + 1;
			for ( long long pos = 0; pos < span; pos++ )
				taI.VAL( st->transList[pos]->id );
		}
	}

	/* Originally here for formatting reasons. Likely can be removed. */
	taI.VAL( 0 );

	taI.CLOSE();

	return out;
}

std::ostream &FlatCodeGen::INDEX_DEFAULTS()
{
	TableArray taID( *this, arrayType(redFsm->maxIndex), ID() );

	taID.OPEN();
	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		if ( st->defTrans != 0 )
			taID.VAL( st->defTrans->id );
		else
			taID.VAL( 0 );
	}
	taID.CLOSE();

	return out;
}

std::ostream &FlatCodeGen::TRANS_TARGS()
{
	TableArray taTT( *this, arrayType(redFsm->maxState), TT() );

	taTT.OPEN();

	/* Transitions must be written ordered by their id. */
	RedTransAp **transPtrs = new RedTransAp*[redFsm->transSet.length()];
	for ( TransApSet::Iter trans = redFsm->transSet; trans.lte(); trans++ )
		transPtrs[trans->id] = trans;

	/* Keep a count of the num of items in the array written. */
	for ( int t = 0; t < redFsm->transSet.length(); t++ ) {
		/* Save the position. Needed for eofTargs. */
		RedTransAp *trans = transPtrs[t];
		trans->pos = t;

		/* Write out the target state. */
		taTT.VAL( trans->targ->id );
	}
	delete[] transPtrs;

	taTT.CLOSE();

	return out;
}


std::ostream &FlatCodeGen::TRANS_ACTIONS()
{
	TableArray taTA( *this, arrayType(redFsm->maxActionLoc), TA() );

	taTA.OPEN();

	/* Transitions must be written ordered by their id. */
	RedTransAp **transPtrs = new RedTransAp*[redFsm->transSet.length()];
	for ( TransApSet::Iter trans = redFsm->transSet; trans.lte(); trans++ )
		transPtrs[trans->id] = trans;

	/* Keep a count of the num of items in the array written. */
	for ( int t = 0; t < redFsm->transSet.length(); t++ ) {
		/* Write the function for the transition. */
		RedTransAp *trans = transPtrs[t];
		TRANS_ACTION( taTA, trans );
	}
	delete[] transPtrs;

	taTA.CLOSE();

	return out;
}

void FlatCodeGen::LOCATE_TRANS()
{
	long lowKey = redFsm->lowKey.getVal();
	long highKey = redFsm->highKey.getVal();

	out <<
		"	_keys = " << ARR_OFF( K(), "(" + vCS() + "<<1)" ) << ";\n"
		"	_inds = " << ARR_OFF( I(), IO() + "[" + vCS() + "]" ) << ";\n"
		"\n"
		"	_slen = " << SP() << "[" << vCS() << "];\n"
		"	if ( _slen > 0 && " <<
					GET_WIDE_KEY() << " <= " << highKey << " &&" <<
					lowKey << " <= " << GET_WIDE_KEY() << " ) {\n"
		"		long _ic = " << AC() << "[" << GET_WIDE_KEY() << " - " << lowKey << "];\n" 
		"		_trans = \n"
		"			_ic <= _keys[1] && _keys[0] <= _ic ?\n" 
		"			_inds[ _ic - _keys[0] ] :\n"
		"			" << ID() << "[" << vCS() << "];\n"
		"	} else {\n"
		"		_trans = " << ID() << "[" << vCS() << "];\n"
		"	}\n"
		"\n";
}

void FlatCodeGen::GOTO( ostream &ret, int gotoDest, bool inFinish )
{
	ret << "{" << vCS() << " = " << gotoDest << "; " << 
			CTRL_FLOW() << "goto _again;}";
}

void FlatCodeGen::GOTO_EXPR( ostream &ret, GenInlineItem *ilItem, bool inFinish )
{
	ret << "{" << vCS() << " = (";
	INLINE_LIST( ret, ilItem->children, 0, inFinish, false );
	ret << "); " << CTRL_FLOW() << "goto _again;}";
}

void FlatCodeGen::CURS( ostream &ret, bool inFinish )
{
	ret << "(_ps)";
}

void FlatCodeGen::TARGS( ostream &ret, bool inFinish, int targState )
{
	ret << "(" << vCS() << ")";
}

void FlatCodeGen::NEXT( ostream &ret, int nextDest, bool inFinish )
{
	ret << vCS() << " = " << nextDest << ";";
}

void FlatCodeGen::NEXT_EXPR( ostream &ret, GenInlineItem *ilItem, bool inFinish )
{
	ret << vCS() << " = (";
	INLINE_LIST( ret, ilItem->children, 0, inFinish, false );
	ret << ");";
}

void FlatCodeGen::CALL( ostream &ret, int callDest, int targState, bool inFinish )
{
	if ( prePushExpr != 0 ) {
		ret << "{";
		INLINE_LIST( ret, prePushExpr, 0, false, false );
	}

	ret << "{" << STACK() << "[" << TOP() << "++] = " << vCS() << "; " << vCS() << " = " << 
			callDest << "; " << CTRL_FLOW() << "goto _again;}";

	if ( prePushExpr != 0 )
		ret << "}";
}


void FlatCodeGen::CALL_EXPR( ostream &ret, GenInlineItem *ilItem, int targState, bool inFinish )
{
	if ( prePushExpr != 0 ) {
		ret << "{";
		INLINE_LIST( ret, prePushExpr, 0, false, false );
	}

	ret << "{" << STACK() << "[" << TOP() << "++] = " << vCS() << "; " << vCS() << " = (";
	INLINE_LIST( ret, ilItem->children, targState, inFinish, false );
	ret << "); " << CTRL_FLOW() << "goto _again;}";

	if ( prePushExpr != 0 )
		ret << "}";
}


void FlatCodeGen::RET( ostream &ret, bool inFinish )
{
	ret << "{" << vCS() << " = " << STACK() << "[--" << TOP() << "];";

	if ( postPopExpr != 0 ) {
		ret << "{";
		INLINE_LIST( ret, postPopExpr, 0, false, false );
		ret << "}";
	}

	ret << CTRL_FLOW() << "goto _again;}";
}

void FlatCodeGen::BREAK( ostream &ret, int targState, bool csForced )
{
	outLabelUsed = true;
	ret << "{" << P() << "++; " << CTRL_FLOW() << "goto _out; }";
}

void FlatCodeGen::writeData()
{
	/* If there are any transtion functions then output the array. If there
	 * are none, don't bother emitting an empty array that won't be used. */
	if ( redFsm->anyActions() )
		ACTIONS_ARRAY();

	if ( redFsm->anyConditions() ) {
		COND_KEYS();
		COND_KEY_SPANS();
		CONDS();
		COND_INDEX_OFFSET();
	}

	KEYS();

	KEY_SPANS();

	FLAT_INDEX_OFFSET();

	ALPH_CLASS();

	INDICIES();

	INDEX_DEFAULTS();

	TRANS_TARGS();

	if ( redFsm->anyActions() )
		TRANS_ACTIONS();

	if ( redFsm->anyToStateActions() )
		TO_STATE_ACTIONS();

	if ( redFsm->anyFromStateActions() )
		FROM_STATE_ACTIONS();

	if ( redFsm->anyEofActions() )
		EOF_ACTIONS();

	if ( redFsm->anyEofTrans() )
		EOF_TRANS();

	STATE_IDS();
}

void FlatCodeGen::COND_TRANSLATE()
{
	out << 
		"	_widec = " << GET_KEY() << ";\n";

	out <<
		"	_keys = " << ARR_OFF( CK(), "(" + vCS() + "<<1)" ) << ";\n"
		"	_conds = " << ARR_OFF( C(), CO() + "[" + vCS() + "]" ) << ";\n"
		"\n"
		"	_slen = " << CSP() << "[" << vCS() << "];\n"
		"	_cond = _slen > 0 && _keys[0] <=" << GET_WIDE_KEY() << " &&\n"
		"		" << GET_WIDE_KEY() << " <= _keys[1] ?\n"
		"		_conds[" << GET_WIDE_KEY() << " - _keys[0]] : 0;\n"
		"\n";

	out <<
		"	switch ( _cond ) {\n";
	for ( CondSpaceList::Iter csi = condSpaceList; csi.lte(); csi++ ) {
		GenCondSpace *condSpace = csi;
		out << "	case " << condSpace->condSpaceId + 1 << ": {\n";
		out << TABS(2) << "_widec = " << CAST(WIDE_ALPH_TYPE()) << "(" <<
				KEY(condSpace->baseKey) << " + (" << GET_KEY() << 
				" - " << KEY(keyOps->minKey) << "));\n";

		for ( GenCondSet::Iter csi = condSpace->condSet; csi.lte(); csi++ ) {
			out << TABS(2) << "if ( ";
			CONDITION( out, *csi );
			Size condValOffset = ((1 << csi.pos()) * keyOps->alphSize());
			out << " ) _widec += " << condValOffset << ";\n";
		}

		out << "		}\n";
		out << "		break;\n";
	}

	SWITCH_DEFAULT();

	out <<
		"	}\n";
}

void FlatCodeGen::writeExec()
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
		"	int _trans";

	if ( redFsm->anyConditions() )
		out << ", _cond";
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
		"	" << PTR_CONST() << ARRAY_TYPE(redFsm->maxIndex) << PTR_CONST_END() << POINTER() << "_inds;\n";

	if ( redFsm->anyConditions() ) {
		out << 
			"	" << PTR_CONST() << ARRAY_TYPE(redFsm->maxCond) << PTR_CONST_END() << POINTER() << "_conds;\n"
			"	" << WIDE_ALPH_TYPE() << " _widec;\n";
	}

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

	if ( redFsm->anyConditions() )
		COND_TRANSLATE();

	LOCATE_TRANS();

	if ( redFsm->anyEofTrans() )
		out << "_eof_trans:\n";

	if ( redFsm->anyRegCurStateRef() )
		out << "	_ps = " << vCS() << ";\n";

	out <<
		"	" << vCS() << " = " << TT() << "[_trans];\n"
		"\n";

	if ( redFsm->anyRegActions() ) {
		out <<
			"	if ( " << TA() << "[_trans] == 0 )\n"
			"		goto _again;\n"
			"\n"
			"	_acts = " << ARR_OFF( A(), TA() + "[_trans]" ) << ";\n"
			"	_nacts = " << CAST(UINT()) << " *_acts++;\n"
			"	while ( _nacts-- > 0 ) {\n"
			"		switch ( *(_acts++) )\n		{\n";
			ACTION_SWITCH();
			SWITCH_DEFAULT() <<
			"		}\n"
			"	}\n"
			"\n";
	}

	if ( redFsm->anyRegActions() || redFsm->anyActionGotos() || 
			redFsm->anyActionCalls() || redFsm->anyActionRets() )
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
