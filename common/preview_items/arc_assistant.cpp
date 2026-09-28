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

#include <preview_items/arc_assistant.h>

#include <preview_items/draw_context.h>
#include <preview_items/preview_utils.h>

#include <gal/graphics_abstraction_layer.h>
#include <view/view.h>

#include <algorithm>

#include <base_units.h>
#include <math/util.h>
#include <trigo.h>

using namespace KIGFX::PREVIEW;

ARC_ASSISTANT::ARC_ASSISTANT( const ARC_GEOM_MANAGER& aManager, const EDA_IU_SCALE& aIuScale,
                              EDA_UNITS aUnits ) :
        EDA_ITEM( NOT_USED ),
        m_constructMan( aManager ),
        m_iuScale( aIuScale ),
        m_units( aUnits ),
        m_drawArc( false )
{
}


const BOX2I ARC_ASSISTANT::ViewBBox() const
{
    BOX2I tmp;

    // no bounding box when no graphic shown
    if( !m_constructMan.HasPreview() )
        return tmp;

    // this is an edit-time artifact; no reason to try and be smart with the bounding box
    // (besides, we can't tell the text extents without a view to know what the scale is)
    tmp.SetMaximum();
    return tmp;
}


void ARC_ASSISTANT::ViewDraw( int aLayer, KIGFX::VIEW* aView ) const
{
    // not in a position to draw anything
    if( !m_constructMan.HasPreview() )
        return;

    aView->GetGAL()->ResetTextAttributes();

    if( m_constructMan.GetMode() == ARC_DRAW_MODE::CENTER_START_END )
        drawCenterGuides( aLayer, aView );
    else
        drawEndpointGuides( aLayer, aView );
}


void ARC_ASSISTANT::drawCenterGuides( int aLayer, KIGFX::VIEW* aView ) const
{
    const VECTOR2I origin = m_constructMan.GetOrigin();

    KIGFX::PREVIEW::DRAW_CONTEXT preview_ctx( *aView );

    // draw first radius line
    bool dimFirstLine = m_constructMan.GetStep() > ARC_GEOM_MANAGER::SECOND_POINT;

    preview_ctx.DrawLineWithAngleHighlight( origin, m_constructMan.GetStartRadiusEnd(),
                                            dimFirstLine );

    wxArrayString cursorStrings;

    if( m_constructMan.GetStep() == ARC_GEOM_MANAGER::SECOND_POINT )
    {
        // haven't started the angle selection phase yet

        EDA_ANGLE initAngle = m_constructMan.GetStartAngle();

        // Draw the radius guide circle if wanted
        if( m_drawArc )
        {
            preview_ctx.DrawCircle( origin, m_constructMan.GetRadius(), true );
        }

        initAngle.Normalize720();

        cursorStrings.push_back(
                DimensionLabel( "r", m_constructMan.GetRadius(), m_iuScale, m_units ) );
        cursorStrings.push_back( DimensionLabel( wxString::FromUTF8( "θ" ),
                                                 initAngle.AsDegrees(), m_iuScale,
                                                 EDA_UNITS::DEGREES ) );
    }
    else
    {
        preview_ctx.DrawLineWithAngleHighlight( origin, m_constructMan.GetEndRadiusEnd(), false );

        EDA_ANGLE start = m_constructMan.GetStartAngle();
        EDA_ANGLE subtended = m_constructMan.GetSubtended();
        EDA_ANGLE normalizedEnd = ( start + subtended ).Normalize180();

        // draw dimmed extender line to cursor
        preview_ctx.DrawLineWithAngleHighlight( origin, m_constructMan.GetLastPoint(), true );

        cursorStrings.push_back( DimensionLabel( wxString::FromUTF8( "Δθ" ), subtended.AsDegrees(),
                                                 m_iuScale, EDA_UNITS::DEGREES ) );
        cursorStrings.push_back( DimensionLabel( wxString::FromUTF8( "θ" ),
                                                 normalizedEnd.AsDegrees(), m_iuScale,
                                                 EDA_UNITS::DEGREES ) );
    }

    // place the text next to cursor, on opposite side from radius
    DrawTextNextToCursor( aView, m_constructMan.GetLastPoint(),
                          origin - m_constructMan.GetLastPoint(), cursorStrings,
                          aLayer == LAYER_SELECT_OVERLAY );
}


void ARC_ASSISTANT::drawEndpointGuides( int aLayer, KIGFX::VIEW* aView ) const
{
    const int index = m_constructMan.GetPointIndex();

    // The first point is only being placed
    if( index == 0 )
        return;

    const ARC_DRAW_MODE         mode = m_constructMan.GetMode();
    const KIGEOM::ARC_SOLUTION& sol = m_constructMan.GetSolution();
    const VECTOR2I              start = m_constructMan.GetStartRadiusEnd();
    const VECTOR2I              cursor = m_constructMan.GetLastPoint();
    const bool chordModes = mode == ARC_DRAW_MODE::START_END_MID || mode == ARC_DRAW_MODE::START_END_CENTER;
    const double pixel = aView->ToWorld( 1.0 );
    const int    dashSize = KiROUND( 12 * pixel );

    KIGFX::PREVIEW::DRAW_CONTEXT preview_ctx( *aView );

    // Long guides, such as the line to a far away center, are cut so the dashes stay bounded
    auto dashedGuide =
            [&]( const VECTOR2I& aFrom, const VECTOR2D& aTo )
            {
                const VECTOR2D delta = aTo - VECTOR2D( aFrom );
                const double   length = delta.EuclideanNorm();
                const double   limit = 400 * pixel;
                const VECTOR2D end = length > limit ? VECTOR2D( aFrom ) + delta * ( limit / length ) : aTo;

                preview_ctx.DrawLineDashed( aFrom, KiROUND( end ), dashSize,
                                            dashSize / 2, false );
            };

    wxArrayString cursorStrings;
    VECTOR2D      labelAnchor( start );

    if( chordModes )
    {
        // The chord is what the first two clicks fixed, and the cursor chases its end until then
        const VECTOR2I chordEnd = index == 1 ? cursor : m_constructMan.GetPoint( 1 );
        const VECTOR2D chordMid = ( VECTOR2D( start ) + VECTOR2D( chordEnd ) ) / 2.0;

        preview_ctx.DrawLineWithAngleHighlight( start, chordEnd, index > 1 );
        labelAnchor = chordMid;

        if( index > 1 && sol.valid )
        {
            const VECTOR2D marked = mode == ARC_DRAW_MODE::START_END_MID ? VECTOR2D( sol.mid ) : sol.center;

            // Bisector guide and a marker on the point derived from the cursor
            dashedGuide( KiROUND( chordMid ), marked );
            preview_ctx.DrawCircle( KiROUND( marked ), 4 * pixel, false );

            if( mode == ARC_DRAW_MODE::START_END_CENTER )
            {
                preview_ctx.DrawLine( KiROUND( marked ), sol.start, true );
                preview_ctx.DrawLine( KiROUND( marked ), sol.end, true );
            }

            cursorStrings.push_back( DimensionLabel( "r", sol.radius, m_iuScale, m_units ) );
            cursorStrings.push_back( DimensionLabel( "s", ( VECTOR2D( sol.mid ) - chordMid ).EuclideanNorm(),
                                                     m_iuScale, m_units ) );
        }
    }
    else
    {
        // The tangent ray is at least as long as the distance to the cursor so it reads as a direction
        const VECTOR2D dir = m_constructMan.GetTangentDirection();
        const double   length = dir.EuclideanNorm();

        if( length > 0.0 )
            dashedGuide( start, VECTOR2D( start ) + dir * ( std::max( length, 100 * pixel ) / length ) );

        if( index == 1 )
        {
            preview_ctx.DrawLineWithAngleHighlight( start, cursor, true );

            EDA_ANGLE heading( dir );

            cursorStrings.push_back( DimensionLabel( wxString::FromUTF8( "θ" ), heading.AsDegrees(), m_iuScale,
                                                     EDA_UNITS::DEGREES ) );
        }
        else if( sol.valid )
        {
            preview_ctx.DrawLine( start, sol.end, true );
            cursorStrings.push_back( DimensionLabel( "r", sol.radius, m_iuScale, m_units ) );
        }
    }

    if( sol.valid )
    {
        cursorStrings.push_back( DimensionLabel( wxString::FromUTF8( "Δθ" ), m_constructMan.GetSubtended().AsDegrees(),
                                                 m_iuScale, EDA_UNITS::DEGREES ) );
    }

    if( cursorStrings.empty() )
        return;

    // place the text next to cursor, on opposite side from the guides
    DrawTextNextToCursor( aView, cursor, labelAnchor - VECTOR2D( cursor ), cursorStrings,
                          aLayer == LAYER_SELECT_OVERLAY );
}
