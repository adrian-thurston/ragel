/*
 *  Copyright 2010-2011 Adrian Thurston <thurston@complang.org>
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

#include <colm/fsmrun.h>
#include <colm/pdarun.h>
#include <colm/input.h>
#include <colm/debug.h>
#include <colm/tree.h>
#include <colm/bytecode.h>
#include <colm/pool.h>
#include <colm/program.h>

#include <string.h>
#include <assert.h>
#include <stdlib.h>

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

void clearFsmRun( Program *prg, FsmRun *fsmRun )
{
	if ( fsmRun->runBuf != 0 ) {
		/* Transfer the run buf list to the program */
		RunBuf *head = fsmRun->runBuf;
		RunBuf *tail = head;
		while ( tail->next != 0 )
			tail = tail->next;

		tail->next = prg->allocRunBuf;
		prg->allocRunBuf = head;
	}

	if ( fsmRun->haveDataOf != 0 ) {
		fsmRun->haveDataOf->hasData = 0;
		fsmRun->haveDataOf = 0;
	}
}

/* Keep the position up to date after consuming text. */
void updatePosition( InputStream *inputStream, const char *data, long length )
{
	if ( !inputStream->handlesLine ) {
		int i;
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

void connectStream( FsmRun *fsmRun, InputStream *inputStream )
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

		connectStream( fsmRun, inputStream );
			
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

	connectStream( fsmRun, inputStream );

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
	connectStream( fsmRun, inputStream );

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

//	#if COLM_LOG
//	if ( memcmp( data, fsmRun->p, length ) != 0 )
//		debug( REALM_PARSE, "mismatch of pushed back text\n" );
//	#endif

	assert( memcmp( data, fsmRun->p, length ) == 0 );
		
	undoPosition( inputStream, data, length );
}

/*
 * Stops on:
 *   PcrRevIgnore
 */
long sendBackIgnore( Program *prg, Tree **sp, PdaRun *pdaRun, FsmRun *fsmRun,
		InputStream *inputStream, Kid *ignoreKidList, long entry )
{
switch ( entry ) {
case PcrStart:
	pdaRun->ignore4 = ignoreKidList;

	while ( pdaRun->ignore4 != 0 ) {
		#ifdef COLM_LOG
		LangElInfo *lelInfo = prg->rtd->lelInfo;
		debug( REALM_PARSE, "sending back: %s%s\n",
			lelInfo[pdaRun->ignore4->tree->id].name, 
			pdaRun->ignore4->tree->flags & AF_ARTIFICIAL ? " (artificial)" : "" );
		#endif

		Head *head = pdaRun->ignore4->tree->tokdata;
		int artificial = pdaRun->ignore4->tree->flags & AF_ARTIFICIAL;

		if ( head != 0 && !artificial )
			sendBackText( fsmRun, inputStream, stringData( head ), head->length );

		decrementConsumed( pdaRun );

		/* Check for reverse code. */
		if ( pdaRun->ignore4->tree->flags & AF_HAS_RCODE ) {

return PcrRevIgnore;
case PcrRevIgnore:

			pdaRun->ignore4->tree->flags &= ~AF_HAS_RCODE;
		}

		pdaRun->ignore4 = pdaRun->ignore4->next;
	}

	Kid *ignore = ignoreKidList;
	while ( ignore != 0 ) {
		Kid *next = ignore->next;
		treeDownref( prg, sp, ignore->tree );
		kidFree( prg, ignore );
		ignore = next;
	}

case PcrDone:
break; }

	return PcrDone;
}

/* Stops on:
 *   PcrRevIgnore3
 *   PcrRevToken
 */

long sendBack( Program *prg, Tree **sp, PdaRun *pdaRun, FsmRun *fsmRun, InputStream *inputStream, Kid *input, long entry )
{
	#ifdef COLM_LOG
	LangElInfo *lelInfo = prg->rtd->lelInfo;
	debug( REALM_PARSE, "sending back: %s  text: %.*s%s\n", 
			lelInfo[input->tree->id].name, 
			stringLength( input->tree->tokdata ), 
			stringData( input->tree->tokdata ), 
			input->tree->flags & AF_ARTIFICIAL ? " (artificial)" : "" );
	#endif

switch ( entry ) {
case PcrStart:

	if ( input->tree->flags & AF_NAMED ) {
		/* Send back anything in the buffer that has not been parsed. */
		if ( fsmRun->p == fsmRun->runBuf->data )
			sendBackRunBufHead( fsmRun, inputStream );

		/* Send the named lang el back first, then send back any leading
		 * whitespace. */
		inputStream->funcs->pushBackNamed( inputStream );
	}

	decrementConsumed( pdaRun );

	/*
	 * Detach ignores.
	 */

	/* Detach right. */
	pdaRun->rightIgnore = 0;
	if ( pdaRun->tokenList != 0 && pdaRun->tokenList->kid->tree->flags & AF_RIGHT_IL_ATTACHED ) {
		Kid *riKid = treeRightIgnoreKid( prg, pdaRun->tokenList->kid->tree );

		/* If the right ignore has a left ignore, then that was the original
		 * right ignore. */
		Kid *li = treeLeftIgnoreKid( prg, riKid->tree );
		if ( li != 0 ) {
			treeUpref( li->tree );
			removeLeftIgnore( prg, sp, riKid->tree );
			pdaRun->rightIgnore = riKid->tree;
			treeUpref( pdaRun->rightIgnore );
			riKid->tree = li->tree;
		}
		else  {
			pdaRun->rightIgnore = riKid->tree;
			treeUpref( pdaRun->rightIgnore );
			removeRightIgnore( prg, sp, pdaRun->tokenList->kid->tree );
			pdaRun->tokenList->kid->tree->flags &= ~AF_RIGHT_IL_ATTACHED;
		}

	}

	/* Detach left. */
	pdaRun->leftIgnore = 0;
	if ( input->tree->flags & AF_LEFT_IL_ATTACHED ) {
		Kid *liKid = treeLeftIgnoreKid( prg, input->tree );

		/* If the left ignore has a right ignore, then that was the original
		 * left ignore. */
		Kid *ri = treeRightIgnoreKid( prg, liKid->tree );
		if ( ri != 0 ) {
			treeUpref( ri->tree );
			removeRightIgnore( prg, sp, liKid->tree );
			pdaRun->leftIgnore = liKid->tree;
			treeUpref( pdaRun->leftIgnore );
			liKid->tree = ri->tree;
		}
		else {
			pdaRun->leftIgnore = liKid->tree;
			treeUpref( pdaRun->leftIgnore );
			removeLeftIgnore( prg, sp, input->tree );
			input->tree->flags &= ~AF_LEFT_IL_ATTACHED;
		}

	}

	/* Artifical were not parsed, instead sent in as items. */
	if ( input->tree->flags & AF_ARTIFICIAL ) {
		treeUpref( input->tree );

		streamPushTree( inputStream, input->tree, false );
	}
	else {
		/* Push back the token data. */
		sendBackText( fsmRun, inputStream, stringData( input->tree->tokdata ), 
				stringLength( input->tree->tokdata ) );

		/* Check for reverse code. */
		if ( input->tree->flags & AF_HAS_RCODE ) {

return PcrRevToken;
case PcrRevToken:

			input->tree->flags &= ~AF_HAS_RCODE;
		}

		/* If eof was just sent back remember that it needs to be sent again. */
		if ( input->tree->id == prg->rtd->eofLelIds[pdaRun->parserId] )
			inputStream->eofSent = false;

		/* If the item is bound then store remove it from the bindings array. */
		unbind( prg, sp, pdaRun, input->tree );
	}

	if ( pdaRun->leftIgnore != 0 ) {
		pdaRun->ignore5 = reverseKidList( pdaRun->leftIgnore->child );
		pdaRun->leftIgnore->child = 0;

		long pcr = sendBackIgnore( prg, sp, pdaRun, fsmRun, inputStream, pdaRun->ignore5, PcrStart );
		while ( pcr != PcrDone ) {

return PcrRevIgnore3;
case PcrRevIgnore3:

			pcr = sendBackIgnore( prg, sp, pdaRun, fsmRun, inputStream, pdaRun->ignore5, PcrRevIgnore );
		}
	}

	if ( pdaRun->consumed == pdaRun->targetConsumed ) {
		debug( REALM_PARSE, "trigger parse stop, consumed = target = %d", pdaRun->targetConsumed );
		pdaRun->stop = true;
	}

	/* Downref the tree that was sent back and free the kid. */
	treeDownref( prg, sp, input->tree );
	treeDownref( prg, sp, pdaRun->rightIgnore );
	treeDownref( prg, sp, pdaRun->leftIgnore );
	kidFree( prg, input );

case PcrDone:
break; }

	return PcrDone;
}

void ignoreTree( Program *prg, PdaRun *pdaRun, Tree *tree )
{
	/* Add the ignore string to the head of the ignore list. */
	Kid *ignore = kidAllocate( prg );
	ignore->tree = tree;

	/* Prepend it to the list of ignore tokens. */
	ignore->next = pdaRun->accumIgnore;
	pdaRun->accumIgnore = ignore;
}

Kid *makeToken( Program *prg, PdaRun *pdaRun, FsmRun *fsmRun, InputStream *inputStream, int id,
		Head *tokdata, int namedLangEl, int bindId )
{
	/* Make the token object. */
	long objectLength = prg->rtd->lelInfo[id].objectLength;
	Kid *attrs = allocAttrs( prg, objectLength );

	Kid *input = 0;
	input = kidAllocate( prg );
	input->tree = (Tree*)parseTreeAllocate( prg );
	debug( REALM_PARSE, "made token %p\n", input->tree );
	input->tree->flags |= AF_PARSE_TREE;

	if ( namedLangEl )
		input->tree->flags |= AF_NAMED;

	input->tree->refs = 1;
	input->tree->id = id;
	input->tree->tokdata = tokdata;

	/* No children and ignores get added later. */
	input->tree->child = attrs;

	LangElInfo *lelInfo = prg->rtd->lelInfo;
	if ( lelInfo[id].numCaptureAttr > 0 ) {
		int i;
		for ( i = 0; i < lelInfo[id].numCaptureAttr; i++ ) {
			CaptureAttr *ca = &prg->rtd->captureAttr[lelInfo[id].captureAttr + i];
			Head *data = stringAllocFull( prg, 
					fsmRun->mark[ca->mark_enter], fsmRun->mark[ca->mark_leave]
					- fsmRun->mark[ca->mark_enter] );
			Tree *string = constructString( prg, data );
			treeUpref( string );
			setAttr( input->tree, ca->offset, string );
		}
	}

	makeTokenPushBinding( pdaRun, bindId, input->tree );

	return input;
}

void addNoToken( Program *prg, Tree **sp, FsmRun *fsmRun, PdaRun *pdaRun, 
		InputStream *inputStream, int frameId, long id, Head *tokdata )
{
	/* Check if there was anything generated. */
	if ( pdaRun->rcodeCollect.tabLen > 0 ) {
		debug( REALM_PARSE, "found reverse code but no token, sending _notoken\n" );

		Tree *tree = (Tree*)parseTreeAllocate( prg );
		tree->flags |= AF_PARSE_TREE;
		tree->refs = 1;
		tree->id = prg->rtd->noTokenId;
		tree->tokdata = 0;

		Kid *send = kidAllocate( prg );
		send->tree = tree;
		send->next = 0;

		/* If there is reverse code then addNoToken will guarantee that the
		 * queue is not empty. Pull the reverse code out and store in the
		 * token. */
		int hasrcode = makeReverseCode( &pdaRun->reverseCode, &pdaRun->rcodeCollect );
		if ( hasrcode )
			tree->flags |= AF_HAS_RCODE;

		incrementConsumed( pdaRun );

		ignoreTree( prg, pdaRun, send->tree );
		kidFree( prg, send );
	}
}


Kid *extractIgnore( PdaRun *pdaRun )
{
	Kid *ignore = pdaRun->accumIgnore;
	pdaRun->accumIgnore = 0;
	return ignore;
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

static void reportParseError( Program *prg, Tree **sp, PdaRun *pdaRun )
{
	Kid *kid = pdaRun->btPoint;
	Head *deepest = 0;
	while ( kid != 0 ) {
		Head *head = kid->tree->tokdata;
		if ( head != 0 && head->location != 0 ) {
			if ( deepest == 0 || head->location->byte > deepest->location->byte ) 
				deepest = head;
		}
		kid = kid->next;
	}

	Head *errorHead = 0;

	/* If there are no error points on record assume the error occurred at the beginning of the stream. */
	if ( deepest == 0 ) 
		errorHead = stringAllocFull( prg, "PARSE ERROR at 1:1", 18 );
	else {
		debug( REALM_PARSE, "deepest location byte: %d\n", deepest->location->byte );

		long line = deepest->location->line;
		long i, column = deepest->location->column;

		for ( i = 0; i < deepest->length; i++ ) {
			if ( deepest->data[i] != '\n' )
				column += 1;
			else {
				line += 1;
				column = 1;
			}
		}

		char formatted[128];
		sprintf( formatted, "PARSE ERROR at %ld:%ld", line, column );
		errorHead = stringAllocFull( prg, formatted, strlen(formatted) );
	}

	Tree *tree = constructString( prg, errorHead );
	treeDownref( prg, sp, prg->lastParseError );
	prg->lastParseError = tree;
	treeUpref( prg->lastParseError );
}

void attachIgnore( Program *prg, Tree **sp, PdaRun *pdaRun, Kid *input )
{
	/* Need to preserve the layout under a tree:
	 *    attributes, ignore tokens, grammar children. */

	/* Reset. */
	input->tree->flags &= ~AF_LEFT_IL_ATTACHED;
	input->tree->flags &= ~AF_RIGHT_IL_ATTACHED;

	/* Pull the ignore tokens out and store in the token. */
	Kid *ignoreKid = extractIgnore( pdaRun );
	
	if ( ignoreKid != 0 ) {
		ignoreKid = reverseKidList( ignoreKid );

		/* Copy the ignore list first if we need to attach it as a right
		 * ignore. */
		IgnoreList *rightIgnore = 0;
		if ( pdaRun->tokenList != 0 ) {
			rightIgnore = ilAllocate( prg );
			rightIgnore->id = LEL_ID_IGNORE_LIST;
			rightIgnore->child = copyKidList( prg, ignoreKid );
			rightIgnore->generation = prg->nextIlGen++;
		}

		/* Make the ignore list for the left-ignore. */
		IgnoreList *leftIgnore = ilAllocate( prg );
		leftIgnore->id = LEL_ID_IGNORE_LIST;
		leftIgnore->child = ignoreKid;
		leftIgnore->generation = prg->nextIlGen++;

		/* Attach as left ignore to the token we are sending. */
		if ( input->tree->flags & AF_LEFT_IGNORE ) {
			/* The token already has a left-ignore. Merge by attaching it as a
			 * right ignore of the new list. */
			Kid *curIgnore = treeLeftIgnoreKid( prg, input->tree );
			attachRightIgnore( prg, (Tree*)leftIgnore, (IgnoreList*)curIgnore->tree );

			/* Replace the current ignore. */
			treeDownref( prg, sp, curIgnore->tree );
			curIgnore->tree = (Tree*)leftIgnore;
			treeUpref( (Tree*)leftIgnore );
		}
		else {
			/* Attach the ignore list. */
			attachLeftIgnore( prg, input->tree, leftIgnore );
		}
		input->tree->flags |= AF_LEFT_IL_ATTACHED;

		if ( pdaRun->tokenList != 0 ) {
			if ( pdaRun->tokenList->kid->tree->flags & AF_RIGHT_IGNORE ) {
				/* The previous token already has a right ignore. Merge by
				 * attaching it as a left ignore of the new list. */
				Kid *curIgnore = treeRightIgnoreKid( prg, pdaRun->tokenList->kid->tree );
				attachLeftIgnore( prg, (Tree*)rightIgnore, (IgnoreList*)curIgnore->tree );

				/* Replace the current ignore. */
				treeDownref( prg, sp, curIgnore->tree );
				curIgnore->tree = (Tree*)rightIgnore;
				treeUpref( (Tree*)rightIgnore );
			}
			else {
				/* Attach The ignore list. */
				attachRightIgnore( prg, pdaRun->tokenList->kid->tree, rightIgnore );
			}

			pdaRun->tokenList->kid->tree->flags |= AF_RIGHT_IL_ATTACHED;
		}
	}
	else {
		/* We need to do this only when it's possible that the token has come
		 * in with an ignore list. */
		input->flags |= KF_SUPPRESS_RIGHT;
	
		if ( pdaRun->tokenList != 0 ) {
			pdaRun->tokenList->kid->flags |= KF_SUPPRESS_LEFT;
		}
	}
}

void handleError( Program *prg, Tree **sp, PdaRun *pdaRun )
{
	/* Check the result. */
	if ( pdaRun->parseError ) {
		/* Error occured in the top-level parser. */
		reportParseError( prg, sp, pdaRun );
	}
	else {
		if ( isParserStopFinished( pdaRun ) ) {
			debug( REALM_PARSE, "stopping the parse\n" );
			pdaRun->stopParsing = true;
		}
	}
}

void sendIgnore( Program *prg, Tree **sp, InputStream *inputStream, FsmRun *fsmRun, PdaRun *pdaRun, long id )
{
	debug( REALM_PARSE, "ignoring: %s\n", prg->rtd->lelInfo[id].name );

	/* Make the ignore string. */
	Head *ignoreStr = extractMatch( prg, fsmRun, inputStream );
	updatePosition( inputStream, fsmRun->tokstart, ignoreStr->length );
	
	Tree *tree = treeAllocate( prg );
	tree->refs = 1;
	tree->id = id;
	tree->tokdata = ignoreStr;

	incrementConsumed( pdaRun );

	/* Send it to the pdaRun. */
	ignoreTree( prg, pdaRun, tree );
}

Head *extractMatch( Program *prg, FsmRun *fsmRun, InputStream *inputStream )
{
	long length = fsmRun->p - fsmRun->tokstart;
	Head *head = stringAllocPointer( prg, fsmRun->tokstart, length );
	head->location = locationAllocate( prg );
	head->location->line = inputStream->line;
	head->location->column = inputStream->column;
	head->location->byte = inputStream->byte;

	debug( REALM_PARSE, "location byte: %d\n", inputStream->byte );
	return head;
}

Kid *sendToken( Program *prg, Tree **sp, InputStream *inputStream, FsmRun *fsmRun, PdaRun *pdaRun, long id )
{
	/* Make the token data. */
	Head *tokdata = extractMatch( prg, fsmRun, inputStream );

	debug( REALM_PARSE, "token: %s  text: %.*s\n",
		prg->rtd->lelInfo[id].name,
		stringLength(tokdata), stringData(tokdata) );

	updatePosition( inputStream, fsmRun->tokstart, tokdata->length );

	Kid *input = makeToken( prg, pdaRun, fsmRun, inputStream, id, tokdata, false, 0 );

	incrementConsumed( pdaRun );

	return input;
}

Kid *sendTree( Program *prg, Tree **sp, PdaRun *pdaRun, FsmRun *fsmRun, InputStream *inputStream )
{
	Kid *input = kidAllocate( prg );
	input->tree = inputStream->funcs->getTree( inputStream );

	incrementConsumed( pdaRun );

	return input;

}

Kid *sendEof( Program *prg, Tree **sp, InputStream *inputStream, FsmRun *fsmRun, PdaRun *pdaRun )
{
	debug( REALM_PARSE, "token: _EOF\n" );

	incrementConsumed( pdaRun );

	Head *head = headAllocate( prg );
	head->location = locationAllocate( prg );
	head->location->line = inputStream->line;
	head->location->column = inputStream->column;
	head->location->byte = inputStream->byte;

	Kid *input = kidAllocate( prg );
	input->tree = (Tree*)parseTreeAllocate( prg );
	input->tree->flags |= AF_PARSE_TREE;

	input->tree->refs = 1;
	input->tree->id = prg->rtd->eofLelIds[pdaRun->parserId];
	input->tree->tokdata = head;

	/* Set the state using the state of the parser. */
	fsmRun->region = pdaRunGetNextRegion( pdaRun, 0 );
	fsmRun->cs = fsmRun->tables->entryByRegion[fsmRun->region];

	return input;
}

void preEofBlock( Program *prg, Tree **sp, InputStream *inputStream, FsmRun *fsmRun, PdaRun *pdaRun, int tokenId )
{
}

void newToken( Program *prg, PdaRun *pdaRun, FsmRun *fsmRun )
{
	/* Init the scanner vars. */
	fsmRun->act = 0;
	fsmRun->tokstart = 0;
	fsmRun->tokend = 0;
	fsmRun->matchedToken = 0;
	fsmRun->tokstart = 0;

	/* Set the state using the state of the parser. */
	fsmRun->region = pdaRunGetNextRegion( pdaRun, 0 );
	fsmRun->cs = fsmRun->tables->entryByRegion[fsmRun->region];

	debug( REALM_PARSE, "scanning using token region: %s\n",
			prg->rtd->regionInfo[fsmRun->region].name );

	/* Clear the mark array. */
	memset( fsmRun->mark, 0, sizeof(fsmRun->mark) );
}

void breakRunBuf( FsmRun *fsmRun )
{
	/* We break the runbuf on named language elements and trees. This makes
	 * backtracking simpler because it allows us to always push back whole
	 * runBufs only. If we did not do this we could get half a runBuf, a named
	 * langEl, then the second half full. During backtracking we would need to
	 * push the halves back separately. */
	if ( fsmRun->p > fsmRun->runBuf->data ) {
		debug( REALM_PARSE, "have a langEl, making a new runBuf\n" );

		/* Compute the length of the current before before we move
		 * past it. */
		fsmRun->runBuf->length = fsmRun->p - fsmRun->runBuf->data;;

		/* Make the new one. */
		RunBuf *newBuf = newRunBuf();
		fsmRun->p = fsmRun->pe = newBuf->data;
		newBuf->next = fsmRun->runBuf;
		fsmRun->runBuf = newBuf;
	}
}

#define SCAN_UNDO              -7
#define SCAN_IGNORE            -6
#define SCAN_TREE              -5
#define SCAN_TRY_AGAIN_LATER   -4
#define SCAN_ERROR             -3
#define SCAN_LANG_EL           -2
#define SCAN_EOF               -1

long scanToken( Program *prg, PdaRun *pdaRun, FsmRun *fsmRun, InputStream *inputStream )
{
	if ( pdaRun->triggerUndo )
		return SCAN_UNDO;

	while ( true ) {
		if ( inputStream->funcs->needFlush( inputStream ) )
			fsmRun->peof = fsmRun->pe;

		fsmExecute( fsmRun, inputStream );

		/* First check if scanning stopped because we have a token. */
		if ( fsmRun->matchedToken > 0 ) {
			/* If the token has a marker indicating the end (due to trailing
			 * context) then adjust data now. */
			LangElInfo *lelInfo = prg->rtd->lelInfo;
			if ( lelInfo[fsmRun->matchedToken].markId >= 0 )
				fsmRun->p = fsmRun->mark[lelInfo[fsmRun->matchedToken].markId];

			return fsmRun->matchedToken;
		}

		/* Check for error. */
		if ( fsmRun->cs == fsmRun->tables->errorState ) {
			/* If a token was started, but not finished (tokstart != 0) then
			 * restore data to the beginning of that token. */
			if ( fsmRun->tokstart != 0 )
				fsmRun->p = fsmRun->tokstart;

			/* Check for a default token in the region. If one is there
			 * then send it and continue with the processing loop. */
			if ( prg->rtd->regionInfo[fsmRun->region].defaultToken >= 0 ) {
				fsmRun->tokstart = fsmRun->tokend = fsmRun->p;
				return prg->rtd->regionInfo[fsmRun->region].defaultToken;
			}

			return SCAN_ERROR;
		}

		/* Got here because the state machine didn't match a token or
		 * encounter an error. Must be because we got to the end of the buffer
		 * data. */
		assert( fsmRun->p == fsmRun->pe );

		/* Check for a named language element or constructed trees. Note that
		 * we can do this only when data == de otherwise we get ahead of what's
		 * already in the buffer. */
		if ( inputStream->funcs->isLangEl( inputStream ) ) {
			breakRunBuf( fsmRun );
			return SCAN_LANG_EL;
		}
		else if ( inputStream->funcs->isTree( inputStream ) ) {
			breakRunBuf( fsmRun );
			return SCAN_TREE;
		}
		else if ( inputStream->funcs->isIgnore( inputStream ) ) {
			breakRunBuf( fsmRun );
			return SCAN_IGNORE;
		}

		/* Maybe need eof. */
 		if ( inputStream->funcs->isEof( inputStream ) ) {
			if ( fsmRun->tokstart != 0 ) {
				/* If a token has been started, but not finshed 
				 * this is an error. */
				fsmRun->cs = fsmRun->tables->errorState;
				return SCAN_ERROR;
			}
			else {
				return SCAN_EOF;
			}
		}

		/* Maybe need to pause parsing until more data is inserted into the
		 * input inputStream. */
		if ( inputStream->funcs->tryAgainLater( inputStream ) )
			return SCAN_TRY_AGAIN_LATER;

		/* There may be space left in the current buffer. If not then we need
		 * to make some. */
		long space = fsmRun->runBuf->data + FSM_BUFSIZE - fsmRun->pe;
		if ( space == 0 ) {
			/* Create a new run buf. */
			RunBuf *newBuf = newRunBuf();

			/* If partway through a token then preserve the prefix. */
			long have = 0;

			if ( fsmRun->tokstart == 0 ) {
				/* No prefix. We filled the previous buffer. */
				fsmRun->runBuf->length = FSM_BUFSIZE;
			}
			else {
				int i;
				if ( fsmRun->tokstart == fsmRun->runBuf->data ) {
					/* A token is started and it is already at the beginning
					 * of the current buffer. This means buffer is full and it
					 * must be grown. Probably need to do this sooner. */
					fatal( "OUT OF BUFFER SPACE\n" );
				}

				/* There is data that needs to be shifted over. */
				have = fsmRun->pe - fsmRun->tokstart;
				memcpy( newBuf->data, fsmRun->tokstart, have );

				/* Compute the length of the previous buffer. */
				fsmRun->runBuf->length = FSM_BUFSIZE - have;

				/* Compute tokstart and tokend. */
				long dist = fsmRun->tokstart - newBuf->data;

				fsmRun->tokend -= dist;
				fsmRun->tokstart = newBuf->data;

				/* Shift any markers. */
				for ( i = 0; i < MARK_SLOTS; i++ ) {
					if ( fsmRun->mark[i] != 0 )
						fsmRun->mark[i] -= dist;
				}
			}

			fsmRun->p = fsmRun->pe = newBuf->data + have;
			fsmRun->peof = 0;

			newBuf->next = fsmRun->runBuf;
			fsmRun->runBuf = newBuf;
		}

		/* We don't have any data. What is next in the input inputStream? */
		space = fsmRun->runBuf->data + FSM_BUFSIZE - fsmRun->pe;
		assert( space > 0 );
			
		connectStream( fsmRun, inputStream );

		/* Get more data. */
		int len = inputStream->funcs->getData( inputStream, fsmRun->p, space );
		fsmRun->pe = fsmRun->p + len;
	}

	/* Should not be reached. */
	return SCAN_ERROR;
}


void sendTreeIgnore( Program *prg, Tree **sp, PdaRun *pdaRun, FsmRun *fsmRun, InputStream *inputStream )
{
	RunBuf *runBuf = inputStreamPopHead( inputStream );

	/* FIXME: using runbufs here for this is a poor use of memory. */
	Tree *tree = runBuf->tree;
	free( runBuf );

	incrementConsumed( pdaRun );

	ignoreTree( prg, pdaRun, tree );
}

/*
 * Stops on:
 *   PcrPreEof
 *   PcrRevIgnore1
 *   PcrREvIgnore2
 *   PcrGeneration
 *   PcrReduction
 *   PcrRevReduction
 *   PcrRevIgnore3
 *   PcrRevToken
 */

long parseLoop( Program *prg, Tree **sp, PdaRun *pdaRun, 
		FsmRun *fsmRun, InputStream *inputStream, long entry )
{
	LangElInfo *lelInfo = prg->rtd->lelInfo;

switch ( entry ) {
case PcrStart:

	pdaRun->stop = false;

	while ( true ) {
		/* Pull the current scanner from the parser. This can change during
		 * parsing due to inputStream pushes, usually for the purpose of includes.
		 * */
		pdaRun->tokenId = scanToken( prg, pdaRun, fsmRun, inputStream );

		if ( pdaRun->tokenId == SCAN_TRY_AGAIN_LATER )
			break;

		pdaRun->input2 = 0;

		/* Check for EOF. */
		if ( pdaRun->tokenId == SCAN_EOF ) {
			inputStream->eofSent = true;
			pdaRun->input2 = sendEof( prg, sp, inputStream, fsmRun, pdaRun );

			pdaRun->frameId = prg->rtd->regionInfo[fsmRun->region].eofFrameId;
			if ( prg->ctxDepParsing && pdaRun->frameId >= 0 ) {
				debug( REALM_PARSE, "HAVE PRE_EOF BLOCK\n" );

return PcrPreEof;
case PcrPreEof:

				/* 
				 * Need a no-token.
				 */
				addNoToken( prg, sp, fsmRun, pdaRun, inputStream, pdaRun->frameId, pdaRun->tokenId, 0 );
			}
		}
		else if ( pdaRun->tokenId == SCAN_UNDO ) {
			/* Fall through with input2 = 0. FIXME: Do we need to send back ignore? */
			debug( REALM_PARSE, "invoking undo from the scanner\n" );
		}
		else if ( pdaRun->tokenId == SCAN_ERROR ) {
			/* Scanner error, maybe retry. */
			if ( pdaRunGetNextRegion( pdaRun, 1 ) != 0 ) {
				debug( REALM_PARSE, "scanner failed, trying next region\n" );

				/* May have accumulated ignore tokens from a previous region.
				 * need to rescan them since we won't be sending tokens from
				 * this region. */
				pdaRun->ignore2 = extractIgnore( pdaRun );
				long pcr = sendBackIgnore( prg, sp, pdaRun, fsmRun, inputStream, pdaRun->ignore2, PcrStart );
				while ( pcr != PcrDone ) {

return PcrRevIgnore1;
case PcrRevIgnore1:

					pcr = sendBackIgnore( prg, sp, pdaRun, fsmRun, inputStream, pdaRun->ignore2, PcrRevIgnore );
				}

				pdaRun->nextRegionInd += 1;
				goto skipSend;
			}
			else if ( pdaRun->numRetry > 0 ) {
				debug( REALM_PARSE, "invoking parse error from the scanner\n" );

				/* Fall through to send null (error). */
				pushBtPoint( prg, pdaRun, 0 );

				/* Send back any accumulated ignore tokens, then trigger error
				 * in the the parser. */
				pdaRun->ignore3 = extractIgnore( pdaRun );
				long pcr = sendBackIgnore( prg, sp, pdaRun, fsmRun, inputStream, pdaRun->ignore3, PcrStart );
				while ( pcr != PcrDone ) {

return PcrRevIgnore2;
case PcrRevIgnore2:

					pcr = sendBackIgnore( prg, sp, pdaRun, fsmRun, inputStream, pdaRun->ignore3, PcrRevIgnore );
				}
			}
			else {
				debug( REALM_PARSE, "no alternate scanning regions\n" );
				/* There are no alternative scanning regions to try, nor are
				 * there any alternatives stored in the current parse tree. No
				 * choice but to end the parse. */
				pushBtPoint( prg, pdaRun, 0 );

				reportParseError( prg, sp, pdaRun );
				pdaRun->parseError = 1;
				goto skipSend;
			}
		}
		else if ( pdaRun->tokenId == SCAN_LANG_EL ) {
			debug( REALM_PARSE, "sending an named lang el\n" );

			/* A named language element (parsing colm program). */
			pdaRun->input2 = sendNamedLangEl( prg, sp, pdaRun, fsmRun, inputStream );
		}
		else if ( pdaRun->tokenId == SCAN_TREE ) {
			debug( REALM_PARSE, "sending an tree\n" );

			/* A tree already built. */
			pdaRun->input2 = sendTree( prg, sp, pdaRun, fsmRun, inputStream );
		}
		else if ( pdaRun->tokenId == SCAN_IGNORE ) {
			debug( REALM_PARSE, "sending an ignore token\n" );

			/* A tree to ignore. */
			sendTreeIgnore( prg, sp, pdaRun, fsmRun, inputStream );
			goto skipSend;
		}
		else if ( prg->ctxDepParsing && lelInfo[pdaRun->tokenId].frameId >= 0 ) {
			/* Has a generation action. */
			debug( REALM_PARSE, "token gen action: %s\n", 
					prg->rtd->lelInfo[pdaRun->tokenId].name );

			/* Make the token data. */
			pdaRun->tokdata = extractMatch( prg, fsmRun, inputStream );

			/* Note that we don't update the position now. It is done when the token
			 * data is pulled from the inputStream. */

			fsmRun->p = fsmRun->tokstart;
			fsmRun->tokstart = 0;

			pdaRun->fi = &prg->rtd->frameInfo[prg->rtd->lelInfo[pdaRun->tokenId].frameId];
			
return PcrGeneration;
case PcrGeneration:

			/* 
			 * Need a no-token.
			 */
			addNoToken( prg, sp, fsmRun, pdaRun, inputStream, 
					prg->rtd->lelInfo[pdaRun->tokenId].frameId, pdaRun->tokenId, pdaRun->tokdata );

			/* Finished with the match text. */
			stringFree( prg, pdaRun->tokdata );

			goto skipSend;
		}
		else if ( lelInfo[pdaRun->tokenId].ignore ) {
			debug( REALM_PARSE, "sending an ignore token: %s\n", 
					prg->rtd->lelInfo[pdaRun->tokenId].name );

			/* Is an ignore token. */
			sendIgnore( prg, sp, inputStream, fsmRun, pdaRun, pdaRun->tokenId );
			goto skipSend;
		}
		else {
			debug( REALM_PARSE, "sending an a plain old token: %s\n", 
					prg->rtd->lelInfo[pdaRun->tokenId].name );

			/* Is a plain token. */
			pdaRun->input2 = sendToken( prg, sp, inputStream, fsmRun, pdaRun, pdaRun->tokenId );
		}

		if ( pdaRun->input2 != 0 ) {
			/* Send the token to the parser. */
			attachIgnore( prg, sp, pdaRun, pdaRun->input2 );
		}

		assert( pdaRun->input1 == 0 );
		pdaRun->input1 = pdaRun->input2;
		pdaRun->input2 = 0;

		long pcr = parseToken( prg, sp, pdaRun, fsmRun, inputStream, PcrStart );
		
		while ( pcr != PcrDone ) {
return pcr;
case PcrReduction:
case PcrRevReduction:
case PcrRevIgnore3:
case PcrRevToken:

			pcr = parseToken( prg, sp, pdaRun, fsmRun, inputStream, entry );
		}

		assert( pcr == PcrDone );

		handleError( prg, sp, pdaRun );

skipSend:
		newToken( prg, pdaRun, fsmRun );

		/* Various stop conditions. This should all be coverned by one test
		 * eventually. */

		if ( pdaRun->triggerUndo ) {
			debug( REALM_PARSE, "parsing stopped by triggerUndo\n" );
			break;
		}

		if ( inputStream->eofSent ) {
			debug( REALM_PARSE, "parsing stopped by EOF\n" );
			break;
		}

		if ( pdaRun->stopParsing ) {
			debug( REALM_PARSE, "scanner has been stopped\n" );
			break;
		}

		if ( pdaRun->stop ) {
			debug( REALM_PARSE, "parsing has been stopped by consumedCount\n" );
			break;
		}

		if ( prg->induceExit ) {
			debug( REALM_PARSE, "parsing has been stopped by a call to exit\n" );
			break;
		}

		if ( pdaRun->parseError ) {
			debug( REALM_PARSE, "parsing stopped by a parse error\n" );
			break;
		}
	}

case PcrDone:
break; }

	return PcrDone;
}
