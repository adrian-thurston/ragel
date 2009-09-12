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

struct FsmRun
{
	FsmRun( Program *prg );
	~FsmRun();

	Program *prg;
	FsmTables *tables;

	/* FsmRun State. */
	long region, cs, act;
	char *tokstart, *tokend;
	char *p, *pe, *peof;
	bool returnResult;
	char *mark[MARK_SLOTS];
	long matchedToken;
};

void exec_action( FsmRun *fsmRun, GenAction *genAction );
void fsm_execute( InputStream *inputStream, FsmRun *fsmRun );
void init_input_stream( InputStream *in );
void send_queued_tokens( Tree **sp, InputStream *inputStream, FsmRun *fsmRun, PdaRun *pdaRun );
void send_handle_error( Tree **sp, InputStream *inputStream, FsmRun *fsmRun, PdaRun *parser, Kid *input );
Head *extract_match( Program *prg, InputStream *inputStream );
void send_back_ignore( Tree **sp, InputStream *inputStream, PdaRun *parser, Kid *ignore );
void send_back( Tree **sp, InputStream *inputStream, FsmRun *fsmRun, PdaRun *parser, Kid *input );
void queue_back( Tree **sp, InputStream *inputStream, FsmRun *fsmRun, PdaRun *parser, Kid *input );
void parse( Tree **sp, InputStream *inputStream, FsmRun *fsmRun, PdaRun *parser );
void parse_frag( Tree **sp, InputStream *inputStream, FsmRun *fsmRun, PdaRun *parser );
void parse_frag_finish( Tree **sp, InputStream *inputStream, FsmRun *fsmRun, PdaRun *parser );
long scan_token( InputStream *inputStream, FsmRun *fsmRun, PdaRun *pdaRun );
Head *extract_prefix( InputStream *inputStream, PdaRun *parser, long length );
void send_back_text( InputStream *inputStream, const char *data, long length );
void stream_push( InputStream *inputStream, const char *data, long length );
void undo_stream_push( InputStream *inputStream, long length );

#endif
