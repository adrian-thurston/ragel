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
#include "flat.h"
#include "redfsm.h"
#include "gendata.h"

namespace C {

std::ostream &Flat::TO_STATE_ACTION( RedStateAp *state )
{
	int act = 0;
	if ( state->toStateAction != 0 )
		act = state->toStateAction->location+1;
	out << act;
	return out;
}

std::ostream &Flat::FROM_STATE_ACTION( RedStateAp *state )
{
	int act = 0;
	if ( state->fromStateAction != 0 )
		act = state->fromStateAction->location+1;
	out << act;
	return out;
}

std::ostream &Flat::EOF_ACTION( RedStateAp *state )
{
	int act = 0;
	if ( state->eofAction != 0 )
		act = state->eofAction->location+1;
	out << act;
	return out;
}

std::ostream &Flat::TRANS_ACTION( RedTransAp *trans )
{
	/* If there are actions, emit them. Otherwise emit zero. */
	int act = 0;
	if ( trans->action != 0 )
		act = trans->action->location+1;
	out << act;
	return out;
}

std::ostream &Flat::TO_STATE_ACTION_SWITCH()
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

std::ostream &Flat::FROM_STATE_ACTION_SWITCH()
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

std::ostream &Flat::EOF_ACTION_SWITCH()
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


std::ostream &Flat::ACTION_SWITCH()
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


std::ostream &Flat::FLAT_INDEX_OFFSET()
{
	out << "\t";
	int totalStateNum = 0, curIndOffset = 0;
	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		/* Write the index offset. */
		out << curIndOffset;
		if ( !st.last() ) {
			out << ", ";
			if ( ++totalStateNum % IALL == 0 )
				out << "\n\t";
		}
		
		/* Move the index offset ahead. */
		if ( st->transList != 0 )
			curIndOffset += keyOps->span( st->lowKey, st->highKey );

		if ( st->defTrans != 0 )
			curIndOffset += 1;
	}
	out << "\n";
	return out;
}

std::ostream &Flat::KEY_SPANS()
{
	out << "\t";
	int totalStateNum = 0;
	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		/* Write singles length. */
		unsigned long long span = 0;
		if ( st->transList != 0 )
			span = keyOps->span( st->lowKey, st->highKey );
		out << span;
		if ( !st.last() ) {
			out << ", ";
			if ( ++totalStateNum % IALL == 0 )
				out << "\n\t";
		}
	}
	out << "\n";
	return out;
}

std::ostream &Flat::TO_STATE_ACTIONS()
{
	out << "\t";
	int totalStateNum = 0;
	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		/* Write any eof action. */
		TO_STATE_ACTION(st);
		if ( !st.last() ) {
			out << ", ";
			if ( ++totalStateNum % IALL == 0 )
				out << "\n\t";
		}
	}
	out << "\n";
	return out;
}

std::ostream &Flat::FROM_STATE_ACTIONS()
{
	out << "\t";
	int totalStateNum = 0;
	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		/* Write any eof action. */
		FROM_STATE_ACTION(st);
		if ( !st.last() ) {
			out << ", ";
			if ( ++totalStateNum % IALL == 0 )
				out << "\n\t";
		}
	}
	out << "\n";
	return out;
}

std::ostream &Flat::EOF_ACTIONS()
{
	out << "\t";
	int totalStateNum = 0;
	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		/* Write any eof action. */
		EOF_ACTION(st);
		if ( !st.last() ) {
			out << ", ";
			if ( ++totalStateNum % IALL == 0 )
				out << "\n\t";
		}
	}
	out << "\n";
	return out;
}

std::ostream &Flat::EOF_TRANS()
{
	out << "\t";
	int totalStateNum = 0;
	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		/* Write any eof action. */

		long trans = 0;
		if ( st->eofTrans != 0 ) {
			assert( st->eofTrans->pos >= 0 );
			trans = st->eofTrans->pos+1;
		}
		out << trans;

		if ( !st.last() ) {
			out << ", ";
			if ( ++totalStateNum % IALL == 0 )
				out << "\n\t";
		}
	}
	out << "\n";
	return out;
}


std::ostream &Flat::COND_KEYS()
{
	out << '\t';
	int totalTrans = 0;
	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		/* Emit just cond low key and cond high key. */
		out << KEY( st->condLowKey ) << ", ";
		out << KEY( st->condHighKey ) << ", ";
		if ( ++totalTrans % IALL == 0 )
			out << "\n\t";
	}

	/* Output one last number so we don't have to figure out when the last
	 * entry is and avoid writing a comma. */
	out << 0 << "\n";
	return out;
}

std::ostream &Flat::COND_KEY_SPANS()
{
	out << "\t";
	int totalStateNum = 0;
	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		/* Write singles length. */
		unsigned long long span = 0;
		if ( st->condList != 0 )
			span = keyOps->span( st->condLowKey, st->condHighKey );
		out << span;
		if ( !st.last() ) {
			out << ", ";
			if ( ++totalStateNum % IALL == 0 )
				out << "\n\t";
		}
	}
	out << "\n";
	return out;
}

std::ostream &Flat::CONDS()
{
	int totalTrans = 0;
	out << '\t';
	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		if ( st->condList != 0 ) {
			/* Walk the singles. */
			unsigned long long span = keyOps->span( st->condLowKey, st->condHighKey );
			for ( unsigned long long pos = 0; pos < span; pos++ ) {
				if ( st->condList[pos] != 0 )
					out << st->condList[pos]->condSpaceId + 1 << ", ";
				else
					out << "0, ";
				if ( ++totalTrans % IALL == 0 )
					out << "\n\t";
			}
		}
	}

	/* Output one last number so we don't have to figure out when the last
	 * entry is and avoid writing a comma. */
	out << 0 << "\n";
	return out;
}

std::ostream &Flat::COND_INDEX_OFFSET()
{
	out << "\t";
	int totalStateNum = 0, curIndOffset = 0;
	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		/* Write the index offset. */
		out << curIndOffset;
		if ( !st.last() ) {
			out << ", ";
			if ( ++totalStateNum % IALL == 0 )
				out << "\n\t";
		}
		
		/* Move the index offset ahead. */
		if ( st->condList != 0 )
			curIndOffset += keyOps->span( st->condLowKey, st->condHighKey );
	}
	out << "\n";
	return out;
}


std::ostream &Flat::KEYS()
{
	out << '\t';
	int totalTrans = 0;
	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		/* Emit just low key and high key. */
		out << KEY( st->lowKey ) << ", ";
		out << KEY( st->highKey ) << ", ";
		if ( ++totalTrans % IALL == 0 )
			out << "\n\t";
	}

	/* Output one last number so we don't have to figure out when the last
	 * entry is and avoid writing a comma. */
	out << 0 << "\n";
	return out;
}

std::ostream &Flat::INDICIES()
{
	int totalTrans = 0;
	out << '\t';
	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		if ( st->transList != 0 ) {
			/* Walk the singles. */
			unsigned long long span = keyOps->span( st->lowKey, st->highKey );
			for ( unsigned long long pos = 0; pos < span; pos++ ) {
				out << st->transList[pos]->id << ", ";
				if ( ++totalTrans % IALL == 0 )
					out << "\n\t";
			}
		}

		/* The state's default index goes next. */
		if ( st->defTrans != 0 )
			out << st->defTrans->id << ", ";

		if ( ++totalTrans % IALL == 0 )
			out << "\n\t";
	}

	/* Output one last number so we don't have to figure out when the last
	 * entry is and avoid writing a comma. */
	out << 0 << "\n";
	return out;
}

std::ostream &Flat::TRANS_TARGS()
{
	/* Transitions must be written ordered by their id. */
	RedTransAp **transPtrs = new RedTransAp*[redFsm->transSet.length()];
	for ( TransApSet::Iter trans = redFsm->transSet; trans.lte(); trans++ )
		transPtrs[trans->id] = trans;

	/* Keep a count of the num of items in the array written. */
	out << '\t';
	int totalStates = 0;
	for ( int t = 0; t < redFsm->transSet.length(); t++ ) {
		/* Save the position. Needed for eofTargs. */
		RedTransAp *trans = transPtrs[t];
		trans->pos = t;

		/* Write out the target state. */
		out << trans->targ->id;
		if ( t < redFsm->transSet.length()-1 ) {
			out << ", ";
			if ( ++totalStates % IALL == 0 )
				out << "\n\t";
		}
	}
	out << "\n";
	delete[] transPtrs;
	return out;
}


std::ostream &Flat::TRANS_ACTIONS()
{
	/* Transitions must be written ordered by their id. */
	RedTransAp **transPtrs = new RedTransAp*[redFsm->transSet.length()];
	for ( TransApSet::Iter trans = redFsm->transSet; trans.lte(); trans++ )
		transPtrs[trans->id] = trans;

	/* Keep a count of the num of items in the array written. */
	out << '\t';
	int totalAct = 0;
	for ( int t = 0; t < redFsm->transSet.length(); t++ ) {
		/* Write the function for the transition. */
		RedTransAp *trans = transPtrs[t];
		TRANS_ACTION( trans );
		if ( t < redFsm->transSet.length()-1 ) {
			out << ", ";
			if ( ++totalAct % IALL == 0 )
				out << "\n\t";
		}
	}
	out << "\n";
	delete[] transPtrs;
	return out;
}

void Flat::LOCATE_TRANS()
{
	out <<
		"	_keys = " << ARR_OFF( K(), "(" + vCS() + "<<1)" ) << ";\n"
		"	_inds = " << ARR_OFF( I(), IO() + "[" + vCS() + "]" ) << ";\n"
		"\n"
		"	_slen = " << SP() << "[" << vCS() << "];\n"
		"	_trans = _inds[ _slen > 0 && _keys[0] <=" << GET_WIDE_KEY() << " &&\n"
		"		" << GET_WIDE_KEY() << " <= _keys[1] ?\n"
		"		" << GET_WIDE_KEY() << " - _keys[0] : _slen ];\n"
		"\n";
}

void Flat::GOTO( ostream &ret, int gotoDest, bool inFinish )
{
	ret << "{" << vCS() << " = " << gotoDest << "; " << 
			CTRL_FLOW() << "goto _again;}";
}

void Flat::GOTO_EXPR( ostream &ret, GenInlineItem *ilItem, bool inFinish )
{
	ret << "{" << vCS() << " = (";
	INLINE_LIST( ret, ilItem->children, 0, inFinish, false );
	ret << "); " << CTRL_FLOW() << "goto _again;}";
}

void Flat::CURS( ostream &ret, bool inFinish )
{
	ret << "(_ps)";
}

void Flat::TARGS( ostream &ret, bool inFinish, int targState )
{
	ret << "(" << vCS() << ")";
}

void Flat::NEXT( ostream &ret, int nextDest, bool inFinish )
{
	ret << vCS() << " = " << nextDest << ";";
}

void Flat::NEXT_EXPR( ostream &ret, GenInlineItem *ilItem, bool inFinish )
{
	ret << vCS() << " = (";
	INLINE_LIST( ret, ilItem->children, 0, inFinish, false );
	ret << ");";
}

void Flat::CALL( ostream &ret, int callDest, int targState, bool inFinish )
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


void Flat::CALL_EXPR( ostream &ret, GenInlineItem *ilItem, int targState, bool inFinish )
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


void Flat::RET( ostream &ret, bool inFinish )
{
	ret << "{" << vCS() << " = " << STACK() << "[--" << TOP() << "];";

	if ( postPopExpr != 0 ) {
		ret << "{";
		INLINE_LIST( ret, postPopExpr, 0, false, false );
		ret << "}";
	}

	ret << CTRL_FLOW() << "goto _again;}";
}

void Flat::BREAK( ostream &ret, int targState, bool csForced )
{
	outLabelUsed = true;
	ret << "{" << P() << "++; " << CTRL_FLOW() << "goto _out; }";
}

void Flat::COND_TRANSLATE()
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

}
