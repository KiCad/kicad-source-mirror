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
#include <view/view.h>

using namespace KIGFX::PREVIEW;

// Selection area colours
const COLOR4D SELECT_MODE_NORMAL( 0.3, 0.3, 0.7, 0.3 ); // Slight blue
const COLOR4D SELECT_MODE_ADDITIVE( 0.3, 0.7, 0.3, 0.3 ); // Slight green
const COLOR4D SELECT_MODE_SUBTRACT( 0.7, 0.3, 0.3, 0.3 ); // Slight red

const COLOR4D SELECT_OUTLINE_L2R( 1.0, 1.0, 0.4, 1.0 );
const COLOR4D SELECT_OUTLINE_R2L( 0.4, 0.4, 1.0, 1.0 );

SELECTION_AREA::SELECTION_AREA() :
        m_additive( false ),
        m_subtractive( false )
{
    SetStrokeColor( SELECT_OUTLINE_L2R );
    SetFillColor( SELECT_MODE_NORMAL );
}


void SELECTION_AREA::SetAdditive( bool aAdditive )
{
    m_additive = aAdditive;

    if( m_additive )
        m_subtractive = false;
}


void SELECTION_AREA::SetSubtractive( bool aSubtractive )
{
    m_subtractive = aSubtractive;

    if( m_subtractive )
        m_additive = false;
}


const BOX2I SELECTION_AREA::ViewBBox() const
{
    BOX2I tmp;

    tmp.SetOrigin( m_origin );
    tmp.SetEnd( m_end );
    tmp.Normalize();
    return tmp;
}


void SELECTION_AREA::drawPreviewShape( KIGFX::GAL& aGal ) const
{
    // Set the fill of the selection rectangle
    // based on the selection mode
    if( m_additive )
    {
        aGal.SetFillColor( SELECT_MODE_ADDITIVE );
    }
    else if( m_subtractive )
    {
        aGal.SetFillColor( SELECT_MODE_SUBTRACT );
    }
    else
    {
        aGal.SetFillColor( SELECT_MODE_NORMAL );
    }

    // Set the stroke color to indicate window or crossing selection
    aGal.SetStrokeColor( ( m_origin.x <= m_end.x ) ? SELECT_OUTLINE_L2R : SELECT_OUTLINE_R2L );

    aGal.DrawRectangle( m_origin, m_end );
}
