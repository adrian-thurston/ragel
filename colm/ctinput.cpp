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


bool InputStreamStatic::tryAgainLater()
{
	if ( later )
		return true;

	return false;
}

void initStaticFuncs()
{
	memcpy( &staticFuncs, &baseFuncs, sizeof(InputFuncs) );
	staticFuncs.isTree = &inputStreamStaticIsTree;
	staticFuncs.isIgnore = &inputStreamStaticIsIgnore;
}


/*
 * Pattern
 */

InputStreamPattern::InputStreamPattern( Pattern *pattern )
: 
	InputStreamStatic(true),
	pattern(pattern),
	patItem(pattern->list->head),
	offset(0)
{
	funcs = &patternFuncs;
}

bool inputStreamPatternIsLangEl( InputStream *_is )
{ 
	InputStreamPattern *is = (InputStreamPattern*) _is;
	return is->patItem != 0 && is->patItem->type == PatternItem::FactorType;
}

int InputStreamPattern::shouldFlush()
{ 
	return patItem == 0 || patItem->type == PatternItem::FactorType;
}

KlangEl *InputStreamPattern::getLangEl( long &bindId, char *&data, long &length )
{ 
	KlangEl *klangEl = patItem->factor->langEl;
	bindId = patItem->bindId;
	data = 0;
	length = 0;
	line = patItem->loc.line;

	patItem = patItem->next;
	offset = 0;
	flush = false;
	return klangEl;
}

int inputStreamPatternGetData( InputStream *_is, char *dest, int length )
{ 
	InputStreamPattern *is = (InputStreamPattern*)_is;
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
		is->flush = is->shouldFlush();
	}
	else {
		/* There is more data in this buffer. Don't flush. */
		is->flush = false;
	}
	return length;
}

int InputStreamPattern::isEof()
{
	return patItem == 0;
}

int InputStreamPattern::needFlush()
{
	return flush;
}

void InputStreamPattern::backup()
{
	if ( patItem == 0 )
		patItem = pattern->list->tail;
	else
		patItem = patItem->prev;
}

void InputStreamPattern::pushBackBuf( RunBuf *runBuf )
{
	char *data = runBuf->data + runBuf->offset;
	long length = runBuf->length;

	if ( length == 0 )
		return;

	/* While pushing back past the current pattern item start. */
	while ( length > offset ) {
		length -= offset;
		if ( offset > 0 )
			assert( memcmp( patItem->data, data-length, offset ) == 0 );
		backup();
		offset = patItem->data.length();
	}

	offset -= length;
	assert( memcmp( &patItem->data[offset], data, length ) == 0 );
}

void InputStreamPattern::pushBackNamed()
{
	backup();
	offset = patItem->data.length();
}


void initPatternFuncs()
{
	memcpy( &patternFuncs, &staticFuncs, sizeof(InputFuncs) );
	patternFuncs.getData = &inputStreamPatternGetData;
	patternFuncs.isLangEl = &inputStreamPatternIsLangEl;
}


/*
 * Replacement
 */

InputStreamRepl::InputStreamRepl( Replacement *replacement )
: 
	InputStreamStatic(true),
	replacement(replacement),
	replItem(replacement->list->head),
	offset(0)
{
	funcs = &replFuncs;
}

bool inputStreamReplIsLangEl( InputStream *_is )
{ 
	InputStreamRepl *is = (InputStreamRepl*)_is;
	return is->replItem != 0 && ( is->replItem->type == ReplItem::ExprType || 
			is->replItem->type == ReplItem::FactorType );
}

int InputStreamRepl::shouldFlush()
{ 
	return replItem == 0 || ( replItem->type == ReplItem::ExprType ||
			replItem->type == ReplItem::FactorType );
}

KlangEl *InputStreamRepl::getLangEl( long &bindId, char *&data, long &length )
{ 
	KlangEl *klangEl = replItem->type == ReplItem::ExprType ? 
			replItem->langEl : replItem->factor->langEl;
	bindId = replItem->bindId;

	data = 0;
	length = 0;
	line = replItem->loc.line;

	if ( replItem->type == ReplItem::FactorType ) {
		if ( replItem->factor->literal != 0 ) {
			bool unusedCI;
			prepareLitString( replItem->data, unusedCI, 
					replItem->factor->literal->token.data,
					replItem->factor->literal->token.loc );

			data = replItem->data;
			length = replItem->data.length();
		}
	}

	replItem = replItem->next;
	offset = 0;
	flush = false;
	return klangEl;
}

int inputStreamReplGetData( InputStream *_is, char *dest, int length )
{ 
	InputStreamRepl *is = (InputStreamRepl*)_is;

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
		is->flush = is->shouldFlush();
	}
	else {
		/* There is more data in this buffer. Don't flush. */
		is->flush = false;
	}
	return length;
}

int InputStreamRepl::isEof()
{
	return replItem == 0;
}

int InputStreamRepl::needFlush()
{
	return flush;
}

void InputStreamRepl::backup()
{
	if ( replItem == 0 )
		replItem = replacement->list->tail;
	else
		replItem = replItem->prev;
}

void InputStreamRepl::pushBackBuf( RunBuf *runBuf )
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
	while ( length > offset ) {
		length -= offset;
		if ( offset > 0 ) 
			assert( memcmp( replItem->data, data-length, offset ) == 0 );
		backup();
		offset = replItem->data.length();
	}

	offset -= length;
	assert( memcmp( &replItem->data[offset], data, length ) == 0 );
}

void InputStreamRepl::pushBackNamed()
{
	backup();
	offset = replItem->data.length();
}

void sendNamedLangEl( Tree **sp, PdaRun *pdaRun, FsmRun *fsmRun, InputStream *inputStream )
{
	/* All three set by getLangEl. */
	long bindId;
	char *data;
	long length;

	KlangEl *klangEl = inputStream->getLangEl( bindId, data, length );
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


void initReplFuncs()
{
	memcpy( &replFuncs, &staticFuncs, sizeof(InputFuncs) );
	replFuncs.getData = &inputStreamReplGetData;
	replFuncs.isLangEl = &inputStreamReplIsLangEl;
}
