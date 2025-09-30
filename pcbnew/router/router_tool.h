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
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __ROUTER_TOOL_H
#define __ROUTER_TOOL_H

#include "pns_tool_base.h"

class PCB_SELECTION_TOOL;

class ROUTER_TOOL : public PNS::TOOL_BASE
{
public:
    ROUTER_TOOL();
    ~ROUTER_TOOL();

    bool Init() override;
    void Reset( RESET_REASON aReason ) override;

    int MainLoop( const TOOL_EVENT& aEvent );
    int RouteSelected( const TOOL_EVENT& aEvent );

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
    int onTrackViaSizeChanged( const TOOL_EVENT& aEvent );

    bool prepareInteractive( VECTOR2D aStartPosition );
    bool finishInteractive();
    void saveRouterDebugLog();

private:
    std::shared_ptr<ACTION_MENU> m_diffPairMenu;
    std::shared_ptr<ACTION_MENU> m_trackViaMenu;

    // Both of these are in board layer ID format and must be converted to PNS layer ID format
    // when used with the PNS interface.
    PCB_LAYER_ID                 m_lastTargetLayer;
    PCB_LAYER_ID                 m_originalActiveLayer;

    bool                         m_inRouterTool;         // Re-entrancy guard
};

#endif
