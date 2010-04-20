/*
 *  Copyright 2007-2010 Adrian Thurston <thurston@complang.org>
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

#ifndef _FSMRUN2_H
#define _FSMRUN2_H

#include "input.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MARK_SLOTS 32

typedef struct _FsmTables
{
	long *actions;
	long *keyOffsets;
	char *transKeys;
	long *singleLengths;
	long *rangeLengths;
	long *indexOffsets;
	long *transTargsWI;
	long *transActionsWI;
	long *toStateActions;
	long *fromStateActions;
	long *eofActions;
	long *eofTargs;
	long *entryByRegion;

	long numStates;
	long numActions;
	long numTransKeys;
	long numSingleLengths;
	long numRangeLengths;
	long numIndexOffsets;
	long numTransTargsWI;
	long numTransActionsWI;
	long numRegions;

	long startState;
	long firstFinal;
	long errorState;

	struct GenAction **actionSwitch;
	long numActionSwitch;
} FsmTables;

struct _Program;

typedef struct _FsmRun
{
	struct _Program *prg;
	FsmTables *tables;

	RunBuf *runBuf;

	/* FsmRun State. */
	long region, cs, act;
	char *tokstart, *tokend;
	char *p, *pe, *peof;
	int returnResult;
	char *mark[MARK_SLOTS];
	long matchedToken;

	InputStream *haveDataOf;
	struct _Tree *curStream;
} FsmRun;

void initFsmRun( FsmRun *fsmRun, struct _Program *prg );
void updatePosition( InputStream *inputStream, const char *data, long length );
void undoPosition( InputStream *inputStream, const char *data, long length );
void takeBackBuffered( InputStream *inputStream );
void connect( FsmRun *fsmRun, InputStream *inputStream );
void sendBackRunBufHead( FsmRun *fsmRun, InputStream *inputStream );
void undoStreamPull( FsmRun *fsmRun, InputStream *inputStream, const char *data, long length );


#ifdef __cplusplus
}
#endif

#endif
