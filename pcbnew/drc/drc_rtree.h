/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#include <board_item.h>
#include <pad.h>
#include <pcb_field.h>
#include <memory>
#include <unordered_set>
#include <set>
#include <vector>

#include <geometry/rtree.h>
#include <geometry/shape.h>
#include <geometry/shape_segment.h>
#include <math/vector2d.h>
#include "geometry/shape_null.h"
#include "board.h"

#define ATOMIC_TABLES true

/**
 * Implement an R-tree for fast spatial and layer indexing of connectable items.
 * Non-owning.
 */
class DRC_RTREE
{
public:
    struct ITEM_WITH_SHAPE
    {
        ITEM_WITH_SHAPE( BOARD_ITEM *aParent, const SHAPE* aShape,
                         std::shared_ptr<SHAPE> aParentShape = nullptr ) :
            parent( aParent ),
            shape( aShape ),
            shapeStorage( nullptr ),
            parentShape( std::move( aParentShape ) )
        {};

        ITEM_WITH_SHAPE( BOARD_ITEM *aParent, const std::shared_ptr<SHAPE>& aShape,
                         std::shared_ptr<SHAPE> aParentShape = nullptr ) :
            parent( aParent ),
            shape( aShape.get() ),
            shapeStorage( aShape ),
            parentShape( std::move( aParentShape ) )
        {};

        BOARD_ITEM*            parent;
        const SHAPE*           shape;
        std::shared_ptr<SHAPE> shapeStorage;
        std::shared_ptr<SHAPE> parentShape;
    };

private:
    using drc_rtree = RTree<ITEM_WITH_SHAPE*, int, 2, double>;

public:
    DRC_RTREE()
    {
        for( int layer : LSET::AllLayersMask() )
            m_tree[layer] = new drc_rtree();

        m_count = 0;
    }

    ~DRC_RTREE()
    {
        for( auto& [_, tree] : m_tree )
        {
            for( DRC_RTREE::ITEM_WITH_SHAPE* el : *tree )
                delete el;

            delete tree;
        }
    }

    /**
     * Insert an item into the tree on a particular layer with an optional worst clearance.
     */
    void Insert( BOARD_ITEM* aItem, PCB_LAYER_ID aLayer, int aWorstClearance = 0, bool aAtomicTables = false )
    {
        Insert( aItem, aLayer, aLayer, aWorstClearance, aAtomicTables );
    }

    /**
     * Insert an item into the tree on a particular layer with a worst clearance.  Allows the
     * source layer to be different from the tree layer.
     */
    void Insert( BOARD_ITEM* aItem, PCB_LAYER_ID aRefLayer, PCB_LAYER_ID aTargetLayer,
                 int aWorstClearance, bool aAtomicTables = false )
    {
        wxCHECK( aTargetLayer != UNDEFINED_LAYER, /* void */ );

        if( aItem->Type() == PCB_FIELD_T && !static_cast<PCB_FIELD*>( aItem )->IsVisible() )
            return;

        BOARD_ITEM* parent = aItem;

        if( aAtomicTables && aItem->Type() == PCB_TABLECELL_T )
            parent = aItem->GetParent();

        std::vector<const SHAPE*> subshapes;
        std::shared_ptr<SHAPE> shape = aItem->GetEffectiveShape( aRefLayer );

        wxCHECK2_MSG( shape, return, wxT( "Item does not have a valid shape for this layer" ) );

        if( shape->HasIndexableSubshapes() )
            shape->GetIndexableSubshapes( subshapes );
        else
            subshapes.push_back( shape.get() );

        for( const SHAPE* subshape : subshapes )
        {
            if( dynamic_cast<const SHAPE_NULL*>( subshape ) )
                continue;

            BOX2I bbox = subshape->BBox();

            bbox.Inflate( aWorstClearance );

            const int        mmin[2] = { bbox.GetX(), bbox.GetY() };
            const int        mmax[2] = { bbox.GetRight(), bbox.GetBottom() };
            ITEM_WITH_SHAPE* itemShape = new ITEM_WITH_SHAPE( parent, subshape, shape );

            m_tree[aTargetLayer]->Insert( mmin, mmax, itemShape );
            m_count++;
        }

        if( aItem->Type() == PCB_PAD_T && aItem->HasHole() )
        {
            std::shared_ptr<SHAPE_SEGMENT> hole = aItem->GetEffectiveHoleShape();
            BOX2I                          bbox = hole->BBox();

            bbox.Inflate( aWorstClearance );

            const int        mmin[2] = { bbox.GetX(), bbox.GetY() };
            const int        mmax[2] = { bbox.GetRight(), bbox.GetBottom() };
            ITEM_WITH_SHAPE* itemShape = new ITEM_WITH_SHAPE( parent, hole, shape );

            m_tree[aTargetLayer]->Insert( mmin, mmax, itemShape );
            m_count++;
        }
    }

    /**
     * Remove all items from the RTree.
     */
    void clear()
    {
        for( auto& [_, tree] : m_tree )
            tree->RemoveAll();

        m_count = 0;
    }

    bool CheckColliding( SHAPE* aRefShape, PCB_LAYER_ID aTargetLayer, int aClearance = 0,
                         std::function<bool( BOARD_ITEM*)> aFilter = nullptr ) const
    {
        BOX2I box = aRefShape->BBox();
        box.Inflate( aClearance );

        int min[2] = { box.GetX(),         box.GetY() };
        int max[2] = { box.GetRight(),     box.GetBottom() };

        int count = 0;

        auto visit =
                [&] ( ITEM_WITH_SHAPE* aItem ) -> bool
                {
                    if( !aFilter || aFilter( aItem->parent ) )
                    {
                        int actual;

                        if( aRefShape->Collide( aItem->shape, aClearance, &actual ) )
                        {
                            count++;
                            return false;
                        }
                    }

                    return true;
                };

        if( auto it = m_tree.find( aTargetLayer ); it != m_tree.end() )
            it->second->Search( min, max, visit );

        return count > 0;
    }

    /**
     * This is a fast test which essentially does bounding-box overlap given a worst-case
     * clearance.  It's used when looking up the specific item-to-item clearance might be
     * expensive and should be deferred till we know we have a possible hit.
     */
    int QueryColliding( BOARD_ITEM* aRefItem, PCB_LAYER_ID aRefLayer, PCB_LAYER_ID aTargetLayer,
                        std::function<bool( BOARD_ITEM* )> aFilter = nullptr,
                        std::function<bool( BOARD_ITEM* )> aVisitor = nullptr,
                        int aClearance = 0 ) const
    {
        // keep track of BOARD_ITEMs that have already been found to collide (some items might
        // be built of COMPOUND/triangulated shapes and a single subshape collision means we have
        // a hit)
        std::unordered_set<BOARD_ITEM*> collidingCompounds;

        // keep track of results of client filter so we don't ask more than once for compound
        // shapes
        std::unordered_map<BOARD_ITEM*, bool> filterResults;

        BOX2I box = aRefItem->GetBoundingBox();
        box.Inflate( aClearance );

        int min[2] = { box.GetX(),     box.GetY() };
        int max[2] = { box.GetRight(), box.GetBottom() };

        std::shared_ptr<SHAPE> refShape = aRefItem->GetEffectiveShape( aRefLayer );

        int count = 0;

        auto visit =
                [&]( ITEM_WITH_SHAPE* aItem ) -> bool
                {
                    if( aItem->parent == aRefItem )
                        return true;

                    if( collidingCompounds.find( aItem->parent ) != collidingCompounds.end() )
                        return true;

                    bool filtered;
                    auto it = filterResults.find( aItem->parent );

                    if( it == filterResults.end() )
                    {
                        filtered = aFilter && !aFilter( aItem->parent );
                        filterResults[ aItem->parent ] = filtered;
                    }
                    else
                    {
                        filtered = it->second;
                    }

                    if( filtered )
                        return true;

                    wxCHECK( aItem->shape, false );

                    if( refShape->Collide( aItem->shape, aClearance ) )
                    {
                        collidingCompounds.insert( aItem->parent );
                        count++;

                        if( aVisitor )
                            return aVisitor( aItem->parent );
                    }

                    return true;
                };

        if( auto it = m_tree.find( aTargetLayer ); it != m_tree.end() )
            it->second->Search( min, max, visit );

        return count;
    }

    /**
     * This one is for tessellated items.  (All shapes in the tree will be from a single
     * BOARD_ITEM.)
     * It checks all items in the bbox overlap to find the minimal actual distance and
     * position.
     */
    bool QueryColliding( const BOX2I& aBox, SHAPE* aRefShape, PCB_LAYER_ID aLayer, int aClearance,
                         int* aActual, VECTOR2I* aPos ) const
    {
        BOX2I bbox = aBox;
        bbox.Inflate( aClearance );

        int min[2] = { bbox.GetX(), bbox.GetY() };
        int max[2] = { bbox.GetRight(), bbox.GetBottom() };

        bool     collision = false;
        int      actual = INT_MAX;
        VECTOR2I pos;

        auto visit =
                [&]( ITEM_WITH_SHAPE* aItem ) -> bool
                {
                    int      curActual;
                    VECTOR2I curPos;

                    if( aRefShape->Collide( aItem->shape, aClearance, &curActual, &curPos ) )
                    {
                        collision = true;

                        if( curActual < actual )
                        {
                            actual = curActual;
                            pos = curPos;
                        }

                        // Stop looking after we have a true collision
                        if( actual <= 0 )
                            return false;
                    }

                    return true;
                };

        if( auto it = m_tree.find( aLayer ); it != m_tree.end() )
            it->second->Search( min, max, visit );

        if( collision )
        {
            if( aActual )
                *aActual = std::max( 0, actual );

            if( aPos )
                *aPos = pos;

            return true;
        }

        return false;
    }

    /**
     * Quicker version of above that just reports a raw yes/no.
     */
    bool QueryColliding( const BOX2I& aBox, SHAPE* aRefShape, PCB_LAYER_ID aLayer ) const
    {
        SHAPE_POLY_SET* poly = dynamic_cast<SHAPE_POLY_SET*>( aRefShape );

        int  min[2] = { aBox.GetX(), aBox.GetY() };
        int  max[2] = { aBox.GetRight(), aBox.GetBottom() };
        bool collision = false;

        // Special case the polygon case.  Otherwise we'll call its Collide() method which will
        // triangulate it as well and then do triangle/triangle collisions.  This ends up being
        // *much* slower than 3 segment Collide()s and a PointInside().
        auto polyVisitor =
                [&]( ITEM_WITH_SHAPE* aItem ) -> bool
                {
                    const SHAPE* shape = aItem->shape;

                    // There are certain degenerate cases that result in empty zone fills, which
                    // will be represented in the rtree with only a root (and no triangles).
                    // https://gitlab.com/kicad/code/kicad/-/issues/18600
                    if( shape->Type() != SH_POLY_SET_TRIANGLE )
                        return true;

                    auto tri = static_cast<const SHAPE_POLY_SET::TRIANGULATED_POLYGON::TRI*>( shape );

                    const SHAPE_LINE_CHAIN& outline = poly->Outline( 0 );

                    for( int ii = 0; ii < (int) tri->GetSegmentCount(); ++ii )
                    {
                        if( outline.Collide( tri->GetSegment( ii ) ) )
                        {
                            collision = true;
                            return false;
                        }
                    }

                    // Also must check for poly being completely inside the triangle
                    if( tri->PointInside( outline.CPoint( 0 ) ) )
                    {
                        collision = true;
                        return false;
                    }

                    return true;
                };

        auto visitor =
                [&]( ITEM_WITH_SHAPE* aItem ) -> bool
                {
                    if( aRefShape->Collide( aItem->shape, 0 ) )
                    {
                        collision = true;
                        return false;
                    }

                    return true;
                };
        auto it = m_tree.find( aLayer );

        if( it == m_tree.end() )
            return false;

        if( poly && poly->OutlineCount() == 1 && poly->HoleCount( 0 ) == 0 )
            it->second->Search( min, max, polyVisitor );
        else
            it->second->Search( min, max, visitor );

        return collision;
    }

    /**
     * Gets the BOARD_ITEMs that overlap the specified point/layer
     * @param aPt Position on the tree
     * @param aLayer Layer to search
     * @return vector of overlapping BOARD_ITEMS*
     */
    std::unordered_set<BOARD_ITEM*> GetObjectsAt( const VECTOR2I& aPt, PCB_LAYER_ID aLayer,
                                                  int aClearance = 0 )
    {
        std::unordered_set<BOARD_ITEM*> retval;
        int min[2] = { aPt.x - aClearance, aPt.y - aClearance };
        int max[2] = { aPt.x + aClearance, aPt.y + aClearance };

        auto visitor =
                [&]( ITEM_WITH_SHAPE* aItem ) -> bool
                {
                    retval.insert( aItem->parent );
                    return true;
                };

        m_tree[aLayer]->Search( min, max, visitor );

        return retval;
    }

    typedef std::pair<PCB_LAYER_ID, PCB_LAYER_ID> LAYER_PAIR;

    struct PAIR_INFO
    {
        PAIR_INFO( LAYER_PAIR aPair, ITEM_WITH_SHAPE* aRef, ITEM_WITH_SHAPE* aTest ) :
            layerPair( aPair ),
            refItem( aRef ),
            testItem( aTest )
        { };

        LAYER_PAIR       layerPair;
        ITEM_WITH_SHAPE* refItem;
        ITEM_WITH_SHAPE* testItem;
    };

    int QueryCollidingPairs( DRC_RTREE* aRefTree, std::vector<LAYER_PAIR> aLayerPairs,
                             std::function<bool( const LAYER_PAIR&, ITEM_WITH_SHAPE*,
                                                 ITEM_WITH_SHAPE*, bool* aCollision )> aVisitor,
                             int aMaxClearance,
                             std::function<bool(int, int )> aProgressReporter ) const
    {
        std::vector<PAIR_INFO> pairsToVisit;

        for( LAYER_PAIR& layerPair : aLayerPairs )
        {
            const PCB_LAYER_ID refLayer = layerPair.first;
            const PCB_LAYER_ID targetLayer = layerPair.second;

            for( ITEM_WITH_SHAPE* refItem : aRefTree->OnLayer( refLayer ) )
            {
                BOX2I box = refItem->shape->BBox();
                box.Inflate( aMaxClearance );

                int min[2] = { box.GetX(),     box.GetY() };
                int max[2] = { box.GetRight(), box.GetBottom() };

                auto visit =
                        [&]( ITEM_WITH_SHAPE* aItemToTest ) -> bool
                        {
                            // don't collide items against themselves
                            if( aItemToTest->parent == refItem->parent )
                                return true;

                            pairsToVisit.emplace_back( layerPair, refItem, aItemToTest );
                            return true;
                        };

                auto it = m_tree.find( targetLayer );

                if( it != m_tree.end() )
                    it->second->Search( min, max, visit );
            };
        }

        // keep track of BOARD_ITEMs pairs that have been already found to collide (some items
        // might be build of COMPOUND/triangulated shapes and a single subshape collision
        // means we have a hit)
        std::unordered_map<PTR_PTR_CACHE_KEY, int> collidingCompounds;

        int progress = 0;
        int count = pairsToVisit.size();

        for( const PAIR_INFO& pair : pairsToVisit )
        {
            if( !aProgressReporter( progress++, count ) )
                break;

            BOARD_ITEM* a = pair.refItem->parent;
            BOARD_ITEM* b = pair.testItem->parent;

            // store canonical order so we don't collide in both directions (a:b and b:a)
            if( static_cast<void*>( a ) > static_cast<void*>( b ) )
                std::swap( a, b );

            // don't report multiple collisions for compound or triangulated shapes
            if( collidingCompounds.count( { a, b } ) )
                continue;

            bool collisionDetected = false;

            if( !aVisitor( pair.layerPair, pair.refItem, pair.testItem, &collisionDetected ) )
                break;

            if( collisionDetected )
                collidingCompounds[ { a, b } ] = 1;
        }

        return 0;
    }

    /**
     * Return the number of items in the tree.
     *
     * @return number of elements in the tree.
     */
    size_t size() const
    {
        return m_count;
    }

    bool empty() const
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

        DRC_LAYER( drc_rtree* aTree, const BOX2I& aRect ) : layer_tree( aTree )
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

    DRC_LAYER OnLayer( PCB_LAYER_ID aLayer ) const
    {
        auto it = m_tree.find( int( aLayer ) );
        return it == m_tree.end() ? DRC_LAYER( nullptr ) : DRC_LAYER( it->second );
    }

    DRC_LAYER Overlapping( PCB_LAYER_ID aLayer, const VECTOR2I& aPoint, int aAccuracy = 0 ) const
    {
        BOX2I rect( aPoint, VECTOR2I( 0, 0 ) );
        rect.Inflate( aAccuracy );
        auto it = m_tree.find( int( aLayer ) );
        return it == m_tree.end() ? DRC_LAYER( nullptr ) : DRC_LAYER( it->second, rect );
    }

    DRC_LAYER Overlapping( PCB_LAYER_ID aLayer, const BOX2I& aRect ) const
    {
        auto it = m_tree.find( int( aLayer ) );
        return it == m_tree.end() ? DRC_LAYER( nullptr ) : DRC_LAYER( it->second, aRect );
    }


private:
    std::map<int, drc_rtree*> m_tree;
    size_t                    m_count;
};


#endif /* DRC_RTREE_H_ */
