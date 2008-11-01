/*
 *  Copyright 2007 Adrian Thurston <thurston@cs.queensu.ca>
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
		tree_downref( prg, kid->tree );
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

#define push(i) (*(--sp) = (i))
#define pop() (*sp++)

bool beenCommitted( Kid *kid )
{
	return kid->tree->alg->flags & AF_COMMITTED;
}

bool beenFreed( Kid *kid )
{
	return kid->tree->alg->flags & AF_REV_FREED;
}

/* The top level of the stack is linked right-to-left. Trees underneath are
 * left to right natural order. */

void PdaRun::commitKid( Tree **root, Kid *lel )
{
	Alg *alg = 0;
	Tree *tree = 0;
	Tree **sp = root;

head:
	/* Load up the parsed tree. */
	tree = lel->tree;
	alg = tree->alg;
	if ( alg->parsed != 0 )
		tree = alg->parsed;

	/* Recurse only on non-generated trees. */
	if ( !(alg->flags & AF_GENERATED) && tree->child != 0 ) {
		push( (Tree*)lel );
		lel = tree_child( prg, tree );

		while ( lel != 0 ) {
			if ( !beenCommitted( lel ) )
				goto head;

			upwards:
			lel = lel->next;
		}

		lel = (Kid*)pop();
	}

	/* Commit */
	#ifdef COLM_LOG_PARSE
	cerr << "commit visiting: " << 
			prg->rtd->lelInfo[lel->tree->id].name << endl;
	#endif

	alg = lel->tree->alg;

	/* Reset retries. */
	if ( alg->retry_lower > 0 ) {
		numRetry -= 1;
		alg->retry_lower = 0;
	}
	if ( alg->retry_upper > 0 ) {
		numRetry -= 1;
		alg->retry_upper = 0;
	}
	alg->flags |= AF_COMMITTED;

	if ( sp != root )
		goto upwards;

	numRetry = 0;
	assert( sp == root );
}


/* The top level of the stack is linked right-to-left. Trees underneath are
 * left to right natural order. */
void parsed_downref_kid( Tree **root, Program *prg, Kid *lel )
{
	Alg *alg = 0;
	Tree *tree = 0;
	Tree **sp = root;

head:
	/* Load up the right tree. */
	tree = lel->tree;
	alg = tree->alg;
	if ( alg->parsed != 0 )
		tree = alg->parsed;

	/* Recurse. */
	if ( !(alg->flags & AF_GENERATED) && tree->child != 0 ) {
		push( (Tree*)lel );
		lel = tree_child( prg, tree );

		while ( lel != 0 ) {
			if ( !beenFreed( lel ) )
				goto head;

			upwards:
			lel = lel->next;
		}

		lel = (Kid*)pop();
	}

	/* Commit */
	#ifdef COLM_LOG_PARSE
	cerr << "rev free visiting: " << 
			prg->rtd->lelInfo[lel->tree->id].name << endl;
	#endif

	alg = lel->tree->alg;

	alg->flags |= AF_REV_FREED;

	tree_downref( prg, alg->parsed );
	alg->parsed = 0;

	if ( sp != root )
		goto upwards;

	assert( sp == root );
}

void parsed_downref( Tree **root, Program *prg, Tree *tree )
{
	#ifdef COLM_LOG_PARSE
	cerr << "running parsed_downref on tree" << endl;
	#endif

	Kid kid;
	kid.next = 0;
	kid.tree = tree;
	parsed_downref_kid( root, prg, &kid );
}

void PdaRun::commit()
{
	#ifdef COLM_LOG_PARSE
	cerr << "running full commit" << endl;
	#endif
	
	Tree **sp = root;
	Kid *kid = stackTop;
	long topLevel = 0;
	while ( kid != 0 && !beenCommitted( kid ) ) {
		push( (Tree*)kid );
		kid = kid->next;
		topLevel += 1;
	}

	while ( topLevel > 0 ) {
		kid = (Kid*)pop();
		commitKid( sp, kid );
		parsed_downref_kid( sp, prg, kid );
		topLevel -= 1;
	}

	/* Affter running the commit the the stack should be where it 
	 * was when we started. */
	assert( sp == root );

	/* Now clear all the rcode. */
	rcode_downref_all( root, prg, &allReverseCode );
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
		cerr << "shifted: " << tables->gbl->lelInfo[lel->tree->id].name;
		#endif
		input = input->next;
		lel->tree->alg->state = cs;
		lel->next = stackTop;
		stackTop = lel;

		/* If shifting a termDup then change it to the nonterm. */
		if ( lel->tree->id < tables->gbl->firstNonTermId &&
				tables->gbl->lelInfo[lel->tree->id].termDupId > 0 )
		{
			lel->tree->id = tables->gbl->lelInfo[lel->tree->id].termDupId;
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

	if ( tables->commitLen[pos] != 0 )
		commit();

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
		redLel->tree->id = tables->gbl->prodInfo[reduction].lhsId;

		redLel->next = 0;
		redAlg->causeReduce = 0;
		redAlg->retry_lower = 0;
		redAlg->retry_upper = lel->tree->alg->retry_lower;
		lel->tree->alg->retry_lower = 0;

		/* Allocate the attributes. */
		objectLength = tables->gbl->lelInfo[redLel->tree->id].objectLength;
		attrs = alloc_attrs( prg, objectLength );

		/* Build the list of children. */
		rhsLen = tables->gbl->prodInfo[reduction].length;
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
				<< tables->gbl->prodInfo[reduction].name
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

		if ( prg->ctxDepParsing && tables->gbl->prodInfo[reduction].frameId >= 0 ) {
			/* Frame info for reduction. */
			FrameInfo *fi = &tables->gbl->frameInfo[tables->gbl->prodInfo[reduction].frameId];

			/* Execution environment for the reduction code. */
			Execution execution( prg, reverseCode, 
					this, fi->code, redLel->tree, 0 );

			/* Take a copy of the lhs and store it in alg. May need it during
			 * reverse parsing. */
			redAlg->parsed = redLel->tree;
			tree_upref( redAlg->parsed );

			/* Execute it. */
			execution.execute( root );

			/* Pull out the reverse code, if any. */
			bool hasrcode = makeReverseCode( allReverseCode, reverseCode );
			if ( hasrcode )
				redAlg->flags |= AF_HAS_RCODE;

			/* Transfer the lhs from the environment to redLel. It is uprefed
			 * while in the environment. */
			redLel->tree = execution.lhs;

			/* Perhaps the execution environment is telling us we need to
			 * reject the reduction. */
			induceReject = execution.reject;
		}

		/* Save the algorithm data in the reduced tree. */
		redLel->tree->alg = redAlg;

		if ( induceReject ) {
			#ifdef COLM_LOG_PARSE
			cerr << "error induced during reduction of " <<
					tables->gbl->lelInfo[redLel->tree->id].name << endl;
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
		if ( stackTop->tree->id < tables->gbl->firstNonTermId || 
				(stackTop->tree->alg->flags & AF_GENERATED) )
		{
			#ifdef COLM_LOG_PARSE
			cerr << "backing up over effective terminal: " <<
					tables->gbl->lelInfo[stackTop->tree->id].name << endl;
			#endif

			/* Pop the item from the stack. */
			stackTop = stackTop->next;

			/* Undo the translation from termDup. */
			if ( undoLel->tree->alg->flags & AF_GENERATED ) {
				undoLel->tree->id = tables->gbl->lelInfo[undoLel->tree->id].termDupId;
				undoLel->tree->alg->flags &= ~AF_GENERATED;
			}

			/* Queue it as next input item. */
			undoLel->next = input;
			input = undoLel;
		}
		else {
			#ifdef COLM_LOG_PARSE
			cerr << "backing up over non-terminal: " <<
					tables->gbl->lelInfo[stackTop->tree->id].name << endl;
			#endif

			/* Take the alg out of undoLel. */
			Alg *alg = undoLel->tree->alg;
			assert( alg != 0 );
			undoLel->tree->alg = 0;

			/* Check for an execution environment. */
			if ( alg->flags & AF_HAS_RCODE ) {
				Execution execution( prg, reverseCode, this, 0, 0, 0 );

				/* Do the reverse exeuction. */
				execution.rexecute( root, 0, allReverseCode );
				alg->flags &= ~AF_HAS_RCODE;
			}

			if ( alg->parsed != 0 ) {
				/* Get the lhs, it may have been reverted. */
				tree_downref( prg, undoLel->tree );
				undoLel->tree = alg->parsed;
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
			tree_downref( prg, undoLel->tree );
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
	/* Maintain the error count. */
	// FIXME: what to put here?
	// gblErrorCount += 1;

	//cerr << token.loc.fileName << ":" << token.loc.line << ":" << token.loc.col << ": ";
	cerr << "error: at token ";
	if ( tokId < 128 )
		cerr << "\"" << tables->gbl->lelInfo[tokId].name << "\"";
	else 
		cerr << tables->gbl->lelInfo[tokId].name;
	if ( string_length( tree->tokdata ) > 0 ) {
		cerr << " with data \"";
		cerr.write( string_data( tree->tokdata ), 
				string_length( tree->tokdata ) );
		cerr << "\"";
	}
	cerr << ": ";
	
	return cerr;
}
