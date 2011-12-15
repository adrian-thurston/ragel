/*
 *  Copyright 2007-2011 Adrian Thurston <thurston@complang.org>
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

#include <iostream>
using std::cerr;
using std::endl;

SourceFuncs staticFuncs;
SourceFuncs patternFuncs;
SourceFuncs replFuncs;

/* 
 * Implementation
 */

int inputStreamStaticIsTree( SourceStream *is )
{
	return false;
}

int inputStreamStaticIsIgnore( SourceStream *is )
{
	return false;
}

int inputStreamStaticTryAgainLater( SourceStream *is )
{
	if ( is->later )
		return true;

	return false;
}

extern "C" void initStaticFuncs()
{
	memcpy( &staticFuncs, &baseFuncs, sizeof(SourceFuncs) );
	staticFuncs.isTree = &inputStreamStaticIsTree;
	staticFuncs.isIgnore = &inputStreamStaticIsIgnore;
	staticFuncs.tryAgainLater = &inputStreamStaticTryAgainLater;
}


/*
 * Pattern
 */

SourceStream *newInputStreamPattern( Pattern *pattern )
{
	SourceStream *is = (SourceStream*)malloc(sizeof(SourceStream));
	memset( is, 0, sizeof(SourceStream) );
	is->handlesLine = true;
	is->pattern = pattern;
	is->patItem = pattern->list->head;
	is->funcs = &patternFuncs;
	return is;
}

int inputStreamPatternIsLangEl( SourceStream *is )
{ 
	return is->patItem != 0 && is->patItem->type == PatternItem::FactorType;
}

int inputStreamPatternShouldFlush( SourceStream *is )
{ 
	return is->patItem == 0 || is->patItem->type == PatternItem::FactorType;
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
	is->flush = false;
	return klangEl;
}

int inputStreamPatternGetData( SourceStream *is, char *dest, int length )
{ 
	if ( is->offset == 0 )
		is->line = is->patItem->loc.line;

	assert ( is->patItem->type == PatternItem::InputText );
	int available = is->patItem->data.length() - is->offset;

	if ( available < length )
		length = available;

	memcpy( dest, is->patItem->data.data + is->offset, length );
	is->offset += length;

	if ( is->offset == is->patItem->data.length() ) {
		/* Read up to the end of the data. Advance the
		 * pattern item. */
		is->patItem = is->patItem->next;
		is->offset = 0;
		is->flush = inputStreamPatternShouldFlush( is );
	}
	else {
		/* There is more data in this buffer. Don't flush. */
		is->flush = false;
	}
	return length;
}

int inputStreamPatternIsEof( SourceStream *is )
{
	return is->patItem == 0;
}

int inputStreamPatternNeedFlush( SourceStream *is )
{
	return is->flush;
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

void inputStreamPatternPushBackNamed( SourceStream *is )
{
	inputStreamPatternBackup( is );
	is->offset = is->patItem->data.length();
}


extern "C" void initPatternFuncs()
{
	memcpy( &patternFuncs, &staticFuncs, sizeof(SourceFuncs) );
	patternFuncs.getData = &inputStreamPatternGetData;
	patternFuncs.isLangEl = &inputStreamPatternIsLangEl;
	patternFuncs.isEof = &inputStreamPatternIsEof;
	patternFuncs.needFlush = &inputStreamPatternNeedFlush;
	patternFuncs.getLangEl = &inputStreamPatternGetLangEl;
	patternFuncs.pushBackBuf = &inputStreamPatternPushBackBuf;
	patternFuncs.pushBackNamed = &inputStreamPatternPushBackNamed;
}


/*
 * Replacement
 */

SourceStream *newInputStreamRepl( Replacement *replacement )
{
	SourceStream *is = (SourceStream*)malloc(sizeof(SourceStream));
	memset( is, 0, sizeof(SourceStream) );
	is->handlesLine = true;
	is->replacement = replacement;
	is->replItem = replacement->list->head;
	is->funcs = &replFuncs;
	return is;
}

int inputStreamReplIsLangEl( SourceStream *is )
{ 
	return is->replItem != 0 && ( is->replItem->type == ReplItem::ExprType || 
			is->replItem->type == ReplItem::FactorType );
}

int inputStreamReplShouldFlush( SourceStream *is )
{ 
	return is->replItem == 0 || ( is->replItem->type == ReplItem::ExprType ||
			is->replItem->type == ReplItem::FactorType );
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
	is->flush = false;
	return klangEl;
}

int inputStreamReplGetData( SourceStream *is, char *dest, int length )
{ 
	if ( is->offset == 0 )
		is->line = is->replItem->loc.line;

	assert ( is->replItem->type == ReplItem::InputText );
	int available = is->replItem->data.length() - is->offset;

	if ( available < length )
		length = available;

	memcpy( dest, is->replItem->data.data+is->offset, length );
	is->offset += length;

	if ( is->offset == is->replItem->data.length() ) {
		/* Read up to the end of the data. Advance the
		 * replacement item. */
		is->replItem = is->replItem->next;
		is->offset = 0;
		is->flush = inputStreamReplShouldFlush( is );
	}
	else {
		/* There is more data in this buffer. Don't flush. */
		is->flush = false;
	}
	return length;
}

int inputStreamReplIsEof( SourceStream *is )
{
	return is->replItem == 0;
}

int inputStreamReplNeedFlush( SourceStream *is )
{
	return is->flush;
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

void inputStreamReplPushBackNamed( SourceStream *is )
{
	inputStreamReplBackup( is );
	is->offset = is->replItem->data.length();
}

extern "C" void initReplFuncs()
{
	memcpy( &replFuncs, &staticFuncs, sizeof(SourceFuncs) );
	replFuncs.getData = &inputStreamReplGetData;
	replFuncs.isLangEl = &inputStreamReplIsLangEl;
	replFuncs.isEof = &inputStreamReplIsEof;
	replFuncs.needFlush = &inputStreamReplNeedFlush;
	replFuncs.getLangEl = &inputStreamReplGetLangEl;
	replFuncs.pushBackBuf = &inputStreamReplPushBackBuf;
	replFuncs.pushBackNamed = &inputStreamReplPushBackNamed;
}

Kid *sendNamedLangEl( Program *prg, Tree **sp, PdaRun *pdaRun, FsmRun *fsmRun, SourceStream *inputStream )
{
	/* All three set by getLangEl. */
	long bindId;
	char *data;
	long length;

	LangEl *klangEl = inputStream->funcs->getLangEl( inputStream, &bindId, &data, &length );
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

	return input;
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

