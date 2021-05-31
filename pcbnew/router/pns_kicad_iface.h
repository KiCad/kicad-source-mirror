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
class PCB_DISPLAY_OPTIONS;
class PCB_TOOL_BASE;
class FOOTPRINT;
class PAD;

namespace PNS
{
    class SIZES_SETTINGS;
}

namespace KIGFX
{
    class VIEW;
}

class PNS_KICAD_IFACE_BASE : public PNS::ROUTER_IFACE
{
public:
    PNS_KICAD_IFACE_BASE();
    ~PNS_KICAD_IFACE_BASE();

    void EraseView() override {};
    void SetBoard( BOARD* aBoard );
    void SyncWorld( PNS::NODE* aWorld ) override;
    bool IsAnyLayerVisible( const LAYER_RANGE& aLayer ) const override { return true; };
    bool IsFlashedOnLayer( const PNS::ITEM* aItem, int aLayer ) const override { return true; };
    bool IsItemVisible( const PNS::ITEM* aItem ) const override { return true; }
    void HideItem( PNS::ITEM* aItem ) override {}
    void DisplayItem( const PNS::ITEM* aItem, int aClearance, bool aEdit = false ) override {}
    void DisplayRatline( const SHAPE_LINE_CHAIN& aRatline, int aColor = -1 ) override {}
    void AddItem( PNS::ITEM* aItem ) override;
    void UpdateItem( PNS::ITEM* aItem ) override;
    void RemoveItem( PNS::ITEM* aItem ) override;
    void Commit() override {}
    bool ImportSizes( PNS::SIZES_SETTINGS& aSizes, PNS::ITEM* aStartItem, int aNet ) override;
    int StackupHeight( int aFirstLayer, int aSecondLayer ) const override;

    void UpdateNet( int aNetCode ) override {}

    void SetDebugDecorator( PNS::DEBUG_DECORATOR *aDec );

    virtual PNS::NODE* GetWorld() const override
    {
        return m_world;
    };

    BOARD* GetBoard() const
    {
        return m_board;
    }

    PNS::RULE_RESOLVER* GetRuleResolver() override;
    PNS::DEBUG_DECORATOR* GetDebugDecorator() override;

protected:
    PNS_PCBNEW_RULE_RESOLVER* m_ruleResolver;
    PNS::DEBUG_DECORATOR* m_debugDecorator;

    std::unique_ptr<PNS::SOLID>   syncPad( PAD* aPad );
    std::unique_ptr<PNS::SEGMENT> syncTrack( TRACK* aTrack );
    std::unique_ptr<PNS::ARC>     syncArc( ARC* aArc );
    std::unique_ptr<PNS::VIA>     syncVia( VIA* aVia );
    bool syncTextItem( PNS::NODE* aWorld, EDA_TEXT* aText, PCB_LAYER_ID aLayer );
    bool syncGraphicalItem( PNS::NODE* aWorld, PCB_SHAPE* aItem );
    bool syncZone( PNS::NODE* aWorld, ZONE* aZone, SHAPE_POLY_SET* aBoardOutline );
    bool inheritTrackWidth( PNS::ITEM* aItem, int* aInheritedWidth );

protected:
    PNS::NODE* m_world;
    BOARD*     m_board;
};

class PNS_KICAD_IFACE : public PNS_KICAD_IFACE_BASE
{
public:
    PNS_KICAD_IFACE();
    ~PNS_KICAD_IFACE();

    void SetHostTool( PCB_TOOL_BASE* aTool );
    void SetDisplayOptions( const PCB_DISPLAY_OPTIONS* aDispOptions );

    void SetView( KIGFX::VIEW* aView );
    void EraseView() override;
    bool IsAnyLayerVisible( const LAYER_RANGE& aLayer ) const override;
    bool IsItemVisible( const PNS::ITEM* aItem ) const override;
    bool IsFlashedOnLayer( const PNS::ITEM* aItem, int aLayer ) const override;
    void HideItem( PNS::ITEM* aItem ) override;
    void DisplayItem( const PNS::ITEM* aItem, int aClearance, bool aEdit = false ) override;
    void DisplayRatline( const SHAPE_LINE_CHAIN& aRatline, int aColor = -1 ) override;
    void Commit() override;
    void AddItem( PNS::ITEM* aItem ) override;
    void UpdateItem( PNS::ITEM* aItem ) override;
    void RemoveItem( PNS::ITEM* aItem ) override;

    void UpdateNet( int aNetCode ) override;

    BOARD_COMMIT* GetCommit() const { return m_commit.get(); }

private:
    struct OFFSET
    {
        VECTOR2I p_old, p_new;
    };

    std::map<PAD*, OFFSET>          m_fpOffsets;
    KIGFX::VIEW*                    m_view;
    KIGFX::VIEW_GROUP*              m_previewItems;
    std::unordered_set<BOARD_ITEM*> m_hiddenItems;

    PCB_TOOL_BASE*                  m_tool;
    std::unique_ptr<BOARD_COMMIT>   m_commit;
    const PCB_DISPLAY_OPTIONS*      m_dispOptions;
};


#endif
