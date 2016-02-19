/*
 *  Copyright 2006-2012 Adrian Thurston <thurston@complang.org>
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

#define EOF_REGION 0

#include <iostream>
#include <iomanip>
#include <errno.h>
#include <stdlib.h>

/* Parsing. */
#include "global.h"
#include "compiler.h"
#include "pdacodegen.h"
#include "pdarun.h"
#include "redfsm.h"
#include "fsmcodegen.h"
#include "redbuild.h"

/* Dumping the fsm. */
#include "mergesort.h"

using std::endl;
using std::cerr;
using std::cout;

char startDefName[] = "start";

extern "C" tree_t **internal_host_call( program_t *prg, long code, tree_t **sp )
{
	return 0;
}

extern "C" void internal_commit_reduce_forward( program_t *prg, tree_t **root,
		struct pda_run *pda_run, parse_tree_t *pt )
{
	commit_clear_parse_tree( prg, root, pda_run, pt->child );
}

extern "C" long internal_commit_union_sz( int reducer )
{
	return 0;
}

extern "C" void internal_init_need()
{
}

extern "C" int internal_reducer_need_tok( program_t *prg, struct pda_run *, int id )
{
	return 3;
}

extern "C" int internal_reducer_need_ign( program_t *prg, struct pda_run * )
{
	return 3;
}

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

LangEl::LangEl( Namespace *nspace, const String &name, Type type )
:
	nspace(nspace),
	name(name),
	lit(name),
	type(type),
	id(-1),
	numAppearances(0),
	commit(false),
	isIgnore(false),
	reduceFirst(false),
	isLiteral(false),
	isRepeat(false),
	isList(false),
	isOpt(false),
	parseStop(false),
	isEOF(false),
	repeatOf(0),
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
	parserId(-1),
	predType(PredNone),
	predValue(0),
	contextDef(0),
	contextIn(0), 
	noPreIgnore(false),
	noPostIgnore(false),
	isZero(false)
{
}
 
PdaGraph *ProdElList::walk( Compiler *pd, Production *prod )
{
	PdaGraph *prodFsm = new PdaGraph();
	PdaState *last = prodFsm->addState();
	prodFsm->setStartState( last );

	int prodLength = 0;
	for ( Iter prodEl = first(); prodEl.lte(); prodEl++, prodLength++ ) {
		//PdaGraph *itemFsm = prodEl->walk( pd );
		long value = prodEl->langEl->id;

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


ProdElList *Compiler::makeProdElList( LangEl *langEl )
{
	ProdElList *prodElList = new ProdElList();
	UniqueType *uniqueType = findUniqueType( TYPE_TREE, langEl );
	TypeRef *typeRef = TypeRef::cons( internal, uniqueType );
	prodElList->append( new ProdEl( internal, typeRef ) );
	prodElList->tail->langEl = langEl;
	return prodElList;
}

void Compiler::makeDefinitionNames()
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
void Compiler::noUndefindLangEls()
{
	for ( LelList::Iter lel = langEls; lel.lte(); lel++ ) {
		if ( lel->type == LangEl::Unknown )
			error() << "'" << lel->name << "' was not defined as anything" << endp;
	}
}

void Compiler::makeLangElIds()
{
	/* The first id 0 is reserved for the stack sentinal. A negative id means
	 * error to the parsing function, inducing backtracking. */
	nextLelId = 1;

	/* First pass assigns to the user terminals. */
	for ( LelList::Iter lel = langEls; lel.lte(); lel++ ) {
		/* Must be a term, and not any of the special reserved terminals.
		 * Remember if the non terminal is a user non terminal. */
		if ( lel->type == LangEl::Term && 
				!lel->isEOF && 
				lel != errorLangEl &&
				lel != noTokenLangEl )
		{
			lel->id = nextLelId++;
		}
	}

	//eofLangEl->id = nextLelId++;
	for ( LelList::Iter lel = langEls; lel.lte(); lel++ ) {
		/* Must be a term, and not any of the special reserved terminals.
		 * Remember if the non terminal is a user non terminal. */
		if ( lel->isEOF )
			lel->id = nextLelId++;
	}

	/* Next assign to the eof notoken, which we always create. */
	noTokenLangEl->id = nextLelId++;

	/* Possibly assign to the error language element. */
	if ( errorLangEl != 0 )
		errorLangEl->id = nextLelId++;

	/* Save this for the code generation. */
	firstNonTermId = nextLelId;

	/* A third and final pass assigns to everything else. */
	for ( LelList::Iter lel = langEls; lel.lte(); lel++ ) {
		/* Anything else not yet assigned gets assigned now. */
		if ( lel->id < 0 )
			lel->id = nextLelId++;
	}

	assert( ptrLangEl->id == LEL_ID_PTR );
//	assert( boolLangEl->id == LEL_ID_BOOL );
//	assert( intLangEl->id == LEL_ID_INT );
	assert( strLangEl->id == LEL_ID_STR );
	assert( ignoreLangEl->id == LEL_ID_IGNORE );
}

void Compiler::makeStructElIds()
{
	int nextId = 0;
	for ( StructElList::Iter sel = structEls; sel.lte(); sel++ )
		sel->id = nextId++;
}

void Compiler::refNameSpace( LangEl *lel, Namespace *nspace )
{
	if ( nspace == rootNamespace ) {
		lel->refName = "::" + lel->refName;
		return;
	}
	
	lel->refName = nspace->name + "::" + lel->refName;
	lel->declName = nspace->name + "::" + lel->declName;
	lel->xmlTag = nspace->name + "::" + lel->xmlTag;
	refNameSpace( lel, nspace->parentNamespace );
}

void Compiler::makeLangElNames()
{
	for ( LelList::Iter lel = langEls; lel.lte(); lel++ ) {
		if ( lel->id == LEL_ID_VOID ) {
			lel->fullName = "_void";
			lel->fullLit = "_void";
			lel->refName = "_void";
			lel->declName = "_void";
			lel->xmlTag = "void";

		}
//		else if ( lel->id == LEL_ID_INT ) {
//			lel->fullName = "_int";
//			lel->fullLit = "_int";
//			lel->refName = "_int";
//			lel->declName = "_int";
//			lel->xmlTag = "int";
//		}
//		else if ( lel->id == LEL_ID_BOOL ) {
//			lel->fullName = "_bool";
//			lel->fullLit = "_bool";
//			lel->refName = "_bool";
//			lel->declName = "_bool";
//			lel->xmlTag = "bool";
//		}
		else {
			lel->fullName = lel->name;
			lel->fullLit = lel->lit;
			lel->refName = lel->lit;
			lel->declName = lel->lit;
			lel->xmlTag = lel->name;
		}

		/* If there is also a namespace next to the type, we add a prefix to
		 * the type. It's not convenient to name C++ classes the same as a
		 * namespace in the same scope. We don't want to restrict colm, so we
		 * add a workaround for the least-common case. The type gets t_ prefix.
		 * */
		Namespace *nspace = lel->nspace->findNamespace( lel->name );
		if ( nspace != 0 ) {
			lel->refName = "t_" + lel->refName;
			lel->fullName = "t_" + lel->fullName;
			lel->declName = "t_" + lel->declName;
			lel->xmlTag = "t_" + lel->xmlTag;
		}

		refNameSpace( lel, lel->nspace );
	}
}

/* Set up dot sets, shift info, and prod sets. */
void Compiler::makeProdFsms()
{
	/* There are two items in the index for each production (high and low). */
	int indexLen = prodList.length() * 2;
	dotItemIndex.setAsNew( indexLen );
	int dsiLow = 0, indexPos = 0;

	/* Build FSMs for all production language elements. */
	for ( DefList::Iter prod = prodList; prod.lte(); prod++ )
		prod->fsm = prod->prodElList->walk( this, prod );

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
	prodIdIndex = new Production*[prodList.length()];
	for ( DefList::Iter prod = prodList; prod.lte(); prod++ )
		prodIdIndex[prod->prodId] = prod;
}

/* Want the first set of over src. If the first set contains epsilon, go over
 * it and over tab. If overSrc is the end of the production, find the follow
 * from the table, taking only the characters on which the parent is reduced.
 * */
void Compiler::findFollow( AlphSet &result, PdaState *overTab, 
		PdaState *overSrc, Production *parentDef )
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

		LangEl *langEl = langElIndex[pastTrans->key];
		if ( langEl != 0 && langEl->type == LangEl::NonTerm ) {
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

PdaState *Compiler::followProd( PdaState *tabState, PdaState *prodState )
{
	while ( prodState->transMap.length() == 1 ) {
		TransMap::Iter prodTrans = prodState->transMap;
		PdaTrans *tabTrans = tabState->findTrans( prodTrans->key );
		prodState = prodTrans->value->toState;
		tabState = tabTrans->toState;
	}
	return tabState;
}

void Compiler::trySetTime( PdaTrans *trans, long code, long &time )
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
void Compiler::pdaOrderFollow( LangEl *rootEl, PdaState *tabState, 
		PdaTrans *tabTrans, PdaTrans *srcTrans, Production *parentDef, 
		Production *definition, long &time )
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
			addRegion( expandToState, tt->value, tt->key, 
					tt->value->noPreIgnore, tt->value->noPostIgnore );
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

void Compiler::addRegion( PdaState *tabState, PdaTrans *tabTrans,
		long pdaKey, bool noPreIgnore, bool noPostIgnore )
{
	LangEl *langEl = langElIndex[pdaKey];
	if ( langEl != 0 && langEl->type == LangEl::Term ) {
		TokenRegion *region = 0;
		RegionSet *regionSet = 0;

		/* If it is not the eof, then use the region associated 
		 * with the token definition. */
		if ( langEl->isZero ) {
			region = langEl->tokenDef->regionSet->collectIgnore;
			regionSet = langEl->tokenDef->regionSet;
		}
		else if ( !langEl->isEOF && langEl->tokenDef != 0 ) {
			region = langEl->tokenDef->regionSet->tokenIgnore;
			regionSet = langEl->tokenDef->regionSet;
		}

		if ( region != 0 ) {
			/* region. */
			TokenRegion *scanRegion = region;

			if ( langEl->noPreIgnore )
				scanRegion = regionSet->tokenOnly;

			if ( !regionVectHas( tabState->regions, scanRegion ) )
				tabState->regions.append( scanRegion );

			/* Pre-region of to state */
			PdaState *toState = tabTrans->toState;
			if ( !langEl->noPostIgnore && 
					regionSet->ignoreOnly != 0 && 
					!regionVectHas( toState->preRegions, regionSet->ignoreOnly ) )
			{
				toState->preRegions.append( regionSet->ignoreOnly );
			}
		}
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

void Compiler::pdaOrderProd( LangEl *rootEl, PdaState *tabState, 
	PdaState *srcState, Production *parentDef, long &time )
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
		LangEl *langEl = langElIndex[srcTrans->key];
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
		addRegion( tabState, tabTrans, srcTrans->key, 
				srcTrans->value->noPreIgnore, srcTrans->value->noPostIgnore );

		/* Go over one in the production. */
		pdaOrderProd( rootEl, tabTrans->toState, 
				srcTrans->value->toState, parentDef, time );
	}
}

void Compiler::pdaActionOrder( PdaGraph *pdaGraph, LangElSet &parserEls )
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
	for ( LangElSet::Iter pe = parserEls; pe.lte(); pe++ ) {
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
				LangEl *lel = langElIndex[trans->lowKey];
				if ( lel != 0 && lel->isEOF )
					state->regions.append( EOF_REGION );
			}
		}
	}

	///* Warn about states with empty token region lists. */
	//for ( PdaStateList::Iter state = pdaGraph->stateList; state.lte(); state++ ) {
	//	if ( state->regions.length() == 0 ) {
	//		warning() << "state has an empty token region, state: " << 
	//			state->stateNum << endl;
	//	}
	//}

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

void Compiler::advanceReductions( PdaGraph *pdaGraph )
{
	/* Loop all states. */
	for ( PdaStateList::Iter state = pdaGraph->stateList; state.lte(); state++ ) {
		if ( !state->advanceReductions )
			continue;

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

void Compiler::sortActions( PdaGraph *pdaGraph )
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
				LangEl *lel = langElIndex[trans->lowKey];
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

void Compiler::reduceActions( PdaGraph *pdaGraph )
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

void Compiler::computeAdvanceReductions( LangEl *langEl, PdaGraph *pdaGraph )
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
			if ( tr->value->lowKey == langEl->eofLel->id )
				st->advanceReductions = true;
		}
	}
}

void Compiler::verifyParseStopGrammar( LangEl *langEl, PdaGraph *pdaGraph )
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

LangEl *Compiler::predOf( PdaTrans *trans, long action )
{
	LangEl *lel;
	if ( action == SHIFT_CODE )
		lel = langElIndex[trans->lowKey];
	else
		lel = prodIdIndex[action >> 2]->predOf;
	return lel;
}


bool Compiler::precedenceSwap( long action1, long action2, LangEl *l1, LangEl *l2 )
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

bool Compiler::precedenceRemoveBoth( LangEl *l1, LangEl *l2 )
{
	if ( l1->predValue == l2->predValue && l1->predType == PredNonassoc )
		return true;
	return false;
}

void Compiler::resolvePrecedence( PdaGraph *pdaGraph )
{
	for ( PdaStateList::Iter state = pdaGraph->stateList; state.lte(); state++ ) {
		assert( CmpDotSet::compare( state->dotSet, state->dotSet2 ) == 0 );
		
		for ( long t = 0; t < state->transMap.length(); /* increment at end */ ) {
			PdaTrans *trans = state->transMap[t].value;

again:
			/* Find action with precedence. */
			for ( int i = 0; i < trans->actions.length(); i++ ) {
				LangEl *li = predOf( trans, trans->actions[i] );
					
				if ( li != 0 && li->predType != PredNone ) {
					/* Find another action with precedence. */
					for ( int j = i+1; j < trans->actions.length(); j++ ) {
						LangEl *lj = predOf( trans, trans->actions[j] );

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

void Compiler::analyzeMachine( PdaGraph *pdaGraph, LangElSet &parserEls )
{
	pdaGraph->maxState = pdaGraph->stateList.length() - 1;
	pdaGraph->maxLelId = nextLelId - 1;
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

	/* Verify that any type we parse_stop can actually be parsed that way. */
	for ( LangElSet::Iter pe = parserEls; pe.lte(); pe++ ) {
		LangEl *lel = *pe;
		if ( lel->parseStop )
			computeAdvanceReductions(lel , pdaGraph);
	}

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
			LangEl *langEl = langElIndex[trans->value->lowKey];
			if ( langEl != 0 && langEl->type == LangEl::NonTerm ) {
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
	for ( LangElSet::Iter pe = parserEls; pe.lte(); pe++ ) {
		LangEl *lel = *pe;
		if ( lel->parseStop )
			verifyParseStopGrammar(lel , pdaGraph);
	}
}

void Compiler::wrapNonTerminals()
{
	/* Make a language element that will be used to make the root productions.
	 * These are used for making parsers rooted at any production (including
	 * the start symbol). */
	rootLangEl = declareLangEl( this, rootNamespace, "_root", LangEl::NonTerm );

	for ( LelList::Iter lel = langEls; lel.lte(); lel++ ) {
		/* Make a single production used when the lel is a root. */
		ProdElList *prodElList = makeProdElList( lel );
		lel->rootDef = Production::cons( InputLoc(), rootLangEl, 
				prodElList, String(), false, 0,
				prodList.length(), rootLangEl->defList.length() );
		prodList.append( lel->rootDef );
		rootLangEl->defList.append( lel->rootDef );

		/* First resolve. */
		for ( ProdElList::Iter prodEl = *prodElList; prodEl.lte(); prodEl++ )
			resolveProdEl( prodEl );
	}
}

bool Compiler::makeNonTermFirstSetProd( Production *prod, PdaState *state )
{
	bool modified = false;
	for ( TransMap::Iter trans = state->transMap; trans.lte(); trans++ ) {
		if ( trans->key >= firstNonTermId ) {
			long *inserted = prod->nonTermFirstSet.insert( trans->key );
			if ( inserted != 0 )
				modified = true;

			bool hasEpsilon = false;
			LangEl *lel = langElIndex[trans->key];
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


void Compiler::makeNonTermFirstSets()
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

void Compiler::printNonTermFirstSets()
{
	for ( DefList::Iter prod = prodList; prod.lte(); prod++ ) {
		cerr << prod->data << ": ";
		for ( ProdIdSet::Iter pid = prod->nonTermFirstSet; pid.lte(); pid++ )
		{
			if ( *pid < 0 )
				cerr << " <EPSILON>";
			else {
				LangEl *lel = langElIndex[*pid];
				cerr << " " << lel->name;
			}
		}
		cerr << endl;

		if ( prod->isLeftRec )
			cerr << "PROD IS LEFT REC: " << prod->data << endl;
	}
}

bool Compiler::makeFirstSetProd( Production *prod, PdaState *state )
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

			LangEl *klangEl = langElIndex[trans->key];
			if ( klangEl != 0 && klangEl->termDup != 0 ) {
				long *inserted2 = prod->firstSet.insert( klangEl->termDup->id );
				if ( inserted2 != 0 )
					modified = true;
			}

			bool hasEpsilon = false;
			LangEl *lel = langElIndex[trans->key];
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


void Compiler::makeFirstSets()
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

void Compiler::printFirstSets()
{
	for ( DefList::Iter prod = prodList; prod.lte(); prod++ ) {
		cerr << prod->data << ": ";
		for ( ProdIdSet::Iter pid = prod->firstSet; pid.lte(); pid++ )
		{
			if ( *pid < 0 )
				cerr << " <EPSILON>";
			else {
				LangEl *lel = langElIndex[*pid];
				if ( lel != 0 ) 
					cerr << endl << "    " << lel->name;
				else
					cerr << endl << "    " << *pid;
			}
		}
		cerr << endl;
	}
}

void Compiler::insertUniqueEmptyProductions()
{
	int limit = prodList.length();
	for ( DefList::Iter prod = prodList; prod.lte(); prod++ ) {
		if ( prod->prodId == limit )
			break;

		/* Get a language element. */
		char name[20];
		sprintf(name, "U%li", prodList.length());
		LangEl *prodName = addLangEl( this, rootNamespace, name, LangEl::NonTerm );
		Production *newDef = Production::cons( InputLoc(), prodName, 
				0, String(), false, 0, prodList.length(), prodName->defList.length() );
		prodName->defList.append( newDef );
		prodList.append( newDef );

		prod->uniqueEmptyLeader = prodName;
	}
}

struct local_info *Compiler::makeLocalInfo( Locals &locals )
{
	struct local_info *localInfo = new local_info[locals.locals.length()];
	memset( localInfo, 0, sizeof(struct local_info) * locals.locals.length() );

	for ( Vector<LocalLoc>::Iter l = locals.locals; l.lte(); l++ ) {
		localInfo[l.pos()].type = (int) l->type;
		localInfo[l.pos()].offset = l->offset;
	}
	return localInfo;
}

short *Compiler::makeTrees( ObjectDef *objectDef, int &numTrees )
{
	numTrees = 0;
	for ( FieldList::Iter of = objectDef->fieldList; of.lte(); of++ ) {
		if ( of->value->exists() ) {
			UniqueType *ut = of->value->typeRef->resolveType( this );
			if ( ut->typeId == TYPE_TREE )
				numTrees += 1;
		}
	}

	short *trees = new short[numTrees];
	memset( trees, 0, sizeof(short) * numTrees );

	short pos = 0;
	for ( FieldList::Iter of = objectDef->fieldList; of.lte(); of++ ) {
		if ( of->value->exists() ) {
			UniqueType *ut = of->value->typeRef->resolveType( this );
			if ( ut->typeId == TYPE_TREE ) {
				trees[pos] = of->value->offset;
				pos += 1;
			}
		}
	}

	return trees;
}


void Compiler::makeRuntimeData()
{
	long count = 0;

	/*
	 * ProdLengths
	 * ProdLhsIs
	 * ProdNames
	 * ProdCodeBlocks
	 * ProdCodeBlockLens
	 */

	runtimeData->frame_info = new frame_info[nextFrameId];
	runtimeData->num_frames = nextFrameId;
	memset( runtimeData->frame_info, 0, sizeof(struct frame_info) * nextFrameId );

	/*
	 * Init code block.
	 */
	if ( rootCodeBlock == 0 ) {
		runtimeData->root_code = 0;
		runtimeData->root_code_len = 0;
		runtimeData->root_frame_id = 0;
	}
	else {
		runtimeData->root_code = rootCodeBlock->codeWC.data;
		runtimeData->root_code_len = rootCodeBlock->codeWC.length();
		runtimeData->root_frame_id = rootCodeBlock->frameId;
	}

	runtimeData->frame_info[rootCodeBlock->frameId].codeWV = 0;
	runtimeData->frame_info[rootCodeBlock->frameId].codeLenWV = 0;

	runtimeData->frame_info[rootCodeBlock->frameId].locals = makeLocalInfo( rootCodeBlock->locals );
	runtimeData->frame_info[rootCodeBlock->frameId].locals_len = rootCodeBlock->locals.locals.length();

	runtimeData->frame_info[rootCodeBlock->frameId].frame_size = rootLocalFrame->size();
	runtimeData->frame_info[rootCodeBlock->frameId].arg_size = 0;
	runtimeData->frame_info[rootCodeBlock->frameId].ret_tree = false;

	/*
	 * prodInfo
	 */
	count = prodList.length();
	runtimeData->prod_info = new prod_info[count];
	runtimeData->num_prods = count;

	count = 0;
	for ( DefList::Iter prod = prodList; prod.lte(); prod++ ) {
		runtimeData->prod_info[count].lhs_id = prod->prodName->id;
		runtimeData->prod_info[count].prod_num = prod->prodNum;
		runtimeData->prod_info[count].length = prod->fsmLength;
		runtimeData->prod_info[count].name = prod->data;
		runtimeData->prod_info[count].frame_id = -1;

		CodeBlock *block = prod->redBlock;
		if ( block != 0 ) {
			runtimeData->prod_info[count].frame_id = block->frameId;
			runtimeData->frame_info[block->frameId].codeWV = block->codeWV.data;
			runtimeData->frame_info[block->frameId].codeLenWV = block->codeWV.length();

			runtimeData->frame_info[block->frameId].locals = makeLocalInfo( block->locals );
			runtimeData->frame_info[block->frameId].locals_len = block->locals.locals.length();

			runtimeData->frame_info[block->frameId].frame_size = block->localFrame->size();
			runtimeData->frame_info[block->frameId].arg_size = 0;
			runtimeData->frame_info[block->frameId].ret_tree = false;
		}

		runtimeData->prod_info[count].lhs_upref = true;
		runtimeData->prod_info[count].copy = prod->copy.data;
		runtimeData->prod_info[count].copy_len = prod->copy.length() / 2;
		count += 1;
	}

	/*
	 * regionInfo
	 */
	runtimeData->num_regions = regionList.length()+1;
	runtimeData->region_info = new region_info[runtimeData->num_regions];
	memset( runtimeData->region_info, 0,
			sizeof(struct region_info) * runtimeData->num_regions );

	runtimeData->region_info[0].default_token = -1;
	runtimeData->region_info[0].eof_frame_id = -1;
	runtimeData->region_info[0].ci_lel_id = 0;

	for ( RegionList::Iter reg = regionList; reg.lte(); reg++ ) {
		long regId = reg->id+1;
		runtimeData->region_info[regId].default_token =
				reg->impl->defaultTokenInstance == 0 ?
				-1 : 
				reg->impl->defaultTokenInstance->tokenDef->tdLangEl->id;
		runtimeData->region_info[regId].eof_frame_id = -1;
		runtimeData->region_info[regId].ci_lel_id = reg->zeroLel != 0 ? reg->zeroLel->id : 0;

		CodeBlock *block = reg->preEofBlock;
		if ( block != 0 ) {
			runtimeData->region_info[regId].eof_frame_id = block->frameId;
			runtimeData->frame_info[block->frameId].codeWV = block->codeWV.data;
			runtimeData->frame_info[block->frameId].codeLenWV = block->codeWV.length();

			runtimeData->frame_info[block->frameId].locals = makeLocalInfo( block->locals );
			runtimeData->frame_info[block->frameId].locals_len = block->locals.locals.length();

			runtimeData->frame_info[block->frameId].frame_size = block->localFrame->size();
			runtimeData->frame_info[block->frameId].arg_size = 0;
			runtimeData->frame_info[block->frameId].ret_tree = false;
		}
	}

	/*
	 * lelInfo
	 */

	count = nextLelId;
	runtimeData->lel_info = new lang_el_info[count];
	runtimeData->num_lang_els = count;
	memset( runtimeData->lel_info, 0, sizeof(struct lang_el_info)*count );

	for ( int i = 0; i < nextLelId; i++ ) {
		LangEl *lel = langElIndex[i];
		if ( lel != 0 ) {
			runtimeData->lel_info[i].name = lel->fullLit;
			runtimeData->lel_info[i].xml_tag = lel->xmlTag;
			runtimeData->lel_info[i].repeat = lel->isRepeat;
			runtimeData->lel_info[i].list = lel->isList;
			runtimeData->lel_info[i].literal = lel->isLiteral;
			runtimeData->lel_info[i].ignore = lel->isIgnore;
			runtimeData->lel_info[i].frame_id = -1;

			CodeBlock *block = lel->transBlock;
			if ( block != 0 ) {
				runtimeData->lel_info[i].frame_id = block->frameId;
				runtimeData->frame_info[block->frameId].codeWV = block->codeWV.data;
				runtimeData->frame_info[block->frameId].codeLenWV = block->codeWV.length();

				runtimeData->frame_info[block->frameId].locals = makeLocalInfo( block->locals );
				runtimeData->frame_info[block->frameId].locals_len = block->locals.locals.length();

				runtimeData->frame_info[block->frameId].frame_size = block->localFrame->size();
				runtimeData->frame_info[block->frameId].arg_size = 0;
				runtimeData->frame_info[block->frameId].ret_tree = false;
			}
			
			runtimeData->lel_info[i].object_type_id = 
					lel->objectDef == 0 ? 0 : lel->objectDef->id;
			runtimeData->lel_info[i].ofi_offset = lel->ofiOffset;
			runtimeData->lel_info[i].object_length = 
					lel->objectDef != 0 ? lel->objectDef->size() : 0;

//			runtimeData->lelInfo[i].contextTypeId = 0;
//					lel->context == 0 ? 0 : lel->context->contextObjDef->id;
//			runtimeData->lelInfo[i].contextLength = 0; //lel->context == 0 ? 0 :
//					lel->context->contextObjDef->size();
//			if ( lel->context != 0 ) {
//				cout << "type: " << runtimeData->lelInfo[i].contextTypeId << " length: " << 
//					runtimeData->lelInfo[i].contextLength << endl;
//			}

			runtimeData->lel_info[i].term_dup_id = lel->termDup == 0 ? 0 : lel->termDup->id;

			if ( lel->tokenDef != 0 && lel->tokenDef->join != 0 && 
					lel->tokenDef->join->context != 0 )
				runtimeData->lel_info[i].mark_id = lel->tokenDef->join->mark->markId;
			else
				runtimeData->lel_info[i].mark_id = -1;

			runtimeData->lel_info[i].num_capture_attr = 0;
		}
		else {
			memset(&runtimeData->lel_info[i], 0, sizeof(struct lang_el_info) );
			runtimeData->lel_info[i].name = "__UNUSED";
			runtimeData->lel_info[i].xml_tag = "__UNUSED";
			runtimeData->lel_info[i].frame_id = -1;
		}
	}

	/*
	 * struct_el_info
	 */

	count = structEls.length();
	runtimeData->sel_info = new struct_el_info[count];
	runtimeData->num_struct_els = count;
	memset( runtimeData->sel_info, 0, sizeof(struct struct_el_info)*count );
	StructElList::Iter sel = structEls;
	for ( int i = 0; i < count; i++, sel++ ) {
		int treesLen;
		runtimeData->sel_info[i].size = sel->structDef->objectDef->size();
		runtimeData->sel_info[i].trees = makeTrees( sel->structDef->objectDef, treesLen );
		runtimeData->sel_info[i].trees_len = treesLen;
	}

	/*
	 * function_info
	 */
	count = functionList.length();

	runtimeData->function_info = new function_info[count];
	runtimeData->num_functions = count;
	memset( runtimeData->function_info, 0, sizeof(struct function_info)*count );
	for ( FunctionList::Iter func = functionList; func.lte(); func++ ) {

		runtimeData->function_info[func->funcId].frame_id = -1;

		CodeBlock *block = func->codeBlock;
		if ( block != 0 ) {
			runtimeData->function_info[func->funcId].frame_id = block->frameId;

			/* Name. */
			runtimeData->frame_info[block->frameId].name = func->name;

			/* Code. */
			runtimeData->frame_info[block->frameId].codeWV = block->codeWV.data;
			runtimeData->frame_info[block->frameId].codeLenWV = block->codeWV.length();
			runtimeData->frame_info[block->frameId].codeWC = block->codeWC.data;
			runtimeData->frame_info[block->frameId].codeLenWC = block->codeWC.length();

			/* Locals. */
			runtimeData->frame_info[block->frameId].locals = makeLocalInfo( block->locals );
			runtimeData->frame_info[block->frameId].locals_len = block->locals.locals.length();

			/* Meta. */
			runtimeData->frame_info[block->frameId].frame_size = func->localFrame->size();
			runtimeData->frame_info[block->frameId].arg_size = func->paramListSize;

			bool retTree = false;
			if ( func->typeRef ) {
				UniqueType *ut = func->typeRef->resolveType( this );
				retTree = ut->tree();
			}
			runtimeData->frame_info[block->frameId].ret_tree = retTree;
		}

		runtimeData->function_info[func->funcId].frame_size = func->localFrame->size();
		runtimeData->function_info[func->funcId].arg_size = func->paramListSize;
	}

	/*
	 * pat_cons_info
	 */

	/* Filled in later after patterns are parsed. */
	runtimeData->pat_repl_info = new pat_cons_info[nextPatConsId];
	memset( runtimeData->pat_repl_info, 0, sizeof(struct pat_cons_info) * nextPatConsId );
	runtimeData->num_patterns = nextPatConsId;
	runtimeData->pat_repl_nodes = 0;
	runtimeData->num_pattern_nodes = 0;

	
	/*
	 * generic_info
	 */
	count = 1;
	for ( NamespaceList::Iter nspace = namespaceList; nspace.lte(); nspace++ )
		count += nspace->genericList.length();
	assert( count == nextGenericId );

	runtimeData->generic_info = new generic_info[count];
	runtimeData->num_generics = count;
	memset( &runtimeData->generic_info[0], 0, sizeof(struct generic_info) );
	for ( NamespaceList::Iter nspace = namespaceList; nspace.lte(); nspace++ ) {
		for ( GenericList::Iter gen = nspace->genericList; gen.lte(); gen++ ) {
			runtimeData->generic_info[gen->id].type = gen->typeId;

			runtimeData->generic_info[gen->id].el_struct_id =
					( gen->typeId == GEN_MAP || gen->typeId == GEN_LIST ) ?
					gen->elUt->structEl->id : -1;
			runtimeData->generic_info[gen->id].el_offset =
					gen->el != 0 ? gen->el->offset : -1;

			runtimeData->generic_info[gen->id].key_type =
					gen->keyUt != 0 ? gen->keyUt->typeId : TYPE_NOTYPE;
			runtimeData->generic_info[gen->id].key_offset = 0;

			runtimeData->generic_info[gen->id].value_type =
					gen->valueUt != 0 ? gen->valueUt->typeId : TYPE_NOTYPE;
			runtimeData->generic_info[gen->id].value_offset = 0;

			runtimeData->generic_info[gen->id].parser_id =
					gen->typeId == GEN_PARSER ? gen->elUt->langEl->parserId : -1;
		}
	}

	runtimeData->argv_generic_id = argvTypeRef->generic->id;

	/*
	 * Literals
	 */
	runtimeData->num_literals = literalStrings.length();
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
//	for ( RegionList::Iter reg = regionList; reg.lte(); reg++ ) {
//		for ( TokenInstanceListReg::Iter td = reg->tokenInstanceList; td.lte(); td++ )
//			numCapturedAttr += td->reCaptureVect.length();
//	}
	runtimeData->capture_attr = new CaptureAttr[numCapturedAttr];
	runtimeData->num_captured_attr = numCapturedAttr;
	memset( runtimeData->capture_attr, 0, sizeof( CaptureAttr ) * numCapturedAttr );

	count = 0;
//	for ( RegionList::Iter reg = regionList; reg.lte(); reg++ ) {
//		for ( TokenInstanceListReg::Iter td = reg->tokenInstanceList; td.lte(); td++ ) {
//			runtimeData->lelInfo[td->token->id].captureAttr = count;
//			runtimeData->lelInfo[td->token->id].numCaptureAttr = td->reCaptureVect.length();
//			for ( ReCaptureVect::Iter c = td->reCaptureVect; c.lte(); c++ ) {
//				runtimeData->captureAttr[count].mark_enter = c->markEnter->markId;
//				runtimeData->captureAttr[count].mark_leave = c->markLeave->markId;
//				runtimeData->captureAttr[count].offset = c->objField->offset;
//
//				count += 1;
//			}
//		}
//	}

	runtimeData->fsm_tables = fsmTables;
	runtimeData->pda_tables = pdaTables;

	/* FIXME: need a parser descriptor. */
	runtimeData->start_states = new int[nextParserId];
	runtimeData->eof_lel_ids = new int[nextParserId];
	runtimeData->parser_lel_ids = new int[nextParserId];
	runtimeData->num_parsers = nextParserId;
	for ( LelList::Iter lel = langEls; lel.lte(); lel++ ) {
		if ( lel->parserId >= 0 ) {
			runtimeData->start_states[lel->parserId] = lel->startState->stateNum;
			runtimeData->eof_lel_ids[lel->parserId] = lel->eofLel->id;
			runtimeData->parser_lel_ids[lel->parserId] = lel->id;
		}
	}

	runtimeData->global_size = globalObjectDef->size();

	/*
	 * firstNonTermId
	 */
	runtimeData->first_non_term_id = firstNonTermId;

	/* Special trees. */
	runtimeData->integer_id = -1; //intLangEl->id;
	runtimeData->string_id = strLangEl->id;
	runtimeData->any_id = anyLangEl->id;
	runtimeData->eof_id = 0; //eofLangEl->id;
	runtimeData->no_token_id = noTokenLangEl->id;
	runtimeData->global_id = globalSel->id;
	runtimeData->argv_el_id = argvElSel->id;

	runtimeData->fsm_execute = &internalFsmExecute;
	runtimeData->send_named_lang_el = &internalSendNamedLangEl;
	runtimeData->init_bindings = &internalInitBindings;
	runtimeData->pop_binding = &internalPopBinding;

	runtimeData->host_call = &internal_host_call;
	runtimeData->commit_reduce_forward = &internal_commit_reduce_forward;
	runtimeData->commit_union_sz = &internal_commit_union_sz;
	runtimeData->init_need = &internal_init_need;
	runtimeData->reducer_need_tok = &internal_reducer_need_tok;
	runtimeData->reducer_need_ign = &internal_reducer_need_ign;
}

/* Borrow alg->state for mapsTo. */
void countNodes( program_t *prg, int &count, parse_tree_t *parseTree, kid_t *kid )
{
	if ( kid != 0 ) {
		count += 1;

		/* Should't have to recurse here. */
		tree_t *ignoreList = tree_left_ignore( prg, kid->tree );
		if ( ignoreList != 0 ) {
			kid_t *ignore = ignoreList->child;
			while ( ignore != 0 ) {
				count += 1;
				ignore = ignore->next;
			}
		}

		ignoreList = tree_right_ignore( prg, kid->tree );
		if ( ignoreList != 0 ) {
			kid_t *ignore = ignoreList->child;
			while ( ignore != 0 ) {
				count += 1;
				ignore = ignore->next;
			}
		}
		
		//count += prg->rtd->lelInfo[kid->tree->id].numCaptureAttr;

		if ( !( parseTree->flags & PF_NAMED ) && 
				!( parseTree->flags & PF_ARTIFICIAL ) && 
				tree_child( prg, kid->tree ) != 0 )
		{
			countNodes( prg, count, parseTree->child, tree_child( prg, kid->tree ) );
		}
		countNodes( prg, count, parseTree->next, kid->next );
	}
}

void fillNodes( program_t *prg, int &nextAvail, struct bindings *bindings, long &bindId, 
		struct pat_cons_node *nodes, parse_tree_t *parseTree, kid_t *kid, int ind )
{
	if ( kid != 0 ) {
		struct pat_cons_node &node = nodes[ind];

		kid_t *child = 
			!( parseTree->flags & PF_NAMED ) && 
			!( parseTree->flags & PF_ARTIFICIAL ) && 
			tree_child( prg, kid->tree ) != 0 
			?
			tree_child( prg, kid->tree ) : 0;

		parse_tree_t *ptChild =
			!( parseTree->flags & PF_NAMED ) && 
			!( parseTree->flags & PF_ARTIFICIAL ) && 
			tree_child( prg, kid->tree ) != 0 
			?
			parseTree->child : 0;

		/* Set up the fields. */
		node.id = kid->tree->id;
		node.prod_num = kid->tree->prod_num;
		node.length = string_length( kid->tree->tokdata );
		node.data = string_data( kid->tree->tokdata );

		/* Ignore items. */
		tree_t *ignoreList = tree_left_ignore( prg, kid->tree );
		kid_t *ignore = ignoreList == 0 ? 0 : ignoreList->child;
		node.left_ignore = ignore == 0 ? -1 : nextAvail;

		while ( ignore != 0 ) {
			struct pat_cons_node &node = nodes[nextAvail++];

			memset( &node, 0, sizeof(struct pat_cons_node) );
			node.id = ignore->tree->id;
			node.prod_num = ignore->tree->prod_num;
			node.next = ignore->next == 0 ? -1 : nextAvail;
				
			node.length = string_length( ignore->tree->tokdata );
			node.data = string_data( ignore->tree->tokdata );

			ignore = ignore->next;
		}

		/* Ignore items. */
		ignoreList = tree_right_ignore( prg, kid->tree );
		ignore = ignoreList == 0 ? 0 : ignoreList->child;
		node.right_ignore = ignore == 0 ? -1 : nextAvail;

		while ( ignore != 0 ) {
			struct pat_cons_node &node = nodes[nextAvail++];

			memset( &node, 0, sizeof(struct pat_cons_node) );
			node.id = ignore->tree->id;
			node.prod_num = ignore->tree->prod_num;
			node.next = ignore->next == 0 ? -1 : nextAvail;
				
			node.length = string_length( ignore->tree->tokdata );
			node.data = string_data( ignore->tree->tokdata );

			ignore = ignore->next;
		}

		///* The captured attributes. */
		//for ( int i = 0; i < prg->rtd->lelInfo[kid->tree->id].numCaptureAttr; i++ ) {
		//	CaptureAttr *cap = prg->rtd->captureAttr + 
		//			prg->rtd->lelInfo[kid->tree->id].captureAttr + i;
		//
		//	tree_t *attr = colm_get_attr( kid->tree, cap->offset );
		//
		//	struct pat_cons_node &node = nodes[nextAvail++];
		//	memset( &node, 0, sizeof(struct pat_cons_node) );
		//
		//	node.id = attr->id;
		//	node.prodNum = attr->prodNum;
		//	node.length = stringLength( attr->tokdata );
		//	node.data = stringData( attr->tokdata );
		//}

		node.stop = parseTree->flags & PF_TERM_DUP;

		node.child = child == 0 ? -1 : nextAvail++; 

		/* Recurse. */
		fillNodes( prg, nextAvail, bindings, bindId, nodes, ptChild, child, node.child );

		/* Since the parser is bottom up the bindings are in a bottom up
		 * traversal order. Check after recursing. */
		node.bind_id = 0;
		if ( bindId < bindings->length() && bindings->data[bindId] == parseTree ) {
			/* Remember that binding ids are indexed from one. */
			node.bind_id = bindId++;

			//cout << "binding match in " << __PRETTY_FUNCTION__ << endl;
			//cout << "bindId: " << node.bindId << endl;
		}

		node.next = kid->next == 0 ? -1 : nextAvail++; 

		/* Move to the next child. */
		fillNodes( prg, nextAvail, bindings, bindId, nodes, parseTree->next, kid->next, node.next );
	}
}

void Compiler::fillInPatterns( program_t *prg )
{
	/*
	 * patReplNodes
	 */

	/* Count is referenced and computed by mapNode. */
	int count = 0;
	for ( PatList::Iter pat = patternList; pat.lte(); pat++ ) {
		countNodes( prg, count, 
				pat->pdaRun->stack_top->next,
				pat->pdaRun->stack_top->next->shadow );
	}

	for ( ConsList::Iter repl = replList; repl.lte(); repl++ ) {
		countNodes( prg, count, 
				repl->pdaRun->stack_top->next,
				repl->pdaRun->stack_top->next->shadow );
	}
	
	runtimeData->pat_repl_nodes = new pat_cons_node[count];
	runtimeData->num_pattern_nodes = count;

	int nextAvail = 0;

	for ( PatList::Iter pat = patternList; pat.lte(); pat++ ) {
		int ind = nextAvail++;
		runtimeData->pat_repl_info[pat->patRepId].offset = ind;

		/* BindIds are indexed base one. */
		runtimeData->pat_repl_info[pat->patRepId].num_bindings = 
				pat->pdaRun->bindings->length() - 1;

		/* Init the bind */
		long bindId = 1;
		fillNodes( prg, nextAvail, pat->pdaRun->bindings, bindId,
				runtimeData->pat_repl_nodes, 
				pat->pdaRun->stack_top->next, 
				pat->pdaRun->stack_top->next->shadow, 
				ind );
	}

	for ( ConsList::Iter repl = replList; repl.lte(); repl++ ) {
		int ind = nextAvail++;
		runtimeData->pat_repl_info[repl->patRepId].offset = ind;

		/* BindIds are indexed base one. */
		runtimeData->pat_repl_info[repl->patRepId].num_bindings = 
				repl->pdaRun->bindings->length() - 1;

		long bindId = 1;
		fillNodes( prg, nextAvail, repl->pdaRun->bindings, bindId,
				runtimeData->pat_repl_nodes, 
				repl->pdaRun->stack_top->next,
				repl->pdaRun->stack_top->next->shadow, 
				ind );
	}

	assert( nextAvail == count );
}


int Compiler::findIndexOff( struct pda_tables *pdaTables, PdaGraph *pdaGraph, PdaState *state, int &curLen )
{
	for ( int start = 0; start < curLen;  ) {
		int offset = start;
		for ( TransMap::Iter trans = state->transMap; trans.lte(); trans++ ) {
			if ( pdaTables->owners[offset] != -1 )
				goto next_start;

			offset++;
			if ( ! trans.last() ) {
				TransMap::Iter next = trans.next();
				offset += next->key - trans->key - 1;
			}
		}

		/* Got though the whole list without a conflict. */
		return start;

next_start:
		start++;
	}

	return curLen;
}

struct CmpSpan
{
	static int compare( PdaState *state1, PdaState *state2 )
	{
		int dist1 = 0, dist2 = 0;

		if ( state1->transMap.length() > 0 ) {
			TransMap::Iter first1 = state1->transMap.first();
			TransMap::Iter last1 = state1->transMap.last();
			dist1 = last1->key - first1->key;
		}

		if ( state2->transMap.length() > 0 ) {
			TransMap::Iter first2 = state2->transMap.first();
			TransMap::Iter last2 = state2->transMap.last();
			dist2 = last2->key - first2->key;
		}

		if ( dist1 < dist2 )
			return 1;
		else if ( dist2 < dist1 )
			return -1;
		return 0;
	}
};

PdaGraph *Compiler::makePdaGraph( LangElSet &parserEls )
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

struct pda_tables *Compiler::makePdaTables( PdaGraph *pdaGraph )
{
	int count, pos;
	struct pda_tables *pdaTables = new pda_tables;

	/*
	 * Counting max indices.
	 */
	count = 0;
	for ( PdaStateList::Iter state = pdaGraph->stateList; state.lte(); state++ ) {
		for ( TransMap::Iter trans = state->transMap; trans.lte(); trans++ ) {
			count++;
			if ( ! trans.last() ) {
				TransMap::Iter next = trans.next();
				count +=  next->key - trans->key - 1;
			}
		}
	}


	/* Allocate indicies and owners. */
	pdaTables->num_indicies = count;
	pdaTables->indicies = new int[count];
	pdaTables->owners = new int[count];
	for ( long i = 0; i < count; i++ ) {
		pdaTables->indicies[i] = -1;
		pdaTables->owners[i] = -1;
	}

	/* Allocate offsets. */
	int numStates = pdaGraph->stateList.length(); 
	pdaTables->offsets = new unsigned int[numStates];
	pdaTables->num_states = numStates;

	/* Place transitions into indicies/owners */
	PdaState **states = new PdaState*[numStates];
	long ds = 0;
	for ( PdaStateList::Iter state = pdaGraph->stateList; state.lte(); state++ )
		states[ds++] = state;

	/* Sorting baseded on span length. Gives an improvement, but incures a
	 * cost. Off for now. */
	//MergeSort< PdaState*, CmpSpan > mergeSort;
	//mergeSort.sort( states, numStates );
	
	int indLen = 0;
	for ( int s = 0; s < numStates; s++ ) {
		PdaState *state = states[s];

		int indOff = findIndexOff( pdaTables, pdaGraph, state, indLen );
		pdaTables->offsets[state->stateNum] = indOff;

		for ( TransMap::Iter trans = state->transMap; trans.lte(); trans++ ) {
			pdaTables->indicies[indOff] = trans->value->actionSetEl->key.id;
			pdaTables->owners[indOff] = state->stateNum;
			indOff++;

			if ( ! trans.last() ) {
				TransMap::Iter next = trans.next();
				indOff += next->key - trans->key - 1;
			}
		}

		if ( indOff > indLen )
			indLen = indOff;
	}

	/* We allocated the max, but cmpression gives us less. */
	pdaTables->num_indicies = indLen;
	delete[] states;
	

	/*
	 * Keys
	 */
	count = pdaGraph->stateList.length() * 2;;
	pdaTables->keys = new int[count];
	pdaTables->num_keys = count;

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
	 * Targs
	 */
	count = pdaGraph->actionSet.length();
	pdaTables->targs = new unsigned int[count];
	pdaTables->num_targs = count;

	count = 0;
	for ( PdaActionSet::Iter asi = pdaGraph->actionSet; asi.lte(); asi++ )
		pdaTables->targs[count++] = asi->key.targ;

	/* 
	 * ActInds
	 */
	count = pdaGraph->actionSet.length();
	pdaTables->act_inds = new unsigned int[count];
	pdaTables->num_act_inds = count;

	count = pos = 0;
	for ( PdaActionSet::Iter asi = pdaGraph->actionSet; asi.lte(); asi++ ) {
		pdaTables->act_inds[count++] = pos;
		pos += asi->key.actions.length() + 1;
	}

	/*
	 * Actions
	 */
	count = 0;
	for ( PdaActionSet::Iter asi = pdaGraph->actionSet; asi.lte(); asi++ )
		count += asi->key.actions.length() + 1;

	pdaTables->actions = new unsigned int[count];
	pdaTables->num_actions = count;

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
	pdaTables->commit_len = new int[count];
	pdaTables->num_commit_len = count;

	count = 0;
	for ( PdaActionSet::Iter asi = pdaGraph->actionSet; asi.lte(); asi++ )
		pdaTables->commit_len[count++] = asi->key.commitLen;
	
	/*
	 * tokenRegionInds. Start at one so region index 0 is null (unset).
	 */
	count = 0;
	pos = 1;
	pdaTables->token_region_inds = new int[pdaTables->num_states];
	for ( PdaStateList::Iter state = pdaGraph->stateList; state.lte(); state++ ) {
		pdaTables->token_region_inds[count++] = pos;
		pos += state->regions.length() + 1;
	}


	/*
	 * tokenRegions. Build in a null at the beginning.
	 */

	count = 1;
	for ( PdaStateList::Iter state = pdaGraph->stateList; state.lte(); state++ )
		count += state->regions.length() + 1;

	pdaTables->num_region_items = count;
	pdaTables->token_regions = new int[pdaTables->num_region_items];

	count = 0;
	pdaTables->token_regions[count++] = 0;
	for ( PdaStateList::Iter state = pdaGraph->stateList; state.lte(); state++ ) {
		for ( RegionVect::Iter reg = state->regions; reg.lte(); reg++ ) {
			int id = ( *reg == EOF_REGION ) ? 0 : (*reg)->id + 1;
			pdaTables->token_regions[count++] = id;
		}

		pdaTables->token_regions[count++] = 0;
	}

	/*
	 * tokenPreRegions. Build in a null at the beginning.
	 */

	count = 1;
	for ( PdaStateList::Iter state = pdaGraph->stateList; state.lte(); state++ )
		count += state->regions.length() + 1;

	pdaTables->num_pre_region_items = count;
	pdaTables->token_pre_regions = new int[pdaTables->num_pre_region_items];

	count = 0;
	pdaTables->token_pre_regions[count++] = 0;
	for ( PdaStateList::Iter state = pdaGraph->stateList; state.lte(); state++ ) {
		for ( RegionVect::Iter reg = state->regions; reg.lte(); reg++ ) {
			assert( state->preRegions.length() <= 1 );
			if ( state->preRegions.length() == 0 || state->preRegions[0]->impl->wasEmpty )
				pdaTables->token_pre_regions[count++] = -1;
			else 
				pdaTables->token_pre_regions[count++] = state->preRegions[0]->id + 1;
		}

		pdaTables->token_pre_regions[count++] = 0;
	}


	return pdaTables;
}

void Compiler::makeParser( LangElSet &parserEls )
{
	pdaGraph = makePdaGraph( parserEls );
	pdaTables = makePdaTables( pdaGraph );
}

