#ifndef NAVLIB_TYPES_H_INCLUDED_
#define NAVLIB_TYPES_H_INCLUDED_
/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (c) 2014-2021 3Dconnexion.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * @file navlib_types.h
 * @brief the variable types used in the 3dconnexion navlib interface.
 */

#include <navlib/navlib_defines.h>

#include <errno.h>
#if (defined(_MSC_VER) && _MSC_VER < 1600)
typedef __int8 int8_t;
typedef unsigned __int32 uint32_t;
typedef unsigned __int64 uint64_t;
#if _WIN64
typedef unsigned __int64 size_t;
#else
typedef unsigned int size_t;
#endif
#else
#include <stddef.h>
#include <stdint.h>
#endif

// 3dxware
#include <siappcmd_types.h>

#if __cplusplus
#include <cstring>
#include <sstream>
#include <stdexcept>
/// <summary>
/// Contains the navigation library API types and functions
/// </summary>
_NAVLIB_BEGIN

/// <summary>
/// Defines a type of object to be thrown as exception. It reports errors that result from attempts
/// to convert a value to an incompatible type.
/// </summary>
class conversion_error : public std::logic_error {
public:
  typedef std::logic_error _Mybase;

  explicit conversion_error(const std::string &_Message)
      : _Mybase(_Message.c_str()) { // construct from message string
  }

  explicit conversion_error(const char *_Message)
      : _Mybase(_Message) { // construct from message string
  }
};
_NAVLIB_END
#endif //__cplusplus

_NAVLIB_BEGIN
/// <summary>
/// Describes the type of a property as well as the type of the value passed in value_t
/// </summary>
/// <remarks>
/// Generally no type conversion is performed. So that if a property is defined as being of
/// float_type then the navlib will expect the value_t structure to contain a value with
/// type=float_type.
/// </remarks>
typedef enum propertyTypes {
  /// <summary>
  /// <see cref="auto"/>.
  /// </summary>
  auto_type = -2,
  /// <summary>
  /// The type is unknown.
  /// </summary>
  unknown_type = -1,
  /// <summary>
  /// <see cref="void"/>*.
  /// </summary>
  voidptr_type = 0,
  /// <summary>
  /// <see cref="bool_t"/>.
  /// </summary>
  bool_type,
  /// <summary>
  /// <see cref="long"/>.
  /// </summary>
  long_type,
  /// <summary>
  /// <see cref="float"/>.
  /// </summary>
  float_type,
  /// <summary>
  /// <see cref="double"/>.
  /// </summary>
  double_type,
  /// <summary>
  /// <see cref="point_t"/>.
  /// </summary>
  point_type,
  /// <summary>
  /// <see cref="vector_t"/>.
  /// </summary>
  vector_type,
  /// <summary>
  /// <see cref="matrix_t"/>.
  /// </summary>
  matrix_type,
  /// <summary>
  /// <see cref="string_t"/>.
  /// </summary>
  string_type,
  /// <summary>
  /// const <see cref="SiActionNodeEx_t"/>*.
  /// </summary>
  actionnodeexptr_type,
  /// <summary>
  /// <see cref="plane_t"/>.
  /// </summary>
  plane_type,
  /// <summary>
  /// <see cref="box_t"/>.
  /// </summary>
  box_type,
  /// <summary>
  /// <see cref="frustum_t"/>.
  /// </summary>
  frustum_type,
  /// <summary>
  /// <see cref="cstr_t"/>.
  /// </summary>
  cstr_type,
  /// <summary>
  /// <see cref="imagearray_t"/>.
  /// </summary>
  imagearray_type
} propertyType_t;

/// <summary>
/// Type used to identify which property is being addressed.
/// </summary>
typedef const char *property_t;

/// <summary>
/// Describes the available property access
/// </summary>
typedef enum propertyAccess {
  /// <summary>
  /// Property cannot be accessed.
  /// </summary>
  eno_access = 0,
  /// <summary>
  /// Property can be changed.
  /// </summary>
  ewrite_access,
  /// <summary>
  /// Property is read only.
  /// </summary>
  eread_access,
  /// <summary>
  /// Property can be read and written to.
  /// </summary>
  eread_write_access
} propertyAccess_t;

/// <summary>
/// Describes a property's type and required access.
/// </summary>
/// <remarks>
/// This defines the value of <see cref="value_t.type"/> used in <see cref="value_t"/> to pass the
/// property's value between the client and the navlib.
/// </remarks>
typedef struct {
  /// <summary>
  /// The name of the property.
  /// </summary>
  property_t name;
  /// <summary>
  /// The type of the value stored by the property.
  /// </summary>
  propertyType_t type;
  /// <summary>
  /// The access the client interface exposes to the navlib server
  /// </summary>
  propertyAccess_t client;
} propertyDescription_t;

/// <summary>
/// Type used to store bool values in <see cref="value_t>"/>
/// </summary>
typedef uint32_t bool_t;

/// <summary>
/// Type used for the client defined parameter used in <see cref="fnGetProperty_t"/> and <see
/// cref="fnSetProperty_t"/>.
/// </summary>
typedef uint64_t param_t;

/// <summary>
/// Represents an x-, y-, and z-coordinate location in 3-D space.
/// </summary>
typedef struct point {
  union {
    struct {
      double x, y, z;
    };
    double coordinates[3];
  };
#if __cplusplus
  typedef const double (&const_array)[3];
  operator const_array() const {
    return coordinates;
  }
#endif
} point_t;

/// <summary>
/// Represents a displacement in 3-D space.
/// </summary>
typedef struct vector {
  union {
    struct {
      double x, y, z;
    };
    double components[3];
  };
#if __cplusplus
  typedef const double (&const_array)[3];
  operator const_array() const {
    return components;
  }
#endif
} vector_t;

/// <summary>
/// Represents a plane in 3-D space.
/// </summary>
/// <remarks>The plane is defined as a unit normal <see cref="vector_t"/> to the plane and the
/// distance of the plane to the origin along the normal. A <see cref="point_t"/> p on the plane
/// satisfies the vector equation: <para>n.(p - point_t(0,0,0)) + d = 0;</para></remarks>
typedef struct plane {
  union {
    struct {
      double x, y, z, d;
    };
    vector_t n;
    double equation[4];
  };
#if __cplusplus
  typedef const double (&const_array)[4];
  operator const_array() const {
    return equation;
  }
#endif
} plane_t;

/// <summary>
/// Represents a 3D-rectangle volume.
/// </summary>
/// <remarks>
/// The volume is described by two diagonally opposing <see cref="point_t"/> locations.
/// <see cref="box_t.min"/> contains the coordinates with the smallest values.
/// </remarks>
typedef struct box {
  union {
    struct {
      double min_x, min_y, min_z, max_x, max_y, max_z;
    };
    struct {
      point_t min, max;
    };
    double b[6];
  };
#if __cplusplus
  typedef const double (&const_array)[6];
  operator const_array() const {
    return b;
  }
  /// <summary>
  /// checks whether the box is empty.
  /// </summary>
  /// <returns></returns>
  bool empty() const {
    return (max_x < min_x || max_y < min_y || max_z < min_z);
  }
#endif
} box_t;

/// <summary>
/// Represents a frustum.
/// </summary>
typedef struct frustum {
  union {
    struct {
      double left, right, bottom, top, nearVal, farVal;
    };
    double parameters[6];
  };
#if __cplusplus
  typedef const double (&const_array)[6];
  operator const_array() const {
    return parameters;
  }
#endif
} frustum_t;

/// <summary>
/// Represents a 4 x 4 matrix used for transformations in 3-D space.
/// </summary>
/// <remarks>
/// The default matrix_t is in column major order this can be changed by specifying row_major_order
/// in the call to <see cref="NlCreate"/>.
/// </remarks>
typedef struct matrix {
  union {
    struct {
      double m00, m01, m02, m03, m10, m11, m12, m13, m20, m21, m22, m23, m30, m31, m32, m33;
    };
    double m[16];
    double m4x4[4][4];
  };
#if __cplusplus
  typedef const double (&const_array)[16];
  operator const_array() const {
    return m;
  }
#endif
} matrix_t;

/// <summary>
/// Represents a writable string.
/// </summary>
typedef struct {
  char *p;
  size_t length;
} string_t;

/// <summary>
/// Represents a read only string.
/// </summary>
typedef struct {
  const char *p;
  size_t length;
} cstr_t;

/// <summary>
/// Represents an array of <see cref="SiImage_t"/> pointers.
/// </summary>
typedef struct {
  const SiImage_t *p;
  size_t count;
} imagearray_t;
_NAVLIB_END
#if __cplusplus
#include <navlib/navlib_templates.h>
#endif
_NAVLIB_BEGIN

/// <summary>
/// Variant type used to transfer property values in the interface.
/// </summary>
/// <remarks><see cref="value_t"/> can hold one of the types defined in
/// <see cref="propertyType_t"/>.</remarks>
typedef struct value {
  /// <summary>
  /// The <see cref="propertyType_t"/> of the contained value.
  /// </summary>
  propertyType_t type;
  /// <summary>
  /// The contained data value.
  /// </summary>
  union {
    void *p;
    bool_t b;
    long l;
    float f;
    double d;
    point_t point;
    vector_t vector;
    plane_t plane;
    box_t box;
    frustum_t frustum;
    matrix_t matrix;
    const SiActionNodeEx_t *pnode;
    string_t string;
    cstr_t cstr_;
    imagearray_t imagearray;
  };

#if __cplusplus
  /// <summary>
  /// Instantiates an <see cref="auto_type"/> <see cref="value"/>.
  /// </summary>
  /// <remarks>
  /// <see cref="auto_type"/> values can be assigned to but not read. The data assignment also
  /// assigns the type.
  /// </remarks>
  value() : type(auto_type) {
  }
  /// <summary>
  /// Instantiates a <see cref="voidptr_type"/> <see cref="value"/> containing a
  /// <see cref="void"/>*.
  /// </summary>
  value(void *_p) : type(voidptr_type), p(_p) {
  }
  /// <summary>
  /// Instantiates a <see cref="bool_type"/> <see cref="value"/> containing a <see cref="bool_t"/>.
  /// </summary>
  value(bool _b) : type(bool_type), b(static_cast<bool_t>(_b)) {
  }
  /// <summary>
  /// Instantiates a <see cref="bool_type"/> <see cref="value"/> containing a <see cref="bool_t"/>.
  /// </summary>
  value(bool_t _b) : type(bool_type), b(_b) {
  }
  /// <summary>
  /// Instantiates a <see cref="long_type"/> <see cref="value"/> containing a <see cref="long"/>.
  /// </summary>
  value(long _l) : type(long_type), l(_l) {
  }
  /// <summary>
  /// Instantiates a <see cref="float_type"/> <see cref="value"/> containing a <see cref="float"/>.
  /// </summary>
  value(float _f) : type(float_type), f(_f) {
  }
  /// <summary>
  /// Instantiates a <see cref="double_type"/> <see cref="value"/> containing a
  /// <see cref="double"/>.
  /// </summary>
  value(double _d) : type(double_type), d(_d) {
  }
  /// <summary>
  /// Instantiates a <see cref="point_type"/> <see cref="value"/> containing a
  /// <see cref="point_t"/>.
  /// </summary>
  value(const point_t &_p) : type(point_type), point(_p) {
  }
  /// <summary>
  /// Instantiates a <see cref="vector_type"/> <see cref="value"/> containing a
  /// <see cref="vector_t"/>.
  /// </summary>
  value(const vector_t &_v) : type(vector_type), vector(_v) {
  }
  /// <summary>
  /// Instantiates a <see cref="plane_type"/> <see cref="value"/> containing a
  /// <see cref="plane_t"/>.
  /// </summary>
  value(const plane_t &_e) : type(plane_type), plane(_e) {
  }
  /// <summary>
  /// Instantiates a <see cref="box_type"/> <see cref="value"/> containing a <see cref="box_t"/>.
  /// </summary>
  value(const box_t &_box) : type(box_type), box(_box) {
  }
  /// <summary>
  /// Instantiates a <see cref="frustum_type"/> <see cref="value"/> containing a
  /// <see cref="frustum_t"/>.
  /// </summary>
  value(const frustum_t &_frustum) : type(frustum_type), frustum(_frustum) {
  }
  /// <summary>
  /// Instantiates a <see cref="matrix_type"/> <see cref="value"/> containing a
  /// <see cref="matrix_t"/>.
  /// </summary>
  value(const matrix_t &_m) : type(matrix_type), matrix(_m) {
  }
  /// <summary>
  /// Instantiates a <see cref="actionnodeexptr_type"/> <see cref="value"/> containing a
  /// <see cref="SiActionNodeEx_t"/>*.
  /// </summary>
  value(const SiActionNodeEx_t *_pnode) : type(actionnodeexptr_type), pnode(_pnode) {
  }
  /// <summary>
  /// Instantiates a <see cref="string_type"/> <see cref="value"/> containing a
  /// <see cref="string_t"/>.
  /// </summary>
  value(string_t &_s) : type(string_type), string(_s) {
  }
  /// <summary>
  /// Instantiates a <see cref="cstr_type"/> <see cref="value"/> containing a <see cref="cstr_t"/>.
  /// </summary>
  value(cstr_t &_cstr) : type(cstr_type), cstr_(_cstr) {
  }
  /// <summary>
  /// Instantiates a <see cref="string_type"/> <see cref="value"/> containing a
  /// <see cref="string_t"/>.
  /// </summary>
  value(char *_string, size_t length) : type(string_type) {
    string.length = length;
    string.p = _string;
  }
  /// <summary>
  /// Instantiates a <see cref="cstr_type"/> <see cref="value"/> containing a <see cref="cstr_t"/>.
  /// </summary>
  value(const char *_cstr) : type(cstr_type) {
    cstr_.p = _cstr;
    cstr_.length = _cstr != 0 ? strlen(_cstr) + 1 : 0;
  }
  /// <summary>
  /// Instantiates a <see cref="cstr_type"/> <see cref="value"/> containing a <see cref="cstr_t"/>.
  /// </summary>
  value(const std::string &_string) : type(cstr_type) {
    cstr_.p = _string.c_str();
    cstr_.length = _string.length() + 1;
  }
  /// <summary>
  /// Instantiates an <see cref="imagearray_type"/> <see cref="value"/> containing a
  /// <see cref="imagearray_t"/>.
  /// </summary>
  value(const imagearray_t &images) : type(imagearray_type), imagearray(images) {
  }
  /// <summary>
  /// Instantiates an <see cref="imagearray_type"/> <see cref="value"/> containing a
  /// <see cref="imagearray_t"/>.
  /// </summary>
  value(const SiImage_t *_pimage, size_t count) : type(imagearray_type) {
    imagearray.count = count;
    imagearray.p = _pimage;
  }

  /// <summary>
  /// Conversion operator
  /// </summary>
  /// <returns>A <see cref="bool"/> representation of the internal value.</returns>
  /// <exception cref="">Throws <see cref="conversion_error"/> if the conversion is not
  /// possible.</exception>
  operator bool() const {
    return cast_value<bool, const value>()(*this);
  }

  /// <summary>
  /// Conversion operator
  /// </summary>
  /// <returns>A <see cref="bool"/> representation of the internal value.</returns>
  /// <exception cref="">Throws <see cref="conversion_error"/> if the conversion is not
  /// possible.</exception>
  operator bool() {
    return cast_value<bool, value>()(*this);
  }

  /// <summary>
  /// Conversion operator
  /// </summary>
  /// <returns>A <see cref="bool_t"/> representation of the internal value.</returns>
  /// <exception cref="">Throws <see cref="conversion_error"/> if the conversion is not
  /// possible.</exception>
  operator bool_t() const {
    return cast_value<bool_t, const value>()(*this);
  }

  /// <summary>
  /// Conversion operator
  /// </summary>
  /// <returns>A <see cref="bool_t"/> representation of the internal value.</returns>
  /// <exception cref="">Throws <see cref="conversion_error"/> if the conversion is not
  /// possible.</exception>
  operator bool_t() {
    return cast_value<bool_t, value>()(*this);
  }

  /// <summary>
  /// Conversion operator
  /// </summary>
  /// <returns>A <see cref="int"/> representation of the internal value.</returns>
  /// <exception cref="">Throws <see cref="conversion_error"/> if the conversion is not
  /// possible.</exception>
  operator int() const {
    return cast_value<int, const value>()(*this);
  }

  /// <summary>
  /// Conversion operator
  /// </summary>
  /// <returns>A <see cref="int"/> representation of the internal value.</returns>
  /// <exception cref="">Throws <see cref="conversion_error"/> if the conversion is not
  /// possible.</exception>
  operator int() {
    return cast_value<int, value>()(*this);
  }

  /// <summary>
  /// Conversion operator
  /// </summary>
  /// <returns>A <see cref="int"/> representation of the internal value.</returns>
  /// <exception cref="">Throws <see cref="conversion_error"/> if the conversion is not
  /// possible.</exception>
  operator long() const {
    return cast_value<long, const value>()(*this);
  }

  /// <summary>
  /// Conversion operator
  /// </summary>
  /// <returns>A <see cref="int"/> representation of the internal value.</returns>
  /// <exception cref="">Throws <see cref="conversion_error"/> if the conversion is not
  /// possible.</exception>
  operator long() {
    return cast_value<long, value>()(*this);
  }

  /// <summary>
  /// Conversion operator
  /// </summary>
  /// <returns>A <see cref="int"/> representation of the internal value.</returns>
  /// <exception cref="">Throws <see cref="conversion_error"/> if the conversion is not
  /// possible.</exception>
  operator float() const {
    return cast_value<float, const value>()(*this);
  }

  /// <summary>
  /// Conversion operator
  /// </summary>
  /// <returns>A <see cref="int"/> representation of the internal value.</returns>
  /// <exception cref="">Throws <see cref="conversion_error"/> if the conversion is not
  /// possible.</exception>
  operator float() {
    return cast_value<float, value>()(*this);
  }

  /// <summary>
  /// Conversion operator
  /// </summary>
  /// <returns>A <see cref="int"/> representation of the internal value.</returns>
  /// <exception cref="">Throws <see cref="conversion_error"/> if the conversion is not
  /// possible.</exception>
  operator double() const {
    return cast_value<double, const value>()(*this);
  }

  /// <summary>
  /// Conversion operator
  /// </summary>
  /// <returns>A <see cref="int"/> representation of the internal value.</returns>
  /// <exception cref="">Throws <see cref="conversion_error"/> if the conversion is not
  /// possible.</exception>
  operator double() {
    return cast_value<double, value>()(*this);
  }

  /// <summary>
  /// Template to enable implicit T& conversion.
  /// </summary>
  template <typename T> operator T &() {
    return conversion_operator<T &>();
  }

  /// <summary>
  /// Template to enable implicit T const & conversion.
  /// </summary>
  template <typename T> operator T const &() const {
    return cast_value<T &, const value>()(*this);
  }

  /// <summary>
  /// Template function to convert a value with type checking.
  /// </summary>
  /// <returns>The inner member variable of the union corresponding to the <see cref="property_t"/>
  /// type.</returns>
  /// <exception cref="conversion_error">Invalid conversion attempted.</exception>
  template <typename T> T conversion_operator() {
    if (std::is_reference<T>::value) {
      if (type == auto_type) {
        type = property_type<T>::type;
      }
    }

    if (type != property_type<T>::type) {
      if ((type == float_type && property_type<T>::type == double_type) ||
          (type == double_type && property_type<T>::type == float_type)) {
        type = property_type<T>::type;
      } else {
        throw_conversion_error(property_type<T>::name);
      }
    }

    return value_member<T, value, typename remove_cvref<T>::type>()(*this);
  }

  /// <summary>
  /// Template function to convert a value with type checking.
  /// </summary>
  /// <returns>The inner member variable of the union corresponding to the <see cref="property_t"/>
  /// type.</returns>
  /// <exception cref="conversion_error">Invalid conversion attempted.</exception>
  template <typename T> T conversion_operator() const {
    if (type != property_type<T>::type) {
      throw_conversion_error(property_type<T>::name);
    }

    return value_member<T, const value, typename remove_cvref<T>::type>()(*this);
  }

  /// <summary>
  /// Conversion / accessor operator.
  /// </summary>
  /// <returns>A <see cref="void"/>*& to the internal value.</returns>
  /// <exception cref="">Throws <see cref="conversion_error"/> if the conversion is not
  /// possible.</exception>
  operator void * &() {
    return conversion_operator<void *&>();
  }

  /// <summary>
  /// Conversion / accessor operator.
  /// </summary>
  /// <returns>A <see cref="point_t"/>& to the internal value.</returns>
  /// <exception cref="">Throws <see cref="conversion_error"/> if the conversion is not
  /// possible.</exception>
  operator point_t &() {
    return conversion_operator<point_t &>();
  }

  /// <summary>
  /// Conversion operator.
  /// </summary>
  /// <returns>A <see cref="point_t"/> const& representation of the internal value.</returns>
  /// <exception cref="">Throws <see cref="conversion_error"/> if the conversion is not
  /// possible.</exception>
  operator const point_t &() const {
    return conversion_operator<const point_t &>();
  }

  /// <summary>
  /// Conversion / accessor operator.
  /// </summary>
  /// <returns>A <see cref="vector_t"/>& to the internal value.</returns>
  /// <exception cref="">Throws <see cref="conversion_error"/> if the conversion is not
  /// possible.</exception>
  operator vector_t &() {
    return conversion_operator<vector_t &>();
  }

  /// <summary>
  /// Conversion operator.
  /// </summary>
  /// <returns>A <see cref="vector_t"/> const& representation of the internal value.</returns>
  /// <exception cref="">Throws <see cref="conversion_error"/> if the conversion is not
  /// possible.</exception>
  operator const vector_t &() const {
    return conversion_operator<const vector_t &>();
  }

  /// <summary>
  /// Conversion / accessor operator.
  /// </summary>
  /// <returns>A <see cref="plane_t"/>& to the internal value.</returns>
  /// <exception cref="">Throws <see cref="conversion_error"/> if the conversion is not
  /// possible.</exception>
  operator plane_t &() {
    return conversion_operator<plane_t &>();
  }

  /// <summary>
  /// Conversion operator.
  /// </summary>
  /// <returns>A <see cref="plane_t"/> const& representation of the internal value.</returns>
  /// <exception cref="">Throws <see cref="conversion_error"/> if the conversion is not
  /// possible.</exception>
  operator const plane_t &() const {
    return conversion_operator<const plane_t &>();
  }

  /// <summary>
  /// Conversion / accessor operator.
  /// </summary>
  /// <returns>A <see cref="box_t"/>& to the internal value.</returns>
  /// <exception cref="">Throws <see cref="conversion_error"/> if the conversion is not
  /// possible.</exception>
  operator box_t &() {
    return conversion_operator<box_t &>();
  }

  /// <summary>
  /// Conversion operator.
  /// </summary>
  /// <returns>A <see cref="box_t"/> const& representation of the internal value.</returns>
  /// <exception cref="">Throws <see cref="conversion_error"/> if the conversion is not
  /// possible.</exception>
  operator const box_t &() const {
    return conversion_operator<const box_t &>();
  }

  /// <summary>
  /// Conversion / accessor operator.
  /// </summary>
  /// <returns>A <see cref="frustum_t"/>& to the internal value.</returns>
  /// <exception cref="">Throws <see cref="conversion_error"/> if the conversion is not
  /// possible.</exception>
  operator frustum_t &() {
    return conversion_operator<frustum_t &>();
  }

  /// <summary>
  /// Conversion operator.
  /// </summary>
  /// <returns>A <see cref="frustum_t"/> const& representation of the internal value.</returns>
  /// <exception cref="">Throws <see cref="conversion_error"/> if the conversion is not
  /// possible.</exception>
  operator const frustum_t &() const {
    return conversion_operator<const frustum_t &>();
  }

  /// <summary>
  /// Conversion / accessor operator.
  /// </summary>
  /// <returns>A <see cref="matrix_t"/>& to the internal value.</returns>
  /// <exception cref="">Throws <see cref="conversion_error"/> if the conversion is not
  /// possible.</exception>
  operator matrix_t &() {
    return conversion_operator<matrix_t &>();
  }

  /// <summary>
  /// Conversion.
  /// </summary>
  /// <returns>A <see cref="matrix_t"/> const& representation of the internal value.</returns>
  /// <exception cref="">Throws <see cref="conversion_error"/> if the conversion is not
  /// possible.</exception>
  operator const matrix_t &() const {
    return conversion_operator<const matrix_t &>();
  }

  typedef const SiActionNodeEx_t *cpSiActionNodeEx_t;

  /// <summary>
  /// Conversion / accessor operator.
  /// </summary>
  /// <returns>A <see cref="cpSiActionNodeEx_t"/>& to the internal value.</returns>
  /// <exception cref="">Throws <see cref="conversion_error"/> if the conversion is not
  /// possible.</exception>
  operator cpSiActionNodeEx_t &() {
    return conversion_operator<cpSiActionNodeEx_t &>();
  }

  /// <summary>
  /// Conversion / accessor operator.
  /// </summary>
  /// <returns>A <see cref="imagearray_t"/>& to the internal value.</returns>
  /// <exception cref="">Throws <see cref="conversion_error"/> if the conversion is not
  /// possible.</exception>
  operator imagearray_t &() {
    return conversion_operator<imagearray_t &>();
  }

  /// <summary>
  /// Conversion operator.
  /// </summary>
  /// <returns>A <see cref="imagearray_t"/> const& representation of the internal value.</returns>
  /// <exception cref="">Throws <see cref="conversion_error"/> if the conversion is not
  /// possible.</exception>
  operator const imagearray_t &() const {
    return conversion_operator<const imagearray_t &>();
  }

  /// <summary>
  /// Throws a <see cref="conversion_error"/> exception.
  /// </summary>
  /// <param name="target_type">The attempted <see cref="property_t"/> type conversion.</param>
  /// <exception cref="conversion_error">Invalid conversion attempted.</exception>
  void throw_conversion_error(const std::string &target_type) const {
    std::ostringstream stream;
    stream << "Invalid conversion from value type " << type << " to " << target_type.c_str();
    throw _NAVLIB conversion_error(stream.str().c_str());
  }

  //////////////////////////////////////////////////////////////////
  // Strings - treat string_t and cstr_t the same to avoid confusion

  /// <summary>
  /// Conversion operator.
  /// </summary>
  /// <returns>A <see cref="char"/> const* representation of the internal value.</returns>
  /// <exception cref="">Throws <see cref="conversion_error"/> if the conversion is not
  /// possible.</exception>
  operator const char *() const {
    if (type != cstr_type && type != string_type) {
      throw_conversion_error("string_type");
    }
    return type == string_type ? string.p : cstr_.p;
  }

  /// <summary>
  /// Conversion operator.
  /// </summary>
  /// <returns>A <see cref="string_t"/> const* representation of the internal value.</returns>
  /// <exception cref="">Throws <see cref="conversion_error"/> if the conversion is not
  /// possible.</exception>
  operator const string_t *() const {
    if (type != cstr_type && type != string_type) {
      throw_conversion_error("cstr_type");
    }
    return &string;
  }

  /// <summary>
  /// Conversion operator.
  /// </summary>
  /// <returns>A <see cref="string_t"/> const& representation of the internal value.</returns>
  /// <exception cref="">Throws <see cref="conversion_error"/> if the conversion is not
  /// possible.</exception>
  operator const string_t &() const {
    if (type != cstr_type && type != string_type) {
      throw_conversion_error("cstr_type");
    }
    return string;
  }

  /// <summary>
  /// Conversion / accessor operator.
  /// </summary>
  /// <returns>A <see cref="string_t"/>& representation of the internal value.</returns>
  /// <exception cref="">Throws <see cref="conversion_error"/> if the conversion is not
  /// possible.</exception>
  operator string_t &() {
    if (type == auto_type) {
      type = string_type;
    }
    if (type != cstr_type && type != string_type) {
      throw_conversion_error("string_type");
    }
    return string;
  }

  /// <summary>
  /// Conversion operator.
  /// </summary>
  /// <returns>A <see cref="cstr_t"/> const* representation of the internal value.</returns>
  /// <exception cref="">Throws <see cref="conversion_error"/> if the conversion is not
  /// possible.</exception>
  operator const cstr_t *() const {
    if (type != cstr_type && type != string_type) {
      throw_conversion_error("cstr_type");
    }
    return &cstr_;
  }

  /// <summary>
  /// Conversion operator.
  /// </summary>
  /// <returns>A <see cref="cstr_t"/> const& representation of the internal value.</returns>
  /// <exception cref="">Throws <see cref="conversion_error"/> if the conversion is not
  /// possible.</exception>
  operator const cstr_t &() const {
    if (type != cstr_type && type != string_type) {
      throw_conversion_error("cstr_type");
    }
    return cstr_;
  }

  /// <summary>
  /// Conversion / accessor operator.
  /// </summary>
  /// <returns>A <see cref="cstr_t"/>& to the internal value.</returns>
  /// <exception cref="">Throws <see cref="conversion_error"/> if the conversion is not
  /// possible.</exception>
  operator cstr_t &() {
    if (type == auto_type) {
      type = cstr_type;
    }
    if (type != cstr_type && type != string_type) {
      throw_conversion_error("cstr_type");
    }
    return cstr_;
  }

  /// <summary>
  /// Conversion operator.
  /// </summary>
  /// <returns>A <see cref="SiActionNodeEx_t"/> const* representation of the internal
  /// value.</returns>
  /// <exception cref="">Throws <see cref="conversion_error"/> if the conversion is
  /// not possible.</exception>
  operator const SiActionNodeEx_t *() const {
    if (type != actionnodeexptr_type) {
      throw_conversion_error("actionnodeexptr_type");
    }
    return pnode;
  }

  /// <summary>
  /// Conversion operator.
  /// </summary>
  /// <returns>A <see cref="SiActionNodeEx_t"/> const& representation of the internal
  /// value.</returns>
  /// <exception cref="">Throws <see cref="conversion_error"/> if the conversion is
  /// not possible.</exception>
  operator const SiActionNodeEx_t &() const {
    if (type != actionnodeexptr_type) {
      throw_conversion_error("actionnodeexptr_type");
    }
    return *pnode;
  }

  /// <summary>
  /// Conversion operator.
  /// </summary>
  /// <returns>A <see cref="imagearray_t"/> const* representation of the internal value.</returns>
  /// <exception cref="">Throws <see cref="conversion_error"/> if the conversion is not
  /// possible.</exception>
  operator const imagearray_t *() const {
    if (type != imagearray_type) {
      throw_conversion_error("imagearray_type");
    }
    return &imagearray;
  }

  // Assignment operators

  /// <summary>
  /// Assignment operator.
  /// </summary>
  /// <param name="p_">The <see cref="void"/>* to assign to the <see cref="value"/> struct.</param>
  /// <returns>A <see cref="value"/>& to this struct.</returns>
  /// <remarks>Assigns a <see cref="void"/>* to the internal value and sets
  /// <see cref="value.type"/> to <see cref="voidptr_type"/>.</remarks>
  struct value &operator=(void *p_) {
    p = p_;
    type = voidptr_type;
    return *this;
  }

  /// <summary>
  /// Assignment operator.
  /// </summary>
  /// <param name="b_">The <see cref="bool"/> to assign to the <see cref="value"/> struct.</param>
  /// <returns>A <see cref="value"/>& to this struct.</returns>
  /// <remarks>Assigns a <see cref="bool"/> to the internal value and sets
  /// <see cref="value.type"/> to <see cref="bool_type"/>.</remarks>
  struct value &operator=(bool b_) {
    b = static_cast<bool_t>(b_);
    type = bool_type;
    return *this;
  }

  /// <summary>
  /// Assignment operator.
  /// </summary>
  /// <param name="l_">The <see cref="long"/> to assign to the <see cref="value"/> struct.</param>
  /// <returns>A <see cref="value"/>& to this struct.</returns>
  /// <remarks>Assigns a <see cref="long"/> to the internal value and sets
  /// <see cref="value.type"/> to <see cref="long_type"/>.</remarks>
  struct value &operator=(long l_) {
    l = l_;
    type = long_type;
    return *this;
  }

  /// <summary>
  /// Assignment operator.
  /// </summary>
  /// <param name="f_">The <see cref="float"/> to assign to the <see cref="value"/> struct.</param>
  /// <returns>A <see cref="value"/>& to this struct.</returns>
  /// <remarks>Assigns a <see cref="float"/> to the internal value and sets
  /// <see cref="value.type"/> to <see cref="float_type"/>.</remarks>
  struct value &operator=(float f_) {
    f = f_;
    type = float_type;
    return *this;
  }

  /// <summary>
  /// Assignment operator.
  /// </summary>
  /// <param name="d_">The <see cref="double"/> to assign to the <see cref="value"/> struct.</param>
  /// <returns>A <see cref="value"/>& to this struct.</returns>
  /// <remarks>Assigns a <see cref="double"/> to the internal value and sets
  /// <see cref="value.type"/> to <see cref="double_type"/>.</remarks>
  struct value &operator=(double d_) {
    d = d_;
    type = double_type;
    return *this;
  }

  /// <summary>
  /// Assignment operator.
  /// </summary>
  /// <param name="point_">The <see cref="point_t"/> to assign to the <see cref="value"/>
  /// struct.</param>
  /// <returns>A <see cref="value"/>& to this struct.</returns>
  /// <remarks>Assigns a <see cref="point_t"/> to the internal value and sets
  /// <see cref="value.type"/> to <see cref="point_type"/>.
  /// </remarks>
  struct value &operator=(const point_t &point_) {
    point = point_;
    type = point_type;
    return *this;
  }

  /// <summary>
  /// Assignment operator.
  /// </summary>
  /// <param name="plane_">The <see cref="plane_t"/> to assign to the <see cref="value"/>
  /// struct.</param>
  /// <returns>A <see cref="value"/>& to this struct.</returns>
  /// <remarks>Assigns a <see cref="plane_t"/> to the internal value and sets
  /// <see cref="value.type"/> to <see cref="plane_type"/>.
  /// </remarks>
  struct value &operator=(const plane_t &plane_) {
    plane = plane_;
    type = plane_type;
    return *this;
  }

  /// <summary>
  /// Assignment operator.
  /// </summary>
  /// <param name="box_">The <see cref="box_t"/> to assign to the <see cref="value"/> struct.
  /// </param>
  /// <returns>A <see cref="value"/>& to this struct.</returns>
  /// <remarks>Assigns a <see cref="box_t"/> to the internal value and sets
  /// <see cref="value.type"/> to <see cref="box_type"/>.</remarks>
  struct value &operator=(const box_t &box_) {
    box = box_;
    type = box_type;
    return *this;
  }

  /// <summary>
  /// Assignment operator.
  /// </summary>
  /// <param name="frustum_">The <see cref="frustum_t"/> to assign to the <see cref="value"/>
  /// struct.</param>
  /// <returns>A <see cref="value"/>& to this struct.</returns>
  /// <remarks>Assigns a <see cref="frustum_t"/> to the internal value and sets
  /// <see cref="value.type"/> to <see cref="frustum_type"/>.</remarks>
  struct value &operator=(const frustum_t &frustum_) {
    frustum = frustum_;
    type = frustum_type;
    return *this;
  }

  /// <summary>
  /// Assignment operator.
  /// </summary>
  /// <param name="matrix_">The <see cref="matrix_t"/> to assign to the <see cref="value"/>
  /// struct.</param>
  /// <returns>A <see cref="value"/>& to this struct.</returns>
  /// <remarks>Assigns a <see cref="matrix_t"/> to the internal value and sets
  /// <see cref="value.type"/> to <see cref="matrix_type"/>.</remarks>
  struct value &operator=(const matrix_t &matrix_) {
    matrix = matrix_;
    type = matrix_type;
    return *this;
  }

  /// <summary>
  /// Assignment operator.
  /// </summary>
  /// <param name="pnode_">The <see cref="SiActionNodeEx_t"/> const* to assign to the
  /// <see cref="value"/> struct.</param>
  /// <returns>A <see cref="value"/>& to this struct.</returns>
  /// <remarks>Assigns a <see cref="SiActionNodeEx_t"/>* to the internal value and sets
  /// <see cref="value.type"/> to <see cref="actionnodeexptr_type"/>.</remarks>
  struct value &operator=(const SiActionNodeEx_t *pnode_) {
    pnode = pnode_;
    type = actionnodeexptr_type;
    return *this;
  }

  /// <summary>
  /// Assignment operator.
  /// </summary>
  /// <param name="string_">The <see cref="string_t"/> const& to assign to the <see cref="value"/>
  /// struct.</param>
  /// <returns>A <see cref="value"/>& to this struct.</returns>
  /// <remarks>Assigns a <see cref="string_t"/> to the internal value and sets
  /// <see cref="value.type"/> to <see cref="string_type"/>.</remarks>
  struct value &operator=(const string_t &string_) {
    string = string_;
    type = string_type;
    return *this;
  }

  /// <summary>
  /// Assignment operator.
  /// </summary>
  /// <param name="cstr">The <see cref="cstr_t"/> const& to assign to the <see cref="value"/>
  /// struct.</param>
  /// <returns>A <see cref="value"/>& to this struct.</returns>
  /// <remarks>Assigns a <see cref="cstr_t"/> to the internal value and sets
  /// <see cref="value.type"/> to <see cref="cstr_type"/>.</remarks>
  struct value &operator=(const cstr_t &cstr) {
    cstr_ = cstr;
    type = cstr_type;
    return *this;
  }

  /// <summary>
  /// Assignment operator.
  /// </summary>
  /// <param name="images">The <see cref="imagearray_t"/> const& to assign to the
  /// <see cref="value"/> struct.</param>
  /// <returns>A <see cref="value"/>& to this struct.</returns>
  /// <remarks>Assigns a <see cref="imagearray_t"/> to the internal value and sets
  /// <see cref="value.type"/> to <see cref="imagearray_type"/>.</remarks>
  struct value &operator=(const imagearray_t &images) {
    imagearray = images;
    type = imagearray_type;
    return *this;
  }
#endif // __cplusplus
} value_t;

/// <summary>
/// The prototype of the function the navlib invokes to retrieve the value of a property.
/// </summary>
/// <param name="param">Value passed in <see cref="accessor_t.param"/> in <see cref="NlCreate"/>.
/// </param>
/// <param name="name">Value passed in <see cref="accessor_t.name"/> in <see cref="NlCreate"/>.
/// </param>
/// <param name="value">Pointer to a <see cref="value_t"/> for the value of the property.</param>
/// <returns>A navlib result code. <seealso cref="make_result_code"/>.</returns>
/// <remarks>When the application services the function call it sets the current value and the type
/// of the property into the <see cref="value_t"/> variant.</remarks>
typedef long(__cdecl *fnGetProperty_t)(const param_t param, const property_t name, value_t *value);

/// <summary>
/// The prototype of the function the navlib invokes to set the value of a property.
/// </summary>
/// <param name="param">Value passed in <see cref="accessor_t.param"/> in <see cref="NlCreate"/>.
/// </param>
/// <param name="name">Value passed in <see cref="accessor_t.name"/> in <see cref="NlCreate"/>.
/// </param>
/// <param name="value">A <see cref="value_t"/> containing the value of the property.</param>
/// <returns>A navlib result code. <seealso cref="make_result_code"/>.</returns>
/// <remarks>When the application services the function call it sets its current value of the
/// property from the <see cref="value_t"/> variant.</remarks>
typedef long(__cdecl *fnSetProperty_t)(const param_t param, const property_t name,
                                       const value_t *value);

/// <summary>
/// Represents a client property accessor and mutator.
/// </summary>
/// <remarks>
/// <para>
/// The application passes an array of accessor_t structures in <see cref="NlCreate"/> to define the
/// interface that the navlib can use to query or apply changes to a client property. Depending on
/// the user/3d mouse input the navlib will query the application for the values of properties
/// needed to fulfill the requirements of the navigation model the user has invoked and when a new
/// camera position has been calculated set the corresponding properties.
/// </para>
/// <para>
/// To retrieve the value of a property the navlib will call the fnGet member of the property's
/// accessor_t, passing in param, the name of the property as well as a pointer to a value_t.
/// Similarly, to apply a change to a property the navlib will invoke the fnSet member of the
/// property's accessor_t, passing in the param, property name and a pointer to a value_t.
/// </para>
/// <para>
/// If either of fnSet or fnGet is null, then the property is respectively read- or write-only.
/// </para>
/// </remarks>
typedef struct tagAccessor {
  /// <summary>
  /// The name of the property
  /// </summary>
  property_t name;
  /// <summary>
  /// The function the navlib calls to retrieve the property's value from the client application.
  /// </summary>
  fnGetProperty_t fnGet;
  /// <summary>
  /// The function the navlib calls to set the property's value in the client application.
  /// </summary>
  fnSetProperty_t fnSet;
  /// <summary>
  /// The value to pass to the <see cref="fnGetProperty_t"/> and <see cref="fnSetProperty_t"/>
  /// functions as the first parameter.
  /// </summary>
  param_t param;
} accessor_t;

/// <summary>
/// Type used for navlib handles.
/// </summary>
typedef uint64_t nlHandle_t;

/// <summary>
/// Configuration options
/// </summary>
typedef enum nlOptions {
  /// <summary>
  /// No options.
  /// </summary>
  none = 0,
  /// <summary>
  /// Use non-queued message passing.
  /// </summary>
  nonqueued_messages = 1,
  /// <summary>
  /// Matrices are stored in row major order.
  /// </summary>
  /// <remarks>
  /// The default is column major order.
  /// </remarks>
  row_major_order = 2
} nlOptions_t;

/// <summary>
/// Structure for navlib initialization options passed in the <see cref="NlCreate"/>NlCreate.
/// </summary>
typedef struct tagNlCreateOptions {
  /// <summary>
  /// Size of this structure.
  /// </summary>
  uint32_t size;
  /// <summary>
  /// true is to use multi-threading, false for single-threaded.
  /// </summary>
  /// <remarks>The default is false (single-threaded).</remarks>
#if __cplusplus
  bool bMultiThreaded;
#else
  int8_t bMultiThreaded;
#endif
  /// <inheritdoc/>
  nlOptions_t options;
} nlCreateOptions_t;

#ifndef TDX_ALWAYS_INLINE
#ifdef _MSC_VER
#define TDX_ALWAYS_INLINE __forceinline
#else
#define TDX_ALWAYS_INLINE __attribute__((always_inline)) inline
#endif
#endif
/// <summary>
/// Makes a result code in the navlib facility.
/// </summary>
/// <param name="x">The error code.</param>
/// <returns>A result code in the navlib facility.</returns>
TDX_ALWAYS_INLINE long make_result_code(unsigned long x) {
  return (long)(x) <= 0 ? (long)(x)
                        : (long)(((x)&0x0000FFFF) | (FACILITY_NAVLIB << 16) | 0x80000000);
}

#if __cplusplus
/// <summary>
/// Contains error codes used by the navlib.
/// </summary>
namespace navlib_errc {
#endif
/// <summary>
/// Error codes used in the navlib facility.
/// </summary>
enum navlib_errc_t {
  /// <summary>
  /// Error.
  /// </summary>
  error = EIO,
  /// <summary>
  /// Already connected.
  /// </summary>
  already_connected = EISCONN,
  /// <summary>
  /// The function is not supported.
  /// </summary>
  function_not_supported = ENOSYS,
  /// <summary>
  /// The argument is invlaid.
  /// </summary>
  invalid_argument = EINVAL,
  /// <summary>
  /// No data is available.
  /// </summary>
  no_data_available = ENODATA,
  /// <summary>
  /// Not enough memory.
  /// </summary>
  not_enough_memory = ENOMEM,
  /// <summary>
  /// The buffer is too small.
  /// </summary>
  insufficient_buffer = ENOBUFS,
  /// <summary>
  /// THe operation is invalid in the current context.
  /// </summary>
  invalid_operation = EPERM,
  /// <summary>
  /// Sorry, not allowed.
  /// </summary>
  permission_denied = EACCES,
  /// <summary>
  /// The property does not exists.
  /// </summary>
  property_not_found = 0x201,
  /// <summary>
  /// The function does not exist.
  /// </summary>
  invalid_function = 0x202,
};
#if __cplusplus
} // namespace navlib::navlib_errc::
#endif

_NAVLIB_END

#endif /* NAVLIB_TYPES_H_INCLUDED_ */