/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015-2016 Mario Luzeiro <mrluzeiro@ua.pt>
 * Copyright (C) 1992-2019 KiCad Developers, see AUTHORS.txt for contributors.
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
 * contours format. It is based on the function found in the files:
 *  board_items_to_polygon_shape_transform.cpp
 *  board_items_to_polygon_shape_transform.cpp
 */

#include "cinfo3d_visu.h"
#include <convert_basic_shapes_to_polygon.h>
#include <class_edge_mod.h>
#include <class_module.h>


// This is the same function as in board_items_to_polygon_shape_transform.cpp
// but it adds the rect/trapezoid shapes with a different winding
void CINFO3D_VISU::buildPadShapePolygon( const D_PAD* aPad,
                                         SHAPE_POLY_SET& aCornerBuffer,
                                         wxSize aInflateValue ) const
{
    wxPoint corners[4];
    wxPoint PadShapePos = aPad->ShapePos(); /* Note: for pad having a shape offset,
                                             * the pad position is NOT the shape position */
    switch( aPad->GetShape() )
    {
    case PAD_SHAPE_CIRCLE:
    case PAD_SHAPE_OVAL:
    case PAD_SHAPE_ROUNDRECT:
    case PAD_SHAPE_CHAMFERED_RECT:
    {
        // We are using TransformShapeWithClearanceToPolygon to build the shape.
        // Currently, this method uses only the same inflate value for X and Y dirs.
        // so because here this is not the case, we use a inflated dummy pad to build
        // the polygonal shape
        // TODO: remove this dummy pad when TransformShapeWithClearanceToPolygon will use
        // a wxSize to inflate the pad size
        D_PAD dummy( *aPad );
        wxSize new_size = aPad->GetSize() + aInflateValue + aInflateValue;
        dummy.SetSize( new_size );
        dummy.TransformShapeWithClearanceToPolygon( aCornerBuffer, 0 );
    }
        break;

    case PAD_SHAPE_TRAPEZOID:
    case PAD_SHAPE_RECT:
    {
        SHAPE_LINE_CHAIN aLineChain;

        aPad->BuildPadPolygon( corners, aInflateValue, aPad->GetOrientation() );

        for( int ii = 0; ii < 4; ++ii )
        {
            corners[3-ii] += PadShapePos;          // Shift origin to position
            aLineChain.Append( corners[3-ii].x, corners[3-ii].y );
        }

        aLineChain.SetClosed( true );

        aCornerBuffer.AddOutline( aLineChain );
    }
        break;

    case PAD_SHAPE_CUSTOM:
        {
        SHAPE_POLY_SET polyList;     // Will contain the pad outlines in board coordinates
        auto           inflate_val = std::max( aInflateValue.x, aInflateValue.y );

        polyList.Append( aPad->GetCustomShapeAsPolygon() );
        aPad->CustomShapeAsPolygonToBoardPosition( &polyList, aPad->ShapePos(), aPad->GetOrientation() );

        if( inflate_val > 0 )
        {
            int numSegs = GetNrSegmentsCircle( inflate_val );
            polyList.Inflate( inflate_val, numSegs );
        }

        aCornerBuffer.Append( polyList );
        }
        break;
    }
}


void CINFO3D_VISU::buildPadShapeThickOutlineAsPolygon( const D_PAD* aPad,
                                                SHAPE_POLY_SET& aCornerBuffer,
                                                int             aWidth ) const
{
    if( aPad->GetShape() == PAD_SHAPE_CIRCLE )    // Draw a ring
    {
        TransformRingToPolygon( aCornerBuffer, aPad->ShapePos(),
                                aPad->GetSize().x / 2, ARC_HIGH_DEF, aWidth );
        return;
    }


    // For other shapes, draw polygon outlines
    SHAPE_POLY_SET corners;

    buildPadShapePolygon( aPad, corners, wxSize( 0, 0 ) );

    // Add outlines as thick segments in polygon buffer

    const SHAPE_LINE_CHAIN& path = corners.COutline( 0 );

    for( int ii = 0; ii < path.PointCount(); ++ii )
    {
        const VECTOR2I& a = path.CPoint( ii );
        const VECTOR2I& b = path.CPoint( ii + 1 );

        TransformSegmentToPolygon( aCornerBuffer, wxPoint( a.x, a.y ),
                                   wxPoint( b.x, b.y ), ARC_HIGH_DEF, aWidth );
    }
}


// Based on the same function name in board_items_to_polyshape_transform.cpp
// It was implemented here to allow dynamic segments count per pad shape
void CINFO3D_VISU::transformPadsShapesWithClearanceToPolygon( const PADS& aPads, PCB_LAYER_ID aLayer,
                                                              SHAPE_POLY_SET& aCornerBuffer,
                                                              int aInflateValue,
                                                              bool aSkipNPTHPadsWihNoCopper ) const
{
    wxSize margin;
    for( auto pad : aPads )
    {
        if( !pad->IsOnLayer(aLayer) )
            continue;

        // NPTH pads are not drawn on layers if the shape size and pos is the same
        // as their hole:
        if( aSkipNPTHPadsWihNoCopper && (pad->GetAttribute() == PAD_ATTRIB_HOLE_NOT_PLATED) )
        {
            if( (pad->GetDrillSize() == pad->GetSize()) &&
                (pad->GetOffset() == wxPoint( 0, 0 )) )
            {
                switch( pad->GetShape() )
                {
                case PAD_SHAPE_CIRCLE:
                    if( pad->GetDrillShape() == PAD_DRILL_SHAPE_CIRCLE )
                        continue;
                    break;

                case PAD_SHAPE_OVAL:
                    if( pad->GetDrillShape() != PAD_DRILL_SHAPE_CIRCLE )
                        continue;
                    break;

                default:
                    break;
                }
            }
        }

        switch( aLayer )
        {
        case F_Mask:
        case B_Mask:
            margin.x = margin.y = pad->GetSolderMaskMargin() + aInflateValue;
            break;

        case F_Paste:
        case B_Paste:
            margin = pad->GetSolderPasteMargin();
            margin.x += aInflateValue;
            margin.y += aInflateValue;
            break;

        default:
            margin.x = margin.y = aInflateValue;
            break;
        }

        buildPadShapePolygon( pad, aCornerBuffer, margin );
    }
}

void CINFO3D_VISU::transformGraphicModuleEdgeToPolygonSet( const MODULE *aModule,
                                                           PCB_LAYER_ID aLayer,
                                                           SHAPE_POLY_SET& aCornerBuffer ) const
{
    for( auto item : aModule->GraphicalItems() )
    {
        switch( item->Type() )
        {
        case PCB_MODULE_EDGE_T:
        {
            EDGE_MODULE*outline = (EDGE_MODULE*) item;

            if( outline->GetLayer() != aLayer )
                break;

            outline->TransformShapeWithClearanceToPolygon( aCornerBuffer, 0 );
        }
            break;

            default:
                break;
        }
    }
}
