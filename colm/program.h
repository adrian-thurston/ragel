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

	void (*fsmExecute)( struct _FsmRun *fsmRun, struct _StreamImpl *inputStream );
	void (*sendNamedLangEl)( struct colm_program *prg, Tree **tree, struct _PdaRun *pdaRun,
			struct _FsmRun *fsmRun, struct _StreamImpl *inputStream );
	void (*initBindings)( struct _PdaRun *pdaRun );
	void (*popBinding)( struct _PdaRun *pdaRun, ParseTree *tree );

} RuntimeData;


typedef struct colm_program
{
	long activeRealm;

	int argc;
	const char **argv;

	unsigned char ctxDepParsing;
	RuntimeData *rtd;
	Tree *global;
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

	Kid *heap;

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
