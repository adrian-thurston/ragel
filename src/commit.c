/*
 * Copyright 2015 Adrian Thurston <thurston@colm.net>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include "config.h"
#include "debug.h"
#include "pdarun.h"
#include "bytecode.h"
#include "tree.h"
#include "pool.h"
#include "internal.h"

void commit_clear_kid_list( program_t *prg, tree_t **sp, kid_t *kid )
{
	kid_t *next;
	while ( kid ) {
		colm_tree_downref( prg, sp, kid->tree );
		next = kid->next;
		kid_free( prg, kid );
		kid = next;
	}
}

void commit_clear_parse_tree( program_t *prg, tree_t **sp,
		struct pda_run *pda_run, parse_tree_t *pt )
{
	tree_t **top = vm_ptop();

	if ( pt == 0 )
		return;

free_tree:
	if ( pt->next != 0 ) {
		vm_push_ptree( pt->next );
	}

	if ( pt->left_ignore != 0 ) {
		vm_push_ptree( pt->left_ignore );
	}

	if ( pt->child != 0 ) {
		vm_push_ptree( pt->child );
	}

	if ( pt->right_ignore != 0 ) {
		vm_push_ptree( pt->right_ignore );
	}

	/* Only the root level of the stack has tree 
	 * shadows and we are below that. */
	assert( pt->shadow == 0 );
	parse_tree_free( pda_run, pt );

	/* Any trees to downref? */
	if ( sp != top ) {
		pt = vm_pop_ptree();
		goto free_tree;
	}
}

static int been_committed( parse_tree_t *parse_tree )
{
	return parse_tree->flags & PF_COMMITTED;
}

void commit_reduce( program_t *prg, tree_t **root, struct pda_run *pda_run )
{
	tree_t **sp = root;
	parse_tree_t *pt = pda_run->stack_top;

	/* The top level of the stack is linked right to left. This is the
	 * traversal order we need for committing. */
	while ( pt != 0 && !been_committed( pt ) ) {
		vm_push_ptree( pt );
		pt = pt->next;
	}

	while ( sp != root ) {
		pt = vm_pop_ptree();

		prg->rtd->commit_reduce_forward( prg, sp, pda_run, pt );
		pt->child = 0;

		pt->flags |= PF_COMMITTED;
		pt = pt->next;
	}
}
