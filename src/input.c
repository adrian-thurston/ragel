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
#include <stdbool.h>

char *colm_filename_add( program_t *prg, const char *fn )
{
	/* Search for it. */
	const char **ptr = prg->stream_fns;
	while ( *ptr != 0 ) {
		if ( strcmp( *ptr, fn ) == 0 )
			return (char*)*ptr;
		ptr += 1;
	}

	/* Not present, find. */
	int items = ptr - prg->stream_fns;

	prg->stream_fns = realloc( prg->stream_fns, sizeof(char*) * ( items + 2 ) );
	prg->stream_fns[items] = strdup( fn );
	prg->stream_fns[items+1] = 0;

	return (char*)prg->stream_fns[items];
}

struct run_buf *new_run_buf( int sz )
{
	struct run_buf *rb;
	if ( sz > FSM_BUFSIZE ) {
		int ssz = sizeof(struct run_buf) + sz - FSM_BUFSIZE;
		rb = (struct run_buf*) malloc( ssz );
		memset( rb, 0, ssz );
	}
	else {
		rb = (struct run_buf*) malloc( sizeof(struct run_buf) );
		memset( rb, 0, sizeof(struct run_buf) );
	}
	return rb;
}

void init_fd_funcs();
void init_file_funcs();
void init_pat_funcs();
void init_cons_funcs();

extern struct stream_funcs file_funcs;
extern struct stream_funcs fd_funcs;
extern struct stream_funcs stream_funcs;

static bool loc_set( location_t *loc )
{
	return loc->line != 0;
}

static void default_loc( location_t *loc )
{
	loc->name = "--";
	loc->line = 1;
	loc->column = 1;
	loc->byte = 1;
}

static void transfer_loc( location_t *loc, struct stream_impl *ss )
{
	loc->name = ss->name;
	loc->line = ss->line;
	loc->column = ss->column;
	loc->byte = ss->byte;
}

void colm_clear_source_stream( struct colm_program *prg,
		tree_t **sp, struct stream_impl *source_stream )
{
	struct run_buf *buf = source_stream->queue;
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

		struct run_buf *next = buf->next;
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
	
	if ( stream->impl->collect != 0 ) {
		str_collect_destroy( stream->impl->collect );
		free( stream->impl->collect );
	}

	/* FIXME: Need to leak this for now. Until we can return strings to a
	 * program loader and free them at a later date (after the colm program is
	 * deleted). */
	// if ( stream->impl->name != 0 )
	//	free( stream->impl->name );

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


static struct run_buf *source_stream_pop_head( struct stream_impl *ss )
{
	struct run_buf *ret = ss->queue;
	ss->queue = ss->queue->next;
	if ( ss->queue == 0 )
		ss->queue_tail = 0;
	else
		ss->queue->prev = 0;
	return ret;
}

static void source_stream_append( struct stream_impl *ss, struct run_buf *run_buf )
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

static void source_stream_prepend( struct stream_impl *ss, struct run_buf *run_buf )
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

static int file_get_parse_block( struct stream_impl *ss, int skip, char **pdp, int *copied )
{
	int ret = 0;
	*copied = 0;

	/* Move over skip bytes. */
	struct run_buf *buf = ss->queue;
	while ( true ) {
		if ( buf == 0 ) {
			/* Got through the in-mem buffers without copying anything. */
			struct run_buf *run_buf = new_run_buf( 0 );
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

static int file_get_data( struct stream_impl *ss, char *dest, int length )
{
	int copied = 0;

	/* Move over skip bytes. */
	struct run_buf *buf = ss->queue;
	while ( true ) {
		if ( buf == 0 ) {
			/* Got through the in-mem buffers without copying anything. */
			struct run_buf *run_buf = new_run_buf( 0 );
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

static int file_consume_data( program_t *prg, tree_t **sp,
		struct stream_impl *ss, int length, location_t *loc )
{
	int consumed = 0;

	/* Move over skip bytes. */
	while ( true ) {
		struct run_buf *buf = ss->queue;

		if ( buf == 0 )
			break;

		if ( buf->type == RunBufTokenType )
			break;
		else if ( buf->type == RunBufIgnoreType )
			break;
		else {
			if ( !loc_set( loc ) )
				transfer_loc( loc, ss );

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

		struct run_buf *run_buf = source_stream_pop_head( ss );
		free( run_buf );
	}

	return consumed;
}

static int file_undo_consume_data( struct stream_impl *ss, const char *data, int length )
{
	struct run_buf *new_buf = new_run_buf( 0 );
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

static int file_get_data_source( struct stream_impl *ss, char *dest, int length )
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

void init_stream_impl( struct stream_impl *is, char *name )
{
	memset( is, 0, sizeof(struct stream_impl) );

	is->name = name;
	is->line = 1;
	is->column = 1;
	is->byte = 0;

	/* Indentation turned off. */
	is->level = COLM_INDENT_OFF;
}

void colm_clear_stream_impl( struct colm_program *prg, tree_t **sp,
		struct stream_impl *input_stream )
{
	struct run_buf *buf = input_stream->queue;
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

		struct run_buf *next = buf->next;
		free( buf );
		buf = next;
	}

	input_stream->queue = 0;
}

static void input_stream_prepend( struct stream_impl *is, struct run_buf *run_buf )
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

static struct run_buf *input_stream_pop_head( struct stream_impl *is )
{
	struct run_buf *ret = is->queue;
	is->queue = is->queue->next;
	if ( is->queue == 0 )
		is->queue_tail = 0;
	else
		is->queue->prev = 0;
	return ret;
}

static void input_stream_append( struct stream_impl *is, struct run_buf *run_buf )
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

static struct run_buf *input_stream_pop_tail( struct stream_impl *is )
{
	struct run_buf *ret = is->queue_tail;
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

static void stream_set_eof( struct stream_impl *is )
{
	//debug( REALM_INPUT, "setting EOF in input stream\n" );
	is->eof = true;
}

static void stream_unset_eof( struct stream_impl *is )
{
	if ( is_source_stream( is ) ) {
		struct stream_impl *si = stream_to_impl( (stream_t*)is->queue->tree );
		si->eof = false;
	}
	else {
		is->eof = false;
	}
}

static int stream_get_parse_block( struct stream_impl *is, int skip, char **pdp, int *copied )
{
	int ret = 0;
	*copied = 0;

	/* Move over skip bytes. */
	struct run_buf *buf = is->queue;
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

static int stream_get_data( struct stream_impl *is, char *dest, int length )
{
	int copied = 0;

	/* Move over skip bytes. */
	struct run_buf *buf = is->queue;
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

static int stream_consume_data( program_t *prg, tree_t **sp, struct stream_impl *is,
		int length, location_t *loc )
{
	//debug( REALM_INPUT, "consuming %d bytes\n", length );

	int consumed = 0;

	/* Move over skip bytes. */
	while ( true ) {
		struct run_buf *buf = is->queue;

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
			if ( !loc_set( loc ) ) {
				if ( is->line > 0 )
					transfer_loc( loc, is );
				else
					default_loc( loc );
			}

			/* Anything available in the current buffer. */
			int avail = buf->length - buf->offset;
			if ( avail > 0 ) {
				/* The source data from the current buffer. */
				int slen = avail <= length ? avail : length;
				consumed += slen;
				length -= slen;
				update_position( is, buf->data + buf->offset, slen );
				buf->offset += slen;
				is->consumed += slen;
			}

		}

		if ( length == 0 ) {
			//debug( REALM_INPUT, "exiting consume\n", length );
			break;
		}

		struct run_buf *run_buf = input_stream_pop_head( is );
		//if ( runBuf->type == RunBufSourceType ) {
		//	stream_t *stream = (stream_t*)runBuf->tree;
		//	colm_tree_downref( prg, sp, (tree_t*) stream );
		//}
		free( run_buf );
	}

	return consumed;
}

static int stream_undo_consume_data( struct stream_impl *is, const char *data, int length )
{
	//debug( REALM_INPUT, "undoing consume of %ld bytes\n", length );

	if ( is->consumed == 0 && is_source_stream( is ) ) {
		struct stream_impl *si = stream_to_impl( (stream_t*)is->queue->tree );
		int len = si->funcs->undo_consume_data( si, data, length );
		return len;
	}
	else {
		struct run_buf *new_buf = new_run_buf( 0 );
		new_buf->length = length;
		memcpy( new_buf->data, data, length );
		input_stream_prepend( is, new_buf );
		is->consumed -= length;

		return length;
	}
}

static tree_t *stream_consume_tree( struct stream_impl *is )
{
	while ( is->queue != 0 && is->queue->type == RunBufDataType && 
			is->queue->offset == is->queue->length )
	{
		struct run_buf *run_buf = input_stream_pop_head( is );
		free( run_buf );
	}

	if ( is->queue != 0 && (is->queue->type == RunBufTokenType || 
			is->queue->type == RunBufIgnoreType) )
	{
		struct run_buf *run_buf = input_stream_pop_head( is );

		/* FIXME: using runbufs here for this is a poor use of memory. */
		tree_t *tree = run_buf->tree;
		free(run_buf);
		return tree;
	}

	return 0;
}

static void stream_undo_consume_tree( struct stream_impl *is, tree_t *tree, int ignore )
{
	/* Create a new buffer for the data. This is the easy implementation.
	 * Something better is needed here. It puts a max on the amount of
	 * data that can be pushed back to the inputStream. */
	struct run_buf *new_buf = new_run_buf( 0 );
	new_buf->type = ignore ? RunBufIgnoreType : RunBufTokenType;
	new_buf->tree = tree;
	input_stream_prepend( is, new_buf );
}

static struct LangEl *stream_consume_lang_el( struct stream_impl *is, long *bind_id,
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

static void stream_undo_consume_lang_el( struct stream_impl *is )
{
	if ( is_source_stream( is ) ) {
		struct stream_impl *si = stream_to_impl( (stream_t*)is->queue->tree );
		return si->funcs->undo_consume_lang_el( si );
	}
	else {
		assert( false );
	}
}

static void stream_prepend_data( struct stream_impl *is, const char *data, long length )
{
	if ( is_source_stream( is ) && 
			stream_to_impl((stream_t*)is->queue->tree)->funcs == &stream_funcs )
	{
		stream_prepend_data( stream_to_impl( (stream_t*)is->queue->tree ), data, length );
	}
	else {
		if ( is_source_stream( is ) ) {
			/* Steal the location information. Note that name allocations are
			 * managed separately from streams and so ptr overwrite transfer is
			 * safe. */
			stream_t *s = ((stream_t*)is->queue->tree);
			is->line = s->impl->line;
			is->column = s->impl->column;
			is->byte = s->impl->byte;
			is->name = strdup(s->impl->name);
		}

		/* Create a new buffer for the data. This is the easy implementation.
		 * Something better is needed here. It puts a max on the amount of
		 * data that can be pushed back to the inputStream. */
		assert( length < FSM_BUFSIZE );

		struct run_buf *new_buf = new_run_buf( 0 );
		new_buf->length = length;
		memcpy( new_buf->data, data, length );

		input_stream_prepend( is, new_buf );
	}
}

static void stream_prepend_tree( struct stream_impl *is, tree_t *tree, int ignore )
{
	/* Create a new buffer for the data. This is the easy implementation.
	 * Something better is needed here. It puts a max on the amount of
	 * data that can be pushed back to the inputStream. */
	struct run_buf *new_buf = new_run_buf( 0 );
	new_buf->type = ignore ? RunBufIgnoreType : RunBufTokenType;
	new_buf->tree = tree;
	input_stream_prepend( is, new_buf );
}

static void stream_prepend_stream( struct stream_impl *in, struct colm_tree *tree )
{
	/* Create a new buffer for the data. This is the easy implementation.
	 * Something better is needed here. It puts a max on the amount of
	 * data that can be pushed back to the inputStream. */
	struct run_buf *new_buf = new_run_buf( 0 );
	new_buf->type = RunBufSourceType;
	new_buf->tree = tree;
	input_stream_prepend( in, new_buf );
}

static int stream_undo_prepend_data( struct stream_impl *is, int length )
{
	//debug( REALM_INPUT, "consuming %d bytes\n", length );

	int consumed = 0;

	/* Move over skip bytes. */
	while ( true ) {
		struct run_buf *buf = is->queue;

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

		struct run_buf *run_buf = input_stream_pop_head( is );
		free( run_buf );
	}

	return consumed;
}

static tree_t *stream_undo_prepend_tree( struct stream_impl *is )
{
	while ( is->queue != 0 && is->queue->type == RunBufDataType &&
			is->queue->offset == is->queue->length )
	{
		struct run_buf *run_buf = input_stream_pop_head( is );
		free( run_buf );
	}

	if ( is->queue != 0 && (is->queue->type == RunBufTokenType ||
			is->queue->type == RunBufIgnoreType) )
	{
		struct run_buf *run_buf = input_stream_pop_head( is );

		/* FIXME: using runbufs here for this is a poor use of memory. */
		tree_t *tree = run_buf->tree;
		free(run_buf);
		return tree;
	}

	return 0;
}

static void stream_append_data( struct stream_impl *is, const char *data, long len )
{
	while ( len > 0 ) {
		struct run_buf *ad = new_run_buf( 0 );
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

static tree_t *stream_undo_append_data( struct stream_impl *is, int length )
{
	int consumed = 0;

	/* Move over skip bytes. */
	while ( true ) {
		struct run_buf *buf = is->queue_tail;

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

		struct run_buf *run_buf = input_stream_pop_tail( is );
		free( run_buf );
	}

	return 0;
}

static void stream_append_tree( struct stream_impl *is, tree_t *tree )
{
	struct run_buf *ad = new_run_buf( 0 );

	input_stream_append( is, ad );

	ad->type = RunBufTokenType;
	ad->tree = tree;
	ad->length = 0;
}

static void stream_append_stream( struct stream_impl *in, struct colm_tree *tree )
{
	struct run_buf *ad = new_run_buf( 0 );

	input_stream_append( in, ad );

	ad->type = RunBufSourceType;
	ad->tree = tree;
	ad->length = 0;
}

static tree_t *stream_undo_append_tree( struct stream_impl *is )
{
	struct run_buf *run_buf = input_stream_pop_tail( is );
	tree_t *tree = run_buf->tree;
	free( run_buf );
	return tree;
}

static tree_t *stream_undo_append_stream( struct stream_impl *is )
{
	struct run_buf *run_buf = input_stream_pop_tail( is );
	tree_t *tree = run_buf->tree;
	free( run_buf );
	return tree;
}

struct stream_funcs stream_funcs = 
{
	&stream_get_parse_block,
	&stream_get_data,
	&stream_consume_data,
	&stream_undo_consume_data,
	&stream_consume_tree,
	&stream_undo_consume_tree,
	&stream_consume_lang_el,
	&stream_undo_consume_lang_el,
	0, // source data get, not needed.
	&stream_set_eof,
	&stream_unset_eof,
	&stream_prepend_data,
	&stream_prepend_tree,
	&stream_prepend_stream,
	&stream_undo_prepend_data,
	&stream_undo_prepend_tree,
	0, // fixme: _add this.
	&stream_append_data,
	&stream_append_tree,
	&stream_append_stream,
	&stream_undo_append_data,
	&stream_undo_append_tree,
	&stream_undo_append_stream,
};

struct stream_funcs file_funcs = 
{
	.get_data = &file_get_data,
	.get_parse_block = &file_get_parse_block,
	.consume_data = &file_consume_data,
	.undo_consume_data = &file_undo_consume_data,
	.get_data_source = &file_get_data_source,
};


static struct stream_impl *colm_impl_new_file( char *name, FILE *file )
{
	struct stream_impl *ss = (struct stream_impl*)malloc(sizeof(struct stream_impl));
	init_stream_impl( ss, name );
	ss->funcs = &file_funcs;
	ss->file = file;
	return ss;
}

static struct stream_impl *colm_impl_new_fd( char *name, long fd )
{
	struct stream_impl *ss = (struct stream_impl*)malloc(sizeof(struct stream_impl));
	init_stream_impl( ss, name );
	ss->funcs = &file_funcs;
	ss->file = fdopen( fd, ( fd == 0 ) ? "r" : "w" );
	return ss;
}

struct stream_impl *colm_impl_new_generic( char *name )
{
	struct stream_impl *ss = (struct stream_impl*)malloc(sizeof(struct stream_impl));
	init_stream_impl( ss, name );
	ss->funcs = &stream_funcs;
	return ss;
}

struct stream_impl *colm_impl_new_collect( char *name )
{
	struct stream_impl *ss = (struct stream_impl*)malloc(sizeof(struct stream_impl));
	init_stream_impl( ss, name );
	ss->funcs = &stream_funcs;
	ss->collect = (StrCollect*) malloc( sizeof( StrCollect ) );
	init_str_collect( ss->collect );
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
	struct stream_impl *impl = colm_impl_new_fd( colm_filename_add( prg, name ), fd );

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
		stream->impl = colm_impl_new_file( colm_filename_add( prg, file_name ), file );
	}

	free( file_name );

	return stream;
}

stream_t *colm_stream_new( program_t *prg )
{
	struct stream_impl *impl = colm_impl_new_generic( colm_filename_add( prg, "<internal>" ) );
	struct colm_stream *stream = colm_stream_new_struct( prg );
	stream->impl = impl;
	return stream;
}

str_t *collect_string( program_t *prg, stream_t *s )
{
	head_t *head = string_alloc_full( prg, s->impl->collect->data, s->impl->collect->length );
	str_t *str = (str_t*)construct_string( prg, head );
	return str;
}


stream_t *colm_stream_open_collect( program_t *prg )
{
	struct stream_impl *impl = colm_impl_new_collect( colm_filename_add( prg, "<internal>" ) );
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
