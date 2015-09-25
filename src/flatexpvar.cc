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
#include "flatexpvar.h"
#include "redfsm.h"
#include "gendata.h"

FlatExpVar::FlatExpVar( const CodeGenArgs &args ) 
:
	FlatVar(args)
{
}

/* Determine if we should use indicies or not. */
void FlatExpVar::calcIndexSize()
{
//	long long sizeWithInds =
//		indicies.size() +
//		transCondSpacesWi.size() +
//		transOffsetsWi.size() +
//		transLengthsWi.size();
//
//	long long sizeWithoutInds =
//		transCondSpaces.size() +
//		transOffsets.size() +
//		transLengths.size();
//
//	//std::cerr << "sizes: " << sizeWithInds << " " << sizeWithoutInds << std::endl;

	useIndicies = false;
}

void FlatExpVar::tableDataPass()
{
	taKeys();
	taCharClass();
	taFlatIndexOffset();

	taIndicies();
	taIndexDefaults();
	taTransCondSpaces();
	if ( condSpaceList.length() > 0 )
		taTransOffsets();
	taCondTargs();
	taCondActions();

	taToStateActions();
	taFromStateActions();
	taEofActions();
	taEofTrans();
	taNfaTargs();
	taNfaOffsets();
	taNfaPushActions();
	taNfaPopTrans();
}

void FlatExpVar::genAnalysis()
{
	redFsm->sortByStateId();

	/* Choose default transitions and the single transition. */
	redFsm->chooseDefaultSpan();
		
	/* Do flat expand. */
	redFsm->makeFlatClass();

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


void FlatExpVar::COND_ACTION( RedCondPair *cond )
{
	int action = 0;
	if ( cond->action != 0 )
		action = cond->action->actListId+1;
	condActions.value( action );
}

void FlatExpVar::TO_STATE_ACTION( RedStateAp *state )
{
	int act = 0;
	if ( state->toStateAction != 0 )
		act = state->toStateAction->actListId+1;
	toStateActions.value( act );
}

void FlatExpVar::FROM_STATE_ACTION( RedStateAp *state )
{
	int act = 0;
	if ( state->fromStateAction != 0 )
		act = state->fromStateAction->actListId+1;
	fromStateActions.value( act );
}

void FlatExpVar::EOF_ACTION( RedStateAp *state )
{
	int act = 0;
	if ( state->eofAction != 0 )
		act = state->eofAction->actListId+1;
	eofActions.value( act );
}

void FlatExpVar::NFA_PUSH_ACTION( RedNfaTarg *targ )
{
	int act = 0;
	if ( targ->push != 0 )
		act = targ->push->actListId+1;
	nfaPushActions.value( act );
}

void FlatExpVar::NFA_POP_TEST( RedNfaTarg *targ )
{
	int act = 0;
	if ( targ->popTest != 0 )
		act = targ->popTest->actListId+1;
	nfaPopTrans.value( act );
}


/* Write out the function switch. This switch is keyed on the values
 * of the func index. */
std::ostream &FlatExpVar::TO_STATE_ACTION_SWITCH()
{
	/* Loop the actions. */
	for ( GenActionTableMap::Iter redAct = redFsm->actionMap; redAct.lte(); redAct++ ) {
		if ( redAct->numToStateRefs > 0 ) {
			/* Write the entry label. */
			out << "\t " << CASE( STR( redAct->actListId+1 ) ) << " {\n";

			/* Write each action in the list of action items. */
			for ( GenActionTable::Iter item = redAct->key; item.lte(); item++ ) {
				ACTION( out, item->value, IlOpts( 0, false, false ) );
				out << "\n\t";
			}

			out << CEND() << "}\n";
		}
	}

	return out;
}

/* Write out the function switch. This switch is keyed on the values
 * of the func index. */
std::ostream &FlatExpVar::FROM_STATE_ACTION_SWITCH()
{
	/* Loop the actions. */
	for ( GenActionTableMap::Iter redAct = redFsm->actionMap; redAct.lte(); redAct++ ) {
		if ( redAct->numFromStateRefs > 0 ) {
			/* Write the entry label. */
			out << "\t " << CASE( STR( redAct->actListId+1 ) ) << " {\n";

			/* Write each action in the list of action items. */
			for ( GenActionTable::Iter item = redAct->key; item.lte(); item++ ) {
				ACTION( out, item->value, IlOpts( 0, false, false ) );
				out << "\n\t";
			}

			out << CEND() << "}\n";
		}
	}

	return out;
}

std::ostream &FlatExpVar::EOF_ACTION_SWITCH()
{
	/* Loop the actions. */
	for ( GenActionTableMap::Iter redAct = redFsm->actionMap; redAct.lte(); redAct++ ) {
		if ( redAct->numEofRefs > 0 ) {
			/* Write the entry label. */
			out << "\t " << CASE( STR( redAct->actListId+1 ) ) << " {\n";

			/* Write each action in the list of action items. */
			for ( GenActionTable::Iter item = redAct->key; item.lte(); item++ ) {
				ACTION( out, item->value, IlOpts( 0, true, false ) );
				out << "\n\t";
			}

			out << CEND() << "}\n";
		}
	}

	return out;
}

/* Write out the function switch. This switch is keyed on the values
 * of the func index. */
std::ostream &FlatExpVar::ACTION_SWITCH()
{
	/* Loop the actions. */
	for ( GenActionTableMap::Iter redAct = redFsm->actionMap; redAct.lte(); redAct++ ) {
		if ( redAct->numTransRefs > 0 ) {
			/* Write the entry label. */
			out << "\t " << CASE( STR( redAct->actListId+1 ) ) << " {\n";

			/* Write each action in the list of action items. */
			for ( GenActionTable::Iter item = redAct->key; item.lte(); item++ ) {
				ACTION( out, item->value, IlOpts( 0, false, false ) );
				out << "\n\t";
			}

			out << CEND() << "}\n";
		}
	}

	return out;
}

void FlatExpVar::writeData()
{
	taKeys();
	taCharClass();
	taFlatIndexOffset();

	taIndicies();
	taIndexDefaults();
	taTransCondSpaces();
	if ( condSpaceList.length() > 0 )
		taTransOffsets();
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

	taNfaTargs();
	taNfaOffsets();
	taNfaPushActions();
	taNfaPopTrans();

	STATE_IDS();
}

void FlatExpVar::writeExec()
{
	testEofUsed = false;
	outLabelUsed = false;

	if ( redFsm->anyNfaStates() ) {
		out << 
			"{\n"
			"	" << UINT() << " _nfa_cont = 1;\n"
			"	" << UINT() << " _nfa_repeat = 1;\n"
			"	while ( _nfa_cont != 0 )\n";
	}

	out <<
		"	{\n";
	
//	out <<
//		"	int _klen;\n";

	if ( redFsm->anyRegCurStateRef() )
		out << "	int _ps;\n";

//	out <<
//		"	" << INDEX( ALPH_TYPE(), "_keys" ) << ";\n"
//		"	" << INDEX( ARR_TYPE( condKeys ), "_ckeys" ) << ";\n"

	if ( condSpaceList.length() > 0 )
		out << "	int _cpc;\n";

	out <<
		"	" << UINT() << " _trans = 0;\n"
		"	" << UINT() << " _have = 0;\n"
		"	" << UINT() << " _cont = 1;\n";

	if ( condSpaceList.length() > 0 ) {
		out <<
			"	" << UINT() << " _cond = 0;\n";
	}

//	if ( redFsm->anyRegNbreak() )
//		out << "	int _nbreak;\n";

	if ( redFsm->classMap != 0 ) {
		out <<
			"	" << INDEX( ALPH_TYPE(), "_keys" ) << ";\n"
			"	" << INDEX( ARR_TYPE( indicies ), "_inds" ) << ";\n";
	}

	out <<
		"	while ( _cont == 1 ) {\n"
		"\n";

	if ( redFsm->errState != 0 ) {
		outLabelUsed = true;
		out << 
			"	if ( " << vCS() << " == " << redFsm->errState->id << " )\n"
			"		_cont = 0;\n";
	}

	out << 
		"_have = 0;\n";

	if ( !noEnd ) {
		out << 
			"	if ( " << P() << " == " << PE() << " ) {\n";

		if ( redFsm->anyEofTrans() || redFsm->anyEofActions() ) {
			out << 
				"	if ( " << P() << " == " << vEOF() << " )\n"
				"	{\n";

			if ( redFsm->anyEofTrans() ) {
				out <<
					"	if ( " << ARR_REF( eofTrans ) << "[" << vCS() << "] > 0 ) {\n"
					"		_trans = " << CAST( UINT() ) << ARR_REF( eofTrans ) << "[" << vCS() << "] - 1;\n";

				if ( condSpaceList.length() > 0 ) {
					out << 
						"		_cond = " << CAST( UINT() ) << ARR_REF( transOffsets ) << "[_trans];\n";
				}

				out <<
					"		_have = 1;\n"
					"	}\n";
					matchCondLabelUsed = true;
			}

			out << "if ( _have == 0 ) {\n";

			if ( redFsm->anyEofActions() ) {
				out <<
					"	switch ( " << ARR_REF( eofActions ) << "[" << vCS() << "] ) {\n";
					EOF_ACTION_SWITCH() <<
					"	}\n";
			}

			out << "}\n";
			
			out << 
				"	}\n"
				"\n";
		}

		out << 
			"	if ( _have == 0 )\n"
			"		_cont = 0;\n"
			"	}\n";

	}

	out << 
		"	if ( _cont == 1 ) {\n"
		"	if ( _have == 0 ) {\n";

	if ( redFsm->anyFromStateActions() ) {
		out <<
			"	switch ( " << ARR_REF( fromStateActions ) << "[" << vCS() << "] ) {\n";
			FROM_STATE_ACTION_SWITCH() <<
			"	}\n"
			"\n";
	}

	NFA_PUSH();

	LOCATE_TRANS();

	string cond = "_cond";
	if ( condSpaceList.length() == 0 )
		cond = "_trans";

	out << "}\n";
	
	out << "if ( _cont == 1 ) {\n";

	if ( redFsm->anyRegCurStateRef() )
		out << "	_ps = " << vCS() << ";\n";

	out <<
		"	" << vCS() << " = " << CAST("int") << ARR_REF( condTargs ) << "[" << cond << "];\n"
		"\n";

	if ( redFsm->anyRegActions() ) {
		out <<
			"	switch ( " << ARR_REF( condActions ) << "[" << cond << "] ) {\n";
			ACTION_SWITCH() <<
			"	}\n"
			"\n";
	}

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
			"		_cont = 0;\n";
	}

	out << 
		"	if ( _cont == 1 )\n"
		"		" << P() << " += 1;\n"
		"\n";

	out <<
		/* cont if. */
		"}}\n";

	/* The loop. */
	out << "}\n";

	NFA_POP();

	/* The execute block. */
	out << "	}\n";

	if ( redFsm->anyNfaStates() )
		out << "}\n";
}
