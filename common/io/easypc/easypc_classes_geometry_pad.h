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

/// @file easypc_classes_geometry_pad.h Free pads, apart because they derive from a connectivity class

#ifndef EASYPC_CLASSES_GEOMETRY_PAD_H
#define EASYPC_CLASSES_GEOMETRY_PAD_H

#include <cstdint>

#include <io/easypc/easypc_classes_connectivity.h>


namespace EASYPC
{

/// A pad of a symbol definition
struct SOURCE_FREE_PAD : SOURCE_CONNECT_POINT
{
    LOAD_TASK Load( ARCHIVE& aAr ) override;

    OBJECT* Layer = nullptr;
    OBJECT* OldPinName = nullptr;     ///< version below 10003
    OBJECT* OldPinNumber = nullptr;   ///< version below 10003
    OBJECT* ValuePositions = nullptr; ///< version 10003 on
    int32_t Number = 0;
};

} // namespace EASYPC

#endif // EASYPC_CLASSES_GEOMETRY_PAD_H
