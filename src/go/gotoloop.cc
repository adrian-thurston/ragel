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
#include "gotoloop.h"
#include "redfsm.h"
#include "bstmap.h"
#include "gendata.h"

namespace Go {

void GotoLooped::tableDataPass()
{
	taActions();
	taToStateActions();
	taFromStateActions();
	taEofActions();
}

void GotoLooped::genAnalysis()
{
	/* For directly executable machines there is no required state
	 * ordering. Choose a depth-first ordering to increase the
	 * potential for fall-throughs. */
	redFsm->depthFirstOrdering();

	/* Choose default transitions and the single transition. */
	redFsm->chooseDefaultSpan();

	/* Choose single. */
	redFsm->chooseSingle();

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

void GotoLooped::writeData()
{
	if ( redFsm->anyActions() )
		taActions();

	if ( redFsm->anyToStateActions() )
		taToStateActions();

	if ( redFsm->anyFromStateActions() )
		taFromStateActions();

	if ( redFsm->anyEofActions() )
		taEofActions();

	STATE_IDS();
}

void GotoLooped::writeExec()
{
	testEofUsed = false;
	outLabelUsed = false;

	out << "	{\n";

	if ( redFsm->anyRegCurStateRef() )
		out << "	var _ps int = 0\n";

	if ( redFsm->anyToStateActions() || redFsm->anyRegActions()
			|| redFsm->anyFromStateActions() )
	{
		out <<
			"	var _acts uint\n" // actions array index
			"	var _nacts uint\n";
	}

	out << "\n";

	if ( !noEnd ) {
		testEofUsed = true;
		out <<
			"	if " << P() << " == " << PE() << " {\n"
			"		goto _test_eof\n"
			"	}\n";
	}

	if ( redFsm->errState != 0 ) {
		outLabelUsed = true;
		out <<
			"	if " << vCS() << " == " << redFsm->errState->id << " {\n"
			"		goto _out\n"
			"	}\n";
	}

	out << "_resume:\n";

	if ( redFsm->anyFromStateActions() ) {
		out <<
			"	_acts = " << "uint(" << ARR_REF( fromStateActions ) << "[" << vCS() << "]" << ")\n" // actions array offset
			"	_nacts = " << "uint(" << ARR_REF( actions ) << "[_acts]); _acts++\n"
			"	for ; _nacts > 0; _nacts-- {\n"
			"		_acts++\n"
			"		switch " << ARR_REF( actions ) << "[_acts - 1]" << " {\n";
			FROM_STATE_ACTION_SWITCH() <<
			"		}\n"
			"	}\n"
			"\n";
	}

	out <<
		"	switch " << vCS() << " {\n";
		STATE_GOTOS() <<
		"	}\n"
		"\n";
		TRANSITIONS() <<
		"\n";

	if ( redFsm->anyRegActions() )
		EXEC_FUNCS() << "\n";

	out << "_again:\n";

	if ( redFsm->anyToStateActions() ) {
		out <<
			"	_acts = " << "uint(" << ARR_REF( toStateActions ) << "[" << vCS() << "]" << ")\n" // actions array offset
			"	_nacts = " << "uint(" << ARR_REF( actions ) << "[_acts]); _acts++\n"
			"	for ; _nacts > 0; _nacts-- {\n"
			"		_acts++\n"
			"		switch " << ARR_REF( actions ) << "[_acts - 1]" << " {\n";
			TO_STATE_ACTION_SWITCH() <<
			"		}\n"
			"	}\n"
			"\n";
	}

	if ( redFsm->errState != 0 ) {
		outLabelUsed = true;
		out <<
			"	if " << vCS() << " == " << redFsm->errState->id << " {\n"
			"		goto _out\n"
			"	}\n";
	}

	if ( !noEnd ) {
		out <<
			"	if " << P() << "++; " << P() << " != " << PE() << " {\n"
			"		goto _resume\n"
			"	}\n";
	}
	else {
		out <<
			"	" << P() << " += 1\n"
			"	goto _resume\n";
	}

	if ( testEofUsed )
		out << "	_test_eof: {}\n";

	if ( redFsm->anyEofTrans() || redFsm->anyEofActions() ) {
		out <<
			"	if " << P() << " == " << vEOF() << " {\n";

		if ( redFsm->anyEofTrans() ) {
			out <<
				"	switch " << vCS() << " {\n";

			for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
				if ( st->eofTrans != 0 ) {
					RedCondAp *cond = st->eofTrans->outConds.data[0].value;
					out << "	case " << st->id << ":\n	goto ctr" << cond->id << "\n";
				}
			}

			out <<
				"	}\n";
		}

		if ( redFsm->anyEofActions() ) {
			out <<
				"	var __acts = " << "uint(" << ARR_REF( eofActions ) << "[" << vCS() << "]" << ")\n" // actions array offset
				"	var __nacts = " << "uint(" << ARR_REF( actions ) << "[__acts]); __acts++\n"
				"	for ; __nacts > 0; __nacts-- {\n"
				"		__acts++\n"
				"		switch " << ARR_REF( actions ) << "[__acts  - 1]" << " {\n";
				EOF_ACTION_SWITCH() <<
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
