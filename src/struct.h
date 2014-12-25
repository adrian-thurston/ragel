#ifndef _COLM_STRUCT_H

struct colm_struct *colm_struct_new( struct colm_program *prg, int id );
void colm_struct_delete( struct colm_program *prg, struct colm_tree **sp,
		struct colm_struct *el );

#define colm_struct_get_field(obj, field) \
	((struct colm_tree**)(((struct colm_struct*)obj)+1))[field]

#define colm_struct_set_field(obj, field, val) \
	((struct colm_tree**)(((struct colm_struct*)obj)+1))[field] = val

#endif
