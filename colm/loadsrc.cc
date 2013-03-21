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
#include "loadsrc.h"
#include "exports2.h"
#include "colm/colm.h"

extern RuntimeData main_runtimeData;

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

TypeRef *LoadSource::walkTypeRef( type_ref typeRefTree )
{
	NamespaceQual *nspaceQual = walkRegionQual( typeRefTree.RegionQual() );
	String id = typeRefTree.Id().text().c_str();
	RepeatType repeatType = walkOptRepeat( typeRefTree.OptRepeat() );

	return TypeRef::cons( internal, nspaceQual, id, repeatType );
}

StmtList *LoadSource::walkLangStmtList( lang_stmt_list langStmtList )
{
	StmtList *retList = new StmtList;
	_repeat_statement stmtList = langStmtList.StmtList();

	/* Walk the list of items. */
	while ( !stmtList.end() ) {
		statement Statement = stmtList.value();
		LangStmt *stmt = walkStatement( Statement );
		retList->append( stmt );
		stmtList = stmtList.next();
	}

	return retList;
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

void LoadSource::walkProdElList( ProdElList *list, prod_el_list &ProdElList )
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

		RepeatType repeatType = RepeatNone;
		if ( El.OptRepeat().Star() != 0 )
			repeatType = RepeatRepeat;

		if ( El.Id() != 0 ) {
			String typeName = El.Id().text().c_str();
			ProdEl *prodEl = prodElName( internal, typeName,
					NamespaceQual::cons(namespaceStack.top()),
					captureField, repeatType, false );
			appendProdEl( list, prodEl );
		}
		else if ( El.Lit() != 0 ) {
			String lit = El.Lit().text().c_str();
			ProdEl *prodEl = prodElLiteral( internal, lit,
					NamespaceQual::cons(namespaceStack.top()),
					captureField, repeatType, false );
			appendProdEl( list, prodEl );

		}
	}
}

void LoadSource::walkProdList( LelDefList *lelDefList, prod_list &ProdList )
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

String transReChars( const String &s )
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
			i+= 2;
		}
		else {
			*d++ = s[i];
			i += 1;
		}
	}
	return out;
}

ReOrItem *LoadSource::walkRegOrChar( reg_or_char regOrChar )
{
	ReOrItem *orItem = 0;
	if ( regOrChar.Char() != 0 ) {
		String c = transReChars( regOrChar.Char().text().c_str() );
		orItem = ReOrItem::cons( internal, c );
	}
	else {
		String low = transReChars( regOrChar.Low().text().c_str() );
		String high = transReChars( regOrChar.High().text().c_str() );
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

LexFactor *LoadSource::walkLexFactor( lex_factor &LexFactorTree )
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
		String low = LexFactorTree.Low().text().c_str();
		Literal *lowLit = Literal::cons( internal, low, Literal::LitString );

		String high = LexFactorTree.High().text().c_str();
		Literal *highLit = Literal::cons( internal, high, Literal::LitString );

		Range *range = Range::cons( lowLit, highLit );
		factor = LexFactor::cons( range );
	}
	else if ( LexFactorTree.PosData() != 0 ) {
		ReOrBlock *block = walkRegOrData( LexFactorTree.PosData() );
		factor = LexFactor::cons( ReItem::cons( internal, block, ReItem::OrBlock ) );
	}
	else if ( LexFactorTree.NegData() != 0 ) {
		ReOrBlock *block = walkRegOrData( LexFactorTree.PosData() );
		factor = LexFactor::cons( ReItem::cons( internal, block, ReItem::NegOrBlock ) );
	}
	return factor;
}

LexFactorNeg *LoadSource::walkLexFactorNeg( lex_factor_neg &LexFactorNegTree )
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

LexFactorRep *LoadSource::walkLexFactorRep( lex_factor_rep &LexFactorRepTree )
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
	else {
		lex_factor_neg LexFactorNegTree = LexFactorRepTree.FactorNeg();
		LexFactorNeg *factorNeg = walkLexFactorNeg( LexFactorNegTree );
		factorRep = LexFactorRep::cons( internal, factorNeg );
	}
	return factorRep;
}

LexFactorAug *LoadSource::walkLexFactorAug( lex_factor_rep &LexFactorRepTree )
{
	LexFactorRep *factorRep = walkLexFactorRep( LexFactorRepTree );
	return LexFactorAug::cons( factorRep );
}

LexTerm *LoadSource::walkLexTerm( lex_term &LexTermTree )
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

LexExpression *LoadSource::walkLexExpr( lex_expr &LexExprTree )
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

void LoadSource::walkTokenDef( token_def TokenDef )
{
	String name = TokenDef.Id().text().c_str();

	ObjectDef *objectDef = ObjectDef::cons( ObjectDef::UserType, name, pd->nextObjectId++ ); 

	lex_expr LexExpr = TokenDef.Expr();
	LexExpression *expr = walkLexExpr( LexExpr );
	LexJoin *join = LexJoin::cons( expr );

	defineToken( internal, name, join, objectDef, 0, false, false, false );
}

void LoadSource::walkIgnoreDef( ignore_def IgnoreDef )
{
	ObjectDef *objectDef = ObjectDef::cons( ObjectDef::UserType, 0, pd->nextObjectId++ ); 

	lex_expr LexExpr = IgnoreDef.Expr();
	LexExpression *expr = walkLexExpr( LexExpr );
	LexJoin *join = LexJoin::cons( expr );

	defineToken( internal, 0, join, objectDef, 0, true, false, false );
}

void LoadSource::walkLexRegion( region_def &regionDef )
{
	pushRegionSet( internal );
	walkRootItemList( regionDef.RootItemList() );
	popRegionSet();
}

void LoadSource::walkCflDef( cfl_def &cflDef )
{
	prod_list prodList = cflDef.ProdList();

	LelDefList *defList = new LelDefList;
	walkProdList( defList, prodList );

	String name = cflDef.DefId().text().c_str();
	NtDef *ntDef = NtDef::cons( name, namespaceStack.top(), contextStack.top(), false );
	ObjectDef *objectDef = ObjectDef::cons( ObjectDef::UserType, name, pd->nextObjectId++ ); 
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
	return LangStmt::cons( internal, LangStmt::PrintType, exprVect );
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

ConsItemList *walkAccumulate( accumulate Accumulate )
{
	String id = Accumulate.Id().text().c_str();
	LangVarRef *varRef = LangVarRef::cons( internal, new QualItemVect, id );
	LangExpr *accumExpr = LangExpr::cons( LangTerm::cons( internal, LangTerm::VarRefType, varRef ) );

	ConsItem *consItem = ConsItem::cons( internal, ConsItem::ExprType, accumExpr );
	ConsItemList *list = ConsItemList::cons( consItem );
	return list;
}

LangExpr *LoadSource::walkCodeFactor( code_factor codeFactor )
{
	LangExpr *expr = 0;
	if ( codeFactor.VarRef() != 0 ) {
		var_ref varRef = codeFactor.VarRef();
		LangVarRef *langVarRef = walkVarRef( varRef );
		LangTerm *term;
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
		ConsItemList *list = walkAccumulate( codeFactor.Accumulate() );

		expr = parseCmd( internal, false, objField, typeRef, 0, list );
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
	return expr;
}

LangExpr *LoadSource::walkCodeExpr( code_expr codeExpr )
{
	return walkCodeFactor( codeExpr.CodeFactor() );
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

LangStmt *LoadSource::walkStatement( statement Statement )
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
		LangExpr *expr = 0;
		if ( Statement.OptDefInit().CodeExpr() != 0 )
			expr = walkCodeExpr( Statement.OptDefInit().CodeExpr() );
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
		LangStmt *elsifList = walkElsifList( Statement.ElsifList() );
		stmt = LangStmt::cons( LangStmt::IfType, expr, stmtList, elsifList );

		popScope();
	}
	return stmt;
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
	if ( rootItem.TokenDef() != 0 ) {
		walkTokenDef( rootItem.TokenDef() );
	}
	else if ( rootItem.IgnoreDef() != 0 ) {
		walkIgnoreDef( rootItem.IgnoreDef() );
	}
	else if ( rootItem.LiteralDef() != 0 ) {
		walkLiteralDef( rootItem.LiteralDef() );
	}
	else if ( rootItem.CflDef() != 0 ) {
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
		if ( stmt != 0 )
			stmtList->append( stmt );
	}
	else if ( rootItem.NamespaceDef() != 0 ) {
		walkNamespaceDef( rootItem.NamespaceDef() );
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

