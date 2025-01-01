/*
 * KiRouter - a push-and-(sometimes-)shove PCB router
 *
 * Copyright (C) 2013-2014 CERN
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

#include <geometry/direction45.h>
#include <settings/parameters.h>

#include "pns_routing_settings.h"

namespace PNS {

const int pnsSchemaVersion = 0;


ROUTING_SETTINGS::ROUTING_SETTINGS( JSON_SETTINGS* aParent, const std::string& aPath ) :
        NESTED_SETTINGS( "pns", pnsSchemaVersion, aParent, aPath )
{
    m_routingMode = RM_Walkaround;
    m_optimizerEffort = OE_MEDIUM;
    m_removeLoops = true;
    m_smartPads = true;
    m_shoveVias = true;
    m_suggestFinish = false;
    m_followMouse = true;
    m_startDiagonal = false;
    m_shoveIterationLimit = 250;
    m_shoveTimeLimit = 1000;
    m_walkaroundIterationLimit = 40;
    m_jumpOverObstacles = false;
    m_smoothDraggedSegments = true;
    m_allowDRCViolations = false;
    m_freeAngleMode = false;
    m_snapToTracks = false;
    m_snapToPads = false;
    m_optimizeEntireDraggedTrack = false;
    m_cornerMode = DIRECTION_45::CORNER_MODE::MITERED_45;
    m_walkaroundHugLengthThreshold = 1.5;
    m_autoPosture = true;
    m_fixAllSegments = true;
    m_viaForcePropIterationLimit = 40;

    m_params.emplace_back( new PARAM<int>( "mode", reinterpret_cast<int*>( &m_routingMode ),
            static_cast<int>( RM_Walkaround ) ) );

    m_params.emplace_back( new PARAM<int>( "effort", reinterpret_cast<int*>( &m_optimizerEffort ),
            static_cast<int>( OE_MEDIUM ) ) );

    m_params.emplace_back( new PARAM<bool>( "remove_loops",     &m_removeLoops,     true ) );
    m_params.emplace_back( new PARAM<bool>( "smart_pads",       &m_smartPads,       true ) );
    m_params.emplace_back( new PARAM<bool>( "shove_vias",       &m_shoveVias,       true ) );
    m_params.emplace_back( new PARAM<bool>( "suggest_finish",   &m_suggestFinish,   false ) );
    m_params.emplace_back( new PARAM<bool>( "follow_mouse",     &m_followMouse,     true ) );
    m_params.emplace_back( new PARAM<bool>( "start_diagonal",   &m_startDiagonal,   false ) );
    m_params.emplace_back( new PARAM<int>( "shove_iteration_limit", &m_shoveIterationLimit, 250 ) );
    m_params.emplace_back( new PARAM<int>( "via_force_prop_iteration_limit", &m_viaForcePropIterationLimit, 40 ) );

    m_params.emplace_back( new PARAM_LAMBDA<int>( "shove_time_limit",
            [this] () -> int
            {
                return m_shoveTimeLimit.Get();
            },
            [this] ( int aVal )
            {
                m_shoveTimeLimit.Set( aVal );
            },
            1000 ) );

    m_params.emplace_back( new PARAM<int>( "walkaround_iteration_limit", &m_walkaroundIterationLimit, 40 ) );
    m_params.emplace_back( new PARAM<bool>( "jump_over_obstacles",       &m_jumpOverObstacles, false ) );

    m_params.emplace_back( new PARAM<bool>( "smooth_dragged_segments",   &m_smoothDraggedSegments, true ) );

    m_params.emplace_back( new PARAM<bool>( "can_violate_drc", &m_allowDRCViolations, false ) );
    m_params.emplace_back( new PARAM<bool>( "free_angle_mode",  &m_freeAngleMode,     false ) );
    m_params.emplace_back( new PARAM<bool>( "snap_to_tracks",   &m_snapToTracks,      false ) );
    m_params.emplace_back( new PARAM<bool>( "snap_to_pads",     &m_snapToPads,        false ) );
    m_params.emplace_back( new PARAM<bool>( "optimize_dragged_track",
                                            &m_optimizeEntireDraggedTrack, false ) );

    m_params.emplace_back( new PARAM<bool>( "auto_posture",     &m_autoPosture,       true ) );
    m_params.emplace_back( new PARAM<bool>( "fix_all_segments", &m_fixAllSegments,    true ) );

    m_params.emplace_back( new PARAM_ENUM<DIRECTION_45::CORNER_MODE>(
            "corner_mode", &m_cornerMode, DIRECTION_45::CORNER_MODE::MITERED_45,
            DIRECTION_45::CORNER_MODE::ROUNDED_90, DIRECTION_45::CORNER_MODE::MITERED_45 ) );

    m_params.emplace_back( new PARAM<double>( "walkaround_hug_length_threshold",     &m_walkaroundHugLengthThreshold,     1.5 ) );

    LoadFromFile();
}


const DIRECTION_45 ROUTING_SETTINGS::InitialDirection() const
{
    if( m_startDiagonal )
        return DIRECTION_45( DIRECTION_45::NE );
    else
        return DIRECTION_45( DIRECTION_45::N );
}


TIME_LIMIT ROUTING_SETTINGS::ShoveTimeLimit() const
{
    return TIME_LIMIT ( m_shoveTimeLimit );
}


int ROUTING_SETTINGS::ShoveIterationLimit() const
{
    return m_shoveIterationLimit;
}

}
