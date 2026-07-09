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

#include "tool/arc_draw_behavior.h"

#include <eda_shape.h>

#include <preview_items/arc_geom_manager.h>


ARC_DRAW_BEHAVIOR::ARC_DRAW_BEHAVIOR( const EDA_IU_SCALE& aIuScale, EDA_UNITS aUnits ) :
        m_iuScale( aIuScale ),
        m_units( aUnits )
{
    Reset();
}


bool ARC_DRAW_BEHAVIOR::IsStarted() const
{
    return !m_manager->IsReset();
}


bool ARC_DRAW_BEHAVIOR::IsComplete() const
{
    return m_manager->IsComplete();
}


bool ARC_DRAW_BEHAVIOR::HasGeometryChanged() const
{
    return m_manager->HasGeometryChanged();
}


void ARC_DRAW_BEHAVIOR::ClearGeometryChanged()
{
    m_manager->ClearGeometryChanged();
}


void ARC_DRAW_BEHAVIOR::Reset()
{
    m_manager = std::make_unique<KIGFX::PREVIEW::ARC_GEOM_MANAGER>();
    m_assistant = std::make_unique<KIGFX::PREVIEW::ARC_ASSISTANT>( *m_manager, m_iuScale, m_units );
}


void ARC_DRAW_BEHAVIOR::AddPoint( const VECTOR2I& aPosition )
{
    m_manager->AddPoint( aPosition, true );
}


void ARC_DRAW_BEHAVIOR::SetCursorPosition( const VECTOR2I& aPosition )
{
    m_manager->AddPoint( aPosition, false );
}


void ARC_DRAW_BEHAVIOR::RemoveLastPoint()
{
    m_manager->RemoveLastPoint();
}


void ARC_DRAW_BEHAVIOR::ToggleClockwise()
{
    m_manager->ToggleClockwise();
}


void ARC_DRAW_BEHAVIOR::SetAngleSnap( bool aSnap )
{
    m_manager->SetAngleSnap( aSnap );
}


VECTOR2I ARC_DRAW_BEHAVIOR::GetCenter() const
{
    return m_manager->GetOrigin();
}


VECTOR2I ARC_DRAW_BEHAVIOR::GetStartPoint() const
{
    return m_manager->GetStartRadiusEnd();
}


VECTOR2I ARC_DRAW_BEHAVIOR::GetEndPoint() const
{
    return m_manager->GetEndRadiusEnd();
}


double ARC_DRAW_BEHAVIOR::GetRadius() const
{
    return m_manager->GetRadius();
}


EDA_ANGLE ARC_DRAW_BEHAVIOR::GetStartAngle() const
{
    return m_manager->GetStartAngle();
}


EDA_ANGLE ARC_DRAW_BEHAVIOR::GetSubtendedAngle() const
{
    return m_manager->GetSubtended();
}


void ARC_DRAW_BEHAVIOR::ApplyToShape( EDA_SHAPE& aShape ) const
{
    const VECTOR2I center = m_manager->GetOrigin();

    aShape.SetCenter( center );

    // ARC_GEOM_MANAGER always treats the first radius line as the "start" and
    // the second as the "end", but EDA_SHAPE stores start/end in geometric
    // order (counter-clockwise around the arc).  Swap when the subtended angle
    // is positive (clockwise) so that start and end follow the arc direction.
    if( m_manager->GetSubtended() < ANGLE_0 )
    {
        aShape.SetStart( m_manager->GetStartRadiusEnd() );
        aShape.SetEnd( m_manager->GetEndRadiusEnd() );
    }
    else
    {
        aShape.SetStart( m_manager->GetEndRadiusEnd() );
        aShape.SetEnd( m_manager->GetStartRadiusEnd() );
    }
}
