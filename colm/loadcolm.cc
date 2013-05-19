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
		if ( lexRangeLit.Lit() != 0 ) {
			String lit = lexRangeLit.Lit().text().c_str();
			literal = Literal::cons( lexRangeLit.Lit().loc(), lit, Literal::LitString );
		}
		else if ( lexRangeLit.Number() != 0 ) {
			String num = lexRangeLit.Number().text().c_str();
			literal = Literal::cons( lexRangeLit.Number().loc(), num, Literal::Number );
		}
		return literal;
	}

	LexFactor *walkLexFactor( lex_factor LexFactorTree )
	{
		LexFactor *factor = 0;
		if ( LexFactorTree.Literal() != 0 ) {
			String litString = LexFactorTree.Literal().text().c_str();
			Literal *literal = Literal::cons( LexFactorTree.Literal().loc(),
					litString, Literal::LitString );
			factor = LexFactor::cons( literal );
		}
		else if ( LexFactorTree.Id() != 0 ) {
			String id = LexFactorTree.Id().text().c_str();
			factor = lexRlFactorName( id, LexFactorTree.Id().loc() );
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
			factor = LexFactor::cons( ReItem::cons( block, ReItem::OrBlock ) );
		}
		else if ( LexFactorTree.NegData() != 0 ) {
			ReOrBlock *block = walkRegOrData( LexFactorTree.NegData() );
			factor = LexFactor::cons( ReItem::cons( block, ReItem::NegOrBlock ) );
		}
		else if ( LexFactorTree.Number() != 0 ) {
			String number = LexFactorTree.Number().text().c_str();
			factor = LexFactor::cons( Literal::cons( LexFactorTree.Number().loc(), 
					number, Literal::Number ) );
		}
		else if ( LexFactorTree.Hex() != 0 ) {
			String number = LexFactorTree.Hex().text().c_str();
			factor = LexFactor::cons( Literal::cons( LexFactorTree.Hex().loc(), 
					number, Literal::Number ) );
		}
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
		if ( codeExpr.Expr() != 0 ) {
			LangExpr *left = walkCodeExpr( codeExpr.Expr() );
			LangExpr *right = walkCodeRelational( codeExpr.Relational() );

			char type = -1;
			InputLoc loc;
			if ( codeExpr.BarBar() != 0 ) {
				type = OP_LogicalOr;
				loc = codeExpr.BarBar().loc();
			}
			else if ( codeExpr.AmpAmp() != 0 ) {
				type = OP_LogicalAnd;
				loc = codeExpr.AmpAmp().loc();
			}

			expr = LangExpr::cons( loc, left, type, right );
		}
		else {
			expr = walkCodeRelational( codeExpr.Relational() );
		}
		return expr;
	}

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

			stmt = forScope( Statement.ForDecl().loc(), forDecl, typeRef, langTerm, stmtList );

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
			stmt = LangStmt::cons( varRef->loc, LangStmt::AssignType, varRef, expr );
		}
		else if ( Statement.YieldVarRef() != 0 ) {
			LangVarRef *varRef = walkVarRef( Statement.YieldVarRef() );
			stmt = LangStmt::cons( LangStmt::YieldType, varRef );
		}
		else if ( Statement.ReturnExpr() != 0 ) {
			LangExpr *expr = walkCodeExpr( Statement.ReturnExpr() );
			stmt = LangStmt::cons( Statement.loc(), LangStmt::ReturnType, expr );
		}
		else if ( Statement.Break() != 0 ) {
			stmt = LangStmt::cons( LangStmt::BreakType );
		}
		else if ( Statement.Reject() != 0 ) {
			stmt = LangStmt::cons( Statement.Reject().loc(), LangStmt::RejectType );
		}
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
		if ( mult.Multiplicitive() != 0 ) {
			LangExpr *left = walkCodeMultiplicitive( mult.Multiplicitive() );
			LangExpr *right = walkCodeUnary( mult.Unary() );

			if ( mult.Star() != 0 ) {
				expr = LangExpr::cons( mult.Star().loc(), left, '*', right );
			}
			else if ( mult.Fslash() != 0 ) {
				expr = LangExpr::cons( mult.Fslash().loc(), left, '/', right );
			}
		}
		else {
			expr = walkCodeUnary( mult.Unary() );
		}
		return expr;
	}

	PatternItemList *walkPatternElTypeOrLit( pattern_el_lel typeOrLit )
	{
		NamespaceQual *nspaceQual = walkRegionQual( typeOrLit.RegionQual() );
		RepeatType repeatType = walkOptRepeat( typeOrLit.OptRepeat() );

		PatternItemList *list = 0;
		if ( typeOrLit.Id() != 0 ) {
			String id = typeOrLit.Id().text().c_str();
			list = patternElNamed( typeOrLit.Id().loc(), nspaceQual, id, repeatType );
		}
		else if ( typeOrLit.Lit() != 0 ) { 
			String lit = typeOrLit.Lit().text().c_str();
			list = patternElType( typeOrLit.Lit().loc(), nspaceQual, lit, repeatType );
		}
		return list;
	}

	LangVarRef *walkOptLabel( opt_label optLabel )
	{
		LangVarRef *varRef = 0;
		if ( optLabel.Id() != 0 ) {
			String id = optLabel.Id().text().c_str();
			varRef = LangVarRef::cons( optLabel.Id().loc(), id );
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
			patternData += '\n';
			PatternItem *patternItem = PatternItem::cons( PatternEl.TildeData().loc(),
					patternData, PatternItem::InputText );
			list = PatternItemList::cons( patternItem );
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
			PatternItem *patternItem = PatternItem::cons( litpatEl.ConsData().loc(),
					consData, PatternItem::InputText );
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
		if ( patternTopEl.LitpatElList() != 0 ) {
			list = walkLitpatElList( patternTopEl.LitpatElList(), patternTopEl.Term().Nl() );
		}
		else if ( patternTopEl.TildeData() != 0 ) {
			String patternData = patternTopEl.TildeData().text().c_str();
			patternData += '\n';
			PatternItem *patternItem = PatternItem::cons( patternTopEl.TildeData().loc(),
					patternData, PatternItem::InputText );
			list = PatternItemList::cons( patternItem );
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

	LangStmt *walkGlobalDef( global_def GlobalDef )
	{
		ObjectField *objField = walkVarDef( GlobalDef.VarDef() );
		LangExpr *expr = walkOptDefInit( GlobalDef.OptDefInit() );

		return globalDef( objField, expr, LangStmt::AssignType );
	}

	void walkAliasDef( alias_def aliasDef )
	{
		String id = aliasDef.Id().text().c_str();
		TypeRef *typeRef = walkTypeRef( aliasDef.TypeRef() );
		alias( aliasDef.Id().loc(), id, typeRef );
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

	PredDecl *walkPredToken( pred_token predToken )
	{
		NamespaceQual *nspaceQual = walkRegionQual( predToken.RegionQual() );
		PredDecl *predDecl = 0;
		if ( predToken.Id() != 0 ) {
			String id = predToken.Id().text().c_str();
			predDecl = predTokenName( predToken.Id().loc(), nspaceQual, id );
		}
		else if ( predToken.Lit() != 0 ) {
			String lit = predToken.Lit().text().c_str();
			predDecl = predTokenLit( predToken.Lit().loc(), lit, nspaceQual );
		}
		return predDecl;
	}

	PredDeclList *walkPredTokenList( pred_token_list predTokenList )
	{
		PredDeclList *list = 0;
		if ( predTokenList.PredTokenList() != 0 ) {
			list = walkPredTokenList( predTokenList.PredTokenList() );
			PredDecl *predDecl = walkPredToken( predTokenList.PredToken() );
			list->append( predDecl );
		}
		else {
			PredDecl *predDecl = walkPredToken( predTokenList.PredToken() );
			list = new PredDeclList;
			list->append( predDecl );
		}
		return list;
	}

	PredType walkPredType( pred_type predType )
	{
		PredType pt;
		if ( predType.Left() != 0 )
			pt = PredLeft;
		else if ( predType.Right() != 0 )
			pt = PredRight;
		else if ( predType.NonAssoc() != 0 )
			pt = PredNonassoc;
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
		String lit = Include.File().text().c_str();
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

	RepeatType walkOptRepeat( opt_repeat OptRepeat )
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

	TypeRef *walkTypeRef( type_ref typeRef )
	{
		TypeRef *tr = 0;

		if ( typeRef.DirectId() != 0 ) {
			NamespaceQual *nspaceQual = walkRegionQual( typeRef.RegionQual() );
			String id = typeRef.DirectId().text().c_str();
			RepeatType repeatType = walkOptRepeat( typeRef.OptRepeat() );
			tr = TypeRef::cons( typeRef.DirectId().loc(), nspaceQual, id, repeatType );
		}
		else if ( typeRef.PtrId() != 0 ) {
			NamespaceQual *nspaceQual = walkRegionQual( typeRef.RegionQual() );
			String id = typeRef.PtrId().text().c_str();
			RepeatType repeatType = walkOptRepeat( typeRef.OptRepeat() );
			TypeRef *inner = TypeRef::cons( typeRef.PtrId().loc(), nspaceQual, id, repeatType );
			tr = TypeRef::cons( typeRef.PtrId().loc(), TypeRef::Ptr, inner );
		}
		else if ( typeRef.MapKeyType() != 0 ) {
			TypeRef *key = walkTypeRef( typeRef.MapKeyType() );
			TypeRef *value = walkTypeRef( typeRef.MapValueType() );
			tr = TypeRef::cons( typeRef.loc(), TypeRef::Map, 0, key, value );
		}
		else if ( typeRef.ListType() != 0 ) {
			TypeRef *type = walkTypeRef( typeRef.ListType() );
			tr = TypeRef::cons( typeRef.loc(), TypeRef::List, 0, type, 0 );
		}
		else if ( typeRef.VectorType() != 0 ) {
			TypeRef *type = walkTypeRef( typeRef.VectorType() );
			tr = TypeRef::cons( typeRef.loc(), TypeRef::Vector, 0, type, 0 );
		}
		else if ( typeRef.ParserType() != 0 ) {
			TypeRef *type = walkTypeRef( typeRef.ParserType() );
			tr = TypeRef::cons( typeRef.loc(), TypeRef::Parser, 0, type, 0 );
		}
		return tr;
	}

	StmtList *walkBlockOrSingle( block_or_single blockOrSingle )
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

	void walkProdEl( ProdElList *list, prod_el El )
	{
		ObjectField *captureField = 0;
		if ( El.OptName().Name() != 0 ) {
			String fieldName = El.OptName().Name().text().c_str();
			captureField = ObjectField::cons( El.OptName().Name().loc(), 0, fieldName );
		}

		RepeatType repeatType = walkOptRepeat( El.OptRepeat() );
		NamespaceQual *nspaceQual = walkRegionQual( El.RegionQual() );

		if ( El.Id() != 0 ) {
			String typeName = El.Id().text().c_str();
			ProdEl *prodEl = prodElName( El.Id().loc(), typeName,
					nspaceQual,
					captureField, repeatType, false );
			appendProdEl( list, prodEl );
		}
		else if ( El.Lit() != 0 ) {
			String lit = El.Lit().text().c_str();
			ProdEl *prodEl = prodElLiteral( El.Lit().loc(), lit,
					nspaceQual,
					captureField, repeatType, false );
			appendProdEl( list, prodEl );

		}
	}

	void walkProdElList( ProdElList *list, prod_el_list ProdElList )
	{
		if ( ProdElList.ProdElList() != 0 ) {
			prod_el_list RightProdElList = ProdElList.ProdElList();
			walkProdElList( list, RightProdElList );
		}
		
		if ( ProdElList.ProdEl() != 0 ) 
			walkProdEl( list, ProdElList.ProdEl() );
	}

	CodeBlock *walkOptReduce( opt_reduce OptReduce )
	{
		CodeBlock *block = 0;
		if ( OptReduce.LangStmtList() != 0 ) {
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
		if ( Prod.OptName().Name() != 0 )
			name = Prod.OptName().Name().text().c_str();

		CodeBlock *codeBlock = walkOptReduce( Prod.OptReduce() );
		bool commit = Prod.OptCommit().Commit() != 0;

		Production *prod = BaseParser::production( Prod.Open().loc(), list, name, commit, codeBlock, 0 );
		prodAppend( lelDefList, prod );
	}

	void walkProdList( LelDefList *lelDefList, prod_list ProdList )
	{
		if ( ProdList.ProdList() != 0 )
			walkProdList( lelDefList, ProdList.ProdList() );

		walkProdudction( lelDefList, ProdList.Prod() );
	}

	ReOrItem *walkRegOrChar( reg_or_char regOrChar )
	{
		ReOrItem *orItem = 0;
		if ( regOrChar.Char() != 0 ) {
			String c = unescape( regOrChar.Char().text().c_str() );
			orItem = ReOrItem::cons( regOrChar.Char().loc(), c );
		}
		else {
			String low = unescape( regOrChar.Low().text().c_str() );
			String high = unescape( regOrChar.High().text().c_str() );
			orItem = ReOrItem::cons( regOrChar.Low().loc(), low[0], high[0] );
		}
		return orItem;
	}

	ReOrBlock *walkRegOrData( reg_or_data regOrData )
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


	LexFactorNeg *walkLexFactorNeg( lex_factor_neg &LexFactorNegTree )
	{
		if ( LexFactorNegTree.Caret() != 0 ) {
			lex_factor_neg Rec = LexFactorNegTree.FactorNeg();
			LexFactorNeg *recNeg = walkLexFactorNeg( Rec );
			LexFactorNeg *factorNeg = LexFactorNeg::cons( recNeg,
					LexFactorNeg::CharNegateType );
			return factorNeg;
		}
		else {
			lex_factor LexFactorTree = LexFactorNegTree.Factor();
			LexFactor *factor = walkLexFactor( LexFactorTree );
			LexFactorNeg *factorNeg = LexFactorNeg::cons( factor );
			return factorNeg;
		}
	}

	LexFactorRep *walkLexFactorRep( lex_factor_rep LexFactorRepTree )
	{
		LexFactorRep *factorRep = 0;
		if ( LexFactorRepTree.Star() != 0 ) {
			lex_factor_rep Rec = LexFactorRepTree.FactorRep();
			LexFactorRep *recRep = walkLexFactorRep( Rec );
			factorRep = LexFactorRep::cons( LexFactorRepTree.Star().loc(),
					recRep, 0, 0, LexFactorRep::StarType );
		}
		else if ( LexFactorRepTree.StarStar() != 0 ) {
			lex_factor_rep Rec = LexFactorRepTree.FactorRep();
			LexFactorRep *recRep = walkLexFactorRep( Rec );
			factorRep = LexFactorRep::cons( LexFactorRepTree.StarStar().loc(),
					recRep, 0, 0, LexFactorRep::StarStarType );
		}
		else if ( LexFactorRepTree.Plus() != 0 ) {
			lex_factor_rep Rec = LexFactorRepTree.FactorRep();
			LexFactorRep *recRep = walkLexFactorRep( Rec );
			factorRep = LexFactorRep::cons( LexFactorRepTree.Plus().loc(),
					recRep, 0, 0, LexFactorRep::PlusType );
		}
		else if ( LexFactorRepTree.Question() != 0 ) {
			lex_factor_rep Rec = LexFactorRepTree.FactorRep();
			LexFactorRep *recRep = walkLexFactorRep( Rec );
			factorRep = LexFactorRep::cons( LexFactorRepTree.Question().loc(),
					recRep, 0, 0, LexFactorRep::OptionalType );
		}
		else if ( LexFactorRepTree.Exact() != 0 ) {
			LexFactorRep *recRep = walkLexFactorRep( LexFactorRepTree.FactorRep() );
			int low = atoi( LexFactorRepTree.Exact().text().c_str() );
			factorRep = LexFactorRep::cons( LexFactorRepTree.Exact().loc(),
					recRep, low, 0, LexFactorRep::ExactType );
		}
		else if ( LexFactorRepTree.Max() != 0 ) {
			LexFactorRep *recRep = walkLexFactorRep( LexFactorRepTree.FactorRep() );
			int high = atoi( LexFactorRepTree.Max().text().c_str() );
			factorRep = LexFactorRep::cons( LexFactorRepTree.Max().loc(),
					recRep, 0, high, LexFactorRep::MaxType );
		}
		else if ( LexFactorRepTree.Min() != 0 ) {
			LexFactorRep *recRep = walkLexFactorRep( LexFactorRepTree.FactorRep() );
			int low = atoi( LexFactorRepTree.Min().text().c_str() );
			factorRep = LexFactorRep::cons( LexFactorRepTree.Min().loc(),
					recRep, low, 0, LexFactorRep::MinType );
		}
		else if ( LexFactorRepTree.Low() != 0 ) {
			LexFactorRep *recRep = walkLexFactorRep( LexFactorRepTree.FactorRep() );
			int low = atoi( LexFactorRepTree.Low().text().c_str() );
			int high = atoi( LexFactorRepTree.High().text().c_str() );
			factorRep = LexFactorRep::cons( LexFactorRepTree.Low().loc(),
					recRep, low, high, LexFactorRep::RangeType );
		}
		else {
			lex_factor_neg LexFactorNegTree = LexFactorRepTree.FactorNeg();
			LexFactorNeg *factorNeg = walkLexFactorNeg( LexFactorNegTree );
			factorRep = LexFactorRep::cons( factorNeg );
		}

		return factorRep;
	}

	LexTerm *walkLexTerm( lex_term LexTermTree )
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

	void walkRlDef( rl_def rlDef )
	{
		String id = rlDef.Id().text().c_str();

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

	ExprVect *walkCodeExprList( _repeat_code_expr codeExprList )
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

	LangStmt *walkPrintStmt( print_stmt &printStmt )
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
			
		return LangStmt::cons( printStmt.O().loc(), type, exprVect );
	}

	QualItemVect *walkQual( qual &Qual )
	{
		QualItemVect *qualItemVect;
		qual RecQual = Qual.Qual();
		if ( RecQual != 0 ) {
			qualItemVect = walkQual( RecQual );
			String id = Qual.Id().text().c_str();
			QualItem::Type type = Qual.Dot() != 0 ? QualItem::Dot : QualItem::Arrow;
			qualItemVect->append( QualItem( Qual.Id().loc(), id, type ) );
		}
		else {
			qualItemVect = new QualItemVect;
		}
		return qualItemVect;
	}

	LangVarRef *walkVarRef( var_ref varRef )
	{
		qual Qual = varRef.Qual();
		QualItemVect *qualItemVect = walkQual( Qual );
		String id = varRef.Id().text().c_str();
		LangVarRef *langVarRef = LangVarRef::cons( varRef.Id().loc(), qualItemVect, id );
		return langVarRef;
	}

	ObjectField *walkOptCapture( opt_capture optCapture )
	{
		ObjectField *objField = 0;
		if ( optCapture.Id() != 0 ) {
			String id = optCapture.Id().text().c_str();
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
		if ( litConsEl.ConsData() != 0 ) {
			String consData = unescape( litConsEl.ConsData().text().c_str() );
			ConsItem *consItem = ConsItem::cons( litConsEl.ConsData().loc(),
					ConsItem::InputText, consData );
			list = ConsItemList::cons( consItem );
		}
		else if ( litConsEl.ConsElList() != 0 ) {
			list = walkConsElList( litConsEl.ConsElList() );
		}
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
			String consData = unescape( Nl.text().c_str() );
			ConsItem *consItem = ConsItem::cons( Nl.loc(), ConsItem::InputText, consData );
			ConsItemList *term = ConsItemList::cons( consItem );
			list = consListConcat( list, term );
		}

		return list;
	}

	ConsItemList *walkConsEl( cons_el consEl )
	{
		ConsItemList *list = 0;
		if ( consEl.Lit() != 0 ) {
			NamespaceQual *nspaceQual = walkRegionQual( consEl.RegionQual() );
			String lit = consEl.Lit().text().c_str();
			list = consElLiteral( consEl.Lit().loc(), lit, nspaceQual );
		}
		else if ( consEl.TildeData() != 0 ) {
			String consData = consEl.TildeData().text().c_str();
			consData += '\n';
			ConsItem *consItem = ConsItem::cons( consEl.TildeData().loc(),
					ConsItem::InputText, consData );
			list = ConsItemList::cons( consItem );
		}
		else if ( consEl.CodeExpr() != 0 ) {
			LangExpr *consExpr = walkCodeExpr( consEl.CodeExpr() );
			ConsItem *consItem = ConsItem::cons( consExpr->loc, ConsItem::ExprType, consExpr );
			list = ConsItemList::cons( consItem );
		}
		else if ( consEl.LitConsElList() != 0 ) {
			list = walkLitConsElList( consEl.LitConsElList(), consEl.Term().Nl() );
		}
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
		if ( consTopEl.LitConsElList() != 0 )
			list = walkLitConsElList( consTopEl.LitConsElList(), consTopEl.Term().Nl() );
		else if ( consTopEl.TildeData() != 0 ) {
			String consData = consTopEl.TildeData().text().c_str();
			consData += '\n';
			ConsItem *consItem = ConsItem::cons( consTopEl.TildeData().loc(),
					ConsItem::InputText, consData );
			list = ConsItemList::cons( consItem );
		}
		else if ( consTopEl.ConsElList() != 0 ) {
			list = walkConsElList( consTopEl.ConsElList() );
		}
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
		if ( litStringEl.ConsData() != 0 ) {
			String consData = unescape( litStringEl.ConsData().text().c_str() );
			ConsItem *stringItem = ConsItem::cons( litStringEl.ConsData().loc(),
					ConsItem::InputText, consData );
			list = ConsItemList::cons( stringItem );
		}
		else if ( litStringEl.StringElList() != 0 ) {
			list = walkStringElList( litStringEl.StringElList() );
		}
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
			String consData = unescape( Nl.text().c_str() );
			ConsItem *consItem = ConsItem::cons( Nl.loc(), ConsItem::InputText, consData );
			ConsItemList *term = ConsItemList::cons( consItem );
			list = consListConcat( list, term );
		}
		return list;
	}

	ConsItemList *walkStringEl( string_el stringEl )
	{
		ConsItemList *list = 0;
		if ( stringEl.LitStringElList() != 0 ) {
			list = walkLitStringElList( stringEl.LitStringElList(), stringEl.Term().Nl() );
		}
		else if ( stringEl.TildeData() != 0 ) {
			String consData = stringEl.TildeData().text().c_str();
			consData += '\n';
			ConsItem *consItem = ConsItem::cons( stringEl.TildeData().loc(),
					ConsItem::InputText, consData );
			list = ConsItemList::cons( consItem );
		}
		else if ( stringEl.CodeExpr() != 0 ) {
			LangExpr *consExpr = walkCodeExpr( stringEl.CodeExpr() );
			ConsItem *consItem = ConsItem::cons( consExpr->loc, ConsItem::ExprType, consExpr );
			list = ConsItemList::cons( consItem );
		}
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
		if ( stringTopEl.LitStringElList() != 0 )
			list = walkLitStringElList( stringTopEl.LitStringElList(), stringTopEl.Term().Nl() );
		else if ( stringTopEl.TildeData() != 0 ) {
			String consData = stringTopEl.TildeData().text().c_str();
			consData += '\n';
			ConsItem *consItem = ConsItem::cons( stringTopEl.TildeData().loc(),
					ConsItem::InputText, consData );
			list = ConsItemList::cons( consItem );
		}
		else if ( stringTopEl.StringElList() != 0 ) {
			list = walkStringElList( stringTopEl.StringElList() );
		}
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
		if ( litAccumEl.ConsData() != 0 ) {
			String consData = unescape( litAccumEl.ConsData().text().c_str() );
			ConsItem *consItem = ConsItem::cons( litAccumEl.ConsData().loc(),
					ConsItem::InputText, consData );
			list = ConsItemList::cons( consItem );
		}
		else if ( litAccumEl.AccumElList() != 0 ) {
			list = walkAccumElList( litAccumEl.AccumElList() );
		}
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
			String consData = unescape( Nl.text().c_str() );
			ConsItem *consItem = ConsItem::cons( Nl.loc(), ConsItem::InputText, consData );
			ConsItemList *term = ConsItemList::cons( consItem );
			list = consListConcat( list, term );
		}

		return list;
	}

	ConsItemList *walkAccumEl( accum_el accumEl )
	{
		ConsItemList *list = 0;
		if ( accumEl.LitAccumElList() != 0 ) {
			list = walkLitAccumElList( accumEl.LitAccumElList(), accumEl.Term().Nl() );
		}
		else if ( accumEl.TildeData() != 0 ) {
			String consData = accumEl.TildeData().text().c_str();
			consData += '\n';
			ConsItem *consItem = ConsItem::cons( accumEl.TildeData().loc(),
					ConsItem::InputText, consData );
			list = ConsItemList::cons( consItem );
		}
		else if ( accumEl.CodeExpr() != 0 ) {
			LangExpr *accumExpr = walkCodeExpr( accumEl.CodeExpr() );
			ConsItem *consItem = ConsItem::cons( accumExpr->loc, ConsItem::ExprType, accumExpr );
			list = ConsItemList::cons( consItem );
		}
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
		if ( accumTopEl.LitAccumElList() != 0 )
			list = walkLitAccumElList( accumTopEl.LitAccumElList(), accumTopEl.Term().Nl() );
		else if ( accumTopEl.TildeData() != 0 ) {
			String consData = accumTopEl.TildeData().text().c_str();
			consData += '\n';
			ConsItem *consItem = ConsItem::cons( accumTopEl.TildeData().loc(),
					ConsItem::InputText, consData );
			list = ConsItemList::cons( consItem );
		}
		else if ( accumTopEl.AccumElList() != 0 ) {
			list = walkAccumElList( accumTopEl.AccumElList() );
		}
		return list;
	}

	ConsItemList *walkAccumList( accum_list accumList )
	{
		ConsItemList *list = walkAccumTopEl( accumList.AccumTopEl() );

		if ( accumList.AccumList() != 0 ) {
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

	LangExpr *walkCodeFactor( code_factor codeFactor )
	{
		LangExpr *expr = 0;
		if ( codeFactor.VarRef() != 0 ) {
			var_ref varRef = codeFactor.VarRef();
			LangVarRef *langVarRef = walkVarRef( varRef );

			LangTerm *term = 0;
			if ( codeFactor.CodeExprList() == 0 ) {
				term = LangTerm::cons( langVarRef->loc, LangTerm::VarRefType, langVarRef );
			}
			else {
				ExprVect *exprVect = walkCodeExprList( codeFactor.CodeExprList() );
				term = LangTerm::cons( langVarRef->loc, langVarRef, exprVect );
			}

			expr = LangExpr::cons( term );
		}
		else if ( codeFactor.Number() != 0 ) {
			String number = codeFactor.Number().text().c_str();
			LangTerm *term = LangTerm::cons( codeFactor.Number().loc(),
					LangTerm::NumberType, number );
			expr = LangExpr::cons( term );
		}
		else if ( codeFactor.Lit() != 0 ) {
			String lit = codeFactor.Lit().text().c_str();
			LangTerm *term = LangTerm::cons( codeFactor.Lit().loc(), LangTerm::StringType, lit );
			expr = LangExpr::cons( term );
		}
		else if ( codeFactor.Parse() != 0 ) {
			/* The type we are parsing. */
			type_ref typeRefTree = codeFactor.TypeRef();
			TypeRef *typeRef = walkTypeRef( typeRefTree );
			ObjectField *objField = walkOptCapture( codeFactor.OptCapture() );
			FieldInitVect *init = walkOptFieldInit( codeFactor.OptFieldInit() );
			ConsItemList *list = walkAccumulate( codeFactor.Accumulate() );

			expr = parseCmd( codeFactor.Parse().loc(), false, objField, typeRef, init, list );
		}
		else if ( codeFactor.ParseStop() != 0 ) {
			/* The type we are parsing. */
			type_ref typeRefTree = codeFactor.TypeRef();
			TypeRef *typeRef = walkTypeRef( typeRefTree );
			ObjectField *objField = walkOptCapture( codeFactor.OptCapture() );
			FieldInitVect *init = walkOptFieldInit( codeFactor.OptFieldInit() );
			ConsItemList *list = walkAccumulate( codeFactor.Accumulate() );

			expr = parseCmd( codeFactor.ParseStop().loc(), true, objField, typeRef, init, list );
		}
		else if ( codeFactor.Cons() != 0 ) {
			/* The type we are parsing. */
			type_ref typeRefTree = codeFactor.TypeRef();
			TypeRef *typeRef = walkTypeRef( typeRefTree );
			ObjectField *objField = walkOptCapture( codeFactor.OptCapture() );
			ConsItemList *list = walkConstructor( codeFactor.Constructor() );
			FieldInitVect *init = walkOptFieldInit( codeFactor.OptFieldInit() );

			expr = construct( codeFactor.Cons().loc(), objField, list, typeRef, init );
		}
		else if ( codeFactor.Send() != 0 ) {
			LangVarRef *varRef = walkVarRef( codeFactor.ToVarRef() );
			ConsItemList *list = walkAccumulate( codeFactor.Accumulate() );
			bool eof = walkOptEos( codeFactor.OptEos() );
			expr = send( codeFactor.Send().loc(), varRef, list, eof );
		}
		else if ( codeFactor.Nil() != 0 ) {
			expr = LangExpr::cons( LangTerm::cons( codeFactor.Nil().loc(),
					LangTerm::NilType ) );
		}
		else if ( codeFactor.True() != 0 ) {
			expr = LangExpr::cons( LangTerm::cons( codeFactor.True().loc(),
					LangTerm::TrueType ) );
		}
		else if ( codeFactor.False() != 0 ) {
			expr = LangExpr::cons( LangTerm::cons( codeFactor.False().loc(),
					LangTerm::FalseType ) );
		}
		else if ( codeFactor.ParenCodeExpr() != 0 ) {
			expr = walkCodeExpr( codeFactor.ParenCodeExpr() );
		}
		else if ( codeFactor.String() != 0 ) {
			ConsItemList *list = walkString( codeFactor.String() );
			expr = LangExpr::cons( LangTerm::cons( codeFactor.String().loc(), list ) );
		}
		else if ( codeFactor.MatchVarRef() != 0 ) {
			LangVarRef *varRef = walkVarRef( codeFactor.MatchVarRef() );
			PatternItemList *list = walkPattern( codeFactor.Pattern() );
			expr = match( codeFactor.loc(), varRef, list );
		}
		else if ( codeFactor.InVarRef() != 0 ) {
			TypeRef *typeRef = walkTypeRef( codeFactor.TypeRef() );
			LangVarRef *varRef = walkVarRef( codeFactor.InVarRef() );
			expr = LangExpr::cons( LangTerm::cons( typeRef->loc,
					LangTerm::SearchType, typeRef, varRef ) );
		}
		else if ( codeFactor.MakeTreeExprList() != 0 ) {
			ExprVect *exprList = walkCodeExprList( codeFactor.MakeTreeExprList() );
			expr = LangExpr::cons( LangTerm::cons( codeFactor.loc(),
					LangTerm::MakeTreeType, exprList ) );
		}
		else if ( codeFactor.MakeTokenExprList() != 0 ) {
			ExprVect *exprList = walkCodeExprList( codeFactor.MakeTokenExprList() );
			expr = LangExpr::cons( LangTerm::cons( codeFactor.loc(),
					LangTerm::MakeTokenType, exprList ) );
		}
		else if ( codeFactor.TypeIdTypeRef() != 0 ) {
			TypeRef *typeRef = walkTypeRef( codeFactor.TypeIdTypeRef() );
			expr = LangExpr::cons( LangTerm::cons( codeFactor.loc(),
					LangTerm::TypeIdType, typeRef ) );
		}
		else if ( codeFactor.NewCodeFactor() != 0 ) {
			LangExpr *newExpr = walkCodeFactor( codeFactor.NewCodeFactor() );
			expr = LangExpr::cons( LangTerm::cons( codeFactor.loc(),
					LangTerm::NewType, newExpr ) );
		}
		return expr;
	}

	LangExpr *walkCodeAdditive( code_additive additive )
	{
		LangExpr *expr = 0;
		if ( additive.Plus() != 0 ) {
			LangExpr *left = walkCodeAdditive( additive.Additive() );
			LangExpr *right = walkCodeMultiplicitive( additive.Multiplicitive() );
			expr = LangExpr::cons( additive.Plus().loc(), left, '+', right );
		}
		else if ( additive.Minus() != 0 ) {
			LangExpr *left = walkCodeAdditive( additive.Additive() );
			LangExpr *right = walkCodeMultiplicitive( additive.Multiplicitive() );
			expr = LangExpr::cons( additive.Minus().loc(), left, '-', right );
		}
		else {
			expr = walkCodeMultiplicitive( additive.Multiplicitive() );
		}
		return expr;
	}


	LangExpr *walkCodeUnary( code_unary unary )
	{
		LangExpr *expr = walkCodeFactor( unary.Factor() );

		if ( unary.Bang() != 0 ) {
			expr = LangExpr::cons( unary.Bang().loc(), '!', expr );
		}
		else if ( unary.Dollar() != 0 ) {
			expr = LangExpr::cons( unary.Dollar().loc(), '$', expr );
		}
		else if ( unary.Caret() != 0 ) {
			expr = LangExpr::cons( unary.Caret().loc(), '^', expr );
		}
		else if ( unary.Percent() != 0 ) {
			expr = LangExpr::cons( unary.Percent().loc(), '%', expr );
		}

		return expr;
	}


	LangExpr *walkCodeRelational( code_relational codeRelational )
	{
		LangExpr *expr = 0;
		if ( codeRelational.Relational() != 0 ) {
			LangExpr *left = walkCodeRelational( codeRelational.Relational() );
			LangExpr *right = walkCodeAdditive( codeRelational.Additive() );

			if ( codeRelational.EqEq() != 0 ) {
				expr = LangExpr::cons( codeRelational.EqEq().loc(), left, OP_DoubleEql, right );
			}
			if ( codeRelational.Neq() != 0 ) {
				expr = LangExpr::cons( codeRelational.Neq().loc(), left, OP_NotEql, right );
			}
			else if ( codeRelational.Lt() != 0 ) {
				expr = LangExpr::cons( codeRelational.Lt().loc(), left, '<', right );
			}
			else if ( codeRelational.Gt() != 0 ) {
				expr = LangExpr::cons( codeRelational.Gt().loc(), left, '>', right );
			}
			else if ( codeRelational.LtEq() != 0 ) {
				expr = LangExpr::cons( codeRelational.LtEq().loc(), left, OP_LessEql, right );
			}
			else if ( codeRelational.GtEq() != 0 ) {
				expr = LangExpr::cons( codeRelational.GtEq().loc(), left, OP_GrtrEql, right );
			}
		}
		else {
			expr = walkCodeAdditive( codeRelational.Additive() );
		}
		return expr;
	}

	LangStmt *walkExprStmt( expr_stmt &exprStmt )
	{
		LangStmt *stmt;
		if ( exprStmt.CodeExpr() != 0 ) {
			code_expr codeExpr = exprStmt.CodeExpr();
			LangExpr *expr = walkCodeExpr( codeExpr );
			stmt = LangStmt::cons( expr->loc, LangStmt::ExprType, expr );
		}
		return stmt;
	}

	ObjectField *walkVarDef( var_def varDef )
	{
		String id = varDef.Id().text().c_str();
		TypeRef *typeRef = walkTypeRef( varDef.TypeRef() );
		return ObjectField::cons( varDef.Id().loc(), typeRef, id );
	}

	LangTerm *walkIterCall( iter_call IterCall )
	{
		LangTerm *langTerm = 0;
		if ( IterCall.Id() != 0 ) {
			String tree = IterCall.Id().text().c_str();
			langTerm = LangTerm::cons( IterCall.Id().loc(),
					LangTerm::VarRefType, LangVarRef::cons( IterCall.Id().loc(), tree ) );
		}
		else {
			LangVarRef *varRef = walkVarRef( IterCall.VarRef() );
			ExprVect *exprVect = walkCodeExprList( IterCall.CodeExprList() );
			langTerm = LangTerm::cons( varRef->loc, varRef, exprVect );
		}
		
		return langTerm;
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
		if ( optionalElse.BlockOrSingle() != 0 ) {
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
		if ( elsifList.ElsifList() != 0 ) {
			stmt = walkElsifClause( elsifList.ElsifClause() );
			stmt->elsePart = walkElsifList( elsifList.ElsifList() );
		}
		else {
			stmt = walkOptionalElse( elsifList.OptionalElse() );
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
		String id = paramVarDef.Id().text().c_str();
		TypeRef *typeRef = 0;

		if ( paramVarDef.TypeRef() != 0 )
			typeRef = walkTypeRef( paramVarDef.TypeRef() );
		else
			typeRef = walkReferenceTypeRef( paramVarDef.RefTypeRef() );
		
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
		return OptExport.Export() != 0;
	}

	void walkFunctionDef( function_def FunctionDef )
	{
		ObjectDef *localFrame = blockOpen();

		bool exprt = walkOptExport( FunctionDef.OptExport() );
		TypeRef *typeRef = walkTypeRef( FunctionDef.TypeRef() );
		String id = FunctionDef.Id().text().c_str();
		ParameterList *paramList = walkParamVarDefList( FunctionDef.ParamVarDefList() );
		StmtList *stmtList = walkLangStmtList( FunctionDef.LangStmtList() );
		functionDef( stmtList, localFrame, paramList, typeRef, id, exprt );

		blockClose();
	}

	void walkIterDef( iter_def IterDef )
	{
		ObjectDef *localFrame = blockOpen();

		String id = IterDef.Id().text().c_str();
		ParameterList *paramList = walkParamVarDefList( IterDef.ParamVarDefList() );
		StmtList *stmtList = walkLangStmtList( IterDef.LangStmtList() );
		iterDef( stmtList, localFrame, paramList, id );

		blockClose();
	}

	void walkContextItem( context_item contextItem )
	{
		switch ( contextItem.prodType() ) {
		case context_item::Rl:
			walkRlDef( contextItem.RlDef() );
			break;
		case context_item::ContextVar:
			walkContextVarDef( contextItem.ContextVarDef() );
			break;
		case context_item::Token:
			walkTokenDef( contextItem.TokenDef() );
			break;
		case context_item::Ignore:
			walkIgnoreDef( contextItem.IgnoreDef() );
			break;
		case context_item::Literal:
			walkLiteralDef( contextItem.LiteralDef() );
			break;
		case context_item::Cfl:
			walkCflDef( contextItem.CflDef() );
			break;
		case context_item::Region:
			walkLexRegion( contextItem.RegionDef() );
			break;
		case context_item::Context:
			walkContextDef( contextItem.ContextDef() );
			break;
		case context_item::Function:
			walkFunctionDef( contextItem.FunctionDef() );
			break;
		case context_item::Iter:
			walkIterDef( contextItem.IterDef() );
			break;
		case context_item::PreEof:
			walkPreEof( contextItem.PreEofDef() );
			break;
		case context_item::Export:
			walkExportDef( contextItem.ExportDef() );
			break;
		case context_item::Precedence:
			walkPrecedenceDef( contextItem.PrecedenceDef() );
			break;
		}
	}

	void walkContextDef( context_def contextDef )
	{
		String name = contextDef.Name().text().c_str();
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
		String name = NamespaceDef.Name().text().c_str();
		createNamespace( NamespaceDef.Name().loc(), name );
		walkRootItemList( NamespaceDef.RootItemList() );
		namespaceStack.pop();
	}

	void walkRootItem( root_item &rootItem, StmtList *stmtList )
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
		else if ( rootItem.PreEofDef() != 0 ) {
			walkPreEof( rootItem.PreEofDef() );
		}
		else if ( rootItem.ExportDef() != 0 ) {
			LangStmt *stmt = walkExportDef( rootItem.ExportDef() );
			if ( stmt != 0 )
				stmtList->append( stmt );
		}
		else if ( rootItem.AliasDef() != 0 ) {
			walkAliasDef( rootItem.AliasDef() );
		}
		else if ( rootItem.PrecedenceDef() != 0 ) {
			walkPrecedenceDef( rootItem.PrecedenceDef() );
		}
		else if ( rootItem.Include() != 0 ) {
			StmtList *includeList = walkInclude( rootItem.Include() );
			stmtList->append( *includeList );
		}
		else if ( rootItem.GlobalDef() != 0 ) {
			LangStmt *stmt = walkGlobalDef( rootItem.GlobalDef() );
			if ( stmt != 0 )
				stmtList->append( stmt );
		}
	}

	bool walkOptNoIgnore( opt_no_ignore OptNoIngore )
	{
		return OptNoIngore.Ni() != 0;
	}

	bool walkOptEos( opt_eos OptEos )
	{
		return OptEos.Eos() != 0;
	}

	void walkLiteralItem( literal_item literalItem )
	{
		bool niLeft = walkOptNoIgnore( literalItem.NiLeft() );
		bool niRight = walkOptNoIgnore( literalItem.NiRight() );

		String lit = literalItem.Lit().text().c_str();
		if ( strcmp( lit, "''" ) == 0 )
			zeroDef( literalItem.Lit().loc(), lit, niLeft, niRight );
		else
			literalDef( literalItem.Lit().loc(), lit, niLeft, niRight );
	}

	void walkLiteralList( literal_list literalList )
	{
		if ( literalList.LiteralList() != 0 )
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
			root_item rootItem = rootItemList.value();
			walkRootItem( rootItem, stmtList );
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
