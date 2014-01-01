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

#ifndef _PARSETREE_H
#define _PARSETREE_H

#include <iostream>
#include <string.h>

#include "global.h"
#include "avlmap.h"
#include "bstmap.h"
#include "bstset.h"
#include "vector.h"
#include "dlist.h"
#include "dlistval.h"
#include "dlistmel.h"
#include "cstring.h"
#include "bytecode.h"
#include "avlbasic.h"

/* Operators that are represented with single symbol characters. */
#define OP_DoubleEql 'e'
#define OP_NotEql 'q'
#define OP_LessEql 'l'
#define OP_GrtrEql 'g'
#define OP_LogicalAnd 'a'
#define OP_LogicalOr 'o'
#define OP_Deref 'd'

#if SIZEOF_LONG != 4 && SIZEOF_LONG != 8 
	#error "SIZEOF_LONG contained an unexpected value"
#endif

struct NameInst;
struct FsmGraph;
struct RedFsm;
struct _FsmRun;
struct ObjectDef;
struct ElementOf;
struct UniqueType;
struct ObjectField;
struct TransBlock;
struct CodeBlock;
struct PdaLiteral;
struct TypeAlias;
struct RegionSet;
struct ObjNameScope;
typedef struct _PdaRun PdaRun;

/* 
 * Code Vector
 */
struct CodeVect : public Vector<Code>
{
	void appendHalf( Half half )
	{
		/* not optimal. */
		append( half & 0xff );
		append( (half>>8) & 0xff );
	}
	
	void appendWord( Word word )
	{
		/* not optimal. */
		append( word & 0xff );
		append( (word>>8) & 0xff );
		append( (word>>16) & 0xff );
		append( (word>>24) & 0xff );
		#if SIZEOF_LONG == 8
		append( (word>>32) & 0xff );
		append( (word>>40) & 0xff );
		append( (word>>48) & 0xff );
		append( (word>>56) & 0xff );
		#endif
	}

	void setHalf( long pos, Half half )
	{
		/* not optimal. */
		data[pos] = half & 0xff;
		data[pos+1] = (half>>8) & 0xff;
	}
	
	void insertHalf( long pos, Half half )
	{
		/* not optimal. */
		insert( pos, half & 0xff );
		insert( pos+1, (half>>8) & 0xff );
	}

	void insertWord( long pos, Word word )
	{
		/* not at all optimal. */
		insert( pos, word & 0xff );
		insert( pos+1, (word>>8) & 0xff );
		insert( pos+2, (word>>16) & 0xff );
		insert( pos+3, (word>>24) & 0xff );
		#if SIZEOF_LONG == 8
		insert( pos+4, (word>>32) & 0xff );
		insert( pos+5, (word>>40) & 0xff );
		insert( pos+6, (word>>48) & 0xff );
		insert( pos+7, (word>>56) & 0xff );
		#endif
	}
	
	void insertTree( long pos, Tree *tree )
		{ insertWord( pos, (Word) tree ); }
};



/* Types of builtin machines. */
enum BuiltinMachine
{
	BT_Any,
	BT_Ascii,
	BT_Extend,
	BT_Alpha,
	BT_Digit,
	BT_Alnum,
	BT_Lower,
	BT_Upper,
	BT_Cntrl,
	BT_Graph,
	BT_Print,
	BT_Punct,
	BT_Space,
	BT_Xdigit,
	BT_Lambda,
	BT_Empty
};

/* Must match the LI defines in pdarun.h. */
enum LocalType
{
	LT_Tree = 1,
	LT_Iter,
	LT_RevIter,
	LT_UserIter
};

struct LocalLoc
{
	LocalLoc( LocalType type, int scope, int offset )
		: scope(scope), type(type), offset(offset) {}

	int scope;
	LocalType type;
	int offset;
};

struct Locals
{
	Vector<LocalLoc> locals;

	void append( const LocalLoc &ll )
	{
		int pos = 0;
		while ( pos < locals.length() && ll.scope >= locals[pos].scope )
			pos += 1;
		locals.insert( pos, ll );
	}
};

struct IterLoc
{
	IterLoc( int scope, int loc )
		: scope(scope), loc(loc) {}

	int scope;
	int loc;
};

struct Iters
{
	Vector<IterLoc> ordered;
	Vector<char> locs;

	void append( const IterLoc &il )
	{
		int pos = 0;
		while ( pos < ordered.length() && il.scope >= ordered[pos].scope )
			pos += 1;
		ordered.insert( pos, il );
		locs.insert( pos, il.loc );
	}

	char *data() { return locs.data; }
	int length() { return locs.length(); }
};

typedef BstSet<char> CharSet;
typedef Vector<unsigned char> UnsignedCharVect;


struct Compiler;
struct TypeRef;

/* Leaf type. */
struct Literal;

/* Tree nodes. */

struct LexTerm;
struct LexFactorAug;
struct LexFactorRep;
struct LexFactorNeg;
struct LexFactor;
struct LexExpression;
struct LexJoin;
struct JoinOrLm;
struct RegionJoinOrLm;
struct TokenRegion;
struct Namespace;
struct Context;
struct TokenDef;
struct TokenDefListReg;
struct TokenDefListNs;
struct TokenInstance;
struct TokenInstanceListReg;
struct Range;
struct LangEl;

enum AugType
{
	at_start,
	at_leave,
};

struct Action;
struct PriorDesc;
struct RegExpr;
struct ReItem;
struct ReOrBlock;
struct ReOrItem;
struct ExplicitMachine;
struct InlineItem;
struct InlineList;

/* Reference to a named state. */
typedef Vector<String> NameRef;
typedef Vector<NameRef*> NameRefList;
typedef Vector<NameInst*> NameTargList;

/* Structure for storing location of epsilon transitons. */
struct EpsilonLink
{
	EpsilonLink( const InputLoc &loc, NameRef &target )
		: loc(loc), target(target) { }

	InputLoc loc;
	NameRef target;
};

struct Label
{
	Label( const InputLoc &loc, const String &data, ObjectField *objField )
		: loc(loc), data(data), objField(objField) { }

	InputLoc loc;
	String data;
	ObjectField *objField;
};

/* Structure represents an action assigned to some LexFactorAug node. The
 * factor with aug will keep an array of these. */
struct ParserAction
{
	ParserAction( const InputLoc &loc, AugType type, int localErrKey, Action *action )
		: loc(loc), type(type), localErrKey(localErrKey), action(action) { }

	InputLoc loc;
	AugType type;
	int localErrKey;
	Action *action;
};

struct Token
{
	String data;
	InputLoc loc;
};

void prepareLitString( String &result, bool &caseInsensitive, 
		const String &srcString, const InputLoc &loc );

std::ostream &operator<<(std::ostream &out, const Token &token );

typedef AvlMap< String, TokenInstance*, CmpStr > LiteralDict;
typedef AvlMapEl< String, TokenInstance* > LiteralDictEl;

/* Store the value and type of a priority augmentation. */
struct PriorityAug
{
	PriorityAug( AugType type, int priorKey, int priorValue ) :
		type(type), priorKey(priorKey), priorValue(priorValue) { }

	AugType type;
	int priorKey;
	int priorValue;
};

/*
 * A Variable Definition
 */
struct LexDefinition
{
	LexDefinition( const String &name, LexJoin *join )
		: name(name), join(join) { }
	
	/* Parse tree traversal. */
	FsmGraph *walk( Compiler *pd );
	void makeNameTree( const InputLoc &loc, Compiler *pd );

	String name;
	LexJoin *join;
};

typedef Vector<String> StringVect;
typedef CmpTable<String, CmpStr> CmpStrVect;

struct NamespaceQual
{
	NamespaceQual()
	:
		cachedNspaceQual(0),
		declInNspace(0)
	{}

	static NamespaceQual *cons( Namespace *declInNspace )
	{
		NamespaceQual *nsq = new NamespaceQual;
		nsq->declInNspace = declInNspace;
		return nsq;
	}

	Namespace *cachedNspaceQual;
	Namespace *declInNspace;

	StringVect qualNames;

	Namespace *searchFrom( Namespace *from, StringVect::Iter &qualPart );
	Namespace *getQual( Compiler *pd );
	bool thisOnly()
		{ return qualNames.length() != 0; }
};

struct ReCapture
{
	ReCapture( Action *markEnter, Action *markLeave, ObjectField *objField )
		: markEnter(markEnter), markLeave(markLeave), objField(objField)  {}

	Action *markEnter;
	Action *markLeave;
	ObjectField *objField;
};

struct ContextStack
	: public Vector<Context*>
{
	Context *top()
		{ return length() > 0 ? Vector<Context*>::top() : 0; }
};


struct Context
{
	Context( const InputLoc &loc, LangEl *lel )
	:
		loc(loc),
		lel(lel)
	{}

	InputLoc loc;
	LangEl *lel;

	ObjectDef *contextObjDef;
};

typedef Vector<ReCapture> ReCaptureVect;

struct TokenDefPtr1
{
	TokenDef *prev, *next;
};

struct TokenDefPtr2
{
	TokenDef *prev, *next;
};

struct TokenDef
:
	public TokenDefPtr1, 
	public TokenDefPtr2
{
	TokenDef()
	: 
		action(0), tdLangEl(0), inLmSelect(false), dupOf(0),
		noPostIgnore(false), noPreIgnore(false), isZero(false)
	{}

	static TokenDef *cons( const String &name, const String &literal, bool isLiteral, bool isIgnore,
		LexJoin *join, CodeBlock *codeBlock, const InputLoc &semiLoc, 
		int longestMatchId, Namespace *nspace, RegionSet *regionSet,
		ObjectDef *objectDef, Context *contextIn )
	{ 
		TokenDef *t = new TokenDef;

		t->name = name;
		t->literal = literal;
		t->isLiteral = isLiteral;
		t->isIgnore = isIgnore;
		t->join = join;
		t->action = 0;
		t->codeBlock = codeBlock;
		t->tdLangEl = 0;
		t->semiLoc = semiLoc;
		t->longestMatchId = longestMatchId;
		t->inLmSelect = false;
		t->nspace = nspace;
		t->regionSet = regionSet;
		t->objectDef = objectDef;
		t->contextIn = contextIn;
		t->dupOf = 0;
		t->noPostIgnore = false;
		t->noPreIgnore = false;
		t->isZero = false;

		return t;
	}

	InputLoc getLoc();
	
	String name;
	String literal;
	bool isLiteral;
	bool isIgnore;
	LexJoin *join;
	Action *action;
	CodeBlock *codeBlock;
	LangEl *tdLangEl;
	InputLoc semiLoc;

	Action *setActId;
	Action *actOnLast;
	Action *actOnNext;
	Action *actLagBehind;
	int longestMatchId;
	bool inLmSelect;
	Namespace *nspace;
	RegionSet *regionSet;
	ReCaptureVect reCaptureVect;
	ObjectDef *objectDef;
	Context *contextIn;

	TokenDef *dupOf;
	bool noPostIgnore;
	bool noPreIgnore;
	bool isZero;
};

struct TokenInstancePtr
{
	TokenInstance *prev, *next;
};

struct TokenInstance
:
	public TokenInstancePtr
{
	TokenInstance()
	: 
		action(0),
		inLmSelect(false),
		dupOf(0)
	{}

	static TokenInstance *cons( TokenDef *tokenDef,
		LexJoin *join, const InputLoc &semiLoc, 
		int longestMatchId, Namespace *nspace, TokenRegion *tokenRegion )
	{ 
		TokenInstance *t = new TokenInstance;

		t->tokenDef = tokenDef;
		t->join = join;
		t->action = 0;
		t->semiLoc = semiLoc;
		t->longestMatchId = longestMatchId;
		t->inLmSelect = false;
		t->nspace = nspace;
		t->tokenRegion = tokenRegion;
		t->dupOf = 0;

		return t;
	}

	InputLoc getLoc();
	
	TokenDef *tokenDef;
	LexJoin *join;
	Action *action;
	InputLoc semiLoc;

	Action *setActId;
	Action *actOnLast;
	Action *actOnNext;
	Action *actLagBehind;
	int longestMatchId;
	bool inLmSelect;
	Namespace *nspace;
	TokenRegion *tokenRegion;

	TokenInstance *dupOf;
};

struct LelDefList;

struct NtDef
{
	static NtDef *cons( const String &name, Namespace *nspace,
		LelDefList *defList, ObjectDef *objectDef,
		Context *contextIn, bool reduceFirst )
	{ 
		NtDef *nt = new NtDef;

		nt->name = name;
		nt->nspace = nspace;
		nt->defList = defList;
		nt->objectDef = objectDef;
		nt->contextIn = contextIn;
		nt->reduceFirst = reduceFirst;

		return nt;
	}

	static NtDef *cons( const String &name, Namespace *nspace,
		Context *contextIn, bool reduceFirst )
	{ 
		NtDef *nt = new NtDef;

		nt->name = name;
		nt->nspace = nspace;
		nt->defList = 0;
		nt->objectDef = 0;
		nt->contextIn = contextIn;
		nt->reduceFirst = reduceFirst;

		return nt;
	}

	String name;
	Namespace *nspace;
	LelDefList *defList;
	ObjectDef *objectDef;
	Context *contextIn;
	bool reduceFirst;

	NtDef *prev, *next;
};

struct NtDefList : DList<NtDef> {};

/* Declare a new type so that ptreetypes.h need not include dlist.h. */
struct TokenInstanceListReg : DListMel<TokenInstance, TokenInstancePtr> {};

/* Declare a new type so that ptreetypes.h need not include dlist.h. */
struct TokenDefListReg : DListMel<TokenDef, TokenDefPtr1> {};
struct TokenDefListNs : DListMel<TokenDef, TokenDefPtr2> {};

struct ContextDef
{
	ContextDef( const String &name, Context *context, Namespace *nspace )
		: name(name), context(context), nspace(nspace) {}

	String name;
	Context *context;
	Namespace *nspace;

	ContextDef *prev, *next;
};

struct ContextDefList : DList<ContextDef> {};

struct TypeMapEl
	: public AvlTreeEl<TypeMapEl>
{
	enum Type
	{
		TypeAliasType = 1,
		LangElType
	};

	const String &getKey() { return key; }

	TypeMapEl( const String &key, TypeRef *typeRef )
		: type(TypeAliasType), key(key), value(0), typeRef(typeRef) {}

	TypeMapEl( const String &key, LangEl *value )
		: type(LangElType), key(key), value(value), typeRef(0) {}

	Type type;
	String key;
	LangEl *value;
	TypeRef *typeRef;
	
	TypeMapEl *prev, *next;
};

/* Symbol Map. */
typedef AvlTree< TypeMapEl, String, CmpStr > TypeMap;

typedef Vector<TokenRegion*> RegionVect;

struct RegionImpl
{
	RegionImpl()
	:
		regionNameInst(0),
		lmActSelect(0),
		lmSwitchHandlesError(false),
		defaultTokenInstance(0),
		wasEmpty(false)
	{}

	InputLoc loc;

	/* This gets saved off during the name walk. Can save it off because token
	 * regions are referenced once only. */
	NameInst *regionNameInst;

	TokenInstanceListReg tokenInstanceList;
	Action *lmActSelect;
	bool lmSwitchHandlesError;
	TokenInstance *defaultTokenInstance;

	/* We alway init empty scanners with a single token. If we had to do this
	 * then wasEmpty is true. */
	bool wasEmpty;

	RegionImpl *prev, *next;

	void runLongestMatch( Compiler *pd, FsmGraph *graph );
	void transferScannerLeavingActions( FsmGraph *graph );
	FsmGraph *walk( Compiler *pd );

	void restart( FsmGraph *graph, FsmTrans *trans );
	void makeNameTree( const InputLoc &loc, Compiler *pd );
	void makeActions( Compiler *pd );
	Action *newAction( Compiler *pd, const InputLoc &loc,
			const String &name, InlineList *inlineList );
};

struct TokenRegion
{
	/* Construct with a list of joins */
	TokenRegion( const InputLoc &loc, int id, RegionImpl *impl )
	: 
		loc(loc),
		id(id),
		preEofBlock(0), 
		zeroLel(0),
		ignoreOnly(0),
		impl(impl)
	{ }

	InputLoc loc;
	int id;

	CodeBlock *preEofBlock;

	LangEl *zeroLel;
	TokenRegion *ignoreOnly;

	RegionImpl *impl;

	TokenRegion *next, *prev;

	/* Tree traversal. */
	void findName( Compiler *pd );
};

struct RegionSet
{
	RegionSet( RegionImpl *implTokenIgnore, RegionImpl *implTokenOnly,
			RegionImpl *implIgnoreOnly, TokenRegion *tokenIgnore,
			TokenRegion *tokenOnly, TokenRegion *ignoreOnly,
			TokenRegion *collectIgnore )
	:
		implTokenIgnore(implTokenIgnore),
		implTokenOnly(implTokenOnly),
		implIgnoreOnly(implIgnoreOnly),

		tokenIgnore(tokenIgnore),
		tokenOnly(tokenOnly),
		ignoreOnly(ignoreOnly),
		collectIgnore(collectIgnore)
	{}

	/* Provides the scanner state machines. We reuse ignore-only. */
	RegionImpl *implTokenIgnore;
	RegionImpl *implTokenOnly;
	RegionImpl *implIgnoreOnly;

	TokenRegion *tokenIgnore;
	TokenRegion *tokenOnly;
	TokenRegion *ignoreOnly;
	TokenRegion *collectIgnore;


	TokenDefListReg tokenDefList;

	RegionSet *next, *prev;
};

typedef Vector<RegionSet*> RegionSetVect;

typedef DList<RegionSet> RegionSetList;
typedef DList<TokenRegion> RegionList;
typedef DList<RegionImpl> RegionImplList;

typedef Vector<Namespace*> NamespaceVect;

struct GenericType 
	: public DListEl<GenericType>
{
	GenericType( const String &name, long typeId, long id, 
			LangEl *langEl, TypeRef *typeArg )
	:
		name(name), typeId(typeId), id(id), langEl(langEl),
		typeArg(typeArg), keyTypeArg(0), 
		utArg(0), keyUT(0),
		objDef(0)
	{}

	const String &getKey() const 
		{ return name; };

	void declare( Compiler *pd, Namespace *nspace );

	String name;
	long typeId;
	long id;
	LangEl *langEl;
	TypeRef *typeArg;
	TypeRef *keyTypeArg;
	UniqueType *utArg;
	UniqueType *keyUT;

	ObjectDef *objDef;
};

typedef DList<GenericType> GenericList;

typedef struct _UserIter UserIter;
typedef AvlMap<String, UserIter*, CmpStr> UserIterMap;
typedef AvlMapEl<String, UserIter*> UserIterMapEl;

/* Graph dictionary. */
struct GraphDictEl 
:
	public AvlTreeEl<GraphDictEl>,
	public DListEl<GraphDictEl>
{
	GraphDictEl( const String &key ) 
		: key(key), value(0), isInstance(false) { }

	GraphDictEl( const String &key, LexDefinition *value ) 
		: key(key), value(value), isInstance(false) { }

	const String &getKey() { return key; }

	String key;
	LexDefinition *value;
	bool isInstance;

	/* Location info of graph definition. Points to variable name of assignment. */
	InputLoc loc;
};

typedef AvlTree<GraphDictEl, String, CmpStr> GraphDict;
typedef DList<GraphDictEl> GraphList;

struct TypeAlias
{
	TypeAlias( const InputLoc &loc, Namespace *nspace, 
			const String &name, TypeRef *typeRef )
	:
		loc(loc),
		nspace(nspace),
		name(name),
		typeRef(typeRef)
	{}

	InputLoc loc;
	Namespace *nspace;
	String name;
	TypeRef *typeRef;

	TypeAlias *prev, *next;
};

typedef DList<TypeAlias> TypeAliasList;

struct Namespace
{
	/* Construct with a list of joins */
	Namespace( const InputLoc &loc, const String &name, int id, 
			Namespace *parentNamespace ) : 
		loc(loc), name(name), id(id),
		parentNamespace(parentNamespace) { }

	/* Tree traversal. */
	Namespace *findNamespace( const String &name );

	InputLoc loc;
	String name;
	int id;

	/* Literal patterns and the dictionary mapping literals to the underlying
	 * tokens. */
	LiteralDict literalDict;

	/* List of tokens defs in the namespace. */
	TokenDefListNs tokenDefList;

	/* List of nonterminal defs in the namespace. */
	NtDefList ntDefList;

	/* List of context definitions for encapsulating the data of a parser. */
	ContextDefList contextDefList;

	/* Dictionary of symbols within the region. */
	TypeMap typeMap;
	GenericList genericList;

	/* regular language definitions. */
	GraphDict rlMap;

	TypeAliasList typeAliasList;

	Namespace *parentNamespace;
	NamespaceVect childNamespaces;

	Namespace *next, *prev;

	void declare( Compiler *pd );
};

typedef DList<Namespace> NamespaceList;
typedef BstSet< Namespace*, CmpOrd<Namespace*> > NamespaceSet;

/*
 * LexJoin
 */
struct LexJoin
{
	LexJoin()
	:
		expr(0),
		context(0),
		mark(0)
	{}

	static LexJoin *cons( LexExpression *expr )
	{
		LexJoin *j = new LexJoin;
		j->expr = expr;
		return j;
	}

	/* Tree traversal. */
	FsmGraph *walk( Compiler *pd );
	void makeNameTree( Compiler *pd );
	void varDecl( Compiler *pd, TokenDef *tokenDef );

	/* Data. */
	LexExpression *expr;
	LexJoin *context;
	Action *mark;
};

/*
 * LexExpression
 */
struct LexExpression
{
	enum Type { 
		OrType,
		IntersectType, 
		SubtractType, 
		StrongSubtractType,
		TermType, 
		BuiltinType
	};

	LexExpression( ) : 
		expression(0), term(0), builtin((BuiltinMachine)-1), 
		type((Type)-1), prev(this), next(this) { }

	/* Construct with an expression on the left and a term on the right. */
	static LexExpression *cons( LexExpression *expression, LexTerm *term, Type type )
	{ 
		LexExpression *ret = new LexExpression;
		ret->type = type;
		ret->expression = expression;
		ret->term = term;
		return ret;
	}

	/* Construct with only a term. */
	static LexExpression *cons( LexTerm *term )
	{
		LexExpression *ret = new LexExpression;
		ret->type = TermType;
		ret->term = term;
		return ret;
	}
	
	/* Construct with a builtin type. */
	static LexExpression *cons( BuiltinMachine builtin )
	{
		LexExpression *ret = new LexExpression;
		ret->type = BuiltinType;
		ret->builtin = builtin;
		return ret;
	}

	~LexExpression();

	/* Tree traversal. */
	FsmGraph *walk( Compiler *pd, bool lastInSeq = true );
	void makeNameTree( Compiler *pd );
	void varDecl( Compiler *pd, TokenDef *tokenDef );

	/* Node data. */
	LexExpression *expression;
	LexTerm *term;
	BuiltinMachine builtin;
	Type type;

	LexExpression *prev, *next;
};

/*
 * LexTerm
 */
struct LexTerm
{
	enum Type { 
		ConcatType, 
		RightStartType,
		RightFinishType,
		LeftType,
		FactorAugType
	};

	LexTerm() :
		term(0), factorAug(0), type((Type)-1) { }

	static LexTerm *cons( LexTerm *term, LexFactorAug *factorAug )
	{
		LexTerm *ret = new LexTerm;
		ret->type = ConcatType;
		ret->term = term;
		ret->factorAug = factorAug;
		return ret;
	}

	static LexTerm *cons( LexTerm *term, LexFactorAug *factorAug, Type type )
	{
		LexTerm *ret = new LexTerm;
		ret->type = type;
		ret->term = term;
		ret->factorAug = factorAug;
		return ret;
	}

	static LexTerm *cons( LexFactorAug *factorAug )
	{
		LexTerm *ret = new LexTerm;
		ret->type = FactorAugType;
		ret->factorAug = factorAug;
		return ret;
	}
	
	~LexTerm();

	FsmGraph *walk( Compiler *pd, bool lastInSeq = true );
	void makeNameTree( Compiler *pd );
	void varDecl( Compiler *pd, TokenDef *tokenDef );

	LexTerm *term;
	LexFactorAug *factorAug;
	Type type;

	/* Priority descriptor for RightFinish type. */
	PriorDesc priorDescs[2];
};


/* Third level of precedence. Augmenting nodes with actions and priorities. */
struct LexFactorAug
{
	LexFactorAug() :
		factorRep(0) { }

	static LexFactorAug *cons( LexFactorRep *factorRep )
	{
		LexFactorAug *f = new LexFactorAug;
		f->factorRep = factorRep;
		return f;
	}

	~LexFactorAug();

	/* Tree traversal. */
	FsmGraph *walk( Compiler *pd );
	void makeNameTree( Compiler *pd );
	void varDecl( Compiler *pd, TokenDef *tokenDef );

	void assignActions( Compiler *pd, FsmGraph *graph, int *actionOrd );

	/* Actions and priorities assigned to the factor node. */
	Vector<ParserAction> actions;
	ReCaptureVect reCaptureVect;

	LexFactorRep *factorRep;
};

/* Fourth level of precedence. Trailing unary operators. Provide kleen star,
 * optional and plus. */
struct LexFactorRep
{
	enum Type { 
		StarType,
		StarStarType,
		OptionalType,
		PlusType, 
		ExactType,
		MaxType,
		MinType,
		RangeType,
		FactorNegType
	};

	LexFactorRep()
	:
		factorRep(0),
		factorNeg(0),
		lowerRep(0), 
		upperRep(upperRep),
		type((Type)-1)
	{ }

	static LexFactorRep *cons( const InputLoc &loc, LexFactorRep *factorRep, 
			int lowerRep, int upperRep, Type type )
	{
		LexFactorRep *f = new LexFactorRep;
		f->type = type;
		f->loc = loc;
		f->factorRep = factorRep;
		f->factorNeg = 0;
		f->lowerRep = lowerRep;
		f->upperRep = upperRep;
		return f;
	}
	
	static LexFactorRep *cons( LexFactorNeg *factorNeg )
	{
		LexFactorRep *f = new LexFactorRep;
		f->type = FactorNegType;
		f->factorNeg = factorNeg;
		return f;
	}

	~LexFactorRep();

	/* Tree traversal. */
	FsmGraph *walk( Compiler *pd );
	void makeNameTree( Compiler *pd );

	InputLoc loc;
	LexFactorRep *factorRep;
	LexFactorNeg *factorNeg;
	int lowerRep, upperRep;
	Type type;

	/* Priority descriptor for StarStar type. */
	PriorDesc priorDescs[2];
};

/* Fifth level of precedence. Provides Negation. */
struct LexFactorNeg
{
	enum Type { 
		NegateType, 
		CharNegateType,
		FactorType
	};

	LexFactorNeg()
	:
		factorNeg(0),
		factor(0),
		type((Type)-1)
	{}

	static LexFactorNeg *cons( LexFactorNeg *factorNeg, Type type )
	{
		LexFactorNeg *f = new LexFactorNeg;
		f->type = type;
		f->factorNeg = factorNeg;
		f->factor = 0;
		return f;
	}

	static LexFactorNeg *cons( LexFactor *factor )
	{
		LexFactorNeg *f = new LexFactorNeg;
		f->type = FactorType;
		f->factorNeg = 0;
		f->factor = factor;
		return f;
	}

	~LexFactorNeg();

	/* Tree traversal. */
	FsmGraph *walk( Compiler *pd );
	void makeNameTree( Compiler *pd );

	LexFactorNeg *factorNeg;
	LexFactor *factor;
	Type type;
};

/*
 * LexFactor
 */
struct LexFactor
{
	/* Language elements a factor node can be. */
	enum Type {
		LiteralType, 
		RangeType, 
		OrExprType,
		RegExprType, 
		ReferenceType,
		ParenType,
	}; 
	
	LexFactor()
	:
		literal(0),
		range(0),
		reItem(0),
		regExp(0),
		varDef(0),
		join(0),
		lower(0),
		upper(0),
		type((Type)-1)
	{}

	/* Construct with a literal fsm. */
	static LexFactor *cons( Literal *literal )
	{
		LexFactor *f = new LexFactor;
		f->type = LiteralType;
		f->literal = literal;
		return f;
	}

	/* Construct with a range. */
	static LexFactor *cons( Range *range )
	{
		LexFactor *f = new LexFactor;
		f->type = RangeType;
		f->range = range;
		return f;
	}
	
	/* Construct with the or part of a regular expression. */
	static LexFactor *cons( ReItem *reItem )
	{
		LexFactor *f = new LexFactor;
		f->type = OrExprType;
		f->reItem = reItem;
		return f;
	}

	/* Construct with a regular expression. */
	static LexFactor *cons( RegExpr *regExp )
	{
		LexFactor *f = new LexFactor;
		f->type = RegExprType;
		f->regExp = regExp;
		return f;
	}

	/* Construct with a reference to a var def. */
	static LexFactor *cons( const InputLoc &loc, LexDefinition *varDef )
	{
		LexFactor *f = new LexFactor;
		f->type = ReferenceType;
		f->loc = loc;
		f->varDef = varDef;
		return f;
	}

	/* Construct with a parenthesized join. */
	static LexFactor *cons( LexJoin *join )
	{
		LexFactor *f = new LexFactor;
		f->type = ParenType;
		f->join = join;
		return f;
	}
	
	/* Cleanup. */
	~LexFactor();

	/* Tree traversal. */
	FsmGraph *walk( Compiler *pd );
	void makeNameTree( Compiler *pd );

	InputLoc loc;
	Literal *literal;
	Range *range;
	ReItem *reItem;
	RegExpr *regExp;
	LexDefinition *varDef;
	LexJoin *join;
	int lower, upper;
	Type type;
};

/* A range machine. Only ever composed of two literals. */
struct Range
{
	static Range *cons( Literal *lowerLit, Literal *upperLit ) 
	{
		Range *r = new Range;
		r->lowerLit = lowerLit;
		r->upperLit = upperLit;
		return r;
	}

	~Range();
	FsmGraph *walk( Compiler *pd );
	bool verifyRangeFsm( FsmGraph *rangeEnd );

	Literal *lowerLit;
	Literal *upperLit;
};

/* Some literal machine. Can be a number or literal string. */
struct Literal
{
	enum LiteralType { Number, LitString };

	static Literal *cons( const InputLoc &loc, const String &literal, LiteralType type )
	{
		Literal *l = new Literal;
		l->loc = loc;
		l->literal = literal;
		l->type = type;
		return l;
	}

	FsmGraph *walk( Compiler *pd );
	
	InputLoc loc;
	String literal;
	LiteralType type;
};

/* Regular expression. */
struct RegExpr
{
	enum RegExpType { RecurseItem, Empty };

	/* Constructors. */
	static RegExpr *cons() 
	{
		RegExpr *r = new RegExpr;
		r->type = Empty;
		r->caseInsensitive = false;
		return r;
	}

	static RegExpr *cons( RegExpr *regExp, ReItem *item )
	{
		RegExpr *r = new RegExpr;
		r->regExp = regExp;
		r->item = item;
		r->type = RecurseItem;
		r->caseInsensitive = false;
		return r;
	}

	~RegExpr();
	FsmGraph *walk( Compiler *pd, RegExpr *rootRegex );

	RegExpr *regExp;
	ReItem *item;
	RegExpType type;
	bool caseInsensitive;
};

/* An item in a regular expression. */
struct ReItem
{
	enum ReItemType { Data, Dot, OrBlock, NegOrBlock };
	
	static ReItem *cons( const String &data ) 
	{
		ReItem *r = new ReItem;
		r->data = data;
		r->type = Data;
		return r;
	}

	static ReItem *cons( ReItemType type )
	{
		ReItem *r = new ReItem;
		r->type = type;
		return r;
	}

	static ReItem *cons( ReOrBlock *orBlock, ReItemType type )
	{
		ReItem *r = new ReItem;
		r->orBlock = orBlock;
		r->type = type;
		return r;
	}

	~ReItem();
	FsmGraph *walk( Compiler *pd, RegExpr *rootRegex );

	String data;
	ReOrBlock *orBlock;
	ReItemType type;
};

/* An or block item. */
struct ReOrBlock
{
	enum ReOrBlockType { RecurseItem, Empty };

	/* Constructors. */
	static ReOrBlock *cons()
	{
		ReOrBlock *r = new ReOrBlock;
		r->type = Empty;
		return r;
	}

	static ReOrBlock *cons( ReOrBlock *orBlock, ReOrItem *item )
	{
		ReOrBlock *r = new ReOrBlock;
		r->orBlock = orBlock;
		r->item = item;
		r->type = RecurseItem;
		return r;
	}

	~ReOrBlock();
	FsmGraph *walk( Compiler *pd, RegExpr *rootRegex );
	
	ReOrBlock *orBlock;
	ReOrItem *item;
	ReOrBlockType type;
};

/* An item in an or block. */
struct ReOrItem
{
	enum ReOrItemType { Data, Range };

	static ReOrItem *cons( const InputLoc &loc, const String &data ) 
	{
		ReOrItem *r = new ReOrItem;
		r->loc = loc;
		r->data = data;
		r->type = Data;
		return r;
	}

	static ReOrItem *cons( const InputLoc &loc, char lower, char upper )
	{
		ReOrItem *r = new ReOrItem;
		r->loc = loc;
		r->lower = lower;
		r->upper = upper;
		r->type = Range;
		return r;
	}

	FsmGraph *walk( Compiler *pd, RegExpr *rootRegex );

	InputLoc loc;
	String data;
	char lower;
	char upper;
	ReOrItemType type;
};


/*
 * Inline code tree
 */
struct InlineList;
struct InlineItem
{
	enum Type 
	{
		Text, 
		LmSwitch, 
		LmSetActId, 
		LmSetTokEnd, 
		LmOnLast, 
		LmOnNext,
		LmOnLagBehind, 
		LmInitAct, 
		LmInitTokStart, 
		LmSetTokStart 
	};

	static InlineItem *cons( const InputLoc &loc, const String &data, Type type )
	{
		InlineItem *i = new InlineItem;
		i->loc = loc;
		i->data = data;
		i->nameRef = 0;
		i->children = 0;
		i->type = type;
		return i;
	}

	static InlineItem *cons( const InputLoc &loc, NameRef *nameRef, Type type )
	{
		InlineItem *i = new InlineItem;
		i->loc = loc;
		i->nameRef = nameRef;
		i->children = 0;
		i->type = type;
		return i;
	}

	static InlineItem *cons( const InputLoc &loc, RegionImpl *tokenRegion, 
		TokenInstance *longestMatchPart, Type type ) 
	{
		InlineItem *i = new InlineItem;
		i->loc = loc;
		i->nameRef = 0;
		i->children = 0;
		i->tokenRegion = tokenRegion;
		i->longestMatchPart = longestMatchPart;
		i->type = type;
		return i;
	}

	static InlineItem *cons( const InputLoc &loc, NameInst *nameTarg, Type type )
	{
		InlineItem *i = new InlineItem;
		i->loc = loc;
		i->nameRef = 0;
		i->nameTarg = nameTarg;
		i->children = 0;
		i->type = type;
		return i;
	}

	static InlineItem *cons( const InputLoc &loc, Type type ) 
	{
		InlineItem *i = new InlineItem;
		i->loc = loc;
		i->nameRef = 0;
		i->children = 0;
		i->type = type;
		return i;
	}
	
	InputLoc loc;
	String data;
	NameRef *nameRef;
	NameInst *nameTarg;
	InlineList *children;
	RegionImpl *tokenRegion;
	TokenInstance *longestMatchPart;
	Type type;

	InlineItem *prev, *next;
};

struct InlineList 
:
	public DList<InlineItem>
{
	InlineList( int i ) {}

	static InlineList *cons()
	{
		return new InlineList( 0 );
	}
};


struct ProdEl;
struct LangVarRef;
struct ObjectField;

struct PatternItem
{
	enum Form { 
		TypeRef,
		InputText
	};

	static PatternItem *cons( const InputLoc &loc, const String &data, Form form )
	{
		PatternItem *p = new PatternItem;
		p->form = form;
		p->loc = loc;
		p->prodEl = 0;
		p->data = data;
		p->region = 0;
		p->varRef = 0;
		p->bindId = 0;
		return p;
	}

	static PatternItem *cons( const InputLoc &loc, ProdEl *prodEl, Form form )
	{
		PatternItem *p = new PatternItem;
		p->form = form;
		p->loc = loc;
		p->prodEl = prodEl;
		p->region = 0;
		p->varRef = 0;
		p->bindId = 0;
		return p;
	}

	Form form;
	InputLoc loc;
	ProdEl *prodEl;
	String data;
	TokenRegion *region;
	LangVarRef *varRef;
	long bindId;
	PatternItem *prev, *next;
};

struct LangExpr;

struct PatternItemList
	: public DList<PatternItem>
{
	static PatternItemList *cons( PatternItem *patternItem )
	{
		PatternItemList *list = new PatternItemList;
		list->append( patternItem );
		return list;
	}
};

struct ConsItem
{
	enum Type { 
		InputText, 
		ExprType,
		LiteralType
	};

	ConsItem()
	:
		type((Type)-1),
		expr(0),
		langEl(0),
		prodEl(0),
		bindId(-1)
	{
	}

	static ConsItem *cons( const InputLoc &loc, Type type, const String &data )
	{
		ConsItem *r = new ConsItem;
		r->loc = loc;
		r->type = type;
		r->data = data;
		return r;
	}

	static ConsItem *cons( const InputLoc &loc, Type type, LangExpr *expr )
	{
		ConsItem *r = new ConsItem;
		r->loc = loc;
		r->type = type;
		r->expr = expr;
		return r;
	}

	static ConsItem *cons( const InputLoc &loc, Type type, ProdEl *prodEl )
	{
		ConsItem *r = new ConsItem;
		r->loc = loc;
		r->type = type;
		r->expr = 0;
		r->prodEl = prodEl;
		return r;
	}

	InputLoc loc;
	Type type;
	String data;
	LangExpr *expr;
	LangEl *langEl;
	ProdEl *prodEl;
	long bindId;
	ConsItem *prev, *next;
};

struct ConsItemList
:
	public DList<ConsItem>
{
	static ConsItemList *cons( ConsItem *ci )
	{
		ConsItemList *cil = new ConsItemList;
		cil->append( ci );
		return cil;
	}
};

struct Pattern
{
	static Pattern *cons( const InputLoc &loc, Namespace *nspace,
			PatternItemList *list, int patRepId )
	{
		Pattern *p = new Pattern;
		p->loc = loc;
		p->nspace = nspace;
		p->list = list;
		p->patRepId = patRepId;
		p->langEl = 0;
		p->pdaRun = 0;
		p->nextBindId = 1;
		return p;
	}
	
	InputLoc loc;
	Namespace *nspace;
	PatternItemList *list;
	long patRepId;
	LangEl *langEl;
	PdaRun *pdaRun;
	long nextBindId;
	Pattern *prev, *next;
};

typedef DList<Pattern> PatList;

struct Constructor
{
	static Constructor *cons( const InputLoc &loc, Namespace *nspace, 
			ConsItemList *list, int patRepId )
	{
		Constructor *r = new Constructor;
		r->loc = loc;
		r->nspace = nspace;
		r->list = list;
		r->patRepId = patRepId;
		r->langEl = 0;
		r->pdaRun = 0;
		r->nextBindId = 1;
		r->parse = true;
		return r;
	}

	InputLoc loc;
	Namespace *nspace;
	ConsItemList *list;
	int patRepId;
	LangEl *langEl;
	PdaRun *pdaRun;
	long nextBindId;
	bool parse;

	Constructor *prev, *next;
};

typedef DList<Constructor> ConsList;

struct ParserText
{
	static ParserText *cons( const InputLoc &loc,
			Namespace *nspace, ConsItemList *list )
	{
		ParserText *p = new ParserText;
		p->loc = loc;
		p->nspace = nspace;
		p->list = list;
		p->langEl = 0;
		p->pdaRun = 0;
		p->nextBindId = 1;
		p->parse = true;
		return p;
	}

	InputLoc loc;
	Namespace *nspace;
	ConsItemList *list;
	LangEl *langEl;
	PdaRun *pdaRun;
	long nextBindId;
	bool parse;
	ParserText *prev, *next;
};

typedef DList<ParserText> ParserTextList;

struct Function;

struct IterDef
{
	enum Type { Tree, Child, RevChild, Repeat, RevRepeat, User };

	IterDef( Type type, Function *func );
	IterDef( Type type );

	Type type;

	Function *func;
	bool useFuncId;
	bool useSearchUT;

	Code inCreateWV;
	Code inCreateWC;
	Code inDestroy;
	Code inAdvance;

	Code inGetCurR;
	Code inGetCurWC;
	Code inSetCurWC;

	Code inRefFromCur;
};

struct CmpIterDef
{
	static int compare( const IterDef &id1, const IterDef &id2 )
	{
		if ( id1.type < id2.type )
			return -1;
		else if ( id1.type > id2.type )
			return 1;
		else if ( id1.type == IterDef::User ) {
			if ( id1.func < id2.func )
				return -1;
			else if ( id1.func > id2.func )
				return 1;
		}
			
		return 0;
	}
};

typedef AvlSet<IterDef, CmpIterDef> IterDefSet;
typedef AvlSetEl<IterDef> IterDefSetEl;


/*
 * Unique Types.
 */

/*
 *  type_ref -> qualified_name
 *  type_ref -> '*' type_ref
 *  type_ref -> '&' type_ref
 *  type_ref -> list type_ref type_ref
 *  type_ref -> map type_ref type_ref
 *  type_ref -> vector type_ref
 *  type_ref -> parser type_ref
 *  type_ref -> iter_tree type_ref
 *  type_ref -> iter_child type_ref
 *  type_ref -> iter_revchild type_ref
 *  type_ref -> iter_repeat type_ref
 *  type_ref -> iter_revrepeat type_ref
 *  type_ref -> iter_user type_ref
 *
 *  type -> nil
 *  type -> def term 
 *  type -> def nonterm
 *  type -> '*' type
 *  type -> '&' type
 *  type -> list type 
 *  type -> map type type
 *  type -> vector type
 *  type -> parser type
 *  type -> iter_tree type
 *  type -> iter_child type
 *  type -> iter_revchild type
 *  type -> iter_repeat type
 *  type -> iter_revrepeat type
 *  type -> iter_user type
 */

struct UniqueType : public AvlTreeEl<UniqueType>
{
	UniqueType( int typeId ) :
		typeId(typeId), 
		langEl(0), 
		iterDef(0) {}

	UniqueType( int typeId, LangEl *langEl ) :
		typeId(typeId),
		langEl(langEl),
		iterDef(0) {}

	UniqueType( int typeId, IterDef *iterDef ) :
		typeId(typeId),
		langEl(langEl),
		iterDef(iterDef) {}

	int typeId;
	LangEl *langEl;
	IterDef *iterDef;
};

ObjectDef *objDefFromUT( Compiler *pd, UniqueType *ut );

struct CmpUniqueType
{
	static int compare( const UniqueType &ut1, const UniqueType &ut2 );
};

typedef AvlBasic< UniqueType, CmpUniqueType > UniqueTypeMap;

enum RepeatType {
	RepeatNone = 1,
	RepeatRepeat,
	RepeatList,
	RepeatOpt,
};

/*
 * Repeat types.
 */

struct UniqueRepeat
	: public AvlTreeEl<UniqueRepeat>
{
	UniqueRepeat( RepeatType repeatType, LangEl *langEl ) :
		repeatType(repeatType),
		langEl(langEl), declLangEl(0) {}

	RepeatType repeatType;
	LangEl *langEl;
	LangEl *declLangEl;
};

struct CmpUniqueRepeat
{
	static int compare( const UniqueRepeat &ut1, const UniqueRepeat &ut2 );
};

typedef AvlBasic< UniqueRepeat, CmpUniqueRepeat > UniqueRepeatMap;

/* 
 * Unique Map Types
 */

struct UniqueMap
	: public AvlTreeEl<UniqueMap>
{
	UniqueMap( UniqueType *key, UniqueType *value ) :
		key(key), value(value), generic(0) {}

	UniqueType *key;
	UniqueType *value;

	GenericType *generic;
};

struct CmpUniqueMap
{
	static int compare( const UniqueMap &ut1, const UniqueMap &ut2 );
};

typedef AvlBasic< UniqueMap, CmpUniqueMap > UniqueMapMap;

/* 
 * Unique List Types
 */

struct UniqueList
	: public AvlTreeEl<UniqueList>
{
	UniqueList( UniqueType *value ) :
		value(value), generic(0) {}

	UniqueType *value;
	GenericType *generic;
};

struct CmpUniqueList
{
	static int compare( const UniqueList &ut1, const UniqueList &ut2 );
};

typedef AvlBasic< UniqueList, CmpUniqueList > UniqueListMap;

/* 
 * Unique Vector Types
 */

struct UniqueVector
	: public AvlTreeEl<UniqueVector>
{
	UniqueVector( UniqueType *value ) :
		value(value), generic(0) {}

	UniqueType *value;
	GenericType *generic;
};

struct CmpUniqueVector
{
	static int compare( const UniqueVector &ut1, const UniqueVector &ut2 );
};

typedef AvlBasic< UniqueVector, CmpUniqueVector > UniqueVectorMap;

/* 
 * Unique Parser Types
 */

struct UniqueParser
	: public AvlTreeEl<UniqueParser>
{
	UniqueParser( UniqueType *parseType ) :
		parseType(parseType), generic(0) {}

	UniqueType *parseType;
	GenericType *generic;
};

struct CmpUniqueParser
{
	static int compare( const UniqueParser &ut1, const UniqueParser &ut2 );
};

typedef AvlBasic< UniqueParser, CmpUniqueParser > UniqueParserMap;

/*
 *
 */

typedef AvlMap< StringVect, int, CmpStrVect > VectorTypeIdMap;
typedef AvlMapEl< StringVect, int > VectorTypeIdMapEl;

typedef Vector<TypeRef*> TypeRefVect;

struct TypeRef
{
	enum Type
	{
		Unspecified,
		Name,
		Literal,
		Iterator,
		Map,
		List,
		Vector,
		Parser,
		Ref,
		Ptr,
	};

	TypeRef()
	:
		type((Type)-1),
		nspaceQual(0),
		pdaLiteral(0),
		iterDef(0),
		typeRef1(0),
		typeRef2(0),
		repeatType(RepeatNone),
		parsedVarRef(0),
		parsedTypeRef(0),
		nspace(0),
		uniqueType(0),
		searchUniqueType(0),
		generic(0)
	{}

	/* Qualification and a type name. These require lookup. */
	static TypeRef *cons( const InputLoc &loc, NamespaceQual *nspaceQual, 
			const String &typeName )
	{
		TypeRef *t = new TypeRef;
		t->type = Name;
		t->loc = loc;
		t->nspaceQual = nspaceQual;
		t->typeName = typeName;
		t->repeatType = RepeatNone;
		return t;
	}

	/* Qualification and a type name. These require lookup. */
	static TypeRef *cons( const InputLoc &loc, NamespaceQual *nspaceQual,
			String typeName, RepeatType repeatType )
	{
		TypeRef *t = cons( loc, nspaceQual, typeName );
		t->repeatType = repeatType;
		return t;
	}

	static TypeRef *cons( const InputLoc &loc, LangVarRef *parsedVarRef,
			NamespaceQual *nspaceQual, String typeName, RepeatType repeatType )
	{
		TypeRef *t = cons( loc, nspaceQual, typeName );
		t->parsedVarRef = parsedVarRef;
		t->repeatType = repeatType;
		return t;
	}

	static TypeRef *cons( const InputLoc &loc, TypeRef *parsedTypeRef,
			NamespaceQual *nspaceQual, String typeName, RepeatType repeatType )
	{
		TypeRef *t = cons( loc, nspaceQual, typeName );
		t->parsedTypeRef = parsedTypeRef;
		t->repeatType = repeatType;
		return t;
	}

	/* Qualification and a type name. These require lookup. */
	static TypeRef *cons( const InputLoc &loc, NamespaceQual *nspaceQual,
			PdaLiteral *pdaLiteral )
	{
		TypeRef *t = new TypeRef;
		t->type = Literal;
		t->loc = loc;
		t->nspaceQual = nspaceQual;
		t->pdaLiteral = pdaLiteral;
		t->repeatType = RepeatNone;
		return t;
	}

	static TypeRef *cons( const InputLoc &loc, TypeRef *parsedTypeRef,
			NamespaceQual *nspaceQual, PdaLiteral *pdaLiteral )
	{
		TypeRef *t = cons( loc, nspaceQual, pdaLiteral );
		t->parsedTypeRef = parsedTypeRef;
		return t;
	}

	/* Qualification and a type name. These require lookup. */
	static TypeRef *cons( const InputLoc &loc, NamespaceQual *nspaceQual,
			PdaLiteral *pdaLiteral, RepeatType repeatType )
	{
		TypeRef *t = cons( loc, nspaceQual, pdaLiteral );
		t->repeatType = repeatType;
		return t;
	}

	static TypeRef *cons( const InputLoc &loc, LangVarRef *parsedVarRef,
			NamespaceQual *nspaceQual, PdaLiteral *pdaLiteral, RepeatType repeatType )
	{
		TypeRef *t = cons( loc, nspaceQual, pdaLiteral );
		t->parsedVarRef = parsedVarRef;
		t->repeatType = repeatType;
		return t;
	}

	static TypeRef *cons( const InputLoc &loc, TypeRef *parsedTypeRef,
			NamespaceQual *nspaceQual, PdaLiteral *pdaLiteral, RepeatType repeatType )
	{
		TypeRef *t = cons( loc, nspaceQual, pdaLiteral );
		t->parsedTypeRef = parsedTypeRef;
		t->repeatType = repeatType;
		return t;
	}

	/* Generics. */
	static TypeRef *cons( const InputLoc &loc, Type type, 
			NamespaceQual *nspaceQual, TypeRef *typeRef1, TypeRef *typeRef2 )
	{
		TypeRef *t = new TypeRef;
		t->type = type;
		t->loc = loc;
		t->nspaceQual = nspaceQual;
		t->typeRef1 = typeRef1;
		t->typeRef2 = typeRef2;
		t->repeatType = RepeatNone;
		return t;
	}
	
	/* Pointers and Refs. */
	static TypeRef *cons( const InputLoc &loc, Type type, TypeRef *typeRef1 )
	{
		TypeRef *t = new TypeRef;
		t->type = type;
		t->loc = loc;
		t->typeRef1 = typeRef1;
		t->repeatType = RepeatNone;
		return t;
	}

	/* Resolution not needed. */

	/* Iterator definition. */
	static TypeRef *cons( const InputLoc &loc, IterDef *iterDef,
			UniqueType *uniqueType, UniqueType *searchUniqueType )
	{
		TypeRef *t = new TypeRef;
		t->type = Iterator;
		t->loc = loc;
		t->iterDef = iterDef;
		t->repeatType = RepeatNone;
		t->uniqueType = uniqueType;
		t->searchUniqueType = searchUniqueType;
		return t;
	}

	/* Unique type is given directly. */
	static TypeRef *cons( const InputLoc &loc, UniqueType *uniqueType )
	{
		TypeRef *t = new TypeRef;
		t->type = Unspecified;
		t->loc = loc;
		t->repeatType = RepeatNone;
		t->uniqueType = uniqueType;
		return t;
	}

	void resolveRepeat( Compiler *pd );

	UniqueType *lookupTypeName( Compiler *pd );
	UniqueType *lookupTypeLiteral( Compiler *pd );
	UniqueType *lookupTypeMap( Compiler *pd );
	UniqueType *lookupTypeList( Compiler *pd );
	UniqueType *lookupTypeVector( Compiler *pd );
	UniqueType *lookupTypeParser( Compiler *pd );
	UniqueType *lookupType( Compiler *pd );
	UniqueType *lookupTypePtr( Compiler *pd );
	UniqueType *lookupTypeRef( Compiler *pd );

	Type type;
	InputLoc loc;
	NamespaceQual *nspaceQual;
	String typeName;
	PdaLiteral *pdaLiteral;
	IterDef *iterDef;
	TypeRef *typeRef1;
	TypeRef *typeRef2;
	RepeatType repeatType;

	/* For pattern and constructor context. */
	LangVarRef *parsedVarRef;
	TypeRef *parsedTypeRef;

	/* Resolved. */
	Namespace *nspace;
	UniqueType *uniqueType;
	UniqueType *searchUniqueType;
	GenericType *generic;
};

typedef DList<ObjectField> ParameterList; 

struct ObjMethod
{
	ObjMethod( UniqueType *returnUT, String name, 
			int opcodeWV, int opcodeWC, int numParams, 
			UniqueType **types, ParameterList *paramList, bool isConst )
	: 
		returnUT(returnUT),
		returnTypeId(0),
		name(name), 
		opcodeWV(opcodeWV),
		opcodeWC(opcodeWC), 
		numParams(numParams),
		paramList(paramList), 
		isConst(isConst),
		funcId(0), 
		useFuncId(false),
		useCallObj(true),
		isCustom(false),
		func(0), 
		iterDef(0)
	{
		this->paramUTs = new UniqueType*[numParams];
		memcpy( this->paramUTs, types, sizeof(UniqueType*)*numParams );
	}

	UniqueType *returnUT;
	long returnTypeId;
	String name;
	long opcodeWV;
	long opcodeWC;
	long numParams;
	UniqueType **paramUTs;
	ParameterList *paramList;
	bool isConst;
	long funcId;
	bool useFuncId;
	bool useCallObj;
	bool isCustom;
	Function *func;
	IterDef *iterDef;
};

typedef AvlMap<String, ObjMethod*, CmpStr> ObjMethodMap;
typedef AvlMapEl<String, ObjMethod*> ObjMethodMapEl;

struct RhsVal
{
	RhsVal( ProdEl *prodEl ) 
	:
		prodEl(prodEl)
	{}

	ProdEl *prodEl;
};

struct ObjectField
{
	ObjectField()
	: 
		typeRef(0),
		context(0),
		scope(0),
		pos(0),
		offset(0),
		beenReferenced(false),
		beenInitialized(false),
		useOffset(true),
		isConst(false), 
		isLhsEl(false), isRhsEl(false), 
		refActive(false),
		isArgv(false),
		isCustom(false),
		isParam(false),
		isRhsGet(false),
		isExport(false),
		dirtyTree(false),
		inGetR( IN_HALT ),
		inGetWC( IN_HALT ),
		inGetWV( IN_HALT ),
		inSetWC( IN_HALT ),
		inSetWV( IN_HALT )
	{}

	static ObjectField *cons( const InputLoc &loc, TypeRef *typeRef, const String &name )
	{
		ObjectField *c = new ObjectField;
		c->loc = loc;
		c->typeRef = typeRef;
		c->name = name;
		return c;
	}
	
	InputLoc loc;
	TypeRef *typeRef;
	String name;
	Context *context;
	ObjNameScope *scope;
	long pos;
	long offset;
	bool beenReferenced;
	bool beenInitialized;
	bool useOffset;
	bool isConst;
	bool isLhsEl;
	bool isRhsEl;
	bool refActive;
	bool isArgv;
	bool isCustom;
	bool isParam;
	bool isRhsGet;
	bool isExport;
	
	/* True if some aspect of the tree has possibly been written to. This does
	 * not include attributes. This is here so we can optimize the storage of
	 * old lhs vars. If only a lhs attribute changes we don't need to preserve
	 * the original for backtracking. */
	bool dirtyTree;

	Vector<RhsVal> rhsVal;

	Code inGetR;
	Code inGetWC;
	Code inGetWV;
	Code inSetWC;
	Code inSetWV;

	ObjectField *prev, *next;
};

typedef AvlMap<String, ObjectField*, CmpStr> ObjFieldMap;
typedef AvlMapEl<String, ObjectField*> ObjFieldMapEl;

typedef DListVal<ObjectField*> ObjFieldList;

typedef DList<ObjectField> ParameterList; 

struct TemplateType;

/* Tree of name scopes for an object def. All of the object fields inside this
 * tree live in one object def. This is used for scoping names in functions. */
struct ObjNameScope
{
	ObjNameScope()
	:
		owner(0),
		parentScope(0),
		childIter(0)
	{}

	ObjectDef *owner;
	ObjFieldMap *objFieldMap;	

	ObjNameScope *parentScope;
	DList<ObjNameScope> children;

	/* For iteration after declaration. */
	ObjNameScope *childIter;

	ObjNameScope *prev, *next;

	int depth()
	{
		int depth = 0;
		ObjNameScope *scope = this;
		while ( scope != 0 ) {
			depth += 1;
			scope = scope->parentScope;
		}
		return depth;
	}

	ObjectField *findField( const String &name ) const;
};

struct ObjectDef
{
	enum Type {
		UserType,
		FrameType,
		IterType,
		BuiltinType
	};

	ObjectDef()
	:
		nextOffset(0),
		firstNonTree(0)
	{}

	static ObjectDef *cons( Type type, String name, int id )
	{
		ObjectDef *o = new ObjectDef;

		o->type = type;
		o->name = name;
		o->id = id;

		o->rootScope = o->curScope = new ObjNameScope;
		o->rootScope->owner = o;
		o->rootScope->objFieldMap = new ObjFieldMap;

		o->objFieldList = new ObjFieldList;
		o->objMethodMap = new ObjMethodMap;

		return o;
	}

	Type type;
	String name;
	ObjFieldList *objFieldList;
	ObjMethodMap *objMethodMap;	

	ObjNameScope *rootScope;
	ObjNameScope *curScope;

	void pushScope();
	void popScope();
	void iterPushScope();
	void iterPopScope();

	long id;
	long nextOffset;
	long firstNonTree;

	void referenceField( Compiler *pd, ObjectField *field );
	void initField( Compiler *pd, ObjectField *field );
	void createCode( Compiler *pd, CodeVect &code );
	ObjectField *checkRedecl( const String &name );
	ObjMethod *findMethod( const String &name ) const;
	ObjectField *findFieldInScope( const String &name, const ObjNameScope *inScope ) const;
	void insertField( const String &name, ObjectField *value );
	void resolve( Compiler *pd );
	ObjectField *findFieldNum( long offset );

	long size() { return nextOffset; }
	long sizeTrees() { return firstNonTree; }
};

struct CallArg
{
	CallArg( LangExpr *expr )
		: expr(expr), exprUT(0), offTmp(-1), offQualRef(-1) {}

	LangExpr *expr;
	UniqueType *exprUT;
	int offTmp;
	int offQualRef;
};

typedef Vector<LangExpr*> ExprVect;
typedef Vector<CallArg*> CallArgVect;
typedef Vector<String> StringVect;

struct FieldInit
{
	static FieldInit *cons( const InputLoc &loc, String name, LangExpr *expr )
	{
		FieldInit *fi = new FieldInit;
		fi->loc = loc;
		fi->name = name;
		fi->expr = expr;
		return fi;
	}

	InputLoc loc;
	String name;
	LangExpr *expr;

	UniqueType *exprUT;
};

typedef Vector<FieldInit*> FieldInitVect;

struct VarRefLookup
{
	VarRefLookup( int lastPtrInQual, int firstConstPart, ObjectDef *inObject ) :
		lastPtrInQual(lastPtrInQual), 
		firstConstPart(firstConstPart),
		inObject(inObject),
		objField(0), 
		objMethod(0), 
		uniqueType(0),
		iterSearchUT(0)
	{}

	int lastPtrInQual;
	int firstConstPart;
	ObjectDef *inObject;
	ObjectField *objField;
	ObjMethod *objMethod;
	UniqueType *uniqueType;
	UniqueType *iterSearchUT;
};

struct QualItem
{
	enum Form { Dot, Arrow };

	QualItem( Form form, const InputLoc &loc, const String &data )
		: form(form), loc(loc), data(data) {}

	Form form;
	InputLoc loc;
	String data;
};

typedef Vector<QualItem> QualItemVect;

struct LangVarRef
{
	LangVarRef()
	:
		qual(0)
	{}
	
	static LangVarRef *cons( const InputLoc &loc, ObjNameScope *scope, QualItemVect *qual, const String &name )
	{
		LangVarRef *l = new LangVarRef;
		l->loc = loc;
		l->scope = scope;
		l->qual = qual;
		l->name = name;
		return l;
	}

	static LangVarRef *cons( const InputLoc &loc, ObjNameScope *scope, const String &name )
	{
		return cons( loc, scope, new QualItemVect, name );
	}

	void resolve( Compiler *pd ) const;
	UniqueType *lookup( Compiler *pd ) const;

	UniqueType *loadField( Compiler *pd, CodeVect &code, ObjectDef *inObject,
			ObjectField *el, bool forWriting, bool revert ) const;

	VarRefLookup lookupMethod( Compiler *pd ) const;
	VarRefLookup lookupField( Compiler *pd ) const;

	VarRefLookup lookupQualification( Compiler *pd, ObjNameScope *rootScope ) const;
	VarRefLookup lookupObj( Compiler *pd ) const;

	bool isCustom( Compiler *pd ) const;
	bool isLocalRef( Compiler *pd ) const;
	bool isContextRef( Compiler *pd ) const;
	void loadQualification( Compiler *pd, CodeVect &code, ObjNameScope *rootScope, 
			int lastPtrInQual, bool forWriting, bool revert ) const;
	void loadCustom( Compiler *pd, CodeVect &code, 
			int lastPtrInQual, bool forWriting ) const;
	void loadLocalObj( Compiler *pd, CodeVect &code, 
			int lastPtrInQual, bool forWriting ) const;
	void loadContextObj( Compiler *pd, CodeVect &code, int lastPtrInQual, bool forWriting ) const;
	void loadGlobalObj( Compiler *pd, CodeVect &code, 
			int lastPtrInQual, bool forWriting ) const;
	void loadObj( Compiler *pd, CodeVect &code, int lastPtrInQual, bool forWriting ) const;
	void canTakeRef( Compiler *pd, VarRefLookup &lookup ) const;
	bool canTakeRefTest( Compiler *pd, VarRefLookup &lookup ) const;

	void setFieldIter( Compiler *pd, CodeVect &code, ObjectDef *inObject,
			ObjectField *objField, UniqueType *objUT, UniqueType *exprType,
			bool revert ) const;
	void setFieldSearch( Compiler *pd, CodeVect &code, 
			ObjectDef *inObject, UniqueType *exprType ) const;
	void setField( Compiler *pd, CodeVect &code, ObjectDef *inObject,
			ObjectField *el, UniqueType *exprUT, bool revert ) const;

	void assignValue( Compiler *pd, CodeVect &code, UniqueType *exprUT ) const;
	ObjectField **evaluateArgs( Compiler *pd, CodeVect &code, 
			VarRefLookup &lookup, CallArgVect *args ) const;
	void callOperation( Compiler *pd, CodeVect &code, VarRefLookup &lookup ) const;
	UniqueType *evaluateCall( Compiler *pd, CodeVect &code, CallArgVect *args );
	UniqueType *evaluate( Compiler *pd, CodeVect &code, bool forWriting = false ) const;
	ObjectField *evaluateRef( Compiler *pd, CodeVect &code, long pushCount ) const;
	ObjectField *preEvaluateRef( Compiler *pd, CodeVect &code ) const;
	void resetActiveRefs( Compiler *pd, VarRefLookup &lookup, ObjectField **paramRefs ) const;
	long loadQualificationRefs( Compiler *pd, CodeVect &code ) const;
	void popRefQuals( Compiler *pd, CodeVect &code, 
			VarRefLookup &lookup, CallArgVect *args ) const;

	InputLoc loc;
	ObjNameScope *scope;
	QualItemVect *qual;
	String name;
};

struct LangTerm
{
	enum Type {
		VarRefType,
		MethodCallType,
		NumberType,
		StringType,
		MatchType,
		NewType,
		ConstructType,
		TypeIdType,
		SearchType,
		NilType,
		TrueType,
		FalseType,
		ParseType,
		ParseStopType,
		SendType,
		MakeTreeType,
		MakeTokenType,
		EmbedStringType,
		CastType,
	};

	LangTerm()
	:
		generic(0),
		parserText(0)
	{}

	static LangTerm *cons( const InputLoc &loc, Type type, LangVarRef *varRef )
	{
		LangTerm *t = new LangTerm;
		t->loc = loc;
		t->type = type;
		t->varRef = varRef;
		return t;
	}

	static LangTerm *cons( const InputLoc &loc, LangVarRef *varRef, CallArgVect *args )
	{
		LangTerm *t = new LangTerm;
		t->loc = loc;
		t->type = MethodCallType;
		t->varRef = varRef;
		t->args = args;
		return t;
	}

	static LangTerm *cons( const InputLoc &loc, Type type, CallArgVect *args )
	{
		LangTerm *t = new LangTerm;
		t->loc = loc;
		t->type = type;
		t->args = args;
		return t;
	}

	static LangTerm *cons( const InputLoc &loc, Type type, String data )
	{
		LangTerm *t = new LangTerm;
		t->loc = loc;
		t->type = type;
		t->varRef = 0;
		t->data = data;
		return t;
	}

	static LangTerm *cons( const InputLoc &loc, Type type )
	{
		LangTerm *t = new LangTerm;
		t->loc = loc;
		t->type = type;
		t->varRef = 0;
		t->typeRef = 0;
		return t;
	}

	static LangTerm *cons( const InputLoc &loc, Type type, TypeRef *typeRef )
	{
		LangTerm *t = new LangTerm;
		t->loc = loc;
		t->type = type;
		t->varRef = 0;
		t->typeRef = typeRef;
		return t;
	}

	static LangTerm *cons( const InputLoc &loc, Type type, TypeRef *typeRef, LangExpr *langExpr )
	{
		LangTerm *t = new LangTerm;
		t->loc = loc;
		t->type = type;
		t->varRef = 0;
		t->typeRef = typeRef;
		t->expr = langExpr;
		return t;
	}

	static LangTerm *cons( const InputLoc &loc, Type type, LangVarRef *varRef, Pattern *pattern )
	{
		LangTerm *t = new LangTerm;
		t->loc = loc;
		t->type = type;
		t->varRef = varRef;
		t->pattern = pattern;
		return t;
	}

	static LangTerm *cons( const InputLoc &loc, Type type, TypeRef *typeRef, LangVarRef *varRef )
	{
		LangTerm *t = new LangTerm;
		t->loc = loc;
		t->type = type;
		t->varRef = varRef;
		t->typeRef = typeRef;
		return t;
	}

	static LangTerm *cons( const InputLoc &loc, Type type, LangVarRef *varRef, ObjectField *objField,
			TypeRef *typeRef, FieldInitVect *fieldInitArgs, Constructor *constructor )
	{
		LangTerm *t = new LangTerm;
		t->loc = loc;
		t->type = type;
		t->varRef = varRef;
		t->objField = objField;
		t->typeRef = typeRef;
		t->fieldInitArgs = fieldInitArgs;
		t->constructor = constructor;
		return t;
	}

	static LangTerm *cons( const InputLoc &loc, Type type, LangVarRef *varRef, ObjectField *objField,
			TypeRef *typeRef, FieldInitVect *fieldInitArgs, Constructor *constructor, 
			ParserText *parserText )
	{
		LangTerm *t = new LangTerm;
		t->loc = loc;
		t->type = type;
		t->varRef = varRef;
		t->objField = objField;
		t->typeRef = typeRef;
		t->fieldInitArgs = fieldInitArgs;
		t->constructor = constructor;
		t->parserText = parserText;
		return t;
	}

	static LangTerm *cons( const InputLoc &loc, Type type, LangExpr *expr )
	{
		LangTerm *t = new LangTerm;
		t->loc = loc;
		t->type = type;
		t->expr = expr;
		return t;
	}
	
	static LangTerm *cons( const InputLoc &loc, ConsItemList *consItemList )
	{
		LangTerm *t = new LangTerm;
		t->loc = loc;
		t->type = EmbedStringType;
		t->consItemList = consItemList;
		return t;
	}

	static LangTerm *cons( const InputLoc &loc, Type type, LangVarRef *varRef, ParserText *parserText ) 
	{
		LangTerm *s = new LangTerm;
		s->loc = loc;
		s->type = type;
		s->varRef = varRef;
		s->parserText = parserText;
		return s;
	}
	
	static LangTerm *consSend( const InputLoc &loc, LangVarRef *varRef, ParserText *parserText, bool eof ) 
	{
		LangTerm *s = new LangTerm;
		s->loc = loc;
		s->type = SendType;
		s->varRef = varRef;
		s->parserText = parserText;
		s->eof = eof;
		return s;
	}
	
	void resolve( Compiler *pd );

	UniqueType *evaluateNew( Compiler *pd, CodeVect &code ) const;
	UniqueType *evaluateConstruct( Compiler *pd, CodeVect &code ) const;
	void parseFrag( Compiler *pd, CodeVect &code, int stopId ) const;
	UniqueType *evaluateParse( Compiler *pd, CodeVect &code, bool stop ) const;
	void evaluateSendStream( Compiler *pd, CodeVect &code ) const;
	void evaluateSendParser( Compiler *pd, CodeVect &code ) const;
	UniqueType *evaluateSend( Compiler *pd, CodeVect &code ) const;
	UniqueType *evaluateMatch( Compiler *pd, CodeVect &code ) const;
	UniqueType *evaluate( Compiler *pd, CodeVect &code ) const;
	void assignFieldArgs( Compiler *pd, CodeVect &code, UniqueType *replUT ) const;
	UniqueType *evaluateMakeToken( Compiler *pd, CodeVect &code ) const;
	UniqueType *evaluateMakeTree( Compiler *pd, CodeVect &code ) const;
	UniqueType *evaluateEmbedString( Compiler *pd, CodeVect &code ) const;
	UniqueType *evaluateSearch( Compiler *pd, CodeVect &code ) const;
	UniqueType *evaluateCast( Compiler *pd, CodeVect &code ) const;

	InputLoc loc;
	Type type;
	LangVarRef *varRef;
	CallArgVect *args;
	NamespaceQual *nspaceQual;
	String data;
	ObjectField *objField;
	TypeRef *typeRef;
	Pattern *pattern;
	FieldInitVect *fieldInitArgs;
	GenericType *generic;
	Constructor *constructor;
	ParserText *parserText;
	LangExpr *expr;
	ConsItemList *consItemList;
	bool eof;
};

struct LangExpr
{
	enum Type {
		BinaryType,
		UnaryType,
		TermType
	};

	static LangExpr *cons( const InputLoc &loc, LangExpr *left, char op, LangExpr *right )
	{
		LangExpr *e = new LangExpr;
		e->loc = loc;
		e->type = BinaryType;
		e->left = left;
		e->op = op;
		e->right = right;
		return e;
	}

	static LangExpr *cons( const InputLoc &loc, char op, LangExpr *right )
	{
		LangExpr *e = new LangExpr;
		e->loc = loc;
		e->type = UnaryType;
		e->left = 0;
		e->op = op;
		e->right =right;
		return e;
	}

	static LangExpr *cons( LangTerm *term )
	{
		LangExpr *e = new LangExpr;
		e->type = TermType;
		e->term = term;
		return e;
	}

	void resolve( Compiler *pd ) const;

	UniqueType *evaluate( Compiler *pd, CodeVect &code ) const;
	bool canTakeRefTest( Compiler *pd ) const;

	InputLoc loc;
	Type type;
	LangExpr *left;
	char op;
	LangExpr *right;
	LangTerm *term;
};

struct LangStmt;
typedef DList<LangStmt> StmtList;

struct LangIterCall
{
	enum Type {
		VarRef,
		IterCall,
		Expr
	};

	LangIterCall()
	:
		langTerm(0),
		langExpr(0)
	{}

	static LangIterCall *cons( Type type, LangTerm *langTerm )
	{
		LangIterCall *iterCall = new LangIterCall;
		iterCall->type = type;
		iterCall->langTerm = langTerm;
		return iterCall;
	}

	static LangIterCall *cons( Type type, LangExpr *langExpr )
	{
		LangIterCall *iterCall = new LangIterCall;
		iterCall->type = type;
		iterCall->langExpr = langExpr;
		return iterCall;
	}

	void resolve( Compiler *pd ) const;

	Type type;
	LangTerm *langTerm;
	LangExpr *langExpr;
};

struct LangStmt
{
	enum Type {
		AssignType,
		PrintType,
		PrintXMLACType,
		PrintXMLType,
		PrintStreamType,
		ExprType,
		IfType,
		ElseType,
		RejectType,
		WhileType,
		ReturnType,
		YieldType,
		ForIterType,
		BreakType
	};

	LangStmt()
	:
		type((Type)-1),
		varRef(0),
		langTerm(0),
		objField(0),
		typeRef(0),
		expr(0),
		constructor(0),
		parserText(0),
		exprPtrVect(0),
		fieldInitVect(0),
		stmtList(0),
		elsePart(0),
		iterCall(0),
		scope(0),

		/* Normally you don't need to initialize double list pointers, however,
		 * we make use of the next pointer for returning a pair of statements
		 * using one pointer to a LangStmt, so we need to initialize it above. */
		prev(0),
		next(0)
	{}

	static LangStmt *cons( const InputLoc &loc, Type type, FieldInitVect *fieldInitVect )
	{
		LangStmt *s = new LangStmt;
		s->loc = loc;
		s->type = type;
		s->fieldInitVect = fieldInitVect;
		return s;
	}

	static LangStmt *cons( const InputLoc &loc, Type type, CallArgVect *exprPtrVect )
	{
		LangStmt *s = new LangStmt;
		s->loc = loc;
		s->type = type;
		s->exprPtrVect = exprPtrVect;
		return s;
	}
	
	static LangStmt *cons( const InputLoc &loc, Type type, LangExpr *expr )
	{
		LangStmt *s = new LangStmt;
		s->loc = loc;
		s->type = type;
		s->expr = expr;
		return s;
	}

	static LangStmt *cons( Type type, LangVarRef *varRef )
	{
		LangStmt *s = new LangStmt;
		s->type = type;
		s->varRef = varRef;
		return s;
	}

	static LangStmt *cons( const InputLoc &loc, Type type, ObjectField *objField )
	{
		LangStmt *s = new LangStmt;
		s->loc = loc;
		s->type = type;
		s->objField = objField;
		return s;
	}
	
	static LangStmt *cons( const InputLoc &loc, Type type, LangVarRef *varRef, LangExpr *expr )
	{
		LangStmt *s = new LangStmt;
		s->loc = loc;
		s->type = type;
		s->varRef = varRef;
		s->expr = expr;
		return s;
	}
	
	static LangStmt *cons( Type type, LangExpr *expr, StmtList *stmtList )
	{
		LangStmt *s = new LangStmt;
		s->type = type;
		s->expr = expr;
		s->stmtList = stmtList;
		return s;
	}

	static LangStmt *cons( Type type, StmtList *stmtList )
	{
		LangStmt *s = new LangStmt;
		s->type = type;
		s->stmtList = stmtList;
		return s;
	}

	static LangStmt *cons( Type type, LangExpr *expr, StmtList *stmtList, LangStmt *elsePart )
	{
		LangStmt *s = new LangStmt;
		s->type = type;
		s->expr = expr;
		s->stmtList = stmtList;
		s->elsePart = elsePart;
		return s;
	}

	static LangStmt *cons( const InputLoc &loc, Type type )
	{
		LangStmt *s = new LangStmt;
		s->loc = loc;
		s->type = type;
		return s;
	}

	static LangStmt *cons( Type type, LangVarRef *varRef, Constructor *constructor )
	{
		LangStmt *s = new LangStmt;
		s->type = type;
		s->varRef = varRef;
		s->constructor = constructor;
		return s;
	}

	static LangStmt *cons( const InputLoc &loc, Type type, ObjectField *objField,
			TypeRef *typeRef, LangTerm *langTerm, StmtList *stmtList )
	{
		LangStmt *s = new LangStmt;
		s->loc = loc;
		s->type = type;
		s->langTerm = langTerm;
		s->objField = objField;
		s->typeRef = typeRef;
		s->stmtList = stmtList;
		return s;
	}

	static LangStmt *cons( const InputLoc &loc, Type type, ObjectField *objField,
			TypeRef *typeRef, LangIterCall *iterCall, StmtList *stmtList, ObjNameScope *scope )
	{
		LangStmt *s = new LangStmt;
		s->loc = loc;
		s->type = type;
		s->objField = objField;
		s->typeRef = typeRef;
		s->iterCall = iterCall;
		s->stmtList = stmtList;
		s->scope = scope;
		return s;
	}


	static LangStmt *cons( Type type )
	{
		LangStmt *s = new LangStmt;
		s->type = type;
		return s;
	}

	void resolve( Compiler *pd ) const;
	void resolveParserItems( Compiler *pd ) const;

	void chooseDefaultIter( Compiler *pd, LangIterCall *iterCall ) const;
	void compileWhile( Compiler *pd, CodeVect &code ) const;
	void compileForIterBody( Compiler *pd, CodeVect &code, UniqueType *iterUT ) const;
	void compileForIter( Compiler *pd, CodeVect &code ) const;
	void compile( Compiler *pd, CodeVect &code ) const;

	InputLoc loc;
	Type type;
	LangVarRef *varRef;
	LangTerm *langTerm;
	ObjectField *objField;
	TypeRef *typeRef;
	LangExpr *expr;
	Constructor *constructor;
	ParserText *parserText;
	CallArgVect *exprPtrVect;
	FieldInitVect *fieldInitVect;
	StmtList *stmtList;
	/* Either another if, or an else. */
	LangStmt *elsePart;
	String name;
	LangIterCall *iterCall;
	ObjNameScope *scope;

	/* Normally you don't need to initialize double list pointers, however, we
	 * make use of the next pointer for returning a pair of statements using
	 * one pointer to a LangStmt, so we need to initialize it above. */
	LangStmt *prev, *next;
};

struct CodeBlock
{
	CodeBlock() 
	:
		frameId(-1),
		context(0)
	{}

	static CodeBlock *cons( StmtList *stmtList, ObjectDef *localFrame ) 
	{
		CodeBlock *c = new CodeBlock;
		c->stmtList = stmtList;
		c->localFrame = localFrame;
		return c;
	}

	void compile( Compiler *pd, CodeVect &code ) const;
	void resolve( Compiler *pd ) const;

	long frameId;
	StmtList *stmtList;
	ObjectDef *localFrame;
	CharSet trees;
	Iters iters;
	Locals locals;
	Context *context;

	/* Each frame has two versions of 
	 * the code: revert and commit. */
	CodeVect codeWV, codeWC;
};

struct Function
{
	Function()
	:
		paramListSize(0),
		paramUTs(0),
		inContext(0)
	{}

	static Function *cons( TypeRef *typeRef, const String &name, 
			ParameterList *paramList, CodeBlock *codeBlock, 
			int funcId, bool isUserIter, bool exprt )
	{
		Function *f = new Function;

		f->typeRef = typeRef;
		f->name = name;
		f->paramList = paramList;
		f->codeBlock = codeBlock;
		f->funcId = funcId;
		f->isUserIter = isUserIter;
		f->exprt = exprt;

		return f;
	}

	TransBlock *transBlock;
	TypeRef *typeRef;
	String name;
	ParameterList *paramList;
	CodeBlock *codeBlock;
	ObjectDef *localFrame;
	long funcId;
	bool isUserIter;
	long paramListSize;
	UniqueType **paramUTs;
	Context *inContext;
	bool exprt;

	Function *prev, *next;
};

typedef DList<Function> FunctionList;

#endif /* _PARSETREE_H */
