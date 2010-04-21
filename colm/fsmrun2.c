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

#include "fsmrun2.h"
#include "pdarun.h"
#include "input.h"
#include "debug.h"
#include "tree.h"
#include "bytecode2.h"
#include "pool.h"

#include <string.h>
#include <assert.h>

#define false 0
#define true 1

void listPrepend( List *list, ListEl *new_el) { listAddBefore(list, list->head, new_el); }
void listAppend( List *list, ListEl *new_el)  { listAddAfter(list, list->tail, new_el); }

ListEl *listDetach( List *list, ListEl *el );
ListEl *listDetachFirst(List *list )        { return listDetach(list, list->head); }
ListEl *listDetachLast(List *list )         { return listDetach(list, list->tail); }

long listLength(List *list)
	{ return list->listLen; }

void initFsmRun( FsmRun *fsmRun, Program *prg )
{
	fsmRun->prg = prg;
	fsmRun->tables = prg->rtd->fsmTables;
	fsmRun->runBuf = 0;
	fsmRun->haveDataOf = 0;
	fsmRun->curStream = 0;

	/* Run buffers need to stick around because 
	 * token strings point into them. */
	fsmRun->runBuf = newRunBuf();
	fsmRun->runBuf->next = 0;

	fsmRun->p = fsmRun->pe = fsmRun->runBuf->data;
	fsmRun->peof = 0;
}


/* Keep the position up to date after consuming text. */
void updatePosition( InputStream *inputStream, const char *data, long length )
{
	int i;
	if ( !inputStream->handlesLine ) {
		for ( i = 0; i < length; i++ ) {
			if ( data[i] != '\n' )
				inputStream->column += 1;
			else {
				inputStream->line += 1;
				inputStream->column = 1;
			}
		}
	}

	inputStream->byte += length;
}

/* Keep the position up to date after sending back text. */
void undoPosition( InputStream *inputStream, const char *data, long length )
{
	/* FIXME: this needs to fetch the position information from the parsed
	 * token and restore based on that.. */
	int i;
	if ( !inputStream->handlesLine ) {
		for ( i = 0; i < length; i++ ) {
			if ( data[i] == '\n' )
				inputStream->line -= 1;
		}
	}

	inputStream->byte -= length;
}

void incrementConsumed( PdaRun *pdaRun )
{
	pdaRun->consumed += 1;
	debug( REALM_PARSE, "consumed up to %ld\n", pdaRun->consumed );
}

void decrementConsumed( PdaRun *pdaRun )
{
	pdaRun->consumed -= 1;
	debug( REALM_PARSE, "consumed down to %ld\n", pdaRun->consumed );
}

void takeBackBuffered( InputStream *inputStream )
{
	if ( inputStream->hasData != 0 ) {
		FsmRun *fsmRun = inputStream->hasData;

		if ( fsmRun->runBuf != 0 ) {
			if ( fsmRun->pe - fsmRun->p > 0 ) {
				debug( REALM_PARSE, "taking back buffered fsmRun: %p input stream: %p\n", fsmRun, inputStream );

				RunBuf *split = newRunBuf();
				memcpy( split->data, fsmRun->p, fsmRun->pe - fsmRun->p );

				split->length = fsmRun->pe - fsmRun->p;
				split->offset = 0;
				split->next = 0;

				fsmRun->pe = fsmRun->p;

				inputStream->funcs->pushBackBuf( inputStream, split );
			}
		}

		inputStream->hasData = 0;
		fsmRun->haveDataOf = 0;
	}
}

void connect( FsmRun *fsmRun, InputStream *inputStream )
{
	if ( inputStream->hasData != 0 && inputStream->hasData != fsmRun ) {
		takeBackBuffered( inputStream );
	}
	
#ifdef COLM_LOG
	if ( inputStream->hasData != fsmRun )
		debug( REALM_PARSE, "connecting fsmRun: %p and input stream %p\n", fsmRun, inputStream );
#endif

	inputStream->hasData = fsmRun;
	fsmRun->haveDataOf = inputStream;
}

/* Load up a token, starting from tokstart if it is set. If not set then
 * start it at data. */
Head *streamPull( Program *prg, FsmRun *fsmRun, InputStream *inputStream, long length )
{
	/* We should not be in the midst of getting a token. */
	assert( fsmRun->tokstart == 0 );

	/* The generated token length has been stuffed into tokdata. */
	if ( fsmRun->p + length > fsmRun->pe ) {
		fsmRun->p = fsmRun->pe = fsmRun->runBuf->data;
		fsmRun->peof = 0;

		long space = fsmRun->runBuf->data + FSM_BUFSIZE - fsmRun->pe;
			
		if ( space == 0 )
			fatal( "OUT OF BUFFER SPACE\n" );

		connect( fsmRun, inputStream );
			
		long len = inputStream->funcs->getData( inputStream, fsmRun->p, space );
		fsmRun->pe = fsmRun->p + len;
	}

	if ( fsmRun->p + length > fsmRun->pe )
		fatal( "NOT ENOUGH DATA TO FETCH TOKEN\n" );

	Head *tokdata = stringAllocPointer( prg, fsmRun->p, length );
	updatePosition( inputStream, fsmRun->p, length );
	fsmRun->p += length;

	return tokdata;
}

void sendBackRunBufHead( FsmRun *fsmRun, InputStream *inputStream )
{
	debug( REALM_PARSE, "pushing back runbuf\n" );

	/* Move to the next run buffer. */
	RunBuf *back = fsmRun->runBuf;
	fsmRun->runBuf = fsmRun->runBuf->next;
		
	/* Flush out the input buffer. */
	back->length = fsmRun->pe - fsmRun->p;
	back->offset = 0;
	inputStream->funcs->pushBackBuf( inputStream, back );

	/* Set data and de. */
	if ( fsmRun->runBuf == 0 ) {
		fsmRun->runBuf = newRunBuf();
		fsmRun->runBuf->next = 0;
		fsmRun->p = fsmRun->pe = fsmRun->runBuf->data;
	}

	fsmRun->p = fsmRun->pe = fsmRun->runBuf->data + fsmRun->runBuf->length;
}

void undoStreamPull( FsmRun *fsmRun, InputStream *inputStream, const char *data, long length )
{
	debug( REALM_PARSE, "undoing stream pull\n" );

	connect( fsmRun, inputStream );

	if ( fsmRun->p == fsmRun->pe && fsmRun->p == fsmRun->runBuf->data )
		sendBackRunBufHead( fsmRun, inputStream );

	assert( fsmRun->p - length >= fsmRun->runBuf->data );
	fsmRun->p -= length;
}

void streamPushText( InputStream *inputStream, const char *data, long length )
{
//	#ifdef COLM_LOG_PARSE
//	if ( colm_log_parse ) {
//		cerr << "readying fake push" << endl;
//	}
//	#endif

	takeBackBuffered( inputStream );
	inputStream->funcs->pushText( inputStream, data, length );
}

void streamPushTree( InputStream *inputStream, Tree *tree, int ignore )
{
//	#ifdef COLM_LOG_PARSE
//	if ( colm_log_parse ) {
//		cerr << "readying fake push" << endl;
//	}
//	#endif

	takeBackBuffered( inputStream );
	inputStream->funcs->pushTree( inputStream, tree, ignore );
}

void undoStreamPush( Program *prg, Tree **sp, InputStream *inputStream, long length )
{
	takeBackBuffered( inputStream );
	Tree *tree = inputStream->funcs->undoPush( inputStream, length );
	if ( tree != 0 )
		treeDownref( prg, sp, tree );
}

void undoStreamAppend( Program *prg, Tree **sp, InputStream *inputStream, long length )
{
	takeBackBuffered( inputStream );
	Tree *tree = inputStream->funcs->undoAppend( inputStream, length );
	if ( tree != 0 )
		treeDownref( prg, sp, tree );
}

/* Should only be sending back whole tokens/ignores, therefore the send back
 * should never cross a buffer boundary. Either we slide back data, or we move to
 * a previous buffer and slide back data. */
void sendBackText( FsmRun *fsmRun, InputStream *inputStream, const char *data, long length )
{
	connect( fsmRun, inputStream );

	debug( REALM_PARSE, "push back of %ld characters\n", length );

	if ( length == 0 )
		return;

	if ( fsmRun->p == fsmRun->runBuf->data )
		sendBackRunBufHead( fsmRun, inputStream );

	/* If there is data in the current buffer then send the whole send back
	 * should be in this buffer. */
	assert( (fsmRun->p - fsmRun->runBuf->data) >= length );

	/* slide data back. */
	fsmRun->p -= length;

	#if COLM_LOG
	if ( memcmp( data, fsmRun->p, length ) != 0 )
		debug( REALM_PARSE, "mismatch of pushed back text\n" );
	#endif

	assert( memcmp( data, fsmRun->p, length ) == 0 );
		
	undoPosition( inputStream, data, length );
}

void sendBackIgnore( Tree **sp, PdaRun *pdaRun, FsmRun *fsmRun, InputStream *inputStream, Kid *ignore )
{
	/* Ignore tokens are queued in reverse order. */
	while ( ignore != 0 ) {
		#ifdef COLM_LOG
		LangElInfo *lelInfo = pdaRun->tables->rtd->lelInfo;
		debug( REALM_PARSE, "sending back: %s%s\n",
			lelInfo[ignore->tree->id].name, 
			ignore->tree->flags & AF_ARTIFICIAL ? " (artificial)" : "" );
		#endif

		Head *head = ignore->tree->tokdata;
		int artificial = ignore->tree->flags & AF_ARTIFICIAL;

		if ( head != 0 && !artificial )
			sendBackText( fsmRun, inputStream, stringData( head ), head->length );

		decrementConsumed( pdaRun );

		/* Check for reverse code. */
		if ( ignore->tree->flags & AF_HAS_RCODE ) {
			Execution exec;
			initExecution( &exec, pdaRun->prg, &pdaRun->reverseCode, 
					pdaRun, fsmRun, 0, 0, 0, 0, 0 );

			/* Do the reverse exeuction. */
			rexecute( &exec, sp, pdaRun->allReverseCode );
			ignore->tree->flags &= ~AF_HAS_RCODE;
		}

		ignore = ignore->next;
	}
}

void sendBack( Tree **sp, PdaRun *pdaRun, FsmRun *fsmRun, InputStream *inputStream, Kid *input )
{
	#ifdef COLM_LOG
	LangElInfo *lelInfo = pdaRun->tables->rtd->lelInfo;
	debug( REALM_PARSE, "sending back: %s  text: %.*s%s\n", 
			lelInfo[input->tree->id].name, 
			stringLength( input->tree->tokdata ), 
			stringData( input->tree->tokdata ), 
			input->tree->flags & AF_ARTIFICIAL ? " (artificial)" : "" );
	#endif

	if ( input->tree->flags & AF_NAMED ) {
		/* Send back anything in the buffer that has not been parsed. */
		if ( fsmRun->p == fsmRun->runBuf->data )
			sendBackRunBufHead( fsmRun, inputStream );

		/* Send the named lang el back first, then send back any leading
		 * whitespace. */
		inputStream->funcs->pushBackNamed( inputStream );
	}

	decrementConsumed( pdaRun );

	if ( input->tree->flags & AF_ARTIFICIAL ) {
		treeUpref( input->tree );
		streamPushTree( inputStream, input->tree, false );

		/* FIXME: need to undo the merge of ignore tokens. */
		Kid *leftIgnore = 0;
		if ( input->tree->flags & AF_LEFT_IGNORE ) {
			leftIgnore = (Kid*)input->tree->child->tree;
			//input->tree->flags &= ~AF_LEFT_IGNORE;
			//input->tree->child = input->tree->child->next;
		}

		//if ( input->tree->flags & AF_RIGHT_IGNORE ) {
		//	cerr << "need to pull out ignore" << endl;
		//}

		sendBackIgnore( sp, pdaRun, fsmRun, inputStream, leftIgnore );

		///* Always push back the ignore text. */
		//sendBackIgnore( sp, pdaRun, fsmRun, inputStream, treeIgnore( fsmRun->prg, input->tree ) );
	}
	else {
		/* Push back the token data. */
		sendBackText( fsmRun, inputStream, stringData( input->tree->tokdata ), 
				stringLength( input->tree->tokdata ) );

		/* Check for reverse code. */
		if ( input->tree->flags & AF_HAS_RCODE ) {
			Execution exec;
			initExecution( &exec, pdaRun->prg, &pdaRun->reverseCode, 
					pdaRun, fsmRun, 0, 0, 0, 0, 0 );

			/* Do the reverse exeuction. */
			rexecute( &exec, sp, pdaRun->allReverseCode );
			input->tree->flags &= ~AF_HAS_RCODE;
		}

		/* Always push back the ignore text. */
		sendBackIgnore( sp, pdaRun, fsmRun, inputStream, treeIgnore( fsmRun->prg, input->tree ) );

		/* If eof was just sent back remember that it needs to be sent again. */
		if ( input->tree->id == pdaRun->tables->rtd->eofLelIds[pdaRun->parserId] )
			inputStream->eofSent = false;

		/* If the item is bound then store remove it from the bindings array. */
		unbind( fsmRun->prg, sp, pdaRun, input->tree );
	}

	if ( pdaRun->consumed == pdaRun->targetConsumed ) {
		#ifdef COLM_LOG_PARSE
		if ( colm_log_parse )
			cerr << "trigger parse stop, consumed = target = " << pdaRun->targetConsumed << endl;
		#endif
			
		pdaRun->stop = true;
	}

	/* Downref the tree that was sent back and free the kid. */
	treeDownref( fsmRun->prg, sp, input->tree );
	kidFree( fsmRun->prg, input );
}

/* This is the entry point for the perser to send back tokens. */
void queueBackTree( Tree **sp, PdaRun *pdaRun, FsmRun *fsmRun, InputStream *inputStream, Kid *input )
{
	/* If there are queued items send them back starting at the tail
	 * (newest). */
	if ( pdaRun->queue != 0 ) {
		/* Reverse the list. */
		Kid *kid = pdaRun->queue, *last = 0;
		while ( kid != 0 ) {
			Kid *next = kid->next;
			kid->next = last;
			last = kid;
			kid = next;
		}

		/* Send them back. */
		while ( last != 0 ) {
			Kid *next = last->next;
			sendBack( sp, pdaRun, fsmRun, inputStream, last );
			last = next;
		}

		pdaRun->queue = 0;
	}

	/* Now that the queue is flushed, can send back the original item. */
	sendBack( sp, pdaRun, fsmRun, inputStream, input );
}


/* If no token was generated but there is reverse code then we must generate
 * a fake token so we can attach the reverse code to it. */
void addNoToken( Program *prg, PdaRun *parser )
{
	/* Check if there was anything generated. */
	if ( parser->queue == 0 && parser->reverseCode.tabLen > 0 ) {
		debug( REALM_PARSE, "found reverse code but no token, sending _notoken\n" );

		Tree *tree = (Tree*)parseTreeAllocate( prg );
		tree->flags |= AF_PARSE_TREE;
		tree->refs = 1;
		tree->id = prg->rtd->noTokenId;
		tree->tokdata = 0;

		parser->queue = kidAllocate( prg );
		parser->queue->tree = tree;
		parser->queue->next = 0;
	}
}

void sendQueuedTokens( Tree **sp, PdaRun *pdaRun, FsmRun *fsmRun, InputStream *inputStream )
{
	LangElInfo *lelInfo = fsmRun->prg->rtd->lelInfo;

	while ( pdaRun->queue != 0 ) {
		/* Pull an item to send off the queue. */
		Kid *send = pdaRun->queue;
		pdaRun->queue = pdaRun->queue->next;

		/* Must clear next, since the parsing algorithm uses it. */
		send->next = 0;
		if ( lelInfo[send->tree->id].ignore ) {
			debug( REALM_PARSE, "ignoring queued item: %s\n",
					pdaRun->tables->rtd->lelInfo[send->tree->id].name );

			incrementConsumed( pdaRun );

			ignore( pdaRun, send->tree );
			kidFree( fsmRun->prg, send );
		}
		else {
			debug( REALM_PARSE, "sending queue item: %s\n",
					pdaRun->tables->rtd->lelInfo[send->tree->id].name );

			incrementConsumed( pdaRun );

			sendHandleError( sp, pdaRun, fsmRun, inputStream, send );
		}
	}
}

Kid *makeToken( PdaRun *pdaRun, FsmRun *fsmRun, InputStream *inputStream, int id,
		Head *tokdata, int namedLangEl, int bindId )
{
	/* Make the token object. */
	long objectLength = pdaRun->tables->rtd->lelInfo[id].objectLength;
	Kid *attrs = allocAttrs( fsmRun->prg, objectLength );

	Kid *input = 0;
	input = kidAllocate( fsmRun->prg );
	input->tree = (Tree*)parseTreeAllocate( fsmRun->prg );
	input->tree->flags |= AF_PARSE_TREE;

	if ( namedLangEl )
		input->tree->flags |= AF_NAMED;

	input->tree->refs = 1;
	input->tree->id = id;
	input->tree->tokdata = tokdata;

	/* No children and ignores get added later. */
	input->tree->child = attrs;

	LangElInfo *lelInfo = pdaRun->tables->rtd->lelInfo;
	if ( lelInfo[id].numCaptureAttr > 0 ) {
		int i;
		for ( i = 0; i < lelInfo[id].numCaptureAttr; i++ ) {
			CaptureAttr *ca = &pdaRun->tables->rtd->captureAttr[lelInfo[id].captureAttr + i];
			Head *data = stringAllocFull( fsmRun->prg, 
					fsmRun->mark[ca->mark_enter], fsmRun->mark[ca->mark_leave]
					- fsmRun->mark[ca->mark_enter] );
			Tree *string = constructString( fsmRun->prg, data );
			treeUpref( string );
			setAttr( input->tree, ca->offset, string );
		}
	}

	makeTokenPushBinding( pdaRun, bindId, input->tree );

	return input;
}

void executeGenerationAction( Tree **sp, Program *prg, FsmRun *fsmRun, PdaRun *pdaRun, 
		Code *code, long id, Head *tokdata )
{
	/* Execute the translation. */
	Execution exec;
	initExecution( &exec, prg, &pdaRun->reverseCode, pdaRun, fsmRun, code, 0, id, tokdata, fsmRun->mark );
	execute( &exec, sp );

	/* If there is revese code but nothing generated we need a noToken. */
	addNoToken( prg, pdaRun );

	/* If there is reverse code then addNoToken will guarantee that the
	 * queue is not empty. Pull the reverse code out and store in the
	 * token. */
	Tree *tree = pdaRun->queue->tree;
	int hasrcode = makeReverseCode( pdaRun->allReverseCode, &pdaRun->reverseCode );
	if ( hasrcode )
		tree->flags |= AF_HAS_RCODE;
}

/* 
 * Not supported:
 *  -invoke failure (the backtracker)
 */

void generationAction( Tree **sp, InputStream *inputStream, FsmRun *fsmRun, 
		PdaRun *pdaRun, int id, Head *tokdata, int namedLangEl, int bindId )
{
	#ifdef COLM_LOG_PARSE
	if ( colm_log_parse ) {
		cerr << "generation action: " << 
				pdaRun->tables->rtd->lelInfo[id].name << endl;
	}
	#endif

	/* Find the code. */
	Code *code = pdaRun->tables->rtd->frameInfo[
			pdaRun->tables->rtd->lelInfo[id].frameId].codeWV;

	/* Execute the action and process the queue. */
	executeGenerationAction( sp, fsmRun->prg, fsmRun, pdaRun, code, id, tokdata );

	/* Finished with the match text. */
	stringFree( fsmRun->prg, tokdata );

	/* Send the queued tokens. */
	sendQueuedTokens( sp, pdaRun, fsmRun, inputStream );
}

Kid *extractIgnore( PdaRun *pdaRun )
{
	Kid *ignore = pdaRun->accumIgnore;
	pdaRun->accumIgnore = 0;
	return ignore;
}

/* Send back the accumulated ignore tokens. */
void sendBackQueuedIgnore( Tree **sp, InputStream *inputStream, FsmRun *fsmRun, PdaRun *pdaRun )
{
	Kid *ignore = extractIgnore( pdaRun );
	sendBackIgnore( sp, pdaRun, fsmRun, inputStream, ignore );
	while ( ignore != 0 ) {
		Kid *next = ignore->next;
		treeDownref( pdaRun->prg, sp, ignore->tree );
		kidFree( pdaRun->prg, ignore );
		ignore = next;
	}
}

void clearIgnoreList( Program *prg, Tree **sp, Kid *kid )
{
	while ( kid != 0 ) {
		Kid *next = kid->next;
		treeDownref( prg, sp, kid->tree );
		kidFree( prg, kid );
		kid = next;
	}
}

void sendWithIgnore( Tree **sp, PdaRun *pdaRun, FsmRun *fsmRun, InputStream *inputStream, Kid *input )
{
	/* Need to preserve the layout under a tree:
	 *    attributes, ignore tokens, grammar children. */

	/* Pull the ignore tokens out and store in the token. */
	Kid *ignore = extractIgnore( pdaRun );
	if ( ignore != 0 ) {
		if ( input->tree->flags & AF_LEFT_IGNORE ) {
			/* FIXME: Leak here. */
			Kid *ignoreHead = input->tree->child;
			clearIgnoreList( pdaRun->prg, sp, (Kid*) ignoreHead->tree );
			ignoreHead->tree = (Tree*) ignore;
		}
		else {
			/* Insert an ignore head in the child list. */
			Kid *ignoreHead = kidAllocate( pdaRun->prg );
			ignoreHead->next = input->tree->child;
			input->tree->child = ignoreHead;

			ignoreHead->tree = (Tree*) ignore;
			input->tree->flags |= AF_LEFT_IGNORE;
		}
	}
	else {
		/* Need to remove any existing ignore data. */
		if ( input->tree->flags & AF_LEFT_IGNORE ) {
			/* FIXME: leak here. */
			Kid *ignoreHead = input->tree->child;
			clearIgnoreList( pdaRun->prg, sp, (Kid*)ignoreHead->tree );
			kidFree( pdaRun->prg, ignoreHead );

			input->tree->child = input->tree->child->next;
			input->tree->flags = input->tree->flags & ~AF_LEFT_IGNORE ;
		}
	}

	parseToken( sp, pdaRun, fsmRun, inputStream, input );
}


