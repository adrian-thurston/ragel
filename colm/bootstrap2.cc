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

void Bootstrap2::prodElList( ProdElList *list, prod_el_list &ProdElList )
{
	if ( ProdElList.ProdElList() != 0 ) {
		prod_el_list RightProdElList = ProdElList.ProdElList();
		prodElList( list, RightProdElList );
	}
	
	if ( ProdElList.ProdEl() != 0 ) {
		prod_el El = ProdElList.ProdEl();
		String typeName = El.Id().text().c_str();

		ObjectField *captureField = 0;
		if ( El.OptName().Name() != 0 ) {
			String fieldName = El.OptName().Name().text().c_str();
			std::cout << "field name: " << fieldName << std::endl;
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

void Bootstrap2::prodList( LelDefList *lelDefList, prod_list &ProdList )
{
	if ( ProdList.ProdList() != 0 ) {
		prod_list RightProdList = ProdList.ProdList();
		prodList( lelDefList, RightProdList );
	}

	ProdElList *list = new ProdElList;
	
	prod_el_list ProdElList = ProdList.Prod().ProdElList();
	prodElList( list, ProdElList );

	Production *prod = BaseParser::production( internal, list, false, 0, 0 );
	prodAppend( lelDefList, prod );
}

LexTerm *litTerm( const char *str )
{
	Literal *lit = Literal::cons( internal, String( str ), Literal::LitString );
	LexFactor *factor = LexFactor::cons( lit );
	LexFactorNeg *factorNeg = LexFactorNeg::cons( internal, factor );
	LexFactorRep *factorRep = LexFactorRep::cons( internal, factorNeg );
	LexFactorAug *factorAug = LexFactorAug::cons( factorRep );
	LexTerm *term = LexTerm::cons( factorAug );
	return term;
}

LexFactor *Bootstrap2::lexFactor( lex_factor &LexFactorTree )
{
	if ( LexFactorTree.Literal() != 0 ) {
		String litString = LexFactorTree.Literal().text().c_str();
		Literal *literal = Literal::cons( internal, litString, Literal::LitString );
		LexFactor *factor = LexFactor::cons( literal );
		return factor;
	}
	else if ( LexFactorTree.Expr() != 0 ) {
		lex_expr LexExpr = LexFactorTree.Expr();
		LexExpression *expr = lexExpr( LexExpr );
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

LexFactorNeg *Bootstrap2::lexFactorNeg( lex_factor_neg &LexFactorNegTree )
{
	if ( LexFactorNegTree.FactorNeg() != 0 ) {
		lex_factor_neg Rec = LexFactorNegTree.FactorNeg();
		LexFactorNeg *recNeg = lexFactorNeg( Rec );
		LexFactorNeg *factorNeg = LexFactorNeg::cons( internal, recNeg, LexFactorNeg::CharNegateType );
		return factorNeg;
	}
	else {
		lex_factor LexFactorTree = LexFactorNegTree.Factor();
		LexFactor *factor = lexFactor( LexFactorTree );
		LexFactorNeg *factorNeg = LexFactorNeg::cons( internal, factor );
		return factorNeg;
	}
}

LexFactorRep *Bootstrap2::lexFactorRep( lex_factor_rep &LexFactorRepTree )
{
	if ( LexFactorRepTree.FactorRep() != 0 ) {
		lex_factor_rep Rec = LexFactorRepTree.FactorRep();
		LexFactorRep *recRep = lexFactorRep( Rec );
		LexFactorRep *factorRep = LexFactorRep::cons( internal, recRep, 0, 0, LexFactorRep::StarType );
		return factorRep;
	}
	else {
		lex_factor_neg LexFactorNegTree = LexFactorRepTree.FactorNeg();
		LexFactorNeg *factorNeg = lexFactorNeg( LexFactorNegTree );
		LexFactorRep *factorRep = LexFactorRep::cons( internal, factorNeg );
		return factorRep;
	}
}

LexFactorAug *Bootstrap2::lexFactorAug( lex_factor_rep &LexFactorRepTree )
{
	LexFactorRep *factorRep = lexFactorRep( LexFactorRepTree );
	return LexFactorAug::cons( factorRep );
}

LexTerm *Bootstrap2::lexTerm( lex_term &LexTermTree )
{
	if ( LexTermTree.Term() != 0 ) {
		lex_term Rec = LexTermTree.Term();
		LexTerm *leftTerm = lexTerm( Rec );

		lex_factor_rep LexFactorRepTree = LexTermTree.FactorRep();
		LexFactorAug *factorAug = lexFactorAug( LexFactorRepTree );
		LexTerm *term = LexTerm::cons( leftTerm, factorAug, LexTerm::ConcatType );
		return term;
	}
	else {
		lex_factor_rep LexFactorRepTree = LexTermTree.FactorRep();
		LexFactorAug *factorAug = lexFactorAug( LexFactorRepTree );
		LexTerm *term = LexTerm::cons( factorAug );
		return term;
	}
}

LexExpression *Bootstrap2::lexExpr( lex_expr &LexExprTree )
{
	if ( LexExprTree.Expr() != 0 ) {
		lex_expr Rec = LexExprTree.Expr();
		LexExpression *leftExpr = lexExpr( Rec );

		lex_term LexTermTree = LexExprTree.Term();
		LexTerm *term = lexTerm( LexTermTree );
		LexExpression *expr = LexExpression::cons( leftExpr, term, LexExpression::OrType );

		return expr;
	}
	else {
		lex_term LexTermTree = LexExprTree.Term();
		LexTerm *term = lexTerm( LexTermTree );
		LexExpression *expr = LexExpression::cons( term );
		return expr;
	}
}

void Bootstrap2::tokenList( token_list &TokenList )
{
	if ( TokenList.TokenList() != 0 ) {
		token_list RightTokenList = TokenList.TokenList();
		tokenList( RightTokenList );
	}
	
	if ( TokenList.TokenDef() != 0 ) {
		token_def TokenDef = TokenList.TokenDef();
		String name = TokenDef.Id().text().c_str();

		ObjectDef *objectDef = ObjectDef::cons( ObjectDef::UserType, name, pd->nextObjectId++ ); 

		lex_expr LexExpr = TokenDef.Expr();
		LexExpression *expr = lexExpr( LexExpr );
		LexJoin *join = LexJoin::cons( expr );

		tokenDef( internal, name, join, objectDef, 0, false, false, false );
	}

	if ( TokenList.IgnoreDef() != 0 ) {
		ignore_def IgnoreDef = TokenList.IgnoreDef();

		ObjectDef *objectDef = ObjectDef::cons( ObjectDef::UserType, 0, pd->nextObjectId++ ); 

		lex_expr LexExpr = IgnoreDef.Expr();
		LexExpression *expr = lexExpr( LexExpr );
		LexJoin *join = LexJoin::cons( expr );

		tokenDef( internal, 0, join, objectDef, 0, true, false, false );
	}
}

void Bootstrap2::lexRegion( item &LexRegion )
{
	pushRegionSet( internal );

	token_list TokenList = LexRegion.TokenList();
	tokenList( TokenList );

	popRegionSet();
}

void Bootstrap2::defineProd( item &Define )
{
	prod_list ProdList = Define.ProdList();

	LelDefList *defList = new LelDefList;
	prodList( defList, ProdList );

	String name = Define.DefId().text().c_str();
	NtDef *ntDef = NtDef::cons( name, namespaceStack.top(), contextStack.top(), false );
	ObjectDef *objectDef = ObjectDef::cons( ObjectDef::UserType, name, pd->nextObjectId++ ); 
	cflDef( ntDef, objectDef, defList );
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
	_repeat_item ItemList = Start.ItemList();
	while ( !ItemList.end() ) {

		item Item = ItemList.value();
		if ( Item.DefId() != 0 )
			defineProd( Item );
		else if ( Item.TokenList() != 0 )
			lexRegion( Item );
		ItemList = ItemList.next();
	}

	colmDeleteProgram( program );

	parseInput( stmtList );
	exportTree( stmtList );

	pd->rootCodeBlock = CodeBlock::cons( stmtList, 0 );
}

