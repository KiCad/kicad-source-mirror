/*
 * KiRouter - a push-and-(sometimes-)shove PCB router
 *
 * Copyright (C) 2013-2014 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 * Author: Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "pns_logger.h"

#include <json_common.h>

#include <wx/log.h>
#include <wx/tokenzr.h>
#include <locale_io.h>

#include <board_item.h>

#include "pns_item.h"
#include "pns_via.h"
#include "pns_segment.h"
#include "pns_line.h"
#include "pns_router.h"


void to_json( nlohmann::json& aJson, const VECTOR2I& aPoint )
{
    aJson = nlohmann::json
            {
                { "x", aPoint.x },
                { "y", aPoint.y }
            };
}


void from_json( const nlohmann::json& aJson, VECTOR2I& aPoint )
{
    aPoint.x = aJson.at( "x" ).get<int>();
    aPoint.y = aJson.at( "y" ).get<int>();
}


namespace PNS {

LOGGER::LOGGER( )
{
}


LOGGER::~LOGGER()
{
}


void LOGGER::Clear()
{
    m_events.clear();
}


void LOGGER::LogM( LOGGER::EVENT_TYPE evt, const VECTOR2I& pos, std::vector<ITEM*> items,
                  const SIZES_SETTINGS* sizes, int aLayer )
{
    LOGGER::EVENT_ENTRY ent;

    ent.type = evt;
    ent.p = pos;
    ent.layer = aLayer;

    if( sizes )
    {
        ent.sizes = *sizes;
    }

    for( auto& item : items )
    {
        if( item && item->Parent() )
            ent.uuids.push_back( item->Parent()->m_Uuid );
    }

    m_events.push_back( ent );
}


void LOGGER::Log( LOGGER::EVENT_TYPE evt, const VECTOR2I& pos, const ITEM* item,
                  const SIZES_SETTINGS* sizes, int aLayer )
{
    std::vector<ITEM*> items;
    items.push_back( const_cast<ITEM*>( item ) );
    LogM( evt, pos, items, sizes, aLayer );
}

wxString LOGGER::FormatLogFileAsJSON( const LOG_DATA& aLogData )
{
    nlohmann::json json;

    
    json["mode"] = aLogData.m_Mode;
    
    if( aLogData.m_TestCaseType.has_value() )
    {
        json["test_case_type"] = static_cast<int>( aLogData.m_TestCaseType.value() );
    }

    if( aLogData.m_BoardHash.has_value() )
    {
        json["board_hash"] = aLogData.m_BoardHash.value();
    }

    nlohmann::json events = nlohmann::json::array();
    for( const EVENT_ENTRY& evt : aLogData.m_Events )
        events.push_back ( LOGGER::FormatEventAsJSON( evt ) );   
    json["events"] = events;

    nlohmann::json removed = nlohmann::json::array();
    for( const KIID& uuid : aLogData.m_RemovedItems )
        removed.push_back( uuid );
    json["removedItems"] = removed;

    nlohmann::json added = nlohmann::json::array();
    for( ITEM* item : aLogData.m_AddedItems )
        added.push_back( formatRouterItemAsJSON( item ) );
    json["addedItems"] = added;

    nlohmann::json headItems = nlohmann::json::array();
    for( ITEM* item : aLogData.m_Heads )
        headItems.push_back( formatRouterItemAsJSON( item ) );
    json["headItems"] = headItems;

    LOCALE_IO dummy;

    std::stringstream buffer;
    buffer << std::setw( 2 ) << json << std::endl;

    return buffer.str();
}


nlohmann::json LOGGER::FormatEventAsJSON( const LOGGER::EVENT_ENTRY& aEvent )
{
    nlohmann::json ret;

    ret["position"] = aEvent.p;
    ret["type"] = aEvent.type;
    ret["layer"] = aEvent.layer;
    
    nlohmann::json uuids = nlohmann::json::array();

    for( int i = 0; i < (int) aEvent.uuids.size(); i++ )
        uuids.push_back( aEvent.uuids[i].AsString() );

    ret["uuids"] = uuids;
    ret["sizes"] = LOGGER::formatSizesAsJSON( aEvent.sizes );
    
    return ret;
}


nlohmann::json LOGGER::formatRouterItemAsJSON( const PNS::ITEM* aItem )
    {
    ROUTER*       router = ROUTER::GetInstance();
    ROUTER_IFACE* iface = router ? router->GetInterface() : nullptr;

    nlohmann::json ret;

    ret ["kind"] = aItem->KindStr();

    if( iface )
    {
        ret ["net"] = iface->GetNetName( aItem->Net() );
    }

    ret ["layers"] = nlohmann::json{ aItem->Layers().Start(), aItem->Layers().End() };

    switch( aItem->Kind() )
    {
        case ITEM::SEGMENT_T:
        case ITEM::ARC_T:
            ret["shape"] = formatShapeAsJSON( aItem->Shape( aItem->Layer() ) );
            break;

        case ITEM::VIA_T:
        {
            auto via = static_cast<const VIA*>( aItem );
            ret["shape"] = formatShapeAsJSON( aItem->Shape( aItem->Layers().Start() ) ); // JE: pad stacks
            ret["drill"] = via->Drill();
            break;
        }
        default:
            break; // currently we don't need to log anything else
    }

    return ret;
}


nlohmann::json LOGGER::formatSizesAsJSON( const SIZES_SETTINGS& aSizes )
{
    return nlohmann::json( {

            { "trackWidth", aSizes.TrackWidth() },
            { "viaDiameter", aSizes.ViaDiameter() },
            { "viaDrill", aSizes.ViaDrill() },
            { "trackWidthIsExplicit", aSizes.TrackWidthIsExplicit() },
            { "layerBottom", aSizes.GetLayerBottom() },
            { "layerTop", aSizes.GetLayerTop() },
            { "viaType", aSizes.ViaType() } } );
}


nlohmann::json LOGGER::formatShapeAsJSON( const SHAPE* aShape )
{
    switch( aShape->Type() )
    {
        case SH_SEGMENT:
        {
            auto seg = static_cast<const SHAPE_SEGMENT*>( aShape );
            return nlohmann::json( {
                { "type", "segment" },
                { "width", seg->GetWidth() },
                { "start", seg->GetStart() },
                { "end", seg->GetEnd() }
            } );
        }
        case SH_ARC:
        {
            auto arc = static_cast<const SHAPE_ARC*>( aShape );
            return nlohmann::json( {
                { "type", "arc" },
                { "width", arc->GetWidth() },
                { "start", arc->GetStart() },
                { "end", arc->GetEnd() },
                { "mid", arc->GetArcMid() }
            } );
        }
        case SH_CIRCLE:
        {
            auto circle = static_cast<const SHAPE_CIRCLE*>( aShape );
            return nlohmann::json( {
                { "type", "circle" },
                { "radius", circle->GetRadius() },
                { "center", circle->GetCenter() },
            } );
        }

        default:
            break;
    }

    return nlohmann::json();
}


LOGGER::EVENT_ENTRY LOGGER::ParseEvent( const wxString& aLine )
{
    wxStringTokenizer tokens( aLine );
    wxString          cmd = tokens.GetNextToken();

    int n_uuids = 0;

    wxCHECK_MSG( cmd == wxT( "event" ), EVENT_ENTRY(), "Line doesn't contain an event!" );

    EVENT_ENTRY evt;
    evt.p.x = wxAtoi( tokens.GetNextToken() );
    evt.p.y = wxAtoi( tokens.GetNextToken() );
    evt.type = (PNS::LOGGER::EVENT_TYPE) wxAtoi( tokens.GetNextToken() );
    evt.layer = wxAtoi( tokens.GetNextToken() );
    n_uuids = wxAtoi( tokens.GetNextToken() );

    for( int i = 0; i < n_uuids; i++)
        evt.uuids.push_back( KIID( tokens.GetNextToken() ) );

    return evt;
}


LOGGER::EVENT_ENTRY LOGGER::ParseEventFromJSON( const nlohmann::json& aJSON )
{
    EVENT_ENTRY evt;

    evt.p = aJSON.at("position").get<VECTOR2I>();
    evt.type = static_cast<EVENT_TYPE>( aJSON.at("type").get<int>() );
    evt.layer = aJSON.at("layer").get<int>();
    
    for( auto &uuid : aJSON.at("uuids") )
        evt.uuids.push_back( uuid.get<KIID>() );

    return evt;
}


}

