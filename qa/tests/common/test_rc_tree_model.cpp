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

#include <boost/test/unit_test.hpp>

#include <memory>

#include <rc_item.h>
#include <marker_base.h>


class TEST_RC_TREE_MODEL : public RC_TREE_MODEL
{
public:
    TEST_RC_TREE_MODEL() :
            RC_TREE_MODEL( nullptr, nullptr )
    {
    }

    RC_TREE_NODE* AddMarkerWithMainItem()
    {
        auto rcItem = std::make_shared<RC_ITEM>();
        rcItem->SetItems( KIID(), niluuid, niluuid, niluuid );

        auto marker = createNode( nullptr, rcItem, RC_TREE_NODE::MARKER );
        auto child = createNode( marker, rcItem, RC_TREE_NODE::MAIN_ITEM );

        marker->m_Children.push_back( child );
        m_tree.push_back( marker );
        return marker;
    }

    RC_TREE_NODE* AddItemlessMarker( const std::shared_ptr<RC_ITEM>& aRcItem )
    {
        auto marker = createNode( nullptr, aRcItem, RC_TREE_NODE::MARKER );
        m_tree.push_back( marker );
        return marker;
    }

    void DetachTopLevel( RC_TREE_NODE* aNode )
    {
        std::erase( m_tree, aNode );
        retireNodeTree( aNode );
        deleteNodeTree( aNode );
    }
};


// Concrete marker so the rc-item has a parent.
class TEST_MARKER : public MARKER_BASE
{
public:
    TEST_MARKER( std::shared_ptr<RC_ITEM> aItem ) :
            MARKER_BASE( 0, std::move( aItem ) )
    {
    }

    const KIID     GetUUID() const override { return m_uuid; }
    KIGFX::COLOR4D getColor() const override { return KIGFX::COLOR4D(); }

    KIID m_uuid;
};


BOOST_AUTO_TEST_SUITE( RCTreeModel )


BOOST_AUTO_TEST_CASE( DetachedHandlesBehaveAsInvalid )
{
    TEST_RC_TREE_MODEL model;
    RC_TREE_NODE*      marker = model.AddMarkerWithMainItem();
    RC_TREE_NODE*      child = marker->m_Children.front();

    const wxDataViewItem staleMarker = RC_TREE_MODEL::ToItem( marker );
    const wxDataViewItem staleChild = RC_TREE_MODEL::ToItem( child );

    model.DetachTopLevel( marker );

    wxDataViewItemArray children;

    BOOST_CHECK( !model.IsContainer( staleMarker ) );
    BOOST_CHECK( !model.GetParent( staleChild ).IsOk() );
    BOOST_CHECK_EQUAL( model.GetChildren( staleMarker, children ), 0U );
    BOOST_CHECK( children.empty() );
    BOOST_CHECK( RC_TREE_MODEL::ToUUID( staleChild ) == niluuid );
}


BOOST_AUTO_TEST_CASE( ValueChangedItemlessMarkerDoesNotCrash )
{
    TEST_RC_TREE_MODEL model;

    auto        rcItem = std::make_shared<RC_ITEM>(); // no items -> marker node has no children
    TEST_MARKER marker( rcItem );
    rcItem->SetParent( &marker );

    RC_TREE_NODE* node = model.AddItemlessMarker( rcItem );
    BOOST_REQUIRE( node->m_Children.empty() );

    // Used to call vector::back() on the empty child list here and crash.
    model.ValueChanged( node );

    // Excluding with a comment must append a comment node even though there were no children.
    marker.SetExcluded( true, wxT( "excluded" ) );
    model.ValueChanged( node );

    BOOST_REQUIRE_EQUAL( node->m_Children.size(), 1u );
    BOOST_CHECK( node->m_Children[0]->m_Type == RC_TREE_NODE::COMMENT );
}


BOOST_AUTO_TEST_SUITE_END()
