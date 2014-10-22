/*
 *  Copyright 2010-2012 Adrian Thurston <thurston@complang.org>
 */

#ifndef _POOL_H
#define _POOL_H

/* Allocation, number of items. */
#define FRESH_BLOCK 8128                    

#include <colm/pdarun.h>
#include <colm/map.h>
#include <colm/tree.h>

#ifdef __cplusplus
extern "C" {
#endif

void initPoolAlloc( PoolAlloc *poolAlloc, int sizeofT );

Kid *kidAllocate( Program *prg );
void kidFree( Program *prg, Kid *el );
void kidClear( Program *prg );
long kidNumLost( Program *prg );

Tree *treeAllocate( Program *prg );
void treeFree( Program *prg, Tree *el );
void treeClear( Program *prg );
long treeNumLost( Program *prg );

ParseTree *parseTreeAllocate( Program *prg );
void parseTreeFree( Program *prg, ParseTree *el );
void parseTreeClear( Program *prg );
long parseTreeNumLost( Program *prg );

ListEl *listElAllocate( Program *prg );
void listElFree( Program *prg, ListEl *el );
void listElClear( Program *prg );
long listElNumLost( Program *prg );

MapEl *mapElAllocate( Program *prg );
void mapElFree( Program *prg, MapEl *el );
void mapElClear( Program *prg );
long mapElNumLost( Program *prg );

Head *headAllocate( Program *prg );
void headFree( Program *prg, Head *el );
void headClear( Program *prg );
long headNumLost( Program *prg );

Location *locationAllocate( Program *prg );
void locationFree( Program *prg, Location *el );
void locationClear( Program *prg );
long locationNumLost( Program *prg );

Stream *streamAllocate( Program *prg );
void streamFree( Program *prg, Stream *stream );

/* Wrong place. */
TreePair mapRemove( Program *prg, Map *map, Tree *key );

#ifdef __cplusplus
}
#endif

#endif
