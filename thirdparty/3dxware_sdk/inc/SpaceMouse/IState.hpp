#ifndef IState_HPP_INCLUDED
#define IState_HPP_INCLUDED
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
 * @file IState.hpp
 * @brief Interface to access the navigation state.
 */
#include <navlib/navlib_types.h>

namespace TDx {
namespace SpaceMouse {
namespace Navigation3D {
  /// <summary>
  /// Interface to access the navigation state.
  /// </summary>
class IState {
public:
  /// <summary>
  /// Is called when the navigation library starts or stops a navigation transaction.
  /// </summary>
  /// <param name="transaction">The transaction number: >0 begin, ==0 end.</param>
  /// <returns>0 = no error, otherwise &lt;0.</returns>
  virtual long SetTransaction(long transaction) = 0;

  /// <summary>
  /// Is called when the navigation instance starts or stops a sequence of motion frames.
  /// </summary>
  /// <param name="motion">The motion flag: true = start, false = end.</param>
  /// <returns>0 = no error, otherwise &lt;0.</returns>
  /// <remarks>This can be used to start an animation loop.</remarks>
  virtual long SetMotionFlag(bool motion) = 0;
};
} // namespace Navigation3D
} // namespace SpaceMouse
} // namespace TDx
#endif // IState_HPP_INCLUDED