#ifndef IModel_HPP_INCLUDED
#define IModel_HPP_INCLUDED
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
 * @file IModel.hpp
 * @brief The 3D model interface.
 */
#include <navlib/navlib_types.h>

namespace TDx {
namespace SpaceMouse {
namespace Navigation3D {
/// <summary>
/// The Model interface
/// </summary>
class IModel {
public:
  /// <summary>
  /// Is called when the navigation library needs to get the extents of the model.
  /// </summary>
  /// <param name="extents">A <see cref="navlib::box_t"/> representing the extents of the
  /// model.</param>
  /// <returns>0 = no error, otherwise &lt;0.</returns>
  virtual long GetModelExtents(navlib::box_t &extents) const = 0;

  /// <summary>
  /// Is called when the navigation library needs to get the extents of the selection.
  /// </summary>
  /// <param name="extents">A <see cref="navlib::box_t"/> representing the extents of the
  /// selection.</param>
  /// <returns>0 = no error, otherwise &lt;0.</returns>
  virtual long GetSelectionExtents(navlib::box_t &extents) const = 0;

  /// <summary>
  /// Is called to get the selections's transform <see cref="navlib::matrix_t"/>.
  /// </summary>
  /// <param name="transform">The world affine <see cref="navlib::matrix_t"/> of the
  /// selection.</param>
  /// <returns>0 = no error, otherwise &lt;0.</returns>
  virtual long GetSelectionTransform(navlib::matrix_t &transform) const = 0;

  /// <summary>
  /// Is called to query if the selection is empty.
  /// </summary>
  /// <param name="empty">true if nothing is selected.</param>
  /// <returns>0 = no error, otherwise &lt;0.</returns>
  virtual long GetIsSelectionEmpty(navlib::bool_t &empty) const = 0;

  /// <summary>
  /// Is called to set the selections's transform <see cref="navlib::matrix_t"/>.
  /// </summary>
  /// <param name="matrix">The world affine <see cref="navlib::matrix_t"/> of the selection.</param>
  /// <returns>0 = no error, otherwise &lt;0.</returns>
  virtual long SetSelectionTransform(const navlib::matrix_t& matrix) = 0;
};
} // namespace Navigation3D
} // namespace SpaceMouse
} // namespace TDx
#endif // IModel_HPP_INCLUDED