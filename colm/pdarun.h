/*
 *  Copyright 2007 Adrian Thurston <thurston@complang.org>
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

#ifndef _PDARUN_H
#define _PDARUN_H

#include <iostream>
#include "dlistval.h"
#include "bytecode.h"
#include "vector.h"
#include "dlist.h"

using std::ostream;

struct Tree;
struct ParseData;
struct FsmRun;
struct KlangEl;
struct PdaTables;
struct FsmTables;
struct InputStream;

struct Kid
{
	/* The tree needs to be first since pointers to kids are used to reference
	 * trees on the stack. A pointer to the word that is a Tree* is cast to
	 * a Kid*. */
	Tree *tree;
	Kid *next;
};

struct Ref
{
	Kid *kid;
	Ref *next;
};

struct Tree
{
	/* First four will be overlaid in other structures. */
	short id;
	unsigned short flags;
	long refs;
	Kid *child;

	Head *tokdata;
};

struct ParseTree
{
	/* Entire structure must overlay Tree. */
	short id;
	unsigned short flags;
	long refs;
	Kid *child;

	Head *tokdata;

	/* Parsing algorithm. */
	long state;
	long region;
	char causeReduce;
	char retry_lower;
	char retry_upper;
};


struct Int
{
	/* Must overlay Tree. */
	short id;
	unsigned short flags;
	long refs;
	Kid *child;

	long value;
};

struct Pointer
{
	/* Must overlay Tree. */
	short id;
	unsigned short flags;
	long refs;
	Kid *child;

	Kid *value;
};

struct Str
{
	/* Must overlay Tree. */
	short id;
	unsigned short flags;
	long refs;
	Kid *child;

	Head *value;
};

struct ListEl
{
	/* Must overlay kid. */
	Tree *value;
	ListEl *next;
	ListEl *prev;

	ListEl() { }
	ListEl( Tree *value )
		: value(value) { }
};

struct List
{
	/* Must overlay Tree. */
	short id;
	unsigned short flags;
	long refs;
	ListEl *head;

	ListEl *tail;
	long listLen;
	GenericInfo *genericInfo;

	void prepend(ListEl *new_el) { addBefore(head, new_el); }
	void append(ListEl *new_el)  { addAfter(tail, new_el); }

	void addAfter( ListEl *prev_el, ListEl *new_el );
	void addBefore( ListEl *next_el, ListEl *new_el );

	ListEl *detachFirst()        { return detach(head); }
	ListEl *detachLast()         { return detach(tail); }
	ListEl *detach(ListEl *el);

	long length() const 
		{ return listLen; }
};

struct MapEl
{
	/* Must overlay Kid. */
	Tree *tree;
	MapEl *next;
	MapEl *prev;

	MapEl *left, *right, *parent;
	long height;
	Tree *key;

	Tree *getKey() const 
		{ return key; }
};

struct Map
{
	/* Must overlay Tree. */
	short id;
	unsigned short flags;
	long refs;
	MapEl *head;

	MapEl *tail;
	MapEl *root;
	long treeSize;
	GenericInfo *genericInfo;

	/* List functions. */
	MapEl *listDetach( MapEl *el );
	void listAddBefore( MapEl *next_el, MapEl *new_el );
	void listAddAfter( MapEl *prev_el, MapEl *new_el );
	void listAbandon();

	int compare( const Tree *w1, const Tree *w2 ) const
		{ return cmp_tree( w1, w2 ); }

	/* Insert a element into the tree. */
	MapEl *insert( MapEl *element, MapEl **lastFound = 0 );

	MapEl *insert( Program *p, Tree *key, MapEl **lastFound = 0 );

	/* Find a element in the tree. Returns the element if 
	 * key exists, false otherwise. */
	MapEl *find( Tree *key ) const;

	/* Detach a element from the tree. */
	MapEl *detach( Tree *key );

	/* Detach and delete a element from the tree. */
	bool remove( Tree *key );

	/* Detach a element from the tree. */
	MapEl *detach( MapEl *element );

	/* Detach and delete a element from the tree. */
	void remove( MapEl *element );

	/* Free all memory used by tree. */
	void empty();

	/** \brief Return the number of elements in the tree. */
	long length() const { return treeSize; }

	/* Recursive worker for the copy constructor. */
	MapEl *copyBranch( Program *p, MapEl *element, Kid *oldNextDown, Kid *&newNextDown );

	/* Recursively delete element in the tree. */
	void deleteChildrenOf(MapEl *n);

	/* rebalance the tree beginning at the leaf whose 
	 * grandparent is unbalanced. */
	MapEl *rebalance(MapEl *start);

	/* Move up the tree from a given element, recalculating the heights. */
	void recalcHeights(MapEl *start);

	/* Move up the tree and find the first element whose 
	 * grand-parent is unbalanced. */
	MapEl *findFirstUnbalGP(MapEl *start);

	/* Move up the tree and find the first element which is unbalanced. */
	MapEl *findFirstUnbalEl(MapEl *start);

	/* Replace a element in the tree with another element not in the tree. */
	void replaceEl(MapEl *element, MapEl *replacement);

	/* Remove a element from the tree and put another (normally a child of element)
	 * in its place. */
	void removeEl(MapEl *element, MapEl *filler);

	/* Once an insertion point is found at a leaf then do the insert. */
	void attachRebal( MapEl *element, MapEl *parentEl, MapEl *lastLess );
};

struct Stream
{
	/* Must overlay Tree. */
	short id;
	unsigned short flags;
	long refs;
	Kid *child;

	FILE *file;
	InputStream *in;
	FsmRun *scanner;
};

/*
 * Iterators.
 */

struct TreeIter
{
	TreeIter( const Ref &rootRef, int searchId, Tree **stackRoot ) : 
		rootRef(rootRef), searchId(searchId), 
		stackRoot(stackRoot), stackSize(0)
	{
		ref.kid = 0;
		ref.next = 0;
	}
	
	Ref rootRef;
	Ref ref;
	long searchId;
	Tree **stackRoot;
	long stackSize;
};

struct FunctionInfo
{
	const char *name;
	long frameId;
	long argSize;
	long ntrees;
	long frameSize;
};

struct UserIter
{
	UserIter( Tree **stackRoot, long argSize, long searchId ) : 
		stackRoot(stackRoot), 
		argSize(argSize), stackSize(0),
		resume(0), frame(0), searchId(searchId)
	{
		ref.kid = 0;
		ref.next = 0;
	}
		
	/* The current item. */
	Ref ref;
	Tree **stackRoot;
	long argSize;
	long stackSize;
	Code *resume;
	Tree **frame;
	long searchId;
};

/*
 * Program Data.
 */

struct PatReplInfo
{
	long offset;
	long numBindings;
};

struct PatReplNode
{
	long id;
	long next;
	long child;
	long bindId;
	const char *data;
	long length;
	long ignore;

	/* Just match nonterminal, don't go inside. */
	bool stop;
};

struct LangElInfo
{
	const char *name;
	bool repeat;
	bool list;
	bool literal;
	bool ignore;

	long frameId;

	long objectTypeId;
	long ofiOffset;
	long objectLength;

	long termDupId;
	long genericId;
	long markId;
	long captureAttr;
	long numCaptureAttr;
};

struct ObjFieldInfo
{
	int typeId;
};

struct ProdInfo
{
	long length;
	unsigned long lhsId;
	const char *name;
	long frameId;
	bool lhsUpref;
};

struct FrameInfo
{
	Code *codeWV;
	long codeLenWV;
	Code *codeWC;
	long codeLenWC;
	char *trees;
	long treesLen;
};

struct RegionInfo
{
	const char *name;
	long defaultToken;
	long eofFrameId;
};

struct CaptureAttr
{
	long mark_enter;
	long mark_leave;
	long offset;
};

struct RuntimeData
{
	LangElInfo *lelInfo;
	long numLangEls;

	ProdInfo *prodInfo;
	long numProds;

	RegionInfo *regionInfo;
	long numRegions;

	Code *rootCode;
	long rootCodeLen;

	FrameInfo *frameInfo;
	long numFrames;

	FunctionInfo *functionInfo;
	long numFunctions;

	PatReplInfo *patReplInfo;
	long numPatterns;

	PatReplNode *patReplNodes;
	long numPatternNodes;

	GenericInfo *genericInfo;
	long numGenerics;

	const char **litdata;
	long *litlen;
	Head **literals;
	long numLiterals;

	CaptureAttr *captureAttr;
	long numCapturedAttr;

	FsmTables *fsmTables;
	PdaTables *pdaTables;
	int *startStates;
	int *eofLelIds;
	long numParsers;

	long globalSize;

	long firstNonTermId;

	long integerId;
	long stringId;
	long anyId;
	long eofId;
	long noTokenId;
};

struct PdaTables
{
	/* Parser table data. */
	int *indicies;
	int *keys;
	unsigned int *offsets;
	unsigned int *targs;
	unsigned int *actInds;
	unsigned int *actions;
	int *commitLen;
	int *tokenRegionInds;
	int *tokenRegions;

	int numIndicies;
	int numKeys;
	int numStates;
	int numTargs;
	int numActInds;
	int numActions;
	int numCommitLen;
	int numRegionItems;

	RuntimeData *rtd;
};

bool make_reverse_code( CodeVect *all, CodeVect &reverseCode );

typedef Vector<Tree*> Bindings;

struct PdaRun
{
	PdaRun( Tree **root, Program *prg, PdaTables *tables, int parserId,
			FsmRun *scanner, long stopTarget, bool revertOn )
	:
		root(root),
		prg(prg),
		tables(tables), 
		parserId(parserId), 
		fsmRun(scanner), 
		stopParsing(false),
		stopTarget(stopTarget),
		queue(0),
		queueLast(0),
		revertOn(revertOn)
	{
	}

	Tree **root;
	int numRetry;
	Kid *stackTop;
	int errCount;
	int cs;
	int nextRegionInd;

	/* Offset can be used to look at the next nextRegionInd. */
	int getNextRegion( int offset = 0 )
		{ return tables->tokenRegions[nextRegionInd+offset]; }

	Tree *getParsedRoot( bool stop );
	void clean();

	Program *prg;
	PdaTables *tables;
	int parserId;

	FsmRun *fsmRun;

	long stackTopTarget();
	void init();
	void commitKid( Tree **root, Kid *lel );
	void commit();
	void parseToken( Kid *input );
	bool isParserStopFinished();
	void match( Kid *tree, Kid *pattern );
	long undoParse( Tree *tree, CodeVect *rev );

	void send( Kid *kid );
	void ignore( Tree *tree );
	void sendBackIgnore();
	Kid *extractIgnore();

	/* Report an error encountered by the parser. */
	ostream &parse_error( int tokId, Tree *tree );

	/* Reused. */
	CodeVect reverseCode;
	CodeVect *allReverseCode;

	bool stopParsing;
	long stopTarget;

	Kid *accumIgnore;
	Kid *queue, *queueLast;

	Bindings bindings;

	bool revertOn;
};

void xml_print_list( RuntimeData *runtimeData, Kid *lel, int depth );

#endif /* _PDARUN_H */
