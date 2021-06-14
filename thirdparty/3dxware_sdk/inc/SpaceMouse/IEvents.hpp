#ifndef IEvents_HPP_INCLUDED
#define IEvents_HPP_INCLUDED
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
 * @file IEvents.hpp
 * @brief The events interface.
 */
#include <navlib/navlib_types.h>

//stdlib
#include <string>

namespace TDx {
namespace SpaceMouse {
namespace Navigation3D {
/// <summary>
/// The Events interface
/// </summary>
class IEvents {
public:
  /// <summary>
  /// Is called when the user invokes an application command from the SpaceMouse.
  /// </summary>
  /// <param name="commandId">The id of the command to invoke.</param>
  /// <returns>The result of the function: 0 = no error, otherwise &lt;0.</returns>
  virtual long SetActiveCommand(std::string commandId) = 0;

  /// <summary>
  /// Is called when the navigation settings change.
  /// </summary>
  /// <param name="count">The change count.</param>
  /// <returns>0 = no error, otherwise &lt;0.</returns>
  virtual long SetSettingsChanged(long count) = 0;

  /// <summary>
  /// Is invoked when the user releases a key on the 3D Mouse, which has been programmed to send a
  /// virtual key code.
  /// </summary>
  /// <param name="vkey">The virtual key code of the key pressed.</param>
  /// <returns>0 = no error, otherwise &lt;0.</returns>
  virtual long SetKeyPress(long vkey) = 0;

  /// <summary>
  /// Is invoked when the user releases a key on the 3D Mouse, which has been programmed to send a
  /// virtual key code.
  /// </summary>
  /// <param name="vkey">The virtual key code of the key released.</param>
  /// <returns>0 = no error, otherwise &lt;0.</returns>
  virtual long SetKeyRelease(long vkey) = 0;
};
} // namespace Navigation3D
} // namespace SpaceMouse
} // namespace TDx
#endif // IEvents_HPP_INCLUDED