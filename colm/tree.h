/*
 *  Copyright 2010 Adrian Thurston <thurston@complang.org>
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

#ifndef _TREE_H
#define _TREE_H

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct _TreePair
{
//	TreePair() : key(0), val(0) {}

	Tree *key;
	Tree *val;
} TreePair;


void treeUpref( Tree *tree );
void treeDownref( Program *prg, Tree **sp, Tree *tree );
long cmpTree( Program *prg, const Tree *tree1, const Tree *tree2 );
Kid *treeIgnore( Program *prg, Tree *tree );
Kid *treeChild( Program *prg, const Tree *tree );
Kid *kidListConcat( Kid *list1, Kid *list2 );
Kid *treeExtractChild( Program *prg, Tree *tree );

Tree *constructInteger( Program *prg, long i );
Tree *constructPointer( Program *prg, Tree *tree );
Tree *constructTerm( Program *prg, Word id, Head *tokdata );
Tree *constructReplacementTree( Tree **bindings, Program *prg, long pat );
Tree *createGeneric( Program *prg, long genericId );

Tree *makeToken2( Tree **root, Program *prg, long nargs );
int testFalse( Program *prg, Tree *tree );
Tree *makeTree( Tree **root, Program *prg, long nargs );
Stream *openFile( Program *prg, Tree *name, Tree *mode );
Stream *openStreamFd( Program *prg, long fd );
Kid *copyIgnoreList( Program *prg, Kid *ignoreHeader );
void streamFree( Program *prg, Stream *s );
Tree *copyTree( Program *prg, Tree *tree, Kid *oldNextDown, Kid **newNextDown );

Tree *getPtrVal( Pointer *ptr );
Tree *getPtrValSplit( Program *prg, Pointer *ptr );
Tree *getField( Tree *tree, Word field );
Tree *getFieldSplit( Program *prg, Tree *tree, Word field );
Tree *getRhsEl( Program *prg, Tree *lhs, long position );
void setField( Program *prg, Tree *tree, long field, Tree *value );

void setTriterCur( TreeIter *iter, Tree *tree );
void refSetValue( Ref *ref, Tree *v );
Tree *treeSearch( Program *prg, Kid *kid, long id );
Tree *treeSearch2( Program *prg, Tree *tree, long id );

int matchPattern( Tree **bindings, Program *prg, long pat, Kid *kid, int checkNext );
Tree *treeIterDerefCur( TreeIter *iter );

/* For making references of attributes. */
Kid *getFieldKid( Tree *tree, Word field );

Tree *copyRealTree( Program *prg, Tree *tree, Kid *oldNextDown, Kid **newNextDown, int parsed );
void splitIterCur( Tree ***psp, Program *prg, TreeIter *iter );
Tree *setListMem( List *list, Half field, Tree *value );

void listAppend2( Program *prg, List *list, Tree *val );
Tree *listRemoveEnd( Program *prg, List *list );
Tree *getListMem( List *list, Word field );
Tree *getListMemSplit( Program *prg, List *list, Word field );

Tree *treeIterAdvance( Program *prg, Tree ***psp, TreeIter *iter );
Tree *treeIterNextChild( Program *prg, Tree ***psp, TreeIter *iter );
Tree *treeRevIterPrevChild( Program *prg, Tree ***psp, RevTreeIter *iter );
Tree *treeIterNextRepeat( Program *prg, Tree ***psp, TreeIter *iter );
Tree *treeIterPrevRepeat( Program *prg, Tree ***psp, TreeIter *iter );

void printXmlTree( Tree **sp, Program *prg, Tree *tree, int commAttr );


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

void printTree( StrCollect *collect, Tree **sp, Program *prg, Tree *tree );

#if defined(__cplusplus)
}
#endif

#endif

