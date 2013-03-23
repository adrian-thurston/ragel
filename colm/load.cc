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
#include "load.h"
#include "exports2.h"
#include "colm/colm.h"

extern RuntimeData main_runtimeData;

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


struct LoadSource
:
	public BaseParser
{
	LoadSource( Compiler *pd, const char *inputFileName )
	:
		BaseParser( pd ),
		inputFileName( inputFileName )
	{}

	const char *inputFileName;

	void go();

	ObjectField *walkVarDef( var_def varDef );
	NamespaceQual *walkRegionQual( region_qual regionQual );
	RepeatType walkOptRepeat( opt_repeat OptRepeat );
	TypeRef *walkTypeRef( type_ref typeRef );

	ReOrItem *walkRegOrChar( reg_or_char regOrChar );
	ReOrBlock *walkRegOrData( reg_or_data regOrData );

	Literal *walkLexRangeLit( lex_range_lit lexRangeLit )
	{
		Literal *literal = 0;
		if ( lexRangeLit.Lit() != 0 ) {
			String lit = lexRangeLit.Lit().text().c_str();
			literal = Literal::cons( internal, lit, Literal::LitString );
		}
		else if ( lexRangeLit.Number() != 0 ) {
			String num = lexRangeLit.Number().text().c_str();
			literal = Literal::cons( internal, num, Literal::Number );
		}
		return literal;
	}

	LexFactor *walkLexFactor( lex_factor LexFactorTree )
	{
		LexFactor *factor = 0;
		if ( LexFactorTree.Literal() != 0 ) {
			String litString = LexFactorTree.Literal().text().c_str();
			Literal *literal = Literal::cons( internal, litString, Literal::LitString );
			factor = LexFactor::cons( literal );
		}
		else if ( LexFactorTree.Id() != 0 ) {
			String id = LexFactorTree.Id().text().c_str();
			factor = lexRlFactorName( id, internal );
		}
		else if ( LexFactorTree.Expr() != 0 ) {
			lex_expr LexExpr = LexFactorTree.Expr();
			LexExpression *expr = walkLexExpr( LexExpr );
			LexJoin *join = LexJoin::cons( expr );
			factor = LexFactor::cons( join );
		}
		else if ( LexFactorTree.Low() != 0 ) {
			Literal *low = walkLexRangeLit( LexFactorTree.Low() );
			Literal *high = walkLexRangeLit( LexFactorTree.High() );

			Range *range = Range::cons( low, high );
			factor = LexFactor::cons( range );
		}
		else if ( LexFactorTree.PosData() != 0 ) {
			ReOrBlock *block = walkRegOrData( LexFactorTree.PosData() );
			factor = LexFactor::cons( ReItem::cons( internal, block, ReItem::OrBlock ) );
		}
		else if ( LexFactorTree.NegData() != 0 ) {
			ReOrBlock *block = walkRegOrData( LexFactorTree.NegData() );
			factor = LexFactor::cons( ReItem::cons( internal, block, ReItem::NegOrBlock ) );
		}
		else if ( LexFactorTree.Number() != 0 ) {
			String number = LexFactorTree.Number().text().c_str();
			factor = LexFactor::cons( Literal::cons( internal, 
					number, Literal::Number ) );
		}
		else if ( LexFactorTree.Hex() != 0 ) {
			String number = LexFactorTree.Hex().text().c_str();
			factor = LexFactor::cons( Literal::cons( internal, 
					number, Literal::Number ) );
		}
		return factor;
	}

	LexFactorNeg *walkLexFactorNeg( lex_factor_neg &LexFactorNegTree );
	LexFactorRep *walkLexFactorRep( lex_factor_rep LexFactorRepTree );

	LexFactorAug *walkLexFactorAug( lex_factor_rep LexFactorRepTree )
	{
		LexFactorRep *factorRep = walkLexFactorRep( LexFactorRepTree );
		return LexFactorAug::cons( factorRep );
	}

	LexTerm *walkLexTerm( lex_term LexTerm );
	ExprVect *walkCodeExprList( _repeat_code_expr codeExprList );

	LangExpr *walkCodeExpr( code_expr codeExpr )
	{
		LangExpr *expr = 0;
		if ( codeExpr.Expr() != 0 ) {
			LangExpr *left = walkCodeExpr( codeExpr.Expr() );
			LangExpr *right = walkCodeRelational( codeExpr.Relational() );

			char type;
			if ( codeExpr.BarBar() != 0 )
				type = OP_LogicalOr;
			else if ( codeExpr.AmpAmp() != 0 )
				type = OP_LogicalAnd;

			expr = LangExpr::cons( internal, left, type, right );
		}
		else {
			expr = walkCodeRelational( codeExpr.Relational() );
		}
		return expr;
	}

	void walkLexRegion( region_def regionDef );
	void walkProdElList( ProdElList *list, prod_el_list ProdElList );
	void walkProdList( LelDefList *lelDefList, prod_list ProdList );
	void walkCflDef( cfl_def cflDef );
	LangTerm *walkIterCall( iter_call IterCall );
	LangStmt *walkOptionalElse( optional_else optionalElse );
	LangStmt *walkElsifClause( elsif_clause elsifClause );
	LangStmt *walkElsifList( elsif_list elsifList );

	LangStmt *walkStatement( statement Statement )
	{
		LangStmt *stmt = 0;
		if ( Statement.Print() != 0 ) {
			print_stmt printStmt = Statement.Print();
			stmt = walkPrintStmt( printStmt );
		}
		else if ( Statement.Expr() != 0 ) {
			expr_stmt exprStmt = Statement.Expr();
			stmt = walkExprStmt( exprStmt );
		}
		else if ( Statement.VarDef() != 0 ) {
			ObjectField *objField = walkVarDef( Statement.VarDef() );
			LangExpr *expr = walkOptDefInit( Statement.OptDefInit() );
			stmt = varDef( objField, expr, LangStmt::AssignType );
		}
		else if ( Statement.ForDecl() != 0 ) {
			pushScope();

			String forDecl = Statement.ForDecl().text().c_str();
			TypeRef *typeRef = walkTypeRef( Statement.TypeRef() );
			StmtList *stmtList = walkBlockOrSingle( Statement.BlockOrSingle() );

			LangTerm *langTerm = walkIterCall( Statement.IterCall() );

			stmt = forScope( internal, forDecl, typeRef, langTerm, stmtList );

			popScope();
		}
		else if ( Statement.IfExpr() != 0 ) {
			pushScope();

			LangExpr *expr = walkCodeExpr( Statement.IfExpr() );
			StmtList *stmtList = walkBlockOrSingle( Statement.BlockOrSingle() );

			popScope();

			LangStmt *elsifList = walkElsifList( Statement.ElsifList() );
			stmt = LangStmt::cons( LangStmt::IfType, expr, stmtList, elsifList );

		}
		else if ( Statement.WhileExpr() != 0 ) {
			pushScope();
			LangExpr *expr = walkCodeExpr( Statement.WhileExpr() );
			StmtList *stmtList = walkBlockOrSingle( Statement.BlockOrSingle() );
			stmt = LangStmt::cons( LangStmt::WhileType, expr, stmtList );
			popScope();
		}
		else if ( Statement.LhsVarRef() != 0 ) {
			LangVarRef *varRef = walkVarRef( Statement.LhsVarRef() );
			LangExpr *expr = walkCodeExpr( Statement.CodeExpr() );
			stmt = LangStmt::cons( internal, LangStmt::AssignType, varRef, expr );
		}
		else if ( Statement.YieldVarRef() != 0 ) {
			LangVarRef *varRef = walkVarRef( Statement.YieldVarRef() );
			stmt = LangStmt::cons( LangStmt::YieldType, varRef );
		}
		else if ( Statement.ReturnExpr() != 0 ) {
			LangExpr *expr = walkCodeExpr( Statement.ReturnExpr() );
			stmt = LangStmt::cons( internal, LangStmt::ReturnType, expr );
		}
		else if ( Statement.Break() != 0 ) {
			stmt = LangStmt::cons( LangStmt::BreakType );
		}
		else if ( Statement.Reject() != 0 ) {
			stmt = LangStmt::cons( internal, LangStmt::RejectType );
		}
		return stmt;
	}

	LangStmt *walkPrintStmt( print_stmt &PrintStmt );
	LangExpr *walkCodeUnary( code_unary codeUnary );
	LangExpr *walkCodeFactor( code_factor codeFactor );
	LangStmt *walkExprStmt( expr_stmt &ExprStmt );
	QualItemVect *walkQual( qual &Qual );
	LangVarRef *walkVarRef( var_ref varRef );
	void walkRootItem( root_item &rootItem, StmtList *stmtList );
	StmtList *walkRootItemList( _repeat_root_item rootItemList );
	void walkNamespaceDef( namespace_def NamespaceDef );

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
			LangExpr *expr = match( internal, varRef, list );

 			StmtList *reqList = walkLangStmtList( langStmtList.OptRequire().LangStmtList() );

			LangStmt *stmt = LangStmt::cons( LangStmt::IfType, expr, reqList, 0 );

			popScope();

			retList->append( stmt );
		}

		return retList;
	}

	StmtList *walkBlockOrSingle( block_or_single blockOrSingle );

	void walkLiteralItem( literal_item literalItem );
	void walkLiteralList( literal_list literalList );
	void walkLiteralDef( literal_def literalDef );

	LexExpression *walkLexExpr( lex_expr LexExprTree )
	{
		if ( LexExprTree.Expr() != 0 ) {
			LexExpression *leftExpr = walkLexExpr( LexExprTree.Expr() );
			LexTerm *term = walkLexTerm( LexExprTree.Term() );

			LexExpression *expr = 0;
			if ( LexExprTree.Bar() != 0 )
				expr = LexExpression::cons( leftExpr, term, LexExpression::OrType );
			else if ( LexExprTree.Amp() != 0 )
				expr = LexExpression::cons( leftExpr, term, LexExpression::IntersectType );
			else if ( LexExprTree.Dash() != 0 )
				expr = LexExpression::cons( leftExpr, term, LexExpression::SubtractType );
			else if ( LexExprTree.DashDash() != 0 )
				expr = LexExpression::cons( leftExpr, term, LexExpression::StrongSubtractType );
			return expr;
		}
		else {
			lex_term LexTermTree = LexExprTree.Term();
			LexTerm *term = walkLexTerm( LexTermTree );
			LexExpression *expr = LexExpression::cons( term );
			return expr;
		}
	}


	void walkTokenDef( token_def TokenDef )
	{
		String name = TokenDef.Id().text().c_str();

		ObjectDef *objectDef = walkVarDefList( TokenDef.VarDefList() );
		objectDef->name = name;

		LexJoin *join = 0;
		if ( TokenDef.OptExpr().Expr() != 0 ) {
			LexExpression *expr = walkLexExpr( TokenDef.OptExpr().Expr() );
			join = LexJoin::cons( expr );
		}

		CodeBlock *translate = walkOptTranslate( TokenDef.OptTranslate() );

		defineToken( internal, name, join, objectDef, translate, false, false, false );
	}

	String walkOptId( opt_id optId )
	{
		String name = 0;
		if ( optId.Id() != 0 )
			name = optId.Id().text().c_str();
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

	void walkPreEof( pre_eof PreEof )
	{
		ObjectDef *localFrame = blockOpen();
		StmtList *stmtList = walkLangStmtList( PreEof.LangStmtList() );
		preEof( internal, stmtList, localFrame );
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

		defineToken( internal, name, join, objectDef, 0, true, false, false );
	}

	void walkContextDef( context_def contextDef );
	void walkContextItem( context_item contextItem );
	void walkContextVarDef( context_var_def contextVarDef );
	CodeBlock *walkOptReduce( opt_reduce optReduce );

	void walkFieldInit( FieldInitVect *list, field_init fieldInit );
	FieldInitVect *walkOptFieldInit( opt_field_init optFieldInit );

	ConsItemList *walkLitAccumEl( lit_accum_el litAccumEl );
	ConsItemList *walkLitAccumElList( _repeat_lit_accum_el litAccumElList, CONS_NL Nl );
	ConsItemList *walkAccumTopEl( accum_top_el accumTopEl );
	ConsItemList *walkAccumList( accum_list accumList );
	ConsItemList *walkAccumulate( accumulate Accumulate );
	ConsItemList *walkAccumEl( accum_el accumEl );
	ConsItemList *walkAccumElList( _repeat_accum_el accumElList );

	void walkRlDef( rl_def RlDef );

	ConsItemList *walkLitConsEl( lit_cons_el litConsEl );
	ConsItemList *walkLitConsElList( _repeat_lit_cons_el litConsElList );
	ConsItemList *walkLitConsElList( _repeat_lit_cons_el litConsElList, CONS_NL Nl );
	ConsItemList *walkConsTopEl( cons_top_el consTopEl );
	ConsItemList *walkConsList( cons_list consList );
	ConsItemList *walkConstructor( constructor Constructor );
	ConsItemList *walkConsEl( cons_el consEl );
	ConsItemList *walkConsElList( _repeat_cons_el consElList );

	LangExpr *walkCodeRelational( code_relational codeRelational );
	LangExpr *walkCodeAdditive( code_additive codeAdditive );

	LangExpr *walkCodeMultiplicitive( code_multiplicitive mult )
	{
		LangExpr *expr;
		if ( mult.Multiplicitive() != 0 ) {
			LangExpr *left = walkCodeMultiplicitive( mult.Multiplicitive() );
			LangExpr *right = walkCodeUnary( mult.Unary() );

			if ( mult.Star() != 0 ) {
				expr = LangExpr::cons( internal, left, '*', right );
			}
			else if ( mult.Fslash() != 0 ) {
				expr = LangExpr::cons( internal, left, '/', right );
			}
		}
		else {
			expr = walkCodeUnary( mult.Unary() );
		}
		return expr;
	}

	ConsItemList *walkLitStringEl( lit_string_el litStringEl );
	ConsItemList *walkLitStringElList( _repeat_lit_string_el litStringElList, CONS_NL Nl );
	ConsItemList *walkStringTopEl( string_top_el stringTopEl );
	ConsItemList *walkStringList( string_list stringList );
	ConsItemList *walkString( cstring String );
	ConsItemList *walkStringEl( string_el stringEl );
	ConsItemList *walkStringElList( _repeat_string_el stringElList );
	void walkFunctionDef( function_def functionDef );

	TypeRef *walkReferenceTypeRef( reference_type_ref ReferenceTypeRef );
	ObjectField *walkParamVarDef( param_var_def ParamVarDef );
	ParameterList *walkParamVarDefList( _repeat_param_var_def ParamVarDefList );
	void walkIterDef( iter_def IterDef );

	PatternItemList *walkPatternElTypeOrLit( pattern_el_type_or_lit typeOrLit )
	{
		NamespaceQual *nspaceQual = walkRegionQual( typeOrLit.RegionQual() );
		RepeatType repeatType = walkOptRepeat( typeOrLit.OptRepeat() );

		PatternItemList *list = 0;
		if ( typeOrLit.Id() != 0 ) {
			String id = typeOrLit.Id().text().c_str();
			list = patternElNamed( internal, nspaceQual, id, repeatType );
		}
		else if ( typeOrLit.Lit() != 0 ) { 
			String lit = typeOrLit.Lit().text().c_str();
			list = patternElType( internal, nspaceQual, lit, repeatType );
		}
		return list;
	}

	LangVarRef *walkOptLabel( opt_label optLabel )
	{
		LangVarRef *varRef = 0;
		if ( optLabel.Id() != 0 ) {
			String id = optLabel.Id().text().c_str();
			varRef = LangVarRef::cons( internal, id );
		}
		return varRef;
	}

	PatternItemList *walkPatternEl( pattern_el PatternEl )
	{
		PatternItemList *list = 0;
		if ( PatternEl.LitpatElList() != 0 ) {
			list = walkLitpatElList( PatternEl.LitpatElList(), PatternEl.Term().Nl() );
		}
		else if ( PatternEl.TildeData() != 0 ) {
			String patternData = PatternEl.TildeData().text().c_str();
			PatternItem *patternItem = PatternItem::cons( internal, patternData,
					PatternItem::InputText );
			PatternItemList *data = PatternItemList::cons( patternItem );

			patternData = PatternEl.Nl().text().c_str();
			patternItem = PatternItem::cons( internal, patternData, 
					PatternItem::InputText );
			PatternItemList *term = PatternItemList::cons( patternItem );

			list = patListConcat( data, term );

		}
		else if ( PatternEl.OptLabel() != 0 ) {
			LangVarRef *varRef = walkOptLabel( PatternEl.OptLabel() );
			PatternItemList *typeOrLitList = walkPatternElTypeOrLit( PatternEl.TypeOrLit() );
			list = patternEl( varRef, typeOrLitList );
		}
		return list;
	}

	PatternItemList *walkLitpatEl( litpat_el litpatEl )
	{
		PatternItemList *list = 0;
		if ( litpatEl.ConsData() != 0 ) {
			String consData = unescape( litpatEl.ConsData().text().c_str() );
			PatternItem *patternItem = PatternItem::cons( internal, consData,
					PatternItem::InputText );
			list = PatternItemList::cons( patternItem );
		}
		else if ( litpatEl.PatternElList() != 0 ) {
			list = walkPatternElList( litpatEl.PatternElList() );
		}
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
			String nl = unescape( Nl.text().c_str() );
			PatternItem *patternItem = PatternItem::cons( internal, nl, PatternItem::InputText );
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
		if ( patternTopEl.LitpatElList() != 0 ) {
			list = walkLitpatElList( patternTopEl.LitpatElList(), patternTopEl.Term().Nl() );
		}
		else if ( patternTopEl.TildeData() != 0 ) {
			String patternData = patternTopEl.TildeData().text().c_str();
			PatternItem *patternItem = PatternItem::cons( internal, patternData,
					PatternItem::InputText );
			PatternItemList *data = PatternItemList::cons( patternItem );

			patternData = patternTopEl.Nl().text().c_str();
			patternItem = PatternItem::cons( internal, patternData, 
					PatternItem::InputText );
			PatternItemList *term = PatternItemList::cons( patternItem );

			list = patListConcat( data, term );
		}
		else if ( patternTopEl.PatternElList() != 0 ) {
			list = walkPatternElList( patternTopEl.PatternElList() );
		}
		return list;
	}

	PatternItemList *walkPatternList( pattern_list patternList )
	{
		PatternItemList *list = 0;
		if ( patternList.PatternList() ) {
			PatternItemList *left = walkPatternList( patternList.PatternList() );
			PatternItemList *right = walkPattternTopEl( patternList.PatternTopEl() );
			list = patListConcat( left, right );
		}
		else {
			list = walkPattternTopEl( patternList.PatternTopEl() );
		}
		return list;
	}

	PatternItemList *walkPattern( pattern Pattern )
	{
		return walkPatternList( Pattern.PatternList() );
	}

	LangExpr *walkOptDefInit( opt_def_init optDefInit )
	{
		LangExpr *expr = 0;
		if ( optDefInit.CodeExpr() != 0 )
			expr = walkCodeExpr( optDefInit.CodeExpr() );
		return expr;
	}

	LangStmt *walkExportDef( export_def exportDef )
	{
		ObjectField *objField = walkVarDef( exportDef.VarDef() );
		LangExpr *expr = walkOptDefInit( exportDef.OptDefInit() );

		return exportStmt( objField, LangStmt::AssignType, expr );
	}

	void walkAliasDef( alias_def aliasDef )
	{
		String id = aliasDef.Id().text().c_str();
		TypeRef *typeRef = walkTypeRef( aliasDef.TypeRef() );
		alias( internal, id, typeRef );
	}

	CodeBlock *walkOptTranslate( opt_translate optTranslate )
	{
		CodeBlock *block = 0;
		if ( optTranslate.LangStmtList() != 0 ) {
			ObjectDef *localFrame = blockOpen();
			StmtList *stmtList = walkLangStmtList( optTranslate.LangStmtList() );
			block = CodeBlock::cons( stmtList, localFrame );
			block->context = contextStack.top();
			blockClose();
		}
		return block;
	}
};


BaseParser *consLoadSource( Compiler *pd, const char *inputFileName )
{
	return new LoadSource( pd, inputFileName );
}

NamespaceQual *LoadSource::walkRegionQual( region_qual regionQual )
{
	NamespaceQual *qual;
	if ( regionQual.RegionQual() != 0 ) {
		qual = walkRegionQual( regionQual.RegionQual() );
		qual->qualNames.append( String( regionQual.Id().text().c_str() ) );
	}
	else {
		qual = NamespaceQual::cons( namespaceStack.top() );
	}
	return qual;
}

RepeatType LoadSource::walkOptRepeat( opt_repeat OptRepeat )
{
	RepeatType repeatType = RepeatNone;
	if ( OptRepeat.Star() != 0 )
		repeatType = RepeatRepeat;
	else if ( OptRepeat.Plus() != 0 )
		repeatType = RepeatList;
	else if ( OptRepeat.Question() != 0 )
		repeatType = RepeatOpt;
	return repeatType;
}

TypeRef *LoadSource::walkTypeRef( type_ref typeRef )
{
	TypeRef *tr = 0;

	if ( typeRef.DirectId() != 0 ) {
		NamespaceQual *nspaceQual = walkRegionQual( typeRef.RegionQual() );
		String id = typeRef.DirectId().text().c_str();
		RepeatType repeatType = walkOptRepeat( typeRef.OptRepeat() );
		tr = TypeRef::cons( internal, nspaceQual, id, repeatType );
	}
	else if ( typeRef.PtrId() != 0 ) {
		NamespaceQual *nspaceQual = walkRegionQual( typeRef.RegionQual() );
		String id = typeRef.PtrId().text().c_str();
		RepeatType repeatType = walkOptRepeat( typeRef.OptRepeat() );
		TypeRef *inner = TypeRef::cons( internal, nspaceQual, id, repeatType );
		tr = TypeRef::cons( internal, TypeRef::Ptr, inner );
	}
	else if ( typeRef.MapKeyType() != 0 ) {
		TypeRef *key = walkTypeRef( typeRef.MapKeyType() );
		TypeRef *value = walkTypeRef( typeRef.MapValueType() );
		tr = TypeRef::cons( internal, TypeRef::Map, 0, key, value );
	}
	else if ( typeRef.ListType() != 0 ) {
		TypeRef *type = walkTypeRef( typeRef.ListType() );
		tr = TypeRef::cons( internal, TypeRef::List, 0, type, 0 );
	}
	else if ( typeRef.VectorType() != 0 ) {
		TypeRef *type = walkTypeRef( typeRef.VectorType() );
		tr = TypeRef::cons( internal, TypeRef::Vector, 0, type, 0 );
	}
	else if ( typeRef.ParserType() != 0 ) {
		TypeRef *type = walkTypeRef( typeRef.ParserType() );
		tr = TypeRef::cons( internal, TypeRef::Parser, 0, type, 0 );
	}
	return tr;
}

StmtList *LoadSource::walkBlockOrSingle( block_or_single blockOrSingle )
{
	StmtList *stmtList = 0;
	if ( blockOrSingle.Statement() != 0 ) {
		stmtList = new StmtList;
		LangStmt *stmt = walkStatement( blockOrSingle.Statement() );
		stmtList->append( stmt );
	}
	else if ( blockOrSingle.LangStmtList() != 0 ) {
		stmtList = walkLangStmtList( blockOrSingle.LangStmtList() );
	}

	return stmtList;
}

void LoadSource::walkProdElList( ProdElList *list, prod_el_list ProdElList )
{
	if ( ProdElList.ProdElList() != 0 ) {
		prod_el_list RightProdElList = ProdElList.ProdElList();
		walkProdElList( list, RightProdElList );
	}
	
	if ( ProdElList.ProdEl() != 0 ) {
		prod_el El = ProdElList.ProdEl();

		ObjectField *captureField = 0;
		if ( El.OptName().Name() != 0 ) {
			String fieldName = El.OptName().Name().text().c_str();
			captureField = ObjectField::cons( internal, 0, fieldName );
		}

		RepeatType repeatType = walkOptRepeat( El.OptRepeat() );
		NamespaceQual *nspaceQual = walkRegionQual( El.RegionQual() );

		if ( El.Id() != 0 ) {
			String typeName = El.Id().text().c_str();
			ProdEl *prodEl = prodElName( internal, typeName,
					nspaceQual,
					captureField, repeatType, false );
			appendProdEl( list, prodEl );
		}
		else if ( El.Lit() != 0 ) {
			String lit = El.Lit().text().c_str();
			ProdEl *prodEl = prodElLiteral( internal, lit,
					nspaceQual,
					captureField, repeatType, false );
			appendProdEl( list, prodEl );

		}
	}
}

CodeBlock *LoadSource::walkOptReduce( opt_reduce optReduce )
{
	CodeBlock *block = 0;
	if ( optReduce.LangStmtList() != 0 ) {
		ObjectDef *localFrame = blockOpen();
		StmtList *stmtList = walkLangStmtList( optReduce.LangStmtList() );

		block = CodeBlock::cons( stmtList, localFrame );
		block->context = contextStack.top();

		blockClose();
	}
	return block;
}

void LoadSource::walkProdList( LelDefList *lelDefList, prod_list ProdList )
{
	if ( ProdList.ProdList() != 0 )
		walkProdList( lelDefList, ProdList.ProdList() );

	ProdElList *list = new ProdElList;

	prod Prod = ProdList.Prod();

	walkProdElList( list, Prod.ProdElList() );
	CodeBlock *codeBlock = walkOptReduce( Prod.OptReduce() );

	bool commit = Prod.OptCommit().Commit() != 0;

	Production *prod = BaseParser::production( internal, list, commit, codeBlock, 0 );
	prodAppend( lelDefList, prod );
}

ReOrItem *LoadSource::walkRegOrChar( reg_or_char regOrChar )
{
	ReOrItem *orItem = 0;
	if ( regOrChar.Char() != 0 ) {
		String c = unescape( regOrChar.Char().text().c_str() );
		orItem = ReOrItem::cons( internal, c );
	}
	else {
		String low = unescape( regOrChar.Low().text().c_str() );
		String high = unescape( regOrChar.High().text().c_str() );
		orItem = ReOrItem::cons( internal, low[0], high[0] );
	}
	return orItem;
}

ReOrBlock *LoadSource::walkRegOrData( reg_or_data regOrData )
{
	ReOrBlock *block = 0;
	if ( regOrData.Data() != 0 ) {
		ReOrBlock *left = walkRegOrData( regOrData.Data() );
		ReOrItem *right = walkRegOrChar( regOrData.Char() );
		block = lexRegularExprData( left, right );
	}
	else {
		block = ReOrBlock::cons();
	}
	return block;
}


LexFactorNeg *LoadSource::walkLexFactorNeg( lex_factor_neg &LexFactorNegTree )
{
	if ( LexFactorNegTree.FactorNeg() != 0 ) {
		lex_factor_neg Rec = LexFactorNegTree.FactorNeg();
		LexFactorNeg *recNeg = walkLexFactorNeg( Rec );
		LexFactorNeg *factorNeg = LexFactorNeg::cons( internal, recNeg,
				LexFactorNeg::CharNegateType );
		return factorNeg;
	}
	else {
		lex_factor LexFactorTree = LexFactorNegTree.Factor();
		LexFactor *factor = walkLexFactor( LexFactorTree );
		LexFactorNeg *factorNeg = LexFactorNeg::cons( internal, factor );
		return factorNeg;
	}
}

LexFactorRep *LoadSource::walkLexFactorRep( lex_factor_rep LexFactorRepTree )
{
	LexFactorRep *factorRep = 0;
	if ( LexFactorRepTree.Star() != 0 ) {
		lex_factor_rep Rec = LexFactorRepTree.FactorRep();
		LexFactorRep *recRep = walkLexFactorRep( Rec );
		factorRep = LexFactorRep::cons( internal, recRep, 0, 0, LexFactorRep::StarType );
	}
	else if ( LexFactorRepTree.StarStar() != 0 ) {
		lex_factor_rep Rec = LexFactorRepTree.FactorRep();
		LexFactorRep *recRep = walkLexFactorRep( Rec );
		factorRep = LexFactorRep::cons( internal, recRep, 0, 0, LexFactorRep::StarStarType );
	}
	else if ( LexFactorRepTree.Plus() != 0 ) {
		lex_factor_rep Rec = LexFactorRepTree.FactorRep();
		LexFactorRep *recRep = walkLexFactorRep( Rec );
		factorRep = LexFactorRep::cons( internal, recRep, 0, 0, LexFactorRep::PlusType );
	}
	else if ( LexFactorRepTree.Question() != 0 ) {
		lex_factor_rep Rec = LexFactorRepTree.FactorRep();
		LexFactorRep *recRep = walkLexFactorRep( Rec );
		factorRep = LexFactorRep::cons( internal, recRep, 0, 0, LexFactorRep::OptionalType );
	}
	else if ( LexFactorRepTree.Exact() != 0 ) {
		LexFactorRep *recRep = walkLexFactorRep( LexFactorRepTree.FactorRep() );
		int low = atoi( LexFactorRepTree.Exact().text().c_str() );
		factorRep = LexFactorRep::cons( internal, recRep, low, 0, LexFactorRep::ExactType );
	}
	else if ( LexFactorRepTree.Max() != 0 ) {
		LexFactorRep *recRep = walkLexFactorRep( LexFactorRepTree.FactorRep() );
		int high = atoi( LexFactorRepTree.Max().text().c_str() );
		factorRep = LexFactorRep::cons( internal, recRep, 0, high, LexFactorRep::MaxType );
	}
	else if ( LexFactorRepTree.Min() != 0 ) {
		LexFactorRep *recRep = walkLexFactorRep( LexFactorRepTree.FactorRep() );
		int low = atoi( LexFactorRepTree.Min().text().c_str() );
		factorRep = LexFactorRep::cons( internal, recRep, low, 0, LexFactorRep::MinType );
	}
	else if ( LexFactorRepTree.Low() != 0 ) {
		LexFactorRep *recRep = walkLexFactorRep( LexFactorRepTree.FactorRep() );
		int low = atoi( LexFactorRepTree.Low().text().c_str() );
		int high = atoi( LexFactorRepTree.High().text().c_str() );
		factorRep = LexFactorRep::cons( internal, recRep, low, high, LexFactorRep::RangeType );
	}
	else {
		lex_factor_neg LexFactorNegTree = LexFactorRepTree.FactorNeg();
		LexFactorNeg *factorNeg = walkLexFactorNeg( LexFactorNegTree );
		factorRep = LexFactorRep::cons( internal, factorNeg );
	}

	return factorRep;
}

LexTerm *LoadSource::walkLexTerm( lex_term LexTermTree )
{
	LexTerm *term = 0;
	if ( LexTermTree.Term() != 0 ) {
		LexTerm *leftTerm = walkLexTerm( LexTermTree.Term() );
		LexFactorAug *factorAug = walkLexFactorAug( LexTermTree.FactorRep() );

		if ( LexTermTree.OptDot() != 0 ) {
			term = LexTerm::cons( leftTerm, factorAug, LexTerm::ConcatType );
		}
		else if ( LexTermTree.ColonGt() != 0 ) {
			term = LexTerm::cons( leftTerm, factorAug, LexTerm::RightStartType );
		}
		else if ( LexTermTree.ColonGtGt() != 0 ) {
			term = LexTerm::cons( leftTerm, factorAug, LexTerm::RightFinishType );
		}
		else if ( LexTermTree.LtColon() != 0 ) {
			term = LexTerm::cons( leftTerm, factorAug, LexTerm::LeftType );
		}
	}
	else {
		lex_factor_rep LexFactorRepTree = LexTermTree.FactorRep();
		LexFactorAug *factorAug = walkLexFactorAug( LexFactorRepTree );
		term = LexTerm::cons( factorAug );
	}
	return term;
}

void LoadSource::walkRlDef( rl_def rlDef )
{
	String id = rlDef.Id().text().c_str();

	lex_expr LexExpr = rlDef.Expr();
	LexExpression *expr = walkLexExpr( LexExpr );
	LexJoin *join = LexJoin::cons( expr );

	addRegularDef( internal, namespaceStack.top(), id, join );
}

void LoadSource::walkLexRegion( region_def regionDef )
{
	pushRegionSet( internal );
	walkRootItemList( regionDef.RootItemList() );
	popRegionSet();
}

void LoadSource::walkCflDef( cfl_def cflDef )
{
	String name = cflDef.DefId().text().c_str();
	ObjectDef *objectDef = walkVarDefList( cflDef.VarDefList() );
	objectDef->name = name;

	LelDefList *defList = new LelDefList;
	walkProdList( defList, cflDef.ProdList() );

	bool reduceFirst = cflDef.OptReduceFirst().ReduceFirst() != 0;

	NtDef *ntDef = NtDef::cons( name, namespaceStack.top(),
			contextStack.top(), reduceFirst );

	BaseParser::cflDef( ntDef, objectDef, defList );
}

ExprVect *LoadSource::walkCodeExprList( _repeat_code_expr codeExprList )
{
	ExprVect *exprVect = new ExprVect;
	while ( !codeExprList.end() ) {
		code_expr codeExpr = codeExprList.value();
		LangExpr *expr = walkCodeExpr( codeExpr );
		exprVect->append( expr );
		codeExprList = codeExprList.next();
	}
	return exprVect;
}

LangStmt *LoadSource::walkPrintStmt( print_stmt &printStmt )
{
	_repeat_code_expr codeExprList = printStmt.CodeExprList();
	ExprVect *exprVect = walkCodeExprList( codeExprList );

	LangStmt::Type type;
	if ( printStmt.Tree() != 0 )
		type = LangStmt::PrintType;
	else if ( printStmt.PrintStream() != 0 )
		type = LangStmt::PrintStreamType;
	else if ( printStmt.Xml() != 0 )
		type = LangStmt::PrintXMLType;
	else if ( printStmt.XmlAc() != 0 )
		type = LangStmt::PrintXMLACType;
		
	return LangStmt::cons( internal, type, exprVect );
}

QualItemVect *LoadSource::walkQual( qual &Qual )
{
	QualItemVect *qualItemVect;
	qual RecQual = Qual.Qual();
	if ( RecQual != 0 ) {
		qualItemVect = walkQual( RecQual );
		String id = Qual.Id().text().c_str();
		QualItem::Type type = Qual.Dot() != 0 ? QualItem::Dot : QualItem::Arrow;
		qualItemVect->append( QualItem( internal, id, type ) );
	}
	else {
		qualItemVect = new QualItemVect;
	}
	return qualItemVect;
}

LangVarRef *LoadSource::walkVarRef( var_ref varRef )
{
	qual Qual = varRef.Qual();
	QualItemVect *qualItemVect = walkQual( Qual );
	String id = varRef.Id().text().c_str();
	LangVarRef *langVarRef = LangVarRef::cons( internal, qualItemVect, id );
	return langVarRef;
}

ObjectField *walkOptCapture( opt_capture optCapture )
{
	ObjectField *objField = 0;
	if ( optCapture.Id() != 0 ) {
		String id = optCapture.Id().text().c_str();
		objField = ObjectField::cons( internal, 0, id );
	}
	return objField;
}

/*
 * Constructor
 */

ConsItemList *LoadSource::walkLitConsEl( lit_cons_el litConsEl )
{
	ConsItemList *list = 0;
	if ( litConsEl.ConsData() != 0 ) {
		String consData = unescape( litConsEl.ConsData().text().c_str() );
		ConsItem *consItem = ConsItem::cons( internal, ConsItem::InputText, consData );
		list = ConsItemList::cons( consItem );
	}
	else if ( litConsEl.ConsElList() != 0 ) {
		list = walkConsElList( litConsEl.ConsElList() );
	}
	return list;
}

ConsItemList *LoadSource::walkLitConsElList( _repeat_lit_cons_el litConsElList, CONS_NL Nl )
{
	ConsItemList *list = new ConsItemList;
	while ( !litConsElList.end() ) {
		ConsItemList *extension = walkLitConsEl( litConsElList.value() );
		list = consListConcat( list, extension );
		litConsElList = litConsElList.next();
	}

	if ( Nl != 0 ) {
		String consData = unescape( Nl.text().c_str() );
		ConsItem *consItem = ConsItem::cons( internal, ConsItem::InputText, consData );
		ConsItemList *term = ConsItemList::cons( consItem );
		list = consListConcat( list, term );
	}

	return list;
}

ConsItemList *LoadSource::walkConsEl( cons_el consEl )
{
	ConsItemList *list = 0;
	if ( consEl.Lit() != 0 ) {
		NamespaceQual *nspaceQual = walkRegionQual( consEl.RegionQual() );
		String lit = consEl.Lit().text().c_str();
		list = consElLiteral( internal, lit, nspaceQual );
	}
	else if ( consEl.CodeExpr() != 0 ) {
		LangExpr *consExpr = walkCodeExpr( consEl.CodeExpr() );
		ConsItem *consItem = ConsItem::cons( internal, ConsItem::ExprType, consExpr );
		list = ConsItemList::cons( consItem );
	}
	else if ( consEl.LitConsElList() != 0 ) {
		list = walkLitConsElList( consEl.LitConsElList(), consEl.Term().Nl() );
	}
	return list;
}

ConsItemList *LoadSource::walkConsElList( _repeat_cons_el consElList )
{
	ConsItemList *list = new ConsItemList;
	while ( !consElList.end() ) {
		ConsItemList *extension = walkConsEl( consElList.value() );
		list = consListConcat( list, extension );
		consElList = consElList.next();
	}
	return list;
}

ConsItemList *LoadSource::walkConsTopEl( cons_top_el consTopEl )
{
	ConsItemList *list = 0;
	if ( consTopEl.LitConsElList() != 0 )
		list = walkLitConsElList( consTopEl.LitConsElList(), consTopEl.Term().Nl() );
	else if ( consTopEl.ConsElList() != 0 ) {
		list = walkConsElList( consTopEl.ConsElList() );
	}
	return list;
}

ConsItemList *LoadSource::walkConsList( cons_list consList )
{
	ConsItemList *list = walkConsTopEl( consList.ConsTopEl() );

	if ( consList.ConsList() != 0 ) {
		ConsItemList *extension = walkConsList( consList.ConsList() );
		consListConcat( list, extension );
	}

	return list;
}

ConsItemList *LoadSource::walkConstructor( constructor Constructor )
{
	ConsItemList *list = walkConsList( Constructor.ConsList() );
	return list;
}

/*
 * String
 */

ConsItemList *LoadSource::walkLitStringEl( lit_string_el litStringEl )
{
	ConsItemList *list = 0;
	if ( litStringEl.ConsData() != 0 ) {
		String consData = unescape( litStringEl.ConsData().text().c_str() );
		ConsItem *stringItem = ConsItem::cons( internal, ConsItem::InputText, consData );
		list = ConsItemList::cons( stringItem );
	}
	else if ( litStringEl.StringElList() != 0 ) {
		list = walkStringElList( litStringEl.StringElList() );
	}
	return list;
}

ConsItemList *LoadSource::walkLitStringElList( _repeat_lit_string_el litStringElList, CONS_NL Nl )
{
	ConsItemList *list = new ConsItemList;
	while ( !litStringElList.end() ) {
		ConsItemList *extension = walkLitStringEl( litStringElList.value() );
		list = consListConcat( list, extension );
		litStringElList = litStringElList.next();
	}

	if ( Nl != 0 ) {
		String consData = unescape( Nl.text().c_str() );
		ConsItem *consItem = ConsItem::cons( internal, ConsItem::InputText, consData );
		ConsItemList *term = ConsItemList::cons( consItem );
		list = consListConcat( list, term );
	}
	return list;
}

ConsItemList *LoadSource::walkStringEl( string_el stringEl )
{
	ConsItemList *list = 0;
	if ( stringEl.LitStringElList() != 0 ) {
		list = walkLitStringElList( stringEl.LitStringElList(), stringEl.Term().Nl() );
	}
	else if ( stringEl.TildeData() != 0 ) {
		String consData = stringEl.TildeData().text().c_str();
		ConsItem *consItem = ConsItem::cons( internal, ConsItem::InputText, consData );
		ConsItemList *data = ConsItemList::cons( consItem );

		consData = stringEl.Nl().text().c_str();
		consItem = ConsItem::cons( internal, ConsItem::InputText, consData );
		ConsItemList *term = ConsItemList::cons( consItem );

		list = consListConcat( data, term );
	}
	else if ( stringEl.CodeExpr() != 0 ) {
		LangExpr *consExpr = walkCodeExpr( stringEl.CodeExpr() );
		ConsItem *consItem = ConsItem::cons( internal, ConsItem::ExprType, consExpr );
		list = ConsItemList::cons( consItem );
	}
	return list;
}

ConsItemList *LoadSource::walkStringElList( _repeat_string_el stringElList )
{
	ConsItemList *list = new ConsItemList;
	while ( !stringElList.end() ) {
		ConsItemList *extension = walkStringEl( stringElList.value() );
		list = consListConcat( list, extension );
		stringElList = stringElList.next();
	}
	return list;
}

ConsItemList *LoadSource::walkStringTopEl( string_top_el stringTopEl )
{
	ConsItemList *list = 0;
	if ( stringTopEl.LitStringElList() != 0 )
		list = walkLitStringElList( stringTopEl.LitStringElList(), stringTopEl.Term().Nl() );
	else if ( stringTopEl.TildeData() != 0 ) {
		String consData = stringTopEl.TildeData().text().c_str();
		ConsItem *consItem = ConsItem::cons( internal, ConsItem::InputText, consData );
		ConsItemList *data = ConsItemList::cons( consItem );

		consData = stringTopEl.Nl().text().c_str();
		consItem = ConsItem::cons( internal, ConsItem::InputText, consData );
		ConsItemList *term = ConsItemList::cons( consItem );

		list = consListConcat( data, term );
	}
	else if ( stringTopEl.StringElList() != 0 ) {
		list = walkStringElList( stringTopEl.StringElList() );
	}
	return list;
}

ConsItemList *LoadSource::walkStringList( string_list stringList )
{
	ConsItemList *list = walkStringTopEl( stringList.StringTopEl() );

	if ( stringList.StringList() != 0 ) {
		ConsItemList *extension = walkStringList( stringList.StringList() );
		consListConcat( list, extension );
	}

	return list;
}

ConsItemList *LoadSource::walkString( cstring String )
{
	ConsItemList *list = walkStringList( String.StringList() );
	return list;
}

/*
 * Accum
 */

ConsItemList *LoadSource::walkLitAccumEl( lit_accum_el litAccumEl )
{
	ConsItemList *list = 0;
	if ( litAccumEl.ConsData() != 0 ) {
		String consData = unescape( litAccumEl.ConsData().text().c_str() );
		ConsItem *consItem = ConsItem::cons( internal, ConsItem::InputText, consData );
		list = ConsItemList::cons( consItem );
	}
	else if ( litAccumEl.AccumElList() != 0 ) {
		list = walkAccumElList( litAccumEl.AccumElList() );
	}
	return list;
}

ConsItemList *LoadSource::walkLitAccumElList( _repeat_lit_accum_el litAccumElList, CONS_NL Nl )
{
	ConsItemList *list = new ConsItemList;
	while ( !litAccumElList.end() ) {
		ConsItemList *extension = walkLitAccumEl( litAccumElList.value() );
		list = consListConcat( list, extension );
		litAccumElList = litAccumElList.next();
	}

	if ( Nl != 0 ) {
		String consData = unescape( Nl.text().c_str() );
		ConsItem *consItem = ConsItem::cons( internal, ConsItem::InputText, consData );
		ConsItemList *term = ConsItemList::cons( consItem );
		list = consListConcat( list, term );
	}

	return list;
}

ConsItemList *LoadSource::walkAccumEl( accum_el accumEl )
{
	ConsItemList *list = 0;
	if ( accumEl.LitAccumElList() != 0 ) {
		list = walkLitAccumElList( accumEl.LitAccumElList(), accumEl.Term().Nl() );
	}
	else if ( accumEl.CodeExpr() != 0 ) {
		LangExpr *accumExpr = walkCodeExpr( accumEl.CodeExpr() );
		ConsItem *consItem = ConsItem::cons( internal, ConsItem::ExprType, accumExpr );
		list = ConsItemList::cons( consItem );
	}
	return list;
}

ConsItemList *LoadSource::walkAccumElList( _repeat_accum_el accumElList )
{
	ConsItemList *list = new ConsItemList;
	while ( !accumElList.end() ) {
		ConsItemList *extension = walkAccumEl( accumElList.value() );
		list = consListConcat( list, extension );
		accumElList = accumElList.next();
	}
	return list;
}

ConsItemList *LoadSource::walkAccumTopEl( accum_top_el accumTopEl )
{
	ConsItemList *list = 0;
	if ( accumTopEl.LitAccumElList() != 0 )
		list = walkLitAccumElList( accumTopEl.LitAccumElList(), accumTopEl.Term().Nl() );
	else if ( accumTopEl.AccumElList() != 0 ) {
		list = walkAccumElList( accumTopEl.AccumElList() );
	}
	return list;
}

ConsItemList *LoadSource::walkAccumList( accum_list accumList )
{
	ConsItemList *list = walkAccumTopEl( accumList.AccumTopEl() );

	if ( accumList.AccumList() != 0 ) {
		ConsItemList *extension = walkAccumList( accumList.AccumList() );
		consListConcat( list, extension );
	}

	return list;
}

ConsItemList *LoadSource::walkAccumulate( accumulate Accumulate )
{
	ConsItemList *list = walkAccumList( Accumulate.AccumList() );
	return list;
}

void LoadSource::walkFieldInit( FieldInitVect *list, field_init fieldInit )
{
	LangExpr *expr = walkCodeExpr( fieldInit.CodeExpr() );
	FieldInit *init = FieldInit::cons( internal, "_name", expr );
	list->append( init );
}

FieldInitVect *LoadSource::walkOptFieldInit( opt_field_init optFieldInit )
{
	FieldInitVect *list = 0;
	if ( optFieldInit.FieldInitList() != 0 ) {
		list = new FieldInitVect;
		_repeat_field_init fieldInitList = optFieldInit.FieldInitList();
		while ( !fieldInitList.end() ) {
			walkFieldInit( list, fieldInitList.value() );
			fieldInitList = fieldInitList.next();
		}
	}
	return list;
}

LangExpr *LoadSource::walkCodeFactor( code_factor codeFactor )
{
	LangExpr *expr = 0;
	if ( codeFactor.VarRef() != 0 ) {
		var_ref varRef = codeFactor.VarRef();
		LangVarRef *langVarRef = walkVarRef( varRef );

		LangTerm *term = 0;
		if ( codeFactor.CodeExprList() == 0 ) {
			term = LangTerm::cons( internal, LangTerm::VarRefType, langVarRef );
		}
		else {
			ExprVect *exprVect = walkCodeExprList( codeFactor.CodeExprList() );
			term = LangTerm::cons( internal, langVarRef, exprVect );
		}

		expr = LangExpr::cons( term );
	}
	else if ( codeFactor.Number() != 0 ) {
		String number = codeFactor.Number().text().c_str();
		LangTerm *term = LangTerm::cons( InputLoc(), LangTerm::NumberType, number );
		expr = LangExpr::cons( term );
	}
	else if ( codeFactor.Lit() != 0 ) {
		String lit = codeFactor.Lit().text().c_str();
		LangTerm *term = LangTerm::cons( internal, LangTerm::StringType, lit );
		expr = LangExpr::cons( term );
	}
	else if ( codeFactor.Parse() != 0 ) {
		/* The type we are parsing. */
		type_ref typeRefTree = codeFactor.TypeRef();
		TypeRef *typeRef = walkTypeRef( typeRefTree );
		ObjectField *objField = walkOptCapture( codeFactor.OptCapture() );
		FieldInitVect *init = walkOptFieldInit( codeFactor.OptFieldInit() );
		ConsItemList *list = walkAccumulate( codeFactor.Accumulate() );

		expr = parseCmd( internal, false, objField, typeRef, init, list );
	}
	else if ( codeFactor.ParseStop() != 0 ) {
		/* The type we are parsing. */
		type_ref typeRefTree = codeFactor.TypeRef();
		TypeRef *typeRef = walkTypeRef( typeRefTree );
		ObjectField *objField = walkOptCapture( codeFactor.OptCapture() );
		FieldInitVect *init = walkOptFieldInit( codeFactor.OptFieldInit() );
		ConsItemList *list = walkAccumulate( codeFactor.Accumulate() );

		expr = parseCmd( internal, true, objField, typeRef, init, list );
	}
	else if ( codeFactor.Cons() != 0 ) {
		/* The type we are parsing. */
		type_ref typeRefTree = codeFactor.TypeRef();
		TypeRef *typeRef = walkTypeRef( typeRefTree );
		ObjectField *objField = walkOptCapture( codeFactor.OptCapture() );
		ConsItemList *list = walkConstructor( codeFactor.Constructor() );
		FieldInitVect *init = walkOptFieldInit( codeFactor.OptFieldInit() );

		expr = construct( internal, objField, list, typeRef, init );
	}
	else if ( codeFactor.Send() != 0 ) {
		LangVarRef *varRef = walkVarRef( codeFactor.ToVarRef() );
		ConsItemList *list = walkAccumulate( codeFactor.Accumulate() );
		expr = send( internal, varRef, list );
	}
	else if ( codeFactor.Nil() != 0 ) {
		expr = LangExpr::cons( LangTerm::cons( internal, LangTerm::NilType ) );
	}
	else if ( codeFactor.True() != 0 ) {
		expr = LangExpr::cons( LangTerm::cons( internal, LangTerm::TrueType ) );
	}
	else if ( codeFactor.False() != 0 ) {
		expr = LangExpr::cons( LangTerm::cons( internal, LangTerm::FalseType ) );
	}
	else if ( codeFactor.ParenCodeExpr() != 0 ) {
		expr = walkCodeExpr( codeFactor.ParenCodeExpr() );
	}
	else if ( codeFactor.String() != 0 ) {
		ConsItemList *list = walkString( codeFactor.String() );
		expr = LangExpr::cons( LangTerm::cons( internal, list ) );
	}
	else if ( codeFactor.MatchVarRef() != 0 ) {
		LangVarRef *varRef = walkVarRef( codeFactor.MatchVarRef() );
		PatternItemList *list = walkPattern( codeFactor.Pattern() );
		expr = match( internal, varRef, list );
	}
	else if ( codeFactor.InVarRef() != 0 ) {
		TypeRef *typeRef = walkTypeRef( codeFactor.TypeRef() );
		LangVarRef *varRef = walkVarRef( codeFactor.InVarRef() );
		expr = LangExpr::cons( LangTerm::cons( internal,
				LangTerm::SearchType, typeRef, varRef ) );
	}
	else if ( codeFactor.MakeTreeExprList() != 0 ) {
		ExprVect *exprList = walkCodeExprList( codeFactor.MakeTreeExprList() );
		expr = LangExpr::cons( LangTerm::cons( internal, LangTerm::MakeTreeType, exprList ) );
	}
	else if ( codeFactor.MakeTokenExprList() != 0 ) {
		ExprVect *exprList = walkCodeExprList( codeFactor.MakeTokenExprList() );
		expr = LangExpr::cons( LangTerm::cons( internal, LangTerm::MakeTokenType, exprList ) );
	}
	else if ( codeFactor.TypeIdTypeRef() != 0 ) {
		TypeRef *typeRef = walkTypeRef( codeFactor.TypeIdTypeRef() );
		expr = LangExpr::cons( LangTerm::cons( internal,
				LangTerm::TypeIdType, typeRef ) );
	}
	else if ( codeFactor.NewCodeFactor() != 0 ) {
		LangExpr *newExpr = walkCodeFactor( codeFactor.NewCodeFactor() );
		expr = LangExpr::cons( LangTerm::cons( internal, LangTerm::NewType, newExpr ) );
	}
	return expr;
}

LangExpr *LoadSource::walkCodeAdditive( code_additive additive )
{
	LangExpr *expr = 0;
	if ( additive.Plus() != 0 ) {
		LangExpr *left = walkCodeAdditive( additive.Additive() );
		LangExpr *right = walkCodeMultiplicitive( additive.Multiplicitive() );
		expr = LangExpr::cons( internal, left, '+', right );
	}
	else if ( additive.Minus() != 0 ) {
		LangExpr *left = walkCodeAdditive( additive.Additive() );
		LangExpr *right = walkCodeMultiplicitive( additive.Multiplicitive() );
		expr = LangExpr::cons( internal, left, '-', right );
	}
	else {
		expr = walkCodeMultiplicitive( additive.Multiplicitive() );
	}
	return expr;
}


LangExpr *LoadSource::walkCodeUnary( code_unary unary )
{
	LangExpr *expr = walkCodeFactor( unary.Factor() );

	if ( unary.Bang() != 0 ) {
		expr = LangExpr::cons( internal, '!', expr );
	}
	else if ( unary.Dollar() != 0 ) {
		expr = LangExpr::cons( internal, '$', expr );
	}
	else if ( unary.Caret() != 0 ) {
		expr = LangExpr::cons( internal, '^', expr );
	}
	else if ( unary.Percent() != 0 ) {
		expr = LangExpr::cons( internal, '%', expr );
	}

	return expr;
}


LangExpr *LoadSource::walkCodeRelational( code_relational codeRelational )
{
	LangExpr *expr = 0;
	if ( codeRelational.Relational() != 0 ) {
		LangExpr *left = walkCodeRelational( codeRelational.Relational() );
		LangExpr *right = walkCodeAdditive( codeRelational.Additive() );

		if ( codeRelational.EqEq() != 0 ) {
			expr = LangExpr::cons( internal, left, OP_DoubleEql, right );
		}
		if ( codeRelational.Neq() != 0 ) {
			expr = LangExpr::cons( internal, left, OP_NotEql, right );
		}
		else if ( codeRelational.Lt() != 0 ) {
			expr = LangExpr::cons( internal, left, '<', right );
		}
		else if ( codeRelational.Gt() != 0 ) {
			expr = LangExpr::cons( internal, left, '>', right );
		}
	}
	else {
		expr = walkCodeAdditive( codeRelational.Additive() );
	}
	return expr;
}

LangStmt *LoadSource::walkExprStmt( expr_stmt &exprStmt )
{
	LangStmt *stmt;
	if ( exprStmt.CodeExpr() != 0 ) {
		code_expr codeExpr = exprStmt.CodeExpr();
		LangExpr *expr = walkCodeExpr( codeExpr );
		stmt = LangStmt::cons( internal, LangStmt::ExprType, expr );
	}
	return stmt;
}

ObjectField *LoadSource::walkVarDef( var_def varDef )
{
	TypeRef *typeRef = walkTypeRef( varDef.TypeRef() );
	String id = varDef.Id().text().c_str();
	return ObjectField::cons( internal, typeRef, id );
}

LangTerm *LoadSource::walkIterCall( iter_call IterCall )
{
	LangTerm *langTerm = 0;
	if ( IterCall.Id() != 0 ) {
		String tree = IterCall.Id().text().c_str();
		langTerm = LangTerm::cons( internal,
				LangTerm::VarRefType, LangVarRef::cons( internal, tree ) );
	}
	else {
		LangVarRef *varRef = walkVarRef( IterCall.VarRef() );
		ExprVect *exprVect = walkCodeExprList( IterCall.CodeExprList() );
		langTerm = LangTerm::cons( internal, varRef, exprVect );
	}
	
	return langTerm;
}


LangStmt *LoadSource::walkElsifClause( elsif_clause elsifClause )
{
	pushScope();
	LangExpr *expr = walkCodeExpr( elsifClause.ElsifExpr() );
	StmtList *stmtList = walkBlockOrSingle( elsifClause.BlockOrSingle() );
	LangStmt *stmt = LangStmt::cons( LangStmt::IfType, expr, stmtList, 0 );
	popScope();
	return stmt;
}

LangStmt *LoadSource::walkOptionalElse( optional_else optionalElse )
{
	LangStmt *stmt = 0;
	if ( optionalElse.BlockOrSingle() != 0 ) {
		pushScope();
		StmtList *stmtList = walkBlockOrSingle( optionalElse.BlockOrSingle() );
		stmt = LangStmt::cons( LangStmt::ElseType, stmtList );
		popScope();
	}
	return stmt;
}

LangStmt *LoadSource::walkElsifList( elsif_list elsifList )
{
	LangStmt *stmt = 0;
	if ( elsifList.ElsifList() != 0 ) {
		stmt = walkElsifClause( elsifList.ElsifClause() );
		stmt->elsePart = walkElsifList( elsifList.ElsifList() );
	}
	else {
		stmt = walkOptionalElse( elsifList.OptionalElse() );
	}
	return stmt;
}

void LoadSource::walkContextVarDef( context_var_def ContextVarDef )
{
	ObjectField *objField = walkVarDef( ContextVarDef.VarDef() );
	contextVarDef( internal, objField );
}

//def reference_type_ref
//	[REF type_ref]
//
//def param_var_def
//	[id COLON type_ref]
//|	[id COLON reference_type_ref]
//
//def param_list
//	[param_list param_var_def]
//|	[param_var_def]
//
//def opt_param_list
//	[param_list]
//|	[]

TypeRef *LoadSource::walkReferenceTypeRef( reference_type_ref ReferenceTypeRef )
{
	TypeRef *typeRef = walkTypeRef( ReferenceTypeRef.TypeRef() );
	return TypeRef::cons( internal, TypeRef::Ref, typeRef );
}

ObjectField *LoadSource::walkParamVarDef( param_var_def paramVarDef )
{
	String id = paramVarDef.Id().text().c_str();
	TypeRef *typeRef = 0;

	if ( paramVarDef.TypeRef() != 0 )
		typeRef = walkTypeRef( paramVarDef.TypeRef() );
	else
		typeRef = walkReferenceTypeRef( paramVarDef.RefTypeRef() );
	
	return addParam( internal, typeRef, id );
}

ParameterList *LoadSource::walkParamVarDefList( _repeat_param_var_def paramVarDefList )
{
	ParameterList *paramList = new ParameterList;
	while ( !paramVarDefList.end() ) {
		ObjectField *param = walkParamVarDef( paramVarDefList.value() );
		appendParam( paramList, param );
		paramVarDefList = paramVarDefList.next();
	}
	return paramList;
}

void LoadSource::walkFunctionDef( function_def FunctionDef )
{
	ObjectDef *localFrame = blockOpen();

	TypeRef *typeRef = walkTypeRef( FunctionDef.TypeRef() );
	String id = FunctionDef.Id().text().c_str();
	ParameterList *paramList = walkParamVarDefList( FunctionDef.ParamVarDefList() );
	StmtList *stmtList = walkLangStmtList( FunctionDef.LangStmtList() );
	functionDef( stmtList, localFrame, paramList, typeRef, id );

	blockClose();
}

void LoadSource::walkIterDef( iter_def IterDef )
{
	ObjectDef *localFrame = blockOpen();

	String id = IterDef.Id().text().c_str();
	ParameterList *paramList = walkParamVarDefList( IterDef.ParamVarDefList() );
	StmtList *stmtList = walkLangStmtList( IterDef.LangStmtList() );
	iterDef( stmtList, localFrame, paramList, id );

	blockClose();
}

void LoadSource::walkContextItem( context_item contextItem )
{
	if ( contextItem.RlDef() != 0 ) {
		walkRlDef( contextItem.RlDef() );
	}
	else if ( contextItem.ContextVarDef() != 0 ) {
		walkContextVarDef( contextItem.ContextVarDef() );
	}
	else if ( contextItem.TokenDef() != 0 ) {
		walkTokenDef( contextItem.TokenDef() );
	}
	else if ( contextItem.IgnoreDef() != 0 ) {
		walkIgnoreDef( contextItem.IgnoreDef() );
	}
	else if ( contextItem.LiteralDef() != 0 ) {
		walkLiteralDef( contextItem.LiteralDef() );
	}
	else if ( contextItem.CflDef() != 0 ) {
		walkCflDef( contextItem.CflDef() );
	}
	else if ( contextItem.RegionDef() != 0 ) {
		walkLexRegion( contextItem.RegionDef() );
	}
	else if ( contextItem.ContextDef() != 0 ) {
		walkContextDef( contextItem.ContextDef() );
	}
	else if ( contextItem.FunctionDef() != 0 ) {
		walkFunctionDef( contextItem.FunctionDef() );
	}
	else if ( contextItem.IterDef() != 0 ) {
		walkIterDef( contextItem.IterDef() );
	}
	else if ( contextItem.PreEof() != 0 ) {
		walkPreEof( contextItem.PreEof() );
	}
	else if ( contextItem.ExportDef() != 0 ) {
		walkExportDef( contextItem.ExportDef() );
	}
}

void LoadSource::walkContextDef( context_def contextDef )
{
	String name = contextDef.Name().text().c_str();
	contextHead( internal, name );

	_repeat_context_item contextItemList = contextDef.ContextItemList();
	while ( !contextItemList.end() ) {
		walkContextItem( contextItemList.value() );
		contextItemList = contextItemList.next();
	}

	contextStack.pop();
	namespaceStack.pop();
}

void LoadSource::walkNamespaceDef( namespace_def NamespaceDef )
{
	String name = NamespaceDef.Name().text().c_str();
	createNamespace( internal, name );
	walkRootItemList( NamespaceDef.RootItemList() );
	namespaceStack.pop();
}

void LoadSource::walkRootItem( root_item &rootItem, StmtList *stmtList )
{
	if ( rootItem.RlDef() != 0 ) {
		walkRlDef( rootItem.RlDef() );
	}
	else if ( rootItem.TokenDef() != 0 ) {
		walkTokenDef( rootItem.TokenDef() );
	}
	else if ( rootItem.IgnoreDef() != 0 ) {
		walkIgnoreDef( rootItem.IgnoreDef() );
	}
	else if ( rootItem.LiteralDef() != 0 ) {
		walkLiteralDef( rootItem.LiteralDef() );
	}
	else if ( rootItem.CflDef() != 0 ) {
		walkCflDef( rootItem.CflDef() );
	}
	else if ( rootItem.RegionDef() != 0 ) {
		walkLexRegion( rootItem.RegionDef() );
	}
	else if ( rootItem.Statement() != 0 ) {
		LangStmt *stmt = walkStatement( rootItem.Statement() );
		if ( stmt != 0 )
			stmtList->append( stmt );
	}
	else if ( rootItem.ContextDef() != 0 ) {
		walkContextDef( rootItem.ContextDef() );
	}
	else if ( rootItem.NamespaceDef() != 0 ) {
		walkNamespaceDef( rootItem.NamespaceDef() );
	}
	else if ( rootItem.FunctionDef() != 0 ) {
		walkFunctionDef( rootItem.FunctionDef() );
	}
	else if ( rootItem.IterDef() != 0 ) {
		walkIterDef( rootItem.IterDef() );
	}
	else if ( rootItem.IterDef() != 0 ) {
		walkIterDef( rootItem.IterDef() );
	}
	else if ( rootItem.PreEof() != 0 ) {
		walkPreEof( rootItem.PreEof() );
	}
	else if ( rootItem.ExportDef() != 0 ) {
		walkExportDef( rootItem.ExportDef() );
	}
	else if ( rootItem.AliasDef() != 0 ) {
		walkAliasDef( rootItem.AliasDef() );
	}
}

bool walkOptNoIgnore( opt_no_ignore OptNoIngore )
{
	return OptNoIngore.Ni() != 0;
}

void LoadSource::walkLiteralItem( literal_item literalItem )
{
	bool niLeft = walkOptNoIgnore( literalItem.NiLeft() );
	bool niRight = walkOptNoIgnore( literalItem.NiRight() );

	String lit = literalItem.Lit().text().c_str();
	if ( strcmp( lit, "''" ) == 0 )
		zeroDef( internal, lit, niLeft, niRight );
	else
		literalDef( internal, lit, niLeft, niRight );
}

void LoadSource::walkLiteralList( literal_list literalList )
{
	if ( literalList.LiteralList() != 0 )
		walkLiteralList( literalList.LiteralList() );
	walkLiteralItem( literalList.LiteralItem() );
}

void LoadSource::walkLiteralDef( literal_def literalDef )
{
	walkLiteralList( literalDef.LiteralList() );
}

StmtList *LoadSource::walkRootItemList( _repeat_root_item rootItemList )
{
	StmtList *stmtList = new StmtList;

	/* Walk the list of items. */
	while ( !rootItemList.end() ) {
		root_item rootItem = rootItemList.value();
		walkRootItem( rootItem, stmtList );
		rootItemList = rootItemList.next();
	}
	return stmtList;
}

void LoadSource::go()
{
	LoadSource::init();

	const char *argv[2];
	argv[0] = inputFileName;
	argv[1] = 0;

	colmInit( 0 );
	ColmProgram *program = colmNewProgram( &main_runtimeData );
	colmRunProgram( program, 1, argv );

	/* Extract the parse tree. */
	start Start = ColmTree( program );

	if ( Start == 0 ) {
		gblErrorCount += 1;
		std::cerr << inputFileName << ": parse error" << std::endl;
		return;
	}

	StmtList *stmtList = walkRootItemList( Start.RootItemList() );
	colmDeleteProgram( program );

	pd->rootCodeBlock = CodeBlock::cons( stmtList, 0 );
}

