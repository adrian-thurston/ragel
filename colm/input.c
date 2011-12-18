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

#include <colm/input.h>
#include <colm/fsmrun.h>
#include <colm/pdarun.h>
#include <colm/debug.h>

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

struct SourceFuncs baseFuncs;
struct SourceFuncs accumFuncs;
struct SourceFuncs dynamicFuncs;
struct SourceFuncs stringFuncs;
struct SourceFuncs fileFuncs;
struct SourceFuncs fdFuncs;

void initInputStream( InputStream *inputStream )
{
	memset( inputStream, 0, sizeof(InputStream) );

	/* FIXME: correct values here. */
	inputStream->line = 1;
	inputStream->column = 1;
	inputStream->byte = 0;
}

void initSourceStream( SourceStream *inputStream )
{
	/* FIXME: correct values here. */
	inputStream->line = 1;
	inputStream->column = 1;
	inputStream->byte = 0;
}

SourceStream *newInputStreamFile( FILE *file )
{
	SourceStream *is = (SourceStream*)malloc(sizeof(SourceStream));
	memset( is, 0, sizeof(SourceStream) );
	is->line = 1;
	is->column = 1;
	is->file = file;
	is->funcs = &fileFuncs;
	return is;
}

SourceStream *newInputStreamFd( long fd )
{
	SourceStream *is = (SourceStream*)malloc(sizeof(SourceStream));
	memset( is, 0, sizeof(SourceStream) );
	is->line = 1;
	is->column = 1;
	is->fd = fd;
	is->funcs = &fdFuncs;
	return is;
}

SourceStream *newInputStreamAccum()
{
	SourceStream *is = (SourceStream*)malloc(sizeof(SourceStream));
	memset( is, 0, sizeof(SourceStream) );
	is->line = 1;
	is->column = 1;
	is->funcs = &accumFuncs;
	return is;
}


RunBuf *inputStreamHead( SourceStream *is )
{ 
	return is->queue;
}

RunBuf *inputStreamTail( SourceStream *is )
{
	return is->queueTail;
}

RunBuf *inputStreamPopHead( SourceStream *is )
{
	RunBuf *ret = is->queue;
	is->queue = is->queue->next;
	if ( is->queue == 0 )
		is->queueTail = 0;
	else
		is->queue->prev = 0;
	return ret;
}

RunBuf *inputStreamPopTail( SourceStream *is )
{
	RunBuf *ret = is->queueTail;
	is->queueTail = is->queue->prev;
	if ( is->queueTail == 0 )
		is->queue = 0;
	else
		is->queueTail->next = 0;
	return ret;
}

void inputStreamAppend( SourceStream *is, RunBuf *runBuf )
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

void inputStreamPrepend( SourceStream *is, RunBuf *runBuf )
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
	memset( &baseFuncs, 0, sizeof(struct SourceFuncs) );

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

int inputStreamDynamicIsTree( SourceStream *is )
{
	if ( is->queue != 0 && is->queue->type == RunBufTokenType )
		return true;
	return false;
}

int inputStreamDynamicIsIgnore( SourceStream *is )
{
	if ( is->queue != 0 && is->queue->type == RunBufIgnoreType )
		return true;
	return false;
}

int inputStreamDynamicIsLangEl( SourceStream *is )
{
	return false;
}

int inputStreamDynamicIsEof( SourceStream *is )
{
	return is->queue == 0 && is->eof;
}

int inputStreamDynamicGetData( SourceStream *is, char *dest, int length )
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

int inputStreamDynamicGetDataRev( SourceStream *is, char *dest, int length )
{
	/* If there is any data in the rubuf queue then read that first. */
	if ( is->queueTail != 0 ) {
		long avail = is->queueTail->length - is->queueTail->offset;
		if ( length >= avail ) {
			memcpy( dest, &is->queueTail->data[is->queue->offset], avail );
			RunBuf *del = inputStreamPopTail(is);
			free(del);
			return avail;
		}
		else {
			memcpy( dest, &is->queueTail->data[is->queueTail->offset], length );
			is->queueTail->length -= length;
			return length;
		}
	}
	return 0;
}

int inputStreamDynamicTryAgainLater( SourceStream *is )
{
	if ( is->later )
		return true;

	return false;
}

Tree *inputStreamDynamicGetTree( SourceStream *is )
{
	if ( is->queue != 0 && (is->queue->type == RunBufTokenType || is->queue->type == RunBufIgnoreType) ) {
		RunBuf *runBuf = inputStreamPopHead( is );

		/* FIXME: using runbufs here for this is a poor use of memory. */
		Tree *tree = runBuf->tree;
		free(runBuf);
		return tree;
	}

	return 0;
}

void inputStreamDynamicPushText( SourceStream *is, const char *data, long length )
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

void inputStreamDynamicPushTree( SourceStream *is, Tree *tree, int ignore )
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

Tree *inputStreamDynamicUndoPush( SourceStream *is, int length )
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

Tree *inputStreamDynamicUndoAppend( SourceStream *is, int length )
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
	memcpy( &dynamicFuncs, &baseFuncs, sizeof(struct SourceFuncs) );
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

int inputStreamStringNeedFlush( SourceStream *is )
{
	return is->eof;
}

int inputStreamStringGetDataImpl( SourceStream *is, char *dest, int length )
{ 
	int available = is->dlen - is->offset;

	if ( available < length )
		length = available;

	memcpy( dest, is->data+is->offset, length );
	is->offset += length;

	if ( is->offset == is->dlen ) {
		//eof = true;
		debug( REALM_PARSE, "setting later = true\n" );
		is->later = true;
	}

	return length;
}

void inputStreamStringPushBackBuf( SourceStream *is, RunBuf *runBuf )
{
	//char *data = runBuf->buf + runBuf->offset;
	long length = runBuf->length;

	assert( length <= is->offset );
	is->offset -= length;
}

void initStringFuncs()
{
	memcpy( &stringFuncs, &dynamicFuncs, sizeof(struct SourceFuncs) );
	stringFuncs.needFlush = &inputStreamStringNeedFlush;
	stringFuncs.getDataImpl = &inputStreamStringGetDataImpl;
	stringFuncs.pushBackBuf = &inputStreamStringPushBackBuf;
}


/*
 * File
 */

int inputStreamFileNeedFlush( SourceStream *is )
{
	return is->queue == 0 && feof( is->file );
}

int inputStreamFileGetDataImpl( SourceStream *is, char *dest, int length )
{
	size_t res = fread( dest, 1, length, is->file );
	if ( res < (size_t) length )
		is->later = true;
	return res;
}

void inputStreamFilePushBackBuf( SourceStream *is, RunBuf *runBuf )
{
	inputStreamPrepend( is, runBuf );
}

void initFileFuncs()
{
	memcpy( &fileFuncs, &dynamicFuncs, sizeof(struct SourceFuncs) );
	fileFuncs.needFlush = &inputStreamFileNeedFlush;
	fileFuncs.getDataImpl = &inputStreamFileGetDataImpl;
	fileFuncs.pushBackBuf = &inputStreamFilePushBackBuf;
}

/*
 * FD
 */

int inputStreamFdNeedFlush( SourceStream *is )
{
	return is->queue == 0 && is->eof;
}

void inputStreamFdPushBackBuf( SourceStream *is, RunBuf *runBuf )
{
	inputStreamPrepend( is, runBuf );
}

int inputStreamFdGetDataImpl( SourceStream *is, char *dest, int length )
{
	long got = read( is->fd, dest, length );
	if ( got == 0 )
		is->later = true;
	return got;
}

void initFdFuncs()
{
	memcpy( &fdFuncs, &dynamicFuncs, sizeof(struct SourceFuncs) );
	fdFuncs.needFlush = &inputStreamFdNeedFlush;
	fdFuncs.getDataImpl = &inputStreamFdGetDataImpl;
	fdFuncs.pushBackBuf = &inputStreamFdPushBackBuf;
}


/*
 * Accum
 */

int inputStreamAccumTryAgainLater( SourceStream *is )
{
	if ( is->later || ( !is->flush && is->queue == 0 )) {
		debug( REALM_PARSE, "try again later %d %d %d\n", is->later, is->flush, is->queue );
		return true;
	}

	return false;
}

int inputStreamAccumNeedFlush( SourceStream *is )
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

int inputStreamAccumGetDataImpl( SourceStream *is, char *dest, int length )
{
	/* No source of data, it is all done with RunBuf list appends. */
	return 0;
}

void inputStreamAccumPushBackBuf( SourceStream *is, RunBuf *runBuf )
{
	inputStreamPrepend( is, runBuf );
}

void inputStreamAccumAppendData( SourceStream *_is, const char *data, long len )
{
	SourceStream *is = (SourceStream*)_is;

	while ( len > 0 ) {
		RunBuf *ad = newRunBuf();
		inputStreamAppend( is, ad );

		long consume = 
			len <= (long)sizeof(ad->data) ? 
			len : (long)sizeof(ad->data);

		memcpy( ad->data, data, consume );
		ad->length = consume;

		len -= consume;
		data += consume;
	}
}

void inputStreamAccumAppendTree( SourceStream *_is, Tree *tree )
{
	SourceStream *is = (SourceStream*)_is;

	RunBuf *ad = newRunBuf();

	inputStreamAppend( is, ad );

	ad->type = RunBufTokenType;
	ad->tree = tree;
	ad->length = 0;
}

void initAccumFuncs()
{
	memcpy( &accumFuncs, &dynamicFuncs, sizeof(struct SourceFuncs) );
	accumFuncs.needFlush = &inputStreamAccumNeedFlush;
	accumFuncs.tryAgainLater = &inputStreamAccumTryAgainLater;
	accumFuncs.getDataImpl = &inputStreamAccumGetDataImpl;
	accumFuncs.appendData = &inputStreamAccumAppendData;
	accumFuncs.appendTree = &inputStreamAccumAppendTree;
	accumFuncs.pushBackBuf = &inputStreamAccumPushBackBuf;
}

/*
 * InputStream struct, this wraps the list of input streams.
 */

static void inputStreamPrepend2( InputStream *is, RunBuf *runBuf )
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

RunBuf *inputStreamPopHead2( InputStream *is )
{
	RunBuf *ret = is->queue;
	is->queue = is->queue->next;
	if ( is->queue == 0 )
		is->queueTail = 0;
	else
		is->queue->prev = 0;
	return ret;
}

static void inputStreamAppend2( InputStream *is, RunBuf *runBuf )
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

static RunBuf *inputStreamPopTail2( InputStream *is )
{
	RunBuf *ret = is->queueTail;
	is->queueTail = is->queue->prev;
	if ( is->queueTail == 0 )
		is->queue = 0;
	else
		is->queueTail->next = 0;
	return ret;
}

static int inputStreamDynamicGetDataRev2( InputStream *is, char *dest, int length )
{
	/* If there is any data in the rubuf queue then read that first. */
	if ( is->queueTail != 0 ) {
		long avail = is->queueTail->length - is->queueTail->offset;
		if ( length >= avail ) {
			memcpy( dest, &is->queueTail->data[is->queue->offset], avail );
			RunBuf *del = inputStreamPopTail2(is);
			free(del);
			return avail;
		}
		else {
			memcpy( dest, &is->queueTail->data[is->queueTail->offset], length );
			is->queueTail->length -= length;
			return length;
		}
	}
	return 0;
}

static int isSourceStream( InputStream *is )
{
	if ( is->queue != 0 && is->queue->type == RunBufSourceType )
		return true;
	return false;
}


//dynamicFuncs.isTree = &inputStreamDynamicIsTree;
int isTree( InputStream *is )
{
	if ( isSourceStream( is ) ) {
		Stream *stream = (Stream*)is->queue->tree;
		return stream->in->funcs->isTree( stream->in );
	}
	else {
		if ( is->queue != 0 && is->queue->type == RunBufTokenType )
			return true;
		return false;
	}
}

//dynamicFuncs.isIgnore = &inputStreamDynamicIsIgnore;
int isIgnore( InputStream *is )
{
	if ( isSourceStream( is ) ) {
		Stream *stream = (Stream*)is->queue->tree;
		return stream->in->funcs->isIgnore( stream->in );
	}
	else {
		if ( is->queue != 0 && is->queue->type == RunBufIgnoreType )
			return true;
		return false;
	}
}

//dynamicFuncs.isLangEl = &inputStreamDynamicIsLangEl;
int isLangEl( InputStream *is )
{
	if ( isSourceStream( is ) ) {
		Stream *stream = (Stream*)is->queue->tree;
		return stream->in->funcs->isLangEl( stream->in );
	}
	else {
		return false;
	}
}

//dynamicFuncs.isEof = &inputStreamDynamicIsEof;
int isEof( InputStream *is )
{
	if ( isSourceStream( is ) ) {
		Stream *stream = (Stream*)is->queue->tree;
		return stream->in->funcs->isEof( stream->in );
	}
	else {
		return is->queue == 0 && is->eof;
	}
}

void setEof( InputStream *is )
{
	if ( isSourceStream( is ) ) {
		Stream *stream = (Stream*)is->queue->tree;
		stream->in->eof = true;
	}
	else {
		is->eof = true;
	}
}

void unsetEof( InputStream *is )
{
	if ( isSourceStream( is ) ) {
		Stream *stream = (Stream*)is->queue->tree;
		stream->in->eof = false;
	}
	else {
		is->eof = false;
	}
}

void unsetLater( InputStream *is )
{
	if ( isSourceStream( is ) ) {
		Stream *stream = (Stream*)is->queue->tree;
		stream->in->later = false;
	}
	else {
		is->later = false;
	}
}

//accumFuncs.needFlush = &inputStreamAccumNeedFlush;
int needFlush( InputStream *is )
{
	if ( isSourceStream( is ) ) {
		Stream *stream = (Stream*)is->queue->tree;
		return stream->in->funcs->needFlush( stream->in );
	}
	else {
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
}

//accumFuncs.tryAgainLater = &inputStreamAccumTryAgainLater;
int tryAgainLater( InputStream *is )
{
	if ( isSourceStream( is ) ) {
		Stream *stream = (Stream*)is->queue->tree;
		return stream->in->funcs->tryAgainLater( stream->in );
	}
	else {
		if ( is->later || ( !is->flush && is->queue == 0 )) {
			debug( REALM_PARSE, "try again later %d %d %d\n", is->later, is->flush, is->queue );
			return true;
		}

		return false;
	}
}

//dynamicFuncs.getData = &inputStreamDynamicGetData;
int getData( InputStream *is, char *dest, int length )
{
	if ( isSourceStream( is ) ) {
		Stream *stream = (Stream*)is->queue->tree;
		return stream->in->funcs->getData( stream->in, dest, length );
	}
	else {
		/* If there is any data in the rubuf queue then read that first. */
		if ( is->queue != 0 ) {
			long avail = is->queue->length - is->queue->offset;
			if ( length >= avail ) {
				memcpy( dest, &is->queue->data[is->queue->offset], avail );
				RunBuf *del = inputStreamPopHead2( is );
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
			return getDataImpl( is, dest, length );
		}
	}
}

//accumFuncs.getDataImpl = &inputStreamAccumGetDataImpl;
int getDataImpl( InputStream *is, char *dest, int length )
{
	if ( isSourceStream( is ) ) {
		Stream *stream = (Stream*)is->queue->tree;
		return stream->in->funcs->getDataImpl( stream->in, dest, length );
	}
	else {
		/* No source of data, it is all done with RunBuf list appends. */
		return 0;
	}
}

//dynamicFuncs.getTree = &inputStreamDynamicGetTree;
Tree *getTree( InputStream *is )
{
	if ( isSourceStream( is ) ) {
		Stream *stream = (Stream*)is->queue->tree;
		return stream->in->funcs->getTree( stream->in );
	}
	else {
		if ( is->queue != 0 && (is->queue->type == RunBufTokenType || is->queue->type == RunBufIgnoreType) ) {
			RunBuf *runBuf = inputStreamPopHead2( is );

			/* FIXME: using runbufs here for this is a poor use of memory. */
			Tree *tree = runBuf->tree;
			free(runBuf);
			return tree;
		}

		return 0;
	}
}

struct LangEl *getLangEl( InputStream *is, long *bindId, char **data, long *length )
{
	if ( isSourceStream( is ) ) {
		Stream *stream = (Stream*)is->queue->tree;
		return stream->in->funcs->getLangEl( stream->in, bindId, data, length );
	}
	else {
		return 0;
	}
}

void pushBackNamed( InputStream *is )
{
	if ( isSourceStream( is ) ) {
		Stream *stream = (Stream*)is->queue->tree;
		return stream->in->funcs->pushBackNamed( stream->in );
	}
}

//dynamicFuncs.pushTree = &inputStreamDynamicPushTree;
void pushTree( InputStream *is, Tree *tree, int ignore )
{
	if ( isSourceStream( is ) ) {
		Stream *stream = (Stream*)is->queue->tree;
		return stream->in->funcs->pushTree( stream->in, tree, ignore );
	}
	else {
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

		inputStreamPrepend2( is, newBuf );
	}
}

//dynamicFuncs.pushText = &inputStreamDynamicPushText;
void pushText( InputStream *is, const char *data, long length )
{
	if ( isSourceStream( is ) ) {
		Stream *stream = (Stream*)is->queue->tree;
		return stream->in->funcs->pushText( stream->in, data, length );
	}
	else {
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

		pushBackBuf( is, newBuf );
	}
}

//dynamicFuncs.undoPush = &inputStreamDynamicUndoPush;
Tree *undoPush( InputStream *is, int length )
{
	if ( isSourceStream( is ) ) {
		Stream *stream = (Stream*)is->queue->tree;
		return stream->in->funcs->undoPush( stream->in, length );
	}
	else {
		if ( is->queue->type == RunBufDataType ) {
			char tmp[length];
			int have = 0;
			while ( have < length ) {
				int res = getData( is, tmp, length-have );
				have += res;
			}
			return 0;
		}
		else {
			/* FIXME: leak here. */
			RunBuf *rb = inputStreamPopHead2( is );
			Tree *tree = rb->tree;
			free(rb);
			return tree;
		}
	}
}

//accumFuncs.appendData = &inputStreamAccumAppendData;
void appendData( InputStream *is, const char *data, long len )
{
	if ( isSourceStream( is ) ) {
		Stream *stream = (Stream*)is->queue->tree;
		return stream->in->funcs->appendData( stream->in, data, len );
	}
	else {
		while ( len > 0 ) {
			RunBuf *ad = newRunBuf();
			inputStreamAppend2( is, ad );

			long consume = 
				len <= (long)sizeof(ad->data) ? 
				len : (long)sizeof(ad->data);

			memcpy( ad->data, data, consume );
			ad->length = consume;

			len -= consume;
			data += consume;
		}
	}
}

//accumFuncs.appendTree = &inputStreamAccumAppendTree;
void appendTree( InputStream *is, Tree *tree )
{
	if ( isSourceStream( is ) ) {
		Stream *stream = (Stream*)is->queue->tree;
		return stream->in->funcs->appendTree( stream->in, tree );
	}
	else {
		RunBuf *ad = newRunBuf();

		inputStreamAppend2( is, ad );

		ad->type = RunBufTokenType;
		ad->tree = tree;
		ad->length = 0;
	}
}

void appendStream( InputStream *in, struct ColmTree *tree )
{
	RunBuf *ad = newRunBuf();

	inputStreamAppend2( in, ad );

	ad->type = RunBufSourceType;
	ad->tree = tree;
	ad->length = 0;
}

//dynamicFuncs.undoAppend = &inputStreamDynamicUndoAppend;
Tree *undoAppend( InputStream *is, int length )
{
	if ( isSourceStream( is ) ) {
		Stream *stream = (Stream*)is->queue->tree;
		return stream->in->funcs->undoAppend( stream->in, length );
	}
	else {
		if ( is->queueTail->type == RunBufDataType ) {
			char tmp[length];
			int have = 0;
			while ( have < length ) {
				int res = inputStreamDynamicGetDataRev2( is, tmp, length-have );
				have += res;
			}
			return 0;
		}
		else {
			/* FIXME: leak here. */
			RunBuf *rb = inputStreamPopTail2( is );
			Tree *tree = rb->tree;
			free(rb);
			return tree;
		}
	}
}

//accumFuncs.pushBackBuf = &inputStreamAccumPushBackBuf;
void pushBackBuf( InputStream *is, RunBuf *runBuf )
{
	if ( isSourceStream( is ) ) {
		Stream *stream = (Stream*)is->queue->tree;
		stream->in->funcs->pushBackBuf( stream->in, runBuf );
	}
	else {
		inputStreamPrepend2( is, runBuf );
	}
}
