/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 Kicad Developers, see AUTHORS.txt for contributors.
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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <preview_items/arc_assistant.h>

#include <preview_items/draw_context.h>
#include <preview_items/preview_utils.h>

#include <gal/graphics_abstraction_layer.h>
#include <view/view.h>

#include <common.h>
#include <base_units.h>

using namespace KIGFX::PREVIEW;

ARC_ASSISTANT::ARC_ASSISTANT( const ARC_GEOM_MANAGER& aManager, EDA_UNITS_T aUnits ) :
    EDA_ITEM( NOT_USED ),
    m_constructMan( aManager ),
    m_units( aUnits )
{
}


const BOX2I ARC_ASSISTANT::ViewBBox() const
{
    BOX2I tmp;

    // no bounding box when no graphic shown
    if( m_constructMan.IsReset() )
        return tmp;

    // just enclose the whle circular area
    auto     origin  = m_constructMan.GetOrigin();
    auto     radius  = m_constructMan.GetRadius();
    VECTOR2D rVec( radius, radius );

    tmp.SetOrigin( origin + rVec );
    tmp.SetEnd( origin - rVec );
    tmp.Normalize();
    return tmp;
}


/**
 * Get deci-degrees from radians, normalised to +/- 360.
 *
 * The normalisation is such that a negative angle will stay
 * negative.
 */
double getNormDeciDegFromRad( double aRadians )
{
    double degs = RAD2DECIDEG( aRadians );

    // normalise to +/- 360
    while( degs < -3600.0 )
        degs += 3600.0;

    while( degs > 3600.0 )
        degs -= 3600.0;

    return degs;
}


void ARC_ASSISTANT::ViewDraw( int aLayer, KIGFX::VIEW* aView ) const
{
    auto& gal = *aView->GetGAL();

    // not in a position to draw anything
    if( m_constructMan.IsReset() )
        return;

    gal.ResetTextAttributes();

    // constant text size on screen
    SetConstantGlyphHeight( gal, 12.0 );

    // angle reference arc size
    const double innerRad = 12.0 / gal.GetWorldScale();

    const auto origin = m_constructMan.GetOrigin();

    KIGFX::PREVIEW::DRAW_CONTEXT preview_ctx( *aView );

    // draw first radius line
    bool dimFirstLine = m_constructMan.GetStep() > ARC_GEOM_MANAGER::SET_START;

    preview_ctx.DrawLineWithAngleHighlight(
            origin, m_constructMan.GetStartRadiusEnd(), dimFirstLine );

    std::vector<wxString> cursorStrings;

    if( m_constructMan.GetStep() == ARC_GEOM_MANAGER::SET_START )
    {
        // haven't started the angle selection phase yet

        double initAngle = m_constructMan.GetStartAngle();

        const auto angleRefLineEnd = m_constructMan.GetOrigin() + VECTOR2D( innerRad * 1.5, 0.0 );

        // draw the short reference baseline
        preview_ctx.DrawLine( origin, angleRefLineEnd, false );

        // draw the angle reference arc
        preview_ctx.DrawArcWithAngleHighlight( origin, innerRad, initAngle, 0.0 );

        // draw the radius guide circle
        preview_ctx.DrawCircle( origin, m_constructMan.GetRadius(), true );

        double degs = getNormDeciDegFromRad( initAngle );

        cursorStrings.push_back( DimensionLabel( "r", m_constructMan.GetRadius(), m_units ) );
        cursorStrings.push_back( DimensionLabel( wxString::FromUTF8( "θ" ), degs, DEGREES ) );
    }
    else
    {
        preview_ctx.DrawLineWithAngleHighlight( origin, m_constructMan.GetEndRadiusEnd(), false );

        auto    start = m_constructMan.GetStartAngle();
        auto    subtended = m_constructMan.GetSubtended();

        preview_ctx.DrawArcWithAngleHighlight( origin, innerRad, start, start + subtended );

        double  subtendedDeg    = getNormDeciDegFromRad( subtended );
        double  endAngleDeg     = getNormDeciDegFromRad( start + subtended );

        // draw dimmed extender line to cursor
        preview_ctx.DrawLineWithAngleHighlight( origin, m_constructMan.GetLastPoint(), true );

        cursorStrings.push_back( DimensionLabel( wxString::FromUTF8( "Δθ" ), subtendedDeg, DEGREES ) );
        cursorStrings.push_back( DimensionLabel( wxString::FromUTF8( "θ" ), endAngleDeg, DEGREES ) );
    }

    // place the text next to cursor, on opposite side from radius
    DrawTextNextToCursor( aView, m_constructMan.GetLastPoint(),
            origin - m_constructMan.GetLastPoint(),
            cursorStrings );
}
