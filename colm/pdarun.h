/*
 *  Copyright 2007-2009 Adrian Thurston <thurston@complang.org>
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
#include "bytecode.h"
#include "rtvector.h"
#include "fsmrun2.h"
#include "input.h"
#include "pdarun2.h"

using std::ostream;

typedef struct _Tree Tree;
struct ParseData;
struct FsmRun;
struct KlangEl;
struct PdaTables;
struct InputStreamAccum;

#include "map.h"

MapEl *mapCopyBranch( Program *prg, Map *map, MapEl *el, Kid *oldNextDown, Kid *&newNextDown );
MapEl *mapInsert( Program *prg, Map *map, MapEl *element, MapEl **lastFound = 0 );
MapEl *mapInsert( Program *prg, Map *map, Tree *key, MapEl **lastFound = 0 );
MapEl *mapImplFind( Program *prg, Map *map, Tree *key );
void mapImplRemove( Program *prg, Map *map, MapEl *element );
bool mapImplRemove( Program *prg, Map *map, Tree *key );
MapEl *mapDetachByKey( Program *prg, Map *map, Tree *key );
MapEl *mapDetach( Program *prg, Map *map, MapEl *element );

typedef struct _Accum
{
	/* Must overlay Tree. */
	short id;
	unsigned short flags;
	long refs;
	Kid *child;

	GenericInfo *genericInfo;

	PdaRun *pdaRun;
	FsmRun *fsmRun;
	Stream *stream;
} Accum;

struct Stream
{
	/* Must overlay Tree. */
	short id;
	unsigned short flags;
	long refs;
	Kid *child;

	FILE *file;
	InputStream *in;
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

/* This must overlay tree iter because some of the same bytecodes are used. */
struct RevTreeIter
{
	RevTreeIter( const Ref &rootRef, int searchId, Tree **stackRoot, int children ) : 
		rootRef(rootRef), searchId(searchId), 
		stackRoot(stackRoot), stackSize(children), kidAtYield(0), children(children)
	{
		ref.kid = 0;
		ref.next = 0;
	}
	
	Ref rootRef;
	Ref ref;
	long searchId;
	Tree **stackRoot;
	long stackSize;

	/* For detecting a split at the leaf. */
	Kid *kidAtYield;
	long children;
	Kid **cur;
};

typedef struct _FunctionInfo
{
	const char *name;
	long frameId;
	long argSize;
	long ntrees;
	long frameSize;
} FunctionInfo;

typedef struct _UserIter
{
	/* The current item. */
	Ref ref;
	Tree **stackRoot;
	long argSize;
	long stackSize;
	Code *resume;
	Tree **frame;
	long searchId;
} UserIter;

void initUserIter( UserIter *userIter, Tree **stackRoot, long argSize, long searchId );


/*
 * Program Data.
 */

typedef struct _PatReplInfo
{
	long offset;
	long numBindings;
} PatReplInfo;

typedef struct _PatReplNode
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
} PatReplNode;

/* FIXME: should have a descriptor for object types to give the length. */

typedef struct _LangElInfo
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

//	long contextTypeId;
//	long contextLength;

	long termDupId;
	long genericId;
	long markId;
	long captureAttr;
	long numCaptureAttr;
} LangElInfo;

typedef struct _ObjFieldInfo
{
	int typeId;
} ObjFieldInfo;

typedef struct _ProdInfo
{
	long length;
	unsigned long lhsId;
	const char *name;
	long frameId;
	bool lhsUpref;
} ProdInfo;

typedef struct _FrameInfo
{
	Code *codeWV;
	long codeLenWV;
	Code *codeWC;
	long codeLenWC;
	char *trees;
	long treesLen;
} FrameInfo;

typedef struct _RegionInfo
{
	const char *name;
	long defaultToken;
	long eofFrameId;
} RegionInfo;

typedef struct _CaptureAttr
{
	long mark_enter;
	long mark_leave;
	long offset;
} CaptureAttr;

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
	int *parserLelIds;
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

bool makeReverseCode( CodeVect *all, CodeVect &reverseCode );

typedef RtVector<Tree*> Bindings;

struct PdaRun
{
	PdaRun( Program *prg, PdaTables *tables, FsmRun *fsmRun, int parserId,
			long stopTarget, bool revertOn )
	:
		prg(prg),
		tables(tables), 
		fsmRun(fsmRun),
		parserId(parserId), 
		stopParsing(false),
		stopTarget(stopTarget),
		queue(0),
		queueLast(0),
		revertOn(revertOn),
		context(0),
		consumed(0),
		targetConsumed(-1)
	{
	}

	int numRetry;
	Kid *stackTop;
	int errCount;
	int cs;
	int nextRegionInd;

	/* Offset can be used to look at the next nextRegionInd. */
	int getNextRegion( int offset = 0 )
		{ return tables->tokenRegions[nextRegionInd+offset]; }

	Program *prg;
	PdaTables *tables;
	FsmRun *fsmRun;
	int parserId;

	long stackTopTarget();
	void commitKid( Tree **root, Kid *lel );
	void commit();
	bool isParserStopFinished();
	void match( Kid *tree, Kid *pattern );

	Kid *extractIgnore();

	/* Report an error encountered by the parser. */
	ostream &parseError( int tokId, Tree *tree );

	/* Reused. */
	CodeVect reverseCode;
	CodeVect *allReverseCode;

	bool stopParsing;
	long stopTarget;

	Kid *accumIgnore;
	Kid *queue, *queueLast;

	Bindings bindings;

	bool revertOn;

	Tree *context;
	void clearContext( Tree **sp );

	//bool fragStop;
	bool stop;

	long consumed;
	long targetConsumed;
};

void cleanParser( Tree **root, PdaRun *pdaRun );
void ignore( PdaRun *pdaRun, Tree *tree );
void parseToken( Tree **sp, PdaRun *pdaRun, FsmRun *fsmRun, InputStream *inputStream, Kid *input );
long undoParse( Tree **sp, PdaRun *pdaRun, FsmRun *fsmRun, InputStream *inputStream, Tree *tree );
void xml_print_list( RuntimeData *runtimeData, Kid *lel, int depth );
ostream &parseError( InputStream *inputStream, FsmRun *fsmRun, PdaRun *pdaRun, int tokId, Tree *tree );
void initPdaRun( PdaRun *pdaRun, Tree *tree );

#endif /* _PDARUN_H */
