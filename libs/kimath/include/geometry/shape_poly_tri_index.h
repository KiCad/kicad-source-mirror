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

#ifndef SHAPE_POLY_TRI_INDEX_H
#define SHAPE_POLY_TRI_INDEX_H

#include <cstdint>
#include <deque>
#include <limits>
#include <vector>
#include <utility>

#include <math/box2.h>
#include <geometry/rtree/packed_rtree.h>
#include <geometry/shape_poly_set.h>

/**
 * A spatial index over the triangles of a cached triangulation, used to prune collision tests.
 * Visitation order is unspecified.
 *
 * The index does not own the triangles, so whoever owns them must drop it whenever they are
 * rebuilt, discarded or moved.
 */
class TRIANGULATION_INDEX
{
public:
    using TRI = SHAPE_POLY_SET::TRIANGULATED_POLYGON::TRI;

    /// Index every triangle of every polygon.  Immutable once constructed.
    explicit TRIANGULATION_INDEX( const std::vector<std::unique_ptr<SHAPE_POLY_SET::TRIANGULATED_POLYGON>>& aPolys )
    {
        size_t count = 0;

        for( const std::unique_ptr<SHAPE_POLY_SET::TRIANGULATED_POLYGON>& poly : aPolys )
            count += poly->GetTriangleCount();

        if( count == 0 )
            return;

        m_tris.reserve( count );

        KIRTREE::PACKED_RTREE<uint32_t, int, 2>::Builder builder;
        builder.Reserve( count );

        for( const std::unique_ptr<SHAPE_POLY_SET::TRIANGULATED_POLYGON>& poly : aPolys )
        {
            for( const TRI& tri : poly->Triangles() )
            {
                const BOX2I bbox = tri.BBox();
                const int   lo[2] = { bbox.GetX(), bbox.GetY() };
                const int   hi[2] = { bbox.GetRight(), bbox.GetBottom() };

                builder.Add( lo, hi, static_cast<uint32_t>( m_tris.size() ) );
                m_tris.push_back( &tri );
            }
        }

        m_tree = builder.Build();
    }

    /**
     * Visit every triangle whose bounding box overlaps @a aBox.  @a aVisitor is called as
     * bool( const TRI* ); returning false stops the search.
     */
    template <typename VISITOR>
    void Query( const BOX2I& aBox, VISITOR aVisitor ) const
    {
        if( m_tris.empty() )
            return;

        const int lo[2] = { aBox.GetX(), aBox.GetY() };
        const int hi[2] = { aBox.GetRight(), aBox.GetBottom() };

        // PACKED_RTREE::Search takes its visitor by non-const reference, so name it.
        auto forward = [&]( uint32_t aIndex ) -> bool
        {
            return aVisitor( m_tris[aIndex] );
        };

        m_tree.Search( lo, hi, forward );
    }

private:
    KIRTREE::PACKED_RTREE<uint32_t, int, 2> m_tree;

    /// Not owning: the triangulation outlives us.
    std::vector<const TRI*> m_tris;
};


/**
 * Visit the triangles that could collide with something inside @a aBox, through @a aIndex when
 * one exists, or by scanning every polygon otherwise.  @a aVisitor is called as 
 * bool( const TRI* ); returning false stops the walk.
 *
 * Implementation detail of the AnyTriangleHit() overloads below; call those instead.
 *
 * @return false if the visitor stopped the walk, true if it ran to completion.
 */
template <typename VISITOR>
bool VisitTriangleCandidates( const TRIANGULATION_INDEX*                                                aIndex,
                              const std::vector<std::unique_ptr<SHAPE_POLY_SET::TRIANGULATED_POLYGON>>& aPolys,
                              const BOX2I& aBox, VISITOR aVisitor )
{
    if( aIndex )
    {
        bool stopped = false;

        aIndex->Query( aBox,
                       [&]( const SHAPE_POLY_SET::TRIANGULATED_POLYGON::TRI* aTri )
                       {
                           if( aVisitor( aTri ) )
                               return true;

                           stopped = true;
                           return false;
                       } );

        return !stopped;
    }

    for( const std::unique_ptr<SHAPE_POLY_SET::TRIANGULATED_POLYGON>& poly : aPolys )
    {
        for( const SHAPE_POLY_SET::TRIANGULATED_POLYGON::TRI& tri : poly->Triangles() )
        {
            if( !aVisitor( &tri ) )
                return false;
        }
    }

    return true;
}


/**
 * Return true if any candidate triangle collides with @a aShape within @a aClearance.
 *
 * With @a aActual or @a aLocation, every candidate is visited and the least collision distance
 * and where it occurs are reported; without them the walk stops at the first hit.
 */
inline bool AnyTriangleHit( const TRIANGULATION_INDEX*                                                aIndex,
                            const std::vector<std::unique_ptr<SHAPE_POLY_SET::TRIANGULATED_POLYGON>>& aPolys,
                            const SHAPE* aShape, int aClearance = 0, int* aActual = nullptr,
                            VECTOR2I* aLocation = nullptr )
{
    // NOT BBox( aClearance ): SHAPE_COMPOUND::BBox() ignores its argument, and inflating
    // separately keeps near-miss triangles in the candidate set.
    BOX2I qbox = aShape->BBox();
    if( aClearance > 0 )
    {
        qbox.Inflate( aClearance );
    }

    if( !aActual && !aLocation )
    {
        return !VisitTriangleCandidates( aIndex, aPolys, qbox,
                                         [&]( const SHAPE_POLY_SET::TRIANGULATED_POLYGON::TRI* aTri )
                                         {
                                             return !aShape->Collide( aTri, aClearance );
                                         } );
    }

    int      actual = std::numeric_limits<int>::max();
    VECTOR2I location;

    VisitTriangleCandidates( aIndex, aPolys, qbox,
                             [&]( const SHAPE_POLY_SET::TRIANGULATED_POLYGON::TRI* aTri )
                             {
                                 int      triActual;
                                 VECTOR2I triLocation;

                                 if( aShape->Collide( aTri, aClearance, &triActual, &triLocation )
                                     && triActual < actual )
                                 {
                                     actual = triActual;
                                     location = triLocation;
                                 }

                                 return true; // Finding a minimum needs every candidate
                             } );

    if( actual == std::numeric_limits<int>::max() )
        return false;

    if( aActual )
        *aActual = actual;

    if( aLocation )
        *aLocation = location;

    return true;
}


/// Return true if any candidate triangle contains @a aPt.
inline bool AnyTriangleHit( const TRIANGULATION_INDEX*                                                aIndex,
                            const std::vector<std::unique_ptr<SHAPE_POLY_SET::TRIANGULATED_POLYGON>>& aPolys,
                            const VECTOR2I&                                                           aPt )
{
    return !VisitTriangleCandidates( aIndex, aPolys, BOX2I( aPt, VECTOR2I( 0, 0 ) ),
                                     [&]( const SHAPE_POLY_SET::TRIANGULATED_POLYGON::TRI* aTri )
                                     {
                                         return !aTri->Collide( aPt );
                                     } );
}

#endif // SHAPE_POLY_TRI_INDEX_H
