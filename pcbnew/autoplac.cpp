/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
 * Copyright (C) 2008-2011 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 1992-2011 KiCad Developers, see change_log.txt for contributors.
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
 * @file autoplac.cpp
 * @brief Routiness to automatically place MODULES on a board.
 */

#include <fctsys.h>
#include <class_drawpanel.h>
#include <confirm.h>
#include <pcbnew.h>
#include <wxPcbStruct.h>
#include <gr_basic.h>
#include <macros.h>
#include <pcbcommon.h>

#include <protos.h>
#include <ar_protos.h>
#include <cell.h>
#include <colors_selection.h>

#include <class_board.h>
#include <class_module.h>
#include <class_track.h>
#include <class_drawsegment.h>


#define GAIN            16
#define KEEP_OUT_MARGIN 500


/* Penalty for guidance given by CntRot90 and CntRot180:
 * graduated from 0 (rotation allowed) to 10 (rotation count null)
 * the count is increased.
 */
static const float OrientPenality[11] =
{
    2.0f,       /* CntRot = 0 rotation prohibited */
    1.9f,       /* CntRot = 1 */
    1.8f,       /* CntRot = 2 */
    1.7f,       /* CntRot = 3 */
    1.6f,       /* CntRot = 4 */
    1.5f,       /* CntRot = 5 */
    1.4f,       /* CntRot = 5 */
    1.3f,       /* CntRot = 7 */
    1.2f,       /* CntRot = 8 */
    1.1f,       /* CntRot = 9 */
    1.0f        /* CntRot = 10 rotation authorized, no penalty */
};

/* Cell states. */
#define OUT_OF_BOARD      -2
#define OCCUPED_By_MODULE -1


static EDA_RECT bbbox;              // boards bounding box

static wxPoint CurrPosition; // Current position of the current module placement
static bool    AutoPlaceShowAll = true;

float          MinCout;

static int  TstModuleOnBoard( BOARD* Pcb, MODULE* Module, bool TstOtherSide );

static void CreateKeepOutRectangle( BOARD* Pcb,
                                    int    ux0,
                                    int    uy0,
                                    int    ux1,
                                    int    uy1,
                                    int    marge,
                                    int    aKeepOut,
                                    int    aLayerMask );

static MODULE* PickModule( PCB_EDIT_FRAME* pcbframe, wxDC* DC );


void PCB_EDIT_FRAME::AutoPlaceModule( MODULE* Module, int place_mode, wxDC* DC )
{
    int      ii, activ;
    MODULE*  ThisModule = NULL;
    wxPoint  PosOK;
    wxPoint  memopos;
    int      error;
    int      NbModules = 0;
    int      NbTotalModules = 0;
    float    Pas;
    int      lay_tmp_TOP, lay_tmp_BOTTOM;

    // Undo: init list
    PICKED_ITEMS_LIST  newList;
    newList.m_Status = UR_CHANGED;
    ITEM_PICKER        picker( NULL, UR_CHANGED );

    if( GetBoard()->m_Modules == NULL )
        return;

    m_canvas->SetAbortRequest( false );

    switch( place_mode )
    {
    case PLACE_1_MODULE:
        ThisModule = Module;

        if( ThisModule == NULL )
            return;

        ThisModule->m_ModuleStatus &= ~(MODULE_is_PLACED | MODULE_to_PLACE);
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
    lay_tmp_BOTTOM = Route_Layer_BOTTOM;
    lay_tmp_TOP    = Route_Layer_TOP;

    Board.m_GridRouting = (int) GetScreen()->GetGridSize().x;

    // Ensure Board.m_GridRouting has a reasonable value:
    if( Board.m_GridRouting < 10 )
        Board.m_GridRouting = 10;                      // Min value = 1/1000 inch

    /* Compute module parameters used in auto place */
    Module = GetBoard()->m_Modules;
    NbTotalModules = 0;

    for( ; Module != NULL; Module = Module->Next() )
    {
        Module->CalculateBoundingBox();
        NbTotalModules ++;
    }

    if( GenPlaceBoard() == 0 )
        return;

    Module = GetBoard()->m_Modules;

    for( ; Module != NULL; Module = Module->Next() )
    {
        Module->m_ModuleStatus &= ~MODULE_to_PLACE;

        switch( place_mode )
        {
        case PLACE_1_MODULE:
            if( ThisModule == Module )
            {
                // Module will be placed, add to undo.
                picker.SetItem( ThisModule );
                newList.PushItem( picker );

                Module->m_ModuleStatus |= MODULE_to_PLACE;
            }

            break;

        case PLACE_OUT_OF_BOARD:
            Module->m_ModuleStatus &= ~MODULE_is_PLACED;

            if( Module->m_ModuleStatus & MODULE_is_LOCKED )
                break;

            if( !bbbox.Contains( Module->m_Pos ) )
            {
                // Module will be placed, add to undo.
                picker.SetItem( Module );
                newList.PushItem( picker );

                Module->m_ModuleStatus |= MODULE_to_PLACE;
            }

            break;

        case PLACE_ALL:
            Module->m_ModuleStatus &= ~MODULE_is_PLACED;

            if( Module->m_ModuleStatus & MODULE_is_LOCKED )
                break;

            // Module will be placed, add to undo.
            picker.SetItem( Module );
            newList.PushItem( picker );

            Module->m_ModuleStatus |= MODULE_to_PLACE;
            break;

        case PLACE_INCREMENTAL:
            if( Module->m_ModuleStatus & MODULE_is_LOCKED )
            {
                Module->m_ModuleStatus &= ~MODULE_is_PLACED;
                break;
            }

            if( !(Module->m_ModuleStatus & MODULE_is_PLACED) )
            {
                // Module will be placed, add to undo.
                picker.SetItem( Module );
                newList.PushItem( picker );

                Module->m_ModuleStatus |= MODULE_to_PLACE;
            }

            break;
        }


        if( Module->m_ModuleStatus & MODULE_to_PLACE )  // Erase from screen
        {
            NbModules++;
            Module->Draw( m_canvas, DC, GR_XOR );
        }
        else
        {
            GenModuleOnBoard( Module );
        }
    }

    // Undo: commit
    if( newList.GetCount() )
        SaveCopyInUndoList( newList, UR_CHANGED );

    activ = 0;
    Pas   = 100.0;

    if( NbModules )
        Pas = 100.0 / (float) NbModules;

    while( ( Module = PickModule( this, DC ) ) != NULL )
    {
        float BestScore;
        DisplayActivity( (int) (activ * Pas), wxEmptyString ); activ++;

        /* Display fill area of interest, barriers, penalties. */
        DrawInfoPlace( DC );

        error     = GetOptimalModulePlacement( Module, DC );
        BestScore = MinCout;
        PosOK     = CurrPosition;

        if( error == ESC )
            goto end_of_tst;

        /* Determine if the best orientation of a module is 180. */
        ii = Module->m_CntRot180 & 0x0F;

        if( ii != 0 )
        {
            int Angle_Rot_Module = 1800;
            Rotate_Module( DC, Module, Angle_Rot_Module, false );
            Module->CalculateBoundingBox();
            error    = GetOptimalModulePlacement( Module, DC );
            MinCout *= OrientPenality[ii];

            if( BestScore > MinCout )   /* This orientation is best. */
            {
                PosOK     = CurrPosition;
                BestScore = MinCout;
            }
            else
            {
                Angle_Rot_Module = -1800;
                Rotate_Module( DC, Module, Angle_Rot_Module, false );
            }

            if( error == ESC )
                goto end_of_tst;
        }

        /* Determine if the best orientation of a module is 90. */
        ii = Module->m_CntRot90 & 0x0F;

        if( ii != 0 )
        {
            int Angle_Rot_Module = 900;
            Rotate_Module( DC, Module, Angle_Rot_Module, false );
            error    = GetOptimalModulePlacement( Module, DC );
            MinCout *= OrientPenality[ii];

            if( BestScore > MinCout )   /* This orientation is best. */
            {
                PosOK     = CurrPosition;
                BestScore = MinCout;
            }
            else
            {
                Angle_Rot_Module = -900;
                Rotate_Module( DC, Module, Angle_Rot_Module, false );
            }

            if( error == ESC )
                goto end_of_tst;
        }

        /*  Determine if the best orientation of a module is 270. */
        ii = (Module->m_CntRot90 >> 4 ) & 0x0F;

        if( ii != 0 )
        {
            int Angle_Rot_Module = 2700;
            Rotate_Module( DC, Module, Angle_Rot_Module, false );
            error    = GetOptimalModulePlacement( Module, DC );
            MinCout *= OrientPenality[ii];

            if( BestScore > MinCout )   /* This orientation is best. */
            {
                PosOK     = CurrPosition;
                BestScore = MinCout;
            }
            else
            {
                Angle_Rot_Module = -2700;
                Rotate_Module( DC, Module, Angle_Rot_Module, false );
            }

            if( error == ESC )
                goto end_of_tst;
        }

end_of_tst:

        if( error == ESC )
            break;

        /* Place module. */
        CurrPosition = GetScreen()->GetCrossHairPosition();
        GetScreen()->SetCrossHairPosition( PosOK );
        PlaceModule( Module, DC );
        GetScreen()->SetCrossHairPosition( CurrPosition );

        Module->CalculateBoundingBox();

        GenModuleOnBoard( Module );
        Module->m_ModuleStatus |= MODULE_is_PLACED;
        Module->m_ModuleStatus &= ~MODULE_to_PLACE;
    }

    CurrPosition = memopos;

    Board.UnInitBoard();

    Route_Layer_TOP    = lay_tmp_TOP;
    Route_Layer_BOTTOM = lay_tmp_BOTTOM;

    Module = GetBoard()->m_Modules;

    for( ; Module != NULL; Module = Module->Next() )
    {
        Module->CalculateBoundingBox();
    }

    GetBoard()->m_Status_Pcb = 0;
    Compile_Ratsnest( DC, true );
    m_canvas->ReDraw( DC, true );
}


void PCB_EDIT_FRAME::DrawInfoPlace( wxDC* DC )
{
    int       color, ii, jj;
    int       ox, oy;
    MATRIX_CELL top_state, bottom_state;

    GRSetDrawMode( DC, GR_COPY );

    for( ii = 0; ii < Board.m_Nrows; ii++ )
    {
        oy = bbbox.GetY() + ( ii * Board.m_GridRouting );

        for( jj = 0; jj < Board.m_Ncols; jj++ )
        {
            ox = bbbox.GetX() + (jj * Board.m_GridRouting);
            color = BLACK;

            top_state    = GetCell( ii, jj, TOP );
            bottom_state = GetCell( ii, jj, BOTTOM );

            if( top_state & CELL_is_ZONE )
                color = BLUE;

            /* obstacles */
            if( ( top_state & CELL_is_EDGE ) || ( bottom_state & CELL_is_EDGE ) )
                color = WHITE;
            else if( top_state & ( HOLE | CELL_is_MODULE ) )
                color = LIGHTRED;
            else if( bottom_state & (HOLE | CELL_is_MODULE) )
                color = LIGHTGREEN;
            else /* Display the filling and keep out regions. */
            {
                if( GetDist( ii, jj, TOP ) || GetDist( ii, jj, BOTTOM ) )
                    color = DARKGRAY;
            }

            GRPutPixel( m_canvas->GetClipBox(), DC, ox, oy, color );
        }
    }
}


int PCB_EDIT_FRAME::GenPlaceBoard()
{
    int       jj, ii;
    int       NbCells;
    EDA_ITEM* PtStruct;
    wxString  msg;

    Board.UnInitBoard();

    bbbox = GetBoard()->ComputeBoundingBox( true );

    if( bbbox.GetWidth() == 0 && bbbox.GetHeight() == 0 )
    {
        DisplayError( this, _( "No PCB edge found, unknown board size!" ) );
        return 0;
    }

    /* The boundary box must have its start point on placing grid: */
    bbbox.SetX( bbbox.GetX() - ( bbbox.GetX() % Board.m_GridRouting ) );
    bbbox.SetY( bbbox.GetY() - ( bbbox.GetY() % Board.m_GridRouting ) );

    /* The boundary box must have its end point on placing grid: */
    wxPoint end = bbbox.GetEnd();
    end.x -= end.x % Board.m_GridRouting;
    end.x += Board.m_GridRouting;
    end.y -= end.y % Board.m_GridRouting;
    end.y += Board.m_GridRouting;
    bbbox.SetEnd( end );

    Nrows = bbbox.GetHeight() / Board.m_GridRouting;
    Ncols = bbbox.GetWidth() / Board.m_GridRouting;
    /* get a small margin for memory allocation: */
    Ncols  += 2; Nrows += 2;
    NbCells = Ncols * Nrows;

    m_messagePanel->EraseMsgBox();
    msg.Printf( wxT( "%d" ), Ncols );
    m_messagePanel->SetMessage( 1, _( "Cols" ), msg, GREEN );
    msg.Printf( wxT( "%d" ), Nrows );
    m_messagePanel->SetMessage( 7, _( "Lines" ), msg, GREEN );
    msg.Printf( wxT( "%d" ), NbCells );
    m_messagePanel->SetMessage( 14, _( "Cells." ), msg, YELLOW );

    /* Choose the number of board sides. */
    Nb_Sides = TWO_SIDES;

    m_messagePanel->SetMessage( 22, wxT( "S" ),
                                ( Nb_Sides == TWO_SIDES ) ? wxT( "2" ) : wxT( "1" ), WHITE );

    Board.InitBoard();

    /* Display memory usage. */
    msg.Printf( wxT( "%d" ), Board.m_MemSize / 1024 );
    m_messagePanel->SetMessage( 24, wxT( "Mem(Kb)" ), msg, CYAN );

    Route_Layer_BOTTOM = LAYER_N_FRONT;

    if( Nb_Sides == TWO_SIDES )
        Route_Layer_BOTTOM = LAYER_N_BACK;

    Route_Layer_TOP = LAYER_N_FRONT;

    /* Place the edge layer segments */
    PtStruct = GetBoard()->m_Drawings;
    TRACK TmpSegm( NULL );

    TmpSegm.SetLayer( -1 );
    TmpSegm.SetNet( -1 );
    TmpSegm.m_Width = Board.m_GridRouting / 2;

    for( ; PtStruct != NULL; PtStruct = PtStruct->Next() )
    {
        DRAWSEGMENT* DrawSegm;

        switch( PtStruct->Type() )
        {
        case PCB_LINE_T:
            DrawSegm = (DRAWSEGMENT*) PtStruct;

            if( DrawSegm->GetLayer() != EDGE_N )
                break;

            TmpSegm.SetStart( DrawSegm->GetStart() );
            TmpSegm.SetEnd(   DrawSegm->GetEnd() );
            TmpSegm.SetShape( DrawSegm->GetShape() );
            TmpSegm.m_Param = DrawSegm->GetAngle();

            TraceSegmentPcb( GetBoard(), &TmpSegm, HOLE | CELL_is_EDGE,
                             Board.m_GridRouting, WRITE_CELL );
            break;

        case PCB_TEXT_T:
        default:
            break;
        }
    }

    /* Init the point of attachment to the area. */
    OrCell( Nrows / 2, Ncols / 2, BOTTOM, CELL_is_ZONE );

    /* Fill bottom layer zones. */
    ii = 1;
    jj = 1;

    while( ii )
    {
        msg.Printf( wxT( "%d" ), jj++ );
        m_messagePanel->SetMessage(  50, _( "Loop" ), msg, CYAN );
        ii = propagate();
    }

    /* Initialize top layer. */
    if( Board.m_BoardSide[TOP] )
        memcpy( Board.m_BoardSide[TOP], Board.m_BoardSide[BOTTOM],
                NbCells * sizeof(MATRIX_CELL) );

    return 1;
}


/* Place module on board.
 */
void PCB_EDIT_FRAME::GenModuleOnBoard( MODULE* Module )
{
    int    ox, oy, fx, fy;
    int    marge = Board.m_GridRouting / 2;
    int    layerMask;
    D_PAD* Pad;

    ox = Module->m_BoundaryBox.GetX() - marge;
    fx = Module->m_BoundaryBox.GetRight() + marge;
    oy = Module->m_BoundaryBox.GetY() - marge;
    fy = Module->m_BoundaryBox.GetBottom() + marge;

    if( ox < bbbox.GetX() )
        ox = bbbox.GetX();

    if( ox > bbbox.GetRight() )
        ox = bbbox.GetRight();

    if( fx < bbbox.GetX() )
        fx = bbbox.GetX();

    if( fx > bbbox.GetRight() )
        fx = bbbox.GetRight();

    if( oy < bbbox.GetY() )
        oy = bbbox.GetY();

    if( oy > bbbox.GetBottom() )
        oy = bbbox.GetBottom();

    if( fy < bbbox.GetY() )
        fy = bbbox.GetY();

    if( fy > bbbox.GetBottom() )
        fy = bbbox.GetBottom();

    layerMask = 0;

    if( Module->GetLayer() == LAYER_N_FRONT )
        layerMask = LAYER_FRONT;

    if( Module->GetLayer() == LAYER_N_BACK )
        layerMask = LAYER_BACK;

    TraceFilledRectangle( GetBoard(), ox, oy, fx, fy, layerMask,
                          CELL_is_MODULE, WRITE_OR_CELL );

    int trackWidth = GetBoard()->m_NetClasses.GetDefault()->GetTrackWidth();
    int clearance  = GetBoard()->m_NetClasses.GetDefault()->GetClearance();

    /* Trace pads and surface safely. */
    marge = trackWidth + clearance;

    for( Pad = Module->m_Pads; Pad != NULL; Pad = Pad->Next() )
    {
        ::PlacePad( GetBoard(), Pad, CELL_is_MODULE, marge, WRITE_OR_CELL );
    }

    /* Trace clearance. */
    marge   = ( Board.m_GridRouting * Module->m_PadNum ) / GAIN;
    CreateKeepOutRectangle( GetBoard(), ox, oy, fx, fy, marge, KEEP_OUT_MARGIN, layerMask );
}


int PCB_EDIT_FRAME::GetOptimalModulePlacement( MODULE* aModule, wxDC* aDC )
{
    int     cx, cy;
    int     ox, oy, fx, fy; /* occupying part of the module focuses on the cursor */
    int     error = 1;
    int     showRat = 0;
    wxPoint LastPosOK;
    float   mincout, cout, Score;
    int     keepOut;
    bool    TstOtherSide;

    aModule->DisplayInfo( this );

    LastPosOK.x = bbbox.GetX();
    LastPosOK.y = bbbox.GetY();

    cx = aModule->m_Pos.x; cy = aModule->m_Pos.y;
    ox = aModule->m_BoundaryBox.GetX() - cx;
    fx = aModule->m_BoundaryBox.GetWidth() + ox;
    oy = aModule->m_BoundaryBox.GetY() - cy;
    fy = aModule->m_BoundaryBox.GetHeight() + oy;

    CurrPosition.x = bbbox.GetX() - ox;
    CurrPosition.y = bbbox.GetY() - oy;

    /* Module placement on grid. */
    CurrPosition.x -= CurrPosition.x % Board.m_GridRouting;
    CurrPosition.y -= CurrPosition.y % Board.m_GridRouting;

    g_Offset_Module.x = cx - CurrPosition.x;
    g_Offset_Module.y = cy - CurrPosition.y;
    GetBoard()->m_Status_Pcb &= ~RATSNEST_ITEM_LOCAL_OK;

    /* Test pads, a printed circuit with components of the 2 dimensions
     * can become a component on opposite side if there is at least 1 patch
     * appearing on the other side.
     */
    TstOtherSide = false;

    if( Nb_Sides == TWO_SIDES )
    {
        D_PAD* Pad;
        int otherLayerMask = LAYER_BACK;

        if( aModule->GetLayer() == LAYER_N_BACK )
            otherLayerMask = LAYER_FRONT;

        for( Pad = aModule->m_Pads; Pad != NULL; Pad = Pad->Next() )
        {
            if( ( Pad->m_layerMask & otherLayerMask ) == 0 )
                continue;

            TstOtherSide = true;
            break;
        }
    }

    DrawModuleOutlines( m_canvas, aDC, aModule );

    mincout = -1.0;
    SetStatusText( wxT( "Score ??, pos ??" ) );

    for( ; CurrPosition.x < bbbox.GetRight() - fx;
         CurrPosition.x += Board.m_GridRouting )
    {
        wxYield();

        if( m_canvas->GetAbortRequest() )
        {
            if( IsOK( this, _( "Ok to abort?" ) ) )
                return ESC;
            else
                m_canvas->SetAbortRequest( false );
        }

        cx = aModule->m_Pos.x; cy = aModule->m_Pos.y;
        aModule->m_BoundaryBox.SetX( ox + CurrPosition.x );
        aModule->m_BoundaryBox.SetY( oy + CurrPosition.y );

        DrawModuleOutlines( m_canvas, aDC, aModule );

        g_Offset_Module.x = cx - CurrPosition.x;
        CurrPosition.y    = bbbox.GetY() - oy;

        /* Placement on grid. */
        CurrPosition.y -= CurrPosition.y % Board.m_GridRouting;

        DrawModuleOutlines( m_canvas, aDC, aModule );

        for( ; CurrPosition.y < bbbox.GetBottom() - fy;
             CurrPosition.y += Board.m_GridRouting )
        {
            /* Erase traces. */
            DrawModuleOutlines( m_canvas, aDC, aModule );

            if( showRat )
                Compute_Ratsnest_PlaceModule( aDC );

            showRat = 0;
            aModule->m_BoundaryBox.SetX( ox + CurrPosition.x );
            aModule->m_BoundaryBox.SetY( oy + CurrPosition.y );

            g_Offset_Module.y = cy - CurrPosition.y;
            DrawModuleOutlines( m_canvas, aDC, aModule );
            keepOut = TstModuleOnBoard( GetBoard(), aModule, TstOtherSide );

            if( keepOut >= 0 ) /* c a d if the module can be placed. */
            {
                error = 0;
                build_ratsnest_module( aModule );
                cout = Compute_Ratsnest_PlaceModule( aDC );
                showRat = 1;
                Score = cout + (float) keepOut;

                if( (mincout >= Score ) || (mincout < 0 ) )
                {
                    LastPosOK = CurrPosition;
                    mincout   = Score;
                    wxString msg;
                    msg.Printf( wxT( "Score %d, pos %3.4f, %3.4f" ),
                                (int) mincout,
                                (float) LastPosOK.x / 10000,
                                (float) LastPosOK.y / 10000 );
                    SetStatusText( msg );
                }
            }

            if( showRat )
                Compute_Ratsnest_PlaceModule( aDC );

            showRat = 0;
        }
    }

    DrawModuleOutlines( m_canvas, aDC, aModule );  /* erasing the last traces */

    if( showRat )
        Compute_Ratsnest_PlaceModule( aDC );

    /* Regeneration of the modified variable. */
    aModule->m_BoundaryBox.SetX( ox + cx );
    aModule->m_BoundaryBox.SetY( oy + cy );
    CurrPosition = LastPosOK;

    GetBoard()->m_Status_Pcb &= ~( RATSNEST_ITEM_LOCAL_OK | LISTE_PAD_OK );

    MinCout = mincout;
    return error;
}


/* Test if the rectangular area (ux, ux .. y0, y1):
 * - is a free zone (except OCCUPED_By_MODULE returns)
 * - is on the working surface of the board (otherwise returns OUT_OF_BOARD)
 *
 * Returns 0 if OK
 */
int TstRectangle( BOARD* Pcb, int ux0, int uy0, int ux1, int uy1, int side )
{
    int          row, col;
    int          row_min, row_max, col_min, col_max;
    unsigned int data;

    ux0 -= Pcb->GetBoundingBox().GetX();
    uy0 -= Pcb->GetBoundingBox().GetY();
    ux1 -= Pcb->GetBoundingBox().GetX();
    uy1 -= Pcb->GetBoundingBox().GetY();

    row_max = uy1 / Board.m_GridRouting;
    col_max = ux1 / Board.m_GridRouting;
    row_min = uy0 / Board.m_GridRouting;

    if( uy0 > row_min * Board.m_GridRouting )
        row_min++;

    col_min = ux0 / Board.m_GridRouting;

    if( ux0 > col_min * Board.m_GridRouting )
        col_min++;

    if( row_min < 0 )
        row_min = 0;

    if( row_max >= ( Nrows - 1 ) )
        row_max = Nrows - 1;

    if( col_min < 0 )
        col_min = 0;

    if( col_max >= ( Ncols - 1 ) )
        col_max = Ncols - 1;

    for( row = row_min; row <= row_max; row++ )
    {
        for( col = col_min; col <= col_max; col++ )
        {
            data = GetCell( row, col, side );

            if( ( data & CELL_is_ZONE ) == 0 )
                return OUT_OF_BOARD;

            if( data & CELL_is_MODULE )
                return OCCUPED_By_MODULE;
        }
    }

    return 0;
}


/* Calculates and returns the clearance area of the rectangular surface
 * (ux, ux .. y0, y1):
 * (Sum of cells in terms of distance)
 */
unsigned int CalculateKeepOutArea( BOARD* Pcb, int ux0, int uy0, int ux1, int uy1, int side )
{
    int          row, col;
    int          row_min, row_max, col_min, col_max;
    unsigned int keepOut;

    ux0 -= Pcb->GetBoundingBox().GetX();
    uy0 -= Pcb->GetBoundingBox().GetY();
    ux1 -= Pcb->GetBoundingBox().GetX();
    uy1 -= Pcb->GetBoundingBox().GetY();

    row_max = uy1 / Board.m_GridRouting;
    col_max = ux1 / Board.m_GridRouting;
    row_min = uy0 / Board.m_GridRouting;

    if( uy0 > row_min * Board.m_GridRouting )
        row_min++;

    col_min = ux0 / Board.m_GridRouting;

    if( ux0 > col_min * Board.m_GridRouting )
        col_min++;

    if( row_min < 0 )
        row_min = 0;

    if( row_max >= ( Nrows - 1 ) )
        row_max = Nrows - 1;

    if( col_min < 0 )
        col_min = 0;

    if( col_max >= ( Ncols - 1 ) )
        col_max = Ncols - 1;

    keepOut = 0;

    for( row = row_min; row <= row_max; row++ )
    {
        for( col = col_min; col <= col_max; col++ )
        {
            keepOut += (int) GetDist( row, col, side );
        }
    }

    return keepOut;
}


/* Test if the module can be placed on the board.
 * Returns the value TstRectangle().
 * Module is known by its rectangle
 */
int TstModuleOnBoard( BOARD* Pcb, MODULE* Module, bool TstOtherSide )
{
    int ox, oy, fx, fy;
    int error, marge, side, otherside;

    side = TOP; otherside = BOTTOM;

    if( Module->GetLayer() == LAYER_N_BACK )
    {
        side = BOTTOM; otherside = TOP;
    }

    ox = Module->m_BoundaryBox.GetX();
    fx = Module->m_BoundaryBox.GetRight();
    oy = Module->m_BoundaryBox.GetY();
    fy = Module->m_BoundaryBox.GetBottom();

    error = TstRectangle( Pcb, ox, oy, fx, fy, side );

    if( error < 0 )
        return error;

    if( TstOtherSide )
    {
        error = TstRectangle( Pcb, ox, oy, fx, fy, otherside );

        if( error < 0 )
            return error;
    }

    marge = ( Board.m_GridRouting * Module->m_PadNum ) / GAIN;

    return CalculateKeepOutArea( Pcb, ox - marge, oy - marge, fx + marge, fy + marge, side );
}


float PCB_EDIT_FRAME::Compute_Ratsnest_PlaceModule( wxDC* DC )
{
    double cout, icout;
    int    ox, oy;
    int    fx, fy;
    int    dx, dy;

    if( ( GetBoard()->m_Status_Pcb & RATSNEST_ITEM_LOCAL_OK ) == 0 )
        return -1;

    cout = 0;

    int color = g_ColorsSettings.GetItemColor(RATSNEST_VISIBLE);

    if( AutoPlaceShowAll )
        GRSetDrawMode( DC, GR_XOR );

    for( unsigned ii = 0; ii < GetBoard()->m_LocalRatsnest.size(); ii++ )
    {
        RATSNEST_ITEM* pt_local_rats_nest = &GetBoard()->m_LocalRatsnest[ii];

        if( !( pt_local_rats_nest->m_Status & LOCAL_RATSNEST_ITEM ) )
        {
            ox = pt_local_rats_nest->m_PadStart->GetPosition().x - g_Offset_Module.x;
            oy = pt_local_rats_nest->m_PadStart->GetPosition().y - g_Offset_Module.y;
            fx = pt_local_rats_nest->m_PadEnd->GetPosition().x;
            fy = pt_local_rats_nest->m_PadEnd->GetPosition().y;

            if( AutoPlaceShowAll )
            {
                GRLine( m_canvas->GetClipBox(), DC, ox, oy, fx, fy, 0, color );
            }

            /* Cost of the ratsnest. */
            dx = fx - ox;
            dy = fy - oy;

            dx = abs( dx );
            dy = abs( dy );

            if( dx < dy )
                EXCHG( dx, dy );  /* dx >= dy */

            /* Cost of the longest connection. */
            icout = (float) dx * dx;

            /* Cost of inclination. */
            icout += 3 * (float) dy * dy;
            icout  = sqrt( icout );
            cout  += icout; /* Total cost = sum of costs of each connection. */
        }
    }

    return (float) cout;
}


/**
 * Function CreateKeepOutRectangle
 * builds the cost map.
 * Cells ( in Dist mao ) inside the rect x0,y0 a x1,y1 are
 *  incremented by value aKeepOut
 *  Cell outside this rectangle, but inside the rectangle
 *  x0,y0 -marge to x1,y1 + marge sont incrementede by a decreasing value
 *  (aKeepOut ... 0). The decreasing value de pends on the distance to the first rectangle
 *  Therefore the cost is high in rect x0,y0 a x1,y1, and decrease outside this rectangle
 */
static void CreateKeepOutRectangle( BOARD* Pcb,
                                    int    ux0,
                                    int    uy0,
                                    int    ux1,
                                    int    uy1,
                                    int    marge,
                                    int    aKeepOut,
                                    int    aLayerMask )
{
    int      row, col;
    int      row_min, row_max, col_min, col_max, pmarge;
    int      trace = 0;
    DIST_CELL data, LocalKeepOut;
    int      lgain, cgain;

    if( aLayerMask & GetLayerMask( Route_Layer_BOTTOM ) )
        trace = 1;     /* Trace on bottom layer. */

    if( ( aLayerMask & GetLayerMask( Route_Layer_TOP ) ) && Nb_Sides )
        trace |= 2;    /* Trace on top layer. */

    if( trace == 0 )
        return;

    ux0 -= Pcb->GetBoundingBox().GetX();
    uy0 -= Pcb->GetBoundingBox().GetY();
    ux1 -= Pcb->GetBoundingBox().GetX();
    uy1 -= Pcb->GetBoundingBox().GetY();

    ux0 -= marge; ux1 += marge;
    uy0 -= marge; uy1 += marge;

    pmarge = marge / Board.m_GridRouting;

    if( pmarge < 1 )
        pmarge = 1;

    /* Calculate the coordinate limits of the rectangle. */
    row_max = uy1 / Board.m_GridRouting;
    col_max = ux1 / Board.m_GridRouting;
    row_min = uy0 / Board.m_GridRouting;

    if( uy0 > row_min * Board.m_GridRouting )
        row_min++;

    col_min = ux0 / Board.m_GridRouting;

    if( ux0 > col_min * Board.m_GridRouting )
        col_min++;

    if( row_min < 0 )
        row_min = 0;

    if( row_max >= (Nrows - 1) )
        row_max = Nrows - 1;

    if( col_min < 0 )
        col_min = 0;

    if( col_max >= (Ncols - 1) )
        col_max = Ncols - 1;

    for( row = row_min; row <= row_max; row++ )
    {
        lgain = 256;

        if( row < pmarge )
            lgain = ( 256 * row ) / pmarge;
        else if( row > row_max - pmarge )
            lgain = ( 256 * ( row_max - row ) ) / pmarge;

        for( col = col_min; col <= col_max; col++ )
        {
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
                data = GetDist( row, col, BOTTOM ) + LocalKeepOut;
                SetDist( row, col, BOTTOM, data );
            }

            if( trace & 2 )
            {
                data = GetDist( row, col, TOP );
                data = MAX( data, LocalKeepOut );
                SetDist( row, col, TOP, data );
            }
        }
    }
}


/* Sort routines */
static bool Tri_PlaceModules( MODULE* ref, MODULE* compare )
{
    double ff1, ff2;

    ff1 = ref->m_Surface * ref->m_PadNum;
    ff2 = compare->m_Surface * compare->m_PadNum;
    return ff2 < ff1;
}


static bool Tri_RatsModules( MODULE* ref, MODULE* compare )
{
    double ff1, ff2;

    ff1 = ref->m_Surface * ref->flag;
    ff2 = compare->m_Surface * compare->flag;
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
    MODULE*  Module;
    std::vector <MODULE*> moduleList;

    // Build sorted footprints list (sort by decreasing size )
    Module = pcbframe->GetBoard()->m_Modules;

    for( ; Module != NULL; Module = Module->Next() )
    {
        Module->CalculateBoundingBox();
        moduleList.push_back(Module);
    }

    sort( moduleList.begin(), moduleList.end(), Tri_PlaceModules );

    for( unsigned ii = 0; ii < moduleList.size(); ii++ )
    {
        Module = moduleList[ii];
        Module->flag = 0;

        if( !( Module->m_ModuleStatus & MODULE_to_PLACE ) )
            continue;

        pcbframe->GetBoard()->m_Status_Pcb &= ~RATSNEST_ITEM_LOCAL_OK;
        Module->DisplayInfo( pcbframe );
        pcbframe->build_ratsnest_module( Module );

        /* Calculate external ratsnest. */
        for( unsigned ii = 0; ii < pcbframe->GetBoard()->m_LocalRatsnest.size(); ii++ )
        {
            if( ( pcbframe->GetBoard()->m_LocalRatsnest[ii].m_Status &
                  LOCAL_RATSNEST_ITEM ) == 0 )
                Module->flag++;
        }
    }

    pcbframe->GetBoard()->m_Status_Pcb &= ~RATSNEST_ITEM_LOCAL_OK;

    sort( moduleList.begin(), moduleList.end(), Tri_RatsModules );

    /* Search for "best" module. */
    MODULE* bestModule = NULL;
    MODULE* altModule = NULL;

    for( unsigned ii = 0; ii < moduleList.size(); ii++ )
    {
        Module = moduleList[ii];

        if( !( Module->m_ModuleStatus & MODULE_to_PLACE ) )
            continue;

        altModule = Module;

        if( Module->flag == 0 )
            continue;

        bestModule = Module;
        break;
    }

    if( bestModule )
        return bestModule;
    else
        return altModule;
}


int PCB_EDIT_FRAME::propagate()
{
    int       row, col;
    long      current_cell, old_cell_H;
    std::vector< long > pt_cell_V;
    int       nbpoints = 0;

#define NO_CELL_ZONE (HOLE | CELL_is_EDGE | CELL_is_ZONE)
    wxString  msg;

    m_messagePanel->SetMessage( 57, wxT( "Detect" ), msg, CYAN );
    m_messagePanel->SetMessage( -1, wxEmptyString, wxT( "1" ), CYAN );

    pt_cell_V.reserve( MAX( Nrows, Ncols ) );
    fill( pt_cell_V.begin(), pt_cell_V.end(), 0 );

    // Search from left to right and top to bottom.
    for( row = 0; row < Nrows; row++ )
    {
        old_cell_H = 0;

        for( col = 0; col < Ncols; col++ )
        {
            current_cell = GetCell( row, col, BOTTOM ) & NO_CELL_ZONE;

            if( current_cell == 0 )  /* a free cell is found */
            {
                if( (old_cell_H & CELL_is_ZONE) || (pt_cell_V[col] & CELL_is_ZONE) )
                {
                    OrCell( row, col, BOTTOM, CELL_is_ZONE );
                    current_cell = CELL_is_ZONE;
                    nbpoints++;
                }
            }

            pt_cell_V[col] = old_cell_H = current_cell;
        }
    }

    // Search from right to left and top to bottom/
    m_messagePanel->SetMessage( -1, wxEmptyString, wxT( "2" ), CYAN );
    fill( pt_cell_V.begin(), pt_cell_V.end(), 0 );

    for( row = 0; row < Nrows; row++ )
    {
        old_cell_H = 0;

        for( col = Ncols - 1; col >= 0; col-- )
        {
            current_cell = GetCell( row, col, BOTTOM ) & NO_CELL_ZONE;

            if( current_cell == 0 )  /* a free cell is found */
            {
                if( (old_cell_H & CELL_is_ZONE) || (pt_cell_V[col] & CELL_is_ZONE) )
                {
                    OrCell( row, col, BOTTOM, CELL_is_ZONE );
                    current_cell = CELL_is_ZONE;
                    nbpoints++;
                }
            }

            pt_cell_V[col] = old_cell_H = current_cell;
        }
    }

    // Search from bottom to top and right to left.
    m_messagePanel->SetMessage( -1, wxEmptyString, wxT( "3" ), CYAN );
    fill( pt_cell_V.begin(), pt_cell_V.end(), 0 );

    for( col = Ncols - 1; col >= 0; col-- )
    {
        old_cell_H = 0;

        for( row = Nrows - 1; row >= 0; row-- )
        {
            current_cell = GetCell( row, col, BOTTOM ) & NO_CELL_ZONE;

            if( current_cell == 0 )  /* a free cell is found */
            {
                if( (old_cell_H & CELL_is_ZONE) || (pt_cell_V[row] & CELL_is_ZONE) )
                {
                    OrCell( row, col, BOTTOM, CELL_is_ZONE );
                    current_cell = CELL_is_ZONE;
                    nbpoints++;
                }
            }

            pt_cell_V[row] = old_cell_H = current_cell;
        }
    }

    // Search from bottom to top and left to right.
    m_messagePanel->SetMessage( -1, wxEmptyString, wxT( "4" ), CYAN );
    fill( pt_cell_V.begin(), pt_cell_V.end(), 0 );

    for( col = 0; col < Ncols; col++ )
    {
        old_cell_H = 0;

        for( row = Nrows - 1; row >= 0; row-- )
        {
            current_cell = GetCell( row, col, BOTTOM ) & NO_CELL_ZONE;

            if( current_cell == 0 )  /* a free cell is found */
            {
                if( (old_cell_H & CELL_is_ZONE) || (pt_cell_V[row] & CELL_is_ZONE) )
                {
                    OrCell( row, col, BOTTOM, CELL_is_ZONE );
                    current_cell = CELL_is_ZONE;
                    nbpoints++;
                }
            }

            pt_cell_V[row] = old_cell_H = current_cell;
        }
    }

    return nbpoints;
}
