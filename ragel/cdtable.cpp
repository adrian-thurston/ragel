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
#include "cdtable.h"
#include "redfsm.h"
#include "gendata.h"

/* Determine if we should use indicies or not. */
void TabCodeGen::calcIndexSize()
{
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

std::ostream &TabCodeGen::TO_STATE_ACTION( TableArray &taTSA, RedStateAp *state )
{
	int act = 0;
	if ( state->toStateAction != 0 )
		act = state->toStateAction->location+1;
	taTSA.VAL( act );
	return out;
}

std::ostream &TabCodeGen::FROM_STATE_ACTION( TableArray &taFSA, RedStateAp *state )
{
	int act = 0;
	if ( state->fromStateAction != 0 )
		act = state->fromStateAction->location+1;
	taFSA.VAL( act );
	return out;
}

std::ostream &TabCodeGen::EOF_ACTION( TableArray &taEA, RedStateAp *state )
{
	int act = 0;
	if ( state->eofAction != 0 )
		act = state->eofAction->location+1;
	taEA.VAL( act );
	return out;
}


std::ostream &TabCodeGen::TRANS_ACTION( TableArray &taTA, RedTransAp *trans )
{
	/* If there are actions, emit them. Otherwise emit zero. */
	int act = 0;
	if ( trans->action != 0 )
		act = trans->action->location+1;
	taTA.VAL( act );
	return out;
}

std::ostream &TabCodeGen::TO_STATE_ACTION_SWITCH()
{
	/* Walk the list of functions, printing the cases. */
	for ( GenActionList::Iter act = actionList; act.lte(); act++ ) {
		/* Write out referenced actions. */
		if ( act->numToStateRefs > 0 ) {
			/* Write the case label, the action and the case break. */
			out << "\tcase " << act->actionId << ":\n";
			ACTION( out, act, 0, false, false );
			out << "\tbreak;\n";
		}
	}

	genLineDirective( out );
	return out;
}

std::ostream &TabCodeGen::FROM_STATE_ACTION_SWITCH()
{
	/* Walk the list of functions, printing the cases. */
	for ( GenActionList::Iter act = actionList; act.lte(); act++ ) {
		/* Write out referenced actions. */
		if ( act->numFromStateRefs > 0 ) {
			/* Write the case label, the action and the case break. */
			out << "\tcase " << act->actionId << ":\n";
			ACTION( out, act, 0, false, false );
			out << "\tbreak;\n";
		}
	}

	genLineDirective( out );
	return out;
}

std::ostream &TabCodeGen::EOF_ACTION_SWITCH()
{
	/* Walk the list of functions, printing the cases. */
	for ( GenActionList::Iter act = actionList; act.lte(); act++ ) {
		/* Write out referenced actions. */
		if ( act->numEofRefs > 0 ) {
			/* Write the case label, the action and the case break. */
			out << "\tcase " << act->actionId << ":\n";
			ACTION( out, act, 0, true, false );
			out << "\tbreak;\n";
		}
	}

	genLineDirective( out );
	return out;
}


std::ostream &TabCodeGen::ACTION_SWITCH()
{
	/* Walk the list of functions, printing the cases. */
	for ( GenActionList::Iter act = actionList; act.lte(); act++ ) {
		/* Write out referenced actions. */
		if ( act->numTransRefs > 0 ) {
			/* Write the case label, the action and the case break. */
			out << "\tcase " << act->actionId << ":\n";
			ACTION( out, act, 0, false, false );
			out << "\tbreak;\n";
		}
	}

	genLineDirective( out );
	return out;
}

std::ostream &TabCodeGen::COND_OFFSETS()
{
	TableArray taCO( *this, arrayType(redFsm->maxCondOffset), CO() );

	taCO.OPEN();

	int curKeyOffset = 0;
	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		/* Write the key offset. */
		taCO.VAL( curKeyOffset );

		/* Move the key offset ahead. */
		curKeyOffset += st->stateCondList.length();
	}

	taCO.CLOSE();

	return out;
}

std::ostream &TabCodeGen::KEY_OFFSETS()
{
	TableArray taKO( *this, arrayType(redFsm->maxKeyOffset), KO() );

	taKO.OPEN();

	int curKeyOffset = 0;
	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		/* Write the key offset. */
		taKO.VAL( curKeyOffset );

		/* Move the key offset ahead. */
		curKeyOffset += st->outSingle.length() + st->outRange.length()*2;
	}

	taKO.CLOSE();

	return out;
}


std::ostream &TabCodeGen::INDEX_OFFSETS()
{
	TableArray taIO( *this, arrayType(redFsm->maxIndexOffset), IO() );

	taIO.OPEN();

	int curIndOffset = 0;
	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		/* Write the index offset. */
		taIO.VAL( curIndOffset );

		/* Move the index offset ahead. */
		curIndOffset += st->outSingle.length() + st->outRange.length();
		if ( st->defTrans != 0 )
			curIndOffset += 1;
	}

	taIO.CLOSE();

	return out;
}

std::ostream &TabCodeGen::COND_LENS()
{
	TableArray taCL( *this, arrayType(redFsm->maxCondLen), CL() );

	taCL.OPEN();

	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		/* Write singles length. */
		taCL.VAL( st->stateCondList.length() );
	}

	taCL.CLOSE();
	return out;
}


std::ostream &TabCodeGen::SINGLE_LENS()
{
	TableArray taSL( *this, arrayType(redFsm->maxSingleLen), SL() );

	taSL.OPEN();

	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		/* Write singles length. */
		taSL.VAL( st->outSingle.length() );
	}

	taSL.CLOSE();
	return out;
}

std::ostream &TabCodeGen::RANGE_LENS()
{
	TableArray taRL( *this, arrayType(redFsm->maxRangeLen), RL() );

	taRL.OPEN();

	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		/* Emit length of range index. */
		taRL.VAL( st->outRange.length() );
	}

	taRL.CLOSE();
	return out;
}

std::ostream &TabCodeGen::TO_STATE_ACTIONS()
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

std::ostream &TabCodeGen::FROM_STATE_ACTIONS()
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

std::ostream &TabCodeGen::EOF_ACTIONS()
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

std::ostream &TabCodeGen::EOF_TRANS()
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


std::ostream &TabCodeGen::COND_KEYS()
{
	TableArray taCK( *this, wideAlphType(), CK() );

	taCK.OPEN();

	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		/* Loop the state's transitions. */
		for ( GenStateCondList::Iter sc = st->stateCondList; sc.lte(); sc++ ) {
			/* Lower key. */
			taCK.KEY( sc->lowKey );

			/* Upper key. */
			taCK.KEY( sc->highKey );
		}
	}

	/* Output one last number so we don't have to figure out when the last
	 * entry is and avoid writing a comma. */
	taCK.KEY( 0 );

	taCK.CLOSE();

	return out;
}

std::ostream &TabCodeGen::COND_SPACES()
{
	TableArray taC( *this, arrayType(redFsm->maxCondSpaceId), C() );

	taC.OPEN();

	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		/* Loop the state's transitions. */
		for ( GenStateCondList::Iter sc = st->stateCondList; sc.lte(); sc++ ) {
			/* Cond Space id. */
			taC.VAL( sc->condSpace->condSpaceId );
		}
	}

	/* Output one last number so we don't have to figure out when the last
	 * entry is and avoid writing a comma. */
	taC.VAL( 0 );

	taC.CLOSE();
	return out;
}

std::ostream &TabCodeGen::KEYS()
{
	TableArray taK( *this, wideAlphType(), K() );

	taK.OPEN();

	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		/* Loop the singles. */
		for ( RedTransList::Iter stel = st->outSingle; stel.lte(); stel++ ) {
			taK.KEY( stel->lowKey );
		}

		/* Loop the state's transitions. */
		for ( RedTransList::Iter rtel = st->outRange; rtel.lte(); rtel++ ) {
			/* Lower key. */
			taK.KEY( rtel->lowKey );

			/* Upper key. */
			taK.KEY( rtel->highKey );
		}
	}

	/* Output one last number so we don't have to figure out when the last
	 * entry is and avoid writing a comma. */
	taK.KEY( 0 );

	taK.CLOSE();

	return out;
}

std::ostream &TabCodeGen::INDICIES()
{
	TableArray taI( *this, arrayType(redFsm->maxIndex), I() );

	taI.OPEN();

	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		/* Walk the singles. */
		for ( RedTransList::Iter stel = st->outSingle; stel.lte(); stel++ ) {
			taI.VAL( stel->value->id );
		}

		/* Walk the ranges. */
		for ( RedTransList::Iter rtel = st->outRange; rtel.lte(); rtel++ ) {
			taI.VAL( rtel->value->id );
		}

		/* The state's default index goes next. */
		if ( st->defTrans != 0 ) {
			taI.VAL( st->defTrans->id );
		}
	}

	/* Output one last number so we don't have to figure out when the last
	 * entry is and avoid writing a comma. */
	taI.VAL( 0 );

	taI.CLOSE();
	return out;
}

std::ostream &TabCodeGen::TRANS_TARGS()
{
	TableArray taTT( *this, arrayType(redFsm->maxState), TT() );

	taTT.OPEN();

	int totalTrans = 0;
	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		/* Walk the singles. */
		for ( RedTransList::Iter stel = st->outSingle; stel.lte(); stel++ ) {
			RedTransAp *trans = stel->value;
			taTT.VAL( trans->targ->id );
			totalTrans += 1;
		}

		/* Walk the ranges. */
		for ( RedTransList::Iter rtel = st->outRange; rtel.lte(); rtel++ ) {
			RedTransAp *trans = rtel->value;
			taTT.VAL( trans->targ->id );
			totalTrans += 1;
		}

		/* The state's default target state. */
		if ( st->defTrans != 0 ) {
			RedTransAp *trans = st->defTrans;
			taTT.VAL( trans->targ->id );
			totalTrans += 1;
		}
	}

	/* Add any eof transitions that have not yet been written out above. */
	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		if ( st->eofTrans != 0 ) {
			RedTransAp *trans = st->eofTrans;
			trans->pos = totalTrans;
			taTT.VAL( trans->targ->id );
			totalTrans += 1;
		}
	}

	/* Output one last number so we don't have to figure out when the last
	 * entry is and avoid writing a comma. */
	taTT.VAL( 0 );

	taTT.CLOSE();

	return out;
}


std::ostream &TabCodeGen::TRANS_ACTIONS()
{
	TableArray taTA( *this, arrayType(redFsm->maxActionLoc), TA() );

	taTA.OPEN();

	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		/* Walk the singles. */
		for ( RedTransList::Iter stel = st->outSingle; stel.lte(); stel++ ) {
			RedTransAp *trans = stel->value;
			TRANS_ACTION( taTA, trans );
		}

		/* Walk the ranges. */
		for ( RedTransList::Iter rtel = st->outRange; rtel.lte(); rtel++ ) {
			RedTransAp *trans = rtel->value;
			TRANS_ACTION( taTA, trans );
		}

		/* The state's default index goes next. */
		if ( st->defTrans != 0 ) {
			RedTransAp *trans = st->defTrans;
			TRANS_ACTION( taTA, trans );
		}
	}

	/* Add any eof transitions that have not yet been written out above. */
	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		if ( st->eofTrans != 0 ) {
			RedTransAp *trans = st->eofTrans;
			TRANS_ACTION( taTA, trans );
		}
	}

	/* Output one last number so we don't have to figure out when the last
	 * entry is and avoid writing a comma. */
	taTA.VAL( 0 );

	taTA.CLOSE();

	return out;
}

std::ostream &TabCodeGen::TRANS_TARGS_WI()
{
	TableArray taTT( *this, arrayType(redFsm->maxState), TT() );

	taTT.OPEN();

	/* Transitions must be written ordered by their id. */
	RedTransAp **transPtrs = new RedTransAp*[redFsm->transSet.length()];
	for ( TransApSet::Iter trans = redFsm->transSet; trans.lte(); trans++ )
		transPtrs[trans->id] = trans;

	/* Keep a count of the num of items in the array written. */
	for ( int t = 0; t < redFsm->transSet.length(); t++ ) {
		/* Record the position, need this for eofTrans. */
		RedTransAp *trans = transPtrs[t];
		trans->pos = t;

		/* Write out the target state. */
		taTT.VAL( trans->targ->id );
	}
	delete[] transPtrs;

	taTT.CLOSE();

	return out;
}


std::ostream &TabCodeGen::TRANS_ACTIONS_WI()
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

void TabCodeGen::LOCATE_TRANS()
{
	out <<
		"	_keys = " << ARR_OFF( K(), KO() + "[" + vCS() + "]" ) << ";\n"
		"	_trans = " << IO() << "[" << vCS() << "];\n"
		"\n"
		"	_klen = " << SL() << "[" << vCS() << "];\n"
		"	if ( _klen > 0 ) {\n"
		"		" << PTR_CONST() << WIDE_ALPH_TYPE() << PTR_CONST_END() << POINTER() << "_lower = _keys;\n"
		"		" << PTR_CONST() << WIDE_ALPH_TYPE() << PTR_CONST_END() << POINTER() << "_mid;\n"
		"		" << PTR_CONST() << WIDE_ALPH_TYPE() << PTR_CONST_END() << POINTER() << "_upper = _keys + _klen - 1;\n"
		"		while (1) {\n"
		"			if ( _upper < _lower )\n"
		"				break;\n"
		"\n"
		"			_mid = _lower + ((_upper-_lower) >> 1);\n"
		"			if ( " << GET_WIDE_KEY() << " < *_mid )\n"
		"				_upper = _mid - 1;\n"
		"			else if ( " << GET_WIDE_KEY() << " > *_mid )\n"
		"				_lower = _mid + 1;\n"
		"			else {\n"
		"				_trans += " << CAST(UINT()) << "(_mid - _keys);\n"
		"				goto _match;\n"
		"			}\n"
		"		}\n"
		"		_keys += _klen;\n"
		"		_trans += _klen;\n"
		"	}\n"
		"\n"
		"	_klen = " << RL() << "[" << vCS() << "];\n"
		"	if ( _klen > 0 ) {\n"
		"		" << PTR_CONST() << WIDE_ALPH_TYPE() << PTR_CONST_END() << POINTER() << "_lower = _keys;\n"
		"		" << PTR_CONST() << WIDE_ALPH_TYPE() << PTR_CONST_END() << POINTER() << "_mid;\n"
		"		" << PTR_CONST() << WIDE_ALPH_TYPE() << PTR_CONST_END() << POINTER() << "_upper = _keys + (_klen<<1) - 2;\n"
		"		while (1) {\n"
		"			if ( _upper < _lower )\n"
		"				break;\n"
		"\n"
		"			_mid = _lower + (((_upper-_lower) >> 1) & ~1);\n"
		"			if ( " << GET_WIDE_KEY() << " < _mid[0] )\n"
		"				_upper = _mid - 2;\n"
		"			else if ( " << GET_WIDE_KEY() << " > _mid[1] )\n"
		"				_lower = _mid + 2;\n"
		"			else {\n"
		"				_trans += " << CAST(UINT()) << "((_mid - _keys)>>1);\n"
		"				goto _match;\n"
		"			}\n"
		"		}\n"
		"		_trans += _klen;\n"
		"	}\n"
		"\n";
}

void TabCodeGen::GOTO( ostream &ret, int gotoDest, bool inFinish )
{
	ret << "{" << vCS() << " = " << gotoDest << "; " << 
			CTRL_FLOW() << "goto _again;}";
}

void TabCodeGen::GOTO_EXPR( ostream &ret, GenInlineItem *ilItem, bool inFinish )
{
	ret << "{" << vCS() << " = (";
	INLINE_LIST( ret, ilItem->children, 0, inFinish, false );
	ret << "); " << CTRL_FLOW() << "goto _again;}";
}

void TabCodeGen::CURS( ostream &ret, bool inFinish )
{
	ret << "(_ps)";
}

void TabCodeGen::TARGS( ostream &ret, bool inFinish, int targState )
{
	ret << "(" << vCS() << ")";
}

void TabCodeGen::NEXT( ostream &ret, int nextDest, bool inFinish )
{
	ret << vCS() << " = " << nextDest << ";";
}

void TabCodeGen::NEXT_EXPR( ostream &ret, GenInlineItem *ilItem, bool inFinish )
{
	ret << vCS() << " = (";
	INLINE_LIST( ret, ilItem->children, 0, inFinish, false );
	ret << ");";
}

void TabCodeGen::CALL( ostream &ret, int callDest, int targState, bool inFinish )
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

void TabCodeGen::CALL_EXPR( ostream &ret, GenInlineItem *ilItem, int targState, bool inFinish )
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

void TabCodeGen::RET( ostream &ret, bool inFinish )
{
	ret << "{" << vCS() << " = " << STACK() << "[--" << 
			TOP() << "]; ";

	if ( postPopExpr != 0 ) {
		ret << "{";
		INLINE_LIST( ret, postPopExpr, 0, false, false );
		ret << "}";
	}

	ret << CTRL_FLOW() <<  "goto _again;}";
}

void TabCodeGen::BREAK( ostream &ret, int targState, bool csForced )
{
	outLabelUsed = true;
	ret << "{" << P() << "++; " << CTRL_FLOW() << "goto _out; }";
}

void TabCodeGen::writeData()
{
	/* If there are any transtion functions then output the array. If there
	 * are none, don't bother emitting an empty array that won't be used. */
	if ( redFsm->anyActions() )
		ACTIONS_ARRAY();

	if ( redFsm->anyConditions() ) {
		COND_OFFSETS();
		COND_LENS();
		COND_KEYS();
		COND_SPACES();
	}

	KEY_OFFSETS();

	KEYS();

	SINGLE_LENS();

	RANGE_LENS();

	INDEX_OFFSETS();

	if ( useIndicies ) {
		INDICIES();

		TRANS_TARGS_WI();

		if ( redFsm->anyActions() )
			TRANS_ACTIONS_WI();
	}
	else {
		TRANS_TARGS();

		if ( redFsm->anyActions() )
			TRANS_ACTIONS();
	}

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

void TabCodeGen::COND_TRANSLATE()
{
	out << 
		"	_widec = " << GET_KEY() << ";\n"
		"	_klen = " << CL() << "[" << vCS() << "];\n"
		"	_keys = " << ARR_OFF( CK(), "(" + CO() + "[" + vCS() + "]*2)" ) << ";\n"
		"	if ( _klen > 0 ) {\n"
		"		" << PTR_CONST() << WIDE_ALPH_TYPE() << PTR_CONST_END() << POINTER() << "_lower = _keys;\n"
		"		" << PTR_CONST() << WIDE_ALPH_TYPE() << PTR_CONST_END() << POINTER() << "_mid;\n"
		"		" << PTR_CONST() << WIDE_ALPH_TYPE() << PTR_CONST_END() << POINTER() << "_upper = _keys + (_klen<<1) - 2;\n"
		"		while (1) {\n"
		"			if ( _upper < _lower )\n"
		"				break;\n"
		"\n"
		"			_mid = _lower + (((_upper-_lower) >> 1) & ~1);\n"
		"			if ( " << GET_WIDE_KEY() << " < _mid[0] )\n"
		"				_upper = _mid - 2;\n"
		"			else if ( " << GET_WIDE_KEY() << " > _mid[1] )\n"
		"				_lower = _mid + 2;\n"
		"			else {\n"
		"				switch ( " << C() << "[" << CO() << "[" << vCS() << "]"
							" + ((_mid - _keys)>>1)] ) {\n";

	for ( CondSpaceList::Iter csi = condSpaceList; csi.lte(); csi++ ) {
		GenCondSpace *condSpace = csi;
		out << "	case " << condSpace->condSpaceId << ": {\n";
		out << TABS(2) << "_widec = " << CAST(WIDE_ALPH_TYPE()) << "(" <<
				KEY(condSpace->baseKey) << " + (" << GET_KEY() << 
				" - " << KEY(keyOps->minKey) << "));\n";

		for ( GenCondSet::Iter csi = condSpace->condSet; csi.lte(); csi++ ) {
			out << TABS(2) << "if ( ";
			CONDITION( out, *csi );
			Size condValOffset = ((1 << csi.pos()) * keyOps->alphSize());
			out << " ) _widec += " << condValOffset << ";\n";
		}

		out << 
			"		break;\n"
			"	}\n";
	}

	SWITCH_DEFAULT();

	out << 
		"				}\n"
		"				break;\n"
		"			}\n"
		"		}\n"
		"	}\n"
		"\n";
}

void TabCodeGen::writeExec()
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
		"	" << UINT() << " _trans;\n";

	if ( redFsm->anyConditions() )
		out << "	" << WIDE_ALPH_TYPE() << " _widec;\n";

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

	if ( redFsm->anyConditions() )
		COND_TRANSLATE();

	LOCATE_TRANS();

	out << "_match:\n";

	if ( useIndicies )
		out << "	_trans = " << I() << "[_trans];\n";
	
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
			"	while ( _nacts-- > 0 )\n	{\n"
			"		switch ( *_acts++ )\n		{\n";
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
