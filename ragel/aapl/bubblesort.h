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

#ifndef _AAPL_BUBBLESORT_H
#define _AAPL_BUBBLESORT_H

#ifdef AAPL_NAMESPACE
namespace Aapl {
#endif

/**
 * \addtogroup sort 
 * @{
 */

/** 
 * \class BubbleSort
 * \brief Bubble sort an array of data.
 *
 * BubbleSort can be used to sort any array of objects of type T provided a
 * compare class is given. BubbleSort is in-place. It does not require any
 * temporary storage.
 *
 * Objects are not made aware that they are being moved around in memory.
 * Assignment operators, constructors and destructors are never invoked by the
 * sort.
 *
 * BubbleSort runs in O(n^2) time. It is most useful when sorting arrays that
 * are nearly sorted. It is best when neighbouring pairs are out of place.
 * BubbleSort is a stable sort, meaning that objects with the same key have
 * their relative ordering preserved.
 */

/*@}*/

/* BubbleSort. */
template <class T, class Compare> class BubbleSort 
	: public Compare
{
public:
	/* Sorting interface routine. */
	void sort(T *data, long len);
};


/**
 * \brief Bubble sort an array of data.
 */
template <class T, class Compare> void BubbleSort<T,Compare>::
		sort(T *data, long len)
{
	bool changed = true;
	for ( long pass = 1; changed && pass < len; pass ++ ) {
		changed = false;
		for ( long i = 0; i < len-pass; i++ ) {
			/* Do we swap pos with the next one? */
			if ( this->compare( data[i], data[i+1] ) > 0 ) {
				char tmp[sizeof(T)];

				/* Swap the two items. */
				memcpy( tmp, data+i, sizeof(T) );
				memcpy( data+i, data+i+1, sizeof(T) );
				memcpy( data+i+1, tmp, sizeof(T) );

				/* Note that we made a change. */
				changed = true;
			}
		}
	}
}

#ifdef AAPL_NAMESPACE
}
#endif

#endif /* _AAPL_BUBBLESORT_H */
