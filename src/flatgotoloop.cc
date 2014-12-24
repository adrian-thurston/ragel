/*
 *  Copyright 2004-2014 Adrian Thurston <thurston@complang.org>
 */

#include "ragel.h"
#include "flatgotoloop.h"
#include "redfsm.h"
#include "gendata.h"

void FlatGotoLoop::tableDataPass()
{
	taActions();
	taKeys();
	taKeySpans();
	taFlatIndexOffset();

	taIndicies();
	taTransCondSpaces();
	taTransOffsets();
	taTransLengths();
	taCondKeys();
	taCondTargs();
	taCondActions();

	taToStateActions();
	taFromStateActions();
	taEofActions();
	taEofTrans();
}

void FlatGotoLoop::genAnalysis()
{
	redFsm->sortByStateId();

	/* Choose default transitions and the single transition. */
	redFsm->chooseDefaultSpan();
		
	/* Do flat expand. */
	redFsm->makeFlat();

	/* If any errors have occured in the input file then don't write anything. */
	if ( gblErrorCount > 0 )
		return;

	/* Anlayze Machine will find the final action reference counts, among other
	 * things. We will use these in reporting the usage of fsm directives in
	 * action code. */
	analyzeMachine();

	setKeyType();

	/* Run the analysis pass over the table data. */
	setTableState( TableArray::AnalyzePass );
	tableDataPass();

	/* Switch the tables over to the code gen mode. */
	setTableState( TableArray::GeneratePass );
}

std::ostream &FlatGotoLoop::TO_STATE_ACTION_SWITCH()
{
	/* Walk the list of functions, printing the cases. */
	for ( GenActionList::Iter act = actionList; act.lte(); act++ ) {
		/* Write out referenced actions. */
		if ( act->numToStateRefs > 0 ) {
			/* Write the case label, the action and the case break */
			out << "\t " << CASE( STR( act->actionId ) ) << " {\n";
			ACTION( out, act, IlOpts( 0, false, false ) );
			out << "\n\t" << CEND() << "}\n";
		}
	}

	return out;
}

std::ostream &FlatGotoLoop::FROM_STATE_ACTION_SWITCH()
{
	/* Walk the list of functions, printing the cases. */
	for ( GenActionList::Iter act = actionList; act.lte(); act++ ) {
		/* Write out referenced actions. */
		if ( act->numFromStateRefs > 0 ) {
			/* Write the case label, the action and the case break */
			out << "\t " << CASE( STR( act->actionId ) ) << " {\n";
			ACTION( out, act, IlOpts( 0, false, false ) );
			out << "\n\t" << CEND() << "}\n";
		}
	}

	return out;
}

std::ostream &FlatGotoLoop::EOF_ACTION_SWITCH()
{
	/* Walk the list of functions, printing the cases. */
	for ( GenActionList::Iter act = actionList; act.lte(); act++ ) {
		/* Write out referenced actions. */
		if ( act->numEofRefs > 0 ) {
			/* Write the case label, the action and the case break */
			out << "\t " << CASE( STR( act->actionId ) ) << " {\n";
			ACTION( out, act, IlOpts( 0, true, false ) );
			out << "\n\t" << CEND() << "}\n";
		}
	}

	return out;
}

std::ostream &FlatGotoLoop::ACTION_SWITCH()
{
	/* Walk the list of functions, printing the cases. */
	for ( GenActionList::Iter act = actionList; act.lte(); act++ ) {
		/* Write out referenced actions. */
		if ( act->numTransRefs > 0 ) {
			/* Write the case label, the action and the case break */
			out << "\t " << CASE( STR( act->actionId ) ) << " {\n";
			ACTION( out, act, IlOpts( 0, false, false ) );
			out << "\n\t" << CEND() << "}\n";
		}
	}

	return out;
}

void FlatGotoLoop::writeData()
{
	/* If there are any transtion functions then output the array. If there
	 * are none, don't bother emitting an empty array that won't be used. */
	if ( redFsm->anyActions() )
		taActions();

	taKeys();
	taKeySpans();
	taFlatIndexOffset();

	taIndicies();
	taTransCondSpaces();
	taTransOffsets();
	taTransLengths();
	taCondKeys();
	taCondTargs();
	taCondActions();

	if ( redFsm->anyToStateActions() )
		taToStateActions();

	if ( redFsm->anyFromStateActions() )
		taFromStateActions();

	if ( redFsm->anyEofActions() )
		taEofActions();

	if ( redFsm->anyEofTrans() )
		taEofTrans();

	STATE_IDS();
}

void FlatGotoLoop::writeExec()
{
	testEofUsed = false;
	outLabelUsed = false;

	out << 
		"	{\n"
		"	int _slen;\n";

	if ( redFsm->anyRegCurStateRef() )
		out << "	int _ps;\n";

	out << 
		"	int _trans;\n"
		"	" << UINT() << " _cond;\n";

	if ( redFsm->anyToStateActions() || 
			redFsm->anyRegActions() || redFsm->anyFromStateActions() )
	{
		out << 
			"	" << INDEX( ARR_TYPE( actions ), "_acts" ) << ";\n"
			"	" << UINT() << " _nacts;\n"; 
	}

	out <<
		"	" << INDEX( ALPH_TYPE(), "_keys" ) << ";\n"
		"	" << INDEX( ARR_TYPE( indicies ), "_inds" ) << ";\n"
		"	" << INDEX( ARR_TYPE( condKeys ), "_ckeys" ) << ";\n"
		"	int _klen;\n"
		"	int _cpc;\n";

	if ( redFsm->anyRegNbreak() )
		out << "	int _nbreak;\n";

	out <<
		"	" << ENTRY() << " {\n";

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

	out << LABEL( "_resume" ) << " {\n";

	if ( redFsm->anyFromStateActions() ) {
		out <<
			"	_acts = " << OFFSET( ARR_REF( actions ), ARR_REF( fromStateActions ) + "[" + vCS() + "]" ) << ";\n"
			"	_nacts = (" << UINT() << ") " << DEREF( ARR_REF( actions ), "_acts" ) << ";\n"
			"	_acts += 1;\n"
			"	while ( _nacts > 0 ) {\n"
			"		switch ( " << DEREF( ARR_REF( actions ), "_acts" ) << " ) {\n";
			FROM_STATE_ACTION_SWITCH() <<
			"		}\n"
			"		_nacts -= 1;\n"
			"		_acts += 1;\n"
			"	}\n"
			"\n";
	}

	LOCATE_TRANS();

	out << "} " << LABEL( "_match_cond" ) << " {\n";

	if ( redFsm->anyRegCurStateRef() )
		out << "	_ps = " << vCS() << ";\n";

	out <<
		"	" << vCS() << " = (int) " << ARR_REF( condTargs ) << "[_cond];\n"
		"\n";

	if ( redFsm->anyRegActions() ) {
		out <<
			"	if ( " << ARR_REF( condActions ) << "[_cond] == 0 )\n"
			"		goto _again;\n"
			"\n";

		if ( redFsm->anyRegNbreak() )
			out << "	_nbreak = 0;\n";

		out <<
			"	_acts = " << OFFSET( ARR_REF( actions ), ARR_REF( condActions ) + "[_cond]" ) << ";\n"
			"	_nacts = (" << UINT() << ") " << DEREF( ARR_REF( actions ), "_acts" ) << ";\n"
			"	_acts += 1;\n"
			"	while ( _nacts > 0 ) {\n"
			"		switch ( " << DEREF( ARR_REF( actions ), "_acts" ) << " )\n"
			"		{\n";
			ACTION_SWITCH() <<
			"		}\n"
			"		_nacts -= 1;\n"
			"		_acts += 1;\n"
			"	}\n"
			"\n";

		if ( redFsm->anyRegNbreak() ) {
			out <<
				"	if ( _nbreak == 1 )\n"
				"		goto _out;\n";
			outLabelUsed = true;
		}

		out <<
			"\n";
	}

//	if ( redFsm->anyRegActions() || redFsm->anyActionGotos() || 
//			redFsm->anyActionCalls() || redFsm->anyActionRets() )
		out << "} " << LABEL( "_again" ) << " {\n";

	if ( redFsm->anyToStateActions() ) {
		out <<
			"	_acts = " << OFFSET( ARR_REF( actions ), ARR_REF( toStateActions ) + "[" + vCS() + "]" ) << ";\n"
			"	_nacts = (" << UINT() << ") " << DEREF( ARR_REF ( actions ), "_acts" ) << "; _acts += 1;\n"
			"	while ( _nacts > 0 ) {\n"
			"		switch ( " << DEREF( ARR_REF( actions ), "_acts" ) << " ) {\n";
			TO_STATE_ACTION_SWITCH() <<
			"		}\n"
			"		_nacts -= 1;\n"
			"		_acts += 1;\n"
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
		out << "} " << LABEL( "_test_eof" ) << " { {}\n";

	if ( redFsm->anyEofTrans() || redFsm->anyEofActions() ) {
		out << 
			"	if ( " << P() << " == " << vEOF() << " )\n"
			"	{\n";

		if ( redFsm->anyEofTrans() ) {
			out <<
				"	if ( " << ARR_REF( eofTrans ) << "[" << vCS() << "] > 0 ) {\n"
				"		_trans = (int)" << ARR_REF( eofTrans ) << "[" << vCS() << "] - 1;\n"
				"		_cond = (" << UINT() << ")" << ARR_REF( transOffsets ) << "[_trans];\n"
				"		goto _match_cond;\n"
				"	}\n";
		}

		if ( redFsm->anyEofActions() ) {
			out <<
				"	" << INDEX( ARR_TYPE( actions ), "__acts" ) << ";\n"
				"	" << UINT() << " __nacts;\n"
				"	__acts = " << OFFSET( ARR_REF( actions ), ARR_REF( eofActions ) + "[" + vCS() + "]" ) << ";\n"
				"	__nacts = (" << UINT() << ") " << DEREF( ARR_REF( actions ), "__acts" ) << "; __acts += 1;\n"
				"	while ( __nacts > 0 ) {\n"
				"		switch ( " << DEREF( ARR_REF( actions ), "__acts" ) << " ) {\n";
				EOF_ACTION_SWITCH() <<
				"		}\n"
				"		__nacts -= 1;\n"
				"		__acts += 1;\n"
				"	}\n";
		}

		out <<
			"	}\n"
			"\n";
	}

	if ( outLabelUsed )
		out << "} " << LABEL( "_out" ) << " { {}\n";

	/* The entry loop. */
	out << "}}\n";

	out << "	}\n";
}

void FlatGotoLoop::TO_STATE_ACTION( RedStateAp *state )
{
	int act = 0;
	if ( state->toStateAction != 0 )
		act = state->toStateAction->location+1;
	toStateActions.value( act );
}

void FlatGotoLoop::FROM_STATE_ACTION( RedStateAp *state )
{
	int act = 0;
	if ( state->fromStateAction != 0 )
		act = state->fromStateAction->location+1;
	fromStateActions.value( act );
}

void FlatGotoLoop::EOF_ACTION( RedStateAp *state )
{
	int act = 0;
	if ( state->eofAction != 0 )
		act = state->eofAction->location+1;
	eofActions.value( act );
}

void FlatGotoLoop::COND_ACTION( RedCondPair *cond )
{
	/* If there are actions, emit them. Otherwise emit zero. */
	int act = 0;
	if ( cond->action != 0 )
		act = cond->action->location+1;
	condActions.value( act );
}
