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

/* This header is not wrapped in ifndef becuase it is not intended to
 * be included by the user. */

#ifdef AAPL_NAMESPACE
namespace Aapl {
#endif

#ifndef __AAPL_BLKSIZEONEKELS_
#define __AAPL_BLKSIZEONEKELS_
/* FIXME: want this to be a fixed size, say sizeof(T)/8K, but don't want to
 * have to replicate the type T in the the Deque parameter list by passing it
 * to the BlockSize class. */
struct BlkSizeOneKEls
{
	long getBlockSize( ) const
		{ return 1024; }
};
#endif

#ifndef __AAPL_DEQUEBLKHEAD_
#define __AAPL_DEQUEBLKHEAD_
/* Header for a chunk of data in a queue */
struct DequeBlkHead
{
	DequeBlkHead *prev, *next;
};
#endif

/* Deque. */
template < class T, class BlkSize = BlkSizeOneKEls > class Deque : 
	public DList<DequeBlkHead>, public BlkSize
{
public:
	Deque() : dequeLen(0) { }
	~Deque() { empty(); }
	
	void empty();
	
	/* Put a single item onto the deque. */
	void append( const T &item );
	void prepend( const T &item );

	/* Put multiple items onto the deque. */
	void append( T *data, long len );
	void prepend( T *data, long len );

	/* Copies len items from the deque. Does not modify. */
	void peekFront( T *dst, long len ) const;
	void peekEnd( T *dst, long len ) const;

	/* Remove len items off from the deque. */
	void removeFront( long len );
	void removeEnd( long len );

	/* Take len items off the deque and store in buffer. */
	void removeFront( T *dst, long len );
	void removeEnd( T *dst, long len );

	/* Ret the number of items in the deque. */
	long length() const { return dequeLen; }

	DequeBlkHead *allocBlk();
	void freeBlk( DequeBlkHead *block );

	/* Markers to the front and end of data. */
	long frontPos, endPos;

	/* Length of the data in the deque. */
	long dequeLen;

	/* Convenience access. */
	long size() const           { return dequeLen; }

	/* Forward this so a ref can be used. */
	struct Iter;

	/* Various classes for setting the iterator */
	struct IterFirst { IterFirst( const Deque &d ) : d(d) { } const Deque &d; };
	struct IterLast { IterLast( const Deque &d ) : d(d) { } const Deque &d; };
	struct IterNext { IterNext( const Iter &i ) : i(i) { } const Iter &i; };
	struct IterPrev { IterPrev( const Iter &i ) : i(i) { } const Iter &i; };

	/* Vector Iterator. */
	struct Iter
	{
		/* Construct, assign. */
		Iter() : ptr(0), ptrBeg(0), ptrEnd(0), deque(0), block(0) { }

		/* Construct. */
		Iter( const Deque &d )              { setWithFirst( d ); }
		Iter( const IterFirst &df )         { setWithFirst( df.d ); }
		Iter( const IterLast &dl )          { setWithLast( dl.d ); }
		inline Iter( const IterNext &dn );
		inline Iter( const IterPrev &dp );

		/* Assign. */
		Iter &operator=( const Deque &d )        
			{ setWithFirst( d ); return *this; }
		Iter &operator=( const IterFirst &df )   
			{ setWithFirst( df.d ); return *this; }
		Iter &operator=( const IterLast &dl )    
			{ setWithLast( dl.d ); return *this; }
		inline Iter &operator=( const IterNext &df );
		inline Iter &operator=( const IterPrev &dl );

		/* Workers for setting. */
		void setWithFirst( const Deque &d );
		void setWithLast( const Deque &d );

		/* \brief Less than end? */
		bool lte() const { return block != 0; }

		/* \brief At end? */
		bool end() const { return block == 0; }

		/* \brief Greater than beginning? */
		bool gtb() const { return block != 0; }

		/* \brief At beginning? */
		bool beg() const { return block == 0; }

		/* \brief At first element? */
		bool first() const { return block->prev == 0 && ptr == ptrBeg+1; }

		/* \brief At last element? */
		bool last() const { return block->next == 0 && ptr == ptrEnd-1; }

		/* \brief Implicit cast to T*. */
		operator T*() const   { return ptr; }

		/* \brief Dereference operator returns T&. */
		T &operator *() const { return *ptr; }

		/* \brief Arrow operator returns T*. */
		T *operator->() const { return ptr; }
		
		/* Worker for increment operators. */
		void findNext()
		{
			if ( ++ptr == ptrEnd ) {
				/* Move to the next chunk. Init ptr, ptrBeg and ptrEnd. */
				block = block->next;
				if ( block != 0 ) {
					ptr = ((T*)(block+1));
					ptrBeg = ptr-1;
					if ( block->next != 0 )
						ptrEnd = ptr + deque->BlkSize::getBlockSize();
					else
						ptrEnd = ptr + deque->endPos;
				}
			}
		}

		/* \brief Move to next item. */
		T *operator++()       
		{
			findNext();
			return ptr;
		}

		/* \brief Move to next item. */
		T *operator++(int)
		{ 
			T *ret = ptr;
			findNext();
			return ret;
		}

		/* \brief Move to next item. */
		T *increment()
		{
			findNext();
			return ptr;
		}

		/* Worker for decrement operators. */
		void findPrev()
		{
			if ( --ptr == ptrBeg ) {
				/* Move to the prev chunk. Init ptr, ptrBeg and ptrEnd. */
				block = block->prev;
				if ( block != 0 ) {
					ptrEnd = ((T*)(block+1)) + deque->BlkSize::getBlockSize();
					ptr = ptrEnd-1;
					ptrBeg = ((T*)(block+1)) - 1;
					if ( block->prev == 0 )
						ptrBeg += deque->frontPos;
				}
			}
		}

		/* \brief Move to previous item. */
		T *operator--()
		{
			findPrev();
			return ptr;
		}


		/* \brief Move to previous item. */
		T *operator--(int)
		{
			T *ret = ptr;
			findPrev();
			return ret;
		}

		/* \brief Move to previous item. */
		T *decrement()
		{
			findPrev();
			return ptr;
		}

#if 0
		/* \brief Return the next item. Does not modify this. */
		inline IterNext next() const { return IterNext(*this); }

		/* \brief Return the previous item. Does not modify this. */
		inline IterPrev prev() const { return IterPrev(*this); }
#endif

		/* The iterator is simply a pointer. */
		T *ptr;

		/* For testing endpoints. */
		T *ptrBeg, *ptrEnd;

		/* Need a pointer back into the deque and to keep track of the current
		 * block. */
		const Deque *deque;
		DequeBlkHead *block;
	};

	/* Return first element. */
	IterFirst first() { return IterFirst( *this ); }

	/* Return last element. */
	IterLast last() { return IterLast( *this ); }
};

/* Init a deque iterator to the first element of a deque. */
template <class T, class BlkSize> void Deque<T, BlkSize>::
		Iter::setWithFirst( const Deque &d ) 
{
	deque = &d;
	block = d.head;
	if ( d.head == 0 )
		ptr = ptrBeg = ptrEnd = 0;
	else {
		ptr = ((T*)(block+1)) + deque->frontPos;
		ptrBeg = ptr - 1;
		if ( block->next )
			ptrEnd = ((T*)(block+1)) + deque->BlkSize::getBlockSize();
		else
			ptrEnd = ((T*)(block+1)) + deque->endPos;
	}
}

/* Init a deque iterator to the last element of a deque. */
template <class T, class BlkSize> void Deque<T, BlkSize>::
		Iter::setWithLast( const Deque &d ) 
{
	deque = &d;
	block = d.tail;
	if ( d.tail == 0 )
		ptr = ptrBeg = ptrEnd = 0;
	else {
		ptrEnd = ((T*)(block+1)) + deque->endPos;
		ptr = ptrEnd - 1;
		if ( block->prev )
			ptrBeg = ((T*)(block+1)) - 1;
		else
			ptrBeg = ((T*)(block+1)) + deque->frontPos - 1;
	}
}

/* Allocate a block accroding to the scheme set out by BlkSize. */
template <class T, class BlkSize> DequeBlkHead *Deque<T, BlkSize>::
		allocBlk()
{
	return (DequeBlkHead*) malloc( sizeof(DequeBlkHead) + 
		sizeof(T) * BlkSize::getBlockSize() );
}

template <class T, class BlkSize> void Deque<T, BlkSize>::
		freeBlk( DequeBlkHead *block )
{
	free( block );
}

#ifdef AAPL_NAMESPACE
}
#endif
