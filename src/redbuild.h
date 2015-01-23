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

#ifndef _FSMREDUCE_H
#define _FSMREDUCE_H

#include <iostream>
#include "avltree.h"
#include "fsmgraph.h"
#include "compiler.h"

/* Forwards. */
struct FsmTrans;
struct FsmGraph;
struct Compiler;
struct FsmCodeGen;
struct RedFsm;
struct GenCondSpace;
struct Condition;

struct RedActionTable
:
	public AvlTreeEl<RedActionTable>
{
	RedActionTable( const ActionTable &key )
	:	
		key(key), 
		id(0)
	{ }

	const ActionTable &getKey() 
		{ return key; }

	ActionTable key;
	int id;
};

typedef AvlTree<RedActionTable, ActionTable, CmpActionTable> ActionTableMap;

struct NextRedTrans
{
	Key lowKey, highKey;
	FsmTrans *trans;
	FsmTrans *next;

	void load() {
		if ( trans != 0 ) {
			next = trans->next;
			lowKey = trans->lowKey;
			highKey = trans->highKey;
		}
	}

	NextRedTrans( FsmTrans *t ) {
		trans = t;
		load();
	}

	void increment() {
		trans = next;
		load();
	}
};

class RedFsmBuild
{
public:
	RedFsmBuild( Compiler *pd, FsmGraph *fsm );
	RedFsm *reduceMachine( );

private:
	void appendTrans( TransListVect &outList, Key lowKey, Key highKey, FsmTrans *trans );
	void makeStateActions( FsmState *state );
	void makeStateList();
	void makeStateConditions( FsmState *state );

	void initActionList( unsigned long length );
	void newAction( int anum, char *name, int line, int col, Action *action );
	void initActionTableList( unsigned long length );
	void initCondSpaceList( ulong length );
	void condSpaceItem( int cnum, long condActionId );
	void newCondSpace( int cnum, int condSpaceId, Key baseKey );
	void initStateCondList( int snum, ulong length );
	void addStateCond( int snum, Key lowKey, Key highKey, long condNum );
	void initStateList( unsigned long length );
	void addRegionToEntry( int regionId, int entryId );
	void addEntryPoint( int entryId, unsigned long entryState );
	void setId( int snum, int id );
	void initTransList( int snum, unsigned long length );
	void newTrans( int snum, int tnum, Key lowKey, Key highKey, 
			long targ, long act );
	void finishTransList( int snum );
	void setFinal( int snum );
	void setEofTrans( int snum, int eofTarget, int actId );
	void setStateActions( int snum, long toStateAction, 
			long fromStateAction, long eofAction );
	void setForcedErrorState();
	void closeMachine();
	Key findMaxKey();

	void makeEntryPoints();
	void makeGetKeyExpr();
	void makeAccessExpr();
	void makeCurStateExpr();
	void makeConditions();
	void makeInlineList( InlineList *inlineList, InlineItem *context );
	void makeActionList();
	void makeActionTableList();
	void reduceTrans( FsmTrans *trans );
	void reduceActionTables();
	void makeTransList( FsmState *state );
	void makeTrans( Key lowKey, Key highKey, FsmTrans *defTrans );
	void makeAction( Action *action );
	void makeLmSwitch( InlineItem *item );
	void makeMachine();
	void makeActionExec( InlineItem *item );
	void makeActionExecTE( InlineItem *item );

	Compiler *pd;
	FsmGraph *fsm;
	ActionTableMap actionTableMap;
	int nextActionTableId;

	int startState;
	int errState;

public:
	RedFsm *redFsm;

private:
	int curAction;
	int curActionTable;
	int curTrans;
	int curState;
	int curCondSpace;
	int curStateCond;
};


#endif /* _FSMREDUCE_H */
