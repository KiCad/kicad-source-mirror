/*
 * KiRouter - a push-and-(sometimes-)shove PCB router
 *
 * Copyright (C) 2013-2014 CERN
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

#include <tool/tool_settings.h>

#include "pns_routing_settings.h"
#include "direction.h"

PNS_ROUTING_SETTINGS::PNS_ROUTING_SETTINGS()
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
    m_canViolateDRC = false;
    m_freeAngleMode = false;
    m_inlineDragEnabled = false;
}

void PNS_ROUTING_SETTINGS::Save ( TOOL_SETTINGS& aSettings ) const
{
    aSettings.Set( "Mode", (int)m_routingMode );
    aSettings.Set( "OptimizerEffort", (int) m_optimizerEffort );
    aSettings.Set( "RemoveLoops", m_removeLoops );
    aSettings.Set( "SmartPads", m_smartPads );
    aSettings.Set( "ShoveVias", m_shoveVias );
    aSettings.Set( "StartDiagonal", m_startDiagonal );
    aSettings.Set( "ShoveTimeLimit", m_shoveTimeLimit.Get() );
    aSettings.Set( "ShoveIterationLimit", m_shoveIterationLimit );
    aSettings.Set( "WalkaroundIterationLimit", m_walkaroundIterationLimit );
    aSettings.Set( "JumpOverObstacles", m_jumpOverObstacles );
    aSettings.Set( "SmoothDraggedSegments", m_smoothDraggedSegments );
    aSettings.Set( "CanViolateDRC", m_canViolateDRC );
    aSettings.Set( "SuggestFinish", m_suggestFinish );
    aSettings.Set( "FreeAngleMode", m_freeAngleMode );
    aSettings.Set( "InlineDragEnabled", m_inlineDragEnabled );
}

void PNS_ROUTING_SETTINGS::Load ( const TOOL_SETTINGS& aSettings )
{
    m_routingMode = (PNS_MODE) aSettings.Get( "Mode", (int) RM_Walkaround );
    m_optimizerEffort = (PNS_OPTIMIZATION_EFFORT) aSettings.Get( "OptimizerEffort", (int) OE_MEDIUM );
    m_removeLoops = aSettings.Get( "RemoveLoops", true );
    m_smartPads = aSettings.Get( "SmartPads", true );
    m_shoveVias = aSettings.Get( "ShoveVias", true );
    m_startDiagonal = aSettings.Get( "StartDiagonal", false );
    m_shoveTimeLimit.Set( aSettings.Get( "ShoveTimeLimit", 1000 ) );
    m_shoveIterationLimit = aSettings.Get( "ShoveIterationLimit", 250 );
    m_walkaroundIterationLimit = aSettings.Get( "WalkaroundIterationLimit", 50 );
    m_jumpOverObstacles = aSettings.Get( "JumpOverObstacles", false  );
    m_smoothDraggedSegments = aSettings.Get( "SmoothDraggedSegments", true );
    m_canViolateDRC = aSettings.Get( "CanViolateDRC", false );
    m_suggestFinish = aSettings.Get( "SuggestFinish", false );
    m_freeAngleMode = aSettings.Get( "FreeAngleMode", false );
    m_inlineDragEnabled = aSettings.Get( "InlineDragEnabled", false );
}

const DIRECTION_45 PNS_ROUTING_SETTINGS::InitialDirection() const
{
    if( m_startDiagonal )
        return DIRECTION_45( DIRECTION_45::NE );
    else
        return DIRECTION_45( DIRECTION_45::N );
}


TIME_LIMIT PNS_ROUTING_SETTINGS::ShoveTimeLimit() const
{
    return TIME_LIMIT ( m_shoveTimeLimit );
}


int PNS_ROUTING_SETTINGS::ShoveIterationLimit() const
{
    return m_shoveIterationLimit;
}
