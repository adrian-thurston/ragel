/*
 *  Copyright 2013 Adrian Thurston <thurston@complang.org>
 */

#include <iostream>
#include "avltree.h"
#include "compiler.h"
#include "parser.h"

struct lex_factor;
struct lex_factor_neg;
struct lex_factor_rep;
struct lex_term;
struct lex_expr;
struct token_list;
struct prod_el_list;
struct prod_list;
struct item;

struct LoadInit
:
	public BaseParser
{
	LoadInit( Compiler *pd, const char *inputFileName )
	:
		BaseParser(pd),
		inputFileName(inputFileName)
	{}

	const char *inputFileName;

	/* Constructing the colm language data structures from the the parse tree. */
	LexFactor *walkLexFactor( lex_factor &LexFactorTree );
	LexFactorNeg *walkLexFactorNeg( lex_factor_neg &LexFactorNegTree );
	LexFactorRep *walkLexFactorRep( lex_factor_rep &LexFactorRepTree );
	LexFactorAug *walkLexFactorAug( lex_factor_rep &LexFactorRepTree );
	LexTerm *walkLexTerm( lex_term &LexTerm );
	LexExpression *walkLexExpr( lex_expr &LexExpr );
	void walkTokenList( token_list &TokenList );
	void walkLexRegion( item &LexRegion );
	void walkProdElList( String defName, ProdElList *list, prod_el_list &prodElList );
	void walkProdList( String defName, LelDefList *list, prod_list &prodList );
	void walkDefinition( item &define );

	/* Constructing statements needed to parse and export the input. */
	void consParseStmt( StmtList *stmtList );
	void consExportTree( StmtList *stmtList );
	void consExportError( StmtList *stmtList );

	virtual void go( long activeRealm );
};
