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
#ifndef __PNS_LOG_PLAYER_H
#define __PNS_LOG_PLAYER_H

#include <map>
#include <pcbnew/board.h>

#include <router/pns_routing_settings.h>
#include <router/pns_kicad_iface.h>
#include <router/pns_router.h>


class PNS_TEST_DEBUG_DECORATOR;
class PNS_LOG_FILE;
class PNS_LOG_PLAYER;
class REPORTER;

class PNS_LOG_VIEW_TRACKER
{
public:
    struct ENTRY
    {
        ENTRY() = default;
        ENTRY( const PNS_LOG_VIEW_TRACKER::ENTRY& ) = delete;

        ENTRY( ENTRY&& aOther ) :
                m_isHideOp( aOther.m_isHideOp ),
                m_item( aOther.m_item ),
                m_ownedItem( std::move( aOther.m_ownedItem ) )
        {}

        bool                             m_isHideOp{ false };
        const PNS::ITEM*                 m_item{ nullptr };
        std::unique_ptr<const PNS::ITEM> m_ownedItem{ nullptr };
    };

    typedef std::vector<ENTRY> VIEW_ENTRIES;

    PNS_LOG_VIEW_TRACKER();
    ~PNS_LOG_VIEW_TRACKER();

    void SetStage( int aStage );
    VIEW_ENTRIES& GetEntriesForStage( int aStage ) { return m_vitems[aStage]; }

    void HideItem( PNS::ITEM* aItem );
    void DisplayItem( const PNS::ITEM* aItem );


private:
    int m_currentStage;
    std::map<int, VIEW_ENTRIES> m_vitems;
};

class PNS_LOG_PLAYER_KICAD_IFACE : public PNS_KICAD_IFACE_BASE
{
public:
    PNS_LOG_PLAYER_KICAD_IFACE( PNS_LOG_VIEW_TRACKER* aViewTracker );
    ~PNS_LOG_PLAYER_KICAD_IFACE() override;

    void HideItem( PNS::ITEM* aItem ) override;
    void DisplayItem( const PNS::ITEM* aItem, int aClearance, bool aEdit = false,
                      int aFlags = 0 ) override;

    int GetNetCode( PNS::NET_HANDLE aNet ) const override;
    wxString GetNetName( PNS::NET_HANDLE aNet ) const override;

private:
    PNS_LOG_VIEW_TRACKER *m_viewTracker;
};


class PNS_LOG_PLAYER
{
public:
    PNS_LOG_PLAYER();
    ~PNS_LOG_PLAYER();

    void ReplayLog( PNS_LOG_FILE* aLog, int aStartEventIndex = 0, int aFrom = 0, int aTo = -1,
                    bool aUpdateExpectedResult = false );

    void SetReporter( REPORTER* aReporter ) { m_reporter = aReporter; }

    PNS_TEST_DEBUG_DECORATOR* GetDebugDecorator() { return m_debugDecorator; };
    std::shared_ptr<PNS_LOG_VIEW_TRACKER> GetViewTracker() { return m_viewTracker; }

    void SetTimeLimit( uint64_t microseconds ) { m_timeLimitUs = microseconds; }

    bool CompareResults( PNS_LOG_FILE* aLog );
    const PNS_LOG_FILE::COMMIT_STATE GetRouterUpdatedItems();

private:
    void createRouter();

    std::shared_ptr<PNS_LOG_VIEW_TRACKER>       m_viewTracker;
    std::unique_ptr<PNS_LOG_PLAYER_KICAD_IFACE> m_iface; // needs to be deleted after m_router
    PNS_TEST_DEBUG_DECORATOR*                   m_debugDecorator;
    std::shared_ptr<BOARD>                      m_board;
    std::unique_ptr<PNS::ROUTER>                m_router;
    std::unique_ptr<PNS::ROUTING_SETTINGS>      m_routingSettings;
    uint64_t m_timeLimitUs;
    REPORTER* m_reporter;
};

#endif
