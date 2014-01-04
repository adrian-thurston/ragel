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

ObjectDef *UniqueType::objectDef()
{
	if ( typeId != TYPE_TREE && typeId != TYPE_REF ) {
		/* This should have generated a compiler error. */
		assert(false);
	}

	return langEl->objectDef;
}

/* Recurisve find through a single object def's scope. */
ObjectField *ObjectDef::findFieldInScope( const String &name, const ObjNameScope *inScope ) const
{
	ObjFieldMapEl *objDefMapEl = inScope->objFieldMap->find( name );
	if ( objDefMapEl != 0 )
		return objDefMapEl->value;
	if ( inScope->parentScope != 0 )
		return findFieldInScope( name, inScope->parentScope );
	return 0;
}

ObjectField *ObjNameScope::findField( const String &name ) const
{
	return owner->findFieldInScope( name, this );
}

ObjMethod *ObjectDef::findMethod( const String &name ) const
{
	ObjMethodMapEl *objMethodMapEl = objMethodMap->find( name );
	if ( objMethodMapEl != 0 )
		return objMethodMapEl->value;
	return 0;
}

VarRefLookup LangVarRef::lookupQualification( Compiler *pd, ObjNameScope *rootScope ) const
{
	int lastPtrInQual = -1;
	ObjNameScope *searchScope = rootScope;
	int firstConstPart = -1;

	for ( QualItemVect::Iter qi = *qual; qi.lte(); qi++ ) {
		/* Lookup the field int the current qualification. */
		ObjectField *el = searchScope->findField( qi->data );
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

		ObjectDef *searchObjDef = qualUT->objectDef();
		searchScope = searchObjDef->rootScope;
	}

	return VarRefLookup( lastPtrInQual, firstConstPart, searchScope->owner, searchScope );
}

bool LangVarRef::isLocalRef() const
{
	if ( qual->length() > 0 ) {
		if ( scope->findField( qual->data[0].data ) != 0 )
			return true;
	}
	else if ( scope->findField( name ) != 0 )
		return true;
	else if ( scope->owner->findMethod( name ) != 0 )
		return true;

	return false;
}

bool LangVarRef::isContextRef() const
{
	if ( context != 0 ) {
		if ( qual->length() > 0 ) {
			if ( context->contextObjDef->rootScope->findField( qual->data[0].data ) != 0 )
				return true;
		}
		else if ( context->contextObjDef->rootScope->findField( name ) != 0 )
			return true;
		else if ( context->contextObjDef->findMethod( name ) != 0 )
			return true;
	}

	return false;
}

bool LangVarRef::isCustom() const
{
	if ( qual->length() > 0 ) {
		ObjectField *field = scope->findField( qual->data[0].data );
		if ( field != 0 && field->isCustom )
			return true;
	}
	else {
		ObjectField *field = scope->findField( name );
		if ( field != 0 ) {
			if ( field->isCustom )
				return true;
		}
		else {
			ObjMethod *method = scope->owner->findMethod( name );
			if ( method != 0 && method->isCustom )
				return true;
		}
	
	}
	return false;
}

VarRefLookup LangVarRef::lookupObj( Compiler *pd ) const
{
	ObjNameScope *rootScope;
	if ( isLocalRef() )
		rootScope = scope;
	else if ( isContextRef() )
		rootScope = context->contextObjDef->rootScope;
	else
		rootScope = pd->globalObjectDef->rootScope;

	return lookupQualification( pd, rootScope );
}

VarRefLookup LangVarRef::lookupField( Compiler *pd ) const
{
	/* Lookup the object that the field is in. */
	VarRefLookup lookup = lookupObj( pd );

	/* Lookup the field. */
	ObjectField *field = lookup.inScope->findField( name );
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
	UniqueType *elUT = el->typeRef->lookupType( pd );

	/* Deref iterators. */
	if ( elUT->typeId == TYPE_ITER )
		elUT = el->typeRef->searchUniqueType;

	return elUT;
}


VarRefLookup LangVarRef::lookupMethod( Compiler *pd ) const
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
