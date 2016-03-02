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

#include "compiler.h"
#include "parsetree.h"
#include "input.h"
#include "debug.h"
#include "pool.h"
#include "pdacodegen.h"

#include <iostream>

using std::cerr;
using std::endl;

extern stream_funcs patternFuncs;
extern stream_funcs replFuncs;

/*
 * Pattern
 */

struct stream_impl *colm_impl_new_pat( char *name, Pattern *pattern )
{
	struct stream_impl *ss = (struct stream_impl*)malloc(sizeof(struct stream_impl));
	memset( ss, 0, sizeof(struct stream_impl) );
	ss->pattern = pattern;
	ss->pat_item = pattern->list->head;
	ss->funcs = &patternFuncs;
	return ss;
}

LangEl *inputStreamPatternGetLangEl( struct stream_impl *ss, long *bindId,
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

int inputStreamPatternGetParseBlock( struct stream_impl *ss, int skip,
		char **pdp, int *copied )
{ 
	*copied = 0;

	PatternItem *buf = ss->pat_item;
	int offset = ss->offset;

	while ( true ) {
		if ( buf == 0 )
			return INPUT_EOD;

		if ( buf->form == PatternItem::TypeRefForm )
			return INPUT_LANG_EL;

		assert ( buf->form == PatternItem::InputTextForm );
		int avail = buf->data.length() - offset;

		if ( avail > 0 ) {
			/* The source data from the current buffer. */
			char *src = &buf->data[offset];
			int slen = avail;

			/* Need to skip? */
			if ( skip > 0 && slen <= skip ) {
				/* Skipping the the whole source. */
				skip -= slen;
			}
			else {
				/* Either skip is zero, or less than slen. Skip goes to zero.
				 * Some data left over, copy it. */
				src += skip;
				slen -= skip;
				skip = 0;

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

int inputStreamPatternGetData( struct stream_impl *ss, char *dest, int length )
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

void inputStreamPatternBackup( struct stream_impl *ss )
{
	if ( ss->pat_item == 0 )
		ss->pat_item = ss->pattern->list->tail;
	else
		ss->pat_item = ss->pat_item->prev;
}

void inputStreamPatternPushBackBuf( struct stream_impl *ss, struct run_buf *runBuf )
{
	char *data = runBuf->data + runBuf->offset;
	long length = runBuf->length;

	if ( length == 0 )
		return;

	/* While pushing back past the current pattern item start. */
	while ( length > ss->offset ) {
		length -= ss->offset;
		if ( ss->offset > 0 )
			assert( memcmp( ss->pat_item->data, data-length, ss->offset ) == 0 );
		inputStreamPatternBackup( ss );
		ss->offset = ss->pat_item->data.length();
	}

	ss->offset -= length;
	assert( memcmp( &ss->pat_item->data[ss->offset], data, length ) == 0 );
}

void inputStreamPatternUndoConsumeLangEl( struct stream_impl *ss )
{
	inputStreamPatternBackup( ss );
	ss->offset = ss->pat_item->data.length();
}

int inputStreamPatternConsumeData( program_t *prg, tree_t **sp,
		struct stream_impl *ss, int length, location_t *loc )
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

int inputStreamPatternUndoConsumeData( struct stream_impl *ss, const char *data, int length )
{
	ss->offset -= length;
	return length;
}

stream_funcs patternFuncs = 
{
	&inputStreamPatternGetParseBlock,
	&inputStreamPatternGetData,
	&inputStreamPatternConsumeData,
	&inputStreamPatternUndoConsumeData,
	0,
	0,
	&inputStreamPatternGetLangEl,
	&inputStreamPatternUndoConsumeLangEl,
};


/*
 * Constructor
 */

struct stream_impl *colm_impl_new_cons( char *name, Constructor *constructor )
{
	struct stream_impl *ss = (struct stream_impl*)malloc(sizeof(struct stream_impl));
	memset( ss, 0, sizeof(struct stream_impl) );
	ss->constructor = constructor;
	ss->cons_item = constructor->list->head;
	ss->funcs = &replFuncs;
	return ss;
}

LangEl *inputStreamConsGetLangEl( struct stream_impl *ss, long *bindId, char **data, long *length )
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

int inputStreamConsGetParseBlock( struct stream_impl *ss,
		int skip, char **pdp, int *copied )
{ 
	*copied = 0;

	ConsItem *buf = ss->cons_item;
	int offset = ss->offset;

	while ( true ) {
		if ( buf == 0 )
			return INPUT_EOD;

		if ( buf->type == ConsItem::ExprType || buf->type == ConsItem::LiteralType )
			return INPUT_LANG_EL;

		assert ( buf->type == ConsItem::InputText );
		int avail = buf->data.length() - offset;

		if ( avail > 0 ) {
			/* The source data from the current buffer. */
			char *src = &buf->data[offset];
			int slen = avail;

			/* Need to skip? */
			if ( skip > 0 && slen <= skip ) {
				/* Skipping the the whole source. */
				skip -= slen;
			}
			else {
				/* Either skip is zero, or less than slen. Skip goes to zero.
				 * Some data left over, copy it. */
				src += skip;
				slen -= skip;
				skip = 0;

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

int inputStreamConsGetData( struct stream_impl *ss, char *dest, int length )
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

void inputStreamConsBackup( struct stream_impl *ss )
{
	if ( ss->cons_item == 0 )
		ss->cons_item = ss->constructor->list->tail;
	else
		ss->cons_item = ss->cons_item->prev;
}

void inputStreamConsPushBackBuf( struct stream_impl *ss, struct run_buf *runBuf )
{
	char *data = runBuf->data + runBuf->offset;
	long length = runBuf->length;

	//cerr << "push back data: ";
	//cerr.write( data, length );
	//cerr << endl;

	if ( length == 0 )
		return;

	/* While pushing back past the current pattern item start. */
	while ( length > ss->offset ) {
		length -= ss->offset;
		if ( ss->offset > 0 ) 
			assert( memcmp( ss->cons_item->data, data-length, ss->offset ) == 0 );
		inputStreamConsBackup( ss );
		ss->offset = ss->cons_item->data.length();
	}

	ss->offset -= length;
	assert( memcmp( &ss->cons_item->data[ss->offset], data, length ) == 0 );
}

void inputStreamConsUndoConsumeLangEl( struct stream_impl *ss )
{
	inputStreamConsBackup( ss );
	ss->offset = ss->cons_item->data.length();
}

int inputStreamConsConsumeData( program_t *prg, tree_t **sp,
		struct stream_impl *ss, int length, location_t *loc )
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

int inputStreamConsUndoConsumeData( struct stream_impl *ss, const char *data, int length )
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

stream_funcs replFuncs =
{
	&inputStreamConsGetParseBlock,
	&inputStreamConsGetData,
	&inputStreamConsConsumeData,
	&inputStreamConsUndoConsumeData,
	0,
	0,
	&inputStreamConsGetLangEl,
	&inputStreamConsUndoConsumeLangEl,
};

void pushBinding( pda_run *pdaRun, parse_tree_t *parseTree )
{
	/* If the item is bound then store it in the bindings array. */
	pdaRun->bindings->push( parseTree );
}

extern "C" void internalSendNamedLangEl( program_t *prg, tree_t **sp,
		struct pda_run *pdaRun, struct stream_impl *is )
{
	/* All three set by consumeLangEl. */
	long bindId;
	char *data;
	long length;

	LangEl *klangEl = is->funcs->consume_lang_el( is, &bindId, &data, &length );
	
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
