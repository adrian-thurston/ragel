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
#include "cdftable.h"
#include "redfsm.h"
#include "gendata.h"

/* Determine if we should use indicies or not. */
void FTabCodeGen::calcIndexSize()
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
		sizeWithInds += arrayTypeSize(redFsm->maxActListId) * redFsm->transSet.length();

	/* Calculate the cost of not using indicies. */
	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		int totalIndex = st->outSingle.length() + st->outRange.length() + 
				(st->defTrans == 0 ? 0 : 1);
		sizeWithoutInds += arrayTypeSize(redFsm->maxState) * totalIndex;
		if ( redFsm->anyActions() )
			sizeWithoutInds += arrayTypeSize(redFsm->maxActListId) * totalIndex;
	}

	/* If using indicies reduces the size, use them. */
	useIndicies = sizeWithInds < sizeWithoutInds;
}

std::ostream &FTabCodeGen::TO_STATE_ACTION( TableArray &taTSA, RedStateAp *state )
{
	int act = 0;
	if ( state->toStateAction != 0 )
		act = state->toStateAction->actListId+1;
	taTSA.VAL( act );
	return out;
}

std::ostream &FTabCodeGen::FROM_STATE_ACTION( TableArray &taFSA, RedStateAp *state )
{
	int act = 0;
	if ( state->fromStateAction != 0 )
		act = state->fromStateAction->actListId+1;
	taFSA.VAL( act );
	return out;
}

std::ostream &FTabCodeGen::EOF_ACTION( TableArray &taEA, RedStateAp *state )
{
	int act = 0;
	if ( state->eofAction != 0 )
		act = state->eofAction->actListId+1;
	taEA.VAL( act );
	return out;
}


/* Write out the function for a transition. */
std::ostream &FTabCodeGen::TRANS_ACTION( TableArray &taTA, RedTransAp *trans )
{
	int action = 0;
	if ( trans->action != 0 )
		action = trans->action->actListId+1;
	taTA.VAL( action );
	return out;
}

/* Write out the function switch. This switch is keyed on the values
 * of the func index. */
std::ostream &FTabCodeGen::TO_STATE_ACTION_SWITCH()
{
	/* Loop the actions. */
	for ( GenActionTableMap::Iter redAct = redFsm->actionMap; redAct.lte(); redAct++ ) {
		if ( redAct->numToStateRefs > 0 ) {
			/* Write the entry label. */
			out << "\tcase " << redAct->actListId+1 << ":\n";

			/* Write each action in the list of action items. */
			for ( GenActionTable::Iter item = redAct->key; item.lte(); item++ )
				ACTION( out, item->value, 0, false, false );

			out << "\tbreak;\n";
		}
	}

	genLineDirective( out );
	return out;
}

/* Write out the function switch. This switch is keyed on the values
 * of the func index. */
std::ostream &FTabCodeGen::FROM_STATE_ACTION_SWITCH()
{
	/* Loop the actions. */
	for ( GenActionTableMap::Iter redAct = redFsm->actionMap; redAct.lte(); redAct++ ) {
		if ( redAct->numFromStateRefs > 0 ) {
			/* Write the entry label. */
			out << "\tcase " << redAct->actListId+1 << ":\n";

			/* Write each action in the list of action items. */
			for ( GenActionTable::Iter item = redAct->key; item.lte(); item++ )
				ACTION( out, item->value, 0, false, false );

			out << "\tbreak;\n";
		}
	}

	genLineDirective( out );
	return out;
}

std::ostream &FTabCodeGen::EOF_ACTION_SWITCH()
{
	/* Loop the actions. */
	for ( GenActionTableMap::Iter redAct = redFsm->actionMap; redAct.lte(); redAct++ ) {
		if ( redAct->numEofRefs > 0 ) {
			/* Write the entry label. */
			out << "\tcase " << redAct->actListId+1 << ":\n";

			/* Write each action in the list of action items. */
			for ( GenActionTable::Iter item = redAct->key; item.lte(); item++ )
				ACTION( out, item->value, 0, true, false );

			out << "\tbreak;\n";
		}
	}

	genLineDirective( out );
	return out;
}

/* Write out the function switch. This switch is keyed on the values
 * of the func index. */
std::ostream &FTabCodeGen::ACTION_SWITCH()
{
	/* Loop the actions. */
	for ( GenActionTableMap::Iter redAct = redFsm->actionMap; redAct.lte(); redAct++ ) {
		if ( redAct->numTransRefs > 0 ) {
			/* Write the entry label. */
			out << "\tcase " << redAct->actListId+1 << ":\n";

			/* Write each action in the list of action items. */
			for ( GenActionTable::Iter item = redAct->key; item.lte(); item++ )
				ACTION( out, item->value, 0, false, false );

			out << "\tbreak;\n";
		}
	}

	genLineDirective( out );
	return out;
}

std::ostream &FTabCodeGen::TRANS_ACTIONS()
{
	TableArray taTA( *this, arrayType(redFsm->maxActListId), TA() );

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

std::ostream &FTabCodeGen::TRANS_ACTIONS_WI()
{
	TableArray taTA( *this, arrayType(redFsm->maxActListId), TA() );

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


std::ostream &FTabCodeGen::TO_STATE_ACTIONS()
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

std::ostream &FTabCodeGen::FROM_STATE_ACTIONS()
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

std::ostream &FTabCodeGen::EOF_ACTIONS()
{
	TableArray taEA( *this, arrayType(redFsm->maxActListId), EA() );

	taEA.OPEN();

	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		/* Write any eof action. */
		EOF_ACTION( taEA, st );
	}

	taEA.CLOSE();

	return out;
}

void FTabCodeGen::writeData()
{
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

void FTabCodeGen::writeExec()
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
		"	" << PTR_CONST() << WIDE_ALPH_TYPE() << PTR_CONST_END() << POINTER() << "_keys;\n"
		"	int _trans;\n";

	if ( redFsm->anyConditions() )
		out << "	" << WIDE_ALPH_TYPE() << " _widec;\n";

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
			"	switch ( " << FSA() << "[" << vCS() << "] ) {\n";
			FROM_STATE_ACTION_SWITCH();
			SWITCH_DEFAULT() <<
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
			"	switch ( " << TA() << "[_trans] ) {\n";
			ACTION_SWITCH();
			SWITCH_DEFAULT() <<
			"	}\n"
			"\n";
	}

	if ( redFsm->anyRegActions() || redFsm->anyActionGotos() || 
			redFsm->anyActionCalls() || redFsm->anyActionRets() )
		out << "_again:\n";

	if ( redFsm->anyToStateActions() ) {
		out <<
			"	switch ( " << TSA() << "[" << vCS() << "] ) {\n";
			TO_STATE_ACTION_SWITCH();
			SWITCH_DEFAULT() <<
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
				"	switch ( " << EA() << "[" << vCS() << "] ) {\n";
				EOF_ACTION_SWITCH();
				SWITCH_DEFAULT() <<
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
