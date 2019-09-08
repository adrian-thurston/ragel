/*
 * Copyright 2002 Adrian Thurston <thurston@colm.net>
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

#ifndef _AAPL_INSERTSORT_H
#define _AAPL_INSERTSORT_H

#ifdef AAPL_NAMESPACE
namespace Aapl {
#endif

/**
 * \addtogroup sort 
 * @{
 */

/** 
 * \class InsertSort
 * \brief Insertion sort an array of data.
 *
 * InsertSort can be used to sort any array of objects of type T provided a
 * compare class is given. InsertSort is in-place. It does not require any
 * temporary storage.
 *
 * Objects are not made aware that they are being moved around in memory.
 * Assignment operators, constructors and destructors are never invoked by the
 * sort.
 *
 * InsertSort runs in O(n^2) time. It is most useful when sorting small arrays.
 * where it can outperform the O(n*log(n)) sorters due to its simplicity.
 * InsertSort is a not a stable sort. Elements with the same key will not have
 * their relative ordering preserved.
 */

/*@}*/

/* InsertSort. */
template <class T, class Compare> class InsertSort
	: public Compare
{
public:
	/* Sorting interface routine. */
	void sort(T *data, long len);
};


/**
 * \brief Insertion sort an array of data.
 */
template <class T, class Compare> 
	void InsertSort<T,Compare>::sort(T *data, long len)
{
	/* For each next largest spot in the sorted array... */
	for ( T *dest = data; dest < data+len-1; dest++ ) {
		/* Find the next smallest element in the unsorted array. */
		T *smallest = dest;
		for ( T *src = dest+1; src < data+len; src++ ) {
			/* If src is smaller than the current src, then use it. */
			if ( compare( *src, *smallest ) < 0 )
				smallest = src;
		}

		if ( smallest != dest ) {
			/* Swap dest, smallest. */
			char tmp[sizeof(T)];
			memcpy( tmp, dest, sizeof(T) );
			memcpy( dest, smallest, sizeof(T) );
			memcpy( smallest, tmp, sizeof(T) );
		}
	}
}

#ifdef AAPL_NAMESPACE
}
#endif

#endif /* _AAPL_INSERTSORT_H */
