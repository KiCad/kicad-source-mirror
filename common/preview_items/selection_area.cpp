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


SELECTION_AREA::SELECTION_AREA()
{
    SetStrokeColor( COLOR4D( 1.0, 1.0, 0.4, 1.0 ) );
    SetFillColor( COLOR4D( 0.3, 0.3, 0.5, 0.3 ) );
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
    aGal.DrawRectangle( m_origin, m_end );
}

