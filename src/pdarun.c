/*
 * Copyright 2007-2015 Adrian Thurston <thurston@colm.net>
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

#include "pdarun.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>

#include "config.h"
#include "debug.h"
#include "bytecode.h"
#include "tree.h"
#include "pool.h"
#include "internal.h"

#define act_sb 0x1
#define act_rb 0x2

#define read_word_p( i, p ) do { \
	i = ((word_t)  p[0]); \
	i |= ((word_t) p[1]) << 8; \
	i |= ((word_t) p[2]) << 16; \
	i |= ((word_t) p[3]) << 24; \
} while(0)

#define read_tree_p( i, p ) do { \
	word_t w; \
	w = ((word_t)  p[0]); \
	w |= ((word_t) p[1]) << 8; \
	w |= ((word_t) p[2]) << 16; \
	w |= ((word_t) p[3]) << 24; \
	i = (tree_t*)w; \
} while(0)

/* bit 0: data needed. bit 1: loc needed */
#define RN_NONE 0x0
#define RN_DATA 0x1
#define RN_LOC  0x2
#define RN_BOTH 0x3


static void init_fsm_run( program_t *prg, struct pda_run *pda_run )
{
	pda_run->fsm_tables = prg->rtd->fsm_tables;

	pda_run->consume_buf = 0;

	pda_run->p = pda_run->pe = 0;
	pda_run->toklen = 0;
	pda_run->eof = 0;

	pda_run->pre_region = -1;
}

static void clear_fsm_run( program_t *prg, struct pda_run *pda_run )
{
	if ( pda_run->consume_buf != 0 ) {
		/* Transfer the run buf list to the program */
		struct run_buf *head = pda_run->consume_buf;
		struct run_buf *tail = head;
		while ( tail->next != 0 )
			tail = tail->next;

		tail->next = prg->alloc_run_buf;
		prg->alloc_run_buf = head;
	}
}

void colm_increment_steps( struct pda_run *pda_run )
{
	pda_run->steps += 1;
	//debug( prg, REALM_PARSE, "steps up to %ld\n", pdaRun->steps );
}

void colm_decrement_steps( struct pda_run *pda_run )
{
	pda_run->steps -= 1;
	//debug( prg, REALM_PARSE, "steps down to %ld\n", pdaRun->steps );
}

head_t *colm_stream_pull( program_t *prg, tree_t **sp, struct pda_run *pda_run,
		struct stream_impl *is, long length )
{
	if ( pda_run != 0 ) {
		struct run_buf *run_buf = pda_run->consume_buf;
		if ( length > ( FSM_BUFSIZE - run_buf->length ) ) {
			run_buf = new_run_buf( 0 );
			run_buf->next = pda_run->consume_buf;
			pda_run->consume_buf = run_buf;
		}

		char *dest = run_buf->data + run_buf->length;

		is->funcs->get_data( is, dest, length );
		location_t *loc = location_allocate( prg );
		is->funcs->consume_data( prg, sp, is, length, loc );

		run_buf->length += length;

		pda_run->p = pda_run->pe = 0;
		pda_run->toklen = 0;

		head_t *tokdata = colm_string_alloc_pointer( prg, dest, length );
		tokdata->location = loc;

		return tokdata;
	}
	else {
		head_t *head = init_str_space( length );
		char *dest = (char*)head->data;

		is->funcs->get_data( is, dest, length );
		location_t *loc = location_allocate( prg );
		is->funcs->consume_data( prg, sp, is, length, loc );
		head->location = loc;

		return head;
	}
}

void undo_stream_pull( struct stream_impl *is, const char *data, long length )
{
	//debug( REALM_PARSE, "undoing stream pull\n" );

	is->funcs->prepend_data( is, data, length );
}

void colm_stream_push_text( struct stream_impl *is, const char *data, long length )
{
	is->funcs->prepend_data( is, data, length );
}

void colm_stream_push_tree( struct stream_impl *is, tree_t *tree, int ignore )
{
	is->funcs->prepend_tree( is, tree, ignore );
}

void colm_stream_push_stream( struct stream_impl *is, tree_t *tree )
{
	is->funcs->prepend_stream( is, tree );
}

void colm_undo_stream_push( program_t *prg, tree_t **sp, struct stream_impl *is, long length )
{
	if ( length < 0 ) {
		tree_t *tree = is->funcs->undo_prepend_tree( is );
		colm_tree_downref( prg, sp, tree );
	}
	else {
		is->funcs->undo_prepend_data( is, length );
	}
}

/* Should only be sending back whole tokens/ignores, therefore the send back
 * should never cross a buffer boundary. Either we slide back data, or we move to
 * a previous buffer and slide back data. */
static void send_back_text( struct stream_impl *is, const char *data, long length )
{
	//debug( REALM_PARSE, "push back of %ld characters\n", length );

	if ( length == 0 )
		return;

	//debug( REALM_PARSE, "sending back text: %.*s\n", 
	//		(int)length, data );

	is->funcs->undo_consume_data( is, data, length );
}

static void send_back_tree( struct stream_impl *is, tree_t *tree )
{
	is->funcs->undo_consume_tree( is, tree, false );
}

/*
 * Stops on:
 *   PCR_REVERSE
 */
static void send_back_ignore( program_t *prg, tree_t **sp,
		struct pda_run *pda_run, struct stream_impl *is, parse_tree_t *parse_tree )
{
	#ifdef DEBUG
	struct lang_el_info *lel_info = prg->rtd->lel_info;
	debug( prg, REALM_PARSE, "sending back: %s%s\n",
		lel_info[parse_tree->shadow->tree->id].name, 
		parse_tree->flags & PF_ARTIFICIAL ? " (artificial)" : "" );
	#endif

	head_t *head = parse_tree->shadow->tree->tokdata;
	int artificial = parse_tree->flags & PF_ARTIFICIAL;

	if ( head != 0 && !artificial )
		send_back_text( is, string_data( head ), head->length );

	colm_decrement_steps( pda_run );

	/* Check for reverse code. */
	if ( parse_tree->flags & PF_HAS_RCODE ) {
		pda_run->on_deck = true;
		parse_tree->flags &= ~PF_HAS_RCODE;
	}

	if ( pda_run->steps == pda_run->target_steps ) {
		debug( prg, REALM_PARSE, "trigger parse stop, steps = "
				"target = %d\n", pda_run->target_steps );
		pda_run->stop = true;
	}
}

static void reset_token( struct pda_run *pda_run )
{
	/* If there is a token started, but never finished for a lack of data, we
	 * must first backup over it. */
	if ( pda_run->tokstart != 0 ) {
		pda_run->p = pda_run->pe = 0;
		pda_run->toklen = 0;
		pda_run->eof = 0;
	}
}

/* Stops on:
 *   PCR_REVERSE
 */

static void send_back( program_t *prg, tree_t **sp, struct pda_run *pda_run,
		struct stream_impl *is, parse_tree_t *parse_tree )
{
	debug( prg, REALM_PARSE, "sending back: %s\n",
			prg->rtd->lel_info[parse_tree->id].name );

	if ( parse_tree->flags & PF_NAMED ) {
		/* Send the named lang el back first, then send back any leading
		 * whitespace. */
		is->funcs->undo_consume_lang_el( is );
	}

	colm_decrement_steps( pda_run );

	/* Artifical were not parsed, instead sent in as items. */
	if ( parse_tree->flags & PF_ARTIFICIAL ) {
		/* Check for reverse code. */
		if ( parse_tree->flags & PF_HAS_RCODE ) {
			debug( prg, REALM_PARSE, "tree has rcode, setting on deck\n" );
			pda_run->on_deck = true;
			parse_tree->flags &= ~PF_HAS_RCODE;
		}

		colm_tree_upref( parse_tree->shadow->tree );

		send_back_tree( is, parse_tree->shadow->tree );
	}
	else {
		/* Check for reverse code. */
		if ( parse_tree->flags & PF_HAS_RCODE ) {
			debug( prg, REALM_PARSE, "tree has rcode, setting on deck\n" );
			pda_run->on_deck = true;
			parse_tree->flags &= ~PF_HAS_RCODE;
		}

		/* Push back the token data. */
		send_back_text( is, string_data( parse_tree->shadow->tree->tokdata ), 
				string_length( parse_tree->shadow->tree->tokdata ) );

		/* If eof was just sent back remember that it needs to be sent again. */
		if ( parse_tree->id == prg->rtd->eof_lel_ids[pda_run->parser_id] )
			is->eof_sent = false;

		/* If the item is bound then store remove it from the bindings array. */
		prg->rtd->pop_binding( pda_run, parse_tree );
	}

	if ( pda_run->steps == pda_run->target_steps ) {
		debug( prg, REALM_PARSE, "trigger parse stop, "
				"steps = target = %d\n", pda_run->target_steps );
		pda_run->stop = true;
	}

	/* Downref the tree that was sent back and free the kid. */
	colm_tree_downref( prg, sp, parse_tree->shadow->tree );
	kid_free( prg, parse_tree->shadow );
	parse_tree_free( pda_run, parse_tree );
}

static void set_region( struct pda_run *pda_run, int empty_ignore, parse_tree_t *tree )
{
	if ( empty_ignore ) {
		/* Recording the next region. */
		tree->retry_region = pda_run->next_region_ind;
		if ( pda_run->pda_tables->token_regions[tree->retry_region+1] != 0 )
			pda_run->num_retry += 1;
	}
}

static void ignore_tree( program_t *prg, struct pda_run *pda_run, tree_t *tree )
{
	int empty_ignore = pda_run->accum_ignore == 0;

	colm_increment_steps( pda_run );

	parse_tree_t *parse_tree = parse_tree_allocate( pda_run );
	parse_tree->shadow = kid_allocate( prg );
	parse_tree->shadow->tree = tree;

	parse_tree->next = pda_run->accum_ignore;
	pda_run->accum_ignore = parse_tree;

	colm_transfer_reverse_code( pda_run, parse_tree );

	if ( pda_run->pre_region >= 0 )
		parse_tree->flags |= PF_RIGHT_IGNORE;

	set_region( pda_run, empty_ignore, pda_run->accum_ignore );
}

static void ignore_tree_art( program_t *prg, struct pda_run *pda_run, tree_t *tree )
{
	int empty_ignore = pda_run->accum_ignore == 0;

	colm_increment_steps( pda_run );

	parse_tree_t *parse_tree = parse_tree_allocate( pda_run );
	parse_tree->flags |= PF_ARTIFICIAL;
	parse_tree->shadow = kid_allocate( prg );
	parse_tree->shadow->tree = tree;

	parse_tree->next = pda_run->accum_ignore;
	pda_run->accum_ignore = parse_tree;

	colm_transfer_reverse_code( pda_run, parse_tree );

	set_region( pda_run, empty_ignore, pda_run->accum_ignore );
}

kid_t *make_token_with_data( program_t *prg, struct pda_run *pda_run,
		struct stream_impl *is, int id, head_t *tokdata )
{
	/* Make the token object. */
	long object_length = prg->rtd->lel_info[id].object_length;
	kid_t *attrs = alloc_attrs( prg, object_length );

	kid_t *input = 0;
	input = kid_allocate( prg );
	input->tree = tree_allocate( prg );

	debug( prg, REALM_PARSE, "made token %p\n", input->tree );

	input->tree->refs = 1;
	input->tree->id = id;
	input->tree->tokdata = tokdata;

	/* No children and ignores get added later. */
	input->tree->child = attrs;

	struct lang_el_info *lel_info = prg->rtd->lel_info;
	if ( lel_info[id].num_capture_attr > 0 ) {
		int i;
		for ( i = 0; i < lel_info[id].num_capture_attr; i++ ) {
			CaptureAttr *ca = &prg->rtd->capture_attr[lel_info[id].capture_attr + i];
			head_t *data = string_alloc_full( prg, 
					pda_run->mark[ca->mark_enter],
					pda_run->mark[ca->mark_leave] -
							pda_run->mark[ca->mark_enter] );
			tree_t *string = construct_string( prg, data );
			colm_tree_upref( string );
			colm_tree_set_field( prg, input->tree, ca->offset, string );
		}
	}

	return input;
}

static void report_parse_error( program_t *prg, tree_t **sp, struct pda_run *pda_run )
{
	kid_t *kid = pda_run->bt_point;
	head_t *deepest = 0;
	while ( kid != 0 ) {
		head_t *head = kid->tree->tokdata;
		if ( head != 0 && head->location != 0 ) {
			if ( deepest == 0 || head->location->byte > deepest->location->byte ) 
				deepest = head;
		}
		kid = kid->next;
	}

	head_t *error_head = 0;

	/* If there are no error points on record assume the error occurred at the
	 * beginning of the stream. */
	if ( deepest == 0 )  {
		error_head = string_alloc_full( prg, "<input>:1:1: parse error", 32 );
		error_head->location = location_allocate( prg );
		error_head->location->line = 1;
		error_head->location->column = 1;
	}
	else {
		debug( prg, REALM_PARSE, "deepest location byte: %d\n",
				deepest->location->byte );

		const char *name = deepest->location->name;
		long line = deepest->location->line;
		long i, column = deepest->location->column;
		long byte = deepest->location->byte;

		for ( i = 0; i < deepest->length; i++ ) {
			if ( deepest->data[i] != '\n' )
				column += 1;
			else {
				line += 1;
				column = 1;
			}
			byte += 1;
		}

		if ( name == 0 )
			name = "<input>";
		char *formatted = malloc( strlen( name ) + 128 );
		sprintf( formatted, "%s:%ld:%ld: parse error", name, line, column );
		error_head = string_alloc_full( prg, formatted, strlen(formatted) );
		free( formatted );

		error_head->location = location_allocate( prg );

		error_head->location->name = deepest->location->name;
		error_head->location->line = line;
		error_head->location->column = column;
		error_head->location->byte = byte;
	}

	tree_t *tree = construct_string( prg, error_head );
	colm_tree_downref( prg, sp, pda_run->parse_error_text );
	pda_run->parse_error_text = tree;
	colm_tree_upref( pda_run->parse_error_text );
}

static void attach_right_ignore( program_t *prg, tree_t **sp,
		struct pda_run *pda_run, parse_tree_t *parse_tree )
{
	if ( pda_run->accum_ignore == 0 )
		return;

	if ( pda_run->stack_top->id > 0 && 
			pda_run->stack_top->id < prg->rtd->first_non_term_id )
	{
		/* OK, do it */
		debug( prg, REALM_PARSE, "attaching right ignore\n" );

		/* Reset. */
		assert( ! ( parse_tree->flags & PF_RIGHT_IL_ATTACHED ) );

		parse_tree_t *accum = pda_run->accum_ignore;

		parse_tree_t *stop_at = 0, *use = accum;
		while ( use != 0 ) {
			if ( ! (use->flags & PF_RIGHT_IGNORE) )
				stop_at = use;
			use = use->next;
		}

		if ( stop_at != 0 ) {
			/* Stop at was set. Make it the last item in the igore list. Take
			 * the rest. */
			accum = stop_at->next;
			stop_at->next = 0;
		}
		else {
			/* Stop at was never set. All right ignore. Use it all. */
			pda_run->accum_ignore = 0;
		}

		/* The data list needs to be extracted and reversed. The parse tree list
		 * can remain in stack order. */
		parse_tree_t *child = accum, *last = 0;
		kid_t *data_child = 0, *data_last = 0;

		while ( child ) {
			data_child = child->shadow;
			parse_tree_t *next = child->next;

			/* Reverse the lists. */
			data_child->next = data_last;
			child->next = last;

			/* Detach the parse tree from the data tree. */
			child->shadow = 0;

			/* Keep the last for reversal. */
			data_last = data_child;
			last = child;

			child = next;
		}

		/* Last is now the first. */
		parse_tree->right_ignore = last;

		if ( data_child != 0 ) {
			debug( prg, REALM_PARSE, "attaching ignore right\n" );

			kid_t *ignore_kid = data_last;

			/* Copy the ignore list first if we need to attach it as a right
			 * ignore. */
			tree_t *right_ignore = 0;

			right_ignore = tree_allocate( prg );
			right_ignore->id = LEL_ID_IGNORE;
			right_ignore->child = ignore_kid;

			tree_t *push_to = parse_tree->shadow->tree;

			push_to = push_right_ignore( prg, push_to, right_ignore );

			parse_tree->shadow->tree = push_to;

			parse_tree->flags |= PF_RIGHT_IL_ATTACHED;
		}
	}
}

static void attach_left_ignore( program_t *prg, tree_t **sp,
		struct pda_run *pda_run, parse_tree_t *parse_tree )
{
	/* Reset. */
	assert( ! ( parse_tree->flags & PF_LEFT_IL_ATTACHED ) );

	parse_tree_t *accum = pda_run->accum_ignore;
	pda_run->accum_ignore = 0;

	/* The data list needs to be extracted and reversed. The parse tree list
	 * can remain in stack order. */
	parse_tree_t *child = accum, *last = 0;
	kid_t *data_child = 0, *data_last = 0;

	while ( child ) {
		data_child = child->shadow;
		parse_tree_t *next = child->next;

		/* Reverse the lists. */
		data_child->next = data_last;
		child->next = last;

		/* Detach the parse tree from the data tree. */
		child->shadow = 0;

		/* Keep the last for reversal. */
		data_last = data_child;
		last = child;

		child = next;
	}

	/* Last is now the first. */
	parse_tree->left_ignore = last;

	if ( data_child != 0 ) {
		debug( prg, REALM_PARSE, "attaching left ignore\n" );

		kid_t *ignore_kid = data_child;

		/* Make the ignore list for the left-ignore. */
		tree_t *left_ignore = tree_allocate( prg );
		left_ignore->id = LEL_ID_IGNORE;
		left_ignore->child = ignore_kid;

		tree_t *push_to = parse_tree->shadow->tree;

		push_to = push_left_ignore( prg, push_to, left_ignore );

		parse_tree->shadow->tree = push_to;

		parse_tree->flags |= PF_LEFT_IL_ATTACHED;
	}
}

/* Not currently used. Need to revive this. WARNING: untested changes here */
static void detach_right_ignore( program_t *prg, tree_t **sp,
		struct pda_run *pda_run, parse_tree_t *parse_tree )
{
	/* Right ignore are immediately discarded since they are copies of
	 * left-ignores. */
	tree_t *right_ignore = 0;
	if ( parse_tree->flags & PF_RIGHT_IL_ATTACHED ) {
		tree_t *pop_from = parse_tree->shadow->tree;

		pop_from = pop_right_ignore( prg, sp, pop_from, &right_ignore );

		parse_tree->shadow->tree = pop_from;

		parse_tree->flags &= ~PF_RIGHT_IL_ATTACHED;
	}

	if ( parse_tree->right_ignore != 0 ) {
		assert( right_ignore != 0 );

		/* Transfer the trees to accumIgnore. */
		parse_tree_t *ignore = parse_tree->right_ignore;
		parse_tree->right_ignore = 0;

		kid_t *data_ignore = right_ignore->child;
		right_ignore->child = 0;

		parse_tree_t *last = 0;
		kid_t *data_last = 0;
		while ( ignore != 0 ) {
			parse_tree_t *next = ignore->next;
			kid_t *data_next = data_ignore->next;

			/* Put the data trees underneath the parse trees. */
			ignore->shadow = data_ignore;

			/* Reverse. */
			ignore->next = last;
			data_ignore->next = data_last;

			/* Keep last for reversal. */
			last = ignore;
			data_last = data_ignore;

			ignore = next;
			data_ignore = data_next;
		}

		pda_run->accum_ignore = last;

		colm_tree_downref( prg, sp, right_ignore );
	}
}

static void detach_left_ignore( program_t *prg, tree_t **sp,
		struct pda_run *pda_run, parse_tree_t *parse_tree )
{
	/* Detach left. */
	tree_t *left_ignore = 0;
	if ( parse_tree->flags & PF_LEFT_IL_ATTACHED ) {
		tree_t *pop_from = parse_tree->shadow->tree;

		pop_from = pop_left_ignore( prg, sp, pop_from, &left_ignore );

		parse_tree->shadow->tree = pop_from;

		parse_tree->flags &= ~PF_LEFT_IL_ATTACHED;
	}

	if ( parse_tree->left_ignore != 0 ) {
		assert( left_ignore != 0 );

		/* Transfer the trees to accumIgnore. */
		parse_tree_t *ignore = parse_tree->left_ignore;
		parse_tree->left_ignore = 0;

		kid_t *data_ignore = left_ignore->child;
		left_ignore->child = 0;

		parse_tree_t *last = 0;
		kid_t *data_last = 0;
		while ( ignore != 0 ) {
			parse_tree_t *next = ignore->next;
			kid_t *data_next = data_ignore->next;

			/* Put the data trees underneath the parse trees. */
			ignore->shadow = data_ignore;

			/* Reverse. */
			ignore->next = last;
			data_ignore->next = data_last;

			/* Keep last for reversal. */
			last = ignore;
			data_last = data_ignore;

			ignore = next;
			data_ignore = data_next;
		}

		pda_run->accum_ignore = last;
	}

	colm_tree_downref( prg, sp, left_ignore );
}

static int is_parser_stop_finished( struct pda_run *pda_run )
{
	int done = 
			pda_run->stack_top->next != 0 && 
			pda_run->stack_top->next->next == 0 &&
			pda_run->stack_top->id == pda_run->stop_target;
	return done;
}

static void handle_error( program_t *prg, tree_t **sp, struct pda_run *pda_run )
{
	/* Check the result. */
	if ( pda_run->parse_error ) {
		/* Error occured in the top-level parser. */
		report_parse_error( prg, sp, pda_run );
	}
	else {
		if ( is_parser_stop_finished( pda_run ) ) {
			debug( prg, REALM_PARSE, "stopping the parse\n" );
			pda_run->stop_parsing = true;
		}
	}
}

static head_t *extract_match( program_t *prg, tree_t **sp,
		struct pda_run *pda_run, struct stream_impl *is )
{
	long length = pda_run->toklen;

	//debug( prg, REALM_PARSE, "extracting token of length: %ld\n", length );

	struct run_buf *run_buf = pda_run->consume_buf;
	if ( run_buf == 0 || length > ( FSM_BUFSIZE - run_buf->length ) ) {
		run_buf = new_run_buf( length );
		run_buf->next = pda_run->consume_buf;
		pda_run->consume_buf = run_buf;
	}

	char *dest = run_buf->data + run_buf->length;

	is->funcs->get_data( is, dest, length );
	location_t *location = location_allocate( prg );
	is->funcs->consume_data( prg, sp, is, length, location );

	run_buf->length += length;

	pda_run->p = pda_run->pe = 0;
	pda_run->toklen = 0;
	pda_run->tokstart = 0;

	head_t *head = colm_string_alloc_pointer( prg, dest, length );

	head->location = location;

	debug( prg, REALM_PARSE, "location byte: %d\n", is->byte );

	return head;
}

static head_t *extract_no_d( program_t *prg, tree_t **sp,
		struct pda_run *pda_run, struct stream_impl *is )
{
	long length = pda_run->toklen;

	/* Just a consume, no data allocate. */
	location_t *location = location_allocate( prg );
	is->funcs->consume_data( prg, sp, is, length, location );

	pda_run->p = pda_run->pe = 0;
	pda_run->toklen = 0;
	pda_run->tokstart = 0;

	head_t *head = colm_string_alloc_pointer( prg, 0, 0 );

	head->location = location;

	debug( prg, REALM_PARSE, "location byte: %d\n", is->byte );

	return head;
}

static head_t *extract_no_l( program_t *prg, tree_t **sp,
		struct pda_run *pda_run, struct stream_impl *is )
{
	long length = pda_run->toklen;

	//debug( prg, REALM_PARSE, "extracting token of length: %ld\n", length );

	struct run_buf *run_buf = pda_run->consume_buf;
	if ( run_buf == 0 || length > ( FSM_BUFSIZE - run_buf->length ) ) {
		run_buf = new_run_buf( length );
		run_buf->next = pda_run->consume_buf;
		pda_run->consume_buf = run_buf;
	}

	char *dest = run_buf->data + run_buf->length;

	is->funcs->get_data( is, dest, length );

	/* Using a dummpy location. */
	location_t location;
	memset( &location, 0, sizeof( location ) );
	is->funcs->consume_data( prg, sp, is, length, &location );

	run_buf->length += length;

	pda_run->p = pda_run->pe = 0;
	pda_run->toklen = 0;
	pda_run->tokstart = 0;

	head_t *head = colm_string_alloc_pointer( prg, dest, length );

	/* Don't pass the location. */
	head->location = 0;

	debug( prg, REALM_PARSE, "location byte: %d\n", is->byte );

	return head;
}

static head_t *consume_match( program_t *prg, tree_t **sp,
		struct pda_run *pda_run, struct stream_impl *is )
{
	long length = pda_run->toklen;

	/* No data or location returned. We just consume the data. */
	location_t dummy_loc;
	memset( &dummy_loc, 0, sizeof(dummy_loc) );
	is->funcs->consume_data( prg, sp, is, length, &dummy_loc );

	pda_run->p = pda_run->pe = 0;
	pda_run->toklen = 0;
	pda_run->tokstart = 0;

	debug( prg, REALM_PARSE, "location byte: %d\n", is->byte );

	return 0;
}


static head_t *peek_match( program_t *prg, struct pda_run *pda_run, struct stream_impl *is )
{
	long length = pda_run->toklen;

	struct run_buf *run_buf = pda_run->consume_buf;
	if ( run_buf == 0 || length > ( FSM_BUFSIZE - run_buf->length ) ) {
		run_buf = new_run_buf( 0 );
		run_buf->next = pda_run->consume_buf;
		pda_run->consume_buf = run_buf;
	}

	char *dest = run_buf->data + run_buf->length;

	is->funcs->get_data( is, dest, length );

	pda_run->p = pda_run->pe = 0;
	pda_run->toklen = 0;

	head_t *head = colm_string_alloc_pointer( prg, dest, length );

	head->location = location_allocate( prg );
	head->location->line = is->line;
	head->location->column = is->column;
	head->location->byte = is->byte;

	debug( prg, REALM_PARSE, "location byte: %d\n", is->byte );

	return head;
}


static void send_ignore( program_t *prg, tree_t **sp,
		struct pda_run *pda_run, struct stream_impl *is, long id )
{
	if ( prg->rtd->reducer_need_ign( prg, pda_run ) == RN_NONE ) {
		consume_match( prg, sp, pda_run, is );
	}
	else {
		debug( prg, REALM_PARSE, "ignoring: %s\n", prg->rtd->lel_info[id].name );

		/* Make the ignore string. */
		head_t *ignore_str = extract_match( prg, sp, pda_run, is );

		debug( prg, REALM_PARSE, "ignoring: %.*s\n", ignore_str->length, ignore_str->data );

		tree_t *tree = tree_allocate( prg );
		tree->refs = 1;
		tree->id = id;
		tree->tokdata = ignore_str;

		/* Send it to the pdaRun. */
		ignore_tree( prg, pda_run, tree );
	}
}

static void send_token( program_t *prg, tree_t **sp,
		struct pda_run *pda_run, struct stream_impl *is, long id )
{
	int empty_ignore = pda_run->accum_ignore == 0;

	/* Make the token data. */
	head_t *tokdata = 0;
	int rn = prg->rtd->reducer_need_tok( prg, pda_run, id );

	switch ( rn ) {
		case RN_NONE:
			tokdata = consume_match( prg, sp, pda_run, is );
			break;
		case RN_DATA:
			tokdata = extract_no_l( prg, sp, pda_run, is );
			break;
		case RN_LOC:
			tokdata = extract_no_d( prg, sp, pda_run, is );
			break;
		case RN_BOTH:
			tokdata = extract_match( prg, sp, pda_run, is );
			break;
	}

	debug( prg, REALM_PARSE, "token: %s  text: %.*s\n",
		prg->rtd->lel_info[id].name,
		string_length(tokdata), string_data(tokdata) );

	kid_t *input = make_token_with_data( prg, pda_run, is, id, tokdata );

	colm_increment_steps( pda_run );

	parse_tree_t *parse_tree = parse_tree_allocate( pda_run );
	parse_tree->id = input->tree->id;
	parse_tree->shadow = input;
		
	pda_run->parse_input = parse_tree;

	/* Store any alternate scanning region. */
	if ( input != 0 && pda_run->pda_cs >= 0 )
		set_region( pda_run, empty_ignore, parse_tree );
}

static void send_tree( program_t *prg, tree_t **sp, struct pda_run *pda_run,
		struct stream_impl *is )
{
	kid_t *input = kid_allocate( prg );
	input->tree = is->funcs->consume_tree( is );

	colm_increment_steps( pda_run );

	parse_tree_t *parse_tree = parse_tree_allocate( pda_run );
	parse_tree->id = input->tree->id;
	parse_tree->flags |= PF_ARTIFICIAL;
	parse_tree->shadow = input;
	
	pda_run->parse_input = parse_tree;
}

static void send_ignore_tree( program_t *prg, tree_t **sp,
		struct pda_run *pda_run, struct stream_impl *is )
{
	tree_t *tree = is->funcs->consume_tree( is );
	ignore_tree_art( prg, pda_run, tree );
}

static void send_collect_ignore( program_t *prg, tree_t **sp,
		struct pda_run *pda_run, struct stream_impl *is, int id )
{
	debug( prg, REALM_PARSE, "token: CI\n" );

	int empty_ignore = pda_run->accum_ignore == 0;

	/* Make the token data. */
	head_t *tokdata = head_allocate( prg );
	tokdata->location = location_allocate( prg );
	tokdata->location->line = is->line;
	tokdata->location->column = is->column;
	tokdata->location->byte = is->byte;

	debug( prg, REALM_PARSE, "token: %s  text: %.*s\n",
		prg->rtd->lel_info[id].name,
		string_length(tokdata), string_data(tokdata) );

	kid_t *input = make_token_with_data( prg, pda_run, is, id, tokdata );

	colm_increment_steps( pda_run );

	parse_tree_t *parse_tree = parse_tree_allocate( pda_run );
	parse_tree->id = input->tree->id;
	parse_tree->shadow = input;

	pda_run->parse_input = parse_tree;

	/* Store any alternate scanning region. */
	if ( input != 0 && pda_run->pda_cs >= 0 )
		set_region( pda_run, empty_ignore, parse_tree );
}

/* Offset can be used to look at the next nextRegionInd. */
static int get_next_region( struct pda_run *pda_run, int offset )
{
	return pda_run->pda_tables->token_regions[pda_run->next_region_ind+offset];
}

static int get_next_pre_region( struct pda_run *pda_run )
{
	return pda_run->pda_tables->token_pre_regions[pda_run->next_region_ind];
}

static void send_eof( program_t *prg, tree_t **sp, struct pda_run *pda_run, struct stream_impl *is )
{
	debug( prg, REALM_PARSE, "token: _EOF\n" );

	colm_increment_steps( pda_run );

	head_t *head = head_allocate( prg );
	head->location = location_allocate( prg );
	head->location->line = is->line;
	head->location->column = is->column;
	head->location->byte = is->byte;

	kid_t *input = kid_allocate( prg );
	input->tree = tree_allocate( prg );

	input->tree->refs = 1;
	input->tree->id = prg->rtd->eof_lel_ids[pda_run->parser_id];
	input->tree->tokdata = head;

	/* Set the state using the state of the parser. */
	pda_run->region = get_next_region( pda_run, 0 );
	pda_run->pre_region = get_next_pre_region( pda_run );
	pda_run->fsm_cs = pda_run->fsm_tables->entry_by_region[pda_run->region];

	parse_tree_t *parse_tree = parse_tree_allocate( pda_run );
	parse_tree->id = input->tree->id;
	parse_tree->shadow = input;
	
	pda_run->parse_input = parse_tree;
}

static void new_token( program_t *prg, struct pda_run *pda_run )
{
	pda_run->p = pda_run->pe = 0;
	pda_run->toklen = 0;
	pda_run->eof = 0;

	/* Init the scanner vars. */
	pda_run->act = 0;
	pda_run->tokstart = 0;
	pda_run->tokend = 0;
	pda_run->matched_token = 0;

	/* Set the state using the state of the parser. */
	pda_run->region = get_next_region( pda_run, 0 );
	pda_run->pre_region = get_next_pre_region( pda_run );
	if ( pda_run->pre_region > 0 ) {
		pda_run->fsm_cs = pda_run->fsm_tables->entry_by_region[pda_run->pre_region];
		pda_run->next_cs = pda_run->fsm_tables->entry_by_region[pda_run->region];
	}
	else {
		pda_run->fsm_cs = pda_run->fsm_tables->entry_by_region[pda_run->region];
	}


	/* Clear the mark array. */
	memset( pda_run->mark, 0, sizeof(pda_run->mark) );
}

static void push_bt_point( program_t *prg, struct pda_run *pda_run )
{
	tree_t *tree = 0;
	if ( pda_run->accum_ignore != 0 ) 
		tree = pda_run->accum_ignore->shadow->tree;
	else if ( pda_run->token_list != 0 )
		tree = pda_run->token_list->kid->tree;

	if ( tree != 0 ) {
		debug( prg, REALM_PARSE, "pushing bt point with location byte %d\n", 
				( tree != 0 && tree->tokdata != 0 && tree->tokdata->location != 0 ) ? 
				tree->tokdata->location->byte : 0 );

		kid_t *kid = kid_allocate( prg );
		kid->tree = tree;
		colm_tree_upref( tree );
		kid->next = pda_run->bt_point;
		pda_run->bt_point = kid;
	}
}


#define SCAN_UNDO              -7
#define SCAN_IGNORE            -6
#define SCAN_TREE              -5
#define SCAN_TRY_AGAIN_LATER   -4
#define SCAN_ERROR             -3
#define SCAN_LANG_EL           -2
#define SCAN_EOF               -1

static long scan_token( program_t *prg, struct pda_run *pda_run, struct stream_impl *is )
{
	if ( pda_run->trigger_undo )
		return SCAN_UNDO;

	while ( true ) {
		char *pd = 0;
		int len = 0;
		int type = is->funcs->get_parse_block( is, pda_run->toklen, &pd, &len );

		switch ( type ) {
			case INPUT_DATA:
				pda_run->p = pd;
				pda_run->pe = pd + len;
				break;

			case INPUT_EOS:
				pda_run->p = pda_run->pe = 0;
				if ( pda_run->tokstart != 0 )
					pda_run->eof = 1;
				debug( prg, REALM_SCAN, "EOS *******************\n" );
				break;

			case INPUT_EOF:
				pda_run->p = pda_run->pe = 0;
				if ( pda_run->tokstart != 0 )
					pda_run->eof = 1;
				else 
					return SCAN_EOF;
				break;

			case INPUT_EOD:
				pda_run->p = pda_run->pe = 0;
				return SCAN_TRY_AGAIN_LATER;

			case INPUT_LANG_EL:
				if ( pda_run->tokstart != 0 )
					pda_run->eof = 1;
				else 
					return SCAN_LANG_EL;
				break;

			case INPUT_TREE:
				if ( pda_run->tokstart != 0 )
					pda_run->eof = 1;
				else 
					return SCAN_TREE;
				break;
			case INPUT_IGNORE:
				if ( pda_run->tokstart != 0 )
					pda_run->eof = 1;
				else
					return SCAN_IGNORE;
				break;
		}

		prg->rtd->fsm_execute( pda_run, is );

		/* First check if scanning stopped because we have a token. */
		if ( pda_run->matched_token > 0 ) {
			/* If the token has a marker indicating the end (due to trailing
			 * context) then adjust data now. */
			struct lang_el_info *lel_info = prg->rtd->lel_info;
			if ( lel_info[pda_run->matched_token].mark_id >= 0 )
				pda_run->p = pda_run->mark[lel_info[pda_run->matched_token].mark_id];

			return pda_run->matched_token;
		}

		/* Check for error. */
		if ( pda_run->fsm_cs == pda_run->fsm_tables->error_state ) {
			/* If a token was started, but not finished (tokstart != 0) then
			 * restore data to the beginning of that token. */
			if ( pda_run->tokstart != 0 )
				pda_run->p = pda_run->tokstart;

			/* Check for a default token in the region. If one is there
			 * then send it and continue with the processing loop. */
			if ( prg->rtd->region_info[pda_run->region].default_token >= 0 ) {
				pda_run->toklen = 0;
				return prg->rtd->region_info[pda_run->region].default_token;
			}

			return SCAN_ERROR;
		}

		/* Check for no match on eof (trailing data that partially matches a token). */
		if ( pda_run->eof )
			return SCAN_ERROR;

		/* Got here because the state machine didn't match a token or encounter
		 * an error. Must be because we got to the end of the buffer data. */
		assert( pda_run->p == pda_run->pe );
	}

	/* Should not be reached. */
	return SCAN_ERROR;
}

static tree_t *get_parsed_root( struct pda_run *pda_run, int stop )
{
	if ( pda_run->parse_error )
		return 0;
	else if ( stop ) {
		if ( pda_run->stack_top->shadow != 0 )
			return pda_run->stack_top->shadow->tree;
	}
	else {
		if ( pda_run->stack_top->next->shadow != 0 )
			return pda_run->stack_top->next->shadow->tree;
	}
	return 0;
}

static void clear_parse_tree( program_t *prg, tree_t **sp,
		struct pda_run *pda_run, parse_tree_t *pt )
{
	tree_t **top = vm_ptop();

	if ( pt == 0 )
		return;

free_tree:
	if ( pt->next != 0 ) {
		vm_push_ptree( pt->next );
	}

	if ( pt->left_ignore != 0 ) {
		vm_push_ptree( pt->left_ignore );
	}

	if ( pt->child != 0 ) {
		vm_push_ptree( pt->child );
	}

	if ( pt->right_ignore != 0 ) {
		vm_push_ptree( pt->right_ignore );
	}

	if ( pt->shadow != 0 ) {
		colm_tree_downref( prg, sp, pt->shadow->tree );
		kid_free( prg, pt->shadow );
	}

	parse_tree_free( pda_run, pt );

	/* Any trees to downref? */
	if ( sp != top ) {
		pt = vm_pop_ptree();
		goto free_tree;
	}
}

void colm_pda_clear( program_t *prg, tree_t **sp, struct pda_run *pda_run )
{
	clear_fsm_run( prg, pda_run );

	/* Remaining stack and parse trees underneath. */
	clear_parse_tree( prg, sp, pda_run, pda_run->stack_top );
	pda_run->stack_top = 0;

	/* Traverse the token list downreffing. */
	ref_t *ref = pda_run->token_list;
	while ( ref != 0 ) {
		ref_t *next = ref->next;
		kid_free( prg, (kid_t*)ref );
		ref = next;
	}
	pda_run->token_list = 0;

	/* Traverse the btPoint list downreffing */
	kid_t *btp = pda_run->bt_point;
	while ( btp != 0 ) {
		kid_t *next = btp->next;
		colm_tree_downref( prg, sp, btp->tree );
		kid_free( prg, (kid_t*)btp );
		btp = next;
	}
	pda_run->bt_point = 0;

	/* Clear out any remaining ignores. */
	clear_parse_tree( prg, sp, pda_run, pda_run->accum_ignore );
	pda_run->accum_ignore = 0;

	/* Clear the input list (scanned tokes, sent trees). */
	clear_parse_tree( prg, sp, pda_run, pda_run->parse_input );
	pda_run->parse_input = 0;

	colm_rcode_downref_all( prg, sp, &pda_run->reverse_code );
	colm_rt_code_vect_empty( &pda_run->reverse_code );
	colm_rt_code_vect_empty( &pda_run->rcode_collect );

	colm_tree_downref( prg, sp, pda_run->parse_error_text );

	if ( pda_run->reducer ) {
		long local_lost = pool_alloc_num_lost( &pda_run->local_pool );

		if ( local_lost )
			message( "warning: reducer local lost parse trees: %ld\n", local_lost );
		pool_alloc_clear( &pda_run->local_pool );
	}
}

void colm_pda_init( program_t *prg, struct pda_run *pda_run, struct pda_tables *tables,
		int parser_id, long stop_target, int revert_on, struct_t *context, int reducer )
{
	memset( pda_run, 0, sizeof(struct pda_run) );

	pda_run->pda_tables = tables;
	pda_run->parser_id = parser_id;
	pda_run->stop_target = stop_target;
	pda_run->revert_on = revert_on;
	pda_run->target_steps = -1;
	pda_run->reducer = reducer;

	/* An initial commit shift count of -1 means we won't ever back up to zero
	 * shifts and think parsing cannot continue. */
	pda_run->shift_count = 0;
	pda_run->commit_shift_count = -1;

	if ( reducer ) {
		init_pool_alloc( &pda_run->local_pool, sizeof(parse_tree_t) +
				prg->rtd->commit_union_sz(reducer) );
		pda_run->parse_tree_pool = &pda_run->local_pool;
	}
	else {
		pda_run->parse_tree_pool = &prg->parse_tree_pool;
	}

	debug( prg, REALM_PARSE, "initializing struct pda_run\n" );

	/* FIXME: need the right one here. */
	pda_run->pda_cs = prg->rtd->start_states[pda_run->parser_id];

	kid_t *sentinal = kid_allocate( prg );
	sentinal->tree = tree_allocate( prg );
	sentinal->tree->refs = 1;

	/* Init the element allocation variables. */
	pda_run->stack_top = parse_tree_allocate( pda_run );
	pda_run->stack_top->state = -1;
	pda_run->stack_top->shadow = sentinal;

	pda_run->num_retry = 0;
	pda_run->next_region_ind = pda_run->pda_tables->token_region_inds[pda_run->pda_cs];
	pda_run->stop_parsing = false;
	pda_run->accum_ignore = 0;
	pda_run->bt_point = 0;
	pda_run->check_next = false;
	pda_run->check_stop = false;

	prg->rtd->init_bindings( pda_run );

	init_rt_code_vect( &pda_run->reverse_code );
	init_rt_code_vect( &pda_run->rcode_collect );

	pda_run->context = context;
	pda_run->parse_error = 0;
	pda_run->parse_input = 0;
	pda_run->trigger_undo = 0;

	pda_run->token_id = 0;

	pda_run->on_deck = false;
	pda_run->parsed = 0;
	pda_run->reject = false;

	pda_run->rc_block_count = 0;

	init_fsm_run( prg, pda_run );
	new_token( prg, pda_run );
}

static long stack_top_target( program_t *prg, struct pda_run *pda_run )
{
	long state;
	if ( pda_run->stack_top->state < 0 )
		state = prg->rtd->start_states[pda_run->parser_id];
	else {
		unsigned shift = pda_run->stack_top->id - 
				pda_run->pda_tables->keys[pda_run->stack_top->state<<1];
		unsigned offset = pda_run->pda_tables->offsets[pda_run->stack_top->state] + shift;
		int index = pda_run->pda_tables->indicies[offset];
		state = pda_run->pda_tables->targs[index];
	}
	return state;
}

/*
 * shift:         retry goes into lower of shifted node.
 * reduce:        retry goes into upper of reduced node.
 * shift-reduce:  cannot be a retry
 */

/* Stops on:
 *   PCR_REDUCTION
 *   PCR_REVERSE
 */
static long parse_token( program_t *prg, tree_t **sp,
		struct pda_run *pda_run, struct stream_impl *is, long entry )
{
	int pos;
	unsigned int *action;
	int rhs_len;
	int owner;
	int induce_reject;
	int ind_pos;

	/* COROUTINE */
	switch ( entry ) {
	case PCR_START:

	/* The scanner will send a null token if it can't find a token. */
	if ( pda_run->parse_input == 0 )
		goto parse_error;

	/* This will cause parseInput to be lost. This 
	 * path should be traced. */
	if ( pda_run->pda_cs < 0 )
		return PCR_DONE;

	/* Record the state in the parse tree. */
	pda_run->parse_input->state = pda_run->pda_cs;

again:
	if ( pda_run->parse_input == 0 )
		goto _out;

	pda_run->lel = pda_run->parse_input;
	pda_run->cur_state = pda_run->pda_cs;

	if ( pda_run->lel->id < pda_run->pda_tables->keys[pda_run->cur_state<<1] ||
			pda_run->lel->id > pda_run->pda_tables->keys[(pda_run->cur_state<<1)+1] )
	{
		debug( prg, REALM_PARSE, "parse error, no transition 1\n" );
		push_bt_point( prg, pda_run );
		goto parse_error;
	}

	ind_pos = pda_run->pda_tables->offsets[pda_run->cur_state] + 
		(pda_run->lel->id - pda_run->pda_tables->keys[pda_run->cur_state<<1]);

	owner = pda_run->pda_tables->owners[ind_pos];
	if ( owner != pda_run->cur_state ) {
		debug( prg, REALM_PARSE, "parse error, no transition 2\n" );
		push_bt_point( prg, pda_run );
		goto parse_error;
	}

	pos = pda_run->pda_tables->indicies[ind_pos];
	if ( pos < 0 ) {
		debug( prg, REALM_PARSE, "parse error, no transition 3\n" );
		push_bt_point( prg, pda_run );
		goto parse_error;
	}

	/* Checking complete. */

	induce_reject = false;
	pda_run->pda_cs = pda_run->pda_tables->targs[pos];
	action = pda_run->pda_tables->actions + pda_run->pda_tables->act_inds[pos];
	if ( pda_run->lel->retry_lower )
		action += pda_run->lel->retry_lower;

	/*
	 * Shift
	 */

	if ( *action & act_sb ) {
		debug( prg, REALM_PARSE, "shifted: %s\n", 
				prg->rtd->lel_info[pda_run->lel->id].name );
		/* Consume. */
		pda_run->parse_input = pda_run->parse_input->next;

		pda_run->lel->state = pda_run->cur_state;

		/* If its a token then attach ignores and record it in the token list
		 * of the next ignore attachment to use. */
		if ( pda_run->lel->id < prg->rtd->first_non_term_id ) {
			if ( pda_run->lel->cause_reduce == 0 )
				attach_right_ignore( prg, sp, pda_run, pda_run->stack_top );
		}

		pda_run->lel->next = pda_run->stack_top;
		pda_run->stack_top = pda_run->lel;

		/* If its a token then attach ignores and record it in the token list
		 * of the next ignore attachment to use. */
		if ( pda_run->lel->id < prg->rtd->first_non_term_id ) {
			attach_left_ignore( prg, sp, pda_run, pda_run->lel );

			ref_t *ref = (ref_t*)kid_allocate( prg );
			ref->kid = pda_run->lel->shadow;
			//colm_tree_upref( pdaRun->tree );
			ref->next = pda_run->token_list;
			pda_run->token_list = ref;
		}

		if ( action[1] == 0 )
			pda_run->lel->retry_lower = 0;
		else {
			debug( prg, REALM_PARSE, "retry: %p\n", pda_run->stack_top );
			pda_run->lel->retry_lower += 1;
			assert( pda_run->lel->retry_upper == 0 );
			/* FIXME: Has the retry already been counted? */
			pda_run->num_retry += 1; 
		}

		pda_run->shift_count += 1;
	}

	/* 
	 * Commit
	 */

	if ( pda_run->pda_tables->commit_len[pos] != 0 ) {
		debug( prg, REALM_PARSE, "commit point\n" );
		pda_run->commit_shift_count = pda_run->shift_count;

		/* Not in a reverting context and the parser result is not used. */
		if ( pda_run->reducer )
			commit_reduce( prg, sp, pda_run );

		if ( pda_run->fail_parsing )
			goto fail;
			
	}

	/*
	 * Reduce
	 */

	if ( *action & act_rb ) {
		int r, object_length;
		parse_tree_t *last, *child;
		kid_t *attrs;
		kid_t *data_last, *data_child;

		/* If there was shift don't attach again. */
		if ( !( *action & act_sb ) && pda_run->lel->id < prg->rtd->first_non_term_id )
			attach_right_ignore( prg, sp, pda_run, pda_run->stack_top );

		pda_run->reduction = *action >> 2;

		if ( pda_run->parse_input != 0 )
			pda_run->parse_input->cause_reduce += 1;

		kid_t *value = kid_allocate( prg );
		value->tree = tree_allocate( prg );
		value->tree->refs = 1;
		value->tree->id = prg->rtd->prod_info[pda_run->reduction].lhs_id;
		value->tree->prod_num = prg->rtd->prod_info[pda_run->reduction].prod_num;

		pda_run->red_lel = parse_tree_allocate( pda_run );
		pda_run->red_lel->id = prg->rtd->prod_info[pda_run->reduction].lhs_id;
		pda_run->red_lel->next = 0;
		pda_run->red_lel->cause_reduce = 0;
		pda_run->red_lel->retry_lower = 0;
		pda_run->red_lel->shadow = value;

		/* Transfer. */
		pda_run->red_lel->retry_upper = pda_run->lel->retry_lower;
		pda_run->lel->retry_lower = 0;

		/* Allocate the attributes. */
		object_length = prg->rtd->lel_info[pda_run->red_lel->id].object_length;
		attrs = alloc_attrs( prg, object_length );

		/* Build the list of children. We will be giving up a reference when we
		 * detach parse tree and data tree, but gaining the reference when we
		 * put the children under the new data tree. No need to alter refcounts
		 * here. */
		rhs_len = prg->rtd->prod_info[pda_run->reduction].length;
		child = last = 0;
		data_child = data_last = 0;
		for ( r = 0; r < rhs_len; r++ ) {

			/* The child. */
			child = pda_run->stack_top;
			data_child = child->shadow;

			/* Pop. */
			pda_run->stack_top = pda_run->stack_top->next;

			/* Detach the parse tree from the data. */
			child->shadow = 0;

			/* Reverse list. */
			child->next = last;
			data_child->next = data_last;

			/* Track last for reversal. */
			last = child;
			data_last = data_child;
		}

		pda_run->red_lel->child = child;
		pda_run->red_lel->shadow->tree->child = kid_list_concat( attrs, data_child );

		debug( prg, REALM_PARSE, "reduced: %s rhsLen %d\n",
				prg->rtd->prod_info[pda_run->reduction].name, rhs_len );
		if ( action[1] == 0 )
			pda_run->red_lel->retry_upper = 0;
		else {
			pda_run->red_lel->retry_upper += 1;
			assert( pda_run->lel->retry_lower == 0 );
			pda_run->num_retry += 1;
			debug( prg, REALM_PARSE, "retry: %p\n", pda_run->red_lel );
		}

		/* When the production is of zero length we stay in the same state.
		 * Otherwise we use the state stored in the first child. */
		pda_run->pda_cs = rhs_len == 0 ? pda_run->cur_state : child->state;

		if ( prg->ctx_dep_parsing && prg->rtd->prod_info[pda_run->reduction].frame_id >= 0 ) {
			/* Frame info for reduction. */
			pda_run->fi = &prg->rtd->frame_info[prg->rtd->prod_info[pda_run->reduction].frame_id];
			pda_run->frame_id = prg->rtd->prod_info[pda_run->reduction].frame_id;
			pda_run->reject = false;
			pda_run->parsed = 0;
			pda_run->code = pda_run->fi->codeWV;

			/* COROUTINE */
			return PCR_REDUCTION;
			case PCR_REDUCTION:

			if ( prg->induce_exit )
				goto fail;

			/* If the lhs was stored and it changed then we need to restore the
			 * original upon backtracking, otherwise downref since we took a
			 * copy above. */
			if ( pda_run->parsed != 0 ) {
				if ( pda_run->parsed != pda_run->red_lel->shadow->tree ) {
					debug( prg, REALM_PARSE, "lhs tree was modified, "
							"adding a restore instruction\n" );
//
//					/* Make it into a parse tree. */
//					tree_t *newPt = prepParseTree( prg, sp, pdaRun->redLel->tree );
//					colm_tree_downref( prg, sp, pdaRun->redLel->tree );
//
//					/* Copy it in. */
//					pdaRun->redLel->tree = newPt;
//					colm_tree_upref( pdaRun->redLel->tree );

					/* Add the restore instruct. */
					append_code_val( &pda_run->rcode_collect, IN_RESTORE_LHS );
					append_word( &pda_run->rcode_collect, (word_t)pda_run->parsed );
					append_code_val( &pda_run->rcode_collect, SIZEOF_CODE + SIZEOF_WORD );
				}
				else {
					/* Not changed. Done with parsed. */
					colm_tree_downref( prg, sp, pda_run->parsed );
				}
				pda_run->parsed = 0;
			}

			/* Pull out the reverse code, if any. */
			colm_make_reverse_code( pda_run );
			colm_transfer_reverse_code( pda_run, pda_run->red_lel );

			/* Perhaps the execution environment is telling us we need to
			 * reject the reduction. */
			induce_reject = pda_run->reject;
		}

		/* If the left hand side was replaced then the only parse algorithm
		 * data that is contained in it will the PF_HAS_RCODE flag. Everthing
		 * else will be in the original. This requires that we restore first
		 * when going backwards and when doing a commit. */

		if ( induce_reject ) {
			debug( prg, REALM_PARSE, "error induced during reduction of %s\n",
					prg->rtd->lel_info[pda_run->red_lel->id].name );
			pda_run->red_lel->state = pda_run->cur_state;
			pda_run->red_lel->next = pda_run->stack_top;
			pda_run->stack_top = pda_run->red_lel;
			/* FIXME: What is the right argument here? */
			push_bt_point( prg, pda_run );
			goto parse_error;
		}

		pda_run->red_lel->next = pda_run->parse_input;
		pda_run->parse_input = pda_run->red_lel;
	}

	goto again;

parse_error:
	debug( prg, REALM_PARSE, "hit error, backtracking\n" );

#if 0
	if ( pda_run->num_retry == 0 ) {
		debug( prg, REALM_PARSE, "out of retries failing parse\n" );
		goto fail;
	}
#endif

	while ( 1 ) {
		if ( pda_run->on_deck ) {
			debug( prg, REALM_BYTECODE, "dropping out for reverse code call\n" );

			pda_run->frame_id = -1;
			pda_run->code = colm_pop_reverse_code( &pda_run->reverse_code );

			/* COROUTINE */
			return PCR_REVERSE;
			case PCR_REVERSE: 

			colm_decrement_steps( pda_run );
		}
		else if ( pda_run->check_next ) {
			pda_run->check_next = false;

			if ( pda_run->next > 0 && pda_run->pda_tables->token_regions[pda_run->next] != 0 ) {
				debug( prg, REALM_PARSE, "found a new region\n" );
				pda_run->num_retry -= 1;
				pda_run->pda_cs = stack_top_target( prg, pda_run );
				pda_run->next_region_ind = pda_run->next;
				return PCR_DONE;
			}
		}
		else if ( pda_run->check_stop ) {
			pda_run->check_stop = false;

			if ( pda_run->stop ) {
				debug( prg, REALM_PARSE, "stopping the backtracking, "
						"steps is %d\n", pda_run->steps );

				pda_run->pda_cs = stack_top_target( prg, pda_run );
				goto _out;
			}
		}
		else if ( pda_run->parse_input != 0 ) {
			/* Either we are dealing with a terminal that was shifted or a
			 * nonterminal that was reduced. */
			if ( pda_run->parse_input->id < prg->rtd->first_non_term_id ) {
				/* This is a terminal. */
				assert( pda_run->parse_input->retry_upper == 0 );

				if ( pda_run->parse_input->retry_lower != 0 ) {
					debug( prg, REALM_PARSE, "found retry targ: %p\n", pda_run->parse_input );

					pda_run->num_retry -= 1;
					pda_run->pda_cs = pda_run->parse_input->state;
					goto again;
				}

				if ( pda_run->parse_input->cause_reduce != 0 ) {
					/* The terminal caused a reduce. Unshift the reduced thing
					 * (will unreduce in the next step. */
					if ( pda_run->shift_count == pda_run->commit_shift_count ) {
						debug( prg, REALM_PARSE, "backed up to commit point, "
								"failing parse\n" );
						goto fail;
					}
					pda_run->shift_count -= 1;

					pda_run->undo_lel = pda_run->stack_top;

					/* Check if we've arrived at the stack sentinal. This guard
					 * is here to allow us to initially set numRetry to one to
					 * cause the parser to backup all the way to the beginning
					 * when an error occurs. */
					if ( pda_run->undo_lel->next == 0 )
						break;

					/* Either we are dealing with a terminal that was
					 * shifted or a nonterminal that was reduced. */
					assert( !(pda_run->stack_top->id < prg->rtd->first_non_term_id) );

					debug( prg, REALM_PARSE, "backing up over non-terminal: %s\n",
							prg->rtd->lel_info[pda_run->stack_top->id].name );

					/* Pop the item from the stack. */
					pda_run->stack_top = pda_run->stack_top->next;

					/* Queue it as next parseInput item. */
					pda_run->undo_lel->next = pda_run->parse_input;
					pda_run->parse_input = pda_run->undo_lel;
				}
				else {
					long region = pda_run->parse_input->retry_region;
					pda_run->next = region > 0 ? region + 1 : 0;
					pda_run->check_next = true;
					pda_run->check_stop = true;

					send_back( prg, sp, pda_run, is, pda_run->parse_input );

					pda_run->parse_input = 0;
				}
			}
			else if ( pda_run->parse_input->flags & PF_HAS_RCODE ) {
				debug( prg, REALM_PARSE, "tree has rcode, setting on deck\n" );
				pda_run->on_deck = true;
				pda_run->parsed = 0;

				/* Only the RCODE flag was in the replaced lhs. All the rest is in
				 * the the original. We read it after restoring. */

				pda_run->parse_input->flags &= ~PF_HAS_RCODE;
			}
			else {
				/* Remove it from the input queue. */
				pda_run->undo_lel = pda_run->parse_input;
				pda_run->parse_input = pda_run->parse_input->next;

				/* Extract children from the child list. */
				parse_tree_t *first = pda_run->undo_lel->child;
				pda_run->undo_lel->child = 0;

				/* This will skip the ignores/attributes, etc. */
				kid_t *data_first = tree_extract_child( prg, pda_run->undo_lel->shadow->tree );

				/* Walk the child list and and push the items onto the parsing
				 * stack one at a time. */
				while ( first != 0 ) {
					/* Get the next item ahead of time. */
					parse_tree_t *next = first->next;
					kid_t *data_next = data_first->next;

					/* Push onto the stack. */
					first->next = pda_run->stack_top;
					pda_run->stack_top = first;

					/* Reattach the data and the parse tree. */
					first->shadow = data_first;

					first = next;
					data_first = data_next;
				}

				/* If there is an parseInput queued, this is one less reduction it has
				 * caused. */
				if ( pda_run->parse_input != 0 )
					pda_run->parse_input->cause_reduce -= 1;

				if ( pda_run->undo_lel->retry_upper != 0 ) {
					/* There is always an parseInput item here because reduce
					 * conflicts only happen on a lookahead character. */
					assert( pda_run->parse_input != pda_run->undo_lel );
					assert( pda_run->parse_input != 0 );
					assert( pda_run->undo_lel->retry_lower == 0 );
					assert( pda_run->parse_input->retry_upper == 0 );

					/* Transfer the retry from undoLel to parseInput. */
					pda_run->parse_input->retry_lower = pda_run->undo_lel->retry_upper;
					pda_run->parse_input->retry_upper = 0;
					pda_run->parse_input->state = stack_top_target( prg, pda_run );
				}

				/* Free the reduced item. */
				colm_tree_downref( prg, sp, pda_run->undo_lel->shadow->tree );
				kid_free( prg, pda_run->undo_lel->shadow );
				parse_tree_free( pda_run, pda_run->undo_lel );

				/* If the stacktop had right ignore attached, detach now. */
				if ( pda_run->stack_top->flags & PF_RIGHT_IL_ATTACHED )
					detach_right_ignore( prg, sp, pda_run, pda_run->stack_top );
			}
		}
		else if ( pda_run->accum_ignore != 0 ) {
			debug( prg, REALM_PARSE, "have accumulated ignore to undo\n" );

			/* Send back any accumulated ignore tokens, then trigger error
			 * in the the parser. */
			parse_tree_t *ignore = pda_run->accum_ignore;
			pda_run->accum_ignore = pda_run->accum_ignore->next;
			ignore->next = 0;

			long region = ignore->retry_region;
			pda_run->next = region > 0 ? region + 1 : 0;
			pda_run->check_next = true;
			pda_run->check_stop = true;
			
			send_back_ignore( prg, sp, pda_run, is, ignore );

			colm_tree_downref( prg, sp, ignore->shadow->tree );
			kid_free( prg, ignore->shadow );
			parse_tree_free( pda_run, ignore );
		}
		else {
			if ( pda_run->shift_count == pda_run->commit_shift_count ) {
				debug( prg, REALM_PARSE, "backed up to commit point, failing parse\n" );
				goto fail;
			}

			pda_run->shift_count -= 1;

			/* Now it is time to undo something. Pick an element from the top of
			 * the stack. */
			pda_run->undo_lel = pda_run->stack_top;

			/* Check if we've arrived at the stack sentinal. This guard is
			 * here to allow us to initially set numRetry to one to cause the
			 * parser to backup all the way to the beginning when an error
			 * occurs. */
			if ( pda_run->undo_lel->next == 0 )
				break;

			/* Either we are dealing with a terminal that was
			 * shifted or a nonterminal that was reduced. */
			if ( pda_run->stack_top->id < prg->rtd->first_non_term_id ) {
				debug( prg, REALM_PARSE, "backing up over effective terminal: %s\n",
							prg->rtd->lel_info[pda_run->stack_top->id].name );

				/* Pop the item from the stack. */
				pda_run->stack_top = pda_run->stack_top->next;

				/* Queue it as next parseInput item. */
				pda_run->undo_lel->next = pda_run->parse_input;
				pda_run->parse_input = pda_run->undo_lel;

				/* Pop from the token list. */
				ref_t *ref = pda_run->token_list;
				pda_run->token_list = ref->next;
				kid_free( prg, (kid_t*)ref );

				assert( pda_run->accum_ignore == 0 );
				detach_left_ignore( prg, sp, pda_run, pda_run->parse_input );
			}
			else {
				debug( prg, REALM_PARSE, "backing up over non-terminal: %s\n",
						prg->rtd->lel_info[pda_run->stack_top->id].name );

				/* Pop the item from the stack. */
				pda_run->stack_top = pda_run->stack_top->next;

				/* Queue it as next parseInput item. */
				pda_run->undo_lel->next = pda_run->parse_input;
				pda_run->parse_input = pda_run->undo_lel;
			}

			/* Undo attach of right ignore. */
			if ( pda_run->stack_top->flags & PF_RIGHT_IL_ATTACHED )
				detach_right_ignore( prg, sp, pda_run, pda_run->stack_top );
		}
	}

fail:
	pda_run->pda_cs = -1;
	pda_run->parse_error = 1;

	/* FIXME: do we still need to fall through here? A fail is permanent now,
	 * no longer called into again. */

	return PCR_DONE;

_out:
	pda_run->next_region_ind = pda_run->pda_tables->token_region_inds[pda_run->pda_cs];

	/* COROUTINE */
	case PCR_DONE:
	break; }

	return PCR_DONE;
}

/*
 * colm_parse_loop
 *
 * Stops on:
 *   PCR_PRE_EOF
 *   PCR_GENERATION
 *   PCR_REDUCTION
 *   PCR_REVERSE
 */

long colm_parse_loop( program_t *prg, tree_t **sp, struct pda_run *pda_run, 
		struct stream_impl *is, long entry )
{
	struct lang_el_info *lel_info = prg->rtd->lel_info;

	/* COROUTINE */
	switch ( entry ) {
	case PCR_START:

	pda_run->stop = false;

	while ( true ) {
		debug( prg, REALM_PARSE, "parse loop start %d:%d\n", is->line, is->column );

		/* Pull the current scanner from the parser. This can change during
		 * parsing due to inputStream pushes, usually for the purpose of includes.
		 * */
		pda_run->token_id = scan_token( prg, pda_run, is );

		if ( pda_run->token_id == SCAN_ERROR ) {
			if ( pda_run->pre_region >= 0 ) {
				pda_run->pre_region = -1;
				pda_run->fsm_cs = pda_run->next_cs;
				continue;
			}
		}

		if ( pda_run->token_id == SCAN_ERROR &&
				( prg->rtd->region_info[pda_run->region].ci_lel_id > 0 ) )
		{
			debug( prg, REALM_PARSE, "sending a collect ignore\n" );
			send_collect_ignore( prg, sp, pda_run, is,
					prg->rtd->region_info[pda_run->region].ci_lel_id );
			goto yes;
		}

		if ( pda_run->token_id == SCAN_TRY_AGAIN_LATER ) {
			debug( prg, REALM_PARSE, "scanner says try again later\n" );
			break;
		}

		assert( pda_run->parse_input == 0 );
		pda_run->parse_input = 0;

		/* Check for EOF. */
		if ( pda_run->token_id == SCAN_EOF ) {
			is->eof_sent = true;
			send_eof( prg, sp, pda_run, is );

			pda_run->frame_id = prg->rtd->region_info[pda_run->region].eof_frame_id;

			if ( prg->ctx_dep_parsing && pda_run->frame_id >= 0 ) {
				debug( prg, REALM_PARSE, "HAVE PRE_EOF BLOCK\n" );

				pda_run->fi = &prg->rtd->frame_info[pda_run->frame_id];
				pda_run->code = pda_run->fi->codeWV;

				/* COROUTINE */
				return PCR_PRE_EOF;
				case PCR_PRE_EOF:

				colm_make_reverse_code( pda_run );
			}
		}
		else if ( pda_run->token_id == SCAN_UNDO ) {
			/* Fall through with parseInput = 0. FIXME: Do we need to send back ignore? */
			debug( prg, REALM_PARSE, "invoking undo from the scanner\n" );
		}
		else if ( pda_run->token_id == SCAN_ERROR ) {
			/* Scanner error, maybe retry. */
			if ( pda_run->accum_ignore == 0 && get_next_region( pda_run, 1 ) != 0 ) {
				debug( prg, REALM_PARSE, "scanner failed, trying next region\n" );

				pda_run->next_region_ind += 1;
				goto skip_send;
			}
			else { // if ( pdaRun->numRetry > 0 ) {
				debug( prg, REALM_PARSE, "invoking parse error from the scanner\n" );

				/* Fall through to send null (error). */
				push_bt_point( prg, pda_run );
			}
#if 0
			else {
				debug( prg, REALM_PARSE, "no alternate scanning regions\n" );

				/* There are no alternative scanning regions to try, nor are
				 * there any alternatives stored in the current parse tree. No
				 * choice but to end the parse. */
				push_bt_point( prg, pda_run );

				report_parse_error( prg, sp, pda_run );
				pda_run->parse_error = 1;
				goto skip_send;
			}
#endif
		}
		else if ( pda_run->token_id == SCAN_LANG_EL ) {
			debug( prg, REALM_PARSE, "sending an named lang el\n" );

			/* A named language element (parsing colm program). */
			prg->rtd->send_named_lang_el( prg, sp, pda_run, is );
		}
		else if ( pda_run->token_id == SCAN_TREE ) {
			debug( prg, REALM_PARSE, "sending a tree\n" );

			/* A tree already built. */
			send_tree( prg, sp, pda_run, is );
		}
		else if ( pda_run->token_id == SCAN_IGNORE ) {
			debug( prg, REALM_PARSE, "sending an ignore token\n" );

			/* A tree to ignore. */
			send_ignore_tree( prg, sp, pda_run, is );
			goto skip_send;
		}
		else if ( prg->ctx_dep_parsing && lel_info[pda_run->token_id].frame_id >= 0 ) {
			/* Has a generation action. */
			debug( prg, REALM_PARSE, "token gen action: %s\n", 
					prg->rtd->lel_info[pda_run->token_id].name );

			/* Make the token data. */
			pda_run->tokdata = peek_match( prg, pda_run, is );

			/* Note that we don't update the position now. It is done when the token
			 * data is pulled from the inputStream. */

			pda_run->p = pda_run->pe = 0;
			pda_run->toklen = 0;
			pda_run->eof = 0;

			pda_run->fi = &prg->rtd->frame_info[prg->rtd->lel_info[pda_run->token_id].frame_id];
			pda_run->frame_id = prg->rtd->lel_info[pda_run->token_id].frame_id;
			pda_run->code = pda_run->fi->codeWV;
			
			/* COROUTINE */
			return PCR_GENERATION;
			case PCR_GENERATION:

			colm_make_reverse_code( pda_run );

			/* Finished with the match text. */
			string_free( prg, pda_run->tokdata );

			goto skip_send;
		}
		else if ( lel_info[pda_run->token_id].ignore ) {
			debug( prg, REALM_PARSE, "sending an ignore token: %s\n", 
					prg->rtd->lel_info[pda_run->token_id].name );

			/* Is an ignore token. */
			send_ignore( prg, sp, pda_run, is, pda_run->token_id );
			goto skip_send;
		}
		else {
			debug( prg, REALM_PARSE, "sending a plain old token: %s\n", 
					prg->rtd->lel_info[pda_run->token_id].name );

			/* Is a plain token. */
			send_token( prg, sp, pda_run, is, pda_run->token_id );
		}
yes:

		if ( pda_run->parse_input != 0 )
			colm_transfer_reverse_code( pda_run, pda_run->parse_input );

		if ( pda_run->parse_input != 0 ) {
			/* If it's a nonterminal with a termdup then flip the parse tree to
			 * the terminal. */
			if ( pda_run->parse_input->id >= prg->rtd->first_non_term_id ) {
				pda_run->parse_input->id =
						prg->rtd->lel_info[pda_run->parse_input->id].term_dup_id;
				pda_run->parse_input->flags |= PF_TERM_DUP;
			}
		}

		long pcr = parse_token( prg, sp, pda_run, is, PCR_START );
		
		while ( pcr != PCR_DONE ) {

			/* COROUTINE */
			return pcr;
			case PCR_REDUCTION:
			case PCR_REVERSE:

			pcr = parse_token( prg, sp, pda_run, is, entry );
		}

		assert( pcr == PCR_DONE );

		handle_error( prg, sp, pda_run );

skip_send:
		new_token( prg, pda_run );

		/* Various stop conditions. This should all be coverned by one test
		 * eventually. */

		if ( pda_run->trigger_undo ) {
			debug( prg, REALM_PARSE, "parsing stopped by triggerUndo\n" );
			break;
		}

		if ( is->eof_sent ) {
			debug( prg, REALM_PARSE, "parsing stopped by EOF\n" );
			break;
		}

		if ( pda_run->stop_parsing ) {
			debug( prg, REALM_PARSE, "scanner has been stopped\n" );
			break;
		}

		if ( pda_run->stop ) {
			debug( prg, REALM_PARSE, "parsing has been stopped by consumedCount\n" );
			break;
		}

		if ( prg->induce_exit ) {
			debug( prg, REALM_PARSE, "parsing has been stopped by a call to exit\n" );
			break;
		}

		if ( pda_run->parse_error ) {
			debug( prg, REALM_PARSE, "parsing stopped by a parse error\n" );
			break;
		}

		/* Disregard any alternate parse paths, just go right to failure. */
		if ( pda_run->fail_parsing ) {
			debug( prg, REALM_PARSE, "parsing failed by explicit request\n" );
			break;
		}
	}

	/* COROUTINE */
	case PCR_DONE:
	break; }

	return PCR_DONE;
}


long colm_parse_frag( program_t *prg, tree_t **sp, struct pda_run *pda_run,
		stream_t *input, long stop_id, long entry )
{
	/* COROUTINE */
	switch ( entry ) {
	case PCR_START:

	if ( ! pda_run->parse_error ) {
		pda_run->stop_target = stop_id;

		long pcr = colm_parse_loop( prg, sp, pda_run, 
				stream_to_impl( input ), entry );

		while ( pcr != PCR_DONE ) {

			/* COROUTINE */
			return pcr;
			case PCR_REDUCTION:
			case PCR_GENERATION:
			case PCR_PRE_EOF:
			case PCR_REVERSE:

			pcr = colm_parse_loop( prg, sp, pda_run, 
					stream_to_impl( input ), entry );
		}
	}

	/* COROUTINE */
	case PCR_DONE:
	break; }

	return PCR_DONE;
}

long colm_parse_finish( tree_t **result, program_t *prg, tree_t **sp,
		struct pda_run *pda_run, stream_t *input , int revert_on, long entry )
{
	struct stream_impl *si;

	/* COROUTINE */
	switch ( entry ) {
	case PCR_START:

	if ( pda_run->stop_target <= 0 ) {
		si = stream_to_impl( input );
		si->funcs->set_eof( si );

		if ( ! pda_run->parse_error ) {
			si = stream_to_impl( input );
			long pcr = colm_parse_loop( prg, sp, pda_run, si, entry );

			while ( pcr != PCR_DONE ) {

				/* COROUTINE */
				return pcr;
				case PCR_REDUCTION:
				case PCR_GENERATION:
				case PCR_PRE_EOF:
				case PCR_REVERSE:

				si = stream_to_impl( input );
				pcr = colm_parse_loop( prg, sp, pda_run, si, entry );
			}
		}
	}

	/* FIXME: need something here to check that we are not stopped waiting for
	 * more data when we are actually expected to finish. This check doesn't
	 * work (at time of writing). */
	//assert( (pdaRun->stopTarget > 0 && pdaRun->stopParsing) || 
	//		streamToImpl( input )->eofSent );

	/* Flush out anything not committed. */
	if ( pda_run->reducer )
		commit_reduce( prg, sp, pda_run );
	
	/* What to do here.
	 * if ( pda_run->fail_parsing )
	 *   goto fail; */

	if ( !revert_on )
		colm_rcode_downref_all( prg, sp, &pda_run->reverse_code );
	
	tree_t *tree = get_parsed_root( pda_run, pda_run->stop_target > 0 );

	if ( pda_run->reducer ) {
		*result = 0;
	}
	else {
		colm_tree_upref( tree );
		*result = tree;
	}


	/* COROUTINE */
	case PCR_DONE:
	break; }

	return PCR_DONE;
}

long colm_parse_undo_frag( program_t *prg, tree_t **sp, struct pda_run *pda_run,
		stream_t *input, long steps, long entry )
{
	debug( prg, REALM_PARSE,
			"undo parse frag, target steps: %ld, pdarun steps: %ld\n",
			steps, pda_run->steps );

	reset_token( pda_run );

	/* COROUTINE */
	switch ( entry ) {
	case PCR_START:

	if ( steps < pda_run->steps ) {
		/* Setup environment for going backwards until we reduced steps to
		 * what we want. */
		pda_run->num_retry += 1;
		pda_run->target_steps = steps;
		pda_run->trigger_undo = 1;

		/* The parse loop will recognise the situation. */
		long pcr = colm_parse_loop( prg, sp, pda_run, stream_to_impl(input), entry );
		while ( pcr != PCR_DONE ) {

			/* COROUTINE */
			return pcr;
			case PCR_REDUCTION:
			case PCR_GENERATION:
			case PCR_PRE_EOF:
			case PCR_REVERSE:

			pcr = colm_parse_loop( prg, sp, pda_run, stream_to_impl(input), entry );
		}

		/* Reset environment. */
		pda_run->trigger_undo = 0;
		pda_run->target_steps = -1;
		pda_run->num_retry -= 1;
	}

	/* COROUTINE */
	case PCR_DONE:
	break; }

	return PCR_DONE;
}

