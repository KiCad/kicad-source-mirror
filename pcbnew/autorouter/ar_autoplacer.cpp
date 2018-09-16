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
#include <class_drawpanel.h>
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

#include <connectivity_data.h>
#include <ratsnest_data.h>

#include <widgets/progress_reporter.h>

#include "ar_matrix.h"
#include "ar_cell.h"
#include "ar_autoplacer.h"

#define AR_GAIN            16
#define AR_KEEPOUT_MARGIN  500
#define AR_ABORT_PLACEMENT -1

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
    m_connectivity.reset( new CONNECTIVITY_DATA );

    for( auto mod : m_board->Modules() )
        m_connectivity->Add( mod );

    m_gridSize = Millimeter2iu( 0.5 );
    m_progressReporter = nullptr;
    m_refreshCallback = nullptr;
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
    {
        //DisplayError( NULL, _( "No PCB edge found, unknown board size!" ) );
        // fixme: no wx here
        return 0;
    }

    m_matrix.ComputeMatrixSize( bbox );
    int nbCells = m_matrix.m_Ncols * m_matrix.m_Nrows;

    // Choose the number of board sides.
    m_matrix.m_RoutingLayersCount = 2;
    m_matrix.InitRoutingMatrix();

    m_matrix.m_routeLayerBottom = F_Cu;

    if( m_matrix.m_RoutingLayersCount > 1 )
        m_matrix.m_routeLayerBottom = B_Cu;

    m_matrix.m_routeLayerTop = F_Cu;

    // Place the edge layer segments
    TRACK tmp( NULL );

    tmp.SetLayer( UNDEFINED_LAYER );
    tmp.SetNetCode( -1 );
    tmp.SetWidth( m_matrix.m_GridRouting / 2 );

    for( auto drawing : m_board->Drawings() )
    {
        DRAWSEGMENT* DrawSegm;

        switch( drawing->Type() )
        {
        case PCB_LINE_T:
            DrawSegm = (DRAWSEGMENT*) drawing;

            if( DrawSegm->GetLayer() != Edge_Cuts )
                break;


            //printf("addSeg %p grid %d\n", DrawSegm,  m_matrix.m_GridRouting );
            m_matrix.TraceSegmentPcb( DrawSegm, CELL_IS_HOLE | CELL_IS_EDGE,
                             m_matrix.m_GridRouting, AR_MATRIX::WRITE_CELL );
            break;

        case PCB_TEXT_T:
        default:
            break;
        }
    }

    // Mark cells of the routing matrix to CELL_IS_ZONE
    // (i.e. availlable cell to place a module )
    // Init a starting point of attachment to the area.
    m_matrix.OrCell( m_matrix.m_Nrows / 2, m_matrix.m_Ncols / 2,
                          AR_SIDE_BOTTOM, CELL_IS_ZONE );

    // find and mark all other availlable cells:
    for( int ii = 1; ii != 0; )
        ii = propagate();

    // Initialize top layer. to the same value as the bottom layer
    if( m_matrix.m_BoardSide[AR_SIDE_TOP] )
        memcpy( m_matrix.m_BoardSide[AR_SIDE_TOP], m_matrix.m_BoardSide[AR_SIDE_BOTTOM],
                nbCells * sizeof(AR_MATRIX::MATRIX_CELL) );

    return 1;
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


/**
 * Function propagate
 * Used only in autoplace calculations
 * Uses the routing matrix to fill the cells within the zone
 * Search and mark cells within the zone, and agree with DRC options.
 * Requirements:
 * Start from an initial point, to fill zone
 * The zone must have no "copper island"
 *  Algorithm:
 *  If the current cell has a neighbor flagged as "cell in the zone", it
 *  become a cell in the zone
 *  The first point in the zone is the starting point
 *  4 searches within the matrix are made:
 *          1 - Left to right and top to bottom
 *          2 - Right to left and top to bottom
 *          3 - bottom to top and Right to left
 *          4 - bottom to top and Left to right
 *  Given the current cell, for each search, we consider the 2 neighbor cells
 *  the previous cell on the same line and the previous cell on the same column.
 *
 *  This function can request some iterations
 *  Iterations are made until no cell is added to the zone.
 *  @return added cells count (i.e. which the attribute CELL_IS_ZONE is set)
 */

int AR_AUTOPLACER::propagate()
{
    int     row, col;
    long    current_cell, old_cell_H;
    std::vector<int> pt_cell_V;
    int     nbpoints = 0;

    const uint32_t NO_CELL_ZONE = CELL_IS_HOLE | CELL_IS_EDGE | CELL_IS_ZONE;

    pt_cell_V.resize( std::max( m_matrix.m_Nrows, m_matrix.m_Ncols ), CELL_IS_EMPTY );

    // Search from left to right and top to bottom.
    for( row = 0; row < m_matrix.m_Nrows; row++ )
    {
        old_cell_H = 0;

        for( col = 0; col < m_matrix.m_Ncols; col++ )
        {
            current_cell = m_matrix.GetCell( row, col, AR_SIDE_BOTTOM ) & NO_CELL_ZONE;

            if( current_cell == 0 )    // a free cell is found
            {
                if( (old_cell_H & CELL_IS_ZONE) || (pt_cell_V[col] & CELL_IS_ZONE) )
                {
                    m_matrix.OrCell( row, col, AR_SIDE_BOTTOM, CELL_IS_ZONE );
                    current_cell = CELL_IS_ZONE;
                    nbpoints++;
                }
            }

            pt_cell_V[col] = old_cell_H = current_cell;
        }
    }

    // Search from right to left and top to bottom/
    fill( pt_cell_V.begin(), pt_cell_V.end(), CELL_IS_EMPTY );

    for( row = 0; row < m_matrix.m_Nrows; row++ )
    {
        old_cell_H = 0;

        for( col = m_matrix.m_Ncols - 1; col >= 0; col-- )
        {
            current_cell = m_matrix.GetCell( row, col, AR_SIDE_BOTTOM ) & NO_CELL_ZONE;

            if( current_cell == 0 )    // a free cell is found
            {
                if( (old_cell_H & CELL_IS_ZONE) || (pt_cell_V[col] & CELL_IS_ZONE) )
                {
                    m_matrix.OrCell( row, col, AR_SIDE_BOTTOM, CELL_IS_ZONE );
                    current_cell = CELL_IS_ZONE;
                    nbpoints++;
                }
            }

            pt_cell_V[col] = old_cell_H = current_cell;
        }
    }

    // Search from bottom to top and right to left.
    fill( pt_cell_V.begin(), pt_cell_V.end(), CELL_IS_EMPTY );

    for( col = m_matrix.m_Ncols - 1; col >= 0; col-- )
    {
        old_cell_H = 0;

        for( row = m_matrix.m_Nrows - 1; row >= 0; row-- )
        {
            current_cell = m_matrix.GetCell( row, col, AR_SIDE_BOTTOM ) & NO_CELL_ZONE;

            if( current_cell == 0 )    // a free cell is found
            {
                if( (old_cell_H & CELL_IS_ZONE) || (pt_cell_V[row] & CELL_IS_ZONE) )
                {
                    m_matrix.OrCell( row, col, AR_SIDE_BOTTOM, CELL_IS_ZONE );
                    current_cell = CELL_IS_ZONE;
                    nbpoints++;
                }
            }

            pt_cell_V[row] = old_cell_H = current_cell;
        }
    }

    // Search from bottom to top and left to right.
    fill( pt_cell_V.begin(), pt_cell_V.end(), CELL_IS_EMPTY );

    for( col = 0; col < m_matrix.m_Ncols; col++ )
    {
        old_cell_H = 0;

        for( row = m_matrix.m_Nrows - 1; row >= 0; row-- )
        {
            current_cell = m_matrix.GetCell( row, col, AR_SIDE_BOTTOM ) & NO_CELL_ZONE;

            if( current_cell == 0 )    // a free cell is found
            {
                if( (old_cell_H & CELL_IS_ZONE) || (pt_cell_V[row] & CELL_IS_ZONE) )
                {
                    m_matrix.OrCell( row, col, AR_SIDE_BOTTOM, CELL_IS_ZONE );
                    current_cell = CELL_IS_ZONE;
                    nbpoints++;
                }
            }

            pt_cell_V[row] = old_cell_H = current_cell;
        }
    }

    return nbpoints;
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

    int         diag = testRectangle( fpBBox, side );

    if( diag != AR_FREE_CELL )
        return diag;

    if( TstOtherSide )
    {
        diag = testRectangle( fpBBox, otherside );

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
        if ( m_refreshCallback )
        {
            if ( m_refreshCallback() == AR_ABORT_PLACEMENT )
                return AR_ABORT_PLACEMENT;
        }

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

            //printf("Dist %lld pad %p\n", dist, pad );

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


/**
 * Function Module
 * find the "best" module place
 * The criteria are:
 * - Maximum ratsnest with modules already placed
 * - Max size, and number of pads max
 */
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
    int         ii, jj;
    COLOR4D     color;
    int         ox, oy;
    AR_MATRIX::MATRIX_CELL top_state, bottom_state;


    for( ii = 0; ii < m_matrix.m_Nrows; ii++ )
    {
        oy = m_matrix.m_BrdBox.GetY() + ( ii * m_matrix.m_GridRouting );

        for( jj = 0; jj < m_matrix.m_Ncols; jj++ )
        {
            ox      = m_matrix.m_BrdBox.GetX() + (jj * m_matrix.m_GridRouting);
            color   = COLOR4D::BLACK;

            top_state       = m_matrix.GetCell( ii, jj, AR_SIDE_TOP );
            bottom_state    = m_matrix.GetCell( ii, jj, AR_SIDE_BOTTOM );

            if(top_state || bottom_state)
            {
             //   printf("[%d, %d] [%d, %d] TS %x BS %x\n",ii,jj, ox, oy, top_state, bottom_state );
            }

            if( top_state & CELL_IS_ZONE )
                color = COLOR4D( BLUE );

            // obstacles
            if( ( top_state & CELL_IS_EDGE ) || ( bottom_state & CELL_IS_EDGE ) )
                color = COLOR4D::WHITE;
            else if( top_state & ( CELL_IS_HOLE | CELL_IS_MODULE ) )
                color = COLOR4D( LIGHTRED );
            else if( bottom_state & ( CELL_IS_HOLE | CELL_IS_MODULE) )
                color = COLOR4D( LIGHTGREEN );
            else    // Display the filling and keep out regions.
            {
                if( m_matrix.GetDist( ii, jj, AR_SIDE_TOP )
                    || m_matrix.GetDist( ii, jj, AR_SIDE_BOTTOM ) )
                    color = DARKGRAY;
            }

            m_overlay->SetIsFill(true);
            m_overlay->SetFillColor( color );

            VECTOR2D p(ox, oy);
            m_overlay->Circle(p, m_matrix.m_GridRouting/4 );
        }
    }
}

AR_RESULT AR_AUTOPLACER::AutoplaceModules( std::vector<MODULE*> aModules, BOARD_COMMIT* aCommit, bool aPlaceOffboardModules )
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
        {
            moduleCount++;
        }
        else
        {
            genModuleOnRoutingMatrix( m );
        }
    }

    drawPlacementRoutingMatrix();

    int         cnt = 0;
    wxString    msg;

    if( m_progressReporter )
    {
        m_progressReporter->Report( _( "Autoplacing components..." ) );
        m_progressReporter->SetMaxProgress( moduleCount );
    }

    while( ( module = pickModule( ) ) != nullptr )
    {
        // Display some info about activity, module placement can take a while:
        //printf( _( "Place footprint %d of %d [%s]\n" ), cnt, moduleCount, (const char *)module->GetReference().c_str() );
        //m_frame->SetStatusText( msg );

        double initialOrient = module->GetOrientation();
        // Display fill area of interest, barriers, penalties.
        //drawPlacementRoutingMatrix( );

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
