/*
 *  Copyright 2005-2007 Adrian Thurston <thurston@complang.org>
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

#include "gendata.h"
#include "ragel.h"
#include "parsedata.h"
#include "fsmgraph.h"
#include "inputdata.h"
#include "version.h"

#include <string.h>
#include <iostream>

string itoa( int i )
{
	char buf[16];
	sprintf( buf, "%i", i );
	return buf;
}

void openHostBlock( char opener, InputData *id, ostream &out, const char *fileName, int line )
{
	out << "host( \"";
	for ( const char *pc = fileName; *pc != 0; pc++ ) {
		if ( *pc == '\\' )
			out << "\\\\";
		else
			out << *pc;
	}
	out << "\", " << line << " ) " << opener << "{";
}

GenBase::GenBase( std::string fsmName, int machineId, ParseData *pd, FsmAp *fsm )
:
	fsmName(fsmName),
	machineId(machineId),
	pd(pd),
	fsm(fsm),
	keyOps(pd->fsmCtx->keyOps),
	nextActionTableId(0)
{
}

void GenBase::appendTrans( TransListVect &outList, Key lowKey, 
		Key highKey, TransAp *trans )
{
	if ( trans->plain() ) {
		if ( trans->tdap()->toState != 0 || trans->tdap()->actionTable.length() > 0 )
			outList.append( TransEl( lowKey, highKey, trans ) );
	}
	else {
		for ( CondList::Iter cond = trans->tcap()->condList; cond.lte(); cond++ ) {
			if ( cond->toState != 0 || cond->actionTable.length() > 0 ) {
				outList.append( TransEl( lowKey, highKey, trans ) );
				break;
			}
		}
	}
}

void GenBase::reduceActionTables()
{
	/* Reduce the actions tables to a set. */
	for ( StateList::Iter st = fsm->stateList; st.lte(); st++ ) {
		RedActionTable *actionTable = 0;

		/* Reduce To State Actions. */
		if ( st->toStateActionTable.length() > 0 ) {
			if ( actionTableMap.insert( st->toStateActionTable, &actionTable ) )
				actionTable->id = nextActionTableId++;
		}

		/* Reduce From State Actions. */
		if ( st->fromStateActionTable.length() > 0 ) {
			if ( actionTableMap.insert( st->fromStateActionTable, &actionTable ) )
				actionTable->id = nextActionTableId++;
		}

		/* Reduce EOF actions. */
		if ( st->eofActionTable.length() > 0 ) {
			if ( actionTableMap.insert( st->eofActionTable, &actionTable ) )
				actionTable->id = nextActionTableId++;
		}

		/* Loop the transitions and reduce their actions. */
		for ( TransList::Iter trans = st->outList; trans.lte(); trans++ ) {
			if ( trans->plain() ) {
				if ( trans->tdap()->actionTable.length() > 0 ) {
					if ( actionTableMap.insert( trans->tdap()->actionTable, &actionTable ) )
						actionTable->id = nextActionTableId++;
				}
			}
			else {
				for ( CondList::Iter cond = trans->tcap()->condList; cond.lte(); cond++ ) {
					if ( cond->actionTable.length() > 0 ) {
						if ( actionTableMap.insert( cond->actionTable, &actionTable ) )
							actionTable->id = nextActionTableId++;
					}
				}
			}
		}

		if ( st->nfaOut != 0 ) {
			for ( NfaTransList::Iter n = *st->nfaOut; n.lte(); n++ ) {
				if ( actionTableMap.insert( n->pushTable, &actionTable ) )
					actionTable->id = nextActionTableId++;

				if ( actionTableMap.insert( n->restoreTable, &actionTable ) )
					actionTable->id = nextActionTableId++;

				if ( actionTableMap.insert( n->popAction, &actionTable ) )
					actionTable->id = nextActionTableId++;

				if ( actionTableMap.insert( n->popTest, &actionTable ) )
					actionTable->id = nextActionTableId++;
			}
		}
	}
}


/* Total error count. */
/* int gblErrorCount = 0; */

CodeGenData::CodeGenData( const CodeGenArgs &args )
:
	GenBase(args.fsmName, args.machineId, args.pd, args.fsm),

	sourceFileName(args.sourceFileName),
	fsmName(args.fsmName), 
	out(args.out),
	redFsm(0), 
	allActions(0),
	allActionTables(0),
	allConditions(0),
	allCondSpaces(0),
	allStates(0),
	nameIndex(0),
	startState(-1),
	errState(-1),
	getKeyExpr(0),
	accessExpr(0),
	prePushExpr(0),
	postPopExpr(0),
	nfaPrePushExpr(0),
	nfaPostPopExpr(0),
	pExpr(0),
	peExpr(0),
	eofExpr(0),
	csExpr(0),
	topExpr(0),
	stackExpr(0),
	actExpr(0),
	tokstartExpr(0),
	tokendExpr(0),
	dataExpr(0),
	hasLongestMatch(false),
	noEnd(false),
	noPrefix(false),
	noFinal(false),
	noError(false),
	noCS(false)
{
}

void CodeGenData::makeText( GenInlineList *outList, InlineItem *item )
{
	GenInlineItem *inlineItem = new GenInlineItem( InputLoc(), GenInlineItem::Text );
	inlineItem->data = item->data;

	outList->append( inlineItem );
}

void CodeGenData::makeTargetItem( GenInlineList *outList, NameInst *nameTarg, 
		GenInlineItem::Type type )
{
	long targetState;
	if ( pd->generatingSectionSubset )
		targetState = -1;
	else {
		EntryMapEl *targ = fsm->entryPoints.find( nameTarg->id );
		targetState = targ->value->alg.stateNum;
	}

	/* Make the item. */
	GenInlineItem *inlineItem = new GenInlineItem( InputLoc(), type );
	inlineItem->targId = targetState;
	outList->append( inlineItem );
}


void CodeGenData::makeSubList( GenInlineList *outList, const InputLoc &loc,
		InlineList *inlineList, GenInlineItem::Type type )
{
	/* Fill the sub list. */
	GenInlineList *subList = new GenInlineList;
	makeGenInlineList( subList, inlineList );

	/* Make the item. */
	GenInlineItem *inlineItem = new GenInlineItem( loc, type );
	inlineItem->children = subList;
	outList->append( inlineItem );
}

/* Make a sublist item with a given type. */
void CodeGenData::makeSubList( GenInlineList *outList, 
		InlineList *inlineList, GenInlineItem::Type type )
{
	makeSubList( outList, InputLoc(), inlineList, type );
}

void CodeGenData::makeLmOnLast( GenInlineList *outList, InlineItem *item )
{
	makeSetTokend( outList, 1 );

	if ( item->longestMatchPart->action != 0 ) {
		Action *action = item->longestMatchPart->action;
		makeSubList( outList, action->loc, action->inlineList, 
				GenInlineItem::HostStmt );
	}
}

void CodeGenData::makeLmOnNext( GenInlineList *outList, InlineItem *item )
{
	makeSetTokend( outList, 0 );
	outList->append( new GenInlineItem( InputLoc(), GenInlineItem::LmHold ) );

	if ( item->longestMatchPart->action != 0 ) {
		Action *action = item->longestMatchPart->action;
		makeSubList( outList, action->loc, action->inlineList,
			GenInlineItem::HostStmt );
	}
}

void CodeGenData::makeExecGetTokend( GenInlineList *outList )
{
	/* Make the Exec item. */
	GenInlineItem *execItem = new GenInlineItem( InputLoc(), GenInlineItem::LmExec );
	execItem->children = new GenInlineList;

	/* Make the GetTokEnd */
	GenInlineItem *getTokend = new GenInlineItem( InputLoc(), GenInlineItem::LmGetTokEnd );
	execItem->children->append( getTokend );

	outList->append( execItem );
}

void CodeGenData::makeLmOnLagBehind( GenInlineList *outList, InlineItem *item )
{
	/* Jump to the tokend. */
	makeExecGetTokend( outList );

	if ( item->longestMatchPart->action != 0 ) {
		Action *action = item->longestMatchPart->action;
		makeSubList( outList, action->loc, action->inlineList,
			GenInlineItem::HostStmt );
	}
}

void CodeGenData::makeLmSwitch( GenInlineList *outList, InlineItem *item )
{
	GenInlineItem *lmSwitch = new GenInlineItem( InputLoc(), GenInlineItem::LmSwitch );
	GenInlineList *lmList = lmSwitch->children = new GenInlineList;
	LongestMatch *longestMatch = item->longestMatch;

	/* We can't put the <exec> here because we may need to handle the error
	 * case and in that case p should not be changed. Instead use a default
	 * label in the switch to adjust p when user actions are not set. An id of
	 * -1 indicates the default. */

	if ( longestMatch->lmSwitchHandlesError ) {
		/* If the switch handles error then we should have also forced the
		 * error state. */
		assert( fsm->errState != 0 );

		GenInlineItem *errCase = new GenInlineItem( InputLoc(), GenInlineItem::HostStmt );
		errCase->lmId = 0;
		errCase->children = new GenInlineList;

		GenInlineItem *host = new GenInlineItem( item->loc, GenInlineItem::HostStmt );
		host->children = new GenInlineList;
		errCase->children->append( host );

		/* Make the item. This should probably be an LM goto, would eliminate
		 * need for wrapping in host statement. .*/
		GenInlineItem *gotoItem = new GenInlineItem( InputLoc(), GenInlineItem::Goto );
		gotoItem->targId = fsm->errState->alg.stateNum;
		host->children->append( gotoItem );

		lmList->append( errCase );
	}
	
	bool needDefault = false;
	for ( LmPartList::Iter lmi = *longestMatch->longestMatchList; lmi.lte(); lmi++ ) {
		if ( lmi->inLmSelect ) {
			if ( lmi->action == 0 )
				needDefault = true;
			else {
				/* Open the action. Write it with the context that sets up _p 
				 * when doing control flow changes from inside the machine. */
				GenInlineItem *lmCase = new GenInlineItem( InputLoc(), GenInlineItem::LmCase );
				lmCase->lmId = lmi->longestMatchId;
				lmCase->children = new GenInlineList;

				makeExecGetTokend( lmCase->children );

				GenInlineItem *subHost = new GenInlineItem( lmi->action->loc,
						GenInlineItem::HostStmt );
				subHost->children = new GenInlineList;
				makeGenInlineList( subHost->children, lmi->action->inlineList );
				lmCase->children->append( subHost );

				lmList->append( lmCase );
			}
		}
	}

	if ( needDefault ) {
		GenInlineItem *defCase = new GenInlineItem( item->loc, GenInlineItem::HostStmt );
		defCase->lmId = -1;
		defCase->children = new GenInlineList;

		makeExecGetTokend( defCase->children );

		lmList->append( defCase );
	}

	outList->append( lmSwitch );
}

void CodeGenData::makeSetTokend( GenInlineList *outList, long offset )
{
	GenInlineItem *inlineItem = new GenInlineItem( InputLoc(), GenInlineItem::LmSetTokEnd );
	inlineItem->offset = offset;
	outList->append( inlineItem );
}

void CodeGenData::makeSetAct( GenInlineList *outList, long lmId )
{
	GenInlineItem *inlineItem = new GenInlineItem( InputLoc(), GenInlineItem::LmSetActId );
	inlineItem->lmId = lmId;
	outList->append( inlineItem );
}

void CodeGenData::makeGenInlineList( GenInlineList *outList, InlineList *inList )
{
	for ( InlineList::Iter item = *inList; item.lte(); item++ ) {
		switch ( item->type ) {
		case InlineItem::Text:
			makeText( outList, item );
			break;
		case InlineItem::Goto:
			makeTargetItem( outList, item->nameTarg, GenInlineItem::Goto );
			break;
		case InlineItem::GotoExpr:
			makeSubList( outList, item->children, GenInlineItem::GotoExpr );
			break;
		case InlineItem::Call:
			makeTargetItem( outList, item->nameTarg, GenInlineItem::Call );
			break;
		case InlineItem::CallExpr:
			makeSubList( outList, item->children, GenInlineItem::CallExpr );
			break;
		case InlineItem::Ncall:
			makeTargetItem( outList, item->nameTarg, GenInlineItem::Ncall );
			break;
		case InlineItem::NcallExpr:
			makeSubList( outList, item->children, GenInlineItem::NcallExpr );
			break;
		case InlineItem::Next:
			makeTargetItem( outList, item->nameTarg, GenInlineItem::Next );
			break;
		case InlineItem::NextExpr:
			makeSubList( outList, item->children, GenInlineItem::NextExpr );
			break;
		case InlineItem::Break:
			outList->append( new GenInlineItem( InputLoc(), GenInlineItem::Break ) );
			break;
		case InlineItem::Nbreak:
			outList->append( new GenInlineItem( InputLoc(), GenInlineItem::Nbreak ) );
			break;
		case InlineItem::Ret: 
			outList->append( new GenInlineItem( InputLoc(), GenInlineItem::Ret ) );
			break;
		case InlineItem::Nret: 
			outList->append( new GenInlineItem( InputLoc(), GenInlineItem::Nret ) );
			break;
		case InlineItem::PChar:
			outList->append( new GenInlineItem( InputLoc(), GenInlineItem::PChar ) );
			break;
		case InlineItem::Char: 
			outList->append( new GenInlineItem( InputLoc(), GenInlineItem::Char ) );
			break;
		case InlineItem::Curs: 
			outList->append( new GenInlineItem( InputLoc(), GenInlineItem::Curs ) );
			break;
		case InlineItem::Targs: 
			outList->append( new GenInlineItem( InputLoc(), GenInlineItem::Targs ) );
			break;
		case InlineItem::Entry:
			makeTargetItem( outList, item->nameTarg, GenInlineItem::Entry );
			break;

		case InlineItem::Hold:
			outList->append( new GenInlineItem( InputLoc(), GenInlineItem::Hold ) );
			break;
		case InlineItem::Exec:
			makeSubList( outList, item->children, GenInlineItem::Exec );
			break;

		case InlineItem::LmSetActId:
			makeSetAct( outList, item->longestMatchPart->longestMatchId );
			break;
		case InlineItem::LmSetTokEnd:
			makeSetTokend( outList, 1 );
			break;

		case InlineItem::LmOnLast:
			makeLmOnLast( outList, item );
			break;
		case InlineItem::LmOnNext:
			makeLmOnNext( outList, item );
			break;
		case InlineItem::LmOnLagBehind:
			makeLmOnLagBehind( outList, item );
			break;
		case InlineItem::LmSwitch: 
			makeLmSwitch( outList, item );
			break;

		case InlineItem::LmInitAct:
			outList->append( new GenInlineItem( InputLoc(), GenInlineItem::LmInitAct ) );
			break;
		case InlineItem::LmInitTokStart:
			outList->append( new GenInlineItem( InputLoc(), GenInlineItem::LmInitTokStart ) );
			break;
		case InlineItem::LmSetTokStart:
			outList->append( new GenInlineItem( InputLoc(), GenInlineItem::LmSetTokStart ) );
			hasLongestMatch = true;
			break;
		case InlineItem::Stmt:
			makeSubList( outList, item->children, GenInlineItem::GenStmt );
			break;
		case InlineItem::Subst: {
			/* Find the subst action. */
			Action *subst = curInlineAction->argList->data[item->substPos];
			makeGenInlineList( outList, subst->inlineList );
			break;
		}
		case InlineItem::NfaWrapAction: {
			GenAction *wrap = allActions + item->wrappedAction->actionId;
			GenInlineItem *gii = new GenInlineItem( InputLoc(),
					GenInlineItem::NfaWrapAction );
			gii->wrappedAction = wrap;
			outList->append( gii );
			break;
		}
		case InlineItem::NfaWrapConds: {
			GenCondSpace *condSpace = allCondSpaces + item->condSpace->condSpaceId;

			GenInlineItem *gii = new GenInlineItem( InputLoc(),
					GenInlineItem::NfaWrapConds );
			gii->condSpace = condSpace;
			gii->condKeySet = item->condKeySet;
			outList->append( gii );
			break;
		}}
	}
}

void CodeGenData::makeExports()
{
	for ( ExportList::Iter exp = pd->exportList; exp.lte(); exp++ )
		exportList.append( new Export( exp->name, exp->key ) );
}

void CodeGenData::makeAction( Action *action )
{
	GenInlineList *genList = new GenInlineList;

	curInlineAction = action;
	makeGenInlineList( genList, action->inlineList );
	curInlineAction = 0;

	newAction( curAction++, action->name, action->loc, genList );
}


void CodeGenData::makeActionList()
{
	/* Determine which actions to write. */
	int nextActionId = 0;
	for ( ActionList::Iter act = pd->actionList; act.lte(); act++ ) {
		if ( act->numRefs() > 0 || act->numCondRefs > 0 )
			act->actionId = nextActionId++;
	}

	/* Write the list. */
	initActionList( nextActionId );
	curAction = 0;

	for ( ActionList::Iter act = pd->actionList; act.lte(); act++ ) {
		if ( act->actionId >= 0 )
			makeAction( act );
	}
}

void CodeGenData::makeActionTableList()
{
	/* Must first order the action tables based on their id. */
	int numTables = nextActionTableId;
	RedActionTable **tables = new RedActionTable*[numTables];
	for ( ActionTableMap::Iter at = actionTableMap; at.lte(); at++ )
		tables[at->id] = at;

	initActionTableList( numTables );
	curActionTable = 0;

	for ( int t = 0; t < numTables; t++ ) {
		long length = tables[t]->key.length();

		/* Collect the action table. */
		RedAction *redAct = allActionTables + curActionTable;
		redAct->actListId = curActionTable;
		redAct->key.setAsNew( length );

		for ( ActionTable::Iter atel = tables[t]->key; atel.lte(); atel++ ) {
			redAct->key[atel.pos()].key = 0;
			redAct->key[atel.pos()].value = allActions + 
					atel->value->actionId;
		}

		/* Insert into the action table map. */
		redFsm->actionMap.insert( redAct );

		curActionTable += 1;
	}

	delete[] tables;
}

void CodeGenData::makeConditions()
{
	if ( pd->fsmCtx->condData->condSpaceMap.length() > 0 ) {
		/* Allocate condition space ids. */
		long nextCondSpaceId = 0;
		for ( CondSpaceMap::Iter cs = pd->fsmCtx->condData->condSpaceMap; cs.lte(); cs++ )
			cs->condSpaceId = nextCondSpaceId++;

		/* Allocate the array of conditions and put them on the list. */
		long length = pd->fsmCtx->condData->condSpaceMap.length();
		allCondSpaces = new GenCondSpace[length];
		for ( long c = 0; c < length; c++ )
			condSpaceList.append( &allCondSpaces[c] );

		long curCondSpace = 0;
		for ( CondSpaceMap::Iter cs = pd->fsmCtx->condData->condSpaceMap; cs.lte(); cs++ ) {
			/* Transfer the id. */
			allCondSpaces[curCondSpace].condSpaceId = cs->condSpaceId;

			curCondSpace += 1;
		}
	}

	makeActionList();
	makeActionTableList();

	if ( pd->fsmCtx->condData->condSpaceMap.length() > 0 ) {
		long curCondSpace = 0;
		for ( CondSpaceMap::Iter cs = pd->fsmCtx->condData->condSpaceMap; cs.lte(); cs++ ) {
			for ( CondSet::Iter csi = cs->condSet; csi.lte(); csi++ )
				condSpaceItem( curCondSpace, (*csi)->actionId );
			curCondSpace += 1;
		}
	}
}

bool CodeGenData::makeNameInst( std::string &res, NameInst *nameInst )
{
	bool written = false;
	if ( nameInst->parent != 0 )
		written = makeNameInst( res, nameInst->parent );
	
	if ( !nameInst->name.empty() ) {
		if ( written )
			res += '_';
		res += nameInst->name;
		written = true;
	}

	return written;
}

void CodeGenData::makeEntryPoints()
{
	/* List of entry points other than start state. */
	if ( fsm->entryPoints.length() > 0 || pd->lmRequiresErrorState ) {
		if ( pd->lmRequiresErrorState )
			setForcedErrorState();

		for ( EntryMap::Iter en = fsm->entryPoints; en.lte(); en++ ) {
			/* Get the name instantiation from nameIndex. */
			NameInst *nameInst = pd->nameIndex[en->key];
			std::string name;
			makeNameInst( name, nameInst );
			StateAp *state = en->value;
			addEntryPoint( strdup(name.c_str()), state->alg.stateNum );
		}
	}
}

void CodeGenData::makeStateActions( StateAp *state )
{
	RedActionTable *toStateActions = 0;
	if ( state->toStateActionTable.length() > 0 )
		toStateActions = actionTableMap.find( state->toStateActionTable );

	RedActionTable *fromStateActions = 0;
	if ( state->fromStateActionTable.length() > 0 )
		fromStateActions = actionTableMap.find( state->fromStateActionTable );

	/* EOF actions go out here only if the state has no eof target. If it has
	 * an eof target then an eof transition will be used instead. */
	RedActionTable *eofActions = 0;
	if ( state->eofTarget == 0 && state->eofActionTable.length() > 0 )
		eofActions = actionTableMap.find( state->eofActionTable );
	
	if ( toStateActions != 0 || fromStateActions != 0 || eofActions != 0 ) {
		long to = -1;
		if ( toStateActions != 0 )
			to = toStateActions->id;

		long from = -1;
		if ( fromStateActions != 0 )
			from = fromStateActions->id;

		long eof = -1;
		if ( eofActions != 0 )
			eof = eofActions->id;

		setStateActions( curState, to, from, eof );
	}
}

void CodeGenData::makeEofTrans( StateAp *state )
{
	RedActionTable *eofActions = 0;
	if ( state->eofActionTable.length() > 0 )
		eofActions = actionTableMap.find( state->eofActionTable );
	
	/* The EOF trans is used when there is an eof target, otherwise the eof
	 * action goes into state actions. */
	if ( state->eofTarget != 0 ) {
		long targ = state->eofTarget->alg.stateNum;
		long action = -1;
		if ( eofActions != 0 )
			action = eofActions->id;

		setEofTrans( curState, targ, action );
	}
}

void CodeGenData::makeTrans( Key lowKey, Key highKey, TransAp *trans )
{
	RedCondEl *outConds;
	int numConds;

	assert( ( allStates + curState ) != redFsm->errState );

	if ( trans->plain() ) {
		long targ = -1;
		long action = -1;

		/* First reduce the action. */
		RedActionTable *actionTable = 0;
		if ( trans->tdap()->actionTable.length() > 0 )
			actionTable = actionTableMap.find( trans->tdap()->actionTable );

		if ( trans->tdap()->toState != 0 )
			targ = trans->tdap()->toState->alg.stateNum;

		if ( actionTable != 0 )
			action = actionTable->id;

		/* Make the new transitions. */
		RedStateAp *targState = targ >= 0 ? (allStates + targ) : redFsm->getErrorState();
		RedAction *at = action >= 0 ? (allActionTables + action) : 0;

		RedTransAp *trans = redFsm->allocateTrans( targState, at );
		newTrans( allStates + curState, lowKey, highKey, trans );
	}
	else {
		numConds = trans->tcap()->condList.length();
		outConds = new RedCondEl[numConds];
		int pos = 0;
		for ( CondList::Iter cti = trans->tcap()->condList; cti.lte(); cti++, pos++ ) {
			long targ = -1;
			long action = -1;

			/* First reduce the action. */
			RedActionTable *actionTable = 0;
			if ( cti->actionTable.length() > 0 )
				actionTable = actionTableMap.find( cti->actionTable );

			if ( cti->toState != 0 )
				targ = cti->toState->alg.stateNum;

			if ( actionTable != 0 )
				action = actionTable->id;

			/* Make the new transitions. */
			RedStateAp *targState = targ >= 0 ? (allStates + targ) : redFsm->getErrorState();
			RedAction *at = action >= 0 ? (allActionTables + action) : 0;
			RedCondAp *cond = redFsm->allocateCond( targState, at );

			outConds[pos].key = cti->key;
			outConds[pos].value = cond;
		}

		GenCondSpace *condSpace = allCondSpaces + trans->condSpace->condSpaceId;

		/* If the cond list is not full then we need an error cond. */
		RedCondAp *errCond = 0;
		if ( numConds < ( 1 << condSpace->condSet.length() ) )
			errCond = redFsm->getErrorCond();
		
		RedTransAp *trans = redFsm->allocateTrans(
				condSpace, outConds, numConds, errCond  );

		trans->v.outConds = outConds;
		trans->v.numConds = numConds;
		trans->v.errCond = errCond;

		newTrans( allStates + curState, lowKey, highKey, trans );
	}
}

void CodeGenData::makeTransList( StateAp *state )
{
	TransListVect outList;

	/* If there is only are no ranges the task is simple. */
	if ( state->outList.length() > 0 ) {
		/* Loop each source range. */
		for ( TransList::Iter trans = state->outList; trans.lte(); trans++ ) {
			/* Reduce the transition. If it reduced to anything then add it. */
			appendTrans( outList, trans->lowKey, trans->highKey, trans );
		}
	}

	initTransList( curState, outList.length() );

	for ( TransListVect::Iter tvi = outList; tvi.lte(); tvi++ )
		makeTrans( tvi->lowKey, tvi->highKey, tvi->value );

	finishTransList( curState );
}

void CodeGenData::makeStateList()
{
	/* Write the list of states. */
	long length = fsm->stateList.length();
	initStateList( length );
	curState = 0;
	for ( StateList::Iter st = fsm->stateList; st.lte(); st++ ) {
		makeStateActions( st );
		makeEofTrans( st );
		makeTransList( st );

		long id = st->alg.stateNum;
		setId( curState, id );

		if ( st->isFinState() )
			setFinal( curState );

		if ( st->nfaOut != 0 ) {
			RedStateAp *from = allStates + curState;
			from->nfaTargs = new RedNfaTargs;
			for ( NfaTransList::Iter targ = *st->nfaOut; targ.lte(); targ++ ) {
				RedStateAp *rtarg = allStates + targ->toState->alg.stateNum;

				RedAction *pushRa = 0;
				RedAction *popTestRa = 0;

				if ( targ->pushTable.length() > 0 ) {
					RedActionTable *pushActions =
							actionTableMap.find( targ->pushTable );
					pushRa = allActionTables + pushActions->id;
				}

				if ( targ->popTest.length() > 0 ) {
					RedActionTable *popActions =
							actionTableMap.find( targ->popTest );
					popTestRa = allActionTables + popActions->id;
				}


				from->nfaTargs->append( RedNfaTarg( rtarg, pushRa,
						popTestRa, targ->order ) );

				MergeSort<RedNfaTarg, RedNfaTargCmp> sort;
				sort.sort( from->nfaTargs->data, from->nfaTargs->length() );
			}
		}

		curState += 1;
	}
}

void CodeGenData::makeMachine()
{
	createMachine();

	/* Action tables. */
	reduceActionTables();

	makeConditions();

	/* Start State. */
	setStartState( fsm->startState->alg.stateNum );

	/* Error state. */
	if ( fsm->errState != 0 )
		setErrorState( fsm->errState->alg.stateNum );

	makeEntryPoints();
	makeStateList();

	resolveTargetStates();
}

void CodeGenData::make( const HostLang *hostLang )
{
	/* Alphabet type. */
	setAlphType( hostLang, pd->fsmCtx->keyOps->alphType->internalName );
	
	/* Getkey expression. */
	if ( pd->getKeyExpr != 0 ) {
		getKeyExpr = new GenInlineList;
		makeGenInlineList( getKeyExpr, pd->getKeyExpr );
	}

	/* Access expression. */
	if ( pd->accessExpr != 0 ) {
		accessExpr = new GenInlineList;
		makeGenInlineList( accessExpr, pd->accessExpr );
	}

	/* PrePush expression. */
	if ( pd->prePushExpr != 0 ) {
		GenInlineList *il = new GenInlineList;
		makeGenInlineList( il, pd->prePushExpr->inlineList );
		prePushExpr = new GenInlineExpr( pd->prePushExpr->loc, il );
	}

	/* PostPop expression. */
	if ( pd->postPopExpr != 0 ) {
		GenInlineList *il = new GenInlineList;
		makeGenInlineList( il, pd->postPopExpr->inlineList );
		postPopExpr = new GenInlineExpr( pd->postPopExpr->loc, il );
	}

	/* PrePush expression. */
	if ( pd->nfaPrePushExpr != 0 ) {
		GenInlineList *il = new GenInlineList;
		makeGenInlineList( il, pd->nfaPrePushExpr->inlineList );
		nfaPrePushExpr = new GenInlineExpr( pd->nfaPrePushExpr->loc, il );
	}

	/* PostPop expression. */
	if ( pd->nfaPostPopExpr != 0 ) {
		GenInlineList *il = new GenInlineList;
		makeGenInlineList( il, pd->nfaPostPopExpr->inlineList );
		nfaPostPopExpr = new GenInlineExpr( pd->nfaPostPopExpr->loc, il );
	}


	/*
	 * Variable expressions.
	 */

	if ( pd->pExpr != 0 ) {
		pExpr = new GenInlineList;
		makeGenInlineList( pExpr, pd->pExpr );
	}
	
	if ( pd->peExpr != 0 ) {
		peExpr = new GenInlineList;
		makeGenInlineList( peExpr, pd->peExpr );
	}

	if ( pd->eofExpr != 0 ) {
		eofExpr = new GenInlineList;
		makeGenInlineList( eofExpr, pd->eofExpr );
	}
	
	if ( pd->csExpr != 0 ) {
		csExpr = new GenInlineList;
		makeGenInlineList( csExpr, pd->csExpr );
	}
	
	if ( pd->topExpr != 0 ) {
		topExpr = new GenInlineList;
		makeGenInlineList( topExpr, pd->topExpr );
	}
	
	if ( pd->stackExpr != 0 ) {
		stackExpr = new GenInlineList;
		makeGenInlineList( stackExpr, pd->stackExpr );
	}
	
	if ( pd->actExpr != 0 ) {
		actExpr = new GenInlineList;
		makeGenInlineList( actExpr, pd->actExpr );
	}
	
	if ( pd->tokstartExpr != 0 ) {
		tokstartExpr = new GenInlineList;
		makeGenInlineList( tokstartExpr, pd->tokstartExpr );
	}
	
	if ( pd->tokendExpr != 0 ) {
		tokendExpr = new GenInlineList;
		makeGenInlineList( tokendExpr, pd->tokendExpr );
	}
	
	if ( pd->dataExpr != 0 ) {
		dataExpr = new GenInlineList;
		makeGenInlineList( dataExpr, pd->dataExpr );
	}
	
	makeExports();
	makeMachine();

	/* Do this before distributing transitions out to singles and defaults
	 * makes life easier. */
	redFsm->maxKey = findMaxKey();

	redFsm->assignActionLocs();

	/* Find the first final state (The final state with the lowest id). */
	redFsm->findFirstFinState();

	/* Code generation anlysis step. */
	genAnalysis();
}

void CodeGenData::createMachine()
{
	redFsm = new RedFsmAp( pd->fsmCtx );
}

void CodeGenData::initActionList( unsigned long length )
{ 
	allActions = new GenAction[length];
	for ( unsigned long a = 0; a < length; a++ )
		actionList.append( allActions+a );
}

void CodeGenData::newAction( int anum, std::string name,
		const InputLoc &loc, GenInlineList *inlineList )
{
	allActions[anum].actionId = anum;
	allActions[anum].name = name;
	allActions[anum].loc = loc;
	allActions[anum].inlineList = inlineList;
}

void CodeGenData::initActionTableList( unsigned long length )
{ 
	allActionTables = new RedAction[length];
}

void CodeGenData::initStateList( unsigned long length )
{
	redFsm->allStates = allStates = new RedStateAp[length];
	for ( unsigned long s = 0; s < length; s++ )
		redFsm->stateList.append( allStates+s );

	/* We get the start state as an offset, set the pointer now. */
	if ( startState >= 0 )
		redFsm->startState = allStates + startState;
	if ( errState >= 0 )
		redFsm->errState = allStates + errState;
	for ( EntryIdVect::Iter en = entryPointIds; en.lte(); en++ )
		redFsm->entryPoints.insert( allStates + *en );

	/* The nextStateId is no longer used to assign state ids (they come in set
	 * from the frontend now), however generation code still depends on it.
	 * Should eventually remove this variable. */
	redFsm->nextStateId = redFsm->stateList.length();
}

void CodeGenData::setStartState( unsigned long _startState )
{
	startState = _startState;
}

void CodeGenData::setErrorState( unsigned long _errState )
{
	errState = _errState;
}

void CodeGenData::addEntryPoint( char *name, unsigned long entryState )
{
	entryPointIds.append( entryState );
	entryPointNames.append( name );
}

void CodeGenData::initTransList( int snum, unsigned long length )
{
	/* Could preallocate the out range to save time growing it. For now do
	 * nothing. */
}

void CodeGenData::newTrans( RedStateAp *state, Key lowKey, Key highKey, RedTransAp *trans )
{
	/* Get the current state and range. */
	RedTransList &destRange = state->outRange;

	/* Reduced machines are complete. We need to fill any gaps with the error
	 * transitions. */
	if ( destRange.length() == 0 ) {
		/* Range is currently empty. */
		if ( keyOps->lt( keyOps->minKey, lowKey ) ) {
			/* The first range doesn't start at the low end. */
			Key fillHighKey = lowKey;
			keyOps->decrement( fillHighKey );

			/* Create the filler with the state's error transition. */
			RedTransEl newTel( pd->fsmCtx->keyOps->minKey, fillHighKey,
					redFsm->getErrorTrans() );
			destRange.append( newTel );
		}
	}
	else {
		/* The range list is not empty, get the the last range. */
		RedTransEl *last = &destRange[destRange.length()-1];
		Key nextKey = last->highKey;
		keyOps->increment( nextKey );
		if ( keyOps->lt( nextKey, lowKey ) ) {
			/* There is a gap to fill. Make the high key. */
			Key fillHighKey = lowKey;
			keyOps->decrement( fillHighKey );

			/* Create the filler with the state's error transtion. */
			RedTransEl newTel( nextKey, fillHighKey, redFsm->getErrorTrans() );
			destRange.append( newTel );
		}
	}

	/* Filler taken care of. Append the range. */
	destRange.append( RedTransEl( lowKey, highKey, trans ) );
}

void CodeGenData::finishTransList( int snum )
{
	/* Get the current state and range. */
	RedStateAp *curState = allStates + snum;
	RedTransList &destRange = curState->outRange;

	if ( curState == redFsm->errState )
		return;

	/* We may need filler on the end. */
	/* Check if there are any ranges already. */
	if ( destRange.length() == 0 ) {
		/* Fill with the whole alphabet. */
		/* Add the range on the lower and upper bound. */
		RedTransEl newTel( pd->fsmCtx->keyOps->minKey,
				pd->fsmCtx->keyOps->maxKey, redFsm->getErrorTrans() );
		destRange.append( newTel );
	}
	else {
		/* Get the last and check for a gap on the end. */
		RedTransEl *last = &destRange[destRange.length()-1];
		if ( keyOps->lt( last->highKey, pd->fsmCtx->keyOps->maxKey ) ) {
			/* Make the high key. */
			Key fillLowKey = last->highKey;
			keyOps->increment( fillLowKey );

			/* Create the new range with the error trans and append it. */
			RedTransEl newTel( fillLowKey, pd->fsmCtx->keyOps->maxKey,
					redFsm->getErrorTrans() );
			destRange.append( newTel );
		}
	}
}

void CodeGenData::setId( int snum, int id )
{
	RedStateAp *curState = allStates + snum;
	curState->id = id;
}

void CodeGenData::setFinal( int snum )
{
	RedStateAp *curState = allStates + snum;
	curState->isFinal = true;
}


void CodeGenData::setStateActions( int snum, long toStateAction, 
		long fromStateAction, long eofAction )
{
	RedStateAp *curState = allStates + snum;
	if ( toStateAction >= 0 )
		curState->toStateAction = allActionTables + toStateAction;
	if ( fromStateAction >= 0 )
		curState->fromStateAction = allActionTables + fromStateAction;
	if ( eofAction >= 0 )
		curState->eofAction = allActionTables + eofAction;
}

void CodeGenData::setEofTrans( int snum, long eofTarget, long actId )
{
	RedStateAp *curState = allStates + snum;
	RedStateAp *targState = allStates + eofTarget;
	RedAction *eofAct = allActionTables + actId;

	RedTransAp *trans = redFsm->allocateTrans( targState, eofAct );

	trans->condSpace = 0;
	trans->p.id = redFsm->nextCondId++;
	trans->p.targ = targState;
	trans->p.action = eofAct;

	curState->eofTrans = trans;
}

void CodeGenData::resolveTargetStates( GenInlineList *inlineList )
{
	for ( GenInlineList::Iter item = *inlineList; item.lte(); item++ ) {
		switch ( item->type ) {
		case GenInlineItem::Goto: case GenInlineItem::Call:
		case GenInlineItem::Ncall: case GenInlineItem::Next:
		case GenInlineItem::Entry:
			item->targState = allStates + item->targId;
			break;
		default:
			break;
		}

		if ( item->children != 0 )
			resolveTargetStates( item->children );
	}
}

void CodeGenData::resolveTargetStates()
{
	for ( GenActionList::Iter a = actionList; a.lte(); a++ )
		resolveTargetStates( a->inlineList );
}

bool CodeGenData::setAlphType( const HostLang *hostLang, const char *data )
{
	HostType *alphType = findAlphTypeInternal( hostLang, data );
	if ( alphType == 0 )
		return false;

	return true;
}

void CodeGenData::condSpaceItem( int cnum, long condActionId )
{
	GenCondSpace *cond = allCondSpaces + cnum;
	cond->condSet.append( allActions + condActionId );
}

void CodeGenData::initStateCondList( int snum, ulong length )
{
	/* Could preallocate these, as we could with transitions. */
}

void CodeGenData::addStateCond( int snum, Key lowKey, Key highKey, long condNum )
{
}

Key CodeGenData::findMaxKey()
{
	Key maxKey = pd->fsmCtx->keyOps->maxKey;
	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		assert( st->outSingle.length() == 0 );
		assert( st->defTrans == 0 );

		long rangeLen = st->outRange.length();
		if ( rangeLen > 0 ) {
			Key highKey = st->outRange[rangeLen-1].highKey;
			if ( keyOps->gt( highKey, maxKey ) )
				maxKey = highKey;
		}
	}
	return maxKey;
}

void CodeGenData::actionActionRefs( RedAction *action )
{
	action->numTransRefs += 1;
	for ( GenActionTable::Iter item = action->key; item.lte(); item++ )
		item->value->numTransRefs += 1;
}

void CodeGenData::transActionRefs( RedTransAp *trans )
{
	for ( int c = 0; c < trans->numConds(); c++ ) {
		RedCondPair *cond = trans->outCond(c);
		if ( cond->action != 0 )
			actionActionRefs( cond->action );
	}

	if ( trans->condSpace != 0 )
		trans->condSpace->numTransRefs += 1;
}

void CodeGenData::transListActionRefs( RedTransList &list )
{
	for ( RedTransList::Iter rtel = list; rtel.lte(); rtel++ )
		transActionRefs( rtel->value );
}

void CodeGenData::findFinalActionRefs()
{
	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		/* Rerence count out of single transitions. */
		transListActionRefs( st->outSingle );

		/* Reference count out of range transitions. */
		transListActionRefs( st->outRange );

		/* Reference count default transition. */
		if ( st->defTrans != 0 )
			transActionRefs( st->defTrans );

		/* Reference count EOF transitions. */
		if ( st->eofTrans != 0 )
			transActionRefs( st->eofTrans );

		/* Reference count to state actions. */
		if ( st->toStateAction != 0 ) {
			st->toStateAction->numToStateRefs += 1;
			for ( GenActionTable::Iter item = st->toStateAction->key; item.lte(); item++ )
				item->value->numToStateRefs += 1;
		}

		/* Reference count from state actions. */
		if ( st->fromStateAction != 0 ) {
			st->fromStateAction->numFromStateRefs += 1;
			for ( GenActionTable::Iter item = st->fromStateAction->key; item.lte(); item++ )
				item->value->numFromStateRefs += 1;
		}

		/* Reference count EOF actions. */
		if ( st->eofAction != 0 ) {
			st->eofAction->numEofRefs += 1;
			for ( GenActionTable::Iter item = st->eofAction->key; item.lte(); item++ )
				item->value->numEofRefs += 1;
		}

		if ( st->nfaTargs != 0 ) {
			for ( RedNfaTargs::Iter nt = *st->nfaTargs; nt.lte(); nt++ ) {

				if ( nt->push != 0 ) {
					nt->push->numNfaPushRefs += 1;
					for ( GenActionTable::Iter item = nt->push->key; item.lte(); item++ )
						item->value->numNfaPushRefs += 1;
				}

				if ( nt->popTest != 0 ) {
					nt->popTest->numNfaPopTestRefs += 1;
					for ( GenActionTable::Iter item = nt->popTest->key; item.lte(); item++ )
						item->value->numNfaPopTestRefs += 1;
				}
			}
		}
	}
}

void CodeGenData::analyzeAction( GenAction *act, GenInlineList *inlineList )
{
	for ( GenInlineList::Iter item = *inlineList; item.lte(); item++ ) {
		/* Only consider actions that are referenced. */
		if ( act->numRefs() > 0 ) {
			if ( item->type == GenInlineItem::Goto || item->type == GenInlineItem::GotoExpr )
			{
				redFsm->bAnyActionGotos = true;
			}
			else if ( item->type == GenInlineItem::Call || item->type == GenInlineItem::CallExpr ) {
				redFsm->bAnyActionCalls = true;
			}
			else if ( item->type == GenInlineItem::Ncall || item->type == GenInlineItem::NcallExpr ) {
				redFsm->bAnyActionCalls = true;
			}
			else if ( item->type == GenInlineItem::Ret )
				redFsm->bAnyActionRets = true;
			else if ( item->type == GenInlineItem::Nret )
				redFsm->bAnyActionNrets = true;
			else if ( item->type == GenInlineItem::LmInitAct || 
					item->type == GenInlineItem::LmSetActId || 
					item->type == GenInlineItem::LmSwitch )
			{
				redFsm->bUsingAct = true;
			}
		}

		/* Check for various things in regular actions. */
		if ( act->numTransRefs > 0 || act->numToStateRefs > 0 || act->numFromStateRefs > 0 ) {
			/* Any returns in regular actions? */
			if ( item->type == GenInlineItem::Ret || item->type == GenInlineItem::Nret )
				redFsm->bAnyRegActionRets = true;

			/* Any next statements in the regular actions? */
			if ( item->type == GenInlineItem::Next || item->type == GenInlineItem::NextExpr ||
					item->type == GenInlineItem::Ncall || item->type == GenInlineItem::NcallExpr ||
					item->type == GenInlineItem::Nret )
				redFsm->bAnyRegNextStmt = true;

			/* Any by value control in regular actions? */
			if ( item->type == GenInlineItem::CallExpr || item->type == GenInlineItem::GotoExpr )
				redFsm->bAnyRegActionByValControl = true;

			/* Any references to the current state in regular actions? */
			if ( item->type == GenInlineItem::Curs )
				redFsm->bAnyRegCurStateRef = true;

			if ( item->type == GenInlineItem::Break )
				redFsm->bAnyRegBreak = true;

			if ( item->type == GenInlineItem::Nbreak )
				redFsm->bAnyRegNbreak = true;
		}

		if ( item->children != 0 )
			analyzeAction( act, item->children );
	}
}

void CodeGenData::analyzeActionList( RedAction *redAct, GenInlineList *inlineList )
{
	for ( GenInlineList::Iter item = *inlineList; item.lte(); item++ ) {
		/* Any next statements in the action table? */
		if ( item->type == GenInlineItem::Next || item->type == GenInlineItem::NextExpr ||
				item->type == GenInlineItem::Ncall || item->type == GenInlineItem::NcallExpr ||
				item->type == GenInlineItem::Nret )
			redAct->bAnyNextStmt = true;

		/* Any references to the current state. */
		if ( item->type == GenInlineItem::Curs )
			redAct->bAnyCurStateRef = true;

		if ( item->type == GenInlineItem::Break )
			redAct->bAnyBreakStmt = true;

		if ( item->type == GenInlineItem::NfaWrapConds )
			item->condSpace->numNfaRefs += 1;

		if ( item->children != 0 )
			analyzeActionList( redAct, item->children );
	}
}

/* Assign ids to referenced actions. */
void CodeGenData::assignActionIds()
{
	int nextActionId = 0;
	for ( GenActionList::Iter act = actionList; act.lte(); act++ ) {
		/* Only ever interested in referenced actions. */
		if ( act->numRefs() > 0 )
			act->actionId = nextActionId++;
	}
}

void CodeGenData::setValueLimits()
{
	redFsm->maxSingleLen = 0;
	redFsm->maxRangeLen = 0;
	redFsm->maxKeyOffset = 0;
	redFsm->maxIndexOffset = 0;
	redFsm->maxActListId = 0;
	redFsm->maxActionLoc = 0;
	redFsm->maxActArrItem = 0;
	redFsm->maxSpan = 0;
	redFsm->maxFlatIndexOffset = 0;
	redFsm->maxCondSpaceId = 0;

	/* In both of these cases the 0 index is reserved for no value, so the max
	 * is one more than it would be if they started at 0. */
	redFsm->maxIndex = redFsm->transSet.length();
	redFsm->maxCond = condSpaceList.length(); 

	/* The nextStateId - 1 is the last state id assigned. */
	redFsm->maxState = redFsm->nextStateId - 1;

	for ( CondSpaceList::Iter csi = condSpaceList; csi.lte(); csi++ ) {
		if ( csi->condSpaceId > redFsm->maxCondSpaceId )
			redFsm->maxCondSpaceId = csi->condSpaceId;
	}

	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		/* Maximum single length. */
		if ( st->outSingle.length() > redFsm->maxSingleLen )
			redFsm->maxSingleLen = st->outSingle.length();

		/* Maximum range length. */
		if ( st->outRange.length() > redFsm->maxRangeLen )
			redFsm->maxRangeLen = st->outRange.length();

		/* The key offset index offset for the state after last is not used, skip it.. */
		if ( ! st.last() ) {
			redFsm->maxKeyOffset += st->outSingle.length() + st->outRange.length()*2;
			redFsm->maxIndexOffset += st->outSingle.length() + st->outRange.length() + 2;
		}

		/* Max key span. */
		if ( st->transList != 0 ) {
			unsigned long long span = pd->fsmCtx->keyOps->span( st->lowKey, st->highKey );
			if ( span > redFsm->maxSpan )
				redFsm->maxSpan = span;
		}

		/* Max flat index offset. */
		if ( ! st.last() ) {
			if ( st->transList != 0 )
				redFsm->maxFlatIndexOffset += pd->fsmCtx->keyOps->span( st->lowKey, st->highKey );
			redFsm->maxFlatIndexOffset += 1;
		}
	}

	for ( GenActionTableMap::Iter at = redFsm->actionMap; at.lte(); at++ ) {
		/* Maximum id of action lists. */
		if ( at->actListId+1 > redFsm->maxActListId )
			redFsm->maxActListId = at->actListId+1;

		/* Maximum location of items in action array. */
		if ( at->location+1 > redFsm->maxActionLoc )
			redFsm->maxActionLoc = at->location+1;

		/* Maximum values going into the action array. */
		if ( at->key.length() > redFsm->maxActArrItem )
			redFsm->maxActArrItem = at->key.length();
		for ( GenActionTable::Iter item = at->key; item.lte(); item++ ) {
			if ( item->value->actionId > redFsm->maxActArrItem )
				redFsm->maxActArrItem = item->value->actionId;
		}
	}
}



/* Gather various info on the machine. */
void CodeGenData::analyzeMachine()
{
	/* Find the true count of action references.  */
	findFinalActionRefs();

	/* Check if there are any calls in action code. */
	for ( GenActionList::Iter act = actionList; act.lte(); act++ ) {
		/* Record the occurrence of various kinds of actions. */
		if ( act->numToStateRefs > 0 )
			redFsm->bAnyToStateActions = true;
		if ( act->numFromStateRefs > 0 )
			redFsm->bAnyFromStateActions = true;
		if ( act->numEofRefs > 0 )
			redFsm->bAnyEofActions = true;
		if ( act->numTransRefs > 0 )
			redFsm->bAnyRegActions = true;

		if ( act->numNfaPushRefs > 0 ) {
			redFsm->bAnyNfaPushPops = true;
			redFsm->bAnyNfaPushes = true;
		}

		if ( act->numNfaPopActionRefs > 0 ) {
			redFsm->bAnyNfaPushPops = true;
			redFsm->bAnyNfaPops = true;
		}

		if ( act->numNfaPopTestRefs > 0 ) {
			redFsm->bAnyNfaPushPops = true;
			redFsm->bAnyNfaPops = true;
		}

		/* Recurse through the action's parse tree looking for various things. */
		analyzeAction( act, act->inlineList );
	}

	/* Analyze reduced action lists. */
	for ( GenActionTableMap::Iter redAct = redFsm->actionMap; redAct.lte(); redAct++ ) {
		for ( GenActionTable::Iter act = redAct->key; act.lte(); act++ )
			analyzeActionList( redAct, act->value->inlineList );
	}

	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		if ( st->nfaTargs != 0 )
			redFsm->bAnyNfaStates = true;
	}

	/* Find states that have transitions with actions that have next
	 * statements. */
	for ( RedStateList::Iter st = redFsm->stateList; st.lte(); st++ ) {
		/* Check any actions out of outSinge. */
		for ( RedTransList::Iter rtel = st->outSingle; rtel.lte(); rtel++ ) {
			RedTransAp *trans = rtel->value;
			for ( int c = 0; c < trans->numConds(); c++ ) {
				RedCondPair *cond = trans->outCond(c);
				if ( cond->action != 0 && cond->action->anyCurStateRef() )
					st->bAnyRegCurStateRef = true;
			}
		}

		/* Check any actions out of outRange. */
		for ( RedTransList::Iter rtel = st->outRange; rtel.lte(); rtel++ ) {
			RedTransAp *trans = rtel->value;
			for ( int c = 0; c < trans->numConds(); c++ ) {
				RedCondPair *cond = trans->outCond(c);
				if ( cond->action != 0 && cond->action->anyCurStateRef() )
					st->bAnyRegCurStateRef = true;
			}
		}

		/* Check any action out of default. */
		if ( st->defTrans != 0 ) {
			RedTransAp *trans = st->defTrans;
			for ( int c = 0; c < trans->numConds(); c++ ) {
				RedCondPair *cond = trans->outCond(c);
				if ( cond->action != 0 && cond->action->anyCurStateRef() )
					st->bAnyRegCurStateRef = true;
			}
		}

		if ( st->eofTrans != 0 )
			redFsm->bAnyEofTrans = true;
	}

	for ( CondSpaceList::Iter csi = condSpaceList; csi.lte(); csi++ ) {
		GenCondSpace *condSpace = csi;

		if ( condSpace->numTransRefs > 0 )
			redFsm->bAnyTransCondRefs = true;

		if ( condSpace->numNfaRefs > 0 )
			redFsm->bAnyNfaCondRefs = true;
	}

	/* Assign ids to actions that are referenced. */
	assignActionIds();

	/* Set the maximums of various values used for deciding types. */
	setValueLimits();
}

void CodeGenData::write_option_error( InputLoc &loc, std::string arg )
{
	source_warning(loc) << "unrecognized write option \"" << arg << "\"" << std::endl;
}

void CodeGenData::writeStatement( InputLoc &loc, int nargs,
		std::string *args, bool generateDot, const HostLang *hostLang )
{
	/* Start write generation on a fresh line. */
	out << '\n';

	if ( args[0] == "data" ) {
		for ( int i = 1; i < nargs; i++ ) {
			if ( args[i] == "noerror" )
				noError = true;
			else if ( args[i] == "noprefix" )
				noPrefix = true;
			else if ( args[i] == "nofinal" )
				noFinal = true;
			else
				write_option_error( loc, args[i] );
		}

		if ( pd->id->printStatistics ) {
			std::cout << "fsm-name\t" << fsmName << std::endl;
			std::cout << "fsm-states\t" << redFsm->stateList.length() << std::endl;
		}

		writeData();
		statsSummary();
	}
	else if ( args[0] == "init" ) {
		for ( int i = 1; i < nargs; i++ ) {
			if ( args[i] == "nocs" )
				noCS = true;
			else
				write_option_error( loc, args[i] );
		}
		writeInit();
	}
	else if ( args[0] == "exec" ) {
		for ( int i = 1; i < nargs; i++ ) {
			if ( args[i] == "noend" )
				noEnd = true;
			else
				write_option_error( loc, args[i] );
		}
		writeExec();
	}
	else if ( args[0] == "exports" ) {
		for ( int i = 1; i < nargs; i++ )
			write_option_error( loc, args[i] );
		writeExports();
	}
	else if ( args[0] == "start" ) {
		for ( int i = 1; i < nargs; i++ )
			write_option_error( loc, args[i] );
		writeStart();
	}
	else if ( args[0] == "first_final" ) {
		for ( int i = 1; i < nargs; i++ )
			write_option_error( loc, args[i] );
		writeFirstFinal();
	}
	else if ( args[0] == "error" ) {
		for ( int i = 1; i < nargs; i++ )
			write_option_error( loc, args[i] );
		writeError();
	}
	else {
		/* EMIT An error here. */
		source_error(loc) << "unrecognized write command \"" << 
				args[0] << "\"" << std::endl;
	}
}

ostream &CodeGenData::source_warning( const InputLoc &loc )
{
	std::cerr << sourceFileName << ":" << loc.line << ":" << loc.col << ": warning: ";
	return std::cerr;
}

ostream &CodeGenData::source_error( const InputLoc &loc )
{
	gblErrorCount += 1;
	std::cerr << sourceFileName << ":" << loc.line << ":" << loc.col << ": ";
	return std::cerr;
}


