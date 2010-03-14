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

bool InputStream::tryAgainLater()
{
	if ( later )
		return true;

	return false;
}

bool InputStream::isTree()
{ 
	if ( head() != 0 && head()->type == RunBuf::TokenType )
		return true;
	return false;
}

bool InputStream::isIgnore()
{ 
	if ( head() != 0 && head()->type == RunBuf::IgnoreType )
		return true;
	return false;
}

Tree *InputStream::getTree()
{
	RunBuf *runBuf = popHead();

	/* FIXME: using runbufs here for this is a poor use of memory. */
	Tree *tree = runBuf->tree;
	delete runBuf;
	return tree;
}


/*
 * String
 */

int InputStreamString::getData( char *dest, int length )
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

void InputStreamString::pushBackBuf( RunBuf *runBuf )
{
	//char *data = runBuf->buf + runBuf->offset;
	long length = runBuf->length;

	assert( length <= offset );
	offset -= length;
}

/*
 * File
 */

int InputStreamFile::isEOF()
{
	return head() == 0 && feof( file );
}

int InputStreamFile::needFlush()
{
	return head() == 0 && feof( file );
}

int InputStreamFile::getData( char *dest, int length )
{
	/* If there is any data in queue2, read from that first. */
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
		return fread( dest, 1, length, file );
	}
}

void InputStreamFile::pushBackBuf( RunBuf *runBuf )
{
	prepend( runBuf );
}

/*
 * FD
 */

int InputStreamFd::isEOF()
{
	return head() == 0 && eof;
}

int InputStreamFd::needFlush()
{
	return head() == 0 && eof;
}

void InputStreamFd::pushBackBuf( RunBuf *runBuf )
{
	prepend( runBuf );
}

int InputStreamFd::getData( char *dest, int length )
{
	/* If there is any data in queue2, read from that first. */
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
		long got = read( fd, dest, length );
		if ( got == 0 )
			later = true;
		//	eof = true;
		return got;
	}
}

/*
 * Accum
 */

int InputStreamAccum::isEOF()
{
	return eof;
}

bool InputStreamAccum::tryAgainLater()
{
	if ( later || ( !flush && adHead() == 0 && adTail() == 0  ))
		return true;

	return false;
}

int InputStreamAccum::needFlush()
{
	if ( flush ) {
		flush = false;
		return true;
	}

	if ( adHead() != 0 )
		return true;

	if ( eof )
		return true;
		
	return false;
}

int InputStreamAccum::getData( char *dest, int length )
{
	/* If there is any data in queue2, read from that first. */
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
		if ( adHead() == 0 )
			return 0;

		int available = adHead()->length - offset;

		if ( available < length )
			length = available;

		memcpy( dest, adHead()->data + offset, length );
		offset += length;

		if ( offset == adHead()->length ) {
			consumeAd();
			offset = 0;
		}

		return length;
	}
}

void InputStreamAccum::pushBackBuf( RunBuf *runBuf )
{
	prepend( runBuf );
}

void InputStreamAccum::append( const char *data, long len )
{
	RunBuf *ad = new RunBuf;

	appendAd( ad );

	/* FIXME: need to deal with this. */
	assert( len < (int)sizeof(ad->data) );

	memcpy( ad->data, data, len );
	ad->length = len;
}

void InputStreamAccum::append( Tree *tree )
{
	RunBuf *ad = new RunBuf;

	appendAd( ad );

	ad->type = RunBuf::TokenType;
	ad->tree = tree;
	ad->length = 0;
}

bool InputStreamAccum::isTree()
{ 
	if ( head() != 0 && head()->type == RunBuf::TokenType )
		return true;

	if ( adHead() != 0 && adHead()->type == RunBuf::TokenType )
		return true;

	return false;
}

Tree *InputStreamAccum::getTree()
{
	if ( head() != 0 && head()->type == RunBuf::TokenType ) {
		RunBuf *runBuf = popHead();

		/* FIXME: using runbufs here for this is a poor use of memory. */
		Tree *tree = runBuf->tree;
		delete runBuf;
		return tree;
	}
	else if ( adHead() != 0 && adHead()->type == RunBuf::TokenType ) {
		RunBuf *ad = adHead();

		consumeAd();

		Tree *tree = ad->tree;
		delete ad;
		return tree;
	}

	assert( false );
}

