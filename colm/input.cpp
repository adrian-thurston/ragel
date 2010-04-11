/*
 *  Copyright 2007, 2008 Adrian Thurston <thurston@complang.org>
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

#include "input.h"
#include "fsmrun.h"
#include <stdio.h>
#include <iostream>

using std::cerr;
using std::endl;

InputFuncs InputStream::baseFuncs =
{
	0
};

/* 
 * Base run-time input streams.
 */

int inputStreamDynamicGetData( InputStream *is, char *dest, int length )
{
	return 0;
}

InputFuncs InputStreamDynamic::dynamicFuncs =
{
	&inputStreamDynamicGetData
};

int InputStreamDynamic::getData( char *dest, int length )
{
	/* If there is any data in the rubuf queue then read that first. */
	if ( head() != 0 ) {
		long avail = head()->length - head()->offset;
		if ( length >= avail ) {
			memcpy( dest, &head()->data[head()->offset], avail );
			RunBuf *del = popHead();
			delete del;
			return avail;
		}
		else {
			memcpy( dest, &head()->data[head()->offset], length );
			head()->offset += length;
			return length;
		}
	}
	else {
		/* No stored data, call the impl version. */
		return getDataImpl( dest, length );
	}
}

int InputStreamDynamic::getDataRev( char *dest, int length )
{
	/* If there is any data in the rubuf queue then read that first. */
	if ( tail() != 0 ) {
		long avail = tail()->length - tail()->offset;
		if ( length >= avail ) {
			memcpy( dest, &head()->data[head()->offset], avail );
			RunBuf *del = popTail();
			delete del;
			return avail;
		}
		else {
			memcpy( dest, &head()->data[head()->offset], length );
			head()->offset += length;
			return length;
		}
	}
	return 0;
}

int InputStreamDynamic::isEof()
{
	return head() == 0 && eof;
}

void InputStreamDynamic::pushBackBuf( RunBuf *runBuf )
{
	pushBackBufImpl( runBuf );
}

void InputStreamDynamic::append( const char *data, long len )
{
	return appendImpl( data, len );
}

void InputStreamDynamic::append( Tree *tree )
{
	return appendImpl( tree );
}

bool InputStreamDynamic::tryAgainLater()
{
	if ( later )
		return true;

	return false;
}

bool InputStreamDynamic::isTree()
{
	if ( head() != 0 && head()->type == RunBuf::TokenType )
		return true;
	return false;
}

Tree *InputStreamDynamic::getTree()
{
	if ( head() != 0 && head()->type == RunBuf::TokenType ) {
		RunBuf *runBuf = popHead();

		/* FIXME: using runbufs here for this is a poor use of memory. */
		Tree *tree = runBuf->tree;
		delete runBuf;
		return tree;
	}

	return 0;
}

bool InputStreamDynamic::isIgnore()
{
	if ( head() != 0 && head()->type == RunBuf::IgnoreType )
		return true;
	return false;
}

KlangEl *InputStreamDynamic::getLangEl( long &bindId, char *&data, long &length )
{
	return getLangElImpl( bindId, data, length );
}

void InputStreamDynamic::pushBackNamed()
{
	return pushBackNamedImpl();
}

Tree *InputStreamDynamic::undoPush( int length )
{
	if ( head()->type == RunBuf::DataType ) {
		char tmp[length];
		int have = 0;
		while ( have < length ) {
			int res = getData( tmp, length-have );
			have += res;
		}
		return 0;
	}
	else {
		/* FIXME: leak here. */
		RunBuf *rb = popHead();
		Tree *tree = rb->tree;
		delete rb;
		return tree;
	}
}

Tree *InputStreamDynamic::undoAppend( int length )
{
	if ( tail()->type == RunBuf::DataType ) {
		char tmp[length];
		int have = 0;
		while ( have < length ) {
			int res = getDataRev( tmp, length-have );
			have += res;
		}
		return 0;
	}
	else {
		/* FIXME: leak here. */
		RunBuf *rb = popTail();
		Tree *tree = rb->tree;
		delete rb;
		return tree;
	}
}

/*
 * String
 */

int InputStreamString::getDataImpl( char *dest, int length )
{ 
	int available = dlen - offset;

	if ( available < length )
		length = available;

	memcpy( dest, data+offset, length );
	offset += length;

	if ( offset == dlen )
		//eof = true;
		later = true;

	return length;
}

void InputStreamString::pushBackBufImpl( RunBuf *runBuf )
{
	//char *data = runBuf->buf + runBuf->offset;
	long length = runBuf->length;

	assert( length <= offset );
	offset -= length;
}

/*
 * File
 */

int InputStreamFile::needFlush()
{
	return head() == 0 && feof( file );
}

int InputStreamFile::getDataImpl( char *dest, int length )
{
	size_t res = fread( dest, 1, length, file );
	if ( res < (size_t) length )
		later = true;
	return res;
}

void InputStreamFile::pushBackBufImpl( RunBuf *runBuf )
{
	prepend( runBuf );
}

/*
 * FD
 */

int InputStreamFd::needFlush()
{
	return head() == 0 && eof;
}

void InputStreamFd::pushBackBufImpl( RunBuf *runBuf )
{
	prepend( runBuf );
}

int InputStreamFd::getDataImpl( char *dest, int length )
{
	long got = read( fd, dest, length );
	if ( got == 0 )
		later = true;
	return got;
}

/*
 * Accum
 */

bool InputStreamAccum::tryAgainLater()
{
	if ( later || ( !flush && head() == 0 ))
		return true;

	return false;
}

int InputStreamAccum::needFlush()
{
	if ( flush ) {
		flush = false;
		return true;
	}

	if ( head() != 0 && head()->type != RunBuf::DataType )
		return true;

	if ( eof )
		return true;
		
	return false;
}

int InputStreamAccum::getDataImpl( char *dest, int length )
{
	/* No source of data, it is all done with RunBuf list appends. */
	return 0;
}

void InputStreamAccum::pushBackBufImpl( RunBuf *runBuf )
{
	prepend( runBuf );
}

void InputStreamAccum::appendImpl( const char *data, long len )
{
	RunBuf *ad = new RunBuf;

	InputStream::append( ad );

	/* FIXME: need to deal with this. */
	assert( len < (int)sizeof(ad->data) );

	memcpy( ad->data, data, len );
	ad->length = len;
}

void InputStreamAccum::appendImpl( Tree *tree )
{
	RunBuf *ad = new RunBuf;

	InputStream::append( ad );

	ad->type = RunBuf::TokenType;
	ad->tree = tree;
	ad->length = 0;
}

