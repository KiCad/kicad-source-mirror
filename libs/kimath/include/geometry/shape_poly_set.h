/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015-2019 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * @author Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
 * @author Alejandro García Montoro <alejandro.garciamontoro@gmail.com>
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

#include <atomic>
#include <cstdio>
#include <deque>                        // for deque
#include <iosfwd>                       // for string, stringstream
#include <memory>
#include <mutex>
#include <set>                          // for set
#include <stdexcept>                    // for out_of_range
#include <stdlib.h>                     // for abs
#include <vector>

#include <clipper2/clipper.h>
#include <core/mirror.h>                // for FLIP_DIRECTION
#include <geometry/corner_strategy.h>
#include <geometry/seg.h>               // for SEG
#include <geometry/shape.h>
#include <geometry/shape_line_chain.h>
#include <math/box2.h>                  // for BOX2I
#include <math/vector2d.h>              // for VECTOR2I
#include <hash_128.h>


/**
 * Represent a set of closed polygons. Polygons may be nonconvex, self-intersecting
 * and have holes. Provides boolean operations (using Clipper library as the backend).
 *
 * Let us define the terms used on this class to clarify methods names and comments:
 *      - Polygon: each polygon in the set.
 *      - Outline: first polyline in each polygon; represents its outer contour.
 *      - Hole: second and following polylines in the polygon.
 *      - Contour: each polyline of each polygon in the set, whether or not it is an
 *      outline or a hole.
 *      - Vertex (or corner): each one of the points that define a contour.
 *
 * TODO: add convex partitioning & spatial index
 */
class SHAPE_POLY_SET : public SHAPE
{
public:
    /// represents a single polygon outline with holes. The first entry is the outline,
    /// the remaining (if any), are the holes
    /// N.B. SWIG only supports typedef, so avoid c++ 'using' keyword
    typedef std::vector<SHAPE_LINE_CHAIN> POLYGON;

    class TRIANGULATED_POLYGON
    {
    public:
        struct TRI : public SHAPE_LINE_CHAIN_BASE
        {
            TRI( int _a = 0, int _b = 0, int _c = 0, TRIANGULATED_POLYGON* aParent = nullptr ) :
                SHAPE_LINE_CHAIN_BASE( SH_POLY_SET_TRIANGLE ),
                a( _a ),
                b( _b ),
                c( _c ),
                parent( aParent )
            {
            }

            virtual void Rotate( const EDA_ANGLE& aAngle,
                                 const VECTOR2I& aCenter = { 0, 0 } ) override {};

            virtual void Move( const VECTOR2I& aVector ) override {};

            virtual bool IsSolid() const override { return true; }

            virtual bool IsClosed() const override { return true; }

            virtual const BOX2I BBox( int aClearance = 0 ) const override;

            virtual const VECTOR2I GetPoint( int aIndex ) const override
            {
                switch(aIndex)
                {
                    case 0: return parent->m_vertices[a];
                    case 1: return parent->m_vertices[b];
                    case 2: return parent->m_vertices[c];
                    default: wxCHECK( false, VECTOR2I() );
                }
            }

            virtual const SEG GetSegment( int aIndex ) const override
            {
                switch(aIndex)
                {
                    case 0: return SEG( parent->m_vertices[a], parent->m_vertices[b] );
                    case 1: return SEG( parent->m_vertices[b], parent->m_vertices[c] );
                    case 2: return SEG( parent->m_vertices[c], parent->m_vertices[a] );
                    default: wxCHECK( false, SEG() );
                }
            }

            virtual size_t GetPointCount() const override { return 3; }
            virtual size_t GetSegmentCount() const override { return 3; }

            double Area() const
            {
                VECTOR2I& aa = parent->m_vertices[a];
                VECTOR2I& bb = parent->m_vertices[b];
                VECTOR2I& cc = parent->m_vertices[c];

                VECTOR2D ba = bb - aa;
                VECTOR2D cb = cc - bb;

                return std::abs( cb.Cross( ba ) * 0.5 );
            }

            int                   a;
            int                   b;
            int                   c;
            TRIANGULATED_POLYGON* parent;
        };

        TRIANGULATED_POLYGON( int aSourceOutline );
        TRIANGULATED_POLYGON( const TRIANGULATED_POLYGON& aOther );
        ~TRIANGULATED_POLYGON();

        void Clear()
        {
            m_vertices.clear();
            m_triangles.clear();
        }

        void GetTriangle( int index, VECTOR2I& a, VECTOR2I& b, VECTOR2I& c ) const
        {
            auto& tri = m_triangles[ index ];
            a = m_vertices[ tri.a ];
            b = m_vertices[ tri.b ];
            c = m_vertices[ tri.c ];
        }

        TRIANGULATED_POLYGON& operator=( const TRIANGULATED_POLYGON& aOther );

        // Move assignment operator.
        TRIANGULATED_POLYGON& operator=( TRIANGULATED_POLYGON&& aOther ) noexcept
        {
            if( this != &aOther )
            {
                m_sourceOutline = aOther.m_sourceOutline;
                m_triangles = std::move( aOther.m_triangles );
                m_vertices = std::move( aOther.m_vertices );

                for( TRI& tri : m_triangles )
                    tri.parent = this;
            }

            return *this;
        }

        void AddTriangle( int a, int b, int c );

        void AddVertex( const VECTOR2I& aP )
        {
            m_vertices.push_back( aP );
        }

        size_t GetTriangleCount() const { return m_triangles.size(); }

        int GetSourceOutlineIndex() const { return m_sourceOutline; }
        void SetSourceOutlineIndex( int aIndex ) { m_sourceOutline = aIndex; }

        const std::deque<TRI>& Triangles() const { return m_triangles; }
        void SetTriangles( const std::deque<TRI>& aTriangles )
        {
            m_triangles.resize( aTriangles.size() );

            for( size_t ii = 0; ii < aTriangles.size(); ii++ )
            {
                m_triangles[ii] = aTriangles[ii];
                m_triangles[ii].parent = this;
            }
        }

        const std::deque<VECTOR2I>& Vertices() const { return m_vertices; }
        void SetVertices( const std::deque<VECTOR2I>& aVertices )
        {
            m_vertices = aVertices;
        }

        size_t GetVertexCount() const
        {
            return m_vertices.size();
        }

        void Move( const VECTOR2I& aVec )
        {
            for( VECTOR2I& vertex : m_vertices )
                vertex += aVec;
        }

    private:
        int                  m_sourceOutline;
        std::deque<TRI>      m_triangles;
        std::deque<VECTOR2I> m_vertices;
    };

    /**
     * Structure to hold the necessary information in order to index a vertex on a
     * SHAPE_POLY_SET object: the polygon index, the contour index relative to the polygon and
     * the vertex index relative the contour.
     */
    struct VERTEX_INDEX
    {
        int m_polygon;   /*!< m_polygon is the index of the polygon. */
        int m_contour;   /*!< m_contour is the index of the contour relative to the polygon. */
        int m_vertex;    /*!< m_vertex is the index of the vertex relative to the contour. */

        VERTEX_INDEX() :
            m_polygon(-1),
            m_contour(-1),
            m_vertex(-1)
        {
        }
    };

    /**
     * Base class for iterating over all vertices in a given SHAPE_POLY_SET.
     */
    template <class T>
    class ITERATOR_TEMPLATE
    {
    public:

        /**
         * @return true if the current vertex is the last one of the current contour
         *         (outline or hole); false otherwise.
         */
        bool IsEndContour() const
        {
            return m_currentVertex + 1 == m_poly->CPolygon( m_currentPolygon )[m_currentContour].PointCount();
        }

        /**
         * @return true if the current outline is the last one; false otherwise.
         */
        bool IsLastPolygon() const
        {
            return m_currentPolygon == m_lastPolygon;
        }

        operator bool() const
        {
            if( m_currentPolygon < m_lastPolygon )
                return true;

            if( m_currentPolygon != m_poly->OutlineCount() - 1 )
                return false;

            const auto& currentPolygon = m_poly->CPolygon( m_currentPolygon );

            return m_currentContour < (int) currentPolygon.size() - 1
                       || m_currentVertex < currentPolygon[m_currentContour].PointCount();
        }

        /**
         * Advance the indices of the current vertex/outline/contour, checking whether the
         * vertices in the holes have to be iterated through.
         */
        void Advance()
        {
            // Advance vertex index
            m_currentVertex ++;

            // Check whether the user wants to iterate through the vertices of the holes
            // and behave accordingly
            if( m_iterateHoles )
            {
                // If the last vertex of the contour was reached, advance the contour index
                if( m_currentVertex >= m_poly->CPolygon( m_currentPolygon )[m_currentContour].PointCount() )
                {
                    m_currentVertex = 0;
                    m_currentContour++;

                    // If the last contour of the current polygon was reached, advance the
                    // outline index
                    int totalContours = m_poly->CPolygon( m_currentPolygon ).size();

                    if( m_currentContour >= totalContours )
                    {
                        m_currentContour = 0;
                        m_currentPolygon++;
                    }
                }
            }
            else
            {
                // If the last vertex of the outline was reached, advance to the following polygon
                if( m_currentVertex >= m_poly->CPolygon( m_currentPolygon )[0].PointCount() )
                {
                    m_currentVertex = 0;
                    m_currentPolygon++;
                }
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

        const T& Get()
        {
            return m_poly->Polygon( m_currentPolygon )[m_currentContour].CPoint( m_currentVertex );
        }

        const T& operator*()
        {
            return Get();
        }

        const T* operator->()
        {
            return &Get();
        }

        /**
         * @return the indices of the current polygon, contour and vertex.
         */
        VERTEX_INDEX GetIndex()
        {
            VERTEX_INDEX index;

            index.m_polygon = m_currentPolygon;
            index.m_contour = m_currentContour;
            index.m_vertex = m_currentVertex;

            return index;
        }

    private:
        friend class SHAPE_POLY_SET;

        SHAPE_POLY_SET* m_poly;
        int             m_currentPolygon;
        int             m_currentContour;
        int             m_currentVertex;
        int             m_lastPolygon;
        bool            m_iterateHoles;
    };

    /**
     * Base class for iterating over all segments in a given SHAPE_POLY_SET.
     */
    template <class T>
    class SEGMENT_ITERATOR_TEMPLATE
    {
    public:
        /**
         * @return true if the current outline is the last one.
         */
        bool IsLastPolygon() const
        {
            return m_currentPolygon == m_lastPolygon;
        }

        operator bool() const
        {
            return m_currentPolygon <= m_lastPolygon;
        }

        /**
         * Advance the indices of the current vertex/outline/contour, checking whether the
         * vertices in the holes have to be iterated through.
         */
        void Advance()
        {
            // Advance vertex index
            m_currentSegment++;
            int last;

            // Check whether the user wants to iterate through the vertices of the holes
            // and behave accordingly.
            if( m_iterateHoles )
            {
                last = m_poly->CPolygon( m_currentPolygon )[m_currentContour].SegmentCount();

                // If the last vertex of the contour was reached, advance the contour index.
                if( m_currentSegment >= last )
                {
                    m_currentSegment = 0;
                    m_currentContour++;

                    // If the last contour of the current polygon was reached, advance the
                    // outline index.
                    int totalContours = m_poly->CPolygon( m_currentPolygon ).size();

                    if( m_currentContour >= totalContours )
                    {
                        m_currentContour = 0;
                        m_currentPolygon++;
                    }
                }
            }
            else
            {
                last = m_poly->CPolygon( m_currentPolygon )[0].SegmentCount();
                // If the last vertex of the outline was reached, advance to the following
                // polygon
                if( m_currentSegment >= last )
                {
                    m_currentSegment = 0;
                    m_currentPolygon++;
                }
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

        T Get()
        {
            return m_poly->Polygon( m_currentPolygon )[m_currentContour].Segment( m_currentSegment );
        }

        T operator*()
        {
            return Get();
        }

        /**
         * @return the indices of the current polygon, contour and vertex.
         */
        VERTEX_INDEX GetIndex() const
        {
            VERTEX_INDEX index;

            index.m_polygon = m_currentPolygon;
            index.m_contour = m_currentContour;
            index.m_vertex = m_currentSegment;

            return index;
        }

        /**
         * @param aOther is an iterator pointing to another segment.
         * @return true if both iterators point to the same segment of the same contour of
         *         the same polygon of the same polygon set; false otherwise.
         */
        bool IsAdjacent( SEGMENT_ITERATOR_TEMPLATE<T> aOther ) const
        {
            // Check that both iterators point to the same contour of the same polygon of the
            // same polygon set.
            if( m_poly == aOther.m_poly && m_currentPolygon == aOther.m_currentPolygon &&
                m_currentContour == aOther.m_currentContour )
            {
                // Compute the total number of segments.
                int numSeg;
                numSeg = m_poly->CPolygon( m_currentPolygon )[m_currentContour].SegmentCount();

                // Compute the difference of the segment indices. If it is exactly one, they
                // are adjacent. The only missing case where they also are adjacent is when
                // the segments are the first and last one, in which case the difference
                // always equals the total number of segments minus one.
                int indexDiff = std::abs( m_currentSegment - aOther.m_currentSegment );

                return ( indexDiff == 1 ) || ( indexDiff == (numSeg - 1) );
            }

            return false;
        }

    private:
        friend class SHAPE_POLY_SET;

        SHAPE_POLY_SET* m_poly;
        int             m_currentPolygon;
        int             m_currentContour;
        int             m_currentSegment;
        int             m_lastPolygon;
        bool            m_iterateHoles;
    };

    // Iterator and const iterator types to visit polygon's points.
    typedef ITERATOR_TEMPLATE<VECTOR2I> ITERATOR;
    typedef ITERATOR_TEMPLATE<const VECTOR2I> CONST_ITERATOR;

    // Iterator and const iterator types to visit polygon's edges.
    typedef SEGMENT_ITERATOR_TEMPLATE<SEG> SEGMENT_ITERATOR;
    typedef SEGMENT_ITERATOR_TEMPLATE<const SEG> CONST_SEGMENT_ITERATOR;

    SHAPE_POLY_SET();

    SHAPE_POLY_SET( const BOX2D& aRect );

    /**
     * Construct a SHAPE_POLY_SET with the first outline given by aOutline.
     *
     * @param aOutline is a closed outline
     */
    SHAPE_POLY_SET( const SHAPE_LINE_CHAIN& aOutline );

    /**
     * Construct a SHAPE_POLY_SET with the first polygon given by aPolygon.
     *
     * @param aPolygon is a polygon
     */
    SHAPE_POLY_SET( const POLYGON& aPolygon );

    /**
     * Copy constructor SHAPE_POLY_SET
     * Performs a deep copy of \p aOther into \p this.
     *
     * @param aOther is the SHAPE_POLY_SET object that will be copied.
     */
    SHAPE_POLY_SET( const SHAPE_POLY_SET& aOther );

    ~SHAPE_POLY_SET();

    SHAPE_POLY_SET& operator=( const SHAPE_POLY_SET& aOther );

    // Move assignment operator
    SHAPE_POLY_SET& operator=( SHAPE_POLY_SET&& aOther ) noexcept
    {
        if (this != &aOther)
        {
            SHAPE::operator=( aOther );

            m_polys = std::move( aOther.m_polys );
            m_triangulatedPolys = std::move( aOther.m_triangulatedPolys );

            m_hash = aOther.m_hash;
            m_hashValid = aOther.m_hashValid;
            m_triangulationValid.store( aOther.m_triangulationValid );
        }

        return *this;
    }

    /**
     * Build a polygon triangulation, needed to draw a polygon on OpenGL and in some
     * other calculations
     * @param aPartition = true to created a trinagulation in a partition on a grid
     * false to create a more basic triangulation of the polygons
     * Note
     * in partition calculations the grid size is hard coded to 1e7.
     * This is a good value for Pcbnew: 1cm, in internal units.
     * But not good for Gerbview (1e7 = 10cm), however using a partition is not useful.
     * @param aSimplify = force the algorithm to simplify the POLY_SET before triangulating
     */
    virtual void CacheTriangulation( bool aPartition = true, bool aSimplify = false )
    {
        cacheTriangulation( aPartition, aSimplify, nullptr );
    }
    bool IsTriangulationUpToDate() const;

    HASH_128 GetHash() const;

    virtual bool HasIndexableSubshapes() const override;

    virtual size_t GetIndexableSubshapeCount() const override;

    virtual void GetIndexableSubshapes( std::vector<const SHAPE*>& aSubshapes ) const override;

    /**
     * Convert a global vertex index ---i.e., a number that globally identifies a vertex in a
     * concatenated list of all vertices in all contours--- and get the index of the vertex
     * relative to the contour relative to the polygon in which it is.
     *
     * @param  aGlobalIdx is the global index of the corner whose structured index wants to
     *                    be found
     * @param  aRelativeIndices is a pointer to the set of relative indices to store.
     * @return true if the global index is correct and the information in \a aRelativeIndices
     *         is valid; false otherwise.
     */
    bool GetRelativeIndices( int aGlobalIdx, VERTEX_INDEX* aRelativeIndices ) const;

    /**
     * Compute the global index of a vertex from the relative indices of polygon, contour and
     * vertex.
     *
     * @param  aRelativeIndices is the set of relative indices.
     * @param  aGlobalIdx [out] is the computed global index.
     * @return true if the relative indices are correct; false otherwise. The computed
     *         global index is returned in the \p aGlobalIdx reference.
     */
    bool GetGlobalIndex( VERTEX_INDEX aRelativeIndices, int& aGlobalIdx ) const;

    /// @copydoc SHAPE::Clone()
    SHAPE* Clone() const override;

    SHAPE_POLY_SET CloneDropTriangulation() const;

    /// Creates a new empty polygon in the set and returns its index
    int NewOutline();

    /// Creates a new hole in a given outline
    int NewHole( int aOutline = -1 );

    /// Adds a new outline to the set and returns its index
    int AddOutline( const SHAPE_LINE_CHAIN& aOutline );

    /// Adds a new hole to the given outline (default: last) and returns its index
    int AddHole( const SHAPE_LINE_CHAIN& aHole, int aOutline = -1 );

    /// Adds a polygon to the set
    int AddPolygon( const POLYGON& apolygon );

    /// Return the area of this poly set
    double Area();

    /// Count the number of arc shapes present
    int ArcCount() const;

    /// Appends all the arcs in this polyset to \a aArcBuffer
    void GetArcs( std::vector<SHAPE_ARC>& aArcBuffer ) const;

    /// Removes all arc references from all the outlines and holes in the polyset
    void ClearArcs();

    /// Appends a vertex at the end of the given outline/hole (default: the last outline)
    /**
     * Add a new vertex to the contour indexed by \p aOutline and \p aHole (defaults to the
     * outline of the last polygon).
     *
     * @param  x                 is the x coordinate of the new vertex.
     * @param  y                 is the y coordinate of the new vertex.
     * @param  aOutline          is the index of the polygon.
     * @param  aHole             is the index of the hole (-1 for the main outline),
     * @param  aAllowDuplication is a flag to indicate whether it is allowed to add this
     *                           corner even if it is duplicated.
     * @return the number of corners of the selected contour after the addition.
     */
    int Append( int x, int y, int aOutline = -1, int aHole = -1, bool aAllowDuplication = false );

    /// Merge polygons from two sets.
    void Append( const SHAPE_POLY_SET& aSet );

    /// Append a vertex at the end of the given outline/hole (default: the last outline)
    void Append( const VECTOR2I& aP, int aOutline = -1, int aHole = -1 );

    /**
     * Append a new arc to the contour indexed by \p aOutline and \p aHole (defaults to the
     * outline of the last polygon).
     * @param aArc      The arc to be inserted
     * @param aOutline  Index of the polygon
     * @param aHole     Index of the hole (-1 for the main outline)
     * @param aMaxError optional; accuracy of the arc representation in IU
     * @return the number of points in the arc (including the interpolated points from the arc)
     */
    int Append( const SHAPE_ARC& aArc, int aOutline = -1, int aHole = -1,
                std::optional<int> aMaxError = {} );

    /**
     * Adds a vertex in the globally indexed position \a aGlobalIndex.
     *
     * @param aGlobalIndex is the global index of the position in which the new vertex will be
     *                     inserted.
     * @param aNewVertex   is the new inserted vertex.
     */
    void InsertVertex( int aGlobalIndex, const VECTOR2I& aNewVertex );

    /// Return the index-th vertex in a given hole outline within a given outline
    const VECTOR2I& CVertex( int aIndex, int aOutline, int aHole ) const;

    /// Return the aGlobalIndex-th vertex in the poly set
    const VECTOR2I& CVertex( int aGlobalIndex ) const;

    /// Return the index-th vertex in a given hole outline within a given outline
    const VECTOR2I& CVertex( VERTEX_INDEX aIndex ) const;

    /**
     * Return the global indexes of the previous and the next corner of the \a aGlobalIndex-th
     * corner of a contour in the polygon set.
     *
     * They are often aGlobalIndex-1 and aGlobalIndex+1, but not for the first and last
     * corner of the contour.
     *
     * @param  aGlobalIndex is index of the corner, globally indexed between all edges in all
     *                      contours
     * @param aPrevious is the globalIndex of the previous corner of the same contour.
     * @param aNext is the globalIndex of the next corner of the same contour.
     * @return true if OK, false if aGlobalIndex is out of range
     */
    bool GetNeighbourIndexes( int aGlobalIndex, int* aPrevious, int* aNext ) const;

    /**
     * Check whether the aPolygonIndex-th polygon in the set is self intersecting.
     *
     * @param aPolygonIndex is the index of the polygon that wants to be checked.
     * @return true if the \a aPolygonIndex-th polygon is self intersecting, false otherwise.
     */
    bool IsPolygonSelfIntersecting( int aPolygonIndex ) const;

    /**
     * Check whether any of the polygons in the set is self intersecting.
     *
     * @return true if any of the polygons is self intersecting, false otherwise.
     */
    bool IsSelfIntersecting() const;

    /// Return the number of triangulated polygons
    unsigned int TriangulatedPolyCount() const { return m_triangulatedPolys.size(); }

    /// Return the number of outlines in the set
    int OutlineCount() const { return m_polys.size(); }

    /// Return the number of vertices in a given outline/hole
    int VertexCount( int aOutline = -1, int aHole = -1 ) const;

    /// Return the number of points in the shape poly set.
    /// mainly for reports
    int FullPointCount() const;

    /// Returns the number of holes in a given outline
    int HoleCount( int aOutline ) const
    {
        if( aOutline < 0 || aOutline >= (int) m_polys.size() || m_polys[aOutline].size() < 2 )
            return 0;

        // the first polygon in m_polys[aOutline] is the main contour,
        // only others are holes:
        return m_polys[aOutline].size() - 1;
    }

    /// Return the reference to aIndex-th outline in the set
    SHAPE_LINE_CHAIN& Outline( int aIndex )
    {
        return m_polys[aIndex][0];
    }

    const SHAPE_LINE_CHAIN& Outline( int aIndex ) const
    {
        return m_polys[aIndex][0];
    }

    /**
     * Return a subset of the polygons in this set, the ones between \a aFirstPolygon and
     * \a aLastPolygon.
     *
     * @param  aFirstPolygon is the first polygon to be included in the returned set.
     * @param  aLastPolygon is the last polygon to be excluded of the returned set.
     * @return a set containing the polygons between \a aFirstPolygon (included)
     *         and \a aLastPolygon (excluded).
     */
    SHAPE_POLY_SET Subset( int aFirstPolygon, int aLastPolygon );

    SHAPE_POLY_SET UnitSet( int aPolygonIndex )
    {
        return Subset( aPolygonIndex, aPolygonIndex + 1 );
    }

    /// Return the reference to aHole-th hole in the aIndex-th outline
    SHAPE_LINE_CHAIN& Hole( int aOutline, int aHole )
    {
        return m_polys[aOutline][aHole + 1];
    }

    /// Return the aIndex-th subpolygon in the set
    POLYGON& Polygon( int aIndex )
    {
        return m_polys[aIndex];
    }

    const POLYGON& Polygon( int aIndex ) const
    {
        return m_polys[aIndex];
    }

    const TRIANGULATED_POLYGON* TriangulatedPolygon( int aIndex ) const
    {
        return m_triangulatedPolys[aIndex].get();
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

    const std::vector<POLYGON>& CPolygons() const { return m_polys; }

    /**
     * Return an object to iterate through the points of the polygons between \p aFirst and
     * \p aLast.
     *
     * @param  aFirst        is the first polygon whose points will be iterated.
     * @param  aLast         is the last polygon whose points will be iterated.
     * @param  aIterateHoles is a flag to indicate whether the points of the holes should be
     *                       iterated.
     * @return ITERATOR - the iterator object.
     */
    ITERATOR Iterate( int aFirst, int aLast, bool aIterateHoles = false )
    {
        ITERATOR iter;

        iter.m_poly = this;
        iter.m_currentPolygon = aFirst;
        iter.m_lastPolygon = aLast < 0 ? OutlineCount() - 1 : aLast;
        iter.m_currentContour = 0;
        iter.m_currentVertex = 0;
        iter.m_iterateHoles = aIterateHoles;

        return iter;
    }

    /**
     * @param aOutline is the index of the polygon to be iterated.
     * @return an iterator object to visit all points in the main outline of the
     *         \a aOutline-th polygon, without visiting the points in the holes.
     */
    ITERATOR Iterate( int aOutline )
    {
        return Iterate( aOutline, aOutline );
    }

    /**
     * @param aOutline the index of the polygon to be iterated.
     * @return an iterator object to visit all points in the main outline of the
     *         \a aOutline-th polygon, visiting also the points in the holes.
     */
    ITERATOR IterateWithHoles( int aOutline )
    {
        return Iterate( aOutline, aOutline, true );
    }

    /**
     * @return an iterator object to visit all points in all outlines of the set,
     *         without visiting the points in the holes.
     */
    ITERATOR Iterate()
    {
        return Iterate( 0, OutlineCount() - 1 );
    }

    /**
     * @return an iterator object to visit all points in all outlines of the set,
     *         visiting also the points in the holes.
     */
    ITERATOR IterateWithHoles()
    {
        return Iterate( 0, OutlineCount() - 1, true );
    }


    CONST_ITERATOR CIterate( int aFirst, int aLast, bool aIterateHoles = false ) const
    {
        CONST_ITERATOR iter;

        iter.m_poly = const_cast<SHAPE_POLY_SET*>( this );
        iter.m_currentPolygon = aFirst;
        iter.m_lastPolygon = aLast < 0 ? OutlineCount() - 1 : aLast;
        iter.m_currentContour = 0;
        iter.m_currentVertex = 0;
        iter.m_iterateHoles = aIterateHoles;

        return iter;
    }

    CONST_ITERATOR CIterate( int aOutline ) const
    {
        return CIterate( aOutline, aOutline );
    }

    CONST_ITERATOR CIterateWithHoles( int aOutline ) const
    {
        return CIterate( aOutline, aOutline, true );
    }

    CONST_ITERATOR CIterate() const
    {
        return CIterate( 0, OutlineCount() - 1 );
    }

    CONST_ITERATOR CIterateWithHoles() const
    {
        return CIterate( 0, OutlineCount() - 1, true );
    }

    ITERATOR IterateFromVertexWithHoles( int aGlobalIdx )
    {
        // Build iterator
        ITERATOR iter = IterateWithHoles();

        // Get the relative indices of the globally indexed vertex
        VERTEX_INDEX indices;

        if( !GetRelativeIndices( aGlobalIdx, &indices ) )
            throw( std::out_of_range( "aGlobalIndex-th vertex does not exist" ) );

        // Adjust where the iterator is pointing
        iter.m_currentPolygon = indices.m_polygon;
        iter.m_currentContour = indices.m_contour;
        iter.m_currentVertex = indices.m_vertex;

        return iter;
    }

    /// Return an iterator object, for iterating between aFirst and aLast outline, with or
    /// without holes (default: without)
    SEGMENT_ITERATOR IterateSegments( int aFirst, int aLast, bool aIterateHoles = false )
    {
        SEGMENT_ITERATOR iter;

        iter.m_poly = this;
        iter.m_currentPolygon = aFirst;
        iter.m_lastPolygon = aLast < 0 ? OutlineCount() - 1 : aLast;
        iter.m_currentContour = 0;
        iter.m_currentSegment = 0;
        iter.m_iterateHoles = aIterateHoles;

        return iter;
    }

    /// Return an iterator object, for iterating between aFirst and aLast outline, with or
    /// without holes (default: without)
    CONST_SEGMENT_ITERATOR CIterateSegments( int aFirst, int aLast,
                                             bool aIterateHoles = false ) const
    {
        CONST_SEGMENT_ITERATOR iter;

        iter.m_poly = const_cast<SHAPE_POLY_SET*>( this );
        iter.m_currentPolygon = aFirst;
        iter.m_lastPolygon = aLast < 0 ? OutlineCount() - 1 : aLast;
        iter.m_currentContour = 0;
        iter.m_currentSegment = 0;
        iter.m_iterateHoles = aIterateHoles;

        return iter;
    }

    /// Return an iterator object, for iterating aPolygonIdx-th polygon edges.
    SEGMENT_ITERATOR IterateSegments( int aPolygonIdx )
    {
        return IterateSegments( aPolygonIdx, aPolygonIdx );
    }

    /// Return an iterator object, for iterating aPolygonIdx-th polygon edges.
    CONST_SEGMENT_ITERATOR CIterateSegments( int aPolygonIdx ) const
    {
        return CIterateSegments( aPolygonIdx, aPolygonIdx );
    }

    /// Return an iterator object, for all outlines in the set (no holes).
    SEGMENT_ITERATOR IterateSegments()
    {
        return IterateSegments( 0, OutlineCount() - 1 );
    }

    /// Returns an iterator object, for all outlines in the set (no holes)
    CONST_SEGMENT_ITERATOR CIterateSegments() const
    {
        return CIterateSegments( 0, OutlineCount() - 1 );
    }

    /// Returns an iterator object, for all outlines in the set (with holes)
    SEGMENT_ITERATOR IterateSegmentsWithHoles()
    {
        return IterateSegments( 0, OutlineCount() - 1, true );
    }

    /// Return an iterator object, for the \a aOutline-th outline in the set (with holes).
    SEGMENT_ITERATOR IterateSegmentsWithHoles( int aOutline )
    {
        return IterateSegments( aOutline, aOutline, true );
    }

    /// Return an iterator object, for the \a aOutline-th outline in the set (with holes).
    CONST_SEGMENT_ITERATOR CIterateSegmentsWithHoles() const
    {
        return CIterateSegments( 0, OutlineCount() - 1, true );
    }

    /// Return an iterator object, for the \a aOutline-th outline in the set (with holes).
    CONST_SEGMENT_ITERATOR CIterateSegmentsWithHoles( int aOutline ) const
    {
        return CIterateSegments( aOutline, aOutline, true );
    }


    /// Perform boolean polyset union
    void BooleanAdd( const SHAPE_POLY_SET& b );

    /// Perform boolean polyset difference
    void BooleanSubtract( const SHAPE_POLY_SET& b );

    /// Perform boolean polyset intersection
    void BooleanIntersection( const SHAPE_POLY_SET& b );

    /// Perform boolean polyset exclusive or
    void BooleanXor( const SHAPE_POLY_SET& b );

    /// Perform boolean polyset union between a and b, store the result in it self
    void BooleanAdd( const SHAPE_POLY_SET& a, const SHAPE_POLY_SET& b );

    /// Perform boolean polyset difference between a and b, store the result in it self
    void BooleanSubtract( const SHAPE_POLY_SET& a, const SHAPE_POLY_SET& b );

    /// Perform boolean polyset intersection between a and b, store the result in it self
    void BooleanIntersection( const SHAPE_POLY_SET& a, const SHAPE_POLY_SET& b );

    /// Perform boolean polyset exclusive or between a and b, store the result in it self
    void BooleanXor( const SHAPE_POLY_SET& a, const SHAPE_POLY_SET& b );

    /**
    * Extract all contours from this polygon set, then recreate polygons with holes.
    * Essentially XOR'ing, but faster. Self-intersecting polygons are not supported.
    */
    void RebuildHolesFromContours();

    /**
     * Perform outline inflation/deflation.
     *
     * Polygons can have holes, but not linked holes with main outlines, if aFactor < 0.  For
     * those use InflateWithLinkedHoles() to avoid odd corners where the link segments meet
     * the outline.
     *
     * @param aAmount is the number of units to offset edges.
     * @param aCornerStrategy #ALLOW_ACUTE_CORNERS to preserve all angles,
     *                        #CHAMFER_ACUTE_CORNERS to chop angles less than 90°,
     *                        #ROUND_ACUTE_CORNERS to round off angles less than 90°,
     *                        #ROUND_ALL_CORNERS to round regardless of angles
     * @param aMaxError is the allowable deviation when rounding corners with an approximated
     *                  polygon
     */
    void Inflate( int aAmount, CORNER_STRATEGY aCornerStrategy, int aMaxError,
                  bool aSimplify = false );

    void Deflate( int aAmount, CORNER_STRATEGY aCornerStrategy, int aMaxError )
    {
        Inflate( -aAmount, aCornerStrategy, aMaxError );
    }

    /**
     * Perform offsetting of a line chain. Replaces this polygon set with the result.
     *
     * @param aLine is the line to perform offsetting on.
     * @param aAmount is the number of units to offset the line chain.
     * @param aCornerStrategy #ALLOW_ACUTE_CORNERS to preserve all angles,
     *                        #CHAMFER_ACUTE_CORNERS to chop angles less than 90°,
     *                        #ROUND_ACUTE_CORNERS to round off angles less than 90°,
     *                        #ROUND_ALL_CORNERS to round regardless of angles
     * @param aMaxError is the allowable deviation when rounding corners with an approximated
     *                  polygon
     * @param aSimplify set to simplify the output polygon.
     */
    void OffsetLineChain( const SHAPE_LINE_CHAIN& aLine, int aAmount,
                          CORNER_STRATEGY aCornerStrategy, int aMaxError, bool aSimplify );

    /**
     * Perform outline inflation/deflation, using round corners.
     *
     * Polygons can have holes and/or linked holes with main outlines.  The resulting
     * polygons are also polygons with linked holes to main outlines.
     */
    void InflateWithLinkedHoles( int aFactor, CORNER_STRATEGY aCornerStrategy, int aMaxError );

    /// Convert a set of polygons with holes to a single outline with "slits"/"fractures"
    /// connecting the outer ring to the inner holes
    void Fracture();

    /// Convert a single outline slitted ("fractured") polygon into a set ouf outlines
    /// with holes.
    void Unfracture();

    /// Return true if the polygon set has any holes.
    bool HasHoles() const;

    /// Return true if the polygon set has any holes that share a vertex.
    bool HasTouchingHoles() const;


    /// Simplify the polyset (merges overlapping polys, eliminates degeneracy/self-intersections)
    void Simplify();

    /**
     * Simplifies the lines in the polyset.  This checks intermediate points to see if they are
     * collinear with their neighbors, and removes them if they are.
     *
     * @param aMaxError is the maximum error to allow when simplifying the lines.
     */
    void SimplifyOutlines( int aMaxError = 0 );

    /**
     * Convert a self-intersecting polygon to one (or more) non self-intersecting polygon(s).
     *
     * Removes null segments.
     *
     * @return the polygon count (always >= 1, because there is at least one polygon)
     *         There are new polygons only if the polygon count is > 1.
     */
    int NormalizeAreaOutlines();

    /// @copydoc SHAPE::Format()
    const std::string Format( bool aCplusPlus = true ) const override;

    /// @copydoc SHAPE::Parse()
    bool Parse( std::stringstream& aStream ) override;

    /// @copydoc SHAPE::Move()
    void Move( const VECTOR2I& aVector ) override;

    /**
     * Mirror the line points about y or x (or both)
     *
     * @param aRef sets the reference point about which to mirror
     * @param aFlipDirection is the direction to mirror the points.
     */
    void Mirror( const VECTOR2I& aRef, FLIP_DIRECTION aFlipDirection );

    /**
     * Rotate all vertices by a given angle.
     *
     * @param aCenter is the rotation center.
     * @param aAngle is the rotation angle.
     */
    void Rotate( const EDA_ANGLE& aAngle, const VECTOR2I& aCenter = { 0, 0 } ) override;

    /// @copydoc SHAPE::IsSolid()
    bool IsSolid() const override
    {
        return true;
    }

    const BOX2I BBox( int aClearance = 0 ) const override;

    /**
     * Check if point \a aP lies on an edge or vertex of some of the outlines or holes.
     *
     * @param aP is the point to check.
     * @return true if the point lies on the edge of any polygon.
     */
    bool PointOnEdge( const VECTOR2I& aP, int aAccuracy = 0 ) const;

    /**
     * Check if the boundary of shape (this) lies closer to the shape \a aShape than \a aClearance,
     * indicating a collision.
     *
     * @param aShape shape to check collision against
     * @param aClearance minimum clearance
     * @param aActual [out] an optional pointer to an int to store the actual distance in the
     *                event of a collision.
     * @param aLocation [out] an option pointer to a point to store a nearby location in the
     *                  event of a collision.
     * @return true if there is a collision.
     */
    bool Collide( const SHAPE* aShape, int aClearance = 0, int* aActual = nullptr,
                  VECTOR2I* aLocation = nullptr ) const override;

    /**
     * Check whether the point \a aP is either inside or on the edge of the polygon set.
     *
     * Note that prior to Jul 2020 we considered the edge to *not* be part of the polygon.
     * However, most other shapes (rects, circles, segments, etc.) include their edges and
     * the difference was causing issues when used for DRC.
     *
     * (FWIW, SHAPE_LINE_CHAIN was a split personality, with Collide() including its edges
     * but PointInside() not.  That has also been corrected.)
     *
     * @param  aP         is the VECTOR2I point whose collision with respect to the poly set
     *                    will be tested.
     * @param  aClearance is the security distance; if the point lies closer to the polygon
     *                    than aClearance distance, then there is a collision.
     * @param aActual an optional pointer to an int to store the actual distance in the event
     *                of a collision.
     * @return true if the point aP collides with the polygon; false in any other case.
     */
    bool Collide( const VECTOR2I& aP, int aClearance = 0, int* aActual = nullptr,
                  VECTOR2I* aLocation = nullptr ) const override;

    /**
     * Check whether the segment \a aSeg collides with the polygon set (or its edge).
     *
     * Note that prior to Jul 2020 we considered the edge to *not* be part of the polygon.
     * However, most other shapes (rects, circles, segments, etc.) include their edges and
     * the difference was causing issues when used for DRC.
     *
     * (FWIW, SHAPE_LINE_CHAIN was a split personality, with Collide() including its edges
     * but PointInside() not.  That has also been corrected.)
     *
     * @param  aSeg       is the SEG segment whose collision with respect to the poly set
     *                    will be tested.
     * @param  aClearance is the security distance; if the segment passes closer to the polygon
     *                    than aClearance distance, then there is a collision.
     * @param aActual an optional pointer to an int to store the actual distance in the event
     *                of a collision.
     * @return true if the segment aSeg collides with the polygon, false in any other case.
     */
    bool Collide( const SEG& aSeg, int aClearance = 0, int* aActual = nullptr,
                  VECTOR2I* aLocation = nullptr ) const override;

    /**
     * Check whether \a aPoint collides with any vertex of any of the contours of the polygon.
     *
     * @param  aPoint     is the #VECTOR2I point whose collision with respect to the polygon
     *                    will be tested.
     * @param  aClearance is the security distance; if \p aPoint lies closer to a vertex than
     *                    aClearance distance, then there is a collision.
     * @param aClosestVertex is the index of the closes vertex to \p aPoint.
     * @return bool - true if there is a collision, false in any other case.
     */
    bool CollideVertex( const VECTOR2I& aPoint, VERTEX_INDEX* aClosestVertex = nullptr,
                        int aClearance = 0 ) const;

    /**
     * Check whether aPoint collides with any edge of any of the contours of the polygon.
     *
     * @param  aPoint     is the VECTOR2I point whose collision with respect to the polygon
     *                    will be tested.
     * @param  aClearance is the security distance; if \p aPoint lies closer to a vertex than
     *                    aClearance distance, then there is a collision.
     * @param aClosestVertex is the index of the closes vertex to \p aPoint.
     * @return bool - true if there is a collision, false in any other case.
     */
    bool CollideEdge( const VECTOR2I& aPoint, VERTEX_INDEX* aClosestVertex = nullptr,
                      int aClearance = 0 ) const;

    bool PointInside( const VECTOR2I& aPt, int aAccuracy = 0,
                      bool aUseBBoxCache = false ) const override;

    /**
     * Construct BBoxCaches for Contains(), below.
     *
     * @note These caches **must** be built before a group of calls to Contains().  They are
     *       **not** kept up-to-date by editing actions.
     */
    void BuildBBoxCaches() const;

    const BOX2I BBoxFromCaches() const;

    /**
     * Return true if a given subpolygon contains the point \a aP.
     *
     * @param aP is the point to check
     * @param aSubpolyIndex is the subpolygon to check, or -1 to check all
     * @param aUseBBoxCaches gives faster performance when multiple calls are made with no
     *                       editing in between, but the caller MUST cache the bbox caches
     *                       before calling (via BuildBBoxCaches(), above)
     * @return true if the polygon contains the point
     */
    bool Contains( const VECTOR2I& aP, int aSubpolyIndex = -1, int aAccuracy = 0,
                   bool aUseBBoxCaches = false ) const;

    /// Return true if the set is empty (no polygons at all)
    bool IsEmpty() const
    {
        return m_polys.empty();
    }

    /**
     * Delete the \a aGlobalIndex-th vertex.
     *
     * @param aGlobalIndex is the global index of the to-be-removed vertex.
     */
    void RemoveVertex( int aGlobalIndex );

    /**
     * Delete the vertex indexed by \a aRelativeIndex (index of polygon, contour and vertex).
     *
     * @param aRelativeIndices is the set of relative indices of the to-be-removed vertex.
     */
    void RemoveVertex( VERTEX_INDEX aRelativeIndices );

    /// Remove all outlines & holes (clears) the polygon set.
    void RemoveAllContours();

    /**
     * Delete the \a aContourIdx-th contour of the \a aPolygonIdx-th polygon in the set.
     *
     * @param aContourIdx is the index of the contour in the aPolygonIdx-th polygon to be
     *                    removed.
     * @param aPolygonIdx is the index of the polygon in which the to-be-removed contour is.
     *                    Defaults to the last polygon in the set.
     */
    void RemoveContour( int aContourIdx, int aPolygonIdx = -1 );


    /**
     * Delete the \a aOutlineIdx-th outline of the set including its contours and holes.
     *
     * @param aOutlineIdx is the index of the outline to be removed.
     */
    void RemoveOutline( int aOutlineIdx );

    /**
     * Look for null segments; ie, segments whose ends are exactly the same and deletes them.
     *
     * @return the number of deleted segments.
     */
    int RemoveNullSegments();

    /**
     * Accessor function to set the position of a specific point.
     *
     * @param aIndex #VERTEX_INDEX of the point to move.
     * @param aPos destination position of the specified point.
     */
    void SetVertex( const VERTEX_INDEX& aIndex, const VECTOR2I& aPos );

    /**
     * Set the vertex based on the global index.
     *
     * Throws if the index doesn't exist.
     *
     * @param aGlobalIndex global index of the to-be-moved vertex
     * @param aPos New position on the vertex
     */
    void SetVertex( int aGlobalIndex, const VECTOR2I& aPos );

    /// Return total number of vertices stored in the set.
    int TotalVertices() const;

    /// Delete \a aIdx-th polygon from the set.
    void DeletePolygon( int aIdx );

    /// Delete \a aIdx-th polygon and its triangulation data from the set.
    /// If called with \a aUpdateHash false, caller must call UpdateTriangulationDataHash().
    void DeletePolygonAndTriangulationData( int aIdx, bool aUpdateHash = true );

    void UpdateTriangulationDataHash();

    /**
     * Return a chamfered version of the \a aIndex-th polygon.
     *
     * @param aDistance is the chamfering distance.
     * @param aIndex is the index of the polygon to be chamfered.
     * @return A polygon containing the chamfered version of the \a aIndex-th polygon.
     */
    POLYGON ChamferPolygon( unsigned int aDistance, int aIndex );

    /**
     * Return a filleted version of the \a aIndex-th polygon.
     *
     * @param aRadius is the fillet radius.
     * @param aErrorMax is the maximum allowable deviation of the polygon from the circle
     * @param aIndex is the index of the polygon to be filleted
     * @return A polygon containing the filleted version of the \a aIndex-th polygon.
     */
    POLYGON FilletPolygon( unsigned int aRadius, int aErrorMax, int aIndex );

    /**
     * Return a chamfered version of the polygon set.
     *
     * @param aDistance is the chamfering distance.
     * @return A set containing the chamfered version of this set.
     */
    SHAPE_POLY_SET Chamfer( int aDistance );

    /**
     * Return a filleted version of the polygon set.
     *
     * @param aRadius is the fillet radius.
     * @param aErrorMax is the maximum allowable deviation of the polygon from the circle
     * @return A set containing the filleted version of this set.
     */
    SHAPE_POLY_SET Fillet( int aRadius, int aErrorMax );

    /**
     * Compute the minimum distance between the \a aIndex-th polygon and \a aPoint.
     *
     * @param  aPoint is the point whose distance to the aIndex-th polygon has to be measured.
     * @param  aIndex is the index of the polygon whose distance to aPoint has to be measured.
     * @param  aNearest [out] an optional pointer to be filled in with the point on the
     *                  polyset which is closest to aPoint.
     * @return The minimum distance between \a aPoint and all the segments of the \a aIndex-th
     *         polygon. If the point is contained in the polygon, the distance is zero.
     */
    SEG::ecoord SquaredDistanceToPolygon( VECTOR2I aPoint, int aIndex,
                                          VECTOR2I* aNearest ) const;

    /**
     * Compute the minimum distance between the aIndex-th polygon and aSegment with a
     * possible width.
     *
     * @param  aSegment is the segment whose distance to the aIndex-th polygon has to be
     *                  measured.
     * @param  aIndex   is the index of the polygon whose distance to aPoint has to be measured.
     * @param  aNearest [out] an optional pointer to be filled in with the point on the
     *                  polyset which is closest to aSegment.
     * @return The minimum distance between \a aSegment and all the segments of the \a aIndex-th
     *         polygon. If the point is contained in the polygon, the distance is zero.
     */
    SEG::ecoord SquaredDistanceToPolygon( const SEG& aSegment, int aIndex,
                                          VECTOR2I* aNearest) const;

    /**
     * Compute the minimum distance squared between aPoint and all the polygons in the set.
     * Squared distances are used because they avoid the cost of doing square-roots.
     *
     * @param  aPoint is the point whose distance to the set has to be measured.
     * @param  aNearest [out] an optional pointer to be filled in with the point on the
     *                  polyset which is closest to aPoint.
     * @return The minimum distance squared between aPoint and all the polygons in the set.
     *         If the point is contained in any of the polygons, the distance is zero.
     */
    SEG::ecoord SquaredDistance( const VECTOR2I& aPoint, bool aOutlineOnly,
                                 VECTOR2I* aNearest ) const;

    SEG::ecoord SquaredDistance( const VECTOR2I& aPoint, bool aOutlineOnly = false ) const override
    {
        return SquaredDistance( aPoint, aOutlineOnly, nullptr );
    }

    /**
     * Compute the minimum distance squared between aSegment and all the polygons in the set.
     * Squared distances are used because they avoid the cost of doing square-roots.
     *
     * @param  aSegment is the segment whose distance to the polygon set has to be measured.
     * @param  aSegmentWidth is the width of the segment; defaults to zero.
     * @param  aNearest [out] an optional pointer to be filled in with the point on the
     *                  polyset which is closest to aSegment.
     * @return  The minimum distance squared between aSegment and all the polygons in the set.
     *          If the point is contained in the polygon, the distance is zero.
     */
    SEG::ecoord SquaredDistanceToSeg( const SEG& aSegment, VECTOR2I* aNearest = nullptr ) const;

    /**
     * Check whether the \a aGlobalIndex-th vertex belongs to a hole.
     *
     * @param  aGlobalIdx is the index of the vertex.
     * @return true if the globally indexed \a aGlobalIdx-th vertex belongs to a hole.
     */
    bool IsVertexInHole( int aGlobalIdx );

    /**
     * Build a SHAPE_POLY_SET from a bunch of outlines in provided in random order.
     *
     * @param aPath set of closed outlines forming the polygon.
     *              Positive orientation = outline, negative = hole
     * @param aEvenOdd forces the even-off fill rule (default is non zero)
     */
    void BuildPolysetFromOrientedPaths( const std::vector<SHAPE_LINE_CHAIN>& aPaths,
                                        bool aEvenOdd = false );

    void TransformToPolygon( SHAPE_POLY_SET& aBuffer, int aError,
                             ERROR_LOC aErrorLoc ) const override
    {
        aBuffer.Append( *this );
    }

    const std::vector<SEG> GenerateHatchLines( const std::vector<double>& aSlopes, int aSpacing,
                                               int aLineLength ) const;

    void Scale( double aScaleFactorX, double aScaleFactorY, const VECTOR2I& aCenter );

protected:
    void cacheTriangulation( bool aPartition, bool aSimplify,
                             std::vector<std::unique_ptr<TRIANGULATED_POLYGON>>* aHintData );

private:
    enum DROP_TRIANGULATION_FLAG { SINGLETON };

    SHAPE_POLY_SET( const SHAPE_POLY_SET& aOther, DROP_TRIANGULATION_FLAG );

    void fractureSingle( POLYGON& paths );
    void unfractureSingle ( POLYGON& path );
    void importTree( Clipper2Lib::PolyTree64&            tree,
                     const std::vector<CLIPPER_Z_VALUE>& aZValueBuffer,
                     const std::vector<SHAPE_ARC>&       aArcBuffe );
    void importPaths( Clipper2Lib::Paths64&               paths,
                     const std::vector<CLIPPER_Z_VALUE>& aZValueBuffer,
                     const std::vector<SHAPE_ARC>&       aArcBuffe );
    void importPolyPath( const std::unique_ptr<Clipper2Lib::PolyPath64>& aPolyPath,
                     const std::vector<CLIPPER_Z_VALUE>&                 aZValueBuffer,
                     const std::vector<SHAPE_ARC>&                       aArcBuffer );

    void inflate2( int aAmount, int aCircleSegCount, CORNER_STRATEGY aCornerStrategy, bool aSimplify = false );

    void inflateLine2( const SHAPE_LINE_CHAIN& aLine, int aAmount, int aCircleSegCount,
                       CORNER_STRATEGY aCornerStrategy, bool aSimplify = false );

    void splitCollinearOutlines();

    /**
     * Check if two line segments are collinear and overlap.
     *
     * @param aSegA First line segment
     * @param aSegB Second line segment
     * @return true if segments are collinear and overlap
     */
    bool isExteriorWaist( const SEG& aSegA, const SEG& aSegB ) const;

    /**
     * This is the engine to execute all polygon boolean transforms (AND, OR, ... and polygon
     * simplification (merging overlapping  polygons).
     *
     * @param aType is the transform type ( see Clipper2Lib::ClipType )
     * @param aOtherShape is the SHAPE_LINE_CHAIN to combine with me.
     */
    void booleanOp( Clipper2Lib::ClipType aType, const SHAPE_POLY_SET& aOtherShape );

    void booleanOp( Clipper2Lib::ClipType aType, const SHAPE_POLY_SET& aShape,
                    const SHAPE_POLY_SET& aOtherShape );

    /**
     * Check whether the point \a aP is inside the \a aSubpolyIndex-th polygon of the polyset. If
     * the points lies on an edge, the polygon is considered to contain it.
     *
     * @param  aP            is the #VECTOR2I point whose position with respect to the inside of
     *                       the aSubpolyIndex-th polygon will be tested.
     * @param  aSubpolyIndex is an integer specifying which polygon in the set has to be
     *                       checked.
     * @param  aAccuracy     accuracy in internal units
     * @param aUseBBoxCaches gives faster performance when multiple calls are made with no
     *                       editing in between, but the caller MUST cache the bbox caches
     *                       before calling (via BuildBBoxCaches(), above)
     * @return true if \a aP is inside aSubpolyIndex-th polygon; false in any other case.
     */
    bool containsSingle( const VECTOR2I& aP, int aSubpolyIndex, int aAccuracy,
                         bool aUseBBoxCaches = false ) const;

    /**
     * Operation ChamferPolygon and FilletPolygon are computed under the private chamferFillet
     * method; this enum is defined to make the necessary distinction when calling this method
     * from the public ChamferPolygon and FilletPolygon methods.
     */
    enum CORNER_MODE
    {
        CHAMFERED,
        FILLETED
    };

    /**
     * Return the chamfered or filleted version of the \a aIndex-th polygon in the set, depending
     * on the \a aMode selected
     * @param  aMode     represent which action will be taken: CORNER_MODE::CHAMFERED will
     *                   return a chamfered version of the polygon, CORNER_MODE::FILLETED will
     *                   return a filleted version of the polygon.
     * @param  aDistance is the chamfering distance if aMode = CHAMFERED; if aMode = FILLETED,
     *                   is the filleting radius.
     * @param  aIndex    is the index of the polygon that will be chamfered/filleted.
     * @param  aErrorMax is the maximum allowable deviation of the polygon from the circle
     *                   if aMode = FILLETED. If aMode = CHAMFERED, it is unused.
     * @return the chamfered/filleted version of the polygon.
     */
    POLYGON chamferFilletPolygon( CORNER_MODE aMode, unsigned int aDistance,
                                  int aIndex, int aErrorMax );

    /// Return true if the polygon set has any holes that touch share a vertex.
    bool hasTouchingHoles( const POLYGON& aPoly ) const;

    HASH_128 checksum() const;

protected:
    std::vector<POLYGON>                               m_polys;
    std::vector<std::unique_ptr<TRIANGULATED_POLYGON>> m_triangulatedPolys;

    std::atomic<bool> m_triangulationValid = false;
    std::mutex  m_triangulationMutex;

private:
    HASH_128 m_hash;
    bool     m_hashValid = false;
};

#endif // __SHAPE_POLY_SET_H
