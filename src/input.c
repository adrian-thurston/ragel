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

DEF_INPUT_FUNCS( input_funcs_seq, input_impl_seq );
extern struct input_funcs_seq input_funcs;

static bool is_tree( struct seq_buf *b )
{
	return b->type == SB_TOKEN || b->type == SB_IGNORE;
}

static bool is_stream( struct seq_buf *b )
{
	return b->type == SB_SOURCE || b->type == SB_ACCUM;
}

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

static struct seq_buf *new_seq_buf()
{
	struct seq_buf *rb = (struct seq_buf*) malloc( sizeof(struct seq_buf) );
	memset( rb, 0, sizeof(struct seq_buf) );
	return rb;
}

static void input_transfer_loc( struct colm_program *prg, location_t *loc, struct input_impl_seq *ss )
{
}

static bool call_destructor( struct seq_buf *buf )
{
	return is_stream( buf ) && buf->own_si;
}

static void colm_input_destroy( program_t *prg, tree_t **sp, struct_t *s )
{
	input_t *input = (input_t*) s;
	struct input_impl *si = input->impl;
	si->funcs->destructor( prg, sp, si );
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

static void maybe_split( struct colm_program *prg, struct input_impl_seq *iis )
{
	struct seq_buf *head = iis->queue.head;
	if ( head != 0 && is_stream( head ) ) {
		/* Maybe the stream will split itself off. */
		struct stream_impl *split_off = head->si->funcs->split_consumed( prg, head->si );
		
		if ( split_off != 0 ) {
			debug( prg, REALM_INPUT, "maybe split: consumed is > 0, splitting\n" );

			struct seq_buf *new_buf = new_seq_buf();
			new_buf->type = SB_ACCUM;
			new_buf->si = split_off;
			new_buf->own_si = 1;

			input_stream_stash_head( prg, iis, new_buf );
		}
	}
}


/*
 * StreamImpl struct, this wraps the list of input streams.
 */

void init_input_impl_seq( struct input_impl_seq *is, char *name )
{
	memset( is, 0, sizeof(struct input_impl_seq) );

	is->type = 'S';
	//is->name = name;
	//is->line = 1;
	//is->column = 1;
	//is->byte = 0;
}

static struct seq_buf *input_stream_seq_pop_head( struct input_impl_seq *is )
{
	struct seq_buf *ret = is->queue.head;
	is->queue.head = is->queue.head->next;
	if ( is->queue.head == 0 )
		is->queue.tail = 0;
	else
		is->queue.head->prev = 0;
	return ret;
}

static void input_stream_seq_append( struct input_impl_seq *is, struct seq_buf *seq_buf )
{
	if ( is->queue.head == 0 ) {
		seq_buf->prev = seq_buf->next = 0;
		is->queue.head = is->queue.tail = seq_buf;
	}
	else {
		is->queue.tail->next = seq_buf;
		seq_buf->prev = is->queue.tail;
		seq_buf->next = 0;
		is->queue.tail = seq_buf;
	}
}

static struct seq_buf *input_stream_seq_pop_tail( struct input_impl_seq *is )
{
	struct seq_buf *ret = is->queue.tail;
	is->queue.tail = is->queue.tail->prev;
	if ( is->queue.tail == 0 )
		is->queue.head = 0;
	else
		is->queue.tail->next = 0;
	return ret;
}

static void input_stream_seq_prepend( struct input_impl_seq *is, struct seq_buf *seq_buf )
{
	if ( is->queue.head == 0 ) {
		seq_buf->prev = seq_buf->next = 0;
		is->queue.head = is->queue.tail = seq_buf;
	}
	else {
		is->queue.head->prev = seq_buf;
		seq_buf->prev = 0;
		seq_buf->next = is->queue.head;
		is->queue.head = seq_buf;
	}
}

void input_set_eof_mark( struct colm_program *prg, struct input_impl_seq *si, char eof_mark )
{
	si->eof_mark = eof_mark;
}

static void input_destructor( program_t *prg, tree_t **sp, struct input_impl_seq *si )
{
	struct seq_buf *buf = si->queue.head;
	while ( buf != 0 ) {
		if ( is_tree( buf ) )
			colm_tree_downref( prg, sp, buf->tree );

		if ( call_destructor( buf ) )
			buf->si->funcs->destructor( prg, sp, buf->si );

		struct seq_buf *next = buf->next;
		free( buf );
		buf = next;
	}

	buf = si->stash;
	while ( buf != 0 ) {
		struct seq_buf *next = buf->next;
		if ( call_destructor( buf ) )
			buf->si->funcs->destructor( prg, sp, buf->si );

		free( buf );
		buf = next;
	}

	si->queue.head = 0;

	/* FIXME: Need to leak this for now. Until we can return strings to a
	 * program loader and free them at a later date (after the colm program is
	 * deleted). */
	// if ( stream->impl->name != 0 )
	//	free( stream->impl->name );

	free( si );
}


static int input_get_parse_block( struct colm_program *prg, struct input_impl_seq *is, int *pskip, char **pdp, int *copied )
{
	int ret = 0;
	*copied = 0;

	/* Move over skip bytes. */
	struct seq_buf *buf = is->queue.head;
	while ( true ) {
		if ( buf == 0 ) {
			/* Got through the in-mem buffers without copying anything. */
			ret = is->eof_mark ? INPUT_EOF : INPUT_EOD;
			break;
		}

		if ( is_stream( buf ) ) {
			struct stream_impl *si = buf->si;
			int type = si->funcs->get_parse_block( prg, si, pskip, pdp, copied );

			if ( type == INPUT_EOD || type == INPUT_EOF ) {
				buf = buf->next;
				continue;
			}

			ret = type;
			break;
		}

		if ( buf->type == SB_TOKEN ) {
			ret = INPUT_TREE;
			break;
		}

		if ( buf->type == SB_IGNORE ) {
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

static int input_get_data( struct colm_program *prg, struct input_impl_seq *is, char *dest, int length )
{
	int copied = 0;

	/* Move over skip bytes. */
	struct seq_buf *buf = is->queue.head;
	while ( true ) {
		if ( buf == 0 ) {
			/* Got through the in-mem buffers without copying anything. */
			break;
		}

		if ( is_stream( buf ) ) {
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
		else if ( buf->type == SB_TOKEN )
			break;
		else if ( buf->type == SB_IGNORE )
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

static int input_consume_data( struct colm_program *prg, struct input_impl_seq *si, int length, location_t *loc )
{
	debug( prg, REALM_INPUT, "input_consume_data: stream %p consuming %d bytes\n", si, length );

	int consumed = 0;

	/* Move over skip bytes. */
	while ( true ) {
		struct seq_buf *buf = si->queue.head;

		if ( buf == 0 )
			break;

		if ( is_stream( buf ) ) {
			struct stream_impl *sub = buf->si;
			int slen = sub->funcs->consume_data( prg, sub, length, loc );
			//debug( REALM_INPUT, " got %d bytes from source\n", slen );

			consumed += slen;
			length -= slen;
		}
		else if ( buf->type == SB_TOKEN )
			break;
		else if ( buf->type == SB_IGNORE )
			break;
		else {
			assert(false);
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

static int input_undo_consume_data( struct colm_program *prg, struct input_impl_seq *si, const char *data, int length )
{
	/* When we push back data we need to move backwards through the block of
	 * text. The source stream type will */
	debug( prg, REALM_INPUT, "input_undo_consume_data: stream %p undoing consume of %d bytes\n", si, length );

	assert( length > 0 );
	long tot = length;
	int offset = 0;
	int remaining = length;

	while ( true ) {
		if ( is_stream( si->queue.head ) ) {
			struct stream_impl *sub = si->queue.head->si;
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

static tree_t *input_consume_tree( struct colm_program *prg, struct input_impl_seq *si )
{
	debug( prg, REALM_INPUT, "input_consume_tree: stream %p\n", si );

	while ( si->queue.head != 0 && is_stream( si->queue.head ) )
	{
		debug( prg, REALM_INPUT, "  stream %p consume: clearing source type\n", si );
		struct seq_buf *seq_buf = input_stream_seq_pop_head( si );
		input_stream_stash_head( prg, si, seq_buf );
	}

	assert( si->queue.head != 0 && ( si->queue.head->type == SB_TOKEN || si->queue.head->type == SB_IGNORE ) );

	{
		struct seq_buf *seq_buf = input_stream_seq_pop_head( si );
		input_stream_stash_head( prg, si, seq_buf );
		tree_t *tree = seq_buf->tree;
		debug( prg, REALM_INPUT, "  stream %p consume: tree: %p\n", si, tree );
		return tree;
	}

	return 0;
}


static void input_undo_consume_tree( struct colm_program *prg, struct input_impl_seq *si, tree_t *tree, int ignore )
{
	debug( prg, REALM_INPUT, "input_undo_consume_tree: stream %p undo consume tree %p\n", si, tree );

	while ( true ) {
		debug( prg, REALM_INPUT, "  stream %p consume: clearing source type\n", si );

		struct seq_buf *b = input_stream_pop_stash( prg, si );
		input_stream_seq_prepend( si, b );

		if ( is_tree( b ) ) {
			assert( b->tree->id == tree->id );
			break;
		}
	}
}

/*
 * Prepend
 */
static void input_prepend_data( struct colm_program *prg, struct input_impl_seq *si, const char *data, long length )
{
	debug( prg, REALM_INPUT, "input_prepend_data: stream %p prepend data length %d\n", si, length );

	maybe_split( prg, si );

	struct stream_impl *sub_si = colm_impl_new_text( "<text1>", data, length );

	struct seq_buf *new_buf = new_seq_buf();
	new_buf->type = SB_ACCUM;
	new_buf->si = sub_si;
	new_buf->own_si = 1;

	input_stream_seq_prepend( si, new_buf );
}

static int input_undo_prepend_data( struct colm_program *prg, struct input_impl_seq *si, int length )
{
	debug( prg, REALM_INPUT, "input_undo_prepend_data: stream %p undo append data length %d\n", si, length );

	struct seq_buf *seq_buf = input_stream_seq_pop_head( si );
	free( seq_buf );

	return 0;
}

static void input_prepend_tree( struct colm_program *prg, struct input_impl_seq *si, tree_t *tree, int ignore )
{
	debug( prg, REALM_INPUT, "input_prepend_tree: stream %p prepend tree %p\n", si, tree );

	maybe_split( prg, si );

	/* Create a new buffer for the data. This is the easy implementation.
	 * Something better is needed here. It puts a max on the amount of
	 * data that can be pushed back to the inputStream. */
	struct seq_buf *new_buf = new_seq_buf();
	new_buf->type = ignore ? SB_IGNORE : SB_TOKEN;
	new_buf->tree = tree;
	input_stream_seq_prepend( si, new_buf );
}

static tree_t *input_undo_prepend_tree( struct colm_program *prg, struct input_impl_seq *si )
{
	debug( prg, REALM_INPUT, "input_undo_prepend_tree: stream %p undo prepend tree\n", si );

	assert( si->queue.head != 0 && ( si->queue.head->type == SB_TOKEN ||
			si->queue.head->type == SB_IGNORE ) );

	struct seq_buf *seq_buf = input_stream_seq_pop_head( si );

	tree_t *tree = seq_buf->tree;
	free(seq_buf);

	debug( prg, REALM_INPUT, "  stream %p tree %p\n", si, tree );

	return tree;
}


static void input_prepend_stream( struct colm_program *prg, struct input_impl_seq *si, struct colm_stream *stream )
{
	maybe_split( prg, si );

	/* Create a new buffer for the data. This is the easy implementation.
	 * Something better is needed here. It puts a max on the amount of
	 * data that can be pushed back to the inputStream. */
	struct seq_buf *new_buf = new_seq_buf();
	new_buf->type = SB_SOURCE;
	new_buf->si = stream_to_impl( stream );
	input_stream_seq_prepend( si, new_buf );

	assert( ((struct stream_impl_data*)new_buf->si)->type == 'D' );
}

static tree_t *input_undo_prepend_stream( struct colm_program *prg, struct input_impl_seq *is )
{
	struct seq_buf *seq_buf = input_stream_seq_pop_head( is );
	free( seq_buf );
	return 0;
}

static void input_append_data( struct colm_program *prg, struct input_impl_seq *si, const char *data, long length )
{
	debug( prg, REALM_INPUT, "input_append_data: stream %p append data length %d\n", si, length );

	if ( si->queue.tail == 0 || si->queue.tail->type != SB_ACCUM ) { 
		debug( prg, REALM_INPUT, "input_append_data: creating accum\n" );

		struct stream_impl *sub_si = colm_impl_new_accum( "<text2>" );

		struct seq_buf *new_buf = new_seq_buf();
		new_buf->type = SB_ACCUM;
		new_buf->si = sub_si;
		new_buf->own_si = 1;

		input_stream_seq_append( si, new_buf );
	}

	si->queue.tail->si->funcs->append_data( prg, si->queue.tail->si, data, length );
}

static tree_t *input_undo_append_data( struct colm_program *prg, struct input_impl_seq *si, int length )
{
	debug( prg, REALM_INPUT, "input_undo_append_data: stream %p undo append data length %d\n", si, length );

	while ( true ) {
		struct seq_buf *buf = si->queue.tail;

		if ( buf == 0 )
			break;

		if ( is_stream( buf ) ) {
			struct stream_impl *sub = buf->si;
			int slen = sub->funcs->undo_append_data( prg, sub, length );
			//debug( REALM_INPUT, " got %d bytes from source\n", slen );
			//consumed += slen;
			length -= slen;
		}
		else if ( buf->type == SB_TOKEN )
			break;
		else if ( buf->type == SB_IGNORE )
			break;
		else {
			assert(false);
		}

		if ( length == 0 ) {
			//debug( REALM_INPUT, "exiting consume\n", length );
			break;
		}

		struct seq_buf *seq_buf = input_stream_seq_pop_tail( si );
		free( seq_buf );
	}
	return 0;
}

static void input_append_tree( struct colm_program *prg, struct input_impl_seq *si, tree_t *tree )
{
	debug( prg, REALM_INPUT, "input_append_tree: stream %p append tree %p\n", si, tree );

	struct seq_buf *ad = new_seq_buf();

	input_stream_seq_append( si, ad );

	ad->type = SB_TOKEN;
	ad->tree = tree;
}

static tree_t *input_undo_append_tree( struct colm_program *prg, struct input_impl_seq *si )
{
	debug( prg, REALM_INPUT, "input_undo_append_tree: stream %p undo append tree\n", si );

	struct seq_buf *seq_buf = input_stream_seq_pop_tail( si );
	tree_t *tree = seq_buf->tree;
	free( seq_buf );
	return tree;
}

static void input_append_stream( struct colm_program *prg, struct input_impl_seq *si, struct colm_stream *stream )
{
	debug( prg, REALM_INPUT, "input_append_stream: stream %p append stream %p\n", si, stream );

	struct seq_buf *ad = new_seq_buf();

	input_stream_seq_append( si, ad );

	ad->type = SB_SOURCE;
	ad->si = stream_to_impl( stream );

	assert( ((struct stream_impl_data*)ad->si)->type == 'D' );
}

static tree_t *input_undo_append_stream( struct colm_program *prg, struct input_impl_seq *si )
{
	debug( prg, REALM_INPUT, "input_undo_append_stream: stream %p undo append stream\n", si );

	struct seq_buf *seq_buf = input_stream_seq_pop_tail( si );
	free( seq_buf );
	return 0;
}

struct input_funcs_seq input_funcs = 
{
	&input_get_parse_block,
	&input_get_data,

	/* Consume. */
	&input_consume_data,
	&input_undo_consume_data,

	&input_consume_tree,
	&input_undo_consume_tree,

	0, /* consume_lang_el */
	0, /* undo_consume_lang_el */

	/* Prepend */
	&input_prepend_data,
	&input_undo_prepend_data,

	&input_prepend_tree,
	&input_undo_prepend_tree,

	&input_prepend_stream,
	&input_undo_prepend_stream,

	/* Append */
	&input_append_data,
	&input_undo_append_data,

	&input_append_tree,
	&input_undo_append_tree,

	&input_append_stream,
	&input_undo_append_stream,

	/* EOF */
	&input_set_eof_mark,

	&input_transfer_loc,
	&input_destructor,
};

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

input_t *colm_input_new( program_t *prg )
{
	struct input_impl *impl = colm_impl_new_generic( colm_filename_add( prg, "<internal>" ) );
	struct colm_input *input = colm_input_new_struct( prg );
	input->impl = impl;
	return input;
}

struct input_impl *input_to_impl( input_t *ptr )
{
	return ptr->impl;
}
