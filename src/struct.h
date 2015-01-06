#ifndef _COLM_STRUCT_H
#define _COLM_STRUCT_H

#if defined(__cplusplus)
extern "C" {
#endif


typedef struct _StreamImpl StreamImpl;

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

	struct _PdaRun *pdaRun;
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

	StreamImpl *impl;
} Stream;

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
	GenericInfo *genericInfo;
} List;

typedef struct _MapEl
{
	/* Must overlay Kid. */
	Tree *tree;
	struct _MapEl *next;
	struct _MapEl *prev;

	struct _MapEl *left, *right, *parent;
	long height;
	Tree *key;
} MapEl;

typedef struct colm_map
{
	short id;
	struct colm_struct *prev, *next;
	colm_destructor_t destructor;

	void *buffer[8];

	MapEl *head;
	MapEl *tail;
	MapEl *root;
	long treeSize;
	GenericInfo *genericInfo;
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

Parser *colm_parser_new( struct colm_program *prg, GenericInfo *gi );
Stream *colm_stream_new( struct colm_program *prg );
Stream *colm_stream_new2( struct colm_program *prg );

List *colm_list_new( struct colm_program *prg );
struct colm_struct *colm_list_get( struct colm_program *prg, List *list,
		Word genId, Word field );
struct colm_struct *colm_list_el_get( struct colm_program *prg,
		ListEl *listEl, Word genId, Word field );
ListEl *colm_list_detach_head( List *list );
long colm_list_length( List *list );

Map *colm_map_new( struct colm_program *prg );

#define STRUCT_INBUILT_ID -1

#if defined(__cplusplus)
}
#endif

#endif
