/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef FILL_TYPE_H
#define FILL_TYPE_H

/**
 * The set of fill types used in plotting or drawing enclosed areas.
 *
 * @warning Do not renumber this enum, the legacy schematic plugin demands on these values.
 */
enum class FILL_TYPE : int
{
    NO_FILL = 0,
    FILLED_SHAPE = 1,             // Fill with object color ("Solid shape")
    FILLED_WITH_BG_BODYCOLOR = 2, // Fill with background body color
                                  //   (not filled in B&W mode when plotting or printing)
    FILLED_WITH_COLOR =3          // Fill with a user-defined color (currently sheets only)
};

#endif
