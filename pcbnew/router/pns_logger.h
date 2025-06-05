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
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __PNS_LOGGER_H
#define __PNS_LOGGER_H

#include <cstdio>
#include <vector>
#include <string>
#include <sstream>

#include <math/vector2d.h>
#include <kiid.h>

#include "pns_sizes_settings.h"

class SHAPE_LINE_CHAIN;
class SHAPE;

namespace PNS {

class ITEM;

class LOGGER
{
public:

    enum EVENT_TYPE {
        EVT_START_ROUTE = 0,
        EVT_START_DRAG,
        EVT_FIX,
        EVT_MOVE,
        EVT_ABORT,
        EVT_TOGGLE_VIA,
        EVT_UNFIX,
        EVT_START_MULTIDRAG
    };

    struct EVENT_ENTRY {
        VECTOR2I p;
        EVENT_TYPE type;
        std::vector<KIID> uuids;
        SIZES_SETTINGS sizes;
        int layer;

        EVENT_ENTRY() :
                layer( 0 ),
                type( EVT_START_ROUTE )
        {
        }

        EVENT_ENTRY( const EVENT_ENTRY& aE ) :
                p( aE.p ),
                type( aE.type ),
                uuids( aE.uuids ),
                sizes( aE.sizes ),
                layer( aE.layer )
        {
        }
    };

    LOGGER();
    ~LOGGER();

    void Clear();

    void LogM( EVENT_TYPE evt, const VECTOR2I& pos = VECTOR2I(), std::vector<ITEM*> items = {},
              const SIZES_SETTINGS* sizes = nullptr, int aLayer = 0 );

    void Log( EVENT_TYPE evt, const VECTOR2I& pos = VECTOR2I(), const ITEM* item = nullptr,
              const SIZES_SETTINGS* sizes = nullptr, int aLayer = 0 );

    const std::vector<EVENT_ENTRY>& GetEvents()
    {
        return m_events;
    }

    static wxString FormatLogFileAsString( int aMode,
                                           const std::vector<ITEM*>& aAddedItems,
                                           const std::set<KIID>&     aRemovedItems,
                                           const std::vector<ITEM*>& aHeads,
                                           const std::vector<EVENT_ENTRY>& aEvents );

    static wxString FormatEvent( const EVENT_ENTRY& aEvent );

    static EVENT_ENTRY ParseEvent( const wxString& aLine );

private:
    std::vector<EVENT_ENTRY> m_events;
};

}

#endif
