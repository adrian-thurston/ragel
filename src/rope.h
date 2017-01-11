/*
 * Copyright 2016 Adrian Thurston <thurston@colm.net>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
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
		hblk(0), tblk(0), toff(0), ropeLen(0), blkHdrSz(0)
	{
	}

	/* Write to the tail block, at tail offset. */
	RopeBlock *hblk;
	RopeBlock *tblk;

	/* Head and tail offset. */
	int toff;
	int ropeLen;

	/* This lets the user put some data at the head of a block to use how they
	 * please. */
	int blkHdrSz;

	RopeBlock *allocateBlock( int supporting )
	{
		int size = ( supporting > RopeBlock::BLOCK_SZ ) ? supporting : RopeBlock::BLOCK_SZ;
		char *bd = new char[sizeof(RopeBlock) + blkHdrSz + size];
		RopeBlock *block = (RopeBlock*) bd;
		block->size = size;
		block->next = 0;
		return block;
	}

	char *data( RopeBlock *rb )
		{ return (char*)rb + sizeof( RopeBlock ) + blkHdrSz; }

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

