/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
 * Copyright (C) 2008-2011 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 2004-2011 KiCad Developers, see change_log.txt for contributors.
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
 * @file board.cpp
 * @brief Functions for autorouting
 */

#include "fctsys.h"
#include "common.h"
#include "pcbcommon.h"

#include "pcbnew.h"
#include "cell.h"
#include "ar_protos.h"

#include "class_board.h"
#include "class_module.h"
#include "class_track.h"
#include "class_drawsegment.h"
#include "class_edge_mod.h"
#include "class_pcb_text.h"


bool MATRIX_ROUTING_HEAD::ComputeMatrixSize( BOARD* aPcb )
{
    aPcb->ComputeBoundingBox();

    /* The boundary box must have its start point on routing grid: */
    aPcb->m_BoundaryBox.m_Pos.x -= aPcb->m_BoundaryBox.m_Pos.x % m_GridRouting;
    aPcb->m_BoundaryBox.m_Pos.y -= aPcb->m_BoundaryBox.m_Pos.y % m_GridRouting;
    m_BrdBox = aPcb->m_BoundaryBox;

    /* The boundary box must have its end point on routing grid: */
    wxPoint end = m_BrdBox.GetEnd();
    end.x -= end.x % m_GridRouting;
    end.x += m_GridRouting;
    end.y -= end.y % m_GridRouting;
    end.y += m_GridRouting;
    aPcb->m_BoundaryBox.SetEnd( end );
    m_BrdBox.SetEnd(end);

    m_Nrows = Nrows = m_BrdBox.m_Size.y / m_GridRouting;
    m_Ncols = Ncols = m_BrdBox.m_Size.x / m_GridRouting;

    /* get a small margin for memory allocation: */
    Ncols += 1; Nrows += 1;

    return true;
}


MATRIX_ROUTING_HEAD::MATRIX_ROUTING_HEAD()
{
    m_BoardSide[0]  = m_BoardSide[1] = NULL;
    m_DistSide[0]   = m_DistSide[1] = NULL;
    m_DirSide[0]    = m_DirSide[1] = NULL;
    m_InitBoardDone = false;
    m_Layers  = MAX_SIDES_COUNT;
    m_Nrows   = m_Ncols = 0;
    m_MemSize = 0;
}


MATRIX_ROUTING_HEAD::~MATRIX_ROUTING_HEAD()
{
}


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
        m_BoardSide[kk] = (MATRIX_CELL*) operator new( ii * sizeof(MATRIX_CELL) );

        if( m_BoardSide[kk] == NULL )
            return -1;

        /***** allocate Distances *****/
        m_DistSide[kk] = (DIST_CELL*) operator new( ii * sizeof(DIST_CELL) );

        if( m_DistSide[kk] == NULL )
            return -1;

        /***** allocate Dir (chars) *****/
        m_DirSide[kk] = (char*) operator new( ii );

        if( m_DirSide[kk] == NULL )
            return -1;
    }

    m_MemSize = m_Layers * ii * ( sizeof(MATRIX_CELL) + sizeof(DIST_CELL) + sizeof(char) );

    return m_MemSize;
}


void MATRIX_ROUTING_HEAD::UnInitBoard()
{
    int ii;

    m_InitBoardDone = false;

    for( ii = 0; ii < MAX_SIDES_COUNT; ii++ )
    {
        /***** de-allocate Dir matrix *****/
        if( m_DirSide[ii] )
        {
            delete m_DirSide[ii];
            m_DirSide[ii] = NULL;
        }

        /***** de-allocate Distances matrix *****/
        if( m_DistSide[ii] )
        {
            delete m_DistSide[ii];
            m_DistSide[ii] = NULL;
        }

        /**** de-allocate cells matrix *****/
        if( m_BoardSide[ii] )
        {
            delete m_BoardSide[ii];
            m_BoardSide[ii] = NULL;
        }
    }

    m_Nrows = m_Ncols = 0;
}


/**
 * Function PlaceCells
 * initializes the cell board is set and VIA_IMPOSSIBLE HOLE according to the setbacks.
 * The elements of net_code = net_code will not be occupied as places but only
 * VIA_IMPOSSIBLE
 * For single-sided Routing 1:
 * BOTTOM side is used and Route_Layer_BOTTOM = Route_Layer_TOP
 *
 * According to the bits = 1 parameter flag:
 * If FORCE_PADS: all pads will be placed even those same net_code.
 */
void PlaceCells( BOARD* aPcb, int net_code, int flag )
{
    int       ux0 = 0, uy0 = 0, ux1, uy1, dx, dy;
    int       marge, via_marge;
    int       layerMask;

    // use the default NETCLASS?
    NETCLASS* nc = aPcb->m_NetClasses.GetDefault();

    int       trackWidth = nc->GetTrackWidth();
    int       clearance  = nc->GetClearance();
    int       viaSize    = nc->GetViaDiameter();

    marge     = clearance + (trackWidth / 2);
    via_marge = clearance + (viaSize / 2);

    // Place PADS on matrix routing:
    for( unsigned i = 0; i < aPcb->GetPadsCount(); ++i )
    {
        D_PAD* pad = aPcb->m_NetInfo->GetPad( i );

        if( net_code != pad->GetNet() || (flag & FORCE_PADS) )
        {
            ::PlacePad( aPcb, pad, HOLE, marge, WRITE_CELL );
        }

        ::PlacePad( aPcb, pad, VIA_IMPOSSIBLE, via_marge, WRITE_OR_CELL );
    }

    // Place outlines of modules on matrix routing, if they are on a copper layer
    // or on the edge layer
    TRACK tmpSegm( NULL );  // A dummy track used to create segments.

    for( MODULE* module = aPcb->m_Modules; module; module = module->Next() )
    {
        for( BOARD_ITEM* item = module->m_Drawings; item; item = item->Next() )
        {
            switch( item->Type() )
            {
            case PCB_MODULE_EDGE_T:
            {
                EDGE_MODULE* edge = (EDGE_MODULE*) item;

                tmpSegm.SetLayer( edge->GetLayer() );

                if( tmpSegm.GetLayer() == EDGE_N )
                    tmpSegm.SetLayer( -1 );

                tmpSegm.m_Start = edge->m_Start;
                tmpSegm.m_End   = edge->m_End;
                tmpSegm.m_Shape = edge->m_Shape;
                tmpSegm.m_Width = edge->m_Width;
                tmpSegm.m_Param = edge->m_Angle;
                tmpSegm.SetNet( -1 );

                TraceSegmentPcb( aPcb, &tmpSegm, HOLE, marge, WRITE_CELL );
                TraceSegmentPcb( aPcb, &tmpSegm, VIA_IMPOSSIBLE, via_marge, WRITE_OR_CELL );
            }
            break;

            default:
                break;
            }
        }
    }

    // Place board outlines and texts on copper layers:
    for( BOARD_ITEM* item = aPcb->m_Drawings; item; item = item->Next() )
    {
        switch( item->Type() )
        {
        case PCB_LINE_T:
        {
            DRAWSEGMENT* DrawSegm;

            int          type_cell = HOLE;
            DrawSegm = (DRAWSEGMENT*) item;
            tmpSegm.SetLayer( DrawSegm->GetLayer() );

            if( DrawSegm->GetLayer() == EDGE_N )
            {
                tmpSegm.SetLayer( -1 );
                type_cell |= CELL_is_EDGE;
            }

            tmpSegm.m_Start = DrawSegm->m_Start;
            tmpSegm.m_End   = DrawSegm->m_End;
            tmpSegm.m_Shape = DrawSegm->m_Shape;
            tmpSegm.m_Width = DrawSegm->m_Width;
            tmpSegm.m_Param = DrawSegm->m_Angle;
            tmpSegm.SetNet( -1 );

            TraceSegmentPcb( aPcb, &tmpSegm, type_cell, marge, WRITE_CELL );
        }
        break;

        case PCB_TEXT_T:
        {
            TEXTE_PCB* PtText;
            PtText = (TEXTE_PCB*) item;

            if( PtText->GetLength() == 0 )
                break;

            EDA_RECT textbox = PtText->GetTextBox( -1 );
            ux0 = textbox.GetX();
            uy0 = textbox.GetY();
            dx  = textbox.GetWidth();
            dy  = textbox.GetHeight();

            /* Put bounding box (rectangle) on matrix */
            dx /= 2;
            dy /= 2;

            ux1 = ux0 + dx;
            uy1 = uy0 + dy;

            ux0 -= dx;
            uy0 -= dy;

            layerMask = g_TabOneLayerMask[PtText->GetLayer()];

            TraceFilledRectangle( aPcb, ux0 - marge, uy0 - marge, ux1 + marge,
                                  uy1 + marge, (int) (PtText->m_Orient),
                                  layerMask, HOLE, WRITE_CELL );

            TraceFilledRectangle( aPcb, ux0 - via_marge, uy0 - via_marge,
                                  ux1 + via_marge, uy1 + via_marge,
                                  (int) (PtText->m_Orient),
                                  layerMask, VIA_IMPOSSIBLE, WRITE_OR_CELL );
        }
        break;

        default:
            break;
        }
    }

    /* Put tracks and vias on matrix */
    for( TRACK* track = aPcb->m_Track; track; track = track->Next() )
    {
        if( net_code == track->GetNet() )
            continue;

        TraceSegmentPcb( aPcb, track, HOLE, marge, WRITE_CELL );
        TraceSegmentPcb( aPcb, track, VIA_IMPOSSIBLE, via_marge, WRITE_OR_CELL );
    }
}


int Build_Work( BOARD* Pcb )
{
    RATSNEST_ITEM* pt_rats;
    D_PAD*         pt_pad;
    int            r1, r2, c1, c2, current_net_code;
    RATSNEST_ITEM* pt_ch;
    int            demi_pas = Board.m_GridRouting / 2;
    wxString       msg;

    InitWork(); /* clear work list */
    Ntotal = 0;

    for( unsigned ii = 0; ii < Pcb->GetRatsnestsCount(); ii++ )
    {
        pt_rats = &Pcb->m_FullRatsnest[ii];

        /* We consider her only ratsnest that are active ( obviously not yet routed)
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
               + demi_pas ) / Board.m_GridRouting;

        if( r1 < 0 || r1 >= Nrows )
        {
            msg.Printf( wxT( "error : row = %d ( padY %d pcbY %d) " ), r1,
                        pt_pad->GetPosition().y, Pcb->m_BoundaryBox.m_Pos.y );
            wxMessageBox( msg );
            return 0;
        }

        c1 = ( pt_pad->GetPosition().x - Pcb->m_BoundaryBox.m_Pos.x
               + demi_pas ) / Board.m_GridRouting;

        if( c1 < 0 || c1 >= Ncols )
        {
            msg.Printf( wxT( "error : col = %d ( padX %d pcbX %d) " ), c1,
                        pt_pad->GetPosition().x, Pcb->m_BoundaryBox.m_Pos.x );
            wxMessageBox( msg );
            return 0;
        }

        pt_pad = pt_rats->m_PadEnd;

        r2 = ( pt_pad->GetPosition().y - Pcb->m_BoundaryBox.m_Pos.y
               + demi_pas ) / Board.m_GridRouting;

        if( r2 < 0 || r2 >= Nrows )
        {
            msg.Printf( wxT( "error : row = %d ( padY %d pcbY %d) " ), r2,
                        pt_pad->GetPosition().y, Pcb->m_BoundaryBox.m_Pos.y );
            wxMessageBox( msg );
            return 0;
        }

        c2 = ( pt_pad->GetPosition().x - Pcb->m_BoundaryBox.m_Pos.x
               + demi_pas ) / Board.m_GridRouting;

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
