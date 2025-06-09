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

#include "preview_items/bezier_assistant.h"

#include <preview_items/draw_context.h>
#include <preview_items/preview_utils.h>

#include <gal/graphics_abstraction_layer.h>
#include <view/view.h>

#include <base_units.h>
#include <trigo.h>

using namespace KIGFX::PREVIEW;

BEZIER_ASSISTANT::BEZIER_ASSISTANT( const BEZIER_GEOM_MANAGER& aManager,
                                    const EDA_IU_SCALE& aIuScale, EDA_UNITS aUnits ) :
        EDA_ITEM( NOT_USED ), m_constructMan( aManager ), m_units( aUnits )
{
}


const BOX2I BEZIER_ASSISTANT::ViewBBox() const
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


void BEZIER_ASSISTANT::ViewDraw( int aLayer, KIGFX::VIEW* aView ) const
{
    KIGFX::GAL& gal = *aView->GetGAL();

    // not in a position to draw anything
    if( m_constructMan.IsReset() )
        return;

    gal.ResetTextAttributes();

    const VECTOR2I                          start = m_constructMan.GetStart();
    const BEZIER_GEOM_MANAGER::BEZIER_STEPS step = m_constructMan.GetStep();

    KIGFX::PREVIEW::DRAW_CONTEXT preview_ctx( *aView );

    int dashSize = KiROUND( aView->ToWorld( 12 ) );

    if( step >= BEZIER_GEOM_MANAGER::BEZIER_STEPS::SET_CONTROL1 )
    {
        // Draw the first control point control line
        preview_ctx.DrawLineDashed( start, m_constructMan.GetControlC1(), dashSize, dashSize / 2,
                                    false );
    }

    if( step >= BEZIER_GEOM_MANAGER::BEZIER_STEPS::SET_CONTROL2 )
    {
        const VECTOR2I c2vec = m_constructMan.GetControlC2() - m_constructMan.GetEnd();

        // Draw the second control point control line as a double length line
        // centered on the end point
        preview_ctx.DrawLineDashed( m_constructMan.GetEnd() - c2vec, m_constructMan.GetControlC2(),
                                    dashSize, dashSize / 2, false );
    }

    wxArrayString cursorStrings;

    if( step >= BEZIER_GEOM_MANAGER::BEZIER_STEPS::SET_END )
    {
        // Going to need a better way to get a length here
        // const int length = m_constructMan.GetBezierLength();
        // Have enough points to report a bezier length
        // cursorStrings.push_back( DimensionLabel( wxString::FromUTF8( "L" ), 12300000,
        //                                         m_iuScale, m_units ) );
    }

    if( !cursorStrings.empty() )
    {
        // place the text next to cursor, on opposite side from radius
        DrawTextNextToCursor( aView, m_constructMan.GetLastPoint(),
                              start - m_constructMan.GetLastPoint(), cursorStrings,
                              aLayer == LAYER_SELECT_OVERLAY );
    }
}
