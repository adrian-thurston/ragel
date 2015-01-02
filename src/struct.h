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

	StreamImpl *impl;
} Stream;


struct colm_struct *colm_struct_new( struct colm_program *prg, int id );
void colm_struct_delete( struct colm_program *prg, struct colm_tree **sp,
		struct colm_struct *el );

struct colm_struct *colm_struct_inbuilt( Program *prg, int size,
		colm_destructor_t destructor );


#define colm_struct_get_field( obj, field ) \
	((struct colm_tree**)(((struct colm_struct*)obj)+1))[field]

#define colm_struct_set_field( obj, field, val ) \
	((struct colm_tree**)(((struct colm_struct*)obj)+1))[field] = val

#define colm_struct_get_field_type( obj, type, field ) \
	((type*)(((struct colm_struct*)obj)+1))[field]

#define colm_struct_set_field_type( obj, type, field, val ) \
	((type*)(((struct colm_struct*)obj)+1))[field] = val


Parser *colm_parser_new( struct colm_program *prg, GenericInfo *gi );
Stream *colm_stream_new( struct colm_program *prg );

#if defined(__cplusplus)
}
#endif

#endif
