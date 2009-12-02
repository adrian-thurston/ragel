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
	RunBuf()
	:
		type(0),
		tree(0),
		length(0),
		offset(0),
		next(0)
	{}

	char buf[FSM_BUFSIZE];
	long type;
	Tree *tree;
	long length;
	long offset;
	RunBuf *next;
};

#define MARK_SLOTS 32

struct FsmRun
{
	FsmRun( Program *prg );

	Program *prg;
	FsmTables *tables;

	RunBuf *runBuf;

	/* FsmRun State. */
	long region, cs, act;
	char *tokstart, *tokend;
	char *p, *pe, *peof;
	bool returnResult;
	char *mark[MARK_SLOTS];
	long matchedToken;

	InputStream *haveDataOf;
	Tree *curStream;
};

void exec_action( FsmRun *fsmRun, GenAction *genAction );
void fsm_execute( FsmRun *fsmRun, InputStream *inputStream );
void init_fsm_run( FsmRun *fsmRun, InputStream *in );
void init_input_stream( InputStream *in );
void send_queued_tokens( Tree **sp, PdaRun *pdaRun, FsmRun *fsmRun, InputStream *inputStream );
void send_handle_error( Tree **sp, PdaRun *pdaRun, FsmRun *fsmRun, InputStream *inputStream, Kid *input );
Head *extract_match( Program *prg, FsmRun *fsmRun, InputStream *inputStream );
void send_back( Tree **sp, PdaRun *pdaRun, FsmRun *fsmRun, InputStream *inputStream, Kid *input );
void queue_back_tree( Tree **sp, PdaRun *pdaRun, FsmRun *fsmRun, InputStream *inputStream, Kid *input );
void parse_loop( Tree **sp, PdaRun *pdaRun, FsmRun *fsmRun, InputStream *inputStream );
long scan_token( PdaRun *pdaRun, FsmRun *fsmRun, InputStream *inputStream );
void send_back_text( FsmRun *fsmRun, InputStream *inputStream, const char *data, long length );

Head *stream_pull( Program *prg, FsmRun *fsmRun, InputStream *inputStream, long length );
void stream_push_text( InputStream *inputStream, const char *data, long length );
void stream_push_tree( InputStream *inputStream, Tree *tree );
void undo_stream_pull( FsmRun *fsmRun, InputStream *inputStream, const char *data, long length );
void undo_stream_push( Tree **sp, FsmRun *fsmRun, InputStream *inputStream, long length );

void send_named_lang_el( Tree **sp, PdaRun *pdaRun, FsmRun *fsmRun, InputStream *inputStream );
Kid *make_token( PdaRun *pdaRun, FsmRun *fsmRun, InputStream *inputStream, int id,
		Head *tokdata, bool namedLangEl, int bindId );

void new_token( PdaRun *pdaRun, FsmRun *fsmRun );
long scan_token( PdaRun *pdaRun, FsmRun *fsmRun, InputStream *inputStream );

#endif
