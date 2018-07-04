/** @file tinyspline.h */
/*
 *  The MIT License (MIT)
 *
 *  Copyright (c) 2016 Marcel Steinbeck
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files (the "Software"), to deal
 *  in the Software without restriction, including without limitation the rights
 *  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *  copies of the Software, and to permit persons to whom the Software is
 *  furnished to do so, subject to the following conditions:
 *
 *  The above copyright notice and this permission notice shall be included in all
 *  copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 *  SOFTWARE.
 */
#ifndef TINYSPLINE_H
#define TINYSPLINE_H

#include <stddef.h>

#ifdef  __cplusplus
extern "C" {
#endif

/******************************************************************************
*                                                                             *
* System Dependent Configuration                                              *
*                                                                             *
* The following configuration values must be adjusted to your system. Some of *
* them may be configured using preprocessor definitions. The default values   *
* should be fine for most modern hardware, such as x86, x86_64, and arm.      *
*                                                                             *
******************************************************************************/
#ifdef TINYSPLINE_FLOAT_PRECISION
typedef float tsReal;
#else
typedef double tsReal;
#endif

#define FLT_MAX_ABS_ERROR   1e-5
#define FLT_MAX_REL_ERROR   1e-8


/******************************************************************************
*                                                                             *
* Data Types                                                                  *
*                                                                             *
* The following section defines all data types available in TinySpline.       *
*                                                                             *
******************************************************************************/
/**
 * Contains all error codes used by TinySpline. The following code snippet
 * shows how to handle errors:
 *
 *      tsError err = ...       // any function call here
 *      if (err < 0) {          // or use err != TS_SUCCESS
 *          printf("we got an error!");
 *
 *          // you may want to reuse error codes
 *          return err;
 *      }
 */
typedef enum
{
    /* No error. */
    TS_SUCCESS = 0,

    /* Unable to allocate memory (using malloc/realloc). */
    TS_MALLOC = -1,

    /* The dimension of the control points are 0. */
    TS_DIM_ZERO = -2,

    /* Degree of spline (deg) >= number of control points (n_ctrlp). */
    TS_DEG_GE_NCTRLP = -3,

    /* Spline is not defined at knot value u. */
    TS_U_UNDEFINED = -4,

    /* Multiplicity of a knot (s) > order of spline.  */
    TS_MULTIPLICITY = -5,

    /* Decreasing knot vector. */
    TS_KNOTS_DECR = -6,

    /* Unexpected number of knots. */
    TS_NUM_KNOTS = -7,

    /* Spline is not derivable */
    TS_UNDERIVABLE = -8
} tsError;

/**
 * Describes how the knot vector of a spline is organized. If you don't know
 * what an opened or clamped spline is, have a look at:
 *
 *     www.cs.mtu.edu/~shene/COURSES/cs3621/NOTES/spline/B-spline/bspline-curve.html
 */
typedef enum
{
    /* Not available/Undefined. */
    TS_NONE = 0,

    /* Uniformly spaced knot vector. */
    TS_OPENED = 1,

    /* Uniformly spaced knot vector with clamped end knots. */
    TS_CLAMPED = 2,

    /* Uniformly spaced knot vector with s(u) = order of spline. */
    TS_BEZIERS = 3
} tsBSplineType;

/**
 * Represents a B-Spline which may also be used for NURBS, Bezier curves,
 * lines, and points. NURBS are represented using homogeneous coordinates where
 * the last component of a control point is its weight. Bezier curves are
 * B-Splines with 'n_ctrlp == order' and clamped knot vector making the curve
 * passing through the first and last control point. If a Bezier curve consists
 * of two control points only, we call them a line. Points, ultimately, are
 * just very short lines having only a single control point. Consequently, the
 * degree of a point is zero.
 *
 * Two dimensional control points are organized as follows:
 *
 *     [x_0, y_0, x_1, y_1, ..., x_n-1, y_n-1]
 *
 * Tree dimensional control points are organized as follows:
 *
 *     [x_0, y_0, z_0, x_1, y_1, z_1, ..., x_n-1, y_n-1, z_n-1]
 *
 * ... and so on. NURBS are represented using homogeneous coordinates. For
 * instance, let's say we have a NURBS in 2D consisting of 11 control points
 * where 'w_i' is the weight of the i'th control point. Then the corresponding
 * control points are organized as follows:
 *
 *     [x_0, y_0, w_0, x_1, y_1, w_1, ..., x_10, y_10, w_10]
 *
 * Note: The fields 'ctrlp' and 'knots' share the same array (similar to the
 *       approach used in 'tsDeBoorNet'). That is, the first elements of this
 *       array contain the control points of a spline whereas the last elements
 *       contain its knots. Accordingly, you should never free 'knots'
 *       explicitly. Using 'ts_bspline_free()' to free dynamically allocated
 *       memory is to be preferred anyway. If 'ctrlp' and 'knots' do not share
 *       the same array, or at least a consistent block of data, functions
 *       provided by TinySpline my fail because values are copied block wise.
 */
typedef struct
{
    /* Degree of B-Spline basis function. */
    size_t deg;

    /* A convenience field for deg+1. */
    size_t order;

    /* Dimension of a control points. */
    size_t dim;

    /* Number of control points. */
    size_t n_ctrlp;

    /* Number of knots (n_ctrlp + deg + 1). */
    size_t n_knots;

    /* Control points of a spline. */
    tsReal* ctrlp;

    /* Knot vector of a spline (ascending order). */
    tsReal* knots;
} tsBSpline;

/**
 * Represents the output of De Boor's algorithm. De Boor's algorithm is used to
 * evaluate a spline at given knot value 'u' by iteratively computing a net of
 * intermediate values until the result is available:
 *
 *     https://en.wikipedia.org/wiki/De_Boor%27s_algorithm
 *     https://www.cs.mtu.edu/~shene/COURSES/cs3621/NOTES/spline/de-Boor.html
 *
 * All points of the net are stored in 'points'. The resulting point of an
 * evaluation is the last point in 'points' and, for the sake of convenience,
 * may be accessed using 'result':
 *
 *     tsDeBoorNet net = ...    // evaluate an arbitrary spline and store
 *                              // resulting net of points in 'net'
 *
 *     net.result ...           // use 'result' to access resulting point
 *
 * Note: You should never free 'result' explicitly as it is just a convenient
 *       accessor for the last point in 'points'. Using 'ts_deboornet_free()'
 *       to free dynamically allocated memory is to be preferred anyway.
 *
 * Two dimensional points are organized as follows:
 *
 *     [x_0, y_0, x_1, y_1, ..., x_n-1, y_n-1]
 *
 * Tree dimensional points are organized as follows:
 *
 *     [x_0, y_0, z_0, x_1, y_1, z_1, ..., x_n-1, y_n-1, z_n-1]
 *
 * ... and so on.
 *
 * There is a special case in which the evaluation of a knot value 'u' returns
 * two instead of one result. It occurs when the multiplicity of 'u' ( s(u) )
 * is equals to a spline's order indicating that the spline is discontinuous at
 * 'u'. This is common practice for B-Splines (or NURBS) consisting of
 * connected Bezier curves where the endpoint of curve 'c_i' is equals to the
 * start point of curve 'c_i+1'. The end point of 'c_i' and the start point of
 * 'c_i+1' may still be completely different though, yielding to a spline
 * having a (real and visible) gap at 'u'. Consequently, De Boor's algorithm
 * must return two results if 's(u) == order' in order to give you access to
 * the desired points.  In such case, 'points' stores only the two resulting
 * points (there is no net to create) and 'result' points to the *first* point
 * in 'points' ('points' and 'result' store the same pointer). Since having
 * (real) gaps in splines is unusual, both points in 'points' are generally
 * equals making it easy to handle this special case by accessing 'result' as
 * already shown above for regular cases:
 *
 *     tsDeBoorNet net = ...    // evaluate a spline which is discontinuous at
 *                              // at given knot value yielding to a net with
 *                              // two results
 *
 *     net.result ...           // use 'result' to access resulting point
 *
 * However, you can use both points if necessary:
 *
 *     tsDeBoorNet net = ...    // evaluate a spline which is discontinuous at
 *                              // at given knot value yielding to a net with
 *                              // two results
 *
 *     net.result[0] ...        // 'result[0]' stores the first component of
 *                              // the first point
 *
 *     net.result[net.dim]      // 'result[net.dim]' stores the first component
 *                              // of the second point
 *
 * As if this wasn't complicated enough, there is an exception for our special
 * case yielding to exactly one result (just like the regular case) even if
 * 's(u) == order'. It occurs when 'u' is the lower or upper bound of a
 * spline's domain. For instance, if 'b' is a spline with domain [0, 1] and is
 * evaluated at 'u = 0' or 'u = 1' then 'result' is *always* a single point
 * regardless of the multiplicity of 'u'. Hence, handling this exception is
 * straightforward:
 *
 *     tsDeBoorNet net = ...    // evaluate a spline at lower or upper bound of
 *                              // its domain, for instance, 0 or 1
 *
 *     net.result ...           // use 'result' to access resulting point
 *
 * In summary, we have three different types of evaluation. 1) The regular case
 * returning all points of the net we used to calculate the resulting point. 2)
 * The special case returning exactly two points which is required for splines
 * having (real) gaps. 3) The exception of 2) returning exactly one point even
 * if 's(u) == order'. All in all this looks quite complex (and actually it is)
 * but for most applications you don't need to bother with them. Just use
 * 'result' to access your evaluation point.
 */
typedef struct
{
    /* The evaluated knot value. */
    tsReal u;

    /* The index [u_k, u_k+1) */
    size_t k;

    /* Multiplicity of u_k. */
    size_t s;

    /* How many times u must be inserted to get the resulting point. */
    size_t h;

    /* Dimension of a control point. */
    size_t dim;

    /* Number of points in 'points'. */
    size_t n_points;

    /* Points of the net used to evaluate u_k. */
    tsReal* points;

    /* A convenient pointer to the result in 'points'. */
    tsReal* result;
} tsDeBoorNet;


/******************************************************************************
*                                                                             *
* Constructors, Destructors, Copy, and Move Functions                         *
*                                                                             *
* The following section contains functions to create and delete instances of  *
* the data types listed above. Additionally, each data type has a copy and    *
* move function.                                                              *
*                                                                             *
******************************************************************************/
/**
 * The default constructor of tsBSpline.
 *
 * All values of \p \_spline\_ are set to 0/NULL.
 *
 * @param \_spline\_
 *  The spline whose values are set 0/NULL.
 */
void ts_bspline_default( tsBSpline* _spline_ );

/**
 * A convenient constructor for tsBSpline.
 *
 * On error, all values of \p \_spline\_ are set to 0/NULL.
 *
 * @param n_ctrlp
 *  The number of control points of \p \_spline\_.
 * @param dim
 *  The dimension of each control point in \p \_spline\_.
 * @param deg
 *  The degree of \p \_spline\_.
 * @param type
 *  How to setup the knot vector of \p \_spline\_.
 * @param \_spline\_
 *  The output parameter storing the result of this function.
 * @return TS_SUCCESS
 *  On success.
 * @return TS_DIM_ZERO
 *  If \p deg == 0.
 * @return TS_DEG_GE_NCTRLP
 *  If \p deg >= \p n_ctrlp.
 * @return TS_NUM_KNOTS
 *  If \p type == ::TS_BEZIERS and (\p n_ctrlp % \p deg + 1) != 0.
 * @return TS_MALLOC
 *  If allocating memory failed.
 */
tsError ts_bspline_new( size_t n_ctrlp, size_t dim, size_t deg,
        tsBSplineType type, tsBSpline* _spline_ );

/**
 * The copy constructor of tsBSpline.
 *
 * Creates a deep copy of \p original and stores the result in \p \_copy\_.
 *
 * On error, all values of \p \_copy\_ are set to 0/NULL. Does nothing, if
 * \p original == \p \_copy\_.
 *
 * @param original
 *  The spline to deep copy.
 * @param \_copy\_
 *  The output parameter storing the copied values of \p original.
 * @return TS_SUCCESS
 *  On success.
 * @return TS_MALLOC
 *  If allocating memory failed.
 */
tsError ts_bspline_copy( const tsBSpline* original, tsBSpline* _copy_ );

/**
 * The move constructor of tsBSpline.
 *
 * Moves all values from \p from to \p \_to\_ and calls ::ts_bspline_default
 * on \p from afterwards. Does nothing, if \p from == \p \_to\_.
 *
 * @param from
 *  The spline whose values are moved to \p \_to\_.
 * @param \_to\_
 *  The output parameter storing the moved values of \p from.
 */
void ts_bspline_move( tsBSpline* from, tsBSpline* _to_ );

/**
 * The destructor of tsBSpline.
 *
 * Frees all dynamically allocated memory in \p \_spline\_ and calls
 * ::ts_bspline_default afterwards.
 *
 * @param \_spline\_
 *  The spline to free.
 */
void ts_bspline_free( tsBSpline* _spline_ );

/**
 * The default constructor of tsDeBoorNet.
 *
 * All values of \p \_deBoorNet\_ are set to 0/NULL.
 *
 * @param \_deBoorNet\_
 *  The net whose values are set 0/NULL.
 */
void ts_deboornet_default( tsDeBoorNet* _deBoorNet_ );

/**
 * The copy constructor of tsDeBoorNet.
 *
 * Creates a deep copy of \p original and stores the result in \p \_copy\_.
 *
 * On error, all values of \p _copy_ are set to 0/NULL. Does nothing, if
 * \p original == \p \_copy\_.
 *
 * @param original
 *  The net to deep copy.
 * @param \_copy\_
 *  The output parameter storing the copied values of \p original.
 * @return TS_SUCCESS
 *  On success.
 * @return TS_MALLOC
 *  If allocating memory failed.
 */
tsError ts_deboornet_copy( const tsDeBoorNet* original, tsDeBoorNet* _copy_ );

/**
 * The move constructor of tsDeBoorNet.
 *
 * Moves all values from \p from to \p \_to\_ and calls ::ts_deboornet_default
 * on \p from afterwards. Does nothing, if \p from == \p \_to\_.
 *
 * @param from
 *  The net whose values are moved to \p \_to\_.
 * @param \_to\_
 *  The output parameter storing the moved values of \p from.
 */
void ts_deboornet_move( tsDeBoorNet* from, tsDeBoorNet* _to_ );

/**
 * The destructor of tsDeBoorNet.
 *
 * Frees all dynamically allocated memory in \p \_deBoorNet\_ and calls
 * ::ts_deboornet_default afterwards.
 *
 * @param \_deBoorNet\_
 *  The net to free.
 */
void ts_deboornet_free( tsDeBoorNet* _deBoorNet_ );


/******************************************************************************
*                                                                             *
* Interpolation and Approximation Functions                                   *
*                                                                             *
* The following section contains functions to interpolate and approximate     *
* arbitrary splines.                                                          *
*                                                                             *
******************************************************************************/
/**
 * Performs a cubic spline interpolation using the thomas algorithm, see:
 *
 *     https://en.wikipedia.org/wiki/Tridiagonal_matrix_algorithm
 *     http://www.math.ucla.edu/~baker/149.1.02w/handouts/dd_splines.pdf
 *     http://www.bakoma-tex.com/doc/generic/pst-bspline/pst-bspline-doc.pdf
 *
 * The resulting spline is a sequence of bezier curves connecting each point
 * in \p points. Each bezier curve _b_ is of degree 3 with \p dim being the
 * dimension of the each control point in _b_. The total number of control
 * points is (\p n - 1) * 4.
 *
 * On error, all values of \p \_spline\_ are set to 0/NULL.
 *
 * Note: \p n is the number of points in \p points and not the length of
 * \p points. For instance, the follwing point vector yields to \p n = 4 and
 * \p dim = 2:
 *
 *     [x0, y0, x1, y1, x2, y2, x3, y3]
 *
 * @param points
 *  The points to interpolate.
 * @param n
 *  The number of points in \p points.
 * @param dim
 *  The dimension of each control point in \p \_spline\_.
 * @param \_spline\_
 *  The output parameter storing the result of this function.
 * @return TS_SUCCESS
 *  On success.
 * @return TS_DIM_ZERO
 *  If \p dim == 0.
 * @return TS_DEG_GE_NCTRLP
 *  If \p n < 2.
 * @return TS_MALLOC
 *  If allocating memory failed.
 */
tsError ts_bspline_interpolate_cubic( const tsReal* points, size_t n,
        size_t dim, tsBSpline* _spline_ );


/******************************************************************************
*                                                                             *
* Query Functions                                                             *
*                                                                             *
* The following section contains functions to query splines.                  *
*                                                                             *
******************************************************************************/
/**
 * Evaluates \p spline at knot value \p u and stores the result in
 * \p \_deBoorNet\_.
 *
 * On error, all values of \p \_deBoorNet\_ are set to 0/NULL.
 *
 * @param spline
 *  The spline to evaluate.
 * @param u
 *  The knot value to evaluate.
 * @param \_deBoorNet\_
 *  The output parameter storing the evaluation result.
 * @return TS_SUCCESS
 *  On success.
 * @return TS_MULTIPLICITY
 *  If multiplicity s(\p u) > order of \p spline.
 * @return TS_U_UNDEFINED
 *  If \p spline is not defined at knot value \p u.
 * @return TS_MALLOC
 *  If allocating memory failed.
 */
tsError ts_bspline_evaluate( const tsBSpline* spline, tsReal u,
        tsDeBoorNet* _deBoorNet_ );


/******************************************************************************
*                                                                             *
* Transformation functions                                                    *
*                                                                             *
* TinySpline is a library focusing on transformations. That is, most          *
* functions are used to transform splines by modifying their state, e.g.,     *
* their number of control points, their degree, and so on. Accordingly, each  *
* transformation functions specifies an input and output parameter (along     *
* with the other parameters required to calculate the actual transformation). *
* By passing a different pointer to the output parameter, the transformation  *
* result is calculated and stored without changing the state of the input.    *
* This is in particular useful when dealing with errors as the original state *
* will never be modified. For instance, let's have a look at the following    *
* code snippet:                                                               *
*                                                                             *
*     tsBSpline in = ...    // an arbitrary spline                            *
*     tsBSpline out;        // result of transformation                       *
*                                                                             *
*     // Subdivide 'in' into sequence of bezier curves and store the result   *
*     // in 'out'. Does not change 'in' in any way.                           *
*     tsError err = ts_bspline_to_beziers(&in, &out);                         *
*     if (err != TS_SUCCESS) {                                                *
*         // fortunately, 'in' has not been changed                           *
*     }                                                                       *
*                                                                             *
* Even if 'ts_bspline_to_beziers' fails, the state of 'in' has not been       *
* changed allowing you to handle the error properly.                          *
*                                                                             *
* Unless stated otherwise, the order of the parameters for transformation     *
* functions is:                                                               *
*                                                                             *
*     function(input, [additional_input], output, [additional_output])        *
*                                                                             *
* 'additional_input' are parameters required to calculate the actual          *
* transformation. 'additional_output' are parameters storing further result.  *
*                                                                             *
* Note: None of TinySpline's transformation functions frees the memory of the *
*       output parameter. Thus, when using the same output parameter multiple *
*       times, make sure to free memory before each call. Otherwise, you will *
*       have a bad time with memory leaks:                                    *
*                                                                             *
*     tsBSpline in = ...                  // an arbitrary spline              *
*     tsBSpline out;                      // result of transformations        *
*                                                                             *
*     ts_bspline_to_beziers(&in, &out);   // first transformation             *
*     ...                                 // some code                        *
*     ts_bspline_free(&out);              // avoid memory leak.               *
*     ts_bspline_buckle(&in, &out);       // next transformation              *
*                                                                             *
* If you want to modify your input directly without having a separate output, *
* pass it as input and output at once:                                        *
*                                                                             *
*     tsBSpline s = ...                       // an arbitrary spline          *
*     tsReal *knots = ...                     // a knot vector                *
*                                                                             *
*     ts_bspline_set_knots(&s, knots, &s);    // copy 'knots' into 's'        *
*                                                                             *
* Note: If a transformation function fails *and* input != output, all fields  *
*       of the output parameter are set to 0/NULL. If input == output, your   *
*       input may have an invalid state in case of errors.                    *
*                                                                             *
******************************************************************************/
/**
 * Computes the derivative of \p spline, see:
 *
 *     http://www.cs.mtu.edu/~shene/COURSES/cs3621/NOTES/spline/B-spline/bspline-derv.html
 *
 * The derivative of a spline _s_ of degree _d_ with _m_ control points and
 * _n_ knots is another spline _s'_ of degree _d-1_ with _m-1_ control points
 * and _n-2_ knots, defined over _s_ as:
 *
 * \f{eqnarray*}{
 *   s'(u) &=& \sum_{i=0}^{n-1} N_{i+1,p-1}(u) *
 *      (P_{i+1} - P_{i}) * p / (u_{i+p+1}-u_{i+1}) \\
 *         &=& \sum_{i=1}^{n} N_{i,p-1}(u) *
 *      (P_{i} - P_{i-1}) * p / (u_{i+p}-u_{i})
 * \f}
 *
 * If _s_ has a clamped knot vector, it can be shown that:
 *
 * \f{eqnarray*}{
 *   s'(u) &=& \sum_{i=0}^{n-1} N_{i,p-1}(u) *
 *      (P_{i+1} - P_{i}) * p / (u_{i+p+1}-u_{i+1})
 * \f}
 *
 * where the multiplicity of the first and the last knot value _u_ is _p_
 * rather than _p+1_.
 *
 * On error, (and if \p spline != \p \_derivative\_) all values of
 * \p \_derivative\_ are set to 0/NULL.
 *
 * @param spline
 *  The spline to derive.
 * @param \_derivative\_
 *	The output parameter storing the derivative of \p spline.
 * @return TS_SUCCESS
 *  On success.
 * @return TS_UNDERIVABLE
 *  If \p spline->deg < 1, \p spline->n_ctrlp < 2, or the multiplicity of
 *  an internal knot of \p spline is greater than the degree of \p spline.
 *  NOTE: This will be fixed in the future.
 * @return TS_MALLOC
 *  If allocating memory failed.
 */
tsError ts_bspline_derive( const tsBSpline* spline, tsBSpline* _derivative_ );

/**
 * Creates a deep copy of \p spline (only if \p spline != \p \_result\_) and
 * copies the first \p spline->n_ctrlp * \p spline->dim values from \p ctrlp
 * to \p \_result\_->ctrlp using memmove. The behaviour of this function is
 * undefined, if the length of \p ctrlp is less than \p spline->n_ctrlp *
 * \p spline->dim.
 *
 * On error, (and if \p spline != \p \_result\_) all values of \p \_result\_
 * are set to 0/NULL.
 *
 * @param spline
 *  The spline to deep copy (if \p spline != \p \_result\_) and whose
 *  control points are replaced with \p ctrlp.
 * @param ctrlp
 *  The control points to copy to \p \_result\_->ctrlp.
 * @param \_result\_
 *  The output parameter storing the result of this function.
 * @return TS_SUCCESS
 *  On success.
 * @return TS_MALLOC
 *  If \p spline != \p \_result\_ and allocating memory failed.
 */
tsError ts_bspline_set_ctrlp( const tsBSpline* spline, const tsReal* ctrlp,
        tsBSpline* _result_ );

/**
 * Creates a deep copy of \p spline (only if \p spline != \p \_result\_) and
 * copies the the first \p spline->n_knots from \p knots to \p \_result\_
 * using memmove. The behaviour of this function is undefined, if the length
 * of \p knots is less than \p spline->n_knots.
 *
 * On error, (and if \p spline != \p \_result\_) all values of \p \_result\_
 * are set to 0/NULL.
 *
 * @param spline
 *  The spline to deep copy (if \p spline != \p \_result\_) and whose
 *  knots are replaced with \p knots.
 * @param knots
 *  The knots to copy to \p \_result\_->knots.
 * @param \_result\_
 *  The output parameter storing the result of this function.
 * @return TS_SUCCESS
 *  On success.
 * @return TS_MALLOC
 *  If \p spline != \p \_result\_ and allocating memory failed.
 */
tsError ts_bspline_set_knots( const tsBSpline* spline, const tsReal* knots,
        tsBSpline* _result_ );

/**
 * Fills the knot vector of \p spline according to \p type with minimum knot
 * value \p min to maximum knot value \p max and stores the result in
 * \p \_result\_. Creates a deep copy of \p spline, if
 * \p spline != \p \_result\_.
 *
 * On error, (and if \p spline != \p \_result\_) all values of \p \_result\_
 * are set to 0/NULL.
 *
 * @param spline
 *  The spline to deep copy (if \p spline != \p \_result\_) and whose knot
 *  vector is filled according to \p type with minimum knot value \p min
 *  and maximum knot value \p max.
 * @param type
 *  How to fill the knot vector of \p \_result\_.
 * @param min
 *  The minimum knot value of the knot vector of \p \_result\_.
 * @param max
 *  The maximum knot value of the knot vector of \p \_result\_.
 * @param \_result\_
 *  The output parameter storing the result of this function.
 * @return TS_SUCCESS
 *  On success.
 * @return TS_DEG_GE_NCTRLP
 *  If \p spline->n_knots < 2*(\p original->deg+1). We can reuse this
 *  error code because \p spline->n_knots < 2*(\p spline->deg+1) implies
 *  \p spline->deg >= \p spline->n_ctrlp. Furthermore, using
 *  TS_DEG_GE_NCTRLP instead of TS_NUM_KNOTS ensures that TS_NUM_KNOTS is
 *  not used twice for this function. To be more fail-safe,
 *  \p spline->deg+1 instead of \p spline->order is used, to make sure
 *  that \p spline->deg+1 >= 1.
 * @return TS_NUM_KNOTS
 *  If \p type == TS_BEZIERS and
 *  \p spline->n_knots % \p spline->order != 0.
 * @return TS_KNOTS_DECR
 *  If \p min >= \p max. (::ts_fequals is used to determine whether
 *  \p min == \p max).
 * @return TS_MALLOC
 *  If \p spline != \p \_result\_ and allocating memory failed.
 */
tsError ts_bspline_fill_knots( const tsBSpline* spline, tsBSplineType type,
        tsReal min, tsReal max, tsBSpline* _result_ );

/**
 * Inserts the knot value \p u \p n times into \p spline and stores the result
 * in \p \_result\_. Creates a deep copy of \p spline, if
 * \p spline != \p \_result\_.
 *
 * On error, (and if \p spline != \p \_result\_) all values of \p \_result\_
 * are set to 0/NULL.
 *
 * @param spline
 *  The spline to deep copy (if \p spline != \p \_result\_) and whose knot
 *  vector is extended with \p u \p n times.
 * @param u
 *  The knot value to insert.
 * @param n
 *  How many times \p u should be inserted.
 * @param \_result\_
 *  The output parameter storing the updated knot vector.
 * @param \_k\_
 *  The output parameter storing the last index of \p u in \p \_result\_.
 * @return TS_SUCCESS
 *  On success.
 * @return TS_MALLOC
 *  If \p spline != \p \_result\_ and allocating memory failed.
 */
tsError ts_bspline_insert_knot( const tsBSpline* spline, tsReal u, size_t n,
        tsBSpline* _result_, size_t* _k_ );

/**
 * Resizes \p spline by \p n (number of control points) and stores the result
 * in \p \_resized\_. Creates a deep copy of \p spline, if
 * \p spline != \p \_result\_. If \p back != 0 \p spline is resized at the
 * end. If \p back == 0 \p spline is resized at front.
 *
 * On error, (and if \p spline != \p \_result\_) all values of \p \_result\_
 * are set to 0/NULL.
 *
 * @return TS_SUCCESS
 *  On success.
 * @return TS_DEG_GE_NCTRLP
 *  If the degree of \p \_resized\_ would be >= the number of the control
 *  points of \p \_resized\_.
 * @return TS_DIM_ZERO
 *  If \p spline != \p \_result\_ and \p spline->dim == 0.
 * @return TS_DEG_GE_NCTRLP
 *  If \p spline != \p \_result\_ and
 *  \p spline->deg >= \p spline->n_ctrlp.
 * @return TS_MALLOC
 *  If \p spline != \p \_result\_ and allocating memory failed.
 */
tsError ts_bspline_resize( const tsBSpline* spline, int n, int back,
        tsBSpline* _resized_ );

/**
 * Splits \p spline at \p u and stores the result in \p \_split\_. That is,
 * \p u is inserted _n_ times such that s(\p u) == \p \_split\_->order.
 * Creates a deep copy of \p spline, if \p spline != \p \_split\_.
 *
 * On error, (and if \p spline != \p \_split\_) all values of \p \_split\_
 * are set to 0/NULL.
 *
 * @param spline
 *  The spline to deep copy (if \p spline != \p \_result\_) and split.
 * @param u
 *  The split point.
 * @param \_split\_
 *  The output parameter storing the split spline.
 * @param \_k\_
 *  The output parameter storing the last index of \p u in \p \_split\_.
 * @return TS_SUCCESS
 *  On success.
 * @return TS_MALLOC
 *  If \p spline != \p \_split\_ and allocating memory failed.
 */
tsError ts_bspline_split( const tsBSpline* spline, tsReal u,
        tsBSpline* _split_, size_t* _k_ );

/**
 * Buckles \p spline by \p b and stores the result in \p \_buckled\_. Creates
 * a deep copy of \p spline, if \p spline != \p \_buckled\_.
 *
 * This function is based on:
 *
 *      Holten, Danny. "Hierarchical edge bundles: Visualization of adjacency
 *      relations in hierarchical data." Visualization and Computer Graphics,
 *      IEEE Transactions on 12.5 (2006): 741-748.
 *
 * Holten calls it "straightening" (page 744, equation 1).
 *
 * Usually, the range of \p b is: 0.0 <= \p b <= 1.0 with 0 yielding to a line
 * connecting the first and the last control point (no buckle) and 1 keeping
 * the original shape (maximum buckle). If \b < 0 or \b > 1 the behaviour is
 * undefined, though, it will not result in an error.
 *
 * On error, (and if \p spline != \p \_buckled\_) all values of \p \_buckled\_
 * are set to 0/NULL.
 *
 * @param spline
 *  The spline to buckle by \p b.
 * @param b
 *  The buckle factor (usually 0.0 <= \p b <= 1.0).
 * @param \_buckled\_
 *  The output parameter storing the buckled spline.
 * @return TS_SUCCESS
 *  On success.
 * @return TS_MALLOC
 *  If \p spline != \p \_buckled\_ and allocating memory failed.
 */
tsError ts_bspline_buckle( const tsBSpline* original, tsReal b,
        tsBSpline* _buckled_ );

/**
 * Subdivides \p spline into a sequence of Bezier curvs by splitting it at
 * each internal knot value. Creates a deep copy of \p spline, if
 * \p spline != \p \_beziers\_.
 *
 * On error, (and if \p spline != \p \_beziers\_) all values of \p \_beziers\_
 * are set to 0/NULL.
 *
 * @param spline
 *  The spline to subdivide into a sequence of Bezier curves.
 * @param \_beziers\_
 *  The output parameter storing the sequence of Bezier curves.
 * @return TS_SUCCESS
 *  On success.
 * @return TS_MALLOC
 *  If \p spline != \p \_beizers\_ and allocating memory failed.
 */
tsError ts_bspline_to_beziers( const tsBSpline* spline, tsBSpline* _beziers_ );


/******************************************************************************
*                                                                             *
* Utility Functions                                                           *
*                                                                             *
* The following section contains utility functions used by TinySpline which   *
* also may be helpful when working with this library.                         *
*                                                                             *
******************************************************************************/
/**
 * Compares the tsReal values \p x and \p y using an absolute and relative
 * epsilon environment.
 *
 * @param x
 *  The x value to compare.
 * @param y
 *  The y value to compare.
 * @return 1
 *  If \p x is equals to \p y.
 * @return 0
 *  Otherwise.
 */
int ts_fequals( tsReal x, tsReal y );

/**
 * Returns the error message associated to \p err. Returns "unknown error" if
 * \p err is no associated (indicating a bug) or is TS_SUCCESS (which is not
 * an actual error).
 */
const char* ts_enum_str( tsError err );

/**
 * Returns the error code associated to \p str or TS_SUCCESS if \p str is not
 * associated. Keep in mind that by concept "unknown error" is not associated,
 * though, TS_SUCCESS is returned.
 */
tsError ts_str_enum( const char* str );

/**
 * Fills the given array \p arr with \p val from \p arr+0 to \p arr+ \p num
 * (exclusive).
 */
void ts_arr_fill( tsReal* arr, size_t num, tsReal val );

/**
 * Returns the euclidean distance of \p x and \p y consisting of \p dim
 * components, respectively.
 *
 * @param x
 *  The x value.
 * @param y
 *  The y value.
 * @param dim
 *  The dimension of \p x and \p y.
 * @return
 *  The euclidean distanc of \p x and \p y.
 */
tsReal ts_ctrlp_dist2( const tsReal* x, const tsReal* y, size_t dim );


#ifdef  __cplusplus
}
#endif

#endif /* TINYSPLINE_H */
