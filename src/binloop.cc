/*
 * Copyright 2018-2018 Adrian Thurston <thurston@colm.net>
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

#include "binloop.h"
#include "redfsm.h"
#include "gendata.h"

void BinLoop::FROM_STATE_ACTION( RedStateAp *state )
{
	int act = 0;
	if ( state->fromStateAction != 0 )
		act = state->fromStateAction->location+1;
	fromStateActions.value( act );
}

void BinLoop::TO_STATE_ACTION( RedStateAp *state )
{
	int act = 0;
	if ( state->toStateAction != 0 )
		act = state->toStateAction->location+1;
	toStateActions.value( act );
}

void BinLoop::COND_ACTION( RedCondPair *cond )
{
	int act = 0;
	if ( cond->action != 0 )
		act = cond->action->location+1;
	condActions.value( act );
}

void BinLoop::EOF_ACTION( RedStateAp *state )
{
	int act = 0;
	if ( state->eofAction != 0 )
		act = state->eofAction->location+1;
	eofActions.value( act );
}

void BinLoop::NFA_PUSH_ACTION( RedNfaTarg *targ )
{
	int act = 0;
	if ( targ->push != 0 )
		act = targ->push->actListId+1;
	nfaPushActions.value( act );
}

void BinLoop::NFA_POP_TEST( RedNfaTarg *targ )
{
	int act = 0;
	if ( targ->popTest != 0 )
		act = targ->popTest->actListId+1;
	nfaPopTrans.value( act );
}

std::ostream &BinLoop::TO_STATE_ACTION_SWITCH()
{
	/* Walk the list of functions, printing the cases. */
	for ( GenActionList::Iter act = red->actionList; act.lte(); act++ ) {
		/* Write out referenced actions. */
		if ( act->numToStateRefs > 0 ) {
			/* Write the case label, the action and the case break. */
			out << "\t " << CASE( STR( act->actionId ) ) << " {\n";
			ACTION( out, act, IlOpts( 0, false, false ) );
			out << "\n\t" << CEND() << "}\n";
		}
	}

	return out;
}

void BinLoop::TO_STATE_ACTIONS()
{
	if ( redFsm->anyToStateActions() ) {
		out <<
			"	_acts = " << OFFSET( ARR_REF( actions ), ARR_REF( toStateActions ) +
					"[" + vCS() + "]" ) << ";\n"
			"	_nacts = " << CAST(UINT()) << DEREF( ARR_REF( actions ), "_acts" ) << ";\n"
			"	_acts += 1;\n"
			"	while ( _nacts > 0 ) {\n"
			"		switch ( " << DEREF( ARR_REF( actions ), "_acts" ) << " ) {\n";
			TO_STATE_ACTION_SWITCH() <<
			"		}\n"
			"		_nacts -= 1;\n"
			"		_acts += 1;\n"
			"	}\n"
			"\n";
	}
}

std::ostream &BinLoop::FROM_STATE_ACTION_SWITCH()
{
	/* Walk the list of functions, printing the cases. */
	for ( GenActionList::Iter act = red->actionList; act.lte(); act++ ) {
		/* Write out referenced actions. */
		if ( act->numFromStateRefs > 0 ) {
			/* Write the case label, the action and the case break. */
			out << "\t " << CASE( STR( act->actionId ) ) << " {\n";
			ACTION( out, act, IlOpts( 0, false, false ) );
			out << "\n\t" << CEND() << "}\n";
		}
	}

	return out;
}

std::ostream &BinLoop::EOF_ACTION_SWITCH()
{
	/* Walk the list of functions, printing the cases. */
	for ( GenActionList::Iter act = red->actionList; act.lte(); act++ ) {
		/* Write out referenced actions. */
		if ( act->numEofRefs > 0 ) {
			/* Write the case label, the action and the case break. */
			out << "\t " << CASE( STR( act->actionId ) ) << " {\n";
			ACTION( out, act, IlOpts( 0, true, false ) );
			out << "\n\t" << CEND() << "}\n";
		}
	}

	return out;
}


std::ostream &BinLoop::ACTION_SWITCH()
{
	/* Walk the list of functions, printing the cases. */
	for ( GenActionList::Iter act = red->actionList; act.lte(); act++ ) {
		/* Write out referenced actions. */
		if ( act->numTransRefs > 0 ) {
			/* Write the case label, the action and the case break. */
			out << "\t " << CASE( STR( act->actionId ) ) << " {\n";
			ACTION( out, act, IlOpts( 0, false, false ) );
			out << "\n\t" << CEND() << "}\n";
		}
	}

	return out;
}


void BinLoop::NFA_FROM_STATE_ACTION_EXEC()
{
	if ( redFsm->anyFromStateActions() ) {
		out <<
			"	_acts = " << OFFSET( ARR_REF( actions ), ARR_REF( fromStateActions ) + "[nfa_bp[nfa_len].state]" ) << ";\n"
			"	_nacts = " << CAST( UINT() ) << DEREF( ARR_REF( actions ), "_acts" ) << ";\n"
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
}

void BinLoop::FROM_STATE_ACTIONS()
{
	if ( redFsm->anyFromStateActions() ) {
		out <<
			"	_acts = " << OFFSET( ARR_REF( actions ),  ARR_REF( fromStateActions ) +
					"[" + vCS() + "]" ) << ";\n"
			"	_nacts = " << CAST(UINT()) << DEREF( ARR_REF( actions ), "_acts" ) << ";\n"
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
}

void BinLoop::REG_ACTIONS()
{
	out <<
		"	_acts = " << OFFSET( ARR_REF( actions ), ARR_REF( condActions ) + "[_cond]" ) << ";\n"
		"	_nacts = " << CAST( UINT() ) << DEREF( ARR_REF( actions ),  "_acts" ) << ";\n"
		"	_acts += 1;\n"
		"	while ( _nacts > 0 )\n	{\n"
		"		switch ( " << DEREF( ARR_REF( actions ), "_acts" ) << " )\n"
		"		{\n";
		ACTION_SWITCH() <<
		"		}\n"
		"		_nacts -= 1;\n"
		"		_acts += 1;\n"
		"	}\n"
		"\n";
}

void BinLoop::EOF_ACTIONS()
{
	if ( redFsm->anyEofActions() ) {
		out <<
			"	" << INDEX( ARR_TYPE( actions ), "__acts" ) << ";\n"
			"	" << UINT() << " __nacts;\n"
			"	__acts = " << OFFSET( ARR_REF( actions ), ARR_REF( eofActions ) + "[" + vCS() + "]" ) << ";\n"
			"	__nacts = " << CAST(UINT()) << DEREF( ARR_REF( actions ), "__acts" ) << ";\n"
			"	__acts += 1;\n"
			"	while ( __nacts > 0 ) {\n"
			"		switch ( " << DEREF( ARR_REF( actions ), "__acts" ) << " ) {\n";
			EOF_ACTION_SWITCH() <<
			"		}\n"
			"		__nacts -= 1;\n"
			"		__acts += 1;\n"
			"	}\n";
	}
}

