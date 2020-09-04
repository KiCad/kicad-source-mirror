/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 KiCad Developers, see AUTHORS.txt for contributors.
 * Copyright (C) 2020 CERN
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-3.0.html
 * or you may search the http://www.gnu.org website for the version 3 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef DRC_RTREE_H_
#define DRC_RTREE_H_

#include <eda_rect.h>
#include <board_connected_item.h>
#include <set>
#include <vector>

#include <geometry/rtree.h>
#include <vector2d.h>

/**
 * DRC_RTREE -
 * Implements an R-tree for fast spatial and layer indexing of connectable items.
 * Non-owning.
 */
class DRC_RTREE
{
private:
    using drc_rtree = RTree<BOARD_ITEM*, int, 2, double>;

public:
    DRC_RTREE()
    {
        for( int layer : LSET::AllCuMask().Seq() )
            m_tree[layer] = new drc_rtree();

        m_count = 0;
    }

    ~DRC_RTREE()
    {
        for( auto tree : m_tree )
            delete tree;
    }

    /**
     * Function Insert()
     * Inserts an item into the tree. Item's bounding box is taken via its GetBoundingBox() method.
     */
    void insert( BOARD_ITEM* aItem )
    {
        if( ZONE_CONTAINER* zone = dyn_cast<ZONE_CONTAINER*>( aItem ) )
        {
            for( int layer : zone->GetLayerSet().Seq() )
            {
                const SHAPE_POLY_SET& polyset = zone->GetFilledPolysList( PCB_LAYER_ID( layer ) );

                for( int ii = 0; ii < polyset.TriangulatedPolyCount(); ++ii )
                {
                    const auto poly = polyset.TriangulatedPolygon( ii );

                    for( int jj = 0; jj < poly->GetTriangleCount(); ++jj )
                    {
                        VECTOR2I a;
                        VECTOR2I b;
                        VECTOR2I c;
                        poly->GetTriangle( jj, a, b, c );

                        const int mmin2[2] = { std::min( a.x, std::min( b.x, c.x ) ),
                                               std::min( a.y, std::min( b.y, c.y ) ) };
                        const int mmax2[2] = { std::max( a.x, std::max( b.x, c.x ) ),
                                               std::max( a.y, std::max( b.y, c.y ) ) };

                        m_tree[layer]->Insert( mmin2, mmax2, aItem );
                    }
                }
            }
        }
        else
        {
            const EDA_RECT& bbox    = aItem->GetBoundingBox();
            const int       mmin[2] = { bbox.GetX(), bbox.GetY() };
            const int       mmax[2] = { bbox.GetRight(), bbox.GetBottom() };

            for( int layer : aItem->GetLayerSet().Seq() )
            {
                m_tree[layer]->Insert( mmin, mmax, aItem );
            }
        }

        m_count++;
    }

    /**
     * Function Remove()
     * Removes an item from the tree. Removal is done by comparing pointers, attempting
     * to remove a copy of the item will fail.
     */
    bool remove( BOARD_ITEM* aItem )
    {
        // First, attempt to remove the item using its given BBox
        const EDA_RECT& bbox    = aItem->GetBoundingBox();
        const int       mmin[2] = { bbox.GetX(), bbox.GetY() };
        const int       mmax[2] = { bbox.GetRight(), bbox.GetBottom() };
        bool            removed = false;

        for( auto layer : aItem->GetLayerSet().Seq() )
        {
            if( ZONE_CONTAINER* zone = dyn_cast<ZONE_CONTAINER*>( aItem ) )
            {
                // Continue removing the zone elements from the tree until they cannot be found
                while( !m_tree[int( layer )]->Remove( mmin, mmax, aItem ) )
                    ;

                const int mmin2[2] = { INT_MIN, INT_MIN };
                const int mmax2[2] = { INT_MAX, INT_MAX };

                // If we are not successful ( true == not found ), then we expand
                // the search to the full tree
                while( !m_tree[int( layer )]->Remove( mmin2, mmax2, aItem ) )
                    ;

                // Loop to the next layer
                continue;
            }

            // The non-zone search expects only a single element in the tree with the same
            // pointer aItem
            if( m_tree[int( layer )]->Remove( mmin, mmax, aItem ) )
            {
                // N.B. We must search the whole tree for the pointer to remove
                // because the item may have been moved before we have the chance to
                // delete it from the tree
                const int mmin2[2] = { INT_MIN, INT_MIN };
                const int mmax2[2] = { INT_MAX, INT_MAX };

                if( m_tree[int( layer )]->Remove( mmin2, mmax2, aItem ) )
                    continue;
            }

            removed = true;
        }

        m_count -= int( removed );

        return removed;
    }

    /**
     * Function RemoveAll()
     * Removes all items from the RTree
     */
    void clear()
    {
        for( auto tree : m_tree )
            tree->RemoveAll();

        m_count = 0;
    }

    /**
     * Determine if a given item exists in the tree.  Note that this does not search the full tree
     * so if the item has been moved, this will return false when it should be true.
     *
     * @param aItem Item that may potentially exist in the tree
     * @param aRobust If true, search the whole tree, not just the bounding box
     * @return true if the item definitely exists, false if it does not exist within bbox
     */
    bool contains( BOARD_ITEM* aItem, bool aRobust = false )
    {
        const EDA_RECT& bbox    = aItem->GetBoundingBox();
        const int       mmin[2] = { bbox.GetX(), bbox.GetY() };
        const int       mmax[2] = { bbox.GetRight(), bbox.GetBottom() };
        bool            found   = false;

        auto search = [&found, &aItem]( const BOARD_ITEM* aSearchItem ) {
            if( aSearchItem == aItem )
            {
                found = true;
                return false;
            }

            return true;
        };

        for( int layer : aItem->GetLayerSet().Seq() )
        {
            m_tree[layer]->Search( mmin, mmax, search );

            if( found )
                break;
        }

        if( !found && aRobust )
        {
            for( int layer : LSET::AllCuMask().Seq() )
            {
                // N.B. We must search the whole tree for the pointer to remove
                // because the item may have been moved.  We do not expand the item
                // layer search as this should not change.

                const int mmin2[2] = { INT_MIN, INT_MIN };
                const int mmax2[2] = { INT_MAX, INT_MAX };

                m_tree[layer]->Search( mmin2, mmax2, search );

                if( found )
                    break;
            }
        }

        return found;
    }

    std::vector<std::pair<int, BOARD_ITEM*>> GetNearest( const wxPoint &aPoint,
                                                                   PCB_LAYER_ID aLayer,
                                                                   int aLimit )
    {

        const int point[2] = { aPoint.x, aPoint.y };
        auto result = m_tree[int( aLayer )]->NearestNeighbors( point,
                [aLimit]( std::size_t a_count, int a_maxDist ) -> bool
                {
                    return a_count >= aLimit;
                },
                []( BOARD_ITEM* aElement) -> bool
                {
                    // Don't remove any elements from the list
                    return false;
                },
                [aLayer]( const int* a_point, BOARD_ITEM* a_data ) -> int
                {
                    switch( a_data->Type() )
                    {
                    case PCB_TRACE_T:
                    {
                        TRACK* track = static_cast<TRACK*>( a_data );
                        SEG seg( track->GetStart(), track->GetEnd() );
                        return seg.Distance( VECTOR2I( a_point[0], a_point[1] ) ) -
                                           ( track->GetWidth() + 1 ) / 2;
                    }
                    case PCB_VIA_T:
                    {
                        VIA* via = static_cast<VIA*>( a_data );
                        return ( VECTOR2I( via->GetPosition() ) -
                                 VECTOR2I( a_point[0], a_point[1] ) ).EuclideanNorm() -
                               ( via->GetWidth() + 1 ) / 2;

                    }
                    default:
                    {
                        VECTOR2I point( a_point[0], a_point[1] );
                        int      dist = 0;
                        auto     shape = a_data->GetEffectiveShape( aLayer );

                        // Here we use a hack to get the distance by colliding with a large area
                        // However, we can't use just MAX_INT because we will overflow the collision calculations
                        shape->Collide( point, std::numeric_limits<int>::max() / 2, &dist);
                        return dist;
                    }
                    }
                    return 0;
                });

        return result;
    }

    /**
     * Returns the number of items in the tree
     * @return number of elements in the tree;
     */
    size_t size()
    {
        return m_count;
    }

    bool empty()
    {
        return m_count == 0;
    }

    using iterator = typename drc_rtree::Iterator;

    /**
     * The DRC_LAYER struct provides a layer-specific auto-range iterator to the RTree.  Using
     * this struct, one can write lines like:
     *
     * for( auto item : rtree.OnLayer( In1_Cu ) )
     *
     * and iterate over only the RTree items that are on In1
     */
    struct DRC_LAYER
    {
        DRC_LAYER( drc_rtree* aTree ) : layer_tree( aTree )
        {
            m_rect = { { INT_MIN, INT_MIN }, { INT_MAX, INT_MAX } };
        };

        DRC_LAYER( drc_rtree* aTree, const EDA_RECT aRect ) : layer_tree( aTree )
        {
            m_rect = { { aRect.GetX(), aRect.GetY() },
                       { aRect.GetRight(), aRect.GetBottom() } };
        };

        drc_rtree::Rect m_rect;
        drc_rtree*      layer_tree;

        iterator begin()
        {
            return layer_tree->begin( m_rect );
        }

        iterator end()
        {
            return layer_tree->end( m_rect );
        }
    };

    DRC_LAYER OnLayer( PCB_LAYER_ID aLayer )
    {
        return DRC_LAYER( m_tree[int( aLayer )] );
    }

    DRC_LAYER Overlapping( PCB_LAYER_ID aLayer, const wxPoint& aPoint, int aAccuracy = 0 )
    {
        EDA_RECT rect( aPoint, wxSize( 0, 0 ) );
        rect.Inflate( aAccuracy );
        return DRC_LAYER( m_tree[int( aLayer )], rect );
    }

    DRC_LAYER Overlapping( PCB_LAYER_ID aLayer, const EDA_RECT& aRect )
    {
        return DRC_LAYER( m_tree[int( aLayer )], aRect );
    }


private:
    drc_rtree*  m_tree[MAX_CU_LAYERS];
    size_t      m_count;
};


#endif /* DRC_RTREE_H_ */
