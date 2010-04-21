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


