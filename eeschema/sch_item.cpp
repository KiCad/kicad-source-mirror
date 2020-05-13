/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2006 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
 * Copyright (C) 1992-2020 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <fctsys.h>
#include <common.h>
#include <gr_basic.h>
#include <base_struct.h>
#include <trace_helpers.h>
#include <sch_item.h>
#include <sch_screen.h>
#include <sch_sheet_path.h>
#include <sch_draw_panel.h>
#include <sch_edit_frame.h>
#include <sch_component.h>
#include <sch_sheet.h>
#include <sch_pin.h>
#include <schematic.h>
#include <general.h>


/* Constructor and destructor for SCH_ITEM */
/* They are not inline because this creates problems with gcc at linking time
 * in debug mode
 */

SCH_ITEM::SCH_ITEM( EDA_ITEM* aParent, KICAD_T aType ) :
    EDA_ITEM( aParent, aType )
{
    m_Layer              = LAYER_WIRE; // It's only a default, in fact
    m_fieldsAutoplaced   = FIELDS_AUTOPLACED_NO;
    m_connectivity_dirty = true;
}


SCH_ITEM::SCH_ITEM( const SCH_ITEM& aItem ) :
    EDA_ITEM( aItem )
{
    m_Layer              = aItem.m_Layer;
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

    newItem->ClearFlags( SELECTED | HIGHLIGHTED | BRIGHTENED );

    if( newItem->Type() == SCH_COMPONENT_T )
    {
        SCH_COMPONENT* component = (SCH_COMPONENT*) newItem;

        for( SCH_PIN* pin : component->GetSchPins() )
            pin->ClearFlags( SELECTED | HIGHLIGHTED | BRIGHTENED );

        for( SCH_FIELD& field : component->GetFields() )
            field.ClearFlags( SELECTED | HIGHLIGHTED | BRIGHTENED );
    }

    if( newItem->Type() == SCH_SHEET_T )
    {
        SCH_SHEET* sheet = (SCH_SHEET*) newItem;

        for( SCH_FIELD& field : sheet->GetFields() )
            field.ClearFlags( SELECTED | HIGHLIGHTED | BRIGHTENED );

        for( SCH_SHEET_PIN* pin : sheet->GetPins() )
            pin->ClearFlags( SELECTED | HIGHLIGHTED | BRIGHTENED );
    }

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
    if( ( m_Flags & STRUCT_DELETED ) || ( m_Flags & SKIP_STRUCT ) )
        return false;

    return doIsConnected( aPosition );
}


SCH_CONNECTION* SCH_ITEM::Connection( const SCH_SHEET_PATH& aSheet ) const
{
    // Warning: the m_connection_map can be empty.
    if( m_connection_map.size() && m_connection_map.count( aSheet ) )
        return m_connection_map.at( aSheet );

    return nullptr;
}


ITEM_SET& SCH_ITEM::ConnectedItems( const SCH_SHEET_PATH& aSheet )
{
    return m_connected_items[ aSheet ];
}


void SCH_ITEM::AddConnectionTo( const SCH_SHEET_PATH& aSheet, SCH_ITEM* aItem )
{
    m_connected_items[ aSheet ].insert( aItem );
}


SCH_CONNECTION*  SCH_ITEM::InitializeConnection( const SCH_SHEET_PATH& aSheet )
{
    if( Connection( aSheet ) )
    {
        Connection( aSheet )->Reset();
        Connection( aSheet )->SetSheet( aSheet );
        return Connection( aSheet );
    }

    auto connection = new SCH_CONNECTION( this );
    connection->SetSheet( aSheet );
    m_connection_map.insert( std::make_pair( aSheet, connection ) );
    return connection;
}


void SCH_ITEM::SwapData( SCH_ITEM* aItem )
{
    wxFAIL_MSG( wxT( "SwapData() method not implemented for class " ) + GetClass() );
}


bool SCH_ITEM::operator < ( const SCH_ITEM& aItem ) const
{
    if( Type() != aItem.Type() )
        return Type() < aItem.Type();

    if( m_Uuid != aItem.m_Uuid )
        return m_Uuid < aItem.m_Uuid;

    if( GetPosition().x != aItem.GetPosition().x )
        return GetPosition().x < aItem.GetPosition().x;

    return GetPosition().y < aItem.GetPosition().y;
}


void SCH_ITEM::Plot( PLOTTER* aPlotter )
{
    wxFAIL_MSG( wxT( "Plot() method not implemented for class " ) + GetClass() );
}
