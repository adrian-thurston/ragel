/*
 *  Copyright 2007, 2008 Adrian Thurston <thurston@complang.org>
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

#include "input.h"
#include "fsmrun.h"
#include "pdarun.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>

RunBuf *newRunBuf()
{
	RunBuf *rb = (RunBuf*)malloc(sizeof(RunBuf));
	memset( rb, 0, sizeof(RunBuf) );
	return rb;
}

#define true 1
#define false 0

void initDynamicFuncs();
void initStringFuncs();
void initFileFuncs();
void initFdFuncs();
void initAccumFuncs();

void initStaticFuncs();
void initPatternFuncs();
void initReplFuncs();

struct InputFuncs baseFuncs;
struct InputFuncs accumFuncs;
struct InputFuncs dynamicFuncs;
struct InputFuncs stringFuncs;
struct InputFuncs fileFuncs;
struct InputFuncs fdFuncs;

InputStream *newInputStreamFile( FILE *file )
{
	InputStream *is = (InputStream*)malloc(sizeof(InputStream));
	memset( is, 0, sizeof(InputStream) );
	is->line = 1;
	is->column = 1;
	is->file = file;
	is->funcs = &fileFuncs;
	return is;
}

InputStream *newInputStreamFd( long fd )
{
	InputStream *is = (InputStream*)malloc(sizeof(InputStream));
	memset( is, 0, sizeof(InputStream) );
	is->line = 1;
	is->column = 1;
	is->fd = fd;
	is->funcs = &fdFuncs;
	return is;
}

InputStream *newInputStreamAccum()
{
	InputStream *is = (InputStream*)malloc(sizeof(InputStream));
	memset( is, 0, sizeof(InputStream) );
	is->line = 1;
	is->column = 1;
	is->funcs = &accumFuncs;
	return is;
}


RunBuf *inputStreamHead( InputStream *is )
{ 
	return is->queue;
}

RunBuf *inputStreamTail( InputStream *is )
{
	return is->queueTail;
}

RunBuf *inputStreamPopHead( InputStream *is )
{
	RunBuf *ret = is->queue;
	is->queue = is->queue->next;
	if ( is->queue == 0 )
		is->queueTail = 0;
	else
		is->queue->prev = 0;
	return ret;
}

RunBuf *inputStreamPopTail( InputStream *is )
{
	RunBuf *ret = is->queueTail;
	is->queueTail = is->queue->prev;
	if ( is->queueTail == 0 )
		is->queue = 0;
	else
		is->queueTail->next = 0;
	return ret;
}

void inputStreamAppend( InputStream *is, RunBuf *runBuf )
{
	if ( is->queue == 0 ) {
		runBuf->prev = runBuf->next = 0;
		is->queue = is->queueTail = runBuf;
	}
	else {
		is->queueTail->next = runBuf;
		runBuf->prev = is->queueTail;
		runBuf->next = 0;
		is->queueTail = runBuf;
	}
}

void inputStreamPrepend( InputStream *is, RunBuf *runBuf )
{
	if ( is->queue == 0 ) {
		runBuf->prev = runBuf->next = 0;
		is->queue = is->queueTail = runBuf;
	}
	else {
		is->queue->prev = runBuf;
		runBuf->prev = 0;
		runBuf->next = is->queue;
		is->queue = runBuf;
	}
}


void initInputFuncs()
{
	memset( &baseFuncs, 0, sizeof(struct InputFuncs) );

	initDynamicFuncs();
	initStringFuncs();
	initFileFuncs();
	initFdFuncs();
	initAccumFuncs();
	initStaticFuncs();
	initPatternFuncs();
	initReplFuncs();
}

/* 
 * Base run-time input streams.
 */

int inputStreamDynamicIsTree( InputStream *is )
{
	if ( is->queue != 0 && is->queue->type == RunBufTokenType )
		return true;
	return false;
}

int inputStreamDynamicIsIgnore( InputStream *is )
{
	if ( is->queue != 0 && is->queue->type == RunBufIgnoreType )
		return true;
	return false;
}

int inputStreamDynamicIsLangEl( InputStream *is )
{
	return false;
}

int inputStreamDynamicIsEof( InputStream *is )
{
	return is->queue == 0 && is->eof;
}

int inputStreamDynamicGetData( InputStream *is, char *dest, int length )
{
	/* If there is any data in the rubuf queue then read that first. */
	if ( is->queue != 0 ) {
		long avail = is->queue->length - is->queue->offset;
		if ( length >= avail ) {
			memcpy( dest, &is->queue->data[is->queue->offset], avail );
			RunBuf *del = inputStreamPopHead( is );
			free(del);
			return avail;
		}
		else {
			memcpy( dest, &is->queue->data[is->queue->offset], length );
			is->queue->offset += length;
			return length;
		}
	}
	else {
		/* No stored data, call the impl version. */
		return is->funcs->getDataImpl( is, dest, length );
	}
}

int inputStreamDynamicGetDataRev( InputStream *is, char *dest, int length )
{
	/* If there is any data in the rubuf queue then read that first. */
	if ( is->queueTail != 0 ) {
		long avail = is->queueTail->length - is->queueTail->offset;
		if ( length >= avail ) {
			memcpy( dest, &is->queue->data[is->queue->offset], avail );
			RunBuf *del = inputStreamPopTail(is);
			free(del);
			return avail;
		}
		else {
			memcpy( dest, &is->queue->data[is->queue->offset], length );
			is->queue->offset += length;
			return length;
		}
	}
	return 0;
}

int inputStreamDynamicTryAgainLater( InputStream *is )
{
	if ( is->later )
		return true;

	return false;
}

Tree *inputStreamDynamicGetTree( InputStream *is )
{
	if ( is->queue != 0 && is->queue->type == RunBufTokenType ) {
		RunBuf *runBuf = inputStreamPopHead( is );

		/* FIXME: using runbufs here for this is a poor use of memory. */
		Tree *tree = runBuf->tree;
		free(runBuf);
		return tree;
	}

	return 0;
}

void inputStreamDynamicPushText( InputStream *is, const char *data, long length )
{
//	#ifdef COLM_LOG_PARSE
//	if ( colm_log_parse ) {
//		cerr << "readying fake push" << endl;
//	}
//	#endif

//	takeBackBuffered( inputStream );

	/* Create a new buffer for the data. This is the easy implementation.
	 * Something better is needed here. It puts a max on the amount of
	 * data that can be pushed back to the inputStream. */
	assert( length < FSM_BUFSIZE );

	RunBuf *newBuf = newRunBuf();
	newBuf->length = length;
	memcpy( newBuf->data, data, length );

	is->funcs->pushBackBuf( is, newBuf );
}

void inputStreamDynamicPushTree( InputStream *is, Tree *tree, int ignore )
{
//	#ifdef COLM_LOG_PARSE
//	if ( colm_log_parse ) {
//		cerr << "readying fake push" << endl;
//	}
//	#endif

//	takeBackBuffered( inputStream );

	/* Create a new buffer for the data. This is the easy implementation.
	 * Something better is needed here. It puts a max on the amount of
	 * data that can be pushed back to the inputStream. */
	RunBuf *newBuf = newRunBuf();
	newBuf->type = ignore ? RunBufIgnoreType : RunBufTokenType;
	newBuf->tree = tree;

	inputStreamPrepend( is, newBuf );
}

Tree *inputStreamDynamicUndoPush( InputStream *is, int length )
{
	if ( is->queue->type == RunBufDataType ) {
		char tmp[length];
		int have = 0;
		while ( have < length ) {
			int res = is->funcs->getData( is, tmp, length-have );
			have += res;
		}
		return 0;
	}
	else {
		/* FIXME: leak here. */
		RunBuf *rb = inputStreamPopHead( is );
		Tree *tree = rb->tree;
		free(rb);
		return tree;
	}
}

Tree *inputStreamDynamicUndoAppend( InputStream *is, int length )
{
	if ( is->queueTail->type == RunBufDataType ) {
		char tmp[length];
		int have = 0;
		while ( have < length ) {
			int res = inputStreamDynamicGetDataRev( is, tmp, length-have );
			have += res;
		}
		return 0;
	}
	else {
		/* FIXME: leak here. */
		RunBuf *rb = inputStreamPopTail( is );
		Tree *tree = rb->tree;
		free(rb);
		return tree;
	}
}

void initDynamicFuncs()
{
	memcpy( &dynamicFuncs, &baseFuncs, sizeof(struct InputFuncs) );
	dynamicFuncs.isTree = &inputStreamDynamicIsTree;
	dynamicFuncs.isIgnore = &inputStreamDynamicIsIgnore;
	dynamicFuncs.isLangEl = &inputStreamDynamicIsLangEl;
	dynamicFuncs.isEof = &inputStreamDynamicIsEof;
	dynamicFuncs.tryAgainLater = &inputStreamDynamicTryAgainLater;
	dynamicFuncs.getData = &inputStreamDynamicGetData;
	dynamicFuncs.getTree = &inputStreamDynamicGetTree;
	dynamicFuncs.pushTree = &inputStreamDynamicPushTree;
	dynamicFuncs.undoPush = &inputStreamDynamicUndoPush;
	dynamicFuncs.undoAppend = &inputStreamDynamicUndoAppend;
	dynamicFuncs.pushText = &inputStreamDynamicPushText;
}

/*
 * String
 */

int inputStreamStringNeedFlush( InputStream *is )
{
	return is->eof;
}

int inputStreamStringGetDataImpl( InputStream *is, char *dest, int length )
{ 
	int available = is->dlen - is->offset;

	if ( available < length )
		length = available;

	memcpy( dest, is->data+is->offset, length );
	is->offset += length;

	if ( is->offset == is->dlen ) {
		//eof = true;
		is->later = true;
	}

	return length;
}

void inputStreamStringPushBackBuf( InputStream *is, RunBuf *runBuf )
{
	//char *data = runBuf->buf + runBuf->offset;
	long length = runBuf->length;

	assert( length <= is->offset );
	is->offset -= length;
}

void initStringFuncs()
{
	memcpy( &stringFuncs, &dynamicFuncs, sizeof(struct InputFuncs) );
	stringFuncs.needFlush = &inputStreamStringNeedFlush;
	stringFuncs.getDataImpl = &inputStreamStringGetDataImpl;
	stringFuncs.pushBackBuf = &inputStreamStringPushBackBuf;
}


/*
 * File
 */

int inputStreamFileNeedFlush( InputStream *is )
{
	return is->queue == 0 && feof( is->file );
}

int inputStreamFileGetDataImpl( InputStream *is, char *dest, int length )
{
	size_t res = fread( dest, 1, length, is->file );
	if ( res < (size_t) length )
		is->later = true;
	return res;
}

void inputStreamFilePushBackBuf( InputStream *is, RunBuf *runBuf )
{
	inputStreamPrepend( is, runBuf );
}

void initFileFuncs()
{
	memcpy( &fileFuncs, &dynamicFuncs, sizeof(struct InputFuncs) );
	fileFuncs.needFlush = &inputStreamFileNeedFlush;
	fileFuncs.getDataImpl = &inputStreamFileGetDataImpl;
	fileFuncs.pushBackBuf = &inputStreamFilePushBackBuf;
}

/*
 * FD
 */

int inputStreamFdNeedFlush( InputStream *is )
{
	return is->queue == 0 && is->eof;
}

void inputStreamFdPushBackBuf( InputStream *is, RunBuf *runBuf )
{
	inputStreamPrepend( is, runBuf );
}

int inputStreamFdGetDataImpl( InputStream *is, char *dest, int length )
{
	long got = read( is->fd, dest, length );
	if ( got == 0 )
		is->later = true;
	return got;
}

void initFdFuncs()
{
	memcpy( &fdFuncs, &dynamicFuncs, sizeof(struct InputFuncs) );
	fdFuncs.needFlush = &inputStreamFdNeedFlush;
	fdFuncs.getDataImpl = &inputStreamFdGetDataImpl;
	fdFuncs.pushBackBuf = &inputStreamFdPushBackBuf;
}


/*
 * Accum
 */

int inputStreamAccumTryAgainLater( InputStream *is )
{
	if ( is->later || ( !is->flush && is->queue == 0 ))
		return true;

	return false;
}

int inputStreamAccumNeedFlush( InputStream *is )
{
	if ( is->flush ) {
		is->flush = false;
		return true;
	}

	if ( is->queue != 0 && is->queue->type != RunBufDataType )
		return true;

	if ( is->eof )
		return true;
		
	return false;
}

int inputStreamAccumGetDataImpl( InputStream *is, char *dest, int length )
{
	/* No source of data, it is all done with RunBuf list appends. */
	return 0;
}

void inputStreamAccumPushBackBuf( InputStream *is, RunBuf *runBuf )
{
	inputStreamPrepend( is, runBuf );
}

void inputStreamAccumAppendData( InputStream *_is, const char *data, long len )
{
	InputStream *is = (InputStream*)_is;

	RunBuf *ad = newRunBuf();
	inputStreamAppend( is, ad );

	/* FIXME: need to deal with this. */
	assert( len < (int)sizeof(ad->data) );

	memcpy( ad->data, data, len );
	ad->length = len;
}

void inputStreamAccumAppendTree( InputStream *_is, Tree *tree )
{
	InputStream *is = (InputStream*)_is;

	RunBuf *ad = newRunBuf();

	inputStreamAppend( is, ad );

	ad->type = RunBufTokenType;
	ad->tree = tree;
	ad->length = 0;
}

void initAccumFuncs()
{
	memcpy( &accumFuncs, &dynamicFuncs, sizeof(struct InputFuncs) );
	accumFuncs.needFlush = &inputStreamAccumNeedFlush;
	accumFuncs.tryAgainLater = &inputStreamAccumTryAgainLater;
	accumFuncs.getDataImpl = &inputStreamAccumGetDataImpl;
	accumFuncs.appendData = &inputStreamAccumAppendData;
	accumFuncs.appendTree = &inputStreamAccumAppendTree;
	accumFuncs.pushBackBuf = &inputStreamAccumPushBackBuf;
}

