/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 CERN
 * @author Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
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

#include <preview_items/selection_area.h>

#include <gal/graphics_abstraction_layer.h>
#include <gal/painter.h>
#include <view/view.h>

using namespace KIGFX::PREVIEW;
using KIGFX::COLOR4D;

struct SELECTION_COLORS
{
    COLOR4D normal;
    COLOR4D additive;
    COLOR4D subtract;
    COLOR4D exclusiveOr;
    COLOR4D outline_l2r;
    COLOR4D outline_r2l;
};

static const SELECTION_COLORS selectionColorScheme[2] = {
    { // dark background
        COLOR4D( 0.3, 0.3, 0.7, 0.3 ), // Slight blue
        COLOR4D( 0.3, 0.7, 0.3, 0.3 ), // Slight green
        COLOR4D( 0.7, 0.3, 0.3, 0.3 ), // Slight red
        COLOR4D( 0.7, 0.3, 0.3, 0.3 ), // Slight red

        COLOR4D( 1.0, 1.0, 0.4, 1.0 ), // yellow
        COLOR4D( 0.4, 0.4, 1.0, 1.0 ) // blue
    },
    { // bright background
        COLOR4D( 0.5, 0.3, 1.0, 0.5 ), // Slight blue
        COLOR4D( 0.5, 1.0, 0.5, 0.5 ), // Slight green
        COLOR4D( 1.0, 0.5, 0.5, 0.5 ), // Slight red
        COLOR4D( 1.0, 0.5, 0.5, 0.5 ), // Slight red

        COLOR4D( 0.7, 0.7, 0.0, 1.0 ), // yellow
        COLOR4D( 0.1, 0.1, 1.0, 1.0 ) // blue
    }
};


SELECTION_AREA::SELECTION_AREA() :
        m_additive( false ),
        m_subtractive( false ),
        m_exclusiveOr( false ),
        m_mode( SELECTION_MODE::INSIDE_RECTANGLE )
{

}


const BOX2I SELECTION_AREA::ViewBBox() const
{
    BOX2I tmp;

    switch( m_mode )
    {
    default:
    case SELECTION_MODE::INSIDE_RECTANGLE:
    case SELECTION_MODE::TOUCHING_RECTANGLE:
        tmp.SetOrigin( m_origin );
        tmp.SetEnd( m_end );
        break;
    case SELECTION_MODE::INSIDE_LASSO:
    case SELECTION_MODE::TOUCHING_LASSO:
        tmp = m_shape_poly.BBox();
        break;
    }

    tmp.Normalize();

    return tmp;
}


void SELECTION_AREA::ViewDraw( int aLayer, KIGFX::VIEW* aView ) const
{
    KIGFX::GAL&      gal = *aView->GetGAL();
    RENDER_SETTINGS* settings = aView->GetPainter()->GetSettings();

    const SELECTION_COLORS& scheme = settings->IsBackgroundDark() ? selectionColorScheme[0]
                                                                  : selectionColorScheme[1];

    // Set the colors of the selection shape based on the selection mode
    if( m_additive )
        gal.SetFillColor( scheme.additive );
    else if( m_subtractive )
        gal.SetFillColor( scheme.subtract );
    else if( m_exclusiveOr )
        gal.SetFillColor( scheme.exclusiveOr );
    else
        gal.SetFillColor( scheme.normal );

    if( m_mode == SELECTION_MODE::INSIDE_RECTANGLE || m_mode == SELECTION_MODE::INSIDE_LASSO )
        gal.SetStrokeColor( scheme.outline_l2r );
    else
        gal.SetStrokeColor( scheme.outline_r2l );

    auto drawSelectionShape =
            [&]()
            {
                switch( m_mode )
                {
                default:
                case SELECTION_MODE::INSIDE_RECTANGLE:
                case SELECTION_MODE::TOUCHING_RECTANGLE:
                    gal.DrawRectangle( m_origin, m_end );
                    break;
                case SELECTION_MODE::INSIDE_LASSO:
                case SELECTION_MODE::TOUCHING_LASSO:
                    if( m_shape_poly.PointCount() > 1 )
                        gal.DrawPolygon( m_shape_poly );
                    break;
                }
            };

    gal.SetIsStroke( true );
    gal.SetIsFill( false );
    // force 1-pixel-wide line
    gal.SetLineWidth( 0.0 );
    drawSelectionShape();

    // draw the fill as the second object so that Z test will not clamp
    // the single-pixel-wide rectangle sides
    gal.SetIsFill( true );
    drawSelectionShape();
}
