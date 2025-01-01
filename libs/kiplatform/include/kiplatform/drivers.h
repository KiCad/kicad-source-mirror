/*
* This program source code file is part of KiCad, a free EDA CAD application.
*
* Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef KIPLATFORM_DRIVERS_H_
#define KIPLATFORM_DRIVERS_H_

#include <string>
#include <vector>
#include <charconv>

namespace KIPLATFORM
{
namespace DRIVERS
{
    /**
     * @brief Get the driver version for a given driver name
     * @return true if the driver version is valid, false otherwise
     */
    bool Valid3DConnexionDriverVersion();

} // namespace DRIVERS
} // namespace KIPLATFORM

#endif // KIPLATFORM_DRIVERS_H_