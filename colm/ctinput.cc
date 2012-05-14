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

SourceStream *newSourceStreamPattern( Pattern *pattern )
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

extern "C" void initPatternFuncs()
{
	memset( &patternFuncs, 0, sizeof(SourceFuncs) );

	patternFuncs.getData = &inputStreamPatternGetData;
	patternFuncs.consumeData = &inputStreamPatternConsumeData;
	patternFuncs.undoConsumeData = &inputStreamPatternUndoConsumeData;

	patternFuncs.consumeLangEl = &inputStreamPatternGetLangEl;
	patternFuncs.undoConsumeLangEl = &inputStreamPatternUndoConsumeLangEl;
}


/*
 * Replacement
 */

SourceStream *newSourceStreamRepl( Replacement *replacement )
{
	SourceStream *is = (SourceStream*)malloc(sizeof(SourceStream));
	memset( is, 0, sizeof(SourceStream) );
	is->handlesLine = true;
	is->replacement = replacement;
	is->replItem = replacement->list->head;
	is->funcs = &replFuncs;
	return is;
}

LangEl *inputStreamReplGetLangEl( SourceStream *is, long *bindId, char **data, long *length )
{ 
	LangEl *klangEl = is->replItem->type == ReplItem::ExprType ? 
			is->replItem->langEl : is->replItem->factor->langEl;
	*bindId = is->replItem->bindId;

	*data = 0;
	*length = 0;
	is->line = is->replItem->loc.line;

	if ( is->replItem->type == ReplItem::FactorType ) {
		if ( is->replItem->factor->typeRef->pdaLiteral != 0 ) {
			bool unusedCI;
			prepareLitString( is->replItem->data, unusedCI, 
					is->replItem->factor->typeRef->pdaLiteral->token.data,
					is->replItem->factor->typeRef->pdaLiteral->token.loc );

			*data = is->replItem->data;
			*length = is->replItem->data.length();
		}
	}

	is->replItem = is->replItem->next;
	is->offset = 0;
	return klangEl;
}

int inputStreamReplGetData( SourceStream *is, int skip, char *dest, int length, int *copied )
{ 
	*copied = 0;

	ReplItem *buf = is->replItem;
	int offset = is->offset;

	while ( true ) {
		if ( buf == 0 )
			return INPUT_EOD;

		if ( buf->type == ReplItem::ExprType || buf->type == ReplItem::FactorType )
			return INPUT_LANG_EL;

		if ( offset == 0 )
			is->line = buf->loc.line;

		assert ( buf->type == ReplItem::InputText );
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

void inputStreamReplBackup( SourceStream *is )
{
	if ( is->replItem == 0 )
		is->replItem = is->replacement->list->tail;
	else
		is->replItem = is->replItem->prev;
}

void inputStreamReplPushBackBuf( SourceStream *is, RunBuf *runBuf )
{
	char *data = runBuf->data + runBuf->offset;
	long length = runBuf->length;

	if ( colm_log_parse ) {
		cerr << "push back data: ";
		cerr.write( data, length );
		cerr << endl;
	}

	if ( length == 0 )
		return;

	/* While pushing back past the current pattern item start. */
	while ( length > is->offset ) {
		length -= is->offset;
		if ( is->offset > 0 ) 
			assert( memcmp( is->replItem->data, data-length, is->offset ) == 0 );
		inputStreamReplBackup( is );
		is->offset = is->replItem->data.length();
	}

	is->offset -= length;
	assert( memcmp( &is->replItem->data[is->offset], data, length ) == 0 );
}

void inputStreamReplUndoConsumeLangEl( SourceStream *is )
{
	inputStreamReplBackup( is );
	is->offset = is->replItem->data.length();
}

int inputStreamReplConsumeData( SourceStream *is, int length )
{
	int consumed = 0;

	while ( true ) {
		if ( is->replItem == 0 )
			break;

		int avail = is->replItem->data.length() - is->offset;

		if ( length >= avail ) {
			/* Read up to the end of the data. Advance the
			 * pattern item. */
			is->replItem = is->replItem->next;
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

int inputStreamReplUndoConsumeData( SourceStream *is, const char *data, int length )
{
	is->offset -= length;
	return length;
}

extern "C" void initReplFuncs()
{
	memset( &replFuncs, 0, sizeof(SourceFuncs) );

	replFuncs.getData = &inputStreamReplGetData;
	replFuncs.consumeData = &inputStreamReplConsumeData;
	replFuncs.undoConsumeData = &inputStreamReplUndoConsumeData;

	replFuncs.consumeLangEl = &inputStreamReplGetLangEl;
	replFuncs.undoConsumeLangEl = &inputStreamReplUndoConsumeLangEl;
}

void sendNamedLangEl( Program *prg, Tree **sp, PdaRun *pdaRun, FsmRun *fsmRun, InputStream *inputStream )
{
	/* All three set by consumeLangEl. */
	long bindId;
	char *data;
	long length;

	LangEl *klangEl = consumeLangEl( inputStream, &bindId, &data, &length );
	if ( klangEl->termDup != 0 )
		klangEl = klangEl->termDup;
	
	#ifdef COLM_LOG_PARSE
	if ( colm_log_parse ) {
		cerr << "named langEl: " << prg->rtd->lelInfo[klangEl->id].name << endl;
	}
	#endif

	/* Copy the token data. */
	Head *tokdata = 0;
	if ( data != 0 )
		tokdata = stringAllocFull( prg, data, length );

	Kid *input = makeTokenWithData( prg, pdaRun, fsmRun, inputStream, klangEl->id, tokdata, true, bindId );

	incrementSteps( pdaRun );

	ParseTree *parseTree = parseTreeAllocate( prg );
	parseTree->id = input->tree->id;
	parseTree->flags = input->tree->flags;
	parseTree->flags &= ~(
		AF_LEFT_IGNORE | AF_LEFT_IL_ATTACHED | 
		AF_RIGHT_IGNORE | AF_RIGHT_IL_ATTACHED
	);
	parseTree->flags |= AF_PARSE_TREE;
	parseTree->refs = 1;
	parseTree->prodNum = input->tree->prodNum;
	parseTree->shadow = input;
	
	pdaRun->parseInput = kid2Allocate( prg );
	pdaRun->parseInput->tree = parseTree;
}

void initBindings( PdaRun *pdaRun )
{
	/* Bindings are indexed at 1. Need a no-binding. */
	pdaRun->bindings = new Bindings;
	pdaRun->bindings->push(0);
}

void makeTokenPushBinding( PdaRun *pdaRun, int bindId, Tree *tree )
{
	/* If the item is bound then store it in the bindings array. */
	if ( bindId > 0 ) {
		pdaRun->bindings->push( tree );
		treeUpref( tree );
	}
}

void unbind( Program *prg, Tree **sp, PdaRun *pdaRun, Tree *tree )
{
	Tree *lastBound = pdaRun->bindings->top();
	if ( lastBound == tree ) {
		pdaRun->bindings->pop();
		treeDownref( prg, sp, tree );
	}
}

