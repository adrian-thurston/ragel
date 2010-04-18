/*
 *  Copyright 2007-2009 Adrian Thurston <thurston@complang.org>
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

#ifndef _PDARUN_H
#define _PDARUN_H

#include <iostream>
#include "bytecode.h"
#include "rtvector.h"
#include "fsmrun2.h"
#include "input.h"
#include "pdarun2.h"

using std::ostream;

typedef struct _Tree Tree;

#include "map.h"

typedef struct _PdaRun
{
	int numRetry;
	Kid *stackTop;
	int errCount;
	int cs;
	int nextRegionInd;

	Program *prg;
	PdaTables *tables;
	FsmRun *fsmRun;
	int parserId;

	/* Reused. */
	RtCodeVect reverseCode;
	RtCodeVect *allReverseCode;

	bool stopParsing;
	long stopTarget;

	Kid *accumIgnore;
	Kid *queue, *queueLast;

	Bindings *bindings;

	bool revertOn;

	Tree *context;

	//bool fragStop;
	bool stop;

	long consumed;
	long targetConsumed;
} PdaRun;


MapEl *mapCopyBranch( Program *prg, Map *map, MapEl *el, Kid *oldNextDown, Kid *&newNextDown );
MapEl *mapInsert( Program *prg, Map *map, MapEl *element, MapEl **lastFound = 0 );
MapEl *mapInsert( Program *prg, Map *map, Tree *key, MapEl **lastFound = 0 );
MapEl *mapImplFind( Program *prg, Map *map, Tree *key );
void mapImplRemove( Program *prg, Map *map, MapEl *element );
bool mapImplRemove( Program *prg, Map *map, Tree *key );
MapEl *mapDetachByKey( Program *prg, Map *map, Tree *key );
MapEl *mapDetach( Program *prg, Map *map, MapEl *element );

/*
 * Iterators.
 */

void initTreeIter( TreeIter *treeIter, const Ref *rootRef, int searchId, Tree **stackRoot );
void initRevTreeIter( RevTreeIter *revTriter, const Ref *rootRef, 
		int searchId, Tree **stackRoot, int children );

typedef struct _Accum
{
	/* Must overlay Tree. */
	short id;
	unsigned short flags;
	long refs;
	Kid *child;

	GenericInfo *genericInfo;

	PdaRun *pdaRun;
	FsmRun *fsmRun;
	Stream *stream;
} Accum;


void initUserIter( UserIter *userIter, Tree **stackRoot, long argSize, long searchId );

bool makeReverseCode( RtCodeVect *all, RtCodeVect &reverseCode );


void initPdaRun( PdaRun *pdaRun, Program *prg, PdaTables *tables,
		FsmRun *fsmRun, int parserId, long stopTarget, bool revertOn, Tree *context );

void clearContext( PdaRun *pdaRun, Tree **sp );
Kid *extractIgnore( PdaRun *pdaRun );
long stackTopTarget( PdaRun *pdaRun );
void commitKid( PdaRun *pdaRun, Tree **root, Kid *lel );
void runCommit( PdaRun *pdaRun );
bool isParserStopFinished( PdaRun *pdaRun );
void pdaRunMatch(  PdaRun *pdaRun, Kid *tree, Kid *pattern );


/* Offset can be used to look at the next nextRegionInd. */
int pdaRunGetNextRegion( PdaRun *pdaRun, int offset );

void cleanParser( Tree **root, PdaRun *pdaRun );
void ignore( PdaRun *pdaRun, Tree *tree );
void parseToken( Tree **sp, PdaRun *pdaRun, FsmRun *fsmRun, InputStream *inputStream, Kid *input );
long undoParse( Tree **sp, PdaRun *pdaRun, FsmRun *fsmRun, InputStream *inputStream, Tree *tree );
void xml_print_list( RuntimeData *runtimeData, Kid *lel, int depth );
ostream &parseError( InputStream *inputStream, FsmRun *fsmRun, PdaRun *pdaRun, int tokId, Tree *tree );

#endif /* _PDARUN_H */
