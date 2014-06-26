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
#include "goto.h"
#include "redfsm.h"
#include "bstmap.h"
#include "gendata.h"

#include <sstream>

using std::ostringstream;

namespace C {

Goto::Goto( const CodeGenArgs &args ) 
:
	CodeGen( args ),
	actions(           "actions",             *this ),
	toStateActions(    "to_state_actions",    *this ),
	fromStateActions(  "from_state_actions",  *this ),
	eofActions(        "eof_actions",         *this )
{}

void Goto::setTableState( TableArray::State state )
{
	for ( ArrayVector::Iter i = arrayVector; i.lte(); i++ ) {
		TableArray *tableArray = *i;
		tableArray->setState( state );
	}
}


/* Emit the goto to take for a given transition. */
std::ostream &Goto::COND_GOTO( RedCondAp *cond, int level )
{
	out << TABS(level) << "goto ctr" << cond->id << ";";
	return out;
}

/* Emit the goto to take for a given transition. */
std::ostream &Goto::TRANS_GOTO( RedTransAp *trans, int level )
{
	if ( trans->condSpace == 0 || trans->condSpace->condSet.length() == 0 ) {
		/* Existing. */
		assert( trans->outConds.length() == 1 );
		RedCondAp *cond = trans->outConds.data[0].value;

		/* Go to the transition which will go to the state. */
		out << TABS(level) << "goto ctr" << cond->id << ";";
	}
	else {
		out << TABS(level) << "int ck = 0;\n";
		for ( GenCondSet::Iter csi = trans->condSpace->condSet; csi.lte(); csi++ ) {
			out << TABS(level) << "if ( ";
			CONDITION( out, *csi );
			Size condValOffset = (1 << csi.pos());
			out << " ) ck += " << condValOffset << ";\n";
		}
		CondKey lower = 0;
		CondKey upper = trans->condFullSize() - 1;
		COND_B_SEARCH( trans, 1, lower, upper, 0, trans->outConds.length()-1 );

		if ( trans->errCond != 0 ) {
			COND_GOTO( trans->errCond, level+1 ) << "\n";
		}
	}

	return out;
}

std::ostream &Goto::TO_STATE_ACTION_SWITCH()
{
	/* Walk the list of functions, printing the cases. */
	for ( GenActionList::Iter act = actionList; act.lte(); act++ ) {
		/* Write out referenced actions. */
		if ( act->numToStateRefs > 0 ) {
			/* Write the case label, the action and the case break. */
			out << "\t case " << act->actionId << ":\n";
			ACTION( out, act, 0, false, false );
			out << "\n\tbreak;\n";
		}
	}

	return out;
}

std::ostream &Goto::FROM_STATE_ACTION_SWITCH()
{
	/* Walk the list of functions, printing the cases. */
	for ( GenActionList::Iter act = actionList; act.lte(); act++ ) {
		/* Write out referenced actions. */
		if ( act->numFromStateRefs > 0 ) {
			/* Write the case label, the action and the case break. */
			out << "\t case " << act->actionId << ":\n";
			ACTION( out, act, 0, false, false );
			out << "\n\tbreak;\n";
		}
	}

	return out;
}

std::ostream &Goto::EOF_ACTION_SWITCH()
{
	/* Walk the list of functions, printing the cases. */
	for ( GenActionList::Iter act = actionList; act.lte(); act++ ) {
		/* Write out referenced actions. */
		if ( act->numEofRefs > 0 ) {
			/* Write the case label, the action and the case break. */
			out << "\t case " << act->actionId << ":\n";
			ACTION( out, act, 0, true, false );
			out << "\n\tbreak;\n";
		}
	}

	return out;
}

std::ostream &Goto::ACTION_SWITCH()
{
	/* Walk the list of functions, printing the cases. */
	for ( GenActionList::Iter act = actionList; act.lte(); act++ ) {
		/* Write out referenced actions. */
		if ( act->numTransRefs > 0 ) {
			/* Write the case label, the action and the case break. */
			out << "\t case " << act->actionId << ":\n";
			ACTION( out, act, 0, false, false );
			out << "\n\tbreak;\n";
		}
	}

	return out;
}

/* Write out the array of actions. */
void Goto::taActions()
{
	actions.start();

	actions.value( 0 );
	
	for ( GenActionTableMap::Iter act = redFsm->actionMap; act.lte(); act++ ) {
		/* Write out the length, which will never be the last character. */
		actions.value( act->key.length() );

		for ( GenActionTable::Iter item = act->key; item.lte(); item++ )
			actions.value( item->value->actionId );
	}

	actions.finish();
}


void Goto::GOTO_HEADER( RedStateAp *state )
{
	/* Label the state. */
	out << "case " << state->id << ":\n";
}


void Goto::SINGLE_SWITCH( RedStateAp *state )
{
	/* Load up the singles. */
	int numSingles = state->outSingle.length();
	RedTransEl *data = state->outSingle.data;

	if ( numSingles == 1 ) {
		/* If there is a single single key then write it out as an if. */
		out << "\tif ( " << GET_KEY() << " == " << 
				KEY(data[0].lowKey) << " ) {\n\t\t"; 

		/* Virtual function for writing the target of the transition. */
		TRANS_GOTO(data[0].value, 0) << "\n";
		out << "\t}\n";
	}
	else if ( numSingles > 1 ) {
		/* Write out single keys in a switch if there is more than one. */
		out << "\tswitch( " << GET_KEY() << " ) {\n";

		/* Write out the single indicies. */
		for ( int j = 0; j < numSingles; j++ ) {
			out << "\t\t case " << KEY(data[j].lowKey) << ": {\n";
			TRANS_GOTO(data[j].value, 0) << "\n";
			out << "\t}\n";
		}
		
		/* Close off the transition switch. */
		out << "\t}\n";
	}
}

void Goto::RANGE_B_SEARCH( RedStateAp *state, int level, Key lower, Key upper, int low, int high )
{
	/* Get the mid position, staying on the lower end of the range. */
	int mid = (low + high) >> 1;
	RedTransEl *data = state->outRange.data;

	/* Determine if we need to look higher or lower. */
	bool anyLower = mid > low;
	bool anyHigher = mid < high;

	/* Determine if the keys at mid are the limits of the alphabet. */
	bool limitLow = keyOps->eq( data[mid].lowKey, lower );
	bool limitHigh = keyOps->eq( data[mid].highKey, upper );

	if ( anyLower && anyHigher ) {
		/* Can go lower and higher than mid. */
		out << TABS(level) << "if ( " << GET_KEY() << " < " << 
				KEY(data[mid].lowKey) << " ) {\n";
		RANGE_B_SEARCH( state, level+1, lower, keyOps->sub( data[mid].lowKey, 1 ), low, mid-1 );
		out << TABS(level) << "} else if ( " << GET_KEY() << " > " << 
				KEY(data[mid].highKey) << " ) {\n";
		RANGE_B_SEARCH( state, level+1, keyOps->add( data[mid].highKey, 1 ), upper, mid+1, high );
		out << TABS(level) << "} else {\n";
		TRANS_GOTO(data[mid].value, level+1) << "\n";
		out << TABS(level) << "}\n";
	}
	else if ( anyLower && !anyHigher ) {
		/* Can go lower than mid but not higher. */
		out << TABS(level) << "if ( " << GET_KEY() << " < " << 
				KEY(data[mid].lowKey) << " ) {\n";
		RANGE_B_SEARCH( state, level+1, lower, keyOps->sub( data[mid].lowKey, 1 ), low, mid-1 );

		/* if the higher is the highest in the alphabet then there is no
		 * sense testing it. */
		if ( limitHigh ) {
			out << TABS(level) << "} else {\n";
			TRANS_GOTO(data[mid].value, level+1) << "\n";
			out << TABS(level) << "}\n";
		}
		else {
			out << TABS(level) << "} else if ( " << GET_KEY() << " <= " << 
					KEY(data[mid].highKey) << " ) {\n";
			TRANS_GOTO(data[mid].value, level+1) << "\n";
			out << TABS(level) << "}\n";
		}
	}
	else if ( !anyLower && anyHigher ) {
		/* Can go higher than mid but not lower. */
		out << TABS(level) << "if ( " << GET_KEY() << " > " << 
				KEY(data[mid].highKey) << " ) {\n";
		RANGE_B_SEARCH( state, level+1, keyOps->add( data[mid].highKey, 1 ), upper, mid+1, high );

		/* If the lower end is the lowest in the alphabet then there is no
		 * sense testing it. */
		if ( limitLow ) {
			out << TABS(level) << "} else {\n";
			TRANS_GOTO(data[mid].value, level+1) << "\n";
			out << TABS(level) << "}\n";
		}
		else {
			out << TABS(level) << "} else if ( " << GET_KEY() << " >= " << 
					KEY(data[mid].lowKey) << " ) {\n";
			TRANS_GOTO(data[mid].value, level+1) << "\n";
			out << TABS(level) << "}\n";
		}
	}
	else {
		/* Cannot go higher or lower than mid. It's mid or bust. What
		 * tests to do depends on limits of alphabet. */
		if ( !limitLow && !limitHigh ) {
			out << TABS(level) << "if ( " << KEY(data[mid].lowKey) << " <= " << 
					GET_KEY() << " && " << GET_KEY() << " <= " << 
					KEY(data[mid].highKey) << " ) {\n";
			TRANS_GOTO(data[mid].value, level+1) << "\n";
			out << TABS(level) << "}\n";
		}
		else if ( limitLow && !limitHigh ) {
			out << TABS(level) << "if ( " << GET_KEY() << " <= " << 
					KEY(data[mid].highKey) << " ) {\n";
			TRANS_GOTO(data[mid].value, level+1) << "\n";
			out << TABS(level) << "}\n";
		}
		else if ( !limitLow && limitHigh ) {
			out << TABS(level) << "if ( " << KEY(data[mid].lowKey) << " <= " << 
					GET_KEY() << " ) {\n";
			TRANS_GOTO(data[mid].value, level+1) << "\n";
			out << TABS(level) << "}\n";
		}
		else {
			/* Both high and low are at the limit. No tests to do. */
			out << TABS(level) << "{\n";
			TRANS_GOTO(data[mid].value, level+1) << "\n";
			out << TABS(level) << "}\n";
		}
	}
}

/* Write out a key from the fsm code gen. Depends on wether or not the key is
 * signed. */
string Goto::CKEY( CondKey key )
{
	ostringstream ret;
	ret << key.getVal();
	return ret.str();
}

void Goto::COND_B_SEARCH( RedTransAp *trans, int level, CondKey lower, CondKey upper, int low, int high )
{
	/* Get the mid position, staying on the lower end of the range. */
	int mid = (low + high) >> 1;
	RedCondEl *data = trans->outConds.data;

	/* Determine if we need to look higher or lower. */
	bool anyLower = mid > low;
	bool anyHigher = mid < high;

	/* Determine if the keys at mid are the limits of the alphabet. */
	bool limitLow = data[mid].key == lower;
	bool limitHigh = data[mid].key == upper;

	if ( anyLower && anyHigher ) {
		/* Can go lower and higher than mid. */
		out << TABS(level) << "if ( " << "ck" << " < " << 
				CKEY(data[mid].key) << " ) {\n";
		COND_B_SEARCH( trans, level+1, lower, data[mid].key-1, low, mid-1 );
		out << TABS(level) << "} else if ( " << "ck" << " > " << 
				CKEY(data[mid].key) << " ) {\n";
		COND_B_SEARCH( trans, level+1, data[mid].key+1, upper, mid+1, high );
		out << TABS(level) << "} else {\n";
		COND_GOTO(data[mid].value, level+1) << "\n";
		out << TABS(level) << "}\n";
	}
	else if ( anyLower && !anyHigher ) {
		/* Can go lower than mid but not higher. */
		out << TABS(level) << "if ( " << "ck" << " < " << 
				CKEY(data[mid].key) << " ) {\n";
		COND_B_SEARCH( trans, level+1, lower, data[mid].key-1, low, mid-1);

		/* if the higher is the highest in the alphabet then there is no
		 * sense testing it. */
		if ( limitHigh ) {
			out << TABS(level) << "} else {\n";
			COND_GOTO(data[mid].value, level+1) << "\n";
			out << TABS(level) << "}\n";
		}
		else {
			out << TABS(level) << "} else if ( " << "ck" << " <= " << 
					CKEY(data[mid].key) << " ) {\n";
			COND_GOTO(data[mid].value, level+1) << "\n";
			out << TABS(level) << "}\n";
		}
	}
	else if ( !anyLower && anyHigher ) {
		/* Can go higher than mid but not lower. */
		out << TABS(level) << "if ( " << "ck" << " > " << 
				CKEY(data[mid].key) << " ) {\n";
		COND_B_SEARCH( trans, level+1, data[mid].key+1, upper, mid+1, high );

		/* If the lower end is the lowest in the alphabet then there is no
		 * sense testing it. */
		if ( limitLow ) {
			out << TABS(level) << "} else {\n";
			COND_GOTO(data[mid].value, level+1) << "\n";
			out << TABS(level) << "}\n";
		}
		else {
			out << TABS(level) << "} else if ( " << "ck" << " >= " << 
					CKEY(data[mid].key) << " ) {\n";
			COND_GOTO(data[mid].value, level+1) << "\n";
			out << TABS(level) << "}\n";
		}
	}
	else {
		/* Cannot go higher or lower than mid. It's mid or bust. What
		 * tests to do depends on limits of alphabet. */
		if ( !limitLow && !limitHigh ) {
			out << TABS(level) << "if ( ck" << " == " << 
					CKEY(data[mid].key) << " ) {\n";
			COND_GOTO(data[mid].value, level+1) << "\n";
			out << TABS(level) << "}\n";
		}
		else if ( limitLow && !limitHigh ) {
			out << TABS(level) << "if ( " << "ck" << " <= " << 
					CKEY(data[mid].key) << " ) {\n";
			COND_GOTO(data[mid].value, level+1) << "\n";
			out << TABS(level) << "}\n";
		}
		else if ( !limitLow && limitHigh ) {
			out << TABS(level) << "if ( " << CKEY(data[mid].key) << " <= " << 
					"ck" << " )\n {";
			COND_GOTO(data[mid].value, level+1) << "\n";
			out << TABS(level) << "}\n";
		}
		else {
			/* Both high and low are at the limit. No tests to do. */
			COND_GOTO(data[mid].value, level) << "\n";
		}
	}
}

void Goto::STATE_GOTO_ERROR()
{
	/* Label the state and bail immediately. */
	outLabelUsed = true;
	RedStateAp *state = redFsm->errState;
	out << "case " << state->id << ":\n";
	out << "	goto _out;\n";
}

std::ostream &Goto::STATE_GOTOS()
{
	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		if ( st == redFsm->errState )
			STATE_GOTO_ERROR();
		else {
			/* Writing code above state gotos. */
			GOTO_HEADER( st );

			/* Try singles. */
			if ( st->outSingle.length() > 0 )
				SINGLE_SWITCH( st );

			/* Default case is to binary search for the ranges, if that fails then */
			if ( st->outRange.length() > 0 ) {
				RANGE_B_SEARCH( st, 1, keyOps->minKey, keyOps->maxKey,
						0, st->outRange.length() - 1 );
			}

			/* Write the default transition. */

			out << "{\n";
			TRANS_GOTO( st->defTrans, 1 ) << "\n";
			out << "}\n";
		}
	}
	return out;
}

std::ostream &Goto::TRANSITIONS()
{
	for ( CondApSet::Iter cond = redFsm->condSet; cond.lte(); cond++ ) {
		/* Write the label for the transition so it can be jumped to. */
		out << "	ctr" << cond->id << ": ";

		/* Destination state. */
		if ( cond->action != 0 && cond->action->anyCurStateRef() )
			out << "_ps = " << vCS() << ";";
		out << vCS() << " = " << cond->targ->id << "; ";

		if ( cond->action != 0 ) {
			/* Write out the transition func. */
			out << "goto f" << cond->action->actListId << ";\n";
		}
		else {
			/* No code to execute, just loop around. */
			out << "goto _again;\n";
		}
	}
	return out;
}

std::ostream &Goto::EXEC_FUNCS()
{
	/* Make labels that set acts and jump to execFuncs. Loop func indicies. */
	for ( GenActionTableMap::Iter redAct = redFsm->actionMap; redAct.lte(); redAct++ ) {
		if ( redAct->numTransRefs > 0 ) {
			out << "	f" << redAct->actListId << ": " <<
				"_acts = offset( " << ARR_REF( actions ) << ", " << itoa( redAct->location+1 ) << ");"
				" goto execFuncs;\n";
		}
	}

	out <<
		"\n"
		"execFuncs:\n"
		"	_nacts = (uint)deref( " << ARR_REF( actions ) << ", _acts );\n"
		"	_acts += 1;\n"
		"	while ( _nacts > 0 ) {\n"
		"		switch ( deref( " << ARR_REF( actions ) << ", _acts ) ) {\n";
		ACTION_SWITCH() << 
		"		}\n"
		"		_acts += 1;\n"
		"		_nacts -= 1;\n"
		"	}\n"
		"	goto _again;\n";
	return out;
}

unsigned int Goto::TO_STATE_ACTION( RedStateAp *state )
{
	int act = 0;
	if ( state->toStateAction != 0 )
		act = state->toStateAction->location+1;
	return act;
}

unsigned int Goto::FROM_STATE_ACTION( RedStateAp *state )
{
	int act = 0;
	if ( state->fromStateAction != 0 )
		act = state->fromStateAction->location+1;
	return act;
}

unsigned int Goto::EOF_ACTION( RedStateAp *state )
{
	int act = 0;
	if ( state->eofAction != 0 )
		act = state->eofAction->location+1;
	return act;
}

void Goto::taToStateActions()
{
	toStateActions.start();

	/* Take one off for the psuedo start state. */
	int numStates = redFsm->stateList.length();
	unsigned int *vals = new unsigned int[numStates];
	memset( vals, 0, sizeof(unsigned int)*numStates );

	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ )
		vals[st->id] = TO_STATE_ACTION(st);

	for ( int st = 0; st < redFsm->nextStateId; st++ ) {
		/* Write any eof action. */
		toStateActions.value( vals[st] );
	}
	delete[] vals;

	toStateActions.finish();
}

void Goto::taFromStateActions()
{
	fromStateActions.start();

	/* Take one off for the psuedo start state. */
	int numStates = redFsm->stateList.length();
	unsigned int *vals = new unsigned int[numStates];
	memset( vals, 0, sizeof(unsigned int)*numStates );

	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ )
		vals[st->id] = FROM_STATE_ACTION(st);

	for ( int st = 0; st < redFsm->nextStateId; st++ ) {
		/* Write any eof action. */
		fromStateActions.value( vals[st] );
	}
	delete[] vals;

	fromStateActions.finish();
}

void Goto::taEofActions()
{
	eofActions.start();

	/* Take one off for the psuedo start state. */
	int numStates = redFsm->stateList.length();
	unsigned int *vals = new unsigned int[numStates];
	memset( vals, 0, sizeof(unsigned int)*numStates );

	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ )
		vals[st->id] = EOF_ACTION(st);

	for ( int st = 0; st < redFsm->nextStateId; st++ ) {
		/* Write any eof action. */
		eofActions.value( vals[st] );
	}
	delete[] vals;

	eofActions.finish();
}

std::ostream &Goto::FINISH_CASES()
{
	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		/* States that are final and have an out action need a case. */
		if ( st->eofAction != 0 ) {
			/* Write the case label. */
			out << "\t\t case " << st->id << ": ";

			/* Write the goto func. */
			out << "goto f" << st->eofAction->actListId << ";\n";
		}
	}
	
	return out;
}

void Goto::GOTO( ostream &ret, int gotoDest, bool inFinish )
{
	ret << "{" << vCS() << " = " << gotoDest << "; " << "goto _again;}";
}

void Goto::GOTO_EXPR( ostream &ret, GenInlineItem *ilItem, bool inFinish )
{
	ret << "{" << vCS() << " = (";
	INLINE_LIST( ret, ilItem->children, 0, inFinish, false );
	ret << "); " << "goto _again;}";
}

void Goto::CURS( ostream &ret, bool inFinish )
{
	ret << "(_ps)";
}

void Goto::TARGS( ostream &ret, bool inFinish, int targState )
{
	ret << "(" << vCS() << ")";
}

void Goto::NEXT( ostream &ret, int nextDest, bool inFinish )
{
	ret << vCS() << " = " << nextDest << ";";
}

void Goto::NEXT_EXPR( ostream &ret, GenInlineItem *ilItem, bool inFinish )
{
	ret << vCS() << " = (";
	INLINE_LIST( ret, ilItem->children, 0, inFinish, false );
	ret << ");";
}

void Goto::CALL( ostream &ret, int callDest, int targState, bool inFinish )
{
	if ( prePushExpr != 0 ) {
		ret << "{";
		INLINE_LIST( ret, prePushExpr, 0, false, false );
	}

	ret << "{" << STACK() << "[" << TOP() << "] = " << vCS() << "; " << TOP() << " += 1;" << vCS() << " = " << 
			callDest << "; " << "goto _again;}";

	if ( prePushExpr != 0 )
		ret << "}";
}

void Goto::CALL_EXPR( ostream &ret, GenInlineItem *ilItem, int targState, bool inFinish )
{
	if ( prePushExpr != 0 ) {
		ret << "{";
		INLINE_LIST( ret, prePushExpr, 0, false, false );
	}

	ret << "{" << STACK() << "[" << TOP() << "] = " << vCS() << "; "  << TOP() << " += 1;"<< vCS() << " = (";
	INLINE_LIST( ret, ilItem->children, targState, inFinish, false );
	ret << "); " << "goto _again;}";

	if ( prePushExpr != 0 )
		ret << "}";
}

void Goto::RET( ostream &ret, bool inFinish )
{
	ret << "{" << TOP() << "-= 1;" << vCS() << " = " << STACK() << "[" << TOP() << "];";

	if ( postPopExpr != 0 ) {
		ret << "{";
		INLINE_LIST( ret, postPopExpr, 0, false, false );
		ret << "}";
	}

	ret << "goto _again;}";
}

void Goto::BREAK( ostream &ret, int targState, bool csForced )
{
	outLabelUsed = true;
	ret << "{" << P() << " += 1; " << "goto _out; }";
}

}
