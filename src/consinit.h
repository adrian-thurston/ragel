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

#include <iostream>

#include <avltree.h>

#include "compiler.h"
#include "parser.h"

#ifndef _COLM_CONSINIT_H
#define _COLM_CONSINIT_H

struct ConsInit
:
	public BaseParser
{
	ConsInit( Compiler *pd )
	:
		BaseParser(pd)
	{}

	ProdEl *prodRefName( const String &name );
	ProdEl *prodRefName( const String &capture, const String &name );
	ProdEl *prodRefNameRepeat( const String &name );
	ProdEl *prodRefNameRepeat( const String &capture, const String &name );
	ProdEl *prodRefLit( const String &lit );

	Production *production();
	Production *production( ProdEl *prodEl1 );
	Production *production( ProdEl *prodEl1, ProdEl *prodEl2 );
	Production *production( ProdEl *prodEl1, ProdEl *prodEl2,
			ProdEl *prodEl3 );
	Production *production( ProdEl *prodEl1, ProdEl *prodEl2,
			ProdEl *prodEl3, ProdEl *prodEl4 );
	Production *production( ProdEl *prodEl1, ProdEl *prodEl2,
			ProdEl *prodEl3, ProdEl *prodEl4, ProdEl *prodEl5 );
	Production *production( ProdEl *prodEl1, ProdEl *prodEl2,
			ProdEl *prodEl3, ProdEl *prodEl4, ProdEl *prodEl5,
			ProdEl *prodEl6, ProdEl *prodEl7 );

	void definition( const String &name, Production *prod );
	void definition( const String &name, Production *prod1, Production *prod2 );
	void definition( const String &name, Production *prod1, Production *prod2, Production *prod3 );
	void definition( const String &name, Production *prod1, Production *prod2,
			Production *prod3, Production *prod4 );

	void keyword( const String &name, const String &lit );
	void keyword( const String &kw );

	void printParseTree( StmtList *stmtList );
	void printParseTree();

	void literalToken();
	void commentIgnore();
	void wsIgnore();
	void idToken();

	void token();
	void ignore();
	void tokenList();

	void lexFactor();
	void lexFactorNeg();
	void lexFactorRep();
	void lexExpr();
	void lexTerm();

	Production *prodProd();
	Production *prodLex();

	void optNi();
	void optRepeat();
	void optProdElName();
	void prodEl();
	void prodElList();
	void item();
	void prodList();
	void optProdName();
	void prod();
	void startProd();
	void optCommit();

	void parseInput( StmtList *stmtList );
	void exportTree( StmtList *stmtList );

	virtual void go( long activeRealm );
};

#endif /* _COLM_CONSINIT_H */

