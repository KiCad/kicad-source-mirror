/*******************************************/
/* Routines to automatically place MODULES */
/*******************************************/

#include "fctsys.h"
#include "gr_basic.h"
#include "common.h"
#include "class_drawpanel.h"
#include "confirm.h"
#include "pcbnew.h"
#include "wxPcbStruct.h"
#include "autorout.h"
#include "zones.h"
#include "cell.h"
#include "class_board_design_settings.h"
#include "colors_selection.h"

#include "protos.h"


#define GAIN     16
#define PENALITE 500

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

static wxPoint CurrPosition; // Current position of the current module
                             // placement
static bool    AutoPlaceShowAll = TRUE;

float          MinCout;

static int  TstModuleOnBoard( BOARD* Pcb, MODULE* Module, bool TstOtherSide );

static void TracePenaliteRectangle( BOARD* Pcb,
                                    int    ux0,
                                    int    uy0,
                                    int    ux1,
                                    int    uy1,
                                    int    marge,
                                    int    Penalite,
                                    int    masque_layer );
static MODULE* PickModule( WinEDA_PcbFrame* pcbframe, wxDC* DC );


/* Routine to automatically place components in the contour of the PCB
 * The components with the FIXED status are not moved.  If the menu is
 * calling the placement of 1 module, it will be replaced.
 */
void WinEDA_PcbFrame::AutoPlaceModule( MODULE* Module,
                                       int     place_mode,
                                       wxDC*   DC )
{
    int      ii, activ;
    MODULE*  ThisModule = NULL;
    wxPoint  PosOK;
    wxPoint  memopos;
    int      error;
    int      NbModules = 0;
    int      NbTotalModules = 0;
    float    Pas;
    int      lay_tmp_TOP, lay_tmp_BOTTOM, OldPasRoute;

    if( GetBoard()->m_Modules == NULL )
        return;

    DrawPanel->m_AbortRequest = FALSE;
    DrawPanel->m_AbortEnable  = TRUE;

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
    OldPasRoute    = g_GridRoutingSize;

    g_GridRoutingSize = (int) GetScreen()->GetGridSize().x;

    // Ensure g_GridRoutingSize has a reasonnable value:
    if( g_GridRoutingSize < 10 )
        g_GridRoutingSize = 10;                      // Min value = 1/1000 inch

    /* Compute module parmeters used in auto place */
    Module = GetBoard()->m_Modules;
    NbTotalModules = 0;
    for( ; Module != NULL; Module = Module->Next() )
    {
        Module->Set_Rectangle_Encadrement();
        Module->SetRectangleExinscrit();
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
                Module->m_ModuleStatus |= MODULE_to_PLACE;
            break;

        case PLACE_OUT_OF_BOARD:
            Module->m_ModuleStatus &= ~MODULE_is_PLACED;
            if( Module->m_ModuleStatus & MODULE_is_LOCKED )
                break;
            if( !GetBoard()->m_BoundaryBox.Inside( Module->m_Pos ) )
                Module->m_ModuleStatus |= MODULE_to_PLACE;
            break;

        case PLACE_ALL:
            Module->m_ModuleStatus &= ~MODULE_is_PLACED;
            if( Module->m_ModuleStatus & MODULE_is_LOCKED )
                break;
            Module->m_ModuleStatus |= MODULE_to_PLACE;
            break;

        case PLACE_INCREMENTAL:
            if( Module->m_ModuleStatus & MODULE_is_LOCKED )
            {
                Module->m_ModuleStatus &= ~MODULE_is_PLACED; break;
            }
            if( !(Module->m_ModuleStatus & MODULE_is_PLACED) )
                Module->m_ModuleStatus |= MODULE_to_PLACE;
            break;
        }


        if( Module->m_ModuleStatus & MODULE_to_PLACE )  // Erase from screen
        {
            NbModules++;
            Module->Draw( DrawPanel, DC, GR_XOR );
        }
        else
        {
            GenModuleOnBoard( Module );
        }
    }

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

        error     = RecherchePlacementModule( Module, DC );
        BestScore = MinCout;
        PosOK     = CurrPosition;
        if( error == ESC )
            goto end_of_tst;

        /* Determine if the best orientation of a module is 180. */
        ii = Module->m_CntRot180 & 0x0F;

        if( ii != 0 )
        {
            int Angle_Rot_Module = 1800;
            Rotate_Module( DC, Module, Angle_Rot_Module, FALSE );
            Module->SetRectangleExinscrit();
            error    = RecherchePlacementModule( Module, DC );
            MinCout *= OrientPenality[ii];
            if( BestScore > MinCout )   /* This orientation is best. */
            {
                PosOK     = CurrPosition;
                BestScore = MinCout;
            }
            else
            {
                Angle_Rot_Module = -1800;
                Rotate_Module( DC, Module, Angle_Rot_Module, FALSE );
            }
            if( error == ESC )
                goto end_of_tst;
        }

        /* Determine if the best orientation of a module is 90. */
        ii = Module->m_CntRot90 & 0x0F;
        if( ii != 0 )
        {
            int Angle_Rot_Module = 900;
            Rotate_Module( DC, Module, Angle_Rot_Module, FALSE );
            error    = RecherchePlacementModule( Module, DC );
            MinCout *= OrientPenality[ii];
            if( BestScore > MinCout )   /* This orientation is best. */
            {
                PosOK     = CurrPosition;
                BestScore = MinCout;
            }
            else
            {
                Angle_Rot_Module = -900;
                Rotate_Module( DC, Module, Angle_Rot_Module, FALSE );
            }
            if( error == ESC )
                goto end_of_tst;
        }

        /*  Determine if the best orientation of a module is 270. */
        ii = (Module->m_CntRot90 >> 4 ) & 0x0F;
        if( ii != 0 )
        {
            int Angle_Rot_Module = 2700;
            Rotate_Module( DC, Module, Angle_Rot_Module, FALSE );
            error    = RecherchePlacementModule( Module, DC );
            MinCout *= OrientPenality[ii];
            if( BestScore > MinCout )   /* This orientation is best. */
            {
                PosOK     = CurrPosition;
                BestScore = MinCout;
            }
            else
            {
                Angle_Rot_Module = -2700;
                Rotate_Module( DC, Module, Angle_Rot_Module, FALSE );
            }
            if( error == ESC )
                goto end_of_tst;
        }

end_of_tst:

        if( error == ESC )
            break;

        /* Place module. */
        CurrPosition = GetScreen()->m_Curseur;
        GetScreen()->m_Curseur = PosOK;
        Place_Module( Module, DC );
        GetScreen()->m_Curseur = CurrPosition;

        Module->Set_Rectangle_Encadrement();
        Module->SetRectangleExinscrit();

        GenModuleOnBoard( Module );
        Module->m_ModuleStatus |= MODULE_is_PLACED;
        Module->m_ModuleStatus &= ~MODULE_to_PLACE;
    }

    CurrPosition = memopos;

    Board.UnInitBoard();

    Route_Layer_TOP    = lay_tmp_TOP;
    Route_Layer_BOTTOM = lay_tmp_BOTTOM;
    g_GridRoutingSize  = OldPasRoute;

    Module = GetBoard()->m_Modules;
    for( ; Module != NULL; Module = Module->Next() )
    {
        Module->Set_Rectangle_Encadrement();
    }

    GetBoard()->m_Status_Pcb = 0;
    Compile_Ratsnest( DC, true );
    DrawPanel->ReDraw( DC, TRUE );

    DrawPanel->m_AbortEnable = FALSE;
}


void WinEDA_PcbFrame::DrawInfoPlace( wxDC* DC )
{
    int       color, ii, jj;
    int       ox, oy;
    BoardCell top_state, bottom_state;

    GRSetDrawMode( DC, GR_COPY );
    for( ii = 0; ii < Nrows; ii++ )
    {
        oy = GetBoard()->m_BoundaryBox.m_Pos.y + ( ii * g_GridRoutingSize );

        for( jj = 0; jj < Ncols; jj++ )
        {
            ox = GetBoard()->m_BoundaryBox.m_Pos.x +
                 (jj * g_GridRoutingSize);
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

            GRPutPixel( &DrawPanel->m_ClipBox, DC, ox, oy, color );
        }
    }
}


/* Generate board (component side copper + rating):
 * Allocate the memory needed to represent in "bitmap" on the grid
 * Current:
 * - The size of clearance area of component (the board)
 * - The bitmap PENALTIES
 * And initialize the cells of the board has
 * - Hole in the cells occupied by a segment EDGE
 * - CELL_is_ZONE for cell internal contour EDGE (if closed)
 *
 * Placement surface (board) gives the cells internal to the contour
 * PCB, and among the latter the free cells and cells already occupied
 *
 * The bitmap PENALTIES give cells occupied by the modules,
 * Plus a surface penalty related to the number of pads of the module
 *
 * Bitmap of the penalty is set to 0
 * Occupation cell is a 0 leaves
 */
int WinEDA_PcbFrame::GenPlaceBoard()
{
    int       jj, ii;
    int       NbCells;
    EDA_ITEM* PtStruct;
    wxString  msg;

    Board.UnInitBoard();

    if( !SetBoardBoundaryBoxFromEdgesOnly() )
    {
        DisplayError( this, _( "No PCB edge found, unknown board size!" ) );
        return 0;
    }

    /* The boundary box must have its start point on placing grid: */
    GetBoard()->m_BoundaryBox.m_Pos.x -= GetBoard()->m_BoundaryBox.m_Pos.x %
                                         g_GridRoutingSize;
    GetBoard()->m_BoundaryBox.m_Pos.y -= GetBoard()->m_BoundaryBox.m_Pos.y %
                                         g_GridRoutingSize;
    /* The boundary box must have its end point on placing grid: */
    wxPoint end = GetBoard()->m_BoundaryBox.GetEnd();
    end.x -= end.x % g_GridRoutingSize; end.x += g_GridRoutingSize;
    end.y -= end.y % g_GridRoutingSize; end.y += g_GridRoutingSize;
    GetBoard()->m_BoundaryBox.SetEnd( end );

    Nrows = GetBoard()->m_BoundaryBox.GetHeight() / g_GridRoutingSize;
    Ncols = GetBoard()->m_BoundaryBox.GetWidth() / g_GridRoutingSize;
    /* get a small margin for memory allocation: */
    Ncols  += 2; Nrows += 2;
    NbCells = Ncols * Nrows;

    MsgPanel->EraseMsgBox();
    msg.Printf( wxT( "%d" ), Ncols );
    Affiche_1_Parametre( this, 1, _( "Cols" ), msg, GREEN );
    msg.Printf( wxT( "%d" ), Nrows );
    Affiche_1_Parametre( this, 7, _( "Lines" ), msg, GREEN );
    msg.Printf( wxT( "%d" ), NbCells );
    Affiche_1_Parametre( this, 14, _( "Cells." ), msg, YELLOW );

    /* Choose the number of board sides. */
    Nb_Sides = TWO_SIDES;

    Affiche_1_Parametre( this, 22, wxT( "S" ),
                         ( Nb_Sides == TWO_SIDES ) ? wxT( "2" ) : wxT( "1" ),
                         WHITE );

    Board.InitBoard();

    /* Display memory usage. */
    msg.Printf( wxT( "%d" ), Board.m_MemSize / 1024 );
    Affiche_1_Parametre( this, 24, wxT( "Mem(Kb)" ), msg, CYAN );

    Route_Layer_BOTTOM = LAYER_N_FRONT;
    if( Nb_Sides == TWO_SIDES )
        Route_Layer_BOTTOM = LAYER_N_BACK;
    Route_Layer_TOP = LAYER_N_FRONT;

    /* Place the edge layer segments */
    PtStruct = GetBoard()->m_Drawings;
    TRACK TmpSegm( NULL );

    TmpSegm.SetLayer( -1 );
    TmpSegm.SetNet( -1 );
    TmpSegm.m_Width = g_GridRoutingSize / 2;
    for( ; PtStruct != NULL; PtStruct = PtStruct->Next() )
    {
        DRAWSEGMENT* DrawSegm;

        switch( PtStruct->Type() )
        {
        case TYPE_DRAWSEGMENT:
            DrawSegm = (DRAWSEGMENT*) PtStruct;
            if( DrawSegm->GetLayer() != EDGE_N )
                break;

            TmpSegm.m_Start = DrawSegm->m_Start;
            TmpSegm.m_End   = DrawSegm->m_End;
            TmpSegm.m_Shape = DrawSegm->m_Shape;
            TmpSegm.m_Param = DrawSegm->m_Angle;

            TraceSegmentPcb( GetBoard(), &TmpSegm, HOLE | CELL_is_EDGE,
                             g_GridRoutingSize, WRITE_CELL );
            break;

        case TYPE_TEXTE:
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
        Affiche_1_Parametre( this, 50, _( "Loop" ), msg, CYAN );
        ii = Propagation( this );
    }

    /* Initialize top layer. */
    if( Board.m_BoardSide[TOP] )
        memcpy( Board.m_BoardSide[TOP], Board.m_BoardSide[BOTTOM],
                NbCells * sizeof(BoardCell) );

    return 1;
}


/* Place module on board.
 */
void WinEDA_PcbFrame::GenModuleOnBoard( MODULE* Module )
{
    int    ox, oy, fx, fy, Penalite;
    int    marge = g_GridRoutingSize / 2;
    int    masque_layer;
    D_PAD* Pad;

    ox = Module->m_RealBoundaryBox.m_Pos.x - marge;
    fx = Module->m_RealBoundaryBox.GetRight() + marge;
    oy = Module->m_RealBoundaryBox.m_Pos.y - marge;
    fy = Module->m_RealBoundaryBox.GetBottom() + marge;

    if( ox < GetBoard()->m_BoundaryBox.m_Pos.x )
        ox = GetBoard()->m_BoundaryBox.m_Pos.x;
    if( ox > GetBoard()->m_BoundaryBox.GetRight() )
        ox = GetBoard()->m_BoundaryBox.GetRight();

    if( fx < GetBoard()->m_BoundaryBox.m_Pos.x )
        fx = GetBoard()->m_BoundaryBox.m_Pos.x;
    if( fx > GetBoard()->m_BoundaryBox.GetRight() )
        fx = GetBoard()->m_BoundaryBox.GetRight();

    if( oy < GetBoard()->m_BoundaryBox.m_Pos.y )
        oy = GetBoard()->m_BoundaryBox.m_Pos.y;
    if( oy > GetBoard()->m_BoundaryBox.GetBottom() )
        oy = GetBoard()->m_BoundaryBox.GetBottom();

    if( fy < GetBoard()->m_BoundaryBox.m_Pos.y )
        fy = GetBoard()->m_BoundaryBox.m_Pos.y;
    if( fy > GetBoard()->m_BoundaryBox.GetBottom() )
        fy = GetBoard()->m_BoundaryBox.GetBottom();

    masque_layer = 0;
    if( Module->GetLayer() == LAYER_N_FRONT )
        masque_layer = LAYER_FRONT;
    if( Module->GetLayer() == LAYER_N_BACK )
        masque_layer = LAYER_BACK;

    TraceFilledRectangle( GetBoard(), ox, oy, fx, fy, masque_layer,
                          CELL_is_MODULE, WRITE_OR_CELL );

    int trackWidth = GetBoard()->m_NetClasses.GetDefault()->GetTrackWidth();
    int clearance  = GetBoard()->m_NetClasses.GetDefault()->GetClearance();

    /* Trace pads and surface safely. */
    marge = trackWidth + clearance;

    for( Pad = Module->m_Pads; Pad != NULL; Pad = Pad->Next() )
    {
        Place_1_Pad_Board( GetBoard(), Pad, CELL_is_MODULE, marge,
                           WRITE_OR_CELL );
    }

    /* Trace clearance. */
    marge    = (g_GridRoutingSize * Module->m_PadNum ) / GAIN;
    Penalite = PENALITE;
    TracePenaliteRectangle( GetBoard(), ox, oy, fx, fy, marge, Penalite,
                            masque_layer );
}


/*
 * Search for the optimal position of the module.
 * Entree:
 * Module tip MODULE struct module's place.
 * Returns:
 * 1 if placement impossible, 0 if OK
 * = MinCout and external variable = cost of best placement
 */
int WinEDA_PcbFrame::RecherchePlacementModule( MODULE* Module, wxDC* DC )
{
    int     cx, cy;
    int     ox, oy, fx, fy; /* occupying part of the module focuses on the
                             * cursor */
    int     error = 1;
    int     DisplayChevelu = 0;
    wxPoint LastPosOK;
    float   mincout, cout, Score;
    int     Penalite;
    bool    TstOtherSide;

    Module->DisplayInfo( this );

    LastPosOK.x = GetBoard()->m_BoundaryBox.m_Pos.x;
    LastPosOK.y = GetBoard()->m_BoundaryBox.m_Pos.y;

    cx = Module->m_Pos.x; cy = Module->m_Pos.y;
    ox = Module->m_RealBoundaryBox.m_Pos.x - cx;
    fx = Module->m_RealBoundaryBox.m_Size.x + ox;
    oy = Module->m_RealBoundaryBox.m_Pos.y - cy;
    fy = Module->m_RealBoundaryBox.m_Size.y + oy;

    CurrPosition.x = GetBoard()->m_BoundaryBox.m_Pos.x - ox;
    CurrPosition.y = GetBoard()->m_BoundaryBox.m_Pos.y - oy;
    /* Module placement on grid. */
    CurrPosition.x -= CurrPosition.x % g_GridRoutingSize;
    CurrPosition.y -= CurrPosition.y % g_GridRoutingSize;

    g_Offset_Module.x = cx - CurrPosition.x;
    g_Offset_Module.y = cy - CurrPosition.y;
    GetBoard()->m_Status_Pcb &= ~RATSNEST_ITEM_LOCAL_OK;

    /* Test pads, a printed circuit with components of the 2 dimensions
     * can become a component on opposite side if there is at least 1 patch
     * appearing on the other side.
     */
    TstOtherSide = FALSE;
    if( Nb_Sides == TWO_SIDES )
    {
        D_PAD* Pad; int masque_otherlayer;
        masque_otherlayer = LAYER_BACK;
        if( Module->GetLayer() == LAYER_N_BACK )
            masque_otherlayer = LAYER_FRONT;

        for( Pad = Module->m_Pads; Pad != NULL; Pad = Pad->Next() )
        {
            if( ( Pad->m_Masque_Layer & masque_otherlayer ) == 0 )
                continue;
            TstOtherSide = TRUE;
            break;
        }
    }


    DrawModuleOutlines( DrawPanel, DC, Module );

    mincout = -1.0;
    Affiche_Message( wxT( "Score ??, pos ??" ) );

    for( ; CurrPosition.x < GetBoard()->m_BoundaryBox.GetRight() - fx;
         CurrPosition.x += g_GridRoutingSize )
    {
        wxYield();
        if( DrawPanel->m_AbortRequest )
        {
            if( IsOK( this, _( "Ok to abort?" ) ) )
                return ESC;
            else
                DrawPanel->m_AbortRequest = FALSE;
        }

        cx = Module->m_Pos.x; cy = Module->m_Pos.y;
        Module->m_RealBoundaryBox.m_Pos.x = ox + CurrPosition.x;
        Module->m_RealBoundaryBox.m_Pos.y = oy + CurrPosition.y;

        DrawModuleOutlines( DrawPanel, DC, Module );

        g_Offset_Module.x = cx - CurrPosition.x;
        CurrPosition.y    = GetBoard()->m_BoundaryBox.m_Pos.y - oy;
        /* Placement on grid. */
        CurrPosition.y -= CurrPosition.y % g_GridRoutingSize;

        DrawModuleOutlines( DrawPanel, DC, Module );

        for( ; CurrPosition.y < GetBoard()->m_BoundaryBox.GetBottom() - fy;
             CurrPosition.y += g_GridRoutingSize )
        {
            /* Erase traces. */
            DrawModuleOutlines( DrawPanel, DC, Module );
            if( DisplayChevelu )
                Compute_Ratsnest_PlaceModule( DC );
            DisplayChevelu = 0;
            Module->m_RealBoundaryBox.m_Pos.x = ox + CurrPosition.x;
            Module->m_RealBoundaryBox.m_Pos.y = oy + CurrPosition.y;

            g_Offset_Module.y = cy - CurrPosition.y;
            DrawModuleOutlines( DrawPanel, DC, Module );
            Penalite = TstModuleOnBoard( GetBoard(), Module, TstOtherSide );
            if( Penalite >= 0 ) /* c a d if the module can be placed. */
            {
                error = 0;
                build_ratsnest_module( DC, Module );
                cout = Compute_Ratsnest_PlaceModule( DC );
                DisplayChevelu = 1;
                Score = cout + (float) Penalite;

                if( (mincout >= Score ) || (mincout < 0 ) )
                {
                    LastPosOK = CurrPosition;
                    mincout   = Score;
                    wxString msg;
                    msg.Printf( wxT( "Score %d, pos %3.4f, %3.4f" ),
                                (int) mincout,
                                (float) LastPosOK.x / 10000,
                                (float) LastPosOK.y / 10000 );
                    Affiche_Message( msg );
                }
            }
            if( DisplayChevelu )
                Compute_Ratsnest_PlaceModule( DC );
            DisplayChevelu = 0;
        }
    }

    DrawModuleOutlines( DrawPanel, DC, Module );  /* erasing the last traces */
    if( DisplayChevelu )
        Compute_Ratsnest_PlaceModule( DC );

    /* Regeneration of the modified variable. */
    Module->m_RealBoundaryBox.m_Pos.x = ox + cx;
    Module->m_RealBoundaryBox.m_Pos.y = oy + cy;
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

    ux0 -= Pcb->m_BoundaryBox.m_Pos.x;
    uy0 -= Pcb->m_BoundaryBox.m_Pos.y;
    ux1 -= Pcb->m_BoundaryBox.m_Pos.x;
    uy1 -= Pcb->m_BoundaryBox.m_Pos.y;

    row_max = uy1 / g_GridRoutingSize;
    col_max = ux1 / g_GridRoutingSize;
    row_min = uy0 / g_GridRoutingSize;
    if( uy0 > row_min * g_GridRoutingSize )
        row_min++;
    col_min = ux0 / g_GridRoutingSize;
    if( ux0 > col_min * g_GridRoutingSize )
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
unsigned int CalculePenaliteRectangle( BOARD* Pcb, int ux0, int uy0,
                                       int ux1, int uy1, int side )
{
    int          row, col;
    int          row_min, row_max, col_min, col_max;
    unsigned int Penalite;

    ux0 -= Pcb->m_BoundaryBox.m_Pos.x;
    uy0 -= Pcb->m_BoundaryBox.m_Pos.y;
    ux1 -= Pcb->m_BoundaryBox.m_Pos.x;
    uy1 -= Pcb->m_BoundaryBox.m_Pos.y;

    row_max = uy1 / g_GridRoutingSize;
    col_max = ux1 / g_GridRoutingSize;
    row_min = uy0 / g_GridRoutingSize;
    if( uy0 > row_min * g_GridRoutingSize )
        row_min++;
    col_min = ux0 / g_GridRoutingSize;
    if( ux0 > col_min * g_GridRoutingSize )
        col_min++;

    if( row_min < 0 )
        row_min = 0;
    if( row_max >= ( Nrows - 1 ) )
        row_max = Nrows - 1;
    if( col_min < 0 )
        col_min = 0;
    if( col_max >= ( Ncols - 1 ) )
        col_max = Ncols - 1;

    Penalite = 0;
    for( row = row_min; row <= row_max; row++ )
    {
        for( col = col_min; col <= col_max; col++ )
        {
            Penalite += (int) GetDist( row, col, side );
        }
    }

    return Penalite;
}


/* Test if the module can be placed on the board.
 * Returns the value TstRectangle().
 * Module is known by its rectangle
 */
int TstModuleOnBoard( BOARD* Pcb, MODULE* Module, bool TstOtherSide )
{
    int ox, oy, fx, fy;
    int error, Penalite, marge, side, otherside;

    side = TOP; otherside = BOTTOM;
    if( Module->GetLayer() == LAYER_N_BACK )
    {
        side = BOTTOM; otherside = TOP;
    }

    ox = Module->m_RealBoundaryBox.m_Pos.x;
    fx = Module->m_RealBoundaryBox.GetRight();
    oy = Module->m_RealBoundaryBox.m_Pos.y;
    fy = Module->m_RealBoundaryBox.GetBottom();

    error = TstRectangle( Pcb, ox, oy, fx, fy, side );
    if( error < 0 )
        return error;

    if( TstOtherSide )
    {
        error = TstRectangle( Pcb, ox, oy, fx, fy, otherside );
        if( error < 0 )
            return error;
    }

    marge = ( g_GridRoutingSize * Module->m_PadNum ) / GAIN;

    Penalite = CalculePenaliteRectangle( Pcb, ox - marge, oy - marge,
                                         fx + marge, fy + marge, side );
    return Penalite;
}


/*
 * Display the module's ratsnet during displacement, and
 * assess the "cost" of the position.
 * The cost is the longest ratsnest distance with penalty for connections
 * approaching 45 degrees.
 */
float WinEDA_PcbFrame::Compute_Ratsnest_PlaceModule( wxDC* DC )
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
        RATSNEST_ITEM* pt_local_chevelu = &GetBoard()->m_LocalRatsnest[ii];
        if( !( pt_local_chevelu->m_Status & LOCAL_RATSNEST_ITEM ) )
        {
            ox = pt_local_chevelu->m_PadStart->GetPosition().x -
                 g_Offset_Module.x;
            oy = pt_local_chevelu->m_PadStart->GetPosition().y -
                 g_Offset_Module.y;
            fx = pt_local_chevelu->m_PadEnd->GetPosition().x;
            fy = pt_local_chevelu->m_PadEnd->GetPosition().y;

            if( AutoPlaceShowAll )
            {
                GRLine( &DrawPanel->m_ClipBox, DC, ox, oy, fx, fy,
                        0, color );
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


/***********************************/
/* Draw keep out area of a module. */
/***********************************/

/* Buid the cost map.
 * Cells ( in Dist mao ) inside the rect x0,y0 a x1,y1 are
 *  incremented by value Penalite
 *  Cell outside this rectangle, but inside the rectangle
 *  x0,y0 -marge to x1,y1 + marge sont incrementede by a decreasing value
 *  (Penalite ... 0). The decreasing value de pends on the distance to the first rectangle
 *  Therefore the cost is hight in  rect x0,y0 a x1,y1, and decrease outside this rectangle
 */
static void TracePenaliteRectangle( BOARD* Pcb,
                                    int    ux0,
                                    int    uy0,
                                    int    ux1,
                                    int    uy1,
                                    int    marge,
                                    int    Penalite,
                                    int    masque_layer )
{
    int      row, col;
    int      row_min, row_max, col_min, col_max, pmarge;
    int      trace = 0;
    DistCell data, LocalPenalite;
    int      lgain, cgain;

    if( masque_layer & g_TabOneLayerMask[Route_Layer_BOTTOM] )
        trace = 1;     /* Trace on bottom layer. */

    if( ( masque_layer & g_TabOneLayerMask[Route_Layer_TOP] ) && Nb_Sides )
        trace |= 2;    /* Trace on top layer. */

    if( trace == 0 )
        return;

    ux0 -= Pcb->m_BoundaryBox.m_Pos.x;
    uy0 -= Pcb->m_BoundaryBox.m_Pos.y;
    ux1 -= Pcb->m_BoundaryBox.m_Pos.x;
    uy1 -= Pcb->m_BoundaryBox.m_Pos.y;

    ux0 -= marge; ux1 += marge;
    uy0 -= marge; uy1 += marge;

    pmarge = marge / g_GridRoutingSize; if( pmarge < 1 )
        pmarge = 1;

    /* Calculate the coordinate limits of the rectangle. */
    row_max = uy1 / g_GridRoutingSize;
    col_max = ux1 / g_GridRoutingSize;
    row_min = uy0 / g_GridRoutingSize; if( uy0 > row_min * g_GridRoutingSize )
        row_min++;
    col_min = ux0 / g_GridRoutingSize; if( ux0 > col_min * g_GridRoutingSize )
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
            LocalPenalite = Penalite;
            if( col < pmarge )
                cgain = ( 256 * col ) / pmarge;
            else if( col > col_max - pmarge )
                cgain = ( 256 * ( col_max - col ) ) / pmarge;

            cgain = ( cgain * lgain ) / 256;
            if( cgain != 256 )
                LocalPenalite = ( LocalPenalite * cgain ) / 256;
            if( trace & 1 )
            {
                data = GetDist( row, col, BOTTOM ) + LocalPenalite;
                SetDist( row, col, BOTTOM, data );
            }
            if( trace & 2 )
            {
                data = GetDist( row, col, TOP );
                data = MAX( data, LocalPenalite );
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


/* Find the "best" module place
 * The criteria of choice are:
 * - Maximum ratsnet with modules already placed
 * - Max size, and number of pads max
 */
static MODULE* PickModule( WinEDA_PcbFrame* pcbframe, wxDC* DC )
{
    MODULE*  Module;
    std::vector <MODULE*> moduleList;

    // Build sorted footprints list (sort by decreasing size )
    Module = pcbframe->GetBoard()->m_Modules;
    for( ; Module != NULL; Module = Module->Next() )
    {
        Module->Set_Rectangle_Encadrement();
        Module->SetRectangleExinscrit();
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
        pcbframe->build_ratsnest_module( DC, Module );

        /* Calculate external ratsnet. */
        for( unsigned ii = 0;
             ii < pcbframe->GetBoard()->m_LocalRatsnest.size();
             ii++ )
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


/*
 * Determine the rectangle of the pcb, according to the contours
 * layer (EDGE) only
 * Output:
 *   GetBoard()->m_BoundaryBox updated
 * Returns FALSE if no contour
 */
bool WinEDA_PcbFrame::SetBoardBoundaryBoxFromEdgesOnly()
{
    int          rayon, cx, cy, d;
    int          xmax, ymax;
    BOARD_ITEM*  PtStruct;
    DRAWSEGMENT* ptr;
    bool         succes = FALSE;

    if( GetBoard() == NULL )
        return FALSE;

    GetBoard()->m_BoundaryBox.m_Pos.x = GetBoard()->m_BoundaryBox.m_Pos.y =
        0x7FFFFFFFl;
    xmax = ymax = -0x7FFFFFFFl;

    PtStruct = GetBoard()->m_Drawings;
    for( ; PtStruct != NULL; PtStruct = PtStruct->Next() )
    {
        if( PtStruct->Type() != TYPE_DRAWSEGMENT )
            continue;
        succes = TRUE;
        ptr    = (DRAWSEGMENT*) PtStruct;
        d = (ptr->m_Width / 2) + 1;
        if( ptr->m_Shape == S_CIRCLE )
        {
            cx    = ptr->m_Start.x; cy = ptr->m_Start.y;
            rayon =
                (int) hypot( (double) ( ptr->m_End.x - cx ),
                            (double) ( ptr->m_End.y - cy ) );
            rayon += d;
            GetBoard()->m_BoundaryBox.m_Pos.x = MIN(
                GetBoard()->m_BoundaryBox.m_Pos.x, cx - rayon );
            GetBoard()->m_BoundaryBox.m_Pos.y = MIN(
                GetBoard()->m_BoundaryBox.m_Pos.y, cy - rayon );
            xmax = MAX( xmax, cx + rayon );
            ymax = MAX( ymax, cy + rayon );
        }
        else
        {
            cx = MIN( ptr->m_Start.x, ptr->m_End.x );
            cy = MIN( ptr->m_Start.y, ptr->m_End.y );
            GetBoard()->m_BoundaryBox.m_Pos.x = MIN(
                GetBoard()->m_BoundaryBox.m_Pos.x, cx - d );
            GetBoard()->m_BoundaryBox.m_Pos.y = MIN(
                GetBoard()->m_BoundaryBox.m_Pos.y, cy - d );
            cx   = MAX( ptr->m_Start.x, ptr->m_End.x );
            cy   = MAX( ptr->m_Start.y, ptr->m_End.y );
            xmax = MAX( xmax, cx + d );
            ymax = MAX( ymax, cy + d );
        }
    }

    GetBoard()->m_BoundaryBox.SetWidth(
        xmax - GetBoard()->m_BoundaryBox.m_Pos.x );
    GetBoard()->m_BoundaryBox.SetHeight(
        ymax - GetBoard()->m_BoundaryBox.m_Pos.y );
    return succes;
}


/********************************************/
int Propagation( WinEDA_PcbFrame* frame )
/********************************************/

/**
 * Function Propagation
 * Used now only in autoplace calculations
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

