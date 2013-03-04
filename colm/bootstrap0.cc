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
#include "bootstrap0.h"

using std::cout;
using std::cerr;
using std::endl;

LexTerm *rangeTerm( const char *low, const char *high )
{
	Literal *lowLit = Literal::cons( internal, String( low ), Literal::LitString );
	Literal *highLit = Literal::cons( internal, String( high ), Literal::LitString );
	Range *range = Range::cons( lowLit, highLit );
	LexFactor *factor = LexFactor::cons( range );
	LexFactorNeg *factorNeg = LexFactorNeg::cons( internal, factor );
	LexFactorRep *factorRep = LexFactorRep::cons( internal, factorNeg );
	LexFactorAug *factorAug = LexFactorAug::cons( factorRep );
	LexTerm *term = LexTerm::cons( factorAug );
	return term;
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

LexExpression *orExpr( LexTerm *term1, LexTerm *term2 )
{
	LexExpression *expr1 = LexExpression::cons( term1 );
	return LexExpression::cons( expr1, term2, LexExpression::OrType );
}

LexExpression *orExpr( LexTerm *term1, LexTerm *term2, LexTerm *term3 )
{
	LexExpression *expr1 = LexExpression::cons( term1 );
	LexExpression *expr2 = LexExpression::cons( expr1, term2, LexExpression::OrType );
	LexExpression *expr3 = LexExpression::cons( expr2, term3, LexExpression::OrType );
	return expr3;
}

LexExpression *orExpr( LexTerm *term1, LexTerm *term2, LexTerm *term3, LexTerm *term4 )
{
	LexExpression *expr1 = LexExpression::cons( term1 );
	LexExpression *expr2 = LexExpression::cons( expr1, term2, LexExpression::OrType );
	LexExpression *expr3 = LexExpression::cons( expr2, term3, LexExpression::OrType );
	LexExpression *expr4 = LexExpression::cons( expr3, term4, LexExpression::OrType );
	return expr4;
}

LexExpression *orExpr( LexTerm *term1, LexTerm *term2, LexTerm *term3,
		LexTerm *term4, LexTerm *term5, LexTerm *term6 )
{
	LexExpression *expr1 = LexExpression::cons( term1 );
	LexExpression *expr2 = LexExpression::cons( expr1, term2, LexExpression::OrType );
	LexExpression *expr3 = LexExpression::cons( expr2, term3, LexExpression::OrType );
	LexExpression *expr4 = LexExpression::cons( expr3, term4, LexExpression::OrType );
	return expr4;
}

LexFactorAug *starFactorAug( LexExpression *expr )
{
	LexJoin *join = LexJoin::cons( expr );
	LexFactor *factor = LexFactor::cons( join );
	LexFactorNeg *factorNeg = LexFactorNeg::cons( internal, factor );
	LexFactorRep *factorRep = LexFactorRep::cons( internal, factorNeg );
	LexFactorRep *staredRep = LexFactorRep::cons( internal, factorRep, 0, 0, LexFactorRep::StarType );
	LexFactorAug *factorAug = LexFactorAug::cons( staredRep );
	return factorAug;
}

LexFactorAug *plusFactorAug( LexExpression *expr )
{
	LexJoin *join = LexJoin::cons( expr );
	LexFactor *factor = LexFactor::cons( join );
	LexFactorNeg *factorNeg = LexFactorNeg::cons( internal, factor );
	LexFactorRep *factorRep = LexFactorRep::cons( internal, factorNeg );
	LexFactorRep *staredRep = LexFactorRep::cons( internal, factorRep, 0, 0, LexFactorRep::PlusType );
	LexFactorAug *factorAug = LexFactorAug::cons( staredRep );
	return factorAug;
}

LexTerm *concatTerm( LexFactorAug *fa1, LexFactorAug *fa2 )
{
	LexTerm *term1 = LexTerm::cons( fa1 );
	LexTerm *term2 = LexTerm::cons( term1, fa2, LexTerm::ConcatType );
	return term2;
}

LexFactorAug *parensFactorAug( LexExpression *expr )
{
	LexJoin *join = LexJoin::cons( expr );
	LexFactor *factor = LexFactor::cons( join );
	LexFactorNeg *factorNeg = LexFactorNeg::cons( internal, factor );
	LexFactorRep *factorRep = LexFactorRep::cons( internal, factorNeg );
	LexFactorAug *factorAug = LexFactorAug::cons( factorRep );
	return factorAug;
}

LexFactorAug *parensFactorAug( LexTerm *term )
{
	LexExpression *expr = LexExpression::cons( term );
	LexJoin *join = LexJoin::cons( expr );
	LexFactor *factor = LexFactor::cons( join );
	LexFactorNeg *factorNeg = LexFactorNeg::cons( internal, factor );
	LexFactorRep *factorRep = LexFactorRep::cons( internal, factorNeg );
	LexFactorAug *factorAug = LexFactorAug::cons( factorRep );
	return factorAug;
}

void Bootstrap0::wsIgnore()
{
	ObjectDef *objectDef = ObjectDef::cons( ObjectDef::UserType, String(), pd->nextObjectId++ ); 

	LexTerm *r1 = litTerm( "' '" );
	LexTerm *r2 = litTerm( "'\t'" );
	LexTerm *r3 = litTerm( "'\v'" );
	LexTerm *r4 = litTerm( "'\n'" );
	LexTerm *r5 = litTerm( "'\r'" );
	LexTerm *r6 = litTerm( "'\f'" );

	LexExpression *whitespace = orExpr( r1, r2, r3, r4, r5, r6 );
	LexFactorAug *whitespaceRep = plusFactorAug( whitespace );

	LexTerm *term = LexTerm::cons( whitespaceRep );
	LexExpression *expr = LexExpression::cons( term );
	LexJoin *join = LexJoin::cons( expr );

	tokenDef( internal, String(), join, objectDef, 0, true, false, false );
}

void Bootstrap0::idToken()
{
	String hello( "id" );

	ObjectDef *objectDef = ObjectDef::cons( ObjectDef::UserType, hello, pd->nextObjectId++ ); 

	LexTerm *r1 = rangeTerm( "'a'", "'z'" );
	LexTerm *r2 = rangeTerm( "'A'", "'Z'" );
	LexTerm *r3 = litTerm( "'_'" );
	LexFactorAug *first = parensFactorAug( orExpr( r1, r2, r3 ) ); 

	LexTerm *r4 = rangeTerm( "'a'", "'z'" );
	LexTerm *r5 = rangeTerm( "'A'", "'Z'" );
	LexTerm *r6 = litTerm( "'_'" );
	LexTerm *r7 = rangeTerm( "'0'", "'9'" );
	LexExpression *second = orExpr( r4, r5, r6, r7 );
	LexFactorAug *secondStar = starFactorAug( second );

	LexTerm *concat = concatTerm( first, secondStar );

	LexExpression *expr = LexExpression::cons( concat );
	LexJoin *join = LexJoin::cons( expr );

	tokenDef( internal, hello, join, objectDef, 0, false, false, false );
}

void Bootstrap0::keyword( const String &kw )
{
	literalDef( internal, kw, false, false );
}

void Bootstrap0::symbol( const String &kw )
{
	literalDef( internal, kw, false, false );
}

ProdEl *Bootstrap0::prodRefName( const String &name )
{
	ProdEl *prodEl = prodElName( internal, name,
			NamespaceQual::cons(namespaceStack.top()), 0,
			RepeatNone, false );
	return prodEl;
}

ProdEl *Bootstrap0::prodRefNameRepeat( const String &name )
{
	ProdEl *prodEl = prodElName( internal, name,
			NamespaceQual::cons(namespaceStack.top()), 0,
			RepeatRepeat, false );
	return prodEl;
}

ProdEl *Bootstrap0::prodRefLit( const String &lit )
{
	ProdEl *prodEl = prodElLiteral( internal, lit, 
			NamespaceQual::cons(namespaceStack.top()), 0,
			RepeatNone, false );
	return prodEl;
}

Production *Bootstrap0::production( ProdEl *prodEl1 )
{
	ProdElList *prodElList = new ProdElList;
	prodElList->append( prodEl1 );
	return BaseParser::production( internal, prodElList, false, 0, 0 );
}

Production *Bootstrap0::production( ProdEl *prodEl1, ProdEl *prodEl2 )
{
	ProdElList *prodElList = new ProdElList;
	prodElList->append( prodEl1 );
	prodElList->append( prodEl2 );
	return BaseParser::production( internal, prodElList, false, 0, 0 );
}

Production *Bootstrap0::production( ProdEl *prodEl1, ProdEl *prodEl2,
		ProdEl *prodEl3 )
{
	ProdElList *prodElList = new ProdElList;
	prodElList->append( prodEl1 );
	prodElList->append( prodEl2 );
	prodElList->append( prodEl3 );
	return BaseParser::production( internal, prodElList, false, 0, 0 );
}

Production *Bootstrap0::production( ProdEl *prodEl1, ProdEl *prodEl2,
		ProdEl *prodEl3, ProdEl *prodEl4 )
{
	ProdElList *prodElList = new ProdElList;

	prodElList->append( prodEl1 );
	prodElList->append( prodEl2 );
	prodElList->append( prodEl3 );
	prodElList->append( prodEl4 );

	return BaseParser::production( internal, prodElList, false, 0, 0 );
}

void Bootstrap0::definition( const String &name, Production *prod )
{
	LelDefList *defList = new LelDefList;
	prodAppend( defList, prod );

	NtDef *ntDef = NtDef::cons( name, namespaceStack.top(), contextStack.top(), false );
	ObjectDef *objectDef = ObjectDef::cons( ObjectDef::UserType, name, pd->nextObjectId++ ); 
	cflDef( ntDef, objectDef, defList );
}

void Bootstrap0::itemProd()
{
	ProdEl *prodEl1 = prodRefLit( "'def'" );
	ProdEl *prodEl2 = prodRefName( "id" );
	ProdEl *prodEl3 = prodRefLit( "'['" );
	ProdEl *prodEl4 = prodRefLit( "']'" );

	Production *prod1 = production( prodEl1, prodEl2, prodEl3, prodEl4 );

	definition( "item",  prod1 );
}

void Bootstrap0::startProd()
{
	ProdEl *prodEl1 = prodRefNameRepeat( "item" );
	Production *prod1 = production( prodEl1 );

	definition( "start",  prod1 );
}

void Bootstrap0::parseInput( StmtList *stmtList )
{
	NamespaceQual *nspaceQual = NamespaceQual::cons( namespaceStack.top() );
	TypeRef *typeRef = TypeRef::cons( internal, nspaceQual, String("start"), RepeatNone );

	LangVarRef *varRef = LangVarRef::cons( internal, new QualItemVect, String("stdin") );
	LangExpr *expr = LangExpr::cons( LangTerm::cons( internal, LangTerm::VarRefType, varRef ) );

	ConsItem *consItem = ConsItem::cons( internal, ConsItem::ExprType, expr );
	ConsItemList *list = ConsItemList::cons( consItem );

	ObjectField *objField = ObjectField::cons( internal, 0, String("P") );

	expr = parseCmd( internal, false, objField, typeRef, 0, list );
	LangStmt *stmt = LangStmt::cons( internal, LangStmt::ExprType, expr );
	stmtList->append( stmt );
}

void Bootstrap0::printParseTree( StmtList *stmtList )
{
	QualItemVect *qual = new QualItemVect;
	qual->append( QualItem( internal, String( "P" ), QualItem::Dot ) );
	LangVarRef *varRef = LangVarRef::cons( internal, qual, String("tree") );
	LangExpr *expr = LangExpr::cons( LangTerm::cons( internal, LangTerm::VarRefType, varRef ) );

	ExprVect *exprVect = new ExprVect;
	exprVect->append( expr );
	LangStmt *stmt = LangStmt::cons( internal, LangStmt::PrintType, exprVect );
	stmtList->append( stmt );
}

void Bootstrap0::go()
{
	StmtList *stmtList = new StmtList;

	/* The token region */
	pushRegionSet( internal );

	wsIgnore();
	keyword( "'def'" );
	idToken();
	symbol( "'['" );
	symbol( "']'" );

	popRegionSet();

	itemProd();
	startProd();

	parseInput( stmtList );

	printParseTree( stmtList );

	pd->rootCodeBlock = CodeBlock::cons( stmtList, 0 );
}

