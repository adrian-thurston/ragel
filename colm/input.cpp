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
#include "colm.h"
#include "fsmrun.h"
#include <stdio.h>
#include <iostream>

using std::cerr;
using std::endl;

/*
 * String
 */

int InputStreamString::getData( char *dest, int length )
{ 
	int available = data.length() - offset;

	if ( available < length )
		length = available;

	memcpy( dest, data.data+offset, length );
	offset += length;

	if ( offset == data.length() )
		eof = true;

	return length;
}

void InputStreamString::pushBackData( char *data, long len )
{
	assert( len <= offset );
	offset -= len;
}

void InputStreamString::pushBackBuf( RunBuf *runBuf )
{
	assert( false );
}

/*
 * File
 */

int InputStreamFile::isEOF()
{
	return queue == 0 && feof( file );
}

int InputStreamFile::needFlush()
{
	return queue == 0 && feof( file );
}

int InputStreamFile::getData( char *dest, int length )
{
	/* If there is any data in queue, read from that first. */
	if ( queue != 0 ) {
		long avail = queue->length - queue->offset;
		if ( length >= avail ) {
			memcpy( dest, &queue->buf[queue->offset], avail );
			RunBuf *del = queue;
			queue = queue->next;
			delete del;
			return avail;
		}
		else {
			memcpy( dest, &queue->buf[queue->offset], length );
			queue->offset += length;
			return length;
		}
	}
	else {
		return fread( dest, 1, length, file );
	}
}

void InputStreamFile::pushBackData( char *data, long len )
{
	assert( false );
}

void InputStreamFile::pushBackBuf( RunBuf *runBuf )
{
	runBuf->next = queue;
	queue = runBuf;
}

/*
 * FD
 */

int InputStreamFD::isEOF()
{
	return queue == 0 && eof;
}

int InputStreamFD::needFlush()
{
	return queue == 0 && eof;
}

void InputStreamFD::pushBackBuf( RunBuf *runBuf )
{
	runBuf->next = queue;
	queue = runBuf;
}

void InputStreamFD::pushBackData( char *data, long len )
{
	assert( false );
}

int InputStreamFD::getData( char *dest, int length )
{
	/* If there is any data in queue, read from that first. */
	if ( queue != 0 ) {
		long avail = queue->length - queue->offset;
		if ( length >= avail ) {
			memcpy( dest, &queue->buf[queue->offset], avail );
			RunBuf *del = queue;
			queue = queue->next;
			delete del;
			return avail;
		}
		else {
			memcpy( dest, &queue->buf[queue->offset], length );
			queue->offset += length;
			return length;
		}
	}
	else {
		long got = read( fd, dest, length );
		if ( got == 0 )
			eof = true;
		return got;
	}
}
