/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2025 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "gerber_to_polyset.h"
#include "gerber_file_image.h"
#include "gerber_draw_item.h"
#include "dcode.h"
#include <convert_basic_shapes_to_polygon.h>


SHAPE_POLY_SET ConvertGerberToPolySet( GERBER_FILE_IMAGE* aImage, int aTolerance )
{
    SHAPE_POLY_SET mergedPolygons;

    if( !aImage )
        return mergedPolygons;

    // Accumulate positive and negative items separately, then perform a single boolean
    // pass for each negative object: a negative object erase only previously drawn items.
    SHAPE_POLY_SET positivePolygons;
    SHAPE_POLY_SET negativePolygons;

    for( GERBER_DRAW_ITEM* item : aImage->GetItems() )
    {
        SHAPE_POLY_SET itemPoly;
        bool           needsFlashOffset = false;

        if( item->m_ShapeAsPolygon.OutlineCount() > 0 )
        {
            itemPoly = item->m_ShapeAsPolygon;
        }
        else if( item->m_ShapeType == GBR_SEGMENT )
        {
            D_CODE* dcode = item->GetDcodeDescr();

            if( dcode && dcode->m_ApertType != APT_RECT )
            {
                int arcError = static_cast<int>( gerbIUScale.IU_PER_MM * ARC_LOW_DEF_MM );
                TransformOvalToPolygon( itemPoly, item->m_Start, item->m_End,
                                        item->m_Size.x, arcError, ERROR_INSIDE );
            }
            else
            {
                item->ConvertSegmentToPolygon( &itemPoly );
            }
        }
        else if( item->m_ShapeType == GBR_ARC )
        {
            item->ConvertSegmentToPolygon( &itemPoly );
        }
        else if( item->m_Flashed )
        {
            D_CODE* dcode = item->GetDcodeDescr();

            if( dcode )
            {
                dcode->ConvertShapeToPolygon( item );
                itemPoly = dcode->m_Polygon;
                needsFlashOffset = true;
            }
        }

        if( itemPoly.OutlineCount() == 0 )
            continue;

        // Flashed shapes from ConvertShapeToPolygon are centered at (0,0).
        // Offset by the item's position before applying the AB transform.
        VECTOR2I offset = needsFlashOffset ? VECTOR2I( item->m_Start ) : VECTOR2I( 0, 0 );

        SHAPE_POLY_SET& dest = item->GetLayerPolarity() ? negativePolygons : positivePolygons;

        for( int i = 0; i < itemPoly.OutlineCount(); i++ )
        {
            const SHAPE_LINE_CHAIN& outline = itemPoly.COutline( i );
            dest.NewOutline();

            for( int j = 0; j < outline.PointCount(); j++ )
                dest.Append( item->GetABPosition( outline.CPoint( j ) + offset ) );

            for( int h = 0; h < itemPoly.HoleCount( i ); h++ )
            {
                const SHAPE_LINE_CHAIN& hole = itemPoly.CHole( i, h );
                dest.NewHole();

                for( int j = 0; j < hole.PointCount(); j++ )
                    dest.Append( item->GetABPosition( hole.CPoint( j ) + offset ) );
            }
        }

        // Handle negative polygons: they are subtracted to previous polygons
        if( negativePolygons.OutlineCount() > 0 )
        {
            positivePolygons.BooleanSubtract( negativePolygons );
            negativePolygons.RemoveAllContours();
        }
    }

    // Single-pass union of all positive items
    positivePolygons.Simplify();

    mergedPolygons = std::move( positivePolygons );

    if( aTolerance > 0 )
        mergedPolygons.Inflate( aTolerance, CORNER_STRATEGY::ROUND_ALL_CORNERS, 16 );

    return mergedPolygons;
}
