/**
 * @file zone_filling_algorithm.cpp:
 * Algorithms used to fill a zone defined by a polygon and a filling starting point.
 */


#include <algorithm> // sort

#include "fctsys.h"
#include "trigo.h"
#include "wxPcbStruct.h"

#include "class_zone.h"

#include "pcbnew.h"
#include "zones.h"
#include "protos.h"

/* Local functions */

/* Local variables */


/**
 * Function BuildFilledPolysListData
 * Build m_FilledPolysList data from real outlines (m_Poly)
 * in order to have drawable (and plottable) filled polygons
 * drawable filled polygons are polygons without hole
 * @param aPcb: the current board (can be NULL for non copper zones)
 * @return number of polygons
 * This function does not add holes for pads and tracks but calls
 * AddClearanceAreasPolygonsToPolysList() to do that for copper layers
 */
int ZONE_CONTAINER::BuildFilledPolysListData( BOARD* aPcb )
{
    m_FilledPolysList.clear();

    /* convert outlines + holes to outlines without holes (adding extra segments if necessary)
     * m_Poly data is expected normalized, i.e. NormalizeAreaOutlines was used after building
     * this zone
     */

    if( GetNumCorners() <= 2 )  // malformed zone. Kbool does not like it ...
        return 0;

    // Make a smoothed polygon out of the user-drawn polygon if required
    if( smoothedPoly ) {
        delete smoothedPoly;
        smoothedPoly = NULL;
    }

    switch( cornerSmoothingType )
    {
    case ZONE_SETTING::SMOOTHING_CHAMFER:
        smoothedPoly = m_Poly->Chamfer( cornerRadius );
        break;
    case ZONE_SETTING::SMOOTHING_FILLET:
        smoothedPoly = m_Poly->Fillet( cornerRadius, m_ArcToSegmentsCount );
        break;
    default:
        smoothedPoly = new CPolyLine;
        smoothedPoly->Copy( m_Poly );
        break;
    }

    smoothedPoly->MakeKboolPoly( -1, -1, NULL, true );
    int count = 0;
    while( smoothedPoly->GetKboolEngine()->StartPolygonGet() )
    {
        CPolyPt corner( 0, 0, false );
        while( smoothedPoly->GetKboolEngine()->PolygonHasMorePoints() )
        {
            corner.x = (int) smoothedPoly->GetKboolEngine()->GetPolygonXPoint();
            corner.y = (int) smoothedPoly->GetKboolEngine()->GetPolygonYPoint();
            corner.end_contour = false;
            m_FilledPolysList.push_back( corner );
            count++;
        }

        corner.end_contour = true;
        m_FilledPolysList.pop_back();
        m_FilledPolysList.push_back( corner );
        smoothedPoly->GetKboolEngine()->EndPolygonGet();
    }

    smoothedPoly->FreeKboolEngine();

    /* For copper layers, we now must add holes in the Polygon list.
     * holes are pads and tracks with their clearance area
     */

    if( IsOnCopperLayer() )
        AddClearanceAreasPolygonsToPolysList( aPcb );

    if ( m_FillMode )   // if fill mode uses segments, create them:
        Fill_Zone_Areas_With_Segments( );

    return count;
}

// Sort function to build filled zones
static bool SortByXValues( const int& a, const int &b)
{
    return a < b;
}


/**
 * Function Fill_Zone_Areas_With_Segments
 * Fill sub areas in a zone with segments with m_ZoneMinThickness width
 * A scan is made line per line, on the whole filled areas, with a step of m_ZoneMinThickness.
 * all intersecting points with the horizontal infinite line and polygons to fill are calculated
 * a list of SEGZONE items is built, line per line
 * @return number of segments created
 */
int ZONE_CONTAINER::Fill_Zone_Areas_With_Segments()
{
    int      ics, ice;
    int count = 0;
    std::vector <int> x_coordinates;
    bool error = false;

    int      istart, iend;      // index od the starting and the endif corner of one filled area in m_FilledPolysList

    int margin = m_ZoneMinThickness * 2 / 10;
    margin = max (2, margin);
    int step = m_ZoneMinThickness - margin;
    step = max(step, 2);

    // Read all filled areas in m_FilledPolysList
    m_FillSegmList.clear();
    istart = 0;
    int end_list =  m_FilledPolysList.size()-1;
    for( int ic = 0; ic <= end_list; ic++ )
    {
        CPolyPt* corner = &m_FilledPolysList[ic];
        if ( corner->end_contour || (ic == end_list) )
        {
            iend = ic;
            EDA_RECT rect = CalculateSubAreaBoundaryBox( istart, iend );

            // Calculate the y limits of the zone
            int refy = rect.GetY();
            int endy = rect.GetBottom();

            for( ; refy < endy; refy += step )
            {
                // find all intersection points of an infinite line with polyline sides
                x_coordinates.clear();
                for( ics = istart, ice = iend; ics <= iend; ice = ics, ics++ )
                {
                    if ( m_FilledPolysList[ice].utility )
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
                    // calculate the x position of the intersection of this segment and the infinite line
                    // this is more easier if we move the X,Y axis origin to the segment start point:
                    seg_endX -= seg_startX;
                    seg_endY -= seg_startY;
                    double newrefy = (double) (refy - seg_startY);
                    double intersec_x;
                    if ( seg_endY == 0 )    // horizontal segment on the same line: skip
                        continue;
                    // Now calculate the x intersection coordinate of the horizontal line at y = newrefy
                    // and the segment from (0,0) to (seg_endX,seg_endY)
                    // with the horizontal line at the new refy position
                    // the line slope is slope = seg_endY/seg_endX; and inv_slope = seg_endX/seg_endY
                    // and the x pos relative to the new origin is intersec_x = refy/slope = refy * inv_slope
                    // Note: because horizontal segments are already tested and skipped, slope exists (seg_end_y not O)
                    double inv_slope      = (double)seg_endX / seg_endY;
                    intersec_x = newrefy * inv_slope;
                    x_coordinates.push_back((int) intersec_x + seg_startX);
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

                if ( error )
                    break;

                int iimax = x_coordinates.size()-1;

                for (int ii = 0; ii < iimax; ii +=2 )
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

            if ( error )
                break;

            istart = iend + 1;  // istart points the first corner of the next area
        }   // End find one end of outline

        if ( error )
            break;
    }   // End examine all areas

    return count;
}


