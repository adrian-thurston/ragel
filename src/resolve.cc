/*
 *  Copyright 2009-2012 Adrian Thurston <thurston@complang.org>
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

UniqueType *TypeRef::lookupTypeName( Compiler *pd )
{
	/* Lookup up the qualifiction and then the name. */
	nspace = nspaceQual->getQual( pd );

	if ( nspace == 0 )
		error(loc) << "do not have region for resolving reference" << endp;

	while ( nspace != 0 ) {
		/* Search for the token in the region by typeName. */
		TypeMapEl *inDict = nspace->typeMap.find( typeName );

		if ( inDict != 0 ) {
			switch ( inDict->type ) {
				/* Defer to the typeRef we are an alias of. We need to guard against loops here. */
				case TypeMapEl::TypeAliasType:
					return inDict->typeRef->lookupType( pd );

				case TypeMapEl::LangElType:
					return pd->findUniqueType( TYPE_TREE, inDict->value );
			}
		}

		if ( nspaceQual->thisOnly() )
			break;

		nspace = nspace->parentNamespace;
	}

	error(loc) << "unknown type in typeof expression" << endp;
	return 0;
}

UniqueType *TypeRef::lookupTypeLiteral( Compiler *pd )
{
	/* Lookup up the qualifiction and then the name. */
	nspace = nspaceQual->getQual( pd );

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

	error(loc) << "unknown type in typeof expression" << endp;
	return 0;
}

UniqueType *TypeRef::lookupTypeMap( Compiler *pd )
{
	nspace = pd->rootNamespace;

	UniqueType *utKey = typeRef1->lookupType( pd );	
	UniqueType *utValue = typeRef2->lookupType( pd );	

	UniqueMap searchKey( utKey, utValue );
	UniqueMap *inMap = pd->uniqueMapMap.find( &searchKey );
	if ( inMap == 0 ) {
		inMap = new UniqueMap( utKey, utValue );
		pd->uniqueMapMap.insert( inMap );

		/* FIXME: Need uniqe name allocator for types. */
		static int mapId = 0;
		String name( 36, "__map%d", mapId++ );

		GenericType *generic = new GenericType( name, GEN_MAP,
				pd->nextGenericId++, 0/*langEl*/, typeRef2 );
		generic->keyTypeArg = typeRef1;

		nspace->genericList.append( generic );

		generic->declare( pd, nspace );

		inMap->generic = generic;
	}

	generic = inMap->generic;
	return pd->findUniqueType( TYPE_TREE, inMap->generic->langEl );
}

UniqueType *TypeRef::lookupTypeList( Compiler *pd )
{
	nspace = pd->rootNamespace;

	UniqueType *utValue = typeRef1->lookupType( pd );	

	UniqueList searchKey( utValue );
	UniqueList *inMap = pd->uniqueListMap.find( &searchKey );
	if ( inMap == 0 ) {
		inMap = new UniqueList( utValue );
		pd->uniqueListMap.insert( inMap );

		/* FIXME: Need uniqe name allocator for types. */
		static int listId = 0;
		String name( 36, "__list%d", listId++ );

		GenericType *generic = new GenericType( name, GEN_LIST,
				pd->nextGenericId++, 0/*langEl*/, typeRef1 );

		nspace->genericList.append( generic );

		generic->declare( pd, nspace );

		inMap->generic = generic;
	}

	generic = inMap->generic;
	return pd->findUniqueType( TYPE_TREE, inMap->generic->langEl );
}

UniqueType *TypeRef::lookupTypeVector( Compiler *pd )
{
	nspace = pd->rootNamespace;

	UniqueType *utValue = typeRef1->lookupType( pd );	

	UniqueVector searchKey( utValue );
	UniqueVector *inMap = pd->uniqueVectorMap.find( &searchKey );
	if ( inMap == 0 ) {
		inMap = new UniqueVector( utValue );
		pd->uniqueVectorMap.insert( inMap );

		/* FIXME: Need uniqe name allocator for types. */
		static int vectorId = 0;
		String name( 36, "__vector%d", vectorId++ );

		GenericType *generic = new GenericType( name, GEN_VECTOR,
				pd->nextGenericId++, 0/*langEl*/, typeRef1 );

		nspace->genericList.append( generic );

		generic->declare( pd, nspace );

		inMap->generic = generic;
	}

	generic = inMap->generic;
	return pd->findUniqueType( TYPE_TREE, inMap->generic->langEl );
}

UniqueType *TypeRef::lookupTypeParser( Compiler *pd )
{
	nspace = pd->rootNamespace;

	UniqueType *utParse = typeRef1->lookupType( pd );	

	UniqueParser searchKey( utParse );
	UniqueParser *inMap = pd->uniqueParserMap.find( &searchKey );
	if ( inMap == 0 ) {
		inMap = new UniqueParser( utParse );
		pd->uniqueParserMap.insert( inMap );

		/* FIXME: Need uniqe name allocator for types. */
		static int accumId = 0;
		String name( 36, "__accum%d", accumId++ );

		GenericType *generic = new GenericType( name, GEN_PARSER,
				pd->nextGenericId++, 0/*langEl*/, typeRef1 );

		nspace->genericList.append( generic );

		generic->declare( pd, nspace );

		inMap->generic = generic;
	}

	generic = inMap->generic;
	return pd->findUniqueType( TYPE_TREE, inMap->generic->langEl );
}

UniqueType *TypeRef::lookupTypePtr( Compiler *pd )
{
	typeRef1->lookupType( pd );
	return pd->findUniqueType( TYPE_PTR, typeRef1->uniqueType->langEl );
}

UniqueType *TypeRef::lookupTypeRef( Compiler *pd )
{
	typeRef1->lookupType( pd );
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
				declLangEl = pd->makeRepeatProd( loc, nspace, repeatName, nspaceQual, typeName );
				break;
			}
			case RepeatList: {
				/* If the factor is a repeat, create the repeat element and link the
				 * factor to it. */
				String listName( 128, "_list_%s", typeName.data );
				declLangEl = pd->makeListProd( loc, nspace, listName, nspaceQual, typeName );
				break;
			}
			case RepeatOpt: {
				/* If the factor is an opt, create the opt element and link the factor
				 * to it. */
				String optName( 128, "_opt_%s", typeName.data );
				declLangEl = pd->makeOptProd( loc, nspace, optName, nspaceQual, typeName );
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

UniqueType *TypeRef::lookupIterator( Compiler *pd )
{
	UniqueType *searchUT = searchTypeRef->lookupType( pd );

	/* Lookup the iterator call. Make sure it is an iterator. */
	VarRefLookup lookup = iterCall->langTerm->varRef->lookupMethod( pd );
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


UniqueType *TypeRef::lookupType( Compiler *pd )
{
	if ( uniqueType != 0 )
		return uniqueType;

	/* Not an iterator. May be a reference. */
	switch ( type ) {
		case Name:
			uniqueType = lookupTypeName( pd );
			break;
		case Literal:
			uniqueType = lookupTypeLiteral( pd );
			break;
		case Map:
			uniqueType = lookupTypeMap( pd );
			break;
		case List:
			uniqueType = lookupTypeList( pd );
			break;
		case Vector:
			uniqueType = lookupTypeVector( pd );
			break;
		case Parser:
			uniqueType = lookupTypeParser( pd );
			break;
		case Ptr:
			uniqueType = lookupTypePtr( pd );
			break;
		case Ref:
			uniqueType = lookupTypeRef( pd );
			break;
		case Iterator:
			uniqueType = lookupIterator( pd );
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
	prodEl->typeRef->lookupType( this );
	prodEl->langEl = prodEl->typeRef->uniqueType->langEl;
}

void LangTerm::resolve( Compiler *pd )
{
	switch ( type ) {
		case ConstructType:
			typeRef->lookupType( pd );

			/* Initialization expressions. */
			if ( fieldInitArgs != 0 ) {
				for ( FieldInitVect::Iter pi = *fieldInitArgs; pi.lte(); pi++ )
					(*pi)->expr->resolve( pd );
			}

			/* Types in constructor. */
			for ( ConsItemList::Iter item = *constructor->list; item.lte(); item++ ) {
				switch ( item->type ) {
				case ConsItem::LiteralType:
					/* Use pdaFactor reference resolving. */
					pd->resolveProdEl( item->prodEl );
					break;
				case ConsItem::InputText:
				case ConsItem::ExprType:
					break;
				}
			}
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
				case PatternItem::TypeRef:
					/* Use pdaFactor reference resolving. */
					pd->resolveProdEl( item->prodEl );
					break;
				case PatternItem::InputText:
					/* Nothing to do here. */
					break;
				}
			}

			break;
		case NewType:
			expr->resolve( pd );
			break;
		case TypeIdType:
			typeRef->lookupType( pd );
			break;
		case SearchType:
			typeRef->lookupType( pd );
			break;
		case NilType:
		case TrueType:
		case FalseType:
			break;

		case ParseStopType:
		case ParseType:
			typeRef->lookupType( pd );
			/* Evaluate the initialization expressions. */
			if ( fieldInitArgs != 0 ) {
				for ( FieldInitVect::Iter pi = *fieldInitArgs; pi.lte(); pi++ )
					(*pi)->expr->resolve( pd );
			}

			for ( ConsItemList::Iter item = *parserText->list; item.lte(); item++ ) {
				switch ( item->type ) {
				case ConsItem::LiteralType:
					pd->resolveProdEl( item->prodEl );
					break;
				case ConsItem::InputText:
				case ConsItem::ExprType:
					break;
				}
			}
			break;

		case SendType:
			break;
		case EmbedStringType:
			break;
		case CastType:
			typeRef->lookupType( pd );
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

void LangIterCall::resolve( Compiler *pd ) const
{
	switch ( type ) {
		case IterCall:
			langTerm->resolve( pd );
			break;
		case VarRef:
		case Expr:
			langExpr->resolve( pd );
			break;
	}
}

void LangStmt::resolveForIter( Compiler *pd ) const
{
	iterCall->resolve( pd );

	/* Search type ref. */
	typeRef->lookupType( pd );

	/* Iterator type ref. */
	objField->typeRef->lookupType( pd );

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
	for ( ObjFieldList::Iter fli = *objFieldList; fli.lte(); fli++ ) {
		ObjectField *field = fli->value;

		if ( field->typeRef != 0 ) {
			field->typeRef->lookupType( pd );
		}
	}
}

void CodeBlock::resolve( Compiler *pd ) const
{
	if ( localFrame != 0 )
		localFrame->resolve( pd );

	for ( StmtList::Iter stmt = *stmtList; stmt.lte(); stmt++ )
		stmt->resolve( pd );
}

void Compiler::resolveFunction( Function *func )
{
	if ( func->typeRef != 0 ) 
		func->typeRef->lookupType( this );

	for ( ParameterList::Iter param = *func->paramList; param.lte(); param++ )
		param->typeRef->lookupType( this );

	CodeBlock *block = func->codeBlock;
	block->resolve( this );
}

void Compiler::resolvePreEof( TokenRegion *region )
{
	CodeBlock *block = region->preEofBlock;
	block->resolve( this );
}

void Compiler::resolveRootBlock()
{
	rootLocalFrame->resolve( this );

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

	/* Init all user object fields (need consistent size). */
	for ( LelList::Iter lel = langEls; lel.lte(); lel++ ) {
		ObjectDef *objDef = lel->objectDef;
		if ( objDef != 0 ) {
			/* Init all fields of the object. */
			for ( ObjFieldList::Iter f = *objDef->objFieldList; f.lte(); f++ ) {
				f->value->typeRef->lookupType( this );
			}
		}
	}

	/* Init all fields of the global object. */
	for ( ObjFieldList::Iter f = *globalObjectDef->objFieldList; f.lte(); f++ ) {
		f->value->typeRef->lookupType( this );
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

void Compiler::resolveGenericTypes()
{
	for ( NamespaceList::Iter ns = namespaceList; ns.lte(); ns++ ) {
		for ( GenericList::Iter gen = ns->genericList; gen.lte(); gen++ ) {
			gen->utArg = gen->typeArg->lookupType( this );

			if ( gen->typeId == GEN_MAP )
				gen->keyUT = gen->keyTypeArg->lookupType( this );
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

void Compiler::typeResolve()
{
	/*
	 * Type Resolving.
	 */

	resolveParseTree();

	resolveGenericTypes();

	argvTypeRef->lookupType( this );

	/* We must do this as the last step in the type resolution process because
	 * all type resolves can cause new language elments with associated
	 * productions. They get tacked onto the end of the list of productions.
	 * Doing it at the end results processing a growing list. */
	resolveProductionEls();
}
