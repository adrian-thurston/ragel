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
#include "switchexpgoto.h"
#include "redfsm.h"
#include "gendata.h"
#include "bstmap.h"

void SwitchExpGoto::tableDataPass()
{
	taToStateActions();
	taFromStateActions();
	taEofActions();

	taNfaTargs();
	taNfaOffsets();
	taNfaPushActions();
	taNfaPopTrans();
}

void SwitchExpGoto::genAnalysis()
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

std::ostream &SwitchExpGoto::EXEC_ACTIONS()
{
	/* Loop the actions. */
	for ( GenActionTableMap::Iter redAct = redFsm->actionMap; redAct.lte(); redAct++ ) {
		if ( redAct->numTransRefs > 0 ) {
			/* 	We are at the start of a glob, write the case. */
			out << "f" << redAct->actListId << ":\n";

			if ( redFsm->anyRegNbreak() )
				out << "_nbreak = 0;\n";

			/* Write each action in the list of action items. */
			for ( GenActionTable::Iter item = redAct->key; item.lte(); item++ )
				ACTION( out, item->value, IlOpts( 0, false, false ) );

			if ( redFsm->anyRegNbreak() ) {
				out << 
					"	if ( _nbreak == 1 )\n"
					"		goto _out;\n";
				outLabelUsed = true;
			}


			out << "\n\tgoto _again;\n";
		}
	}
	return out;
}

/* Write out the function switch. This switch is keyed on the values
 * of the func index. */
std::ostream &SwitchExpGoto::TO_STATE_ACTION_SWITCH()
{
	/* Loop the actions. */
	for ( GenActionTableMap::Iter redAct = redFsm->actionMap; redAct.lte(); redAct++ ) {
		if ( redAct->numToStateRefs > 0 ) {
			/* Write the entry label. */
			out << "\t" << CASE( STR( redAct->actListId+1 ) )  << "{\n";

			/* Write each action in the list of action items. */
			for ( GenActionTable::Iter item = redAct->key; item.lte(); item++ )
				ACTION( out, item->value, IlOpts( 0, false, false ) );

			out << "\n\t" << CEND() << "}\n";
		}
	}

	return out;
}

/* Write out the function switch. This switch is keyed on the values
 * of the func index. */
std::ostream &SwitchExpGoto::FROM_STATE_ACTION_SWITCH()
{
	/* Loop the actions. */
	for ( GenActionTableMap::Iter redAct = redFsm->actionMap; redAct.lte(); redAct++ ) {
		if ( redAct->numFromStateRefs > 0 ) {
			/* Write the entry label. */
			out << "\t" << CASE( STR( redAct->actListId+1 ) ) << "{\n";

			/* Write each action in the list of action items. */
			for ( GenActionTable::Iter item = redAct->key; item.lte(); item++ )
				ACTION( out, item->value, IlOpts( 0, false, false ) );

			out << "\n\t" << CEND() << "}\n";
		}
	}

	return out;
}

std::ostream &SwitchExpGoto::EOF_ACTION_SWITCH()
{
	/* Loop the actions. */
	for ( GenActionTableMap::Iter redAct = redFsm->actionMap; redAct.lte(); redAct++ ) {
		if ( redAct->numEofRefs > 0 ) {
			/* Write the entry label. */
			out << "\t" << CASE( STR( redAct->actListId+1 ) ) << "{\n";

			/* Write each action in the list of action items. */
			for ( GenActionTable::Iter item = redAct->key; item.lte(); item++ )
				ACTION( out, item->value, IlOpts( 0, true, false ) );

			out << "\n\t" << CEND() << "}\n";
		}
	}

	return out;
}

unsigned int SwitchExpGoto::TO_STATE_ACTION( RedStateAp *state )
{
	int act = 0;
	if ( state->toStateAction != 0 )
		act = state->toStateAction->actListId+1;
	return act;
}

unsigned int SwitchExpGoto::FROM_STATE_ACTION( RedStateAp *state )
{
	int act = 0;
	if ( state->fromStateAction != 0 )
		act = state->fromStateAction->actListId+1;
	return act;
}

unsigned int SwitchExpGoto::EOF_ACTION( RedStateAp *state )
{
	int act = 0;
	if ( state->eofAction != 0 )
		act = state->eofAction->actListId+1;
	return act;
}

void SwitchExpGoto::NFA_PUSH_ACTION( RedNfaTarg *targ )
{
	int act = 0;
	if ( targ->push != 0 )
		act = targ->push->actListId+1;
	nfaPushActions.value( act );
}

void SwitchExpGoto::NFA_POP_TEST( RedNfaTarg *targ )
{
	int act = 0;
	if ( targ->popTest != 0 )
		act = targ->popTest->actListId+1;
	nfaPopTrans.value( act );
}


void SwitchExpGoto::writeData()
{
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

void SwitchExpGoto::writeExec()
{
	testEofUsed = false;
	outLabelUsed = false;

	out << "	{\n";

	if ( redFsm->anyRegCurStateRef() )
		out << "	int _ps = 0;\n";

	if ( redFsm->anyRegNbreak() ) {
		out << 
			"	int _nbreak;\n";
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
			"		goto _out;\n";
	}

	out << "_resume:\n";

	if ( redFsm->anyFromStateActions() ) {
		out <<
			"	switch ( " << ARR_REF( fromStateActions ) << "[" << vCS() << "] ) {\n";
			FROM_STATE_ACTION_SWITCH() <<
			"	}\n"
			"\n";
	}

	NFA_PUSH();

	out << 
		"	switch ( " << vCS() << " ) {\n";
		STATE_GOTOS() <<
		"	}\n"
		"\n";
		TRANSITIONS() << 
		"\n";

	if ( redFsm->anyRegActions() )
		EXEC_ACTIONS() << "\n";

	out << "_again:\n";

	if ( redFsm->anyToStateActions() ) {
		out <<
			"	switch ( " << ARR_REF( toStateActions ) << "[" << vCS() << "] ) {\n";
			TO_STATE_ACTION_SWITCH() <<
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
			"	" << P() << "+= 1;\n"
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

		if ( redFsm->anyEofActions() ) {
			out <<
				"	switch ( " << ARR_REF( eofActions ) << "[" << vCS() << "] ) {\n";
				EOF_ACTION_SWITCH() <<
				"	}\n";
		}

		out <<
			"	}\n"
			"\n";
	}

	if ( outLabelUsed )
		out << "	_out: {}\n";

	NFA_POP();

	out << "	}\n";
}
