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
#include "ipgoto.h"
#include "redfsm.h"
#include "gendata.h"
#include "bstmap.h"

#include <sstream>

using std::ostringstream;

void IpGoto::tableDataPass()
{
	taNfaTargs();
	taNfaOffsets();
	taNfaPushActions();
	taNfaPopTrans();
}

void IpGoto::genAnalysis()
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
	if ( gblErrorCount > 0 )
		return;
	
	redFsm->setInTrans();

	/* Anlayze Machine will find the final action reference counts, among other
	 * things. We will use these in reporting the usage of fsm directives in
	 * action code. */
	analyzeMachine();

	/* Run the analysis pass over the table data. */
	setTableState( TableArray::AnalyzePass );
	tableDataPass();

	/* Switch the tables over to the code gen mode. */
	setTableState( TableArray::GeneratePass );
}

bool IpGoto::useAgainLabel()
{
	return redFsm->anyRegActionRets() || 
			redFsm->anyRegActionByValControl() || 
			redFsm->anyRegNextStmt();
}

void IpGoto::GOTO( ostream &ret, int gotoDest, bool inFinish )
{
	ret << OPEN_GEN_BLOCK() << "goto st" << gotoDest << ";" << CLOSE_GEN_BLOCK();
}

void IpGoto::GOTO_EXPR( ostream &ret, GenInlineItem *ilItem, bool inFinish )
{
	ret << OPEN_GEN_BLOCK() << vCS() << " = " << OPEN_HOST_EXPR();
	INLINE_LIST( ret, ilItem->children, 0, inFinish, false );
	ret << CLOSE_HOST_EXPR() << "; " << "goto _again;" << CLOSE_GEN_BLOCK();
}

void IpGoto::CALL( ostream &ret, int callDest, int targState, bool inFinish )
{
	ret << OPEN_GEN_BLOCK();

	if ( prePushExpr != 0 ) {
		ret << OPEN_HOST_BLOCK( prePushExpr );
		INLINE_LIST( ret, prePushExpr->inlineList, 0, false, false );
		ret << CLOSE_HOST_BLOCK();
	}

	ret << STACK() << "[" << TOP() << "] = " << targState << 
			"; " << TOP() << "+= 1; " << "goto st" << callDest << ";" <<
			CLOSE_GEN_BLOCK();
}

void IpGoto::NCALL( ostream &ret, int callDest, int targState, bool inFinish )
{
	ret << OPEN_GEN_BLOCK();

	if ( prePushExpr != 0 ) {
		ret << OPEN_HOST_BLOCK( prePushExpr );
		INLINE_LIST( ret, prePushExpr->inlineList, 0, false, false );
		ret << CLOSE_HOST_BLOCK();
	}

	ret << STACK() << "[" << TOP() << "] = " << targState << 
			"; " << TOP() << "+= 1; " << vCS() << " = " << callDest << "; " <<
			CLOSE_GEN_BLOCK();
}

void IpGoto::CALL_EXPR( ostream &ret, GenInlineItem *ilItem, int targState, bool inFinish )
{
	ret << OPEN_GEN_BLOCK();

	if ( prePushExpr != 0 ) {
		ret << OPEN_HOST_BLOCK( prePushExpr );
		INLINE_LIST( ret, prePushExpr->inlineList, 0, false, false );
		ret << CLOSE_HOST_BLOCK();
	}

	ret << STACK() << "[" << TOP() << "] = " << targState << "; " << TOP() << "+= 1;" <<
			vCS() << " = " << OPEN_HOST_EXPR();
	INLINE_LIST( ret, ilItem->children, 0, inFinish, false );
	ret << CLOSE_HOST_EXPR() << "; goto _again;" << CLOSE_GEN_BLOCK();
}

void IpGoto::NCALL_EXPR( ostream &ret, GenInlineItem *ilItem, int targState, bool inFinish )
{
	ret << OPEN_GEN_BLOCK();

	if ( prePushExpr != 0 ) {
		ret << OPEN_HOST_BLOCK( prePushExpr );
		INLINE_LIST( ret, prePushExpr->inlineList, 0, false, false );
		ret << CLOSE_HOST_BLOCK();
	}

	ret << STACK() << "[" << TOP() << "] = " << targState << "; " << TOP() << "+= 1;" <<
			vCS() << " = " << OPEN_HOST_EXPR();
	INLINE_LIST( ret, ilItem->children, 0, inFinish, false );
	ret << CLOSE_HOST_EXPR() << "; " << CLOSE_GEN_BLOCK();
}

void IpGoto::RET( ostream &ret, bool inFinish )
{
	ret << OPEN_GEN_BLOCK() << TOP() << " -= 1;" << vCS() << " = "
			<< STACK() << "[" << TOP() << "];";

	if ( postPopExpr != 0 ) {
		ret << OPEN_HOST_BLOCK( postPopExpr );
		INLINE_LIST( ret, postPopExpr->inlineList, 0, false, false );
		ret << CLOSE_HOST_BLOCK();
	}

	ret << "goto _again;" << CLOSE_GEN_BLOCK();
}

void IpGoto::NRET( ostream &ret, bool inFinish )
{
	ret << OPEN_GEN_BLOCK() << TOP() << " -= 1;" << vCS() << " = "
			<< STACK() << "[" << TOP() << "];";

	if ( postPopExpr != 0 ) {
		ret << OPEN_HOST_BLOCK( postPopExpr );
		INLINE_LIST( ret, postPopExpr->inlineList, 0, false, false );
		ret << CLOSE_HOST_BLOCK();
	}

	ret << CLOSE_GEN_BLOCK();
}

void IpGoto::NEXT( ostream &ret, int nextDest, bool inFinish )
{
	ret << vCS() << " = " << nextDest << ";";
}

void IpGoto::NEXT_EXPR( ostream &ret, GenInlineItem *ilItem, bool inFinish )
{
	ret << vCS() << " = (";
	INLINE_LIST( ret, ilItem->children, 0, inFinish, false );
	ret << ");";
}

void IpGoto::CURS( ostream &ret, bool inFinish )
{
	ret << "(_ps)";
}

void IpGoto::TARGS( ostream &ret, bool inFinish, int targState )
{
	ret << targState;
}

void IpGoto::BREAK( ostream &ret, int targState, bool csForced )
{
	outLabelUsed = true;
	ret << "{" << P() << "+= 1; ";
	if ( !csForced ) 
		ret << vCS() << " = " << targState << "; ";
	ret << "goto _out;}";
}

void IpGoto::NBREAK( ostream &ret, int targState, bool csForced )
{
	outLabelUsed = true;
	ret << "{" << P() << "+= 1; ";
	if ( !csForced ) 
		ret << vCS() << " = " << targState << "; ";
	ret << "_nbreak = 1;}";
}

void IpGoto::NFA_PUSH_ACTION( RedNfaTarg *targ )
{
	int act = 0;
	if ( targ->push != 0 )
		act = targ->push->actListId+1;
	nfaPushActions.value( act );
}

void IpGoto::NFA_POP_TEST( RedNfaTarg *targ )
{
	int act = 0;
	if ( targ->popTest != 0 )
		act = targ->popTest->actListId+1;
	nfaPopTrans.value( act );
}


bool IpGoto::IN_TRANS_ACTIONS( RedStateAp *state )
{
	bool anyWritten = false;

	/* Emit any transitions that have actions and that go to this state. */
	for ( int it = 0; it < state->numInConds; it++ ) {
		RedCondPair *trans = state->inConds[it];
		if ( trans->action != 0 ) {
			/* Remember that we wrote an action so we know to write the
			 * line directive for going back to the output. */
			anyWritten = true;

			/* Write the label for the transition so it can be jumped to. */
			out << "ctr" << trans->id << ":\n";

			/* If the action contains a next, then we must preload the current
			 * state since the action may or may not set it. */
			if ( trans->action->anyNextStmt() )
				out << "	" << vCS() << " = " << trans->targ->id << ";\n";

			if ( redFsm->anyRegNbreak() )
				out << "_nbreak = 0;\n";

			/* Write each action in the list. */
			for ( GenActionTable::Iter item = trans->action->key; item.lte(); item++ ) {
				ACTION( out, item->value, IlOpts( trans->targ->id, false, 
						trans->action->anyNextStmt() ) );
				out << "\n";
			}

			if ( redFsm->anyRegNbreak() ) {
				out <<
					"if ( _nbreak == 1 )\n"
					"	goto _out;\n";
				outLabelUsed = true;
			}
				
 
			/* If the action contains a next then we need to reload, otherwise
			 * jump directly to the target state. */
			if ( trans->action->anyNextStmt() )
				out << "\n\tgoto _again;\n";
			else
				out << "\n\tgoto st" << trans->targ->id << ";\n";
		}
	}


	return anyWritten;
}

/* Called from GotoCodeGen::STATE_GOTOS just before writing the gotos for each
 * state. */
void IpGoto::GOTO_HEADER( RedStateAp *state )
{
	IN_TRANS_ACTIONS( state );

	if ( state->labelNeeded ) 
		out << "st" << state->id << ":\n";

	if ( state->toStateAction != 0 ) {
		/* Write every action in the list. */
		for ( GenActionTable::Iter item = state->toStateAction->key; item.lte(); item++ ) {
			ACTION( out, item->value, IlOpts( state->id, false, 
					state->toStateAction->anyNextStmt() ) );
			out << "\n";
		}
	}

	/* Advance and test buffer pos. */
	if ( state->labelNeeded ) {
		if ( !noEnd ) {
			out <<
				"	" << P() << "+= 1;\n"
				"	if ( " << P() << " == " << PE() << " )\n"
				"		goto _test_eof" << state->id << ";\n";
		}
		else {
			out << 
				"	" << P() << " += 1;\n";
		}
	}

	/* Give the state a switch case. */
	out << "st_case_" << state->id << ":\n";

	if ( state->fromStateAction != 0 ) {
		/* Write every action in the list. */
		for ( GenActionTable::Iter item = state->fromStateAction->key; item.lte(); item++ ) {
			ACTION( out, item->value, IlOpts( state->id, false,
					state->fromStateAction->anyNextStmt() ) );
			out << "\n";
		}
	}

	/* Record the prev state if necessary. */
	if ( state->anyRegCurStateRef() )
		out << "	_ps = " << state->id << ";\n";
}

void IpGoto::STATE_GOTO_ERROR()
{
	/* In the error state we need to emit some stuff that usually goes into
	 * the header. */
	RedStateAp *state = redFsm->errState;
	IN_TRANS_ACTIONS( state );

	out << "st_case_" << state->id << ":\n";
	if ( state->labelNeeded ) 
		out << "st" << state->id << ":\n";

	/* Break out here. */
	outLabelUsed = true;
	out << vCS() << " = " << state->id << ";\n";
	out << "	goto _out;\n";
}


/* Emit the goto to take for a given transition. */
std::ostream &IpGoto::TRANS_GOTO( RedTransAp *trans, int level )
{
	if ( trans->condSpace == 0 || trans->condSpace->condSet.length() == 0 ) {
		/* Existing. */
		assert( trans->numConds() == 1 );
		RedCondPair *cond = trans->outCond( 0 );
		if ( cond->action != 0 ) {
			/* Go to the transition which will go to the state. */
			out << TABS(level) << "goto ctr" << cond->id << ";";
		}
		else {
			/* Go directly to the target state. */
			out << TABS(level) << "goto st" << cond->targ->id << ";";
		}
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
		COND_B_SEARCH( trans, 1, lower, upper, 0, trans->numConds() - 1 );

		if ( trans->errCond() != 0 ) {
			COND_GOTO( trans->errCond(), level+1 ) << "\n";
		}
	}

	return out;
}

/* Emit the goto to take for a given transition. */
std::ostream &IpGoto::COND_GOTO( RedCondPair *cond, int level )
{
	/* Existing. */
	if ( cond->action != 0 ) {
		/* Go to the transition which will go to the state. */
		out << TABS(level) << "goto ctr" << cond->id << ";";
	}
	else {
		/* Go directly to the target state. */
		out << TABS(level) << "goto st" << cond->targ->id << ";";
	}

	return out;
}

std::ostream &IpGoto::EXIT_STATES()
{
	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		if ( st->outNeeded ) {
			testEofUsed = true;
			out << "	_test_eof" << st->id << ": " << vCS() << " = " << 
					st->id << "; goto _test_eof; \n";
		}
	}
	return out;
}

std::ostream &IpGoto::AGAIN_CASES()
{
	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		out << 
			"		case " << st->id << ": goto st" << st->id << ";\n";
	}
	return out;
}

std::ostream &IpGoto::STATE_GOTO_CASES()
{
	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		out << TABS(1) << "case " << st->id << ":\n";
		out << TABS(2) << "goto st_case_" << st->id << ";\n";
	}
	return out;
}

void IpGoto::NFA_PUSH( RedStateAp *state )
{
	if ( redFsm->anyNfaStates() ) {
		out <<
			"	if ( " << ARR_REF( nfaOffsets ) << "[" << state->id << "] ) {\n"
			"		int alt = 0; \n"
			"		int new_recs = " << ARR_REF( nfaTargs ) << "[" << CAST("int") <<
						ARR_REF( nfaOffsets ) << "[" << state->id << "]];\n";

		if ( nfaPrePushExpr != 0 ) {
			out << OPEN_HOST_BLOCK( nfaPrePushExpr );
			INLINE_LIST( out, nfaPrePushExpr->inlineList, 0, false, false );
			out << CLOSE_HOST_BLOCK();
		}

		out <<
			"		while ( alt < new_recs ) { \n";


		out <<
			"			nfa_bp[nfa_len].state = " << ARR_REF( nfaTargs ) << "[" << CAST("int") <<
							ARR_REF( nfaOffsets ) << "[" << state->id << "] + 1 + alt];\n"
			"			nfa_bp[nfa_len].p = " << P() << ";\n";

		if ( redFsm->bAnyNfaPops ) {
			out <<
				"			nfa_bp[nfa_len].popTrans = " << CAST("long") <<
								ARR_REF( nfaOffsets ) << "[" << state->id << "] + 1 + alt;\n"
				"\n"
				;
		}

		if ( redFsm->bAnyNfaPushes ) {
			out <<
				"			switch ( " << ARR_REF( nfaPushActions ) << "[" << CAST("int") <<
								ARR_REF( nfaOffsets ) << "[" << state->id << "] + 1 + alt] ) {\n";

			/* Loop the actions. */
			for ( GenActionTableMap::Iter redAct = redFsm->actionMap;
					redAct.lte(); redAct++ )
			{
				if ( redAct->numNfaPushRefs > 0 ) {
					/* Write the entry label. */
					out << "\t " << CASE( STR( redAct->actListId+1 ) ) << " {\n";

					/* Write each action in the list of action items. */
					for ( GenActionTable::Iter item = redAct->key; item.lte(); item++ )
						ACTION( out, item->value, IlOpts( 0, false, false ) );

					out << "\n\t" << CEND() << "}\n";
				}
			}

			out <<
				"			}\n";
		}


		out <<
			"			nfa_len += 1;\n"
			"			alt += 1;\n"
			"		}\n"
			"	}\n"
			;
	}
}

std::ostream &IpGoto::STATE_GOTOS()
{
	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		if ( st == redFsm->errState )
			STATE_GOTO_ERROR();
		else {
			/* Writing code above state gotos. */
			GOTO_HEADER( st );

			NFA_PUSH( st );

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


std::ostream &IpGoto::FINISH_CASES()
{
	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		if ( st->eofAction != 0 ) {
			if ( st->eofAction->eofRefs == 0 )
				st->eofAction->eofRefs = new IntSet;
			st->eofAction->eofRefs->insert( st->id );
		}
	}

	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		if ( st->eofTrans != 0 ) {
			RedCondPair *cond = st->eofTrans->outCond( 0 );
			out << "	case " << st->id << ": goto ctr" << cond->id << ";\n";
		}
	}

	for ( GenActionTableMap::Iter act = redFsm->actionMap; act.lte(); act++ ) {
		if ( act->eofRefs != 0 ) {
			for ( IntSet::Iter pst = *act->eofRefs; pst.lte(); pst++ ) {
				out << "	case " << *pst << ": \n";
				if ( ! pst.last() )
					out << "		" << FALLTHROUGH() << "\n";
			}

			/* Write each action in the eof action list. */
			for ( GenActionTable::Iter item = act->key; item.lte(); item++ )
				ACTION( out, item->value, IlOpts( STATE_ERR_STATE, true, false ) );
			out << "\n\tbreak;\n";
		}
	}

	return out;
}

void IpGoto::setLabelsNeeded( GenInlineList *inlineList )
{
	for ( GenInlineList::Iter item = *inlineList; item.lte(); item++ ) {
		switch ( item->type ) {
		case GenInlineItem::Goto: case GenInlineItem::Call:
		case GenInlineItem::Ncall: {
			/* Mark the target as needing a label. */
			item->targState->labelNeeded = true;
			break;
		}
		default: break;
		}

		if ( item->children != 0 )
			setLabelsNeeded( item->children );
	}
}

void IpGoto::setLabelsNeeded( RedCondPair *pair )
{
	/* If there is no action with a next statement, then the label will be
	 * needed. */
	if ( pair->action == 0 || !pair->action->anyNextStmt() )
		pair->targ->labelNeeded = true;

	/* Need labels for states that have goto or calls in action code
	 * invoked on characters (ie, not from out action code). */
	if ( pair->action != 0 ) {
		/* Loop the actions. */
		for ( GenActionTable::Iter act = pair->action->key; act.lte(); act++ ) {
			/* Get the action and walk it's tree. */
			setLabelsNeeded( act->value->inlineList );
		}
	}
}

/* Set up labelNeeded flag for each state. */
void IpGoto::setLabelsNeeded()
{
	/* If we use the _again label, then we the _again switch, which uses all
	 * labels. */
	if ( useAgainLabel() ) {
		for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ )
			st->labelNeeded = true;
	}
	else {
		/* Do not use all labels by default, init all labelNeeded vars to false. */
		for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ )
			st->labelNeeded = false;

		for ( TransApSet::Iter trans = redFsm->transSet; trans.lte(); trans++ ) {
			if ( trans->condSpace == 0 )
				setLabelsNeeded( &trans->p );
		}

		for ( CondApSet::Iter cond = redFsm->condSet; cond.lte(); cond++ )
			setLabelsNeeded( &cond->p );
	}

	if ( !noEnd ) {
		for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
			if ( st != redFsm->errState )
				st->outNeeded = st->labelNeeded;
		}
	}
}

void IpGoto::writeData()
{
	STATE_IDS();

	taNfaTargs();
	taNfaOffsets();
	taNfaPushActions();
	taNfaPopTrans();
}

void IpGoto::writeExec()
{
	/* Must set labels immediately before writing because we may depend on the
	 * noend write option. */
	setLabelsNeeded();
	testEofUsed = false;
	outLabelUsed = false;

	out << "	{\n";

	if ( redFsm->anyRegNbreak() )
		out << "	int _nbreak;\n";

	if ( redFsm->anyRegCurStateRef() )
		out << "	int _ps = 0;\n";

	if ( !noEnd ) {
		testEofUsed = true;
		out << 
			"	if ( " << P() << " == " << PE() << " )\n"
			"		goto _test_eof;\n";
	}

	if ( useAgainLabel() ) {
		out << 
			"	goto _resume;\n"
			"\n"
			"_again:\n"
			"	switch ( " << vCS() << " ) {\n";
			AGAIN_CASES() <<
			"	}\n"
			"\n";

	}

	if ( useAgainLabel() || redFsm->anyNfaStates() ) 
		out << "_resume:\n";

	out <<
		"	switch ( " << vCS() << " )\n"
		"	{\n";
		STATE_GOTO_CASES() <<
		"	}\n"
		"	goto st_out;\n";
		STATE_GOTOS() <<
		"	st_out:\n";
		EXIT_STATES() <<
		"\n";

	if ( testEofUsed ) 
		out << "	_test_eof: {}\n";

	if ( redFsm->anyEofTrans() || redFsm->anyEofActions() ) {
		out <<
			"	if ( " << P() << " == " << vEOF() << " )\n"
			"	{\n"
			"	switch ( " << vCS() << " ) {\n";
			FINISH_CASES() <<
			"	}\n"
			"	}\n"
			"\n";
	}

	if ( outLabelUsed ) 
		out << "	_out: {}\n";

	NFA_POP();

	out <<
		"	}\n";
}
