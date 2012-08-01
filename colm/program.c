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
#include <colm/fsmrun.h>
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

void colmInit( long debugRealm )
{
	/* Always on because because logging is controlled with ifdefs in\n" the
	 * runtime lib. */
	colm_log_bytecode = 1;
	colm_log_parse = 1;
	colm_log_match = 1;
	colm_log_compile = 1;
	colm_log_conds = 1;
	colmActiveRealm = debugRealm;
	initInputFuncs();

	assert( sizeof(Int)      <= sizeof(Tree) );
	assert( sizeof(Str)      <= sizeof(Tree) );
	assert( sizeof(Pointer)  <= sizeof(Tree) );
	assert( sizeof(Map)      <= sizeof(MapEl) );
	assert( sizeof(List)     <= sizeof(MapEl) );
	assert( sizeof(Stream)   <= sizeof(MapEl) );
	assert( sizeof(Parser)   <= sizeof(MapEl) );
}

void clearGlobal( Program *prg, Tree **sp )
{
	/* Downref all the fields in the global object. */
	int g;
	for ( g = 0; g < prg->rtd->globalSize; g++ ) {
		//assert( getAttr( global, g )->refs == 1 );
		treeDownref( prg, sp, getAttr( prg->global, g ) );
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

Tree **stackAlloc()
{
	return (Tree**)mmap( 0, sizeof(Tree*)*VM_STACK_SIZE,
		PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0 );
}

void stackFree( Tree **stack )
{
	munmap( stack, sizeof(Tree*)*VM_STACK_SIZE );
}

Tree **vm_root( struct ColmProgram *prg )
{
	return prg->vm_root;
}

Tree *returnVal( struct ColmProgram *prg )
{
	return prg->returnVal;
}


Program *colmNewProgram( RuntimeData *rtd )
{
	Program *prg = malloc(sizeof(Program));
	memset( prg, 0, sizeof(Program) );
	prg->argc = 0;
	prg->argv = 0;
	prg->rtd = rtd;
	prg->ctxDepParsing = 1;
	prg->global = 0;
	prg->heap = 0;
	prg->stdinVal = 0;
	prg->stdoutVal = 0;
	prg->stderrVal = 0;
	prg->induceExit = 0;
	prg->exitStatus = 0;

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

	prg->allocRunBuf = 0;
	prg->returnVal = 0;
	prg->lastParseError = 0;

	/* Allocate the global variable. */
	allocGlobal( prg );

	/*
	 * Allocate the VM stack.
	 */
	prg->vm_stack = stackAlloc();
	prg->vm_root = &prg->vm_stack[VM_STACK_SIZE];

	return prg;
}

void colmRunProgram( Program *prg, int argc, const char **argv )
{
	/* Make the arguments available to the program. */
	prg->argc = argc;
	prg->argv = argv;

	/*
	 * Execute
	 */
	if ( prg->rtd->rootCodeLen > 0 ) {
		Execution execution;

		initExecution( &execution, 0, 0, 0, 0, prg->rtd->rootFrameId );
		mainExecution( prg, &execution, prg->rtd->rootCode );
	}

	/* Clear the arg and stack. */
	prg->argc = 0;
	prg->argv = 0;
}


int colmDeleteProgram( Program *prg )
{
	Tree **sp = prg->vm_root;
	int exitStatus = prg->exitStatus;

	#ifdef COLM_LOG_BYTECODE
	if ( colm_log_bytecode ) {
		cerr << "clearing the prg" << endl;
	}
	#endif

	treeDownref( prg, sp, prg->returnVal );
	treeDownref( prg, sp, prg->lastParseError );
	clearGlobal( prg, sp );

	/* Clear the heap. */
	Kid *a = prg->heap;
	while ( a != 0 ) {
		Kid *next = a->next;
		treeDownref( prg, sp, a->tree );
		kidFree( prg, a );
		a = next;
	}

	//assert( trueVal->refs == 1 );
	//assert( falseVal->refs == 1 );
	treeDownref( prg, sp, prg->trueVal );
	treeDownref( prg, sp, prg->falseVal );

	treeDownref( prg, sp, (Tree*)prg->stdinVal );
	treeDownref( prg, sp, (Tree*)prg->stdoutVal );
	treeDownref( prg, sp, (Tree*)prg->stderrVal );

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

	stackFree( prg->vm_stack );
	free( prg );

	return exitStatus;
}
