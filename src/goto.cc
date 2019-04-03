/*
 * Copyright 2001-2018 Adrian Thurston <thurston@colm.net>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "ragel.h"
#include "goto.h"
#include "redfsm.h"
#include "bstmap.h"
#include "gendata.h"

#include <sstream>

using std::ostringstream;

void Goto::setTableState( TableArray::State state )
{
	for ( ArrayVector::Iter i = arrayVector; i.lte(); i++ ) {
		TableArray *tableArray = *i;
		tableArray->setState( state );
	}
}


/* Emit the goto to take for a given transition. */
std::ostream &Goto::COND_GOTO( RedCondPair *cond, int level )
{
	out << TABS(level) << "goto ctr" << cond->id << ";";
	return out;
}

/* Emit the goto to take for a given transition. */
std::ostream &Goto::TRANS_GOTO( RedTransAp *trans, int level )
{
	if ( trans->condSpace == 0 || trans->condSpace->condSet.length() == 0 ) {
		/* Existing. */
		assert( trans->numConds() == 1 );
		RedCondPair *cond = trans->outCond( 0 );

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
		COND_B_SEARCH( trans, 1, lower, upper, 0, trans->numConds()-1 );

		if ( trans->errCond() != 0 ) {
			COND_GOTO( trans->errCond(), level+1 ) << "\n";
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

void Goto::NFA_POP()
{
	if ( redFsm->anyNfaStates() ) {
		out <<
			"	if ( nfa_len > 0 ) {\n";

		if ( redFsm->bAnyNfaCondRefs )
			out << "	int " << cpc << ";\n";

		out <<
			"		nfa_count += 1;\n"
			"		nfa_len -= 1;\n"
			"		" << P() << " = nfa_bp[nfa_len].p;\n"
			;

		if ( redFsm->bAnyNfaPops ) {
			out << 
				"		int _pop_test = 1;\n"
				"		switch ( nfa_bp[nfa_len].popTrans ) {\n";

			/* Loop the actions. */
			for ( GenActionTableMap::Iter redAct = redFsm->actionMap;
					redAct.lte(); redAct++ )
			{
				if ( redAct->numNfaPopTestRefs > 0 ) {
					/* Write the entry label. */
					out << "\t " << CASE( STR( redAct->actListId+1 ) ) << " {\n";

					/* Write each action in the list of action items. */
					for ( GenActionTable::Iter item = redAct->key; item.lte(); item++ )
						NFA_CONDITION( out, item->value, item.last() );

					out << "\n\t" << CEND() << "}\n";
				}
			}

			out <<
				"		}\n";

			out <<
				"		if ( _pop_test ) {\n"
				"			" << vCS() << " = nfa_bp[nfa_len].state;\n";

			NFA_POST_POP();

			out << 
				"	if ( " << P() << " == " << PE() << " )\n"
				"		goto _test_eof;\n";


			out <<
				"			goto _resume;\n"
				"		}\n";

			if ( red->nfaPostPopExpr != 0 ) {
				out <<
					"		else {\n";

				NFA_POST_POP();

				out <<
					"		}\n";
			}
		}
		else {
			out <<
				"		" << vCS() << " = nfa_bp[nfa_len].state;\n";

			NFA_POST_POP();

			out << 
				"	if ( " << P() << " == " << PE() << " )\n"
				"		goto _test_eof;\n";

			out <<
				"		goto _resume;\n";
		}

		outLabelUsed = true;
		out << 
			"		goto _pop;\n"
			"	}\n";
	}
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

void Goto::COND_B_SEARCH( RedTransAp *trans, int level, CondKey lower,
		CondKey upper, int low, int high )
{
	/* Get the mid position, staying on the lower end of the range. */
	int mid = (low + high) >> 1;
//	RedCondEl *data = trans->outCond(0);

	/* Determine if we need to look higher or lower. */
	bool anyLower = mid > low;
	bool anyHigher = mid < high;

	CondKey midKey = trans->outCondKey( mid );
	RedCondPair *midTrans = trans->outCond( mid );

	/* Determine if the keys at mid are the limits of the alphabet. */
	bool limitLow = midKey == lower;
	bool limitHigh = midKey == upper;

	if ( anyLower && anyHigher ) {
		/* Can go lower and higher than mid. */
		out << TABS(level) << "if ( " << "ck" << " < " << 
				CKEY(midKey) << " ) {\n";
		COND_B_SEARCH( trans, level+1, lower, midKey-1, low, mid-1 );
		out << TABS(level) << "} else if ( " << "ck" << " > " << 
				CKEY(midKey) << " ) {\n";
		COND_B_SEARCH( trans, level+1, midKey+1, upper, mid+1, high );
		out << TABS(level) << "} else {\n";
		COND_GOTO(midTrans, level+1) << "\n";
		out << TABS(level) << "}\n";
	}
	else if ( anyLower && !anyHigher ) {
		/* Can go lower than mid but not higher. */
		out << TABS(level) << "if ( " << "ck" << " < " << 
				CKEY(midKey) << " ) {\n";
		COND_B_SEARCH( trans, level+1, lower, midKey-1, low, mid-1);

		/* if the higher is the highest in the alphabet then there is no
		 * sense testing it. */
		if ( limitHigh ) {
			out << TABS(level) << "} else {\n";
			COND_GOTO(midTrans, level+1) << "\n";
			out << TABS(level) << "}\n";
		}
		else {
			out << TABS(level) << "} else if ( " << "ck" << " <= " << 
					CKEY(midKey) << " ) {\n";
			COND_GOTO(midTrans, level+1) << "\n";
			out << TABS(level) << "}\n";
		}
	}
	else if ( !anyLower && anyHigher ) {
		/* Can go higher than mid but not lower. */
		out << TABS(level) << "if ( " << "ck" << " > " << 
				CKEY(midKey) << " ) {\n";
		COND_B_SEARCH( trans, level+1, midKey+1, upper, mid+1, high );

		/* If the lower end is the lowest in the alphabet then there is no
		 * sense testing it. */
		if ( limitLow ) {
			out << TABS(level) << "} else {\n";
			COND_GOTO(midTrans, level+1) << "\n";
			out << TABS(level) << "}\n";
		}
		else {
			out << TABS(level) << "} else if ( " << "ck" << " >= " << 
					CKEY(midKey) << " ) {\n";
			COND_GOTO(midTrans, level+1) << "\n";
			out << TABS(level) << "}\n";
		}
	}
	else {
		/* Cannot go higher or lower than mid. It's mid or bust. What
		 * tests to do depends on limits of alphabet. */
		if ( !limitLow && !limitHigh ) {
			out << TABS(level) << "if ( ck" << " == " << 
					CKEY(midKey) << " ) {\n";
			COND_GOTO(midTrans, level+1) << "\n";
			out << TABS(level) << "}\n";
		}
		else if ( limitLow && !limitHigh ) {
			out << TABS(level) << "if ( " << "ck" << " <= " << 
					CKEY(midKey) << " ) {\n";
			COND_GOTO(midTrans, level+1) << "\n";
			out << TABS(level) << "}\n";
		}
		else if ( !limitLow && limitHigh ) {
			out << TABS(level) << "if ( " << CKEY(midKey) << " <= " << 
					"ck" << " )\n {";
			COND_GOTO(midTrans, level+1) << "\n";
			out << TABS(level) << "}\n";
		}
		else {
			/* Both high and low are at the limit. No tests to do. */
			COND_GOTO(midTrans, level) << "\n";
		}
	}
}

void Goto::STATE_GOTO_ERROR()
{
	/* Label the state and bail immediately. */
	outLabelUsed = true;
	RedStateAp *state = redFsm->errState;
	out << "case " << state->id << ":\n";
	out << "	goto _pop;\n";
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

std::ostream &Goto::TRANSITION( RedCondPair *pair )
{
	/* Write the label for the transition so it can be jumped to. */
	out << "	ctr" << pair->id << ": ";

	/* Destination state. */
	if ( pair->action != 0 && pair->action->anyCurStateRef() )
		out << "_ps = " << vCS() << ";";
	out << vCS() << " = " << pair->targ->id << "; ";

	if ( pair->action != 0 ) {
		/* Write out the transition func. */
		out << "goto f" << pair->action->actListId << ";\n";
	}
	else {
		/* No code to execute, just loop around. */
		out << "goto _again;\n";
	}
	return out;
}

std::ostream &Goto::TRANSITIONS()
{
	for ( TransApSet::Iter trans = redFsm->transSet; trans.lte(); trans++ ) {
		if ( trans->condSpace == 0 )
			TRANSITION( &trans->p );
	}

	for ( CondApSet::Iter cond = redFsm->condSet; cond.lte(); cond++ )
		TRANSITION( &cond->p );

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

void Goto::taNfaOffsets()
{
	nfaOffsets.start();

	/* Offset of zero means no NFA targs, real targs start at 1. */
	long offset = 1;

	/* Take one off for the psuedo start state. */
	int numStates = redFsm->stateList.length();
	unsigned int *vals = new unsigned int[numStates];
	memset( vals, 0, sizeof(unsigned int)*numStates );

	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		if ( st->nfaTargs == 0 ) {
			vals[st->id] = 0;
			//nfaOffsets.value( 0 );
		}
		else {
			vals[st->id] = offset;
			//nfaOffsets.value( offset );
			offset += 1 + st->nfaTargs->length();
		}
	}

	for ( int st = 0; st < redFsm->nextStateId; st++ )
		nfaOffsets.value( vals[st] );
	delete[] vals;

	nfaOffsets.finish();
}

void Goto::taNfaTargs()
{
	nfaTargs.start();

	/* Offset of zero means no NFA targs, put a filler there. */
	nfaTargs.value( 0 );

	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		if ( st->nfaTargs != 0 ) {
			nfaTargs.value( st->nfaTargs->length() );
			for ( RedNfaTargs::Iter targ = *st->nfaTargs; targ.lte(); targ++ )
				nfaTargs.value( targ->state->id );
		}
	}

	nfaTargs.finish();
}

/* These need to mirror nfa targs. */
void Goto::taNfaPushActions()
{
	nfaPushActions.start();

	nfaPushActions.value( 0 );

	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		if ( st->nfaTargs != 0 ) {
			nfaPushActions.value( 0 );
			for ( RedNfaTargs::Iter targ = *st->nfaTargs; targ.lte(); targ++ )
				NFA_PUSH_ACTION( targ );
		}
	}

	nfaPushActions.finish();
}

void Goto::taNfaPopTrans()
{
	nfaPopTrans.start();

	nfaPopTrans.value( 0 );

	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		if ( st->nfaTargs != 0 ) {
			nfaPopTrans.value( 0 );
			for ( RedNfaTargs::Iter targ = *st->nfaTargs; targ.lte(); targ++ )
				NFA_POP_TEST( targ );
		}
	}

	nfaPopTrans.finish();
}


void Goto::GOTO( ostream &ret, int gotoDest, bool inFinish )
{
	ret << OPEN_GEN_BLOCK() << vCS() << " = " << gotoDest << "; ";

	if ( inFinish && !noEnd )
		EOF_CHECK( ret );

	ret << "goto _again;";
	
	ret << CLOSE_GEN_BLOCK();
}

void Goto::GOTO_EXPR( ostream &ret, GenInlineItem *ilItem, bool inFinish )
{
	ret << OPEN_GEN_BLOCK() << vCS() << " = " << OPEN_HOST_EXPR();
	INLINE_LIST( ret, ilItem->children, 0, inFinish, false );
	ret << CLOSE_HOST_EXPR() << ";";

	if ( inFinish && !noEnd )
		EOF_CHECK( ret );
	
	ret << " goto _again;";

	ret << CLOSE_GEN_BLOCK();
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
	ret << OPEN_GEN_BLOCK();

	if ( red->prePushExpr != 0 ) {
		ret << OPEN_HOST_BLOCK( red->prePushExpr );
		INLINE_LIST( ret, red->prePushExpr->inlineList, 0, false, false );
		ret << CLOSE_HOST_BLOCK();
	}

	ret << STACK() << "[" << TOP() << "] = " << vCS() << "; " <<
			TOP() << " += 1;" << vCS() << " = " << 
			callDest << ";";

	if ( inFinish && !noEnd )
		EOF_CHECK( ret );

	ret << " goto _again;";

	ret << CLOSE_GEN_BLOCK();
}

void Goto::NCALL( ostream &ret, int callDest, int targState, bool inFinish )
{
	ret << OPEN_GEN_BLOCK();

	if ( red->prePushExpr != 0 ) {
		ret << OPEN_HOST_BLOCK( red->prePushExpr );
		INLINE_LIST( ret, red->prePushExpr->inlineList, 0, false, false );
		ret << CLOSE_HOST_BLOCK();
	}

	ret << STACK() << "[" << TOP() << "] = " << vCS() << "; " <<
			TOP() << " += 1;" << vCS() << " = " << 
			callDest << "; " << CLOSE_GEN_BLOCK();
}

void Goto::CALL_EXPR( ostream &ret, GenInlineItem *ilItem, int targState, bool inFinish )
{
	ret << OPEN_GEN_BLOCK();

	if ( red->prePushExpr != 0 ) {
		ret << OPEN_HOST_BLOCK( red->prePushExpr );
		INLINE_LIST( ret, red->prePushExpr->inlineList, 0, false, false );
		ret << CLOSE_HOST_BLOCK();
	}

	ret << STACK() << "[" << TOP() << "] = " << vCS() << "; "  << TOP() << " += 1;" <<
			vCS() << " = " << OPEN_HOST_EXPR();
	INLINE_LIST( ret, ilItem->children, targState, inFinish, false );
	ret << CLOSE_HOST_EXPR() << ";";

	if ( inFinish && !noEnd )
		EOF_CHECK( ret );

	ret << " goto _again;";

	ret << CLOSE_GEN_BLOCK();
}

void Goto::NCALL_EXPR( ostream &ret, GenInlineItem *ilItem, int targState, bool inFinish )
{
	ret << OPEN_GEN_BLOCK();

	if ( red->prePushExpr != 0 ) {
		ret << OPEN_HOST_BLOCK( red->prePushExpr );
		INLINE_LIST( ret, red->prePushExpr->inlineList, 0, false, false );
		ret << CLOSE_HOST_BLOCK();
	}

	ret << STACK() << "[" << TOP() << "] = " << vCS() << "; "  << TOP() << " += 1;" <<
			vCS() << " = " << OPEN_HOST_EXPR();
	INLINE_LIST( ret, ilItem->children, targState, inFinish, false );
	ret << CLOSE_HOST_EXPR() << "; " << CLOSE_GEN_BLOCK();
}

void Goto::RET( ostream &ret, bool inFinish )
{
	ret << OPEN_GEN_BLOCK() << TOP() << "-= 1;" << vCS() << " = " << STACK() << "[" << TOP() << "];";

	if ( red->postPopExpr != 0 ) {
		ret << OPEN_HOST_BLOCK( red->postPopExpr );
		INLINE_LIST( ret, red->postPopExpr->inlineList, 0, false, false );
		ret << CLOSE_HOST_BLOCK();
	}

	if ( inFinish && !noEnd )
		EOF_CHECK( ret );

	ret << "goto _again;" << CLOSE_GEN_BLOCK();
}

void Goto::NRET( ostream &ret, bool inFinish )
{
	ret << OPEN_GEN_BLOCK() << TOP() << "-= 1;" << vCS() << " = " << STACK() << "[" << TOP() << "];";

	if ( red->postPopExpr != 0 ) {
		ret << OPEN_HOST_BLOCK( red->postPopExpr );
		INLINE_LIST( ret, red->postPopExpr->inlineList, 0, false, false );
		ret << CLOSE_HOST_BLOCK();
	}

	ret << CLOSE_GEN_BLOCK();
}

void Goto::BREAK( ostream &ret, int targState, bool csForced )
{
	outLabelUsed = true;
	ret << OPEN_GEN_BLOCK() << P() << " += 1; " << "goto _pop; " << CLOSE_GEN_BLOCK();
}

void Goto::NBREAK( ostream &ret, int targState, bool csForced )
{
	outLabelUsed = true;
	ret << OPEN_GEN_BLOCK() << P() << " += 1; " << " _nbreak = 1; " << CLOSE_GEN_BLOCK();
}

void Goto::tableDataPass()
{
	if ( type == Loop )
		taActions();

	taToStateActions();
	taFromStateActions();
	taEofActions();

	taNfaTargs();
	taNfaOffsets();
	taNfaPushActions();
	taNfaPopTrans();
}

void Goto::genAnalysis()
{
	/* For directly executable machines there is no required state
	 * ordering. Choose a depth-first ordering to increase the
	 * potential for fall-throughs. */
	redFsm->depthFirstOrdering();

	/* Choose default transitions and the single transition. */
	redFsm->chooseDefaultSpan();
		
	/* Choose single. */
	redFsm->moveSelectTransToSingle();

	/* If any errors have occured in the input file then don't write anything. */
	if ( red->id->errorCount > 0 )
		return;

	/* Anlayze Machine will find the final action reference counts, among other
	 * things. We will use these in reporting the usage of fsm directives in
	 * action code. */
	red->analyzeMachine();

	/* Run the analysis pass over the table data. */
	setTableState( TableArray::AnalyzePass );
	tableDataPass();

	/* Switch the tables over to the code gen mode. */
	setTableState( TableArray::GeneratePass );
}

void Goto::writeData()
{
	if ( type == Loop ) {
		if ( redFsm->anyActions() )
			taActions();
	}

	if ( redFsm->anyToStateActions() )
		taToStateActions();

	if ( redFsm->anyFromStateActions() )
		taFromStateActions();

	if ( redFsm->anyEofActions() )
		taEofActions();

	taNfaTargs();
	taNfaOffsets();
	taNfaPushActions();
	taNfaPopTrans();

	STATE_IDS();
}

void Goto::writeExec()
{
	testEofUsed = false;
	outLabelUsed = false;

	out << "	{\n";

	if ( type == Loop ) {
		if ( redFsm->anyToStateActions() || redFsm->anyRegActions() 
				|| redFsm->anyFromStateActions() )
		{
			out << 
				"	" << INDEX( ARR_TYPE( actions ), "" + string(acts) + "" ) << ";\n"
				"	" << UINT() << " " << nacts << ";\n";
		}
	}

	if ( redFsm->anyRegCurStateRef() )
		out << "	int _ps = 0;\n";

	out << "\n";

	if ( redFsm->anyRegNbreak() ) {
		out << "	int _nbreak;\n";
	}

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
			"		goto _pop;\n";
	}

	out << "_resume:\n";

	FROM_STATE_ACTIONS();

	NFA_PUSH( vCS() ); 
	out <<
		"	switch ( " << vCS() << " ) {\n";
		STATE_GOTOS() <<
		"	}\n"
		"\n";
		TRANSITIONS() <<
		"\n";

	if ( redFsm->anyRegActions() )
		EXEC_FUNCS() << "\n";

	out << "_again:\n";

	TO_STATE_ACTIONS();

	if ( redFsm->errState != 0 ) {
		outLabelUsed = true;
		out << 
			"	if ( " << vCS() << " == " << redFsm->errState->id << " )\n"
			"		goto _pop;\n";
	}

	if ( !noEnd ) {
		out << 
			"	" << P() << " += 1;\n"
			"	if ( " << P() << " != " << PE() << " )\n"
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

		NFA_PUSH( vCS() );

		out <<
			"	switch ( " << vCS() << " ) {\n";

		bool okeydokey = false;
		for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
			if ( st->outCondSpace != 0 ) {
				out << "	case " << st->id << ": {\n";
				out << "int ck = 0;\n";
				for ( GenCondSet::Iter csi = st->outCondSpace->condSet; csi.lte(); csi++ ) {
					out << "if ( ";
					CONDITION( out, *csi );
					Size condValOffset = (1 << csi.pos());
					out << " ) ck += " << condValOffset << ";\n";
				}

				out << "	switch ( ck ) {\n";
				for ( CondKeySet::Iter k = st->outCondKeys; k.lte(); k++ ) {
					out << "case " << *k << ": goto _okeydokey;\n";
					okeydokey = true;
				}
				out << "}\n";
				out << vCS() << " = " << ERROR_STATE() << ";\n";
				out << "goto _pop;\n";
				outLabelUsed = true;

				out << "}\n";
			}

		}

		out <<
			"	}\n";

		if ( okeydokey ) {
			out <<
				"_okeydokey: {}\n";
		}

		EOF_ACTIONS();

		if ( redFsm->anyEofTrans() ) {
			out <<
				"	switch ( " << vCS() << " ) {\n";

			for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
				if ( st->eofTrans != 0 ) {
					RedCondPair *cond = st->eofTrans->outCond( 0 );
					out << "	case " << st->id << ": goto ctr" << cond->id << ";\n";
				}
			}

			out <<
				"	}\n";
		}


		out <<
			"	}\n"
			"\n";

	}

	out <<
		"	if ( " << vCS() << " >= " << FIRST_FINAL_STATE() << " )\n"
		"		goto _out; ";

	if ( outLabelUsed )
		out << "	_pop: {}\n";

	NFA_POP();

	out << "_out: {}\n";

	out << "	}\n";
}
