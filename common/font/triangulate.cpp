/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2021 Ola Rinta-Koski
 * Copyright (C) 2021 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <limits>
#include <font/triangulate.h>

void Triangulate( const SHAPE_POLY_SET& aPolylist, TRIANGULATE_CALLBACK aCallback,
                  void* aCallbackData )
{
    SHAPE_POLY_SET polys( aPolylist );

    polys.Fracture( SHAPE_POLY_SET::PM_FAST ); // TODO verify aFastMode
    polys.CacheTriangulation();

    for( unsigned int i = 0; i < polys.TriangulatedPolyCount(); i++ )
    {
        const SHAPE_POLY_SET::TRIANGULATED_POLYGON* polygon = polys.TriangulatedPolygon( i );
        for ( size_t j = 0; j < polygon->GetTriangleCount(); j++ )
        {
            VECTOR2I a;
            VECTOR2I b;
            VECTOR2I c;

            polygon->GetTriangle( j, a, b, c );
            aCallback( i, a, b, c, aCallbackData );
        }
    }
}


