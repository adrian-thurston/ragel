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

std::string TabVar::BREAK( GotoLabel &label )
{
	return "{ _cont = 0; _again = 0; }";
}

std::string TabVar::CONTINUE( GotoLabel &label )
{
	return "{ _cont = 0; _again = 1; }";
}

std::string TabVar::BREAK_LABEL( GotoLabel &label )
{
	return "";
}

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
	ret <<
		OPEN_GEN_BLOCK() <<
			P() << "+= 1;\n" <<
			nbreak << " = 1;" <<
		CLOSE_GEN_BLOCK();
}

void TabVar::writeExec()
{
	out <<
		"{\n";

	DECLARE( INT(), ps );
	DECLARE( INT(), cpc );
	DECLARE( INT(), nbreak );
	DECLARE( INT(), klen );
	DECLARE( INDEX( ARR_TYPE( condKeys ) ), ckeys );
	DECLARE( INDEX( ARR_TYPE( eofCondKeys ) ), cekeys );
	DECLARE( UINT(), trans, " = 0" );
	DECLARE( UINT(), cond, " = 0" );
	DECLARE( INDEX( ALPH_TYPE() ), keys );
	DECLARE( INDEX( ARR_TYPE( actions ) ), acts );
	DECLARE( INDEX( ARR_TYPE( indices ) ), inds );
	DECLARE( UINT(), nacts );
	DECLARE( INT(), have );
	DECLARE( INT(), pop_test );
	DECLARE( INT(), new_recs );
	DECLARE( INT(), alt );
	DECLARE( INT(), ic );

	out << UINT() << " _have = 0;\n";
	out << UINT() << " _cont = 1;\n";
	out << UINT() << " _again = 1;\n";
	out << UINT() << " _bsc = 1;\n";
	
	out << BREAK_LABEL( _resume );

	/* Do we break out on no more input. */
	bool eof = redFsm->anyEofActivity() || redFsm->anyNfaStates();
	if ( !noEnd ) {
		if ( eof ) {
			out << 
				"       while ( _again == 1 && ( " << P() << " != " << PE() << " || " << P() << " == " << vEOF() << " ) ) {\n";
		}
		else {
			out << 
				"       while ( _again == 1 && " << P() << " != " << PE() << " ) {\n";
		}
	}
	else {
			out << 
				"       while ( _again == 1 ) {\n";

	}

	out << "_cont = 1;\n";
	out << "_again = 1;\n";

	NFA_PUSH( vCS() );

	FROM_STATE_ACTIONS();

	if ( !noEnd && eof ) {
		out << 
			"if ( " << P() << " == " << vEOF() << " ) {\n";

		if ( redFsm->anyEofTrans() || redFsm->anyEofActions() ) {
			if ( redFsm->anyEofTrans() ) {
				out <<
					"	if ( " << ARR_REF( eofTrans ) << "[" << vCS() << "] > 0 ) {\n"
					"		" << trans << " = " <<
								CAST(UINT()) << ARR_REF( eofTrans ) << "[" << vCS() << "] - 1;\n"
					"	}\n";
			}
		}

		out << 
			"}\n"
			"else {\n";
	}

	LOCATE_TRANS();

	if ( !noEnd && eof ) {
		out << 
			"}\n";
	}

	LOCATE_COND();

	if ( redFsm->anyRegCurStateRef() )
		out << "	" << ps << " = " << vCS() << ";\n";

	string condVar =
			red->condSpaceList.length() != 0 ? cond.ref() : trans.ref();

	out <<
		"	" << vCS() << " = " << CAST(INT()) << ARR_REF( condTargs ) << "[" << condVar << "];\n\n";

	if ( redFsm->anyRegActions() ) {
		out <<
			"	if ( " << ARR_REF( condActions ) << "[" << condVar << "] != 0 ) {\n"
			"\n";

		if ( redFsm->anyRegNbreak() )
			out << "	" << nbreak << " = 0;\n";

		REG_ACTIONS( condVar );

		if ( redFsm->anyRegNbreak() ) {
			out <<
				"	if ( " << nbreak << " == 1 )\n"
				"		" << BREAK( _resume ) << "\n";
		}

		out << "}\n";
	}

	out << "if ( _cont == 1 ) {\n";

	out << "\n" << EMIT_LABEL( _again );

	if ( !noEnd && eof ) {
		out << 
			"	if ( " << P() << " == " << vEOF() << " ) {\n"
			"		if ( " << vCS() << " >= " << FIRST_FINAL_STATE() << " )\n"
			"			" << BREAK( _resume ) << "\n"
			"	}\n"
			"	else {\n";
	}

	TO_STATE_ACTIONS();

	if ( redFsm->errState != 0 ) {
		out << 
			"	if ( " << vCS() << " != " << redFsm->errState->id << " ) {\n";
	}

	out << 
		"	" << P() << " += 1;\n"
		"	" << CONTINUE( _resume ) << "\n";

	if ( redFsm->errState != 0 ) {
		out << 
			"	}\n";
	}

	if ( !noEnd && eof ) {
		out <<
			"	}\n";
	}

	out << "if ( _cont == 1 ) {\n";

	if ( redFsm->anyNfaStates() ) {
		out <<
			"	if ( nfa_len == 0 )\n"
			"		" << BREAK ( _resume ) << "\n"
			"\n";

		out << "if ( _cont == 1 ) {\n";

		out <<
			"	nfa_count += 1;\n"
			"	nfa_len -= 1;\n"
			"	" << P() << " = nfa_bp[nfa_len].p;\n"
			;

		if ( redFsm->bAnyNfaPops ) {
			NFA_FROM_STATE_ACTION_EXEC();

			NFA_POP_TEST_EXEC();

			out <<
				"	if ( " << pop_test << " )\n"
				"		" << vCS() << " = nfa_bp[nfa_len].state;\n"
				"	else\n"
				"		" << vCS() << " = " << ERROR_STATE() << ";\n";
		}
		else {
			out <<
				"	" << vCS() << " = nfa_bp[nfa_len].state;\n";

		}

		NFA_POST_POP();

		/* cont */
		out << "}\n";
	}
	else {
		out << 
			"	" << BREAK( _resume ) << "\n";
	}

	/* cont */
	out << "}}\n";

	/* P loop. */
	out << "}\n";

	out << EMIT_LABEL( _out );

	/* Variable decl. */
	out << "}\n";
}

