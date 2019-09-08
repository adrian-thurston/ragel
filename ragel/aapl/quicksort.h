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

#ifndef _AAPL_QUICKSORT_H
#define _AAPL_QUICKSORT_H

#include "insertsort.h"

#ifdef AAPL_NAMESPACE
namespace Aapl {
#endif

/**
 * \addtogroup sort 
 * @{
 */

/** 
 * \class QuickSort
 * \brief Quick sort an array of data.
 *
 * QuickSort can be used to sort any array of objects of type T provided a
 * compare class is given. QuickSort is in-place. It does not require any
 * temporary storage.
 *
 * Objects are not made aware that they are being moved around in memory.
 * Assignment operators, constructors and destructors are never invoked by the
 * sort.
 *
 * QuickSort runs in O(n*log(n)) time in the average case. It is faster than
 * mergsort in the average case because it does less moving of data. The
 * performance of quicksort depends mostly on the choice of pivot. This
 * implementation picks the pivot as the median of first, middle, last. This
 * choice of pivot avoids the O(n^2) worst case for input already sorted, but
 * it is still possible to encounter the O(n^2) worst case. For example an
 * array of identical elements will run in O(n^2)
 *
 * QuickSort is not a stable sort. Elements with the same key will not have
 * their relative ordering preserved.  QuickSort switches to an InsertSort
 * when the size of the array being sorted is small. This happens when
 * directly sorting a small array or when QuickSort calls iteself recursively
 * on a small portion of a larger array.
 */

/*@}*/

/* QuickSort. */
template <class T, class Compare> class QuickSort : 
		public InsertSort<T, Compare>
{
public:
	/* Sorting interface routine. */
	void sort(T *data, long len);

private:
	/* Recursive worker. */
	void doSort(T *start, T *end);
	T *partition(T *start, T *end);
	inline T *median(T *start, T *end);
};

#define _QS_INSERTION_THRESH 16

/* Finds the median of start, middle, end. */
template <class T, class Compare> T *QuickSort<T,Compare>::
		median(T *start, T *end)
{
	T *pivot, *mid = start + (end-start)/2;

	/* CChoose the pivot. */
	if ( compare(*start, *mid) < 0  ) {
		if ( compare(*mid, *end) < 0 )
			pivot = mid;
		else if ( compare(*start, *end) < 0 )
			pivot = end;
		else
			pivot = start;
	}
	else if ( compare(*start, *end) < 0 )
		pivot = start;
	else if ( compare(*mid, *end) < 0 )
		pivot = end;
	else
		pivot = mid;

	return pivot;
}

template <class T, class Compare> T *QuickSort<T,Compare>::
		partition(T *start, T *end)
{
	/* Use the median of start, middle, end as the pivot. First save
	 * it off then move the last element to the free spot. */
	char pcPivot[sizeof(T)];
	T *pivot = median(start, end);

	memcpy( pcPivot, pivot, sizeof(T) );
	if ( pivot != end )
		memcpy( pivot, end, sizeof(T) );

	T *first = start-1;
	T *last = end;
	pivot = (T*) pcPivot;

	/* Shuffle element to the correct side of the pivot, ending
	 * up with the free spot where the pivot will go. */
	while ( true ) {
		/* Throw one element ahead to the free spot at last. */
		while ( true ) {
			first += 1;
			if ( first == last )
				goto done;
			if ( compare( *first, *pivot ) > 0 ) {
				memcpy(last, first, sizeof(T));
				break;
			}
		}

		/* Throw one element back to the free spot at first. */
		while ( true ) {
			last -= 1;
			if ( last == first )
				goto done;
			if ( compare( *last, *pivot ) < 0 ) {
				memcpy(first, last, sizeof(T));
				break;
			}
		}
	}
done:
	/* Put the pivot into the middle spot for it. */
	memcpy( first, pivot, sizeof(T) );
	return first;
}


template< class T, class Compare> void QuickSort<T,Compare>::
		doSort(T *start, T *end)
{
	long len = end - start + 1;
	if ( len > _QS_INSERTION_THRESH ) {
		/* Use quicksort. */
		T *pivot = partition( start, end );
		doSort(start, pivot-1);
		doSort(pivot+1, end);
	} 
	else if ( len > 1 ) {
		/* Array is small, use insertion sort. */
		InsertSort<T, Compare>::sort( start, len );
	}
}

/**
 * \brief Quick sort an array of data.
 */
template< class T, class Compare> 
	void QuickSort<T,Compare>::sort(T *data, long len)
{
	/* Call recursive worker. */
	doSort(data, data+len-1);
}

#ifdef AAPL_NAMESPACE
}
#endif

#endif /* _AAPL_QUICKSORT_H */
