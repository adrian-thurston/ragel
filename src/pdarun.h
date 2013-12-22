/*
 *  Copyright 2007-2012 Adrian Thurston <thurston@complang.org>
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

#ifndef __COLM_PDARUN_H
#define __COLM_PDARUN_H

#include <colm/input.h>
#include <colm/defs.h>
#include <colm/tree.h>

#ifdef __cplusplus
extern "C" {
#endif

struct colm_program;

#define MARK_SLOTS 32

typedef struct _FsmTables
{
	long *actions;
	long *keyOffsets;
	char *transKeys;
	long *singleLengths;
	long *rangeLengths;
	long *indexOffsets;
	long *transTargsWI;
	long *transActionsWI;
	long *toStateActions;
	long *fromStateActions;
	long *eofActions;
	long *eofTargs;
	long *entryByRegion;

	long numStates;
	long numActions;
	long numTransKeys;
	long numSingleLengths;
	long numRangeLengths;
	long numIndexOffsets;
	long numTransTargsWI;
	long numTransActionsWI;
	long numRegions;

	long startState;
	long firstFinal;
	long errorState;

	struct GenAction **actionSwitch;
	long numActionSwitch;
} FsmTables;

typedef struct _FsmRun
{
	FsmTables *tables;

	RunBuf *consumeBuf;

	/* FsmRun State. */
	long region, preRegion;
	long cs, ncs, act;
	char *start;
	char *tokstart;
	long tokend;
	long toklen;
	char *p, *pe;

	/* Bits. */
	char eof;
	char returnResult;
	char skipToklen;

	char *mark[MARK_SLOTS];
	long matchedToken;
} FsmRun;

void undoStreamPull( StreamImpl *inputStream, const char *data, long length );

#if SIZEOF_LONG != 4 && SIZEOF_LONG != 8 
	#error "SIZEOF_LONG contained an unexpected value"
#endif

struct _Execution;

typedef struct _RtCodeVect
{
	Code *data;
	long tabLen;
	long allocLen;

	/* FIXME: leak when freed. */
} RtCodeVect;

void listAddAfter( List *list, ListEl *prev_el, ListEl *new_el );
void listAddBefore( List *list, ListEl *next_el, ListEl *new_el );

void listPrepend( List *list, ListEl *new_el );
void listAppend( List *list, ListEl *new_el );

ListEl *listDetach( List *list, ListEl *el );
ListEl *listDetachFirst(List *list );
ListEl *listDetachLast(List *list );

long listLength(List *list);

typedef struct _FunctionInfo
{
	const char *name;
	long frameId;
	long argSize;
	long frameSize;
} FunctionInfo;

/*
 * Program Data.
 */

typedef struct _PatConsInfo
{
	long offset;
	long numBindings;
} PatConsInfo;

typedef struct _PatConsNode
{
	long id;
	long prodNum;
	long next;
	long child;
	long bindId;
	const char *data;
	long length;
	long leftIgnore;
	long rightIgnore;

	/* Just match nonterminal, don't go inside. */
	unsigned char stop;
} PatConsNode;

/* FIXME: should have a descriptor for object types to give the length. */

typedef struct _LangElInfo
{
	const char *name;
	const char *xmlTag;
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
	unsigned long lhsId;
	short prodNum;
	long length;
	const char *name;
	long frameId;
	unsigned char lhsUpref;
	unsigned char *copy;
	long copyLen;
} ProdInfo;

typedef struct _LocalInfo
{
	char type;
	short offset;
} LocalInfo;

typedef struct _FrameInfo
{
	Code *codeWV;
	long codeLenWV;
	Code *codeWC;
	long codeLenWC;
	char *trees;
	long treesLen;
	char *iters;
	long itersLen;
	LocalInfo *locals;
	long localsLen;
	long argSize;
	long frameSize;
} FrameInfo;

typedef struct _RegionInfo
{
	long defaultToken;
	long eofFrameId;
	int ciLelId;
} RegionInfo;

typedef struct _CaptureAttr
{
	long mark_enter;
	long mark_leave;
	long offset;
} CaptureAttr;

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
	int *tokenPreRegions;

	int numIndicies;
	int numKeys;
	int numStates;
	int numTargs;
	int numActInds;
	int numActions;
	int numCommitLen;
	int numRegionItems;
	int numPreRegionItems;
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

typedef struct _PdaRun
{
	int numRetry;
	ParseTree *stackTop;
	Ref *tokenList;
	int cs;
	int nextRegionInd;

	PdaTables *tables;
	int parserId;

	/* Reused. */
	RtCodeVect rcodeCollect;
	RtCodeVect reverseCode;

	int stopParsing;
	long stopTarget;

	ParseTree *accumIgnore;

	Kid *btPoint;

	struct Bindings *bindings;

	int revertOn;

	Tree *context;

	int stop;
	int parseError;

	long steps;
	long targetSteps;

	int onDeck;

	/*
	 * Data we added when refactoring the parsing engine into a coroutine.
	 */

	ParseTree *parseInput;
	FrameInfo *fi;
	int reduction;
	ParseTree *redLel;
	int curState;
	ParseTree *lel;
	int triggerUndo;

	int tokenId;
	Head *tokdata;
	int frameId;
	int next;
	ParseTree *undoLel;

	int checkNext;
	int checkStop;

	/* The lhs is sometimes saved before reduction actions in case it is
	 * replaced and we need to restore it on backtracking */
	Tree *parsed;

	int reject;

	/* Instruction pointer to use when we stop parsing and execute code. */
	Code *code;

	int rcBlockCount;

	Tree *parseErrorText;

	FsmRun *fsmRun;
	FsmRun _fsmRun;
} PdaRun;

void initPdaRun( struct colm_program *prg, PdaRun *pdaRun, PdaTables *tables,
		int parserId, long stopTarget, int revertOn, Tree *context );
void clearPdaRun( struct colm_program *prg, struct colm_tree **sp, PdaRun *pdaRun );
void rtCodeVectReplace( RtCodeVect *vect, long pos, const Code *val, long len );
void rtCodeVectEmpty( RtCodeVect *vect );
void rtCodeVectRemove( RtCodeVect *vect, long pos, long len );

void initRtCodeVect( RtCodeVect *codeVect );

//inline static void remove( RtCodeVect *vect, long pos );
inline static void appendCode( RtCodeVect *vect, const Code val );
inline static void appendCode2( RtCodeVect *vect, const Code *val, long len );
inline static void appendHalf( RtCodeVect *vect, Half half );
inline static void appendWord( RtCodeVect *vect, Word word );

inline static void appendCode2( RtCodeVect *vect, const Code *val, long len )
{
	rtCodeVectReplace( vect, vect->tabLen, val, len );
}

inline static void appendCode( RtCodeVect *vect, const Code val )
{
	rtCodeVectReplace( vect, vect->tabLen, &val, 1 );
}

inline static void appendHalf( RtCodeVect *vect, Half half )
{
	/* not optimal. */
	appendCode( vect, half & 0xff );
	appendCode( vect, (half>>8) & 0xff );
}

inline static void appendWord( RtCodeVect *vect, Word word )
{
	/* not optimal. */
	appendCode( vect, word & 0xff );
	appendCode( vect, (word>>8) & 0xff );
	appendCode( vect, (word>>16) & 0xff );
	appendCode( vect, (word>>24) & 0xff );
	#if SIZEOF_LONG == 8
	appendCode( vect, (word>>32) & 0xff );
	appendCode( vect, (word>>40) & 0xff );
	appendCode( vect, (word>>48) & 0xff );
	appendCode( vect, (word>>56) & 0xff );
	#endif
}

void incrementSteps( PdaRun *pdaRun );
void decrementSteps( PdaRun *pdaRun );

int makeReverseCode( PdaRun *pdaRun );
void transferReverseCode( PdaRun *pdaRun, ParseTree *tree );

void clearStreamImpl( struct colm_program *prg, Tree **sp, StreamImpl *inputStream );
void initSourceStream( StreamImpl *in );
void clearSourceStream( struct colm_program *prg, Tree **sp, StreamImpl *sourceStream );


void clearContext( PdaRun *pdaRun, Tree **sp );
Kid *extractIgnore( PdaRun *pdaRun );
long stackTopTarget( struct colm_program *prg, PdaRun *pdaRun );
void runCommit( PdaRun *pdaRun );
int isParserStopFinished( PdaRun *pdaRun );
void pdaRunMatch(  PdaRun *pdaRun, Kid *tree, Kid *pattern );

/* Offset can be used to look at the next nextRegionInd. */
int pdaRunGetNextRegion( PdaRun *pdaRun, int offset );
int pdaRunGetNextPreRegion( PdaRun *pdaRun );

#define PcrStart         1
#define PcrDone          2
#define PcrReduction     3
#define PcrGeneration    4
#define PcrPreEof        5
#define PcrReverse       6

long parseToken( struct colm_program *prg, Tree **sp, PdaRun *pdaRun, 
		FsmRun *fsmRun, StreamImpl *inputStream, long entry );

long undoParse( Tree **sp, PdaRun *pdaRun, FsmRun *fsmRun, StreamImpl *inputStream, Tree *tree );

Head *streamPull( struct colm_program *prg, PdaRun *pdaRun, StreamImpl *is, long length );
Head *stringAllocPointer( struct colm_program *prg, const char *data, long length );

void streamPushText( StreamImpl *inputStream, const char *data, long length );
void streamPushTree( StreamImpl *inputStream, Tree *tree, int ignore );
void streamPushStream( StreamImpl *inputStream, Tree *tree );
void undoStreamPush( struct colm_program *prg, Tree **sp, StreamImpl *inputStream, long length );
void undoStreamAppend( struct colm_program *prg, Tree **sp, StreamImpl *inputStream, struct colm_tree *tree, long length );
Kid *makeTokenWithData( struct colm_program *prg, PdaRun *pdaRun, FsmRun *fsmRun, 
		StreamImpl *inputStream, int id, Head *tokdata );

void pushBinding( PdaRun *pdaRun, ParseTree *parseTree );

void executeGenerationAction( struct colm_program *prg, Tree **sp, FsmRun *fsmRun, PdaRun *pdaRun, 
		StreamImpl *inputStream, int frameId, Code *code, long id, Head *tokdata );
Kid *extractIgnore( PdaRun *pdaRun );
void clearIgnoreList( struct colm_program *prg, Tree **sp, Kid *kid );
long parseLoop( struct colm_program *prg, Tree **sp, PdaRun *pdaRun, 
		StreamImpl *inputStream, long entry );
Tree *getParsedRoot( PdaRun *pdaRun, int stop );

void resetToken( PdaRun *pdaRun );

#ifdef __cplusplus
}
#endif

#endif
