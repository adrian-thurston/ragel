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
#include "parsedata.h"
#include <iostream>
#include <assert.h>

void Compiler::initUniqueTypes( )
{
	uniqueTypeNil = new UniqueType( TYPE_NIL );
	uniqueTypeVoid = new UniqueType( TYPE_TREE, voidLangEl );
	uniqueTypePtr = new UniqueType( TYPE_TREE, ptrLangEl );
	uniqueTypeBool = new UniqueType( TYPE_TREE, boolLangEl );
	uniqueTypeInt = new UniqueType( TYPE_TREE, intLangEl );
	uniqueTypeStr = new UniqueType( TYPE_TREE, strLangEl );
	uniqueTypeStream = new UniqueType( TYPE_TREE, streamLangEl );
	uniqueTypeIgnore = new UniqueType( TYPE_TREE, ignoreLangEl );
	uniqueTypeAny = new UniqueType( TYPE_TREE, anyLangEl );

	uniqeTypeMap.insert( uniqueTypeNil );
	uniqeTypeMap.insert( uniqueTypeVoid );
	uniqeTypeMap.insert( uniqueTypePtr );
	uniqeTypeMap.insert( uniqueTypeBool );
	uniqeTypeMap.insert( uniqueTypeInt );
	uniqeTypeMap.insert( uniqueTypeStr );
	uniqeTypeMap.insert( uniqueTypeStream );
	uniqeTypeMap.insert( uniqueTypeIgnore );
	uniqeTypeMap.insert( uniqueTypeAny );
}

ObjectField *ObjNameScope::checkRedecl( const String &name )
{
	return owner->checkRedecl( this, name );
}

void ObjNameScope::insertField( const String &name, ObjectField *value )
{
	return owner->insertField( this, name, value );
}

ObjectField *ObjectDef::checkRedecl( ObjNameScope *inScope, const String &name )
{
	ObjFieldMapEl *objDefMapEl = inScope->objFieldMap->find( name );
	if ( objDefMapEl != 0 )
		return objDefMapEl->value;
	return 0;
}

void ObjectDef::insertField( ObjNameScope *inScope, const String &name, ObjectField *value )
{
	inScope->objFieldMap->insert( name, value );
	objFieldList->append( value );
	value->scope = inScope;
}

ObjNameScope *ObjectDef::pushScope( ObjNameScope *curScope )
{
	ObjNameScope *newScope = new ObjNameScope;
	newScope->objFieldMap = new ObjFieldMap;

	newScope->owner = this;
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

LangEl *declareLangEl( Compiler *pd, Namespace *nspace, const String &data, LangEl::Type type )
{
    /* If the id is already in the dict, it will be placed in last found. If
     * it is not there then it will be inserted and last found will be set to it. */
	TypeMapEl *inDict = nspace->typeMap.find( data );
	if ( inDict != 0 )
		error() << "'" << data << "' already defined as something else" << endp;

	/* Language element not there. Make the new lang el and insert.. */
	LangEl *langEl = new LangEl( nspace, data, type );
	TypeMapEl *typeMapEl = new TypeMapEl( data, langEl );
	nspace->typeMap.insert( typeMapEl );
	pd->langEls.append( langEl );

	return langEl;
}

/* Does not map the new language element. */
LangEl *addLangEl( Compiler *pd, Namespace *nspace, const String &data, LangEl::Type type )
{
	LangEl *langEl = new LangEl( nspace, data, type );
	pd->langEls.append( langEl );
	return langEl;
}

void declareTypeAlias( Compiler *pd, Namespace *nspace, const String &data, TypeRef *typeRef )
{
    /* If the id is already in the dict, it will be placed in last found. If
     * it is not there then it will be inserted and last found will be set to it. */
	TypeMapEl *inDict = nspace->typeMap.find( data );
	if ( inDict != 0 )
		error() << "'" << data << "' already defined as something else" << endp;

	/* Language element not there. Make the new lang el and insert.. */
	TypeMapEl *typeMapEl = new TypeMapEl( data, typeRef );
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
	boolLangEl = declareLangEl( this, rootNamespace, "bool", LangEl::Term );
	intLangEl = declareLangEl( this, rootNamespace, "int", LangEl::Term );
	strLangEl = declareLangEl( this, rootNamespace, "str", LangEl::Term );
	streamLangEl = declareLangEl( this, rootNamespace, "stream", LangEl::Term );
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
	ObjectField *el = ObjectField::cons( internal, typeRef, "lhs" );

	el->isLhsEl = true;

	initLocalInstructions( el );

	localFrame->rootScope->insertField( el->name, el );
}

void Compiler::addProdLHSLoad( Production *prod, CodeVect &code, long &insertPos )
{
	ObjNameScope *scope = prod->redBlock->localFrame->rootScope;
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
	ObjNameScope *scope = block->localFrame->rootScope;
	ObjectField *lhsField = scope->findField("lhs");
	assert( lhsField != 0 );

	if ( lhsField->beenReferenced ) {
		code.append( IN_STORE_LHS_EL );
		code.appendHalf( lhsField->offset );
	}
}

void Compiler::addProdRHSVars( ObjectDef *localFrame, ProdElList *prodElList )
{
	long position = 1;
	for ( ProdElList::Iter rhsEl = *prodElList; rhsEl.lte(); rhsEl++, position++ ) {
		if ( rhsEl->type == ProdEl::ReferenceType ) {
			/* Use an offset of zero. For frame objects we compute the offset on
			 * demand. */
			String name( 8, "r%d", position );
			ObjectField *el = ObjectField::cons( InputLoc(), rhsEl->typeRef, name );
			rhsEl->rhsElField = el;

			/* Right hand side elements are constant. */
			el->isConst = true;
			el->isRhsEl = true;

			/* Only ever fetch for reading since they are constant. */
			el->inGetR = IN_GET_LOCAL_R;

			localFrame->rootScope->insertField( el->name, el );
		}
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

void GenericType::declare( Compiler *pd, Namespace *nspace )
{
	//std::cout << "generic " << g->name << std::endl;

	LangEl *langEl = declareLangEl( pd, nspace, name, LangEl::NonTerm );

	/* Add one empty production. */
	ProdElList *emptyList = new ProdElList;
	//addProduction( g->loc, langEl, emptyList, false, 0, 0 );

	{
		LangEl *prodName = langEl;
		assert( prodName->type == LangEl::NonTerm );

		Production *newDef = Production::cons( InputLoc(), prodName, 
			emptyList, String(), false, 0,
			pd->prodList.length(), prodName->defList.length() );
			
		prodName->defList.append( newDef );
		pd->prodList.append( newDef );
		newDef->predOf = 0;
	}

	langEl->generic = this;
	this->langEl = langEl;

	utArg = typeArg->uniqueType;
 
	if ( typeId == GEN_MAP )
		keyUT = keyTypeArg->uniqueType; 

	objDef = ObjectDef::cons( ObjectDef::BuiltinType, 
			name, pd->nextObjectId++ );

	switch ( typeId ) {
		case GEN_MAP: 
			pd->initMapFunctions( this );
			break;
		case GEN_LIST:
			pd->initListFunctions( this );
			pd->initListFields( this );
			break;
		case GEN_VECTOR:
			pd->initVectorFunctions( this );
			break;
		case GEN_PARSER:
			/* Need to generate a parser for the type. */
			utArg->langEl->parserId = pd->nextParserId++;
			pd->initParserFunctions( this );
			pd->initParserFields( this );
			break;
	}

	langEl->objectDef = objDef;
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

	for ( ContextDefList::Iter c = contextDefList; c.lte(); c++ ) {
		LangEl *lel = declareLangEl( pd, this, c->name, LangEl::NonTerm );
		ProdElList *emptyList = new ProdElList;
		//addProduction( c->context->loc, c->name, emptyList, false, 0, 0 );

		{
			LangEl *prodName = lel;
			assert( prodName->type == LangEl::NonTerm );

			Production *newDef = Production::cons( loc, prodName, 
				emptyList, String(), false, 0,
				pd->prodList.length(), prodName->defList.length() );
			
			prodName->defList.append( newDef );
			pd->prodList.append( newDef );
			newDef->predOf = 0;

			/* If the token has the same name as the region it is in, then also
			 * insert it into the symbol map for the parent region. */
			if ( strcmp( c->name, this->name ) == 0 ) {
				/* Insert the name into the top of the region stack after popping the
				 * region just created. We need it in the parent. */
				TypeMapEl *typeMapEl = new TypeMapEl( c->name, prodName );
				this->parentNamespace->typeMap.insert( typeMapEl );
			}
		}

		c->context->lel = lel;
		lel->contextDef = c->context;
		lel->objectDef = c->context->contextObjDef;
	}

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
	/* The iterator name. */
	LangVarRef *callVarRef = LangVarRef::cons( loc, context, scope, "triter" );

	/* The parameters. */
	CallArgVect *callExprVect = new CallArgVect;
	callExprVect->append( new CallArg( iterCall->langExpr ) );
	iterCall->langTerm = LangTerm::cons( InputLoc(), callVarRef, callExprVect );
	iterCall->langExpr = 0;
	iterCall->form = IterCall::IterCallForm;
}


void LangStmt::declareForIter( Compiler *pd ) const
{
	if ( iterCall->form != IterCall::IterCallForm )
		chooseDefaultIter( pd, iterCall );

	objField->typeRef = TypeRef::cons( loc, typeRef, iterCall );
}

void LangStmt::declare( Compiler *pd ) const
{
	switch ( type ) {
		case PrintType: 
			break;
		case PrintXMLACType:
			break;
		case PrintXMLType:
			break;
		case PrintStreamType:
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
	addCtx( block->localFrame );

	block->declare( this );
}

void Compiler::declarePreEof( TokenRegion *region )
{
	CodeBlock *block = region->preEofBlock;

	addInput( block->localFrame );
	addCtx( block->localFrame );

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
		ObjMethod *objMethod = initFunction( uniqueTypeAny, globalObjectDef, 
				"triter", IN_HALT, IN_HALT, anyRefUT, true );

		IterDef *triter = findIterDef( IterDef::Tree );
		objMethod->iterDef = triter;
	}

	/* Child iterator. */
	{
		UniqueType *anyRefUT = findUniqueType( TYPE_REF, anyLangEl );
		ObjMethod *objMethod = initFunction( uniqueTypeAny, globalObjectDef, 
				"child", IN_HALT, IN_HALT, anyRefUT, true );

		IterDef *triter = findIterDef( IterDef::Child );
		objMethod->iterDef = triter;
	}

	/* Reverse iterator. */
	{
		UniqueType *anyRefUT = findUniqueType( TYPE_REF, anyLangEl );
		ObjMethod *objMethod = initFunction( uniqueTypeAny, globalObjectDef, 
				"rev_child", IN_HALT, IN_HALT, anyRefUT, true );

		IterDef *triter = findIterDef( IterDef::RevChild );
		objMethod->iterDef = triter;
	}

	/* Repeat iterator. */
	{
		UniqueType *anyRefUT = findUniqueType( TYPE_REF, anyLangEl );
		ObjMethod *objMethod = initFunction( uniqueTypeAny, globalObjectDef, 
				"repeat", IN_HALT, IN_HALT, anyRefUT, true );

		IterDef *triter = findIterDef( IterDef::Repeat );
		objMethod->iterDef = triter;
	}

	/* Reverse repeat iterator. */
	{
		UniqueType *anyRefUT = findUniqueType( TYPE_REF, anyLangEl );
		ObjMethod *objMethod = initFunction( uniqueTypeAny, globalObjectDef, 
				"rev_repeat", IN_HALT, IN_HALT, anyRefUT, true );

		IterDef *triter = findIterDef( IterDef::RevRepeat );
		objMethod->iterDef = triter;
	}
}

void Compiler::addMatchLength( ObjectDef *frame, LangEl *lel )
{
	/* Make the type ref. */
	TypeRef *typeRef = TypeRef::cons( internal, uniqueTypeInt );

	/* Create the field and insert it into the map. */
	ObjectField *el = ObjectField::cons( InputLoc(), typeRef, "match_length" );
	el->beenReferenced = true;
	el->beenInitialized = true;
	el->isConst = true;
	el->useOffset = false;
	el->inGetR    = IN_GET_MATCH_LENGTH_R;
	frame->rootScope->insertField( el->name, el );
}

void Compiler::addMatchText( ObjectDef *frame, LangEl *lel )
{
	/* Make the type ref. */
	TypeRef *typeRef = TypeRef::cons( internal, uniqueTypeStr );

	/* Create the field and insert it into the map. */
	ObjectField *el = ObjectField::cons( internal, typeRef, "match_text" );
	el->beenReferenced = true;
	el->beenInitialized = true;
	el->isConst = true;
	el->useOffset = false;
	el->inGetR    = IN_GET_MATCH_TEXT_R;
	frame->rootScope->insertField( el->name, el );
}

void Compiler::addInput( ObjectDef *frame )
{
	/* Make the type ref. */
	TypeRef *typeRef = TypeRef::cons( internal, uniqueTypeStream );

	/* Create the field and insert it into the map. */
	ObjectField *el = ObjectField::cons( internal, typeRef, "input" );
	el->beenReferenced = true;
	el->beenInitialized = true;
	el->isConst   = false;
	el->useOffset = false;
	el->isCustom  = true;
	el->inGetR    = IN_LOAD_INPUT_R;
	el->inGetWV   = IN_LOAD_INPUT_WV;
	el->inGetWC   = IN_LOAD_INPUT_WC;
	frame->rootScope->insertField( el->name, el );
}

void Compiler::addCtx( ObjectDef *frame )
{
	/* Make the type ref. */
	TypeRef *typeRef = TypeRef::cons( internal, uniqueTypeStream );

	/* Create the field and insert it into the map. */
	ObjectField *el = ObjectField::cons( internal, typeRef, "ctx" );
	el->beenReferenced = true;
	el->beenInitialized = true;
	el->isConst   = false;
	el->useOffset = false;
	el->isCustom  = true;
	el->inGetR    = IN_LOAD_CTX_R;
	el->inGetWV   = IN_LOAD_CTX_WV;
	el->inGetWC   = IN_LOAD_CTX_WC;
	frame->rootScope->insertField( el->name, el );
}

void Compiler::initIntObject( )
{
	intObj = ObjectDef::cons( ObjectDef::BuiltinType, "int", nextObjectId++ );
	intLangEl->objectDef = intObj;

	initFunction( uniqueTypeStr, intObj, "to_string", IN_INT_TO_STR, IN_INT_TO_STR, true );
}

void Compiler::initStrObject( )
{
	strObj = ObjectDef::cons( ObjectDef::BuiltinType, "str", nextObjectId++ );
	strLangEl->objectDef = strObj;

	initFunction( uniqueTypeInt, strObj, "atoi",   IN_STR_ATOI, IN_STR_ATOI, true );
	initFunction( uniqueTypeInt, strObj, "uord8",  IN_STR_UORD8,  IN_STR_UORD8, true );
	initFunction( uniqueTypeInt, strObj, "sord8",  IN_STR_SORD8,  IN_STR_SORD8, true );
	initFunction( uniqueTypeInt, strObj, "uord16", IN_STR_UORD16, IN_STR_UORD16, true );
	initFunction( uniqueTypeInt, strObj, "sord16", IN_STR_SORD16, IN_STR_SORD16, true );
	initFunction( uniqueTypeInt, strObj, "uord32", IN_STR_UORD32, IN_STR_UORD32, true );
	initFunction( uniqueTypeInt, strObj, "sord32", IN_STR_SORD32, IN_STR_SORD32, true );
	addLengthField( strObj, IN_STR_LENGTH );

	initFunction( uniqueTypeStr, globalObjectDef, "sprintf", 
			IN_SPRINTF, IN_SPRINTF, uniqueTypeStr, uniqueTypeInt, true );
}

void Compiler::initStreamObject( )
{
	streamObj = ObjectDef::cons( ObjectDef::BuiltinType,
			"stream", nextObjectId++ );
	streamLangEl->objectDef = streamObj;

	initFunction( uniqueTypeStr, streamObj, "pull",  
			IN_INPUT_PULL_WV, IN_INPUT_PULL_WC, uniqueTypeInt, false );
	initFunction( uniqueTypeStr, streamObj, "push",  
			IN_INPUT_PUSH_WV, IN_INPUT_PUSH_WV, uniqueTypeAny, false );
	initFunction( uniqueTypeStr, streamObj, "push_ignore",  
			IN_INPUT_PUSH_IGNORE_WV, IN_INPUT_PUSH_IGNORE_WV, uniqueTypeAny, false );
}

ObjectField *Compiler::makeDataEl()
{
	/* Create the "data" field. */
	TypeRef *typeRef = TypeRef::cons( internal, uniqueTypeStr );
	ObjectField *el = ObjectField::cons( internal, typeRef, "data" );

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

ObjectField *Compiler::makePosEl()
{
	/* Create the "data" field. */
	TypeRef *typeRef = TypeRef::cons( internal, uniqueTypeInt );
	ObjectField *el = ObjectField::cons( internal, typeRef, "pos" );

	/* Setting beenReferenced to true prevents us from assigning instructions
	 * and an offset to the field. */

	el->isConst = true;
	el->beenReferenced = true;
	el->beenInitialized = true;
	el->useOffset = false;
	el->inGetR = IN_GET_TOKEN_POS_R;
	return el;
}

ObjectField *Compiler::makeLineEl()
{
	/* Create the "data" field. */
	TypeRef *typeRef = TypeRef::cons( internal, uniqueTypeInt );
	ObjectField *el = ObjectField::cons( internal, typeRef, "line" );

	/* Setting beenReferenced to true prevents us from assigning instructions
	 * and an offset to the field. */

	el->isConst = true;
	el->beenReferenced = true;
	el->beenInitialized = true;
	el->useOffset = false;
	el->inGetR = IN_GET_TOKEN_LINE_R;
	return el;
}

void Compiler::initFieldInstructions( ObjectField *el )
{
	el->inGetR =   IN_GET_FIELD_R;
	el->inGetWC =  IN_GET_FIELD_WC;
	el->inGetWV =  IN_GET_FIELD_WV;
	el->inSetWC =  IN_SET_FIELD_WC;
	el->inSetWV =  IN_SET_FIELD_WV;
}

void Compiler::initLocalInstructions( ObjectField *el )
{
	el->inGetR =   IN_GET_LOCAL_R;
	el->inGetWC =  IN_GET_LOCAL_WC;
	el->inSetWC =  IN_SET_LOCAL_WC;
}

void Compiler::initLocalRefInstructions( ObjectField *el )
{
	el->inGetR =   IN_GET_LOCAL_REF_R;
	el->inGetWC =  IN_GET_LOCAL_REF_WC;
	el->inSetWC =  IN_SET_LOCAL_REF_WC;
}

/* Add a constant length field to the object. 
 * Opcode supplied by the caller. */
void Compiler::addLengthField( ObjectDef *objDef, Code getLength )
{
	/* Create the "length" field. */
	TypeRef *typeRef = TypeRef::cons( internal, uniqueTypeInt );
	ObjectField *el = ObjectField::cons( internal, typeRef, "length" );
	el->beenReferenced = true;
	el->beenInitialized = true;
	el->isConst = true;
	el->useOffset = false;
	el->inGetR = getLength;

	objDef->rootScope->insertField( el->name, el );
}

void Compiler::initTokenObjects( )
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

void Compiler::initGlobalFunctions()
{
	ObjMethod *method;

	method = initFunction( uniqueTypeStream, globalObjectDef, "open",
		IN_OPEN_FILE, IN_OPEN_FILE, uniqueTypeStr, uniqueTypeStr, true );
	method->useCallObj = false;

	method = initFunction( uniqueTypeStr, globalObjectDef, "tolower",
		IN_TO_LOWER, IN_TO_LOWER, uniqueTypeStr, true );
	method->useCallObj = false;

	method = initFunction( uniqueTypeStr, globalObjectDef, "toupper",
		IN_TO_UPPER, IN_TO_UPPER, uniqueTypeStr, true );
	method->useCallObj = false;

	method = initFunction( uniqueTypeInt, globalObjectDef, "exit",
		IN_EXIT, IN_EXIT, uniqueTypeInt, true );

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
	ObjectField *el = ObjectField::cons( internal, typeRef, "stdin" );
	el->beenReferenced = true;
	el->beenInitialized = true;
	el->isConst = true;
	el->useOffset = false;
	el->inGetR     = IN_GET_STDIN;
	el->inGetWC    = IN_GET_STDIN;
	el->inGetWV    = IN_GET_STDIN;
	globalObjectDef->rootScope->insertField( el->name, el );
}

void Compiler::addStdout()
{
	/* Make the type ref. */
	TypeRef *typeRef = TypeRef::cons( internal, uniqueTypeStream );

	/* Create the field and insert it into the map. */
	ObjectField *el = ObjectField::cons( internal, typeRef, "stdout" );
	el->beenReferenced = true;
	el->beenInitialized = true;
	el->isConst = true;
	el->useOffset = false;
	el->inGetR    = IN_GET_STDOUT;
	el->inGetWC    = IN_GET_STDOUT;
	el->inGetWV    = IN_GET_STDOUT;
	globalObjectDef->rootScope->insertField( el->name, el );
}

void Compiler::addStderr()
{
	/* Make the type ref. */
	TypeRef *typeRef = TypeRef::cons( internal, uniqueTypeStream );

	/* Create the field and insert it into the map. */
	ObjectField *el = ObjectField::cons( internal, typeRef, "stderr" );
	el->beenReferenced = true;
	el->beenInitialized = true;
	el->isConst = true;
	el->useOffset = false;
	el->inGetR    = IN_GET_STDERR;
	el->inGetWC    = IN_GET_STDERR;
	el->inGetWV    = IN_GET_STDERR;
	globalObjectDef->rootScope->insertField( el->name, el );
}

void Compiler::addArgv()
{
	/* Create the field and insert it into the map. */
	ObjectField *el = ObjectField::cons( internal, argvTypeRef, "argv" );
	el->isArgv = true;
	el->isConst = true;
	globalObjectDef->rootScope->insertField( el->name, el );
}

void Compiler::addError()
{
	/* Make the type ref. */
	TypeRef *typeRef = TypeRef::cons( internal, uniqueTypeStr );

	/* Create the field and insert it into the map. */
	ObjectField *el = ObjectField::cons( internal, typeRef, "error" );
	el->beenReferenced = true;
	el->beenInitialized = true;
	el->isConst = true;
	el->useOffset = false;
	el->inGetR     = IN_GET_ERROR;
	el->inGetWC    = IN_GET_ERROR;
	el->inGetWV    = IN_GET_ERROR;
	globalObjectDef->rootScope->insertField( el->name, el );
}

int Compiler::argvOffset()
{
	for ( ObjFieldList::Iter field = *globalObjectDef->objFieldList;
			field.lte(); field++ )
	{
		if ( field->value->isArgv ) {
			globalObjectDef->referenceField( this, field->value );
			return field->value->offset;
		}
	}
	assert(false);
}

void Compiler::initMapFunctions( GenericType *gen )
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

void Compiler::initListFunctions( GenericType *gen )
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

void Compiler::initListField( GenericType *gen, const char *name, int offset )
{
	/* Make the type ref and create the field. */
	TypeRef *typeRef = TypeRef::cons( internal, gen->utArg );
	ObjectField *el = ObjectField::cons( internal, typeRef, name );

	el->inGetR =  IN_GET_LIST_MEM_R;
	el->inGetWC = IN_GET_LIST_MEM_WC;
	el->inGetWV = IN_GET_LIST_MEM_WV;
	el->inSetWC = IN_SET_LIST_MEM_WC;
	el->inSetWV = IN_SET_LIST_MEM_WV;

	gen->objDef->rootScope->insertField( el->name, el );

	el->useOffset = true;
	el->beenReferenced = true;
	el->beenInitialized = true;

	/* Zero for head, One for tail. */
	el->offset = offset;
}

void Compiler::initListFields( GenericType *gen )
{
	initListField( gen, "head", 0 );
	initListField( gen, "tail", 1 );
	initListField( gen, "top", 1 );
}

void Compiler::initVectorFunctions( GenericType *gen )
{
	addLengthField( gen->objDef, IN_VECTOR_LENGTH );
	initFunction( uniqueTypeInt, gen->objDef, "append", 
			IN_VECTOR_APPEND_WV, IN_VECTOR_APPEND_WC, gen->utArg, false );
	initFunction( uniqueTypeInt, gen->objDef, "insert", 
			IN_VECTOR_INSERT_WV, IN_VECTOR_INSERT_WC, uniqueTypeInt, gen->utArg, false );
}

void Compiler::initParserFunctions( GenericType *gen )
{
	initFunction( gen->utArg, gen->objDef, "finish",
			IN_PARSE_FINISH_WV, IN_PARSE_FINISH_WC, true );

	initFunction( gen->utArg, gen->objDef, "eof",
			IN_PARSE_FINISH_WV, IN_PARSE_FINISH_WC, true );
}

void Compiler::initParserField( GenericType *gen, const char *name, int offset, TypeRef *typeRef )
{
	/* Make the type ref and create the field. */
	ObjectField *el = ObjectField::cons( internal, typeRef, name );

	el->inGetR =  IN_GET_PARSER_MEM_R;
	el->inGetWC = IN_GET_PARSER_MEM_WC;
	el->inGetWV = IN_GET_PARSER_MEM_WV;
	el->inSetWC = IN_SET_PARSER_MEM_WC;
	el->inSetWV = IN_SET_PARSER_MEM_WV;

	gen->objDef->rootScope->insertField( el->name, el );

	el->useOffset = true;
	el->beenReferenced = true;
	el->beenInitialized = true;

	/* Zero for head, One for tail. */
	el->offset = offset;
}

void Compiler::initCtxField( GenericType *gen )
{
	LangEl *langEl = gen->utArg->langEl;
	Context *context = langEl->contextIn;

	/* Make the type ref and create the field. */
	UniqueType *ctxUT = findUniqueType( TYPE_TREE, context->lel );
	TypeRef *typeRef = TypeRef::cons( internal, ctxUT );
	ObjectField *el = ObjectField::cons( internal, typeRef, "ctx" );

	el->inGetR =  IN_GET_PARSER_CTX_R;
	el->inGetWC = IN_GET_PARSER_CTX_WC;
	el->inGetWV = IN_GET_PARSER_CTX_WV;
	el->inSetWC = IN_SET_PARSER_CTX_WC;
	el->inSetWV = IN_SET_PARSER_CTX_WV;

	gen->objDef->rootScope->insertField( el->name, el );

	el->useOffset = false;
	el->beenReferenced = true;
	el->beenInitialized = true;
}

void Compiler::initParserFields( GenericType *gen )
{
	LangEl *langEl = gen->utArg->langEl;
	if ( langEl->contextIn != 0 )
		initCtxField( gen );

	TypeRef *typeRef;

	typeRef = TypeRef::cons( internal, gen->utArg );
	initParserField( gen, "tree", 0, typeRef );

	typeRef = TypeRef::cons( internal, uniqueTypeStr );
	initParserField( gen, "error", 1, typeRef );
}

void Compiler::makeFuncVisible( Function *func, bool isUserIter )
{
	func->localFrame = func->codeBlock->localFrame;

	/* Set up the parameters. */
	long paramPos = 0;
	for ( ParameterList::Iter param = *func->paramList; param.lte(); param++ ) {
		if ( func->localFrame->rootScope->findField( param->name ) != 0 )
			error(param->loc) << "parameter " << param->name << " redeclared" << endp;

		func->localFrame->rootScope->insertField( param->name, param );
		param->beenInitialized = true;
		param->pos = paramPos;

		paramPos += 1;
	}

	/* Insert the function into the global function map. */
	ObjMethod *objMethod = new ObjMethod( func->typeRef, func->name, 
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

	globalObjectDef->objMethodMap->insert( func->name, objMethod );

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

	rootNamespace->declare( this );

	/* Will fill in zero lels that were not declared. */
	makeIgnoreCollectors();

	declareByteCode();

	initIntObject();
	initStrObject();
	initStreamObject();
	initTokenObjects();
	initGlobalFunctions();

	/* Fill any empty scanners with a default token. */
	initEmptyScanners();
}
