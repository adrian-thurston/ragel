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

#include "parsedata.h"
#include "parsetree.h"
#include "input.h"
#include "debug.h"
#include "pool.h"

#include <iostream>

using std::cerr;
using std::endl;

extern StreamFuncs patternFuncs;
extern StreamFuncs replFuncs;

/*
 * Pattern
 */

StreamImpl *newSourceStreamPat( const char *name, Pattern *pattern )
{
	StreamImpl *ss = (StreamImpl*)malloc(sizeof(StreamImpl));
	memset( ss, 0, sizeof(StreamImpl) );
	ss->pattern = pattern;
	ss->patItem = pattern->list->head;
	ss->funcs = &patternFuncs;
	return ss;
}

LangEl *inputStreamPatternGetLangEl( StreamImpl *ss, long *bindId, char **data, long *length )
{ 
	LangEl *klangEl = ss->patItem->prodEl->langEl;
	*bindId = ss->patItem->bindId;
	*data = 0;
	*length = 0;

	ss->patItem = ss->patItem->next;
	ss->offset = 0;
	return klangEl;
}

int inputStreamPatternGetParseBlock( StreamImpl *ss, int skip, char **pdp, int *copied )
{ 
	*copied = 0;

	PatternItem *buf = ss->patItem;
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

int inputStreamPatternGetData( StreamImpl *ss, char *dest, int length )
{ 
	int copied = 0;

	PatternItem *buf = ss->patItem;
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

void inputStreamPatternBackup( StreamImpl *ss )
{
	if ( ss->patItem == 0 )
		ss->patItem = ss->pattern->list->tail;
	else
		ss->patItem = ss->patItem->prev;
}

void inputStreamPatternPushBackBuf( StreamImpl *ss, RunBuf *runBuf )
{
	char *data = runBuf->data + runBuf->offset;
	long length = runBuf->length;

	if ( length == 0 )
		return;

	/* While pushing back past the current pattern item start. */
	while ( length > ss->offset ) {
		length -= ss->offset;
		if ( ss->offset > 0 )
			assert( memcmp( ss->patItem->data, data-length, ss->offset ) == 0 );
		inputStreamPatternBackup( ss );
		ss->offset = ss->patItem->data.length();
	}

	ss->offset -= length;
	assert( memcmp( &ss->patItem->data[ss->offset], data, length ) == 0 );
}

void inputStreamPatternUndoConsumeLangEl( StreamImpl *ss )
{
	inputStreamPatternBackup( ss );
	ss->offset = ss->patItem->data.length();
}

int inputStreamPatternConsumeData( StreamImpl *ss, int length, Location *loc )
{
	//debug( REALM_INPUT, "consuming %ld bytes\n", length );

	int consumed = 0;

	while ( true ) {
		if ( ss->patItem == 0 )
			break;

		int avail = ss->patItem->data.length() - ss->offset;

		if ( length >= avail ) {
			/* Read up to the end of the data. Advance the
			 * pattern item. */
			ss->patItem = ss->patItem->next;
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

int inputStreamPatternUndoConsumeData( StreamImpl *ss, const char *data, int length )
{
	ss->offset -= length;
	return length;
}

StreamFuncs patternFuncs = 
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

StreamImpl *newSourceStreamCons( const char *name, Constructor *constructor )
{
	StreamImpl *ss = (StreamImpl*)malloc(sizeof(StreamImpl));
	memset( ss, 0, sizeof(StreamImpl) );
	ss->constructor = constructor;
	ss->consItem = constructor->list->head;
	ss->funcs = &replFuncs;
	return ss;
}

LangEl *inputStreamConsGetLangEl( StreamImpl *ss, long *bindId, char **data, long *length )
{ 
	LangEl *klangEl = ss->consItem->type == ConsItem::ExprType ? 
			ss->consItem->langEl : ss->consItem->prodEl->langEl;
	*bindId = ss->consItem->bindId;

	*data = 0;
	*length = 0;

	if ( ss->consItem->type == ConsItem::LiteralType ) {
		if ( ss->consItem->prodEl->typeRef->pdaLiteral != 0 ) {
			bool unusedCI;
			prepareLitString( ss->consItem->data, unusedCI, 
					ss->consItem->prodEl->typeRef->pdaLiteral->data,
					ss->consItem->prodEl->typeRef->pdaLiteral->loc );

			*data = ss->consItem->data;
			*length = ss->consItem->data.length();
		}
	}

	ss->consItem = ss->consItem->next;
	ss->offset = 0;
	return klangEl;
}

int inputStreamConsGetParseBlock( StreamImpl *ss,
		int skip, char **pdp, int *copied )
{ 
	*copied = 0;

	ConsItem *buf = ss->consItem;
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

int inputStreamConsGetData( StreamImpl *ss, char *dest, int length )
{ 
	int copied = 0;

	ConsItem *buf = ss->consItem;
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

void inputStreamConsBackup( StreamImpl *ss )
{
	if ( ss->consItem == 0 )
		ss->consItem = ss->constructor->list->tail;
	else
		ss->consItem = ss->consItem->prev;
}

void inputStreamConsPushBackBuf( StreamImpl *ss, RunBuf *runBuf )
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
			assert( memcmp( ss->consItem->data, data-length, ss->offset ) == 0 );
		inputStreamConsBackup( ss );
		ss->offset = ss->consItem->data.length();
	}

	ss->offset -= length;
	assert( memcmp( &ss->consItem->data[ss->offset], data, length ) == 0 );
}

void inputStreamConsUndoConsumeLangEl( StreamImpl *ss )
{
	inputStreamConsBackup( ss );
	ss->offset = ss->consItem->data.length();
}

int inputStreamConsConsumeData( StreamImpl *ss, int length, Location *loc )
{
	int consumed = 0;

	while ( true ) {
		if ( ss->consItem == 0 )
			break;

		int avail = ss->consItem->data.length() - ss->offset;

		if ( length >= avail ) {
			/* Read up to the end of the data. Advance the
			 * pattern item. */
			ss->consItem = ss->consItem->next;
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

int inputStreamConsUndoConsumeData( StreamImpl *ss, const char *data, int length )
{
	ss->offset -= length;
	return length;
}

StreamFuncs replFuncs =
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

extern "C" void internalSendNamedLangEl( Program *prg, Tree **sp, PdaRun *pdaRun, FsmRun *fsmRun, StreamImpl *is )
{
	/* All three set by consumeLangEl. */
	long bindId;
	char *data;
	long length;

	LangEl *klangEl = is->funcs->consumeLangEl( is, &bindId, &data, &length );
	
	//cerr << "named langEl: " << prg->rtd->lelInfo[klangEl->id].name << endl;

	/* Copy the token data. */
	Head *tokdata = 0;
	if ( data != 0 )
		tokdata = stringAllocFull( prg, data, length );

	Kid *input = makeTokenWithData( prg, pdaRun, fsmRun, is, klangEl->id, tokdata );

	incrementSteps( pdaRun );

	ParseTree *parseTree = parseTreeAllocate( prg );
	parseTree->id = input->tree->id;
	parseTree->flags |= PF_NAMED;
	parseTree->shadow = input;

	if ( bindId > 0 )
		pushBinding( pdaRun, parseTree );
	
	pdaRun->parseInput = parseTree;
}

extern "C" void internalInitBindings( PdaRun *pdaRun )
{
	/* Bindings are indexed at 1. Need a no-binding. */
	pdaRun->bindings = new Bindings;
	pdaRun->bindings->push(0);
}

void pushBinding( PdaRun *pdaRun, ParseTree *parseTree )
{
	/* If the item is bound then store it in the bindings array. */
	pdaRun->bindings->push( parseTree );
}

extern "C" void internalPopBinding( PdaRun *pdaRun, ParseTree *parseTree )
{
	ParseTree *lastBound = pdaRun->bindings->top();
	if ( lastBound == parseTree )
		pdaRun->bindings->pop();
}
