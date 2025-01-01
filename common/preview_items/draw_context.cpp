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
#include <preview_items/draw_context.h>
#include <preview_items/preview_utils.h>
#include <view/view.h>

using namespace KIGFX::PREVIEW;

using KIGFX::COLOR4D;


static constexpr double ANGLE_EPSILON = 1e-9;

static bool angleIsSpecial( EDA_ANGLE aAngle )
{
    return std::fabs( std::remainder( aAngle.AsRadians(), M_PI_4 ) ) < ANGLE_EPSILON;
}


static COLOR4D deemphasise( const COLOR4D& aColor, bool aDeEmphasised )
{
    return aColor.WithAlpha( PreviewOverlayDeemphAlpha( aDeEmphasised ) );
}


DRAW_CONTEXT::DRAW_CONTEXT( KIGFX::VIEW& aView )
        : m_gal( *aView.GetGAL() ),
          m_render_settings( *aView.GetPainter()->GetSettings() ),
          m_currLayer( LAYER_AUX_ITEMS ),
          m_lineWidth( 1.0f )
{
}


void DRAW_CONTEXT::DrawCircle( const VECTOR2I& aOrigin, double aRad, bool aDeEmphasised )
{
    const COLOR4D& color = m_render_settings.GetLayerColor( m_currLayer );

    m_gal.SetLineWidth( m_lineWidth );
    m_gal.SetStrokeColor( deemphasise( color, aDeEmphasised ) );
    m_gal.SetIsStroke( true );
    m_gal.SetIsFill( false );
    m_gal.DrawCircle( aOrigin, aRad );
}


void DRAW_CONTEXT::DrawCircleDashed( const VECTOR2I& aOrigin, double aRad, double aStepAngle,
                                     double aFillAngle, bool aDeEmphasised )
{
    const COLOR4D& color = m_render_settings.GetLayerColor( m_currLayer );

    m_gal.SetLineWidth( m_lineWidth );
    m_gal.SetStrokeColor( deemphasise( color, aDeEmphasised ) );
    m_gal.SetIsStroke( true );
    m_gal.SetIsFill( false );

    for( int i = 0; i < 360; i += aStepAngle )
    {
        m_gal.DrawArc( aOrigin, aRad, EDA_ANGLE( i, DEGREES_T ),
                       EDA_ANGLE( i + aFillAngle, DEGREES_T ) );
    }
}


void DRAW_CONTEXT::DrawLine( const VECTOR2I& aStart, const VECTOR2I& aEnd, bool aDeEmphasised )
{
    COLOR4D strokeColor = m_render_settings.GetLayerColor( m_currLayer );

    m_gal.SetLineWidth( m_lineWidth );
    m_gal.SetIsStroke( true );
    m_gal.SetStrokeColor( deemphasise( strokeColor, aDeEmphasised ) );
    m_gal.DrawLine( aStart, aEnd );
}


void DRAW_CONTEXT::DrawLineDashed( const VECTOR2I& aStart, const VECTOR2I& aEnd, int aDashStep,
                                   int aDashFill, bool aDeEmphasised )
{
    COLOR4D strokeColor = m_render_settings.GetLayerColor( m_currLayer );

    m_gal.SetLineWidth( m_lineWidth );
    m_gal.SetIsStroke( true );
    m_gal.SetStrokeColor( deemphasise( strokeColor, aDeEmphasised ) );

    VECTOR2I delta = aEnd - aStart;
    int      vecLen = delta.EuclideanNorm();

    for( int i = 0; i < vecLen; i += aDashStep )
    {
        VECTOR2I a = aStart + delta.Resize( i );
        VECTOR2I b = aStart + delta.Resize( std::min( i + aDashFill, vecLen ) );

        m_gal.DrawLine( a, b );
    }
}


void DRAW_CONTEXT::DrawLineWithAngleHighlight( const VECTOR2I& aStart, const VECTOR2I& aEnd,
                                               bool aDeEmphasised )
{
    const VECTOR2I vec = aEnd - aStart;
    COLOR4D        strokeColor = m_render_settings.GetLayerColor( m_currLayer );

    if( angleIsSpecial( EDA_ANGLE( vec ) ) )
        strokeColor = getSpecialAngleColour();

    m_gal.SetLineWidth( m_lineWidth );
    m_gal.SetIsStroke( true );
    m_gal.SetStrokeColor( deemphasise( strokeColor, aDeEmphasised ) );
    m_gal.DrawLine( aStart, aEnd );
}


COLOR4D DRAW_CONTEXT::getSpecialAngleColour() const
{
    return m_render_settings.IsBackgroundDark() ? COLOR4D( 0.5, 1.0, 0.5, 1.0 ) :
                                                  COLOR4D( 0.0, 0.7, 0.0, 1.0 );
}