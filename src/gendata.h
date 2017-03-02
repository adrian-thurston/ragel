/*
 * Copyright 2005-2007 Adrian Thurston <thurston@colm.net>
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

#ifndef _GENDATA_H
#define _GENDATA_H

#include <iostream>
#include <string>
#include <vector>
#include "config.h"
#include "redfsm.h"
#include "common.h"
#include "fsmgraph.h"

/* Forwards. */
struct TransAp;
struct FsmAp;
struct PdBase;
struct InputData;
struct FsmGbl;
struct GenInlineList;
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

struct RedBase
{
	RedBase( FsmGbl *id, FsmCtx *fsmCtx, FsmAp *fsm, std::string fsmName, int machineId )
	:
		id(id),
		fsmCtx(fsmCtx),
		fsm(fsm),
		fsmName(fsmName),
		machineId(machineId),
		keyOps(fsm->ctx->keyOps),
		nextActionTableId(0)
	{
	}

	FsmGbl *id;
	FsmCtx *fsmCtx;
	FsmAp *fsm;
	std::string fsmName;
	int machineId;

	KeyOps *keyOps;

	ActionTableMap actionTableMap;
	int nextActionTableId;
};

struct NameInst;
typedef DList<GenAction> GenActionList;

typedef unsigned long ulong;

void openHostBlock( char opener, InputData *id, std::ostream &out, const char *fileName, int line );

string itoa( int i );

struct Reducer
	: public RedBase
{
	Reducer( FsmGbl *id, FsmCtx *fsmCtx, FsmAp *fsm, std::string fsmName, int machineId )
	:
		RedBase( id, fsmCtx, fsm, fsmName, machineId ),
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

	~Reducer()
	{
		if ( redFsm != 0 )
			delete redFsm;

		delete[] allActions;
		delete[] allActionTables;
		delete[] allConditions;
		delete[] allCondSpaces;

		actionTableMap.empty();

		if ( getKeyExpr != 0 )
			delete getKeyExpr;
		if ( accessExpr != 0 )
			delete accessExpr;
		if ( prePushExpr != 0 )
			delete prePushExpr;
		if ( postPopExpr != 0 )
			delete postPopExpr;
		if ( nfaPrePushExpr != 0 )
			delete nfaPrePushExpr;
		if ( nfaPostPopExpr != 0 )
			delete nfaPostPopExpr;
		if ( pExpr != 0 )
			delete pExpr;
		if ( peExpr != 0 )
			delete peExpr;
		if ( eofExpr != 0 )
			delete eofExpr;
		if ( csExpr != 0 )
			delete csExpr;
		if ( topExpr != 0 )
			delete topExpr;
		if ( stackExpr != 0 )
			delete stackExpr;
		if ( actExpr != 0 )
			delete actExpr;
		if ( tokstartExpr != 0 )
			delete tokstartExpr;
		if ( tokendExpr != 0 )
			delete tokendExpr;
		if ( dataExpr != 0 )
			delete dataExpr;
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


	void appendTrans( TransListVect &outList, Key lowKey, Key highKey, TransAp *trans );
	void reduceActionTables();

public:

	Key findMaxKey();
	void makeMachine();
	void makeExports();
	void makeGenInlineList( GenInlineList *outList, InlineList *inList );
	bool setAlphType( const HostLang *hostLang, const char *data );
	void analyzeMachine();
	void make( const HostLang *hostLang, const HostType *alphType );

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

struct CodeGenArgs
{
	CodeGenArgs( FsmGbl *id, Reducer *red, HostType *alphType, int machineId, std::string sourceFileName,
			std::string fsmName, std::ostream &out, CodeStyle codeStyle )
	:
		id(id),
		red(red),
		alphType(alphType),
		machineId(machineId),
		sourceFileName(sourceFileName),
		fsmName(fsmName),
		out(out),
		codeStyle(codeStyle),
		lineDirectives(true)
	{}

	FsmGbl *id;
	Reducer *red;
	HostType *alphType;
	int machineId;
	std::string sourceFileName;
	std::string fsmName;
	std::ostream &out;
	CodeStyle codeStyle;
	bool lineDirectives;
};

struct CodeGenData
{
	CodeGenData( const CodeGenArgs &args )
	:
		red(args.red),
		redFsm(args.red->redFsm),
		sourceFileName(args.sourceFileName),
		fsmName(args.fsmName), 
		keyOps(red->keyOps),
		alphType(args.alphType),
		out(args.out),
		noEnd(false),
		noPrefix(false),
		noFinal(false),
		noError(false),
		noCS(false),
		lineDirectives(args.lineDirectives),
		cleared(false)
	{
	}

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
	virtual void writeClear();

	/* Show some stats after a write data. */
	virtual void statsSummary() = 0;

	/* This can also be overridden to modify the processing of write
	 * statements. */
	virtual void writeStatement( InputLoc &loc, int nargs,
			std::vector<std::string> &args, bool generateDot, const HostLang *hostLang );

	/********************/

	virtual ~CodeGenData()
	{
	}

	void clear()
	{
		delete red->redFsm;
		red->redFsm = 0;
	}

protected:

	Reducer *red;
	RedFsmAp *redFsm;
	std::string sourceFileName;
	std::string fsmName;
	KeyOps *keyOps;
	HostType *alphType;
	ostream &out;

	/* Write options. */
	bool noEnd;
	bool noPrefix;
	bool noFinal;
	bool noError;
	bool noCS;

	void write_option_error( InputLoc &loc, std::string arg );

	bool lineDirectives;
	bool cleared;
};

/* Selects and constructs the codegen based on the output options. */
CodeGenData *makeCodeGen( const HostLang *hostLang, const CodeGenArgs &args );

typedef AvlMap<char *, CodeGenData*, CmpStr> CodeGenMap;
typedef AvlMapEl<char *, CodeGenData*> CodeGenMapEl;

#endif
