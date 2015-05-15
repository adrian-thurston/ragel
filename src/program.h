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

struct stack_block
{
	Tree **data;
	int len;
	int offset;
	struct stack_block *next;
};

struct colm_sections
{
	struct lang_el_info *lelInfo;
	long numLangEls;

	struct struct_el_info *selInfo;
	long numStructEls;

	struct prod_info *prodInfo;
	long numProds;

	struct region_info *regionInfo;
	long numRegions;

	code_t *rootCode;
	long rootCodeLen;
	long rootFrameId;

	struct frame_info *frameInfo;
	long numFrames;

	struct function_info *functionInfo;
	long numFunctions;

	struct pat_cons_info *patReplInfo;
	long numPatterns;

	struct pat_cons_node *patReplNodes;
	long numPatternNodes;

	struct generic_info *genericInfo;
	long numGenerics;

	long argvGenericId;

	const char **litdata;
	long *litlen;
	Head **literals;
	long numLiterals;

	CaptureAttr *captureAttr;
	long numCapturedAttr;

	struct fsm_tables *fsmTables;
	struct pda_tables *pdaTables;
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
	long argvElId;

	void (*fsm_execute)( struct pda_run *pdaRun, struct stream_impl *inputStream );
	void (*sendNamedLangEl)( struct colm_program *prg, Tree **tree,
			struct pda_run *pdaRun, struct stream_impl *inputStream );
	void (*initBindings)( struct pda_run *pdaRun );
	void (*popBinding)( struct pda_run *pdaRun, ParseTree *tree );

};

struct heap_list
{
	struct colm_struct *head;
	struct colm_struct *tail;
};

struct colm_program
{
	long activeRealm;

	int argc;
	const char **argv;

	unsigned char ctxDepParsing;
	struct colm_sections *rtd;
	struct colm_struct *global;
	int induceExit;
	int exitStatus;

	struct pool_alloc kidPool;
	struct pool_alloc treePool;
	struct pool_alloc parseTreePool;
	struct pool_alloc headPool;
	struct pool_alloc locationPool;

	Tree *trueVal;
	Tree *falseVal;

	struct heap_list heap;

	Stream *stdinVal;
	Stream *stdoutVal;
	Stream *stderrVal;

	Tree *error;

	RunBuf *allocRunBuf;

	/* Current stack block limits. Changed when crossing block boundaries. */
	Tree **sb_beg;
	Tree **sb_end;
	long sb_total;
	struct stack_block *reserve;
	struct stack_block *stackBlock;
	Tree **stackRoot;

	/* Returned value for main program and any exported functions. */
	Tree *returnVal;
};

#endif
