/*
 * KiRouter - a push-and-(sometimes-)shove PCB router
 *
 * Copyright (C) 2013-2016 CERN
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
namespace KIGFX {
    class VIEW;

};

class PNS_KICAD_IFACE : public PNS_ROUTER_IFACE {
public:
    PNS_KICAD_IFACE ();
    ~PNS_KICAD_IFACE ();

    void SetRouter( PNS_ROUTER *aRouter );
    void SetHostFrame ( PCB_EDIT_FRAME *aFrame );

    void SetBoard( BOARD* aBoard );
    void SetView ( KIGFX::VIEW *aView );
    void SyncWorld( PNS_NODE *aWorld );
    void EraseView();
    void HideItem ( PNS_ITEM *aItem );
    void DisplayItem( const PNS_ITEM* aItem, int aColor = 0, int aClearance = 0);
    void AddItem ( PNS_ITEM *aItem );
    void RemoveItem ( PNS_ITEM *aItem );
    void Commit ();

    void UpdateNet ( int aNetCode );

    PNS_RULE_RESOLVER* GetRuleResolver();
    PNS_DEBUG_DECORATOR* GetDebugDecorator();

private:

    PNS_PCBNEW_RULE_RESOLVER* m_ruleResolver;
    PNS_PCBNEW_DEBUG_DECORATOR* m_debugDecorator;

    PNS_ITEM* syncPad( D_PAD* aPad );
    PNS_ITEM* syncTrack( TRACK* aTrack );
    PNS_ITEM* syncVia( VIA* aVia );

    KIGFX::VIEW *m_view;
    KIGFX::VIEW_GROUP* m_previewItems;
    std::unordered_set<BOARD_CONNECTED_ITEM*> m_hiddenItems;

    PNS_NODE *m_world;
    PNS_ROUTER *m_router;
    BOARD *m_board;
    PICKED_ITEMS_LIST m_undoBuffer;
    PCB_EDIT_FRAME *m_frame;
};

#endif
