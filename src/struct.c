#include <colm/program.h>
#include <colm/struct.h>

#include <stdlib.h>
#include <string.h>

struct colm_tree *colm_get_global( Program *prg, long pos )
{
	return colm_struct_get_field( prg->global, pos );
}

static struct colm_struct *colm_struct_new_size( Program *prg, int size )
{
	size_t memsize = sizeof(struct colm_struct) + ( sizeof(Tree*) * size );
	struct colm_struct *item = (struct colm_struct*) malloc( memsize );
	memset( item, 0, memsize );

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
	
	return item;
}

struct colm_struct *colm_struct_new( Program *prg, int id )
{
	struct colm_struct *s = colm_struct_new_size( prg, prg->rtd->selInfo[id].size );
	s->id = id;
	return s;
}

struct colm_struct *colm_struct_inbuilt( Program *prg, int size,
	colm_destructor_t destructor )
{
	struct colm_struct *s = colm_struct_new_size( prg, size + 1 );
	s->id = -1;
	colm_struct_set_field_type( s, colm_destructor_t, 0, destructor );
	return s;
}

void colm_struct_delete( Program *prg, Tree **sp, struct colm_struct *el )
{
	if ( el->id == -1 ) {
		colm_destructor_t destructor = colm_struct_get_field_type(
				el, colm_destructor_t, 0 );

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

void colm_parser_destroy( Program *prg, Tree **sp, struct colm_struct *parser )
{
	/* Free the PDA run. */
	PdaRun *pdaRun = colm_struct_get_field_type( parser, PdaRun *, 6 );
	clearPdaRun( prg, sp, pdaRun );
	free( pdaRun );

	/* Free the result. */
	Tree *result = colm_struct_get_field_type( parser, Tree *, 8 );
	treeDownref( prg, sp, result );
}

Parser *colm_parser_new( Program *prg, GenericInfo *gi )
{
	PdaRun *pdaRun = malloc( sizeof(PdaRun) );

	/* Start off the parsing process. */
	colm_pda_init( prg, pdaRun, prg->rtd->pdaTables, 
			gi->parserId, 0, 0, 0 );
	
	struct colm_struct *s = colm_struct_inbuilt( prg, 16, colm_parser_destroy );
	colm_struct_set_field_type( s, PdaRun*, 6, pdaRun );

	return (Parser*) s;
}

