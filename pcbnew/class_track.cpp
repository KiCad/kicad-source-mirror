/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2009 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
 * Copyright (C) 2009 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 1992-2011 KiCad Developers, see AUTHORS.txt for contributors.
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
 * @file class_track.h
 * @brief Functions relatives to tracks, vias and segments used to fill zones.
 */

#include "fctsys.h"
#include "gr_basic.h"
#include "common.h"
#include "trigo.h"
#include "class_drawpanel.h"
#include "class_pcb_screen.h"
#include "drawtxt.h"
#include "pcbcommon.h"
#include "colors_selection.h"
#include "wxstruct.h"
#include "macros.h"
#include "wxBasePcbFrame.h"

#include "class_board.h"
#include "class_track.h"

#include "pcbnew.h"
#include "protos.h"

/**
 * Function ShowClearance
 * tests to see if the clearance border is drawn on the given track.
 * @return bool - true if should draw clearance, else false.
 */
static bool ShowClearance( const TRACK* aTrack )
{
    // maybe return true for tracks and vias, not for zone segments
    return aTrack->GetLayer() <= LAST_COPPER_LAYER
           && ( aTrack->Type() == PCB_TRACE_T || aTrack->Type() == PCB_VIA_T )
           && ( ( DisplayOpt.ShowTrackClearanceMode == SHOW_CLEARANCE_NEW_AND_EDITED_TRACKS_AND_VIA_AREAS
            && ( aTrack->m_Flags & IS_DRAGGED || aTrack->m_Flags & IS_MOVED || aTrack->m_Flags & IS_NEW ) )
            || ( DisplayOpt.ShowTrackClearanceMode == SHOW_CLEARANCE_ALWAYS )
            );

}


/*
 * return true if the dist between p1 and p2 < max_dist
 * Currently in test (currently rasnest algos work only if p1 == p2)
 */
inline bool IsNear( wxPoint& p1, wxPoint& p2, int max_dist )
{
#if 0   // Do not change it: does not work
    int dist;
    dist = abs( p1.x - p2.x ) + abs( p1.y - p2.y );
    dist *= 7;
    dist /= 10;

    if ( dist < max_dist )
        return true;
#else
    if ( p1 == p2 )
        return true;
#endif
    return false;
}


TRACK* GetTrace( TRACK* aStartTrace, TRACK* aEndTrace, const wxPoint& aPosition, int aLayerMask )
{
    TRACK* PtSegm;

    if( aStartTrace == NULL )
        return NULL;

    for( PtSegm = aStartTrace; PtSegm != NULL; PtSegm =  PtSegm->Next() )
    {
        if( PtSegm->GetState( IS_DELETED | BUSY ) == 0 )
        {
            if( aPosition == PtSegm->m_Start )
            {
                if( aLayerMask & PtSegm->ReturnMaskLayer() )
                    return PtSegm;
            }

            if( aPosition == PtSegm->m_End )
            {
                if( aLayerMask & PtSegm->ReturnMaskLayer() )
                    return PtSegm;
            }
        }

        if( PtSegm == aEndTrace )
            break;
    }

    return NULL;
}


TRACK::TRACK( BOARD_ITEM* aParent, KICAD_T idtype ) :
    BOARD_CONNECTED_ITEM( aParent, idtype )
{
    m_Width = 0;
    m_Shape = S_SEGMENT;
    start   = end = NULL;
    SetDrillDefault();
    m_Param = 0;
}


wxString TRACK::ShowWidth() const
{
    wxString msg;

    valeur_param( m_Width, msg );

    return msg;
}


SEGZONE::SEGZONE( BOARD_ITEM* aParent ) :
    TRACK( aParent, PCB_ZONE_T )
{
}


wxString SEGZONE::GetSelectMenuText() const
{
    wxString text;
    NETINFO_ITEM* net;
    BOARD* board = GetBoard();

    text << _( "Zone" ) << wxT( " " ) << wxString::Format( wxT( "(%8.8X)" ), m_TimeStamp );

    if( board )
    {
        net = board->FindNet( GetNet() );

        if( net )
            text << wxT( " [" ) << net->GetNetname() << wxT( "]" );
    }
    else
    {
        text << _( "** BOARD NOT DEFINED **" );
    }

    text << _( " on " ) << GetLayerName();

    return text;
}


SEGVIA::SEGVIA( BOARD_ITEM* aParent ) :
    TRACK( aParent, PCB_VIA_T )
{
}


wxString SEGVIA::GetSelectMenuText() const
{
    wxString text;
    NETINFO_ITEM* net;
    BOARD* board = GetBoard();

    text << _( "Via" ) << wxT( " " ) << ShowWidth();

    int shape = GetShape();

    if( shape == VIA_BLIND_BURIED )
        text << wxT( " " ) << _( "Blind/Buried" );
    else if( shape == VIA_MICROVIA )
        text << wxT( " " ) << _( "Micro Via" );
    // else say nothing about normal (through) vias

    if( board )
    {
        net = board->FindNet( GetNet() );

        if( net )
            text << wxT( " [" ) << net->GetNetname() << wxT( "]" );

        text << wxChar( ' ' ) << _( "Net:" ) << GetNet();

        if( shape != VIA_THROUGH )
        {
            // say which layers, only two for now
            int topLayer;
            int botLayer;
            ReturnLayerPair( &topLayer, &botLayer );
            text << _( " on " ) << board->GetLayerName( topLayer ).Trim() << wxT( " <-> " )
                 << board->GetLayerName( botLayer ).Trim();
        }
    }
    else
    {
        text << _( "** BOARD NOT DEFINED **" );
    }

    return text;
}


// Copy constructor
TRACK::TRACK( const TRACK& Source ) :
    BOARD_CONNECTED_ITEM( Source )
{
    m_Shape = Source.m_Shape;
    SetNet( Source.GetNet() );

    m_Flags     = Source.m_Flags;
    SetTimeStamp( Source.m_TimeStamp );
    SetStatus( Source.GetStatus() );
    m_Start = Source.m_Start;
    m_End   = Source.m_End;
    m_Width = Source.m_Width;
    m_Drill = Source.m_Drill;
    SetSubNet( Source.GetSubNet() );
    m_Param = Source.m_Param;
}


TRACK* TRACK::Copy() const
{
    if( Type() == PCB_TRACE_T )
        return new TRACK( *this );

    if( Type() == PCB_VIA_T )
        return new SEGVIA( (const SEGVIA &) * this );

    if( Type() == PCB_ZONE_T )
        return new SEGZONE( (const SEGZONE &) * this );

    return NULL;    // should never happen
}


int TRACK::GetClearance( BOARD_CONNECTED_ITEM* aItem ) const
{
    // Currently tracks have no specific clearance parameter on a per track or per
    // segment basis.  The NETCLASS clearance is used.
    return BOARD_CONNECTED_ITEM::GetClearance( aItem );
}


int TRACK::GetDrillValue() const
{
    if( Type() != PCB_VIA_T )
        return 0;

    if( m_Drill > 0 )      // Use the specific value.
        return m_Drill;

    // Use the default value from the Netclass
    NETCLASS* netclass = GetNetClass();

    if( m_Shape == VIA_MICROVIA )
        return netclass->GetuViaDrill();

    return netclass->GetViaDrill();
}


bool TRACK::IsNull()
{
    if( ( Type() != PCB_VIA_T ) && ( m_Start == m_End ) )
        return true;
    else
        return false;
}


int TRACK::IsPointOnEnds( const wxPoint& point, int min_dist )
{
    int result = 0;

    if( min_dist < 0 )
        min_dist = m_Width / 2;

    int dx = m_Start.x - point.x;
    int dy = m_Start.y - point.y;

    if( min_dist == 0 )
    {
        if( (dx == 0) && (dy == 0 ) )
            result |= STARTPOINT;
    }
    else
    {
        double dist = hypot( (double)dx, (double) dy );

        if( min_dist >= (int) dist )
            result |= STARTPOINT;
    }

    dx = m_End.x - point.x;
    dy = m_End.y - point.y;

    if( min_dist == 0 )
    {
        if( (dx == 0) && (dy == 0 ) )
            result |= ENDPOINT;
    }
    else
    {
        double dist = hypot( (double) dx, (double) dy );

        if( min_dist >= (int) dist )
            result |= ENDPOINT;
    }

    return result;
}


EDA_RECT TRACK::GetBoundingBox() const
{
    // end of track is round, this is its radius, rounded up
    int radius = ( m_Width + 1 ) / 2;

    int ymax;
    int xmax;

    int ymin;
    int xmin;

    if( Type() == PCB_VIA_T )
    {
        // Because vias are sometimes drawn larger than their m_Width would
        // provide, erasing them using a dirty rect must also compensate for this
        // possibility (that the via is larger on screen than its m_Width would provide).
        // Because it is cheap to return a larger BoundingBox, do it so that
        // the via gets erased properly.  Do not divide width by 2 for this reason.
        radius = m_Width;

        ymax = m_Start.y;
        xmax = m_Start.x;

        ymin = m_Start.y;
        xmin = m_Start.x;
    }
    else
    {
        radius = ( m_Width + 1 ) / 2;

        ymax = MAX( m_Start.y, m_End.y );
        xmax = MAX( m_Start.x, m_End.x );

        ymin = MIN( m_Start.y, m_End.y );
        xmin = MIN( m_Start.x, m_End.x );
    }

    if( ShowClearance( this ) )
    {
        // + 1 is for the clearance line itself.
        radius += GetClearance() + 1;
    }

    ymax += radius;
    xmax += radius;

    ymin -= radius;
    xmin -= radius;

    // return a rectangle which is [pos,dim) in nature.  therefore the +1
    EDA_RECT ret( wxPoint( xmin, ymin ), wxSize( xmax - xmin + 1, ymax - ymin + 1 ) );

    return ret;
}


void TRACK::Rotate( const wxPoint& aRotCentre, double aAngle )
{
    RotatePoint( &m_Start, aRotCentre, aAngle );
    RotatePoint( &m_End, aRotCentre, aAngle );
}


void TRACK::Flip( const wxPoint& aCentre )
{
    m_Start.y = aCentre.y - (m_Start.y - aCentre.y);
    m_End.y   = aCentre.y - (m_End.y - aCentre.y);

    if( Type() == PCB_VIA_T )
    {
        // Huh?  Wouldn't it be better to us Type() != VIA and get rid of these brackets?
    }
    else
    {
        SetLayer( ChangeSideNumLayer( GetLayer() ) );
    }
}


// see class_track.h
SEARCH_RESULT TRACK::Visit( INSPECTOR* inspector, const void* testData,
                            const KICAD_T scanTypes[] )
{
    KICAD_T stype = *scanTypes;

#if 0 && defined(DEBUG)
    std::cout << GetClass().mb_str() << ' ';
#endif

    // If caller wants to inspect my type
    if( stype == Type() )
    {
        if( SEARCH_QUIT == inspector->Inspect( this, testData ) )
            return SEARCH_QUIT;
    }

    return SEARCH_CONTINUE;
}


bool SEGVIA::IsOnLayer( int layer_number ) const
{
    int bottom_layer, top_layer;

    ReturnLayerPair( &top_layer, &bottom_layer );

    if( bottom_layer <= layer_number && layer_number <= top_layer )
        return true;
    else
        return false;
}


int TRACK::ReturnMaskLayer() const
{
    if( Type() == PCB_VIA_T )
    {
        int via_type = GetShape();

        if( via_type == VIA_THROUGH )
            return ALL_CU_LAYERS;

        // VIA_BLIND_BURIED or VIA_MICRVIA:

        int bottom_layer, top_layer;

        // ReturnLayerPair() knows how layers are stored
        ( (SEGVIA*) this )->ReturnLayerPair( &top_layer, &bottom_layer );

        int layermask = 0;

        while( bottom_layer <= top_layer )
        {
            layermask |= GetLayerMask( bottom_layer++ );
        }

        return layermask;
    }
    else
    {
        return GetLayerMask( m_Layer );
    }
}


void SEGVIA::SetLayerPair( int top_layer, int bottom_layer )
{
    if( GetShape() == VIA_THROUGH )
    {
        top_layer    = LAYER_N_FRONT;
        bottom_layer = LAYER_N_BACK;
    }

    if( bottom_layer > top_layer )
        EXCHG( bottom_layer, top_layer );

    m_Layer = (top_layer & 15) + ( (bottom_layer & 15) << 4 );
}


void SEGVIA::ReturnLayerPair( int* top_layer, int* bottom_layer ) const
{
    int b_layer = LAYER_N_BACK;
    int t_layer = LAYER_N_FRONT;

    if( GetShape() != VIA_THROUGH )
    {
        b_layer = (m_Layer >> 4) & 15;
        t_layer = m_Layer & 15;

        if( b_layer > t_layer )
            EXCHG( b_layer, t_layer );
    }

    if( top_layer )
        *top_layer = t_layer;

    if( bottom_layer )
        *bottom_layer = b_layer;
}


TRACK* TRACK::GetBestInsertPoint( BOARD* aPcb )
{
    TRACK* track;

    if( Type() == PCB_ZONE_T )
        track = aPcb->m_Zone;
    else
        track = aPcb->m_Track;

    for( ; track;  track = track->Next() )
    {
        if( GetNet() <= track->GetNet() )
            return track;
    }

    return NULL;
}


TRACK* TRACK::GetStartNetCode( int NetCode )
{
    TRACK* Track = this;
    int    ii    = 0;

    if( NetCode == -1 )
        NetCode = GetNet();

    while( Track != NULL )
    {
        if( Track->GetNet() > NetCode )
            break;

        if( Track->GetNet() == NetCode )
        {
            ii++;
            break;
        }

        Track = (TRACK*) Track->Pnext;
    }

    if( ii )
        return Track;
    else
        return NULL;
}


TRACK* TRACK::GetEndNetCode( int NetCode )
{
    TRACK* NextS, * Track = this;
    int    ii = 0;

    if( Track == NULL )
        return NULL;

    if( NetCode == -1 )
        NetCode = GetNet();

    while( Track != NULL )
    {
        NextS = (TRACK*) Track->Pnext;

        if( Track->GetNet() == NetCode )
            ii++;

        if( NextS == NULL )
            break;

        if( NextS->GetNet() > NetCode )
            break;

        Track = NextS;
    }

    if( ii )
        return Track;
    else
        return NULL;
}


void TRACK::Draw( EDA_DRAW_PANEL* panel, wxDC* DC, int draw_mode, const wxPoint& aOffset )
{
    int l_trace;
    int color;
    int radius;
    int curr_layer = ( (PCB_SCREEN*) panel->GetScreen() )->m_Active_Layer;

    if( Type() == PCB_ZONE_T && DisplayOpt.DisplayZonesMode != 0 )
        return;

    BOARD * brd =  GetBoard( );
    color = brd->GetLayerColor(m_Layer);

    if( brd->IsLayerVisible( m_Layer ) == false && ( color & HIGHLIGHT_FLAG ) != HIGHLIGHT_FLAG )
        return;

    if( DisplayOpt.ContrastModeDisplay )
    {
        if( !IsOnLayer( curr_layer ) )
        {
            color &= ~MASKCOLOR;
            color |= DARKDARKGRAY;
        }
    }

    if( draw_mode & GR_HIGHLIGHT )
    {
        if( draw_mode & GR_AND )
            color &= ~HIGHLIGHT_FLAG;
        else
            color |= HIGHLIGHT_FLAG;
    }

    if( color & HIGHLIGHT_FLAG )
        color = ColorRefs[color & MASKCOLOR].m_LightColor;

    SetAlpha( &color, 150 );

    GRSetDrawMode( DC, draw_mode );


    l_trace = m_Width >> 1;

    if( m_Shape == S_CIRCLE )
    {
        radius = (int) hypot( (double) ( m_End.x - m_Start.x ),
                              (double) ( m_End.y - m_Start.y ) );

        if( DC->LogicalToDeviceXRel( l_trace ) < MIN_DRAW_WIDTH )
        {
            GRCircle( &panel->m_ClipBox, DC, m_Start.x + aOffset.x,
                      m_Start.y + aOffset.y, radius, color );
        }
        else
        {

            if( DC->LogicalToDeviceXRel( l_trace ) <= 1 ) /* Sketch mode if l_trace/zoom <= 1 */
            {
                GRCircle( &panel->m_ClipBox, DC, m_Start.x + aOffset.x,
                          m_Start.y + aOffset.y, radius, color );
            }
            else if( ( !DisplayOpt.DisplayPcbTrackFill) || GetState( FORCE_SKETCH ) )
            {
                GRCircle( &panel->m_ClipBox, DC, m_Start.x + aOffset.x,
                          m_Start.y + aOffset.y, radius - l_trace, color );
                GRCircle( &panel->m_ClipBox, DC, m_Start.x + aOffset.x,
                          m_Start.y + aOffset.y, radius + l_trace, color );
            }
            else
            {
                GRCircle( &panel->m_ClipBox, DC, m_Start.x + aOffset.x,
                          m_Start.y + aOffset.y, radius, m_Width, color );
            }
        }

        return;
    }

    if( DC->LogicalToDeviceXRel( l_trace ) < MIN_DRAW_WIDTH )
    {
        GRLine( &panel->m_ClipBox, DC, m_Start + aOffset, m_End + aOffset, 0, color );
        return;
    }

    if( !DisplayOpt.DisplayPcbTrackFill || GetState( FORCE_SKETCH ) )
    {
        GRCSegm( &panel->m_ClipBox, DC, m_Start + aOffset, m_End + aOffset, m_Width, color );
    }
    else
    {
        GRFillCSegm( &panel->m_ClipBox, DC, m_Start.x + aOffset.x,
                     m_Start.y + aOffset.y,
                     m_End.x + aOffset.x, m_End.y + aOffset.y, m_Width, color );
    }

    if( panel->GetScreen()->m_IsPrinting )
        return;

    // Show clearance for tracks, not for zone segments
    if( ShowClearance( this ) )
    {
        GRCSegm( &panel->m_ClipBox, DC, m_Start + aOffset, m_End + aOffset,
                 m_Width + (GetClearance() * 2), color );
    }

    /* Display the short netname for tracks, not for zone segments.
     *  we must filter tracks, to avoid a lot of texts.
     *  - only horizontal or vertical tracks are eligible
     *  - only  tracks with a length > 10 * thickness are eligible
     * and, of course, if we are not printing the board
     */
    if( Type() == PCB_ZONE_T )
        return;

    if( DisplayOpt.DisplayNetNamesMode == 0 || DisplayOpt.DisplayNetNamesMode == 1 )
        return;

    #define THRESHOLD 10
    if( (m_End.x - m_Start.x) != 0 &&  (m_End.y - m_Start.y) != 0 )
        return;

    int len = ABS( (m_End.x - m_Start.x) + (m_End.y - m_Start.y) );

    if( len < THRESHOLD * m_Width )
        return;

    if( DC->LogicalToDeviceXRel( m_Width ) < 6 )     // no room to display a text inside track
        return;

    if( GetNet() == 0 )
        return;

    NETINFO_ITEM* net = ( (BOARD*) GetParent() )->FindNet( GetNet() );

    if( net == NULL )
        return;

    int textlen = net->GetShortNetname().Len();

    if( textlen > 0 )
    {
        // calculate a good size for the text
        int     tsize = MIN( m_Width, len / textlen );
        wxPoint tpos  = m_Start + m_End;
        tpos.x /= 2;
        tpos.y /= 2;

        // Calculate angle: if the track segment is vertical, angle = 90 degrees
        int angle = 0;

        if( (m_End.x - m_Start.x) == 0 )    // Vertical segment
            angle = 900;                    // angle is in 0.1 degree

        if( DC->LogicalToDeviceXRel( tsize ) >= 6 )
        {
            if( !(!IsOnLayer( curr_layer )&& DisplayOpt.ContrastModeDisplay) )
            {
                tsize = (tsize * 8) / 10;       // small reduction to give a better look
                DrawGraphicText( panel, DC, tpos,
                                 WHITE, net->GetShortNetname(), angle, wxSize( tsize, tsize ),
                                 GR_TEXT_HJUSTIFY_CENTER, GR_TEXT_VJUSTIFY_CENTER, tsize / 7,
                                 false, false );
            }
        }
    }
}


void SEGVIA::Draw( EDA_DRAW_PANEL* panel, wxDC* DC, int draw_mode, const wxPoint& aOffset )
{
    int color;
    int radius;
    int curr_layer = ( (PCB_SCREEN*) panel->GetScreen() )->m_Active_Layer;

    int fillvia = 0;
    PCB_BASE_FRAME* frame  = (PCB_BASE_FRAME*) panel->GetParent();
    PCB_SCREEN*     screen = frame->GetScreen();

    if( frame->m_DisplayViaFill == FILLED )
        fillvia = 1;

    GRSetDrawMode( DC, draw_mode );

    BOARD * brd =  GetBoard( );
    color = brd->GetVisibleElementColor(VIAS_VISIBLE + m_Shape);

    if( brd->IsElementVisible( PCB_VISIBLE(VIAS_VISIBLE + m_Shape) ) == false
        && ( color & HIGHLIGHT_FLAG ) != HIGHLIGHT_FLAG )
        return;

    if( DisplayOpt.ContrastModeDisplay )
    {
        if( !IsOnLayer( curr_layer ) )
        {
            color &= ~MASKCOLOR;
            color |= DARKDARKGRAY;
        }
    }

    if( draw_mode & GR_HIGHLIGHT )
    {
        if( draw_mode & GR_AND )
            color &= ~HIGHLIGHT_FLAG;
        else
            color |= HIGHLIGHT_FLAG;
    }

    if( color & HIGHLIGHT_FLAG )
        color = ColorRefs[color & MASKCOLOR].m_LightColor;

    SetAlpha( &color, 150 );


    radius = m_Width >> 1;
    // for small via size on screen (radius < 4 pixels) draw a simplified shape

    int radius_in_pixels = DC->LogicalToDeviceXRel( radius );

    bool fast_draw = false;

    // Vias are drawn as a filled circle or a double circle. The hole will be drawn later
    int drill_radius = GetDrillValue() / 2;

    int inner_radius = radius - DC->DeviceToLogicalXRel( 2 );

    if( radius_in_pixels < 3 )
    {
        fast_draw = true;
        fillvia = false;
    }

    if( fillvia )
    {
        GRFilledCircle( &panel->m_ClipBox, DC, m_Start + aOffset, radius, color );
    }
    else
    {
        GRCircle( &panel->m_ClipBox, DC, m_Start + aOffset,radius, 0, color );
        if ( fast_draw )
            return;
        GRCircle( &panel->m_ClipBox, DC, m_Start + aOffset, inner_radius, 0, color );
    }

    // Draw the via hole if the display option allows it
    if( DisplayOpt.m_DisplayViaMode != VIA_HOLE_NOT_SHOW )
    {
        // Display all drill holes requested or Display non default holes requested
        if( (DisplayOpt.m_DisplayViaMode == ALL_VIA_HOLE_SHOW)
          || ( (drill_radius > 0 ) && !IsDrillDefault() ) )
        {
            if( fillvia )
            {
                bool blackpenstate = false;

                if( screen->m_IsPrinting )
                {
                    blackpenstate = GetGRForceBlackPenState();
                    GRForceBlackPen( false );
                    color = g_DrawBgColor;
                }
                else
                {
                    color = BLACK;     // or DARKGRAY;
                }

                if( draw_mode != GR_XOR )
                    GRSetDrawMode( DC, GR_COPY );
                else
                    GRSetDrawMode( DC, GR_XOR );

                if( DC->LogicalToDeviceXRel( drill_radius ) > 1 )  // Draw hole if large enough.
                    GRFilledCircle( &panel->m_ClipBox, DC, m_Start.x + aOffset.x,
                                    m_Start.y + aOffset.y, drill_radius, 0, color, color );

                if( screen->m_IsPrinting )
                    GRForceBlackPen( blackpenstate );
            }
            else
            {
                if( drill_radius < inner_radius )         // We can show the via hole
                    GRCircle( &panel->m_ClipBox, DC, m_Start + aOffset, drill_radius, 0, color );
            }
        }
    }

    if( DisplayOpt.ShowTrackClearanceMode == SHOW_CLEARANCE_ALWAYS )
        GRCircle( &panel->m_ClipBox, DC, m_Start + aOffset, radius + GetClearance(), 0, color );

    // for Micro Vias, draw a partial cross : X on component layer, or + on copper layer
    // (so we can see 2 superimposed microvias ):
    if( GetShape() == VIA_MICROVIA )
    {
        int ax, ay, bx, by;

        if( IsOnLayer( LAYER_N_BACK ) )
        {
            ax = radius; ay = 0;
            bx = drill_radius; by = 0;
        }
        else
        {
            ax = ay = (radius * 707) / 1000;
            bx = by = (drill_radius * 707) / 1000;
        }

        /* lines | or \ */
        GRLine( &panel->m_ClipBox, DC, m_Start.x + aOffset.x - ax,
                m_Start.y + aOffset.y - ay,
                m_Start.x + aOffset.x - bx,
                m_Start.y + aOffset.y - by, 0, color );
        GRLine( &panel->m_ClipBox, DC, m_Start.x + aOffset.x + bx,
                m_Start.y + aOffset.y + by,
                m_Start.x + aOffset.x + ax,
                m_Start.y + aOffset.y + ay, 0, color );

        /* lines - or / */
        GRLine( &panel->m_ClipBox, DC, m_Start.x + aOffset.x + ay,
                m_Start.y + aOffset.y - ax,
                m_Start.x + aOffset.x + by,
                m_Start.y + aOffset.y - bx, 0, color );
        GRLine( &panel->m_ClipBox, DC, m_Start.x + aOffset.x - by,
                m_Start.y + aOffset.y + bx,
                m_Start.x + aOffset.x - ay,
                m_Start.y + aOffset.y + ax, 0, color );
    }

    // for Buried Vias, draw a partial line : orient depending on layer pair
    // (so we can see superimposed buried vias ):
    if( GetShape() == VIA_BLIND_BURIED )
    {
        int ax = 0, ay = radius, bx = 0, by = drill_radius;
        int layer_top, layer_bottom;

        ( (SEGVIA*) this )->ReturnLayerPair( &layer_top, &layer_bottom );

        /* lines for the top layer */
        RotatePoint( &ax, &ay, layer_top * 3600 / brd->GetCopperLayerCount( ) );
        RotatePoint( &bx, &by, layer_top * 3600 / brd->GetCopperLayerCount( ) );
        GRLine( &panel->m_ClipBox, DC, m_Start.x + aOffset.x - ax,
                m_Start.y + aOffset.y - ay,
                m_Start.x + aOffset.x - bx,
                m_Start.y + aOffset.y - by, 0, color );

        /* lines for the bottom layer */
        ax = 0; ay = radius; bx = 0; by = drill_radius;
        RotatePoint( &ax, &ay, layer_bottom * 3600 / brd->GetCopperLayerCount( ) );
        RotatePoint( &bx, &by, layer_bottom * 3600 / brd->GetCopperLayerCount( ) );
        GRLine( &panel->m_ClipBox, DC, m_Start.x + aOffset.x - ax,
                m_Start.y + aOffset.y - ay,
                m_Start.x + aOffset.x - bx,
                m_Start.y + aOffset.y - by, 0, color );
    }

    // Display the short netname:
    if( GetNet() == 0 )
        return;

    if( DisplayOpt.DisplayNetNamesMode == 0 || DisplayOpt.DisplayNetNamesMode == 1 )
        return;

    NETINFO_ITEM* net = ( (BOARD*) GetParent() )->FindNet( GetNet() );

    if( net == NULL )
        return;

    int len = net->GetShortNetname().Len();

    if( len > 0 )
    {
        // calculate a good size for the text
        int tsize = m_Width / len;

        if( DC->LogicalToDeviceXRel( tsize ) >= 6 )
        {
            tsize = (tsize * 8) / 10;        // small reduction to give a better look, inside via
            DrawGraphicText( panel, DC, m_Start,
                             WHITE, net->GetShortNetname(), 0, wxSize( tsize, tsize ),
                             GR_TEXT_HJUSTIFY_CENTER, GR_TEXT_VJUSTIFY_CENTER, tsize / 7,
                             false, false );
        }
    }
}


// see class_track.h
void TRACK::DisplayInfo( EDA_DRAW_FRAME* frame )
{
    wxString msg;
    BOARD*   board = ( (PCB_BASE_FRAME*) frame )->GetBoard();

    // Display basic infos
    DisplayInfoBase( frame );

    // Display full track length (in Pcbnew)
    if( frame->IsType( PCB_FRAME ) )
    {
        int trackLen = 0;
        int lenDie = 0;
        board->MarkTrace( this, NULL, &trackLen, &lenDie, false );
        msg = frame->CoordinateToString( trackLen );
        frame->AppendMsgPanel( _( "Track Len" ), msg, DARKCYAN );

        if( lenDie != 0 )
        {
            msg = frame->CoordinateToString( trackLen + lenDie );
            frame->AppendMsgPanel( _( "Full Len" ), msg, DARKCYAN );

            msg = frame->CoordinateToString( lenDie );
            frame->AppendMsgPanel( _( "On Die" ), msg, DARKCYAN );
        }
    }

    NETCLASS* netclass = GetNetClass();

    if( netclass )
    {
        frame->AppendMsgPanel( _( "NC Name" ), netclass->GetName(), DARKMAGENTA );
        frame->AppendMsgPanel( _( "NC Clearance" ),
                               frame->CoordinateToString( netclass->GetClearance(), true ),
                               DARKMAGENTA );
        frame->AppendMsgPanel( _( "NC Width" ),
                               frame->CoordinateToString( netclass->GetTrackWidth(), true ),
                               DARKMAGENTA );
        frame->AppendMsgPanel( _( "NC Via Size"),
                               frame->CoordinateToString( netclass->GetViaDiameter(), true ),
                               DARKMAGENTA );
        frame->AppendMsgPanel( _( "NC Via Drill"),
                               frame->CoordinateToString( netclass->GetViaDrill(), true ),
                               DARKMAGENTA );
    }
}


void TRACK::DisplayInfoBase( EDA_DRAW_FRAME* frame )
{
    wxString msg;
    BOARD*   board = ( (PCB_BASE_FRAME*) frame )->GetBoard();

    frame->ClearMsgPanel();

    switch( Type() )
    {
    case PCB_VIA_T:
        switch( GetShape() )
        {
            default:
            case 0:
                msg =  _( "??? Via" ); // Not used yet, does not exist currently
                break;

            case 1:
                msg = _( "Micro Via" ); // from external layer (TOP or BOTTOM) from
                                        // the near neighbor inner layer only
                break;

            case 2:
                msg = _( "Blind/Buried Via" );  // from inner or external to inner
                                                // or external layer (no restriction)
                break;

            case 3:
                msg =  _( "Through Via" );  // Usual via (from TOP to BOTTOM layer only )
                break;
        }
        break;

    case PCB_TRACE_T:
        msg = _( "Track" );
        break;

    case PCB_ZONE_T:
        msg = _( "Zone" );
        break;

    default:
        msg = wxT( "????" );
        break;
    }

    frame->AppendMsgPanel( _( "Type" ), msg, DARKCYAN );

    // Display Net Name (in Pcbnew)
    if( frame->IsType( PCB_FRAME ) )
    {
        NETINFO_ITEM* net = board->FindNet( GetNet() );

        if( net )
            msg = net->GetNetname();
        else
            msg = wxT( "<noname>" );

        frame->AppendMsgPanel( _( "NetName" ), msg, RED );

        /* Display net code : (useful in test or debug) */
        msg.Printf( wxT( "%d .%d" ), GetNet(), GetSubNet() );
        frame->AppendMsgPanel( _( "NetCode" ), msg, RED );
    }

#if defined(DEBUG)

    // Display the flags
    msg.Printf( wxT( "0x%08X" ), m_Flags );
    frame->AppendMsgPanel( wxT( "Flags" ), msg, BLUE );

#if 0
    // Display start and end pointers:
    msg.Printf( wxT( "%p" ), start );
    frame->AppendMsgPanel( wxT( "start ptr" ), msg, BLUE );
    msg.Printf( wxT( "%p" ), end );
    frame->AppendMsgPanel( wxT( "end ptr" ), msg, BLUE );
    // Display this ptr
    msg.Printf( wxT( "%p" ), this );
    frame->AppendMsgPanel( wxT( "this" ), msg, BLUE );
#endif

#if 0
    // Display start and end positions:
    msg.Printf( wxT( "%d %d" ), m_Start.x, m_Start.y );
    frame->AppendMsgPanel( wxT( "Start pos" ), msg, BLUE );
    msg.Printf( wxT( "%d %d" ), m_End.x, m_End.y );
    frame->AppendMsgPanel( wxT( "End pos" ), msg, BLUE );
#endif

#endif  // defined(DEBUG)

    /* Display the State member */
    msg = wxT( ". . " );

    if( GetState( TRACK_LOCKED ) )
        msg[0] = 'F';

    if( GetState( TRACK_AR ) )
        msg[2] = 'A';

    frame->AppendMsgPanel( _( "Status" ), msg, MAGENTA );

    /* Display layer or layer pair) */
    if( Type() == PCB_VIA_T )
    {
        SEGVIA* Via = (SEGVIA*) this;
        int     top_layer, bottom_layer;

        Via->ReturnLayerPair( &top_layer, &bottom_layer );
        msg = board->GetLayerName( top_layer ) + wxT( "/" ) + board->GetLayerName( bottom_layer );
    }
    else
    {
        msg = board->GetLayerName( m_Layer );
    }

    frame->AppendMsgPanel( _( "Layer" ), msg, BROWN );

    /* Display width */
    msg = frame->CoordinateToString( (unsigned) m_Width );

    if( Type() == PCB_VIA_T )      // Display Diam and Drill values
    {
        // Display diameter value:
        frame->AppendMsgPanel( _( "Diam" ), msg, DARKCYAN );

        // Display drill value
        int drill_value = GetDrillValue();

        msg = frame->CoordinateToString( (unsigned) drill_value );

        wxString title = _( "Drill" );
        title += wxT( " " );

        if( m_Drill >= 0 )
            title += _( "(Specific)" );
        else
            title += _( "(Default)" );

        frame->AppendMsgPanel( title, msg, RED );
    }
    else
    {
        frame->AppendMsgPanel( _( "Width" ), msg, DARKCYAN );
    }

    // Display segment length
    if( Type() != PCB_VIA_T )      // Display Diam and Drill values
    {
        msg = frame->CoordinateToString( wxRound( GetLength() ) );
        frame->AppendMsgPanel( _( "Segment Length" ), msg, DARKCYAN );
    }
}


bool TRACK::HitTest( const wxPoint& refPos )
{
    int radius = m_Width >> 1;

    // (dx, dy) is a vector from m_Start to m_End (an origin of m_Start)
    int dx = m_End.x - m_Start.x;
    int dy = m_End.y - m_Start.y;

    // (spot_cX, spot_cY) is a vector from m_Start to ref_pos (an origin of m_Start)
    int spot_cX = refPos.x - m_Start.x;
    int spot_cY = refPos.y - m_Start.y;

    if( Type() == PCB_VIA_T )
    {
        return (double) spot_cX * spot_cX + (double) spot_cY * spot_cY <= (double) radius * radius;
    }
    else
    {
        if( DistanceTest( radius, dx, dy, spot_cX, spot_cY ) )
            return true;
    }

    return false;
}


bool TRACK::HitTest( EDA_RECT& refArea )
{
    if( refArea.Contains( m_Start ) )
        return true;

    if( refArea.Contains( m_End ) )
        return true;

    return false;
}


TRACK* TRACK::GetVia( const wxPoint& aPosition, int aLayerMask )
{
    TRACK* track;

    for( track = this;   track;  track = track->Next() )
    {
        if( track->Type() != PCB_VIA_T )
            continue;

        if( !track->HitTest( aPosition ) )
            continue;

        if( track->GetState( BUSY | IS_DELETED ) )
            continue;

        if( aLayerMask < 0 )
            break;

        if( track->IsOnLayer( aLayerMask ) )
            break;
    }

    return track;
}


TRACK* TRACK::GetVia( TRACK* aEndTrace, const wxPoint& aPosition, int aLayerMask )
{
    TRACK* trace;

    for( trace = this; trace != NULL; trace = trace->Next() )
    {
        if( trace->Type() == PCB_VIA_T )
        {
            if( aPosition == trace->m_Start )
            {
                if( trace->GetState( BUSY | IS_DELETED ) == 0 )
                {
                    if( aLayerMask & trace->ReturnMaskLayer() )
                        return trace;
                }
            }
        }

        if( trace == aEndTrace )
            break;
    }

    return NULL;
}


TRACK* TRACK::GetTrace( TRACK* aStartTrace, TRACK* aEndTrace, int aEndPoint )
{
    const int NEIGHTBOUR_COUNT_MAX = 50;

    TRACK*  previousSegment;
    TRACK*  nextSegment;
    int     Reflayer;
    wxPoint position;
    int     ii;
    int     max_dist;

    if( aEndPoint == START )
        position = m_Start;
    else
        position = m_End;

    Reflayer = ReturnMaskLayer();

    previousSegment = nextSegment = this;

    // Local search:
    for( ii = 0; ii < NEIGHTBOUR_COUNT_MAX; ii++ )
    {
        if( (nextSegment == NULL) && (previousSegment == NULL) )
            break;

        if( nextSegment )
        {
            if( nextSegment->GetState( BUSY | IS_DELETED ) )
                goto suite;

            if( nextSegment == this )
                goto suite;

            /* max_dist is the max distance between 2 track ends which
             * ensure a copper continuity */
            max_dist = ( nextSegment->m_Width + this->m_Width ) / 2;

            if( IsNear( position, nextSegment->m_Start, max_dist ) )
            {
                if( Reflayer & nextSegment->ReturnMaskLayer() )
                    return nextSegment;
            }

            if( IsNear( position, nextSegment->m_End, max_dist ) )
            {
                if( Reflayer & nextSegment->ReturnMaskLayer() )
                    return nextSegment;
            }
suite:
            if( nextSegment == aEndTrace )
                nextSegment = NULL;
            else
                nextSegment =  nextSegment->Next();
        }

        if( previousSegment )
        {
            if( previousSegment->GetState( BUSY | IS_DELETED ) )
                goto suite1;

            if( previousSegment == this )
                goto suite1;

            max_dist = ( previousSegment->m_Width + m_Width ) / 2;

            if( IsNear( position, previousSegment->m_Start, max_dist ) )
            {
                if( Reflayer & previousSegment->ReturnMaskLayer() )
                    return previousSegment;
            }

            if( IsNear( position, previousSegment->m_End, max_dist ) )
            {
                if( Reflayer & previousSegment->ReturnMaskLayer() )
                    return previousSegment;
            }
suite1:
            if( previousSegment == aStartTrace )
                previousSegment = NULL;
            else if( previousSegment->Type() != PCB_T )
                previousSegment =  previousSegment->Back();
            else
                previousSegment = NULL;
        }
    }

    /* General search. */
    for( nextSegment = aStartTrace; nextSegment != NULL; nextSegment =  nextSegment->Next() )
    {
        if( nextSegment->GetState( IS_DELETED | BUSY ) )
        {
            if( nextSegment == aEndTrace )
                break;

            continue;
        }

        if( nextSegment == this )
        {
            if( nextSegment == aEndTrace )
                break;

            continue;
        }

        max_dist = ( nextSegment->m_Width + m_Width ) / 2;

        if( IsNear( position, nextSegment->m_Start, max_dist ) )
        {
            if( Reflayer & nextSegment->ReturnMaskLayer() )
                return nextSegment;
        }

        if( IsNear( position, nextSegment->m_End, max_dist ) )
        {
            if( Reflayer & nextSegment->ReturnMaskLayer() )
                return nextSegment;
        }

        if( nextSegment == aEndTrace )
            break;
    }

    return NULL;
}


int TRACK::GetEndSegments( int aCount, TRACK** aStartTrace, TRACK** aEndTrace )
{
    TRACK* Track, * via, * segm, * TrackListEnd;
    int    NbEnds, layerMask, ii, ok = 0;

    if( aCount <= 1 )
    {
        *aStartTrace = *aEndTrace = this;
        return 1;
    }

    /* Calculation of the limit analysis. */
    *aStartTrace = *aEndTrace = NULL;
    TrackListEnd = Track = this;
    ii = 0;

    for( ; ( Track != NULL ) && ( ii < aCount ); ii++, Track = Track->Next() )
    {
        TrackListEnd   = Track;
        Track->m_Param = 0;
    }

    /* Calculate the extremes. */
    NbEnds = 0;
    Track = this;
    ii = 0;

    for( ; ( Track != NULL ) && ( ii < aCount ); ii++, Track = Track->Next() )
    {
        if( Track->Type() == PCB_VIA_T )
            continue;

        layerMask = Track->ReturnMaskLayer();
        via = GetVia( TrackListEnd, Track->m_Start, layerMask );

        if( via )
        {
            layerMask |= via->ReturnMaskLayer();
            via->SetState( BUSY, ON );
        }

        Track->SetState( BUSY, ON );
        segm = ::GetTrace( this, TrackListEnd, Track->m_Start, layerMask );
        Track->SetState( BUSY, OFF );

        if( via )
            via->SetState( BUSY, OFF );

        if( segm == NULL )
        {
            switch( NbEnds )
            {
            case 0:
                *aStartTrace = Track; NbEnds++;
                break;

            case 1:
                int BeginPad, EndPad;
                *aEndTrace = Track;

                /* Swap ox, oy with fx, fy */
                BeginPad = Track->GetState( BEGIN_ONPAD );
                EndPad   = Track->GetState( END_ONPAD );

                Track->SetState( BEGIN_ONPAD | END_ONPAD, OFF );

                if( BeginPad )
                    Track->SetState( END_ONPAD, ON );

                if( EndPad )
                    Track->SetState( BEGIN_ONPAD, ON );

                EXCHG( Track->m_Start, Track->m_End );
                EXCHG( Track->start, Track->end );
                ok = 1;
                return ok;
            }
        }

        layerMask = Track->ReturnMaskLayer();
        via = GetVia( TrackListEnd, Track->m_End, layerMask );

        if( via )
        {
            layerMask |= via->ReturnMaskLayer();
            via->SetState( BUSY, ON );
        }

        Track->SetState( BUSY, ON );
        segm = ::GetTrace( this, TrackListEnd, Track->m_End, layerMask );
        Track->SetState( BUSY, OFF );

        if( via )
            via->SetState( BUSY, OFF );

        if( segm == NULL )
        {
            switch( NbEnds )
            {
            case 0:
                int BeginPad, EndPad;
                *aStartTrace = Track;
                NbEnds++;

                /* Swap ox, oy with fx, fy */
                BeginPad = Track->GetState( BEGIN_ONPAD );
                EndPad   = Track->GetState( END_ONPAD );

                Track->SetState( BEGIN_ONPAD | END_ONPAD, OFF );

                if( BeginPad )
                    Track->SetState( END_ONPAD, ON );

                if( EndPad )
                    Track->SetState( BEGIN_ONPAD, ON );

                EXCHG( Track->m_Start, Track->m_End );
                EXCHG( Track->start, Track->end );
                break;

            case 1:
                *aEndTrace = Track;
                ok = 1;
                return ok;
            }
        }
    }

    return ok;
}


wxString TRACK::GetSelectMenuText() const
{
    wxString text;
    wxString temp;
    NETINFO_ITEM* net;
    BOARD* board = GetBoard();

    // deleting tracks requires all the information we can get to
    // disambiguate all the choices under the cursor!
    text << _( "Track" ) << wxT( " " ) << ShowWidth();

    if( board )
    {
        net = board->FindNet( GetNet() );

        if( net )
            text << wxT( " [" ) << net->GetNetname() << wxT( "]" );
    }
    else
    {
        text << _( "** BOARD NOT DEFINED **" );
    }

    text << _( " on " ) << GetLayerName() << wxT("  ") << _("Net:") << GetNet()
         << wxT("  ") << _("Length:") << valeur_param( GetLength(), temp );

    return text;
}


#if defined(DEBUG)

void TRACK::Show( int nestLevel, std::ostream& os )
{
    NestedSpace( nestLevel, os ) << '<' << GetClass().Lower().mb_str() <<

//        " shape=\""     << m_Shape      << '"' <<
    " addr=\"" << std::hex << this << std::dec << '"' <<
    " layer=\"" << m_Layer << '"' <<
    " width=\"" << m_Width << '"' <<
    " flags=\"" << m_Flags << '"' <<
    " status=\"" << GetState( -1 ) << '"' <<

//        " drill=\""     << GetDrillValue()   << '"' <<
    " netcode=\"" << GetNet() << "\">" <<
    "<start" << m_Start << "/>" <<
    "<end" << m_End << "/>";

    os << "</" << GetClass().Lower().mb_str() << ">\n";
}


void SEGVIA::Show( int nestLevel, std::ostream& os )
{
    const char* cp;

    switch( GetShape() )
    {
    case VIA_THROUGH:
        cp = "through";
        break;

    case VIA_BLIND_BURIED:
        cp = "blind/buried";
        break;

    case VIA_MICROVIA:
        cp = "micro via";
        break;

    default:
    case VIA_NOT_DEFINED:
        cp = "undefined";
        break;
    }

    int    topLayer;
    int    botLayer;
    BOARD* board = (BOARD*) m_Parent;


    ReturnLayerPair( &topLayer, &botLayer );

    NestedSpace( nestLevel, os ) << '<' << GetClass().Lower().mb_str() <<
    " type=\"" << cp << '"';

    if( board )
        os << " layers=\"" << board->GetLayerName( topLayer ).Trim().mb_str() << ","
           << board->GetLayerName( botLayer ).Trim().mb_str() << '"';

    os << " width=\"" << m_Width << '"'
       << " drill=\"" << GetDrillValue() << '"'
       << " netcode=\"" << GetNet() << "\">"
       << "<pos" << m_Start << "/>";

    os << "</" << GetClass().Lower().mb_str() << ">\n";
}


wxString TRACK::ShowState( int stateBits )
{
    wxString ret;

    if( stateBits & IS_LINKED )
        ret << wxT( " | IS_LINKED" );

    if( stateBits & TRACK_AR )
        ret << wxT( " | TRACK_AR" );

    if( stateBits & TRACK_LOCKED )
        ret << wxT( " | TRACK_LOCKED" );

    if( stateBits & IN_EDIT )
        ret << wxT( " | IN_EDIT" );

    if( stateBits & IS_DRAGGED )
        ret << wxT( " | IS_DRAGGED" );

    if( stateBits & DO_NOT_DRAW )
        ret << wxT( " | DO_NOT_DRAW" );

    if( stateBits & IS_DELETED )
        ret << wxT( " | IS_DELETED" );

    if( stateBits & BUSY )
        ret << wxT( " | BUSY" );

    if( stateBits & END_ONPAD )
        ret << wxT( " | END_ONPAD" );

    if( stateBits & BEGIN_ONPAD )
        ret << wxT( " | BEGIN_ONPAD" );

    if( stateBits & FLAG0 )
        ret << wxT( " | FLAG0" );

    if( stateBits & FLAG1 )
        ret << wxT( " | FLAG1" );

    return ret;
}

#endif
