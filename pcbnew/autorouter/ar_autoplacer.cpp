/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 Jean-Pierre Charras, jean-pierre.charras@ujf-grenoble.fr
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2011 Wayne Stambaugh <stambaughw@verizon.net>
 *
 * Copyright (C) 1992-2012 KiCad Developers, see change_log.txt for contributors.
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

#include <fctsys.h>
#include <confirm.h>
#include <pcbnew.h>
#include <pcb_edit_frame.h>
#include <gr_basic.h>
#include <macros.h>
#include <msgpanel.h>
#include <class_board.h>
#include <class_module.h>
#include <class_track.h>
#include <class_drawsegment.h>
#include <class_pad.h>
#include <board_commit.h>
#include <connectivity/connectivity_data.h>
#include <ratsnest_data.h>
#include <widgets/progress_reporter.h>

#include "ar_autoplacer.h"
#include "ar_cell.h"
#include "ar_matrix.h"
#include <memory>

#define AR_GAIN            16
#define AR_KEEPOUT_MARGIN  500
#define AR_ABORT_PLACEMENT -1

#define STEP_AR_MM 1.0

/* Penalty (cost) for CntRot90 and CntRot180:
 * CntRot90 and CntRot180 are from 0 (rotation allowed) to 10 (rotation not allowed)
 */
static const double OrientationPenalty[11] =
{
    2.0,        // CntRot = 0 rotation prohibited
    1.9,        // CntRot = 1
    1.8,        // CntRot = 2
    1.7,        // CntRot = 3
    1.6,        // CntRot = 4
    1.5,        // CntRot = 5
    1.4,        // CntRot = 5
    1.3,        // CntRot = 7
    1.2,        // CntRot = 8
    1.1,        // CntRot = 9
    1.0         // CntRot = 10 rotation authorized, no penalty
};


AR_AUTOPLACER::AR_AUTOPLACER( BOARD* aBoard )
{
    m_board = aBoard;
    m_connectivity = std::make_unique<CONNECTIVITY_DATA>( );

    for( auto mod : m_board->Modules() )
        m_connectivity->Add( mod );

    m_gridSize = Millimeter2iu( STEP_AR_MM );
    m_progressReporter = nullptr;
    m_refreshCallback = nullptr;
    m_minCost = 0.0;
}


void AR_AUTOPLACER::placeModule( MODULE* aModule, bool aDoNotRecreateRatsnest, const wxPoint& aPos )
{
    if( !aModule )
        return;

    aModule->SetPosition( aPos );
    m_connectivity->Update( aModule );
}


int AR_AUTOPLACER::genPlacementRoutingMatrix()
{
    m_matrix.UnInitRoutingMatrix();

    EDA_RECT bbox = m_board->GetBoardEdgesBoundingBox();

    if( bbox.GetWidth() == 0 || bbox.GetHeight() == 0 )
        return 0;

    // Build the board shape
    m_board->GetBoardPolygonOutlines( m_boardShape /*, aErrorText, aErrorLocation*/ );
    m_topFreeArea = m_boardShape;
    m_bottomFreeArea = m_boardShape;

    m_matrix.ComputeMatrixSize( bbox );
    int nbCells = m_matrix.m_Ncols * m_matrix.m_Nrows;

    // Choose the number of board sides.
    m_matrix.m_RoutingLayersCount = 2;
    m_matrix.InitRoutingMatrix();
    m_matrix.m_routeLayerBottom = B_Cu;
    m_matrix.m_routeLayerTop = F_Cu;

    // Fill (mark) the cells inside the board:
    fillMatrix();

    // Other obstacles can be added here:
    for( auto drawing : m_board->Drawings() )
    {
        switch( drawing->Type() )
        {
        case PCB_LINE_T:
            if( drawing->GetLayer() != Edge_Cuts )
            {
                m_matrix.TraceSegmentPcb( (DRAWSEGMENT*)drawing, CELL_IS_HOLE | CELL_IS_EDGE,
                                          m_matrix.m_GridRouting, AR_MATRIX::WRITE_CELL );
            }
            break;

        default:
            break;
        }
    }

    // Initialize top layer. to the same value as the bottom layer
    if( m_matrix.m_BoardSide[AR_SIDE_TOP] )
        memcpy( m_matrix.m_BoardSide[AR_SIDE_TOP], m_matrix.m_BoardSide[AR_SIDE_BOTTOM],
                nbCells * sizeof(AR_MATRIX::MATRIX_CELL) );

    return 1;
}


bool AR_AUTOPLACER::fillMatrix()
{
    std::vector <int> x_coordinates;
    bool success = true;
    int step = m_matrix.m_GridRouting;
    wxPoint coord_orgin = m_matrix.GetBrdCoordOrigin(); // Board coordinate of matruix cell (0,0)

    // Create a single board outline:
    SHAPE_POLY_SET brd_shape = m_boardShape;
    brd_shape.Fracture( SHAPE_POLY_SET::PM_FAST );
    const SHAPE_LINE_CHAIN& outline = brd_shape.Outline(0);
    const BOX2I& rect = outline.BBox();

    // Creates the horizontal segments
    // Calculate the y limits of the area
    for( int refy = rect.GetY(), endy = rect.GetBottom(); refy < endy; refy += step )
    {
        // The row index (vertical position) of current line scan inside the placement matrix
        int idy = (refy - coord_orgin.y) / step;

        // Ensure we are still inside the placement matrix
        if( idy >= m_matrix.m_Nrows )
            break;

        // Ensure we are inside the placement matrix
        if( idy <= 0 )
            continue;

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
        std::sort( x_coordinates.begin(), x_coordinates.end() );

        // An even number of coordinates is expected, because a segment has 2 ends.
        // An if this algorithm always works, it must always find an even count.
        if( ( x_coordinates.size() & 1 ) != 0 )
        {
            success = false;
            break;
        }

        // Fill cells having the same Y coordinate
        int iimax = x_coordinates.size() - 1;

        for( int ii = 0; ii < iimax; ii += 2 )
        {
            int seg_start_x = x_coordinates[ii] - coord_orgin.x;
            int seg_end_x = x_coordinates[ii + 1] - coord_orgin.x;
            // Fill cells at y coord = idy,
            // and at x cood >= seg_start_x and <= seg_end_x

            for( int idx = seg_start_x / step; idx < m_matrix.m_Ncols; idx++ )
            {
                if( idx * step > seg_end_x )
                    break;

                if( idx * step >= seg_start_x )
                    m_matrix.SetCell( idy, idx, AR_SIDE_BOTTOM, CELL_IS_ZONE );
            }

        }
    }   // End examine segments in one area

    return success;
}



void AR_AUTOPLACER::rotateModule( MODULE* module, double angle, bool incremental )
{
    if( module == NULL )
        return;

    if( incremental )
        module->SetOrientation( module->GetOrientation() + angle );
    else
        module->SetOrientation( angle );


    m_board->GetConnectivity()->Update( module );
}


void AR_AUTOPLACER::addFpBody( wxPoint aStart, wxPoint aEnd, LSET aLayerMask )
{
    // Add a polygonal shape (rectangle) to m_fpAreaFront and/or m_fpAreaBack
    if( aLayerMask[ F_Cu ] )
    {
        m_fpAreaTop.NewOutline();
        m_fpAreaTop.Append( aStart.x, aStart.y );
        m_fpAreaTop.Append( aEnd.x, aStart.y );
        m_fpAreaTop.Append( aEnd.x, aEnd.y );
        m_fpAreaTop.Append( aStart.x, aEnd.y );
    }
    if( aLayerMask[ B_Cu ] )
    {
        m_fpAreaBottom.NewOutline();
        m_fpAreaBottom.Append( aStart.x, aStart.y );
        m_fpAreaBottom.Append( aEnd.x, aStart.y );
        m_fpAreaBottom.Append( aEnd.x, aEnd.y );
        m_fpAreaBottom.Append( aStart.x, aEnd.y );
    }
}

void AR_AUTOPLACER::addPad( D_PAD* aPad, int aClearance )
{
    // Add a polygonal shape (rectangle) to m_fpAreaFront and/or m_fpAreaBack
    EDA_RECT bbox = aPad->GetBoundingBox();
    bbox.Inflate( aClearance );

    if( aPad->IsOnLayer( F_Cu ) )
    {
        m_fpAreaTop.NewOutline();
        m_fpAreaTop.Append( bbox.GetLeft(), bbox.GetTop() );
        m_fpAreaTop.Append( bbox.GetRight(), bbox.GetTop() );
        m_fpAreaTop.Append( bbox.GetRight(), bbox.GetBottom() );
        m_fpAreaTop.Append( bbox.GetLeft(), bbox.GetBottom() );
    }
    if( aPad->IsOnLayer( B_Cu ) )
    {
        m_fpAreaBottom.NewOutline();
        m_fpAreaBottom.Append( bbox.GetLeft(), bbox.GetTop() );
        m_fpAreaBottom.Append( bbox.GetRight(), bbox.GetTop() );
        m_fpAreaBottom.Append( bbox.GetRight(), bbox.GetBottom() );
        m_fpAreaBottom.Append( bbox.GetLeft(), bbox.GetBottom() );
    }
}


void AR_AUTOPLACER::buildFpAreas( MODULE* aFootprint, int aFpClearance )
{
    m_fpAreaTop.RemoveAllContours();
    m_fpAreaBottom.RemoveAllContours();

    if( aFootprint->BuildPolyCourtyard() )
    {
        m_fpAreaTop = aFootprint->GetPolyCourtyardFront();
        m_fpAreaBottom = aFootprint->GetPolyCourtyardBack();
    }

    LSET        layerMask;

    if( aFootprint->GetLayer() == F_Cu )
        layerMask.set( F_Cu );

    if( aFootprint->GetLayer() == B_Cu )
        layerMask.set( B_Cu );

    EDA_RECT    fpBBox = aFootprint->GetBoundingBox();

    fpBBox.Inflate( ( m_matrix.m_GridRouting / 2 ) + aFpClearance );

    // Add a minimal area to the fp area:
    addFpBody( fpBBox.GetOrigin(), fpBBox.GetEnd(), layerMask );

    // Trace pads + clearance areas.
    for( auto pad : aFootprint->Pads() )
    {
        int margin = (m_matrix.m_GridRouting / 2) + pad->GetClearance();
        addPad( pad, margin );
    }
}


void AR_AUTOPLACER::genModuleOnRoutingMatrix( MODULE* Module )
{
    int         ox, oy, fx, fy;
    LSET        layerMask;
    EDA_RECT    fpBBox = Module->GetBoundingBox();

    fpBBox.Inflate( m_matrix.m_GridRouting / 2 );
    ox  = fpBBox.GetX();
    fx  = fpBBox.GetRight();
    oy  = fpBBox.GetY();
    fy  = fpBBox.GetBottom();

    if( ox < m_matrix.m_BrdBox.GetX() )
        ox = m_matrix.m_BrdBox.GetX();

    if( ox > m_matrix.m_BrdBox.GetRight() )
        ox = m_matrix.m_BrdBox.GetRight();

    if( fx < m_matrix.m_BrdBox.GetX() )
        fx = m_matrix.m_BrdBox.GetX();

    if( fx > m_matrix.m_BrdBox.GetRight() )
        fx = m_matrix.m_BrdBox.GetRight();

    if( oy < m_matrix.m_BrdBox.GetY() )
        oy = m_matrix.m_BrdBox.GetY();

    if( oy > m_matrix.m_BrdBox.GetBottom() )
        oy = m_matrix.m_BrdBox.GetBottom();

    if( fy < m_matrix.m_BrdBox.GetY() )
        fy = m_matrix.m_BrdBox.GetY();

    if( fy > m_matrix.m_BrdBox.GetBottom() )
        fy = m_matrix.m_BrdBox.GetBottom();

    if( Module->GetLayer() == F_Cu )
        layerMask.set( F_Cu );

    if( Module->GetLayer() == B_Cu )
        layerMask.set( B_Cu );

    m_matrix.TraceFilledRectangle( ox, oy, fx, fy, layerMask,
                          CELL_IS_MODULE, AR_MATRIX::WRITE_OR_CELL );

    // Trace pads + clearance areas.
    for( auto pad : Module->Pads() )
    {
        int margin = (m_matrix.m_GridRouting / 2) + pad->GetClearance();
        m_matrix.PlacePad( pad, CELL_IS_MODULE, margin, AR_MATRIX::WRITE_OR_CELL );
    }

    // Trace clearance.
    int margin = ( m_matrix.m_GridRouting * Module->GetPadCount() ) / AR_GAIN;
    m_matrix.CreateKeepOutRectangle( ox, oy, fx, fy, margin, AR_KEEPOUT_MARGIN , layerMask );

    // Build the footprint courtyard
    buildFpAreas( Module, margin );

    // Substract the shape to free areas
    m_topFreeArea.BooleanSubtract( m_fpAreaTop, SHAPE_POLY_SET::PM_FAST );
    m_bottomFreeArea.BooleanSubtract( m_fpAreaBottom, SHAPE_POLY_SET::PM_FAST );
}


/* Test if the rectangular area (ux, ux .. y0, y1):
 * - is a free zone (except OCCUPED_By_MODULE returns)
 * - is on the working surface of the board (otherwise returns OUT_OF_BOARD)
 *
 * Returns OUT_OF_BOARD, or OCCUPED_By_MODULE or FREE_CELL if OK
 */
int AR_AUTOPLACER::testRectangle( const EDA_RECT& aRect, int side )
{
    EDA_RECT rect = aRect;

    rect.Inflate( m_matrix.m_GridRouting / 2 );

    wxPoint start   = rect.GetOrigin();
    wxPoint end     = rect.GetEnd();

    start   -= m_matrix.m_BrdBox.GetOrigin();
    end     -= m_matrix.m_BrdBox.GetOrigin();

    int row_min = start.y / m_matrix.m_GridRouting;
    int row_max = end.y / m_matrix.m_GridRouting;
    int col_min = start.x / m_matrix.m_GridRouting;
    int col_max = end.x / m_matrix.m_GridRouting;

    if( start.y > row_min * m_matrix.m_GridRouting )
        row_min++;

    if( start.x > col_min * m_matrix.m_GridRouting )
        col_min++;

    if( row_min < 0 )
        row_min = 0;

    if( row_max >= ( m_matrix.m_Nrows - 1 ) )
        row_max = m_matrix.m_Nrows - 1;

    if( col_min < 0 )
        col_min = 0;

    if( col_max >= ( m_matrix.m_Ncols - 1 ) )
        col_max = m_matrix.m_Ncols - 1;

    for( int row = row_min; row <= row_max; row++ )
    {
        for( int col = col_min; col <= col_max; col++ )
        {
            unsigned int data = m_matrix.GetCell( row, col, side );

            if( ( data & CELL_IS_ZONE ) == 0 )
                return AR_OUT_OF_BOARD;

            if( (data & CELL_IS_MODULE) )
                return AR_OCCUIPED_BY_MODULE;
        }
    }

    return AR_FREE_CELL;
}

int AR_AUTOPLACER::testModuleByPolygon( MODULE* aModule, int aSide, const wxPoint& aOffset )
{
    // Test for footprint out of board:
    // If a footprint is not fully inside the board, substract board polygon
    // to the footprint polygon gives a non null area.
    SHAPE_POLY_SET fp_area = m_fpAreaTop;
    fp_area.Move( -aOffset );
    SHAPE_POLY_SET out_of_board_area;
    out_of_board_area.BooleanSubtract( fp_area, m_topFreeArea, SHAPE_POLY_SET::PM_FAST );

    if( out_of_board_area.OutlineCount() )
        return AR_OCCUIPED_BY_MODULE;

    return AR_FREE_CELL;
}


/* Calculates and returns the clearance area of the rectangular surface
 * aRect):
 * (Sum of cells in terms of distance)
 */
unsigned int AR_AUTOPLACER::calculateKeepOutArea( const EDA_RECT& aRect, int side )
{
    wxPoint start   = aRect.GetOrigin();
    wxPoint end     = aRect.GetEnd();

    start   -= m_matrix.m_BrdBox.GetOrigin();
    end     -= m_matrix.m_BrdBox.GetOrigin();

    int row_min = start.y / m_matrix.m_GridRouting;
    int row_max = end.y / m_matrix.m_GridRouting;
    int col_min = start.x / m_matrix.m_GridRouting;
    int col_max = end.x / m_matrix.m_GridRouting;

    if( start.y > row_min * m_matrix.m_GridRouting )
        row_min++;

    if( start.x > col_min * m_matrix.m_GridRouting )
        col_min++;

    if( row_min < 0 )
        row_min = 0;

    if( row_max >= ( m_matrix.m_Nrows - 1 ) )
        row_max = m_matrix.m_Nrows - 1;

    if( col_min < 0 )
        col_min = 0;

    if( col_max >= ( m_matrix.m_Ncols - 1 ) )
        col_max = m_matrix.m_Ncols - 1;

    unsigned int keepOutCost = 0;

    for( int row = row_min; row <= row_max; row++ )
    {
        for( int col = col_min; col <= col_max; col++ )
        {
            // m_matrix.GetDist returns the "cost" of the cell
            // at position (row, col)
            // in autoplace this is the cost of the cell, if it is
            // inside aRect
            keepOutCost += m_matrix.GetDist( row, col, side );
        }
    }

    return keepOutCost;
}


/* Test if the module can be placed on the board.
 * Returns the value TstRectangle().
 * Module is known by its bounding box
 */
int AR_AUTOPLACER::testModuleOnBoard( MODULE* aModule, bool TstOtherSide, const wxPoint& aOffset )
{
    int side = AR_SIDE_TOP;
    int otherside = AR_SIDE_BOTTOM;

    if( aModule->GetLayer() == B_Cu )
    {
        side = AR_SIDE_BOTTOM; otherside = AR_SIDE_TOP;
    }

    EDA_RECT    fpBBox = aModule->GetFootprintRect();
    fpBBox.Move( -aOffset );

    buildFpAreas( aModule, 0 );

    int diag = //testModuleByPolygon( aModule, side, aOffset );
        testRectangle( fpBBox, side );
//printf("test %p diag %d\n", aModule, diag);fflush(0);
    if( diag != AR_FREE_CELL )
        return diag;

    if( TstOtherSide )
    {
        diag = //testModuleByPolygon( aModule, otherside, aOffset );
                testRectangle( fpBBox, otherside );

        if( diag != AR_FREE_CELL )
            return diag;
    }

    int marge = ( m_matrix.m_GridRouting * aModule->GetPadCount() ) / AR_GAIN;

    fpBBox.Inflate( marge );
    return calculateKeepOutArea( fpBBox, side );
}


int AR_AUTOPLACER::getOptimalModulePlacement(MODULE* aModule)
{
    int     error = 1;
    wxPoint LastPosOK;
    double  min_cost, curr_cost, Score;
    bool    TstOtherSide;

    aModule->CalculateBoundingBox();

    LastPosOK = m_matrix.m_BrdBox.GetOrigin();

    wxPoint     mod_pos = aModule->GetPosition();
    EDA_RECT    fpBBox  = aModule->GetFootprintRect();

    // Move fpBBox to have the footprint position at (0,0)
    fpBBox.Move( -mod_pos );
    wxPoint fpBBoxOrg = fpBBox.GetOrigin();

    // Calculate the limit of the footprint position, relative
    // to the routing matrix area
    wxPoint xylimit = m_matrix.m_BrdBox.GetEnd() - fpBBox.GetEnd();

    wxPoint initialPos = m_matrix.m_BrdBox.GetOrigin() - fpBBoxOrg;

    // Stay on grid.
    initialPos.x    -= initialPos.x % m_matrix.m_GridRouting;
    initialPos.y    -= initialPos.y % m_matrix.m_GridRouting;

    m_curPosition = initialPos;
    auto moduleOffset = mod_pos - m_curPosition;

    /* Examine pads, and set TstOtherSide to true if a footprint
     * has at least 1 pad through.
     */
    TstOtherSide = false;

    if( m_matrix.m_RoutingLayersCount > 1 )
    {
        LSET    other( aModule->GetLayer() == B_Cu  ? F_Cu : B_Cu );

        for( auto pad : aModule->Pads() )
        {
            if( !( pad->GetLayerSet() & other ).any() )
                continue;

            TstOtherSide = true;
            break;
        }
    }

    fpBBox.SetOrigin( fpBBoxOrg + m_curPosition );

    min_cost = -1.0;
//    m_frame->SetStatusText( wxT( "Score ??, pos ??" ) );


    for( ; m_curPosition.x < xylimit.x; m_curPosition.x += m_matrix.m_GridRouting )
    {
        m_curPosition.y = initialPos.y;

        for( ; m_curPosition.y < xylimit.y; m_curPosition.y += m_matrix.m_GridRouting )
        {

            fpBBox.SetOrigin( fpBBoxOrg + m_curPosition );
            moduleOffset = mod_pos - m_curPosition;
            int keepOutCost = testModuleOnBoard( aModule, TstOtherSide, moduleOffset );

            if( keepOutCost >= 0 )    // i.e. if the module can be put here
            {
                error = 0;
                // m_frame->build_ratsnest_module( aModule ); // fixme
                curr_cost   = computePlacementRatsnestCost( aModule, moduleOffset );
                Score       = curr_cost + keepOutCost;

                if( (min_cost >= Score ) || (min_cost < 0 ) )
                {
                    LastPosOK   = m_curPosition;
                    min_cost    = Score;
                    wxString msg;
/*                    msg.Printf( wxT( "Score %g, pos %s, %s" ),
                                min_cost,
                                GetChars( ::CoordinateToString( LastPosOK.x ) ),
                                GetChars( ::CoordinateToString( LastPosOK.y ) ) );
                    m_frame->SetStatusText( msg );*/
                }
            }
        }
    }

    // Regeneration of the modified variable.
    m_curPosition = LastPosOK;

    m_minCost = min_cost;
    return error;
}


const D_PAD* AR_AUTOPLACER::nearestPad( MODULE *aRefModule, D_PAD* aRefPad, const wxPoint& aOffset)
{
    const D_PAD* nearest = nullptr;
    int64_t nearestDist = INT64_MAX;

    for ( auto mod : m_board->Modules() )
    {
        if ( mod == aRefModule )
            continue;

        if( !m_matrix.m_BrdBox.Contains( mod->GetPosition() ) )
            continue;

        for ( auto pad: mod->Pads() )
        {
            if ( pad->GetNetCode() != aRefPad->GetNetCode() || pad->GetNetCode() <= 0 )
                continue;

            auto dist = (VECTOR2I( aRefPad->GetPosition() - aOffset ) - VECTOR2I( pad->GetPosition() ) ).EuclideanNorm();

            if ( dist < nearestDist )
            {
                nearestDist = dist;
                nearest = pad;
            }
        }
    }

    return nearest;
}


double AR_AUTOPLACER::computePlacementRatsnestCost( MODULE *aModule, const wxPoint& aOffset )
{
    double  curr_cost;
    VECTOR2I start;      // start point of a ratsnest
    VECTOR2I end;        // end point of a ratsnest
    int     dx, dy;

    curr_cost = 0;

    for ( auto pad : aModule->Pads() )
    {
        auto nearest = nearestPad( aModule, pad, aOffset );

        if( !nearest )
            continue;

        //printf("pad %s nearest %s\n", (const char *)aModule->GetReference().c_str(), (const char *)nearest->GetParent()->GetReference().c_str());

        start   = VECTOR2I( pad->GetPosition() ) - VECTOR2I(aOffset);
        end     = VECTOR2I( nearest->GetPosition() );

        //m_overlay->SetIsStroke( true );
        //m_overlay->SetStrokeColor( COLOR4D(0.0, 1.0, 0.0, 1.0) );
        //m_overlay->Line( start, end );

        // Cost of the ratsnest.
        dx  = end.x - start.x;
        dy  = end.y - start.y;

        dx  = abs( dx );
        dy  = abs( dy );

        // ttry to have always dx >= dy to calculate the cost of the rastsnet
        if( dx < dy )
            std::swap( dx, dy );

        // Cost of the connection = length + penalty due to the slope
        // dx is the biggest length relative to the X or Y axis
        // the penalty is max for 45 degrees ratsnests,
        // and 0 for horizontal or vertical ratsnests.
        // For Horizontal and Vertical ratsnests, dy = 0;
        double conn_cost = hypot( dx, dy * 2.0 );
        curr_cost += conn_cost;    // Total cost = sum of costs of each connection
    }

    return curr_cost;
}


// Sort routines
static bool sortFootprintsByComplexity( MODULE* ref, MODULE* compare )
{
    double ff1, ff2;

    ff1 = ref->GetArea() * ref->GetPadCount();
    ff2 = compare->GetArea() * compare->GetPadCount();

    return ff2 < ff1;
}


static bool sortFootprintsByRatsnestSize( MODULE* ref, MODULE* compare )
{
    double ff1, ff2;

    ff1 = ref->GetArea() * ref->GetFlag();
    ff2 = compare->GetArea() * compare->GetFlag();
    return ff2 < ff1;
}


MODULE* AR_AUTOPLACER::pickModule( )
{
    MODULE* module;
    std::vector <MODULE*> moduleList;


    for( auto m : m_board->Modules() )
    {
        m->CalculateBoundingBox();
        moduleList.push_back( m );
    }

    sort( moduleList.begin(), moduleList.end(), sortFootprintsByComplexity );

    for( unsigned kk = 0; kk < moduleList.size(); kk++ )
    {
        module = moduleList[kk];
        module->SetFlag( 0 );

        if( !module->NeedsPlaced() )
            continue;

        m_connectivity->Update( module );
    }

    m_connectivity->RecalculateRatsnest();

    for( unsigned kk = 0; kk < moduleList.size(); kk++ )
    {
        module = moduleList[kk];

        auto edges = m_connectivity->GetRatsnestForComponent( module, true );

        module->SetFlag( edges.size() ) ;
    }

    sort( moduleList.begin(), moduleList.end(), sortFootprintsByRatsnestSize );

    // Search for "best" module.
    MODULE* bestModule  = nullptr;
    MODULE* altModule   = nullptr;

    for( unsigned ii = 0; ii < moduleList.size(); ii++ )
    {
        module = moduleList[ii];

        if( !module->NeedsPlaced() )
            continue;

        altModule = module;

        if( module->GetFlag() == 0 )
            continue;

        bestModule = module;
        break;
    }

    if( bestModule )
        return bestModule;
    else
        return altModule;
}


void AR_AUTOPLACER::drawPlacementRoutingMatrix( )
{
    // Draw the board free area
    m_overlay->Clear();
    m_overlay->SetIsFill( true );
    m_overlay->SetIsStroke( false );

    SHAPE_POLY_SET freeArea = m_topFreeArea;
    freeArea.Fracture( SHAPE_POLY_SET::PM_FAST );

    // Draw the free polygon areas, top side:
    if( freeArea.OutlineCount() > 0 )
    {
        m_overlay->SetIsFill( true );
        m_overlay->SetIsStroke( false );
        m_overlay->SetFillColor( COLOR4D(0.7, 0.0, 0.1, 0.2) );
        m_overlay->Polygon( freeArea );
    }

    freeArea = m_bottomFreeArea;
    freeArea.Fracture( SHAPE_POLY_SET::PM_FAST );

    // Draw the free polygon areas, bottom side:
    if( freeArea.OutlineCount() > 0 )
    {
        m_overlay->SetFillColor( COLOR4D(0.0, 0.7, 0.0, 0.2) );
        m_overlay->Polygon( freeArea );
    }
}


AR_RESULT AR_AUTOPLACER::AutoplaceModules( std::vector<MODULE*> aModules,
                                           BOARD_COMMIT* aCommit, bool aPlaceOffboardModules )
{
    wxPoint             PosOK;
    wxPoint             memopos;
    int                 error;
    MODULE* module = nullptr;
    bool cancelled = false;

    memopos = m_curPosition;

    //printf("set grid: %d\n", m_gridSize);

    m_matrix.m_GridRouting = m_gridSize; //(int) m_frame->GetScreen()->GetGridSize().x;

    // Ensure Board.m_GridRouting has a reasonable value:
    if( m_matrix.m_GridRouting < Millimeter2iu( 0.25 ) )
        m_matrix.m_GridRouting = Millimeter2iu( 0.25 );

    // Compute module parameters used in auto place
    if( genPlacementRoutingMatrix( ) == 0 )
        return AR_FAILURE;

    int moduleCount = 0;

    for ( auto m : m_board->Modules() )
    {
        m->SetNeedsPlaced( false );
    }

    std::vector<MODULE *> offboardMods;

    if( aPlaceOffboardModules )
    {
        for ( auto m : m_board->Modules() )
        {
            if( !m_matrix.m_BrdBox.Contains( m->GetPosition() ) )
            {
                offboardMods.push_back( m );
            }
        }
    }

    for ( auto m : aModules )
    {
        m->SetNeedsPlaced( true );
        aCommit->Modify(m);
    }

    for ( auto m : offboardMods )
    {
        m->SetNeedsPlaced( true );
        aCommit->Modify(m);
    }

    for ( auto m : m_board->Modules() )
    {
        if( m->NeedsPlaced() )    // Erase from screen
            moduleCount++;
        else
            genModuleOnRoutingMatrix( m );
    }


    int         cnt = 0;
    wxString    msg;

    if( m_progressReporter )
    {
        m_progressReporter->Report( _( "Autoplacing components..." ) );
        m_progressReporter->SetMaxProgress( moduleCount );
    }

    drawPlacementRoutingMatrix();

    if( m_refreshCallback )
        m_refreshCallback( nullptr );


    while( ( module = pickModule( ) ) != nullptr )
    {
        // Display some info about activity, module placement can take a while:
        //m_frame->SetStatusText( msg );

        if( m_progressReporter )
            m_progressReporter->SetTitle( wxString::Format(
                                          _( "Autoplacing %s" ), module->GetReference() ) );

        double initialOrient = module->GetOrientation();

        error = getOptimalModulePlacement( module );
        double bestScore = m_minCost;
        double bestRotation = 0.0;
        int rotAllowed;
        PosOK = m_curPosition;

        if( error == AR_ABORT_PLACEMENT )
            goto end_of_tst;

        // Try orientations 90, 180, 270 degrees from initial orientation
        rotAllowed = module->GetPlacementCost180();

        //printf("rotAllowed %d\n", rotAllowed);

        if( rotAllowed != 0 )
        {
            rotateModule( module, 1800.0, true );
            error   = getOptimalModulePlacement( module );
            m_minCost *= OrientationPenalty[rotAllowed];

            if( bestScore > m_minCost )    // This orientation is better.
            {
                PosOK       = m_curPosition;
                bestScore   = m_minCost;
                bestRotation = 1800.0;
            }
            else
            {
                rotateModule( module, initialOrient, false );
            }

            if( error == AR_ABORT_PLACEMENT )
                goto end_of_tst;
        }

        // Determine if the best orientation of a module is 90.
        rotAllowed = module->GetPlacementCost90();

        if( rotAllowed != 0 )
        {
            rotateModule( module, 900.0, true );
            error   = getOptimalModulePlacement( module );
            m_minCost *= OrientationPenalty[rotAllowed];

            if( bestScore > m_minCost )    // This orientation is better.
            {
                PosOK       = m_curPosition;
                bestScore   = m_minCost;
                bestRotation = 900.0;
            }
            else
            {
                rotateModule( module, initialOrient, false );
            }

            if( error == AR_ABORT_PLACEMENT )
                goto end_of_tst;
        }

        // Determine if the best orientation of a module is -90.
        if( rotAllowed != 0 )
        {
            rotateModule( module, 2700.0, true );
            error   = getOptimalModulePlacement( module );
            m_minCost *= OrientationPenalty[rotAllowed];

            if( bestScore > m_minCost )    // This orientation is better.
            {
                PosOK       = m_curPosition;
                bestScore   = m_minCost;
                bestRotation = 2700.0;
            }
            else
            {
                rotateModule( module, initialOrient, false );
            }

            if( error == AR_ABORT_PLACEMENT )
                goto end_of_tst;
        }

end_of_tst:

        if( error == AR_ABORT_PLACEMENT )
            break;


        bestRotation += initialOrient;

        if( bestRotation != module->GetOrientation() )
        {
            //printf("best rotation %d\n",  bestRotation );
            rotateModule( module, bestRotation, false );
        }

        // Place module.
        placeModule( module, true, m_curPosition );

        module->CalculateBoundingBox();
        genModuleOnRoutingMatrix( module );
        module->SetIsPlaced( true );
        module->SetNeedsPlaced( false );
        drawPlacementRoutingMatrix();

        if( m_refreshCallback )
            m_refreshCallback( module );


        if( m_progressReporter )
        {
            m_progressReporter->AdvanceProgress();

            if ( !m_progressReporter->KeepRefreshing( false ) )
            {
                cancelled = true;
                break;
            }
        }
        cnt++;
    }

    m_curPosition = memopos;

    m_matrix.UnInitRoutingMatrix();

    for ( auto m : m_board->Modules() )
    {
        m->CalculateBoundingBox();
    }

    return cancelled ? AR_CANCELLED : AR_COMPLETED;
}
