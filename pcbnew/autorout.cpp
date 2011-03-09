/***************************************/
/* PCBNEW: Autorouting command control */
/***************************************/

#include "fctsys.h"
#include "common.h"
#include "class_drawpanel.h"
#include "pcbnew.h"
#include "wxPcbStruct.h"
#include "autorout.h"
#include "cell.h"
#include "zones.h"
#include "class_board_design_settings.h"


int E_scale;         /* Scaling factor of distance tables. */
int Nb_Sides;        /* Number of layer for autorouting (0 or 1) */
int Nrows = ILLEGAL;
int Ncols = ILLEGAL;
int Ntotal;
int OpenNodes;       /* total number of nodes opened */
int ClosNodes;       /* total number of nodes closed */
int MoveNodes;       /* total number of nodes moved */
int MaxNodes;        /* maximum number of nodes opened at one time */

MATRIX_ROUTING_HEAD Board;     /* 2-sided board */


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
        Route_Layer_TOP    = GetScreen()->m_Route_Layer_TOP;
        Route_Layer_BOTTOM = GetScreen()->m_Route_Layer_BOTTOM;
    }
    else
    {
        Route_Layer_TOP =
            Route_Layer_BOTTOM = LAYER_N_BACK;
    }

    switch( mode )
    {
    case ROUTE_NET:
        if( GetScreen()->GetCurItem() )
        {
            switch( GetScreen()->GetCurItem()->Type() )
            {
            case TYPE_PAD:
                Pad = (D_PAD*) GetScreen()->GetCurItem();
                autoroute_net_code = Pad->GetNet();
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
        if( (Module == NULL) || (Module->Type() != TYPE_MODULE) )
        {
            wxMessageBox( _( "Module not selected" ) );
            return;
        }
        break;

    case ROUTE_PAD:
        Pad = (D_PAD*) GetScreen()->GetCurItem();
        if( (Pad == NULL)  || (Pad->Type() != TYPE_PAD) )
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
            D_PAD* pt_pad = (D_PAD*) Module->m_Pads;
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
    Board.m_GridRouting = (int)GetScreen()->GetGridSize().x;
    if( Board.m_GridRouting < 50 )
        Board.m_GridRouting = 50;
    E_scale = Board.m_GridRouting / 50; if( E_scale < 1 )
        E_scale = 1;

    /* Calculated ncol and nrow, matrix size for routing. */
    Board.ComputeMatrixSize( GetBoard() );

    MsgPanel->EraseMsgBox();

    /* Map the board */
    Nb_Sides = ONE_SIDE;
    if( Route_Layer_TOP != Route_Layer_BOTTOM )
        Nb_Sides = TWO_SIDES;

    if( Board.InitBoard() < 0 )
    {
        wxMessageBox( _( "No memory for autorouting" ) );
        Board.UnInitBoard();  /* Free memory. */
        return;
    }

    SetStatusText( _( "Place Cells" ) );
    PlaceCells( GetBoard(), -1, FORCE_PADS );

    /* Construction of the track list for router. */
    Build_Work( GetBoard() );

    // DisplayBoard(DrawPanel, DC);

    if( Nb_Sides == TWO_SIDES )
        Solve( DC, TWO_SIDES ); /* double face */
    else
        Solve( DC, ONE_SIDE );  /* simple face */

    /* Free memory. */
    FreeQueue();
    InitWork();             /* Free memory for the list of router connections. */
    Board.UnInitBoard();
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
void DisplayBoard( EDA_DRAW_PANEL* panel, wxDC* DC )
{
    int row, col, i, j;
    int dcell0, dcell1 = 0, color;
    int maxi;

    maxi = 600 / Ncols;
    maxi = ( maxi * 3 ) / 4;
    if( !maxi )
        maxi = 1;

    GRSetDrawMode( DC, GR_COPY );
    for( col = 0; col < Ncols; col++ )
    {
        for( row = 0; row < Nrows; row++ )
        {
            color  = 0;
            dcell0 = GetCell( row, col, BOTTOM );
            if( dcell0 & HOLE )
                color = GREEN;
//            if( Nb_Sides )
//                dcell1 = GetCell( row, col, TOP );
            if( dcell1 & HOLE )
                color |= RED;
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
                for( i = 0; i < maxi; i++ )
                    for( j = 0; j < maxi; j++ )
                        GRPutPixel( &panel->m_ClipBox, DC,
                                    ( col * maxi ) + i + DRAW_OFFSET_X,
                                    ( row * maxi ) + j + DRAW_OFFSET_Y, color );

            }
        }
    }
}
