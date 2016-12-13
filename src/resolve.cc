/*
 * Copyright 2009-2012 Adrian Thurston <thurston@colm.net>
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

#include <stdbool.h>

#include <iostream>

#include "compiler.h"
/*
 * Type Resolution.
 */

using std::cout;
using std::cerr;
using std::endl;

Namespace *TypeRef::resolveNspace( Compiler *pd )
{
	if ( parsedVarRef != 0 && !nspaceQual->thisOnly() ) {
		UniqueType *ut = parsedVarRef->lookup( pd );
		return ut->langEl->nspace;
	}
	else if ( parsedTypeRef != 0 && !nspaceQual->thisOnly() ) {
		UniqueType *ut = parsedTypeRef->resolveType( pd );
		return ut->langEl->nspace;
	}
	else {
		/* Lookup up the qualifiction and then the name. */
		return nspaceQual->getQual( pd );
	}
}

UniqueType *TypeRef::resolveTypeName( Compiler *pd )
{
	nspace = resolveNspace( pd );

	if ( nspace == 0 )
		error(loc) << "do not have region for resolving reference" << endp;

	while ( nspace != 0 ) {
		/* Search for the token in the region by typeName. */
		TypeMapEl *inDict = nspace->typeMap.find( typeName );

		if ( inDict != 0 ) {
			switch ( inDict->type ) {
				/* Defer to the typeRef we are an alias of. We need to guard
				 * against loops here. */
				case TypeMapEl::AliasType: {
					return inDict->typeRef->resolveType( pd );
				}

				case TypeMapEl::LangElType: {
					UniqueType *ut = pd->findUniqueType( TYPE_TREE, inDict->value );
					return ut;
				}
				case TypeMapEl::StructType: {
					UniqueType *ut = pd->findUniqueType( TYPE_STRUCT, inDict->structEl );
					return ut;
				}
			}
		}

		if ( nspaceQual->thisOnly() )
			break;

		nspace = nspace->parentNamespace;
	}

	error(loc) << "unknown type " << typeName << " in typeof expression" << endp;
	return 0;
}

UniqueType *TypeRef::resolveTypeLiteral( Compiler *pd )
{
	/* Lookup up the qualifiction and then the name. */
	nspace = resolveNspace( pd );

	if ( nspace == 0 )
		error(loc) << "do not have region for resolving reference" << endp;

	/* Interpret escape sequences and remove quotes. */
	bool unusedCI;
	String interp;
	prepareLitString( interp, unusedCI, pdaLiteral->data,
			pdaLiteral->loc );

	while ( nspace != 0 ) {
		LiteralDictEl *ldel = nspace->literalDict.find( interp );

		if ( ldel != 0 )
			return pd->findUniqueType( TYPE_TREE, ldel->value->tokenDef->tdLangEl );

		if ( nspaceQual->thisOnly() )
			break;

		nspace = nspace->parentNamespace;
	}

	error(loc) << "unknown type " << typeName << " in typeof expression" << endp;
	return 0;
}

bool TypeRef::uniqueGeneric( UniqueGeneric *&inMap, Compiler *pd,
		const UniqueGeneric &searchKey )
{
	bool inserted = false;
	inMap = pd->uniqueGenericMap.find( &searchKey );
	if ( inMap == 0 ) {
		inserted = true;
		inMap = new UniqueGeneric( searchKey );
		pd->uniqueGenericMap.insert( inMap );
	}
	return inserted;
}

StructEl *TypeRef::declareListEl( Compiler *pd, TypeRef *valType )
{
	static long vlistElId = 1;
	String name( 32, "list_el_%d", vlistElId++ );
	ObjectDef *objectDef = ObjectDef::cons( ObjectDef::StructType,
			name, pd->nextObjectId++ );

	StructDef *structDef = new StructDef( loc, name, objectDef );

	pd->rootNamespace->structDefList.append( structDef );

	/* Value Element. */
	String id = "value";
	ObjectField *elValObjField = ObjectField::cons( internal,
				ObjectField::StructFieldType, valType, id );

	objectDef->rootScope->insertField( elValObjField->name, elValObjField );

	/* Typeref for the struct. Used for pointers. */
	NamespaceQual *nspaceQual = NamespaceQual::cons( pd->rootNamespace );
	TypeRef *selfTypeRef = TypeRef::cons( InputLoc(), nspaceQual, name, RepeatNone );

	/* Type ref for the list pointers psuedo type. */
	TypeRef *elTr = TypeRef::cons( InputLoc(), TypeRef::ListPtrs, 0, selfTypeRef, 0 );

	ObjectField *of = ObjectField::cons( InputLoc(),
			ObjectField::GenericElementType, elTr, name );

	objectDef->rootScope->insertField( of->name, of );

	return declareStruct( pd, pd->rootNamespace, name, structDef );
}

void ConsItemList::resolve( Compiler *pd )
{
	/* Types in constructor. */
	for ( ConsItemList::Iter item = first(); item.lte(); item++ ) {
		switch ( item->type ) {
		case ConsItem::LiteralType:
			/* Use pdaFactor reference resolving. */
			pd->resolveProdEl( item->prodEl );
			break;
		case ConsItem::InputText:
			break;
		case ConsItem::ExprType:
			item->expr->resolve( pd );
			break;
		}
	}
}

UniqueType *TypeRef::resolveTypeListEl( Compiler *pd )
{
	TypeRef *valTr = typeRef1;
	UniqueType *utValue = valTr->resolveType( pd );	

	UniqueGeneric *inMap = 0, searchKey( UniqueGeneric::ListEl, utValue );
	if ( uniqueGeneric( inMap, pd, searchKey ) )
		inMap->structEl = declareListEl( pd, valTr );

	return pd->findUniqueType( TYPE_STRUCT, inMap->structEl );
}

UniqueType *TypeRef::resolveTypeList( Compiler *pd )
{
	nspace = pd->rootNamespace;

	UniqueType *utValue = typeRef1->resolveType( pd );	

	if ( utValue->typeId != TYPE_STRUCT )
		error( loc ) << "only structs can be list elements" << endp;

	/* Find the list element. */
	ObjectDef *elObjDef = utValue->structEl->structDef->objectDef;
	UniqueType *ptrsUt = pd->findUniqueType( TYPE_LIST_PTRS );
	ObjectField *listEl = elObjDef->findFieldType( pd, ptrsUt );

	if ( !listEl )
		error( loc ) << "could not find list element in type ref" << endp;

	UniqueGeneric *inMap = 0, searchKey( UniqueGeneric::List, utValue );
	if ( uniqueGeneric( inMap, pd, searchKey ) ) {

		GenericType *generic = new GenericType( GEN_LIST,
				pd->nextGenericId++, typeRef1, 0, typeRef2, listEl );

		nspace->genericList.append( generic );

		generic->declare( pd, nspace );

		inMap->generic = generic;
	}

	generic = inMap->generic;
	return pd->findUniqueType( TYPE_GENERIC, inMap->generic );
}

StructEl *TypeRef::declareMapElStruct( Compiler *pd, TypeRef *keyType, TypeRef *valType )
{
	static long vlistElId = 1;
	String name( 32, "map_el_%d", vlistElId++ );
	ObjectDef *objectDef = ObjectDef::cons( ObjectDef::StructType,
			name, pd->nextObjectId++ );

	StructDef *structDef = new StructDef( loc, name, objectDef );

	pd->rootNamespace->structDefList.append( structDef );

	/* Value Element. */
	String id = "value";
	ObjectField *elValObjField = ObjectField::cons( internal,
				ObjectField::StructFieldType, valType, id );

	objectDef->rootScope->insertField( elValObjField->name, elValObjField );

	/* Typeref for the pointers. */
	NamespaceQual *nspaceQual = NamespaceQual::cons( pd->rootNamespace );
	TypeRef *selfTypeRef = TypeRef::cons( InputLoc(), nspaceQual, name, RepeatNone );

	TypeRef *elTr = TypeRef::cons( InputLoc(), TypeRef::MapPtrs, 0, selfTypeRef, keyType );

	ObjectField *of = ObjectField::cons( InputLoc(),
			ObjectField::GenericElementType, elTr, name );

	objectDef->rootScope->insertField( of->name, of );

	StructEl *sel = declareStruct( pd, pd->rootNamespace, name, structDef );
	return sel;
}

UniqueType *TypeRef::resolveTypeMapEl( Compiler *pd )
{
	TypeRef *keyType = typeRef1;
	TypeRef *valType = typeRef2;

	UniqueType *utKey = keyType->resolveType( pd );	
	UniqueType *utValue = valType->resolveType( pd );	

	UniqueGeneric *inMap = 0, searchKey( UniqueGeneric::MapEl, utKey, utValue );
	if ( uniqueGeneric( inMap, pd, searchKey ) )
		inMap->structEl = declareMapElStruct( pd, keyType, valType );

	return pd->findUniqueType( TYPE_STRUCT, inMap->structEl );
}


UniqueType *TypeRef::resolveTypeMap( Compiler *pd )
{
	nspace = pd->rootNamespace;

	UniqueType *utKey = typeRef1->resolveType( pd );	
	UniqueType *utEl = typeRef2->resolveType( pd );	

	if ( utEl->typeId != TYPE_STRUCT )
		error( loc ) << "only structs can be map elements" << endp;

	/* Find the list element. */
	ObjectDef *elObjDef = utEl->structEl->structDef->objectDef;
	UniqueType *ptrsUt = pd->findUniqueType( TYPE_MAP_PTRS );
	ObjectField *mapEl = elObjDef->findFieldType( pd, ptrsUt );

	if ( !mapEl )
		error( loc ) << "could not find map element in type ref" << endp;

	UniqueGeneric *inMap = 0, searchKey( UniqueGeneric::Map, utKey, utEl );

	if ( uniqueGeneric( inMap, pd, searchKey ) ) {

		GenericType *generic = new GenericType( GEN_MAP,
				pd->nextGenericId++, typeRef2, typeRef1, typeRef3, mapEl );

		nspace->genericList.append( generic );

		generic->declare( pd, nspace );

		inMap->generic = generic;
	}

	generic = inMap->generic;
	return pd->findUniqueType( TYPE_GENERIC, inMap->generic );
}

UniqueType *TypeRef::resolveTypeParser( Compiler *pd )
{
	nspace = pd->rootNamespace;

	UniqueType *utParse = typeRef1->resolveType( pd );	

	UniqueGeneric *inMap = 0, searchKey( UniqueGeneric::Parser, utParse );
	if ( uniqueGeneric( inMap, pd, searchKey ) ) {
		GenericType *generic = new GenericType( GEN_PARSER,
				pd->nextGenericId++, typeRef1, 0, 0, 0 );

		nspace->genericList.append( generic );

		generic->declare( pd, nspace );

		inMap->generic = generic;
	}

	generic = inMap->generic;
	return pd->findUniqueType( TYPE_GENERIC, inMap->generic );
}


/*
 * End object based list/map
 */


UniqueType *TypeRef::resolveTypeRef( Compiler *pd )
{
	typeRef1->resolveType( pd );
	return pd->findUniqueType( TYPE_REF, typeRef1->uniqueType->langEl );
}

void TypeRef::resolveRepeat( Compiler *pd )
{
	if ( uniqueType->typeId != TYPE_TREE )
		error(loc) << "cannot repeat non-tree type" << endp;

	UniqueRepeat searchKey( repeatType, uniqueType->langEl );
	UniqueRepeat *uniqueRepeat = pd->uniqeRepeatMap.find( &searchKey );
	if ( uniqueRepeat == 0 ) {
		uniqueRepeat = new UniqueRepeat( repeatType, uniqueType->langEl );
		pd->uniqeRepeatMap.insert( uniqueRepeat );

		LangEl *declLangEl = 0;
	
		switch ( repeatType ) {
			case RepeatRepeat: {
				/* If the factor is a repeat, create the repeat element and link the
				 * factor to it. */
				String repeatName( 128, "_repeat_%s", typeName.data );
				declLangEl = pd->makeRepeatProd( loc, nspace, repeatName, uniqueType );
				break;
			}
			case RepeatList: {
				/* If the factor is a repeat, create the repeat element and link the
				 * factor to it. */
				String listName( 128, "_list_%s", typeName.data );
				declLangEl = pd->makeListProd( loc, nspace, listName, uniqueType );
				break;
			}
			case RepeatOpt: {
				/* If the factor is an opt, create the opt element and link the factor
				 * to it. */
				String optName( 128, "_opt_%s", typeName.data );
				declLangEl = pd->makeOptProd( loc, nspace, optName, uniqueType );
				break;
			}

			case RepeatNone:
				break;
		}

		uniqueRepeat->declLangEl = declLangEl;
		declLangEl->repeatOf = uniqueRepeat->langEl;
	}

	uniqueType = pd->findUniqueType( TYPE_TREE, uniqueRepeat->declLangEl );
}

UniqueType *TypeRef::resolveIterator( Compiler *pd )
{
	UniqueType *searchUT = searchTypeRef->resolveType( pd );

	/* Lookup the iterator call. Make sure it is an iterator. */
	VarRefLookup lookup = iterCall->langTerm->varRef->lookupIterCall( pd );
	if ( lookup.objMethod->iterDef == 0 ) {
		error(loc) << "attempt to iterate using something "
				"that is not an iterator" << endp;
	}

	/* Now that we have done the iterator call lookup we can make the type
	 * reference for the object field. */
	UniqueType *iterUniqueType = pd->findUniqueType( TYPE_ITER, lookup.objMethod->iterDef );

	iterDef = lookup.objMethod->iterDef;
	searchUniqueType = searchUT;

	return iterUniqueType;
}


UniqueType *TypeRef::resolveType( Compiler *pd )
{
	if ( uniqueType != 0 )
		return uniqueType;

	/* Not an iterator. May be a reference. */
	switch ( type ) {
		case Name:
			uniqueType = resolveTypeName( pd );
			break;
		case Literal:
			uniqueType = resolveTypeLiteral( pd );
			break;
		case Parser:
			uniqueType = resolveTypeParser( pd );
			break;
		case Ref:
			uniqueType = resolveTypeRef( pd );
			break;
		case Iterator:
			uniqueType = resolveIterator( pd );
			break;

		case List:
			uniqueType = resolveTypeList( pd );
			break;
		case ListPtrs:
			uniqueType = pd->findUniqueType( TYPE_LIST_PTRS );
			break;
		case ListEl:
			uniqueType = resolveTypeListEl( pd );
			break;

		case Map:
			uniqueType = resolveTypeMap( pd );
			break;
		case MapPtrs:
			uniqueType = pd->findUniqueType( TYPE_MAP_PTRS );
			break;
		case MapEl:
			uniqueType = resolveTypeMapEl( pd );
			break;
			
		case Unspecified:
			/* No lookup needed, unique type(s) set when constructed. */
			break;
	}

	if ( repeatType != RepeatNone )
		resolveRepeat( pd );

	return uniqueType;
}

void Compiler::resolveProdEl( ProdEl *prodEl )
{
	prodEl->typeRef->resolveType( this );
	prodEl->langEl = prodEl->typeRef->uniqueType->langEl;
}

void LangTerm::resolveFieldArgs( Compiler *pd )
{
	/* Initialization expressions. */
	if ( fieldInitArgs != 0 ) {
		for ( FieldInitVect::Iter pi = *fieldInitArgs; pi.lte(); pi++ )
			(*pi)->expr->resolve( pd );
	}
}

void LangTerm::resolve( Compiler *pd )
{
	switch ( type ) {
		case ConstructType:
			typeRef->resolveType( pd );

			resolveFieldArgs( pd );

			/* Types in constructor. */
			constructor->list->resolve( pd );
			break;

		case VarRefType:
			break;

		case MakeTreeType:
		case MakeTokenType:
		case MethodCallType:
			if ( args != 0 ) {
				for ( CallArgVect::Iter pe = *args; pe.lte(); pe++ )
					(*pe)->expr->resolve( pd );
			}
			break;

		case NumberType:
		case StringType:
			break;

		case MatchType:
			for ( PatternItemList::Iter item = *pattern->list; item.lte(); item++ ) {
				switch ( item->form ) {
				case PatternItem::TypeRefForm:
					/* Use pdaFactor reference resolving. */
					pd->resolveProdEl( item->prodEl );
					break;
				case PatternItem::InputTextForm:
					/* Nothing to do here. */
					break;
				}
			}

			break;
		case NewType:
			/* Init args, then the new type. */
			resolveFieldArgs( pd );
			typeRef->resolveType( pd );
			break;
		case TypeIdType:
			typeRef->resolveType( pd );
			break;
		case SearchType:
			typeRef->resolveType( pd );
			break;
		case NilType:
		case TrueType:
		case FalseType:
			break;

		case ParseType:
		case ParseTreeType:
		case ParseStopType:
			typeRef->resolveType( pd );

			resolveFieldArgs( pd );

			parserText->list->resolve( pd );
			break;

		case SendType:
		case SendTreeType:
		case EmbedStringType:
			break;

		case CastType:
			typeRef->resolveType( pd );
			expr->resolve( pd );
			break;
	}
}

void LangVarRef::resolve( Compiler *pd ) const
{
}

void LangExpr::resolve( Compiler *pd ) const
{
	switch ( type ) {
		case BinaryType: {
			left->resolve( pd );
			right->resolve( pd );
			break;
		}
		case UnaryType: {
			right->resolve( pd );
			break;
		}
		case TermType: {
			term->resolve( pd );
			break;
		}
	}
}

void IterCall::resolve( Compiler *pd ) const
{
	switch ( form ) {
		case Call:
			langTerm->resolve( pd );
			break;
		case Expr:
			langExpr->resolve( pd );
			break;
	}
}

void LangStmt::resolveForIter( Compiler *pd ) const
{
	iterCall->resolve( pd );

	/* Search type ref. */
	typeRef->resolveType( pd );

	/* Iterator type ref. */
	objField->typeRef->resolveType( pd );

	/* Resolve the statements. */
	for ( StmtList::Iter stmt = *stmtList; stmt.lte(); stmt++ )
		stmt->resolve( pd );
}

void LangStmt::resolve( Compiler *pd ) const
{
	switch ( type ) {
		case PrintType: 
		case PrintXMLACType:
		case PrintXMLType:
		case PrintStreamType: {
			/* Push the args backwards. */
			for ( CallArgVect::Iter pex = exprPtrVect->last(); pex.gtb(); pex-- )
				(*pex)->expr->resolve( pd );
			break;
		}
		case PrintAccum: {
			consItemList->resolve( pd );
			break;
		}
		case ExprType: {
			/* Evaluate the exrepssion, then pop it immediately. */
			expr->resolve( pd );
			break;
		}
		case IfType: {
			/* Evaluate the test. */
			expr->resolve( pd );

			/* Analyze the if true branch. */
			for ( StmtList::Iter stmt = *stmtList; stmt.lte(); stmt++ )
				stmt->resolve( pd );

			if ( elsePart != 0 )
				elsePart->resolve( pd );

			break;
		}
		case ElseType: {
			for ( StmtList::Iter stmt = *stmtList; stmt.lte(); stmt++ )
				stmt->resolve( pd );
			break;
		}
		case RejectType:
			break;
		case WhileType: {
			expr->resolve( pd );

			/* Compute the while block. */
			for ( StmtList::Iter stmt = *stmtList; stmt.lte(); stmt++ )
				stmt->resolve( pd );
			break;
		}
		case AssignType: {
			/* Evaluate the exrepssion. */
			expr->resolve( pd );
			break;
		}
		case ForIterType: {
			resolveForIter( pd );
			break;
		}
		case ReturnType: {
			/* Evaluate the exrepssion. */
			expr->resolve( pd );
			break;
		}
		case BreakType: {
			break;
		}
		case YieldType: {
			/* take a reference and yield it. Immediately reset the referece. */
			varRef->resolve( pd );
			break;
		}
	}
}

void ObjectDef::resolve( Compiler *pd )
{
	for ( FieldList::Iter fli = fieldList; fli.lte(); fli++ ) {
		ObjectField *field = fli->value;

		if ( field->typeRef != 0 )
			field->typeRef->resolveType( pd );
	}
}

void CodeBlock::resolve( Compiler *pd ) const
{
	if ( localFrame != 0 ) {
		localFrame->resolve( pd );
	}

	for ( StmtList::Iter stmt = *stmtList; stmt.lte(); stmt++ )
		stmt->resolve( pd );
}

void Compiler::resolveFunction( Function *func )
{
	if ( func->typeRef != 0 ) 
		func->typeRef->resolveType( this );

	for ( ParameterList::Iter param = *func->paramList; param.lte(); param++ )
		param->typeRef->resolveType( this );

	CodeBlock *block = func->codeBlock;
	block->resolve( this );
}

void Compiler::resolveInHost( Function *func )
{
	if ( func->typeRef != 0 ) 
		func->typeRef->resolveType( this );

	for ( ParameterList::Iter param = *func->paramList; param.lte(); param++ )
		param->typeRef->resolveType( this );
}


void Compiler::resolvePreEof( TokenRegion *region )
{
	CodeBlock *block = region->preEofBlock;
	block->resolve( this );
}

void Compiler::resolveRootBlock()
{
	CodeBlock *block = rootCodeBlock;
	block->resolve( this );
}

void Compiler::resolveTranslateBlock( LangEl *langEl )
{
	CodeBlock *block = langEl->transBlock;
	block->resolve( this );
}

void Compiler::resolveReductionCode( Production *prod )
{
	CodeBlock *block = prod->redBlock;
	block->resolve( this );
}

void Compiler::resolveParseTree()
{
	/* Compile functions. */
	for ( FunctionList::Iter f = functionList; f.lte(); f++ )
		resolveFunction( f );

	for ( FunctionList::Iter f = inHostList; f.lte(); f++ )
		resolveInHost( f );

	/* Compile the reduction code. */
	for ( DefList::Iter prod = prodList; prod.lte(); prod++ ) {
		if ( prod->redBlock != 0 )
			resolveReductionCode( prod );
	}

	/* Compile the token translation code. */
	for ( LelList::Iter lel = langEls; lel.lte(); lel++ ) {
		if ( lel->transBlock != 0 )
			resolveTranslateBlock( lel );
	}

	/* Compile preeof blocks. */
	for ( RegionList::Iter r = regionList; r.lte(); r++ ) {
		if ( r->preEofBlock != 0 )
			resolvePreEof( r );
	}

	/* Compile the init code */
	resolveRootBlock( );

	rootLocalFrame->resolve( this );

	/* Init all user object fields (need consistent size). */
	for ( LelList::Iter lel = langEls; lel.lte(); lel++ ) {
		ObjectDef *objDef = lel->objectDef;
		if ( objDef != 0 ) {
			/* Init all fields of the object. */
			for ( FieldList::Iter f = objDef->fieldList; f.lte(); f++ )
				f->value->typeRef->resolveType( this );
		}
	}

	for ( StructElList::Iter sel = structEls; sel.lte(); sel++ ) {
		ObjectDef *objDef = sel->structDef->objectDef;
		for ( FieldList::Iter f = objDef->fieldList; f.lte(); f++ )
			f->value->typeRef->resolveType( this );
	}

	/* Init all fields of the global object. */
	for ( FieldList::Iter f = globalObjectDef->fieldList; f.lte(); f++ ) {
		f->value->typeRef->resolveType( this );
	}
}

/* Resolves production els and computes the precedence of each prod. */
void Compiler::resolveProductionEls()
{
	/* NOTE: as we process this list it may be growing! */
	for ( DefList::Iter prod = prodList; prod.lte(); prod++ ) {
		/* First resolve. */
		for ( ProdElList::Iter prodEl = *prod->prodElList; prodEl.lte(); prodEl++ )
			resolveProdEl( prodEl );

		/* If there is no explicit precdence ... */
		if ( prod->predOf == 0 )  {
			/* Compute the precedence of the productions. */
			for ( ProdElList::Iter prodEl = prod->prodElList->last(); prodEl.gtb(); prodEl-- ) {
				/* Production inherits the precedence of the last terminal with
				 * precedence. */
				if ( prodEl->langEl->predType != PredNone ) {
					prod->predOf = prodEl->langEl;
					break;
				}
			}
		}
	}
}

void Compiler::makeTerminalWrappers()
{
	/* Make terminal language elements corresponding to each nonterminal in
	 * the grammar. */
	for ( LelList::Iter lel = langEls; lel.lte(); lel++ ) {
		if ( lel->type == LangEl::NonTerm ) {
			String name( lel->name.length() + 5, "_T_%s", lel->name.data );
			LangEl *termDup = new LangEl( lel->nspace, name, LangEl::Term );

			/* Give the dup the attributes of the nonterminal. This ensures
			 * that the attributes are allocated when patterns and
			 * constructors are parsed. */
			termDup->objectDef = lel->objectDef;

			langEls.append( termDup );
			lel->termDup = termDup;
			termDup->termDup = lel;
		}
	}
}

void Compiler::makeEofElements()
{
	/* Make eof language elements for each user terminal. This is a bit excessive and
	 * need to be reduced to the ones that we need parsers for, but we don't know that yet.
	 * Another pass before this one is needed. */
	for ( LelList::Iter lel = langEls; lel.lte(); lel++ ) {
		if ( lel->eofLel == 0 &&
				lel != eofLangEl &&
				lel != errorLangEl &&
				lel != noTokenLangEl /* &&
				!( lel->tokenInstance == 0 || lel->tokenInstance->dupOf == 0 ) */ )
		{
			String name( lel->name.length() + 5, "_eof_%s", lel->name.data );
			LangEl *eofLel = new LangEl( lel->nspace, name, LangEl::Term );

			langEls.append( eofLel );
			lel->eofLel = eofLel;
			eofLel->eofLel = lel;
			eofLel->isEOF = true;
		}
	}
}

void Compiler::resolvePrecedence()
{
	for ( PredDeclList::Iter predDecl = predDeclList; predDecl != 0; predDecl++ ) {
		predDecl->typeRef->resolveType( this );

		LangEl *langEl = predDecl->typeRef->uniqueType->langEl;
		langEl->predType = predDecl->predType;
		langEl->predValue = predDecl->predValue;
	}
}

void Compiler::resolveReductionActions()
{
	for ( ReductionVect::Iter r = rootNamespace->reductions; r.lte(); r++ ) {
		for ( ReduceNonTermList::Iter rni = (*r)->reduceNonTerms; rni.lte(); rni++ )
			rni->nonTerm->resolveType( this );

		for ( ReduceActionList::Iter rai = (*r)->reduceActions; rai.lte(); rai++ )
			rai->nonTerm->resolveType( this );
	}
}

void Compiler::findReductionActionProds()
{
	for ( ReductionVect::Iter r = rootNamespace->reductions; r.lte(); r++ ) {
		for ( ReduceActionList::Iter rai = (*r)->reduceActions; rai.lte(); rai++ ) {
			rai->nonTerm->resolveType( this );
			LangEl *langEl = rai->nonTerm->uniqueType->langEl;

			Production *prod = 0;
			for ( LelDefList::Iter ldi = langEl->defList; ldi.lte(); ldi++ ) {
				if ( strcmp( ldi->name, rai->prod ) == 0 ) {
					prod = ldi;
					break;
				}
			}

			if ( prod == 0 ) {
				error(rai->loc) << "could not find production \"" <<
						rai->prod << "\"" << endp;
			}

			rai->production = prod;
		}
	}
}

void Compiler::resolveReducers()
{
	for ( ParserTextList::Iter pt = parserTextList; pt.lte(); pt++ ) {
		if ( pt->reduce ) {
			Reduction *reduction = rootNamespace->findReduction( pt->reducer );
			if ( reduction == 0 ) {
				error ( pt->loc ) << "could not locate reduction \"" <<
					pt->reducer << "\"" << endp;
			}

			pt->reducerId = reduction->id;
		}
	}
}

void Compiler::resolvePass()
{
	/*
	 * Type Resolving.
	 */

	resolvePrecedence();

	resolveParseTree();

	UniqueType *argvUT = argvTypeRef->resolveType( this );
	argvElSel = argvUT->generic->elUt->structEl;

	resolveReductionActions();

	/* We must do this as the last step in the type resolution process because
	 * all type resolves can cause new language elments with associated
	 * productions. They get tacked onto the end of the list of productions.
	 * Doing it at the end results processing a growing list. */
	resolveProductionEls();

	findReductionActionProds();

	resolveReducers();
}
