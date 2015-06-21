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

RunBuf *new_run_buf()
{
	RunBuf *rb = (RunBuf*)malloc(sizeof(RunBuf));
	memset( rb, 0, sizeof(RunBuf) );
	return rb;
}

void init_fd_funcs();
void init_file_funcs();
void init_pat_funcs();
void init_cons_funcs();

extern struct stream_funcs file_funcs;
extern struct stream_funcs fd_funcs;
extern struct stream_funcs stream_funcs;

void colm_clear_source_stream( struct colm_program *prg,
		tree_t **sp, struct stream_impl *source_stream )
{
	RunBuf *buf = source_stream->queue;
	while ( buf != 0 ) {
		switch ( buf->type ) {
			case RunBufDataType:
				break;

			case RunBufTokenType:
			case RunBufIgnoreType:
				colm_tree_downref( prg, sp, buf->tree );
				break;
			case RunBufSourceType:
				break;
		}

		RunBuf *next = buf->next;
		free( buf );
		buf = next;
	}

	source_stream->queue = 0;
}

void colm_stream_destroy( program_t *prg, tree_t **sp, struct_t *s )
{
	stream_t *stream = (stream_t*) s;
	colm_clear_source_stream( prg, sp, stream->impl );

	if ( stream->impl->file != 0 )
		fclose( stream->impl->file );

	free( stream->impl );
}


/* Keep the position up to date after consuming text. */
void update_position( struct stream_impl *is, const char *data, long length )
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
void undo_position( struct stream_impl *is, const char *data, long length )
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


static RunBuf *source_stream_pop_head( struct stream_impl *ss )
{
	RunBuf *ret = ss->queue;
	ss->queue = ss->queue->next;
	if ( ss->queue == 0 )
		ss->queue_tail = 0;
	else
		ss->queue->prev = 0;
	return ret;
}

static void source_stream_append( struct stream_impl *ss, RunBuf *run_buf )
{
	if ( ss->queue == 0 ) {
		run_buf->prev = run_buf->next = 0;
		ss->queue = ss->queue_tail = run_buf;
	}
	else {
		ss->queue_tail->next = run_buf;
		run_buf->prev = ss->queue_tail;
		run_buf->next = 0;
		ss->queue_tail = run_buf;
	}
}

static void source_stream_prepend( struct stream_impl *ss, RunBuf *run_buf )
{
	if ( ss->queue == 0 ) {
		run_buf->prev = run_buf->next = 0;
		ss->queue = ss->queue_tail = run_buf;
	}
	else {
		ss->queue->prev = run_buf;
		run_buf->prev = 0;
		run_buf->next = ss->queue;
		ss->queue = run_buf;
	}
}

/* 
 * Base run-time input streams.
 */

int file_get_parse_block( struct stream_impl *ss, int skip, char **pdp, int *copied )
{
	int ret = 0;
	*copied = 0;

	/* Move over skip bytes. */
	RunBuf *buf = ss->queue;
	while ( true ) {
		if ( buf == 0 ) {
			/* Got through the in-mem buffers without copying anything. */
			RunBuf *run_buf = new_run_buf();
			source_stream_append( ss, run_buf );
			int received = ss->funcs->get_data_source( ss, run_buf->data, FSM_BUFSIZE );
			if ( received == 0 ) {
				ret = INPUT_EOD;
				break;
			}
			run_buf->length = received;

			int slen = received;
			*pdp = run_buf->data;
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

int file_get_data( struct stream_impl *ss, char *dest, int length )
{
	int copied = 0;

	/* Move over skip bytes. */
	RunBuf *buf = ss->queue;
	while ( true ) {
		if ( buf == 0 ) {
			/* Got through the in-mem buffers without copying anything. */
			RunBuf *run_buf = new_run_buf();
			source_stream_append( ss, run_buf );
			int received = ss->funcs->get_data_source( ss, run_buf->data, FSM_BUFSIZE );
			run_buf->length = received;
			if ( received == 0 )
				break;

			buf = run_buf;
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

int file_consume_data( program_t *prg, tree_t **sp,
		struct stream_impl *ss, int length, location_t *loc )
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
				update_position( ss, buf->data + buf->offset, slen );
				buf->offset += slen;
				ss->consumed += slen;
			}
		}

		if ( length == 0 )
			break;

		RunBuf *run_buf = source_stream_pop_head( ss );
		free( run_buf );
	}

	return consumed;
}

int file_undo_consume_data( struct stream_impl *ss, const char *data, int length )
{
	RunBuf *new_buf = new_run_buf();
	new_buf->length = length;
	memcpy( new_buf->data, data, length );
	source_stream_prepend( ss, new_buf );
	undo_position( ss, data, length );
	ss->consumed -= length;

	return length;
}

/*
 * File
 */

int file_get_data_source( struct stream_impl *ss, char *dest, int length )
{
	size_t read = fread( dest, 1, length, ss->file );
	return read;
}

void init_file_funcs()
{
	memset( &file_funcs, 0, sizeof(struct stream_funcs) );
}

/*
 * StreamImpl struct, this wraps the list of input streams.
 */

void init_stream_impl( struct stream_impl *is, const char *name )
{
	memset( is, 0, sizeof(struct stream_impl) );

	is->name = name;
	is->line = 1;
	is->column = 1;
	is->byte = 0;
}

void colm_clear_stream_impl( struct colm_program *prg, tree_t **sp, struct stream_impl *input_stream )
{
	RunBuf *buf = input_stream->queue;
	while ( buf != 0 ) {
		switch ( buf->type ) {
			case RunBufDataType:
				break;

			case RunBufTokenType:
			case RunBufIgnoreType:
				colm_tree_downref( prg, sp, buf->tree );
				break;

			case RunBufSourceType:
				break;
		}

		RunBuf *next = buf->next;
		free( buf );
		buf = next;
	}

	input_stream->queue = 0;
}

static void input_stream_prepend( struct stream_impl *is, RunBuf *run_buf )
{
	if ( is->queue == 0 ) {
		run_buf->prev = run_buf->next = 0;
		is->queue = is->queue_tail = run_buf;
	}
	else {
		is->queue->prev = run_buf;
		run_buf->prev = 0;
		run_buf->next = is->queue;
		is->queue = run_buf;
	}
}

static RunBuf *input_stream_pop_head( struct stream_impl *is )
{
	RunBuf *ret = is->queue;
	is->queue = is->queue->next;
	if ( is->queue == 0 )
		is->queue_tail = 0;
	else
		is->queue->prev = 0;
	return ret;
}

static void input_stream_append( struct stream_impl *is, RunBuf *run_buf )
{
	if ( is->queue == 0 ) {
		run_buf->prev = run_buf->next = 0;
		is->queue = is->queue_tail = run_buf;
	}
	else {
		is->queue_tail->next = run_buf;
		run_buf->prev = is->queue_tail;
		run_buf->next = 0;
		is->queue_tail = run_buf;
	}
}

static RunBuf *input_stream_pop_tail( struct stream_impl *is )
{
	RunBuf *ret = is->queue_tail;
	is->queue_tail = is->queue_tail->prev;
	if ( is->queue_tail == 0 )
		is->queue = 0;
	else
		is->queue_tail->next = 0;
	return ret;
}

static int is_source_stream( struct stream_impl *is )
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
	if ( is_source_stream( is ) ) {
		struct stream_impl *si = stream_to_impl( (stream_t*)is->queue->tree );
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
			struct stream_impl *si = stream_to_impl( (stream_t*)buf->tree );
			int type = si->funcs->get_parse_block( si, skip, pdp, copied );

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
			struct stream_impl *si = stream_to_impl( (stream_t*)buf->tree );
			int glen = si->funcs->get_data( si, dest+copied, length );

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
			struct stream_impl *si = stream_to_impl( (stream_t*)buf->tree );
			int slen = si->funcs->consume_data( prg, sp, si, length, loc );
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

		RunBuf *run_buf = input_stream_pop_head( is );
		//if ( runBuf->type == RunBufSourceType ) {
		//	stream_t *stream = (stream_t*)runBuf->tree;
		//	colm_tree_downref( prg, sp, (tree_t*) stream );
		//}
		free( run_buf );
	}

	return consumed;
}

static int _undoConsumeData( struct stream_impl *is, const char *data, int length )
{
	//debug( REALM_INPUT, "undoing consume of %ld bytes\n", length );

	if ( is->consumed == 0 && is_source_stream( is ) ) {
		struct stream_impl *si = stream_to_impl( (stream_t*)is->queue->tree );
		int len = si->funcs->undo_consume_data( si, data, length );
		return len;
	}
	else {
		RunBuf *new_buf = new_run_buf();
		new_buf->length = length;
		memcpy( new_buf->data, data, length );
		input_stream_prepend( is, new_buf );
		is->consumed -= length;

		return length;
	}
}

static tree_t *_consumeTree( struct stream_impl *is )
{
	while ( is->queue != 0 && is->queue->type == RunBufDataType && 
			is->queue->offset == is->queue->length )
	{
		RunBuf *run_buf = input_stream_pop_head( is );
		free( run_buf );
	}

	if ( is->queue != 0 && (is->queue->type == RunBufTokenType || 
			is->queue->type == RunBufIgnoreType) )
	{
		RunBuf *run_buf = input_stream_pop_head( is );

		/* FIXME: using runbufs here for this is a poor use of memory. */
		tree_t *tree = run_buf->tree;
		free(run_buf);
		return tree;
	}

	return 0;
}

static void _undoConsumeTree( struct stream_impl *is, tree_t *tree, int ignore )
{
	/* Create a new buffer for the data. This is the easy implementation.
	 * Something better is needed here. It puts a max on the amount of
	 * data that can be pushed back to the inputStream. */
	RunBuf *new_buf = new_run_buf();
	new_buf->type = ignore ? RunBufIgnoreType : RunBufTokenType;
	new_buf->tree = tree;
	input_stream_prepend( is, new_buf );
}

static struct LangEl *_consumeLangEl( struct stream_impl *is, long *bind_id,
		char **data, long *length )
{
	if ( is_source_stream( is ) ) {
		struct stream_impl *si = stream_to_impl( (stream_t*)is->queue->tree );
		return si->funcs->consume_lang_el( si, bind_id, data, length );
	}
	else {
		assert( false );
	}
}

static void _undoConsumeLangEl( struct stream_impl *is )
{
	if ( is_source_stream( is ) ) {
		struct stream_impl *si = stream_to_impl( (stream_t*)is->queue->tree );
		return si->funcs->undo_consume_lang_el( si );
	}
	else {
		assert( false );
	}
}

static void _prependData( struct stream_impl *is, const char *data, long length )
{
	if ( is_source_stream( is ) && 
			stream_to_impl((stream_t*)is->queue->tree)->funcs == &stream_funcs )
	{
		_prependData( stream_to_impl( (stream_t*)is->queue->tree ), data, length );
	}
	else {
		/* Create a new buffer for the data. This is the easy implementation.
		 * Something better is needed here. It puts a max on the amount of
		 * data that can be pushed back to the inputStream. */
		assert( length < FSM_BUFSIZE );

		RunBuf *new_buf = new_run_buf();
		new_buf->length = length;
		memcpy( new_buf->data, data, length );

		input_stream_prepend( is, new_buf );
	}
}

static void _prependTree( struct stream_impl *is, tree_t *tree, int ignore )
{
	/* Create a new buffer for the data. This is the easy implementation.
	 * Something better is needed here. It puts a max on the amount of
	 * data that can be pushed back to the inputStream. */
	RunBuf *new_buf = new_run_buf();
	new_buf->type = ignore ? RunBufIgnoreType : RunBufTokenType;
	new_buf->tree = tree;
	input_stream_prepend( is, new_buf );
}

static void _prependStream( struct stream_impl *in, struct colm_tree *tree )
{
	/* Create a new buffer for the data. This is the easy implementation.
	 * Something better is needed here. It puts a max on the amount of
	 * data that can be pushed back to the inputStream. */
	RunBuf *new_buf = new_run_buf();
	new_buf->type = RunBufSourceType;
	new_buf->tree = tree;
	input_stream_prepend( in, new_buf );
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
			struct stream_impl *si = stream_to_impl( (stream_t*)buf->tree );
			int slen = si->funcs->undo_prepend_data( si, length );

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

		RunBuf *run_buf = input_stream_pop_head( is );
		free( run_buf );
	}

	return consumed;
}

static tree_t *_undoPrependTree( struct stream_impl *is )
{
	while ( is->queue != 0 && is->queue->type == RunBufDataType &&
			is->queue->offset == is->queue->length )
	{
		RunBuf *run_buf = input_stream_pop_head( is );
		free( run_buf );
	}

	if ( is->queue != 0 && (is->queue->type == RunBufTokenType ||
			is->queue->type == RunBufIgnoreType) )
	{
		RunBuf *run_buf = input_stream_pop_head( is );

		/* FIXME: using runbufs here for this is a poor use of memory. */
		tree_t *tree = run_buf->tree;
		free(run_buf);
		return tree;
	}

	return 0;
}

static void _appendData( struct stream_impl *is, const char *data, long len )
{
	while ( len > 0 ) {
		RunBuf *ad = new_run_buf();
		input_stream_append( is, ad );

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
		RunBuf *buf = is->queue_tail;

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

		RunBuf *run_buf = input_stream_pop_tail( is );
		free( run_buf );
	}

	return 0;
}

static void _appendTree( struct stream_impl *is, tree_t *tree )
{
	RunBuf *ad = new_run_buf();

	input_stream_append( is, ad );

	ad->type = RunBufTokenType;
	ad->tree = tree;
	ad->length = 0;
}

static void _appendStream( struct stream_impl *in, struct colm_tree *tree )
{
	RunBuf *ad = new_run_buf();

	input_stream_append( in, ad );

	ad->type = RunBufSourceType;
	ad->tree = tree;
	ad->length = 0;
}

static tree_t *_undoAppendTree( struct stream_impl *is )
{
	RunBuf *run_buf = input_stream_pop_tail( is );
	tree_t *tree = run_buf->tree;
	free( run_buf );
	return tree;
}

static tree_t *_undoAppendStream( struct stream_impl *is )
{
	RunBuf *run_buf = input_stream_pop_tail( is );
	tree_t *tree = run_buf->tree;
	free( run_buf );
	return tree;
}

struct stream_funcs stream_funcs = 
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

struct stream_funcs file_funcs = 
{
	.get_data = &file_get_data,
	.get_parse_block = &file_get_parse_block,
	.consume_data = &file_consume_data,
	.undo_consume_data = &file_undo_consume_data,
	.get_data_source = &file_get_data_source,
};


struct stream_impl *colm_impl_new_file( const char *name, FILE *file )
{
	struct stream_impl *ss = (struct stream_impl*)malloc(sizeof(struct stream_impl));
	init_stream_impl( ss, name );
	ss->funcs = &file_funcs;

	ss->file = file;

	return ss;
}

struct stream_impl *colm_impl_new_fd( const char *name, long fd )
{
	struct stream_impl *ss = (struct stream_impl*)malloc(sizeof(struct stream_impl));
	init_stream_impl( ss, name );
	ss->funcs = &file_funcs;

	ss->file = fdopen( fd, ( fd == 0 ) ? "r" : "w" );

	return ss;
}

struct stream_impl *colm_impl_new_generic( const char *name )
{
	struct stream_impl *ss = (struct stream_impl*)malloc(sizeof(struct stream_impl));
	init_stream_impl( ss, name );
	ss->funcs = &stream_funcs;

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
	head_t *head_name = ((str_t*)name)->value;
	head_t *head_mode = ((str_t*)mode)->value;
	stream_t *stream = 0;

	const char *given_mode = string_data(head_mode);
	const char *fopen_mode = 0;
	if ( memcmp( given_mode, "r", string_length(head_mode) ) == 0 )
		fopen_mode = "rb";
	else if ( memcmp( given_mode, "w", string_length(head_mode) ) == 0 )
		fopen_mode = "wb";
	else if ( memcmp( given_mode, "a", string_length(head_mode) ) == 0 )
		fopen_mode = "ab";
	else {
		fatal( "unknown file open mode: %s\n", given_mode );
	}
	
	/* Need to make a C-string (null terminated). */
	char *file_name = (char*)malloc(string_length(head_name)+1);
	memcpy( file_name, string_data(head_name), string_length(head_name) );
	file_name[string_length(head_name)] = 0;
	FILE *file = fopen( file_name, fopen_mode );
	if ( file != 0 ) {
		stream = colm_stream_new_struct( prg );
		stream->impl = colm_impl_new_file( file_name, file );
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

struct stream_impl *stream_to_impl( stream_t *ptr )
{
	return ptr->impl;
}
