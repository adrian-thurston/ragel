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

int InputStream::getData( char *dest, int length )
{
	return getDataImpl( dest, length );
}

int InputStream::isEof()
{
	return isEofImpl();
}

int InputStream::needFlush()
{
	return needFlushImpl();
}

void InputStream::pushBackBuf( RunBuf *runBuf )
{
	pushBackBufImpl( runBuf );
}

void InputStream::append( const char *data, long len )
{
	return appendImpl( data, len );
}

void InputStream::append( Tree *tree )
{
	return appendImpl( tree );
}

bool InputStream::tryAgainLater()
{
	return tryAgainLaterImpl();
}

bool InputStream::isTree()
{
	return isTreeImpl();
}

Tree *InputStream::getTree()
{
	return getTreeImpl();
}

bool InputStream::isIgnore()
{
	return isIgnoreImpl();
}

bool InputStream::isLangEl()
{
	return isLangElImpl();
}

KlangEl *InputStream::getLangEl( long &bindId, char *&data, long &length )
{
	return getLangElImpl( bindId, data, length );
}

void InputStream::pushBackNamed()
{
	return pushBackNamedImpl();
}

/* 
 * Implementation
 */


bool InputStream::tryAgainLaterImpl()
{
	if ( later )
		return true;

	return false;
}

bool InputStream::isTreeImpl()
{ 
	if ( head() != 0 && head()->type == RunBuf::TokenType )
		return true;
	return false;
}

bool InputStream::isIgnoreImpl()
{ 
	if ( head() != 0 && head()->type == RunBuf::IgnoreType )
		return true;
	return false;
}

Tree *InputStream::getTreeImpl()
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

int InputStreamFile::isEofImpl()
{
	return head() == 0 && feof( file );
}

int InputStreamFile::needFlushImpl()
{
	return head() == 0 && feof( file );
}

int InputStreamFile::getDataImpl( char *dest, int length )
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

void InputStreamFile::pushBackBufImpl( RunBuf *runBuf )
{
	prepend( runBuf );
}

/*
 * FD
 */

int InputStreamFd::isEofImpl()
{
	return head() == 0 && eof;
}

int InputStreamFd::needFlushImpl()
{
	return head() == 0 && eof;
}

void InputStreamFd::pushBackBufImpl( RunBuf *runBuf )
{
	prepend( runBuf );
}

int InputStreamFd::getDataImpl( char *dest, int length )
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

int InputStreamAccum::isEofImpl()
{
	return eof;
}

bool InputStreamAccum::tryAgainLaterImpl()
{
	if ( later || ( !flush && head() == 0 ))
		return true;

	return false;
}

int InputStreamAccum::needFlushImpl()
{
	if ( flush ) {
		flush = false;
		return true;
	}

	if ( head() != 0 )
		return true;

	if ( eof )
		return true;
		
	return false;
}

int InputStreamAccum::getDataImpl( char *dest, int length )
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

bool InputStreamAccum::isTreeImpl()
{ 
	if ( head() != 0 && head()->type == RunBuf::TokenType )
		return true;

	return false;
}

Tree *InputStreamAccum::getTreeImpl()
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

