#ifndef IHit_HPP_INCLUDED
#define IHit_HPP_INCLUDED
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
 * @file IHit.hpp
 * @brief The hit-testing interface.
 */
#include <navlib/navlib_types.h>

namespace TDx {
namespace SpaceMouse {
namespace Navigation3D {
/// <summary>
/// The hit-testing interface.
/// </summary>
class IHit {
public:
  /// <summary>
  /// Is called when the navigation library queries the result of the hit-testing.
  /// </summary>
  /// <param name="position">The hit <see cref="navlib::point_t"/> in world coordinates.</param>
  /// <returns>0 =no error, otherwise &lt;0 <see cref="navlib::make_result_code"/>.</returns>
  virtual long GetHitLookAt(navlib::point_t &position) const = 0;

  /// <summary>
  /// Is called when the navigation library sets the aperture of the hit-testing ray/cone.
  /// </summary>
  /// <param name="aperture">The aperture of the ray/cone on the near plane.</param>
  /// <returns>0 =no error, otherwise &lt;0 <see cref="navlib::make_result_code"/>.</returns>
  virtual long SetHitAperture(double aperture) = 0;

  /// <summary>
  /// Is called when the navigation library sets the direction of the hit-testing ray/cone.
  /// </summary>
  /// <param name="direction">The <see cref="navlib::vector_t"/> direction of the ray/cone.</param>
  /// <returns>0 =no error, otherwise &lt;0 <see cref="navlib::make_result_code"/>.</returns>
  virtual long SetHitDirection(const navlib::vector_t& direction) = 0;

  /// <summary>
  /// Is called when the navigation library sets the source of the hit-testing ray/cone.
  /// </summary>
  /// <param name="eye">The source <see cref="navlib::point_t"/> of the hit cone.</param>
  /// <returns>0 =no error, otherwise &lt;0 <see cref="navlib::make_result_code"/>.</returns>
  virtual long SetHitLookFrom(const navlib::point_t& eye) = 0;

  /// <summary>
  /// Is called when the navigation library sets the selection filter for hit-testing.
  /// </summary>
  /// <param name="onlySelection">true = ignore non-selected items.</param>
  /// <returns>0 =no error, otherwise &lt;0 <see cref="navlib::make_result_code"/>.</returns>
  virtual long SetHitSelectionOnly(bool onlySelection) = 0;
};
} // namespace Navigation3D
} // namespace SpaceMouse
} // namespace TDx
#endif // IHit_HPP_INCLUDED