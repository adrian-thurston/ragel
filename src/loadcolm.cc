/*
 * Copyright 2006-2012 Adrian Thurston <thurston@colm.net>
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

#include <stdbool.h>
#include <string.h>
#include <iostream>

#include "loadcolm.h"
#include "gen/if2.h"

extern colm_sections colm_object;

InputLoc::InputLoc( colm_location *pcloc )
{
	if ( pcloc != 0 ) {
		fileName = pcloc->name;
		line = pcloc->line;
		col = pcloc->column;
	}
	else {
		fileName = 0;
		line = -1;
		col = -1;
	}
}

String unescape( const String &s )
{
	String out( String::Fresh(), s.length() );
	char *d = out.data;

	for ( int i = 0; i < s.length(); ) {
		if ( s[i] == '\\' ) {
			switch ( s[i+1] ) {
				case '0': *d++ = '\0'; break;
				case 'a': *d++ = '\a'; break;
				case 'b': *d++ = '\b'; break;
				case 't': *d++ = '\t'; break;
				case 'n': *d++ = '\n'; break;
				case 'v': *d++ = '\v'; break;
				case 'f': *d++ = '\f'; break;
				case 'r': *d++ = '\r'; break;
				default: *d++ = s[i+1]; break;
			}
			i += 2;
		}
		else {
			*d++ = s[i];
			i += 1;
		}
	}
	out.chop( d - out.data );
	return out;
}


struct LoadColm
:
	public BaseParser
{
	LoadColm( Compiler *pd, const char *inputFileName )
	:
		BaseParser( pd ),
		inputFileName( inputFileName )
	{}

	const char *inputFileName;

	Literal *walkLexRangeLit( lex_range_lit lexRangeLit )
	{
		Literal *literal = 0;
		switch ( lexRangeLit.prodName() ) {
		case lex_range_lit::Lit: {
			String lit = lexRangeLit.lex_lit().data();
			literal = Literal::cons( lexRangeLit.lex_lit().loc(), lit, Literal::LitString );
			break;
		}
		case lex_range_lit::Number: {
			String num = lexRangeLit.lex_num().text().c_str();
			literal = Literal::cons( lexRangeLit.lex_num().loc(), num, Literal::Number );
			break;
		}}
		return literal;
	}

	LexFactor *walkLexFactor( lex_factor lexFactor )
	{
		LexFactor *factor = 0;
		switch ( lexFactor.prodName() ) {
		case lex_factor::Literal: {
			String litString = lexFactor.lex_lit().data();
			Literal *literal = Literal::cons( lexFactor.lex_lit().loc(),
					litString, Literal::LitString );
			factor = LexFactor::cons( literal );
			break;
		}
		case lex_factor::Id: {
			String id = lexFactor.lex_id().data();
			factor = lexRlFactorName( id, lexFactor.lex_id().loc() );
			break;
		}
		case lex_factor::Range: {
			Literal *low = walkLexRangeLit( lexFactor.Low() );
			Literal *high = walkLexRangeLit( lexFactor.High() );

			Range *range = Range::cons( low, high );
			factor = LexFactor::cons( range );
			break;
		}
		case lex_factor::PosOrBlock: {
			ReOrBlock *block = walkRegOrData( lexFactor.reg_or_data() );
			factor = LexFactor::cons( ReItem::cons( block, ReItem::OrBlock ) );
			break;
		}
		case lex_factor::NegOrBlock: {
			ReOrBlock *block = walkRegOrData( lexFactor.reg_or_data() );
			factor = LexFactor::cons( ReItem::cons( block, ReItem::NegOrBlock ) );
			break;
		}
		case lex_factor::Number: {
			String number = lexFactor.lex_uint().text().c_str();
			factor = LexFactor::cons( Literal::cons( lexFactor.lex_uint().loc(), 
					number, Literal::Number ) );
			break;
		}
		case lex_factor::Hex: {
			String number = lexFactor.lex_hex().text().c_str();
			factor = LexFactor::cons( Literal::cons( lexFactor.lex_hex().loc(), 
					number, Literal::Number ) );
			break;
		}
		case lex_factor::Paren: {
			lex_expr LexExpr = lexFactor.lex_expr();
			LexExpression *expr = walkLexExpr( LexExpr );
			LexJoin *join = LexJoin::cons( expr );
			factor = LexFactor::cons( join );
			break;
		}}
		return factor;
	}

	LexFactorAug *walkLexFactorAug( lex_factor_rep LexFactorRepTree )
	{
		LexFactorRep *factorRep = walkLexFactorRep( LexFactorRepTree );
		return LexFactorAug::cons( factorRep );
	}

	LangExpr *walkCodeExpr( code_expr codeExpr, bool used = true )
	{
		LangExpr *expr = 0;

		switch ( codeExpr.prodName() ) {
		case code_expr::AmpAmp: {
			LangExpr *relational = walkCodeRelational( codeExpr.code_relational() );
			LangExpr *left = walkCodeExpr( codeExpr._code_expr() );

			InputLoc loc = codeExpr.AMPAMP().loc();
			expr = LangExpr::cons( loc, left, OP_LogicalAnd, relational );
			break;
		}
		case code_expr::BarBar: {
			LangExpr *relational = walkCodeRelational( codeExpr.code_relational() );
			LangExpr *left = walkCodeExpr( codeExpr._code_expr() );

			InputLoc loc = codeExpr.BARBAR().loc();
			expr = LangExpr::cons( loc, left, OP_LogicalOr, relational );
			break;
		}
		case code_expr::Base: {
			LangExpr *relational = walkCodeRelational( codeExpr.code_relational(), used );
			expr = relational;
			break;
		}}
		return expr;
	}

	LangStmt *walkStatement( statement Statement )
	{
		LangStmt *stmt = 0;
		switch ( Statement.prodName() ) {
		case statement::Print: {
			print_stmt printStmt = Statement.print_stmt();
			stmt = walkPrintStmt( printStmt );
			break;
		}
		case statement::Expr: {
			expr_stmt exprStmt = Statement.expr_stmt();
			stmt = walkExprStmt( exprStmt );
			break;
		}
		case statement::VarDef: {
			ObjectField *objField = walkVarDef( Statement.var_def(),
					ObjectField::UserLocalType );
			LangExpr *expr = walkOptDefInit( Statement.opt_def_init() );
			stmt = varDef( objField, expr, LangStmt::AssignType );
			break;
		}
		case statement::For: {
			pushScope();

			String forDecl = Statement.id().text().c_str();
			TypeRef *typeRef = walkTypeRef( Statement.type_ref() );
			StmtList *stmtList = walkBlockOrSingle( Statement.block_or_single() );

			IterCall *iterCall = walkIterCall( Statement.iter_call() );

			stmt = forScope( Statement.id().loc(), forDecl,
					curScope(), typeRef, iterCall, stmtList );

			popScope();
			break;
		}
		case statement::If: {
			pushScope();

			LangExpr *expr = walkCodeExpr( Statement.code_expr() );
			StmtList *stmtList = walkBlockOrSingle( Statement.block_or_single() );

			popScope();

			LangStmt *elsifList = walkElsifList( Statement.elsif_list() );
			stmt = LangStmt::cons( LangStmt::IfType, expr, stmtList, elsifList );
			break;
		}
		case statement::SwitchUnder:
		case statement::SwitchBlock: {
			pushScope();
			stmt = walkCaseClauseList( Statement.case_clause_list(), Statement.var_ref() );
			popScope();
			break;
		}
		case statement::While: {
			pushScope();
			LangExpr *expr = walkCodeExpr( Statement.code_expr() );
			StmtList *stmtList = walkBlockOrSingle( Statement.block_or_single() );
			stmt = LangStmt::cons( LangStmt::WhileType, expr, stmtList );
			popScope();
			break;
		}
		case statement::LhsVarRef: {
			LangVarRef *varRef = walkVarRef( Statement.var_ref() );
			LangExpr *expr = walkCodeExpr( Statement.code_expr() );
			stmt = LangStmt::cons( varRef->loc, LangStmt::AssignType, varRef, expr );
			break;
		}
		case statement::Yield: {
			LangVarRef *varRef = walkVarRef( Statement.var_ref() );
			stmt = LangStmt::cons( LangStmt::YieldType, varRef );
			break;
		}
		case statement::Return: {
			LangExpr *expr = walkCodeExpr( Statement.code_expr() );
			stmt = LangStmt::cons( Statement.loc(), LangStmt::ReturnType, expr );
			break;
		}
		case statement::Break: {
			stmt = LangStmt::cons( LangStmt::BreakType );
			break;
		}
		case statement::Reject: {
			stmt = LangStmt::cons( Statement.REJECT().loc(), LangStmt::RejectType );
			break;
		}}
		return stmt;
	}

	StmtList *walkLangStmtList( lang_stmt_list langStmtList )
	{
		StmtList *retList = new StmtList;
		_repeat_statement stmtList = langStmtList.StmtList();

		/* Walk the list of items. */
		while ( !stmtList.end() ) {
			statement Statement = stmtList.value();
			LangStmt *stmt = walkStatement( Statement );
			if ( stmt != 0 )
				retList->append( stmt );
			stmtList = stmtList.next();
		}

		require_pattern require = langStmtList.opt_require_stmt().require_pattern();
		if ( require != 0 ) {
			pushScope();

			LangVarRef *varRef = walkVarRef( require.var_ref() );
			PatternItemList *list = walkPattern( require.pattern(), varRef );
			LangExpr *expr = match( require.REQUIRE().loc(), varRef, list );

			StmtList *reqList = walkLangStmtList( langStmtList.opt_require_stmt().lang_stmt_list() );

			LangStmt *stmt = LangStmt::cons( LangStmt::IfType, expr, reqList, 0 );

			popScope();

			retList->append( stmt );
		}

		return retList;
	}

	void walkTokenDef( token_def TokenDef )
	{
		String name = TokenDef.id().data();

		bool niLeft = walkNoIgnoreLeft( TokenDef.no_ignore_left() );
		bool niRight = walkNoIgnoreRight( TokenDef.no_ignore_right() );

		ObjectDef *objectDef = walkVarDefList( TokenDef.VarDefList() );
		objectDef->name = name;

		LexJoin *join = 0;
		if ( TokenDef.opt_lex_expr().lex_expr() != 0 ) {
			LexExpression *expr = walkLexExpr( TokenDef.opt_lex_expr().lex_expr() );
			join = LexJoin::cons( expr );
		}

		CodeBlock *translate = walkOptTranslate( TokenDef.opt_translate() );

		defineToken( TokenDef.id().loc(), name, join, objectDef,
				translate, false, niLeft, niRight );
	}

	void walkIgnoreCollector( ic_def IgnoreCollector )
	{
		String id = IgnoreCollector.id().data();
		zeroDef( IgnoreCollector.id().loc(), id );
	}

	String walkOptId( opt_id optId )
	{
		String name;
		if ( optId.prodName() == opt_id::Id )
			name = optId.id().data();
		return name;
	}

	ObjectDef *walkVarDefList( _repeat_var_def varDefList )
	{
		ObjectDef *objectDef = ObjectDef::cons( ObjectDef::UserType,
				String(), pd->nextObjectId++ ); 

		while ( !varDefList.end() ) {
			ObjectField *varDef = walkVarDef( varDefList.value(),
					ObjectField::UserFieldType );
			objVarDef( objectDef, varDef );
			varDefList = varDefList.next();
		}

		return objectDef;
	}

	void walkPreEof( pre_eof_def PreEofDef )
	{
		ObjectDef *localFrame = blockOpen();
		StmtList *stmtList = walkLangStmtList( PreEofDef.lang_stmt_list() );
		preEof( PreEofDef.PREEOF().loc(), stmtList, localFrame );
		blockClose();
	}

	void walkIgnoreDef( ignore_def IgnoreDef )
	{
		String name = walkOptId( IgnoreDef.opt_id() );
		ObjectDef *objectDef = ObjectDef::cons( ObjectDef::UserType,
				name, pd->nextObjectId++ ); 

		LexJoin *join = 0;
		if ( IgnoreDef.opt_lex_expr().lex_expr() != 0 ) {
			LexExpression *expr = walkLexExpr( IgnoreDef.opt_lex_expr().lex_expr() );
			join = LexJoin::cons( expr );
		}

		defineToken( IgnoreDef.IGNORE().loc(), name, join, objectDef,
				0, true, false, false );
	}

	LangExpr *walkCodeMultiplicitive( code_multiplicitive mult, bool used = true )
	{
		LangExpr *expr = 0;
		switch ( mult.prodName() ) {
		case code_multiplicitive::Star: {
			LangExpr *right = walkCodeUnary( mult.code_unary() );
			LangExpr *left = walkCodeMultiplicitive( mult._code_multiplicitive() );
			expr = LangExpr::cons( mult.STAR().loc(), left, '*', right );
			break;
		}
		case code_multiplicitive::Fslash: {
			LangExpr *right = walkCodeUnary( mult.code_unary() );
			LangExpr *left = walkCodeMultiplicitive( mult._code_multiplicitive() );
			expr = LangExpr::cons( mult.FSLASH().loc(), left, '/', right );
			break;
		}
		case code_multiplicitive::Base: {
			LangExpr *right = walkCodeUnary( mult.code_unary(), used );
			expr = right;
			break;
		}}
		return expr;
	}

	PatternItemList *walkPatternElTypeOrLit( pattern_el_lel typeOrLit,
			LangVarRef *patternVarRef )
	{
		NamespaceQual *nspaceQual = walkRegionQual( typeOrLit.region_qual() );
		RepeatType repeatType = walkOptRepeat( typeOrLit.opt_repeat() );

		PatternItemList *list = 0;
		switch ( typeOrLit.prodName() ) {
		case pattern_el_lel::Id: {
			String id = typeOrLit.id().data();
			list = patternElNamed( typeOrLit.id().loc(), patternVarRef,
					nspaceQual, id, repeatType );
			break;
		}
		case pattern_el_lel::Lit: {
			String lit = typeOrLit.backtick_lit().data();
			list = patternElType( typeOrLit.backtick_lit().loc(), patternVarRef,
					nspaceQual, lit, repeatType );
			break;
		}}

		return list;
	}

	LangVarRef *walkOptLabel( opt_label optLabel )
	{
		LangVarRef *varRef = 0;
		if ( optLabel.prodName() == opt_label::Id ) {
			String id = optLabel.id().data();
			varRef = LangVarRef::cons( optLabel.id().loc(),
					curNspace(), curStruct(), curScope(), id );
		}
		return varRef;
	}

	PatternItemList *walkPatternEl( pattern_el patternEl, LangVarRef *patternVarRef )
	{
		PatternItemList *list = 0;
		switch ( patternEl.prodName() ) {
		case pattern_el::Dq: {
			list = walkLitpatElList( patternEl.LitpatElList(),
					patternEl.dq_lit_term().LIT_DQ_NL(), patternVarRef );
			break;
		}
		case pattern_el::Sq: {
			list = walkPatSqConsDataList( patternEl.SqConsDataList(),
					patternEl.sq_lit_term().CONS_SQ_NL() );
			break;
		}
		case pattern_el::Tilde: {
			String patternData = patternEl.opt_tilde_data().text().c_str();
			patternData += '\n';
			PatternItem *patternItem = PatternItem::cons( PatternItem::InputTextForm,
					patternEl.opt_tilde_data().loc(), patternData );
			list = PatternItemList::cons( patternItem );
			break;
		}
		case pattern_el::PatternEl: {
			PatternItemList *typeOrLitList = walkPatternElTypeOrLit(
					patternEl.pattern_el_lel(), patternVarRef );
			LangVarRef *varRef = walkOptLabel( patternEl.opt_label() );
			list = consPatternEl( varRef, typeOrLitList );
			break;
		}}
		return list;
	}

	PatternItemList *walkLitpatEl( litpat_el litpatEl, LangVarRef *patternVarRef )
	{
		PatternItemList *list = 0;
		switch ( litpatEl.prodName() ) {
		case litpat_el::ConsData: {
			String consData = unescape( litpatEl.lit_dq_data().text().c_str() );
			PatternItem *patternItem = PatternItem::cons( PatternItem::InputTextForm,
					litpatEl.lit_dq_data().loc(), consData );
			list = PatternItemList::cons( patternItem );
			break;
		}
		case litpat_el::SubList: {
			list = walkPatternElList( litpatEl.PatternElList(), patternVarRef );
			break;
		}}
		return list;
	}

	PatternItemList *walkPatSqConsDataList( _repeat_sq_cons_data sqConsDataList, CONS_SQ_NL Nl )
	{
		PatternItemList *list = new PatternItemList;
		while ( !sqConsDataList.end() ) {
			String consData = unescape( sqConsDataList.value().text().c_str() );
			PatternItem *patternItem = PatternItem::cons( PatternItem::InputTextForm,
					sqConsDataList.value().loc(), consData );
			PatternItemList *tail = PatternItemList::cons( patternItem );
			list = patListConcat( list, tail );

			sqConsDataList = sqConsDataList.next();
		}

		if ( Nl != 0 ) {
			String nl = unescape( Nl.data() );
			PatternItem *patternItem = PatternItem::cons( PatternItem::InputTextForm,
					Nl.loc(), nl );
			PatternItemList *term = PatternItemList::cons( patternItem );
			list = patListConcat( list, term );
		}

		return list;
	}

	ConsItemList *walkConsSqConsDataList( _repeat_sq_cons_data sqConsDataList, CONS_SQ_NL Nl )
	{
		ConsItemList *list = new ConsItemList;
		while ( !sqConsDataList.end() ) {
			String consData = unescape( sqConsDataList.value().text().c_str() );
			ConsItem *consItem = ConsItem::cons(
					sqConsDataList.value().loc(), ConsItem::InputText, consData );
			ConsItemList *tail = ConsItemList::cons( consItem );
			list = consListConcat( list, tail );

			sqConsDataList = sqConsDataList.next();
		}

		if ( Nl != 0 ) {
			String nl = unescape( Nl.data() );
			ConsItem *consItem = ConsItem::cons(
					Nl.loc(), ConsItem::InputText, nl );
			ConsItemList *term = ConsItemList::cons( consItem );
			list = consListConcat( list, term );
		}

		return list;
	}

	PatternItemList *walkLitpatElList( _repeat_litpat_el litpatElList, LIT_DQ_NL Nl,
			LangVarRef *patternVarRef )
	{
		PatternItemList *list = new PatternItemList;
		while ( !litpatElList.end() ) {
			PatternItemList *tail = walkLitpatEl( litpatElList.value(), patternVarRef );
			list = patListConcat( list, tail );
			litpatElList = litpatElList.next();
		}

		if ( Nl != 0 ) {
			String nl = unescape( Nl.data() );
			PatternItem *patternItem = PatternItem::cons( PatternItem::InputTextForm,
					Nl.loc(), nl );
			PatternItemList *term = PatternItemList::cons( patternItem );
			list = patListConcat( list, term );
		}

		return list;
	}

	PatternItemList *walkPatternElList( _repeat_pattern_el patternElList,
			LangVarRef *patternVarRef )
	{
		PatternItemList *list = new PatternItemList;
		while ( !patternElList.end() ) {
			PatternItemList *tail = walkPatternEl( patternElList.value(), patternVarRef );
			list = patListConcat( list, tail );
			patternElList = patternElList.next();
		}
		return list;
	}

	PatternItemList *walkPattternTopEl( pattern_top_el patternTopEl,
			LangVarRef *patternVarRef )
	{
		PatternItemList *list = 0;
		switch ( patternTopEl.prodName() ) {
		case pattern_top_el::Dq: {
			list = walkLitpatElList( patternTopEl.LitpatElList(),
					patternTopEl.dq_lit_term().LIT_DQ_NL(), patternVarRef );
			break;
		}
		case pattern_top_el::Sq: {
			list = walkPatSqConsDataList( patternTopEl.SqConsDataList(),
					patternTopEl.sq_lit_term().CONS_SQ_NL() );
			break;
		}
		case pattern_top_el::Tilde: {
			String patternData = patternTopEl.opt_tilde_data().text().c_str();
			patternData += '\n';
			PatternItem *patternItem = PatternItem::cons( PatternItem::InputTextForm,
					patternTopEl.opt_tilde_data().loc(), patternData );
			list = PatternItemList::cons( patternItem );
			break;
		}
		case pattern_top_el::SubList: {
			list = walkPatternElList( patternTopEl.PatternElList(), patternVarRef );
			break;
		}}
		return list;
	}

	PatternItemList *walkPatternList( pattern_list patternList, LangVarRef *patternVarRef )
	{
		PatternItemList *list = 0;
		switch ( patternList.prodName() ) {
		case pattern_list::List: {

			PatternItemList *left = walkPatternList(
					patternList._pattern_list(), patternVarRef );

			PatternItemList *right = walkPattternTopEl(
					patternList.pattern_top_el(), patternVarRef );

			list = patListConcat( left, right );
			break;
		}
		case pattern_list::Base: {
			list = walkPattternTopEl( patternList.pattern_top_el(), patternVarRef );
			break;
		}}
		return list;
	}

	PatternItemList *walkPattern( pattern Pattern, LangVarRef *patternVarRef )
	{
		return walkPatternList( Pattern.pattern_list(), patternVarRef );
	}

	LangExpr *walkOptDefInit( opt_def_init optDefInit )
	{
		LangExpr *expr = 0;
		if ( optDefInit.prodName() == opt_def_init::Init )
			expr = walkCodeExpr( optDefInit.code_expr() );
		return expr;
	}

	LangStmt *walkExportDef( export_def exportDef )
	{
		ObjectField *objField = walkVarDef( exportDef.var_def(),
				ObjectField::StructFieldType );
		LangExpr *expr = walkOptDefInit( exportDef.opt_def_init() );

		return exportStmt( objField, LangStmt::AssignType, expr );
	}

	LangStmt *walkGlobalDef( global_def GlobalDef )
	{
		ObjectField *objField = walkVarDef( GlobalDef.var_def(),
				ObjectField::StructFieldType );
		LangExpr *expr = walkOptDefInit( GlobalDef.opt_def_init() );

		return globalDef( objField, expr, LangStmt::AssignType );
	}

	void walkAliasDef( alias_def aliasDef )
	{
		String id = aliasDef.id().data();
		TypeRef *typeRef = walkTypeRef( aliasDef.type_ref() );
		alias( aliasDef.id().loc(), id, typeRef );
	}

	CodeBlock *walkOptTranslate( opt_translate optTranslate )
	{
		CodeBlock *block = 0;
		if ( optTranslate.prodName() == opt_translate::Translate ) {
			ObjectDef *localFrame = blockOpen();
			StmtList *stmtList = walkLangStmtList( optTranslate.lang_stmt_list() );
			block = CodeBlock::cons( stmtList, localFrame );
			block->context = curStruct();
			blockClose();
		}
		return block;
	}

	PredDecl *walkPredToken( pred_token predToken )
	{
		NamespaceQual *nspaceQual = walkRegionQual( predToken.region_qual() );
		PredDecl *predDecl = 0;
		switch ( predToken.prodName() ) {
		case pred_token::Id: {
			String id = predToken.id().data();
			predDecl = predTokenName( predToken.id().loc(), nspaceQual, id );
			break;
		}
		case pred_token::Lit: {
			String lit = predToken.backtick_lit().data();
			predDecl = predTokenLit( predToken.backtick_lit().loc(), lit, nspaceQual );
			break;
		}}
		return predDecl;
	}

	PredDeclList *walkPredTokenList( pred_token_list predTokenList )
	{
		PredDeclList *list = 0;
		switch ( predTokenList.prodName() ) {
		case pred_token_list::List: {
			list = walkPredTokenList( predTokenList._pred_token_list() );
			PredDecl *predDecl = walkPredToken( predTokenList.pred_token() );
			list->append( predDecl );
			break;
		}
		case pred_token_list::Base: {
			PredDecl *predDecl = walkPredToken( predTokenList.pred_token() );
			list = new PredDeclList;
			list->append( predDecl );
			break;
		}}
		return list;
	}

	PredType walkPredType( pred_type predType )
	{
		PredType pt = PredLeft;
		switch ( predType.prodName() ) {
		case pred_type::Left:
			pt = PredLeft;
			break;
		case pred_type::Right:
			pt = PredRight;
			break;
		case pred_type::NonAssoc:
			pt = PredNonassoc;
			break;
		}

		return pt;
	}

	void walkPrecedenceDef( precedence_def precedenceDef )
	{
		PredType predType = walkPredType( precedenceDef.pred_type() );
		PredDeclList *predDeclList = walkPredTokenList(
				precedenceDef.pred_token_list() );
		precedenceStmt( predType, predDeclList );
	}

	StmtList *walkInclude( include Include )
	{
		String lit = "";
		_repeat_sq_cons_data sqConsDataList = Include.SqConsDataList();
		while ( !sqConsDataList.end() ) {
			colm_data *data = sqConsDataList.value().data();
			lit.append( data->data, data->length );
			sqConsDataList = sqConsDataList.next();
		}

		String file = unescape( lit );

		/* Check if we can open the input file for reading. */
		if ( ! readCheck( file.data ) ) {

			bool found = false;
			for ( ArgsVector::Iter av = includePaths; av.lte(); av++ ) {
				String path = String( *av ) + "/" + file;
				if ( readCheck( path.data ) ) {
					found = true;
					file = path;
					break;
				}
			}

			if ( !found )
				error() << "could not open " << file.data << " for reading" << endp;
		}

		const char *argv[3];
		argv[0] = "load-include";
		argv[1] = file.data;
		argv[2] = 0;

		colm_program *program = colm_new_program( &colm_object );
		colm_run_program( program, 2, argv );

		/* Extract the parse tree. */
		start Start = ColmTree( program );
		str Error = ColmError( program );

		if ( Start == 0 ) {
			gblErrorCount += 1;
			InputLoc loc = Error.loc();
			error(loc) << file.data << ": parse error: " << Error.text() << std::endl;
			return 0;
		}

		StmtList *stmtList = walkRootItemList( Start.RootItemList() );
		pd->streamFileNames.append( colm_extract_fns( program ) );
		colm_delete_program( program );
		return stmtList;
	}


	NamespaceQual *walkRegionQual( region_qual regionQual )
	{
		NamespaceQual *qual = 0;
		switch ( regionQual.prodName() ) {
		case region_qual::Qual: {
			qual = walkRegionQual( regionQual._region_qual() );
			qual->qualNames.append( String( regionQual.id().data() ) );
			break;
		}
		case region_qual::Base: {
			qual = NamespaceQual::cons( curNspace() );
			break;
		}}
		return qual;
	}

	RepeatType walkOptRepeat( opt_repeat OptRepeat )
	{
		RepeatType repeatType = RepeatNone;
		switch ( OptRepeat.prodName() ) {
		case opt_repeat::Star:
			repeatType = RepeatRepeat;
			break;
		case opt_repeat::Plus:
			repeatType = RepeatList;
			break;
		case opt_repeat::Question:
			repeatType = RepeatOpt;
			break;
		}
		return repeatType;
	}

	TypeRef *walkValueList( type_ref typeRef )
	{
		TypeRef *valType = walkTypeRef( typeRef._type_ref() );
		TypeRef *elType = TypeRef::cons( typeRef.loc(), TypeRef::ListEl, valType );
		return TypeRef::cons( typeRef.loc(), TypeRef::List, 0, elType, valType );
	}

	TypeRef *walkListEl( type_ref typeRef )
	{
		TypeRef *valType = walkTypeRef( typeRef._type_ref() );
		return TypeRef::cons( typeRef.loc(), TypeRef::ListEl, valType );
	}

	TypeRef *walkValueMap( type_ref typeRef )
	{
		TypeRef *keyType = walkTypeRef( typeRef.KeyType() );
		TypeRef *valType = walkTypeRef( typeRef.ValType() );
		TypeRef *elType = TypeRef::cons( typeRef.loc(),
				TypeRef::MapEl, 0, keyType, valType );

		return TypeRef::cons( typeRef.loc(), TypeRef::Map, 0,
				keyType, elType, valType );
	}

	TypeRef *walkMapEl( type_ref typeRef )
	{
		TypeRef *keyType = walkTypeRef( typeRef.KeyType() );
		TypeRef *valType = walkTypeRef( typeRef.ValType() );

		return TypeRef::cons( typeRef.loc(), TypeRef::MapEl, 0, keyType, valType );
	}

	TypeRef *walkTypeRef( type_ref typeRef )
	{
		TypeRef *tr = 0;
		switch ( typeRef.prodName() ) {
		case type_ref::Id: {
			NamespaceQual *nspaceQual = walkRegionQual( typeRef.region_qual() );
			String id = typeRef.id().data();
			RepeatType repeatType = walkOptRepeat( typeRef.opt_repeat() );
			tr = TypeRef::cons( typeRef.id().loc(), nspaceQual, id, repeatType );
			break;
		}
		case type_ref::Int: {
			tr = TypeRef::cons( internal, pd->uniqueTypeInt );
			break;
		}
		case type_ref::Bool: {
			tr = TypeRef::cons( internal, pd->uniqueTypeBool );
			break;
		}
		case type_ref::Parser: {
			TypeRef *type = walkTypeRef( typeRef._type_ref() );
			tr = TypeRef::cons( typeRef.loc(), TypeRef::Parser, 0, type, 0 );
			break;
		}
		case type_ref::List: {
			tr = walkValueList( typeRef );
			break;
		}
		case type_ref::Map: {
			tr = walkValueMap( typeRef );
			break;
		}
		case type_ref::ListEl: {
			tr = walkListEl( typeRef );
			break;
		}
		case type_ref::MapEl: {
			tr = walkMapEl( typeRef );
			break;
		}}
		return tr;
	}

	StmtList *walkBlockOrSingle( block_or_single blockOrSingle )
	{
		StmtList *stmtList = 0;
		switch ( blockOrSingle.prodName() ) {
		case block_or_single::Single: {
			stmtList = new StmtList;
			LangStmt *stmt = walkStatement( blockOrSingle.statement() );
			stmtList->append( stmt );
			break;
		}
		case block_or_single::Block: {
			stmtList = walkLangStmtList( blockOrSingle.lang_stmt_list() );
			break;
		}}

		return stmtList;
	}

	void walkProdEl( const String &defName, ProdElList *list, prod_el El )
	{
		ObjectField *captureField = 0;
		if ( El.opt_prod_el_name().prodName() == opt_prod_el_name::Name ) {
			String fieldName = El.opt_prod_el_name().id().data();
			captureField = ObjectField::cons( El.opt_prod_el_name().id().loc(),
				ObjectField::RhsNameType, 0, fieldName );
		}
		else {
			/* default the prod name. */
			if ( El.prodName() == prod_el::Id ) {
				String fieldName = El.id().data();
				opt_repeat::prod_name orpn = El.opt_repeat().prodName();
				if ( orpn == opt_repeat::Star )
					fieldName = "_repeat_" + fieldName;
				else if ( orpn == opt_repeat::Plus )
					fieldName = "_list_" + fieldName;
				else if ( orpn == opt_repeat::Question )
					fieldName = "_opt_" + fieldName;
				else if ( strcmp( fieldName, defName ) == 0 )
					fieldName = "_" + fieldName;
				captureField = ObjectField::cons( El.id().loc(),
						ObjectField::RhsNameType, 0, fieldName );
			}
		}

		RepeatType repeatType = walkOptRepeat( El.opt_repeat() );
		NamespaceQual *nspaceQual = walkRegionQual( El.region_qual() );

		switch ( El.prodName() ) {
		case prod_el::Id: {
			String typeName = El.id().data();
			ProdEl *prodEl = prodElName( El.id().loc(), typeName,
					nspaceQual, captureField, repeatType, false );
			appendProdEl( list, prodEl );
			break;
		}
		case prod_el::Lit: {
			String lit = El.backtick_lit().data();
			ProdEl *prodEl = prodElLiteral( El.backtick_lit().loc(), lit,
					nspaceQual, captureField, repeatType, false );
			appendProdEl( list, prodEl );
			break;
		}}
	}

	void walkProdElList( const String &defName, ProdElList *list, prod_el_list ProdElList )
	{
		if ( ProdElList.prodName() == prod_el_list::List ) {
			prod_el_list RightProdElList = ProdElList._prod_el_list();
			walkProdElList( defName, list, RightProdElList );
			walkProdEl( defName, list, ProdElList.prod_el() );
		}
	}

	CodeBlock *walkOptReduce( opt_reduce OptReduce )
	{
		CodeBlock *block = 0;
		if ( OptReduce.prodName() == opt_reduce::Reduce ) {
			ObjectDef *localFrame = blockOpen();
			StmtList *stmtList = walkLangStmtList( OptReduce.lang_stmt_list() );

			block = CodeBlock::cons( stmtList, localFrame );
			block->context = curStruct();

			blockClose();
		}
		return block;
	}

	void walkProdudction( const String &defName, LelDefList *lelDefList, prod Prod )
	{
		ProdElList *list = new ProdElList;

		walkProdElList( defName, list, Prod.prod_el_list() );

		String name;
		if ( Prod.opt_prod_name().prodName() == opt_prod_name::Name )
			name = Prod.opt_prod_name().id().data();

		CodeBlock *codeBlock = walkOptReduce( Prod.opt_reduce() );
		bool commit = Prod.opt_commit().prodName() == opt_commit::Commit;

		Production *prod = BaseParser::production( Prod.SQOPEN().loc(),
				list, name, commit, codeBlock, 0 );
		prodAppend( lelDefList, prod );
	}

	void walkProdList( const String &name, LelDefList *lelDefList, prod_list ProdList )
	{
		if ( ProdList.prodName() == prod_list::List ) 
			walkProdList( name, lelDefList, ProdList._prod_list() );

		walkProdudction( name, lelDefList, ProdList.prod() );
	}

	ReOrItem *walkRegOrChar( reg_or_char regOrChar )
	{
		ReOrItem *orItem = 0;
		switch ( regOrChar.prodName() ) {
		case reg_or_char::Char: {
			String c = unescape( regOrChar.RE_CHAR().data() );
			orItem = ReOrItem::cons( regOrChar.RE_CHAR().loc(), c );
			break;
		}
		case reg_or_char::Range: {
			String low = unescape( regOrChar.Low().data() );
			String high = unescape( regOrChar.High().data() );
			orItem = ReOrItem::cons( regOrChar.Low().loc(), low[0], high[0] );
			break;
		}}
		return orItem;
	}

	ReOrBlock *walkRegOrData( reg_or_data regOrData )
	{
		ReOrBlock *block = 0;
		switch ( regOrData.prodName() ) {
		case reg_or_data::Data: {
			ReOrBlock *left = walkRegOrData( regOrData._reg_or_data() );
			ReOrItem *right = walkRegOrChar( regOrData.reg_or_char() );
			block = lexRegularExprData( left, right );
			break;
		}
		case reg_or_data::Base: {
			block = ReOrBlock::cons();
			break;
		}}
		return block;
	}

	LexFactorNeg *walkLexFactorNeg( lex_factor_neg lexFactorNeg )
	{
		LexFactorNeg *factorNeg = 0;
		switch ( lexFactorNeg.prodName() ) {
		case lex_factor_neg::Caret: {
			LexFactorNeg *recNeg = walkLexFactorNeg( lexFactorNeg._lex_factor_neg() );
			factorNeg = LexFactorNeg::cons( recNeg, LexFactorNeg::CharNegateType );
			break;
		}
		case lex_factor_neg::Base: {
			LexFactor *factor = walkLexFactor( lexFactorNeg.lex_factor() );
			factorNeg = LexFactorNeg::cons( factor );
			break;
		}}
		return factorNeg;
	}

	LexFactorRep *walkLexFactorRep( lex_factor_rep lexFactorRep )
	{
		LexFactorRep *factorRep = 0;
		LexFactorRep *recRep = 0;
		lex_factor_rep::prod_name pn = lexFactorRep.prodName();

		if ( pn != lex_factor_rep::Base )
			recRep = walkLexFactorRep( lexFactorRep._lex_factor_rep() );

		switch ( pn ) {
		case lex_factor_rep::Star: {
			factorRep = LexFactorRep::cons( lexFactorRep.LEX_STAR().loc(),
					recRep, 0, 0, LexFactorRep::StarType );
			break;
		}
		case lex_factor_rep::StarStar: {
			factorRep = LexFactorRep::cons( lexFactorRep.LEX_STARSTAR().loc(),
					recRep, 0, 0, LexFactorRep::StarStarType );
			break;
		}
		case lex_factor_rep::Plus: {
			factorRep = LexFactorRep::cons( lexFactorRep.LEX_PLUS().loc(),
					recRep, 0, 0, LexFactorRep::PlusType );
			break;
		}
		case lex_factor_rep::Question: {
			factorRep = LexFactorRep::cons( lexFactorRep.LEX_QUESTION().loc(),
					recRep, 0, 0, LexFactorRep::OptionalType );
			break;
		}
		case lex_factor_rep::Exact: {
			int low = atoi( lexFactorRep.lex_uint().data()->data );
			factorRep = LexFactorRep::cons( lexFactorRep.lex_uint().loc(),
					recRep, low, 0, LexFactorRep::ExactType );
			break;
		}
		case lex_factor_rep::Max: {
			int high = atoi( lexFactorRep.lex_uint().data()->data );
			factorRep = LexFactorRep::cons( lexFactorRep.lex_uint().loc(),
					recRep, 0, high, LexFactorRep::MaxType );
			break;
		}
		case lex_factor_rep::Min: {
			int low = atoi( lexFactorRep.lex_uint().data()->data );
			factorRep = LexFactorRep::cons( lexFactorRep.lex_uint().loc(),
					recRep, low, 0, LexFactorRep::MinType );
			break;
		}
		case lex_factor_rep::Range: {
			int low = atoi( lexFactorRep.Low().data()->data );
			int high = atoi( lexFactorRep.High().data()->data );
			factorRep = LexFactorRep::cons( lexFactorRep.Low().loc(),
					recRep, low, high, LexFactorRep::RangeType );
			break;
		}
		case lex_factor_rep::Base: {
			LexFactorNeg *factorNeg = walkLexFactorNeg( lexFactorRep.lex_factor_neg() );
			factorRep = LexFactorRep::cons( factorNeg );
		}}

		return factorRep;
	}

	LexTerm *walkLexTerm( lex_term lexTerm )
	{
		LexTerm *term = 0;
		lex_term::prod_name pn = lexTerm.prodName();

		LexTerm *leftTerm = 0;
		if ( pn != lex_term::Base )
			leftTerm = walkLexTerm( lexTerm._lex_term() );

		LexFactorAug *factorAug = walkLexFactorAug( lexTerm.lex_factor_rep() );

		switch ( pn ) {
		case lex_term::Dot:
			term = LexTerm::cons( leftTerm, factorAug, LexTerm::ConcatType );
			break;
		case lex_term::ColonGt:
			term = LexTerm::cons( leftTerm, factorAug, LexTerm::RightStartType );
			break;
		case lex_term::ColonGtGt:
			term = LexTerm::cons( leftTerm, factorAug, LexTerm::RightFinishType );
			break;
		case lex_term::LtColon:
			term = LexTerm::cons( leftTerm, factorAug, LexTerm::LeftType );
			break;
		default:
			term = LexTerm::cons( factorAug );
			break;
		}

		return term;
	}

	LexExpression *walkLexExpr( lex_expr lexExpr )
	{
		LexExpression *expr = 0;
		lex_expr::prod_name pn = lexExpr.prodName();

		LexExpression *leftExpr = 0;
		if ( pn != lex_expr::Base )
			leftExpr = walkLexExpr( lexExpr._lex_expr() );

		LexTerm *term = walkLexTerm( lexExpr.lex_term() );

		switch ( pn ) {
		case lex_expr::Bar:
			expr = LexExpression::cons( leftExpr, term, LexExpression::OrType );
			break;
		case lex_expr::Amp:
			expr = LexExpression::cons( leftExpr, term, LexExpression::IntersectType );
			break;
		case lex_expr::Dash:
			expr = LexExpression::cons( leftExpr, term, LexExpression::SubtractType );
			break;
		case lex_expr::DashDash:
			expr = LexExpression::cons( leftExpr, term, LexExpression::StrongSubtractType );
			break;
		case lex_expr::Base:
			expr = LexExpression::cons( term );
		}
		return expr;
	}


	void walkRlDef( rl_def rlDef )
	{
		String id = rlDef.id().data();

		lex_expr LexExpr = rlDef.lex_expr();
		LexExpression *expr = walkLexExpr( LexExpr );
		LexJoin *join = LexJoin::cons( expr );

		addRegularDef( rlDef.id().loc(), curNspace(), id, join );
	}

	void walkLexRegion( region_def regionDef )
	{
		pushRegionSet( regionDef.loc() );
		walkRootItemList( regionDef.RootItemList() );
		popRegionSet();
	}

	void walkCflDef( cfl_def cflDef )
	{
		String name = cflDef.id().data();
		ObjectDef *objectDef = walkVarDefList( cflDef.VarDefList() );
		objectDef->name = name;

		LelDefList *defList = new LelDefList;
		walkProdList( name, defList, cflDef.prod_list() );

		bool reduceFirst = cflDef.opt_reduce_first().REDUCEFIRST() != 0;

		NtDef *ntDef = NtDef::cons( name, curNspace(),
				curStruct(), reduceFirst );

		BaseParser::cflDef( ntDef, objectDef, defList );
	}

	CallArgVect *walkCallArgSeq( call_arg_seq callArgSeq )
	{
		CallArgVect *callArgVect = new CallArgVect;
		while ( callArgSeq != 0 ) {
			code_expr codeExpr = callArgSeq.code_expr();
			LangExpr *expr = walkCodeExpr( codeExpr );
			callArgVect->append( new CallArg(expr) );
			callArgSeq = callArgSeq._call_arg_seq();
		}
		return callArgVect;
	}

	CallArgVect *walkCallArgList( call_arg_list callArgList )
	{
		CallArgVect *callArgVect = walkCallArgSeq( callArgList.call_arg_seq() );
		return callArgVect;
	}

	LangStmt *walkPrintStmt( print_stmt &printStmt )
	{
		if ( printStmt.prodName() == print_stmt::Accum ) {
			ConsItemList *list = walkAccumulate( printStmt.accumulate() );
			return LangStmt::cons( printStmt.PRINT().loc(), LangStmt::PrintAccum, list );
		}
		else {
			CallArgVect *exprVect = walkCallArgList( printStmt.call_arg_list() );

			LangStmt::Type type = LangStmt::PrintType;
			switch ( printStmt.prodName() ) {
			case print_stmt::Tree:
				type = LangStmt::PrintType;
				break;
			case print_stmt::PrintStream:
				type = LangStmt::PrintStreamType;
				break;
			case print_stmt::Xml:
				type = LangStmt::PrintXMLType;
				break;
			case print_stmt::XmlAc:
				type = LangStmt::PrintXMLACType;
				break;
			case print_stmt::Accum:
				/* unreachable (see above) */
				break;
			}
				
			return LangStmt::cons( printStmt.POPEN().loc(), type, exprVect );
		}
	}

	QualItemVect *walkQual( qual &Qual )
	{
		QualItemVect *qualItemVect = 0;
		qual RecQual = Qual._qual();
		switch ( Qual.prodName() ) {
		case qual::Dot:
		case qual::Arrow: {
			qualItemVect = walkQual( RecQual );
			String id = Qual.id().data();
			QualItem::Form form = Qual.DOT() != 0 ? QualItem::Dot : QualItem::Arrow;
			qualItemVect->append( QualItem( form, Qual.id().loc(), id ) );
			break;
		}
		case qual::Base: {
			qualItemVect = new QualItemVect;
			break;
		}}
		return qualItemVect;
	}

	LangVarRef *walkVarRef( var_ref varRef )
	{
		NamespaceQual *nspaceQual = walkRegionQual( varRef.region_qual() );
		qual Qual = varRef.qual();
		QualItemVect *qualItemVect = walkQual( Qual );
		String id = varRef.id().data();
		LangVarRef *langVarRef = LangVarRef::cons( varRef.id().loc(),
				curNspace(), curStruct(), curScope(), nspaceQual, qualItemVect, id );
		return langVarRef;
	}

	ObjectField *walkOptCapture( opt_capture optCapture )
	{
		ObjectField *objField = 0;
		if ( optCapture.prodName() == opt_capture::Id ) {
			String id = optCapture.id().data();
			objField = ObjectField::cons( optCapture.id().loc(),
					ObjectField::UserLocalType, 0, id );
		}
		return objField;
	}

	/*
	 * Constructor
	 */

	ConsItemList *walkLitConsEl( lit_cons_el litConsEl, TypeRef *consTypeRef )
	{
		ConsItemList *list = 0;
		switch ( litConsEl.prodName() ) {
		case lit_cons_el::ConsData: {
			String consData = unescape( litConsEl.lit_dq_data().text().c_str() );
			ConsItem *consItem = ConsItem::cons( litConsEl.lit_dq_data().loc(),
					ConsItem::InputText, consData );
			list = ConsItemList::cons( consItem );
			break;
		}
		case lit_cons_el::SubList: {
			list = walkConsElList( litConsEl.ConsElList(), consTypeRef );
			break;
		}}
		return list;
	}

	ConsItemList *walkLitConsElList( _repeat_lit_cons_el litConsElList,
			LIT_DQ_NL Nl, TypeRef *consTypeRef )
	{
		ConsItemList *list = new ConsItemList;
		while ( !litConsElList.end() ) {
			ConsItemList *extension = walkLitConsEl( litConsElList.value(), consTypeRef );
			list = consListConcat( list, extension );
			litConsElList = litConsElList.next();
		}

		if ( Nl != 0 ) {
			String consData = unescape( Nl.data() );
			ConsItem *consItem = ConsItem::cons( Nl.loc(), ConsItem::InputText, consData );
			ConsItemList *term = ConsItemList::cons( consItem );
			list = consListConcat( list, term );
		}

		return list;
	}

	/* Returns the trim status. (true for trim, false otherwise) */
	bool walkTrim( opt_no_trim OptNoTrim )
	{
		return OptNoTrim.prodName() != opt_no_trim::At;
	}

	ConsItemList *walkConsEl( cons_el consEl, TypeRef *consTypeRef )
	{
		ConsItemList *list = 0;
		switch ( consEl.prodName() ) {
		case cons_el::Lit: {
			NamespaceQual *nspaceQual = walkRegionQual( consEl.region_qual() );
			String lit = consEl.backtick_lit().data();
			list = consElLiteral( consEl.backtick_lit().loc(), consTypeRef, lit, nspaceQual );
			break;
		}
		case cons_el::Tilde: {
			String consData = consEl.opt_tilde_data().text().c_str();
			consData += '\n';
			ConsItem *consItem = ConsItem::cons( consEl.opt_tilde_data().loc(),
					ConsItem::InputText, consData );
			list = ConsItemList::cons( consItem );
			break;
		}
		case cons_el::Sq: {
			list = walkConsSqConsDataList( consEl.SqConsDataList(),
					consEl.sq_lit_term().CONS_SQ_NL() );
			break;
		}
		case cons_el::CodeExpr: {
			bool trim = walkTrim( consEl.opt_no_trim() );
			LangExpr *consExpr = walkCodeExpr( consEl.code_expr() );
			ConsItem *consItem = ConsItem::cons( consExpr->loc,
					ConsItem::ExprType, consExpr, trim );
			list = ConsItemList::cons( consItem );
			break;
		}
		case cons_el::Dq: {
			list = walkLitConsElList( consEl.LitConsElList(),
					consEl.dq_lit_term().LIT_DQ_NL(), consTypeRef );
			break;
		}}
		return list;
	}

	ConsItemList *walkConsElList( _repeat_cons_el consElList, TypeRef *consTypeRef )
	{
		ConsItemList *list = new ConsItemList;
		while ( !consElList.end() ) {
			ConsItemList *extension = walkConsEl( consElList.value(), consTypeRef );
			list = consListConcat( list, extension );
			consElList = consElList.next();
		}
		return list;
	}

	ConsItemList *walkConsTopEl( cons_top_el consTopEl, TypeRef *consTypeRef )
	{
		ConsItemList *list = 0;
		switch ( consTopEl.prodName() ) {
		case cons_top_el::Dq: {
			list = walkLitConsElList( consTopEl.LitConsElList(),
					consTopEl.dq_lit_term().LIT_DQ_NL(), consTypeRef );
			break;
		}
		case cons_top_el::Sq: {
			list = walkConsSqConsDataList( consTopEl.SqConsDataList(),
					consTopEl.sq_lit_term().CONS_SQ_NL() );
			break;
		}
		case cons_top_el::Tilde: {
			String consData = consTopEl.opt_tilde_data().text().c_str();
			consData += '\n';
			ConsItem *consItem = ConsItem::cons( consTopEl.opt_tilde_data().loc(),
					ConsItem::InputText, consData );
			list = ConsItemList::cons( consItem );
			break;
		}
		case cons_top_el::SubList: {
			list = walkConsElList( consTopEl.ConsElList(), consTypeRef );
			break;
		}}
		return list;
	}

	ConsItemList *walkConsList( cons_list consList, TypeRef *consTypeRef )
	{
		ConsItemList *list = walkConsTopEl( consList.cons_top_el(), consTypeRef );

		if ( consList._cons_list() != 0 ) {
			ConsItemList *extension = walkConsList( consList._cons_list(), consTypeRef );
			consListConcat( list, extension );
		}

		return list;
	}

	ConsItemList *walkConstructor( constructor Constructor, TypeRef *consTypeRef )
	{
		ConsItemList *list = walkConsList( Constructor.cons_list(), consTypeRef );
		return list;
	}

	/*
	 * String
	 */

	ConsItemList *walkLitStringEl( lit_string_el litStringEl )
	{
		ConsItemList *list = 0;
		switch ( litStringEl.prodName() ) {
		case lit_string_el::ConsData: {
			String consData = unescape( litStringEl.lit_dq_data().text().c_str() );
			ConsItem *stringItem = ConsItem::cons( litStringEl.lit_dq_data().loc(),
					ConsItem::InputText, consData );
			list = ConsItemList::cons( stringItem );
			break;
		}
		case lit_string_el::SubList: {
			list = walkStringElList( litStringEl.StringElList() );
			break;
		}}
		return list;
	}

	ConsItemList *walkLitStringElList( _repeat_lit_string_el litStringElList, LIT_DQ_NL Nl )
	{
		ConsItemList *list = new ConsItemList;
		while ( !litStringElList.end() ) {
			ConsItemList *extension = walkLitStringEl( litStringElList.value() );
			list = consListConcat( list, extension );
			litStringElList = litStringElList.next();
		}

		if ( Nl != 0 ) {
			String consData = unescape( Nl.data() );
			ConsItem *consItem = ConsItem::cons( Nl.loc(),
					ConsItem::InputText, consData );
			ConsItemList *term = ConsItemList::cons( consItem );
			list = consListConcat( list, term );
		}
		return list;
	}

	ConsItemList *walkStringEl( string_el stringEl )
	{
		ConsItemList *list = 0;
		switch ( stringEl.prodName() ) {
		case string_el::Dq: {
			list = walkLitStringElList( stringEl.LitStringElList(),
					stringEl.dq_lit_term().LIT_DQ_NL() );
			break;
		}
		case string_el::Sq: {
			list = walkConsSqConsDataList( stringEl.SqConsDataList(),
					stringEl.sq_lit_term().CONS_SQ_NL() );
			break;
		}
		case string_el::Tilde: {
			String consData = stringEl.opt_tilde_data().text().c_str();
			consData += '\n';
			ConsItem *consItem = ConsItem::cons( stringEl.opt_tilde_data().loc(),
					ConsItem::InputText, consData );
			list = ConsItemList::cons( consItem );
			break;
		}
		case string_el::CodeExpr: {
			bool trim = walkTrim( stringEl.opt_no_trim() );
			LangExpr *consExpr = walkCodeExpr( stringEl.code_expr() );
			ConsItem *consItem = ConsItem::cons( consExpr->loc,
					ConsItem::ExprType, consExpr, trim );
			list = ConsItemList::cons( consItem );
			break;
		}}
		return list;
	}

	ConsItemList *walkStringElList( _repeat_string_el stringElList )
	{
		ConsItemList *list = new ConsItemList;
		while ( !stringElList.end() ) {
			ConsItemList *extension = walkStringEl( stringElList.value() );
			list = consListConcat( list, extension );
			stringElList = stringElList.next();
		}
		return list;
	}

	ConsItemList *walkStringTopEl( string_top_el stringTopEl )
	{
		ConsItemList *list = 0;
		switch ( stringTopEl.prodName() ) {
		case string_top_el::Dq: {
			list = walkLitStringElList( stringTopEl.LitStringElList(),
					stringTopEl.dq_lit_term().LIT_DQ_NL() );
			break;
		}
		case string_el::Sq: {
			list = walkConsSqConsDataList( stringTopEl.SqConsDataList(),
					stringTopEl.sq_lit_term().CONS_SQ_NL() );
			break;
		}
		case string_top_el::Tilde: {
			String consData = stringTopEl.opt_tilde_data().text().c_str();
			consData += '\n';
			ConsItem *consItem = ConsItem::cons( stringTopEl.opt_tilde_data().loc(),
					ConsItem::InputText, consData );
			list = ConsItemList::cons( consItem );
			break;
		}
		case string_top_el::SubList: {
			list = walkStringElList( stringTopEl.StringElList() );
			break;
		}}
		return list;
	}

	ConsItemList *walkStringList( string_list stringList )
	{
		ConsItemList *list = walkStringTopEl( stringList.string_top_el() );

		if ( stringList._string_list() != 0 ) {
			ConsItemList *extension = walkStringList( stringList._string_list() );
			consListConcat( list, extension );
		}

		return list;
	}

	ConsItemList *walkString( string String )
	{
		ConsItemList *list = walkStringList( String.string_list() );
		return list;
	}

	/*
	 * Accum
	 */

	ConsItemList *walkLitAccumEl( lit_accum_el litAccumEl )
	{
		ConsItemList *list = 0;
		switch ( litAccumEl.prodName() ) {
		case lit_accum_el::ConsData: {
			String consData = unescape( litAccumEl.lit_dq_data().text().c_str() );
			ConsItem *consItem = ConsItem::cons( litAccumEl.lit_dq_data().loc(),
					ConsItem::InputText, consData );
			list = ConsItemList::cons( consItem );
			break;
		}
		case lit_accum_el::SubList: {
			list = walkAccumElList( litAccumEl.AccumElList() );
			break;
		}}
		return list;
	}

	ConsItemList *walkLitAccumElList( _repeat_lit_accum_el litAccumElList, LIT_DQ_NL Nl )
	{
		ConsItemList *list = new ConsItemList;
		while ( !litAccumElList.end() ) {
			ConsItemList *extension = walkLitAccumEl( litAccumElList.value() );
			list = consListConcat( list, extension );
			litAccumElList = litAccumElList.next();
		}

		if ( Nl != 0 ) {
			String consData = unescape( Nl.data() );
			ConsItem *consItem = ConsItem::cons( Nl.loc(), ConsItem::InputText, consData );
			ConsItemList *term = ConsItemList::cons( consItem );
			list = consListConcat( list, term );
		}

		return list;
	}

	ConsItemList *walkAccumEl( accum_el accumEl )
	{
		ConsItemList *list = 0;
		switch ( accumEl.prodName() ) {
		case accum_el::Dq: {
			list = walkLitAccumElList( accumEl.LitAccumElList(),
					accumEl.dq_lit_term().LIT_DQ_NL() );
			break;
		}
		case accum_el::Sq: {
			list = walkConsSqConsDataList( accumEl.SqConsDataList(),
					accumEl.sq_lit_term().CONS_SQ_NL() );
			break;
		}
		case accum_el::Tilde: {
			String consData = accumEl.opt_tilde_data().text().c_str();
			consData += '\n';
			ConsItem *consItem = ConsItem::cons( accumEl.opt_tilde_data().loc(),
					ConsItem::InputText, consData );
			list = ConsItemList::cons( consItem );
			break;
		}
		case accum_el::CodeExpr: {
			bool trim = walkTrim( accumEl.opt_no_trim() );
			LangExpr *accumExpr = walkCodeExpr( accumEl.code_expr() );
			ConsItem *consItem = ConsItem::cons( accumExpr->loc,
					ConsItem::ExprType, accumExpr, trim );
			list = ConsItemList::cons( consItem );
			break;
		}}
		return list;
	}

	ConsItemList *walkAccumElList( _repeat_accum_el accumElList )
	{
		ConsItemList *list = new ConsItemList;
		while ( !accumElList.end() ) {
			ConsItemList *extension = walkAccumEl( accumElList.value() );
			list = consListConcat( list, extension );
			accumElList = accumElList.next();
		}
		return list;
	}

	ConsItemList *walkAccumTopEl( accum_top_el accumTopEl )
	{
		ConsItemList *list = 0;
		switch ( accumTopEl.prodName() ) {
		case accum_top_el::Dq: {
			list = walkLitAccumElList( accumTopEl.LitAccumElList(),
					accumTopEl.dq_lit_term().LIT_DQ_NL() );
			break;
		}
		case accum_top_el::Sq: {
			list = walkConsSqConsDataList( accumTopEl.SqConsDataList(),
					accumTopEl.sq_lit_term().CONS_SQ_NL() );
			break;
		}
		case accum_top_el::Tilde: {
			String consData = accumTopEl.opt_tilde_data().text().c_str();
			consData += '\n';
			ConsItem *consItem = ConsItem::cons( accumTopEl.opt_tilde_data().loc(),
					ConsItem::InputText, consData );
			list = ConsItemList::cons( consItem );
			break;
		}
		case accum_top_el::SubList: {
			list = walkAccumElList( accumTopEl.AccumElList() );
			break;
		}}
		return list;
	}

	ConsItemList *walkAccumList( accum_list accumList )
	{
		ConsItemList *list = walkAccumTopEl( accumList.accum_top_el() );

		if ( accumList.prodName() == accum_list::List ) {
			ConsItemList *extension = walkAccumList( accumList._accum_list() );
			consListConcat( list, extension );
		}

		return list;
	}

	ConsItemList *walkAccumulate( accumulate Accumulate )
	{
		ConsItemList *list = walkAccumList( Accumulate.accum_list() );
		return list;
	}

	void walkFieldInit( FieldInitVect *list, field_init fieldInit )
	{
		LangExpr *expr = walkCodeExpr( fieldInit.code_expr() );
		FieldInit *init = FieldInit::cons( expr->loc, "_name", expr );
		list->append( init );
	}

	FieldInitVect *walkFieldInit( _repeat_field_init fieldInitList )
	{
		FieldInitVect *list = new FieldInitVect;
		while ( !fieldInitList.end() ) {
			walkFieldInit( list, fieldInitList.value() );
			fieldInitList = fieldInitList.next();
		}
		return list;
	}
	FieldInitVect *walkOptFieldInit( opt_field_init optFieldInit )
	{
		FieldInitVect *list = 0;
		if ( optFieldInit.prodName() == opt_field_init::Init )
			list = walkFieldInit( optFieldInit.FieldInitList() );
		return list;
	}

	LangExpr *walkCodeFactor( code_factor codeFactor, bool used = true )
	{
		LangExpr *expr = 0;
		switch ( codeFactor.prodName() ) {
		case code_factor::VarRef: {
			LangVarRef *langVarRef = walkVarRef( codeFactor.var_ref() );
			LangTerm *term = LangTerm::cons( langVarRef->loc,
					LangTerm::VarRefType, langVarRef );
			expr = LangExpr::cons( term );
			break;
		}
		case code_factor::Call: {
			LangVarRef *langVarRef = walkVarRef( codeFactor.var_ref() );
			CallArgVect *exprVect = walkCallArgList( codeFactor.call_arg_list() );
			LangTerm *term = LangTerm::cons( langVarRef->loc, langVarRef, exprVect );
			expr = LangExpr::cons( term );
			break;
		}
		case code_factor::Number: {
			String number = codeFactor.number().text().c_str();
			LangTerm *term = LangTerm::cons( codeFactor.number().loc(),
					LangTerm::NumberType, number );
			expr = LangExpr::cons( term );
			break;
		}
		case code_factor::Parse: {
			/* The type we are parsing. */
			type_ref typeRefTree = codeFactor.type_ref();
			TypeRef *typeRef = walkTypeRef( typeRefTree );
			ObjectField *objField = walkOptCapture( codeFactor.opt_capture() );
			FieldInitVect *init = walkOptFieldInit( codeFactor.opt_field_init() );
			ConsItemList *list = walkAccumulate( codeFactor.accumulate() );

			expr = parseCmd( codeFactor.PARSE().loc(), false, false, objField,
					typeRef, init, list, used, false, "" );
			break;
		}
		case code_factor::ParseTree: {
			/* The type we are parsing. */
			type_ref typeRefTree = codeFactor.type_ref();
			TypeRef *typeRef = walkTypeRef( typeRefTree );
			ObjectField *objField = walkOptCapture( codeFactor.opt_capture() );
			FieldInitVect *init = walkOptFieldInit( codeFactor.opt_field_init() );
			ConsItemList *list = walkAccumulate( codeFactor.accumulate() );

			expr = parseCmd( codeFactor.PARSE_TREE().loc(), true, false, objField,
					typeRef, init, list, used, false, "" );
			break;
		}
		case code_factor::ParseStop: {
			/* The type we are parsing. */
			type_ref typeRefTree = codeFactor.type_ref();
			TypeRef *typeRef = walkTypeRef( typeRefTree );
			ObjectField *objField = walkOptCapture( codeFactor.opt_capture() );
			FieldInitVect *init = walkOptFieldInit( codeFactor.opt_field_init() );
			ConsItemList *list = walkAccumulate( codeFactor.accumulate() );

			expr = parseCmd( codeFactor.PARSE_STOP().loc(), false, true, objField,
					typeRef, init, list, used, false, "" );
			break;
		}
		case code_factor::Reduce: {
			/* The reducer name. */
			String reducer = codeFactor.id().data();

			/* The type we are parsing. */
			type_ref typeRefTree = codeFactor.type_ref();
			TypeRef *typeRef = walkTypeRef( typeRefTree );
			FieldInitVect *init = walkOptFieldInit( codeFactor.opt_field_init() );
			ConsItemList *list = walkAccumulate( codeFactor.accumulate() );

			expr = parseCmd( codeFactor.REDUCE().loc(), false, false, 0,
					typeRef, init, list, used, true, reducer );
			break;
		}
		case code_factor::Cons: {
			/* The type we are parsing. */
			type_ref typeRefTree = codeFactor.type_ref();
			TypeRef *typeRef = walkTypeRef( typeRefTree );
			ObjectField *objField = walkOptCapture( codeFactor.opt_capture() );
			ConsItemList *list = walkConstructor( codeFactor.constructor(), typeRef );
			FieldInitVect *init = walkOptFieldInit( codeFactor.opt_field_init() );

			expr = construct( codeFactor.CONS().loc(), objField, list, typeRef, init );
			break;
		}
		case code_factor::Send: {
			LangVarRef *varRef = walkVarRef( codeFactor.var_ref() );
			ConsItemList *list = walkAccumulate( codeFactor.accumulate() );
			bool eof = walkOptEos( codeFactor.opt_eos() );
			expr = send( codeFactor.SEND().loc(), varRef, list, eof );
			break;
		}
		case code_factor::SendTree: {
			LangVarRef *varRef = walkVarRef( codeFactor.var_ref() );
			ConsItemList *list = walkAccumulate( codeFactor.accumulate() );
			bool eof = walkOptEos( codeFactor.opt_eos() );
			expr = sendTree( codeFactor.SEND_TREE().loc(), varRef, list, eof );
			break;
		}
		case code_factor::Nil: {
			expr = LangExpr::cons( LangTerm::cons( codeFactor.NIL().loc(),
					LangTerm::NilType ) );
			break;
		}
		case code_factor::True: {
			expr = LangExpr::cons( LangTerm::cons( codeFactor.TRUE().loc(),
					LangTerm::TrueType ) );
			break;
		}
		case code_factor::False: {
			expr = LangExpr::cons( LangTerm::cons( codeFactor.FALSE().loc(),
					LangTerm::FalseType ) );
			break;
		}
		case code_factor::Paren: {
			expr = walkCodeExpr( codeFactor.code_expr() );
			break;
		}
		case code_factor::String: {
			ConsItemList *list = walkString( codeFactor.string() );
			expr = LangExpr::cons( LangTerm::cons( codeFactor.string().loc(), list ) );
			break;
		}
		case code_factor::Match: {
			LangVarRef *varRef = walkVarRef( codeFactor.var_ref() );
			PatternItemList *list = walkPattern( codeFactor.pattern(), varRef );
			expr = match( codeFactor.loc(), varRef, list );
			break;
		}
		case code_factor::In: {
			TypeRef *typeRef = walkTypeRef( codeFactor.type_ref() );
			LangVarRef *varRef = walkVarRef( codeFactor.var_ref() );
			expr = LangExpr::cons( LangTerm::cons( typeRef->loc,
					LangTerm::SearchType, typeRef, varRef ) );
			break;
		}
		case code_factor::MakeTree: {
			CallArgVect *exprList = walkCallArgList( codeFactor.call_arg_list() );
			expr = LangExpr::cons( LangTerm::cons( codeFactor.loc(),
					LangTerm::MakeTreeType, exprList ) );
			break;
		}
		case code_factor::MakeToken: {
			CallArgVect *exprList = walkCallArgList( codeFactor.call_arg_list() );
			expr = LangExpr::cons( LangTerm::cons( codeFactor.loc(),
					LangTerm::MakeTokenType, exprList ) );
			break;
		}
		case code_factor::TypeId: {
			TypeRef *typeRef = walkTypeRef( codeFactor.type_ref() );
			expr = LangExpr::cons( LangTerm::cons( codeFactor.loc(),
					LangTerm::TypeIdType, typeRef ) );
			break;
		}
		case code_factor::New: {
			TypeRef *typeRef = walkTypeRef( codeFactor.type_ref() );

			ObjectField *captureField = walkOptCapture( codeFactor.opt_capture() );
			FieldInitVect *init = walkFieldInit( codeFactor.FieldInitList() );

			LangVarRef *captureVarRef = 0;
			if ( captureField != 0 ) {
				captureVarRef = LangVarRef::cons( captureField->loc,
						curNspace(), curStruct(), curScope(), captureField->name );
			}

			expr = LangExpr::cons( LangTerm::consNew(
					codeFactor.loc(), typeRef, captureVarRef, init ) );

			/* Check for redeclaration. */
			if ( captureField != 0 ) {
				if ( curScope()->checkRedecl( captureField->name ) != 0 ) {
					error( captureField->loc ) << "variable " <<
							captureField->name << " redeclared" << endp;
				}

				/* Insert it into the field map. */
				captureField->typeRef = typeRef;
				curScope()->insertField( captureField->name, captureField );
			}
			break;
		}
		case code_factor::Cast: {
			TypeRef *typeRef = walkTypeRef( codeFactor.type_ref() );
			LangExpr *castExpr = walkCodeFactor( codeFactor._code_factor() );
			expr = LangExpr::cons( LangTerm::cons( codeFactor.loc(),
					LangTerm::CastType, typeRef, castExpr ) );
			break;
		}}
		return expr;
	}

	LangExpr *walkCodeAdditive( code_additive additive, bool used = true )
	{
		LangExpr *expr = 0;
		switch ( additive.prodName() ) {
		case code_additive::Plus: {
			LangExpr *left = walkCodeAdditive( additive._code_additive() );
			LangExpr *right = walkCodeMultiplicitive( additive.code_multiplicitive() );
			expr = LangExpr::cons( additive.PLUS().loc(), left, '+', right );
			break;
		}
		case code_additive::Minus: {
			LangExpr *left = walkCodeAdditive( additive._code_additive() );
			LangExpr *right = walkCodeMultiplicitive( additive.code_multiplicitive() );
			expr = LangExpr::cons( additive.MINUS().loc(), left, '-', right );
			break;
		}
		case code_additive::Base: {
			expr = walkCodeMultiplicitive( additive.code_multiplicitive(), used );
			break;
		}}
		return expr;
	}

	LangExpr *walkCodeUnary( code_unary unary, bool used = true )
	{
		LangExpr *expr = 0;

		switch ( unary.prodName() ) {
		case code_unary::Bang: {
			LangExpr *factor = walkCodeFactor( unary.code_factor() );
			expr = LangExpr::cons( unary.BANG().loc(), '!', factor );
			break;
		}
		case code_unary::Dollar: {
			LangExpr *factor = walkCodeFactor( unary.code_factor() );
			expr = LangExpr::cons( unary.DOLLAR().loc(), '$', factor );
			break;
		}
		case code_unary::DollarDollar: {
			LangExpr *factor = walkCodeFactor( unary.code_factor() );
			expr = LangExpr::cons( unary.DOLLAR().loc(), 'S', factor );
			break;
		}
		case code_unary::Caret: {
			LangExpr *factor = walkCodeFactor( unary.code_factor() );
			expr = LangExpr::cons( unary.CARET().loc(), '^', factor );
			break;
		}
		case code_unary::Percent: {
			LangExpr *factor = walkCodeFactor( unary.code_factor() );
			expr = LangExpr::cons( unary.PERCENT().loc(), '%', factor );
			break;
		}
		case code_unary::Base: {
			LangExpr *factor = walkCodeFactor( unary.code_factor(), used );
			expr = factor;
		}}

		return expr;
	}

	LangExpr *walkCodeRelational( code_relational codeRelational, bool used = true )
	{
		LangExpr *expr = 0, *left = 0;

		bool base = codeRelational.prodName() == code_relational::Base;

		if ( ! base ) {
			used = true;
			left = walkCodeRelational( codeRelational._code_relational() );
		}
		
		LangExpr *additive = walkCodeAdditive( codeRelational.code_additive(), used );

		switch ( codeRelational.prodName() ) {
		case code_relational::EqEq: {
			expr = LangExpr::cons( codeRelational.loc(), left, OP_DoubleEql, additive );
			break;
		}
		case code_relational::Neq: {
			expr = LangExpr::cons( codeRelational.loc(), left, OP_NotEql, additive );
			break;
		}
		case code_relational::Lt: {
			expr = LangExpr::cons( codeRelational.loc(), left, '<', additive );
			break;
		}
		case code_relational::Gt: {
			expr = LangExpr::cons( codeRelational.loc(), left, '>', additive );
			break;
		}
		case code_relational::LtEq: {
			expr = LangExpr::cons( codeRelational.loc(), left, OP_LessEql, additive );
			break;
		}
		case code_relational::GtEq: {
			expr = LangExpr::cons( codeRelational.loc(), left, OP_GrtrEql, additive );
			break;
		}
		case code_relational::Base: {
			expr = additive;
			break;
		}}
		return expr;
	}

	LangStmt *walkExprStmt( expr_stmt exprStmt )
	{
		LangExpr *expr = walkCodeExpr( exprStmt.code_expr(), false );
		LangStmt *stmt = LangStmt::cons( expr->loc, LangStmt::ExprType, expr );
		return stmt;
	}

	ObjectField *walkVarDef( var_def varDef, ObjectField::Type type )
	{
		String id = varDef.id().data();
		TypeRef *typeRef = walkTypeRef( varDef.type_ref() );
		return ObjectField::cons( varDef.id().loc(), type, typeRef, id );
	}

	IterCall *walkIterCall( iter_call Tree )
	{
		IterCall *iterCall = 0;
		switch ( Tree.prodName() ) {
		case iter_call::Call: {
			LangVarRef *varRef = walkVarRef( Tree.var_ref() );
			CallArgVect *exprVect = walkCallArgList( Tree.call_arg_list() );
			LangTerm *langTerm = LangTerm::cons( varRef->loc, varRef, exprVect );
			iterCall = IterCall::cons( IterCall::Call, langTerm );
			break;
		}
		case iter_call::Id: {
			String tree = Tree.id().data();
			LangVarRef *varRef = LangVarRef::cons( Tree.id().loc(),
					curNspace(), curStruct(), curScope(), tree );
			LangTerm *langTerm = LangTerm::cons( Tree.id().loc(),
					LangTerm::VarRefType, varRef );
			LangExpr *langExpr = LangExpr::cons( langTerm );
			iterCall = IterCall::cons( IterCall::Expr, langExpr );
			break;
		}
		case iter_call::Expr: {
			LangExpr *langExpr = walkCodeExpr( Tree.code_expr() );
			iterCall = IterCall::cons( IterCall::Expr, langExpr );
			break;
		}}
		
		return iterCall;
	}

	LangStmt *walkElsifClause( elsif_clause elsifClause )
	{
		pushScope();
		LangExpr *expr = walkCodeExpr( elsifClause.code_expr() );
		StmtList *stmtList = walkBlockOrSingle( elsifClause.block_or_single() );
		LangStmt *stmt = LangStmt::cons( LangStmt::IfType, expr, stmtList, 0 );
		popScope();
		return stmt;
	}

	LangStmt *walkOptionalElse( optional_else optionalElse )
	{
		LangStmt *stmt = 0;
		if ( optionalElse.prodName() == optional_else::Else ) {
			pushScope();
			StmtList *stmtList = walkBlockOrSingle( optionalElse.block_or_single() );
			stmt = LangStmt::cons( LangStmt::ElseType, stmtList );
			popScope();
		}
		return stmt;
	}

	LangStmt *walkElsifList( elsif_list elsifList )
	{
		LangStmt *stmt = 0;
		switch ( elsifList.prodName() ) {
			case elsif_list::Clause:
				stmt = walkElsifClause( elsifList.elsif_clause() );
				stmt->elsePart = walkElsifList( elsifList._elsif_list() );
				break;
			case elsif_list::OptElse:
				stmt = walkOptionalElse( elsifList.optional_else() );
				break;
		}
		return stmt;
	}

	LangStmt *walkCaseClauseList( case_clause_list CaseClauseList, var_ref VarRef )
	{
		LangStmt *stmt = 0;
		switch ( CaseClauseList.prodName() ) {
			case case_clause_list::Recursive: {
				pushScope();

				LangVarRef *varRef = walkVarRef( VarRef );
				PatternItemList *list = walkPattern( CaseClauseList.case_clause().pattern(), varRef );
				LangExpr *expr = match( CaseClauseList.loc(), varRef, list );

				StmtList *stmtList = walkBlockOrSingle(
						CaseClauseList.case_clause().block_or_single() );

				popScope();

				LangStmt *recList = walkCaseClauseList(
						CaseClauseList._case_clause_list(), VarRef );

				stmt = LangStmt::cons( LangStmt::IfType, expr, stmtList, recList );
				break;
			}
			case case_clause_list::BaseCase: {
				pushScope();

				LangVarRef *varRef = walkVarRef( VarRef );
				PatternItemList *list = walkPattern(
						CaseClauseList.case_clause().pattern(), varRef );
				LangExpr *expr = match( CaseClauseList.loc(), varRef, list );

				StmtList *stmtList = walkBlockOrSingle(
						CaseClauseList.case_clause().block_or_single() );
				popScope();
				stmt = LangStmt::cons( LangStmt::IfType, expr, stmtList, 0 );
				break;
			}
			case case_clause_list::BaseDefault: {
				pushScope();
				StmtList *stmtList = walkBlockOrSingle(
						CaseClauseList.default_clause().block_or_single() );
				popScope();
				stmt = LangStmt::cons( LangStmt::ElseType, stmtList );
				break;
			}
		}
		return stmt;
	}

	void walkStructVarDef( struct_var_def StructVarDef )
	{
		ObjectField *objField = walkVarDef( StructVarDef.var_def(), 
				ObjectField::StructFieldType );
		structVarDef( objField->loc, objField );
	}

	TypeRef *walkReferenceTypeRef( reference_type_ref ReferenceTypeRef )
	{
		TypeRef *typeRef = walkTypeRef( ReferenceTypeRef.type_ref() );
		return TypeRef::cons( ReferenceTypeRef.REF().loc(), TypeRef::Ref, typeRef );
	}

	ObjectField *walkParamVarDef( param_var_def paramVarDef )
	{
		String id = paramVarDef.id().data();
		TypeRef *typeRef = 0;
		ObjectField::Type type;

		switch ( paramVarDef.prodName() ) {
		case param_var_def::Type: 
			typeRef = walkTypeRef( paramVarDef.type_ref() );
			type = ObjectField::ParamValType;
			break;
		case param_var_def::Ref:
			typeRef = walkReferenceTypeRef( paramVarDef.reference_type_ref() );
			type = ObjectField::ParamRefType;
			break;
		}
		
		return addParam( paramVarDef.id().loc(), type, typeRef, id );
	}

	ParameterList *walkParamVarDefSeq( param_var_def_seq paramVarDefSeq )
	{
		ParameterList *paramList = new ParameterList;
		while ( paramVarDefSeq != 0 ) {
			ObjectField *param = walkParamVarDef( paramVarDefSeq.param_var_def() );
			appendParam( paramList, param );
			paramVarDefSeq = paramVarDefSeq._param_var_def_seq();
		}
		return paramList;
	}

	ParameterList *walkParamVarDefList( param_var_def_list paramVarDefList )
	{
		ParameterList *paramList = walkParamVarDefSeq(
				paramVarDefList.param_var_def_seq() );
		return paramList;
	}

	bool walkOptExport( opt_export OptExport )
	{
		return OptExport.prodName() == opt_export::Export;
	}

	void walkFunctionDef( function_def FunctionDef )
	{
		ObjectDef *localFrame = blockOpen();

		bool exprt = walkOptExport( FunctionDef.opt_export() );
		TypeRef *typeRef = walkTypeRef( FunctionDef.type_ref() );
		String id = FunctionDef.id().data();
		ParameterList *paramList = walkParamVarDefList( FunctionDef.ParamVarDefList() );
		StmtList *stmtList = walkLangStmtList( FunctionDef.lang_stmt_list() );
		functionDef( stmtList, localFrame, paramList, typeRef, id, exprt );

		blockClose();
	}

	void walkInHostDef( in_host_def InHostDef )
	{
		ObjectDef *localFrame = blockOpen();

		TypeRef *typeRef = walkTypeRef( InHostDef.type_ref() );
		String id = InHostDef.id().data();
		ParameterList *paramList = walkParamVarDefList( InHostDef.ParamVarDefList() );
		inHostDef( InHostDef.HostFunc().data(), localFrame, paramList, typeRef, id, false );

		blockClose();
	}

	void walkIterDef( iter_def IterDef )
	{
		ObjectDef *localFrame = blockOpen();

		String id = IterDef.id().data();
		ParameterList *paramList = walkParamVarDefList( IterDef.ParamVarDefList() );
		StmtList *stmtList = walkLangStmtList( IterDef.lang_stmt_list() );
		iterDef( stmtList, localFrame, paramList, id );

		blockClose();
	}

	void walkStructItem( struct_item structItem )
	{
		switch ( structItem.prodName() ) {
		case struct_item::Rl:
			walkRlDef( structItem.rl_def() );
			break;
		case struct_item::StructVar:
			walkStructVarDef( structItem.struct_var_def() );
			break;
		case struct_item::Token:
			walkTokenDef( structItem.token_def() );
			break;
		case struct_item::IgnoreCollector:
			walkIgnoreCollector( structItem.ic_def() );
			break;
		case struct_item::Ignore:
			walkIgnoreDef( structItem.ignore_def() );
			break;
		case struct_item::Literal:
			walkLiteralDef( structItem.literal_def() );
			break;
		case struct_item::Cfl:
			walkCflDef( structItem.cfl_def() );
			break;
		case struct_item::Region:
			walkLexRegion( structItem.region_def() );
			break;
		case struct_item::Struct:
			walkStructDef( structItem.struct_def() );
			break;
		case struct_item::Function:
			walkFunctionDef( structItem.function_def() );
			break;
		case struct_item::InHost:
			walkInHostDef( structItem.in_host_def() );
			break;
		case struct_item::Iter:
			walkIterDef( structItem.iter_def() );
			break;
		case struct_item::PreEof:
			walkPreEof( structItem.pre_eof_def() );
			break;
		case struct_item::Export:
			walkExportDef( structItem.export_def() );
			break;
		case struct_item::Precedence:
			walkPrecedenceDef( structItem.precedence_def() );
			break;
//		case struct_item::ListEl:
//			listElDef( structItem.list_el_def().id().data() );
//			break;
//		case struct_item::MapEl: {
//			map_el_def Def = structItem.map_el_def();
//			TypeRef *keyTr = walkTypeRef( Def.type_ref() );
//			mapElDef( Def.id().data(), keyTr );
//			break;
//		}
		case struct_item::Alias:
			walkAliasDef( structItem.alias_def() );
			break;
		}
	}

	void walkStructDef( struct_def structDef )
	{
		String name = structDef.id().data();
		structHead( structDef.id().loc(), curNspace(), name, ObjectDef::StructType );

		_repeat_struct_item structItemList = structDef.ItemList();
		while ( !structItemList.end() ) {
			walkStructItem( structItemList.value() );
			structItemList = structItemList.next();
		}

		structStack.pop();
		namespaceStack.pop();
	}

	void walkNamespaceDef( namespace_def NamespaceDef, StmtList *stmtList )
	{
		String name = NamespaceDef.id().data();
		createNamespace( NamespaceDef.id().loc(), name );
		walkNamespaceItemList( NamespaceDef.ItemList(), stmtList );
		namespaceStack.pop();
	}

	void walkRedItem( host_item item, ReduceTextItemList &list )
	{
		if ( item.RED_LHS() != 0 ) {
			ReduceTextItem *rti = new ReduceTextItem;
			rti->type = ReduceTextItem::LhsRef;
			list.append( rti );
		}
		else if ( item.RED_RHS_REF() != 0 ) {
			ReduceTextItem *rti = new ReduceTextItem;
			rti->type = ReduceTextItem::RhsRef;
			rti->txt = item.RED_RHS_REF().text().c_str();
			list.append( rti );
		}
		else if ( item.RED_RHS_LOC() != 0 ) {
			ReduceTextItem *rti = new ReduceTextItem;
			rti->type = ReduceTextItem::RhsLoc;
			rti->txt = item.RED_RHS_LOC().text().c_str();
			list.append( rti );
		}
		else if ( item.RED_RHS_NREF() != 0 ) {
			ReduceTextItem *rti = new ReduceTextItem;
			rti->type = ReduceTextItem::RhsRef;
			rti->n = atoi( item.RED_RHS_NREF().text().c_str() + 1 );
			list.append( rti );
		}
		else if ( item.RED_RHS_NLOC() != 0 ) {
			ReduceTextItem *rti = new ReduceTextItem;
			rti->type = ReduceTextItem::RhsLoc;
			rti->n = atoi( item.RED_RHS_NLOC().text().c_str() + 1 );
			list.append( rti );
		}
		else if ( item.RED_OPEN() != 0 ) {
			ReduceTextItem *open = new ReduceTextItem;
			open->type = ReduceTextItem::Txt;
			open->txt = "{";
			list.append( open );

			walkRedItemList( item.HostItems(), list );

			ReduceTextItem *close = new ReduceTextItem;
			close->type = ReduceTextItem::Txt;
			close->txt = "}";
			list.append( close );
		}
		else {
			if ( list.length() > 0 && list.tail->type == ReduceTextItem::Txt ) {
				std::string txt = item.text();
				list.tail->txt.append( txt.c_str(), txt.size() );
			}
			else {
				ReduceTextItem *rti = new ReduceTextItem;
				rti->type = ReduceTextItem::Txt;
				rti->txt = item.text().c_str();
				list.append( rti );
			}
		}
	}

	void walkRedItemList( _repeat_host_item itemList, ReduceTextItemList &list )
	{
		while ( !itemList.end() ) {
			walkRedItem( itemList.value(), list );
			itemList = itemList.next();
		}
	}

	void walkRedNonTerm( red_nonterm RN )
	{
		InputLoc loc = RN.RED_OPEN().loc();

		TypeRef *typeRef = walkTypeRef( RN.type_ref() );

		ReduceNonTerm *rnt = new ReduceNonTerm( loc, typeRef );

		walkRedItemList( RN.HostItems(), rnt->itemList );

		curReduction()->reduceNonTerms.append( rnt );
	}

	void walkRedAction( red_action RA )
	{
		InputLoc loc = RA.RED_OPEN().loc();
		String text = RA.HostItems().text().c_str();

		TypeRef *typeRef = walkTypeRef( RA.type_ref() );
			
		ReduceAction *ra = new ReduceAction( loc, typeRef, RA.id().data() );

		walkRedItemList( RA.HostItems(), ra->itemList );

		curReduction()->reduceActions.append( ra );
	}

	void walkReductionItem( reduction_item reductionItem )
	{
		switch ( reductionItem.prodName() ) {
			case reduction_item::NonTerm: {
				walkRedNonTerm( reductionItem.red_nonterm() );
				break;
			}
			case reduction_item::Action: {
				walkRedAction( reductionItem.red_action() );
				break;
			}
		}
	}

	void walkReductionList( _repeat_reduction_item itemList )
	{
		while ( !itemList.end() ) {
			walkReductionItem( itemList.value() );
			itemList = itemList.next();
		}
	}

	void walkRootItem( root_item rootItem, StmtList *stmtList )
	{
		switch ( rootItem.prodName() ) {
		case root_item::Rl:
			walkRlDef( rootItem.rl_def() );
			break;
		case root_item::Token:
			walkTokenDef( rootItem.token_def() );
			break;
		case root_item::IgnoreCollector:
			walkIgnoreCollector( rootItem.ic_def() );
			break;
		case root_item::Ignore:
			walkIgnoreDef( rootItem.ignore_def() );
			break;
		case root_item::Literal:
			walkLiteralDef( rootItem.literal_def() );
			break;
		case root_item::Cfl:
			walkCflDef( rootItem.cfl_def() );
			break;
		case root_item::Region:
			walkLexRegion( rootItem.region_def() );
			break;
		case root_item::Statement: {
			LangStmt *stmt = walkStatement( rootItem.statement() );
			if ( stmt != 0 )
				stmtList->append( stmt );
			break;
		}
		case root_item::Struct:
			walkStructDef( rootItem.struct_def() );
			break;
		case root_item::Namespace:
			walkNamespaceDef( rootItem.namespace_def(), stmtList );
			break;
		case root_item::Function:
			walkFunctionDef( rootItem.function_def() );
			break;
		case struct_item::InHost:
			walkInHostDef( rootItem.in_host_def() );
			break;
		case root_item::Iter:
			walkIterDef( rootItem.iter_def() );
			break;
		case root_item::PreEof:
			walkPreEof( rootItem.pre_eof_def() );
			break;
		case root_item::Export: {
			LangStmt *stmt = walkExportDef( rootItem.export_def() );
			if ( stmt != 0 )
				stmtList->append( stmt );
			break;
		}
		case root_item::Alias:
			walkAliasDef( rootItem.alias_def() );
			break;
		case root_item::Precedence:
			walkPrecedenceDef( rootItem.precedence_def() );
			break;
		case root_item::Include: {
			StmtList *includeList = walkInclude( rootItem.include() );
			if ( includeList )
				stmtList->append( *includeList );
			break;
		}
		case root_item::Global: {
			LangStmt *stmt = walkGlobalDef( rootItem.global_def() );
			if ( stmt != 0 )
				stmtList->append( stmt );
			break;
		}
		case root_item::Reduction: {
			reduction_def RD = rootItem.reduction_def();

			InputLoc loc = RD.REDUCTION().loc();
			String id = RD.id().data();

			createReduction( loc, id );

			walkReductionList( RD.ItemList() );

			reductionStack.pop();
			break;
		}}
	}

	void walkNamespaceItem( namespace_item item, StmtList *stmtList )
	{
		switch ( item.prodName() ) {
		case namespace_item::Rl:
			walkRlDef( item.rl_def() );
			break;
		case namespace_item::Token:
			walkTokenDef( item.token_def() );
			break;
		case root_item::IgnoreCollector:
			walkIgnoreCollector( item.ic_def() );
			break;
		case namespace_item::Ignore:
			walkIgnoreDef( item.ignore_def() );
			break;
		case namespace_item::Literal:
			walkLiteralDef( item.literal_def() );
			break;
		case namespace_item::Cfl:
			walkCflDef( item.cfl_def() );
			break;
		case namespace_item::Region:
			walkLexRegion( item.region_def() );
			break;
		case namespace_item::Struct:
			walkStructDef( item.struct_def() );
			break;
		case namespace_item::Namespace:
			walkNamespaceDef( item.namespace_def(), stmtList );
			break;
		case namespace_item::Function:
			walkFunctionDef( item.function_def() );
			break;
		case struct_item::InHost:
			walkInHostDef( item.in_host_def() );
			break;
		case namespace_item::Iter:
			walkIterDef( item.iter_def() );
			break;
		case namespace_item::PreEof:
			walkPreEof( item.pre_eof_def() );
			break;
		case namespace_item::Alias:
			walkAliasDef( item.alias_def() );
			break;
		case namespace_item::Precedence:
			walkPrecedenceDef( item.precedence_def() );
			break;
		case namespace_item::Include: {
			StmtList *includeList = walkInclude( item.include() );
			stmtList->append( *includeList );
			break;
		}
		case namespace_item::Global: {
			LangStmt *stmt = walkGlobalDef( item.global_def() );
			if ( stmt != 0 )
				stmtList->append( stmt );
			break;
		}}
	}

	bool walkNoIgnoreLeft( no_ignore_left OptNoIngore )
	{
		return OptNoIngore.prodName() == no_ignore_left::Ni;
	}

	bool walkNoIgnoreRight( no_ignore_right OptNoIngore )
	{
		return OptNoIngore.prodName() == no_ignore_right::Ni;
	}

	bool walkOptEos( opt_eos OptEos )
	{
		opt_eos::prod_name pn = OptEos.prodName();
		return pn == opt_eos::Dot || pn == opt_eos::Eos;
	}

	void walkLiteralItem( literal_item literalItem )
	{
		bool niLeft = walkNoIgnoreLeft( literalItem.no_ignore_left() );
		bool niRight = walkNoIgnoreRight( literalItem.no_ignore_right() );

		String lit = literalItem.backtick_lit().data();
		literalDef( literalItem.backtick_lit().loc(), lit, niLeft, niRight );
	}

	void walkLiteralList( literal_list literalList )
	{
		if ( literalList.prodName() == literal_list::Item )
			walkLiteralList( literalList._literal_list() );
		walkLiteralItem( literalList.literal_item() );
	}

	void walkLiteralDef( literal_def literalDef )
	{
		walkLiteralList( literalDef.literal_list() );
	}

	void walkNamespaceItemList( _repeat_namespace_item itemList, StmtList *stmtList )
	{
		/* Walk the list of items. */
		while ( !itemList.end() ) {
			walkNamespaceItem( itemList.value(), stmtList );
			itemList = itemList.next();
		}
	}

	StmtList *walkRootItemList( _repeat_root_item rootItemList )
	{
		StmtList *stmtList = new StmtList;

		/* Walk the list of items. */
		while ( !rootItemList.end() ) {
			walkRootItem( rootItemList.value(), stmtList );
			rootItemList = rootItemList.next();
		}
		return stmtList;
	}

	virtual void go( long activeRealm );
};

void LoadColm::go( long activeRealm )
{
	LoadColm::init();

	const char *argv[3];
	argv[0] = "load-colm";
	argv[1] = inputFileName;
	argv[2] = 0;

	colm_program *program = colm_new_program( &colm_object );
	colm_set_debug( program, activeRealm );
	colm_run_program( program, 2, argv );

	/* Extract the parse tree. */
	start Start = ColmTree( program );
	str Error = ColmError( program );

	if ( Start == 0 ) {
		gblErrorCount += 1;
		InputLoc loc = Error.loc();
		error(loc) << inputFileName << ": parse error: " << Error.text() << std::endl;
		return;
	}

	StmtList *stmtList = walkRootItemList( Start.RootItemList() );
	pd->streamFileNames.append( colm_extract_fns( program ) );
	colm_delete_program( program );

	pd->rootCodeBlock = CodeBlock::cons( stmtList, 0 );
}

BaseParser *consLoadColm( Compiler *pd, const char *inputFileName )
{
	return new LoadColm( pd, inputFileName );
}
