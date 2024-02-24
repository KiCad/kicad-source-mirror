/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 Ethan Chien <liangtie.qian@gmail.com>
 * Copyright (C) 2023 KiCad Developers, see AUTHORS.txt for contributors.
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

#include "zone_painter.h"
#include "board_edges_bounding_item.h"
#include "zone_manager_preference.h"
#include <layer_ids.h>
#include <convert_basic_shapes_to_polygon.h>
#include <gal/graphics_abstraction_layer.h>
#include <callback_gal.h>
#include <geometry/geometry_utils.h>
#include <geometry/shape_line_chain.h>
#include <geometry/shape_rect.h>
#include <geometry/shape_segment.h>
#include <geometry/shape_simple.h>
#include <geometry/shape_circle.h>
#include <bezier_curves.h>
#include <kiface_base.h>
#include <gr_text.h>
#include <pgm_base.h>
#include <wx/gdicmn.h>
#include <board_item.h>
#include <zone.h>

bool ZONE_PAINTER::Draw( const KIGFX::VIEW_ITEM* aItem, int aLayer )
{
    const BOARD_EDGES_BOUNDING_ITEM* item = dynamic_cast<const BOARD_EDGES_BOUNDING_ITEM*>( aItem );

    if( item )
    {
        draw( item, aLayer );
        return true;
    }

    return KIGFX::PCB_PAINTER::Draw( aItem, aLayer );
}

void ZONE_PAINTER::draw( const BOARD_EDGES_BOUNDING_ITEM* aBox, int aLayer )
{
    m_gal->Save();
    m_gal->SetFillColor( ZONE_MANAGER_PREFERENCE::GetBoundBoundingFillColor() );
    m_gal->SetLineWidth( 0 );
    m_gal->SetIsFill( true );
    m_gal->SetIsStroke( false );
    m_gal->DrawRectangle( aBox->ViewBBox() );
    m_gal->Restore();
}
