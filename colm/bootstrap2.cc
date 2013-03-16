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
#include "lmparse.h"
#include "global.h"
#include "input.h"
#include "bootstrap2.h"
#include "exports2.h"
#include "colm/colm.h"

extern RuntimeData main_runtimeData;

void Bootstrap2::walkProdElList( ProdElList *list, prod_el_list &ProdElList )
{
	if ( ProdElList.ProdElList() != 0 ) {
		prod_el_list RightProdElList = ProdElList.ProdElList();
		walkProdElList( list, RightProdElList );
	}
	
	if ( ProdElList.ProdEl() != 0 ) {
		prod_el El = ProdElList.ProdEl();
		String typeName = El.Id().text().c_str();

		ObjectField *captureField = 0;
		if ( El.OptName().Name() != 0 ) {
			String fieldName = El.OptName().Name().text().c_str();
			captureField = ObjectField::cons( internal, 0, fieldName );
		}

		RepeatType repeatType = RepeatNone;
		if ( El.OptRepeat().Star() != 0 )
			repeatType = RepeatRepeat;

		ProdEl *prodEl = prodElName( internal, typeName,
				NamespaceQual::cons(namespaceStack.top()),
				captureField, repeatType, false );

		appendProdEl( list, prodEl );
	}
}

void Bootstrap2::walkProdList( LelDefList *lelDefList, prod_list &ProdList )
{
	if ( ProdList.ProdList() != 0 ) {
		prod_list RightProdList = ProdList.ProdList();
		walkProdList( lelDefList, RightProdList );
	}

	ProdElList *list = new ProdElList;
	
	prod_el_list ProdElList = ProdList.Prod().ProdElList();
	walkProdElList( list, ProdElList );

	Production *prod = BaseParser::production( internal, list, false, 0, 0 );
	prodAppend( lelDefList, prod );
}

LexFactor *Bootstrap2::walkLexFactor( lex_factor &LexFactorTree )
{
	if ( LexFactorTree.Literal() != 0 ) {
		String litString = LexFactorTree.Literal().text().c_str();
		Literal *literal = Literal::cons( internal, litString, Literal::LitString );
		LexFactor *factor = LexFactor::cons( literal );
		return factor;
	}
	else if ( LexFactorTree.Expr() != 0 ) {
		lex_expr LexExpr = LexFactorTree.Expr();
		LexExpression *expr = walkLexExpr( LexExpr );
		LexJoin *join = LexJoin::cons( expr );
		LexFactor *factor = LexFactor::cons( join );
		return factor;
	}
	else {
		String low = LexFactorTree.Low().text().c_str();
		Literal *lowLit = Literal::cons( internal, low, Literal::LitString );

		String high = LexFactorTree.High().text().c_str();
		Literal *highLit = Literal::cons( internal, high, Literal::LitString );

		Range *range = Range::cons( lowLit, highLit );
		LexFactor *factor = LexFactor::cons( range );
		return factor;
	}
}

LexFactorNeg *Bootstrap2::walkLexFactorNeg( lex_factor_neg &LexFactorNegTree )
{
	if ( LexFactorNegTree.FactorNeg() != 0 ) {
		lex_factor_neg Rec = LexFactorNegTree.FactorNeg();
		LexFactorNeg *recNeg = walkLexFactorNeg( Rec );
		LexFactorNeg *factorNeg = LexFactorNeg::cons( internal, recNeg, LexFactorNeg::CharNegateType );
		return factorNeg;
	}
	else {
		lex_factor LexFactorTree = LexFactorNegTree.Factor();
		LexFactor *factor = walkLexFactor( LexFactorTree );
		LexFactorNeg *factorNeg = LexFactorNeg::cons( internal, factor );
		return factorNeg;
	}
}

LexFactorRep *Bootstrap2::walkLexFactorRep( lex_factor_rep &LexFactorRepTree )
{
	if ( LexFactorRepTree.FactorRep() != 0 ) {
		lex_factor_rep Rec = LexFactorRepTree.FactorRep();
		LexFactorRep *recRep = walkLexFactorRep( Rec );
		LexFactorRep *factorRep = LexFactorRep::cons( internal, recRep, 0, 0, LexFactorRep::StarType );
		return factorRep;
	}
	else {
		lex_factor_neg LexFactorNegTree = LexFactorRepTree.FactorNeg();
		LexFactorNeg *factorNeg = walkLexFactorNeg( LexFactorNegTree );
		LexFactorRep *factorRep = LexFactorRep::cons( internal, factorNeg );
		return factorRep;
	}
}

LexFactorAug *Bootstrap2::walkLexFactorAug( lex_factor_rep &LexFactorRepTree )
{
	LexFactorRep *factorRep = walkLexFactorRep( LexFactorRepTree );
	return LexFactorAug::cons( factorRep );
}

LexTerm *Bootstrap2::walkLexTerm( lex_term &LexTermTree )
{
	if ( LexTermTree.Term() != 0 ) {
		lex_term Rec = LexTermTree.Term();
		LexTerm *leftTerm = walkLexTerm( Rec );

		lex_factor_rep LexFactorRepTree = LexTermTree.FactorRep();
		LexFactorAug *factorAug = walkLexFactorAug( LexFactorRepTree );
		LexTerm *term = LexTerm::cons( leftTerm, factorAug, LexTerm::ConcatType );
		return term;
	}
	else {
		lex_factor_rep LexFactorRepTree = LexTermTree.FactorRep();
		LexFactorAug *factorAug = walkLexFactorAug( LexFactorRepTree );
		LexTerm *term = LexTerm::cons( factorAug );
		return term;
	}
}

LexExpression *Bootstrap2::walkLexExpr( lex_expr &LexExprTree )
{
	if ( LexExprTree.Expr() != 0 ) {
		lex_expr Rec = LexExprTree.Expr();
		LexExpression *leftExpr = walkLexExpr( Rec );

		lex_term LexTermTree = LexExprTree.Term();
		LexTerm *term = walkLexTerm( LexTermTree );
		LexExpression *expr = LexExpression::cons( leftExpr, term, LexExpression::OrType );

		return expr;
	}
	else {
		lex_term LexTermTree = LexExprTree.Term();
		LexTerm *term = walkLexTerm( LexTermTree );
		LexExpression *expr = LexExpression::cons( term );
		return expr;
	}
}

void Bootstrap2::walkTokenList( token_list &TokenList )
{
	if ( TokenList.TokenList() != 0 ) {
		token_list RightTokenList = TokenList.TokenList();
		walkTokenList( RightTokenList );
	}
	
	if ( TokenList.TokenDef() != 0 ) {
		token_def TokenDef = TokenList.TokenDef();
		String name = TokenDef.Id().text().c_str();

		ObjectDef *objectDef = ObjectDef::cons( ObjectDef::UserType, name, pd->nextObjectId++ ); 

		lex_expr LexExpr = TokenDef.Expr();
		LexExpression *expr = walkLexExpr( LexExpr );
		LexJoin *join = LexJoin::cons( expr );

		defineToken( internal, name, join, objectDef, 0, false, false, false );
	}

	if ( TokenList.IgnoreDef() != 0 ) {
		ignore_def IgnoreDef = TokenList.IgnoreDef();

		ObjectDef *objectDef = ObjectDef::cons( ObjectDef::UserType, 0, pd->nextObjectId++ ); 

		lex_expr LexExpr = IgnoreDef.Expr();
		LexExpression *expr = walkLexExpr( LexExpr );
		LexJoin *join = LexJoin::cons( expr );

		defineToken( internal, 0, join, objectDef, 0, true, false, false );
	}
}

void Bootstrap2::walkLexRegion( region_def &regionDef )
{
	pushRegionSet( internal );

	token_list TokenList = regionDef.TokenList();
	walkTokenList( TokenList );

	popRegionSet();
}

void Bootstrap2::walkCflDef( cfl_def &cflDef )
{
	prod_list prodList = cflDef.ProdList();

	LelDefList *defList = new LelDefList;
	walkProdList( defList, prodList );

	String name = cflDef.DefId().text().c_str();
	NtDef *ntDef = NtDef::cons( name, namespaceStack.top(), contextStack.top(), false );
	ObjectDef *objectDef = ObjectDef::cons( ObjectDef::UserType, name, pd->nextObjectId++ ); 
	BaseParser::cflDef( ntDef, objectDef, defList );
}

ExprVect *Bootstrap2::walkCodeExprList( _repeat_code_expr &codeExprList )
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

LangStmt *Bootstrap2::walkPrintStmt( print_stmt &printStmt )
{
	std::cerr << "print statement: " << printStmt.text() << std::endl;

	_repeat_code_expr codeExprList = printStmt.CodeExprList();
	ExprVect *exprVect = walkCodeExprList( codeExprList );
	return LangStmt::cons( internal, LangStmt::PrintType, exprVect );
}

LangVarRef *Bootstrap2::walkVarRef( var_ref &varRef )
{
	QualItemVect *qual = new QualItemVect;
	String id = varRef.Id().text().c_str();
	LangVarRef *langVarRef = LangVarRef::cons( internal, qual, id );
	return langVarRef;
}

LangExpr *Bootstrap2::walkCodeExpr( code_expr &codeExpr )
{
	LangExpr *expr = 0;
	if ( codeExpr.VarRef() != 0 ) {
		var_ref varRef = codeExpr.VarRef();
		LangVarRef *langVarRef = walkVarRef( varRef );
		LangTerm *term = LangTerm::cons( internal, LangTerm::VarRefType, langVarRef );
		expr = LangExpr::cons( term );
	}
	else if ( codeExpr.Lit() != 0 ) {
		String lit = codeExpr.Lit().text().c_str();
		LangTerm *term = LangTerm::cons( internal, LangTerm::StringType, lit );
		expr = LangExpr::cons( term );
	}
	return expr;
}

LangStmt *Bootstrap2::walkExprStmt( expr_stmt &exprStmt )
{
	LangStmt *stmt;
	if ( exprStmt.CodeExpr() != 0 ) {
		code_expr codeExpr = exprStmt.CodeExpr();
		LangExpr *expr = walkCodeExpr( codeExpr );
		stmt = LangStmt::cons( internal, LangStmt::ExprType, expr );
	}
	return stmt;
}

LangStmt *Bootstrap2::walkStatement( statement &Statement )
{
	LangStmt *stmt;
	if ( Statement.Print() != 0 ) {
		print_stmt printStmt = Statement.Print();
		stmt = walkPrintStmt( printStmt );
	}
	else if ( Statement.Expr() != 0 ) {
		expr_stmt exprStmt = Statement.Expr();
		stmt = walkExprStmt( exprStmt );
	}
	return stmt;
}

void Bootstrap2::go()
{
	StmtList *stmtList = new StmtList;

	colmInit( 0 );
	ColmProgram *program = colmNewProgram( &main_runtimeData );
	colmRunProgram( program, 0, 0 );

	/* Extract the parse tree. */
	start Start = ColmTree( program );

	if ( Start == 0 ) {
		std::cerr << "error parsing input" << std::endl;
		return;
	}

	/* Walk the list of items. */
	_repeat_root_item rootItemList = Start.RootItemList();
	while ( !rootItemList.end() ) {

		root_item rootItem = rootItemList.value();

		if ( rootItem.CflDef() != 0 ) {
			cfl_def cflDef = rootItem.CflDef();
			walkCflDef( cflDef );
		}
		else if ( rootItem.RegionDef() != 0 ) {
			region_def regionDef = rootItem.RegionDef();
			walkLexRegion( regionDef );
		}
		else if ( rootItem.Statement() != 0 ) {
			statement Statement = rootItem.Statement();
			LangStmt *stmt = walkStatement( Statement );
			stmtList->append( stmt );
		}

		rootItemList = rootItemList.next();
	}

	colmDeleteProgram( program );

	pd->rootCodeBlock = CodeBlock::cons( stmtList, 0 );
}

