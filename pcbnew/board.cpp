/* BOARD.CPP : functions for autorouting */

#include "fctsys.h"
#include "gr_basic.h"
#include "common.h"
#include "confirm.h"
#include "pcbnew.h"
#include "autorout.h"
#include "zones.h"
#include "cell.h"

#include "protos.h"


int         Build_Work( BOARD* Pcb );
void        PlaceCells( BOARD* Pcb, int net_code, int flag );
int         InitBoard();
BoardCell   GetCell( int, int, int );
void        SetCell( int row, int col, int side, BoardCell x );
void        OrCell( int, int, int, BoardCell );
void        AndCell( int, int, int, BoardCell );
void        AddCell( int, int, int, BoardCell );
void        XorCell( int, int, int, BoardCell );
void        AddCell( int, int, int, BoardCell );
DistCell    GetDist( int, int, int );
void        SetDist( int, int, int, DistCell );
int         GetDir( int, int, int );
void        SetDir( int, int, int, int );


/*
 * Calculates nrows and ncols, dimensions of the matrix representation of BOARD
 * for routing and automatic calculation of area.
 */
bool ComputeMatriceSize( WinEDA_BasePcbFrame* frame, int g_GridRoutingSize )
{
    BOARD* pcb = frame->GetBoard();

    pcb->ComputeBoundaryBox();

    /* The boundary box must have its start point on routing grid: */
    pcb->m_BoundaryBox.m_Pos.x -= pcb->m_BoundaryBox.m_Pos.x % g_GridRoutingSize;
    pcb->m_BoundaryBox.m_Pos.y -= pcb->m_BoundaryBox.m_Pos.y % g_GridRoutingSize;
    /* The boundary box must have its end point on routing grid: */
    wxPoint end = pcb->m_BoundaryBox.GetEnd();
    end.x -= end.x % g_GridRoutingSize; end.x += g_GridRoutingSize;
    end.y -= end.y % g_GridRoutingSize; end.y += g_GridRoutingSize;
    pcb->m_BoundaryBox.SetEnd( end );

    Nrows = pcb->m_BoundaryBox.m_Size.y / g_GridRoutingSize;
    Ncols = pcb->m_BoundaryBox.m_Size.x / g_GridRoutingSize;
    /* get a small margin for memory allocation: */
    Ncols += 2; Nrows += 2;

    return TRUE;
}


/*******************/
/* class BOARDHEAD */
/*******************/

BOARDHEAD::BOARDHEAD()
{
    m_BoardSide[0]  = m_BoardSide[1] = NULL;
    m_DistSide[0]   = m_DistSide[1] = NULL;
    m_DirSide[0]    = m_DirSide[1] = NULL;
    m_InitBoardDone = FALSE;
    m_Layers  = 2;
    m_Nrows   = m_Ncols = 0;
    m_MemSize = 0;
}


BOARDHEAD::~BOARDHEAD()
{
}


/* initialize the data structures
 *  returns the RAM size used, or -1 if default
 */
int BOARDHEAD::InitBoard()
{
    int ii, kk;

    if( Nrows <= 0 || Ncols <= 0 )
        return 0;
    m_Nrows = Nrows;
    m_Ncols = Ncols;

    m_InitBoardDone = TRUE; /* we have been called */

    ii = (Nrows + 1) * (Ncols + 1);

    for( kk = 0; kk < m_Layers; kk++ )
    {
        m_BoardSide[kk] = NULL;
        m_DistSide[kk]  = NULL;
        m_DirSide[kk]   = NULL;

        /* allocate Board & initialize everything to empty */
        m_BoardSide[kk] = (BoardCell*) MyZMalloc( ii * sizeof(BoardCell) );
        if( m_BoardSide[kk] == NULL )
            return -1;

        /***** allocate Distances *****/
        m_DistSide[kk] = (DistCell*) MyZMalloc( ii * sizeof(DistCell) );
        if( m_DistSide[kk] == NULL )
            return -1;

        /***** allocate Dir (chars) *****/
        m_DirSide[kk] = (char*) MyZMalloc( ii );
        if( m_DirSide[kk] == NULL )
            return -1;
    }

    m_MemSize = m_Layers * ii * ( sizeof(BoardCell) + sizeof(DistCell) + sizeof(char) );

    return m_MemSize;
}


void BOARDHEAD::UnInitBoard()
{
    int ii;

    m_InitBoardDone = FALSE;

    for( ii = 0; ii < 2; ii++ )
    {
        /***** de-allocate Dir (chars) *****/
        if( m_DirSide[ii] )
        {
            MyFree( m_DirSide[ii] ); m_DirSide[ii] = NULL;
        }

        /***** de-allocate Distances *****/
        if( m_DistSide[ii] )
        {
            MyFree( m_DistSide[ii] ); m_DistSide[ii] = NULL;
        }

        /**** de-allocate Board *****/
        if( m_BoardSide[ii] )
        {
            MyFree( m_BoardSide[ii] ); m_BoardSide[ii] = NULL;
        }
    }

    m_Nrows = m_Ncols = 0;
}


/* Initialize the cell board is set and VIA_IMPOSSIBLE HOLE according to
 * the setbacks
 * The elements of net_code = net_code will not be occupied as places
 * but only VIA_IMPOSSIBLE
 * For single-sided Routing 1:
 * BOTTOM side is used and Route_Layer_BOTTOM = Route_Layer_TOP
 *
 * According to the bits = 1 parameter flag:
 * If FORCE_PADS: all pads will be placed even those same net_code.
 */
void PlaceCells( BOARD* aPcb, int net_code, int flag )
{
    int         ux0 = 0, uy0 = 0, ux1, uy1, dx, dy;
    int         marge, via_marge;
    int         masque_layer;

    // use the default NETCLASS?
    NETCLASS*   nc = aPcb->m_NetClasses.GetDefault();

    int         trackWidth = nc->GetTrackWidth();
    int         clearance  = nc->GetClearance();
    int         viaSize    = nc->GetViaDiameter();

    marge     = clearance + (trackWidth / 2);
    via_marge = clearance + (viaSize / 2);

    //////////////////////////
    // Place PADS on board. //
    //////////////////////////

    for( unsigned i=0; i < aPcb->GetPadsCount(); ++i )
    {
        D_PAD* pad = aPcb->m_NetInfo->GetPad(i);

        if( net_code != pad->GetNet() || (flag & FORCE_PADS) )
        {
            Place_1_Pad_Board( aPcb, pad, HOLE, marge, WRITE_CELL );
        }
        Place_1_Pad_Board( aPcb, pad, VIA_IMPOSSIBLE, via_marge, WRITE_OR_CELL );
    }

    ////////////////////////////////////////////
    // Placing the elements of modules on PCB //
    ////////////////////////////////////////////
    for( MODULE* module = aPcb->m_Modules;  module;  module = module->Next() )
    {
        for( BOARD_ITEM* item = module->m_Drawings;  item;  item = item->Next() )
        {
            switch( item->Type() )
            {
            case TYPE_EDGE_MODULE:
                {
                    EDGE_MODULE* edge = (EDGE_MODULE*) item;

                    TRACK* TmpSegm = new TRACK( NULL );

                    TmpSegm->SetLayer( edge->GetLayer() );
                    if( TmpSegm->GetLayer() == EDGE_N )
                        TmpSegm->SetLayer( -1 );

                    TmpSegm->m_Start   = edge->m_Start;
                    TmpSegm->m_End     = edge->m_End;
                    TmpSegm->m_Shape   = edge->m_Shape;
                    TmpSegm->m_Width   = edge->m_Width;
                    TmpSegm->m_Param   = edge->m_Angle;
                    TmpSegm->SetNet( -1 );

                    TraceSegmentPcb( aPcb, TmpSegm, HOLE, marge, WRITE_CELL );
                    TraceSegmentPcb( aPcb, TmpSegm, VIA_IMPOSSIBLE, via_marge,
                                     WRITE_OR_CELL );
                    delete TmpSegm;
                }
                break;

            default:
                break;
            }
        }
    }

    ////////////////////////////////////////////
    // Placement contours and segments on PCB //
    ////////////////////////////////////////////
    for( BOARD_ITEM* item = aPcb->m_Drawings; item; item = item->Next() )
    {
        switch( item->Type() )
        {
        case TYPE_DRAWSEGMENT:
            {
                DRAWSEGMENT*    DrawSegm;

                int    type_cell = HOLE;
                TRACK* TmpSegm   = new TRACK( NULL );
                DrawSegm = (DRAWSEGMENT*) item;
                TmpSegm->SetLayer( DrawSegm->GetLayer() );
                if( DrawSegm->GetLayer() == EDGE_N )
                {
                    TmpSegm->SetLayer( -1 );
                    type_cell |= CELL_is_EDGE;
                }

                TmpSegm->m_Start   = DrawSegm->m_Start;
                TmpSegm->m_End     = DrawSegm->m_End;
                TmpSegm->m_Shape   = DrawSegm->m_Shape;
                TmpSegm->m_Width   = DrawSegm->m_Width;
                TmpSegm->m_Param   = DrawSegm->m_Angle;
                TmpSegm->SetNet( -1 );

                TraceSegmentPcb( aPcb, TmpSegm, type_cell, marge, WRITE_CELL );

 // TraceSegmentPcb(Pcb, TmpSegm, VIA_IMPOSSIBLE, via_marge,WRITE_OR_CELL );
                delete TmpSegm;
            }
            break;

        case TYPE_TEXTE:
        {
            TEXTE_PCB*      PtText;
            PtText = (TEXTE_PCB*) item;

            if( PtText->GetLength() == 0 )
                break;

        EDA_Rect textbox = PtText->GetTextBox(-1);
            ux0 = textbox.GetX(); uy0 = textbox.GetY();
            dx = textbox.GetWidth();
            dy = textbox.GetHeight();

            /* Put bounding box (rectangle) on matrix */
            dx /= 2;
            dy /= 2;

            ux1 = ux0 + dx;
            uy1 = uy0 + dy;

            ux0 -= dx;
            uy0 -= dy;

            masque_layer = g_TabOneLayerMask[PtText->GetLayer()];

            TraceFilledRectangle( aPcb, ux0 - marge, uy0 - marge, ux1 + marge,
                                  uy1 + marge, (int) (PtText->m_Orient),
                                  masque_layer, HOLE, WRITE_CELL );

            TraceFilledRectangle( aPcb, ux0 - via_marge, uy0 - via_marge,
                                  ux1 + via_marge, uy1 + via_marge,
                                  (int) (PtText->m_Orient),
                                  masque_layer, VIA_IMPOSSIBLE, WRITE_OR_CELL );
        }
            break;

        default:
            break;
        }
    }

    /* Put tracks and vias on matrix */
    for( TRACK* track = aPcb->m_Track;  track;  track = track->Next() )
    {
        if( net_code == track->GetNet() )
            continue;

        TraceSegmentPcb( aPcb, track, HOLE, marge, WRITE_CELL );
        TraceSegmentPcb( aPcb, track, VIA_IMPOSSIBLE, via_marge, WRITE_OR_CELL );
    }

    // Put zone filling on matrix
    for( SEGZONE* zone = aPcb->m_Zone;  zone;  zone = zone->Next() )
    {
        if( net_code == zone->GetNet() )
            continue;

        TraceSegmentPcb( aPcb, zone, HOLE, marge, WRITE_CELL );
        TraceSegmentPcb( aPcb, zone, VIA_IMPOSSIBLE, via_marge, WRITE_OR_CELL );
    }
}


int Build_Work( BOARD* Pcb )
{
    RATSNEST_ITEM* pt_rats;
    D_PAD*   pt_pad;
    int      r1, r2, c1, c2, current_net_code;
    RATSNEST_ITEM* pt_ch;
    int      demi_pas = g_GridRoutingSize / 2;
    wxString msg;

    InitWork(); /* clear work list */
    Ntotal = 0;
    for( unsigned ii = 0; ii < Pcb->GetRatsnestsCount(); ii++ )
    {
        pt_rats = &Pcb->m_FullRatsnest[ii];
        /* We consider her only ratsnets that are active ( obviously not yet routed)
         * and routables (that are not yet attempt to be routed and fail
         */
         if( (pt_rats->m_Status & CH_ACTIF) == 0 )
            continue;
        if( pt_rats->m_Status & CH_UNROUTABLE )
            continue;
        if( (pt_rats->m_Status & CH_ROUTE_REQ) == 0 )
            continue;
        pt_pad = pt_rats->m_PadStart;

        current_net_code = pt_pad->GetNet();
        pt_ch = pt_rats;

        r1 = ( pt_pad->GetPosition().y - Pcb->m_BoundaryBox.m_Pos.y
               + demi_pas ) / g_GridRoutingSize;
        if( r1 < 0 || r1 >= Nrows )
        {
            msg.Printf( wxT( "error : row = %d ( padY %d pcbY %d) " ), r1,
                        pt_pad->GetPosition().y, Pcb->m_BoundaryBox.m_Pos.y );
            DisplayError( NULL, msg );
            return 0;
        }
        c1 = ( pt_pad->GetPosition().x - Pcb->m_BoundaryBox.m_Pos.x
               + demi_pas ) / g_GridRoutingSize;
        if( c1 < 0 || c1 >= Ncols )
        {
            msg.Printf( wxT( "error : col = %d ( padX %d pcbX %d) " ), c1,
                        pt_pad->GetPosition().x, Pcb->m_BoundaryBox.m_Pos.x );
            DisplayError( NULL, msg );
            return 0;
        }

        pt_pad = pt_rats->m_PadEnd;

        r2 = ( pt_pad->GetPosition().y - Pcb->m_BoundaryBox.m_Pos.y
               + demi_pas ) / g_GridRoutingSize;
        if( r2 < 0 || r2 >= Nrows )
        {
            msg.Printf( wxT( "error : row = %d ( padY %d pcbY %d) " ), r2,
                        pt_pad->GetPosition().y, Pcb->m_BoundaryBox.m_Pos.y );
            DisplayError( NULL, msg );
            return 0;
        }
        c2 = ( pt_pad->GetPosition().x - Pcb->m_BoundaryBox.m_Pos.x
               + demi_pas ) / g_GridRoutingSize;
        if( c2 < 0 || c2 >= Ncols )
        {
            msg.Printf( wxT( "error : col = %d ( padX %d pcbX %d) " ), c2,
                        pt_pad->GetPosition().x, Pcb->m_BoundaryBox.m_Pos.x );
            DisplayError( NULL, msg );
            return 0;
        }

        SetWork( r1, c1, current_net_code, r2, c2, pt_ch, 0 );
        Ntotal++;
    }

    SortWork();
    return Ntotal;
}


BoardCell GetCell( int row, int col, int side )
{
    BoardCell* p;

    p = Board.m_BoardSide[side];
    return p[row * Ncols + col];
}


/************************************************/
/* void SetCell(int r,int c,int s,BoardCell x ) */
/************************************************/

/* store board cell */
void SetCell( int row, int col, int side, BoardCell x )
{
    BoardCell* p;

    p = Board.m_BoardSide[side];
    p[row * Ncols + col] = x;
}


/******************************************/
/* void OrCell(int r,int c,int s,BoardCell x ) */
/******************************************/

void OrCell( int r, int c, int s, BoardCell x )
{
    BoardCell* p;

    p = Board.m_BoardSide[s];
    p[r * Ncols + c] |= x;
}


/******************************************/
/* void XorCell(int r,int c,int s,BoardCell x ) */
/******************************************/

void XorCell( int r, int c, int s, BoardCell x )
{
    BoardCell* p;

    p = Board.m_BoardSide[s];
    p[r * Ncols + c] ^= x;
}


/************************************************/
/* void AndCell(int r,int c,int s,BoardCell x ) */
/************************************************/

void AndCell( int r, int c, int s, BoardCell x )
{
    BoardCell* p;

    p = Board.m_BoardSide[s];
    p[r * Ncols + c] &= x;
}


/************************************************/
/* void AddCell(int r,int c,int s,BoardCell x ) */
/************************************************/

void AddCell( int r, int c, int s, BoardCell x )
{
    BoardCell* p;

    p = Board.m_BoardSide[s];
    p[r * Ncols + c] += x;
}


/****************************************/
/* DistCell GetDist(int r,int c,int s ) */
/****************************************/

/* fetch distance cell */
DistCell GetDist( int r, int c, int s ) /* fetch distance cell */
{
    DistCell* p;

    p = Board.m_DistSide[s];
    return p[r * Ncols + c];
}


/***********************************************/
/* void SetDist(int r,int c,int s,DistCell x ) */
/***********************************************/

/* store distance cell */
void SetDist( int r, int c, int s, DistCell x )
{
    DistCell* p;

    p = Board.m_DistSide[s];
    p[r * Ncols + c] = x;
}


/**********************************/
/* int GetDir(int r,int c,int s ) */
/**********************************/

/* fetch direction cell */
int GetDir( int r, int c, int s )
{
    char* p;

    p = Board.m_DirSide[s];
    return (int) (p[r * Ncols + c]);
}


/*****************************************/
/* void SetDir(int r,int c,int s,int x ) */
/*****************************************/

/* store direction cell */
void SetDir( int r, int c, int s, int x )
{
    char* p;

    p = Board.m_DirSide[s];
    p[r * Ncols + c] = (char) x;
}
