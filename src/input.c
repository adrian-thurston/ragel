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
#include <colm/tree.h>
#include <colm/bytecode.h>
#include <colm/pool.h>
#include <colm/struct.h>

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

void colm_clear_source_stream( struct colm_program *prg,
		tree_t **sp, struct stream_impl *sourceStream )
{
	RunBuf *buf = sourceStream->queue;
	while ( buf != 0 ) {
		switch ( buf->type ) {
			case RunBufDataType:
				break;

			case RunBufTokenType:
			case RunBufIgnoreType:
				treeDownref( prg, sp, buf->tree );
				break;
			case RunBufSourceType:
				break;
		}

		RunBuf *next = buf->next;
		free( buf );
		buf = next;
	}

	sourceStream->queue = 0;
}

void colm_stream_destroy( program_t *prg, tree_t **sp, struct_t *s )
{
	stream_t *stream = (stream_t*) s;
	colm_clear_source_stream( prg, sp, stream->impl );

	if ( stream->impl->file != 0 )
		fclose( stream->impl->file );
	else if ( stream->impl->fd >= 0 )
		close( stream->impl->fd );

	free( stream->impl );
}


/* Keep the position up to date after consuming text. */
void updatePosition( struct stream_impl *is, const char *data, long length )
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
void undoPosition( struct stream_impl *is, const char *data, long length )
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


static RunBuf *sourceStreamPopHead( struct stream_impl *ss )
{
	RunBuf *ret = ss->queue;
	ss->queue = ss->queue->next;
	if ( ss->queue == 0 )
		ss->queueTail = 0;
	else
		ss->queue->prev = 0;
	return ret;
}

static void sourceStreamAppend( struct stream_impl *ss, RunBuf *runBuf )
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

static void sourceStreamPrepend( struct stream_impl *ss, RunBuf *runBuf )
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

int fdGetParseBlock( struct stream_impl *ss, int skip, char **pdp, int *copied )
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

int fdGetData( struct stream_impl *ss, char *dest, int length )
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

int fdConsumeData( program_t *prg, tree_t **sp, struct stream_impl *ss, int length, location_t *loc )
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

int fdUndoConsumeData( struct stream_impl *ss, const char *data, int length )
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

int fileGetDataSource( struct stream_impl *ss, char *dest, int length )
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

int fdGetDataSource( struct stream_impl *ss, char *dest, int length )
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

void initStreamImpl( struct stream_impl *is, const char *name )
{
	memset( is, 0, sizeof(struct stream_impl) );

	is->name = name;
	is->line = 1;
	is->column = 1;
	is->byte = 0;
}

void colm_clear_stream_impl( struct colm_program *prg, tree_t **sp, struct stream_impl *inputStream )
{
	RunBuf *buf = inputStream->queue;
	while ( buf != 0 ) {
		switch ( buf->type ) {
			case RunBufDataType:
				break;

			case RunBufTokenType:
			case RunBufIgnoreType:
				treeDownref( prg, sp, buf->tree );
				break;

			case RunBufSourceType:
				break;
		}

		RunBuf *next = buf->next;
		free( buf );
		buf = next;
	}

	inputStream->queue = 0;
}

static void inputStreamPrepend( struct stream_impl *is, RunBuf *runBuf )
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

static RunBuf *inputStreamPopHead( struct stream_impl *is )
{
	RunBuf *ret = is->queue;
	is->queue = is->queue->next;
	if ( is->queue == 0 )
		is->queueTail = 0;
	else
		is->queue->prev = 0;
	return ret;
}

static void inputStreamAppend( struct stream_impl *is, RunBuf *runBuf )
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

static RunBuf *inputStreamPopTail( struct stream_impl *is )
{
	RunBuf *ret = is->queueTail;
	is->queueTail = is->queueTail->prev;
	if ( is->queueTail == 0 )
		is->queue = 0;
	else
		is->queueTail->next = 0;
	return ret;
}

static int isSourceStream( struct stream_impl *is )
{
	if ( is->queue != 0 && is->queue->type == RunBufSourceType )
		return true;
	return false;
}

static void _setEof( struct stream_impl *is )
{
	//debug( REALM_INPUT, "setting EOF in input stream\n" );
	is->eof = true;
}

static void _unsetEof( struct stream_impl *is )
{
	if ( isSourceStream( is ) ) {
		struct stream_impl *si = streamToImpl( (stream_t*)is->queue->tree );
		si->eof = false;
	}
	else {
		is->eof = false;
	}
}

static int _getParseBlock( struct stream_impl *is, int skip, char **pdp, int *copied )
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
			struct stream_impl *si = streamToImpl( (stream_t*)buf->tree );
			int type = si->funcs->getParseBlock( si, skip, pdp, copied );

//			if ( type == INPUT_EOD && !si->eosSent ) {
//				si->eosSent = 1;
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

static int _getData( struct stream_impl *is, char *dest, int length )
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
			struct stream_impl *si = streamToImpl( (stream_t*)buf->tree );
			int glen = si->funcs->getData( si, dest+copied, length );

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

static int _consumeData( program_t *prg, tree_t **sp, struct stream_impl *is,
		int length, location_t *loc )
{
	//debug( REALM_INPUT, "consuming %d bytes\n", length );

	int consumed = 0;

	/* Move over skip bytes. */
	while ( true ) {
		RunBuf *buf = is->queue;

		if ( buf == 0 )
			break;

		if ( buf->type == RunBufSourceType ) {
			struct stream_impl *si = streamToImpl( (stream_t*)buf->tree );
			int slen = si->funcs->consumeData( prg, sp, si, length, loc );
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
		//if ( runBuf->type == RunBufSourceType ) {
		//	stream_t *stream = (stream_t*)runBuf->tree;
		//	treeDownref( prg, sp, (tree_t*) stream );
		//}
		free( runBuf );
	}

	return consumed;
}

static int _undoConsumeData( struct stream_impl *is, const char *data, int length )
{
	//debug( REALM_INPUT, "undoing consume of %ld bytes\n", length );

	if ( is->consumed == 0 && isSourceStream( is ) ) {
		struct stream_impl *si = streamToImpl( (stream_t*)is->queue->tree );
		int len = si->funcs->undoConsumeData( si, data, length );
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

static tree_t *_consumeTree( struct stream_impl *is )
{
	while ( is->queue != 0 && is->queue->type == RunBufDataType && 
			is->queue->offset == is->queue->length )
	{
		RunBuf *runBuf = inputStreamPopHead( is );
		free( runBuf );
	}

	if ( is->queue != 0 && (is->queue->type == RunBufTokenType || 
			is->queue->type == RunBufIgnoreType) )
	{
		RunBuf *runBuf = inputStreamPopHead( is );

		/* FIXME: using runbufs here for this is a poor use of memory. */
		tree_t *tree = runBuf->tree;
		free(runBuf);
		return tree;
	}

	return 0;
}

static void _undoConsumeTree( struct stream_impl *is, tree_t *tree, int ignore )
{
	/* Create a new buffer for the data. This is the easy implementation.
	 * Something better is needed here. It puts a max on the amount of
	 * data that can be pushed back to the inputStream. */
	RunBuf *newBuf = newRunBuf();
	newBuf->type = ignore ? RunBufIgnoreType : RunBufTokenType;
	newBuf->tree = tree;
	inputStreamPrepend( is, newBuf );
}

static struct LangEl *_consumeLangEl( struct stream_impl *is, long *bindId,
		char **data, long *length )
{
	if ( isSourceStream( is ) ) {
		struct stream_impl *si = streamToImpl( (stream_t*)is->queue->tree );
		return si->funcs->consumeLangEl( si, bindId, data, length );
	}
	else {
		assert( false );
	}
}

static void _undoConsumeLangEl( struct stream_impl *is )
{
	if ( isSourceStream( is ) ) {
		struct stream_impl *si = streamToImpl( (stream_t*)is->queue->tree );
		return si->funcs->undoConsumeLangEl( si );
	}
	else {
		assert( false );
	}
}

static void _prependData( struct stream_impl *is, const char *data, long length )
{
	if ( isSourceStream( is ) && 
			streamToImpl((stream_t*)is->queue->tree)->funcs == &streamFuncs )
	{
		_prependData( streamToImpl( (stream_t*)is->queue->tree ), data, length );
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

static void _prependTree( struct stream_impl *is, tree_t *tree, int ignore )
{
	/* Create a new buffer for the data. This is the easy implementation.
	 * Something better is needed here. It puts a max on the amount of
	 * data that can be pushed back to the inputStream. */
	RunBuf *newBuf = newRunBuf();
	newBuf->type = ignore ? RunBufIgnoreType : RunBufTokenType;
	newBuf->tree = tree;
	inputStreamPrepend( is, newBuf );
}

static void _prependStream( struct stream_impl *in, struct colm_tree *tree )
{
	/* Create a new buffer for the data. This is the easy implementation.
	 * Something better is needed here. It puts a max on the amount of
	 * data that can be pushed back to the inputStream. */
	RunBuf *newBuf = newRunBuf();
	newBuf->type = RunBufSourceType;
	newBuf->tree = tree;
	inputStreamPrepend( in, newBuf );
}

static int _undoPrependData( struct stream_impl *is, int length )
{
	//debug( REALM_INPUT, "consuming %d bytes\n", length );

	int consumed = 0;

	/* Move over skip bytes. */
	while ( true ) {
		RunBuf *buf = is->queue;

		if ( buf == 0 )
			break;

		if ( buf->type == RunBufSourceType ) {
			struct stream_impl *si = streamToImpl( (stream_t*)buf->tree );
			int slen = si->funcs->undoPrependData( si, length );

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

static tree_t *_undoPrependTree( struct stream_impl *is )
{
	while ( is->queue != 0 && is->queue->type == RunBufDataType &&
			is->queue->offset == is->queue->length )
	{
		RunBuf *runBuf = inputStreamPopHead( is );
		free( runBuf );
	}

	if ( is->queue != 0 && (is->queue->type == RunBufTokenType ||
			is->queue->type == RunBufIgnoreType) )
	{
		RunBuf *runBuf = inputStreamPopHead( is );

		/* FIXME: using runbufs here for this is a poor use of memory. */
		tree_t *tree = runBuf->tree;
		free(runBuf);
		return tree;
	}

	return 0;
}

static void _appendData( struct stream_impl *is, const char *data, long len )
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

static tree_t *_undoAppendData( struct stream_impl *is, int length )
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

static void _appendTree( struct stream_impl *is, tree_t *tree )
{
	RunBuf *ad = newRunBuf();

	inputStreamAppend( is, ad );

	ad->type = RunBufTokenType;
	ad->tree = tree;
	ad->length = 0;
}

static void _appendStream( struct stream_impl *in, struct colm_tree *tree )
{
	RunBuf *ad = newRunBuf();

	inputStreamAppend( in, ad );

	ad->type = RunBufSourceType;
	ad->tree = tree;
	ad->length = 0;
}

static tree_t *_undoAppendTree( struct stream_impl *is )
{
	RunBuf *runBuf = inputStreamPopTail( is );
	tree_t *tree = runBuf->tree;
	free( runBuf );
	return tree;
}

static tree_t *_undoAppendStream( struct stream_impl *is )
{
	RunBuf *runBuf = inputStreamPopTail( is );
	tree_t *tree = runBuf->tree;
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


struct stream_impl *colm_impl_new_file( const char *name, FILE *file )
{
	struct stream_impl *ss = (struct stream_impl*)malloc(sizeof(struct stream_impl));
	initStreamImpl( ss, name );
	ss->funcs = &fileFuncs;

	ss->file = file;

	return ss;
}

struct stream_impl *colm_impl_new_fd( const char *name, long fd )
{
	struct stream_impl *ss = (struct stream_impl*)malloc(sizeof(struct stream_impl));
	initStreamImpl( ss, name );
	ss->funcs = &fdFuncs;

	ss->fd = fd;

	return ss;
}

struct stream_impl *colm_impl_new_generic( const char *name )
{
	struct stream_impl *ss = (struct stream_impl*)malloc(sizeof(struct stream_impl));
	initStreamImpl( ss, name );
	ss->funcs = &streamFuncs;

	return ss;
}

stream_t *colm_stream_new_struct( program_t *prg )
{
	size_t memsize = sizeof(struct colm_stream);
	struct colm_stream *stream = (struct colm_stream*) malloc( memsize );
	memset( stream, 0, memsize );
	colm_struct_add( prg, (struct colm_struct *)stream );
	stream->id = STRUCT_INBUILT_ID;
	stream->destructor = &colm_stream_destroy;
	return stream;
}

stream_t *colm_stream_open_fd( program_t *prg, char *name, long fd )
{
	struct stream_impl *impl = colm_impl_new_fd( name, fd );

	struct colm_stream *s = colm_stream_new_struct( prg );
	s->impl = impl;
	return s;
}

stream_t *colm_stream_open_file( program_t *prg, tree_t *name, tree_t *mode )
{
	head_t *headName = ((str_t*)name)->value;
	head_t *headMode = ((str_t*)mode)->value;
	stream_t *stream = 0;

	const char *givenMode = stringData(headMode);
	const char *fopenMode = 0;
	if ( memcmp( givenMode, "r", stringLength(headMode) ) == 0 )
		fopenMode = "rb";
	else if ( memcmp( givenMode, "w", stringLength(headMode) ) == 0 )
		fopenMode = "wb";
	else if ( memcmp( givenMode, "a", stringLength(headMode) ) == 0 )
		fopenMode = "ab";
	else {
		fatal( "unknown file open mode: %s\n", givenMode );
	}
	
	/* Need to make a C-string (null terminated). */
	char *fileName = (char*)malloc(stringLength(headName)+1);
	memcpy( fileName, stringData(headName), stringLength(headName) );
	fileName[stringLength(headName)] = 0;
	FILE *file = fopen( fileName, fopenMode );
	if ( file != 0 ) {
		stream = colm_stream_new_struct( prg );
		stream->impl = colm_impl_new_file( fileName, file );
	}

	return stream;
}

stream_t *colm_stream_new( program_t *prg )
{
	struct stream_impl *impl = colm_impl_new_generic( "<internal>" );

	struct colm_stream *stream = colm_stream_new_struct( prg );
	stream->impl = impl;
	return stream;
}

struct stream_impl *colm_stream_impl( struct colm_struct *s )
{
	return ((stream_t*)s)->impl;
}

struct stream_impl *streamToImpl( stream_t *ptr )
{
	return ptr->impl;
}
