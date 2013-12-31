/*
 *  Copyright 2007-2014 Adrian Thurston <thurston@complang.org>
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

#include "bytecode.h"
#include "parsedata.h"

#include <iostream>
#include <assert.h>

using std::cout;
using std::cerr;
using std::endl;

/* Recurisve find through a single object def's scope. */
ObjectField *ObjectDef::findFieldInScope( const String &name, ObjNameScope *inScope )
{
	ObjFieldMapEl *objDefMapEl = inScope->objFieldMap->find( name );
	if ( objDefMapEl != 0 )
		return objDefMapEl->value;
	if ( inScope->parentScope != 0 )
		return findFieldInScope( name, inScope->parentScope );
	return 0;
}

ObjectField *ObjectDef::findField( const String &name )
{
	//cout << "looking for " << name << endl;
	ObjectField *objField = findFieldInScope( name, scope );
	if ( objField != 0 )
		return objField;
	return 0;
}

UniqueType *LangVarRef::lookup( Compiler *pd ) const
{
	/* Lookup the loadObj. */
	VarRefLookup lookup = lookupField( pd );

	ObjectField *el = lookup.objField;
	UniqueType *elUT = el->typeRef->uniqueType;

	/* Deref iterators. */
	if ( elUT->typeId == TYPE_ITER )
		elUT = el->typeRef->searchUniqueType;

	return elUT;
}

