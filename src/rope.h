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
		size(BLOCK_SZ),
		hoff(0),
		toff(0)
	{
	}

	int size;
	int hoff;
	int toff;
	RopeBlock *next;
};

struct Rope
{
	Rope()
	:
		hblk(0),
		tblk(0),
		ropeLen(0)
	{
	}

	/* Write to the tail block, at tail offset. */
	RopeBlock *hblk;
	RopeBlock *tblk;

	/* Number of bytes in rope. */
	int ropeLen;

	RopeBlock *allocateBlock( int supporting )
	{
		int size = ( supporting > RopeBlock::BLOCK_SZ ) ? supporting : RopeBlock::BLOCK_SZ;
		char *bd = new char[sizeof(RopeBlock) + size];
		RopeBlock *block = (RopeBlock*) bd;
		block->size = size;
		block->hoff = 0;
		block->toff = 0;
		block->next = 0;
		return block;
	}

	char *data( RopeBlock *rb )
		{ return (char*)rb + sizeof( RopeBlock ) + rb->hoff; }
	
	char *writeTo( RopeBlock *rb )
		{ return (char*)rb + sizeof( RopeBlock ) + rb->toff; }

	int length( RopeBlock *rb )
		{ return rb->toff - rb->hoff; }
	
	int length()
		{ return ropeLen; }

	int available( RopeBlock *rb )
		{ return rb->size - rb->toff; }

	char *append( const char *src, int len )
	{
		if ( tblk == 0 ) {
			/* There are no blocks. */
			hblk = tblk = allocateBlock( len );
		}
		else {
			int avail = available( tblk );

			/* Move to the next block? */
			if ( len > avail ) {
				RopeBlock *block = allocateBlock( len );
				tblk->next = block;
				tblk = block;
			}
		}

		char *ret = writeTo(tblk);
		tblk->toff += len;
		ropeLen += len;

		if ( src != 0 )
			memcpy( ret, src, len );
		return ret;
	}

	char *appendBlock( int len )
	{
		if ( tblk == 0 ) {
			/* There are no blocks. */
			hblk = tblk = allocateBlock( len );
		}
		else {
			RopeBlock *block = allocateBlock( len );
			tblk->next = block;
			tblk = block;
		}

		char *ret = writeTo(tblk);
		tblk->toff += len;
		ropeLen += len;
		return ret;
	}

	/* Transfer data from other. Leaves it empty. */
	void append( Rope &other )
	{
		if ( hblk == 0 ) {
			transfer( other );
		}
		else if ( other.hblk == 0 ) {
			/* nothing to do, other list empty. */
		}
		else {
			tblk->next = other.hblk;
			tblk = other.tblk;
			ropeLen += other.ropeLen;
		}

		other.abandon();
	}

	void empty()
	{
		RopeBlock *blk = hblk;
		while ( blk != 0 ) {
			RopeBlock *next = blk->next;
			delete[] (char*)blk;
			blk = next;
		}

		hblk = 0;
		tblk = 0;
		ropeLen = 0;
	}

	void abandon()
	{
		hblk = 0;
		tblk = 0;
		ropeLen = 0;
	}

	void transfer( Rope &from )
	{
		empty();

		this->hblk = from.hblk;
		this->tblk = from.tblk;
		this->ropeLen = from.ropeLen;

		from.hblk = from.tblk = 0;
		from.ropeLen = 0;
	}
};

#endif

