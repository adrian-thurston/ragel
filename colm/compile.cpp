/*
 *  Copyright 2007 Adrian Thurston <thurston@complang.org>
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

void ParseData::initUniqueTypes( )
{
	uniqueTypeNil = new UniqueType( TYPE_NIL );
	uniqueTypePtr = new UniqueType( TYPE_TREE, ptrKlangEl );
	uniqueTypeBool = new UniqueType( TYPE_TREE, boolKlangEl );
	uniqueTypeInt = new UniqueType( TYPE_TREE, intKlangEl );
	uniqueTypeStr = new UniqueType( TYPE_TREE, strKlangEl );
	uniqueTypeStream = new UniqueType( TYPE_TREE, streamKlangEl );
	uniqueTypeAny = new UniqueType( TYPE_TREE, anyKlangEl );

	uniqeTypeMap.insert( uniqueTypeNil );
	uniqeTypeMap.insert( uniqueTypePtr );
	uniqeTypeMap.insert( uniqueTypeBool );
	uniqeTypeMap.insert( uniqueTypeInt );
	uniqeTypeMap.insert( uniqueTypeStr );
	uniqeTypeMap.insert( uniqueTypeStream );
	uniqeTypeMap.insert( uniqueTypeAny );
}

IterDef::IterDef( Type type ) : 
	type(type), 
	func(0),
	useFuncId(false),
	useSearchUT(false)
{
	if ( type == Tree ) {
		inCreate = IN_TRITER_FROM_REF;
		inDestroy = IN_TRITER_DESTROY;
		inAdvance = IN_TRITER_ADVANCE;

		inGetCurR = IN_TRITER_GET_CUR_R;
		inGetCurWC = IN_TRITER_GET_CUR_WC;
		inSetCurWC = IN_TRITER_SET_CUR_WC;
		inRefFromCur = IN_TRITER_REF_FROM_CUR;
		useSearchUT = true;
	}
	else if ( type == Child ) {
		inCreate = IN_TRITER_FROM_REF;
		inDestroy = IN_TRITER_DESTROY;
		inAdvance = IN_TRITER_NEXT_CHILD;

		inGetCurR = IN_TRITER_GET_CUR_R;
		inGetCurWC = IN_TRITER_GET_CUR_WC;
		inSetCurWC = IN_TRITER_SET_CUR_WC;
		inRefFromCur = IN_TRITER_REF_FROM_CUR;
		useSearchUT = true;
	}
	else if ( type == RevChild ) {
		inCreate = IN_TRITER_FROM_REF;
		inDestroy = IN_TRITER_DESTROY;
		inAdvance = IN_TRITER_PREV_CHILD;

		inGetCurR = IN_TRITER_GET_CUR_R;
		inGetCurWC = IN_TRITER_GET_CUR_WC;
		inSetCurWC = IN_TRITER_SET_CUR_WC;
		inRefFromCur = IN_TRITER_REF_FROM_CUR;
		useSearchUT = true;
	}
	else
		assert(false);
}

IterDef::IterDef( Type type, Function *func ) : 
	type(type),
	func(func),
	useFuncId(true),
	useSearchUT(true),
	inCreate(IN_UITER_CREATE),
	inDestroy(IN_UITER_DESTROY),
	inAdvance(IN_UITER_ADVANCE),
	inGetCurR(IN_UITER_GET_CUR_R),
	inGetCurWC(IN_UITER_GET_CUR_WC),
	inSetCurWC(IN_UITER_SET_CUR_WC),
	inRefFromCur(IN_UITER_REF_FROM_CUR)
{}

ObjMethod *initFunction( UniqueType *retType, ObjectDef *obj, 
		const String &name, int methIdWV, int methIdWC, bool isConst )
{
	ObjMethod *objMethod = new ObjMethod( retType, name, 
			methIdWV, methIdWC, 0, 0, 0, isConst );
	obj->objMethodMap->insert( name, objMethod );
	return objMethod;
}

ObjMethod *initFunction( UniqueType *retType, ObjectDef *obj, 
		const String &name, int methIdWV, int methIdWC, UniqueType *arg1, bool isConst )
{
	UniqueType *args[] = { arg1 };
	ObjMethod *objMethod = new ObjMethod( retType, name, 
			methIdWV, methIdWC, 1, args, 0, isConst );
	obj->objMethodMap->insert( name, objMethod );
	return objMethod;
}

ObjMethod *initFunction( UniqueType *retType, ObjectDef *obj, 
		const String &name, int methIdWV, int methIdWC, 
		UniqueType *arg1, UniqueType *arg2, bool isConst )
{
	UniqueType *args[] = { arg1, arg2 };
	ObjMethod *objMethod = new ObjMethod( retType, name, 
			methIdWV, methIdWC, 2, args, 0, isConst );
	obj->objMethodMap->insert( name, objMethod );
	return objMethod;
}

IterDef *ParseData::findIterDef( IterDef::Type type, Function *func )
{
	IterDefSetEl *el = iterDefSet.find( IterDef( type, func ) );
	if ( el == 0 )
		el = iterDefSet.insert( IterDef( type, func ) );
	return &el->key;
}

IterDef *ParseData::findIterDef( IterDef::Type type )
{
	IterDefSetEl *el = iterDefSet.find( IterDef( type ) );
	if ( el == 0 )
		el = iterDefSet.insert( IterDef( type ) );
	return &el->key;
}

UniqueType *ParseData::findUniqueType( int typeId )
{
	UniqueType searchKey( typeId );
	UniqueType *uniqueType = uniqeTypeMap.find( &searchKey );
	if ( uniqueType == 0 ) {
		uniqueType = new UniqueType( typeId );
		uniqeTypeMap.insert( uniqueType );
	}
	return uniqueType;
}

UniqueType *ParseData::findUniqueType( int typeId, KlangEl *langEl )
{
	UniqueType searchKey( typeId, langEl );
	UniqueType *uniqueType = uniqeTypeMap.find( &searchKey );
	if ( uniqueType == 0 ) {
		uniqueType = new UniqueType( typeId, langEl );
		uniqeTypeMap.insert( uniqueType );
	}
	return uniqueType;
}

UniqueType *ParseData::findUniqueType( int typeId, IterDef *iterDef )
{
	UniqueType searchKey( typeId, iterDef );
	UniqueType *uniqueType = uniqeTypeMap.find( &searchKey );
	if ( uniqueType == 0 ) {
		uniqueType = new UniqueType( typeId, iterDef );
		uniqeTypeMap.insert( uniqueType );
	}
	return uniqueType;
}

UniqueType *TypeRef::lookupTypePart( ParseData *pd, 
		NamespaceQual *qual, const String &name )
{
	/* Lookup up the qualifiction and then the name. */
	Namespace *nspace = qual->getQual( pd );

	if ( nspace == 0 )
		error(loc) << "do not have region for resolving reference" << endp;

	/* Search for the token in the region by name. */
	SymbolMapEl *inDict = nspace->symbolMap.find( name );
	if ( inDict != 0 ) {
		long typeId = ( isPtr ? TYPE_PTR : ( isRef ? TYPE_REF : TYPE_TREE ) );
		return pd->findUniqueType( typeId, inDict->value );
	}

	error(loc) << "unknown type in typeof expression" << endp;
	return 0;
}

UniqueType *TypeRef::lookupType( ParseData *pd )
{
	if ( uniqueType != 0 )
		return uniqueType;
	
	if ( iterDef != 0 )
		uniqueType = pd->findUniqueType( TYPE_ITER, iterDef );
	else if ( factor != 0 )
		uniqueType = pd->findUniqueType( TYPE_TREE, factor->langEl );
	else {
		String name = typeName;
		if ( isOpt )
			name.setAs( 32, "_opt_%s", name.data );
		else if ( isRepeat )
			name.setAs( 32, "_repeat_%s", name.data );

		/* Not an iterator. May be a reference. */
		uniqueType = lookupTypePart( pd, nspaceQual, name );
	}

	return uniqueType;
}

ObjField *ObjectDef::findField( String name )
{
	ObjFieldMapEl *objDefMapEl = objFieldMap->find( name );
	if ( objDefMapEl != 0 )
		return objDefMapEl->value;
	return 0;
}

ObjMethod *ObjectDef::findMethod( String name )
{
	ObjMethodMapEl *objMethodMapEl = objMethodMap->find( name );
	if ( objMethodMapEl != 0 )
		return objMethodMapEl->value;
	return 0;
}

long sizeOfField( UniqueType *fieldUT )
{
	long size = 0;
	if ( fieldUT->typeId == TYPE_ITER ) {
		/* Select on the iterator type. */
		if ( fieldUT->iterDef->type == IterDef::Tree ||
				fieldUT->iterDef->type == IterDef::Child ||
				fieldUT->iterDef->type == IterDef::RevChild )
			size = sizeof(TreeIter) / sizeof(Word);
		else if ( fieldUT->iterDef->type == IterDef::User ) {
			/* User iterators are just a pointer to the UserIter struct. The
			 * struct needs to go right beneath the call to the user iterator
			 * so it can be found by a yield. It is therefore allocated on the
			 * stack right before the call. */
			size = 1;
		}
		else {
			assert(false);
		}
	}
	else if ( fieldUT->typeId == TYPE_REF )
		size = 2;
	else
		size = 1;

	return size;
}

void ObjectDef::referenceField( ParseData *pd, ObjField *field )
{
	field->beenReferenced = true;
	initField( pd, field );
}

void ObjectDef::initField( ParseData *pd, ObjField *field )
{
	if ( !field->beenInitialized ) {
		field->beenInitialized = true;
		UniqueType *fieldUT = field->typeRef->lookupType( pd );

		if ( type == FrameType ) {
			nextOffset += sizeOfField( fieldUT );
			field->offset = -nextOffset;

			pd->initLocalInstructions( field );
		}
		else {
			field->offset = nextOffset;
			nextOffset += sizeOfField( fieldUT );

			/* Initialize the instructions. */
			pd->initFieldInstructions( field );
		}
	}
}

UniqueType *LangVarRef::loadFieldInstr( ParseData *pd, CodeVect &code, 
		ObjectDef *inObject, ObjField *el, bool forWriting, bool revert )
{
	/* Ensure that the field is referenced. */
	inObject->referenceField( pd, el );

	UniqueType *elUT = el->typeRef->lookupType( pd );

	/* If it's a reference then we load it read always. */
	if ( forWriting ) {
		/* The instruction, depends on whether or not we are reverting. */
		if ( elUT->typeId == TYPE_ITER )
			code.append( elUT->iterDef->inGetCurWC );
		else if ( pd->revertOn && revert )
			code.append( el->inGetWV );
		else
			code.append( el->inGetWC );
	}
	else {
		/* Loading something for writing */
		if ( elUT->typeId == TYPE_ITER )
			code.append( elUT->iterDef->inGetCurR );
		else
			code.append( el->inGetR );
	}

	if ( el->useOffset ) {
		/* Gets of locals and fields require offsets. Fake vars like token
		 * data and lhs don't require it. */
		code.appendHalf( el->offset );
	}

	/* If we are dealing with an iterator then dereference it. */
	if ( elUT->typeId == TYPE_ITER )
		elUT = el->typeRef->searchTypeRef->lookupType( pd );

	return elUT;
}

ObjectDef *objDefFromUT( ParseData *pd, UniqueType *ut )
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

void LangVarRef::loadQualification( ParseData *pd, CodeVect &code, 
		ObjectDef *rootObj, int lastPtrInQual, bool forWriting, bool revert )
{
	/* Start the search from the root object. */
	ObjectDef *searchObjDef = rootObj;

	for ( QualItemVect::Iter qi = *qual; qi.lte(); qi++ ) {
		/* Lookup the field int the current qualification. */
		ObjFieldMapEl *objDefMapEl = searchObjDef->objFieldMap->find( qi->data );
		if ( objDefMapEl == 0 )
			error(qi->loc) << "cannot resolve qualification " << qi->data << endp;
		ObjField *el = objDefMapEl->value;

		if ( forWriting && el->refActive )
			error(qi->loc) << "reference active, cannot write to object" << endp;

		bool lfForWriting = forWriting;
		bool lfRevert = revert;

		/* If there is a pointer in the qualification, we need to compute
		 * forWriting and revert. */
		if ( lastPtrInQual >= 0 ) {
			if ( qi.pos() <= lastPtrInQual ) {
				/* If we are before or at the pointer we are strictly read
				 * only, regardless of the origin. */
				lfForWriting = false;
				lfRevert = false;
			}
			else {
				/* If we are past the pointer then we are always reverting
				 * because the object is global. Forwriting is as passed in.
				 * */
				lfRevert = true;
			}
		}

		UniqueType *qualUT = loadFieldInstr( pd, code, searchObjDef, 
				el, lfForWriting, lfRevert );
		
		if ( qi->type == QualItem::Dot ) {
			/* Cannot a reference. Iterator yes (access of the iterator not
			 * hte current) */
			if ( qualUT->typeId == TYPE_PTR )
				error(loc) << "dot cannot be used to access a pointer" << endp;
		}
		else if ( qi->type == QualItem::Arrow ) {
			if ( qualUT->typeId == TYPE_PTR ) {
				/* Always dereference references when used for qualification. If
				 * this is the last one then we must start with the reverse
				 * execution business. */
				if ( pd->revertOn && qi.pos() == lastPtrInQual && forWriting ) {
					/* This is like a global load. */
					code.append( IN_PTR_DEREF_WV );
				}
				else {
					/* If reading or not yet the last in ref then we only need a
					 * reading deref. */
					code.append( IN_PTR_DEREF_R );
				}

				qualUT = pd->findUniqueType( TYPE_TREE, qualUT->langEl );
			}
			else {
				error(loc) << "arrow operator cannot be used to access this type" << endp;
			}
		}

		searchObjDef = objDefFromUT( pd, qualUT );
	}
}

void LangVarRef::loadGlobalObj( ParseData *pd, CodeVect &code, 
		int lastPtrInQual, bool forWriting )
{
	/* Start the search in the global object. */
	ObjectDef *rootObj = pd->globalObjectDef;

	if ( forWriting && lastPtrInQual < 0 ) {
		/* If we are writing an no reference was found in the qualification
		 * then load the gloabl with a revert. */
		if ( pd->revertOn )
			code.append( IN_LOAD_GLOBAL_WV );
		else
			code.append( IN_LOAD_GLOBAL_WC );
	}
	else {
		/* Either we are reading or we are loading a pointer that will be
		 * dereferenced. */
		code.append( IN_LOAD_GLOBAL_R );
	}

	loadQualification( pd, code, rootObj, lastPtrInQual, forWriting, true );
}

void LangVarRef::loadLocalObj( ParseData *pd, CodeVect &code, 
		int lastPtrInQual, bool forWriting )
{
	/* Start the search in the local frame. */
	ObjectDef *rootObj = pd->curLocalFrame;
	loadQualification( pd, code, rootObj, lastPtrInQual, forWriting, false );
}

bool LangVarRef::isLocalRef( ParseData *pd )
{
	if ( qual->length() > 0 ) {
		if ( pd->curLocalFrame->objFieldMap->find( qual->data[0].data ) != 0 )
			return true;
	}
	else if ( pd->curLocalFrame->objFieldMap->find( name ) != 0 )
		return true;
	else if ( pd->curLocalFrame->objMethodMap->find( name ) != 0 )
		return true;

	return false;
}

void LangVarRef::loadObj( ParseData *pd, CodeVect &code, 
		int lastPtrInQual, bool forWriting )
{
	if ( isLocalRef( pd ) )
		loadLocalObj( pd, code, lastPtrInQual, forWriting );
	else
		loadGlobalObj( pd, code, lastPtrInQual, forWriting );
}

VarRefLookup LangVarRef::lookupQualification( ParseData *pd, ObjectDef *rootDef ) const
{
	int lastPtrInQual = -1;
	ObjectDef *searchObjDef = rootDef;
	int firstConstPart = -1;

	for ( QualItemVect::Iter qi = *qual; qi.lte(); qi++ ) {
		/* Lookup the field int the current qualification. */
		ObjFieldMapEl *objDefMapEl = searchObjDef->objFieldMap->find( qi->data );
		if ( objDefMapEl == 0 )
			error(qi->loc) << "cannot resolve qualification " << qi->data << endp;
		ObjField *el = objDefMapEl->value;

		/* Lookup the type of the field. */
		UniqueType *qualUT = el->typeRef->lookupType( pd );

		/* If we are dealing with an iterator then dereference it. */
		if ( qualUT->typeId == TYPE_ITER )
			qualUT = el->typeRef->searchTypeRef->lookupType( pd );

		/* Is it const? */
		if ( firstConstPart < 0 && el->isConst )
			firstConstPart = qi.pos();

		/* Check for references. When loop is done we will have the last one
		 * present, if any. */
		if ( qualUT->typeId == TYPE_PTR )
			lastPtrInQual = qi.pos();

		if ( qi->type == QualItem::Dot ) {
			/* Cannot dot a reference. Iterator yes (access of the iterator
			 * not the current) */
			if ( qualUT->typeId == TYPE_PTR )
				error(loc) << "dot cannot be used to access a pointer" << endp;
		}
		else if ( qi->type == QualItem::Arrow ) {
			if ( qualUT->typeId == TYPE_ITER )
				qualUT = el->typeRef->searchTypeRef->lookupType( pd );
			else if ( qualUT->typeId == TYPE_PTR )
				qualUT = pd->findUniqueType( TYPE_TREE, qualUT->langEl );
		}

		searchObjDef = objDefFromUT( pd, qualUT );
	}

	return VarRefLookup( lastPtrInQual, firstConstPart, searchObjDef );
}

VarRefLookup LangVarRef::lookupObj( ParseData *pd )
{
	ObjectDef *rootDef;
	if ( isLocalRef( pd ) )
		rootDef = pd->curLocalFrame;
	else
		rootDef = pd->globalObjectDef;

	return lookupQualification( pd, rootDef );
}

VarRefLookup LangVarRef::lookupField( ParseData *pd )
{
	/* Lookup the object that the field is in. */
	VarRefLookup lookup = lookupObj( pd );

	/* Lookup the field. */
	ObjFieldMapEl *objDefMapEl = lookup.inObject->objFieldMap->find( name );
	if ( objDefMapEl == 0 )
		error(loc) << "cannot find name " << name << " in object" << endp;

	ObjField *field = objDefMapEl->value;

	lookup.objField = field;
	lookup.uniqueType = field->typeRef->lookupType( pd );

	if ( field->typeRef->searchTypeRef != 0 )
		lookup.iterSearchUT = field->typeRef->searchTypeRef->lookupType( pd );

	return lookup;
}

VarRefLookup LangVarRef::lookupMethod( ParseData *pd )
{
	/* Lookup the object that the field is in. */
	VarRefLookup lookup = lookupObj( pd );

	/* Find the method. */
	assert( lookup.inObject->objMethodMap != 0 );
	ObjMethod *method = lookup.inObject->findMethod( name );
	if ( method == 0 )
		error(loc) << "cannot find " << name << "(...) in object" << endp;
	
	lookup.objMethod = method;
	lookup.uniqueType = method->returnUT;
	
	return lookup;
}

void LangVarRef::setFieldInstr( ParseData *pd, CodeVect &code, 
		ObjectDef *inObject, ObjField *el, UniqueType *exprUT, bool revert )
{
	/* Ensure that the field is referenced. */
	inObject->referenceField( pd, el );

	if ( pd->revertOn && revert )
		code.append( el->inSetWV );
	else
		code.append( el->inSetWC );

	/* Maybe write out an offset. */
	if ( el->useOffset )
		code.appendHalf( el->offset );
}

bool castAssignment( ParseData *pd, CodeVect &code, UniqueType *destUT, 
		UniqueType *destSearchUT, UniqueType *srcUT )
{
	if ( destUT == srcUT )
		return true;

	/* Casting trees to any. */
	if ( destUT->typeId == TYPE_TREE && destUT->langEl == pd->anyKlangEl &&
			srcUT->typeId == TYPE_TREE )
		return true;
	
	/* Setting a reference from a tree. */
	if ( destUT->typeId == TYPE_REF && srcUT->typeId == TYPE_TREE &&
			destUT->langEl == srcUT->langEl )
		return true;
	
	/* Setting an iterator from a tree. */
	if ( destUT->typeId == TYPE_ITER && srcUT->typeId == TYPE_TREE && 
			destSearchUT->langEl == srcUT->langEl )
		return true;
	
	/* Assigning nil to a tree. */
	if ( destUT->typeId == TYPE_TREE && srcUT->typeId == TYPE_NIL )
		return true;

	/* Assigning nil to a pointer. */
	if ( destUT->typeId == TYPE_PTR && srcUT->typeId == TYPE_NIL )
		return true;

	return false;
}

void LangVarRef::setField( ParseData *pd, CodeVect &code, 
		ObjectDef *inObject, UniqueType *exprUT, bool revert )
{
	ObjFieldMapEl *objDefMapEl = inObject->objFieldMap->find( name );
	if ( objDefMapEl == 0 )
		error(loc) << "cannot find name " << name << " in object" << endp;

	ObjField *el = objDefMapEl->value;
	setFieldInstr( pd, code, inObject, el, exprUT, revert );
}

void LangVarRef::setFieldIter( ParseData *pd, CodeVect &code, 
		ObjectDef *inObject, UniqueType *objUT, UniqueType *exprType, bool revert )
{
	ObjFieldMapEl *objDefMapEl = inObject->objFieldMap->find( name );
	if ( objDefMapEl == 0 )
		error(loc) << "cannot find name " << name << " in object" << endp;

	ObjField *el = objDefMapEl->value;
	code.append( objUT->iterDef->inSetCurWC );
	code.appendHalf( el->offset );
}

UniqueType *LangVarRef::evaluate( ParseData *pd, CodeVect &code, bool forWriting )
{
	/* Lookup the loadObj. */
	VarRefLookup lookup = lookupField( pd );

	/* Load the object, if any. */
	loadObj( pd, code, lookup.lastPtrInQual, forWriting );

	/* Load the field. */
	UniqueType *ut = loadFieldInstr( pd, code, lookup.inObject, 
			lookup.objField, forWriting, false );

	return ut;
}

/* Return the field referenced. */
ObjField *LangVarRef::evaluateRef( ParseData *pd, CodeVect &code )
{
	/* Lookup the loadObj. */
	VarRefLookup lookup = lookupField( pd );

	if ( lookup.inObject->type != ObjectDef::FrameType )
		error(loc) << "can only take references of local variables" << endl;
	
	if ( lookup.objField->refActive )
		error(loc) << "reference current active, cannot take another" << endl;

	/* Ensure that the field is referenced. */
	lookup.inObject->referenceField( pd, lookup.objField );

	/* Note that we could have modified children. */
	lookup.objField->refActive = true;

	if ( lookup.objField->typeRef->iterDef != 0 ) {
		code.append( lookup.objField->typeRef->iterDef->inRefFromCur );
		code.appendHalf( lookup.objField->offset );
	}
	else if ( lookup.objField->typeRef->isRef ) {
		code.append( IN_REF_FROM_REF );
		code.appendHalf( lookup.objField->offset );
	}
	else {
		code.append( IN_REF_FROM_LOCAL );
		code.appendHalf( lookup.objField->offset );
	}

	return lookup.objField;
}

ObjField **LangVarRef::evaluateArgs( ParseData *pd, CodeVect &code, 
		VarRefLookup &lookup, ExprVect *args )
{
	/* Parameter list is given only for user defined methods. Otherwise it
	 * will be null. */
	ParameterList *paramList = lookup.objMethod->paramList;

	/* Match the number of arguments. */
	int numArgs = args != 0 ? args->length() : 0;
	if ( numArgs != lookup.objMethod->numParams )
		error(loc) << "wrong number of arguments" << endp;

	/* This is for storing the object fields used by references. */
	ObjField **paramRefs = new ObjField*[numArgs];
	memset( paramRefs, 0, sizeof(ObjField*) * numArgs );

	/* Evaluate and push the args. */
	if ( args != 0 ) {
		/* If we have the parameter list, initialize an iterator. */
		ParameterList::Iter p;
		paramList != 0 && ( p = *paramList );

		for ( ExprVect::Iter pe = *args; pe.lte(); pe++ ) {
			/* Get the expression and the UT for the arg. */
			LangExpr *expression = *pe;
			UniqueType *paramUT = lookup.objMethod->paramUTs[pe.pos()];

			if ( paramUT->typeId == TYPE_REF ) {
				/* Make sure we are dealing with a variable reference. */
				if ( expression->type != LangExpr::TermType )
					error(loc) << "not a term: argument must be a local variable" << endp;
				if ( expression->term->type != LangTerm::VarRefType )
					error(loc) << "not a variable: argument must be a local variable" << endp;

				/* Lookup the field. */
				LangVarRef *varRef = expression->term->varRef;

				ObjField *refOf = varRef->evaluateRef( pd, code );
				paramRefs[pe.pos()] = refOf;
			}
			else {
				UniqueType *exprUT = expression->evaluate( pd, code );

				if ( !castAssignment( pd, code, paramUT, 0, exprUT ) )
					error(loc) << "arg " << pe.pos()+1 << " is of the wrong type" << endp;
			}

			/* Advance the parameter list iterator if we have it. */
			paramList != 0 && p.increment();
		}
	}

	return paramRefs;
}

void LangVarRef::resetActiveRefs( ParseData *pd, VarRefLookup &lookup, ObjField **paramRefs )
{
	/* Parameter list is given only for user defined methods. Otherwise it
	 * will be null. */
	for ( long p = 0; p < lookup.objMethod->numParams; p++ ) {
		if ( paramRefs[p] != 0 )
			paramRefs[p]->refActive = false;
	}
}


void LangVarRef::callOperation( ParseData *pd, CodeVect &code, VarRefLookup &lookup )
{
	/* This is for writing if it is a non-const builtin. */
	bool forWriting = lookup.objMethod->func == 0 && 
			!lookup.objMethod->isConst;

	if ( lookup.objMethod->useCallObj ) {
		/* Load the object, if any. */
		loadObj( pd, code, lookup.lastPtrInQual, forWriting );
	}

	/* Check if we need to revert the function. If it operates on a reference
	 * or if it is not local then we need to revert it. */
	bool revert = lookup.lastPtrInQual >= 0 || !isLocalRef(pd);
	
	/* The call instruction. */
	if ( pd->revertOn && revert ) 
		code.append( lookup.objMethod->opcodeWV );
	else
		code.append( lookup.objMethod->opcodeWC );
	
	if ( lookup.objMethod->useFuncId )
		code.appendHalf( lookup.objMethod->funcId );
}

UniqueType *LangVarRef::evaluateCall( ParseData *pd, CodeVect &code, ExprVect *args )
{
	/* Evaluate the object. */
	VarRefLookup lookup = lookupMethod( pd );

	/* Evaluate and push the arguments. */
	ObjField **paramRefs = evaluateArgs( pd, code, lookup, args );

	/* Write the call opcode. */
	callOperation( pd, code, lookup );

	resetActiveRefs( pd, lookup, paramRefs );
	delete[] paramRefs;

	/* Return the type to the expression. */
	return lookup.uniqueType;
}

UniqueType *LangTerm::evaluateMatch( ParseData *pd, CodeVect &code )
{
	/* Add the vars bound by the pattern into the local scope. */
	for ( PatternItemList::Iter item = *pattern->list; item.lte(); item++ ) {
		if ( item->varRef != 0 )
			item->bindId = pattern->nextBindId++;
	}

	UniqueType *ut = varRef->evaluate( pd, code );
	if ( ut->typeId != TYPE_TREE )
		error(varRef->loc) << "expected match against a tree type" << endp;

	/* Store the language element type in the pattern. This is needed by
	 * the pattern parser. */
	pattern->langEl = ut->langEl;

	code.append( IN_MATCH );
	code.appendHalf( pattern->patRepId );

	for ( PatternItemList::Iter item = pattern->list->last(); item.gtb(); item-- ) {
		if ( item->varRef != 0 ) {
			/* Compute the unique type. */
			UniqueType *exprType = pd->findUniqueType( TYPE_TREE, item->factor->langEl );

			/* Get the type of the variable being assigned to. */
			VarRefLookup lookup = item->varRef->lookupField( pd );

			item->varRef->loadObj( pd, code, lookup.lastPtrInQual, false );
			item->varRef->setField( pd, code, lookup.inObject, exprType, false );
		}
	}

	return ut;
}

UniqueType *LangTerm::evaluateNew( ParseData *pd, CodeVect &code )
{
	/* Evaluate the expression. */
	UniqueType *ut = expr->evaluate( pd, code );
	if ( ut->typeId != TYPE_TREE )
		error() << "new can only be applied to tree types" << endp;

	code.append( IN_TREE_NEW );
	return pd->findUniqueType( TYPE_PTR, ut->langEl );
}

void LangTerm::assignFieldArgs( ParseData *pd, CodeVect &code, UniqueType *replUT )
{
	/* Now assign the field initializations. Note that we need to do this in
	 * reverse because the last expression evaluated is at the top of the
	 * stack. */
	if ( fieldInitArgs != 0 && fieldInitArgs->length() > 0 ) {
		ObjectDef *objDef = objDefFromUT( pd, replUT );
		/* Note the reverse traversal. */
		for ( FieldInitVect::Iter pi = fieldInitArgs->last(); pi.gtb(); pi-- ) {
			FieldInit *fieldInit = *pi;
			ObjFieldMapEl *el = objDef->objFieldMap->find( fieldInit->name );
			if ( el == 0 ) {
				error(fieldInit->loc) << "failed to find init name " << 
					fieldInit->name << " in object" << endp;
			}

			/* Lookup the type of the field and compare it to the type of the
			 * expression. */
			ObjField *field = el->value;
			UniqueType *fieldUT = field->typeRef->lookupType( pd );
			if ( !castAssignment( pd, code, fieldUT, 0, fieldInit->exprUT ) )
				error(fieldInit->loc) << "type mismatch in initialization" << endp;

			/* The set field instruction must leave the object on the top of
			 * the stack. */
			code.append( IN_SET_FIELD_LEAVE_WC );
			code.appendHalf( field->offset );
		}
	}
}

UniqueType *LangTerm::evaluateTreeConstruct( ParseData *pd, CodeVect &code )
{
	/* Evaluate the initialization expressions. */
	if ( fieldInitArgs != 0 && fieldInitArgs->length() > 0 ) {
		for ( FieldInitVect::Iter pi = *fieldInitArgs; pi.lte(); pi++ ) {
			FieldInit *fieldInit = *pi;
			fieldInit->exprUT = fieldInit->expr->evaluate( pd, code );
		}
	}

	/* Assign bind ids to the variables in the replacement. */
	for ( ReplItemList::Iter item = *replacement->list; item.lte(); item++ ) {
		if ( item->varRef != 0 )
			item->bindId = replacement->nextBindId++;
	}

	/* Evaluate variable references. */
	for ( ReplItemList::Iter item = replacement->list->last(); item.gtb(); item-- ) {
		if ( item->type == ReplItem::VarRefType ) {
			UniqueType *ut = item->varRef->evaluate( pd, code );
		
			if ( ut->typeId != TYPE_TREE )
				error() << "variables used in replacements must be trees" << endp;

			item->langEl = ut->langEl;
		}
	}

	/* Construct the tree using the tree information stored in the compiled
	 * code. */
	code.append( IN_CONSTRUCT );
	code.appendHalf( replacement->patRepId );

	/* Lookup the type of the replacement and store it in the replacement
	 * object so that replacement parsing has a target. */
	UniqueType *replUT = typeRef->lookupType( pd );
	if ( replUT->typeId == TYPE_TREE )
		replacement->langEl = replUT->langEl;
	else
		error(loc) << "don't know how to construct this type" << endp;

	assignFieldArgs( pd, code, replUT );

	return replUT;
}


UniqueType *LangTerm::evaluateTermConstruct( ParseData *pd, CodeVect &code )
{
	/* Going to make this replacement directly. Take it out of the list of
	 * replacements so that we don't try to parse it. */
	pd->replList.remove( replacement );

	/* Evaluate the initialization expressions. */
	if ( fieldInitArgs != 0 && fieldInitArgs->length() > 0 ) {
		for ( FieldInitVect::Iter pi = *fieldInitArgs; pi.lte(); pi++ ) {
			FieldInit *fieldInit = *pi;
			fieldInit->exprUT = fieldInit->expr->evaluate( pd, code );
		}
	}

	UniqueType *replUT = typeRef->lookupType( pd );

	/* Evaluate the expression that we are constructing the term with and make
	 * the term. */
	ReplItem *replItem = replacement->list->head;
	replItem->varRef->evaluate( pd, code );
	code.append( IN_CONSTRUCT_TERM );
	code.appendHalf( replUT->langEl->id );

	assignFieldArgs( pd, code, replUT );
	return replUT;
}

bool LangTerm::constructTermFromString( ParseData *pd )
{
	UniqueType *replUT = typeRef->lookupType( pd );
	if ( replUT->typeId == TYPE_TREE && replUT->langEl->id < pd->firstNonTermId ) {
		if ( replacement->list->length() == 1 ) {
			ReplItem *replItem = replacement->list->head;
			if ( replItem->type == ReplItem::VarRefType ) {
				VarRefLookup lookup = replItem->varRef->lookupField( pd );
				if ( lookup.uniqueType == pd->uniqueTypeStr )
					return true;
			}
		}
	}
	return false;
}

UniqueType *LangTerm::evaluateConstruct( ParseData *pd, CodeVect &code )
{
	/* If the type is a token and the replacement contains just a string then
	 * construct a token using the text of the string. Otherwise do a normal
	 * tree construct. */
	if ( constructTermFromString( pd ) )
		return evaluateTermConstruct( pd, code );
	else
		return evaluateTreeConstruct( pd, code );
}

UniqueType *LangTerm::evaluateParse( ParseData *pd, CodeVect &code, bool stop )
{
	UniqueType *ut = typeRef->lookupType( pd );
	if ( ut->typeId != TYPE_TREE )
		error(loc) << "can only parse trees" << endl;
	
	/* Should be one arg, a stream. */
	if ( args == 0 || args->length() != 1 )
		error(loc) << "expecting one argument" << endp;

	UniqueType *argUT = args->data[0]->evaluate( pd, code );
	if ( argUT != pd->uniqueTypeStream )
		error(loc) << "single argument must be a stream" << endp;

	/* Allocate a parser id. This will cause a parser to be built for
	 * the type. */
	ut->langEl->parserId = pd->nextParserId++;

	code.append( IN_PARSE );
	code.appendHalf( ut->langEl->parserId );
	if ( stop )
		code.appendHalf( ut->langEl->id );
	else 
		code.appendHalf( 0 );
	code.append( pd->revertOn );
	return ut;
}

UniqueType *LangTerm::evaluate( ParseData *pd, CodeVect &code )
{
	switch ( type ) {
		case VarRefType:
			return varRef->evaluate( pd, code );
		case MethodCallType:
			return varRef->evaluateCall( pd, code, args );
		case NilType:
			code.append( IN_LOAD_NIL );
			return pd->uniqueTypeNil;
		case TrueType:
			code.append( IN_LOAD_TRUE );
			return pd->uniqueTypeBool;
		case FalseType:
			code.append( IN_LOAD_FALSE );
			return pd->uniqueTypeBool;
		case MakeTokenType:
			return evaluateMakeToken( pd, code );
		case MakeTreeType:
			return evaluateMakeTree( pd, code );
		case NumberType: {
			unsigned int n = atoi( data );
			code.append( IN_LOAD_INT );
			code.appendWord( n );
			return pd->uniqueTypeInt;
		}
		case StringType: {
			String interp;
			bool unused;
			prepareLitString( interp, unused, data, InputLoc() );

			/* Make sure we have this string. */
			StringMapEl *mapEl = 0;
			if ( pd->literalStrings.insert( interp, &mapEl ) )
				mapEl->value = pd->literalStrings.length()-1;

			code.append( IN_LOAD_STR );
			code.appendWord( mapEl->value );
			return pd->uniqueTypeStr;
		}
		case MatchType:
			return evaluateMatch( pd, code );
		case ParseType:
			return evaluateParse( pd, code, false );
		case ParseStopType:
			return evaluateParse( pd, code, true );
		case ConstructType:
			return evaluateConstruct( pd, code );
		case NewType:
			return evaluateNew( pd, code );
		case TypeIdType: {
			/* Evaluate the expression. */
			UniqueType *ut = typeRef->lookupType( pd );
			if ( ut->typeId != TYPE_TREE )
				error() << "typeid can only be applied to tree types" << endp;

			code.append( IN_LOAD_INT );
			code.appendWord( ut->langEl->id );
			return pd->uniqueTypeInt;
		}
		case SearchType: {
			/* Evaluate the expression. */
			UniqueType *ut = typeRef->lookupType( pd );
			if ( ut->typeId != TYPE_TREE )
				error(loc) << "can only search for tree types" << endp;

			UniqueType *treeUT = varRef->evaluate( pd, code );
			if ( treeUT->typeId != TYPE_TREE )
				error(loc) << "search can be applied only to tree types" << endl;

			code.append( IN_TREE_SEARCH );
			code.appendWord( ut->langEl->id );
			return ut;
		};
	}
	return 0;
}

UniqueType *LangExpr::evaluate( ParseData *pd, CodeVect &code )
{
	switch ( type ) {
		case BinaryType: {
			switch ( op ) {
				case '+': {
					UniqueType *lt = left->evaluate( pd, code );
					UniqueType *rt = right->evaluate( pd, code );

					if ( lt == pd->uniqueTypeInt && rt == pd->uniqueTypeInt ) {
						code.append( IN_ADD_INT );
						return pd->uniqueTypeInt;
					}

					if ( lt == pd->uniqueTypeStr && rt == pd->uniqueTypeStr ) {
						code.append( IN_CONCAT_STR );
						return pd->uniqueTypeStr;
					}

					error(loc) << "do not have an addition operator for these types" << endp;
					break;
				}
				case '-': {
					UniqueType *lt = left->evaluate( pd, code );
					UniqueType *rt = right->evaluate( pd, code );

					if ( lt == pd->uniqueTypeInt && rt == pd->uniqueTypeInt ) {
						code.append( IN_SUB_INT );
						return pd->uniqueTypeInt;
					}

					error(loc) << "do not have an addition operator for these types" << endp;
					break;
				}
				case '*': {
					UniqueType *lt = left->evaluate( pd, code );
					UniqueType *rt = right->evaluate( pd, code );

					if ( lt == pd->uniqueTypeInt && rt == pd->uniqueTypeInt ) {
						code.append( IN_MULT_INT );
						return pd->uniqueTypeInt;
					}

					error(loc) << "do not have an multiplication "
							"operator for these types" << endp;
					break;
				}
				case OP_DoubleEql: {
					UniqueType *lt = left->evaluate( pd, code );
					UniqueType *rt = right->evaluate( pd, code );

					if ( lt != rt )
						error(loc) << "comparison of different types" << endp;
						
					code.append( IN_TST_EQL );
					return pd->uniqueTypeBool;
				}
				case OP_NotEql: {
					UniqueType *lt = left->evaluate( pd, code );
					UniqueType *rt = right->evaluate( pd, code );

					if ( lt != rt )
						error(loc) << "comparison of different types" << endp;

					code.append( IN_TST_NOT_EQL );
					return pd->uniqueTypeBool;
				}
				case '<': {
					left->evaluate( pd, code );
					right->evaluate( pd, code );

					code.append( IN_TST_LESS );
					return pd->uniqueTypeBool;
				}
				case '>': {
					left->evaluate( pd, code );
					right->evaluate( pd, code );

					code.append( IN_TST_GRTR );
					return pd->uniqueTypeBool;
				}
				case OP_LessEql: {
					left->evaluate( pd, code );
					right->evaluate( pd, code );

					code.append( IN_TST_LESS_EQL );
					return pd->uniqueTypeBool;
				}
				case OP_GrtrEql: {
					left->evaluate( pd, code );
					right->evaluate( pd, code );

					code.append( IN_TST_GRTR_EQL );
					return pd->uniqueTypeBool;
				}
				case OP_LogicalAnd: {
					/* Evaluate the left and duplicate it. */
					left->evaluate( pd, code );
					code.append( IN_DUP_TOP );

					/* Jump over the right if false, leaving the original left
					 * result on the top of the stack. We don't know the
					 * distance yet so record the position of the jump. */
					long jump = code.length();
					code.append( IN_JMP_FALSE );
					code.appendHalf( 0 );

					/* Evauluate the right, add the test. Store it separately. */
					right->evaluate( pd, code );
					code.append( IN_TST_LOGICAL_AND );

					/* Set the distance of the jump. */
					long distance = code.length() - jump - 3;
					code.setHalf( jump+1, distance );

					return pd->uniqueTypeInt;
				}
				case OP_LogicalOr: {
					/* Evaluate the left and duplicate it. */
					left->evaluate( pd, code );
					code.append( IN_DUP_TOP );

					/* Jump over the right if true, leaving the original left
					 * result on the top of the stack. We don't know the
					 * distance yet so record the position of the jump. */
					long jump = code.length();
					code.append( IN_JMP_TRUE );
					code.appendHalf( 0 );

					/* Evauluate the right, add the test. */
					right->evaluate( pd, code );
					code.append( IN_TST_LOGICAL_OR );

					/* Set the distance of the jump. */
					long distance = code.length() - jump - 3;
					code.setHalf( jump+1, distance );

					return pd->uniqueTypeInt;
				}
			}

			assert(false);
			return 0;
		}
		case UnaryType: {
			switch ( op ) {
				case '!': {
					/* Evaluate the left and duplicate it. */
					right->evaluate( pd, code );
					code.append( IN_NOT );
					return pd->uniqueTypeBool;
				}
				case OP_Deref: {
					UniqueType *ut = right->evaluate( pd, code );
					if ( ut->typeId != TYPE_PTR )
						error(loc) << "can only dereference pointers" << endl;

					code.append( IN_PTR_DEREF_R );
					ut = pd->findUniqueType( TYPE_TREE, ut->langEl );
					return ut;
				}
				default: 
					assert(false);
			}
			return 0;
		}
		case TermType: {
			return term->evaluate( pd, code );
		}
	}
	return 0;
}

void LangVarRef::assignValue( ParseData *pd, CodeVect &code, 
		UniqueType *exprUT )
{
	/* Lookup the left hand side of the assignment. */
	VarRefLookup lookup = lookupField( pd );

	if ( lookup.objField->refActive )
		error(loc) << "reference active, cannot write to object" << endp;

	if ( lookup.firstConstPart >= 0 ) {
		error(loc) << "left hand side qualification \"" <<
			qual->data[lookup.firstConstPart].data << "\" is const" << endp;
	}

	if ( lookup.objField->isConst )
		error(loc) << "field \"" << name << "\" is const" << endp;

	/* Check the types of the assignment and possibly cast. */
	UniqueType *objUT = lookup.objField->typeRef->lookupType( pd );
	assert( lookup.uniqueType == lookup.objField->typeRef->lookupType( pd ) );
	if ( !castAssignment( pd, code, objUT, lookup.iterSearchUT, exprUT ) )
		error(loc) << "type mismatch in assignment" << endp;
	
	/* Decide if we need to revert the assignment. */
	bool revert = lookup.lastPtrInQual >= 0 || !isLocalRef(pd);

	/* Load the object and generate the field setting code. */
	loadObj( pd, code, lookup.lastPtrInQual, true );

	if ( lookup.uniqueType->typeId == TYPE_ITER )
		setFieldIter( pd, code, lookup.inObject, lookup.uniqueType, exprUT, false );
	else
		setField( pd, code, lookup.inObject, exprUT, revert );
}

UniqueType *LangTerm::evaluateMakeToken( ParseData *pd, CodeVect &code )
{
//	if ( pd->compileContext != ParseData::CompileTranslation )
//		error(loc) << "make_token can be used only in a translation block" << endp;

	/* Match the number of arguments. */
	int numArgs = args != 0 ? args->length() : 0;
	if ( numArgs < 2 )
		error(loc) << "need at least two arguments" << endp;

	for ( ExprVect::Iter pe = *args; pe.lte(); pe++ ) {
		/* Evaluate. */
		UniqueType *exprUT = (*pe)->evaluate( pd, code );

		if ( pe.pos() == 0 && exprUT != pd->uniqueTypeInt )
			error(loc) << "first arg, id, must be an int" << endp;

		if ( pe.pos() == 1 && exprUT != pd->uniqueTypeStr )
			error(loc) << "second arg, length, must be a string" << endp;
	}

	/* The token is now created, send it. */
	code.append( IN_MAKE_TOKEN );
	code.append( args->length() );

	return pd->uniqueTypeAny;
}

UniqueType *LangTerm::evaluateMakeTree( ParseData *pd, CodeVect &code )
{
	if ( pd->compileContext != ParseData::CompileTranslation )
		error(loc) << "make_tree can be used only in a translation block" << endp;

	/* Match the number of arguments. */
	int numArgs = args != 0 ? args->length() : 0;
	if ( numArgs < 1 )
		error(loc) << "need at least one argument" << endp;

	for ( ExprVect::Iter pe = *args; pe.lte(); pe++ ) {
		/* Evaluate. */
		UniqueType *exprUT = (*pe)->evaluate( pd, code );

		if ( pe.pos() == 0 && exprUT != pd->uniqueTypeInt )
			error(loc) << "first arg, nonterm id, must be an int" << endp;
	}

	/* The token is now created, send it. */
	code.append( IN_MAKE_TREE );
	code.append( args->length() );

	return pd->uniqueTypeAny;
}

void LangStmt::compileForIterBody( ParseData *pd, CodeVect &code, 
		ObjField *iterObjField, LangVarRef *iterVarRef, 
		UniqueType *iterUT )
{
	/* Remember the top of the loop. */
	long top = code.length();

	/* Advance */
	code.append( iterUT->iterDef->inAdvance );
	code.appendHalf( iterObjField->offset );

	/* Test: jump past the while block if false. Note that we don't have the
	 * distance yet. */
	long jumpFalse = code.length();
	code.append( IN_JMP_FALSE );
	code.appendHalf( 0 );

	/*
	 * Set up the loop cleanup code. 
	 */

	/* Set up the current loop cleanup. */
	CodeVect loopCleanup;
	if ( pd->loopCleanup != 0 )
		loopCleanup.setAs( *pd->loopCleanup );

	/* Add the cleanup for the current loop. */
	loopCleanup.append( iterUT->iterDef->inDestroy );
	loopCleanup.appendHalf( iterObjField->offset );

	/* Push the loop cleanup. */
	CodeVect *oldLoopCleanup = pd->loopCleanup;
	pd->loopCleanup = &loopCleanup;

	/* Compile the contents. */
	for ( StmtList::Iter stmt = *stmtList; stmt.lte(); stmt++ )
		stmt->compile( pd, code );

	pd->loopCleanup = oldLoopCleanup;

	/* Jump back to the top to retest. */
	long retestDist = code.length() - top + 3;
	code.append( IN_JMP );
	code.appendHalf( -retestDist );

	/* Set the jump false distance. */
	long falseDist = code.length() - jumpFalse - 3;
	code.setHalf( jumpFalse+1, falseDist );

	/* Compute the jump distance for the break jumps. */
	for ( LongVect::Iter brk = pd->breakJumps; brk.lte(); brk++ ) {
		long distance = code.length() - *brk - 3;
		code.setHalf( *brk+1, distance );
	}
	pd->breakJumps.empty();

	/* Destroy the iterator. */
	code.append( iterUT->iterDef->inDestroy );
	code.appendHalf( iterObjField->offset );

	unscopeIterVariable( pd, iterObjField );
}

ObjField *LangStmt::createIterVariable( ParseData *pd, TypeRef *iterTypeRef )
{
	/* Check for redeclaration. */
	if ( pd->curLocalFrame->objFieldMap->find( name ) != 0 )
		error(loc) << "variable " << name << " redeclared" << endp;

	/* Create the field and insert it into the field map. */
	ObjField *iterObjField = new ObjField( loc, iterTypeRef, name );
	pd->curLocalFrame->objFieldMap->insert( name, iterObjField );
	pd->curLocalFrame->initField( pd, iterObjField );
	return iterObjField;
}

void LangStmt::unscopeIterVariable( ParseData *pd, ObjField *iterObjField )
{
	pd->curLocalFrame->objFieldMap->detach( name );
}

LangTerm *LangStmt::chooseDefaultIter( ParseData *pd )
{
	/* Lookup the lang term and decide what iterator to use based
	 * on its type. */
	VarRefLookup lookup = langTerm->varRef->lookupField( pd );
	
	if ( lookup.inObject->type != ObjectDef::FrameType )
		error(loc) << "root of iteration must be a local" << endp;
	
	LangVarRef *callVarRef = 0;
	if ( lookup.uniqueType->typeId == TYPE_TREE || 
			lookup.uniqueType->typeId == TYPE_REF ||
			lookup.uniqueType->typeId == TYPE_ITER ||
			lookup.uniqueType->typeId == TYPE_PTR )
	{
		/* The iterator name. */
		callVarRef = new LangVarRef( loc, new QualItemVect, "triter" );
	}
	else {
		error(loc) << "there is no default iterator for a "
				"root of that type" << endp;
	}

	/* The parameters. */
	ExprVect *callExprVect = new ExprVect;
	LangExpr *callExpr = new LangExpr( new LangTerm( 
			LangTerm::VarRefType, langTerm->varRef ) );
	callExprVect->append( callExpr );

	LangTerm *callLangTerm = new LangTerm( callVarRef, callExprVect );

	return callLangTerm;
}

void LangStmt::compileForIter( ParseData *pd, CodeVect &code )
{
	if ( langTerm->type != LangTerm::MethodCallType )
		langTerm = chooseDefaultIter( pd );

	/* The type we are searching for. */
	UniqueType *searchUT = typeRef->lookupType( pd );

	/* 
	 * Declare the iterator variable.
	 */
	VarRefLookup lookup = langTerm->varRef->lookupMethod( pd );
	if ( lookup.objMethod->iterDef == 0 ) {
		error(loc) << "attempt to iterate using something "
				"that is not an iterator" << endp;
	}

	/* Type ref and object field for the iterator. */
	TypeRef *iterTypeRef = new TypeRef( loc, lookup.objMethod->iterDef, typeRef );
	ObjField *iterObjField = createIterVariable( pd, iterTypeRef );

	/* 
	 * Create the iterator from the local var.
	 */

	LangVarRef *iterVarRef = new LangVarRef( loc, new QualItemVect, name ); 
	UniqueType *iterUT = iterTypeRef->lookupType( pd );

	/* Evaluate and push the arguments. */
	ObjField **paramRefs = langTerm->varRef->evaluateArgs( 
			pd, code, lookup, langTerm->args );

	code.append( iterUT->iterDef->inCreate );
	code.appendHalf( iterObjField->offset );
	if ( lookup.objMethod->func != 0 )
		code.appendHalf( lookup.objMethod->func->funcId );

	if ( iterUT->iterDef->useSearchUT ) {
		if ( searchUT->typeId == TYPE_PTR )
			code.appendHalf( pd->uniqueTypePtr->langEl->id );
		else
			code.appendHalf( searchUT->langEl->id );
	}

	compileForIterBody( pd, code, iterObjField, iterVarRef, iterUT );

	langTerm->varRef->resetActiveRefs( pd, lookup, paramRefs );
	delete[] paramRefs;
}

void LangStmt::compileWhile( ParseData *pd, CodeVect &code )
{
	/* Generate code for the while test. Remember the top. */
	long top = code.length();
	expr->evaluate( pd, code );

	/* Jump past the while block if false. Note that we don't have the
	 * distance yet. */
	long jumpFalse = code.length();
	code.append( IN_JMP_FALSE );
	code.appendHalf( 0 );

	/* Compute the while block. */
	for ( StmtList::Iter stmt = *stmtList; stmt.lte(); stmt++ )
		stmt->compile( pd, code );

	/* Jump back to the top to retest. */
	long retestDist = code.length() - top + 3;
	code.append( IN_JMP );
	code.appendHalf( -retestDist );

	/* Set the jump false distance. */
	long falseDist = code.length() - jumpFalse - 3;
	code.setHalf( jumpFalse+1, falseDist );

	/* Compute the jump distance for the break jumps. */
	for ( LongVect::Iter brk = pd->breakJumps; brk.lte(); brk++ ) {
		long distance = code.length() - *brk - 3;
		code.setHalf( *brk+1, distance );
	}
	pd->breakJumps.empty();
}

void LangStmt::compile( ParseData *pd, CodeVect &code )
{
	switch ( type ) {
		case PrintType: 
		case PrintXMLType: {
			UniqueType **types = new UniqueType*[exprPtrVect->length()];
			
			/* Push the args backwards. */
			for ( ExprVect::Iter pex = exprPtrVect->last(); pex.gtb(); pex-- )
				types[pex.pos()] = (*pex)->evaluate( pd, code );

			/* Run the printing forwards. */
			if ( type == PrintType ) {
				for ( ExprVect::Iter pex = *exprPtrVect; pex.lte(); pex++ )
					code.append( IN_PRINT );
			}
			else {
				for ( ExprVect::Iter pex = *exprPtrVect; pex.lte(); pex++ )
					code.append( IN_PRINT_XML );
			}

			delete[] types;

			break;
		}
		case ExprType: {
			/* Evaluate the exrepssion, then pop it immediately. */
			expr->evaluate( pd, code );
			code.append( IN_POP );
			break;
		}
		case IfType: {
			long jumpFalse, jumpPastElse, distance;

			/* Evaluate the test. */
			expr->evaluate( pd, code );

			/* Jump past the if block if false. We don't know the distance
			 * yet so store the location of the jump. */
			jumpFalse = code.length();
			code.append( IN_JMP_FALSE );
			code.appendHalf( 0 );

			/* Compile the if true branch. */
			for ( StmtList::Iter stmt = *stmtList; stmt.lte(); stmt++ )
				stmt->compile( pd, code );

			if ( elsePart != 0 ) {
				/* Jump past the else code for the if true branch. */
				jumpPastElse = code.length();
				code.append( IN_JMP );
				code.appendHalf( 0 );
			}

			/* Set the distance for the jump false case. */
			distance = code.length() - jumpFalse - 3;
			code.setHalf( jumpFalse+1, distance );

			if ( elsePart != 0 ) {
				/* Compile the else branch. */
				for ( StmtList::Iter stmt = *elsePart; stmt.lte(); stmt++ )
					stmt->compile( pd, code );

				/* Set the distance for jump over the else part. */
				distance = code.length() - jumpPastElse - 3;
				code.setHalf( jumpPastElse+1, distance );
			}

			break;
		}
		case RejectType: {
			code.append( IN_REJECT );
			break;
		}
		case WhileType: {
			compileWhile( pd, code );
			break;
		}
		case AssignType: {
			/* Evaluate the exrepssion. */
			UniqueType *exprUT = expr->evaluate( pd, code );

			/* Do the assignment. */
			varRef->assignValue( pd, code, exprUT );
			break;
		}
		case ForIterType: {
			compileForIter( pd, code );
			break;
		}
		case ReturnType: {
			/* Evaluate the exrepssion. */
			UniqueType *exprUT = expr->evaluate( pd, code );

			UniqueType *resUT = pd->curFunction->typeRef->lookupType( pd );
			if ( !castAssignment( pd, code, resUT, 0, exprUT ) )
				error(loc) << "return value wrong type" << endp;

			code.append( IN_SAVE_RET );

			/* The loop cleanup code. */
			if ( pd->loopCleanup != 0 )
				code.append( *pd->loopCleanup );

			/* Jump to the return label. The distnacnce will be filled in
			 * later. */
			pd->returnJumps.append( code.length() );
			code.append( IN_JMP );
			code.appendHalf( 0 );
			break;
		}
		case BreakType: {
			pd->breakJumps.append( code.length() );
			code.append( IN_JMP );
			code.appendHalf( 0 );
			break;
		}
		case YieldType: {
			/* take a reference and yield it. Immediately reset the referece. */
			ObjField *objField = varRef->evaluateRef( pd, code );
			objField->refActive = false;
			code.append( IN_YIELD );
			break;
		}
	}
}

void CodeBlock::compile( ParseData *pd, CodeVect &code )
{
	for ( StmtList::Iter stmt = *stmtList; stmt.lte(); stmt++ )
		stmt->compile( pd, code );
}

void ParseData::addProdRedObjectVar( ObjectDef *localFrame, KlangEl *nonTerm )
{
	UniqueType *prodNameUT = findUniqueType( TYPE_TREE, nonTerm );
	TypeRef *typeRef = new TypeRef( InputLoc(), prodNameUT );
	ObjField *el = new ObjField( InputLoc(), typeRef, "lhs" );

	/* Is the only item pushed to the stack just before a reduction action is
	 * executed. We rely on a zero offset. */
	el->beenReferenced = true;
	el->beenInitialized = true;
	el->isLhsEl = true;
	el->offset = 0;

	initLocalInstructions( el );

	localFrame->objFieldMap->insert( el->name, el );
}

void ParseData::addProdRHSVars( ObjectDef *localFrame, ProdElList *prodElList )
{
	long position = 1;
	for ( ProdElList::Iter rhsEl = *prodElList; rhsEl.lte(); rhsEl++, position++ ) {
		if ( rhsEl->type == PdaFactor::ReferenceType ) {
			TypeRef *typeRef = new TypeRef( rhsEl->loc, rhsEl->nspaceQual, rhsEl->refName );

			/* Use an offset of zero. For frame objects we compute the offset on
			 * demand. */
			String name( 8, "r%d", position );
			ObjField *el = new ObjField( InputLoc(), typeRef, name );
			rhsEl->objField = el;

			/* Right hand side elements are constant. */
			el->isConst = true;
			el->isRhsEl = true;

			/* Only ever fetch for reading since they are constant. */
			el->inGetR = IN_GET_LOCAL_R;

			localFrame->objFieldMap->insert( el->name, el );
		}
	}
}

void ParseData::addProdRHSLoads( Definition *prod, long codeInsertPos, CodeVect &code )
{
	CodeVect loads;
	long position = 0;
	for ( ProdElList::Iter rhsEl = *prod->prodElList; rhsEl.lte(); rhsEl++, position++ ) {
		if ( rhsEl->type == PdaFactor::ReferenceType ) {
			if ( rhsEl->objField->beenReferenced ) {
				loads.append ( IN_INIT_RHS_EL );
				loads.appendHalf( position );
				loads.appendHalf( rhsEl->objField->offset );
			}
		}
	}
	code.insert( codeInsertPos, loads );
}

void ParseData::addMatchLength( ObjectDef *frame, KlangEl *lel )
{
	/* Make the type ref. */
	TypeRef *typeRef = new TypeRef( InputLoc(), uniqueTypeInt );

	/* Create the field and insert it into the map. */
	ObjField *el = new ObjField( InputLoc(), typeRef, "match_length" );
	el->beenReferenced = true;
	el->beenInitialized = true;
	el->isConst = true;
	el->useOffset = false;
	el->inGetR    = IN_GET_MATCH_LENGTH_R;
	frame->objFieldMap->insert( el->name, el );
}

void ParseData::addMatchText( ObjectDef *frame, KlangEl *lel )
{
	/* Make the type ref. */
	TypeRef *typeRef = new TypeRef( InputLoc(), uniqueTypeStr );

	/* Create the field and insert it into the map. */
	ObjField *el = new ObjField( InputLoc(), typeRef, "match_text" );
	el->beenReferenced = true;
	el->beenInitialized = true;
	el->isConst = true;
	el->useOffset = false;
	el->inGetR    = IN_GET_MATCH_TEXT_R;
	frame->objFieldMap->insert( el->name, el );
}

void ParseData::initFieldInstructions( ObjField *el )
{
	el->inGetR =   IN_GET_FIELD_R;
	el->inGetWC =  IN_GET_FIELD_WC;
	el->inGetWV =  IN_GET_FIELD_WV;
	el->inSetWC =  IN_SET_FIELD_WC;
	el->inSetWV =  IN_SET_FIELD_WV;
}

void ParseData::initLocalInstructions( ObjField *el )
{
	el->inGetR =   IN_GET_LOCAL_R;
	el->inGetWC =  IN_GET_LOCAL_WC;
	el->inSetWC =  IN_SET_LOCAL_WC;
}

void ParseData::initLocalRefInstructions( ObjField *el )
{
	el->inGetR =   IN_GET_LOCAL_REF_R;
	el->inGetWC =  IN_GET_LOCAL_REF_WC;
	el->inSetWC =  IN_SET_LOCAL_REF_WC;
}

void ParseData::initIntObject( )
{
	ObjFieldMap *fieldMap = new ObjFieldMap;
	ObjMethodMap *methodMap = new ObjMethodMap;
	intObj = new ObjectDef( ObjectDef::BuiltinType, "int", 
			fieldMap, methodMap, nextObjectId++ );
	intKlangEl->objectDef = intObj;

	initFunction( uniqueTypeStr, intObj, "to_string", IN_INT_TO_STR, IN_INT_TO_STR, true );
}

/* Add a constant length field to the object. 
 * Opcode supplied by the caller. */
void ParseData::addLengthField( ObjectDef *objDef, Code getLength )
{
	/* Create the "length" field. */
	TypeRef *typeRef = new TypeRef( InputLoc(), uniqueTypeInt );
	ObjField *el = new ObjField( InputLoc(), typeRef, "length" );
	el->beenReferenced = true;
	el->beenInitialized = true;
	el->isConst = true;
	el->useOffset = false;
	el->inGetR = getLength;

	objDef->objFieldMap->insert( el->name, el );
}

void ParseData::initStrObject( )
{
	ObjFieldMap *fieldMap = new ObjFieldMap;
	ObjMethodMap *methodMap = new ObjMethodMap;
	strObj = new ObjectDef( ObjectDef::BuiltinType, "str", 
			fieldMap, methodMap, nextObjectId++ );
	strKlangEl->objectDef = strObj;

	initFunction( uniqueTypeInt, strObj, "atoi",   IN_STR_ATOI, IN_STR_ATOI, true );
	initFunction( uniqueTypeInt, strObj, "uord8",  IN_STR_UORD8,  IN_STR_UORD8, true );
	initFunction( uniqueTypeInt, strObj, "sord8",  IN_STR_SORD8,  IN_STR_SORD8, true );
	initFunction( uniqueTypeInt, strObj, "uord16", IN_STR_UORD16, IN_STR_UORD16, true );
	initFunction( uniqueTypeInt, strObj, "sord16", IN_STR_SORD16, IN_STR_SORD16, true );
	initFunction( uniqueTypeInt, strObj, "uord32", IN_STR_UORD32, IN_STR_UORD32, true );
	initFunction( uniqueTypeInt, strObj, "sord32", IN_STR_SORD32, IN_STR_SORD32, true );
	addLengthField( strObj, IN_STR_LENGTH );
}

void ParseData::initStreamObject( )
{
	ObjFieldMap *fieldMap = new ObjFieldMap;
	ObjMethodMap *methodMap = new ObjMethodMap;
	streamObj = new ObjectDef( ObjectDef::BuiltinType, "stream", 
			fieldMap, methodMap, nextObjectId++ );
	streamKlangEl->objectDef = streamObj;

//	initFunction( uniqueTypeInt, strObj, "atoi",   IN_STR_ATOI, IN_STR_ATOI, true );
//	initFunction( uniqueTypeInt, strObj, "uord8",  IN_STR_UORD8,  IN_STR_UORD8, true );
//	initFunction( uniqueTypeInt, strObj, "sord8",  IN_STR_SORD8,  IN_STR_SORD8, true );
//	initFunction( uniqueTypeInt, strObj, "uord16", IN_STR_UORD16, IN_STR_UORD16, true );
//	initFunction( uniqueTypeInt, strObj, "sord16", IN_STR_SORD16, IN_STR_SORD16, true );
//	initFunction( uniqueTypeInt, strObj, "uord32", IN_STR_UORD32, IN_STR_UORD32, true );
//	initFunction( uniqueTypeInt, strObj, "sord32", IN_STR_SORD32, IN_STR_SORD32, true );
//	addLengthField( strObj, IN_STR_LENGTH );
}

ObjField *ParseData::makeDataEl()
{
	/* Create the "data" field. */
	TypeRef *typeRef = new TypeRef( InputLoc(), uniqueTypeStr );
	ObjField *el = new ObjField( InputLoc(), typeRef, "data" );

	/* Setting beenReferenced to true prevents us from assigning instructions
	 * and an offset to the field. */

	el->beenReferenced = true;
	el->beenInitialized = true;
	el->useOffset = false;
	el->inGetR = IN_GET_TOKEN_DATA_R;
	el->inSetWC = IN_SET_TOKEN_DATA_WC;
	el->inSetWV = IN_SET_TOKEN_DATA_WV;
	return el;
}

ObjField *ParseData::makePosEl()
{
	/* Create the "data" field. */
	TypeRef *typeRef = new TypeRef( InputLoc(), uniqueTypeInt );
	ObjField *el = new ObjField( InputLoc(), typeRef, "pos" );

	/* Setting beenReferenced to true prevents us from assigning instructions
	 * and an offset to the field. */

	el->isConst = true;
	el->beenReferenced = true;
	el->beenInitialized = true;
	el->useOffset = false;
	el->inGetR = IN_GET_TOKEN_POS_R;
	return el;
}

void ParseData::initTokenObjects( )
{
	/* Make a default object Definition. */
	ObjFieldMap *fieldMap = new ObjFieldMap;
	ObjMethodMap *methodMap = new ObjMethodMap;
	tokenObj = new ObjectDef( ObjectDef::BuiltinType, "token", fieldMap, 
			methodMap, nextObjectId++ );

	ObjField *dataEl = makeDataEl();
	tokenObj->objFieldMap->insert( dataEl->name, dataEl );

	ObjField *posEl = makePosEl();
	tokenObj->objFieldMap->insert( posEl->name, posEl );


	/* Give all user terminals the token object type. */
	for ( LelList::Iter lel = langEls; lel.lte(); lel++ ) {
		if ( lel->isUserTerm ) {
			if ( lel->objectDef == 0 )
				lel->objectDef = tokenObj;
			else {
				/* Create the "data" field. */
				ObjField *dataEl = makeDataEl();
				lel->objectDef->objFieldMap->insert( dataEl->name, dataEl );

				/* Create the "pos" field. */
				ObjField *posEl = makePosEl();
				lel->objectDef->objFieldMap->insert( posEl->name, posEl );
			}
		}
	}
}

void ParseData::findLocalTrees( CharSet &trees )
{
	/* We exlcude "lhs" from being downrefed because we need to use if after
	 * the frame is is cleaned and so it must survive. */
	for ( ObjFieldMap::Iter of = *curLocalFrame->objFieldMap; of.lte(); of++ ) {
		ObjField *el = of->value;
		if ( !el->isLhsEl && el->beenReferenced ) {
			UniqueType *ut = el->typeRef->lookupType( this );
			if ( ut->typeId == TYPE_TREE || ut->typeId == TYPE_PTR )
				trees.insert( el->offset );
		}
	}
}

void ParseData::compileReductionCode( Definition *prod )
{
	CodeBlock *block = prod->redBlock;

	/* Init the compilation context. */
	compileContext = CompileReduction;
	curLocalFrame = block->localFrame;
	revertOn = true;
	block->frameId = nextFrameId++;

	CodeVect &code = block->codeWV;

	/* Add the alloc frame opcode. We don't have the right 
	 * frame size yet. We will fill it in later. */
	code.append( IN_INIT_LOCALS );
	code.appendHalf( 0 );
	long afterAllocFrame = code.length();

	/* Compile the reduce block. */
	block->compile( this, code );

	/* We have the frame size now. Set in the alloc frame instruction. */
	long frameSize = curLocalFrame->size();
	code.setHalf( 1, frameSize );

	addProdRHSLoads( prod, afterAllocFrame, code );

	code.append( IN_POP_LOCALS );
	code.appendHalf( block->frameId );
	code.appendHalf( frameSize );

	code.append( IN_STOP );

	/* Now that compilation is done variables are referenced. Make the local
	 * trees descriptor. */
	findLocalTrees( block->trees );
}

void ParseData::compileTranslateBlock( KlangEl *langEl )
{
	CodeBlock *block = langEl->transBlock;

	/* Set up compilation context. */
	compileContext = CompileTranslation;
	curLocalFrame = block->localFrame;
	revertOn = true;
	block->frameId = nextFrameId++;

	/* References to the reduce item. */
	addMatchLength( curLocalFrame, langEl );
	addMatchText( curLocalFrame, langEl );
	initFunction( uniqueTypeStr, curLocalFrame, "pull",
			IN_STREAM_PULL, IN_STREAM_PULL, uniqueTypeStream, uniqueTypeInt, true );
	initFunction( uniqueTypeInt, curLocalFrame, "push",
			IN_STREAM_PUSH, IN_STREAM_PUSH, uniqueTypeStream, uniqueTypeAny, true );

	initFunction( uniqueTypeInt, curLocalFrame, "send",
			IN_SEND, IN_SEND, uniqueTypeAny, true );
	initFunction( uniqueTypeInt, curLocalFrame, "send_ignore",
			IN_IGNORE, IN_IGNORE, uniqueTypeAny, true );

	CodeVect &code = block->codeWV;

	/* Add the alloc frame opcode. We don't have the right 
	 * frame size yet. We will fill it in later. */
	code.append( IN_INIT_LOCALS );
	code.appendHalf( 0 );

	/* Set the local frame and compile the reduce block. */
	block->compile( this, code );

	/* We have the frame size now. Set in the alloc frame instruction. */
	long frameSize = curLocalFrame->size();
	code.setHalf( 1, frameSize );

	code.append( IN_POP_LOCALS );
	code.appendHalf( block->frameId );
	code.appendHalf( frameSize );

	code.append( IN_STOP );

	/* Now that compilation is done variables are referenced. Make the local
	 * trees descriptor. */
	findLocalTrees( block->trees );
}

void ParseData::compilePreEof( TokenRegion *region )
{
	CodeBlock *block = region->preEofBlock;

	/* Set up compilation context. */
	compileContext = CompileTranslation;
	curLocalFrame = region->preEofBlock->localFrame;
	revertOn = true;
	block->frameId = nextFrameId++;

	/* References to the reduce item. */
//	addMatchLength( curLocalFrame, langEl );
//	addMatchText( curLocalFrame, langEl );
//	initFunction( uniqueTypeStr, curLocalFrame, "pull",
//			IN_STREAM_PULL, IN_STREAM_PULL, uniqueTypeStream, uniqueTypeInt, true );

	initFunction( uniqueTypeInt, curLocalFrame, "send",
			IN_SEND, IN_SEND, uniqueTypeAny, true );
	initFunction( uniqueTypeInt, curLocalFrame, "send_ignore",
			IN_IGNORE, IN_IGNORE, uniqueTypeAny, true );

	CodeVect &code = block->codeWV;

	/* Add the alloc frame opcode. We don't have the right 
	 * frame size yet. We will fill it in later. */
	code.append( IN_INIT_LOCALS );
	code.appendHalf( 0 );

	/* Set the local frame and compile the reduce block. */
	block->compile( this, code );

	/* We have the frame size now. Set in the alloc frame instruction. */
	long frameSize = curLocalFrame->size();
	code.setHalf( 1, frameSize );

	code.append( IN_POP_LOCALS );
	code.appendHalf( block->frameId );
	code.appendHalf( frameSize );

	code.append( IN_STOP );

	/* Now that compilation is done variables are referenced. Make the local
	 * trees descriptor. */
	findLocalTrees( block->trees );
}

void ParseData::compileRootBlock( )
{
	CodeBlock *block = rootCodeBlock;

	/* The root block never needs to be reverted. */

	/* Set up the compile context. No locals are needed for the root code
	 * block, but we need an empty local frame for the compile. */
	compileContext = CompileRoot;
	curLocalFrame = rootLocalFrame;
	revertOn = false;

	/* The block needs a frame id. */
	block->frameId = nextFrameId++;

	/* The root block is not reverted. */
	CodeVect &code = block->codeWC;

	/* Add the alloc frame opcode. We don't have the right 
	 * frame size yet. We will fill it in later. */
	code.append( IN_INIT_LOCALS );
	code.appendHalf( 0 );

	block->compile( this, code );

	/* We have the frame size now. Store it in frame init. */
	long frameSize = curLocalFrame->size();
	code.setHalf( 1, frameSize );

	code.append( IN_POP_LOCALS );
	code.appendHalf( block->frameId );
	code.appendHalf( frameSize );

	code.append( IN_STOP );

	/* Make the local trees descriptor. */
	findLocalTrees( block->trees );
}

void ParseData::initAllLanguageObjects()
{
	/* Init all user object fields (need consistent size). */
	for ( LelList::Iter lel = langEls; lel.lte(); lel++ ) {
		ObjectDef *obj = lel->objectDef;
		if ( obj != 0 ) {
			/* Init all fields of the object. */
			for ( ObjFieldMap::Iter f = *obj->objFieldMap; f.lte(); f++ )
				obj->initField( this, f->value );
		}
	}

	/* Init all fields of the global object. */
	for ( ObjFieldMap::Iter f = *globalObjectDef->objFieldMap; f.lte(); f++ )
		globalObjectDef->initField( this, f->value );
}

void ParseData::initMapFunctions( GenericType *gen )
{
	addLengthField( gen->objDef, IN_MAP_LENGTH );
	initFunction( gen->utArg, gen->objDef, "find", 
			IN_MAP_FIND,      IN_MAP_FIND, gen->keyUT, true );
	initFunction( uniqueTypeInt, gen->objDef, "insert", 
			IN_MAP_INSERT_WV, IN_MAP_INSERT_WC, gen->keyUT, gen->utArg, false );
	initFunction( uniqueTypeInt, gen->objDef, "store", 
			IN_MAP_STORE_WV,  IN_MAP_STORE_WC, gen->keyUT, gen->utArg, false );
	initFunction( gen->utArg, gen->objDef, "remove", 
			IN_MAP_REMOVE_WV, IN_MAP_REMOVE_WC, gen->keyUT, false );
}

void ParseData::initListFunctions( GenericType *gen )
{
	addLengthField( gen->objDef, IN_LIST_LENGTH );

	initFunction( uniqueTypeInt, gen->objDef, "append", 
			IN_LIST_APPEND_WV, IN_LIST_APPEND_WC, gen->utArg, false );
	initFunction( uniqueTypeInt, gen->objDef, "push", 
			IN_LIST_APPEND_WV, IN_LIST_APPEND_WC, gen->utArg, false );

	initFunction( gen->utArg, gen->objDef, "remove_end", 
			IN_LIST_REMOVE_END_WV, IN_LIST_REMOVE_END_WC, false );
	initFunction( gen->utArg, gen->objDef, "pop", 
			IN_LIST_REMOVE_END_WV, IN_LIST_REMOVE_END_WC, false );
}

void ParseData::initListField( GenericType *gen, const char *name, int offset )
{
	/* Make the type ref and create the field. */
	TypeRef *typeRef = new TypeRef( InputLoc(), gen->utArg );
	ObjField *el = new ObjField( InputLoc(), typeRef, name );

	el->inGetR =  IN_GET_LIST_MEM_R;
	el->inGetWC = IN_GET_LIST_MEM_WC;
	el->inGetWV = IN_GET_LIST_MEM_WV;
	el->inSetWC = IN_SET_LIST_MEM_WC;
	el->inSetWV = IN_SET_LIST_MEM_WV;

	gen->objDef->objFieldMap->insert( el->name, el );

	el->useOffset = true;
	el->beenReferenced = true;
	el->beenInitialized = true;

	/* Zero for head, One for tail. */
	el->offset = offset;
}

void ParseData::initListFields( GenericType *gen )
{
	initListField( gen, "head", 0 );
	initListField( gen, "tail", 1 );
	initListField( gen, "top", 1 );
}

void ParseData::initVectorFunctions( GenericType *gen )
{
	addLengthField( gen->objDef, IN_VECTOR_LENGTH );
	initFunction( uniqueTypeInt, gen->objDef, "append", 
			IN_VECTOR_APPEND_WV, IN_VECTOR_APPEND_WC, gen->utArg, false );
	initFunction( uniqueTypeInt, gen->objDef, "insert", 
			IN_VECTOR_INSERT_WV, IN_VECTOR_INSERT_WC, uniqueTypeInt, gen->utArg, false );
}

void ParseData::resolveGenericTypes()
{
	for ( NamespaceList::Iter ns = namespaceList; ns.lte(); ns++ ) {
		for ( GenericList::Iter gen = ns->genericList; gen.lte(); gen++ ) {
			gen->utArg = gen->typeArg->lookupType( this );

			if ( gen->typeId == GEN_MAP )
				gen->keyUT = gen->keyTypeArg->lookupType( this );

			ObjFieldMap *fieldMap = new ObjFieldMap;
			ObjMethodMap *methodMap = new ObjMethodMap;
			gen->objDef = new ObjectDef( ObjectDef::BuiltinType, 
					gen->name, fieldMap, methodMap, nextObjectId++ );

			switch ( gen->typeId ) {
				case GEN_MAP: 
					initMapFunctions( gen );
					break;
				case GEN_LIST:
					initListFunctions( gen );
					initListFields( gen );
					break;
				case GEN_VECTOR:
					initVectorFunctions( gen );
					break;
			}

			gen->langEl->objectDef = gen->objDef;
		}
	}
}

void ParseData::makeFuncVisible( Function *func, bool isUserIter )
{
	/* Need an object for the local frame. */
	curLocalFrame = func->codeBlock->localFrame;
	func->localFrame = func->codeBlock->localFrame;

	/* Set up the parameters. */
	long paramPos = 0, paramListSize = 0;
	UniqueType **paramUTs = new UniqueType*[func->paramList->length()];
	for ( ParameterList::Iter param = *func->paramList; param.lte(); param++ ) {
		paramUTs[paramPos] = param->typeRef->lookupType( this );

		if ( func->localFrame->objFieldMap->find( param->name ) != 0 )
			error(param->loc) << "parameter " << param->name << " redeclared" << endp;

		func->localFrame->objFieldMap->insert( param->name, param );
		param->beenInitialized = true;
		param->pos = paramPos;

		/* Initialize the object field as a local variable. We also want trees
		 * downreffed. */
		if ( paramUTs[paramPos]->typeId == TYPE_REF )
			initLocalRefInstructions( param );
		else
			initLocalInstructions( param );

		paramListSize += sizeOfField( paramUTs[paramPos] );
		paramPos += 1;
	}

	/* Param offset is relative to one past the last item in the array of
	 * words containing the args. */
	long paramOffset = 0;
	for ( ParameterList::Iter param = *func->paramList; param.lte(); param++ ) {

		/* Moving downward, and need the offset to point to the lower half of
		 * the argument. */
		paramOffset -= sizeOfField( paramUTs[param->pos] );

		/* How much space do we need to make for call overhead. */
		long frameAfterArgs = isUserIter ? IFR_AA : FR_AA;

		/* Going up first we have the frame data, then maybe
		 * the user iterator, then the args from high to low. */
		param->offset = frameAfterArgs + 
				( isUserIter ? ( sizeof(UserIter) / sizeof(Word) ) : 0 ) +
				paramListSize + paramOffset;
	}

	func->paramListSize = paramListSize;
	func->paramUTs = paramUTs;

	/* Insert the function into the global function map. */
	UniqueType *returnUT = func->typeRef != 0 ? 
			func->typeRef->lookupType(this) : uniqueTypeInt;
	ObjMethod *objMethod = new ObjMethod( returnUT, func->name, 
			IN_CALL_WV, IN_CALL_WC, 
			func->paramList->length(), paramUTs, func->paramList, false );
	objMethod->funcId = func->funcId;
	objMethod->useFuncId = true;
	objMethod->useCallObj = false;
	objMethod->func = func;

	if ( isUserIter ) {
		IterDef *uiter = findIterDef( IterDef::User, func );
		objMethod->iterDef = uiter;
	}

	globalObjectDef->objMethodMap->insert( func->name, objMethod );
}

void ParseData::compileUserIter( Function *func )
{
	CodeBlock *block = func->codeBlock;

	compileContext = CompileFunction;
	curFunction = func;
	revertOn = true;
	block->frameId = nextFrameId++;

	makeFuncVisible( func, true );

	CodeVect &code = block->codeWV;

	/* Add the alloc frame opcode. We don't have the right 
	 * frame size yet. We will fill it in later. */
	code.append( IN_INIT_LOCALS );
	code.appendHalf( 0 );

	/* Compile the block. */
	block->compile( this, code );

	/* We have the frame size now. Set in the alloc frame instruction. */
	int frameSize = func->localFrame->size();
	code.setHalf( 1, frameSize );

	/* Check for a return statement. */
	if ( block->stmtList->length() == 0 ||
			block->stmtList->tail->type != LangStmt::YieldType )
	{
		/* Push the return value. */
		code.append( IN_LOAD_NIL );
		code.append( IN_YIELD );
	}

	/* Now that compilation is done variables are referenced. Make the local
	 * trees descriptor. */
	findLocalTrees( block->trees );

	/* FIXME: Need to deal with the freeing of local trees. */
}

/* Called for each type of function compile: revert and commit. */
void ParseData::compileFunction( Function *func, CodeVect &code )
{
	CodeBlock *block = func->codeBlock;

	/* Add the alloc frame opcode. We don't have the right 
	 * frame size yet. We will fill it in later. */
	code.append( IN_INIT_LOCALS );
	code.appendHalf( 0 );

	/* Compile the block. */
	block->compile( this, code );

	/* We have the frame size now. Set in the alloc frame instruction. */
	int frameSize = func->localFrame->size();
	code.setHalf( 1, frameSize );

	/* Check for a return statement. */
	if ( block->stmtList->length() == 0 ||
			block->stmtList->tail->type != LangStmt::ReturnType )
	{
		/* Push the return value. */
		code.append( IN_LOAD_NIL );
		code.append( IN_SAVE_RET );
	}

	/* Compute the jump distance for the return jumps. */
	for ( LongVect::Iter rj = returnJumps; rj.lte(); rj++ ) {
		long distance = code.length() - *rj - 3;
		code.setHalf( *rj+1, distance );
	}

	/* Reset the vector of return jumps. */
	returnJumps.empty();

	/* Return cleans up the stack (including the args) and leaves the return
	 * value on the top. */
	code.append( IN_RET );
	code.appendHalf( func->funcId );
}

void ParseData::compileFunction( Function *func )
{
	CodeBlock *block = func->codeBlock;

	/* Set up the compilation context. */
	compileContext = CompileFunction;
	curFunction = func;

	/* Assign a frame Id. */
	block->frameId = nextFrameId++;

	makeFuncVisible( func, false );

	/* Compile once for revert. */
	revertOn = true;
	compileFunction( func, block->codeWV );

	/* Compile once for commit. */
	revertOn = false;
	compileFunction( func, block->codeWC );

	/* Now that compilation is done variables are referenced. Make the local
	 * trees descriptor. */
	findLocalTrees( block->trees );
}

void ParseData::makeDefaultIterators()
{
	/* Tree iterator. */
	{
		UniqueType *anyRefUT = findUniqueType( TYPE_REF, anyKlangEl );
		ObjMethod *objMethod = initFunction( uniqueTypeAny, globalObjectDef, 
				"triter", IN_HALT, IN_HALT, anyRefUT, true );

		IterDef *triter = findIterDef( IterDef::Tree );
		objMethod->iterDef = triter;
	}

	/* Child iterator. */
	{
		UniqueType *anyRefUT = findUniqueType( TYPE_REF, anyKlangEl );
		ObjMethod *objMethod = initFunction( uniqueTypeAny, globalObjectDef, 
				"child", IN_HALT, IN_HALT, anyRefUT, true );

		IterDef *triter = findIterDef( IterDef::Child );
		objMethod->iterDef = triter;
	}

	/* Reverse iterator. */
	{
		UniqueType *anyRefUT = findUniqueType( TYPE_REF, anyKlangEl );
		ObjMethod *objMethod = initFunction( uniqueTypeAny, globalObjectDef, 
				"rev_child", IN_HALT, IN_HALT, anyRefUT, true );

		IterDef *triter = findIterDef( IterDef::RevChild );
		objMethod->iterDef = triter;
	}
}

void ParseData::addStdin()
{
	/* Make the type ref. */
	TypeRef *typeRef = new TypeRef( InputLoc(), uniqueTypeStream );

	/* Create the field and insert it into the map. */
	ObjField *el = new ObjField( InputLoc(), typeRef, "stdin" );
	el->beenReferenced = true;
	el->beenInitialized = true;
	el->isConst = true;
	el->useOffset = false;
	el->inGetR    = IN_GET_STDIN;
	globalObjectDef->objFieldMap->insert( el->name, el );
}

void ParseData::addStdout()
{
	/* Make the type ref. */
	TypeRef *typeRef = new TypeRef( InputLoc(), uniqueTypeStr );

	/* Create the field and insert it into the map. */
	ObjField *el = new ObjField( InputLoc(), typeRef, "stout" );
	el->beenReferenced = true;
	el->beenInitialized = true;
	el->isConst = true;
	el->useOffset = false;
	el->inGetR    = IN_GET_STDOUT;
	globalObjectDef->objFieldMap->insert( el->name, el );
}

void ParseData::addStderr()
{
	/* Make the type ref. */
	TypeRef *typeRef = new TypeRef( InputLoc(), uniqueTypeStr );

	/* Create the field and insert it into the map. */
	ObjField *el = new ObjField( InputLoc(), typeRef, "stderr" );
	el->beenReferenced = true;
	el->beenInitialized = true;
	el->isConst = true;
	el->useOffset = false;
	el->inGetR    = IN_GET_STDERR;
	globalObjectDef->objFieldMap->insert( el->name, el );
}

void ParseData::initGlobalFunctions()
{
	ObjMethod *method;

	method = initFunction( uniqueTypeStream, globalObjectDef, "open_file",
		IN_OPEN_FILE, IN_OPEN_FILE, uniqueTypeStr, true );
	method->useCallObj = false;

	addStdin();
	addStdout();
	addStderr();
}

void ParseData::compileByteCode()
{
	initUniqueTypes();
	initIntObject();
	initStrObject();
	initStreamObject();
	initTokenObjects();
	makeDefaultIterators();
	initAllLanguageObjects();
	resolveGenericTypes();

	initGlobalFunctions();

	/* The function info structure relies on functions being compile first,
	 * then iterators. */

	/* Compile functions. */
	for ( FunctionList::Iter f = functionList; f.lte(); f++ ) {
		if ( f->isUserIter )
			compileUserIter( f );
		else
			compileFunction( f );
	}

	/* Compile the reduction code. */
	for ( DefList::Iter prod = prodList; prod.lte(); prod++ ) {
		if ( prod->redBlock != 0 )
			compileReductionCode( prod );
	}

	/* Compile the token translation code. */
	for ( LelList::Iter lel = langEls; lel.lte(); lel++ ) {
		if ( lel->transBlock != 0 )
			compileTranslateBlock( lel );
	}

	/* Compile preeof blocks. */
	for ( RegionList::Iter r = regionList; r.lte(); r++ ) {
		if ( r->preEofBlock != 0 )
			compilePreEof( r );
	}

	/* Compile the init code */
	compileRootBlock( );
}
