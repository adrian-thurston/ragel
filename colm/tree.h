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

typedef unsigned char Code;
typedef unsigned long Word;
typedef unsigned long Half;
struct Bindings;

typedef struct _File
{
	struct _File *prev;
	struct _File *next;
} File;

typedef struct _Location
{
	File *file;
	long line;
	long column;
	long byte;
} Location;

/* Header located just before string data. */
typedef struct _Head
{
	const char *data; 
	long length;
	Location *location;
} Head;

typedef struct ColmKid
{
	/* The tree needs to be first since pointers to kids are used to reference
	 * trees on the stack. A pointer to the word that is a Tree* is cast to
	 * a Kid*. */
	struct ColmTree *tree;
	struct ColmKid *next;
	unsigned char flags;
} Kid;

typedef struct ColmKid2
{
	/* The tree needs to be first since pointers to kids are used to reference
	 * trees on the stack. A pointer to the word that is a Tree* is cast to
	 * a Kid*. */
	struct _ParseTree *tree;
	struct ColmKid2 *next;
	unsigned char flags;
} Kid2;

typedef struct _Ref
{
	struct ColmKid *kid;
	struct _Ref *next;
} Ref;

typedef struct ColmTree
{
	/* First four will be overlaid in other structures. */
	short id;
	unsigned short flags;
	long refs;
	Kid *child;

	Head *tokdata;

	/* FIXME: this needs to go somewhere else. Will do for now. */
	unsigned short prodNum;
} Tree;


typedef struct _TreePair
{
	Tree *key;
	Tree *val;
} TreePair;

typedef struct _IgnoreList
{
	/* First four will be overlaid in other structures. */
	short id;
	unsigned short flags;
	long refs;
	Kid *child;

	long generation;
} IgnoreList;

typedef struct _ParseTree
{
	/* Entire structure must overlay Tree. */
	short id;
	unsigned short flags;
	long refs;
	Kid2 *child;

	Head *tokdata;

	unsigned short prodNum;

	/* Parsing algorithm. */
	long state;
	long region;
	short causeReduce;

	/* FIXME: unify probably. */
	char retryLower;
	char retryUpper;

	Kid *shadow;
	Kid2 *ignore;
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

	Kid *value;
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

typedef struct _ListEl
{
	/* Must overlay kid. */
	Tree *value;
	struct _ListEl *next;
	struct _ListEl *prev;
} ListEl;

/*
 * Maps
 */
typedef struct _GenericInfo
{
	long type;
	long typeArg;
	long keyOffset;
	long keyType;
	long langElId;
	long parserId;
} GenericInfo;

typedef struct _List
{
	/* Must overlay Tree. */
	short id;
	unsigned short flags;
	long refs;
	ListEl *head;

	ListEl *tail;
	long listLen;
	GenericInfo *genericInfo;

} List;

typedef struct _Stream
{
	/* Must overlay Tree. */
	short id;
	unsigned short flags;
	long refs;
	Kid *child;

	FILE *file;
	SourceStream *in;
} Stream;

typedef struct _Input
{
	/* Must overlay Tree. */
	short id;
	unsigned short flags;
	long refs;
	Kid *child;

	InputStream *in;
} Input;

typedef struct _Parser
{
	/* Must overlay Tree. */
	short id;
	unsigned short flags;
	long refs;
	Kid *child;

	GenericInfo *genericInfo;

	struct _PdaRun *pdaRun;
	struct _FsmRun *fsmRun;
	struct _Input *input;
	Tree *result;
} Parser;

typedef struct _TreeIter
{
	Ref rootRef;
	Ref ref;
	long searchId;
	Tree **stackRoot;
	long stackSize;
} TreeIter;

/* This must overlay tree iter because some of the same bytecodes are used. */
typedef struct _RevTreeIter
{
	Ref rootRef;
	Ref ref;
	long searchId;
	Tree **stackRoot;
	long stackSize;

	/* For detecting a split at the leaf. */
	Kid *kidAtYield;
	long children;
	Kid **cur;
} RevTreeIter;


typedef struct _UserIter
{
	/* The current item. */
	Ref ref;
	Tree **stackRoot;
	long argSize;
	long stackSize;
	Code *resume;
	Tree **frame;
	long searchId;
} UserIter;


void treeUpref( Tree *tree );
void treeDownref( struct ColmProgram *prg, Tree **sp, Tree *tree );
long cmpTree( struct ColmProgram *prg, const Tree *tree1, const Tree *tree2 );
void attachLeftIgnore( struct ColmProgram *prg, Tree *tree, IgnoreList *ignoreList );
void attachRightIgnore( struct ColmProgram *prg, Tree *tree, IgnoreList *ignoreList );
void removeLeftIgnore( struct ColmProgram *prg, Tree **sp, Tree *tree );
void removeRightIgnore( struct ColmProgram *prg, Tree **sp, Tree *tree );
IgnoreList *treeLeftIgnore( struct ColmProgram *prg, Tree *tree );
IgnoreList *treeRightIgnore( struct ColmProgram *prg, Tree *tree );
Kid *treeLeftIgnoreKid( struct ColmProgram *prg, Tree *tree );
Kid *treeRightIgnoreKid( struct ColmProgram *prg, Tree *tree );
Kid *treeChild( struct ColmProgram *prg, const Tree *tree );
Kid *treeAttr( struct ColmProgram *prg, const Tree *tree );
Kid *kidListConcat( Kid *list1, Kid *list2 );
Kid *treeExtractChild( struct ColmProgram *prg, Tree *tree );
Kid *reverseKidList( Kid *kid );

Tree *constructInteger( struct ColmProgram *prg, long i );
Tree *constructPointer( struct ColmProgram *prg, Tree *tree );
Tree *constructTerm( struct ColmProgram *prg, Word id, Head *tokdata );
Tree *constructReplacementTree( Kid *kid, Tree **bindings, struct ColmProgram *prg, long pat );
Tree *createGeneric( struct ColmProgram *prg, long genericId );
Tree *constructToken( struct ColmProgram *prg, Tree **root, long nargs );
Tree *constructInput( struct ColmProgram *prg );


int testFalse( struct ColmProgram *prg, Tree *tree );
Tree *makeTree( struct ColmProgram *prg, Tree **root, long nargs );
Stream *openFile( struct ColmProgram *prg, Tree *name, Tree *mode );
Stream *openStreamFd( struct ColmProgram *prg, long fd );
Kid *copyIgnoreList( struct ColmProgram *prg, Kid *ignoreHeader );
Kid *copyKidList( struct ColmProgram *prg, Kid *kidList );
void streamFree( struct ColmProgram *prg, Stream *s );
Tree *copyTree( struct ColmProgram *prg, Tree *tree, Kid *oldNextDown, Kid **newNextDown );

Tree *getPtrVal( Pointer *ptr );
Tree *getPtrValSplit( struct ColmProgram *prg, Pointer *ptr );
Tree *getField( Tree *tree, Word field );
Tree *getFieldSplit( struct ColmProgram *prg, Tree *tree, Word field );
Tree *getRhsEl( struct ColmProgram *prg, Tree *lhs, long position );
void setField( struct ColmProgram *prg, Tree *tree, long field, Tree *value );

void setTriterCur( struct ColmProgram *prg, TreeIter *iter, Tree *tree );
void setUiterCur( struct ColmProgram *prg, UserIter *uiter, Tree *tree );
void refSetValue( Ref *ref, Tree *v );
Tree *treeSearch( struct ColmProgram *prg, Kid *kid, long id );
Tree *treeSearch2( struct ColmProgram *prg, Tree *tree, long id );

int matchPattern( Tree **bindings, struct ColmProgram *prg, long pat, Kid *kid, int checkNext );
Tree *treeIterDerefCur( TreeIter *iter );

/* For making references of attributes. */
Kid *getFieldKid( Tree *tree, Word field );

Tree *copyRealTree( struct ColmProgram *prg, Tree *tree, Kid *oldNextDown, Kid **newNextDown );
void splitIterCur( struct ColmProgram *prg, Tree ***psp, TreeIter *iter );
Tree *setListMem( List *list, Half field, Tree *value );

void listAppend2( struct ColmProgram *prg, List *list, Tree *val );
Tree *listRemoveEnd( struct ColmProgram *prg, List *list );
Tree *getListMem( List *list, Word field );
Tree *getListMemSplit( struct ColmProgram *prg, List *list, Word field );

Tree *treeIterAdvance( struct ColmProgram *prg, Tree ***psp, TreeIter *iter );
Tree *treeIterNextChild( struct ColmProgram *prg, Tree ***psp, TreeIter *iter );
Tree *treeRevIterPrevChild( struct ColmProgram *prg, Tree ***psp, RevTreeIter *iter );
Tree *treeIterNextRepeat( struct ColmProgram *prg, Tree ***psp, TreeIter *iter );
Tree *treeIterPrevRepeat( struct ColmProgram *prg, Tree ***psp, TreeIter *iter );

void printXmlStdout( struct ColmProgram *prg, Tree **sp, Tree *tree, int commAttr );


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

void printTree( struct ColmProgram *prg, Tree **sp, StrCollect *collect, Tree *tree );
void printTermTree( struct ColmProgram *prg, Tree **sp, struct ColmPrintArgs *printArgs, Kid *kid );
void printTreeCollect( struct ColmProgram *prg, Tree **sp, StrCollect *collect, Tree *tree );
void printTreeFile( struct ColmProgram *prg, Tree **sp, FILE *out, Tree *tree );

#if defined(__cplusplus)
}
#endif

#endif

