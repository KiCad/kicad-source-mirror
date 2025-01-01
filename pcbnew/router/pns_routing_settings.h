/*
 * KiRouter - a push-and-(sometimes-)shove PCB router
 *
 * Copyright (C) 2013-2014 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * @author Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
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

#ifndef __PNS_ROUTING_SETTINGS
#define __PNS_ROUTING_SETTINGS

#include <cstdio>

#include <settings/nested_settings.h>
#include <geometry/direction45.h>

#include "time_limit.h"

class DIRECTION_45;
class TOOL_SETTINGS;

namespace PNS {

///< Routing modes
enum PNS_MODE
{
    RM_MarkObstacles = 0,   ///< Ignore collisions, mark obstacles
    RM_Shove,               ///< Only shove
    RM_Walkaround,          ///< Only walk around
};

///< Optimization effort.
enum PNS_OPTIMIZATION_EFFORT
{
    OE_LOW = 0,
    OE_MEDIUM = 1,
    OE_FULL = 2
};

/**
 * Contain all persistent settings of the router, such as the mode, optimization effort, etc.
 */

class ROUTING_SETTINGS : public NESTED_SETTINGS
{
public:
    ROUTING_SETTINGS( JSON_SETTINGS* aParent, const std::string& aPath );

    ///< Return the routing mode.
    PNS_MODE Mode() const { return m_routingMode; }

    ///< Set the routing mode.
    void SetMode( PNS_MODE aMode ) { m_routingMode = aMode; }

    ///< Return the optimizer effort. Bigger means cleaner traces, but slower routing.
    PNS_OPTIMIZATION_EFFORT OptimizerEffort() const { return m_optimizerEffort; }

    ///< Set the optimizer effort. Bigger means cleaner traces, but slower routing.
    void SetOptimizerEffort( PNS_OPTIMIZATION_EFFORT aEffort ) { m_optimizerEffort = aEffort; }

    ///< Return true if shoving vias is enabled.
    bool ShoveVias() const { return m_shoveVias; }

    ///< Enable/disable shoving vias.
    void SetShoveVias( bool aShoveVias ) { m_shoveVias = aShoveVias; }

    ///< Return true if loop (redundant track) removal is on.
    bool RemoveLoops() const { return m_removeLoops; }

    ///< Enable/disable loop (redundant track) removal.
    void SetRemoveLoops( bool aRemoveLoops ) { m_removeLoops = aRemoveLoops; }

    ///< Return true if suggesting the finish of currently placed track is on.
    bool SuggestFinish() { return m_suggestFinish; }

    ///< Enable displaying suggestions for finishing the currently placed track.
    void SetSuggestFinish( bool aSuggestFinish ) { m_suggestFinish = aSuggestFinish; }

    ///< Return true if Smart Pads (optimized connections) is enabled.
    bool SmartPads() const { return m_smartPads; }

    ///< Enable/disable Smart Pads (optimized connections).
    void SetSmartPads( bool aSmartPads ) { m_smartPads = aSmartPads; }

    ///< Return true if follow mouse mode is active (permanently on for the moment).
    bool FollowMouse() const
    {
        return m_followMouse && !( Mode() == RM_MarkObstacles );
    }

    ///< Return true if smoothing segments during dragging is enabled.
    bool SmoothDraggedSegments() const { return m_smoothDraggedSegments; }

    ///< Enable/disable smoothing segments during dragging.
    void SetSmoothDraggedSegments( bool aSmooth ) { m_smoothDraggedSegments = aSmooth; }

    ///< Return true if jumping over unmovable obstacles is on.
    bool JumpOverObstacles() const { return m_jumpOverObstacles; }
    void SetJumpOverObstacles( bool aJump ) { m_jumpOverObstacles = aJump; }

    void SetStartDiagonal( bool aStartDiagonal ) { m_startDiagonal = aStartDiagonal; }

    bool AllowDRCViolations() const
    {
        return m_routingMode == PNS_MODE::RM_MarkObstacles && m_allowDRCViolations;
    }

    bool GetAllowDRCViolationsSetting() const { return m_allowDRCViolations; }
    void SetAllowDRCViolations( bool aViolate ) { m_allowDRCViolations = aViolate; }

    bool GetFreeAngleMode() const { return m_freeAngleMode; }

    void SetFreeAngleMode( bool aEnable ) { m_freeAngleMode = aEnable; }

    const DIRECTION_45 InitialDirection() const;

    int ShoveIterationLimit() const;
    TIME_LIMIT ShoveTimeLimit() const;

    int WalkaroundIterationLimit() const { return m_walkaroundIterationLimit; };
    TIME_LIMIT WalkaroundTimeLimit() const;

    void SetSnapToTracks( bool aSnap ) { m_snapToTracks = aSnap; }
    void SetSnapToPads( bool aSnap ) { m_snapToPads = aSnap; }

    bool GetSnapToTracks() const { return m_snapToTracks; }
    bool GetSnapToPads() const { return m_snapToPads; }

    DIRECTION_45::CORNER_MODE GetCornerMode() const { return m_cornerMode; }
    void SetCornerMode( DIRECTION_45::CORNER_MODE aMode ) { m_cornerMode = aMode; }

    bool GetOptimizeEntireDraggedTrack() const { return m_optimizeEntireDraggedTrack; }
    void SetOptimizeEntireDraggedTrack( bool aEnable ) { m_optimizeEntireDraggedTrack = aEnable; }

    bool GetAutoPosture() const { return m_autoPosture; }
    void SetAutoPosture( bool aEnable ) { m_autoPosture = aEnable; }

    bool GetFixAllSegments() const { return m_fixAllSegments; }
    void SetFixAllSegments( bool aEnable ) { m_fixAllSegments = aEnable; }

    double WalkaroundHugLengthThreshold() const { return m_walkaroundHugLengthThreshold; }

    int ViaForcePropIterationLimit() const { return m_viaForcePropIterationLimit; }
    void SetViaForcePropIterationLimit(int aLimit) { m_viaForcePropIterationLimit = aLimit; }

private:
    bool m_shoveVias;
    bool m_startDiagonal;
    bool m_removeLoops;
    bool m_smartPads;
    bool m_suggestFinish;
    bool m_followMouse;
    bool m_jumpOverObstacles;
    bool m_smoothDraggedSegments;
    bool m_allowDRCViolations;
    bool m_freeAngleMode;
    bool m_snapToTracks;
    bool m_snapToPads;
    bool m_optimizeEntireDraggedTrack;
    bool m_autoPosture;
    bool m_fixAllSegments;

    DIRECTION_45::CORNER_MODE m_cornerMode;

    PNS_MODE m_routingMode;
    PNS_OPTIMIZATION_EFFORT m_optimizerEffort;

    int m_walkaroundIterationLimit;
    int m_shoveIterationLimit;
    int m_viaForcePropIterationLimit;
    double m_walkaroundHugLengthThreshold;

    TIME_LIMIT m_shoveTimeLimit;
    TIME_LIMIT m_walkaroundTimeLimit;
};

}

#endif
