/*
 *  Copyright 2016 Adrian Thurston <thurston@complang.org>
 */

/*  This file is part of Aapl.
 *
 *  Aapl is free software; you can redistribute it and/or modify it under the
 *  terms of the GNU Lesser General Public License as published by the Free
 *  Software Foundation; either version 2.1 of the License, or (at your option)
 *  any later version.
 *
 *  Aapl is distributed in the hope that it will be useful, but WITHOUT ANY
 *  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 *  FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
 *  more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with Aapl; if not, write to the Free Software Foundation, Inc., 59
 *  Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#ifndef _AAPL_ROPE_H
#define _AAPL_ROPE_H

#include <string.h>

struct RopeBlock
{
	static const int BLOCK_SZ = 8192;

	RopeBlock()
	:
		size(BLOCK_SZ)
	{
	}

	long size;
	RopeBlock *next;
};

struct Rope
{
	Rope()
	:
		hblk(0), tblk(0), toff(0), ropeLen(0)
	{
	}

	/* Write to the tail block, at tail offset. */
	RopeBlock *hblk;
	RopeBlock *tblk;

	/* Head and tail offset. */
	int toff;
	int ropeLen;

	RopeBlock *allocateBlock( int supporting )
	{
		int size = ( supporting > RopeBlock::BLOCK_SZ ) ? supporting : RopeBlock::BLOCK_SZ;
		char *bd = new char[sizeof(RopeBlock) + size];
		RopeBlock *block = (RopeBlock*) bd;
		block->size = size;
		block->next = 0;
		return block;
	}

	char *data( RopeBlock *rb )
		{ return (char*)rb + sizeof( RopeBlock ); }

	int length( RopeBlock *rb )
		{ return rb->next != 0 ? rb->size : toff; }
	
	int length()
	{
		return ropeLen;
	}

	char *append( const char *src, int len )
	{
		if ( tblk == 0 ) {
			/* There are no blocks. */
			hblk = tblk = allocateBlock( len );
			toff = 0;
		}
		else {
			int avail = tblk->size - toff;

			/* Move to the next block? */
			if ( len > avail ) {
				/* Terminate the existing tail block by setting the size to the
				 * farthest we got in. */
				tblk->size = toff;

				RopeBlock *block = allocateBlock( len );
				tblk->next = block;
				tblk = block;
				toff = 0;
			}
		}

		char *ret = data(tblk) + toff;
		toff += len;
		ropeLen += len;

		if ( src != 0 )
			memcpy( ret, src, len );
		return ret;
	}

	void empty()
	{
		hblk = 0;
		tblk = 0;
		toff = 0;
		ropeLen = 0;
	}
};

#endif

