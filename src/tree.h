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

#ifndef __COLM_TREE_H
#define __COLM_TREE_H

#if defined(__cplusplus)
extern "C" {
#endif

#include <colm/colm.h>
#include <colm/type.h>
#include <colm/input.h>
#include <colm/internal.h>

typedef unsigned char code_t;
typedef unsigned long word_t;
typedef unsigned long half_t;

struct bindings;
struct function_info;

typedef struct colm_stream stream_t;
typedef struct colm_parser parser_t;
typedef struct colm_list list_t;
typedef struct colm_list_el list_el_t;
typedef struct colm_map map_t;
typedef struct colm_map_el map_el_t;

typedef struct colm_location
{
	const char *name;
	long line;
	long column;
	long byte;
} location_t;

/* Header located just before string data. */
typedef struct colm_data
{
	const char *data; 
	long length;
	struct colm_location *location;
} head_t;

typedef struct colm_kid
{
	/* The tree needs to be first since pointers to kids are used to reference
	 * trees on the stack. A pointer to the word that is a tree_t* is cast to
	 * a kid_t*. */
	struct colm_tree *tree;
	struct colm_kid *next;
	unsigned char flags;
} kid_t;

typedef struct colm_ref
{
	kid_t *kid;
	struct colm_ref *next;
} ref_t;

typedef struct colm_tree tree_t;

struct tree_pair
{
	tree_t *key;
	tree_t *val;
};

typedef struct colm_parse_tree
{
	short id;
	unsigned short flags;

	struct colm_parse_tree *child;
	struct colm_parse_tree *next;
	struct colm_parse_tree *leftIgnore;
	struct colm_parse_tree *rightIgnore;
	kid_t *shadow;

	/* Parsing algorithm. */
	long state;
	short causeReduce;

	/* Retry vars. Might be able to unify lower and upper. */
	long retryRegion;
	char retryLower;
	char retryUpper;
} parse_tree_t;

typedef struct colm_pointer
{
	/* Must overlay tree_t. */
	short id;
	unsigned short flags;
	long refs;
	kid_t *child;

	colm_value_t value;
} pointer_t;

typedef struct colm_str
{
	/* Must overlay tree_t. */
	short id;
	unsigned short flags;
	long refs;
	kid_t *child;

	head_t *value;
} str_t;

/*
 * Maps
 */
struct generic_info
{
	long type;

	long elType;
	long elStructId;
	long elOffset;

	enum TYPE keyType;
	long keyOffset;

	enum TYPE valueType;
	long valueOffset;

	long parserId;
};

enum IterType
{
	IT_Tree = 1,
	IT_RevTree,
	IT_User
};

typedef struct _TreeIter
{
	enum IterType type;
	ref_t rootRef;
	ref_t ref;
	long searchId;
	tree_t **stackRoot;
	long argSize;
	long yieldSize;
	long rootSize;
} tree_iter_t;

typedef struct colm_generic_iter
{
	enum IterType type;
	ref_t rootRef;
	ref_t ref;
	tree_t **stackRoot;
	long argSize;
	long yieldSize;
	long rootSize;
	long genericId;
} generic_iter_t;

/* This must overlay tree iter because some of the same bytecodes are used. */
typedef struct _RevTreeIter
{
	enum IterType type;
	ref_t rootRef;
	ref_t ref;
	long searchId;
	tree_t **stackRoot;
	long argSize;
	long yieldSize;
	long rootSize;

	/* For detecting a split at the leaf. */
	kid_t *kidAtYield;
	long children;
} rev_tree_iter_t;

typedef struct colm_user_iter
{
	enum IterType type;
	/* The current item. */
	ref_t ref;
	tree_t **stackRoot;
	long argSize;
	long yieldSize;
	long rootSize;

	code_t *resume;
	tree_t **frame;
	long searchId;
} user_iter_t;

void colm_tree_upref( tree_t *tree );
void colm_tree_downref( struct colm_program *prg, tree_t **sp, tree_t *tree );
long colm_cmp_tree( struct colm_program *prg, const tree_t *tree1, const tree_t *tree2 );

tree_t *push_right_ignore( struct colm_program *prg, tree_t *pushTo, tree_t *rightIgnore );
tree_t *push_left_ignore( struct colm_program *prg, tree_t *pushTo, tree_t *leftIgnore );
tree_t *popRightIgnore( struct colm_program *prg, tree_t **sp,
		tree_t *popFrom, tree_t **rightIgnore );
tree_t *popLeftIgnore( struct colm_program *prg, tree_t **sp,
		tree_t *popFrom, tree_t **leftIgnore );
tree_t *treeLeftIgnore( struct colm_program *prg, tree_t *tree );
tree_t *treeRightIgnore( struct colm_program *prg, tree_t *tree );
kid_t *treeLeftIgnoreKid( struct colm_program *prg, tree_t *tree );
kid_t *treeRightIgnoreKid( struct colm_program *prg, tree_t *tree );
kid_t *treeChild( struct colm_program *prg, const tree_t *tree );
kid_t *treeAttr( struct colm_program *prg, const tree_t *tree );
kid_t *kidListConcat( kid_t *list1, kid_t *list2 );
kid_t *treeExtractChild( struct colm_program *prg, tree_t *tree );
kid_t *reverseKidList( kid_t *kid );

tree_t *colm_construct_pointer( struct colm_program *prg, colm_value_t value );
tree_t *colm_construct_term( struct colm_program *prg, word_t id, head_t *tokdata );
tree_t *colm_construct_tree( struct colm_program *prg, kid_t *kid,
		tree_t **bindings, long pat );
tree_t *colm_construct_object( struct colm_program *prg, kid_t *kid,
		tree_t **bindings, long langElId );
tree_t *colm_construct_token( struct colm_program *prg, tree_t **args, long nargs );

int testFalse( struct colm_program *prg, tree_t *tree );
tree_t *makeTree( struct colm_program *prg, tree_t **args, long nargs );
stream_t *openFile( struct colm_program *prg, tree_t *name, tree_t *mode );
stream_t *colm_stream_open_file( struct colm_program *prg, tree_t *name, tree_t *mode );
stream_t *colm_stream_open_fd( struct colm_program *prg, char *name, long fd );
kid_t *copyIgnoreList( struct colm_program *prg, kid_t *ignoreHeader );
kid_t *copyKidList( struct colm_program *prg, kid_t *kidList );
void colm_stream_free( struct colm_program *prg, stream_t *s );
tree_t *colm_copy_tree( struct colm_program *prg, tree_t *tree,
		kid_t *oldNextDown, kid_t **newNextDown );

colm_value_t colm_get_pointer_val( tree_t *pointer );
tree_t *colm_tree_get_field( tree_t *tree, word_t field );
tree_t *getFieldSplit( struct colm_program *prg, tree_t *tree, word_t field );
tree_t *getRhsEl( struct colm_program *prg, tree_t *lhs, long position );
kid_t *getRhsElKid( struct colm_program *prg, tree_t *lhs, long position );
void colm_tree_set_field( struct colm_program *prg, tree_t *tree, long field, tree_t *value );

void setTriterCur( struct colm_program *prg, tree_iter_t *iter, tree_t *tree );
void setUiterCur( struct colm_program *prg, user_iter_t *uiter, tree_t *tree );
void refSetValue( struct colm_program *prg, tree_t **sp, ref_t *ref, tree_t *v );
tree_t *treeSearch( struct colm_program *prg, tree_t *tree, long id );
location_t *locSearch( struct colm_program *prg, tree_t *tree );

int matchPattern( tree_t **bindings, struct colm_program *prg,
		long pat, kid_t *kid, int checkNext );
tree_t *treeIterDerefCur( tree_iter_t *iter );

/* For making references of attributes. */
kid_t *getFieldKid( tree_t *tree, word_t field );

tree_t *copyRealTree( struct colm_program *prg, tree_t *tree,
		kid_t *oldNextDown, kid_t **newNextDown );
void splitIterCur( struct colm_program *prg, tree_t ***psp, tree_iter_t *iter );
tree_t *setListMem( list_t *list, half_t field, tree_t *value );

void listPushTail( struct colm_program *prg, list_t *list, tree_t *val );
void listPushHead( struct colm_program *prg, list_t *list, tree_t *val );
tree_t *listRemoveEnd( struct colm_program *prg, list_t *list );
tree_t *listRemoveHead( struct colm_program *prg, list_t *list );
tree_t *getListMemSplit( struct colm_program *prg, list_t *list, word_t field );
tree_t *getParserMem( parser_t *parser, word_t field );

tree_t *treeIterAdvance( struct colm_program *prg, tree_t ***psp, tree_iter_t *iter );
tree_t *treeIterNextChild( struct colm_program *prg, tree_t ***psp, tree_iter_t *iter );
tree_t *treeRevIterPrevChild( struct colm_program *prg, tree_t ***psp, rev_tree_iter_t *iter );
tree_t *treeIterNextRepeat( struct colm_program *prg, tree_t ***psp, tree_iter_t *iter );
tree_t *treeIterPrevRepeat( struct colm_program *prg, tree_t ***psp, tree_iter_t *iter );

/* An automatically grown buffer for collecting tokens. Always reuses space;
 * never down resizes. */
typedef struct _StrCollect
{
	char *data;
	int allocated;
	int length;
} StrCollect;

void initStrCollect( StrCollect *collect );
void strCollectDestroy( StrCollect *collect );
void strCollectAppend( StrCollect *collect, const char *data, long len );
void strCollectClear( StrCollect *collect );
tree_t *treeTrim( struct colm_program *prg, tree_t **sp, tree_t *tree );

void printTreeCollect( struct colm_program *prg, tree_t **sp,
		StrCollect *collect, tree_t *tree, int trim );
void printTreeFile( struct colm_program *prg, tree_t **sp, FILE *out, tree_t *tree, int trim );
void printTreeFd( struct colm_program *prg, tree_t **sp, int fd, tree_t *tree, int trim );
void printXmlStdout( struct colm_program *prg, tree_t **sp,
		tree_t *tree, int commAttr, int trim );

/*
 * Iterators.
 */

user_iter_t *colm_uiter_create( struct colm_program *prg, tree_t ***psp,
		struct function_info *fi, long searchId );
void uiterInit( struct colm_program *prg, tree_t **sp, user_iter_t *uiter, 
		struct function_info *fi, int revertOn );

void colm_init_tree_iter( tree_iter_t *treeIter, tree_t **stackRoot,
		long argSize, long rootSize, const ref_t *rootRef, int searchId );
void colm_init_rev_tree_iter( rev_tree_iter_t *revTriter, tree_t **stackRoot,
		long argSize, long rootSize, const ref_t *rootRef, int searchId, int children );
void colm_init_user_iter( user_iter_t *userIter, tree_t **stackRoot, long rootSize,
		long argSize, long searchId );

void colm_tree_iter_destroy( struct colm_program *prg, tree_t ***psp, tree_iter_t *iter );
void colm_rev_tree_iter_destroy( struct colm_program *prg, tree_t ***psp, rev_tree_iter_t *iter );
void colm_uiter_destroy( struct colm_program *prg, tree_t ***psp, user_iter_t *uiter );
void colm_uiter_unwind( struct colm_program *prg, tree_t ***psp, user_iter_t *uiter );

tree_t *castTree( struct colm_program *prg, int langElId, tree_t *tree );
struct stream_impl *streamToImpl( stream_t *ptr );

void colm_init_list_iter( generic_iter_t *listIter, tree_t **stackRoot,
		long argSize, long rootSize, const ref_t *rootRef, int genericId );
void colm_list_iter_destroy( struct colm_program *prg, tree_t ***psp, generic_iter_t *iter );
tree_t *colm_list_iter_advance( struct colm_program *prg, tree_t ***psp, generic_iter_t *iter );
tree_t *colm_list_iter_deref_cur( struct colm_program *prg, generic_iter_t *iter );
void colm_list_append( struct colm_list *list, struct colm_list_el *newEl );
void colm_list_prepend( struct colm_list *list, struct colm_list_el *newEl );

void colm_vlist_append( struct colm_program *prg, list_t *list, value_t value );
void colm_vlist_prepend( struct colm_program *prg, list_t *list, value_t value );
value_t colm_vlist_detach_head( struct colm_program *prg, list_t *list );
value_t colm_vlist_detach_tail( struct colm_program *prg, list_t *list );

value_t colm_viter_deref_cur( struct colm_program *prg, generic_iter_t *iter );

str_t *string_prefix( program_t *prg, str_t *str, long len );
str_t *string_suffix( program_t *prg, str_t *str, long pos );
head_t *stringAllocFull( struct colm_program *prg, const char *data, long length );
tree_t *constructString( struct colm_program *prg, head_t *s );

#if defined(__cplusplus)
}
#endif

#endif

