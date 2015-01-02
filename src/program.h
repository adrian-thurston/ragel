/*
 *  Copyright 2007-2012 Adrian Thurston <thurston@complang.org>
 */

#ifndef __COLM_PROGRAM_H
#define __COLM_PROGRAM_H

#include <colm/pdarun.h>

typedef struct ColmStackBlock
{
	Tree **data;
	int len;
	int offset;
	struct ColmStackBlock *next;
} StackBlock;

typedef struct colm_sections
{
	LangElInfo *lelInfo;
	long numLangEls;

	StructElInfo *selInfo;
	long numStructEls;

	ProdInfo *prodInfo;
	long numProds;

	RegionInfo *regionInfo;
	long numRegions;

	Code *rootCode;
	long rootCodeLen;
	long rootFrameId;

	FrameInfo *frameInfo;
	long numFrames;

	FunctionInfo *functionInfo;
	long numFunctions;

	PatConsInfo *patReplInfo;
	long numPatterns;

	PatConsNode *patReplNodes;
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
	long globalId;

	void (*fsmExecute)( struct _FsmRun *fsmRun, struct _StreamImpl *inputStream );
	void (*sendNamedLangEl)( struct colm_program *prg, Tree **tree, struct _PdaRun *pdaRun,
			struct _FsmRun *fsmRun, struct _StreamImpl *inputStream );
	void (*initBindings)( struct _PdaRun *pdaRun );
	void (*popBinding)( struct _PdaRun *pdaRun, ParseTree *tree );

} RuntimeData;

typedef struct colm_heap_list
{
	struct colm_struct *head;
	struct colm_struct *tail;
} HeapList;

typedef struct colm_program
{
	long activeRealm;

	int argc;
	const char **argv;

	unsigned char ctxDepParsing;
	RuntimeData *rtd;
	struct colm_struct *global;
	int induceExit;
	int exitStatus;

	PoolAlloc kidPool;
	PoolAlloc treePool;
	PoolAlloc parseTreePool;
	PoolAlloc listElPool;
	PoolAlloc mapElPool;
	PoolAlloc headPool;
	PoolAlloc locationPool;

	Tree *trueVal;
	Tree *falseVal;

	Kid *origHeap;

	struct colm_heap_list heap;

	Stream *stdinVal;
	Stream *stdoutVal;
	Stream *stderrVal;

	Tree *error;

	RunBuf *allocRunBuf;

	/* Current stack block limits. Changed when crossing block boundaries. */
	Tree **sb_beg;
	Tree **sb_end;
	long sb_total;
	StackBlock *reserve;

	StackBlock *stackBlock;
	Tree **stackRoot;

	/* Returned value for main program and any exported functions. */
	Tree *returnVal;
} Program;

#endif
