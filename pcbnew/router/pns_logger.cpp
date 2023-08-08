/*
 * KiRouter - a push-and-(sometimes-)shove PCB router
 *
 * Copyright (C) 2013-2014 CERN
 * Copyright (C) 2016-2023 KiCad Developers, see AUTHORS.txt for contributors.
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
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "pns_logger.h"
#include "pns_item.h"
#include "pns_via.h"

#include <wx/log.h>

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


void LOGGER::Log( LOGGER::EVENT_TYPE evt, const VECTOR2I& pos, const ITEM* item,
                  const SIZES_SETTINGS* sizes )
{
    LOGGER::EVENT_ENTRY ent;

    ent.type = evt;
    ent.p = pos;
    ent.uuid = KIID( 0 );

    if( sizes )
    {
        ent.sizes = *sizes;
    }

    if( item && item->Parent() )
        ent.uuid = item->Parent()->m_Uuid;

    m_events.push_back( ent );
}


wxString LOGGER::FormatLogFileAsString( int aMode,
                                        const std::vector<ITEM*>& aAddedItems,
                                        const std::set<KIID>&     aRemovedItems,
                                        const std::vector<ITEM*>& aHeads,
                                        const std::vector<LOGGER::EVENT_ENTRY>& aEvents )
{
    wxString result = wxString::Format( "mode %d\n", aMode );

    for( const EVENT_ENTRY& evt : aEvents )
        result += PNS::LOGGER::FormatEvent( evt );

    for( const KIID& uuid : aRemovedItems )
        result += wxString::Format( "removed %s\n", uuid.AsString().c_str() );

    for( ITEM* item : aAddedItems )
        result += wxString::Format( "added %s\n", item->Format().c_str() );

    for( ITEM* item : aHeads )
        result += wxString::Format( "head %s\n", item->Format().c_str() );

    return result;
}


wxString LOGGER::FormatEvent( const LOGGER::EVENT_ENTRY& aEvent )
{
    return wxString::Format(
            "event %d %d %d %s %d %d %d %d %d %d %d\n", aEvent.p.x, aEvent.p.y, aEvent.type,
            static_cast<const char*>( aEvent.uuid.AsString().c_str() ), aEvent.sizes.TrackWidth(),
            aEvent.sizes.ViaDiameter(), aEvent.sizes.ViaDrill(),
            aEvent.sizes.TrackWidthIsExplicit() ? 1 : 0, aEvent.sizes.GetLayerBottom(),
            aEvent.sizes.GetLayerTop(), static_cast<int>( aEvent.sizes.ViaType() ) );

}


LOGGER::EVENT_ENTRY LOGGER::ParseEvent( const wxString& aLine )
{
    wxStringTokenizer tokens( aLine );
    wxString          cmd = tokens.GetNextToken();

    wxCHECK_MSG( cmd == wxT( "event" ), EVENT_ENTRY(), "Line doesn't contain an event!" );

    EVENT_ENTRY evt;
    evt.p.x = wxAtoi( tokens.GetNextToken() );
    evt.p.y = wxAtoi( tokens.GetNextToken() );
    evt.type = (PNS::LOGGER::EVENT_TYPE) wxAtoi( tokens.GetNextToken() );
    evt.uuid = KIID( tokens.GetNextToken() );

    return evt;
}

}
