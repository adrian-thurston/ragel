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
#include <colm/input.h>
#include <colm/internal.h>

typedef unsigned char Code;
typedef unsigned long Word;
typedef unsigned long Half;
struct Bindings;
struct _FunctionInfo;

typedef struct colm_stream Stream;
typedef struct colm_parser Parser;
typedef struct colm_list List;
typedef struct colm_map Map;

typedef struct colm_location
{
	const char *name;
	long line;
	long column;
	long byte;
} Location;

/* Header located just before string data. */
typedef struct colm_data
{
	const char *data; 
	long length;
	struct colm_location *location;
} Head;

typedef struct colm_kid
{
	/* The tree needs to be first since pointers to kids are used to reference
	 * trees on the stack. A pointer to the word that is a Tree* is cast to
	 * a Kid*. */
	struct colm_tree *tree;
	struct colm_kid *next;
	unsigned char flags;
} Kid;

typedef struct _Ref
{
	Kid *kid;
	struct _Ref *next;
} Ref;

typedef struct colm_tree Tree;

typedef struct _TreePair
{
	Tree *key;
	Tree *val;
} TreePair;

typedef struct _ParseTree
{
	short id;
	unsigned short flags;

	struct _ParseTree *child;
	struct _ParseTree *next;
	struct _ParseTree *leftIgnore;
	struct _ParseTree *rightIgnore;
	Kid *shadow;

	/* Parsing algorithm. */
	long state;
	short causeReduce;

	/* Retry vars. Might be able to unify lower and upper. */
	long retryRegion;
	char retryLower;
	char retryUpper;
} ParseTree;

typedef struct _Int
{
	/* Must overlay Tree. */
	short id;
	unsigned short flags;
	long refs;
	Kid *child;

	long value;
} Int;

typedef struct _Pointer
{
	/* Must overlay Tree. */
	short id;
	unsigned short flags;
	long refs;
	Kid *child;

	colm_value_t value;
} Pointer;

typedef struct _Str
{
	/* Must overlay Tree. */
	short id;
	unsigned short flags;
	long refs;
	Kid *child;

	Head *value;
} Str;

/*
 * Maps
 */
typedef struct _GenericInfo
{
	long type;

	long elType;
	long elStructId;
	long elOffset;

	long keyType;
	long keyOffset;

	long valueType;
	long valueOffset;

	long parserId;
} GenericInfo;

enum IterType
{
	IT_Tree = 1,
	IT_RevTree,
	IT_User
};

typedef struct _TreeIter
{
	enum IterType type;
	Ref rootRef;
	Ref ref;
	long searchId;
	Tree **stackRoot;
	long argSize;
	long yieldSize;
	long rootSize;
} TreeIter;

typedef struct _ListIter
{
	enum IterType type;
	Ref rootRef;
	Ref ref;
	Tree **stackRoot;
	long argSize;
	long yieldSize;
	long rootSize;
	long genericId;
} GenericIter;

/* This must overlay tree iter because some of the same bytecodes are used. */
typedef struct _RevTreeIter
{
	enum IterType type;
	Ref rootRef;
	Ref ref;
	long searchId;
	Tree **stackRoot;
	long argSize;
	long yieldSize;
	long rootSize;

	/* For detecting a split at the leaf. */
	Kid *kidAtYield;
	long children;
} RevTreeIter;

typedef struct _UserIter
{
	enum IterType type;
	/* The current item. */
	Ref ref;
	Tree **stackRoot;
	long argSize;
	long yieldSize;
	long rootSize;

	Code *resume;
	Tree **frame;
	long searchId;
} UserIter;

void treeUpref( Tree *tree );
void treeDownref( struct colm_program *prg, Tree **sp, Tree *tree );
long cmpTree( struct colm_program *prg, const Tree *tree1, const Tree *tree2 );

Tree *pushRightIgnore( struct colm_program *prg, Tree *pushTo, Tree *rightIgnore );
Tree *pushLeftIgnore( struct colm_program *prg, Tree *pushTo, Tree *leftIgnore );
Tree *popRightIgnore( struct colm_program *prg, Tree **sp,
		Tree *popFrom, Tree **rightIgnore );
Tree *popLeftIgnore( struct colm_program *prg, Tree **sp,
		Tree *popFrom, Tree **leftIgnore );
Tree *treeLeftIgnore( struct colm_program *prg, Tree *tree );
Tree *treeRightIgnore( struct colm_program *prg, Tree *tree );
Kid *treeLeftIgnoreKid( struct colm_program *prg, Tree *tree );
Kid *treeRightIgnoreKid( struct colm_program *prg, Tree *tree );
Kid *treeChild( struct colm_program *prg, const Tree *tree );
Kid *treeAttr( struct colm_program *prg, const Tree *tree );
Kid *kidListConcat( Kid *list1, Kid *list2 );
Kid *treeExtractChild( struct colm_program *prg, Tree *tree );
Kid *reverseKidList( Kid *kid );

Tree *constructInteger( struct colm_program *prg, long i );
Tree *colm_construct_pointer( struct colm_program *prg, colm_value_t value );
Tree *constructTerm( struct colm_program *prg, Word id, Head *tokdata );
Tree *constructTree( struct colm_program *prg, Kid *kid,
		Tree **bindings, long pat );
Tree *constructObject( struct colm_program *prg, Kid *kid,
		Tree **bindings, long langElId );
Tree *constructToken( struct colm_program *prg, Tree **args, long nargs );

int testFalse( struct colm_program *prg, Tree *tree );
Tree *makeTree( struct colm_program *prg, Tree **args, long nargs );
Stream *openFile( struct colm_program *prg, Tree *name, Tree *mode );
Stream *colm_stream_open_file( struct colm_program *prg, Tree *name, Tree *mode );
Stream *colm_stream_open_fd( struct colm_program *prg, char *name, long fd );
Kid *copyIgnoreList( struct colm_program *prg, Kid *ignoreHeader );
Kid *copyKidList( struct colm_program *prg, Kid *kidList );
void streamFree( struct colm_program *prg, Stream *s );
Tree *copyTree( struct colm_program *prg, Tree *tree,
		Kid *oldNextDown, Kid **newNextDown );

colm_value_t colm_get_pointer_val( Tree *pointer );
Tree *colm_tree_get_field( Tree *tree, Word field );
Tree *getFieldSplit( struct colm_program *prg, Tree *tree, Word field );
Tree *getRhsEl( struct colm_program *prg, Tree *lhs, long position );
Kid *getRhsElKid( struct colm_program *prg, Tree *lhs, long position );
void colm_tree_set_field( struct colm_program *prg, Tree *tree, long field, Tree *value );

void setTriterCur( struct colm_program *prg, TreeIter *iter, Tree *tree );
void setUiterCur( struct colm_program *prg, UserIter *uiter, Tree *tree );
void refSetValue( struct colm_program *prg, Tree **sp, Ref *ref, Tree *v );
Tree *treeSearch( struct colm_program *prg, Tree *tree, long id );
Location *locSearch( struct colm_program *prg, Tree *tree );

int matchPattern( Tree **bindings, struct colm_program *prg,
		long pat, Kid *kid, int checkNext );
Tree *treeIterDerefCur( TreeIter *iter );

/* For making references of attributes. */
Kid *getFieldKid( Tree *tree, Word field );

Tree *copyRealTree( struct colm_program *prg, Tree *tree,
		Kid *oldNextDown, Kid **newNextDown );
void splitIterCur( struct colm_program *prg, Tree ***psp, TreeIter *iter );
Tree *setListMem( List *list, Half field, Tree *value );

void listPushTail( struct colm_program *prg, List *list, Tree *val );
void listPushHead( struct colm_program *prg, List *list, Tree *val );
Tree *listRemoveEnd( struct colm_program *prg, List *list );
Tree *listRemoveHead( struct colm_program *prg, List *list );
Tree *getListMemSplit( struct colm_program *prg, List *list, Word field );
Tree *getParserMem( Parser *parser, Word field );

Tree *treeIterAdvance( struct colm_program *prg, Tree ***psp, TreeIter *iter );
Tree *treeIterNextChild( struct colm_program *prg, Tree ***psp, TreeIter *iter );
Tree *treeRevIterPrevChild( struct colm_program *prg, Tree ***psp, RevTreeIter *iter );
Tree *treeIterNextRepeat( struct colm_program *prg, Tree ***psp, TreeIter *iter );
Tree *treeIterPrevRepeat( struct colm_program *prg, Tree ***psp, TreeIter *iter );

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
Tree *treeTrim( struct colm_program *prg, Tree **sp, Tree *tree );

void printTreeCollect( struct colm_program *prg, Tree **sp,
		StrCollect *collect, Tree *tree, int trim );
void printTreeFile( struct colm_program *prg, Tree **sp, FILE *out, Tree *tree, int trim );
void printTreeFd( struct colm_program *prg, Tree **sp, int fd, Tree *tree, int trim );
void printXmlStdout( struct colm_program *prg, Tree **sp,
		Tree *tree, int commAttr, int trim );

/*
 * Iterators.
 */

UserIter *uiterCreate( struct colm_program *prg, Tree ***psp,
		struct _FunctionInfo *fi, long searchId );
void uiterInit( struct colm_program *prg, Tree **sp, UserIter *uiter, 
		struct _FunctionInfo *fi, int revertOn );

void initTreeIter( TreeIter *treeIter, Tree **stackRoot, long argSize, long rootSize,
		const Ref *rootRef, int searchId );
void initRevTreeIter( RevTreeIter *revTriter, Tree **stackRoot, long argSize, long rootSize, 
		const Ref *rootRef, int searchId, int children );
void initUserIter( UserIter *userIter, Tree **stackRoot, long rootSize,
		long argSize, long searchId );

void treeIterDestroy( struct colm_program *prg, Tree ***psp, TreeIter *iter );
void revTreeIterDestroy( struct colm_program *prg, Tree ***psp, RevTreeIter *iter );
void userIterDestroy( struct colm_program *prg, Tree ***psp, UserIter *uiter );
void userIterDestroy2( struct colm_program *prg, Tree ***psp, UserIter *uiter );

Tree *castTree( struct colm_program *prg, int langElId, Tree *tree );
StreamImpl *streamToImpl( Stream *ptr );

void colm_init_list_iter( GenericIter *listIter, Tree **stackRoot,
		long argSize, long rootSize, const Ref *rootRef, int genericId );
void colm_list_iter_destroy( struct colm_program *prg, Tree ***psp, GenericIter *iter );
Tree *colm_list_iter_advance( struct colm_program *prg, Tree ***psp, GenericIter *iter );
Tree *colm_list_iter_deref_cur( struct colm_program *prg, GenericIter *iter );

void colm_vlist_append( struct colm_program *prg, List *list, Value value );
void colm_vlist_prepend( struct colm_program *prg, List *list, Value value );
Value colm_vlist_detach_head( struct colm_program *prg, List *list );
Value colm_vlist_detach_tail( struct colm_program *prg, List *list );

Value colm_viter_deref_cur( struct colm_program *prg, GenericIter *iter );

#if defined(__cplusplus)
}
#endif

#endif

