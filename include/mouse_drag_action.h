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

#ifndef MOUSE_DRAG_ACTION_H
#define MOUSE_DRAG_ACTION_H

enum class MOUSE_DRAG_ACTION
{
    // WARNING: these are encoded as integers in the file, so don't change their values.
    DRAG_ANY = -2,
    DRAG_SELECTED,
    SELECT,
    ZOOM,
    PAN,
    NONE
};

#endif // MOUSE_DRAG_ACTION_H
