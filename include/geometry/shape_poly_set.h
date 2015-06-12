/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 CERN
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

#ifndef __SHAPE_POLY_SET_H
#define __SHAPE_POLY_SET_H

#include <vector>
#include <cstdio>
#include <geometry/shape.h>
#include <geometry/shape_line_chain.h>

#include "clipper.hpp"

/**
 * Class SHAPE_POLY_SET
 *
 * Represents a set of closed polygons. Polygons may be nonconvex, self-intersecting
 * and have holes. Provides boolean operations (using Clipper library as the backend).
 *
 * TODO: document, derive from class SHAPE, add convex partitioning & spatial index
 */
class SHAPE_POLY_SET : public SHAPE
{
    public:
        SHAPE_POLY_SET() : SHAPE (SH_POLY_SET) {};
        ~SHAPE_POLY_SET() {};

        ///> Creates a new empty polygon in the set and returns its index
        int NewOutline ();

        ///> Cretes a new empty hole in the given outline (default: last one) and returns its index
        int NewHole( int aOutline = -1);

        ///> Adds a new outline to the set and returns its index
        int AddOutline ( const SHAPE_LINE_CHAIN& aOutline );

        ///> Adds a new hole to the given outline (default: last) and returns its index
        int AddHole ( const SHAPE_LINE_CHAIN& aHole, int aOutline = -1 );

        ///> Appends a vertex at the end of the given outline/hole (default: last hole in the last outline)
        int AppendVertex ( int x, int y, int aOutline = -1, int aHole = -1 );

        ///> Returns the index-th vertex in a given hole outline within a given outline
        const VECTOR2I GetVertex ( int index, int aOutline = -1, int aHole = -1) const;

        ///> Returns true if any of the outlines is self-intersecting
        bool IsSelfIntersecting();

        ///> Returns the number of outlines in the set
        int OutlineCount() const { return m_polys.size(); }

        ///> Returns the number of vertices in a given outline/hole
        int VertexCount ( int aOutline = -1, int aHole = -1 ) const;

        ///> Returns the internal representation (ClipperLib) of a given polygon (outline + holes)
        const ClipperLib::Paths& GetPoly ( int aIndex ) const
        {
            return m_polys[aIndex];
        }

        ///> Performs boolean polyset difference
        void Subtract( const SHAPE_POLY_SET& b );

        ///> Performs boolean polyset union
        void Add( const SHAPE_POLY_SET& b );

        ///> Performs smooth outline inflation (Minkowski sum of the outline and a circle of a given radius)
        void SmoothInflate ( int aFactor );

        ///> Performs outline erosion/shrinking
        void Erode ( int aFactor );

        ///> Converts a set of polygons with holes to a singe outline with 'slits'/'fractures' connecting the outer ring
        ///> to the inner holes
        void Fracture ();

        ///> Simplifies the polyset (merges overlapping polys, eliminates degeneracy/self-intersections)
        void Simplify ();

        /// @copydoc SHAPE::Format()
        const std::string Format() const;

        /// @copydoc SHAPE::Parse()
        bool Parse( std::stringstream& aStream );

        void  Move( const VECTOR2I& aVector ) { assert(false ); };

        bool IsSolid() const
        {
            return true;
        }

        const BOX2I BBox( int aClearance = 0 ) const;

        bool Collide( const VECTOR2I& aP, int aClearance = 0 ) const { return false; }
        bool Collide( const SEG& aSeg, int aClearance = 0 ) const { return false; }

    private:

        void fractureSingle( ClipperLib::Paths& paths );
        void importTree ( ClipperLib::PolyTree* tree);
        void booleanOp( ClipperLib::ClipType type, const SHAPE_POLY_SET& b );

        const ClipperLib::Path convert( const SHAPE_LINE_CHAIN& aPath );

        typedef std::vector<ClipperLib::Paths> Polyset;

        Polyset m_polys;
};

#endif
