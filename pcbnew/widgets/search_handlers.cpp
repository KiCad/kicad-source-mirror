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

#include <footprint.h>
#include <pcb_edit_frame.h>
#include <pcb_marker.h>
#include <pcb_painter.h>
#include <pcb_textbox.h>
#include <pcb_text.h>
#include <string_utils.h>
#include <tool/tool_manager.h>
#include <tools/pcb_actions.h>
#include <zone.h>
#include "search_handlers.h"


PCB_SEARCH_HANDLER::PCB_SEARCH_HANDLER( wxString aName, PCB_EDIT_FRAME* aFrame ) :
        SEARCH_HANDLER( aName ), m_frame( aFrame )
{
}


void PCB_SEARCH_HANDLER::ActivateItem( long aItemRow )
{
    std::vector<long> item = { aItemRow };
    SelectItems( item );

    m_frame->GetToolManager()->RunAction( PCB_ACTIONS::properties, true );
}


FOOTPRINT_SEARCH_HANDLER::FOOTPRINT_SEARCH_HANDLER( PCB_EDIT_FRAME* aFrame ) :
        PCB_SEARCH_HANDLER( wxT( "Footprints" ), aFrame )
{
    m_columns.emplace_back( wxT( "Reference" ), 1 );
    m_columns.emplace_back( wxT( "Value" ), 2 );
    m_columns.emplace_back( wxT( "Layer" ), 1 );
    m_columns.emplace_back( wxT( "X" ), 1 );
    m_columns.emplace_back( wxT( "Y" ), 1 );
}


int FOOTPRINT_SEARCH_HANDLER::Search( const wxString& aQuery )
{
    m_hitlist.clear();
    BOARD* board = m_frame->GetBoard();

    EDA_SEARCH_DATA frp;
    frp.findString = aQuery;
    frp.matchMode = EDA_SEARCH_MATCH_MODE::WILDCARD;

    for( FOOTPRINT* fp : board->Footprints() )
    {
        if( aQuery.IsEmpty()
            || fp->Reference().Matches( frp, nullptr )
            || fp->Value().Matches( frp, nullptr ) )
        {
            m_hitlist.push_back( fp );
        }
    }

    return (int) m_hitlist.size();
}


wxString FOOTPRINT_SEARCH_HANDLER::GetResultCell( int aRow, int aCol )
{
    FOOTPRINT* fp = m_hitlist[aRow];

    if( aCol == 0 )
        return fp->GetReference();
    else if( aCol == 1 )
        return UnescapeString( fp->GetValue() );
    else if( aCol == 2 )
        return fp->GetLayerName();
    else if( aCol == 3 )
        return m_frame->MessageTextFromValue( fp->GetX() );
    else if( aCol == 4 )
        return m_frame->MessageTextFromValue( fp->GetY() );

    return wxEmptyString;
}


void FOOTPRINT_SEARCH_HANDLER::SelectItems( std::vector<long>& aItemRows )
{
    std::vector<EDA_ITEM*> selectedItems;

    for( long row : aItemRows )
    {
        if( row >= 0 && row < (long) m_hitlist.size() )
        {
            FOOTPRINT* fp = m_hitlist[row];
            selectedItems.push_back( fp );
        }
    }

    m_frame->GetToolManager()->RunAction( PCB_ACTIONS::selectionClear, true );

    if( selectedItems.size() )
        m_frame->GetToolManager()->RunAction( PCB_ACTIONS::selectItems, true, &selectedItems );

    m_frame->GetCanvas()->Refresh( false );
}


ZONE_SEARCH_HANDLER::ZONE_SEARCH_HANDLER( PCB_EDIT_FRAME* aFrame ) :
        PCB_SEARCH_HANDLER( wxT( "Zones" ), aFrame )
{
    m_columns.emplace_back( wxT( "Name" ), 2 );
    m_columns.emplace_back( wxT( "Net" ), 1 );
    m_columns.emplace_back( wxT( "Layer" ), 1 );
    m_columns.emplace_back( wxT( "Priority" ), 1 );
    m_columns.emplace_back( wxT( "X" ), 1 );
    m_columns.emplace_back( wxT( "Y" ), 1 );
}


int ZONE_SEARCH_HANDLER::Search( const wxString& aQuery )
{
    m_hitlist.clear();
    BOARD* board = m_frame->GetBoard();

    EDA_SEARCH_DATA frp;
    frp.findString = aQuery;
    frp.matchMode = EDA_SEARCH_MATCH_MODE::WILDCARD;

    for( BOARD_ITEM* item : board->Zones() )
    {
        ZONE* zoneItem = dynamic_cast<ZONE*>( item );

        if( zoneItem && ( aQuery.IsEmpty() || zoneItem->Matches( frp, nullptr ) ) )
            m_hitlist.push_back( zoneItem );
    }

    return (int) m_hitlist.size();
}


wxString ZONE_SEARCH_HANDLER::GetResultCell( int aRow, int aCol )
{
    ZONE* zone = m_hitlist[aRow];

    if( aCol == 0 )
        return zone->GetZoneName();
    if( aCol == 1 )
        return UnescapeString( zone->GetNetname() );
    else if( aCol == 2 )
    {
        wxArrayString layers;
        BOARD*        board = m_frame->GetBoard();

        for( PCB_LAYER_ID layer : zone->GetLayerSet().Seq() )
            layers.Add( board->GetLayerName( layer ) );

        return wxJoin( layers, ',' );
    }
    else if( aCol == 3 )
        return wxString::Format( "%d", zone->GetAssignedPriority() );
    else if( aCol == 4 )
        return m_frame->MessageTextFromValue( zone->GetX() );
    else if( aCol == 5 )
        return m_frame->MessageTextFromValue( zone->GetY() );

    return wxEmptyString;
}


void ZONE_SEARCH_HANDLER::SelectItems( std::vector<long>& aItemRows )
{
    std::vector<EDA_ITEM*> selectedItems;

    for( long row : aItemRows )
    {
        if( row >= 0 && row < (long) m_hitlist.size() )
        {
            ZONE* zone = m_hitlist[row];
            selectedItems.push_back( zone );
        }
    }

    m_frame->GetToolManager()->RunAction( PCB_ACTIONS::selectionClear, true );

    if( selectedItems.size() )
        m_frame->GetToolManager()->RunAction( PCB_ACTIONS::selectItems, true, &selectedItems );

    m_frame->GetCanvas()->Refresh( false );
}


TEXT_SEARCH_HANDLER::TEXT_SEARCH_HANDLER( PCB_EDIT_FRAME* aFrame ) :
        PCB_SEARCH_HANDLER( wxT( "Text" ), aFrame )
{
    m_columns.emplace_back( wxT( "Type" ), 1 );
    m_columns.emplace_back( wxT( "Text" ), 3 );
    m_columns.emplace_back( wxT( "Layer" ), 1 );
    m_columns.emplace_back( wxT( "X" ), 1 );
    m_columns.emplace_back( wxT( "Y" ), 1 );
}


int TEXT_SEARCH_HANDLER::Search( const wxString& aQuery )
{
    m_hitlist.clear();
    BOARD* board = m_frame->GetBoard();

    EDA_SEARCH_DATA frp;
    frp.findString = aQuery;
    frp.matchMode = EDA_SEARCH_MATCH_MODE::WILDCARD;

    for( BOARD_ITEM* item : board->Drawings() )
    {
        PCB_TEXT* textItem = dynamic_cast<PCB_TEXT*>( item );
        PCB_TEXTBOX* textBoxItem = dynamic_cast<PCB_TEXTBOX*>( item );

        if( textItem && ( aQuery.IsEmpty() || textItem->Matches( frp, nullptr ) ) )
            m_hitlist.push_back( textItem );
        else if( textBoxItem && ( aQuery.IsEmpty() || textBoxItem->Matches( frp, nullptr ) ) )
            m_hitlist.push_back( textBoxItem );
    }

    return (int) m_hitlist.size();
}


wxString TEXT_SEARCH_HANDLER::GetResultCell( int aRow, int aCol )
{
    BOARD_ITEM* text = m_hitlist[aRow];

    if( aCol == 0 )
    {
        if( PCB_TEXT::ClassOf( text ) )
            return _( "Text" );
        else if( PCB_TEXTBOX::ClassOf( text ) )
            return _( "Textbox" );
    }
    else if( aCol == 1 )
    {
        if( PCB_TEXT::ClassOf( text ) )
            return UnescapeString( static_cast<PCB_TEXT*>( text )->GetText() );
        else if( PCB_TEXTBOX::ClassOf( text ) )
            return UnescapeString( static_cast<PCB_TEXTBOX*>( text )->GetText() );
    }
    if( aCol == 2 )
        return text->GetLayerName();
    else if( aCol == 3 )
        return m_frame->MessageTextFromValue( text->GetX() );
    else if( aCol == 4 )
        return m_frame->MessageTextFromValue( text->GetY() );

    return wxEmptyString;
}


void TEXT_SEARCH_HANDLER::SelectItems( std::vector<long>& aItemRows )
{
    std::vector<EDA_ITEM*> selectedItems;

    for( long row : aItemRows )
    {
        if( row >= 0 && row < (long) m_hitlist.size() )
        {
            BOARD_ITEM* text = m_hitlist[row];
            selectedItems.push_back( text );
        }
    }

    m_frame->GetToolManager()->RunAction( PCB_ACTIONS::selectionClear, true );

    if( selectedItems.size() )
        m_frame->GetToolManager()->RunAction( PCB_ACTIONS::selectItems, true, &selectedItems );

    m_frame->GetCanvas()->Refresh( false );
}


NETS_SEARCH_HANDLER::NETS_SEARCH_HANDLER( PCB_EDIT_FRAME* aFrame ) :
        PCB_SEARCH_HANDLER( wxT( "Nets" ), aFrame )
{
    m_columns.emplace_back( wxT( "Name" ), 2 );
    m_columns.emplace_back( wxT( "Class" ), 2 );
}


int NETS_SEARCH_HANDLER::Search( const wxString& aQuery )
{
    m_hitlist.clear();

    EDA_SEARCH_DATA frp;
    frp.findString = aQuery;
    frp.matchMode = EDA_SEARCH_MATCH_MODE::WILDCARD;

    BOARD* board = m_frame->GetBoard();

    for( NETINFO_ITEM* net : board->GetNetInfo() )
    {
        if( net && ( aQuery.IsEmpty() || net->Matches( frp, nullptr ) ) )
            m_hitlist.push_back( net );
    }

    return (int) m_hitlist.size();
}


wxString NETS_SEARCH_HANDLER::GetResultCell( int aRow, int aCol )
{
    NETINFO_ITEM* net = m_hitlist[aRow];

    if( net->GetNetCode() == 0 )
    {
        if( aCol == 0 )
            return _( "No Net" );
        else if( aCol == 1 )
            return wxT( "" );
    }

    if( aCol == 0 )
        return UnescapeString( net->GetNetname() );
    else if( aCol == 1 )
        return net->GetNetClass()->GetName();

    return wxEmptyString;
}


void NETS_SEARCH_HANDLER::SelectItems( std::vector<long>& aItemRows )
{
    RENDER_SETTINGS* ps = m_frame->GetCanvas()->GetView()->GetPainter()->GetSettings();
    ps->SetHighlight( false );

    std::vector<NETINFO_ITEM*> selectedItems;

    for( long row : aItemRows )
    {
        if( row >= 0 && row < (long) m_hitlist.size() )
        {
            NETINFO_ITEM* net = m_hitlist[row];

            ps->SetHighlight( true, net->GetNetCode(), true );
        }
    }

    m_frame->GetCanvas()->GetView()->UpdateAllLayersColor();
    m_frame->GetCanvas()->Refresh();
}


void NETS_SEARCH_HANDLER::ActivateItem( long aItemRow )
{
    m_frame->ShowBoardSetupDialog( _( "Net Classes" ) );
}
