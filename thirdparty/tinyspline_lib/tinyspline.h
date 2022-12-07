/** @file */

#ifndef TINYSPLINE_H
#define TINYSPLINE_H

#include <stddef.h>



/*! @name Deprecation
 *
 * The macro \c TS_DEPRECATED can be used to mark functions as
 * deprecated.
 *
 * @{
 */
#if defined(__GNUC__) || defined(__clang__)
#define TS_DEPRECATED __attribute__((deprecated))
#elif defined(_MSC_VER)
#define TS_DEPRECATED __declspec(deprecated)
#elif defined(SWIG)
#define TS_DEPRECATED
#else
#warning "WARNING: TS_DEPRECATED is not supported by the compiler"
#define TS_DEPRECATED
#endif
/*! @} */



/*! @name Library Export/Import
 *
 * If TinySpline is built for Windows, the macros \c TINYSPLINE_SHARED_EXPORT
 * and \c TINYSPLINE_SHARED_IMPORT define the Microsoft specific directives \c
 * __declspec(dllexport) and \c __declspec(dllimport), respectively. More
 * information on these directives can be found at:
 *
 *     https://docs.microsoft.com/en-us/cpp/cpp/dllexport-dllimport
 *
 * If TinySpline is built to the ELF (most Unix like environments) or Mach (OS
 * X) object format, \c TINYSPLINE_SHARED_EXPORT defines the directive
 * <tt>__attribute__ ((visibility ("default")))</tt> which, in combination with
 * \c -fvisibility=hidden, behaves similar to \c __declspec(dllexport). \c
 * TINYSPLINE_SHARED_IMPORT is set empty (i.e., it defines nothing).
 *
 * If none of the above applies, \c TINYSPLINE_SHARED_EXPORT and \c
 * TINYSPLINE_SHARED_IMPORT are set empty (i.e., they define nothing).
 *
 * Depending on whether TinySpline is compiled <em>as shared library</em> or
 * linked against <em>as shared library</em>, \c TINYSPLINE_API points to \c
 * TINYSPLINE_SHARED_EXPORT (compiled) or \c TINYSPLINE_SHARED_IMPORT (linked
 * against). All elements of TinySpline that needs to be exported/imported are
 * annotated with \c TINYSPLINE_API. This eliminates the need for a
 * module-definition (.def) file. If TinySpline is compiled or linked against
 * <em>as static library</em>, \c TINYSPLINE_API is set empty (i.e., it defines
 * nothing).
 *
 * If you consume TinySpline as shared library built for Windows, all you need
 * is to define \c TINYSPLINE_SHARED. This will automatically import all
 * required symbols. When compiling TinySpline, the build system should set all
 * necessary defines.
 *
 * @{
 */
#if defined(_WIN32) || defined(__CYGWIN__)
#define TINYSPLINE_SHARED_EXPORT __declspec(dllexport)
#define TINYSPLINE_SHARED_IMPORT __declspec(dllimport)
#elif defined(__ELF__) || defined(__MACH__)
#define TINYSPLINE_SHARED_EXPORT __attribute__ ((visibility ("default")))
#define TINYSPLINE_SHARED_IMPORT
#else
#define TINYSPLINE_SHARED_EXPORT
#define TINYSPLINE_SHARED_IMPORT
#endif

#ifdef TINYSPLINE_SHARED
#ifdef TINYSPLINE_EXPORT
#define TINYSPLINE_API TINYSPLINE_SHARED_EXPORT
#else
#define TINYSPLINE_API TINYSPLINE_SHARED_IMPORT
#endif
#else
#define TINYSPLINE_API
#endif

#ifdef	__cplusplus
extern "C" {
#endif
/*! @} */



/*! @name Predefined Constants
 *
 * The following constants have been adjusted to maintain internal consistency
 * and should only be changed with great caution! The values chosen should be
 * suitable for most environments and can be used with float (single) and
 * double precision (see ::tsReal). If changes are necessary, please read the
 * documentation of the constants in advance.
 *
 * @{
 */
/**
 * The mathematical constant pi.
 */
#define TS_PI 3.14159265358979323846

/**
 * The maximum number of knots a spline can have. This constant is strongly
 * related to ::TS_KNOT_EPSILON in that the larger ::TS_MAX_NUM_KNOTS is, the
 * less precise ::TS_KNOT_EPSILON has to be (i.e., knots with greater distance
 * are considered equal). Likewise, the more precise ::TS_KNOT_EPSILON is
 * (i.e., knots with smaller distance are considered equal), the less
 * ::TS_MAX_NUM_KNOTS has to be. By default, the relation between
 * ::TS_MAX_NUM_KNOTS and ::TS_KNOT_EPSILON is as follows:
 *
 *     TS_MAX_NUM_KNOTS = 1 / TS_KNOTS_EPSILON
 */
#define TS_MAX_NUM_KNOTS 10000

/**
 * The minimum of the domain of newly created splines. Must be less than
 * ::TS_DOMAIN_DEFAULT_MAX. This constant is used only when creating new
 * splines. After creation, the domain of a spline can be adjusted as needed.
 */
#define TS_DOMAIN_DEFAULT_MIN 0.0f

/**
 * The maximum of the domain of newly created splines. Must be greater than
 * ::TS_DOMAIN_DEFAULT_MIN. This constant is used only when creating new
 * splines. After creation, the domain of a spline can be adjusted as needed.
 */
#define TS_DOMAIN_DEFAULT_MAX 1.0f

/**
 * If the distance between two knots falls below this threshold, they are
 * considered equal. Must be positive ( > 0 ). This constant is strongly
 * related to ::TS_MAX_NUM_KNOTS in that the more precise ::TS_KNOT_EPSILON is
 * (i.e., knots with smaller distance are considered equal), the less
 * ::TS_MAX_NUM_KNOTS has to be. Likewise, the larger ::TS_MAX_NUM_KNOTS is,
 * the less precise ::TS_KNOT_EPSILON has to be (i.e., knots with greater
 * distance are considered equal). By default, the relation between
 * ::TS_KNOT_EPSILON and ::TS_MAX_NUM_KNOTS is as follows:
 *
 *     TS_KNOT_EPSILON = 1 / TS_MAX_NUM_KNOTS
 *
 * It is recommended that ::TS_KNOT_EPSILON is aligned to the span of
 * ::TS_DOMAIN_DEFAULT_MIN and ::TS_DOMAIN_DEFAULT_MAX. That is, adjacent
 * floating point values in the domain [::TS_DOMAIN_DEFAULT_MIN,
 * ::TS_DOMAIN_DEFAULT_MAX] should not be equal according to
 * ::TS_KNOT_EPSILON. This is in particular recommended when ::TS_KNOT_EPSILON
 * and ::TS_MAX_NUM_KNOTS are related to each other as described above.
 */
#define TS_KNOT_EPSILON 1e-4f

/**
 * If the distance between two (control) points is less than or equal to this
 * threshold, they are considered equal. This constant is not used directly by
 * the C interface. Rather, it serves as a viable default value for functions
 * requiring an epsilon environment to decide whether two (control) points are
 * equal or not. The C++ interface, for example, uses this as default value for
 * optional parameters.
 */
#ifdef TINYSPLINE_FLOAT_PRECISION
#define TS_POINT_EPSILON 1e-3f
#else
#define TS_POINT_EPSILON 1e-5f
#endif

/**
 * If the length of an element (e.g., a vector) is less than this threshold,
 * the length is considered \c 0. Must be positive ( > 0 ).
 */
#ifdef TINYSPLINE_FLOAT_PRECISION
#define TS_LENGTH_ZERO 1e-3f
#else
#define TS_LENGTH_ZERO 1e-4f
#endif
/*! @} */



/*! @name API Configuration
 *
 * In the following section, different aspects of TinySpline's API can be
 * configured (compile-time). It is recommended to configure the API by
 * supplying the corresponding preprocessor definition(s). That said, there is
 * nothing wrong with editing the source code directly.
 *
 * @{
 */
/**
 * TinySpline uses its own typedef for floating point numbers. Supported are
 * floats (single precision) and doubles (double precision). By default,
 * doubles are used. Note that this typedef affects the entire API (i.e., types
 * are not mixed; all structs and functions rely only on tsReal). Float
 * precision is primarily used in combination with GLUT because GLUT's API
 * doesn't support doubles:
 *
 *     https://www.glprogramming.com/red/chapter12.html
 *
 * Generally, double precision is the right choice. Floats are mainly supported
 * for legacy reasons. Yet, floats are not considered deprecated! If necessary,
 * tsReal can also be typedefed to any other floating point representation. In
 * this case, make sure to adjust TS_MAX_NUM_KNOTS and TS_KNOT_EPSILON
 * (cf. Section "Predefined Constants").
 */
#ifdef TINYSPLINE_FLOAT_PRECISION
typedef float tsReal;
#else
typedef double tsReal;
#endif
/*! @} */



/*! @name Error Handling
 *
 * There are three types of error handling in TinySpline.
 *
 * 1. Return value: Functions that can fail return a special error code
 * (::tsError). If the error code is not \c 0 (::TS_SUCCESS), an error occurred
 * during execution. For example:
 *
 *     if ( ts_bspline_to_beziers(&spline, &beziers, NULL) ) {
 *          ... An error occurred ...
 *     }
 *
 *   It is of course possible to check the actual type of error:
 *
 *       tsError error = ts_bspline_to_beziers(&spline, &beziers, NULL);
 *       if (error == TS_MALLOC) {
 *            ... Out of memory ...
 *       } else if (error == ...
 *
 *   This type of error handling is used in many C programs. The disadvantage
 *   is that there is no additional error message besides the error code (with
 *   which the cause of an error could be specified in more detail). Some
 *   libraries make do with global variables in which error messages are stored
 *   for later purpose (e.g., \a errno and \a strerror). Unfortunately,
 *   however, this approach (by design) is often not thread-safe. The second
 *   error handling option solves this issue.
 *
 * 2. ::tsStatus objects: Functions that can fail do not only return an error
 * code, but also take a pointer to a ::tsStatus object as an optional
 * parameter. In the event of an error, and if the supplied pointer is not
 * NULL, the error message is stored in tsStatus#message and can be accessed by
 * the caller. Using a ::tsStatus object, the example given in 1. can be
 * modified as follows:
 *
 *     tsStatus status;
 *     if ( ts_bspline_to_beziers(&spline, &beziers, &status) ) {
 *         status.code; // error code
 *         status.message; // error message
 *     }
 *
 *   Note that ::tsStatus objects can be reused:
 *
 *       tsStatus status;
 *       if ( ts_bspline_to_beziers(&spline, &beziers, &status) ) {
 *           ...
 *       }
 *       ...
 *       if ( ts_bspline_derive(&beziers, 1, 0.001, &beziers, &status) ) {
 *           ...
 *       }
 *
 *   If you would like to use this type of error handling in your own functions
 *   (in particular the optional ::tsStatus parameter), you may wonder whether
 *   there is an easy way to return error codes and format error messages. This
 *   is where the macros ::TS_RETURN_0 -- ::TS_RETURN_4 come into play. They
 *   can, for example, be used as follows:
 *
 *       tsError my_function(..., tsStatus *status, ...)
 *       {
 *           ...
 *           tsReal *points = (tsReal *) malloc(len * sizeof(tsReal));
 *           if (!points)
 *               TS_RETURN_0(status, TS_MALLOC, "out of memory")
 *           ...
 *       }
 *
 *   The \c N in \c TS_RETURN_<N> denotes the number of format specifier in the
 *   supplied format string (cf. sprintf(char *, const char *, ... )).
 *
 * 3. Try-catch-finally blocks: TinySpline provides a set of macros that can be
 * used when a complex control flow is necessary. The macros create a structure
 * that is similar to the exception handling mechanisms of high-level languages
 * such as C++. The basic structure is as follows:
 *
 *     TS_TRY(try, error, status) // `status' may be NULL
 *         ...
 *         TS_CALL( try, error, ts_bspline_to_beziers(
 *             &spline, &beziers, status) )
 *         ...
 *     TS_CATCH(error)
 *         ... Executed in case of an error ...
 *         ... `error' (tsError) indicates the type of error.
 *         ... `status' (tsStatus) contains the error code and message ...
 *     TS_FINALLY
 *         ... Executed in any case ...
 *     TS_END_TRY
 *
 *   ::TS_TRY and ::TS_END_TRY mark the boundaries of a try-catch-finally
 *   block. Every block has an identifier (name) that must be unique within a
 *   scope. The name of a block is set via the first argument of ::TS_TRY (\c
 *   try in the example listed above). The control flow of a try-catch-finally
 *   block is directed via the second and third argument of ::TS_TRY (\c error
 *   and \c status in the example listed above) and the utility macro
 *   ::TS_CALL. The second argument of ::TS_TRY, a ::tsError, is mandatory. The
 *   third argument of ::TS_TRY, a ::tsStatus object, is optional, that is, it
 *   may be \c NULL. ::TS_CALL serves as a wrapper for functions with return
 *   type ::tsError. If the called functions fails (more on that later),
 *   ::TS_CALL immediately jumps into the ::TS_CATCH section where \c error and
 *   \c status can be evaluated as needed (note that \c status may be \c
 *   NULL). The ::TS_FINALLY section is executed in any case and is in
 *   particularly helpful when resources (such as heap-memory, file-handles
 *   etc.) must be released.
 *
 *   While ::TS_CALL can be used to wrap functions with return type ::tsError,
 *   sooner or later it will be necessary to delegate the failure of other
 *   kinds of functions (i.e., functions outside of TinySpline; e.g.,
 *   malloc(size_t)). This is the purpose of the ::TS_THROW_0 -- ::TS_THROW_4
 *   macros. It is not by coincidence that the signature of the \c TS_THROW_<N>
 *   macros is quite similar to that of the \c TS_RETURN_<N> macros. Both
 *   "macro groups" are used to report errors. The difference between \c
 *   TS_RETURN_<N> and TS_THROW_<N>, however, is that the former exits a
 *   function (i.e., a \c return statement is inserted by these macros) while
 *   the latter jumps into a catch block (the catch block to jump into is set
 *   via the first argument of \c TS_THROW_<N>):
 *
 *       tsBSpline spline = ts_bspline_init();
 *       tsReal *points = NULL;
 *       TS_TRY(try, error, status)
 *           ...
 *           tsReal *points = (tsReal *) malloc(len * sizeof(tsReal));
 *           if (!points)
 *               TS_THROW_0(try, status, TS_MALLOC, "out of memory")
 *           ...
 *           TS_CALL( try, error, ts_bspline_interpolate_cubic_natural(
 *               points, len / dim, dim, &spline, status) )
 *           ...
 *       TS_CATCH(error)
 *           ... Log error message ...
 *       TS_FINALLY
 *           ts_bspline_free(&spline);
 *           if (points)
 *               free(points);
 *       TS_END_TRY
 *
 *   In all likelihood, you are already familiar with this kind error
 *   handling. Actually, there are a plethora of examples available online
 *   showing how exception-like error handling can be implemented in C. What
 *   most of these examples have in common is that they suggest to wrap the
 *   functions \c setjmp and \c longjmp (see setjmp.h) with macros. While this
 *   undoubtedly is a clever trick, \c setjmp and \c longjmp have no viable
 *   (i.e, thread-safe) solution for propagating the cause of an error (in the
 *   form of a human-readable error message) back to the client of a
 *   library. Therefore, TinySpline implements try-catch-finally blocks with \c
 *   if statements, labels, and \c goto statements (TS_THROW_<N>).
 *
 *   ::TS_TRY is flexible enough to be used in functions that are in turn
 *   embedded into TinySpline's error handling system:
 *
 *       tsError my_function(..., tsStatus *status, ...)
 *       {
 *           tsError error;
 *           TS_TRY(try, error, status)
 *               ...
 *           TS_END_TRY
 *           return error;
 *       }
 *
 *   as well as functions forming the root of a call stack that uses
 *   TinySpline's error handling system:
 *
 *       tsStatus status;
 *       TS_TRY(try, status.code, &status)
 *           ...
 *       TS_END_TRY
 *
 *   There is some utility macros that might be useful when dealing with
 *   try-catch-finally blocks:
 *
 *   - ::TS_END_TRY_RETURN: Returns the supplied error code immediately after
 *     completing the corresponding block. Can be used as follows:
 *
 *         tsError my_function(..., tsStatus *status, ...)
 *         {
 *             tsError error;
 *             TS_TRY(try, error, status)
 *                 ...
 *             TS_END_TRY_RETURN(error)
 *         }
 *
 *   - ::TS_END_TRY_ROE: Like ::TS_END_TRY_RETURN but returns the supplied
 *     error code, \c e, if \c e is not ::TS_SUCCESS (\c ROE means
 *     <b>R</b>eturn <b>O</b>n <b>E</b>rror). Can be used as follows:
 *
 *         tsError my_function(..., tsStatus *status, ...)
 *         {
 *             tsError error;
 *             TS_TRY(try, error, status)
 *                 ...
 *             TS_END_TRY_ROE(error)
 *             ... Additional code. The code is executed only if `error' is
 *                 TS_SUCCESS, that is, if no error occurred in the try block
 *                 above ...
 *         }
 *
 *   - ::TS_CALL_ROE: Calls the supplied function and returns its error code,
 *     \c e, if \c e is not ::TS_SUCCESS. This macro can be seen as a \e
 *     minified try block (a dedicated try block is not needed).
 *
 *   - ::TS_RETURN_SUCCESS: Shortcut for ::TS_RETURN_0 with error code
 *     ::TS_SUCCESS and an empty error message.
 *
 * @{
 */
/**
 * Defines different error codes.
 */
typedef enum
{
	/** No error. */
	TS_SUCCESS = 0,

	/** Memory cannot be allocated (malloc, realloc etc.). */
	TS_MALLOC = -1,

	/** Points have dimensionality 0. */
	TS_DIM_ZERO = -2,

	/** degree >= num(control_points). */
	TS_DEG_GE_NCTRLP = -3,

	/** Knot is not within the domain. */
	TS_U_UNDEFINED = -4,

	/** multiplicity(knot) > order */
	TS_MULTIPLICITY = -5,

	/** Decreasing knot vector. */
	TS_KNOTS_DECR = -6,

	/** Unexpected number of knots. */
	TS_NUM_KNOTS = -7,

	/** Spline is not derivable. */
	TS_UNDERIVABLE = -8,

	/** len(control_points) % dimension != 0. */
	TS_LCTRLP_DIM_MISMATCH = -10,

	/** Error while reading/writing a file. */
	TS_IO_ERROR = -11,

	/** Error while parsing a serialized entity. */
	TS_PARSE_ERROR = -12,

	/** Index does not exist (e.g., when accessing an array). */
	TS_INDEX_ERROR = -13,

	/** Function returns without result (e.g., approximations). */
	TS_NO_RESULT = -14,

	/** Unexpected number of points. */
	TS_NUM_POINTS = -15
} tsError;

/**
 * Stores an error code (see ::tsError) with corresponding message.
 */
typedef struct
{
	/** The error code. */
	tsError code;

	/**
	 * The corresponding error message (encoded as C string). Memory is
	 * allocated on stack so as to be able to provide a meaningful message
	 * in the event of memory issues (cf. ::TS_MALLOC).
	 */
	char message[100];
} tsStatus;

#define TS_TRY(label, error, status)         \
{                                            \
	(error) = TS_SUCCESS;                \
	if ((status) != NULL) {              \
		(status)->code = TS_SUCCESS; \
		(status)->message[0] = '\0'; \
	}                                    \
	__ ## label ## __:                   \
	if (!(error)) {

#define TS_CALL(label, error, call)                  \
		(error) = (call);                    \
		if ((error)) goto __ ## label ## __;

#define TS_CATCH(error) \
	} if ((error)) {

#define TS_FINALLY \
	} {

#define TS_END_TRY \
	}          \
}

#define TS_END_TRY_RETURN(error)   \
	TS_END_TRY return (error);

#define TS_END_TRY_ROE(error)                 \
	TS_END_TRY if ((error)) return error;

#define TS_CALL_ROE(error, call)   \
{                                  \
	(error) = (call);          \
	if ((error)) return error; \
}

#define TS_RETURN_SUCCESS(status)            \
{                                            \
	if ((status) != NULL) {              \
		(status)->code = TS_SUCCESS; \
		(status)->message[0] = '\0'; \
	}                                    \
	return TS_SUCCESS;                   \
}

#define TS_RETURN_0(status, error, msg)          \
{                                                \
	if ((status) != NULL) {                  \
		(status)->code = error;          \
		sprintf((status)->message, msg); \
	}                                        \
	return error;                            \
}

#define TS_RETURN_1(status, error, msg, arg1)          \
{                                                      \
	if ((status) != NULL) {                        \
		(status)->code = error;                \
		sprintf((status)->message, msg, arg1); \
	}                                              \
	return error;                                  \
}

#define TS_RETURN_2(status, error, msg, arg1, arg2)          \
{                                                            \
	if ((status) != NULL) {                              \
		(status)->code = error;                      \
		sprintf((status)->message, msg, arg1, arg2); \
	}                                                    \
	return error;                                        \
}

#define TS_RETURN_3(status, error, msg, arg1, arg2, arg3)          \
{                                                                  \
	if ((status) != NULL) {                                    \
		(status)->code = error;                            \
		sprintf((status)->message, msg, arg1, arg2, arg3); \
	}                                                          \
	return error;                                              \
}

#define TS_RETURN_4(status, error, msg, arg1, arg2, arg3, arg4)          \
{                                                                        \
	if ((status) != NULL) {                                          \
		(status)->code = error;                                  \
		sprintf((status)->message, msg, arg1, arg2, arg3, arg4); \
	}                                                                \
	return error;                                                    \
}

#define TS_THROW_0(label, error, status, val, msg) \
{                                                  \
	(error) = val;                             \
	if ((status) != NULL) {                    \
		(status)->code = val;              \
		sprintf((status)->message, msg);   \
	}                                          \
	goto __ ## label ## __;                    \
}

#define TS_THROW_1(label, error, status, val, msg, arg1) \
{                                                        \
	(error) = val;                                   \
	if ((status) != NULL) {                          \
		(status)->code = val;                    \
		sprintf((status)->message, msg, arg1);   \
	}                                                \
	goto __ ## label ## __;                          \
}

#define TS_THROW_2(label, error, status, val, msg, arg1, arg2) \
{                                                              \
	(error) = val;                                         \
	if ((status) != NULL) {                                \
		(status)->code = val;                          \
		sprintf((status)->message, msg, arg1, arg2);   \
	}                                                      \
	goto __ ## label ## __;                                \
}

#define TS_THROW_3(label, error, status, val, msg, arg1, arg2, arg3) \
{                                                                    \
	(error) = val;                                               \
	if ((status) != NULL) {                                      \
		(status)->code = val;                                \
		sprintf((status)->message, msg, arg1, arg2, arg3);   \
	}                                                            \
	goto __ ## label ## __;                                      \
}

#define TS_THROW_4(label, error, status, val, msg, arg1, arg2, arg3, arg4) \
{                                                                          \
	(error) = val;                                                     \
	if ((status) != NULL) {                                            \
		(status)->code = val;                                      \
		sprintf((status)->message, msg, arg1, arg2, arg3, arg4);   \
	}                                                                  \
	goto __ ## label ## __;                                            \
}
/*! @} */



/*! @name Utility Structs and Enums
 *
 * @{
 */
/**
 * Describes the structure of the knot vector. More details can be found at:
 *
 *     www.cs.mtu.edu/~shene/COURSES/cs3621/NOTES/spline/B-spline/bspline-curve.html
 */
typedef enum
{
	/** Uniformly spaced knot vector with opened end knots. */
	TS_OPENED = 0,

	/** Uniformly spaced knot vector with clamped end knots. */
	TS_CLAMPED = 1,

	/**
	 * Uniformly spaced knot vector where the multiplicity of each knot is
	 * equal to the order of the spline.
	 */
	TS_BEZIERS = 2
} tsBSplineType;

/**
 * A three-dimensional TNB-vector with position. More details can be found at:
 *
 *     https://en.wikipedia.org/wiki/Frenet-Serret_formulas
 *     https://www.math.tamu.edu/~tkiffe/calc3/tnb/tnb.html
 *
 * TNB stands for \e tangent, \e normal, and \e binormal.
 */
typedef struct
{
	/** Position of the TNB-vector. */
	tsReal position[3];

	/** Tangent of the TNB-vector. */
	tsReal tangent[3];

	/** Normal of the TNB-vector. */
	tsReal normal[3];

	/** Binormal of the TNB-vector. */
	tsReal binormal[3];
} tsFrame;
/*! @} */



/*! @name B-Spline Data
 *
 * The internal state of ::tsBSpline is protected using the PIMPL design
 * pattern (see https://en.cppreference.com/w/cpp/language/pimpl for more
 * details). The data of an instance can be accessed with the functions listed
 * in this section.
 *
 * @{
 */
/**
 * Represents a B-Spline, which may also be used for NURBS, Bezier curves,
 * lines, and points. NURBS use homogeneous coordinates to store their control
 * points (i.e. the last component of a control point stores the weight).
 * Bezier curves are B-Splines with 'num_control_points == order' and a
 * clamped knot vector, which lets them pass through their first and last
 * control point (a property which does not necessarily apply to B-Splines and
 * NURBS). Lines and points, on that basis, are Bezier curves of degree 1
 * (lines) and 0 (points).
 *
 * Two dimensional control points are stored as follows:
 *
 *     [x_0, y_0, x_1, y_1, ..., x_n-1, y_n-1]
 *
 * Tree dimensional control points are stored as follows:
 *
 *     [x_0, y_0, z_0, x_1, y_1, z_1, ..., x_n-1, y_n-1, z_n-1]
 *
 * ... and so on. As already mentioned, NURBS use homogeneous coordinates to
 * store their control points. For example, a NURBS in 2D stores its control
 * points as follows:
 *
 *     [x_0*w_0, y_0*w_0, w_0, x_1*w_1, y_1*w_1, w_1, ...]
 *
 * where 'w_i' is the weight of the i'th control point.
 */
typedef struct
{
	struct tsBSplineImpl *pImpl; /**< The actual implementation. */
} tsBSpline;

/**
 * Returns the degree of \p spline.
 *
 * @param[in] spline
 * 	The spline whose degree is read.
 * @return
 * 	The degree of \p spline.
 */
size_t TINYSPLINE_API
ts_bspline_degree(const tsBSpline *spline);

/**
 * Returns the order (degree + 1) of \p spline.
 *
 * @param[in] spline
 * 	The spline whose order is read.
 * @return
 * 	The order of \p spline.
 */
size_t TINYSPLINE_API
ts_bspline_order(const tsBSpline *spline);

/**
 * Returns the dimensionality of \p spline, that is, the number of components
 * of its control points (::ts_bspline_control_points). One-dimensional splines
 * are possible, albeit their benefit might be questionable.
 *
 * @param[in] spline
 * 	The spline whose dimension is read.
 * @return
 * 	The dimension of \p spline (>= 1).
 */
size_t TINYSPLINE_API
ts_bspline_dimension(const tsBSpline *spline);

/**
 * Returns the length of the control point array of \p spline.
 *
 * @param[in] spline
 * 	The spline whose length of the control point array is read.
 * @return
 * 	The length of the control point array of \p spline.
 */
size_t TINYSPLINE_API
ts_bspline_len_control_points(const tsBSpline *spline);

/**
 * Returns the number of control points of \p spline.
 *
 * @param[in] spline
 * 	The spline whose number of control points is read.
 * @return
 * 	The number of control points of \p spline.
 */
size_t TINYSPLINE_API
ts_bspline_num_control_points(const tsBSpline *spline);

/**
 * Returns the size of the control point array of \p spline. This function may
 * be useful when copying control points using \e memcpy or \e memmove.
 *
 * @param[in] spline
 * 	The spline whose size of the control point array is read.
 * @return
 * 	The size of the control point array of \p spline.
 */
size_t TINYSPLINE_API
ts_bspline_sof_control_points(const tsBSpline *spline);

/**
 * Returns the pointer to the control point array of \p spline. Note that the
 * return type of this function is \c const for a reason. Clients should only
 * read the returned array. When suppressing the constness and writing to the
 * array against better knowledge, the client is on its own with regard to the
 * consistency of the internal state of \p spline. If the control points of a
 * spline need to be changed, use ::ts_bspline_control_points to obtain a copy
 * of the control point array and ::ts_bspline_set_control_points to copy the
 * changed values back to the spline.
 *
 * @param[in] spline
 * 	The spline whose pointer to the control point array is returned.
 * @return
 * 	Pointer to the control point array of \p spline.
 */
const tsReal TINYSPLINE_API *
ts_bspline_control_points_ptr(const tsBSpline *spline);

/**
 * Returns a deep copy of the control points of \p spline.
 *
 * @param[in] spline
 * 	The spline whose control points are read.
 * @param[out] ctrlp
 * 	The output array. \b Note: It is the responsibility of the client to
 * 	release the allocated memory after use.
 * @param[out] status
 * 	The status of this function. May be NULL.
 * @return TS_SUCCESS
 * 	On success.
 * @return TS_MALLOC
 * 	If allocating memory failed.
 */
tsError TINYSPLINE_API
ts_bspline_control_points(const tsBSpline *spline,
                          tsReal **ctrlp,
                          tsStatus *status);

/**
 * Returns the pointer to the control point of \p spline at \p index. Note that
 * the type of the out parameter \p ctrlp is \c const for a reason. Clients
 * should only read the returned array. When suppressing the constness of \p
 * ctrlp and writing to the array against better knowledge, the client is on
 * its own with regard to the consistency of the internal state of \p
 * spline. If one of the control points of a spline needs to be changed, use
 * ::ts_bspline_set_control_points to copy the new control point to the spline.
 *
 * @param[in] spline
 * 	The spline whose pointer to the control point at \p index is returned.
 * @param[in] index
 * 	Zero-based index of the control point to be returned.
 * @param[out] ctrlp
 * 	Pointer to the control point of \p spline at \p index.
 * @param[out] status
 * 	The status of this function. May be NULL.
 * @return TS_SUCCESS
 * 	On success.
 * @return TS_INDEX_ERROR
 * 	If \p index is out of range.
 */
tsError TINYSPLINE_API
ts_bspline_control_point_at_ptr(const tsBSpline *spline,
                                size_t index,
                                const tsReal **ctrlp,
                                tsStatus *status);

/**
 * Sets the control points of \p spline. Creates a deep copy of \p ctrlp.
 *
 * @pre
 * 	\p ctrlp has length ::ts_bspline_len_control_points.
 * @param[out] spline
 * 	The spline whose control points are set.
 * @param[in] ctrlp
 * 	The value.
 * @param[out] status
 * 	The status of this function. May be NULL.
 * @return TS_SUCCESS
 * 	On success.
 */
tsError TINYSPLINE_API
ts_bspline_set_control_points(tsBSpline *spline,
                              const tsReal *ctrlp,
                              tsStatus *status);

/**
 * Sets the control point of \p spline at \p index. Creates a deep copy of
 * \p ctrlp.
 *
 * @pre
 * 	\p ctrlp has length ::ts_bspline_dimension.
 * @param[out] spline
 * 	The spline whose control point is set at \p index.
 * @param[in] index
 * 	Zero-based index of the control point to be set.
 * @param[in] ctrlp
 * 	The value.
 * @param[out] status
 * 	The status of this function. May be NULL.
 * @return TS_SUCCESS
 * 	On success.
 * @return TS_INDEX_ERROR
 * 	If \p index is out of range.
 */
tsError TINYSPLINE_API
ts_bspline_set_control_point_at(tsBSpline *spline,
                                size_t index,
                                const tsReal *ctrlp,
                                tsStatus *status);

/**
 * Returns the number of knots of \p spline.
 *
 * @param[in] spline
 * 	The spline whose number of knots is read.
 * @return
 * 	The number of knots of \p spline.
 */
size_t TINYSPLINE_API
ts_bspline_num_knots(const tsBSpline *spline);

/**
 * Returns the size of the knot array of \p spline. This function may be useful
 * when copying knots using \e memcpy or \e memmove.
 *
 * @param[in] spline
 * 	The spline whose size of the knot array is read.
 * @return TS_SUCCESS
 * 	The size of the knot array of \p spline.
 */
size_t TINYSPLINE_API
ts_bspline_sof_knots(const tsBSpline *spline);

/**
 * Returns the pointer to the knot vector of \p spline. Note that the return
 * type of this function is \c const for a reason. Clients should only read the
 * returned array. When suppressing the constness and writing to the array
 * against better knowledge, the client is on its own with regard to the
 * consistency of the internal state of \p spline. If the knot vector of a
 * spline needs to be changed, use ::ts_bspline_knots to obtain a copy of the
 * knot vector and ::ts_bspline_set_knots to copy the changed values back to
 * the spline.
 *
 * @param[in] spline
 * 	The spline whose pointer to the knot vector is returned.
 * @return
 * 	Pointer to the knot vector of \p spline.
 */
const tsReal TINYSPLINE_API *
ts_bspline_knots_ptr(const tsBSpline *spline);

/**
 * Returns a deep copy of the knots of \p spline.
 *
 * @param[in] spline
 * 	The spline whose knots are read.
 * @param[out] knots
 * 	The output array. \b Note: It is the responsibility of the client to
 * 	release the allocated memory after use.
 * @param[out] status
 * 	The status of this function. May be NULL.
 * @return TS_SUCCESS
 * 	On success.
 * @return TS_MALLOC
 * 	If allocating memory failed.
 */
tsError TINYSPLINE_API
ts_bspline_knots(const tsBSpline *spline,
                 tsReal **knots,
                 tsStatus *status);

/**
 * Returns the knot of \p spline at \p index.
 *
 * @param[in] spline
 * 	The spline whose knot is read at \p index.
 * @param[in] index
 * 	Zero-based index of the knot to be read.
 * @param[out] knot
 * 	The output value.
 * @param[out] status
 * 	The status of this function. May be NULL.
 * @return TS_SUCCESS
 * 	On success.
 * @return TS_INDEX_ERROR
 * 	If \p index is out of range.
 */
tsError TINYSPLINE_API
ts_bspline_knot_at(const tsBSpline *spline,
                   size_t index,
                   tsReal *knot,
                   tsStatus *status);

/**
 * Sets the knots of \p spline. Creates a deep copy of \p knots.
 *
 * @pre
 * 	\p knots has length ::ts_bspline_num_knots.
 * @param[out] spline
 * 	The spline whose knots are set.
 * @param[in] knots
 * 	The value.
 * @param[out] status
 * 	The status of this function. May be NULL.
 * @return TS_SUCCESS
 * 	On success.
 * @return TS_KNOTS_DECR
 * 	If the knot vector is decreasing.
 * @return TS_MULTIPLICITY
 * 	If there is a knot with multiplicity > order
 */
tsError TINYSPLINE_API
ts_bspline_set_knots(tsBSpline *spline,
                     const tsReal *knots,
                     tsStatus *status);

/**
 * Sets the knots of \p spline supplied as varargs. As all splines have at
 * least two knots, the first two knots have a named parameter. Note that, by
 * design of varargs in C, the last named parameter must not be float. Thus,
 * \p knot1 is of type double instead of ::tsReal.
 *
 * @pre
 * 	::ts_bspline_num_knots knots are supplied as varargs.
 * @param[out] spline
 *	The spline whose knots are set.
 * @param[out] status
 * 	The status of this function. May be NULL.
 * @param[in] knot0
 * 	The first knot.
 * @param[in] knot1
 * 	the second knot.
 * @param[in] ...
 * 	The remaining knots.
 * @return TS_SUCCESS
 * 	On success.
 * @return TS_MALLOC
 * 	If allocating memory failed.
 * @return TS_KNOTS_DECR
 * 	If the knot vector is decreasing.
 * @return TS_MULTIPLICITY
 * 	If there is a knot with multiplicity > order
 */
tsError TINYSPLINE_API
ts_bspline_set_knots_varargs(tsBSpline *spline,
                             tsStatus *status,
                             tsReal knot0,
                             double knot1,
                             ...);

/**
 * Sets the knot of \p spline at \p index.
 *
 * @param[in] spline
 * 	The spline whose knot is set at \p index.
 * @param[in] index
 * 	Zero-based index of the knot to be set.
 * @param[in] knot
 * 	The value.
 * @param[out] status
 * 	The status of this function. May be NULL.
 * @return TS_SUCCESS
 * 	On success.
 * @return TS_INDEX_ERROR
 * 	If \p index is out of range.
 * @return TS_KNOTS_DECR
 * 	If setting the knot at \p index results in a decreasing knot vector.
 * @return TS_MULTIPLICITY
 * 	If setting the knot at \p index results in a knot vector containing
 * 	\p knot with multiplicity greater than the order of \p spline.
 */
tsError TINYSPLINE_API
ts_bspline_set_knot_at(tsBSpline *spline,
                       size_t index,
                       tsReal knot,
                       tsStatus *status);
/*! @} */



/*! @name B-Spline Initialization
 *
 * The following functions are used to create and release ::tsBSpline instances
 * as well as to copy and move the internal data of a ::tsBSpline instance to
 * another instance.
 *
 * \b Note: It is recommended to initialize an instance with
 * ::ts_bspline_init. This way, ::ts_bspline_free can be called in ::TS_CATCH
 * and ::TS_FINALLY blocks (see Section Error Handling for more details)
 * without further checking. For example:
 *
 *     tsBSpline spline = ts_bspline_init();
 *     TS_TRY(...)
 *         ...
 *     TS_FINALLY
 *         ts_bspline_free(&spline);
 *     TS_END_TRY
 *
 * @{
 */
/**
 * Creates a new spline whose data points to NULL.
 *
 * @return
 * 	A new spline whose data points to NULL.
 */
tsBSpline TINYSPLINE_API
ts_bspline_init(void);

/**
 * Creates a new spline and stores the result in \p spline.
 *
 * @param[in] num_control_points
 * 	The number of control points of \p spline.
 * @param[in] dimension
 * 	The dimension of the control points of \p spline.
 * @param[in] degree
 * 	The degree of \p spline.
 * @param[in] type
 * 	How to setup the knot vector of \p spline.
 * @param[out] spline
 * 	The output spline.
 * @param[out] status
 * 	The status of this function. May be NULL.
 * @return TS_SUCCESS
 * 	On success.
 * @return TS_DIM_ZERO
 * 	If \p dimension is \c 0.
 * @return TS_DEG_GE_NCTRLP
 * 	If \p degree >= \p num_control_points.
 * @return TS_NUM_KNOTS
 * 	If \p type is ::TS_BEZIERS and
 * 	(\p num_control_points % \p degree + 1) != 0.
 * @return TS_MALLOC
 * 	If allocating memory failed.
 */
tsError TINYSPLINE_API
ts_bspline_new(size_t num_control_points,
               size_t dimension,
               size_t degree,
               tsBSplineType type,
               tsBSpline *spline,
               tsStatus *status);

/**
 * Creates a new spline with given control points (varargs) and stores the
 * result in \p spline. As all splines have at least one control point (with
 * minimum dimensionality one), the first component of the first control point
 * has a named parameter. Note that, by design of varargs in C, the last named
 * parameter must not be float. Thus, \p first is of type double instead of
 * ::tsReal.
 *
 * @param[in] num_control_points
 * 	The number of control points of \p spline.
 * @param[in] dimension
 * 	The dimension of the control points of \p spline.
 * @param[in] degree
 * 	The degree of \p spline.
 * @param[in] type
 * 	How to setup the knot vector of \p spline.
 * @param[out] spline
 * 	The output spline.
 * @param[out] status
 * 	The status of this function. May be NULL.
 * @param[in] first
 * 	The first component of the first control point.
 * @param[in] ...
 * 	The remaining components (control points).
 * @return TS_SUCCESS
 * 	On success.
 * @return TS_DIM_ZERO
 * 	If \p dimension is \c 0.
 * @return TS_DEG_GE_NCTRLP
 * 	If \p degree >= \p num_control_points.
 * @return TS_NUM_KNOTS
 * 	If \p type is ::TS_BEZIERS and
 * 	(\p num_control_points % \p degree + 1) != 0.
 * @return TS_MALLOC
 * 	If allocating memory failed.
 */
tsError TINYSPLINE_API
ts_bspline_new_with_control_points(size_t num_control_points,
                                   size_t dimension,
                                   size_t degree,
                                   tsBSplineType type,
                                   tsBSpline *spline,
                                   tsStatus *status,
                                   double first,
                                   ...);

/**
 * Creates a deep copy of \p src and stores the copied data in \p dest. \p src
 * and \p dest can be the same instance.
 *
 * \b Note: Unlike \e memcpy and \e memmove, the first parameter is the source
 * and the second parameter is the destination.
 *
 * @param[in] src
 * 	The spline to be deep copied.
 * @param[out] dest
 * 	The output spline.
 * @param[out] status
 * 	The status of this function. May be NULL.
 * @return TS_SUCCESS
 * 	On success.
 * @return TS_MALLOC
 * 	If allocating memory failed.
 */
tsError TINYSPLINE_API
ts_bspline_copy(const tsBSpline *src,
                tsBSpline *dest,
                tsStatus *status);

/**
 * Moves the ownership of the data of \p src to \p dest. After calling this
 * function, the data of \p src points to NULL. Does not release the data of \p
 * dest. \p src and \p dest can be the same instance (in this case, the data of
 * \p src remains).
 *
 * @param[in, out] src
 * 	The spline whose data is moved to \p dest.
 * @param[out] dest
 * 	The spline that receives the data of \p src.
 */
void TINYSPLINE_API
ts_bspline_move(tsBSpline *src,
                tsBSpline *dest);

/**
 * Releases the data of \p spline. After calling this function, the data of \p
 * spline points to NULL.
 *
 * @param[out] spline
 * 	The spline to be released.
 */
void TINYSPLINE_API
ts_bspline_free(tsBSpline *spline);
/*! @} */



/*! @name De Boor Net Data
 *
 * The internal state of ::tsDeBoorNet is protected using the PIMPL design
 * pattern (see https://en.cppreference.com/w/cpp/language/pimpl for more
 * details). The data of an instance can be accessed with the functions listed
 * in this section.
 *
 * @{
 */
/**
 * Represents the output of De Boor's algorithm. De Boor's algorithm is used to
 * evaluate a spline at a certain knot by iteratively computing a net of
 * intermediate points until the resulting point is available:
 *
 *     https://en.wikipedia.org/wiki/De_Boor%27s_algorithm
 *     https://www.cs.mtu.edu/~shene/COURSES/cs3621/NOTES/spline/de-Boor.html
 *
 * All points of a net are stored in \c points (::ts_deboornet_points). The
 * resulting point is the last point in \c points and, for the sake of
 * convenience, can be accessed with ::ts_deboornet_result.
 *
 * Two dimensional points are stored as follows:
 *
 *     [x_0, y_0, x_1, y_1, ..., x_n-1, y_n-1]
 *
 * Tree dimensional points are stored as follows:
 *
 *     [x_0, y_0, z_0, x_1, y_1, z_1, ..., x_n-1, y_n-1, z_n-1]
 *
 * ... and so on. The output also supports homogeneous coordinates
 * (cf. ::tsBSpline).
 *
 * There is a special case in which the evaluation of a knot \c u returns two
 * results instead of one. It occurs when the multiplicity of \c u (\c s(u)) is
 * equals to the order of the evaluated spline, indicating that the spline is
 * discontinuous at \c u. This is common practice for B-Splines (and NURBS)
 * consisting of connected Bezier curves where the endpoint of curve \c c_i is
 * equal to the start point of curve \c c_i+1. Yet, the end point of \c c_i and
 * the start point of \c c_i+1 may still be completely different, yielding to
 * visible gaps (if distance of the points is large enough). In such case (\c
 * s(u) == \c order), ::ts_deboornet_points stores only the two resulting
 * points (there is no net to calculate) and ::ts_deboornet_result points to
 * the \e first point in ::ts_deboornet_points. Since having gaps in splines is
 * unusual, both points in ::ts_deboornet_points are generally equal, making it
 * easy to handle this special case by simply calling
 * ::ts_deboornet_result. However, one can access both points if necessary:
 *
 *     ts_deboornet_result(...)[0] ...       // Access the first component of
 *                                           // the first result.
 *
 *     ts_deboornet_result(...)[dim(spline)] // Access the first component of
 *                                           // the second result.
 *
 * As if this wasn't complicated enough, there is an exception for this special
 * case, yielding to exactly one result (just like the regular case) even if \c
 * s(u) == \c order. It occurs when \c u is the lower or upper bound of the
 * domain of the evaluated spline. For instance, if \c b is a spline with
 * domain [0, 1] and \c b is evaluated at \c u = \c 0 or \c u = \c 1, then
 * ::ts_deboornet_result is \e always a single point regardless of the
 * multiplicity of \c u.
 *
 * In summary, there are three different types of evaluation:
 *
 * 1. The regular case, in which all points of the net are returned.
 *
 * 2. A special case, in which two results are returned (required for splines
 * with gaps).
 *
 * 3. The exception of 2., in which exactly one result is returned (even if \c
 * s(u) == \c order).
 *
 * All in all this looks quite complex (and actually it is), but for most
 * applications you do not have to deal with this. Just use
 * ::ts_deboornet_result to access the outcome of De Boor's algorithm.
 */
typedef struct
{
	struct tsDeBoorNetImpl *pImpl; /**< The actual implementation. */
} tsDeBoorNet;

/**
 * Returns the knot (sometimes also referred to as \c u or \c t) of \p net.
 *
 * @param[in] net
 * 	The net whose knot is read.
 * @return
 * 	The knot of \p net.
 */
tsReal TINYSPLINE_API
ts_deboornet_knot(const tsDeBoorNet *net);

/**
 * Returns the index of the knot of \p net.
 *
 * @param[in] net
 * 	The net whose index is read.
 * @return
 * 	The index [u_k, u_k+1) with \c u being the knot of \p net.
 */
size_t TINYSPLINE_API
ts_deboornet_index(const tsDeBoorNet *net);

/**
 * Returns the multiplicity of the knot of \p net.
 *
 * @param[in] net
 * 	The net whose multiplicity is read.
 * @return
 * 	The multiplicity of the knot of \p net.
 */
size_t TINYSPLINE_API
ts_deboornet_multiplicity(const tsDeBoorNet *net);

/**
 * Returns the number of insertion that were necessary to evaluate the knot of
 * \p net.
 *
 * @param[in] net
 * 	The net whose number of insertions of its knot is read.
 * @return
 * 	The number of insertions that were necessary to evaluate the knot of \p
 * 	net.
 */
size_t TINYSPLINE_API
ts_deboornet_num_insertions(const tsDeBoorNet *net);

/**
 * Returns the dimensionality of \p net, that is, the number of components of
 * its points (::ts_deboornet_points) and result (::ts_deboornet_result).
 * One-dimensional nets are possible, albeit their benefit might be
 * questionable.
 *
 * @param[in] net
 * 	The net whose dimension is read.
 * @return
 * 	The dimensionality of \p net (>= 1).
 */
size_t TINYSPLINE_API
ts_deboornet_dimension(const tsDeBoorNet *net);

/**
 * Returns the length of the point array of \p net.
 *
 * @param[in] net
 * 	The net whose length of the point array is read.
 * @return
 * 	The length of the point array of \p net.
 */
size_t TINYSPLINE_API
ts_deboornet_len_points(const tsDeBoorNet *net);

/**
 * Returns the number of points of \p net.
 *
 * @param[in] net
 * 	The net whose number of points is read.
 * @return
 * 	The number of points of \p net.
 */
size_t TINYSPLINE_API
ts_deboornet_num_points(const tsDeBoorNet *net);

/**
 * Returns the size of the point array of \p net. This function may be useful
 * when copying points using \e memcpy or \e memmove.
 *
 * @param[in] net
 * 	The net whose size of the point array is read.
 * @return
 * 	The size of the point array of \p net.
 */
size_t TINYSPLINE_API
ts_deboornet_sof_points(const tsDeBoorNet *net);

/**
 * Returns the pointer to the point array of \p net. Note that the return type
 * of this function is \c const for a reason. Clients should only read the
 * returned array. When suppressing the constness and writing to the array
 * against better knowledge, the client is on its own with regard to the
 * consistency of the internal state of \p net. To obtain a copy of the points
 * of \p net, use ::ts_deboornet_points.
 *
 * @param[in] net
 * 	The net whose pointer to the point array is returned.
 * @return
 * 	Pointer to the point array of \p net.
 */
const tsReal TINYSPLINE_API *
ts_deboornet_points_ptr(const tsDeBoorNet *net);

/**
 * Returns a deep copy of the points of \p net.
 *
 * @param[in] net
 * 	The net whose points are read.
 * @param[out] points
 * 	The output array. \b Note: It is the responsibility of the client to
 * 	release the allocated memory after use.
 * @param[out] status
 * 	The status of this function. May be NULL.
 * @return TS_SUCCESS
 * 	On success.
 * @return TS_MALLOC
 * 	If allocating memory failed.
 */
tsError TINYSPLINE_API
ts_deboornet_points(const tsDeBoorNet *net,
                    tsReal **points,
                    tsStatus *status);

/**
 * Returns the length of the result array of \p net.
 *
 * @param[in] net
 * 	The net whose length of the result array is read.
 * @return
 * 	The length of the result array of \p net.
 */
size_t TINYSPLINE_API
ts_deboornet_len_result(const tsDeBoorNet *net);

/**
 * Returns the number of points in the result array of \p net
 * (1 <= num_result <= 2).
 *
 * @param[in] net
 * 	The net whose number of points in the result array is read.
 * @return
 * 	The number of points in the result array of \p net.
 */
size_t TINYSPLINE_API
ts_deboornet_num_result(const tsDeBoorNet *net);

/**
 * Returns the size of the result array of \p net. This function may be useful
 * when copying results using \e memcpy or \e memmove.
 *
 * @param[in] net
 * 	The net whose size of the result array is read.
 * @return TS_SUCCESS
 * 	The size of the result array of \p net.
 */
size_t TINYSPLINE_API
ts_deboornet_sof_result(const tsDeBoorNet *net);

/**
 * Returns the pointer to the result array of \p net. Note that the return type
 * of this function is \c const for a reason. Clients should only read the
 * returned array. When suppressing the constness and writing to the array
 * against better knowledge, the client is on its own with regard to the
 * consistency of the internal state of \p net. To obtain a copy of the result
 * of \p net, use ::ts_deboornet_result.
 *
 * @param[in] net
 * 	The net whose pointer to the result array is returned.
 * @return
 * 	Pointer to the result array of \p net.
 */
const tsReal TINYSPLINE_API *
ts_deboornet_result_ptr(const tsDeBoorNet *net);

/**
 * Returns a deep copy of the result of \p net.
 *
 * @param[in] net
 * 	The net whose result is read.
 * @param[out] result
 * 	The output array. \b Note: It is the responsibility of the client to
 * 	release the allocated memory after use.
 * @param[out] status
 * 	The status of this function. May be NULL.
 * @return TS_SUCCESS
 * 	On success.
 * @return TS_MALLOC
 * 	If allocating memory failed.
 */
tsError TINYSPLINE_API
ts_deboornet_result(const tsDeBoorNet *net,
                    tsReal **result,
                    tsStatus *status);
/*! @} */



/*! @name De Boor Net Initialization
 *
 * The following functions are used to create and release ::tsDeBoorNet
 * instances as well as to copy and move the internal data of a ::tsDeBoorNet
 * instance to another instance.
 *
 * \b Note: It is recommended to initialize an instance with
 * ::ts_deboornet_init. This way, ::ts_deboornet_free can be called in
 * ::TS_CATCH and ::TS_FINALLY blocks (see Section Error Handling for more
 * details) without further checking. For example:
 *
 *     tsDeBoorNet net = ts_deboornet_init();
 *     TS_TRY(...)
 *         ...
 *     TS_FINALLY
 *         ts_deboornet_free(&net);
 *     TS_END_TRY
 *
 * @{
 */
/**
 * Creates a new net whose data points to NULL.
 *
 * @return
 * 	A new net whose data points to NULL.
 */
tsDeBoorNet TINYSPLINE_API
ts_deboornet_init(void);

/**
 * Creates a deep copy of \p src and stores the copied data in \p dest. \p src
 * and \p dest can be the same instance.
 *
 * \b Note: Unlike \e memcpy and \e memmove, the first parameter is the source
 * and the second parameter is the destination.
 *
 * @param[in] src
 * 	The net to be deep copied.
 * @param[out] dest
 * 	The output net.
 * @param[out] status
 * 	The status of this function. May be NULL.
 * @return TS_SUCCESS
 * 	On success.
 * @return TS_MALLOC
 * 	If allocating memory failed.
 */
tsError TINYSPLINE_API
ts_deboornet_copy(const tsDeBoorNet *src,
                  tsDeBoorNet *dest,
                  tsStatus *status);

/**
 * Moves the ownership of the data of \p src to \p dest. After calling this
 * function, the data of \p src points to NULL. Does not release the data of \p
 * dest. \p src and \p dest can be the same instance (in this case, the data of
 * \p src remains).
 *
 * @param[out] src
 * 	The net whose data is moved to \p dest.
 * @param[out] dest
 * 	The net that receives the data of \p src.
 */
void TINYSPLINE_API
ts_deboornet_move(tsDeBoorNet *src,
                  tsDeBoorNet *dest);

/**
 * Releases the data of \p net. After calling this function, the data of \p net
 * points to NULL.
 *
 * @param[out] net
 * 	The net to be released.
 */
void TINYSPLINE_API
ts_deboornet_free(tsDeBoorNet *net);
/*! @} */



/*! @name Interpolation and Approximation Functions
 *
 * Given a set (or a sequence) of points, interpolate/approximate a spline that
 * follows these points.
 *
 * Note: Approximations have not yet been implemented. Pull requests are
 * welcome.
 *
 * @{
 */
/**
 * Interpolates a cubic spline with natural end conditions. For more details
 * see:
 *
 *     https://en.wikipedia.org/wiki/Tridiagonal_matrix_algorithm
 *     http://www.math.ucla.edu/~baker/149.1.02w/handouts/dd_splines.pdf
 *     http://www.bakoma-tex.com/doc/generic/pst-bspline/pst-bspline-doc.pdf
 *
 * The interpolated spline is a sequence of bezier curves connecting each point
 * in \p points. Each bezier curve is of degree \c 3 with dimensionality \p
 * dimension. The total number of control points is:
 *
 *     min(1, \p num_points - 1) * 4
 *
 * Note: \p num_points is the number of points in \p points and not the length
 * of \p points. For instance, the following point vector has
 * \p num_points = 4 and \p dimension = 2:
 *
 *     [x0, y0, x1, y1, x2, y2, x3, y3]
 *
 * @param[in] points
 * 	The points to be interpolated.
 * @param[in] num_points
 * 	The number of points in \p points. If \c 1, a cubic point (i.e., a
 * 	spline with four times the same control point) is created.
 * @param[in] dimension
 * 	The dimensionality of the points.
 * @param[out] spline
 * 	The interpolated spline.
 * @param[out] status
 * 	The status of this function. May be NULL.
 * @return TS_SUCCESS
 * 	On success.
 * @return TS_DIM_ZERO
 * 	If \p dimension is 0.
 * @return TS_NUM_POINTS
 * 	If \p num_points is 0.
 * @return TS_MALLOC
 * 	If allocating memory failed.
 */
tsError TINYSPLINE_API
ts_bspline_interpolate_cubic_natural(const tsReal *points,
                                     size_t num_points,
                                     size_t dimension,
                                     tsBSpline *spline,
                                     tsStatus *status);

/**
 * Interpolates a piecewise cubic spline by translating the given catmull-rom
 * control points into a sequence of bezier curves. In order to avoid division
 * by zero, successive control points with distance less than or equal to \p
 * epsilon are filtered out. If the resultant sequence contains only a single
 * point, a cubic point (i.e., a spline with four times the same control point)
 * is created. Optionally, the first and last control point can be specified
 * (see \p first and \p last).
 *
 * @param[in] points
 * 	The points to be interpolated.
 * @param[in] num_points
 * 	The number of points in \p points. If \c 1, a cubic point (i.e., a
 * 	spline with four times the same control point) is created.
 * @param[in] dimension
 * 	The dimensionality of the points.
 * @param[in] alpha
 * 	Knot parameterization: 0 => uniform, 0.5 => centripetal, 1 => chordal.
 * 	The input value is clamped to the domain [0, 1].
 * @param[in] first
 * 	The first control point of the catmull-rom sequence. If NULL, an
 * 	appropriate point is generated based on the first two points in
 * 	\p points. If the distance between \p first and the first control point
 * 	in \p points is less than or equals to \p epsilon, \p first is treated
 * 	as NULL. This is necessary to avoid division by zero.
 * @param[in] last
 * 	The last control point of the catmull-rom sequence. If NULL, an
 * 	appropriate point is generated based on the last two points in
 * 	\p points. If the distance between \p last and the last control point
 * 	in \p points is less than or equals to \p epsilon, \p last is treated
 * 	as NULL. This is necessary to avoid division by zero.
 * @param[in] epsilon
 * 	The maximum distance between points with "same" coordinates. That is,
 * 	if the distance between neighboring points is less than or equal to
 * 	\p epsilon, they are considered equal. For the sake of fail-safeness,
 * 	the sign is removed with fabs. It is advisable to pass a value greater
 * 	than zero, however, it is not necessary.
 * @param[out] spline
 * 	The interpolated spline.
 * @param[out] status
 * 	The status of this function. May be NULL.
 * @return TS_SUCCESS
 * 	On success.
 * @return TS_DIM_ZERO
 * 	If \p dimension is 0.
 * @return TS_NUM_POINTS
 * 	If \p num_points is 0.
 * @return TS_MALLOC
 * 	If allocating memory failed.
 */
tsError TINYSPLINE_API
ts_bspline_interpolate_catmull_rom(const tsReal *points,
                                   size_t num_points,
                                   size_t dimension,
                                   tsReal alpha,
                                   const tsReal *first,
                                   const tsReal *last,
                                   tsReal epsilon,
                                   tsBSpline *spline,
                                   tsStatus *status);
/*! @} */



/*! @name Query Functions
 *
 * Functions for querying different kinds of data from splines.
 *
 * @{
 */
/**
 * Evaluates \p spline at \p knot and stores the result (see ::tsDeBoorNet) in
 * \p net.
 *
 * @param[in] spline
 * 	The spline to evaluate.
 * @param[in] knot
 * 	The knot to evaluate \p spline at.
 * @param[out] net
 * 	Stores the evaluation result.
 * @param[out] status
 * 	The status of this function. May be NULL.
 * @return TS_SUCCESS
 * 	On success.
 * @return TS_U_UNDEFINED
 * 	If \p spline is not defined at \p knot.
 * @return TS_MALLOC
 * 	If allocating memory failed.
 */
tsError TINYSPLINE_API
ts_bspline_eval(const tsBSpline *spline,
                tsReal knot,
                tsDeBoorNet *net,
                tsStatus *status);

/**
 * Evaluates \p spline at each knot in \p knots and stores the evaluated points
 * (see ::ts_deboornet_result) in \p points. If \p knots contains one or more
 * knots where \p spline is discontinuous at, only the first point of the
 * corresponding evaluation result is taken. After calling this function \p
 * points contains exactly \code num * ts_bspline_dimension(spline) \endcode
 * values.
 *
 * This function is in particular useful in cases where a multitude of knots
 * need to be evaluated, because only a single instance of ::tsDeBoorNet is
 * created and reused for all evaluation tasks (therefore the memory footprint
 * is reduced to a minimum).
 *
 * @param[in] spline
 * 	The spline to evaluate.
 * @param[in] knots
 * 	The knot values to evaluate.
 * @param[in] num
 * 	The number of knots in \p us.
 * @param[out] points
 * 	The output parameter.
 * @param[out] status
 * 	The status of this function. May be NULL.
 * @return TS_SUCCESS
 * 	On success.
 * @return TS_U_UNDEFINED
 * 	If \p spline is not defined at one of the knot values in \p us.
 * @return TS_MALLOC
 * 	If allocating memory failed.
 */
tsError TINYSPLINE_API
ts_bspline_eval_all(const tsBSpline *spline,
                    const tsReal *knots,
                    size_t num,
                    tsReal **points,
                    tsStatus *status);

/**
 * Generates a sequence of \p num different knots, passes this sequence to
 * ::ts_bspline_eval_all, and stores the resultant points in \p points. The
 * sequence of knots is generated using ::ts_bspline_uniform_knot_seq. If \p
 * num is 0, the default value \c 100 is used as fallback.
 *
 * For the sake of stability regarding future changes, the actual number of
 * generated knots (which only differs from \p num if \p num is 0) is stored in
 * \p actual_num. If \p num is 1, the point located at the minimum of the
 * domain of \p spline is evaluated.
 *
 * @param[in] spline
 * 	The spline to be evaluate.
 * @param[in] num
 * 	The number of knots to be generate.
 * @param[out] points
 * 	The output parameter.
 * @param[out] actual_num
 * 	The actual number of generated knots. Differs from \p num only if
 * 	\p num is 0. Must not be NULL.
 * @param[out] status
 * 	The status of this function. May be NULL.
 * @return TS_SUCCESS
 * 	On success.
 * @return TS_MALLOC
 * 	If allocating memory failed.
 */
tsError TINYSPLINE_API
ts_bspline_sample(const tsBSpline *spline,
                  size_t num,
                  tsReal **points,
                  size_t *actual_num,
                  tsStatus *status);

/**
 * Tries to find a point P on \p spline such that:
 *
 *     ts_distance(P[index], value, 1) <= fabs(epsilon)
 *
 * This function is using the bisection method to determine P. Accordingly, it
 * is expected that the control points of \p spline are sorted at component
 * \p index either in ascending order (if \p ascending != 0) or in descending
 * order (if \p ascending == 0). If the control points of \p spline are not
 * sorted at component \p index, the behaviour of this function is undefined.
 * For the sake of fail-safeness, the distance of P[index] and \p value is
 * compared with the absolute value of \p epsilon (using fabs).
 *
 * The bisection method is an iterative approach which minimizes the error
 * (\p epsilon) with each iteration step until an "optimum" was found. However,
 * there may be no point P satisfying the distance condition. Thus, the number
 * of iterations must be limited (\p max_iter). Depending on the domain of the
 * control points of \p spline at component \p index and \p epsilon,
 * \p max_iter ranges from 7 to 50. In most cases \p max_iter == 30 should be
 * fine though. The parameter \p persnickety allows to define the behaviour of
 * this function is case no point was found after \p max_iter iterations. If
 * enabled (!= 0), TS_NO_RESULT is returned. If disabled (== 0), the best
 * fitting point is returned.
 *
 * @param[in] spline
 * 	The spline to evaluate
 * @param[in] value
 * 	The value (point at component \p index) to find.
 * @param[in] epsilon
 * 	The maximum distance (inclusive).
 * @param[in] persnickety
 * 	Indicates whether TS_NO_RESULT should be returned if there is no point
 * 	P satisfying the distance condition (!= 0 to enable, == 0 to disable).
 * 	If disabled, the best fitting point is returned.
 * @param[in] index
 * 	The point's component.
 * @param[in] ascending
 * 	Indicates whether the control points of \p spline are sorted in
 * 	ascending (!= 0) or in descending (== 0) order at component \p index.
 * @param[in] max_iter
 * 	The maximum number of iterations (30 is a sane default value).
 * @param[out] net
 * 	The output parameter.
 * @param[out] status
 * 	The status of this function. May be NULL.
 * @return TS_SUCCESS
 * 	On success.
 * @return TS_INDEX_ERROR
 * 	If the dimension of the control points of \p spline <= \p index.
 * @return TS_NO_RESULT
 * 	If \p persnickety is enabled (!= 0) and there is no point P satisfying
 * 	the distance condition.
 * @return TS_MALLOC
 * 	If allocating memory failed.
 */
tsError TINYSPLINE_API
ts_bspline_bisect(const tsBSpline *spline,
                  tsReal value,
                  tsReal epsilon,
                  int persnickety,
                  size_t index,
                  int ascending,
                  size_t max_iter,
                  tsDeBoorNet *net,
                  tsStatus *status);

/**
 * Returns the domain of \p spline.
 *
 * @param[in] spline
 * 	The spline to query.
 * @param[out] min
 * 	The lower bound of the domain of \p spline.
 * @param[out] max
 * 	The upper bound of the domain of \p spline.
 */
void TINYSPLINE_API
ts_bspline_domain(const tsBSpline *spline,
                  tsReal *min,
                  tsReal *max);

/**
 * Checks whether the distance of the endpoints of \p spline is less than or
 * equal to \p epsilon for the first 'ts_bspline_degree - 1' derivatives
 * (starting with the zeroth derivative).
 *
 * @param[in] spline
 * 	The spline to query.
 * @param[in] epsilon
 * 	The maximum distance.
 * @param[out] closed
 * 	The output parameter. 1 if true, 0 otherwise.
 * @param[out] status
 * 	The status of this function. May be NULL.
 * @return TS_SUCCESS
 * 	On success.
 * @return TS_MALLOC
 * 	If allocating memory failed.
 */
tsError TINYSPLINE_API
ts_bspline_is_closed(const tsBSpline *spline,
                     tsReal epsilon,
                     int *closed,
                     tsStatus *status);

/**
 * Computes a sequence of three-dimensional frames (see ::tsFrame) for the
 * spline \p spline. The position of the frames corresponds to the knots in \p
 * knots. The implementation is based on:
 *
 *     @article{10.1145/1330511.1330513,
 *       author =       {Wang, Wenping and J\"{u}ttler, Bert and Zheng, Dayue
 *                       and Liu, Yang},
 *       title =        {Computation of Rotation Minimizing Frames},
 *       year =         {2008},
 *       issue_date =   {March 2008},
 *       publisher =    {Association for Computing Machinery},
 *       address =      {New York, NY, USA},
 *       volume =       {27},
 *       number =       {1},
 *       issn =         {0730-0301},
 *       url =          {https://doi.org/10.1145/1330511.1330513},
 *       doi =          {10.1145/1330511.1330513},
 *       abstract =     {Due to its minimal twist, the rotation minimizing
 *                       frame (RMF) is widely used in computer graphics,
 *                       including sweep or blending surface modeling, motion
 *                       design and control in computer animation and
 *                       robotics, streamline visualization, and tool path
 *                       planning in CAD/CAM. We present a novel simple and
 *                       efficient method for accurate and stable computation
 *                       of RMF of a curve in 3D. This method, called the
 *                       double reflection method, uses two reflections to
 *                       compute each frame from its preceding one to yield a
 *                       sequence of frames to approximate an exact RMF. The
 *                       double reflection method has the fourth order global
 *                       approximation error, thus it is much more accurate
 *                       than the two currently prevailing methods with the
 *                       second order approximation error—the projection
 *                       method by Klok and the rotation method by
 *                       Bloomenthal, while all these methods have nearly the
 *                       same per-frame computational cost. Furthermore, the
 *                       double reflection method is much simpler and faster
 *                       than using the standard fourth order Runge-Kutta
 *                       method to integrate the defining ODE of the RMF,
 *                       though they have the same accuracy. We also
 *                       investigate further properties and extensions of the
 *                       double reflection method, and discuss the
 *                       variational principles in design moving frames with
 *                       boundary conditions, based on RMF.},
 *       journal =      {ACM Trans. Graph.},
 *       month =        mar,
 *       articleno =    {2},
 *       numpages =     {18},
 *       keywords =     {motion design, sweep surface, motion, differential
 *                       geometry, Curve, rotation minimizing frame}
 *     }
 *
 * @pre \p knots and \p frames have \p num entries.
 * @param[in] spline
 * 	The spline to query.
 * @param[in] knots
 * 	The knots to query \p spline at.
 * @param[in] num
 * 	Number of elements in \p knots and \p frames. Can be \c 0.
 * @param[in] has_first_normal
 * 	Indicates whether the normal of the first element of \p frames should
 * 	be taken as starting value for the algorithm. If \c 0, the starting
 * 	normal is determined based on the tangent of \p spline at \c knots[0].
 * 	Note that, if the argument value is not \c 0, it is up to the caller of
 * 	this function to ensure that the supplied normal is valid. The function
 * 	only normalizes the supplied value.
 * @param[in, out] frames
 * 	Stores the computed frames.
 * @param[out] status
 * 	The status of this function. May be NULL.
 * @return TS_SUCCESS
 * 	On success.
 * @return TS_MALLOC
 * 	If memory allocation failed.
 */
tsError TINYSPLINE_API
ts_bspline_compute_rmf(const tsBSpline *spline,
                       const tsReal *knots,
                       size_t num,
                       int has_first_normal,
                       tsFrame *frames,
                       tsStatus *status);

/**
 * Computes the cumulative chord lengths of the points of the given
 * knots. Note that the first length (i.e., <tt>lengths[0]</tt>) is
 * always \c 0, even if the minimum of the domain of \p spline is less
 * than the first knot (i.e., <tt>knots[0]</tt>). Also, the returned
 * lengths may be inaccurate if \p spline is discontinuous (i.e., the
 * multiplicity of one of the interior knots is equal to the order of
 * \p spline) with unequal evaluation points---in such case only the
 * first result of the evaluation is included in the calculation.
 *
 * @pre \p knots and \p lengths have length \p num.
 * @param[in] spline
 * 	The spline to query.
 * @param[in] knots
 * 	The knots to evaluate \p spline at.
 * @param[in] num
 * 	Number of knots in \p knots.
 * @param[out] lengths
 * 	The cumulative chord lengths. <tt>lengths[i]</tt> is the length of \p
 * 	spline at knot <tt>i</tt> (<tt>knots[i]</tt>).
 * @param[out] status
 * 	The status of this function. May be NULL.
 * @return TS_SUCCESS
 * 	On success.
 * @return TS_U_UNDEFINED
 * 	If \p spline is not defined at one of the knots in \p knots.
 * @return TS_KNOTS_DECR
 * 	If \p knots is not monotonically increasing.
 * @return TS_MALLOC
 * 	If memory allocation failed.
 */
tsError TINYSPLINE_API
ts_bspline_chord_lengths(const tsBSpline *spline,
                         const tsReal *knots,
                         size_t num,
                         tsReal *lengths,
                         tsStatus *status);

/**
 * Extracts a sub-spline from \p spline with respect to the given domain
 * <tt>[knot0, knot1]</tt>. The knots \p knot0 and \p knot1 must lie within the
 * domain of \p spline and must not be equal according to
 * ::ts_knots_equal. However, \p knot0 can be greater than \p knot1. In this
 * case, the control points of the sub-spline are reversed.

 * @param[in] spline
 * 	The spline to query.
 * @param[in] knot0
 * 	Lower bound of the domain of the queried sub-spline if \p knot0 is less
 * 	than \p knot1. Upper bound otherwise.
 * @param[in] knot1
 * 	Upper bound of the domain of the queried sub-spline if \p knot1 is
 * 	greater than \p knot0. Lower bound otherwise.
 * @param[out] sub
 * 	The queried sub-spline.
 * @param[out] status
 * 	The status of this function. May be NULL.
 * @return TS_SUCCESS
 * 	On success.
 * @return TS_U_UNDEFINED
 * 	If \p spline is not defined at \p knot0 or \p knot1.
 * @return TS_NO_RESULT
 * 	If \p knot0 and \p knot1 are equal according to ::ts_knots_equal.
 * @return TS_MALLOC
 * 	If memory allocation failed.
 */
tsError TINYSPLINE_API
ts_bspline_sub_spline(const tsBSpline *spline,
                      tsReal knot0,
                      tsReal knot1,
                      tsBSpline *sub,
                      tsStatus *status);

/**
 * Generates a sequence of \p num knots with uniform distribution. \e Uniform
 * means that consecutive knots in \p knots have the same distance.
 *
 * @param[in] spline
 * 	The spline to query.
 * @param[in] num
 * 	Number of knots in \p knots.
 * @param[out] knots
 * 	Stores the generated knot sequence.
 */
void TINYSPLINE_API
ts_bspline_uniform_knot_seq(const tsBSpline *spline,
                            size_t num,
                            tsReal *knots);

/**
 * Short-cut function for ::ts_chord_lengths_equidistant_knot_seq. The ordering
 * of the parameters (in particular, \p num_samples after \p knots) is aligned
 * to ::ts_bspline_uniform_knot_seq so that it is easier for users to replace
 * one call with the other.
 *
 * @param[in] spline
 * 	The spline to query.
 * @param[in] num
 * 	Number of knots in \p knots.
 * @param[out] knots
 * 	Stores the generated knot sequence.
 * @param[in] num_samples
 * 	Number of knots to be sampled for the 'reparametrization by arc length'.
 * 	\c 200 yields a quite precise mapping (subpixel accuracy). For very,
 * 	very high precision requirements, \c 500 should be sufficient. If \c 0,
 * 	the default value \c 200 is used as fallback.
 * @param[out] status
 * 	The status of this function. May be NULL.
 * @return TS_SUCCESS
 * 	On success.
 * @return TS_MALLOC
 * 	If memory allocation failed.
 */
tsError TINYSPLINE_API
ts_bspline_equidistant_knot_seq(const tsBSpline *spline,
                                size_t num,
                                tsReal *knots,
                                size_t num_samples,
                                tsStatus *status);
/*! @} */



/*! @name Transformation Functions
 *
 * Transformations modify the internal state of a spline---e.g., the number of
 * control points, the structure of the knot vector, the degree, and so on. Some
 * transformations modify the state without changing the shape of the spline
 * (e.g., ::ts_bspline_elevate_degree). All transformations specify at least
 * three parameters: i) an input spline (the spline to be transformed), ii) an
 * output spline (the spline which receives the result of the transformation),
 * and iii) a ::tsStatus object (output parameter for error handling). Along
 * with these parameters, additional parameters may be necessary to i) calculate
 * the transformation (such as ::ts_bspline_tension) and ii) store additional
 * results (such as ::ts_bspline_insert_knot). Unless stated otherwise, the
 * order of the parameters of a transformation function, \c t, is:
 *
 *     t(input, [additional_input], output, [additional_output], status)
 *
 * \b Note: Transformation functions do not releases the memory of the output
 * spline before assigning the result of the transformation to it. Thus, when
 * using the same output spline multiple times, make sure to release its memory
 * before each call (after the first one). If not, severe memory leaks are to be
 * expected:
 *
 *     tsBSpline in = ...                   // an arbitrary spline
 *     tsBSpline out = ts_bspline_init();   // stores the result
 *
 *     ts_bspline_to_beziers(&in, &out);    // first transformation
 *     ...                                  // some code
 *     ts_bspline_free(&out);               // avoid memory leak.
 *     ts_bspline_tension(&in, 0.85, &out); // next transformation
 *
 * It is possible to pass a spline as input and output argument at the same
 * time. In this case, the called transformation uses a temporary buffer to
 * store the working data. If the transformation succeeds, the memory of the
 * supplied spline is released and the result is assigned to it. So even if a
 * transformation fails, the internal state of the supplied splines stays intact
 * (i.e., it remains unchanged).
 *
 * \b Note: It is not necessary to release the memory of a spline which is
 * passed as input and output argument at the same time before calling the next
 * transformation (in fact that would fail due to a null pointer):
 *
 *     tsBSpline spline = ...                      // an arbitrary spline
 *     ts_bspline_to_beziers(&spline, &spline);    // first transformation
 *     ts_bspline_tension(&spline, 0.85, &spline); // next transformation
 *
 * @{
 */
/**
 * Returns the \p n'th derivative of \p spline as ::tsBSpline instance. The
 * derivative of a spline \c s of degree \c d (\c d > 0) with \c m control
 * points and \c n knots is another spline \c s' of degree \c d-1 with \c m-1
 * control points and \c n-2 knots, defined over \c s as:
 *
 * \f{eqnarray*}{
 *   s'(u) &=& \sum_{i=0}^{n-1} N_{i+1,p-1}(u)    *
 *                              (P_{i+1} - P_{i}) *
 *                              p / (u_{i+p+1}-u_{i+1}) \\
 *         &=& \sum_{i=1}^{n} N_{i,p-1}(u)      *
 *                            (P_{i} - P_{i-1}) *
 *                            p / (u_{i+p}-u_{i})
 * \f}
 *
 * If \c s has a clamped knot vector, it can be shown that:
 *
 * \f{eqnarray*}{
 *   s'(u) &=& \sum_{i=0}^{n-1} N_{i,p-1}(u)      *
 *                              (P_{i+1} - P_{i}) *
 *                              p / (u_{i+p+1}-u_{i+1})
 * \f}
 *
 * where the multiplicity of the first and the last knot value \c u is \c p
 * rather than \c p+1. The derivative of a point (degree 0) is another point
 * with coordinate 0. For more details, see:
 *
 *     http://www.cs.mtu.edu/~shene/COURSES/cs3621/NOTES/spline/B-spline/bspline-derv.html
 *
 * If \p spline != \p deriv, the internal state of \p spline is not modified,
 * that is, \p deriv is a new, independent ::tsBSpline instance.
 *
 * @param[in] spline
 * 	The spline to be derived.
 * @param[in] n
 * 	Number of derivations.
 * @param[in] epsilon
 * 	The maximum distance of discontinuous points. If negative,
 * 	discontinuity is ignored and the derivative is computed based on the
 * 	first result of the corresponding ::tsDeBoorNet.
 * @param[out] deriv
 *	The derivative of \p spline.
 * @param[out] status
 * 	The status of this function. May be NULL.
 * @return TS_SUCCESS
 * 	On success.
 * @return TS_UNDERIVABLE
 * 	If \p spline is discontinuous at an internal knot and the distance
 * 	between the corresponding points is greater than \p epsilon.
 * @return TS_MALLOC
 * 	If allocating memory failed.
 */
tsError TINYSPLINE_API
ts_bspline_derive(const tsBSpline *spline,
                  size_t n,
                  tsReal epsilon,
                  tsBSpline *deriv,
                  tsStatus *status);

/**
 * Inserts \p knot \p num times into the knot vector of \p spline. The
 * operation fails if \p result would have an invalid knot vector (i.e.,
 * multiplicity(knot) > order(result)). If \p spline != \p result, the internal
 * state of \p spline is not modified, that is, \p result is a new, independent
 * ::tsBSpline instance.
 *
 * @param[in] spline
 * 	The spline into which \p knot is inserted \p num times.
 * @param[in] knot
 * 	The knot to be inserted.
 * @param[in] num
 * 	Number of insertions.
 * @param[out] result
 * 	The output spline.
 * @param[out] k
 * 	Stores the last index of \p knot in \p result.
 * @param status
 * 	The status of this function. May be NULL.
 * @return TS_SUCCESS
 * 	On success.
 * @return TS_U_UNDEFINED
 * 	If \p knot is not within the domain of \p spline.
 * @return TS_MULTIPLICITY
 * 	If the multiplicity of \p knot in \p spline plus \p num is greater than
 * 	the order of \p spline.
 * @return TS_MALLOC
 * 	If allocating memory failed.
 */
tsError TINYSPLINE_API
ts_bspline_insert_knot(const tsBSpline *spline,
                       tsReal knot, size_t num,
                       tsBSpline *result,
                       size_t *k,
                       tsStatus *status);

/**
 * Splits \p spline at \p knot. That is, \p knot is inserted into \p spline \c
 * n times such that the multiplicity of \p knot is equal the spline's order.
 * If \p spline != \p split, the internal state of \p spline is not modified,
 * that is, \p split is a new, independent ::tsBSpline instance.
 *
 * @param[in] spline
 * 	The spline to be split.
 * @param[in] knot
 * 	The split point (knot).
 * @param[out] split
 * 	The split spline.
 * @param[out] k
 * 	Stores the last index of \p knot in \p split.
 * @param[out] status
 * 	The status of this function. May be NULL.
 * @return TS_SUCCESS
 * 	On success.
 * @return TS_U_UNDEFINED
 * 	If \p spline is not defined at \p knot.
 * @return TS_MALLOC
 * 	If allocating memory failed.
 */
tsError TINYSPLINE_API
ts_bspline_split(const tsBSpline *spline,
                 tsReal knot,
                 tsBSpline *split,
                 size_t *k,
                 tsStatus *status);

/**
 * Straightens the control points of \p spline according to \p beta (0 yields a
 * line connecting the first and the last control point; 1 keeps the original
 * shape). The value of \p beta is clamped to [0, 1]. If \p spline != \p out,
 * the internal state of \p spline is not modified, that is, \p out is a new,
 * independent ::tsBSpline instance.
 *
 * This function is based on:
 *
 *     @ARTICLE{10.1109/TVCG.2006.147,
 *       author =       {Holten, Danny},
 *       journal =      {IEEE Transactions on Visualization and Computer
 *                       Graphics},
 *       title =        {Hierarchical Edge Bundles: Visualization of
 *                       Adjacency Relations in Hierarchical Data},
 *       year =         {2006},
 *       volume =       {12},
 *       number =       {5},
 *       pages =        {741-748},
 *       abstract =     {A compound graph is a frequently encountered type of
 *                       data set. Relations are given between items, and a
 *                       hierarchy is defined on the items as well. We
 *                       present a new method for visualizing such compound
 *                       graphs. Our approach is based on visually bundling
 *                       the adjacency edges, i.e., non-hierarchical edges,
 *                       together. We realize this as follows. We assume that
 *                       the hierarchy is shown via a standard tree
 *                       visualization method. Next, we bend each adjacency
 *                       edge, modeled as a B-spline curve, toward the
 *                       polyline defined by the path via the inclusion edges
 *                       from one node to another. This hierarchical bundling
 *                       reduces visual clutter and also visualizes implicit
 *                       adjacency edges between parent nodes that are the
 *                       result of explicit adjacency edges between their
 *                       respective child nodes. Furthermore, hierarchical
 *                       edge bundling is a generic method which can be used
 *                       in conjunction with existing tree visualization
 *                       techniques. We illustrate our technique by providing
 *                       example visualizations and discuss the results based
 *                       on an informal evaluation provided by potential
 *                       users of such visualizations.},
 *       keywords =     {},
 *       doi =          {10.1109/TVCG.2006.147},
 *       ISSN =         {1941-0506},
 *       month =        {Sep.},
 *     }
 *
 * Holten calls it "straightening" (page 744, equation 1).
 *
 * @param[in] spline
 * 	The spline to be straightened.
 * @param[in] beta
 * 	The straightening factor. The value is clamped to the domain [0, 1].
 * @param[out] out
 * 	The straightened spline.
 * @param[out] status
 * 	The status of this function. May be NULL.
 * @return TS_SUCCESS
 * 	On success.
 * @return TS_MALLOC
 * 	If allocating memory failed.
 */
tsError TINYSPLINE_API
ts_bspline_tension(const tsBSpline *spline,
                   tsReal beta,
                   tsBSpline *out,
                   tsStatus *status);

/**
 * Decomposes \p spline into a sequence of Bezier curves by splitting it at
 * each internal knot. If \p spline != \p beziers, the internal state of \p
 * spline is not modified, that is, \p beziers is a new, independent
 * ::tsBSpline instance.
 *
 * @param[in] spline
 * 	The spline to be decomposed.
 * @param[out] beziers
 * 	The bezier decomposition of \p spline.
 * @param[out] status
 * 	The status of this function. May be NULL.
 * @return TS_SUCCESS
 * 	On success.
 * @return TS_MALLOC
 * 	If allocating memory failed.
 */
tsError TINYSPLINE_API
ts_bspline_to_beziers(const tsBSpline *spline,
                      tsBSpline *beziers,
                      tsStatus *status);

/**
 * Elevates the degree of \p spline by \p amount and stores the result in
 * \p elevated. If \p spline != \p elevated, the internal state of \p spline is
 * not modified, that is, \p elevated is a new, independent ::tsBSpline
 * instance.
 *
 * @param[in] spline
 * 	The spline to be elevated.
 * @param[in] amount
 * 	How often to elevate the degree of \p spline.
 * @param[in] epsilon
 * 	In order to elevate the degree of a spline, it must be decomposed into
 * 	a sequence of bezier curves (see ::ts_bspline_to_beziers). After degree
 * 	elevation, the split points of the bezier curves are merged again. This
 * 	parameter is used to distinguish between the split points of the
 * 	decomposition process and discontinuity points that should be retained.
 * 	A viable default value is ::TS_POINT_EPSILON. If negative, the resulting
 * 	spline, \p elevated, forms a sequence of bezier curves.
 * @param[out] elevated
 * 	The elevated spline.
 * @param[out] status
 * 	The status of this function. May be NULL.
 * @return TS_SUCCESS
 * 	On success.
 * @return TS_MALLOC
 * 	If memory allocation failed.
 */
tsError TINYSPLINE_API
ts_bspline_elevate_degree(const tsBSpline *spline,
                          size_t amount,
                          tsReal epsilon,
                          tsBSpline *elevated,
                          tsStatus *status);

/**
 * Modifies the splines \p s1 and \p s2 such that they have same degree and
 * number of control points/knots (without modifying the shape of \p s1 and
 * \p s2). The resulting splines are stored in \p s1_out and \p s2_out. If
 * \p s1 != \p s1_out, the internal state of \p s1 is not modified, that is,
 * \p s1_out is a new, independent ::tsBSpline instance. The same is true for
 * \p s2 and \p s2_out.
 *
 * @param[in] s1
 * 	The spline which is to be aligned with \p s2.
 * @param[in] s2
 * 	The spline which is to be aligned with \p s1.
 * @param[in] epsilon
 * 	Spline alignment relies on degree elevation. This parameter is used in
 * 	::ts_bspline_elevate_degree to check whether two control points, \c p1
 * 	and \c p2, are "equal", that is, the distance between \c p1 and \c p2
 * 	is less than or equal to \p epsilon. A viable default value is
 * 	::TS_POINT_EPSILON.
 * @param[out] s1_out
 * 	The aligned version of \p s1.
 * @param[out] s2_out
 * 	The aligned version of \p s2.
 * @param[out] status
 * 	The status of this function. May be NULL.
 * @return TS_SUCCESS
 * 	On success.
 * @return TS_MALLOC
 * 	If memory allocation failed.
 */
tsError TINYSPLINE_API
ts_bspline_align(const tsBSpline *s1,
                 const tsBSpline *s2,
                 tsReal epsilon,
                 tsBSpline *s1_out,
                 tsBSpline *s2_out,
                 tsStatus *status);

/**
 * Interpolates between \p origin and \p target with respect to the time
 * parameter \p t (domain: [0, 1]; clamped if necessary). The resulting spline
 * is stored in \p out. Because it is to be expected that this function is
 * called several times in a row (e.g., to have a smooth transition from one
 * spline to another), memory for \p out is allocated only if it points to NULL
 * or if it has to be enlarged to store the result of the interpolation (which
 * can only happen if \p origin or \p target---or both---have been changed
 * since the last call). This way, this function can be used as follows:
 *
 *     tsReal t;
 *     tsBSpline origin = ...
 *     tsBSpline target = ...
 *     tsBSpline morph = ts_bspline_init();
 *     for (t = (tsReal) 0.0; t <= (tsReal) 1.0; t += (tsReal) 0.001)
 *         ts_bspline_morph(&origin, &target, t, ..., &morph, ...);
 *     ts_bspline_free(&morph);
 *
 * It should be noted that this function, if necessary, aligns \p origin and \p
 * target using ::ts_bspline_align. In order to avoid the overhead of spline
 * alignment, \p origin and \p target should be aligned in advance.
 *
 * @param[in] origin
 * 	Origin spline.
 * @param[in] target
 * 	Target spline.
 * @param[in] t
 * 	The time parameter. If 0, \p out becomes \p origin. If 1, \p out becomes
 * 	\p target. Note that the value passed is clamped to the domain [0, 1].
 * @param[in] epsilon
 * 	If \p origin and \p target must be aligned, this parameter is passed
 * 	::ts_bspline_elevate_degree to check whether two control points, \c p1
 * 	and \c p2, are "equal", that is, the distance between \c p1 and \c p2
 * 	is less than or equal to \p epsilon. A viable default value is
 * 	::TS_POINT_EPSILON.
 * @param[out] out
 * 	The resulting spline.
 * @return[out] TS_SUCCESS
 * 	On success.
 * @return TS_MALLOC
 * 	If memory allocation failed.
 */
tsError TINYSPLINE_API
ts_bspline_morph(const tsBSpline *origin,
                 const tsBSpline *target,
                 tsReal t,
                 tsReal epsilon,
                 tsBSpline *out,
                 tsStatus *status);
/*! @} */



/*! @name Serialization and Persistence
 *
 * The following functions can be used to serialize and persist (i.e., store
 * the serialized data in a file) splines. There are also functions to load
 * serialized splines.
 *
 * @{
 */
/**
 * Serializes \p spline to a null-terminated JSON string and stores the result
 * in \p json.
 *
 * @param[in] spline
 * 	The spline to be serialized.
 * @param[out] json
 * 	The serialized JSON string.
 * @param[out] status
 * 	The status of this function. May be NULL.
 * @return TS_SUCCESS
 * 	On success.
 * @return TS_MALLOC
 * 	If allocating memory failed.
 */
tsError TINYSPLINE_API
ts_bspline_to_json(const tsBSpline *spline,
                   char **json,
                   tsStatus *status);

/**
 * Parses \p json and stores the result in \p spline.
 *
 * @param[in] json
 * 	The JSON string to be parsed.
 * @param[out] spline
 * 	The output spline.
 * @param[out] status
 * 	The status of this function. May be NULL.
 * @return TS_SUCCESS
 * 	On success.
 * @return TS_PARSE_ERROR
 * 	If an error occurred while parsing \p json.
 * @return TS_DIM_ZERO
 * 	If the dimension is \c 0.
 * @return TS_LCTRLP_DIM_MISMATCH
 * 	If the length of the control point array modulo dimension is not \c 0.
 * @return TS_DEG_GE_NCTRLP
 * 	If the degree is greater or equals to the number of control points.
 * @return TS_NUM_KNOTS
 * 	If the number of knots does not match to the number of control points
 * 	plus the degree of the spline.
 * @return TS_KNOTS_DECR
 * 	If the knot vector is decreasing.
 * @return TS_MULTIPLICITY
 * 	If there is a knot with multiplicity greater than order.
 * @return TS_MALLOC
 * 	If allocating memory failed.
 */
tsError TINYSPLINE_API
ts_bspline_parse_json(const char *json,
                      tsBSpline *spline,
                      tsStatus *status);

/**
 * Saves \p spline as JSON ASCII file.
 *
 * @param[in] spline
 * 	The spline to be saved.
 * @param[in] path
 * 	Path of the JSON file.
 * @param[out] status
 * 	The status of this function. May be NULL.
 * @return TS_SUCCESS
 * 	On success.
 * @return TS_IO_ERROR
 * 	If an error occurred while saving \p spline.
 * @return TS_MALLOC
 * 	If allocating memory failed.
 */
tsError TINYSPLINE_API
ts_bspline_save(const tsBSpline *spline,
                const char *path,
                tsStatus *status);

/**
 * Loads \p spline from a JSON ASCII file.
 *
 * @param[in] path
 * 	Path of the JSON file to be loaded.
 * @param[out] spline
 * 	The output spline.
 * @param[ou] status
 * 	The status of this function. May be NULL.
 * @return TS_SUCCESS
 * 	On success.
 * @return TS_IO_ERROR
 * 	If \p path does not exist.
 * @return TS_PARSE_ERROR
 * 	If an error occurred while parsing the contents of \p path.
 * @return TS_DIM_ZERO
 * 	If the dimension is \c 0.
 * @return TS_LCTRLP_DIM_MISMATCH
 * 	If the length of the control point array modulo dimension is not \c 0.
 * @return TS_DEG_GE_NCTRLP
 * 	If the degree is greater or equals to the number of control points.
 * @return TS_NUM_KNOTS
 * 	If the number of knots does not match to the number of control points
 * 	plus the degree of the spline.
 * @return TS_KNOTS_DECR
 * 	If the knot vector is decreasing.
 * @return TS_MULTIPLICITY
 * 	If there is a knot with multiplicity greater than order.
 * @return TS_MALLOC
 * 	If allocating memory failed.
 */
tsError TINYSPLINE_API
ts_bspline_load(const char *path,
                tsBSpline *spline,
                tsStatus *status);



/*! @name Vector Math
 *
 * Vector math is a not insignificant part of TinySpline, and so it's not
 * surprising that some utility functions around vectors are needed. Because
 * these functions might be useful for others, they are part of TinySpline's
 * public API. However, note that the code is \b not highly optimized (with,
 * for example, instruction set extensions like SSE). If high performance
 * vector math is needed, other libraries should be used instead.
 *
 * @{
 */
/**
 * Initializes vector \p out with \p x and \p y.
 *
 * @pre
 * 	\p out has dimensionality \c 2.
 * @param[out] out
 * 	Target vector.
 * @param[in] x
 * 	The x value.
 * @param[in] y
 * 	The y value.
 */
void TINYSPLINE_API
ts_vec2_init(tsReal *out,
             tsReal x,
             tsReal y);

/**
 * Initializes vector \p out with \p x, \p y, and \p z.
 *
 * @pre
 * 	\p out has dimensionality \c 3.
 * @param[out] out
 * 	Target vector.
 * @param[in] x
 * 	The x value.
 * @param[in] y
 * 	The y value.
 * @param[in] z
 * 	The z value.
 */
void TINYSPLINE_API
ts_vec3_init(tsReal *out,
             tsReal x,
             tsReal y,
             tsReal z);

/**
 * Initializes vector \p out with \p x, \p y, \p z, and \p w.
 *
 * @pre
 * 	\p out has dimensionality \c 4.
 * @param[out] out
 * 	Target vector.
 * @param[in] x
 * 	The x value.
 * @param[in] y
 * 	The y value.
 * @param[in] z
 * 	The z value.
 * @param[in] w
 * 	The w value.
 */
void TINYSPLINE_API
ts_vec4_init(tsReal *out,
             tsReal x,
             tsReal y,
             tsReal z,
             tsReal w);

/**
 * Copies the values of vector \p x (a vector with dimensionality \p dim) to
 * vector \p out (a vector with dimensionality \c 2). If \p dim is less than \c
 * 2, the remaining values of \p out are set to \c 0. Excess values in \p x
 * (i.e., \p dim is greater than \c 2) are ignored.
 *
 * @pre
 * 	\p out has dimensionality \c 2.
 * @param[out] out
 * 	Target vector.
 * @param[in] x
 * 	Vector to read the values from.
 * @param[in] dim
 * 	Dimensionality of \p x.
 */
void TINYSPLINE_API
ts_vec2_set(tsReal *out,
            const tsReal *x,
            size_t dim);

/**
 * Copies the values of vector \p x (a vector with dimensionality \p dim) to
 * vector \p out (a vector with dimensionality \c 3). If \p dim is less than \c
 * 3, the remaining values of \p out are set to \c 0. Excess values in \p x
 * (i.e., \p dim is greater than \c 3) are ignored.
 *
 * @pre
 * 	\p out has dimensionality \c 3.
 * @param[out] out
 * 	Target vector.
 * @param[in] x
 * 	Vector to read the values from.
 * @param[in] dim
 * 	Dimensionality of \p x.
 */
void TINYSPLINE_API
ts_vec3_set(tsReal *out,
            const tsReal *x,
            size_t dim);

/**
 * Copies the values of vector \p x (a vector with dimensionality \p dim) to
 * vector \p out (a vector with dimensionality \c 4). If \p dim is less than \c
 * 4, the remaining values of \p out are set to \c 0. Excess values in \p x
 * (i.e., \p dim is greater than \c 4) are ignored.
 *
 * @pre
 * 	\p out has dimensionality \c 4.
 * @param[out] out
 * 	Target vector.
 * @param[in] x
 * 	Vector to read the values from.
 * @param[in] dim
 * 	Dimensionality of \p x.
 */
void TINYSPLINE_API
ts_vec4_set(tsReal *out,
            const tsReal *x,
            size_t dim);

/**
 * Adds vector \p y to vector \p x and stores the result in vector \p out.
 *
 * @param[in] x
 * 	First vector.
 * @param[in] y
 * 	Second vector.
 * @param[in] dim
 * 	Dimensionality of \p x, \p y, and \p out.
 * @param[out] out
 * 	Result vector. Can be same as \p x or \p y, i.e., the result can be
 * 	stored in-place.
 */
void TINYSPLINE_API
ts_vec_add(const tsReal *x,
           const tsReal *y,
           size_t dim,
           tsReal *out);

/**
 * Subtracts vector \p y from vector \p x and stores the result in vector \p
 * out.
 *
 * @param[in] x
 * 	First vector.
 * @param[in] y
 * 	Second vector.
 * @param[in] dim
 * 	Dimensionality of \p x, \p y, and \p out.
 * @param[out] out
 * 	Result vector. Can be same as \p x or \p y, i.e., the result can be
 * 	stored in-place.
 */
void TINYSPLINE_API
ts_vec_sub(const tsReal *x,
           const tsReal *y,
           size_t dim,
           tsReal *out);

/**
 * Computes the dot product (also known as scalar product) of the vectors \p x
 * and \p y.
 *
 * @post
 * 	\c 0 if \p dim is \c 0.
 * @param[in] x
 * 	First vector.
 * @param[in] y
 * 	Second vector.
 * @param[in] dim
 * 	Dimensionality of \p x and \p y.
 * @return
 * 	The dot product of \p x and \y.
 */
tsReal TINYSPLINE_API
ts_vec_dot(const tsReal *x,
           const tsReal *y,
           size_t dim);

/**
 * Computes the angle in degrees between the vectors \p x and \p y. The angle
 * returned is unsigned, that is, the smaller of the two possible angles is
 * computed. The nullable parameter \p buf servers as a buffer in case \p x or
 * \p y (or both) are not normalized. If \p buf is \c NULL, it is expected that
 * \p x and \p y are already normalized. If \p buf is not \c NULL, a storage
 * twice the size of \p dim is expected in which the normalized vectors of \p x
 * and \p y are stored.
 *
 * @pre
 * 	\p buf is either \c NULL or has length <tt>2 * dim</tt>.
 * @param[in] x
 * 	First vector.
 * @param[in] y
 * 	Second vector.
 * @param[out] buf
 * 	A buffer in which the normalized vectors of \p x and \y are stored. If
 * 	\c NULL, it is expected that \p x and \p y are already normalized.
 * @param[in] dim
 * 	Dimensionality of \p x and \p y.
 * @return
 * 	The angle between \p x and \y with <tt>0.0 <= angle <= 180.0</tt>.
 */
tsReal TINYSPLINE_API
ts_vec_angle(const tsReal *x,
             const tsReal *y,
             tsReal *buf,
             size_t dim);

/**
 * Computes the cross product (also known as vector product or directed area
 * product) of the vectors \p x and \p y.
 *
 * @pre \p x and \p y have dimensionality \c 3.
 * @param[in] x
 * 	First vector.
 * @param[in] y
 * 	Second vector.
 * @param[out] out
 * 	Result vector. Can be same as \p x or \p y, i.e., the result can be
 * 	stored in-place.
 */
void TINYSPLINE_API
ts_vec3_cross(const tsReal *x,
              const tsReal *y,
              tsReal *out);

/**
 * Normalizes vector \p x.
 *
 * @post
 * 	\c 0 if the length of \p x (see ::ts_vec_mag) is less than
 * 	::TS_LENGTH_ZERO.
 * @param[in] x
 * 	A vector.
 * @param[in] dim
 * 	Dimensionality of \p x.
 * @param[out] out
 * 	Result vector. Can be same as \p x, i.e., the result can be stored
 * 	in-place.
 */
void TINYSPLINE_API
ts_vec_norm(const tsReal *x,
            size_t dim,
            tsReal *out);

/**
 * Determines the length of vector \p x.
 *
 * @post
 * 	\c 0 if \p dim is \c 0.
 * @param[in] x
 * 	A vector.
 * @param[in] dim
 * 	Dimensionality of \p x.
 */
tsReal TINYSPLINE_API
ts_vec_mag(const tsReal *x,
           size_t dim);

/**
 * Multiplies vector \p x with scalar \p val and stores the result in vector \p
 * out.
 *
 * @param[in] x
 * 	A vector.
 * @param[in] dim
 * 	Dimensionality of \p x.
 * @param[in] val
 * 	Scalar value.
 * @param[out] out
 * 	Result vector. Can be same as \p x, i.e., the result can be stored
 * 	in-place.
 */
void TINYSPLINE_API
ts_vec_mul(const tsReal *x,
           size_t dim,
           tsReal val,
           tsReal *out);
/*! @} */



/*! @name Chord Length Method
 *
 * Functions for processing the cumulative chord lengths of a spline
 * as computed by ::ts_bspline_chord_lengths.
 *
 * @{
 */
/**
 * Maps \p len to a knot, \c k, such that <tt>ts_bspline_eval(..., k ...)</tt>
 * yields a point whose length, with respect to <tt>knots[0]</tt>, is close to
 * \p len. Note that \p len is clamped to the domain of \p lengths. The domain
 * of the result, \p knot, is <tt>[knots[0], knots[num-1]]</tt>.
 *
 * The precision of the mapping depends on the resolution of \p knots and \p
 * lengths. That is, the more chord lengths were computed, the more precise the
 * length-to-knot-mapping becomes. Generally, \c 200 chord lengths yields a
 * quite precise mapping (subpixel accuracy). For very, very high precision
 * requirements, \c 500 chord lengths should be sufficient.
 *
 * Returns ::TS_NO_RESULT if \p num is \c 0.
 *
 * @pre
 * 	\p lengths is monotonically increasing and contains only non-negative
 * 	values (distances cannot be negative).
 * @param[in] knots
 * 	Knots that were passed to ::ts_bspline_chord_lengths.
 * @param[in] lengths
 * 	Cumulative chord lengths as computed by ::ts_bspline_chord_lengths.
 * @param[in] num
 * 	Number of values in \p knots and \p lengths.
 * @param[in] len
 * 	Length to be mapped. Clamped to the domain of \p lengths.
 * @param[out] knot
 * 	A knot, such that <tt>ts_bspline_eval(..., knot ...)</tt> yields a
 * 	point whose length, with respect to <tt>knots[0]</tt>, is close to \p
 * 	len.
 * @param[out] status
 * 	The status of this function. May be NULL.
 * @return TS_SUCCESS
 * 	On success.
 * @return TS_NO_RESULT
 * 	If \p num is \c 0.
 */
tsError
ts_chord_lengths_length_to_knot(const tsReal *knots,
                                const tsReal *lengths,
                                size_t num,
                                tsReal len,
                                tsReal *knot,
                                tsStatus *status);

/**
 * Same as ::ts_chord_lengths_length_to_knot, except that this function takes a
 * chord length parameter, \p t, with domain [0, 1] (clamped), which indicates
 * the to be evaluated relative proportion of the total length
 * (<tt>lengths[num-1]</tt>).
 *
 * @pre
 * 	\p lengths is monotonically increasing and contains only non-negative
 * 	values (distances cannot be negative).
 * @param[in] knots
 * 	Knots that were passed to ::ts_bspline_chord_lengths.
 * @param[in] lengths
 * 	Cumulative chord lengths as computed by ::ts_bspline_chord_lengths.
 * @param[in] num
 * 	Number of values in \p knots and \p lengths.
 * @param[in] t
 * 	Chord length parameter (relative proportion of the total length).
 * 	Clamped to the domain [0, 1].
 * @param[out] knot
 * 	A knot, such that <tt>ts_bspline_eval(..., knot ...)</tt> yields a
 * 	point whose length, with respect to <tt>knots[0]</tt>, is close to
 * 	<tt>t * lengths[num-1]<tt>
 * @param[out] status
 * 	The status of this function. May be NULL.
 * @return TS_SUCCESS
 * 	On success.
 * @return TS_NO_RESULT
 * 	If \p num is \c 0.
 */
tsError TINYSPLINE_API
ts_chord_lengths_t_to_knot(const tsReal *knots,
                           const tsReal *lengths,
                           size_t num,
                           tsReal t,
                           tsReal *knot,
                           tsStatus *status);

/**
 * Generates a sequence of \p num_knot_seq knots with equidistant distribution.
 * \e Equidistant means that the points evaluated from consecutive knots in \p
 * knot_seq have the same distance along the spline. This is also known as
 * 'reparametrization by arc length'.
 *
 * @pre
 * 	\p lengths is monotonically increasing and contains only non-negative
 * 	values (distances cannot be negative).
 * @param[in] knots
 * 	Knots that were passed to ::ts_bspline_chord_lengths.
 * @param[in] lengths
 * 	Cumulative chord lengths as computed by ::ts_bspline_chord_lengths.
 * @param[in] num
 * 	Number of values in \p knots and \p lengths.
 * @param[in] num_knot_seq
 * 	Number of knots in \p knot_seq.
 * @param[out] knot_seq
 * 	Stores the generated knot sequence.
 * @param[out] status
 * 	The status of this function. May be NULL.
 * @return TS_SUCCESS
 * 	On success.
 * @return TS_NO_RESULT
 * 	If \p num is \c 0.
 */
tsError TINYSPLINE_API
ts_chord_lengths_equidistant_knot_seq(const tsReal *knots,
                                      const tsReal *lengths,
                                      size_t num,
                                      size_t num_knot_seq,
                                      tsReal *knot_seq,
                                      tsStatus *status);
/*! @} */



/*! @name Utility Functions
 *
 * @{
 */
/**
 * Returns whether the knots \p x and \p y are equal with respect to the epsilon
 * environment ::TS_KNOT_EPSILON (i.e., their distance is <u>less</u> than
 * ::TS_KNOT_EPSILON).
 *
 * @param[in] x
 * 	First knot.
 * @param[in] y
 * 	Second knot.
 * @return 1
 * 	If \p x and \p y are equal.
 * @return 0
 * 	If \p x and \p y are not equal.
 */
int TINYSPLINE_API
ts_knots_equal(tsReal x,
               tsReal y);

/**
 * Fills the given array \p arr with \p val.
 *
 * @param[in] arr
 * 	The array to be filled.
 * @param[in] num
 * 	Fill length.
 * @param[in] val
 * 	The value to fill into \p arr.
 */
void TINYSPLINE_API
ts_arr_fill(tsReal *arr,
            size_t num,
            tsReal val);

/**
 * Returns the euclidean distance of the points \p x and \p y.
 *
 * @param[in] x
 * 	First point.
 * @param[in] y
 * 	Second point.
 * @param[in] dim
 * 	Dimensionality of \p x and \p y.
 * @return
 * 	The euclidean distance of the points \p x and \p y.
 */
tsReal TINYSPLINE_API
ts_distance(const tsReal *x,
            const tsReal *y,
            size_t dim);
/*! @} */



#ifdef	__cplusplus
}
#endif

#endif	/* TINYSPLINE_H */

