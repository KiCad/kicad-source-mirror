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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

/**
 * @file
 * Regression test for the VIEW R-tree corruption behind the frequent 10.0.x kigal.dll crash
 * (RTree::RemoveAllRec deref of a dangling node). VIEW::updateBbox() removed the item from the
 * per-layer R-tree using a bbox that aliased the cached bbox it had just overwritten, so the
 * removal was issued with the new box. The Greene R-tree only descends into subtrees overlapping
 * the supplied box, so when an item moves far enough that old and new boxes do not overlap the
 * removal silently misses and the following Insert leaves a second, stale entry. The duplicate
 * eventually becomes a dangling pointer once the item is freed.
 *
 * The R-tree's leaf match is by pointer with no overlap gate, so the defect only manifests once
 * the tree has internal nodes; the test inserts well over MAXNODES items to force that.
 */

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <gal/gal_display_options.h>
#include <gal/graphics_abstraction_layer.h>
#include <math/box2.h>
#include <view/view.h>
#include <view/view_item.h>

using namespace KIGFX;

namespace
{
constexpr int TEST_LAYER = 0;

/// VIEW::UpdateItems() requires a GAL reporting visible + initialized (both default true on the
/// base class). No drawing is exercised because the test keeps every layer non-cached.
class STUB_GAL : public GAL
{
public:
    explicit STUB_GAL( GAL_DISPLAY_OPTIONS& aOptions ) : GAL( aOptions ) {}
};

/// VIEW item whose bounding box can be moved after insertion, to drive updateBbox() with a box
/// that does not overlap the one it was inserted with.
class MOVABLE_ITEM : public VIEW_ITEM
{
public:
    explicit MOVABLE_ITEM( const BOX2I& aBox ) : m_bbox( aBox ) {}

    wxString GetClass() const override { return wxT( "MOVABLE_ITEM" ); }
    const BOX2I ViewBBox() const override { return m_bbox; }
    std::vector<int> ViewGetLayers() const override { return { TEST_LAYER }; }

    void SetBBox( const BOX2I& aBox ) { m_bbox = aBox; }

private:
    BOX2I m_bbox;
};


int countItemEntries( VIEW& aView, const BOX2I& aRect, const VIEW_ITEM* aItem )
{
    int count = 0;

    aView.Query( aRect,
                 [&]( VIEW_ITEM* aFound )
                 {
                     if( aFound == aItem )
                         count++;

                     return true;
                 } );

    return count;
}
} // namespace


BOOST_AUTO_TEST_SUITE( ViewRTreeStaleBbox )


BOOST_AUTO_TEST_CASE( UpdateBboxRemovesOldEntryOnFarMove )
{
    GAL_DISPLAY_OPTIONS opts;
    STUB_GAL            gal( opts );

    VIEW view;
    view.SetGAL( &gal );

    // Keep everything non-cached so updateItemGeometry() (which would need a painter) is skipped.
    for( int layer = 0; layer < VIEW::VIEW_MAX_LAYERS; ++layer )
        view.SetLayerTarget( layer, TARGET_NONCACHED );

    const BOX2I nearBox( VECTOR2I( 0, 0 ), VECTOR2I( 1000, 1000 ) );

    MOVABLE_ITEM target( nearBox );
    view.Add( &target );

    // Enough clustered neighbours near the origin to force internal R-tree nodes, so removing the
    // target requires the overlap-gated descent that the aliasing bug defeats.
    std::vector<std::unique_ptr<MOVABLE_ITEM>> fillers;

    for( int i = 0; i < 200; ++i )
    {
        const VECTOR2I pos( ( i % 20 ) * 3000, ( i / 20 ) * 3000 );
        fillers.push_back( std::make_unique<MOVABLE_ITEM>( BOX2I( pos, VECTOR2I( 1000, 1000 ) ) ) );
        view.Add( fillers.back().get() );
    }

    // Flush the INITIAL_ADD updates queued by Add().
    view.UpdateItems();

    BOOST_REQUIRE_EQUAL( countItemEntries( view, nearBox, &target ), 1 );

    // Move the target far enough that its new box shares no area with the cluster it lived in. A
    // single changed item keeps the changed ratio well under the bulk-rebuild threshold, so the
    // update is routed through updateBbox().
    const BOX2I farBox( VECTOR2I( 10000000, 10000000 ), VECTOR2I( 1000, 1000 ) );
    target.SetBBox( farBox );
    view.Update( &target, GEOMETRY );
    view.UpdateItems();

    const BOX2I everything( VECTOR2I( -1000000, -1000000 ), VECTOR2I( 20000000, 20000000 ) );

    // With the aliasing bug the stale near-origin entry survives, so the target is indexed twice.
    BOOST_TEST( countItemEntries( view, everything, &target ) == 1 );
    BOOST_TEST( countItemEntries( view, nearBox, &target ) == 0 );

    view.Remove( &target );

    for( auto& filler : fillers )
        view.Remove( filler.get() );
}


BOOST_AUTO_TEST_SUITE_END()
