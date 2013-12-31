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

ObjectDef *objDefFromUT( Compiler *pd, UniqueType *ut )
{
	ObjectDef *objDef = 0;
	if ( ut->typeId == TYPE_TREE || ut->typeId == TYPE_REF )
		objDef = ut->langEl->objectDef;
	else {
		/* This should have generated a compiler error. */
		assert(false);
	}
	return objDef;
}

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

ObjectField *ObjNameScope::findField( const String &name )
{
	return owner->findFieldInScope( name, this );
}

ObjMethod *ObjectDef::findMethod( const String &name )
{
	ObjMethodMapEl *objMethodMapEl = objMethodMap->find( name );
	if ( objMethodMapEl != 0 )
		return objMethodMapEl->value;
	return 0;
}

VarRefLookup LangVarRef::lookupQualification( Compiler *pd, ObjectDef *rootDef ) const
{
	int lastPtrInQual = -1;
	ObjectDef *searchObjDef = rootDef;
	int firstConstPart = -1;

	for ( QualItemVect::Iter qi = *qual; qi.lte(); qi++ ) {
		/* Lookup the field int the current qualification. */
		ObjectField *el = searchObjDef->scope->findField( qi->data );
		if ( el == 0 )
			error(qi->loc) << "cannot resolve qualification " << qi->data << endp;

		/* Lookup the type of the field. */
		UniqueType *qualUT = el->typeRef->uniqueType;

		/* If we are dealing with an iterator then dereference it. */
		if ( qualUT->typeId == TYPE_ITER )
			qualUT = el->typeRef->searchUniqueType;

		/* Is it const? */
		if ( firstConstPart < 0 && el->isConst )
			firstConstPart = qi.pos();

		/* Check for references. When loop is done we will have the last one
		 * present, if any. */
		if ( qualUT->typeId == TYPE_PTR )
			lastPtrInQual = qi.pos();

		if ( qi->form == QualItem::Dot ) {
			/* Cannot dot a reference. Iterator yes (access of the iterator
			 * not the current) */
			if ( qualUT->typeId == TYPE_PTR )
				error(loc) << "dot cannot be used to access a pointer" << endp;
		}
		else if ( qi->form == QualItem::Arrow ) {
			if ( qualUT->typeId == TYPE_ITER )
				qualUT = el->typeRef->searchUniqueType;
			else if ( qualUT->typeId == TYPE_PTR )
				qualUT = pd->findUniqueType( TYPE_TREE, qualUT->langEl );
		}

		searchObjDef = objDefFromUT( pd, qualUT );
	}

	return VarRefLookup( lastPtrInQual, firstConstPart, searchObjDef );
}


VarRefLookup LangVarRef::lookupObj( Compiler *pd ) const
{
	ObjectDef *rootDef;
	if ( isLocalRef( pd ) )
		rootDef = pd->curLocalFrame;
	else if ( isContextRef( pd ) )
		rootDef = pd->context->contextObjDef;
	else
		rootDef = pd->globalObjectDef;

	return lookupQualification( pd, rootDef );
}

VarRefLookup LangVarRef::lookupField( Compiler *pd ) const
{
	/* Lookup the object that the field is in. */
	VarRefLookup lookup = lookupObj( pd );

	/* Lookup the field. */
	ObjectField *field = lookup.inObject->scope->findField( name );
	if ( field == 0 )
		error(loc) << "cannot find name " << name << " in object" << endp;

	lookup.objField = field;
	lookup.uniqueType = field->typeRef->uniqueType;

	if ( field->typeRef->searchUniqueType != 0 )
		lookup.iterSearchUT = field->typeRef->searchUniqueType;

	return lookup;
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


VarRefLookup LangVarRef::lookupMethod( Compiler *pd ) 
{
	/* Lookup the object that the field is in. */
	VarRefLookup lookup = lookupObj( pd );

	/* Find the method. */
	assert( lookup.inObject->objMethodMap != 0 );
	ObjMethod *method = lookup.inObject->findMethod( name );
	if ( method == 0 ) {
		/* Not found as a method, try it as an object on which we will call a
		 * default function. */
		qual->append( QualItem( QualItem::Dot, loc, name ) );

		/* Lookup the object that the field is in. */
		VarRefLookup lookup = lookupObj( pd );

		/* Find the method. */
		assert( lookup.inObject->objMethodMap != 0 );
		method = lookup.inObject->findMethod( "finish" );
		if ( method == 0 )
			error(loc) << "cannot find " << name << "(...) in object" << endp;
	}
	
	lookup.objMethod = method;
	lookup.uniqueType = method->returnUT;
	
	return lookup;
}
