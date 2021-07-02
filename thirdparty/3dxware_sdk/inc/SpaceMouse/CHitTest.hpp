#ifndef CHitTest_HPP_INCLUDED
#define CHitTest_HPP_INCLUDED
/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (c) 2018-2021 3Dconnexion.
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
 * @file CHitTest.hpp
 * @brief Hit-test properties.
 */

 // 3dxware
#include <navlib/navlib_types.h>

namespace TDx {
namespace SpaceMouse {
/// <summary>
/// Class can be used to hold the hit-test properties.
/// </summary>
template <class point_ = navlib::point_t, class vector_ = navlib::vector_t> class CHitTest {
public:
  typedef point_ point_type;
  typedef vector_ vector_type;

public:
  /// <summary>
  /// Creates a new instance of the <see cref="CHitTest"/> class.
  /// </summary>
  CHitTest() : m_aperture(1), m_dirty(false), m_selectionOnly(false) {
  }

#if USE_DECLSPEC_PROPERTY
  /// <summary>
  /// Property accessors
  /// </summary>
  __declspec(property(get = GetDirection, put = PutDirection)) vector_type Direction;
  __declspec(property(get = GetLookFrom, put = PutLookFrom)) point_type LookFrom;
  __declspec(property(get = GetLookingAt, put = PutLookingAt)) point_type LookingAt;
  __declspec(property(get = GetIsDirty, put = PutIsDirty)) bool IsDirty;
  __declspec(property(get = GetAperture, put = PutAperture)) double Aperture;
  __declspec(property(get = GetSelectionOnly, put = PutSelectionOnly)) bool SelectionOnly;
#endif

  /// <summary>
  /// Gets or sets the ray direction.
  /// </summary>
  void PutDirection(vector_type value) {
    if (!m_dirty) {
      m_dirty = static_cast<bool>(m_direction != value);
    }
    m_direction = std::move(value);
  }
  vector_type GetDirection() const {
    return m_direction;
  }

  /// <summary>
  /// Gets or sets the ray origin.
  /// </summary>
  void PutLookFrom(point_type value) {
    if (!m_dirty) {
      m_dirty = static_cast<bool>(m_lookFrom != value);
    }
    m_lookFrom = std::move(value);
  }
  const point_type GetLookFrom() const {
    return m_lookFrom;
  }

  /// <summary>
  /// Gets or sets the ray hit test result location.
  /// </summary>
  void PutLookingAt(point_type value) {
    m_lookingAt = std::move(value);
    m_dirty = false;
  }
  const point_type GetLookingAt() const {
    return m_lookingAt;
  }

  /// <summary>
  /// Gets or sets a value indicating if a the parameters have changed since the last hit calculation.
  /// </summary>
  void PutIsDirty(bool value) {
    m_dirty = value;
  }
  bool GetIsDirty() const {
    return m_dirty;
  }

  /// <summary>
  /// Gets or sets the ray diameter / aperture on the near clipping plane.
  /// </summary>
  void PutAperture(double value) {
    m_dirty = (m_aperture != value);
    m_aperture = value;
  }
  double GetAperture() const {
    return m_aperture;
  }

  /// <summary>
  /// Gets or sets a value indicating whether the hit-testing is limited to the selection set.
  /// </summary>
  void PutSelectionOnly(bool value) {
    m_selectionOnly = value;
  }
  bool GetSelectionOnly() const {
    return m_selectionOnly;
  }

private:
  double m_aperture;
  mutable bool m_dirty;
  vector_type m_direction;
  point_type m_lookFrom;
  mutable point_type m_lookingAt;
  bool m_selectionOnly;
};
} // namespace SpaceMouse
} // namespace TDx
#endif // CHitTest_HPP_INCLUDED

