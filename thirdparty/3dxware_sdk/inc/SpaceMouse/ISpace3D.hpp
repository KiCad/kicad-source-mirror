#ifndef ISpace3D_HPP_INCLUDED
#define ISpace3D_HPP_INCLUDED
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
 * @file ISpace3D.hpp
 * @brief Interface to the client coordinate system.
 */
#include <navlib/navlib_types.h>

namespace TDx {
namespace SpaceMouse {
namespace Navigation3D {
/// <summary>
/// The interface to access the client coordinate system.
/// </summary>
class ISpace3D {
public:
  /// <summary>
  /// Gets the coordinate system used by the client.
  /// </summary>
  /// <param name="matrix">The coordinate system <see cref="navlib::matrix_t"/>.</param>
  /// <returns>0 = no error, otherwise &lt;0.</returns>
  /// <remarks>The matrix describes the applications coordinate frame in the navlib coordinate
  /// system. i.e. the application to navlib transform.</remarks>
  virtual long GetCoordinateSystem(navlib::matrix_t &matrix) const = 0;

  /// <summary>
  /// Gets the orientation of the front view.
  /// </summary>
  /// <param name="matrix">The front view transform <see cref="navlib::matrix_t"/>.</param>
  /// <returns>0 = no error, otherwise &lt;0.</returns>
  virtual long GetFrontView(navlib::matrix_t &matrix) const = 0;
};
} // namespace Navigation3D
} // namespace SpaceMouse
} // namespace TDx
#endif // ISpace3D_HPP_INCLUDED