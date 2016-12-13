/*
 * Copyright 2016 Adrian Thurston <thurston@colm.net>
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

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include <colm/program.h>
#include <colm/struct.h>

#include "internal.h"
#include "bytecode.h"

struct colm_tree *colm_get_global( program_t *prg, long pos )
{
	return colm_struct_get_field( prg->global, tree_t*, pos );
}

void colm_struct_add( program_t *prg, struct colm_struct *item )
{
	if ( prg->heap.head == 0 ) {
		prg->heap.head = prg->heap.tail = item;
		item->prev = item->next = 0;
	}
	else {
		item->prev = prg->heap.tail;
		item->next = 0;
		prg->heap.tail->next = item;
		prg->heap.tail = item;
	}
}

struct colm_struct *colm_struct_new_size( program_t *prg, int size )
{
	size_t memsize = sizeof(struct colm_struct) + ( sizeof(tree_t*) * size );
	struct colm_struct *item = (struct colm_struct*) malloc( memsize );
	memset( item, 0, memsize );

	colm_struct_add( prg, item );
	return item;
}

struct colm_struct *colm_struct_new( program_t *prg, int id )
{
	struct colm_struct *s = colm_struct_new_size( prg, prg->rtd->sel_info[id].size );
	s->id = id;
	return s;
}

void colm_struct_delete( program_t *prg, tree_t **sp, struct colm_struct *el )
{
	if ( el->id == STRUCT_INBUILT_ID ) {
		colm_destructor_t destructor = ((struct colm_inbuilt*)el)->destructor;
		if ( destructor != 0 )
			(*destructor)( prg, sp, el );
	}

	if ( el->id >= 0 ) { 
		short *t = prg->rtd->sel_info[el->id].trees;
		int i, len = prg->rtd->sel_info[el->id].trees_len;
		for ( i = 0; i < len; i++ ) {
			tree_t *tree = colm_struct_get_field( el, tree_t*, t[i] );
			colm_tree_downref( prg, sp, tree );
		}
	}
	free( el );
}

void colm_parser_destroy( program_t *prg, tree_t **sp, struct colm_struct *s )
{
	struct colm_parser *parser = (struct colm_parser*) s;

	/* Free the PDA run. */
	colm_pda_clear( prg, sp, parser->pda_run );
	free( parser->pda_run );

	/* Free the result. */
	colm_tree_downref( prg, sp, parser->result );
}

parser_t *colm_parser_new( program_t *prg, struct generic_info *gi, int reducer )
{
	struct pda_run *pda_run = malloc( sizeof(struct pda_run) );

	/* Start off the parsing process. */
	colm_pda_init( prg, pda_run, prg->rtd->pda_tables, 
			gi->parser_id, 0, 0, 0, reducer );
	
	size_t memsize = sizeof(struct colm_parser);
	struct colm_parser *parser = (struct colm_parser*) malloc( memsize );
	memset( parser, 0, memsize );
	colm_struct_add( prg, (struct colm_struct*) parser );

	parser->id = STRUCT_INBUILT_ID;
	parser->destructor = &colm_parser_destroy;
	parser->pda_run = pda_run;

	return parser;
}

void colm_map_destroy( program_t *prg, tree_t **sp, struct colm_struct *s )
{
	struct colm_map *map = (struct colm_map*) s;

	map_el_t *el = map->head;
	while ( el != 0 ) {
		map_el_t *next = el->next;
		colm_tree_downref( prg, sp, el->key );
		//mapElFree( prg, el );
		el = next;
	}
}

map_t *colm_map_new( struct colm_program *prg )
{
	size_t memsize = sizeof(struct colm_map);
	struct colm_map *map = (struct colm_map*) malloc( memsize );
	memset( map, 0, memsize );
	colm_struct_add( prg, (struct colm_struct *)map );
	map->id = STRUCT_INBUILT_ID;
	return map;
}

struct_t *colm_construct_generic( program_t *prg, long generic_id )
{
	struct generic_info *generic_info = &prg->rtd->generic_info[generic_id];
	struct_t *new_generic = 0;
	switch ( generic_info->type ) {
		case GEN_MAP: {
			map_t *map = colm_map_new( prg );
			map->generic_info = generic_info;
			new_generic = (struct_t*) map;
			break;
		}
		case GEN_LIST: {
			list_t *list = colm_list_new( prg );
			list->generic_info = generic_info;
			new_generic = (struct_t*) list;
			break;
		}
		case GEN_PARSER: {
			parser_t *parser = colm_parser_new( prg, generic_info, 0 );
			parser->input = colm_stream_new( prg );
			new_generic = (struct_t*) parser;
			break;
		}
	}

	return new_generic;
}

struct_t *colm_construct_reducer( program_t *prg, long generic_id, int reducer_id )
{
	struct generic_info *generic_info = &prg->rtd->generic_info[generic_id];
	struct_t *new_generic = 0;

	parser_t *parser = colm_parser_new( prg, generic_info, reducer_id );
	parser->input = colm_stream_new( prg );
	new_generic = (struct_t*) parser;

	return new_generic;
}
