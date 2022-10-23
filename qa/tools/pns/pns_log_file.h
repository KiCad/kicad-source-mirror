/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020-2022 KiCad Developers.
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


// WARNING - this Tom's crappy PNS hack tool code. Please don't complain about its quality
// (unless you want to improve it).

#ifndef __PNS_LOG_FILE_H
#define __PNS_LOG_FILE_H

#include <memory>
#include <vector>
#include <set>

#include <wx/filename.h>

#include <kiid.h>
#include <math/vector2d.h>

#include <router/pns_logger.h>
#include <router/pns_router.h>
#include <router/pns_item.h>
#include <router/pns_routing_settings.h>

#include <pcbnew/board.h>
#include <pcbnew/board_connected_item.h>

#include <settings/settings_manager.h>

class PNS_LOG_FILE
{
public:
    PNS_LOG_FILE();
    ~PNS_LOG_FILE() {}

    struct EVENT_ENTRY
    {
        VECTOR2I                p;
        PNS::LOGGER::EVENT_TYPE type;
        const PNS::ITEM*        item;
        KIID                    uuid;
    };

    struct COMMIT_STATE 
    {
        COMMIT_STATE() {};
        COMMIT_STATE( const COMMIT_STATE& aOther ) :
            m_removedIds( aOther.m_removedIds ),
            m_addedItems( aOther.m_addedItems )
        {

        }

        std::set<KIID>                      m_removedIds;
        std::set<PNS::ITEM*>                m_addedItems;

        bool Compare( const COMMIT_STATE& aOther );
    };

    // Loads a P&S event log and the associated board file. These two always go together.
    bool Load( const wxFileName& logFileName, REPORTER* aRpt );

    BOARD_CONNECTED_ITEM* ItemById( const EVENT_ENTRY& evt );

    std::vector<EVENT_ENTRY>& Events() { return m_events; }

    void SetBoard( std::shared_ptr<BOARD> brd ) { m_board = brd; }
    std::shared_ptr<BOARD> GetBoard() const { return m_board; }

    PNS::ROUTING_SETTINGS* GetRoutingSettings() const { return m_routerSettings.get(); }

    const COMMIT_STATE& GetExpectedResult() const { return m_commitState; }

    PNS::ROUTER_MODE GetMode() const { return m_mode; }

private:
    std::shared_ptr<SETTINGS_MANAGER>      m_settingsMgr;
    std::unique_ptr<PNS::ROUTING_SETTINGS> m_routerSettings;
    std::vector<EVENT_ENTRY>               m_events;
    std::shared_ptr<BOARD>                 m_board;
    COMMIT_STATE                           m_commitState;
    PNS::ROUTER_MODE                       m_mode;
};

#endif
