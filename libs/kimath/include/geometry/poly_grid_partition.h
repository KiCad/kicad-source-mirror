/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2016-2017 CERN
 * @author Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef __POLY_GRID_PARTITION_H
#define __POLY_GRID_PARTITION_H


#include <algorithm>
#include <functional>
#include <set>
#include <unordered_map>
#include <vector>

#include <geometry/seg.h>
#include <geometry/shape_line_chain.h>
#include <geometry/shape_rect.h>
#include <math/vector2d.h>

/**
 * Class POLY_GRID_PARTITION
 *
 * Provides a fast test for point inside polygon.
 *
 * Takes a large poly and splits it into a grid of rectangular cells, forming a spatial hash table.
 * Each cell contains only the edges that 'touch it' (any point of the edge belongs to the cell).
 * Edges can be marked as leading or trailing. Leading edge indicates that space to the left of it (x-wise) is outside the polygon.
 * Trailing edge, conversely, means space to the right is outside the polygon.
 * The point inside check for point (p) works as follows:
 * - determine the cell coordinates of (p) (poly2grid)
 * - find the matching grid cell ( O(0), if the cell coordinates are outside the range, the point is not in the polygon )
 * - if the cell contains edges, find the first edge to the left or right of the point, whichever comes first.
 * - if the edge to the left is the 'lead edge', the point is inside. if it's a trailing edge, the point is outside.
 * - idem for the edge to the right of (p), just reverse the edge types
 * - if the cell doesn't contain any edges, scan horizontal cells to the left and right (switching sides with each iteration)
 *   until an edge if found.
 * NOTE: the rescale_trunc() function is used for grid<->world coordinate conversion because it rounds towards 0 (not to nearest)
 * It's important as rounding to nearest (which the standard rescale() function does) will shift the grid by half a cell.
 */

class POLY_GRID_PARTITION
{
public:
    POLY_GRID_PARTITION( const SHAPE_LINE_CHAIN& aPolyOutline, int gridSize );

    int ContainsPoint( const VECTOR2I& aP, int aClearance = 0 );

    const BOX2I& BBox() const
    {
        return m_bbox;
    }

private:
    enum HASH_FLAG
    {
        LEAD_EDGE  = 1,
        TRAIL_EDGE = 2,
    };

    using EDGE_LIST = std::vector<int>;

    template <class T>
    inline void hash_combine( std::size_t& seed, const T& v )
    {
        std::hash<T> hasher;
        seed ^= hasher( v ) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    }

    struct segsEqual
    {
        bool operator()( const SEG& a, const SEG& b ) const
        {
            return (a.A == b.A && a.B == b.B) || (a.A == b.B && a.B == b.A);
        }
    };

    struct segHash
    {
        std::size_t operator()(  const SEG& a ) const
        {
            return a.A.x + a.B.x + a.A.y + a.B.y;
        }
    };

    int containsPoint( const VECTOR2I& aP, bool debug = false ) const;

    bool checkClearance( const VECTOR2I& aP, int aClearance );

    int rescale_trunc( int aNumerator, int aValue, int aDenominator ) const;

    // convertes grid cell coordinates to the polygon coordinates
    const VECTOR2I grid2poly( const VECTOR2I& p ) const;

    int grid2polyX( int x ) const;

    int grid2polyY( int y ) const;

    const VECTOR2I poly2grid( const VECTOR2I& p ) const;

    int poly2gridX( int x ) const;

    int poly2gridY( int y ) const;

    void build( const SHAPE_LINE_CHAIN& aPolyOutline, int gridSize );

    bool inRange( int v1, int v2, int x ) const;

    struct SCAN_STATE
    {
        SCAN_STATE()
        {
            dist_prev   = INT_MAX;
            dist_max    = INT_MAX;
            nearest     = -1;
            nearest_prev = -1;
        };

        int dist_prev;
        int dist_max;
        int nearest_prev;
        int nearest;
    };

    void scanCell( SCAN_STATE& state, const EDGE_LIST& cell, const VECTOR2I& aP, int cx,
                   int cy ) const;

private:
    int                    m_gridSize;
    SHAPE_LINE_CHAIN       m_outline;
    BOX2I                  m_bbox;
    std::vector<int>       m_flags;
    std::vector<EDGE_LIST> m_grid;
};

#endif
