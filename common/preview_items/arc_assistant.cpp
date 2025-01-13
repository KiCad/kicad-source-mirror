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

#include <base_units.h>
#include <trigo.h>

using namespace KIGFX::PREVIEW;

ARC_ASSISTANT::ARC_ASSISTANT( const ARC_GEOM_MANAGER& aManager, const EDA_IU_SCALE& aIuScale,
                              EDA_UNITS aUnits ) :
        EDA_ITEM( NOT_USED ),
        m_constructMan( aManager ),
        m_iuScale( aIuScale ),
        m_units( aUnits )
{
}


const BOX2I ARC_ASSISTANT::ViewBBox() const
{
    BOX2I tmp;

    // no bounding box when no graphic shown
    if( m_constructMan.IsReset() )
        return tmp;

    // this is an edit-time artifact; no reason to try and be smart with the bounding box
    // (besides, we can't tell the text extents without a view to know what the scale is)
    tmp.SetMaximum();
    return tmp;
}


void ARC_ASSISTANT::ViewDraw( int aLayer, KIGFX::VIEW* aView ) const
{
    KIGFX::GAL& gal = *aView->GetGAL();

    // not in a position to draw anything
    if( m_constructMan.IsReset() )
        return;

    gal.ResetTextAttributes();

    const VECTOR2I origin = m_constructMan.GetOrigin();

    KIGFX::PREVIEW::DRAW_CONTEXT preview_ctx( *aView );

    // draw first radius line
    bool dimFirstLine = m_constructMan.GetStep() > ARC_GEOM_MANAGER::SET_START;

    preview_ctx.DrawLineWithAngleHighlight( origin, m_constructMan.GetStartRadiusEnd(),
                                            dimFirstLine );

    wxArrayString cursorStrings;

    if( m_constructMan.GetStep() == ARC_GEOM_MANAGER::SET_START )
    {
        // haven't started the angle selection phase yet

        EDA_ANGLE initAngle = m_constructMan.GetStartAngle();

        // draw the radius guide circle
        preview_ctx.DrawCircle( origin, m_constructMan.GetRadius(), true );

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
