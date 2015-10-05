/*
 *  Copyright 2010-2012 Adrian Thurston <thurston@complang.org>
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

#ifndef _POOL_H
#define _POOL_H

/* Allocation, number of items. */
#define FRESH_BLOCK 8128                    

#include <colm/pdarun.h>
#include <colm/map.h>
#include <colm/tree.h>

#ifdef __cplusplus
extern "C" {
#endif

void init_pool_alloc( struct pool_alloc *pool_alloc, int sizeofT );

kid_t *kid_allocate( program_t *prg );
void kid_free( program_t *prg, kid_t *el );
void kid_clear( program_t *prg );
long kid_num_lost( program_t *prg );

tree_t *tree_allocate( program_t *prg );
void tree_free( program_t *prg, tree_t *el );
void tree_clear( program_t *prg );
long tree_num_lost( program_t *prg );

/* Parse tree allocators go into pda_run structs. */
parse_tree_t *parse_tree_allocate( struct pda_run *pda_run );
void parse_tree_free( struct pda_run *pda_run, parse_tree_t *el );
void parse_tree_clear( struct pool_alloc *pool_alloc );
long parse_tree_num_lost( struct pool_alloc *pool_alloc );

head_t *head_allocate( program_t *prg );
void head_free( program_t *prg, head_t *el );
void head_clear( program_t *prg );
long head_num_lost( program_t *prg );

location_t *location_allocate( program_t *prg );
void location_free( program_t *prg, location_t *el );
void location_clear( program_t *prg );
long location_num_lost( program_t *prg );

void pool_alloc_clear( struct pool_alloc *pool_alloc );
long pool_alloc_num_lost( struct pool_alloc *pool_alloc );

#ifdef __cplusplus
}
#endif

#endif
