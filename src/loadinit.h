/*
 *  Copyright 2013 Adrian Thurston <thurston@complang.org>
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
