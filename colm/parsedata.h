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
#include "astring.h"
#include "pdagraph.h"
#include "compare.h"
#include "pdarun.h"
#include "bytecode.h"
#include "program.h"

using std::ostream;

struct exit_object { };
extern exit_object endp;
void operator<<( std::ostream &out, exit_object & );

/* Forwards. */
struct RedFsm;
struct LangEl;
struct ParseData;
struct PdaCodeGen;
struct FsmCodeGen;

#define SHIFT_CODE 0x1
#define REDUCE_CODE 0x2
#define SHIFT_REDUCE_CODE 0x3

inline long makeReduceCode( long reduction, bool isShiftReduce )
{
	return ( isShiftReduce ? SHIFT_REDUCE_CODE : REDUCE_CODE ) | 
		( reduction << 2 );
}

struct ProdEl;
struct ProdElList;
struct PdaLiteral;
struct Definition;

/* A pointer to this is in PdaRun, but it's specification is not known by the
 * runtime code. The runtime functions that access it are defined in
 * ctinput.cpp and stubbed in fsmcodegen.cpp */
struct Bindings
	: public Vector<ParseTree*>
{};

struct DefListEl { Definition *prev, *next; };
struct LelDefListEl { Definition *prev, *next; };
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
	PredDecl( TypeRef *typeRef, PredType predType, long predValue )
		: typeRef(typeRef), predType(predType), predValue(predValue)
	{}

	TypeRef *typeRef;
	PredType predType;
	long predValue;

	PredDecl *prev, *next;
};

typedef DList<PredDecl> PredDeclList;

/* Graph dictionary. */
struct Definition 
:
	public DefListEl, public LelDefListEl
{
	enum Type { Production };

	Definition( const InputLoc &loc, LangEl *prodName, ProdElList *prodElList, 
			bool prodCommit, CodeBlock *redBlock, int prodId, int prodNum, Type type ) : 
		loc(loc), prodName(prodName), prodElList(prodElList), 
		prodCommit(prodCommit), redBlock(redBlock), prodId(prodId), prodNum(prodNum),
		type(type), fsm(0), fsmLength(0), uniqueEmptyLeader(0), 
		isLeftRec(false), localFrame(0), lhsField(0), predOf(0) {}

	InputLoc loc;
	LangEl *prodName;
	ProdElList *prodElList;
	bool prodCommit;

	CodeBlock *redBlock;

	int prodId;
	int prodNum;
	Type type;

	PdaGraph *fsm;
	int fsmLength;
	String data;
	LongSet reducesTo;

	LangEl *uniqueEmptyLeader;

	ProdIdSet nonTermFirstSet;
	AlphSet firstSet;
	bool isLeftRec;

	ObjectDef *localFrame;
	ObjField *lhsField;

	LangEl *predOf;

	UnsignedCharVect copy;
};

struct CmpDefById
{
	static int compare( Definition *d1, Definition *d2 )
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
typedef BstMap< int, Definition*, CmpOrd<int> > DotItemIndex;
typedef BstMapEl< int, Definition*> DotItemIndexEl;

struct DefList
:
	public DListMel<Definition, DefListEl>
{};

/* A vector of production vectors. Each non terminal can have many productions. */
struct LelDefList
:
	public DListMel<Definition, LelDefListEl> 
{};

/* A set of machines made during a closure round. */
typedef Vector< PdaGraph* > Machines;

/* List of language elements. */
typedef DList<LangEl> LelList;

typedef Vector< TokenDef* > TokenDefVect;

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
	bool isUserTerm;
	bool isContext;
	String displayString;
	long numAppearances;
	bool commit;
	bool ignore;
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
	Definition *rootDef;
	LangEl *termDup;
	LangEl *eofLel;

	PdaGraph *pdaGraph;
	PdaTables *pdaTables;

	PdaState *startState;

	CodeBlock *transBlock;

	ObjectDef *objectDef;
	NamespaceQual *objectDefUsesQual;
	String objectDefUses;

	long thisSize;
	long ofiOffset;

	GenericType *generic;

	long parserId;

	PredType predType;
	long predValue;

	Context *contextDef;
	Context *contextIn;
	bool preNoIgnore;
	bool postNoIgnore;
};

struct ProdEl
{
	/* Language elements a factor node can be. */
	enum Type {
		LiteralType, 
		ReferenceType
	}; 

	/* Construct with a reference to a var def. */
	ProdEl( Type type, const InputLoc &loc, ObjField *captureField, bool commit, TypeRef *typeRef, int priorVal )
	:
		captureField(captureField),
		commit(commit),
		typeRef(typeRef),
		langEl(0),
		priorVal(priorVal),
		type(type), 
		objField(0) {}

	ProdEl( const InputLoc &loc, TypeRef *typeRef )
	:
		captureField(0), 
		commit(false), 
		typeRef(typeRef), 
		langEl(0), 
		priorVal(0), 
		type(ReferenceType), objField(0) {}

	ObjField *captureField;
	bool commit;

	TypeRef *typeRef;

	LangEl *langEl;
	int priorVal;
	Type type;
	ObjField *objField;
	ProdEl *prev, *next;
};

struct ProdElList : public DList<ProdEl>
{
	PdaGraph *walk( ParseData *pd );
};

/* This should be renamed. It is a literal string in a type reference. */
struct PdaLiteral
{
	PdaLiteral( const InputLoc &loc, const Token &token )
		: loc(loc), token(token), value(0) { }

	InputLoc loc;
	Token token;
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

	Action( const InputLoc &loc, const String &name, InlineList *inlineList )
	:
		loc(loc),
		name(name),
		markType(MarkNone),
		objField(0),
		markId(-1),
		inlineList(inlineList), 
		actionId(-1),
		numTransRefs(0),
		numToStateRefs(0),
		numFromStateRefs(0),
		numEofRefs(0),
		numCondRefs(0),
		anyCall(false),
		isLmAction(false)
	{
	}

	Action( MarkType markType, long markId )
	:
		name("mark"),
		markType(markType),
		objField(0),
		markId(markId),
		inlineList(new InlineList), 
		actionId(-1),
		numTransRefs(0),
		numToStateRefs(0),
		numFromStateRefs(0),
		numEofRefs(0),
		numCondRefs(0),
		anyCall(false),
		isLmAction(false)
	{
	}

	/* Key for action dictionary. */
	const String &getKey() const { return name; }

	/* Data collected during parse. */
	InputLoc loc;
	String name;
	
	MarkType markType;
	ObjField *objField;
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
typedef AvlTree<Action, String, CmpStr> ActionDict;

struct VarDef;
struct Join;
struct Expression;
struct Term;
struct FactorWithAug;
struct FactorWithLabel;
struct FactorWithRep;
struct FactorWithNeg;
struct Factor;
struct Literal;
struct Range;
struct RegExpr;
struct ReItem;
struct ReOrBlock;
struct ReOrItem;
struct TokenRegion;

/* Priority name dictionary. */
typedef AvlMapEl<String, int> PriorDictEl;
typedef AvlMap<String, int, CmpStr> PriorDict;

/* Local error name dictionary. */
typedef AvlMapEl<String, int> LocalErrDictEl;
typedef AvlMap<String, int, CmpStr> LocalErrDict;

/* Tree of instantiated names. */
typedef BstMapEl<String, NameInst*> NameMapEl;
typedef BstMap<String, NameInst*, CmpStr> NameMap;
typedef Vector<NameInst*> NameVect;
typedef BstSet<NameInst*> NameSet;

/* Node in the tree of instantiated names. */
struct NameInst
{
	NameInst( const InputLoc &loc, NameInst *parent, const String &name, 
				int id, bool isLabel ) : 
		loc(loc), parent(parent), name(name), id(id), isLabel(isLabel),
		isLongestMatch(false), numRefs(0), numUses(0), start(0), final(0) {}

	InputLoc loc;

	/* Keep parent pointers in the name tree to retrieve 
	 * fully qulified names. */
	NameInst *parent;

	String name;
	int id;
	bool isLabel;
	bool isLongestMatch;

	int numRefs;
	int numUses;

	/* Names underneath us, excludes anonymous names. */
	NameMap children;

	/* All names underneath us in order of appearance. */
	NameVect childVect;

	/* Join scopes need an implicit "final" target. */
	NameInst *start, *final;

	/* During a fsm generation walk, lists the names that are referenced by
	 * epsilon operations in the current scope. After the link is made by the
	 * epsilon reference and the join operation is complete, the label can
	 * have its refcount decremented. Once there are no more references the
	 * entry point can be removed from the fsm returned. */
	NameVect referencedNames;

	/* Pointers for the name search queue. */
	NameInst *prev, *next;

	/* Check if this name inst or any name inst below is referenced. */
	bool anyRefsRec();
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
struct ParseData
{
	/* Create a new parse data object. This is done at the beginning of every
	 * fsm specification. */
	ParseData( const String &fileName, const String &sectionName, 
			const InputLoc &sectionLoc, ostream &out );
	~ParseData();

	/*
	 * Setting up the graph dict.
	 */

	void compileLiteralTokens();
	void initEmptyScanners();
	void initUniqueTypes();

	/* Initialize a graph dict with the basic fsms. */
	void initGraphDict();
	void createBuiltin( const char *name, BuiltinMachine builtin );

	/* Make a name id in the current name instantiation scope if it is not
	 * already there. */
	NameInst *addNameInst( const InputLoc &loc, char *data, bool isLabel );
	NameInst *makeJoinNameTree( Join *join );
	NameInst *makeNameTree( );
	void fillNameIndex( NameInst **nameIndex, NameInst *from );
	NameInst **makeNameIndex( NameInst *rootName );


	void printNameTree( NameInst *rootName );
	void printNameIndex( NameInst **nameIndex );

	/* Increments the usage count on entry names. Names that are no longer
	 * needed will have their entry points unset. */
	void unsetObsoleteEntries( FsmGraph *graph );

	/* Resove name references in action code and epsilon transitions. */
	NameSet resolvePart( NameInst *refFrom, const char *data, bool recLabelsOnly );
	void resolveFrom( NameSet &result, NameInst *refFrom, 
			const NameRef &nameRef, int namePos );
	void referenceRegions( NameInst *root );

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
	FsmGraph *makeJoin( Join *join );
	FsmGraph *makeAllRegions();
	FsmGraph *makeFsmGraph( Join *join );
	FsmGraph *makeScanner() 
		{ return makeFsmGraph(0); }

	void analyzeAction( Action *action, InlineList *inlineList );
	void analyzeGraph( FsmGraph *graph );
	void resolvePrecedence( PdaGraph *pdaGraph );
	LangEl *predOf( PdaTrans *trans, long action );
	bool precedenceSwap( long action1, long action2, LangEl *l1, LangEl *l2 );
	bool precedenceRemoveBoth( LangEl *l1, LangEl *l2 );

	void initKeyOps();

	/*
	 * Data collected during the parse.
	 */

	/* The list of instances. */
	GraphList instanceList;

	/* Dictionary of actions. Lets actions be defined and then referenced. */
	ActionDict actionDict;

	/* Dictionary of named priorities. */
	PriorDict priorDict;

	/* Dictionary of named local errors. */
	LocalErrDict localErrDict;

	/* List of actions. Will be pasted into a switch statement. */
	ActionList actionList;

	/* The id of the next priority name and label. */
	int nextPriorKey, nextLocalErrKey, nextNameId;
	
	/* The default priority number key for a machine. This is active during
	 * the parse of the rhs of a machine assignment. */
	int curDefPriorKey;

	int curDefLocalErrKey;

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

	/* The name of the file the fsm is from, and the spec name. */
	String fileName;
	String sectionName;
	InputLoc sectionLoc;

	/* Number of errors encountered parsing the fsm spec. */
	int errorCount;

	/* Counting the action and priority ordering. */
	int curActionOrd;
	int curPriorOrd;

	/* Root of the name tree. */
	NameInst *curNameInst;
	int curNameChild;

	/* The place where resolved epsilon transitions go. These cannot go into
	 * the parse tree because a single epsilon op can resolve more than once
	 * to different nameInsts if the machine it's in is used more than once. */
	NameVect epsilonResolvedLinks;
	int nextEpsilonResolvedLink;

	/* Root of the name tree used for doing local name searches. */
	NameInst *localNameScope;

	void setLmInRetLoc( InlineList *inlineList );
	void initLongestMatchData();
	void initNameWalk( NameInst *rootName );
	NameInst *nextNameScope() { return curNameInst->childVect[curNameChild]; }
	NameFrame enterNameScope( bool isLocal, int numScopes );
	void popNameScope( const NameFrame &frame );
	void resetNameScope( const NameFrame &frame );

	/* Counter for assigning ids to longest match items. */
	int nextTokenId;

	/* List of all longest match parse tree items. */
	RegionList regionList;

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
		::condData = &thisCondData;
		::keyOps = &thisKeyOps;
	}

	CondData thisCondData;
	KeyOps thisKeyOps;

	UniqueType *mainReturnUT;

	/* CONTEXT FREE */
	ProdElList *makeProdElList( LangEl *langEl );
	void wrapNonTerminals();
	void makeDefinitionNames();
	void noUndefindLangEls();
	void declareBaseLangEls();
	void makeLangElIds();
	void makeLangElNames();
	void makeTerminalWrappers();
	void makeEofElements();
	void setPrecedence();

	void typeDeclaration();
	void typeResolve();

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
			PdaTrans *expandFrom, Definition *prod );
	void lr0InvokeClosure( PdaGraph *pdaGraph, PdaState *state );
	void lr0CloseAllStates( PdaGraph *pdaGraph );

	void lalr1GenerateParser( PdaGraph *pdaGraph, LangElSet &parserEls );

	void reduceActions( PdaGraph *pdaGraph );

	bool makeNonTermFirstSetProd( Definition *prod, PdaState *state );
	void makeNonTermFirstSets();

	bool makeFirstSetProd( Definition *prod, PdaState *state );
	void makeFirstSets();

	int findIndexOff( PdaTables *pdaTables, PdaGraph *pdaGraph, PdaState *state, int &currLen );
	void trySetTime( PdaTrans *trans, long code, long &time );
	void addRegion( PdaState *tabState, PdaTrans *pdaTrans, long pdaKey );
	PdaState *followProd( PdaState *tabState, PdaState *prodState );
	void findFollow( AlphSet &result, PdaState *overTab, 
			PdaState *overSrc, Definition *parentDef );
	void pdaActionOrder( PdaGraph *pdaGraph, LangElSet &parserEls );
	void pdaOrderFollow( LangEl *rootEl, PdaState *tabState, 
			PdaTrans *tabTrans, PdaTrans *srcTrans,
			Definition *parentDef, Definition *definition, long &time );
	void pdaOrderProd( LangEl *rootEl, PdaState *tabState, 
			PdaState *srcState, Definition *parentDef, long &time );
	void analyzeMachine( PdaGraph *pdaGraph, LangElSet &parserEls );

	void makeProdFsms();
	void insertUniqueEmptyProductions();
	void printNonTermFirstSets();
	void printFirstSets();

	LangEl *makeRepeatProd( Namespace *nspace, const String &repeatName, 
			NamespaceQual *nspaceQual, const String &name );
	LangEl *makeListProd( Namespace *nspace, const String &listName,
			NamespaceQual *nspaceQual, const String &name );
	LangEl *makeOptProd( Namespace *nspace, const String &optName,
			NamespaceQual *nspaceQual, const String &name );
	void resolveFactor( ProdEl *fact );
	void resolveProductionEls();
	void resolvePatternEls();
	void resolveReplacementEls();
	void resolveParserEls();

	void addMatchText( ObjectDef *frame, LangEl *lel );
	void addMatchLength( ObjectDef *frame, LangEl *lel );
	void addInput( ObjectDef *frame );
	void addCtx( ObjectDef *frame );
	void addTransTokVar( ObjectDef *frame, LangEl *lel );
	void addProdRHSVars( ObjectDef *localFrame, ProdElList *prodElList );
	void addProdRedObjectVar( ObjectDef *localFrame, LangEl *langEl );
	void addProdObjects();

	void addProdRHSLoads( Definition *prod, CodeVect &code, long &insertPos );
	void addProdLHSLoad( Definition *prod, CodeVect &code, long &insertPos );
	void addPushBackLHS( Definition *prod, CodeVect &code, long &insertPos );

	void prepGrammar();
	void parsePatterns();

	void collectParserEls( LangElSet &parserEls );
	void makeParser( LangElSet &parserEls );
	PdaGraph *makePdaGraph( BstSet<LangEl*> &parserEls  );
	PdaTables *makePdaTables( PdaGraph *pdaGraph );

	void fillInPatterns( Program *prg );
	void makeRuntimeData();

	/* Generate and write out the fsm. */
	void generateGraphviz();

	void verifyParseStopGrammar( LangEl *langEl, PdaGraph *pdaGraph );
	void computeAdvanceReductions( LangEl *langEl, PdaGraph *pdaGraph );

	void initFieldInstructions( ObjField *el );
	void initLocalInstructions( ObjField *el );
	void initLocalRefInstructions( ObjField *el );

	void initMapFunctions( GenericType *gen );
	void initListField( GenericType *gen, const char *name, int offset );
	void initListFields( GenericType *gen );
	void initListFunctions( GenericType *gen );
	void initVectorFunctions( GenericType *gen );
	void initParserFunctions( GenericType *gen );
	void initParserFields( GenericType *gen );
	void initCtxField( GenericType *gen );

	void addStdin();
	void addStdout();
	void addStderr();
	void addArgv();
	int argvOffset();
	void initGlobalFunctions();
	void makeDefaultIterators();
	void addLengthField( ObjectDef *objDef, Code getLength );
	ObjectDef *findObject( const String &name );
	void initAllLanguageObjects();
	void resolveListElementOf( ObjectDef *container, ObjectDef *obj, ElementOf *elof );
	void resolveMapElementOf( ObjectDef *container, ObjectDef *obj, ElementOf *elof );
	void resolveElementOf( ObjectDef *obj );
	void makeFuncVisible( Function *func, bool isUserIter );

	void resolveFunction( Function *func );
	void resolveUserIter( Function *func );
	void resolvePreEof( TokenRegion *region );
	void resolveRootBlock();
	void resolveTranslateBlock( LangEl *langEl );
	void resolveReductionCode( Definition *prod );
	void resolveParseTree();
	void resolveGenericTypes();

	void compileFunction( Function *func, CodeVect &code );
	void compileFunction( Function *func );
	void compileUserIter( Function *func, CodeVect &code );
	void compileUserIter( Function *func );
	void compilePreEof( TokenRegion *region );
	void compileRootBlock();
	void compileTranslateBlock( LangEl *langEl );
	void findLocalTrees( CharSet &trees );
	void makeProdCopies( Definition *prod );
	void compileReductionCode( Definition *prod );
	void initGenericTypes();
	void removeNonUnparsableRepls();
	void compileByteCode();

	void resolveUses();
	void createDefaultScanner();
	void semanticAnalysis();
	void generateOutput();

	void openNameSpace( ostream &out, Namespace *nspace );
	void closeNameSpace( ostream &out, Namespace *nspace );
	void refNameSpace( LangEl *lel, Namespace *nspace );
	void generateExports();
	void generateExportsImpl();

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
	DefList prodList;

	/* Dumping. */
	DotItemIndex dotItemIndex;

	PredDeclList predDeclList;

	/* The name of the file the fsm is from, and the spec name. */
	// EXISTS IN RL: char *fileName; 
	String parserName;
	ostream &out;
	// EXISTS IN RL: InputLoc sectionLoc;

	/* How to access the instance data. */
	String access;

	/* The name of the token structure. */
	String tokenStruct;

	GenericType *anyList;
	GenericType *anyMap;
	GenericType *anyVector;

	LangEl *ptrLangEl;
	LangEl *boolLangEl;
	LangEl *intLangEl;
	LangEl *strLangEl;
	LangEl *streamLangEl;
	LangEl *inputLangEl;
	LangEl *anyLangEl;
	LangEl *rootLangEl;
	LangEl *noTokenLangEl;
	LangEl *eofLangEl;
	LangEl *errorLangEl;
	LangEl *defaultCharLangEl;
	LangEl *ignoreLangEl;

	TokenRegion *rootRegion;
	TokenRegion *defaultRegion;
	TokenRegion *eofTokenRegion;

	Namespace *defaultNamespace;
	Namespace *rootNamespace;

	int nextSymbolId;
	int firstNonTermId;

	LangEl **langElIndex;
	PdaState *actionDestState;
	DefSetSet prodSetSet;

	Definition **prodIdIndex;
	AlphSet literalSet;

	PatternList patternList;
	ReplList replList;
	ParserTextList parserTextList;

	ObjectDef *globalObjectDef;

	VectorTypeIdMap vectorTypeIdMap;
	ObjectDef *curLocalFrame;

	UniqueType *findUniqueType( int typeId );
	UniqueType *findUniqueType( int typeId, LangEl *langEl );
	UniqueType *findUniqueType( int typeId, IterDef *iterDef );

	UniqueType *uniqueTypeNil;
	UniqueType *uniqueTypePtr;
	UniqueType *uniqueTypeBool;
	UniqueType *uniqueTypeInt;
	UniqueType *uniqueTypeStr;
	UniqueType *uniqueTypeStream;
	UniqueType *uniqueTypeInput;
	UniqueType *uniqueTypeIgnore;
	UniqueType *uniqueTypeAny;

	UniqueTypeMap uniqeTypeMap;
	UniqueRepeatMap uniqeRepeatMap;
	UniqueMapMap uniqueMapMap;
	UniqueListMap uniqueListMap;
	UniqueVectorMap uniqueVectorMap;
	UniqueParserMap uniqueParserMap;

	void initStrObject();
	void initStreamObject();
	void initInputObject();
	void initIntObject();
	void initTokenObjects();

	ObjectDef *intObj;
	ObjectDef *strObj;
	ObjectDef *streamObj;
	ObjectDef *inputObj;
	ObjectDef *tokenObj;

	FsmTables *fsmTables;
	RuntimeData *runtimeData;

	int nextPatReplId;
	int nextGenericId;

	FunctionList functionList;
	int nextFuncId;

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

	/* Loops fill this in for return statements to use. */
	CodeVect *loopCleanup;

	ObjField *makeDataEl();
	ObjField *makePosEl();
	ObjField *makeLineEl();

	IterDef *findIterDef( IterDef::Type type, GenericType *generic );
	IterDef *findIterDef( IterDef::Type type, Function *func );
	IterDef *findIterDef( IterDef::Type type );
	IterDefSet iterDefSet;

	enum GeneratesType { GenToken, GenIgnore, GenCfl };

	int nextObjectId;
	GeneratesType generatesType;
	bool generatesIgnore;
	bool insideRegion;

	StringMap literalStrings;

	long nextFrameId;
	long nextParserId;

	ObjectDef *rootLocalFrame;

	long nextLabelId;
	ObjectDef *objectDef;

	bool revertOn;

	RedFsm *redFsm;

	PdaGraph *pdaGraph;
	PdaTables *pdaTables;

	long predValue;
	long nextMatchEndNum;

	TypeRef *argvTypeRef;

	Context *context;
};

void afterOpMinimize( FsmGraph *fsm, bool lastInSeq = true );
Key makeFsmKeyHex( char *str, const InputLoc &loc, ParseData *pd );
Key makeFsmKeyDec( char *str, const InputLoc &loc, ParseData *pd );
Key makeFsmKeyNum( char *str, const InputLoc &loc, ParseData *pd );
Key makeFsmKeyChar( char c, ParseData *pd );
void makeFsmKeyArray( Key *result, char *data, int len, ParseData *pd );
void makeFsmUniqueKeyArray( KeySet &result, char *data, int len, 
		bool caseInsensitive, ParseData *pd );
FsmGraph *makeBuiltin( BuiltinMachine builtin, ParseData *pd );
FsmGraph *dotFsm( ParseData *pd );
FsmGraph *dotStarFsm( ParseData *pd );

void errorStateLabels( const NameSet &locations );

struct ColmParser;

typedef AvlMap<String, ColmParser *, CmpStr> ParserDict;
typedef AvlMapEl<String, ColmParser *> ParserDictEl;

LangEl *declareLangEl( ParseData *pd, Namespace *nspace, const String &data, LangEl::Type type );
LangEl *addLangEl( ParseData *pd, Namespace *nspace, const String &data, LangEl::Type type );
void declareTypeAlias( ParseData *pd, Namespace *nspace, const String &data, TypeRef *typeRef );
LangEl *findType( ParseData *pd, Namespace *nspace, const String &data );

#endif /* _PARSEDATA_H */
