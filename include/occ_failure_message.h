/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include <Standard_Failure.hxx>
#include <Standard_Version.hxx>

/**
 * @return the message of an OpenCASCADE exception.  OCCT 8 deprecates GetMessageString in favor of
 *         std::exception::what(), which older releases do not have.
 */
inline const char* OccFailureMessage( const Standard_Failure& aFailure )
{
#if OCC_VERSION_HEX >= 0x080000
    return aFailure.what();
#else
    return aFailure.GetMessageString();
#endif
}
