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

#include <plotters/drill_markers.h>

#include <trigo.h>


namespace DRILL_MARKERS
{

namespace
{
const unsigned char marker_patterns[MARKER_COUNT] = {

        // Bit order is O Square Lozenge - | \ /
        // Simple shapes first
        0003,  // X
        0100,  // O
        0014,  // +
        0040,  // Sq
        0020,  // Lz

        // Two simple shapes
        0103,  // X O
        0017,  // X +
        0043,  // X Sq
        0023,  // X Lz
        0114,  // O +
        0140,  // O Sq
        0120,  // O Lz
        0054,  // + Sq
        0034,  // + Lz
        0060,  // Sq Lz

        // Three simple shapes
        0117,  // X O +
        0143,  // X O Sq
        0123,  // X O Lz
        0057,  // X + Sq
        0037,  // X + Lz
        0063,  // X Sq Lz
        0154,  // O + Sq
        0134,  // O + Lz
        0074,  // + Sq Lz

        // Four simple shapes
        0174,  // O Sq Lz +
        0163,  // X O Sq Lz
        0157,  // X O Sq +
        0137,  // X O Lz +
        0077,  // X Sq Lz +

        // This draws *everything *
        0177,  // X O Sq Lz +

        // Here we use the single bars... so the cross is forbidden
        0110,  // O -
        0104,  // O |
        0101,  // O /
        0050,  // Sq -
        0044,  // Sq |
        0041,  // Sq /
        0030,  // Lz -
        0024,  // Lz |
        0021,  // Lz /
        0150,  // O Sq -
        0144,  // O Sq |
        0141,  // O Sq /
        0130,  // O Lz -
        0124,  // O Lz |
        0121,  // O Lz /
        0070,  // Sq Lz -
        0064,  // Sq Lz |
        0061,  // Sq Lz /
        0170,  // O Sq Lz -
        0164,  // O Sq Lz |
        0161,  // O Sq Lz /

        // The backlash component is the last resort, because it is easy to confound
        0102,  // \ O
        0042,  // \ Sq
        0022,  // \ Lz
        0142,  // \ O Sq
        0122,  // \ O Lz
        0062,  // \ Sq Lz
        0162   // \ O Sq Lz
    };

/// Single-part and two-part marks only. The three-part ones are too easily confounded at
/// chart size
const unsigned curated[] = { 0, 1, 2, 3, 4, 5, 6, 8, 9, 10, 12, 16 };


void addSegment( std::vector<MARKER_PART>& aParts, const VECTOR2I& aStart, const VECTOR2I& aEnd )
{
    MARKER_PART part;
    part.m_Type = MARKER_PART::SEGMENT;
    part.m_Points = { aStart, aEnd };
    aParts.push_back( part );
}


void addPolyline( std::vector<MARKER_PART>& aParts, std::vector<VECTOR2I> aPoints )
{
    MARKER_PART part;
    part.m_Type = MARKER_PART::POLYLINE;
    part.m_Points = std::move( aPoints );
    aParts.push_back( part );
}

} // namespace


int CuratedShapeCount()
{
    return static_cast<int>( sizeof( curated ) / sizeof( curated[0] ) );
}


unsigned CuratedShape( int aIndex )
{
    if( aIndex < 0 || aIndex >= CuratedShapeCount() )
        return 0;

    return curated[aIndex];
}


std::vector<MARKER_PART> BuildMarker( const VECTOR2I& aPosition, int aRadius, unsigned aShapeId )
{
    std::vector<MARKER_PART> parts;

    auto addCircle =
            [&]()
            {
                MARKER_PART part;
                part.m_Type = MARKER_PART::CIRCLE;
                part.m_Radius = aRadius;
                parts.push_back( part );
            };

    if( aShapeId >= MARKER_COUNT )
    {
        addCircle();
        return parts;
    }

    const unsigned char pat = marker_patterns[aShapeId];
    const int           r = aRadius;

    if( pat & 0001 )
    {
        addSegment( parts, VECTOR2I( aPosition.x - r, aPosition.y - r ),
                    VECTOR2I( aPosition.x + r, aPosition.y + r ) );
    }

    if( pat & 0002 )
    {
        addSegment( parts, VECTOR2I( aPosition.x + r, aPosition.y - r ),
                    VECTOR2I( aPosition.x - r, aPosition.y + r ) );
    }

    if( pat & 0004 )
    {
        addSegment( parts, VECTOR2I( aPosition.x, aPosition.y - r ),
                    VECTOR2I( aPosition.x, aPosition.y + r ) );
    }

    if( pat & 0010 )
    {
        addSegment( parts, VECTOR2I( aPosition.x - r, aPosition.y ),
                    VECTOR2I( aPosition.x + r, aPosition.y ) );
    }

    if( pat & 0020 )
    {
        addPolyline( parts, { VECTOR2I( aPosition.x, aPosition.y + r ),
                              VECTOR2I( aPosition.x + r, aPosition.y ),
                              VECTOR2I( aPosition.x, aPosition.y - r ),
                              VECTOR2I( aPosition.x - r, aPosition.y ),
                              VECTOR2I( aPosition.x, aPosition.y + r ) } );
    }

    if( pat & 0040 )
    {
        // The square is inscribed in the marker circle, so its half-side is r/sqrt(2)
        const int s = KiROUND( r / 1.4142 );

        addPolyline( parts, { VECTOR2I( aPosition.x + s, aPosition.y + s ),
                              VECTOR2I( aPosition.x + s, aPosition.y - s ),
                              VECTOR2I( aPosition.x - s, aPosition.y - s ),
                              VECTOR2I( aPosition.x - s, aPosition.y + s ),
                              VECTOR2I( aPosition.x + s, aPosition.y + s ) } );
    }

    if( pat & 0100 )
        addCircle();

    return parts;
}

} // namespace DRILL_MARKERS
