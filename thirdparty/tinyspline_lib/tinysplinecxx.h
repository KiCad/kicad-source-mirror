/** @file */

#pragma once

#include "tinyspline.h"
#include <vector>
#include <string>

#ifndef TINYSPLINECXX_API
#define TINYSPLINECXX_API TINYSPLINE_API
#endif



/*! @name Emscripten Extensions
 *
 * Please see the following references for more details on how to use
 * Emscripten with C++:
 *
 * https://emscripten.org/docs/porting/connecting_cpp_and_javascript/embind.html
 * https://emscripten.org/docs/api_reference/bind.h.html
 *
 * @{
 */
#ifdef TINYSPLINE_EMSCRIPTEN

// Additional includes and namespaces.
#include <stdexcept>
#include <emscripten/bind.h>
using namespace emscripten;

/* Used by value_objects to guard read-only properties. */
void inline
cannotWrite()
{ throw std::runtime_error("cannot write read-only property"); }

/* Allow clients to read exception messages in the Javascript binding. */
std::string
exceptionMessage(int ptr)
{ return std::string(reinterpret_cast<std::exception *>(ptr)->what()); }

// Map: std::vector <--> JS array
// https://github.com/emscripten-core/emscripten/issues/11070#issuecomment-717675128
namespace emscripten {
namespace internal {
template <typename T, typename Allocator>
struct BindingType<std::vector<T, Allocator>> {
	using ValBinding = BindingType<val>;
	using WireType = ValBinding::WireType;

	static WireType
	toWireType(const std::vector<T, Allocator> &vec)
	{ return ValBinding::toWireType(val::array(vec)); }

	static std::vector<T, Allocator>
	fromWireType(WireType value)
	{ return vecFromJSArray<T>(ValBinding::fromWireType(value)); }
};

template <typename T>
struct TypeID<T, // this is ridiculous...
              typename std::enable_if_t<std::is_same<
                  typename Canonicalized<T>::type,
                  std::vector<typename Canonicalized<T>::type::value_type,
                  typename Canonicalized<T>::type::allocator_type>>::value>> {
	static constexpr TYPEID
	get() { return TypeID<val>::get(); }
};
}  // namespace internal
}  // namespace emscripten
#endif
/*! @} */



namespace tinyspline {

/*! @name API Configuration
 *
 * @{
 */
typedef tsReal real;
/*! @} */



/*! @name Swig Type Mapping
 *
 * Methods that do not return or set the value of a class attribute (let's call
 * these methods `non-accessor methods') must return/take instances of
 * std::vector as pointer. Otherwise, they aren't type mapped by Swig to the
 * std::vector representation of the target language.
 *
 * @{
 */
#ifdef SWIG
using std_real_vector_in = std::vector<tinyspline::real> *;
using std_real_vector_out = std::vector<tinyspline::real> *;
#else
using std_real_vector_in = const std::vector<tinyspline::real> &;
using std_real_vector_out = std::vector<tinyspline::real>;
#endif
/*! @} */



/*! @name Vector Math
 *
 * Wrapper classes for TinySpline's vector math.
 *
 * @{
 */
class TINYSPLINECXX_API Vec2 {
public:
	Vec2();
	Vec2(real x, real y);

	Vec2 operator+(const Vec2 &other);
	Vec2 operator-(const Vec2 &other);
	Vec2 operator*(real scalar);

	real x() const;
	void setX(real val);
	real y() const;
	void setY(real val);
	std::vector<real> values() const;

	Vec2 add(const Vec2 &other) const;
	Vec2 subtract(const Vec2 &other) const;
	Vec2 multiply(real scalar) const;
	Vec2 normalize() const;
	TS_DEPRECATED Vec2 norm() const;
	real magnitude() const;
	real dot(const Vec2 &other) const;
	real angle(const Vec2 &other) const;
	real distance(const Vec2 &other) const;

	std::string toString() const;

private:
	real m_vals[2];

#ifdef TINYSPLINE_EMSCRIPTEN
public:
	void setValues(std::vector<real>) { cannotWrite(); }
#endif
};

class TINYSPLINECXX_API Vec3 {
public:
	Vec3();
	Vec3(real x, real y, real z);

	Vec3 operator+(const Vec3 &other);
	Vec3 operator-(const Vec3 &other);
	Vec3 operator*(real scalar);

	real x() const;
	void setX(real val);
	real y() const;
	void setY(real val);
	real z() const;
	void setZ(real val);
	std::vector<real> values() const;

	Vec3 add(const Vec3 &other) const;
	Vec3 subtract(const Vec3 &other) const;
	Vec3 multiply(real scalar) const;
	Vec3 cross(const Vec3 &other) const;
	Vec3 normalize() const;
	TS_DEPRECATED Vec3 norm() const;
	real magnitude() const;
	real dot(const Vec3 &other) const;
	real angle(const Vec3 &other) const;
	real distance(const Vec3 &other) const;

	std::string toString() const;

private:
	real m_vals[3];

#ifdef TINYSPLINE_EMSCRIPTEN
public:
	void setValues(std::vector<real>) { cannotWrite(); }
#endif
};

class TINYSPLINECXX_API Vec4 {
public:
	Vec4();
	Vec4(real x, real y, real z, real w);

	Vec4 operator+(const Vec4 &other);
	Vec4 operator-(const Vec4 &other);
	Vec4 operator*(real scalar);

	real x() const;
	void setX(real val);
	real y() const;
	void setY(real val);
	real z() const;
	void setZ(real val);
	real w() const;
	void setW(real val);
	std::vector<real> values() const;

	Vec4 add(const Vec4 &other) const;
	Vec4 subtract(const Vec4 &other) const;
	Vec4 multiply(real scalar) const;
	Vec4 normalize() const;
	TS_DEPRECATED Vec4 norm() const;
	real magnitude() const;
	real dot(const Vec4 &other) const;
	real angle(const Vec4 &other) const;
	real distance(const Vec4 &other) const;

	std::string toString() const;

private:
	real m_vals[4];

#ifdef TINYSPLINE_EMSCRIPTEN
public:
	void setValues(std::vector<real>) { cannotWrite(); }
#endif
};

#ifdef TINYSPLINE_EMSCRIPTEN
class VecMath {
public:
	/*! @name Add
	*
	* @{
	*/
	static Vec2
	add2(const Vec2 &a, const Vec2 &b)
	{ return a.add(b); }

	static Vec3
	add3(const Vec3 &a, const Vec3 &b)
	{ return a.add(b); }

	static Vec4
	add4(const Vec4 &a, const Vec4 &b)
	{ return a.add(b); }
	/*! @} */

	/*! @name Subtract
	*
	* @{
	*/
	static Vec2
	subtract2(const Vec2 &a, const Vec2 &b)
	{ return a.subtract(b); }

	static Vec3
	subtract3(const Vec3 &a, const Vec3 &b)
	{ return a.subtract(b); }

	static Vec4
	subtract4(const Vec4 &a, const Vec4 &b)
	{ return a.subtract(b); }
	/*! @} */

	/*! @name Multiply
	*
	* @{
	*/
	static Vec2
	multiply2(const Vec2 &vec, real scalar)
	{ return vec.multiply(scalar); }

	static Vec3
	multiply3(const Vec3 &vec, real scalar)
	{ return vec.multiply(scalar); }

	static Vec4
	multiply4(const Vec4 &vec, real scalar)
	{ return vec.multiply(scalar); }
	/*! @} */

	/*! @name Cross
	*
	* @{
	*/
	static Vec3
	cross3(const Vec3 &a, Vec3 &b)
	{ return a.cross(b); }
	/*! @} */

	/*! @name Normalize
	*
	* @{
	*/
	static Vec2
	normalize2(const Vec2 &vec)
	{ return vec.normalize(); }

	static Vec3
	normalize3(const Vec3 &vec)
	{ return vec.normalize(); }

	static Vec4
	normalize4(const Vec4 &vec)
	{ return vec.normalize(); }
	/*! @} */

	/*! @name Magnitude
	*
	* @{
	*/
	static real
	magnitude2(const Vec2 &vec)
	{ return vec.magnitude(); }

	static real
	magnitude3(const Vec3 &vec)
	{ return vec.magnitude(); }

	static real
	magnitude4(const Vec4 &vec)
	{ return vec.magnitude(); }
	/*! @} */

	/*! @name Dot
	*
	* @{
	*/
	static real
	dot2(const Vec2 &a, Vec2 &b)
	{ return a.dot(b); }

	static real
	dot3(const Vec3 &a, Vec3 &b)
	{ return a.dot(b); }

	static real
	dot4(const Vec4 &a, Vec4 &b)
	{ return a.dot(b); }
	/*! @} */

	/*! @name Angle
	*
	* @{
	*/
	static real
	angle2(const Vec2 &a, Vec2 &b)
	{ return a.angle(b); }

	static real
	angle3(const Vec3 &a, Vec3 &b)
	{ return a.angle(b); }

	static real
	angle4(const Vec4 &a, Vec4 &b)
	{ return a.angle(b); }
	/*! @} */

	/*! @name Distance
	*
	* @{
	*/
	static real
	distance2(const Vec2 &a, Vec2 &b)
	{ return a.distance(b); }

	static real
	distance3(const Vec3 &a, Vec3 &b)
	{ return a.distance(b); }

	static real
	distance4(const Vec4 &a, Vec4 &b)
	{ return a.distance(b); }
	/*! @} */
};
#endif
/*! @} */



/*! @name Spline Framing
 *
 * Object-oriented representation of ::tsFrame (::Frame) and sequences of
 * ::tsFrame (::FrameSeq). Instances of ::FrameSeq are created by
 * ::BSpline::computeRMF.
 *
 * @{
 */
class TINYSPLINECXX_API Frame {
public:
	Frame(Vec3 &position,
	      Vec3 &tangent,
	      Vec3 &normal,
	      Vec3 &binormal);

	Vec3 position() const;
	Vec3 tangent() const;
	Vec3 normal() const;
	Vec3 binormal() const;

	std::string toString() const;

private:
	Vec3 m_position,
	     m_tangent,
	     m_normal,
	     m_binormal;

#ifdef TINYSPLINE_EMSCRIPTEN
public:
	Frame() {}
	void setPosition(Vec3) { cannotWrite(); }
	void setTangent(Vec3) { cannotWrite(); }
	void setNormal(Vec3) { cannotWrite(); }
	void setBinormal(Vec3) { cannotWrite(); }
#endif
};

class TINYSPLINECXX_API FrameSeq {
public:
	FrameSeq();
	FrameSeq(const FrameSeq &other);
	FrameSeq(FrameSeq &&other);
	virtual ~FrameSeq();

	FrameSeq &operator=(const FrameSeq &other);
	FrameSeq &operator=(FrameSeq &&other);

	size_t size() const;
	Frame at(size_t idx) const;

	std::string toString() const;

private:
	tsFrame *m_frames;
	size_t m_size;
	FrameSeq(tsFrame *frames,
	         size_t len);
	friend class BSpline;
};
/*! @} */



/*! @name Utility Classes
 *
 * Little helper classes, such as value classes or classes with only static
 * methods. These are primarily classes that do not really fit into another
 * group or are too small to form their own group.
 *
 * @{
 */
class TINYSPLINECXX_API Domain {
public:
	/* Constructors & Destructors */
	Domain(real min, real max);

	/* Accessors */
	real min() const;
	real max() const;

	/* Debug */
	std::string toString() const;

private:
	real m_min,
	     m_max;

#ifdef TINYSPLINE_EMSCRIPTEN
public:
	Domain()
	: Domain(TS_DOMAIN_DEFAULT_MIN, TS_DOMAIN_DEFAULT_MAX)
	{}
	void setMin(real) { cannotWrite(); }
	void setMax(real) { cannotWrite(); }
#endif
};
/*! @} */



class TINYSPLINECXX_API DeBoorNet {
public:
	/* Constructors & Destructors */
	DeBoorNet(const DeBoorNet &other);
	DeBoorNet(DeBoorNet &&other);
	virtual ~DeBoorNet();

	/* Operators */
	DeBoorNet & operator=(const DeBoorNet &other);
	DeBoorNet & operator=(DeBoorNet &&other);

	/* Accessors */
	real knot() const;
	size_t index() const;
	size_t multiplicity() const;
	size_t numInsertions() const;
	size_t dimension() const;
	std::vector<real> points() const;
	std::vector<real> result() const;

	/**
	* Returns the result at \p idx as ::Vec2. Note that, by design, \p idx
	* cannot be greater than \c 1. It is safe to call this method even if
	* ::dimension is less than \c 2. In this case, the missing components
	* are set to \c 0. If ::dimension is greater than \c 2, the excess
	* values are ignored.
	*
	* @return
	* 	The result at \p idx as ::Vec2.
	* @throws std::out_of_range
	* 	If \p idx is greater than \c 1, or if \p idx is \c 1, but there
	* 	is only one result.
	*/
	Vec2 resultVec2(size_t idx = 0) const;

	/**
	* Returns the result at \p idx as ::Vec3. Note that, by design, \p idx
	* cannot be greater than \c 1. It is safe to call this method even if
	* ::dimension is less than \c 3. In this case, the missing components
	* are set to \c 0. If ::dimension is greater than \c 3, the excess
	* values are ignored.
	*
	* @return
	* 	The result at \p idx as ::Vec3.
	* @throws std::out_of_range
	* 	If \p idx is greater than \c 1, or if \p idx is \c 1, but there
	* 	is only one result.
	*/
	Vec3 resultVec3(size_t idx = 0) const;

	/**
	* Returns the result at \p idx as ::Vec4. Note that, by design, \p idx
	* cannot be greater than \c 1. It is safe to call this method even if
	* ::dimension is less than \c 4. In this case, the missing components
	* are set to \c 0. If ::dimension is greater than \c 4, the excess
	* values are ignored.
	*
	* @return
	* 	The result at \p idx as ::Vec4.
	* @throws std::out_of_range
	* 	If \p idx is greater than \c 1, or if \p idx is \c 1, but there
	* 	is only one result.
	*/
	Vec4 resultVec4(size_t idx = 0) const;

	/* Debug */
	std::string toString() const;

private:
	tsDeBoorNet net;

	/* Constructors & Destructors */
	explicit DeBoorNet(tsDeBoorNet &data);

	friend class BSpline;

#ifdef TINYSPLINE_EMSCRIPTEN
public:
	DeBoorNet() : net(ts_deboornet_init()) {}
	void setKnot(real) { cannotWrite(); }
	void setIndex(size_t) { cannotWrite(); }
	void setMultiplicity(size_t) { cannotWrite(); }
	void setNumInsertions(size_t) { cannotWrite(); }
	void setDimension(size_t) { cannotWrite(); }
	void setPoints(std::vector<real>) { cannotWrite(); }
	void setResult(std::vector<real>) { cannotWrite(); }
#endif
};



class Morphism;
class ChordLengths;
class TINYSPLINECXX_API BSpline {
public:
	enum Type { Opened, Clamped, Beziers };

	/* Constructors & Destructors */
	BSpline();
	BSpline(const BSpline &other);
	BSpline(BSpline &&other);
	explicit BSpline(size_t numControlPoints,
	                 size_t dimension = 2,
	                 size_t degree = 3,
	                 Type type = Type::Clamped);
	virtual ~BSpline();

	/* Create from static method */
	static BSpline interpolateCubicNatural(std_real_vector_in points,
	                                       size_t dimension);
	static BSpline interpolateCatmullRom(std_real_vector_in points,
	                                     size_t dimension,
	                                     real alpha = (real) 0.5,
	                                     std::vector<real> *first = nullptr,
	                                     std::vector<real> *last = nullptr,
	                                     real epsilon = TS_POINT_EPSILON);
	static BSpline parseJson(std::string json);
	static BSpline load(std::string path);

	static bool knotsEqual(real x, real y);

	/* Operators */
	BSpline & operator=(const BSpline &other);
	BSpline & operator=(BSpline &&other);
	DeBoorNet operator()(real u) const;

	/* Accessors */
	size_t degree() const;
	size_t order() const;
	size_t dimension() const;
	std::vector<real> controlPoints() const;
	Vec2 controlPointVec2At(size_t idx) const;
	Vec3 controlPointVec3At(size_t idx) const;
	Vec4 controlPointVec4At(size_t idx) const;
	std::vector<real> knots() const;
	real knotAt(size_t index) const;

	/* Query */
	size_t numControlPoints() const;
	DeBoorNet eval(real u) const;
	std_real_vector_out evalAll(std_real_vector_in knots) const;
	std_real_vector_out sample(size_t num = 0) const;
	DeBoorNet bisect(real value,
	                 real epsilon = (real) 0.0,
	                 bool persnickety = false,
	                 size_t index = 0,
	                 bool ascending = true,
	                 size_t maxIter = 50) const;
	Domain domain() const;
	bool isClosed(real epsilon = TS_POINT_EPSILON) const;
	FrameSeq computeRMF(std_real_vector_in knots,
	                    Vec3 *firstNormal = nullptr) const;
	BSpline subSpline(real knot0, real knot1) const;
	std_real_vector_out uniformKnotSeq(size_t num = 100) const;
	std_real_vector_out equidistantKnotSeq(size_t num = 100,
	                                       size_t numSamples = 0) const;
	ChordLengths chordLengths(std_real_vector_in knots) const;
	ChordLengths chordLengths(size_t numSamples = 200) const;

	/* Serialization */
	std::string toJson() const;
	void save(std::string path) const;

	/* Modifications */
	void setControlPoints(const std::vector<real> &ctrlp);
	void setControlPointVec2At(size_t idx, Vec2 &cp);
	void setControlPointVec3At(size_t idx, Vec3 &cp);
	void setControlPointVec4At(size_t idx, Vec4 &cp);
	void setKnots(const std::vector<real> &knots);
	void setKnotAt(size_t index, real knot);

	/* Transformations */
	BSpline insertKnot(real u, size_t n) const;
	BSpline split(real u) const;
	BSpline tension(real tension) const;
	BSpline toBeziers() const;
	BSpline derive(size_t n = 1,
		real epsilon = TS_POINT_EPSILON) const;
	BSpline elevateDegree(size_t amount,
		real epsilon = TS_POINT_EPSILON) const;
	BSpline alignWith(const BSpline &other, BSpline &otherAligned,
		real epsilon = TS_POINT_EPSILON) const;
	Morphism morphTo(const BSpline &other,
		real epsilon = TS_POINT_EPSILON) const;

	/* Debug */
	std::string toString() const;

private:
	tsBSpline spline;

	/* Constructors & Destructors */
	explicit BSpline(tsBSpline &data);

	/* Needs to access ::spline. */
	friend class Morphism;

#ifdef TINYSPLINE_EMSCRIPTEN
public:
	std_real_vector_out sample0() const { return sample(); }
	std_real_vector_out sample1(size_t num) const { return sample(num); }
	BSpline derive0() const { return derive(); }
	BSpline derive1(size_t n) const { return derive(n); }
	BSpline derive2(size_t n, real eps) const { return derive(n, eps); }
#endif
};



/*! @name Spline Morphing
 *
 * @{
 */
class TINYSPLINECXX_API Morphism {
public:
	Morphism(const BSpline &origin,
		 const BSpline &target,
		 real epsilon = TS_POINT_EPSILON);

	BSpline origin() const;
	BSpline target() const;
	real epsilon() const;

	BSpline eval(real t);
	BSpline operator()(real t);

	std::string toString() const;
private:
	BSpline m_origin, m_target;
	real m_epsilon;
	BSpline m_originAligned, m_targetAligned;
	BSpline m_buffer;
};
/*! @} */



/*! @name Reparametrization by Arc Length
 * @{
 */
class TINYSPLINECXX_API ChordLengths {
public:
	ChordLengths();
	ChordLengths(const ChordLengths &other);
	ChordLengths(ChordLengths &&other);
	virtual ~ChordLengths();

	ChordLengths &operator=(const ChordLengths &other);
	ChordLengths &operator=(ChordLengths &&other);

	BSpline spline() const;
	std::vector<real> knots() const;
	std::vector<real> values() const;
	size_t size() const;

	real arcLength() const;
	real lengthToKnot(real len) const;
	real tToKnot(real t) const;

	std::string toString() const;
private:
	BSpline m_spline;
	real *m_knots, *m_chordLengths;
	size_t m_size;
	ChordLengths(const BSpline &spline,
	             real *knots,
	             real *chordLengths,
	             size_t size);
	friend class BSpline;
};
/*! @} */

}



#ifdef TINYSPLINE_EMSCRIPTEN
using namespace tinyspline;
EMSCRIPTEN_BINDINGS(tinyspline) {
	function("exceptionMessage", &exceptionMessage);

	// Vector Math
	value_object<Vec2>("Vec2")
		.field("x",
		       &Vec2::x,
		       &Vec2::setX)
		.field("y",
		       &Vec2::y,
		       &Vec2::setY)
		.field("values",
		       &Vec2::values,
		       &Vec2::setValues)
	;
	value_object<Vec3>("Vec3")
		.field("x",
		       &Vec3::x,
		       &Vec3::setX)
		.field("y",
		       &Vec3::y,
		       &Vec3::setY)
		.field("z",
		       &Vec3::z,
		       &Vec3::setZ)
		.field("values",
		       &Vec3::values,
		       &Vec3::setValues)
	;
	value_object<Vec4>("Vec4")
		.field("x",
		       &Vec4::x,
		       &Vec4::setX)
		.field("y",
		       &Vec4::y,
		       &Vec4::setY)
		.field("z",
		       &Vec4::z,
		       &Vec4::setZ)
		.field("w",
		       &Vec4::w,
		       &Vec4::setW)
		.field("values",
		       &Vec4::values,
		       &Vec4::setValues)
	;
	class_<VecMath>("VecMath")
		.class_function("add", &VecMath::add2)
		.class_function("add", &VecMath::add3)
		.class_function("add", &VecMath::add4)
		.class_function("subtract", &VecMath::subtract2)
		.class_function("subtract", &VecMath::subtract3)
		.class_function("subtract", &VecMath::subtract4)
		.class_function("multiply", &VecMath::multiply2)
		.class_function("multiply", &VecMath::multiply3)
		.class_function("multiply", &VecMath::multiply4)
		.class_function("cross", &VecMath::cross3)
		.class_function("normalize", &VecMath::normalize2)
		.class_function("normalize", &VecMath::normalize3)
		.class_function("normalize", &VecMath::normalize4)
		.class_function("magnitude", &VecMath::magnitude2)
		.class_function("magnitude", &VecMath::magnitude3)
		.class_function("magnitude", &VecMath::magnitude4)
		.class_function("dot", &VecMath::dot2)
		.class_function("dot", &VecMath::dot3)
		.class_function("dot", &VecMath::dot4)
		.class_function("angle", &VecMath::angle2)
		.class_function("angle", &VecMath::angle3)
		.class_function("angle", &VecMath::angle4)
		.class_function("distance", &VecMath::distance2)
		.class_function("distance", &VecMath::distance3)
		.class_function("distance", &VecMath::distance4)
	;

	// Spline Framing
	value_object<Frame>("Frame")
		.field("position",
		       &Frame::position,
		       &Frame::setPosition)
		.field("tangent",
		       &Frame::tangent,
		       &Frame::setTangent)
		.field("normal",
		       &Frame::normal,
		       &Frame::setNormal)
		.field("binormal",
		       &Frame::normal,
		       &Frame::setNormal)
	;
	class_<FrameSeq>("FrameSeq")
		.constructor<>()
		.constructor<FrameSeq>()
		.function("size", &FrameSeq::size)
		.function("at", &FrameSeq::at)
		.function("toString", &FrameSeq::toString)
	;

	// Utility Classes
	value_object<Domain>("Domain")
		.field("min",
		       &Domain::min,
		       &Domain::setMin)
		.field("max",
		       &Domain::max,
		       &Domain::setMax)
	;

	value_object<DeBoorNet>("DeBoorNet")
		.field("knot",
		       &DeBoorNet::knot,
		       &DeBoorNet::setKnot)
		.field("index",
		       &DeBoorNet::index,
		       &DeBoorNet::setIndex)
		.field("multiplicity",
		       &DeBoorNet::multiplicity,
		       &DeBoorNet::setMultiplicity)
		.field("numInsertions",
		       &DeBoorNet::numInsertions,
		       &DeBoorNet::setNumInsertions)
		.field("dimension",
		       &DeBoorNet::dimension,
		       &DeBoorNet::setDimension)
		.field("points",
		       &DeBoorNet::points,
		       &DeBoorNet::setPoints)
		.field("result",
		       &DeBoorNet::result,
		       &DeBoorNet::setResult)
	;

	class_<BSpline>("BSpline")
	        .constructor<>()
	        //.constructor<BSpline>()
	        .constructor<size_t>()
	        .constructor<size_t, size_t>()
	        .constructor<size_t, size_t, size_t>()
	        .constructor<size_t, size_t, size_t, BSpline::Type>()

	        .class_function("interpolateCubicNatural",
			&BSpline::interpolateCubicNatural)
	        .class_function("interpolateCatmullRom",
			&BSpline::interpolateCatmullRom,
			allow_raw_pointers())
	        .class_function("parseJson", &BSpline::parseJson)

	        .property("degree", &BSpline::degree)
	        .property("order", &BSpline::order)
	        .property("dimension", &BSpline::dimension)
	        .property("controlPoints", &BSpline::controlPoints,
			&BSpline::setControlPoints)
	        .property("knots", &BSpline::knots, &BSpline::setKnots)
	        .property("domain", &BSpline::domain)

	        /* Property by index */
	        .function("knotAt", &BSpline::knotAt)
	        .function("setKnotAt", &BSpline::setKnotAt)

	        /* Query */
	        .function("numControlPoints", &BSpline::numControlPoints)
	        .function("eval", &BSpline::eval)
	        .function("evalAll", &BSpline::evalAll)
	        .function("sample",
			select_overload<std_real_vector_out() const>
			(&BSpline::sample0))
	        .function("sample",
			select_overload<std_real_vector_out(size_t) const>
			(&BSpline::sample1))
	        .function("sample", &BSpline::sample)
	        .function("bisect", &BSpline::bisect)
	        .function("isClosed", &BSpline::isClosed)

		/* Serialization */
	        .function("toJson", &BSpline::toJson)

	        /* Transformations */
	        .function("insertKnot", &BSpline::insertKnot)
	        .function("split", &BSpline::split)
	        .function("tension", &BSpline::tension)
	        .function("toBeziers", &BSpline::toBeziers)
	        .function("derive",
			select_overload<BSpline() const>
			(&BSpline::derive0))
	        .function("derive",
			select_overload<BSpline(size_t) const>
			(&BSpline::derive1))
	        .function("derive",
			select_overload<BSpline(size_t, real) const>
			(&BSpline::derive2))

	        /* Debug */
	        .function("toString", &BSpline::toString)
	;

	enum_<BSpline::Type>("BSplineType")
	        .value("Opened", BSpline::Type::Opened)
	        .value("Clamped", BSpline::Type::Clamped)
	        .value("Beziers", BSpline::Type::Beziers)
	;
}
#endif
