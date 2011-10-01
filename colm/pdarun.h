/*
 *  Copyright 2007-2011 Adrian Thurston <thurston@complang.org>
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

#include "input.h"
#include "fsmrun.h"
#include "config.h"
#include "defs.h"

#ifdef __cplusplus
extern "C" {
#endif

#if SIZEOF_LONG != 4 && SIZEOF_LONG != 8 
	#error "SIZEOF_LONG contained an unexpected value"
#endif

typedef unsigned char Code;
typedef unsigned long Word;
typedef unsigned long Half;
struct Bindings;

typedef struct _RtCodeVect
{
	Code *data;
	long tabLen;
	long allocLen;

	/* FIXME: leak when freed. */
} RtCodeVect;


typedef struct _File
{
	struct _File *prev;
	struct _File *next;
} File;

typedef struct _Location
{
	File *file;
	long line;
	long column;
	long byte;
} Location;

/* Header located just before string data. */
typedef struct _Head
{
	const char *data; long length;
	Location *location;
} Head;

typedef struct _Kid
{
	/* The tree needs to be first since pointers to kids are used to reference
	 * trees on the stack. A pointer to the word that is a Tree* is cast to
	 * a Kid*. */
	struct _Tree *tree;
	struct _Kid *next;
	unsigned char flags;
} Kid;

typedef struct _Ref
{
	struct _Kid *kid;
	struct _Ref *next;
} Ref;

typedef struct _Tree
{
	/* First four will be overlaid in other structures. */
	short id;
	unsigned short flags;
	long refs;
	Kid *child;

	Head *tokdata;
} Tree;

typedef struct _IgnoreList
{
	/* First four will be overlaid in other structures. */
	short id;
	unsigned short flags;
	long refs;
	Kid *child;

	long generation;
} IgnoreList;

typedef struct _ParseTree
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
} ParseTree;

typedef struct _Int
{
	/* Must overlay Tree. */
	short id;
	unsigned short flags;
	long refs;
	Kid *child;

	long value;
} Int;

typedef struct _Pointer
{
	/* Must overlay Tree. */
	short id;
	unsigned short flags;
	long refs;
	Kid *child;

	Kid *value;
} Pointer;

typedef struct _Str
{
	/* Must overlay Tree. */
	short id;
	unsigned short flags;
	long refs;
	Kid *child;

	Head *value;
} Str;

typedef struct _ListEl
{
	/* Must overlay kid. */
	Tree *value;
	struct _ListEl *next;
	struct _ListEl *prev;
} ListEl;

/*
 * Maps
 */
typedef struct _GenericInfo
{
	long type;
	long typeArg;
	long keyOffset;
	long keyType;
	long langElId;
	long parserId;
} GenericInfo;

typedef struct _List
{
	/* Must overlay Tree. */
	short id;
	unsigned short flags;
	long refs;
	ListEl *head;

	ListEl *tail;
	long listLen;
	GenericInfo *genericInfo;

} List;


void listAddAfter( List *list, ListEl *prev_el, ListEl *new_el );
void listAddBefore( List *list, ListEl *next_el, ListEl *new_el );

void listPrepend( List *list, ListEl *new_el );
void listAppend( List *list, ListEl *new_el );

ListEl *listDetach( List *list, ListEl *el );
ListEl *listDetachFirst(List *list );
ListEl *listDetachLast(List *list );

long listLength(List *list);

typedef struct _Stream
{
	/* Must overlay Tree. */
	short id;
	unsigned short flags;
	long refs;
	Kid *child;

	FILE *file;
	InputStream *in;
} Stream;

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
	unsigned char stop;
} PatReplNode;

/* FIXME: should have a descriptor for object types to give the length. */

typedef struct _LangElInfo
{
	const char *name;
	unsigned char repeat;
	unsigned char list;
	unsigned char literal;
	unsigned char ignore;

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
	unsigned char lhsUpref;
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

typedef struct _RuntimeData
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

	long argvGenericId;

	const char **litdata;
	long *litlen;
	Head **literals;
	long numLiterals;

	CaptureAttr *captureAttr;
	long numCapturedAttr;

	FsmTables *fsmTables;
	struct _PdaTables *pdaTables;
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
} RuntimeData;

typedef struct _PdaTables
{
	/* Parser table data. */
	int *indicies;
	int *owners;
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
} PdaTables;


typedef struct _PoolBlock
{
	void *data;
	struct _PoolBlock *next;
} PoolBlock;

typedef struct _PoolItem
{
	struct _PoolItem *next;
} PoolItem;

typedef struct _PoolAlloc
{
	PoolBlock *head;
	long nextel;
	PoolItem *pool;
	int sizeofT;
} PoolAlloc;

typedef struct _Program
{
	int argc;
	char **argv;

	unsigned char ctxDepParsing;
	RuntimeData *rtd;
	Tree *global;
	long nextIlGen;

	PoolAlloc kidPool;
	PoolAlloc treePool;
	PoolAlloc parseTreePool;
	PoolAlloc listElPool;
	PoolAlloc mapElPool;
	PoolAlloc headPool;
	PoolAlloc locationPool;
	PoolAlloc ilPool;

	Tree *trueVal;
	Tree *falseVal;

	Kid *heap;

	Stream *stdinVal;
	Stream *stdoutVal;
	Stream *stderrVal;
} Program;

typedef struct _TreeIter
{
	Ref rootRef;
	Ref ref;
	long searchId;
	Tree **stackRoot;
	long stackSize;
} TreeIter;

/* This must overlay tree iter because some of the same bytecodes are used. */
typedef struct _RevTreeIter
{
	Ref rootRef;
	Ref ref;
	long searchId;
	Tree **stackRoot;
	long stackSize;

	/* For detecting a split at the leaf. */
	Kid *kidAtYield;
	long children;
	Kid **cur;
} RevTreeIter;

typedef struct _PdaRun
{
	int numRetry;
	Kid *stackTop;
	Ref *tokenList;
	int errCount;
	int cs;
	int nextRegionInd;

	Program *prg;
	PdaTables *tables;
	FsmRun *fsmRun;
	int parserId;

	/* Reused. */
	RtCodeVect rcodeCollect;
	RtCodeVect reverseCode;

	int stopParsing;
	long stopTarget;

	Kid *accumIgnore;
	Kid *queue, *queueLast;

	struct Bindings *bindings;

	int revertOn;

	Tree *context;

	//bool fragStop;
	int stop;

	long consumed;
	long targetConsumed;
} PdaRun;

typedef struct AccumStruct
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

void rtCodeVectReplace( RtCodeVect *vect, long pos, const Code *val, long len );
void rtCodeVectEmpty( RtCodeVect *vect );
void rtCodeVectRemove( RtCodeVect *vect, long pos, long len );

void initRtCodeVect( RtCodeVect *codeVect );

//inline static void remove( RtCodeVect *vect, long pos );
inline static void append( RtCodeVect *vect, const Code val );
inline static void append2( RtCodeVect *vect, const Code *val, long len );
inline static void appendHalf( RtCodeVect *vect, Half half );
inline static void appendWord( RtCodeVect *vect, Word word );

inline static void append2( RtCodeVect *vect, const Code *val, long len )
{
	rtCodeVectReplace( vect, vect->tabLen, val, len );
}

inline static void append( RtCodeVect *vect, const Code val )
{
	rtCodeVectReplace( vect, vect->tabLen, &val, 1 );
}

inline static void appendHalf( RtCodeVect *vect, Half half )
{
	/* not optimal. */
	append( vect, half & 0xff );
	append( vect, (half>>8) & 0xff );
}

inline static void appendWord( RtCodeVect *vect, Word word )
{
	/* not optimal. */
	append( vect, word & 0xff );
	append( vect, (word>>8) & 0xff );
	append( vect, (word>>16) & 0xff );
	append( vect, (word>>24) & 0xff );
	#if SIZEOF_LONG == 8
	append( vect, (word>>32) & 0xff );
	append( vect, (word>>40) & 0xff );
	append( vect, (word>>48) & 0xff );
	append( vect, (word>>56) & 0xff );
	#endif
}

void incrementConsumed( PdaRun *pdaRun );
void decrementConsumed( PdaRun *pdaRun );

void parseError( InputStream *inputStream, FsmRun *fsmRun, PdaRun *pdaRun, int tokId, Tree *tree );
int makeReverseCode( RtCodeVect *all, RtCodeVect *reverseCode );


void initPdaRun( PdaRun *pdaRun, Program *prg, PdaTables *tables,
		FsmRun *fsmRun, int parserId, long stopTarget, int revertOn, Tree *context );

void clearContext( PdaRun *pdaRun, Tree **sp );
Kid *extractIgnore( PdaRun *pdaRun );
long stackTopTarget( PdaRun *pdaRun );
void runCommit( PdaRun *pdaRun );
int isParserStopFinished( PdaRun *pdaRun );
void pdaRunMatch(  PdaRun *pdaRun, Kid *tree, Kid *pattern );

/* Offset can be used to look at the next nextRegionInd. */
int pdaRunGetNextRegion( PdaRun *pdaRun, int offset );

void cleanParser( Tree **root, PdaRun *pdaRun );
void ignoreTree( PdaRun *pdaRun, Tree *tree );
void parseToken( Tree **sp, PdaRun *pdaRun, FsmRun *fsmRun, InputStream *inputStream, Kid *input );
long undoParse( Tree **sp, PdaRun *pdaRun, FsmRun *fsmRun, InputStream *inputStream, Tree *tree );
void xml_print_list( RuntimeData *runtimeData, Kid *lel, int depth );

Head *streamPull( Program *prg, FsmRun *fsmRun, InputStream *inputStream, long length );
Head *stringAllocPointer( Program *prg, const char *data, long length );

void streamPushText( InputStream *inputStream, const char *data, long length );
void streamPushTree( InputStream *inputStream, Tree *tree, int ignore );
void undoStreamPush( Program *prg, Tree **sp, InputStream *inputStream, long length );
void undoStreamAppend( Program *prg, Tree **sp, InputStream *inputStream, long length );
void sendBackText( FsmRun *fsmRun, InputStream *inputStream, const char *data, long length );
void sendBackIgnore( Tree **sp, PdaRun *pdaRun, FsmRun *fsmRun, InputStream *inputStream, Kid *ignore );
void sendBack( Tree **sp, PdaRun *pdaRun, FsmRun *fsmRun, InputStream *inputStream, Kid *input );
void unbind( Program *prg, Tree **sp, PdaRun *pdaRun, Tree *tree );
void queueBackTree( Tree **sp, PdaRun *pdaRun, FsmRun *fsmRun, InputStream *inputStream, Kid *input );
void addNoToken( Program *prg, PdaRun *parser );
void sendQueuedTokens( Tree **sp, PdaRun *pdaRun, FsmRun *fsmRun, InputStream *inputStream );
void sendHandleError( Tree **sp, PdaRun *pdaRun, FsmRun *fsmRun, InputStream *inputStream, Kid *input );
Kid *makeToken( PdaRun *pdaRun, FsmRun *fsmRun, InputStream *inputStream, int id,
		Head *tokdata, int namedLangEl, int bindId );
void makeTokenPushBinding( PdaRun *pdaRun, int bindId, Tree *tree );
void executeGenerationAction( Tree **sp, Program *prg, FsmRun *fsmRun, PdaRun *pdaRun, 
		Code *code, long id, Head *tokdata );
void generationAction( Tree **sp, InputStream *inputStream, FsmRun *fsmRun, 
		PdaRun *pdaRun, int id, Head *tokdata, int namedLangEl, int bindId );
Kid *extractIgnore( PdaRun *pdaRun );
void sendBackQueuedIgnore( Tree **sp, InputStream *inputStream, FsmRun *fsmRun, PdaRun *pdaRun );
void clearIgnoreList( Program *prg, Tree **sp, Kid *kid );
void sendWithIgnore( Tree **sp, PdaRun *pdaRun, FsmRun *fsmRun, InputStream *inputStream, Kid *input );
Head *extractMatch( Program *prg, FsmRun *fsmRun, InputStream *inputStream );
void execGen( Tree **sp, InputStream *inputStream, FsmRun *fsmRun, PdaRun *pdaRun, long id );
void sendIgnore( Tree **sp, InputStream *inputStream, FsmRun *fsmRun, PdaRun *pdaRun, long id );
Head *extractMatch( Program *prg, FsmRun *fsmRun, InputStream *inputStream );
void sendToken( Tree **sp, InputStream *inputStream, FsmRun *fsmRun, PdaRun *pdaRun, long id );
void sendEof( Tree **sp, InputStream *inputStream, FsmRun *fsmRun, PdaRun *pdaRun );
void initInputStream( InputStream *in );
void newToken( PdaRun *pdaRun, FsmRun *fsmRun );
void breakRunBuf( FsmRun *fsmRun );
void fsmExecute( FsmRun *fsmRun, InputStream *inputStream );
void sendNamedLangEl( Tree **sp, PdaRun *pdaRun, FsmRun *fsmRun, InputStream *inputStream );
void parseLoop( Tree **sp, PdaRun *pdaRun, FsmRun *fsmRun, InputStream *inputStream );
void initBindings( PdaRun *pdaRun );
Tree *getParsedRoot( PdaRun *pdaRun, int stop );

#ifdef __cplusplus
}
#endif

#endif
