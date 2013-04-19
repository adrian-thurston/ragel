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
#include <colm/pdarun.h>
#include <colm/debug.h>
#include <colm/program.h>

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

void initFdFuncs();
void initFileFuncs();
void initPatFuncs();
void initConsFuncs();

extern struct StreamFuncs fileFuncs;
extern struct StreamFuncs fdFuncs;
extern struct StreamFuncs streamFuncs;

void clearSourceStream( struct colm_program *prg, Tree **sp, StreamImpl *sourceStream )
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

/* Keep the position up to date after consuming text. */
void updatePosition( StreamImpl *is, const char *data, long length )
{
	int i;
	for ( i = 0; i < length; i++ ) {
		if ( data[i] != '\n' )
			is->column += 1;
		else {
			is->line += 1;
			is->column = 1;
		}
	}

	is->byte += length;
}

/* Keep the position up to date after sending back text. */
void undoPosition( StreamImpl *is, const char *data, long length )
{
	/* FIXME: this needs to fetch the position information from the parsed
	 * token and restore based on that.. */
	int i;
	for ( i = 0; i < length; i++ ) {
		if ( data[i] == '\n' )
			is->line -= 1;
	}

	is->byte -= length;
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

/* 
 * Base run-time input streams.
 */

int fdGetParseBlock( StreamImpl *ss, int skip, char **pdp, int *copied )
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

			int slen = received;
			*pdp = runBuf->data;
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

				int slen = avail;
				*pdp = src;
				*copied += slen;
				ret = INPUT_DATA;
				break;
			}
		}

		buf = buf->next;
	}

	return ret;
}

int fdGetData( StreamImpl *ss, char *dest, int length )
{
	int copied = 0;

	/* Move over skip bytes. */
	RunBuf *buf = ss->queue;
	while ( true ) {
		if ( buf == 0 ) {
			/* Got through the in-mem buffers without copying anything. */
			RunBuf *runBuf = newRunBuf();
			sourceStreamAppend( ss, runBuf );
			int received = ss->funcs->getDataSource( ss, runBuf->data, FSM_BUFSIZE );
			runBuf->length = received;
			if ( received == 0 )
				break;

			buf = runBuf;
		}

		int avail = buf->length - buf->offset;

		/* Anything available in the current buffer. */
		if ( avail > 0 ) {
			/* The source data from the current buffer. */
			char *src = &buf->data[buf->offset];

			int slen = avail < length ? avail : length;
			memcpy( dest+copied, src, slen ) ;
			copied += slen;
			length -= slen;
		}

		if ( length == 0 ) {
			//debug( REALM_INPUT, "exiting get data\n", length );
			break;
		}

		buf = buf->next;
	}

	return copied;
}

int fdConsumeData( StreamImpl *ss, int length, Location *loc )
{
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
			if ( loc->line == 0 ) {
				loc->name = ss->name;
				loc->line = ss->line;
				loc->column = ss->column;
				loc->byte = ss->byte;
			}

			/* Anything available in the current buffer. */
			int avail = buf->length - buf->offset;
			if ( avail > 0 ) {
				/* The source data from the current buffer. */
				int slen = avail <= length ? avail : length;
				consumed += slen;
				length -= slen;
				updatePosition( ss, buf->data + buf->offset, slen );
				buf->offset += slen;
				ss->consumed += slen;
			}
		}

		if ( length == 0 )
			break;

		RunBuf *runBuf = sourceStreamPopHead( ss );
		free( runBuf );
	}

	return consumed;
}

int fdUndoConsumeData( StreamImpl *ss, const char *data, int length )
{
	RunBuf *newBuf = newRunBuf();
	newBuf->length = length;
	memcpy( newBuf->data, data, length );
	sourceStreamPrepend( ss, newBuf );
	undoPosition( ss, data, length );
	ss->consumed -= length;

	return length;
}

/*
 * File
 */

int fileGetDataSource( StreamImpl *ss, char *dest, int length )
{
	//debug( REALM_INPUT, "inputStreamFileGetDataSource length = %ld\n", length );
	size_t res = fread( dest, 1, length, ss->file );
	return res;
}

void initFileFuncs()
{
	memset( &fileFuncs, 0, sizeof(struct StreamFuncs) );
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

/*
 * StreamImpl struct, this wraps the list of input streams.
 */

void initStreamImpl( StreamImpl *is, const char *name )
{
	memset( is, 0, sizeof(StreamImpl) );

	is->name = name;
	is->line = 1;
	is->column = 1;
	is->byte = 0;
}

void clearStreamImpl( struct colm_program *prg, Tree **sp, StreamImpl *inputStream )
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

static void _setEof( StreamImpl *is )
{
	//debug( REALM_INPUT, "setting EOF in input stream\n" );
	is->eof = true;
}

static void _unsetEof( StreamImpl *is )
{
	if ( isSourceStream( is ) ) {
		Stream *stream = (Stream*)is->queue->tree;
		stream->in->eof = false;
	}
	else {
		is->eof = false;
	}
}

static int _getParseBlock( StreamImpl *is, int skip, char **pdp, int *copied )
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
			int type = stream->in->funcs->getParseBlock( stream->in, skip, pdp, copied );

//			if ( type == INPUT_EOD && !stream->in->eosSent ) {
//				stream->in->eosSent = 1;
//				ret = INPUT_EOS;
//				continue;
//			}

			if ( type == INPUT_EOD || type == INPUT_EOF ) {
				//debug( REALM_INPUT, "skipping over input\n" );
				buf = buf->next;
				continue;
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

				*pdp = src;
				*copied += avail;
				ret = INPUT_DATA;
				break;
			}
		}

		buf = buf->next;
	}

#if DEBUG
	switch ( ret ) {
		case INPUT_DATA:
			//debug( REALM_INPUT, "get parse block: DATA: %d\n", *copied );
			break;
		case INPUT_EOD:
			//debug( REALM_INPUT, "get parse block: EOD\n" );
			break;
		case INPUT_EOF:
			//debug( REALM_INPUT, "get parse block: EOF\n" );
			break;
		case INPUT_TREE:
			//debug( REALM_INPUT, "get parse block: TREE\n" );
			break;
		case INPUT_IGNORE:
			//debug( REALM_INPUT, "get parse block: IGNORE\n" );
			break;
		case INPUT_LANG_EL:
			//debug( REALM_INPUT, "get parse block: LANG_EL\n" );
			break;
	}
#endif

	return ret;
}

static int _getData( StreamImpl *is, char *dest, int length )
{
	int copied = 0;

	/* Move over skip bytes. */
	RunBuf *buf = is->queue;
	while ( true ) {
		if ( buf == 0 ) {
			/* Got through the in-mem buffers without copying anything. */
			break;
		}

		if ( buf->type == RunBufSourceType ) {
			Stream *stream = (Stream*)buf->tree;
			int glen = stream->in->funcs->getData( stream->in, dest+copied, length );

			if ( glen == 0 ) {
				//debug( REALM_INPUT, "skipping over input\n" );
				buf = buf->next;
				continue;
			}

			copied += glen;
			length -= glen;
		}
		else if ( buf->type == RunBufTokenType )
			break;
		else if ( buf->type == RunBufIgnoreType )
			break;
		else {
			int avail = buf->length - buf->offset;

			/* Anything available in the current buffer. */
			if ( avail > 0 ) {
				/* The source data from the current buffer. */
				char *src = &buf->data[buf->offset];

				int slen = avail <= length ? avail : length;
				memcpy( dest+copied, src, slen ) ;

				copied += slen;
				length -= slen;
			}
		}

		if ( length == 0 ) {
			//debug( REALM_INPUT, "exiting get data\n", length );
			break;
		}

		buf = buf->next;
	}

	return copied;
}

static int _consumeData( StreamImpl *is, int length, Location *loc )
{
	//debug( REALM_INPUT, "consuming %d bytes\n", length );

	int consumed = 0;

	/* Move over skip bytes. */
	while ( true ) {
		RunBuf *buf = is->queue;

		if ( buf == 0 )
			break;

		if ( buf->type == RunBufSourceType ) {
			Stream *stream = (Stream*)buf->tree;
			int slen = stream->in->funcs->consumeData( stream->in, length, loc );
			//debug( REALM_INPUT, " got %d bytes from source\n", slen );

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
				is->consumed += slen;
			}
		}

		if ( length == 0 ) {
			//debug( REALM_INPUT, "exiting consume\n", length );
			break;
		}

		RunBuf *runBuf = inputStreamPopHead( is );
		free( runBuf );
	}

	return consumed;
}

static int _undoConsumeData( StreamImpl *is, const char *data, int length )
{
	//debug( REALM_INPUT, "undoing consume of %ld bytes\n", length );

	if ( is->consumed == 0 && isSourceStream( is ) ) {
		Stream *stream = (Stream*)is->queue->tree;
		int len = stream->in->funcs->undoConsumeData( stream->in, data, length );
		return len;
	}
	else {
		RunBuf *newBuf = newRunBuf();
		newBuf->length = length;
		memcpy( newBuf->data, data, length );
		inputStreamPrepend( is, newBuf );
		is->consumed -= length;

		return length;
	}
}

static Tree *_consumeTree( StreamImpl *is )
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

static void _undoConsumeTree( StreamImpl *is, Tree *tree, int ignore )
{
	/* Create a new buffer for the data. This is the easy implementation.
	 * Something better is needed here. It puts a max on the amount of
	 * data that can be pushed back to the inputStream. */
	RunBuf *newBuf = newRunBuf();
	newBuf->type = ignore ? RunBufIgnoreType : RunBufTokenType;
	newBuf->tree = tree;
	inputStreamPrepend( is, newBuf );
}

static struct LangEl *_consumeLangEl( StreamImpl *is, long *bindId, char **data, long *length )
{
	if ( isSourceStream( is ) ) {
		Stream *stream = (Stream*)is->queue->tree;
		return stream->in->funcs->consumeLangEl( stream->in, bindId, data, length );
	}
	else {
		assert( false );
	}
}

static void _undoConsumeLangEl( StreamImpl *is )
{
	if ( isSourceStream( is ) ) {
		Stream *stream = (Stream*)is->queue->tree;
		return stream->in->funcs->undoConsumeLangEl( stream->in );
	}
	else {
		assert( false );
	}
}

static void _prependData( StreamImpl *is, const char *data, long length )
{
	if ( isSourceStream( is ) && ((Stream*)is->queue->tree)->in->funcs == &streamFuncs ) {
		Stream *stream = (Stream*)is->queue->tree;

		_prependData( stream->in, data, length );
	}
	else {
		/* Create a new buffer for the data. This is the easy implementation.
		 * Something better is needed here. It puts a max on the amount of
		 * data that can be pushed back to the inputStream. */
		assert( length < FSM_BUFSIZE );

		RunBuf *newBuf = newRunBuf();
		newBuf->length = length;
		memcpy( newBuf->data, data, length );

		inputStreamPrepend( is, newBuf );
	}
}

static void _prependTree( StreamImpl *is, Tree *tree, int ignore )
{
	/* Create a new buffer for the data. This is the easy implementation.
	 * Something better is needed here. It puts a max on the amount of
	 * data that can be pushed back to the inputStream. */
	RunBuf *newBuf = newRunBuf();
	newBuf->type = ignore ? RunBufIgnoreType : RunBufTokenType;
	newBuf->tree = tree;
	inputStreamPrepend( is, newBuf );
}

static void _prependStream( StreamImpl *in, struct colm_tree *tree )
{
	/* Create a new buffer for the data. This is the easy implementation.
	 * Something better is needed here. It puts a max on the amount of
	 * data that can be pushed back to the inputStream. */
	RunBuf *newBuf = newRunBuf();
	newBuf->type = RunBufSourceType;
	newBuf->tree = tree;
	inputStreamPrepend( in, newBuf );
}

static int _undoPrependData( StreamImpl *is, int length )
{
	//debug( REALM_INPUT, "consuming %d bytes\n", length );

	int consumed = 0;

	/* Move over skip bytes. */
	while ( true ) {
		RunBuf *buf = is->queue;

		if ( buf == 0 )
			break;

		if ( buf->type == RunBufSourceType ) {
			Stream *stream = (Stream*)buf->tree;
			int slen = stream->in->funcs->undoPrependData( stream->in, length );

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

static Tree *_undoPrependTree( StreamImpl *is )
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

static void _appendData( StreamImpl *is, const char *data, long len )
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

static Tree *_undoAppendData( StreamImpl *is, int length )
{
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

static void _appendTree( StreamImpl *is, Tree *tree )
{
	RunBuf *ad = newRunBuf();

	inputStreamAppend( is, ad );

	ad->type = RunBufTokenType;
	ad->tree = tree;
	ad->length = 0;
}

static void _appendStream( StreamImpl *in, struct colm_tree *tree )
{
	RunBuf *ad = newRunBuf();

	inputStreamAppend( in, ad );

	ad->type = RunBufSourceType;
	ad->tree = tree;
	ad->length = 0;
}

static Tree *_undoAppendTree( StreamImpl *is )
{
	RunBuf *runBuf = inputStreamPopTail( is );
	Tree *tree = runBuf->tree;
	free( runBuf );
	return tree;
}

static Tree *_undoAppendStream( StreamImpl *is )
{
	RunBuf *runBuf = inputStreamPopTail( is );
	Tree *tree = runBuf->tree;
	free( runBuf );
	return tree;
}

struct StreamFuncs streamFuncs = 
{
	&_getParseBlock,
	&_getData,
	&_consumeData,
	&_undoConsumeData,
	&_consumeTree,
	&_undoConsumeTree,
	&_consumeLangEl,
	&_undoConsumeLangEl,
	0, // source data get, not needed.
	&_setEof,
	&_unsetEof,
	&_prependData,
	&_prependTree,
	&_prependStream,
	&_undoPrependData,
	&_undoPrependTree,
	0, // FIXME: Add this.
	&_appendData,
	&_appendTree,
	&_appendStream,
	&_undoAppendData,
	&_undoAppendTree,
	&_undoAppendStream,
};

struct StreamFuncs fdFuncs = 
{
	.getData = &fdGetData,
	.getParseBlock = &fdGetParseBlock,
	.consumeData = &fdConsumeData,
	.undoConsumeData = &fdUndoConsumeData,
	.getDataSource = &fdGetDataSource,
};

struct StreamFuncs fileFuncs = 
{
	.getData = &fdGetData,
	.getParseBlock = &fdGetParseBlock,
	.consumeData = &fdConsumeData,
	.undoConsumeData = &fdUndoConsumeData,
	.getDataSource = &fileGetDataSource,
};


StreamImpl *newSourceStreamFile( const char *name, FILE *file )
{
	StreamImpl *ss = (StreamImpl*)malloc(sizeof(StreamImpl));
	initStreamImpl( ss, name );
	ss->funcs = &fileFuncs;

	ss->file = file;

	return ss;
}

StreamImpl *newSourceStreamFd( const char *name, long fd )
{
	StreamImpl *ss = (StreamImpl*)malloc(sizeof(StreamImpl));
	initStreamImpl( ss, name );
	ss->funcs = &fdFuncs;

	ss->fd = fd;

	return ss;
}

StreamImpl *newSourceStreamGeneric( const char *name )
{
	StreamImpl *ss = (StreamImpl*)malloc(sizeof(StreamImpl));
	initStreamImpl( ss, name );
	ss->funcs = &streamFuncs;

	return ss;
}
