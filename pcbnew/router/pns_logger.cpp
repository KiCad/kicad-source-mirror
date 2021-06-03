/*
 * KiRouter - a push-and-(sometimes-)shove PCB router
 *
 * Copyright (C) 2013-2014 CERN
 * Copyright (C) 2016 KiCad Developers, see AUTHORS.txt for contributors.
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


void LOGGER::Save( const std::string& aFilename )
{
    FILE* f = fopen( aFilename.c_str(), "wb" );

    wxLogTrace( "PNS", "Saving to '%s' [%p]", aFilename.c_str(), f );

    for( const EVENT_ENTRY& evt : m_events )
    {
        uint64_t id = 0;

        fprintf( f, "event %d %d %d %s\n", evt.type, evt.p.x, evt.p.y, evt.uuid.c_str() );
    }

    fclose( f );
}


void LOGGER::Log( LOGGER::EVENT_TYPE evt, VECTOR2I pos, const ITEM* item )
{
    LOGGER::EVENT_ENTRY ent;

    ent.type = evt;
    ent.p = pos;
    ent.uuid = "null";


    if( item && item->Parent() )
        ent.uuid = item->Parent()->m_Uuid.AsString();

    m_events.push_back( ent );
}

}
