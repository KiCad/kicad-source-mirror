/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <board_item.h>
#include <track.h>
#include <unordered_set>
#include <set>
#include <vector>

#include <geometry/rtree.h>
#include <math/vector2d.h>

/**
 * DRC_RTREE -
 * Implements an R-tree for fast spatial and layer indexing of connectable items.
 * Non-owning.
 */
class DRC_RTREE
{

public:

    struct ITEM_WITH_SHAPE
    {
        ITEM_WITH_SHAPE( BOARD_ITEM *aParent, SHAPE* aShape,
                         std::shared_ptr<SHAPE> aParentShape = nullptr ) :
            parent ( aParent ),
            shape ( aShape ),
            parentShape( aParentShape )
        {};

        BOARD_ITEM* parent;
        SHAPE* shape;
        std::shared_ptr<SHAPE> parentShape;
    };

private:

    using drc_rtree = RTree<ITEM_WITH_SHAPE*, int, 2, double>;

public:

    DRC_RTREE()
    {
        for( int layer : LSET::AllLayersMask().Seq() )
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
     * Inserts an item into the tree.
     */
    void Insert( BOARD_ITEM* aItem, int aWorstClearance = 0, int aLayer = UNDEFINED_LAYER )
    {
        std::vector<SHAPE*> subshapes;

        auto addLayer =
                [&]( PCB_LAYER_ID layer )
                {
                    std::shared_ptr<SHAPE> shape = aItem->GetEffectiveShape( layer );
                    subshapes.clear();

                    if( shape->HasIndexableSubshapes() )
                        shape->GetIndexableSubshapes( subshapes );
                    else
                        subshapes.push_back( shape.get() );

                    for( SHAPE* subshape : subshapes )
                    {
                        BOX2I bbox = subshape->BBox();

                        bbox.Inflate( aWorstClearance );

                        const int mmin[2] = { bbox.GetX(), bbox.GetY() };
                        const int mmax[2] = { bbox.GetRight(), bbox.GetBottom() };

                        m_tree[layer]->Insert( mmin, mmax, new ITEM_WITH_SHAPE( aItem, subshape,
                                                                                shape ) );
                        m_count++;
                    }
                };

        if( aItem->Type() == PCB_FP_TEXT_T && !static_cast<FP_TEXT*>( aItem )->IsVisible() )
            return;

        if( aLayer != UNDEFINED_LAYER )
        {
            addLayer( (PCB_LAYER_ID) aLayer );
        }
        else
        {
            LSET layers = aItem->GetLayerSet();

            // Special-case pad holes which pierce all the copper layers
            if( aItem->Type() == PCB_PAD_T )
            {
                PAD* pad = static_cast<PAD*>( aItem );

                if( pad->GetDrillSizeX() > 0 && pad->GetDrillSizeY() > 0 )
                    layers |= LSET::AllCuMask();
            }

            for( int layer : layers.Seq() )
                addLayer( (PCB_LAYER_ID) layer );
        }
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

        this->m_tree[aTargetLayer]->Search( min, max, visit );
        return count > 0;
    }

    /**
     * This is a fast test which essentially does bounding-box overlap given a worst-case
     * clearance.  It's used when looking up the specific item-to-item clearance might be
     * expensive and should be deferred till we know we have a possible hit.
     */
    int QueryColliding( BOARD_ITEM* aRefItem,
                        PCB_LAYER_ID aRefLayer,
                        PCB_LAYER_ID aTargetLayer,
                        std::function<bool( BOARD_ITEM* )> aFilter = nullptr,
                        std::function<bool( BOARD_ITEM* )> aVisitor = nullptr,
                        int aClearance = 0 ) const
    {
        // keep track of BOARD_ITEMs that have been already found to collide (some items
        // might be build of COMPOUND/triangulated shapes and a single subshape collision
        // means we have a hit)
        std::unordered_set<BOARD_ITEM*> collidingCompounds;

        EDA_RECT box = aRefItem->GetBoundingBox();
        box.Inflate( aClearance );

        int min[2] = { box.GetX(),         box.GetY() };
        int max[2] = { box.GetRight(),     box.GetBottom() };

        std::shared_ptr<SHAPE> refShape = aRefItem->GetEffectiveShape( aRefLayer );

        int count = 0;

        auto visit =
                [&]( ITEM_WITH_SHAPE* aItem ) -> bool
                {
                    if( aItem->parent == aRefItem )
                        return true;

                    if( collidingCompounds.find( aItem->parent ) != collidingCompounds.end() )
                        return true;

                    if( !aFilter || aFilter( aItem->parent ) )
                    {
                        if( refShape->Collide( aItem->shape, aClearance ) )
                        {
                            collidingCompounds.insert( aItem->parent );
                            count++;

                            if( aVisitor )
                                return aVisitor( aItem->parent );
                        }
                    }

                    return true;
                };

        this->m_tree[aTargetLayer]->Search( min, max, visit );
        return count;
    }

    /**
     * This one is for tessellated items.  (All shapes in the tree will be from a single
     * BOARD_ITEM.)
     * It checks all items in the bbox overlap to find the minimal actual distance and
     * position.
     */
    bool QueryColliding( EDA_RECT aBox, SHAPE* aRefShape, PCB_LAYER_ID aLayer, int aClearance,
                         int* aActual, VECTOR2I* aPos ) const
    {
        aBox.Inflate( aClearance );

        int min[2] = { aBox.GetX(), aBox.GetY() };
        int max[2] = { aBox.GetRight(), aBox.GetBottom() };

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
                    }

                    return true;
                };

        this->m_tree[aLayer]->Search( min, max, visit );

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

    int QueryCollidingPairs( DRC_RTREE* aRefTree,
                             std::vector<LAYER_PAIR> aLayerPairs,
                             std::function<bool( const LAYER_PAIR&,
                                                 ITEM_WITH_SHAPE*, ITEM_WITH_SHAPE*,
                                                 bool* aCollision )> aVisitor,
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

                this->m_tree[targetLayer]->Search( min, max, visit );
            };
        }

        // keep track of BOARD_ITEMs pairs that have been already found to collide (some items
        // might be build of COMPOUND/triangulated shapes and a single subshape collision
        // means we have a hit)
        std::map< std::pair<BOARD_ITEM*, BOARD_ITEM*>, int> collidingCompounds;

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
     * Returns the number of items in the tree
     * @return number of elements in the tree;
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
    drc_rtree*  m_tree[PCB_LAYER_ID_COUNT];
    size_t      m_count;
};


#endif /* DRC_RTREE_H_ */
