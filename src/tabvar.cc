/*
 * Copyright 2018 Adrian Thurston <thurston@colm.net>
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

#include "tables.h"
#include "flatvar.h"
#include "binvar.h"

void TabVar::GOTO( ostream &ret, int gotoDest, bool inFinish )
{
	ret << OPEN_GEN_BLOCK() << vCS() << " = " << gotoDest << ";" << CLOSE_GEN_BLOCK();
}

void TabVar::GOTO_EXPR( ostream &ret, GenInlineItem *ilItem, bool inFinish )
{
	ret << OPEN_GEN_BLOCK() << vCS() << " = " << OPEN_HOST_EXPR( "-", 1 );
	INLINE_LIST( ret, ilItem->children, 0, inFinish, false );
	ret << CLOSE_HOST_EXPR() << ";" << CLOSE_GEN_BLOCK();
}

void TabVar::CALL( ostream &ret, int callDest, int targState, bool inFinish )
{
	red->id->error() << "cannot use fcall in -B mode" << std::endl;
	red->id->abortCompile( 1 );
}

void TabVar::NCALL( ostream &ret, int callDest, int targState, bool inFinish )
{
	ret << OPEN_GEN_BLOCK();

	if ( red->prePushExpr != 0 ) {
		ret << OPEN_HOST_BLOCK( red->prePushExpr );
		INLINE_LIST( ret, red->prePushExpr->inlineList, 0, false, false );
		ret << CLOSE_HOST_BLOCK();
	}

	ret << STACK() << "[" << TOP() << "] = " <<
			vCS() << "; " << TOP() << " += 1;" << vCS() << " = " <<
			callDest << ";" << CLOSE_GEN_BLOCK();
}

void TabVar::CALL_EXPR( ostream &ret, GenInlineItem *ilItem, int targState, bool inFinish )
{
	red->id->error() << "cannot use fcall in -B mode" << std::endl;
	red->id->abortCompile( 1 );
}

void TabVar::NCALL_EXPR( ostream &ret, GenInlineItem *ilItem, int targState, bool inFinish )
{
	ret << OPEN_GEN_BLOCK();

	if ( red->prePushExpr != 0 ) {
		ret << OPEN_HOST_BLOCK( red->prePushExpr );
		INLINE_LIST( ret, red->prePushExpr->inlineList, 0, false, false );
		ret << CLOSE_HOST_BLOCK();
	}

	ret << STACK() << "[" << TOP() << "] = " <<
			vCS() << "; " << TOP() << " += 1;" << vCS() <<
			" = " << OPEN_HOST_EXPR( "-", 1 );
	INLINE_LIST( ret, ilItem->children, targState, inFinish, false );
	ret << CLOSE_HOST_EXPR() << ";" << CLOSE_GEN_BLOCK();
}

void TabVar::RET( ostream &ret, bool inFinish )
{
	red->id->error() << "cannot use fret in -B mode" << std::endl;
	red->id->abortCompile( 1 );
}

void TabVar::NRET( ostream &ret, bool inFinish )
{
	ret << OPEN_GEN_BLOCK() << TOP() << "-= 1;" << vCS() << " = " <<
			STACK() << "[" << TOP() << "]; ";

	if ( red->postPopExpr != 0 ) {
		ret << OPEN_HOST_BLOCK( red->postPopExpr );
		INLINE_LIST( ret, red->postPopExpr->inlineList, 0, false, false );
		ret << CLOSE_HOST_BLOCK();
	}

	ret << CLOSE_GEN_BLOCK();
}

void TabVar::BREAK( ostream &ret, int targState, bool csForced )
{
	red->id->error() << "cannot use fbreak in -B mode" << std::endl;
	red->id->abortCompile( 1 );
}

void TabVar::NBREAK( ostream &ret, int targState, bool csForced )
{
	ret << OPEN_GEN_BLOCK() << P() << "+= 1; _cont = 0; " << CLOSE_GEN_BLOCK();
}

void TabVar::NFA_POP()
{
	if ( redFsm->anyNfaStates() ) {
		out <<
			"	" << nfa_repeat << " = 1;\n"
			"	while ( " << nfa_repeat << " ) {\n"
			"		" << nfa_repeat << " = 0;\n"
			"	if ( nfa_len > 0 ) {\n"
			"		int _pop_test = 1;\n"
			"		nfa_count += 1;\n"
			"		nfa_len -= 1;\n"
			"		" << P() << " = nfa_bp[nfa_len].p;\n"
			;

		if ( redFsm->bAnyNfaPops ) {
			NFA_FROM_STATE_ACTION_EXEC();

			out << 
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

			if ( red->nfaPostPopExpr != 0 ) {
				out << OPEN_HOST_BLOCK( red->nfaPostPopExpr );
				INLINE_LIST( out, red->nfaPostPopExpr->inlineList, 0, false, false );
				out << CLOSE_HOST_BLOCK();
			}

			out <<
				// "			goto _resume;\n"
				"			" << nfa_cont << " = 1;\n"
				"			" << nfa_repeat << " = 0;\n"
				"		}\n";

			if ( red->nfaPostPopExpr != 0 ) {
				out <<
				"			else {\n"
				"			" << OPEN_HOST_BLOCK( red->nfaPostPopExpr );
				INLINE_LIST( out, red->nfaPostPopExpr->inlineList, 0, false, false );
				out << CLOSE_HOST_BLOCK() << "\n"
				// "				goto _out;\n"
				"				" << nfa_cont << " = 0;\n"
				"				" << nfa_repeat << " = 1;\n"
				"			}\n";
			}
			else {
				out <<
				"			else {\n"
				// "				goto _out;\n"
				"				" << nfa_cont << " = 0;\n"
				"				" << nfa_repeat << " = 1;\n"
				"			}\n"
				;
			}
		}
		else {
			out <<
				"		" << vCS() << " = nfa_bp[nfa_len].state;\n";

			if ( red->nfaPostPopExpr != 0 ) {
				out << OPEN_HOST_BLOCK( red->nfaPostPopExpr );
				INLINE_LIST( out, red->nfaPostPopExpr->inlineList, 0, false, false );
				out << CLOSE_HOST_BLOCK();
			}

			out <<
				// "		goto _resume;\n"
				"		" << nfa_cont << " = 1;\n"
				"		" << nfa_repeat << " = 0;\n"
				;
		}

		out << 
			"	}\n"
			"	else {\n"
			"		" << nfa_cont << " = 0;\n"
			"		" << nfa_repeat << " = 0;\n"
			"	}\n"
			"}\n"
			;
	}
}

/*
 * 0 means stop, (goto out)
 * 1 means fall through to pop and repeat (goto pop)
 * 2 means fall through to next and repeat (no goto)
*/

void TabVar::writeExec()
{
	if ( redFsm->anyNfaStates() ) {
		out <<
			"{\n";
	}

	DECLARE( UINT(), nfa_cont,   " = 1" );
	DECLARE( UINT(), nfa_repeat, " = 1" );
	DECLARE( UINT(), nfa_test,   " = 1" );

	if ( redFsm->anyNfaStates() ) {
		out <<
			"	while ( " << nfa_cont << " != 0 )\n";
	}

	out <<
		"	{\n";

	if ( redFsm->anyRegCurStateRef() )
		out << "	int _ps;\n";

	out <<
		"	" << UINT() << " " << trans << " = 0;\n"
		"	" << UINT() << " _have = 0;\n"
		"	" << UINT() << " _cont = 1;\n";

	DECLARE( "int",  klen );
	DECLARE( UINT(), cond, " = 0" );
	DECLARE( "int", cpc );
	DECLARE( INDEX( ALPH_TYPE() ), keys );
	DECLARE( INDEX( ARR_TYPE( actions ) ), acts );
	DECLARE( UINT(), nacts );
	DECLARE( INDEX( ARR_TYPE( condKeys ) ), ckeys );
	DECLARE( INDEX( ARR_TYPE( eofCondKeys ) ), cekeys );
	DECLARE( INDEX( ARR_TYPE( indicies ) ), inds );

	out <<
		"	while ( _cont == 1 ) {\n"
		"\n";

	if ( redFsm->errState != 0 ) {
		out << 
			"	if ( " << vCS() << " == " << redFsm->errState->id << " )\n"
			"		_cont = 0;\n";
	}

	out << 
		"_have = 0;\n";

	if ( !noEnd ) {
		out << 
			"	if ( " << P() << " == " << PE() << " ) {\n"
			"		" << nfa_test << " = 0;\n"
			"		" << nfa_cont << " = 0;\n";

		if ( redFsm->anyEofTrans() || redFsm->anyEofActions() ) {
			out << 
				"	if ( " << P() << " == " << vEOF() << " )\n"
				"	{\n";

			NFA_PUSH( vCS() );

			out << UINT() << " _eofcont = 0;\n";

			out <<
				"	if ( " << ARR_REF( eofCondSpaces ) << "[" << vCS() << "] != -1 ) {\n"
				"		" << cekeys << " = " << OFFSET( ARR_REF( eofCondKeys ),
							/*CAST( UINT() ) + */ ARR_REF( eofCondKeyOffs ) + "[" + vCS() + "]" ) << ";\n"
				"		" << klen << " = " << CAST( "int" ) << ARR_REF( eofCondKeyLens ) + "[" + vCS() + "]" << ";\n"
				"		" << cpc << " = 0;\n"
			;

			if ( red->condSpaceList.length() > 0 )
				COND_EXEC( ARR_REF( eofCondSpaces ) + "[" + vCS() + "]" );

			out <<
				"	{\n"
				"		" << INDEX( ARR_TYPE( eofCondKeys ), "_lower" ) << " = " << cekeys << ";\n"
				"		" << INDEX( ARR_TYPE( eofCondKeys ), "_upper" ) << " = " << cekeys << " + " << klen << " - 1;\n"
				"		" << INDEX( ARR_TYPE( eofCondKeys ), "_mid" ) << ";\n"
				"		while ( _eofcont == 0 && _lower <= _upper ) {\n"
				"			_mid = _lower + ((_upper-_lower) >> 1);\n"
				"			if ( " << cpc << " < " << CAST( "int" ) << DEREF( ARR_REF( eofCondKeys ), "_mid" ) << " )\n"
				"				_upper = _mid - 1;\n"
				"			else if ( " << cpc << " > " << CAST("int" ) << DEREF( ARR_REF( eofCondKeys ), "_mid" ) << " )\n"
				"				_lower = _mid + 1;\n"
				"			else {\n"
				"				_eofcont = 1;\n"
				"			}\n"
				"		}\n"
				"		if ( _eofcont == 0 ) {\n"
				"			" << vCS() << " = " << ERROR_STATE() << ";\n"
				"			" << nfa_test << " = 1;\n"
				"			" << nfa_cont << " = 1;\n"
				"		}\n"
				"	}\n"
			;

			out << 
				"	}\n"
				"	else { _eofcont = 1; }\n"
			;

			out << "if ( _eofcont == 1 ) {\n";

			EOF_ACTIONS();

			out << "	}\n";

			out << "if ( _eofcont == 1 ) {\n";

			if ( redFsm->anyEofTrans() ) {
				out <<
					"	if ( " << ARR_REF( eofTrans ) << "[" << vCS() << "] > 0 ) {\n";

				EOF_TRANS();

				string condVar =
						red->condSpaceList.length() != 0 ? string(cond) : string(trans);

				out <<
					"		" << vCS() << " = " << CAST("int") << ARR_REF( condTargs ) << "[" << condVar << "];\n"
					"\n";

				out <<
					"	}\n";
			}

			out << "}\n";

			out << "}\n";
		}

		out <<
			"	if ( " << vCS() << " < " << FIRST_FINAL_STATE() << " ) {\n"
			"		" << nfa_test << " = 1;\n"
			"		" << nfa_cont << " = 1;\n"
			"	}\n";

		out << 
			"	_cont = 0;\n"
			"}\n";
	}

	out << 
		"	if ( _cont == 1 ) {\n";

	FROM_STATE_ACTIONS();

	NFA_PUSH( vCS() );

	LOCATE_TRANS();

	out << "if ( _cont == 1 ) {\n";

	if ( redFsm->anyRegCurStateRef() )
		out << "	_ps = " << vCS() << ";\n";

	string condVar =
			red->condSpaceList.length() != 0 ? string(cond) : string(trans);

	out <<
		"	" << vCS() << " = " << CAST("int") << ARR_REF( condTargs ) << "[" << condVar << "];\n"
		"\n";

	if ( redFsm->anyRegActions() ) {
		out <<
			"	if ( " << ARR_REF( condActions ) << "[" << condVar << "] != 0 ) {\n";

		REG_ACTIONS( condVar );

		out <<
			"	}\n";
	}

	/* _again: */

	TO_STATE_ACTIONS();

	if ( redFsm->errState != 0 ) {
		out << 
			"	if ( " << vCS() << " == " << redFsm->errState->id << " )\n"
			"		_cont = 0;\n";
	}

	out << 
		"	if ( _cont == 1 )\n"
		"		" << P() << " += 1;\n";

	out << "}\n";
	out << "}\n";
	out << "}\n";

	out <<
		"	if ( " << nfa_test << " == 1 ) {\n";

	NFA_POP();

	out << "}\n";
	out << "}\n";

	if ( redFsm->anyNfaStates() )
		out << "}\n";
}

