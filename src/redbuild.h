/*
 * Copyright 2006-2012 Adrian Thurston <thurston@colm.net>
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

#ifndef _COLM_FSMREDUCE_H
#define _COLM_FSMREDUCE_H

#include <iostream>

#include <avltree.h>

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

#endif /* _COLM_FSMREDUCE_H */

