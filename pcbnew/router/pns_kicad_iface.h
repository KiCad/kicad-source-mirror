/*
 * KiRouter - a push-and-(sometimes-)shove PCB router
 *
 * Copyright (C) 2013-2016 CERN
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

#ifndef __PNS_KICAD_IFACE_H
#define __PNS_KICAD_IFACE_H

#include <unordered_set>
#include <unordered_map>
#include <vector>

#include "pns_router.h"

class PNS_PCBNEW_RULE_RESOLVER;
class PNS_PCBNEW_DEBUG_DECORATOR;

class BOARD;
class BOARD_COMMIT;
class PCB_TEXT;
class PCB_DISPLAY_OPTIONS;
class PCB_TOOL_BASE;
class FOOTPRINT;
class PCB_BARCODE;
class PAD;
class EDA_TEXT;
class LENGTH_DELAY_CALCULATION_ITEM;
class BOARD_ITEM;
class EDA_GROUP;

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
    ~PNS_KICAD_IFACE_BASE() override;

    void EraseView() override {};
    void SetBoard( BOARD* aBoard );
    void SyncWorld( PNS::NODE* aWorld ) override;
    bool IsAnyLayerVisible( const PNS_LAYER_RANGE& aLayer ) const override { return true; };
    bool IsFlashedOnLayer( const PNS::ITEM* aItem, int aLayer ) const override;
    bool IsFlashedOnLayer( const PNS::ITEM* aItem, const PNS_LAYER_RANGE& aLayer ) const override;
    bool IsItemVisible( const PNS::ITEM* aItem ) const override { return true; };
    bool IsPNSCopperLayer( int aPNSLayer ) const override;
    bool IsKicadCopperLayer( PCB_LAYER_ID aPcbnewLayer ) const;
    void HideItem( PNS::ITEM* aItem ) override {}
    void DisplayItem( const PNS::ITEM* aItem, int aClearance, bool aEdit = false,
                      int aFlags = 0 ) override {}
    void DisplayPathLine( const SHAPE_LINE_CHAIN& aLine, int aImportance ) override {}
    void DisplayRatline( const SHAPE_LINE_CHAIN& aRatline, PNS::NET_HANDLE aNet ) override {}
    void AddItem( PNS::ITEM* aItem ) override;
    void UpdateItem( PNS::ITEM* aItem ) override;
    void RemoveItem( PNS::ITEM* aItem ) override;
    void Commit() override {}
    bool ImportSizes( PNS::SIZES_SETTINGS& aSizes, PNS::ITEM* aStartItem, PNS::NET_HANDLE aNet,
                      VECTOR2D aStartPosition ) override;
    int StackupHeight( int aFirstLayer, int aSecondLayer ) const override;

    int GetNetCode( PNS::NET_HANDLE aNet ) const override { return -1; }
    wxString GetNetName( PNS::NET_HANDLE aNet ) const override { return wxEmptyString; }
    void UpdateNet( PNS::NET_HANDLE aNet ) override {}
    PNS::NET_HANDLE GetOrphanedNetHandle() override;

    void SetDebugDecorator( PNS::DEBUG_DECORATOR* aDec );

    long long int CalculateRoutedPathLength( const PNS::ITEM_SET& aLine, const PNS::SOLID* aStartPad,
                                             const PNS::SOLID* aEndPad, const NETCLASS* aNetClass ) override;
    int64_t       CalculateRoutedPathDelay( const PNS::ITEM_SET& aLine, const PNS::SOLID* aStartPad,
                                            const PNS::SOLID* aEndPad, const NETCLASS* aNetClass ) override;
    int64_t       CalculateLengthForDelay( int64_t aDesiredDelay, int aWidth, bool aIsDiffPairCoupled,
                                           int aDiffPairCouplingGap, int aPNSLayer, const NETCLASS* aNetClass ) override;
    int64_t       CalculateDelayForShapeLineChain( const SHAPE_LINE_CHAIN& aShape, int aWidth, bool aIsDiffPairCoupled,
                                                   int aDiffPairCouplingGap, int aPNSLayer,
                                                   const NETCLASS* aNetClass ) override;

    PCB_LAYER_ID GetBoardLayerFromPNSLayer( int aLayer ) const override;
    int GetPNSLayerFromBoardLayer( PCB_LAYER_ID aLayer ) const override;

    void SetStartLayerFromPCBNew( PCB_LAYER_ID aLayer );
    void SetStartLayerFromPNS( int aLayer ) { m_startLayer = aLayer; }

    PNS_LAYER_RANGE SetLayersFromPCBNew( PCB_LAYER_ID aStartLayer, PCB_LAYER_ID aEndLayer );

    virtual PNS::NODE* GetWorld() const override { return m_world; };

    BOARD* GetBoard() const { return m_board; }

    virtual EDA_UNITS GetUnits() const { return EDA_UNITS::MM; };

    PNS::RULE_RESOLVER* GetRuleResolver() override;
    PNS::DEBUG_DECORATOR* GetDebugDecorator() override;

protected:
    PNS_PCBNEW_RULE_RESOLVER* m_ruleResolver;
    PNS::DEBUG_DECORATOR* m_debugDecorator;

    std::vector<std::unique_ptr<PNS::SOLID>> syncPad( PAD* aPad );
    std::unique_ptr<PNS::SEGMENT> syncTrack( PCB_TRACK* aTrack );
    std::unique_ptr<PNS::ARC>     syncArc( PCB_ARC* aArc );
    std::unique_ptr<PNS::VIA>     syncVia( PCB_VIA* aVia );
    bool syncTextItem( PNS::NODE* aWorld, BOARD_ITEM* aItem, PCB_LAYER_ID aLayer );
    bool syncGraphicalItem( PNS::NODE* aWorld, PCB_SHAPE* aItem );
    bool syncZone( PNS::NODE* aWorld, ZONE* aZone, SHAPE_POLY_SET* aBoardOutline );
    bool syncBarcode( PNS::NODE* aWorld, PCB_BARCODE* aBarcode );
    bool inheritTrackWidth( PNS::ITEM* aItem, int* aInheritedWidth );
    std::vector<LENGTH_DELAY_CALCULATION_ITEM> getLengthDelayCalculationItems( const PNS::ITEM_SET& aLine,
                                                                               const NETCLASS*      aNetClass ) const;

protected:
    PNS::NODE* m_world;
    BOARD*     m_board;
    int        m_startLayer; // The starting layer, in PNS layer coordinates
};

class PNS_KICAD_IFACE : public PNS_KICAD_IFACE_BASE
{
public:
    PNS_KICAD_IFACE();
    ~PNS_KICAD_IFACE() override;

    virtual void SetHostTool( PCB_TOOL_BASE* aTool );

    void SetView( KIGFX::VIEW* aView );
    void EraseView() override;
    bool IsAnyLayerVisible( const PNS_LAYER_RANGE& aLayer ) const override;
    bool IsItemVisible( const PNS::ITEM* aItem ) const override;
    void HideItem( PNS::ITEM* aItem ) override;
    void DisplayItem( const PNS::ITEM* aItem, int aClearance, bool aEdit = false,
                      int aFlags = 0 ) override;
    void DisplayPathLine( const SHAPE_LINE_CHAIN& aLine, int aImportance ) override;
    void DisplayRatline( const SHAPE_LINE_CHAIN& aRatline, PNS::NET_HANDLE aNet ) override;
    void Commit() override;
    void AddItem( PNS::ITEM* aItem ) override;
    void UpdateItem( PNS::ITEM* aItem ) override;
    void RemoveItem( PNS::ITEM* aItem ) override;

    int GetNetCode( PNS::NET_HANDLE aNet ) const override;
    wxString GetNetName( PNS::NET_HANDLE aNet ) const override;
    void UpdateNet( PNS::NET_HANDLE aNet ) override;

    EDA_UNITS GetUnits() const override;

    void SetCommitFlags( int aCommitFlags ) { m_commitFlags = aCommitFlags; }

protected:
    BOARD_CONNECTED_ITEM* createBoardItem( PNS::ITEM* aItem );
    void                  modifyBoardItem( PNS::ITEM* aItem );

    struct OFFSET
    {
        VECTOR2I p_old, p_new;
    };

    std::map<PAD*, OFFSET>          m_fpOffsets;
    KIGFX::VIEW*                    m_view;
    KIGFX::VIEW_GROUP*              m_previewItems;
    std::unordered_set<BOARD_ITEM*> m_hiddenItems;

    std::unordered_map<BOARD_ITEM*, EDA_GROUP*>               m_itemGroups;
    std::unordered_map<BOARD_ITEM*, std::vector<BOARD_ITEM*>> m_replacementMap;

    PCB_TOOL_BASE*                  m_tool;
    std::unique_ptr<BOARD_COMMIT>   m_commit;
    int                             m_commitFlags;
};


#endif
