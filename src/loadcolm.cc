/*
 *  Copyright 2006-2012 Adrian Thurston <thurston@complang.org>
 */

/*  This file is part of Colm.
 *
 *  Colm is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 * 
 *  Colm is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 * 
 *  You should have received a copy of the GNU General Public License
 *  along with Colm; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA 
 */

#include <iostream>
#include <errno.h>

#include "parser.h"
#include "config.h"
#include "avltree.h"
#include "parsedata.h"
#include "parser.h"
#include "global.h"
#include "input.h"
#include "loadcolm.h"
#include "if2.h"
#include "colm/colm.h"

extern RuntimeData colm_object;

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
		case lex_range_lit::_Lit: {
			String lit = lexRangeLit.lex_lit().data();
			literal = Literal::cons( lexRangeLit.lex_lit().loc(), lit, Literal::LitString );
			break;
		}
		case lex_range_lit::_Number: {
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
		case lex_factor::_Literal: {
			String litString = lexFactor.lex_lit().data();
			Literal *literal = Literal::cons( lexFactor.lex_lit().loc(),
					litString, Literal::LitString );
			factor = LexFactor::cons( literal );
			break;
		}
		case lex_factor::_Id: {
			String id = lexFactor.lex_id().data();
			factor = lexRlFactorName( id, lexFactor.lex_id().loc() );
			break;
		}
		case lex_factor::_Range: {
			Literal *low = walkLexRangeLit( lexFactor.Low() );
			Literal *high = walkLexRangeLit( lexFactor.High() );

			Range *range = Range::cons( low, high );
			factor = LexFactor::cons( range );
			break;
		}
		case lex_factor::_PosOrBlock: {
			ReOrBlock *block = walkRegOrData( lexFactor.reg_or_data() );
			factor = LexFactor::cons( ReItem::cons( block, ReItem::OrBlock ) );
			break;
		}
		case lex_factor::_NegOrBlock: {
			ReOrBlock *block = walkRegOrData( lexFactor.reg_or_data() );
			factor = LexFactor::cons( ReItem::cons( block, ReItem::NegOrBlock ) );
			break;
		}
		case lex_factor::_Number: {
			String number = lexFactor.lex_uint().text().c_str();
			factor = LexFactor::cons( Literal::cons( lexFactor.lex_uint().loc(), 
					number, Literal::Number ) );
			break;
		}
		case lex_factor::_Hex: {
			String number = lexFactor.lex_hex().text().c_str();
			factor = LexFactor::cons( Literal::cons( lexFactor.lex_hex().loc(), 
					number, Literal::Number ) );
			break;
		}
		case lex_factor::_Paren: {
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

	LangExpr *walkCodeExpr( code_expr codeExpr )
	{
		LangExpr *expr = 0;
		LangExpr *relational = walkCodeRelational( codeExpr.code_relational() );

		switch ( codeExpr.prodName() ) {
		case code_expr::_AmpAmp: {
			LangExpr *left = walkCodeExpr( codeExpr._code_expr() );

			InputLoc loc = codeExpr.AMPAMP().loc();
			expr = LangExpr::cons( loc, left, OP_LogicalAnd, relational );
			break;
		}
		case code_expr::_BarBar: {
			LangExpr *left = walkCodeExpr( codeExpr._code_expr() );

			InputLoc loc = codeExpr.BARBAR().loc();
			expr = LangExpr::cons( loc, left, OP_LogicalOr, relational );
			break;
		}
		case code_expr::_Base: {
			expr = relational;
			break;
		}}
		return expr;
	}

	LangStmt *walkStatement( statement Statement )
	{
		LangStmt *stmt = 0;
		switch ( Statement.prodName() ) {
		case statement::_Print: {
			print_stmt printStmt = Statement.print_stmt();
			stmt = walkPrintStmt( printStmt );
			break;
		}
		case statement::_Expr: {
			expr_stmt exprStmt = Statement.expr_stmt();
			stmt = walkExprStmt( exprStmt );
			break;
		}
		case statement::_VarDef: {
			ObjectField *objField = walkVarDef( Statement.var_def() );
			LangExpr *expr = walkOptDefInit( Statement.opt_def_init() );
			stmt = varDef( objField, expr, LangStmt::AssignType );
			break;
		}
		case statement::_For: {
			pushScope();

			String forDecl = Statement.id().text().c_str();
			TypeRef *typeRef = walkTypeRef( Statement.type_ref() );
			StmtList *stmtList = walkBlockOrSingle( Statement.block_or_single() );

			LangIterCall *iterCall = walkIterCall( Statement.iter_call() );

			stmt = forScope( Statement.id().loc(), forDecl,
					pd->curLocalFrame->curScope, typeRef, iterCall, stmtList );

			popScope();
			break;
		}
		case statement::_If: {
			pushScope();

			LangExpr *expr = walkCodeExpr( Statement.code_expr() );
			StmtList *stmtList = walkBlockOrSingle( Statement.block_or_single() );

			popScope();

			LangStmt *elsifList = walkElsifList( Statement.elsif_list() );
			stmt = LangStmt::cons( LangStmt::IfType, expr, stmtList, elsifList );
			break;
		}
		case statement::_While: {
			pushScope();
			LangExpr *expr = walkCodeExpr( Statement.code_expr() );
			StmtList *stmtList = walkBlockOrSingle( Statement.block_or_single() );
			stmt = LangStmt::cons( LangStmt::WhileType, expr, stmtList );
			popScope();
			break;
		}
		case statement::_LhsVarRef: {
			LangVarRef *varRef = walkVarRef( Statement.var_ref() );
			LangExpr *expr = walkCodeExpr( Statement.code_expr() );
			stmt = LangStmt::cons( varRef->loc, LangStmt::AssignType, varRef, expr );
			break;
		}
		case statement::_Yield: {
			LangVarRef *varRef = walkVarRef( Statement.var_ref() );
			stmt = LangStmt::cons( LangStmt::YieldType, varRef );
			break;
		}
		case statement::_Return: {
			LangExpr *expr = walkCodeExpr( Statement.code_expr() );
			stmt = LangStmt::cons( Statement.loc(), LangStmt::ReturnType, expr );
			break;
		}
		case statement::_Break: {
			stmt = LangStmt::cons( LangStmt::BreakType );
			break;
		}
		case statement::_Reject: {
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

		if ( langStmtList.opt_require_stmt().require_pattern() != 0 ) {
			pushScope();
			require_pattern require = langStmtList.opt_require_stmt().require_pattern();

			LangVarRef *varRef = walkVarRef( require.var_ref() );
			PatternItemList *list = walkPattern( require.pattern(), varRef );
			LangExpr *expr = match( langStmtList.opt_require_stmt().require_pattern().REQUIRE().loc(), varRef, list );

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

		bool niLeft = walkOptNoIgnore( TokenDef.NiLeft() );
		bool niRight = walkOptNoIgnore( TokenDef.NiRight() );

		ObjectDef *objectDef = walkVarDefList( TokenDef.VarDefList() );
		objectDef->name = name;

		LexJoin *join = 0;
		if ( TokenDef.opt_lex_expr().lex_expr() != 0 ) {
			LexExpression *expr = walkLexExpr( TokenDef.opt_lex_expr().lex_expr() );
			join = LexJoin::cons( expr );
		}

		CodeBlock *translate = walkOptTranslate( TokenDef.opt_translate() );

		defineToken( TokenDef.id().loc(), name, join, objectDef, translate, false, niLeft, niRight );
	}

	String walkOptId( opt_id optId )
	{
		String name;
		if ( optId.prodName() == opt_id::_Id )
			name = optId.id().data();
		return name;
	}

	ObjectDef *walkVarDefList( _repeat_var_def varDefList )
	{
		ObjectDef *objectDef = ObjectDef::cons( ObjectDef::UserType,
				String(), pd->nextObjectId++ ); 

		while ( !varDefList.end() ) {
			ObjectField *varDef = walkVarDef( varDefList.value() );
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
		ObjectDef *objectDef = ObjectDef::cons( ObjectDef::UserType, name, pd->nextObjectId++ ); 

		LexJoin *join = 0;
		if ( IgnoreDef.opt_lex_expr().lex_expr() != 0 ) {
			LexExpression *expr = walkLexExpr( IgnoreDef.opt_lex_expr().lex_expr() );
			join = LexJoin::cons( expr );
		}

		defineToken( IgnoreDef.IGNORE().loc(), name, join, objectDef, 0, true, false, false );
	}

	LangExpr *walkCodeMultiplicitive( code_multiplicitive mult )
	{
		LangExpr *expr = 0;
		LangExpr *right = walkCodeUnary( mult.code_unary() );
		switch ( mult.prodName() ) {
		case code_multiplicitive::_Star: {
			LangExpr *left = walkCodeMultiplicitive( mult._code_multiplicitive() );
			expr = LangExpr::cons( mult.STAR().loc(), left, '*', right );
			break;
		}
		case code_multiplicitive::_Fslash: {
			LangExpr *left = walkCodeMultiplicitive( mult._code_multiplicitive() );
			expr = LangExpr::cons( mult.FSLASH().loc(), left, '/', right );
			break;
		}
		case code_multiplicitive::_Base: {
			expr = right;
			break;
		}}
		return expr;
	}

	PatternItemList *walkPatternElTypeOrLit( pattern_el_lel typeOrLit, LangVarRef *patternVarRef )
	{
		NamespaceQual *nspaceQual = walkRegionQual( typeOrLit.region_qual() );
		RepeatType repeatType = walkOptRepeat( typeOrLit.opt_repeat() );

		PatternItemList *list = 0;
		switch ( typeOrLit.prodName() ) {
		case pattern_el_lel::_Id: {
			String id = typeOrLit.id().data();
			list = patternElNamed( typeOrLit.id().loc(), patternVarRef, nspaceQual, id, repeatType );
			break;
		}
		case pattern_el_lel::_Lit: {
			String lit = typeOrLit.lit().data();
			list = patternElType( typeOrLit.lit().loc(), patternVarRef, nspaceQual, lit, repeatType );
			break;
		}}
		return list;
	}

	LangVarRef *walkOptLabel( opt_label optLabel )
	{
		Context *context = contextStack.length() == 0 ? 0 : contextStack.top();

		LangVarRef *varRef = 0;
		if ( optLabel.prodName() == opt_label::_Id ) {
			String id = optLabel.id().data();
			varRef = LangVarRef::cons( optLabel.id().loc(),
					context, pd->curLocalFrame->curScope, id );
		}
		return varRef;
	}

	PatternItemList *walkPatternEl( pattern_el patternEl, LangVarRef *patternVarRef )
	{
		PatternItemList *list = 0;
		switch ( patternEl.prodName() ) {
		case pattern_el::_Dq: {
			list = walkLitpatElList( patternEl.LitpatElList(), patternEl.dq_lit_term().CONS_NL(), patternVarRef );
			break;
		}
		case pattern_el::_Tilde: {
			String patternData = patternEl.opt_tilde_data().text().c_str();
			patternData += '\n';
			PatternItem *patternItem = PatternItem::cons( patternEl.opt_tilde_data().loc(),
					patternData, PatternItem::InputText );
			list = PatternItemList::cons( patternItem );
			break;
		}
		case pattern_el::_PatternEl: {
			PatternItemList *typeOrLitList = walkPatternElTypeOrLit( patternEl.pattern_el_lel(), patternVarRef );
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
		case litpat_el::_ConsData: {
			String consData = unescape( litpatEl.cons_data().text().c_str() );
			PatternItem *patternItem = PatternItem::cons( litpatEl.cons_data().loc(),
					consData, PatternItem::InputText );
			list = PatternItemList::cons( patternItem );
			break;
		}
		case litpat_el::_SubList: {
			list = walkPatternElList( litpatEl.PatternElList(), patternVarRef );
			break;
		}}
		return list;
	}

	PatternItemList *walkLitpatElList( _repeat_litpat_el litpatElList, CONS_NL Nl, LangVarRef *patternVarRef )
	{
		PatternItemList *list = new PatternItemList;
		while ( !litpatElList.end() ) {
			PatternItemList *tail = walkLitpatEl( litpatElList.value(), patternVarRef );
			list = patListConcat( list, tail );
			litpatElList = litpatElList.next();
		}

		if ( Nl != 0 ) {
			String nl = unescape( Nl.data() );
			PatternItem *patternItem = PatternItem::cons( Nl.loc(), nl, PatternItem::InputText );
			PatternItemList *term = PatternItemList::cons( patternItem );
			list = patListConcat( list, term );
		}

		return list;
	}

	PatternItemList *walkPatternElList( _repeat_pattern_el patternElList, LangVarRef *patternVarRef )
	{
		PatternItemList *list = new PatternItemList;
		while ( !patternElList.end() ) {
			PatternItemList *tail = walkPatternEl( patternElList.value(), patternVarRef );
			list = patListConcat( list, tail );
			patternElList = patternElList.next();
		}
		return list;
	}

	PatternItemList *walkPattternTopEl( pattern_top_el patternTopEl, LangVarRef *patternVarRef )
	{
		PatternItemList *list = 0;
		switch ( patternTopEl.prodName() ) {
		case pattern_top_el::_Dq: {
			list = walkLitpatElList( patternTopEl.LitpatElList(), patternTopEl.dq_lit_term().CONS_NL(), patternVarRef );
			break;
		}
		case pattern_top_el::_Tilde: {
			String patternData = patternTopEl.opt_tilde_data().text().c_str();
			patternData += '\n';
			PatternItem *patternItem = PatternItem::cons( patternTopEl.opt_tilde_data().loc(),
					patternData, PatternItem::InputText );
			list = PatternItemList::cons( patternItem );
			break;
		}
		case pattern_top_el::_SubList: {
			list = walkPatternElList( patternTopEl.PatternElList(), patternVarRef );
			break;
		}}
		return list;
	}

	PatternItemList *walkPatternList( pattern_list patternList, LangVarRef *patternVarRef )
	{
		PatternItemList *list = 0;
		switch ( patternList.prodName() ) {
		case pattern_list::_List: {
			PatternItemList *left = walkPatternList( patternList._pattern_list(), patternVarRef );
			PatternItemList *right = walkPattternTopEl( patternList.pattern_top_el(), patternVarRef );
			list = patListConcat( left, right );
			break;
		}
		case pattern_list::_Base: {
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
		if ( optDefInit.prodName() == opt_def_init::_Init )
			expr = walkCodeExpr( optDefInit.code_expr() );
		return expr;
	}

	LangStmt *walkExportDef( export_def exportDef )
	{
		ObjectField *objField = walkVarDef( exportDef.var_def() );
		LangExpr *expr = walkOptDefInit( exportDef.opt_def_init() );

		return exportStmt( objField, LangStmt::AssignType, expr );
	}

	LangStmt *walkGlobalDef( global_def GlobalDef )
	{
		ObjectField *objField = walkVarDef( GlobalDef.var_def() );
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
		if ( optTranslate.prodName() == opt_translate::_Translate ) {
			ObjectDef *localFrame = blockOpen();
			StmtList *stmtList = walkLangStmtList( optTranslate.lang_stmt_list() );
			block = CodeBlock::cons( stmtList, localFrame );
			block->context = contextStack.top();
			blockClose();
		}
		return block;
	}

	PredDecl *walkPredToken( pred_token predToken )
	{
		NamespaceQual *nspaceQual = walkRegionQual( predToken.region_qual() );
		PredDecl *predDecl = 0;
		switch ( predToken.prodName() ) {
		case pred_token::_Id: {
			String id = predToken.id().data();
			predDecl = predTokenName( predToken.id().loc(), nspaceQual, id );
			break;
		}
		case pred_token::_Lit: {
			String lit = predToken.lit().data();
			predDecl = predTokenLit( predToken.lit().loc(), lit, nspaceQual );
			break;
		}}
		return predDecl;
	}

	PredDeclList *walkPredTokenList( pred_token_list predTokenList )
	{
		PredDeclList *list = 0;
		switch ( predTokenList.prodName() ) {
		case pred_token_list::_List: {
			list = walkPredTokenList( predTokenList._pred_token_list() );
			PredDecl *predDecl = walkPredToken( predTokenList.pred_token() );
			list->append( predDecl );
			break;
		}
		case pred_token_list::_Base: {
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
		case pred_type::_Left:
			pt = PredLeft;
			break;
		case pred_type::_Right:
			pt = PredRight;
			break;
		case pred_type::_NonAssoc:
			pt = PredNonassoc;
			break;
		}

		return pt;
	}

	void walkPrecedenceDef( precedence_def precedenceDef )
	{
		PredType predType = walkPredType( precedenceDef.pred_type() );
		PredDeclList *predDeclList = walkPredTokenList( precedenceDef.pred_token_list() );
		precedenceStmt( predType, predDeclList );
	}

	StmtList *walkInclude( include Include )
	{
		String lit = Include.lit().data();
		String file;
		bool unused;
		prepareLitString( file, unused, lit, Include.lit().loc() );

		const char *argv[2];
		argv[0] = file.data;
		argv[1] = 0;

		colm_program *program = colm_new_program( &colm_object );
		colm_run_program( program, 1, argv );

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
		colm_delete_program( program );
		return stmtList;
	}


	NamespaceQual *walkRegionQual( region_qual regionQual )
	{
		NamespaceQual *qual = 0;
		switch ( regionQual.prodName() ) {
		case region_qual::_Qual: {
			qual = walkRegionQual( regionQual._region_qual() );
			qual->qualNames.append( String( regionQual.id().data() ) );
			break;
		}
		case region_qual::_Base: {
			qual = NamespaceQual::cons( namespaceStack.top() );
			break;
		}}
		return qual;
	}

	RepeatType walkOptRepeat( opt_repeat OptRepeat )
	{
		RepeatType repeatType = RepeatNone;
		switch ( OptRepeat.prodName() ) {
		case opt_repeat::_Star:
			repeatType = RepeatRepeat;
			break;
		case opt_repeat::_Plus:
			repeatType = RepeatList;
			break;
		case opt_repeat::_Question:
			repeatType = RepeatOpt;
			break;
		}
		return repeatType;
	}

	TypeRef *walkTypeRef( type_ref typeRef )
	{
		TypeRef *tr = 0;
		switch ( typeRef.prodName() ) {
		case type_ref::_Id: {
			NamespaceQual *nspaceQual = walkRegionQual( typeRef.region_qual() );
			String id = typeRef.id().data();
			RepeatType repeatType = walkOptRepeat( typeRef.opt_repeat() );
			tr = TypeRef::cons( typeRef.id().loc(), nspaceQual, id, repeatType );
			break;
		}
		case type_ref::_Ptr: {
			NamespaceQual *nspaceQual = walkRegionQual( typeRef.region_qual() );
			String id = typeRef.id().data();
			RepeatType repeatType = walkOptRepeat( typeRef.opt_repeat() );
			TypeRef *inner = TypeRef::cons( typeRef.id().loc(), nspaceQual, id, repeatType );
			tr = TypeRef::cons( typeRef.id().loc(), TypeRef::Ptr, inner );
			break;
		}
		case type_ref::_Map: {
			TypeRef *key = walkTypeRef( typeRef.MapKeyType() );
			TypeRef *value = walkTypeRef( typeRef.MapValueType() );
			tr = TypeRef::cons( typeRef.loc(), TypeRef::Map, 0, key, value );
			break;
		}
		case type_ref::_List: {
			TypeRef *type = walkTypeRef( typeRef._type_ref() );
			tr = TypeRef::cons( typeRef.loc(), TypeRef::List, 0, type, 0 );
			break;
		}
		case type_ref::_Vector: {
			TypeRef *type = walkTypeRef( typeRef._type_ref() );
			tr = TypeRef::cons( typeRef.loc(), TypeRef::Vector, 0, type, 0 );
			break;
		}
		case type_ref::_Parser: {
			TypeRef *type = walkTypeRef( typeRef._type_ref() );
			tr = TypeRef::cons( typeRef.loc(), TypeRef::Parser, 0, type, 0 );
			break;
		}}
		return tr;
	}

	StmtList *walkBlockOrSingle( block_or_single blockOrSingle )
	{
		StmtList *stmtList = 0;
		switch ( blockOrSingle.prodName() ) {
		case block_or_single::_Single: {
			stmtList = new StmtList;
			LangStmt *stmt = walkStatement( blockOrSingle.statement() );
			stmtList->append( stmt );
			break;
		}
		case block_or_single::_Block: {
			stmtList = walkLangStmtList( blockOrSingle.lang_stmt_list() );
			break;
		}}

		return stmtList;
	}

	void walkProdEl( const String &defName, ProdElList *list, prod_el El )
	{
		ObjectField *captureField = 0;
		if ( El.opt_prod_el_name().prodName() == opt_prod_el_name::_Name ) {
			String fieldName = El.opt_prod_el_name().id().data();
			captureField = ObjectField::cons( El.opt_prod_el_name().id().loc(), 0, fieldName );
		}
		else {
			/* default the prod name. */
			if ( El.prodName() == prod_el::_Id ) {
				String fieldName = El.id().data();
				opt_repeat::prod_name orpn = El.opt_repeat().prodName();
				if ( orpn == opt_repeat::_Star )
					fieldName = "_repeat_" + fieldName;
				else if ( orpn == opt_repeat::_Plus )
					fieldName = "_list_" + fieldName;
				else if ( orpn == opt_repeat::_Question )
					fieldName = "_opt_" + fieldName;
				else if ( strcmp( fieldName, defName ) == 0 )
					fieldName = "_" + fieldName;
				captureField = ObjectField::cons( El.id().loc(), 0, fieldName );
			}
		}

		RepeatType repeatType = walkOptRepeat( El.opt_repeat() );
		NamespaceQual *nspaceQual = walkRegionQual( El.region_qual() );

		switch ( El.prodName() ) {
		case prod_el::_Id: {
			String typeName = El.id().data();
			ProdEl *prodEl = prodElName( El.id().loc(), typeName,
					nspaceQual, captureField, repeatType, false );
			appendProdEl( list, prodEl );
			break;
		}
		case prod_el::_Lit: {
			String lit = El.lit().data();
			ProdEl *prodEl = prodElLiteral( El.lit().loc(), lit,
					nspaceQual, captureField, repeatType, false );
			appendProdEl( list, prodEl );
			break;
		}}
	}

	void walkProdElList( const String &defName, ProdElList *list, prod_el_list ProdElList )
	{
		if ( ProdElList.prodName() == prod_el_list::_List ) {
			prod_el_list RightProdElList = ProdElList._prod_el_list();
			walkProdElList( defName, list, RightProdElList );
			walkProdEl( defName, list, ProdElList.prod_el() );
		}
	}

	CodeBlock *walkOptReduce( opt_reduce OptReduce )
	{
		CodeBlock *block = 0;
		if ( OptReduce.prodName() == opt_reduce::_Reduce ) {
			ObjectDef *localFrame = blockOpen();
			StmtList *stmtList = walkLangStmtList( OptReduce.lang_stmt_list() );

			block = CodeBlock::cons( stmtList, localFrame );
			block->context = contextStack.top();

			blockClose();
		}
		return block;
	}

	void walkProdudction( const String &defName, LelDefList *lelDefList, prod Prod )
	{
		ProdElList *list = new ProdElList;

		walkProdElList( defName, list, Prod.prod_el_list() );

		String name;
		if ( Prod.opt_prod_name().prodName() == opt_prod_name::_Name )
			name = Prod.opt_prod_name().id().data();

		CodeBlock *codeBlock = walkOptReduce( Prod.opt_reduce() );
		bool commit = Prod.opt_commit().prodName() == opt_commit::_Commit;

		Production *prod = BaseParser::production( Prod.SQOPEN().loc(), list, name, commit, codeBlock, 0 );
		prodAppend( lelDefList, prod );
	}

	void walkProdList( const String &name, LelDefList *lelDefList, prod_list ProdList )
	{
		if ( ProdList.prodName() == prod_list::_List ) 
			walkProdList( name, lelDefList, ProdList._prod_list() );

		walkProdudction( name, lelDefList, ProdList.prod() );
	}

	ReOrItem *walkRegOrChar( reg_or_char regOrChar )
	{
		ReOrItem *orItem = 0;
		switch ( regOrChar.prodName() ) {
		case reg_or_char::_Char: {
			String c = unescape( regOrChar.RE_CHAR().data() );
			orItem = ReOrItem::cons( regOrChar.RE_CHAR().loc(), c );
			break;
		}
		case reg_or_char::_Range: {
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
		case reg_or_data::_Data: {
			ReOrBlock *left = walkRegOrData( regOrData._reg_or_data() );
			ReOrItem *right = walkRegOrChar( regOrData.reg_or_char() );
			block = lexRegularExprData( left, right );
			break;
		}
		case reg_or_data::_Base: {
			block = ReOrBlock::cons();
			break;
		}}
		return block;
	}

	LexFactorNeg *walkLexFactorNeg( lex_factor_neg lexFactorNeg )
	{
		LexFactorNeg *factorNeg = 0;
		switch ( lexFactorNeg.prodName() ) {
		case lex_factor_neg::_Caret: {
			LexFactorNeg *recNeg = walkLexFactorNeg( lexFactorNeg._lex_factor_neg() );
			factorNeg = LexFactorNeg::cons( recNeg, LexFactorNeg::CharNegateType );
			break;
		}
		case lex_factor_neg::_Base: {
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

		if ( pn != lex_factor_rep::_Base )
			recRep = walkLexFactorRep( lexFactorRep._lex_factor_rep() );

		switch ( pn ) {
		case lex_factor_rep::_Star: {
			factorRep = LexFactorRep::cons( lexFactorRep.LEX_STAR().loc(),
					recRep, 0, 0, LexFactorRep::StarType );
			break;
		}
		case lex_factor_rep::_StarStar: {
			factorRep = LexFactorRep::cons( lexFactorRep.LEX_STARSTAR().loc(),
					recRep, 0, 0, LexFactorRep::StarStarType );
			break;
		}
		case lex_factor_rep::_Plus: {
			factorRep = LexFactorRep::cons( lexFactorRep.LEX_PLUS().loc(),
					recRep, 0, 0, LexFactorRep::PlusType );
			break;
		}
		case lex_factor_rep::_Question: {
			factorRep = LexFactorRep::cons( lexFactorRep.LEX_QUESTION().loc(),
					recRep, 0, 0, LexFactorRep::OptionalType );
			break;
		}
		case lex_factor_rep::_Exact: {
			int low = atoi( lexFactorRep.lex_uint().data()->data );
			factorRep = LexFactorRep::cons( lexFactorRep.lex_uint().loc(),
					recRep, low, 0, LexFactorRep::ExactType );
			break;
		}
		case lex_factor_rep::_Max: {
			int high = atoi( lexFactorRep.lex_uint().data()->data );
			factorRep = LexFactorRep::cons( lexFactorRep.lex_uint().loc(),
					recRep, 0, high, LexFactorRep::MaxType );
			break;
		}
		case lex_factor_rep::_Min: {
			int low = atoi( lexFactorRep.lex_uint().data()->data );
			factorRep = LexFactorRep::cons( lexFactorRep.lex_uint().loc(),
					recRep, low, 0, LexFactorRep::MinType );
			break;
		}
		case lex_factor_rep::_Range: {
			int low = atoi( lexFactorRep.Low().data()->data );
			int high = atoi( lexFactorRep.High().data()->data );
			factorRep = LexFactorRep::cons( lexFactorRep.Low().loc(),
					recRep, low, high, LexFactorRep::RangeType );
			break;
		}
		case lex_factor_rep::_Base: {
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
		if ( pn != lex_term::_Base )
			leftTerm = walkLexTerm( lexTerm._lex_term() );

		LexFactorAug *factorAug = walkLexFactorAug( lexTerm.lex_factor_rep() );

		switch ( pn ) {
		case lex_term::_Dot:
			term = LexTerm::cons( leftTerm, factorAug, LexTerm::ConcatType );
			break;
		case lex_term::_ColonGt:
			term = LexTerm::cons( leftTerm, factorAug, LexTerm::RightStartType );
			break;
		case lex_term::_ColonGtGt:
			term = LexTerm::cons( leftTerm, factorAug, LexTerm::RightFinishType );
			break;
		case lex_term::_LtColon:
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
		if ( pn != lex_expr::_Base )
			leftExpr = walkLexExpr( lexExpr._lex_expr() );

		LexTerm *term = walkLexTerm( lexExpr.lex_term() );

		switch ( pn ) {
		case lex_expr::_Bar:
			expr = LexExpression::cons( leftExpr, term, LexExpression::OrType );
			break;
		case lex_expr::_Amp:
			expr = LexExpression::cons( leftExpr, term, LexExpression::IntersectType );
			break;
		case lex_expr::_Dash:
			expr = LexExpression::cons( leftExpr, term, LexExpression::SubtractType );
			break;
		case lex_expr::_DashDash:
			expr = LexExpression::cons( leftExpr, term, LexExpression::StrongSubtractType );
			break;
		case lex_expr::_Base:
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

		addRegularDef( rlDef.id().loc(), namespaceStack.top(), id, join );
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

		NtDef *ntDef = NtDef::cons( name, namespaceStack.top(),
				contextStack.top(), reduceFirst );

		BaseParser::cflDef( ntDef, objectDef, defList );
	}

	CallArgVect *walkCodeExprList( _repeat_code_expr codeExprList )
	{
		CallArgVect *callArgVect = new CallArgVect;
		while ( !codeExprList.end() ) {
			code_expr codeExpr = codeExprList.value();
			LangExpr *expr = walkCodeExpr( codeExpr );
			callArgVect->append( new CallArg(expr) );
			codeExprList = codeExprList.next();
		}
		return callArgVect;
	}

	LangStmt *walkPrintStmt( print_stmt &printStmt )
	{
		_repeat_code_expr codeExprList = printStmt.CodeExprList();
		CallArgVect *exprVect = walkCodeExprList( codeExprList );

		LangStmt::Type type = LangStmt::PrintType;
		switch ( printStmt.prodName() ) {
		case print_stmt::_Tree:
			type = LangStmt::PrintType;
			break;
		case print_stmt::_PrintStream:
			type = LangStmt::PrintStreamType;
			break;
		case print_stmt::_Xml:
			type = LangStmt::PrintXMLType;
			break;
		case print_stmt::_XmlAc:
			type = LangStmt::PrintXMLACType;
			break;
		}
			
		return LangStmt::cons( printStmt.POPEN().loc(), type, exprVect );
	}

	QualItemVect *walkQual( qual &Qual )
	{
		QualItemVect *qualItemVect = 0;
		qual RecQual = Qual._qual();
		switch ( Qual.prodName() ) {
		case qual::_Dot:
		case qual::_Arrow: {
			qualItemVect = walkQual( RecQual );
			String id = Qual.id().data();
			QualItem::Form form = Qual.DOT() != 0 ? QualItem::Dot : QualItem::Arrow;
			qualItemVect->append( QualItem( form, Qual.id().loc(), id ) );
			break;
		}
		case qual::_Base: {
			qualItemVect = new QualItemVect;
			break;
		}}
		return qualItemVect;
	}

	LangVarRef *walkVarRef( var_ref varRef )
	{
		Context *context = contextStack.length() == 0 ? 0 : contextStack.top();

		qual Qual = varRef.qual();
		QualItemVect *qualItemVect = walkQual( Qual );
		String id = varRef.id().data();
		LangVarRef *langVarRef = LangVarRef::cons( varRef.id().loc(),
				context, pd->curLocalFrame->curScope, qualItemVect, id );
		return langVarRef;
	}

	ObjectField *walkOptCapture( opt_capture optCapture )
	{
		ObjectField *objField = 0;
		if ( optCapture.prodName() == opt_capture::_Id ) {
			String id = optCapture.id().data();
			objField = ObjectField::cons( optCapture.id().loc(), 0, id );
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
		case lit_cons_el::_ConsData: {
			String consData = unescape( litConsEl.cons_data().text().c_str() );
			ConsItem *consItem = ConsItem::cons( litConsEl.cons_data().loc(),
					ConsItem::InputText, consData );
			list = ConsItemList::cons( consItem );
			break;
		}
		case lit_cons_el::_SubList: {
			list = walkConsElList( litConsEl.ConsElList(), consTypeRef );
			break;
		}}
		return list;
	}

	ConsItemList *walkLitConsElList( _repeat_lit_cons_el litConsElList, CONS_NL Nl, TypeRef *consTypeRef )
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

	ConsItemList *walkConsEl( cons_el consEl, TypeRef *consTypeRef )
	{
		ConsItemList *list = 0;
		switch ( consEl.prodName() ) {
		case cons_el::_Lit: {
			NamespaceQual *nspaceQual = walkRegionQual( consEl.region_qual() );
			String lit = consEl.lit().data();
			list = consElLiteral( consEl.lit().loc(), consTypeRef, lit, nspaceQual );
			break;
		}
		case cons_el::_Tilde: {
			String consData = consEl.opt_tilde_data().text().c_str();
			consData += '\n';
			ConsItem *consItem = ConsItem::cons( consEl.opt_tilde_data().loc(),
					ConsItem::InputText, consData );
			list = ConsItemList::cons( consItem );
			break;
		}
		case cons_el::_CodeExpr: {
			LangExpr *consExpr = walkCodeExpr( consEl.code_expr() );
			ConsItem *consItem = ConsItem::cons( consExpr->loc, ConsItem::ExprType, consExpr );
			list = ConsItemList::cons( consItem );
			break;
		}
		case cons_el::_Dq: {
			list = walkLitConsElList( consEl.LitConsElList(), consEl.dq_lit_term().CONS_NL(), consTypeRef );
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
		case cons_top_el::_Dq: {
			list = walkLitConsElList( consTopEl.LitConsElList(), consTopEl.dq_lit_term().CONS_NL(), consTypeRef );
			break;
		}
		case cons_top_el::_Tilde: {
			String consData = consTopEl.opt_tilde_data().text().c_str();
			consData += '\n';
			ConsItem *consItem = ConsItem::cons( consTopEl.opt_tilde_data().loc(),
					ConsItem::InputText, consData );
			list = ConsItemList::cons( consItem );
			break;
		}
		case cons_top_el::_SubList: {
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
		case lit_string_el::_ConsData: {
			String consData = unescape( litStringEl.cons_data().text().c_str() );
			ConsItem *stringItem = ConsItem::cons( litStringEl.cons_data().loc(),
					ConsItem::InputText, consData );
			list = ConsItemList::cons( stringItem );
			break;
		}
		case lit_string_el::_SubList: {
			list = walkStringElList( litStringEl.StringElList() );
			break;
		}}
		return list;
	}

	ConsItemList *walkLitStringElList( _repeat_lit_string_el litStringElList, CONS_NL Nl )
	{
		ConsItemList *list = new ConsItemList;
		while ( !litStringElList.end() ) {
			ConsItemList *extension = walkLitStringEl( litStringElList.value() );
			list = consListConcat( list, extension );
			litStringElList = litStringElList.next();
		}

		if ( Nl != 0 ) {
			String consData = unescape( Nl.data() );
			ConsItem *consItem = ConsItem::cons( Nl.loc(), ConsItem::InputText, consData );
			ConsItemList *term = ConsItemList::cons( consItem );
			list = consListConcat( list, term );
		}
		return list;
	}

	ConsItemList *walkStringEl( string_el stringEl )
	{
		ConsItemList *list = 0;
		switch ( stringEl.prodName() ) {
		case string_el::_Dq: {
			list = walkLitStringElList( stringEl.LitStringElList(), stringEl.dq_lit_term().CONS_NL() );
			break;
		}
		case string_el::_Tilde: {
			String consData = stringEl.opt_tilde_data().text().c_str();
			consData += '\n';
			ConsItem *consItem = ConsItem::cons( stringEl.opt_tilde_data().loc(),
					ConsItem::InputText, consData );
			list = ConsItemList::cons( consItem );
			break;
		}
		case string_el::_CodeExpr: {
			LangExpr *consExpr = walkCodeExpr( stringEl.code_expr() );
			ConsItem *consItem = ConsItem::cons( consExpr->loc, ConsItem::ExprType, consExpr );
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
		case string_top_el::_Dq: {
			list = walkLitStringElList( stringTopEl.LitStringElList(), stringTopEl.dq_lit_term().CONS_NL() );
			break;
		}
		case string_top_el::_Tilde: {
			String consData = stringTopEl.opt_tilde_data().text().c_str();
			consData += '\n';
			ConsItem *consItem = ConsItem::cons( stringTopEl.opt_tilde_data().loc(),
					ConsItem::InputText, consData );
			list = ConsItemList::cons( consItem );
			break;
		}
		case string_top_el::_SubList: {
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

	ConsItemList *walkString( cstring String )
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
		case lit_accum_el::_ConsData: {
			String consData = unescape( litAccumEl.cons_data().text().c_str() );
			ConsItem *consItem = ConsItem::cons( litAccumEl.cons_data().loc(),
					ConsItem::InputText, consData );
			list = ConsItemList::cons( consItem );
			break;
		}
		case lit_accum_el::_SubList: {
			list = walkAccumElList( litAccumEl.AccumElList() );
			break;
		}}
		return list;
	}

	ConsItemList *walkLitAccumElList( _repeat_lit_accum_el litAccumElList, CONS_NL Nl )
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
		case accum_el::_Dq: {
			list = walkLitAccumElList( accumEl.LitAccumElList(), accumEl.dq_lit_term().CONS_NL() );
			break;
		}
		case accum_el::_Tilde: {
			String consData = accumEl.opt_tilde_data().text().c_str();
			consData += '\n';
			ConsItem *consItem = ConsItem::cons( accumEl.opt_tilde_data().loc(),
					ConsItem::InputText, consData );
			list = ConsItemList::cons( consItem );
			break;
		}
		case accum_el::_CodeExpr: {
			LangExpr *accumExpr = walkCodeExpr( accumEl.code_expr() );
			ConsItem *consItem = ConsItem::cons( accumExpr->loc, ConsItem::ExprType, accumExpr );
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
		case accum_top_el::_Dq: {
			list = walkLitAccumElList( accumTopEl.LitAccumElList(), accumTopEl.dq_lit_term().CONS_NL() );
			break;
		}
		case accum_top_el::_Tilde: {
			String consData = accumTopEl.opt_tilde_data().text().c_str();
			consData += '\n';
			ConsItem *consItem = ConsItem::cons( accumTopEl.opt_tilde_data().loc(),
					ConsItem::InputText, consData );
			list = ConsItemList::cons( consItem );
			break;
		}
		case accum_top_el::_SubList: {
			list = walkAccumElList( accumTopEl.AccumElList() );
			break;
		}}
		return list;
	}

	ConsItemList *walkAccumList( accum_list accumList )
	{
		ConsItemList *list = walkAccumTopEl( accumList.accum_top_el() );

		if ( accumList.prodName() == accum_list::_List ) {
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

	FieldInitVect *walkOptFieldInit( opt_field_init optFieldInit )
	{
		FieldInitVect *list = 0;
		if ( optFieldInit.prodName() == opt_field_init::_Init ) {
			list = new FieldInitVect;
			_repeat_field_init fieldInitList = optFieldInit.FieldInitList();
			while ( !fieldInitList.end() ) {
				walkFieldInit( list, fieldInitList.value() );
				fieldInitList = fieldInitList.next();
			}
		}
		return list;
	}

	LangExpr *walkCodeFactor( code_factor codeFactor )
	{
		LangExpr *expr = 0;
		switch ( codeFactor.prodName() ) {
		case code_factor::_VarRef: {
			LangVarRef *langVarRef = walkVarRef( codeFactor.var_ref() );
			LangTerm *term = LangTerm::cons( langVarRef->loc, LangTerm::VarRefType, langVarRef );
			expr = LangExpr::cons( term );
			break;
		}
		case code_factor::_Call: {
			LangVarRef *langVarRef = walkVarRef( codeFactor.var_ref() );
			CallArgVect *exprVect = walkCodeExprList( codeFactor.CodeExprList() );
			LangTerm *term = LangTerm::cons( langVarRef->loc, langVarRef, exprVect );
			expr = LangExpr::cons( term );
			break;
		}
		case code_factor::_Number: {
			String number = codeFactor.number().text().c_str();
			LangTerm *term = LangTerm::cons( codeFactor.number().loc(),
					LangTerm::NumberType, number );
			expr = LangExpr::cons( term );
			break;
		}
		case code_factor::_Lit: {
			String lit = codeFactor.lit().data();
			LangTerm *term = LangTerm::cons( codeFactor.lit().loc(), LangTerm::StringType, lit );
			expr = LangExpr::cons( term );
			break;
		}
		case code_factor::_Parse: {
			/* The type we are parsing. */
			type_ref typeRefTree = codeFactor.type_ref();
			TypeRef *typeRef = walkTypeRef( typeRefTree );
			ObjectField *objField = walkOptCapture( codeFactor.opt_capture() );
			FieldInitVect *init = walkOptFieldInit( codeFactor.opt_field_init() );
			ConsItemList *list = walkAccumulate( codeFactor.accumulate() );

			expr = parseCmd( codeFactor.PARSE().loc(), false, objField, typeRef, init, list );
			break;
		}
		case code_factor::_ParseStop: {
			/* The type we are parsing. */
			type_ref typeRefTree = codeFactor.type_ref();
			TypeRef *typeRef = walkTypeRef( typeRefTree );
			ObjectField *objField = walkOptCapture( codeFactor.opt_capture() );
			FieldInitVect *init = walkOptFieldInit( codeFactor.opt_field_init() );
			ConsItemList *list = walkAccumulate( codeFactor.accumulate() );

			expr = parseCmd( codeFactor.PARSE_STOP().loc(), true, objField, typeRef, init, list );
			break;
		}
		case code_factor::_Cons: {
			/* The type we are parsing. */
			type_ref typeRefTree = codeFactor.type_ref();
			TypeRef *typeRef = walkTypeRef( typeRefTree );
			ObjectField *objField = walkOptCapture( codeFactor.opt_capture() );
			ConsItemList *list = walkConstructor( codeFactor.constructor(), typeRef );
			FieldInitVect *init = walkOptFieldInit( codeFactor.opt_field_init() );

			expr = construct( codeFactor.CONS().loc(), objField, list, typeRef, init );
			break;
		}
		case code_factor::_Send: {
			LangVarRef *varRef = walkVarRef( codeFactor.var_ref() );
			ConsItemList *list = walkAccumulate( codeFactor.accumulate() );
			bool eof = walkOptEos( codeFactor.opt_eos() );
			expr = send( codeFactor.SEND().loc(), varRef, list, eof );
			break;
		}
		case code_factor::_Nil: {
			expr = LangExpr::cons( LangTerm::cons( codeFactor.NIL().loc(),
					LangTerm::NilType ) );
			break;
		}
		case code_factor::_True: {
			expr = LangExpr::cons( LangTerm::cons( codeFactor.TRUE().loc(),
					LangTerm::TrueType ) );
			break;
		}
		case code_factor::_False: {
			expr = LangExpr::cons( LangTerm::cons( codeFactor.FALSE().loc(),
					LangTerm::FalseType ) );
			break;
		}
		case code_factor::_Paren: {
			expr = walkCodeExpr( codeFactor.code_expr() );
			break;
		}
		case code_factor::_String: {
			ConsItemList *list = walkString( codeFactor.cstring() );
			expr = LangExpr::cons( LangTerm::cons( codeFactor.cstring().loc(), list ) );
			break;
		}
		case code_factor::_Match: {
			LangVarRef *varRef = walkVarRef( codeFactor.var_ref() );
			PatternItemList *list = walkPattern( codeFactor.pattern(), varRef );
			expr = match( codeFactor.loc(), varRef, list );
			break;
		}
		case code_factor::_In: {
			TypeRef *typeRef = walkTypeRef( codeFactor.type_ref() );
			LangVarRef *varRef = walkVarRef( codeFactor.var_ref() );
			expr = LangExpr::cons( LangTerm::cons( typeRef->loc,
					LangTerm::SearchType, typeRef, varRef ) );
			break;
		}
		case code_factor::_MakeTree: {
			CallArgVect *exprList = walkCodeExprList( codeFactor.MakeTreeExprList() );
			expr = LangExpr::cons( LangTerm::cons( codeFactor.loc(),
					LangTerm::MakeTreeType, exprList ) );
			break;
		}
		case code_factor::_MakeToken: {
			CallArgVect *exprList = walkCodeExprList( codeFactor.MakeTokenExprList() );
			expr = LangExpr::cons( LangTerm::cons( codeFactor.loc(),
					LangTerm::MakeTokenType, exprList ) );
			break;
		}
		case code_factor::_TypeId: {
			TypeRef *typeRef = walkTypeRef( codeFactor.type_ref() );
			expr = LangExpr::cons( LangTerm::cons( codeFactor.loc(),
					LangTerm::TypeIdType, typeRef ) );
			break;
		}
		case code_factor::_New: {
			LangExpr *newExpr = walkCodeFactor( codeFactor._code_factor() );
			expr = LangExpr::cons( LangTerm::cons( codeFactor.loc(),
					LangTerm::NewType, newExpr ) );
			break;
		}
		case code_factor::_Cast: {
			TypeRef *typeRef = walkTypeRef( codeFactor.type_ref() );
			LangExpr *castExpr = walkCodeFactor( codeFactor._code_factor() );
			expr = LangExpr::cons( LangTerm::cons( codeFactor.loc(),
					LangTerm::CastType, typeRef, castExpr ) );
			break;
		}}
		return expr;
	}

	LangExpr *walkCodeAdditive( code_additive additive )
	{
		LangExpr *expr = 0;
		switch ( additive.prodName() ) {
		case code_additive::_Plus: {
			LangExpr *left = walkCodeAdditive( additive._code_additive() );
			LangExpr *right = walkCodeMultiplicitive( additive.code_multiplicitive() );
			expr = LangExpr::cons( additive.PLUS().loc(), left, '+', right );
			break;
		}
		case code_additive::_Minus: {
			LangExpr *left = walkCodeAdditive( additive._code_additive() );
			LangExpr *right = walkCodeMultiplicitive( additive.code_multiplicitive() );
			expr = LangExpr::cons( additive.MINUS().loc(), left, '-', right );
			break;
		}
		case code_additive::_Base: {
			expr = walkCodeMultiplicitive( additive.code_multiplicitive() );
			break;
		}}
		return expr;
	}

	LangExpr *walkCodeUnary( code_unary unary )
	{
		LangExpr *expr = 0, *factor = walkCodeFactor( unary.code_factor() );

		switch ( unary.prodName() ) {
		case code_unary::_Bang: {
			expr = LangExpr::cons( unary.BANG().loc(), '!', factor );
			break;
		}
		case code_unary::_Dollar: {
			expr = LangExpr::cons( unary.DOLLAR().loc(), '$', factor );
			break;
		}
		case code_unary::_Caret: {
			expr = LangExpr::cons( unary.CARET().loc(), '^', factor );
			break;
		}
		case code_unary::_Percent: {
			expr = LangExpr::cons( unary.PERCENT().loc(), '%', factor );
			break;
		}
		case code_unary::_Base: {
			expr = factor;
		}}

		return expr;
	}

	LangExpr *walkCodeRelational( code_relational codeRelational )
	{
		LangExpr *expr = 0, *left = 0;

		if ( codeRelational.prodName() != code_relational::_Base )
			left = walkCodeRelational( codeRelational._code_relational() );

		LangExpr *additive = walkCodeAdditive( codeRelational.code_additive() );

		switch ( codeRelational.prodName() ) {
		case code_relational::_EqEq: {
			expr = LangExpr::cons( codeRelational.loc(), left, OP_DoubleEql, additive );
			break;
		}
		case code_relational::_Neq: {
			expr = LangExpr::cons( codeRelational.loc(), left, OP_NotEql, additive );
			break;
		}
		case code_relational::_Lt: {
			expr = LangExpr::cons( codeRelational.loc(), left, '<', additive );
			break;
		}
		case code_relational::_Gt: {
			expr = LangExpr::cons( codeRelational.loc(), left, '>', additive );
			break;
		}
		case code_relational::_LtEq: {
			expr = LangExpr::cons( codeRelational.loc(), left, OP_LessEql, additive );
			break;
		}
		case code_relational::_GtEq: {
			expr = LangExpr::cons( codeRelational.loc(), left, OP_GrtrEql, additive );
			break;
		}
		case code_relational::_Base: {
			expr = additive;
			break;
		}}
		return expr;
	}

	LangStmt *walkExprStmt( expr_stmt exprStmt )
	{
		LangExpr *expr = walkCodeExpr( exprStmt.code_expr() );
		LangStmt *stmt = LangStmt::cons( expr->loc, LangStmt::ExprType, expr );
		return stmt;
	}

	ObjectField *walkVarDef( var_def varDef )
	{
		String id = varDef.id().data();
		TypeRef *typeRef = walkTypeRef( varDef.type_ref() );
		return ObjectField::cons( varDef.id().loc(), typeRef, id );
	}

	LangIterCall *walkIterCall( iter_call IterCall )
	{
		Context *context = contextStack.length() == 0 ? 0 : contextStack.top();

		LangIterCall *iterCall = 0;
		switch ( IterCall.prodName() ) {
		case iter_call::_Call: {
			LangVarRef *varRef = walkVarRef( IterCall.var_ref() );
			CallArgVect *exprVect = walkCodeExprList( IterCall.CodeExprList() );
			LangTerm *langTerm = LangTerm::cons( varRef->loc, varRef, exprVect );
			iterCall = LangIterCall::cons( LangIterCall::IterCall, langTerm );
			break;
		}
		case iter_call::_Id: {
			String tree = IterCall.id().data();
			LangVarRef *varRef = LangVarRef::cons( IterCall.id().loc(),
					context, pd->curLocalFrame->curScope, tree );
			LangTerm *langTerm = LangTerm::cons( IterCall.id().loc(),
					LangTerm::VarRefType, varRef );
			LangExpr *langExpr = LangExpr::cons( langTerm );
			iterCall = LangIterCall::cons( LangIterCall::VarRef, langExpr );
			break;
		}
		case iter_call::_Expr: {
			LangExpr *langExpr = walkCodeExpr( IterCall.code_expr() );
			iterCall = LangIterCall::cons( LangIterCall::Expr, langExpr );
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
		if ( optionalElse.prodName() == optional_else::_Else ) {
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
			case elsif_list::_Clause:
				stmt = walkElsifClause( elsifList.elsif_clause() );
				stmt->elsePart = walkElsifList( elsifList._elsif_list() );
				break;
			case elsif_list::_OptElse:
				stmt = walkOptionalElse( elsifList.optional_else() );
				break;
		}
		return stmt;
	}

	void walkContextVarDef( context_var_def ContextVarDef )
	{
		ObjectField *objField = walkVarDef( ContextVarDef.var_def() );
		contextVarDef( objField->loc, objField );
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

		switch ( paramVarDef.prodName() ) {
		case param_var_def::_Type: 
			typeRef = walkTypeRef( paramVarDef.type_ref() );
			break;
		case param_var_def::_Ref:
			typeRef = walkReferenceTypeRef( paramVarDef.reference_type_ref() );
			break;
		}
		
		return addParam( paramVarDef.id().loc(), typeRef, id );
	}

	ParameterList *walkParamVarDefList( _repeat_param_var_def paramVarDefList )
	{
		ParameterList *paramList = new ParameterList;
		while ( !paramVarDefList.end() ) {
			ObjectField *param = walkParamVarDef( paramVarDefList.value() );
			appendParam( paramList, param );
			paramVarDefList = paramVarDefList.next();
		}
		return paramList;
	}

	bool walkOptExport( opt_export OptExport )
	{
		return OptExport.prodName() == opt_export::_Export;
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

	void walkIterDef( iter_def IterDef )
	{
		ObjectDef *localFrame = blockOpen();

		String id = IterDef.id().data();
		ParameterList *paramList = walkParamVarDefList( IterDef.ParamVarDefList() );
		StmtList *stmtList = walkLangStmtList( IterDef.lang_stmt_list() );
		iterDef( stmtList, localFrame, paramList, id );

		blockClose();
	}

	void walkContextItem( context_item contextItem )
	{
		switch ( contextItem.prodName() ) {
		case context_item::_Rl:
			walkRlDef( contextItem.rl_def() );
			break;
		case context_item::_ContextVar:
			walkContextVarDef( contextItem.context_var_def() );
			break;
		case context_item::_Token:
			walkTokenDef( contextItem.token_def() );
			break;
		case context_item::_Ignore:
			walkIgnoreDef( contextItem.ignore_def() );
			break;
		case context_item::_Literal:
			walkLiteralDef( contextItem.literal_def() );
			break;
		case context_item::_Cfl:
			walkCflDef( contextItem.cfl_def() );
			break;
		case context_item::_Region:
			walkLexRegion( contextItem.region_def() );
			break;
		case context_item::_Context:
			walkContextDef( contextItem.context_def() );
			break;
		case context_item::_Function:
			walkFunctionDef( contextItem.function_def() );
			break;
		case context_item::_Iter:
			walkIterDef( contextItem.iter_def() );
			break;
		case context_item::_PreEof:
			walkPreEof( contextItem.pre_eof_def() );
			break;
		case context_item::_Export:
			walkExportDef( contextItem.export_def() );
			break;
		case context_item::_Precedence:
			walkPrecedenceDef( contextItem.precedence_def() );
			break;
		}
	}

	void walkContextDef( context_def contextDef )
	{
		String name = contextDef.id().data();
		contextHead( contextDef.id().loc(), name );

		_repeat_context_item contextItemList = contextDef.ContextItemList();
		while ( !contextItemList.end() ) {
			walkContextItem( contextItemList.value() );
			contextItemList = contextItemList.next();
		}

		contextStack.pop();
		namespaceStack.pop();
	}

	void walkNamespaceDef( namespace_def NamespaceDef )
	{
		String name = NamespaceDef.id().data();
		createNamespace( NamespaceDef.id().loc(), name );
		walkRootItemList( NamespaceDef.RootItemList() );
		namespaceStack.pop();
	}

	void walkRootItem( root_item rootItem, StmtList *stmtList )
	{
		switch ( rootItem.prodName() ) {
		case root_item::_Rl:
			walkRlDef( rootItem.rl_def() );
			break;
		case root_item::_Token:
			walkTokenDef( rootItem.token_def() );
			break;
		case root_item::_Ignore:
			walkIgnoreDef( rootItem.ignore_def() );
			break;
		case root_item::_Literal:
			walkLiteralDef( rootItem.literal_def() );
			break;
		case root_item::_Cfl:
			walkCflDef( rootItem.cfl_def() );
			break;
		case root_item::_Region:
			walkLexRegion( rootItem.region_def() );
			break;
		case root_item::_Statement: {
			LangStmt *stmt = walkStatement( rootItem.statement() );
			if ( stmt != 0 )
				stmtList->append( stmt );
			break;
		}
		case root_item::_Context:
			walkContextDef( rootItem.context_def() );
			break;
		case root_item::_Namespace:
			walkNamespaceDef( rootItem.namespace_def() );
			break;
		case root_item::_Function:
			walkFunctionDef( rootItem.function_def() );
			break;
		case root_item::_Iter:
			walkIterDef( rootItem.iter_def() );
			break;
		case root_item::_PreEof:
			walkPreEof( rootItem.pre_eof_def() );
			break;
		case root_item::_Export: {
			LangStmt *stmt = walkExportDef( rootItem.export_def() );
			if ( stmt != 0 )
				stmtList->append( stmt );
			break;
		}
		case root_item::_Alias:
			walkAliasDef( rootItem.alias_def() );
			break;
		case root_item::_Precedence:
			walkPrecedenceDef( rootItem.precedence_def() );
			break;
		case root_item::_Include: {
			StmtList *includeList = walkInclude( rootItem.include() );
			stmtList->append( *includeList );
			break;
		}
		case root_item::_Global: {
			LangStmt *stmt = walkGlobalDef( rootItem.global_def() );
			if ( stmt != 0 )
				stmtList->append( stmt );
			break;
		}}
	}

	bool walkOptNoIgnore( opt_no_ignore OptNoIngore )
	{
		return OptNoIngore.prodName() == opt_no_ignore::_Ni;
	}

	bool walkOptEos( opt_eos OptEos )
	{
		opt_eos::prod_name pn = OptEos.prodName();
		return pn == opt_eos::_Dot || pn == opt_eos::_Eos;
	}

	void walkLiteralItem( literal_item literalItem )
	{
		bool niLeft = walkOptNoIgnore( literalItem.NiLeft() );
		bool niRight = walkOptNoIgnore( literalItem.NiRight() );

		String lit = literalItem.lit().data();
		if ( strcmp( lit, "''" ) == 0 )
			zeroDef( literalItem.lit().loc(), lit, niLeft, niRight );
		else
			literalDef( literalItem.lit().loc(), lit, niLeft, niRight );
	}

	void walkLiteralList( literal_list literalList )
	{
		if ( literalList.prodName() == literal_list::_Item )
			walkLiteralList( literalList._literal_list() );
		walkLiteralItem( literalList.literal_item() );
	}

	void walkLiteralDef( literal_def literalDef )
	{
		walkLiteralList( literalDef.literal_list() );
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

	const char *argv[2];
	argv[0] = inputFileName;
	argv[1] = 0;

	colm_program *program = colm_new_program( &colm_object );
	colm_set_debug( program, activeRealm );
	colm_run_program( program, 1, argv );

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
	colm_delete_program( program );

	pd->rootCodeBlock = CodeBlock::cons( stmtList, 0 );
}

BaseParser *consLoadColm( Compiler *pd, const char *inputFileName )
{
	return new LoadColm( pd, inputFileName );
}
