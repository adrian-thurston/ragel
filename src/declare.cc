/*
 *  Copyright 2012 Adrian Thurston <thurston@complang.org>
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
	uniqueTypeBool = new UniqueType( TYPE_TREE, boolLangEl );
	uniqueTypeInt = new UniqueType( TYPE_TREE, intLangEl );
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

ObjectField *NameScope::checkRedecl( const String &name )
{
	return owner->checkRedecl( this, name );
}

void NameScope::insertField( const String &name, ObjectField *value )
{
	return owner->insertField( this, name, value );
}

ObjectField *ObjectDef::checkRedecl( NameScope *inScope, const String &name )
{
	FieldMapEl *objDefMapEl = inScope->fieldMap->find( name );
	if ( objDefMapEl != 0 )
		return objDefMapEl->value;
	return 0;
}

void ObjectDef::insertField( NameScope *inScope, const String &name, ObjectField *value )
{
	inScope->fieldMap->insert( name, value );
	fieldList->append( value );
	value->scope = inScope;
}

NameScope *ObjectDef::pushScope( NameScope *curScope )
{
	NameScope *newScope = new NameScope;
	newScope->fieldMap = new FieldMap;

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

LangEl *declareLangEl( Compiler *pd, Namespace *nspace,
		const String &data, LangEl::Type type )
{
    /* If the id is already in the dict, it will be placed in last found. If
     * it is not there then it will be inserted and last found will be set to it. */
	TypeMapEl *inDict = nspace->typeMap.find( data );
	if ( inDict != 0 )
		error() << "'" << data << "' already defined as something else" << endp;

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
			error() << "'" << data << "' already defined as something else" << endp;
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
		error() << "'" << data << "' already defined as something else" << endp;

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
	boolLangEl = declareLangEl( this, rootNamespace, "bool", LangEl::Term );
	intLangEl = declareLangEl( this, rootNamespace, "int", LangEl::Term );
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
	utArg = typeArg->uniqueType;
 
	if ( typeId == GEN_MAP )
		keyUT = keyTypeArg->uniqueType; 

	objDef = ObjectDef::cons( ObjectDef::BuiltinType, 
			"generic", pd->nextObjectId++ );

	switch ( typeId ) {
		case GEN_MAP:
			pd->initMapFunctions( this );
			break;
		case GEN_LIST:
			pd->initListFunctions( this );
			pd->initListFields( this );
			pd->initListElFields( this );
			break;
		case GEN_LIST_EL:
//			pd->initListElFields( this );
			break;
		case GEN_PARSER:
			utArg->langEl->parserId = pd->nextParserId++;
			pd->initParserFunctions( this );
			pd->initParserFields( this );
			break;
		case GEN_MAP_EL:
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
	/* The iterator name. */
	LangVarRef *callVarRef = LangVarRef::cons( loc, context, scope, "triter" );

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
		ObjectMethod *objMethod = initFunction( uniqueTypeAny, globalObjectDef, 
				"triter", IN_HALT, IN_HALT, anyRefUT, true );

		IterDef *triter = findIterDef( IterDef::Tree );
		objMethod->iterDef = triter;
	}

	/* Child iterator. */
	{
		UniqueType *anyRefUT = findUniqueType( TYPE_REF, anyLangEl );
		ObjectMethod *objMethod = initFunction( uniqueTypeAny, globalObjectDef, 
				"child", IN_HALT, IN_HALT, anyRefUT, true );

		IterDef *triter = findIterDef( IterDef::Child );
		objMethod->iterDef = triter;
	}

	/* Reverse iterator. */
	{
		UniqueType *anyRefUT = findUniqueType( TYPE_REF, anyLangEl );
		ObjectMethod *objMethod = initFunction( uniqueTypeAny, globalObjectDef, 
				"rev_child", IN_HALT, IN_HALT, anyRefUT, true );

		IterDef *triter = findIterDef( IterDef::RevChild );
		objMethod->iterDef = triter;
	}

	/* Repeat iterator. */
	{
		UniqueType *anyRefUT = findUniqueType( TYPE_REF, anyLangEl );
		ObjectMethod *objMethod = initFunction( uniqueTypeAny, globalObjectDef, 
				"repeat", IN_HALT, IN_HALT, anyRefUT, true );

		IterDef *triter = findIterDef( IterDef::Repeat );
		objMethod->iterDef = triter;
	}

	/* Reverse repeat iterator. */
	{
		UniqueType *anyRefUT = findUniqueType( TYPE_REF, anyLangEl );
		ObjectMethod *objMethod = initFunction( uniqueTypeAny, globalObjectDef, 
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

void Compiler::addCtx( ObjectDef *frame )
{
	/* Make the type ref. */
	TypeRef *typeRef = TypeRef::cons( internal, uniqueTypeStream );

	/* Create the field and insert it into the map. */
	ObjectField *el = ObjectField::cons( internal,
			ObjectField::InbuiltObjectType, typeRef, "ctx" );
	el->inGetR     = IN_LOAD_CTX_R;
	el->inGetWV    = IN_LOAD_CTX_WV;
	el->inGetWC    = IN_LOAD_CTX_WC;
	el->inGetValR   = IN_LOAD_CTX_R;
	el->inGetValWC  = IN_LOAD_CTX_WC;
	el->inGetValWV  = IN_LOAD_CTX_WV;
	frame->rootScope->insertField( el->name, el );
}

void Compiler::declareIntFields( )
{
	intObj = ObjectDef::cons( ObjectDef::BuiltinType, "int", nextObjectId++ );
	intLangEl->objectDef = intObj;

	initFunction( uniqueTypeStr, intObj, "to_string", IN_INT_TO_STR, IN_INT_TO_STR, true );
}

void Compiler::declareStrFields( )
{
	strObj = ObjectDef::cons( ObjectDef::BuiltinType, "str", nextObjectId++ );
	strLangEl->objectDef = strObj;

	initFunction( uniqueTypeInt, strObj, "atoi",
			IN_STR_ATOI,   IN_STR_ATOI, true, true );

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

	addLengthField( strObj, IN_STR_LENGTH );

	initFunction( uniqueTypeStr, globalObjectDef, "sprintf", 
			IN_SPRINTF, IN_SPRINTF, uniqueTypeStr, uniqueTypeInt, true );
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
	return el;
}

/* Add a constant length field to the object. 
 * Opcode supplied by the caller. */
void Compiler::addLengthField( ObjectDef *objDef, Code getLength )
{
	/* Create the "length" field. */
	TypeRef *typeRef = TypeRef::cons( internal, uniqueTypeInt );
	ObjectField *el = ObjectField::cons( internal,
			ObjectField::InbuiltFieldType, typeRef, "length" );
	el->isConst = true;
	el->inGetR = getLength;

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

	method = initFunction( uniqueTypeInt, globalObjectDef, "system",
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
	globalObjectDef->rootScope->insertField( el->name, el );
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
	globalObjectDef->rootScope->insertField( el->name, el );
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
	globalObjectDef->rootScope->insertField( el->name, el );
}

void Compiler::addArgv()
{
	/* Create the field and insert it into the map. */
	ObjectField *el = ObjectField::cons( internal,
			ObjectField::StructFieldType, argvTypeRef, "argv" );
	el->isConst = true;
	globalObjectDef->rootScope->insertField( el->name, el );
	argv = el;

	TypeRef *typeRef = TypeRef::cons( internal, uniqueTypeStr );

	el = ObjectField::cons( internal,
			ObjectField::StructFieldType, typeRef, "arg0" );
	el->isConst = true;
	globalObjectDef->rootScope->insertField( el->name, el );
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
	globalObjectDef->rootScope->insertField( el->name, el );
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

	initFunction( uniqueTypeInt, gen->objDef, "push_head", 
			IN_LIST_PUSH_HEAD_WV, IN_LIST_PUSH_HEAD_WC, gen->utArg, false, false, gen );

	initFunction( uniqueTypeInt, gen->objDef, "push_tail", 
			IN_LIST_PUSH_TAIL_WV, IN_LIST_PUSH_TAIL_WC, gen->utArg, false, false, gen );

	initFunction( uniqueTypeInt, gen->objDef, "push", 
			IN_LIST_PUSH_HEAD_WV, IN_LIST_PUSH_HEAD_WC, gen->utArg, false, false, gen );

	initFunction( gen->utArg, gen->objDef, "pop_head", 
			IN_LIST_POP_HEAD_WV, IN_LIST_POP_HEAD_WC, false, false, gen );

	initFunction( gen->utArg, gen->objDef, "pop_tail", 
			IN_LIST_POP_TAIL_WV, IN_LIST_POP_TAIL_WC, false, false, gen );

	initFunction( gen->utArg, gen->objDef, "pop", 
			IN_LIST_POP_HEAD_WV, IN_LIST_POP_HEAD_WC, false, false, gen );
}

void Compiler::initListElField( GenericType *gen, const char *name, int offset )
{
	/* Make the type ref and create the field. */
	ObjectField *el = ObjectField::cons( internal,
			ObjectField::InbuiltOffType, gen->typeArg, name );

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

	gen->utArg->structEl->structDef->objectDef->rootScope->insertField( el->name, el );
}


void Compiler::initListElFields( GenericType *gen )
{
	initListElField( gen, "prev", 0 );
	initListElField( gen, "next", 1 );
}

void Compiler::initListField( GenericType *gen, const char *name, int offset )
{
	/* Make the type ref and create the field. */
	ObjectField *el = ObjectField::cons( internal,
			ObjectField::InbuiltOffType, gen->typeArg, name );

	el->inGetR =  IN_GET_LIST_MEM_R;
	el->inGetWC = IN_GET_LIST_MEM_WC;
	el->inGetWV = IN_GET_LIST_MEM_WV;
	el->inSetWC = IN_SET_LIST_MEM_WC;
	el->inSetWV = IN_SET_LIST_MEM_WV;

	el->inGetValR  =  IN_GET_LIST_MEM_R;
	el->inGetValWC =  IN_GET_LIST_MEM_WC;
	el->inGetValWV =  IN_GET_LIST_MEM_WV;

	gen->objDef->rootScope->insertField( el->name, el );

	el->useGenericId = true;
	el->generic = gen;

	/* Zero for head, One for tail. */
	el->offset = offset;
}

void Compiler::initListFields( GenericType *gen )
{
	initListField( gen, "head", 0 );
	initListField( gen, "tail", 1 );
	initListField( gen, "top", 0 );
}

void Compiler::initParserFunctions( GenericType *gen )
{
	initFunction( gen->utArg, gen->objDef, "finish",
			IN_PARSE_FINISH_WV, IN_PARSE_FINISH_WC, true );

	initFunction( gen->utArg, gen->objDef, "eof",
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

void Compiler::initCtxField( GenericType *gen )
{
	LangEl *langEl = gen->utArg->langEl;
	StructDef *structDef = langEl->contextIn;

	/* Make the type ref and create the field. */
	UniqueType *ctxUT = findUniqueType( TYPE_STRUCT, structDef->structEl );
	TypeRef *typeRef = TypeRef::cons( internal, ctxUT );
	ObjectField *el = ObjectField::cons( internal,
			ObjectField::InbuiltFieldType, typeRef, "ctx" );

	el->inGetR =  IN_GET_PARSER_CTX_R;
	el->inGetWC = IN_GET_PARSER_CTX_WC;
	el->inGetWV = IN_GET_PARSER_CTX_WV;
	el->inSetWC = IN_SET_PARSER_CTX_WC;
	el->inSetWV = IN_SET_PARSER_CTX_WV;

	el->inGetValR =  IN_GET_PARSER_CTX_R;
	el->inSetValWC = IN_SET_PARSER_CTX_WC;
	el->inSetValWV = IN_SET_PARSER_CTX_WV;

	gen->objDef->rootScope->insertField( el->name, el );
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

	globalObjectDef->methodMap->insert( func->name, objMethod );

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

	declareIntFields();
	declareStrFields();
	declareStreamFields();
	declareTokenFields();
	declareGlobalFields();

	/* Fill any empty scanners with a default token. */
	initEmptyScanners();
}
