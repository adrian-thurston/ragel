/*
 *  Copyright 2001-2007 Adrian Thurston <thurston@complang.org>
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

#include <iostream>
#include <iomanip>
#include <errno.h>
#include <stdlib.h>

/* Parsing. */
#include "colm.h"
#include "parsedata.h"
#include "pdacodegen.h"
#include "pdarun.h"
#include "redfsm.h"
#include "fsmcodegen.h"
#include "redbuild.h"
#include "fsmrun.h"

/* Dumping the fsm. */
#include "mergesort.h"

#define pt(var) ((ParseTree*)(var))

using namespace std;

char startDefName[] = "start";

/* Count the transitions in the fsm by walking the state list. */
int countTransitions( PdaGraph *fsm )
{
	int numTrans = 0;
	PdaState *state = fsm->stateList.head;
	while ( state != 0 ) {
		numTrans += state->transMap.length();
		state = state->next;
	}
	return numTrans;
}

KlangEl::KlangEl( Namespace *nspace, const String &name, Type type )
:
	nspace(nspace),
	name(name),
	lit(name),
	type(type),
	id(-1),
	isUserTerm(false),
	isContext(false),
	displayString(0),
	numAppearances(0),
	commit(false),
	ignore(false),
	reduceFirst(false),
	isLiteral(false),
	isRepeat(false),
	isList(false),
	isOpt(false),
	parseStop(false),
	isEOF(false),
	tokenDef(0),
	rootDef(0),
	termDup(0),
	eofLel(0),
	pdaGraph(0),
	pdaTables(0),
	transBlock(0),
	objectDef(0),
	thisSize(0),
	ofiOffset(0),
	generic(0),
	parserId(-1),
	predType(PredNone),
	predValue(0)
{
}
 
PdaGraph *ProdElList::walk( ParseData *pd )
{
	PdaGraph *prodFsm = new PdaGraph();
	PdaState *last = prodFsm->addState();
	prodFsm->setStartState( last );

	int prodLength = 0;
	for ( Iter prodEl = first(); prodEl.lte(); prodEl++, prodLength++ ) {
		//PdaGraph *itemFsm = prodEl->walk( pd );
		long value = 0;

		value = prodEl->langEl->id;

		PdaState *newState = prodFsm->addState();
		PdaTrans *newTrans = prodFsm->appendNewTrans( last, newState, value, value );

		newTrans->isShift = true;
		newTrans->shiftPrior = prodEl->priorVal;
		//cerr << "PRIOR VAL: " << newTrans->shiftPrior << endl;

		if ( prodEl->commit ) {
			//cout << "COMMIT: inserting commit of length: " << pd->prodLength << endl;
			/* Insert the commit into transitions out of last */
			for ( TransMap::Iter trans = last->transMap; trans.lte(); trans++ )
				trans->value->commits.insert( prodLength );
		}

		last = newState;
	}

	/* Make the last state the final state. */
	prodFsm->setFinState( last );
	return prodFsm;
}


KlangEl *getKlangEl( ParseData *pd, Namespace *nspace, const String &data )
{
    /* If the id is already in the dict, it will be placed in last found. If
     * it is not there then it will be inserted and last found will be set to it. */
    SymbolMapEl *inDict = nspace->symbolMap.find( data );
    if ( inDict == 0 ) {
        /* Language element not there. Make the new lang el and insert.. */
        KlangEl *langEl = new KlangEl( nspace, data, KlangEl::Unknown );
        inDict = nspace->symbolMap.insert( langEl->name, langEl );
        pd->langEls.append( langEl );
    }
    return inDict->value;
}

ProdElList *makeProdElList( KlangEl *langEl )
{
	ProdElList *prodElList = new ProdElList();
	prodElList->append( new PdaFactor( InputLoc(), langEl ) );
	prodElList->tail->langEl = langEl;
	return prodElList;
}

void ParseData::makeDefinitionNames()
{
	for ( LelList::Iter lel = langEls; lel.lte(); lel++ ) {
		int prodNum = 1;
		for ( LelDefList::Iter def = lel->defList; def.lte(); def++ ) {
			def->data.setAs( lel->name.length() + 32, "%s-%i", 
					lel->name.data, prodNum++ );
		}
	}
}

/* Make sure there there are no language elements whose type is unkonwn. This
 * can happen when an id is used on the rhs of a definition but is not defined
 * as anything. */
void ParseData::noUndefindKlangEls()
{
	for ( LelList::Iter lel = langEls; lel.lte(); lel++ ) {
		if ( lel->type == KlangEl::Unknown )
			error() << "'" << lel->name << "' was not defined as anything" << endp;
	}
}

void ParseData::makeKlangElIds()
{
	/* Make the "stream" language element */
	streamKlangEl = new KlangEl( rootNamespace, strdup("stream"), KlangEl::Term );
	langEls.prepend( streamKlangEl );
	SymbolMapEl *streamMapEl = rootNamespace->symbolMap.insert( 
			streamKlangEl->name, streamKlangEl );
	assert( streamMapEl != 0 );

	/* Make the "str" language element */
	strKlangEl = new KlangEl( rootNamespace, strdup("str"), KlangEl::Term );
	langEls.prepend( strKlangEl );
	SymbolMapEl *stringMapEl = rootNamespace->symbolMap.insert( 
			strKlangEl->name, strKlangEl );
	assert( stringMapEl != 0 );

	/* Make the "int" language element */
	intKlangEl = new KlangEl( rootNamespace, strdup("int"), KlangEl::Term );
	langEls.prepend( intKlangEl );
	SymbolMapEl *integerMapEl = rootNamespace->symbolMap.insert( 
			intKlangEl->name, intKlangEl );
	assert( integerMapEl != 0 );

	/* Make the "bool" language element */
	boolKlangEl = new KlangEl( rootNamespace, strdup("bool"), KlangEl::Term );
	langEls.prepend( boolKlangEl );
	SymbolMapEl *boolMapEl = rootNamespace->symbolMap.insert( 
			boolKlangEl->name, boolKlangEl );
	assert( boolMapEl != 0 );

	/* Make the "ptr" language element */
	ptrKlangEl = new KlangEl( rootNamespace, strdup("ptr"), KlangEl::Term );
	langEls.prepend( ptrKlangEl );
	SymbolMapEl *ptrMapEl = rootNamespace->symbolMap.insert( 
			ptrKlangEl->name, ptrKlangEl );
	assert( ptrMapEl != 0 );

	/* Make a "_notoken" language element. This element is used when a
	 * generation action fails to generate anything, but there is reverse code
	 * that needs to be associated with a language element. This allows us to
	 * always associate reverse code with the first language element produced
	 * after a generation action. */
	noTokenKlangEl = new KlangEl( rootNamespace, strdup("_notoken"), KlangEl::Term );
	noTokenKlangEl->ignore = true;
	langEls.prepend( noTokenKlangEl );
	SymbolMapEl *noTokenMapEl = rootNamespace->symbolMap.insert( 
			noTokenKlangEl->name, noTokenKlangEl );
	assert( noTokenMapEl != 0 );

	/* Make the EOF language element. */
	eofKlangEl = 0;
//	eofKlangEl = new KlangEl( rootNamespace, strdup("_eof"), KlangEl::Term );
//	langEls.prepend( eofKlangEl );
//	SymbolMapEl *eofMapEl = rootNamespace->symbolMap.insert( eofKlangEl->name, eofKlangEl );
//	assert( eofMapEl != 0 );

	/* Make the "any" language element */
	anyKlangEl = new KlangEl( rootNamespace, strdup("any"), KlangEl::NonTerm );
	langEls.prepend( anyKlangEl );
	SymbolMapEl *anyMapEl = rootNamespace->symbolMap.insert( anyKlangEl->name, anyKlangEl );
	assert( anyMapEl != 0 );

	/* Make terminal language elements corresponding to each nonterminal in
	 * the grammar. */
	for ( LelList::Iter lel = langEls; lel.lte(); lel++ ) {
		if ( lel->type == KlangEl::NonTerm ) {
			String name( lel->name.length() + 5, "_T_%s", lel->name.data );
			KlangEl *termDup = new KlangEl( lel->nspace, name, KlangEl::Term );

			/* Give the dup the attributes of the nonterminal. This ensures
			 * that the attributes are allocated when patterns and
			 * constructors are parsed. */
			termDup->objectDef = lel->objectDef;

			langEls.append( termDup );
			lel->termDup = termDup;
			termDup->termDup = lel;
		}
	}

	/* Make eof language elements for each user terminal. This is a bit excessive and
	 * need to be reduced to the ones that we need parsers for, but we don't know that yet.
	 * Another pass before this one is needed. */
	for ( LelList::Iter lel = langEls; lel.lte(); lel++ ) {
		if ( lel->eofLel == 0 &&
				lel != eofKlangEl &&
				lel != errorKlangEl &&
				lel != noTokenKlangEl )
		{
			String name( lel->name.length() + 5, "_eof_%s", lel->name.data );
			KlangEl *eofLel = new KlangEl( lel->nspace, name, KlangEl::Term );

			langEls.append( eofLel );
			lel->eofLel = eofLel;
			eofLel->eofLel = lel;
			eofLel->isEOF = true;
		}
	}

	/* The first id 0 is reserved for the stack sentinal. A negative id means
	 * error to the parsing function, inducing backtracking. */
	nextSymbolId = 1;

	/* First pass assigns to the user terminals. */
	for ( LelList::Iter lel = langEls; lel.lte(); lel++ ) {
		/* Must be a term, and not any of the special reserved terminals.
		 * Remember if the non terminal is a user non terminal. */
		if ( lel->type == KlangEl::Term && 
				!lel->isEOF && 
				lel != errorKlangEl &&
				lel != noTokenKlangEl )
		{
			lel->isUserTerm = true;
			lel->id = nextSymbolId++;
		}
	}

	//eofKlangEl->id = nextSymbolId++;
	for ( LelList::Iter lel = langEls; lel.lte(); lel++ ) {
		/* Must be a term, and not any of the special reserved terminals.
		 * Remember if the non terminal is a user non terminal. */
		if ( lel->isEOF )
			lel->id = nextSymbolId++;
	}

	/* Next assign to the eof notoken, which we always create. */
	noTokenKlangEl->id = nextSymbolId++;

	/* Possibly assign to the error language element. */
	if ( errorKlangEl != 0 )
		errorKlangEl->id = nextSymbolId++;

	/* Save this for the code generation. */
	firstNonTermId = nextSymbolId;

	/* A third and final pass assigns to everything else. */
	for ( LelList::Iter lel = langEls; lel.lte(); lel++ ) {
		/* Anything else not yet assigned gets assigned now. */
		if ( lel->id < 0 )
			lel->id = nextSymbolId++;
	}

	assert( ptrKlangEl->id == LEL_ID_PTR );
	assert( boolKlangEl->id == LEL_ID_BOOL );
	assert( intKlangEl->id == LEL_ID_INT );
	assert( strKlangEl->id == LEL_ID_STR );
	assert( streamKlangEl->id == LEL_ID_STREAM );
}

void ParseData::makeKlangElNames()
{
	for ( LelList::Iter lel = langEls; lel.lte(); lel++ ) {
		lel->fullName = lel->name;
		lel->fullLit = lel->lit;

		Namespace *nspace = lel->nspace;
		while ( nspace != 0 ) {
			if ( nspace == defaultNamespace || nspace == rootNamespace )
				break;
			lel->fullName = nspace->name + "_" + lel->fullName;
			nspace = nspace->parentNamespace;
		}

		nspace = lel->nspace;
		while ( nspace != 0 ) {
			if ( nspace == defaultNamespace || nspace == rootNamespace )
				break;
			lel->fullLit = nspace->name + "::" + lel->fullLit;
			nspace = nspace->parentNamespace;
		}
	}
}

/* Set up dot sets, shift info, and prod sets. */
void ParseData::makeProdFsms()
{
	/* There are two items in the index for each production (high and low). */
	int indexLen = prodList.length() * 2;
	dotItemIndex.setAsNew( indexLen );
	int dsiLow = 0, indexPos = 0;

	/* Build FSMs for all production language elements. */
	for ( DefList::Iter prod = prodList; prod.lte(); prod++ )
		prod->fsm = prod->prodElList->walk( this );

	makeNonTermFirstSets();
	makeFirstSets();

	/* Build FSMs for all production language elements. */
	for ( DefList::Iter prod = prodList; prod.lte(); prod++ ) {
		if ( addUniqueEmptyProductions ) {
			/* This must be re-implemented. */
			assert( false );
			//if ( !prod->isLeftRec && prod->uniqueEmptyLeader != 0 ) {
			//	PdaGraph *emptyLeader = prod->uniqueEmptyLeader->walk( this );
			//	emptyLeader->concatOp( prod->fsm );
			//	prod->fsm = emptyLeader;
			//}
		}

		/* Compute the machine's length. */
		prod->fsmLength = prod->fsm->fsmLength( );

		/* Productions have a unique production id for each final state.
		 * This lets us use a production length specific to each final state.
		 * Start states are always isolated therefore if the start state is
		 * final then reductions from it will always have a fixed production
		 * length. This is a simple method for determining the length
		 * of zero-length derivations when reducing. */

		/* Number of dot items needed for the production is elements + 1
		 * because the dot can be before the first and after the last element. */
		int numForProd = prod->fsm->stateList.length() + 1;

		/* Set up the low and high values in the index for this production. */
		dotItemIndex.data[indexPos].key = dsiLow;
		dotItemIndex.data[indexPos].value = prod;
		dotItemIndex.data[indexPos+1].key = dsiLow + numForProd - 1;
		dotItemIndex.data[indexPos+1].value = prod;

		int dsi = dsiLow;
		for ( PdaStateList::Iter state = prod->fsm->stateList; state.lte(); state++, dsi++ ) {
			/* All transitions are shifts. */
			for ( TransMap::Iter out = state->transMap; out.lte(); out++ )
				assert( out->value->isShift );

			state->dotSet.insert( dsi );
		}

		/* Move over the production. */
		dsiLow += numForProd;
		indexPos += 2;

		if ( prod->prodCommit ) {
			for ( PdaStateSet::Iter fin = prod->fsm->finStateSet; fin.lte(); fin++ ) {
				int length = prod->fsmLength;
				//cerr << "PENDING COMMIT IN FINAL STATE of " << prod->prodId <<
				//		" with len: " << length << endl;
				(*fin)->pendingCommits.insert( ProdIdPair( prod->prodId, length ) );
			}
		}
	}

	/* Make the final state specific prod id to prod id mapping. */
	prodIdIndex = new Definition*[prodList.length()];
	for ( DefList::Iter prod = prodList; prod.lte(); prod++ )
		prodIdIndex[prod->prodId] = prod;
}

/* Want the first set of over src. If the first set contains epsilon, go over
 * it and over tab. If overSrc is the end of the production, find the follow
 * from the table, taking only the characters on which the parent is reduced.
 * */
void ParseData::findFollow( AlphSet &result, PdaState *overTab, 
		PdaState *overSrc, Definition *parentDef )
{
	if ( overSrc->isFinState() ) {
		assert( overSrc->transMap.length() == 0 );

		/* At the end of the production. Turn to the table. */
		long redCode = makeReduceCode( parentDef->prodId, false );
		for ( TransMap::Iter tabTrans = overTab->transMap; tabTrans.lte(); tabTrans++ ) {
			for ( ActDataList::Iter adl = tabTrans->value->actions; adl.lte(); adl++ ) {
				if ( *adl == redCode )
					result.insert( tabTrans->key );
			}
		}
	}
	else {
		/* Get the first set of the item. If the first set contains epsilon
		 * then move over overSrc and overTab and recurse. */
		assert( overSrc->transMap.length() == 1 );
		TransMap::Iter pastTrans = overSrc->transMap;

		KlangEl *langEl = langElIndex[pastTrans->key];
		if ( langEl != 0 && langEl->type == KlangEl::NonTerm ) {
			bool hasEpsilon = false;
			for ( LelDefList::Iter def = langEl->defList; def.lte(); def++ ) {
				result.insert( def->firstSet );

				if ( def->firstSet.find( -1 ) )
					hasEpsilon = true;
			}

			/* Find the equivalent state in the parser. */
			if ( hasEpsilon ) {
				PdaTrans *tabTrans = overTab->findTrans( pastTrans->key );
				findFollow( result, tabTrans->toState, 
						pastTrans->value->toState, parentDef );
			}

			/* Now possibly the dup. */
			if ( langEl->termDup != 0 )
				result.insert( langEl->termDup->id );
		}
		else {
			result.insert( pastTrans->key );
		}
	}
}

PdaState *ParseData::followProd( PdaState *tabState, PdaState *prodState )
{
	while ( prodState->transMap.length() == 1 ) {
		TransMap::Iter prodTrans = prodState->transMap;
		PdaTrans *tabTrans = tabState->findTrans( prodTrans->key );
		prodState = prodTrans->value->toState;
		tabState = tabTrans->toState;
	}
	return tabState;
}

void ParseData::trySetTime( PdaTrans *trans, long code, long &time )
{
	/* Find the item. */
	for ( ActDataList::Iter adl = trans->actions; adl.lte(); adl++ ) {
		if ( *adl == code ) {
			/* If the time of the shift is not already set, set it. */
			if ( trans->actOrds[adl.pos()] == 0 ) {
				//cerr << "setting time: state = " << tabState->stateNum 
				//		<< ", trans = " << tabTrans->lowKey
				//		<< ", time = " << time << endl;
				trans->actOrds[adl.pos()] = time++;
			}
			break;
		}
	}
}

/* Go down a defintiion and then handle the follow actions. */
void ParseData::pdaOrderFollow( KlangEl *rootEl, PdaState *tabState, 
		PdaTrans *tabTrans, PdaTrans *srcTrans, Definition *parentDef, 
		Definition *definition, long &time )
{
	/* We need the follow from tabState/srcState over the defintion we are
	 * currently processing. */
	PdaState *overTab = tabTrans->toState;
	PdaState *overSrc = srcTrans->toState;

	AlphSet alphSet;
	if ( parentDef == rootEl->rootDef )
		alphSet.insert( rootEl->eofLel->id );
	else
		findFollow( alphSet, overTab, overSrc, parentDef );		

	/* Now follow the production to find out where it expands to. */
	PdaState *expandToState = followProd( tabState, definition->fsm->startState );

	/* Find the reduce item. */
	long redCode = makeReduceCode( definition->prodId, false );

	for ( TransMap::Iter tt = expandToState->transMap; tt.lte(); tt++ ) {
		if ( alphSet.find( tt->key ) ) {
			trySetTime( tt->value, redCode, time );
	
			/* If the items token region is not recorded in the state, do it now. */
			addRegion( expandToState, tt->key );
		}
	}
}

bool regionVectHas( RegionVect &regVect, TokenRegion *region )
{
	for ( RegionVect::Iter trvi = regVect; trvi.lte(); trvi++ ) {
		if ( *trvi == region )
			return true;
	}
	return false;
}

void ParseData::addRegion( PdaState *tabState, long pdaKey )
{
	KlangEl *klangEl = langElIndex[pdaKey];
	if ( klangEl != 0 && klangEl->type == KlangEl::Term ) {
		TokenRegion *region = 0;

		/* If it is not the eof, then use the region associated 
		 * with the token definition. */
		if ( !klangEl->isEOF && klangEl->tokenDef != 0 )
			region = klangEl->tokenDef->tokenRegion;

		if ( region != 0 && !regionVectHas( tabState->regions, region ) )
			tabState->regions.append( region );
	}
}

#if 0
    orderState( tabState, prodState, time ):
        if not tabState.dotSet.find( prodState.dotID )
            tabState.dotSet.insert( prodState.dotID )
            tabTrans = tabState.findMatchingTransition( prodState.getTransition() )

            if tabTrans is NonTerminal:
                for production in tabTrans.nonTerm.prodList:
                    orderState( tabState, production.startState, time )

                    for all expandToState in tabTrans.expandToStates:
                        for all followTrans in expandToState.transList 
                            reduceAction = findAction( production.reduction )
                            if reduceAction.time is unset:
                                reduceAction.time = time++
                            end
                        end
                    end
                end
            end

            shiftAction = tabTrans.findAction( shift )
            if shiftAction.time is unset:
                shiftAction.time = time++
            end

            orderState( tabTrans.toState, prodTrans.toState, time )
        end
    end

    orderState( parseTable.startState, startProduction.startState, 1 )
#endif

void ParseData::pdaOrderProd( KlangEl *rootEl, PdaState *tabState, 
	PdaState *srcState, Definition *parentDef, long &time )
{
	assert( srcState->dotSet.length() == 1 );
	if ( tabState->dotSet2.find( srcState->dotSet[0] ) )
		return;
	tabState->dotSet2.insert( srcState->dotSet[0] );
	
	assert( srcState->transMap.length() == 0 || srcState->transMap.length() == 1 );

	if ( srcState->transMap.length() == 1 ) { 
		TransMap::Iter srcTrans = srcState->transMap;

		/* Find the equivalent state in the parser. */
		PdaTrans *tabTrans = tabState->findTrans( srcTrans->key );

		/* Recurse into the transition if it is a non-terminal. */
		KlangEl *langEl = langElIndex[srcTrans->key];
		if ( langEl != 0 ) {
			if ( langEl->reduceFirst ) {
				/* Use a shortest match ordering for the contents of this
				 * nonterminal. Does follows for all productions first, then
				 * goes down the productions. */
				for ( LelDefList::Iter expDef = langEl->defList; expDef.lte(); expDef++ ) {
					pdaOrderFollow( rootEl, tabState, tabTrans, srcTrans->value, 
							parentDef, expDef, time );
				}
				for ( LelDefList::Iter expDef = langEl->defList; expDef.lte(); expDef++ )
					pdaOrderProd( rootEl, tabState, expDef->fsm->startState, expDef, time );
				
			}
			else {
				/* The default action ordering. For each prod, goes down the
				 * prod then sets the follow before going to the next prod. */
				for ( LelDefList::Iter expDef = langEl->defList; expDef.lte(); expDef++ ) {
					pdaOrderProd( rootEl, tabState, expDef->fsm->startState, expDef, time );

					pdaOrderFollow( rootEl, tabState, tabTrans, srcTrans->value, 
							parentDef, expDef, time );
				}
			}
		}

		trySetTime( tabTrans, SHIFT_CODE, time );

		/* Now possibly for the dup. */
		if ( langEl != 0 && langEl->termDup != 0 ) {
			PdaTrans *dupTrans = tabState->findTrans( langEl->termDup->id );
			trySetTime( dupTrans, SHIFT_CODE, time );
		}

		/* If the items token region is not recorded in the state, do it now. */
		addRegion( tabState, srcTrans->key );

		/* Go over one in the production. */
		pdaOrderProd( rootEl, tabTrans->toState, 
				srcTrans->value->toState, parentDef, time );
	}
}

void ParseData::pdaActionOrder( PdaGraph *pdaGraph, KlangElSet &parserEls )
{
	for ( PdaStateList::Iter state = pdaGraph->stateList; state.lte(); state++ ) {
		assert( (state->stateBits & SB_ISMARKED) == 0 );

		/* Traverse the src state's transitions. */
		long last = 0;
		for ( TransMap::Iter trans = state->transMap; trans.lte(); trans++ ) {
			if ( ! trans.first() )
				assert( last < trans->key );
			last = trans->key;
		}
	}

	/* Compute the action orderings, record the max value. */
	long time = 1;
	for ( KlangElSet::Iter pe = parserEls; pe.lte(); pe++ ) {
		PdaState *startState = (*pe)->rootDef->fsm->startState;
		pdaOrderProd( *pe, (*pe)->startState, startState, (*pe)->rootDef, time );

		/* Walk over the start lang el and set the time for shift of
		 * the eof action that completes the parse. */
		PdaTrans *overStart = (*pe)->startState->findTrans( (*pe)->id );
		PdaTrans *eofTrans = overStart->toState->findTrans( (*pe)->eofLel->id );
		eofTrans->actOrds[0] = time++;
	}

	for ( PdaStateList::Iter state = pdaGraph->stateList; state.lte(); state++ ) {
		if ( state->regions.length() == 0 ) {
			for ( TransMap::Iter tel = state->transMap; tel.lte(); tel++ ) {
				/* There are no regions and EOF leaves the state. Add the eof
				 * token region. */
				PdaTrans *trans = tel->value;
				KlangEl *lel = langElIndex[trans->lowKey];
				if ( lel != 0 && lel->isEOF )
					state->regions.append( eofTokenRegion );
			}
		}
	}

	if ( colm_log_compile ) {
		/* Warn about states with empty token region lists. */
		for ( PdaStateList::Iter state = pdaGraph->stateList; state.lte(); state++ ) {
			if ( state->regions.length() == 0 ) {
				warning() << "state has an empty token region, state: " << 
					state->stateNum << endl;
			}
		}
	}

	/* Some actions may not have an ordering. I believe these to be actions
	 * that result in a parse error and they arise because the state tables
	 * are LALR(1) but the action ordering is LR(1). LALR(1) causes some
	 * reductions that lead nowhere. */
	for ( PdaStateList::Iter state = pdaGraph->stateList; state.lte(); state++ ) {
		assert( CmpDotSet::compare( state->dotSet, state->dotSet2 ) == 0 );
		for ( TransMap::Iter tel = state->transMap; tel.lte(); tel++ ) {
			PdaTrans *trans = tel->value;
			/* Check every action has an ordering. */
			for ( ActDataList::Iter adl = trans->actOrds; adl.lte(); adl++ ) {
				if ( *adl == 0 )
					*adl = time++;
			}
		}
	}
}

void ParseData::advanceReductions( PdaGraph *pdaGraph )
{
	/* Loop all states. */
	for ( PdaStateList::Iter state = pdaGraph->stateList; state.lte(); state++ ) {
		bool outHasShift = false;
		ReductionMap outReds;
		LongSet outCommits;
		for ( TransMap::Iter out = state->transMap; out.lte(); out++ ) {
			/* Get the transition from the trans el. */
			if ( out->value->isShift )
				outHasShift = true;
			outReds.insert( out->value->reductions );
			outCommits.insert( out->value->commits );
		}

		bool inHasShift = false;
		ReductionMap inReds;
		for ( PdaTransInList::Iter in = state->inRange; in.lte(); in++ ) {
			/* Get the transition from the trans el. */
			if ( in->isShift )
				inHasShift = true;
			inReds.insert( in->reductions );
		}

		if ( !outHasShift && outReds.length() == 1 && 
				inHasShift && inReds.length() == 0 )
		{
			//cerr << "moving reduction to shift" << endl;

			/* Move the reduction to all in transitions. */
			for ( PdaTransInList::Iter in = state->inRange; in.lte(); in++ ) {
				assert( in->actions.length() == 1 );
				assert( in->actions[0] == SHIFT_CODE );
				in->actions[0] = makeReduceCode( outReds[0].key, true );
				in->afterShiftCommits.insert( outCommits );
			}

			/* 
			 * Remove all transitions out of the state.
			 */

			/* Detach out range transitions. */
			for ( TransMap::Iter trans = state->transMap; trans.lte(); trans++ ) {
				pdaGraph->detachTrans( state, trans->value->toState, trans->value );
				delete trans->value;
			}
			state->transMap.empty();

			/* Redirect all the in transitions to the actionDestState. */
			pdaGraph->inTransMove( actionDestState, state );
		}
	}

	pdaGraph->removeUnreachableStates();
}

void ParseData::sortActions( PdaGraph *pdaGraph )
{
	/* Sort the actions. */
	for ( PdaStateList::Iter state = pdaGraph->stateList; state.lte(); state++ ) {
		assert( CmpDotSet::compare( state->dotSet, state->dotSet2 ) == 0 );
		for ( TransMap::Iter tel = state->transMap; tel.lte(); tel++ ) {
			PdaTrans *trans = tel->value;

			/* Sort by the action ords. */
			ActDataList actions( trans->actions );
			ActDataList actOrds( trans->actOrds );
			ActDataList actPriors( trans->actPriors );
			trans->actions.empty();
			trans->actOrds.empty();
			trans->actPriors.empty();
			while ( actOrds.length() > 0 ) {
				int min = 0;
				for ( int i = 1; i < actOrds.length(); i++ ) {
					if ( actPriors[i] > actPriors[min] ||
							(actPriors[i] == actPriors[min] &&
							actOrds[i] < actOrds[min] ) )
					{
						min = i;
					}
				}
				trans->actions.append( actions[min] );
				trans->actOrds.append( actOrds[min] );
				trans->actPriors.append( actPriors[min] );
				actions.remove(min);
				actOrds.remove(min);
				actPriors.remove(min);
			}

			if ( branchPointInfo && trans->actions.length() > 1 ) {
				cerr << "info: branch point"
						<< " state: " << state->stateNum
						<< " trans: ";
				KlangEl *lel = langElIndex[trans->lowKey];
				if ( lel == 0 )
					cerr << (char)trans->lowKey << endl;
				else
					cerr << lel->lit << endl;

				for ( ActDataList::Iter act = trans->actions; act.lte(); act++ ) {
					switch ( *act & 0x3 ) {
					case 1: 
						cerr << "    shift" << endl;
						break;
					case 2: 
						cerr << "    reduce " << 
								prodIdIndex[(*act >> 2)]->data << endl;
						break;
					case 3:
						cerr << "    shift-reduce" << endl;
						break;
					}
				}
			}

			/* Verify that shifts of nonterminals don't have any branch
			 * points or commits. */
			if ( trans->lowKey >= firstNonTermId ) {
				if ( trans->actions.length() != 1 || 
					(trans->actions[0] & 0x3) != 1 )
				{
					error() << "TRANS ON NONTERMINAL is something "
						"other than a shift" << endl;
				}
				if ( trans->commits.length() > 0 )
					error() << "TRANS ON NONTERMINAL has a commit" << endl;
			}

			/* TODO: Shift-reduces are optimizations. Verify that
			 * shift-reduces exist only if they don't entail a conflict. */
		}
	}
}

void ParseData::reduceActions( PdaGraph *pdaGraph )
{
	/* Reduce the actions. */
	for ( PdaStateList::Iter state = pdaGraph->stateList; state.lte(); state++ ) {
		for ( TransMap::Iter tel = state->transMap; tel.lte(); tel++ ) {
			PdaTrans *trans = tel->value;
			PdaActionSetEl *inSet;

			int commitLen = trans->commits.length() > 0 ?
				trans->commits[trans->commits.length()-1] : 0;

			if ( trans->afterShiftCommits.length() > 0 ) {
				int afterShiftCommit = trans->afterShiftCommits[
					trans->afterShiftCommits.length()-1];

				if ( commitLen > 0 && commitLen+1 > afterShiftCommit )
					commitLen = ( commitLen + 1 );
				else
					commitLen = afterShiftCommit;
			}
			else {
				commitLen = commitLen * -1;
			}
			
			//if ( commitLen != 0 ) {
			//	cerr << "FINAL ACTION COMMIT LEN: " << commitLen << endl;
			//}

			pdaGraph->actionSet.insert( ActionData( trans->toState->stateNum, 
					trans->actions, commitLen ), &inSet );
			trans->actionSetEl = inSet;
		}
	}
}

void ParseData::verifyParseStopGrammar( KlangEl *langEl, PdaGraph *pdaGraph )
{
	/* Get the entry into the graph and traverse over the root. The resulting
	 * state can have eof, nothing else can. */
	PdaState *overStart = pdaGraph->followFsm( 
			langEl->startState,
			langEl->rootDef->fsm );

	/* The graph must reduce to root all on it's own. It cannot depend on
	 * require EOF. */
	for ( PdaStateList::Iter st = pdaGraph->stateList; st.lte(); st++ ) {
		if ( st == overStart )
			continue;

		for ( TransMap::Iter tr = st->transMap; tr.lte(); tr++ ) {
			if ( tr->value->lowKey == langEl->eofLel->id ) {
				/* This needs a better error message. Appears to be voodoo. */
				error() << "grammar is not usable with parse_stop" << endp;
			}
		}
	}
}

KlangEl *ParseData::predOf( PdaTrans *trans, long action )
{
	KlangEl *lel;
	if ( action == SHIFT_CODE )
		lel = langElIndex[trans->lowKey];
	else
		lel = prodIdIndex[action >> 2]->predOf;
	return lel;
}


bool ParseData::precedenceSwap( long action1, long action2, KlangEl *l1, KlangEl *l2 )
{
	bool swap = false;
	if ( l2->predValue > l1->predValue )
		swap = true;
	else if ( l1->predValue == l2->predValue ) {
		if ( l1->predType == PredLeft && action1 == SHIFT_CODE )
			swap = true;
		else if ( l1->predType == PredRight && action2 == SHIFT_CODE )
			swap = true;
	}
	return swap;
}

bool ParseData::precedenceRemoveBoth( KlangEl *l1, KlangEl *l2 )
{
	if ( l1->predValue == l2->predValue && l1->predType == PredNonassoc )
		return true;
	return false;
}

void ParseData::resolvePrecedence( PdaGraph *pdaGraph )
{
	for ( PdaStateList::Iter state = pdaGraph->stateList; state.lte(); state++ ) {
		assert( CmpDotSet::compare( state->dotSet, state->dotSet2 ) == 0 );
		
		for ( long t = 0; t < state->transMap.length(); /* increment at end */ ) {
			PdaTrans *trans = state->transMap[t].value;

again:
			/* Find action with precedence. */
			for ( int i = 0; i < trans->actions.length(); i++ ) {
				KlangEl *li = predOf( trans, trans->actions[i] );
					
				if ( li != 0 && li->predType != PredNone ) {
					/* Find another action with precedence. */
					for ( int j = i+1; j < trans->actions.length(); j++ ) {
						KlangEl *lj = predOf( trans, trans->actions[j] );

						if ( lj != 0 && lj->predType != PredNone ) {
							/* Conflict to check. */
							bool swap = precedenceSwap( trans->actions[i], 
									trans->actions[j], li, lj );
							
							if ( swap ) {
								long t = trans->actions[i];
								trans->actions[i] = trans->actions[j];
								trans->actions[j] = t;
							}

							trans->actions.remove( j );
							if ( precedenceRemoveBoth( li, lj ) )
								trans->actions.remove( i );

							goto again;
						}
					}
				}
			}

			/* If there are still actions then move to the next one. If not,
			 * (due to nonassoc) then remove the transition. */
			if ( trans->actions.length() > 0 )
				t += 1;
			else
				state->transMap.vremove( t );
		}
	}
}

void ParseData::analyzeMachine( PdaGraph *pdaGraph, KlangElSet &parserEls )
{
	pdaGraph->maxState = pdaGraph->stateList.length() - 1;
	pdaGraph->maxLelId = nextSymbolId - 1;
	pdaGraph->maxOffset = pdaGraph->stateList.length() * pdaGraph->maxLelId;

	for ( PdaStateList::Iter state = pdaGraph->stateList; state.lte(); state++ ) {
		for ( TransMap::Iter trans = state->transMap; trans.lte(); trans++ ) {
			if ( trans->value->isShift ) {
				trans->value->actions.append( SHIFT_CODE );
				trans->value->actPriors.append( trans->value->shiftPrior );
			}
			for ( ReductionMap::Iter red = trans->value->reductions; red.lte(); red++ ) {
				trans->value->actions.append( makeReduceCode( red->key, false ) );
				trans->value->actPriors.append( red->value );
			}
			trans->value->actOrds.appendDup( 0, trans->value->actions.length() );
		}
	}

	pdaActionOrder( pdaGraph, parserEls );
	sortActions( pdaGraph );
	resolvePrecedence( pdaGraph );
	advanceReductions( pdaGraph );
	pdaGraph->setStateNumbers();
	reduceActions( pdaGraph );

	/* Set the action ids. */
	int actionSetId = 0;
	for ( PdaActionSet::Iter asi = pdaGraph->actionSet; asi.lte(); asi++ )
		asi->key.id = actionSetId++;
	
	/* Get the max index. */
	pdaGraph->maxIndex = actionSetId - 1;

	/* Compute the max prod length. */
	pdaGraph->maxProdLen = 0;
	for ( DefList::Iter prod = prodList; prod.lte(); prod++ ) {
		if ( (unsigned)prod->fsmLength > pdaGraph->maxProdLen )
			pdaGraph->maxProdLen = prod->fsmLength;
	}

	/* Asserts that any transition with a nonterminal has a single action
	 * which is either a shift or a shift-reduce. */
	for ( PdaStateList::Iter state = pdaGraph->stateList; state.lte(); state++ ) {
		for ( TransMap::Iter trans = state->transMap; trans.lte(); trans++ ) {
			KlangEl *langEl = langElIndex[trans->value->lowKey];
			if ( langEl != 0 && langEl->type == KlangEl::NonTerm ) {
				assert( trans->value->actions.length() == 1 );
				assert( trans->value->actions[0] == SHIFT_CODE ||
					(trans->value->actions[0] & 0x3) == SHIFT_REDUCE_CODE );
			}
		}
	}

	/* Assert that shift reduces always appear on their own. */
	for ( PdaStateList::Iter state = pdaGraph->stateList; state.lte(); state++ ) {
		for ( TransMap::Iter trans = state->transMap; trans.lte(); trans++ ) {
			for ( ActDataList::Iter act = trans->value->actions; act.lte(); act++ ) {
				if ( (*act & 0x3) == SHIFT_REDUCE_CODE )
					assert( trans->value->actions.length() == 1 );
			}
		}
	}

	/* Verify that any type we parse_stop can actually be parsed that way. */
	for ( KlangElSet::Iter pe = parserEls; pe.lte(); pe++ ) {
		KlangEl *lel = *pe;
		if ( lel->parseStop )
			verifyParseStopGrammar(lel , pdaGraph);
	}
}

void ParseData::wrapNonTerminals()
{
	/* Make a language element that will be used to make the root productions.
	 * These are used for making parsers rooted at any production (including
	 * the start symbol). */
	rootKlangEl = new KlangEl( rootNamespace, "_root", KlangEl::NonTerm );
	langEls.append( rootKlangEl );
	SymbolMapEl *rootMapEl = rootNamespace->symbolMap.insert( 
			rootKlangEl->name, rootKlangEl );
	assert( rootMapEl != 0 );

	for ( LelList::Iter lel = langEls; lel.lte(); lel++ ) {
		/* Make a single production used when the lel is a root. */
		ProdElList *prodElList = makeProdElList( lel );
		lel->rootDef = new Definition( InputLoc(), rootKlangEl, 
				prodElList, false, 0,
				prodList.length(), Definition::Production );
		prodList.append( lel->rootDef );
		rootKlangEl->defList.append( lel->rootDef );
	}
}

bool ParseData::makeNonTermFirstSetProd( Definition *prod, PdaState *state )
{
	bool modified = false;
	for ( TransMap::Iter trans = state->transMap; trans.lte(); trans++ ) {
		if ( trans->key >= firstNonTermId ) {
			long *inserted = prod->nonTermFirstSet.insert( trans->key );
			if ( inserted != 0 )
				modified = true;

			bool hasEpsilon = false;
			KlangEl *lel = langElIndex[trans->key];
			for ( LelDefList::Iter ldef = lel->defList; ldef.lte(); ldef++ ) {
				for ( ProdIdSet::Iter pid = ldef->nonTermFirstSet; 
						pid.lte(); pid++ )
				{
					if ( *pid == -1 )
						hasEpsilon = true;
					else {
						long *inserted = prod->nonTermFirstSet.insert( *pid );
						if ( inserted != 0 )
							modified = true;
					}
				}
			}

			if ( hasEpsilon ) {
				if ( trans->value->toState->isFinState() ) {
					long *inserted = prod->nonTermFirstSet.insert( -1 );
					if ( inserted != 0 )
						modified = true;
				}

				bool lmod = makeNonTermFirstSetProd( prod, trans->value->toState );
				if ( lmod )
					modified = true;
			}
		}
	}
	return modified;
}


void ParseData::makeNonTermFirstSets()
{
	bool modified = true;
	while ( modified ) {
		modified = false;
		for ( DefList::Iter prod = prodList; prod.lte(); prod++ ) {
			if ( prod->fsm->startState->isFinState() ) {
				long *inserted = prod->nonTermFirstSet.insert( -1 );
				if ( inserted != 0 )
					modified = true;
			}

			bool lmod = makeNonTermFirstSetProd( prod, prod->fsm->startState );
			if ( lmod )
				modified = true;
		}
	}

	for ( DefList::Iter prod = prodList; prod.lte(); prod++ ) {
		if ( prod->nonTermFirstSet.find( prod->prodName->id ) )
			prod->isLeftRec = true;
	}
}

void ParseData::printNonTermFirstSets()
{
	for ( DefList::Iter prod = prodList; prod.lte(); prod++ ) {
		cerr << prod->data << ": ";
		for ( ProdIdSet::Iter pid = prod->nonTermFirstSet; pid.lte(); pid++ )
		{
			if ( *pid < 0 )
				cerr << " <EPSILON>";
			else {
				KlangEl *lel = langElIndex[*pid];
				cerr << " " << lel->name;
			}
		}
		cerr << endl;

		if ( prod->isLeftRec )
			cerr << "PROD IS LEFT REC: " << prod->data << endl;
	}
}

bool ParseData::makeFirstSetProd( Definition *prod, PdaState *state )
{
	bool modified = false;
	for ( TransMap::Iter trans = state->transMap; trans.lte(); trans++ ) {
		if ( trans->key < firstNonTermId ) {
			long *inserted = prod->firstSet.insert( trans->key );
			if ( inserted != 0 )
				modified = true;
		}
		else {
			long *inserted = prod->firstSet.insert( trans->key );
			if ( inserted != 0 )
				modified = true;

			KlangEl *klangEl = langElIndex[trans->key];
			if ( klangEl != 0 && klangEl->termDup != 0 ) {
				long *inserted2 = prod->firstSet.insert( klangEl->termDup->id );
				if ( inserted2 != 0 )
					modified = true;
			}

			bool hasEpsilon = false;
			KlangEl *lel = langElIndex[trans->key];
			for ( LelDefList::Iter ldef = lel->defList; ldef.lte(); ldef++ ) {
				for ( ProdIdSet::Iter pid = ldef->firstSet; 
						pid.lte(); pid++ )
				{
					if ( *pid == -1 )
						hasEpsilon = true;
					else {
						long *inserted = prod->firstSet.insert( *pid );
						if ( inserted != 0 )
							modified = true;
					}
				}
			}

			if ( hasEpsilon ) {
				if ( trans->value->toState->isFinState() ) {
					long *inserted = prod->firstSet.insert( -1 );
					if ( inserted != 0 )
						modified = true;
				}

				bool lmod = makeFirstSetProd( prod, trans->value->toState );
				if ( lmod )
					modified = true;
			}
		}
	}
	return modified;
}


void ParseData::makeFirstSets()
{
	bool modified = true;
	while ( modified ) {
		modified = false;
		for ( DefList::Iter prod = prodList; prod.lte(); prod++ ) {
			if ( prod->fsm->startState->isFinState() ) {
				long *inserted = prod->firstSet.insert( -1 );
				if ( inserted != 0 )
					modified = true;
			}

			bool lmod = makeFirstSetProd( prod, prod->fsm->startState );
			if ( lmod )
				modified = true;
		}
	}
}

void ParseData::printFirstSets()
{
	for ( DefList::Iter prod = prodList; prod.lte(); prod++ ) {
		cerr << prod->data << ": ";
		for ( ProdIdSet::Iter pid = prod->firstSet; pid.lte(); pid++ )
		{
			if ( *pid < 0 )
				cerr << " <EPSILON>";
			else {
				KlangEl *lel = langElIndex[*pid];
				if ( lel != 0 ) 
					cerr << endl << "    " << lel->name;
				else
					cerr << endl << "    " << *pid;
			}
		}
		cerr << endl;
	}
}

void ParseData::insertUniqueEmptyProductions()
{
	int limit = prodList.length();
	for ( DefList::Iter prod = prodList; prod.lte(); prod++ ) {
		if ( prod->prodId == limit )
			break;

		/* Get a language element. */
		char name[20];
		sprintf(name, "U%li", prodList.length());
		KlangEl *prodName = getKlangEl( this, rootNamespace, name );
		assert( prodName->type == KlangEl::Unknown );
		prodName->type = KlangEl::NonTerm;
		Definition *newDef = new Definition( InputLoc(), prodName, 
				0 /* FIXME new VarDef( name, 0 )*/, 
				false, 0, prodList.length(), Definition::Production );
		prodName->defList.append( newDef );
		prodList.append( newDef );

		prod->uniqueEmptyLeader = prodName;
	}
}

void ParseData::makeRuntimeData()
{
	long count = 0;

	/*
	 * ProdLengths
	 * ProdLhsIs
	 * ProdNames
	 * ProdCodeBlocks
	 * ProdCodeBlockLens
	 */

	runtimeData->frameInfo = new FrameInfo[nextFrameId];
	runtimeData->numFrames = nextFrameId;
	memset( runtimeData->frameInfo, 0, sizeof(FrameInfo) * nextFrameId );

	/*
	 * Init code block.
	 */
	if ( rootCodeBlock == 0 ) {
		runtimeData->rootCode = 0;
		runtimeData->rootCodeLen = 0;
	}
	else {
		runtimeData->rootCode = rootCodeBlock->codeWC.data;
		runtimeData->rootCodeLen = rootCodeBlock->codeWC.length();
	}

	runtimeData->frameInfo[rootCodeBlock->frameId].codeWV = 0;
	runtimeData->frameInfo[rootCodeBlock->frameId].codeLenWV = 0;
	runtimeData->frameInfo[rootCodeBlock->frameId].trees = rootCodeBlock->trees.data;
	runtimeData->frameInfo[rootCodeBlock->frameId].treesLen = rootCodeBlock->trees.length();

	/*
	 * prodInfo
	 */
	count = prodList.length();
	runtimeData->prodInfo = new ProdInfo[count];
	runtimeData->numProds = count;

	count = 0;
	for ( DefList::Iter prod = prodList; prod.lte(); prod++ ) {
		runtimeData->prodInfo[count].length = prod->fsmLength;
		runtimeData->prodInfo[count].lhsId = prod->prodName->id;
		runtimeData->prodInfo[count].name = prod->data;
		runtimeData->prodInfo[count].frameId = -1;

		CodeBlock *block = prod->redBlock;
		if ( block != 0 ) {
			runtimeData->prodInfo[count].frameId = block->frameId;
			runtimeData->frameInfo[block->frameId].codeWV = block->codeWV.data;
			runtimeData->frameInfo[block->frameId].codeLenWV = block->codeWV.length();

			runtimeData->frameInfo[block->frameId].trees = block->trees.data;
			runtimeData->frameInfo[block->frameId].treesLen = block->trees.length();
		}

		runtimeData->prodInfo[count].lhsUpref = true;
		count += 1;
	}

	/*
	 * regionInfo
	 */
	runtimeData->numRegions = regionList.length()+1;
	runtimeData->regionInfo = new RegionInfo[runtimeData->numRegions];
	runtimeData->regionInfo[0].name = "___EMPTY";
	runtimeData->regionInfo[0].defaultToken = -1;
	for ( RegionList::Iter reg = regionList; reg.lte(); reg++ ) {
		long regId = reg->id+1;
		runtimeData->regionInfo[regId].name = reg->name;
		runtimeData->regionInfo[regId].defaultToken =
			reg->defaultTokenDef == 0 ? -1 : reg->defaultTokenDef->token->id;
		runtimeData->regionInfo[regId].eofFrameId = -1;

		CodeBlock *block = reg->preEofBlock;
		if ( block != 0 ) {
			runtimeData->regionInfo[regId].eofFrameId = block->frameId;
			runtimeData->frameInfo[block->frameId].codeWV = block->codeWV.data;
			runtimeData->frameInfo[block->frameId].codeLenWV = block->codeWV.length();

			runtimeData->frameInfo[block->frameId].trees = block->trees.data;
			runtimeData->frameInfo[block->frameId].treesLen = block->trees.length();
		}
	}

	/*
	 * lelInfo
	 */

	count = nextSymbolId;
	runtimeData->lelInfo = new LangElInfo[count];
	runtimeData->numLangEls = count;

	for ( int i = 0; i < nextSymbolId; i++ ) {
		KlangEl *lel = langElIndex[i];
		if ( lel != 0 ) {
			runtimeData->lelInfo[i].name = lel->fullLit;
			runtimeData->lelInfo[i].repeat = lel->isRepeat;
			runtimeData->lelInfo[i].list = lel->isList;
			runtimeData->lelInfo[i].literal = lel->isLiteral;
			runtimeData->lelInfo[i].ignore = lel->ignore;
			runtimeData->lelInfo[i].frameId = -1;

			CodeBlock *block = lel->transBlock;
			if ( block != 0 ) {
				runtimeData->lelInfo[i].frameId = block->frameId;
				runtimeData->frameInfo[block->frameId].codeWV = block->codeWV.data;
				runtimeData->frameInfo[block->frameId].codeLenWV = block->codeWV.length();

				runtimeData->frameInfo[block->frameId].trees = block->trees.data;
				runtimeData->frameInfo[block->frameId].treesLen = block->trees.length();
			}

			runtimeData->lelInfo[i].objectTypeId = 
					lel->objectDef == 0 ? 0 : lel->objectDef->id;
			runtimeData->lelInfo[i].ofiOffset = lel->ofiOffset;
			runtimeData->lelInfo[i].objectLength = 
					( lel->objectDef == 0 || lel->objectDef == tokenObj ) ? 0 : 
					lel->objectDef->size();
			runtimeData->lelInfo[i].termDupId = lel->termDup == 0 ? 0 : lel->termDup->id;
			runtimeData->lelInfo[i].genericId = lel->generic == 0 ? 0 : lel->generic->id;

			if ( lel->tokenDef != 0 && lel->tokenDef->join != 0 && 
					lel->tokenDef->join->context != 0 )
				runtimeData->lelInfo[i].markId = lel->tokenDef->join->mark->markId;
			else
				runtimeData->lelInfo[i].markId = -1;

			runtimeData->lelInfo[i].numCaptureAttr = 0;
		}
		else {
			memset(&runtimeData->lelInfo[i], 0, sizeof(LangElInfo) );
			runtimeData->lelInfo[i].name = "__UNUSED";
			runtimeData->lelInfo[i].frameId = -1;
		}
	}

	/*
	 * FunctionInfo
	 */
	count = functionList.length();

	runtimeData->functionInfo = new FunctionInfo[count];
	runtimeData->numFunctions = count;
	for ( FunctionList::Iter func = functionList; func.lte(); func++ ) {
		runtimeData->functionInfo[func->funcId].name = func->name;
		runtimeData->functionInfo[func->funcId].frameId = -1;

		CodeBlock *block = func->codeBlock;
		if ( block != 0 ) {
			runtimeData->functionInfo[func->funcId].frameId = block->frameId;

			runtimeData->frameInfo[block->frameId].codeWV = block->codeWV.data;
			runtimeData->frameInfo[block->frameId].codeLenWV = block->codeWV.length();

			runtimeData->frameInfo[block->frameId].codeWC = block->codeWC.data;
			runtimeData->frameInfo[block->frameId].codeLenWC = block->codeWC.length();

			runtimeData->frameInfo[block->frameId].trees = block->trees.data;
			runtimeData->frameInfo[block->frameId].treesLen = block->trees.length();
		}

		runtimeData->functionInfo[func->funcId].argSize = func->paramListSize;
		runtimeData->functionInfo[func->funcId].ntrees = func->localFrame->sizeTrees();
		runtimeData->functionInfo[func->funcId].frameSize = func->localFrame->size();
	}

	/*
	 * PatReplInfo
	 */

	/* Filled in later after patterns are parsed. */
	runtimeData->patReplInfo = new PatReplInfo[nextPatReplId];
	memset( runtimeData->patReplInfo, 0, sizeof(PatReplInfo) * nextPatReplId );
	runtimeData->numPatterns = nextPatReplId;
	runtimeData->patReplNodes = 0;
	runtimeData->numPatternNodes = 0;

	
	/*
	 * GenericInfo
	 */
	count = 1;
	for ( NamespaceList::Iter nspace = namespaceList; nspace.lte(); nspace++ )
		count += nspace->genericList.length();
	assert( count == nextGenericId );

	runtimeData->genericInfo = new GenericInfo[count];
	runtimeData->numGenerics = count;
	memset( &runtimeData->genericInfo[0], 0, sizeof(GenericInfo) );
	for ( NamespaceList::Iter nspace = namespaceList; nspace.lte(); nspace++ ) {
		for ( GenericList::Iter gen = nspace->genericList; gen.lte(); gen++ ) {
			runtimeData->genericInfo[gen->id].type = gen->typeId;
			runtimeData->genericInfo[gen->id].typeArg = gen->utArg->typeId;
			runtimeData->genericInfo[gen->id].keyType = gen->keyUT != 0 ? 
					gen->keyUT->typeId : 0;
			runtimeData->genericInfo[gen->id].keyOffset = 0;
			runtimeData->genericInfo[gen->id].langElId = gen->langEl->id;
		}
	}

	/*
	 * Literals
	 */
	runtimeData->numLiterals = literalStrings.length();
	runtimeData->litdata = new const char *[literalStrings.length()];
	runtimeData->litlen = new long [literalStrings.length()];
	runtimeData->literals = 0;
	for ( StringMap::Iter el = literalStrings; el.lte(); el++ ) {
		/* Data. */
		char *data = new char[el->key.length()+1];
		memcpy( data, el->key.data, el->key.length() );
		data[el->key.length()] = 0;
		runtimeData->litdata[el->value] = data;

		/* Length. */
		runtimeData->litlen[el->value] = el->key.length();
	}

	/* Captured attributes. Loop over tokens and count first. */
	long numCapturedAttr = 0;
	for ( RegionList::Iter reg = regionList; reg.lte(); reg++ ) {
		for ( TokenDefList::Iter td = reg->tokenDefList; td.lte(); td++ )
			numCapturedAttr += td->reCaptureVect.length();
	}
	runtimeData->captureAttr = new CaptureAttr[numCapturedAttr];
	runtimeData->numCapturedAttr = numCapturedAttr;

	count = 0;
	for ( RegionList::Iter reg = regionList; reg.lte(); reg++ ) {
		for ( TokenDefList::Iter td = reg->tokenDefList; td.lte(); td++ ) {
			runtimeData->lelInfo[td->token->id].captureAttr = count;
			runtimeData->lelInfo[td->token->id].numCaptureAttr = td->reCaptureVect.length();
			for ( ReCaptureVect::Iter c = td->reCaptureVect; c.lte(); c++ ) {
				runtimeData->captureAttr[count].mark_enter = c->markEnter->markId;
				runtimeData->captureAttr[count].mark_leave = c->markLeave->markId;
				runtimeData->captureAttr[count].offset = c->objField->offset;

				count += 1;
			}
		}
	}

	runtimeData->fsmTables = fsmTables;
	runtimeData->pdaTables = pdaTables;

	runtimeData->startStates = new int[nextParserId];
	runtimeData->eofLelIds = new int[nextParserId];
	runtimeData->numParsers = nextParserId;
	for ( LelList::Iter lel = langEls; lel.lte(); lel++ ) {
		if ( lel->parserId >= 0 ) {
			runtimeData->startStates[lel->parserId] = lel->startState->stateNum;
			runtimeData->eofLelIds[lel->parserId] = lel->eofLel->id;
		}
	}

	runtimeData->globalSize = globalObjectDef->size();

	/*
	 * firstNonTermId
	 */
	runtimeData->firstNonTermId = firstNonTermId;

	/* Special trees. */
	runtimeData->integerId = intKlangEl->id;
	runtimeData->stringId = strKlangEl->id;
	runtimeData->anyId = anyKlangEl->id;
	runtimeData->eofId = 0; //eofKlangEl->id;
	runtimeData->noTokenId = noTokenKlangEl->id;
}

/* Borrow alg->state for mapsTo. */
void mapNodes( Program *prg, int &count, Kid *kid )
{
	if ( kid != 0 ) {
		pt(kid->tree)->state = count++;

		Kid *ignore = tree_ignore( prg, kid->tree );
		while ( tree_is_ignore( prg, ignore ) ) {
			count += 1;
			ignore = ignore->next;
		}
		
		count += prg->rtd->lelInfo[kid->tree->id].numCaptureAttr;

		mapNodes( prg, count, tree_child( prg, kid->tree ) );
		mapNodes( prg, count, kid->next );
	}
}

void fillNodes( Program *prg, Bindings &bindings, long &bindId, 
		PatReplNode *nodes, Kid *kid )
{
	if ( kid != 0 ) {
		long ind = pt(kid->tree)->state;
		PatReplNode &node = nodes[ind++];

		Kid *child = tree_child( prg, kid->tree );

		/* Set up the fields. */
		node.id = kid->tree->id;
		node.child = child == 0 ? -1 : pt(child->tree)->state;
		node.next = kid->next == 0 ? -1 : pt(kid->next->tree)->state;
		node.length = string_length( kid->tree->tokdata );
		node.data = string_data( kid->tree->tokdata );

		/* Ignore items. */
		Kid *ignore = tree_ignore( prg, kid->tree );
		node.ignore = ignore == 0 ? -1 : ind;

		while ( ignore != 0 ) {
			PatReplNode &node = nodes[ind++];

			memset( &node, 0, sizeof(PatReplNode) );
			node.id = ignore->tree->id;
			node.next = ignore->next == 0 ? -1 : ind;
			
			node.length = string_length( ignore->tree->tokdata );
			node.data = string_data( ignore->tree->tokdata );

			ignore = ignore->next;
		}

		/* The captured attributes. */
		for ( int i = 0; i < prg->rtd->lelInfo[kid->tree->id].numCaptureAttr; i++ ) {
			CaptureAttr *cap = prg->rtd->captureAttr + 
					prg->rtd->lelInfo[kid->tree->id].captureAttr + i;

			Tree *attr = get_attr( kid->tree, cap->offset );

			PatReplNode &node = nodes[ind++];
			memset( &node, 0, sizeof(PatReplNode) );

			node.id = attr->id;
			node.length = string_length( attr->tokdata );
			node.data = string_data( attr->tokdata );
		}

		node.stop = kid->tree->flags & AF_TERM_DUP;

		/* Recurse. */
		fillNodes( prg, bindings, bindId, nodes, child );

		/* Since the parser is bottom up the bindings are in a bottom up
		 * traversal order. Check after recursing. */
		node.bindId = 0;
		if ( bindings.data[bindId] == kid->tree ) {
			/* Remember that binding ids are indexed from one. */
			node.bindId = bindId++;

			//cout << "binding match in " << __PRETTY_FUNCTION__ << endl;
			//cout << "bindId: " << node.bindId << endl;
		}

		/* Move to the next child. */
		fillNodes( prg, bindings, bindId, nodes, kid->next );
	}
}

void ParseData::fillInPatterns( Program *prg )
{
	/*
	 * patReplNodes
	 */

	/* Count is referenced and computed by mapNode. */
	int count = 0;
	for ( PatternList::Iter pat = patternList; pat.lte(); pat++ )
		mapNodes( prg, count, pat->pdaRun->stackTop );

	for ( ReplList::Iter repl = replList; repl.lte(); repl++ )
		mapNodes( prg, count, repl->pdaRun->stackTop );
	
	runtimeData->patReplNodes = new PatReplNode[count];
	runtimeData->numPatternNodes = count;

	for ( PatternList::Iter pat = patternList; pat.lte(); pat++ ) {
		runtimeData->patReplInfo[pat->patRepId].offset = 
				pt(pat->pdaRun->stackTop->next->tree)->state;

		/* BindIds are indexed base one. */
		runtimeData->patReplInfo[pat->patRepId].numBindings = 
				pat->pdaRun->bindings.length() - 1;

		/* Init the bind */
		long bindId = 1;
		fillNodes( prg, pat->pdaRun->bindings, bindId,
			runtimeData->patReplNodes, pat->pdaRun->stackTop );
	}

	for ( ReplList::Iter repl = replList; repl.lte(); repl++ ) {
		runtimeData->patReplInfo[repl->patRepId].offset = 
				pt(repl->pdaRun->stackTop->next->tree)->state;

		/* BindIds are indexed base one. */
		runtimeData->patReplInfo[repl->patRepId].numBindings = 
				repl->pdaRun->bindings.length() - 1;

		long bindId = 1;
		fillNodes( prg, repl->pdaRun->bindings, bindId,
				runtimeData->patReplNodes, repl->pdaRun->stackTop );
	}
}


PdaTables *ParseData::makePdaTables( PdaGraph *pdaGraph )
{
	int count, curOffset, pos;
	PdaTables *pdaTables = new PdaTables;

	/*
	 * Indicies.
	 */
	count = 0;
	for ( PdaStateList::Iter state = pdaGraph->stateList; state.lte(); state++ ) {
		for ( TransMap::Iter trans = state->transMap; trans.lte(); trans++ ) {
			count++;
			if ( ! trans.last() ) {
				TransMap::Iter next = trans.next();
				for ( long key = trans->key+1; key < next->key; key++ )
					count++;
			}
		}
	}
	pdaTables->indicies = new int[count];
	pdaTables->numIndicies = count;

	count = 0;
	for ( PdaStateList::Iter state = pdaGraph->stateList; state.lte(); state++ ) {
		for ( TransMap::Iter trans = state->transMap; trans.lte(); trans++ ) {
			pdaTables->indicies[count++] = trans->value->actionSetEl->key.id;

			if ( ! trans.last() ) {
				TransMap::Iter next = trans.next();
				for ( long key = trans->key+1; key < next->key; key++ )
					pdaTables->indicies[count++] = -1;
			}
		}
	}

	/*
	 * Keys
	 */
	count = pdaGraph->stateList.length() * 2;;
	pdaTables->keys = new int[count];
	pdaTables->numKeys = count;

	count = 0;
	for ( PdaStateList::Iter state = pdaGraph->stateList; state.lte(); state++ ) {
		if ( state->transMap.length() == 0 ) {
			pdaTables->keys[count+0] = 0;
			pdaTables->keys[count+1] = 0;
		}
		else {
			TransMap::Iter first = state->transMap.first();
			TransMap::Iter last = state->transMap.last();
			pdaTables->keys[count+0] = first->key;
			pdaTables->keys[count+1] = last->key;
		}
		count += 2;
	}

	/*
	 * Offsets
	 */
	count = pdaGraph->stateList.length(); 
	pdaTables->offsets = new unsigned int[count];
	pdaTables->numStates = count;

	count = 0;
	curOffset = 0;
	for ( PdaStateList::Iter state = pdaGraph->stateList; state.lte(); state++ ) {
		pdaTables->offsets[count++] = curOffset;

		/* Increment the offset. */
		if ( state->transMap.length() > 0 ) {
			TransMap::Iter first = state->transMap.first();
			TransMap::Iter last = state->transMap.last();
			curOffset += last->key - first->key + 1;
		}
	}

	/*
	 * Targs
	 */
	count = pdaGraph->actionSet.length();
	pdaTables->targs = new unsigned int[count];
	pdaTables->numTargs = count;

	count = 0;
	for ( PdaActionSet::Iter asi = pdaGraph->actionSet; asi.lte(); asi++ )
		pdaTables->targs[count++] = asi->key.targ;

	/* 
	 * ActInds
	 */
	count = pdaGraph->actionSet.length();
	pdaTables->actInds = new unsigned int[count];
	pdaTables->numActInds = count;

	count = pos = 0;
	for ( PdaActionSet::Iter asi = pdaGraph->actionSet; asi.lte(); asi++ ) {
		pdaTables->actInds[count++] = pos;
		pos += asi->key.actions.length() + 1;
	}

	/*
	 * Actions
	 */
	count = 0;
	for ( PdaActionSet::Iter asi = pdaGraph->actionSet; asi.lte(); asi++ )
		count += asi->key.actions.length() + 1;

	pdaTables->actions = new unsigned int[count];
	pdaTables->numActions = count;

	count = 0;
	for ( PdaActionSet::Iter asi = pdaGraph->actionSet; asi.lte(); asi++ ) {
		for ( ActDataList::Iter ali = asi->key.actions; ali.lte(); ali++ )
			pdaTables->actions[count++] = *ali;

		pdaTables->actions[count++] = 0;
	}

	/*
	 * CommitLen
	 */
	count = pdaGraph->actionSet.length();
	pdaTables->commitLen = new int[count];
	pdaTables->numCommitLen = count;

	count = 0;
	for ( PdaActionSet::Iter asi = pdaGraph->actionSet; asi.lte(); asi++ )
		pdaTables->commitLen[count++] = asi->key.commitLen;
	
	/*
	 * tokenRegionInds
	 */
	count = pos = 0;
	pdaTables->tokenRegionInds = new int[pdaTables->numStates];
	for ( PdaStateList::Iter state = pdaGraph->stateList; state.lte(); state++ ) {
		pdaTables->tokenRegionInds[count++] = pos;
		pos += state->regions.length() + 1;
	}

	/*
	 * tokenRegions
	 */

	count = 0;
	for ( PdaStateList::Iter state = pdaGraph->stateList; state.lte(); state++ )
		count += state->regions.length() + 1;

	pdaTables->numRegionItems = count;
	pdaTables->tokenRegions = new int[pdaTables->numRegionItems];

	count = 0;
	for ( PdaStateList::Iter state = pdaGraph->stateList; state.lte(); state++ ) {
		for ( RegionVect::Iter reg = state->regions; reg.lte(); reg++ )
			pdaTables->tokenRegions[count++] = (*reg)->id + 1;

		pdaTables->tokenRegions[count++] = 0;
	}

	/* Get a pointer to the (yet unfilled) global runtime data. */
	pdaTables->rtd = runtimeData;

	return pdaTables;
}

void ParseData::prepGrammar()
{
	wrapNonTerminals();
	makeKlangElIds();
	makeKlangElNames();
	makeDefinitionNames();
	noUndefindKlangEls();

	/* Put the language elements in an index by language element id. */
	langElIndex = new KlangEl*[nextSymbolId+1];
	memset( langElIndex, 0, sizeof(KlangEl*)*(nextSymbolId+1) );
	for ( LelList::Iter lel = langEls; lel.lte(); lel++ )
		langElIndex[lel->id] = lel;

	makeProdFsms();

	/* Allocate the Runtime data now. Every PdaTable that we make 
	 * will reference it, but it will be filled in after all the tables are
	 * built. */
	runtimeData = new RuntimeData;
}

PdaGraph *ParseData::makePdaGraph( KlangElSet &parserEls )
{
	//for ( DefList::Iter prod = prodList; prod.lte(); prod++ )
	//	cerr << prod->prodId << " " << prod->data << endl;

	PdaGraph *pdaGraph = new PdaGraph();
	lalr1GenerateParser( pdaGraph, parserEls );
	pdaGraph->setStateNumbers();
	analyzeMachine( pdaGraph, parserEls );

	//cerr << "NUMBER OF STATES: " << pdaGraph->stateList.length() << endl;

	return pdaGraph;
}

void ParseData::makeParser( KlangElSet &parserEls )
{
	pdaGraph = makePdaGraph( parserEls );
	pdaTables = makePdaTables( pdaGraph );
}
