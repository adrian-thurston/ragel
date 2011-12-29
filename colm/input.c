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

void initStaticFuncs();
void initPatternFuncs();
void initReplFuncs();

struct SourceFuncs baseFuncs;
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

int inputStreamDynamicIsEof( SourceStream *is, int offset )
{
	int isEof = false;
	if ( is->eof ) {
		if ( is->queue == 0 )
			isEof = true;
		else if ( is->queue != 0 && is->queue->offset + offset >= is->queue->length )
			isEof = true;
	}

	debug( REALM_INPUT, "is eof: %d\n", (int)isEof );
	return isEof;
}

int inputStreamDynamicGetData( SourceStream *is, int skip, char *dest, int length, int *copied )
{
	int ret = 0;
	*copied = 0;

	/* Move over skip bytes. */
	RunBuf *buf = is->queue;
	while ( true ) {
		if ( buf == 0 ) {
			/* Got through the in-mem buffers without copying anything. */
			RunBuf *runBuf = newRunBuf();
			inputStreamPrepend( is, runBuf );
			int received = is->funcs->getDataImpl( is, runBuf->data, FSM_BUFSIZE );
			if ( received == 0 ) {
				ret = INPUT_EOD;
				break;
			}

			int slen = received < length ? received : length;
			memcpy( dest, runBuf->data, slen );
			*copied = slen;
			ret = INPUT_DATA;
			break;
		}

		int avail = buf->length - buf->offset;

		/* Anything available in the current buffer. */
		if ( avail > 0 ) {
			/* The source data from the current buffer. */
			char *src = &buf->data[buf->offset];
			int slen = avail <= length ? avail : length;

			/* Need to skip? */
			if ( skip > 0 && slen <= skip ) {
				/* Skipping the the whole source. */
				skip -= slen;
			}
			else {
				/* Either skip is zero, or less than slen. Skip goes to zero.
				 * Some data left over, copy it. */
				src += skip;
				slen -= skip;
				skip = 0;

				memcpy( dest, src, slen ) ;
				*copied += slen;
				ret = INPUT_DATA;
				break;
			}
		}

		buf = buf->next;
	}

#if DEBUG
	switch ( ret ) {
		case INPUT_DATA:
			debug( REALM_INPUT, "get data: DATA copied: %d\n", *copied );
			break;
		case INPUT_EOD:
			debug( REALM_INPUT, "get data: EOD\n" );
			break;
	}
#endif

	return ret;
}

int inputStreamDynamicConsumeData( SourceStream *is, int length )
{
	debug( REALM_INPUT, "consuming %ld bytes\n", length );
	is->queue->offset += length;
	return length;
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
			int res = 0;
			is->funcs->getData( is, 0, tmp, length-have, &res );
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
	dynamicFuncs.getData = &inputStreamDynamicGetData;
	dynamicFuncs.consumeData = &inputStreamDynamicConsumeData;
	dynamicFuncs.getTree = &inputStreamDynamicGetTree;
	dynamicFuncs.pushTree = &inputStreamDynamicPushTree;
	dynamicFuncs.undoPush = &inputStreamDynamicUndoPush;
	dynamicFuncs.undoAppend = &inputStreamDynamicUndoAppend;
	dynamicFuncs.pushText = &inputStreamDynamicPushText;
}

/*
 * String
 */

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
	stringFuncs.getDataImpl = &inputStreamStringGetDataImpl;
	stringFuncs.pushBackBuf = &inputStreamStringPushBackBuf;
}


/*
 * File
 */

int inputStreamFileGetDataImpl( SourceStream *is, char *dest, int length )
{
	debug( REALM_INPUT, "inputStreamFileGetDataImpl length = %ld\n", length );
	size_t res = fread( dest, 1, length, is->file );
	if ( res < (size_t) length ) {
		debug( REALM_INPUT, "setting later = true\n" );
		is->later = true;
	}
	return res;
}

void inputStreamFilePushBackBuf( SourceStream *is, RunBuf *runBuf )
{
	inputStreamPrepend( is, runBuf );
}

void initFileFuncs()
{
	memcpy( &fileFuncs, &dynamicFuncs, sizeof(struct SourceFuncs) );
	fileFuncs.getDataImpl = &inputStreamFileGetDataImpl;
	fileFuncs.pushBackBuf = &inputStreamFilePushBackBuf;
}

/*
 * FD
 */

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
	fdFuncs.getDataImpl = &inputStreamFdGetDataImpl;
	fdFuncs.pushBackBuf = &inputStreamFdPushBackBuf;
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
int isEof( InputStream *is, int offset )
{
	if ( isSourceStream( is ) ) {
		Stream *stream = (Stream*)is->queue->tree;
		return stream->in->funcs->isEof( stream->in, offset );
	}
	else {
		debug( REALM_INPUT, "checking input stream eof\n" );
		return is->queue == 0 && is->eof;
	}
}

void setEof( InputStream *is )
{
	debug( REALM_INPUT, "setting EOF in input stream\n" );
	is->eof = true;
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

int getData( InputStream *is, int skip, char *dest, int length, int *copied )
{
	int ret = 0;
	*copied = 0;

	/* Move over skip bytes. */
	RunBuf *buf = is->queue;
	while ( true ) {
		if ( buf == 0 ) {
			/* Got through the in-mem buffers without copying anything. */
			ret = is->eof ? INPUT_EOF : INPUT_EOD;
			break;
		}

		if ( buf->type == RunBufSourceType ) {
			Stream *stream = (Stream*)buf->tree;
			int type = stream->in->funcs->getData( stream->in, skip, dest, length, copied );

			if ( type == INPUT_EOD && is->eof ) {
				ret = INPUT_EOF;
				break;
			}

			ret = type;
			break;
		}

		if ( buf->type == RunBufTokenType ) {
			ret = INPUT_TREE;
			break;
		}

		int avail = buf->length - buf->offset;

		/* Anything available in the current buffer. */
		if ( avail > 0 ) {
			/* The source data from the current buffer. */
			char *src = &buf->data[buf->offset];
			int slen = avail <= length ? avail : length;

			/* Need to skip? */
			if ( skip > 0 && slen <= skip ) {
				/* Skipping the the whole source. */
				skip -= slen;
			}
			else {
				/* Either skip is zero, or less than slen. Skip goes to zero.
				 * Some data left over, copy it. */
				src += skip;
				slen -= skip;
				skip = 0;

				memcpy( dest, src, slen ) ;
				*copied += slen;
				ret = INPUT_DATA;
				break;
			}
		}

		buf = buf->next;
	}

#if DEBUG
	switch ( ret ) {
		case INPUT_DATA:
			debug( REALM_INPUT, "get data: DATA copied: %d\n", *copied );
			break;
		case INPUT_EOD:
			debug( REALM_INPUT, "get data: EOD\n" );
			break;
		case INPUT_EOF:
			debug( REALM_INPUT, "get data: EOF\n" );
			break;
		case INPUT_TREE:
			debug( REALM_INPUT, "get data: TREE\n" );
			break;
		case INPUT_LANG_EL:
			debug( REALM_INPUT, "get data: LANG_EL\n" );
			break;
	}
#endif

	return ret;
}

int consumeData( InputStream *is, int length )
{
	if ( isSourceStream( is ) ) {
		Stream *stream = (Stream*)is->queue->tree;
		return stream->in->funcs->consumeData( stream->in, length );
	}
	else {
		debug( REALM_INPUT, "consuming %ld bytes\n", length );

		is->queue->offset += length;
		if ( is->queue->offset == is->queue->length ) {
			RunBuf *runBuf = inputStreamPopHead2( is );
			free( runBuf );
		}

		return length;
	}
}

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
//	if ( isSourceStream( is ) ) {
//		Stream *stream = (Stream*)is->queue->tree;
//		return stream->in->funcs->getTree( stream->in );
//	}
//	else {

	if ( is->queue != 0 && (is->queue->type == RunBufTokenType || is->queue->type == RunBufIgnoreType) ) {
		RunBuf *runBuf = inputStreamPopHead2( is );

		/* FIXME: using runbufs here for this is a poor use of memory. */
		Tree *tree = runBuf->tree;
		free(runBuf);
		return tree;
	}

//		return 0;
//	}
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
				int res = 0;
				getData( is, 0, tmp, length-have, &res );
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

void appendData( InputStream *is, const char *data, long len )
{
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

void appendTree( InputStream *is, Tree *tree )
{
	RunBuf *ad = newRunBuf();

	inputStreamAppend2( is, ad );

	ad->type = RunBufTokenType;
	ad->tree = tree;
	ad->length = 0;
}

void appendStream( InputStream *in, struct ColmTree *tree )
{
	RunBuf *ad = newRunBuf();

	inputStreamAppend2( in, ad );

	ad->type = RunBufSourceType;
	ad->tree = tree;
	ad->length = 0;
}

Tree *undoAppendStream( InputStream *in )
{
	/* FIXME: leak here. */
	RunBuf *rb = inputStreamPopTail2( in );
	Tree *tree = rb->tree;
	free(rb);
	return tree;
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
