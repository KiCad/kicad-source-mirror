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

#ifndef DRILL_MARKERS_H
#define DRILL_MARKERS_H

#include <vector>

#include <math/vector2d.h>


/**
 * Drill marker outlines, as plain geometry.
 *
 * The plotter and the canvas painter both build their marks from here so a symbol on screen
 * and the same symbol in a plotted fabrication drawing cannot drift apart.
 */
namespace DRILL_MARKERS
{

/**
 * Total marks the pattern table can express.
 */
constexpr unsigned MARKER_COUNT = 58;


/**
 * One stroke of a mark.
 *
 * The primitive matters, not just the shape. The plotters emit a polygon differently from a
 * run of separate segments, so a mark that used to be a polygon has to stay one.
 */
struct MARKER_PART
{
    enum TYPE
    {
        SEGMENT,
        POLYLINE,
        CIRCLE
    };

    TYPE                  m_Type = SEGMENT;
    std::vector<VECTOR2I> m_Points;
    int                   m_Radius = 0;
};


/**
 * Marks that stay legible at chart size and in monochrome.
 *
 * The later patterns in the table stack up to three parts on top of each other and are hard
 * to tell apart on paper, so symbol assignment works through this subset first and falls
 * back to letters rather than reaching for them.
 */
int CuratedShapeCount();

/**
 * Pattern index for the nth curated mark.
 */
unsigned CuratedShape( int aIndex );

/**
 * Decompose one mark, in the order the parts must be drawn.
 *
 * An out-of-range shape id yields a plain circle, matching the plotter's long-standing
 * fallback.
 */
std::vector<MARKER_PART> BuildMarker( const VECTOR2I& aPosition, int aRadius, unsigned aShapeId );

} // namespace DRILL_MARKERS

#endif // DRILL_MARKERS_H
