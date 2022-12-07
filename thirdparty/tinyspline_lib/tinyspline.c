#define TINYSPLINE_EXPORT
#include "tinyspline.h"
#include "parson.h" /* serialization */

#include <stdlib.h> /* malloc, free */
#include <math.h>   /* fabs, sqrt, acos */
#include <string.h> /* memcpy, memmove */
#include <stdio.h>  /* FILE, fopen */
#include <stdarg.h> /* varargs */

/* Suppress some useless MSVC warnings. */
#ifdef _MSC_VER
#pragma warning(push)
/* address of dllimport */
#pragma warning(disable:4232)
/* function not inlined */
#pragma warning(disable:4710)
/* byte padding */
#pragma warning(disable:4820)
/* meaningless deprecation */
#pragma warning(disable:4996)
/* Spectre mitigation */
#pragma warning(disable:5045)
#endif

#define INIT_OUT_BSPLINE(in, out)              \
	if ((in) != (out))                     \
		ts_int_bspline_init(out);



/*! @name Internal Structs and Functions
 *
 * Internal functions are prefixed with \e ts_int (int for internal).
 *
 * @{
 */
/**
 * Stores the private data of ::tsBSpline.
 */
struct tsBSplineImpl
{
	size_t deg; /**< Degree of B-Spline basis function. */
	size_t dim; /**< Dimensionality of the control points (2D => x, y). */
	size_t n_ctrlp; /**< Number of control points. */
	size_t n_knots; /**< Number of knots (n_ctrlp + deg + 1). */
};

/**
 * Stores the private data of ::tsDeBoorNet.
 */
struct tsDeBoorNetImpl
{
	tsReal u; /**< The evaluated knot. */
	size_t k; /**< The index [u_k, u_k+1) */
	size_t s; /**< Multiplicity of u_k. */
	size_t h; /**< Number of insertions required to obtain result. */
	size_t dim; /**< Dimensionality of the points (2D => x, y). */
	size_t n_points; /** Number of points in `points'. */
};

void
ts_int_bspline_init(tsBSpline *spline)
{
	spline->pImpl = NULL;
}

size_t
ts_int_bspline_sof_state(const tsBSpline *spline)
{
	return sizeof(struct tsBSplineImpl) +
	       ts_bspline_sof_control_points(spline) +
	       ts_bspline_sof_knots(spline);
}

tsReal *
ts_int_bspline_access_ctrlp(const tsBSpline *spline)
{
	return (tsReal *) (& spline->pImpl[1]);
}

tsReal *
ts_int_bspline_access_knots(const tsBSpline *spline)
{
	return ts_int_bspline_access_ctrlp(spline) +
	       ts_bspline_len_control_points(spline);
}

tsError
ts_int_bspline_access_ctrlp_at(const tsBSpline *spline,
                               size_t index,
                               tsReal **ctrlp,
                               tsStatus *status)
{
	const size_t num = ts_bspline_num_control_points(spline);
	if (index >= num) {
		TS_RETURN_2(status, TS_INDEX_ERROR,
		            "index (%lu) >= num(control_points) (%lu)",
		            (unsigned long) index,
		            (unsigned long) num)
	}
	*ctrlp = ts_int_bspline_access_ctrlp(spline) +
	         index * ts_bspline_dimension(spline);
	TS_RETURN_SUCCESS(status)
}

tsError
ts_int_bspline_access_knot_at(const tsBSpline *spline,
                              size_t index,
                              tsReal *knot,
                              tsStatus *status)
{
	const size_t num = ts_bspline_num_knots(spline);
	if (index >= num) {
		TS_RETURN_2(status, TS_INDEX_ERROR,
		            "index (%lu) >= num(knots) (%lu)",
		            (unsigned long) index,
		            (unsigned long) num)
	}
	*knot = ts_int_bspline_access_knots(spline)[index];
	TS_RETURN_SUCCESS(status)
}

void
ts_int_deboornet_init(tsDeBoorNet *net)
{
	net->pImpl = NULL;
}

size_t
ts_int_deboornet_sof_state(const tsDeBoorNet *net)
{
	return sizeof(struct tsDeBoorNetImpl) +
	       ts_deboornet_sof_points(net) +
	       ts_deboornet_sof_result(net);
}

tsReal *
ts_int_deboornet_access_points(const tsDeBoorNet *net)
{
	return (tsReal *) (& net->pImpl[1]);
}

tsReal *
ts_int_deboornet_access_result(const tsDeBoorNet *net)
{
	if (ts_deboornet_num_result(net) == 2) {
		return ts_int_deboornet_access_points(net);
	} else {
		return ts_int_deboornet_access_points(net) +
		       /* Last point in `points`. */
		       (ts_deboornet_len_points(net) -
		        ts_deboornet_dimension(net));
	}
}
/*! @} */



/*! @name B-Spline Data
 *
 * @{
 */
size_t
ts_bspline_degree(const tsBSpline *spline)
{
	return spline->pImpl->deg;
}

size_t
ts_bspline_order(const tsBSpline *spline)
{
	return ts_bspline_degree(spline) + 1;
}

size_t
ts_bspline_dimension(const tsBSpline *spline)
{
	return spline->pImpl->dim;
}

size_t
ts_bspline_len_control_points(const tsBSpline *spline)
{
	return ts_bspline_num_control_points(spline) *
	       ts_bspline_dimension(spline);
}

size_t
ts_bspline_num_control_points(const tsBSpline *spline)
{
	return spline->pImpl->n_ctrlp;
}

size_t
ts_bspline_sof_control_points(const tsBSpline *spline)
{
	return ts_bspline_len_control_points(spline) * sizeof(tsReal);
}

const tsReal *
ts_bspline_control_points_ptr(const tsBSpline *spline)
{
	return ts_int_bspline_access_ctrlp(spline);
}

tsError
ts_bspline_control_points(const tsBSpline *spline,
                          tsReal **ctrlp,
                          tsStatus *status)
{
	const size_t size = ts_bspline_sof_control_points(spline);
	*ctrlp = (tsReal*) malloc(size);
	if (!*ctrlp) TS_RETURN_0(status, TS_MALLOC, "out of memory")
	memcpy(*ctrlp, ts_int_bspline_access_ctrlp(spline), size);
	TS_RETURN_SUCCESS(status)
}

tsError
ts_bspline_control_point_at_ptr(const tsBSpline *spline,
                                size_t index,
                                const tsReal **ctrlp,
                                tsStatus *status)
{
	tsReal *vals;
	tsError err;
	TS_TRY(try, err, status)
		TS_CALL(try, err, ts_int_bspline_access_ctrlp_at(
		        spline, index, &vals, status))
		*ctrlp = vals;
	TS_CATCH(err)
		*ctrlp = NULL;
	TS_END_TRY_RETURN(err)
}

tsError
ts_bspline_set_control_points(tsBSpline *spline,
                              const tsReal *ctrlp,
                              tsStatus *status)
{
	const size_t size = ts_bspline_sof_control_points(spline);
	memmove(ts_int_bspline_access_ctrlp(spline), ctrlp, size);
	TS_RETURN_SUCCESS(status)
}

tsError
ts_bspline_set_control_point_at(tsBSpline *spline,
                                size_t index,
                                const tsReal *ctrlp,
                                tsStatus *status)
{
	tsReal *to;
	size_t size;
	tsError err;
	TS_TRY(try, err, status)
		TS_CALL(try, err, ts_int_bspline_access_ctrlp_at(
		        spline, index, &to, status))
		size = ts_bspline_dimension(spline) * sizeof(tsReal);
		memcpy(to, ctrlp, size);
	TS_END_TRY_RETURN(err)
}

size_t
ts_bspline_num_knots(const tsBSpline *spline)
{
	return spline->pImpl->n_knots;
}

size_t
ts_bspline_sof_knots(const tsBSpline *spline)
{
	return ts_bspline_num_knots(spline) * sizeof(tsReal);
}

const tsReal *
ts_bspline_knots_ptr(const tsBSpline *spline)
{
	return ts_int_bspline_access_knots(spline);
}

tsError
ts_bspline_knots(const tsBSpline *spline,
                 tsReal **knots,
                 tsStatus *status)
{
	const size_t size = ts_bspline_sof_knots(spline);
	*knots = (tsReal*) malloc(size);
	if (!*knots) TS_RETURN_0(status, TS_MALLOC, "out of memory")
	memcpy(*knots, ts_int_bspline_access_knots(spline), size);
	TS_RETURN_SUCCESS(status)
}

tsError
ts_bspline_knot_at(const tsBSpline *spline,
                   size_t index,
                   tsReal *knot,
                   tsStatus *status)
{
	return ts_int_bspline_access_knot_at(spline, index, knot, status);
}

tsError
ts_bspline_set_knots(tsBSpline *spline,
                     const tsReal *knots,
                     tsStatus *status)
{
	const size_t size = ts_bspline_sof_knots(spline);
	const size_t num_knots = ts_bspline_num_knots(spline);
	const size_t order = ts_bspline_order(spline);
	size_t idx, mult;
	tsReal lst_knot, knot;
	lst_knot = knots[0];
	mult = 1;
	for (idx = 1; idx < num_knots; idx++) {
		knot = knots[idx];
		if (ts_knots_equal(lst_knot, knot)) {
			mult++;
		} else if (lst_knot > knot) {
			TS_RETURN_1(status, TS_KNOTS_DECR,
			            "decreasing knot vector at index: %lu",
			            (unsigned long) idx)
		} else {
			mult = 0;
		}
		if (mult > order) {
			TS_RETURN_3(status, TS_MULTIPLICITY,
			            "mult(%f) (%lu) > order (%lu)",
			            knot, (unsigned long) mult,
			            (unsigned long) order)
		}
		lst_knot = knot;
	}
	memmove(ts_int_bspline_access_knots(spline), knots, size);
	TS_RETURN_SUCCESS(status)
}

tsError
ts_bspline_set_knots_varargs(tsBSpline *spline,
                             tsStatus *status,
                             tsReal knot0,
                             double knot1,
                             ...)
{
	tsReal *values = NULL;
	va_list argp;
	size_t idx;
	tsError err;

	TS_TRY(try, err, status)
		TS_CALL(try, err, ts_bspline_knots(
		        spline, &values, status))

		values[0] = knot0;
		values[1] = (tsReal) knot1;
		va_start(argp, knot1);
		for (idx = 2; idx < ts_bspline_num_knots(spline); idx++)
			values[idx] = (tsReal) va_arg(argp, double);
		va_end(argp);

		TS_CALL(try, err, ts_bspline_set_knots(
		        spline, values, status))
	TS_FINALLY
		if (values) free(values);
	TS_END_TRY_RETURN(err)
}

tsError
ts_bspline_set_knot_at(tsBSpline *spline,
                       size_t index,
                       tsReal knot,
                       tsStatus *status)
{
	tsError err;
	tsReal *knots = NULL;
	/* This is only for initialization. */
	tsReal oldKnot = ts_int_bspline_access_knots(spline)[0];
	TS_TRY(try, err, status)
		TS_CALL(try, err, ts_int_bspline_access_knot_at(
		        spline, index, &oldKnot, status))
		/* knots must be set after reading oldKnot because the catch
		 * block assumes that oldKnot contains the correct value if
		 * knots is not NULL. */
		knots = ts_int_bspline_access_knots(spline);
		knots[index] = knot;
		TS_CALL(try, err, ts_bspline_set_knots(
		        spline, knots, status))
	TS_CATCH(err)
		/* If knots is not NULL, oldKnot contains the correct value. */
		if (knots) knots[index] = oldKnot;
	TS_END_TRY_RETURN(err)
}
/*! @} */



/*! @name B-Spline Initialization
 *
 * @{
 */
tsBSpline
ts_bspline_init(void)
{
	tsBSpline spline;
	ts_int_bspline_init(&spline);
	return spline;
}

tsError
ts_int_bspline_generate_knots(const tsBSpline *spline,
                              tsBSplineType type,
                              tsStatus *status)
{
	const size_t n_knots = ts_bspline_num_knots(spline);
	const size_t deg = ts_bspline_degree(spline);
	const size_t order = ts_bspline_order(spline);
	tsReal fac; /**< Factor used to calculate the knot values. */
	size_t i; /**< Used in for loops. */
	tsReal *knots; /**< Pointer to the knots of \p _result_. */

	/* order >= 1 implies 2*order >= 2 implies n_knots >= 2 */
	if (type == TS_BEZIERS && n_knots % order != 0) {
		TS_RETURN_2(status, TS_NUM_KNOTS,
		            "num(knots) (%lu) %% order (%lu) != 0",
		            (unsigned long) n_knots, (unsigned long) order)
	}

	knots = ts_int_bspline_access_knots(spline);

	if (type == TS_OPENED) {
		knots[0] = TS_DOMAIN_DEFAULT_MIN; /* n_knots >= 2 */
		fac = (TS_DOMAIN_DEFAULT_MAX - TS_DOMAIN_DEFAULT_MIN)
		      / (n_knots - 1); /* n_knots >= 2 */
		for (i = 1; i < n_knots-1; i++)
			knots[i] = TS_DOMAIN_DEFAULT_MIN + i*fac;
		knots[i] = TS_DOMAIN_DEFAULT_MAX; /* n_knots >= 2 */
	} else if (type == TS_CLAMPED) {
		/* n_knots >= 2*order == 2*(deg+1) == 2*deg + 2 > 2*deg - 1 */
		fac = (TS_DOMAIN_DEFAULT_MAX - TS_DOMAIN_DEFAULT_MIN)
		      / (n_knots - 2*deg - 1);
		ts_arr_fill(knots, order, TS_DOMAIN_DEFAULT_MIN);
		for (i = order ;i < n_knots-order; i++)
			knots[i] = TS_DOMAIN_DEFAULT_MIN + (i-deg)*fac;
		ts_arr_fill(knots + i, order, TS_DOMAIN_DEFAULT_MAX);
	} else if (type == TS_BEZIERS) {
		/* n_knots >= 2*order implies n_knots/order >= 2 */
		fac = (TS_DOMAIN_DEFAULT_MAX - TS_DOMAIN_DEFAULT_MIN)
		      / (n_knots/order - 1);
		ts_arr_fill(knots, order, TS_DOMAIN_DEFAULT_MIN);
		for (i = order; i < n_knots-order; i += order)
			ts_arr_fill(knots + i,
			            order,
			            TS_DOMAIN_DEFAULT_MIN + (i/order)*fac);
		ts_arr_fill(knots + i, order, TS_DOMAIN_DEFAULT_MAX);
	}
	TS_RETURN_SUCCESS(status)
}

tsError
ts_bspline_new(size_t num_control_points,
               size_t dimension,
               size_t degree,
               tsBSplineType type,
               tsBSpline *spline,
               tsStatus *status)
{
	const size_t order = degree + 1;
	const size_t num_knots = num_control_points + order;
	const size_t len_ctrlp = num_control_points * dimension;

	const size_t sof_real = sizeof(tsReal);
	const size_t sof_impl = sizeof(struct tsBSplineImpl);
	const size_t sof_ctrlp_vec = len_ctrlp * sof_real;
	const size_t sof_knots_vec = num_knots * sof_real;
	const size_t sof_spline = sof_impl + sof_ctrlp_vec + sof_knots_vec;
	tsError err;

	ts_int_bspline_init(spline);

	if (dimension < 1) {
		TS_RETURN_0(status, TS_DIM_ZERO, "unsupported dimension: 0")
	}
	if (num_knots > TS_MAX_NUM_KNOTS) {
		TS_RETURN_2(status, TS_NUM_KNOTS,
		            "unsupported number of knots: %lu > %i",
		            (unsigned long) num_knots, TS_MAX_NUM_KNOTS)
	}
	if (degree >= num_control_points) {
		TS_RETURN_2(status, TS_DEG_GE_NCTRLP,
		            "degree (%lu) >= num(control_points) (%lu)",
		            (unsigned long) degree,
		            (unsigned long) num_control_points)
	}

	spline->pImpl = (struct tsBSplineImpl *) malloc(sof_spline);
	if (!spline->pImpl) TS_RETURN_0(status, TS_MALLOC, "out of memory")

	spline->pImpl->deg = degree;
	spline->pImpl->dim = dimension;
	spline->pImpl->n_ctrlp = num_control_points;
	spline->pImpl->n_knots = num_knots;

	TS_TRY(try, err, status)
		TS_CALL(try, err, ts_int_bspline_generate_knots(
		        spline, type, status))
	TS_CATCH(err)
		ts_bspline_free(spline);
	TS_END_TRY_RETURN(err)
}

tsError
ts_bspline_new_with_control_points(size_t num_control_points,
                                   size_t dimension,
                                   size_t degree,
                                   tsBSplineType type,
                                   tsBSpline *spline,
                                   tsStatus *status,
                                   double first,
                                   ...)
{
	tsReal *ctrlp = NULL;
	va_list argp;
	size_t i;
	tsError err;

	TS_TRY(try, err, status)
		TS_CALL(try, err, ts_bspline_new(
		        num_control_points, dimension,
		        degree, type, spline, status))
	TS_CATCH(err)
		ts_bspline_free(spline);
	TS_END_TRY_ROE(err)
	ctrlp = ts_int_bspline_access_ctrlp(spline);

	ctrlp[0] = (tsReal) first;
	va_start(argp, first);
	for (i = 1; i < ts_bspline_len_control_points(spline); i++)
		ctrlp[i] = (tsReal) va_arg(argp, double);
	va_end(argp);

	TS_RETURN_SUCCESS(status)
}

tsError
ts_bspline_copy(const tsBSpline *src,
                tsBSpline *dest,
                tsStatus *status)
{
	size_t size;
	if (src == dest) TS_RETURN_SUCCESS(status)
	ts_int_bspline_init(dest);
	size = ts_int_bspline_sof_state(src);
	dest->pImpl = (struct tsBSplineImpl *) malloc(size);
	if (!dest->pImpl) TS_RETURN_0(status, TS_MALLOC, "out of memory")
	memcpy(dest->pImpl, src->pImpl, size);
	TS_RETURN_SUCCESS(status)
}

void
ts_bspline_move(tsBSpline *src,
                tsBSpline *dest)
{
	if (src == dest) return;
	dest->pImpl = src->pImpl;
	ts_int_bspline_init(src);
}

void
ts_bspline_free(tsBSpline *spline)
{
	if (spline->pImpl) free(spline->pImpl);
	ts_int_bspline_init(spline);
}
/*! @} */



/*! @name De Boor Net Data
 *
 * @{
 */
tsReal
ts_deboornet_knot(const tsDeBoorNet *net)
{
	return net->pImpl->u;
}

size_t
ts_deboornet_index(const tsDeBoorNet *net)
{
	return net->pImpl->k;
}

size_t
ts_deboornet_multiplicity(const tsDeBoorNet *net)
{
	return net->pImpl->s;
}

size_t
ts_deboornet_num_insertions(const tsDeBoorNet *net)
{
	return net->pImpl->h;
}

size_t
ts_deboornet_dimension(const tsDeBoorNet *net)
{
	return net->pImpl->dim;
}

size_t
ts_deboornet_len_points(const tsDeBoorNet *net)
{
	return ts_deboornet_num_points(net) *
	       ts_deboornet_dimension(net);
}

size_t
ts_deboornet_num_points(const tsDeBoorNet *net)
{
	return net->pImpl->n_points;
}

size_t
ts_deboornet_sof_points(const tsDeBoorNet *net)
{
	return ts_deboornet_len_points(net) * sizeof(tsReal);
}

const tsReal *
ts_deboornet_points_ptr(const tsDeBoorNet *net)
{
	return ts_int_deboornet_access_points(net);
}

tsError
ts_deboornet_points(const tsDeBoorNet *net,
                    tsReal **points,
                    tsStatus *status)
{
	const size_t size = ts_deboornet_sof_points(net);
	*points = (tsReal*) malloc(size);
	if (!*points) TS_RETURN_0(status, TS_MALLOC, "out of memory")
	memcpy(*points, ts_int_deboornet_access_points(net), size);
	TS_RETURN_SUCCESS(status)
}

size_t
ts_deboornet_len_result(const tsDeBoorNet *net)
{
	return ts_deboornet_num_result(net) *
	       ts_deboornet_dimension(net);
}

size_t
ts_deboornet_num_result(const tsDeBoorNet *net)
{
	return ts_deboornet_num_points(net) == 2 ? 2 : 1;
}

size_t
ts_deboornet_sof_result(const tsDeBoorNet *net)
{
	return ts_deboornet_len_result(net) * sizeof(tsReal);
}

const tsReal *
ts_deboornet_result_ptr(const tsDeBoorNet *net)
{
	return ts_int_deboornet_access_result(net);
}

tsError
ts_deboornet_result(const tsDeBoorNet *net,
                    tsReal **result,
                    tsStatus *status)
{
	const size_t size = ts_deboornet_sof_result(net);
	*result = (tsReal*) malloc(size);
	if (!*result) TS_RETURN_0(status, TS_MALLOC, "out of memory")
	memcpy(*result, ts_int_deboornet_access_result(net), size);
	TS_RETURN_SUCCESS(status)
}
/*! @} */



/*! @name De Boor Net Initialization
 *
 * @{
 */
tsDeBoorNet
ts_deboornet_init(void)
{
	tsDeBoorNet net;
	ts_int_deboornet_init(&net);
	return net;
}

tsError
ts_int_deboornet_new(const tsBSpline *spline,
                     tsDeBoorNet *net,
                     tsStatus *status)
{
	const size_t dim = ts_bspline_dimension(spline);
	const size_t deg = ts_bspline_degree(spline);
	const size_t order = ts_bspline_order(spline);
	const size_t num_points = (size_t)(order * (order+1) * 0.5f);
	/* Handle `order == 1' which generates too few points. */
	const size_t fixed_num_points = num_points < 2 ? 2 : num_points;

	const size_t sof_real = sizeof(tsReal);
	const size_t sof_impl = sizeof(struct tsDeBoorNetImpl);
	const size_t sof_points_vec = fixed_num_points * dim * sof_real;
	const size_t sof_net = sof_impl * sof_points_vec;

	net->pImpl = (struct tsDeBoorNetImpl *) malloc(sof_net);
	if (!net->pImpl) TS_RETURN_0(status, TS_MALLOC, "out of memory")

	net->pImpl->u = 0.f;
	net->pImpl->k = 0;
	net->pImpl->s = 0;
	net->pImpl->h = deg;
	net->pImpl->dim = dim;
	net->pImpl->n_points = fixed_num_points;
	TS_RETURN_SUCCESS(status)
}

void
ts_deboornet_free(tsDeBoorNet *net)
{
	if (net->pImpl) free(net->pImpl);
	ts_int_deboornet_init(net);
}

tsError
ts_deboornet_copy(const tsDeBoorNet *src,
                  tsDeBoorNet *dest,
                  tsStatus *status)
{
	size_t size;
	if (src == dest) TS_RETURN_SUCCESS(status)
	ts_int_deboornet_init(dest);
	size = ts_int_deboornet_sof_state(src);
	dest->pImpl = (struct tsDeBoorNetImpl *) malloc(size);
	if (!dest->pImpl) TS_RETURN_0(status, TS_MALLOC, "out of memory")
	memcpy(dest->pImpl, src->pImpl, size);
	TS_RETURN_SUCCESS(status)
}

void
ts_deboornet_move(tsDeBoorNet *src,
                  tsDeBoorNet *dest)
{
	if (src == dest) return;
	dest->pImpl = src->pImpl;
	ts_int_deboornet_init(src);
}
/*! @} */



/*! @name Interpolation and Approximation Functions
 *
 * @{
 */
tsError
ts_int_cubic_point(const tsReal *point,
                   size_t dim,
                   tsBSpline *spline,
                   tsStatus *status)
{
	const size_t size = dim * sizeof(tsReal);
	tsReal *ctrlp = NULL;
	size_t i;
	tsError err;
	TS_CALL_ROE(err, ts_bspline_new(
	            4, dim, 3,
	            TS_CLAMPED, spline, status))
	ctrlp = ts_int_bspline_access_ctrlp(spline);
	for (i = 0; i < 4; i++) {
		memcpy(ctrlp + i*dim,
		       point,
		       size);
	}
	TS_RETURN_SUCCESS(status)
}

tsError
ts_int_thomas_algorithm(const tsReal *a,
                        const tsReal *b,
                        const tsReal *c,
                        size_t num,
                        size_t dim,
                        tsReal *d,
                        tsStatus *status)
{
	size_t i, j, k, l;
	tsReal m, *cc = NULL;
	tsError err;

	if (dim == 0) {
		TS_RETURN_0(status, TS_DIM_ZERO,
		            "unsupported dimension: 0")
	}
	if (num <= 1) {
		TS_RETURN_1(status, TS_NUM_POINTS,
		            "num(points) (%lu) <= 1",
		            (unsigned long) num)
	}
	cc = (tsReal *) malloc(num * sizeof(tsReal));
	if (!cc) TS_RETURN_0(status, TS_MALLOC, "out of memory")

	TS_TRY(try, err, status)
		/* Forward sweep. */
		if (fabs(b[0]) <= fabs(c[0])) {
			TS_THROW_2(try, err, status, TS_NO_RESULT,
			           "error: |%f| <= |%f|", b[0], c[0])
		}
		/* |b[i]| > |c[i]| implies that |b[i]| > 0. Thus, the following
		 * statements cannot evaluate to division by zero.*/
		cc[0] = c[0] / b[0];
		for (i = 0; i < dim; i++)
			d[i] = d[i] / b[0];
		for (i = 1; i < num; i++) {
			if (fabs(b[i]) <= fabs(a[i]) + fabs(c[i])) {
				TS_THROW_3(try, err, status, TS_NO_RESULT,
				           "error: |%f| <= |%f| + |%f|",
				           b[i], a[i], c[i])
			}
			/* |a[i]| < |b[i]| and cc[i - 1] < 1. Therefore, the
			 * following statement cannot evaluate to division by
			 * zero. */
			m = 1.f / (b[i] - a[i] * cc[i - 1]);
			/* |b[i]| > |a[i]| + |c[i]| implies that there must be
			 * an eps > 0 such that |b[i]| = |a[i]| + |c[i]| + eps.
			 * Even if |a[i]| is 0 (by which the result of the
			 * following statement becomes maximum), |c[i]| is less
			 * than |b[i]| by an amount of eps. By substituting the
			 * previous and the following statements (under the
			 * assumption that |a[i]| is 0), we obtain c[i] / b[i],
			 * which must be less than 1. */
			cc[i] = c[i] * m;
			for (j = 0; j < dim; j++) {
				k = i * dim + j;
				l = (i-1) * dim + j;
				d[k] = (d[k] - a[i] * d[l]) * m;
			}
		}

		/* Back substitution. */
		for (i = num-1; i > 0; i--) {
			for (j = 0; j < dim; j++) {
				k = (i-1) * dim + j;
				l = i * dim + j;
				d[k] -= cc[i-1] * d[l];
			}
		}
	TS_FINALLY
		free(cc);
	TS_END_TRY_RETURN(err)
}

tsError
ts_int_relaxed_uniform_cubic_bspline(const tsReal *points,
                                     size_t n,
                                     size_t dim,
                                     tsBSpline *spline,
                                     tsStatus *status)
{
	const size_t order = 4;    /**< Order of spline to interpolate. */
	const tsReal as = 1.f/6.f; /**< The value 'a sixth'. */
	const tsReal at = 1.f/3.f; /**< The value 'a third'. */
	const tsReal tt = 2.f/3.f; /**< The value 'two third'. */
	size_t sof_ctrlp;          /**< Size of a single control point. */
	const tsReal* b = points;  /**< Array of the b values. */
	tsReal* s;                 /**< Array of the s values. */
	size_t i, d;               /**< Used in for loops */
	size_t j, k, l;            /**< Used as temporary indices. */
	tsReal *ctrlp; /**< Pointer to the control points of \p _spline_. */
	tsError err;

	/* input validation */
	if (dim == 0)
		TS_RETURN_0(status, TS_DIM_ZERO, "unsupported dimension: 0")
	if (n <= 1) {
		TS_RETURN_1(status, TS_NUM_POINTS,
		            "num(points) (%lu) <= 1",
		            (unsigned long) n)
	}
	/* in the following n >= 2 applies */

	sof_ctrlp = dim * sizeof(tsReal); /* dim > 0 implies sof_ctrlp > 0 */

	s = NULL;
	TS_TRY(try, err, status)
		/* n >= 2 implies n-1 >= 1 implies (n-1)*4 >= 4 */
		TS_CALL(try, err, ts_bspline_new(
		        (n-1) * 4, dim, order - 1,
		        TS_BEZIERS, spline, status))
		ctrlp = ts_int_bspline_access_ctrlp(spline);

		s = (tsReal*) malloc(n * sof_ctrlp);
		if (!s) {
			TS_THROW_0(try, err, status, TS_MALLOC,
			           "out of memory")
		}

		/* set s_0 to b_0 and s_n = b_n */
		memcpy(s, b, sof_ctrlp);
		memcpy(s + (n-1)*dim, b + (n-1)*dim, sof_ctrlp);

		/* set s_i = 1/6*b_i + 2/3*b_{i-1} + 1/6*b_{i+1}*/
		for (i = 1; i < n-1; i++) {
			for (d = 0; d < dim; d++) {
				j = (i-1)*dim+d;
				k = i*dim+d;
				l = (i+1)*dim+d;
				s[k] = as * b[j];
				s[k] += tt * b[k];
				s[k] += as * b[l];
			}
		}

		/* create beziers from b and s */
		for (i = 0; i < n-1; i++) {
			for (d = 0; d < dim; d++) {
				j = i*dim+d;
				k = i*4*dim+d;
				l = (i+1)*dim+d;
				ctrlp[k] = s[j];
				ctrlp[k+dim] = tt*b[j] + at*b[l];
				ctrlp[k+2*dim] = at*b[j] + tt*b[l];
				ctrlp[k+3*dim] = s[l];
			}
		}
	TS_CATCH(err)
		ts_bspline_free(spline);
	TS_FINALLY
		if (s)
			free(s);
	TS_END_TRY_RETURN(err)
}

tsError
ts_bspline_interpolate_cubic_natural(const tsReal *points,
                                     size_t num_points,
                                     size_t dimension,
                                     tsBSpline *spline,
                                     tsStatus *status)
{
	const size_t sof_ctrlp = dimension * sizeof(tsReal);
	const size_t len_points = num_points * dimension;
	const size_t num_int_points = num_points - 2;
	const size_t len_int_points = num_int_points * dimension;
	tsReal *thomas, *a, *b, *c, *d;
	size_t i, j, k, l;
	tsError err;

	ts_int_bspline_init(spline);
	if (num_points == 0)
		TS_RETURN_0(status, TS_NUM_POINTS, "num(points) == 0")
	if (num_points == 1) {
		TS_CALL_ROE(err, ts_int_cubic_point(
		            points, dimension, spline, status))
		TS_RETURN_SUCCESS(status)
	}
	if (num_points == 2) {
		return ts_int_relaxed_uniform_cubic_bspline(
			points, num_points, dimension, spline, status);
	}
	/* `num_points` >= 3 */
	thomas = NULL;
	TS_TRY(try, err, status)
		thomas = (tsReal *) malloc(
			/* `a', `b', `c' (note that `c' is equal to `a') */
			2 * num_int_points * sizeof(tsReal) +
			/* `d' and "result of the thomas algorithm" (which
			   contains `num_points' points) */
			num_points * dimension * sizeof(tsReal));
		if (!thomas) {
			TS_THROW_0(try, err, status, TS_MALLOC,
			           "out of memory")
		}
		/* The system of linear equations is taken from:
		 *     http://www.bakoma-tex.com/doc/generic/pst-bspline/
		 *     pst-bspline-doc.pdf */
		a = c = thomas;
		ts_arr_fill(a, num_int_points, 1);
		b = a + num_int_points;
		ts_arr_fill(b, num_int_points, 4);
		d = b + num_int_points;
		/* 6 * S_{i+1} */
		for (i = 0; i < num_int_points; i++) {
			for (j = 0; j < dimension; j++) {
				k = i * dimension + j;
				l = (i+1) * dimension + j;
				d[k] = 6 * points[l];
			}
		}
		for (i = 0; i < dimension; i++) {
			/* 6 * S_{1} - S_{0} */
			d[i] -= points[i];
			/* 6 * S_{n-1} - S_{n} */
			k = len_int_points - (i+1);
			l = len_points - (i+1);
			d[k] -= points[l];
		}
		/* The Thomas algorithm requires at least two points. Hence,
		 * `num_int_points` == 1 must be handled separately (let's call
		 * it "Mini Thomas"). */
		if (num_int_points == 1) {
			for (i = 0; i < dimension; i++)
				d[i] *= (tsReal) 0.25f;
		} else {
			TS_CALL(try, err, ts_int_thomas_algorithm(
			        a, b, c, num_int_points, dimension, d,
			        status))
		}
		memcpy(thomas, points, sof_ctrlp);
		memmove(thomas + dimension, d, num_int_points * sof_ctrlp);
		memcpy(thomas + (num_int_points+1) * dimension,
		       points + (num_points-1) * dimension, sof_ctrlp);
		TS_CALL(try, err, ts_int_relaxed_uniform_cubic_bspline(
		        thomas, num_points, dimension, spline, status))
	TS_CATCH(err)
		ts_bspline_free(spline);
	TS_FINALLY
		if (thomas)
			free(thomas);
	TS_END_TRY_RETURN(err)
}

tsError
ts_bspline_interpolate_catmull_rom(const tsReal *points,
                                   size_t num_points,
                                   size_t dimension,
                                   tsReal alpha,
                                   const tsReal *first,
                                   const tsReal *last,
                                   tsReal epsilon,
                                   tsBSpline *spline,
                                   tsStatus *status)
{
	const size_t sof_real = sizeof(tsReal);
	const size_t sof_ctrlp = dimension * sof_real;
	const tsReal eps = (tsReal) fabs(epsilon);
	tsReal *bs_ctrlp; /* Points to the control points of `spline`. */
	tsReal *cr_ctrlp; /**< The points to interpolate based on `points`. */
	size_t i, d; /**< Used in for loops. */
	tsError err; /**< Local error handling. */
	/* [https://en.wikipedia.org/wiki/
	 * Centripetal_Catmull%E2%80%93Rom_spline] */
	tsReal t0, t1, t2, t3; /**< Catmull-Rom knots. */
	/* [https://stackoverflow.com/questions/30748316/
	 * catmull-rom-interpolation-on-svg-paths/30826434#30826434] */
	tsReal c1, c2, d1, d2, m1, m2; /**< Used to calculate derivatives. */
	tsReal *p0, *p1, *p2, *p3; /**< Processed Catmull-Rom points. */

	ts_int_bspline_init(spline);
	if (dimension == 0)
		TS_RETURN_0(status, TS_DIM_ZERO, "unsupported dimension: 0")
	if (num_points == 0)
		TS_RETURN_0(status, TS_NUM_POINTS, "num(points) == 0")
	if (alpha < (tsReal) 0.0) alpha = (tsReal) 0.0;
	if (alpha > (tsReal) 1.0) alpha = (tsReal) 1.0;

	/* Copy `points` to `cr_ctrlp`. Add space for `first` and `last`. */
	cr_ctrlp = (tsReal *) malloc((num_points + 2) * sof_ctrlp);
	if (!cr_ctrlp)
		TS_RETURN_0(status, TS_MALLOC, "out of memory")
	memcpy(cr_ctrlp + dimension, points, num_points * sof_ctrlp);

	/* Remove redundant points from `cr_ctrlp`. Update `num_points`. */
	for (i = 1 /* 0 (`first`) is not assigned yet */;
	     i < num_points /* skip last point (inclusive end) */;
	     i++) {
		p0 = cr_ctrlp + (i * dimension);
		p1 = p0 + dimension;
		if (ts_distance(p0, p1, dimension) <= eps) {
			/* Are there any other points (after the one that is
			 * to be removed) that need to be shifted? */
			if (i < num_points - 1) {
				memmove(p1, p1 + dimension,
				        (num_points - (i + 1)) * sof_ctrlp);
			}
			num_points--;
			i--;
		}
	}

	/* Check if there are still enough points for interpolation. */
	if (num_points == 1) { /* `num_points` can't be 0 */
		free(cr_ctrlp); /* The point is copied from `points`. */
		TS_CALL_ROE(err, ts_int_cubic_point(
		            points, dimension, spline, status))
		TS_RETURN_SUCCESS(status)
	}

	/* Add or generate `first` and `last`. Update `num_points`. */
	p0 = cr_ctrlp + dimension;
	if (first && ts_distance(first, p0, dimension) > eps) {
		memcpy(cr_ctrlp, first, sof_ctrlp);
	} else {
		p1 = p0 + dimension;
		for (d = 0; d < dimension; d++)
			cr_ctrlp[d] = p0[d] + (p0[d] - p1[d]);
	}
	p1 = cr_ctrlp + (num_points * dimension);
	if (last && ts_distance(p1, last, dimension) > eps) {
		memcpy(cr_ctrlp + ((num_points + 1) * dimension),
		       last, sof_ctrlp);
	} else {
		p0 = p1 - dimension;
		for (d = 0; d < dimension; d++) {
			cr_ctrlp[((num_points + 1) * dimension) + d] =
				p1[d] + (p1[d] - p0[d]);
		}
	}
	num_points = num_points + 2;

	/* Transform the sequence of Catmull-Rom splines. */
	bs_ctrlp = NULL;
	TS_TRY(try, err, status)
		TS_CALL(try, err, ts_bspline_new(
		        (num_points - 3) * 4, dimension, 3,
		        TS_BEZIERS, spline, status))
		bs_ctrlp = ts_int_bspline_access_ctrlp(spline);
	TS_CATCH(err)
		free(cr_ctrlp);
	TS_END_TRY_ROE(err)
	for (i = 0; i < ts_bspline_num_control_points(spline) / 4; i++) {
		p0 = cr_ctrlp + ((i+0) * dimension);
		p1 = cr_ctrlp + ((i+1) * dimension);
		p2 = cr_ctrlp + ((i+2) * dimension);
		p3 = cr_ctrlp + ((i+3) * dimension);

		t0 = (tsReal) 0.f;
		t1 = t0 + (tsReal) pow(ts_distance(p0, p1, dimension), alpha);
		t2 = t1 + (tsReal) pow(ts_distance(p1, p2, dimension), alpha);
		t3 = t2 + (tsReal) pow(ts_distance(p2, p3, dimension), alpha);

		c1 = (t2-t1) / (t2-t0);
		c2 = (t1-t0) / (t2-t0);
		d1 = (t3-t2) / (t3-t1);
		d2 = (t2-t1) / (t3-t1);

		for (d = 0; d < dimension; d++) {
			m1 = (t2-t1)*(c1*(p1[d]-p0[d])/(t1-t0)
			              + c2*(p2[d]-p1[d])/(t2-t1));
			m2 = (t2-t1)*(d1*(p2[d]-p1[d])/(t2-t1)
			              + d2*(p3[d]-p2[d])/(t3-t2));
			bs_ctrlp[((i*4 + 0) * dimension) + d] = p1[d];
			bs_ctrlp[((i*4 + 1) * dimension) + d] = p1[d] + m1/3;
			bs_ctrlp[((i*4 + 2) * dimension) + d] = p2[d] - m2/3;
			bs_ctrlp[((i*4 + 3) * dimension) + d] = p2[d];
		}
	}
	free(cr_ctrlp);
	TS_RETURN_SUCCESS(status)
}
/*! @} */



/*! @name Query Functions
 *
 * @{
 */
tsError
ts_int_bspline_find_knot(const tsBSpline *spline,
                         tsReal *knot, /* in: knot; out: actual knot */
                         size_t *idx,  /* out: index of `knot' */
                         size_t *mult, /* out: multiplicity of `knot' */
                         tsStatus *status)
{
	const size_t deg = ts_bspline_degree(spline);
	const size_t num_knots = ts_bspline_num_knots(spline);
	const tsReal *knots = ts_int_bspline_access_knots(spline);
	tsReal min, max;
	size_t low, high;

	ts_bspline_domain(spline, &min, &max);
	if (*knot < min) {
		/* Avoid infinite loop (issue #222) */
		if (ts_knots_equal(*knot, min)) *knot = min;
		else {
			TS_RETURN_2(status, TS_U_UNDEFINED,
			            "knot (%f) < min(domain) (%f)",
			            *knot, min)
		}
	}
	else if (*knot > max && !ts_knots_equal(*knot, max)) {
		TS_RETURN_2(status, TS_U_UNDEFINED,
		            "knot (%f) > max(domain) (%f)",
		            *knot, max)
	}

	/* Based on 'The NURBS Book' (Les Piegl and Wayne Tiller). */
	if (ts_knots_equal(*knot, knots[num_knots - 1])) {
		*idx = num_knots - 1;
	} else {
		low = 0;
		high = num_knots - 1;
		*idx = (low+high) / 2;
		while (*knot < knots[*idx] || *knot >= knots[*idx + 1]) {
			if (*knot < knots[*idx])
				high = *idx;
			else
				low = *idx;
			*idx = (low+high) / 2;
		}
	}

	/* Handle floating point errors. */
	while (*idx < num_knots - 1 && /* there is a next knot */
	       ts_knots_equal(*knot, knots[*idx + 1])) {
		(*idx)++;
	}
	if (ts_knots_equal(*knot, knots[*idx]))
		*knot = knots[*idx]; /* set actual knot */

	/* Calculate knot's multiplicity. */
	for (*mult = deg + 1; *mult > 0 ; (*mult)--) {
		if (ts_knots_equal(*knot, knots[*idx - (*mult-1)]))
			break;
	}

	TS_RETURN_SUCCESS(status)
}

tsError
ts_int_bspline_eval_woa(const tsBSpline *spline,
                        tsReal u,
                        tsDeBoorNet *net,
                        tsStatus *status)
{
	const size_t deg = ts_bspline_degree(spline);
	const size_t order = ts_bspline_order(spline);
	const size_t dim = ts_bspline_dimension(spline);
	const size_t num_knots = ts_bspline_num_knots(spline);
	const size_t sof_ctrlp = dim * sizeof(tsReal);

	const tsReal *ctrlp = ts_int_bspline_access_ctrlp(spline);
	const tsReal *knots = ts_int_bspline_access_knots(spline);
	tsReal *points = NULL;  /**< Pointer to the points of \p net. */

	size_t k;        /**< Index of \p u. */
	size_t s;        /**< Multiplicity of \p u. */

	size_t from;     /**< Offset used to copy values. */
	size_t fst;      /**< First affected control point, inclusive. */
	size_t lst;      /**< Last affected control point, inclusive. */
	size_t N;        /**< Number of affected control points. */

	/* The following indices are used to create the DeBoor net. */
	size_t lidx;     /**< Current left index. */
	size_t ridx;     /**< Current right index. */
	size_t tidx;     /**< Current to index. */
	size_t r, i, d;  /**< Used in for loop. */
	tsReal ui;       /**< Knot value at index i. */
	tsReal a, a_hat; /**< Weighting factors of control points. */

	tsError err;

	points = ts_int_deboornet_access_points(net);

	/* 1. Find index k such that u is in between [u_k, u_k+1).
	 * 2. Setup already known values.
	 * 3. Decide by multiplicity of u how to calculate point P(u). */

	/* 1. */
	k = s = 0;
	TS_CALL_ROE(err, ts_int_bspline_find_knot(
	            spline, &u, &k, &s, status))

	/* 2. */
	net->pImpl->u = u;
	net->pImpl->k = k;
	net->pImpl->s = s;
	net->pImpl->h = deg < s ? 0 : deg-s; /* prevent underflow */

	/* 3. (by 1. s <= order)
	 *
	 * 3a) Check for s = order.
	 *     Take the two points k-s and k-s + 1. If one of
	 *     them doesn't exist, take only the other.
	 * 3b) Use de boor algorithm to find point P(u). */
	if (s == order) {
		/* only one of the two control points exists */
		if (k == deg || /* only the first */
		    k == num_knots - 1) { /* only the last */
			from = k == deg ? 0 : (k-s) * dim;
			net->pImpl->n_points = 1;
			memcpy(points, ctrlp + from, sof_ctrlp);
		} else {
			from = (k-s) * dim;
			net->pImpl->n_points = 2;
			memcpy(points, ctrlp + from, 2 * sof_ctrlp);
		}
	} else { /* by 3a) s <= deg (order = deg+1) */
		fst = k-deg; /* by 1. k >= deg */
		lst = k-s; /* s <= deg <= k */
		N = lst-fst + 1; /* lst <= fst implies N >= 1 */

		net->pImpl->n_points = (size_t)(N * (N+1) * 0.5f);

		/* copy initial values to output */
		memcpy(points, ctrlp + fst*dim, N * sof_ctrlp);

		lidx = 0;
		ridx = dim;
		tidx = N*dim; /* N >= 1 implies tidx > 0 */
		r = 1;
		for (;r <= ts_deboornet_num_insertions(net); r++) {
			i = fst + r;
			for (; i <= lst; i++) {
				ui = knots[i];
				a = (ts_deboornet_knot(net) - ui) /
					(knots[i+deg-r+1] - ui);
				a_hat = 1.f-a;

				for (d = 0; d < dim; d++) {
					points[tidx++] =
						a_hat * points[lidx++] +
						a     * points[ridx++];
				}
			}
			lidx += dim;
			ridx += dim;
		}
	}
	TS_RETURN_SUCCESS(status)
}

tsError
ts_bspline_eval(const tsBSpline *spline,
                tsReal knot,
                tsDeBoorNet *net,
                tsStatus *status)
{
	tsError err;
	ts_int_deboornet_init(net);
	TS_TRY(try, err, status)
		TS_CALL(try, err, ts_int_deboornet_new(
		        spline, net, status))
		TS_CALL(try, err, ts_int_bspline_eval_woa(
		        spline, knot, net, status))
	TS_CATCH(err)
		ts_deboornet_free(net);
	TS_END_TRY_RETURN(err)
}

tsError
ts_bspline_eval_all(const tsBSpline *spline,
                    const tsReal *knots,
                    size_t num,
                    tsReal **points,
                    tsStatus *status)
{
	const size_t dim = ts_bspline_dimension(spline);
	const size_t sof_point = dim * sizeof(tsReal);
	const size_t sof_points = num * sof_point;
	tsDeBoorNet net = ts_deboornet_init();
	tsReal *result;
	size_t i;
	tsError err;
	TS_TRY(try, err, status)
		*points = (tsReal *) malloc(sof_points);
		if (!*points) {
			TS_THROW_0(try, err, status, TS_MALLOC,
			           "out of memory")
		}
		TS_CALL(try, err, ts_int_deboornet_new(
		        spline,&net, status))
		for (i = 0; i < num; i++) {
			TS_CALL(try, err, ts_int_bspline_eval_woa(
			        spline, knots[i], &net, status))
			result = ts_int_deboornet_access_result(&net);
			memcpy((*points) + i * dim, result, sof_point);
		}
	TS_CATCH(err)
		if (*points)
			free(*points);
		*points = NULL;
	TS_FINALLY
		ts_deboornet_free(&net);
	TS_END_TRY_RETURN(err)
}

tsError
ts_bspline_sample(const tsBSpline *spline,
                  size_t num,
                  tsReal **points,
                  size_t *actual_num,
                  tsStatus *status)
{
	tsError err;
	tsReal *knots;

	num = num == 0 ? 100 : num;
	*actual_num = num;
	knots = (tsReal *) malloc(num * sizeof(tsReal));
	if (!knots) {
		*points = NULL;
		TS_RETURN_0(status, TS_MALLOC, "out of memory")
	}
	ts_bspline_uniform_knot_seq(spline, num, knots);
	TS_TRY(try, err, status)
		TS_CALL(try, err, ts_bspline_eval_all(
		        spline, knots, num, points, status))
	TS_FINALLY
		free(knots);
	TS_END_TRY_RETURN(err)
}

tsError
ts_bspline_bisect(const tsBSpline *spline,
                  tsReal value,
                  tsReal epsilon,
                  int persnickety,
                  size_t index,
                  int ascending,
                  size_t max_iter,
                  tsDeBoorNet *net,
                  tsStatus *status)
{
	tsError err;
	const size_t dim = ts_bspline_dimension(spline);
	const tsReal eps = (tsReal) fabs(epsilon);
	size_t i = 0;
	tsReal dist = 0;
	tsReal min, max, mid;
	tsReal *P;

	ts_int_deboornet_init(net);

	if (dim < index) {
		TS_RETURN_2(status, TS_INDEX_ERROR,
		            "dimension (%lu) <= index (%lu)",
		            (unsigned long) dim,
		            (unsigned long) index)
	}
	if(max_iter == 0)
		TS_RETURN_0(status, TS_NO_RESULT, "0 iterations")

	ts_bspline_domain(spline, &min, &max);
	TS_TRY(try, err, status)
		TS_CALL(try, err, ts_int_deboornet_new(
		        spline, net, status))
		do {
			mid = (tsReal) ((min + max) / 2.0);
			TS_CALL(try, err, ts_int_bspline_eval_woa(
			        spline, mid, net, status))
			P = ts_int_deboornet_access_result(net);
			dist = ts_distance(&P[index], &value, 1);
			if (dist <= eps)
				TS_RETURN_SUCCESS(status)
			if (ascending) {
				if (P[index] < value)
					min = mid;
				else
					max = mid;
			} else {
				if (P[index] < value)
					max = mid;
				else
					min = mid;
			}
		} while (i++ < max_iter);
		if (persnickety) {
			TS_THROW_1(try, err, status, TS_NO_RESULT,
			           "maximum iterations (%lu) exceeded",
			           (unsigned long) max_iter)
		}
	TS_CATCH(err)
		ts_deboornet_free(net);
	TS_END_TRY_RETURN(err)
}

void ts_bspline_domain(const tsBSpline *spline,
                       tsReal *min,
                       tsReal *max)
{
	*min = ts_int_bspline_access_knots(spline)
		[ts_bspline_degree(spline)];
	*max = ts_int_bspline_access_knots(spline)
		[ts_bspline_num_knots(spline) - ts_bspline_order(spline)];
}

tsError
ts_bspline_is_closed(const tsBSpline *spline,
                     tsReal epsilon,
                     int *closed,
                     tsStatus *status)
{
	const size_t deg = ts_bspline_degree(spline);
	const size_t dim = ts_bspline_dimension(spline);
	tsBSpline derivative;
	tsReal min, max;
	tsDeBoorNet first, last;
	size_t i;
	tsError err;

	ts_int_bspline_init(&derivative);
	ts_int_deboornet_init(&first);
	ts_int_deboornet_init(&last);

	TS_TRY(try, err, status)
		for (i = 0; i < deg; i++) {
			TS_CALL(try, err, ts_bspline_derive(
			        spline, i, -1.f, &derivative, status))
			ts_bspline_domain(&derivative, &min, &max);
			TS_CALL(try, err, ts_bspline_eval(
			        &derivative, min, &first, status))
			TS_CALL(try, err, ts_bspline_eval(
			        &derivative, max, &last, status))
			*closed = ts_distance(
				ts_int_deboornet_access_result(&first),
				ts_int_deboornet_access_result(&last),
				dim) <= epsilon ? 1 : 0;
			ts_bspline_free(&derivative);
			ts_deboornet_free(&first);
			ts_deboornet_free(&last);
			if (!*closed)
				TS_RETURN_SUCCESS(status)
		}
	TS_FINALLY
		ts_bspline_free(&derivative);
		ts_deboornet_free(&first);
		ts_deboornet_free(&last);
	TS_END_TRY_RETURN(err)
}

tsError
ts_bspline_compute_rmf(const tsBSpline *spline,
                       const tsReal *knots,
                       size_t num,
                       int has_first_normal,
                       tsFrame *frames,
                       tsStatus *status)
{
	tsError err;
	size_t i;
	tsReal fx, fy, fz, fmin;
	tsReal xc[3], xn[3], v1[3], c1, v2[3], c2, rL[3], tL[3];
	tsBSpline deriv = ts_bspline_init();
	tsDeBoorNet curr = ts_deboornet_init();
	tsDeBoorNet next = ts_deboornet_init();

	if (num < 1)
		TS_RETURN_SUCCESS(status);

	TS_TRY(try, err, status)
		TS_CALL(try, err, ts_int_deboornet_new(
		        spline, &curr, status))
		TS_CALL(try, err, ts_int_deboornet_new(
		        spline, &next, status))
		TS_CALL(try, err, ts_bspline_derive(
		        spline, 1, (tsReal) -1.0, &deriv, status))

		/* Set position. */
		TS_CALL(try, err, ts_int_bspline_eval_woa(
		        spline, knots[0], &curr, status))
		ts_vec3_set(frames[0].position,
		            ts_int_deboornet_access_result(&curr),
		            ts_bspline_dimension(spline));
		/* Set tangent. */
		TS_CALL(try, err, ts_int_bspline_eval_woa(
		        &deriv, knots[0], &curr, status))
		ts_vec3_set(frames[0].tangent,
		            ts_int_deboornet_access_result(&curr),
		            ts_bspline_dimension(&deriv));
		ts_vec_norm(frames[0].tangent, 3, frames[0].tangent);
		/* Set normal. */
		if (!has_first_normal) {
			fx = (tsReal) fabs(frames[0].tangent[0]);
			fy = (tsReal) fabs(frames[0].tangent[1]);
			fz = (tsReal) fabs(frames[0].tangent[2]);
			fmin = fx; /* x is min => 1, 0, 0 */
			ts_vec3_init(frames[0].normal,
			             (tsReal) 1.0,
			             (tsReal) 0.0,
			             (tsReal) 0.0);
			if (fy < fmin) { /* y is min => 0, 1, 0 */
				fmin = fy;
				ts_vec3_init(frames[0].normal,
				             (tsReal) 0.0,
				             (tsReal) 1.0,
				             (tsReal) 0.0);
			}
			if (fz < fmin) { /* z is min => 0, 0, 1 */
				ts_vec3_init(frames[0].normal,
				             (tsReal) 0.0,
				             (tsReal) 0.0,
				             (tsReal) 1.0);
			}
			ts_vec3_cross(frames[0].tangent,
			              frames[0].normal,
			              frames[0].normal);
			ts_vec_norm(frames[0].normal, 3, frames[0].normal);
			if (ts_bspline_dimension(spline) >= 3) {
				/* In 3D (and higher) an additional rotation of
				   the normal along the tangent is needed in
				   order to let the normal extend sideways (as
				   it does in 2D and lower). */
				ts_vec3_cross(frames[0].tangent,
				              frames[0].normal,
				              frames[0].normal);
			}
		} else {
			/* Never trust user input! */
			ts_vec_norm(frames[0].normal, 3, frames[0].normal);
		}
		/* Set binormal. */
		ts_vec3_cross(frames[0].tangent,
		              frames[0].normal,
		              frames[0].binormal);

		for (i = 0; i < num - 1; i++) {
			/* Eval current and next point. */
			TS_CALL(try, err, ts_int_bspline_eval_woa(
			        spline, knots[i], &curr, status))
			TS_CALL(try, err, ts_int_bspline_eval_woa(
			        spline, knots[i+1], &next, status))
			ts_vec3_set(xc, /* xc is now the current point */
			            ts_int_deboornet_access_result(&curr),
			            ts_bspline_dimension(spline));
			ts_vec3_set(xn, /* xn is now the next point */
			            ts_int_deboornet_access_result(&next),
			            ts_bspline_dimension(spline));

			/* Set position of U_{i+1}. */
			ts_vec3_set(frames[i+1].position, xn, 3);

			/* Compute reflection vector of R_{1}. */
			ts_vec_sub(xn, xc, 3, v1);
			c1 = ts_vec_dot(v1, v1, 3);

			/* Compute r_{i}^{L} = R_{1} * r_{i}. */
			rL[0] = (tsReal) 2.0 / c1;
			rL[1] = ts_vec_dot(v1, frames[i].normal, 3);
			rL[2] = rL[0] * rL[1];
			ts_vec_mul(v1, 3, rL[2], rL);
			ts_vec_sub(frames[i].normal, rL, 3, rL);

			/* Compute t_{i}^{L} = R_{1} * t_{i}. */
			tL[0] = (tsReal) 2.0 / c1;
			tL[1] = ts_vec_dot(v1, frames[i].tangent, 3);
			tL[2] = tL[0] * tL[1];
			ts_vec_mul(v1, 3, tL[2], tL);
			ts_vec_sub(frames[i].tangent, tL, 3, tL);

			/* Compute reflection vector of R_{2}. */
			TS_CALL(try, err, ts_int_bspline_eval_woa(
			        &deriv, knots[i+1], &next, status))
			ts_vec3_set(xn, /* xn is now the next tangent */
			            ts_int_deboornet_access_result(&next),
			            ts_bspline_dimension(&deriv));
			ts_vec_norm(xn, 3, xn);
			ts_vec_sub(xn, tL, 3, v2);
			c2 = ts_vec_dot(v2, v2, 3);

			/* Compute r_{i+1} = R_{2} * r_{i}^{L}. */
			ts_vec3_set(xc, /* xc is now the next normal */
			            frames[i+1].normal, 3);
			xc[0] = (tsReal) 2.0 / c2;
			xc[1] = ts_vec_dot(v2, rL, 3);
			xc[2] = xc[0] * xc[1];
			ts_vec_mul(v2, 3, xc[2], xc);
			ts_vec_sub(rL, xc, 3, xc);
			ts_vec_norm(xc, 3, xc);

			/* Compute vector s_{i+1} of U_{i+1}. */
			ts_vec3_cross(xn, xc, frames[i+1].binormal);

			/* Set vectors t_{i+1} and r_{i+1} of U_{i+1}. */
			ts_vec3_set(frames[i+1].tangent, xn, 3);
			ts_vec3_set(frames[i+1].normal, xc, 3);
		}
	TS_FINALLY
		ts_bspline_free(&deriv);
		ts_deboornet_free(&curr);
		ts_deboornet_free(&next);
	TS_END_TRY_RETURN(err)
}


tsError
ts_bspline_chord_lengths(const tsBSpline *spline,
                         const tsReal *knots,
                         size_t num,
                         tsReal *lengths,
                         tsStatus *status)
{
	tsError err;
	tsReal dist, lst_knot, cur_knot;
	size_t i, dim = ts_bspline_dimension(spline);
	tsDeBoorNet lst = ts_deboornet_init();
	tsDeBoorNet cur = ts_deboornet_init();
	tsDeBoorNet tmp = ts_deboornet_init();

	if (num == 0) TS_RETURN_SUCCESS(status);

	TS_TRY(try, err, status)
		TS_CALL(try, err, ts_int_deboornet_new(
		        spline, &lst, status))
		TS_CALL(try, err, ts_int_deboornet_new(
		        spline, &cur, status))

		/* num >= 1 */
		TS_CALL(try, err, ts_int_bspline_eval_woa(
		        spline, knots[0], &lst, status));
		lengths[0] = (tsReal) 0.0;

		for (i = 1; i < num; i++) {
			TS_CALL(try, err, ts_int_bspline_eval_woa(
			        spline, knots[i], &cur, status));
			lst_knot = ts_deboornet_knot(&lst);
			cur_knot = ts_deboornet_knot(&cur);
			if (cur_knot < lst_knot) {
				TS_THROW_1(try, err, status, TS_KNOTS_DECR,
				            "decreasing knot at index: %lu",
				            (unsigned long) i)
			}
			dist = ts_distance(ts_deboornet_result_ptr(&lst),
			                   ts_deboornet_result_ptr(&cur),
			                   dim);
			lengths[i] = lengths[i-1] + dist;
			ts_deboornet_move(&lst, &tmp);
			ts_deboornet_move(&cur, &lst);
			ts_deboornet_move(&tmp, &cur);
		}
	TS_FINALLY
		ts_deboornet_free(&lst);
		ts_deboornet_free(&cur);
	TS_END_TRY_RETURN(err)
}


tsError
ts_bspline_sub_spline(const tsBSpline *spline,
                      tsReal knot0,
                      tsReal knot1,
                      tsBSpline *sub,
                      tsStatus *status)
{
	int reverse; /* reverse `spline`? (if `knot0 > knot1`) */
	tsReal *tmp = NULL; /* a buffer to swap control points */
	tsReal min, max; /* domain of `spline` */
	size_t dim, deg, order; /* properties of `spline` (and `sub`) */
	tsBSpline worker; /* stores the result of the `split' operations */
	tsReal *ctrlp, *knots; /* control points and knots of `worker` */
	size_t k0, k1; /* indices returned by the `split' operations */
	size_t c0, c1; /* indices of the control points to be moved */
	size_t nc, nk; /* number of control points and knots of `sub` */
	size_t i; /* for various needs */
	tsError err; /* for local try-catch block */

	/* Make sure that `worker` points to `NULL'. This allows us to call
	 * `ts_bspline_free` in `TS_CATCH` without further checks. Also, `NULL'
	 * serves as an indicator of whether `ts_bspline_split` has been called
	 * on `spline` at least once (if not, `worker` needs to be initialized
	 * manually). */
	ts_int_bspline_init(&worker);
	INIT_OUT_BSPLINE(spline, sub)

	ts_bspline_domain(spline, &min, &max);
	dim = ts_bspline_dimension(spline);
	deg = ts_bspline_degree(spline);
	order = ts_bspline_order(spline);

	/* Cannot create valid knot vector from empty domain. */
	if (ts_knots_equal(knot0, knot1)) {
		TS_RETURN_0(status,
		            TS_NO_RESULT,
		            "empty domain")
	}

	/* Check for `reverse mode'. Reverse mode means that the copied sequence
	 * of (sub) control points need to be reversed, forming a `backwards'
	 * spline. */
	reverse = knot0 > knot1;
	if (reverse) { /* swap `knot0` and `knot1` */
		tmp = (tsReal *) malloc(dim * sizeof(tsReal));
		if (!tmp) TS_RETURN_0(status, TS_MALLOC, "out of memory");
		*tmp = knot0; /* `tmp` can  hold at least one value */
		knot0 = knot1;
		knot1 = *tmp;
	}

	TS_TRY(try, err, status)
		if (!ts_knots_equal(knot0 , min)) {
			TS_CALL(try , err, ts_bspline_split(
			        spline, knot0, &worker, &k0, status))
		} else { k0 = deg; }
		if (!ts_knots_equal(knot1, max)) {
			TS_CALL(try , err, ts_bspline_split(
			        /* If `NULL', the split operation
			           above was not called. */
			        !worker.pImpl ? spline : &worker,
			        knot1, &worker, &k1, status))
		} else {
			k1 = ts_bspline_num_knots(
			        /* If `NULL', the split operation
			           above was not called. */
			        !worker.pImpl ? spline : &worker) - 1;
		}

		/* Set up `worker`. */
		if (!worker.pImpl) { /* => no split applied */
			TS_CALL(try, err, ts_bspline_copy(
			        spline, &worker, status))
			/* Needed in `reverse mode'. */
			ctrlp = ts_int_bspline_access_ctrlp(&worker);
			knots = ts_int_bspline_access_knots(&worker);
			nc = ts_bspline_num_control_points(&worker);
		} else {
			c0 = (k0-deg) * dim;
			c1 = (k1-order) * dim;
			nc = ((c1-c0) / dim) + 1;
			nk = (k1-k0) + order;

			/* Also needed in `reverse mode'. */
			ctrlp = ts_int_bspline_access_ctrlp(&worker);
			knots = ts_int_bspline_access_knots(&worker);

			/* Move control points. */
			memmove(ctrlp,
			        ctrlp + c0,
			        nc * dim * sizeof(tsReal));
			/* Move knots. */
			memmove(ctrlp + nc * dim,
			        knots + (k0-deg),
			        nk * sizeof(tsReal));

			/* Remove superfluous control points and knots from
			 * the memory of `worker`. */
			worker.pImpl->n_knots = nk;
			worker.pImpl->n_ctrlp = nc;
			i = ts_int_bspline_sof_state(&worker);
			worker.pImpl = realloc(worker.pImpl, i);
			if (worker.pImpl == NULL) { /* unlikely to fail */
				TS_THROW_0(try, err, status, TS_MALLOC,
				           "out of memory")
			}
		}

		/* Reverse control points (if necessary). */
		if (reverse) {
			for (i = 0; i < nc / 2; i++) {
				memcpy(tmp,
				       ctrlp  +     i      * dim,
				       dim * sizeof(tsReal));
				memmove(ctrlp +     i      * dim,
				        ctrlp + (nc-1 - i) * dim,
				        dim * sizeof(tsReal));
				memcpy(ctrlp  + (nc-1 - i) * dim,
				       tmp,
				       dim * sizeof(tsReal));
			}
		}

		/* Move `worker' to output parameter. */
		if (spline == sub) ts_bspline_free(sub);
		ts_bspline_move(&worker, sub);
	TS_CATCH(err)
		ts_bspline_free(&worker);
	TS_FINALLY
		if (tmp) free(tmp);
	TS_END_TRY_RETURN(err)
}

void
ts_bspline_uniform_knot_seq(const tsBSpline *spline,
                            size_t num,
                            tsReal *knots)
{
	size_t i;
	tsReal min, max;
	if (num == 0) return;
	ts_bspline_domain(spline, &min, &max);
	for (i = 0; i < num; i++) {
		knots[i] = max - min;
		knots[i] *= (tsReal) i / (num - 1);
		knots[i] += min;
	}
	/* Set `knots[0]` after `knots[num - 1]` to ensure
	   that `knots[0] = min` if `num` is `1'. */
	knots[num - 1] = max;
	knots[0] = min;
}

tsError
ts_bspline_equidistant_knot_seq(const tsBSpline *spline,
                                size_t num,
                                tsReal *knots,
                                size_t num_samples,
                                tsStatus *status)
{
	tsError err;
	tsReal *samples = NULL, *lengths = NULL;

	if (num == 0) TS_RETURN_SUCCESS(status);
	if (num_samples == 0) num_samples = 200;

	samples = (tsReal *) malloc(2 * num_samples * sizeof(tsReal));
	if (!samples) TS_RETURN_0(status, TS_MALLOC, "out of memory");
	ts_bspline_uniform_knot_seq(spline, num_samples, samples);
	lengths = samples + num_samples;
	TS_TRY(try, err, status)
		TS_CALL(try, err, ts_bspline_chord_lengths(
		        spline, samples, num_samples, lengths, status))
		TS_CALL(try, err, ts_chord_lengths_equidistant_knot_seq(
		        samples, lengths, num_samples, num, knots, status))
	TS_FINALLY
		free(samples); /* cannot be NULL */
		/* free(lengths); NO! */
	TS_END_TRY_RETURN(err)
}
/*! @} */



/*! @name Transformation Functions
 *
 * @{
 */
tsError
ts_int_bspline_resize(const tsBSpline *spline,
                      int n,
                      int back,
                      tsBSpline *resized,
                      tsStatus *status)
{
	const size_t deg = ts_bspline_degree(spline);
	const size_t dim = ts_bspline_dimension(spline);
	const size_t sof_real = sizeof(tsReal);

	const size_t num_ctrlp = ts_bspline_num_control_points(spline);
	const size_t num_knots = ts_bspline_num_knots(spline);
	const size_t nnum_ctrlp = num_ctrlp + n; /**< New length of ctrlp. */
	const size_t nnum_knots = num_knots + n; /**< New length of knots. */
	const size_t min_num_ctrlp_vec = n < 0 ? nnum_ctrlp : num_ctrlp;
	const size_t min_num_knots_vec = n < 0 ? nnum_knots : num_knots;

	const size_t sof_min_num_ctrlp = min_num_ctrlp_vec * dim * sof_real;
	const size_t sof_min_num_knots = min_num_knots_vec * sof_real;

	tsBSpline tmp; /**< Temporarily stores the result. */
	const tsReal* from_ctrlp = ts_int_bspline_access_ctrlp(spline);
	const tsReal* from_knots = ts_int_bspline_access_knots(spline);
	tsReal* to_ctrlp = NULL; /**< Pointer to the control points of tmp. */
	tsReal* to_knots = NULL; /**< Pointer to the knots of tmp. */

	tsError err;

	if (n == 0) return ts_bspline_copy(spline, resized, status);

	INIT_OUT_BSPLINE(spline, resized)
	TS_CALL_ROE(err, ts_bspline_new(
	            nnum_ctrlp, dim, deg, TS_OPENED,
	            &tmp, status))
	to_ctrlp = ts_int_bspline_access_ctrlp(&tmp);
	to_knots = ts_int_bspline_access_knots(&tmp);

	/* Copy control points and knots. */
	if (!back && n < 0) {
		memcpy(to_ctrlp, from_ctrlp - n*dim, sof_min_num_ctrlp);
		memcpy(to_knots, from_knots - n    , sof_min_num_knots);
	} else if (!back && n > 0) {
		memcpy(to_ctrlp + n*dim, from_ctrlp, sof_min_num_ctrlp);
		memcpy(to_knots + n    , from_knots, sof_min_num_knots);
	} else {
		/* n != 0 implies back == true */
		memcpy(to_ctrlp, from_ctrlp, sof_min_num_ctrlp);
		memcpy(to_knots, from_knots, sof_min_num_knots);
	}

	if (spline == resized)
		ts_bspline_free(resized);
	ts_bspline_move(&tmp, resized);
	TS_RETURN_SUCCESS(status)
}

tsError
ts_bspline_derive(const tsBSpline *spline,
                  size_t n,
                  tsReal epsilon,
                  tsBSpline *deriv,
                  tsStatus *status)
{
	const size_t sof_real = sizeof(tsReal);
	const size_t dim = ts_bspline_dimension(spline);
	const size_t sof_ctrlp = dim * sof_real;
	size_t deg = ts_bspline_degree(spline);
	size_t num_ctrlp = ts_bspline_num_control_points(spline);
	size_t num_knots = ts_bspline_num_knots(spline);

	tsBSpline worker; /**< Stores the intermediate result. */
	tsReal* ctrlp;    /**< Pointer to the control points of worker. */
	tsReal* knots;    /**< Pointer to the knots of worker. */

	size_t m, i, j, k, l; /**< Used in for loops. */
	tsReal *fst, *snd; /**< Pointer to first and second control point. */
	tsReal dist; /**< Distance between fst and snd. */
	tsReal kid1, ki1; /**< Knots at i+deg+1 and i+1. */
	tsReal span; /**< Distance between kid1 and ki1. */

	tsBSpline swap; /**< Used to swap worker and derivative. */
	tsError err;

	INIT_OUT_BSPLINE(spline, deriv)
	TS_CALL_ROE(err, ts_bspline_copy(spline, &worker, status))
	ctrlp = ts_int_bspline_access_ctrlp(&worker);
	knots = ts_int_bspline_access_knots(&worker);

	TS_TRY(try, err, status)
		for (m = 1; m <= n; m++) { /* from 1st to n'th derivative */
			if (deg == 0) {
				ts_arr_fill(ctrlp, dim, 0.f);
				ts_bspline_domain(spline,
				                  &knots[0],
				                  &knots[1]);
				num_ctrlp = 1;
				num_knots = 2;
				break;
			}
			/* Check and, if possible, fix discontinuity. */
			for (i = 2*deg + 1; i < num_knots - (deg+1); i++) {
				if (!ts_knots_equal(knots[i], knots[i-deg]))
					continue;
				fst = ctrlp + (i - (deg+1)) * dim;
				snd = fst + dim;
				dist = ts_distance(fst, snd, dim);
				if (epsilon >= 0.f && dist > epsilon) {
					TS_THROW_1(try, err, status,
					           TS_UNDERIVABLE,
					           "discontinuity at knot: %f",
					           knots[i])
				}
				memmove(snd,
				        snd + dim,
				        (num_ctrlp - (i+1-deg)) * sof_ctrlp);
				memmove(&knots[i],
				        &knots[i+1],
				        (num_knots - (i+1)) * sof_real);
				num_ctrlp--;
				num_knots--;
				i += deg-1;
			}
			/* Derive continuous worker. */
			for (i = 0; i < num_ctrlp-1; i++) {
				for (j = 0; j < dim; j++) {
					k = i    *dim + j;
					l = (i+1)*dim + j;
					ctrlp[k] = ctrlp[l] - ctrlp[k];
					kid1 = knots[i+deg+1];
					ki1  = knots[i+1];
					span = kid1 - ki1;
					if (span < TS_KNOT_EPSILON)
						span = TS_KNOT_EPSILON;
					ctrlp[k] *= deg;
					ctrlp[k] /= span;
				}
			}
			deg       -= 1;
			num_ctrlp -= 1;
			num_knots -= 2;
			knots     += 1;
		}
		TS_CALL(try, err, ts_bspline_new(
		        num_ctrlp, dim, deg, TS_OPENED,
		        &swap, status))
		memcpy(ts_int_bspline_access_ctrlp(&swap),
		       ctrlp,
		       num_ctrlp * sof_ctrlp);
		memcpy(ts_int_bspline_access_knots(&swap),
		       knots,
		       num_knots * sof_real);
		if (spline == deriv)
			ts_bspline_free(deriv);
		ts_bspline_move(&swap, deriv);
	TS_FINALLY
		ts_bspline_free(&worker);
	TS_END_TRY_RETURN(err)
}

tsError
ts_int_bspline_insert_knot(const tsBSpline *spline,
                           const tsDeBoorNet *net,
                           size_t n,
                           tsBSpline *result,
                           tsStatus *status)
{
	const size_t deg = ts_bspline_degree(spline);
	const size_t order = ts_bspline_order(spline);
	const size_t dim = ts_bspline_dimension(spline);
	const tsReal knot = ts_deboornet_knot(net);
	const size_t k = ts_deboornet_index(net);
	const size_t mult = ts_deboornet_multiplicity(net);
	const size_t sof_real = sizeof(tsReal);
	const size_t sof_ctrlp = dim * sof_real;

	size_t N;     /**< Number of affected control points. */
	tsReal* from; /**< Pointer to copy the values from. */
	tsReal* to;   /**< Pointer to copy the values to. */
	int stride;   /**< Stride of the next pointer to copy. */
	size_t i;     /**< Used in for loops. */

	tsReal *ctrlp_spline, *ctrlp_result;
	tsReal *knots_spline, *knots_result;
	size_t num_ctrlp_result;
	size_t num_knots_result;

	tsError err;

	INIT_OUT_BSPLINE(spline, result)
	if (n == 0)
		return ts_bspline_copy(spline, result, status);
	if (mult + n > order) {
		TS_RETURN_4(status, TS_MULTIPLICITY,
		            "multiplicity(%f) (%lu) + %lu > order (%lu)",
		            knot, (unsigned long) mult, (unsigned long) n,
		            (unsigned long) order)
	}

	TS_CALL_ROE(err, ts_int_bspline_resize(
	            spline, (int)n, 1, result, status))
	ctrlp_spline = ts_int_bspline_access_ctrlp(spline);
	knots_spline = ts_int_bspline_access_knots(spline);
	ctrlp_result = ts_int_bspline_access_ctrlp(result);
	knots_result = ts_int_bspline_access_knots(result);
	num_ctrlp_result = ts_bspline_num_control_points(result);
	num_knots_result = ts_bspline_num_knots(result);

	/* mult + n <= deg + 1 (order) with n >= 1
	 * =>  mult <= deg
	 * =>  regular evaluation
	 * =>  N = h + 1 is valid */
	N = ts_deboornet_num_insertions(net) + 1;

	/* 1. Copy the relevant control points and knots from `spline'.
	 * 2. Copy the relevant control points and knots from `net'. */

	/* 1.
	 *
	 * a) Copy left hand side control points from `spline'.
	 * b) Copy right hand side control points from `spline'.
	 * c) Copy left hand side knots from `spline'.
	 * d) Copy right hand side knots form `spline'. */
	/*** Copy Control Points ***/
	memmove(ctrlp_result, ctrlp_spline, (k-deg) * sof_ctrlp);      /* a) */
	from = (tsReal *) ctrlp_spline + dim*(k-deg+N);
	/* n >= 0 implies to >= from */
	to = ctrlp_result + dim*(k-deg+N+n);
	memmove(to, from, (num_ctrlp_result-n-(k-deg+N)) * sof_ctrlp); /* b) */
	/*** Copy Knots ***/
	memmove(knots_result, knots_spline, (k+1) * sof_real);         /* c) */
	from = (tsReal *) knots_spline + k+1;
	/* n >= 0 implies to >= from */
	to = knots_result + k+1+n;
	memmove(to, from, (num_knots_result-n-(k+1)) * sof_real);      /* d) */

	/* 2.
	 *
	 * a) Copy left hand side control points from `net'.
	 * b) Copy middle part control points from `net'.
	 * c) Copy right hand side control points from `net'.
	 * d) Copy knot from `net' (`knot'). */
	from = ts_int_deboornet_access_points(net);
	to = ctrlp_result + (k-deg)*dim;
	stride = (int)(N*dim);

	/*** Copy Control Points ***/
	for (i = 0; i < n; i++) {                                      /* a) */
		memcpy(to, from, sof_ctrlp);
		from += stride;
		to += dim;
		stride -= (int)dim;
	}
	memcpy(to, from, (N-n) * sof_ctrlp);                           /* b) */

	from -= dim;
	to += (N-n)*dim;
	/* N = h+1 with h = deg-mult (ts_int_bspline_eval)
	 * =>  N = deg-mult+1 = order-mult.
	 *
	 * n <= order-mult
	 * =>  N-n+1 >= order-mult - order-mult + 1 = 1
	 * =>  -(int)(N-n+1) <= -1. */
	stride = -(int)(N-n+1) * (int)dim;

	for (i = 0; i < n; i++) {                                      /* c) */
		memcpy(to, from, sof_ctrlp);
		from += stride;
		stride -= (int)dim;
		to += dim;
	}
	/*** Copy Knot ***/
	to = knots_result + k+1;
	for (i = 0; i < n; i++) {                                      /* d) */
		*to = knot;
		to++;
	}
	TS_RETURN_SUCCESS(status)
}

tsError
ts_bspline_insert_knot(const tsBSpline *spline,
                       tsReal knot,
                       size_t num,
                       tsBSpline *result,
                       size_t* k,
                       tsStatus *status)
{
	tsDeBoorNet net;
	tsError err;
	INIT_OUT_BSPLINE(spline, result)
	ts_int_deboornet_init(&net);
	TS_TRY(try, err, status)
		TS_CALL(try, err, ts_bspline_eval(
		        spline, knot, &net, status))
		TS_CALL(try, err, ts_int_bspline_insert_knot(
		        spline, &net, num, result, status))
		ts_deboornet_free(&net);
		TS_CALL(try, err, ts_bspline_eval(
		        result, knot, &net, status))
		*k = ts_deboornet_index(&net);
	TS_CATCH(err)
		*k = 0;
	TS_FINALLY
		ts_deboornet_free(&net);
	TS_END_TRY_RETURN(err)
}

tsError
ts_bspline_split(const tsBSpline *spline,
                 tsReal knot,
                 tsBSpline *split,
                 size_t* k,
                 tsStatus *status)
{
	tsDeBoorNet net;
	tsError err;
	INIT_OUT_BSPLINE(spline, split)
	ts_int_deboornet_init(&net);
	TS_TRY(try, err, status)
		TS_CALL(try, err, ts_bspline_eval(
		        spline, knot, &net, status))
		if (ts_deboornet_multiplicity(&net)
		    == ts_bspline_order(spline)) {
			TS_CALL(try, err, ts_bspline_copy(
			        spline, split, status))
			*k = ts_deboornet_index(&net);
		} else {
			TS_CALL(try, err, ts_int_bspline_insert_knot(
			        spline, &net,
			        ts_deboornet_num_insertions(&net) + 1,
			        split, status))
			*k = ts_deboornet_index(&net) +
			     ts_deboornet_num_insertions(&net) + 1;
		}
	TS_CATCH(err)
		*k = 0;
	TS_FINALLY
		ts_deboornet_free(&net);
	TS_END_TRY_RETURN(err)
}

tsError
ts_bspline_tension(const tsBSpline *spline,
                   tsReal beta,
                   tsBSpline *out,
                   tsStatus *status)
{
	const size_t dim = ts_bspline_dimension(spline);
	const size_t N = ts_bspline_num_control_points(spline);
	const tsReal* p0 = ts_int_bspline_access_ctrlp(spline);
	const tsReal* pn_1 = p0 + (N-1)*dim;

	tsReal s; /**< The straightening factor. */
	tsReal *ctrlp; /**< Pointer to the control points of `out'. */
	size_t i, d; /**< Used in for loops. */
	tsReal vec; /**< Straightening vector. */
	tsError err;

	TS_CALL_ROE(err, ts_bspline_copy(spline, out, status))
	ctrlp = ts_int_bspline_access_ctrlp(out);
	if (beta < (tsReal) 0.0) beta = (tsReal) 0.0;
	if (beta > (tsReal) 1.0) beta = (tsReal) 1.0;
	s = 1.f - beta;

	for (i = 0; i < N; i++) {
		for (d = 0; d < dim; d++) {
			vec = ((tsReal)i / (N-1)) * (pn_1[d] - p0[d]);
			ctrlp[i*dim + d] = beta * ctrlp[i*dim + d] +
			                   s * (p0[d] + vec);
		}
	}
	TS_RETURN_SUCCESS(status)
}

tsError
ts_bspline_to_beziers(const tsBSpline *spline,
                      tsBSpline *beziers,
                      tsStatus *status)
{
	const size_t deg = ts_bspline_degree(spline);
	const size_t order = ts_bspline_order(spline);

	int resize;    /**< Number of control points to add/remove. */
	size_t k;      /**< Index of the split knot value. */
	tsReal u_min;  /**< Minimum of the knot values. */
	tsReal u_max;  /**< Maximum of the knot values. */

	tsBSpline tmp;    /**< Temporarily stores the result. */
	tsReal *knots;    /**< Pointer to the knots of tmp. */
	size_t num_knots; /**< Number of knots in tmp. */

	tsError err;

	INIT_OUT_BSPLINE(spline, beziers)
	TS_CALL_ROE(err, ts_bspline_copy(spline, &tmp, status))
	knots = ts_int_bspline_access_knots(&tmp);
	num_knots = ts_bspline_num_knots(&tmp);

	TS_TRY(try, err, status)
		/* DO NOT FORGET TO UPDATE knots AND num_knots AFTER EACH
		 * TRANSFORMATION OF tmp! */

		/* Fix first control point if necessary. */
		u_min = knots[deg];
		if (!ts_knots_equal(knots[0], u_min)) {
			TS_CALL(try, err, ts_bspline_split(
			        &tmp, u_min, &tmp, &k, status))
			resize = (int)(-1*deg + (deg*2 - k));
			TS_CALL(try, err, ts_int_bspline_resize(
			        &tmp, resize, 0, &tmp, status))
			knots = ts_int_bspline_access_knots(&tmp);
			num_knots = ts_bspline_num_knots(&tmp);
		}

		/* Fix last control point if necessary. */
		u_max = knots[num_knots - order];
		if (!ts_knots_equal(knots[num_knots - 1], u_max)) {
			TS_CALL(try, err, ts_bspline_split(
			        &tmp, u_max, &tmp, &k, status))
			num_knots = ts_bspline_num_knots(&tmp);
			resize = (int)(-1*deg + (k - (num_knots - order)));
			TS_CALL(try, err, ts_int_bspline_resize(
			        &tmp, resize, 1, &tmp, status))
			knots = ts_int_bspline_access_knots(&tmp);
			num_knots = ts_bspline_num_knots(&tmp);
		}

		/* Split internal knots. */
		k = order;
		while (k < num_knots - order) {
			TS_CALL(try, err, ts_bspline_split(
			        &tmp, knots[k], &tmp, &k, status))
			knots = ts_int_bspline_access_knots(&tmp);
			num_knots = ts_bspline_num_knots(&tmp);
			k++;
		}

		if (spline == beziers)
			ts_bspline_free(beziers);
		ts_bspline_move(&tmp, beziers);
	TS_FINALLY
		ts_bspline_free(&tmp);
	TS_END_TRY_RETURN(err)
}

tsError
ts_bspline_elevate_degree(const tsBSpline *spline,
                          size_t amount,
                          tsReal epsilon,
                          tsBSpline *elevated,
                          tsStatus * status)
{
	tsBSpline worker;
	size_t dim, order;
	tsReal *ctrlp, *knots;
	size_t num_beziers, i, a, c, d, offset, idx;
	tsReal f, f_hat, *first, *last;
	tsError err;

	/* Trivial case. */
	if (amount == 0)
		return ts_bspline_copy(spline, elevated, status);

	/* An overview of this algorithm can be found at:
	 * https://pages.mtu.edu/~shene/COURSES/cs3621/LAB/curve/elevation.html */
	INIT_OUT_BSPLINE(spline, elevated);
	worker = ts_bspline_init();
	TS_TRY(try, err, status)
		/* Decompose `spline' into a sequence of bezier curves and make
		 * space for the additional control points and knots that are
		 * to be inserted. Results are stored in `worker'. */
		TS_CALL(try, err, ts_bspline_to_beziers(
		        spline, &worker, status));
		num_beziers = ts_bspline_num_control_points(&worker) /
		              ts_bspline_order(&worker);
		TS_CALL(try, err, ts_int_bspline_resize(
		        /* Resize by the number of knots to be inserted. Note
		         * that this creates too many control points (due to
		         * increasing the degree), which are removed at the end
		         * of this function. */
		        &worker, (int) ((num_beziers+1) * amount), 1, &worker,
		        status));
		dim = ts_bspline_dimension(&worker);
		order = ts_bspline_order(&worker);
		ctrlp = ts_int_bspline_access_ctrlp(&worker);
		knots = ts_int_bspline_access_knots(&worker);

		/* Move all but the first bezier curve to their new location in
		 * the control point array so that the additional control
		 * points can be inserted without overwriting the others. Note
		 * that iteration must run backwards. Otherwise, the moved
		 * values overwrite each other. */
		for (i = num_beziers - 1; i > 0; i--) {
			/* `i' can be interpreted as the number of bezier
			 * curves before the current bezier curve. */

			/* Location of current bezier curve. */
			offset = i * order * dim;
			/* Each elevation inserts an additional control point
			 * into every bezier curve. `i * amount' is the total
			 * number of control points to be inserted before the
			 * current bezier curve. */
			memmove(ctrlp + offset + (i * amount * dim),
			        ctrlp + offset,
			        dim * order * sizeof(tsReal));
		}

		/* Move all but the first group of knots to their new location
		 * in the knot vector so that the additional knots can be
		 * inserted without overwriting the others. Note that iteration
		 * must run backwards. Otherwise, the moved values overwrite
		 * each other. */
		for (i = num_beziers; i > 0; i--) {
			/* Note that the number of knot groups is one more than
			 * the number of bezier curves. `i' can be interpreted
			 * as the number of knot groups before the current
			 * group. */

			/* Location of current knot group. */
			offset = i * order;
			/* Each elevation inserts an additional knot into every
			 * group of knots. `i * amount' is the total number of
			 * knots to be inserted before the current knot
			 * group. */
			memmove(knots + offset + (i * amount),
			        knots + offset,
			        order * sizeof(tsReal));
		}

		/* `worker' is now fully set up.
		 * The following formulas are based on:
		 * https://pages.mtu.edu/~shene/COURSES/cs3621/NOTES/spline/Bezier/bezier-elev.html */
		for (a = 0; a < amount; a++) {
			/* For each bezier curve... */
			for (i = 0; i < num_beziers; i++) {
				/* ... 1) Insert and update control points. */

				/* Location of current bezier curve. Each
				 * elevation (`a') inserts an additional
				 * control point into every bezier curve and
				 * increases the degree (`order') by one. The
				 * location is thus made up of two parts:
				 *
				 * i) `i * order', which is the location taking
				 * into account the increasing order but
				 * neglecting the control points that are to be
				 * inserted before the current bezier curve. It
				 * can be seen as some sort of base location:
				 * Where would the bezier curve be (with
				 * respect to the current value of `order') if
				 * no additional control points had to be
				 * inserted?
				 *
				 * ii) `i * (amount - a)', which is the total
				 * number of control points to be inserted
				 * before the current bezier curve
				 * (`i * amount') taking into account the
				 * increasing order (`order' and `a' are
				 * increased equally, thus, `a' compensates for
				 * the increasing value of `order'). This part
				 * adds the necessary offset to the base
				 * location (`i * order'). */
				offset = (i * order + i * (amount - a)) * dim;
				/* Duplicate last control point to the new end
				 * position (next control point). */
				memmove(ctrlp + offset + ((order) * dim),
				        ctrlp + offset + ((order-1) * dim),
				        dim * sizeof(tsReal));
				/* All but the outer control points must be
				 * recalculated (domain: [1, order - 1]). By
				 * traversing backwards, control points can be
				 * modified in-place. */
				for (c = order - 1; c > 0; c--) {
					/* Location of current control point
					 * within current bezier curve. */
					idx = offset + c * dim;
					f = (tsReal) c / (tsReal) (order);
					f_hat = 1 - f;
					for (d = 0; d < dim; d++) {
						/* For the sake of space, we
						 * increment idx by d and
						 * decrement it at the end of
						 * this loop. */
						idx += d;
						ctrlp[idx] =
							f * ctrlp[idx - dim] +
							f_hat * ctrlp[idx];
						/* Reset idx. */
						idx -= d;
					}
				}

				/* ...2) Increase the multiplicity of the
				 * second knot group (maximum of the domain of
				 * the current bezier curve) by one. Note that
				 * this loop misses the last knot group (the
				 * group of the last bezier curve) as there is
				 * one more knot group than bezier curves to
				 * process. Thus, the last group must be
				 * increased separately after this loop. */

				/* Location of current knot group. Each
				 * elevation (`a') inserts an additional
				 * knot into the knot vector of every bezier
				 * curve and increases the degree (`order') by
				 * one. The location is thus made up of two
				 * parts:
				 *
				 * i) `i * order', which is the location taking
				 * into account the increasing order but
				 * neglecting the knots that are to be inserted
				 * before the current knot group. It can be
				 * seen as some sort of base location: Where
				 * would the knot group be (with respect to the
				 * current value of `order') if no additional
				 * knots had to be inserted?
				 *
				 * ii) `i * (amount - a)', which is the total
				 * number of knots to be inserted before the
				 * current knot group (`i * amount') taking
				 * into account the increasing order (`order'
				 * and `a' are increased equally, thus, `a'
				 * compensates for the increasing value of
				 * `order'). This part adds the necessary
				 * offset to the base location
				 * (`i * order'). */
				offset = i * order + i * (amount - a);
				/* Duplicate knot. */
				knots[offset + order] = knots[offset];
			}

			/* Increase the multiplicity of the very last knot
			 * group (the second group of the last bezier curve)
			 * by one. For more details, see knot duplication in
			 * previous loop. */
			offset = num_beziers * order +
			         num_beziers * (amount - a);
			knots[offset + order] = knots[offset];

			/* Elevated by one. */
			order++;
		}

		/* Combine bezier curves. */
		d = 0; /* Number of removed knots/control points. */
		for (i = 0; i < num_beziers - 1; i++) {
			/* Is the last control point of bezier curve `i' equal
			 * to the first control point of bezier curve `i+1'? */
			last = ctrlp + (
				i * order /* base location of `i' */
				- d /* minus the number of removed values */
				+ (order - 1) /* jump to last control point */
				) * dim;
			first = last + dim; /* next control point */
			if (ts_distance(last, first, dim) <= epsilon) {
				/* Move control points. */
				memmove(last, first, (num_beziers - 1 - i) *
				        order * dim * sizeof(tsReal));

				/* Move knots. `last' is the last knot of the
				 * second knot group of bezier curve `i'.
				 * `first' is the first knot of the first knot
				 * group of bezier curve `i+1'. The
				 * calculations are quite similar to those for
				 * the control points `last' and `first' (see
				 * above). */
				last = knots + i * order - d + (2 * order - 1);
				first = last + 1;
				memmove(last, first, (num_beziers - 1 - i) *
				        order * sizeof(tsReal));

				/* Removed one knot/control point. */
				d++;
			}
		}

		/* Repair internal state. */
		worker.pImpl->deg = order - 1;
		worker.pImpl->n_knots -= d;
		worker.pImpl->n_ctrlp = ts_bspline_num_knots(&worker) - order;
		memmove(ts_int_bspline_access_knots(&worker),
		        knots, ts_bspline_sof_knots(&worker));
		worker.pImpl = realloc(worker.pImpl,
		                       ts_int_bspline_sof_state(&worker));
		if (worker.pImpl == NULL) {
			TS_THROW_0(try, err, status, TS_MALLOC,
			           "out of memory")
		}

		/* Move `worker' to output parameter. */
		if (spline == elevated)
			ts_bspline_free(elevated);
		ts_bspline_move(&worker, elevated);
	TS_FINALLY
		ts_bspline_free(&worker);
	TS_END_TRY_RETURN(err)
}

tsError
ts_bspline_align(const tsBSpline *s1,
                 const tsBSpline *s2,
                 tsReal epsilon,
                 tsBSpline *s1_out,
                 tsBSpline *s2_out,
                 tsStatus *status)
{
	tsBSpline s1_worker, s2_worker, *smaller, *larger;
	tsDeBoorNet net; /* the net of `smaller'. */
	size_t i, missing, remaining;
	tsReal min, max, shift, nextKnot;
	tsError err;

	INIT_OUT_BSPLINE(s1, s1_out)
	INIT_OUT_BSPLINE(s2, s2_out)
	s1_worker = ts_bspline_init();
	s2_worker = ts_bspline_init();
	smaller = larger = NULL;
	TS_TRY(try, err, status)
		/* Set up `s1_worker' and `s2_worker'. After this
		 * if-elseif-else-block, `s1_worker' and `s2_worker' have same
		 * degree. */
		if (ts_bspline_degree(s1) < ts_bspline_degree(s2)) {
			TS_CALL(try, err, ts_bspline_elevate_degree(s1,
			        ts_bspline_degree(s2) - ts_bspline_degree(s1),
			        epsilon, &s1_worker, status))
			TS_CALL(try, err, ts_bspline_copy(
			        s2, &s2_worker, status))
		} else if (ts_bspline_degree(s2) < ts_bspline_degree(s1)) {
			TS_CALL(try, err, ts_bspline_elevate_degree(s2,
			        ts_bspline_degree(s1) - ts_bspline_degree(s2),
			        epsilon, &s2_worker, status))
			TS_CALL(try, err, ts_bspline_copy(
			        s1, &s1_worker, status))
		} else {
			TS_CALL(try, err, ts_bspline_copy(
			        s1, &s1_worker, status))
			TS_CALL(try, err, ts_bspline_copy(
			        s2, &s2_worker, status))
		}

		/* Set up `smaller', `larger', and `net'. */
		if (ts_bspline_num_knots(&s1_worker) <
		    ts_bspline_num_knots(&s2_worker)) {
			smaller = &s1_worker;
			larger  = &s2_worker;
		} else {
			smaller = &s2_worker;
			larger  = &s1_worker;
		}
		TS_CALL(try, err, ts_int_deboornet_new(
		        smaller, &net, status))

		/* Insert knots into `smaller' until it has the same number of
		 * knots (and therefore the same number of control points) as
		 * `larger'. */
		ts_bspline_domain(smaller, &min, &max);
		missing = remaining = ts_bspline_num_knots(larger) -
			              ts_bspline_num_knots(smaller);
		shift = (tsReal) 0.0;
		if (missing > 0)
			shift = ( (tsReal) 1.0 / missing ) * (tsReal) 0.5;
		for (i = 0; remaining > 0; i++, remaining--) {
			nextKnot = (max - min) * ((tsReal)i / missing) + min;
			nextKnot += shift;
			TS_CALL(try, err, ts_int_bspline_eval_woa(
			        smaller, nextKnot, &net, status))
			while (!ts_deboornet_num_insertions(&net)) {
				/* Linear exploration for next knot. */
				nextKnot += 5 * TS_KNOT_EPSILON;
				if (nextKnot > max) {
					TS_THROW_0(try, err, status,
					           TS_NO_RESULT,
					          "no more knots for insertion")
				}
				TS_CALL(try, err, ts_int_bspline_eval_woa(
				        smaller, nextKnot, &net, status))
			}
			TS_CALL(try, err, ts_int_bspline_insert_knot(
			        smaller, &net, 1, smaller, status))
		}

		if (s1 == s1_out)
			ts_bspline_free(s1_out);
		if (s2 == s2_out)
			ts_bspline_free(s2_out);
		ts_bspline_move(&s1_worker, s1_out);
		/* if `s1_out' == `s2_out', `s2_worker' must not be moved
		 * because otherwise the memory of `s1_worker' is leaked
		 * (`s2_worker' overrides `s1_worker'). */
		if (s1_out != s2_out)
			ts_bspline_move(&s2_worker, s2_out);
	TS_FINALLY
		ts_bspline_free(&s1_worker);
		ts_bspline_free(&s2_worker);
		ts_deboornet_free(&net);
	TS_END_TRY_RETURN(err)
}

tsError
ts_bspline_morph(const tsBSpline *origin,
                 const tsBSpline *target,
                 tsReal t,
                 tsReal epsilon,
                 tsBSpline *out,
                 tsStatus *status)
{
	tsBSpline origin_al, target_al; /* aligned origin and target */
	tsReal *origin_al_c, *origin_al_k; /* control points and knots */
	tsReal *target_al_c, *target_al_k; /* control points and knots */

	/* Properties of `out'. */
	size_t deg, dim, num_ctrlp, num_knots;
	tsReal *ctrlp, *knots;
	tsBSpline tmp; /* temporary buffer if `out' must be resized */

	tsReal t_hat;
	size_t i, offset, d;
	tsError err;

	origin_al = ts_bspline_init();
	target_al = ts_bspline_init();
	TS_TRY(try, err, status)
		/* Clamp `t' to domain [0, 1] and set up `t_hat'. */
		if (t < (tsReal) 0.0) t = (tsReal) 0.0;
		if (t > (tsReal) 1.0) t = (tsReal) 1.0;
		t_hat = (tsReal) 1.0 - t;

		/* Set up `origin_al' and `target_al'. */
		/* Degree must be elevated... */
		if (ts_bspline_degree(origin) != ts_bspline_degree(target) ||
		 /* .. or knots (and thus control points) must be inserted. */
		 ts_bspline_num_knots(origin) != ts_bspline_num_knots(target)) {
			TS_CALL(try, err, ts_bspline_align(
			        origin, target, epsilon, &origin_al, &target_al,
			        status));
		} else {
			/* Flat copy. */
			origin_al = *origin;
			target_al = *target;
		}
		origin_al_c = ts_int_bspline_access_ctrlp(&origin_al);
		origin_al_k = ts_int_bspline_access_knots(&origin_al);
		target_al_c = ts_int_bspline_access_ctrlp(&target_al);
		target_al_k = ts_int_bspline_access_knots(&target_al);

		/* Set up `out'. */
		deg = ts_bspline_degree(&origin_al);
		num_ctrlp = ts_bspline_num_control_points(&origin_al);
		dim = ts_bspline_dimension(&origin_al);
		if (ts_bspline_dimension(&target_al) < dim)
			dim = ts_bspline_dimension(&target_al);
		if (out->pImpl == NULL) {
			TS_CALL(try, err, ts_bspline_new(num_ctrlp, dim, deg,
			        TS_OPENED /* doesn't matter */, out, status))
		} else if (ts_bspline_degree(out) != deg ||
		           ts_bspline_num_control_points(out) != num_ctrlp ||
		           ts_bspline_dimension(out) != dim) {
			TS_CALL(try, err, ts_bspline_new(num_ctrlp, dim, deg,
			        TS_OPENED /* doesn't matter */, &tmp, status))
			ts_bspline_free(out);
			ts_bspline_move(&tmp, out);
		}
		num_knots = ts_bspline_num_knots(out);
		ctrlp = ts_int_bspline_access_ctrlp(out);
		knots = ts_int_bspline_access_knots(out);

		/* Interpolate control points. */
		for (i = 0; i < num_ctrlp; i++) {
			for (d = 0; d < dim; d++) {
				offset = i * dim + d;
				ctrlp[offset] = t * target_al_c[offset] +
				                t_hat * origin_al_c[offset];
			}
		}

		/* Interpolate knots. */
		for (i = 0; i < num_knots; i++) {
			knots[i] = t * target_al_k[i] +
			           t_hat * origin_al_k[i];
		}
	TS_FINALLY
		if (origin->pImpl != origin_al.pImpl)
			ts_bspline_free(&origin_al);
		if (target->pImpl != target_al.pImpl)
			ts_bspline_free(&target_al);
	TS_END_TRY_RETURN(err)
}
/*! @} */



/*! @name Serialization and Persistence
 *
 * @{
 */
tsError
ts_int_bspline_to_json(const tsBSpline *spline,
                       JSON_Value **value,
                       tsStatus *status)
{
	const size_t deg = ts_bspline_degree(spline);
	const size_t dim = ts_bspline_dimension(spline);
	const size_t len_ctrlp = ts_bspline_len_control_points(spline);
	const size_t len_knots = ts_bspline_num_knots(spline);
	const tsReal *ctrlp = ts_int_bspline_access_ctrlp(spline);
	const tsReal *knots = ts_int_bspline_access_knots(spline);

	size_t i; /**< Used in loops */
	tsError err;

	JSON_Value  *ctrlp_value;
	JSON_Value  *knots_value;
	JSON_Object *spline_object;
	JSON_Array  *ctrlp_array;
	JSON_Array  *knots_array;

	*value = ctrlp_value = knots_value = NULL;
	TS_TRY(values, err, status)
		/* Init memory. */
		*value = json_value_init_object();
		if (!*value) {
			TS_THROW_0(values, err, status, TS_MALLOC,
			           "out of memory")
		}
		ctrlp_value = json_value_init_array();
		if (!ctrlp_value) {
			TS_THROW_0(values, err, status, TS_MALLOC,
			           "out of memory")
		}
		knots_value = json_value_init_array();
		if (!knots_value) {
			TS_THROW_0(values, err, status, TS_MALLOC,
			           "out of memory")
		}

		/* Although the following functions cannot fail, that is, they
		 * won't return NULL or JSONFailure, we nevertheless handle
		 * unexpected return values. */

		/* Init output. */
		spline_object = json_value_get_object(*value);
		if (!spline_object) {
			TS_THROW_0(values, err, status, TS_MALLOC,
			           "out of memory")
		}

		/* Set degree and dimension. */
		if (JSONSuccess != json_object_set_number(spline_object,
		                                          "degree",
		                                          (double) deg)) {
			TS_THROW_0(values, err, status, TS_MALLOC,
			           "out of memory")
		}
		if (JSONSuccess != json_object_set_number(spline_object,
		                                          "dimension",
		                                          (double) dim)) {
			TS_THROW_0(values, err, status, TS_MALLOC,
			           "out of memory")
		}

		/* Set control points. */
		if (JSONSuccess != json_object_set_value(spline_object,
		                                         "control_points",
		                                         ctrlp_value)) {
			TS_THROW_0(values, err, status, TS_MALLOC,
			           "out of memory")
		}
		ctrlp_array = json_array(ctrlp_value);
		if (!ctrlp_array) {
			TS_THROW_0(values, err, status, TS_MALLOC,
			           "out of memory")
		}
		for (i = 0; i < len_ctrlp; i++) {
			if (JSONSuccess != json_array_append_number(
				    ctrlp_array, (double) ctrlp[i])) {
				TS_THROW_0(values, err, status, TS_MALLOC,
				           "out of memory")
			}
		}

		/* Set knots. */
		if (JSONSuccess != json_object_set_value(spline_object,
		                                         "knots",
		                                         knots_value)) {
			TS_THROW_0(values, err, status, TS_MALLOC,
			           "out of memory")
		}
		knots_array = json_array(knots_value);
		if (!knots_array) {
			TS_THROW_0(values, err, status, TS_MALLOC,
				"out of memory")
		}
		for (i = 0; i < len_knots; i++) {
			if (JSONSuccess != json_array_append_number(
				    knots_array, (double) knots[i])) {
				TS_THROW_0(values, err, status, TS_MALLOC,
				           "out of memory")
			}
		}
	TS_CATCH(err)
		if (*value)
			json_value_free(*value);
		if (ctrlp_value && !json_value_get_parent(ctrlp_value))
			json_value_free(ctrlp_value);
		if (knots_value && !json_value_get_parent(knots_value))
			json_value_free(knots_value);
		*value = NULL;
	TS_END_TRY_RETURN(err)
}

tsError
ts_int_bspline_parse_json(const JSON_Value *spline_value,
                          tsBSpline *spline,
                          tsStatus *status)
{
	size_t deg, dim, len_ctrlp, num_knots;
	tsReal *ctrlp, *knots;

	JSON_Object *spline_object;
	JSON_Value *deg_value;
	JSON_Value *dim_value;
	JSON_Value *ctrlp_value;
	JSON_Array *ctrlp_array;
	JSON_Value *knots_value;
	JSON_Array *knots_array;
	JSON_Value *real_value;
	size_t i;
	tsError err;

	ts_int_bspline_init(spline);

	/* Read spline object. */
	if (json_value_get_type(spline_value) != JSONObject)
		TS_RETURN_0(status, TS_PARSE_ERROR, "invalid json input")
	spline_object = json_value_get_object(spline_value);
	if (!spline_object)
		TS_RETURN_0(status, TS_PARSE_ERROR, "invalid json input")

	/* Read degree. */
	deg_value = json_object_get_value(spline_object, "degree");
	if (json_value_get_type(deg_value) != JSONNumber)
		TS_RETURN_0(status, TS_PARSE_ERROR, "degree is not a number")
	if (json_value_get_number(deg_value) < -0.01f) {
		TS_RETURN_1(status, TS_PARSE_ERROR,
		            "degree (%f) < 0",
		            json_value_get_number(deg_value))
	}
	deg = (size_t) json_value_get_number(deg_value);

	/* Read dimension. */
	dim_value = json_object_get_value(spline_object, "dimension");
	if (json_value_get_type(dim_value) != JSONNumber) {
		TS_RETURN_0(status, TS_PARSE_ERROR,
		            "dimension is not a number")
	}
	if (json_value_get_number(dim_value) < 0.99f) {
		TS_RETURN_1(status, TS_PARSE_ERROR,
		            "dimension (%f) < 1",
		            json_value_get_number(deg_value))
	}
	dim = (size_t) json_value_get_number(dim_value);

	/* Read length of control point vector. */
	ctrlp_value = json_object_get_value(spline_object, "control_points");
	if (json_value_get_type(ctrlp_value) != JSONArray) {
		TS_RETURN_0(status, TS_PARSE_ERROR,
		            "control_points is not an array")
	}
	ctrlp_array = json_value_get_array(ctrlp_value);
	len_ctrlp = json_array_get_count(ctrlp_array);
	if (len_ctrlp % dim != 0) {
		TS_RETURN_2(status, TS_PARSE_ERROR,
		           "len(control_points) (%lu) %% dimension (%lu) != 0",
		            (unsigned long) len_ctrlp, (unsigned long) dim)
	}

	/* Read number of knots. */
	knots_value = json_object_get_value(spline_object, "knots");
	if (json_value_get_type(knots_value) != JSONArray) {
		TS_RETURN_0(status, TS_PARSE_ERROR,
		            "knots is not an array")
	}
	knots_array = json_value_get_array(knots_value);
	num_knots = json_array_get_count(knots_array);

	/* Create spline. */
	TS_TRY(try, err, status)
		TS_CALL(try, err, ts_bspline_new(
		        len_ctrlp/dim, dim, deg,
		        TS_CLAMPED, spline, status))
		if (num_knots != ts_bspline_num_knots(spline))
			TS_THROW_2(try, err, status, TS_NUM_KNOTS,
			           "unexpected num(knots): (%lu) != (%lu)",
			           (unsigned long) num_knots,
			          (unsigned long) ts_bspline_num_knots(spline))

		/* Set control points. */
		ctrlp = ts_int_bspline_access_ctrlp(spline);
		for (i = 0; i < len_ctrlp; i++) {
			real_value = json_array_get_value(ctrlp_array, i);
			if (json_value_get_type(real_value) != JSONNumber)
				TS_THROW_1(try, err, status, TS_PARSE_ERROR,
				"control_points: value at index %lu is not a number",
				           (unsigned long) i)
			ctrlp[i] = (tsReal) json_value_get_number(real_value);
		}
		TS_CALL(try, err, ts_bspline_set_control_points(
		        spline, ctrlp, status))

		/* Set knots. */
		knots = ts_int_bspline_access_knots(spline);
		for (i = 0; i < num_knots; i++) {
			real_value = json_array_get_value(knots_array, i);
			if (json_value_get_type(real_value) != JSONNumber)
			TS_THROW_1(try, err, status, TS_PARSE_ERROR,
			           "knots: value at index %lu is not a number",
			           (unsigned long) i)
			knots[i] = (tsReal) json_value_get_number(real_value);
		}
		TS_CALL(try, err, ts_bspline_set_knots(
		        spline, knots, status))
	TS_CATCH(err)
		ts_bspline_free(spline);
	TS_END_TRY_RETURN(err)
}

tsError
ts_bspline_to_json(const tsBSpline *spline,
                   char **json,
                   tsStatus *status)
{
	tsError err;
	JSON_Value *value = NULL;
	*json = NULL;
	TS_CALL_ROE(err, ts_int_bspline_to_json(spline, &value, status))
	*json = json_serialize_to_string_pretty(value);
	json_value_free(value);
	if (!*json)
		TS_RETURN_0(status, TS_MALLOC, "out of memory")
	TS_RETURN_SUCCESS(status)
}

tsError
ts_bspline_parse_json(const char *json,
                      tsBSpline *spline,
                      tsStatus *status)
{
	tsError err;
	JSON_Value *value = NULL;
	ts_int_bspline_init(spline);
	TS_TRY(try, err, status)
		value = json_parse_string(json);
		if (!value) {
			TS_RETURN_0(status, TS_PARSE_ERROR,
			            "invalid json input")
		}
		TS_CALL(try, err, ts_int_bspline_parse_json(
		        value, spline, status))
	TS_FINALLY
		if (value)
			json_value_free(value);
	TS_END_TRY_RETURN(err)
}

tsError
ts_bspline_save(const tsBSpline *spline,
                const char *path,
                tsStatus *status)
{
	tsError err;
	JSON_Status json_status;
	JSON_Value *value = NULL;
	TS_CALL_ROE(err, ts_int_bspline_to_json(spline, &value, status))
	json_status = json_serialize_to_file_pretty(value, path);
	json_value_free(value);
	if (json_status != JSONSuccess)
		TS_RETURN_0(status, TS_IO_ERROR, "unexpected io error")
	TS_RETURN_SUCCESS(status)
}

tsError
ts_bspline_load(const char *path,
                tsBSpline *spline,
                tsStatus *status)
{
	tsError err;
	FILE *file = NULL;
	JSON_Value *value = NULL;
	ts_int_bspline_init(spline);
	TS_TRY(try, err, status)
		file = fopen(path, "r");
		if (!file) {
			TS_THROW_0(try, err, status, TS_IO_ERROR,
			           "unable to open file")
		}
		value = json_parse_file(path);
		if (!value) {
			TS_RETURN_0(status, TS_PARSE_ERROR,
			            "invalid json input")
		}
		TS_CALL(try, err, ts_int_bspline_parse_json(
		        value, spline, status))
	TS_FINALLY
		if (file)
			fclose(file);
		if (value)
			json_value_free(value);
	TS_CATCH(err)
		ts_bspline_free(spline);
	TS_END_TRY_RETURN(err)
}
/*! @} */



/*! @name Vector Math
 * @{
 */
void
ts_vec2_init(tsReal *out,
             tsReal x,
             tsReal y)
{
	out[0] = x;
	out[1] = y;
}

void
ts_vec3_init(tsReal *out,
             tsReal x,
             tsReal y,
             tsReal z)
{
	out[0] = x;
	out[1] = y;
	out[2] = z;
}

void
ts_vec4_init(tsReal *out,
             tsReal x,
             tsReal y,
             tsReal z,
             tsReal w)
{
	out[0] = x;
	out[1] = y;
	out[2] = z;
	out[3] = w;
}

void
ts_vec2_set(tsReal *out,
            const tsReal *x,
            size_t dim)
{
	const size_t n = dim > 2 ? 2 : dim;
	memmove(out, x, n * sizeof(tsReal));
	if (dim < 2)
		ts_arr_fill(out + dim, 2 - dim, (tsReal) 0.0);
}

void
ts_vec3_set(tsReal *out,
            const tsReal *x,
            size_t dim)
{
	const size_t n = dim > 3 ? 3 : dim;
	memmove(out, x, n * sizeof(tsReal));
	if (dim < 3)
		ts_arr_fill(out + dim, 3 - dim, (tsReal) 0.0);
}

void
ts_vec4_set(tsReal *out,
            const tsReal *x,
            size_t dim)
{
	const size_t n = dim > 4 ? 4 : dim;
	memmove(out, x, n * sizeof(tsReal));
	if (dim < 4)
		ts_arr_fill(out + dim, 4 - dim, (tsReal) 0.0);
}

void
ts_vec_add(const tsReal *x,
           const tsReal *y,
           size_t dim,
           tsReal *out)
{
	size_t i;
	for (i = 0; i < dim; i++)
		out[i] = x[i] + y[i];
}

void
ts_vec_sub(const tsReal *x,
           const tsReal *y,
           size_t dim,
           tsReal *out)
{
	size_t i;
	if (x == y) {
		/* More stable version. */
		ts_arr_fill(out, dim, (tsReal) 0.0);
		return;
	}
	for (i = 0; i < dim; i++)
		out[i] = x[i] - y[i];
}

tsReal
ts_vec_dot(const tsReal *x,
           const tsReal *y,
           size_t dim)
{
	size_t i;
	tsReal dot = 0;
	for (i = 0; i < dim; i++)
		dot += x[i] * y[i];
	return dot;
}

tsReal
ts_vec_angle(const tsReal *x,
             const tsReal *y,
             tsReal *buf,
             size_t dim)
{
	const tsReal *x_norm, *y_norm;
	if (buf) {
		ts_vec_norm(x, dim, buf);
		ts_vec_norm(y, dim, buf + dim);
		x_norm = buf;
		y_norm = buf + dim;
	} else {
		x_norm = x;
		y_norm = y;
	}
	return (tsReal) (
		/* Use doubles as long as possible. */
		acos(ts_vec_dot(x_norm,
		                y_norm,
		                dim))
		* (180.0 / TS_PI) /* radiant to degree */
		);
}

void
ts_vec3_cross(const tsReal *x,
              const tsReal *y,
              tsReal *out)
{
	tsReal a, b, c;
	a = x[1] * y[2] - x[2] * y[1];
	b = x[2] * y[0] - x[0] * y[2];
	c = x[0] * y[1] - x[1] * y[0];
	out[0] = a;
	out[1] = b;
	out[2] = c;
}

void
ts_vec_norm(const tsReal *x,
            size_t dim,
            tsReal *out)
{
	size_t i;
	const tsReal m = ts_vec_mag(x, dim);
	if (m < TS_LENGTH_ZERO) {
		ts_arr_fill(out, dim, (tsReal) 0.0);
		return;
	}
	for (i = 0; i < dim; i++)
		out[i] = x[i] / m;
}

tsReal
ts_vec_mag(const tsReal *x,
           size_t dim)
{
	size_t i;
	tsReal sum = 0;
	for (i = 0; i < dim; i++)
		sum += (x[i] * x[i]);
	return (tsReal) sqrt(sum);
}

void
ts_vec_mul(const tsReal *x,
           size_t dim,
           tsReal val,
           tsReal *out)
{
	size_t i;
	for (i = 0; i < dim; i++)
		out[i] = x[i] * val;
}
/*! @} */



/*! @name Chord Length Method
 *
 * @{
 */
tsError
ts_chord_lengths_length_to_knot(const tsReal *knots,
                                const tsReal *lengths,
                                size_t num,
                                tsReal len,
                                tsReal *knot,
                                tsStatus *status)
{
	tsReal numer, denom, r, r_hat;
	size_t idx, low, high;

	/* Handle spacial cases. */
	if (num == 0) { /* well... */
		TS_RETURN_0(status, TS_NO_RESULT, "empty chord lengths")
	}
	if (num == 1) { /* no computation needed */
		*knot = knots[0];
		TS_RETURN_SUCCESS(status)
	}
	if (lengths[num - 1] < TS_LENGTH_ZERO) { /* spline is too short */
		*knot = knots[0];
		TS_RETURN_SUCCESS(status)
	}
	if (len <= lengths[0]) { /* clamp `len' to lower bound */
		*knot = knots[0];
		TS_RETURN_SUCCESS(status)
	}
	if (len >= lengths[num - 1]) { /* clamp `len' to upper bound */
		*knot = knots[num - 1];
		TS_RETURN_SUCCESS(status)
	}

	/* From now on: i) `len' is less than the last chord length in
	   `lengths' and ii) `lengths' contains at least two values. Hence, the
	   index (`idx') determined by the following binary search cannot be
	   the last index in `knots' and `lengths', respectively (i.e., `idx <=
	   num - 2`). It is therefore safe to access `knots' and `lengths' at
	   index `idx + 1`. */

	/* Binary search. Similar to how locating a knot within a knot vector
	   is implemented in ::ts_int_bspline_find_knot. */
	low = 0;
	high = num - 1;
	idx = (low+high) / 2;
	while (len < lengths[idx] || len >= lengths[(idx) + 1]) {
		if (len < lengths[idx]) high = idx;
		else                     low  = idx;
		idx = (low+high) / 2;
	}

	/* Determine `knot' by linear interpolation. */
	denom = lengths[(idx) + 1] - lengths[idx];
	if (denom < TS_LENGTH_ZERO) { /* segment is too short */
		*knot = knots[idx];
		TS_RETURN_SUCCESS(status)
	}
	numer = len - lengths[idx];
	r = numer / denom; /* denom >= TS_LENGTH_ZERO */
	r_hat = (tsReal) 1.0 - r;
	*knot = r * knots[(idx) + 1] + r_hat * knots[idx];
	TS_RETURN_SUCCESS(status)
}

tsError
ts_chord_lengths_t_to_knot(const tsReal *knots,
                           const tsReal *lengths,
                           size_t num,
                           tsReal t,
                           tsReal *knot,
                           tsStatus *status)
{
	/* Delegate error handling.  If `num' is `0`,
	  `ts_chord_lengths_length_to_knot' doesn't read `len' at all. */
	tsReal len = num == 0 ? 0 : t * lengths[num - 1];
	return ts_chord_lengths_length_to_knot(knots,
	                                       lengths,
	                                       num,
	                                       len,
	                                       knot,
	                                       status);
}

tsError
ts_chord_lengths_equidistant_knot_seq(const tsReal *knots,
                                      const tsReal *lengths,
                                      size_t num,
                                      size_t num_knot_seq,
                                      tsReal *knot_seq,
                                      tsStatus *status)
{
	tsError err;
	size_t i;
	tsReal t, knot;
	if (num_knot_seq == 0) TS_RETURN_SUCCESS(status)
	TS_TRY(try, err, status)
		for (i = 0; i < num_knot_seq; i++) {
			t = (tsReal) i / (num_knot_seq - 1);
			TS_CALL(try, err, ts_chord_lengths_t_to_knot(
			        knots, lengths, num, t, &knot, status))
			knot_seq[i] = knot;
		}
		/* Set `knot_seq[0]` after `knot_seq[num_knot_seq - 1]` to
		   ensure that `knot_seq[0] = min` if `num_knot_seq` is
		   `1'. Note that `num_knot_seq` and `num` can't be `0'. */
		knot_seq[num_knot_seq - 1] = knots[num - 1];
		knot_seq[0] = knots[0];
	TS_END_TRY_RETURN(err)
}
/*! @} */



/*! @name Utility Functions
 *
 * @{
 */
int ts_knots_equal(tsReal x,
                   tsReal y)
{
	return fabs(x-y) < TS_KNOT_EPSILON ? 1 : 0;
}

void ts_arr_fill(tsReal *arr,
                 size_t num,
                 tsReal val)
{
	size_t i;
	for (i = 0; i < num; i++)
		arr[i] = val;
}

tsReal ts_distance(const tsReal *x,
                   const tsReal *y,
                   size_t dim)
{
	size_t i;
	tsReal sum = 0;
	for (i = 0; i < dim; i++)
		sum += (x[i] - y[i]) * (x[i] - y[i]);
	return (tsReal) sqrt(sum);
}
/*! @} */

#ifdef _MSC_VER
#pragma warning(pop)
#endif
