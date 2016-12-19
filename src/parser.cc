/*
 * Copyright 2006-2014 Adrian Thurston <thurston@colm.net>
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

#include "parser.h"

#include <stdbool.h>
#include <stdlib.h>
#include <errno.h>

#include <iostream>

using std::endl;

void BaseParser::listElDef( String name )
{
	/*
	 * The unique type. This is a def with a single empty form.
	 */
	ObjectDef *objectDef = ObjectDef::cons( ObjectDef::UserType,
			name, pd->nextObjectId++ );

	LelDefList *defList = new LelDefList;

	Production *prod = BaseParser::production( InputLoc(),
			new ProdElList, String(), false, 0, 0 );

	prodAppend( defList, prod );

	NtDef *ntDef = NtDef::cons( name, curNspace(), curStruct(), false );
	BaseParser::cflDef( ntDef, objectDef, defList );

	/*
	 * List element with the same name as containing context.
	 */
	NamespaceQual *nspaceQual = NamespaceQual::cons( curNspace() );
	String id = curStruct()->objectDef->name;
	RepeatType repeatType = RepeatNone;
	TypeRef *objTr = TypeRef::cons( InputLoc(), nspaceQual, id, repeatType );
	TypeRef *elTr = TypeRef::cons( InputLoc(), TypeRef::ListPtrs, 0, objTr, 0 );

	ObjectField *of = ObjectField::cons( InputLoc(),
			ObjectField::GenericElementType, elTr, name );

	structVarDef( InputLoc(), of );
}

void BaseParser::mapElDef( String name, TypeRef *keyType )
{
	/*
	 * The unique type. This is a def with a single empty form.
	 */
	ObjectDef *objectDef = ObjectDef::cons( ObjectDef::UserType,
			name, pd->nextObjectId++ );

	LelDefList *defList = new LelDefList;

	Production *prod = BaseParser::production( InputLoc(),
			new ProdElList, String(), false, 0, 0 );
	prodAppend( defList, prod );

	NtDef *ntDef = NtDef::cons( name, curNspace(), curStruct(), false );
	BaseParser::cflDef( ntDef, objectDef, defList );

	/*
	 * Same name as containing context.
	 */
	NamespaceQual *nspaceQual = NamespaceQual::cons( curNspace() );
	String id = curStruct()->objectDef->name;
	TypeRef *objTr = TypeRef::cons( InputLoc(), nspaceQual, id, RepeatNone );
	TypeRef *elTr = TypeRef::cons( InputLoc(), TypeRef::MapPtrs, 0, objTr, keyType );

	ObjectField *of = ObjectField::cons( InputLoc(),
			ObjectField::GenericElementType, elTr, name );
	structVarDef( InputLoc(), of );
}

#if 0
void BaseParser::argvDecl()
{
	String structName = "argv_el";
	structHead( internal, pd->rootNamespace, structName, ObjectDef::StructType );

	/* First the argv value. */
	String name = "value";
	String type = "str";
	NamespaceQual *nspaceQual = NamespaceQual::cons( curNspace() );
	TypeRef *typeRef = TypeRef::cons( internal, nspaceQual, type, RepeatNone );
	ObjectField *objField = ObjectField::cons( internal,
			ObjectField::StructFieldType, typeRef, name );
	structVarDef( objField->loc, objField );

	pd->argvEl = objField->context;

	/* Now the list element. */
	listElDef( "el" );

	structStack.pop();
	namespaceStack.pop();
}
#endif

void BaseParser::init()
{
	/* Set up the root namespace. */
	pd->rootNamespace = createRootNamespace();

	/* Setup the global object. */
	String global = "global";
	pd->globalObjectDef = ObjectDef::cons( ObjectDef::UserType,
			global, pd->nextObjectId++ ); 

	pd->rootNamespace->rootScope->owningObj = pd->globalObjectDef;

	pd->global = new StructDef( internal, global, pd->globalObjectDef );
	pd->globalSel = declareStruct( pd, 0, global, pd->global );

	/* Setup the stream object. */
	global = "stream";
	ObjectDef *objectDef = ObjectDef::cons( ObjectDef::BuiltinType,
			global, pd->nextObjectId++ ); 

	pd->stream = new StructDef( internal, global, objectDef );
	pd->streamSel = declareStruct( pd, pd->rootNamespace,
			pd->stream->name, pd->stream );
	
	/* Initialize the dictionary of graphs. This is our symbol table. The
	 * initialization needs to be done on construction which happens at the
	 * beginning of a machine spec so any assignment operators can reference
	 * the builtins. */
	pd->initGraphDict();

	pd->rootLocalFrame = ObjectDef::cons( ObjectDef::FrameType, 
				"local", pd->nextObjectId++ );
	localFrameTop = pd->rootLocalFrame;
	scopeTop = pd->rootLocalFrame->rootScope;


	/* Declarations of internal types. They must be declared now because we use
	 * them directly, rather than via type lookup. */
	pd->declareBaseLangEls();
	pd->initUniqueTypes();

	//argvDecl();

	/* Internal variables. */
	addArgvList();
}

void BaseParser::addRegularDef( const InputLoc &loc, Namespace *nspace,
		const String &name, LexJoin *join )
{
	GraphDictEl *newEl = nspace->rlMap.insert( name );
	if ( newEl != 0 ) {
		/* New element in the dict, all good. */
		newEl->value = new LexDefinition( name, join );
		newEl->isInstance = false;
		newEl->loc = loc;
	}
	else {
		// Recover by ignoring the duplicate.
		error(loc) << "regular definition \"" << name << "\" already exists" << endl;
	}
}

TokenRegion *BaseParser::createRegion( const InputLoc &loc, RegionImpl *impl )
{
	TokenRegion *tokenRegion = new TokenRegion( loc,
			pd->regionList.length(), impl );

	pd->regionList.append( tokenRegion );

	return tokenRegion;
}

void BaseParser::pushRegionSet( const InputLoc &loc )
{
	RegionImpl *implTokenIgnore = new RegionImpl;
	RegionImpl *implTokenOnly = new RegionImpl;
	RegionImpl *implIgnoreOnly = new RegionImpl;

	pd->regionImplList.append( implTokenIgnore );
	pd->regionImplList.append( implTokenOnly );
	pd->regionImplList.append( implIgnoreOnly );

	TokenRegion *tokenIgnore = createRegion( loc, implTokenIgnore );
	TokenRegion *tokenOnly = createRegion( loc, implTokenOnly );
	TokenRegion *ignoreOnly = createRegion( loc, implIgnoreOnly );
	TokenRegion *collectIgnore = createRegion( loc, implIgnoreOnly );

	RegionSet *regionSet = new RegionSet(
			implTokenIgnore, implTokenIgnore, implIgnoreOnly,
			tokenIgnore, tokenOnly, ignoreOnly, collectIgnore );

	collectIgnore->ignoreOnly = ignoreOnly;

	pd->regionSetList.append( regionSet );
	regionStack.push( regionSet );
}

void BaseParser::popRegionSet()
{
	regionStack.pop();
}

Namespace *BaseParser::createRootNamespace()
{
	/* Gets id of zero and default name. No parent. */
	Namespace *nspace = new Namespace( internal,
			String("___ROOT_NAMESPACE"), 0, 0 );

	nspace->rootScope->owningObj = pd->globalObjectDef;

	pd->namespaceList.append( nspace );
	namespaceStack.push( nspace );

	return nspace;
}

Namespace *BaseParser::createNamespace( const InputLoc &loc, const String &name )
{
	Namespace *parent = namespaceStack.top();

	/* Make the new namespace. */
	Namespace *nspace = parent->findNamespace( name );
	
	if ( nspace == 0 ) {
		nspace = new Namespace( loc, name,
				pd->namespaceList.length(), parent );

		/* Link the new namespace's scope to the parent namespace's scope. */
		nspace->rootScope->parentScope = parent->rootScope;
		nspace->rootScope->owningObj = pd->globalObjectDef;

		parent->childNamespaces.append( nspace );
		pd->namespaceList.append( nspace );
	}

	namespaceStack.push( nspace );

	return nspace;
}

Reduction *BaseParser::createReduction( const InputLoc loc, const String &name )
{
	Namespace *parent = namespaceStack.top();
	Reduction *reduction = parent->findReduction( name );

	if ( reduction == 0 ) {
		reduction = new Reduction( loc, name );
		parent->reductions.append( reduction );
	}

	reductionStack.push( reduction );

	return reduction;
}

LexJoin *BaseParser::literalJoin( const InputLoc &loc, const String &data )
{
	Literal *literal = Literal::cons( loc, data, Literal::LitString );
	LexFactor *factor = LexFactor::cons( literal );
	LexFactorNeg *factorNeg = LexFactorNeg::cons( factor );
	LexFactorRep *factorRep = LexFactorRep::cons( factorNeg );
	LexFactorAug *factorAug = LexFactorAug::cons( factorRep );
	LexTerm *term = LexTerm::cons( factorAug );
	LexExpression *expr = LexExpression::cons( term );
	LexJoin *join = LexJoin::cons( expr );
	return join;
}

void BaseParser::defineToken( const InputLoc &loc, String name, LexJoin *join,
		ObjectDef *objectDef, CodeBlock *transBlock, bool ignore,
		bool noPreIgnore, bool noPostIgnore )
{
	bool pushedRegion = false;
	if ( !insideRegion() ) {
		if ( ignore )
			error(loc) << "ignore tokens can only appear inside scanners" << endp;
	
		pushedRegion = true;
		pushRegionSet( internal );
	}

	/* Check the name if this is a token. */
	if ( !ignore && name == 0 )
		error(loc) << "tokens must have a name" << endp;

	/* Give a default name to ignores. */ 
	if ( name == 0 )
		name.setAs( 32, "_ignore_%.4x", pd->nextTokenId );

	Namespace *nspace = curNspace();
	RegionSet *regionSet = regionStack.top();

	TokenDef *tokenDef = TokenDef::cons( name, String(), false, ignore, join, 
			transBlock, loc, 0, nspace, regionSet, objectDef, curStruct() );

	regionSet->tokenDefList.append( tokenDef );
	nspace->tokenDefList.append( tokenDef );

	tokenDef->noPreIgnore = noPreIgnore;
	tokenDef->noPostIgnore = noPostIgnore;

	TokenInstance *tokenInstance = TokenInstance::cons( tokenDef,
			join, loc, pd->nextTokenId++, nspace, 
			regionSet->tokenIgnore );

	regionSet->tokenIgnore->impl->tokenInstanceList.append( tokenInstance );

	tokenDef->noPreIgnore = noPreIgnore;
	tokenDef->noPostIgnore = noPostIgnore;

	if ( ignore ) {
		/* The instance for the ignore-only. */
		TokenInstance *tokenInstanceIgn = TokenInstance::cons( tokenDef,
				join, loc, pd->nextTokenId++, nspace, regionSet->ignoreOnly );

		tokenInstanceIgn->dupOf = tokenInstance;

		regionSet->ignoreOnly->impl->tokenInstanceList.append( tokenInstanceIgn );
	}
	else {
		/* The instance for the token-only. */
		TokenInstance *tokenInstanceTok = TokenInstance::cons( tokenDef,
				join, loc, pd->nextTokenId++, nspace, regionSet->tokenOnly );

		tokenInstanceTok->dupOf = tokenInstance;

		regionSet->tokenOnly->impl->tokenInstanceList.append( tokenInstanceTok );
	}

	/* This is created and pushed in the name. */
	if ( pushedRegion )
		popRegionSet();

	if ( join != 0 ) {
		/* Create a regular language definition so the token can be used to
		 * make other tokens */
		addRegularDef( loc, curNspace(), name, join );
	}
}

void BaseParser::zeroDef( const InputLoc &loc, const String &name )
{
	if ( !insideRegion() )
		error(loc) << "zero token should be inside token" << endp;

	RegionSet *regionSet = regionStack.top();
	Namespace *nspace = curNspace();

	LexJoin *join = literalJoin( loc, String("`") );

	TokenDef *tokenDef = TokenDef::cons( name, String(), false, false, join,
			0, loc, 0, nspace, regionSet, 0, curStruct() );

	tokenDef->isZero = true;

	regionSet->tokenDefList.append( tokenDef );
	nspace->tokenDefList.append( tokenDef );

	/* No token instance created. */
}

void BaseParser::literalDef( const InputLoc &loc, const String &data,
		bool noPreIgnore, bool noPostIgnore )
{
	/* Create a name for the literal. */
	String name( 32, "_literal_%.4x", pd->nextTokenId );

	bool pushedRegion = false;
	if ( !insideRegion() ) {
		pushRegionSet( loc );
		pushedRegion = true;
	}

	bool unusedCI;
	String interp;
	prepareLitString( interp, unusedCI, data, loc );

	/* Look for the production's associated region. */
	Namespace *nspace = curNspace();
	RegionSet *regionSet = regionStack.top();

	LiteralDictEl *ldel = nspace->literalDict.find( interp );
	if ( ldel != 0 )
		error( loc ) << "literal already defined in this namespace" << endp;

	LexJoin *join = literalJoin( loc, data );

	ObjectDef *objectDef = ObjectDef::cons( ObjectDef::UserType, 
			name, pd->nextObjectId++ );

	/* The token definition. */
	TokenDef *tokenDef = TokenDef::cons( name, data, true, false, join, 
			0, loc, 0, nspace, regionSet, objectDef, 0 );

	regionSet->tokenDefList.append( tokenDef );
	nspace->tokenDefList.append( tokenDef );

	/* The instance for the token/ignore region. */
	TokenInstance *tokenInstance = TokenInstance::cons( tokenDef, join, 
			loc, pd->nextTokenId++, nspace, regionSet->tokenIgnore );

	regionSet->tokenIgnore->impl->tokenInstanceList.append( tokenInstance );

	ldel = nspace->literalDict.insert( interp, tokenInstance );

	/* Make the duplicate for the token-only region. */
	tokenDef->noPreIgnore = noPreIgnore;
	tokenDef->noPostIgnore = noPostIgnore;

	/* The instance for the token-only region. */
	TokenInstance *tokenInstanceTok = TokenInstance::cons( tokenDef,
			join, loc, pd->nextTokenId++, nspace,
			regionSet->tokenOnly );

	tokenInstanceTok->dupOf = tokenInstance;

	regionSet->tokenOnly->impl->tokenInstanceList.append( tokenInstanceTok );

	if ( pushedRegion )
		popRegionSet();
}

void BaseParser::addArgvList()
{
	TypeRef *valType = TypeRef::cons( internal, pd->uniqueTypeStr );
	TypeRef *elType = TypeRef::cons( internal, TypeRef::ListEl, valType );
	pd->argvTypeRef = TypeRef::cons( internal, TypeRef::List, 0, elType, valType );
}

ObjectDef *BaseParser::blockOpen()
{
	/* Init the object representing the local frame. */
	ObjectDef *frame = ObjectDef::cons( ObjectDef::FrameType, 
			"local", pd->nextObjectId++ );

	localFrameTop = frame;
	scopeTop = frame->rootScope;
	return frame;
}

void BaseParser::blockClose()
{
	localFrameTop = pd->rootLocalFrame;
	scopeTop = pd->rootLocalFrame->rootScope;
}

void BaseParser::functionDef( StmtList *stmtList, ObjectDef *localFrame,
	ParameterList *paramList, TypeRef *typeRef, const String &name, bool exprt )
{
	CodeBlock *codeBlock = CodeBlock::cons( stmtList, localFrame );
	Function *newFunction = Function::cons( curNspace(), typeRef, name, 
			paramList, codeBlock, pd->nextFuncId++, false, exprt );
	pd->functionList.append( newFunction );
	newFunction->inContext = curStruct();
}

void BaseParser::inHostDef( const String &hostCall, ObjectDef *localFrame,
	ParameterList *paramList, TypeRef *typeRef, const String &name, bool exprt )
{
	Function *newFunction = Function::cons( curNspace(), typeRef, name, 
			paramList, 0, pd->nextHostId++, false, exprt );
	newFunction->hostCall = hostCall;
	newFunction->localFrame = localFrame;
	newFunction->inHost = true;
	pd->inHostList.append( newFunction );
	newFunction->inContext = curStruct();
}

void BaseParser::iterDef( StmtList *stmtList, ObjectDef *localFrame,
	ParameterList *paramList, const String &name )
{
	CodeBlock *codeBlock = CodeBlock::cons( stmtList, localFrame );
	Function *newFunction = Function::cons( curNspace(), 0, name, 
			paramList, codeBlock, pd->nextFuncId++, true, false );
	pd->functionList.append( newFunction );
}

LangStmt *BaseParser::globalDef( ObjectField *objField, LangExpr *expr, 
		LangStmt::Type assignType )
{
	LangStmt *stmt = 0;
	ObjectDef *object = pd->globalObjectDef;
	Namespace *nspace = curNspace(); //pd->rootNamespace;

	if ( nspace->rootScope->checkRedecl( objField->name ) != 0 )
		error(objField->loc) << "object field renamed" << endp;

	object->insertField( nspace->rootScope, objField->name, objField );

	if ( expr != 0 ) {
		LangVarRef *varRef = LangVarRef::cons( objField->loc,
				curNspace(), curStruct(), curScope(), objField->name );

		stmt = LangStmt::cons( objField->loc, assignType, varRef, expr );
	}

	return stmt;
}

LangStmt *BaseParser::exportStmt( ObjectField *objField,
		LangStmt::Type assignType, LangExpr *expr )
{
	LangStmt *stmt = 0;

	ObjectDef *object = pd->globalObjectDef;
	Namespace *nspace = curNspace(); //pd->rootNamespace;

	if ( curStruct() != 0 )
		error(objField->loc) << "cannot export parser context variables" << endp;

	if ( nspace->rootScope->checkRedecl( objField->name ) != 0 )
		error(objField->loc) << "object field renamed" << endp;

	object->insertField( nspace->rootScope, objField->name, objField );
	objField->isExport = true;

	if ( expr != 0 ) {
		LangVarRef *varRef = LangVarRef::cons( objField->loc, 
				curNspace(), 0, curScope(), objField->name );

		stmt = LangStmt::cons( objField->loc, assignType, varRef, expr );
	}

	return stmt;
}


void BaseParser::cflDef( NtDef *ntDef, ObjectDef *objectDef, LelDefList *defList )
{
	Namespace *nspace = curNspace();

	ntDef->objectDef = objectDef;
	ntDef->defList = defList;

	nspace->ntDefList.append( ntDef );

	/* Declare the captures in the object. */
	for ( LelDefList::Iter prod = *defList; prod.lte(); prod++ ) {
		for ( ProdElList::Iter pel = *prod->prodElList; pel.lte(); pel++ ) {
			/* If there is a capture, create the field. */
			if ( pel->captureField != 0 ) {
				/* Might already exist. */
				ObjectField *newOf = objectDef->rootScope->checkRedecl(
						pel->captureField->name );
				if ( newOf != 0 ) {
					/* FIXME: check the types are the same. */
				}
				else {
					newOf = pel->captureField;
					newOf->typeRef = pel->typeRef;
					objectDef->rootScope->insertField( newOf->name, newOf );
				}

				newOf->rhsVal.append( RhsVal( pel ) );
			}
		}
	}
}

ReOrBlock *BaseParser::lexRegularExprData( ReOrBlock *reOrBlock, ReOrItem *reOrItem )
{
	ReOrBlock *ret;

	/* An optimization to lessen the tree size. If an or char is directly under
	 * the left side on the right and the right side is another or char then
	 * paste them together and return the left side. Otherwise just put the two
	 * under a new or data node. */
	if ( reOrItem->type == ReOrItem::Data &&
			reOrBlock->type == ReOrBlock::RecurseItem &&
			reOrBlock->item->type == ReOrItem::Data )
	{
		/* Append the right side to right side of the left and toss the
		 * right side. */
		reOrBlock->item->data += reOrItem->data;
		delete reOrItem;
		ret = reOrBlock;
	}
	else {
		/* Can't optimize, put the left and right under a new node. */
		ret = ReOrBlock::cons( reOrBlock, reOrItem );
	}
	return ret;
}

LexFactor *BaseParser::lexRlFactorName( const String &data, const InputLoc &loc )
{
	LexFactor *factor = 0;
	/* Find the named graph. */
	Namespace *nspace = curNspace();

	while ( nspace != 0 ) {
		GraphDictEl *gdNode = nspace->rlMap.find( data );
		if ( gdNode != 0 ) {
			if ( gdNode->isInstance ) {
				/* Recover by retuning null as the factor node. */
				error(loc) << "references to graph instantiations not allowed "
						"in expressions" << endl;
				factor = 0;
			}
			else {
				/* Create a factor node that is a lookup of an expression. */
				factor = LexFactor::cons( loc, gdNode->value );
			}
			break;
		}

		nspace = nspace->parentNamespace;
	}

	if ( nspace == 0 ) {
		/* Recover by returning null as the factor node. */
		error(loc) << "graph lookup of \"" << data << "\" failed" << endl;
		factor = 0;
	}

	return factor;
}

int BaseParser::lexFactorRepNum( const InputLoc &loc, const String &data )
{
	/* Convert the priority number to a long. Check for overflow. */
	errno = 0;
	long rep = strtol( data, 0, 10 );
	if ( errno == ERANGE && rep == LONG_MAX ) {
		/* Repetition too large. Recover by returing repetition 1. */
		error(loc) << "repetition number " << data << " overflows" << endl;
		rep = 1;
	}
	return rep;
}

LexFactorAug *BaseParser::lexFactorLabel( const InputLoc &loc,
		const String &data, LexFactorAug *factorAug )
{
	/* Create the object field. */
	TypeRef *typeRef = TypeRef::cons( loc, pd->uniqueTypeStr );
	ObjectField *objField = ObjectField::cons( loc,
			ObjectField::LexSubstrType, typeRef, data );

	/* Create the enter and leaving actions that will mark the substring. */
	Action *enter = Action::cons( MarkMark, pd->nextMatchEndNum++ );
	Action *leave = Action::cons( MarkMark, pd->nextMatchEndNum++ );
	pd->actionList.append( enter );
	pd->actionList.append( leave );
	
	/* Add entering and leaving actions. */
	factorAug->actions.append( ParserAction( loc, at_start, 0, enter ) );
	factorAug->actions.append( ParserAction( loc, at_leave, 0, leave ) );

	factorAug->reCaptureVect.append( ReCapture( enter, leave, objField ) );

	return factorAug;
}

LexJoin *BaseParser::lexOptJoin( LexJoin *join, LexJoin *context )
{
	if ( context != 0 ) {
		/* Create the enter and leaving actions that will mark the substring. */
		Action *mark = Action::cons( MarkMark, pd->nextMatchEndNum++ );
		pd->actionList.append( mark );

		join->context = context;
		join->mark = mark;
	}

	return join;
}

LangExpr *BaseParser::send( const InputLoc &loc, LangVarRef *varRef,
		ConsItemList *list, bool eof )
{
	ParserText *parserText = ParserText::cons( loc,
			curNspace(), list, true, false, "" );
	pd->parserTextList.append( parserText );

	return LangExpr::cons( LangTerm::consSend( loc, varRef,
			parserText, eof ) );
}

LangExpr *BaseParser::sendTree( const InputLoc &loc, LangVarRef *varRef,
		ConsItemList *list, bool eof )
{
	ParserText *parserText = ParserText::cons( loc,
			curNspace(), list, true, false, "" );
	pd->parserTextList.append( parserText );

	return LangExpr::cons( LangTerm::consSendTree( loc, varRef,
			parserText, eof ) );
}

LangExpr *BaseParser::parseCmd( const InputLoc &loc, bool tree, bool stop,
		ObjectField *objField, TypeRef *typeRef, FieldInitVect *fieldInitVect,
		ConsItemList *list, bool used, bool reduce, const String &reducer )
{
	LangExpr *expr = 0;

	/* Item list for what we are sending to the parser. */
	ConsItemList *consItemList = new ConsItemList;
	
	/* The parser may be referenced. */
	LangVarRef *varRef = 0;
	if ( objField != 0 ) {
		varRef = LangVarRef::cons( objField->loc,
				curNspace(), curStruct(), curScope(), objField->name );
	}

	/* The typeref for the parser. */
	TypeRef *parserTypeRef = TypeRef::cons( loc,
			TypeRef::Parser, 0, typeRef, 0 );

	if ( objField != 0 )
		used = true;

	ParserText *parserText = ParserText::cons( loc, curNspace(),
			list, used, reduce, reducer );
	pd->parserTextList.append( parserText );

	LangTerm::Type langTermType = stop ? LangTerm::ParseStopType : ( tree ? 
					LangTerm::ParseTreeType : LangTerm::ParseType );

	expr = LangExpr::cons( LangTerm::cons( loc, langTermType,
			varRef, objField, parserTypeRef, fieldInitVect, consItemList,
			parserText ) );

	/* Check for redeclaration. */
	if ( objField != 0 ) {
		if ( curScope()->checkRedecl( objField->name ) != 0 ) {
			error( objField->loc ) << "variable " << objField->name <<
					" redeclared" << endp;
		}

		/* Insert it into the field map. */
		objField->typeRef = typeRef;
		curScope()->insertField( objField->name, objField );
	}

	return expr;
}

PatternItemList *BaseParser::consPatternEl( LangVarRef *varRef, PatternItemList *list )
{
	/* Store the variable reference in the pattern itemm. */
	list->head->varRef = varRef;

	if ( varRef != 0 ) {
		if ( curScope()->checkRedecl( varRef->name ) != 0 ) {
			error( varRef->loc ) << "variable " << varRef->name << 
					" redeclared" << endp;
		}

		TypeRef *typeRef = list->head->prodEl->typeRef;
		ObjectField *objField = ObjectField::cons( InputLoc(),
				ObjectField::UserLocalType, typeRef, varRef->name );

		/* Insert it into the field map. */
		curScope()->insertField( varRef->name, objField );
	}

	return list;
}

PatternItemList *BaseParser::patternElNamed( const InputLoc &loc,
		LangVarRef *parsedVarRef, NamespaceQual *nspaceQual, const String &data,
		RepeatType repeatType )
{
	TypeRef *typeRef = TypeRef::cons( loc, parsedVarRef, nspaceQual, data, repeatType );
	ProdEl *prodEl = new ProdEl( ProdEl::ReferenceType, loc, 0, false, typeRef, 0 );
	PatternItem *patternItem = PatternItem::cons( PatternItem::TypeRefForm, loc, prodEl );
	return PatternItemList::cons( patternItem );
}

PatternItemList *BaseParser::patternElType( const InputLoc &loc,
		LangVarRef *parsedVarRef, NamespaceQual *nspaceQual, const String &data,
		RepeatType repeatType )
{
	PdaLiteral *literal = new PdaLiteral( loc, data );
	TypeRef *typeRef = TypeRef::cons( loc, parsedVarRef, nspaceQual, literal, repeatType );

	ProdEl *prodEl = new ProdEl( ProdEl::ReferenceType, loc, 0, false, typeRef, 0 );
	PatternItem *patternItem = PatternItem::cons( PatternItem::TypeRefForm, loc, prodEl );
	return PatternItemList::cons( patternItem );
}

ProdElList *BaseParser::appendProdEl( ProdElList *prodElList, ProdEl *prodEl )
{
	prodEl->pos = prodElList->length();
	prodElList->append( prodEl );
	return prodElList;
}

PatternItemList *BaseParser::patListConcat( PatternItemList *list1,
		PatternItemList *list2 )
{
	list1->append( *list2 );
	delete list2;
	return list1;
}

ConsItemList *BaseParser::consListConcat( ConsItemList *list1,
		ConsItemList *list2 )
{
	list1->append( *list2 );
	delete list2;
	return list1;
}

LangStmt *BaseParser::forScope( const InputLoc &loc, const String &data,
		NameScope *scope, TypeRef *typeRef, IterCall *iterCall, StmtList *stmtList )
{
	/* Check for redeclaration. */
	if ( curScope()->checkRedecl( data ) != 0 )
		error( loc ) << "variable " << data << " redeclared" << endp;

	/* Note that we pass in a null type reference. This type is dependent on
	 * the result of the iter_call lookup since it must contain a reference to
	 * the iterator that is called. This lookup is done at compile time. */
	ObjectField *iterField = ObjectField::cons( loc,
			ObjectField::UserLocalType, (TypeRef*)0, data );
	curScope()->insertField( data, iterField );

	LangStmt *stmt = LangStmt::cons( loc, LangStmt::ForIterType, 
			iterField, typeRef, iterCall, stmtList, curStruct(), scope );

	return stmt;
}

void BaseParser::preEof( const InputLoc &loc, StmtList *stmtList, ObjectDef *localFrame )
{
	if ( !insideRegion() )
		error(loc) << "preeof must be used inside an existing region" << endl;

	CodeBlock *codeBlock = CodeBlock::cons( stmtList, localFrame );
	codeBlock->context = curStruct();

	RegionSet *regionSet = regionStack.top();
	regionSet->tokenIgnore->preEofBlock = codeBlock;
}

ProdEl *BaseParser::prodElName( const InputLoc &loc, const String &data,
		NamespaceQual *nspaceQual, ObjectField *objField,
		RepeatType repeatType, bool commit )
{
	TypeRef *typeRef = TypeRef::cons( loc, nspaceQual, data, repeatType );
	ProdEl *prodEl = new ProdEl( ProdEl::ReferenceType, loc, objField, commit, typeRef, 0 );
	return prodEl;
}

ProdEl *BaseParser::prodElLiteral( const InputLoc &loc, const String &data,
		NamespaceQual *nspaceQual, ObjectField *objField, RepeatType repeatType,
		bool commit )
{
	/* Create a new prodEl node going to a concat literal. */
	PdaLiteral *literal = new PdaLiteral( loc, data );
	TypeRef *typeRef = TypeRef::cons( loc, nspaceQual, literal, repeatType );
	ProdEl *prodEl = new ProdEl( ProdEl::LiteralType, loc, objField, commit, typeRef, 0 );
	return prodEl;
}

ConsItemList *BaseParser::consElLiteral( const InputLoc &loc,
		TypeRef *consTypeRef, const String &data, NamespaceQual *nspaceQual )
{
	PdaLiteral *literal = new PdaLiteral( loc, data );
	TypeRef *typeRef = TypeRef::cons( loc, consTypeRef, nspaceQual, literal );
	ProdEl *prodEl = new ProdEl( ProdEl::LiteralType, loc, 0, false, typeRef, 0 );
	ConsItem *consItem = ConsItem::cons( loc, ConsItem::LiteralType, prodEl );
	ConsItemList *list = ConsItemList::cons( consItem );
	return list;
}

Production *BaseParser::production( const InputLoc &loc, ProdElList *prodElList,
		String name, bool commit, CodeBlock *codeBlock, LangEl *predOf )
{
	Production *prod = Production::cons( loc, 0, prodElList,
			name, commit, codeBlock, pd->prodList.length(), 0 );
	prod->predOf = predOf;

	/* Link the production elements back to the production. */
	for ( ProdEl *prodEl = prodElList->head; prodEl != 0; prodEl = prodEl->next )
		prodEl->production = prod;

	pd->prodList.append( prod );

	return prod;
}

void BaseParser::objVarDef( ObjectDef *objectDef, ObjectField *objField )
{
	if ( objectDef->rootScope->checkRedecl( objField->name ) != 0 )
		error() << "object field renamed" << endp;

	objectDef->rootScope->insertField( objField->name, objField );
}

LelDefList *BaseParser::prodAppend( LelDefList *defList, Production *definition )
{
	definition->prodNum = defList->length();
	defList->append( definition );
	return defList;
}

LangExpr *BaseParser::construct( const InputLoc &loc, ObjectField *objField,
		ConsItemList *list, TypeRef *typeRef, FieldInitVect *fieldInitVect )
{
	Constructor *constructor = Constructor::cons( loc, curNspace(),
			list, pd->nextPatConsId++ );
	pd->replList.append( constructor );
	
	LangVarRef *varRef = 0;
	if ( objField != 0 ) {
		varRef = LangVarRef::cons( objField->loc,
				curNspace(), curStruct(), curScope(), objField->name );
	}

	LangExpr *expr = LangExpr::cons( LangTerm::cons( loc, LangTerm::ConstructType,
			varRef, objField, typeRef, fieldInitVect, constructor ) );

	/* Check for redeclaration. */
	if ( objField != 0 ) {
		if ( curScope()->checkRedecl( objField->name ) != 0 ) {
			error( objField->loc ) << "variable " << objField->name <<
					" redeclared" << endp;
		}

		/* Insert it into the field map. */
		objField->typeRef = typeRef;
		curScope()->insertField( objField->name, objField );
	}

	return expr;
}

LangExpr *BaseParser::match( const InputLoc &loc, LangVarRef *varRef,
		PatternItemList *list )
{
	Pattern *pattern = Pattern::cons( loc, curNspace(),
			list, pd->nextPatConsId++ );
	pd->patternList.append( pattern );

	LangExpr *expr = LangExpr::cons( LangTerm::cons( 
			InputLoc(), LangTerm::MatchType, varRef, pattern ) );

	return expr;
}

LangStmt *BaseParser::varDef( ObjectField *objField,
		LangExpr *expr, LangStmt::Type assignType )
{
	LangStmt *stmt = 0;

	/* Check for redeclaration. */
	if ( curScope()->checkRedecl( objField->name ) != 0 ) {
		error( objField->loc ) << "variable " << objField->name <<
				" redeclared" << endp;
	}

	/* Insert it into the field map. */
	curScope()->insertField( objField->name, objField );

	//cout << "var def " << $1->objField->name << endl;

	if ( expr != 0 ) {
		LangVarRef *varRef = LangVarRef::cons( objField->loc,
				curNspace(), curStruct(), curScope(), objField->name );

		stmt = LangStmt::cons( objField->loc, assignType, varRef, expr );
	}

	return stmt;
}

LangExpr *BaseParser::require( const InputLoc &loc,
		LangVarRef *varRef, PatternItemList *list )
{
	Pattern *pattern = Pattern::cons( loc, curNspace(),
			list, pd->nextPatConsId++ );
	pd->patternList.append( pattern );

	LangExpr *expr = LangExpr::cons( LangTerm::cons(
			InputLoc(), LangTerm::MatchType, varRef, pattern ) );
	return expr;
}

void BaseParser::structVarDef( const InputLoc &loc, ObjectField *objField )
{
	ObjectDef *object;
	if ( curStruct() == 0 )
		error(loc) << "internal error: no context stack items found" << endp;

	StructDef *structDef = curStruct();
	object = structDef->objectDef;

	if ( object->rootScope->checkRedecl( objField->name ) != 0 )
		error(objField->loc) << "object field renamed" << endp;

	object->rootScope->insertField( objField->name, objField );
}

void BaseParser::structHead( const InputLoc &loc, Namespace *inNspace,
		const String &data, ObjectDef::Type objectType )
{
	ObjectDef *objectDef = ObjectDef::cons( objectType,
			data, pd->nextObjectId++ ); 

	StructDef *context = new StructDef( loc, data, objectDef );
	structStack.push( context );

	inNspace->structDefList.append( context );

	/* Make the namespace for the struct. */
	createNamespace( loc, data );
}

StmtList *BaseParser::appendStatement( StmtList *stmtList, LangStmt *stmt )
{
	if ( stmt != 0 )
		stmtList->append( stmt );
	return stmtList;
}

ParameterList *BaseParser::appendParam( ParameterList *paramList, ObjectField *objField )
{
	paramList->append( objField );
	return paramList;
}

ObjectField *BaseParser::addParam( const InputLoc &loc,
		ObjectField::Type type, TypeRef *typeRef, const String &name )
{
	ObjectField *objField = ObjectField::cons( loc, type, typeRef, name );
	return objField;
}

PredDecl *BaseParser::predTokenName( const InputLoc &loc, NamespaceQual *qual,
		const String &data )
{
	TypeRef *typeRef = TypeRef::cons( loc, qual, data );
	PredDecl *predDecl = new PredDecl( typeRef, pd->predValue );
	return predDecl;
}

PredDecl *BaseParser::predTokenLit( const InputLoc &loc, const String &data,
		NamespaceQual *nspaceQual )
{
	PdaLiteral *literal = new PdaLiteral( loc, data );
	TypeRef *typeRef = TypeRef::cons( loc, nspaceQual, literal );
	PredDecl *predDecl = new PredDecl( typeRef, pd->predValue );
	return predDecl;
}

void BaseParser::alias( const InputLoc &loc, const String &data, TypeRef *typeRef )
{
	Namespace *nspace = curNspace();
	TypeAlias *typeAlias = new TypeAlias( loc, nspace, data, typeRef );
	nspace->typeAliasList.append( typeAlias );
}

void BaseParser::precedenceStmt( PredType predType, PredDeclList *predDeclList )
{
	while ( predDeclList->length() > 0 ) {
		PredDecl *predDecl = predDeclList->detachFirst();
		predDecl->predType = predType;
		pd->predDeclList.append( predDecl );
	}
	pd->predValue++;
}

void BaseParser::pushScope()
{
	scopeTop = curLocalFrame()->pushScope( curScope() );
}

void BaseParser::popScope()
{
	scopeTop = curScope()->parentScope;
}
