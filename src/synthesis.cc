/*
 * Copyright 2007-2012 Adrian Thurston <thurston@colm.net>
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

#include <assert.h>
#include <stdbool.h>

#include <iostream>

#include "compiler.h"

using std::cout;
using std::cerr;
using std::endl;

bool isStr( UniqueType *ut )
{
	return ut->typeId == TYPE_TREE && ut->langEl != 0 && ut->langEl->id == LEL_ID_STR;
}

bool isTree( UniqueType *ut )
{
	return ut->typeId == TYPE_TREE;
}

IterDef::IterDef( Type type )
: 
	type(type), 
	func(0)
{
}

IterDef::IterDef( Type type, Function *func )
:
	type(type),
	func(func)
{}

IterImpl::IterImpl( Type type ) : 
	type(type), 
	func(0),
	useFuncId(false),
	useSearchUT(false),
	useGenericId(false)
{
	switch ( type ) {
	case Tree:
		inCreateWV = IN_TRITER_FROM_REF;
		inCreateWC = IN_TRITER_FROM_REF;
		inUnwind =   IN_TRITER_UNWIND;
		inDestroy =  IN_TRITER_DESTROY;
		inAdvance =  IN_TRITER_ADVANCE;

		inGetCurR =  IN_TRITER_GET_CUR_R;
		inGetCurWC = IN_TRITER_GET_CUR_WC;
		inSetCurWC = IN_TRITER_SET_CUR_WC;
		inRefFromCur = IN_TRITER_REF_FROM_CUR;
		useSearchUT = true;
		break;

	case Child:
		inCreateWV = IN_TRITER_FROM_REF;
		inCreateWC = IN_TRITER_FROM_REF;
		inUnwind =   IN_TRITER_UNWIND;
		inDestroy =  IN_TRITER_DESTROY;
		inAdvance =  IN_TRITER_NEXT_CHILD;

		inGetCurR =  IN_TRITER_GET_CUR_R;
		inGetCurWC = IN_TRITER_GET_CUR_WC;
		inSetCurWC = IN_TRITER_SET_CUR_WC;
		inRefFromCur = IN_TRITER_REF_FROM_CUR;
		useSearchUT = true;
		break;

	case RevChild:
		inCreateWV = IN_REV_TRITER_FROM_REF;
		inCreateWC = IN_REV_TRITER_FROM_REF;
		inUnwind =   IN_REV_TRITER_UNWIND;
		inDestroy =  IN_REV_TRITER_DESTROY;
		inAdvance =  IN_REV_TRITER_PREV_CHILD;

		inGetCurR =  IN_TRITER_GET_CUR_R;
		inGetCurWC = IN_TRITER_GET_CUR_WC;
		inSetCurWC = IN_TRITER_SET_CUR_WC;
		inRefFromCur = IN_TRITER_REF_FROM_CUR;
		useSearchUT = true;
		break;
	
	case Repeat:
		inCreateWV = IN_TRITER_FROM_REF;
		inCreateWC = IN_TRITER_FROM_REF;
		inUnwind =   IN_TRITER_UNWIND;
		inDestroy =  IN_TRITER_DESTROY;
		inAdvance =  IN_TRITER_NEXT_REPEAT;

		inGetCurR =  IN_TRITER_GET_CUR_R;
		inGetCurWC = IN_TRITER_GET_CUR_WC;
		inSetCurWC = IN_TRITER_SET_CUR_WC;
		inRefFromCur = IN_TRITER_REF_FROM_CUR;
		useSearchUT = true;
		break;

	case RevRepeat:
		inCreateWV = IN_TRITER_FROM_REF;
		inCreateWC = IN_TRITER_FROM_REF;
		inUnwind =   IN_TRITER_UNWIND;
		inDestroy =  IN_TRITER_DESTROY;
		inAdvance =  IN_TRITER_PREV_REPEAT;

		inGetCurR =  IN_TRITER_GET_CUR_R;
		inGetCurWC = IN_TRITER_GET_CUR_WC;
		inSetCurWC = IN_TRITER_SET_CUR_WC;
		inRefFromCur = IN_TRITER_REF_FROM_CUR;
		useSearchUT = true;
		break;
	
	case ListEl:
		inCreateWV =   IN_GEN_ITER_FROM_REF;
		inCreateWC =   IN_GEN_ITER_FROM_REF;
		inUnwind =     IN_GEN_ITER_UNWIND;
		inDestroy =    IN_GEN_ITER_DESTROY;
		inAdvance =    IN_LIST_ITER_ADVANCE;

		inGetCurR =    IN_GEN_ITER_GET_CUR_R;
//		inGetCurWC =   //IN_LIST_ITER_GET_CUR_WC;
//		inSetCurWC =   //IN_HALT;
//		inRefFromCur = //IN_LIST_ITER_REF_FROM_CUR;
		useGenericId = true;
		break;

	case ListVal:
		inCreateWV =   IN_GEN_ITER_FROM_REF;
		inCreateWC =   IN_GEN_ITER_FROM_REF;
		inUnwind =     IN_GEN_ITER_UNWIND;
		inDestroy =    IN_GEN_ITER_DESTROY;
		inAdvance =    IN_LIST_ITER_ADVANCE;

		inGetCurR =    IN_GEN_VITER_GET_CUR_R;
//		inGetCurWC =   //IN_LIST_ITER_GET_CUR_WC;
//		inSetCurWC =   //IN_HALT;
//		inRefFromCur = //IN_LIST_ITER_REF_FROM_CUR;
		useGenericId = true;
		break;

	case RevListVal:
		inCreateWV =   IN_GEN_ITER_FROM_REF;
		inCreateWC =   IN_GEN_ITER_FROM_REF;
		inUnwind =     IN_GEN_ITER_UNWIND;
		inDestroy =    IN_GEN_ITER_DESTROY;
		inAdvance =    IN_REV_LIST_ITER_ADVANCE;

		inGetCurR =    IN_GEN_VITER_GET_CUR_R;
//		inGetCurWC =   //IN_LIST_ITER_GET_CUR_WC;
//		inSetCurWC =   //IN_HALT;
//		inRefFromCur = //IN_LIST_ITER_REF_FROM_CUR;
		useGenericId = true;
		break;


	case MapVal:
		inCreateWV =   IN_GEN_ITER_FROM_REF;
		inCreateWC =   IN_GEN_ITER_FROM_REF;
		inUnwind =     IN_GEN_ITER_UNWIND;
		inDestroy =    IN_GEN_ITER_DESTROY;
		inAdvance =    IN_MAP_ITER_ADVANCE;

		inGetCurR =    IN_GEN_VITER_GET_CUR_R;
		inGetCurWC =   IN_GEN_VITER_GET_CUR_R; //IN_HALT; //IN_LIST_ITER_GET_CUR_WC;
//		inSetCurWC =   IN_HALT;//IN_HALT;
//		inRefFromCur = IN_HALT;//IN_LIST_ITER_REF_FROM_CUR;
		useGenericId = true;
		break;

	case MapEl:
		inCreateWV =   IN_GEN_ITER_FROM_REF;
		inCreateWC =   IN_GEN_ITER_FROM_REF;
		inUnwind =     IN_GEN_ITER_UNWIND;
		inDestroy =    IN_GEN_ITER_DESTROY;
		inAdvance =    IN_MAP_ITER_ADVANCE;

		inGetCurR =    IN_GEN_ITER_GET_CUR_R;
//		inGetCurWC =   //IN_LIST_ITER_GET_CUR_WC;
//		inSetCurWC =   //IN_HALT;
//		inRefFromCur = //IN_LIST_ITER_REF_FROM_CUR;
		useGenericId = true;
		break;

	case User:
		assert(false);
	}
}

IterImpl::IterImpl( Type type, Function *func ) : 
	type(type),
	func(func),
	useFuncId(true),
	useSearchUT(true),
	useGenericId(false),
	inCreateWV(IN_UITER_CREATE_WV),
	inCreateWC(IN_UITER_CREATE_WC),
	inUnwind(IN_UITER_UNWIND),
	inDestroy(IN_UITER_DESTROY),
	inAdvance(IN_UITER_ADVANCE),
	inGetCurR(IN_UITER_GET_CUR_R),
	inGetCurWC(IN_UITER_GET_CUR_WC),
	inSetCurWC(IN_UITER_SET_CUR_WC),
	inRefFromCur(IN_UITER_REF_FROM_CUR)
{}

IterDef *Compiler::findIterDef( IterDef::Type type, Function *func )
{
	IterDefSetEl *el = iterDefSet.find( IterDef( type, func ) );
	if ( el == 0 )
		el = iterDefSet.insert( IterDef( type, func ) );
	return &el->key;
}

IterDef *Compiler::findIterDef( IterDef::Type type )
{
	IterDefSetEl *el = iterDefSet.find( IterDef( type ) );
	if ( el == 0 )
		el = iterDefSet.insert( IterDef( type ) );
	return &el->key;
}

UniqueType *Compiler::findUniqueType( enum TYPE typeId )
{
	UniqueType searchKey( typeId );
	UniqueType *uniqueType = uniqeTypeMap.find( &searchKey );
	if ( uniqueType == 0 ) {
		uniqueType = new UniqueType( typeId );
		uniqeTypeMap.insert( uniqueType );
	}
	return uniqueType;
}

UniqueType *Compiler::findUniqueType( enum TYPE typeId, LangEl *langEl )
{
	UniqueType searchKey( typeId, langEl );
	UniqueType *uniqueType = uniqeTypeMap.find( &searchKey );
	if ( uniqueType == 0 ) {
		uniqueType = new UniqueType( typeId, langEl );
		uniqeTypeMap.insert( uniqueType );
	}
	return uniqueType;
}

UniqueType *Compiler::findUniqueType( enum TYPE typeId, IterDef *iterDef )
{
	UniqueType searchKey( typeId, iterDef );
	UniqueType *uniqueType = uniqeTypeMap.find( &searchKey );
	if ( uniqueType == 0 ) {
		uniqueType = new UniqueType( typeId, iterDef );
		uniqeTypeMap.insert( uniqueType );
	}
	return uniqueType;
}

UniqueType *Compiler::findUniqueType( enum TYPE typeId, StructEl *structEl )
{
	UniqueType searchKey( typeId, structEl );
	UniqueType *uniqueType = uniqeTypeMap.find( &searchKey );
	if ( uniqueType == 0 ) {
		uniqueType = new UniqueType( typeId, structEl );
		uniqeTypeMap.insert( uniqueType );
	}
	return uniqueType;
}

UniqueType *Compiler::findUniqueType( enum TYPE typeId, GenericType *generic )
{
	UniqueType searchKey( typeId, generic );
	UniqueType *uniqueType = uniqeTypeMap.find( &searchKey );
	if ( uniqueType == 0 ) {
		uniqueType = new UniqueType( typeId, generic );
		uniqeTypeMap.insert( uniqueType );
	}
	return uniqueType;
}

/* 0-based. */
ObjectField *ObjectDef::findFieldNum( long offset )
{
	/* Bounds check. */
	if ( offset >= fieldList.length() )
		return 0;

	int fn = 0;
	FieldList::Iter field = fieldList; 
	while ( fn < offset ) {
		fn++;
		field++;
	}

	return field->value;
}

/* Finds the first field by type. */
ObjectField *ObjectDef::findFieldType( Compiler *pd, UniqueType *ut )
{
	for ( FieldList::Iter f = fieldList; f.lte(); f++ ) {
		UniqueType *fUT = f->value->typeRef->resolveType( pd );
		if ( fUT == ut )
			return f->value;
	}
	return 0;
}


long sizeOfField( UniqueType *fieldUT )
{
	long size = 0;
	switch ( fieldUT->typeId ) {
	case TYPE_ITER:
		/* Select on the iterator type. */
		switch ( fieldUT->iterDef->type ) {
			case IterDef::Tree:
			case IterDef::Child:
			case IterDef::Repeat:
			case IterDef::RevRepeat:
				size = sizeof(tree_iter_t) / sizeof(word_t);
				break;

			case IterDef::RevChild:
				size = sizeof(rev_tree_iter_t) / sizeof(word_t);
				break;

			case IterDef::MapEl:
			case IterDef::ListEl:
			case IterDef::RevListVal:
				size = sizeof(generic_iter_t) / sizeof(word_t);
				break;

			case IterDef::User:
				/* User iterators are just a pointer to the user_iter_t struct. The
				 * struct needs to go right beneath the call to the user iterator
				 * so it can be found by a yield. It is therefore allocated on the
				 * stack right before the call. */
				size = 1;
				break;
		}
		break;
	case TYPE_REF:
		size = 2;
		break;
	case TYPE_GENERIC:
		size = 1;
		break;
	case TYPE_LIST_PTRS:
		size = COLM_LIST_EL_SIZE;
		break;
	case TYPE_MAP_PTRS:
		size = COLM_MAP_EL_SIZE;
		break;
	default:
		size = 1;
		break;
	}

	return size;
}

void ObjectDef::referenceField( Compiler *pd, ObjectField *field )
{
	field->beenReferenced = true;
}

UniqueType *LangVarRef::loadField( Compiler *pd, CodeVect &code, 
		ObjectDef *inObject, ObjectField *el, bool forWriting, bool revert ) const
{
	/* Ensure that the field is referenced. */
	inObject->referenceField( pd, el );

	UniqueType *elUT = el->typeRef->uniqueType;

	if ( elUT->val() ) {
		if ( forWriting ) {
			/* The instruction, depends on whether or not we are reverting. */
			if ( pd->revertOn && revert )
				code.append( el->inGetValWV );
			else
				code.append( el->inGetValWC );
		}
		else {
			/* Loading for writing */
			code.append( el->inGetValR );
		}
	}
	else {
		/* If it's a reference then we load it read always. */
		if ( forWriting ) {
			/* The instruction, depends on whether or not we are reverting. */
			if ( elUT->typeId == TYPE_ITER )
				code.append( el->iterImpl->inGetCurWC );
			else if ( pd->revertOn && revert )
				code.append( el->inGetWV );
			else
				code.append( el->inGetWC );
		}
		else {
			/* Loading something for reading */
			if ( elUT->typeId == TYPE_ITER )
				code.append( el->iterImpl->inGetCurR );
			else
				code.append( el->inGetR );
		}
	}

	if ( el->useGenericId )
		code.appendHalf( el->generic->id );

	if ( el->useOffset() ) {
		/* Gets of locals and fields require offsets. Fake vars like token
		 * data and lhs don't require it. */
		code.appendHalf( el->offset );
	}
	else if ( el->isRhsGet() ) {
		/* Need to place the array computing the val. */
		code.append( el->rhsVal.length() );
		for ( Vector<RhsVal>::Iter rg = el->rhsVal; rg.lte(); rg++ ) {
			code.append( rg->prodEl->production->prodNum );
			code.append( rg->prodEl->pos );
		}
	}

	if ( el->isConstVal ) {
		code.appendHalf( el->constValId );

		if ( el->constValId == IN_CONST_ARG ) {
			/* Make sure we have this string. */
			StringMapEl *mapEl = 0;
			if ( pd->literalStrings.insert( el->constValArg, &mapEl ) )
				mapEl->value = pd->literalStrings.length()-1;

			code.appendWord( mapEl->value );
		}
	}

	/* If we are dealing with an iterator then dereference it. */
	if ( elUT->typeId == TYPE_ITER )
		elUT = el->typeRef->searchUniqueType;

	return elUT;
}

/* The qualification must start at a local frame. There cannot be any pointer. */
long LangVarRef::loadQualificationRefs( Compiler *pd, CodeVect &code,
		NameScope *rootScope ) const
{
	long count = 0;

	/* Start the search from the root object. */
	NameScope *searchScope = rootScope;

	for ( QualItemVect::Iter qi = *qual; qi.lte(); qi++ ) {
		/* Lookup the field in the current qualification. */
		ObjectField *el = searchScope->findField( qi->data );
		if ( el == 0 )
			error(qi->loc) << "cannot resolve qualification " << qi->data << endp;

		if ( qi.pos() > 0 ) {
			if ( el->isRhsGet() ) {
				code.append( IN_RHS_REF_FROM_QUAL_REF );
				code.appendHalf( 0 );

				code.append( el->rhsVal.length() );
				for ( Vector<RhsVal>::Iter rg = el->rhsVal; rg.lte(); rg++ ) {
					code.append( rg->prodEl->production->prodNum );
					code.append( rg->prodEl->pos );
				}
			}
			else { 
				code.append( IN_REF_FROM_QUAL_REF );
				code.appendHalf( 0 );
				code.appendHalf( el->offset );
			}
		}
		else if ( el->iterImpl != 0 ) {
			code.append( el->iterImpl->inRefFromCur );
			code.appendHalf( el->offset );
		}
		else if ( el->typeRef->type == TypeRef::Ref ) {
			code.append( IN_REF_FROM_REF );
			code.appendHalf( el->offset );
		}
		else {
			code.append( IN_REF_FROM_LOCAL );
			code.appendHalf( el->offset );
		}

		UniqueType *elUT = el->typeRef->uniqueType;
		if ( elUT->typeId == TYPE_ITER )
			elUT = el->typeRef->searchUniqueType;
		
		assert( qi->form == QualItem::Dot );

		ObjectDef *searchObjDef = elUT->objectDef();
		searchScope = searchObjDef->rootScope;

		count += 1;
	}
	return count;
}

void LangVarRef::loadQualification( Compiler *pd, CodeVect &code, 
		NameScope *rootScope, int lastPtrInQual, bool forWriting, bool revert ) const
{
	/* Start the search from the root object. */
	NameScope *searchScope = rootScope;

	for ( QualItemVect::Iter qi = *qual; qi.lte(); qi++ ) {
		/* Lookup the field int the current qualification. */
		ObjectField *el = searchScope->findField( qi->data );
		if ( el == 0 )
			error(qi->loc) << "cannot resolve qualification " << qi->data << endp;

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

		UniqueType *qualUT = loadField( pd, code, searchScope->owningObj, 
				el, lfForWriting, lfRevert );
		
		if ( qi->form == QualItem::Dot ) {
			/* Cannot a reference. Iterator yes (access of the iterator not
			 * hte current) */
			if ( qualUT->ptr() )
				error(loc) << "dot cannot be used to access a pointer" << endp;
		}
		else if ( qi->form == QualItem::Arrow ) {
			if ( qualUT->ptr() ) {
				/* This deref instruction exists to capture the pointer reverse
				 * execution purposes. */
				if ( pd->revertOn && qi.pos() == lastPtrInQual && forWriting ) {
					/* This is like a global load. */
					code.append( IN_PTR_ACCESS_WV );
				}
			}
			else {
				error(loc) << "arrow operator cannot be used to "
						"access this type" << endp;
			}
		}

		ObjectDef *searchObjDef = qualUT->objectDef();
		searchScope = searchObjDef->rootScope;
	}
}

void LangVarRef::loadContextObj( Compiler *pd, CodeVect &code, 
		int lastPtrInQual, bool forWriting ) const
{
	/* Start the search in the global object. */
	ObjectDef *rootObj = structDef->objectDef;

	if ( forWriting && lastPtrInQual < 0 ) {
		/* If we are writing an no reference was found in the qualification
		 * then load the gloabl with a revert. */
		if ( pd->revertOn )
			code.append( IN_LOAD_CONTEXT_WV );
		else
			code.append( IN_LOAD_CONTEXT_WC );
	}
	else {
		/* Either we are reading or we are loading a pointer that will be
		 * dereferenced. */
		code.append( IN_LOAD_CONTEXT_R );
	}

	loadQualification( pd, code, rootObj->rootScope, lastPtrInQual, forWriting, true );
}

void LangVarRef::loadGlobalObj( Compiler *pd, CodeVect &code, 
		int lastPtrInQual, bool forWriting ) const
{
	NameScope *scope = nspace != 0 ? nspace->rootScope : pd->rootNamespace->rootScope;

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

	loadQualification( pd, code, scope, lastPtrInQual, forWriting, true );
}

void LangVarRef::loadScopedObj( Compiler *pd, CodeVect &code, 
		NameScope *scope, int lastPtrInQual, bool forWriting ) const
{
//	NameScope *scope = nspace != 0 ? nspace->rootScope : pd->rootNamespace->rootScope;

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

	loadQualification( pd, code, scope, lastPtrInQual, forWriting, true );
}

void LangVarRef::loadInbuiltObject( Compiler *pd, CodeVect &code, 
		int lastPtrInQual, bool forWriting ) const
{
	/* Start the search in the local frame. */
	loadQualification( pd, code, scope, lastPtrInQual, forWriting, pd->revertOn );
}

void LangVarRef::loadLocalObj( Compiler *pd, CodeVect &code, 
		int lastPtrInQual, bool forWriting ) const
{
	/* Start the search in the local frame. */
	loadQualification( pd, code, scope, lastPtrInQual, forWriting, false );
}

void LangVarRef::loadObj( Compiler *pd, CodeVect &code, 
		int lastPtrInQual, bool forWriting ) const
{
	if ( nspaceQual != 0 && nspaceQual->qualNames.length() > 0 ) {
		Namespace *nspace = pd->rootNamespace->findNamespace( nspaceQual->qualNames[0] );
		loadScopedObj( pd, code, nspace->rootScope, lastPtrInQual, forWriting );
	}
	else if ( isInbuiltObject() )
		loadInbuiltObject( pd, code, lastPtrInQual, forWriting );
	else if ( isLocalRef() )
		loadLocalObj( pd, code, lastPtrInQual, forWriting );
	else if ( isStructRef() )
		loadContextObj( pd, code, lastPtrInQual, forWriting );
	else
		loadGlobalObj( pd, code, lastPtrInQual, forWriting );
}


bool castAssignment( Compiler *pd, CodeVect &code, UniqueType *destUT, 
		UniqueType *destSearchUT, UniqueType *srcUT )
{
	if ( destUT == srcUT )
		return true;

	/* Casting trees to any. */
	if ( destUT->typeId == TYPE_TREE && destUT->langEl == pd->anyLangEl &&
			srcUT->typeId == TYPE_TREE )
		return true;

	/* Setting a reference from a tree. */
	if ( destUT->typeId == TYPE_REF && srcUT->typeId == TYPE_TREE &&
			destUT->langEl == srcUT->langEl )
		return true;

	/* Setting a tree from a reference. */
	if ( destUT->typeId == TYPE_TREE && srcUT->typeId == TYPE_REF &&
			destUT->langEl == srcUT->langEl )
		return true;
	
	/* Setting an iterator from a tree. */
	if ( destUT->typeId == TYPE_ITER && srcUT->typeId == TYPE_TREE && 
			destSearchUT->langEl == srcUT->langEl )
		return true;
	
	/* Assigning nil to a tree. */
	if ( destUT->typeId == TYPE_TREE && srcUT->typeId == TYPE_NIL )
		return true;

	if ( destUT->typeId == TYPE_STRUCT && srcUT->typeId == TYPE_NIL )
		return true;

	if ( destUT->typeId == TYPE_GENERIC && srcUT->typeId == TYPE_NIL )
		return true;

	if ( destUT->typeId == TYPE_TREE && srcUT->typeId == TYPE_TREE &&
			srcUT->langEl == pd->anyLangEl )
		return true;

	return false;
}

void LangVarRef::setFieldIter( Compiler *pd, CodeVect &code, 
		ObjectDef *inObject, ObjectField *el, UniqueType *objUT,
		UniqueType *exprType, bool revert ) const
{
	code.append( el->iterImpl->inSetCurWC );
	code.appendHalf( el->offset );
}

void LangVarRef::setField( Compiler *pd, CodeVect &code, 
		ObjectDef *inObject, ObjectField *el,
		UniqueType *exprUT, bool revert ) const
{
	/* Ensure that the field is referenced. */
	inObject->referenceField( pd, el );

	if ( exprUT->val()  ) {
		if ( pd->revertOn && revert )
			code.append( el->inSetValWV );
		else
			code.append( el->inSetValWC );
	}
	else {
		if ( pd->revertOn && revert )
			code.append( el->inSetWV );
		else
			code.append( el->inSetWC );
	}

	/* Maybe write out an offset. */
	if ( el->useOffset() )
		code.appendHalf( el->offset );
}


UniqueType *LangVarRef::evaluate( Compiler *pd, CodeVect &code, bool forWriting ) const
{
	/* Lookup the loadObj. */
	VarRefLookup lookup = lookupField( pd );

	/* Load the object, if any. */
	loadObj( pd, code, lookup.lastPtrInQual, forWriting );

	/* Load the field. */
	UniqueType *ut = loadField( pd, code, lookup.inObject, 
			lookup.objField, forWriting, false );

	return ut;
}

bool LangVarRef::canTakeRef( Compiler *pd, VarRefLookup &lookup ) const
{
	bool canTake = false;

	/* If the var is not a local, it must be an attribute accessed
	 * via a local and attributes. */
	if ( lookup.inObject->type == ObjectDef::FrameType )
		canTake = true;
	else if ( isLocalRef() ) {
		if ( lookup.lastPtrInQual < 0 && ! lookup.uniqueType->ptr() ) 
			canTake = true;
	}

	return canTake;
}

void LangVarRef::verifyRefPossible( Compiler *pd, VarRefLookup &lookup ) const
{
	bool canTake = canTakeRef( pd, lookup );

	if ( !canTake ) {
		error(loc) << "can only take references of locals or "
				"attributes accessed via a local" << endp;
	}

	if ( lookup.objField->refActive )
		error(loc) << "reference currently active, cannot take another" << endp;
}

bool LangExpr::canTakeRef( Compiler *pd ) const
{
	bool canTake = false;

	if ( type == LangExpr::TermType && term->type == LangTerm::VarRefType ) {
		VarRefLookup lookup = term->varRef->lookupField( pd );
		if ( term->varRef->canTakeRef( pd, lookup ) )
			canTake = true;
	}

	return canTake;
}


/* Return the field referenced. */
ObjectField *LangVarRef::preEvaluateRef( Compiler *pd, CodeVect &code ) const
{
	VarRefLookup lookup = lookupField( pd );

	verifyRefPossible( pd, lookup );

	loadQualificationRefs( pd, code, scope );

	return lookup.objField;
}

/* Return the field referenced. */
ObjectField *LangVarRef::evaluateRef( Compiler *pd, CodeVect &code, long pushCount ) const
{
	VarRefLookup lookup = lookupField( pd );

	verifyRefPossible( pd, lookup );

	/* Ensure that the field is referenced. */
	lookup.inObject->referenceField( pd, lookup.objField );

	/* Note that we could have modified children. */
	if ( qual->length() == 0 )
		lookup.objField->refActive = true;

	/* Whenever we take a reference we have to assume writing and that the
	 * tree is dirty. */
	lookup.objField->dirtyTree = true;

	if ( qual->length() > 0 ) {
		if ( lookup.objField->isRhsGet() ) {
			code.append( IN_RHS_REF_FROM_QUAL_REF );
			code.appendHalf( pushCount );

			ObjectField *el = lookup.objField;
			code.append( el->rhsVal.length() );
			for ( Vector<RhsVal>::Iter rg = el->rhsVal; rg.lte(); rg++ ) {
				code.append( rg->prodEl->production->prodNum );
				code.append( rg->prodEl->pos );
			}
		}
		else {
			code.append( IN_REF_FROM_QUAL_REF );
			code.appendHalf( pushCount );
			code.appendHalf( lookup.objField->offset );
		}
	}
	else if ( lookup.objField->iterImpl != 0 ) {
		code.append( lookup.objField->iterImpl->inRefFromCur );
		code.appendHalf( lookup.objField->offset );
	}
	else if ( lookup.objField->typeRef->type == TypeRef::Ref ) {
		code.append( IN_REF_FROM_REF );
		code.appendHalf( lookup.objField->offset );
	}
	else {
		code.append( IN_REF_FROM_LOCAL );
		code.appendHalf( lookup.objField->offset );
	}

	return lookup.objField;
}

IterImpl *LangVarRef::chooseTriterCall( Compiler *pd,
		UniqueType *searchUT, CallArgVect *args )
{
	IterImpl *iterImpl = 0;

	/* Evaluate the triter args and choose the triter call based on it. */
	if ( args->length() == 1 ) {
		/* Evaluate the expression. */
		CodeVect unused;
		CallArgVect::Iter pe = *args;
		UniqueType *exprUT = (*pe)->expr->evaluate( pd, unused );

		if ( exprUT->typeId == TYPE_GENERIC && exprUT->generic->typeId == GEN_LIST ) {
			if ( searchUT == exprUT->generic->elUt )
				iterImpl = new IterImpl( IterImpl::ListEl );
			else
				iterImpl = new IterImpl( IterImpl::ListVal );
		}

		if ( exprUT->typeId == TYPE_GENERIC && exprUT->generic->typeId == GEN_MAP ) {
			if ( searchUT == exprUT->generic->elUt )
				iterImpl = new IterImpl( IterImpl::MapEl );
			else
				iterImpl = new IterImpl( IterImpl::MapVal );
		}
	}

	if ( iterImpl == 0 )
		iterImpl = new IterImpl( IterImpl::Tree );

	return iterImpl;
}

ObjectField **LangVarRef::evaluateArgs( Compiler *pd, CodeVect &code, 
		VarRefLookup &lookup, CallArgVect *args )
{
	/* Parameter list is given only for user defined methods. Otherwise it
	 * will be null. */
	ParameterList *paramList = lookup.objMethod->paramList;

	/* Match the number of arguments. */
	int numArgs = args != 0 ? args->length() : 0;
	if ( numArgs != lookup.objMethod->numParams )
		error(loc) << "wrong number of arguments" << endp;

	/* This is for storing the object fields used by references. */
	ObjectField **paramRefs = new ObjectField*[numArgs];
	memset( paramRefs, 0, sizeof(ObjectField*) * numArgs );

	/* Done now if there are no args. */
	if ( args == 0 )
		return paramRefs;

	/* We use this only if there is a paramter list. */
	ParameterList::Iter p;
	long size = 0;
	long tempPops = 0;
	long pos = 0;

	paramList != 0 && ( p = *paramList );
	for ( CallArgVect::Iter pe = *args; pe.lte(); pe++ ) {
		/* Get the expression and the UT for the arg. */
		LangExpr *expression = (*pe)->expr;
		UniqueType *paramUT = lookup.objMethod->paramUTs[pe.pos()];

		if ( paramUT->typeId == TYPE_REF ) {
			if ( expression->canTakeRef( pd ) ) {
				/* Push object loads for reference parameters. */
				LangVarRef *varRef = expression->term->varRef;
				ObjectField *refOf = varRef->preEvaluateRef( pd, code );
				paramRefs[pe.pos()] = refOf;

				size += varRef->qual->length() * 2;
				(*pe)->offQualRef = size;
			/**/

				refOf = varRef->evaluateRef( pd, code, 0 ); //(size - (*pe)->offQualRef) );
				paramRefs[pe.pos()] = refOf;

				//size += 2;
			}
			else {
				/* First pass we need to allocate and evaluate temporaries. */
				UniqueType *exprUT = expression->evaluate( pd, code );

				(*pe)->exprUT = exprUT;

				size += 1;
				(*pe)->offTmp = size;
				tempPops += 1;
			/**/
				code.append( IN_REF_FROM_BACK );
				code.appendHalf( 0 ); //size - (*pe)->offTmp );

				//size += 2;
			}

			if ( lookup.objMethod->func ) {
				code.append( IN_STASH_ARG );
				code.appendHalf( pos );
				code.appendHalf( 2 );
			}

			pos += 2;
		}
		else {
			UniqueType *exprUT = expression->evaluate( pd, code );
			// pd->unwindCode.remove( 0, 1 );

			if ( !castAssignment( pd, code, paramUT, 0, exprUT ) )
				error(loc) << "arg " << pe.pos()+1 << " is of the wrong type" << endp;

			size += 1;

			if ( lookup.objMethod->func && !lookup.objMethod->func->inHost ) {
				code.append( IN_STASH_ARG );
				code.appendHalf( pos );
				code.appendHalf( 1 );
			}

			pos += 1;
		}

		/* Advance the parameter list iterator if we have it. */
		paramList != 0 && p.increment();
	}

	argSize = tempPops;

	return paramRefs;
}

void LangVarRef::resetActiveRefs( Compiler *pd, VarRefLookup &lookup,
		ObjectField **paramRefs ) const
{
	/* Parameter list is given only for user defined methods. Otherwise it
	 * will be null. */
	for ( long p = 0; p < lookup.objMethod->numParams; p++ ) {
		if ( paramRefs[p] != 0 )
			paramRefs[p]->refActive = false;
	}
}

void LangVarRef::callOperation( Compiler *pd, CodeVect &code, VarRefLookup &lookup ) const
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
	bool revert = lookup.lastPtrInQual >= 0 || !isLocalRef() || isInbuiltObject();
	bool unwind = false;
	
	/* The call instruction. */
	if ( pd->revertOn && revert )  {
		if ( lookup.objMethod->opcodeWV == IN_PARSE_FINISH_WV ) {
			code.append( IN_PARSE_LOAD );
			code.append( IN_PARSE_FINISH_WV );
			code.appendHalf( 0 );
			code.append( IN_PCR_CALL );
			code.append( IN_PARSE_FINISH_EXIT_WV );

			code.append( IN_GET_PARSER_MEM_R );
			code.appendHalf( 0 );
		}
		else {
			if ( lookup.objMethod->opcodeWV == IN_CALL_WV ||
					lookup.objMethod->opcodeWC == IN_EXIT )
				unwind = true;

			if ( lookup.objMethod->useFnInstr )
				code.append( IN_FN );
			code.append( lookup.objMethod->opcodeWV );
		}
	}
	else {
		if ( lookup.objMethod->opcodeWC == IN_PARSE_FINISH_WC ) {
			code.append( IN_PARSE_LOAD );
			code.append( IN_PARSE_FINISH_WC );
			code.appendHalf( 0 );
			code.append( IN_PCR_CALL );
			code.append( IN_PARSE_FINISH_EXIT_WC );

			code.append( IN_GET_PARSER_MEM_R );
			code.appendHalf( 0 );
		}
		else {
			if ( lookup.objMethod->opcodeWC == IN_CALL_WC ||
					lookup.objMethod->opcodeWC == IN_EXIT )
				unwind = true;

			if ( lookup.objMethod->useFnInstr )
				code.append( IN_FN );
			code.append( lookup.objMethod->opcodeWC );
		}
	}
	
	if ( lookup.objMethod->useFuncId )
		code.appendHalf( lookup.objMethod->funcId );

	if ( lookup.objMethod->useGenericId )
		code.appendHalf( lookup.objMethod->generic->id );
	
	if ( unwind ) {
		if ( pd->unwindCode.length() == 0 )
			code.appendHalf( 0 );
		else {
			code.appendHalf( pd->unwindCode.length() + 1 );
			code.append( pd->unwindCode );
			code.append( IN_DONE );
		}
	}
}

void LangVarRef::popRefQuals( Compiler *pd, CodeVect &code, 
		VarRefLookup &lookup, CallArgVect *args, bool temps ) const
{
	long popCount = 0;

	/* Evaluate and push the args. */
	if ( args != 0 ) {
		for ( CallArgVect::Iter pe = *args; pe.lte(); pe++ ) {
			/* Get the expression and the UT for the arg. */
			LangExpr *expression = (*pe)->expr;
			UniqueType *paramUT = lookup.objMethod->paramUTs[pe.pos()];

			if ( paramUT->typeId == TYPE_REF ) {
				if ( expression->canTakeRef( pd ) ) {
					LangVarRef *varRef = expression->term->varRef;
					popCount += varRef->qual->length() * 2;
				}
			}
		}

		if ( popCount > 0 ) {
			code.append( IN_POP_N_WORDS );
			code.appendHalf( (short)popCount );
		}

		if ( temps ) {
			for ( CallArgVect::Iter pe = *args; pe.lte(); pe++ ) {
				/* Get the expression and the UT for the arg. */
				LangExpr *expression = (*pe)->expr;
				UniqueType *paramUT = lookup.objMethod->paramUTs[pe.pos()];

				if ( paramUT->typeId == TYPE_REF ) {
					if ( ! expression->canTakeRef( pd ) )
						code.append( IN_POP_TREE );
				}
			}
		}
	}
}


UniqueType *LangVarRef::evaluateCall( Compiler *pd, CodeVect &code, CallArgVect *args ) 
{
	/* Evaluate the object. */
	VarRefLookup lookup = lookupMethod( pd );

	Function *func = lookup.objMethod->func;

	/* Prepare the contiguous call args space. */
	int asLoc;
	if ( func != 0 && !func->inHost ) {
		code.append( IN_PREP_ARGS );
		asLoc = code.length();
		code.appendHalf( 0 );
	}

	/* Evaluate and push the arguments. */
	ObjectField **paramRefs = evaluateArgs( pd, code, lookup, args );

	/* Write the call opcode. */
	callOperation( pd, code, lookup );

	popRefQuals( pd, code, lookup, args, true );

	resetActiveRefs( pd, lookup, paramRefs);
	delete[] paramRefs;

	if ( func != 0 && !func->inHost ) {
		code.append( IN_CLEAR_ARGS );
		code.appendHalf( func->paramListSize );
		code.setHalf( asLoc, func->paramListSize );
	}

	if ( func != 0 && !func->inHost )
		code.append( IN_LOAD_RETVAL );

	/* Return the type to the expression. */
	return lookup.uniqueType;
}

/* Can match on a tree or a ref. A tree always comes back. */
UniqueType *LangTerm::evaluateMatch( Compiler *pd, CodeVect &code ) const
{
	/* Add the vars bound by the pattern into the local scope. */
	for ( PatternItemList::Iter item = *pattern->list; item.lte(); item++ ) {
		if ( item->varRef != 0 )
			item->bindId = pattern->nextBindId++;
	}

	UniqueType *ut = varRef->evaluate( pd, code );
	if ( ut->typeId != TYPE_TREE && ut->typeId != TYPE_REF ) {
		error(varRef->loc) << "expected match against a tree/ref type" << endp;
	}

	/* Store the language element type in the pattern. This is needed by
	 * the pattern parser. */
	pattern->langEl = ut->langEl;

	code.append( IN_MATCH );
	code.appendHalf( pattern->patRepId );

	for ( PatternItemList::Iter item = pattern->list->last(); item.gtb(); item-- ) {
		if ( item->varRef != 0 ) {
			/* Compute the unique type. */
			UniqueType *exprType = pd->findUniqueType( TYPE_TREE, item->prodEl->langEl );

			/* Get the type of the variable being assigned to. */
			VarRefLookup lookup = item->varRef->lookupField( pd );

			item->varRef->loadObj( pd, code, lookup.lastPtrInQual, false );
			item->varRef->setField( pd, code, lookup.inObject,
					lookup.objField, exprType, false );
		}
	}

	/* The process of matching turns refs into trees. */
	if ( ut->typeId == TYPE_REF )
		ut = pd->findUniqueType( TYPE_TREE, ut->langEl );

	return ut;
}

void LangTerm::evaluateCapture( Compiler *pd, CodeVect &code, UniqueType *valUt ) const
{
	if ( varRef != 0 ) {
		/* Get the type of the variable being assigned to. */
		VarRefLookup lookup = varRef->lookupField( pd );

		/* Need a copy of the tree. */
		code.append( lookup.uniqueType->tree() ? IN_DUP_TREE : IN_DUP_VAL );

		varRef->loadObj( pd, code, lookup.lastPtrInQual, false );
		varRef->setField( pd, code, lookup.inObject, lookup.objField, valUt, false );
	}
}

UniqueType *LangTerm::evaluateNew( Compiler *pd, CodeVect &code ) const
{
	/* What is being newstructed. */
	UniqueType *newUT = typeRef->uniqueType;

	if ( newUT->typeId != TYPE_STRUCT && newUT->typeId != TYPE_GENERIC )
		error(loc) << "can only new a struct or generic" << endp;

	bool context = false;
	if ( newUT->typeId == TYPE_GENERIC &&
			newUT->generic->typeId == GEN_PARSER &&
			newUT->generic->elUt->langEl->contextIn != 0 )
	{
		if ( fieldInitArgs == 0 || fieldInitArgs->length() != 1 )
			error(loc) << "parse command requires just input" << endp;
		context = true;
	}

	if ( newUT->typeId == TYPE_GENERIC ) {
		code.append( IN_CONS_GENERIC );
		code.appendHalf( newUT->generic->id );

		if ( newUT->generic->typeId == GEN_PARSER ) {

		}
	}
	else if ( newUT->typeId == TYPE_STRUCT && newUT->structEl == pd->streamSel ) {
		code.append( IN_NEW_STREAM );
	}
	else {
		code.append( IN_NEW_STRUCT );
		code.appendHalf( newUT->structEl->id );
	}

	/*
	 * First load the context into the parser.
	 */
	if ( context ) {
		/* Eval the context. */
		UniqueType *argUT = fieldInitArgs->data[0]->expr->evaluate( pd, code );

		if ( argUT != pd->uniqueTypeStream && argUT->typeId != TYPE_STRUCT )
			error(loc) << "context argument must be a stream or a tree" << endp;

		/* Store the context. Leaves the parser on the stack. */
		code.append( IN_SET_PARSER_CONTEXT );
	}

	evaluateCapture( pd, code, newUT );

	return newUT;
}

UniqueType *LangTerm::evaluateCast( Compiler *pd, CodeVect &code ) const
{
	expr->evaluate( pd, code );
	code.append( IN_TREE_CAST );
	code.appendHalf( typeRef->uniqueType->langEl->id );
	return typeRef->uniqueType;
}

void LangTerm::assignFieldArgs( Compiler *pd, CodeVect &code, UniqueType *replUT ) const
{
	/* Now assign the field initializations. Note that we need to do this in
	 * reverse because the last expression evaluated is at the top of the
	 * stack. */
	if ( fieldInitArgs != 0 && fieldInitArgs->length() > 0 ) {
		ObjectDef *objDef = replUT->objectDef();
		/* Note the reverse traversal. */
		for ( FieldInitVect::Iter pi = fieldInitArgs->last(); pi.gtb(); pi-- ) {
			FieldInit *fieldInit = *pi;
			ObjectField *field = objDef->findFieldNum( pi.pos() );
			if ( field == 0 ) {
				error(fieldInit->loc) << "failed to find init pos " << 
					pi.pos() << " in object" << endp;
			}

			/* Lookup the type of the field and compare it to the type of the
			 * expression. */
			UniqueType *fieldUT = field->typeRef->uniqueType;
			if ( !castAssignment( pd, code, fieldUT, 0, fieldInit->exprUT ) )
				error(fieldInit->loc) << "type mismatch in initialization" << endp;

			/* The set field instruction must leave the object on the top of
			 * the stack. */
			code.append( IN_SET_FIELD_TREE_LEAVE_WC );
			code.appendHalf( field->offset );
		}
	}
}

UniqueType *LangTerm::evaluateConstruct( Compiler *pd, CodeVect &code ) const
{
	/* Evaluate the initialization expressions. */
	if ( fieldInitArgs != 0 && fieldInitArgs->length() > 0 ) {
		for ( FieldInitVect::Iter pi = *fieldInitArgs; pi.lte(); pi++ ) {
			FieldInit *fieldInit = *pi;
			fieldInit->exprUT = fieldInit->expr->evaluate( pd, code );
		}
	}

	/* Assign bind ids to the variables in the replacement. */
	for ( ConsItemList::Iter item = *constructor->list; item.lte(); item++ ) {
		if ( item->expr != 0 )
			item->bindId = constructor->nextBindId++;
	}

	/* Evaluate variable references. */
	for ( ConsItemList::Iter item = constructor->list->last(); item.gtb(); item-- ) {
		if ( item->type == ConsItem::ExprType ) {
			UniqueType *ut = item->expr->evaluate( pd, code );
		
			if ( ut->typeId != TYPE_TREE ) {
				error(constructor->loc) << "variables used in "
						"replacements must be trees" << endp;
			}

			if ( !isStr( ut ) ) {
				if ( item->trim ) 
					code.append( IN_TREE_TRIM );
			}

			item->langEl = ut->langEl;
		}
	}

	/* Construct the tree using the tree information stored in the compiled
	 * code. */
	code.append( IN_CONSTRUCT );
	code.appendHalf( constructor->patRepId );

	/* Lookup the type of the replacement and store it in the replacement
	 * object so that replacement parsing has a target. */
	UniqueType *replUT = typeRef->uniqueType;
	if ( replUT->typeId != TYPE_TREE )
		error(loc) << "don't know how to construct this type" << endp;
	
	constructor->langEl = replUT->langEl;
	assignFieldArgs( pd, code, replUT );

	evaluateCapture( pd, code, replUT );

	return replUT;
}


void LangTerm::parseFrag( Compiler *pd, CodeVect &code, int stopId ) const
{
	/* Parse instruction, dependent on whether or not we are producing
	 * revert or commit code. */
	if ( pd->revertOn ) {
		code.append( IN_PARSE_LOAD );
		code.append( IN_PARSE_FRAG_WV );
		code.appendHalf( stopId );
		code.append( IN_PCR_CALL );
		code.append( IN_PARSE_FRAG_EXIT_WV );
	}
	else {
		code.append( IN_PARSE_LOAD );
		code.append( IN_PARSE_FRAG_WC );
		code.appendHalf( stopId );
		code.append( IN_PCR_CALL );
		code.append( IN_PARSE_FRAG_EXIT_WC );
	}
}

UniqueType *LangTerm::evaluateParse( Compiler *pd, CodeVect &code,
		bool tree, bool stop ) const
{
	UniqueType *parserUT = typeRef->uniqueType;
	UniqueType *targetUT = parserUT->generic->elUt;

	/* If this is a parse stop then we need to verify that the type is
	 * compatible with parse stop. */
	if ( stop )
		targetUT->langEl->parseStop = true;
	int stopId = stop ? targetUT->langEl->id : 0;

	bool context = false;
	if ( targetUT->langEl->contextIn != 0 ) {
		if ( fieldInitArgs == 0 || fieldInitArgs->length() != 1 )
			error(loc) << "parse command requires just input" << endp;
		context = true;
	}

	/* Evaluate variable references. */
	for ( ConsItemList::Iter item = consItemList->last(); item.gtb(); item-- ) {
		if ( item->type == ConsItem::ExprType ) {
			UniqueType *ut = item->expr->evaluate( pd, code );
		
			if ( ut->typeId != TYPE_TREE )
				error() << "variables used in replacements must be trees" << endp;

			item->langEl = ut->langEl;
		}
	}

	/* Construct the parser. */

	if ( parserText->reduce ) {
		code.append( IN_CONS_REDUCER );
		code.appendHalf( parserUT->generic->id );
		code.appendHalf( parserText->reducerId );
	}
	else {
		code.append( IN_CONS_GENERIC );
		code.appendHalf( parserUT->generic->id );
	}

	/*
	 * First load the context into the parser.
	 */
	if ( context ) {
		/* Eval the context. */
		UniqueType *argUT = fieldInitArgs->data[0]->expr->evaluate( pd, code );

		if ( argUT != pd->uniqueTypeStream && argUT->typeId != TYPE_STRUCT )
			error(loc) << "context argument must be a stream or a tree" << endp;

		/* Store the context. */
		code.append( IN_SET_PARSER_CONTEXT );
	}

	/*****************************/
	
	/* Assign bind ids to the variables in the replacement. */
	for ( ConsItemList::Iter item = *parserText->list; item.lte(); item++ ) {
		bool isStream = false;
		switch ( item->type ) {
		case ConsItem::LiteralType: {
			String result;
			bool unusedCI;
			prepareLitString( result, unusedCI, 
					item->prodEl->typeRef->pdaLiteral->data,
					item->prodEl->typeRef->pdaLiteral->loc );

			/* Make sure we have this string. */
			StringMapEl *mapEl = 0;
			if ( pd->literalStrings.insert( result, &mapEl ) )
				mapEl->value = pd->literalStrings.length()-1;

			code.append( IN_LOAD_STR );
			code.appendWord( mapEl->value );
			break;
		}
		case ConsItem::InputText: {
			/* Make sure we have this string. */
			StringMapEl *mapEl = 0;
			if ( pd->literalStrings.insert( item->data, &mapEl ) )
				mapEl->value = pd->literalStrings.length()-1;

			code.append( IN_LOAD_STR );
			code.appendWord( mapEl->value );
			break;
		}
		case ConsItem::ExprType:
			UniqueType *ut = item->expr->evaluate( pd, code );

			if ( ut->typeId == TYPE_TREE && ut->langEl == pd->voidLangEl ) {
				/* Clear it away if return type is void. */
				code.append( IN_POP_TREE );
				continue;
			}

			if ( ut->typeId == TYPE_INT || ut->typeId == TYPE_BOOL )
				code.append( IN_INT_TO_STR );

			if ( ut == pd->uniqueTypeStream )
				isStream = true;

			if ( !tree && ut->typeId == TYPE_TREE &&
					ut->langEl != pd->strLangEl && ut != pd->uniqueTypeStream )
			{
				/* Convert it to a string. */
				code.append( IN_TREE_TO_STR_TRIM );
			}
			break;
		}

		if ( isStream ) {
			if ( pd->revertOn )
				code.append( IN_PARSE_APPEND_STREAM_WV );
			else
				code.append( IN_PARSE_APPEND_STREAM_WC );
		}
		else {
			if ( pd->revertOn )
				code.append( IN_PARSE_APPEND_WV );
			else
				code.append( IN_PARSE_APPEND_WC );
		}

		/* Parse instruction, dependent on whether or not we are producing
		 * revert or commit code. */
		parseFrag( pd, code, stopId );
	}

	/*
	 * Finish operation
	 */

	/* Parse instruction, dependent on whether or not we are producing revert
	 * or commit code. */
	if ( pd->revertOn ) {
		/* Finish immediately. */
		code.append( IN_PARSE_LOAD );
		code.append( IN_PARSE_FINISH_WV );
		code.appendHalf( stopId );
		code.append( IN_PCR_CALL );
		code.append( IN_PARSE_FINISH_EXIT_WV );
	}
	else {
		/* Finish immediately. */
		code.append( IN_PARSE_LOAD );
		code.append( IN_PARSE_FINISH_WC );
		code.appendHalf( stopId );
		code.append( IN_PCR_CALL );
		code.append( IN_PARSE_FINISH_EXIT_WC );
	}

	/* Parser is on the top of the stack. */

	/* Pull out the error and save it off. */
	code.append( IN_DUP_VAL );
	code.append( IN_GET_PARSER_MEM_R );
	code.appendHalf( 1 );
	code.append( IN_SET_ERROR );

	/* Replace the parser with the parsed tree. */
	code.append( IN_GET_PARSER_MEM_R );
	code.appendHalf( 0 );

	/* Capture to the local var. */
	evaluateCapture( pd, code, targetUT );

	return targetUT;
}

void ConsItemList::evaluateSendStream( Compiler *pd, CodeVect &code )
{
	for ( ConsItemList::Iter item = first(); item.lte(); item++ ) {
		/* Load a dup of the stream. */
		code.append( IN_DUP_VAL );

		switch ( item->type ) {
		case ConsItem::LiteralType: {
			String result;
			bool unusedCI;
			prepareLitString( result, unusedCI, 
					item->prodEl->typeRef->pdaLiteral->data,
					item->prodEl->typeRef->pdaLiteral->loc );

			/* Make sure we have this string. */
			StringMapEl *mapEl = 0;
			if ( pd->literalStrings.insert( result, &mapEl ) )
				mapEl->value = pd->literalStrings.length()-1;

			code.append( IN_LOAD_STR );
			code.appendWord( mapEl->value );
			break;
		}
		case ConsItem::InputText: {
			/* Make sure we have this string. */
			StringMapEl *mapEl = 0;
			if ( pd->literalStrings.insert( item->data, &mapEl ) )
				mapEl->value = pd->literalStrings.length()-1;

			code.append( IN_LOAD_STR );
			code.appendWord( mapEl->value );
			break;
		}
		case ConsItem::ExprType:
			UniqueType *ut = item->expr->evaluate( pd, code );
			if ( ut->typeId == TYPE_TREE && ut->langEl == pd->voidLangEl ) {
				/* Clear it away if the the return type is void. */
				code.append( IN_POP_TREE );
				code.append( IN_POP_TREE );
				continue;
			}
			else if ( ut->typeId == TYPE_TREE && !isStr( ut ) ) {
				if ( item->trim )
					code.append( IN_TREE_TRIM );
			}

			if ( ut->typeId == TYPE_INT || ut->typeId == TYPE_BOOL )
				code.append( IN_INT_TO_STR );
			break;
		}

		code.append( IN_PRINT_STREAM );
		code.append( 1 );
	}
}

void LangTerm::evaluateSendStream( Compiler *pd, CodeVect &code ) const
{
	varRef->evaluate( pd, code );
	parserText->list->evaluateSendStream( pd, code );

	/* Normally we would have to pop the stream var ref that we evaluated
	 * before all the print arguments (which includes the stream, evaluated
	 * last), however we send is part of an expression, and is supposed to
	 * leave the varref on the stack. */
}

void LangTerm::evaluateSendParser( Compiler *pd, CodeVect &code, bool strings ) const
{
	varRef->evaluate( pd, code );

	/* Assign bind ids to the variables in the replacement. */
	for ( ConsItemList::Iter item = *parserText->list; item.lte(); item++ ) {
		bool isStream = false;
		switch ( item->type ) {
		case ConsItem::LiteralType: {
			String result;
			bool unusedCI;
			prepareLitString( result, unusedCI, 
					item->prodEl->typeRef->pdaLiteral->data,
					item->prodEl->typeRef->pdaLiteral->loc );

			/* Make sure we have this string. */
			StringMapEl *mapEl = 0;
			if ( pd->literalStrings.insert( result, &mapEl ) )
				mapEl->value = pd->literalStrings.length()-1;

			code.append( IN_LOAD_STR );
			code.appendWord( mapEl->value );
			break;
		}
		case ConsItem::InputText: {
			/* Make sure we have this string. */
			StringMapEl *mapEl = 0;
			if ( pd->literalStrings.insert( item->data, &mapEl ) )
				mapEl->value = pd->literalStrings.length()-1;

			code.append( IN_LOAD_STR );
			code.appendWord( mapEl->value );
			break;
		}
		case ConsItem::ExprType:
			UniqueType *ut = item->expr->evaluate( pd, code );
			if ( ut->typeId == TYPE_TREE && ut->langEl == pd->voidLangEl ) {
				/* Clear it away if return type is void. */
				code.append( IN_POP_TREE );
				continue;
			}

			if ( ut == pd->uniqueTypeStream )
				isStream = true;

			if ( ut->typeId == TYPE_INT || ut->typeId == TYPE_BOOL )
				code.append( IN_INT_TO_STR );

			if ( strings && ut->typeId == TYPE_TREE &&
					ut->langEl != pd->strLangEl && ut != pd->uniqueTypeStream )
			{
				/* Convert it to a string. */
				code.append( IN_TREE_TO_STR );
			}
			break;
		}

		if ( isStream ) {
			if ( pd->revertOn )
				code.append( IN_PARSE_APPEND_STREAM_WV );
			else
				code.append( IN_PARSE_APPEND_STREAM_WC );
		}
		else {
			if ( pd->revertOn )
				code.append( IN_PARSE_APPEND_WV );
			else
				code.append( IN_PARSE_APPEND_WC );
		}

		parseFrag( pd, code, 0 );
	}

	if ( eof ) { 
		if ( pd->revertOn ) {
			code.append( IN_PARSE_LOAD );
			code.append( IN_PARSE_FINISH_WV );
			code.appendHalf( 0 );
			code.append( IN_PCR_CALL );
			code.append( IN_PARSE_FINISH_EXIT_WV );
		}
		else {
			code.append( IN_PARSE_LOAD );
			code.append( IN_PARSE_FINISH_WC );
			code.appendHalf( 0 );
			code.append( IN_PCR_CALL );
			code.append( IN_PARSE_FINISH_EXIT_WC );
		}
	}
}

UniqueType *LangTerm::evaluateSend( Compiler *pd, CodeVect &code ) const
{
	UniqueType *varUt = varRef->lookup( pd );

	if ( varUt == pd->uniqueTypeStream )
		evaluateSendStream( pd, code );
	else if ( varUt->parser() )
		evaluateSendParser( pd, code, true );
	else
		error(loc) << "can only send to parsers and streams" << endl;

	return varUt;
}


UniqueType *LangTerm::evaluateSendTree( Compiler *pd, CodeVect &code ) const
{
	UniqueType *varUt = varRef->lookup( pd );

	if ( varUt->parser() )
		evaluateSendParser( pd, code, false );
	else
		error(loc) << "can only send_tree to parsers" << endl;

	return varUt;
}

UniqueType *LangTerm::evaluateEmbedString( Compiler *pd, CodeVect &code ) const
{
	/* Assign bind ids to the variables in the replacement. */
	for ( ConsItemList::Iter item = *consItemList; item.lte(); item++ ) {
		switch ( item->type ) {
		case ConsItem::LiteralType: {
			String result;
			bool unusedCI;
			prepareLitString( result, unusedCI, 
					item->prodEl->typeRef->pdaLiteral->data,
					item->prodEl->typeRef->pdaLiteral->loc );

			/* Make sure we have this string. */
			StringMapEl *mapEl = 0;
			if ( pd->literalStrings.insert( result, &mapEl ) )
				mapEl->value = pd->literalStrings.length()-1;

			code.append( IN_LOAD_STR );
			code.appendWord( mapEl->value );
			break;
		}
		case ConsItem::InputText: {
			/* Make sure we have this string. */
			StringMapEl *mapEl = 0;
			if ( pd->literalStrings.insert( item->data, &mapEl ) )
				mapEl->value = pd->literalStrings.length()-1;

			code.append( IN_LOAD_STR );
			code.appendWord( mapEl->value );
			break;
		}
		case ConsItem::ExprType: {
			UniqueType *ut = item->expr->evaluate( pd, code );

			if ( ut->typeId == TYPE_INT || ut->typeId == TYPE_BOOL )
				code.append( IN_INT_TO_STR );

			if ( ut->typeId == TYPE_TREE &&
					ut->langEl != pd->strLangEl && ut != pd->uniqueTypeStream )
			{
				/* Convert it to a string. */
				code.append( IN_TREE_TO_STR_TRIM );
			}
			break;
		}}
	}

	/* If there was nothing loaded, load the empty string. We must produce
	 * something. */
	if ( consItemList->length() == 0 ) {
		String result = "";

		/* Make sure we have this string. */
		StringMapEl *mapEl = 0;
		if ( pd->literalStrings.insert( result, &mapEl ) )
			mapEl->value = pd->literalStrings.length()-1;

		code.append( IN_LOAD_STR );
		code.appendWord( mapEl->value );
	}

	long items = consItemList->length();
	for ( long i = 0; i < items-1; i++ )
		code.append( IN_CONCAT_STR );

	return pd->uniqueTypeStr;
}

UniqueType *LangTerm::evaluateSearch( Compiler *pd, CodeVect &code ) const
{
	UniqueType *ut = typeRef->uniqueType;
	if ( ut->typeId != TYPE_TREE )
		error(loc) << "can only search for tree types" << endp;

	/* Evaluate the expression. */
	UniqueType *treeUT = varRef->evaluate( pd, code );
	if ( treeUT->typeId != TYPE_TREE && treeUT->typeId != TYPE_REF )
		error(loc) << "search can be applied only to tree/ref types" << endp;

	/* Run the search. */
	code.append( IN_TREE_SEARCH );
	code.appendWord( ut->langEl->id );
	return ut;
}

UniqueType *LangTerm::evaluate( Compiler *pd, CodeVect &code ) const
{
	UniqueType *retUt = 0;
	switch ( type ) {
		case VarRefType:
			retUt = varRef->evaluate( pd, code );
			break;
		case MethodCallType:
			retUt = varRef->evaluateCall( pd, code, args );
			break;
		case NilType:
			code.append( IN_LOAD_NIL );
			retUt = pd->uniqueTypeNil;
			break;
		case TrueType:
			code.append( IN_LOAD_TRUE );
			retUt = pd->uniqueTypeBool;
			break;
		case FalseType:
			code.append( IN_LOAD_FALSE );
			retUt = pd->uniqueTypeBool;
			break;
		case MakeTokenType:
			retUt = evaluateMakeToken( pd, code );
			break;
		case MakeTreeType:
			retUt = evaluateMakeTree( pd, code );
			break;
		case NumberType: {
			unsigned int n = atoi( data );
			code.append( IN_LOAD_INT );
			code.appendWord( n );
			retUt = pd->uniqueTypeInt;
			break;
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
			retUt = pd->uniqueTypeStr;
			break;
		}
		case MatchType:
			retUt = evaluateMatch( pd, code );
			break;
		case ParseType:
			retUt = evaluateParse( pd, code, false, false );
			break;
		case ParseTreeType:
			retUt = evaluateParse( pd, code, true, false );
			break;
		case ParseStopType:
			retUt = evaluateParse( pd, code, false, true );
			break;
		case ConstructType:
			retUt = evaluateConstruct( pd, code );
			break;
		case SendType:
			retUt = evaluateSend( pd, code );
			break;
		case SendTreeType:
			retUt = evaluateSendTree( pd, code );
			break;
		case NewType:
			retUt = evaluateNew( pd, code );
			break;
		case TypeIdType: {
			/* Evaluate the expression. */
			UniqueType *ut = typeRef->uniqueType;
			if ( ut->typeId != TYPE_TREE )
				error() << "typeid can only be applied to tree types" << endp;

			code.append( IN_LOAD_INT );
			code.appendWord( ut->langEl->id );
			retUt = pd->uniqueTypeInt;
			break;
		}
		case SearchType:
			retUt = evaluateSearch( pd, code );
			break;
		case EmbedStringType:
			retUt = evaluateEmbedString( pd, code );
			break;
		case CastType:
			retUt = evaluateCast( pd, code );
			break;
	}

	// if ( retUt->val() )
	//	pd->unwindCode.insert( 0, IN_POP_VAL );
	// else
	//	pd->unwindCode.insert( 0, IN_POP_TREE );

	return retUt;
}

UniqueType *LangExpr::evaluate( Compiler *pd, CodeVect &code ) const
{
	switch ( type ) {
		case BinaryType: {
			switch ( op ) {
				case '+': {
					UniqueType *lt = left->evaluate( pd, code );
					UniqueType *rt = right->evaluate( pd, code );

					// pd->unwindCode.remove( 0, 2 );
					// pd->unwindCode.insert( 0, IN_POP_TREE );

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
				case '/': {
					UniqueType *lt = left->evaluate( pd, code );
					UniqueType *rt = right->evaluate( pd, code );

					if ( lt == pd->uniqueTypeInt && rt == pd->uniqueTypeInt ) {
						code.append( IN_DIV_INT );
						return pd->uniqueTypeInt;
					}

					error(loc) << "do not have an division"
							"operator for these types" << endp;
					break;
				}
				case OP_DoubleEql: {
					UniqueType *lt = left->evaluate( pd, code );
					UniqueType *rt = right->evaluate( pd, code );

					if ( lt != rt )
						error(loc) << "comparison of different types" << endp;
						
					if ( lt->val() )
						code.append( IN_TST_EQL_VAL );
					else
						code.append( IN_TST_EQL_TREE );
					return pd->uniqueTypeBool;
				}
				case OP_NotEql: {
					UniqueType *lt = left->evaluate( pd, code );
					UniqueType *rt = right->evaluate( pd, code );

					if ( lt != rt )
						error(loc) << "comparison of different types" << endp;

					if ( lt->val() )
						code.append( IN_TST_NOT_EQL_VAL );
					else
						code.append( IN_TST_NOT_EQL_TREE );

					return pd->uniqueTypeBool;
				}
				case '<': {
					UniqueType *lt = left->evaluate( pd, code );
					UniqueType *rt = right->evaluate( pd, code );

					if ( lt != rt )
						error(loc) << "comparison of different types" << endp;

					if ( lt->val() )
						code.append( IN_TST_LESS_VAL );
					else
						code.append( IN_TST_LESS_TREE );
					return pd->uniqueTypeBool;
				}
				case '>': {
					UniqueType *lt = left->evaluate( pd, code );
					UniqueType *rt = right->evaluate( pd, code );

					if ( lt != rt )
						error(loc) << "comparison of different types" << endp;

					if ( lt->val() )
						code.append( IN_TST_GRTR_VAL );
					else
						code.append( IN_TST_GRTR_TREE );

					return pd->uniqueTypeBool;
				}
				case OP_LessEql: {
					UniqueType *lt = left->evaluate( pd, code );
					UniqueType *rt = right->evaluate( pd, code );

					if ( lt != rt )
						error(loc) << "comparison of different types" << endp;

					if ( lt->val() )
						code.append( IN_TST_LESS_EQL_VAL );
					else
						code.append( IN_TST_LESS_EQL_TREE );

					return pd->uniqueTypeBool;
				}
				case OP_GrtrEql: {
					UniqueType *lt = left->evaluate( pd, code );
					UniqueType *rt = right->evaluate( pd, code );

					if ( lt != rt )
						error(loc) << "comparison of different types" << endp;

					if ( lt->val() )
						code.append( IN_TST_GRTR_EQL_VAL );
					else
						code.append( IN_TST_GRTR_EQL_TREE );

					return pd->uniqueTypeBool;
				}
				case OP_LogicalAnd: {
					/* Evaluate the left and duplicate it. */
					UniqueType *lut = left->evaluate( pd, code );
					if ( !lut->val() )
						code.append( IN_TST_NZ_TREE );
					code.append( IN_DUP_VAL );

					/* Jump over the right if false, leaving the original left
					 * result on the top of the stack. We don't know the
					 * distance yet so record the position of the jump. */
					long jump = code.length();
					code.append( IN_JMP_FALSE_VAL );
					code.appendHalf( 0 );

					/* Evauluate the right, add the test. Store it separately. */
					UniqueType *rut = right->evaluate( pd, code );
					if ( !rut->val() )
						code.append( IN_TST_NZ_TREE );

					code.append( IN_TST_LOGICAL_AND );

					/* Set the distance of the jump. */
					long distance = code.length() - jump - 3;
					code.setHalf( jump+1, distance );

					return pd->uniqueTypeInt;
				}
				case OP_LogicalOr: {
					/* Evaluate the left and duplicate it. */
					UniqueType *lut = left->evaluate( pd, code );
					if ( !lut->val() )
						code.append( IN_TST_NZ_TREE );
					code.append( IN_DUP_VAL );

					/* Jump over the right if true, leaving the original left
					 * result on the top of the stack. We don't know the
					 * distance yet so record the position of the jump. */
					long jump = code.length();
					code.append( IN_JMP_TRUE_VAL );
					code.appendHalf( 0 );

					/* Evauluate the right, add the test. */
					UniqueType *rut = right->evaluate( pd, code );
					if ( !rut->val() )
						code.append( IN_TST_NZ_TREE );

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
					UniqueType *ut = right->evaluate( pd, code );
					if ( ut->val() )
						code.append( IN_NOT_VAL );
					else
						code.append( IN_NOT_TREE );
					return pd->uniqueTypeBool;
				}
				case '$': {
					UniqueType *ut = right->evaluate( pd, code );

					if ( ut->typeId == TYPE_INT || ut->typeId == TYPE_BOOL )
						code.append( IN_INT_TO_STR );

					code.append( IN_TREE_TO_STR_TRIM );
					return pd->uniqueTypeStr;
					
				}
				case 'S': {
					UniqueType *ut = right->evaluate( pd, code );

					if ( ut->typeId == TYPE_INT || ut->typeId == TYPE_BOOL )
						code.append( IN_INT_TO_STR );

					code.append( IN_TREE_TO_STR_TRIM_A );
					return pd->uniqueTypeStr;
				}
				case '%': {
					UniqueType *ut = right->evaluate( pd, code );
					if ( ut->typeId == TYPE_INT || ut->typeId == TYPE_BOOL )
						code.append( IN_INT_TO_STR );
					else 
						code.append( IN_TREE_TO_STR );
					return pd->uniqueTypeStr;
				}
				case '^': {
					UniqueType *rt = right->evaluate( pd, code );
					code.append( IN_TREE_TRIM );
					return rt;
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

void LangVarRef::assignValue( Compiler *pd, CodeVect &code, 
		UniqueType *exprUT ) const
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

	/* Writing guarantees the field is dirty. tree is dirty. */
	lookup.objField->dirtyTree = true;

	/* Check the types of the assignment and possibly cast. */
	UniqueType *objUT = lookup.objField->typeRef->uniqueType;
	assert( lookup.uniqueType == lookup.objField->typeRef->uniqueType );
	if ( !castAssignment( pd, code, objUT, lookup.iterSearchUT, exprUT ) )
		error(loc) << "type mismatch in assignment" << endp;
	
	/* Decide if we need to revert the assignment. */
	bool revert = lookup.lastPtrInQual >= 0 || !isLocalRef();

	/* Load the object and generate the field setting code. */
	loadObj( pd, code, lookup.lastPtrInQual, true );

	if ( lookup.uniqueType->typeId == TYPE_ITER )
		setFieldIter( pd, code, lookup.inObject, lookup.objField, lookup.uniqueType, exprUT, false );
	else
		setField( pd, code, lookup.inObject, lookup.objField, exprUT, revert );
}

UniqueType *LangTerm::evaluateMakeToken( Compiler *pd, CodeVect &code ) const
{
//	if ( pd->compileContext != Compiler::CompileTranslation )
//		error(loc) << "make_token can be used only in a translation block" << endp;

	/* Match the number of arguments. */
	int numArgs = args != 0 ? args->length() : 0;
	if ( numArgs < 2 )
		error(loc) << "need at least two arguments" << endp;

	for ( CallArgVect::Iter pe = *args; pe.lte(); pe++ ) {
		/* Evaluate. */
		UniqueType *exprUT = (*pe)->expr->evaluate( pd, code );

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

UniqueType *LangTerm::evaluateMakeTree( Compiler *pd, CodeVect &code ) const
{
//	if ( pd->compileContext != Compiler::CompileTranslation )
//		error(loc) << "make_tree can be used only in a translation block" << endp;

	/* Match the number of arguments. */
	int numArgs = args != 0 ? args->length() : 0;
	if ( numArgs < 1 )
		error(loc) << "need at least one argument" << endp;

	for ( CallArgVect::Iter pe = *args; pe.lte(); pe++ ) {
		/* Evaluate. */
		UniqueType *exprUT = (*pe)->expr->evaluate( pd, code );

		if ( pe.pos() == 0 && exprUT != pd->uniqueTypeInt )
			error(loc) << "first arg, nonterm id, must be an int" << endp;
	}

	/* The token is now created, send it. */
	code.append( IN_MAKE_TREE );
	code.append( args->length() );

	return pd->uniqueTypeAny;
}

void LangStmt::compileForIterBody( Compiler *pd, 
		CodeVect &code, UniqueType *iterUT ) const
{
	/* Remember the top of the loop. */
	long top = code.length();

	/* Advance */
	code.append( objField->iterImpl->inAdvance );
	code.appendHalf( objField->offset );

	/* Test: jump past the while block if false. Note that we don't have the
	 * distance yet. */
	long jumpFalse = code.length();
	code.append( IN_JMP_FALSE_VAL );
	code.appendHalf( 0 );

	/*
	 * Set up the loop cleanup code. 
	 */

	/* Add the cleanup for the current loop. */
	int lcLen = pd->unwindCode.length();
	pd->unwindCode.insertHalf( 0, objField->offset );
	pd->unwindCode.insert( 0, objField->iterImpl->inUnwind );

	/* Compile the contents. */
	for ( StmtList::Iter stmt = *stmtList; stmt.lte(); stmt++ )
		stmt->compile( pd, code );

	pd->unwindCode.remove( 0, pd->unwindCode.length() - lcLen );

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
	code.append( objField->iterImpl->inDestroy );
	code.appendHalf( objField->offset );

	/* Clean up any prepush args. */
}

void LangStmt::compileForIter( Compiler *pd, CodeVect &code ) const
{
	/* The type we are searching for. */
	UniqueType *searchUT = typeRef->uniqueType;

	/* Lookup the iterator call. Make sure it is an iterator. */
	VarRefLookup lookup = iterCall->langTerm->varRef->lookupIterCall( pd );
	if ( lookup.objMethod->iterDef == 0 ) {
		error(loc) << "attempt to iterate using something "
				"that is not an iterator" << endp;
	}

	/* Prepare the contiguous call args space. */
	Function *func = lookup.objMethod->func;
	int asLoc;
	if ( func != 0 ) {
		code.append( IN_PREP_ARGS );
		asLoc = code.length();
		code.appendHalf( 0 );
	}

	/* 
	 * Create the iterator from the local var.
	 */

	UniqueType *iterUT = objField->typeRef->uniqueType;
	IterImpl *iterImpl = 0;

	switch ( iterUT->iterDef->type ) {
		case IterDef::Tree:
			iterImpl = iterCall->langTerm->varRef->chooseTriterCall( pd,
					searchUT, iterCall->langTerm->args );
			break;
		case IterDef::Child:
			iterImpl = new IterImpl( IterImpl::Child );
			break;
		case IterDef::RevChild:
			iterImpl = new IterImpl( IterImpl::RevChild );
			break;
		case IterDef::Repeat:
			iterImpl = new IterImpl( IterImpl::Repeat );
			break;
		case IterDef::RevRepeat:
			iterImpl = new IterImpl( IterImpl::RevRepeat );
			break;
		case IterDef::User:
			iterImpl = new IterImpl( IterImpl::User, iterUT->iterDef->func );
			break;
		case IterDef::ListEl:
			iterImpl = new IterImpl( IterImpl::ListEl );
			break;
		case IterDef::RevListVal:
			iterImpl = new IterImpl( IterImpl::RevListVal );
			break;
		case IterDef::MapEl:
			iterImpl = new IterImpl( IterImpl::MapEl );
			break;
	}

	objField->iterImpl = iterImpl;

	/* Evaluate and push the arguments. */
	ObjectField **paramRefs = iterCall->langTerm->varRef->evaluateArgs(
			pd, code, lookup, iterCall->langTerm->args );

	if ( pd->revertOn )
		code.append( iterImpl->inCreateWV );
	else
		code.append( iterImpl->inCreateWC );

	code.appendHalf( objField->offset );

	/* Arg size (or func id for user iters). */
	if ( lookup.objMethod->func != 0 )
		code.appendHalf( lookup.objMethod->func->funcId );
	else
		code.appendHalf( iterCall->langTerm->varRef->argSize );

	/* Search type. */
	if ( iterImpl->useSearchUT )
		code.appendHalf( searchUT->langEl->id );

	if ( iterImpl->useGenericId ) {
		CodeVect unused; 
		UniqueType *ut = 
				iterCall->langTerm->args->data[0]->expr->evaluate( pd, unused );

		code.appendHalf( ut->generic->id );
	}

	compileForIterBody( pd, code, iterUT );

	iterCall->langTerm->varRef->popRefQuals( pd, code, lookup,
			iterCall->langTerm->args, false );

	iterCall->langTerm->varRef->resetActiveRefs( pd, lookup, paramRefs );
	delete[] paramRefs;

	if ( func != 0 ) {
		code.append( IN_CLEAR_ARGS );
		code.appendHalf( func->paramListSize );
		code.setHalf( asLoc, func->paramListSize );
	}
}

void LangStmt::compileWhile( Compiler *pd, CodeVect &code ) const
{
	/* Generate code for the while test. Remember the top. */
	long top = code.length();
	UniqueType *eut = expr->evaluate( pd, code );

	/* Jump past the while block if false. Note that we don't have the
	 * distance yet. */
	long jumpFalse = code.length();
	half_t jinstr = eut->tree() ? IN_JMP_FALSE_TREE : IN_JMP_FALSE_VAL;
	code.append( jinstr );
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

void LangStmt::compile( Compiler *pd, CodeVect &code ) const
{
	CodeVect block;

	StringMapEl *mapEl = 0;
	if ( pd->literalStrings.insert( "unwind code\n", &mapEl ) )
		mapEl->value = pd->literalStrings.length()-1;

	block.append( IN_LOAD_STR );
	block.appendWord( mapEl->value );

	block.append( IN_POP_TREE );

	pd->unwindCode.insert( 0, block );

	switch ( type ) {
		case PrintType: 
		case PrintXMLACType:
		case PrintXMLType:
		case PrintStreamType: {
			UniqueType **types = new UniqueType*[exprPtrVect->length()];
			
			/* Push the args backwards. */
			for ( CallArgVect::Iter pex = exprPtrVect->first(); pex.lte(); pex++ ) {
				types[pex.pos()] = (*pex)->expr->evaluate( pd, code );
				if ( types[pex.pos()]->typeId == TYPE_INT || 
					types[pex.pos()]->typeId == TYPE_BOOL )
				{
					code.append( IN_INT_TO_STR );
				}
				else if ( isTree( types[pex.pos()] ) && !isStr( types[pex.pos()] ) ) {
					code.append( IN_TREE_TRIM );
				}
			}

			/* Run the printing forwards. */
			if ( type == PrintType ) {
				code.append( IN_PRINT );
				code.append( exprPtrVect->length() );
			}
			else if ( type == PrintXMLACType ) {
				code.append( IN_PRINT_XML_AC );
				code.append( exprPtrVect->length() );
			}
			else if ( type == PrintXMLType ) {
				code.append( IN_PRINT_XML );
				code.append( exprPtrVect->length() );
			}
			else if ( type == PrintStreamType ) {
				/* Minus one because the first arg is the stream. */
				code.append( IN_PRINT_STREAM );
				code.append( exprPtrVect->length() - 1 );
			}

			delete[] types;

			break;
		}
		case PrintAccum: {
			code.append( IN_LOAD_GLOBAL_R );
			code.append(     IN_GET_CONST );
			code.appendHalf( IN_CONST_STDOUT );
			consItemList->evaluateSendStream( pd, code );
			code.append( IN_POP_TREE );
			break;
		}
		case ExprType: {
			/* Evaluate the exrepssion, then pop it immediately. */
			UniqueType *exprUt = expr->evaluate( pd, code );
			if ( exprUt->tree() )
				code.append( IN_POP_TREE );
			else
				code.append( IN_POP_VAL );

			// pd->unwindCode.remove( 0, 1 );
			break;
		}
		case IfType: {
			long jumpFalse = 0, jumpPastElse = 0, distance = 0;

			/* Evaluate the test. */
			UniqueType *eut = expr->evaluate( pd, code );

			/* Jump past the if block if false. We don't know the distance
			 * yet so store the location of the jump. */
			jumpFalse = code.length();
			half_t jinstr = eut->tree() ? IN_JMP_FALSE_TREE : IN_JMP_FALSE_VAL;

			code.append( jinstr );
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
				elsePart->compile( pd, code );

				/* Set the distance for jump over the else part. */
				distance = code.length() - jumpPastElse - 3;
				code.setHalf( jumpPastElse+1, distance );
			}

			break;
		}
		case ElseType: {
			/* Compile the else branch. */
			for ( StmtList::Iter stmt = *stmtList; stmt.lte(); stmt++ )
				stmt->compile( pd, code );
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

			if ( pd->curFunction == 0 ) {
				/* In the main function */
				pd->mainReturnUT = exprUT;
			}
			else {
				UniqueType *resUT = pd->curFunction->typeRef->uniqueType;
				if ( resUT != pd->uniqueTypeVoid && 
						!castAssignment( pd, code, resUT, 0, exprUT ) )
					error(loc) << "return value wrong type" << endp;
			}

			code.append( IN_SAVE_RET );

			/* The loop cleanup code. */
			if ( pd->unwindCode.length() > 0 )
				code.append( pd->unwindCode );

			/* Jump to the return label. The distance will be filled in
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
			varRef->preEvaluateRef( pd, code );
			ObjectField *objField = varRef->evaluateRef( pd, code, 0 );
			code.append( IN_YIELD );

			if ( varRef->qual->length() > 0 ) {
				code.append( IN_POP_N_WORDS );
				code.appendHalf( (short)(varRef->qual->length()*2) );
			}

			objField->refActive = false;
			break;
		}
	}

	pd->unwindCode.remove( 0, block.length() );
}

void CodeBlock::compile( Compiler *pd, CodeVect &code ) const
{
	for ( StmtList::Iter stmt = *stmtList; stmt.lte(); stmt++ )
		stmt->compile( pd, code );
}

void Compiler::findLocals( ObjectDef *localFrame, CodeBlock *block )
{
	Locals &locals = block->locals;

	for ( FieldList::Iter ol = localFrame->fieldList; ol.lte(); ol++ ) {
		ObjectField *el = ol->value;

		/* FIXME: This test needs to be improved. Match_text was getting
		 * through before useOffset was tested. What will? */
		if ( el->useOffset() && !el->isLhsEl() &&
				( el->beenReferenced || el->isParam() ) )
		{
			UniqueType *ut = el->typeRef->uniqueType;
			if ( ut->tree() ) {
				int depth = el->scope->depth();
				locals.append( LocalLoc( LT_Tree, depth, el->offset ) );
			}
		}

		if ( el->useOffset() ) {
			UniqueType *ut = el->typeRef->uniqueType;
			if ( ut->typeId == TYPE_ITER ) {
				int depth = el->scope->depth();
				LocalType type = LT_Tree;
				switch ( ut->iterDef->type ) {
					case IterDef::Tree:
					case IterDef::Child:
					case IterDef::Repeat:
					case IterDef::RevRepeat:
						type = LT_Iter;
						break;

					case IterDef::MapEl:
					case IterDef::ListEl:
					case IterDef::RevListVal:
						/* ? */
						type = LT_Iter;
						break;

					case IterDef::RevChild:
						type = LT_RevIter;
						break;
					case IterDef::User:
						type = LT_UserIter;
						break;
				}

				locals.append( LocalLoc( type, depth, (int)el->offset ) );
			}
		}
	}
}

void Compiler::addProdLHSLoad( Production *prod, CodeVect &code, long &insertPos )
{
	NameScope *scope = prod->redBlock->localFrame->rootScope;
	ObjectField *lhsField = scope->findField("lhs");
	assert( lhsField != 0 );

	CodeVect loads;
	if ( lhsField->beenReferenced ) {
		loads.append( IN_INIT_LHS_EL );
		loads.appendHalf( lhsField->offset );
	}

	code.insert( insertPos, loads );
	insertPos += loads.length();
}

void Compiler::addPushBackLHS( Production *prod, CodeVect &code, long &insertPos )
{
	CodeBlock *block = prod->redBlock;

	/* If the lhs tree is dirty then we will need to save off the old lhs
	 * before it gets modified. We want to avoid this for attribute
	 * modifications. The computation of dirtyTree should deal with this for
	 * us. */
	NameScope *scope = block->localFrame->rootScope;
	ObjectField *lhsField = scope->findField("lhs");
	assert( lhsField != 0 );

	if ( lhsField->beenReferenced ) {
		code.append( IN_STORE_LHS_EL );
		code.appendHalf( lhsField->offset );
	}
}

void Compiler::addProdRHSLoads( Production *prod, CodeVect &code, long &insertPos )
{
	CodeVect loads;
	long elPos = 0;
	for ( ProdElList::Iter rhsEl = *prod->prodElList; rhsEl.lte(); rhsEl++, elPos++ ) {
		if ( rhsEl->type == ProdEl::ReferenceType ) {
			if ( rhsEl->rhsElField->beenReferenced ) {
				loads.append ( IN_INIT_RHS_EL );
				loads.appendHalf( elPos );
				loads.appendHalf( rhsEl->rhsElField->offset );
			}
		}
	}

	/* Insert and update the insert position. */
	code.insert( insertPos, loads );
	insertPos += loads.length();
}



void Compiler::makeProdCopies( Production *prod )
{
	int pos = 0;
	for ( ProdElList::Iter pel = *prod->prodElList; pel.lte(); pel++, pos++) {
		if ( pel->captureField != 0 ) {
			prod->copy.append( pel->captureField->offset );
			prod->copy.append( pos );
		}
	}
}

void Compiler::compileReductionCode( Production *prod )
{
	CodeBlock *block = prod->redBlock;

	/* Init the compilation context. */
	compileContext = CompileReduction;
	revertOn = true;
	block->frameId = nextFrameId++;

	CodeVect &code = block->codeWV;

	long afterInit = code.length();

	/* Compile the reduce block. */
	block->compile( this, code );

	/* Might need to load right hand side values. */
	addProdRHSLoads( prod, code, afterInit );

	addProdLHSLoad( prod, code, afterInit );
	addPushBackLHS( prod, code, afterInit );

	code.append( IN_PCR_RET );

	/* Now that compilation is done variables are referenced. Make the local
	 * trees descriptor. */
	findLocals( block->localFrame, block );
}

void Compiler::compileTranslateBlock( LangEl *langEl )
{
	CodeBlock *block = langEl->transBlock;

	/* Set up compilation context. */
	compileContext = CompileTranslation;
	revertOn = true;
	block->frameId = nextFrameId++;

	CodeVect &code = block->codeWV;

	if ( langEl->tokenDef->reCaptureVect.length() > 0 ) {
		code.append( IN_INIT_CAPTURES );
		code.append( langEl->tokenDef->reCaptureVect.length() );
	}

	/* Set the local frame and compile the reduce block. */
	block->compile( this, code );

	code.append( IN_PCR_RET );

	/* Now that compilation is done variables are referenced. Make the local
	 * trees descriptor. */
	findLocals( block->localFrame, block );
}

void Compiler::compilePreEof( TokenRegion *region )
{
	CodeBlock *block = region->preEofBlock;

	/* Set up compilation context. */
	compileContext = CompileTranslation;
	revertOn = true;
	block->frameId = nextFrameId++;

	addInput( block->localFrame );
	addThis( block->localFrame );

	CodeVect &code = block->codeWV;

	/* Set the local frame and compile the reduce block. */
	block->compile( this, code );

	code.append( IN_PCR_RET );

	/* Now that compilation is done variables are referenced. Make the local
	 * trees descriptor. */
	findLocals( block->localFrame, block );
}

int Compiler::arg0Offset()
{
	globalObjectDef->referenceField( this, arg0 );
	return arg0->offset;
}

int Compiler::argvOffset()
{
	globalObjectDef->referenceField( this, argv );
	return argv->offset;
}

void Compiler::compileRootBlock( )
{
	CodeBlock *block = rootCodeBlock;

	/* The root block never needs to be reverted. */

	/* Set up the compile context. No locals are needed for the root code
	 * block, but we need an empty local frame for the compile. */
	compileContext = CompileRoot;
	revertOn = false;

	/* The block needs a frame id. */
	block->frameId = nextFrameId++;

	/* The root block is not reverted. */
	CodeVect &code = block->codeWC;

	code.append( IN_FN );
	code.append( IN_LOAD_ARG0 );
	code.appendHalf( arg0Offset() );

	code.append( IN_FN );
	code.append( IN_LOAD_ARGV );
	code.appendHalf( argvOffset() );

	block->compile( this, code );

	code.append( IN_FN );
	code.append( IN_STOP );

	/* Make the local trees descriptor. */
	findLocals( rootLocalFrame, block );
}

void ObjectField::initField()
{
	switch ( type ) {
		case UserLocalType:
		case LhsElType:
		case ParamValType:
		case RedRhsType:
			inGetR      =  IN_GET_LOCAL_R;
			inGetWC     =  IN_GET_LOCAL_WC;
			inSetWC     =  IN_SET_LOCAL_WC;
			inGetValR   =  IN_GET_LOCAL_VAL_R;
			inGetValWC  =  IN_GET_LOCAL_VAL_R;
			inGetValWV  =  IN_GET_LOCAL_VAL_R;
			inSetValWC  =  IN_SET_LOCAL_VAL_WC;
			break;

		case ParamRefType:
			inGetR  =  IN_GET_LOCAL_REF_R;
			inGetWC =  IN_GET_LOCAL_REF_WC;
			inSetWC =  IN_SET_LOCAL_REF_WC;
			break;

		case UserFieldType:
			inGetR  =  IN_GET_FIELD_TREE_R;
			inGetWC =  IN_GET_FIELD_TREE_WC;
			inGetWV =  IN_GET_FIELD_TREE_WV;
			inSetWC =  IN_SET_FIELD_TREE_WC;
			inSetWV =  IN_SET_FIELD_TREE_WV;

			//inGetValR;
			inGetValR  = IN_GET_FIELD_VAL_R;
			//inGetValWC = IN_GET_FIELD_VAL_WC;
			//inGetValWV;
			inSetValWC = IN_SET_FIELD_VAL_WC;
			//inSetValWV;
			break;

		case GenericElementType:
		case GenericDependentType:
		case StructFieldType:
			inGetR  =  IN_GET_STRUCT_R;
			inGetWC =  IN_GET_STRUCT_WC;
			inGetWV =  IN_GET_STRUCT_WV;
			inSetWC =  IN_SET_STRUCT_WC;
			inSetWV =  IN_SET_STRUCT_WV;
			inGetValR  = IN_GET_STRUCT_VAL_R;
			inGetValWC = IN_GET_STRUCT_VAL_R;
			inGetValWV = IN_GET_STRUCT_VAL_R;
			inSetValWC = IN_SET_STRUCT_VAL_WC;
			inSetValWV = IN_SET_STRUCT_VAL_WV;
			break;

		case RhsNameType:
			inGetR  =  IN_GET_RHS_VAL_R;
			inGetWC =  IN_GET_RHS_VAL_WC;
			inGetWV =  IN_GET_RHS_VAL_WV;
			inSetWC =  IN_SET_RHS_VAL_WC;
			inSetWV =  IN_SET_RHS_VAL_WC;
			break;

		/* Inbuilts have instructions intialized outside the cons, at place of
		 * call. */
		case InbuiltFieldType:
		case InbuiltObjectType:
		case InbuiltOffType:
			break;

		/* Out of date impl. */
		case LexSubstrType:
			break;
	}
}

void ObjectDef::placeField( Compiler *pd, ObjectField *field )
{
	UniqueType *fieldUT = field->typeRef->uniqueType;

	switch ( field->type ) {
		case ObjectField::LhsElType:
		case ObjectField::UserLocalType:
		case ObjectField::RedRhsType:

			/* Local frame fields. Move the running offset first since this is
			 * a negative off from the end. */
			nextOffset += sizeOfField( fieldUT );
			field->offset = -nextOffset;
			break;


		case ObjectField::GenericElementType: {

			/* Tree object frame fields. Record the position, then move the
			 * running offset. */
			field->offset = nextOffset;
			nextOffset += sizeOfField( fieldUT );

			if ( fieldUT->typeId == TYPE_MAP_PTRS ) {
				if ( field->mapKeyField != 0 )
					field->mapKeyField->offset = field->offset;
			}

			break;
		}

		case ObjectField::UserFieldType:

			/* Tree object frame fields. Record the position, then move the
			 * running offset. */
			field->offset = nextOffset;
			nextOffset += sizeOfField( fieldUT );
			break;

		case ObjectField::StructFieldType:
			field->offset = nextOffset;
			nextOffset += sizeOfField( fieldUT );
			break;

		case ObjectField::GenericDependentType:
			/* There is an object field that this type depends on. When it is
			 * placed, this one will be placed as well. Nothing to do now. */

		case ObjectField::InbuiltFieldType:
		case ObjectField::InbuiltOffType:
		case ObjectField::InbuiltObjectType:
		case ObjectField::RhsNameType:
		case ObjectField::LexSubstrType:

		case ObjectField::ParamValType:
		case ObjectField::ParamRefType:
			break;
	}
}

void Compiler::placeAllLanguageObjects()
{
	/* Init all user object fields (need consistent size). */
	for ( LelList::Iter lel = langEls; lel.lte(); lel++ ) {
		ObjectDef *objDef = lel->objectDef;
		if ( objDef != 0 ) {
			/* Init all fields of the object. */
			for ( FieldList::Iter f = objDef->fieldList; f.lte(); f++ )
				objDef->placeField( this, f->value );
		}
	}
}

void Compiler::placeAllStructObjects()
{
	for ( StructElList::Iter s = structEls; s.lte(); s++ ) {
		ObjectDef *objectDef = s->structDef->objectDef;
		for ( FieldList::Iter f = objectDef->fieldList; f.lte(); f++ )
			objectDef->placeField( this, f->value );
	}
}

void Compiler::placeFrameFields( ObjectDef *localFrame )
{
	for ( FieldList::Iter f = localFrame->fieldList; f.lte(); f++ )
		localFrame->placeField( this, f->value );
}

void Compiler::placeAllFrameObjects()
{
	/* Functions. */
	for ( FunctionList::Iter f = functionList; f.lte(); f++ )
		placeFrameFields( f->localFrame );

	for ( FunctionList::Iter f = inHostList; f.lte(); f++ )
		placeFrameFields( f->localFrame );

	/* Reduction code. */
	for ( DefList::Iter prod = prodList; prod.lte(); prod++ ) {
		if ( prod->redBlock != 0 )
			placeFrameFields( prod->redBlock->localFrame );
	}

	/* Token translation code. */
	for ( LelList::Iter lel = langEls; lel.lte(); lel++ ) {
		if ( lel->transBlock != 0 ) {
			ObjectDef *localFrame = lel->transBlock->localFrame;
			if ( lel->tokenDef->reCaptureVect.length() > 0 ) {
				FieldList::Iter f = localFrame->fieldList;
				for ( int i = 0; i < lel->tokenDef->reCaptureVect.length(); i++, f++ )
					localFrame->placeField( this, f->value );
			}

			placeFrameFields( localFrame );
		}
	}

	/* Preeof blocks. */
	for ( RegionList::Iter r = regionList; r.lte(); r++ ) {
		if ( r->preEofBlock != 0 )
			placeFrameFields( r->preEofBlock->localFrame );
	}

	/* Root code. */
	placeFrameFields( rootLocalFrame );
}

void Compiler::placeUserFunction( Function *func, bool isUserIter )
{
	/* Set up the parameters. */
	long paramPos = 0, paramListSize = 0, paramOffset = 0;
	UniqueType **paramUTs = new UniqueType*[func->paramList->length()];
	for ( ParameterList::Iter param = *func->paramList; param.lte(); param++, paramPos++ ) {
		paramUTs[paramPos] = param->typeRef->uniqueType;
		paramListSize += sizeOfField( paramUTs[paramPos] );
	}

	/* Param offset is relative to one past the last item in the array of
	 * words containing the args. */
	paramOffset = 0;
	paramPos = 0;
	for ( ParameterList::Iter param = *func->paramList; param.lte(); param++, paramPos++ ) {
		/* How much space do we need to make for call overhead. */
		long frameAfterArgs = isUserIter ? IFR_AA : FR_AA;

		param->offset = frameAfterArgs + paramOffset;

		paramOffset += sizeOfField( paramUTs[paramPos] );
	}

	func->paramListSize = paramListSize;
	func->paramUTs = paramUTs;

	func->objMethod->paramUTs = paramUTs;

	/* Insert the function into the global function map. */
	UniqueType *returnUT = func->typeRef != 0 ? 
			func->typeRef->uniqueType : uniqueTypeInt;
	func->objMethod->returnUT = returnUT;

	func->objMethod->paramUTs = new UniqueType*[func->paramList->length()];
	memcpy( func->objMethod->paramUTs, paramUTs,
			sizeof(UniqueType*) * func->paramList->length() );
}

void Compiler::placeAllFunctions()
{
	for ( FunctionList::Iter f = functionList; f.lte(); f++ )
		placeUserFunction( f, f->isUserIter );

	for ( FunctionList::Iter f = inHostList; f.lte(); f++ )
		placeUserFunction( f, false );
}


void Compiler::compileUserIter( Function *func, CodeVect &code )
{
	CodeBlock *block = func->codeBlock;

	/* Compile the block. */
	block->compile( this, code );

	/* Always yeild a nil at the end. This causes iteration to stop. */
	code.append( IN_LOAD_NIL );
	code.append( IN_YIELD );
}

void Compiler::compileUserIter( Function *func )
{
	CodeBlock *block = func->codeBlock;

	/* Set up the context. */
	compileContext = CompileFunction;
	curFunction = func;
	block->frameId = nextFrameId++;

	/* Compile for revert and commit. */
	revertOn = true;
	compileUserIter( func, block->codeWV );

	revertOn = false;
	compileUserIter( func, block->codeWC );

	/* Now that compilation is done variables are referenced. Make the local
	 * trees descriptor. */
	findLocals( block->localFrame, block );

	/* FIXME: Need to deal with the freeing of local trees. */
}

/* Called for each type of function compile: revert and commit. */
void Compiler::compileFunction( Function *func, CodeVect &code )
{
	CodeBlock *block = func->codeBlock;

	/* Compile the block. */
	block->compile( this, code );

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
}

void Compiler::compileFunction( Function *func )
{
	CodeBlock *block = func->codeBlock;

	/* Set up the compilation context. */
	compileContext = CompileFunction;
	curFunction = func;

	/* Assign a frame Id. */
	block->frameId = nextFrameId++;

	/* Compile once for revert. */
	revertOn = true;
	compileFunction( func, block->codeWV );

	/* Compile once for commit. */
	revertOn = false;
	compileFunction( func, block->codeWC );

	/* Now that compilation is done variables are referenced. Make the local
	 * trees descriptor. */
	findLocals( block->localFrame, block );
}

void Compiler::removeNonUnparsableRepls()
{
	for ( ConsList::Iter repl = replList; repl.lte(); ) {
		Constructor *maybeDel = repl++;
		if ( !maybeDel->parse )
			replList.detach( maybeDel );
	}
}

void Compiler::compileByteCode()
{
	/* Compile functions. */
	for ( FunctionList::Iter f = functionList; f.lte(); f++ ) {
		if ( f->isUserIter )
			compileUserIter( f );
		else
			compileFunction( f );
	}

	/* Compile the reduction code. */
	for ( DefList::Iter prod = prodList; prod.lte(); prod++ ) {
		makeProdCopies( prod );
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
	removeNonUnparsableRepls();
}
