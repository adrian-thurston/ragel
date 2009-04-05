/*
 *  Copyright 2007 Adrian Thurston <thurston@complang.org>
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

#ifndef _FSMRUN_H
#define _FSMRUN_H

#include "astring.h"
#include "pdarun.h"
#include "input.h"

#define FSM_BUFSIZE 8192
//#define FSM_BUFSIZE 8

struct GenAction;
struct KlangEl;
struct PdaRun;
struct ParseData;
struct Kid;
struct Pattern;
struct PatternItem;
struct Replacement;
struct ReplItem;

struct FsmTables
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

	GenAction **actionSwitch;
	long numActionSwitch;
};

struct RunBuf
{
	char buf[FSM_BUFSIZE];
	long length;
	long offset;
	RunBuf *next;
};

#define MARK_SLOTS 32

void parse( PdaRun *parser );

struct FsmRun
{
	FsmRun( Program *prg );
	~FsmRun();

	void sendEOF( PdaRun *parser );

	void sendBackIgnore( PdaRun *parser, Kid *ignore );
	void sendBack( PdaRun *parser, Kid *input );
	void queueBack( PdaRun *parser, Kid *input );
	void sendBackText( const char *data, long length );
	void execAction( GenAction *action );

	long scanToken( PdaRun *parser );
	void attachInputStream( InputStream *in );
	void streamPush( const char *data, long length );
	void undoStreamPush( long length );

	Head *extractMatch();
	Head *extractPrefix( PdaRun *parser, long len );

	void execute();

	Program *prg;
	FsmTables *tables;
	InputStream *inputStream;

	/* FsmRun State. */
	long region, cs, act;
	char *tokstart, *tokend;
	char *p, *pe, *peof;
	bool eofSent;
	RunBuf *runBuf;
	bool returnResult;
	char *mark[MARK_SLOTS];
	long matchedToken;
};

void send_queued_tokens( FsmRun *fsmRun, PdaRun *parser );
void send_handle_error( FsmRun *fsmRun, PdaRun *parser, Kid *input );

#endif
