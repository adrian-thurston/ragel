/*
 *  Copyright 2007-2012 Adrian Thurston <thurston@complang.org>
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

static void initFsmRun( Program *prg, FsmRun *fsmRun )
{
	fsmRun->tables = prg->rtd->fsmTables;

	fsmRun->consumeBuf = 0;

	fsmRun->p = fsmRun->pe = 0;
	fsmRun->toklen = 0;
	fsmRun->eof = 0;

	fsmRun->preRegion = -1;
}

void clearFsmRun( Program *prg, FsmRun *fsmRun )
{
	if ( fsmRun->consumeBuf != 0 ) {
		/* Transfer the run buf list to the program */
		RunBuf *head = fsmRun->consumeBuf;
		RunBuf *tail = head;
		while ( tail->next != 0 )
			tail = tail->next;

		tail->next = prg->allocRunBuf;
		prg->allocRunBuf = head;
	}
}

void incrementSteps( PdaRun *pdaRun )
{
	pdaRun->steps += 1;
	//debug( prg, REALM_PARSE, "steps up to %ld\n", pdaRun->steps );
}

void decrementSteps( PdaRun *pdaRun )
{
	pdaRun->steps -= 1;
	//debug( prg, REALM_PARSE, "steps down to %ld\n", pdaRun->steps );
}

Head *streamPull( Program *prg, PdaRun *pdaRun, StreamImpl *is, long length )
{
	if ( pdaRun != 0 ) {
		FsmRun *fsmRun = pdaRun->fsmRun;
		RunBuf *runBuf = fsmRun->consumeBuf;
		if ( length > ( FSM_BUFSIZE - runBuf->length ) ) {
			runBuf = newRunBuf();
			runBuf->next = fsmRun->consumeBuf;
			fsmRun->consumeBuf = runBuf;
		}

		char *dest = runBuf->data + runBuf->length;

		is->funcs->getData( is, dest, length );
		Location *loc = locationAllocate( prg );
		is->funcs->consumeData( is, length, loc );

		runBuf->length += length;

		fsmRun->p = fsmRun->pe = 0;
		fsmRun->toklen = 0;

		Head *tokdata = stringAllocPointer( prg, dest, length );
		tokdata->location = loc;

		return tokdata;
	}
	else {
		Head *head = initStrSpace( length );
		char *dest = (char*)head->data;

		is->funcs->getData( is, dest, length );
		Location *loc = locationAllocate( prg );
		is->funcs->consumeData( is, length, loc );
		head->location = loc;

		return head;
	}
}

void undoStreamPull( StreamImpl *is, const char *data, long length )
{
	//debug( REALM_PARSE, "undoing stream pull\n" );

	is->funcs->prependData( is, data, length );
}

void streamPushText( StreamImpl *is, const char *data, long length )
{
	is->funcs->prependData( is, data, length );
}

void streamPushTree( StreamImpl *is, Tree *tree, int ignore )
{
	is->funcs->prependTree( is, tree, ignore );
}

void streamPushStream( StreamImpl *is, Tree *tree )
{
	is->funcs->prependStream( is, tree );
}

void undoStreamPush( Program *prg, Tree **sp, StreamImpl *is, long length )
{
	if ( length < 0 ) {
		Tree *tree = is->funcs->undoPrependTree( is );
		treeDownref( prg, sp, tree );
	}
	else {
		is->funcs->undoPrependData( is, length );
	}
}

void undoStreamAppend( Program *prg, Tree **sp, StreamImpl *is, Tree *input, long length )
{
	if ( input->id == LEL_ID_STR )
		is->funcs->undoAppendData( is, length );
	else if ( input->id == LEL_ID_STREAM )
		is->funcs->undoAppendStream( is );
	else {
		Tree *tree = is->funcs->undoAppendTree( is );
		treeDownref( prg, sp, tree );
	}
}

/* Should only be sending back whole tokens/ignores, therefore the send back
 * should never cross a buffer boundary. Either we slide back data, or we move to
 * a previous buffer and slide back data. */
static void sendBackText( FsmRun *fsmRun, StreamImpl *is, const char *data, long length )
{
	//debug( REALM_PARSE, "push back of %ld characters\n", length );

	if ( length == 0 )
		return;

	//debug( REALM_PARSE, "sending back text: %.*s\n", 
	//		(int)length, data );

	is->funcs->undoConsumeData( is, data, length );
}

void sendBackTree( StreamImpl *is, Tree *tree )
{
	is->funcs->undoConsumeTree( is, tree, false );
}

/*
 * Stops on:
 *   PcrRevIgnore
 */
static void sendBackIgnore( Program *prg, Tree **sp, PdaRun *pdaRun, FsmRun *fsmRun,
		StreamImpl *is, ParseTree *parseTree )
{
	#ifdef DEBUG
	LangElInfo *lelInfo = prg->rtd->lelInfo;
	debug( prg, REALM_PARSE, "sending back: %s%s\n",
		lelInfo[parseTree->shadow->tree->id].name, 
		parseTree->flags & PF_ARTIFICIAL ? " (artificial)" : "" );
	#endif

	Head *head = parseTree->shadow->tree->tokdata;
	int artificial = parseTree->flags & PF_ARTIFICIAL;

	if ( head != 0 && !artificial )
		sendBackText( fsmRun, is, stringData( head ), head->length );

	decrementSteps( pdaRun );

	/* Check for reverse code. */
	if ( parseTree->flags & PF_HAS_RCODE ) {
		pdaRun->onDeck = true;
		parseTree->flags &= ~PF_HAS_RCODE;
	}

	if ( pdaRun->steps == pdaRun->targetSteps ) {
		debug( prg, REALM_PARSE, "trigger parse stop, steps = target = %d\n", pdaRun->targetSteps );
		pdaRun->stop = true;
	}
}

void resetToken( PdaRun *pdaRun )
{
	FsmRun *fsmRun = pdaRun->fsmRun;

	/* If there is a token started, but never finished for a lack of data, we
	 * must first backup over it. */
	if ( fsmRun->tokstart != 0 ) {
		fsmRun->p = fsmRun->pe = 0;
		fsmRun->toklen = 0;
		fsmRun->eof = 0;
	}
}

/* Stops on:
 *   PcrRevToken
 */

static void sendBack( Program *prg, Tree **sp, PdaRun *pdaRun, FsmRun *fsmRun, 
		StreamImpl *is, ParseTree *parseTree )
{
	debug( prg, REALM_PARSE, "sending back: %s\n", prg->rtd->lelInfo[parseTree->id].name );

	if ( parseTree->flags & PF_NAMED ) {
		/* Send the named lang el back first, then send back any leading
		 * whitespace. */
		is->funcs->undoConsumeLangEl( is );
	}

	decrementSteps( pdaRun );

	/* Artifical were not parsed, instead sent in as items. */
	if ( parseTree->flags & PF_ARTIFICIAL ) {
		/* Check for reverse code. */
		if ( parseTree->flags & PF_HAS_RCODE ) {
			debug( prg, REALM_PARSE, "tree has rcode, setting on deck\n" );
			pdaRun->onDeck = true;
			parseTree->flags &= ~PF_HAS_RCODE;
		}

		treeUpref( parseTree->shadow->tree );

		sendBackTree( is, parseTree->shadow->tree );
	}
	else {
		/* Check for reverse code. */
		if ( parseTree->flags & PF_HAS_RCODE ) {
			debug( prg, REALM_PARSE, "tree has rcode, setting on deck\n" );
			pdaRun->onDeck = true;
			parseTree->flags &= ~PF_HAS_RCODE;
		}

		/* Push back the token data. */
		sendBackText( fsmRun, is, stringData( parseTree->shadow->tree->tokdata ), 
				stringLength( parseTree->shadow->tree->tokdata ) );

		/* If eof was just sent back remember that it needs to be sent again. */
		if ( parseTree->id == prg->rtd->eofLelIds[pdaRun->parserId] )
			is->eofSent = false;

		/* If the item is bound then store remove it from the bindings array. */
		prg->rtd->popBinding( pdaRun, parseTree );
	}

	if ( pdaRun->steps == pdaRun->targetSteps ) {
		debug( prg, REALM_PARSE, "trigger parse stop, steps = target = %d\n", pdaRun->targetSteps );
		pdaRun->stop = true;
	}

	/* Downref the tree that was sent back and free the kid. */
	treeDownref( prg, sp, parseTree->shadow->tree );
	kidFree( prg, parseTree->shadow );
	parseTreeFree( prg, parseTree );
}

void setRegion( PdaRun *pdaRun, int emptyIgnore, ParseTree *tree )
{
	if ( emptyIgnore ) {
		/* Recording the next region. */
		tree->region = pdaRun->nextRegionInd;
		if ( pdaRun->tables->tokenRegions[tree->region+1] != 0 )
			pdaRun->numRetry += 1;
	}
}

void ignoreTree( Program *prg, FsmRun *fsmRun, PdaRun *pdaRun, Tree *tree )
{
	int emptyIgnore = pdaRun->accumIgnore == 0;

	incrementSteps( pdaRun );

	ParseTree *parseTree = parseTreeAllocate( prg );
	parseTree->shadow = kidAllocate( prg );
	parseTree->shadow->tree = tree;

	parseTree->next = pdaRun->accumIgnore;
	pdaRun->accumIgnore = parseTree;

	transferReverseCode( pdaRun, parseTree );

	if ( fsmRun->preRegion >= 0 )
		parseTree->flags |= PF_RIGHT_IGNORE;

	setRegion( pdaRun, emptyIgnore, pdaRun->accumIgnore );
}

void ignoreTree2( Program *prg, PdaRun *pdaRun, Tree *tree )
{
	int emptyIgnore = pdaRun->accumIgnore == 0;

	incrementSteps( pdaRun );

	ParseTree *parseTree = parseTreeAllocate( prg );
	parseTree->flags |= PF_ARTIFICIAL;
	parseTree->shadow = kidAllocate( prg );
	parseTree->shadow->tree = tree;

	parseTree->next = pdaRun->accumIgnore;
	pdaRun->accumIgnore = parseTree;

	transferReverseCode( pdaRun, parseTree );

	setRegion( pdaRun, emptyIgnore, pdaRun->accumIgnore );
}

Kid *makeTokenWithData( Program *prg, PdaRun *pdaRun, FsmRun *fsmRun, 
		StreamImpl *is, int id, Head *tokdata )
{
	/* Make the token object. */
	long objectLength = prg->rtd->lelInfo[id].objectLength;
	Kid *attrs = allocAttrs( prg, objectLength );

	Kid *input = 0;
	input = kidAllocate( prg );
	input->tree = treeAllocate( prg );

	debug( prg, REALM_PARSE, "made token %p\n", input->tree );

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

	return input;
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

	/* If there are no error points on record assume the error occurred at the
	 * beginning of the stream. */
	if ( deepest == 0 )  {
		errorHead = stringAllocFull( prg, "<input>:1:1: parse error", 32 );
		errorHead->location = locationAllocate( prg );
		errorHead->location->line = 1;
		errorHead->location->column = 1;
	}
	else {
		debug( prg, REALM_PARSE, "deepest location byte: %d\n", deepest->location->byte );

		const char *name = deepest->location->name;
		long line = deepest->location->line;
		long i, column = deepest->location->column;
		long byte = deepest->location->byte;

		for ( i = 0; i < deepest->length; i++ ) {
			if ( deepest->data[i] != '\n' )
				column += 1;
			else {
				line += 1;
				column = 1;
			}
			byte += 1;
		}

		if ( name == 0 )
			name = "<input>";
		char *formatted = malloc( strlen( name ) + 128 );
		sprintf( formatted, "%s:%ld:%ld: parse error", name, line, column );
		errorHead = stringAllocFull( prg, formatted, strlen(formatted) );
		free( formatted );

		errorHead->location = locationAllocate( prg );

		errorHead->location->name = deepest->location->name;
		errorHead->location->line = line;
		errorHead->location->column = column;
		errorHead->location->byte = byte;
	}

	Tree *tree = constructString( prg, errorHead );
	treeDownref( prg, sp, pdaRun->parseErrorText );
	pdaRun->parseErrorText = tree;
	treeUpref( pdaRun->parseErrorText );
}

static void attachRightIgnore( Program *prg, Tree **sp, PdaRun *pdaRun, ParseTree *parseTree )
{
	if ( pdaRun->accumIgnore == 0 )
		return;

	if ( pdaRun->stackTop->id > 0 && pdaRun->stackTop->id < prg->rtd->firstNonTermId ) {
		/* OK, do it */
		debug( prg, REALM_PARSE, "attaching right ignore\n" );

		/* Reset. */
		assert( ! ( parseTree->flags & PF_RIGHT_IL_ATTACHED ) );

		ParseTree *accum = pdaRun->accumIgnore;

		ParseTree *stopAt = 0, *use = accum;
		while ( use != 0 ) {
			if ( ! (use->flags & PF_RIGHT_IGNORE) )
				stopAt = use;
			use = use->next;
		}

		if ( stopAt != 0 ) {
			/* Stop at was set. Make it the last item in the igore list. Take
			 * the rest. */
			accum = stopAt->next;
			stopAt->next = 0;
		}
		else {
			/* Stop at was never set. All right ignore. Use it all. */
			pdaRun->accumIgnore = 0;
		}

		/* The data list needs to be extracted and reversed. The parse tree list
		 * can remain in stack order. */
		ParseTree *child = accum, *last = 0;
		Kid *dataChild = 0, *dataLast = 0;

		while ( child ) {
			dataChild = child->shadow;
			ParseTree *next = child->next;

			/* Reverse the lists. */
			dataChild->next = dataLast;
			child->next = last;

			/* Detach the parse tree from the data tree. */
			child->shadow = 0;

			/* Keep the last for reversal. */
			dataLast = dataChild;
			last = child;

			child = next;
		}

		/* Last is now the first. */
		parseTree->rightIgnore = last;

		if ( dataChild != 0 ) {
			debug( prg, REALM_PARSE, "attaching ignore right\n" );

			Kid *ignoreKid = dataLast;

			/* Copy the ignore list first if we need to attach it as a right
			 * ignore. */
			Tree *rightIgnore = 0;

			rightIgnore = treeAllocate( prg );
			rightIgnore->id = LEL_ID_IGNORE;
			rightIgnore->child = ignoreKid;

			Tree *pushTo = parseTree->shadow->tree;

			pushTo = pushRightIgnore( prg, pushTo, rightIgnore );

			parseTree->shadow->tree = pushTo;

			parseTree->flags |= PF_RIGHT_IL_ATTACHED;
		}
	}
}

static void attachLeftIgnore( Program *prg, Tree **sp, PdaRun *pdaRun, ParseTree *parseTree )
{
	/* Reset. */
	assert( ! ( parseTree->flags & PF_LEFT_IL_ATTACHED ) );

	ParseTree *accum = pdaRun->accumIgnore;
	pdaRun->accumIgnore = 0;

	/* The data list needs to be extracted and reversed. The parse tree list
	 * can remain in stack order. */
	ParseTree *child = accum, *last = 0;
	Kid *dataChild = 0, *dataLast = 0;

	while ( child ) {
		dataChild = child->shadow;
		ParseTree *next = child->next;

		/* Reverse the lists. */
		dataChild->next = dataLast;
		child->next = last;

		/* Detach the parse tree from the data tree. */
		child->shadow = 0;

		/* Keep the last for reversal. */
		dataLast = dataChild;
		last = child;

		child = next;
	}

	/* Last is now the first. */
	parseTree->leftIgnore = last;

	if ( dataChild != 0 ) {
		debug( prg, REALM_PARSE, "attaching left ignore\n" );

		Kid *ignoreKid = dataChild;

		/* Make the ignore list for the left-ignore. */
		Tree *leftIgnore = treeAllocate( prg );
		leftIgnore->id = LEL_ID_IGNORE;
		leftIgnore->child = ignoreKid;

		Tree *pushTo = parseTree->shadow->tree;

		pushTo = pushLeftIgnore( prg, pushTo, leftIgnore );

		parseTree->shadow->tree = pushTo;

		parseTree->flags |= PF_LEFT_IL_ATTACHED;
	}
}

/* Not currently used. Need to revive this. WARNING: untested changes here */
static void detachRightIgnore( Program *prg, Tree **sp, PdaRun *pdaRun, ParseTree *parseTree )
{
	/* Right ignore are immediately discarded since they are copies of
	 * left-ignores. */
	Tree *rightIgnore = 0;
	if ( parseTree->flags & PF_RIGHT_IL_ATTACHED ) {
		Tree *popFrom = parseTree->shadow->tree;

		popFrom = popRightIgnore( prg, sp, popFrom, &rightIgnore );

		parseTree->shadow->tree = popFrom;

		parseTree->flags &= ~PF_RIGHT_IL_ATTACHED;
	}

	if ( parseTree->rightIgnore != 0 ) {
		assert( rightIgnore != 0 );

		/* Transfer the trees to accumIgnore. */
		ParseTree *ignore = parseTree->rightIgnore;
		parseTree->rightIgnore = 0;

		Kid *dataIgnore = rightIgnore->child;
		rightIgnore->child = 0;

		ParseTree *last = 0;
		Kid *dataLast = 0;
		while ( ignore != 0 ) {
			ParseTree *next = ignore->next;
			Kid *dataNext = dataIgnore->next;

			/* Put the data trees underneath the parse trees. */
			ignore->shadow = dataIgnore;

			/* Reverse. */
			ignore->next = last;
			dataIgnore->next = dataLast;

			/* Keep last for reversal. */
			last = ignore;
			dataLast = dataIgnore;

			ignore = next;
			dataIgnore = dataNext;
		}

		pdaRun->accumIgnore = last;

		treeDownref( prg, sp, rightIgnore );
	}
}

static void detachLeftIgnore( Program *prg, Tree **sp, PdaRun *pdaRun, FsmRun *fsmRun, ParseTree *parseTree )
{
	/* Detach left. */
	Tree *leftIgnore = 0;
	if ( parseTree->flags & PF_LEFT_IL_ATTACHED ) {
		Tree *popFrom = parseTree->shadow->tree;

		popFrom = popLeftIgnore( prg, sp, popFrom, &leftIgnore );

		parseTree->shadow->tree = popFrom;

		parseTree->flags &= ~PF_LEFT_IL_ATTACHED;
	}

	if ( parseTree->leftIgnore != 0 ) {
		assert( leftIgnore != 0 );

		/* Transfer the trees to accumIgnore. */
		ParseTree *ignore = parseTree->leftIgnore;
		parseTree->leftIgnore = 0;

		Kid *dataIgnore = leftIgnore->child;
		leftIgnore->child = 0;

		ParseTree *last = 0;
		Kid *dataLast = 0;
		while ( ignore != 0 ) {
			ParseTree *next = ignore->next;
			Kid *dataNext = dataIgnore->next;

			/* Put the data trees underneath the parse trees. */
			ignore->shadow = dataIgnore;

			/* Reverse. */
			ignore->next = last;
			dataIgnore->next = dataLast;

			/* Keep last for reversal. */
			last = ignore;
			dataLast = dataIgnore;

			ignore = next;
			dataIgnore = dataNext;
		}

		pdaRun->accumIgnore = last;
	}

	treeDownref( prg, sp, leftIgnore );
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
			debug( prg, REALM_PARSE, "stopping the parse\n" );
			pdaRun->stopParsing = true;
		}
	}
}

static Head *extractMatch( Program *prg, FsmRun *fsmRun, StreamImpl *is )
{
	long length = fsmRun->toklen;

	//debug( prg, REALM_PARSE, "extracting token of length: %ld\n", length );

	RunBuf *runBuf = fsmRun->consumeBuf;
	if ( runBuf == 0 || length > ( FSM_BUFSIZE - runBuf->length ) ) {
		runBuf = newRunBuf();
		runBuf->next = fsmRun->consumeBuf;
		fsmRun->consumeBuf = runBuf;
	}

	char *dest = runBuf->data + runBuf->length;

	is->funcs->getData( is, dest, length );
	Location *location = locationAllocate( prg );
	is->funcs->consumeData( is, length, location );

	runBuf->length += length;

	fsmRun->p = fsmRun->pe = 0;
	fsmRun->toklen = 0;
	fsmRun->tokstart = 0;

	Head *head = stringAllocPointer( prg, dest, length );

	head->location = location;

	debug( prg, REALM_PARSE, "location byte: %d\n", is->byte );

	return head;
}

static Head *peekMatch( Program *prg, FsmRun *fsmRun, StreamImpl *is )
{
	long length = fsmRun->toklen;

	RunBuf *runBuf = fsmRun->consumeBuf;
	if ( runBuf == 0 || length > ( FSM_BUFSIZE - runBuf->length ) ) {
		runBuf = newRunBuf();
		runBuf->next = fsmRun->consumeBuf;
		fsmRun->consumeBuf = runBuf;
	}

	char *dest = runBuf->data + runBuf->length;

	is->funcs->getData( is, dest, length );

	fsmRun->p = fsmRun->pe = 0;
	fsmRun->toklen = 0;

	Head *head = stringAllocPointer( prg, dest, length );

	head->location = locationAllocate( prg );
	head->location->line = is->line;
	head->location->column = is->column;
	head->location->byte = is->byte;

	debug( prg, REALM_PARSE, "location byte: %d\n", is->byte );

	return head;
}


static void sendIgnore( Program *prg, Tree **sp, StreamImpl *is, FsmRun *fsmRun, PdaRun *pdaRun, long id )
{
	debug( prg, REALM_PARSE, "ignoring: %s\n", prg->rtd->lelInfo[id].name );

	/* Make the ignore string. */
	Head *ignoreStr = extractMatch( prg, fsmRun, is );

	debug( prg, REALM_PARSE, "ignoring: %.*s\n", ignoreStr->length, ignoreStr->data );

	Tree *tree = treeAllocate( prg );
	tree->refs = 1;
	tree->id = id;
	tree->tokdata = ignoreStr;

	/* Send it to the pdaRun. */
	ignoreTree( prg, fsmRun, pdaRun, tree );
}


static void sendToken( Program *prg, Tree **sp, StreamImpl *is, FsmRun *fsmRun, PdaRun *pdaRun, long id )
{
	int emptyIgnore = pdaRun->accumIgnore == 0;

	/* Make the token data. */
	Head *tokdata = extractMatch( prg, fsmRun, is );

	debug( prg, REALM_PARSE, "token: %s  text: %.*s\n",
		prg->rtd->lelInfo[id].name,
		stringLength(tokdata), stringData(tokdata) );

	Kid *input = makeTokenWithData( prg, pdaRun, fsmRun, is, id, tokdata );

	incrementSteps( pdaRun );

	ParseTree *parseTree = parseTreeAllocate( prg );
	parseTree->id = input->tree->id;
	parseTree->shadow = input;
		
	pdaRun->parseInput = parseTree;

	/* Store any alternate scanning region. */
	if ( input != 0 && pdaRun->cs >= 0 )
		setRegion( pdaRun, emptyIgnore, parseTree );
}

static void sendTree( Program *prg, Tree **sp, PdaRun *pdaRun, FsmRun *fsmRun, StreamImpl *is )
{
	Kid *input = kidAllocate( prg );
	input->tree = is->funcs->consumeTree( is );

	incrementSteps( pdaRun );

	ParseTree *parseTree = parseTreeAllocate( prg );
	parseTree->id = input->tree->id;
	parseTree->flags |= PF_ARTIFICIAL;
	parseTree->shadow = input;
	
	pdaRun->parseInput = parseTree;
}

static void sendIgnoreTree( Program *prg, Tree **sp, PdaRun *pdaRun, FsmRun *fsmRun, StreamImpl *is )
{
	Tree *tree = is->funcs->consumeTree( is );
	ignoreTree2( prg, pdaRun, tree );
}

static void sendCi( Program *prg, Tree **sp, StreamImpl *is, FsmRun *fsmRun, PdaRun *pdaRun, int id )
{
	debug( prg, REALM_PARSE, "token: CI\n" );

	int emptyIgnore = pdaRun->accumIgnore == 0;

	/* Make the token data. */
	Head *tokdata = headAllocate( prg );
	tokdata->location = locationAllocate( prg );
	tokdata->location->line = is->line;
	tokdata->location->column = is->column;
	tokdata->location->byte = is->byte;

	debug( prg, REALM_PARSE, "token: %s  text: %.*s\n",
		prg->rtd->lelInfo[id].name,
		stringLength(tokdata), stringData(tokdata) );

	Kid *input = makeTokenWithData( prg, pdaRun, fsmRun, is, id, tokdata );

	incrementSteps( pdaRun );

	ParseTree *parseTree = parseTreeAllocate( prg );
	parseTree->id = input->tree->id;
	parseTree->shadow = input;

	pdaRun->parseInput = parseTree;

	/* Store any alternate scanning region. */
	if ( input != 0 && pdaRun->cs >= 0 )
		setRegion( pdaRun, emptyIgnore, parseTree );
}


static void sendEof( Program *prg, Tree **sp, StreamImpl *is, FsmRun *fsmRun, PdaRun *pdaRun )
{
	debug( prg, REALM_PARSE, "token: _EOF\n" );

	incrementSteps( pdaRun );

	Head *head = headAllocate( prg );
	head->location = locationAllocate( prg );
	head->location->line = is->line;
	head->location->column = is->column;
	head->location->byte = is->byte;

	Kid *input = kidAllocate( prg );
	input->tree = treeAllocate( prg );

	input->tree->refs = 1;
	input->tree->id = prg->rtd->eofLelIds[pdaRun->parserId];
	input->tree->tokdata = head;

	/* Set the state using the state of the parser. */
	fsmRun->region = pdaRunGetNextRegion( pdaRun, 0 );
	fsmRun->preRegion = pdaRunGetNextPreRegion( pdaRun );
	fsmRun->cs = fsmRun->tables->entryByRegion[fsmRun->region];

	ParseTree *parseTree = parseTreeAllocate( prg );
	parseTree->id = input->tree->id;
	parseTree->shadow = input;
	
	pdaRun->parseInput = parseTree;
}

static void newToken( Program *prg, PdaRun *pdaRun, FsmRun *fsmRun )
{
	fsmRun->p = fsmRun->pe = 0;
	fsmRun->toklen = 0;
	fsmRun->eof = 0;

	/* Init the scanner vars. */
	fsmRun->act = 0;
	fsmRun->tokstart = 0;
	fsmRun->tokend = 0;
	fsmRun->matchedToken = 0;

	/* Set the state using the state of the parser. */
	fsmRun->region = pdaRunGetNextRegion( pdaRun, 0 );
	fsmRun->preRegion = pdaRunGetNextPreRegion( pdaRun );
	if ( fsmRun->preRegion > 0 ) {
		fsmRun->cs = fsmRun->tables->entryByRegion[fsmRun->preRegion];
		fsmRun->ncs = fsmRun->tables->entryByRegion[fsmRun->region];
	}
	else {
		fsmRun->cs = fsmRun->tables->entryByRegion[fsmRun->region];
	}


	/* Clear the mark array. */
	memset( fsmRun->mark, 0, sizeof(fsmRun->mark) );
}

static void pushBtPoint( Program *prg, PdaRun *pdaRun )
{
	Tree *tree = 0;
	if ( pdaRun->accumIgnore != 0 ) 
		tree = pdaRun->accumIgnore->shadow->tree;
	else if ( pdaRun->tokenList != 0 )
		tree = pdaRun->tokenList->kid->tree;

	if ( tree != 0 ) {
		debug( prg, REALM_PARSE, "pushing bt point with location byte %d\n", 
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

long scanToken( Program *prg, PdaRun *pdaRun, FsmRun *fsmRun, StreamImpl *is )
{
	if ( pdaRun->triggerUndo )
		return SCAN_UNDO;

	while ( true ) {
		char *pd = 0;
		int len = 0;
		int type = is->funcs->getParseBlock( is, fsmRun->toklen, &pd, &len );

		switch ( type ) {
			case INPUT_DATA:
				fsmRun->p = pd;
				fsmRun->pe = pd + len;
				break;

			case INPUT_EOS:
				fsmRun->p = fsmRun->pe = 0;
				if ( fsmRun->tokstart != 0 )
					fsmRun->eof = 1;
				debug( prg, REALM_SCAN, "EOS *******************\n" );
				break;

			case INPUT_EOF:
				fsmRun->p = fsmRun->pe = 0;
				if ( fsmRun->tokstart != 0 )
					fsmRun->eof = 1;
				else 
					return SCAN_EOF;
				break;

			case INPUT_EOD:
				fsmRun->p = fsmRun->pe = 0;
				return SCAN_TRY_AGAIN_LATER;

			case INPUT_LANG_EL:
				if ( fsmRun->tokstart != 0 )
					fsmRun->eof = 1;
				else 
					return SCAN_LANG_EL;
				break;

			case INPUT_TREE:
				if ( fsmRun->tokstart != 0 )
					fsmRun->eof = 1;
				else 
					return SCAN_TREE;
				break;
			case INPUT_IGNORE:
				if ( fsmRun->tokstart != 0 )
					fsmRun->eof = 1;
				else
					return SCAN_IGNORE;
				break;
		}

		prg->rtd->fsmExecute( fsmRun, is );

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
				fsmRun->toklen = 0;
				return prg->rtd->regionInfo[fsmRun->region].defaultToken;
			}

			return SCAN_ERROR;
		}

		/* Got here because the state machine didn't match a token or encounter
		 * an error. Must be because we got to the end of the buffer data. */
		assert( fsmRun->p == fsmRun->pe );
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
		StreamImpl *is, long entry )
{
	FsmRun *fsmRun = pdaRun->fsmRun;
	LangElInfo *lelInfo = prg->rtd->lelInfo;

switch ( entry ) {
case PcrStart:

	pdaRun->stop = false;

	while ( true ) {
		debug( prg, REALM_PARSE, "parse loop start %d:%d\n", is->line, is->column );

		/* Pull the current scanner from the parser. This can change during
		 * parsing due to inputStream pushes, usually for the purpose of includes.
		 * */
		pdaRun->tokenId = scanToken( prg, pdaRun, fsmRun, is );

		if ( pdaRun->tokenId == SCAN_ERROR ) {
			if ( fsmRun->preRegion >= 0 ) {
				fsmRun->preRegion = -1;
				fsmRun->cs = fsmRun->ncs;
				continue;
			}
		}

		if ( pdaRun->tokenId == SCAN_ERROR &&
				( prg->rtd->regionInfo[fsmRun->region].ciLelId > 0 ) )
		{
			debug( prg, REALM_PARSE, "sending a collect ignore\n" );
			sendCi( prg, sp, is, fsmRun, pdaRun, prg->rtd->regionInfo[fsmRun->region].ciLelId );
			goto yes;
		}

		if ( pdaRun->tokenId == SCAN_TRY_AGAIN_LATER ) {
			debug( prg, REALM_PARSE, "scanner says try again later\n" );
			break;
		}

		assert( pdaRun->parseInput == 0 );
		pdaRun->parseInput = 0;

		/* Check for EOF. */
		if ( pdaRun->tokenId == SCAN_EOF ) {
			is->eofSent = true;
			sendEof( prg, sp, is, fsmRun, pdaRun );

			pdaRun->frameId = prg->rtd->regionInfo[fsmRun->region].eofFrameId;

			if ( prg->ctxDepParsing && pdaRun->frameId >= 0 ) {
				debug( prg, REALM_PARSE, "HAVE PRE_EOF BLOCK\n" );

				pdaRun->fi = &prg->rtd->frameInfo[pdaRun->frameId];
				pdaRun->code = pdaRun->fi->codeWV;

return PcrPreEof;
case PcrPreEof:
				makeReverseCode( pdaRun );
			}
		}
		else if ( pdaRun->tokenId == SCAN_UNDO ) {
			/* Fall through with parseInput = 0. FIXME: Do we need to send back ignore? */
			debug( prg, REALM_PARSE, "invoking undo from the scanner\n" );
		}
		else if ( pdaRun->tokenId == SCAN_ERROR ) {
			/* Scanner error, maybe retry. */
			if ( pdaRun->accumIgnore == 0 && pdaRunGetNextRegion( pdaRun, 1 ) != 0 ) {
				debug( prg, REALM_PARSE, "scanner failed, trying next region\n" );

				pdaRun->nextRegionInd += 1;
				goto skipSend;
			}
			else if ( pdaRun->numRetry > 0 ) {
				debug( prg, REALM_PARSE, "invoking parse error from the scanner\n" );

				/* Fall through to send null (error). */
				pushBtPoint( prg, pdaRun );
			}
			else {
				debug( prg, REALM_PARSE, "no alternate scanning regions\n" );

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
			debug( prg, REALM_PARSE, "sending an named lang el\n" );

			/* A named language element (parsing colm program). */
			prg->rtd->sendNamedLangEl( prg, sp, pdaRun, fsmRun, is );
		}
		else if ( pdaRun->tokenId == SCAN_TREE ) {
			debug( prg, REALM_PARSE, "sending a tree\n" );

			/* A tree already built. */
			sendTree( prg, sp, pdaRun, fsmRun, is );
		}
		else if ( pdaRun->tokenId == SCAN_IGNORE ) {
			debug( prg, REALM_PARSE, "sending an ignore token\n" );

			/* A tree to ignore. */
			sendIgnoreTree( prg, sp, pdaRun, fsmRun, is );
			goto skipSend;
		}
		else if ( prg->ctxDepParsing && lelInfo[pdaRun->tokenId].frameId >= 0 ) {
			/* Has a generation action. */
			debug( prg, REALM_PARSE, "token gen action: %s\n", 
					prg->rtd->lelInfo[pdaRun->tokenId].name );

			/* Make the token data. */
			pdaRun->tokdata = peekMatch( prg, fsmRun, is );

			/* Note that we don't update the position now. It is done when the token
			 * data is pulled from the inputStream. */

			fsmRun->p = fsmRun->pe = 0;
			fsmRun->toklen = 0;
			fsmRun->eof = 0;

			pdaRun->fi = &prg->rtd->frameInfo[prg->rtd->lelInfo[pdaRun->tokenId].frameId];
			pdaRun->frameId = prg->rtd->lelInfo[pdaRun->tokenId].frameId;
			pdaRun->code = pdaRun->fi->codeWV;
			
return PcrGeneration;
case PcrGeneration:

			makeReverseCode( pdaRun );

			/* Finished with the match text. */
			stringFree( prg, pdaRun->tokdata );

			goto skipSend;
		}
		else if ( lelInfo[pdaRun->tokenId].ignore ) {
			debug( prg, REALM_PARSE, "sending an ignore token: %s\n", 
					prg->rtd->lelInfo[pdaRun->tokenId].name );

			/* Is an ignore token. */
			sendIgnore( prg, sp, is, fsmRun, pdaRun, pdaRun->tokenId );
			goto skipSend;
		}
		else {
			debug( prg, REALM_PARSE, "sending an a plain old token: %s\n", 
					prg->rtd->lelInfo[pdaRun->tokenId].name );

			/* Is a plain token. */
			sendToken( prg, sp, is, fsmRun, pdaRun, pdaRun->tokenId );
		}
yes:

		if ( pdaRun->parseInput != 0 )
			transferReverseCode( pdaRun, pdaRun->parseInput );

		if ( pdaRun->parseInput != 0 ) {
			/* If it's a nonterminal with a termdup then flip the parse tree to the terminal. */
			if ( pdaRun->parseInput->id >= prg->rtd->firstNonTermId ) {
				pdaRun->parseInput->id = prg->rtd->lelInfo[pdaRun->parseInput->id].termDupId;
				pdaRun->parseInput->flags |= PF_TERM_DUP;
			}
		}

		long pcr = parseToken( prg, sp, pdaRun, fsmRun, is, PcrStart );
		
		while ( pcr != PcrDone ) {

return pcr;
case PcrReduction:
case PcrReverse:

			pcr = parseToken( prg, sp, pdaRun, fsmRun, is, entry );
		}

		assert( pcr == PcrDone );

		handleError( prg, sp, pdaRun );

skipSend:
		newToken( prg, pdaRun, fsmRun );

		/* Various stop conditions. This should all be coverned by one test
		 * eventually. */

		if ( pdaRun->triggerUndo ) {
			debug( prg, REALM_PARSE, "parsing stopped by triggerUndo\n" );
			break;
		}

		if ( is->eofSent ) {
			debug( prg, REALM_PARSE, "parsing stopped by EOF\n" );
			break;
		}

		if ( pdaRun->stopParsing ) {
			debug( prg, REALM_PARSE, "scanner has been stopped\n" );
			break;
		}

		if ( pdaRun->stop ) {
			debug( prg, REALM_PARSE, "parsing has been stopped by consumedCount\n" );
			break;
		}

		if ( prg->induceExit ) {
			debug( prg, REALM_PARSE, "parsing has been stopped by a call to exit\n" );
			break;
		}

		if ( pdaRun->parseError ) {
			debug( prg, REALM_PARSE, "parsing stopped by a parse error\n" );
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

int pdaRunGetNextPreRegion( PdaRun *pdaRun )
{
	return pdaRun->tables->tokenPreRegions[pdaRun->nextRegionInd];
}

Tree *getParsedRoot( PdaRun *pdaRun, int stop )
{
	if ( pdaRun->parseError )
		return 0;
	else if ( stop ) {
		if ( pdaRun->stackTop->shadow != 0 )
			return pdaRun->stackTop->shadow->tree;
	}
	else {
		if ( pdaRun->stackTop->next->shadow != 0 )
			return pdaRun->stackTop->next->shadow->tree;
	}
	return 0;
}

void clearParseTree( Program *prg, Tree **sp, ParseTree *pt )
{
	Tree **top = vm_ptop();

	if ( pt == 0 )
		return;

free_tree:
	if ( pt->next != 0 ) {
		vm_push( (Tree*)pt->next );
	}

	if ( pt->leftIgnore != 0 ) {
		vm_push( (Tree*)pt->leftIgnore );
	}

	if ( pt->child != 0 ) {
		vm_push( (Tree*)pt->child );
	}

	if ( pt->rightIgnore != 0 ) {
		vm_push( (Tree*)pt->rightIgnore );
	}

	if ( pt->shadow != 0 ) {
		treeDownref( prg, sp, pt->shadow->tree );
		kidFree( prg, pt->shadow );
	}

	parseTreeFree( prg, pt );

	/* Any trees to downref? */
	if ( sp != top ) {
		pt = (ParseTree*)vm_pop();
		goto free_tree;
	}
}

void clearPdaRun( Program *prg, Tree **sp, PdaRun *pdaRun )
{
	clearFsmRun( prg, pdaRun->fsmRun );

	/* Remaining stack and parse trees underneath. */
	clearParseTree( prg, sp, pdaRun->stackTop );
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
	clearParseTree( prg, sp, pdaRun->accumIgnore );
	pdaRun->accumIgnore = 0;

	if ( pdaRun->context != 0 )
		treeDownref( prg, sp, pdaRun->context );

	rcodeDownrefAll( prg, sp, &pdaRun->reverseCode );
	rtCodeVectEmpty( &pdaRun->reverseCode );
	rtCodeVectEmpty( &pdaRun->rcodeCollect );

	treeDownref( prg, sp, pdaRun->parseErrorText );
}

int isParserStopFinished( PdaRun *pdaRun )
{
	int done = 
			pdaRun->stackTop->next != 0 && 
			pdaRun->stackTop->next->next == 0 &&
			pdaRun->stackTop->id == pdaRun->stopTarget;
	return done;
}

void initPdaRun( Program *prg, PdaRun *pdaRun, PdaTables *tables,
		int parserId, long stopTarget, int revertOn, Tree *context )
{
	memset( pdaRun, 0, sizeof(PdaRun) );

	pdaRun->tables = tables;
	pdaRun->parserId = parserId;
	pdaRun->stopTarget = stopTarget;
	pdaRun->revertOn = revertOn;
	pdaRun->targetSteps = -1;

	debug( prg, REALM_PARSE, "initializing PdaRun\n" );

	/* FIXME: need the right one here. */
	pdaRun->cs = prg->rtd->startStates[pdaRun->parserId];

	Kid *sentinal = kidAllocate( prg );
	sentinal->tree = treeAllocate( prg );
	sentinal->tree->refs = 1;

	/* Init the element allocation variables. */
	pdaRun->stackTop = parseTreeAllocate( prg );
	pdaRun->stackTop->state = -1;
	pdaRun->stackTop->shadow = sentinal;

	pdaRun->numRetry = 0;
	pdaRun->nextRegionInd = pdaRun->tables->tokenRegionInds[pdaRun->cs];
	pdaRun->stopParsing = false;
	pdaRun->accumIgnore = 0;
	pdaRun->btPoint = 0;
	pdaRun->checkNext = false;
	pdaRun->checkStop = false;

	prg->rtd->initBindings( pdaRun );

	initRtCodeVect( &pdaRun->reverseCode );
	initRtCodeVect( &pdaRun->rcodeCollect );

	pdaRun->context = splitTree( prg, context );
	pdaRun->parseError = 0;
	pdaRun->parseInput = 0;
	pdaRun->triggerUndo = 0;

	pdaRun->tokenId = 0;

	pdaRun->onDeck = false;
	pdaRun->parsed = 0;
	pdaRun->reject = false;

	pdaRun->rcBlockCount = 0;

	pdaRun->fsmRun = &pdaRun->_fsmRun;
	initFsmRun( prg, pdaRun->fsmRun );
	newToken( prg, pdaRun, pdaRun->fsmRun );
}

long stackTopTarget( Program *prg, PdaRun *pdaRun )
{
	long state;
	if ( pdaRun->stackTop->state < 0 )
		state = prg->rtd->startStates[pdaRun->parserId];
	else {
		state = pdaRun->tables->targs[(int)pdaRun->tables->indicies[pdaRun->tables->offsets[
				pdaRun->stackTop->state] + 
				(pdaRun->stackTop->id - pdaRun->tables->keys[pdaRun->stackTop->state<<1])]];
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

int beenCommitted( ParseTree *parseTree )
{
	return parseTree->flags & PF_COMMITTED;
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
void commitKid( Program *prg, PdaRun *pdaRun, Tree **root, ParseTree *lel, Code **rcode, long *causeReduce )
{
	ParseTree *tree = 0;
	Tree **sp = root;
	//Tree *restore = 0;

head:
	/* Commit */
	debug( prg, REALM_PARSE, "commit: visiting %s\n",
			prg->rtd->lelInfo[lel->id].name );

	/* Load up the parsed tree. */
	tree = lel;

	/* Check for reverse code. */
	//restore = 0;
	if ( tree->flags & PF_HAS_RCODE ) {
		/* If tree caused some reductions, now is not the right time to backup
		 * over the reverse code. We need to backup over the reductions first. Store
		 * the count of the reductions and do it when the count drops to zero. */
		if ( tree->causeReduce > 0 ) {
			/* The top reduce block does not correspond to this alg. */
			debug( prg, REALM_PARSE, "commit: causeReduce found, delaying backup: %ld\n",
						(long)tree->causeReduce );
			*causeReduce = tree->causeReduce;
		}
		else {
			*rcode = backupOverRcode( *rcode );

			//if ( **rcode == IN_RESTORE_LHS ) {
			//	debug( prg, REALM_PARSE, "commit: has restore_lhs\n" );
			//	read_tree_p( restore, (*rcode+1) );
			//}
		}
	}

	//FIXME: what was this about?
	//if ( restore != 0 )
	//	tree = restore;

	/* All the parse algorithm data except for the RCODE flag is in the
	 * original. That is why we restore first, then we can clear the retry
	 * values. */

	/* Check causeReduce, might be time to backup over the reverse code
	 * belonging to a nonterminal that caused previous reductions. */
	if ( *causeReduce > 0 && 
			tree->id >= prg->rtd->firstNonTermId &&
			!(tree->flags & PF_TERM_DUP) )
	{
		*causeReduce -= 1;

		if ( *causeReduce == 0 ) {
			debug( prg, REALM_PARSE, "commit: causeReduce dropped to zero, backing up over rcode\n" );

			/* Cause reduce just dropped down to zero. */
			*rcode = backupOverRcode( *rcode );
		}
	}

	///* FIXME: why was this here?
	// * Reset retries. */
	//if ( tree->flags & AF_PARSED ) {
	//	if ( tree->retryLower > 0 ) {
	//		pdaRun->numRetry -= 1;
	//		tree->retryLower = 0;
	//	}
	//	if ( tree->retryUpper > 0 ) {
	//		pdaRun->numRetry -= 1;
	//		tree->retryUpper = 0;
	//	}
	//}

	tree->flags |= PF_COMMITTED;

	/* Do not recures on trees that are terminal dups. */
	if ( !(tree->flags & PF_TERM_DUP) && 
			!(tree->flags & PF_NAMED) && 
			!(tree->flags & PF_ARTIFICIAL) && 
			tree->child != 0 )
	{
		vm_push( (Tree*)lel );
		lel = tree->child;

		if ( lel != 0 ) {
			while ( lel != 0 ) {
				vm_push( (Tree*)lel );
				lel = lel->next;
			}
		}
	}

backup:
	if ( sp != root ) {
		ParseTree *next = (ParseTree*)vm_pop();
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
	debug( prg, REALM_PARSE, "running full commit\n" );
	
	ParseTree *parseTree = pdaRun->stackTop;
	Code *rcode = pdaRun->reverseCode.data + pdaRun->reverseCode.tabLen;

	/* The top level of the stack is linked right to left. This is the
	 * traversal order we need for committing. */
	while ( parseTree != 0 && !beenCommitted( parseTree ) ) {
		commitKid( prg, pdaRun, sp, parseTree, &rcode, &causeReduce );
		parseTree = parseTree->next;
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
		FsmRun *fsmRun, StreamImpl *is, long entry )
{
	int pos;
	unsigned int *action;
	int rhsLen;
	int owner;
	int induceReject;
	int indPos;
	//LangElInfo *lelInfo = prg->rtd->lelInfo;

switch ( entry ) {
case PcrStart:

	/* The scanner will send a null token if it can't find a token. */
	if ( pdaRun->parseInput == 0 )
		goto parseError;

	/* This will cause parseInput to be lost. This 
	 * path should be traced. */
	if ( pdaRun->cs < 0 )
		return PcrDone;

	/* Record the state in the parse tree. */
	pdaRun->parseInput->state = pdaRun->cs;

again:
	if ( pdaRun->parseInput == 0 )
		goto _out;

	pdaRun->lel = pdaRun->parseInput;
	pdaRun->curState = pdaRun->cs;

	if ( pdaRun->lel->id < pdaRun->tables->keys[pdaRun->curState<<1] ||
			pdaRun->lel->id > pdaRun->tables->keys[(pdaRun->curState<<1)+1] ) {
		debug( prg, REALM_PARSE, "parse error, no transition 1\n" );
		pushBtPoint( prg, pdaRun );
		goto parseError;
	}

	indPos = pdaRun->tables->offsets[pdaRun->curState] + 
		(pdaRun->lel->id - pdaRun->tables->keys[pdaRun->curState<<1]);

	owner = pdaRun->tables->owners[indPos];
	if ( owner != pdaRun->curState ) {
		debug( prg, REALM_PARSE, "parse error, no transition 2\n" );
		pushBtPoint( prg, pdaRun );
		goto parseError;
	}

	pos = pdaRun->tables->indicies[indPos];
	if ( pos < 0 ) {
		debug( prg, REALM_PARSE, "parse error, no transition 3\n" );
		pushBtPoint( prg, pdaRun );
		goto parseError;
	}

	/* Checking complete. */

	induceReject = false;
	pdaRun->cs = pdaRun->tables->targs[pos];
	action = pdaRun->tables->actions + pdaRun->tables->actInds[pos];
	if ( pdaRun->lel->retryLower )
		action += pdaRun->lel->retryLower;

	/*
	 * Shift
	 */

	if ( *action & act_sb ) {
		debug( prg, REALM_PARSE, "shifted: %s\n", 
				prg->rtd->lelInfo[pdaRun->lel->id].name );
		/* Consume. */
		pdaRun->parseInput = pdaRun->parseInput->next;

		pdaRun->lel->state = pdaRun->curState;

		/* If its a token then attach ignores and record it in the token list
		 * of the next ignore attachment to use. */
		if ( pdaRun->lel->id < prg->rtd->firstNonTermId ) {
			if ( pdaRun->lel->causeReduce == 0 )
				attachRightIgnore( prg, sp, pdaRun, pdaRun->stackTop );
		}

		pdaRun->lel->next = pdaRun->stackTop;
		pdaRun->stackTop = pdaRun->lel;

		/* If its a token then attach ignores and record it in the token list
		 * of the next ignore attachment to use. */
		if ( pdaRun->lel->id < prg->rtd->firstNonTermId ) {
			attachLeftIgnore( prg, sp, pdaRun, pdaRun->lel );

			Ref *ref = (Ref*)kidAllocate( prg );
			ref->kid = pdaRun->lel->shadow;
			//treeUpref( pdaRun->tree );
			ref->next = pdaRun->tokenList;
			pdaRun->tokenList = ref;
		}

		if ( action[1] == 0 )
			pdaRun->lel->retryLower = 0;
		else {
			debug( prg, REALM_PARSE, "retry: %p\n", pdaRun->stackTop );
			pdaRun->lel->retryLower += 1;
			assert( pdaRun->lel->retryUpper == 0 );
			/* FIXME: Has the retry already been counted? */
			pdaRun->numRetry += 1; 
		}
	}

	/* 
	 * Commit
	 */

	if ( pdaRun->tables->commitLen[pos] != 0 ) {
		long causeReduce = 0;
		if ( pdaRun->parseInput != 0 ) { 
			if ( pdaRun->parseInput->flags & PF_HAS_RCODE )
				causeReduce = pdaRun->parseInput->causeReduce;
		}
		commitFull( prg, sp, pdaRun, causeReduce );
	}

	/*
	 * Reduce
	 */

	if ( *action & act_rb ) {
		int r, objectLength;
		ParseTree *last, *child;
		Kid *attrs;
		Kid *dataLast, *dataChild;

		/* If there was shift don't attach again. */
		if ( !( *action & act_sb ) && pdaRun->lel->id < prg->rtd->firstNonTermId )
			attachRightIgnore( prg, sp, pdaRun, pdaRun->stackTop );

		pdaRun->reduction = *action >> 2;

		if ( pdaRun->parseInput != 0 )
			pdaRun->parseInput->causeReduce += 1;

		Kid *value = kidAllocate( prg );
		value->tree = treeAllocate( prg );
		value->tree->refs = 1;
		value->tree->id = prg->rtd->prodInfo[pdaRun->reduction].lhsId;
		value->tree->prodNum = prg->rtd->prodInfo[pdaRun->reduction].prodNum;

		pdaRun->redLel = parseTreeAllocate( prg );
		pdaRun->redLel->id = prg->rtd->prodInfo[pdaRun->reduction].lhsId;
		pdaRun->redLel->next = 0;
		pdaRun->redLel->causeReduce = 0;
		pdaRun->redLel->retryLower = 0;
		pdaRun->redLel->shadow = value;

		/* Transfer. */
		pdaRun->redLel->retryUpper = pdaRun->lel->retryLower;
		pdaRun->lel->retryLower = 0;

		/* Allocate the attributes. */
		objectLength = prg->rtd->lelInfo[pdaRun->redLel->id].objectLength;
		attrs = allocAttrs( prg, objectLength );

		/* Build the list of children. We will be giving up a reference when we
		 * detach parse tree and data tree, but gaining the reference when we
		 * put the children under the new data tree. No need to alter refcounts
		 * here. */
		rhsLen = prg->rtd->prodInfo[pdaRun->reduction].length;
		child = last = 0;
		dataChild = dataLast = 0;
		for ( r = 0; r < rhsLen; r++ ) {

			/* The child. */
			child = pdaRun->stackTop;
			dataChild = child->shadow;

			/* Pop. */
			pdaRun->stackTop = pdaRun->stackTop->next;

			/* Detach the parse tree from the data. */
			child->shadow = 0;

			/* Reverse list. */
			child->next = last;
			dataChild->next = dataLast;

			/* Track last for reversal. */
			last = child;
			dataLast = dataChild;
		}

		pdaRun->redLel->child = child;
		pdaRun->redLel->shadow->tree->child = kidListConcat( attrs, dataChild );

		debug( prg, REALM_PARSE, "reduced: %s rhsLen %d\n",
				prg->rtd->prodInfo[pdaRun->reduction].name, rhsLen );
		if ( action[1] == 0 )
			pdaRun->redLel->retryUpper = 0;
		else {
			pdaRun->redLel->retryUpper += 1;
			assert( pdaRun->lel->retryLower == 0 );
			pdaRun->numRetry += 1;
			debug( prg, REALM_PARSE, "retry: %p\n", pdaRun->redLel );
		}

		/* When the production is of zero length we stay in the same state.
		 * Otherwise we use the state stored in the first child. */
		pdaRun->cs = rhsLen == 0 ? pdaRun->curState : child->state;

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
				if ( pdaRun->parsed != pdaRun->redLel->shadow->tree ) {
					debug( prg, REALM_PARSE, "lhs tree was modified, adding a restore instruction\n" );
//
//					/* Make it into a parse tree. */
//					Tree *newPt = prepParseTree( prg, sp, pdaRun->redLel->tree );
//					treeDownref( prg, sp, pdaRun->redLel->tree );
//
//					/* Copy it in. */
//					pdaRun->redLel->tree = newPt;
//					treeUpref( pdaRun->redLel->tree );

					/* Add the restore instruct. */
					appendCode( &pdaRun->rcodeCollect, IN_RESTORE_LHS );
					appendWord( &pdaRun->rcodeCollect, (Word)pdaRun->parsed );
					appendCode( &pdaRun->rcodeCollect, SIZEOF_CODE + SIZEOF_WORD );
				}
				else {
					/* Not changed. Done with parsed. */
					treeDownref( prg, sp, pdaRun->parsed );
				}
				pdaRun->parsed = 0;
			}

			/* Pull out the reverse code, if any. */
			makeReverseCode( pdaRun );
			transferReverseCode( pdaRun, pdaRun->redLel );

			/* Perhaps the execution environment is telling us we need to
			 * reject the reduction. */
			induceReject = pdaRun->reject;
		}

		/* If the left hand side was replaced then the only parse algorithm
		 * data that is contained in it will the PF_HAS_RCODE flag. Everthing
		 * else will be in the original. This requires that we restore first
		 * when going backwards and when doing a commit. */

		if ( induceReject ) {
			debug( prg, REALM_PARSE, "error induced during reduction of %s\n",
					prg->rtd->lelInfo[pdaRun->redLel->id].name );
			pdaRun->redLel->state = pdaRun->curState;
			pdaRun->redLel->next = pdaRun->stackTop;
			pdaRun->stackTop = pdaRun->redLel;
			/* FIXME: What is the right argument here? */
			pushBtPoint( prg, pdaRun );
			goto parseError;
		}

		pdaRun->redLel->next = pdaRun->parseInput;
		pdaRun->parseInput = pdaRun->redLel;
	}

	goto again;

parseError:
	debug( prg, REALM_PARSE, "hit error, backtracking\n" );

	if ( pdaRun->numRetry == 0 ) {
		debug( prg, REALM_PARSE, "out of retries failing parse\n" );
		goto fail;
	}

	while ( 1 ) {
		if ( pdaRun->onDeck ) {
			debug( prg, REALM_BYTECODE, "dropping out for reverse code call\n" );

			pdaRun->frameId = -1;
			pdaRun->code = popReverseCode( &pdaRun->reverseCode );

return PcrReverse;
case PcrReverse: 

			decrementSteps( pdaRun );
		}
		else if ( pdaRun->checkNext ) {
			pdaRun->checkNext = false;

			if ( pdaRun->next > 0 && pdaRun->tables->tokenRegions[pdaRun->next] != 0 ) {
				debug( prg, REALM_PARSE, "found a new region\n" );
				pdaRun->numRetry -= 1;
				pdaRun->cs = stackTopTarget( prg, pdaRun );
				pdaRun->nextRegionInd = pdaRun->next;
				return PcrDone;
			}
		}
		else if ( pdaRun->checkStop ) {
			pdaRun->checkStop = false;

			if ( pdaRun->stop ) {
				debug( prg, REALM_PARSE, "stopping the backtracking, steps is %d\n", pdaRun->steps );

				pdaRun->cs = stackTopTarget( prg, pdaRun );
				goto _out;
			}
		}
		else if ( pdaRun->parseInput != 0 ) {
			/* Either we are dealing with a terminal that was
			 * shifted or a nonterminal that was reduced. */
			if ( pdaRun->parseInput->id < prg->rtd->firstNonTermId ) {
				assert( pdaRun->parseInput->retryUpper == 0 );

				if ( pdaRun->parseInput->retryLower != 0 ) {
					debug( prg, REALM_PARSE, "found retry targ: %p\n", pdaRun->parseInput );

					pdaRun->numRetry -= 1;
					pdaRun->cs = pdaRun->parseInput->state;
					goto again;
				}

				if ( pdaRun->parseInput->causeReduce != 0 ) {
					pdaRun->undoLel = pdaRun->stackTop;

					/* Check if we've arrived at the stack sentinal. This guard
					 * is here to allow us to initially set numRetry to one to
					 * cause the parser to backup all the way to the beginning
					 * when an error occurs. */
					if ( pdaRun->undoLel->next == 0 )
						break;

					/* Either we are dealing with a terminal that was
					 * shifted or a nonterminal that was reduced. */
					assert( !(pdaRun->stackTop->id < prg->rtd->firstNonTermId) );

					debug( prg, REALM_PARSE, "backing up over non-terminal: %s\n",
							prg->rtd->lelInfo[pdaRun->stackTop->id].name );

					/* Pop the item from the stack. */
					pdaRun->stackTop = pdaRun->stackTop->next;

					/* Queue it as next parseInput item. */
					pdaRun->undoLel->next = pdaRun->parseInput;
					pdaRun->parseInput = pdaRun->undoLel;
				}
				else {
					long region = pdaRun->parseInput->region;
					pdaRun->next = region > 0 ? region + 1 : 0;
					pdaRun->checkNext = true;
					pdaRun->checkStop = true;

					sendBack( prg, sp, pdaRun, fsmRun, is, pdaRun->parseInput );

					pdaRun->parseInput = 0;
				}
			}
			else if ( pdaRun->parseInput->flags & PF_HAS_RCODE ) {
				debug( prg, REALM_PARSE, "tree has rcode, setting on deck\n" );
				pdaRun->onDeck = true;
				pdaRun->parsed = 0;

				/* Only the RCODE flag was in the replaced lhs. All the rest is in
				 * the the original. We read it after restoring. */

				pdaRun->parseInput->flags &= ~PF_HAS_RCODE;
			}
			else {
				/* Remove it from the input queue. */
				pdaRun->undoLel = pdaRun->parseInput;
				pdaRun->parseInput = pdaRun->parseInput->next;

				/* Extract children from the child list. */
				ParseTree *first = pdaRun->undoLel->child;
				pdaRun->undoLel->child = 0;

				/* This will skip the ignores/attributes, etc. */
				Kid *dataFirst = treeExtractChild( prg, pdaRun->undoLel->shadow->tree );

				/* Walk the child list and and push the items onto the parsing
				 * stack one at a time. */
				while ( first != 0 ) {
					/* Get the next item ahead of time. */
					ParseTree *next = first->next;
					Kid *dataNext = dataFirst->next;

					/* Push onto the stack. */
					first->next = pdaRun->stackTop;
					pdaRun->stackTop = first;

					/* Reattach the data and the parse tree. */
					first->shadow = dataFirst;

					first = next;
					dataFirst = dataNext;
				}

				/* If there is an parseInput queued, this is one less reduction it has
				 * caused. */
				if ( pdaRun->parseInput != 0 )
					pdaRun->parseInput->causeReduce -= 1;

				if ( pdaRun->undoLel->retryUpper != 0 ) {
					/* There is always an parseInput item here because reduce
					 * conflicts only happen on a lookahead character. */
					assert( pdaRun->parseInput != pdaRun->undoLel );
					assert( pdaRun->parseInput != 0 );
					assert( pdaRun->undoLel->retryLower == 0 );
					assert( pdaRun->parseInput->retryUpper == 0 );

					/* Transfer the retry from undoLel to parseInput. */
					pdaRun->parseInput->retryLower = pdaRun->undoLel->retryUpper;
					pdaRun->parseInput->retryUpper = 0;
					pdaRun->parseInput->state = stackTopTarget( prg, pdaRun );
				}

				/* Free the reduced item. */
				treeDownref( prg, sp, pdaRun->undoLel->shadow->tree );
				kidFree( prg, pdaRun->undoLel->shadow );
				parseTreeFree( prg, pdaRun->undoLel );

				/* If the stacktop had right ignore attached, detach now. */
				if ( pdaRun->stackTop->flags & PF_RIGHT_IL_ATTACHED )
					detachRightIgnore( prg, sp, pdaRun, pdaRun->stackTop );
			}
		}
		else if ( pdaRun->accumIgnore != 0 ) {
			debug( prg, REALM_PARSE, "have accumulated ignore to undo\n" );

			/* Send back any accumulated ignore tokens, then trigger error
			 * in the the parser. */
			ParseTree *ignore = pdaRun->accumIgnore;
			pdaRun->accumIgnore = pdaRun->accumIgnore->next;
			ignore->next = 0;

			long region = ignore->region;
			pdaRun->next = region > 0 ? region + 1 : 0;
			pdaRun->checkNext = true;
			pdaRun->checkStop = true;
			
			sendBackIgnore( prg, sp, pdaRun, fsmRun, is, ignore );

			treeDownref( prg, sp, ignore->shadow->tree );
			kidFree( prg, ignore->shadow );
			parseTreeFree( prg, ignore );
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
			if ( pdaRun->stackTop->id < prg->rtd->firstNonTermId ) {
				debug( prg, REALM_PARSE, "backing up over effective terminal: %s\n",
							prg->rtd->lelInfo[pdaRun->stackTop->id].name );

				/* Pop the item from the stack. */
				pdaRun->stackTop = pdaRun->stackTop->next;

				/* Queue it as next parseInput item. */
				pdaRun->undoLel->next = pdaRun->parseInput;
				pdaRun->parseInput = pdaRun->undoLel;

				/* Pop from the token list. */
				Ref *ref = pdaRun->tokenList;
				pdaRun->tokenList = ref->next;
				kidFree( prg, (Kid*)ref );

				assert( pdaRun->accumIgnore == 0 );
				detachLeftIgnore( prg, sp, pdaRun, fsmRun, pdaRun->parseInput );
			}
			else {
				debug( prg, REALM_PARSE, "backing up over non-terminal: %s\n",
						prg->rtd->lelInfo[pdaRun->stackTop->id].name );

				/* Pop the item from the stack. */
				pdaRun->stackTop = pdaRun->stackTop->next;

				/* Queue it as next parseInput item. */
				pdaRun->undoLel->next = pdaRun->parseInput;
				pdaRun->parseInput = pdaRun->undoLel;
			}

			/* Undo attach of right ignore. */
			if ( pdaRun->stackTop->flags & PF_RIGHT_IL_ATTACHED )
				detachRightIgnore( prg, sp, pdaRun, pdaRun->stackTop );
		}
	}

fail:
	pdaRun->cs = -1;
	pdaRun->parseError = 1;

	/* If we failed parsing on tree we must free it. The caller expected us to
	 * either consume it or send it back to the parseInput. */
	if ( pdaRun->parseInput != 0 ) {
		//treeDownref( prg, sp, (Tree*)pdaRun->parseInput->tree );
		//ptKidFree( prg, pdaRun->parseInput );
		pdaRun->parseInput = 0;
	}

	/* FIXME: do we still need to fall through here? A fail is permanent now,
	 * no longer called into again. */

	return PcrDone;

_out:
	pdaRun->nextRegionInd = pdaRun->tables->tokenRegionInds[pdaRun->cs];

case PcrDone:
break; }

	return PcrDone;
}
