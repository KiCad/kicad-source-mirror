#ifndef IPivot_HPP_INCLUDED
#define IPivot_HPP_INCLUDED
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
 * @file IPivot.hpp
 * @brief Interface to access the pivot.
 */
#include <navlib/navlib_types.h>

namespace TDx {
namespace SpaceMouse {
namespace Navigation3D {
  /// <summary>
  /// The interface to access the pivot.
  /// </summary>
class IPivot {
public:
  /// <summary>
  /// Gets the position of the rotation pivot.
  /// </summary>
  /// <param name="position">The pivot <see cref="navlib::point_t"/> in world coordinates.</param>
  /// <returns>0 = no error, otherwise &lt;0.</returns>
  virtual long GetPivotPosition(navlib::point_t &position) const = 0;

  /// <summary>
  /// Queries if the user has manually set a pivot point.
  /// </summary>
  /// <param name="userPivot">true if the user has set a pivot otherwise false.</param>
  /// <returns>0 = no error, otherwise &lt;0.</returns>
  virtual long IsUserPivot(navlib::bool_t &userPivot) const = 0;

  /// <summary>
  /// Sets the position of the rotation pivot.
  /// </summary>
  /// <param name="position">The pivot <see cref="navlib::point_t"/> in world coordinates.</param>
  /// <returns>0 = no error, otherwise &lt;0.</returns>
  virtual long SetPivotPosition(const navlib::point_t& position) = 0;

  /// <summary>
  /// Queries the visibility of the pivot image.
  /// </summary>
  /// <param name="visible">true if the pivot is visible otherwise false.</param>
  /// <returns>0 = no error, otherwise &lt;0.</returns>
  virtual long GetPivotVisible(navlib::bool_t &visible) const = 0;

  /// <summary>
  /// Sets the visibility of the pivot image.
  /// </summary>
  /// <param name="visible">true if the pivot is visible otherwise false.</param>
  /// <returns>0 = no error, otherwise &lt;0.</returns>
  virtual long SetPivotVisible(bool visible) = 0;
};
} // namespace Navigation3D
} // namespace SpaceMouse
} // namespace TDx
#endif // IPivot_HPP_INCLUDED