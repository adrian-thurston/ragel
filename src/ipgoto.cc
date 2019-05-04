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
#include "ipgoto.h"
#include "redfsm.h"
#include "gendata.h"
#include "bstmap.h"
#include "parsedata.h"
#include "inputdata.h"

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
	if ( red->id->errorCount > 0 )
		return;
	
	redFsm->setInTrans();

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

bool IpGoto::useAgainLabel()
{
	return redFsm->anyActionRets() || 
			redFsm->anyActionByValControl() ||
			redFsm->anyRegNextStmt();
}

void IpGoto::EOF_CHECK( ostream &ret, int gotoDest )
{
	ret << 
		"       if ( " << P() << " == " << PE() << " )\n"
		"               goto " << eofLabel[gotoDest].reference() << ";\n";
}

void IpGoto::GOTO( ostream &ret, int gotoDest, bool inFinish )
{
	ret << OPEN_GEN_BLOCK();

	if ( inFinish && !noEnd )
		EOF_CHECK( ret, gotoDest );

	ret << "goto " << stLabel[gotoDest].reference() << ";";

	ret << CLOSE_GEN_BLOCK();
}

void IpGoto::GOTO_EXPR( ostream &ret, GenInlineItem *ilItem, bool inFinish )
{
	ret << OPEN_GEN_BLOCK() << vCS() << " = " << OPEN_HOST_EXPR();
	INLINE_LIST( ret, ilItem->children, 0, inFinish, false );
	ret << CLOSE_HOST_EXPR() << ";";

	/* Since we are setting CS above and can select on it, call the all-state
	 * test_eof. */
	if ( inFinish && !noEnd )
		Goto::EOF_CHECK( ret );
	
	ret << " goto " << _again << ";";
	
	ret << CLOSE_GEN_BLOCK();
}

void IpGoto::CALL( ostream &ret, int callDest, int targState, bool inFinish )
{
	ret << OPEN_GEN_BLOCK();

	if ( red->prePushExpr != 0 ) {
		ret << OPEN_HOST_BLOCK( red->prePushExpr );
		INLINE_LIST( ret, red->prePushExpr->inlineList, 0, false, false );
		ret << CLOSE_HOST_BLOCK();
	}

	ret << STACK() << "[" << TOP() << "] = " << targState << 
			"; " << TOP() << "+= 1; ";

	if ( inFinish && !noEnd )
		EOF_CHECK( ret, callDest );

	ret << "goto " << stLabel[callDest].reference() << ";";

	ret << CLOSE_GEN_BLOCK();
}

void IpGoto::NCALL( ostream &ret, int callDest, int targState, bool inFinish )
{
	ret << OPEN_GEN_BLOCK();

	if ( red->prePushExpr != 0 ) {
		ret << OPEN_HOST_BLOCK( red->prePushExpr );
		INLINE_LIST( ret, red->prePushExpr->inlineList, 0, false, false );
		ret << CLOSE_HOST_BLOCK();
	}

	ret << STACK() << "[" << TOP() << "] = " << targState << 
			"; " << TOP() << "+= 1; " << vCS() << " = " << callDest << "; " <<
			CLOSE_GEN_BLOCK();
}

void IpGoto::CALL_EXPR( ostream &ret, GenInlineItem *ilItem, int targState, bool inFinish )
{
	ret << OPEN_GEN_BLOCK();

	if ( red->prePushExpr != 0 ) {
		ret << OPEN_HOST_BLOCK( red->prePushExpr );
		INLINE_LIST( ret, red->prePushExpr->inlineList, 0, false, false );
		ret << CLOSE_HOST_BLOCK();
	}

	ret << STACK() << "[" << TOP() << "] = " << targState << "; " << TOP() << "+= 1;" <<
			vCS() << " = " << OPEN_HOST_EXPR();
	INLINE_LIST( ret, ilItem->children, 0, inFinish, false );
	ret << CLOSE_HOST_EXPR() << ";";

	/* Since we are setting CS above and can select on it, call the all-state
	 * test_eof. */
	if ( inFinish && !noEnd )
		Goto::EOF_CHECK( ret );

	ret << " goto " << _again << ";";
	
	ret << CLOSE_GEN_BLOCK();
}

void IpGoto::NCALL_EXPR( ostream &ret, GenInlineItem *ilItem, int targState, bool inFinish )
{
	ret << OPEN_GEN_BLOCK();

	if ( red->prePushExpr != 0 ) {
		ret << OPEN_HOST_BLOCK( red->prePushExpr );
		INLINE_LIST( ret, red->prePushExpr->inlineList, 0, false, false );
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

	if ( red->postPopExpr != 0 ) {
		ret << OPEN_HOST_BLOCK( red->postPopExpr );
		INLINE_LIST( ret, red->postPopExpr->inlineList, 0, false, false );
		ret << CLOSE_HOST_BLOCK();
	}

	if ( inFinish && !noEnd )
		Goto::EOF_CHECK( ret );

	ret << "goto " << _again << ";" << CLOSE_GEN_BLOCK();
}

void IpGoto::NRET( ostream &ret, bool inFinish )
{
	ret << OPEN_GEN_BLOCK() << TOP() << " -= 1;" << vCS() << " = "
			<< STACK() << "[" << TOP() << "];";

	if ( red->postPopExpr != 0 ) {
		ret << OPEN_HOST_BLOCK( red->postPopExpr );
		INLINE_LIST( ret, red->postPopExpr->inlineList, 0, false, false );
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
	ret << "(" << ps << ")";
}

void IpGoto::TARGS( ostream &ret, bool inFinish, int targState )
{
	ret << targState;
}

void IpGoto::BREAK( ostream &ret, int targState, bool csForced )
{
	ret << "{" << P() << "+= 1; ";
	if ( !csForced ) 
		ret << vCS() << " = " << targState << "; ";
	ret << "goto " << _out << ";}";
}

void IpGoto::NBREAK( ostream &ret, int targState, bool csForced )
{
	ret << "{" << P() << "+= 1; ";
	if ( !csForced ) 
		ret << vCS() << " = " << targState << "; ";
	ret << nbreak << " = 1;}";
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
			if ( ctrLabel[trans->id].isReferenced )
				out << "_ctr" << trans->id << ":\n";

			/* If the action contains a next, then we must preload the current
			 * state since the action may or may not set it. */
			if ( trans->action->anyNextStmt() )
				out << "	" << vCS() << " = " << trans->targ->id << ";\n";

			if ( redFsm->anyRegNbreak() )
				out << nbreak << " = 0;\n";

			/* Write each action in the list. */
			for ( GenActionTable::Iter item = trans->action->key; item.lte(); item++ ) {
				ACTION( out, item->value, IlOpts( trans->targ->id, false, 
						trans->action->anyNextStmt() ) );
				out << "\n";
			}

			if ( redFsm->anyRegNbreak() ) {
				out <<
					"if ( " << nbreak << " == 1 )\n"
					"	goto " << _out << ";\n";
			}
				
 
			/* If the action contains a next then we need to reload, otherwise
			 * jump directly to the target state. */
			if ( trans->action->anyNextStmt() )
				out << "goto " << _again << ";\n";
			else
				out << "goto " << stLabel[trans->targ->id].reference() << ";\n";
		}
	}


	return anyWritten;
}

/* Called from GotoCodeGen::STATE_GOTOS just before writing the gotos for each
 * state. */
void IpGoto::GOTO_HEADER( RedStateAp *state )
{
	IN_TRANS_ACTIONS( state );

	if ( stLabel[state->id].isReferenced ) 
		out << "_st" << state->id << ":\n";

	/* need to do this if the transition is an eof transition, or if the action
	 * contains fexec. Otherwise, no need. */
	if ( redFsm->anyEofActivity() ) {
		out <<
			"if ( " << P() << " == " << vEOF() << " ) {\n"
			"	if ( " << vCS() << " >= " << FIRST_FINAL_STATE() << " )\n"
			"		goto " << _out << ";\n"
			"	else\n"
			"		goto " << _pop << ";\n"
			"}\n";
	}

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
				P() << "+= 1;\n"
				"if ( " << P() << " == " << PE() << " )\n"
				"	goto " << eofLabel[state->id].reference() << ";\n";
		}
		else {
			out << 
				P() << " += 1;\n";
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
		out << ps << " = " << state->id << ";\n";
}

void IpGoto::STATE_GOTO_ERROR()
{
	/* In the error state we need to emit some stuff that usually goes into
	 * the header. */
	RedStateAp *state = redFsm->errState;
	IN_TRANS_ACTIONS( state );

	out << "st_case_" << state->id << ":\n";
	if ( stLabel[state->id].isReferenced ) 
		out << "_st" << state->id << ":\n";

	/* Break out here. */
	out << vCS() << " = " << state->id << ";\n";
	out << "goto " << _pop << ";\n";
}


/* Emit the goto to take for a given transition. */
std::ostream &IpGoto::TRANS_GOTO( RedTransAp *trans )
{
	if ( trans->condSpace == 0 || trans->condSpace->condSet.length() == 0 ) {
		/* Existing. */
		assert( trans->numConds() == 1 );
		RedCondPair *cond = trans->outCond( 0 );
		if ( cond->action != 0 ) {
			/* Go to the transition which will go to the state. */
			out << "goto " << ctrLabel[trans->p.id].reference() << ";";
		}
		else {
			/* Go directly to the target state. */
			out << "goto " << stLabel[cond->targ->id].reference() << ";";
		}
	}
	else {
		out << ck << " = 0;\n";
		for ( GenCondSet::Iter csi = trans->condSpace->condSet; csi.lte(); csi++ ) {
			out << "if ( ";
			CONDITION( out, *csi );
			Size condValOffset = (1 << csi.pos());
			out << " )\n" << ck << " += " << condValOffset << ";\n";
		}
		CondKey lower = 0;
		CondKey upper = trans->condFullSize() - 1;
		COND_B_SEARCH( trans, lower, upper, 0, trans->numConds() - 1 );

		if ( trans->errCond() != 0 ) {
			COND_GOTO( trans->errCond() ) << "\n";
		}
	}

	return out;
}

/* Emit the goto to take for a given transition. */
std::ostream &IpGoto::COND_GOTO( RedCondPair *cond )
{
	/* Existing. */
	if ( cond->action != 0 ) {
		/* Go to the transition which will go to the state. */
		out << "goto " << ctrLabel[cond->id].reference() << ";";
	}
	else {
		/* Go directly to the target state. */
		out << "goto " << stLabel[cond->targ->id].reference() << ";";
	}

	return out;
}

std::ostream &IpGoto::EXIT_STATES()
{
	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		if ( eofLabel[st->id].isReferenced ) {
			out << eofLabel[st->id].define() << ": " << vCS() << " = " << 
					st->id << "; goto " << _test_eof << "; \n";
		}
	}
	return out;
}

std::ostream &IpGoto::AGAIN_CASES()
{
	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		out << 
			"case " << st->id << ": goto " << stLabel[st->id].reference() << ";\n";
	}
	return out;
}

std::ostream &IpGoto::STATE_GOTO_CASES()
{
	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		out << "case " << st->id << ":\n";
		out << "goto st_case_" << st->id << ";\n";
	}
	return out;
}

void IpGoto::NFA_PUSH( RedStateAp *state )
{
	std::stringstream ss;
	ss << state->id;
	std::string _state = ss.str();

	if ( redFsm->anyNfaStates() ) {

		if ( state->nfaTargs != 0 ) {
			out <<
				"if ( " << ARR_REF( nfaOffsets ) << "[" << _state << "] ) {\n"
				"	" << new_recs << " = " << state->nfaTargs->length() << ";\n";

			if ( red->nfaPrePushExpr != 0 ) {
				out << OPEN_HOST_BLOCK( red->nfaPrePushExpr );
				INLINE_LIST( out, red->nfaPrePushExpr->inlineList, 0, false, false );
				out << CLOSE_HOST_BLOCK();
			}

			int alt = 0;
			for ( RedNfaTargs::Iter nt = *state->nfaTargs; nt.lte(); nt++ ) {
				out <<
					"	nfa_bp[nfa_len].state = " << nt->state->id << ";\n"
					"	nfa_bp[nfa_len].p = " << P() << ";\n";

				if ( nt->popTest != 0 ) {
					out <<
						"	nfa_bp[nfa_len].popTrans = " << (nt->popTest->actListId+1) << ";\n";
				}
				else if ( redFsm->bAnyNfaPops ) {
					out <<
						"	nfa_bp[nfa_len].popTrans = 0;\n";
				}

				if ( nt->push != 0 )  {
					for ( GenActionTable::Iter item = nt->push->key; item.lte(); item++ )
						ACTION( out, item->value, IlOpts( 0, false, false ) );
				}

				out <<
					"	nfa_len += 1;\n";

				alt += 1;
			}

			out <<
				"}\n";
		}
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
				RANGE_B_SEARCH( st, keyOps->minKey, keyOps->maxKey,
						0, st->outRange.length() - 1 );
			}

			/* Write the default transition. */
			out << "{\n";
			TRANS_GOTO( st->defTrans ) << "\n";
			out << "}\n";
		}
	}
	return out;
}

std::ostream &IpGoto::FINISH_CASES()
{
	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		if ( st->eofTrans != 0 ) {
			out <<
				"case " << st->id << ":\n";

			TRANS_GOTO( st->eofTrans );
		}
	}

	return out;
}

void IpGoto::setLabelsNeeded( GenInlineList *inlineList )
{
	for ( GenInlineList::Iter item = *inlineList; item.lte(); item++ ) {
		switch ( item->type ) {
			case GenInlineItem::Goto:
			case GenInlineItem::Call:
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
	/* If we use the _again label, then we generate the _again switch, which
	 * uses all labels. */
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

		for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
			if ( st->eofAction != 0 ) {
				for ( GenActionTable::Iter item = st->eofAction->key; item.lte(); item++ )
					setLabelsNeeded( item->value->inlineList );
			}
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

void IpGoto::NFA_FROM_STATE_ACTION_EXEC()
{
//	if ( redFsm->anyFromStateActions() ) {
//		/* Unimplemented feature. Don't have the from state actions array in
//		 * this mode. Need to add it, or to alter the NFA pop codegen to be
//		 * consistent with the mode. */ 
//		assert( false );
//	}
}

void IpGoto::writeExec()
{
	int maxCtrId = redFsm->nextCondId > redFsm->nextTransId ? redFsm->nextCondId : redFsm->nextTransId;

	stLabel = allocateLabels( stLabel, IpLabel::St, redFsm->nextStateId );
	eofLabel = allocateLabels( eofLabel, IpLabel::TestEof, redFsm->nextStateId );
	ctrLabel = allocateLabels( ctrLabel, IpLabel::Ctr, maxCtrId );

	/* Must set labels immediately before writing because we may depend on the
	 * noend write option. */
	setLabelsNeeded();

	out << "{\n";

	DECLARE( INT(), cpc );
	DECLARE( INT(), ck );
	DECLARE( INT(), pop_test );
	DECLARE( INT(), nbreak );
	DECLARE( INT(), ps );
	DECLARE( INT(), new_recs );
	DECLARE( INT(), alt );

	if ( !noEnd ) {
		out << 
			"	if ( " << P() << " == " << PE() << " )\n"
			"		goto " << _test_eof << ";\n";
	}

	if ( _again.isReferenced ) {
		out << 
			"	goto " << _resume << ";\n"
			"\n";

		out << EMIT_LABEL( _again );

		out <<
			"	switch ( " << vCS() << " ) {\n";
			AGAIN_CASES() <<
			"	}\n"
			"\n";

	}

	out << EMIT_LABEL( _resume );

	out <<
		"	switch ( " << vCS() << " ) {\n";
		STATE_GOTO_CASES() <<
		"	}\n"
		"	goto st_out;\n";
		STATE_GOTOS() <<
		"	st_out:\n";
		EXIT_STATES() <<
		"\n";

	out << EMIT_LABEL( _test_eof );

	if ( redFsm->anyEofActivity() ) {
		out <<
			"	if ( " << P() << " == " << vEOF() << " ) {\n";

		out <<
			"		switch ( " << vCS() << " ) {\n";

		for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
			out << "case " << st->id << ": {\n";
			NFA_PUSH( st );
			FROM_STATE_ACTION_EMIT( st );

			out << "break;\n}\n";
		}

		out <<
			"		}\n";

		out <<
			"		switch ( " << vCS() << " ) {\n";

		bool okeydokey = false;
		for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
			if ( st->outCondSpace != 0 ) {
				out << "case " << st->id << ": {\n";

				out << ck << " = 0;\n";
				for ( GenCondSet::Iter csi = st->outCondSpace->condSet; csi.lte(); csi++ ) {
					out << "if ( ";
					CONDITION( out, *csi );
					Size condValOffset = (1 << csi.pos());
					out << " )\n" << ck << " += " << condValOffset << ";\n";
				}

				out << "switch ( " << ck << " ) {\n";
				for ( CondKeySet::Iter k = st->outCondKeys; k.lte(); k++ ) {
					out << "case " << *k << ": goto _okeydokey;\n";
					okeydokey = true;
				}
				out << "}\n";

				out << vCS() << " = " << ERROR_STATE() << ";\n";
				out << "goto " << _pop << ";\n";

				out << "}\n";
			}
		}

		out <<
			"	}\n";

		if ( okeydokey ) {
			out <<
				"_okeydokey: {}\n";
		}

		out <<
			"	switch ( " << vCS() << " ) {\n";
			FINISH_CASES() <<
			"	}\n";


		out <<
			"	}\n"
			"\n";
	}

	out <<
		"if ( " << vCS() << " >= " << FIRST_FINAL_STATE() << " )\n"
		"	goto " << _out << "; ";

	out << EMIT_LABEL( _pop );

	if ( redFsm->anyNfaStates() ) {
		out <<
			"if ( nfa_len == 0 )\n"
			"	goto " << _out << ";\n"
			"\n";

		out <<
			"nfa_count += 1;\n"
			"nfa_len -= 1;\n" <<
			P() << " = nfa_bp[nfa_len].p;\n";

		if ( redFsm->bAnyNfaPops ) {
			NFA_FROM_STATE_ACTION_EXEC();

			NFA_POP_TEST_EXEC();

			out <<
				"if ( " << pop_test << " )\n"
				"	" << vCS() << " = nfa_bp[nfa_len].state;\n"
				"else\n"
				"	" << vCS() << " = " << ERROR_STATE() << ";\n";
		}
		else {
			out <<
				vCS() << " = nfa_bp[nfa_len].state;\n";

		}

		NFA_POST_POP();

		out << "goto " << _resume << ";\n";
	}

	out << EMIT_LABEL( _out );

	out <<
		"}\n";
}
