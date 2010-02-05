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

/*
 * Pattern
 */

InputStreamPattern::InputStreamPattern( Pattern *pattern )
: 
	InputStream(true),
	pattern(pattern),
	patItem(pattern->list->head),
	offset(0),
	flush(false)
{}

bool InputStreamPattern::isLangEl()
{ 
	return patItem != 0 && patItem->type == PatternItem::FactorType;
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


int InputStreamPattern::getData( char *dest, int length )
{ 
	if ( offset == 0 )
		line = patItem->loc.line;

	assert ( patItem->type == PatternItem::InputText );
	int available = patItem->data.length() - offset;

	if ( available < length )
		length = available;

	memcpy( dest, patItem->data.data+offset, length );
	offset += length;

	if ( offset == patItem->data.length() ) {
		/* Read up to the end of the data. Advance the
		 * pattern item. */
		patItem = patItem->next;
		offset = 0;
		flush = shouldFlush();
	}
	else {
		/* There is more data in this buffer. Don't flush. */
		flush = false;
	}
	return length;
}

int InputStreamPattern::isEOF()
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
	char *data = runBuf->buf + runBuf->offset;
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


/*
 * Replacement
 */

InputStreamRepl::InputStreamRepl( Replacement *replacement )
: 
	InputStream(true),
	replacement(replacement),
	replItem(replacement->list->head),
	offset(0),
	flush(false)
{}

bool InputStreamRepl::isLangEl()
{ 
	return replItem != 0 && ( replItem->type == ReplItem::ExprType || 
			replItem->type == ReplItem::FactorType );
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

int InputStreamRepl::getData( char *dest, int length )
{ 
	if ( offset == 0 )
		line = replItem->loc.line;

	assert ( replItem->type == ReplItem::InputText );
	int available = replItem->data.length() - offset;

	if ( available < length )
		length = available;

	memcpy( dest, replItem->data.data+offset, length );
	offset += length;

	if ( offset == replItem->data.length() ) {
		/* Read up to the end of the data. Advance the
		 * replacement item. */
		replItem = replItem->next;
		offset = 0;
		flush = shouldFlush();
	}
	else {
		/* There is more data in this buffer. Don't flush. */
		flush = false;
	}
	return length;
}

int InputStreamRepl::isEOF()
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
	char *data = runBuf->buf + runBuf->offset;
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

void send_named_lang_el( Tree **sp, PdaRun *pdaRun, FsmRun *fsmRun, InputStream *inputStream )
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
		tokdata = string_alloc_full( fsmRun->prg, data, length );

	Kid *input = make_token( pdaRun, fsmRun, inputStream, klangEl->id, tokdata, true, bindId );

	pdaRun->consumed += 1;

	send_handle_error( sp, pdaRun, fsmRun, inputStream, input );
}

