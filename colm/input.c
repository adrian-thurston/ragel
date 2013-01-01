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

#include <colm/input.h>
#include <colm/fsmrun.h>
#include <colm/pdarun.h>
#include <colm/debug.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>

#define true 1
#define false 0

RunBuf *newRunBuf()
{
	RunBuf *rb = (RunBuf*)malloc(sizeof(RunBuf));
	memset( rb, 0, sizeof(RunBuf) );
	return rb;
}

void initStreamFuncs();
void initFdFuncs();
void initFileFuncs();
void initPatFuncs();
void initConsFuncs();

struct StreamFuncs dynamicFuncs;
struct StreamFuncs fileFuncs;
struct StreamFuncs fdFuncs;
struct StreamFuncs streamFuncs;

void initSourceStream( StreamImpl *inputStream )
{
}

void clearSourceStream( struct ColmProgram *prg, Tree **sp, StreamImpl *sourceStream )
{
	RunBuf *buf = sourceStream->queue;
	while ( buf != 0 ) {
		switch ( buf->type ) {
			case RunBufDataType:
				break;

			case RunBufTokenType:
			case RunBufIgnoreType:
			case RunBufSourceType:
				treeDownref( prg, sp, buf->tree );
				break;
		}

		RunBuf *next = buf->next;
		free( buf );
		buf = next;
	}

	sourceStream->queue = 0;
}

StreamImpl *newSourceStreamFile( FILE *file )
{
	StreamImpl *ss = (StreamImpl*)malloc(sizeof(StreamImpl));
	memset( ss, 0, sizeof(StreamImpl) );
	ss->file = file;
	ss->funcs = &fileFuncs;
	return ss;
}

StreamImpl *newSourceStreamFd( long fd )
{
	StreamImpl *ss = (StreamImpl*)malloc(sizeof(StreamImpl));
	memset( ss, 0, sizeof(StreamImpl) );
	ss->fd = fd;
	ss->funcs = &fdFuncs;
	return ss;
}

static RunBuf *sourceStreamPopHead( StreamImpl *ss )
{
	RunBuf *ret = ss->queue;
	ss->queue = ss->queue->next;
	if ( ss->queue == 0 )
		ss->queueTail = 0;
	else
		ss->queue->prev = 0;
	return ret;
}

static void sourceStreamAppend( StreamImpl *ss, RunBuf *runBuf )
{
	if ( ss->queue == 0 ) {
		runBuf->prev = runBuf->next = 0;
		ss->queue = ss->queueTail = runBuf;
	}
	else {
		ss->queueTail->next = runBuf;
		runBuf->prev = ss->queueTail;
		runBuf->next = 0;
		ss->queueTail = runBuf;
	}
}

static void sourceStreamPrepend( StreamImpl *ss, RunBuf *runBuf )
{
	if ( ss->queue == 0 ) {
		runBuf->prev = runBuf->next = 0;
		ss->queue = ss->queueTail = runBuf;
	}
	else {
		ss->queue->prev = runBuf;
		runBuf->prev = 0;
		runBuf->next = ss->queue;
		ss->queue = runBuf;
	}
}

void initStreamFuncs()
{
	memset( &streamFuncs, 0, sizeof(struct StreamFuncs) );
	streamFuncs.getData = &_getData;
	streamFuncs.consumeData = &_consumeData;
	streamFuncs.undoConsumeData = &_undoConsumeData;
	streamFuncs.consumeTree = &_consumeTree;
	streamFuncs.undoConsumeTree = &_undoConsumeTree;
	streamFuncs.consumeLangEl = &_consumeLangEl;
	streamFuncs.undoConsumeLangEl = &_undoConsumeLangEl;

	streamFuncs.setEof = &_setEof;
	streamFuncs.unsetEof = &_unsetEof;

	streamFuncs.prependData = &_prependData;
	streamFuncs.undoPrependData = &_undoPrependData;

	streamFuncs.prependTree = &_prependTree;
	streamFuncs.undoPrependTree = &_undoPrependTree;

	streamFuncs.appendData = &_appendData;
	streamFuncs.appendTree = &_appendTree;
	streamFuncs.appendStream = &_appendStream;
	streamFuncs.undoAppendData = &_undoAppendData;
	streamFuncs.undoAppendStream = &_undoAppendStream;
	streamFuncs.undoAppendTree = &_undoAppendTree;
}


void initInputFuncs()
{
	initStreamFuncs();
	initFdFuncs();
	initFileFuncs();
	initPatFuncs();
	initConsFuncs();
}

/* 
 * Base run-time input streams.
 */

int fdGetData( FsmRun *fsmRun, StreamImpl *ss, int skip, char *dest, int length, int *copied )
{
	int ret = 0;
	*copied = 0;

	/* Move over skip bytes. */
	RunBuf *buf = ss->queue;
	while ( true ) {
		if ( buf == 0 ) {
			/* Got through the in-mem buffers without copying anything. */
			RunBuf *runBuf = newRunBuf();
			sourceStreamAppend( ss, runBuf );
			int received = ss->funcs->getDataSource( ss, runBuf->data, FSM_BUFSIZE );
			if ( received == 0 ) {
				ret = INPUT_EOD;
				break;
			}
			runBuf->length = received;

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

			/* Need to skip? */
			if ( skip > 0 && skip >= avail ) {
				/* Skipping the the whole source. */
				skip -= avail;
			}
			else {
				/* Either skip is zero, or less than slen. Skip goes to zero.
				 * Some data left over, copy it. */
				src += skip;
				avail -= skip;
				skip = 0;

				int slen = avail < length ? avail : length;
				memcpy( dest, src, slen ) ;
				*copied += slen;
				ret = INPUT_DATA;
				break;
			}
		}

		buf = buf->next;
	}

	return ret;
}

int fdConsumeData( StreamImpl *ss, int length )
{
	debug( REALM_INPUT, "source consuming %ld bytes\n", length );

	int consumed = 0;

	/* Move over skip bytes. */
	while ( true ) {
		RunBuf *buf = ss->queue;

		if ( buf == 0 )
			break;

		if ( buf->type == RunBufTokenType )
			break;
		else if ( buf->type == RunBufIgnoreType )
			break;
		else {
			/* Anything available in the current buffer. */
			int avail = buf->length - buf->offset;
			if ( avail > 0 ) {
				/* The source data from the current buffer. */
				int slen = avail <= length ? avail : length;
				debug( REALM_INPUT, "consumed: %.*s\n", slen, buf->data + buf->offset );
				consumed += slen;
				length -= slen;
				buf->offset += slen;
			}
		}

		if ( length == 0 )
			break;

		RunBuf *runBuf = sourceStreamPopHead( ss );
		free( runBuf );
	}

	return consumed;
}

int fdUndoConsumeData( FsmRun *fsmRun, StreamImpl *ss, const char *data, int length )
{
	debug( REALM_INPUT, "undoing consume of %ld bytes\n", length );

	RunBuf *newBuf = newRunBuf();
	newBuf->length = length;
	memcpy( newBuf->data, data, length );
	sourceStreamPrepend( ss, newBuf );

	return length;
}

/*
 * File
 */

int fileGetDataSource( StreamImpl *ss, char *dest, int length )
{
	debug( REALM_INPUT, "inputStreamFileGetDataSource length = %ld\n", length );
	size_t res = fread( dest, 1, length, ss->file );
	return res;
}

void initFileFuncs()
{
	memset( &fileFuncs, 0, sizeof(struct StreamFuncs) );
	fileFuncs.getData = &fdGetData;
	fileFuncs.consumeData = &fdConsumeData;
	fileFuncs.undoConsumeData = &fdUndoConsumeData;
	fileFuncs.getDataSource = &fileGetDataSource;
}

/*
 * FD
 */

int fdGetDataSource( StreamImpl *ss, char *dest, int length )
{
	if ( ss->eof )
		return 0;
	else {
		long got = read( ss->fd, dest, length );
		if ( got == 0 )
			ss->eof = true;
		return got;
	}
}

void initFdFuncs()
{
	memset( &fdFuncs, 0, sizeof(struct StreamFuncs) );
	fdFuncs.getData = &fdGetData;
	fdFuncs.consumeData = &fdConsumeData;
	fdFuncs.undoConsumeData = &fdUndoConsumeData;
	fdFuncs.getDataSource = &fdGetDataSource;
}

/*
 * StreamImpl struct, this wraps the list of input streams.
 */

void initStreamImpl( StreamImpl *inputStream )
{
	memset( inputStream, 0, sizeof(StreamImpl) );

	inputStream->line = 1;
	inputStream->column = 1;
	inputStream->byte = 0;

	inputStream->funcs = &streamFuncs;
}

void clearStreamImpl( struct ColmProgram *prg, Tree **sp, StreamImpl *inputStream )
{
	RunBuf *buf = inputStream->queue;
	while ( buf != 0 ) {
		switch ( buf->type ) {
			case RunBufDataType:
				break;

			case RunBufTokenType:
			case RunBufIgnoreType:
			case RunBufSourceType:
				treeDownref( prg, sp, buf->tree );
				break;
		}

		RunBuf *next = buf->next;
		free( buf );
		buf = next;
	}

	inputStream->queue = 0;
}

static void inputStreamPrepend( StreamImpl *is, RunBuf *runBuf )
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

static RunBuf *inputStreamPopHead( StreamImpl *is )
{
	RunBuf *ret = is->queue;
	is->queue = is->queue->next;
	if ( is->queue == 0 )
		is->queueTail = 0;
	else
		is->queue->prev = 0;
	return ret;
}

static void inputStreamAppend( StreamImpl *is, RunBuf *runBuf )
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

static RunBuf *inputStreamPopTail( StreamImpl *is )
{
	RunBuf *ret = is->queueTail;
	is->queueTail = is->queueTail->prev;
	if ( is->queueTail == 0 )
		is->queue = 0;
	else
		is->queueTail->next = 0;
	return ret;
}

static int isSourceStream( StreamImpl *is )
{
	if ( is->queue != 0 && is->queue->type == RunBufSourceType )
		return true;
	return false;
}

void _setEof( StreamImpl *is )
{
	debug( REALM_INPUT, "setting EOF in input stream\n" );
	is->eof = true;
}

void _unsetEof( StreamImpl *is )
{
	if ( isSourceStream( is ) ) {
		Stream *stream = (Stream*)is->queue->tree;
		stream->in->eof = false;
	}
	else {
		is->eof = false;
	}
}

int _getData( FsmRun *fsmRun, StreamImpl *is, int skip, char *dest, int length, int *copied )
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
			int type = stream->in->funcs->getData( fsmRun, stream->in, skip, dest, length, copied );

			attachSource( fsmRun, stream->in );

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

		if ( buf->type == RunBufIgnoreType ) {
			ret = INPUT_IGNORE;
			break;
		}

		int avail = buf->length - buf->offset;

		/* Anything available in the current buffer. */
		if ( avail > 0 ) {
			/* The source data from the current buffer. */
			char *src = &buf->data[buf->offset];

			/* Need to skip? */
			if ( skip > 0 && skip >= avail ) {
				/* Skipping the the whole source. */
				skip -= avail;
			}
			else {
				/* Either skip is zero, or less than slen. Skip goes to zero.
				 * Some data left over, copy it. */
				src += skip;
				avail -= skip;
				skip = 0;

				int slen = avail <= length ? avail : length;
				memcpy( dest, src, slen ) ;
				*copied += slen;
				ret = INPUT_DATA;
				break;
			}
		}

		buf = buf->next;
	}

	attachInput( fsmRun, is );

#if DEBUG
	switch ( ret ) {
		case INPUT_DATA:
			debug( REALM_INPUT, "get data: DATA copied: %d: %.*s\n", *copied, (int)*copied, dest );
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
		case INPUT_IGNORE:
			debug( REALM_INPUT, "get data: IGNORE\n" );
			break;
		case INPUT_LANG_EL:
			debug( REALM_INPUT, "get data: LANG_EL\n" );
			break;
	}
#endif

	return ret;
}

int _consumeData( StreamImpl *is, int length )
{
	debug( REALM_INPUT, "consuming %d bytes\n", length );

	int consumed = 0;

	/* Move over skip bytes. */
	while ( true ) {
		RunBuf *buf = is->queue;

		if ( buf == 0 )
			break;

		if ( buf->type == RunBufSourceType ) {
			Stream *stream = (Stream*)buf->tree;
			int slen = stream->in->funcs->consumeData( stream->in, length );

			consumed += slen;
			length -= slen;
		}
		else if ( buf->type == RunBufTokenType )
			break;
		else if ( buf->type == RunBufIgnoreType )
			break;
		else {
			/* Anything available in the current buffer. */
			int avail = buf->length - buf->offset;
			if ( avail > 0 ) {
				/* The source data from the current buffer. */
				int slen = avail <= length ? avail : length;
				consumed += slen;
				length -= slen;
				buf->offset += slen;
			}
		}

		if ( length == 0 )
			break;

		RunBuf *runBuf = inputStreamPopHead( is );
		free( runBuf );
	}

	return consumed;
}

int _undoConsumeData( FsmRun *fsmRun, StreamImpl *is, const char *data, int length )
{
	debug( REALM_INPUT, "undoing consume of %ld bytes\n", length );

	if ( isSourceStream( is ) ) {
		Stream *stream = (Stream*)is->queue->tree;
		int len = stream->in->funcs->undoConsumeData( fsmRun, stream->in, data, length );

		if ( stream->in->attached != 0 )
			detachSource( stream->in->attached, stream->in );

		return len;
	}
	else {
		RunBuf *newBuf = newRunBuf();
		newBuf->length = length;
		memcpy( newBuf->data, data, length );
		inputStreamPrepend( is, newBuf );

		if ( is->attached != 0 )
			detachInput( is->attached, is );

		return length;
	}
}

Tree *_consumeTree( StreamImpl *is )
{
	while ( is->queue != 0 && is->queue->type == RunBufDataType && is->queue->offset == is->queue->length ) {
		RunBuf *runBuf = inputStreamPopHead( is );
		free( runBuf );
	}

	if ( is->queue != 0 && (is->queue->type == RunBufTokenType || is->queue->type == RunBufIgnoreType) ) {
		RunBuf *runBuf = inputStreamPopHead( is );

		/* FIXME: using runbufs here for this is a poor use of memory. */
		Tree *tree = runBuf->tree;
		free(runBuf);
		return tree;
	}

	return 0;
}

void _undoConsumeTree( StreamImpl *is, Tree *tree, int ignore )
{
	if ( is->attached != 0 )
		detachInput( is->attached, is );

	/* Create a new buffer for the data. This is the easy implementation.
	 * Something better is needed here. It puts a max on the amount of
	 * data that can be pushed back to the inputStream. */
	RunBuf *newBuf = newRunBuf();
	newBuf->type = ignore ? RunBufIgnoreType : RunBufTokenType;
	newBuf->tree = tree;
	inputStreamPrepend( is, newBuf );
}

struct LangEl *_consumeLangEl( StreamImpl *is, long *bindId, char **data, long *length )
{
	if ( isSourceStream( is ) ) {
		Stream *stream = (Stream*)is->queue->tree;
		return stream->in->funcs->consumeLangEl( stream->in, bindId, data, length );
	}
	else {
		assert( false );
	}
}

void _undoConsumeLangEl( StreamImpl *is )
{
	if ( isSourceStream( is ) ) {
		Stream *stream = (Stream*)is->queue->tree;
		return stream->in->funcs->undoConsumeLangEl( stream->in );
	}
	else {
		assert( false );
	}
}

void _prependData( StreamImpl *is, const char *data, long length )
{
	if ( is->attached != 0 )
		detachInput( is->attached, is );

	/* Create a new buffer for the data. This is the easy implementation.
	 * Something better is needed here. It puts a max on the amount of
	 * data that can be pushed back to the inputStream. */
	assert( length < FSM_BUFSIZE );

	RunBuf *newBuf = newRunBuf();
	newBuf->length = length;
	memcpy( newBuf->data, data, length );

	inputStreamPrepend( is, newBuf );
}

int _undoPrependData( StreamImpl *is, int length )
{
	if ( is->attached != 0 )
		detachInput( is->attached, is );

	debug( REALM_INPUT, "consuming %d bytes\n", length );

	int consumed = 0;

	/* Move over skip bytes. */
	while ( true ) {
		RunBuf *buf = is->queue;

		if ( buf == 0 )
			break;

		if ( buf->type == RunBufSourceType ) {
			Stream *stream = (Stream*)buf->tree;
			int slen = stream->in->funcs->consumeData( stream->in, length );

			consumed += slen;
			length -= slen;
		}
		else if ( buf->type == RunBufTokenType )
			break;
		else if ( buf->type == RunBufIgnoreType )
			break;
		else {
			/* Anything available in the current buffer. */
			int avail = buf->length - buf->offset;
			if ( avail > 0 ) {
				/* The source data from the current buffer. */
				int slen = avail <= length ? avail : length;
				consumed += slen;
				length -= slen;
				buf->offset += slen;
			}
		}

		if ( length == 0 )
			break;

		RunBuf *runBuf = inputStreamPopHead( is );
		free( runBuf );
	}

	return consumed;
}

void _prependTree( StreamImpl *is, Tree *tree, int ignore )
{
	if ( is->attached != 0 )
		detachInput( is->attached, is );

	/* Create a new buffer for the data. This is the easy implementation.
	 * Something better is needed here. It puts a max on the amount of
	 * data that can be pushed back to the inputStream. */
	RunBuf *newBuf = newRunBuf();
	newBuf->type = ignore ? RunBufIgnoreType : RunBufTokenType;
	newBuf->tree = tree;
	inputStreamPrepend( is, newBuf );
}

Tree *_undoPrependTree( StreamImpl *is )
{
	if ( is->attached != 0 )
		detachInput( is->attached, is );

	while ( is->queue != 0 && is->queue->type == RunBufDataType && is->queue->offset == is->queue->length ) {
		RunBuf *runBuf = inputStreamPopHead( is );
		free( runBuf );
	}

	if ( is->queue != 0 && (is->queue->type == RunBufTokenType || is->queue->type == RunBufIgnoreType) ) {
		RunBuf *runBuf = inputStreamPopHead( is );

		/* FIXME: using runbufs here for this is a poor use of memory. */
		Tree *tree = runBuf->tree;
		free(runBuf);
		return tree;
	}

	return 0;
}

void _appendData( StreamImpl *is, const char *data, long len )
{
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

Tree *_undoAppendData( StreamImpl *is, int length )
{
	if ( is->attached != 0 )
		detachInput( is->attached, is );

	int consumed = 0;

	/* Move over skip bytes. */
	while ( true ) {
		RunBuf *buf = is->queueTail;

		if ( buf == 0 )
			break;

		if ( buf->type == RunBufTokenType )
			break;
		else if ( buf->type == RunBufIgnoreType )
			break;
		else {
			/* Anything available in the current buffer. */
			int avail = buf->length - buf->offset;
			if ( avail > 0 ) {
				/* The source data from the current buffer. */
				int slen = avail <= length ? avail : length;
				consumed += slen;
				length -= slen;
				buf->length -= slen;
			}
		}

		if ( length == 0 )
			break;

		RunBuf *runBuf = inputStreamPopTail( is );
		free( runBuf );
	}

	return 0;
}

void _appendTree( StreamImpl *is, Tree *tree )
{
	RunBuf *ad = newRunBuf();

	inputStreamAppend( is, ad );

	ad->type = RunBufTokenType;
	ad->tree = tree;
	ad->length = 0;
}

void _appendStream( StreamImpl *in, struct ColmTree *tree )
{
	RunBuf *ad = newRunBuf();

	inputStreamAppend( in, ad );

	ad->type = RunBufSourceType;
	ad->tree = tree;
	ad->length = 0;
}

Tree *_undoAppendStream( StreamImpl *is )
{
	if ( is->attached != 0 )
		detachInput( is->attached, is );

	RunBuf *runBuf = inputStreamPopTail( is );
	Tree *tree = runBuf->tree;
	free( runBuf );
	return tree;
}

Tree *_undoAppendTree( StreamImpl *is )
{
	if ( is->attached != 0 )
		detachInput( is->attached, is );

	RunBuf *runBuf = inputStreamPopTail( is );
	Tree *tree = runBuf->tree;
	free( runBuf );
	return tree;
}
