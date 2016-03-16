/*
 *  Copyright 2001-2012 Adrian Thurston <thurston@complang.org>
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

#ifndef _PARSEDATA_H
#define _PARSEDATA_H

#include <iostream>
#include <limits.h>
#include "bstset.h"
#include "global.h"
#include "avlmap.h"
#include "avlset.h"
#include "bstmap.h"
#include "vector.h"
#include "dlist.h"
#include "dlistmel.h"
#include "fsmgraph.h"
#include "compare.h"
#include "vector.h"
#include "keyops.h"
#include "parsetree.h"
#include "cstring.h"
#include "pdagraph.h"
#include "compare.h"
#include "pdarun.h"
#include "bytecode.h"
#include "program.h"
#include "internal.h"

using std::ostream;

struct exit_object { };
extern exit_object endp;
void operator<<( std::ostream &out, exit_object & );
extern const char *objectName;
extern bool hostAdapters;

/* Forwards. */
struct RedFsm;
struct LangEl;
struct Compiler;
struct PdaCodeGen;
struct FsmCodeGen;

#define SHIFT_CODE 0x1
#define REDUCE_CODE 0x2
#define SHIFT_REDUCE_CODE 0x3

typedef Vector<const char**> CharVectVect;

/* This is used for tracking the current stack of include file/machine pairs. It is
 * is used to detect and recursive include structure. */
struct IncludeStackItem
{
	IncludeStackItem( const char *fileName )
		: fileName(fileName) {}

	const char *fileName;
};

typedef Vector<IncludeStackItem> IncludeStack;
typedef Vector<const char *> ArgsVector;

extern ArgsVector includePaths;

inline long makeReduceCode( long reduction, bool isShiftReduce )
{
	return ( isShiftReduce ? SHIFT_REDUCE_CODE : REDUCE_CODE ) | 
		( reduction << 2 );
}

struct ProdEl;
struct ProdElList;
struct PdaLiteral;
struct Production;

/* A pointer to this is in struct pda_run, but it's specification is not known by the
 * runtime code. The runtime functions that access it are defined in
 * ctinput.cpp and stubbed in fsmcodegen.cpp */
struct bindings
	: public Vector<parse_tree_t*>
{};

struct DefListEl { Production *prev, *next; };
struct LelDefListEl { Production *prev, *next; };
typedef Vector< LangEl* > LangElVect;
typedef Vector< ProdEl* > FactorVect;

typedef AvlMap<String, long, CmpStr> StringMap;
typedef AvlMapEl<String, long> StringMapEl;

enum PredType { 
	PredLeft,
	PredRight,
	PredNonassoc,
	PredNone
};

struct PredDecl
{
	PredDecl( TypeRef *typeRef, long predValue )
		: typeRef(typeRef), predValue(predValue)
	{}

	TypeRef *typeRef;
	PredType predType;
	long predValue;

	PredDecl *prev, *next;
};

typedef DList<PredDecl> PredDeclList;

/* Graph dictionary. */
struct Production 
:
	public DefListEl, public LelDefListEl
{
	Production()
	: 
		prodName(0), prodElList(0), prodCommit(false), redBlock(0),
		prodId(0), prodNum(0), fsm(0), fsmLength(0), uniqueEmptyLeader(0),
		isLeftRec(false), localFrame(0), lhsField(0), predOf(0) {}

	static Production* cons( const InputLoc &loc, LangEl *prodName, ProdElList *prodElList, 
			String name, bool prodCommit, CodeBlock *redBlock, int prodId, int prodNum )
	{
		Production *p = new Production;
		p->loc = loc;
		p->prodName = prodName;
		p->name = name;
		p->prodElList = prodElList;
		p->prodCommit = prodCommit;
		p->redBlock = redBlock;
		p->prodId = prodId;
		p->prodNum = prodNum;
		return p;
	}

	InputLoc loc;
	LangEl *prodName;
	ProdElList *prodElList;
	String name;
	bool prodCommit;

	CodeBlock *redBlock;

	int prodId;
	int prodNum;

	PdaGraph *fsm;
	int fsmLength;
	String data;
	LongSet reducesTo;

	LangEl *uniqueEmptyLeader;

	ProdIdSet nonTermFirstSet;
	AlphSet firstSet;
	bool isLeftRec;

	ObjectDef *localFrame;
	ObjectField *lhsField;

	LangEl *predOf;

	UnsignedCharVect copy;
};

struct CmpDefById
{
	static int compare( Production *d1, Production *d2 )
	{
		if ( d1->prodId < d2->prodId )
			return -1;
		else if ( d1->prodId > d2->prodId )
			return 1;
		else
			return 0;
	}
};


/* Map dotItems to productions. */
typedef BstMap< int, Production*, CmpOrd<int> > DotItemIndex;
typedef BstMapEl< int, Production*> DotItemIndexEl;

struct DefList
:
	public DListMel<Production, DefListEl>
{};

/* A vector of production vectors. Each non terminal can have many productions. */
struct LelDefList
:
	public DListMel<Production, LelDefListEl> 
{};

/* A set of machines made during a closure round. */
typedef Vector< PdaGraph* > Machines;

/* List of language elements. */
typedef DList<LangEl> LelList;

typedef Vector< TokenInstance* > TokenInstanceVect;

struct UniqueType;

typedef Vector<LangEl*> LangElVect;
typedef BstSet<LangEl*> LangElSet;

/* A language element class. Can be a nonTerm or a term. */
struct LangEl : public DListEl<LangEl>
{
	enum Type { Unknown, Term, NonTerm };

	LangEl( Namespace *nspace, const String &name, Type type );
	~LangEl();

	/* The region the language element was defined in. */
	Namespace *nspace;

	String name;
	String lit;

	String fullName;
	String fullLit;

	/* For referencing the type. */
	String refName;

	/* For declaring things inside the type. */
	String declName;

	String xmlTag;

	Type type;
	long id;
	String displayString;
	long numAppearances;
	bool commit;
	bool isIgnore;
	bool reduceFirst;
	bool isLiteral;
	bool isRepeat;
	bool isList;
	bool isOpt;
	bool parseStop;
	bool isEOF;

	LangEl *repeatOf;

	/* Productions from the language element if it is a non-terminal. */
	LelDefList defList;

	TokenDef *tokenDef;
	Production *rootDef;
	LangEl *termDup;
	LangEl *eofLel;

	PdaGraph *pdaGraph;
	struct pda_tables *pdaTables;

	PdaState *startState;

	CodeBlock *transBlock;

	ObjectDef *objectDef;

	long thisSize;
	long ofiOffset;

	long parserId;

	PredType predType;
	long predValue;

	StructDef *contextDef;
	StructDef *contextIn;
	bool noPreIgnore;
	bool noPostIgnore;
	bool isZero;
	RegionSet *regionSet;
};

struct ProdEl
{
	/* Language elements a factor node can be. */
	enum Type {
		LiteralType, 
		ReferenceType
	}; 

	/* Construct with a reference to a var def. */
	ProdEl( Type type, const InputLoc &loc, ObjectField *captureField,
			bool commit, TypeRef *typeRef, int priorVal )
	:
		type(type), 
		production(0),
		captureField(captureField),
		rhsElField(0),
		commit(commit),
		typeRef(typeRef),
		langEl(0),
		priorVal(priorVal)
	{}

	ProdEl( const InputLoc &loc, TypeRef *typeRef )
	:
		type(ReferenceType),
		production(0),
		captureField(0), 
		rhsElField(0),
		commit(false), 
		typeRef(typeRef), 
		langEl(0), 
		priorVal(0)
	{}

	Type type;
	Production *production;
	int pos;
	ObjectField *captureField;
	ObjectField *rhsElField;
	bool commit;
	TypeRef *typeRef;
	LangEl *langEl;
	int priorVal;

	ProdEl *prev, *next;
};

struct ProdElList : public DList<ProdEl>
{
	PdaGraph *walk( Compiler *pd, Production *prod );
};

/* This should be renamed. It is a literal string in a type reference. */
struct PdaLiteral
{
	PdaLiteral( const InputLoc &loc, const String &data )
		: loc(loc), data(data), value(0) { }

	InputLoc loc;
	String data;
	long value;
};

/* Nodes in the tree that use this action. */
typedef Vector<NameInst*> ActionRefs;

/* Element in list of actions. Contains the string for the code to exectute. */
struct Action 
:
	public DListEl<Action>,
	public AvlTreeEl<Action>
{
public:

	static Action *cons( const InputLoc &loc, const String &name, InlineList *inlineList )
	{
		Action *a = new Action;
		a->loc = (loc);
		a->name = (name);
		a->markType = (MarkNone);
		a->objField = (0);
		a->markId = (-1);
		a->inlineList = (inlineList);
		a->actionId = (-1);
		a->numTransRefs = (0);
		a->numToStateRefs = (0);
		a->numFromStateRefs = (0);
		a->numEofRefs = (0);
		a->numCondRefs = (0);
		a->anyCall = (false);
		a->isLmAction = (false);
		return a;
	}

	static Action *cons( MarkType markType, long markId )
	{
		Action *a = new Action;
		a->name = ("mark");
		a->markType = (markType);
		a->objField = (0);
		a->markId = (markId);
		a->inlineList = (InlineList::cons());
		a->actionId = (-1);
		a->numTransRefs = (0);
		a->numToStateRefs = (0);
		a->numFromStateRefs = (0);
		a->numEofRefs = (0);
		a->numCondRefs = (0);
		a->anyCall = (false);
		a->isLmAction = (false);
		return a;
	}

	/* Key for action dictionary. */
	const String &getKey() const { return name; }

	/* Data collected during parse. */
	InputLoc loc;
	String name;
	
	MarkType markType;
	ObjectField *objField;
	long markId;

	InlineList *inlineList;
	int actionId;

	void actionName( ostream &out )
	{
		if ( name != 0 )
			out << name;
		else
			out << loc.line << ":" << loc.col;
	}

	/* Places in the input text that reference the action. */
	ActionRefs actionRefs;

	/* Number of references in the final machine. */
	bool numRefs() 
		{ return numTransRefs + numToStateRefs + numFromStateRefs + numEofRefs; }
	int numTransRefs;
	int numToStateRefs;
	int numFromStateRefs;
	int numEofRefs;
	int numCondRefs;
	bool anyCall;

	bool isLmAction;
};

/* A list of actions. */
typedef DList<Action> ActionList;

struct VarDef;
struct LexJoin;
struct LexTerm;
struct FactorAug;
struct FactorLabel;
struct FactorRep;
struct FactorNeg;
struct Factor;
struct Literal;
struct Range;
struct RegExpr;
struct ReItem;
struct ReOrBlock;
struct ReOrItem;
struct TokenRegion;

/* tree_t of instantiated names. */
typedef BstMapEl<String, NameInst*> NameMapEl;
typedef BstMap<String, NameInst*, CmpStr> NameMap;
typedef Vector<NameInst*> NameVect;
typedef BstSet<NameInst*> NameSet;

/* Node in the tree of instantiated names. */
struct NameInst
{
	NameInst( int id )
		: id(id) {}

	int id;

	/* Pointers for the name search queue. */
	NameInst *prev, *next;
};

typedef DList<NameInst> NameInstList;

/* Stack frame used in walking the name tree. */
struct NameFrame 
{
	NameInst *prevNameInst;
	int prevNameChild;
	NameInst *prevLocalScope;
};

/* Class to collect information about the machine during the 
 * parse of input. */
struct Compiler
{
	/* Create a new parse data object. This is done at the beginning of every
	 * fsm specification. */
	Compiler();
	~Compiler();

	/*
	 * Setting up the graph dict.
	 */

	void compileLiteralTokens();
	void initEmptyScanners();
	void initEmptyScanner( RegionSet *regionSet, TokenRegion *reg );
	void initUniqueTypes();

	/* Initialize a graph dict with the basic fsms. */
	void initGraphDict();
	void createBuiltin( const char *name, BuiltinMachine builtin );

	/* Make a name id in the current name instantiation scope if it is not
	 * already there. */
	NameInst *makeJoinNameTree( LexJoin *join );
	NameInst *makeNameTree();
	NameInst **makeNameIndex();

	void printNameTree( NameInst *rootName );
	void printNameIndex( NameInst **nameIndex );

	/* Resove name references in action code and epsilon transitions. */
	NameSet resolvePart( NameInst *refFrom, const char *data, bool recLabelsOnly );
	void resolveFrom( NameSet &result, NameInst *refFrom, 
			const NameRef &nameRef, int namePos );

	/* Set the alphabet type. If type types are not valid returns false. */
	bool setAlphType( char *s1, char *s2 );
	bool setAlphType( char *s1 );

	/* Unique actions. */
	void removeDups( ActionTable &actionTable );
	void removeActionDups( FsmGraph *graph );

	/* Dumping the name instantiation tree. */
	void printNameInst( NameInst *nameInst, int level );

	/* Make the graph from a graph dict node. Does minimization. */
	void finishGraphBuild( FsmGraph *graph );
	FsmGraph *makeAllRegions();
	FsmGraph *makeScanner();

	void analyzeAction( Action *action, InlineList *inlineList );
	void analyzeGraph( FsmGraph *graph );
	void resolvePrecedence( PdaGraph *pdaGraph );
	LangEl *predOf( PdaTrans *trans, long action );
	bool precedenceSwap( long action1, long action2, LangEl *l1, LangEl *l2 );
	bool precedenceRemoveBoth( LangEl *l1, LangEl *l2 );

	void placeFrameFields( ObjectDef *localFrame );
	void placeUserFunction( Function *func, bool isUserIter );
	void placeAllStructObjects();
	void placeAllLanguageObjects();
	void placeAllFrameObjects();
	void placeAllFunctions();

	void initKeyOps();

	/*
	 * Data collected during the parse.
	 */

	/* List of actions. Will be pasted into a switch statement. */
	ActionList actionList;

	/* The id of the next priority name and label. */
	int nextPriorKey, nextNameId;

	/* Alphabet type. */
	HostType *userAlphType;
	bool alphTypeSet;

	/* Element type and get key expression. */
	InlineList *getKeyExpr;
	InlineList *accessExpr;
	InlineList *curStateExpr;

	/* The alphabet range. */
	char *lowerNum, *upperNum;
	Key lowKey, highKey;
	InputLoc rangeLowLoc, rangeHighLoc;

	/* Number of errors encountered parsing the fsm spec. */
	int errorCount;

	/* Counting the action and priority ordering. */
	int curActionOrd;
	int curPriorOrd;

	/* Root of the name tree. */
	NameInst *curNameInst;
	int curNameChild;
	NameInstList nameInstList;

	/* The place where resolved epsilon transitions go. These cannot go into
	 * the parse tree because a single epsilon op can resolve more than once
	 * to different nameInsts if the machine it's in is used more than once. */
	NameVect epsilonResolvedLinks;
	int nextEpsilonResolvedLink;

	/* Root of the name tree used for doing local name searches. */
	NameInst *localNameScope;

	void setLmInRetLoc( InlineList *inlineList );
	void initLongestMatchData();

	/* Counter for assigning ids to longest match items. */
	int nextTokenId;

	RegionImplList regionImplList;
	RegionList regionList;
	RegionSetList regionSetList;

	NamespaceList namespaceList;

	Action *newAction( const String &name, InlineList *inlineList );

	Action *setTokStart;
	int setTokStartOrd;

	Action *initActId;
	int initActIdOrd;

	Action *setTokEnd;
	int setTokEndOrd;

	CodeBlock *rootCodeBlock;

	void beginProcessing()
	{
		::keyOps = &thisKeyOps;
	}

	KeyOps thisKeyOps;

	UniqueType *mainReturnUT;

	CharVectVect streamFileNames;

	/* CONTEXT FREE */
	ProdElList *makeProdElList( LangEl *langEl );
	void wrapNonTerminals();
	void makeDefinitionNames();
	void noUndefindLangEls();
	void declareBaseLangEls();
	void makeLangElIds();
	void makeStructElIds();
	void makeLangElNames();
	void makeTerminalWrappers();
	void makeEofElements();
	void makeIgnoreCollectors();
	void resolvePrecedence();
	void resolveReductionActions();
	void findReductionActionProds();
	void resolveReducers();

	void declarePass();
	void resolvePass();

	/* Parser generation. */
	void advanceReductions( PdaGraph *pdaGraph );
	void sortActions( PdaGraph *pdaGraph );
	void addDupTerms( PdaGraph *pdaGraph );
	void linkExpansions( PdaGraph *pdaGraph );
	void lalr1FollowEpsilonOp( PdaGraph *pdaGraph );

	void transferCommits( PdaGraph *pdaGraph, PdaTrans *trans, PdaState *state, long prodId );

	void lalr1AddFollow2( PdaGraph *pdaGraph, PdaTrans *trans, FollowToAdd &followKeys );
	void lalr1AddFollow1( PdaGraph *pdaGraph, PdaState *state );

	void lalr1AddFollow2( PdaGraph *pdaGraph, PdaTrans *trans, long followKey, long prior );
	void lalr1AddFollow1( PdaGraph *pdaGraph, PdaTrans *trans );

	void lalr1AddFollowSets( PdaGraph *pdaGraph, LangElSet &parserEls );

	void lr0BringInItem( PdaGraph *pdaGraph, PdaState *dest, PdaState *prodState, 
			PdaTrans *expandFrom, Production *prod );
	void lr0InvokeClosure( PdaGraph *pdaGraph, PdaState *state );
	void lr0CloseAllStates( PdaGraph *pdaGraph );

	void lalr1GenerateParser( PdaGraph *pdaGraph, LangElSet &parserEls );

	void reduceActions( PdaGraph *pdaGraph );

	bool makeNonTermFirstSetProd( Production *prod, PdaState *state );
	void makeNonTermFirstSets();

	bool makeFirstSetProd( Production *prod, PdaState *state );
	void makeFirstSets();

	int findIndexOff( struct pda_tables *pdaTables, PdaGraph *pdaGraph,
			PdaState *state, int &currLen );
	void trySetTime( PdaTrans *trans, long code, long &time );
	void addRegion( PdaState *tabState, PdaTrans *pdaTrans, long pdaKey,
			bool noPreIgnore, bool noPostIgnore );
	PdaState *followProd( PdaState *tabState, PdaState *prodState );
	void findFollow( AlphSet &result, PdaState *overTab, 
			PdaState *overSrc, Production *parentDef );
	void pdaActionOrder( PdaGraph *pdaGraph, LangElSet &parserEls );
	void pdaOrderFollow( LangEl *rootEl, PdaState *tabState, 
			PdaTrans *tabTrans, PdaTrans *srcTrans,
			Production *parentDef, Production *definition, long &time );
	void pdaOrderProd( LangEl *rootEl, PdaState *tabState, 
			PdaState *srcState, Production *parentDef, long &time );
	void analyzeMachine( PdaGraph *pdaGraph, LangElSet &parserEls );

	void makeProdFsms();
	void insertUniqueEmptyProductions();
	void printNonTermFirstSets();
	void printFirstSets();

	LangEl *makeRepeatProd( const InputLoc &loc, Namespace *nspace,
			const String &repeatName, UniqueType *ut );
	LangEl *makeListProd( const InputLoc &loc, Namespace *nspace,
			const String &listName, UniqueType *ut );
	LangEl *makeOptProd( const InputLoc &loc, Namespace *nspace,
			const String &optName, UniqueType *ut );
	void resolveProdEl( ProdEl *prodEl );
	void resolveProductionEls();

	void addMatchText( ObjectDef *frame, LangEl *lel );
	void addMatchLength( ObjectDef *frame, LangEl *lel );
	void addInput( ObjectDef *frame );
	void addThis( ObjectDef *frame );
	void addTransTokVar( ObjectDef *frame, LangEl *lel );
	void addProdRHSVars( ObjectDef *localFrame, ProdElList *prodElList );
	void addProdRedObjectVar( ObjectDef *localFrame, LangEl *langEl );
	void addProdObjects();

	void addProdRHSLoads( Production *prod, CodeVect &code, long &insertPos );
	void addProdLHSLoad( Production *prod, CodeVect &code, long &insertPos );
	void addPushBackLHS( Production *prod, CodeVect &code, long &insertPos );

	void prepGrammar();
	struct pda_run *parsePattern( program_t *prg, tree_t **sp, const InputLoc &loc,
			int parserId, struct stream_impl *sourceStream );
	void parsePatterns();

	void collectParserEls( LangElSet &parserEls );
	void makeParser( LangElSet &parserEls );
	PdaGraph *makePdaGraph( BstSet<LangEl*> &parserEls  );
	struct pda_tables *makePdaTables( PdaGraph *pdaGraph );

	void fillInPatterns( program_t *prg );
	void makeRuntimeData();

	/* Generate and write out the fsm. */
	void generateGraphviz();

	void verifyParseStopGrammar( LangEl *langEl, PdaGraph *pdaGraph );
	void computeAdvanceReductions( LangEl *langEl, PdaGraph *pdaGraph );

	void initListElField( GenericType *gen, const char *name, int offset );
	void initListFieldEl( GenericType *gen, const char *name, int offset );
	void initListFieldVal( GenericType *gen, const char *name, int offset );

	void initListFields( GenericType *gen );
	void initListFunctions( GenericType *gen );

	void initMapElKey( GenericType *gen, const char *name, int offset );
	void initMapElField( GenericType *gen, const char *name, int offset );
	void initMapField( GenericType *gen, const char *name, int offset );

	void initMapFields( GenericType *gen );
	void initMapFunctions( GenericType *gen );

	void initVectorFunctions( GenericType *gen );
	void initParserField( GenericType *gen, const char *name,
			int offset, TypeRef *typeRef );
	void initParserFunctions( GenericType *gen );
	void initParserFields( GenericType *gen );

	void addStdin();
	void addStdout();
	void addStderr();
	void addArgv();
	void addError();
	int argvOffset();
	int arg0Offset();
	void makeDefaultIterators();
	void addLengthField( ObjectDef *objDef, code_t getLength );
	ObjectDef *findObject( const String &name );
	void resolveListElementOf( ObjectDef *container, ObjectDef *obj, ElementOf *elof );
	void resolveMapElementOf( ObjectDef *container, ObjectDef *obj, ElementOf *elof );
	void resolveElementOf( ObjectDef *obj );
	void makeFuncVisible( Function *func, bool isUserIter );
	void makeInHostVisible( Function *func );

	void declareFunction( Function *func );
	void declareReductionCode( Production *prod );
	void declareTranslateBlock( LangEl *langEl );
	void declarePreEof( TokenRegion *region );
	void declareRootBlock();
	void declareByteCode();

	void resolveFunction( Function *func );
	void resolveInHost( Function *func );
	void resolvePreEof( TokenRegion *region );
	void resolveRootBlock();
	void resolveTranslateBlock( LangEl *langEl );
	void resolveReductionCode( Production *prod );
	void resolveParseTree();

	void compileFunction( Function *func, CodeVect &code );
	void compileFunction( Function *func );
	void compileUserIter( Function *func, CodeVect &code );
	void compileUserIter( Function *func );
	void compilePreEof( TokenRegion *region );
	void compileRootBlock();
	void compileTranslateBlock( LangEl *langEl );
	void findLocals( ObjectDef *localFrame, CodeBlock *block );
	void makeProdCopies( Production *prod );
	void compileReductionCode( Production *prod );
	void removeNonUnparsableRepls();
	void compileByteCode();

	void resolveUses();
	void generateOutput( long activeRealm, bool includeCommit );
	void compile();

	void openNameSpace( ostream &out, Namespace *nspace );
	void closeNameSpace( ostream &out, Namespace *nspace );
	void refNameSpace( LangEl *lel, Namespace *nspace );
	void generateExports();
	void generateExportsImpl();

	struct local_info *makeLocalInfo( Locals &locals );
	short *makeTrees( ObjectDef *objectDef, int &numTrees );

	/* 
	 * Graphviz Generation
	 */
	void writeTransList( PdaState *state );
	void writeDotFile( PdaGraph *graph );
	void writeDotFile( );
	

	/*
	 * Data collected during the parse.
	 */

	LelList langEls;
	StructElList structEls;
	DefList prodList;

	/* Dumping. */
	DotItemIndex dotItemIndex;

	PredDeclList predDeclList;

	/* The name of the file the fsm is from, and the spec name. */
	// EXISTS IN RL: char *fileName; 
	String parserName;
	// EXISTS IN RL: InputLoc sectionLoc;

	/* How to access the instance data. */
	String access;

	/* The name of the token structure. */
	String tokenStruct;

	GenericType *anyList;
	GenericType *anyMap;
	GenericType *anyVector;

	LangEl *ptrLangEl;
	LangEl *voidLangEl;
	LangEl *strLangEl;
	LangEl *anyLangEl;
	LangEl *rootLangEl;
	LangEl *noTokenLangEl;
	LangEl *eofLangEl;
	LangEl *errorLangEl;
	LangEl *ignoreLangEl;

	Namespace *rootNamespace;

	int nextLelId;
	int firstNonTermId;

	LangEl **langElIndex;
	PdaState *actionDestState;
	DefSetSet prodSetSet;

	Production **prodIdIndex;
	AlphSet literalSet;

	PatList patternList;
	ConsList replList;
	ParserTextList parserTextList;

	StructDef *global;
	StructEl *globalSel;
	ObjectDef *globalObjectDef;
	ObjectField *arg0;
	ObjectField *argv;
	StructDef *argvEl;
	StructEl *argvElSel;

	StructDef *stream;
	StructEl *streamSel;

	VectorTypeIdMap vectorTypeIdMap;

	UniqueType *findUniqueType( enum TYPE typeId );
	UniqueType *findUniqueType( enum TYPE typeId, LangEl *langEl );
	UniqueType *findUniqueType( enum TYPE typeId, IterDef *iterDef );
	UniqueType *findUniqueType( enum TYPE typeId, StructEl *structEl );
	UniqueType *findUniqueType( enum TYPE typeId, GenericType *generic );

	UniqueGeneric *findUniqueGeneric( UniqueGeneric::Type type,
			UniqueType *utKey, UniqueType *utValue );
	UniqueGeneric *findUniqueGeneric( UniqueGeneric::Type type,
			UniqueType *utValue );

	UniqueType *uniqueTypeNil;
	UniqueType *uniqueTypeVoid;
	UniqueType *uniqueTypePtr;
	UniqueType *uniqueTypeBool;
	UniqueType *uniqueTypeInt;
	UniqueType *uniqueTypeStr;
	UniqueType *uniqueTypeIgnore;
	UniqueType *uniqueTypeAny;

	UniqueType *uniqueTypeStream;

	UniqueTypeMap uniqeTypeMap;
	UniqueRepeatMap uniqeRepeatMap;
	UniqueGenericMap uniqueGenericMap;

	void declareGlobalFields();
	void declareStrFields();

	void declareStreamField( ObjectDef *objDef, code_t getLength );
	void declareStreamFields();

	void declareIntFields();
	void declareTokenFields();

	ObjectDef *intObj;
	ObjectDef *strObj;
	ObjectDef *streamObj;

	struct fsm_tables *fsmTables;
	struct colm_sections *runtimeData;

	int nextPatConsId;
	int nextGenericId;

	FunctionList functionList;
	FunctionList inHostList;
	int nextFuncId;
	int nextHostId;

	enum CompileContext {
		CompileTranslation,
		CompileReduction,
		CompileFunction,
		CompileRoot
	};

	CompileContext compileContext;
	LongVect returnJumps;
	LongVect breakJumps;
	Function *curFunction;

	/* For stack unwinding. Used at exits, returns, iterator destroy, etc. */
	CodeVect unwindCode;

	ObjectField *makeDataEl();
	ObjectField *makePosEl();
	ObjectField *makeLineEl();

	IterDef *findIterDef( IterDef::Type type, GenericType *generic );
	IterDef *findIterDef( IterDef::Type type, Function *func );
	IterDef *findIterDef( IterDef::Type type );
	IterDefSet iterDefSet;

	enum GeneratesType { GenToken, GenIgnore, GenCfl };

	int nextObjectId;
	GeneratesType generatesType;
	bool generatesIgnore;

	StringMap literalStrings;

	long nextFrameId;
	long nextParserId;

	ObjectDef *rootLocalFrame;

	bool revertOn;

	RedFsm *redFsm;

	PdaGraph *pdaGraph;
	struct pda_tables *pdaTables;

	long predValue;
	long nextMatchEndNum;

	TypeRef *argvTypeRef;

	bool inContiguous;
	int contiguousOffset;
	int contiguousStretch;

	void declareReVars();

	void initReductionNeeds( Reduction *reduction );
	void loadRefs( Reduction *reduction, Production *production,
			const ReduceTextItemList &list );

	void writeHostCall();
	void writeNeeds();
	void writeCommit();
	void writeLhsRef( Production *production, ReduceTextItem *i );
	void writeRhsRef( Production *production, ReduceTextItem *i );
	void writeRhsLoc( Production *production, ReduceTextItem *i );
	void writeHostItemList( Production *production, const ReduceTextItemList &list );
	void writeCommitStub();
};

void afterOpMinimize( FsmGraph *fsm, bool lastInSeq = true );
Key makeFsmKeyHex( char *str, const InputLoc &loc, Compiler *pd );
Key makeFsmKeyDec( char *str, const InputLoc &loc, Compiler *pd );
Key makeFsmKeyNum( char *str, const InputLoc &loc, Compiler *pd );
Key makeFsmKeyChar( char c, Compiler *pd );
void makeFsmKeyArray( Key *result, char *data, int len, Compiler *pd );
void makeFsmUniqueKeyArray( KeySet &result, char *data, int len, 
		bool caseInsensitive, Compiler *pd );
FsmGraph *makeBuiltin( BuiltinMachine builtin, Compiler *pd );
FsmGraph *dotFsm( Compiler *pd );
FsmGraph *dotStarFsm( Compiler *pd );

void errorStateLabels( const NameSet &locations );

struct ColmParser;

typedef AvlMap<String, ColmParser *, CmpStr> ParserDict;
typedef AvlMapEl<String, ColmParser *> ParserDictEl;

LangEl *declareLangEl( Compiler *pd, Namespace *nspace,
		const String &data, LangEl::Type type );
LangEl *addLangEl( Compiler *pd, Namespace *nspace,
		const String &data, LangEl::Type type );

StructEl *declareStruct( Compiler *pd, Namespace *nspace,
		const String &data, StructDef *context );

void declareTypeAlias( Compiler *pd, Namespace *nspace,
		const String &data, TypeRef *typeRef );

LangEl *findType( Compiler *pd, Namespace *nspace, const String &data );

ObjectMethod *initFunction( UniqueType *retType, ObjectDef *obj, 
		const String &name, int methIdWV, int methIdWC,
		bool isConst, bool useFnInstr = false, GenericType *useGeneric = 0 );

ObjectMethod *initFunction( UniqueType *retType, ObjectDef *obj, 
		const String &name, int methIdWV, int methIdWC,
		UniqueType *arg1, bool isConst, bool useFnInstr = false,
		GenericType *useGeneric = 0 );

ObjectMethod *initFunction( UniqueType *retType, ObjectDef *obj, 
		const String &name, int methIdWV, int methIdWC, 
		UniqueType *arg1, UniqueType *arg2, bool isConst,
		bool useFnInstr = false, GenericType *useGeneric = 0 );

ObjectMethod *initFunction( UniqueType *retType, Namespace *nspace, ObjectDef *obj, 
		const String &name, int methIdWV, int methIdWC,
		bool isConst, bool useFnInstr = false, GenericType *useGeneric = 0 );

ObjectMethod *initFunction( UniqueType *retType, Namespace *nspace, ObjectDef *obj, 
		const String &name, int methIdWV, int methIdWC,
		UniqueType *arg1, bool isConst, bool useFnInstr = false,
		GenericType *useGeneric = 0 );

ObjectMethod *initFunction( UniqueType *retType, Namespace *nspace, ObjectDef *obj, 
		const String &name, int methIdWV, int methIdWC, 
		UniqueType *arg1, UniqueType *arg2, bool isConst,
		bool useFnInstr = false, GenericType *useGeneric = 0 );


#endif /* _PARSEDATA_H */
