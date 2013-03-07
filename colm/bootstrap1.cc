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
#include <string>
#include <errno.h>

#include "parser.h"
#include "config.h"
#include "lmparse.h"
#include "global.h"
#include "input.h"
#include "bootstrap1.h"
#include "exports1.h"
#include "colm/colm.h"

using std::string;

extern RuntimeData main_runtimeData;

void Bootstrap1::defineProd( item &Prod )
{
	String name = Prod.DefId().text().c_str();
	ProdElList *prodElList = new ProdElList;
	Production *prod = BaseParser::production( internal, prodElList, false, 0, 0 );
	LelDefList *defList = new LelDefList;
	prodAppend( defList, prod );
	NtDef *ntDef = NtDef::cons( name, namespaceStack.top(), contextStack.top(), false );
	ObjectDef *objectDef = ObjectDef::cons( ObjectDef::UserType, name, pd->nextObjectId++ ); 
	cflDef( ntDef, objectDef, defList );
}

void Bootstrap1::go()
{
	StmtList *stmtList = new StmtList;

	colmInit( 0 );
	ColmProgram *program = colmNewProgram( &main_runtimeData );
	colmRunProgram( program, 0, 0 );

	/* Extract the parse tree. */
	start Start = Colm0Tree( program );

	/* Walk the list of items. */
	_repeat_item ItemList = Start.ItemList();
	while ( !ItemList.end() ) {

		item Item = ItemList.value();
		if ( Item.DefId() != 0 ) {

			std::cout << "define: " << Item.text() << std::endl;
		}
		else {
			std::cout << "other:  " << Item.text() << std::endl;
		}
		ItemList = ItemList.next();
	}

	colmDeleteProgram( program );

	//parseInput( stmtList );
	//exportTree( stmtList );

	pd->rootCodeBlock = CodeBlock::cons( stmtList, 0 );
}

