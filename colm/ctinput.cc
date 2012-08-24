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
#include "fsmrun.h"
#include "debug.h"
#include "pool.h"

#include <iostream>

using std::cerr;
using std::endl;

SourceFuncs patternFuncs;
SourceFuncs replFuncs;

/*
 * Pattern
 */

SourceStream *newSourceStreamPat( Pattern *pattern )
{
	SourceStream *is = (SourceStream*)malloc(sizeof(SourceStream));
	memset( is, 0, sizeof(SourceStream) );
	is->handlesLine = true;
	is->pattern = pattern;
	is->patItem = pattern->list->head;
	is->funcs = &patternFuncs;
	return is;
}

LangEl *inputStreamPatternGetLangEl( SourceStream *is, long *bindId, char **data, long *length )
{ 
	LangEl *klangEl = is->patItem->factor->langEl;
	*bindId = is->patItem->bindId;
	*data = 0;
	*length = 0;
	is->line = is->patItem->loc.line;

	is->patItem = is->patItem->next;
	is->offset = 0;
	return klangEl;
}

int inputStreamPatternGetData( SourceStream *is, int skip, char *dest, int length, int *copied )
{ 
	*copied = 0;

	PatternItem *buf = is->patItem;
	int offset = is->offset;

	while ( true ) {
		if ( buf == 0 )
			return INPUT_EOD;

		if ( buf->type == PatternItem::FactorType )
			return INPUT_LANG_EL;

		if ( offset == 0 )
			is->line = buf->loc.line;

		assert ( buf->type == PatternItem::InputText );
		int avail = buf->data.length() - offset;

		if ( avail > 0 ) {
			/* The source data from the current buffer. */
			char *src = &buf->data[offset];
			int slen = avail <= length ? avail : length;

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

				memcpy( dest, src, slen ) ;
				*copied += slen;
				break;
			}
		}

		buf = buf->next;
		offset = 0;
	}

	return INPUT_DATA;
}

void inputStreamPatternBackup( SourceStream *is )
{
	if ( is->patItem == 0 )
		is->patItem = is->pattern->list->tail;
	else
		is->patItem = is->patItem->prev;
}

void inputStreamPatternPushBackBuf( SourceStream *is, RunBuf *runBuf )
{
	char *data = runBuf->data + runBuf->offset;
	long length = runBuf->length;

	if ( length == 0 )
		return;

	/* While pushing back past the current pattern item start. */
	while ( length > is->offset ) {
		length -= is->offset;
		if ( is->offset > 0 )
			assert( memcmp( is->patItem->data, data-length, is->offset ) == 0 );
		inputStreamPatternBackup( is );
		is->offset = is->patItem->data.length();
	}

	is->offset -= length;
	assert( memcmp( &is->patItem->data[is->offset], data, length ) == 0 );
}

void inputStreamPatternUndoConsumeLangEl( SourceStream *is )
{
	inputStreamPatternBackup( is );
	is->offset = is->patItem->data.length();
}

int inputStreamPatternConsumeData( SourceStream *is, int length )
{
	debug( REALM_INPUT, "consuming %ld bytes\n", length );

	int consumed = 0;

	while ( true ) {
		if ( is->patItem == 0 )
			break;

		int avail = is->patItem->data.length() - is->offset;

		if ( length >= avail ) {
			/* Read up to the end of the data. Advance the
			 * pattern item. */
			is->patItem = is->patItem->next;
			is->offset = 0;

			length -= avail;
			consumed += avail;

			if ( length == 0 )
				break;
		}
		else {
			is->offset += length;
			consumed += length;
			break;
		}
	}

	return consumed;
}

int inputStreamPatternUndoConsumeData( SourceStream *is, const char *data, int length )
{
	is->offset -= length;
	return length;
}

extern "C" void initPatFuncs()
{
	memset( &patternFuncs, 0, sizeof(SourceFuncs) );

	patternFuncs.getData = &inputStreamPatternGetData;
	patternFuncs.consumeData = &inputStreamPatternConsumeData;
	patternFuncs.undoConsumeData = &inputStreamPatternUndoConsumeData;

	patternFuncs.consumeLangEl = &inputStreamPatternGetLangEl;
	patternFuncs.undoConsumeLangEl = &inputStreamPatternUndoConsumeLangEl;
}


/*
 * Constructor
 */

SourceStream *newSourceStreamCons( Constructor *constructor )
{
	SourceStream *is = (SourceStream*)malloc(sizeof(SourceStream));
	memset( is, 0, sizeof(SourceStream) );
	is->handlesLine = true;
	is->constructor = constructor;
	is->consItem = constructor->list->head;
	is->funcs = &replFuncs;
	return is;
}

LangEl *inputStreamConsGetLangEl( SourceStream *is, long *bindId, char **data, long *length )
{ 
	LangEl *klangEl = is->consItem->type == ConsItem::ExprType ? 
			is->consItem->langEl : is->consItem->factor->langEl;
	*bindId = is->consItem->bindId;

	*data = 0;
	*length = 0;
	is->line = is->consItem->loc.line;

	if ( is->consItem->type == ConsItem::FactorType ) {
		if ( is->consItem->factor->typeRef->pdaLiteral != 0 ) {
			bool unusedCI;
			prepareLitString( is->consItem->data, unusedCI, 
					is->consItem->factor->typeRef->pdaLiteral->data,
					is->consItem->factor->typeRef->pdaLiteral->loc );

			*data = is->consItem->data;
			*length = is->consItem->data.length();
		}
	}

	is->consItem = is->consItem->next;
	is->offset = 0;
	return klangEl;
}

int inputStreamConsGetData( SourceStream *is, int skip, char *dest, int length, int *copied )
{ 
	*copied = 0;

	ConsItem *buf = is->consItem;
	int offset = is->offset;

	while ( true ) {
		if ( buf == 0 )
			return INPUT_EOD;

		if ( buf->type == ConsItem::ExprType || buf->type == ConsItem::FactorType )
			return INPUT_LANG_EL;

		if ( offset == 0 )
			is->line = buf->loc.line;

		assert ( buf->type == ConsItem::InputText );
		int avail = buf->data.length() - offset;

		if ( avail > 0 ) {
			/* The source data from the current buffer. */
			char *src = &buf->data[offset];
			int slen = avail <= length ? avail : length;

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

				memcpy( dest, src, slen ) ;
				*copied += slen;
				break;
			}
		}

		buf = buf->next;
		offset = 0;
	}

	return INPUT_DATA;
}

void inputStreamConsBackup( SourceStream *is )
{
	if ( is->consItem == 0 )
		is->consItem = is->constructor->list->tail;
	else
		is->consItem = is->consItem->prev;
}

void inputStreamConsPushBackBuf( SourceStream *is, RunBuf *runBuf )
{
	char *data = runBuf->data + runBuf->offset;
	long length = runBuf->length;

	//cerr << "push back data: ";
	//cerr.write( data, length );
	//cerr << endl;

	if ( length == 0 )
		return;

	/* While pushing back past the current pattern item start. */
	while ( length > is->offset ) {
		length -= is->offset;
		if ( is->offset > 0 ) 
			assert( memcmp( is->consItem->data, data-length, is->offset ) == 0 );
		inputStreamConsBackup( is );
		is->offset = is->consItem->data.length();
	}

	is->offset -= length;
	assert( memcmp( &is->consItem->data[is->offset], data, length ) == 0 );
}

void inputStreamConsUndoConsumeLangEl( SourceStream *is )
{
	inputStreamConsBackup( is );
	is->offset = is->consItem->data.length();
}

int inputStreamConsConsumeData( SourceStream *is, int length )
{
	int consumed = 0;

	while ( true ) {
		if ( is->consItem == 0 )
			break;

		int avail = is->consItem->data.length() - is->offset;

		if ( length >= avail ) {
			/* Read up to the end of the data. Advance the
			 * pattern item. */
			is->consItem = is->consItem->next;
			is->offset = 0;

			length -= avail;
			consumed += avail;

			if ( length == 0 )
				break;
		}
		else {
			is->offset += length;
			consumed += length;
			break;
		}
	}

	return consumed;
}

int inputStreamConsUndoConsumeData( SourceStream *is, const char *data, int length )
{
	is->offset -= length;
	return length;
}

extern "C" void initConsFuncs()
{
	memset( &replFuncs, 0, sizeof(SourceFuncs) );

	replFuncs.getData = &inputStreamConsGetData;
	replFuncs.consumeData = &inputStreamConsConsumeData;
	replFuncs.undoConsumeData = &inputStreamConsUndoConsumeData;

	replFuncs.consumeLangEl = &inputStreamConsGetLangEl;
	replFuncs.undoConsumeLangEl = &inputStreamConsUndoConsumeLangEl;
}

void sendNamedLangEl( Program *prg, Tree **sp, PdaRun *pdaRun, FsmRun *fsmRun, InputStream *inputStream )
{
	/* All three set by consumeLangEl. */
	long bindId;
	char *data;
	long length;

	LangEl *klangEl = consumeLangEl( inputStream, &bindId, &data, &length );
	
	//cerr << "named langEl: " << prg->rtd->lelInfo[klangEl->id].name << endl;

	/* Copy the token data. */
	Head *tokdata = 0;
	if ( data != 0 )
		tokdata = stringAllocFull( prg, data, length );

	Kid *input = makeTokenWithData( prg, pdaRun, fsmRun, inputStream, klangEl->id, tokdata );

	incrementSteps( pdaRun );

	ParseTree *parseTree = parseTreeAllocate( prg );
	parseTree->id = input->tree->id;
	parseTree->flags |= PF_NAMED;
	parseTree->shadow = input;

	if ( bindId > 0 )
		pushBinding( pdaRun, parseTree );
	
	pdaRun->parseInput = parseTree;
}

void initBindings( PdaRun *pdaRun )
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

void popBinding( PdaRun *pdaRun, ParseTree *parseTree )
{
	ParseTree *lastBound = pdaRun->bindings->top();
	if ( lastBound == parseTree )
		pdaRun->bindings->pop();
}
