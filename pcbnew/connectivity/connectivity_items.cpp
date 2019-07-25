/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2016-2018 CERN
 * Copyright (C) 2019  KiCad Developers, see AUTHORS.txt for contributors.
 *
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

#include <connectivity/connectivity_items.h>

int CN_ITEM::AnchorCount() const
{
    if( !m_valid )
        return 0;

    return m_parent->Type() == PCB_TRACE_T ? 2 : 1;
}


const VECTOR2I CN_ITEM::GetAnchor( int n ) const
{
    if( !m_valid )
        return VECTOR2I();

    switch( m_parent->Type() )
    {
        case PCB_PAD_T:
            return static_cast<const D_PAD*>( m_parent )->ShapePos();
            break;

        case PCB_TRACE_T:
        {
            auto tr = static_cast<const TRACK*>( m_parent );
            return ( n == 0 ? tr->GetStart() : tr->GetEnd() );

            break;
        }

        case PCB_VIA_T:
            return static_cast<const VIA*>( m_parent )->GetStart();

        default:
            assert( false );
            return VECTOR2I();
    }
}


int CN_ITEM::Net() const
{
    if( !m_parent || !m_valid )
        return -1;

    return m_parent->GetNetCode();
}


void CN_ITEM::Dump()
{
    printf("    valid: %d, connected: \n", !!Valid());

    for( auto i : m_connected )
    {
        TRACK* t = static_cast<TRACK*>( i->Parent() );
        printf( "    - %p %d\n", t, t->Type() );
    }
}


int CN_ZONE::AnchorCount() const
{
    if( !Valid() )
        return 0;

    const auto zone = static_cast<const ZONE_CONTAINER*>( Parent() );
    const auto& outline = zone->GetFilledPolysList().COutline( m_subpolyIndex );

    return outline.PointCount() ? 1 : 0;
}


const VECTOR2I CN_ZONE::GetAnchor( int n ) const
{
    if( !Valid() )
        return VECTOR2I();

    const auto zone = static_cast<const ZONE_CONTAINER*> ( Parent() );
    const auto& outline = zone->GetFilledPolysList().COutline( m_subpolyIndex );

    return outline.CPoint( 0 );
}


void CN_ITEM::RemoveInvalidRefs()
{
    for( auto it = m_connected.begin(); it != m_connected.end(); )
    {
        if( !(*it)->Valid() )
            it = m_connected.erase( it );
        else
            ++it;
    }
}


CN_ITEM* CN_LIST::Add( D_PAD* pad )
 {
    if( !pad->IsOnCopperLayer() )
         return nullptr;

     auto item = new CN_ITEM( pad, false, 1 );
     item->AddAnchor( pad->ShapePos() );
     item->SetLayers( LAYER_RANGE( F_Cu, B_Cu ) );

     switch( pad->GetAttribute() )
     {
     case PAD_ATTRIB_SMD:
     case PAD_ATTRIB_HOLE_NOT_PLATED:
     case PAD_ATTRIB_CONN:
     {
         LSET lmsk = pad->GetLayerSet();

         for( int i = 0; i <= MAX_CU_LAYERS; i++ )
         {
             if( lmsk[i] )
             {
                 item->SetLayer( i );
                 break;
             }
         }
         break;
     }
     default:
         break;
     }

     addItemtoTree( item );
     m_items.push_back( item );
     SetDirty();
     return item;
}

 CN_ITEM* CN_LIST::Add( TRACK* track )
 {
     auto item = new CN_ITEM( track, true );
     m_items.push_back( item );
     item->AddAnchor( track->GetStart() );
     item->AddAnchor( track->GetEnd() );
     item->SetLayer( track->GetLayer() );
     addItemtoTree( item );
     SetDirty();
     return item;
 }

 CN_ITEM* CN_LIST::Add( VIA* via )
 {
     auto item = new CN_ITEM( via, true, 1 );

     m_items.push_back( item );
     item->AddAnchor( via->GetStart() );
     item->SetLayers( LAYER_RANGE( F_Cu, B_Cu ) );
     addItemtoTree( item );
     SetDirty();
     return item;
 }

 const std::vector<CN_ITEM*> CN_LIST::Add( ZONE_CONTAINER* zone )
 {
     const auto& polys = zone->GetFilledPolysList();

     std::vector<CN_ITEM*> rv;

     for( int j = 0; j < polys.OutlineCount(); j++ )
     {
         CN_ZONE* zitem = new CN_ZONE( zone, false, j );
         const auto& outline = zone->GetFilledPolysList().COutline( j );

         for( int k = 0; k < outline.PointCount(); k++ )
             zitem->AddAnchor( outline.CPoint( k ) );

         m_items.push_back( zitem );
         zitem->SetLayer( zone->GetLayer() );
         addItemtoTree( zitem );
         rv.push_back( zitem );
         SetDirty();
     }

     return rv;
 }


void CN_LIST::RemoveInvalidItems( std::vector<CN_ITEM*>& aGarbage )
{
    if( !m_hasInvalid )
        return;

    auto lastItem = std::remove_if(m_items.begin(), m_items.end(), [&aGarbage] ( CN_ITEM* item )
    {
        if( !item->Valid() )
        {
            aGarbage.push_back ( item );
            return true;
        }

        return false;
    } );

    m_items.resize( lastItem - m_items.begin() );

    for( auto item : m_items )
        item->RemoveInvalidRefs();

    for( auto item : aGarbage )
        m_index.Remove( item );

    m_hasInvalid = false;
}


BOARD_CONNECTED_ITEM* CN_ANCHOR::Parent() const
{
    assert( m_item->Valid() );
    return m_item->Parent();
}


bool CN_ANCHOR::Valid() const
{
    if( !m_item )
        return false;

    return m_item->Valid();
}


bool CN_ANCHOR::IsDangling() const
{
    if( !m_cluster )
        return true;

    // the minimal number of items connected to item_ref
    // at this anchor point to decide the anchor is *not* dangling
    size_t minimal_count = 1;
    size_t connected_count = m_item->ConnectedItems().size();

    // a via can be removed if connected to only one other item.
    if( Parent()->Type() == PCB_VIA_T )
        return connected_count < 2;

    if( m_item->AnchorCount() == 1 )
        return connected_count < minimal_count;

    // Items with multiple anchors have usually items connected to each anchor.
    // We want only the item count of this anchor point
    connected_count = 0;
    for( auto item : m_item->ConnectedItems() )
    {
        if( item->Parent()->Type() == PCB_ZONE_AREA_T )
        {
            ZONE_CONTAINER* zone = static_cast<ZONE_CONTAINER*>( item->Parent() );

            if( zone->HitTestFilledArea( wxPoint( Pos().x, Pos().y ) ) )
                connected_count++;
        }
        else if( item->Parent()->HitTest( wxPoint( Pos().x, Pos().y ) ) )
            connected_count++;
    }

    return connected_count < minimal_count;
}


int CN_ANCHOR::ConnectedItemsCount() const
{
    if( !m_cluster )
        return 0;

    int connected_count = 0;

    for( auto item : m_item->ConnectedItems() )
    {
        if( item->Parent()->Type() == PCB_ZONE_AREA_T )
        {
            ZONE_CONTAINER* zone = static_cast<ZONE_CONTAINER*>( item->Parent() );

            if( zone->HitTestFilledArea( wxPoint( Pos().x, Pos().y ) ) )
                connected_count++;
        }
        else if( item->Parent()->HitTest( wxPoint( Pos().x, Pos().y ) ) )
            connected_count++;
    }

    return connected_count;
}


CN_CLUSTER::CN_CLUSTER()
{
    m_items.reserve( 64 );
    m_originPad = nullptr;
    m_originNet = -1;
    m_conflicting = false;
}


CN_CLUSTER::~CN_CLUSTER()
{
}


wxString CN_CLUSTER::OriginNetName() const
{
    if( !m_originPad || !m_originPad->Valid() )
        return "<none>";
    else
        return m_originPad->Parent()->GetNetname();
}


bool CN_CLUSTER::Contains( const CN_ITEM* aItem )
{
    return std::find( m_items.begin(), m_items.end(), aItem ) != m_items.end();
}


bool CN_CLUSTER::Contains( const BOARD_CONNECTED_ITEM* aItem )
{
    return std::find_if( m_items.begin(), m_items.end(), [ &aItem ] ( const CN_ITEM* item )
            { return item->Valid() && item->Parent() == aItem; } ) != m_items.end();
}


void CN_CLUSTER::Dump()
{
    for( auto item : m_items )
    {
        wxLogTrace( "CN", " - item : %p bitem : %p type : %d inet %s\n", item, item->Parent(),
                item->Parent()->Type(), (const char*) item->Parent()->GetNetname().c_str() );
        printf( "- item : %p bitem : %p type : %d inet %s\n", item, item->Parent(),
                        item->Parent()->Type(), (const char*) item->Parent()->GetNetname().c_str() );
        item->Dump();
    }
}


void CN_CLUSTER::Add( CN_ITEM* item )
{
    m_items.push_back( item );

    if( item->Net() <= 0 )
        return;

    if( m_originNet <= 0 )
    {
        m_originNet = item->Net();
    }

    if( item->Parent()->Type() == PCB_PAD_T )
    {
        if( !m_originPad )
        {
            m_originPad = item;
            m_originNet = item->Net();
        }

        if( m_originPad && item->Net() != m_originNet )
        {
            m_conflicting = true;
        }
    }
}
