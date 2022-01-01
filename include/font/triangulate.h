/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2021 Ola Rinta-Koski
 * Copyright (C) 2021-2022 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef TRIANGULATE_H
#define TRIANGULATE_H

#include <math/vector2d.h>
#include <font/glyph.h>
#include <geometry/shape_poly_set.h>
#include <functional>


typedef std::function<void( int, const VECTOR2I& aPoint1, const VECTOR2I& aPoint2,
                            const VECTOR2I& aPoint3, void* aCallbackData )>
        TRIANGULATE_CALLBACK;

void Triangulate( const SHAPE_POLY_SET& aPolylist, TRIANGULATE_CALLBACK aCallback,
                  void* aCallbackData = nullptr );

#endif // TRIANGULATE_H
