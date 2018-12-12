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

DEF_STREAM_FUNCS( stream_funcs_data, stream_impl_data );

extern struct stream_funcs_data file_funcs;
extern struct stream_funcs_data accum_funcs;

void stream_impl_push_line( struct stream_impl_data *ss, int ll )
{
	if ( ss->line_len == 0 ) {
		ss->lines_cur = 0;
		ss->lines_alloc = 16;
		ss->line_len = malloc( sizeof(int) * ss->lines_alloc );
	}
	else if ( ss->lines_cur == ss->lines_alloc ) {
		int lines_alloc_new = ss->lines_alloc * 2;
		int *line_len_new = malloc( sizeof(int) * lines_alloc_new );
		memcpy( line_len_new, ss->line_len, sizeof(int) * ss->lines_alloc );
		ss->lines_alloc = lines_alloc_new;
		ss->line_len = line_len_new;
	}

	ss->line_len[ ss->lines_cur ] = ll;
	ss->lines_cur += 1;
}

int stream_impl_pop_line( struct stream_impl_data *ss )
{
	int len = 0;
	if ( ss->lines_cur > 0 ) {
		ss->lines_cur -= 1;
		len = ss->line_len[ss->lines_cur];
	}
	return len;
}

static void dump_contents( struct colm_program *prg, struct stream_impl_data *sid )
{
	struct run_buf *rb = sid->queue.head;
	while ( rb != 0 ) {
		debug( prg, REALM_INPUT, " %p contents |%d|%d|%d|%.*s|\n", sid,
				rb->offset, rb->length,
				rb->length - rb->offset,
				(int)rb->length - rb->offset,
				rb->data + rb->offset );
		rb = rb->next;
	}
}

static bool loc_set( location_t *loc )
{
	return loc->line != 0;
}

static void close_stream_file( FILE *file )
{
	if ( file != stdin && file != stdout && file != stderr &&
			fileno(file) != 0 && fileno( file) != 1 && fileno(file) != 2 )
	{
		fclose( file );
	}
}

static void si_data_push_tail( struct stream_impl_data *ss, struct run_buf *run_buf )
{
	if ( ss->queue.head == 0 ) {
		run_buf->prev = run_buf->next = 0;
		ss->queue.head = ss->queue.tail = run_buf;
	}
	else {
		ss->queue.tail->next = run_buf;
		run_buf->prev = ss->queue.tail;
		run_buf->next = 0;
		ss->queue.tail = run_buf;
	}
}

static struct run_buf *si_data_pop_tail( struct stream_impl_data *ss )
{
	struct run_buf *ret = ss->queue.tail;
	ss->queue.tail = ss->queue.tail->prev;
	if ( ss->queue.tail == 0 )
		ss->queue.head = 0;
	else
		ss->queue.tail->next = 0;
	return ret;
}


static void si_data_push_head( struct stream_impl_data *ss, struct run_buf *run_buf )
{
	if ( ss->queue.head == 0 ) {
		run_buf->prev = run_buf->next = 0;
		ss->queue.head = ss->queue.tail = run_buf;
	}
	else {
		ss->queue.head->prev = run_buf;
		run_buf->prev = 0;
		run_buf->next = ss->queue.head;
		ss->queue.head = run_buf;
	}
}

static struct run_buf *si_data_pop_head( struct stream_impl_data *ss )
{
	struct run_buf *ret = ss->queue.head;
	ss->queue.head = ss->queue.head->next;
	if ( ss->queue.head == 0 )
		ss->queue.tail = 0;
	else
		ss->queue.head->prev = 0;
	return ret;
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

/* Keep the position up to date after consuming text. */
void update_position_data( struct stream_impl_data *is, const char *data, long length )
{
	int i;
	for ( i = 0; i < length; i++ ) {
		if ( data[i] == '\n' ) {
			stream_impl_push_line( is, is->column );
			is->line += 1;
			is->column = 1;
		}
		else {
			is->column += 1;
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
		if ( data[i] == '\n' ) {
			is->line -= 1;
			is->column = stream_impl_pop_line( is );
		}
		else {
			is->column -= 1;
		}
	}

	is->byte -= length;
}


/*
 * Interface
 */

static void data_transfer_loc( struct colm_program *prg, location_t *loc, struct stream_impl_data *ss )
{
	loc->name = ss->name;
	loc->line = ss->line;
	loc->column = ss->column;
	loc->byte = ss->byte;
}

/*
 * Data inputs: files, strings, etc.
 */

static int data_get_data( struct colm_program *prg, struct stream_impl_data *ss, char *dest, int length )
{
	int copied = 0;

	/* Move over skip bytes. */
	struct run_buf *buf = ss->queue.head;
	while ( true ) {
		if ( buf == 0 ) {
			/* Got through the in-mem buffers without copying anything. */
			struct run_buf *run_buf = new_run_buf( 0 );
			int received = ss->funcs->get_data_source( prg, (struct stream_impl*)ss, run_buf->data, FSM_BUFSIZE );
			if ( received == 0 ) {
				free( run_buf );
				break;
			}

			run_buf->length = received;
			si_data_push_tail( ss, run_buf );

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

static struct stream_impl *data_split_consumed( program_t *prg, struct stream_impl_data *sid )
{
	struct stream_impl *split_off = 0;
	if ( sid->consumed > 0 ) {
		debug( prg, REALM_INPUT, "maybe split: consumed is > 0, splitting\n" );
		split_off = colm_impl_consumed( "<text3>", sid->consumed );
		sid->consumed = 0;
	}
	return split_off;
}

int data_append_data( struct colm_program *prg, struct stream_impl_data *sid, const char *data, int length )
{
	struct run_buf *tail = sid->queue.tail;
	if ( tail == 0 || length > (FSM_BUFSIZE - tail->length) ) {
		debug( prg, REALM_INPUT, "data_append_data: allocating run buf\n" );
		tail = new_run_buf( length );
		si_data_push_tail( sid, tail );
	}

	debug( prg, REALM_INPUT, "data_append_data: appending to "
			"accum tail, offset: %d, length: %d, dlen: %d\n",
			tail->offset, tail->length, length );

	memcpy( tail->data + tail->length, data, length );
	tail->length += length;

#ifdef DEBUG
	dump_contents( prg, sid );
#endif

	return length;
}

int data_undo_append_data( struct colm_program *prg, struct stream_impl_data *sid, int length )
{
	int consumed = 0;
	int remaining = length;

	/* Move over skip bytes. */
	while ( true ) {
		struct run_buf *buf = sid->queue.tail;

		if ( buf == 0 )
			break;

		/* Anything available in the current buffer. */
		int avail = buf->length - buf->offset;
		if ( avail > 0 ) {
			/* The source data from the current buffer. */
			int slen = avail <= remaining ? avail : remaining;
			consumed += slen;
			remaining -= slen;
			buf->length -= slen;
			//sid->consumed += slen;
		}

		if ( remaining == 0 )
			break;

		struct run_buf *run_buf = si_data_pop_tail( sid );
		free( run_buf );
	}

	debug( prg, REALM_INPUT, "data_undo_append_data: stream %p "
			"ask: %d, consumed: %d, now: %d\n", sid, length, consumed );

#ifdef DEBUG
	dump_contents( prg, sid );
#endif

	return consumed;

}

static void data_destructor( program_t *prg, tree_t **sp, struct stream_impl_data *si )
{
	if ( si->file != 0 )
		close_stream_file( si->file );
	
	if ( si->collect != 0 ) {
		str_collect_destroy( si->collect );
		free( si->collect );
	}

	struct run_buf *buf = si->queue.head;
	while ( buf != 0 ) {
		struct run_buf *next = buf->next;
		free( buf );
		buf = next;
	}

	si->queue.head = 0;

	if ( si->data != 0 )
		free( (char*)si->data );

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
		close_stream_file( si->file );
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

static int data_get_parse_block( struct colm_program *prg, struct stream_impl_data *ss, int *pskip, char **pdp, int *copied )
{
	int ret = 0;
	*copied = 0;

	/* Move over skip bytes. */
	struct run_buf *buf = ss->queue.head;
	while ( true ) {
		if ( buf == 0 ) {
			/* Got through the in-mem buffers without copying anything. */
			struct run_buf *run_buf = new_run_buf( 0 );
			int received = ss->funcs->get_data_source( prg, (struct stream_impl*)ss, run_buf->data, FSM_BUFSIZE );
			if ( received == 0 ) {
				free( run_buf );
				ret = INPUT_EOD;
				break;
			}

			run_buf->length = received;
			si_data_push_tail( ss, run_buf );

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

static int data_consume_data( struct colm_program *prg, struct stream_impl_data *sid, int length, location_t *loc )
{
	int consumed = 0;
	int remaining = length;

	/* Move over skip bytes. */
	while ( true ) {
		struct run_buf *buf = sid->queue.head;

		if ( buf == 0 )
			break;

		/* Anything available in the current buffer. */
		int avail = buf->length - buf->offset;
		if ( avail > 0 ) {

			if ( !loc_set( loc ) )
				data_transfer_loc( prg, loc, sid );

			/* The source data from the current buffer. */
			int slen = avail <= remaining ? avail : remaining;
			consumed += slen;
			remaining -= slen;
			update_position_data( sid, buf->data + buf->offset, slen );
			buf->offset += slen;
			sid->consumed += slen;
		}

		if ( remaining == 0 )
			break;

		struct run_buf *run_buf = si_data_pop_head( sid );
		free( run_buf );
	}

	debug( prg, REALM_INPUT, "data_consume_data: stream %p "
			"ask: %d, consumed: %d, now: %d\n", sid, length, consumed, sid->consumed );

#ifdef DEBUG
	dump_contents( prg, sid );
#endif

	return consumed;
}

static int data_undo_consume_data( struct colm_program *prg, struct stream_impl_data *sid, const char *data, int length )
{
	const char *end = data + length;
	int amount = length;
	if ( amount > sid->consumed )
		amount = sid->consumed;

	int remaining = amount;
	struct run_buf *head = sid->queue.head;
	if ( head != 0 && head->offset > 0 ) {
		/* Fill into the offset space. */
		int fill = remaining > head->offset ? head->offset : remaining;
		end -= fill;
		remaining -= fill;

		undo_position_data( sid, end, fill );
		memcpy( head->data + (head->offset - fill), end, fill );

		head->offset -= fill;
		sid->consumed -= fill;
	}

	if ( remaining > 0 ) {
		end -= remaining;
		struct run_buf *new_buf = new_run_buf( 0 );
		new_buf->length = remaining;
		undo_position_data( sid, end, remaining );
		memcpy( new_buf->data, end, remaining );
		si_data_push_head( sid, new_buf );
		sid->consumed -= amount;
	}

	debug( prg, REALM_INPUT, "data_undo_consume_data: stream %p "
			"undid consume %d of %d bytes, consumed now %d, \n", sid, amount, length, sid->consumed );

#ifdef DEBUG
	dump_contents( prg, sid );
#endif

	return amount;
}

/*
 * File Inputs
 */

static int file_get_data_source( struct colm_program *prg, struct stream_impl_data *si, char *dest, int length )
{
	return fread( dest, 1, length, si->file );
}

/*
 * Text inputs
 */

static int accum_get_data_source( struct colm_program *prg, struct stream_impl_data *si, char *dest, int want )
{
	long avail = si->dlen - si->offset;
	long take = avail < want ? avail : want;
	if ( take > 0 ) 
		memcpy( dest, si->data + si->offset, take );
	si->offset += take;
	return take;
}

char stream_get_eof_sent( struct colm_program *prg, struct input_impl_seq *si )
{
	return si->eof_sent;
}

void stream_set_eof_sent( struct colm_program *prg, struct input_impl_seq *si, char eof_sent )
{
	si->eof_sent = eof_sent;
}

struct stream_funcs_data file_funcs = 
{
	&data_get_parse_block,
	&data_get_data,
	&file_get_data_source,

	&data_consume_data,
	&data_undo_consume_data,

	&data_transfer_loc,
	&data_get_collect,
	&data_flush_stream,
	&data_close_stream,
	&data_print_tree,
	&data_split_consumed,
	&data_append_data,
	&data_undo_append_data,
	&data_destructor,
};

struct stream_funcs_data accum_funcs = 
{
	&data_get_parse_block,
	&data_get_data,
	&accum_get_data_source,

	&data_consume_data,
	&data_undo_consume_data,

	&data_transfer_loc,
	&data_get_collect,
	&data_flush_stream,
	&data_close_stream,
	&data_print_tree,

	&data_split_consumed,
	&data_append_data,
	&data_undo_append_data,
	&data_destructor,
};

static void si_data_init( struct stream_impl_data *is, char *name )
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

struct stream_impl *colm_impl_new_accum( char *name )
{
	struct stream_impl_data *si = (struct stream_impl_data*)malloc(sizeof(struct stream_impl_data));
	si_data_init( si, name );
	si->funcs = (struct stream_funcs*)&accum_funcs;

	return (struct stream_impl*)si;
}

static struct stream_impl *colm_impl_new_file( char *name, FILE *file )
{
	struct stream_impl_data *ss = (struct stream_impl_data*)malloc(sizeof(struct stream_impl_data));
	si_data_init( ss, name );
	ss->funcs = (struct stream_funcs*)&file_funcs;
	ss->file = file;
	return (struct stream_impl*)ss;
}

static struct stream_impl *colm_impl_new_fd( char *name, long fd )
{
	struct stream_impl_data *si = (struct stream_impl_data*)malloc(sizeof(struct stream_impl_data));
	si_data_init( si, name );
	si->funcs = (struct stream_funcs*)&file_funcs;
	si->file = fdopen( fd, ( fd == 0 ) ? "r" : "w" );
	return (struct stream_impl*)si;
}

struct stream_impl *colm_impl_consumed( char *name, int len )
{
	struct stream_impl_data *si = (struct stream_impl_data*)malloc(sizeof(struct stream_impl_data));
	si_data_init( si, name );
	si->funcs = (struct stream_funcs*)&accum_funcs;

	si->data = 0;
	si->consumed = len;
	si->offset = len;

	si->dlen = len;

	return (struct stream_impl*)si;
}

struct stream_impl *colm_impl_new_text( char *name, const char *data, int len )
{
	struct stream_impl_data *si = (struct stream_impl_data*)malloc(sizeof(struct stream_impl_data));
	si_data_init( si, name );
	si->funcs = (struct stream_funcs*)&accum_funcs;

	char *buf = (char*)malloc( len );
	memcpy( buf, data, len );

	si->data = buf;
	si->dlen = len;

	return (struct stream_impl*)si;
}

struct stream_impl *colm_impl_new_collect( char *name )
{
	struct stream_impl_data *ss = (struct stream_impl_data*)malloc(sizeof(struct stream_impl_data));
	si_data_init( ss, name );
	ss->funcs = (struct stream_funcs*)&accum_funcs;
	ss->collect = (struct colm_str_collect*) malloc( sizeof( struct colm_str_collect ) );
	init_str_collect( ss->collect );
	return (struct stream_impl*)ss;
}

struct stream_impl *stream_to_impl( stream_t *ptr )
{
	return ptr->impl;
}

str_t *collect_string( program_t *prg, stream_t *s )
{
	str_collect_t *collect = s->impl->funcs->get_collect( prg, s->impl );
	head_t *head = string_alloc_full( prg, collect->data, collect->length );
	str_t *str = (str_t*)construct_string( prg, head );
	return str;
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


void colm_stream_destroy( program_t *prg, tree_t **sp, struct_t *s )
{
	stream_t *stream = (stream_t*) s;
	struct stream_impl *si = stream->impl;
	si->funcs->destructor( prg, sp, si );
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

stream_t *colm_stream_open_collect( program_t *prg )
{
	struct stream_impl *impl = colm_impl_new_collect( colm_filename_add( prg, "<internal>" ) );
	struct colm_stream *stream = colm_stream_new_struct( prg );
	stream->impl = impl;
	return stream;
}

