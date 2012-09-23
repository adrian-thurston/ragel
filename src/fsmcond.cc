/*
 *  Copyright 2001, 2002, 2006, 2011 Adrian Thurston <thurston@complang.org>
 */

/*  This file is part of Ragel.
 *
 *  Ragel is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 * 
 *  Ragel is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 * 
 *  You should have received a copy of the GNU General Public License
 *  along with Ragel; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA 
 */

/*
 * Setting conditions and merging states with conditions are similar activities
 * when expressed in code. The critical difference is that a merge is a union
 * of multiple paths. We have to take both paths. Setting a condition, however,
 * is a restriction. We have to expand the transition to follow both values of
 * the condition, then remove the one that is not set.
 */

#include <assert.h>
#include <iostream>

#include "fsmgraph.h"
#include "mergesort.h"
#include "parsedata.h"

using std::cerr;
using std::endl;
using std::cout;

void logNewExpansion( Expansion *exp );
void logCondSpace( CondSpace *condSpace );

void FsmAp::expandConds( StateAp *fromState, TransAp *trans, const CondSet &fromCS, const CondSet &mergedCS )
{
	/*CondSpace *mergedCondSpace = */addCondSpace( mergedCS );

	CondSet newCS = mergedCS;
	for ( CondSet::Iter mcsi = fromCS; mcsi.lte(); mcsi++ )
		newCS.remove( *mcsi );
	/*long newLen = */newCS.length();


	/* Need to transform condition element to the merged set. */
	for ( CondTransList::Iter cti = trans->ctList; cti.lte(); cti++ ) {
		long origVal = cti->lowKey.getVal();
		long newVal = 0;

		/* Iterate the bit positions in the from set. */
		for ( CondSet::Iter csi = fromCS; csi.lte(); csi++ ) {
			/* If set, find it in the merged set and flip the bit to 1. */
			if ( origVal & (1 << csi.pos()) ) {
				/* The condition is set. Find the bit position in the merged
				 * set. */
				Action **cim = mergedCS.find( *csi );
				long bitPos = (cim - mergedCS.data);
				newVal |= 1 << bitPos;
			}
		}

		if ( origVal != newVal ) {
			cout << "orig: " << origVal << " new: " << newVal << endl;
			cti->lowKey = cti->highKey = newVal;
		}
	}

	/* Need to double up the whole transition list for each condition test in
	 * merged that is not in from. The one we add has the bit in question set.
	 * */
	for ( CondSet::Iter csi = mergedCS; csi.lte(); csi++ ) {
		Action **cim = fromCS.find( *csi );
		if ( cim == 0 ) {
			CondTransList newItems;
			cout << "doubling up on condition" << endl;
			for ( CondTransList::Iter cti = trans->ctList; cti.lte(); cti++ ) {
				CondAp *cond = dupCondTrans( fromState, trans, cti  );

				cond->lowKey = cond->highKey = cti->lowKey.getVal() | (1 << csi.pos());

				newItems.append( cond );
			}

			trans->ctList.append( newItems );
		}
	}
#if 0
#endif

#if 0
	cout << "newCS.length() = " << newCS.length() << endl;

	if ( newCS.length() > 0 ) {
		#ifdef LOG_CONDS
		cerr << "there are " << newCS.length() << " item(s) that are "
					"only in the mergedCS" << endl;
		#endif

		long fromLen = fromCS.length();
	
		/* Loop all values in the dest space. */
		for ( long fromBits = 0; fromBits < (1 << fromLen); fromBits++ ) {
			long basicVals = 0;
			for ( CondSet::Iter csi = fromCS; csi.lte(); csi++ ) {
				if ( fromBits & (1 << csi.pos()) ) {
					Action **cim = mergedCS.find( *csi );
					long bitPos = (cim - mergedCS.data);
					basicVals |= 1 << bitPos;
				}
			}

			cerr << "basicVals: " << basicVals << endl;
	
			/* Loop all new values. */
			LongVect expandToVals;
			for ( long newVals = 0; newVals < (1 << newLen); newVals++ ) {
				long targVals = basicVals;
				for ( CondSet::Iter csi = newCS; csi.lte(); csi++ ) {
					if ( newVals & (1 << csi.pos()) ) {
						Action **cim = mergedCS.find( *csi );
						long bitPos = (cim - mergedCS.data);
						targVals |= 1 << bitPos;
					}
				}
				cerr << "targVals: " << targVals << endl;
				expandToVals.append( targVals );
			}
	
//			findCondExpInTrans( expansionList, destState, 
//					condCond.s1Tel.lowKey, condCond.s1Tel.highKey, 
//					fromCondSpace, toCondSpace, destVals, expandToVals );
		}
	}
#endif
}

void FsmAp::expandCondTransitions( StateAp *fromState, TransAp *destTrans, TransAp *srcTrans )
{
	CondSet destCS, srcCS;
	CondSet mergedCS;

	if ( destTrans->condSpace != 0 )
		destCS.insert( destTrans->condSpace->condSet );

	if ( srcTrans->condSpace != 0 )
		srcCS.insert( srcTrans->condSpace->condSet );
	
	mergedCS.insert( destCS );
	mergedCS.insert( srcCS );

	expandConds( fromState, destTrans, destCS, mergedCS );
	expandConds( fromState, srcTrans, srcCS, mergedCS );

	CondSpace *mergedCondSpace = addCondSpace( mergedCS );
	destTrans->condSpace = mergedCondSpace;
}

void FsmAp::expansionTrans( Expansion *expansion, TransAp *src )
{
	expansion->fromTrans = new TransAp(*src);
	CondAp *condTransAp = new CondAp( *src->ctList.head, expansion->fromTrans );
	expansion->fromTrans->ctList.append( condTransAp );
	expansion->fromTrans->ctList.head->fromState = 0;
	expansion->fromTrans->ctList.head->toState = src->ctList.head->toState;
}

void FsmAp::findTransExpansions( ExpansionList &expansionList, 
		StateAp *destState, StateAp *srcState )
{
	RangePairIter<TransAp, StateCond> transCond( destState->outList.head,
			srcState->stateCondList.head );
	for ( ; !transCond.end(); transCond++ ) {
		if ( transCond.userState == RangePairIter<TransAp, StateCond>::RangeOverlap ) {
			Expansion *expansion = new Expansion( transCond.s1Tel.lowKey, 
					transCond.s1Tel.highKey );

			expansionTrans( expansion, transCond.s1Tel.trans );

			expansion->fromCondSpace = 0;
			expansion->fromVals = 0;
			CondSpace *srcCS = transCond.s2Tel.trans->condSpace;
			expansion->toCondSpace = srcCS;

			long numTargVals = (1 << srcCS->condSet.length());
			for ( long targVals = 0; targVals < numTargVals; targVals++ )
				expansion->toValsList.append( targVals );
			#ifdef LOG_CONDS
			logNewExpansion( expansion );
			#endif
			expansionList.append( expansion );
		}
	}
}

void FsmAp::findEmbedExpansions( ExpansionList &expansionList, 
		StateAp *destState, Action *condAction, bool sense )
{
	StateCondList destList;
	RangePairIter<TransAp, StateCond> transCond( destState->outList.head,
			destState->stateCondList.head );
	for ( ; !transCond.end(); transCond++ ) {
		switch ( transCond.userState ) {
			case RangePairIter<TransAp, StateCond>::RangeInS1: {
				if ( transCond.s1Tel.lowKey <= keyOps->maxKey ) {
					assert( transCond.s1Tel.highKey <= keyOps->maxKey );

					/* Make a new state cond. */
					StateCond *newStateCond = new StateCond( transCond.s1Tel.lowKey,
							transCond.s1Tel.highKey );
					newStateCond->condSpace = addCondSpace( CondSet( condAction ) );
					destList.append( newStateCond );

					/* Create the expansion. */
					Expansion *expansion = new Expansion( transCond.s1Tel.lowKey,
							transCond.s1Tel.highKey );

					expansionTrans( expansion, transCond.s1Tel.trans );

					expansion->fromCondSpace = 0;
					expansion->fromVals = 0;
					expansion->toCondSpace = newStateCond->condSpace;
					expansion->toValsList.append( sense?1:0 );

					#ifdef LOG_CONDS
					logNewExpansion( expansion );
					#endif
					expansionList.append( expansion );
				}
				break;
			}
			case RangePairIter<TransAp, StateCond>::RangeInS2: {
				/* Enhance state cond and find the expansion. */
				StateCond *stateCond = transCond.s2Tel.trans;
				stateCond->lowKey = transCond.s2Tel.lowKey;
				stateCond->highKey = transCond.s2Tel.highKey;

				CondSet &destCS = stateCond->condSpace->condSet;
				long destLen = destCS.length();
				CondSpace *fromCondSpace = stateCond->condSpace;

				CondSet mergedCS = destCS;
				mergedCS.insert( condAction );
				CondSpace *toCondSpace = addCondSpace( mergedCS );
				stateCond->condSpace = toCondSpace;
				destList.append( stateCond );

				/* Loop all values in the dest space. */
				for ( long destVals = 0; destVals < (1 << destLen); destVals++ ) {
					long basicVals = 0;
					for ( CondSet::Iter csi = destCS; csi.lte(); csi++ ) {
						if ( destVals & (1 << csi.pos()) ) {
							Action **cim = mergedCS.find( *csi );
							long bitPos = (cim - mergedCS.data);
							basicVals |= 1 << bitPos;
						}
					}

					long targVals = basicVals;
					Action **cim = mergedCS.find( condAction );
					long bitPos = (cim - mergedCS.data);
					targVals |= (sense?1:0) << bitPos;
					
					LongVect expandToVals( targVals );
					findCondExpInTrans( expansionList, destState, 
						transCond.s2Tel.lowKey, transCond.s2Tel.highKey, 
						fromCondSpace, toCondSpace, destVals, expandToVals );
				}
				break;
			}


			case RangePairIter<TransAp, StateCond>::RangeOverlap:
			case RangePairIter<TransAp, StateCond>::BreakS1:
			case RangePairIter<TransAp, StateCond>::BreakS2:
				assert( false );
				break;
		}
	}

	destState->stateCondList.transfer( destList );
}


CondSpace *FsmAp::addCondSpace( const CondSet &condSet )
{
	CondSpace *condSpace = condData->condSpaceMap.find( condSet );
	if ( condSpace == 0 ) {
		/* Do we have enough keyspace left? */
		Size availableSpace = condData->lastCondKey.availableSpace();
		Size neededSpace = (1 << condSet.length() ) * keyOps->alphSize();
		if ( neededSpace > availableSpace )
			throw FsmConstructFail( FsmConstructFail::CondNoKeySpace );

		Key baseKey = condData->lastCondKey;
		baseKey.increment();
		condData->lastCondKey += (1 << condSet.length() ) * keyOps->alphSize();

		condSpace = new CondSpace( condSet );
		condSpace->baseKey = baseKey;
		condData->condSpaceMap.insert( condSpace );

		#ifdef LOG_CONDS
		cerr << "adding new condition space" << endl;
		cerr << "  condition set: ";
		logCondSpace( condSpace );
		cerr << endl;
		cerr << "  baseKey: " << baseKey.getVal() << endl;
		#endif
	}
	return condSpace;
}


void FsmAp::embedCondition( MergeData &md, StateAp *state, Action *condAction, bool sense )
{
#if 0
	ExpansionList expList;

	findEmbedExpansions( expList, state, condAction, sense );
	doExpand( md, state, expList );
	doRemove( md, state, expList );
	expList.empty();
#endif

	for ( TransList::Iter tr = state->outList; tr.lte(); tr++ ) {

		CondSet destCS;
		CondSet mergedCS;

		if ( tr->condSpace != 0 )
			destCS.insert( tr->condSpace->condSet );

		mergedCS.insert( destCS );
		mergedCS.insert( condAction );

		CondSpace *mergedCondSpace = addCondSpace( mergedCS );
		tr->condSpace = mergedCondSpace;

		CondSet newCS = mergedCS;
		for ( CondSet::Iter mcsi = destCS; mcsi.lte(); mcsi++ )
			newCS.remove( *mcsi );
		/*long newLen = */newCS.length();

		/* Need to transform condition element to the merged set. */
		for ( CondTransList::Iter cti = tr->ctList; cti.lte(); cti++ ) {
			long origVal = cti->lowKey.getVal();
			long newVal = 0;

			/* Iterate the bit positions in the from set. */
			for ( CondSet::Iter csi = destCS; csi.lte(); csi++ ) {
				/* If set, find it in the merged set and flip the bit to 1. */
				if ( origVal & (1 << csi.pos()) ) {
					/* The condition is set. Find the bit position in the merged
					 * set. */
					Action **cim = mergedCS.find( *csi );
					long bitPos = (cim - mergedCS.data);
					newVal |= 1 << bitPos;
				}
			}

			if ( origVal != newVal ) {
				cout << "orig: " << origVal << " new: " << newVal << endl;
				cti->lowKey = cti->highKey = newVal;
			}

			if ( sense ) {
				Action **cim = mergedCS.find( condAction );
				int pos = cim - mergedCS.data;
				cti->lowKey = cti->highKey = cti->lowKey.getVal() | (1 << pos);
			}
		}
	}
}

#if 0
	/* Need to double up the whole transition list for each condition test in
	 * merged that is not in from. The one we add has the bit in question set.
	 * */
	for ( CondSet::Iter csi = mergedCS; csi.lte(); csi++ ) {
		Action **cim = fromCS.find( *csi );
		if ( cim == 0 ) {
			CondTransList newItems;
			cout << "doubling up on condition" << endl;
			for ( CondTransList::Iter cti = trans->ctList; cti.lte(); cti++ ) {
				CondAp *cond = dupCondTrans( fromState, trans, cti  );

				cond->lowKey = cond->highKey = cti->lowKey.getVal() | (1 << csi.pos());

				newItems.append( cond );
			}

			trans->ctList.append( newItems );
		}
	}
#if 0
#endif

#if 0
	cout << "newCS.length() = " << newCS.length() << endl;

	if ( newCS.length() > 0 ) {
		#ifdef LOG_CONDS
		cerr << "there are " << newCS.length() << " item(s) that are "
					"only in the mergedCS" << endl;
		#endif

		long fromLen = fromCS.length();
	
		/* Loop all values in the dest space. */
		for ( long fromBits = 0; fromBits < (1 << fromLen); fromBits++ ) {
			long basicVals = 0;
			for ( CondSet::Iter csi = fromCS; csi.lte(); csi++ ) {
				if ( fromBits & (1 << csi.pos()) ) {
					Action **cim = mergedCS.find( *csi );
					long bitPos = (cim - mergedCS.data);
					basicVals |= 1 << bitPos;
				}
			}

			cerr << "basicVals: " << basicVals << endl;
	
			/* Loop all new values. */
			LongVect expandToVals;
			for ( long newVals = 0; newVals < (1 << newLen); newVals++ ) {
				long targVals = basicVals;
				for ( CondSet::Iter csi = newCS; csi.lte(); csi++ ) {
					if ( newVals & (1 << csi.pos()) ) {
						Action **cim = mergedCS.find( *csi );
						long bitPos = (cim - mergedCS.data);
						targVals |= 1 << bitPos;
					}
				}
				cerr << "targVals: " << targVals << endl;
				expandToVals.append( targVals );
			}
	
//			findCondExpInTrans( expansionList, destState, 
//					condCond.s1Tel.lowKey, condCond.s1Tel.highKey, 
//					fromCondSpace, toCondSpace, destVals, expandToVals );
		}
	}
#endif
#endif

void FsmAp::embedCondition( StateAp *state, Action *condAction, bool sense )
{
	MergeData md;
	ExpansionList expList;

	/* Turn on misfit accounting to possibly catch the old start state. */
	setMisfitAccounting( true );

	/* Worker. */
	embedCondition( md, state, condAction, sense );

	/* Fill in any states that were newed up as combinations of others. */
	fillInStates( md );

	/* Remove the misfits and turn off misfit accounting. */
	removeMisfits();
	setMisfitAccounting( false );
}
