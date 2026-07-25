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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef GRAPHIC_TRIM_H
#define GRAPHIC_TRIM_H

#include <tools/graphic_edit.h>

class BOARD_ITEM;

namespace GRAPHIC_TRIM_PLANNER
{

/**
 * Plan removal of the part of aSource under aPointer.
 *
 * A cut past an end leaves one piece.  A middle cut leaves two, ordered along the source.
 * The caller creates the second item.
 */
GRAPHIC_EDIT_RESULT Plan( const BOARD_ITEM& aSource, const VECTOR2I& aPointer,
                          const std::vector<const BOARD_ITEM*>& aBoundaries );

}

#endif
