#include <colm/program.h>
#include <colm/struct.h>

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define STRUCT_INBUILT_ID -1

struct colm_tree *colm_get_global( Program *prg, long pos )
{
	return colm_struct_get_field( prg->global, pos );
}

static void colm_struct_add( Program *prg, struct colm_struct *item )
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
			Tree *tree = colm_struct_get_field( el, t[i] );
			treeDownref( prg, sp, tree );
		}
	}
	free( el );
}

void colm_parser_destroy( Program *prg, Tree **sp, struct colm_struct *s )
{
	struct colm_parser *parser = (struct colm_parser*) s;

	/* Free the PDA run. */
	clearPdaRun( prg, sp, parser->pdaRun );
	free( parser->pdaRun );

//	/* Free the result. */
//	treeDownref( prg, sp, parser->result );
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

Stream *colm_stream_new2( Program *prg )
{
	size_t memsize = sizeof(struct colm_stream);
	struct colm_stream *stream = (struct colm_stream*) malloc( memsize );
	memset( stream, 0, memsize );
	colm_struct_add( prg, (struct colm_struct *)stream );
	stream->id = STRUCT_INBUILT_ID;
	return stream;
}

void colm_list_destroy( Program *prg, Tree **sp, struct colm_struct *s )
{
	struct colm_list *list = (struct colm_list*) s;

	ListEl *el = list->head;
	while ( el != 0 ) {
		ListEl *next = el->list_next;
//		treeDownref( prg, sp, el->value );
		//listElFree( prg, el );
		el = next;
	}
}

List *colm_list_new( struct colm_program *prg )
{
	size_t memsize = sizeof(struct colm_list);
	struct colm_list *list = (struct colm_list*) malloc( memsize );
	memset( list, 0, memsize );
	colm_struct_add( prg, (struct colm_struct *)list );
	list->id = STRUCT_INBUILT_ID;
	list->destructor = &colm_list_destroy;
	return list;
}

void colm_map_destroy( Program *prg, Tree **sp, struct colm_struct *s )
{
	struct colm_map *map = (struct colm_map*) s;

	MapEl *el = map->head;
	while ( el != 0 ) {
		MapEl *next = el->next;
		treeDownref( prg, sp, el->key );
		treeDownref( prg, sp, el->tree );
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

Tree *colm_list_get( List *list, Word field )
{
	Tree *result = 0;
	switch ( field ) {
		case 0: 
			result = (Tree*)list->head;
			break;
		case 1: 
			result = (Tree*)list->tail;
			break;
		default:
			assert( 0 );
			break;
	}
	return result;
}

Tree *colm_list_el_get( ListEl *listEl, Word field )
{
	Tree *result = 0;
	switch ( field ) {
		case 0: 
			result = (Tree*)listEl->list_prev;
			break;
		case 1: 
			result = (Tree*)listEl->list_next;
			break;
		case 2: 
//			result = listEl->value;
//			treeUpref( result );
			break;
//		default:
//			assert( false );
//			break;
	}
	return result;
}
