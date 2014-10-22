/*
 *  Copyright 2013 Adrian Thurston <thurston@complang.org>
 */

#include <iostream>
#include "avltree.h"
#include "parsedata.h"
#include "parser.h"

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


