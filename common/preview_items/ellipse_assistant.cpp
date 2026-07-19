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

#include <preview_items/ellipse_assistant.h>

#include <preview_items/draw_context.h>
#include <preview_items/preview_utils.h>

#include <gal/graphics_abstraction_layer.h>
#include <view/view.h>

#include <base_units.h>
#include <math/util.h>
#include <trigo.h>

using namespace KIGFX::PREVIEW;

ELLIPSE_ASSISTANT::ELLIPSE_ASSISTANT( const ELLIPSE_GEOM_MANAGER& aManager, const EDA_IU_SCALE& aIuScale,
                                      EDA_UNITS aUnits ) :
        EDA_ITEM( NOT_USED ),
        m_constructMan( aManager ),
        m_iuScale( aIuScale ),
        m_units( aUnits ),
        m_drawArc( false )
{
}


const BOX2I ELLIPSE_ASSISTANT::ViewBBox() const
{
    BOX2I tmp;

    if( m_constructMan.IsReset() )
        return tmp;

    tmp.SetMaximum();
    return tmp;
}


void ELLIPSE_ASSISTANT::ViewDraw( int aLayer, KIGFX::VIEW* aView ) const
{
    KIGFX::GAL& gal = *aView->GetGAL();

    if( m_constructMan.IsReset() )
        return;

    gal.ResetTextAttributes();

    KIGFX::PREVIEW::DRAW_CONTEXT preview_ctx( *aView );

    ELLIPSE_GEOM_MANAGER::ELLIPSE_STEPS step = m_constructMan.GetStep();

    if( step <= ELLIPSE_GEOM_MANAGER::SET_BBOX_C2 )
    {
        // Bbox phase: draw bbox rectangle and fitted ellipse outline.

        const VECTOR2I c1 = m_constructMan.GetBboxCorner1();
        const VECTOR2I c2 = ( step == ELLIPSE_GEOM_MANAGER::SET_BBOX_C1 )
                                ? m_constructMan.GetLastPoint()
                                : m_constructMan.GetBboxCorner2();

        // Draw bbox
        preview_ctx.DrawRectangle( c1, c2, false );

        // Draw fitted ellipse if we have valid radii
        if( step >= ELLIPSE_GEOM_MANAGER::SET_BBOX_C2 )
        {
            const ELLIPSE<int> ellipse = m_constructMan.GetEllipse();

            if( m_drawArc )
            {
                preview_ctx.DrawEllipse( ellipse, false );
            }

            // Draw major axis
            {
                VECTOR2I endPoint = ellipse.GetPointAtAngle( ANGLE_0 );
                preview_ctx.DrawLine( ellipse.Center, endPoint, true );
            }

            // Draw minor axis
            {
                VECTOR2I endPoint = ellipse.GetPointAtAngle( ANGLE_90 );
                preview_ctx.DrawLine( ellipse.Center, endPoint, true );
            }
        }
    }
    else
    {
        // Angle phase: draw ellipse, radial lines at angles, and arc segment.
        const ELLIPSE<int> ellipse = m_constructMan.GetEllipse();

        // Draw full ellipse outline (de-emphasised)
        if( m_drawArc )
        {
            preview_ctx.DrawEllipse( ellipse, true );
        }

        const VECTOR2I startPt = ellipse.GetArcStartPoint();
        const VECTOR2I endPt = ellipse.GetArcEndPoint();
        const VECTOR2I cursorPt = m_constructMan.GetLastPoint();

        // Draw radial lines from center to start/end
        preview_ctx.DrawLineWithAngleHighlight( ellipse.Center, startPt, false );
        preview_ctx.DrawLineWithAngleHighlight( ellipse.Center, endPt, false );

        // Draw dimmed extender line to cursor
        preview_ctx.DrawLineWithAngleHighlight( ellipse.Center, cursorPt, true );

        // Dimension labels
        wxArrayString cursorStrings;

        cursorStrings.push_back( DimensionLabel( wxString::FromUTF8( "Δθ" ), ellipse.GetSubtendedAngle().AsDegrees(),
                                                 m_iuScale, EDA_UNITS::DEGREES ) );

        EDA_ANGLE normalizedEnd = ellipse.EndAngle;
        normalizedEnd.Normalize180();
        cursorStrings.push_back(
                DimensionLabel( wxString::FromUTF8( "θ" ), normalizedEnd.AsDegrees(), m_iuScale, EDA_UNITS::DEGREES ) );

        DrawTextNextToCursor( aView, m_constructMan.GetLastPoint(), ellipse.Center - cursorPt, cursorStrings,
                              aLayer == LAYER_SELECT_OVERLAY );
    }
}
