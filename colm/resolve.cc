/*
 *  Copyright 2009-2011 Adrian Thurston <thurston@complang.org>
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
#include "fsmrun.h"
#include <iostream>
#include <assert.h>

using std::cout;
using std::cerr;
using std::endl;

UniqueType *TypeRef::lookupTypePart( ParseData *pd, 
		NamespaceQual *qual, const String &name )
{
	/* Lookup up the qualifiction and then the name. */
	Namespace *nspace = qual->getQual( pd );

	if ( nspace == 0 )
		error(loc) << "do not have region for resolving reference" << endp;

	while ( nspace != 0 ) {
		/* Search for the token in the region by name. */
		SymbolMapEl *inDict = nspace->symbolMap.find( name );
		if ( inDict != 0 ) {
			long typeId = ( isPtr ? TYPE_PTR : ( isRef ? TYPE_REF : TYPE_TREE ) );
			return pd->findUniqueType( typeId, inDict->value );
		}

		nspace = nspace->parentNamespace;
	}

	error(loc) << "unknown type in typeof expression" << endp;
	return 0;
}

UniqueType *TypeRef::lookupType( ParseData *pd )
{
	if ( uniqueType != 0 )
		return uniqueType;

//	cout << __PRETTY_FUNCTION__ << " " << typeName.data << " " << this << endl;
	
	if ( iterDef != 0 )
		uniqueType = pd->findUniqueType( TYPE_ITER, iterDef );
	else if ( factor != 0 )
		uniqueType = pd->findUniqueType( TYPE_TREE, factor->langEl );
	else {
		String name = typeName;
		if ( repeatType == RepeatOpt )
			name.setAs( 32, "_opt_%s", name.data );
		else if ( repeatType == RepeatRepeat )
			name.setAs( 32, "_repeat_%s", name.data );
		else if ( repeatType == RepeatList )
			name.setAs( 32, "_list_%s", name.data );

		/* Not an iterator. May be a reference. */
		uniqueType = lookupTypePart( pd, nspaceQual, name );
	}

	return uniqueType;
}


void TypeRef::resolve( ParseData *pd ) const
{
	/* Look for the production's associated region. */
	Namespace *nspace = nspaceQual->getQual( pd );

	if ( nspace == 0 )
		error(loc) << "do not have namespace for resolving reference" << endp;
	
	if ( repeatType == RepeatRepeat ) {
		/* If the factor is a repeat, create the repeat element and link the
		 * factor to it. */
		String repeatName( 128, "_repeat_%s", typeName.data );

	  	SymbolMapEl *inDict = nspace->symbolMap.find( repeatName );
	    if ( inDict == 0 )
			pd->makeRepeatProd( nspace, repeatName, nspaceQual, typeName );
	}
	else if ( repeatType == RepeatList ) {
		/* If the factor is a repeat, create the repeat element and link the
		 * factor to it. */
		String listName( 128, "_list_%s", typeName.data );

		SymbolMapEl *inDict = nspace->symbolMap.find( listName );
	    if ( inDict == 0 )
			pd->makeListProd( nspace, listName, nspaceQual, typeName );
	}
	else if ( repeatType == RepeatOpt ) {
		/* If the factor is an opt, create the opt element and link the factor
		 * to it. */
		String optName( 128, "_opt_%s", typeName.data );

		SymbolMapEl *inDict = nspace->symbolMap.find( optName );
	    if ( inDict == 0 )
			pd->makeOptProd( nspace, optName, nspaceQual, typeName );
	}
}

void LangTerm::resolve( ParseData *pd ) const
{
	switch ( type ) {
		case ConstructType:
			typeRef->resolve( pd );
			typeRef->lookupType( pd );

			/* Evaluate the initialization expressions. */
			if ( fieldInitArgs != 0 ) {
				for ( FieldInitVect::Iter pi = *fieldInitArgs; pi.lte(); pi++ )
					(*pi)->expr->resolve( pd );
			}
			break;
		case VarRefType:
			break;

		case MakeTreeType:
		case MakeTokenType:
		case MethodCallType:
			if ( args != 0 ) {
				for ( ExprVect::Iter pe = *args; pe.lte(); pe++ )
					(*pe)->resolve( pd );
			}
			break;

		case NumberType:
		case StringType:
		case MatchType:
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

		case ParseType:
		case ParseStopType:
			typeRef->resolve( pd );
			typeRef->lookupType( pd );
			break;

		case EmbedStringType:
			break;
	}
}

void LangVarRef::resolve( ParseData *pd ) const
{

}

void LangExpr::resolve( ParseData *pd ) const
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

void LangStmt::resolveAccumItems( ParseData *pd ) const
{
	/* Assign bind ids to the variables in the replacement. */
	for ( ReplItemList::Iter item = *accumText->list; item.lte(); item++ ) {
		varRef->resolve( pd );

		switch ( item->type ) {
		case ReplItem::FactorType:
			break;
		case ReplItem::InputText:
			break;
		case ReplItem::ExprType:
			item->expr->resolve( pd );
			break;
		}
	}
}

void LangStmt::resolve( ParseData *pd ) const
{
	switch ( type ) {
		case PrintType: 
		case PrintXMLACType:
		case PrintXMLType:
		case PrintStreamType: {
			/* Push the args backwards. */
			for ( ExprVect::Iter pex = exprPtrVect->last(); pex.gtb(); pex-- )
				(*pex)->resolve( pd );
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
//			cout << "Assign Type" << endl;
			expr->resolve( pd );
			break;
		}
		case ForIterType: {
			typeRef->lookupType( pd );

			/* Evaluate and push the arguments. */
			langTerm->resolve( pd );

			/* Compile the contents. */
			for ( StmtList::Iter stmt = *stmtList; stmt.lte(); stmt++ )
				stmt->resolve( pd );

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
		case AccumType: {
			//for ( )
			break;
		}
	}
}

void ObjectDef::resolve( ParseData *pd )
{
	for ( ObjFieldList::Iter fli = *objFieldList; fli.lte(); fli++ ) {
		ObjField *field = fli->value;

		if ( field->typeRef != 0 ) {
			field->typeRef->lookupType( pd );
		}
	}
}

void CodeBlock::resolve( ParseData *pd ) const
{
	if ( localFrame != 0 )
		localFrame->resolve( pd );

	for ( StmtList::Iter stmt = *stmtList; stmt.lte(); stmt++ )
		stmt->resolve( pd );
}

void ParseData::resolveFunction( Function *func )
{
	CodeBlock *block = func->codeBlock;
	block->resolve( this );
}

void ParseData::resolveUserIter( Function *func )
{
	CodeBlock *block = func->codeBlock;
	block->resolve( this );
}

void ParseData::resolvePreEof( TokenRegion *region )
{
	CodeBlock *block = region->preEofBlock;
	block->resolve( this );
}

void ParseData::resolveRootBlock()
{
	rootLocalFrame->resolve( this );

	CodeBlock *block = rootCodeBlock;
	block->resolve( this );
}

void ParseData::resolveTranslateBlock( KlangEl *langEl )
{
	CodeBlock *block = langEl->transBlock;
	block->resolve( this );
}

void ParseData::resolveReductionCode( Definition *prod )
{
	CodeBlock *block = prod->redBlock;
	block->resolve( this );
}

void ParseData::resolveParseTree()
{
	/* Compile functions. */
	for ( FunctionList::Iter f = functionList; f.lte(); f++ ) {
		if ( f->isUserIter )
			resolveUserIter( f );
		else
			resolveFunction( f );
		
		if ( f->typeRef != 0 ) 
			f->typeRef->lookupType( this );

		for ( ParameterList::Iter param = *f->paramList; param.lte(); param++ )
			param->typeRef->lookupType( this );
	}

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


void ParseData::resolveUses()
{
	for ( LelList::Iter lel = langEls; lel.lte(); lel++ ) {
		if ( lel->objectDefUses != 0 ) {
			/* Look for the production's associated region. */
			Namespace *nspace = lel->objectDefUsesQual->getQual( this );

			if ( nspace == 0 )
				error() << "do not have namespace for resolving reference" << endp;
	
			/* Look up the language element in the region. */
			KlangEl *langEl = getKlangEl( this, nspace, lel->objectDefUses );
			lel->objectDef = langEl->objectDef;
		}
	}
}

void ParseData::resolveLiteralFactor( PdaFactor *fact )
{
	/* Interpret escape sequences and remove quotes. */
	bool unusedCI;
	String interp;
	prepareLitString( interp, unusedCI, fact->literal->token.data, 
			fact->literal->token.loc );

	//cerr << "resolving literal: " << fact->literal->token << endl;

	/* Look for the production's associated region. */
	Namespace *nspace = fact->nspaceQual->getQual( this );

	if ( nspace == 0 )
		error(fact->loc) << "do not have region for resolving literal" << endp;

	LiteralDictEl *ldel = nspace->literalDict.find( interp );
	if ( ldel == 0 )
		cerr << "could not resolve literal: " << fact->literal->token << endp;

	TokenDef *tokenDef = ldel->value;
	fact->langEl = tokenDef->token;
}

void ParseData::resolveReferenceFactor( PdaFactor *fact )
{
	/* Look for the production's associated region. */
	Namespace *nspace = fact->nspaceQual->getQual( this );

	if ( nspace == 0 )
		error(fact->loc) << "do not have namespace for resolving reference" << endp;
	
	fact->nspace = nspace;

	/* Look up the language element in the region. */
	KlangEl *langEl = getKlangEl( this, nspace, fact->refName );

	if ( fact->repeatType == RepeatRepeat ) {
		/* If the factor is a repeat, create the repeat element and link the
		 * factor to it. */
		String repeatName( 32, "_repeat_%s", fact->refName.data );

    	SymbolMapEl *inDict = nspace->symbolMap.find( repeatName );
	    if ( inDict != 0 )
			fact->langEl = inDict->value;
		else
			fact->langEl = makeRepeatProd( nspace, repeatName, fact->nspaceQual, fact->refName );
	}
	else if ( fact->repeatType == RepeatList ) {
		/* If the factor is a repeat, create the repeat element and link the
		 * factor to it. */
		String listName( 32, "_list_%s", fact->refName.data );

    	SymbolMapEl *inDict = nspace->symbolMap.find( listName );
	    if ( inDict != 0 )
			fact->langEl = inDict->value;
		else
			fact->langEl = makeListProd( nspace, listName, fact->nspaceQual, fact->refName );
	}
	else if ( fact->repeatType == RepeatOpt ) {
		/* If the factor is an opt, create the opt element and link the factor
		 * to it. */
		String optName( 32, "_opt_%s", fact->refName.data );

    	SymbolMapEl *inDict = nspace->symbolMap.find( optName );
	    if ( inDict != 0 )
			fact->langEl = inDict->value;
		else
			fact->langEl = makeOptProd( nspace, optName, fact->nspaceQual, fact->refName );
	}
	else {
		/* The factor is not a repeat. Link to the language element. */
		fact->langEl = langEl;
	}
}

void ParseData::resolveFactor( PdaFactor *fact )
{
	switch ( fact->type ) {
		case PdaFactor::LiteralType:
			resolveLiteralFactor( fact );
			break;
		case PdaFactor::ReferenceType:
			resolveReferenceFactor( fact );
			break;
	}
}

void ParseData::resolvePatternEls()
{
	for ( PatternList::Iter pat = patternList; pat.lte(); pat++ ) {
		for ( PatternItemList::Iter item = *pat->list; item.lte(); item++ ) {
			switch ( item->type ) {
			case PatternItem::FactorType:
				/* Use pdaFactor reference resolving. */
				resolveFactor( item->factor );
				break;
			case PatternItem::InputText:
				/* Nothing to do here. */
				break;
			}
		}
	}
}

void ParseData::resolveReplacementEls()
{
	for ( ReplList::Iter repl = replList; repl.lte(); repl++ ) {
		for ( ReplItemList::Iter item = *repl->list; item.lte(); item++ ) {
			switch ( item->type ) {
			case ReplItem::FactorType:
				/* Use pdaFactor reference resolving. */
				resolveFactor( item->factor );
				break;
			case ReplItem::InputText:
			case ReplItem::ExprType:
				break;
			}
		}
	}
}

void ParseData::resolveAccumEls()
{
	for ( AccumTextList::Iter accum = accumTextList; accum.lte(); accum++ ) {
		for ( ReplItemList::Iter item = *accum->list; item.lte(); item++ ) {
			switch ( item->type ) {
			case ReplItem::FactorType:
				resolveFactor( item->factor );
				break;
			case ReplItem::InputText:
			case ReplItem::ExprType:
				break;
			}
		}
	}
}

/* Resolves production els and computes the precedence of each prod. */
void ParseData::resolveProductionEls()
{
	for ( DefList::Iter prod = prodList; prod.lte(); prod++ ) {
		/* First resolve. */
		for ( ProdElList::Iter fact = *prod->prodElList; fact.lte(); fact++ )
			resolveFactor( fact );

		/* If there is no explicit precdence ... */
		if ( prod->predOf == 0 )  {
			/* Compute the precedence of the productions. */
			for ( ProdElList::Iter fact = prod->prodElList->last(); fact.gtb(); fact-- ) {
				/* Production inherits the precedence of the last terminal with
				 * precedence. */
				if ( fact->langEl->predType != PredNone ) {
					prod->predOf = fact->langEl;
					break;
				}
			}
		}
	}
}

void ParseData::resolveGenericTypes()
{
	for ( NamespaceList::Iter ns = namespaceList; ns.lte(); ns++ ) {
		for ( GenericList::Iter gen = ns->genericList; gen.lte(); gen++ ) {
//			cout << __PRETTY_FUNCTION__ << " " << gen->name.data << " " << gen->typeArg << endl;

			gen->utArg = gen->typeArg->lookupType( this );

			if ( gen->typeId == GEN_MAP )
				gen->keyUT = gen->keyTypeArg->lookupType( this );
		}
	}
}

