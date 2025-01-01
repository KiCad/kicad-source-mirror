/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers.
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

    struct COMMIT_STATE
    {
        COMMIT_STATE(){};
        COMMIT_STATE( const COMMIT_STATE& aOther ) :
                m_removedIds( aOther.m_removedIds ), m_addedItems( aOther.m_addedItems ),
                m_heads( aOther.m_heads )
        {
        }

        std::set<KIID>          m_removedIds;
        std::vector<PNS::ITEM*> m_addedItems;
        std::vector<PNS::ITEM*> m_heads;

        bool Compare( const COMMIT_STATE& aOther );
    };

    // Saves a P&S event log only (e.g. after fixing a bug and wanting a new "golden" commit state)
    bool SaveLog( const wxFileName& logFileName, REPORTER* aRpt );

    // Loads a P&S event log and the associated board file. These two always go together.
    bool Load( const wxFileName& logFileName, REPORTER* aRpt );

    std::vector<BOARD_CONNECTED_ITEM*> ItemsById( const PNS::LOGGER::EVENT_ENTRY& evt );
    BOARD_CONNECTED_ITEM* ItemById( const PNS::LOGGER::EVENT_ENTRY& evt );

    std::vector<PNS::LOGGER::EVENT_ENTRY>& Events() { return m_events; }

    void SetBoard( std::shared_ptr<BOARD> brd ) { m_board = brd; }
    std::shared_ptr<BOARD> GetBoard() const { return m_board; }

    PNS::ROUTING_SETTINGS* GetRoutingSettings() const { return m_routerSettings.get(); }

    const COMMIT_STATE& GetExpectedResult() const { return m_commitState; }

    void SetExpectedResult( const COMMIT_STATE&                     aCommitState,
                            std::vector<std::unique_ptr<PNS::ITEM>> aParsedItems )
    {
        m_commitState = aCommitState;
        m_parsed_items = std::move( aParsedItems );
    }

    PNS::ROUTER_MODE GetMode() const { return m_mode; }

    void SetMode( PNS::ROUTER_MODE aMode ) { m_mode = aMode; }

private:
    bool parseCommonPnsProps( PNS::ITEM* aItem, const wxString& cmd, wxStringTokenizer& aTokens );

    std::unique_ptr<PNS::SEGMENT> parsePnsSegmentFromString( wxStringTokenizer& aTokens );

    std::unique_ptr<PNS::VIA> parsePnsViaFromString( wxStringTokenizer& aTokens );

    std::unique_ptr<PNS::ITEM> parseItemFromString( wxStringTokenizer& aTokens );

    std::shared_ptr<SHAPE> parseShape( SHAPE_TYPE expectedType, wxStringTokenizer& aTokens );

private:
    std::shared_ptr<SETTINGS_MANAGER>       m_settingsMgr;
    std::unique_ptr<PNS::ROUTING_SETTINGS>  m_routerSettings;
    std::vector<PNS::LOGGER::EVENT_ENTRY>   m_events;
    std::shared_ptr<BOARD>                  m_board;
    COMMIT_STATE                            m_commitState;
    std::vector<std::unique_ptr<PNS::ITEM>> m_parsed_items;
    PNS::ROUTER_MODE                        m_mode;
};

#endif
