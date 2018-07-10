/*
 * Copyright 2007-2018 Adrian Thurston <thurston@colm.net>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <colm/input.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <stdbool.h>

#include <colm/pdarun.h>
#include <colm/debug.h>
#include <colm/program.h>
#include <colm/tree.h>
#include <colm/bytecode.h>
#include <colm/pool.h>
#include <colm/struct.h>

static struct stream_impl *colm_impl_consumed( char *name, int len );
static struct stream_impl *colm_impl_new_text( char *name, const char *data, int len );

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

struct seq_buf *new_seq_buf( int sz )
{
	struct seq_buf *rb;
	if ( sz > FSM_BUFSIZE ) {
		int ssz = sizeof(struct seq_buf) + sz - FSM_BUFSIZE;
		rb = (struct seq_buf*) malloc( ssz );
		memset( rb, 0, ssz );
	}
	else {
		rb = (struct seq_buf*) malloc( sizeof(struct seq_buf) );
		memset( rb, 0, sizeof(struct seq_buf) );
	}
	return rb;
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

DEF_INPUT_FUNCS( input_funcs_seq, input_impl_seq );
DEF_STREAM_FUNCS( stream_funcs_data, stream_impl_data );

extern struct input_funcs_seq input_funcs;
extern struct stream_funcs_data file_funcs;
extern struct stream_funcs_data text_funcs;

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

void transfer_loc_seq( struct colm_program *prg, location_t *loc, struct input_impl_seq *ss )
{
	loc->name = ss->name;
	loc->line = ss->line;
	loc->column = ss->column;
	loc->byte = ss->byte;
}

static void transfer_loc_data( struct colm_program *prg, location_t *loc, struct stream_impl_data *ss )
{
	loc->name = ss->name;
	loc->line = ss->line;
	loc->column = ss->column;
	loc->byte = ss->byte;
}

static void colm_clear_source_stream( struct colm_program *prg,
		tree_t **sp, struct input_impl_seq *si )
{
	struct seq_buf *buf = si->queue;
	while ( buf != 0 ) {
		switch ( buf->type ) {

			case SEQ_BUF_TOKEN_TYPE:
			case SEQ_BUF_IGNORE_TYPE:
				colm_tree_downref( prg, sp, buf->tree );
				break;
			case SEQ_BUF_SOURCE_TYPE:
				break;
		}

		struct seq_buf *next = buf->next;
		free( buf );
		buf = next;
	}

	si->queue = 0;
}

void colm_close_stream_file( FILE *file )
{
	if ( file != stdin && file != stdout && file != stderr &&
			fileno(file) != 0 && fileno( file) != 1 && fileno(file) != 2 )
	{
		fclose( file );
	}
}

void colm_input_destroy( program_t *prg, tree_t **sp, struct_t *s )
{
	input_t *input = (input_t*) s;
	struct input_impl *si = input->impl;
	si->funcs->destructor( prg, sp, si );
}

void colm_stream_destroy( program_t *prg, tree_t **sp, struct_t *s )
{
	stream_t *stream = (stream_t*) s;
	struct stream_impl *si = stream->impl;
	si->funcs->destructor( prg, sp, si );
}

/* Keep the position up to date after consuming text. */
void update_position_seq( struct input_impl_seq *is, const char *data, long length )
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
void undo_position_seq( struct input_impl_seq *is, const char *data, long length )
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



/* Keep the position up to date after consuming text. */
void update_position_data( struct stream_impl_data *is, const char *data, long length )
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
void undo_position_data( struct stream_impl_data *is, const char *data, long length )
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

static struct run_buf *source_stream_data_pop_head( struct stream_impl_data *ss )
{
	struct run_buf *ret = ss->queue;
	ss->queue = ss->queue->next;
	if ( ss->queue == 0 )
		ss->queue_tail = 0;
	else
		ss->queue->prev = 0;
	return ret;
}

static void source_stream_data_append( struct stream_impl_data *ss, struct run_buf *run_buf )
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

static void source_stream_data_prepend( struct stream_impl_data *ss, struct run_buf *run_buf )
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
 * Data inputs: files, strings, etc.
 */

static int data_get_data( struct colm_program *prg, struct stream_impl_data *ss, char *dest, int length )
{
	int copied = 0;

	/* Move over skip bytes. */
	struct run_buf *buf = ss->queue;
	while ( true ) {
		if ( buf == 0 ) {
			/* Got through the in-mem buffers without copying anything. */
			struct run_buf *run_buf = new_run_buf( 0 );
			source_stream_data_append( ss, run_buf );
			int received = ss->funcs->get_data_source( prg, (struct stream_impl*)ss, run_buf->data, FSM_BUFSIZE );
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

static void data_destructor( program_t *prg, tree_t **sp, struct stream_impl_data *si )
{
//	colm_clear_source_stream( prg, sp, (struct input_impl_seq*)stream->impl );

	if ( si->file != 0 )
		colm_close_stream_file( si->file );
	
	if ( si->collect != 0 ) {
		str_collect_destroy( si->collect );
		free( si->collect );
	}

	/* FIXME: Need to leak this for now. Until we can return strings to a
	 * program loader and free them at a later date (after the colm program is
	 * deleted). */
	// if ( si->name != 0 )
	//	free( si->name );

	free( si );
}

static str_collect_t *data_get_collect( struct colm_program *prg, struct stream_impl_data *si )
{
	return si->collect;
}

static void data_flush_stream( struct colm_program *prg, struct stream_impl_data *si )
{
	if ( si->file != 0 )
		fflush( si->file );
}

static void data_close_stream( struct colm_program *prg, struct stream_impl_data *si )
{
	if ( si->file != 0 ) {
		colm_close_stream_file( si->file );
		si->file = 0;
	}
}

static void data_print_tree( struct colm_program *prg, tree_t **sp,
		struct stream_impl_data *si, tree_t *tree, int trim )
{
	if ( si->file != 0 )
		colm_print_tree_file( prg, sp, (struct stream_impl*)si, tree, false );
	else if ( si->collect != 0 )
		colm_print_tree_collect( prg, sp, si->collect, tree, false );
}

char data_get_eof_sent( struct colm_program *prg, struct stream_impl_data *si )
{
	return si->eof_sent;
}

void data_set_eof_sent( struct colm_program *prg, struct stream_impl_data *si, char eof_sent )
{
	si->eof_sent = eof_sent;
}

static int data_get_parse_block( struct colm_program *prg, struct stream_impl_data *ss, int *pskip, char **pdp, int *copied )
{
	int ret = 0;
	*copied = 0;

	/* Move over skip bytes. */
	struct run_buf *buf = ss->queue;
	while ( true ) {
		if ( buf == 0 ) {
			/* Got through the in-mem buffers without copying anything. */
			struct run_buf *run_buf = new_run_buf( 0 );
			source_stream_data_append( ss, run_buf );
			int received = ss->funcs->get_data_source( prg, (struct stream_impl*)ss, run_buf->data, FSM_BUFSIZE );
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
			if ( *pskip > 0 && *pskip >= avail ) {
				/* Skipping the the whole source. */
				*pskip -= avail;
			}
			else {
				/* Either skip is zero, or less than slen. Skip goes to zero.
				 * Some data left over, copy it. */
				src += *pskip;
				avail -= *pskip;
				*pskip = 0;

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

static void input_stream_stash_head( struct colm_program *prg, struct input_impl_seq *si, struct seq_buf *seq_buf )
{
	debug( prg, REALM_INPUT, "stash_head: stream %p buf %p\n", si, seq_buf );
	seq_buf->next = si->stash;
	si->stash = seq_buf;
}

static struct seq_buf *input_stream_pop_stash( struct colm_program *prg, struct input_impl_seq *si )
{
	struct seq_buf *seq_buf = si->stash;
	si->stash = si->stash->next;

	debug( prg, REALM_INPUT, "pop_stash: stream %p buf %p\n", si, seq_buf );

	return seq_buf;
}


static void maybe_split( struct colm_program *prg, struct input_impl_seq *si )
{
	if ( si->queue != 0 && si->queue->type == SEQ_BUF_SOURCE_TYPE ) {

		/* NOT A GOOD IDEA. Use func instead */
		struct stream_impl_data *sid = (struct stream_impl_data*)si->queue->si;
		if ( sid->consumed > 0 ) {
			debug( prg, REALM_INPUT, "maybe split: consumed is > 0, splitting\n" );
			struct stream_impl *sub_si = colm_impl_consumed( "<text>", sid->consumed );
			sid->consumed = 0;

			struct seq_buf *new_buf = new_seq_buf( 0 );
			new_buf->type = SEQ_BUF_SOURCE_TYPE;
			new_buf->si = sub_si;

			input_stream_stash_head( prg, si, new_buf );
		}
	}
}

static int data_consume_data( struct colm_program *prg, struct stream_impl_data *si, int length, location_t *loc )
{
	int consumed = 0;
	int remaining = length;

	/* Move over skip bytes. */
	while ( true ) {
		struct run_buf *buf = si->queue;

		if ( buf == 0 )
			break;

		if ( !loc_set( loc ) )
			transfer_loc_data( prg, loc, si );

		/* Anything available in the current buffer. */
		int avail = buf->length - buf->offset;
		if ( avail > 0 ) {
			/* The source data from the current buffer. */
			int slen = avail <= remaining ? avail : remaining;
			consumed += slen;
			remaining -= slen;
			update_position_data( si, buf->data + buf->offset, slen );
			buf->offset += slen;
			si->consumed += slen;
		}

		if ( remaining == 0 )
			break;

		struct run_buf *run_buf = source_stream_data_pop_head( si );
		free( run_buf );
	}

	debug( prg, REALM_INPUT, "data_consume_data: stream %p "
			"ask: %d, consumed: %d, now: %d\n", si, length, consumed, si->consumed );

	return consumed;
}

static int data_undo_consume_data( struct colm_program *prg, struct stream_impl_data *si, const char *data, int length )
{
	int amount = length;
	if ( amount > si->consumed )
		amount = si->consumed;

	if ( amount > 0 ) {
		struct run_buf *new_buf = new_run_buf( 0 );
		new_buf->length = amount;
		memcpy( new_buf->data, data + ( length - amount ), amount );
		source_stream_data_prepend( si, new_buf );
		undo_position_data( si, data, amount );
		si->consumed -= amount;
	}

	debug( prg, REALM_INPUT, "data_undo_consume_data: stream %p "
			"undid consume %d of %d bytes, consumed now %d, \n", si, amount, length, si->consumed );

	return amount;
}

/*
 * File Inputs
 */

static int file_get_data_source( struct colm_program *prg, struct stream_impl_data *si, char *dest, int length )
{
	return fread( dest, 1, length, si->file );
}

void init_file_funcs()
{
	memset( &file_funcs, 0, sizeof(struct stream_funcs) );
}

/*
 * Text inputs
 */

static int text_get_data_source( struct colm_program *prg, struct stream_impl_data *si, char *dest, int want )
{
	long avail = si->dlen - si->offset;
	long take = avail < want ? avail : want;
	if ( take > 0 ) 
		memcpy( dest, si->data + si->offset, take );
	si->offset += take;
	return take;
}

/*
 * StreamImpl struct, this wraps the list of input streams.
 */

void init_input_impl_seq( struct input_impl_seq *is, char *name )
{
	memset( is, 0, sizeof(struct input_impl_seq) );

	is->type = 'S';
	is->name = name;
	is->line = 1;
	is->column = 1;
	is->byte = 0;
}

void init_stream_impl_data( struct stream_impl_data *is, char *name )
{
	memset( is, 0, sizeof(struct stream_impl_data) );

	is->type = 'D';
	is->name = name;
	is->line = 1;
	is->column = 1;
	is->byte = 0;

	/* Indentation turned off. */
	is->level = COLM_INDENT_OFF;
}

static struct seq_buf *input_stream_seq_pop_head( struct input_impl_seq *is )
{
	struct seq_buf *ret = is->queue;
	is->queue = is->queue->next;
	if ( is->queue == 0 )
		is->queue_tail = 0;
	else
		is->queue->prev = 0;
	return ret;
}

static void input_stream_seq_append( struct input_impl_seq *is, struct seq_buf *seq_buf )
{
	if ( is->queue == 0 ) {
		seq_buf->prev = seq_buf->next = 0;
		is->queue = is->queue_tail = seq_buf;
	}
	else {
		is->queue_tail->next = seq_buf;
		seq_buf->prev = is->queue_tail;
		seq_buf->next = 0;
		is->queue_tail = seq_buf;
	}
}

static struct seq_buf *input_stream_seq_pop_tail( struct input_impl_seq *is )
{
	struct seq_buf *ret = is->queue_tail;
	is->queue_tail = is->queue_tail->prev;
	if ( is->queue_tail == 0 )
		is->queue = 0;
	else
		is->queue_tail->next = 0;
	return ret;
}

static void input_stream_seq_prepend( struct input_impl_seq *is, struct seq_buf *seq_buf )
{
	if ( is->queue == 0 ) {
		seq_buf->prev = seq_buf->next = 0;
		is->queue = is->queue_tail = seq_buf;
	}
	else {
		is->queue->prev = seq_buf;
		seq_buf->prev = 0;
		seq_buf->next = is->queue;
		is->queue = seq_buf;
	}
}

void stream_set_eof( struct colm_program *prg, struct input_impl_seq *si )
{
	si->eof = true;
}

void stream_unset_eof( struct colm_program *prg, struct input_impl_seq *si )
{
	si->eof = false;
}

static void stream_destructor( program_t *prg, tree_t **sp, struct input_impl_seq *si )
{
	colm_clear_source_stream( prg, sp, si );

	/* FIXME: Need to leak this for now. Until we can return strings to a
	 * program loader and free them at a later date (after the colm program is
	 * deleted). */
	// if ( stream->impl->name != 0 )
	//	free( stream->impl->name );

	free( si );
}

char stream_get_eof_sent( struct colm_program *prg, struct input_impl_seq *si )
{
	return si->eof_sent;
}

void stream_set_eof_sent( struct colm_program *prg, struct input_impl_seq *si, char eof_sent )
{
	si->eof_sent = eof_sent;
}


static int stream_get_parse_block( struct colm_program *prg, struct input_impl_seq *is, int *pskip, char **pdp, int *copied )
{
	int ret = 0;
	*copied = 0;

	/* Move over skip bytes. */
	struct seq_buf *buf = is->queue;
	while ( true ) {
		if ( buf == 0 ) {
			/* Got through the in-mem buffers without copying anything. */
			ret = is->eof ? INPUT_EOF : INPUT_EOD;
			break;
		}

		if ( buf->type == SEQ_BUF_SOURCE_TYPE ) {
			struct stream_impl *si = buf->si;
			int type = si->funcs->get_parse_block( prg, si, pskip, pdp, copied );

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

		if ( buf->type == SEQ_BUF_TOKEN_TYPE ) {
			ret = INPUT_TREE;
			break;
		}

		if ( buf->type == SEQ_BUF_IGNORE_TYPE ) {
			ret = INPUT_IGNORE;
			break;
		}

		buf = buf->next;
	}

#if DEBUG
	switch ( ret ) {
		case INPUT_DATA:
			if ( *pdp != 0 ) {
				debug( prg, REALM_INPUT, "get parse block: DATA: %d %.*s\n", *copied, (int)(*copied), *pdp );
			}
			else {
				debug( prg, REALM_INPUT, "get parse block: DATA: %d\n", *copied );
			}
			break;
		case INPUT_EOD:
			debug( prg, REALM_INPUT, "get parse block: EOD\n" );
			break;
		case INPUT_EOF:
			debug( prg, REALM_INPUT, "get parse block: EOF\n" );
			break;
		case INPUT_TREE:
			debug( prg, REALM_INPUT, "get parse block: TREE\n" );
			break;
		case INPUT_IGNORE:
			debug( prg, REALM_INPUT, "get parse block: IGNORE\n" );
			break;
		case INPUT_LANG_EL:
			debug( prg, REALM_INPUT, "get parse block: LANG_EL\n" );
			break;
	}
#endif

	return ret;
}

static int stream_get_data( struct colm_program *prg, struct input_impl_seq *is, char *dest, int length )
{
	int copied = 0;

	/* Move over skip bytes. */
	struct seq_buf *buf = is->queue;
	while ( true ) {
		if ( buf == 0 ) {
			/* Got through the in-mem buffers without copying anything. */
			break;
		}

		if ( buf->type == SEQ_BUF_SOURCE_TYPE ) {
			struct stream_impl *si = buf->si;
			int glen = si->funcs->get_data( prg, si, dest+copied, length );

			if ( glen == 0 ) {
				//debug( REALM_INPUT, "skipping over input\n" );
				buf = buf->next;
				continue;
			}

			copied += glen;
			length -= glen;
		}
		else if ( buf->type == SEQ_BUF_TOKEN_TYPE )
			break;
		else if ( buf->type == SEQ_BUF_IGNORE_TYPE )
			break;

		if ( length == 0 ) {
			//debug( REALM_INPUT, "exiting get data\n", length );
			break;
		}

		buf = buf->next;
	}

	return copied;
}

/*
 * Consume
 */

static int stream_consume_data( struct colm_program *prg, struct input_impl_seq *si, int length, location_t *loc )
{
	debug( prg, REALM_INPUT, "stream_consume_data: stream %p consuming %d bytes\n", si, length );

	int consumed = 0;

	/* Move over skip bytes. */
	while ( true ) {
		struct seq_buf *buf = si->queue;

		if ( buf == 0 )
			break;

		if ( buf->type == SEQ_BUF_SOURCE_TYPE ) {
			struct stream_impl *sub = buf->si;
			int slen = sub->funcs->consume_data( prg, sub, length, loc );
			//debug( REALM_INPUT, " got %d bytes from source\n", slen );

			consumed += slen;
			length -= slen;
		}
		else if ( buf->type == SEQ_BUF_TOKEN_TYPE )
			break;
		else if ( buf->type == SEQ_BUF_IGNORE_TYPE )
			break;
		else {
			if ( !loc_set( loc ) ) {
				if ( si->line > 0 )
					transfer_loc_seq( prg, loc, si );
				else
					default_loc( loc );
			}
		}

		if ( length == 0 ) {
			//debug( REALM_INPUT, "exiting consume\n", length );
			break;
		}

		struct seq_buf *seq_buf = input_stream_seq_pop_head( si );
		input_stream_stash_head( prg, si, seq_buf );
	}

	return consumed;
}

static int stream_undo_consume_data( struct colm_program *prg, struct input_impl_seq *si, const char *data, int length )
{
	/* When we push back data we need to move backwards through the block of
	 * text. The source stream type will */
	debug( prg, REALM_INPUT, "stream_undo_consume_data: stream %p undoing consume of %d bytes\n", si, length );

	assert( length > 0 );
	long tot = length;
	int offset = 0;
	int remaining = length;

	while ( true ) {
		if ( si->queue->type == SEQ_BUF_SOURCE_TYPE ) {
			struct stream_impl *sub = si->queue->si;
			int pushed_back = sub->funcs->undo_consume_data( prg, sub, data, remaining );
			remaining -= pushed_back;
			offset += pushed_back;

			if ( remaining == 0 )
				break;
		}

		struct seq_buf *b = input_stream_pop_stash( prg, si );
		input_stream_seq_prepend( si, b );
	}

	return tot;
}

static tree_t *stream_consume_tree( struct colm_program *prg, struct input_impl_seq *si )
{
	debug( prg, REALM_INPUT, "stream_consume_tree: stream %p\n", si );

	while ( si->queue != 0 && ( si->queue->type == SEQ_BUF_SOURCE_TYPE ) )
	{
		debug( prg, REALM_INPUT, "  stream %p consume: clearing source type\n", si );
		struct seq_buf *seq_buf = input_stream_seq_pop_head( si );
		input_stream_stash_head( prg, si, seq_buf );
	}

	assert( si->queue != 0 && ( si->queue->type == SEQ_BUF_TOKEN_TYPE || si->queue->type == SEQ_BUF_IGNORE_TYPE ) );

	{
		struct seq_buf *seq_buf = input_stream_seq_pop_head( si );
		input_stream_stash_head( prg, si, seq_buf );
		tree_t *tree = seq_buf->tree;
		debug( prg, REALM_INPUT, "  stream %p consume: tree: %p\n", si, tree );
		return tree;
	}

	return 0;
}

static void stream_undo_consume_tree( struct colm_program *prg, struct input_impl_seq *si, tree_t *tree, int ignore )
{
	debug( prg, REALM_INPUT, "stream_undo_consume_tree: stream %p undo consume tree %p\n", si, tree );

	while ( true )
	{
		debug( prg, REALM_INPUT, "  stream %p consume: clearing source type\n", si );

		struct seq_buf *b = input_stream_pop_stash( prg, si );
		input_stream_seq_prepend( si, b );

		if ( b->type == SEQ_BUF_SOURCE_TYPE )
		{
		}
		else
		{
			assert( b->tree->id == tree->id );
			break;
		}
	}
}

/*
 * Prepend
 */
static void stream_prepend_data( struct colm_program *prg, struct input_impl_seq *si, const char *data, long length )
{
	debug( prg, REALM_INPUT, "stream_prepend_data: stream %p prepend data length %d\n", si, length );

	maybe_split( prg, si );

	struct stream_impl *sub_si = colm_impl_new_text( "<text>", data, length );

	struct seq_buf *new_buf = new_seq_buf( 0 );
	new_buf->type = SEQ_BUF_SOURCE_TYPE;
	new_buf->si = sub_si;

	input_stream_seq_prepend( si, new_buf );
}

static int stream_undo_prepend_data( struct colm_program *prg, struct input_impl_seq *si, int length )
{
	debug( prg, REALM_INPUT, "stream_undo_prepend_data: stream %p undo append data length %d\n", si, length );

	struct seq_buf *seq_buf = input_stream_seq_pop_head( si );
	free( seq_buf );

	return 0;
}

static void stream_prepend_tree( struct colm_program *prg, struct input_impl_seq *si, tree_t *tree, int ignore )
{
	debug( prg, REALM_INPUT, "stream_prepend_tree: stream %p prepend tree %p\n", si, tree );

	maybe_split( prg, si );

	/* Create a new buffer for the data. This is the easy implementation.
	 * Something better is needed here. It puts a max on the amount of
	 * data that can be pushed back to the inputStream. */
	struct seq_buf *new_buf = new_seq_buf( 0 );
	new_buf->type = ignore ? SEQ_BUF_IGNORE_TYPE : SEQ_BUF_TOKEN_TYPE;
	new_buf->tree = tree;
	input_stream_seq_prepend( si, new_buf );
}

static tree_t *stream_undo_prepend_tree( struct colm_program *prg, struct input_impl_seq *si )
{
	debug( prg, REALM_INPUT, "stream_undo_prepend_tree: stream %p undo prepend tree\n", si );

	assert( si->queue != 0 && ( si->queue->type == SEQ_BUF_TOKEN_TYPE ||
			si->queue->type == SEQ_BUF_IGNORE_TYPE ) );

	struct seq_buf *seq_buf = input_stream_seq_pop_head( si );

	tree_t *tree = seq_buf->tree;
	free(seq_buf);

	debug( prg, REALM_INPUT, "  stream %p tree %p\n", si, tree );

	return tree;
}


static void stream_prepend_stream( struct colm_program *prg, struct input_impl_seq *si, struct colm_stream *stream )
{
	maybe_split( prg, si );

	/* Create a new buffer for the data. This is the easy implementation.
	 * Something better is needed here. It puts a max on the amount of
	 * data that can be pushed back to the inputStream. */
	struct seq_buf *new_buf = new_seq_buf( 0 );
	new_buf->type = SEQ_BUF_SOURCE_TYPE;
	new_buf->si = stream_to_impl( stream );
	input_stream_seq_prepend( si, new_buf );

	assert( ((struct stream_impl_data*)new_buf->si)->type == 'D' );
}

static tree_t *stream_undo_prepend_stream( struct colm_program *prg, struct input_impl_seq *is )
{
	struct seq_buf *seq_buf = input_stream_seq_pop_head( is );
	free( seq_buf );
	return 0;
}

static void stream_append_data( struct colm_program *prg, struct input_impl_seq *si, const char *data, long length )
{
	debug( prg, REALM_INPUT, "stream_append_data: stream %p append data length %d\n", si, length );

	struct stream_impl *sub_si = colm_impl_new_text( "<text>", data, length );

	struct seq_buf *new_buf = new_seq_buf( 0 );
	new_buf->type = SEQ_BUF_SOURCE_TYPE;
	new_buf->si = sub_si;

	input_stream_seq_append( si, new_buf );
}

static tree_t *stream_undo_append_data( struct colm_program *prg, struct input_impl_seq *si, int length )
{
	debug( prg, REALM_INPUT, "stream_undo append_data: stream %p undo append data length %d\n", si, length );

	struct seq_buf *seq_buf = input_stream_seq_pop_tail( si );
	free( seq_buf );

	return 0;
}

static void stream_append_tree( struct colm_program *prg, struct input_impl_seq *si, tree_t *tree )
{
	debug( prg, REALM_INPUT, "stream_append_tree: stream %p append tree %p\n", si, tree );

	struct seq_buf *ad = new_seq_buf( 0 );

	input_stream_seq_append( si, ad );

	ad->type = SEQ_BUF_TOKEN_TYPE;
	ad->tree = tree;
}

static tree_t *stream_undo_append_tree( struct colm_program *prg, struct input_impl_seq *si )
{
	debug( prg, REALM_INPUT, "stream_undo_append_tree: stream %p undo append tree\n", si );

	struct seq_buf *seq_buf = input_stream_seq_pop_tail( si );
	tree_t *tree = seq_buf->tree;
	free( seq_buf );
	return tree;
}

static void stream_append_stream( struct colm_program *prg, struct input_impl_seq *si, struct colm_stream *stream )
{
	debug( prg, REALM_INPUT, "stream_append_stream: stream %p append stream %p\n", si, stream );

	struct seq_buf *ad = new_seq_buf( 0 );

	input_stream_seq_append( si, ad );

	ad->type = SEQ_BUF_SOURCE_TYPE;
	ad->si = stream_to_impl( stream );

	assert( ((struct stream_impl_data*)ad->si)->type == 'D' );
}

static tree_t *stream_undo_append_stream( struct colm_program *prg, struct input_impl_seq *si )
{
	debug( prg, REALM_INPUT, "stream_undo_append_stream: stream %p undo append stream\n", si );

	struct seq_buf *seq_buf = input_stream_seq_pop_tail( si );
	free( seq_buf );
	return 0;
}

struct input_funcs_seq input_funcs = 
{
	&stream_get_parse_block,
	&stream_get_data,

	/* Consume. */
	&stream_consume_data,
	&stream_undo_consume_data,

	&stream_consume_tree,
	&stream_undo_consume_tree,

	0, /* consume_lang_el */
	0, /* undo_consume_lang_el */

	/* Prepend */
	&stream_prepend_data,
	&stream_undo_prepend_data,

	&stream_prepend_tree,
	&stream_undo_prepend_tree,

	&stream_prepend_stream,
	&stream_undo_prepend_stream,

	/* Append */
	&stream_append_data,
	&stream_undo_append_data,

	&stream_append_tree,
	&stream_undo_append_tree,

	&stream_append_stream,
	&stream_undo_append_stream,

	/* EOF */
	&stream_set_eof,
	&stream_unset_eof,
	&stream_get_eof_sent,
	&stream_set_eof_sent,

	&transfer_loc_seq,
	&stream_destructor,
};

struct stream_funcs_data file_funcs = 
{
	.get_parse_block =   &data_get_parse_block,
	.get_data =          &data_get_data,
	.get_data_source =   &file_get_data_source,

	.consume_data =      &data_consume_data,
	.undo_consume_data = &data_undo_consume_data,

	.destructor =        &data_destructor,
	.get_collect =       &data_get_collect,
	.flush_stream =      &data_flush_stream,
	.close_stream =      &data_close_stream,
	.print_tree =        &data_print_tree,

	.get_eof_sent =      &data_get_eof_sent,
	.set_eof_sent =      &data_set_eof_sent,

	.transfer_loc =      &transfer_loc_data,
};

struct stream_funcs_data text_funcs = 
{
	.get_parse_block =   &data_get_parse_block,
	.get_data =          &data_get_data,
	.get_data_source =   &text_get_data_source,

	.consume_data =      &data_consume_data,
	.undo_consume_data = &data_undo_consume_data,

	.destructor =        &data_destructor,
	.get_collect =       &data_get_collect,
	.flush_stream =      &data_flush_stream,
	.close_stream =      &data_close_stream,
	.print_tree =        &data_print_tree,

	.get_eof_sent =      &data_get_eof_sent,
	.set_eof_sent =      &data_set_eof_sent,

	.transfer_loc =      &transfer_loc_data,
};

static struct stream_impl *colm_impl_new_file( char *name, FILE *file )
{
	struct stream_impl_data *ss = (struct stream_impl_data*)malloc(sizeof(struct stream_impl_data));
	init_stream_impl_data( ss, name );
	ss->funcs = (struct stream_funcs*)&file_funcs;
	ss->file = file;
	return (struct stream_impl*)ss;
}

static struct stream_impl *colm_impl_new_fd( char *name, long fd )
{
	struct stream_impl_data *si = (struct stream_impl_data*)malloc(sizeof(struct stream_impl_data));
	init_stream_impl_data( si, name );
	si->funcs = (struct stream_funcs*)&file_funcs;
	si->file = fdopen( fd, ( fd == 0 ) ? "r" : "w" );
	return (struct stream_impl*)si;
}

static struct stream_impl *colm_impl_consumed( char *name, int len )
{
	struct stream_impl_data *si = (struct stream_impl_data*)malloc(sizeof(struct stream_impl_data));
	init_stream_impl_data( si, name );
	si->funcs = (struct stream_funcs*)&text_funcs;

	si->data = 0;
	si->consumed = len;
	si->offset = len;

	si->dlen = len;

	return (struct stream_impl*)si;
}

static struct stream_impl *colm_impl_new_text( char *name, const char *data, int len )
{
	struct stream_impl_data *si = (struct stream_impl_data*)malloc(sizeof(struct stream_impl_data));
	init_stream_impl_data( si, name );
	si->funcs = (struct stream_funcs*)&text_funcs;

	char *buf = (char*)malloc( len );
	memcpy( buf, data, len );

	si->data = buf;
	si->dlen = len;

	return (struct stream_impl*)si;
}

struct stream_impl *colm_impl_new_collect( char *name )
{
	struct stream_impl_data *ss = (struct stream_impl_data*)malloc(sizeof(struct stream_impl_data));
	init_stream_impl_data( ss, name );
	ss->funcs = (struct stream_funcs*)&text_funcs;
	ss->collect = (struct colm_str_collect*) malloc( sizeof( struct colm_str_collect ) );
	init_str_collect( ss->collect );
	return (struct stream_impl*)ss;
}

struct input_impl *colm_impl_new_generic( char *name )
{
	struct input_impl_seq *ss = (struct input_impl_seq*)malloc(sizeof(struct input_impl_seq));
	init_input_impl_seq( ss, name );
	ss->funcs = (struct input_funcs*)&input_funcs;
	return (struct input_impl*)ss;
}

input_t *colm_input_new_struct( program_t *prg )
{
	size_t memsize = sizeof(struct colm_input);
	struct colm_input *input = (struct colm_input*) malloc( memsize );
	memset( input, 0, memsize );
	colm_struct_add( prg, (struct colm_struct *)input );
	input->id = prg->rtd->struct_input_id;
	input->destructor = &colm_input_destroy;
	return input;
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

input_t *colm_input_new( program_t *prg )
{
	struct input_impl *impl = colm_impl_new_generic( colm_filename_add( prg, "<internal>" ) );
	struct colm_input *input = colm_input_new_struct( prg );
	input->impl = impl;
	return input;
}

stream_t *colm_stream_new_struct( program_t *prg )
{
	size_t memsize = sizeof(struct colm_stream);
	struct colm_stream *stream = (struct colm_stream*) malloc( memsize );
	memset( stream, 0, memsize );
	colm_struct_add( prg, (struct colm_struct *)stream );
	stream->id = prg->rtd->struct_stream_id;
	stream->destructor = &colm_stream_destroy;
	return stream;
}


str_t *collect_string( program_t *prg, stream_t *s )
{
	str_collect_t *collect = s->impl->funcs->get_collect( prg, s->impl );
	head_t *head = string_alloc_full( prg, collect->data, collect->length );
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

struct input_impl *input_to_impl( input_t *ptr )
{
	return ptr->impl;
}
