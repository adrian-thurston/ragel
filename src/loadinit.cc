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

#include "loadinit.h"

#include <string.h>

#include <iostream>

#include "gen/if1.h"

using std::string;

extern colm_sections colm_object;

void LoadInit::walkProdElList( String defName, ProdElList *list, prod_el_list &prodElList )
{
	if ( prodElList.ProdElList() != 0 ) {
		prod_el_list RightProdElList = prodElList.ProdElList();
		walkProdElList( defName, list, RightProdElList );
	}
	
	if ( prodElList.ProdEl() != 0 ) {
		prod_el El = prodElList.ProdEl();
		String typeName = El.Id().text().c_str();

		ObjectField *captureField = 0;
		if ( El.OptName().Name() != 0 ) {
			/* Has a capture. */
			String fieldName = El.OptName().Name().text().c_str();
			captureField = ObjectField::cons( internal,
					ObjectField::RhsNameType, 0, fieldName );
		}
		else {
			/* Default the capture to the name of the type. */
			String fieldName = typeName;
			if ( strcmp( fieldName, defName ) == 0 )
				fieldName = "_" + defName;
			captureField = ObjectField::cons( internal,
					ObjectField::RhsNameType, 0, fieldName );
		}

		RepeatType repeatType = RepeatNone;
		if ( El.OptRepeat().Star() != 0 )
			repeatType = RepeatRepeat;

		ProdEl *prodEl = prodElName( internal, typeName,
				NamespaceQual::cons( curNspace() ),
				captureField, repeatType, false );

		appendProdEl( list, prodEl );
	}
}

void LoadInit::walkProdList( String defName, LelDefList *outProdList, prod_list &prodList )
{
	if ( prodList.ProdList() != 0 ) {
		prod_list RightProdList = prodList.ProdList();
		walkProdList( defName, outProdList, RightProdList );
	}

	ProdElList *outElList = new ProdElList;
	prod_el_list prodElList = prodList.Prod().ProdElList();
	walkProdElList( defName, outElList, prodElList );

	String name;
	if ( prodList.Prod().OptName().Name() != 0 )
		name = prodList.Prod().OptName().Name().text().c_str();

	bool commit = prodList.Prod().OptCommit().Commit() != 0;

	Production *prod = BaseParser::production( internal, outElList, name, commit, 0, 0 );
	prodAppend( outProdList, prod );
}

LexFactor *LoadInit::walkLexFactor( lex_factor &lexFactor )
{
	LexFactor *factor = 0;
	if ( lexFactor.Literal() != 0 ) {
		String litString = lexFactor.Literal().text().c_str();
		Literal *literal = Literal::cons( internal, litString, Literal::LitString );
		factor = LexFactor::cons( literal );
	}
	if ( lexFactor.Id() != 0 ) {
		String id = lexFactor.Id().text().c_str();
		factor = lexRlFactorName( id, internal );
	}
	else if ( lexFactor.Expr() != 0 ) {
		lex_expr LexExpr = lexFactor.Expr();
		LexExpression *expr = walkLexExpr( LexExpr );
		LexJoin *join = LexJoin::cons( expr );
		factor = LexFactor::cons( join );
	}
	else if ( lexFactor.Low() != 0 ) {
		String low = lexFactor.Low().text().c_str();
		Literal *lowLit = Literal::cons( internal, low, Literal::LitString );

		String high = lexFactor.High().text().c_str();
		Literal *highLit = Literal::cons( internal, high, Literal::LitString );

		Range *range = Range::cons( lowLit, highLit );
		factor = LexFactor::cons( range );
	}
	return factor;
}

LexFactorNeg *LoadInit::walkLexFactorNeg( lex_factor_neg &lexFactorNeg )
{
	if ( lexFactorNeg.FactorNeg() != 0 ) {
		lex_factor_neg Rec = lexFactorNeg.FactorNeg();
		LexFactorNeg *recNeg = walkLexFactorNeg( Rec );
		LexFactorNeg *factorNeg = LexFactorNeg::cons( recNeg, LexFactorNeg::CharNegateType );
		return factorNeg;
	}
	else {
		lex_factor LexFactorTree = lexFactorNeg.Factor();
		LexFactor *factor = walkLexFactor( LexFactorTree );
		LexFactorNeg *factorNeg = LexFactorNeg::cons( factor );
		return factorNeg;
	}
}

LexFactorRep *LoadInit::walkLexFactorRep( lex_factor_rep &lexFactorRep )
{
	LexFactorRep *factorRep = 0;
	if ( lexFactorRep.Star() != 0 ) {
		lex_factor_rep Rec = lexFactorRep.FactorRep();
		LexFactorRep *recRep = walkLexFactorRep( Rec );
		factorRep = LexFactorRep::cons( internal, recRep, 0, 0, LexFactorRep::StarType );
	}
	else if ( lexFactorRep.Plus() != 0 ) {
		lex_factor_rep Rec = lexFactorRep.FactorRep();
		LexFactorRep *recRep = walkLexFactorRep( Rec );
		factorRep = LexFactorRep::cons( internal, recRep, 0, 0, LexFactorRep::PlusType );
	}
	else {
		lex_factor_neg LexFactorNegTree = lexFactorRep.FactorNeg();
		LexFactorNeg *factorNeg = walkLexFactorNeg( LexFactorNegTree );
		factorRep = LexFactorRep::cons( factorNeg );
	}
	return factorRep;
}

LexFactorAug *LoadInit::walkLexFactorAug( lex_factor_rep &lexFactorRep )
{
	LexFactorRep *factorRep = walkLexFactorRep( lexFactorRep );
	return LexFactorAug::cons( factorRep );
}

LexTerm *LoadInit::walkLexTerm( lex_term &lexTerm )
{
	if ( lexTerm.Term() != 0 ) {
		lex_term Rec = lexTerm.Term();
		LexTerm *leftTerm = walkLexTerm( Rec );

		lex_factor_rep LexFactorRepTree = lexTerm.FactorRep();
		LexFactorAug *factorAug = walkLexFactorAug( LexFactorRepTree );

		LexTerm::Type type = lexTerm.Dot() != 0 ?
				LexTerm::ConcatType : LexTerm::RightFinishType;

		LexTerm *term = LexTerm::cons( leftTerm, factorAug, type );

		return term;
	}
	else {
		lex_factor_rep LexFactorRepTree = lexTerm.FactorRep();
		LexFactorAug *factorAug = walkLexFactorAug( LexFactorRepTree );
		LexTerm *term = LexTerm::cons( factorAug );
		return term;
	}
}

LexExpression *LoadInit::walkLexExpr( lex_expr &LexExprTree )
{
	if ( LexExprTree.Expr() != 0 ) {
		lex_expr Rec = LexExprTree.Expr();
		LexExpression *leftExpr = walkLexExpr( Rec );

		lex_term lexTerm = LexExprTree.Term();
		LexTerm *term = walkLexTerm( lexTerm );
		LexExpression *expr = LexExpression::cons( leftExpr, term, LexExpression::OrType );

		return expr;
	}
	else {
		lex_term lexTerm = LexExprTree.Term();
		LexTerm *term = walkLexTerm( lexTerm );
		LexExpression *expr = LexExpression::cons( term );
		return expr;
	}
}

bool walkNoIgnore( opt_ni OptNi )
{
	return OptNi.Ni() != 0;
}

void LoadInit::walkTokenList( token_list &tokenList )
{
	if ( tokenList.TokenList() != 0 ) {
		token_list RightTokenList = tokenList.TokenList();
		walkTokenList( RightTokenList );
	}
	
	if ( tokenList.TokenDef() != 0 ) {
		token_def tokenDef = tokenList.TokenDef();
		String name = tokenDef.Id().text().c_str();

		ObjectDef *objectDef = ObjectDef::cons( ObjectDef::UserType, name, pd->nextObjectId++ ); 

		lex_expr LexExpr = tokenDef.Expr();
		LexExpression *expr = walkLexExpr( LexExpr );
		LexJoin *join = LexJoin::cons( expr );

		bool leftNi = walkNoIgnore( tokenDef.LeftNi() );
		bool rightNi = walkNoIgnore( tokenDef.RightNi() );

		defineToken( internal, name, join, objectDef, 0, false, leftNi, rightNi );
	}

	if ( tokenList.IgnoreDef() != 0 ) {
		ignore_def IgnoreDef = tokenList.IgnoreDef();

		ObjectDef *objectDef = ObjectDef::cons( ObjectDef::UserType, String(), pd->nextObjectId++ ); 

		lex_expr LexExpr = IgnoreDef.Expr();
		LexExpression *expr = walkLexExpr( LexExpr );
		LexJoin *join = LexJoin::cons( expr );

		defineToken( internal, String(), join, objectDef, 0, true, false, false );
	}
}

void LoadInit::walkLexRegion( item &LexRegion )
{
	pushRegionSet( internal );

	token_list tokenList = LexRegion.TokenList();
	walkTokenList( tokenList );

	popRegionSet();
}

void LoadInit::walkDefinition( item &define )
{
	prod_list ProdList = define.ProdList();

	String name = define.DefId().text().c_str();

	LelDefList *defList = new LelDefList;
	walkProdList( name, defList, ProdList );

	NtDef *ntDef = NtDef::cons( name, curNspace(), curStruct(), false );
	ObjectDef *objectDef = ObjectDef::cons( ObjectDef::UserType, name,
			pd->nextObjectId++ ); 
	cflDef( ntDef, objectDef, defList );
}

void LoadInit::consParseStmt( StmtList *stmtList )
{
	/* Pop argv, this yields the file name . */
	CallArgVect *popArgs = new CallArgVect;
	QualItemVect *popQual = new QualItemVect;
	popQual->append( QualItem( QualItem::Arrow, internal, String( "argv" ) ) );

	LangVarRef *popRef = LangVarRef::cons( internal, curNspace(), 0,
			curLocalFrame()->rootScope, NamespaceQual::cons( curNspace() ),
			popQual, String("pop") );
	LangExpr *pop = LangExpr::cons( LangTerm::cons( InputLoc(), popRef, popArgs ) );

	TypeRef *typeRef = TypeRef::cons( internal, pd->uniqueTypeStr );
	ObjectField *objField = ObjectField::cons( internal,
			ObjectField::UserLocalType, typeRef, "A" );

	LangStmt *stmt = varDef( objField, pop, LangStmt::AssignType );
	stmtList->append( stmt );

	/* Construct a literal string 'r', for second arg to open. */
	ConsItem *modeConsItem = ConsItem::cons( internal,
			ConsItem::InputText, String("r") );
	ConsItemList *modeCons = new ConsItemList;
	modeCons->append( modeConsItem );
	LangExpr *modeExpr = LangExpr::cons( LangTerm::cons( internal, modeCons ) );

	/* Reference A->value */
	LangVarRef *varRef = LangVarRef::cons( internal, curNspace(), 0,
			curLocalFrame()->rootScope, String("A") );
	LangExpr *Avalue = LangExpr::cons( LangTerm::cons( internal,
			LangTerm::VarRefType, varRef ) );
	
	/* Call open. */
	LangVarRef *openRef = LangVarRef::cons( internal,
			curNspace(), 0, curLocalFrame()->rootScope, String("open") );
	CallArgVect *openArgs = new CallArgVect;
	openArgs->append( new CallArg(Avalue) );
	openArgs->append( new CallArg(modeExpr) );
	LangExpr *open = LangExpr::cons( LangTerm::cons( InputLoc(), openRef, openArgs ) );

	/* Construct a list containing the open stream. */
	ConsItem *consItem = ConsItem::cons( internal, ConsItem::ExprType, open, false );
	ConsItemList *list = ConsItemList::cons( consItem );

	/* Will capture the parser to "P" */
	objField = ObjectField::cons( internal,
			ObjectField::UserLocalType, 0, String("P") );

	/* Ref the start def. */
	NamespaceQual *nspaceQual = NamespaceQual::cons( curNspace() );
	typeRef = TypeRef::cons( internal, nspaceQual,
			String("start"), RepeatNone );

	/* Parse the above list. */
	LangExpr *parseExpr = parseCmd( internal, false, false, objField,
			typeRef, 0, list, true, false, "" );
	LangStmt *parseStmt = LangStmt::cons( internal, LangStmt::ExprType, parseExpr );
	stmtList->append( parseStmt );
}

void LoadInit::consExportTree( StmtList *stmtList )
{
	LangVarRef *varRef = LangVarRef::cons( internal, curNspace(), 0,
			curLocalFrame()->rootScope, String("P") );
	LangExpr *expr = LangExpr::cons( LangTerm::cons( internal,
			LangTerm::VarRefType, varRef ) );

	NamespaceQual *nspaceQual = NamespaceQual::cons( curNspace() );
	TypeRef *typeRef = TypeRef::cons( internal, nspaceQual, String("start"), RepeatNone );
	ObjectField *program = ObjectField::cons( internal,
			ObjectField::StructFieldType, typeRef, String("ColmTree") );
	LangStmt *programExport = exportStmt( program, LangStmt::AssignType, expr );
	stmtList->append( programExport );
}

void LoadInit::consExportError( StmtList *stmtList )
{
	LangVarRef *varRef = LangVarRef::cons( internal, curNspace(), 0,
			curLocalFrame()->rootScope, String("error") );
	LangExpr *expr = LangExpr::cons( LangTerm::cons( internal,
			LangTerm::VarRefType, varRef ) );

	NamespaceQual *nspaceQual = NamespaceQual::cons( curNspace() );
	TypeRef *typeRef = TypeRef::cons( internal, nspaceQual, String("str"), RepeatNone );
	ObjectField *program = ObjectField::cons( internal,
			ObjectField::StructFieldType, typeRef, String("ColmError") );
	LangStmt *programExport = exportStmt( program, LangStmt::AssignType, expr );
	stmtList->append( programExport );
}

void LoadInit::go( long activeRealm )
{
	LoadInit::init();

	StmtList *stmtList = new StmtList;

	const char *argv[3];
	argv[0] = "load-init";
	argv[1] = inputFileName;
	argv[2] = 0;

	colm_program *program = colm_new_program( &colm_object );
	colm_set_debug( program, 0 );
	colm_run_program( program, 2, argv );

	/* Extract the parse tree. */
	start Start = ColmTree( program );

	if ( Start == 0 ) {
		gblErrorCount += 1;
		std::cerr << inputFileName << ": parse error" << std::endl;
		return;
	}

	/* Walk the list of items. */
	_repeat_item ItemList = Start.ItemList();
	while ( !ItemList.end() ) {

		item Item = ItemList.value();
		if ( Item.DefId() != 0 )
			walkDefinition( Item );
		else if ( Item.TokenList() != 0 )
			walkLexRegion( Item );
		ItemList = ItemList.next();
	}

	pd->streamFileNames.append( colm_extract_fns( program ) );
	colm_delete_program( program );

	consParseStmt( stmtList );
	consExportTree( stmtList );
	consExportError( stmtList );

	pd->rootCodeBlock = CodeBlock::cons( stmtList, 0 );
}
