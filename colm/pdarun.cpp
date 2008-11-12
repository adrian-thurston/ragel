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

#include <iostream>
#include <errno.h>
#include <stdio.h>
#include <fstream>
#include <string>

#include "config.h"
#include "pdarun.h"
#include "fsmrun.h"

using std::ostream;
using std::cout;
using std::cerr;
using std::endl;

#define act_sb 0x1
#define act_rb 0x2
#define lower 0x0000ffff
#define upper 0xffff0000
#define reject() induceReject = 1

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


Tree *PdaRun::getParsedRoot( bool stop )
{
	return stop ? stackTop->tree : stackTop->next->tree;
}

void PdaRun::clean()
{
	/* Traverse the stack, cleaning. */
	Kid *kid = stackTop;
	while ( kid != 0 ) {
		Kid *next = kid->next;
		tree_downref( prg, root, kid->tree );
		prg->kidPool.free( kid );
		kid = next;
	}
}

bool PdaRun::isParserStopFinished()
{
	bool done = 
			stackTop->next != 0 && 
			stackTop->next->next == 0 &&
			stackTop->tree->id == stopTarget;
	return done;
}

void PdaRun::init()
{
	cs = tables->startState;

	/* Init the element allocation variables. */
	stackTop = prg->kidPool.allocate();
	stackTop->tree = prg->treePool.allocate();
	stackTop->tree->alg = prg->algPool.allocate();

	stackTop->tree->alg->state = -1;
	stackTop->tree->refs = 1;
	numRetry = 0;
	errCount = 0;
	nextRegionInd = tables->tokenRegionInds[cs];
	stopParsing = false;
	accumIgnore = 0;

	/* Bindings are indexed at 1. Need a no-binding. */
	bindings.push(0);

	allReverseCode = new CodeVect;
}

long PdaRun::stackTopTarget()
{
	long state;
	if ( stackTop->tree->alg->state < 0 )
		state = tables->startState;
	else {
		state = tables->targs[(int)tables->indicies[tables->offsets[
				stackTop->tree->alg->state] + 
				(stackTop->tree->id - tables->keys[stackTop->tree->alg->state<<1])]];
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

bool been_committed( Kid *kid )
{
	return kid->tree->alg->flags & AF_COMMITTED;
}

Code *backup_over_rcode( Code *rcode )
{
	Word len;
	rcode -= 4;
	read_word_p( len, rcode );
	rcode -= len;
	return rcode;
}

/* The top level of the stack is linked right-to-left. Trees underneath are
 * linked left-to-right. */
void commit_kid( PdaRun *parser, Tree **root, Kid *lel, Code *&rcode, long &causeReduce )
{
	Alg *alg = 0;
	Tree *tree = 0;
	Tree **sp = root;
	Tree *restore = 0;

head:
	/* Commit */
	#ifdef COLM_LOG_PARSE
	cerr << "commit: visiting " << 
			parser->prg->rtd->lelInfo[lel->tree->id].name << endl;
	#endif

	/* Load up the parsed tree. */
	tree = lel->tree;
	alg = tree->alg;

	/* Check for reverse code. */
	restore = 0;
	if ( alg->flags & AF_HAS_RCODE ) {
		/* If tree caused some reductions, now is not the right time to backup
		 * over the reverse code. We need to backup over the reductions first. Store
		 * the count of the reductions and do it when the count drops to zero. */
		if ( alg->causeReduce > 0 ) {
			/* The top reduce block does not correspond to this alg. */
			#ifdef COLM_LOG_PARSE
			cerr << "commit: causeReduce found, delaying backup: " << 
					(long)alg->causeReduce << endl;
			#endif
			causeReduce = alg->causeReduce;
		}
		else {
			rcode = backup_over_rcode( rcode );

			if ( *rcode == IN_RESTORE_LHS ) {
				#if COLM_LOG_PARSE
				cerr << "commit: has restore_lhs" << endl;
				#endif
				read_tree_p( restore, (rcode+1) );
			}
		}
	}

	if ( restore != 0 )
		tree = restore;

	/* Check causeReduce, might be time to backup over the reverse code
	 * belonging to a nonterminal that caused previous reductions. */
	if ( causeReduce > 0 && 
			tree->id >= parser->tables->rtd->firstNonTermId &&
			!(alg->flags & AF_GENERATED) )
	{
		causeReduce -= 1;

		if ( causeReduce == 0 ) {
			#ifdef COLM_LOG_PARSE
			cerr << "commit: causeReduce dropped to zero, backing up over rcode" << endl;
			#endif

			/* Cause reduce just dropped down to zero. */
			rcode = backup_over_rcode( rcode );
		}
	}

	/* Reset retries. */
	if ( alg->retry_lower > 0 ) {
		parser->numRetry -= 1;
		alg->retry_lower = 0;
	}
	if ( alg->retry_upper > 0 ) {
		parser->numRetry -= 1;
		alg->retry_upper = 0;
	}
	alg->flags |= AF_COMMITTED;

	/* Recurse only on non-generated trees. */
	if ( !(alg->flags & AF_GENERATED) && tree_child( parser->prg, tree ) != 0 ) {
		vm_push( (Tree*)lel );
		lel = tree_child( parser->prg, tree );

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

			if ( !been_committed( lel ) )
				goto head;
		}
		else {
			/* Moving upwards. */
			lel = next;
		}

		goto backup;
	}

	parser->numRetry = 0;
	assert( sp == root );
}

void commit_full( PdaRun *parser, long causeReduce )
{
	#ifdef COLM_LOG_PARSE
	cerr << "running full commit" << endl;
	#endif
	
	Tree **sp = parser->root;
	Kid *kid = parser->stackTop;

	Code *rcode = parser->allReverseCode->data + parser->allReverseCode->length();

	/* The top level of the stack is linked right to left. This is the
	 * traversal order we need for committing. */
	while ( kid != 0 && !been_committed( kid ) ) {
		commit_kid( parser, sp, kid, rcode, causeReduce );
		kid = kid->next;
	}

	/* We cannot always clear all the rcode here. We may need to backup over
	 * the parse statement. We depend on the context flag. */
	if ( !parser->revertOn )
		rcode_downref_all( parser->prg, parser->root, parser->allReverseCode );
}


/*
 * shift:         retry goes into lower of shifted node.
 * reduce:        retry goes into upper of reduced node.
 * shift-reduce:  cannot be a retry
 */

void PdaRun::parseToken( Kid *input )
{
	int pos, targState;
	unsigned int *action;
	int rhsLen;
	Kid *lel;
	bool induceReject;

	/* The scanner will send a null token if it can't find a token. */
	if ( input == 0 )
		goto parseError;

	/* This will cause input to be lost. This 
	 * path should be Should be traced. */
	if ( cs < 0 )
		return;

	input->tree->alg->region = nextRegionInd;
	input->tree->alg->state = cs;
	if ( tables->tokenRegions[input->tree->alg->region+1] != 0 )
		numRetry += 1;

again:
	if ( input == 0 )
		goto _out;

	lel = input;
	if ( lel->tree->id < tables->keys[cs<<1] || lel->tree->id > tables->keys[(cs<<1)+1] )
		goto parseError;

	pos = tables->indicies[tables->offsets[cs] + (lel->tree->id - tables->keys[cs<<1])];
	if ( pos < 0 )
		goto parseError;

	induceReject = false;
	targState = tables->targs[pos];
	action = tables->actions + tables->actInds[pos];
	if ( lel->tree->alg->retry_lower )
		action += lel->tree->alg->retry_lower;

	if ( *action & act_sb ) {
		#ifdef COLM_LOG_PARSE
		cerr << "shifted: " << tables->rtd->lelInfo[lel->tree->id].name;
		#endif
		input = input->next;
		lel->tree->alg->state = cs;
		lel->next = stackTop;
		stackTop = lel;

		/* If shifting a termDup then change it to the nonterm. */
		if ( lel->tree->id < tables->rtd->firstNonTermId &&
				tables->rtd->lelInfo[lel->tree->id].termDupId > 0 )
		{
			lel->tree->id = tables->rtd->lelInfo[lel->tree->id].termDupId;
			lel->tree->alg->flags |= AF_GENERATED;
		}

		if ( action[1] == 0 )
			lel->tree->alg->retry_lower = 0;
		else {
			lel->tree->alg->retry_lower += 1;
			assert( lel->tree->alg->retry_upper == 0 );
			numRetry += 1; /* FIXME: Has the retry already been counted? */
			#ifdef COLM_LOG_PARSE
			cerr << " retry: " << stackTop;
			#endif
		}
		#ifdef COLM_LOG_PARSE
		cerr << endl;
		#endif
	}

	if ( tables->commitLen[pos] != 0 ) {
		long causeReduce = 0;
		if ( input != 0 ) { 
			Alg *alg = input->tree->alg;
			if ( alg->flags & AF_HAS_RCODE )
				causeReduce = alg->causeReduce;
		}
		commit_full( this, causeReduce );
	}

	if ( *action & act_rb ) {
		int objectLength, reduction = *action >> 2;
		Kid *last, *redLel, *child, *attrs;
		Alg *redAlg;

		if ( input != 0 )
			input->tree->alg->causeReduce += 1;

		redLel = prg->kidPool.allocate();
		redLel->tree = prg->treePool.allocate();
		redAlg = prg->algPool.allocate();

		redLel->tree->refs = 1;
		redLel->tree->id = tables->rtd->prodInfo[reduction].lhsId;

		redLel->next = 0;
		redAlg->causeReduce = 0;
		redAlg->retry_lower = 0;
		redAlg->retry_upper = lel->tree->alg->retry_lower;
		lel->tree->alg->retry_lower = 0;

		/* Allocate the attributes. */
		objectLength = tables->rtd->lelInfo[redLel->tree->id].objectLength;
		attrs = alloc_attrs( prg, objectLength );

		/* Build the list of children. */
		rhsLen = tables->rtd->prodInfo[reduction].length;
		child = last = 0;
		for ( int r = 0; r < rhsLen; r++ ) {
			child = stackTop;
			stackTop = stackTop->next;
			child->next = last;
			last = child;
		}

		redLel->tree->child = kid_list_concat( attrs, child );

		#ifdef COLM_LOG_PARSE
		cerr << "reduced: "
				<< tables->rtd->prodInfo[reduction].name
				<< " rhsLen: " << rhsLen;
		#endif
		if ( action[1] == 0 )
			redAlg->retry_upper = 0;
		else {
			redAlg->retry_upper += 1;
			assert( lel->tree->alg->retry_lower == 0 );
			numRetry += 1;
			#ifdef COLM_LOG_PARSE
			cerr << " retry: " << redLel;
			#endif
		}

		#ifdef COLM_LOG_PARSE
		cerr << endl;
		#endif

		/* When the production is of zero length we stay in the same state.
		 * Otherwise we use the state stored in the first child. */
		targState = rhsLen == 0 ? cs : child->tree->alg->state;

		assert( redLel->tree->refs == 1 );

		if ( prg->ctxDepParsing && tables->rtd->prodInfo[reduction].frameId >= 0 ) {
			/* Frame info for reduction. */
			FrameInfo *fi = &tables->rtd->frameInfo[tables->rtd->prodInfo[reduction].frameId];

			/* Execution environment for the reduction code. */
			Execution execution( prg, reverseCode, 
					this, fi->codeWV, redLel->tree, 0 );

			/* Execute it. */
			execution.execute( root );

			/* Transfer the lhs from the environment to redLel. It is uprefed
			 * while in the environment. */
			redLel->tree = execution.lhs;

			/* If the lhs was saved and it changed then we need to restore the
			 * original upon backtracking, otherwise downref since we took a
			 * copy above. */
			if ( execution.parsed != 0 && execution.parsed != redLel->tree ) {
				#ifdef COLM_LOG_PARSE
				cerr << "lhs tree was modified, adding a restore instruction" << endl;
				#endif

				reverseCode.append( IN_RESTORE_LHS );
				reverseCode.appendWord( (Word)execution.parsed );
				reverseCode.append( 5 );
			}
			else {
				/* No change in the the lhs. Just free the parsed copy we
				 * took. */
				tree_downref( prg, root, execution.parsed );
			}

			/* Pull out the reverse code, if any. */
			bool hasrcode = make_reverse_code( allReverseCode, reverseCode );
			if ( hasrcode )
				redAlg->flags |= AF_HAS_RCODE;

			/* Perhaps the execution environment is telling us we need to
			 * reject the reduction. */
			induceReject = execution.reject;
		}

		/* Save the algorithm data in the reduced tree. */
		redLel->tree->alg = redAlg;

		if ( induceReject ) {
			#ifdef COLM_LOG_PARSE
			cerr << "error induced during reduction of " <<
					tables->rtd->lelInfo[redLel->tree->id].name << endl;
			#endif
			redLel->tree->alg->state = cs;
			redLel->next = stackTop;
			stackTop = redLel;
			cs = targState;
			goto parseError;
		}
		else {
			redLel->next = input;
			input = redLel;
		}
	}


	cs = targState;
	goto again;

parseError:
	#ifdef COLM_LOG_PARSE
	cerr << "hit error, backtracking" << endl;
	#endif

	if ( numRetry == 0 )
		goto fail;

	while ( 1 ) {
		if ( input != 0 ) {
			assert( input->tree->alg->retry_upper == 0 );

			if ( input->tree->alg->retry_lower != 0 ) {
				#ifdef COLM_LOG_PARSE
				cerr << "found retry targ: " << input << endl;
				#endif
				numRetry -= 1;
				#ifdef COLM_LOG_PARSE
				cerr << "found retry: " << input << endl;
				#endif

				cs = input->tree->alg->state;
				goto again;
			}

			/* If there is no retry and there are no reductions caused by the
			 * current input token then we are finished with it. Send it back. */
			if ( input->tree->alg->causeReduce == 0 ) {
				int next = input->tree->alg->region + 1;

				fsmRun->queueBack( input );
				input = 0;
				if ( tables->tokenRegions[next] != 0 ) {
					#ifdef COLM_LOG_PARSE
					cerr << "found a new region" << endl;
					#endif
					numRetry -= 1;
					cs = stackTopTarget();
					nextRegionInd = next;
					return;
				}
			}
		}

		/* Now it is time to undo something. Pick an element from the top of
		 * the stack. */
		Kid *undoLel = stackTop;

		/* Check if we've arrived at the stack sentinal. This guard is
		 * here to allow us to initially set numRetry to one to cause the
		 * parser to backup all the way to the beginning when an error
		 * occurs. */
		if ( undoLel->next == 0 )
			break;

		/* Either we are dealing with a terminal that was
		 * shifted or a nonterminal that was reduced. */
		if ( stackTop->tree->id < tables->rtd->firstNonTermId || 
				(stackTop->tree->alg->flags & AF_GENERATED) )
		{
			#ifdef COLM_LOG_PARSE
			cerr << "backing up over effective terminal: " <<
					tables->rtd->lelInfo[stackTop->tree->id].name << endl;
			#endif

			/* Pop the item from the stack. */
			stackTop = stackTop->next;

			/* Undo the translation from termDup. */
			if ( undoLel->tree->alg->flags & AF_GENERATED ) {
				undoLel->tree->id = tables->rtd->lelInfo[undoLel->tree->id].termDupId;
				undoLel->tree->alg->flags &= ~AF_GENERATED;
			}

			/* Queue it as next input item. */
			undoLel->next = input;
			input = undoLel;
		}
		else {
			#ifdef COLM_LOG_PARSE
			cerr << "backing up over non-terminal: " <<
					tables->rtd->lelInfo[stackTop->tree->id].name << endl;
			#endif

			/* Take the alg out of undoLel. */
			Alg *alg = undoLel->tree->alg;
			assert( alg != 0 );
			undoLel->tree->alg = 0;

			/* Check for an execution environment. */
			if ( alg->flags & AF_HAS_RCODE ) {
				Execution execution( prg, reverseCode, this, 0, 0, 0 );

				/* Do the reverse exeuction. */
				execution.rexecute( root, allReverseCode );
				alg->flags &= ~AF_HAS_RCODE;

				if ( execution.lhs != 0 ) {
					/* Get the lhs, it may have been reverted. */
					tree_downref( prg, root, undoLel->tree );
					undoLel->tree = execution.lhs;
				}
			}

			/* Warm fuzzies ... */
			assert( undoLel == stackTop );

			/* Take the nonterm off the stack. */
			stackTop = stackTop->next;

			/* Extract the real children from the child list. */
			Kid *first = tree_extract_child( prg, undoLel->tree );

			/* Walk the child list and and push the items onto the parsing
			 * stack one at a time. */
			while ( first != 0 ) {
				/* Get the next item ahead of time. */
				Kid *next = first->next;

				/* Push onto the stack. */
				first->next = stackTop;
				stackTop = first;

				first = next;
			}

			/* If there is an input queued, this is one less reduction it has
			 * caused. */
			if ( input != 0 )
				input->tree->alg->causeReduce -= 1;

			if ( alg->retry_upper != 0 ) {
				/* There is always an input item here because reduce
				 * conflicts only happen on a lookahead character. */
				assert( input != undoLel );
				assert( input != 0 );
				assert( alg->retry_lower == 0 );
				assert( input->tree->alg->retry_upper == 0 );

				/* Transfer the retry from undoLel to input. */
				input->tree->alg->retry_lower = alg->retry_upper;
				input->tree->alg->retry_upper = 0;
				input->tree->alg->state = stackTopTarget();
			}

			/* Free the reduced item. */
			tree_downref( prg, root, undoLel->tree );
			prg->kidPool.free( undoLel );
			prg->algPool.free( alg );
		}
	}

fail:
	cs = -1;
	errCount += 1;
_out:
	nextRegionInd = tables->tokenRegionInds[cs];
}

ostream &PdaRun::parse_error( int tokId, Tree *tree )
{
	cerr << "error:" << fsmRun->line << ": at token ";
	if ( tokId < 128 )
		cerr << "\"" << tables->rtd->lelInfo[tokId].name << "\"";
	else 
		cerr << tables->rtd->lelInfo[tokId].name;
	if ( string_length( tree->tokdata ) > 0 ) {
		cerr << " with data \"";
		cerr.write( string_data( tree->tokdata ), 
				string_length( tree->tokdata ) );
		cerr << "\"";
	}
	cerr << ": ";
	
	return cerr;
}
