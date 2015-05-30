/**
 * @file zone_filling_algorithm.cpp:
 * Algorithms used to fill a zone defined by a polygon and a filling starting point.
 */
/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 Jean-Pierre Charras, jean-pierre.charras@ujf-grenoble.fr
 * Copyright (C) 1992-2012 KiCad Developers, see AUTHORS.txt for contributors.
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

bool ZONE_CONTAINER::BuildFilledSolidAreasPolygons( BOARD* aPcb, CPOLYGONS_LIST* aOutlineBuffer )
{
    /* convert outlines + holes to outlines without holes (adding extra segments if necessary)
     * m_Poly data is expected normalized, i.e. NormalizeAreaOutlines was used after building
     * this zone
     */

    if( GetNumCorners() <= 2 )  // malformed zone. polygon calculations do not like it ...
        return 0;

    // Make a smoothed polygon out of the user-drawn polygon if required
    if( m_smoothedPoly )
    {
        delete m_smoothedPoly;
        m_smoothedPoly = NULL;
    }

    switch( m_cornerSmoothingType )
    {
    case ZONE_SETTINGS::SMOOTHING_CHAMFER:
        m_smoothedPoly = m_Poly->Chamfer( m_cornerRadius );
        break;

    case ZONE_SETTINGS::SMOOTHING_FILLET:
        m_smoothedPoly = m_Poly->Fillet( m_cornerRadius, m_ArcToSegmentsCount );
        break;

    default:
        // Acute angles between adjacent edges can create issues in calculations,
        // in inflate/deflate outlines transforms, especially when the angle is very small.
        // We can avoid issues by creating a very small chamfer which remove acute angles,
        // or left it without chamfer and use only CPOLYGONS_LIST::InflateOutline to create
        // clearance areas
        m_smoothedPoly = m_Poly->Chamfer( Millimeter2iu( 0.0 ) );
        break;
    }

    if( aOutlineBuffer )
        aOutlineBuffer->Append( m_smoothedPoly->m_CornersList );

    /* For copper layers, we now must add holes in the Polygon list.
     * holes are pads and tracks with their clearance area
     * for non copper layers just recalculate the m_FilledPolysList
     * with m_ZoneMinThickness taken in account
     */
    else
    {
        m_FilledPolysList.RemoveAllContours();

        if( IsOnCopperLayer() )
            AddClearanceAreasPolygonsToPolysList( aPcb );
        else
        {
            int         margin = m_ZoneMinThickness / 2;
            m_smoothedPoly->m_CornersList.InflateOutline(m_FilledPolysList, -margin, true );
        }

        if( m_FillMode )   // if fill mode uses segments, create them:
            FillZoneAreasWithSegments();

        m_IsFilled = true;
    }

    return true;
}


// Sort function to build filled zones
static bool SortByXValues( const int& a, const int &b )
{
    return a < b;
}


int ZONE_CONTAINER::FillZoneAreasWithSegments()
{
    int ics, ice;
    int count = 0;
    std::vector <int> x_coordinates;
    bool error = false;
    int istart, iend; // index of the starting and the endif corner of one filled area in m_FilledPolysList
    int margin = m_ZoneMinThickness * 2 / 10;
    int minwidth = Mils2iu( 2 );
    margin = std::max ( minwidth, margin );
    int step = m_ZoneMinThickness - margin;
    step = std::max( step, minwidth );

    // Read all filled areas in m_FilledPolysList
    m_FillSegmList.clear();
    istart = 0;
    int end_list =  m_FilledPolysList.GetCornersCount() - 1;

    for( int ic = 0; ic <= end_list; ic++ )
    {
        CPolyPt* corner = &m_FilledPolysList[ic];
        if ( corner->end_contour || ( ic == end_list ) )
        {
            iend = ic;
            EDA_RECT rect = CalculateSubAreaBoundaryBox( istart, iend );

            // Calculate the y limits of the zone
            for( int refy = rect.GetY(), endy = rect.GetBottom(); refy < endy; refy += step )
            {
                // find all intersection points of an infinite line with polyline sides
                x_coordinates.clear();

                for( ics = istart, ice = iend; ics <= iend; ice = ics, ics++ )
                {
                    if( m_FilledPolysList[ice].m_flags )
                        continue;

                    int seg_startX = m_FilledPolysList[ics].x;
                    int seg_startY = m_FilledPolysList[ics].y;
                    int seg_endX   = m_FilledPolysList[ice].x;
                    int seg_endY   = m_FilledPolysList[ice].y;


                    /* Trivial cases: skip if ref above or below the segment to test */
                    if( ( seg_startY > refy ) && (seg_endY > refy ) )
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
                sort( x_coordinates.begin(), x_coordinates.end(), SortByXValues );

                // Create segments

                if ( !error && ( x_coordinates.size() & 1 ) != 0 )
                {   // An even number of coordinates is expected, because a segment has 2 ends.
                    // An if this algorithm always works, it must always find an even count.
                    wxString msg = wxT("Fill Zone: odd number of points at y = ");
                    msg << refy;
                    wxMessageBox(msg );
                    error = true;
                }

                if( error )
                    break;

                int iimax = x_coordinates.size() - 1;

                for( int ii = 0; ii < iimax; ii +=2 )
                {
                    wxPoint  seg_start, seg_end;
                    count++;
                    seg_start.x = x_coordinates[ii];
                    seg_start.y = refy;
                    seg_end.x = x_coordinates[ii+1];
                    seg_end.y = refy;
                    SEGMENT segment( seg_start, seg_end );
                    m_FillSegmList.push_back( segment );
                }
            }   //End examine segments in one area

            if( error )
                break;

            istart = iend + 1;  // istart points the first corner of the next area
        }   // End find one end of outline

        if( error )
            break;
    }   // End examine all areas

    if( !error )
        m_IsFilled = true;

    return count;
}


