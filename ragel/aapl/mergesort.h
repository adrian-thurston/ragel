/*
 * Copyright 2001, 2002 Adrian Thurston <thurston@colm.net>
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

#ifndef _AAPL_MERGESORT_H
#define _AAPL_MERGESORT_H

#include "bubblesort.h"

#ifdef AAPL_NAMESPACE
namespace Aapl {
#endif

/**
 * \addtogroup sort 
 * @{
 */

/** 
 * \class MergeSort
 * \brief Merge sort an array of data.
 *
 * MergeSort can be used to sort any array of objects of type T provided a
 * compare class is given. MergeSort is not in-place, it requires temporary
 * storage equal to the size of the array. The temporary storage is allocated
 * on the heap.
 *
 * Objects are not made aware that they are being moved around in memory.
 * Assignment operators, constructors and destructors are never invoked by the
 * sort. 
 *
 * MergeSort runs in worst case O(n*log(n)) time. In most cases it is slower
 * than QuickSort because more copying is neccessary. But on the other hand,
 * it is a stable sort, meaning that objects with the same key have their
 * relative ordering preserved. Also, its worst case is better. MergeSort
 * switches to a BubbleSort when the size of the array being sorted is small.
 * This happens when directly sorting a small array or when MergeSort calls
 * itself recursively on a small portion of a larger array.
 */

/*@}*/


/* MergeSort. */
template <class T, class Compare> class MergeSort 
		: public BubbleSort<T, Compare>
{
public:
	/* Sorting interface routine. */
	void sort(T *data, long len);

private:
	/* Recursive worker. */
	void doSort(T *tmpStor, T *data, long len);
};

#define _MS_BUBBLE_THRESH 16

/* Recursive mergesort worker. Split data, make recursive calls, merge
 * results. */
template< class T, class Compare> void MergeSort<T,Compare>::
		doSort(T *tmpStor, T *data, long len)
{
	if ( len <= 1 )
		return;

	if ( len <= _MS_BUBBLE_THRESH ) {
		BubbleSort<T, Compare>::sort( data, len );
		return;
	}

	long mid = len / 2;

	doSort( tmpStor, data, mid );
	doSort( tmpStor + mid, data + mid, len - mid );
	
	/* Merge the data. */
	T *endLower = data + mid, *lower = data;
	T *endUpper = data + len, *upper = data + mid;
	T *dest = tmpStor;
	while ( true ) {
		if ( lower == endLower ) {
			/* Possibly upper left. */
			if ( upper != endUpper )
				memcpy( dest, upper, (endUpper - upper) * sizeof(T) );
			break;
		}
		else if ( upper == endUpper ) {
			/* Only lower left. */
			if ( lower != endLower )
				memcpy( dest, lower, (endLower - lower) * sizeof(T) );
			break;
		}
		else {
			/* Both upper and lower left. */
			if ( this->compare(*lower, *upper) <= 0 )
				memcpy( dest++, lower++, sizeof(T) );
			else
				memcpy( dest++, upper++, sizeof(T) );
		}
	}

	/* Copy back from the tmpStor array. */
	memcpy( data, tmpStor, sizeof( T ) * len );
}

/**
 * \brief Merge sort an array of data.
 */
template< class T, class Compare> 
	void MergeSort<T,Compare>::sort(T *data, long len)
{
	/* Allocate the tmp space needed by merge sort, sort and free. */
	T *tmpStor = (T*) new char[sizeof(T) * len];
	doSort( tmpStor, data, len );
	delete[] (char*) tmpStor;
}

#ifdef AAPL_NAMESPACE
}
#endif

#endif /* _AAPL_MERGESORT_H */
