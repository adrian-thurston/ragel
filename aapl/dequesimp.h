/*
 *  Copyright 2003 Adrian Thurston <adriant@ragel.ca>
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

#ifndef _AAPL_DEQUESIMP_H
#define _AAPL_DEQUESIMP_H

#include <string.h>
#include <malloc.h>
#include "dlist.h"

/* Get the common deque code. */
#define Deque DequeSimp
#define DEQUESIMP
#include "dequecommon.h"
#undef DEQUESIMP
#undef Deque

#ifdef AAPL_NAMESPACE
namespace Aapl {
#endif

/**
 * \addtogroup deque
 * @{
 */

/** \class DequeSimp
 * \brief Double Ended Queue for classes with trivial initialization.
 */

/*@}*/

template <class T, class BlkSize> void DequeSimp<T, BlkSize>::empty()
{
	/* Delete each Iterate over all elements and delete each. */
	DequeBlkHead *blk = head;
	while ( blk != 0 ) {
		/* Get next, free and then goto the next. */
		DequeBlkHead *next = blk->next;
		freeBlk( blk );
		blk = next;
	}

	/* Reset base list data and our data. */
	head = tail = 0;
	listLen = dequeLen = 0;
}

template <class T, class BlkSize> void DequeSimp<T, BlkSize>::
		append( const T &item )
{
	/* Mem allocation failures throw an exception, we will always succeed. */
	dequeLen += 1;

	/* If the list is initially empty, we need to init the frontPos. */
	if ( listLen == 0 )
		frontPos = 0;

	/* If the list is empty or if the last block is full we need to make
	 * some space to write to. */
	if ( listLen == 0 || endPos == BlkSize::getBlockSize() ) {
		/* Need a new block because the list is initially empty or because the
		 * last block is full. Set up the initial enpos now. */
		endPos = 0;
		DList<DequeBlkHead>::append( allocBlk() );
	}

	/* Copy into the space left. */
	T *dst = ((T*)(tail+1)) + endPos;
	memcpy( dst, &item, sizeof(T) );
	endPos += 1;
}

template <class T, class BlkSize> void DequeSimp<T, BlkSize>::
		prepend( const T &item )
{
	/* Mem allocation failures throw an exception, we will always succeed. */
	dequeLen += 1;

	/* If the list is initially empty, we need to init the endPos. */
	if ( listLen == 0 )
		endPos = BlkSize::getBlockSize();

	/* If the list is empty or if the first block is full we need to make some
	 * space to write to. */
	if ( listLen == 0 || frontPos == 0 ) {
		/* Need a new block because the list is initially empty or because the
		 * first block is full. Set up the initial frontPos. */
		frontPos = BlkSize::getBlockSize();
		DList<DequeBlkHead>::prepend( allocBlk() );
	}

	/* Copy the item in. */
	T *dst = ((T*)(head+1)) + frontPos - 1;
	memcpy( dst, &item, sizeof(T) );
	frontPos -= 1;
}

template <class T, class BlkSize> void DequeSimp<T, BlkSize>::
		append( T *data, long len )
{
	/* Mem allocation failures throw an exception, we will always succeed. */
	dequeLen += len;

	/* If the list is initially empty, we need to init the frontPos. */
	if ( listLen == 0 )
		frontPos = 0;

	/* If the list is empty or if the last block is full we need to make
	 * some space to do the first write to. */
	if ( listLen == 0 || endPos == BlkSize::getBlockSize() ) {
		/* Need a new block because the list is initially empty or because the
		 * last block is full. Set up the initial enpos now. */
		endPos = 0;
		DList<DequeBlkHead>::append( allocBlk() );
	}

	while ( true ) {
		/* Copy what's left in the current block, but no more then len. */
		long copyLen = BlkSize::getBlockSize() - endPos;
		if ( copyLen > len )
			copyLen = len;

		/* Copy into the space left. */
		T *dst = ((T*)(tail+1))+endPos;
		memcpy( dst, data, copyLen*sizeof(T) );

		/* Advance over the data coppied. */
		data += copyLen;
		len -= copyLen;

		/* If no more to copy, break. */
		endPos += copyLen;
		if ( len == 0 )
			break;

		/* Add another chunk for the remaing data. */
		DList<DequeBlkHead>::append( allocBlk() );
		endPos = 0;
	}
}

template <class T, class BlkSize> void DequeSimp<T, BlkSize>::
		prepend( T *data, long len )
{
	/* Mem allocation failures throw an exception, we will always succeed. */
	dequeLen += len;

	/* If the list is initially empty, we need to init the endPos. */
	if ( listLen == 0 )
		endPos = BlkSize::getBlockSize();

	/* If the list is empty or if the first block is full we need to make some
	 * space to do the first write to. */
	if ( listLen == 0 || frontPos == 0 ) {
		/* Need a new block because the list is initially empty or because the
		 * first block is full. Set up the initial frontPos. */
		frontPos = BlkSize::getBlockSize();
		DList<DequeBlkHead>::prepend( allocBlk() );
	}

	/* Start at the end and work backwards. */
	data += len;

	while ( true ) {
		/* Copy what's left in the current block, but no more then len. */
		long copyLen = frontPos;
		if ( copyLen > len )
			copyLen = len;

		/* Copy into the space left. */
		T *dst = ((T*)(head+1)) + frontPos - copyLen;
		memcpy( dst, data-copyLen, copyLen*sizeof(T) );

		/* Advance over the data coppied. */
		data -= copyLen;
		len -= copyLen;

		/* If no more to copy, break. */
		frontPos -= copyLen;
		if ( len == 0 )
			break;

		/* Add another chunk for the remaing data. */
		frontPos = BlkSize::getBlockSize();
		DList<DequeBlkHead>::prepend( allocBlk() );
	}
}


template <class T, class BlkSize> void DequeSimp<T, BlkSize>::
		peekFront( T *dst, long len ) const
{
	/* The number of bytes in the first chunk. */
	long lenInHead = (listLen > 1 ? BlkSize::getBlockSize() : endPos) - frontPos;
	T *src = ((T*)(head+1)) + frontPos;
	DequeBlkHead *block = head;

	while ( true ) {
		long copyLen = len < lenInHead ? len : lenInHead;
		memcpy( dst, src, copyLen*sizeof(T) );

		/* If no more is needed, we are done. */
		len -= copyLen;
		dst += copyLen;
		if ( len == 0 )
			break;

		/* Go to the next block, load up the len in head and src pointers. */
		block = block->next;
		lenInHead = block->next == 0 ? endPos : BlkSize::getBlockSize();
		src = ((T*)(block+1));
	}
}


template <class T, class BlkSize> void DequeSimp<T, BlkSize>::
		peekEnd( T *dst, long len ) const
{
	/* The number of bytes in the last block. */
	long lenInTail = endPos - ((listLen > 1) ? 0 : frontPos);
	T *src = ((T*)(tail+1)) + endPos;
	DequeBlkHead *block = tail;

	/* Starting at the end and moving backwards. */
	dst += len;

	while ( true ) {
		long copyLen = len < lenInTail ? len : lenInTail;
		memcpy( dst-copyLen, src-copyLen, copyLen*sizeof(T) );

		/* If no more is needed, we are done. */
		len -= copyLen;
		dst -= copyLen;
		if ( len == 0 )
			break;

		/* Move to the previous block, load up lenInTail and src pointer. */
		block = block->prev;
		lenInTail = BlkSize::getBlockSize() - ((block->prev == 0) ? frontPos : 0);
		src = ((T*)(block+1)) + BlkSize::getBlockSize();
	}
}


template <class T, class BlkSize> void DequeSimp<T, BlkSize>::
		removeFront( long len )
{
	/* The number of bytes in the first chunk. */
	long lenInHead = (listLen > 1 ? BlkSize::getBlockSize() : endPos) - frontPos;

	/* Decrement the deque length. */
	dequeLen -= len;

	while ( true ) {
		/* How much data to delete? */
		long delLen = len < lenInHead ? len : lenInHead;
		frontPos += delLen;
		if ( len < lenInHead )
			break;

		/* Block has become empty. */
		freeBlk( DList<DequeBlkHead>::detachFirst( ) );
		frontPos = 0;

		len -= delLen;
		if ( len == 0 )
			break;

		lenInHead = listLen > 1 ? BlkSize::getBlockSize() : endPos;
	}
}

template <class T, class BlkSize> void DequeSimp<T, BlkSize>::
		removeEnd( long len )
{
	/* The number of bytes in the last block. */
	long lenInTail = endPos - ((listLen > 1) ? 0 : frontPos);

	/* Decrement the deque length. */
	dequeLen -= len;

	while ( true ) {
		long delLen = len < lenInTail ? len : lenInTail;
		endPos -= delLen;
		if ( len < lenInTail )
			break;

		freeBlk( DList<DequeBlkHead>::detachLast( ) );
		endPos = BlkSize::getBlockSize();

		len -= delLen;
		if ( len == 0 )
			break;

		lenInTail = BlkSize::getBlockSize() - ((listLen > 1) ? 0 : frontPos);
	}
}

template <class T, class BlkSize> void DequeSimp<T, BlkSize>::
		removeFront( T *dst, long len )
{
	/* The number of bytes in the first chunk. */
	long lenInHead = (listLen > 1 ? BlkSize::getBlockSize() : endPos) - frontPos;
	T *src = ((T*)(head+1)) + frontPos;

	/* Decrement the deque length. */
	dequeLen -= len;

	while ( true ) {
		long copyLen = len < lenInHead ? len : lenInHead;
		memcpy( dst, src, copyLen*sizeof(T) );

		/* If no more data is needed, then done. */
		frontPos += copyLen;
		if ( len < lenInHead )
			break;

		freeBlk( DList<DequeBlkHead>::detachFirst( ) );
		frontPos = 0;

		len -= copyLen;
		dst += copyLen;
		if ( len == 0 )
			break;

		lenInHead = listLen > 1 ? BlkSize::getBlockSize() : endPos;
		src = ((T*)(head+1));
	}
}


template <class T, class BlkSize> void DequeSimp<T, BlkSize>::
		removeEnd( T *dst, long len )
{
	/* The number of bytes in the last block. */
	long lenInTail = endPos - ((listLen > 1) ? 0 : frontPos);
	T *src = ((T*)(tail+1)) + endPos;

	/* Decrement the deque length. */
	dequeLen -= len;

	/* Starting at the end and moving backwards. */
	dst += len;

	while ( true ) {
		long copyLen = len < lenInTail ? len : lenInTail;
		memcpy( dst-copyLen, src-copyLen, copyLen*sizeof(T) );

		/* If no more data is needed, then done. */
		endPos -= copyLen;
		if ( len < lenInTail )
			break;

		freeBlk( DList<DequeBlkHead>::detachLast( ) );
		endPos = BlkSize::getBlockSize();

		len -= copyLen;
		dst -= copyLen;
		if ( len == 0 )
			break;

		lenInTail = BlkSize::getBlockSize() - ((listLen > 1) ? 0 : frontPos);
		src = ((T*)(tail+1)) + BlkSize::getBlockSize();
	}
}


#ifdef AAPL_NAMESPACE
}
#endif

#endif /* _AAPL_DEQUESIMP_H */

