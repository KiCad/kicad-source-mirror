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

#include <gal/graphics_abstraction_layer.h>
#include <preview_items/two_point_assistant.h>
#include <preview_items/draw_context.h>
#include <preview_items/preview_utils.h>
#include <view/view.h>


using namespace KIGFX::PREVIEW;

TWO_POINT_ASSISTANT::TWO_POINT_ASSISTANT( const TWO_POINT_GEOMETRY_MANAGER& aManager,
                                          const EDA_IU_SCALE& aIuScale, EDA_UNITS aUnits,
                                          GEOM_SHAPE aShape ) :
        EDA_ITEM( NOT_USED ),
        m_constructMan( aManager ),
        m_units( aUnits ),
        m_shape( aShape ),
        m_iuScale( aIuScale )
{
}


const BOX2I TWO_POINT_ASSISTANT::ViewBBox() const
{
    BOX2I tmp;

    // no bounding box when no graphic shown
    if( m_constructMan.IsReset() )
        return tmp;

    // this is an edit-time artefact; no reason to try and be smart with the bounding box
    // (besides, we can't tell the text extents without a view to know what the scale is)
    tmp.SetMaximum();
    return tmp;
}


void TWO_POINT_ASSISTANT::ViewDraw( int aLayer, KIGFX::VIEW* aView ) const
{
    KIGFX::GAL& gal = *aView->GetGAL();

    // not in a position to draw anything
    if( m_constructMan.IsReset() )
        return;

    const VECTOR2I origin = m_constructMan.GetOrigin();
    const VECTOR2I end    = m_constructMan.GetEnd();
    const VECTOR2D radVec = end - origin;

    // Ensures that +90° is up and -90° is down in pcbnew
    const EDA_ANGLE deltaAngle( VECTOR2I( radVec.x, -radVec.y ) );

    if( radVec.x == 0 && radVec.y == 0 )
    {
        return; // text next to cursor jumps around a lot in this corner case
    }

    gal.ResetTextAttributes();

    wxArrayString cursorStrings;

    if( m_shape == GEOM_SHAPE::SEGMENT )
    {
        cursorStrings.push_back(
                DimensionLabel( "l", radVec.EuclideanNorm(), m_iuScale, m_units ) );
        cursorStrings.push_back( DimensionLabel( wxString::FromUTF8( "θ" ), deltaAngle.AsDegrees(),
                                                 m_iuScale, EDA_UNITS::DEGREES ) );
    }
    else if( m_shape == GEOM_SHAPE::RECT )
    {
        cursorStrings.push_back( DimensionLabel( "x", std::abs( radVec.x ), m_iuScale, m_units ) );
        cursorStrings.push_back( DimensionLabel( "y", std::abs( radVec.y ), m_iuScale, m_units ) );
    }
    else if( m_shape == GEOM_SHAPE::CIRCLE )
    {
        KIGFX::PREVIEW::DRAW_CONTEXT preview_ctx( *aView );
        preview_ctx.DrawLine( origin, end, false );

        cursorStrings.push_back(
                DimensionLabel( "r", radVec.EuclideanNorm(), m_iuScale, m_units ) );
    }

    // place the text next to cursor, on opposite side from drawing
    DrawTextNextToCursor( aView, end, origin - end, cursorStrings, aLayer == LAYER_SELECT_OVERLAY );
}
