/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 Jean-Pierre Charras, jean-pierre.charras@ujf-grenoble.fr
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2012 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 1992-2012 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <fctsys.h>
#include <gr_basic.h>
#include <common.h>
#include <trigo.h>
#include <macros.h>
#include <class_drawpanel.h>
#include <class_pcb_screen.h>
#include <drawtxt.h>
#include <pcbcommon.h>
#include <colors_selection.h>
#include <wxstruct.h>
#include <wxBasePcbFrame.h>
#include <class_board.h>
#include <class_track.h>
#include <pcbnew.h>
#include <base_units.h>
#include <msgpanel.h>


/**
 * Function ShowClearance
 * tests to see if the clearance border is drawn on the given track.
 * @return bool - true if should draw clearance, else false.
 */
static bool ShowClearance( const TRACK* aTrack )
{
    // maybe return true for tracks and vias, not for zone segments
    return IsCopperLayer( aTrack->GetLayer() )
           && ( aTrack->Type() == PCB_TRACE_T || aTrack->Type() == PCB_VIA_T )
           && ( ( DisplayOpt.ShowTrackClearanceMode == SHOW_CLEARANCE_NEW_AND_EDITED_TRACKS_AND_VIA_AREAS
                  && ( aTrack->IsDragging() || aTrack->IsMoving() || aTrack->IsNew() ) )
            || ( DisplayOpt.ShowTrackClearanceMode == SHOW_CLEARANCE_ALWAYS )
            );

}


/*
 * return true if the dist between p1 and p2 < max_dist
 * Currently in test (currently ratsnest algos work only if p1 == p2)
 */
inline bool IsNear( const wxPoint& p1, const wxPoint& p2, int max_dist )
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


TRACK* GetTrack( TRACK* aStartTrace, const TRACK* aEndTrace,
        const wxPoint& aPosition, LAYER_MSK aLayerMask )
{
    for( TRACK *PtSegm = aStartTrace; PtSegm != NULL; PtSegm = PtSegm->Next() )
    {
        if( PtSegm->GetState( IS_DELETED | BUSY ) == 0 )
        {
            if( aPosition == PtSegm->GetStart() )
            {
                if( aLayerMask & PtSegm->GetLayerMask() )
                    return PtSegm;
            }

            if( aPosition == PtSegm->GetEnd() )
            {
                if( aLayerMask & PtSegm->GetLayerMask() )
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
    m_Width = Millimeter2iu( 0.2 );
    start   = end = NULL;
    m_Param = 0;
}


EDA_ITEM* TRACK::Clone() const
{
    return new TRACK( *this );
}


wxString TRACK::ShowWidth() const
{
    wxString msg = ::CoordinateToString( m_Width );
    return msg;
}


SEGZONE::SEGZONE( BOARD_ITEM* aParent ) :
    TRACK( aParent, PCB_ZONE_T )
{
}


EDA_ITEM* SEGZONE::Clone() const
{
    return new SEGZONE( *this );
}


wxString SEGZONE::GetSelectMenuText() const
{
    wxString text, nettxt;
    BOARD* board = GetBoard();

    if( board )
    {
        nettxt = GetNetname();
    }
    else
    {
        wxFAIL_MSG( wxT( "SEGZONE::GetSelectMenuText: BOARD is NULL" ) );
        nettxt = wxT( "???" );
    }

    text.Printf( _( "Zone (%08lX) [%s] on %s" ),
                 m_TimeStamp, GetChars( nettxt ), GetChars( GetLayerName() ) );

    return text;
}


VIA::VIA( BOARD_ITEM* aParent ) :
    TRACK( aParent, PCB_VIA_T )
{
    SetViaType( VIA_THROUGH );
    m_BottomLayer = LAYER_N_BACK;
    SetDrillDefault();
}


EDA_ITEM* VIA::Clone() const
{
    return new VIA( *this );
}


wxString VIA::GetSelectMenuText() const
{
    wxString text;
    wxString format;
    BOARD* board = GetBoard();

    switch( GetViaType() )
    {
    case VIA_BLIND_BURIED:
        format = _( "Blind/Buried Via %s, net[%s] (%d) on layers %s/%s" );
        break;
    case VIA_MICROVIA:
        format = _( "Micro Via %s, Net [%s] (%d) on layers %s/%s" );
        break;
    // else say nothing about normal (through) vias
    default:
        format = _( "Via %s net [%s] (%d) on layers %s/%s" );
        break;
    }


    if( board )
    {
        wxString netname = GetNetname();

        // say which layers, only two for now
        LAYER_NUM topLayer;
        LAYER_NUM botLayer;
        LayerPair( &topLayer, &botLayer );
        text.Printf( format.GetData(), GetChars( ShowWidth() ),
                     GetChars( netname ), GetNetCode(),
                     GetChars( board->GetLayerName( topLayer ) ),
                     GetChars( board->GetLayerName( botLayer ) ) );

    }
    else
    {
        wxFAIL_MSG( wxT( "VIA::GetSelectMenuText: BOARD is NULL" ) );
        text.Printf( format.GetData(), GetChars( ShowWidth() ),
                     wxT( "???" ), 0,
                     wxT( "??" ), wxT( "??" ) );
    }

    return text;
}


int TRACK::GetClearance( BOARD_CONNECTED_ITEM* aItem ) const
{
    // Currently tracks have no specific clearance parameter on a per track or per
    // segment basis.  The NETCLASS clearance is used.
    return BOARD_CONNECTED_ITEM::GetClearance( aItem );
}


int VIA::GetDrillValue() const
{
    if( m_Drill > 0 )      // Use the specific value.
        return m_Drill;

    // Use the default value from the Netclass
    NETCLASS* netclass = GetNetClass();

    if( GetViaType() == VIA_MICROVIA )
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


STATUS_FLAGS TRACK::IsPointOnEnds( const wxPoint& point, int min_dist )
{
    STATUS_FLAGS result = 0;

    if( min_dist < 0 )
        min_dist = m_Width / 2;

    if( min_dist == 0 )
    {
        if( m_Start == point  )
            result |= STARTPOINT;

        if( m_End == point )
            result |= ENDPOINT;
    }
    else
    {
        double dist = GetLineLength( m_Start, point );

        if( min_dist >= KiROUND( dist ) )
            result |= STARTPOINT;

        dist = GetLineLength( m_End, point );

        if( min_dist >= KiROUND( dist ) )
            result |= ENDPOINT;
    }

    return result;
}


const EDA_RECT TRACK::GetBoundingBox() const
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

        ymax = std::max( m_Start.y, m_End.y );
        xmax = std::max( m_Start.x, m_End.x );

        ymin = std::min( m_Start.y, m_End.y );
        xmin = std::min( m_Start.x, m_End.x );
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
    SetLayer( FlipLayer( GetLayer() ) );
}


void VIA::Flip( const wxPoint& aCentre )
{
    m_Start.y = aCentre.y - (m_Start.y - aCentre.y);
    m_End.y   = aCentre.y - (m_End.y - aCentre.y);
}


// see class_track.h
SEARCH_RESULT TRACK::Visit( INSPECTOR* inspector, const void* testData,
                            const KICAD_T scanTypes[] )
{
    KICAD_T stype = *scanTypes;

    // If caller wants to inspect my type
    if( stype == Type() )
    {
        if( SEARCH_QUIT == inspector->Inspect( this, testData ) )
            return SEARCH_QUIT;
    }

    return SEARCH_CONTINUE;
}


bool VIA::IsOnLayer( LAYER_NUM layer_number ) const
{
    LAYER_NUM bottom_layer, top_layer;

    LayerPair( &top_layer, &bottom_layer );

    if( bottom_layer <= layer_number && layer_number <= top_layer )
        return true;
    else
        return false;
}

LAYER_MSK VIA::GetLayerMask() const
{
    if( GetViaType() == VIA_THROUGH )
        return ALL_CU_LAYERS;

    // VIA_BLIND_BURIED or VIA_MICRVIA:

    LAYER_NUM bottom_layer, top_layer;

    // LayerPair() knows how layers are stored
    LayerPair( &top_layer, &bottom_layer );

    LAYER_MSK layermask = NO_LAYERS;

    while( bottom_layer <= top_layer )
    {
        layermask |= ::GetLayerMask( bottom_layer );
        ++bottom_layer;
    }

    return layermask;
}

LAYER_MSK TRACK::GetLayerMask() const
{
    return ::GetLayerMask( m_Layer );
}


void VIA::SetLayerPair( LAYER_NUM aTopLayer, LAYER_NUM aBottomLayer )
{
    if( GetViaType() == VIA_THROUGH )
    {
        aTopLayer    = LAYER_N_FRONT;
        aBottomLayer = LAYER_N_BACK;
    }

    if( aBottomLayer > aTopLayer )
        EXCHG( aBottomLayer, aTopLayer );

    m_Layer = aTopLayer;
    m_BottomLayer = aBottomLayer;
}


void VIA::LayerPair( LAYER_NUM* top_layer, LAYER_NUM* bottom_layer ) const
{
    LAYER_NUM b_layer = LAYER_N_BACK;
    LAYER_NUM t_layer = LAYER_N_FRONT;

    if( GetViaType() != VIA_THROUGH )
    {
        b_layer = m_BottomLayer;
        t_layer = m_Layer;

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
        if( GetNetCode() <= track->GetNetCode() )
            return track;
    }

    return NULL;
}


TRACK* TRACK::GetStartNetCode( int NetCode )
{
    TRACK* Track = this;
    int    ii    = 0;

    if( NetCode == -1 )
        NetCode = GetNetCode();

    while( Track != NULL )
    {
        if( Track->GetNetCode() > NetCode )
            break;

        if( Track->GetNetCode() == NetCode )
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
        NetCode = GetNetCode();

    while( Track != NULL )
    {
        NextS = (TRACK*) Track->Pnext;

        if( Track->GetNetCode() == NetCode )
            ii++;

        if( NextS == NULL )
            break;

        if( NextS->GetNetCode() > NetCode )
            break;

        Track = NextS;
    }

    if( ii )
        return Track;
    else
        return NULL;
}

void TRACK::DrawShortNetname( EDA_DRAW_PANEL* panel,
        wxDC* aDC, GR_DRAWMODE aDrawMode, EDA_COLOR_T aBgColor )
{
    /* we must filter tracks, to avoid a lot of texts.
     *  - only tracks with a length > 10 * thickness are eligible
     * and, of course, if we are not printing the board
     */
    if( DisplayOpt.DisplayNetNamesMode == 0 || DisplayOpt.DisplayNetNamesMode == 1 )
        return;

    #define THRESHOLD 10

    int len = KiROUND( GetLineLength( m_Start, m_End ) );

    if( len < THRESHOLD * m_Width )
        return;

    // no room to display a text inside track
    if( aDC->LogicalToDeviceXRel( m_Width ) < MIN_TEXT_SIZE )
        return;

    if( GetNetCode() == NETINFO_LIST::UNCONNECTED )
        return;

    NETINFO_ITEM* net = GetNet();

    if( net == NULL )
        return;

    int textlen = net->GetShortNetname().Len();

    if( textlen > 0 )
    {
        // calculate a good size for the text
        int     tsize = std::min( m_Width, len / textlen );
        int     dx = m_End.x - m_Start.x ;
        int     dy = m_End.y - m_Start.y ;
        wxPoint tpos  = m_Start + m_End;
        tpos.x /= 2;
        tpos.y /= 2;

        // Calculate angle: if the track segment is vertical, angle = 90 degrees
        // If horizontal 0 degrees, otherwise compute it
        double angle;        // angle is in 0.1 degree

        if( dy == 0 )        // Horizontal segment
        {
            angle = 0;
        }
        else
        {
            if( dx == 0 )    // Vertical segment
            {
                angle = 900;
            }
            else
            {
                /* atan2 is *not* the solution here, since it can give upside
                   down text. We want to work only in the first and fourth quadrant */
                angle = RAD2DECIDEG( -atan( double( dy ) / double( dx ) ) );
            }
        }

        LAYER_NUM curr_layer = ( (PCB_SCREEN*) panel->GetScreen() )->m_Active_Layer;
        if( ( aDC->LogicalToDeviceXRel( tsize ) >= MIN_TEXT_SIZE )
         && ( !(!IsOnLayer( curr_layer )&& DisplayOpt.ContrastModeDisplay) ) )
        {
            if( (aDrawMode & GR_XOR) == 0 )
                GRSetDrawMode( aDC, GR_COPY );

            tsize = (tsize * 7) / 10;       // small reduction to give a better look
            EDA_RECT* clipbox = panel? panel->GetClipBox() : NULL;
            DrawGraphicHaloText( clipbox, aDC, tpos,
                                 aBgColor, BLACK, WHITE, net->GetShortNetname(), angle,
                                 wxSize( tsize, tsize ),
                                 GR_TEXT_HJUSTIFY_CENTER, GR_TEXT_VJUSTIFY_CENTER,
                                 tsize / 7,
                                 false, false );
        }
    }
}

void TRACK::Draw( EDA_DRAW_PANEL* panel, wxDC* aDC, GR_DRAWMODE aDrawMode,
                  const wxPoint& aOffset )
{
    BOARD * brd = GetBoard( );
    EDA_COLOR_T color = brd->GetLayerColor(m_Layer);

    if( brd->IsLayerVisible( m_Layer ) == false && !( aDrawMode & GR_HIGHLIGHT ) )
        return;

#ifdef USE_WX_OVERLAY
    // If dragged not draw in OnPaint otherwise remains impressed in wxOverlay
    if( (m_Flags && IS_DRAGGED) && aDC->IsKindOf(wxCLASSINFO(wxPaintDC)))
      return;
#endif

    if( ( aDrawMode & GR_ALLOW_HIGHCONTRAST ) && DisplayOpt.ContrastModeDisplay )
    {
        LAYER_NUM curr_layer = ( (PCB_SCREEN*) panel->GetScreen() )->m_Active_Layer;

        if( !IsOnLayer( curr_layer ) )
            ColorTurnToDarkDarkGray( &color );
    }

    if( aDrawMode & GR_HIGHLIGHT )
        ColorChangeHighlightFlag( &color, !(aDrawMode & GR_AND) );

    ColorApplyHighlightFlag( &color );

    SetAlpha( &color, 150 );

    GRSetDrawMode( aDC, aDrawMode );

    int l_trace = m_Width / 2;

    if( aDC->LogicalToDeviceXRel( l_trace ) <= MIN_DRAW_WIDTH )
    {
        GRLine( panel->GetClipBox(), aDC, m_Start + aOffset, m_End + aOffset, 0, color );
        return;
    }

    if( !DisplayOpt.DisplayPcbTrackFill || GetState( FORCE_SKETCH ) )
    {
        GRCSegm( panel->GetClipBox(), aDC, m_Start + aOffset, m_End + aOffset, m_Width, color );
    }
    else
    {
        GRFillCSegm( panel->GetClipBox(), aDC, m_Start.x + aOffset.x,
                     m_Start.y + aOffset.y,
                     m_End.x + aOffset.x, m_End.y + aOffset.y, m_Width, color );
    }

    if( panel->GetScreen()->m_IsPrinting )
        return;

    // Show clearance for tracks, not for zone segments
    if( ShowClearance( this ) )
    {
        GRCSegm( panel->GetClipBox(), aDC, m_Start + aOffset, m_End + aOffset,
                 m_Width + (GetClearance() * 2), color );
    }

    DrawShortNetname( panel, aDC, aDrawMode, color );
}

void SEGZONE::Draw( EDA_DRAW_PANEL* panel, wxDC* aDC, GR_DRAWMODE aDrawMode,
                    const wxPoint& aOffset )
{
    if( DisplayOpt.DisplayZonesMode != 0 )
        return;

    BOARD * brd = GetBoard( );
    EDA_COLOR_T color = brd->GetLayerColor(m_Layer);

    if( brd->IsLayerVisible( m_Layer ) == false && !( aDrawMode & GR_HIGHLIGHT ) )
        return;

#ifdef USE_WX_OVERLAY
    // If dragged not draw in OnPaint otherwise remains impressed in wxOverlay
    if( (m_Flags && IS_DRAGGED) && aDC->IsKindOf(wxCLASSINFO(wxPaintDC)))
      return;
#endif

    if( ( aDrawMode & GR_ALLOW_HIGHCONTRAST ) && DisplayOpt.ContrastModeDisplay )
    {
        LAYER_NUM curr_layer = ( (PCB_SCREEN*) panel->GetScreen() )->m_Active_Layer;

        if( !IsOnLayer( curr_layer ) )
            ColorTurnToDarkDarkGray( &color );
    }

    if( aDrawMode & GR_HIGHLIGHT )
        ColorChangeHighlightFlag( &color, !(aDrawMode & GR_AND) );

    ColorApplyHighlightFlag( &color );

    SetAlpha( &color, 150 );

    GRSetDrawMode( aDC, aDrawMode );

    int l_trace = m_Width / 2;

    if( aDC->LogicalToDeviceXRel( l_trace ) <= MIN_DRAW_WIDTH )
    {
        GRLine( panel->GetClipBox(), aDC, m_Start + aOffset, m_End + aOffset, 0, color );
        return;
    }

    if( !DisplayOpt.DisplayPcbTrackFill || GetState( FORCE_SKETCH ) )
    {
        GRCSegm( panel->GetClipBox(), aDC, m_Start + aOffset, m_End + aOffset, m_Width, color );
    }
    else
    {
        GRFillCSegm( panel->GetClipBox(), aDC, m_Start.x + aOffset.x,
                     m_Start.y + aOffset.y,
                     m_End.x + aOffset.x, m_End.y + aOffset.y, m_Width, color );
    }

    // No clearance or netnames for zones
}


void TRACK::ViewGetLayers( int aLayers[], int& aCount ) const
{
    // Show the track and its netname on different layers
    aLayers[0] = GetLayer();
    aLayers[1] = GetNetnameLayer( aLayers[0] );
    aCount = 2;
}


unsigned int TRACK::ViewGetLOD( int aLayer ) const
{
    // Netnames will be shown only if zoom is appropriate
    if( IsNetnameLayer( aLayer ) )
    {
        return ( 20000000 / ( m_Width + 1 ) );
    }

    // Other layers are shown without any conditions
    return 0;
}


void VIA::Draw( EDA_DRAW_PANEL* panel, wxDC* aDC, GR_DRAWMODE aDrawMode,
                   const wxPoint& aOffset )
{
    int radius;
    LAYER_NUM curr_layer = ( (PCB_SCREEN*) panel->GetScreen() )->m_Active_Layer;

    int fillvia = 0;
    PCB_BASE_FRAME* frame  = (PCB_BASE_FRAME*) panel->GetParent();
    PCB_SCREEN*     screen = frame->GetScreen();

    if( frame->m_DisplayViaFill == FILLED )
        fillvia = 1;

    GRSetDrawMode( aDC, aDrawMode );

    BOARD * brd =  GetBoard( );
    EDA_COLOR_T color = brd->GetVisibleElementColor(VIAS_VISIBLE + GetViaType());

    if( brd->IsElementVisible( PCB_VISIBLE(VIAS_VISIBLE + GetViaType()) ) == false
        && ( color & HIGHLIGHT_FLAG ) != HIGHLIGHT_FLAG )
        return;

    if( DisplayOpt.ContrastModeDisplay )
    {
        if( !IsOnLayer( curr_layer ) )
            ColorTurnToDarkDarkGray( &color );
    }

    if( aDrawMode & GR_HIGHLIGHT )
        ColorChangeHighlightFlag( &color, !(aDrawMode & GR_AND) );

    ColorApplyHighlightFlag( &color );

    SetAlpha( &color, 150 );


    radius = m_Width >> 1;
    // for small via size on screen (radius < 4 pixels) draw a simplified shape

    int radius_in_pixels = aDC->LogicalToDeviceXRel( radius );

    bool fast_draw = false;

    // Vias are drawn as a filled circle or a double circle. The hole will be drawn later
    int drill_radius = GetDrillValue() / 2;

    int inner_radius = radius - aDC->DeviceToLogicalXRel( 2 );

    if( radius_in_pixels < MIN_VIA_DRAW_SIZE )
    {
        fast_draw = true;
        fillvia = false;
    }

    if( fillvia )
    {
        GRFilledCircle( panel->GetClipBox(), aDC, m_Start + aOffset, radius, color );
    }
    else
    {
        GRCircle( panel->GetClipBox(), aDC, m_Start + aOffset, radius, 0, color );

        if ( fast_draw )
            return;

        GRCircle( panel->GetClipBox(), aDC, m_Start + aOffset, inner_radius, 0, color );
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

                if( (aDrawMode & GR_XOR) == 0)
                    GRSetDrawMode( aDC, GR_COPY );

                if( aDC->LogicalToDeviceXRel( drill_radius ) > MIN_DRAW_WIDTH )  // Draw hole if large enough.
                    GRFilledCircle( panel->GetClipBox(), aDC, m_Start.x + aOffset.x,
                                    m_Start.y + aOffset.y, drill_radius, 0, color, color );

                if( screen->m_IsPrinting )
                    GRForceBlackPen( blackpenstate );
            }
            else
            {
                if( drill_radius < inner_radius )         // We can show the via hole
                    GRCircle( panel->GetClipBox(), aDC, m_Start + aOffset, drill_radius, 0, color );
            }
        }
    }

    if( ShowClearance( this ) )
    {
        GRCircle( panel->GetClipBox(), aDC, m_Start + aOffset, radius + GetClearance(), 0, color );
    }

    // for Micro Vias, draw a partial cross : X on component layer, or + on copper layer
    // (so we can see 2 superimposed microvias ):
    if( GetViaType() == VIA_MICROVIA )
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
        GRLine( panel->GetClipBox(), aDC, m_Start.x + aOffset.x - ax,
                m_Start.y + aOffset.y - ay,
                m_Start.x + aOffset.x - bx,
                m_Start.y + aOffset.y - by, 0, color );
        GRLine( panel->GetClipBox(), aDC, m_Start.x + aOffset.x + bx,
                m_Start.y + aOffset.y + by,
                m_Start.x + aOffset.x + ax,
                m_Start.y + aOffset.y + ay, 0, color );

        // lines - or /
        GRLine( panel->GetClipBox(), aDC, m_Start.x + aOffset.x + ay,
                m_Start.y + aOffset.y - ax,
                m_Start.x + aOffset.x + by,
                m_Start.y + aOffset.y - bx, 0, color );
        GRLine( panel->GetClipBox(), aDC, m_Start.x + aOffset.x - by,
                m_Start.y + aOffset.y + bx,
                m_Start.x + aOffset.x - ay,
                m_Start.y + aOffset.y + ax, 0, color );
    }

    // for Buried Vias, draw a partial line : orient depending on layer pair
    // (so we can see superimposed buried vias ):
    if( GetViaType() == VIA_BLIND_BURIED )
    {
        int ax = 0, ay = radius, bx = 0, by = drill_radius;
        LAYER_NUM layer_top, layer_bottom;

        ( (VIA*) this )->LayerPair( &layer_top, &layer_bottom );

        // lines for the top layer
        RotatePoint( &ax, &ay, layer_top * 3600.0 / brd->GetCopperLayerCount( ) );
        RotatePoint( &bx, &by, layer_top * 3600.0 / brd->GetCopperLayerCount( ) );
        GRLine( panel->GetClipBox(), aDC, m_Start.x + aOffset.x - ax,
                m_Start.y + aOffset.y - ay,
                m_Start.x + aOffset.x - bx,
                m_Start.y + aOffset.y - by, 0, color );

        // lines for the bottom layer
        ax = 0; ay = radius; bx = 0; by = drill_radius;
        RotatePoint( &ax, &ay, layer_bottom * 3600.0 / brd->GetCopperLayerCount( ) );
        RotatePoint( &bx, &by, layer_bottom * 3600.0 / brd->GetCopperLayerCount( ) );
        GRLine( panel->GetClipBox(), aDC, m_Start.x + aOffset.x - ax,
                m_Start.y + aOffset.y - ay,
                m_Start.x + aOffset.x - bx,
                m_Start.y + aOffset.y - by, 0, color );
    }

    // Display the short netname:
    if( GetNetCode() == NETINFO_LIST::UNCONNECTED )
        return;

    if( DisplayOpt.DisplayNetNamesMode == 0 || DisplayOpt.DisplayNetNamesMode == 1 )
        return;

    NETINFO_ITEM* net = GetNet();

    if( net == NULL )
        return;

    int len = net->GetShortNetname().Len();

    if( len > 0 )
    {
        // calculate a good size for the text
        int tsize = m_Width / len;

        if( aDC->LogicalToDeviceXRel( tsize ) >= MIN_TEXT_SIZE )
        {
            tsize = (tsize * 7) / 10;        // small reduction to give a better look, inside via
            if( (aDrawMode & GR_XOR) == 0 )
                GRSetDrawMode( aDC, GR_COPY );

            EDA_RECT* clipbox = panel? panel->GetClipBox() : NULL;
            DrawGraphicHaloText( clipbox, aDC, m_Start,
                                 color, WHITE, BLACK, net->GetShortNetname(), 0,
                                 wxSize( tsize, tsize ),
                                 GR_TEXT_HJUSTIFY_CENTER, GR_TEXT_VJUSTIFY_CENTER,
                                 tsize / 7, false, false );
        }
    }
}


void VIA::ViewGetLayers( int aLayers[], int& aCount ) const
{
    // Just show it on common via & via holes layers
    aLayers[0] = ITEM_GAL_LAYER( VIA_THROUGH_VISIBLE );
    aLayers[1] = ITEM_GAL_LAYER( VIAS_HOLES_VISIBLE );
    aCount = 2;
}


// see class_track.h
void TRACK::GetMsgPanelInfo( std::vector< MSG_PANEL_ITEM >& aList )
{
    wxString msg;
    BOARD*   board = GetBoard();

    // Display basic infos
    GetMsgPanelInfoBase( aList );

    // Display full track length (in Pcbnew)
    if( board )
    {
        double trackLen = 0;
        double lenPadToDie = 0;
        board->MarkTrace( this, NULL, &trackLen, &lenPadToDie, false );
        msg = ::CoordinateToString( trackLen );
        aList.push_back( MSG_PANEL_ITEM( _( "Track Len" ), msg, DARKCYAN ) );

        if( lenPadToDie != 0 )
        {
            msg = ::LengthDoubleToString( trackLen + lenPadToDie );
            aList.push_back( MSG_PANEL_ITEM( _( "Full Len" ), msg, DARKCYAN ) );

            msg = ::LengthDoubleToString( lenPadToDie );
            aList.push_back( MSG_PANEL_ITEM( _( "In Package" ), msg, DARKCYAN ) );
        }
    }

    NETCLASS* netclass = GetNetClass();

    if( netclass )
    {
        aList.push_back( MSG_PANEL_ITEM( _( "NC Name" ), netclass->GetName(), DARKMAGENTA ) );
        aList.push_back( MSG_PANEL_ITEM( _( "NC Clearance" ),
                                         ::CoordinateToString( netclass->GetClearance(), true ),
                                         DARKMAGENTA ) );
        aList.push_back( MSG_PANEL_ITEM( _( "NC Width" ),
                                         ::CoordinateToString( netclass->GetTrackWidth(), true ),
                                         DARKMAGENTA ) );
        aList.push_back( MSG_PANEL_ITEM( _( "NC Via Size" ),
                                         ::CoordinateToString( netclass->GetViaDiameter(), true ),
                                         DARKMAGENTA ) );
        aList.push_back( MSG_PANEL_ITEM( _( "NC Via Drill"),
                                         ::CoordinateToString( netclass->GetViaDrill(), true ),
                                         DARKMAGENTA ) );
    }
}

void TRACK::GetMsgPanelInfoBase_Common( std::vector< MSG_PANEL_ITEM >& aList )
{
    wxString msg;

    // Display Net Name
    if( GetBoard() )
    {
        NETINFO_ITEM* net = GetNet();

        if( net )
            msg = net->GetNetname();
        else
            msg = wxT( "<noname>" );

        aList.push_back( MSG_PANEL_ITEM( _( "NetName" ), msg, RED ) );

        // Display net code : (useful in test or debug)
        msg.Printf( wxT( "%d.%d" ), GetNetCode(), GetSubNet() );
        aList.push_back( MSG_PANEL_ITEM( _( "NetCode" ), msg, RED ) );
    }

#if defined(DEBUG)

    // Display the flags
    msg.Printf( wxT( "0x%08X" ), m_Flags );
    aList.push_back( MSG_PANEL_ITEM( wxT( "Flags" ), msg, BLUE ) );

#if 0
    // Display start and end pointers:
    msg.Printf( wxT( "%p" ), start );
    aList.push_back( MSG_PANEL_ITEM( wxT( "start ptr" ), msg, BLUE ) );
    msg.Printf( wxT( "%p" ), end );
    aList.push_back( MSG_PANEL_ITEM( wxT( "end ptr" ), msg, BLUE ) );
    // Display this ptr
    msg.Printf( wxT( "%p" ), this );
    aList.push_back( MSG_PANEL_ITEM( wxT( "this" ), msg, BLUE ) );
#endif

#if 0
    // Display start and end positions:
    msg.Printf( wxT( "%d %d" ), m_Start.x, m_Start.y );
    aList.push_back( MSG_PANEL_ITEM( wxT( "Start pos" ), msg, BLUE ) );
    msg.Printf( wxT( "%d %d" ), m_End.x, m_End.y );
    aList.push_back( MSG_PANEL_ITEM( wxT( "End pos" ), msg, BLUE ) );
#endif

#endif  // defined(DEBUG)

    // Display the State member
    msg = wxT( ". . " );

    if( GetState( TRACK_LOCKED ) )
        msg[0] = 'F';

    if( GetState( TRACK_AR ) )
        msg[2] = 'A';

    aList.push_back( MSG_PANEL_ITEM( _( "Status" ), msg, MAGENTA ) );
}

void TRACK::GetMsgPanelInfoBase( std::vector< MSG_PANEL_ITEM >& aList )
{
    wxString msg;
    BOARD* board = GetBoard();

    aList.push_back( MSG_PANEL_ITEM( _( "Type" ), _( "Track" ), DARKCYAN ) );

    GetMsgPanelInfoBase_Common( aList );

    // Display layer
    if( board )
        msg = board->GetLayerName( m_Layer );
    else
        msg.Printf(wxT("%d"), m_Layer );

    aList.push_back( MSG_PANEL_ITEM( _( "Layer" ), msg, BROWN ) );

    // Display width
    msg = ::CoordinateToString( (unsigned) m_Width );

    aList.push_back( MSG_PANEL_ITEM( _( "Width" ), msg, DARKCYAN ) );

    // Display segment length
    msg = ::LengthDoubleToString( GetLength() );
    aList.push_back( MSG_PANEL_ITEM( _( "Segment Length" ), msg, DARKCYAN ) );
}

void SEGZONE::GetMsgPanelInfoBase( std::vector< MSG_PANEL_ITEM >& aList )
{
    wxString msg;
    BOARD* board = GetBoard();

    aList.push_back( MSG_PANEL_ITEM( _( "Type" ), _( "Zone " ), DARKCYAN ) );

    GetMsgPanelInfoBase_Common( aList );

    // Display layer
    if( board )
        msg = board->GetLayerName( m_Layer );
    else
        msg.Printf(wxT("%d"), m_Layer );

    aList.push_back( MSG_PANEL_ITEM( _( "Layer" ), msg, BROWN ) );

    // Display width
    msg = ::CoordinateToString( (unsigned) m_Width );

    aList.push_back( MSG_PANEL_ITEM( _( "Width" ), msg, DARKCYAN ) );

    // Display segment length
    msg = ::LengthDoubleToString( GetLength() );
    aList.push_back( MSG_PANEL_ITEM( _( "Segment Length" ), msg, DARKCYAN ) );
}

void VIA::GetMsgPanelInfoBase( std::vector< MSG_PANEL_ITEM >& aList )
{
    wxString msg;
    BOARD*   board = GetBoard();

    switch( GetViaType() )
    {
    default:
    case VIA_NOT_DEFINED:
        msg =  wxT( "???" ); // Not used yet, does not exist currently
        break;

    case VIA_MICROVIA:
        msg = _( "Micro Via" ); // from external layer (TOP or BOTTOM) from
                                    // the near neighbor inner layer only
        break;

    case VIA_BLIND_BURIED:
        msg = _( "Blind/Buried Via" );  // from inner or external to inner
                                            // or external layer (no restriction)
        break;

    case VIA_THROUGH:
        msg =  _( "Through Via" );  // Usual via (from TOP to BOTTOM layer only )
        break;
    }

    aList.push_back( MSG_PANEL_ITEM( _( "Type" ), msg, DARKCYAN ) );

    GetMsgPanelInfoBase_Common( aList );


    // Display layer pair
    LAYER_NUM top_layer, bottom_layer;

    LayerPair( &top_layer, &bottom_layer );
    if( board )
        msg = board->GetLayerName( top_layer ) + wxT( "/" )
            + board->GetLayerName( bottom_layer );
    else
        msg.Printf(wxT("%d/%d"), top_layer, bottom_layer );

    aList.push_back( MSG_PANEL_ITEM( _( "Layers" ), msg, BROWN ) );

    // Display width
    msg = ::CoordinateToString( (unsigned) m_Width );

    // Display diameter value:
    aList.push_back( MSG_PANEL_ITEM( _( "Diam" ), msg, DARKCYAN ) );

    // Display drill value
    int drill_value = GetDrillValue();

    msg = ::CoordinateToString( drill_value );

    wxString title = _( "Drill" );
    title += wxT( " " );

    if( m_Drill >= 0 )
        title += _( "(Specific)" );
    else
        title += _( "(Default)" );

    aList.push_back( MSG_PANEL_ITEM( title, msg, RED ) );
}


bool TRACK::HitTest( const wxPoint& aPosition ) const
{
    return TestSegmentHit( aPosition, m_Start, m_End, m_Width / 2 );
}

bool VIA::HitTest( const wxPoint& aPosition ) const
{
    int max_dist = m_Width / 2;

    // rel_pos is aPosition relative to m_Start (or the center of the via)
    wxPoint rel_pos = aPosition - m_Start;
    double dist = (double) rel_pos.x * rel_pos.x + (double) rel_pos.y * rel_pos.y;
    return  dist <= (double) max_dist * max_dist;
}


bool TRACK::HitTest( const EDA_RECT& aRect, bool aContained, int aAccuracy ) const
{
    EDA_RECT arect = aRect;
    arect.Inflate( aAccuracy );

    if( aContained )
        /* Tracks are a special case:
         * they are considered inside the rect if one end is inside the rect */
        return arect.Contains( GetStart() ) || arect.Contains( GetEnd() );
    else
        return arect.Intersects( GetStart(), GetEnd() );
}

bool VIA::HitTest( const EDA_RECT& aRect, bool aContained, int aAccuracy ) const
{
    EDA_RECT box;
    EDA_RECT arect = aRect;
    arect.Inflate( aAccuracy );

    box.SetOrigin( GetStart() );
    box.Inflate( GetWidth() / 2 );

    if( aContained )
        return arect.Contains( box );
    else
        return arect.Intersects( box );
}

VIA* TRACK::GetVia( const wxPoint& aPosition, LAYER_NUM aLayer)
{
    for( VIA *via = GetFirstVia( this ); via; via = GetFirstVia( via->Next() ) )
    {
        if( via->HitTest( aPosition ) &&
                !via->GetState( BUSY | IS_DELETED ) &&
                ((aLayer == UNDEFINED_LAYER) || (via->IsOnLayer( aLayer ))) )
            return via;
    }

    return NULL;
}


VIA* TRACK::GetVia( TRACK* aEndTrace, const wxPoint& aPosition, LAYER_MSK aLayerMask )
{
    for( VIA *via = GetFirstVia( this, aEndTrace ); via; via = GetFirstVia( via->Next() ) )
    {
        if( via->HitTest( aPosition ) &&
                !via->GetState( BUSY | IS_DELETED ) &&
                (aLayerMask & via->GetLayerMask()) )
            return via;
    }

    return NULL;
}


TRACK* TRACK::GetTrack( TRACK* aStartTrace, TRACK* aEndTrace, ENDPOINT_T aEndPoint )
{
    const int NEIGHTBOUR_COUNT_MAX = 50;

    TRACK*  previousSegment;
    TRACK*  nextSegment;
    int     Reflayer;
    int     ii;
    int     max_dist;
    const wxPoint &position = GetEndPoint( aEndPoint );

    Reflayer = GetLayerMask();

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
                if( Reflayer & nextSegment->GetLayerMask() )
                    return nextSegment;
            }

            if( IsNear( position, nextSegment->m_End, max_dist ) )
            {
                if( Reflayer & nextSegment->GetLayerMask() )
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
                if( Reflayer & previousSegment->GetLayerMask() )
                    return previousSegment;
            }

            if( IsNear( position, previousSegment->m_End, max_dist ) )
            {
                if( Reflayer & previousSegment->GetLayerMask() )
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

    // General search
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
            if( Reflayer & nextSegment->GetLayerMask() )
                return nextSegment;
        }

        if( IsNear( position, nextSegment->m_End, max_dist ) )
        {
            if( Reflayer & nextSegment->GetLayerMask() )
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
    int      NbEnds, ii, ok = 0;
    LAYER_MSK layerMask;

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

        layerMask = Track->GetLayerMask();
        via = GetVia( TrackListEnd, Track->m_Start, layerMask );

        if( via )
        {
            layerMask |= via->GetLayerMask();
            via->SetState( BUSY, true );
        }

        Track->SetState( BUSY, true );
        segm = ::GetTrack( this, TrackListEnd, Track->m_Start, layerMask );
        Track->SetState( BUSY, false );

        if( via )
            via->SetState( BUSY, false );

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

                Track->SetState( BEGIN_ONPAD | END_ONPAD, false );

                if( BeginPad )
                    Track->SetState( END_ONPAD, true );

                if( EndPad )
                    Track->SetState( BEGIN_ONPAD, true );

                EXCHG( Track->m_Start, Track->m_End );
                EXCHG( Track->start, Track->end );
                ok = 1;
                return ok;
            }
        }

        layerMask = Track->GetLayerMask();
        via = GetVia( TrackListEnd, Track->m_End, layerMask );

        if( via )
        {
            layerMask |= via->GetLayerMask();
            via->SetState( BUSY, true );
        }

        Track->SetState( BUSY, true );
        segm = ::GetTrack( this, TrackListEnd, Track->m_End, layerMask );
        Track->SetState( BUSY, false );

        if( via )
            via->SetState( BUSY, false );

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

                Track->SetState( BEGIN_ONPAD | END_ONPAD, false );

                if( BeginPad )
                    Track->SetState( END_ONPAD, true );

                if( EndPad )
                    Track->SetState( BEGIN_ONPAD, true );

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
    wxString netname;
    NETINFO_ITEM* net;
    BOARD* board = GetBoard();

    // deleting tracks requires all the information we can get to
    // disambiguate all the choices under the cursor!
    if( board )
    {
        net = GetNet();

        if( net )
            netname = net->GetNetname();
        else
            netname = _("Not found");
    }
    else
    {
        wxFAIL_MSG( wxT( "TRACK::GetSelectMenuText: BOARD is NULL" ) );
        netname = wxT( "???" );
    }

    text.Printf( _("Track %s, net [%s] (%d) on layer %s, length: %s" ),
                 GetChars( ShowWidth() ), GetChars( netname ),
                 GetNetCode(), GetChars( GetLayerName() ),
                 GetChars( ::LengthDoubleToString( GetLength() ) ) );

    return text;
}


#if defined(DEBUG)

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
