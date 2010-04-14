/*
 *  Copyright 2007-2009 Adrian Thurston <thurston@complang.org>
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

InputFuncs staticFuncs;
InputFuncs patternFuncs;
InputFuncs replFuncs;

/* 
 * Implementation
 */

bool inputStreamStaticIsTree( InputStream *is )
{
	return false;
}

bool inputStreamStaticIsIgnore( InputStream *is )
{
	return false;
}


bool inputStreamStaticTryAgainLater( InputStream *is )
{
	if ( is->later )
		return true;

	return false;
}

void initStaticFuncs()
{
	memcpy( &staticFuncs, &baseFuncs, sizeof(InputFuncs) );
	staticFuncs.isTree = &inputStreamStaticIsTree;
	staticFuncs.isIgnore = &inputStreamStaticIsIgnore;
	staticFuncs.tryAgainLater = &inputStreamStaticTryAgainLater;
}


/*
 * Pattern
 */

InputStream *newInputStreamPattern( Pattern *pattern )
{
	InputStream *is = (InputStream*)malloc(sizeof(InputStream));
	memset( is, 0, sizeof(InputStream) );
	is->handlesLine = true;
	is->pattern = pattern;
	is->patItem = pattern->list->head;
	is->funcs = &patternFuncs;
	return is;
}

bool inputStreamPatternIsLangEl( InputStream *is )
{ 
	return is->patItem != 0 && is->patItem->type == PatternItem::FactorType;
}

int inputStreamPatternShouldFlush( InputStream *is )
{ 
	return is->patItem == 0 || is->patItem->type == PatternItem::FactorType;
}

KlangEl *inputStreamPatternGetLangEl( InputStream *is, long &bindId, char *&data, long &length )
{ 
	KlangEl *klangEl = is->patItem->factor->langEl;
	bindId = is->patItem->bindId;
	data = 0;
	length = 0;
	is->line = is->patItem->loc.line;

	is->patItem = is->patItem->next;
	is->offset = 0;
	is->flush = false;
	return klangEl;
}

int inputStreamPatternGetData( InputStream *is, char *dest, int length )
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

bool inputStreamPatternIsEof( InputStream *is )
{
	return is->patItem == 0;
}

int inputStreamPatternNeedFlush( InputStream *is )
{
	return is->flush;
}

void inputStreamPatternBackup( InputStream *is )
{
	if ( is->patItem == 0 )
		is->patItem = is->pattern->list->tail;
	else
		is->patItem = is->patItem->prev;
}

void inputStreamPatternPushBackBuf( InputStream *is, RunBuf *runBuf )
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

void inputStreamPatternPushBackNamed( InputStream *is )
{
	inputStreamPatternBackup( is );
	is->offset = is->patItem->data.length();
}


void initPatternFuncs()
{
	memcpy( &patternFuncs, &staticFuncs, sizeof(InputFuncs) );
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

InputStream *newInputStreamRepl( Replacement *replacement )
{
	InputStream *is = (InputStream*)malloc(sizeof(InputStream));
	memset( is, 0, sizeof(InputStream) );
	is->handlesLine = true;
	is->replacement = replacement;
	is->replItem = replacement->list->head;
	is->funcs = &replFuncs;
	return is;
}

bool inputStreamReplIsLangEl( InputStream *is )
{ 
	return is->replItem != 0 && ( is->replItem->type == ReplItem::ExprType || 
			is->replItem->type == ReplItem::FactorType );
}

int inputStreamReplShouldFlush( InputStream *is )
{ 
	return is->replItem == 0 || ( is->replItem->type == ReplItem::ExprType ||
			is->replItem->type == ReplItem::FactorType );
}

KlangEl *inputStreamReplGetLangEl( InputStream *is, long &bindId, char *&data, long &length )
{ 
	KlangEl *klangEl = is->replItem->type == ReplItem::ExprType ? 
			is->replItem->langEl : is->replItem->factor->langEl;
	bindId = is->replItem->bindId;

	data = 0;
	length = 0;
	is->line = is->replItem->loc.line;

	if ( is->replItem->type == ReplItem::FactorType ) {
		if ( is->replItem->factor->literal != 0 ) {
			bool unusedCI;
			prepareLitString( is->replItem->data, unusedCI, 
					is->replItem->factor->literal->token.data,
					is->replItem->factor->literal->token.loc );

			data = is->replItem->data;
			length = is->replItem->data.length();
		}
	}

	is->replItem = is->replItem->next;
	is->offset = 0;
	is->flush = false;
	return klangEl;
}

int inputStreamReplGetData( InputStream *is, char *dest, int length )
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

bool inputStreamReplIsEof( InputStream *is )
{
	return is->replItem == 0;
}

int inputStreamReplNeedFlush( InputStream *is )
{
	return is->flush;
}

void inputStreamReplBackup( InputStream *is )
{
	if ( is->replItem == 0 )
		is->replItem = is->replacement->list->tail;
	else
		is->replItem = is->replItem->prev;
}

void inputStreamReplPushBackBuf( InputStream *is, RunBuf *runBuf )
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

void inputStreamReplPushBackNamed( InputStream *is )
{
	inputStreamReplBackup( is );
	is->offset = is->replItem->data.length();
}

void initReplFuncs()
{
	memcpy( &replFuncs, &staticFuncs, sizeof(InputFuncs) );
	replFuncs.getData = &inputStreamReplGetData;
	replFuncs.isLangEl = &inputStreamReplIsLangEl;
	replFuncs.isEof = &inputStreamReplIsEof;
	replFuncs.needFlush = &inputStreamReplNeedFlush;
	replFuncs.getLangEl = &inputStreamReplGetLangEl;
	replFuncs.pushBackBuf = &inputStreamReplPushBackBuf;
	replFuncs.pushBackNamed = &inputStreamReplPushBackNamed;
}

void sendNamedLangEl( Tree **sp, PdaRun *pdaRun, FsmRun *fsmRun, InputStream *inputStream )
{
	/* All three set by getLangEl. */
	long bindId;
	char *data;
	long length;

	KlangEl *klangEl = inputStream->funcs->getLangEl( inputStream, bindId, data, length );
	if ( klangEl->termDup != 0 )
		klangEl = klangEl->termDup;
	
	#ifdef COLM_LOG_PARSE
	if ( colm_log_parse ) {
		cerr << "named langEl: " << pdaRun->tables->rtd->lelInfo[klangEl->id].name << endl;
	}
	#endif

	/* Copy the token data. */
	Head *tokdata = 0;
	if ( data != 0 )
		tokdata = stringAllocFull( fsmRun->prg, data, length );

	Kid *input = makeToken( pdaRun, fsmRun, inputStream, klangEl->id, tokdata, true, bindId );

	pdaRun->consumed += 1;

	sendHandleError( sp, pdaRun, fsmRun, inputStream, input );
}

