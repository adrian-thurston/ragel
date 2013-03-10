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
#include "parsedata.h"
#include "parser.h"
#include "bootstrap.h"

struct Bootstrap0
:
	public BootstrapBase
{
	Bootstrap0( Compiler *pd )
	:
		BootstrapBase(pd)
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

	void definition( const String &name, Production *prod );
	void definition( const String &name, Production *prod1, Production *prod2 );

	void keyword( const String &kw );
	void symbol( const String &kw );

	void printParseTree( StmtList *stmtList );
	void printParseTree();

	void literalToken();
	void wsIgnore();
	void starToken();
	void idToken();

	void token();
	void tokenList();

	void lexFactor();
	void lexFactorRep();
	void lexExpr();
	void lexTerm();

	Production *prodProd();
	Production *prodLex();

	void optRepeat();
	void optProdName();
	void prodEl();
	void prodElList();
	void item();
	void prodList();
	void prod();
	void startProd();
	void go();
};


