/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2006 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
 * Copyright (C) 1992-2024 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <pgm_base.h>
#include <settings/settings_manager.h>
#include <eeschema_settings.h>
#include <eda_item.h>
#include <sch_connection.h>
#include <sch_item.h>
#include <sch_screen.h>
#include <sch_sheet_path.h>
#include <sch_draw_panel.h>
#include <sch_edit_frame.h>
#include <schematic.h>
#include <symbol.h>
#include <trace_helpers.h>
#include <general.h>
#include <netclass.h>
#include <project/project_file.h>
#include <project/net_settings.h>


// Rendering fonts is expensive (particularly when using outline fonts).  At small effective
// sizes (ie: zoomed out) the visual differences between outline and/or stroke fonts and the
// bitmap font becomes immaterial, and there's often more to draw when zoomed out so the
// performance gain becomes more significant.
#define BITMAP_FONT_SIZE_THRESHOLD 3


/* Constructor and destructor for SCH_ITEM */
/* They are not inline because this creates problems with gcc at linking time in debug mode */

SCH_ITEM::SCH_ITEM( EDA_ITEM* aParent, KICAD_T aType ) :
    EDA_ITEM( aParent, aType )
{
    m_layer              = LAYER_WIRE;   // It's only a default, in fact
    m_fieldsAutoplaced   = FIELDS_AUTOPLACED_NO;
    m_connectivity_dirty = false;        // Item is unconnected until it is placed, so it's clean
}


SCH_ITEM::SCH_ITEM( const SCH_ITEM& aItem ) :
    EDA_ITEM( aItem )
{
    m_layer              = aItem.m_layer;
    m_fieldsAutoplaced   = aItem.m_fieldsAutoplaced;
    m_connectivity_dirty = aItem.m_connectivity_dirty;
}


SCH_ITEM& SCH_ITEM::operator=( const SCH_ITEM& aItem )
{
    m_layer              = aItem.m_layer;
    m_fieldsAutoplaced   = aItem.m_fieldsAutoplaced;
    m_connectivity_dirty = aItem.m_connectivity_dirty;

    return *this;
}


SCH_ITEM::~SCH_ITEM()
{
    for( const auto& it : m_connection_map )
        delete it.second;
}


SCH_ITEM* SCH_ITEM::Duplicate( bool doClone ) const
{
    SCH_ITEM* newItem = (SCH_ITEM*) Clone();

    if( !doClone )
        const_cast<KIID&>( newItem->m_Uuid ) = KIID();

    newItem->ClearFlags( SELECTED | BRIGHTENED );

    newItem->RunOnChildren(
            []( SCH_ITEM* aChild )
            {
                aChild->ClearFlags( SELECTED | BRIGHTENED );
            } );

    return newItem;
}


SCHEMATIC* SCH_ITEM::Schematic() const
{
    EDA_ITEM* parent = GetParent();

    while( parent )
    {
        if( parent->Type() == SCHEMATIC_T )
            return static_cast<SCHEMATIC*>( parent );
        else
            parent = parent->GetParent();
    }

    return nullptr;
}


const SYMBOL* SCH_ITEM::GetParentSymbol() const
{
    wxCHECK( m_parent->Type() == SCH_SYMBOL_T, nullptr );

    return static_cast<const SCH_SYMBOL*>( m_parent );
}


SYMBOL* SCH_ITEM::GetParentSymbol()
{
    wxCHECK( m_parent->Type() == SCH_SYMBOL_T, nullptr );

    return static_cast<SCH_SYMBOL*>( m_parent );
}


void SCH_ITEM::ViewGetLayers( int aLayers[], int& aCount ) const
{
    // Basic fallback
    aCount     = 2;
    aLayers[0] = LAYER_DEVICE;
    aLayers[1] = LAYER_SELECTION_SHADOWS;
}


bool SCH_ITEM::IsConnected( const VECTOR2I& aPosition ) const
{
    if(( m_flags & STRUCT_DELETED ) || ( m_flags & SKIP_STRUCT ) )
        return false;

    return doIsConnected( aPosition );
}


SCH_CONNECTION* SCH_ITEM::Connection( const SCH_SHEET_PATH* aSheet ) const
{
    if( !IsConnectable() )
        return nullptr;

    if( !aSheet )
        aSheet = &Schematic()->CurrentSheet();

    auto it = m_connection_map.find( *aSheet );

    if( it == m_connection_map.end() )
        return nullptr;
    else
        return it->second;
}


void SCH_ITEM::SetConnectionGraph( CONNECTION_GRAPH* aGraph )
{
    for( auto& [path, conn] : m_connection_map )
    {
        conn->SetGraph( aGraph );

        for( auto& member : conn->AllMembers() )
            member->SetGraph( aGraph );
    }
}


std::shared_ptr<NETCLASS> SCH_ITEM::GetEffectiveNetClass( const SCH_SHEET_PATH* aSheet ) const
{
    static std::shared_ptr<NETCLASS> nullNetclass = std::make_shared<NETCLASS>( wxEmptyString );

    SCHEMATIC* schematic = Schematic();

    if( schematic )
    {
        std::shared_ptr<NET_SETTINGS>& netSettings = schematic->Prj().GetProjectFile().m_NetSettings;
        SCH_CONNECTION*                connection = Connection( aSheet );

        if( connection )
            return netSettings->GetEffectiveNetClass( connection->Name() );
        else
            return netSettings->m_DefaultNetClass;
    }

    return nullNetclass;
}


SCH_ITEM_SET& SCH_ITEM::ConnectedItems( const SCH_SHEET_PATH& aSheet )
{
    return m_connected_items[ aSheet ];
}


void SCH_ITEM::AddConnectionTo( const SCH_SHEET_PATH& aSheet, SCH_ITEM* aItem )
{
    SCH_ITEM_SET& set = m_connected_items[ aSheet ];

    // The vector elements are small, so reserve 1k at a time to prevent re-allocations
    if( set.size() == set.capacity() )
        set.reserve( set.size() + 4096 );

    set.emplace_back( aItem );
}


SCH_CONNECTION* SCH_ITEM::InitializeConnection( const SCH_SHEET_PATH& aSheet,
                                                CONNECTION_GRAPH* aGraph )
{
    SCH_CONNECTION* connection = Connection( &aSheet );

    // N.B. Do not clear the dirty connectivity flag here because we may need
    // to create a connection for a different sheet, and we don't want to
    // skip the connection creation because the flag is cleared.
    if( connection )
    {
        connection->Reset();
    }
    else
    {
        connection = new SCH_CONNECTION( this );
        m_connection_map.insert( std::make_pair( aSheet, connection ) );
    }

    connection->SetGraph( aGraph );
    connection->SetSheet( aSheet );
    return connection;
}


SCH_CONNECTION* SCH_ITEM::GetOrInitConnection( const SCH_SHEET_PATH& aSheet,
                                               CONNECTION_GRAPH* aGraph )
{
    if( !IsConnectable() )
        return nullptr;

    SCH_CONNECTION* connection = Connection( &aSheet );

    if( connection )
        return connection;
    else
        return InitializeConnection( aSheet, aGraph );
}


const wxString& SCH_ITEM::GetCachedDriverName() const
{
    static wxString s_empty;
    return s_empty;
}


void SCH_ITEM::SwapData( SCH_ITEM* aItem )
{
    UNIMPLEMENTED_FOR( GetClass() );
}


void SCH_ITEM::SwapFlags( SCH_ITEM* aItem )
{
    EDA_ITEM_FLAGS editFlags = GetEditFlags();
    EDA_ITEM_FLAGS tempFlags = GetTempFlags();
    EDA_ITEM_FLAGS aItem_editFlags = aItem->GetEditFlags();
    EDA_ITEM_FLAGS aItem_tempFlags = aItem->GetTempFlags();

    std::swap( m_flags, aItem->m_flags );

    ClearEditFlags();
    SetFlags( editFlags );
    ClearTempFlags();
    SetFlags( tempFlags );

    aItem->ClearEditFlags();
    aItem->SetFlags( aItem_editFlags );
    aItem->ClearTempFlags();
    aItem->SetFlags( aItem_tempFlags );
}


void SCH_ITEM::ClearCaches()
{
    auto clearTextCaches =
            []( SCH_ITEM* aItem )
            {
                EDA_TEXT* text = dynamic_cast<EDA_TEXT*>( aItem );

                if( text )
                {
                    text->ClearBoundingBoxCache();
                    text->ClearRenderCache();
                }
            };

    clearTextCaches( this );

    RunOnChildren( clearTextCaches );
}


bool SCH_ITEM::operator < ( const SCH_ITEM& aItem ) const
{
    if( Type() != aItem.Type() )
        return Type() < aItem.Type();

    if( GetPosition().x != aItem.GetPosition().x )
        return GetPosition().x < aItem.GetPosition().x;

    if( GetPosition().y != aItem.GetPosition().y )
        return GetPosition().y < aItem.GetPosition().y;

    return m_Uuid < aItem.m_Uuid;
}


const wxString& SCH_ITEM::GetDefaultFont() const
{
    EESCHEMA_SETTINGS* cfg = Pgm().GetSettingsManager().GetAppSettings<EESCHEMA_SETTINGS>();

    return cfg->m_Appearance.default_font;
}


const KIFONT::METRICS& SCH_ITEM::GetFontMetrics() const
{
    if( SCHEMATIC* schematic = Schematic() )
        return schematic->Settings().m_FontMetrics;

    return KIFONT::METRICS::Default();
}


bool SCH_ITEM::RenderAsBitmap( double aWorldScale ) const
{
    if( IsHypertext() )
        return false;

    if( const EDA_TEXT* text = dynamic_cast<const EDA_TEXT*>( this ) )
        return text->GetTextHeight() * aWorldScale < BITMAP_FONT_SIZE_THRESHOLD;

    return false;
}


static struct SCH_ITEM_DESC
{
    SCH_ITEM_DESC()
    {
#ifdef NOTYET
        ENUM_MAP<SCH_LAYER_ID>& layerEnum = ENUM_MAP<SCH_LAYER_ID>::Instance();

        if( layerEnum.Choices().GetCount() == 0 )
        {
            layerEnum.Undefined( SCH_LAYER_ID_END );

            for( SCH_LAYER_ID value : magic_enum::enum_values<SCH_LAYER_ID>() )
                layerEnum.Map( value, LayerName( value ) );
        }
#endif

        PROPERTY_MANAGER& propMgr = PROPERTY_MANAGER::Instance();
        REGISTER_TYPE( SCH_ITEM );
        propMgr.InheritsAfter( TYPE_HASH( SCH_ITEM ), TYPE_HASH( EDA_ITEM ) );

#ifdef NOTYET
        // Not sure if this will ever be needed
        propMgr.AddProperty( new PROPERTY_ENUM<SCH_ITEM, SCH_LAYER_ID>( _HKI( "Layer" ),
                &SCH_ITEM::SetLayer, &SCH_ITEM::GetLayer ) )
                .SetIsHiddenFromPropertiesManager();
#endif

#ifdef NOTYET
        // Not yet functional in UI
        propMgr.AddProperty( new PROPERTY<SCH_ITEM, bool>( _HKI( "Locked" ),
                &SCH_ITEM::SetLocked, &SCH_ITEM::IsLocked ) );
#endif
    }
} _SCH_ITEM_DESC;

IMPLEMENT_ENUM_TO_WXANY( SCH_LAYER_ID )


static bool lessYX( const DANGLING_END_ITEM& a, const DANGLING_END_ITEM& b )
{
    const auto aPos = a.GetPosition();
    const auto bPos = b.GetPosition();
    return aPos.y < bPos.y ? true : ( aPos.y > bPos.y ? false : aPos.x < bPos.x );
};


static bool lessType( const DANGLING_END_ITEM& a, const DANGLING_END_ITEM& b )
{
    return a.GetType() < b.GetType();
};


std::vector<DANGLING_END_ITEM>::iterator
DANGLING_END_ITEM_HELPER::get_lower_pos( std::vector<DANGLING_END_ITEM>& aItemListByPos,
                                         const VECTOR2I&                 aPos )
{
    DANGLING_END_ITEM needle = DANGLING_END_ITEM( PIN_END, nullptr, aPos );
    auto              start = aItemListByPos.begin();
    auto              end = aItemListByPos.end();
    return std::lower_bound( start, end, needle, lessYX );
}


std::vector<DANGLING_END_ITEM>::iterator
DANGLING_END_ITEM_HELPER::get_lower_type( std::vector<DANGLING_END_ITEM>& aItemListByType,
                                          const DANGLING_END_T&           aType )
{
    DANGLING_END_ITEM needle = DANGLING_END_ITEM( aType, nullptr, VECTOR2I{} );
    auto              start = aItemListByType.begin();
    auto              end = aItemListByType.end();
    return std::lower_bound( start, end, needle, lessType );
}


void DANGLING_END_ITEM_HELPER::sort_dangling_end_items(
        std::vector<DANGLING_END_ITEM>& aItemListByType,
        std::vector<DANGLING_END_ITEM>& aItemListByPos )
{
    // WIRE_END pairs must be kept together. Hence stable sort.
    std::stable_sort( aItemListByType.begin(), aItemListByType.end(), lessType );
    // Sort by y first, pins are more likely to share x than y.
    std::sort( aItemListByPos.begin(), aItemListByPos.end(), lessYX );
}
