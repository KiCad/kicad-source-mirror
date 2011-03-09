/* BOARD.CPP : functions for autorouting */

#include "fctsys.h"
#include "common.h"
#include "pcbnew.h"
#include "autorout.h"
#include "cell.h"


/*
 * Calculates nrows and ncols, dimensions of the matrix representation of BOARD
 * for routing and automatic calculation of area.
 */
bool ComputeMatriceSize( BOARD * aPcb, int aGridRouting )
{
    aPcb->ComputeBoundingBox();

    /* The boundary box must have its start point on routing grid: */
    aPcb->m_BoundaryBox.m_Pos.x -= aPcb->m_BoundaryBox.m_Pos.x % aGridRouting;
    aPcb->m_BoundaryBox.m_Pos.y -= aPcb->m_BoundaryBox.m_Pos.y % aGridRouting;
    /* The boundary box must have its end point on routing grid: */
    wxPoint end = aPcb->m_BoundaryBox.GetEnd();
    end.x -= end.x % aGridRouting;
    end.x += aGridRouting;
    end.y -= end.y % aGridRouting;
    end.y += aGridRouting;
    aPcb->m_BoundaryBox.SetEnd( end );

    Nrows = aPcb->m_BoundaryBox.m_Size.y / aGridRouting;
    Ncols = aPcb->m_BoundaryBox.m_Size.x / aGridRouting;
    /* get a small margin for memory allocation: */
    Ncols += 2; Nrows += 2;

    return true;
}


/* class MATRIX_ROUTING_HEAD
*/
MATRIX_ROUTING_HEAD::MATRIX_ROUTING_HEAD()
{
    m_BoardSide[0]  = m_BoardSide[1] = NULL;
    m_DistSide[0]   = m_DistSide[1] = NULL;
    m_DirSide[0]    = m_DirSide[1] = NULL;
    m_InitBoardDone = FALSE;
    m_Layers  = MAX_SIDES_COUNT;
    m_Nrows   = m_Ncols = 0;
    m_MemSize = 0;
}


MATRIX_ROUTING_HEAD::~MATRIX_ROUTING_HEAD()
{
}


/* initialize the data structures
 *  returns the RAM size used, or -1 if default
 */
int MATRIX_ROUTING_HEAD::InitBoard()
{
    int ii, kk;

    if( Nrows <= 0 || Ncols <= 0 )
        return 0;
    m_Nrows = Nrows;
    m_Ncols = Ncols;

    m_InitBoardDone = true; /* we have been called */

    ii = (Nrows + 1) * (Ncols + 1);

    for( kk = 0; kk < m_Layers; kk++ )
    {
        m_BoardSide[kk] = NULL;
        m_DistSide[kk]  = NULL;
        m_DirSide[kk]   = NULL;

        /* allocate Board & initialize everything to empty */
        m_BoardSide[kk] = (MATRIX_CELL*) MyZMalloc( ii * sizeof(MATRIX_CELL) );
        if( m_BoardSide[kk] == NULL )
            return -1;

        /***** allocate Distances *****/
        m_DistSide[kk] = (DIST_CELL*) MyZMalloc( ii * sizeof(DIST_CELL) );
        if( m_DistSide[kk] == NULL )
            return -1;

        /***** allocate Dir (chars) *****/
        m_DirSide[kk] = (char*) MyZMalloc( ii );
        if( m_DirSide[kk] == NULL )
            return -1;
    }

    m_MemSize = m_Layers * ii * ( sizeof(MATRIX_CELL) + sizeof(DIST_CELL) + sizeof(char) );

    return m_MemSize;
}


void MATRIX_ROUTING_HEAD::UnInitBoard()
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
            wxMessageBox( msg );
            return 0;
        }
        c1 = ( pt_pad->GetPosition().x - Pcb->m_BoundaryBox.m_Pos.x
               + demi_pas ) / g_GridRoutingSize;
        if( c1 < 0 || c1 >= Ncols )
        {
            msg.Printf( wxT( "error : col = %d ( padX %d pcbX %d) " ), c1,
                        pt_pad->GetPosition().x, Pcb->m_BoundaryBox.m_Pos.x );
            wxMessageBox( msg );
            return 0;
        }

        pt_pad = pt_rats->m_PadEnd;

        r2 = ( pt_pad->GetPosition().y - Pcb->m_BoundaryBox.m_Pos.y
               + demi_pas ) / g_GridRoutingSize;
        if( r2 < 0 || r2 >= Nrows )
        {
            msg.Printf( wxT( "error : row = %d ( padY %d pcbY %d) " ), r2,
                        pt_pad->GetPosition().y, Pcb->m_BoundaryBox.m_Pos.y );
            wxMessageBox( msg );
            return 0;
        }
        c2 = ( pt_pad->GetPosition().x - Pcb->m_BoundaryBox.m_Pos.x
               + demi_pas ) / g_GridRoutingSize;
        if( c2 < 0 || c2 >= Ncols )
        {
            msg.Printf( wxT( "error : col = %d ( padX %d pcbX %d) " ), c2,
                        pt_pad->GetPosition().x, Pcb->m_BoundaryBox.m_Pos.x );
            wxMessageBox( msg );
            return 0;
        }

        SetWork( r1, c1, current_net_code, r2, c2, pt_ch, 0 );
        Ntotal++;
    }

    SortWork();
    return Ntotal;
}


/* return the value stored in a cell
*/
MATRIX_CELL GetCell( int aRow, int aCol, int aSide )
{
    MATRIX_CELL* p;

    p = Board.m_BoardSide[aSide];
    return p[aRow * Ncols + aCol];
}


/* basic cell operation : WRITE operation
*/
void SetCell( int aRow, int aCol, int aSide, MATRIX_CELL x )
{
    MATRIX_CELL* p;

    p = Board.m_BoardSide[aSide];
    p[aRow * Ncols + aCol] = x;
}


/* basic cell operation : OR operation
*/
void OrCell( int aRow, int aCol, int aSide, MATRIX_CELL x )
{
    MATRIX_CELL* p;

    p = Board.m_BoardSide[aSide];
    p[aRow * Ncols + aCol] |= x;
}


/* basic cell operation : XOR operation
*/
void XorCell( int aRow, int aCol, int aSide, MATRIX_CELL x )
{
    MATRIX_CELL* p;

    p = Board.m_BoardSide[aSide];
    p[aRow * Ncols + aCol] ^= x;
}


/* basic cell operation : AND operation
*/
void AndCell( int aRow, int aCol, int aSide, MATRIX_CELL x )
{
    MATRIX_CELL* p;

    p = Board.m_BoardSide[aSide];
    p[aRow * Ncols + aCol] &= x;
}


/* basic cell operation : ADD operation
*/
void AddCell( int aRow, int aCol, int aSide, MATRIX_CELL x )
{
    MATRIX_CELL* p;

    p = Board.m_BoardSide[aSide];
    p[aRow * Ncols + aCol] += x;
}


/* fetch distance cell */
DIST_CELL GetDist( int aRow, int aCol, int aSide ) /* fetch distance cell */
{
    DIST_CELL* p;

    p = Board.m_DistSide[aSide];
    return p[aRow * Ncols + aCol];
}


/* store distance cell */
void SetDist( int aRow, int aCol, int aSide, DIST_CELL x )
{
    DIST_CELL* p;

    p = Board.m_DistSide[aSide];
    p[aRow * Ncols + aCol] = x;
}


/* fetch direction cell */
int GetDir( int aRow, int aCol, int aSide )
{
    DIR_CELL* p;

    p = Board.m_DirSide[aSide];
    return (int) (p[aRow * Ncols + aCol]);
}


/* store direction cell */
void SetDir( int aRow, int aCol, int aSide, int x )
{
    DIR_CELL* p;

    p = Board.m_DirSide[aSide];
    p[aRow * Ncols + aCol] = (char) x;
}
