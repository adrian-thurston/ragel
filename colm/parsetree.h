/*
 *  Copyright 2001-2006 Adrian Thurston <thurston@complang.org>
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
#include "colm.h"
#include "avlmap.h"
#include "bstmap.h"
#include "bstset.h"
#include "vector.h"
#include "dlist.h"
#include "dlistval.h"
#include "astring.h"
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

struct NameInst;
struct FsmGraph;
struct RedFsm;
struct FsmTables;
struct FsmRun;
struct ObjectDef;
struct ElementOf;
struct UniqueType;
struct ObjField;
struct TransBlock;
struct CodeBlock;

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

typedef BstSet<char> CharSet;


struct ParseData;
struct TypeRef;

/* Leaf type. */
struct Literal;

/* Tree nodes. */

struct Term;
struct FactorWithAug;
struct FactorWithRep;
struct FactorWithNeg;
struct Factor;
struct Expression;
struct Join;
struct JoinOrLm;
struct TokenRegion;
struct Namespace;
struct TokenDef;
struct TokenDefList;
struct Range;
struct KlangEl;

/* Type of augmentation. Describes locations in the machine. */
enum AugType
{
	/* Transition actions/priorities. */
	at_start,
	at_all,
	at_finish,
	at_leave,

	/* Global error actions. */
	at_start_gbl_error,
	at_all_gbl_error,
	at_final_gbl_error,
	at_not_start_gbl_error,
	at_not_final_gbl_error,
	at_middle_gbl_error,

	/* Local error actions. */
	at_start_local_error,
	at_all_local_error,
	at_final_local_error,
	at_not_start_local_error,
	at_not_final_local_error,
	at_middle_local_error,
	
	/* To State Action embedding. */
	at_start_to_state,
	at_all_to_state,
	at_final_to_state,
	at_not_start_to_state,
	at_not_final_to_state,
	at_middle_to_state,

	/* From State Action embedding. */
	at_start_from_state,
	at_all_from_state,
	at_final_from_state,
	at_not_start_from_state,
	at_not_final_from_state,
	at_middle_from_state,

	/* EOF Action embedding. */
	at_start_eof,
	at_all_eof,
	at_final_eof,
	at_not_start_eof,
	at_not_final_eof,
	at_middle_eof
};

/* IMPORTANT: These must follow the same order as the state augs in AugType
 * since we will be using this to compose AugType. */
enum StateAugType
{
	sat_start = 0,
	sat_all,
	sat_final,
	sat_not_start,
	sat_not_final,
	sat_middle
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
	Label( const InputLoc &loc, const String &data, ObjField *objField )
		: loc(loc), data(data), objField(objField) { }

	InputLoc loc;
	String data;
	ObjField *objField;
};

/* Structure represents an action assigned to some FactorWithAug node. The
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

typedef AvlMap< String, KlangEl*, CmpStr > LiteralDict;
typedef AvlMapEl< String, KlangEl* > LiteralDictEl;

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
struct VarDef
{
	VarDef( const String &name, JoinOrLm *joinOrLm )
		: name(name), joinOrLm(joinOrLm) { }
	
	/* Parse tree traversal. */
	FsmGraph *walk( ParseData *pd );
	void makeNameTree( const InputLoc &loc, ParseData *pd );
	void resolveNameRefs( ParseData *pd );

	String name;
	JoinOrLm *joinOrLm;
};

typedef Vector<String> StringVect;
typedef CmpTable<String, CmpStr> CmpStrVect;

struct NamespaceQual
{
	NamespaceQual( Namespace *declInNspace, TokenRegion *declInRegion ) : 
		cachedNspaceQual(0), declInNspace(declInNspace) {}

	Namespace *cachedNspaceQual;
	Namespace *declInNspace;

	StringVect qualNames;

	Namespace *searchFrom( Namespace *from, StringVect::Iter &qualPart );
	Namespace *getQual( ParseData *pd );
};

struct ReCapture
{
	ReCapture( Action *markEnter, Action *markLeave, ObjField *objField )
		: markEnter(markEnter), markLeave(markLeave), objField(objField)  {}

	Action *markEnter;
	Action *markLeave;
	ObjField *objField;
};

typedef Vector<ReCapture> ReCaptureVect;

struct TokenDef
{
	TokenDef( Join *join, KlangEl *token, InputLoc &semiLoc, 
		int longestMatchId, Namespace *nspace, TokenRegion *tokenRegion )
	: 
		join(join), action(0), token(token), semiLoc(semiLoc), 
		longestMatchId(longestMatchId), inLmSelect(false), 
		nspace(nspace), tokenRegion(tokenRegion) {}

	InputLoc getLoc();
	
	Join *join;
	Action *action;
	KlangEl *token;
	InputLoc semiLoc;

	Action *setActId;
	Action *actOnLast;
	Action *actOnNext;
	Action *actLagBehind;
	int longestMatchId;
	bool inLmSelect;
	Namespace *nspace;
	TokenRegion *tokenRegion;
	ReCaptureVect reCaptureVect;

	TokenDef *prev, *next;
};

/* Declare a new type so that ptreetypes.h need not include dlist.h. */
struct TokenDefList : DList<TokenDef> {};

/* Symbol Map. */
typedef AvlMap< String, KlangEl*, CmpStr > SymbolMap;
typedef AvlMapEl< String, KlangEl* > SymbolMapEl;

typedef Vector<TokenRegion*> RegionVect;

struct TokenRegion
{
	/* Construct with a list of joins */
	TokenRegion( const InputLoc &loc, const String &name, int id, 
			TokenRegion *parentRegion ) : 
		loc(loc), name(name), id(id),
		lmSwitchHandlesError(false), regionNameInst(0),
		parentRegion(parentRegion), defaultTokenDef(0),
		preEofBlock(0) { }

	/* Tree traversal. */
	FsmGraph *walk( ParseData *pd );
	void makeNameTree( ParseData *pd );
	void resolveNameRefs( ParseData *pd );
	void runLongestMatch( ParseData *pd, FsmGraph *graph );
	void transferScannerLeavingActions( FsmGraph *graph );
	Action *newAction( ParseData *pd, const InputLoc &loc, const String &name, 
			InlineList *inlineList );
	void makeActions( ParseData *pd );
	void findName( ParseData *pd );
	void restart( FsmGraph *graph, FsmTrans *trans );

	InputLoc loc;
	TokenDefList tokenDefList;
	String name;
	int id;

	Action *lmActSelect;
	bool lmSwitchHandlesError;

	/* This gets saved off during the name walk. Can save it off because token
	 * regions are referenced once only. */
	NameInst *regionNameInst;

	TokenRegion *parentRegion;
	RegionVect childRegions;

	TokenDef *defaultTokenDef;

	TokenRegion *next, *prev;

	CodeBlock *preEofBlock;
};

typedef DList<TokenRegion> RegionList;
typedef BstSet< TokenRegion*, CmpOrd<TokenRegion*> > RegionSet;

typedef Vector<Namespace*> NamespaceVect;

struct GenericType 
	: public DListEl<GenericType>
{
	GenericType( const String &name, long typeId, long id, 
			KlangEl *langEl, TypeRef *typeArg )
	:
		name(name), typeId(typeId), id(id), langEl(langEl),
		typeArg(typeArg), keyTypeArg(0), 
		utArg(0), keyUT(0),
		objDef(0)
	{}

	const String &getKey() const 
		{ return name; };

	String name;
	long typeId;
	long id;
	KlangEl *langEl;
	TypeRef *typeArg;
	TypeRef *keyTypeArg;
	UniqueType *utArg;
	UniqueType *keyUT;

	ObjectDef *objDef;
};

typedef DList<GenericType> GenericList;

struct UserIter;
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
	GraphDictEl( const String &key, VarDef *value ) 
		: key(key), value(value), isInstance(false) { }

	const String &getKey() { return key; }

	String key;
	VarDef *value;
	bool isInstance;

	/* Location info of graph definition. Points to variable name of assignment. */
	InputLoc loc;
};

typedef AvlTree<GraphDictEl, String, CmpStr> GraphDict;
typedef DList<GraphDictEl> GraphList;

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

	/* Dictionary of symbols within the region. */
	SymbolMap symbolMap;
	GenericList genericList;

	/* Dictionary of graphs. Both instances and non-instances go here. */
	GraphDict graphDict;

	Namespace *parentNamespace;
	NamespaceVect childNamespaces;

	Namespace *next, *prev;
};

typedef DList<Namespace> NamespaceList;
typedef BstSet< Namespace*, CmpOrd<Namespace*> > NamespaceSet;

/* List of Expressions. */
typedef DList<Expression> ExprList;

struct JoinOrLm
{
	enum Type {
		JoinType,
		LongestMatchType
	};

	JoinOrLm( Join *join ) : 
		join(join), type(JoinType) {}
	JoinOrLm( TokenRegion *tokenRegion ) :
		tokenRegion(tokenRegion), type(LongestMatchType) {}

	FsmGraph *walk( ParseData *pd );
	void makeNameTree( ParseData *pd );
	void resolveNameRefs( ParseData *pd );
	
	Join *join;
	TokenRegion *tokenRegion;
	Type type;
};

/*
 * Join
 */
struct Join
{
	/* Construct with the first expression. */
	Join( Expression *expr );

	/* Tree traversal. */
	FsmGraph *walk( ParseData *pd );
	void makeNameTree( ParseData *pd );
	void resolveNameRefs( ParseData *pd );

	/* Data. */
	ExprList exprList;

	Join *context;
	Action *mark;
};

/*
 * Expression
 */
struct Expression
{
	enum Type { 
		OrType,
		IntersectType, 
		SubtractType, 
		StrongSubtractType,
		TermType, 
		BuiltinType
	};

	/* Construct with an expression on the left and a term on the right. */
	Expression( Expression *expression, Term *term, Type type ) : 
		expression(expression), term(term), 
		builtin(builtin), type(type), prev(this), next(this) { }

	/* Construct with only a term. */
	Expression( Term *term ) : 
		expression(0), term(term), builtin(builtin), 
		type(TermType) , prev(this), next(this) { }
	
	/* Construct with a builtin type. */
	Expression( BuiltinMachine builtin ) : 
		expression(0), term(0), builtin(builtin), 
		type(BuiltinType), prev(this), next(this) { }

	~Expression();

	/* Tree traversal. */
	FsmGraph *walk( ParseData *pd, bool lastInSeq = true );
	void makeNameTree( ParseData *pd );
	void resolveNameRefs( ParseData *pd );

	/* Node data. */
	Expression *expression;
	Term *term;
	BuiltinMachine builtin;
	Type type;

	Expression *prev, *next;
};

/*
 * Term
 */
struct Term 
{
	enum Type { 
		ConcatType, 
		RightStartType,
		RightFinishType,
		LeftType,
		FactorWithAugType
	};

	Term( Term *term, FactorWithAug *factorWithAug ) :
		term(term), factorWithAug(factorWithAug), type(ConcatType) { }

	Term( Term *term, FactorWithAug *factorWithAug, Type type ) :
		term(term), factorWithAug(factorWithAug), type(type) { }

	Term( FactorWithAug *factorWithAug ) :
		term(0), factorWithAug(factorWithAug), type(FactorWithAugType) { }
	
	~Term();

	FsmGraph *walk( ParseData *pd, bool lastInSeq = true );
	void makeNameTree( ParseData *pd );
	void resolveNameRefs( ParseData *pd );

	Term *term;
	FactorWithAug *factorWithAug;
	Type type;

	/* Priority descriptor for RightFinish type. */
	PriorDesc priorDescs[2];
};


/* Third level of precedence. Augmenting nodes with actions and priorities. */
struct FactorWithAug
{
	FactorWithAug( FactorWithRep *factorWithRep ) :
		priorDescs(0), factorWithRep(factorWithRep) { }
	~FactorWithAug();

	/* Tree traversal. */
	FsmGraph *walk( ParseData *pd );
	void makeNameTree( ParseData *pd );
	void resolveNameRefs( ParseData *pd );

	void assignActions( ParseData *pd, FsmGraph *graph, int *actionOrd );
	void assignPriorities( FsmGraph *graph, int *priorOrd );

	void assignConditions( FsmGraph *graph );

	/* Actions and priorities assigned to the factor node. */
	Vector<ParserAction> actions;
	Vector<PriorityAug> priorityAugs;
	PriorDesc *priorDescs;
	Vector<Label> labels;
	Vector<EpsilonLink> epsilonLinks;
	Vector<ParserAction> conditions;

	FactorWithRep *factorWithRep;
};

/* Fourth level of precedence. Trailing unary operators. Provide kleen star,
 * optional and plus. */
struct FactorWithRep
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
		FactorWithNegType
	};

	 FactorWithRep( const InputLoc &loc, FactorWithRep *factorWithRep, 
			int lowerRep, int upperRep, Type type ) :
		loc(loc), factorWithRep(factorWithRep), 
		factorWithNeg(0), lowerRep(lowerRep), 
		upperRep(upperRep), type(type) { }
	
	FactorWithRep( const InputLoc &loc, FactorWithNeg *factorWithNeg )
		: loc(loc), factorWithNeg(factorWithNeg), type(FactorWithNegType) { }

	~FactorWithRep();

	/* Tree traversal. */
	FsmGraph *walk( ParseData *pd );
	void makeNameTree( ParseData *pd );
	void resolveNameRefs( ParseData *pd );

	InputLoc loc;
	FactorWithRep *factorWithRep;
	FactorWithNeg *factorWithNeg;
	int lowerRep, upperRep;
	Type type;

	/* Priority descriptor for StarStar type. */
	PriorDesc priorDescs[2];
};

/* Fifth level of precedence. Provides Negation. */
struct FactorWithNeg
{
	enum Type { 
		NegateType, 
		CharNegateType,
		FactorType
	};

	FactorWithNeg( const InputLoc &loc, FactorWithNeg *factorWithNeg, Type type) :
		loc(loc), factorWithNeg(factorWithNeg), factor(0), type(type) { }

	FactorWithNeg( const InputLoc &loc, Factor *factor ) :
		loc(loc), factorWithNeg(0), factor(factor), type(FactorType) { }

	~FactorWithNeg();

	/* Tree traversal. */
	FsmGraph *walk( ParseData *pd );
	void makeNameTree( ParseData *pd );
	void resolveNameRefs( ParseData *pd );

	InputLoc loc;
	FactorWithNeg *factorWithNeg;
	Factor *factor;
	Type type;
};

/*
 * Factor
 */
struct Factor
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

	/* Construct with a literal fsm. */
	Factor( Literal *literal ) :
		literal(literal), type(LiteralType) { }

	/* Construct with a range. */
	Factor( Range *range ) : 
		range(range), type(RangeType) { }
	
	/* Construct with the or part of a regular expression. */
	Factor( ReItem *reItem ) :
		reItem(reItem), type(OrExprType) { }

	/* Construct with a regular expression. */
	Factor( RegExpr *regExp ) :
		regExp(regExp), type(RegExprType) { }

	/* Construct with a reference to a var def. */
	Factor( const InputLoc &loc, VarDef *varDef ) :
		loc(loc), varDef(varDef), type(ReferenceType) {}

	/* Construct with a parenthesized join. */
	Factor( Join *join ) :
		join(join), type(ParenType) {}
	
	/* Cleanup. */
	~Factor();

	/* Tree traversal. */
	FsmGraph *walk( ParseData *pd );
	void makeNameTree( ParseData *pd );
	void resolveNameRefs( ParseData *pd );

	InputLoc loc;
	Literal *literal;
	Range *range;
	ReItem *reItem;
	RegExpr *regExp;
	VarDef *varDef;
	Join *join;
	int lower, upper;
	Type type;
};

/* A range machine. Only ever composed of two literals. */
struct Range
{
	Range( Literal *lowerLit, Literal *upperLit ) 
		: lowerLit(lowerLit), upperLit(upperLit) { }

	~Range();
	FsmGraph *walk( ParseData *pd );
	bool verifyRangeFsm( FsmGraph *rangeEnd );

	Literal *lowerLit;
	Literal *upperLit;
};

/* Some literal machine. Can be a number or literal string. */
struct Literal
{
	enum LiteralType { Number, LitString };

	Literal( const InputLoc &loc, const String &literal, LiteralType type )
		: loc(loc), literal(literal), type(type) { }

	FsmGraph *walk( ParseData *pd );
	
	InputLoc loc;
	String literal;
	LiteralType type;
};

/* Regular expression. */
struct RegExpr
{
	enum RegExpType { RecurseItem, Empty };

	/* Constructors. */
	RegExpr() : 
		type(Empty), caseInsensitive(false) { }
	RegExpr(RegExpr *regExp, ReItem *item) : 
		regExp(regExp), item(item), 
		type(RecurseItem), caseInsensitive(false) { }

	~RegExpr();
	FsmGraph *walk( ParseData *pd, RegExpr *rootRegex );

	RegExpr *regExp;
	ReItem *item;
	RegExpType type;
	bool caseInsensitive;
};

/* An item in a regular expression. */
struct ReItem
{
	enum ReItemType { Data, Dot, OrBlock, NegOrBlock };
	
	ReItem( const InputLoc &loc, const String &data ) 
		: loc(loc), data(data), star(false), type(Data) { }
	ReItem( const InputLoc &loc, ReItemType type )
		: loc(loc), star(false), type(type) { }
	ReItem( const InputLoc &loc, ReOrBlock *orBlock, ReItemType type )
		: loc(loc), orBlock(orBlock), star(false), type(type) { }

	~ReItem();
	FsmGraph *walk( ParseData *pd, RegExpr *rootRegex );

	InputLoc loc;
	String data;
	ReOrBlock *orBlock;
	bool star;
	ReItemType type;
};

/* An or block item. */
struct ReOrBlock
{
	enum ReOrBlockType { RecurseItem, Empty };

	/* Constructors. */
	ReOrBlock()
		: type(Empty) { }
	ReOrBlock(ReOrBlock *orBlock, ReOrItem *item)
		: orBlock(orBlock), item(item), type(RecurseItem) { }

	~ReOrBlock();
	FsmGraph *walk( ParseData *pd, RegExpr *rootRegex );
	
	ReOrBlock *orBlock;
	ReOrItem *item;
	ReOrBlockType type;
};

/* An item in an or block. */
struct ReOrItem
{
	enum ReOrItemType { Data, Range };

	ReOrItem( const InputLoc &loc, const String &data ) 
		: loc(loc), data(data), type(Data) {}
	ReOrItem( const InputLoc &loc, char lower, char upper )
		: loc(loc), lower(lower), upper(upper), type(Range) { }

	FsmGraph *walk( ParseData *pd, RegExpr *rootRegex );

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

	InlineItem( const InputLoc &loc, const String &data, Type type ) : 
		loc(loc), data(data), nameRef(0), children(0), type(type) { }

	InlineItem( const InputLoc &loc, NameRef *nameRef, Type type ) : 
		loc(loc), nameRef(nameRef), children(0), type(type) { }

	InlineItem( const InputLoc &loc, TokenRegion *tokenRegion, 
		TokenDef *longestMatchPart, Type type ) : loc(loc),
		nameRef(0), children(0), tokenRegion(tokenRegion),
		longestMatchPart(longestMatchPart), type(type) { } 

	InlineItem( const InputLoc &loc, NameInst *nameTarg, Type type ) : 
		loc(loc), nameRef(0), nameTarg(nameTarg), children(0),
		type(type) { }

	InlineItem( const InputLoc &loc, Type type ) : 
		loc(loc), nameRef(0), children(0), type(type) { }
	
	InputLoc loc;
	String data;
	NameRef *nameRef;
	NameInst *nameTarg;
	InlineList *children;
	TokenRegion *tokenRegion;
	TokenDef *longestMatchPart;
	Type type;

	InlineItem *prev, *next;
};

/* Normally this would be atypedef, but that would entail including DList from
 * ptreetypes, which should be just typedef forwards. */
struct InlineList : public DList<InlineItem> { };

struct PdaFactor;
struct LangVarRef;
struct ObjField;

struct PatternItem
{
	enum Type { 
		FactorType,
		InputText
	};

	PatternItem( const InputLoc &loc, const String &data, Type type ) : 
			loc(loc), factor(0), data(data), type(type), region(0), 
			varRef(0), bindId(0) {}

	PatternItem( const InputLoc &loc, PdaFactor *factor, Type type ) : 
			loc(loc), factor(factor), type(type), region(0), 
			varRef(0), bindId(0) {}

	InputLoc loc;
	PdaFactor *factor;
	String data;
	Type type;
	TokenRegion *region;
	LangVarRef *varRef;
	long bindId;

	PatternItem *prev, *next;
};

struct LangExpr;
typedef DList<PatternItem> PatternItemList;

struct ReplItem
{
	enum Type { 
		InputText, 
		ExprType,
		FactorType
	};

	ReplItem( const InputLoc &loc, Type type, const String &data ) : 
		loc(loc), type(type), data(data), expr(0), bindId(0) {}

	ReplItem( const InputLoc &loc, Type type, LangExpr *expr ) : 
		loc(loc), type(type), expr(expr), bindId(0) {}

	ReplItem( const InputLoc &loc, Type type, PdaFactor *factor ) : 
		loc(loc), type(type), expr(expr), factor(factor), bindId(0) {}

	InputLoc loc;
	Type type;
	String data;
	LangExpr *expr;
	KlangEl *langEl;
	PdaFactor *factor;
	long bindId;

	ReplItem *prev, *next;
};

typedef DList<ReplItem> ReplItemList;

struct PdaRun;

struct Pattern
{
	Pattern( const InputLoc &loc, Namespace *nspace, TokenRegion *region, 
			PatternItemList *list, int patRepId ) : 
		loc(loc), nspace(nspace), region(region), list(list), patRepId(patRepId), 
		langEl(0), pdaRun(0), nextBindId(1) {}
	
	InputLoc loc;
	Namespace *nspace;
	TokenRegion *region;
	PatternItemList *list;
	long patRepId;
	KlangEl *langEl;
	PdaRun *pdaRun;
	long nextBindId;

	Pattern *prev, *next;
};

typedef DList<Pattern> PatternList;

struct Replacement
{
	Replacement( const InputLoc &loc, Namespace *nspace, 
			TokenRegion *region, ReplItemList *list, int patRepId ) :
		loc(loc), nspace(nspace), region(region), list(list), 
		patRepId(patRepId), langEl(0), pdaRun(0), nextBindId(1), parse(true) {}

	InputLoc loc;
	Namespace *nspace;
	TokenRegion *region;
	ReplItemList *list;
	int patRepId;
	KlangEl *langEl;
	PdaRun *pdaRun;
	long nextBindId;
	bool parse;

	Replacement *prev, *next;
};

typedef DList<Replacement> ReplList;

struct AccumText
{
	AccumText( const InputLoc &loc, Namespace *nspace, 
			TokenRegion *region, ReplItemList *list ) :
		loc(loc), nspace(nspace), region(region), list(list), 
		langEl(0), pdaRun(0), nextBindId(1), parse(true) {}

	InputLoc loc;
	Namespace *nspace;
	TokenRegion *region;
	ReplItemList *list;
	KlangEl *langEl;
	PdaRun *pdaRun;
	long nextBindId;
	bool parse;

	AccumText *prev, *next;
};

typedef DList<AccumText> AccumTextList;

struct UserIter;
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
 * Language features.
 */

struct UniqueType : public AvlTreeEl<UniqueType>
{
	UniqueType( int typeId ) :
		typeId(typeId), 
		langEl(0), 
		iterDef(0) {}

	UniqueType( int typeId, KlangEl *langEl ) :
		typeId(typeId),
		langEl(langEl),
		iterDef(0) {}

	UniqueType( int typeId, IterDef *iterDef ) :
		typeId(typeId),
		langEl(langEl),
		iterDef(iterDef) {}

	int typeId;
	KlangEl *langEl;
	IterDef *iterDef;
};

struct CmpUniqueType
{
	static int compare( const UniqueType &ut1, const UniqueType &ut2 );
};


typedef AvlBasic< UniqueType, CmpUniqueType > UniqueTypeMap;

typedef AvlMap< StringVect, int, CmpStrVect > VectorTypeIdMap;
typedef AvlMapEl< StringVect, int > VectorTypeIdMapEl;

typedef Vector<TypeRef*> TypeRefVect;

enum RepeatType {
	RepeatRepeat,
	RepeatList,
	RepeatOpt,
	RepeatNone
};

struct TypeRef
{
	/* Qualification and a type name. These require lookup. */
	TypeRef( const InputLoc &loc, NamespaceQual *nspaceQual, String typeName ) :
		loc(loc), nspaceQual(nspaceQual), typeName(typeName), iterDef(0),
		searchTypeRef(0), factor(0),
		isPtr(false), isRef(false), repeatType(RepeatNone),
		uniqueType(0) {}

	/* Iterator definition. */
	TypeRef( const InputLoc &loc, IterDef *iterDef, TypeRef *searchTypeRef ) :
		loc(loc), iterDef(iterDef), searchTypeRef(searchTypeRef), factor(0),
		isPtr(false), isRef(false), repeatType(RepeatNone),
		uniqueType(0) {}

	/* Unique type is given directly. */
	TypeRef( const InputLoc &loc, UniqueType *uniqueType ) :
		loc(loc), nspaceQual(0), iterDef(0), searchTypeRef(0), factor(0),
		isPtr(false), isRef(false), repeatType(RepeatNone),
		uniqueType(uniqueType) {}

	/* A factor in a pattern. In the case of matches we need a type ref at
	 * parse time, but factors have not been resolved yet, so this allows us
	 * to do it on demand. */
	TypeRef( const InputLoc &loc, PdaFactor *factor ) :
		loc(loc), nspaceQual(0), iterDef(0), searchTypeRef(0), factor(factor),
		isPtr(false), isRef(false), repeatType(RepeatNone),
		uniqueType(0) {}

	void analyze( ParseData *pd ) const;
	UniqueType *lookupType( ParseData *pd );

	InputLoc loc;
	NamespaceQual *nspaceQual;
	String typeName;
	IterDef *iterDef;
	TypeRef *searchTypeRef;
	PdaFactor *factor;
	bool isPtr;
	bool isRef;
	RepeatType repeatType;

private:
	UniqueType *lookupTypePart( ParseData *pd, NamespaceQual *nspaceQual, 
			const String &name );
	UniqueType *uniqueType;
};

typedef DList<ObjField> ParameterList; 

struct ObjMethod
{
	ObjMethod( UniqueType *returnUT, String name, 
			int opcodeWV, int opcodeWC, int numParams, 
			UniqueType **types, ParameterList *paramList, bool isConst )
		: 
			returnUT(returnUT),
			returnTypeId(0), name(name), 
			opcodeWV(opcodeWV), opcodeWC(opcodeWC), 
			numParams(numParams), paramList(paramList), 
			isConst(isConst), funcId(0), 
			useFuncId(false), useCallObj(true), func(0), 
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
	Function *func;
	IterDef *iterDef;
};

typedef AvlMap<String, ObjMethod*, CmpStr> ObjMethodMap;
typedef AvlMapEl<String, ObjMethod*> ObjMethodMapEl;

struct ObjField
{
	ObjField( const InputLoc &loc, TypeRef *typeRef, const String &name ) : 
		loc(loc), typeRef(typeRef), name(name), pos(0), offset(0),
		beenReferenced(false),
		beenInitialized(false),
		useOffset(true),
		isConst(false), 
		isLhsEl(false), isRhsEl(false), 
		refActive(false),
		isArgv1(false),
		dirtyTree(false),
		inGetR( IN_HALT ),
		inGetWC( IN_HALT ),
		inGetWV( IN_HALT ),
		inSetWC( IN_HALT ),
		inSetWV( IN_HALT )
		{}
	
	InputLoc loc;
	TypeRef *typeRef;
	String name;
	long pos;
	long offset;
	bool beenReferenced;
	bool beenInitialized;
	bool useOffset;
	bool isConst;
	bool isLhsEl;
	bool isRhsEl;
	bool refActive;
	bool isArgv1;
	
	/* True if some aspect of the tree has possibly been written to. This does
	 * not include attributes. This is here so we can optimize the storage of
	 * old lhs vars. If only a lhs attribute changes we don't need to preserve
	 * the original for backtracking. */
	bool dirtyTree;

	Code inGetR;
	Code inGetWC;
	Code inGetWV;
	Code inSetWC;
	Code inSetWV;

	ObjField *prev, *next;
};

typedef AvlMap<String, ObjField*, CmpStr> ObjFieldMap;
typedef AvlMapEl<String, ObjField*> ObjFieldMapEl;

typedef DListVal<ObjField*> ObjFieldList;

typedef DList<ObjField> ParameterList; 

struct TemplateType;

struct ObjectDef
{
	enum Type {
		UserType,
		FrameType,
		IterType,
		BuiltinType
	};

	ObjectDef( Type type, String name, 
			ObjFieldMap *objFieldMap, ObjFieldList *objFieldList, 
			ObjMethodMap *objMethodMap, int id )
	:
		type(type), name(name), objFieldMap(objFieldMap), objFieldList(objFieldList),
		objMethodMap(objMethodMap), id(id), nextOffset(0), parentScope(0) {}

	Type type;
	String name;
	ObjFieldMap *objFieldMap;	
	ObjFieldList *objFieldList;
	ObjMethodMap *objMethodMap;	

	long id;
	long nextOffset;
	long firstNonTree;

	void referenceField( ParseData *pd, ObjField *field );
	void initField( ParseData *pd, ObjField *field );
	void createCode( ParseData *pd, CodeVect &code );
	ObjMethod *findMethod( const String &name );
	ObjField *findField( const String &name );
	void insertField( const String &name, ObjField *value );

	long size() { return nextOffset; }
	long sizeTrees() { return firstNonTree; }

	/* For building a hierarchy of scopes. Scopes point up to their parent. */
	ObjectDef *parentScope;

	ObjectDef *prev, *next;
};

typedef Vector<LangExpr*> ExprVect;
typedef Vector<String> StringVect;

struct FieldInit
{
	FieldInit( const InputLoc &loc, String name, LangExpr *expr )
		: loc(loc), name(name), expr(expr) {}

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
	ObjField *objField;
	ObjMethod *objMethod;
	UniqueType *uniqueType;
	UniqueType *iterSearchUT;
};

struct QualItem
{
	enum Type { Dot, Arrow };

	QualItem( const InputLoc &loc, const String &data, Type type )
		: loc(loc), data(data), type(type) {}

	InputLoc loc;
	String data;
	Type type;
};

typedef Vector<QualItem> QualItemVect;

struct LangVarRef
{
	LangVarRef( const InputLoc &loc, QualItemVect *qual, String name )
		: loc(loc), qual(qual), name(name) {}

	void analyze( ParseData *pd ) const;

	UniqueType *loadFieldInstr( ParseData *pd, CodeVect &code, ObjectDef *inObject,
			ObjField *el, bool forWriting, bool revert ) const;
	void setFieldInstr( ParseData *pd, CodeVect &code, ObjectDef *inObject, 
			ObjField *el, UniqueType *exprUT, bool revert ) const;

	VarRefLookup lookupMethod( ParseData *pd ) const;
	VarRefLookup lookupField( ParseData *pd ) const;

	VarRefLookup lookupQualification( ParseData *pd, ObjectDef *rootDef ) const;
	VarRefLookup lookupObj( ParseData *pd ) const;

	bool isLocalRef( ParseData *pd ) const;
	void loadQualification( ParseData *pd, CodeVect &code, ObjectDef *rootObj, 
			int lastPtrInQual, bool forWriting, bool revert ) const;
	void loadLocalObj( ParseData *pd, CodeVect &code, 
			int lastPtrInQual, bool forWriting ) const;
	void loadGlobalObj( ParseData *pd, CodeVect &code, 
			int lastPtrInQual, bool forWriting ) const;
	void loadObj( ParseData *pd, CodeVect &code, int lastPtrInQual, bool forWriting ) const;
	void canTakeRef( ParseData *pd, VarRefLookup &lookup ) const;

	void setFieldIter( ParseData *pd, CodeVect &code, 
			ObjectDef *inObject, UniqueType *objUT, UniqueType *exprType, bool revert ) const;
	void setFieldSearch( ParseData *pd, CodeVect &code, 
			ObjectDef *inObject, UniqueType *exprType ) const;
	void setField( ParseData *pd, CodeVect &code, 
			ObjectDef *inObject, UniqueType *type, bool revert ) const;

	void assignValue( ParseData *pd, CodeVect &code, UniqueType *exprUT ) const;
	ObjField **evaluateArgs( ParseData *pd, CodeVect &code, 
			VarRefLookup &lookup, ExprVect *args ) const;
	void callOperation( ParseData *pd, CodeVect &code, VarRefLookup &lookup ) const;
	UniqueType *evaluateCall( ParseData *pd, CodeVect &code, ExprVect *args ) const;
	UniqueType *evaluate( ParseData *pd, CodeVect &code, bool forWriting = false ) const;
	ObjField *evaluateRef( ParseData *pd, CodeVect &code, long pushCount ) const;
	ObjField *preEvaluateRef( ParseData *pd, CodeVect &code ) const;
	void resetActiveRefs( ParseData *pd, VarRefLookup &lookup, ObjField **paramRefs ) const;
	long loadQualificationRefs( ParseData *pd, CodeVect &code ) const;
	void popRefQuals( ParseData *pd, CodeVect &code, 
			VarRefLookup &lookup, ExprVect *args ) const;

	InputLoc loc;
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
		MakeTreeType,
		MakeTokenType
	};

	LangTerm( Type type, LangVarRef *varRef )
		: type(type), varRef(varRef) {}

	LangTerm( LangVarRef *varRef, ExprVect *args )
		: type(MethodCallType), varRef(varRef), args(args) {}

	LangTerm( const InputLoc &loc, Type type, ExprVect *args )
		: loc(loc), type(type), args(args) {}

	LangTerm( Type type, String data )
		: type(type), varRef(0), data(data) {}

	LangTerm( Type type, NamespaceQual *nspaceQual, const String &data )
		: type(type), varRef(0), nspaceQual(nspaceQual), data(data) {}

	LangTerm( const InputLoc &loc, Type type )
		: loc(loc), type(type), varRef(0), typeRef(0) {}

	LangTerm( const InputLoc &loc, Type type, TypeRef *typeRef )
		: loc(loc), type(type), varRef(0), typeRef(typeRef) {}

	LangTerm( const InputLoc &loc, Type type, LangVarRef *varRef )
		: loc(loc), type(type), varRef(varRef) {}

	LangTerm( Type type, LangVarRef *varRef, Pattern *pattern )
		: type(type), varRef(varRef), pattern(pattern) {}

	LangTerm( const InputLoc &loc, Type type, TypeRef *typeRef, LangVarRef *varRef )
		: loc(loc), type(type), varRef(varRef), typeRef(typeRef) {}

	LangTerm( const InputLoc &loc, Type type, TypeRef *typeRef, FieldInitVect *fieldInitArgs, 
			Replacement *replacement )
		: loc(loc), type(type), typeRef(typeRef), fieldInitArgs(fieldInitArgs), 
		replacement(replacement) {}

	LangTerm( Type type, LangExpr *expr )
		: type(type), expr(expr) {}

	void analyze( ParseData *pd ) const;

	UniqueType *evaluateParse( ParseData *pd, CodeVect &code, bool stop ) const;
	UniqueType *evaluateNew( ParseData *pd, CodeVect &code ) const;
	UniqueType *evaluateConstruct( ParseData *pd, CodeVect &code ) const;
	UniqueType *evaluateMatch( ParseData *pd, CodeVect &code ) const;
	UniqueType *evaluate( ParseData *pd, CodeVect &code ) const;
	void assignFieldArgs( ParseData *pd, CodeVect &code, UniqueType *replUT ) const;
	UniqueType *evaluateMakeToken( ParseData *pd, CodeVect &code ) const;
	UniqueType *evaluateMakeTree( ParseData *pd, CodeVect &code ) const;

	InputLoc loc;
	Type type;
	LangVarRef *varRef;
	ExprVect *args;
	NamespaceQual *nspaceQual;
	String data;
	TypeRef *typeRef;
	Pattern *pattern;
	FieldInitVect *fieldInitArgs;
	Replacement *replacement;
	LangExpr *expr;
};

struct LangExpr
{
	enum Type {
		BinaryType,
		UnaryType,
		TermType
	};

	LangExpr( const InputLoc &loc, LangExpr *left, char op, LangExpr *right )
		: loc(loc), type(BinaryType), left(left), op(op), right(right) {}

	LangExpr( const InputLoc &loc, char op, LangExpr *right )
		: loc(loc), type(UnaryType), left(0), op(op), right(right) {}

	LangExpr( LangTerm *term )
		: type(TermType), term(term) {}

	void analyze( ParseData *pd ) const;

	UniqueType *evaluate( ParseData *pd, CodeVect &code ) const;

	InputLoc loc;
	Type type;
	LangExpr *left;
	char op;
	LangExpr *right;
	LangTerm *term;
};

struct LangStmt;
typedef DList<LangStmt> StmtList;

struct LangStmt
{
	enum Type {
		AssignType,
		PrintType,
		PrintXMLACType,
		PrintXMLType,
		ExprType,
		IfType,
		RejectType,
		WhileType,
		ReturnType,
		YieldType,
		ForIterType,
		BreakType,
		AccumType
	};

	LangStmt( const InputLoc &loc, Type type, FieldInitVect *fieldInitVect ) : 
		loc(loc), type(type), varRef(0), expr(0), fieldInitVect(fieldInitVect), next(0) {}

	LangStmt( const InputLoc &loc, Type type, ExprVect *exprPtrVect ) : 
		loc(loc), type(type), varRef(0), expr(0), exprPtrVect(exprPtrVect), next(0) {}
	
	LangStmt( const InputLoc &loc, Type type, LangExpr *expr ) : 
		loc(loc), type(type), varRef(0), expr(expr), exprPtrVect(0), next(0) {}

	LangStmt( Type type, LangVarRef *varRef ) : 
		type(type), varRef(varRef), expr(0), exprPtrVect(0), next(0) {}

	LangStmt( const InputLoc &loc, Type type, ObjField *objField ) :
		loc(loc), type(type), varRef(0), objField(objField), expr(0), 
		exprPtrVect(0), next(0) {}
	
	LangStmt( const InputLoc &loc, Type type, LangVarRef *varRef, LangExpr *expr ) : 
		loc(loc), type(type), varRef(varRef), expr(expr), exprPtrVect(0), next(0) {}
	
	LangStmt( Type type, LangExpr *expr, StmtList *stmtList ) : 
		type(type), expr(expr), stmtList(stmtList), next(0) {}

	LangStmt( Type type, LangExpr *expr, StmtList *stmtList, StmtList *elsePart ) : 
		type(type), expr(expr), stmtList(stmtList), elsePart(elsePart), next(0) {}

	LangStmt( const InputLoc &loc, Type type ) : 
		loc(loc), type(type), next(0) {}

	LangStmt( Type type, LangVarRef *varRef, Replacement *replacement ) : 
		type(type), varRef(varRef), expr(0), replacement(replacement), 
		exprPtrVect(0), next(0) {}

	LangStmt( Type type, LangVarRef *varRef, AccumText *accumText ) : 
		type(type), varRef(varRef), expr(0), accumText(accumText), 
		exprPtrVect(0), next(0) {}

	/* ForIterType */
	LangStmt( const InputLoc &loc, Type type, ObjField *objField, 
			TypeRef *typeRef, LangTerm *langTerm, StmtList *stmtList ) : 
		loc(loc), type(type), langTerm(langTerm), objField(objField), typeRef(typeRef), 
		stmtList(stmtList), next(0) {}

	LangStmt( Type type ) : 
		type(type), next(0) {}

	void analyze( ParseData *pd ) const;

	void evaluateAccumItems( ParseData *pd, CodeVect &code ) const;
	LangTerm *chooseDefaultIter( ParseData *pd, LangTerm *fromVarRef ) const;
	void compileWhile( ParseData *pd, CodeVect &code ) const;
	void compileForIterBody( ParseData *pd, CodeVect &code, UniqueType *iterUT ) const;
	void compileForIter( ParseData *pd, CodeVect &code ) const;
	void compile( ParseData *pd, CodeVect &code ) const;

	InputLoc loc;
	Type type;
	LangVarRef *varRef;
	LangTerm *langTerm;
	ObjField *objField;
	TypeRef *typeRef;
	LangExpr *expr;
	Replacement *replacement;
	AccumText *accumText;
	ExprVect *exprPtrVect;
	FieldInitVect *fieldInitVect;
	StmtList *stmtList;
	StmtList *elsePart;
	String name;

	/* Normally you don't need to initialize double list pointers, however, we
	 * make use of the next pointer for returning a pair of statements using
	 * one pointer to a LangStmt, so we need to initialize it above. */
	LangStmt *prev, *next;
};

struct CodeBlock
{
	CodeBlock( StmtList *stmtList ) 
		: frameId(-1), stmtList(stmtList), localFrame(0) {}

	void compile( ParseData *pd, CodeVect &code ) const;
	void analyze( ParseData *pd ) const;

	long frameId;
	StmtList *stmtList;
	ObjectDef *localFrame;
	CharSet trees;

	/* Each frame has two versions of 
	 * the code: revert and commit. */
	CodeVect codeWV, codeWC;
};

struct Function
{
	Function( TypeRef *typeRef, const String &name, 
			ParameterList *paramList, CodeBlock *codeBlock, 
			int funcId, bool isUserIter )
	:
		typeRef(typeRef),
		name(name),
		paramList(paramList),
		codeBlock(codeBlock),
		funcId(funcId),
		isUserIter(isUserIter),
		paramListSize(0),
		paramUTs(0)
	{}

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

	Function *prev, *next;
};

typedef DList<Function> FunctionList;

#endif /* _PARSETREE_H */
