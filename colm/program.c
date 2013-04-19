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

#include <alloca.h>
#include <sys/mman.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>

#define VM_STACK_SIZE (8192)

void clearGlobal( Program *prg, Tree **sp )
{
	/* Downref all the fields in the global object. */
	int g;
	for ( g = 0; g < prg->rtd->globalSize; g++ ) {
		//assert( colm_get_attr( global, g )->refs == 1 );
		treeDownref( prg, sp, colm_get_attr( prg->global, g ) );
	}

	/* Free the global object. */
	if ( prg->rtd->globalSize > 0 )
		freeAttrs( prg, prg->global->child );
	treeFree( prg, prg->global );
}

void allocGlobal( Program *prg )
{
	/* Alloc the global. */
	Tree *tree = treeAllocate( prg );
	tree->child = allocAttrs( prg, prg->rtd->globalSize );
	tree->refs = 1;
	prg->global = tree;
}

void vm_init( Program *prg )
{
	StackBlock *b = malloc( sizeof(StackBlock) );
	b->data = malloc( sizeof(Tree*) * VM_STACK_SIZE );
	b->len = VM_STACK_SIZE;
	b->offset = 0;
	b->next = 0;

	prg->stackBlock = b;

	prg->sb_beg = prg->stackBlock->data;
	prg->sb_end = prg->stackBlock->data + prg->stackBlock->len;

	prg->stackRoot = prg->sb_end;
}

Tree **colm_vm_root( Program *prg )
{
	return prg->stackRoot;
}

Tree **vm_bs_add( Program *prg, Tree **sp, int n )
{
	/* Close off the current block. */
	if ( prg->stackBlock != 0 ) {
		prg->stackBlock->offset = sp - prg->stackBlock->data;
		prg->sb_total += prg->stackBlock->len - prg->stackBlock->offset;
	}

	if ( prg->reserve != 0 && prg->reserve->len >= n) {
		StackBlock *b = prg->reserve;
		b->next = prg->stackBlock;
		b->offset = 0;

		prg->stackBlock = b;
		prg->reserve = 0;
	}
	else {
		StackBlock *b = malloc( sizeof(StackBlock) );
		int size = VM_STACK_SIZE;
		if ( n > size )
			size = n;
		b->next = prg->stackBlock;
		b->data = malloc( sizeof(Tree*) * size );
		b->len = size;
		b->offset = 0;

		prg->stackBlock = b;
	}

	prg->sb_beg = prg->stackBlock->data;
	prg->sb_end = prg->stackBlock->data + prg->stackBlock->len;

	return prg->sb_end;
}

Tree **vm_bs_pop( Program *prg, Tree **sp, int n )
{
	while ( 1 ) {
		Tree **end = prg->stackBlock->data + prg->stackBlock->len;
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
		StackBlock *b = prg->stackBlock;
		prg->stackBlock = prg->stackBlock->next;
		prg->reserve = b;

		/* Setup the bounds. Need to use offset here? Maybe we can grant more
		 * space in case any push that follows has fewer requirements. */
		prg->sb_beg = prg->stackBlock->data + prg->stackBlock->offset;
		prg->sb_end = prg->stackBlock->data + prg->stackBlock->len;

		/* Update the total stack usage. */
		prg->sb_total -= prg->stackBlock->len - prg->stackBlock->offset;

		n -= remaining;
		sp = prg->sb_beg;
	}
}

void vm_clear( Program *prg )
{
	while ( prg->stackBlock != 0 ) {
		StackBlock *b = prg->stackBlock;
		prg->stackBlock = prg->stackBlock->next;
		
		free( b->data );
		free( b );
	}

	if ( prg->reserve != 0 ) {
		free( prg->reserve->data );
		free( prg->reserve );
	}
}

Tree *colm_return_val( struct colm_program *prg )
{
	return prg->returnVal;
}

void colm_set_debug( Program *prg, long activeRealm )
{
	prg->activeRealm = activeRealm;
}

Program *colm_new_program( RuntimeData *rtd )
{
	Program *prg = malloc(sizeof(Program));
	memset( prg, 0, sizeof(Program) );

	assert( sizeof(Int)      <= sizeof(Tree) );
	assert( sizeof(Str)      <= sizeof(Tree) );
	assert( sizeof(Pointer)  <= sizeof(Tree) );
	assert( sizeof(Map)      <= sizeof(MapEl) );
	assert( sizeof(List)     <= sizeof(MapEl) );
	assert( sizeof(Stream)   <= sizeof(MapEl) );
	assert( sizeof(Parser)   <= sizeof(MapEl) );

	prg->rtd = rtd;
	prg->ctxDepParsing = 1;

	initPoolAlloc( &prg->kidPool, sizeof(Kid) );
	initPoolAlloc( &prg->treePool, sizeof(Tree) );
	initPoolAlloc( &prg->parseTreePool, sizeof(ParseTree) );
	initPoolAlloc( &prg->listElPool, sizeof(ListEl) );
	initPoolAlloc( &prg->mapElPool, sizeof(MapEl) );
	initPoolAlloc( &prg->headPool, sizeof(Head) );
	initPoolAlloc( &prg->locationPool, sizeof(Location) );

	Int *trueInt = (Int*) treeAllocate( prg );
	trueInt->id = LEL_ID_BOOL;
	trueInt->refs = 1;
	trueInt->value = 1;

	Int *falseInt = (Int*) treeAllocate( prg );
	falseInt->id = LEL_ID_BOOL;
	falseInt->refs = 1;
	falseInt->value = 0;

	prg->trueVal = (Tree*)trueInt;
	prg->falseVal = (Tree*)falseInt;

	/* Allocate the global variable. */
	allocGlobal( prg );

	/* Allocate the VM stack. */
	vm_init( prg );
	return prg;
}

void colm_run_program( Program *prg, int argc, const char **argv )
{
	if ( prg->rtd->rootCodeLen == 0 )
		return;

	/* Make the arguments available to the program. */
	prg->argc = argc;
	prg->argv = argv;

	Execution execution;
	memset( &execution, 0, sizeof(execution) );
	execution.frameId = prg->rtd->rootFrameId;

	mainExecution( prg, &execution, prg->rtd->rootCode );

	/* Clear the arg and stack. */
	prg->argc = 0;
	prg->argv = 0;
}

Tree *colm_run_func( struct colm_program *prg, int frameId, const char **params, int paramCount )
{
	/* Make the arguments available to the program. */
	prg->argc = 0;
	prg->argv = 0;

	Execution execution;
	memset( &execution, 0, sizeof(execution) );

	Tree **sp = prg->stackRoot;

	FrameInfo *fi = &prg->rtd->frameInfo[frameId];
	Code *code = fi->codeWC;
	long stretch = fi->argSize + 4 + fi->frameSize;
	vm_contiguous( stretch );

	int p;
	for ( p = 0; p < paramCount; p++ ) {
		if ( params[p] == 0 ) {
			vm_push( 0 );
		}
		else {
			Head *head = stringAllocPointer( prg, params[p], strlen(params[p]) );
			Tree *tree = constructString( prg, head );
			treeUpref( tree );
			vm_push( tree );
		}
	}

	/* Set up the stack as if we have called. We allow a return value. */
	vm_push( 0 ); 
	vm_push( 0 );
	vm_push( 0 );
	vm_push( 0 );

	execution.framePtr = vm_ptop();
	execution.frameId = frameId;

	/* Execution loop. */
	sp = executeCode( prg, &execution, sp, code );
	
	treeDownref( prg, sp, prg->returnVal );
	prg->returnVal = vm_pop();

	assert( sp == prg->stackRoot );

	return prg->returnVal;
};

int colm_delete_program( Program *prg )
{
	Tree **sp = prg->stackRoot;
	int exitStatus = prg->exitStatus;

	treeDownref( prg, sp, prg->returnVal );
	clearGlobal( prg, sp );

	/* Clear the heap. */
	Kid *a = prg->heap;
	while ( a != 0 ) {
		Kid *next = a->next;
		treeDownref( prg, sp, a->tree );
		kidFree( prg, a );
		a = next;
	}

	treeDownref( prg, sp, prg->trueVal );
	treeDownref( prg, sp, prg->falseVal );

	treeDownref( prg, sp, (Tree*)prg->stdinVal );
	treeDownref( prg, sp, (Tree*)prg->stdoutVal );
	treeDownref( prg, sp, (Tree*)prg->stderrVal );

	treeDownref( prg, sp, prg->error );

#if DEBUG
	long kidLost = kidNumLost( prg );
	long treeLost = treeNumLost( prg );
	long parseTreeLost = parseTreeNumLost( prg );
	long listLost = listElNumLost( prg );
	long mapLost = mapElNumLost( prg );
	long headLost = headNumLost( prg );
	long locationLost = locationNumLost( prg );

	if ( kidLost )
		message( "warning: lost kids: %ld\n", kidLost );

	if ( treeLost )
		message( "warning: lost trees: %ld\n", treeLost );

	if ( parseTreeLost )
		message( "warning: lost parse trees: %ld\n", parseTreeLost );

	if ( listLost )
		message( "warning: lost listEls: %ld\n", listLost );

	if ( mapLost )
		message( "warning: lost mapEls: %ld\n", mapLost );

	if ( headLost )
		message( "warning: lost heads: %ld\n", headLost );

	if ( locationLost )
		message( "warning: lost locations: %ld\n", locationLost );
#endif

	kidClear( prg );
	treeClear( prg );
	headClear( prg );
	parseTreeClear( prg );
	listElClear( prg );
	mapElClear( prg );
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
