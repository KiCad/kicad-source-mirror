/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 CERN
 * Copyright (C) 2022-2023 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <pcb_dimension.h>
#include <ratsnest/ratsnest_data.h>
#include <string_utils.h>
#include <tool/tool_manager.h>
#include <tools/pcb_actions.h>
#include <zone.h>
#include "search_handlers.h"


void PCB_SEARCH_HANDLER::ActivateItem( long aItemRow )
{
    std::vector<long> item = { aItemRow };
    SelectItems( item );

    m_frame->GetToolManager()->RunAction( PCB_ACTIONS::properties );
}


void PCB_SEARCH_HANDLER::Sort( int aCol, bool aAscending, std::vector<long>* aSelection )
{
    std::vector<BOARD_ITEM*> selection;

    for( long i = 0; i < (long) m_hitlist.size(); ++i )
    {
        if( alg::contains( *aSelection, i ) )
            selection.push_back( m_hitlist[i] );
    }

    int col = std::max( 0, aCol );  // Provide a stable order by sorting on first column if no
                                    // sort column provided.

    std::sort( m_hitlist.begin(), m_hitlist.end(),
            [&]( BOARD_ITEM* a, BOARD_ITEM* b ) -> bool
            {
                // N.B. To meet the iterator sort conditions, we cannot simply invert the truth
                // to get the opposite sort.  i.e. ~(a<b) != (a>b)
                if( aAscending )
                    return StrNumCmp( getResultCell( a, col ), getResultCell( b, col ), true ) < 0;
                else
                    return StrNumCmp( getResultCell( b, col ), getResultCell( a, col ), true ) < 0;
            } );


    aSelection->clear();

    for( long i = 0; i < (long) m_hitlist.size(); ++i )
    {
        if( alg::contains( selection, m_hitlist[i] ) )
            aSelection->push_back( i );
    }
}


void PCB_SEARCH_HANDLER::SelectItems( std::vector<long>& aItemRows )
{
    std::vector<EDA_ITEM*> selectedItems;

    for( long row : aItemRows )
    {
        if( row >= 0 && row < (long) m_hitlist.size() )
            selectedItems.push_back( m_hitlist[row] );
    }

    m_frame->GetToolManager()->RunAction( PCB_ACTIONS::selectionClear );

    if( selectedItems.size() )
        m_frame->GetToolManager()->RunAction( PCB_ACTIONS::selectItems, &selectedItems );

    m_frame->GetCanvas()->Refresh( false );
}


FOOTPRINT_SEARCH_HANDLER::FOOTPRINT_SEARCH_HANDLER( PCB_EDIT_FRAME* aFrame ) :
        PCB_SEARCH_HANDLER( _HKI( "Footprints" ), aFrame )
{
    m_columns.emplace_back( _HKI( "Reference" ), 2, wxLIST_FORMAT_LEFT );
    m_columns.emplace_back( _HKI( "Value" ),     8, wxLIST_FORMAT_LEFT );
    m_columns.emplace_back( _HKI( "Layer" ),     3, wxLIST_FORMAT_CENTER );
    m_columns.emplace_back( wxT( "X" ),          3, wxLIST_FORMAT_CENTER );
    m_columns.emplace_back( wxT( "Y" ),          3, wxLIST_FORMAT_CENTER );
}


int FOOTPRINT_SEARCH_HANDLER::Search( const wxString& aQuery )
{
    m_hitlist.clear();
    BOARD* board = m_frame->GetBoard();

    if( board == nullptr )
        return 0;

    EDA_SEARCH_DATA frp;
    frp.findString = aQuery;

    // Try to handle whatever the user throws at us (substring, wildcards, regex, etc.)
    frp.matchMode = EDA_SEARCH_MATCH_MODE::PERMISSIVE;

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


wxString FOOTPRINT_SEARCH_HANDLER::getResultCell( BOARD_ITEM* aItem, int aCol )
{
    FOOTPRINT* fp = static_cast<FOOTPRINT*>( aItem );

    if( aCol == 0 )
        return fp->GetReference();
    else if( aCol == 1 )
        return UnescapeString( fp->GetValue() );
    else if( aCol == 2 )
        return fp->GetLayerName();
    else if( aCol == 3 )
        return m_frame->MessageTextFromCoord( fp->GetX(), ORIGIN_TRANSFORMS::ABS_X_COORD );
    else if( aCol == 4 )
        return m_frame->MessageTextFromCoord( fp->GetY(), ORIGIN_TRANSFORMS::ABS_Y_COORD );

    return wxEmptyString;
}


ZONE_SEARCH_HANDLER::ZONE_SEARCH_HANDLER( PCB_EDIT_FRAME* aFrame ) :
        PCB_SEARCH_HANDLER( _HKI( "Zones" ), aFrame )
{
    m_columns.emplace_back( _HKI( "Name" ),     6, wxLIST_FORMAT_LEFT );
    m_columns.emplace_back( _HKI( "Net" ),      6, wxLIST_FORMAT_LEFT);
    m_columns.emplace_back( _HKI( "Layer" ),    3, wxLIST_FORMAT_CENTER );
    m_columns.emplace_back( _HKI( "Priority" ), 2, wxLIST_FORMAT_CENTER );
    m_columns.emplace_back( wxT( "X" ),         3, wxLIST_FORMAT_CENTER );
    m_columns.emplace_back( wxT( "Y" ),         3, wxLIST_FORMAT_CENTER );
}


int ZONE_SEARCH_HANDLER::Search( const wxString& aQuery )
{
    m_hitlist.clear();
    BOARD* board = m_frame->GetBoard();

    EDA_SEARCH_DATA frp;
    frp.findString = aQuery;

    // Try to handle whatever the user throws at us (substring, wildcards, regex, etc.)
    frp.matchMode = EDA_SEARCH_MATCH_MODE::PERMISSIVE;

    for( BOARD_ITEM* item : board->Zones() )
    {
        ZONE* zoneItem = dynamic_cast<ZONE*>( item );

        if( zoneItem && ( aQuery.IsEmpty() || zoneItem->Matches( frp, nullptr ) ) )
            m_hitlist.push_back( zoneItem );
    }

    return (int) m_hitlist.size();
}


wxString ZONE_SEARCH_HANDLER::getResultCell( BOARD_ITEM* aItem, int aCol )
{
    ZONE* zone = static_cast<ZONE*>( aItem );

    if( aCol == 0 )
        return zone->GetZoneName();
    else if( aCol == 1 )
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
        return m_frame->MessageTextFromCoord( zone->GetX(), ORIGIN_TRANSFORMS::ABS_X_COORD );
    else if( aCol == 5 )
        return m_frame->MessageTextFromCoord( zone->GetY(), ORIGIN_TRANSFORMS::ABS_Y_COORD );


    return wxEmptyString;
}


TEXT_SEARCH_HANDLER::TEXT_SEARCH_HANDLER( PCB_EDIT_FRAME* aFrame ) :
        PCB_SEARCH_HANDLER( _HKI( "Text" ), aFrame )
{
    m_columns.emplace_back( _HKI( "Type" ),  2,  wxLIST_FORMAT_LEFT );
    m_columns.emplace_back( _HKI( "Text" ),  12, wxLIST_FORMAT_LEFT );
    m_columns.emplace_back( _HKI( "Layer" ), 3,  wxLIST_FORMAT_CENTER );
    m_columns.emplace_back( wxT( "X" ),      3,  wxLIST_FORMAT_CENTER );
    m_columns.emplace_back( wxT( "Y" ),      3,  wxLIST_FORMAT_CENTER );
}


int TEXT_SEARCH_HANDLER::Search( const wxString& aQuery )
{
    m_hitlist.clear();
    BOARD* board = m_frame->GetBoard();

    EDA_SEARCH_DATA frp;
    frp.findString = aQuery;

    // Try to handle whatever the user throws at us (substring, wildcards, regex, etc.)
    frp.matchMode = EDA_SEARCH_MATCH_MODE::PERMISSIVE;

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


wxString TEXT_SEARCH_HANDLER::getResultCell( BOARD_ITEM* aItem, int aCol )
{
    if( aCol == 0 )
    {
        if( PCB_TEXT::ClassOf( aItem ) )
            return _( "Text" );
        else if( PCB_TEXTBOX::ClassOf( aItem ) )
            return _( "Textbox" );
        else if( dynamic_cast<PCB_DIMENSION_BASE*>( aItem ) )
            return _( "Dimension" );
    }
    else if( aCol == 1 )
    {
        if( PCB_TEXT::ClassOf( aItem ) )
            return UnescapeString( static_cast<PCB_TEXT*>( aItem )->GetText() );
        else if( PCB_TEXTBOX::ClassOf( aItem ) )
            return UnescapeString( static_cast<PCB_TEXTBOX*>( aItem )->GetText() );
        else if( PCB_DIMENSION_BASE* dimension = dynamic_cast<PCB_DIMENSION_BASE*>( aItem ) )
            return UnescapeString( dimension->GetText() );
    }
    else if( aCol == 2 )
        return aItem->GetLayerName();
    else if( aCol == 3 )
        return m_frame->MessageTextFromCoord( aItem->GetX(), ORIGIN_TRANSFORMS::ABS_X_COORD );
    else if( aCol == 4 )
        return m_frame->MessageTextFromCoord( aItem->GetY(), ORIGIN_TRANSFORMS::ABS_Y_COORD );

    return wxEmptyString;
}


NETS_SEARCH_HANDLER::NETS_SEARCH_HANDLER( PCB_EDIT_FRAME* aFrame ) :
        PCB_SEARCH_HANDLER( _HKI( "Nets" ), aFrame )
{
    m_columns.emplace_back( _HKI( "Name" ),  6, wxLIST_FORMAT_LEFT );
    m_columns.emplace_back( _HKI( "Class" ), 6, wxLIST_FORMAT_LEFT );
}


int NETS_SEARCH_HANDLER::Search( const wxString& aQuery )
{
    m_hitlist.clear();

    EDA_SEARCH_DATA frp;
    frp.findString = aQuery;

    // Try to handle whatever the user throws at us (substring, wildcards, regex, etc.)
    frp.matchMode = EDA_SEARCH_MATCH_MODE::PERMISSIVE;

    BOARD* board = m_frame->GetBoard();

    for( NETINFO_ITEM* net : board->GetNetInfo() )
    {
        if( net && ( aQuery.IsEmpty() || net->Matches( frp, nullptr ) ) )
            m_hitlist.push_back( net );
    }

    return (int) m_hitlist.size();
}


wxString NETS_SEARCH_HANDLER::getResultCell( BOARD_ITEM* aItem, int aCol )
{
    NETINFO_ITEM* net = static_cast<NETINFO_ITEM*>( aItem );

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
            NETINFO_ITEM* net = static_cast<NETINFO_ITEM*>( m_hitlist[row] );

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


RATSNEST_SEARCH_HANDLER::RATSNEST_SEARCH_HANDLER( PCB_EDIT_FRAME* aFrame ) :
        PCB_SEARCH_HANDLER( _HKI( "Ratsnest" ), aFrame )
{
    m_columns.emplace_back( _HKI( "Name" ),  6, wxLIST_FORMAT_LEFT );
    m_columns.emplace_back( _HKI( "Class" ), 6, wxLIST_FORMAT_LEFT );
}


int RATSNEST_SEARCH_HANDLER::Search( const wxString& aQuery )
{
    m_hitlist.clear();

    EDA_SEARCH_DATA frp;
    frp.findString = aQuery;

    // Try to handle whatever the user throws at us (substring, wildcards, regex, etc.)
    frp.matchMode = EDA_SEARCH_MATCH_MODE::PERMISSIVE;

    BOARD* board = m_frame->GetBoard();

    for( NETINFO_ITEM* net : board->GetNetInfo() )
    {
        if( net == nullptr || !net->Matches( frp, nullptr ) )
            continue;

        RN_NET* rn = board->GetConnectivity()->GetRatsnestForNet( net->GetNetCode() );

        if( rn && !rn->GetEdges().empty() )
            m_hitlist.push_back( net );
    }

    return (int) m_hitlist.size();
}


wxString RATSNEST_SEARCH_HANDLER::getResultCell( BOARD_ITEM* aItem, int aCol )
{
    NETINFO_ITEM* net = static_cast<NETINFO_ITEM*>( aItem );

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


void RATSNEST_SEARCH_HANDLER::SelectItems( std::vector<long>& aItemRows )
{
    RENDER_SETTINGS* ps = m_frame->GetCanvas()->GetView()->GetPainter()->GetSettings();
    ps->SetHighlight( false );

    std::vector<NETINFO_ITEM*> selectedItems;

    for( long row : aItemRows )
    {
        if( row >= 0 && row < (long) m_hitlist.size() )
        {
            NETINFO_ITEM* net = static_cast<NETINFO_ITEM*>( m_hitlist[row] );

            ps->SetHighlight( true, net->GetNetCode(), true );
        }
    }

    m_frame->GetCanvas()->GetView()->UpdateAllLayersColor();
    m_frame->GetCanvas()->Refresh();
}


void RATSNEST_SEARCH_HANDLER::ActivateItem( long aItemRow )
{
    m_frame->ShowBoardSetupDialog( _( "Net Classes" ) );
}
