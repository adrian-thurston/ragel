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
#include "gotoloop.h"
#include "redfsm.h"
#include "bstmap.h"
#include "gendata.h"
#include "parsedata.h"
#include "inputdata.h"

std::ostream &GotoLoop::ACTION_SWITCH()
{
	/* Walk the list of functions, printing the cases. */
	for ( GenActionList::Iter act = red->actionList; act.lte(); act++ ) {
		/* Write out referenced actions. */
		if ( act->numTransRefs > 0 ) {
			/* Write the case label, the action and the case break. */
			out << "\t" << CASE( STR( act->actionId ) ) << "{\n";
			ACTION( out, act, IlOpts( 0, false, false ) );
			out << "\n\t" << CEND() << "\n}\n";
		}
	}

	return out;
}

std::ostream &GotoLoop::EOF_ACTION_SWITCH()
{
	/* Walk the list of functions, printing the cases. */
	for ( GenActionList::Iter act = red->actionList; act.lte(); act++ ) {
		/* Write out referenced actions. */
		if ( act->numEofRefs > 0 ) {
			/* Write the case label, the action and the case break. */
			out << "\t" << CASE( STR( act->actionId ) ) << "{\n";
			ACTION( out, act, IlOpts( 0, true, false ) );
			out << "\n\t" << CEND() << "\n}\n";
		}
	}

	return out;
}

std::ostream &GotoLoop::FROM_STATE_ACTION_SWITCH()
{
	/* Walk the list of functions, printing the cases. */
	for ( GenActionList::Iter act = red->actionList; act.lte(); act++ ) {
		/* Write out referenced actions. */
		if ( act->numFromStateRefs > 0 ) {
			/* Write the case label, the action and the case break. */
			out << "\t" << CASE( STR( act->actionId ) ) << "{\n";
			ACTION( out, act, IlOpts( 0, false, false ) );
			out << "\n\t" << CEND() << "\n}\n";
		}
	}

	return out;
}

std::ostream &GotoLoop::TO_STATE_ACTION_SWITCH()
{
	/* Walk the list of functions, printing the cases. */
	for ( GenActionList::Iter act = red->actionList; act.lte(); act++ ) {
		/* Write out referenced actions. */
		if ( act->numToStateRefs > 0 ) {
			/* Write the case label, the action and the case break. */
			out << "\t" << CASE( STR( act->actionId ) ) << "{\n";
			ACTION( out, act, IlOpts( 0, false, false ) );
			out << "\n\t" << CEND() << "\n}\n";
		}
	}

	return out;
}

void GotoLoop::NFA_PUSH_ACTION( RedNfaTarg *targ )
{
	int act = 0;
	if ( targ->push != 0 )
		act = targ->push->actListId+1;
	nfaPushActions.value( act );
}

void GotoLoop::NFA_POP_TEST( RedNfaTarg *targ )
{
	int act = 0;
	if ( targ->popTest != 0 )
		act = targ->popTest->actListId+1;
	nfaPopTrans.value( act );
}

std::ostream &GotoLoop::EXEC_FUNCS()
{
	/* Make labels that set acts and jump to execFuncs. Loop func indices. */
	for ( GenActionTableMap::Iter redAct = redFsm->actionMap; redAct.lte(); redAct++ ) {
		if ( redAct->numTransRefs > 0 ) {
			out << "	f" << redAct->actListId << ": " <<
				"" << acts << " = " << OFFSET( ARR_REF( actions ), itoa( redAct->location+1 ) ) << ";"
				" goto execFuncs;\n";
		}
	}

	out <<
		"\n"
		"execFuncs:\n";

	if ( redFsm->anyRegNbreak() )
		out << nbreak << " = 0;\n";

	out <<
		"	" << nacts << " = " << CAST( UINT() ) << DEREF( ARR_REF( actions ), "" + acts.ref() + "" ) << ";\n"
		"	" << acts << " += 1;\n"
		"	while ( " << nacts << " > 0 ) {\n"
		"		switch ( " << DEREF( ARR_REF( actions ), "" + acts.ref() + "" ) << " ) {\n";
		ACTION_SWITCH() << 
		"		}\n"
		"		" << acts << " += 1;\n"
		"		" << nacts << " -= 1;\n"
		"	}\n"
		"\n";

	if ( redFsm->anyRegNbreak() ) {
		out <<
			"	if ( " << nbreak << " == 1 )\n"
			"		goto " << _out << ";\n";
	}

	out <<
		"	goto _again;\n";
	return out;
}

void GotoLoop::NFA_FROM_STATE_ACTION_EXEC()
{
	if ( redFsm->anyFromStateActions() ) {
		out <<
			"	" << acts << " = " << OFFSET( ARR_REF( actions ), ARR_REF( fromStateActions ) + "[nfa_bp[nfa_len].state]" ) << ";\n"
			"	" << nacts << " = " << CAST( UINT() ) << DEREF( ARR_REF( actions ), "" + acts.ref() + "" ) << ";\n"
			"	" << acts << " += 1;\n"
			"	while ( " << nacts << " > 0 ) {\n"
			"		switch ( " << DEREF( ARR_REF( actions ), "" + acts.ref() + "" ) << " ) {\n";
			FROM_STATE_ACTION_SWITCH() <<
			"		}\n"
			"		" << nacts << " -= 1;\n"
			"		" << acts << " += 1;\n"
			"	}\n"
			"\n";
	}
}

void GotoLoop::FROM_STATE_ACTIONS()
{
	if ( redFsm->anyFromStateActions() ) {
		out <<
			"	" << acts << " = " << OFFSET( ARR_REF( actions ),
					ARR_REF( fromStateActions ) + "[" + vCS() + "]" ) << ";\n"
			"	" << nacts << " = " << CAST( UINT() ) << DEREF( ARR_REF( actions ), "" + acts.ref() + "" ) << "; " << acts << " += 1;\n"
			"	while ( " << nacts << " > 0 ) {\n"
			"		switch ( " << DEREF( ARR_REF( actions ), "" + acts.ref() + "" ) << " ) {\n";
			FROM_STATE_ACTION_SWITCH() <<
			"		}\n"
			"		" << acts << " += 1;\n"
			"		" << nacts << " -= 1;\n"
			"	}\n"
			"\n";
	}
}

void GotoLoop::TO_STATE_ACTIONS()
{
	if ( redFsm->anyToStateActions() ) {
		out <<
			"	" << acts << " = " << OFFSET( ARR_REF( actions ),
					ARR_REF( toStateActions ) + "[" + vCS() + "]" ) << ";\n"
			"	" << nacts << " = " << CAST( UINT() ) << DEREF( ARR_REF( actions ), "" + acts.ref() + "" ) << "; " << acts << " += 1;\n"
			"	while ( " << nacts << " > 0 ) {\n"
			"		switch ( " << DEREF( ARR_REF( actions ), "" + acts.ref() + "" ) << " ) {\n";
			TO_STATE_ACTION_SWITCH() <<
			"		}\n"
			"		" << acts << " += 1;\n"
			"		" << nacts << " -= 1;\n"
			"	}\n"
			"\n";
	}
}

void GotoLoop::REG_ACTIONS()
{
}

void GotoLoop::EOF_ACTIONS()
{
	if ( redFsm->anyEofActions() ) {
		out <<
			"	" << INDEX( ARR_TYPE( actions ), "__acts" ) << ";\n"
			"	" << UINT() << " __nacts;\n"
			"	__acts = " << OFFSET( ARR_REF( actions ), 
					ARR_REF( eofActions ) + "[" + vCS() + "]" ) << ";\n"
			"	__nacts = " << CAST( UINT() ) << DEREF( ARR_REF( actions ), "__acts" ) << "; __acts += 1;\n"
			"	while ( __nacts > 0 ) {\n"
			"		switch ( " << DEREF( ARR_REF( actions ), "__acts" ) << " ) {\n";
			EOF_ACTION_SWITCH() <<
			"		}\n"
			"		__acts += 1;\n"
			"		__nacts -= 1;\n"
			"	}\n";
	}
}
