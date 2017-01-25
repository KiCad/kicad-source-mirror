/*
 * KiRouter - a push-and-(sometimes-)shove PCB router
 *
 * Copyright (C) 2013-2016 CERN
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

#ifndef __PNS_KICAD_IFACE_H
#define __PNS_KICAD_IFACE_H

#include <unordered_set>

#include "pns_router.h"

class PNS_PCBNEW_RULE_RESOLVER;
class PNS_PCBNEW_DEBUG_DECORATOR;

class BOARD;
class BOARD_COMMIT;
class DISPLAY_OPTIONS;

namespace KIGFX
{
    class VIEW;
};

class PNS_KICAD_IFACE : public PNS::ROUTER_IFACE {
public:
    PNS_KICAD_IFACE();
    ~PNS_KICAD_IFACE();

    void SetRouter( PNS::ROUTER* aRouter ) override;
    void SetHostFrame( PCB_EDIT_FRAME* aFrame );

    void SetBoard( BOARD* aBoard );
    void SetView( KIGFX::VIEW* aView );
    void SyncWorld( PNS::NODE* aWorld ) override;
    void EraseView() override;
    void HideItem( PNS::ITEM* aItem ) override;
    void DisplayItem( const PNS::ITEM* aItem, int aColor = 0, int aClearance = 0 ) override;
    void AddItem( PNS::ITEM* aItem ) override;
    void RemoveItem( PNS::ITEM* aItem ) override;
    void Commit() override;

    void UpdateNet( int aNetCode ) override;

    PNS::RULE_RESOLVER* GetRuleResolver() override;
    PNS::DEBUG_DECORATOR* GetDebugDecorator() override;

private:
    PNS_PCBNEW_RULE_RESOLVER* m_ruleResolver;
    PNS_PCBNEW_DEBUG_DECORATOR* m_debugDecorator;

    std::unique_ptr<PNS::SOLID>   syncPad( D_PAD* aPad );
    std::unique_ptr<PNS::SEGMENT> syncTrack( TRACK* aTrack );
    std::unique_ptr<PNS::VIA>     syncVia( VIA* aVia );

    KIGFX::VIEW* m_view;
    KIGFX::VIEW_GROUP* m_previewItems;
    std::unordered_set<BOARD_CONNECTED_ITEM*> m_hiddenItems;

    PNS::NODE* m_world;
    PNS::ROUTER* m_router;
    BOARD* m_board;
    PICKED_ITEMS_LIST m_undoBuffer;
    PCB_EDIT_FRAME* m_frame;
    std::unique_ptr<BOARD_COMMIT> m_commit;
    DISPLAY_OPTIONS* m_dispOptions;
};

#endif
