/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2022 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <base_units.h>
#include <footprint.h>
#include <tool/tool_manager.h>
#include <tools/pcb_actions.h>
#include <pcb_edit_frame.h>
#include <pcb_marker.h>
#include <pcb_text.h>
#include <zone.h>
#include "search_handlers.h"


FOOTPRINT_SEARCH_HANDLER::FOOTPRINT_SEARCH_HANDLER( PCB_EDIT_FRAME* aFrame ) :
        SEARCH_HANDLER( wxT( "Footprint" ) ), m_frame( aFrame )
{
    m_columnNames.emplace_back( wxT( "Reference" ) );
    m_columnNames.emplace_back( wxT( "Value" ) );
    m_columnNames.emplace_back( wxT( "Layer" ) );
    m_columnNames.emplace_back( wxT( "X" ) );
    m_columnNames.emplace_back( wxT( "Y" ) );
}


int FOOTPRINT_SEARCH_HANDLER::Search( const wxString& query )
{
    m_hitlist.clear();
    BOARD* board = m_frame->GetBoard();

    EDA_SEARCH_DATA frp;
    frp.findString = query;
    frp.matchMode = EDA_SEARCH_MATCH_MODE::WILDCARD;

    for( FOOTPRINT* fp : board->Footprints() )
    {
        if( fp->Reference().Matches( frp, nullptr ) || fp->Value().Matches( frp, nullptr ) )
        {
            m_hitlist.push_back( fp );
        }
    }

    return m_hitlist.size();
}


wxString FOOTPRINT_SEARCH_HANDLER::GetResultCell( int row, int col )
{
    FOOTPRINT* fp = m_hitlist[row];

    if( col == 0 )
        return fp->GetReference();
    else if( col == 1 )
        return fp->GetValue();
    else if( col == 2 )
        return fp->GetLayerName();
    else if( col == 3 )
        return m_frame->MessageTextFromValue( fp->GetX() );
    else if( col == 4 )
        return m_frame->MessageTextFromValue( fp->GetY() );

    return wxEmptyString;
}


void FOOTPRINT_SEARCH_HANDLER::SelectItem( int row )
{
    FOOTPRINT* fp = m_hitlist[row];

    m_frame->GetToolManager()->RunAction( PCB_ACTIONS::selectionClear, true );
    m_frame->GetToolManager()->RunAction( PCB_ACTIONS::selectItem, true, fp );
}


ZONE_SEARCH_HANDLER::ZONE_SEARCH_HANDLER( PCB_EDIT_FRAME* aFrame ) :
        SEARCH_HANDLER( wxT( "Zones" ) ), m_frame( aFrame )
{
    m_columnNames.emplace_back( wxT( "Name" ) );
    m_columnNames.emplace_back( wxT( "Layer" ) );
    m_columnNames.emplace_back( wxT( "X" ) );
    m_columnNames.emplace_back( wxT( "Y" ) );
}


int ZONE_SEARCH_HANDLER::Search( const wxString& query )
{
    m_hitlist.clear();
    BOARD* board = m_frame->GetBoard();

    EDA_SEARCH_DATA frp;
    frp.findString = query;
    frp.matchMode = EDA_SEARCH_MATCH_MODE::WILDCARD;

    for( BOARD_ITEM* item : board->Zones() )
    {
        ZONE* zoneItem = dynamic_cast<ZONE*>( item );

        if( zoneItem && zoneItem->Matches( frp, nullptr ) )
        {
            m_hitlist.push_back( zoneItem );
        }
    }

    return m_hitlist.size();
}


wxString ZONE_SEARCH_HANDLER::GetResultCell( int row, int col )
{
    ZONE* zone = m_hitlist[row];

    if( col == 0 )
        return zone->GetNetname();
    else if( col == 1 )
        return zone->GetLayerName();
    else if( col == 2 )
        return m_frame->MessageTextFromValue( zone->GetX() );
    else if( col == 3 )
        return m_frame->MessageTextFromValue( zone->GetY() );

    return wxEmptyString;
}


void ZONE_SEARCH_HANDLER::SelectItem( int row )
{
    ZONE* zone = m_hitlist[row];

    m_frame->GetToolManager()->RunAction( PCB_ACTIONS::selectionClear, true );
    m_frame->GetToolManager()->RunAction( PCB_ACTIONS::selectItem, true, zone );
}


TEXT_SEARCH_HANDLER::TEXT_SEARCH_HANDLER( PCB_EDIT_FRAME* aFrame ) :
        SEARCH_HANDLER( wxT( "Text" ) ), m_frame( aFrame )
{
    m_columnNames.emplace_back( wxT( "Text" ) );
    m_columnNames.emplace_back( wxT( "Layer" ) );
    m_columnNames.emplace_back( wxT( "X" ) );
    m_columnNames.emplace_back( wxT( "Y" ) );
}


int TEXT_SEARCH_HANDLER::Search( const wxString& query )
{
    m_hitlist.clear();
    BOARD* board = m_frame->GetBoard();

    EDA_SEARCH_DATA frp;
    frp.findString = query;
    frp.matchMode = EDA_SEARCH_MATCH_MODE::WILDCARD;

    for( BOARD_ITEM* item : board->Drawings() )
    {
        PCB_TEXT* textItem = dynamic_cast<PCB_TEXT*>( item );

        if( textItem && textItem->Matches( frp, nullptr ) )
        {
            m_hitlist.push_back( textItem );
        }
    }

    return m_hitlist.size();
}


wxString TEXT_SEARCH_HANDLER::GetResultCell( int row, int col )
{
    PCB_TEXT* text = m_hitlist[row];

    if( col == 0 )
        return text->GetText();
    if( col == 1 )
        return text->GetLayerName();
    else if( col == 2 )
        return m_frame->MessageTextFromValue( text->GetX() );
    else if( col == 3 )
        return m_frame->MessageTextFromValue( text->GetY() );

    return wxEmptyString;
}


void TEXT_SEARCH_HANDLER::SelectItem( int row )
{
    PCB_TEXT* text = m_hitlist[row];

    m_frame->GetToolManager()->RunAction( PCB_ACTIONS::selectionClear, true );
    m_frame->GetToolManager()->RunAction( PCB_ACTIONS::selectItem, true, text );
}


NETS_SEARCH_HANDLER::NETS_SEARCH_HANDLER( PCB_EDIT_FRAME* aFrame ) :
        SEARCH_HANDLER( wxT( "Nets" ) ), m_frame( aFrame )
{
    m_columnNames.emplace_back( wxT( "Name" ) );
    m_columnNames.emplace_back( wxT( "Class" ) );
}


int NETS_SEARCH_HANDLER::Search( const wxString& query )
{
    m_hitlist.clear();

    EDA_SEARCH_DATA frp;
    frp.findString = query;
    frp.matchMode = EDA_SEARCH_MATCH_MODE::WILDCARD;

    BOARD* board = m_frame->GetBoard();
    for( NETINFO_ITEM* net : board->GetNetInfo() )
    {
        if( net && net->Matches( frp, nullptr ) )
        {
            m_hitlist.push_back( net );
        }
    }

    return m_hitlist.size();
}


wxString NETS_SEARCH_HANDLER::GetResultCell( int row, int col )
{
    NETINFO_ITEM* net = m_hitlist[row];

    if( col == 0 )
        return net->GetNetname();
    else if( col == 1 )
        return net->GetNetClass()->GetName();

    return wxEmptyString;
}