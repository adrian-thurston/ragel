#ifndef _COLM_STRUCT_H
#define _COLM_STRUCT_H

#if defined(__cplusplus)
extern "C" {
#endif

typedef void (*colm_destructor_t)( struct colm_program *prg,
		Tree **sp, struct colm_struct *s );

struct colm_struct
{
	short id;
	struct colm_struct *prev, *next;
};

/* Must overlay colm_struct. */
struct colm_inbuilt
{
	short id;
	struct colm_struct *prev, *next;
	colm_destructor_t destructor;
};

/* Must overlay colm_inbuilt. */
typedef struct colm_parser
{
	short id;
	struct colm_struct *prev, *next;
	colm_destructor_t destructor;

	void *buffer[10];

	struct pda_run *pdaRun;
	struct colm_stream *input;
	Tree *result;
} Parser;


/* Must overlay colm_inbuilt. */
typedef struct colm_stream
{
	short id;
	struct colm_struct *prev, *next;
	colm_destructor_t destructor;

	void *buffer[8];

	struct stream_impl *impl;
} Stream;

#define COLM_LIST_EL_SIZE 2
typedef struct colm_list_el
{
	struct colm_list_el *list_next;
	struct colm_list_el *list_prev;
} ListEl;

/* Must overlay colm_inbuilt. */
typedef struct colm_list
{
	short id;
	struct colm_struct *prev, *next;
	colm_destructor_t destructor;

	void *buffer[8];

	ListEl *head, *tail;
	long listLen;
	struct generic_info *genericInfo;
} List;

typedef struct colm_map_el
{
	Tree *key;

	struct colm_map_el *left, *right, *parent;
	long height;

	struct colm_map_el *next, *prev;
} MapEl;

#define COLM_MAP_EL_SIZE ( sizeof(colm_map_el) / sizeof(void*) )

typedef struct colm_map
{
	short id;
	struct colm_struct *prev, *next;
	colm_destructor_t destructor;

	void *buffer[8];

	struct colm_map_el *head, *tail, *root;
	long treeSize;
	struct generic_info *genericInfo;
} Map;

struct colm_struct *colm_struct_new_size( struct colm_program *prg, int size );
struct colm_struct *colm_struct_new( struct colm_program *prg, int id );
void colm_struct_add( struct colm_program *prg, struct colm_struct *item );
void colm_struct_delete( struct colm_program *prg, struct colm_tree **sp,
		struct colm_struct *el );

struct colm_struct *colm_struct_inbuilt( struct colm_program *prg, int size,
		colm_destructor_t destructor );

#define colm_struct_get_field( obj, type, field ) \
	(type)(((void**)(((struct colm_struct*)obj)+1))[field])

#define colm_struct_set_field( obj, type, field, val ) \
	((type*)(((struct colm_struct*)obj)+1))[field] = val

#define colm_struct_get_addr( obj, type, field ) \
	(type)(&(((void **)(((struct colm_struct*)obj)+1))[field]))

#define colm_struct_container( el, field ) \
	((void*)el) - (field * sizeof(void*)) - sizeof(struct colm_struct)

#define colm_generic_el_container( prg, el, genId ) \
	colm_struct_container( el, prg->rtd->genericInfo[genId].elOffset )

#define colm_struct_to_list_el( prg, obj, genId ) \
	colm_struct_get_addr( obj, ListEl*, prg->rtd->genericInfo[genId].elOffset )

#define colm_struct_to_map_el( prg, obj, genId ) \
	colm_struct_get_addr( obj, MapEl*, prg->rtd->genericInfo[genId].elOffset )

Parser *colm_parser_new( struct colm_program *prg, struct generic_info *gi );
Stream *colm_stream_new( struct colm_program *prg );
Stream *colm_stream_new_struct( struct colm_program *prg );

List *colm_list_new( struct colm_program *prg );
struct colm_struct *colm_list_get( struct colm_program *prg, List *list,
		word_t genId, word_t field );
struct colm_struct *colm_list_el_get( struct colm_program *prg,
		ListEl *listEl, word_t genId, word_t field );
ListEl *colm_list_detach_head( List *list );
ListEl *colm_list_detach_tail( List *list );
long colm_list_length( List *list );

Map *colm_map_new( struct colm_program *prg );
struct colm_struct *colm_map_el_get( struct colm_program *prg,
		MapEl *mapEl, word_t genId, word_t field );
struct colm_struct *colm_map_get( struct colm_program *prg, Map *map,
		word_t genId, word_t field );

struct colm_struct *colm_construct_generic( struct colm_program *prg, long genericId );

#define STRUCT_INBUILT_ID -1

#if defined(__cplusplus)
}
#endif

#endif
