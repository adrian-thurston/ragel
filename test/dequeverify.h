/*
 *  Copyright 2003 Adrian Thurston <thurston@cs.queensu.ca>
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

#include <assert.h>

#ifdef AAPL_NAMESPACE
namespace Aapl {
#endif


template < class T, class BlkSize = BlkSizeOneKEls > class DequeVer 
: 
	public Deque<T, BlkSize>
{
	typedef Deque<T, BlkSize> BaseDeque;

public:
	void verifyIntegrity();
};

template < class T, class BlkSize > void DequeVer<T, BlkSize>::verifyIntegrity()
{
	if ( BaseDeque::listLen == 0 ) {
		/* Len should be zero, everything else unspecified. */
		assert( BaseDeque::dequeLen == 0 );
	}
	else if ( BaseDeque::listLen == 1 ) {
		/* Front block should always have at least one. */
		assert( BaseDeque::dequeLen > 0 );
		assert( 0 <= BaseDeque::frontPos );
		assert( BaseDeque::frontPos < BaseDeque::endPos );
		assert( BaseDeque::endPos <= BlkSize::getBlockSize() );
	}
	else if ( BaseDeque::listLen > 1 ) {
		/* Must be more than one item. */
		assert( BaseDeque::dequeLen > 1 );

		/* Front and end blocks should always have at least one. */
		assert( 0 <= BaseDeque::frontPos );
		assert( BaseDeque::frontPos < BlkSize::getBlockSize() );

		/* Front block should always have at least one. */
		assert( 0 < BaseDeque::endPos );
		assert( BaseDeque::endPos <= BlkSize::getBlockSize() );
	}
}

/* namespace Aapl */
#ifdef AAPL_NAMESPACE
}
#endif

