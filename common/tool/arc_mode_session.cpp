/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <tool/arc_mode_session.h>

#include <settings/app_settings.h>
#include <tool/actions.h>
#include <tool/shape_draw_behavior.h>
#include <tool/tool_event.h>


ARC_DRAW_MODE StartArcDrawMode( const TOOL_EVENT& aEvent, APP_SETTINGS_BASE* aSettings,
                                SHAPE_DRAW_BEHAVIOR& aBehavior )
{
    ARC_DRAW_MODE mode = aSettings ? aSettings->m_ArcDrawMode : ARC_DRAW_MODE::CENTER_START_END;

    if( std::optional<ARC_DRAW_MODE> requested = aEvent.ParameterIf<ARC_DRAW_MODE>() )
        mode = *requested;

    if( aSettings )
        aSettings->m_ArcDrawMode = mode;

    aBehavior.SetArcDrawMode( mode );
    return mode;
}


ARC_MODE_SESSION::ARC_MODE_SESSION( SHAPE_DRAW_BEHAVIOR& aBehavior, APP_SETTINGS_BASE* aSettings, bool aDrawingArc,
                                    const std::optional<ARC_TANGENT_SEED>& aChainSeed, SEED_FINDER aFindSeed,
                                    std::function<void( ARC_DRAW_MODE )> aOnModeChanged ) :
        m_behavior( aBehavior ),
        m_settings( aSettings ),
        m_drawingArc( aDrawingArc ),
        m_chainSeeded( aChainSeed.has_value() ),
        m_findSeed( std::move( aFindSeed ) ),
        m_onModeChanged( std::move( aOnModeChanged ) )
{
    if( aChainSeed )
        m_behavior.SetTangentSeed( aChainSeed->m_start, aChainSeed->m_direction, aChainSeed->m_directionIsAxis );
}


ARC_DRAW_MODE ARC_MODE_SESSION::Mode() const
{
    return m_settings ? m_settings->m_ArcDrawMode : ARC_DRAW_MODE::CENTER_START_END;
}


void ARC_MODE_SESSION::SetMode( ARC_DRAW_MODE aMode )
{
    if( !m_drawingArc )
        return;

    if( m_settings )
        m_settings->m_ArcDrawMode = aMode;

    m_behavior.SetArcDrawMode( aMode );
    m_chainSeeded = false;
    m_clickSeeded = false;

    if( m_onModeChanged )
        m_onModeChanged( aMode );
}


bool ARC_MODE_SESSION::HandleModeEvent( const TOOL_EVENT& aEvent )
{
    if( !m_drawingArc || aEvent.Category() != TC_COMMAND )
        return false;

    if( std::optional<ARC_DRAW_MODE> mode = aEvent.ParameterIf<ARC_DRAW_MODE>() )
    {
        SetMode( *mode );
        return true;
    }

    if( aEvent.IsAction( &ACTIONS::cycleArcDrawMode ) )
    {
        SetMode( IncrementArcDrawMode( Mode() ) );
        return true;
    }

    return false;
}


void ARC_MODE_SESSION::PlacePoint( const VECTOR2I& aMouse, const VECTOR2I& aPos )
{
    const bool firstPoint = !m_chainSeeded && !m_clickSeeded && m_behavior.GetStep() == 0;

    if( m_drawingArc && firstPoint && m_findSeed && Mode() == ARC_DRAW_MODE::TANGENT )
    {
        if( std::optional<ARC_TANGENT_SEED> seed = m_findSeed( aMouse, aPos ) )
        {
            m_behavior.SetTangentSeed( seed->m_start, seed->m_direction, seed->m_directionIsAxis );
            m_clickSeeded = true;
            return;
        }
    }

    m_behavior.AddPoint( aPos );
    m_clickSeeded = false;
}


void ARC_MODE_SESSION::RemoveLastPoint()
{
    // Reset() drops a seed but keeps the mode
    if( m_clickSeeded )
    {
        m_behavior.Reset();
        m_clickSeeded = false;
    }
    else
    {
        m_behavior.RemoveLastPoint();
    }
}
