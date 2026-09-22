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
 * Regression test for issue #24391: entering a group could reach VIEW::Hide() with a null
 * VIEW_ITEM, which was dereferenced (viewPrivData() on a null this) and crashed. The public
 * mutators must tolerate a null item the same way they already tolerate an item with no view
 * data.
 */

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <gal/graphics_abstraction_layer.h>
#include <view/view.h>
#include <view/view_group.h>

using namespace KIGFX;

namespace
{
class HIDDEN_GAL : public GAL
{
public:
    HIDDEN_GAL( GAL_DISPLAY_OPTIONS& aOptions ) :
            GAL( aOptions )
    {
    }

    bool IsVisible() const override { return false; }
};


class REORDER_TEST_ITEM : public VIEW_ITEM
{
public:
    REORDER_TEST_ITEM( int aLayer ) :
            m_layer( aLayer )
    {
    }

    wxString GetClass() const override { return wxT( "REORDER_TEST_ITEM" ); }

    const BOX2I ViewBBox() const override { return BOX2I( VECTOR2I( 0, 0 ), VECTOR2I( 100, 100 ) ); }

    std::vector<int> ViewGetLayers() const override { return { m_layer }; }

private:
    int m_layer;
};
} // namespace


BOOST_AUTO_TEST_SUITE( ViewNullSafety )


BOOST_AUTO_TEST_CASE( HideAndSetVisibleTolerateNullItem )
{
    VIEW view;

    // Before the fix each of these dereferenced the null item via viewPrivData() and aborted.
    view.Hide( nullptr, true );
    view.Hide( nullptr, false, true );
    view.SetVisible( nullptr, true );
    view.SetVisible( nullptr, false );

    // Reaching this point without crashing is the assertion.
    BOOST_TEST( true );
}


BOOST_AUTO_TEST_CASE( ViewGroupRejectsNullItem )
{
    // Issue #24778: a re-entrant ExitGroup() during SCH_SELECTION_TOOL::EnterGroup() left the
    // group overlay with a null (stale) member. VIEW_GROUP stored it, then the next repaint
    // dereferenced it in ViewBBox()/ViewDraw(). Add() must drop a null the same way the VIEW
    // mutators do.
    VIEW_GROUP group;

    group.Add( nullptr );

    BOOST_TEST( group.GetSize() == 0u );

    // Without the guard this iterated m_groupItems[0] on a null item and crashed.
    group.ViewBBox();
}


BOOST_AUTO_TEST_CASE( ReorderLayerDataNonPermutationKeepsLayerTrees )
{
    GAL_DISPLAY_OPTIONS options;
    HIDDEN_GAL          gal( options );
    VIEW                view;

    view.SetGAL( &gal );
    view.ReorderLayerData( { { 11, 10 }, { 12, 11 } } );

    REORDER_TEST_ITEM item( 12 );
    view.Add( &item );

    std::vector<VIEW::LAYER_ITEM_PAIR> found;
    view.Query( BOX2I( VECTOR2I( -1, -1 ), VECTOR2I( 200, 200 ) ), found );

    bool itemFound = false;

    for( const VIEW::LAYER_ITEM_PAIR& pair : found )
    {
        if( pair.first == &item )
            itemFound = true;
    }

    BOOST_TEST( itemFound );

    view.Remove( &item );
}


BOOST_AUTO_TEST_SUITE_END()
