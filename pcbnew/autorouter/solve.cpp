/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 Jean-Pierre Charras, jean-pierre.charras@ujf-grenoble.fr
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 *
 * Copyright (C) 1992-2012 KiCad Developers, see change_log.txt for contributors.
 *
 * First copyright (C) Randy Nevin, 1989 (see PCBCA package)
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

/* see "Autorouting With the A* Algorithm" (Dr.Dobbs journal)
*/

/**
 * @file solve.cpp
 */

#include <fctsys.h>
#include <class_drawpanel.h>
#include <confirm.h>
#include <wxPcbStruct.h>
#include <gr_basic.h>
#include <macros.h>
#include <pcbcommon.h>

#include <class_board.h>
#include <class_track.h>

#include <pcbnew.h>
#include <protos.h>
#include <autorout.h>
#include <cell.h>


static int Autoroute_One_Track( PCB_EDIT_FRAME* pcbframe,
                                wxDC*           DC,
                                int             two_sides,
                                int             row_source,
                                int             col_source,
                                int             row_target,
                                int             col_target,
                                RATSNEST_ITEM*  pt_rat );

static int Retrace( PCB_EDIT_FRAME* pcbframe,
                    wxDC*           DC,
                    int,
                    int,
                    int,
                    int,
                    int,
                    int              net_code );

static void OrCell_Trace( BOARD* pcb,
                          int    col,
                          int    row,
                          int    side,
                          int    orient,
                          int    current_net_code );

static void AddNewTrace( PCB_EDIT_FRAME* pcbframe, wxDC* DC );


static int            segm_oX, segm_oY;
static int            segm_fX, segm_fY; /* Origin and position of the current
                                         * trace segment. */
static RATSNEST_ITEM* pt_cur_ch;
static int            s_Clearance;  // Clearance value used in autorouter

static PICKED_ITEMS_LIST s_ItemsListPicker;

int OpenNodes;       /* total number of nodes opened */
int ClosNodes;       /* total number of nodes closed */
int MoveNodes;       /* total number of nodes moved */
int MaxNodes;        /* maximum number of nodes opened at one time */

#define NOSUCCESS       0
#define STOP_FROM_ESC   -1
#define ERR_MEMORY      -2
#define SUCCESS         1
#define TRIVIAL_SUCCESS 2

/*
** visit neighboring cells like this (where [9] is on the other side):
**
**  +---+---+---+
**  | 1 | 2 | 3 |
**  +---+---+---+
**  | 4 |[9]| 5 |
**  +---+---+---+
**  | 6 | 7 | 8 |
**  +---+---+---+
*/

/* for visiting neighbors on the same side: increments/decrements coord of
 * [] [0] = row [] (1] = col was added to the coord of the midpoint for
 * Get the coord of the 8 neighboring points.
 */
static const int delta[8][2] =
{
    {  1, -1 },     /* northwest    */
    {  1, 0  },     /* north        */
    {  1, 1  },     /* northeast    */
    {  0, -1 },     /* west     */
    {  0, 1  },     /* east     */
    { -1, -1 },     /* southwest    */
    { -1, 0  },     /* south        */
    { -1, 1  }      /* southeast    */
};

static const int ndir[8] =
{
    /* for building paths back to source */
    FROM_SOUTHEAST, FROM_SOUTH,     FROM_SOUTHWEST,
    FROM_EAST,      FROM_WEST,
    FROM_NORTHEAST, FROM_NORTH,     FROM_NORTHWEST
};

/* blocking masks for neighboring cells */
#define BLOCK_NORTHEAST ( DIAG_NEtoSW | BENT_StoNE | BENT_WtoNE \
                          | ANGLE_NEtoSE | ANGLE_NWtoNE         \
                          | SHARP_NtoNE | SHARP_EtoNE | HOLE )
#define BLOCK_SOUTHEAST ( DIAG_SEtoNW | BENT_NtoSE | BENT_WtoSE \
                          | ANGLE_NEtoSE | ANGLE_SEtoSW         \
                          | SHARP_EtoSE | SHARP_StoSE | HOLE )
#define BLOCK_SOUTHWEST ( DIAG_NEtoSW | BENT_NtoSW | BENT_EtoSW \
                          | ANGLE_SEtoSW | ANGLE_SWtoNW         \
                          | SHARP_StoSW | SHARP_WtoSW | HOLE )
#define BLOCK_NORTHWEST ( DIAG_SEtoNW | BENT_EtoNW | BENT_StoNW \
                          | ANGLE_SWtoNW | ANGLE_NWtoNE         \
                          | SHARP_WtoNW | SHARP_NtoNW | HOLE )
#define BLOCK_NORTH     ( LINE_VERTICAL | BENT_NtoSE | BENT_NtoSW      \
                          | BENT_EtoNW | BENT_WtoNE                    \
                          | BENT_StoNE | BENT_StoNW                    \
                          | CORNER_NORTHEAST | CORNER_NORTHWEST        \
                          | ANGLE_NEtoSE | ANGLE_SWtoNW | ANGLE_NWtoNE \
                          | DIAG_NEtoSW | DIAG_SEtoNW                  \
                          | SHARP_NtoNE | SHARP_NtoNW                  \
                          | SHARP_EtoNE | SHARP_WtoNW | HOLE )
#define BLOCK_EAST      ( LINE_HORIZONTAL | BENT_EtoSW | BENT_EtoNW    \
                          | BENT_NtoSE | BENT_StoNE                    \
                          | BENT_WtoNE | BENT_WtoSE                    \
                          | CORNER_NORTHEAST | CORNER_SOUTHEAST        \
                          | ANGLE_NEtoSE | ANGLE_SEtoSW | ANGLE_NWtoNE \
                          | DIAG_NEtoSW | DIAG_SEtoNW                  \
                          | SHARP_EtoNE | SHARP_EtoSE                  \
                          | SHARP_NtoNE | SHARP_StoSE | HOLE )
#define BLOCK_SOUTH     ( LINE_VERTICAL | BENT_StoNE | BENT_StoNW      \
                          | BENT_EtoSW | BENT_WtoSE                    \
                          | BENT_NtoSE | BENT_NtoSW                    \
                          | CORNER_SOUTHEAST | CORNER_SOUTHWEST        \
                          | ANGLE_NEtoSE | ANGLE_SWtoNW | ANGLE_SEtoSW \
                          | DIAG_NEtoSW | DIAG_SEtoNW                  \
                          | SHARP_StoSE | SHARP_StoSW                  \
                          | SHARP_EtoSE | SHARP_WtoSW | HOLE )
#define BLOCK_WEST      ( LINE_HORIZONTAL | BENT_WtoNE | BENT_WtoSE    \
                          | BENT_NtoSW | BENT_StoNW                    \
                          | BENT_EtoSW | BENT_EtoNW                    \
                          | CORNER_SOUTHWEST | CORNER_NORTHWEST        \
                          | ANGLE_SWtoNW | ANGLE_SEtoSW | ANGLE_NWtoNE \
                          | DIAG_NEtoSW | DIAG_SEtoNW                  \
                          | SHARP_WtoSW | SHARP_WtoNW                  \
                          | SHARP_NtoNW | SHARP_StoSW | HOLE )

struct block
{
    int  r1, c1;
    long b1;
    int  r2, c2;
    long b2;
};

/* blocking masks for diagonal traces */
static struct block blocking[8] =
{ {
      0, -1,
      BLOCK_NORTHEAST,
      1, 0,
      BLOCK_SOUTHWEST
  },
  {
      0, 0, 0,
      0, 0, 0
  },
  {
      1, 0,
      BLOCK_SOUTHEAST,
      0, 1,
      BLOCK_NORTHWEST
  },
  {
      0, 0, 0,
      0, 0, 0
  },
  {
      0, 0, 0,
      0, 0, 0
  },
  {
      0, -1,
      BLOCK_SOUTHEAST,
      -1, 0,
      BLOCK_NORTHWEST
  },
  {
      0, 0, 0,
      0, 0, 0
  },
  {
      -1, 0,
      BLOCK_NORTHEAST,
      0, 1,
      BLOCK_SOUTHWEST
  } };

/* mask for hole-related blocking effects */
static struct
{
    long trace;
    int  present;
} selfok2[8] =
{
    { HOLE_NORTHWEST, 0 },
    { HOLE_NORTH,     0 },
    { HOLE_NORTHEAST, 0 },
    { HOLE_WEST,      0 },
    { HOLE_EAST,      0 },
    { HOLE_SOUTHWEST, 0 },
    { HOLE_SOUTH,     0 },
    { HOLE_SOUTHEAST, 0 }
};

static long newmask[8] =
{
    /* patterns to mask out in neighbor cells */
    0,
    CORNER_NORTHWEST | CORNER_NORTHEAST,
    0,
    CORNER_NORTHWEST | CORNER_SOUTHWEST,
    CORNER_NORTHEAST | CORNER_SOUTHEAST,
    0,
    CORNER_SOUTHWEST | CORNER_SOUTHEAST,
    0
};


/* Route all traces
 * :
 *  1 if OK
 * -1 if escape (stop being routed) request
 * -2 if default memory allocation
 */
int PCB_EDIT_FRAME::Solve( wxDC* DC, int aLayersCount )
{
    int           current_net_code;
    int           row_source, col_source, row_target, col_target;
    int           success, nbsucces = 0, nbunsucces = 0;
    NETINFO_ITEM* net;
    bool          stop = false;
    wxString      msg;
    int           routedCount = 0;      // routed ratsnest count
    bool          two_sides = aLayersCount == 2;

    m_canvas->SetAbortRequest( false );

    s_Clearance = GetBoard()->GetDesignSettings().m_NetClasses.GetDefault()->GetClearance();

    // Prepare the undo command info
    s_ItemsListPicker.ClearListAndDeleteItems();  // Should not be necessary, but...

    /* go until no more work to do */
    GetWork( &row_source, &col_source, &current_net_code,
             &row_target, &col_target, &pt_cur_ch ); // First net to route.

    for( ; row_source != ILLEGAL; GetWork( &row_source, &col_source,
                                           &current_net_code, &row_target,
                                           &col_target,
                                           &pt_cur_ch ) )
    {
        /* Test to stop routing ( escape key pressed ) */
        wxYield();

        if( m_canvas->GetAbortRequest() )
        {
            if( IsOK( this, _( "Abort routing?" ) ) )
            {
                success = STOP_FROM_ESC;
                stop    = true;
                break;
            }
            else
            {
                m_canvas->SetAbortRequest( false );
            }
        }

        EraseMsgBox();

        routedCount++;
        net = GetBoard()->FindNet( current_net_code );

        if( net )
        {
            msg.Printf( wxT( "[%8.8s]" ), GetChars( net->GetNetname() ) );
            AppendMsgPanel( wxT( "Net route" ), msg, BROWN );
            msg.Printf( wxT( "%d / %d" ), routedCount, RoutingMatrix.m_RouteCount );
            AppendMsgPanel( wxT( "Activity" ), msg, BROWN );
        }

        segm_oX = GetBoard()->GetBoundingBox().GetX() + (RoutingMatrix.m_GridRouting * col_source);
        segm_oY = GetBoard()->GetBoundingBox().GetY() + (RoutingMatrix.m_GridRouting * row_source);
        segm_fX = GetBoard()->GetBoundingBox().GetX() + (RoutingMatrix.m_GridRouting * col_target);
        segm_fY = GetBoard()->GetBoundingBox().GetY() + (RoutingMatrix.m_GridRouting * row_target);

        /* Draw segment. */
        GRLine( m_canvas->GetClipBox(), DC,
                segm_oX, segm_oY, segm_fX, segm_fY,
                0, WHITE );
        pt_cur_ch->m_PadStart->Draw( m_canvas, DC, GR_OR | GR_HIGHLIGHT );
        pt_cur_ch->m_PadEnd->Draw( m_canvas, DC, GR_OR | GR_HIGHLIGHT );

        success = Autoroute_One_Track( this, DC,
                                       two_sides, row_source, col_source,
                                       row_target, col_target, pt_cur_ch );

        switch( success )
        {
        case NOSUCCESS:
            pt_cur_ch->m_Status |= CH_UNROUTABLE;
            nbunsucces++;
            break;

        case STOP_FROM_ESC:
            stop = true;
            break;

        case ERR_MEMORY:
            stop = true;
            break;

        default:
            nbsucces++;
            break;
        }

        msg.Printf( wxT( "%d" ), nbsucces );
        AppendMsgPanel( wxT( "OK" ), msg, GREEN );
        msg.Printf( wxT( "%d" ), nbunsucces );
        AppendMsgPanel( wxT( "Fail" ), msg, RED );
        msg.Printf( wxT( "  %d" ), GetBoard()->GetUnconnectedNetCount() );
        AppendMsgPanel( wxT( "Not Connected" ), msg, CYAN );

        /* Delete routing from display. */
        pt_cur_ch->m_PadStart->Draw( m_canvas, DC, GR_AND );
        pt_cur_ch->m_PadEnd->Draw( m_canvas, DC, GR_AND );

        if( stop )
            break;
    }

    SaveCopyInUndoList( s_ItemsListPicker, UR_UNSPECIFIED );
    s_ItemsListPicker.ClearItemsList(); // s_ItemsListPicker is no more owner of picked items

    return SUCCESS;
}


/* Route a trace on the BOARD.
 * Parameters:
 * 1 side / 2 sides (0 / 1)
 * Coord source (row, col)
 * Coord destination (row, col)
 * Net_code
 * Pointer to the ratsnest reference
 *
 * Returns:
 * SUCCESS if routed
 * TRIVIAL_SUCCESS if pads are connected by overlay (no track needed)
 * If failure NOSUCCESS
 * Escape STOP_FROM_ESC if demand
 * ERR_MEMORY if memory allocation failed.
 */
static int Autoroute_One_Track( PCB_EDIT_FRAME* pcbframe,
                                wxDC*           DC,
                                int             two_sides,
                                int             row_source,
                                int             col_source,
                                int             row_target,
                                int             col_target,
                                RATSNEST_ITEM*  pt_rat )
{
    int          r, c, side, d, apx_dist, nr, nc;
    int          result, skip;
    int          i;
    long         curcell, newcell, buddy, lastopen, lastclos, lastmove;
    int          newdist, olddir, _self;
    int          current_net_code;
    int          marge;
    int          padLayerMaskStart;    /* Mask layers belonging to the starting pad. */
    int          padLayerMaskEnd;      /* Mask layers belonging to the ending pad. */
    int          topLayerMask = GetLayerMask( g_Route_Layer_TOP );
    int          bottomLayerMask = GetLayerMask( g_Route_Layer_BOTTOM );
    int          routeLayerMask;       /* Mask two layers for routing. */
    int          tab_mask[2];       /* Enables the calculation of the mask layer being
                                     * tested. (side = TOP or BOTTOM) */
    int          start_mask_layer = 0;
    wxString     msg;

    wxBusyCursor dummy_cursor;      // Set an hourglass cursor while routing a
                                    // track

    result = NOSUCCESS;

    marge = s_Clearance + ( pcbframe->GetDesignSettings().GetCurrentTrackWidth() / 2 );

    /* clear direction flags */
    i = RoutingMatrix.m_Nrows * RoutingMatrix.m_Ncols * sizeof(DIR_CELL);

    if( two_sides )
        memset( RoutingMatrix.m_DirSide[TOP], FROM_NOWHERE, i );
    memset( RoutingMatrix.m_DirSide[BOTTOM], FROM_NOWHERE, i );

    lastopen = lastclos = lastmove = 0;

    /* Set tab_masque[side] for final test of routing. */
    if( two_sides )
        tab_mask[TOP]    = topLayerMask;
    tab_mask[BOTTOM] = bottomLayerMask;

    /* Set active layers mask. */
    routeLayerMask = topLayerMask | bottomLayerMask;

    pt_cur_ch = pt_rat;

    current_net_code  = pt_rat->GetNet();
    padLayerMaskStart = pt_cur_ch->m_PadStart->GetLayerMask();

    padLayerMaskEnd = pt_cur_ch->m_PadEnd->GetLayerMask();


    /* First Test if routing possible ie if the pads are accessible
     * on the routing layers.
     */
    if( ( routeLayerMask & padLayerMaskStart ) == 0 )
        goto end_of_route;

    if( ( routeLayerMask & padLayerMaskEnd ) == 0 )
        goto end_of_route;

    /* Then test if routing possible ie if the pads are accessible
     * On the routing grid (1 grid point must be in the pad)
     */
    {
        int cX = ( RoutingMatrix.m_GridRouting * col_source )
                 + pcbframe->GetBoard()->GetBoundingBox().GetX();
        int cY = ( RoutingMatrix.m_GridRouting * row_source )
                 + pcbframe->GetBoard()->GetBoundingBox().GetY();
        int dx = pt_cur_ch->m_PadStart->GetSize().x / 2;
        int dy = pt_cur_ch->m_PadStart->GetSize().y / 2;
        int px = pt_cur_ch->m_PadStart->GetPosition().x;
        int py = pt_cur_ch->m_PadStart->GetPosition().y;

        if( ( ( int( pt_cur_ch->m_PadStart->GetOrientation() ) / 900 ) & 1 ) != 0 )
            EXCHG( dx, dy );

        if( ( abs( cX - px ) > dx ) || ( abs( cY - py ) > dy ) )
            goto end_of_route;

        cX = ( RoutingMatrix.m_GridRouting * col_target )
             + pcbframe->GetBoard()->GetBoundingBox().GetX();
        cY = ( RoutingMatrix.m_GridRouting * row_target )
             + pcbframe->GetBoard()->GetBoundingBox().GetY();
        dx = pt_cur_ch->m_PadEnd->GetSize().x / 2;
        dy = pt_cur_ch->m_PadEnd->GetSize().y / 2;
        px = pt_cur_ch->m_PadEnd->GetPosition().x;
        py = pt_cur_ch->m_PadEnd->GetPosition().y;

        if( ( ( int( pt_cur_ch->m_PadEnd->GetOrientation() ) / 900) & 1 ) != 0 )
            EXCHG( dx, dy );

        if( ( abs( cX - px ) > dx ) || ( abs( cY - py ) > dy ) )
            goto end_of_route;
    }

    /* Test the trivial case: direct connection overlay pads. */
    if( ( row_source == row_target ) && ( col_source == col_target )
       && ( padLayerMaskEnd & padLayerMaskStart &
            g_TabAllCopperLayerMask[pcbframe->GetBoard()->GetCopperLayerCount() - 1] ) )
    {
        result = TRIVIAL_SUCCESS;
        goto end_of_route;
    }

    /* Placing the bit to remove obstacles on 2 pads to a link. */
    pcbframe->SetStatusText( wxT( "Gen Cells" ) );

    PlacePad( pt_cur_ch->m_PadStart, CURRENT_PAD, marge, WRITE_OR_CELL );
    PlacePad( pt_cur_ch->m_PadEnd, CURRENT_PAD, marge, WRITE_OR_CELL );

    /* Regenerates the remaining barriers (which may encroach on the placement bits precedent)
     */
    i = pcbframe->GetBoard()->GetPadCount();

    for( unsigned ii = 0; ii < pcbframe->GetBoard()->GetPadCount(); ii++ )
    {
        D_PAD* ptr = pcbframe->GetBoard()->GetPad( ii );

        if( ( pt_cur_ch->m_PadStart != ptr ) && ( pt_cur_ch->m_PadEnd != ptr ) )
        {
            PlacePad( ptr, ~CURRENT_PAD, marge, WRITE_AND_CELL );
        }
    }

    InitQueue(); /* initialize the search queue */
    apx_dist = RoutingMatrix.GetApxDist( row_source, col_source, row_target, col_target );

    /* Initialize first search. */
    if( two_sides )   /* Preferred orientation. */
    {
        if( abs( row_target - row_source ) > abs( col_target - col_source ) )
        {
            if( padLayerMaskStart & topLayerMask )
            {
                start_mask_layer = 2;

                if( SetQueue( row_source, col_source, TOP, 0, apx_dist,
                              row_target, col_target ) == 0 )
                {
                    return ERR_MEMORY;
                }
            }

            if( padLayerMaskStart & bottomLayerMask )
            {
                start_mask_layer |= 1;

                if( SetQueue( row_source, col_source, BOTTOM, 0, apx_dist,
                              row_target, col_target ) == 0 )
                {
                    return ERR_MEMORY;
                }
            }
        }
        else
        {
            if( padLayerMaskStart & bottomLayerMask )
            {
                start_mask_layer = 1;

                if( SetQueue( row_source, col_source, BOTTOM, 0, apx_dist,
                              row_target, col_target ) == 0 )
                {
                    return ERR_MEMORY;
                }
            }

            if( padLayerMaskStart & topLayerMask )
            {
                start_mask_layer |= 2;

                if( SetQueue( row_source, col_source, TOP, 0, apx_dist,
                              row_target, col_target ) == 0 )
                {
                    return ERR_MEMORY;
                }
            }
        }
    }
    else if( padLayerMaskStart & bottomLayerMask )
    {
        start_mask_layer = 1;

        if( SetQueue( row_source, col_source, BOTTOM, 0, apx_dist, row_target, col_target ) == 0 )
        {
            return ERR_MEMORY;
        }
    }

    /* search until success or we exhaust all possibilities */
    GetQueue( &r, &c, &side, &d, &apx_dist );

    for( ; r != ILLEGAL; GetQueue( &r, &c, &side, &d, &apx_dist ) )
    {
        curcell = RoutingMatrix.GetCell( r, c, side );

        if( curcell & CURRENT_PAD )
            curcell &= ~HOLE;

        if( (r == row_target) && (c == col_target)  /* success if layer OK */
           && ( tab_mask[side] & padLayerMaskEnd) )
        {
            /* Remove link. */
            GRSetDrawMode( DC, GR_XOR );
            GRLine( pcbframe->GetCanvas()->GetClipBox(),
                    DC,
                    segm_oX,
                    segm_oY,
                    segm_fX,
                    segm_fY,
                    0,
                    WHITE );

            /* Generate trace. */
            if( Retrace( pcbframe, DC, row_source, col_source,
                         row_target, col_target, side, current_net_code ) )
            {
                result = SUCCESS;   /* Success : Route OK */
            }

            break;                  /* Routing complete. */
        }

        if( pcbframe->GetCanvas()->GetAbortRequest() )
        {
            result = STOP_FROM_ESC;
            break;
        }

        /* report every COUNT new nodes or so */
        #define COUNT 20000

        if( ( OpenNodes - lastopen > COUNT )
           || ( ClosNodes - lastclos > COUNT )
           || ( MoveNodes - lastmove > COUNT ) )
        {
            lastopen = OpenNodes;
            lastclos = ClosNodes;
            lastmove = MoveNodes;
            msg.Printf( wxT( "Activity: Open %d   Closed %d   Moved %d" ),
                        OpenNodes, ClosNodes, MoveNodes );
            pcbframe->SetStatusText( msg );
        }

        _self = 0;

        if( curcell & HOLE )
        {
            _self = 5;

            /* set 'present' bits */
            for( i = 0; i < 8; i++ )
            {
                selfok2[i].present = 0;

                if( curcell & selfok2[i].trace )
                    selfok2[i].present = 1;
            }
        }

        for( i = 0; i < 8; i++ ) /* consider neighbors */
        {
            nr = r + delta[i][0];
            nc = c + delta[i][1];

            /* off the edge? */
            if( nr < 0 || nr >= RoutingMatrix.m_Nrows ||
                nc < 0 || nc >= RoutingMatrix.m_Ncols )
                continue;  /* off the edge */

            if( _self == 5 && selfok2[i].present )
                continue;

            newcell = RoutingMatrix.GetCell( nr, nc, side );

            if( newcell & CURRENT_PAD )
                newcell &= ~HOLE;

            /* check for non-target hole */
            if( newcell & HOLE )
            {
                if( nr != row_target || nc != col_target )
                    continue;
            }
            /* check for traces */
            else if( newcell & HOLE & ~(newmask[i]) )
            {
                continue;
            }

            /* check blocking on corner neighbors */
            if( delta[i][0] && delta[i][1] )
            {
                /* check first buddy */
                buddy = RoutingMatrix.GetCell( r + blocking[i].r1, c + blocking[i].c1, side );

                if( buddy & CURRENT_PAD )
                    buddy &= ~HOLE;

                if( buddy & HOLE )
                    continue;

//              if (buddy & (blocking[i].b1)) continue;
                /* check second buddy */
                buddy = RoutingMatrix.GetCell( r + blocking[i].r2, c + blocking[i].c2, side );

                if( buddy & CURRENT_PAD )
                    buddy &= ~HOLE;

                if( buddy & HOLE )
                    continue;

//              if (buddy & (blocking[i].b2)) continue;
            }

            olddir  = RoutingMatrix.GetDir( r, c, side );
            newdist = d + RoutingMatrix.CalcDist( ndir[i], olddir,
                                    ( olddir == FROM_OTHERSIDE ) ?
                                    RoutingMatrix.GetDir( r, c, 1 - side ) : 0, side );

            /* if (a) not visited yet, or (b) we have */
            /* found a better path, add it to queue */
            if( !RoutingMatrix.GetDir( nr, nc, side ) )
            {
                RoutingMatrix.SetDir( nr, nc, side, ndir[i] );
                RoutingMatrix.SetDist( nr, nc, side, newdist );

                if( SetQueue( nr, nc, side, newdist,
                              RoutingMatrix.GetApxDist( nr, nc, row_target, col_target ),
                              row_target, col_target ) == 0 )
                {
                    return ERR_MEMORY;
                }
            }
            else if( newdist < RoutingMatrix.GetDist( nr, nc, side ) )
            {
                RoutingMatrix.SetDir( nr, nc, side, ndir[i] );
                RoutingMatrix.SetDist( nr, nc, side, newdist );
                ReSetQueue( nr, nc, side, newdist,
                            RoutingMatrix.GetApxDist( nr, nc, row_target, col_target ),
                            row_target, col_target );
            }
        }

        /** Test the other layer. **/
        if( two_sides )
        {
            olddir = RoutingMatrix.GetDir( r, c, side );

            if( olddir == FROM_OTHERSIDE )
                continue;   /* useless move, so don't bother */

            if( curcell )   /* can't drill via if anything here */
                continue;

            /* check for holes or traces on other side */
            if( ( newcell = RoutingMatrix.GetCell( r, c, 1 - side ) ) != 0 )
                continue;

            /* check for nearby holes or traces on both sides */
            for( skip = 0, i = 0; i < 8; i++ )
            {
                nr = r + delta[i][0]; nc = c + delta[i][1];

                if( nr < 0 || nr >= RoutingMatrix.m_Nrows ||
                    nc < 0 || nc >= RoutingMatrix.m_Ncols )
                    continue;  /* off the edge !! */

                if( RoutingMatrix.GetCell( nr, nc, side ) /* & blocking2[i]*/ )
                {
                    skip = 1; /* can't drill via here */
                    break;
                }

                if( RoutingMatrix.GetCell( nr, nc, 1 - side ) /* & blocking2[i]*/ )
                {
                    skip = 1; /* can't drill via here */
                    break;
                }
            }

            if( skip )      /* neighboring hole or trace? */
                continue;   /* yes, can't drill via here */

            newdist = d + RoutingMatrix.CalcDist( FROM_OTHERSIDE, olddir, 0, side );

            /*  if (a) not visited yet,
             *  or (b) we have found a better path,
             *  add it to queue */
            if( !RoutingMatrix.GetDir( r, c, 1 - side ) )
            {
                RoutingMatrix.SetDir( r, c, 1 - side, FROM_OTHERSIDE );
                RoutingMatrix.SetDist( r, c, 1 - side, newdist );

                if( SetQueue( r, c, 1 - side, newdist, apx_dist, row_target, col_target ) == 0 )
                {
                    return ERR_MEMORY;
                }
            }
            else if( newdist < RoutingMatrix.GetDist( r, c, 1 - side ) )
            {
                RoutingMatrix.SetDir( r, c, 1 - side, FROM_OTHERSIDE );
                RoutingMatrix.SetDist( r, c, 1 - side, newdist );
                ReSetQueue( r, c,
                            1 - side,
                            newdist,
                            apx_dist,
                            row_target,
                            col_target );
            }
        }     /* Finished attempt to route on other layer. */
    }

end_of_route:
    PlacePad( pt_cur_ch->m_PadStart, ~CURRENT_PAD, marge, WRITE_AND_CELL );
    PlacePad( pt_cur_ch->m_PadEnd, ~CURRENT_PAD, marge, WRITE_AND_CELL );

    msg.Printf( wxT( "Activity: Open %d   Closed %d   Moved %d"),
                OpenNodes, ClosNodes, MoveNodes );
    pcbframe->SetStatusText( msg );

    return result;
}


static long bit[8][9] =
{
    /* OT=Otherside */
    /* N, NE, E, SE, S, SW, W, NW, OT */
/* N */
    { LINE_VERTICAL,
      BENT_StoNE,
      CORNER_SOUTHEAST,
      SHARP_StoSE,
      0,
      SHARP_StoSW,
      CORNER_SOUTHWEST,
      BENT_StoNW,
      ( HOLE | HOLE_SOUTH )
    },
/* NE */
    {
        BENT_NtoSW,
        DIAG_NEtoSW,
        BENT_EtoSW,
        ANGLE_SEtoSW,
        SHARP_StoSW,
        0,
        SHARP_WtoSW,
        ANGLE_SWtoNW,
        ( HOLE | HOLE_SOUTHWEST )
    },
/* E */
    {
        CORNER_NORTHWEST,
        BENT_WtoNE,
        LINE_HORIZONTAL,
        BENT_WtoSE,
        CORNER_SOUTHWEST,
        SHARP_WtoSW,
        0,
        SHARP_WtoNW,
        ( HOLE | HOLE_WEST )
    },
/* SE */
    {
        SHARP_NtoNW,
        ANGLE_NWtoNE,
        BENT_EtoNW,
        DIAG_SEtoNW,
        BENT_StoNW,
        ANGLE_SWtoNW,
        SHARP_WtoNW,
        0,
        ( HOLE | HOLE_NORTHWEST )
    },
/* S */
    {
        0,
        SHARP_NtoNE,
        CORNER_NORTHEAST,
        BENT_NtoSE,
        LINE_VERTICAL,
        BENT_NtoSW,
        CORNER_NORTHWEST,
        SHARP_NtoNW,
        ( HOLE | HOLE_NORTH )
    },
/* SW */
    {
        SHARP_NtoNE,
        0,
        SHARP_EtoNE,
        ANGLE_NEtoSE,
        BENT_StoNE,
        DIAG_NEtoSW,
        BENT_WtoNE,
        ANGLE_NWtoNE,
        ( HOLE | HOLE_NORTHEAST )
    },
/* W */
    {
        CORNER_NORTHEAST,
        SHARP_EtoNE,
        0,
        SHARP_EtoSE,
        CORNER_SOUTHEAST,
        BENT_EtoSW,
        LINE_HORIZONTAL,
        BENT_EtoNW,
        ( HOLE | HOLE_EAST )
    },
/* NW */
    {
        BENT_NtoSE,
        ANGLE_NEtoSE,
        SHARP_EtoSE,
        0,
        SHARP_StoSE,
        ANGLE_SEtoSW,
        BENT_WtoSE,
        DIAG_SEtoNW,
        ( HOLE | HOLE_SOUTHEAST )
    }
};


/* work from target back to source, actually laying the traces
 *  Parameters:
 *      start on side target_side, of coordinates row_target, col_target.
 *      arrive on side masque_layer_start, coordinate row_source, col_source
 * The search is done in reverse routing, the point of arrival (target) to
 * the starting point (source)
 * The router.
 *
 * Target_side = symbol (TOP / BOTTOM) of departure
 * = Mask_layer_source mask layers Arrival
 *
 * Returns:
 * 0 if error
 * > 0 if Ok
 */
static int Retrace( PCB_EDIT_FRAME* pcbframe, wxDC* DC,
                    int row_source, int col_source,
                    int row_target, int col_target, int target_side,
                    int current_net_code )
{
    int  r0, c0, s0;
    int  r1, c1, s1;    /* row, col, starting side. */
    int  r2, c2, s2;    /* row, col, ending side. */
    int  x, y = -1;
    long b;

    r1 = row_target;
    c1 = col_target;    /* start point is target ( end point is source )*/
    s1 = target_side;
    r0 = c0 = s0 = ILLEGAL;

    wxASSERT( g_CurrentTrackList.GetCount() == 0 );

    do
    {
        /* find where we came from to get here */
        r2 = r1; c2 = c1; s2 = s1;
        x  = RoutingMatrix.GetDir( r1, c1, s1 );

        switch( x )
        {
        case FROM_NORTH:
            r2++;
            break;

        case FROM_EAST:
            c2++;
            break;

        case FROM_SOUTH:
            r2--;
            break;

        case FROM_WEST:
            c2--;
            break;

        case FROM_NORTHEAST:
            r2++;
            c2++;
            break;

        case FROM_SOUTHEAST:
            r2--;
            c2++;
            break;

        case FROM_SOUTHWEST:
            r2--;
            c2--;
            break;

        case FROM_NORTHWEST:
            r2++;
            c2--;
            break;

        case FROM_OTHERSIDE:
            s2 = 1 - s2;
            break;

        default:
            wxMessageBox( wxT( "Retrace: internal error: no way back" ) );
            return 0;
        }

        if( r0 != ILLEGAL )
            y = RoutingMatrix.GetDir( r0, c0, s0 );

        /* see if target or hole */
        if( ( ( r1 == row_target ) && ( c1 == col_target ) ) || ( s1 != s0 ) )
        {
            int p_dir;

            switch( x )
            {
            case FROM_NORTH:
                p_dir = HOLE_NORTH;
                break;

            case FROM_EAST:
                p_dir = HOLE_EAST;
                break;

            case FROM_SOUTH:
                p_dir = HOLE_SOUTH;
                break;

            case FROM_WEST:
                p_dir = HOLE_WEST;
                break;

            case FROM_NORTHEAST:
                p_dir = HOLE_NORTHEAST;
                break;

            case FROM_SOUTHEAST:
                p_dir = HOLE_SOUTHEAST;
                break;

            case FROM_SOUTHWEST:
                p_dir = HOLE_SOUTHWEST;
                break;

            case FROM_NORTHWEST:
                p_dir = HOLE_NORTHWEST;
                break;

            case FROM_OTHERSIDE:
            default:
                DisplayError( pcbframe, wxT( "Retrace: error 1" ) );
                return 0;
            }

            OrCell_Trace( pcbframe->GetBoard(), r1, c1, s1, p_dir, current_net_code );
        }
        else
        {
            if( ( y == FROM_NORTH || y == FROM_NORTHEAST
                  || y == FROM_EAST || y == FROM_SOUTHEAST
                  || y == FROM_SOUTH || y == FROM_SOUTHWEST
                  || y == FROM_WEST || y == FROM_NORTHWEST )
               && ( x == FROM_NORTH || x == FROM_NORTHEAST
                    || x == FROM_EAST || x == FROM_SOUTHEAST
                    || x == FROM_SOUTH || x == FROM_SOUTHWEST
                    || x == FROM_WEST || x == FROM_NORTHWEST
                    || x == FROM_OTHERSIDE )
               && ( ( b = bit[y - 1][x - 1] ) != 0 ) )
            {
                OrCell_Trace( pcbframe->GetBoard(), r1, c1, s1, b, current_net_code );

                if( b & HOLE )
                    OrCell_Trace( pcbframe->GetBoard(), r2, c2, s2, HOLE, current_net_code );
            }
            else
            {
                wxMessageBox( wxT( "Retrace: error 2" ) );
                return 0;
            }
        }

        if( ( r2 == row_source ) && ( c2 == col_source ) ) /* see if source */
        {
            int p_dir;

            switch( x )
            {
            case FROM_NORTH:
                p_dir = HOLE_SOUTH;
                break;

            case FROM_EAST:
                p_dir = HOLE_WEST;
                break;

            case FROM_SOUTH:
                p_dir = HOLE_NORTH;
                break;

            case FROM_WEST:
                p_dir = HOLE_EAST;
                break;

            case FROM_NORTHEAST:
                p_dir = HOLE_SOUTHWEST;
                break;

            case FROM_SOUTHEAST:
                p_dir = HOLE_NORTHWEST;
                break;

            case FROM_SOUTHWEST:
                p_dir = HOLE_NORTHEAST;
                break;

            case FROM_NORTHWEST:
                p_dir = HOLE_SOUTHEAST;
                break;

            case FROM_OTHERSIDE:
            default:
                wxMessageBox( wxT( "Retrace: error 3" ) );
                return 0;
            }

            OrCell_Trace( pcbframe->GetBoard(), r2, c2, s2, p_dir, current_net_code );
        }

        /* move to next cell */
        r0 = r1;
        c0 = c1;
        s0 = s1;
        r1 = r2;
        c1 = c2;
        s1 = s2;
    } while( !( ( r2 == row_source ) && ( c2 == col_source ) ) );

    AddNewTrace( pcbframe, DC );
    return 1;
}


/* This function is used by Retrace and read the autorouting matrix data cells to create
 * the real track on the physical board
 */
static void OrCell_Trace( BOARD* pcb, int col, int row,
                          int side, int orient, int current_net_code )
{
    if( orient == HOLE )  // placement of a via
    {
        VIA *newVia = new VIA( pcb );

        g_CurrentTrackList.PushBack( newVia );

        g_CurrentTrackSegment->SetState( TRACK_AR, true );
        g_CurrentTrackSegment->SetLayer( 0x0F );

        g_CurrentTrackSegment->SetStart(wxPoint( pcb->GetBoundingBox().GetX() +
                                                ( RoutingMatrix.m_GridRouting * row ),
                                                pcb->GetBoundingBox().GetY() +
                                                ( RoutingMatrix.m_GridRouting * col )));
        g_CurrentTrackSegment->SetEnd( g_CurrentTrackSegment->GetStart() );

        g_CurrentTrackSegment->SetWidth( pcb->GetDesignSettings().GetCurrentViaSize() );
        newVia->SetViaType( pcb->GetDesignSettings().m_CurrentViaType );

        g_CurrentTrackSegment->SetNetCode( current_net_code );
    }
    else    // placement of a standard segment
    {
        TRACK *newTrack = new TRACK( pcb );
        int    dx0, dy0, dx1, dy1;


        g_CurrentTrackList.PushBack( newTrack );

        g_CurrentTrackSegment->SetLayer( g_Route_Layer_BOTTOM );

        if( side == TOP )
            g_CurrentTrackSegment->SetLayer( g_Route_Layer_TOP );

        g_CurrentTrackSegment->SetState( TRACK_AR, true );
        g_CurrentTrackSegment->SetEnd( wxPoint( pcb->GetBoundingBox().GetX() +
                                         ( RoutingMatrix.m_GridRouting * row ),
                                         pcb->GetBoundingBox().GetY() +
                                         ( RoutingMatrix.m_GridRouting * col )));
        g_CurrentTrackSegment->SetNetCode( current_net_code );

        if( g_CurrentTrackSegment->Back() == NULL ) /* Start trace. */
        {
            g_CurrentTrackSegment->SetStart( wxPoint( segm_fX, segm_fY ) );

            /* Placement on the center of the pad if outside grid. */
            dx1 = g_CurrentTrackSegment->GetEnd().x - g_CurrentTrackSegment->GetStart().x;
            dy1 = g_CurrentTrackSegment->GetEnd().y - g_CurrentTrackSegment->GetStart().y;

            dx0 = pt_cur_ch->m_PadEnd->GetPosition().x - g_CurrentTrackSegment->GetStart().x;
            dy0 = pt_cur_ch->m_PadEnd->GetPosition().y - g_CurrentTrackSegment->GetStart().y;

            /* If aligned, change the origin point. */
            if( abs( dx0 * dy1 ) == abs( dx1 * dy0 ) )
            {
                g_CurrentTrackSegment->SetStart( pt_cur_ch->m_PadEnd->GetPosition() );
            }
            else    // Creation of a supplemental segment
            {
                g_CurrentTrackSegment->SetStart( pt_cur_ch->m_PadEnd->GetPosition() );

                newTrack = (TRACK*)g_CurrentTrackSegment->Clone();
                newTrack->SetStart( g_CurrentTrackSegment->GetEnd());

                g_CurrentTrackList.PushBack( newTrack );
            }
        }
        else
        {
            if( g_CurrentTrackSegment->Back() )
            {
                g_CurrentTrackSegment->SetStart( g_CurrentTrackSegment->Back()->GetEnd() );
            }
        }

        g_CurrentTrackSegment->SetWidth( pcb->GetDesignSettings().GetCurrentTrackWidth() );

        if( g_CurrentTrackSegment->GetStart() != g_CurrentTrackSegment->GetEnd() )
        {
            /* Reduce aligned segments by one. */
            TRACK* oldTrack = g_CurrentTrackSegment->Back();

            if( oldTrack &&  oldTrack->Type() != PCB_VIA_T )
            {
                dx1 = g_CurrentTrackSegment->GetEnd().x - g_CurrentTrackSegment->GetStart().x;
                dy1 = g_CurrentTrackSegment->GetEnd().y - g_CurrentTrackSegment->GetStart().y;

                dx0 = oldTrack->GetEnd().x - oldTrack->GetStart().x;
                dy0 = oldTrack->GetEnd().y - oldTrack->GetStart().y;

                if( abs( dx0 * dy1 ) == abs( dx1 * dy0 ) )
                {
                    oldTrack->SetEnd( g_CurrentTrackSegment->GetEnd() );

                    delete g_CurrentTrackList.PopBack();
                }
            }
        }
    }
}


/* Insert the new track created in the list of tracks.
 * amend the points of beginning and end of the track so that they are
 * connected
 * Center on pads even if they are off grid.
 */
static void AddNewTrace( PCB_EDIT_FRAME* pcbframe, wxDC* DC )
{
    if( g_FirstTrackSegment == NULL )
        return;

    int dx0, dy0, dx1, dy1;
    int marge, via_marge;
    EDA_DRAW_PANEL* panel = pcbframe->GetCanvas();
    PCB_SCREEN* screen = pcbframe->GetScreen();

    marge = s_Clearance + ( pcbframe->GetDesignSettings().GetCurrentTrackWidth() / 2 );
    via_marge = s_Clearance + ( pcbframe->GetDesignSettings().GetCurrentViaSize() / 2 );

    dx1 = g_CurrentTrackSegment->GetEnd().x - g_CurrentTrackSegment->GetStart().x;
    dy1 = g_CurrentTrackSegment->GetEnd().y - g_CurrentTrackSegment->GetStart().y;

    /* Place on center of pad if off grid. */
    dx0 = pt_cur_ch->m_PadStart->GetPosition().x - g_CurrentTrackSegment->GetStart().x;
    dy0 = pt_cur_ch->m_PadStart->GetPosition().y - g_CurrentTrackSegment->GetStart().y;

    /* If aligned, change the origin point. */
    if( abs( dx0 * dy1 ) == abs( dx1 * dy0 ) )
    {
        g_CurrentTrackSegment->SetEnd( pt_cur_ch->m_PadStart->GetPosition() );
    }
    else
    {
        TRACK* newTrack = (TRACK*)g_CurrentTrackSegment->Clone();

        newTrack->SetEnd( pt_cur_ch->m_PadStart->GetPosition() );
        newTrack->SetStart( g_CurrentTrackSegment->GetEnd() );

        g_CurrentTrackList.PushBack( newTrack );
    }

    g_FirstTrackSegment->start = pcbframe->GetBoard()->GetPad( g_FirstTrackSegment,
            ENDPOINT_START );

    if( g_FirstTrackSegment->start )
        g_FirstTrackSegment->SetState( BEGIN_ONPAD, true );

    g_CurrentTrackSegment->end = pcbframe->GetBoard()->GetPad( g_CurrentTrackSegment,
            ENDPOINT_END );

    if( g_CurrentTrackSegment->end )
        g_CurrentTrackSegment->SetState( END_ONPAD, true );

    /* Out the new track on the matrix board */
    for( TRACK* track = g_FirstTrackSegment; track; track = track->Next() )
    {
        TraceSegmentPcb( track, HOLE, marge, WRITE_CELL );
        TraceSegmentPcb( track, VIA_IMPOSSIBLE, via_marge, WRITE_OR_CELL );
    }

    // Insert new segments in  real board
    int    netcode    = g_FirstTrackSegment->GetNetCode();
    TRACK* firstTrack = g_FirstTrackSegment;
    int    newCount   = g_CurrentTrackList.GetCount();

    // Put entire new current segment list in BOARD
    TRACK* track;
    TRACK* insertBeforeMe = g_CurrentTrackSegment->GetBestInsertPoint( pcbframe->GetBoard() );

    while( ( track = g_CurrentTrackList.PopFront() ) != NULL )
    {
        ITEM_PICKER picker( track, UR_NEW );
        s_ItemsListPicker.PushItem( picker );
        pcbframe->GetBoard()->m_Track.Insert( track, insertBeforeMe );
    }

    DrawTraces( panel, DC, firstTrack, newCount, GR_OR );

    pcbframe->TestNetConnection( DC, netcode );

    screen->SetModify();
}
