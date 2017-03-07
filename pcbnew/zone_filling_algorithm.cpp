/**
 * @file zone_filling_algorithm.cpp:
 * Algorithms used to fill a zone defined by a polygon and a filling starting point.
 */
/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 1992-2016 KiCad Developers, see AUTHORS.txt for contributors.
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


#include <algorithm> // sort

#include <fctsys.h>
#include <trigo.h>
#include <wxPcbStruct.h>
#include <convert_basic_shapes_to_polygon.h>

#include <class_zone.h>

#include <pcbnew.h>
#include <zones.h>

/* Build the filled solid areas data from real outlines (stored in m_Poly)
 * The solid areas can be more than one on copper layers, and do not have holes
  ( holes are linked by overlapping segments to the main outline)
 * aPcb: the current board (can be NULL for non copper zones)
 * aCornerBuffer: A reference to a buffer to store polygon corners, or NULL
 * if aCornerBuffer == NULL:
 * - m_FilledPolysList is used to store solid areas polygons.
 * - on copper layers, tracks and other items shapes of other nets are
 * removed from solid areas
 * if not null:
 * Only the zone outline (with holes, if any) are stored in aCornerBuffer
 * with holes linked. Therefore only one polygon is created
 * This function calls AddClearanceAreasPolygonsToPolysList()
 * to add holes for pads and tracks and other items not in net.
 */

bool ZONE_CONTAINER::BuildFilledSolidAreasPolygons( BOARD* aPcb, SHAPE_POLY_SET* aOutlineBuffer )
{
    /* convert outlines + holes to outlines without holes (adding extra segments if necessary)
     * m_Poly data is expected normalized, i.e. NormalizeAreaOutlines was used after building
     * this zone
     */

    if( GetNumCorners() <= 2 )  // malformed zone. polygon calculations do not like it ...
        return false;

    // Make a smoothed polygon out of the user-drawn polygon if required
    if( m_smoothedPoly )
    {
        delete m_smoothedPoly;
        m_smoothedPoly = NULL;
    }

    switch( m_cornerSmoothingType )
    {
    case ZONE_SETTINGS::SMOOTHING_CHAMFER:
        m_smoothedPoly = new SHAPE_POLY_SET();
        *m_smoothedPoly = m_Poly->Chamfer( m_cornerRadius );
        break;

    case ZONE_SETTINGS::SMOOTHING_FILLET:
        m_smoothedPoly = new SHAPE_POLY_SET();
        *m_smoothedPoly = m_Poly->Fillet( m_cornerRadius, m_ArcToSegmentsCount );
        break;

    default:
        // Acute angles between adjacent edges can create issues in calculations,
        // in inflate/deflate outlines transforms, especially when the angle is very small.
        // We can avoid issues by creating a very small chamfer which remove acute angles,
        // or left it without chamfer and use only CPOLYGONS_LIST::InflateOutline to create
        // clearance areas
        m_smoothedPoly = new SHAPE_POLY_SET();
        *m_smoothedPoly = m_Poly->Chamfer( Millimeter2iu( 0.0 ) );
        break;
    }

    if( aOutlineBuffer )
        aOutlineBuffer->Append( *m_smoothedPoly );

    /* For copper layers, we now must add holes in the Polygon list.
     * holes are pads and tracks with their clearance area
     * For non copper layers, just recalculate the m_FilledPolysList
     * with m_ZoneMinThickness taken in account
     */
    else
    {
        m_FilledPolysList.RemoveAllContours();

        if( IsOnCopperLayer() )
        {
            AddClearanceAreasPolygonsToPolysList_NG( aPcb );

            if( m_FillMode )   // if fill mode uses segments, create them:
            {
                if( !FillZoneAreasWithSegments() )
                    return false;
            }
        }
        else
        {
            m_FillMode = 0;     // Fill by segments is no more used in non copper layers
                                // force use solid polygons (usefull only for old boards)
            m_FilledPolysList = *m_smoothedPoly;

            // The filled areas are deflated by -m_ZoneMinThickness / 2, because
            // the outlines are drawn with a line thickness = m_ZoneMinThickness to
            // give a good shape with the minimal thickness
            m_FilledPolysList.Inflate( -m_ZoneMinThickness / 2, 16 );
            m_FilledPolysList.Fracture( SHAPE_POLY_SET::PM_FAST );
        }

        m_IsFilled = true;
    }

    return true;
}


/** Helper function fillPolygonWithHorizontalSegments
 * fills a polygon with horizontal segments.
 * It can be used for any angle, if the zone outline to fill is rotated by this angle
 * and the result is rotated by -angle
 * @param aPolygon = a SHAPE_LINE_CHAIN polygon to fill
 * @param aFillSegmList = a std::vector <SEGMENT> which will be populated by filling segments
 * @param aStep = the horizontal grid size
 */
bool fillPolygonWithHorizontalSegments( const SHAPE_LINE_CHAIN& aPolygon,
                                 std::vector <SEGMENT>& aFillSegmList, int aStep );

bool ZONE_CONTAINER::FillZoneAreasWithSegments()
{
    bool success = true;
    // segments are on something like a grid. Give it a minimal size
    // to avoid too many segments, and use the m_ZoneMinThickness when (this is usually the case)
    // the size is > mingrid_size.
    // This is not perfect, but the actual purpose of this code
    // is to allow filling zones on a grid, with grid size > m_ZoneMinThickness,
    // in order to have really a grid.
    //
    // Using a user selectable grid size is for future Kicad versions.
    // For now the area is fully filled.
    int mingrid_size = Millimeter2iu( 0.05 );
    int grid_size = std::max( mingrid_size, m_ZoneMinThickness );
    // Make segments slightly overlapping to ensure a good full filling
    grid_size -= grid_size/20;

    // All filled areas are in m_FilledPolysList
    // m_FillSegmList will contain the horizontal and vertical segments
    // the segment width is m_ZoneMinThickness.
    m_FillSegmList.clear();

    // Creates the horizontal segments
    for ( int index = 0; index < m_FilledPolysList.OutlineCount(); index++ )
    {
        const SHAPE_LINE_CHAIN& outline0 = m_FilledPolysList.COutline( index );
        success = fillPolygonWithHorizontalSegments( outline0, m_FillSegmList, grid_size );

        if( !success )
            break;

        // Creates the vertical segments. Because the filling algo creates horizontal segments,
        // to reuse the fillPolygonWithHorizontalSegments function, we rotate the polygons to fill
        // then fill them, then inverse rotate the result
        SHAPE_LINE_CHAIN outline90;
        outline90.Append( outline0 );

        // Rotate 90 degrees the outline:
        for( int ii = 0; ii < outline90.PointCount(); ii++ )
        {
            VECTOR2I& point = outline90.Point( ii );
            std::swap( point.x, point.y );
            point.y = -point.y;
        }

        int first_point = m_FillSegmList.size();
        success = fillPolygonWithHorizontalSegments( outline90, m_FillSegmList, grid_size );

        if( !success )
            break;

        // Rotate -90 degrees the segments:
        for( unsigned ii = first_point; ii < m_FillSegmList.size(); ii++ )
        {
            SEGMENT& segm = m_FillSegmList[ii];
            std::swap( segm.m_Start.x, segm.m_Start.y );
            std::swap( segm.m_End.x, segm.m_End.y );
            segm.m_Start.x = - segm.m_Start.x;
            segm.m_End.x = - segm.m_End.x;
        }
    }

    if( success )
        m_IsFilled = true;
    else
        m_FillSegmList.clear();

    return success;
}


bool fillPolygonWithHorizontalSegments( const SHAPE_LINE_CHAIN& aPolygon,
                                        std::vector <SEGMENT>& aFillSegmList, int aStep )
{
    std::vector <int> x_coordinates;
    bool success = true;

    // Creates the horizontal segments
    const SHAPE_LINE_CHAIN& outline = aPolygon;
    const BOX2I& rect = outline.BBox();

    // Calculate the y limits of the zone
    for( int refy = rect.GetY(), endy = rect.GetBottom(); refy < endy; refy += aStep )
    {
        // find all intersection points of an infinite line with polyline sides
        x_coordinates.clear();

        for( int v = 0; v < outline.PointCount(); v++ )
        {

            int seg_startX = outline.CPoint( v ).x;
            int seg_startY = outline.CPoint( v ).y;
            int seg_endX   = outline.CPoint( v + 1 ).x;
            int seg_endY   = outline.CPoint( v + 1 ).y;

            /* Trivial cases: skip if ref above or below the segment to test */
            if( ( seg_startY > refy ) && ( seg_endY > refy ) )
                continue;

            // segment below ref point, or its Y end pos on Y coordinate ref point: skip
            if( ( seg_startY <= refy ) && (seg_endY <= refy ) )
                continue;

            /* at this point refy is between seg_startY and seg_endY
             * see if an horizontal line at Y = refy is intersecting this segment
             */
            // calculate the x position of the intersection of this segment and the
            // infinite line this is more easier if we move the X,Y axis origin to
            // the segment start point:

            seg_endX -= seg_startX;
            seg_endY -= seg_startY;
            double newrefy = (double) ( refy - seg_startY );
            double intersec_x;

            if ( seg_endY == 0 )    // horizontal segment on the same line: skip
                continue;

            // Now calculate the x intersection coordinate of the horizontal line at
            // y = newrefy and the segment from (0,0) to (seg_endX,seg_endY) with the
            // horizontal line at the new refy position the line slope is:
            // slope = seg_endY/seg_endX; and inv_slope = seg_endX/seg_endY
            // and the x pos relative to the new origin is:
            // intersec_x = refy/slope = refy * inv_slope
            // Note: because horizontal segments are already tested and skipped, slope
            // exists (seg_end_y not O)
            double inv_slope = (double) seg_endX / seg_endY;
            intersec_x = newrefy * inv_slope;
            x_coordinates.push_back( (int) intersec_x + seg_startX );
        }

        // A line scan is finished: build list of segments

        // Sort intersection points by increasing x value:
        // So 2 consecutive points are the ends of a segment
        sort( x_coordinates.begin(), x_coordinates.end() );

        // An even number of coordinates is expected, because a segment has 2 ends.
        // An if this algorithm always works, it must always find an even count.
        if( ( x_coordinates.size() & 1 ) != 0 )
        {
            success = false;
            break;
        }

        // Create segments having the same Y coordinate
        int iimax = x_coordinates.size() - 1;

        for( int ii = 0; ii < iimax; ii += 2 )
        {
            wxPoint  seg_start, seg_end;
            seg_start.x = x_coordinates[ii];
            seg_start.y = refy;
            seg_end.x = x_coordinates[ii + 1];
            seg_end.y = refy;
            SEGMENT segment( seg_start, seg_end );
            aFillSegmList.push_back( segment );
        }
    }   // End examine segments in one area

    return success;
}
