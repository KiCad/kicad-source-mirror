/* filling_zone_algorithm:
 * Algos used to fill a zone defined by a polygon and a filling starting point
 */


#include <algorithm> // sort

#include "fctsys.h"

#include "common.h"
#include "pcbnew.h"

#include "autorout.h"
#include "zones.h"
#include "cell.h"
#include "trigo.h"
#include "protos.h"

/* Local functions */

/* Local variables */


/***********************************************************/
int ZONE_CONTAINER::BuildFilledPolysListData( BOARD* aPcb )
/***********************************************************/

/** function BuildFilledPolysListData
 * Build m_FilledPolysList data from real outlines (m_Poly)
 * in order to have drawable (and plottable) filled polygons
 * drawable filled polygons are polygons without hole
 * @param aPcb: the current board (can be NULL for non copper zones)
 * @return number of polygons
 * This function does not add holes for pads and tracks but calls
 * AddClearanceAreasPolygonsToPolysList() to do that for copper layers
 */
{
    m_FilledPolysList.clear();

    /* convert outlines + holes to outlines without holes (adding extra segments if necessary)
     * m_Poly data is expected normalized, i.e. NormalizeAreaOutlines was used after building this zone
     */

    if( GetNumCorners() <= 2 )  // malformed zone. Kbool does not like it ...
        return 0;

    m_Poly->MakeKboolPoly( -1, -1, NULL, true );
    int count = 0;
    while( m_Poly->GetKboolEngine()->StartPolygonGet() )
    {
        CPolyPt corner( 0, 0, false );
        while( m_Poly->GetKboolEngine()->PolygonHasMorePoints() )
        {
            corner.x = (int) m_Poly->GetKboolEngine()->GetPolygonXPoint();
            corner.y = (int) m_Poly->GetKboolEngine()->GetPolygonYPoint();
            corner.end_contour = false;
            m_FilledPolysList.push_back( corner );
            count++;
        }

        corner.end_contour = true;
        m_FilledPolysList.pop_back();
        m_FilledPolysList.push_back( corner );
        m_Poly->GetKboolEngine()->EndPolygonGet();
    }

    m_Poly->FreeKboolEngine();

    /* For copper layers, we now must add holes in the Polygon list.
     * holes are pads and tracks with their clearance area
     */

    if( IsOnCopperLayer() )
        AddClearanceAreasPolygonsToPolysList( aPcb );

    if ( m_FillMode )   // if fill mode uses segments, create them:
        Fill_Zone_Areas_With_Segments( (WinEDA_PcbFrame*) aPcb->m_PcbFrame );

    return count;
}

// Sort function to build filled zones
static bool SortByXValues( const int& a, const int &b)
{
    return a < b;
}

/***********************************************************************************/
int ZONE_CONTAINER::Fill_Zone_Areas_With_Segments( WinEDA_PcbFrame* aFrame )
/***********************************************************************************/

/** Function Fill_Zone_Areas_With_Segments()
 *  Fill sub areas in a zone with segments with m_ZoneMinThickness width
 * A scan is made line per line, on the whole filled areas, with a step of m_ZoneMinThickness.
 * all intersecting points with the horizontal infinite line and polygons to fill are calculated
 * a list of SEGZONE items is built, line per line
 * @param aFrame = reference to the main frame
 * @return number of segments created
 */
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
    istart = 0;
    int end_list =  m_FilledPolysList.size()-1;
    for( int ic = 0; ic <= end_list; ic++ )
    {
        CPolyPt* corner = &m_FilledPolysList[ic];
        if ( corner->end_contour || (ic == end_list) )
        {
            iend = ic;
            EDA_Rect rect = CalculateSubAreaBoundaryBox( istart, iend );

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
                
                if ( error ) break;
                int iimax = x_coordinates.size()-1;
                for (int ii = 0; ii < iimax; ii +=2 )
                {
                    wxPoint  seg_start, seg_end;
                    count++;
                    seg_start.x = x_coordinates[ii];
                    seg_start.y = refy;
                    seg_end.x = x_coordinates[ii+1];
                    seg_end.y = refy;
                    SEGZONE* segment = new SEGZONE( aFrame->m_Pcb );
                    segment->m_Start = seg_start;
                    segment->m_End   = seg_end;
                    segment->SetNet( GetNet() );
                    segment->m_TimeStamp = m_TimeStamp;
                    segment->m_Width = m_ZoneMinThickness;
                    segment->SetLayer( GetLayer() );
                    aFrame->m_Pcb->Add( segment );
                }
            }   //End examine segments in one area
            if ( error ) break;
            istart = iend + 1;  // istart points the first corner of the next area
        }   // End find one end of outline
        if ( error ) break;
    }   // End examine all areas

    return count;
}



/********************************************/
int Propagation( WinEDA_PcbFrame* frame )
/********************************************/

/** Function Propagation()
 * An important function to calculate zones
 * Uses the routing matrix to fill the cells within the zone
 * Search and mark cells within the zone, and agree with DRC options.
 * Requirements:
 * Start from an initial point, to fill zone
 * The zone must have no "copper island"
 *  Algorithm:
 *  If the current cell has a neightbour flagged as "cell in the zone", it
 *  become a cell in the zone
 *  The first point in the zone is the starting point
 *  4 searches within the matrix are made:
 *          1 - Left to right and top to bottom
 *          2 - Right to left and top to bottom
 *          3 - bottom to top and Right to left
 *          4 - bottom to top and Left to right
 *  Given the current cell, for each search, we consider the 2 neightbour cells
 *  the previous cell on the same line and the previous cell on the same column.
 *
 *  This funtion can request some iterations
 *  Iterations are made until no cell is added to the zone.
 *  @return: added cells count (i.e. which the attribute CELL_is_ZONE is set)
 */
{
    int       row, col, nn;
    long      current_cell, old_cell_H;
    int long* pt_cell_V;
    int       nbpoints = 0;

#define NO_CELL_ZONE (HOLE | CELL_is_EDGE | CELL_is_ZONE)
    wxString  msg;

    Affiche_1_Parametre( frame, 57, wxT( "Detect" ), msg, CYAN );
    Affiche_1_Parametre( frame, -1, wxEmptyString, wxT( "1" ), CYAN );

    // Alloc memory to handle 1 line or 1 colunmn on the routing matrix
    nn = MAX( Nrows, Ncols ) * sizeof(*pt_cell_V);
    pt_cell_V = (long*) MyMalloc( nn );

    /* search 1 : from left to right and top to bottom */
    memset( pt_cell_V, 0, nn );
    for( row = 0; row < Nrows; row++ )
    {
        old_cell_H = 0;
        for( col = 0; col < Ncols; col++ )
        {
            current_cell = GetCell( row, col, BOTTOM ) & NO_CELL_ZONE;
            if( current_cell == 0 )  /* a free cell is found */
            {
                if( (old_cell_H & CELL_is_ZONE)
                   || (pt_cell_V[col] & CELL_is_ZONE) )
                {
                    OrCell( row, col, BOTTOM, CELL_is_ZONE );
                    current_cell = CELL_is_ZONE;
                    nbpoints++;
                }
            }
            pt_cell_V[col] = old_cell_H = current_cell;
        }
    }

    /* search 2 : from right to left and top to bottom */
    Affiche_1_Parametre( frame, -1, wxEmptyString, wxT( "2" ), CYAN );
    memset( pt_cell_V, 0, nn );
    for( row = 0; row < Nrows; row++ )
    {
        old_cell_H = 0;
        for( col = Ncols - 1; col >= 0; col-- )
        {
            current_cell = GetCell( row, col, BOTTOM ) & NO_CELL_ZONE;
            if( current_cell == 0 )  /* a free cell is found */
            {
                if( (old_cell_H & CELL_is_ZONE)
                   || (pt_cell_V[col] & CELL_is_ZONE) )
                {
                    OrCell( row, col, BOTTOM, CELL_is_ZONE );
                    current_cell = CELL_is_ZONE;
                    nbpoints++;
                }
            }
            pt_cell_V[col] = old_cell_H = current_cell;
        }
    }

    /* search 3 : from bottom to top and right to left balayage */
    Affiche_1_Parametre( frame, -1, wxEmptyString, wxT( "3" ), CYAN );
    memset( pt_cell_V, 0, nn );
    for( col = Ncols - 1; col >= 0; col-- )
    {
        old_cell_H = 0;
        for( row = Nrows - 1; row >= 0; row-- )
        {
            current_cell = GetCell( row, col, BOTTOM ) & NO_CELL_ZONE;
            if( current_cell == 0 )  /* a free cell is found */
            {
                if( (old_cell_H & CELL_is_ZONE)
                   || (pt_cell_V[row] & CELL_is_ZONE) )
                {
                    OrCell( row, col, BOTTOM, CELL_is_ZONE );
                    current_cell = CELL_is_ZONE;
                    nbpoints++;
                }
            }
            pt_cell_V[row] = old_cell_H = current_cell;
        }
    }

    /* search 4 : from bottom to top and left to right */
    Affiche_1_Parametre( frame, -1, wxEmptyString, wxT( "4" ), CYAN );
    memset( pt_cell_V, 0, nn );
    for( col = 0; col < Ncols; col++ )
    {
        old_cell_H = 0;
        for( row = Nrows - 1; row >= 0; row-- )
        {
            current_cell = GetCell( row, col, BOTTOM ) & NO_CELL_ZONE;
            if( current_cell == 0 )  /* a free cell is found */
            {
                if( (old_cell_H & CELL_is_ZONE)
                   || (pt_cell_V[row] & CELL_is_ZONE) )
                {
                    OrCell( row, col, BOTTOM, CELL_is_ZONE );
                    current_cell = CELL_is_ZONE;
                    nbpoints++;
                }
            }
            pt_cell_V[row] = old_cell_H = current_cell;
        }
    }

    MyFree( pt_cell_V );

    return nbpoints;
}

