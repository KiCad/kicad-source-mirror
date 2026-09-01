/*
 * KiRouter - a push-and-(sometimes-)shove PCB router
 *
 * Copyright (C) 2013-2017 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 * Author: Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
 * Author: Maciej Suminski <maciej.suminski@cern.ch>
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

#ifndef __ROUTER_TOOL_H
#define __ROUTER_TOOL_H

#include "pns_tool_base.h"
#include <board_design_settings.h>

class PCB_SELECTION_TOOL;
class PCB_VIA;


// A stacked drop armed during a route, expanded into a stack when the route finishes.
struct PENDING_STACK_EXPANSION
{
    PCB_LAYER_ID     m_Start;
    PCB_LAYER_ID     m_End;
    int              m_Net;
    VIA_STACK_PRESET m_Preset;
};


/**
 * Preset a route's stacked drop wants for @a aVia, or nullptr when the via is not one of
 * them. Vias in @a aPreRoute already existed, so the route did not create them.
 */
const VIA_STACK_PRESET* MatchPendingStackExpansion( PCB_VIA* aVia, const std::set<KIID>& aPreRoute,
                                                    const std::vector<PENDING_STACK_EXPANSION>& aPending );


/**
 * Layer a microvia stack drop lands on when invoked on @a aCurrent. A preset places only from
 * its own terminals: the start layer runs the span, the end layer runs it back.
 * UNDEFINED_LAYER anywhere else.
 */
PCB_LAYER_ID ViaStackTargetLayer( PCB_LAYER_ID aStart, PCB_LAYER_ID aEnd, PCB_LAYER_ID aCurrent );


class ROUTER_TOOL : public PNS::TOOL_BASE
{
public:
    ROUTER_TOOL();
    ~ROUTER_TOOL();

    bool Init() override;
    void Reset( RESET_REASON aReason ) override;

    int MainLoop( const TOOL_EVENT& aEvent );
    int RouteSelected( const TOOL_EVENT& aEvent );
    int OptimizeSelected( const TOOL_EVENT& aEvent );

    int InlineBreakTrack( const TOOL_EVENT& aEvent );
    bool CanInlineDrag( int aDragMode );
    int InlineDrag( const TOOL_EVENT& aEvent );

    int SelectCopperLayerPair( const TOOL_EVENT& aEvent );
    int DpDimensionsDialog( const TOOL_EVENT& aEvent );
    int SettingsDialog( const TOOL_EVENT& aEvent );
    int ChangeRouterMode( const TOOL_EVENT& aEvent );
    int CycleRouterMode( const TOOL_EVENT& aEvent );
    int CustomTrackWidthDialog( const TOOL_EVENT& aEvent );

    PNS::PNS_MODE GetRouterMode();

    /**
     * Layer the next route must start on, used by the microvia stack handoff. One shot, so
     * only the primed resume is affected and a normal route start still follows the snap.
     */
    void SetViaStackResumeLayer( PCB_LAYER_ID aLayer ) { m_viaStackResumeLayer = aLayer; }

    /**
     * @brief Returns whether routing is currently active.
     *
     * @return True if actively routing, false if not routing or
     *         tool is activated and idle.
     */
    bool RoutingInProgress();

    void setTransitions() override;

    // A filter for narrowing a collection representing a simple corner
    // or a non-fanout-via to a single PCB_TRACK item.
    static void NeighboringSegmentFilter( const VECTOR2I& aPt, GENERAL_COLLECTOR& aCollector,
                                          PCB_SELECTION_TOOL* aSelTool );

    void UpdateMessagePanel();

private:
    void performRouting( VECTOR2D aStartPosition );
    void performDragging( int aMode = PNS::DM_ANY );
    void breakTrack();
    void restoreSelection( const PCB_SELECTION& aOriginalSelection );

    void handleCommonEvents( TOOL_EVENT& evt );
    int handleLayerSwitch( const TOOL_EVENT& aEvent, bool aForceVia );
    int handlePnSCornerModeChange( const TOOL_EVENT& aEvent );

    // Returns the board layer ID for the start layer of the router
    PCB_LAYER_ID getStartLayer( const PNS::ITEM* aItem );

    void switchLayerOnViaPlacement();
    void updateSizesAfterRouterEvent( int targetLayer, const VECTOR2I& aPos );

    int onLayerCommand( const TOOL_EVENT& aEvent );
    int onViaCommand( const TOOL_EVENT& aEvent );
    int onViaStackCommand( const TOOL_EVENT& aEvent );

    /// Build and commit a staggered via stack queued during routing (after the PNS world is gone).
    void commitPendingViaStack();
    int onTrackViaSizeChanged( const TOOL_EVENT& aEvent );

    bool prepareInteractive( VECTOR2D aStartPosition );
    bool finishInteractive();
    void saveRouterDebugLog();

private:
    std::shared_ptr<ACTION_MENU> m_diffPairMenu;
    std::shared_ptr<ACTION_MENU> m_trackViaMenu;

    // These are in board layer ID format and must be converted to PNS layer ID format
    // when used with the PNS interface.
    PCB_LAYER_ID                 m_lastTargetLayer;
    PCB_LAYER_ID                 m_originalActiveLayer;
    PCB_LAYER_ID                 m_viaStackResumeLayer;

    bool                         m_inRouterTool;         // Re-entrancy guard
    bool                         m_inRouteSelected;

    bool                         m_startWithVia;         // User pressed V before routing started

    // A staggered via stack queued mid-route, committed after the PNS world is torn down.
    bool         m_pendingViaStack = false;
    VECTOR2I     m_pendingStackHead;
    PCB_LAYER_ID m_pendingStackStart = UNDEFINED_LAYER;
    PCB_LAYER_ID m_pendingStackEnd = UNDEFINED_LAYER;

    // Stacked drops armed during this route, expanded into stacks when the route finishes.
    std::vector<PENDING_STACK_EXPANSION> m_pendingStackedExpansions;

    // Vias already expandable when the first drop was armed; the route did not create these.
    std::set<KIID> m_preRouteExpandableVias;
};

#endif
