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
			String lit = lexRangeLit.Lit().data();
			literal = Literal::cons( lexRangeLit.Lit().loc(), lit, Literal::LitString );
			break;
		}
		case lex_range_lit::_Number: {
			String num = lexRangeLit.Number().text().c_str();
			literal = Literal::cons( lexRangeLit.Number().loc(), num, Literal::Number );
			break;
		}}
		return literal;
	}

	LexFactor *walkLexFactor( lex_factor lexFactor )
	{
		LexFactor *factor = 0;
		switch ( lexFactor.prodName() ) {
		case lex_factor::_Literal: {
			String litString = lexFactor.Literal().data();
			Literal *literal = Literal::cons( lexFactor.Literal().loc(),
					litString, Literal::LitString );
			factor = LexFactor::cons( literal );
			break;
		}
		case lex_factor::_Id: {
			String id = lexFactor.Id().data();
			factor = lexRlFactorName( id, lexFactor.Id().loc() );
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
			ReOrBlock *block = walkRegOrData( lexFactor.PosData() );
			factor = LexFactor::cons( ReItem::cons( block, ReItem::OrBlock ) );
			break;
		}
		case lex_factor::_NegOrBlock: {
			ReOrBlock *block = walkRegOrData( lexFactor.NegData() );
			factor = LexFactor::cons( ReItem::cons( block, ReItem::NegOrBlock ) );
			break;
		}
		case lex_factor::_Number: {
			String number = lexFactor.Number().text().c_str();
			factor = LexFactor::cons( Literal::cons( lexFactor.Number().loc(), 
					number, Literal::Number ) );
			break;
		}
		case lex_factor::_Hex: {
			String number = lexFactor.Hex().text().c_str();
			factor = LexFactor::cons( Literal::cons( lexFactor.Hex().loc(), 
					number, Literal::Number ) );
			break;
		}
		case lex_factor::_Paren: {
			lex_expr LexExpr = lexFactor.Expr();
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
		LangExpr *relational = walkCodeRelational( codeExpr.Relational() );

		switch ( codeExpr.prodName() ) {
		case code_expr::_AmpAmp: {
			LangExpr *left = walkCodeExpr( codeExpr.Expr() );

			InputLoc loc = codeExpr.AmpAmp().loc();
			expr = LangExpr::cons( loc, left, OP_LogicalAnd, relational );
			break;
		}
		case code_expr::_BarBar: {
			LangExpr *left = walkCodeExpr( codeExpr.Expr() );

			InputLoc loc = codeExpr.BarBar().loc();
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
			print_stmt printStmt = Statement.Print();
			stmt = walkPrintStmt( printStmt );
			break;
		}
		case statement::_Expr: {
			expr_stmt exprStmt = Statement.Expr();
			stmt = walkExprStmt( exprStmt );
			break;
		}
		case statement::_VarDef: {
			ObjectField *objField = walkVarDef( Statement.VarDef() );
			LangExpr *expr = walkOptDefInit( Statement.OptDefInit() );
			stmt = varDef( objField, expr, LangStmt::AssignType );
			break;
		}
		case statement::_For: {
			pushScope();

			String forDecl = Statement.ForDecl().text().c_str();
			TypeRef *typeRef = walkTypeRef( Statement.TypeRef() );
			StmtList *stmtList = walkBlockOrSingle( Statement.BlockOrSingle() );

			LangIterCall *iterCall = walkIterCall( Statement.IterCall() );

			stmt = forScope( Statement.ForDecl().loc(), forDecl, typeRef, iterCall, stmtList );

			popScope();
			break;
		}
		case statement::_If: {
			pushScope();

			LangExpr *expr = walkCodeExpr( Statement.IfExpr() );
			StmtList *stmtList = walkBlockOrSingle( Statement.BlockOrSingle() );

			popScope();

			LangStmt *elsifList = walkElsifList( Statement.ElsifList() );
			stmt = LangStmt::cons( LangStmt::IfType, expr, stmtList, elsifList );
			break;
		}
		case statement::_While: {
			pushScope();
			LangExpr *expr = walkCodeExpr( Statement.WhileExpr() );
			StmtList *stmtList = walkBlockOrSingle( Statement.BlockOrSingle() );
			stmt = LangStmt::cons( LangStmt::WhileType, expr, stmtList );
			popScope();
			break;
		}
		case statement::_LhsVarRef: {
			LangVarRef *varRef = walkVarRef( Statement.LhsVarRef() );
			LangExpr *expr = walkCodeExpr( Statement.CodeExpr() );
			stmt = LangStmt::cons( varRef->loc, LangStmt::AssignType, varRef, expr );
			break;
		}
		case statement::_Yield: {
			LangVarRef *varRef = walkVarRef( Statement.YieldVarRef() );
			stmt = LangStmt::cons( LangStmt::YieldType, varRef );
			break;
		}
		case statement::_Return: {
			LangExpr *expr = walkCodeExpr( Statement.ReturnExpr() );
			stmt = LangStmt::cons( Statement.loc(), LangStmt::ReturnType, expr );
			break;
		}
		case statement::_Break: {
			stmt = LangStmt::cons( LangStmt::BreakType );
			break;
		}
		case statement::_Reject: {
			stmt = LangStmt::cons( Statement.Reject().loc(), LangStmt::RejectType );
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

		if ( langStmtList.OptRequire().Require() != 0 ) {
			pushScope();
			require_pattern require = langStmtList.OptRequire().Require();

			LangVarRef *varRef = walkVarRef( require.VarRef() );
			PatternItemList *list = walkPattern( require.Pattern() );
			LangExpr *expr = match( langStmtList.OptRequire().Require().R().loc(), varRef, list );

 			StmtList *reqList = walkLangStmtList( langStmtList.OptRequire().LangStmtList() );

			LangStmt *stmt = LangStmt::cons( LangStmt::IfType, expr, reqList, 0 );

			popScope();

			retList->append( stmt );
		}

		return retList;
	}

	void walkTokenDef( token_def TokenDef )
	{
		String name = TokenDef.Id().data();

		bool niLeft = walkOptNoIgnore( TokenDef.NiLeft() );
		bool niRight = walkOptNoIgnore( TokenDef.NiRight() );

		ObjectDef *objectDef = walkVarDefList( TokenDef.VarDefList() );
		objectDef->name = name;

		LexJoin *join = 0;
		if ( TokenDef.OptExpr().Expr() != 0 ) {
			LexExpression *expr = walkLexExpr( TokenDef.OptExpr().Expr() );
			join = LexJoin::cons( expr );
		}

		CodeBlock *translate = walkOptTranslate( TokenDef.OptTranslate() );

		defineToken( TokenDef.Id().loc(), name, join, objectDef, translate, false, niLeft, niRight );
	}

	String walkOptId( opt_id optId )
	{
		String name;
		if ( optId.prodName() == opt_id::_Id )
			name = optId.Id().data();
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
		StmtList *stmtList = walkLangStmtList( PreEofDef.LangStmtList() );
		preEof( PreEofDef.PreEof().loc(), stmtList, localFrame );
		blockClose();
	}

	void walkIgnoreDef( ignore_def IgnoreDef )
	{
		String name = walkOptId( IgnoreDef.OptId() );
		ObjectDef *objectDef = ObjectDef::cons( ObjectDef::UserType, name, pd->nextObjectId++ ); 

		LexJoin *join = 0;
		if ( IgnoreDef.OptExpr().Expr() != 0 ) {
			LexExpression *expr = walkLexExpr( IgnoreDef.OptExpr().Expr() );
			join = LexJoin::cons( expr );
		}

		defineToken( IgnoreDef.I().loc(), name, join, objectDef, 0, true, false, false );
	}

	LangExpr *walkCodeMultiplicitive( code_multiplicitive mult )
	{
		LangExpr *expr = 0;
		LangExpr *right = walkCodeUnary( mult.Unary() );
		switch ( mult.prodName() ) {
		case code_multiplicitive::_Star: {
			LangExpr *left = walkCodeMultiplicitive( mult.Multiplicitive() );
			expr = LangExpr::cons( mult.Star().loc(), left, '*', right );
			break;
		}
		case code_multiplicitive::_Fslash: {
			LangExpr *left = walkCodeMultiplicitive( mult.Multiplicitive() );
			expr = LangExpr::cons( mult.Fslash().loc(), left, '/', right );
			break;
		}
		case code_multiplicitive::_Base: {
			expr = right;
			break;
		}}
		return expr;
	}

	PatternItemList *walkPatternElTypeOrLit( pattern_el_lel typeOrLit )
	{
		NamespaceQual *nspaceQual = walkRegionQual( typeOrLit.RegionQual() );
		RepeatType repeatType = walkOptRepeat( typeOrLit.OptRepeat() );

		PatternItemList *list = 0;
		switch ( typeOrLit.prodName() ) {
		case pattern_el_lel::_Id: {
			String id = typeOrLit.Id().data();
			list = patternElNamed( typeOrLit.Id().loc(), nspaceQual, id, repeatType );
			break;
		}
		case pattern_el_lel::_Lit: {
			String lit = typeOrLit.Lit().data();
			list = patternElType( typeOrLit.Lit().loc(), nspaceQual, lit, repeatType );
			break;
		}}
		return list;
	}

	LangVarRef *walkOptLabel( opt_label optLabel )
	{
		LangVarRef *varRef = 0;
		if ( optLabel.prodName() == opt_label::_Id ) {
			String id = optLabel.Id().data();
			varRef = LangVarRef::cons( optLabel.Id().loc(), id );
		}
		return varRef;
	}

	PatternItemList *walkPatternEl( pattern_el patternEl )
	{
		PatternItemList *list = 0;
		switch ( patternEl.prodName() ) {
		case pattern_el::_Dq: {
			list = walkLitpatElList( patternEl.LitpatElList(), patternEl.Term().Nl() );
			break;
		}
		case pattern_el::_Tilde: {
			String patternData = patternEl.TildeData().text().c_str();
			patternData += '\n';
			PatternItem *patternItem = PatternItem::cons( patternEl.TildeData().loc(),
					patternData, PatternItem::InputText );
			list = PatternItemList::cons( patternItem );
			break;
		}
		case pattern_el::_PatternEl: {
			LangVarRef *varRef = walkOptLabel( patternEl.OptLabel() );
			PatternItemList *typeOrLitList = walkPatternElTypeOrLit( patternEl.TypeOrLit() );
			list = consPatternEl( varRef, typeOrLitList );
			break;
		}}
		return list;
	}

	PatternItemList *walkLitpatEl( litpat_el litpatEl )
	{
		PatternItemList *list = 0;
		switch ( litpatEl.prodName() ) {
		case litpat_el::_ConsData: {
			String consData = unescape( litpatEl.ConsData().text().c_str() );
			PatternItem *patternItem = PatternItem::cons( litpatEl.ConsData().loc(),
					consData, PatternItem::InputText );
			list = PatternItemList::cons( patternItem );
			break;
		}
		case litpat_el::_SubList: {
			list = walkPatternElList( litpatEl.PatternElList() );
			break;
		}}
		return list;
	}

	PatternItemList *walkLitpatElList( _repeat_litpat_el litpatElList, CONS_NL Nl )
	{
		PatternItemList *list = new PatternItemList;
		while ( !litpatElList.end() ) {
			PatternItemList *tail = walkLitpatEl( litpatElList.value() );
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

	PatternItemList *walkPatternElList( _repeat_pattern_el patternElList )
	{
		PatternItemList *list = new PatternItemList;
		while ( !patternElList.end() ) {
			PatternItemList *tail = walkPatternEl( patternElList.value() );
			list = patListConcat( list, tail );
			patternElList = patternElList.next();
		}
		return list;
	}

	PatternItemList *walkPattternTopEl( pattern_top_el patternTopEl )
	{
		PatternItemList *list = 0;
		switch ( patternTopEl.prodName() ) {
		case pattern_top_el::_Dq: {
			list = walkLitpatElList( patternTopEl.LitpatElList(), patternTopEl.Term().Nl() );
			break;
		}
		case pattern_top_el::_Tilde: {
			String patternData = patternTopEl.TildeData().text().c_str();
			patternData += '\n';
			PatternItem *patternItem = PatternItem::cons( patternTopEl.TildeData().loc(),
					patternData, PatternItem::InputText );
			list = PatternItemList::cons( patternItem );
			break;
		}
		case pattern_top_el::_SubList: {
			list = walkPatternElList( patternTopEl.PatternElList() );
			break;
		}}
		return list;
	}

	PatternItemList *walkPatternList( pattern_list patternList )
	{
		PatternItemList *list = 0;
		switch ( patternList.prodName() ) {
		case pattern_list::_List: {
			PatternItemList *left = walkPatternList( patternList.PatternList() );
			PatternItemList *right = walkPattternTopEl( patternList.PatternTopEl() );
			list = patListConcat( left, right );
			break;
		}
		case pattern_list::_Base: {
			list = walkPattternTopEl( patternList.PatternTopEl() );
			break;
		}}
		return list;
	}

	PatternItemList *walkPattern( pattern Pattern )
	{
		return walkPatternList( Pattern.PatternList() );
	}

	LangExpr *walkOptDefInit( opt_def_init optDefInit )
	{
		LangExpr *expr = 0;
		if ( optDefInit.prodName() == opt_def_init::_Init )
			expr = walkCodeExpr( optDefInit.CodeExpr() );
		return expr;
	}

	LangStmt *walkExportDef( export_def exportDef )
	{
		ObjectField *objField = walkVarDef( exportDef.VarDef() );
		LangExpr *expr = walkOptDefInit( exportDef.OptDefInit() );

		return exportStmt( objField, LangStmt::AssignType, expr );
	}

	LangStmt *walkGlobalDef( global_def GlobalDef )
	{
		ObjectField *objField = walkVarDef( GlobalDef.VarDef() );
		LangExpr *expr = walkOptDefInit( GlobalDef.OptDefInit() );

		return globalDef( objField, expr, LangStmt::AssignType );
	}

	void walkAliasDef( alias_def aliasDef )
	{
		String id = aliasDef.Id().data();
		TypeRef *typeRef = walkTypeRef( aliasDef.TypeRef() );
		alias( aliasDef.Id().loc(), id, typeRef );
	}

	CodeBlock *walkOptTranslate( opt_translate optTranslate )
	{
		CodeBlock *block = 0;
		if ( optTranslate.prodName() == opt_translate::_Translate ) {
			ObjectDef *localFrame = blockOpen();
			StmtList *stmtList = walkLangStmtList( optTranslate.LangStmtList() );
			block = CodeBlock::cons( stmtList, localFrame );
			block->context = contextStack.top();
			blockClose();
		}
		return block;
	}

	PredDecl *walkPredToken( pred_token predToken )
	{
		NamespaceQual *nspaceQual = walkRegionQual( predToken.RegionQual() );
		PredDecl *predDecl = 0;
		switch ( predToken.prodName() ) {
		case pred_token::_Id: {
			String id = predToken.Id().data();
			predDecl = predTokenName( predToken.Id().loc(), nspaceQual, id );
			break;
		}
		case pred_token::_Lit: {
			String lit = predToken.Lit().data();
			predDecl = predTokenLit( predToken.Lit().loc(), lit, nspaceQual );
			break;
		}}
		return predDecl;
	}

	PredDeclList *walkPredTokenList( pred_token_list predTokenList )
	{
		PredDeclList *list = 0;
		switch ( predTokenList.prodName() ) {
		case pred_token_list::_List: {
			list = walkPredTokenList( predTokenList.PredTokenList() );
			PredDecl *predDecl = walkPredToken( predTokenList.PredToken() );
			list->append( predDecl );
			break;
		}
		case pred_token_list::_Base: {
			PredDecl *predDecl = walkPredToken( predTokenList.PredToken() );
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
		PredType predType = walkPredType( precedenceDef.PredType() );
		PredDeclList *predDeclList = walkPredTokenList( precedenceDef.PredTokenList() );
		precedenceStmt( predType, predDeclList );
	}

	StmtList *walkInclude( include Include )
	{
		String lit = Include.File().data();
		String file;
		bool unused;
		prepareLitString( file, unused, lit, Include.File().loc() );

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
			qual = walkRegionQual( regionQual.RegionQual() );
			qual->qualNames.append( String( regionQual.Id().data() ) );
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
			NamespaceQual *nspaceQual = walkRegionQual( typeRef.RegionQual() );
			String id = typeRef.DirectId().data();
			RepeatType repeatType = walkOptRepeat( typeRef.OptRepeat() );
			tr = TypeRef::cons( typeRef.DirectId().loc(), nspaceQual, id, repeatType );
			break;
		}
		case type_ref::_Ptr: {
			NamespaceQual *nspaceQual = walkRegionQual( typeRef.RegionQual() );
			String id = typeRef.PtrId().data();
			RepeatType repeatType = walkOptRepeat( typeRef.OptRepeat() );
			TypeRef *inner = TypeRef::cons( typeRef.PtrId().loc(), nspaceQual, id, repeatType );
			tr = TypeRef::cons( typeRef.PtrId().loc(), TypeRef::Ptr, inner );
			break;
		}
		case type_ref::_Map: {
			TypeRef *key = walkTypeRef( typeRef.MapKeyType() );
			TypeRef *value = walkTypeRef( typeRef.MapValueType() );
			tr = TypeRef::cons( typeRef.loc(), TypeRef::Map, 0, key, value );
			break;
		}
		case type_ref::_List: {
			TypeRef *type = walkTypeRef( typeRef.ListType() );
			tr = TypeRef::cons( typeRef.loc(), TypeRef::List, 0, type, 0 );
			break;
		}
		case type_ref::_Vector: {
			TypeRef *type = walkTypeRef( typeRef.VectorType() );
			tr = TypeRef::cons( typeRef.loc(), TypeRef::Vector, 0, type, 0 );
			break;
		}
		case type_ref::_Parser: {
			TypeRef *type = walkTypeRef( typeRef.ParserType() );
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
			LangStmt *stmt = walkStatement( blockOrSingle.Statement() );
			stmtList->append( stmt );
			break;
		}
		case block_or_single::_Block: {
			stmtList = walkLangStmtList( blockOrSingle.LangStmtList() );
			break;
		}}

		return stmtList;
	}

	void walkProdEl( ProdElList *list, prod_el El )
	{
		ObjectField *captureField = 0;
		if ( El.OptName().prodName() == opt_prod_el_name::_Name ) {
			String fieldName = El.OptName().Name().data();
			captureField = ObjectField::cons( El.OptName().Name().loc(), 0, fieldName );
		}

		RepeatType repeatType = walkOptRepeat( El.OptRepeat() );
		NamespaceQual *nspaceQual = walkRegionQual( El.RegionQual() );

		switch ( El.prodName() ) {
		case prod_el::_Id: {
			String typeName = El.Id().data();
			ProdEl *prodEl = prodElName( El.Id().loc(), typeName,
					nspaceQual,
					captureField, repeatType, false );
			appendProdEl( list, prodEl );
			break;
		}
		case prod_el::_Lit: {
			String lit = El.Lit().data();
			ProdEl *prodEl = prodElLiteral( El.Lit().loc(), lit,
					nspaceQual,
					captureField, repeatType, false );
			appendProdEl( list, prodEl );
			break;
		}}
	}

	void walkProdElList( ProdElList *list, prod_el_list ProdElList )
	{
		if ( ProdElList.prodName() == prod_el_list::_List ) {
			prod_el_list RightProdElList = ProdElList.ProdElList();
			walkProdElList( list, RightProdElList );
			walkProdEl( list, ProdElList.ProdEl() );
		}
	}

	CodeBlock *walkOptReduce( opt_reduce OptReduce )
	{
		CodeBlock *block = 0;
		if ( OptReduce.prodName() == opt_reduce::_Reduce ) {
			ObjectDef *localFrame = blockOpen();
			StmtList *stmtList = walkLangStmtList( OptReduce.LangStmtList() );

			block = CodeBlock::cons( stmtList, localFrame );
			block->context = contextStack.top();

			blockClose();
		}
		return block;
	}

	void walkProdudction( LelDefList *lelDefList, prod Prod )
	{
		ProdElList *list = new ProdElList;

		walkProdElList( list, Prod.ProdElList() );

		String name;
		if ( Prod.OptName().prodName() == opt_prod_name::_Name )
			name = Prod.OptName().Name().data();

		CodeBlock *codeBlock = walkOptReduce( Prod.OptReduce() );
		bool commit = Prod.OptCommit().prodName() == opt_commit::_Commit;

		Production *prod = BaseParser::production( Prod.Open().loc(), list, name, commit, codeBlock, 0 );
		prodAppend( lelDefList, prod );
	}

	void walkProdList( LelDefList *lelDefList, prod_list ProdList )
	{
		if ( ProdList.prodName() == prod_list::_List ) 
			walkProdList( lelDefList, ProdList.ProdList() );

		walkProdudction( lelDefList, ProdList.Prod() );
	}

	ReOrItem *walkRegOrChar( reg_or_char regOrChar )
	{
		ReOrItem *orItem = 0;
		switch ( regOrChar.prodName() ) {
		case reg_or_char::_Char: {
			String c = unescape( regOrChar.Char().data() );
			orItem = ReOrItem::cons( regOrChar.Char().loc(), c );
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
			ReOrBlock *left = walkRegOrData( regOrData.Data() );
			ReOrItem *right = walkRegOrChar( regOrData.Char() );
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
			LexFactorNeg *recNeg = walkLexFactorNeg( lexFactorNeg.FactorNeg() );
			factorNeg = LexFactorNeg::cons( recNeg, LexFactorNeg::CharNegateType );
			break;
		}
		case lex_factor_neg::_Base: {
			LexFactor *factor = walkLexFactor( lexFactorNeg.Factor() );
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
			recRep = walkLexFactorRep( lexFactorRep.FactorRep() );

		switch ( pn ) {
		case lex_factor_rep::_Star: {
			factorRep = LexFactorRep::cons( lexFactorRep.Star().loc(),
					recRep, 0, 0, LexFactorRep::StarType );
			break;
		}
		case lex_factor_rep::_StarStar: {
			factorRep = LexFactorRep::cons( lexFactorRep.StarStar().loc(),
					recRep, 0, 0, LexFactorRep::StarStarType );
			break;
		}
		case lex_factor_rep::_Plus: {
			factorRep = LexFactorRep::cons( lexFactorRep.Plus().loc(),
					recRep, 0, 0, LexFactorRep::PlusType );
			break;
		}
		case lex_factor_rep::_Question: {
			factorRep = LexFactorRep::cons( lexFactorRep.Question().loc(),
					recRep, 0, 0, LexFactorRep::OptionalType );
			break;
		}
		case lex_factor_rep::_Exact: {
			int low = atoi( lexFactorRep.Exact().data()->data );
			factorRep = LexFactorRep::cons( lexFactorRep.Exact().loc(),
					recRep, low, 0, LexFactorRep::ExactType );
			break;
		}
		case lex_factor_rep::_Max: {
			int high = atoi( lexFactorRep.Max().data()->data );
			factorRep = LexFactorRep::cons( lexFactorRep.Max().loc(),
					recRep, 0, high, LexFactorRep::MaxType );
			break;
		}
		case lex_factor_rep::_Min: {
			int low = atoi( lexFactorRep.Min().data()->data );
			factorRep = LexFactorRep::cons( lexFactorRep.Min().loc(),
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
			LexFactorNeg *factorNeg = walkLexFactorNeg( lexFactorRep.FactorNeg() );
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
			leftTerm = walkLexTerm( lexTerm.Term() );

		LexFactorAug *factorAug = walkLexFactorAug( lexTerm.FactorRep() );

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
			leftExpr = walkLexExpr( lexExpr.Expr() );

		LexTerm *term = walkLexTerm( lexExpr.Term() );

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
		String id = rlDef.Id().data();

		lex_expr LexExpr = rlDef.Expr();
		LexExpression *expr = walkLexExpr( LexExpr );
		LexJoin *join = LexJoin::cons( expr );

		addRegularDef( rlDef.Id().loc(), namespaceStack.top(), id, join );
	}

	void walkLexRegion( region_def regionDef )
	{
		pushRegionSet( regionDef.loc() );
		walkRootItemList( regionDef.RootItemList() );
		popRegionSet();
	}

	void walkCflDef( cfl_def cflDef )
	{
		String name = cflDef.DefId().data();
		ObjectDef *objectDef = walkVarDefList( cflDef.VarDefList() );
		objectDef->name = name;

		LelDefList *defList = new LelDefList;
		walkProdList( defList, cflDef.ProdList() );

		bool reduceFirst = cflDef.OptReduceFirst().ReduceFirst() != 0;

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
			
		return LangStmt::cons( printStmt.O().loc(), type, exprVect );
	}

	QualItemVect *walkQual( qual &Qual )
	{
		QualItemVect *qualItemVect = 0;
		qual RecQual = Qual.Qual();
		switch ( Qual.prodName() ) {
		case qual::_Dot:
		case qual::_Arrow: {
			qualItemVect = walkQual( RecQual );
			String id = Qual.Id().data();
			QualItem::Type type = Qual.Dot() != 0 ? QualItem::Dot : QualItem::Arrow;
			qualItemVect->append( QualItem( Qual.Id().loc(), id, type ) );
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
		qual Qual = varRef.Qual();
		QualItemVect *qualItemVect = walkQual( Qual );
		String id = varRef.Id().data();
		LangVarRef *langVarRef = LangVarRef::cons( varRef.Id().loc(), qualItemVect, id );
		return langVarRef;
	}

	ObjectField *walkOptCapture( opt_capture optCapture )
	{
		ObjectField *objField = 0;
		if ( optCapture.prodName() == opt_capture::_Id ) {
			String id = optCapture.Id().data();
			objField = ObjectField::cons( optCapture.Id().loc(), 0, id );
		}
		return objField;
	}

	/*
	 * Constructor
	 */

	ConsItemList *walkLitConsEl( lit_cons_el litConsEl )
	{
		ConsItemList *list = 0;
		switch ( litConsEl.prodName() ) {
		case lit_cons_el::_ConsData: {
			String consData = unescape( litConsEl.ConsData().text().c_str() );
			ConsItem *consItem = ConsItem::cons( litConsEl.ConsData().loc(),
					ConsItem::InputText, consData );
			list = ConsItemList::cons( consItem );
			break;
		}
		case lit_cons_el::_SubList: {
			list = walkConsElList( litConsEl.ConsElList() );
			break;
		}}
		return list;
	}

	ConsItemList *walkLitConsElList( _repeat_lit_cons_el litConsElList, CONS_NL Nl )
	{
		ConsItemList *list = new ConsItemList;
		while ( !litConsElList.end() ) {
			ConsItemList *extension = walkLitConsEl( litConsElList.value() );
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

	ConsItemList *walkConsEl( cons_el consEl )
	{
		ConsItemList *list = 0;
		switch ( consEl.prodName() ) {
		case cons_el::_Lit: {
			NamespaceQual *nspaceQual = walkRegionQual( consEl.RegionQual() );
			String lit = consEl.Lit().data();
			list = consElLiteral( consEl.Lit().loc(), lit, nspaceQual );
			break;
		}
		case cons_el::_Tilde: {
			String consData = consEl.TildeData().text().c_str();
			consData += '\n';
			ConsItem *consItem = ConsItem::cons( consEl.TildeData().loc(),
					ConsItem::InputText, consData );
			list = ConsItemList::cons( consItem );
			break;
		}
		case cons_el::_CodeExpr: {
			LangExpr *consExpr = walkCodeExpr( consEl.CodeExpr() );
			ConsItem *consItem = ConsItem::cons( consExpr->loc, ConsItem::ExprType, consExpr );
			list = ConsItemList::cons( consItem );
			break;
		}
		case cons_el::_Dq: {
			list = walkLitConsElList( consEl.LitConsElList(), consEl.Term().Nl() );
			break;
		}}
		return list;
	}

	ConsItemList *walkConsElList( _repeat_cons_el consElList )
	{
		ConsItemList *list = new ConsItemList;
		while ( !consElList.end() ) {
			ConsItemList *extension = walkConsEl( consElList.value() );
			list = consListConcat( list, extension );
			consElList = consElList.next();
		}
		return list;
	}

	ConsItemList *walkConsTopEl( cons_top_el consTopEl )
	{
		ConsItemList *list = 0;
		switch ( consTopEl.prodName() ) {
		case cons_top_el::_Dq: {
			list = walkLitConsElList( consTopEl.LitConsElList(), consTopEl.Term().Nl() );
			break;
		}
		case cons_top_el::_Tilde: {
			String consData = consTopEl.TildeData().text().c_str();
			consData += '\n';
			ConsItem *consItem = ConsItem::cons( consTopEl.TildeData().loc(),
					ConsItem::InputText, consData );
			list = ConsItemList::cons( consItem );
			break;
		}
		case cons_top_el::_SubList: {
			list = walkConsElList( consTopEl.ConsElList() );
			break;
		}}
		return list;
	}

	ConsItemList *walkConsList( cons_list consList )
	{
		ConsItemList *list = walkConsTopEl( consList.ConsTopEl() );

		if ( consList.ConsList() != 0 ) {
			ConsItemList *extension = walkConsList( consList.ConsList() );
			consListConcat( list, extension );
		}

		return list;
	}

	ConsItemList *walkConstructor( constructor Constructor )
	{
		ConsItemList *list = walkConsList( Constructor.ConsList() );
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
			String consData = unescape( litStringEl.ConsData().text().c_str() );
			ConsItem *stringItem = ConsItem::cons( litStringEl.ConsData().loc(),
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
			list = walkLitStringElList( stringEl.LitStringElList(), stringEl.Term().Nl() );
			break;
		}
		case string_el::_Tilde: {
			String consData = stringEl.TildeData().text().c_str();
			consData += '\n';
			ConsItem *consItem = ConsItem::cons( stringEl.TildeData().loc(),
					ConsItem::InputText, consData );
			list = ConsItemList::cons( consItem );
			break;
		}
		case string_el::_CodeExpr: {
			LangExpr *consExpr = walkCodeExpr( stringEl.CodeExpr() );
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
			list = walkLitStringElList( stringTopEl.LitStringElList(), stringTopEl.Term().Nl() );
			break;
		}
		case string_top_el::_Tilde: {
			String consData = stringTopEl.TildeData().text().c_str();
			consData += '\n';
			ConsItem *consItem = ConsItem::cons( stringTopEl.TildeData().loc(),
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
		ConsItemList *list = walkStringTopEl( stringList.StringTopEl() );

		if ( stringList.StringList() != 0 ) {
			ConsItemList *extension = walkStringList( stringList.StringList() );
			consListConcat( list, extension );
		}

		return list;
	}

	ConsItemList *walkString( cstring String )
	{
		ConsItemList *list = walkStringList( String.StringList() );
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
			String consData = unescape( litAccumEl.ConsData().text().c_str() );
			ConsItem *consItem = ConsItem::cons( litAccumEl.ConsData().loc(),
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
			list = walkLitAccumElList( accumEl.LitAccumElList(), accumEl.Term().Nl() );
			break;
		}
		case accum_el::_Tilde: {
			String consData = accumEl.TildeData().text().c_str();
			consData += '\n';
			ConsItem *consItem = ConsItem::cons( accumEl.TildeData().loc(),
					ConsItem::InputText, consData );
			list = ConsItemList::cons( consItem );
			break;
		}
		case accum_el::_CodeExpr: {
			LangExpr *accumExpr = walkCodeExpr( accumEl.CodeExpr() );
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
			list = walkLitAccumElList( accumTopEl.LitAccumElList(), accumTopEl.Term().Nl() );
			break;
		}
		case accum_top_el::_Tilde: {
			String consData = accumTopEl.TildeData().text().c_str();
			consData += '\n';
			ConsItem *consItem = ConsItem::cons( accumTopEl.TildeData().loc(),
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
		ConsItemList *list = walkAccumTopEl( accumList.AccumTopEl() );

		if ( accumList.prodName() == accum_list::_List ) {
			ConsItemList *extension = walkAccumList( accumList.AccumList() );
			consListConcat( list, extension );
		}

		return list;
	}

	ConsItemList *walkAccumulate( accumulate Accumulate )
	{
		ConsItemList *list = walkAccumList( Accumulate.AccumList() );
		return list;
	}

	void walkFieldInit( FieldInitVect *list, field_init fieldInit )
	{
		LangExpr *expr = walkCodeExpr( fieldInit.CodeExpr() );
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
			LangVarRef *langVarRef = walkVarRef( codeFactor.VarRef() );
			LangTerm *term = LangTerm::cons( langVarRef->loc, LangTerm::VarRefType, langVarRef );
			expr = LangExpr::cons( term );
			break;
		}
		case code_factor::_Call: {
			LangVarRef *langVarRef = walkVarRef( codeFactor.VarRef() );
			CallArgVect *exprVect = walkCodeExprList( codeFactor.CodeExprList() );
			LangTerm *term = LangTerm::cons( langVarRef->loc, langVarRef, exprVect );
			expr = LangExpr::cons( term );
			break;
		}
		case code_factor::_Number: {
			String number = codeFactor.Number().text().c_str();
			LangTerm *term = LangTerm::cons( codeFactor.Number().loc(),
					LangTerm::NumberType, number );
			expr = LangExpr::cons( term );
			break;
		}
		case code_factor::_Lit: {
			String lit = codeFactor.Lit().data();
			LangTerm *term = LangTerm::cons( codeFactor.Lit().loc(), LangTerm::StringType, lit );
			expr = LangExpr::cons( term );
			break;
		}
		case code_factor::_Parse: {
			/* The type we are parsing. */
			type_ref typeRefTree = codeFactor.TypeRef();
			TypeRef *typeRef = walkTypeRef( typeRefTree );
			ObjectField *objField = walkOptCapture( codeFactor.OptCapture() );
			FieldInitVect *init = walkOptFieldInit( codeFactor.OptFieldInit() );
			ConsItemList *list = walkAccumulate( codeFactor.Accumulate() );

			expr = parseCmd( codeFactor.Parse().loc(), false, objField, typeRef, init, list );
			break;
		}
		case code_factor::_ParseStop: {
			/* The type we are parsing. */
			type_ref typeRefTree = codeFactor.TypeRef();
			TypeRef *typeRef = walkTypeRef( typeRefTree );
			ObjectField *objField = walkOptCapture( codeFactor.OptCapture() );
			FieldInitVect *init = walkOptFieldInit( codeFactor.OptFieldInit() );
			ConsItemList *list = walkAccumulate( codeFactor.Accumulate() );

			expr = parseCmd( codeFactor.ParseStop().loc(), true, objField, typeRef, init, list );
			break;
		}
		case code_factor::_Cons: {
			/* The type we are parsing. */
			type_ref typeRefTree = codeFactor.TypeRef();
			TypeRef *typeRef = walkTypeRef( typeRefTree );
			ObjectField *objField = walkOptCapture( codeFactor.OptCapture() );
			ConsItemList *list = walkConstructor( codeFactor.Constructor() );
			FieldInitVect *init = walkOptFieldInit( codeFactor.OptFieldInit() );

			expr = construct( codeFactor.Cons().loc(), objField, list, typeRef, init );
			break;
		}
		case code_factor::_Send: {
			LangVarRef *varRef = walkVarRef( codeFactor.ToVarRef() );
			ConsItemList *list = walkAccumulate( codeFactor.Accumulate() );
			bool eof = walkOptEos( codeFactor.OptEos() );
			expr = send( codeFactor.Send().loc(), varRef, list, eof );
			break;
		}
		case code_factor::_Nil: {
			expr = LangExpr::cons( LangTerm::cons( codeFactor.Nil().loc(),
					LangTerm::NilType ) );
			break;
		}
		case code_factor::_True: {
			expr = LangExpr::cons( LangTerm::cons( codeFactor.True().loc(),
					LangTerm::TrueType ) );
			break;
		}
		case code_factor::_False: {
			expr = LangExpr::cons( LangTerm::cons( codeFactor.False().loc(),
					LangTerm::FalseType ) );
			break;
		}
		case code_factor::_Paren: {
			expr = walkCodeExpr( codeFactor.ParenCodeExpr() );
			break;
		}
		case code_factor::_String: {
			ConsItemList *list = walkString( codeFactor.String() );
			expr = LangExpr::cons( LangTerm::cons( codeFactor.String().loc(), list ) );
			break;
		}
		case code_factor::_Match: {
			LangVarRef *varRef = walkVarRef( codeFactor.MatchVarRef() );
			PatternItemList *list = walkPattern( codeFactor.Pattern() );
			expr = match( codeFactor.loc(), varRef, list );
			break;
		}
		case code_factor::_In: {
			TypeRef *typeRef = walkTypeRef( codeFactor.TypeRef() );
			LangVarRef *varRef = walkVarRef( codeFactor.InVarRef() );
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
			TypeRef *typeRef = walkTypeRef( codeFactor.TypeIdTypeRef() );
			expr = LangExpr::cons( LangTerm::cons( codeFactor.loc(),
					LangTerm::TypeIdType, typeRef ) );
			break;
		}
		case code_factor::_New: {
			LangExpr *newExpr = walkCodeFactor( codeFactor.NewCodeFactor() );
			expr = LangExpr::cons( LangTerm::cons( codeFactor.loc(),
					LangTerm::NewType, newExpr ) );
			break;
		}}
		return expr;
	}

	LangExpr *walkCodeAdditive( code_additive additive )
	{
		LangExpr *expr = 0;
		switch ( additive.prodName() ) {
		case code_additive::_Plus: {
			LangExpr *left = walkCodeAdditive( additive.Additive() );
			LangExpr *right = walkCodeMultiplicitive( additive.Multiplicitive() );
			expr = LangExpr::cons( additive.Plus().loc(), left, '+', right );
			break;
		}
		case code_additive::_Minus: {
			LangExpr *left = walkCodeAdditive( additive.Additive() );
			LangExpr *right = walkCodeMultiplicitive( additive.Multiplicitive() );
			expr = LangExpr::cons( additive.Minus().loc(), left, '-', right );
			break;
		}
		case code_additive::_Base: {
			expr = walkCodeMultiplicitive( additive.Multiplicitive() );
			break;
		}}
		return expr;
	}

	LangExpr *walkCodeUnary( code_unary unary )
	{
		LangExpr *expr = 0, *factor = walkCodeFactor( unary.Factor() );

		switch ( unary.prodName() ) {
		case code_unary::_Bang: {
			expr = LangExpr::cons( unary.Bang().loc(), '!', factor );
			break;
		}
		case code_unary::_Dollar: {
			expr = LangExpr::cons( unary.Dollar().loc(), '$', factor );
			break;
		}
		case code_unary::_Caret: {
			expr = LangExpr::cons( unary.Caret().loc(), '^', factor );
			break;
		}
		case code_unary::_Percent: {
			expr = LangExpr::cons( unary.Percent().loc(), '%', factor );
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
			left = walkCodeRelational( codeRelational.Relational() );

		LangExpr *additive = walkCodeAdditive( codeRelational.Additive() );

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
		LangExpr *expr = walkCodeExpr( exprStmt.CodeExpr() );
		LangStmt *stmt = LangStmt::cons( expr->loc, LangStmt::ExprType, expr );
		return stmt;
	}

	ObjectField *walkVarDef( var_def varDef )
	{
		String id = varDef.Id().data();
		TypeRef *typeRef = walkTypeRef( varDef.TypeRef() );
		return ObjectField::cons( varDef.Id().loc(), typeRef, id );
	}

	LangIterCall *walkIterCall( iter_call IterCall )
	{
		LangIterCall *iterCall = 0;
		switch ( IterCall.prodName() ) {
		case iter_call::_Call: {
			LangVarRef *varRef = walkVarRef( IterCall.VarRef() );
			CallArgVect *exprVect = walkCodeExprList( IterCall.CodeExprList() );
			LangTerm *langTerm = LangTerm::cons( varRef->loc, varRef, exprVect );
			iterCall = LangIterCall::cons( LangIterCall::IterCall, langTerm );
			break;
		}
		case iter_call::_Id: {
			String tree = IterCall.Id().data();
			LangTerm *langTerm = LangTerm::cons( IterCall.Id().loc(),
					LangTerm::VarRefType, LangVarRef::cons( IterCall.Id().loc(), tree ) );
			LangExpr *langExpr = LangExpr::cons( langTerm );
			iterCall = LangIterCall::cons( LangIterCall::VarRef, langExpr );
			break;
		}
		case iter_call::_Expr: {
			LangExpr *langExpr = walkCodeExpr( IterCall.Expr() );
			iterCall = LangIterCall::cons( LangIterCall::Expr, langExpr );
			break;
		}}
		
		return iterCall;
	}

	LangStmt *walkElsifClause( elsif_clause elsifClause )
	{
		pushScope();
		LangExpr *expr = walkCodeExpr( elsifClause.ElsifExpr() );
		StmtList *stmtList = walkBlockOrSingle( elsifClause.BlockOrSingle() );
		LangStmt *stmt = LangStmt::cons( LangStmt::IfType, expr, stmtList, 0 );
		popScope();
		return stmt;
	}

	LangStmt *walkOptionalElse( optional_else optionalElse )
	{
		LangStmt *stmt = 0;
		if ( optionalElse.prodName() == optional_else::_Else ) {
			pushScope();
			StmtList *stmtList = walkBlockOrSingle( optionalElse.BlockOrSingle() );
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
				stmt = walkElsifClause( elsifList.ElsifClause() );
				stmt->elsePart = walkElsifList( elsifList.ElsifList() );
				break;
			case elsif_list::_OptElse:
				stmt = walkOptionalElse( elsifList.OptionalElse() );
				break;
		}
		return stmt;
	}

	void walkContextVarDef( context_var_def ContextVarDef )
	{
		ObjectField *objField = walkVarDef( ContextVarDef.VarDef() );
		contextVarDef( objField->loc, objField );
	}

	TypeRef *walkReferenceTypeRef( reference_type_ref ReferenceTypeRef )
	{
		TypeRef *typeRef = walkTypeRef( ReferenceTypeRef.TypeRef() );
		return TypeRef::cons( ReferenceTypeRef.R().loc(), TypeRef::Ref, typeRef );
	}

	ObjectField *walkParamVarDef( param_var_def paramVarDef )
	{
		String id = paramVarDef.Id().data();
		TypeRef *typeRef = 0;

		switch ( paramVarDef.prodName() ) {
		case param_var_def::_Type: 
			typeRef = walkTypeRef( paramVarDef.TypeRef() );
			break;
		case param_var_def::_Ref:
			typeRef = walkReferenceTypeRef( paramVarDef.RefTypeRef() );
			break;
		}
		
		return addParam( paramVarDef.Id().loc(), typeRef, id );
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

		bool exprt = walkOptExport( FunctionDef.OptExport() );
		TypeRef *typeRef = walkTypeRef( FunctionDef.TypeRef() );
		String id = FunctionDef.Id().data();
		ParameterList *paramList = walkParamVarDefList( FunctionDef.ParamVarDefList() );
		StmtList *stmtList = walkLangStmtList( FunctionDef.LangStmtList() );
		functionDef( stmtList, localFrame, paramList, typeRef, id, exprt );

		blockClose();
	}

	void walkIterDef( iter_def IterDef )
	{
		ObjectDef *localFrame = blockOpen();

		String id = IterDef.Id().data();
		ParameterList *paramList = walkParamVarDefList( IterDef.ParamVarDefList() );
		StmtList *stmtList = walkLangStmtList( IterDef.LangStmtList() );
		iterDef( stmtList, localFrame, paramList, id );

		blockClose();
	}

	void walkContextItem( context_item contextItem )
	{
		switch ( contextItem.prodName() ) {
		case context_item::_Rl:
			walkRlDef( contextItem.RlDef() );
			break;
		case context_item::_ContextVar:
			walkContextVarDef( contextItem.ContextVarDef() );
			break;
		case context_item::_Token:
			walkTokenDef( contextItem.TokenDef() );
			break;
		case context_item::_Ignore:
			walkIgnoreDef( contextItem.IgnoreDef() );
			break;
		case context_item::_Literal:
			walkLiteralDef( contextItem.LiteralDef() );
			break;
		case context_item::_Cfl:
			walkCflDef( contextItem.CflDef() );
			break;
		case context_item::_Region:
			walkLexRegion( contextItem.RegionDef() );
			break;
		case context_item::_Context:
			walkContextDef( contextItem.ContextDef() );
			break;
		case context_item::_Function:
			walkFunctionDef( contextItem.FunctionDef() );
			break;
		case context_item::_Iter:
			walkIterDef( contextItem.IterDef() );
			break;
		case context_item::_PreEof:
			walkPreEof( contextItem.PreEofDef() );
			break;
		case context_item::_Export:
			walkExportDef( contextItem.ExportDef() );
			break;
		case context_item::_Precedence:
			walkPrecedenceDef( contextItem.PrecedenceDef() );
			break;
		}
	}

	void walkContextDef( context_def contextDef )
	{
		String name = contextDef.Name().data();
		contextHead( contextDef.Name().loc(), name );

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
		String name = NamespaceDef.Name().data();
		createNamespace( NamespaceDef.Name().loc(), name );
		walkRootItemList( NamespaceDef.RootItemList() );
		namespaceStack.pop();
	}

	void walkRootItem( root_item rootItem, StmtList *stmtList )
	{
		switch ( rootItem.prodName() ) {
		case root_item::_Rl:
			walkRlDef( rootItem.RlDef() );
			break;
		case root_item::_Token:
			walkTokenDef( rootItem.TokenDef() );
			break;
		case root_item::_Ignore:
			walkIgnoreDef( rootItem.IgnoreDef() );
			break;
		case root_item::_Literal:
			walkLiteralDef( rootItem.LiteralDef() );
			break;
		case root_item::_Cfl:
			walkCflDef( rootItem.CflDef() );
			break;
		case root_item::_Region:
			walkLexRegion( rootItem.RegionDef() );
			break;
		case root_item::_Statement: {
			LangStmt *stmt = walkStatement( rootItem.Statement() );
			if ( stmt != 0 )
				stmtList->append( stmt );
			break;
		}
		case root_item::_Context:
			walkContextDef( rootItem.ContextDef() );
			break;
		case root_item::_Namespace:
			walkNamespaceDef( rootItem.NamespaceDef() );
			break;
		case root_item::_Function:
			walkFunctionDef( rootItem.FunctionDef() );
			break;
		case root_item::_Iter:
			walkIterDef( rootItem.IterDef() );
			break;
		case root_item::_PreEof:
			walkPreEof( rootItem.PreEofDef() );
			break;
		case root_item::_Export: {
			LangStmt *stmt = walkExportDef( rootItem.ExportDef() );
			if ( stmt != 0 )
				stmtList->append( stmt );
			break;
		}
		case root_item::_Alias:
			walkAliasDef( rootItem.AliasDef() );
			break;
		case root_item::_Precedence:
			walkPrecedenceDef( rootItem.PrecedenceDef() );
			break;
		case root_item::_Include: {
			StmtList *includeList = walkInclude( rootItem.Include() );
			stmtList->append( *includeList );
			break;
		}
		case root_item::_Global: {
			LangStmt *stmt = walkGlobalDef( rootItem.GlobalDef() );
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

		String lit = literalItem.Lit().data();
		if ( strcmp( lit, "''" ) == 0 )
			zeroDef( literalItem.Lit().loc(), lit, niLeft, niRight );
		else
			literalDef( literalItem.Lit().loc(), lit, niLeft, niRight );
	}

	void walkLiteralList( literal_list literalList )
	{
		if ( literalList.prodName() == literal_list::_Item )
			walkLiteralList( literalList.LiteralList() );
		walkLiteralItem( literalList.LiteralItem() );
	}

	void walkLiteralDef( literal_def literalDef )
	{
		walkLiteralList( literalDef.LiteralList() );
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
