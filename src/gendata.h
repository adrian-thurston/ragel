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
	GenBase( std::string fsmName, int machineId, ParseData *pd, FsmAp *fsm );

	void appendTrans( TransListVect &outList, Key lowKey, Key highKey, TransAp *trans );
	void reduceActionTables();

	std::string fsmName;
	int machineId;
	ParseData *pd;
	FsmAp *fsm;
	KeyOps *keyOps;

	ActionTableMap actionTableMap;
	int nextActionTableId;
};

using std::ostream;

struct NameInst;
typedef DList<GenAction> GenActionList;

typedef unsigned long ulong;

extern int gblErrorCount;

struct CodeGenData;
struct InputData;
struct ParseData;
struct FsmAp;

typedef AvlMap<char *, CodeGenData*, CmpStr> CodeGenMap;
typedef AvlMapEl<char *, CodeGenData*> CodeGenMapEl;

void openHostBlock( char opener, InputData *id, ostream &out, const char *fileName, int line );

string itoa( int i );

struct CodeGenArgs;
CodeGenData *makeCodeGen( const HostLang *hostLang, const CodeGenArgs &args );


/*********************************/

struct CodeGenArgs
{
	CodeGenArgs( std::string sourceFileName, std::string fsmName,
			int machineId, ParseData *pd, FsmAp *fsm, CodeStyle codeStyle,
			std::ostream &out )
	:
		sourceFileName(sourceFileName),
		fsmName(fsmName),
		machineId(machineId),
		pd(pd),
		fsm(fsm),
		codeStyle(codeStyle),
		out(out)
	{}

	std::string sourceFileName;
	std::string fsmName;
	int machineId;
	ParseData *pd;
	FsmAp *fsm;
	CodeStyle codeStyle;
	std::ostream &out;
};

struct CodeGenData : protected GenBase
{
public:
	CodeGenData( const CodeGenArgs &args );
	void make( const HostLang *hostLang );

private:
	void makeGenInlineList( GenInlineList *outList, InlineList *inList );
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
	void makeExports();
	void makeMachine();
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

	/* Collected during parsing. */
	int curAction;
	int curActionTable;
	int curState;

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

	/* 
	 * Collecting the machine.
	 */

	std::string sourceFileName;
	std::string fsmName;
	ostream &out;
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

	/* Write options. */
	bool noEnd;
	bool noPrefix;
	bool noFinal;
	bool noError;
	bool noCS;

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

	bool setAlphType( const HostLang *hostLang, const char *data );

	void resolveTargetStates( GenInlineList *inlineList );
	Key findMaxKey();

	void makeSubList( GenInlineList *outList, const InputLoc &loc,
			InlineList *inlineList, GenInlineItem::Type type );

	/* Gather various info on the machine. */
	void analyzeActionList( RedAction *redAct, GenInlineList *inlineList );
	void analyzeAction( GenAction *act, GenInlineList *inlineList );
	void actionActionRefs( RedAction *action );
	void transListActionRefs( RedTransList &list );
	void transActionRefs( RedTransAp *trans );
	void findFinalActionRefs();
	void analyzeMachine();

	void resolveTargetStates();
	void setValueLimits();
	void assignActionIds();

	ostream &source_warning( const InputLoc &loc );
	ostream &source_error( const InputLoc &loc );
	void write_option_error( InputLoc &loc, std::string arg );
};

#endif
