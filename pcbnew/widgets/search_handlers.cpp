/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
#include <pcb_group.h>
#include <pcb_textbox.h>
#include <pcb_text.h>
#include <pcb_dimension.h>
#include <pcbnew_settings.h>
#include <ratsnest/ratsnest_data.h>
#include <string_utils.h>
#include <tool/tool_manager.h>
#include <tools/pcb_actions.h>
#include <zone.h>
#include <pad.h>
#include <pcb_track.h>
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
    APP_SETTINGS_BASE::SEARCH_PANE& settings = m_frame->config()->m_SearchPane;
    std::vector<EDA_ITEM*> selectedItems;

    for( long row : aItemRows )
    {
        if( row >= 0 && row < (long) m_hitlist.size() )
            selectedItems.push_back( m_hitlist[row] );
    }

    m_frame->GetToolManager()->RunAction( ACTIONS::selectionClear );

    if( selectedItems.size() )
    {
        m_frame->GetToolManager()->RunAction( ACTIONS::selectItems, &selectedItems );

        switch( settings.selection_zoom )
        {
        case APP_SETTINGS_BASE::SEARCH_PANE::SELECTION_ZOOM::PAN:
            m_frame->GetToolManager()->RunAction( ACTIONS::centerSelection );
            break;
        case APP_SETTINGS_BASE::SEARCH_PANE::SELECTION_ZOOM::ZOOM:
            m_frame->GetToolManager()->RunAction( ACTIONS::zoomFitSelection );
            break;
        case APP_SETTINGS_BASE::SEARCH_PANE::SELECTION_ZOOM::NONE:
            break;
        }
    }

    m_frame->GetCanvas()->Refresh( false );
}


FOOTPRINT_SEARCH_HANDLER::FOOTPRINT_SEARCH_HANDLER( PCB_EDIT_FRAME* aFrame ) :
        PCB_SEARCH_HANDLER( _HKI( "Footprints" ), aFrame )
{
    m_columns.emplace_back( _HKI( "Reference" ),            2, wxLIST_FORMAT_LEFT );
    m_columns.emplace_back( _HKI( "Value" ),                6, wxLIST_FORMAT_LEFT );
    m_columns.emplace_back( _HKI( "Layer" ),                2, wxLIST_FORMAT_CENTER );
    m_columns.emplace_back(  wxT( "X" ),                    3, wxLIST_FORMAT_CENTER );
    m_columns.emplace_back(  wxT( "Y" ),                    3, wxLIST_FORMAT_CENTER );
    m_columns.emplace_back( _HKI( "Library Link" ),         8, wxLIST_FORMAT_LEFT );
    m_columns.emplace_back( _HKI( "Library Description" ), 10, wxLIST_FORMAT_LEFT );
}


int FOOTPRINT_SEARCH_HANDLER::Search( const wxString& aQuery )
{
    m_hitlist.clear();
    BOARD* board = m_frame->GetBoard();

    if( board == nullptr )
        return 0;

    APP_SETTINGS_BASE::SEARCH_PANE& settings = m_frame->config()->m_SearchPane;
    EDA_SEARCH_DATA                 frp;

    frp.searchAllFields = settings.search_hidden_fields;
    frp.searchMetadata = settings.search_metadata;
    frp.findString = aQuery;

    // Try to handle whatever the user throws at us (substring, wildcards, regex, etc.)
    frp.matchMode = EDA_SEARCH_MATCH_MODE::PERMISSIVE;

    for( FOOTPRINT* fp : board->Footprints() )
    {
        bool found = false;

        if( frp.findString.IsEmpty() )
            found = true;

        if( !found && fp->Matches( frp, nullptr ) )
            found = true;

        if( !found )
        {
            for( PCB_FIELD* field : fp->GetFields() )
            {
                wxCHECK2( field, continue );

                if( field->Matches( frp, nullptr ) )
                {
                    found = true;
                    break;
                }
            }
        }

        if( found )
            m_hitlist.push_back( fp );
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
    else if( aCol == 5 )
        return fp->GetFPID().Format();
    else if( aCol == 6 )
        return fp->GetLibDescription();

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
    m_columns.emplace_back( _HKI( "Area" ),     3, wxLIST_FORMAT_RIGHT );
}


int ZONE_SEARCH_HANDLER::Search( const wxString& aQuery )
{
    m_hitlist.clear();
    BOARD* board = m_frame->GetBoard();

    APP_SETTINGS_BASE::SEARCH_PANE& settings = m_frame->config()->m_SearchPane;
    EDA_SEARCH_DATA                 frp;

    frp.searchAllFields = settings.search_hidden_fields;
    frp.searchMetadata = settings.search_metadata;
    frp.findString = aQuery;

    // Try to handle whatever the user throws at us (substring, wildcards, regex, etc.)
    frp.matchMode = EDA_SEARCH_MATCH_MODE::PERMISSIVE;

    for( BOARD_ITEM* item : board->Zones() )
    {
        if( frp.findString.IsEmpty() || item->Matches( frp, nullptr ) )
            m_hitlist.push_back( item );
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
    else if( aCol == 6 )
    {
        return m_frame->MessageTextFromValue( zone->GetIsRuleArea() ? zone->GetOutlineArea() : zone->GetFilledArea(),
                                              true, EDA_DATA_TYPE::AREA );
    }


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

    APP_SETTINGS_BASE::SEARCH_PANE& settings = m_frame->config()->m_SearchPane;
    EDA_SEARCH_DATA                 frp;

    frp.searchAllFields = settings.search_hidden_fields;
    frp.searchMetadata = settings.search_metadata;
    frp.findString = aQuery;

    // Try to handle whatever the user throws at us (substring, wildcards, regex, etc.)
    frp.matchMode = EDA_SEARCH_MATCH_MODE::PERMISSIVE;

    for( BOARD_ITEM* item : board->Drawings() )
    {
        if( item->Type() == PCB_TEXT_T
            || BaseType( item->Type() ) == PCB_DIMENSION_T
            || item->Type() == PCB_TEXTBOX_T
            || item->Type() == PCB_TABLECELL_T )
        {
            if( frp.findString.IsEmpty() || item->Matches( frp, nullptr ) )
                m_hitlist.push_back( item );
        }
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


GROUP_SEARCH_HANDLER::GROUP_SEARCH_HANDLER( PCB_EDIT_FRAME* aFrame ) :
        PCB_SEARCH_HANDLER( _HKI( "Groups" ), aFrame )
{
    m_columns.emplace_back( _HKI( "Type" ),  2,  wxLIST_FORMAT_LEFT );
    m_columns.emplace_back( _HKI( "Name" ),  6,  wxLIST_FORMAT_LEFT );
    m_columns.emplace_back( wxT( "X" ),      3,  wxLIST_FORMAT_CENTER );
    m_columns.emplace_back( wxT( "Y" ),      3,  wxLIST_FORMAT_CENTER );
}


int GROUP_SEARCH_HANDLER::Search( const wxString& aQuery )
{
    m_hitlist.clear();
    BOARD* board = m_frame->GetBoard();

    APP_SETTINGS_BASE::SEARCH_PANE& settings = m_frame->config()->m_SearchPane;
    EDA_SEARCH_DATA                 frp;

    frp.searchAllFields = settings.search_hidden_fields;
    frp.searchMetadata = settings.search_metadata;
    frp.findString = aQuery;

    // Try to handle whatever the user throws at us (substring, wildcards, regex, etc.)
    frp.matchMode = EDA_SEARCH_MATCH_MODE::PERMISSIVE;

    for( BOARD_ITEM* item : board->Groups() )
    {
        // Skip generators, they are for internal use, not user-facing grouping
        if( item->Type() == PCB_GENERATOR_T )
            continue;

        if( frp.findString.IsEmpty() || item->Matches( frp, nullptr ) )
            m_hitlist.push_back( item );
    }

    return (int) m_hitlist.size();
}


wxString GROUP_SEARCH_HANDLER::getResultCell( BOARD_ITEM* aItem, int aCol )
{
    if( aCol == 0 )
    {
        if( aItem->Type() == PCB_GROUP_T )
            return _( "Group" );
        else if( aItem->Type() == PCB_GENERATOR_T )
            return _( "Generator" );
    }
    else if( aCol == 1 )
        return static_cast<PCB_GROUP*>( aItem )->GetName();
    else if( aCol == 2 )
        return m_frame->MessageTextFromCoord( aItem->GetX(), ORIGIN_TRANSFORMS::ABS_X_COORD );
    else if( aCol == 3 )
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

    APP_SETTINGS_BASE::SEARCH_PANE& settings = m_frame->config()->m_SearchPane;
    EDA_SEARCH_DATA                 frp;

    frp.searchAllFields = settings.search_hidden_fields;
    frp.searchMetadata = settings.search_metadata;
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

    APP_SETTINGS_BASE::SEARCH_PANE& settings = m_frame->config()->m_SearchPane;
    EDA_SEARCH_DATA                 frp;

    frp.searchAllFields = settings.search_hidden_fields;
    frp.searchMetadata = settings.search_metadata;
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


DRILL_SEARCH_HANDLER::DRILL_SEARCH_HANDLER( PCB_EDIT_FRAME* aFrame ) :
        PCB_SEARCH_HANDLER( _HKI( "Drills" ), aFrame ),
        m_frame( aFrame )
{
    m_columns.emplace_back( _HKI( "Count" ),       2, wxLIST_FORMAT_RIGHT );
    m_columns.emplace_back( _HKI( "Shape" ),       3, wxLIST_FORMAT_LEFT );
    m_columns.emplace_back( _HKI( "X Size" ),      3, wxLIST_FORMAT_CENTER );
    m_columns.emplace_back( _HKI( "Y Size" ),      3, wxLIST_FORMAT_CENTER );
    m_columns.emplace_back( _HKI( "Plated" ),      2, wxLIST_FORMAT_CENTER );
    m_columns.emplace_back( _HKI( "Via/Pad" ),     2, wxLIST_FORMAT_CENTER );
    m_columns.emplace_back( _HKI( "Start Layer" ), 4, wxLIST_FORMAT_CENTER );
    m_columns.emplace_back( _HKI( "Stop Layer" ),  4, wxLIST_FORMAT_CENTER );
}


int DRILL_SEARCH_HANDLER::Search( const wxString& aQuery )
{
    BOARD* board = m_frame->GetBoard();

    m_drills.clear();
    m_ptrToDrill.clear();
    m_hitlist.clear();

    auto addEntryOrIncrement = [&]( const DRILL_LINE_ITEM& d, BOARD_ITEM* rep )
        {
            for( DRILL_ROW& g : m_drills )
            {
                if( g.entry == d )
                {
                    g.entry.m_Qty++;
                    return;
                }
            }

            DRILL_ROW g = { .entry = d, .item = rep, };
            g.entry.m_Qty = 1;

            m_drills.push_back( g );
        };

    // Collect from pads
    for( FOOTPRINT* fp : board->Footprints() )
    {
        for( PAD* pad : fp->Pads() )
        {
            if( !pad->HasHole() )
                continue;

            int xs = pad->GetDrillSize().x;
            int ys = pad->GetDrillSize().y;
            if( xs <= 0 || ys <= 0 )
                continue;

            PCB_LAYER_ID top, bottom;

            if( pad->GetLayerSet().CuStack().empty() )
            {
                top = UNDEFINED_LAYER;
                bottom = UNDEFINED_LAYER;
            }
            else
            {
                top = pad->GetLayerSet().CuStack().front();
                bottom = pad->GetLayerSet().CuStack().back();
            }

            DRILL_LINE_ITEM d( xs, ys, pad->GetDrillShape(),
                               pad->GetAttribute() != PAD_ATTRIB::NPTH,
                               true, top, bottom );

            addEntryOrIncrement( d, pad );
        }
    }

    // Collect from vias
    for( PCB_TRACK* t : board->Tracks() )
    {
        if( t->Type() != PCB_VIA_T )
            continue;

        PCB_VIA* via = static_cast<PCB_VIA*>( t );
        int      dmm = via->GetDrillValue();
        if( dmm <= 0 )
            continue;

        DRILL_LINE_ITEM d( dmm, dmm, PAD_DRILL_SHAPE::CIRCLE, true,
                           false, via->TopLayer(), via->BottomLayer() );
        addEntryOrIncrement( d, via );
    }

    std::sort( m_drills.begin(), m_drills.end(),
               []( const DRILL_ROW& a, const DRILL_ROW& b )
               {
                   DRILL_LINE_ITEM::COMPARE cmp( DRILL_LINE_ITEM::COL_COUNT, false );
                   return cmp( a.entry, b.entry );
               } );

    // Apply filter and populate display list
    for( size_t i = 0; i < m_drills.size(); ++i )
    {
        if( aQuery.IsEmpty() || rowMatchesQuery( m_drills[i].entry, aQuery.Lower() ) )
        {
            m_hitlist.push_back( m_drills[i].item );
            m_ptrToDrill[m_drills[i].item] = (int) i;
        }
    }

    return (int) m_hitlist.size();
}


wxString DRILL_SEARCH_HANDLER::getResultCell( BOARD_ITEM* aItem, int aCol )
{
    auto it = m_ptrToDrill.find( aItem );

    if( it == m_ptrToDrill.end() )
        return wxEmptyString;

    const auto& e = m_drills[it->second].entry;

    return cellText( e, aCol );
}


void DRILL_SEARCH_HANDLER::Sort( int aCol, bool aAscending, std::vector<long>* aSelection )
{
    // Preserve current selection pointers
    std::vector<BOARD_ITEM*> selPtrs;

    if( aSelection )
    {
        for( long row : *aSelection )
        {
            if( row >= 0 && row < (long) m_hitlist.size() )
                selPtrs.push_back( m_hitlist[row] );
        }
    }

    auto cmpPtr = [&]( BOARD_ITEM* pa, BOARD_ITEM* pb )
    {
        const auto& a = m_drills[m_ptrToDrill[pa]].entry;
        const auto& b = m_drills[m_ptrToDrill[pb]].entry;

        int col = aCol < 0 ? 0 : aCol;
        DRILL_LINE_ITEM::COMPARE cmp( static_cast<DRILL_LINE_ITEM::COL_ID>( col ), aAscending );

        return cmp( a, b );
    };

    std::sort( m_hitlist.begin(), m_hitlist.end(), cmpPtr );

    // Rebuild selection rows from pointers
    if( aSelection )
    {
        aSelection->clear();

        for( long row = 0; row < (long) m_hitlist.size(); ++row )
        {
            if( alg::contains( selPtrs, m_hitlist[row] ) )
                aSelection->push_back( row );
        }
    }
}


void DRILL_SEARCH_HANDLER::SelectItems( std::vector<long>& aItemRows )
{
    BOARD*                          board = m_frame->GetBoard();
    std::vector<EDA_ITEM*>          selectedItems;
    APP_SETTINGS_BASE::SEARCH_PANE& settings = m_frame->config()->m_SearchPane;

    // Collect matching items
    for( long row : aItemRows )
    {
        if( row < 0 || row >= (long) m_hitlist.size() )
            continue;

        BOARD_ITEM* rep = m_hitlist[row];
        auto        it = m_ptrToDrill.find( rep );

        if( it == m_ptrToDrill.end() )
            continue;

        const auto* target = &m_drills[it->second].entry;

        // Pads
        for( FOOTPRINT* fp : board->Footprints() )
        {
            for( PAD* pad : fp->Pads() )
            {
                if( !pad->HasHole() )
                    continue;

                int          xs = pad->GetDrillSize().x;
                int          ys = pad->GetDrillSize().y;
                PCB_LAYER_ID top, bottom;

                if( pad->GetLayerSet().CuStack().empty() )
                {
                    top = UNDEFINED_LAYER;
                    bottom = UNDEFINED_LAYER;
                }
                else
                {
                    top = pad->GetLayerSet().CuStack().front();
                    bottom = pad->GetLayerSet().CuStack().back();
                }

                DRILL_LINE_ITEM e( xs, ys, pad->GetDrillShape(), pad->GetAttribute() != PAD_ATTRIB::NPTH, true,
                                   top, bottom );

                if( e == *target )
                    selectedItems.push_back( pad );
            }
        }

        // Vias
        for( PCB_TRACK* t : board->Tracks() )
        {
            if( t->Type() != PCB_VIA_T )
                continue;

            PCB_VIA* via = static_cast<PCB_VIA*>( t );
            DRILL_LINE_ITEM e( via->GetDrillValue(), via->GetDrillValue(), PAD_DRILL_SHAPE::CIRCLE, true,
                               false, via->TopLayer(), via->BottomLayer() );

            if( e == *target )
                selectedItems.push_back( via );
        }
    }


    m_frame->GetToolManager()->RunAction( ACTIONS::selectionClear );

    if( selectedItems.size() )
    {
        m_frame->GetToolManager()->RunAction( ACTIONS::selectItems, &selectedItems );

        switch( settings.selection_zoom )
        {
        case APP_SETTINGS_BASE::SEARCH_PANE::SELECTION_ZOOM::PAN:
            m_frame->GetToolManager()->RunAction( ACTIONS::centerSelection );
            break;
        case APP_SETTINGS_BASE::SEARCH_PANE::SELECTION_ZOOM::ZOOM:
            m_frame->GetToolManager()->RunAction( ACTIONS::zoomFitSelection );
            break;
        case APP_SETTINGS_BASE::SEARCH_PANE::SELECTION_ZOOM::NONE:
            break;
        }
    }

    m_frame->GetCanvas()->Refresh( false );
}


wxString DRILL_SEARCH_HANDLER::cellText( const DRILL_LINE_ITEM& e, int col ) const
{
    BOARD* board = m_frame->GetBoard();

    switch( col )
    {
    case 0:  return wxString::Format( "%d", e.m_Qty );
    case 1:  return e.shape == PAD_DRILL_SHAPE::CIRCLE ? _( "Round" ) : _( "Slot" );
    case 2:  return m_frame->MessageTextFromValue( e.xSize );
    case 3:  return m_frame->MessageTextFromValue( e.ySize );
    case 4:  return e.isPlated ? _( "PTH" ) : _( "NPTH" );
    case 5:  return e.isPad ? _( "Pad" ) : _( "Via" );
    case 6:  return ( e.startLayer == UNDEFINED_LAYER ) ? _( "N/A" ) : board->GetLayerName( e.startLayer );
    case 7:  return ( e.stopLayer == UNDEFINED_LAYER ) ? _( "N/A" ) : board->GetLayerName( e.stopLayer );
    default: return wxEmptyString;
    }
}


bool DRILL_SEARCH_HANDLER::rowMatchesQuery( const DRILL_LINE_ITEM& e, const wxString& aQuery ) const
{
    if( aQuery.IsEmpty() )
        return true;

    for( int col = 0; col < 8; ++col )
    {
        if( cellText( e, col ).Lower().Contains( aQuery ) )
            return true;
    }

    return false;
}
