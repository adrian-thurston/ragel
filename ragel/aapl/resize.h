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

#ifndef _AAPL_RESIZE_H
#define _AAPL_RESIZE_H

#include <assert.h>

#ifdef AAPL_NAMESPACE
namespace Aapl {
#endif

/* This step is expressed in units of T. Changing this requires changes to
 * docs in ResizeLin constructor.  */
#define LIN_DEFAULT_STEP 256

/*
 * Resizing macros giving different resize methods.
 */

/* If needed is greater than existing, give twice needed. */
#define EXPN_UP( existing, needed ) \
		needed > existing ? (needed<<1) : existing
	
/* If needed is less than 1 quarter existing, give twice needed. */
#define EXPN_DOWN( existing, needed ) \
		needed < (existing>>2) ? (needed<<1) : existing

/* If needed is greater than existing, give needed plus step. */
#define LIN_UP( existing, needed ) \
	needed > existing ? (needed+step) : existing

/* If needed is less than existing - 2 * step then give needed plus step. */
#define LIN_DOWN( existing, needed ) \
	needed < (existing-(step<<1)) ? (needed+step) : existing

/* Return existing. */
#define CONST_UP( existing, needed ) existing

/* Return existing. */
#define CONST_DOWN( existing, needed ) existing

/**
 * \addtogroup vector
 * @{
 */

/** \class ResizeLin
 * \brief Linear table resizer.
 *
 * When an up resize or a down resize is needed, ResizeLin allocates the space
 * needed plus some user defined step. The result is that when growing the
 * vector in a linear fashion, the number of resizes is also linear.
 *
 * If only up resizing is done, then there will never be more than step unused
 * spaces in the vector. If down resizing is done as well, there will never be
 * more than 2*step unused spaces in the vector. The up resizing and down
 * resizing policies are offset to improve performance when repeatedly
 * inserting and removing a small number of elements relative to the step.
 * This scheme guarantees that repetitive inserting and removing of a small
 * number of elements will never result in repetative reallocation.
 *
 * The vectors pass sizes to the resizer in units of T, so the step gets
 * interpreted as units of T.
 */

/*@}*/

/* Linear resizing. */
class ResizeLin
{
protected:
	/**
	 * \brief Default constructor.
	 *
	 * Intializes resize step to 256 units of the table type T.
	 */
	ResizeLin() : step(LIN_DEFAULT_STEP) { }

	/**
	 * \brief Determine the new table size when up resizing.
	 *
	 * If the existing size is insufficient for the space needed, then allocate
	 * the space needed plus the step. The step is in units of T.
	 */
	inline long upResize( long existing, long needed )
		{ return LIN_UP(existing, needed); }

	/**
	 * \brief Determine the new table size when down resizing.
	 *
	 * If space needed is less than the existing - 2*step, then allocate the
	 * space needed space plus the step. The step is in units of T.
	 */
	inline long downResize( long existing, long needed )
		{ return LIN_DOWN(existing, needed); }

public:
	/**
	 * \brief Step for linear resize.
	 *
	 * Amount of extra space in units of T added each time a resize must take
	 * place. This may be changed at any time. The step should be >= 0.
	 */
	long step;
};

/**
 * \addtogroup vector
 * @{
 */

/** \class ResizeCtLin
 * \brief Linear table resizer with compile time step.
 *
 * When an up resize or a down resize is needed, ResizeCtLin allocates the
 * space needed plus some compile time defined step. The result is that when
 * growing the vector in a linear fashion, the number of resizes is also
 * linear.
 *
 * If only up resizing is done, then there will never be more than step unused
 * spaces in the vector. If down resizing is done as well, there will never be
 * more than 2*step unused spaces in the vector. The up resizing and down
 * resizing policies are offset to improve performance when repeatedly
 * inserting and removing a small number of elements relative to the step.
 * This scheme guarantees that repetitive inserting and removing of a small
 * number of elements will never result in repetative reallocation.
 *
 * The vectors pass sizes to the resizer in units of T, so the step gets
 * interpreted as units of T.
 */

/*@}*/

/* Linear resizing. */
template <long step> class ResizeCtLin
{
protected:
	/**
	 * \brief Determine the new table size when up resizing.
	 *
	 * If the existing size is insufficient for the space needed, then allocate
	 * the space needed plus the step. The step is in units of T.
	 */
	inline long upResize( long existing, long needed )
		{ return LIN_UP(existing, needed); }

	/**
	 * \brief Determine the new table size when down resizing.
	 *
	 * If space needed is less than the existing - 2*step, then allocate the
	 * space needed space plus the step. The step is in units of T.
	 */
	inline long downResize( long existing, long needed )
		{ return LIN_DOWN(existing, needed); }
};

/**
 * \addtogroup vector
 * @{
 */

/** \class ResizeConst
 * \brief Constant table resizer.
 *
 * When an up resize is needed the existing size is always used. ResizeConst
 * does not allow dynamic resizing. To use ResizeConst, the vector needs to be
 * constructed with and initial allocation amount otherwise it will be
 * unusable.
 */

/*@}*/

/* Constant table resizing. */
class ResizeConst
{
protected:
	/* Assert don't need more than exists. Return existing. */
	static inline long upResize( long existing, long needed );

	/**
	 * \brief Determine the new table size when down resizing.
	 *
	 * Always returns the existing table size.
	 */
	static inline long downResize( long existing, long needed )
		{ return CONST_DOWN(existing, needed); }
};

/**
 * \brief Determine the new table size when up resizing.
 *
 * If the existing size is insufficient for the space needed, then an assertion
 * will fail. Otherwise returns the existing size.
 */
inline long ResizeConst::upResize( long existing, long needed )
{	
	assert( needed <= existing ); 
	return CONST_UP(existing, needed); 
}

/**
 * \addtogroup vector
 * @{
 */

/** \class ResizeRunTime
 * \brief Run time settable table resizer.
 *
 * ResizeRunTime can have it's up and down resizing policies set at run time.
 * Both up and down policies can be set independently to one of Exponential,
 * Linear, or Constant. See the documentation for ResizeExpn, ResizeLin, and
 * ResizeConst for the details of the resizing policies. 
 *
 * The policies may be changed at any time. The default policies are
 * both Exponential.
 */

/*@}*/

/* Run time resizing. */
class ResizeRunTime
{
protected:
	/**
	 * \brief Default constuctor.
	 *
	 * The up and down resizing it initialized to Exponetial. The step
	 * defaults to 256 units of T.
	 */
	inline ResizeRunTime();

	/**
	 * \brief Resizing policies.
	 */
	enum ResizeType {
		Exponential,  /*!< Exponential resizing. */
		Linear,       /*!< Linear resizing. */
		Constant      /*!< Constant table size. */
	};

	inline long upResize( long existing, long needed );
	inline long downResize( long existing, long needed );

public:
	/**
	 * \brief Step for linear resize.
	 *
	 * Amount of extra space in units of T added each time a resize must take
	 * place. This may be changed at any time. The step should be >= 0.
	 */
	long step;

	/**
	 * \brief Up resizing policy.
	 */
	ResizeType upResizeType;

	/**
	 * \brief Down resizing policy.
	 */
	ResizeType downResizeType;
};

inline ResizeRunTime::ResizeRunTime()
:
	step( LIN_DEFAULT_STEP ),
	upResizeType( Exponential ),
	downResizeType( Exponential )
{
}

/**
 * \brief Determine the new table size when up resizing.
 *
 * Type of up resizing is determined by upResizeType. Exponential, Linear and
 * Constant resizing is the same as that of ResizeExpn, ResizeLin and
 * ResizeConst.
 */
inline long ResizeRunTime::upResize( long existing, long needed )
{
	switch ( upResizeType ) {
	case Exponential:
		return EXPN_UP(existing, needed);
	case Linear:
		return LIN_UP(existing, needed);
	case Constant:
		assert( needed <= existing ); 
		return CONST_UP(existing, needed);
	}
	return 0;
};

/**
 * \brief Determine the new table size when down resizing.
 *
 * Type of down resizing is determined by downResiizeType. Exponential, Linear
 * and Constant resizing is the same as that of ResizeExpn, ResizeLin and
 * ResizeConst.
 */
inline long ResizeRunTime::downResize( long existing, long needed )
{
	switch ( downResizeType ) {
	case Exponential:
		return EXPN_DOWN(existing, needed);
	case Linear:
		return LIN_DOWN(existing, needed);
	case Constant:
		return CONST_DOWN(existing, needed);
	}
	return 0;
}

/* Don't need these anymore. */
#undef EXPN_UP
#undef EXPN_DOWN
#undef LIN_UP
#undef LIN_DOWN
#undef CONST_UP
#undef CONST_DOWN

#ifdef AAPL_NAMESPACE
}
#endif

#endif /* _AAPL_RESIZE_H */
