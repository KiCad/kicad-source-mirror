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
 * TODO: add convex partitioning & spatial index
 */
class SHAPE_POLY_SET : public SHAPE
{
    public:
        ///> represents a single polygon outline with holes. The first entry is the outline,
        ///> the remaining (if any), are the holes
        typedef std::vector<SHAPE_LINE_CHAIN> POLYGON;

        /**
         * Class ITERATOR_TEMPLATE
         *
         * Base class for iterating over all vertices in a given SHAPE_POLY_SET
         */
        template <class T>
        class ITERATOR_TEMPLATE {
            public:

                bool IsEndContour() const
                {
                    return m_currentVertex + 1 == m_poly->CPolygon( m_currentOutline )[0].PointCount();
                }

                bool IsLastContour() const
                {
                    return m_currentOutline == m_lastOutline;
                }

                operator bool() const
                {
                    return m_currentOutline <= m_lastOutline;
                }

                void Advance()
                {
                    m_currentVertex ++;

                    if( m_currentVertex >= m_poly->CPolygon( m_currentOutline )[0].PointCount() )
                    {
                        m_currentVertex = 0;
                        m_currentOutline++;
                    }
                }

                void operator++( int dummy )
                {
                    Advance();
                }

                void operator++()
                {
                    Advance();
                }

                T& Get()
                {
                    return m_poly->Polygon( m_currentOutline )[0].Point( m_currentVertex );
                }

                T& operator*()
                {
                    return Get();
                }

                T* operator->()
                {
                    return &Get();
                }


            private:
                friend class SHAPE_POLY_SET;

                SHAPE_POLY_SET* m_poly;
                int m_currentOutline;
                int m_lastOutline;
                int m_currentVertex;
        };

        typedef ITERATOR_TEMPLATE<VECTOR2I> ITERATOR;
        typedef ITERATOR_TEMPLATE<const VECTOR2I> CONST_ITERATOR;

        SHAPE_POLY_SET();
        ~SHAPE_POLY_SET();

        ///> Creates a new empty polygon in the set and returns its index
        int NewOutline();

        ///> Creates a new hole in a given outline
        int NewHole( int aOutline = -1 );

        ///> Adds a new outline to the set and returns its index
        int AddOutline( const SHAPE_LINE_CHAIN& aOutline );

        ///> Adds a new hole to the given outline (default: last) and returns its index
        int AddHole( const SHAPE_LINE_CHAIN& aHole, int aOutline = -1 );

        ///> Appends a vertex at the end of the given outline/hole (default: the last outline)
        int Append( int x, int y, int aOutline = -1, int aHole = -1 );

        ///> Merges polygons from two sets.
        void Append( const SHAPE_POLY_SET& aSet );

        ///> Appends a vertex at the end of the given outline/hole (default: the last outline)
        void Append( const VECTOR2I& aP, int aOutline = -1, int aHole = -1 );

        ///> Returns the index-th vertex in a given hole outline within a given outline
        VECTOR2I& Vertex( int index, int aOutline = -1, int aHole = -1 );

        ///> Returns the index-th vertex in a given hole outline within a given outline
        const VECTOR2I& CVertex( int index, int aOutline = -1, int aHole = -1 ) const;

        ///> Returns true if any of the outlines is self-intersecting
        bool IsSelfIntersecting();

        ///> Returns the number of outlines in the set
        int OutlineCount() const { return m_polys.size(); }

        ///> Returns the number of vertices in a given outline/hole
        int VertexCount( int aOutline = -1, int aHole = -1 ) const;

        ///> Returns the number of holes in a given outline
        int HoleCount( int aOutline ) const;

        ///> Returns the reference to aIndex-th outline in the set
        SHAPE_LINE_CHAIN& Outline( int aIndex )
        {
            return m_polys[aIndex][0];
        }

        ///> Returns the reference to aHole-th hole in the aIndex-th outline
        SHAPE_LINE_CHAIN& Hole( int aOutline, int aHole )
        {
            return m_polys[aOutline][aHole + 1];
        }

        ///> Returns the aIndex-th subpolygon in the set
        POLYGON& Polygon( int aIndex )
        {
            return m_polys[aIndex];
        }

        const SHAPE_LINE_CHAIN& COutline( int aIndex ) const
        {
            return m_polys[aIndex][0];
        }

        const SHAPE_LINE_CHAIN& CHole( int aOutline, int aHole ) const
        {
            return m_polys[aOutline][aHole + 1];
        }

        const POLYGON& CPolygon( int aIndex ) const
        {
            return m_polys[aIndex];
        }

        ///> Returns an iterator object, for iterating between aFirst and aLast outline.
        ITERATOR Iterate( int aFirst, int aLast )
        {
            ITERATOR iter;

            iter.m_poly = this;
            iter.m_currentOutline = aFirst;
            iter.m_lastOutline = aLast < 0 ? OutlineCount() - 1 : aLast;
            iter.m_currentVertex = 0;

            return iter;
        }

        ///> Returns an iterator object, for iterating aOutline-th outline
        ITERATOR Iterate( int aOutline )
        {
            return Iterate( aOutline, aOutline );
        }

        ///> Returns an iterator object, for all outlines in the set (no holes)
        ITERATOR Iterate()
        {
            return Iterate( 0, OutlineCount() - 1 );
        }

        CONST_ITERATOR CIterate( int aFirst, int aLast ) const
        {
            CONST_ITERATOR iter;

            iter.m_poly = const_cast<SHAPE_POLY_SET*>( this );
            iter.m_currentOutline = aFirst;
            iter.m_lastOutline = aLast < 0 ? OutlineCount() - 1 : aLast;
            iter.m_currentVertex = 0;

            return iter;
        }

        CONST_ITERATOR CIterate( int aOutline ) const
        {
            return CIterate( aOutline, aOutline );
        }

        CONST_ITERATOR CIterate() const
        {
            return CIterate( 0, OutlineCount() - 1 );
        }


        ///> Performs boolean polyset union
        void BooleanAdd( const SHAPE_POLY_SET& b );

        ///> Performs boolean polyset difference
        void BooleanSubtract( const SHAPE_POLY_SET& b );

        ///> Performs outline inflation/deflation, using round corners.
        void Inflate ( int aFactor, int aCircleSegmentsCount = 32 );

        ///> Converts a set of polygons with holes to a singe outline with "slits"/"fractures" connecting the outer ring
        ///> to the inner holes
        void Fracture();

        ///> Converts a set of slitted polygons to a set of polygons with holes
        void Unfracture();

        ///> Returns true if the polygon set has any holes.
        bool HasHoles() const;

        ///> Simplifies the polyset (merges overlapping polys, eliminates degeneracy/self-intersections)
        void Simplify();

        /// @copydoc SHAPE::Format()
        const std::string Format() const;

        /// @copydoc SHAPE::Parse()
        bool Parse( std::stringstream& aStream );

        /// @copydoc SHAPE::Move()
        void Move( const VECTOR2I& aVector );

        /// @copydoc SHAPE::IsSolid()
        bool IsSolid() const
        {
            return true;
        }

        const BOX2I BBox( int aClearance = 0 ) const;

        // fixme: add collision support
        bool Collide( const VECTOR2I& aP, int aClearance = 0 ) const { return false; }
        bool Collide( const SEG& aSeg, int aClearance = 0 ) const { return false; }


        ///> Returns true is a given subpolygon contains the point aP. If aSubpolyIndex < 0 (default value),
        ///> checks all polygons in the set
        bool Contains( const VECTOR2I& aP, int aSubpolyIndex = -1 ) const;

        ///> Returns true if the set is empty (no polygons at all)
        bool IsEmpty() const
        {
            return m_polys.size() == 0;
        }

        ///> Removes all outlines & holes (clears) the polygon set.
        void RemoveAllContours();

        ///> Returns total number of vertices stored in the set.
        int TotalVertices() const;

        ///> Deletes aIdx-th polygon from the set
        void DeletePolygon( int aIdx );

    private:

        SHAPE_LINE_CHAIN& getContourForCorner( int aCornerId, int& aIndexWithinContour );
        VECTOR2I& vertex( int aCornerId );
        const VECTOR2I& cvertex( int aCornerId ) const;


        void fractureSingle( POLYGON& paths );
        void importTree( ClipperLib::PolyTree* tree );
        void booleanOp( ClipperLib::ClipType type, const SHAPE_POLY_SET& b );

        bool pointInPolygon( const VECTOR2I& aP, const SHAPE_LINE_CHAIN& aPath ) const;

        const ClipperLib::Path convertToClipper( const SHAPE_LINE_CHAIN& aPath, bool aRequiredOrientation );
        const SHAPE_LINE_CHAIN convertFromClipper( const ClipperLib::Path& aPath );

        typedef std::vector<POLYGON> Polyset;

        Polyset m_polys;
};

#endif
