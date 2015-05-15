/*
 *  Copyright 2006-2012 Adrian Thurston <thurston@complang.org>
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

#include <colm/pdarun.h>
#include <colm/tree.h>
#include <colm/bytecode.h>
#include <colm/pool.h>
#include <colm/debug.h>
#include <colm/config.h>
#include <colm/struct.h>

#include <alloca.h>
#include <sys/mman.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>

#define VM_STACK_SIZE (8192)

static void colm_alloc_global( program_t *prg )
{
	/* Alloc the global. */
	prg->global = colm_struct_new( prg, prg->rtd->globalId ) ;
}

void vm_init( program_t *prg )
{
	struct stack_block *b = malloc( sizeof(struct stack_block) );
	b->data = malloc( sizeof(tree_t*) * VM_STACK_SIZE );
	b->len = VM_STACK_SIZE;
	b->offset = 0;
	b->next = 0;

	prg->stackBlock = b;

	prg->sb_beg = prg->stackBlock->data;
	prg->sb_end = prg->stackBlock->data + prg->stackBlock->len;

	prg->stackRoot = prg->sb_end;
}

tree_t **colm_vm_root( program_t *prg )
{
	return prg->stackRoot;
}

tree_t **vm_bs_add( program_t *prg, tree_t **sp, int n )
{
	/* Close off the current block. */
	if ( prg->stackBlock != 0 ) {
		prg->stackBlock->offset = sp - prg->stackBlock->data;
		prg->sb_total += prg->stackBlock->len - prg->stackBlock->offset;
	}

	if ( prg->reserve != 0 && prg->reserve->len >= n) {
		struct stack_block *b = prg->reserve;
		b->next = prg->stackBlock;
		b->offset = 0;

		prg->stackBlock = b;
		prg->reserve = 0;
	}
	else {
		struct stack_block *b = malloc( sizeof(struct stack_block) );
		int size = VM_STACK_SIZE;
		if ( n > size )
			size = n;
		b->next = prg->stackBlock;
		b->data = malloc( sizeof(tree_t*) * size );
		b->len = size;
		b->offset = 0;

		prg->stackBlock = b;
	}

	prg->sb_beg = prg->stackBlock->data;
	prg->sb_end = prg->stackBlock->data + prg->stackBlock->len;

	return prg->sb_end;
}

tree_t **vm_bs_pop( program_t *prg, tree_t **sp, int n )
{
	while ( 1 ) {
		tree_t **end = prg->stackBlock->data + prg->stackBlock->len;
		int remaining = end - sp;

		/* Don't have to free this block. Remaining values to pop leave us
		 * inside it. */
		if ( n < remaining ) {
			sp += n;
			return sp;
		}

		if ( prg->stackBlock->next == 0 ) {
			/* Don't delete the sentinal stack block. Returns the end as in the
			 * creation of the first stack block. */
			return prg->sb_end;
		}
	
		/* Clear any previous reserve. We are going to save this block as the
		 * reserve. */
		if ( prg->reserve != 0 ) {
			free( prg->reserve->data );
			free( prg->reserve );
		}

		/* Pop the stack block. */
		struct stack_block *b = prg->stackBlock;
		prg->stackBlock = prg->stackBlock->next;
		prg->reserve = b;

		/* Setup the bounds. Note that we restore the full block, which is
		 * necessary to honour any CONTIGUOUS statements that counted on it
		 * before a subsequent CONTIGUOUS triggered a new block. */
		prg->sb_beg = prg->stackBlock->data; 
		prg->sb_end = prg->stackBlock->data + prg->stackBlock->len;

		/* Update the total stack usage. */
		prg->sb_total -= prg->stackBlock->len - prg->stackBlock->offset;

		n -= remaining;
		sp = prg->stackBlock->data + prg->stackBlock->offset;
	}
}

void vm_clear( program_t *prg )
{
	while ( prg->stackBlock != 0 ) {
		struct stack_block *b = prg->stackBlock;
		prg->stackBlock = prg->stackBlock->next;
		
		free( b->data );
		free( b );
	}

	if ( prg->reserve != 0 ) {
		free( prg->reserve->data );
		free( prg->reserve );
	}
}

tree_t *colm_return_val( struct colm_program *prg )
{
	return prg->returnVal;
}

void colm_set_debug( program_t *prg, long activeRealm )
{
	prg->activeRealm = activeRealm;
}

program_t *colm_new_program( struct colm_sections *rtd )
{
	program_t *prg = malloc(sizeof(program_t));
	memset( prg, 0, sizeof(program_t) );

	assert( sizeof(str_t)      <= sizeof(tree_t) );
	assert( sizeof(pointer_t)  <= sizeof(tree_t) );

	prg->rtd = rtd;
	prg->ctxDepParsing = 1;

	initPoolAlloc( &prg->kidPool, sizeof(kid_t) );
	initPoolAlloc( &prg->treePool, sizeof(tree_t) );
	initPoolAlloc( &prg->parseTreePool, sizeof(parse_tree_t) );
	initPoolAlloc( &prg->headPool, sizeof(head_t) );
	initPoolAlloc( &prg->locationPool, sizeof(location_t) );

	prg->trueVal = (tree_t*) 1;
	prg->falseVal = (tree_t*) 0;

	/* Allocate the global variable. */
	colm_alloc_global( prg );

	/* Allocate the VM stack. */
	vm_init( prg );
	return prg;
}

void colm_run_program( program_t *prg, int argc, const char **argv )
{
	if ( prg->rtd->rootCodeLen == 0 )
		return;

	/* Make the arguments available to the program. */
	prg->argc = argc;
	prg->argv = argv;

	Execution execution;
	memset( &execution, 0, sizeof(execution) );
	execution.frameId = prg->rtd->rootFrameId;

	colm_execute( prg, &execution, prg->rtd->rootCode );

	/* Clear the arg and stack. */
	prg->argc = 0;
	prg->argv = 0;
}

static void colm_clear_heap( program_t *prg, tree_t **sp )
{
	struct colm_struct *hi = prg->heap.head;
	while ( hi != 0 ) {
		struct colm_struct *next = hi->next;
		colm_struct_delete( prg, sp, hi );
		hi = next;
	}
}

int colm_delete_program( program_t *prg )
{
	tree_t **sp = prg->stackRoot;
	int exitStatus = prg->exitStatus;

	treeDownref( prg, sp, prg->returnVal );
	colm_clear_heap( prg, sp );

	treeDownref( prg, sp, prg->error );

#if DEBUG
	long kidLost = kidNumLost( prg );
	long treeLost = treeNumLost( prg );
	long parseTreeLost = parseTreeNumLost( prg );
	long headLost = headNumLost( prg );
	long locationLost = locationNumLost( prg );

	if ( kidLost )
		message( "warning: lost kids: %ld\n", kidLost );

	if ( treeLost )
		message( "warning: lost trees: %ld\n", treeLost );

	if ( parseTreeLost )
		message( "warning: lost parse trees: %ld\n", parseTreeLost );

	if ( headLost )
		message( "warning: lost heads: %ld\n", headLost );

	if ( locationLost )
		message( "warning: lost locations: %ld\n", locationLost );
#endif

	kidClear( prg );
	treeClear( prg );
	headClear( prg );
	parseTreeClear( prg );
	locationClear( prg );

	RunBuf *rb = prg->allocRunBuf;
	while ( rb != 0 ) {
		RunBuf *next = rb->next;
		free( rb );
		rb = next;
	}

	vm_clear( prg );

	free( prg );

	return exitStatus;
}
