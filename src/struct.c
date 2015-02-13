#include <colm/program.h>
#include <colm/struct.h>

#include "internal.h"
#include "bytecode.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>

struct colm_tree *colm_get_global( Program *prg, long pos )
{
	return colm_struct_get_field( prg->global, Tree*, pos );
}

void colm_struct_add( Program *prg, struct colm_struct *item )
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

struct colm_struct *colm_struct_new_size( Program *prg, int size )
{
	size_t memsize = sizeof(struct colm_struct) + ( sizeof(Tree*) * size );
	struct colm_struct *item = (struct colm_struct*) malloc( memsize );
	memset( item, 0, memsize );

	colm_struct_add( prg, item );
	return item;
}

struct colm_struct *colm_struct_new( Program *prg, int id )
{
	struct colm_struct *s = colm_struct_new_size( prg, prg->rtd->selInfo[id].size );
	s->id = id;
	return s;
}

void colm_struct_delete( Program *prg, Tree **sp, struct colm_struct *el )
{
	if ( el->id == STRUCT_INBUILT_ID ) {
		colm_destructor_t destructor = ((struct colm_inbuilt*)el)->destructor;
		if ( destructor != 0 )
			(*destructor)( prg, sp, el );
	}

	if ( el->id >= 0 ) { 
		short *t = prg->rtd->selInfo[el->id].trees;
		int i, len = prg->rtd->selInfo[el->id].treesLen;
		for ( i = 0; i < len; i++ ) {
			Tree *tree = colm_struct_get_field( el, Tree*, t[i] );
			treeDownref( prg, sp, tree );
		}
	}
	free( el );
}

void colm_parser_destroy( Program *prg, Tree **sp, struct colm_struct *s )
{
	struct colm_parser *parser = (struct colm_parser*) s;

	/* Free the PDA run. */
	colm_pda_clear( prg, sp, parser->pdaRun );
	free( parser->pdaRun );

	/* Free the result. */
	treeDownref( prg, sp, parser->result );
}

Parser *colm_parser_new( Program *prg, GenericInfo *gi )
{
	PdaRun *pdaRun = malloc( sizeof(PdaRun) );

	/* Start off the parsing process. */
	colm_pda_init( prg, pdaRun, prg->rtd->pdaTables, 
			gi->parserId, 0, 0, 0 );
	
	size_t memsize = sizeof(struct colm_parser);
	struct colm_parser *parser = (struct colm_parser*) malloc( memsize );
	memset( parser, 0, memsize );
	colm_struct_add( prg, (struct colm_struct*) parser );

	parser->id = STRUCT_INBUILT_ID;
	parser->destructor = &colm_parser_destroy;
	parser->pdaRun = pdaRun;

	return parser;
}

void colm_map_destroy( Program *prg, Tree **sp, struct colm_struct *s )
{
	struct colm_map *map = (struct colm_map*) s;

	MapEl *el = map->head;
	while ( el != 0 ) {
		MapEl *next = el->next;
		treeDownref( prg, sp, el->key );
		mapElFree( prg, el );
		el = next;
	}
}

Map *colm_map_new( struct colm_program *prg )
{
	size_t memsize = sizeof(struct colm_map);
	struct colm_map *map = (struct colm_map*) malloc( memsize );
	memset( map, 0, memsize );
	colm_struct_add( prg, (struct colm_struct *)map );
	map->id = STRUCT_INBUILT_ID;
	return map;
}

Struct *colm_construct_generic( Program *prg, long genericId )
{
	GenericInfo *genericInfo = &prg->rtd->genericInfo[genericId];
	Struct *newGeneric = 0;
	switch ( genericInfo->type ) {
		case GEN_MAP_EL:
		case GEN_LIST_EL:
			break;

		case GEN_MAP:
		case GEN_VMAP:
		{
			Map *map = colm_map_new( prg );
			map->genericInfo = genericInfo;
			newGeneric = (Struct*) map;
			break;
		}
		case GEN_LIST:
		case GEN_VLIST:
		{
			List *list = colm_list_new( prg );
			list->genericInfo = genericInfo;
			newGeneric = (Struct*) list;
			break;
		}
		case GEN_PARSER: {
			Parser *parser = colm_parser_new( prg, genericInfo );
			parser->input = colm_stream_new( prg );
			newGeneric = (Struct*) parser;
			break;
		}
	}

	return newGeneric;
}
