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

#ifndef __PNS_LOGGER_H
#define __PNS_LOGGER_H

#include <cstdio>
#include <vector>
#include <string>
#include <sstream>
#include <set>

#include <math/vector2d.h>
#include <kiid.h>
#include <json_common.h>

#include "pns_sizes_settings.h"
#include "pns_router.h"

class SHAPE_LINE_CHAIN;
class SHAPE;

void to_json( nlohmann::json& aJson, const VECTOR2I& aPoint );
void from_json( const nlohmann::json& aJson, VECTOR2I& aPoint );

namespace PNS {

class ITEM;

class LOGGER
{
public:

    enum TEST_CASE_TYPE 
    {
        TCT_STRICT_GEOMETRY = 0,
        TCT_CONNECTIVITY_ONLY,
        TCT_EXPECTED_FAIL,
        TCT_KNOWN_BUG
    };

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
                type( EVT_START_ROUTE ),
                layer( 0 )
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

    struct LOG_DATA
    {
        LOG_DATA() {};
        ~LOG_DATA() {};
        ROUTER_MODE m_Mode;
        std::optional<wxString> m_BoardHash;
        std::vector<ITEM*> m_AddedItems;
        std::set<KIID> m_RemovedItems;
        std::vector<ITEM*> m_Heads;
        std::vector<EVENT_ENTRY> m_Events;
        std::optional<TEST_CASE_TYPE> m_TestCaseType;
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

    static nlohmann::json FormatEventAsJSON( const EVENT_ENTRY& aEvent );

    static EVENT_ENTRY ParseEvent( const wxString& aLine );
    static EVENT_ENTRY ParseEventFromJSON( const nlohmann::json& aJSON );
    static wxString FormatLogFileAsJSON( const LOG_DATA& aLogData );

private:

    static nlohmann::json formatSizesAsJSON( const SIZES_SETTINGS& aEvent );
    static nlohmann::json formatRouterItemAsJSON( const PNS::ITEM* aItem );
    static nlohmann::json formatShapeAsJSON( const SHAPE* aShape );

    std::vector<EVENT_ENTRY> m_events;
    std::optional<TEST_CASE_TYPE> m_testCaseType;
};

}

#endif
