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
#include "binary.h"
#include "flat.h"

void TabGoto::GOTO( ostream &ret, int gotoDest, bool inFinish )
{
	ret << OPEN_GEN_BLOCK() << vCS() << " = " << gotoDest << ";";

	if ( inFinish && !noEnd )
		EOF_CHECK( ret );

	ret << "goto _again;";
	ret << CLOSE_GEN_BLOCK();
}

void TabGoto::GOTO_EXPR( ostream &ret, GenInlineItem *ilItem, bool inFinish )
{
	ret << OPEN_GEN_BLOCK() << vCS() << " = " << OPEN_HOST_EXPR();
	INLINE_LIST( ret, ilItem->children, 0, inFinish, false );
	ret << CLOSE_HOST_EXPR() << ";";

	if ( inFinish && !noEnd )
		EOF_CHECK( ret );
	
	ret << " goto _again;";
	
	ret << CLOSE_GEN_BLOCK();
}

void TabGoto::CALL( ostream &ret, int callDest, int targState, bool inFinish )
{
	ret << OPEN_GEN_BLOCK();

	if ( red->prePushExpr != 0 ) {
		ret << OPEN_HOST_BLOCK( red->prePushExpr );
		INLINE_LIST( ret, red->prePushExpr->inlineList, 0, false, false );
		ret << CLOSE_HOST_BLOCK();
	}

	ret << STACK() << "[" << TOP() << "] = " <<
			vCS() << "; " << TOP() << " += 1;" << vCS() << " = " << 
			callDest << ";";

	if ( inFinish && !noEnd )
		EOF_CHECK( ret );

	ret << " goto _again;";

	ret << CLOSE_GEN_BLOCK();
}

void TabGoto::NCALL( ostream &ret, int callDest, int targState, bool inFinish )
{
	ret << OPEN_GEN_BLOCK();

	if ( red->prePushExpr != 0 ) {
		ret << OPEN_HOST_BLOCK( red->prePushExpr );
		INLINE_LIST( ret, red->prePushExpr->inlineList, 0, false, false );
		ret << CLOSE_HOST_BLOCK();
	}

	ret << STACK() << "[" << TOP() << "] = " << vCS() << "; " <<
			TOP() << " += 1;" << vCS() << " = " << 
			callDest << "; " << CLOSE_GEN_BLOCK();
}

void TabGoto::CALL_EXPR( ostream &ret, GenInlineItem *ilItem, int targState, bool inFinish )
{
	ret << OPEN_GEN_BLOCK();

	if ( red->prePushExpr != 0 ) {
		ret << OPEN_HOST_BLOCK( red->prePushExpr );
		INLINE_LIST( ret, red->prePushExpr->inlineList, 0, false, false );
		ret << CLOSE_HOST_BLOCK();
	}

	ret << STACK() << "[" << TOP() << "] = " <<
			vCS() << "; " << TOP() << " += 1;" << vCS() <<
			" = " << OPEN_HOST_EXPR();
	INLINE_LIST( ret, ilItem->children, targState, inFinish, false );
	ret << CLOSE_HOST_EXPR() << ";";

	if ( inFinish && !noEnd )
		EOF_CHECK( ret );

	ret << "goto _again;";

	ret << CLOSE_GEN_BLOCK();
}

void TabGoto::NCALL_EXPR( ostream &ret, GenInlineItem *ilItem, int targState, bool inFinish )
{
	ret << OPEN_GEN_BLOCK();

	if ( red->prePushExpr != 0 ) {
		ret << OPEN_HOST_BLOCK( red->prePushExpr );
		INLINE_LIST( ret, red->prePushExpr->inlineList, 0, false, false );
		ret << CLOSE_HOST_BLOCK();
	}

	ret << STACK() << "[" << TOP() << "] = " << vCS() << "; " << TOP() << " += 1;" << vCS() <<
			" = " << OPEN_HOST_EXPR();
	INLINE_LIST( ret, ilItem->children, targState, inFinish, false );
	ret << CLOSE_HOST_EXPR() << "; " << CLOSE_GEN_BLOCK();
}

void TabGoto::RET( ostream &ret, bool inFinish )
{
	ret << OPEN_GEN_BLOCK() << TOP() << " -= 1;" << vCS() << " = " << STACK() << "[" << TOP() << "];";

	if ( red->postPopExpr != 0 ) {
		ret << OPEN_HOST_BLOCK( red->postPopExpr );
		INLINE_LIST( ret, red->postPopExpr->inlineList, 0, false, false );
		ret << CLOSE_HOST_BLOCK();
	}

	if ( inFinish && !noEnd )
		EOF_CHECK( ret );

	ret << "goto _again;" << CLOSE_GEN_BLOCK();
}

void TabGoto::NRET( ostream &ret, bool inFinish )
{
	ret << OPEN_GEN_BLOCK() << TOP() << " -= 1;" << vCS() << " = " << STACK() << "[" << TOP() << "];";

	if ( red->postPopExpr != 0 ) {
		ret << OPEN_HOST_BLOCK( red->postPopExpr );
		INLINE_LIST( ret, red->postPopExpr->inlineList, 0, false, false );
		ret << CLOSE_HOST_BLOCK();
	}

	/* FIXME: ws in front of } will cause rlhc failure. */
	ret << CLOSE_GEN_BLOCK();
}

void TabGoto::BREAK( ostream &ret, int targState, bool csForced )
{
	outLabelUsed = true;
	ret << OPEN_GEN_BLOCK() << P() << " += 1; " << "goto _out; " << CLOSE_GEN_BLOCK();
}

void TabGoto::NBREAK( ostream &ret, int targState, bool csForced )
{
	outLabelUsed = true;
	ret << OPEN_GEN_BLOCK() << P() << " += 1; " << " _nbreak = 1;" << CLOSE_GEN_BLOCK();
}

void TabGoto::NFA_POP()
{
	if ( redFsm->anyNfaStates() ) {
		out <<
			"	if ( nfa_len > 0 ) {\n"
			"		nfa_count += 1;\n"
			"		nfa_len -= 1;\n"
			"		" << P() << " = nfa_bp[nfa_len].p;\n"
			;

		if ( redFsm->bAnyNfaPops ) {
			NFA_FROM_STATE_ACTION_EXEC();

			out << 
				"		int _pop_test = 1;\n"
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

					out << CEND() << "}\n";
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
				"	if ( " << P() << " == " << PE() << " )\n"
				"		goto _test_eof;\n";

			out <<
				"			goto _resume;\n"
				"		}\n";

			if ( red->nfaPostPopExpr != 0 ) {
				out <<
				"			else {\n"
				"			" << OPEN_HOST_BLOCK( red->nfaPostPopExpr );
				INLINE_LIST( out, red->nfaPostPopExpr->inlineList, 0, false, false );
				out << CLOSE_HOST_BLOCK() << "\n"
				"			}\n";
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
				"	if ( " << P() << " == " << PE() << " )\n"
				"		goto _test_eof;\n";

			out <<
				"		goto _resume;\n";
		}

		out << 
			"		goto _out;\n"
			"	}\n";
	}
}

void TabGoto::writeExec()
{
	testEofUsed = false;
	outLabelUsed = false;

	out <<
		"	{\n";

	if ( redFsm->anyRegCurStateRef() )
		out << "	int _ps;\n";

	if ( redFsm->anyEofTrans() || redFsm->anyEofActions() || red->condSpaceList.length() > 0 )
		out << "	int " << cpc << ";\n";

	if ( redFsm->anyRegNbreak() )
		out << "	int _nbreak;\n";

	DECLARE( "int", klen );
	DECLARE( INDEX( ARR_TYPE( condKeys ) ), ckeys );
	DECLARE( INDEX( ARR_TYPE( eofCondKeys ) ), cekeys );
	DECLARE( UINT(), trans, " = 0" );
	DECLARE( UINT(), cond, " = 0" );
	DECLARE( INDEX( ALPH_TYPE() ), keys );
	DECLARE( INDEX( ARR_TYPE( actions ) ), acts );
	DECLARE( INDEX( ARR_TYPE( indicies ) ), inds );
	DECLARE( UINT(), nacts );

	out <<
		"	" << ENTRY() << " {\n";

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

	FROM_STATE_ACTIONS();

	NFA_PUSH( vCS() );

	LOCATE_TRANS();

	out << "}\n";
	out << LABEL( "_match_cond" ) << " {\n";

	if ( redFsm->anyRegCurStateRef() )
		out << "	_ps = " << vCS() << ";\n";

	string condVar =
			red->condSpaceList.length() != 0 ? string(cond) : string(trans);

	out <<
		"	" << vCS() << " = " << CAST("int") << ARR_REF( condTargs ) << "[" << condVar << "];\n\n";

	if ( redFsm->anyRegActions() ) {
		out <<
			"	if ( " << ARR_REF( condActions ) << "[" << condVar << "] == 0 )\n"
			"		goto _again;\n"
			"\n";

		if ( redFsm->anyRegNbreak() )
			out << "	_nbreak = 0;\n";

		REG_ACTIONS( condVar );

		if ( redFsm->anyRegNbreak() ) {
			out <<
				"	if ( _nbreak == 1 )\n"
				"		goto _out;\n";
			outLabelUsed = true;
		}

		out << "\n";
	}

	if ( redFsm->anyRegActions() || redFsm->anyActionGotos() || 
			redFsm->anyActionCalls() || redFsm->anyActionRets() )
		out << "}\n" << LABEL( "_again" ) << " {\n";

	TO_STATE_ACTIONS();

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
		out << "}\n" << LABEL( "_test_eof" ) << " { {}\n";

	if ( redFsm->anyEofTrans() || redFsm->anyEofActions() ) {
		out << 
			"	if ( " << P() << " == " << vEOF() << " )\n"
			"	{\n";

		NFA_PUSH( vCS() );

		out <<
			"	if ( " << ARR_REF( eofCondSpaces ) << "[" << vCS() << "] != -1 ) {\n"
			"		" << cekeys << " = " << OFFSET( ARR_REF( eofCondKeys ),
						/*CAST( UINT() ) + */ ARR_REF( eofCondKeyOffs ) + "[" + vCS() + "]" ) << ";\n"
			"		" << klen << " = " << CAST( "int" ) << ARR_REF( eofCondKeyLens ) + "[" + vCS() + "]" << ";\n"
			"		" << cpc << " = 0;\n"
		;

		if ( red->condSpaceList.length() > 0 )
			COND_EXEC( ARR_REF( eofCondSpaces ) + "[" + vCS() + "]" );

		COND_BIN_SEARCH( cekeys, eofCondKeys, "goto _ok;", "goto _out;" );

		out << 
			"		_ok: {}\n"
			"	}\n"
		;

		outLabelUsed = true;

		EOF_ACTIONS();

		if ( redFsm->anyEofTrans() ) {
			out <<
				"	if ( " << ARR_REF( eofTrans ) << "[" << vCS() << "] > 0 ) {\n";

			EOF_TRANS();

			out <<
				"		goto _match_cond;\n"
				"	}\n";
		}

		out << "	if ( " << vCS() << " < " << FIRST_FINAL() << " ) goto _out; ";

		out <<
			"	}\n"
			"\n";
	}

	out << "goto _out2;\n";

	if ( outLabelUsed )
		out << "}\n" << LABEL( "_out" ) << " { {}\n";

	out << "}\n";

	out << "}\n";

	NFA_POP();

	out << "}\n" << LABEL( "_out2" ) << " { {}\n";

	out << "	}\n";
}


