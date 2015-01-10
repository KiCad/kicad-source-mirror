/**
 * @file auto_place_footprints.cpp
 * @brief Functions to automatically place Footprints on a board.
 */

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
#include <wxPcbStruct.h>
#include <gr_basic.h>
#include <macros.h>
#include <msgpanel.h>

#include <autorout.h>
#include <cell.h>
#include <colors_selection.h>

#include <class_board.h>
#include <class_module.h>
#include <class_track.h>
#include <class_drawsegment.h>
#include <convert_to_biu.h>
#include <base_units.h>
#include <protos.h>


#define GAIN            16
#define KEEP_OUT_MARGIN 500


/* Penalty (cost) for CntRot90 and CntRot180:
 * CntRot90 and CntRot180 are from 0 (rotation allowed) to 10 (rotation not allowed)
 */
static const double OrientPenality[11] =
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

// Cell states.
#define OUT_OF_BOARD        -2
#define OCCUPED_By_MODULE   -1
#define FREE_CELL   0


static wxPoint  CurrPosition; // Current position of the current module placement
double          MinCout;


/* generates the Routing matrix, used to fing the best placement
 * of a footprint.
 * Allocate a "bitmap" which is an image of the real board
 * the bitmap handles:
 * - The free areas
 * - penalties (cell not occupied, but near occupied areas)
 * - cells occupied by footprints, board cutout ...
 */
int             genPlacementRoutingMatrix(  BOARD* aBrd, EDA_MSG_PANEL* messagePanel );

/* searches for the optimal position of aModule.
 * return 1 if placement impossible or 0 if OK.
 */
static int      getOptimalModulePlacement( PCB_EDIT_FRAME* aFrame,
                                           MODULE* aModule, wxDC* aDC );

/*
 * Function compute_Ratsnest_PlaceModule
 * displays the module's ratsnest during displacement, and assess the "cost"
 * of the position.
 *
 * The cost is the longest ratsnest distance with penalty for connections
 * approaching 45 degrees.
 */
static double   compute_Ratsnest_PlaceModule( BOARD* aBrd );

/* Place a footprint on the Routing matrix.
 */
void            genModuleOnRoutingMatrix( MODULE* Module );
/*
 * Displays the Placement/Routing matrix on the screen
 */
static void     drawPlacementRoutingMatrix( BOARD* aBrd, wxDC* DC );

static int      TstModuleOnBoard( BOARD* Pcb, MODULE* Module, bool TstOtherSide );

static void     CreateKeepOutRectangle( int ux0, int uy0, int ux1, int uy1,
                                        int marge, int aKeepOut, LSET aLayerMask );

static MODULE*  PickModule( PCB_EDIT_FRAME* pcbframe, wxDC* DC );
static int      propagate();

void PCB_EDIT_FRAME::AutoPlaceModule( MODULE* Module, int place_mode, wxDC* DC )
{
    MODULE*             currModule = NULL;
    wxPoint             PosOK;
    wxPoint             memopos;
    int                 error;
    LAYER_ID            lay_tmp_TOP, lay_tmp_BOTTOM;

    // Undo: init list
    PICKED_ITEMS_LIST   newList;

    newList.m_Status = UR_CHANGED;
    ITEM_PICKER         picker( NULL, UR_CHANGED );

    if( GetBoard()->m_Modules == NULL )
        return;

    m_canvas->SetAbortRequest( false );

    switch( place_mode )
    {
    case PLACE_1_MODULE:
        currModule = Module;

        if( currModule == NULL )
            return;

        currModule->SetIsPlaced( false );
        currModule->SetNeedsPlaced( false );
        break;

    case PLACE_OUT_OF_BOARD:
        break;

    case PLACE_ALL:

        if( !IsOK( this, _( "Footprints NOT LOCKED will be moved" ) ) )
            return;

        break;

    case PLACE_INCREMENTAL:

        if( !IsOK( this, _( "Footprints NOT PLACED will be moved" ) ) )
            return;

        break;
    }

    memopos = CurrPosition;
    lay_tmp_BOTTOM  = g_Route_Layer_BOTTOM;
    lay_tmp_TOP     = g_Route_Layer_TOP;

    RoutingMatrix.m_GridRouting = (int) GetScreen()->GetGridSize().x;

    // Ensure Board.m_GridRouting has a reasonable value:
    if( RoutingMatrix.m_GridRouting < Millimeter2iu( 0.25 ) )
        RoutingMatrix.m_GridRouting = Millimeter2iu( 0.25 );

    // Compute module parameters used in auto place
    if( genPlacementRoutingMatrix( GetBoard(), m_messagePanel ) == 0 )
        return;

    int moduleCount = 0;
    Module = GetBoard()->m_Modules;

    for( ; Module != NULL; Module = Module->Next() )
    {
        Module->SetNeedsPlaced( false );

        switch( place_mode )
        {
        case PLACE_1_MODULE:

            if( currModule == Module )
            {
                // Module will be placed, add to undo.
                picker.SetItem( currModule );
                newList.PushItem( picker );
                Module->SetNeedsPlaced( true );
            }

            break;

        case PLACE_OUT_OF_BOARD:
            Module->SetIsPlaced( false );

            if( Module->IsLocked() )
                break;

            if( !RoutingMatrix.m_BrdBox.Contains( Module->GetPosition() ) )
            {
                // Module will be placed, add to undo.
                picker.SetItem( Module );
                newList.PushItem( picker );
                Module->SetNeedsPlaced( true );
            }

            break;

        case PLACE_ALL:
            Module->SetIsPlaced( false );

            if( Module->IsLocked() )
                break;

            // Module will be placed, add to undo.
            picker.SetItem( Module );
            newList.PushItem( picker );
            Module->SetNeedsPlaced( true );
            break;

        case PLACE_INCREMENTAL:

            if( Module->IsLocked() )
            {
                Module->SetIsPlaced( false );
                break;
            }

            if( !Module->NeedsPlaced() )
            {
                // Module will be placed, add to undo.
                picker.SetItem( Module );
                newList.PushItem( picker );
                Module->SetNeedsPlaced( true );
            }

            break;
        }

        if( Module->NeedsPlaced() )    // Erase from screen
        {
            moduleCount++;
            Module->Draw( m_canvas, DC, GR_XOR );
        }
        else
        {
            genModuleOnRoutingMatrix( Module );
        }
    }

    // Undo command: prepare list
    if( newList.GetCount() )
        SaveCopyInUndoList( newList, UR_CHANGED );

    int         cnt = 0;
    wxString    msg;

    while( ( Module = PickModule( this, DC ) ) != NULL )
    {
        // Display some info about activity, module placement can take a while:
        msg.Printf( _( "Place footprint %d of %d" ), cnt, moduleCount );
        SetStatusText( msg );

        double initialOrient = Module->GetOrientation();
        // Display fill area of interest, barriers, penalties.
        drawPlacementRoutingMatrix( GetBoard(), DC );

        error = getOptimalModulePlacement( this, Module, DC );
        double bestScore = MinCout;
        double bestRotation = 0.0;
        int rotAllowed;
        PosOK = CurrPosition;

        if( error == ESC )
            goto end_of_tst;

        // Try orientations 90, 180, 270 degrees from initial orientation
        rotAllowed = Module->GetPlacementCost180();

        if( rotAllowed != 0 )
        {
            Rotate_Module( DC, Module, 1800.0, true );
            error   = getOptimalModulePlacement( this, Module, DC );
            MinCout *= OrientPenality[rotAllowed];

            if( bestScore > MinCout )    // This orientation is better.
            {
                PosOK       = CurrPosition;
                bestScore   = MinCout;
                bestRotation = 1800.0;
            }
            else
            {
                Rotate_Module( DC, Module, initialOrient, false );
            }

            if( error == ESC )
                goto end_of_tst;
        }

        // Determine if the best orientation of a module is 90.
        rotAllowed = Module->GetPlacementCost90();

        if( rotAllowed != 0 )
        {
            Rotate_Module( DC, Module, 900.0, true );
            error   = getOptimalModulePlacement( this, Module, DC );
            MinCout *= OrientPenality[rotAllowed];

            if( bestScore > MinCout )    // This orientation is better.
            {
                PosOK       = CurrPosition;
                bestScore   = MinCout;
                bestRotation = 900.0;
            }
            else
            {
                Rotate_Module( DC, Module, initialOrient, false );
            }

            if( error == ESC )
                goto end_of_tst;
        }

        // Determine if the best orientation of a module is -90.
        if( rotAllowed != 0 )
        {
            Rotate_Module( DC, Module, 2700.0, true );
            error   = getOptimalModulePlacement( this, Module, DC );
            MinCout *= OrientPenality[rotAllowed];

            if( bestScore > MinCout )    // This orientation is better.
            {
                PosOK       = CurrPosition;
                bestScore   = MinCout;
                bestRotation = 2700.0;
            }
            else
            {
                Rotate_Module( DC, Module, initialOrient, false );
            }

            if( error == ESC )
                goto end_of_tst;
        }

end_of_tst:

        if( error == ESC )
            break;

        // Place module.
        CurrPosition = GetCrossHairPosition();
        SetCrossHairPosition( PosOK );

        PlaceModule( Module, DC );

        bestRotation += initialOrient;

        if( bestRotation != Module->GetOrientation() )
            Rotate_Module( DC, Module, bestRotation, false );

        SetCrossHairPosition( CurrPosition );

        Module->CalculateBoundingBox();

        genModuleOnRoutingMatrix( Module );
        Module->SetIsPlaced( true );
        Module->SetNeedsPlaced( false );
    }

    CurrPosition = memopos;

    RoutingMatrix.UnInitRoutingMatrix();

    g_Route_Layer_TOP       = lay_tmp_TOP;
    g_Route_Layer_BOTTOM    = lay_tmp_BOTTOM;

    Module = GetBoard()->m_Modules;

    for( ; Module != NULL; Module = Module->Next() )
    {
        Module->CalculateBoundingBox();
    }

    GetBoard()->m_Status_Pcb = 0;
    Compile_Ratsnest( DC, true );
    m_canvas->ReDraw( DC, true );
}


void drawPlacementRoutingMatrix( BOARD* aBrd, wxDC* DC )
{
    int         ii, jj;
    EDA_COLOR_T color;
    int         ox, oy;
    MATRIX_CELL top_state, bottom_state;

    GRSetDrawMode( DC, GR_COPY );

    for( ii = 0; ii < RoutingMatrix.m_Nrows; ii++ )
    {
        oy = RoutingMatrix.m_BrdBox.GetY() + ( ii * RoutingMatrix.m_GridRouting );

        for( jj = 0; jj < RoutingMatrix.m_Ncols; jj++ )
        {
            ox      = RoutingMatrix.m_BrdBox.GetX() + (jj * RoutingMatrix.m_GridRouting);
            color   = BLACK;

            top_state       = RoutingMatrix.GetCell( ii, jj, TOP );
            bottom_state    = RoutingMatrix.GetCell( ii, jj, BOTTOM );

            if( top_state & CELL_is_ZONE )
                color = BLUE;

            // obstacles
            if( ( top_state & CELL_is_EDGE ) || ( bottom_state & CELL_is_EDGE ) )
                color = WHITE;
            else if( top_state & ( HOLE | CELL_is_MODULE ) )
                color = LIGHTRED;
            else if( bottom_state & (HOLE | CELL_is_MODULE) )
                color = LIGHTGREEN;
            else    // Display the filling and keep out regions.
            {
                if( RoutingMatrix.GetDist( ii, jj, TOP )
                    || RoutingMatrix.GetDist( ii, jj, BOTTOM ) )
                    color = DARKGRAY;
            }

            GRPutPixel( NULL, DC, ox, oy, color );
        }
    }
}


int genPlacementRoutingMatrix( BOARD* aBrd, EDA_MSG_PANEL* messagePanel )
{
    wxString msg;

    RoutingMatrix.UnInitRoutingMatrix();

    EDA_RECT bbox = aBrd->ComputeBoundingBox( true );

    if( bbox.GetWidth() == 0 || bbox.GetHeight() == 0 )
    {
        DisplayError( NULL, _( "No PCB edge found, unknown board size!" ) );
        return 0;
    }

    RoutingMatrix.ComputeMatrixSize( aBrd, true );
    int nbCells = RoutingMatrix.m_Ncols * RoutingMatrix.m_Nrows;

    messagePanel->EraseMsgBox();
    msg.Printf( wxT( "%d" ), RoutingMatrix.m_Ncols );
    messagePanel->SetMessage( 1, _( "Cols" ), msg, GREEN );
    msg.Printf( wxT( "%d" ), RoutingMatrix.m_Nrows );
    messagePanel->SetMessage( 7, _( "Lines" ), msg, GREEN );
    msg.Printf( wxT( "%d" ), nbCells );
    messagePanel->SetMessage( 14, _( "Cells." ), msg, YELLOW );

    // Choose the number of board sides.
    RoutingMatrix.m_RoutingLayersCount = 2;

    RoutingMatrix.InitRoutingMatrix();

    // Display memory usage.
    msg.Printf( wxT( "%d" ), RoutingMatrix.m_MemSize / 1024 );
    messagePanel->SetMessage( 24, wxT( "Mem(Kb)" ), msg, CYAN );

    g_Route_Layer_BOTTOM = F_Cu;

    if( RoutingMatrix.m_RoutingLayersCount > 1 )
        g_Route_Layer_BOTTOM = B_Cu;

    g_Route_Layer_TOP = F_Cu;

    // Place the edge layer segments
    TRACK TmpSegm( NULL );

    TmpSegm.SetLayer( UNDEFINED_LAYER );
    TmpSegm.SetNetCode( -1 );
    TmpSegm.SetWidth( RoutingMatrix.m_GridRouting / 2 );

    EDA_ITEM* PtStruct = aBrd->m_Drawings;

    for( ; PtStruct != NULL; PtStruct = PtStruct->Next() )
    {
        DRAWSEGMENT* DrawSegm;

        switch( PtStruct->Type() )
        {
        case PCB_LINE_T:
            DrawSegm = (DRAWSEGMENT*) PtStruct;

            if( DrawSegm->GetLayer() != Edge_Cuts )
                break;

            TraceSegmentPcb( DrawSegm, HOLE | CELL_is_EDGE,
                             RoutingMatrix.m_GridRouting, WRITE_CELL );
            break;

        case PCB_TEXT_T:
        default:
            break;
        }
    }

    // Mark cells of the routing matrix to CELL_is_ZONE
    // (i.e. availlable cell to place a module )
    // Init a starting point of attachment to the area.
    RoutingMatrix.OrCell( RoutingMatrix.m_Nrows / 2, RoutingMatrix.m_Ncols / 2,
                          BOTTOM, CELL_is_ZONE );

    // find and mark all other availlable cells:
    for( int ii = 1; ii != 0; )
        ii = propagate();

    // Initialize top layer. to the same value as the bottom layer
    if( RoutingMatrix.m_BoardSide[TOP] )
        memcpy( RoutingMatrix.m_BoardSide[TOP], RoutingMatrix.m_BoardSide[BOTTOM],
                nbCells * sizeof(MATRIX_CELL) );

    return 1;
}


/* Place module on Routing matrix.
 */
void genModuleOnRoutingMatrix( MODULE* Module )
{
    int         ox, oy, fx, fy;
    LSET        layerMask;
    D_PAD*      Pad;

    EDA_RECT    fpBBox = Module->GetBoundingBox();

    fpBBox.Inflate( RoutingMatrix.m_GridRouting / 2 );
    ox  = fpBBox.GetX();
    fx  = fpBBox.GetRight();
    oy  = fpBBox.GetY();
    fy  = fpBBox.GetBottom();

    if( ox < RoutingMatrix.m_BrdBox.GetX() )
        ox = RoutingMatrix.m_BrdBox.GetX();

    if( ox > RoutingMatrix.m_BrdBox.GetRight() )
        ox = RoutingMatrix.m_BrdBox.GetRight();

    if( fx < RoutingMatrix.m_BrdBox.GetX() )
        fx = RoutingMatrix.m_BrdBox.GetX();

    if( fx > RoutingMatrix.m_BrdBox.GetRight() )
        fx = RoutingMatrix.m_BrdBox.GetRight();

    if( oy < RoutingMatrix.m_BrdBox.GetY() )
        oy = RoutingMatrix.m_BrdBox.GetY();

    if( oy > RoutingMatrix.m_BrdBox.GetBottom() )
        oy = RoutingMatrix.m_BrdBox.GetBottom();

    if( fy < RoutingMatrix.m_BrdBox.GetY() )
        fy = RoutingMatrix.m_BrdBox.GetY();

    if( fy > RoutingMatrix.m_BrdBox.GetBottom() )
        fy = RoutingMatrix.m_BrdBox.GetBottom();

    if( Module->GetLayer() == F_Cu )
        layerMask.set( F_Cu );

    if( Module->GetLayer() == B_Cu )
        layerMask.set( B_Cu );

    TraceFilledRectangle( ox, oy, fx, fy, layerMask,
                          CELL_is_MODULE, WRITE_OR_CELL );

    // Trace pads + clearance areas.
    for( Pad = Module->Pads(); Pad != NULL; Pad = Pad->Next() )
    {
        int margin = (RoutingMatrix.m_GridRouting / 2) + Pad->GetClearance();
        ::PlacePad( Pad, CELL_is_MODULE, margin, WRITE_OR_CELL );
    }

    // Trace clearance.
    int margin = ( RoutingMatrix.m_GridRouting * Module->GetPadCount() ) / GAIN;
    CreateKeepOutRectangle( ox, oy, fx, fy, margin, KEEP_OUT_MARGIN, layerMask );
}

// A minor helper function to draw a bounding box:
inline void draw_FootprintRect(EDA_RECT * aClipBox, wxDC* aDC, EDA_RECT& fpBBox, EDA_COLOR_T aColor)
{
#ifndef USE_WX_OVERLAY
    GRRect( aClipBox, aDC, fpBBox, 0, aColor );
#endif
}

int getOptimalModulePlacement( PCB_EDIT_FRAME* aFrame, MODULE* aModule, wxDC* aDC )
{
    int     error = 1;
    wxPoint LastPosOK;
    double  min_cost, curr_cost, Score;
    bool    TstOtherSide;
    DISPLAY_OPTIONS* displ_opts = (DISPLAY_OPTIONS*)aFrame->GetDisplayOptions();
    BOARD*  brd = aFrame->GetBoard();

    aModule->CalculateBoundingBox();

    bool showRats = displ_opts->m_Show_Module_Ratsnest;
    displ_opts->m_Show_Module_Ratsnest = false;

    brd->m_Status_Pcb &= ~RATSNEST_ITEM_LOCAL_OK;
    aFrame->SetMsgPanel( aModule );

    LastPosOK = RoutingMatrix.m_BrdBox.GetOrigin();

    wxPoint     mod_pos = aModule->GetPosition();
    EDA_RECT    fpBBox  = aModule->GetFootprintRect();

    // Move fpBBox to have the footprint position at (0,0)
    fpBBox.Move( -mod_pos );
    wxPoint fpBBoxOrg = fpBBox.GetOrigin();

    // Calculate the limit of the footprint position, relative
    // to the routing matrix area
    wxPoint xylimit = RoutingMatrix.m_BrdBox.GetEnd() - fpBBox.GetEnd();

    wxPoint initialPos = RoutingMatrix.m_BrdBox.GetOrigin() - fpBBoxOrg;

    // Stay on grid.
    initialPos.x    -= initialPos.x % RoutingMatrix.m_GridRouting;
    initialPos.y    -= initialPos.y % RoutingMatrix.m_GridRouting;

    CurrPosition = initialPos;

    // Undraw the current footprint
    g_Offset_Module = wxPoint( 0, 0 );
    DrawModuleOutlines( aFrame->GetCanvas(), aDC, aModule );

    g_Offset_Module = mod_pos - CurrPosition;

    /* Examine pads, and set TstOtherSide to true if a footprint
     * has at least 1 pad through.
     */
    TstOtherSide = false;

    if( RoutingMatrix.m_RoutingLayersCount > 1 )
    {
        LSET    other( aModule->GetLayer() == B_Cu  ? F_Cu : B_Cu );

        for( D_PAD* pad = aModule->Pads(); pad; pad = pad->Next() )
        {
            if( !( pad->GetLayerSet() & other ).any() )
                continue;

            TstOtherSide = true;
            break;
        }
    }

    // Draw the initial bounding box position
    EDA_COLOR_T color = BROWN;
    fpBBox.SetOrigin( fpBBoxOrg + CurrPosition );
    draw_FootprintRect(aFrame->GetCanvas()->GetClipBox(), aDC, fpBBox, color);

    min_cost = -1.0;
    aFrame->SetStatusText( wxT( "Score ??, pos ??" ) );

    for( ; CurrPosition.x < xylimit.x; CurrPosition.x += RoutingMatrix.m_GridRouting )
    {
        wxYield();

        if( aFrame->GetCanvas()->GetAbortRequest() )
        {
            if( IsOK( aFrame, _( "OK to abort?" ) ) )
                return ESC;
            else
                aFrame->GetCanvas()->SetAbortRequest( false );
        }

        CurrPosition.y = initialPos.y;

        for( ; CurrPosition.y < xylimit.y; CurrPosition.y += RoutingMatrix.m_GridRouting )
        {
            // Erase traces.
            draw_FootprintRect( aFrame->GetCanvas()->GetClipBox(), aDC, fpBBox, color );

            fpBBox.SetOrigin( fpBBoxOrg + CurrPosition );
            g_Offset_Module = mod_pos - CurrPosition;
            int keepOutCost = TstModuleOnBoard( brd, aModule, TstOtherSide );

            // Draw at new place
            color = keepOutCost >= 0 ? BROWN : RED;
            draw_FootprintRect( aFrame->GetCanvas()->GetClipBox(), aDC, fpBBox, color );

            if( keepOutCost >= 0 )    // i.e. if the module can be put here
            {
                error = 0;
                aFrame->build_ratsnest_module( aModule );
                curr_cost   = compute_Ratsnest_PlaceModule( brd );
                Score       = curr_cost + keepOutCost;

                if( (min_cost >= Score ) || (min_cost < 0 ) )
                {
                    LastPosOK   = CurrPosition;
                    min_cost    = Score;
                    wxString msg;
                    msg.Printf( wxT( "Score %g, pos %s, %s" ),
                                min_cost,
                                GetChars( ::CoordinateToString( LastPosOK.x ) ),
                                GetChars( ::CoordinateToString( LastPosOK.y ) ) );
                    aFrame->SetStatusText( msg );
                }
            }
        }
    }

    // erasing the last traces
    GRRect( aFrame->GetCanvas()->GetClipBox(), aDC, fpBBox, 0, BROWN );

    displ_opts->m_Show_Module_Ratsnest = showRats;

    // Regeneration of the modified variable.
    CurrPosition = LastPosOK;

    brd->m_Status_Pcb &= ~( RATSNEST_ITEM_LOCAL_OK | LISTE_PAD_OK );

    MinCout = min_cost;
    return error;
}


/* Test if the rectangular area (ux, ux .. y0, y1):
 * - is a free zone (except OCCUPED_By_MODULE returns)
 * - is on the working surface of the board (otherwise returns OUT_OF_BOARD)
 *
 * Returns OUT_OF_BOARD, or OCCUPED_By_MODULE or FREE_CELL if OK
 */
int TstRectangle( BOARD* Pcb, const EDA_RECT& aRect, int side )
{
    EDA_RECT rect = aRect;

    rect.Inflate( RoutingMatrix.m_GridRouting / 2 );

    wxPoint start   = rect.GetOrigin();
    wxPoint end     = rect.GetEnd();

    start   -= RoutingMatrix.m_BrdBox.GetOrigin();
    end     -= RoutingMatrix.m_BrdBox.GetOrigin();

    int row_min = start.y / RoutingMatrix.m_GridRouting;
    int row_max = end.y / RoutingMatrix.m_GridRouting;
    int col_min = start.x / RoutingMatrix.m_GridRouting;
    int col_max = end.x / RoutingMatrix.m_GridRouting;

    if( start.y > row_min * RoutingMatrix.m_GridRouting )
        row_min++;

    if( start.x > col_min * RoutingMatrix.m_GridRouting )
        col_min++;

    if( row_min < 0 )
        row_min = 0;

    if( row_max >= ( RoutingMatrix.m_Nrows - 1 ) )
        row_max = RoutingMatrix.m_Nrows - 1;

    if( col_min < 0 )
        col_min = 0;

    if( col_max >= ( RoutingMatrix.m_Ncols - 1 ) )
        col_max = RoutingMatrix.m_Ncols - 1;

    for( int row = row_min; row <= row_max; row++ )
    {
        for( int col = col_min; col <= col_max; col++ )
        {
            unsigned int data = RoutingMatrix.GetCell( row, col, side );

            if( ( data & CELL_is_ZONE ) == 0 )
                return OUT_OF_BOARD;

            if( (data & CELL_is_MODULE) )
                return OCCUPED_By_MODULE;
        }
    }

    return FREE_CELL;
}


/* Calculates and returns the clearance area of the rectangular surface
 * aRect):
 * (Sum of cells in terms of distance)
 */
unsigned int CalculateKeepOutArea( const EDA_RECT& aRect, int side )
{
    wxPoint start   = aRect.GetOrigin();
    wxPoint end     = aRect.GetEnd();

    start   -= RoutingMatrix.m_BrdBox.GetOrigin();
    end     -= RoutingMatrix.m_BrdBox.GetOrigin();

    int row_min = start.y / RoutingMatrix.m_GridRouting;
    int row_max = end.y / RoutingMatrix.m_GridRouting;
    int col_min = start.x / RoutingMatrix.m_GridRouting;
    int col_max = end.x / RoutingMatrix.m_GridRouting;

    if( start.y > row_min * RoutingMatrix.m_GridRouting )
        row_min++;

    if( start.x > col_min * RoutingMatrix.m_GridRouting )
        col_min++;

    if( row_min < 0 )
        row_min = 0;

    if( row_max >= ( RoutingMatrix.m_Nrows - 1 ) )
        row_max = RoutingMatrix.m_Nrows - 1;

    if( col_min < 0 )
        col_min = 0;

    if( col_max >= ( RoutingMatrix.m_Ncols - 1 ) )
        col_max = RoutingMatrix.m_Ncols - 1;

    unsigned int keepOutCost = 0;

    for( int row = row_min; row <= row_max; row++ )
    {
        for( int col = col_min; col <= col_max; col++ )
        {
            // RoutingMatrix.GetDist returns the "cost" of the cell
            // at position (row, col)
            // in autoplace this is the cost of the cell, if it is
            // inside aRect
            keepOutCost += RoutingMatrix.GetDist( row, col, side );
        }
    }

    return keepOutCost;
}


/* Test if the module can be placed on the board.
 * Returns the value TstRectangle().
 * Module is known by its bounding box
 */
int TstModuleOnBoard( BOARD* Pcb, MODULE* aModule, bool TstOtherSide )
{
    int side = TOP;
    int otherside = BOTTOM;

    if( aModule->GetLayer() == B_Cu )
    {
        side = BOTTOM; otherside = TOP;
    }

    EDA_RECT    fpBBox = aModule->GetFootprintRect();
    fpBBox.Move( -g_Offset_Module );

    int         diag = TstRectangle( Pcb, fpBBox, side );

    if( diag != FREE_CELL )
        return diag;

    if( TstOtherSide )
    {
        diag = TstRectangle( Pcb, fpBBox, otherside );

        if( diag != FREE_CELL )
            return diag;
    }

    int marge = ( RoutingMatrix.m_GridRouting * aModule->GetPadCount() ) / GAIN;

    fpBBox.Inflate( marge );
    return CalculateKeepOutArea( fpBBox, side );
}


double compute_Ratsnest_PlaceModule( BOARD* aBrd )
{
    double  curr_cost;
    wxPoint start;      // start point of a ratsnest
    wxPoint end;        // end point of a ratsnest
    int     dx, dy;

    if( ( aBrd->m_Status_Pcb & RATSNEST_ITEM_LOCAL_OK ) == 0 )
        return -1;

    curr_cost = 0;

    for( unsigned ii = 0; ii < aBrd->m_LocalRatsnest.size(); ii++ )
    {
        RATSNEST_ITEM* pt_local_rats_nest = &aBrd->m_LocalRatsnest[ii];

        if( ( pt_local_rats_nest->m_Status & LOCAL_RATSNEST_ITEM ) )
            continue; // Skip ratsnest between 2 pads of the current module

        // Skip modules not inside the board area
        MODULE* module = pt_local_rats_nest->m_PadEnd->GetParent();

        if( !RoutingMatrix.m_BrdBox.Contains( module->GetPosition() ) )
            continue;

        start   = pt_local_rats_nest->m_PadStart->GetPosition() - g_Offset_Module;
        end     = pt_local_rats_nest->m_PadEnd->GetPosition();

        // Cost of the ratsnest.
        dx  = end.x - start.x;
        dy  = end.y - start.y;

        dx  = abs( dx );
        dy  = abs( dy );

        // ttry to have always dx >= dy to calculate the cost of the rastsnet
        if( dx < dy )
            EXCHG( dx, dy );

        // Cost of the connection = lenght + penalty due to the slope
        // dx is the biggest lenght relative to the X or Y axis
        // the penalty is max for 45 degrees ratsnests,
        // and 0 for horizontal or vertical ratsnests.
        // For Horizontal and Vertical ratsnests, dy = 0;
        double conn_cost = hypot( dx, dy * 2.0 );
        curr_cost += conn_cost;    // Total cost = sum of costs of each connection
    }

    return curr_cost;
}


/**
 * Function CreateKeepOutRectangle
 * builds the cost map:
 * Cells ( in Dist map ) inside the rect x0,y0 a x1,y1 are
 *  incremented by value aKeepOut
 *  Cell outside this rectangle, but inside the rectangle
 *  x0,y0 -marge to x1,y1 + marge are incremented by a decreasing value
 *  (aKeepOut ... 0). The decreasing value depends on the distance to the first rectangle
 *  Therefore the cost is high in rect x0,y0 to x1,y1, and decrease outside this rectangle
 */
void CreateKeepOutRectangle( int ux0, int uy0, int ux1, int uy1,
                             int marge, int aKeepOut, LSET aLayerMask )
{
    int         row, col;
    int         row_min, row_max, col_min, col_max, pmarge;
    int         trace = 0;
    DIST_CELL   data, LocalKeepOut;
    int         lgain, cgain;

    if( aLayerMask[g_Route_Layer_BOTTOM] )
        trace = 1; // Trace on bottom layer.

    if( aLayerMask[g_Route_Layer_TOP] && RoutingMatrix.m_RoutingLayersCount )
        trace |= 2; // Trace on top layer.

    if( trace == 0 )
        return;

    ux0 -= RoutingMatrix.m_BrdBox.GetX();
    uy0 -= RoutingMatrix.m_BrdBox.GetY();
    ux1 -= RoutingMatrix.m_BrdBox.GetX();
    uy1 -= RoutingMatrix.m_BrdBox.GetY();

    ux0 -= marge; ux1 += marge;
    uy0 -= marge; uy1 += marge;

    pmarge = marge / RoutingMatrix.m_GridRouting;

    if( pmarge < 1 )
        pmarge = 1;

    // Calculate the coordinate limits of the rectangle.
    row_max = uy1 / RoutingMatrix.m_GridRouting;
    col_max = ux1 / RoutingMatrix.m_GridRouting;
    row_min = uy0 / RoutingMatrix.m_GridRouting;

    if( uy0 > row_min * RoutingMatrix.m_GridRouting )
        row_min++;

    col_min = ux0 / RoutingMatrix.m_GridRouting;

    if( ux0 > col_min * RoutingMatrix.m_GridRouting )
        col_min++;

    if( row_min < 0 )
        row_min = 0;

    if( row_max >= (RoutingMatrix.m_Nrows - 1) )
        row_max = RoutingMatrix.m_Nrows - 1;

    if( col_min < 0 )
        col_min = 0;

    if( col_max >= (RoutingMatrix.m_Ncols - 1) )
        col_max = RoutingMatrix.m_Ncols - 1;

    for( row = row_min; row <= row_max; row++ )
    {
        lgain = 256;

        if( row < pmarge )
            lgain = ( 256 * row ) / pmarge;
        else if( row > row_max - pmarge )
            lgain = ( 256 * ( row_max - row ) ) / pmarge;

        for( col = col_min; col <= col_max; col++ )
        {
            // RoutingMatrix Dist map containt the "cost" of the cell
            // at position (row, col)
            // in autoplace this is the cost of the cell, when
            // a footprint overlaps it, near a "master" footprint
            // this cost is hight near the "master" footprint
            // and decrease with the distance
            cgain = 256;
            LocalKeepOut = aKeepOut;

            if( col < pmarge )
                cgain = ( 256 * col ) / pmarge;
            else if( col > col_max - pmarge )
                cgain = ( 256 * ( col_max - col ) ) / pmarge;

            cgain = ( cgain * lgain ) / 256;

            if( cgain != 256 )
                LocalKeepOut = ( LocalKeepOut * cgain ) / 256;

            if( trace & 1 )
            {
                data = RoutingMatrix.GetDist( row, col, BOTTOM ) + LocalKeepOut;
                RoutingMatrix.SetDist( row, col, BOTTOM, data );
            }

            if( trace & 2 )
            {
                data    = RoutingMatrix.GetDist( row, col, TOP );
                data    = std::max( data, LocalKeepOut );
                RoutingMatrix.SetDist( row, col, TOP, data );
            }
        }
    }
}


// Sort routines
static bool Tri_PlaceModules( MODULE* ref, MODULE* compare )
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
 * Function PickModule
 * find the "best" module place
 * The criteria are:
 * - Maximum ratsnest with modules already placed
 * - Max size, and number of pads max
 */
static MODULE* PickModule( PCB_EDIT_FRAME* pcbframe, wxDC* DC )
{
    MODULE* Module;
    std::vector <MODULE*> moduleList;

    // Build sorted footprints list (sort by decreasing size )
    Module = pcbframe->GetBoard()->m_Modules;

    for( ; Module != NULL; Module = Module->Next() )
    {
        Module->CalculateBoundingBox();
        moduleList.push_back( Module );
    }

    sort( moduleList.begin(), moduleList.end(), Tri_PlaceModules );

    for( unsigned ii = 0; ii < moduleList.size(); ii++ )
    {
        Module = moduleList[ii];
        Module->SetFlag( 0 );

        if( !Module->NeedsPlaced() )
            continue;

        pcbframe->GetBoard()->m_Status_Pcb &= ~RATSNEST_ITEM_LOCAL_OK;
        pcbframe->SetMsgPanel( Module );
        pcbframe->build_ratsnest_module( Module );

        // Calculate external ratsnest.
        for( unsigned ii = 0; ii < pcbframe->GetBoard()->m_LocalRatsnest.size(); ii++ )
        {
            if( ( pcbframe->GetBoard()->m_LocalRatsnest[ii].m_Status &
                  LOCAL_RATSNEST_ITEM ) == 0 )
                Module->IncrementFlag();
        }
    }

    pcbframe->GetBoard()->m_Status_Pcb &= ~RATSNEST_ITEM_LOCAL_OK;

    sort( moduleList.begin(), moduleList.end(), sortFootprintsByRatsnestSize );

    // Search for "best" module.
    MODULE* bestModule  = NULL;
    MODULE* altModule   = NULL;

    for( unsigned ii = 0; ii < moduleList.size(); ii++ )
    {
        Module = moduleList[ii];

        if( !Module->NeedsPlaced() )
            continue;

        altModule = Module;

        if( Module->GetFlag() == 0 )
            continue;

        bestModule = Module;
        break;
    }

    if( bestModule )
        return bestModule;
    else
        return altModule;
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
 *  @return added cells count (i.e. which the attribute CELL_is_ZONE is set)
 */
int propagate()
{
    int     row, col;
    long    current_cell, old_cell_H;
    std::vector<long> pt_cell_V;
    int     nbpoints = 0;

#define NO_CELL_ZONE (HOLE | CELL_is_EDGE | CELL_is_ZONE)

    pt_cell_V.reserve( std::max( RoutingMatrix.m_Nrows, RoutingMatrix.m_Ncols ) );
    fill( pt_cell_V.begin(), pt_cell_V.end(), 0 );

    // Search from left to right and top to bottom.
    for( row = 0; row < RoutingMatrix.m_Nrows; row++ )
    {
        old_cell_H = 0;

        for( col = 0; col < RoutingMatrix.m_Ncols; col++ )
        {
            current_cell = RoutingMatrix.GetCell( row, col, BOTTOM ) & NO_CELL_ZONE;

            if( current_cell == 0 )    // a free cell is found
            {
                if( (old_cell_H & CELL_is_ZONE) || (pt_cell_V[col] & CELL_is_ZONE) )
                {
                    RoutingMatrix.OrCell( row, col, BOTTOM, CELL_is_ZONE );
                    current_cell = CELL_is_ZONE;
                    nbpoints++;
                }
            }

            pt_cell_V[col] = old_cell_H = current_cell;
        }
    }

    // Search from right to left and top to bottom/
    fill( pt_cell_V.begin(), pt_cell_V.end(), 0 );

    for( row = 0; row < RoutingMatrix.m_Nrows; row++ )
    {
        old_cell_H = 0;

        for( col = RoutingMatrix.m_Ncols - 1; col >= 0; col-- )
        {
            current_cell = RoutingMatrix.GetCell( row, col, BOTTOM ) & NO_CELL_ZONE;

            if( current_cell == 0 )    // a free cell is found
            {
                if( (old_cell_H & CELL_is_ZONE) || (pt_cell_V[col] & CELL_is_ZONE) )
                {
                    RoutingMatrix.OrCell( row, col, BOTTOM, CELL_is_ZONE );
                    current_cell = CELL_is_ZONE;
                    nbpoints++;
                }
            }

            pt_cell_V[col] = old_cell_H = current_cell;
        }
    }

    // Search from bottom to top and right to left.
    fill( pt_cell_V.begin(), pt_cell_V.end(), 0 );

    for( col = RoutingMatrix.m_Ncols - 1; col >= 0; col-- )
    {
        old_cell_H = 0;

        for( row = RoutingMatrix.m_Nrows - 1; row >= 0; row-- )
        {
            current_cell = RoutingMatrix.GetCell( row, col, BOTTOM ) & NO_CELL_ZONE;

            if( current_cell == 0 )    // a free cell is found
            {
                if( (old_cell_H & CELL_is_ZONE) || (pt_cell_V[row] & CELL_is_ZONE) )
                {
                    RoutingMatrix.OrCell( row, col, BOTTOM, CELL_is_ZONE );
                    current_cell = CELL_is_ZONE;
                    nbpoints++;
                }
            }

            pt_cell_V[row] = old_cell_H = current_cell;
        }
    }

    // Search from bottom to top and left to right.
    fill( pt_cell_V.begin(), pt_cell_V.end(), 0 );

    for( col = 0; col < RoutingMatrix.m_Ncols; col++ )
    {
        old_cell_H = 0;

        for( row = RoutingMatrix.m_Nrows - 1; row >= 0; row-- )
        {
            current_cell = RoutingMatrix.GetCell( row, col, BOTTOM ) & NO_CELL_ZONE;

            if( current_cell == 0 )    // a free cell is found
            {
                if( (old_cell_H & CELL_is_ZONE) || (pt_cell_V[row] & CELL_is_ZONE) )
                {
                    RoutingMatrix.OrCell( row, col, BOTTOM, CELL_is_ZONE );
                    current_cell = CELL_is_ZONE;
                    nbpoints++;
                }
            }

            pt_cell_V[row] = old_cell_H = current_cell;
        }
    }

    return nbpoints;
}
