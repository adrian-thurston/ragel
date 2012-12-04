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
#include "flatexp.h"
#include "redfsm.h"
#include "gendata.h"

namespace C {

void FlatExpanded::calcIndexSize()
{
	setTransPos();
	keys.setType( ALPH_TYPE() );
	keys.isSigned = keyOps->isSigned;
}


void FlatExpanded::setTableState( TableArray::State state )
{
	for ( ArrayVector::Iter i = arrayVector; i.lte(); i++ ) {
		TableArray *tableArray = *i;
		tableArray->setState( state );
	}
}

void FlatExpanded::tableDataPass()
{
	taKeys();
	taKeySpans();
}

std::ostream &FlatExpanded::TO_STATE_ACTION( RedStateAp *state )
{
	int act = 0;
	if ( state->toStateAction != 0 )
		act = state->toStateAction->actListId+1;
	out << act;
	return out;
}

std::ostream &FlatExpanded::FROM_STATE_ACTION( RedStateAp *state )
{
	int act = 0;
	if ( state->fromStateAction != 0 )
		act = state->fromStateAction->actListId+1;
	out << act;
	return out;
}

std::ostream &FlatExpanded::EOF_ACTION( RedStateAp *state )
{
	int act = 0;
	if ( state->eofAction != 0 )
		act = state->eofAction->actListId+1;
	out << act;
	return out;
}

std::ostream &FlatExpanded::COND_ACTION( RedCondAp *cond )
{
	int action = 0;
	if ( cond->action != 0 )
		action = cond->action->actListId+1;
	out << action;
	return out;
}

/* Write out the function switch. This switch is keyed on the values
 * of the func index. */
std::ostream &FlatExpanded::TO_STATE_ACTION_SWITCH()
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
std::ostream &FlatExpanded::FROM_STATE_ACTION_SWITCH()
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

std::ostream &FlatExpanded::EOF_ACTION_SWITCH()
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
std::ostream &FlatExpanded::ACTION_SWITCH()
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

void FlatExpanded::writeData()
{
	setTableState( TableArray::GeneratePass );

	taKeys();
	taKeySpans();

	OPEN_ARRAY( ARRAY_TYPE(redFsm->maxFlatIndexOffset), IO() );
	FLAT_INDEX_OFFSET();
	CLOSE_ARRAY() <<
	"\n";

	OPEN_ARRAY( ARRAY_TYPE(redFsm->maxIndex), I() );
	INDICIES();
	CLOSE_ARRAY() <<
	"\n";

	OPEN_ARRAY( ARRAY_TYPE(redFsm->maxState), TCS() );
	TRANS_COND_SPACES();
	CLOSE_ARRAY() <<
	"\n";

	OPEN_ARRAY( "int", TO() );
	TRANS_OFFSETS();
	CLOSE_ARRAY() <<
	"\n";

	OPEN_ARRAY( ARRAY_TYPE(redFsm->maxState), TL() );
	TRANS_LENGTHS();
	CLOSE_ARRAY() <<
	"\n";

	OPEN_ARRAY( "char", CK() );
	COND_KEYS();
	CLOSE_ARRAY() <<
	"\n";

	OPEN_ARRAY( ARRAY_TYPE(redFsm->maxState), CT() );
	COND_TARGS();
	CLOSE_ARRAY() <<
	"\n";

	OPEN_ARRAY( ARRAY_TYPE(redFsm->maxActionLoc), CA() );
	COND_ACTIONS();
	CLOSE_ARRAY() <<
	"\n";

	if ( redFsm->anyToStateActions() ) {
		OPEN_ARRAY( ARRAY_TYPE(redFsm->maxActionLoc),  TSA() );
		TO_STATE_ACTIONS();
		CLOSE_ARRAY() <<
		"\n";
	}

	if ( redFsm->anyFromStateActions() ) {
		OPEN_ARRAY( ARRAY_TYPE(redFsm->maxActionLoc), FSA() );
		FROM_STATE_ACTIONS();
		CLOSE_ARRAY() <<
		"\n";
	}

	if ( redFsm->anyEofActions() ) {
		OPEN_ARRAY( ARRAY_TYPE(redFsm->maxActListId), EA() );
		EOF_ACTIONS();
		CLOSE_ARRAY() <<
		"\n";
	}

	if ( redFsm->anyEofTrans() ) {
		OPEN_ARRAY( ARRAY_TYPE(redFsm->maxIndexOffset+1), ET() );
		EOF_TRANS();
		CLOSE_ARRAY() <<
		"\n";
	}

	STATE_IDS();
}

void FlatExpanded::writeExec()
{
	testEofUsed = false;
	outLabelUsed = false;

	out << 
		"	{\n"
		"	int _slen";

	if ( redFsm->anyRegCurStateRef() )
		out << ", _ps";
	
	out << ";\n";
	out << "	int _trans, _cond;\n";

	out <<
		"	const " << ALPH_TYPE() << " *_keys;\n"
		"	const " << ARRAY_TYPE(redFsm->maxIndex) << " *_inds;\n"
		"	const char *_ckeys;\n"
		"	int _klen;\n"
		"	int _cpc;\n";

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

	LOCATE_TRANS();

	out << "_match_cond:\n";

	if ( redFsm->anyEofTrans() )
		out << "_eof_trans:\n";
	
	if ( redFsm->anyRegCurStateRef() )
		out << "	_ps = " << vCS() << ";\n";

	out << 
		"	" << vCS() << " = " << CT() << "[_cond];\n\n";

	if ( redFsm->anyRegActions() ) {
		out << 
			"	if ( " << CA() << "[_cond] == 0 )\n"
			"		goto _again;\n"
			"\n"
			"	switch ( " << CA() << "[_cond] ) {\n";
			ACTION_SWITCH();
			SWITCH_DEFAULT() <<
			"	}\n"
			"\n";
	}

//	if ( redFsm->anyRegActions() || redFsm->anyActionGotos() || 
//			redFsm->anyActionCalls() || redFsm->anyActionRets() )
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
				"		_cond = " << TO() << "[_trans];\n"
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

}
