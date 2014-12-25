#ifndef _COLM_STRUCT_H

#define colm_struct_val(obj, field) \
	((struct colm_tree**)(((struct colm_struct*)obj)+1))[field]

struct colm_struct *colm_new_struct( struct colm_program *prg, int id );

#endif
