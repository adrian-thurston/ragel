/*
 *  Copyright 2001 Adrian Thurston <thurston@cs.queensu.ca>
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

#ifndef _AAPL_COMPARE_H
#define _AAPL_COMPARE_H

#include <string.h>
#include "table.h"

#ifdef AAPL_NAMESPACE
namespace Aapl {
#endif

/**
 * \defgroup compare Compare
 * \brief Basic compare clases.
 *
 * Compare classes are used by data structures that need to know the relative
 * ordering of elemets. To become a compare class, a class must imlement a
 * static routine Compare() that behaves just like strcmp. In some cases the
 * compare class is passed to the data structure. Such is the case with
 * BstMap. In other cases the comapre class is expected to be the structure
 * that is being contained. AvlTree expects the element to implement the compare
 * routine.
 *
 * @{
 */

/**
 * \brief Compare two c-style strings
 *
 * Wrapper for strcmp.
 */
struct CmpStr
{
	/**
	 * \brief Compare two null terminated string types.
	 */
	static inline long compare(const char *k1, const char *k2)
		{ return strcmp(k1, k2); }
};

/**
 * \brief Compare a an ordinal type that implements '<' and '>' operators.
 *
 * CmpOrd is suitable for simple types such as integers and pointers that by
 * default have the less than and greater than operators defined.
 */
template <class T> struct CmpOrd
{
	/**
	 * \brief Compare two ordinal types.
	 *
	 * This compare routine copies its arguements in by value.
	 */
	static inline long compare(const T k1, const T k2)
	{
		if (k1 < k2)
			return -1;
		else if (k1 > k2)
			return 1;
		else
			return 0;
	}
};

/**
 * \brief Compare two tables of type T
 *
 * CmpTable is useful for keying a data structure on a vector or binary search
 * table. T is the type of the table that the compare is for. CmpTable
 * requires that a compare class for T be given.
 */
template < class T, class CompareT = CmpOrd<T> > struct CmpTable
{
	/**
	 * \brief Compare two tables storing type T.
	 */
	static inline long compare(const Table<T> &t1, const Table<T> &t2)
	{
		if ( t1.tabLen < t2.tabLen )
			return -1;
		else if ( t1.tabLen > t2.tabLen )
			return 1;
		else
		{
			T *i1 = t1.data, *i2 = t2.data;
			long len = t1.tabLen, cmpResult;
			for ( long pos = 0; pos < len;
					pos += 1, i1 += 1, i2 += 1 )
			{
				cmpResult = CompareT::compare(*i1, *i2);
				if ( cmpResult != 0 )
					return cmpResult;
			}
			return 0;
		}
	}
};

/**
 * \brief Compare two tables of type T -- non-static version.
 *
 * CmpTable is useful for keying a data structure on a vector or binary search
 * table. T is the type of the table that the compare is for. CmpTable
 * requires that a compare class for T be given.
 */
template < class T, class CompareT = CmpOrd<T> > struct CmpTableNs
{
	/**
	 * \brief Compare two tables storing type T.
	 *
	 * Non-static for CompareT classes that have non-static compare routine.
	 */
	inline long compare(const Table<T> &t1, const Table<T> &t2) const
	{
		if ( t1.tabLen < t2.tabLen )
			return -1;
		else if ( t1.tabLen > t2.tabLen )
			return 1;
		else
		{
			T *i1 = t1.data, *i2 = t2.data;
			long len = t1.tabLen, cmpResult;
			for ( long pos = 0; pos < len;
					pos += 1, i1 += 1, i2 += 1 )
			{
				cmpResult = CompareT::compare(*i1, *i2);
				if ( cmpResult != 0 )
					return cmpResult;
			}
			return 0;
		}
	}
};

/**
 * \brief Compare two implicitly shared tables of type T
 *
 * CmpTable is useful for keying a data structure on a vector or binary search
 * table. T is the type of the table that the compare is for. CmpTable
 * requires that a compare class for T be given.
 */
template < class T, class CompareT = CmpOrd<T> > struct CmpSTable
{
	/**
	 * \brief Compare two tables storing type T.
	 */
	static inline long compare(const STable<T> &t1, const STable<T> &t2)
	{
		long t1Length = t1.length();
		long t2Length = t2.length();

		/* Compare lengths. */
		if ( t1Length < t2Length )
			return -1;
		else if ( t1Length > t2Length )
			return 1;
		else {
			/* Compare the table data. */
			T *i1 = t1.data, *i2 = t2.data;
			for ( long pos = 0; pos < t1Length;
					pos += 1, i1 += 1, i2 += 1 )
			{
				long cmpResult = CompareT::compare(*i1, *i2);
				if ( cmpResult != 0 )
					return cmpResult;
			}
			return 0;
		}
	}
};

/**
 * \brief Compare two implicitly shared tables of type T -- non-static
 * version.
 *
 * CmpTable is useful for keying a data structure on a vector or binary search
 * table. T is the type of the table that the compare is for. CmpTable
 * requires that a compare class for T be given.
 */
template < class T, class CompareT = CmpOrd<T> > struct CmpSTableNs
{
	/**
	 * \brief Compare two tables storing type T.
	 *
	 * Non-static for CompareT classes that have non-static compare routine.
	 */
	inline long compare(const STable<T> &t1, const STable<T> &t2) const
	{
		long t1Length = t1.length();
		long t2Length = t2.length();

		/* Compare lengths. */
		if ( t1Length < t2Length )
			return -1;
		else if ( t1Length > t2Length )
			return 1;
		else {
			/* Compare the table data. */
			T *i1 = t1.data, *i2 = t2.data;
			for ( long pos = 0; pos < t1Length;
					pos += 1, i1 += 1, i2 += 1 )
			{
				long cmpResult = CompareT::compare(*i1, *i2);
				if ( cmpResult != 0 )
					return cmpResult;
			}
			return 0;
		}
	}
};


/*@}*/

#ifdef AAPL_NAMESPACE
}
#endif

#endif /* _AAPL_COMPARE_H */
