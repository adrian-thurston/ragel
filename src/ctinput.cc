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

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>

#include <iostream>

#include "compiler.h"
#include "pool.h"
//#include "debug.h"

using std::cerr;
using std::endl;

DEF_INPUT_FUNCS( input_funcs_ct, input_impl_ct );

extern input_funcs_ct pat_funcs;
extern input_funcs_ct repl_funcs;

struct input_impl_ct
{
	struct input_funcs *funcs;

	char *name;
	long line;
	long column;
	long byte;

	struct Pattern *pattern;
	struct PatternItem *pat_item;
	struct Constructor *constructor;
	struct ConsItem *cons_item;

	char eof_mark;
	char eof_sent;

	int offset;
};

void ct_destructor( program_t *prg, tree_t **sp, struct input_impl_ct *ss )
{
}

char ct_get_eof_sent( struct colm_program *prg, struct input_impl_ct *si )
{
	return si->eof_sent;
}

void ct_set_eof_sent( struct colm_program *prg, struct input_impl_ct *si, char eof_sent )
{
	si->eof_sent = eof_sent;
}

/*
 * Pattern
 */

struct input_impl *colm_impl_new_pat( char *name, Pattern *pattern )
{
	struct input_impl_ct *ss = (struct input_impl_ct*)malloc(sizeof(struct input_impl_ct));
	memset( ss, 0, sizeof(struct input_impl_ct) );
	ss->pattern = pattern;
	ss->pat_item = pattern->list->head;
	ss->funcs = (struct input_funcs*)&pat_funcs;
	return (struct input_impl*) ss;
}

int pat_get_parse_block( struct colm_program *prg, struct input_impl_ct *ss, int *pskip,
		char **pdp, int *copied )
{ 
	*copied = 0;

	PatternItem *buf = ss->pat_item;
	int offset = ss->offset;

	while ( true ) {
		if ( buf == 0 )
			return INPUT_EOF;

		if ( buf->form == PatternItem::TypeRefForm )
			return INPUT_LANG_EL;

		assert ( buf->form == PatternItem::InputTextForm );
		int avail = buf->data.length() - offset;

		if ( avail > 0 ) {
			/* The source data from the current buffer. */
			char *src = &buf->data[offset];
			int slen = avail;

			/* Need to skip? */
			if ( *pskip > 0 && slen <= *pskip ) {
				/* Skipping the the whole source. */
				*pskip -= slen;
			}
			else {
				/* Either skip is zero, or less than slen. Skip goes to zero.
				 * Some data left over, copy it. */
				src += *pskip;
				slen -= *pskip;
				*pskip = 0;

				*pdp = src;
				*copied += slen;
				break;
			}
		}

		buf = buf->next;
		offset = 0;
	}

	return INPUT_DATA;
}


int pat_get_data( struct colm_program *prg, struct input_impl_ct *ss, char *dest, int length )
{ 
	int copied = 0;

	PatternItem *buf = ss->pat_item;
	int offset = ss->offset;

	while ( true ) {
		if ( buf == 0 )
			break;

		if ( buf->form == PatternItem::TypeRefForm )
			break;

		assert ( buf->form == PatternItem::InputTextForm );
		int avail = buf->data.length() - offset;

		if ( avail > 0 ) {
			/* The source data from the current buffer. */
			char *src = &buf->data[offset];
			int slen = avail <= length ? avail : length;

			memcpy( dest+copied, src, slen ) ;
			copied += slen;
			length -= slen;
		}

		if ( length == 0 )
			break;

		buf = buf->next;
		offset = 0;
	}

	return copied;
}

void pat_backup( struct input_impl_ct *ss )
{
	if ( ss->pat_item == 0 )
		ss->pat_item = ss->pattern->list->tail;
	else
		ss->pat_item = ss->pat_item->prev;
}

int pat_consume_data( struct colm_program *prg, struct input_impl_ct *ss, int length, location_t *loc )
{
	//debug( REALM_INPUT, "consuming %ld bytes\n", length );

	int consumed = 0;

	while ( true ) {
		if ( ss->pat_item == 0 )
			break;

		int avail = ss->pat_item->data.length() - ss->offset;

		if ( length >= avail ) {
			/* Read up to the end of the data. Advance the
			 * pattern item. */
			ss->pat_item = ss->pat_item->next;
			ss->offset = 0;

			length -= avail;
			consumed += avail;

			if ( length == 0 )
				break;
		}
		else {
			ss->offset += length;
			consumed += length;
			break;
		}
	}

	return consumed;
}

int pat_undo_consume_data( struct colm_program *prg, struct input_impl_ct *ss, const char *data, int length )
{
	ss->offset -= length;
	return length;
}

LangEl *pat_consume_lang_el( struct colm_program *prg, struct input_impl_ct *ss, long *bindId,
		char **data, long *length )
{ 
	LangEl *klangEl = ss->pat_item->prodEl->langEl;
	*bindId = ss->pat_item->bindId;
	*data = 0;
	*length = 0;

	ss->pat_item = ss->pat_item->next;
	ss->offset = 0;
	return klangEl;
}

void pat_undo_consume_lang_el( struct colm_program *prg, struct input_impl_ct *ss )
{
	pat_backup( ss );
	ss->offset = ss->pat_item->data.length();
}

void ct_set_eof_mark( struct colm_program *prg, struct input_impl_ct *si, char eof_mark )
{
	si->eof_mark = eof_mark;
}

void ct_transfer_loc_seq( struct colm_program *prg, location_t *loc, struct input_impl_ct *ss )
{
	loc->name = ss->name;
	loc->line = ss->line;
	loc->column = ss->column;
	loc->byte = ss->byte;
}

input_funcs_ct pat_funcs = 
{
	&pat_get_parse_block,
	&pat_get_data,

	&pat_consume_data,
	&pat_undo_consume_data,

	0, /* consume_tree */
	0, /* undo_consume_tree */

	&pat_consume_lang_el,
	&pat_undo_consume_lang_el,
	
	0, 0, 0, 0, 0, 0, /* prepend funcs. */
	0, 0, 0, 0, 0, 0, /* append funcs */

	&ct_set_eof_mark,

	&ct_transfer_loc_seq,
	&ct_destructor,
};


/*
 * Replacements
 */

struct input_impl *colm_impl_new_cons( char *name, Constructor *constructor )
{
	struct input_impl_ct *ss = (struct input_impl_ct*)malloc(sizeof(struct input_impl_ct));
	memset( ss, 0, sizeof(struct input_impl_ct) );
	ss->constructor = constructor;
	ss->cons_item = constructor->list->head;
	ss->funcs = (struct input_funcs*)&repl_funcs;
	return (struct input_impl*)ss;
}

LangEl *repl_consume_lang_el( struct colm_program *prg, struct input_impl_ct *ss, long *bindId, char **data, long *length )
{ 
	LangEl *klangEl = ss->cons_item->type == ConsItem::ExprType ? 
			ss->cons_item->langEl : ss->cons_item->prodEl->langEl;
	*bindId = ss->cons_item->bindId;

	*data = 0;
	*length = 0;

	if ( ss->cons_item->type == ConsItem::LiteralType ) {
		if ( ss->cons_item->prodEl->typeRef->pdaLiteral != 0 ) {
			bool unusedCI;
			prepareLitString( ss->cons_item->data, unusedCI, 
					ss->cons_item->prodEl->typeRef->pdaLiteral->data,
					ss->cons_item->prodEl->typeRef->pdaLiteral->loc );

			*data = ss->cons_item->data;
			*length = ss->cons_item->data.length();
		}
	}

	ss->cons_item = ss->cons_item->next;
	ss->offset = 0;
	return klangEl;
}

int repl_get_parse_block( struct colm_program *prg, struct input_impl_ct *ss,
		int *pskip, char **pdp, int *copied )
{ 
	*copied = 0;

	ConsItem *buf = ss->cons_item;
	int offset = ss->offset;

	while ( true ) {
		if ( buf == 0 )
			return INPUT_EOF;

		if ( buf->type == ConsItem::ExprType || buf->type == ConsItem::LiteralType )
			return INPUT_LANG_EL;

		assert ( buf->type == ConsItem::InputText );
		int avail = buf->data.length() - offset;

		if ( avail > 0 ) {
			/* The source data from the current buffer. */
			char *src = &buf->data[offset];
			int slen = avail;

			/* Need to skip? */
			if ( *pskip > 0 && slen <= *pskip ) {
				/* Skipping the the whole source. */
				*pskip -= slen;
			}
			else {
				/* Either skip is zero, or less than slen. Skip goes to zero.
				 * Some data left over, copy it. */
				src += *pskip;
				slen -= *pskip;
				*pskip = 0;

				*pdp = src;
				*copied += slen;
				break;
			}
		}

		buf = buf->next;
		offset = 0;
	}

	return INPUT_DATA;
}

int repl_get_data( struct colm_program *prg, struct input_impl_ct *ss, char *dest, int length )
{ 
	int copied = 0;

	ConsItem *buf = ss->cons_item;
	int offset = ss->offset;

	while ( true ) {
		if ( buf == 0 )
			break;

		if ( buf->type == ConsItem::ExprType || buf->type == ConsItem::LiteralType )
			break;

		assert ( buf->type == ConsItem::InputText );
		int avail = buf->data.length() - offset;

		if ( avail > 0 ) {
			/* The source data from the current buffer. */
			char *src = &buf->data[offset];
			int slen = avail <= length ? avail : length;

			memcpy( dest+copied, src, slen ) ;
			copied += slen;
			length -= slen;
		}

		if ( length == 0 )
			break;

		buf = buf->next;
		offset = 0;
	}

	return copied;
}

void repl_backup( struct input_impl_ct *ss )
{
	if ( ss->cons_item == 0 )
		ss->cons_item = ss->constructor->list->tail;
	else
		ss->cons_item = ss->cons_item->prev;
}

void repl_undo_consume_lang_el( struct colm_program *prg, struct input_impl_ct *ss )
{
	repl_backup( ss );
	ss->offset = ss->cons_item->data.length();
}


int repl_consume_data( struct colm_program *prg, struct input_impl_ct *ss, int length, location_t *loc )
{
	int consumed = 0;

	while ( true ) {
		if ( ss->cons_item == 0 )
			break;

		int avail = ss->cons_item->data.length() - ss->offset;

		if ( length >= avail ) {
			/* Read up to the end of the data. Advance the
			 * pattern item. */
			ss->cons_item = ss->cons_item->next;
			ss->offset = 0;

			length -= avail;
			consumed += avail;

			if ( length == 0 )
				break;
		}
		else {
			ss->offset += length;
			consumed += length;
			break;
		}
	}

	return consumed;
}

int repl_undo_consume_data( struct colm_program *prg, struct input_impl_ct *ss, const char *data, int length )
{
	int origLen = length;
	while ( true ) {
		int avail = ss->offset;

		/* Okay to go up to the front of the buffer. */
		if ( length > avail ) {
			ss->cons_item= ss->cons_item->prev;
			ss->offset = ss->cons_item->data.length();
			length -= avail;
		}
		else {
			ss->offset -= length;
			break;
		}
	}

	return origLen;
}

input_funcs_ct repl_funcs =
{
	&repl_get_parse_block,
	&repl_get_data,

	&repl_consume_data,
	&repl_undo_consume_data,

	0, /* consume_tree */
	0, /* undo_consume_tree. */

	&repl_consume_lang_el,
	&repl_undo_consume_lang_el,

	0, 0, 0, 0, 0, 0, /* prepend. */
	0, 0, 0, 0, 0, 0, /* append. */

	&ct_set_eof_mark,

	&ct_transfer_loc_seq,
	&ct_destructor,
};

void pushBinding( pda_run *pdaRun, parse_tree_t *parseTree )
{
	/* If the item is bound then store it in the bindings array. */
	pdaRun->bindings->push( parseTree );
}

extern "C" void internalSendNamedLangEl( program_t *prg, tree_t **sp,
		struct pda_run *pdaRun, struct input_impl *is )
{
	/* All three set by consumeLangEl. */
	long bindId;
	char *data;
	long length;

	LangEl *klangEl = is->funcs->consume_lang_el( prg, is, &bindId, &data, &length );
	
	//cerr << "named langEl: " << prg->rtd->lelInfo[klangEl->id].name << endl;

	/* Copy the token data. */
	head_t *tokdata = 0;
	if ( data != 0 )
		tokdata = string_alloc_full( prg, data, length );

	kid_t *input = make_token_with_data( prg, pdaRun, is, klangEl->id, tokdata );

	colm_increment_steps( pdaRun );

	parse_tree_t *parseTree = parse_tree_allocate( pdaRun );
	parseTree->id = input->tree->id;
	parseTree->flags |= PF_NAMED;
	parseTree->shadow = input;

	if ( bindId > 0 )
		pushBinding( pdaRun, parseTree );
	
	pdaRun->parse_input = parseTree;
}

extern "C" void internalInitBindings( pda_run *pdaRun )
{
	/* Bindings are indexed at 1. Need a no-binding. */
	pdaRun->bindings = new bindings;
	pdaRun->bindings->push(0);
}

extern "C" void internalPopBinding( pda_run *pdaRun, parse_tree_t *parseTree )
{
	parse_tree_t *lastBound = pdaRun->bindings->top();
	if ( lastBound == parseTree )
		pdaRun->bindings->pop();
}
