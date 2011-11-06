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

#include "config.h"
#include "debug.h"
#include "pdarun.h"
#include "fsmrun.h"
#include "bytecode.h"
#include "tree.h"
#include "pool.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#define true 1
#define false 0

#define act_sb 0x1
#define act_rb 0x2
#define lower 0x0000ffff
#define upper 0xffff0000
#define reject() induceReject = 1
#define pt(var) ((ParseTree*)(var))

#define read_word_p( i, p ) do { \
	i = ((Word)  p[0]); \
	i |= ((Word) p[1]) << 8; \
	i |= ((Word) p[2]) << 16; \
	i |= ((Word) p[3]) << 24; \
} while(0)

#define read_tree_p( i, p ) do { \
	Word w; \
	w = ((Word)  p[0]); \
	w |= ((Word) p[1]) << 8; \
	w |= ((Word) p[2]) << 16; \
	w |= ((Word) p[3]) << 24; \
	i = (Tree*)w; \
} while(0)

/* Offset can be used to look at the next nextRegionInd. */
int pdaRunGetNextRegion( PdaRun *pdaRun, int offset )
{
	return pdaRun->tables->tokenRegions[pdaRun->nextRegionInd+offset];
}

Tree *getParsedRoot( PdaRun *pdaRun, int stop )
{
	if ( pdaRun->parseError )
		return 0;
	else if ( stop )
		return pdaRun->stackTop->tree;
	else
		return pdaRun->stackTop->next->tree;
}

void clearPdaRun( Program *prg, Tree **sp, PdaRun *pdaRun )
{
	/* Traverse the stack downreffing. */
	Kid *kid = pdaRun->stackTop;
	while ( kid != 0 ) {
		Kid *next = kid->next;
		treeDownref( prg, sp, kid->tree );
		kidFree( prg, kid );
		kid = next;
	}
	pdaRun->stackTop = 0;

	/* Traverse the token list downreffing. */
	Ref *ref = pdaRun->tokenList;
	while ( ref != 0 ) {
		Ref *next = ref->next;
		kidFree( prg, (Kid*)ref );
		ref = next;
	}
	pdaRun->tokenList = 0;

	/* Traverse the btPoint list downreffing */
	Kid *btp = pdaRun->btPoint;
	while ( btp != 0 ) {
		Kid *next = btp->next;
		treeDownref( prg, sp, btp->tree );
		kidFree( prg, (Kid*)btp );
		btp = next;
	}
	pdaRun->btPoint = 0;

	/* Clear out any remaining ignores. */
	Kid *ignore = pdaRun->accumIgnore;
	while ( ignore != 0 ) {
		Kid *next = ignore->next;
		treeDownref( prg, sp, ignore->tree );
		kidFree( prg, (Kid*)ignore );
		ignore = next;
	}

	if ( pdaRun->context != 0 )
		treeDownref( prg, sp, pdaRun->context );

	rcodeDownrefAll( prg, sp, &pdaRun->reverseCode );
	rtCodeVectEmpty( &pdaRun->reverseCode );
	rtCodeVectEmpty( &pdaRun->rcodeCollect );
}

int isParserStopFinished( PdaRun *pdaRun )
{
	int done = 
			pdaRun->stackTop->next != 0 && 
			pdaRun->stackTop->next->next == 0 &&
			pdaRun->stackTop->tree->id == pdaRun->stopTarget;
	return done;
}

void initPdaRun( PdaRun *pdaRun, Program *prg, PdaTables *tables,
		FsmRun *fsmRun, int parserId, long stopTarget, int revertOn, Tree *context )
{
	memset( pdaRun, 0, sizeof(PdaRun) );
	pdaRun->tables = tables;
	pdaRun->parserId = parserId;
	pdaRun->stopTarget = stopTarget;
	pdaRun->revertOn = revertOn;
	pdaRun->targetConsumed = -1;

	debug( REALM_PARSE, "initializing PdaRun\n" );

	/* FIXME: need the right one here. */
	pdaRun->cs = prg->rtd->startStates[pdaRun->parserId];

	/* Init the element allocation variables. */
	pdaRun->stackTop = kidAllocate( prg );
	pdaRun->stackTop->tree = (Tree*)parseTreeAllocate( prg );
	pdaRun->stackTop->tree->flags |= AF_PARSE_TREE;

	pt(pdaRun->stackTop->tree)->state = -1;
	pdaRun->stackTop->tree->refs = 1;
	pdaRun->numRetry = 0;
	pdaRun->nextRegionInd = pdaRun->tables->tokenRegionInds[pdaRun->cs];
	pdaRun->stopParsing = false;
	pdaRun->accumIgnore = 0;
	pdaRun->btPoint = 0;

	initBindings( pdaRun );

	initRtCodeVect( &pdaRun->reverseCode );
	initRtCodeVect( &pdaRun->rcodeCollect );

	pdaRun->context = splitTree( prg, context );
	pdaRun->parseError = 0;
	pdaRun->input = 0;
}

long stackTopTarget( Program *prg, PdaRun *pdaRun )
{
	long state;
	if ( pt(pdaRun->stackTop->tree)->state < 0 )
		state = prg->rtd->startStates[pdaRun->parserId];
	else {
		state = pdaRun->tables->targs[(int)pdaRun->tables->indicies[pdaRun->tables->offsets[
				pt(pdaRun->stackTop->tree)->state] + 
				(pdaRun->stackTop->tree->id - pdaRun->tables->keys[pt(pdaRun->stackTop->tree)->state<<1])]];
	}
	return state;
}

/*
 * Local commit:
 * 		-clears reparse flags underneath
 * 		-must be possible to backtrack after
 * Global commit (revertOn)
 * 		-clears all reparse flags
 * 		-must be possible to backtrack after
 * Global commit (!revertOn)
 * 		-clears all reparse flags
 * 		-clears all 'parsed' reverse code
 * 		-clears all reverse code
 * 		-clears all alg structures
 */

int beenCommitted( Kid *kid )
{
	return kid->tree->flags & AF_COMMITTED;
}

Code *backupOverRcode( Code *rcode )
{
	Word len;
	rcode -= SIZEOF_WORD;
	read_word_p( len, rcode );
	rcode -= len;
	return rcode;
}

/* The top level of the stack is linked right-to-left. Trees underneath are
 * linked left-to-right. */
void commitKid( Program *prg, PdaRun *pdaRun, Tree **root, Kid *lel, Code **rcode, long *causeReduce )
{
	Tree *tree = 0;
	Tree **sp = root;
	Tree *restore = 0;

head:
	/* Commit */
	debug( REALM_PARSE, "commit: visiting %s\n",
			prg->rtd->lelInfo[lel->tree->id].name );

	/* Load up the parsed tree. */
	tree = lel->tree;

	/* Check for reverse code. */
	restore = 0;
	if ( tree->flags & AF_HAS_RCODE ) {
		/* If tree caused some reductions, now is not the right time to backup
		 * over the reverse code. We need to backup over the reductions first. Store
		 * the count of the reductions and do it when the count drops to zero. */
		if ( pt(tree)->causeReduce > 0 ) {
			/* The top reduce block does not correspond to this alg. */
//			#ifdef COLM_LOG_PARSE
//			if ( colm_log_parse ) {
//				cerr << "commit: causeReduce found, delaying backup: " << 
//						(long)pt(tree)->causeReduce << endl;
//			}
//			#endif
			*causeReduce = pt(tree)->causeReduce;
		}
		else {
			*rcode = backupOverRcode( *rcode );

			if ( **rcode == IN_RESTORE_LHS ) {
//				#if COLM_LOG_PARSE
//				cerr << "commit: has restore_lhs" << endl;
//				#endif
				read_tree_p( restore, (*rcode+1) );
			}
		}
	}

	if ( restore != 0 )
		tree = restore;

	/* All the parse algorithm data except for the RCODE flag is in the
	 * original. That is why we restore first, then we can clear the retry
	 * values. */

	/* Check causeReduce, might be time to backup over the reverse code
	 * belonging to a nonterminal that caused previous reductions. */
	if ( *causeReduce > 0 && 
			tree->id >= prg->rtd->firstNonTermId &&
			!(tree->flags & AF_TERM_DUP) )
	{
		*causeReduce -= 1;

		if ( *causeReduce == 0 ) {
			debug( REALM_PARSE, "commit: causeReduce dropped to zero, backing up over rcode\n" );

			/* Cause reduce just dropped down to zero. */
			*rcode = backupOverRcode( *rcode );
		}
	}

	/* Reset retries. */
	if ( tree->flags & AF_PARSED ) {
		if ( pt(tree)->retry_lower > 0 ) {
			pdaRun->numRetry -= 1;
			pt(tree)->retry_lower = 0;
		}
		if ( pt(tree)->retry_upper > 0 ) {
			pdaRun->numRetry -= 1;
			pt(tree)->retry_upper = 0;
		}
	}
	tree->flags |= AF_COMMITTED;

	/* Do not recures on trees that are terminal dups. */
	if ( !(tree->flags & AF_TERM_DUP) && treeChild( prg, tree ) != 0 ) {
		vm_push( (Tree*)lel );
		lel = treeChild( prg, tree );

		if ( lel != 0 ) {
			while ( lel != 0 ) {
				vm_push( (Tree*)lel );
				lel = lel->next;
			}
		}
	}

backup:
	if ( sp != root ) {
		Kid *next = (Kid*)vm_pop();
		if ( next->next == lel ) {
			/* Moving backwards. */
			lel = next;

			if ( !beenCommitted( lel ) )
				goto head;
		}
		else {
			/* Moving upwards. */
			lel = next;
		}

		goto backup;
	}

	pdaRun->numRetry = 0;
	assert( sp == root );
}

void commitFull( Program *prg, Tree **sp, PdaRun *pdaRun, long causeReduce )
{
//	#ifdef COLM_LOG_PARSE
//	if ( colm_log_parse ) {
//		cerr << "running full commit" << endl;
//	}
//	#endif
	
	Kid *kid = pdaRun->stackTop;
	Code *rcode = pdaRun->reverseCode.data + pdaRun->reverseCode.tabLen;

	/* The top level of the stack is linked right to left. This is the
	 * traversal order we need for committing. */
	while ( kid != 0 && !beenCommitted( kid ) ) {
		commitKid( prg, pdaRun, sp, kid, &rcode, &causeReduce );
		kid = kid->next;
	}

	/* We cannot always clear all the rcode here. We may need to backup over
	 * the parse statement. We depend on the context flag. */
	if ( !pdaRun->revertOn )
		rcodeDownrefAll( prg, sp, &pdaRun->reverseCode );
}

void pushBtPoint( Program *prg, PdaRun *pdaRun, Tree *tree )
{
	Kid *kid = kidAllocate( prg );
	kid->tree = tree;
	treeUpref( tree );
	kid->next = pdaRun->btPoint;
	pdaRun->btPoint = kid;
}

/*
 * shift:         retry goes into lower of shifted node.
 * reduce:        retry goes into upper of reduced node.
 * shift-reduce:  cannot be a retry
 */

enum ParseTokenResult parseToken( Program *prg, Tree **sp, PdaRun *pdaRun,
		FsmRun *fsmRun, InputStream *inputStream, enum ParseTokenEntry entry )
{
	int pos;
	unsigned int *action;
	int rhsLen;
	int owner;
	int induceReject;
	int indPos;
	LangElInfo *lelInfo = prg->rtd->lelInfo;

	/* The scanner will send a null token if it can't find a token. */
	switch ( entry ) {
		case PteError: {
			/* Grab the most recently accepted item. */
			assert( pdaRun->input == 0 );
			pushBtPoint( prg, pdaRun, pdaRun->tokenList->kid->tree );
			goto parse_error;
		}
		case PteReduction:
			goto pteReduction;
		case PteToken:
			/* Fall through */
			break;
	}

	/* The tree we are given must be * parse tree size. It also must have at
	 * least one reference. */
	assert( pdaRun->input->tree->flags & AF_PARSE_TREE );
	assert( pdaRun->input->tree->refs > 0 );

	/* This will cause input to be lost. This 
	 * path should be traced. */
	if ( pdaRun->cs < 0 )
		return PtrDone;

	pt(pdaRun->input->tree)->region = pdaRun->nextRegionInd;
	pt(pdaRun->input->tree)->state = pdaRun->cs;
	if ( pdaRun->tables->tokenRegions[pt(pdaRun->input->tree)->region+1] != 0 )
		pdaRun->numRetry += 1;

again:
	if ( pdaRun->input == 0 )
		goto _out;

	pdaRun->lel = pdaRun->input;
	pdaRun->curState = pdaRun->cs;

	/* This can disappear. An old experiment. */
	if ( lelInfo[pdaRun->lel->tree->id].ignore ) {
		/* Consume. */
		pdaRun->input = pdaRun->input->next;

		Tree *stTree = pdaRun->stackTop->tree;
		if ( stTree->id == LEL_ID_IGNORE_LIST ) {
			pdaRun->lel->next = stTree->child;
			stTree->child = pdaRun->lel;
		}
		else {
			Kid *kid = kidAllocate( prg );
			IgnoreList *ignoreList = ilAllocate( prg );
			ignoreList->generation = prg->nextIlGen++;

			kid->tree = (Tree*)ignoreList;
			kid->tree->id = LEL_ID_IGNORE_LIST;
			kid->tree->refs = 1;
			kid->tree->child = pdaRun->lel;
			pdaRun->lel->next = 0;

			kid->next = pdaRun->stackTop;
			pdaRun->stackTop = kid;
		}

		/* Note: no state change. */
		goto again;
	}

	if ( pdaRun->lel->tree->id < pdaRun->tables->keys[pdaRun->curState<<1] ||
			pdaRun->lel->tree->id > pdaRun->tables->keys[(pdaRun->curState<<1)+1] ) {
		pushBtPoint( prg, pdaRun, pdaRun->lel->tree );
		goto parse_error;
	}

	indPos = pdaRun->tables->offsets[pdaRun->curState] + 
		(pdaRun->lel->tree->id - pdaRun->tables->keys[pdaRun->curState<<1]);

	owner = pdaRun->tables->owners[indPos];
	if ( owner != pdaRun->curState ) {
		pushBtPoint( prg, pdaRun, pdaRun->lel->tree );
		goto parse_error;
	}

	pos = pdaRun->tables->indicies[indPos];
	if ( pos < 0 ) {
		pushBtPoint( prg, pdaRun, pdaRun->lel->tree );
		goto parse_error;
	}

	/* Checking complete. */

	induceReject = false;
	pdaRun->cs = pdaRun->tables->targs[pos];
	action = pdaRun->tables->actions + pdaRun->tables->actInds[pos];
	if ( pt(pdaRun->lel->tree)->retry_lower )
		action += pt(pdaRun->lel->tree)->retry_lower;

	if ( *action & act_sb ) {
		debug( REALM_PARSE, "shifted: %s\n", 
				prg->rtd->lelInfo[pt(pdaRun->lel->tree)->id].name );
		/* Consume. */
		pdaRun->input = pdaRun->input->next;

		pt(pdaRun->lel->tree)->state = pdaRun->curState;
		pdaRun->lel->next = pdaRun->stackTop;
		pdaRun->stackTop = pdaRun->lel;

		/* Record the last shifted token. Need this for attaching ignores. */
		if ( pdaRun->lel->tree->id < prg->rtd->firstNonTermId ) {
			Ref *ref = (Ref*)kidAllocate( prg );
			ref->kid = pdaRun->lel;
			//treeUpref( pdaRun->lel->tree );
			ref->next = pdaRun->tokenList;
			pdaRun->tokenList = ref;
		}

		/* If shifting a termDup then change it to the nonterm. */
		if ( pdaRun->lel->tree->id < prg->rtd->firstNonTermId &&
				prg->rtd->lelInfo[pdaRun->lel->tree->id].termDupId > 0 )
		{
			pdaRun->lel->tree->id = prg->rtd->lelInfo[pdaRun->lel->tree->id].termDupId;
			pdaRun->lel->tree->flags |= AF_TERM_DUP;
		}

		if ( action[1] == 0 )
			pt(pdaRun->lel->tree)->retry_lower = 0;
		else {
			debug( REALM_PARSE, "retry: %p\n", pdaRun->stackTop );
			pt(pdaRun->lel->tree)->retry_lower += 1;
			assert( pt(pdaRun->lel->tree)->retry_upper == 0 );
			/* FIXME: Has the retry already been counted? */
			pdaRun->numRetry += 1; 
		}
	}

	if ( pdaRun->tables->commitLen[pos] != 0 ) {
		long causeReduce = 0;
		if ( pdaRun->input != 0 ) { 
			if ( pdaRun->input->tree->flags & AF_HAS_RCODE )
				causeReduce = pt(pdaRun->input->tree)->causeReduce;
		}
		commitFull( prg, sp, pdaRun, causeReduce );
	}

	if ( *action & act_rb ) {
		int r, objectLength;
		Kid *last, *child, *attrs;

		pdaRun->reduction = *action >> 2;

		if ( pdaRun->input != 0 )
			pt(pdaRun->input->tree)->causeReduce += 1;

		pdaRun->redLel = kidAllocate( prg );
		pdaRun->redLel->tree = (Tree*)parseTreeAllocate( prg );
		pdaRun->redLel->tree->flags |= AF_PARSE_TREE;
		pdaRun->redLel->tree->refs = 1;
		pdaRun->redLel->tree->id = prg->rtd->prodInfo[pdaRun->reduction].lhsId;
		pdaRun->redLel->tree->prodNum = prg->rtd->prodInfo[pdaRun->reduction].prodNum;

		pdaRun->redLel->next = 0;
		pt(pdaRun->redLel->tree)->causeReduce = 0;
		pt(pdaRun->redLel->tree)->retry_lower = 0;
		pt(pdaRun->redLel->tree)->retry_upper = pt(pdaRun->lel->tree)->retry_lower;
		pt(pdaRun->lel->tree)->retry_lower = 0;

		/* Allocate the attributes. */
		objectLength = prg->rtd->lelInfo[pdaRun->redLel->tree->id].objectLength;
		attrs = allocAttrs( prg, objectLength );

		/* Build the list of children. */
		Kid *realChild = 0;
		rhsLen = prg->rtd->prodInfo[pdaRun->reduction].length;
		child = last = 0;
		for ( r = 0;; ) {
			if ( r == rhsLen && pdaRun->stackTop->tree->id != LEL_ID_IGNORE_LIST )
				break;

			child = pdaRun->stackTop;
			pdaRun->stackTop = pdaRun->stackTop->next;
			child->next = last;
			last = child;
			
			if ( child->tree->id != LEL_ID_IGNORE_LIST ) {
				/* Count it only if it is not an ignore token. */
				r++;
			}

			if ( child->tree->id != LEL_ID_IGNORE_LIST )
				realChild = child;
		}

		pdaRun->redLel->tree->child = kidListConcat( attrs, child );

		debug( REALM_PARSE, "reduced: %s rhsLen %d\n",
				prg->rtd->prodInfo[pdaRun->reduction].name, rhsLen );
		if ( action[1] == 0 )
			pt(pdaRun->redLel->tree)->retry_upper = 0;
		else {
			pt(pdaRun->redLel->tree)->retry_upper += 1;
			assert( pt(pdaRun->lel->tree)->retry_lower == 0 );
			pdaRun->numRetry += 1;
			debug( REALM_PARSE, "retry: %p\n", pdaRun->redLel );
		}

		/* When the production is of zero length we stay in the same state.
		 * Otherwise we use the state stored in the first child. */
		pdaRun->cs = rhsLen == 0 ? pdaRun->curState : pt(realChild->tree)->state;

		assert( pdaRun->redLel->tree->refs == 1 );

		if ( prg->ctxDepParsing && prg->rtd->prodInfo[pdaRun->reduction].frameId >= 0 ) {
			/* Frame info for reduction. */
			pdaRun->fi = &prg->rtd->frameInfo[prg->rtd->prodInfo[pdaRun->reduction].frameId];

			return PtrReduction;
			pteReduction:

			if ( prg->induceExit )
				goto fail;

			/* If the lhs was saved and it changed then we need to restore the
			 * original upon backtracking, otherwise downref since we took a
			 * copy above. */
			if ( pdaRun->exec->parsed != 0 && pdaRun->exec->parsed != pdaRun->redLel->tree ) {
				debug( REALM_PARSE, "lhs tree was modified, adding a restore instruction\n" );

				/* Transfer the lhs from the environment to redLel. */
				pdaRun->redLel->tree = prepParseTree( prg, sp, pdaRun->exec->lhs );
				treeUpref( pdaRun->redLel->tree );
				treeDownref( prg, sp, pdaRun->exec->lhs );

				append( &pdaRun->rcodeCollect, IN_RESTORE_LHS );
				appendWord( &pdaRun->rcodeCollect, (Word)pdaRun->exec->parsed );
				append( &pdaRun->rcodeCollect, SIZEOF_CODE + SIZEOF_WORD );
			}

			/* Pull out the reverse code, if any. */
			int hasrcode = makeReverseCode( &pdaRun->reverseCode, &pdaRun->rcodeCollect );
			if ( hasrcode )
				pdaRun->redLel->tree->flags |= AF_HAS_RCODE;

			/* Perhaps the execution environment is telling us we need to
			 * reject the reduction. */
			induceReject = pdaRun->exec->reject;
		}

		/* If the left hand side was replaced then the only parse algorithm
		 * data that is contained in it will the AF_HAS_RCODE flag. Everthing
		 * else will be in the original. This requires that we restore first
		 * when going backwards and when doing a commit. */

		if ( induceReject ) {
			debug( REALM_PARSE, "error induced during reduction of %s\n",
					prg->rtd->lelInfo[pdaRun->redLel->tree->id].name );
			pt(pdaRun->redLel->tree)->state = pdaRun->curState;
			pdaRun->redLel->next = pdaRun->stackTop;
			pdaRun->stackTop = pdaRun->redLel;
			pushBtPoint( prg, pdaRun, pdaRun->lel->tree );
			goto parse_error;
		}

		pdaRun->redLel->next = pdaRun->input;
		pdaRun->input = pdaRun->redLel;
	}

	goto again;

parse_error:
	debug( REALM_PARSE, "hit error, backtracking\n" );

	if ( pdaRun->numRetry == 0 )
		goto fail;

	while ( 1 ) {
		if ( pdaRun->input != 0 ) {
			assert( pt(pdaRun->input->tree)->retry_upper == 0 );

			if ( pt(pdaRun->input->tree)->retry_lower != 0 ) {
				debug( REALM_PARSE, "found retry targ: %p\n", pdaRun->input );

				pdaRun->numRetry -= 1;
				pdaRun->cs = pt(pdaRun->input->tree)->state;
				goto again;
			}

			/* If there is no retry and there are no reductions caused by the
			 * current input token then we are finished with it. Send it back. */
			if ( pt(pdaRun->input->tree)->causeReduce == 0 ) {
				int next = pt(pdaRun->input->tree)->region + 1;

				queueBackTree( prg, sp, pdaRun, fsmRun, inputStream, pdaRun->input );
				pdaRun->input = 0;
				if ( pdaRun->tables->tokenRegions[next] != 0 ) {
					debug( REALM_PARSE, "found a new region\n" );
					pdaRun->numRetry -= 1;
					pdaRun->cs = stackTopTarget( prg, pdaRun );
					pdaRun->nextRegionInd = next;
					return PtrDone;
				}

				if ( pdaRun->stop ) {
					debug( REALM_PARSE, "stopping the backtracking, consumed is %d", pdaRun->consumed );

					pdaRun->cs = stackTopTarget( prg, pdaRun );
					goto _out;
				}
			}
		}

		/* Now it is time to undo something. Pick an element from the top of
		 * the stack. */
		Kid *undoLel = pdaRun->stackTop;

		/* Check if we've arrived at the stack sentinal. This guard is
		 * here to allow us to initially set numRetry to one to cause the
		 * parser to backup all the way to the beginning when an error
		 * occurs. */
		if ( undoLel->next == 0 )
			break;

		/* Either we are dealing with a terminal that was
		 * shifted or a nonterminal that was reduced. */
		if ( pdaRun->stackTop->tree->id < prg->rtd->firstNonTermId || 
				(pdaRun->stackTop->tree->flags & AF_TERM_DUP) )
		{
			debug( REALM_PARSE, "backing up over effective terminal: %s\n",
						prg->rtd->lelInfo[pdaRun->stackTop->tree->id].name );

			/* Pop the item from the stack. */
			pdaRun->stackTop = pdaRun->stackTop->next;

			/* Undo the translation from termDup. */
			if ( undoLel->tree->flags & AF_TERM_DUP ) {
				undoLel->tree->id = prg->rtd->lelInfo[undoLel->tree->id].termDupId;
				undoLel->tree->flags &= ~AF_TERM_DUP;
			}

			/* Queue it as next input item. */
			undoLel->next = pdaRun->input;
			pdaRun->input = undoLel;

			/* Record the last shifted token. Need this for attaching ignores. */
			Ref *ref = pdaRun->tokenList;
			pdaRun->tokenList = ref->next;
			//treeDownref( prg, sp, kid->tree );
			kidFree( prg, (Kid*)ref );
		}
		else {
			debug( REALM_PARSE, "backing up over non-terminal: %s\n",
					prg->rtd->lelInfo[pdaRun->stackTop->tree->id].name );

			/* Check for an execution environment. */
			if ( undoLel->tree->flags & AF_HAS_RCODE ) {
				Execution exec;
				initReverseExecution( &exec, prg, &pdaRun->rcodeCollect, 
						pdaRun, fsmRun, -1, 0, 0, 0, 0, fsmRun->mark );

				/* Do the reverse exeuction. */
				reverseExecution( &exec, sp, &pdaRun->reverseCode );
				undoLel->tree->flags &= ~AF_HAS_RCODE;

				if ( exec.lhs != 0 ) {
					/* Get the lhs, it may have been reverted. */
					treeDownref( prg, sp, undoLel->tree );
					undoLel->tree = exec.lhs;
				}
			}

			/* Only the RCODE flag was in the replaced lhs. All the rest is in
			 * the the original. We read it after restoring. */

			/* Warm fuzzies ... */
			assert( undoLel == pdaRun->stackTop );

			/* Take the nonterm off the stack. */
			pdaRun->stackTop = pdaRun->stackTop->next;

			/* Extract the real children from the child list. */
			Kid *first = treeExtractChild( prg, undoLel->tree );

			/* Walk the child list and and push the items onto the parsing
			 * stack one at a time. */
			while ( first != 0 ) {
				/* Get the next item ahead of time. */
				Kid *next = first->next;

				/* Push onto the stack. */
				first->next = pdaRun->stackTop;
				pdaRun->stackTop = first;

				first = next;
			}

			/* If there is an input queued, this is one less reduction it has
			 * caused. */
			if ( pdaRun->input != 0 )
				pt(pdaRun->input->tree)->causeReduce -= 1;

			if ( pt(undoLel->tree)->retry_upper != 0 ) {
				/* There is always an input item here because reduce
				 * conflicts only happen on a lookahead character. */
				assert( pdaRun->input != undoLel );
				assert( pdaRun->input != 0 );
				assert( pt(undoLel->tree)->retry_lower == 0 );
				assert( pt(pdaRun->input->tree)->retry_upper == 0 );

				/* Transfer the retry from undoLel to input. */
				pt(pdaRun->input->tree)->retry_lower = pt(undoLel->tree)->retry_upper;
				pt(pdaRun->input->tree)->retry_upper = 0;
				pt(pdaRun->input->tree)->state = stackTopTarget( prg, pdaRun );
			}

			/* Free the reduced item. */
			treeDownref( prg, sp, undoLel->tree );
			kidFree( prg, undoLel );
		}
	}

fail:
	pdaRun->cs = -1;
	pdaRun->parseError = 1;

	/* If we failed parsing on tree we must free it. The caller expected us to
	 * either consume it or send it back to the input. */
	if ( pdaRun->input != 0 ) {
		treeDownref( prg, sp, pdaRun->input->tree );
		kidFree( prg, pdaRun->input );
		pdaRun->input = 0;
	}

	/* FIXME: do we still need to fall through here? A fail is permanent now,
	 * no longer called into again. */

_out:
	pdaRun->nextRegionInd = pdaRun->tables->tokenRegionInds[pdaRun->cs];
	return PtrDone;
}
