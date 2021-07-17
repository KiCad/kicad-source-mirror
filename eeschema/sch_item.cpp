/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2006 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
 * Copyright (C) 1992-2021 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <eda_item.h>
#include <trace_helpers.h>
#include <sch_item.h>
#include <sch_screen.h>
#include <sch_sheet_path.h>
#include <sch_draw_panel.h>
#include <sch_edit_frame.h>
#include <sch_symbol.h>
#include <sch_sheet.h>
#include <sch_pin.h>
#include <schematic.h>
#include <general.h>
#include <netclass.h>
#include <project/project_file.h>
#include <project/net_settings.h>


/* Constructor and destructor for SCH_ITEM */
/* They are not inline because this creates problems with gcc at linking time
 * in debug mode
 */

SCH_ITEM::SCH_ITEM( EDA_ITEM* aParent, KICAD_T aType ) :
    EDA_ITEM( aParent, aType )
{
    m_layer              = LAYER_WIRE; // It's only a default, in fact
    m_fieldsAutoplaced   = FIELDS_AUTOPLACED_NO;
    m_connectivity_dirty = true;
}


SCH_ITEM::SCH_ITEM( const SCH_ITEM& aItem ) :
    EDA_ITEM( aItem )
{
    m_layer              = aItem.m_layer;
    m_fieldsAutoplaced   = aItem.m_fieldsAutoplaced;
    m_connectivity_dirty = true;
}


SCH_ITEM::~SCH_ITEM()
{
    // Do not let the connections container go out of scope with any objects or they
    // will be deleted by the container will cause the Eeschema to crash.  These objects
    // are owned by the sheet object container.
    if( !m_connections.empty() )
        m_connections.clear();

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


void SCH_ITEM::ViewGetLayers( int aLayers[], int& aCount ) const
{
    // Basic fallback
    aCount     = 2;
    aLayers[0] = LAYER_DEVICE;
    aLayers[1] = LAYER_SELECTION_SHADOWS;
}


bool SCH_ITEM::IsConnected( const wxPoint& aPosition ) const
{
    if(( m_flags & STRUCT_DELETED ) || ( m_flags & SKIP_STRUCT ) )
        return false;

    return doIsConnected( aPosition );
}


SCH_CONNECTION* SCH_ITEM::Connection( const SCH_SHEET_PATH* aSheet ) const
{
    if( !aSheet )
        aSheet = &Schematic()->CurrentSheet();

    auto it = m_connection_map.find( *aSheet );

    if( it == m_connection_map.end() )
        return nullptr;
    else
        return it->second;
}


NETCLASSPTR SCH_ITEM::NetClass( const SCH_SHEET_PATH* aSheet ) const
{
    if( m_connection_map.size() )
    {
        SCH_CONNECTION* connection = Connection( aSheet );

        if( connection )
        {
            NET_SETTINGS&   netSettings = Schematic()->Prj().GetProjectFile().NetSettings();
            const wxString& netclassName = netSettings.GetNetclassName( connection->Name() );

            return netSettings.m_NetClasses.Find( netclassName );
        }
    }

    return nullptr;
}


SCH_ITEM_SET& SCH_ITEM::ConnectedItems( const SCH_SHEET_PATH& aSheet )
{
    return m_connected_items[ aSheet ];
}


void SCH_ITEM::AddConnectionTo( const SCH_SHEET_PATH& aSheet, SCH_ITEM* aItem )
{
    m_connected_items[ aSheet ].insert( aItem );
}


SCH_CONNECTION* SCH_ITEM::InitializeConnection( const SCH_SHEET_PATH& aSheet,
                                                CONNECTION_GRAPH* aGraph )
{
    SCH_CONNECTION* connection = Connection( &aSheet );

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


void SCH_ITEM::SwapData( SCH_ITEM* aItem )
{
    UNIMPLEMENTED_FOR( GetClass() );
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


void SCH_ITEM::Plot( PLOTTER* aPlotter ) const
{
    wxFAIL_MSG( wxT( "Plot() method not implemented for class " ) + GetClass() );
}
