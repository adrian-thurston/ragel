/*
 *  Copyright 2007 Adrian Thurston <thurston@cs.queensu.ca>
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

struct FsmRun
{
	FsmRun( FsmTables *tables );
	~FsmRun();

	void set_AF_GROUP_MEM();

	void sendLangEl( Kid *input );
	void makeToken( int id, Head *tokdata, bool namedLangEl, int bindId );
	void translateLangEl( int id, Head *tokdata, bool namedLangEl, int bindId );
	void sendNamedLangEl();
	void sendEOF();
	void sendIgnore( long id );
	void sendQueuedTokens();
	void sendToken( long id );

	void sendBackIgnore( Kid *ignore );
	void sendBack( Kid *input );
	void queueBack( Kid *input );
	void sendBackText( const char *data, long length );
	void emitToken( KlangEl *token );
	void execAction( GenAction *action );

	long run( PdaRun *parser );
	void attachInputStream( InputStream *in );
	void streamPush( const char *data, long length );
	void undoStreamPush( long length );

	Head *extractToken( long len );

	void execute();

	FsmTables *tables;
	PdaRun *parser;
	InputStream *inputStream;

	/* FsmRun State. */
	int region, cs, act;
	char *tokstart, *tokend;
	char *p, *pe, *peof;
	bool eofSent;
	RunBuf *runBuf;
	bool gotoResume;
	long position;
	char *mark_enter[32];
	char *mark_leave[32];
};

#endif
