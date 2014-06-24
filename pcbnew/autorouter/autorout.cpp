/**
 * @file autorout.cpp
 * @brief Autorouting command and control.
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
#include <wxPcbStruct.h>
#include <gr_basic.h>
#include <msgpanel.h>

#include <pcbnew.h>
#include <cell.h>
#include <zones.h>

#include <class_board.h>
#include <class_module.h>
#include <class_track.h>
#include <convert_to_biu.h>

#include <autorout.h>


MATRIX_ROUTING_HEAD RoutingMatrix;     // routing matrix (grid) to route 2-sided boards

/* init board, route traces*/
void PCB_EDIT_FRAME::Autoroute( wxDC* DC, int mode )
{
    int      start, stop;
    MODULE*  Module = NULL;
    D_PAD*   Pad    = NULL;
    int      autoroute_net_code = -1;
    wxString msg;

    if( GetBoard()->GetCopperLayerCount() > 1 )
    {
        g_Route_Layer_TOP    = GetScreen()->m_Route_Layer_TOP;
        g_Route_Layer_BOTTOM = GetScreen()->m_Route_Layer_BOTTOM;
    }
    else
    {
        g_Route_Layer_TOP = g_Route_Layer_BOTTOM = B_Cu;
    }

    switch( mode )
    {
    case ROUTE_NET:
        if( GetScreen()->GetCurItem() )
        {
            switch( GetScreen()->GetCurItem()->Type() )
            {
            case PCB_PAD_T:
                Pad = (D_PAD*) GetScreen()->GetCurItem();
                autoroute_net_code = Pad->GetNetCode();
                break;

            default:
                break;
            }
        }
        if( autoroute_net_code <= 0 )
        {
            wxMessageBox( _( "Net not selected" ) ); return;
        }
        break;

    case ROUTE_MODULE:
        Module = (MODULE*) GetScreen()->GetCurItem();
        if( (Module == NULL) || (Module->Type() != PCB_MODULE_T) )
        {
            wxMessageBox( _( "Module not selected" ) );
            return;
        }
        break;

    case ROUTE_PAD:
        Pad = (D_PAD*) GetScreen()->GetCurItem();

        if( (Pad == NULL)  || (Pad->Type() != PCB_PAD_T) )
        {
            wxMessageBox( _( "Pad not selected" ) );
            return;
        }

        break;
    }

    if( (GetBoard()->m_Status_Pcb & LISTE_RATSNEST_ITEM_OK ) == 0 )
        Compile_Ratsnest( DC, true );

    /* Set the flag on the ratsnest to CH_ROUTE_REQ. */
    for( unsigned ii = 0; ii < GetBoard()->GetRatsnestsCount(); ii++ )
    {
        RATSNEST_ITEM* ptmp = &GetBoard()->m_FullRatsnest[ii];
        ptmp->m_Status &= ~CH_ROUTE_REQ;

        switch( mode )
        {
        case ROUTE_ALL:
            ptmp->m_Status |= CH_ROUTE_REQ;
            break;

        case ROUTE_NET:
            if( autoroute_net_code == ptmp->GetNet() )
                ptmp->m_Status |= CH_ROUTE_REQ;
            break;

        case ROUTE_MODULE:
        {
            D_PAD* pt_pad = (D_PAD*) Module->Pads();
            for( ; pt_pad != NULL; pt_pad = pt_pad->Next() )
            {
                if( ptmp->m_PadStart == pt_pad )
                    ptmp->m_Status |= CH_ROUTE_REQ;

                if( ptmp->m_PadEnd == pt_pad )
                    ptmp->m_Status |= CH_ROUTE_REQ;
            }

            break;
        }

        case ROUTE_PAD:
            if( ( ptmp->m_PadStart == Pad ) || ( ptmp->m_PadEnd == Pad ) )
                ptmp->m_Status |= CH_ROUTE_REQ;

            break;
        }
    }

    start = time( NULL );

    /* Calculation of no fixed routing to 5 mils and more. */
    RoutingMatrix.m_GridRouting = (int)GetScreen()->GetGridSize().x;

    if( RoutingMatrix.m_GridRouting < (5*IU_PER_MILS) )
        RoutingMatrix.m_GridRouting = 5*IU_PER_MILS;


    /* Calculated ncol and nrow, matrix size for routing. */
    RoutingMatrix.ComputeMatrixSize( GetBoard() );

    m_messagePanel->EraseMsgBox();

    /* Map the board */
    RoutingMatrix.m_RoutingLayersCount = 1;

    if( g_Route_Layer_TOP != g_Route_Layer_BOTTOM )
        RoutingMatrix.m_RoutingLayersCount = 2;

    if( RoutingMatrix.InitRoutingMatrix() < 0 )
    {
        wxMessageBox( _( "No memory for autorouting" ) );
        RoutingMatrix.UnInitRoutingMatrix();  /* Free memory. */
        return;
    }

    SetStatusText( _( "Place Cells" ) );
    PlaceCells( GetBoard(), -1, FORCE_PADS );

    /* Construction of the track list for router. */
    RoutingMatrix.m_RouteCount = Build_Work( GetBoard() );

    // DisplayRoutingMatrix( m_canvas, DC );

    Solve( DC, RoutingMatrix.m_RoutingLayersCount );

    /* Free memory. */
    FreeQueue();
    InitWork();             /* Free memory for the list of router connections. */
    RoutingMatrix.UnInitRoutingMatrix();
    stop = time( NULL ) - start;
    msg.Printf( wxT( "time = %d second%s" ), stop, ( stop == 1 ) ? wxT( "" ) : wxT( "s" ) );
    SetStatusText( msg );
}


/* Clear the flag CH_NOROUTABLE which is set to 1 by Solve(),
 * when a track was not routed.
 * (If this flag is 1 the corresponding track it is not rerouted)
 */
void PCB_EDIT_FRAME::Reset_Noroutable( wxDC* DC )
{
    if( ( GetBoard()->m_Status_Pcb & LISTE_RATSNEST_ITEM_OK )== 0 )
        Compile_Ratsnest( DC, true );

    for( unsigned ii = 0; ii < GetBoard()->GetRatsnestsCount(); ii++ )
    {
        GetBoard()->m_FullRatsnest[ii].m_Status &= ~CH_UNROUTABLE;
    }
}


/* DEBUG Function: displays the routing matrix */
void DisplayRoutingMatrix( EDA_DRAW_PANEL* panel, wxDC* DC )
{
    int dcell0, dcell1 = 0;
    EDA_COLOR_T color;

    int maxi = 600 / RoutingMatrix.m_Ncols;
    maxi = ( maxi * 3 ) / 4;

    if( !maxi )
        maxi = 1;

    GRSetDrawMode( DC, GR_COPY );

    for( int col = 0; col < RoutingMatrix.m_Ncols; col++ )
    {
        for( int row = 0; row < RoutingMatrix.m_Nrows; row++ )
        {
            color  = BLACK;
            dcell0 = RoutingMatrix.GetCell( row, col, BOTTOM );

            if( dcell0 & HOLE )
                color = GREEN;

//            if( RoutingMatrix.m_RoutingLayersCount )
//                dcell1 = GetCell( row, col, TOP );

            if( dcell1 & HOLE )
                color = RED;

//            dcell0 |= dcell1;

            if( !color && ( dcell0 & VIA_IMPOSSIBLE ) )
                color = BLUE;

            if( dcell0 & CELL_is_EDGE )
                color = YELLOW;
            else if( dcell0 & CELL_is_ZONE )
                color = YELLOW;

            #define DRAW_OFFSET_X -20
            #define DRAW_OFFSET_Y 20
//            if( color )
            {
                for( int i = 0; i < maxi; i++ )
                    for( int j = 0; j < maxi; j++ )
                        GRPutPixel( panel->GetClipBox(), DC,
                                    ( col * maxi ) + i + DRAW_OFFSET_X,
                                    ( row * maxi ) + j + DRAW_OFFSET_Y, color );

            }
        }
    }
}
