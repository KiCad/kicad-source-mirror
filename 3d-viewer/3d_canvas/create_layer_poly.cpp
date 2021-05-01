/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015-2016 Mario Luzeiro <mrluzeiro@ua.pt>
 * Copyright (C) 1992-2020 KiCad Developers, see AUTHORS.txt for contributors.
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

/**
 * @file  create_layer_poly.cpp
 * @brief This file implements the creation of the pcb board items in the poly
 * contours format.
 */

#include "board_adapter.h"
#include <convert_basic_shapes_to_polygon.h>
#include <fp_shape.h>
#include <footprint.h>


void BOARD_ADAPTER::buildPadOutlineAsPolygon( const PAD* aPad, SHAPE_POLY_SET& aCornerBuffer,
                                              int aWidth ) const
{
    if( aPad->GetShape() == PAD_SHAPE::CIRCLE )    // Draw a ring
    {
        TransformRingToPolygon( aCornerBuffer, aPad->ShapePos(), aPad->GetSize().x / 2,
                                aWidth, ARC_HIGH_DEF, ERROR_INSIDE );
        return;
    }

    // For other shapes, add outlines as thick segments in polygon buffer
    const std::shared_ptr<SHAPE_POLY_SET>& corners = aPad->GetEffectivePolygon();
    const SHAPE_LINE_CHAIN&                path = corners->COutline( 0 );

    for( int ii = 0; ii < path.PointCount(); ++ii )
    {
        const VECTOR2I& a = path.CPoint( ii );
        const VECTOR2I& b = path.CPoint( ii + 1 );

        TransformOvalToPolygon( aCornerBuffer, (wxPoint) a, (wxPoint) b, aWidth, ARC_HIGH_DEF,
                                ERROR_INSIDE );
    }
}


void BOARD_ADAPTER::transformFPShapesToPolygon( const FOOTPRINT* aFootprint, PCB_LAYER_ID aLayer,
                                                SHAPE_POLY_SET& aCornerBuffer ) const
{
    for( BOARD_ITEM* item : aFootprint->GraphicalItems() )
    {
        if( item->Type() == PCB_FP_SHAPE_T )
        {
            FP_SHAPE* outline = (FP_SHAPE*) item;

            if( outline->GetLayer() == aLayer )
            {
                outline->TransformShapeWithClearanceToPolygon( aCornerBuffer, aLayer, 0,
                                                               ARC_HIGH_DEF, ERROR_INSIDE );
            }
        }
    }
}
