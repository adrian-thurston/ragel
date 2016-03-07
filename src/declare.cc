/*
 *  Copyright 2012 Adrian Thurston <thurston@complang.org>
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
#include "compiler.h"
#include <iostream>
#include <assert.h>

void Compiler::initUniqueTypes( )
{
	uniqueTypeNil = new UniqueType( TYPE_NIL );
	uniqueTypeVoid = new UniqueType( TYPE_TREE, voidLangEl );
	uniqueTypePtr = new UniqueType( TYPE_TREE, ptrLangEl );
	uniqueTypeBool = new UniqueType( TYPE_BOOL );
	uniqueTypeInt = new UniqueType( TYPE_INT );
	uniqueTypeStr = new UniqueType( TYPE_TREE, strLangEl );
	uniqueTypeIgnore = new UniqueType( TYPE_TREE, ignoreLangEl );
	uniqueTypeAny = new UniqueType( TYPE_TREE, anyLangEl );

	uniqueTypeStream = new UniqueType( TYPE_STRUCT, streamSel );

	uniqeTypeMap.insert( uniqueTypeNil );
	uniqeTypeMap.insert( uniqueTypeVoid );
	uniqeTypeMap.insert( uniqueTypePtr );
	uniqeTypeMap.insert( uniqueTypeBool );
	uniqeTypeMap.insert( uniqueTypeInt );
	uniqeTypeMap.insert( uniqueTypeStr );
	uniqeTypeMap.insert( uniqueTypeIgnore );
	uniqeTypeMap.insert( uniqueTypeAny );

	uniqeTypeMap.insert( uniqueTypeStream );
}

ObjectMethod *initFunction( UniqueType *retType, Namespace *nspace, ObjectDef *obj, 
		const String &name, int methIdWV, int methIdWC,
		int nargs, UniqueType **args, bool isConst, bool useFnInstr,
		GenericType *useGeneric )
{
	ObjectMethod *objMethod = new ObjectMethod( retType, name, 
			methIdWV, methIdWC, nargs, args, 0, isConst );
	objMethod->useFnInstr = useFnInstr;

	if ( nspace != 0 )
		nspace->rootScope->methodMap.insert( name, objMethod );
	else
		obj->rootScope->methodMap.insert( name, objMethod );

	if ( useGeneric ) {
		objMethod->useGenericId = true;
		objMethod->generic = useGeneric;
	}

	return objMethod;
}

ObjectMethod *initFunction( UniqueType *retType, ObjectDef *obj, 
		const String &name, int methIdWV, int methIdWC, bool isConst,
		bool useFnInstr, GenericType *useGeneric )
{
	return initFunction( retType, 0, obj, name, methIdWV, methIdWC,
			0, 0, isConst, useFnInstr, useGeneric );
}

ObjectMethod *initFunction( UniqueType *retType, ObjectDef *obj, 
		const String &name, int methIdWV, int methIdWC, UniqueType *arg1,
		bool isConst, bool useFnInstr, GenericType *useGeneric )
{
	UniqueType *args[] = { arg1 };
	return initFunction( retType, 0, obj, name, methIdWV, methIdWC,
			1, args, isConst, useFnInstr, useGeneric );
}

ObjectMethod *initFunction( UniqueType *retType, ObjectDef *obj, 
		const String &name, int methIdWV, int methIdWC, 
		UniqueType *arg1, UniqueType *arg2,
		bool isConst, bool useFnInstr, GenericType *useGeneric )
{
	UniqueType *args[] = { arg1, arg2 };
	return initFunction( retType, 0, obj, name, methIdWV, methIdWC,
			2, args, isConst, useFnInstr, useGeneric );
}

/*
 * With namespace supplied. Global functions.
 */

ObjectMethod *initFunction( UniqueType *retType, Namespace *nspace, ObjectDef *obj, 
		const String &name, int methIdWV, int methIdWC, bool isConst,
		bool useFnInstr, GenericType *useGeneric )
{
	return initFunction( retType, nspace, obj, name, methIdWV, methIdWC,
			0, 0, isConst, useFnInstr, useGeneric );
}

ObjectMethod *initFunction( UniqueType *retType, Namespace *nspace, ObjectDef *obj, 
		const String &name, int methIdWV, int methIdWC, UniqueType *arg1,
		bool isConst, bool useFnInstr, GenericType *useGeneric )
{
	UniqueType *args[] = { arg1 };
	return initFunction( retType, nspace, obj, name, methIdWV, methIdWC,
			1, args, isConst, useFnInstr, useGeneric );
}

ObjectMethod *initFunction( UniqueType *retType, Namespace *nspace, ObjectDef *obj, 
		const String &name, int methIdWV, int methIdWC, 
		UniqueType *arg1, UniqueType *arg2,
		bool isConst, bool useFnInstr, GenericType *useGeneric )
{
	UniqueType *args[] = { arg1, arg2 };
	return initFunction( retType, nspace, obj, name, methIdWV, methIdWC,
			2, args, isConst, useFnInstr, useGeneric );
}

ObjectField *NameScope::checkRedecl( const String &name )
{
	return owningObj->checkRedecl( this, name );
}

void NameScope::insertField( const String &name, ObjectField *value )
{
	return owningObj->insertField( this, name, value );
}

ObjectField *ObjectDef::checkRedecl( NameScope *inScope, const String &name )
{
	FieldMapEl *objDefMapEl = inScope->fieldMap.find( name );
	if ( objDefMapEl != 0 )
		return objDefMapEl->value;
	return 0;
}

void ObjectDef::insertField( NameScope *inScope, const String &name, ObjectField *value )
{
	inScope->fieldMap.insert( name, value );
	fieldList.append( value );
	value->scope = inScope;
}

NameScope *ObjectDef::pushScope( NameScope *curScope )
{
	NameScope *newScope = new NameScope;

	newScope->owningObj = this;
	newScope->parentScope = curScope;
	curScope->children.append( newScope );

	return newScope;
}

void LexJoin::varDecl( Compiler *pd, TokenDef *tokenDef )
{
	expr->varDecl( pd, tokenDef );
}

void LexExpression::varDecl( Compiler *pd, TokenDef *tokenDef )
{
	switch ( type ) {
		case OrType: case IntersectType: case SubtractType:
		case StrongSubtractType:
			expression->varDecl( pd, tokenDef );
			term->varDecl( pd, tokenDef );
			break;
		case TermType:
			term->varDecl( pd, tokenDef );
			break;
		case BuiltinType:
			break;
	}
}

void LexTerm::varDecl( Compiler *pd, TokenDef *tokenDef )
{
	switch ( type ) {
		case ConcatType:
		case RightStartType:
		case RightFinishType:
		case LeftType:
			term->varDecl( pd, tokenDef );
			factorAug->varDecl( pd, tokenDef );
			break;
		case FactorAugType:
			factorAug->varDecl( pd, tokenDef );
			break;
	}
}

void LexFactorAug::varDecl( Compiler *pd, TokenDef *tokenDef )
{
	for ( ReCaptureVect::Iter re = reCaptureVect; re.lte(); re++ ) {
		if ( tokenDef->objectDef->rootScope->checkRedecl( re->objField->name ) != 0 ) {
			error(re->objField->loc) << "label name \"" <<
					re->objField->name << "\" already in use" << endp;
		}

		/* Insert it into the map. */
		tokenDef->objectDef->rootScope->insertField( re->objField->name, re->objField );

		/* Store it in the TokenDef. */
		tokenDef->reCaptureVect.append( *re );
	}
}

void Compiler::declareReVars()
{
	for ( NamespaceList::Iter n = namespaceList; n.lte(); n++ ) {
		for ( TokenDefListNs::Iter tok = n->tokenDefList; tok.lte(); tok++ ) {
			if ( tok->join != 0 )
				tok->join->varDecl( this, tok );
		}
	}

	/* FIXME: declare RE captures in token generation actions. */
#if 0
	/* Add captures to the local frame. We Depend on these becoming the
	 * first local variables so we can compute their location. */

	/* Make local variables corresponding to the local capture vector. */
	for ( ReCaptureVect::Iter c = reCaptureVect; c.lte(); c++ )
	{
		ObjectField *objField = ObjectField::cons( c->objField->loc,
				c->objField->typeRef, c->objField->name );

		/* Insert it into the field map. */
		pd->curLocalFrame->insertField( objField->name, objField );
	}
#endif
}

LangEl *declareLangEl( Compiler *pd, Namespace *nspace,
		const String &data, LangEl::Type type )
{
    /* If the id is already in the dict, it will be placed in last found. If
     * it is not there then it will be inserted and last found will be set to it. */
	TypeMapEl *inDict = nspace->typeMap.find( data );
	if ( inDict != 0 )
		error() << "language element '" << data << "' already defined as something else" << endp;

	/* Language element not there. Make the new lang el and insert.. */
	LangEl *langEl = new LangEl( nspace, data, type );
	TypeMapEl *typeMapEl = new TypeMapEl( TypeMapEl::LangElType, data, langEl );
	nspace->typeMap.insert( typeMapEl );
	pd->langEls.append( langEl );

	return langEl;
}

StructEl *declareStruct( Compiler *pd, Namespace *inNspace,
		const String &data, StructDef *structDef )
{
	if ( inNspace != 0 ) {
		TypeMapEl *inDict = inNspace->typeMap.find( data );
		if ( inDict != 0 )
			error() << "struct '" << data << "' already defined as something else" << endp;
	}

	StructEl *structEl = new StructEl( data, structDef );
	pd->structEls.append( structEl );
	structDef->structEl = structEl;

	if ( inNspace ) {
		TypeMapEl *typeMapEl = new TypeMapEl( TypeMapEl::StructType, data, structEl );
		inNspace->typeMap.insert( typeMapEl );
	}

	return structEl;
}

/* Does not map the new language element. */
LangEl *addLangEl( Compiler *pd, Namespace *inNspace,
		const String &data, LangEl::Type type )
{
	LangEl *langEl = new LangEl( inNspace, data, type );
	pd->langEls.append( langEl );
	return langEl;
}

void declareTypeAlias( Compiler *pd, Namespace *nspace,
		const String &data, TypeRef *typeRef )
{
    /* If the id is already in the dict, it will be placed in last found. If
     * it is not there then it will be inserted and last found will be set to it. */
	TypeMapEl *inDict = nspace->typeMap.find( data );
	if ( inDict != 0 )
		error() << "alias '" << data << "' already defined as something else" << endp;

	/* Language element not there. Make the new lang el and insert. */
	TypeMapEl *typeMapEl = new TypeMapEl( TypeMapEl::AliasType, data, typeRef );
	nspace->typeMap.insert( typeMapEl );
}

LangEl *findType( Compiler *pd, Namespace *nspace, const String &data )
{
	/* If the id is already in the dict, it will be placed in last found. If
	 * it is not there then it will be inserted and last found will be set to it. */
	TypeMapEl *inDict = nspace->typeMap.find( data );

	if ( inDict == 0 )
		error() << "'" << data << "' not declared as anything" << endp;

	return inDict->value;
}


void Compiler::declareBaseLangEls()
{
	/* Order here is important because we make assumptions about the inbuilt
	 * language elements in the runtime. Note tokens are have identifiers set
	 * in an initial pass. */

	/* Make a "_notoken" language element. This element is used when a
	 * generation action fails to generate anything, but there is reverse code
	 * that needs to be associated with a language element. This allows us to
	 * always associate reverse code with the first language element produced
	 * after a generation action. */
	noTokenLangEl = declareLangEl( this, rootNamespace, "_notoken", LangEl::Term );
	noTokenLangEl->isIgnore = true;
	
	ptrLangEl = declareLangEl( this, rootNamespace, "ptr", LangEl::Term );
	voidLangEl = declareLangEl( this, rootNamespace, "void", LangEl::Term );
	strLangEl = declareLangEl( this, rootNamespace, "str", LangEl::Term );
	ignoreLangEl = declareLangEl( this, rootNamespace, "il", LangEl::Term );

	/* Make the EOF language element. */
	eofLangEl = 0;

	/* Make the "any" language element */
	anyLangEl = declareLangEl( this, rootNamespace, "any", LangEl::NonTerm );
}


void Compiler::addProdRedObjectVar( ObjectDef *localFrame, LangEl *nonTerm )
{
	UniqueType *prodNameUT = findUniqueType( TYPE_TREE, nonTerm );
	TypeRef *typeRef = TypeRef::cons( internal, prodNameUT );
	ObjectField *el = ObjectField::cons( internal,
			ObjectField::LhsElType, typeRef, "lhs" );

	localFrame->rootScope->insertField( el->name, el );
}

void Compiler::addProdRHSVars( ObjectDef *localFrame, ProdElList *prodElList )
{
	long position = 1;
	for ( ProdElList::Iter rhsEl = *prodElList; rhsEl.lte(); rhsEl++, position++ ) {
		if ( rhsEl->type == ProdEl::ReferenceType ) {
			/* Use an offset of zero. For frame objects we compute the offset on
			 * demand. */
			String name( 8, "r%d", position );
			ObjectField *el = ObjectField::cons( InputLoc(),
					ObjectField::RedRhsType, rhsEl->typeRef, name );
			rhsEl->rhsElField = el;

			/* Right hand side elements are constant. */
			el->isConst = true;
			localFrame->rootScope->insertField( el->name, el );
		}
	}
}

void GenericType::declare( Compiler *pd, Namespace *nspace )
{
	elUt = elTr->resolveType( pd );
 
	if ( typeId == GEN_MAP )
		keyUt = keyTr->resolveType( pd );
	
	if ( typeId == GEN_MAP || typeId == GEN_LIST )
		valueUt = valueTr->resolveType( pd );
	
	objDef = ObjectDef::cons( ObjectDef::BuiltinType, 
			"generic", pd->nextObjectId++ );

	switch ( typeId ) {
		case GEN_MAP:
			pd->initMapFunctions( this );
			pd->initMapFields( this );
			break;
		case GEN_LIST:
			pd->initListFunctions( this );
			pd->initListFields( this );
			break;
		case GEN_PARSER:
			elUt->langEl->parserId = pd->nextParserId++;
			pd->initParserFunctions( this );
			pd->initParserFields( this );
			break;
	}
}

void Namespace::declare( Compiler *pd )
{
	for ( GenericList::Iter g = genericList; g.lte(); g++ )
		g->declare( pd, this );

	for ( TokenDefListNs::Iter tokenDef = tokenDefList; tokenDef.lte(); tokenDef++ ) {
		if ( tokenDef->isLiteral ) {
			if ( tokenDef->isZero ) {
				assert( tokenDef->regionSet->collectIgnore->zeroLel != 0 );
				tokenDef->tdLangEl = tokenDef->regionSet->collectIgnore->zeroLel;
			}
			else {
				/* Original. Create a token for the literal. */
				LangEl *litEl = declareLangEl( pd, this, tokenDef->name, LangEl::Term );

				litEl->lit = tokenDef->literal;
				litEl->isLiteral = true;
				litEl->tokenDef = tokenDef;
				litEl->objectDef = tokenDef->objectDef;

				tokenDef->tdLangEl = litEl;

				if ( tokenDef->noPreIgnore )
					litEl->noPreIgnore = true;
				if ( tokenDef->noPostIgnore )
					litEl->noPostIgnore = true;
			}
		}
	}

	for ( StructDefList::Iter s = structDefList; s.lte(); s++ )
		declareStruct( pd, this, s->name, s );

	for ( TokenDefListNs::Iter tokenDef = tokenDefList; tokenDef.lte(); tokenDef++ ) {
		/* Literals already taken care of. */
		if ( ! tokenDef->isLiteral ) {
			/* Create the token. */
			LangEl *tokEl = declareLangEl( pd, this, tokenDef->name, LangEl::Term );
			tokEl->isIgnore = tokenDef->isIgnore;
			tokEl->transBlock = tokenDef->codeBlock;
			tokEl->objectDef = tokenDef->objectDef;
			tokEl->contextIn = tokenDef->contextIn;
			tokEl->tokenDef = tokenDef;

			if ( tokenDef->noPreIgnore )
				tokEl->noPreIgnore = true;
			if ( tokenDef->noPostIgnore )
				tokEl->noPostIgnore = true;

			tokenDef->tdLangEl = tokEl;

			if ( tokenDef->isZero ) {
				/* Setting zero lel to newly created tokEl. */
				tokenDef->regionSet->collectIgnore->zeroLel = tokEl;
				tokEl->isZero = true;
			}
		}
	}

	for ( NtDefList::Iter n = ntDefList; n.lte(); n++ ) {
		/* Get the language element. */
		LangEl *langEl = declareLangEl( pd, this, n->name, LangEl::NonTerm );
		//$$->langEl = langEl;

		/* Get the language element. */
		langEl->objectDef = n->objectDef;
		langEl->reduceFirst = n->reduceFirst;
		langEl->contextIn = n->contextIn;
		langEl->defList.transfer( *n->defList );

		for ( LelDefList::Iter d = langEl->defList; d.lte(); d++ ) {
			d->prodName = langEl;

			if ( d->redBlock != 0 ) {
				pd->addProdRedObjectVar( d->redBlock->localFrame, langEl );
				pd->addProdRHSVars( d->redBlock->localFrame, d->prodElList );
			}

			/* References to the reduce item. */
		}
	}

	for ( TypeAliasList::Iter ta = typeAliasList; ta.lte(); ta++ )
		declareTypeAlias( pd, this, ta->name, ta->typeRef );

	/* Go into child aliases. */
	for ( NamespaceVect::Iter c = childNamespaces; c.lte(); c++ )
		(*c)->declare( pd );
}

void Compiler::makeIgnoreCollectors()
{
	for ( RegionSetList::Iter regionSet = regionSetList; regionSet.lte(); regionSet++ ) {
		if ( regionSet->collectIgnore->zeroLel == 0 ) {
			String name( 128, "_ign_%p", regionSet->tokenIgnore );
			LangEl *zeroLel = new LangEl( rootNamespace, name, LangEl::Term );
			langEls.append( zeroLel );
			zeroLel->isZero = true;
			zeroLel->regionSet = regionSet;

			regionSet->collectIgnore->zeroLel = zeroLel;
		}
	}
}

void LangStmt::chooseDefaultIter( Compiler *pd, IterCall *iterCall ) const
{
	/* This is two-part, It gets rewritten before evaluation in synthesis. */

	/* The iterator name. */
	LangVarRef *callVarRef = LangVarRef::cons( loc, 0, context, scope, "triter" );

	/* The parameters. */
	CallArgVect *callExprVect = new CallArgVect;
	callExprVect->append( new CallArg( iterCall->langExpr ) );
	iterCall->langTerm = LangTerm::cons( InputLoc(), callVarRef, callExprVect );
	iterCall->langExpr = 0;
	iterCall->form = IterCall::Call;
	iterCall->wasExpr = true;
}

void LangStmt::declareForIter( Compiler *pd ) const
{
	if ( iterCall->form != IterCall::Call )
		chooseDefaultIter( pd, iterCall );

	objField->typeRef = TypeRef::cons( loc, typeRef, iterCall );
}

void LangStmt::declare( Compiler *pd ) const
{
	switch ( type ) {
		case PrintType: 
		case PrintXMLACType:
		case PrintXMLType:
		case PrintStreamType:
		case PrintAccum:
			break;
		case ExprType:
			break;
		case IfType:
			for ( StmtList::Iter stmt = *stmtList; stmt.lte(); stmt++ )
				stmt->declare( pd );

			if ( elsePart != 0 )
				elsePart->declare( pd );
			break;

		case ElseType:
			for ( StmtList::Iter stmt = *stmtList; stmt.lte(); stmt++ )
				stmt->declare( pd );
			break;
		case RejectType:
			break;
		case WhileType:
			for ( StmtList::Iter stmt = *stmtList; stmt.lte(); stmt++ )
				stmt->declare( pd );
			break;
		case AssignType:
			break;
		case ForIterType:
			declareForIter( pd );

			for ( StmtList::Iter stmt = *stmtList; stmt.lte(); stmt++ )
				stmt->declare( pd );
			break;
		case ReturnType:
			break;
		case BreakType:
			break;
		case YieldType:
			break;
	}
}

void CodeBlock::declare( Compiler *pd ) const
{
	for ( StmtList::Iter stmt = *stmtList; stmt.lte(); stmt++ )
		stmt->declare( pd );
}

void Compiler::declareFunction( Function *func )
{
	CodeBlock *block = func->codeBlock;
	block->declare( this );
}

void Compiler::declareReductionCode( Production *prod )
{
	CodeBlock *block = prod->redBlock;
	block->declare( this );
}

void Compiler::declareTranslateBlock( LangEl *langEl )
{
	CodeBlock *block = langEl->transBlock;

	/* References to the reduce item. */
	addMatchLength( block->localFrame, langEl );
	addMatchText( block->localFrame, langEl );
	addInput( block->localFrame );
	addThis( block->localFrame );

	block->declare( this );
}

void Compiler::declarePreEof( TokenRegion *region )
{
	CodeBlock *block = region->preEofBlock;

	addInput( block->localFrame );
	addThis( block->localFrame );

	block->declare( this );
}

void Compiler::declareRootBlock()
{
	CodeBlock *block = rootCodeBlock;
	block->declare( this );
}

void Compiler::declareByteCode()
{
	for ( FunctionList::Iter f = functionList; f.lte(); f++ )
		declareFunction( f );

	for ( DefList::Iter prod = prodList; prod.lte(); prod++ ) {
		if ( prod->redBlock != 0 )
			declareReductionCode( prod );
	}

	for ( LelList::Iter lel = langEls; lel.lte(); lel++ ) {
		if ( lel->transBlock != 0 )
			declareTranslateBlock( lel );
	}

	for ( RegionList::Iter r = regionList; r.lte(); r++ ) {
		if ( r->preEofBlock != 0 )
			declarePreEof( r );
	}

	declareRootBlock( );
}

void Compiler::makeDefaultIterators()
{
	/* Tree iterator. */
	{
		UniqueType *anyRefUT = findUniqueType( TYPE_REF, anyLangEl );
		ObjectMethod *objMethod = initFunction( uniqueTypeAny, rootNamespace, globalObjectDef, 
				"triter", IN_HALT, IN_HALT, anyRefUT, true );

		IterDef *triter = findIterDef( IterDef::Tree );
		objMethod->iterDef = triter;
	}

	/* Child iterator. */
	{
		UniqueType *anyRefUT = findUniqueType( TYPE_REF, anyLangEl );
		ObjectMethod *objMethod = initFunction( uniqueTypeAny, rootNamespace, globalObjectDef, 
				"child", IN_HALT, IN_HALT, anyRefUT, true );

		IterDef *triter = findIterDef( IterDef::Child );
		objMethod->iterDef = triter;
	}

	/* Reverse iterator. */
	{
		UniqueType *anyRefUT = findUniqueType( TYPE_REF, anyLangEl );
		ObjectMethod *objMethod = initFunction( uniqueTypeAny, rootNamespace, globalObjectDef, 
				"rev_child", IN_HALT, IN_HALT, anyRefUT, true );

		IterDef *triter = findIterDef( IterDef::RevChild );
		objMethod->iterDef = triter;
	}

	/* Repeat iterator. */
	{
		UniqueType *anyRefUT = findUniqueType( TYPE_REF, anyLangEl );
		ObjectMethod *objMethod = initFunction( uniqueTypeAny, rootNamespace, globalObjectDef, 
				"repeat", IN_HALT, IN_HALT, anyRefUT, true );

		IterDef *triter = findIterDef( IterDef::Repeat );
		objMethod->iterDef = triter;
	}

	/* Reverse repeat iterator. */
	{
		UniqueType *anyRefUT = findUniqueType( TYPE_REF, anyLangEl );
		ObjectMethod *objMethod = initFunction( uniqueTypeAny, rootNamespace, globalObjectDef, 
				"rev_repeat", IN_HALT, IN_HALT, anyRefUT, true );

		IterDef *triter = findIterDef( IterDef::RevRepeat );
		objMethod->iterDef = triter;
	}

	/* List iterator. */
	{
		UniqueType *anyRefUT = findUniqueType( TYPE_REF, anyLangEl );
		ObjectMethod *objMethod = initFunction( uniqueTypeAny, rootNamespace, globalObjectDef, 
				"list_iter", IN_HALT, IN_HALT, anyRefUT, true );

		IterDef *triter = findIterDef( IterDef::ListEl );
		objMethod->iterDef = triter;
	}

	/* Reverse Value List iterator. */
	{
		UniqueType *anyRefUT = findUniqueType( TYPE_REF, anyLangEl );
		ObjectMethod *objMethod = initFunction( uniqueTypeAny, rootNamespace, globalObjectDef, 
				"rev_list_iter", IN_HALT, IN_HALT, anyRefUT, true );

		IterDef *triter = findIterDef( IterDef::RevListVal );
		objMethod->iterDef = triter;
	}

	/* Map iterator. */
	{
		UniqueType *anyRefUT = findUniqueType( TYPE_REF, anyLangEl );
		ObjectMethod *objMethod = initFunction( uniqueTypeAny, rootNamespace, globalObjectDef, 
				"map_iter", IN_HALT, IN_HALT, anyRefUT, true );

		IterDef *triter = findIterDef( IterDef::MapEl );
		objMethod->iterDef = triter;
	}
}

void Compiler::addMatchLength( ObjectDef *frame, LangEl *lel )
{
	/* Make the type ref. */
	TypeRef *typeRef = TypeRef::cons( internal, uniqueTypeInt );

	/* Create the field and insert it into the map. */
	ObjectField *el = ObjectField::cons( InputLoc(),
			ObjectField::InbuiltFieldType, typeRef, "match_length" );
	el->isConst = true;
	el->inGetR      = IN_GET_MATCH_LENGTH_R;
	el->inGetValR   = IN_GET_MATCH_LENGTH_R;
	frame->rootScope->insertField( el->name, el );
}

void Compiler::addMatchText( ObjectDef *frame, LangEl *lel )
{
	/* Make the type ref. */
	TypeRef *typeRef = TypeRef::cons( internal, uniqueTypeStr );

	/* Create the field and insert it into the map. */
	ObjectField *el = ObjectField::cons( internal,
			ObjectField::InbuiltFieldType, typeRef, "match_text" );
	el->isConst = true;
	el->inGetR      = IN_GET_MATCH_TEXT_R;
	el->inGetValR   = IN_GET_MATCH_TEXT_R;
	frame->rootScope->insertField( el->name, el );
}

void Compiler::addInput( ObjectDef *frame )
{
	/* Make the type ref. */
	TypeRef *typeRef = TypeRef::cons( internal, uniqueTypeStream );

	/* Create the field and insert it into the map. */
	ObjectField *el = ObjectField::cons( internal,
			ObjectField::InbuiltObjectType, typeRef, "input" );
	el->inGetR     = IN_LOAD_INPUT_R;
	el->inGetWV    = IN_LOAD_INPUT_WV;
	el->inGetWC    = IN_LOAD_INPUT_WC;
	el->inGetValR   = IN_LOAD_INPUT_R;
	el->inGetValWC  = IN_LOAD_INPUT_WC;
	el->inGetValWV  = IN_LOAD_INPUT_WV;
	frame->rootScope->insertField( el->name, el );
}

void Compiler::addThis( ObjectDef *frame )
{
	/* Make the type ref. */
	TypeRef *typeRef = TypeRef::cons( internal, uniqueTypeStream );

	/* Create the field and insert it into the map. */
	ObjectField *el = ObjectField::cons( internal,
			ObjectField::InbuiltObjectType, typeRef, "this" );
	el->inGetR     = IN_LOAD_CONTEXT_R;
	el->inGetWV    = IN_LOAD_CONTEXT_WV;
	el->inGetWC    = IN_LOAD_CONTEXT_WC;
	el->inGetValR   = IN_LOAD_CONTEXT_R;
	el->inGetValWC  = IN_LOAD_CONTEXT_WC;
	el->inGetValWV  = IN_LOAD_CONTEXT_WV;
	frame->rootScope->insertField( el->name, el );
}

void Compiler::declareIntFields( )
{
	intObj = ObjectDef::cons( ObjectDef::BuiltinType, "int", nextObjectId++ );
//	intLangEl->objectDef = intObj;

	initFunction( uniqueTypeStr, intObj, "to_string", IN_INT_TO_STR, IN_INT_TO_STR, true );
}

void Compiler::declareStrFields( )
{
	strObj = ObjectDef::cons( ObjectDef::BuiltinType, "str", nextObjectId++ );
	strLangEl->objectDef = strObj;

	initFunction( uniqueTypeInt, strObj, "atoi",
			IN_STR_ATOI,   IN_STR_ATOI, true, true );

	initFunction( uniqueTypeInt, strObj, "atoo",
			IN_STR_ATOO,   IN_STR_ATOO, true, true );

	initFunction( uniqueTypeInt, strObj, "uord8",
			IN_STR_UORD8,  IN_STR_UORD8, true, true );

	initFunction( uniqueTypeInt, strObj, "sord8",
			IN_STR_SORD8,  IN_STR_SORD8, true, true );

	initFunction( uniqueTypeInt, strObj, "uord16",
			IN_STR_UORD16, IN_STR_UORD16, true, true );

	initFunction( uniqueTypeInt, strObj, "sord16",
			IN_STR_SORD16, IN_STR_SORD16, true, true );

	initFunction( uniqueTypeInt, strObj, "uord32",
			IN_STR_UORD32, IN_STR_UORD32, true, true );

	initFunction( uniqueTypeInt, strObj, "sord32",
			IN_STR_SORD32, IN_STR_SORD32, true, true );

	initFunction( uniqueTypeStr, strObj, "prefix",
			IN_STR_PREFIX, IN_STR_PREFIX, uniqueTypeInt, true, true );

	initFunction( uniqueTypeStr, strObj, "suffix",
			IN_STR_SUFFIX, IN_STR_SUFFIX, uniqueTypeInt, true, true );

	initFunction( uniqueTypeStr, rootNamespace, globalObjectDef, "sprintf", 
			IN_SPRINTF, IN_SPRINTF, uniqueTypeStr, uniqueTypeInt, true );

	addLengthField( strObj, IN_STR_LENGTH );
}

void Compiler::declareStreamField( ObjectDef *objDef, code_t getLength )
{
	/* Create the "length" field. */
	TypeRef *typeRef = TypeRef::cons( internal, uniqueTypeStr );
	ObjectField *el = ObjectField::cons( internal,
			ObjectField::InbuiltFieldType, typeRef, "tree" );
	el->isConst = true;
	el->inGetR = IN_GET_COLLECT_STRING;
	el->inGetValR = IN_GET_COLLECT_STRING;

	objDef->rootScope->insertField( el->name, el );
}

void Compiler::declareStreamFields( )
{
	streamObj = streamSel->structDef->objectDef;

	initFunction( uniqueTypeStr, streamObj, "pull",  
			IN_INPUT_PULL_WV, IN_INPUT_PULL_WC, uniqueTypeInt, false );

	initFunction( uniqueTypeStr, streamObj, "push",  
			IN_INPUT_PUSH_WV, IN_INPUT_PUSH_WV, uniqueTypeAny, false );

	initFunction( uniqueTypeStr, streamObj, "push_ignore",  
			IN_INPUT_PUSH_IGNORE_WV, IN_INPUT_PUSH_IGNORE_WV, uniqueTypeAny, false );

	initFunction( uniqueTypeStr, streamObj, "push_stream",  
		IN_INPUT_PUSH_STREAM_WV, IN_INPUT_PUSH_STREAM_WV, uniqueTypeStream, false );

	initFunction( uniqueTypeVoid, streamObj, "close",
			IN_INPUT_CLOSE_WC, IN_INPUT_CLOSE_WC, false );

	declareStreamField( streamObj, 0 );
}

ObjectField *Compiler::makeDataEl()
{
	/* Create the "data" field. */
	TypeRef *typeRef = TypeRef::cons( internal, uniqueTypeStr );
	ObjectField *el = ObjectField::cons( internal,
			ObjectField::InbuiltFieldType, typeRef, "data" );

	el->inGetR = IN_GET_TOKEN_DATA_R;
	el->inSetWC = IN_SET_TOKEN_DATA_WC;
	el->inSetWV = IN_SET_TOKEN_DATA_WV;
	return el;
}

ObjectField *Compiler::makePosEl()
{
	/* Create the "data" field. */
	TypeRef *typeRef = TypeRef::cons( internal, uniqueTypeInt );
	ObjectField *el = ObjectField::cons( internal,
			ObjectField::InbuiltFieldType, typeRef, "pos" );

	el->isConst = true;
	el->inGetR = IN_GET_TOKEN_POS_R;
	el->inGetValR = IN_GET_TOKEN_POS_R;
	return el;
}

ObjectField *Compiler::makeLineEl()
{
	/* Create the "data" field. */
	TypeRef *typeRef = TypeRef::cons( internal, uniqueTypeInt );
	ObjectField *el = ObjectField::cons( internal,
			ObjectField::InbuiltFieldType, typeRef, "line" );

	el->isConst = true;
	el->inGetR = IN_GET_TOKEN_LINE_R;
	el->inGetValR = IN_GET_TOKEN_LINE_R;
	return el;
}

/* Add a constant length field to the object. 
 * Opcode supplied by the caller. */
void Compiler::addLengthField( ObjectDef *objDef, code_t getLength )
{
	/* Create the "length" field. */
	TypeRef *typeRef = TypeRef::cons( internal, uniqueTypeInt );
	ObjectField *el = ObjectField::cons( internal,
			ObjectField::InbuiltFieldType, typeRef, "length" );
	el->isConst = true;
	el->inGetR = getLength;
	el->inGetValR = getLength;

	objDef->rootScope->insertField( el->name, el );
}

void Compiler::declareTokenFields( )
{
	/* Give all user terminals the token object type. */
	for ( LelList::Iter lel = langEls; lel.lte(); lel++ ) {
		if ( lel->type == LangEl::Term ) {
			if ( lel->objectDef != 0 ) {
				/* Create the "data" field. */
				ObjectField *dataEl = makeDataEl();
				lel->objectDef->rootScope->insertField( dataEl->name, dataEl );

				/* Create the "pos" field. */
				ObjectField *posEl = makePosEl();
				lel->objectDef->rootScope->insertField( posEl->name, posEl );

				/* Create the "line" field. */
				ObjectField *lineEl = makeLineEl();
				lel->objectDef->rootScope->insertField( lineEl->name, lineEl );
			}
		}
	}
}

void Compiler::declareGlobalFields()
{
	ObjectMethod *method;

	method = initFunction( uniqueTypeStream, rootNamespace, globalObjectDef, "open",
		IN_OPEN_FILE, IN_OPEN_FILE, uniqueTypeStr, uniqueTypeStr, true );
	method->useCallObj = false;

	method = initFunction( uniqueTypeStr, rootNamespace, globalObjectDef, "tolower",
		IN_TO_LOWER, IN_TO_LOWER, uniqueTypeStr, true );
	method->useCallObj = false;

	method = initFunction( uniqueTypeStr, rootNamespace, globalObjectDef, "toupper",
		IN_TO_UPPER, IN_TO_UPPER, uniqueTypeStr, true );
	method->useCallObj = false;

	method = initFunction( uniqueTypeInt, rootNamespace, globalObjectDef, "atoi",
			IN_STR_ATOI,   IN_STR_ATOI, uniqueTypeStr, true, true );
	method->useCallObj = false;

	method = initFunction( uniqueTypeInt, rootNamespace, globalObjectDef, "atoo",
			IN_STR_ATOO,   IN_STR_ATOO, uniqueTypeStr, true, true );
	method->useCallObj = false;

	method = initFunction( uniqueTypeStr, rootNamespace, globalObjectDef, "prefix",
			IN_PREFIX, IN_PREFIX, uniqueTypeStr, uniqueTypeInt, true, true );
	method->useCallObj = false;

	method = initFunction( uniqueTypeStr, rootNamespace, globalObjectDef, "suffix",
			IN_SUFFIX, IN_SUFFIX, uniqueTypeStr, uniqueTypeInt, true, true );
	method->useCallObj = false;


	method = initFunction( uniqueTypeInt, rootNamespace, globalObjectDef, "uord8",
			IN_STR_UORD8,  IN_STR_UORD8, uniqueTypeStr, true, true );
	method->useCallObj = false;

	method = initFunction( uniqueTypeInt, rootNamespace, globalObjectDef, "sord8",
			IN_STR_SORD8,  IN_STR_SORD8, uniqueTypeStr, true, true );
	method->useCallObj = false;

	method = initFunction( uniqueTypeInt, rootNamespace, globalObjectDef, "uord16",
			IN_STR_UORD16, IN_STR_UORD16, uniqueTypeStr, true, true );
	method->useCallObj = false;

	method = initFunction( uniqueTypeInt, rootNamespace, globalObjectDef, "sord16",
			IN_STR_SORD16, IN_STR_SORD16, uniqueTypeStr, true, true );
	method->useCallObj = false;

	method = initFunction( uniqueTypeInt, rootNamespace, globalObjectDef, "uord32",
			IN_STR_UORD32, IN_STR_UORD32, uniqueTypeStr, true, true );
	method->useCallObj = false;

	method = initFunction( uniqueTypeInt, rootNamespace, globalObjectDef, "sord32",
			IN_STR_SORD32, IN_STR_SORD32, uniqueTypeStr, true, true );
	method->useCallObj = false;

	method = initFunction( uniqueTypeInt, rootNamespace, globalObjectDef, "exit",
		IN_EXIT, IN_EXIT, uniqueTypeInt, true, true );

	method = initFunction( uniqueTypeInt, rootNamespace, globalObjectDef, "exit_hard",
		IN_EXIT_HARD, IN_EXIT_HARD, uniqueTypeInt, true, true );

	method = initFunction( uniqueTypeInt, rootNamespace, globalObjectDef, "system",
		IN_SYSTEM, IN_SYSTEM, uniqueTypeStr, true );

	addStdin();
	addStdout();
	addStderr();
	addArgv();
	addError();
}

void Compiler::addStdin()
{
	/* Make the type ref. */
	TypeRef *typeRef = TypeRef::cons( internal, uniqueTypeStream );

	/* Create the field and insert it into the map. */
	ObjectField *el = ObjectField::cons( internal,
			ObjectField::InbuiltFieldType, typeRef, "stdin" );
	el->isConst = true;
	el->inGetR     = IN_GET_STDIN;
	el->inGetWC    = IN_GET_STDIN;
	el->inGetWV    = IN_GET_STDIN;
	el->inGetValR   = IN_GET_STDIN;
	el->inGetValWC  = IN_GET_STDIN;
	el->inGetValWV  = IN_GET_STDIN;
	rootNamespace->rootScope->insertField( el->name, el );
}

void Compiler::addStdout()
{
	/* Make the type ref. */
	TypeRef *typeRef = TypeRef::cons( internal, uniqueTypeStream );

	/* Create the field and insert it into the map. */
	ObjectField *el = ObjectField::cons( internal,
			ObjectField::InbuiltFieldType, typeRef, "stdout" );
	el->isConst = true;
	el->inGetR     = IN_GET_STDOUT;
	el->inGetWC    = IN_GET_STDOUT;
	el->inGetWV    = IN_GET_STDOUT;

	el->inGetValR   = IN_GET_STDOUT;
	el->inGetValWC  = IN_GET_STDOUT;
	el->inGetValWV  = IN_GET_STDOUT;
	rootNamespace->rootScope->insertField( el->name, el );
}

void Compiler::addStderr()
{
	/* Make the type ref. */
	TypeRef *typeRef = TypeRef::cons( internal, uniqueTypeStream );

	/* Create the field and insert it into the map. */
	ObjectField *el = ObjectField::cons( internal,
			ObjectField::InbuiltFieldType, typeRef, "stderr" );
	el->isConst = true;
	el->inGetR     = IN_GET_STDERR;
	el->inGetWC    = IN_GET_STDERR;
	el->inGetWV    = IN_GET_STDERR;

	el->inGetValR   = IN_GET_STDERR;
	el->inGetValWC  = IN_GET_STDERR;
	el->inGetValWV  = IN_GET_STDERR;
	rootNamespace->rootScope->insertField( el->name, el );
}

void Compiler::addArgv()
{
	/* Create the field and insert it into the map. */
	ObjectField *el = ObjectField::cons( internal,
			ObjectField::StructFieldType, argvTypeRef, "argv" );
	el->isConst = true;
	rootNamespace->rootScope->insertField( el->name, el );
	argv = el;

	TypeRef *typeRef = TypeRef::cons( internal, uniqueTypeStr );

	el = ObjectField::cons( internal,
			ObjectField::StructFieldType, typeRef, "arg0" );
	el->isConst = true;
	rootNamespace->rootScope->insertField( el->name, el );
	arg0 = el;
}

void Compiler::addError()
{
	/* Make the type ref. */
	TypeRef *typeRef = TypeRef::cons( internal, uniqueTypeStr );

	/* Create the field and insert it into the map. */
	ObjectField *el = ObjectField::cons( internal,
			ObjectField::InbuiltFieldType, typeRef, "error" );
	el->isConst = true;
	el->inGetR     = IN_GET_ERROR;
	el->inGetWC    = IN_GET_ERROR;
	el->inGetWV    = IN_GET_ERROR;
	rootNamespace->rootScope->insertField( el->name, el );
}

void Compiler::initMapFunctions( GenericType *gen )
{
	/* Value functions. */
	initFunction( gen->valueUt, gen->objDef, "find", 
			IN_VMAP_FIND,      IN_VMAP_FIND, gen->keyUt, true, true, gen );

	initFunction( uniqueTypeInt, gen->objDef, "insert", 
			IN_VMAP_INSERT_WV, IN_VMAP_INSERT_WC, gen->keyUt, gen->valueUt,
			false, true, gen );

	initFunction( gen->elUt, gen->objDef, "remove", 
			IN_VMAP_REMOVE_WV, IN_VMAP_REMOVE_WC, gen->keyUt, false, true, gen );

	/*
	 * Element Functions
	 */
	initFunction( gen->elUt, gen->objDef, "find_el", 
			IN_MAP_FIND,      IN_MAP_FIND, gen->keyUt, true, true, gen );

	initFunction( uniqueTypeInt, gen->objDef, "insert_el", 
			IN_MAP_INSERT_WV, IN_MAP_INSERT_WC, gen->elUt, false, true, gen );

	initFunction( gen->elUt, gen->objDef, "detach_el", 
			IN_MAP_DETACH_WV, IN_MAP_DETACH_WC, gen->elUt, false, true, gen );
}

void Compiler::initMapField( GenericType *gen, const char *name, int offset )
{
	/* Make the type ref and create the field. */
	ObjectField *el = ObjectField::cons( internal,
			ObjectField::InbuiltOffType, gen->elTr, name );

	el->inGetR =  IN_GET_MAP_MEM_R;
	el->inGetWC = IN_GET_MAP_MEM_WC;
	el->inGetWV = IN_GET_MAP_MEM_WV;
//	el->inSetWC = IN_SET_MAP_MEM_WC;
//	el->inSetWV = IN_SET_MAP_MEM_WV;

	el->inGetValR  =  IN_GET_MAP_MEM_R;
	el->inGetValWC =  IN_GET_MAP_MEM_WC;
	el->inGetValWV =  IN_GET_MAP_MEM_WV;

	gen->objDef->rootScope->insertField( el->name, el );

	el->useGenericId = true;
	el->generic = gen;

	/* Zero for head, One for tail. */
	el->offset = offset;
}

void Compiler::initMapFields( GenericType *gen )
{
	addLengthField( gen->objDef, IN_MAP_LENGTH );

	initMapField( gen, "head_el", 0 );
	initMapField( gen, "tail_el", 1 );

	initMapElKey( gen, "key", 0 );

	initMapElField( gen, "prev", 0 );
	initMapElField( gen, "next", 1 );
}

void Compiler::initMapElKey( GenericType *gen, const char *name, int offset )
{
	/* Make the type ref and create the field. */
	ObjectField *el = ObjectField::cons( internal,
			ObjectField::GenericDependentType, gen->keyTr, name );
	
	gen->el->mapKeyField = el;
	
	/* Offset will be computed when the offset of the owning map element field
	 * is computed. */

	gen->elUt->structEl->structDef->objectDef->rootScope->insertField( el->name, el );
}

void Compiler::initMapElField( GenericType *gen, const char *name, int offset )
{
	/* Make the type ref and create the field. */
	ObjectField *el = ObjectField::cons( internal,
			ObjectField::InbuiltOffType, gen->elTr, name );

	el->inGetR    = IN_GET_MAP_EL_MEM_R;
	el->inGetValR = IN_GET_MAP_EL_MEM_R;
//	el->inGetWC = IN_GET_LIST2EL_MEM_WC;
//	el->inGetWV = IN_GET_LIST2EL_MEM_WV;
//	el->inSetWC = IN_SET_LIST2EL_MEM_WC;
//	el->inSetWV = IN_SET_LIST2EL_MEM_WV;

	el->useGenericId = true;
	el->generic = gen;

	/* Zero for head, One for tail. */
	el->offset = offset;

	gen->elUt->structEl->structDef->objectDef->rootScope->insertField( el->name, el );
}

void Compiler::initListFunctions( GenericType *gen )
{
	initFunction( uniqueTypeInt, gen->objDef, "push_head", 
			IN_VLIST_PUSH_HEAD_WV, IN_VLIST_PUSH_HEAD_WC, gen->valueUt, false, true, gen );

	initFunction( uniqueTypeInt, gen->objDef, "push_tail", 
			IN_VLIST_PUSH_TAIL_WV, IN_VLIST_PUSH_TAIL_WC, gen->valueUt, false, true, gen );

	initFunction( uniqueTypeInt, gen->objDef, "push", 
			IN_VLIST_PUSH_HEAD_WV, IN_VLIST_PUSH_HEAD_WC, gen->valueUt, false, true, gen );

	initFunction( gen->valueUt, gen->objDef, "pop_head", 
			IN_VLIST_POP_HEAD_WV, IN_VLIST_POP_HEAD_WC, false, true, gen );

	initFunction( gen->valueUt, gen->objDef, "pop_tail", 
			IN_VLIST_POP_TAIL_WV, IN_VLIST_POP_TAIL_WC, false, true, gen );

	initFunction( gen->valueUt, gen->objDef, "pop", 
			IN_VLIST_POP_HEAD_WV, IN_VLIST_POP_HEAD_WC, false, true, gen );

	initFunction( uniqueTypeInt, gen->objDef, "push_head_el", 
			IN_LIST_PUSH_HEAD_WV, IN_LIST_PUSH_HEAD_WC, gen->elUt, false, true, gen );

	initFunction( uniqueTypeInt, gen->objDef, "push_tail_el", 
			IN_LIST_PUSH_TAIL_WV, IN_LIST_PUSH_TAIL_WC, gen->elUt, false, true, gen );

	initFunction( uniqueTypeInt, gen->objDef, "push_el", 
			IN_LIST_PUSH_HEAD_WV, IN_LIST_PUSH_HEAD_WC, gen->elUt, false, true, gen );

	initFunction( gen->elUt, gen->objDef, "pop_head_el", 
			IN_LIST_POP_HEAD_WV, IN_LIST_POP_HEAD_WC, false, true, gen );

	initFunction( gen->elUt, gen->objDef, "pop_tail_el", 
			IN_LIST_POP_TAIL_WV, IN_LIST_POP_TAIL_WC, false, true, gen );

	initFunction( gen->elUt, gen->objDef, "pop_el", 
			IN_LIST_POP_HEAD_WV, IN_LIST_POP_HEAD_WC, false, true, gen );
}

void Compiler::initListElField( GenericType *gen, const char *name, int offset )
{
	/* Make the type ref and create the field. */
	ObjectField *el = ObjectField::cons( internal,
			ObjectField::InbuiltOffType, gen->elTr, name );

	el->inGetR    = IN_GET_LIST_EL_MEM_R;
	el->inGetValR = IN_GET_LIST_EL_MEM_R;
//	el->inGetWC = IN_GET_LIST2EL_MEM_WC;
//	el->inGetWV = IN_GET_LIST2EL_MEM_WV;
//	el->inSetWC = IN_SET_LIST2EL_MEM_WC;
//	el->inSetWV = IN_SET_LIST2EL_MEM_WV;

	el->useGenericId = true;
	el->generic = gen;

	/* Zero for head, One for tail. */
	el->offset = offset;

	gen->elUt->structEl->structDef->objectDef->rootScope->insertField( el->name, el );
}

void Compiler::initListFieldEl( GenericType *gen, const char *name, int offset )
{
	/* Make the type ref and create the field. */
	ObjectField *el = ObjectField::cons( internal,
			ObjectField::InbuiltOffType, gen->elTr, name );

	el->inGetR =  IN_GET_LIST_MEM_R;
	el->inGetWC = IN_GET_LIST_MEM_WC;
	el->inGetWV = IN_GET_LIST_MEM_WV;
//	el->inSetWC = IN_SET_LIST_MEM_WC;
//	el->inSetWV = IN_SET_LIST_MEM_WV;

	el->inGetValR  =  IN_GET_LIST_MEM_R;
	el->inGetValWC =  IN_GET_LIST_MEM_WC;
	el->inGetValWV =  IN_GET_LIST_MEM_WV;

	gen->objDef->rootScope->insertField( el->name, el );

	el->useGenericId = true;
	el->generic = gen;

	/* Zero for head, One for tail. */
	el->offset = offset;
}

void Compiler::initListFieldVal( GenericType *gen, const char *name, int offset )
{
	/* Make the type ref and create the field. */
	ObjectField *el = ObjectField::cons( internal,
			ObjectField::InbuiltOffType, gen->valueTr, name );

	el->inGetR =  IN_GET_VLIST_MEM_R;
	el->inGetWC = IN_GET_VLIST_MEM_WC;
	el->inGetWV = IN_GET_VLIST_MEM_WV;
//	el->inSetWC = IN_SET_VLIST_MEM_WC;
//	el->inSetWV = IN_SET_VLIST_MEM_WV;

	el->inGetValR  =  IN_GET_VLIST_MEM_R;
	el->inGetValWC =  IN_GET_VLIST_MEM_WC;
	el->inGetValWV =  IN_GET_VLIST_MEM_WV;

	gen->objDef->rootScope->insertField( el->name, el );

	el->useGenericId = true;
	el->generic = gen;

	/* Zero for head, One for tail. */
	el->offset = offset;
}

void Compiler::initListFields( GenericType *gen )
{
	/* The value fields. */
	initListFieldVal( gen, "head", 0 );
	initListFieldVal( gen, "tail", 1 );
	initListFieldVal( gen, "top", 0 );

	/* The element fields. */
	initListFieldEl( gen, "head_el", 0 );
	initListFieldEl( gen, "tail_el", 1 );
	initListFieldEl( gen, "top_el", 0 );

	addLengthField( gen->objDef, IN_LIST_LENGTH );

	/* The fields of the list element. */
	initListElField( gen, "prev", 0 );
	initListElField( gen, "next", 1 );
}

void Compiler::initParserFunctions( GenericType *gen )
{
	initFunction( gen->elUt, gen->objDef, "finish",
			IN_PARSE_FINISH_WV, IN_PARSE_FINISH_WC, true );

	initFunction( gen->elUt, gen->objDef, "eof",
			IN_PARSE_FINISH_WV, IN_PARSE_FINISH_WC, true );
}

void Compiler::initParserField( GenericType *gen, const char *name,
		int offset, TypeRef *typeRef )
{
	/* Make the type ref and create the field. */
	ObjectField *el = ObjectField::cons( internal,
			ObjectField::InbuiltOffType, typeRef, name );

	el->inGetR =  IN_GET_PARSER_MEM_R;
	el->inGetWC = IN_GET_PARSER_MEM_WC;
	el->inGetWV = IN_GET_PARSER_MEM_WV;
	el->inSetWC = IN_SET_PARSER_MEM_WC;
	el->inSetWV = IN_SET_PARSER_MEM_WV;

	gen->objDef->rootScope->insertField( el->name, el );

	/* Zero for head, One for tail. */
	el->offset = offset;
}

void Compiler::initParserFields( GenericType *gen )
{
	TypeRef *typeRef;

	typeRef = TypeRef::cons( internal, gen->elUt );
	initParserField( gen, "tree", 0, typeRef );

	typeRef = TypeRef::cons( internal, uniqueTypeStr );
	initParserField( gen, "error", 1, typeRef );
}

void Compiler::makeFuncVisible( Function *func, bool isUserIter )
{
	func->localFrame = func->codeBlock->localFrame;

	/* Set up the parameters. */
	for ( ParameterList::Iter param = *func->paramList; param.lte(); param++ ) {
		if ( func->localFrame->rootScope->findField( param->name ) != 0 )
			error(param->loc) << "parameter " << param->name << " redeclared" << endp;

		func->localFrame->rootScope->insertField( param->name, param );
	}

	/* Insert the function into the global function map. */
	ObjectMethod *objMethod = new ObjectMethod( func->typeRef, func->name, 
			IN_CALL_WV, IN_CALL_WC, 
			func->paramList->length(), 0, func->paramList, false );
	objMethod->funcId = func->funcId;
	objMethod->useFuncId = true;
	objMethod->useCallObj = false;
	objMethod->func = func;

	if ( isUserIter ) {
		IterDef *uiter = findIterDef( IterDef::User, func );
		objMethod->iterDef = uiter;
	}

	NameScope *scope = func->nspace->rootScope; // : globalObjectDef->rootScope;

	if ( !scope->methodMap.insert( func->name, objMethod ) )
		error(func->typeRef->loc) << "function " << func->name << " redeclared" << endp;

	func->objMethod = objMethod;
}

void Compiler::makeInHostVisible( Function *func )
{
	/* Set up the parameters. */
	for ( ParameterList::Iter param = *func->paramList; param.lte(); param++ ) {
		if ( func->localFrame->rootScope->findField( param->name ) != 0 )
			error(param->loc) << "parameter " << param->name << " redeclared" << endp;

		func->localFrame->rootScope->insertField( param->name, param );
	}

	/* Insert the function into the global function map. */
	ObjectMethod *objMethod = new ObjectMethod( func->typeRef, func->name, 
			IN_HOST, IN_HOST, 
			func->paramList->length(), 0, func->paramList, false );
	objMethod->funcId = func->funcId;
	objMethod->useFuncId = true;
	objMethod->useCallObj = false;
	objMethod->func = func;

	NameScope *scope = func->nspace->rootScope;

	if ( !scope->methodMap.insert( func->name, objMethod ) ) {
		error(func->typeRef->loc) << "in-host function " << func->name <<
				" redeclared" << endp;
	}

	func->objMethod = objMethod;
}

/*
 * Type Declaration Root.
 */
void Compiler::declarePass()
{
	declareReVars();

	makeDefaultIterators();

	for ( FunctionList::Iter f = functionList; f.lte(); f++ )
		makeFuncVisible( f, f->isUserIter );

	for ( FunctionList::Iter f = inHostList; f.lte(); f++ )
		makeInHostVisible( f );

	rootNamespace->declare( this );

	/* Will fill in zero lels that were not declared. */
	makeIgnoreCollectors();

	declareByteCode();

	declareIntFields();
	declareStrFields();
	declareStreamFields();
	declareTokenFields();
	declareGlobalFields();

	/* Fill any empty scanners with a default token. */
	initEmptyScanners();
}
