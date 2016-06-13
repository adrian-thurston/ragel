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

#ifndef _GENDATA_H
#define _GENDATA_H

#include <iostream>
#include "config.h"
#include "redfsm.h"
#include "common.h"
#include "fsmgraph.h"

/* Forwards. */
struct TransAp;
struct FsmAp;
struct ParseData;
struct PdBase;
struct InputData;
struct IdBase;
struct GenInlineList;
struct CodeGenData;
struct InlineItem;

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
	TransAp *trans;
	TransAp *next;

	void load() {
		if ( trans != 0 ) {
			next = trans->next;
			lowKey = trans->lowKey;
			highKey = trans->highKey;
		}
	}

	NextRedTrans( TransAp *t ) {
		trans = t;
		load();
	}

	void increment() {
		trans = next;
		load();
	}
};

struct GenBase
{
	GenBase( std::string fsmName, int machineId, IdBase *id, PdBase *pd, FsmAp *fsm );

	void appendTrans( TransListVect &outList, Key lowKey, Key highKey, TransAp *trans );
	void reduceActionTables();

	std::string fsmName;
	int machineId;
	IdBase *id;
	PdBase *pd;
	FsmAp *fsm;
	KeyOps *keyOps;

	ActionTableMap actionTableMap;
	int nextActionTableId;
};

using std::ostream;

struct NameInst;
typedef DList<GenAction> GenActionList;

typedef unsigned long ulong;

struct CodeGenData;
struct InputData;
struct ParseData;
struct FsmAp;

typedef AvlMap<char *, CodeGenData*, CmpStr> CodeGenMap;
typedef AvlMapEl<char *, CodeGenData*> CodeGenMapEl;

void openHostBlock( char opener, InputData *id, ostream &out, const char *fileName, int line );

string itoa( int i );

struct CodeGenArgs;
struct Reducer;

CodeGenData *makeCodeGen( const HostLang *hostLang, const CodeGenArgs &args );


/*********************************/

struct CodeGenArgs
{
	CodeGenArgs( Reducer *red, std::string sourceFileName, std::string fsmName,
			int machineId, IdBase *id, PdBase *pd, FsmAp *fsm, CodeStyle codeStyle,
			std::ostream &out )
	:
		red(red),
		sourceFileName(sourceFileName),
		fsmName(fsmName),
		machineId(machineId),
		id(id),
		pd(pd),
		fsm(fsm),
		codeStyle(codeStyle),
		out(out)
	{}

	Reducer *red;
	std::string sourceFileName;
	std::string fsmName;
	int machineId;
	IdBase *id;
	PdBase *pd;
	FsmAp *fsm;
	CodeStyle codeStyle;
	std::ostream &out;
};

struct Reducer
	: public GenBase
{
	Reducer( std::string fsmName, int machineId, IdBase *id, PdBase *pd, FsmAp *fsm )
	:
		GenBase( fsmName, machineId, id, pd, fsm ),
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
		hasLongestMatch(false)
	{
	}

protected:
	/* Collected during parsing. */
	int curAction;
	int curActionTable;
	int curState;

	void makeKey( GenInlineList *outList, Key key );
	void makeText( GenInlineList *outList, InlineItem *item );
	void makeLmOnLast( GenInlineList *outList, InlineItem *item );
	void makeLmOnNext( GenInlineList *outList, InlineItem *item );
	void makeLmOnLagBehind( GenInlineList *outList, InlineItem *item );
	void makeActionExec( GenInlineList *outList, InlineItem *item );
	void makeLmSwitch( GenInlineList *outList, InlineItem *item );
	void makeSetTokend( GenInlineList *outList, long offset );
	void makeSetAct( GenInlineList *outList, long lmId );
	void makeSubList( GenInlineList *outList, InlineList *inlineList, 
			GenInlineItem::Type type );
	void makeTargetItem( GenInlineList *outList, NameInst *nameTarg,
			GenInlineItem::Type type );
	void makeExecGetTokend( GenInlineList *outList );
	void makeActionList();
	void makeAction( Action *action );
	void makeActionTableList();
	void makeConditions();
	void makeEntryPoints();
	bool makeNameInst( std::string &out, NameInst *nameInst );
	void makeStateList();

	void makeStateActions( StateAp *state );
	void makeEofTrans( StateAp *state );
	void makeTransList( StateAp *state );
	void makeTrans( Key lowKey, Key highKey, TransAp *trans );
	void newTrans( RedStateAp *state, Key lowKey, Key highKey, RedTransAp *trans );

	void makeSubList( GenInlineList *outList, const InputLoc &loc,
			InlineList *inlineList, GenInlineItem::Type type );

	void createMachine();
	void initActionList( unsigned long length );
	void newAction( int anum, std::string name,
			const InputLoc &loc, GenInlineList *inlineList );
	void initActionTableList( unsigned long length );
	void initStateList( unsigned long length );
	void setStartState( unsigned long startState );
	void setErrorState( unsigned long errState );
	void addEntryPoint( char *name, unsigned long entryState );
	void setId( int snum, int id );
	void setFinal( int snum );
	void initTransList( int snum, unsigned long length );

	void newTrans( int snum, int tnum, Key lowKey, Key highKey,
			GenCondSpace *gcs, RedTransAp *trans );

	void finishTransList( int snum );
	void setStateActions( int snum, long toStateAction, 
			long fromStateAction, long eofAction );
	void setEofTrans( int snum, long targ, long eofAction );
	void setForcedErrorState()
		{ redFsm->forcedErrorState = true; }

	void condSpaceItem( int cnum, long condActionId );
	void newCondSpace( int cnum, int condSpaceId );

	void initStateCondList( int snum, ulong length );
	void addStateCond( int snum, Key lowKey, Key highKey, long condNum );


	void resolveTargetStates( GenInlineList *inlineList );
	void resolveTargetStates();


	/* Gather various info on the machine. */
	void analyzeActionList( RedAction *redAct, GenInlineList *inlineList );
	void analyzeAction( GenAction *act, GenInlineList *inlineList );
	void actionActionRefs( RedAction *action );
	void transListActionRefs( RedTransList &list );
	void transActionRefs( RedTransAp *trans );
	void findFinalActionRefs();

	void setValueLimits();
	void assignActionIds();


public:

	Key findMaxKey();
	void makeMachine();
	void makeExports();
	void makeGenInlineList( GenInlineList *outList, InlineList *inList );
	bool setAlphType( const HostLang *hostLang, const char *data );
	void analyzeMachine();

	/* 
	 * Collecting the machine.
	 */

	RedFsmAp *redFsm;
	GenAction *allActions;
	RedAction *allActionTables;
	Condition *allConditions;
	GenCondSpace *allCondSpaces;
	RedStateAp *allStates;
	NameInst **nameIndex;
	int startState;
	int errState;
	GenActionList actionList;
	CondSpaceList condSpaceList;

	GenInlineList *getKeyExpr;
	GenInlineList *accessExpr;
	GenInlineExpr *prePushExpr;
	GenInlineExpr *postPopExpr;

	GenInlineExpr *nfaPrePushExpr;
	GenInlineExpr *nfaPostPopExpr;

	/* Overriding variables. */
	GenInlineList *pExpr;
	GenInlineList *peExpr;
	GenInlineList *eofExpr;
	GenInlineList *csExpr;
	GenInlineList *topExpr;
	GenInlineList *stackExpr;
	GenInlineList *actExpr;
	GenInlineList *tokstartExpr;
	GenInlineList *tokendExpr;
	GenInlineList *dataExpr;

	EntryIdVect entryPointIds;
	EntryNameVect entryPointNames;
	bool hasLongestMatch;
	ExportList exportList;
	Action *curInlineAction;
};

struct CodeGenData
//	: public Reducer
{
public:
	CodeGenData( Reducer *red, const CodeGenArgs &args );
	void make( const HostLang *hostLang );

private:

public:
	/*
	 * The interface to the code generator.
	 */
	virtual void genAnalysis() = 0;

	/* These are invoked by writeStatement and are normally what are used to
	 * implement the code generators. */
	virtual void writeData() {};
	virtual void writeInit() {};
	virtual void writeExec() {};
	virtual void writeExports() {};
	virtual void writeStart() {};
	virtual void writeFirstFinal() {};
	virtual void writeError() {};

	/* Show some stats after a write data. */
	virtual void statsSummary() = 0;

	/* This can also be overridden to modify the processing of write
	 * statements. */
	virtual void writeStatement( InputLoc &loc, int nargs,
			std::string *args, bool generateDot, const HostLang *hostLang );

	/********************/

	virtual ~CodeGenData() {}

	Reducer *red;

	KeyOps *keyOps;
	FsmAp *fsm;
	RedFsmAp *redFsm;

	std::string sourceFileName;
	std::string fsmName;
	ostream &out;

	/* Write options. */
	bool noEnd;
	bool noPrefix;
	bool noFinal;
	bool noError;
	bool noCS;

	void write_option_error( InputLoc &loc, std::string arg );

	bool lineDirectives;
};

#endif
