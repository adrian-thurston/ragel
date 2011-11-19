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

#ifndef __COLM_PDARUN_H
#define __COLM_PDARUN_H

#include <colm/input.h>
#include <colm/fsmrun.h>
#include <colm/defs.h>
#include <colm/tree.h>

#ifdef __cplusplus
extern "C" {
#endif

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
	long ntrees;
	long frameSize;
} FunctionInfo;

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
	long prodNum;
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
	const char *nameNonLit;
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
	Kid *stackTop;
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

	Kid *accumIgnore;

	Kid *btPoint;

	struct Bindings *bindings;

	int revertOn;

	Tree *context;

	int stop;
	int parseError;

	long consumed;
	long targetConsumed;

	/*
	 * Data we added when refactoring the parsing engine into a coroutine.
	 */

	Kid *input1;
	FrameInfo *fi;
	int reduction;
	Kid *redLel;
	int curState;
	Kid *lel;
	struct _Execution *exec;
	int triggerUndo;

	int tokenId;
	Head *tokdata;
	int frameId;
	Kid *ignore2;
	Kid *ignore3;
	int next;
	Kid *undoLel;
	Kid *ignore4;
	Kid *ignore5;
	Kid *ignore6;
} PdaRun;

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

int makeReverseCode( RtCodeVect *all, RtCodeVect *reverseCode );

void initPdaRun( PdaRun *pdaRun, struct ColmProgram *prg, PdaTables *tables,
		FsmRun *fsmRun, int parserId, long stopTarget, int revertOn, Tree *context );
void clearPdaRun( struct ColmProgram *prg, Tree **root, PdaRun *pdaRun );

void clearContext( PdaRun *pdaRun, Tree **sp );
Kid *extractIgnore( PdaRun *pdaRun );
long stackTopTarget( struct ColmProgram *prg, PdaRun *pdaRun );
void runCommit( PdaRun *pdaRun );
int isParserStopFinished( PdaRun *pdaRun );
void pdaRunMatch(  PdaRun *pdaRun, Kid *tree, Kid *pattern );

/* Offset can be used to look at the next nextRegionInd. */
int pdaRunGetNextRegion( PdaRun *pdaRun, int offset );

#define PcrStart        1
#define PcrDone         2
#define PcrReduction    3
#define PcrGeneration   4
#define PcrPreEof       5
#define PcrRevIgnore    6
#define PcrRevToken     11
#define PcrRevReduction 12

long parseToken( struct ColmProgram *prg, Tree **sp, PdaRun *pdaRun, 
		FsmRun *fsmRun, InputStream *inputStream, long entry );

long undoParse( Tree **sp, PdaRun *pdaRun, FsmRun *fsmRun, InputStream *inputStream, Tree *tree );

Head *streamPull( struct ColmProgram *prg, FsmRun *fsmRun, InputStream *inputStream, long length );
Head *stringAllocPointer( struct ColmProgram *prg, const char *data, long length );

void streamPushText( InputStream *inputStream, const char *data, long length );
void streamPushTree( InputStream *inputStream, Tree *tree, int ignore );
void undoStreamPush( struct ColmProgram *prg, Tree **sp, InputStream *inputStream, long length );
void undoStreamAppend( struct ColmProgram *prg, Tree **sp, InputStream *inputStream, long length );
void sendBackText( FsmRun *fsmRun, InputStream *inputStream, const char *data, long length );
long sendBackIgnore( struct ColmProgram *prg, Tree **sp, PdaRun *pdaRun, FsmRun *fsmRun,
		InputStream *inputStream, Kid *ignoreKidList, long entry );
long sendBack( struct ColmProgram *prg, Tree **sp, PdaRun *pdaRun, FsmRun *fsmRun,
		InputStream *inputStream, Kid *input, long entry );
void unbind( struct ColmProgram *prg, Tree **sp, PdaRun *pdaRun, Tree *tree );
void queueBackTree( struct ColmProgram *prg, Tree **sp, PdaRun *pdaRun, FsmRun *fsmRun, InputStream *inputStream, Kid *input );
Kid *makeTokenWithData( struct ColmProgram *prg, PdaRun *pdaRun, FsmRun *fsmRun, 
		InputStream *inputStream, int id, Head *tokdata, int namedLangEl, int bindId );
void makeTokenPushBinding( PdaRun *pdaRun, int bindId, Tree *tree );
void executeGenerationAction( struct ColmProgram *prg, Tree **sp, FsmRun *fsmRun, PdaRun *pdaRun, 
		InputStream *inputStream, int frameId, Code *code, long id, Head *tokdata );
Kid *extractIgnore( PdaRun *pdaRun );
long sendBackQueuedIgnore( struct ColmProgram *prg, Tree **sp, InputStream *inputStream,
		FsmRun *fsmRun, PdaRun *pdaRun, long entry );
void clearIgnoreList( struct ColmProgram *prg, Tree **sp, Kid *kid );
Head *extractMatch( struct ColmProgram *prg, FsmRun *fsmRun, InputStream *inputStream );
Head *extractMatch( struct ColmProgram *prg, FsmRun *fsmRun, InputStream *inputStream );
void initInputStream( InputStream *in );
void newToken( struct ColmProgram *prg, PdaRun *pdaRun, FsmRun *fsmRun );
void breakRunBuf( FsmRun *fsmRun );
void fsmExecute( FsmRun *fsmRun, InputStream *inputStream );
Kid *sendNamedLangEl( struct ColmProgram *prg, Tree **sp, PdaRun *pdaRun, FsmRun *fsmRun, InputStream *inputStream );
long parseLoop( struct ColmProgram *prg, Tree **sp, PdaRun *pdaRun, 
		FsmRun *fsmRun, InputStream *inputStream, long entry );
void initBindings( PdaRun *pdaRun );
Tree *getParsedRoot( PdaRun *pdaRun, int stop );
void pushBtPoint( struct ColmProgram *prg, PdaRun *pdaRun );
void undoParseStream( struct ColmProgram *prg, Tree **sp, InputStream *inputStream, FsmRun *fsmRun, 
		PdaRun *pdaRun, long consumed );
void attachIgnore( struct ColmProgram *prg, Tree **sp, PdaRun *pdaRun, Kid *input );
void detachIgnores( struct ColmProgram *prg, Tree **sp, PdaRun *pdaRun, FsmRun *fsmRun, Kid *input );

#ifdef __cplusplus
}
#endif

#endif
