/*
 * Copyright 2001 Adrian Thurston <thurston@colm.net>
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
 * routine long compare(const T &key1, const T &key2) that behaves just like
 * strcmp. 
 *
 * Compare classes are passed to the template data structure as a template
 * parameter and are inherited. In most cases the compare routine will base
 * the key comparision only on the two keys and the compare routine can
 * therefore be static. Though sometimes it is useful to include data in the
 * compare class and use this data in the comparison. For example the compare
 * class may contain a pointer to some other data structure to which the
 * comparison is delegated.
 *
 * @{
 */

/**
 * \brief Compare a type for which < and > are implemented.
 *
 * CmpOrd is suitable for simple types such as integers and pointers that by
 * default have the less-than and greater-than operators defined.
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
 * Table comparison is useful for keying a data structure on a vector or
 * binary search table. T is the element type stored in the table.
 * CompareT is the comparison structure used to compare the individual values
 * in the table.
 */
template < class T, class CompareT = CmpOrd<T> > struct CmpTable 
		: public CompareT
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
 * CmpTableNs is identical to CmpTable, however the compare routine is
 * non-static.  If the CompareT class contains a non-static compare, then this
 * version must be used because a static member cannot invoke a non-static
 * member.
 *
 * Table comparison is useful for keying a data structure on a vector or binary
 * search table. T is the element type stored in the table. CompareT
 * is the comparison structure used to compare the individual values in the
 * table.
 */
template < class T, class CompareT = CmpOrd<T> > struct CmpTableNs 
		: public CompareT
{
	/**
	 * \brief Compare two tables storing type T.
	 */
	inline long compare(const Table<T> &t1, const Table<T> &t2)
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
 * This table comparison is for data structures based on implicitly
 * shared tables.
 *
 * Table comparison is useful for keying a data structure on a vector or
 * binary search table. T is the element type stored in the table.
 * CompareT is the comparison structure used to compare the individual values
 * in the table.
 */
template < class T, class CompareT = CmpOrd<T> > struct CmpSTable : public CompareT
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
 * This is a non-static table comparison for data structures based on
 * implicitly shared tables. If the CompareT class contains a non-static
 * compare, then this version must be used because a static member cannot
 * invoke a non-static member.
 *
 * Table comparison is useful for keying a data structure on a vector or
 * binary search table. T is the element type stored in the table.
 * CompareT is the comparison structure used to compare the individual values
 * in the table.
 */
template < class T, class CompareT = CmpOrd<T> > struct CmpSTableNs 
		: public CompareT
{
	/**
	 * \brief Compare two tables storing type T.
	 */
	inline long compare(const STable<T> &t1, const STable<T> &t2)
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
