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

#include <preview_items/preview_utils.h>
#include <gal/graphics_abstraction_layer.h>
#include <view/view.h>

#include <common.h>
#include <base_units.h>

using namespace KIGFX::PREVIEW;

ARC_ASSISTANT::ARC_ASSISTANT( const ARC_GEOM_MANAGER& aManager ) :
    EDA_ITEM( NOT_USED ),
    m_constructMan( aManager )
{
}


const BOX2I ARC_ASSISTANT::ViewBBox() const
{
    BOX2I tmp;

    // no bounding box when no graphic shown
    if( m_constructMan.IsReset() )
        return tmp;

    // just enclose the whle circular area
    auto    origin  = m_constructMan.GetOrigin();
    auto    radius  = m_constructMan.GetRadius();
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


static const double ANGLE_EPSILON = 1e-9;

double angleIsSpecial( double aRadians )
{
    return std::fabs( std::remainder( aRadians, M_PI_4 ) ) < ANGLE_EPSILON;
}


static void drawLineWithHilight( KIGFX::GAL& aGal,
        const VECTOR2I& aStart, const VECTOR2I& aEnd, bool aDim )
{
    const auto vec = aEnd - aStart;
    COLOR4D strokeColor = PreviewOverlayDefaultColor();

    if( angleIsSpecial( vec.Angle() ) )
        strokeColor = PreviewOverlaySpecialAngleColor();

    aGal.SetStrokeColor( strokeColor.WithAlpha( PreviewOverlayDeemphAlpha( aDim ) ) );
    aGal.DrawLine( aStart, aEnd );
}


static void drawArcWithHilight( KIGFX::GAL& aGal,
        const VECTOR2I& aOrigin, double aRad, double aStartAngle,
        double aEndAngle )
{
    COLOR4D color = PreviewOverlayDefaultColor();

    if( angleIsSpecial( aStartAngle - aEndAngle ) )
        color = PreviewOverlaySpecialAngleColor();

    aGal.SetStrokeColor( color );
    aGal.SetFillColor( color.WithAlpha( 0.2 ) );

    // draw the angle reference arc
    aGal.DrawArc( aOrigin, aRad, -aStartAngle, -aEndAngle );
}


void ARC_ASSISTANT::ViewDraw( int aLayer, KIGFX::VIEW* aView ) const
{
    auto& gal = *aView->GetGAL();

    // not in a position to draw anything
    if( m_constructMan.IsReset() )
        return;

    gal.SetLineWidth( 1.0 );
    gal.SetIsStroke( true );
    gal.SetIsFill( true );

    gal.ResetTextAttributes();

    // constant text size on screen
    SetConstantGlyphHeight( gal, 12.0 );

    // angle reference arc size
    const double innerRad = 12.0 / gal.GetWorldScale();

    const auto origin = m_constructMan.GetOrigin();

    // draw first radius line
    bool dimFirstLine = m_constructMan.GetStep() > ARC_GEOM_MANAGER::SET_START;

    drawLineWithHilight( gal, origin, m_constructMan.GetStartRadiusEnd(), dimFirstLine );

    std::vector<wxString> cursorStrings;

    if( m_constructMan.GetStep() == ARC_GEOM_MANAGER::SET_START )
    {
        // haven't started the angle selection phase yet

        auto initAngle = m_constructMan.GetStartAngle();

        const auto angleRefLineEnd = m_constructMan.GetOrigin() + VECTOR2D( innerRad * 1.5, 0.0 );

        gal.SetStrokeColor( PreviewOverlayDefaultColor() );
        gal.DrawLine( origin, angleRefLineEnd );

        // draw the angle reference arc
        drawArcWithHilight( gal, origin, innerRad, initAngle, 0.0 );

        double degs = getNormDeciDegFromRad( initAngle );

        cursorStrings.push_back( DimensionLabel( "r", m_constructMan.GetRadius(), g_UserUnit ) );
        cursorStrings.push_back( DimensionLabel( "θ", degs, DEGREES ) );
    }
    else
    {
        drawLineWithHilight( gal, origin, m_constructMan.GetEndRadiusEnd(), false );

        auto    start = m_constructMan.GetStartAngle();
        auto    subtended = m_constructMan.GetSubtended();

        drawArcWithHilight( gal, origin, innerRad, start, start + subtended );

        double  subtendedDeg    = getNormDeciDegFromRad( subtended );
        double  endAngleDeg     = getNormDeciDegFromRad( start + subtended );

        // draw dimmed extender line to cursor
        drawLineWithHilight( gal, origin, m_constructMan.GetLastPoint(), true );

        cursorStrings.push_back( DimensionLabel( "Δθ", subtendedDeg, DEGREES ) );
        cursorStrings.push_back( DimensionLabel( "θ", endAngleDeg, DEGREES ) );
    }

    // FIXME: spaces choke OpenGL lp:1668455
    for( auto& str : cursorStrings )
    {
        str.erase( std::remove( str.begin(), str.end(), ' ' ),
                str.end() );
    }

    // place the text next to cursor, on opposite side from radius
    DrawTextNextToCursor( gal, m_constructMan.GetLastPoint(),
            origin - m_constructMan.GetLastPoint(),
            cursorStrings );
}
