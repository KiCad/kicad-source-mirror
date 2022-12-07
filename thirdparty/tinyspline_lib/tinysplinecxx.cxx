#define TINYSPLINE_EXPORT
#include "tinysplinecxx.h"

#include <stdlib.h>
#include <cstring>
#include <stdexcept>
#include <cstdio>
#include <sstream>

/* Suppress some useless MSVC warnings. */
#ifdef _MSC_VER
#pragma warning(push)
/* address of dllimport */
#pragma warning(disable:4232)
/* binding rvalues to non-const references */
#pragma warning(disable:4350)
/* unreferenced inline function */
#pragma warning(disable:4514)
/* function not inlined */
#pragma warning(disable:4710)
/* byte padding */
#pragma warning(disable:4820)
/* meaningless deprecation */
#pragma warning(disable:4996)
/* Spectre mitigation */
#pragma warning(disable:5045)
#endif



/*! @name Swig Type Mapping
 *
 * See tinysplinecxx.h for more details.
 *
 * @{
 */
#ifdef SWIG
#define std_real_vector_init(var) \
	std_real_vector_out var = new std::vector<tinyspline::real>
#define std_real_vector_read(var) var->
#else
#define std_real_vector_init(var) \
	std_real_vector_out var
#define std_real_vector_read(var) var.
#endif
/*! @} */



/*! @name Vec2
 *
 * @{
 */
tinyspline::Vec2::Vec2()
{
	const real v = (real) 0.0;
	ts_vec2_init(m_vals, v, v);
}

tinyspline::Vec2::Vec2(real x,
                       real y)
{
	ts_vec2_init(m_vals, x, y);
}

tinyspline::Vec2
tinyspline::Vec2::operator+(const Vec2 &other)
{
	return add(other);
}

tinyspline::Vec2
tinyspline::Vec2::operator-(const Vec2 &other)
{
	return subtract(other);
}

tinyspline::Vec2
tinyspline::Vec2::operator*(real scalar)
{
	return multiply(scalar);
}

tinyspline::real
tinyspline::Vec2::x() const
{
	return m_vals[0];
}

void
tinyspline::Vec2::setX(real val)
{
	m_vals[0] = val;
}

tinyspline::real
tinyspline::Vec2::y() const
{
	return m_vals[1];
}

void
tinyspline::Vec2::setY(real val)
{
	m_vals[1] = val;
}

std::vector<tinyspline::real>
tinyspline::Vec2::values() const
{
	return std::vector<real>({ x(), y() });
}

tinyspline::Vec2
tinyspline::Vec2::add(const Vec2 &other) const
{
	Vec2 vec;
	ts_vec_add(m_vals, other.m_vals, 2, vec.m_vals);
	return vec;
}

tinyspline::Vec2
tinyspline::Vec2::subtract(const Vec2 &other) const
{
	Vec2 vec;
	ts_vec_sub(m_vals, other.m_vals, 2, vec.m_vals);
	return vec;
}

tinyspline::Vec2
tinyspline::Vec2::multiply(real scalar) const
{
	Vec2 vec;
	ts_vec_mul(m_vals, 2, scalar, vec.m_vals);
	return vec;
}

tinyspline::Vec2
tinyspline::Vec2::normalize() const
{
	Vec2 vec;
	ts_vec_norm(m_vals, 2, vec.m_vals);
	return vec;
}

tinyspline::Vec2
tinyspline::Vec2::norm() const
{
	return normalize();
}

tinyspline::real
tinyspline::Vec2::magnitude() const
{
	return ts_vec_mag(m_vals, 2);
}

tinyspline::real
tinyspline::Vec2::dot(const Vec2 &other) const
{
	return ts_vec_dot(m_vals, other.m_vals, 2);
}

tinyspline::real
tinyspline::Vec2::angle(const Vec2 &other) const
{
	real buf[4];
	return ts_vec_angle(m_vals, other.m_vals, buf, 2);
}

tinyspline::real
tinyspline::Vec2::distance(const Vec2 &other) const
{
	return ts_distance(m_vals, other.m_vals, 2);
}

std::string
tinyspline::Vec2::toString() const
{
	std::ostringstream oss;
	oss << "Vec2{"
	    << "x: " << x()
	    << ", y: " << y()
	    << "}";
	return oss.str();
}
/*! @} */



/*! @name Vec3
 *
 * @{
 */
tinyspline::Vec3::Vec3()
{
	const real v = (real) 0.0;
	ts_vec3_init(m_vals, v, v, v);
}

tinyspline::Vec3::Vec3(real x,
                       real y,
                       real z)
{
	ts_vec3_init(m_vals, x, y, z);
}

tinyspline::Vec3
tinyspline::Vec3::operator+(const Vec3 &other)
{
	return add(other);
}

tinyspline::Vec3
tinyspline::Vec3::operator-(const Vec3 &other)
{
	return subtract(other);
}

tinyspline::Vec3
tinyspline::Vec3::operator*(real scalar)
{
	return multiply(scalar);
}

tinyspline::real
tinyspline::Vec3::x() const
{
	return m_vals[0];
}

void
tinyspline::Vec3::setX(real val)
{
	m_vals[0] = val;
}

tinyspline::real
tinyspline::Vec3::y() const
{
	return m_vals[1];
}

void
tinyspline::Vec3::setY(real val)
{
	m_vals[1] = val;
}

tinyspline::real
tinyspline::Vec3::z() const
{
	return m_vals[2];
}

void
tinyspline::Vec3::setZ(real val)
{
	m_vals[2] = val;
}

std::vector<tinyspline::real>
tinyspline::Vec3::values() const
{
	return std::vector<real>({ x(), y(), z() });
}

tinyspline::Vec3
tinyspline::Vec3::add(const Vec3 &other) const
{
	Vec3 vec;
	ts_vec_add(m_vals, other.m_vals, 3, vec.m_vals);
	return vec;
}

tinyspline::Vec3
tinyspline::Vec3::subtract(const Vec3 &other) const
{
	Vec3 vec;
	ts_vec_sub(m_vals, other.m_vals, 3, vec.m_vals);
	return vec;
}

tinyspline::Vec3
tinyspline::Vec3::multiply(real scalar) const
{
	Vec3 vec;
	ts_vec_mul(m_vals, 3, scalar, vec.m_vals);
	return vec;
}

tinyspline::Vec3
tinyspline::Vec3::cross(const Vec3 &other) const
{
	Vec3 vec;
	ts_vec3_cross(m_vals, other.m_vals, vec.m_vals);
	return vec;
}

tinyspline::Vec3
tinyspline::Vec3::normalize() const
{
	Vec3 vec;
	ts_vec_norm(m_vals, 3, vec.m_vals);
	return vec;
}

tinyspline::Vec3
tinyspline::Vec3::norm() const
{
	return normalize();
}

tinyspline::real
tinyspline::Vec3::magnitude() const
{
	return ts_vec_mag(m_vals, 3);
}

tinyspline::real
tinyspline::Vec3::dot(const Vec3 &other) const
{
	return ts_vec_dot(m_vals, other.m_vals, 3);
}

tinyspline::real
tinyspline::Vec3::angle(const Vec3 &other) const
{
	real buf[6];
	return ts_vec_angle(m_vals, other.m_vals, buf, 3);
}

tinyspline::real
tinyspline::Vec3::distance(const Vec3 &other) const
{
	return ts_distance(m_vals, other.m_vals, 3);
}

std::string
tinyspline::Vec3::toString() const
{
	std::ostringstream oss;
	oss << "Vec3{"
	    << "x: " << x()
	    << ", y: " << y()
	    << ", z: " << z()
	    << "}";
	return oss.str();
}
/*! @} */



/*! @name Vec4
 *
 * @{
 */
tinyspline::Vec4::Vec4()
{
	const real v = (real) 0.0;
	ts_vec4_init(m_vals, v, v, v, v);
}

tinyspline::Vec4::Vec4(real x,
                       real y,
                       real z,
                       real w)
{
	ts_vec4_init(m_vals, x, y, z, w);
}

tinyspline::Vec4
tinyspline::Vec4::operator+(const Vec4 &other)
{
	return add(other);
}

tinyspline::Vec4
tinyspline::Vec4::operator-(const Vec4 &other)
{
	return subtract(other);
}

tinyspline::Vec4
tinyspline::Vec4::operator*(real scalar)
{
	return multiply(scalar);
}

tinyspline::real
tinyspline::Vec4::x() const
{
	return m_vals[0];
}

void
tinyspline::Vec4::setX(real val)
{
	m_vals[0] = val;
}

tinyspline::real
tinyspline::Vec4::y() const
{
	return m_vals[1];
}

void
tinyspline::Vec4::setY(real val)
{
	m_vals[1] = val;
}

tinyspline::real
tinyspline::Vec4::z() const
{
	return m_vals[2];
}

void
tinyspline::Vec4::setZ(real val)
{
	m_vals[2] = val;
}

tinyspline::real
tinyspline::Vec4::w() const
{
	return m_vals[3];
}

void
tinyspline::Vec4::setW(real val)
{
	m_vals[3] = val;
}

std::vector<tinyspline::real>
tinyspline::Vec4::values() const
{
	return std::vector<real>({ x(), y(), z(), w() });
}

tinyspline::Vec4
tinyspline::Vec4::add(const Vec4 &other) const
{
	Vec4 vec;
	ts_vec_add(m_vals, other.m_vals, 4, vec.m_vals);
	return vec;
}

tinyspline::Vec4
tinyspline::Vec4::subtract(const Vec4 &other) const
{
	Vec4 vec;
	ts_vec_sub(m_vals, other.m_vals, 4, vec.m_vals);
	return vec;
}

tinyspline::Vec4
tinyspline::Vec4::multiply(real scalar) const
{
	Vec4 vec;
	ts_vec_mul(m_vals, 4, scalar, vec.m_vals);
	return vec;
}

tinyspline::Vec4
tinyspline::Vec4::normalize() const
{
	Vec4 vec;
	ts_vec_norm(m_vals, 4, vec.m_vals);
	return vec;
}

tinyspline::Vec4
tinyspline::Vec4::norm() const
{
	return normalize();
}

tinyspline::real
tinyspline::Vec4::magnitude() const
{
	return ts_vec_mag(m_vals, 4);
}

tinyspline::real
tinyspline::Vec4::dot(const Vec4 &other) const
{
	return ts_vec_dot(m_vals, other.m_vals, 4);
}

tinyspline::real
tinyspline::Vec4::angle(const Vec4 &other) const
{
	real buf[8];
	return ts_vec_angle(m_vals, other.m_vals, buf, 4);
}

tinyspline::real
tinyspline::Vec4::distance(const Vec4 &other) const
{
	return ts_distance(m_vals, other.m_vals, 4);
}

std::string
tinyspline::Vec4::toString() const
{
	std::ostringstream oss;
	oss << "Vec4{"
	    << "x: " << x()
	    << ", y: " << y()
	    << ", z: " << z()
	    << ", w: " << w()
	    << "}";
	return oss.str();
}
/*! @} */



/*! @name Frame
 *
 * @{
 */
tinyspline::Frame::Frame(Vec3 &position,
                         Vec3 &tangent,
                         Vec3 &normal,
                         Vec3 &binormal)
: m_position(position),
  m_tangent(tangent),
  m_normal(normal),
  m_binormal(binormal)
{}

tinyspline::Vec3
tinyspline::Frame::position() const
{
	return m_position;
}

tinyspline::Vec3
tinyspline::Frame::tangent() const
{
	return m_tangent;
}

tinyspline::Vec3
tinyspline::Frame::normal() const
{
	return m_normal;
}

tinyspline::Vec3
tinyspline::Frame::binormal() const
{
	return m_binormal;
}

std::string
tinyspline::Frame::toString() const
{
	std::ostringstream oss;
	oss << "Frame{"
	    << "position: " << position().toString()
	    << ", tangent: " << tangent().toString()
	    << ", normal: " << normal().toString()
	    << ", binormal: " << binormal().toString()
	    << "}";
	return oss.str();
}
/*! @} */



/*! @name FrameSeq
 *
 * @{
 */
tinyspline::FrameSeq::FrameSeq()
: m_frames(nullptr), m_size(0)
{}

tinyspline::FrameSeq::FrameSeq(tsFrame *frames,
                               size_t len)
: m_frames(frames), m_size(len)
{}

tinyspline::FrameSeq::FrameSeq(const FrameSeq &other)
: m_frames(nullptr), m_size(other.m_size)
{
	m_frames = new tsFrame[m_size];
	std::copy(other.m_frames,
	          other.m_frames + m_size,
	          m_frames);
}

tinyspline::FrameSeq::FrameSeq(FrameSeq &&other)
: m_frames(nullptr), m_size(other.m_size)
{
	m_frames = other.m_frames;
	other.m_frames = nullptr;
	other.m_size = 0;
}

tinyspline::FrameSeq::~FrameSeq()
{
	delete [] m_frames;
	m_size = 0;
}

tinyspline::FrameSeq &
tinyspline::FrameSeq::operator=(const FrameSeq &other)
{
	if (&other != this) {
		tsFrame *data = new tsFrame[other.m_size];
		std::copy(other.m_frames,
		          other.m_frames + other.m_size,
		          data);
		delete [] m_frames;
		m_frames = data;
		m_size = other.m_size;
	}
	return *this;
}

tinyspline::FrameSeq &
tinyspline::FrameSeq::operator=(FrameSeq &&other)
{
	if (&other != this) {
		delete [] m_frames;
		m_frames = other.m_frames;
		m_size = other.m_size;
		other.m_frames = nullptr;
		other.m_size = 0;
	}
	return *this;
}

size_t
tinyspline::FrameSeq::size() const
{
	return m_size;
}

tinyspline::Frame
tinyspline::FrameSeq::at(size_t idx) const
{
	if (idx >= m_size)
		throw std::out_of_range( "idx >= size");
	tsFrame frame = m_frames[idx];
	Vec3 position = Vec3(frame.position[0],
	                     frame.position[1],
	                     frame.position[2]);
	Vec3 tangent = Vec3(frame.tangent[0],
	                    frame.tangent[1],
	                    frame.tangent[2]);
	Vec3 normal = Vec3(frame.normal[0],
	                   frame.normal[1],
	                   frame.normal[2]);
	Vec3 binormal = Vec3(frame.binormal[0],
	                     frame.binormal[1],
	                     frame.binormal[2]);
	return Frame(position, tangent, normal, binormal);
}

std::string
tinyspline::FrameSeq::toString() const
{
	std::ostringstream oss;
	oss << "FrameSeq{"
	    << "frames: " << size()
	    << "}";
	return oss.str();
}
/*! @} */



/*! @name Domain
 *
 * @{
 */
tinyspline::Domain::Domain(real min,
                           real max)
: m_min(min), m_max(max)
{}

tinyspline::real
tinyspline::Domain::min() const
{
	return m_min;
}

tinyspline::real
tinyspline::Domain::max() const
{
	return m_max;
}

std::string
tinyspline::Domain::toString() const
{
	std::ostringstream oss;
        oss << "Domain{"
            << "min: " << min()
            << ", max: " << max()
            << "}";
	return oss.str();
}
/*! @} */



/*! @name DeBoorNet
 *
 * @{
 */
tinyspline::DeBoorNet::DeBoorNet(tsDeBoorNet &data)
: net(ts_deboornet_init())
{
	ts_deboornet_move(&data, &net);
}

tinyspline::DeBoorNet::DeBoorNet(const DeBoorNet &other)
: net(ts_deboornet_init())
{
	tsStatus status;
	if (ts_deboornet_copy(&other.net, &net, &status))
		throw std::runtime_error(status.message);
}

tinyspline::DeBoorNet::DeBoorNet(DeBoorNet &&other)
: net(ts_deboornet_init())
{
	ts_deboornet_move(&other.net, &net);
}

tinyspline::DeBoorNet::~DeBoorNet()
{
	ts_deboornet_free(&net);
}

tinyspline::DeBoorNet &
tinyspline::DeBoorNet::operator=(const DeBoorNet &other)
{
	if (&other != this) {
		tsDeBoorNet data = ts_deboornet_init();
		tsStatus status;
		if (ts_deboornet_copy(&other.net, &data, &status))
			throw std::runtime_error(status.message);
		ts_deboornet_free(&net);
		ts_deboornet_move(&data, &net);
	}
	return *this;
}

tinyspline::DeBoorNet &
tinyspline::DeBoorNet::operator=(DeBoorNet && other)
{
	if (&other != this) {
		ts_deboornet_free(&net);
		ts_deboornet_move(&other.net, &net);
	}
	return *this;
}

tinyspline::real
tinyspline::DeBoorNet::knot() const
{
	return ts_deboornet_knot(&net);
}

size_t
tinyspline::DeBoorNet::index() const
{
	return ts_deboornet_index(&net);
}

size_t
tinyspline::DeBoorNet::multiplicity() const
{
	return ts_deboornet_multiplicity(&net);
}

size_t
tinyspline::DeBoorNet::numInsertions() const
{
	return ts_deboornet_num_insertions(&net);
}

size_t
tinyspline::DeBoorNet::dimension() const
{
	return ts_deboornet_dimension(&net);
}

std::vector<tinyspline::real>
tinyspline::DeBoorNet::points() const
{
	const real *points = ts_deboornet_points_ptr(&net);
	size_t len = ts_deboornet_len_points(&net);
	return std::vector<real>(points, points + len);
}

std::vector<tinyspline::real>
tinyspline::DeBoorNet::result() const
{
	const real *result = ts_deboornet_result_ptr(&net);
	size_t len = ts_deboornet_len_result(&net);
	return std::vector<real>(result, result + len);
}

tinyspline::Vec2
tinyspline::DeBoorNet::resultVec2(size_t idx) const
{
	Vec4 vec4 = resultVec4(idx);
	return Vec2(vec4.x(), vec4.y());
}

tinyspline::Vec3
tinyspline::DeBoorNet::resultVec3(size_t idx) const
{
	Vec4 vec4 = resultVec4(idx);
	return Vec3(vec4.x(), vec4.y(), vec4.z());
}

tinyspline::Vec4
tinyspline::DeBoorNet::resultVec4(size_t idx) const
{
	if (idx >= ts_deboornet_num_result(&net))
		throw std::out_of_range( "idx >= num(result)");
	const real *result = ts_deboornet_result_ptr(&net);
	real vals[4];
	ts_vec4_set(vals, result + idx * dimension(), dimension());
	return Vec4(vals[0], vals[1], vals[2], vals[3]);
}

std::string
tinyspline::DeBoorNet::toString() const
{
	std::ostringstream oss;
	oss << "DeBoorNet{"
	    << "knot: " << knot()
	    << ", index: " << index()
	    << ", multiplicity: " << multiplicity()
	    << ", insertions: " << numInsertions()
	    << ", dimension: " << dimension()
	    << ", points: " << ts_deboornet_num_points(&net)
	    << "}";
	return oss.str();
}
/*! @} */



/******************************************************************************
*                                                                             *
* BSpline                                                                     *
*                                                                             *
******************************************************************************/
tinyspline::BSpline::BSpline(tsBSpline &data)
: spline(ts_bspline_init())
{
	ts_bspline_move(&data, &spline);
}

tinyspline::BSpline::BSpline()
: spline(ts_bspline_init())
{
	tsStatus status;
	if (ts_bspline_new_with_control_points(1,
	                                       3,
	                                       0,
	                                       TS_CLAMPED,
	                                       &spline,
	                                       &status,
	                                       0.0, 0.0, 0.0))
		throw std::runtime_error(status.message);
}

tinyspline::BSpline::BSpline(const tinyspline::BSpline &other)
: spline(ts_bspline_init())
{
	tsStatus status;
	if (ts_bspline_copy(&other.spline, &spline, &status))
		throw std::runtime_error(status.message);
}

tinyspline::BSpline::BSpline(BSpline &&other)
: spline(ts_bspline_init())
{
	ts_bspline_move(&other.spline, &spline);
}

tinyspline::BSpline::BSpline(size_t numControlPoints,
                             size_t dimension,
                             size_t degree,
                             Type type)
: spline(ts_bspline_init())
{
	tsBSplineType c_type = TS_CLAMPED;
	switch (type) {
	case Opened:
		c_type = TS_OPENED;
		break;
	case Clamped:
		c_type = TS_CLAMPED;
		break;
	case Beziers:
		c_type = TS_BEZIERS;
		break;
	default:
		throw std::runtime_error("unknown type");
	}
	tsStatus status;
	if (ts_bspline_new(numControlPoints,
	                   dimension,
	                   degree,
	                   c_type,
	                   &spline,
	                   &status))
		throw std::runtime_error(status.message);
}

tinyspline::BSpline::~BSpline()
{
	ts_bspline_free(&spline);
}

tinyspline::BSpline tinyspline::BSpline::interpolateCubicNatural(
	std_real_vector_in points, size_t dimension)
{
	if (dimension == 0)
		throw std::runtime_error("unsupported dimension: 0");
	if (std_real_vector_read(points)size() % dimension != 0)
		throw std::runtime_error("#points % dimension != 0");
	tsBSpline data = ts_bspline_init();
	tsStatus status;
	if (ts_bspline_interpolate_cubic_natural(
			std_real_vector_read(points)data(),
			std_real_vector_read(points)size()/dimension,
			dimension, &data, &status))
		throw std::runtime_error(status.message);
	return BSpline(data);
}

tinyspline::BSpline
tinyspline::BSpline::interpolateCatmullRom(std_real_vector_in points,
                                           size_t dimension,
                                           real alpha,
                                           std::vector<real> *first,
                                           std::vector<real> *last,
                                           real epsilon)
{
	if (dimension == 0)
		throw std::runtime_error("unsupported dimension: 0");
	if (std_real_vector_read(points)size() % dimension != 0)
		throw std::runtime_error("#points % dimension != 0");
	real *fst = nullptr;
	if (first && first->size() >= dimension)
		fst = first->data();
	real *lst = nullptr;
	if (last && last->size() >= dimension)
		lst = last->data();
	tsBSpline data = ts_bspline_init();
	tsStatus status;
	if (ts_bspline_interpolate_catmull_rom(
			std_real_vector_read(points)data(),
			std_real_vector_read(points)size()/dimension,
			dimension, alpha, fst, lst, epsilon, &data,
			&status))
		throw std::runtime_error(status.message);
	return BSpline(data);
}

tinyspline::BSpline tinyspline::BSpline::parseJson(std::string json)
{
	tsBSpline data = ts_bspline_init();
	tsStatus status;
	if (ts_bspline_parse_json(json.c_str(), &data, &status))
		throw std::runtime_error(status.message);
	return BSpline(data);
}

tinyspline::BSpline tinyspline::BSpline::load(std::string path)
{
	tsBSpline data = ts_bspline_init();
	tsStatus status;
	if (ts_bspline_load(path.c_str(), &data, &status))
		throw std::runtime_error(status.message);
	return BSpline(data);
}

bool
tinyspline::BSpline::knotsEqual(real x, real y)
{
	return ts_knots_equal(x, y);
}

tinyspline::BSpline &
tinyspline::BSpline::operator=(const BSpline &other)
{
	if (&other != this) {
		tsBSpline data = ts_bspline_init();
		tsStatus status;
		if (ts_bspline_copy(&other.spline, &data, &status))
			throw std::runtime_error(status.message);
		ts_bspline_free(&spline);
		ts_bspline_move(&data, &spline);
	}
	return *this;
}

tinyspline::BSpline &
tinyspline::BSpline::operator=(BSpline &&other)
{
	if (&other != this) {
		ts_bspline_free(&spline);
		ts_bspline_move(&other.spline, &spline);
	}
	return *this;
}

tinyspline::DeBoorNet tinyspline::BSpline::operator()(tinyspline::real u) const
{
	return eval(u);
}

size_t tinyspline::BSpline::degree() const
{
	return ts_bspline_degree(&spline);
}

size_t tinyspline::BSpline::order() const
{
	return ts_bspline_order(&spline);
}

size_t tinyspline::BSpline::dimension() const
{
	return ts_bspline_dimension(&spline);
}

std::vector<tinyspline::real>
tinyspline::BSpline::controlPoints() const
{
	const real *ctrlps = ts_bspline_control_points_ptr(&spline);
	const size_t len = ts_bspline_len_control_points(&spline);
	return std::vector<real>(ctrlps, ctrlps + len);
}

tinyspline::Vec2
tinyspline::BSpline::controlPointVec2At(size_t idx) const
{
	const Vec4 vec4 = controlPointVec4At(idx);
	return Vec2(vec4.x(), vec4.y());
}

tinyspline::Vec3
tinyspline::BSpline::controlPointVec3At(size_t idx) const
{
	const Vec4 vec4 = controlPointVec4At(idx);
	return Vec3(vec4.x(), vec4.y(), vec4.z());
}

tinyspline::Vec4
tinyspline::BSpline::controlPointVec4At(size_t idx) const
{
	const real *ctrlp;
	tsStatus status;
	if (ts_bspline_control_point_at_ptr(&spline,
	                                    idx,
	                                    &ctrlp,
	                                    &status))
		throw std::runtime_error(status.message);
	real vals[4];
	ts_vec4_set(vals, ctrlp, dimension());
	return Vec4(vals[0], vals[1], vals[2], vals[3]);
}

std::vector<tinyspline::real>
tinyspline::BSpline::knots() const
{
	const real *knots = ts_bspline_knots_ptr(&spline);
	size_t num = ts_bspline_num_knots(&spline);
	return std::vector<real>(knots, knots + num);
}

tinyspline::real tinyspline::BSpline::knotAt(size_t index) const
{
	real knot;
	tsStatus status;
	if (ts_bspline_knot_at(&spline, index, &knot, &status))
		throw std::runtime_error(status.message);
	return knot;
}

size_t tinyspline::BSpline::numControlPoints() const
{
	return ts_bspline_num_control_points(&spline);
}

tinyspline::DeBoorNet tinyspline::BSpline::eval(tinyspline::real u) const
{
	tsDeBoorNet net = ts_deboornet_init();
	tsStatus status;
	if (ts_bspline_eval(&spline, u, &net, &status))
		throw std::runtime_error(status.message);
	return tinyspline::DeBoorNet(net);
}

tinyspline::std_real_vector_out
tinyspline::BSpline::evalAll(std_real_vector_in knots) const
{
	const size_t num_knots = std_real_vector_read(knots)size();
	const real *knots_ptr = std_real_vector_read(knots)data();
	tinyspline::real *points;
	tsStatus status;
	if (ts_bspline_eval_all(&spline,
	                        knots_ptr,
	                        num_knots,
	                        &points,
	                        &status)) {
		throw std::runtime_error(status.message);
	}
	real *first = points;
	real *last = first + num_knots * dimension();
	std_real_vector_init(vec)(first, last);
	std::free(points);
	return vec;
}

tinyspline::std_real_vector_out
tinyspline::BSpline::sample(size_t num) const
{
	tinyspline::real *points;
	size_t actualNum;
	tsStatus status;
	if (ts_bspline_sample(&spline,
	                      num,
	                      &points,
	                      &actualNum,
	                      &status)) {
		throw std::runtime_error(status.message);
	}
	real *first = points;
	real *last = first + actualNum * dimension();
	std_real_vector_init(vec)(first, last);
	std::free(points);
	return vec;
}

tinyspline::DeBoorNet tinyspline::BSpline::bisect(real value,
                                                  real epsilon,
                                                  bool persnickety,
                                                  size_t index,
                                                  bool ascending,
                                                  size_t maxIter) const
{
	tsDeBoorNet net = ts_deboornet_init();
	tsStatus status;
	if (ts_bspline_bisect(&spline,
	                      value,
	                      epsilon,
	                      persnickety,
	                      index,
	                      ascending,
	                      maxIter,
	                      &net,
	                      &status))
		throw std::runtime_error(status.message);
	return DeBoorNet(net);
}

tinyspline::Domain tinyspline::BSpline::domain() const
{
	real min, max;
	ts_bspline_domain(&spline, &min, &max);
	return Domain(min, max);
}

bool tinyspline::BSpline::isClosed(tinyspline::real epsilon) const
{
	int closed = 0;
	tsStatus status;
	if (ts_bspline_is_closed(&spline, epsilon, &closed, &status))
		throw std::runtime_error(status.message);
	return closed == 1;
}

tinyspline::FrameSeq
tinyspline::BSpline::computeRMF(std_real_vector_in knots,
                                tinyspline::Vec3 *firstNormal) const
{
	tsStatus status;
	size_t num = std_real_vector_read(knots)size();
	const real *knots_ptr = std_real_vector_read(knots)data();
	tsFrame *frames = new tsFrame[num];
	if (firstNormal && num > 0) {
		ts_vec3_init(frames[0].normal,
		             firstNormal->x(),
		             firstNormal->y(),
		             firstNormal->z());
	}
	if (ts_bspline_compute_rmf(&spline,
	                           knots_ptr,
	                           num,
	                           firstNormal != nullptr,
	                           frames,
	                           &status))
		throw std::runtime_error(status.message);
	FrameSeq seq = FrameSeq(frames, num);
	return seq;
}


tinyspline::BSpline
tinyspline::BSpline::subSpline(real knot0, real knot1) const
{
	tsBSpline data = ts_bspline_init();
	tsStatus status;
	if (ts_bspline_sub_spline(&spline,
	                          knot0,
	                          knot1,
	                          &data,
	                          &status))
		throw std::runtime_error(status.message);
	return BSpline(data);
}

tinyspline::std_real_vector_out
tinyspline::BSpline::uniformKnotSeq(size_t num) const
{
	std_real_vector_init(knots)(num);
	real *knots_ptr = std_real_vector_read(knots)data();
	ts_bspline_uniform_knot_seq(&spline, num, knots_ptr);
	return knots;
}

tinyspline::std_real_vector_out
tinyspline::BSpline::equidistantKnotSeq(size_t num,
                                        size_t numSamples) const
{
	tsStatus status;
	std_real_vector_init(knots)(num);
	real *knots_ptr = std_real_vector_read(knots)data();
	if (ts_bspline_equidistant_knot_seq(&spline,
	                                    num,
	                                    knots_ptr,
	                                    numSamples,
	                                    &status))
		throw std::runtime_error(status.message);
	return knots;
}

tinyspline::ChordLengths
tinyspline::BSpline::chordLengths(std_real_vector_in knots) const
{
	tsStatus status;
	size_t num = std_real_vector_read(knots)size();
	real *knotsArr = new real[num];
	real *lengths = new real[num];
	std::copy(std_real_vector_read(knots)begin(),
	          std_real_vector_read(knots)end(),
	          knotsArr);
	if (ts_bspline_chord_lengths(&spline,
	                             knotsArr,
	                             num,
	                             lengths,
	                             &status))
		throw std::runtime_error(status.message);
	return ChordLengths(*this, knotsArr, lengths, num);
}

tinyspline::ChordLengths
tinyspline::BSpline::chordLengths(size_t numSamples) const
{
	return chordLengths(uniformKnotSeq(numSamples));
}

std::string tinyspline::BSpline::toJson() const
{
	char *json;
	tsStatus status;
	if (ts_bspline_to_json(&spline, &json, &status))
		throw std::runtime_error(status.message);
	std::string string(json);
	std::free(json);
	return string;
}

void tinyspline::BSpline::save(std::string path) const
{
	tsStatus status;
	if (ts_bspline_save(&spline, path.c_str(), &status))
		throw std::runtime_error(status.message);
}

void tinyspline::BSpline::setControlPoints(
	const std::vector<tinyspline::real> &ctrlp)
{
	size_t expected = ts_bspline_len_control_points(&spline);
	size_t actual = ctrlp.size();
	if (expected != actual) {
		std::ostringstream oss;
		oss << "Expected size: " << expected
		    << ", Actual size: " << actual;
		throw std::runtime_error(oss.str());
	}
	tsStatus status;
	if (ts_bspline_set_control_points(&spline, ctrlp.data(), &status))
		throw std::runtime_error(status.message);
}

void
tinyspline::BSpline::setControlPointVec2At(size_t idx, Vec2 &cp)
{
	Vec4 vec4(cp.x(), cp.y(), (real) 0.0, (real) 0.0);
	setControlPointVec4At(idx, vec4);
}

void
tinyspline::BSpline::setControlPointVec3At(size_t idx, Vec3 &cp)
{
	Vec4 vec4(cp.x(), cp.y(), cp.z(), (real) 0.0);
	setControlPointVec4At(idx, vec4);
}

void
tinyspline::BSpline::setControlPointVec4At(size_t idx, Vec4 &cp)
{
	std::vector<real> vals(dimension());
	for (size_t i = 0; i < vals.size(); i++)
		vals[i] = (real) 0.0;
	if (vals.size() >= 4) vals[3] = cp.w();
	if (vals.size() >= 3) vals[2] = cp.z();
	if (vals.size() >= 2) vals[1] = cp.y();
	if (vals.size() >= 1) vals[0] = cp.x();
	tsStatus status;
	if (ts_bspline_set_control_point_at(&spline,
	                                    idx,
	                                    vals.data(),
	                                    &status))
		throw std::runtime_error(status.message);
}

void
tinyspline::BSpline::setKnots(const std::vector<real> &knots)
{
	size_t expected = ts_bspline_num_knots(&spline);
	size_t actual = knots.size();
	if (expected != actual) {
		std::ostringstream oss;
		oss << "Expected size: " << expected
		    << ", Actual size: " << actual;
		throw std::runtime_error(oss.str());
	}
	tsStatus status;
	if (ts_bspline_set_knots(&spline,
	                         knots.data(),
	                         &status))
		throw std::runtime_error(status.message);
}

void tinyspline::BSpline::setKnotAt(size_t index, tinyspline::real knot)
{
	tsStatus status;
	if (ts_bspline_set_knot_at(&spline, index, knot, &status))
		throw std::runtime_error(status.message);
}

tinyspline::BSpline tinyspline::BSpline::insertKnot(tinyspline::real u,
	size_t n) const
{
	tsBSpline data = ts_bspline_init();
	size_t k;
	tsStatus status;
	if (ts_bspline_insert_knot(&spline, u, n, &data, &k, &status))
		throw std::runtime_error(status.message);
	return BSpline(data);
}

tinyspline::BSpline tinyspline::BSpline::split(tinyspline::real u) const
{
	tsBSpline data = ts_bspline_init();
	size_t k;
	tsStatus status;
	if (ts_bspline_split(&spline, u, &data, &k, &status))
		throw std::runtime_error(status.message);
	return BSpline(data);
}

tinyspline::BSpline tinyspline::BSpline::tension(
	tinyspline::real tension) const
{
	tsBSpline data = ts_bspline_init();
	tsStatus status;
	if (ts_bspline_tension(&spline, tension, &data, &status))
		throw std::runtime_error(status.message);
	return BSpline(data);
}

tinyspline::BSpline tinyspline::BSpline::toBeziers() const
{
	tsBSpline data = ts_bspline_init();
	tsStatus status;
	if (ts_bspline_to_beziers(&spline, &data, &status))
		throw std::runtime_error(status.message);
	return BSpline(data);
}

tinyspline::BSpline tinyspline::BSpline::derive(size_t n, real epsilon) const
{
	tsBSpline data = ts_bspline_init();
	tsStatus status;
	if (ts_bspline_derive(&spline, n, epsilon, &data, &status))
		throw std::runtime_error(status.message);
	return BSpline(data);
}

tinyspline::BSpline tinyspline::BSpline::elevateDegree(
	size_t amount, real epsilon) const
{
	tsBSpline data = ts_bspline_init();
	tsStatus status;
	if (ts_bspline_elevate_degree(&spline, amount, epsilon,
		&data, &status)) {
		throw std::runtime_error(status.message);
	}
	return BSpline(data);
}

tinyspline::BSpline tinyspline::BSpline::alignWith(
	const BSpline &other, BSpline &otherAligned, real epsilon) const
{
	tsBSpline data = ts_bspline_init();
	tsBSpline deleteIf_Other_And_OtherAligned_AreDifferent =
		otherAligned.spline;
	tsStatus status;
	if (ts_bspline_align(&spline, &other.spline, epsilon, &data,
		&otherAligned.spline, &status)) {
		throw std::runtime_error(status.message);
	}
	if (&other != &otherAligned)
		ts_bspline_free(&deleteIf_Other_And_OtherAligned_AreDifferent);
	return BSpline(data);
}

tinyspline::Morphism tinyspline::BSpline::morphTo(
	const BSpline &other, real epsilon) const
{
	return Morphism(*this, other, epsilon);
}

std::string tinyspline::BSpline::toString() const
{
	Domain d = domain();
	std::ostringstream oss;
	oss << "BSpline{"
	    << "dimension: " << dimension()
	    << ", degree: " << degree()
	    << ", domain: [" << d.min() << ", " << d.max() << "]"
	    << ", control points: " << numControlPoints()
	    << ", knots: " << ts_bspline_num_knots(&spline)
	    << "}";
	return oss.str();
}



/*! @name Morphism
 *
 * @{
 */
tinyspline::Morphism::Morphism(const BSpline &origin,
			       const BSpline &target,
			       real epsilon)
: m_origin(origin), m_target(target), m_epsilon(epsilon)
{
	m_originAligned = origin.alignWith(target, m_targetAligned, epsilon);
	// Make buffer compatible by copying one of the aligned splines.
	m_buffer = m_originAligned;
}

tinyspline::BSpline
tinyspline::Morphism::eval(real t)
{
	tsStatus status;
	if (t <= 0) return m_origin;
	if (t >= 1) return m_target;
	if (ts_bspline_morph(&m_originAligned.spline,
			     &m_targetAligned.spline,
			     t, m_epsilon,
			     &m_buffer.spline, &status)) {
		throw std::runtime_error(status.message);
	}
	return m_buffer;
}

tinyspline::BSpline
tinyspline::Morphism::origin() const
{
	return m_origin;
}

tinyspline::BSpline
tinyspline::Morphism::target() const
{
	return m_target;
}

tinyspline::real
tinyspline::Morphism::epsilon() const
{
	return m_epsilon;
}

tinyspline::BSpline
tinyspline::Morphism::operator()(real t)
{
	return eval(t);
}

std::string tinyspline::Morphism::toString() const
{
	std::ostringstream oss;
	oss << "Morphism{"
	    << "buffer: " << m_buffer.toString()
	    << ", epsilon: " << epsilon()
	    << "}";
	return oss.str();
}
/*! @} */



/*! @name ChordLenghts
 * @{
 */
tinyspline::ChordLengths::ChordLengths()
: m_spline(),
  m_knots(nullptr),
  m_chordLengths(nullptr),
  m_size(0)
{}

tinyspline::ChordLengths::ChordLengths(const BSpline &spline,
                                       real *knots,
                                       real *chordLengths,
                                       size_t size)
: m_spline(spline),
  m_knots(knots),
  m_chordLengths(chordLengths),
  m_size(size)
{}

tinyspline::ChordLengths::ChordLengths(const ChordLengths &other)
: m_spline(other.m_spline),
  m_knots(nullptr),
  m_chordLengths(nullptr),
  m_size(other.m_size)
{
	m_knots = new real[m_size];
	std::copy(other.m_knots,
	          other.m_knots + m_size,
	          m_knots);
	m_chordLengths = new real[m_size];
	std::copy(other.m_chordLengths,
	          other.m_chordLengths + m_size,
	          m_chordLengths);
}

tinyspline::ChordLengths::ChordLengths(ChordLengths &&other)
: m_spline(),
  m_knots(nullptr),
  m_chordLengths(nullptr),
  m_size(0)
{
	*this = std::move(other);
}

tinyspline::ChordLengths::~ChordLengths()
{
	delete [] m_knots;
	delete [] m_chordLengths;
	m_size = 0;
}

tinyspline::ChordLengths &
tinyspline::ChordLengths::operator=(const ChordLengths &other)
{
	if (&other != this) {
		real *knots = new real[other.m_size];
		std::copy(other.m_knots,
		          other.m_knots + other.m_size,
		          knots);
		real *chordLengths = new real[other.m_size];
		std::copy(other.m_chordLengths,
		          other.m_chordLengths + other.m_size,
		          chordLengths);
		delete [] m_knots;
		delete [] m_chordLengths;
		m_spline = other.m_spline;
		m_knots = knots;
		m_chordLengths = chordLengths;
		m_size = other.m_size;
	}
	return *this;
}

tinyspline::ChordLengths &
tinyspline::ChordLengths::operator=(ChordLengths &&other)
{
	if (&other != this) {
		delete [] m_knots;
		delete [] m_chordLengths;
		m_spline = other.m_spline;
		m_knots = other.m_knots;
		m_chordLengths = other.m_chordLengths;
		m_size = other.m_size;
		other.m_spline = BSpline();
		other.m_knots = nullptr;
		other.m_chordLengths = nullptr;
		other.m_size = 0;
	}
	return *this;
}

tinyspline::BSpline
tinyspline::ChordLengths::spline() const
{
	return m_spline;
}

std::vector<tinyspline::real>
tinyspline::ChordLengths::knots() const
{
	return std::vector<real>(m_knots,
	                         m_knots + m_size);
}

std::vector<tinyspline::real>
tinyspline::ChordLengths::values() const
{
	return std::vector<real>(m_chordLengths,
	                         m_chordLengths + m_size);
}

size_t
tinyspline::ChordLengths::size() const
{
	return m_size;
}

tinyspline::real
tinyspline::ChordLengths::arcLength() const
{
	return m_size == 0 ? 0 : m_chordLengths[m_size - 1];
}

tinyspline::real
tinyspline::ChordLengths::lengthToKnot(real len) const
{
	tsStatus status;
	real knot;
	if (ts_chord_lengths_length_to_knot(m_knots,
	                               m_chordLengths,
	                               m_size,
	                               len,
	                               &knot,
	                               &status))
		throw std::runtime_error(status.message);
	return knot;
}

tinyspline::real
tinyspline::ChordLengths::tToKnot(real t) const
{
	tsStatus status;
	real knot;
	if (ts_chord_lengths_t_to_knot(m_knots,
	                               m_chordLengths,
	                               m_size,
	                               t,
	                               &knot,
	                               &status))
		throw std::runtime_error(status.message);
	return knot;
}

std::string
tinyspline::ChordLengths::toString() const
{
	std::ostringstream oss;
	oss << "ChordLengths{"
	    << "spline: " << m_spline.toString()
	    << ", values: " << m_size
	    << "}";
	return oss.str();
}
/*! @} */

#ifdef _MSC_VER
#pragma warning(pop)
#endif
