/*
 * Copyright 2013 Adrian Thurston <thurston@colm.net>
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

#ifndef _COLM_LOADINIT_H
#define _COLM_LOADINIT_H

#include <iostream>

#include <avltree.h>

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

#endif /* _COLM_LOAD_INIT_H */

