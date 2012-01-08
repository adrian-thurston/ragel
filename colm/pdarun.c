/*
 *  Copyright 2007-2011 Adrian Thurston <thurston@complang.org>
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

#include "config.h"
#include "debug.h"
#include "pdarun.h"
#include "fsmrun.h"
#include "bytecode.h"
#include "tree.h"
#include "pool.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#define true 1
#define false 0

#define act_sb 0x1
#define act_rb 0x2
#define lower 0x0000ffff
#define upper 0xffff0000
#define pt(var) ((ParseTree*)(var))

#define read_word_p( i, p ) do { \
	i = ((Word)  p[0]); \
	i |= ((Word) p[1]) << 8; \
	i |= ((Word) p[2]) << 16; \
	i |= ((Word) p[3]) << 24; \
} while(0)

#define read_tree_p( i, p ) do { \
	Word w; \
	w = ((Word)  p[0]); \
	w |= ((Word) p[1]) << 8; \
	w |= ((Word) p[2]) << 16; \
	w |= ((Word) p[3]) << 24; \
	i = (Tree*)w; \
} while(0)

void initFsmRun( FsmRun *fsmRun, Program *prg )
{
	fsmRun->tables = prg->rtd->fsmTables;
	fsmRun->runBuf = 0;

	/* Run buffers need to stick around because 
	 * token strings point into them. */
	fsmRun->runBuf = newRunBuf();
	fsmRun->runBuf->next = 0;

	fsmRun->p = fsmRun->pe = fsmRun->runBuf->data;
	fsmRun->peof = 0;

	fsmRun->attached1 = 0;
	fsmRun->attached2 = 0;
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

void incrementSteps( PdaRun *pdaRun )
{
	pdaRun->steps += 1;
	debug( REALM_PARSE, "steps up to %ld\n", pdaRun->steps );
}

void decrementSteps( PdaRun *pdaRun )
{
	pdaRun->steps -= 1;
	debug( REALM_PARSE, "steps down to %ld\n", pdaRun->steps );
}

/* Load up a token, starting from tokstart if it is set. If not set then
 * start it at data. */
Head *streamPull( Program *prg, FsmRun *fsmRun, InputStream *inputStream, long length )
{
	/* We should not be in the midst of getting a token. */
	assert( fsmRun->tokstart == 0 );

	RunBuf *runBuf = newRunBuf();
	runBuf->next = fsmRun->runBuf;
	fsmRun->runBuf = runBuf;

	int len = 0;
	getData( fsmRun, inputStream, 0, runBuf->data, length, &len );
	consumeData( inputStream, length );
	fsmRun->p = fsmRun->pe = runBuf->data + length;

	Head *tokdata = stringAllocPointer( prg, runBuf->data, length );
	updatePosition( inputStream, runBuf->data, length );

	return tokdata;
}

void undoStreamPull( FsmRun *fsmRun, InputStream *inputStream, const char *data, long length )
{
	debug( REALM_PARSE, "undoing stream pull\n" );

	prependData( inputStream, data, length );
}

void streamPushText( FsmRun *fsmRun, InputStream *inputStream, const char *data, long length )
{
	prependData( inputStream, data, length );
}

void streamPushTree( FsmRun *fsmRun, InputStream *inputStream, Tree *tree, int ignore )
{
	prependTree( inputStream, tree, ignore );
}

void undoStreamPush( Program *prg, Tree **sp, FsmRun *fsmRun, InputStream *inputStream, long length )
{
	if ( length < 0 ) 
		undoPrependTree( inputStream );
	else
		undoPrependData( inputStream, length );
}

void undoStreamAppend( Program *prg, Tree **sp, FsmRun *fsmRun, InputStream *inputStream, Tree *input, long length )
{
	if ( input->id == LEL_ID_STR )
		undoAppendData( inputStream, length );
	else if ( input->id == LEL_ID_STREAM )
		undoAppendStream( inputStream );
	else
		undoAppendTree( inputStream );
}

/* Should only be sending back whole tokens/ignores, therefore the send back
 * should never cross a buffer boundary. Either we slide back data, or we move to
 * a previous buffer and slide back data. */
static void sendBackText( FsmRun *fsmRun, InputStream *inputStream, const char *data, long length )
{
	debug( REALM_PARSE, "push back of %ld characters\n", length );

	if ( length == 0 )
		return;

	undoConsumeData( fsmRun, inputStream, data, length );
	undoPosition( inputStream, data, length );
}

/*
 * Stops on:
 *   PcrRevIgnore
 */
static void sendBackIgnore( Program *prg, Tree **sp, PdaRun *pdaRun, FsmRun *fsmRun,
		InputStream *inputStream, Kid *ignoreKidList )
{
	pdaRun->ignore4 = ignoreKidList;

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

	decrementSteps( pdaRun );

	/* Check for reverse code. */
	if ( pdaRun->ignore4->tree->flags & AF_HAS_RCODE ) {
		pdaRun->onDeck = true;
		pdaRun->ignore4->tree->flags &= ~AF_HAS_RCODE;
	}

	if ( pdaRun->steps == pdaRun->targetSteps ) {
		debug( REALM_PARSE, "trigger parse stop, steps = target = %d\n", pdaRun->targetSteps );
		pdaRun->stop = true;
	}

	treeDownref( prg, sp, pdaRun->ignore4->tree );
	kidFree( prg, pdaRun->ignore4 );
}

void detachIgnores( Program *prg, Tree **sp, PdaRun *pdaRun, FsmRun *fsmRun, Kid *input )
{
	assert( pdaRun->accumIgnore == 0 );

	/* Right ignore are immediately discarded since they are copies of
	 * left-ignores. */
	Tree *rightIgnore = 0;
	if ( pdaRun->tokenList != 0 && pdaRun->tokenList->kid->tree->flags & AF_RIGHT_IL_ATTACHED ) {
		Kid *riKid = treeRightIgnoreKid( prg, pdaRun->tokenList->kid->tree );

		/* If the right ignore has a left ignore, then that was the original
		 * right ignore. */
		Kid *li = treeLeftIgnoreKid( prg, riKid->tree );
		if ( li != 0 ) {
			treeUpref( li->tree );
			removeLeftIgnore( prg, sp, riKid->tree );
			rightIgnore = riKid->tree;
			treeUpref( rightIgnore );
			riKid->tree = li->tree;
		}
		else  {
			rightIgnore = riKid->tree;
			treeUpref( rightIgnore );
			removeRightIgnore( prg, sp, pdaRun->tokenList->kid->tree );
			pdaRun->tokenList->kid->tree->flags &= ~AF_RIGHT_IL_ATTACHED;
		}

	}

	treeDownref( prg, sp, rightIgnore );

	/* Detach left. */
	Tree *leftIgnore = 0;
	if ( input->tree->flags & AF_LEFT_IL_ATTACHED ) {
		Kid *liKid = treeLeftIgnoreKid( prg, input->tree );

		/* If the left ignore has a right ignore, then that was the original
		 * left ignore. */
		Kid *ri = treeRightIgnoreKid( prg, liKid->tree );
		if ( ri != 0 ) {
			treeUpref( ri->tree );
			removeRightIgnore( prg, sp, liKid->tree );
			leftIgnore = liKid->tree;
			treeUpref( leftIgnore );
			liKid->tree = ri->tree;
		}
		else {
			leftIgnore = liKid->tree;
			treeUpref( leftIgnore );
			removeLeftIgnore( prg, sp, input->tree );
			input->tree->flags &= ~AF_LEFT_IL_ATTACHED;
		}
	}

	if ( leftIgnore != 0 ) {
		pdaRun->accumIgnore = reverseKidList( leftIgnore->child );
		leftIgnore->child = 0;
	}

	treeDownref( prg, sp, leftIgnore );
}

void attachInput1( FsmRun *fsmRun, InputStream *is )
{
	if ( is->attached1 != 0 && is->attached1 != fsmRun )
		detachInput1( is->attached1, is );

	debug( REALM_INPUT, "attaching fsm run to input stream:  %p %p\n", fsmRun, is );
	fsmRun->attached1 = is;
	is->attached1 = fsmRun;
}

void attachInput2( FsmRun *fsmRun, SourceStream *is )
{
	if ( is->attached2 != 0 && is->attached2 != fsmRun )
		detachInput2( is->attached2, is );

	debug( REALM_INPUT, "attaching fsm run to source stream: %p %p\n", fsmRun, is );
	fsmRun->attached2 = is;
	is->attached2 = fsmRun;
}

void detachInput1( FsmRun *fsmRun, InputStream *is )
{
	debug( REALM_INPUT, "detaching fsm run from input stream:  %p %p\n", fsmRun, is );

	fsmRun->attached1 = 0;
	is->attached1 = 0;

	clearBuffered( fsmRun );

	if ( fsmRun->attached2 != 0 ) {
		fsmRun->attached2->attached2 = 0;
		fsmRun->attached2 = 0;
	}
}

void detachInput2( FsmRun *fsmRun, SourceStream *is )
{
	debug( REALM_INPUT, "detaching fsm run from source stream: %p %p\n", fsmRun, is );

	fsmRun->attached2 = 0;
	is->attached2 = 0;

	clearBuffered( fsmRun );

	if ( fsmRun->attached1 != 0 ) {
		fsmRun->attached1->attached1 = 0;
		fsmRun->attached1 = 0;
	}
}


void clearBuffered( FsmRun *fsmRun )
{
	/* If there is data in the current buffer then send the whole send back
	 * should be in this buffer. */
	if ( fsmRun->tokstart != 0 ) {
		fsmRun->p = fsmRun->pe = fsmRun->tokstart;
		fsmRun->tokstart = 0;
	}
	else {
		fsmRun->pe = fsmRun->p;
	}
}

void resetToken( FsmRun *fsmRun )
{
	/* If there is a token started, but never finished for a lack of data, we
	 * must first backup over it. */
	if ( fsmRun->tokstart != 0 ) {
		fsmRun->p = fsmRun->tokstart;
		fsmRun->tokstart = 0;
	}
}

/* Stops on:
 *   PcrRevToken
 */


static void sendBack( Program *prg, Tree **sp, PdaRun *pdaRun, FsmRun *fsmRun, 
		InputStream *inputStream, Kid *input )
{
	#ifdef COLM_LOG
	LangElInfo *lelInfo = prg->rtd->lelInfo;
	debug( REALM_PARSE, "sending back: %s  text: %.*s%s\n", 
			lelInfo[input->tree->id].name, 
			stringLength( input->tree->tokdata ), 
			stringData( input->tree->tokdata ), 
			input->tree->flags & AF_ARTIFICIAL ? " (artificial)" : "" );
	#endif

	if ( input->tree->flags & AF_NAMED ) {
		/* Send back anything in the buffer that has not been parsed. */
//		if ( fsmRun->p == fsmRun->runBuf->data )
//			sendBackRunBufHead( fsmRun, inputStream );

		/* Send the named lang el back first, then send back any leading
		 * whitespace. */
		undoConsumeLangEl( inputStream );
	}

	decrementSteps( pdaRun );

	/* Artifical were not parsed, instead sent in as items. */
	if ( input->tree->flags & AF_ARTIFICIAL ) {
		/* Check for reverse code. */
		if ( input->tree->flags & AF_HAS_RCODE ) {
			debug( REALM_PARSE, "tree has rcode, setting on deck\n" );
			pdaRun->onDeck = true;
			input->tree->flags &= ~AF_HAS_RCODE;
		}

		treeUpref( input->tree );

		streamPushTree( fsmRun, inputStream, input->tree, false );
	}
	else {
		/* Check for reverse code. */
		if ( input->tree->flags & AF_HAS_RCODE ) {
			debug( REALM_PARSE, "tree has rcode, setting on deck\n" );
			pdaRun->onDeck = true;
			input->tree->flags &= ~AF_HAS_RCODE;
		}

		/* Push back the token data. */
		sendBackText( fsmRun, inputStream, stringData( input->tree->tokdata ), 
				stringLength( input->tree->tokdata ) );


		/* If eof was just sent back remember that it needs to be sent again. */
		if ( input->tree->id == prg->rtd->eofLelIds[pdaRun->parserId] )
			inputStream->eofSent = false;

		/* If the item is bound then store remove it from the bindings array. */
		unbind( prg, sp, pdaRun, input->tree );
	}

	if ( pdaRun->steps == pdaRun->targetSteps ) {
		debug( REALM_PARSE, "trigger parse stop, steps = target = %d\n", pdaRun->targetSteps );
		pdaRun->stop = true;
	}

	/* Downref the tree that was sent back and free the kid. */
	treeDownref( prg, sp, input->tree );
	kidFree( prg, input );
}

void setRegion( PdaRun *pdaRun, Tree *tree )
{
	if ( pdaRun->accumIgnore == 0 ) {
		/* Recording the next region. */
		pt(tree)->region = pdaRun->nextRegionInd;
		if ( pdaRun->tables->tokenRegions[pt(tree)->region+1] != 0 )
			pdaRun->numRetry += 1;
	}
}

void ignoreTree( Program *prg, PdaRun *pdaRun, Tree *tree )
{
	transferReverseCode( pdaRun, tree );

	incrementSteps( pdaRun );

	setRegion( pdaRun, tree );

	/* Add the ignore string to the head of the ignore list. */
	Kid *ignore = kidAllocate( prg );
	ignore->tree = tree;

	/* Prepend it to the list of ignore tokens. */
	ignore->next = pdaRun->accumIgnore;
	pdaRun->accumIgnore = ignore;
}

Kid *makeTokenWithData( Program *prg, PdaRun *pdaRun, FsmRun *fsmRun, InputStream *inputStream, int id,
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
		debug( REALM_PARSE, "attaching ignore\n" );

		ignoreKid = reverseKidList( ignoreKid );

		/* Copy the ignore list first if we need to attach it as a right
		 * ignore. */
		IgnoreList *rightIgnore = 0;
		if ( pdaRun->tokenList != 0 ) {
			rightIgnore = ilAllocate( prg );
			rightIgnore->id = LEL_ID_IGNORE;
			rightIgnore->child = copyKidList( prg, ignoreKid );
			rightIgnore->generation = prg->nextIlGen++;
		}

		/* Make the ignore list for the left-ignore. */
		IgnoreList *leftIgnore = ilAllocate( prg );
		leftIgnore->id = LEL_ID_IGNORE;
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
	
	debug( REALM_PARSE, "ignoring: %.*s\n", ignoreStr->length, ignoreStr->data );

	Tree *tree = (Tree*)parseTreeAllocate( prg );
	tree->flags |= AF_PARSE_TREE;
	tree->refs = 1;
	tree->id = id;
	tree->tokdata = ignoreStr;

	/* Send it to the pdaRun. */
	ignoreTree( prg, pdaRun, tree );
}


/* Doesn't consume. */
Head *peekMatch( Program *prg, FsmRun *fsmRun, InputStream *inputStream )
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

/* Consumes. */
Head *extractMatch( Program *prg, FsmRun *fsmRun, InputStream *inputStream )
{
	long length = fsmRun->p - fsmRun->tokstart;
	Head *head = stringAllocPointer( prg, fsmRun->tokstart, length );
	head->location = locationAllocate( prg );
	head->location->line = inputStream->line;
	head->location->column = inputStream->column;
	head->location->byte = inputStream->byte;

	debug( REALM_PARSE, "location byte: %d\n", inputStream->byte );

	consumeData( inputStream, length );

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

	Kid *input = makeTokenWithData( prg, pdaRun, fsmRun, inputStream, id, tokdata, false, 0 );

	incrementSteps( pdaRun );

	/* Store any alternate scanning region. */
	if ( input != 0 && pdaRun->cs >= 0 )
		setRegion( pdaRun, input->tree );

	return input;
}

static Kid *sendTree( Program *prg, Tree **sp, PdaRun *pdaRun, FsmRun *fsmRun, InputStream *inputStream )
{
	Kid *input = kidAllocate( prg );
	input->tree = consumeTree( inputStream );

	incrementSteps( pdaRun );

	return input;
}

static void sendIgnoreTree( Program *prg, Tree **sp, PdaRun *pdaRun, FsmRun *fsmRun, InputStream *inputStream )
{
	Tree *tree = consumeTree( inputStream );
	ignoreTree( prg, pdaRun, tree );
}


Kid *sendEof( Program *prg, Tree **sp, InputStream *inputStream, FsmRun *fsmRun, PdaRun *pdaRun )
{
	debug( REALM_PARSE, "token: _EOF\n" );

	incrementSteps( pdaRun );

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

	/* Set the state using the state of the parser. */
	fsmRun->region = pdaRunGetNextRegion( pdaRun, 0 );
	fsmRun->cs = fsmRun->tables->entryByRegion[fsmRun->region];

	debug( REALM_PARSE, "scanning using token region: %s\n",
			prg->rtd->regionInfo[fsmRun->region].name );

	/* Clear the mark array. */
	memset( fsmRun->mark, 0, sizeof(fsmRun->mark) );
}

/* Tree null means compute from what we find in the parser. */
static void pushBtPoint( Program *prg, PdaRun *pdaRun )
{
	Tree *tree = 0;
	if ( pdaRun->accumIgnore != 0 ) 
		tree = pdaRun->accumIgnore->tree;
	else if ( pdaRun->tokenList != 0 )
		tree = pdaRun->tokenList->kid->tree;

	if ( tree != 0 ) {
		debug( REALM_PARSE, "pushing bt point with location byte %d\n", 
				( tree != 0 && tree->tokdata != 0 && tree->tokdata->location != 0 ) ? 
				tree->tokdata->location->byte : 0 );

		Kid *kid = kidAllocate( prg );
		kid->tree = tree;
		treeUpref( tree );
		kid->next = pdaRun->btPoint;
		pdaRun->btPoint = kid;
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

				debug( REALM_SCAN, "copying data over to new buffer\n" );
				assert( fsmRun->runBuf->offset == 0 );

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
			
		/* Get more data. */
		int have = fsmRun->tokstart != 0 ? fsmRun->p - fsmRun->tokstart : 0;
		int len = 0;
		debug( REALM_SCAN, "fetching data: have: %d  space: %d\n", have, space );
		int type = getData( fsmRun, inputStream, have, fsmRun->p, space, &len );

		switch ( type ) {
			case INPUT_DATA:
				fsmRun->pe = fsmRun->p + len;
				break;

			case INPUT_EOF:
				if ( fsmRun->tokstart != 0 )
					fsmRun->peof = fsmRun->pe;
				else 
					return SCAN_EOF;
				break;

			case INPUT_EOD:
				return SCAN_TRY_AGAIN_LATER;

			case INPUT_LANG_EL:
				if ( fsmRun->tokstart != 0 )
					fsmRun->peof = fsmRun->pe;
				else 
					return SCAN_LANG_EL;
				break;

			case INPUT_TREE:
				if ( fsmRun->tokstart != 0 )
					fsmRun->peof = fsmRun->pe;
				else 
					return SCAN_TREE;
				break;
			case INPUT_IGNORE:
				if ( fsmRun->tokstart != 0 )
					fsmRun->peof = fsmRun->pe;
				else
					return SCAN_IGNORE;
				break;
		}
	}

	/* Should not be reached. */
	return SCAN_ERROR;
}

/*
 * Stops on:
 *   PcrPreEof
 *   PcrGeneration
 *   PcrReduction
 *   PcrRevReduction
 *   PcrRevIgnore
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
		debug( REALM_PARSE, "parse loop start\n" );

		/* Pull the current scanner from the parser. This can change during
		 * parsing due to inputStream pushes, usually for the purpose of includes.
		 * */
		pdaRun->tokenId = scanToken( prg, pdaRun, fsmRun, inputStream );

		if ( pdaRun->tokenId == SCAN_TRY_AGAIN_LATER ) {
			debug( REALM_PARSE, "scanner says try again later\n" );
			break;
		}

		assert( pdaRun->input1 == 0 );
		pdaRun->input1 = 0;

		/* Check for EOF. */
		if ( pdaRun->tokenId == SCAN_EOF ) {
			inputStream->eofSent = true;
			pdaRun->input1 = sendEof( prg, sp, inputStream, fsmRun, pdaRun );

			pdaRun->frameId = prg->rtd->regionInfo[fsmRun->region].eofFrameId;

			if ( prg->ctxDepParsing && pdaRun->frameId >= 0 ) {
				debug( REALM_PARSE, "HAVE PRE_EOF BLOCK\n" );

				pdaRun->fi = &prg->rtd->frameInfo[pdaRun->frameId];
				pdaRun->code = pdaRun->fi->codeWV;

return PcrPreEof;
case PcrPreEof:
				{}

				/* 
				 * Need a no-token.
				 */
				//addNoToken( prg, sp, fsmRun, pdaRun, inputStream, pdaRun->frameId, pdaRun->tokenId, 0 );
			}
		}
		else if ( pdaRun->tokenId == SCAN_UNDO ) {
			/* Fall through with input1 = 0. FIXME: Do we need to send back ignore? */
			debug( REALM_PARSE, "invoking undo from the scanner\n" );
		}
		else if ( pdaRun->tokenId == SCAN_ERROR ) {
			/* Scanner error, maybe retry. */
			if ( pdaRun->accumIgnore == 0 && pdaRunGetNextRegion( pdaRun, 1 ) != 0 ) {
				debug( REALM_PARSE, "scanner failed, trying next region\n" );

				pdaRun->nextRegionInd += 1;
				goto skipSend;
			}
			else if ( pdaRun->numRetry > 0 ) {
				debug( REALM_PARSE, "invoking parse error from the scanner\n" );

				/* Fall through to send null (error). */
				pushBtPoint( prg, pdaRun );
			}
			else {
				debug( REALM_PARSE, "no alternate scanning regions\n" );

				/* There are no alternative scanning regions to try, nor are
				 * there any alternatives stored in the current parse tree. No
				 * choice but to end the parse. */
				pushBtPoint( prg, pdaRun );

				reportParseError( prg, sp, pdaRun );
				pdaRun->parseError = 1;
				goto skipSend;
			}
		}
		else if ( pdaRun->tokenId == SCAN_LANG_EL ) {
			debug( REALM_PARSE, "sending an named lang el\n" );

			/* A named language element (parsing colm program). */
			pdaRun->input1 = sendNamedLangEl( prg, sp, pdaRun, fsmRun, inputStream );
		}
		else if ( pdaRun->tokenId == SCAN_TREE ) {
			debug( REALM_PARSE, "sending a tree\n" );

			/* A tree already built. */
			pdaRun->input1 = sendTree( prg, sp, pdaRun, fsmRun, inputStream );
		}
		else if ( pdaRun->tokenId == SCAN_IGNORE ) {
			debug( REALM_PARSE, "sending an ignore token\n" );

			/* A tree to ignore. */
			sendIgnoreTree( prg, sp, pdaRun, fsmRun, inputStream );
			goto skipSend;
		}
		else if ( prg->ctxDepParsing && lelInfo[pdaRun->tokenId].frameId >= 0 ) {
			/* Has a generation action. */
			debug( REALM_PARSE, "token gen action: %s\n", 
					prg->rtd->lelInfo[pdaRun->tokenId].name );

			/* Make the token data. */
			pdaRun->tokdata = peekMatch( prg, fsmRun, inputStream );

			/* Note that we don't update the position now. It is done when the token
			 * data is pulled from the inputStream. */

			fsmRun->p = fsmRun->tokstart;
			fsmRun->tokstart = 0;

			pdaRun->fi = &prg->rtd->frameInfo[prg->rtd->lelInfo[pdaRun->tokenId].frameId];
			pdaRun->frameId = prg->rtd->lelInfo[pdaRun->tokenId].frameId;
			pdaRun->code = pdaRun->fi->codeWV;
			
return PcrGeneration;
case PcrGeneration:

			/* 
			 * May need a no-token.
			 */
			//addNoToken( prg, sp, fsmRun, pdaRun, inputStream, 
			//		prg->rtd->lelInfo[pdaRun->tokenId].frameId, pdaRun->tokenId, pdaRun->tokdata );

			makeReverseCode( pdaRun );

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
			pdaRun->input1 = sendToken( prg, sp, inputStream, fsmRun, pdaRun, pdaRun->tokenId );
		}

		if ( pdaRun->input1 != 0 )
			transferReverseCode( pdaRun, pdaRun->input1->tree );

		long pcr = parseToken( prg, sp, pdaRun, fsmRun, inputStream, PcrStart );
		
		while ( pcr != PcrDone ) {

return pcr;
case PcrReduction:
case PcrReverse:

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

/* Offset can be used to look at the next nextRegionInd. */
int pdaRunGetNextRegion( PdaRun *pdaRun, int offset )
{
	return pdaRun->tables->tokenRegions[pdaRun->nextRegionInd+offset];
}

Tree *getParsedRoot( PdaRun *pdaRun, int stop )
{
	if ( pdaRun->parseError )
		return 0;
	else if ( stop )
		return pdaRun->stackTop->tree;
	else
		return pdaRun->stackTop->next->tree;
}

void clearPdaRun( Program *prg, Tree **sp, PdaRun *pdaRun )
{
	/* Traverse the stack downreffing. */
	Kid *kid = pdaRun->stackTop;
	while ( kid != 0 ) {
		Kid *next = kid->next;
		treeDownref( prg, sp, kid->tree );
		kidFree( prg, kid );
		kid = next;
	}
	pdaRun->stackTop = 0;

	/* Traverse the token list downreffing. */
	Ref *ref = pdaRun->tokenList;
	while ( ref != 0 ) {
		Ref *next = ref->next;
		kidFree( prg, (Kid*)ref );
		ref = next;
	}
	pdaRun->tokenList = 0;

	/* Traverse the btPoint list downreffing */
	Kid *btp = pdaRun->btPoint;
	while ( btp != 0 ) {
		Kid *next = btp->next;
		treeDownref( prg, sp, btp->tree );
		kidFree( prg, (Kid*)btp );
		btp = next;
	}
	pdaRun->btPoint = 0;

	/* Clear out any remaining ignores. */
	Kid *ignore = pdaRun->accumIgnore;
	while ( ignore != 0 ) {
		Kid *next = ignore->next;
		treeDownref( prg, sp, ignore->tree );
		kidFree( prg, (Kid*)ignore );
		ignore = next;
	}

	if ( pdaRun->context != 0 )
		treeDownref( prg, sp, pdaRun->context );

	rcodeDownrefAll( prg, sp, &pdaRun->reverseCode );
	rtCodeVectEmpty( &pdaRun->reverseCode );
	rtCodeVectEmpty( &pdaRun->rcodeCollect );
}

int isParserStopFinished( PdaRun *pdaRun )
{
	int done = 
			pdaRun->stackTop->next != 0 && 
			pdaRun->stackTop->next->next == 0 &&
			pdaRun->stackTop->tree->id == pdaRun->stopTarget;
	return done;
}

void initPdaRun( PdaRun *pdaRun, Program *prg, PdaTables *tables,
		FsmRun *fsmRun, int parserId, long stopTarget, int revertOn, Tree *context )
{
	memset( pdaRun, 0, sizeof(PdaRun) );
	pdaRun->tables = tables;
	pdaRun->parserId = parserId;
	pdaRun->stopTarget = stopTarget;
	pdaRun->revertOn = revertOn;
	pdaRun->targetSteps = -1;

	debug( REALM_PARSE, "initializing PdaRun\n" );

	/* FIXME: need the right one here. */
	pdaRun->cs = prg->rtd->startStates[pdaRun->parserId];

	/* Init the element allocation variables. */
	pdaRun->stackTop = kidAllocate( prg );
	pdaRun->stackTop->tree = (Tree*)parseTreeAllocate( prg );
	pdaRun->stackTop->tree->flags |= AF_PARSE_TREE;

	pt(pdaRun->stackTop->tree)->state = -1;
	pdaRun->stackTop->tree->refs = 1;
	pdaRun->numRetry = 0;
	pdaRun->nextRegionInd = pdaRun->tables->tokenRegionInds[pdaRun->cs];
	pdaRun->stopParsing = false;
	pdaRun->accumIgnore = 0;
	pdaRun->btPoint = 0;
	pdaRun->checkNext = false;
	pdaRun->checkStop = false;

	initBindings( pdaRun );

	initRtCodeVect( &pdaRun->reverseCode );
	initRtCodeVect( &pdaRun->rcodeCollect );

	pdaRun->context = splitTree( prg, context );
	pdaRun->parseError = 0;
	pdaRun->input1 = 0;
	pdaRun->triggerUndo = 0;

	pdaRun->tokenId = 0;

	pdaRun->onDeck = false;
	pdaRun->parsed = 0;
	pdaRun->reject = false;

	pdaRun->rcBlockCount = 0;
}

long stackTopTarget( Program *prg, PdaRun *pdaRun )
{
	long state;
	if ( pt(pdaRun->stackTop->tree)->state < 0 )
		state = prg->rtd->startStates[pdaRun->parserId];
	else {
		state = pdaRun->tables->targs[(int)pdaRun->tables->indicies[pdaRun->tables->offsets[
				pt(pdaRun->stackTop->tree)->state] + 
				(pdaRun->stackTop->tree->id - pdaRun->tables->keys[pt(pdaRun->stackTop->tree)->state<<1])]];
	}
	return state;
}

/*
 * Local commit:
 * 		-clears reparse flags underneath
 * 		-must be possible to backtrack after
 * Global commit (revertOn)
 * 		-clears all reparse flags
 * 		-must be possible to backtrack after
 * Global commit (!revertOn)
 * 		-clears all reparse flags
 * 		-clears all 'parsed' reverse code
 * 		-clears all reverse code
 * 		-clears all alg structures
 */

int beenCommitted( Kid *kid )
{
	return kid->tree->flags & AF_COMMITTED;
}

Code *backupOverRcode( Code *rcode )
{
	Word len;
	rcode -= SIZEOF_WORD;
	read_word_p( len, rcode );
	rcode -= len;
	return rcode;
}

/* The top level of the stack is linked right-to-left. Trees underneath are
 * linked left-to-right. */
void commitKid( Program *prg, PdaRun *pdaRun, Tree **root, Kid *lel, Code **rcode, long *causeReduce )
{
	Tree *tree = 0;
	Tree **sp = root;
	Tree *restore = 0;

head:
	/* Commit */
	debug( REALM_PARSE, "commit: visiting %s\n",
			prg->rtd->lelInfo[lel->tree->id].name );

	/* Load up the parsed tree. */
	tree = lel->tree;

	/* Check for reverse code. */
	restore = 0;
	if ( tree->flags & AF_HAS_RCODE ) {
		/* If tree caused some reductions, now is not the right time to backup
		 * over the reverse code. We need to backup over the reductions first. Store
		 * the count of the reductions and do it when the count drops to zero. */
		if ( pt(tree)->causeReduce > 0 ) {
			/* The top reduce block does not correspond to this alg. */
//			#ifdef COLM_LOG_PARSE
//			if ( colm_log_parse ) {
//				cerr << "commit: causeReduce found, delaying backup: " << 
//						(long)pt(tree)->causeReduce << endl;
//			}
//			#endif
			*causeReduce = pt(tree)->causeReduce;
		}
		else {
			*rcode = backupOverRcode( *rcode );

//			if ( **rcode == IN_RESTORE_LHS ) {
//				#if COLM_LOG_PARSE
//				cerr << "commit: has restore_lhs" << endl;
//				#endif
//				read_tree_p( restore, (*rcode+1) );
//			}
		}
	}

//	FIXME: what was this about?
//	if ( restore != 0 )
//		tree = restore;

	/* All the parse algorithm data except for the RCODE flag is in the
	 * original. That is why we restore first, then we can clear the retry
	 * values. */

	/* Check causeReduce, might be time to backup over the reverse code
	 * belonging to a nonterminal that caused previous reductions. */
	if ( *causeReduce > 0 && 
			tree->id >= prg->rtd->firstNonTermId &&
			!(tree->flags & AF_TERM_DUP) )
	{
		*causeReduce -= 1;

		if ( *causeReduce == 0 ) {
			debug( REALM_PARSE, "commit: causeReduce dropped to zero, backing up over rcode\n" );

			/* Cause reduce just dropped down to zero. */
			*rcode = backupOverRcode( *rcode );
		}
	}

	/* Reset retries. */
	if ( tree->flags & AF_PARSED ) {
		if ( pt(tree)->retryLower > 0 ) {
			pdaRun->numRetry -= 1;
			pt(tree)->retryLower = 0;
		}
		if ( pt(tree)->retryUpper > 0 ) {
			pdaRun->numRetry -= 1;
			pt(tree)->retryUpper = 0;
		}
	}
	tree->flags |= AF_COMMITTED;

	/* Do not recures on trees that are terminal dups. */
	if ( !(tree->flags & AF_TERM_DUP) && treeChild( prg, tree ) != 0 ) {
		vm_push( (Tree*)lel );
		lel = treeChild( prg, tree );

		if ( lel != 0 ) {
			while ( lel != 0 ) {
				vm_push( (Tree*)lel );
				lel = lel->next;
			}
		}
	}

backup:
	if ( sp != root ) {
		Kid *next = (Kid*)vm_pop();
		if ( next->next == lel ) {
			/* Moving backwards. */
			lel = next;

			if ( !beenCommitted( lel ) )
				goto head;
		}
		else {
			/* Moving upwards. */
			lel = next;
		}

		goto backup;
	}

	pdaRun->numRetry = 0;
	assert( sp == root );
}

void commitFull( Program *prg, Tree **sp, PdaRun *pdaRun, long causeReduce )
{
//	#ifdef COLM_LOG_PARSE
//	if ( colm_log_parse ) {
//		cerr << "running full commit" << endl;
//	}
//	#endif
	
	Kid *kid = pdaRun->stackTop;
	Code *rcode = pdaRun->reverseCode.data + pdaRun->reverseCode.tabLen;

	/* The top level of the stack is linked right to left. This is the
	 * traversal order we need for committing. */
	while ( kid != 0 && !beenCommitted( kid ) ) {
		commitKid( prg, pdaRun, sp, kid, &rcode, &causeReduce );
		kid = kid->next;
	}

	/* We cannot always clear all the rcode here. We may need to backup over
	 * the parse statement. We depend on the context flag. */
	if ( !pdaRun->revertOn )
		rcodeDownrefAll( prg, sp, &pdaRun->reverseCode );
}

/*
 * shift:         retry goes into lower of shifted node.
 * reduce:        retry goes into upper of reduced node.
 * shift-reduce:  cannot be a retry
 */

/* Stops on:
 *   PcrReduction
 *   PcrRevToken
 *   PcrRevReduction
 */
long parseToken( Program *prg, Tree **sp, PdaRun *pdaRun,
		FsmRun *fsmRun, InputStream *inputStream, long entry )
{
	int pos;
	unsigned int *action;
	int rhsLen;
	int owner;
	int induceReject;
	int indPos;
	LangElInfo *lelInfo = prg->rtd->lelInfo;

switch ( entry ) {
case PcrStart:

	/* The scanner will send a null token if it can't find a token. */
	if ( pdaRun->input1 == 0 )
		goto parseError;

	/* The tree we are given must be * parse tree size. It also must have at
	 * least one reference. */
	assert( pdaRun->input1->tree->flags & AF_PARSE_TREE );
	assert( pdaRun->input1->tree->refs > 0 );

	/* This will cause input1 to be lost. This 
	 * path should be traced. */
	if ( pdaRun->cs < 0 )
		return PcrDone;

	/* Record the state in the parse tree. */
	pt(pdaRun->input1->tree)->state = pdaRun->cs;

again:
	if ( pdaRun->input1 == 0 )
		goto _out;

	pdaRun->lel = pdaRun->input1;
	pdaRun->curState = pdaRun->cs;

	/* This can disappear. An old experiment. */
	if ( lelInfo[pdaRun->lel->tree->id].ignore ) {
		/* Consume. */
		pdaRun->input1 = pdaRun->input1->next;

		Tree *stTree = pdaRun->stackTop->tree;
		if ( stTree->id == LEL_ID_IGNORE ) {
			pdaRun->lel->next = stTree->child;
			stTree->child = pdaRun->lel;
		}
		else {
			Kid *kid = kidAllocate( prg );
			IgnoreList *ignoreList = ilAllocate( prg );
			ignoreList->generation = prg->nextIlGen++;

			kid->tree = (Tree*)ignoreList;
			kid->tree->id = LEL_ID_IGNORE;
			kid->tree->refs = 1;
			kid->tree->child = pdaRun->lel;
			pdaRun->lel->next = 0;

			kid->next = pdaRun->stackTop;
			pdaRun->stackTop = kid;
		}

		/* Note: no state change. */
		goto again;
	}

	if ( pdaRun->lel->tree->id < pdaRun->tables->keys[pdaRun->curState<<1] ||
			pdaRun->lel->tree->id > pdaRun->tables->keys[(pdaRun->curState<<1)+1] ) {
		debug( REALM_PARSE, "parse error, no transition 1\n" );
		pushBtPoint( prg, pdaRun );
		goto parseError;
	}

	indPos = pdaRun->tables->offsets[pdaRun->curState] + 
		(pdaRun->lel->tree->id - pdaRun->tables->keys[pdaRun->curState<<1]);

	owner = pdaRun->tables->owners[indPos];
	if ( owner != pdaRun->curState ) {
		debug( REALM_PARSE, "parse error, no transition 2\n" );
		pushBtPoint( prg, pdaRun );
		goto parseError;
	}

	pos = pdaRun->tables->indicies[indPos];
	if ( pos < 0 ) {
		debug( REALM_PARSE, "parse error, no transition 3\n" );
		pushBtPoint( prg, pdaRun );
		goto parseError;
	}

	/* Checking complete. */

	induceReject = false;
	pdaRun->cs = pdaRun->tables->targs[pos];
	action = pdaRun->tables->actions + pdaRun->tables->actInds[pos];
	if ( pt(pdaRun->lel->tree)->retryLower )
		action += pt(pdaRun->lel->tree)->retryLower;

	/*
	 * Shift
	 */

	if ( *action & act_sb ) {
		debug( REALM_PARSE, "shifted: %s\n", 
				prg->rtd->lelInfo[pt(pdaRun->lel->tree)->id].name );
		/* Consume. */
		pdaRun->input1 = pdaRun->input1->next;

		pt(pdaRun->lel->tree)->state = pdaRun->curState;
		pdaRun->lel->next = pdaRun->stackTop;
		pdaRun->stackTop = pdaRun->lel;

		/* If its a token then attach ignores and record it in the token list
		 * of the next ignore attachment to use. */
		if ( pdaRun->lel->tree->id < prg->rtd->firstNonTermId ) {
			attachIgnore( prg, sp, pdaRun, pdaRun->lel );

			Ref *ref = (Ref*)kidAllocate( prg );
			ref->kid = pdaRun->lel;
			//treeUpref( pdaRun->lel->tree );
			ref->next = pdaRun->tokenList;
			pdaRun->tokenList = ref;
		}

		/* If shifting a termDup then change it to the nonterm. */
		if ( pdaRun->lel->tree->id < prg->rtd->firstNonTermId &&
				prg->rtd->lelInfo[pdaRun->lel->tree->id].termDupId > 0 )
		{
			pdaRun->lel->tree->id = prg->rtd->lelInfo[pdaRun->lel->tree->id].termDupId;
			pdaRun->lel->tree->flags |= AF_TERM_DUP;
		}

		if ( action[1] == 0 )
			pt(pdaRun->lel->tree)->retryLower = 0;
		else {
			debug( REALM_PARSE, "retry: %p\n", pdaRun->stackTop );
			pt(pdaRun->lel->tree)->retryLower += 1;
			assert( pt(pdaRun->lel->tree)->retryUpper == 0 );
			/* FIXME: Has the retry already been counted? */
			pdaRun->numRetry += 1; 
		}
	}

	/* 
	 * Commit
	 */

	if ( pdaRun->tables->commitLen[pos] != 0 ) {
		long causeReduce = 0;
		if ( pdaRun->input1 != 0 ) { 
			if ( pdaRun->input1->tree->flags & AF_HAS_RCODE )
				causeReduce = pt(pdaRun->input1->tree)->causeReduce;
		}
		commitFull( prg, sp, pdaRun, causeReduce );
	}

	/*
	 * Reduce
	 */

	if ( *action & act_rb ) {
		int r, objectLength;
		Kid *last, *child, *attrs;

		pdaRun->reduction = *action >> 2;

		if ( pdaRun->input1 != 0 )
			pt(pdaRun->input1->tree)->causeReduce += 1;

		pdaRun->redLel = kidAllocate( prg );
		pdaRun->redLel->tree = (Tree*)parseTreeAllocate( prg );
		pdaRun->redLel->tree->flags |= AF_PARSE_TREE;
		pdaRun->redLel->tree->refs = 1;
		pdaRun->redLel->tree->id = prg->rtd->prodInfo[pdaRun->reduction].lhsId;
		pdaRun->redLel->tree->prodNum = prg->rtd->prodInfo[pdaRun->reduction].prodNum;

		pdaRun->redLel->next = 0;
		pt(pdaRun->redLel->tree)->causeReduce = 0;
		pt(pdaRun->redLel->tree)->retryLower = 0;
		pt(pdaRun->redLel->tree)->retryUpper = pt(pdaRun->lel->tree)->retryLower;
		pt(pdaRun->lel->tree)->retryLower = 0;

		/* Allocate the attributes. */
		objectLength = prg->rtd->lelInfo[pdaRun->redLel->tree->id].objectLength;
		attrs = allocAttrs( prg, objectLength );

		/* Build the list of children. */
		Kid *realChild = 0;
		rhsLen = prg->rtd->prodInfo[pdaRun->reduction].length;
		child = last = 0;
		for ( r = 0;; ) {
			if ( r == rhsLen && pdaRun->stackTop->tree->id != LEL_ID_IGNORE )
				break;

			child = pdaRun->stackTop;
			pdaRun->stackTop = pdaRun->stackTop->next;
			child->next = last;
			last = child;
			
			if ( child->tree->id != LEL_ID_IGNORE ) {
				/* Count it only if it is not an ignore token. */
				r++;
			}

			if ( child->tree->id != LEL_ID_IGNORE )
				realChild = child;
		}

		pdaRun->redLel->tree->child = kidListConcat( attrs, child );

		debug( REALM_PARSE, "reduced: %s rhsLen %d\n",
				prg->rtd->prodInfo[pdaRun->reduction].name, rhsLen );
		if ( action[1] == 0 )
			pt(pdaRun->redLel->tree)->retryUpper = 0;
		else {
			pt(pdaRun->redLel->tree)->retryUpper += 1;
			assert( pt(pdaRun->lel->tree)->retryLower == 0 );
			pdaRun->numRetry += 1;
			debug( REALM_PARSE, "retry: %p\n", pdaRun->redLel );
		}

		/* When the production is of zero length we stay in the same state.
		 * Otherwise we use the state stored in the first child. */
		pdaRun->cs = rhsLen == 0 ? pdaRun->curState : pt(realChild->tree)->state;

		assert( pdaRun->redLel->tree->refs == 1 );

		if ( prg->ctxDepParsing && prg->rtd->prodInfo[pdaRun->reduction].frameId >= 0 ) {
			/* Frame info for reduction. */
			pdaRun->fi = &prg->rtd->frameInfo[prg->rtd->prodInfo[pdaRun->reduction].frameId];
			pdaRun->frameId = prg->rtd->prodInfo[pdaRun->reduction].frameId;
			pdaRun->reject = false;
			pdaRun->parsed = 0;
			pdaRun->code = pdaRun->fi->codeWV;

return PcrReduction;
case PcrReduction:

			if ( prg->induceExit )
				goto fail;

			/* If the lhs was stored and it changed then we need to restore the
			 * original upon backtracking, otherwise downref since we took a
			 * copy above. */
			if ( pdaRun->parsed != 0 ) {
				if ( pdaRun->parsed != pdaRun->redLel->tree ) {
					debug( REALM_PARSE, "lhs tree was modified, adding a restore instruction\n" );

					/* Transfer the lhs from the environment to redLel. */
					pdaRun->redLel->tree = prepParseTree( prg, sp, pdaRun->redLel->tree );
					treeUpref( pdaRun->redLel->tree );

					append( &pdaRun->rcodeCollect, IN_RESTORE_LHS );
					appendWord( &pdaRun->rcodeCollect, (Word)pdaRun->parsed );
					append( &pdaRun->rcodeCollect, SIZEOF_CODE + SIZEOF_WORD );
				}
			}

			/* Pull out the reverse code, if any. */
			makeReverseCode( pdaRun );
			transferReverseCode( pdaRun, pdaRun->redLel->tree );

			/* Perhaps the execution environment is telling us we need to
			 * reject the reduction. */
			induceReject = pdaRun->reject;
		}

		/* If the left hand side was replaced then the only parse algorithm
		 * data that is contained in it will the AF_HAS_RCODE flag. Everthing
		 * else will be in the original. This requires that we restore first
		 * when going backwards and when doing a commit. */

		if ( induceReject ) {
			debug( REALM_PARSE, "error induced during reduction of %s\n",
					prg->rtd->lelInfo[pdaRun->redLel->tree->id].name );
			pt(pdaRun->redLel->tree)->state = pdaRun->curState;
			pdaRun->redLel->next = pdaRun->stackTop;
			pdaRun->stackTop = pdaRun->redLel;
			/* FIXME: What is the right argument here? */
			pushBtPoint( prg, pdaRun );
			goto parseError;
		}

		pdaRun->redLel->next = pdaRun->input1;
		pdaRun->input1 = pdaRun->redLel;
	}

	goto again;

parseError:
	debug( REALM_PARSE, "hit error, backtracking\n" );

	if ( pdaRun->numRetry == 0 ) {
		debug( REALM_PARSE, "out of retries failing parse\n" );
		goto fail;
	}

	while ( 1 ) {
		if ( pdaRun->onDeck ) {
			debug( REALM_BYTECODE, "dropping out for reverse code call\n" );

			pdaRun->frameId = -1;
			pdaRun->code = popReverseCode( &pdaRun->reverseCode );

return PcrReverse;
case PcrReverse: 

			decrementSteps( pdaRun );
			{}

		}
		else if ( pdaRun->checkNext ) {
			pdaRun->checkNext = false;

			if ( pdaRun->next > 0 && pdaRun->tables->tokenRegions[pdaRun->next] != 0 ) {
				debug( REALM_PARSE, "found a new region\n" );
				pdaRun->numRetry -= 1;
				pdaRun->cs = stackTopTarget( prg, pdaRun );
				pdaRun->nextRegionInd = pdaRun->next;
				return PcrDone;
			}
		}
		else if ( pdaRun->checkStop ) {
			pdaRun->checkStop = false;

			if ( pdaRun->stop ) {
				debug( REALM_PARSE, "stopping the backtracking, steps is %d\n", pdaRun->steps );

				pdaRun->cs = stackTopTarget( prg, pdaRun );
				goto _out;
			}
		}
		else if ( pdaRun->input1 != 0 ) {
			/* Either we are dealing with a terminal that was
			 * shifted or a nonterminal that was reduced. */
			if ( pdaRun->input1->tree->id < prg->rtd->firstNonTermId || 
					(pdaRun->input1->tree->flags & AF_TERM_DUP) )
			{
				assert( pt(pdaRun->input1->tree)->retryUpper == 0 );

				if ( pt(pdaRun->input1->tree)->retryLower != 0 ) {
					debug( REALM_PARSE, "found retry targ: %p\n", pdaRun->input1 );

					pdaRun->numRetry -= 1;
					pdaRun->cs = pt(pdaRun->input1->tree)->state;
					goto again;
				}

				if ( pt(pdaRun->input1->tree)->causeReduce != 0 ) {
					pdaRun->undoLel = pdaRun->stackTop;

					/* Check if we've arrived at the stack sentinal. This guard
					 * is here to allow us to initially set numRetry to one to
					 * cause the parser to backup all the way to the beginning
					 * when an error occurs. */
					if ( pdaRun->undoLel->next == 0 )
						break;

					/* Either we are dealing with a terminal that was
					 * shifted or a nonterminal that was reduced. */
					assert( !(pdaRun->stackTop->tree->id < prg->rtd->firstNonTermId || 
							(pdaRun->stackTop->tree->flags & AF_TERM_DUP) ) );

					debug( REALM_PARSE, "backing up over non-terminal: %s\n",
							prg->rtd->lelInfo[pdaRun->stackTop->tree->id].name );

					/* Pop the item from the stack. */
					pdaRun->stackTop = pdaRun->stackTop->next;

					/* Queue it as next input1 item. */
					pdaRun->undoLel->next = pdaRun->input1;
					pdaRun->input1 = pdaRun->undoLel;
				}
				else {
					long region = pt(pdaRun->input1->tree)->region;
					pdaRun->next = region > 0 ? region + 1 : 0;
					pdaRun->checkNext = true;
					pdaRun->checkStop = true;

					sendBack( prg, sp, pdaRun, fsmRun, inputStream, pdaRun->input1 );

					pdaRun->input1 = 0;

				}
			}
			else if ( pdaRun->input1->tree->flags & AF_HAS_RCODE ) {
				debug( REALM_PARSE, "tree has rcode, setting on deck\n" );
				pdaRun->onDeck = true;
				pdaRun->parsed = 0;

				/* Only the RCODE flag was in the replaced lhs. All the rest is in
				 * the the original. We read it after restoring. */

				pdaRun->input1->tree->flags &= ~AF_HAS_RCODE;
			}
			else {
				/* Remove it from the input queue. */
				pdaRun->undoLel = pdaRun->input1;
				pdaRun->input1 = pdaRun->input1->next;

				/* Extract the real children from the child list. */
				Kid *first = treeExtractChild( prg, pdaRun->undoLel->tree );

				/* Walk the child list and and push the items onto the parsing
				 * stack one at a time. */
				while ( first != 0 ) {
					/* Get the next item ahead of time. */
					Kid *next = first->next;

					/* Push onto the stack. */
					first->next = pdaRun->stackTop;
					pdaRun->stackTop = first;

					first = next;
				}

				/* If there is an input1 queued, this is one less reduction it has
				 * caused. */
				if ( pdaRun->input1 != 0 )
					pt(pdaRun->input1->tree)->causeReduce -= 1;

				if ( pt(pdaRun->undoLel->tree)->retryUpper != 0 ) {
					/* There is always an input1 item here because reduce
					 * conflicts only happen on a lookahead character. */
					assert( pdaRun->input1 != pdaRun->undoLel );
					assert( pdaRun->input1 != 0 );
					assert( pt(pdaRun->undoLel->tree)->retryLower == 0 );
					assert( pt(pdaRun->input1->tree)->retryUpper == 0 );

					/* Transfer the retry from undoLel to input1. */
					pt(pdaRun->input1->tree)->retryLower = pt(pdaRun->undoLel->tree)->retryUpper;
					pt(pdaRun->input1->tree)->retryUpper = 0;
					pt(pdaRun->input1->tree)->state = stackTopTarget( prg, pdaRun );
				}

				/* Free the reduced item. */
				treeDownref( prg, sp, pdaRun->undoLel->tree );
				kidFree( prg, pdaRun->undoLel );
			}
		}
		else if ( pdaRun->accumIgnore != 0 ) {
			debug( REALM_PARSE, "have accumulated ignore to undo\n" );

			/* Send back any accumulated ignore tokens, then trigger error
			 * in the the parser. */
			pdaRun->ignore6 = pdaRun->accumIgnore;
			pdaRun->accumIgnore = pdaRun->accumIgnore->next;
			pdaRun->ignore6->next = 0;
			
			long region = pt(pdaRun->ignore6->tree)->region;
			pdaRun->next = region > 0 ? region + 1 : 0;
			pdaRun->checkNext = true;
			pdaRun->checkStop = true;
			
			sendBackIgnore( prg, sp, pdaRun, fsmRun, inputStream, pdaRun->ignore6 );
		}
		else {

			/* Now it is time to undo something. Pick an element from the top of
			 * the stack. */
			pdaRun->undoLel = pdaRun->stackTop;

			/* Check if we've arrived at the stack sentinal. This guard is
			 * here to allow us to initially set numRetry to one to cause the
			 * parser to backup all the way to the beginning when an error
			 * occurs. */
			if ( pdaRun->undoLel->next == 0 )
				break;

			/* Either we are dealing with a terminal that was
			 * shifted or a nonterminal that was reduced. */
			if ( pdaRun->stackTop->tree->id < prg->rtd->firstNonTermId || 
					(pdaRun->stackTop->tree->flags & AF_TERM_DUP) )
			{
				debug( REALM_PARSE, "backing up over effective terminal: %s\n",
							prg->rtd->lelInfo[pdaRun->stackTop->tree->id].name );

				/* Pop the item from the stack. */
				pdaRun->stackTop = pdaRun->stackTop->next;

				/* Undo the translation from termDup. */
				if ( pdaRun->undoLel->tree->flags & AF_TERM_DUP ) {
					pdaRun->undoLel->tree->id = prg->rtd->lelInfo[pdaRun->undoLel->tree->id].termDupId;
					pdaRun->undoLel->tree->flags &= ~AF_TERM_DUP;
				}

				/* Queue it as next input1 item. */
				pdaRun->undoLel->next = pdaRun->input1;
				pdaRun->input1 = pdaRun->undoLel;

				/* Pop from the token list. */
				Ref *ref = pdaRun->tokenList;
				pdaRun->tokenList = ref->next;
				kidFree( prg, (Kid*)ref );

				detachIgnores( prg, sp, pdaRun, fsmRun, pdaRun->input1 );
			}
			else {
				debug( REALM_PARSE, "backing up over non-terminal: %s\n",
						prg->rtd->lelInfo[pdaRun->stackTop->tree->id].name );

				/* Pop the item from the stack. */
				pdaRun->stackTop = pdaRun->stackTop->next;

				/* Queue it as next input1 item. */
				pdaRun->undoLel->next = pdaRun->input1;
				pdaRun->input1 = pdaRun->undoLel;
			}
		}
	}

fail:
	pdaRun->cs = -1;
	pdaRun->parseError = 1;

	/* If we failed parsing on tree we must free it. The caller expected us to
	 * either consume it or send it back to the input1. */
	if ( pdaRun->input1 != 0 ) {
		treeDownref( prg, sp, pdaRun->input1->tree );
		kidFree( prg, pdaRun->input1 );
		pdaRun->input1 = 0;
	}

	/* FIXME: do we still need to fall through here? A fail is permanent now,
	 * no longer called into again. */

_out:
	pdaRun->nextRegionInd = pdaRun->tables->tokenRegionInds[pdaRun->cs];

case PcrDone:
break; }

	return PcrDone;
}
