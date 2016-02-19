/*
 *  Copyright 2015 Adrian Thurston <thurston@complang.org>
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
#include "bytecode.h"
#include "tree.h"
#include "pool.h"

#include "internal.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

//#define true 1
//#define false 0

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
