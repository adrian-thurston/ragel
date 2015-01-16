/*
 *  Copyright 2007-2014 Adrian Thurston <thurston@complang.org>
 */

#include "bytecode.h"
#include "compiler.h"

#include <iostream>
#include <assert.h>

using std::cout;
using std::cerr;
using std::endl;

ObjectDef *UniqueType::objectDef()
{
	if ( typeId == TYPE_TREE || typeId == TYPE_REF ) {
		return langEl->objectDef;
	}
	else if ( typeId == TYPE_STRUCT ) {
		return structEl->structDef->objectDef;
	}
	else if ( typeId == TYPE_GENERIC ) {
		return generic->objDef;
	}

	/* This should have generated a compiler error. */
	assert( false );
}

/* Recurisve find through a single object def's scope. */
ObjectField *ObjectDef::findFieldInScope( const NameScope *inScope,
		const String &name ) const
{
	FieldMapEl *objDefMapEl = inScope->fieldMap->find( name );
	if ( objDefMapEl != 0 )
		return objDefMapEl->value;
	if ( inScope->parentScope != 0 )
		return findFieldInScope( inScope->parentScope, name );
	return 0;
}

ObjectField *NameScope::findField( const String &name ) const
{
	return owner->findFieldInScope( this, name );
}

ObjectMethod *ObjectDef::findMethod( const String &name ) const
{
	MethodMapEl *methodMapEl = methodMap->find( name );
	if ( methodMapEl != 0 )
		return methodMapEl->value;
	return 0;
}

VarRefLookup LangVarRef::lookupQualification( Compiler *pd, NameScope *rootScope ) const
{
	int lastPtrInQual = -1;
	NameScope *searchScope = rootScope;
	int firstConstPart = -1;

	for ( QualItemVect::Iter qi = *qual; qi.lte(); qi++ ) {
		/* Lookup the field int the current qualification. */
		ObjectField *el = searchScope->findField( qi->data );
		if ( el == 0 )
			error(qi->loc) << "cannot resolve qualification " << qi->data << endp;

		/* Lookup the type of the field. */
		el->typeRef->resolveType( pd );
		UniqueType *qualUT = el->typeRef->uniqueType;

		/* If we are dealing with an iterator then dereference it. */
		if ( qualUT->typeId == TYPE_ITER )
			qualUT = el->typeRef->searchUniqueType;

		/* Is it const? */
		if ( firstConstPart < 0 && el->isConst )
			firstConstPart = qi.pos();

		/* Check for references. When loop is done we will have the last one
		 * present, if any. */
		if ( qualUT->ptr() )
			lastPtrInQual = qi.pos();

		if ( qi->form == QualItem::Dot ) {
			/* Cannot dot a reference. Iterator yes (access of the iterator
			 * not the current) */
			if ( qualUT->ptr() )
				error(loc) << "dot cannot be used to access a pointer" << endp;
		}
		else if ( qi->form == QualItem::Arrow ) {
			if ( qualUT->typeId == TYPE_ITER )
				qualUT = el->typeRef->searchUniqueType;
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
			if ( context->objectDef->rootScope->findField( qual->data[0].data ) != 0 )
				return true;
		}
		else if ( context->objectDef->rootScope->findField( name ) != 0 )
			return true;
		else if ( context->objectDef->findMethod( name ) != 0 )
			return true;
	}

	return false;
}

bool LangVarRef::isInbuiltObject() const
{
	if ( qual->length() > 0 ) {
		ObjectField *field = scope->findField( qual->data[0].data );
		if ( field != 0 && field->isInbuiltObject() )
			return true;
	}
	else {
		ObjectField *field = scope->findField( name );
		if ( field != 0 ) {
			if ( field->isInbuiltObject() )
				return true;
		}
	}
	return false;
}

VarRefLookup LangVarRef::lookupObj( Compiler *pd ) const
{
	NameScope *rootScope;
	if ( isLocalRef() )
		rootScope = scope;
	else if ( isContextRef() )
		rootScope = context->objectDef->rootScope;
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
	UniqueType *elUT = el->typeRef->resolveType( pd );

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
	assert( lookup.inObject->methodMap != 0 );
	ObjectMethod *method = lookup.inObject->findMethod( name );
	if ( method == 0 ) {
		/* Not found as a method, try it as an object on which we will call a
		 * default function. */
		qual->append( QualItem( QualItem::Dot, loc, name ) );

		/* Lookup the object that the field is in. */
		VarRefLookup lookup = lookupObj( pd );

		/* Find the method. */
		assert( lookup.inObject->methodMap != 0 );
		method = lookup.inObject->findMethod( "finish" );
		if ( method == 0 )
			error(loc) << "cannot find " << name << "(...) in object" << endp;
	}
	
	lookup.objMethod = method;
	lookup.uniqueType = method->returnUT;
	
	return lookup;
}
