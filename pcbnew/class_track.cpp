/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 Jean-Pierre Charras, jean-pierre.charras@ujf-grenoble.fr
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2012 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 1992-2016 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <gr_basic.h>
#include <common.h>
#include <trigo.h>
#include <macros.h>
#include <pcb_screen.h>
#include <gr_text.h>
#include <pcb_base_frame.h>
#include <class_board.h>
#include <class_track.h>
#include <pcbnew.h>
#include <base_units.h>
#include <msgpanel.h>
#include <bitmaps.h>
#include <view/view.h>

/**
 * Function ShowClearance
 * tests to see if the clearance border is drawn on the given track.
 * @return bool - true if should draw clearance, else false.
 */
static bool ShowClearance( PCB_DISPLAY_OPTIONS* aDisplOpts, const TRACK* aTrack )
{
    // maybe return true for tracks and vias, not for zone segments
    return IsCopperLayer( aTrack->GetLayer() )
           && ( aTrack->Type() == PCB_TRACE_T || aTrack->Type() == PCB_VIA_T )
           && ( ( aDisplOpts->m_ShowTrackClearanceMode == PCB_DISPLAY_OPTIONS::SHOW_CLEARANCE_NEW_AND_EDITED_TRACKS_AND_VIA_AREAS
                  && ( aTrack->IsDragging() || aTrack->IsMoving() || aTrack->IsNew() ) )
            || ( aDisplOpts->m_ShowTrackClearanceMode == PCB_DISPLAY_OPTIONS::SHOW_CLEARANCE_ALWAYS )
            );

}


TRACK::TRACK( BOARD_ITEM* aParent, KICAD_T idtype ) :
    BOARD_CONNECTED_ITEM( aParent, idtype )
{
    m_Width = Millimeter2iu( 0.2 );
}


EDA_ITEM* TRACK::Clone() const
{
    return new TRACK( *this );
}


VIA::VIA( BOARD_ITEM* aParent ) :
    TRACK( aParent, PCB_VIA_T )
{
    SetViaType( VIA_THROUGH );
    m_BottomLayer = B_Cu;
    SetDrillDefault();
}


EDA_ITEM* VIA::Clone() const
{
    return new VIA( *this );
}


wxString VIA::GetSelectMenuText( EDA_UNITS_T aUnits ) const
{
    wxString format;
    BOARD* board = GetBoard();

    switch( GetViaType() )
    {
    case VIA_BLIND_BURIED:
        format = _( "Blind/Buried Via %s %s on %s - %s" );
        break;
    case VIA_MICROVIA:
        format = _( "Micro Via %s %s on %s - %s" );
        break;
    // else say nothing about normal (through) vias
    default:
        format = _( "Via %s %s on %s - %s" );
        break;
    }

    if( board )
    {
        // say which layers, only two for now
        PCB_LAYER_ID topLayer;
        PCB_LAYER_ID botLayer;
        LayerPair( &topLayer, &botLayer );
        return wxString::Format( format.GetData(),
                                 MessageTextFromValue( aUnits, m_Width ),
                                 GetNetnameMsg(),
                                 board->GetLayerName( topLayer ),
                                 board->GetLayerName( botLayer ) );

    }
    else
    {
        return wxString::Format( format.GetData(),
                                 MessageTextFromValue( aUnits, m_Width ),
                                 GetNetnameMsg(),
                                 wxT( "??" ),
                                 wxT( "??" ) );
    }
}


BITMAP_DEF VIA::GetMenuImage() const
{
    return via_xpm;
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
    NETCLASSPTR netclass = GetNetClass();

    if( GetViaType() == VIA_MICROVIA )
        return netclass->GetuViaDrill();

    return netclass->GetViaDrill();
}


STATUS_FLAGS TRACK::IsPointOnEnds( const wxPoint& point, int min_dist ) const
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
    int ymax, xmax, ymin, xmin;

    if( Type() == PCB_VIA_T )
    {
        ymax = m_Start.y;
        xmax = m_Start.x;

        ymin = m_Start.y;
        xmin = m_Start.x;
    }
    else
    {
        ymax = std::max( m_Start.y, m_End.y );
        xmax = std::max( m_Start.x, m_End.x );

        ymin = std::min( m_Start.y, m_End.y );
        xmin = std::min( m_Start.x, m_End.x );
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


void TRACK::Flip( const wxPoint& aCentre, bool aFlipLeftRight )
{
    if( aFlipLeftRight )
    {
        m_Start.x = aCentre.x - ( m_Start.x - aCentre.x );
        m_End.x   = aCentre.x - ( m_End.x - aCentre.x );
    }
    else
    {
        m_Start.y = aCentre.y - ( m_Start.y - aCentre.y );
        m_End.y   = aCentre.y - ( m_End.y - aCentre.y );
    }

    int copperLayerCount = GetBoard()->GetCopperLayerCount();
    SetLayer( FlipLayer( GetLayer(), copperLayerCount ) );
}


void VIA::Flip( const wxPoint& aCentre, bool aFlipLeftRight )
{
    if( aFlipLeftRight )
    {
        m_Start.x = aCentre.x - ( m_Start.x - aCentre.x );
        m_End.x   = aCentre.x - ( m_End.x - aCentre.x );
    }
    else
    {
        m_Start.y = aCentre.y - ( m_Start.y - aCentre.y );
        m_End.y   = aCentre.y - ( m_End.y - aCentre.y );
    }

    if( GetViaType() != VIA_THROUGH )
    {
        int copperLayerCount = GetBoard()->GetCopperLayerCount();
        PCB_LAYER_ID top_layer;
        PCB_LAYER_ID bottom_layer;
        LayerPair( &top_layer, &bottom_layer );
        top_layer = FlipLayer( top_layer, copperLayerCount );
        bottom_layer = FlipLayer( bottom_layer, copperLayerCount );
        SetLayerPair( top_layer, bottom_layer );
    }
}


// see class_track.h
SEARCH_RESULT TRACK::Visit( INSPECTOR inspector, void* testData, const KICAD_T scanTypes[] )
{
    KICAD_T stype = *scanTypes;

    // If caller wants to inspect my type
    if( stype == Type() )
    {
        if( SEARCH_QUIT == inspector( this, testData ) )
            return SEARCH_QUIT;
    }

    return SEARCH_CONTINUE;
}


bool VIA::IsOnLayer( PCB_LAYER_ID layer_number ) const
{
    PCB_LAYER_ID bottom_layer, top_layer;

    LayerPair( &top_layer, &bottom_layer );

    wxASSERT( top_layer <= bottom_layer );

    if( top_layer <= layer_number && layer_number <= bottom_layer )
        return true;
    else
        return false;
}


LSET VIA::GetLayerSet() const
{
    if( GetViaType() == VIA_THROUGH )
        return LSET::AllCuMask();

    // VIA_BLIND_BURIED or VIA_MICRVIA:

    LSET layermask;

    wxASSERT( m_Layer <= m_BottomLayer );

    // PCB_LAYER_IDs are numbered from front to back, this is top to bottom.
    for( LAYER_NUM id = m_Layer;  id <= m_BottomLayer;  ++id )
    {
        layermask.set( id );
    }

    return layermask;
}


void VIA::SetLayerPair( PCB_LAYER_ID aTopLayer, PCB_LAYER_ID aBottomLayer )
{

    m_Layer = aTopLayer;
    m_BottomLayer = aBottomLayer;
    SanitizeLayers();
}


void VIA::SetTopLayer( PCB_LAYER_ID aLayer )
{
    m_Layer = aLayer;
}


void VIA::SetBottomLayer( PCB_LAYER_ID aLayer )
{
    m_BottomLayer = aLayer;
}


void VIA::LayerPair( PCB_LAYER_ID* top_layer, PCB_LAYER_ID* bottom_layer ) const
{
    PCB_LAYER_ID t_layer = F_Cu;
    PCB_LAYER_ID b_layer = B_Cu;

    if( GetViaType() != VIA_THROUGH )
    {
        b_layer = m_BottomLayer;
        t_layer = m_Layer;

        if( b_layer < t_layer )
            std::swap( b_layer, t_layer );
    }

    if( top_layer )
        *top_layer = t_layer;

    if( bottom_layer )
        *bottom_layer = b_layer;
}


PCB_LAYER_ID VIA::TopLayer() const
{
    return m_Layer;
}


PCB_LAYER_ID VIA::BottomLayer() const
{
    return m_BottomLayer;
}


void VIA::SanitizeLayers()
{
    if( GetViaType() == VIA_THROUGH )
    {
        m_Layer    = F_Cu;
        m_BottomLayer = B_Cu;
    }

    if( m_BottomLayer < m_Layer )
        std::swap( m_BottomLayer, m_Layer );
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

void TRACK::Print( PCB_BASE_FRAME* aFrame, wxDC* aDC, const wxPoint& aOffset )
{
    BOARD* brd = GetBoard();
    auto   color = aFrame->Settings().Colors().GetLayerColor( m_Layer );

    if( !brd->IsLayerVisible( m_Layer ) || !brd->IsElementVisible( LAYER_TRACKS ) )
        return;

    auto displ_opts = (PCB_DISPLAY_OPTIONS*)( aFrame->GetDisplayOptions() );

    color.a = 0.588;

    // Draw track as line if width <= 1pixel:
    if( aDC->LogicalToDeviceXRel( m_Width ) <= 1 )
    {
        GRLine( nullptr, aDC, m_Start + aOffset, m_End + aOffset, m_Width, color );
        return;
    }

    if( !displ_opts->m_DisplayPcbTrackFill || GetState( FORCE_SKETCH ) )
    {
        GRCSegm( nullptr, aDC, m_Start + aOffset, m_End + aOffset, m_Width, color );
    }
    else
    {
        GRFillCSegm( nullptr, aDC, m_Start.x + aOffset.x, m_Start.y + aOffset.y,
                     m_End.x + aOffset.x, m_End.y + aOffset.y, m_Width, color );
    }
}


void TRACK::ViewGetLayers( int aLayers[], int& aCount ) const
{
    // Show the track and its netname on different layers
    aLayers[0] = GetLayer();
    aLayers[1] = GetNetnameLayer( aLayers[0] );
    aCount = 2;
}


unsigned int TRACK::ViewGetLOD( int aLayer, KIGFX::VIEW* aView ) const
{
    const int HIDE = std::numeric_limits<unsigned int>::max();

    if( !aView->IsLayerVisible( LAYER_TRACKS ) )
        return HIDE;

    // Netnames will be shown only if zoom is appropriate
    if( IsNetnameLayer( aLayer ) )
    {
        return ( Millimeter2iu( 4 ) / ( m_Width + 1 ) );
    }

    // Other layers are shown without any conditions
    return 0;
}


const BOX2I TRACK::ViewBBox() const
{
    BOX2I bbox = GetBoundingBox();
    bbox.Inflate( 2 * GetClearance() );
    return bbox;
}


void VIA::Print( PCB_BASE_FRAME* aFrame, wxDC* aDC, const wxPoint& aOffset )
{
    int                  radius;
    int                  fillvia = 0;
    PCB_SCREEN*          screen = aFrame->GetScreen();
    PCB_DISPLAY_OPTIONS* displ_opts = (PCB_DISPLAY_OPTIONS*) aFrame->GetDisplayOptions();
    BOARD*               brd =  GetBoard();
    COLOR4D              color = aFrame->Settings().Colors().GetItemColor( LAYER_VIAS + GetViaType() );

    if( displ_opts->m_DisplayViaFill == FILLED )
        fillvia = 1;

    if( !brd->IsElementVisible( LAYER_VIAS + GetViaType() ) )
       return;

    // Only draw the via if at least one of the layers it crosses is being displayed
    if( !( brd->GetVisibleLayers() & GetLayerSet() ).any() )
        return;

    color.a = 0.588;

    radius = m_Width >> 1;
    // for small via size on screen (radius < 4 pixels) draw a simplified shape

    int  radius_in_pixels = aDC->LogicalToDeviceXRel( radius );
    bool fast_draw = false;

    // Vias are drawn as a filled circle or a double circle. The hole will be drawn later
    int  drill_radius = GetDrillValue() / 2;
    int  inner_radius = radius - aDC->DeviceToLogicalXRel( 2 );

    if( radius_in_pixels < MIN_VIA_DRAW_SIZE )
    {
        fast_draw = true;
        fillvia = false;
    }

    if( fillvia )
    {
        GRFilledCircle( nullptr, aDC, m_Start + aOffset, radius, color );
    }
    else
    {
        GRCircle( nullptr, aDC, m_Start + aOffset, radius, 0, color );

        if ( fast_draw )
            return;

        GRCircle( nullptr, aDC, m_Start + aOffset, inner_radius, 0, color );
    }

    if( fillvia )
    {
        bool blackpenstate = false;

        if( screen->m_IsPrinting )
        {
            blackpenstate = GetGRForceBlackPenState();
            GRForceBlackPen( false );
            color = WHITE;
        }
        else
        {
            color = BLACK;     // or DARKGRAY;
        }

        // Draw hole if the radius is > 1pixel.
        if( aDC->LogicalToDeviceXRel( drill_radius ) > 1 )
            GRFilledCircle( nullptr, aDC, m_Start.x + aOffset.x, m_Start.y + aOffset.y,
                            drill_radius, 0, color, color );

        if( screen->m_IsPrinting )
            GRForceBlackPen( blackpenstate );
    }
    else
    {
        if( drill_radius < inner_radius )         // We can show the via hole
            GRCircle( nullptr, aDC, m_Start + aOffset, drill_radius, 0, color );
    }

    if( ShowClearance( displ_opts, this ) )
    {
        GRCircle( nullptr, aDC, m_Start + aOffset, radius + GetClearance(), 0, color );
    }

    // for Micro Vias, draw a partial cross : X on component layer, or + on copper layer
    // (so we can see 2 superimposed microvias ):
    if( GetViaType() == VIA_MICROVIA )
    {
        int ax, ay, bx, by;

        if( IsOnLayer( B_Cu ) )
        {
            ax = radius; ay = 0;
            bx = drill_radius; by = 0;
        }
        else
        {
            ax = ay = (radius * 707) / 1000;
            bx = by = (drill_radius * 707) / 1000;
        }

        // lines '|' or '\'
        GRLine( nullptr, aDC, m_Start.x + aOffset.x - ax, m_Start.y + aOffset.y - ay,
                m_Start.x + aOffset.x - bx, m_Start.y + aOffset.y - by, 0, color );
        GRLine( nullptr, aDC, m_Start.x + aOffset.x + bx, m_Start.y + aOffset.y + by,
                m_Start.x + aOffset.x + ax, m_Start.y + aOffset.y + ay, 0, color );

        // lines - or '/'
        GRLine( nullptr, aDC, m_Start.x + aOffset.x + ay, m_Start.y + aOffset.y - ax,
                m_Start.x + aOffset.x + by, m_Start.y + aOffset.y - bx, 0, color );
        GRLine( nullptr, aDC, m_Start.x + aOffset.x - by, m_Start.y + aOffset.y + bx,
                m_Start.x + aOffset.x - ay, m_Start.y + aOffset.y + ax, 0, color );
    }

    // for Buried Vias, draw a partial line : orient depending on layer pair
    // (so we can see superimposed buried vias ):
    if( GetViaType() == VIA_BLIND_BURIED )
    {
        int ax = 0, ay = radius, bx = 0, by = drill_radius;
        PCB_LAYER_ID layer_top, layer_bottom;

        LayerPair( &layer_top, &layer_bottom );

        // lines for the top layer
        RotatePoint( &ax, &ay, layer_top * 3600.0 / brd->GetCopperLayerCount( ) );
        RotatePoint( &bx, &by, layer_top * 3600.0 / brd->GetCopperLayerCount( ) );
        GRLine( nullptr, aDC, m_Start.x + aOffset.x - ax, m_Start.y + aOffset.y - ay,
                m_Start.x + aOffset.x - bx, m_Start.y + aOffset.y - by, 0, color );

        // lines for the bottom layer
        ax = 0; ay = radius; bx = 0; by = drill_radius;
        RotatePoint( &ax, &ay, layer_bottom * 3600.0 / brd->GetCopperLayerCount( ) );
        RotatePoint( &bx, &by, layer_bottom * 3600.0 / brd->GetCopperLayerCount( ) );
        GRLine( nullptr, aDC, m_Start.x + aOffset.x - ax, m_Start.y + aOffset.y - ay,
                m_Start.x + aOffset.x - bx, m_Start.y + aOffset.y - by, 0, color );
    }

    // Display the short netname:
    if( GetNetCode() == NETINFO_LIST::UNCONNECTED )
        return;

    if( displ_opts->m_DisplayNetNamesMode == 0 || displ_opts->m_DisplayNetNamesMode == 1 )
        return;

    NETINFO_ITEM* net = GetNet();

    if( net == NULL )
        return;

    wxString text = UnescapeString( net->GetShortNetname() );
    int len = text.Len();

    if( len > 0 )
    {
        // calculate a good size for the text
        int tsize = m_Width / len;

        if( aDC->LogicalToDeviceXRel( tsize ) >= MIN_TEXT_SIZE )
        {
            tsize = (tsize * 7) / 10;        // small reduction to give a better look, inside via

            GRHaloText( aDC, m_Start, color, WHITE, BLACK, text, 0, wxSize( tsize, tsize ),
                        GR_TEXT_HJUSTIFY_CENTER, GR_TEXT_VJUSTIFY_CENTER, tsize/7, false, false );
        }
    }
}


void VIA::ViewGetLayers( int aLayers[], int& aCount ) const
{
    aLayers[0] = LAYER_VIAS_HOLES;
    aLayers[1] = LAYER_VIAS_NETNAMES;
    aCount = 3;

    // Just show it on common via & via holes layers
    switch( GetViaType() )
    {
    case VIA_THROUGH:
        aLayers[2] = LAYER_VIA_THROUGH;
        break;

    case VIA_BLIND_BURIED:
        aLayers[2] = LAYER_VIA_BBLIND;
        aLayers[3] = m_Layer;
        aLayers[4] = m_BottomLayer;
        aCount += 2;
        break;

    case VIA_MICROVIA:
        aLayers[2] = LAYER_VIA_MICROVIA;
        break;

    default:
        aLayers[2] = LAYER_GP_OVERLAY;
        wxASSERT( false );
        break;
    }
}


unsigned int VIA::ViewGetLOD( int aLayer, KIGFX::VIEW* aView ) const
{
    constexpr unsigned int HIDE = std::numeric_limits<unsigned int>::max();

    // Netnames will be shown only if zoom is appropriate
    if( IsNetnameLayer( aLayer ) )
        return m_Width == 0 ? HIDE : ( Millimeter2iu( 10 ) / m_Width );


    BOARD* board = GetBoard();

    // Only draw the via if at least one of the layers it crosses is being displayed
    if( board && ( board->GetVisibleLayers() & GetLayerSet() ).any() )
        return 0;

    return HIDE;
}


// see class_track.h
void TRACK::GetMsgPanelInfo( EDA_UNITS_T aUnits, std::vector< MSG_PANEL_ITEM >& aList )
{
    wxString msg;
    BOARD*   board = GetBoard();

    // Display basic infos
    GetMsgPanelInfoBase( aUnits, aList );

    // Display full track length (in Pcbnew)
    if( board )
    {
        int    count;
        double trackLen;
        double lenPadToDie;

        std::tie( count, trackLen, lenPadToDie ) = board->GetTrackLength( *this );

        msg = MessageTextFromValue( aUnits, trackLen );
        aList.push_back( MSG_PANEL_ITEM( _( "Length" ), msg, DARKCYAN ) );

        if( lenPadToDie != 0 )
        {
            msg = MessageTextFromValue( aUnits, trackLen + lenPadToDie );
            aList.push_back( MSG_PANEL_ITEM( _( "Full Length" ), msg, DARKCYAN ) );

            msg = MessageTextFromValue( aUnits, lenPadToDie, true );
            aList.push_back( MSG_PANEL_ITEM( _( "Pad To Die Length" ), msg, DARKCYAN ) );
        }
    }

    NETCLASSPTR netclass = GetNetClass();

    if( netclass )
    {
        aList.push_back( MSG_PANEL_ITEM( _( "NC Name" ), netclass->GetName(), DARKMAGENTA ) );

        msg = MessageTextFromValue( aUnits, netclass->GetClearance(), true );
        aList.push_back( MSG_PANEL_ITEM( _( "NC Clearance" ), msg, DARKMAGENTA ) );

        msg = MessageTextFromValue( aUnits, netclass->GetTrackWidth(), true );
        aList.push_back( MSG_PANEL_ITEM( _( "NC Width" ), msg, DARKMAGENTA ) );

        msg = MessageTextFromValue( aUnits, netclass->GetViaDiameter(), true );
        aList.push_back( MSG_PANEL_ITEM( _( "NC Via Size" ), msg, DARKMAGENTA ) );

        msg = MessageTextFromValue( aUnits, netclass->GetViaDrill(), true );
        aList.push_back( MSG_PANEL_ITEM( _( "NC Via Drill"), msg, DARKMAGENTA ) );
    }
}

void TRACK::GetMsgPanelInfoBase_Common( EDA_UNITS_T aUnits, std::vector< MSG_PANEL_ITEM >& aList )
{
    wxString msg;

    // Display Net Name
    if( GetBoard() )
    {
        NETINFO_ITEM* net = GetNet();

        if( net )
            msg = UnescapeString( net->GetNetname() );
        else
            msg = wxT( "<no name>" );

        aList.push_back( MSG_PANEL_ITEM( _( "NetName" ), msg, RED ) );

        // Display net code : (useful in test or debug)
        msg.Printf( wxT( "%d" ), GetNetCode() );
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
        msg[0] = 'L';

    if( GetState( TRACK_AR ) )
        msg[2] = 'A';

    aList.push_back( MSG_PANEL_ITEM( _( "Status" ), msg, MAGENTA ) );
}

void TRACK::GetMsgPanelInfoBase( EDA_UNITS_T aUnits, std::vector< MSG_PANEL_ITEM >& aList )
{
    wxString msg;
    BOARD* board = GetBoard();

    aList.push_back( MSG_PANEL_ITEM( _( "Type" ), _( "Track" ), DARKCYAN ) );

    GetMsgPanelInfoBase_Common( aUnits, aList );

    // Display layer
    if( board )
        msg = board->GetLayerName( m_Layer );
    else
        msg.Printf(wxT("%d"), m_Layer );

    aList.push_back( MSG_PANEL_ITEM( _( "Layer" ), msg, BROWN ) );

    // Display width
    msg = MessageTextFromValue( aUnits, m_Width, true );

    aList.push_back( MSG_PANEL_ITEM( _( "Width" ), msg, DARKCYAN ) );

    // Display segment length
    msg = ::MessageTextFromValue( aUnits, GetLength() );
    aList.push_back( MSG_PANEL_ITEM( _( "Segment Length" ), msg, DARKCYAN ) );
}

void VIA::GetMsgPanelInfoBase( EDA_UNITS_T aUnits, std::vector< MSG_PANEL_ITEM >& aList )
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

    GetMsgPanelInfoBase_Common( aUnits, aList );


    // Display layer pair
    PCB_LAYER_ID top_layer, bottom_layer;

    LayerPair( &top_layer, &bottom_layer );

    if( board )
        msg = board->GetLayerName( top_layer ) + wxT( "/" )
            + board->GetLayerName( bottom_layer );
    else
        msg.Printf( wxT( "%d/%d" ), top_layer, bottom_layer );

    aList.push_back( MSG_PANEL_ITEM( _( "Layers" ), msg, BROWN ) );

    // Display width
    msg = MessageTextFromValue( aUnits, m_Width, true );

    // Display diameter value:
    aList.push_back( MSG_PANEL_ITEM( _( "Diameter" ), msg, DARKCYAN ) );

    // Display drill value
    msg = MessageTextFromValue( aUnits, GetDrillValue() );

    wxString title = _( "Drill" );
    title += wxT( " " );

    bool drl_specific = true;

    if( GetBoard() )
    {
        NETINFO_ITEM* net = GetNet();
        int drill_class_value = 0;

        if( net )
        {
            if( GetViaType() == VIA_MICROVIA )
                drill_class_value = net->GetMicroViaDrillSize();
            else
                drill_class_value = net->GetViaDrillSize();
        }

        drl_specific = GetDrillValue() != drill_class_value;
    }


    if( drl_specific )
        title += _( "(Specific)" );
    else
        title += _( "(NetClass)" );

    aList.push_back( MSG_PANEL_ITEM( title, msg, RED ) );
}


bool TRACK::HitTest( const wxPoint& aPosition, int aAccuracy ) const
{
    return TestSegmentHit( aPosition, m_Start, m_End, aAccuracy + ( m_Width / 2 ) );
}

bool VIA::HitTest( const wxPoint& aPosition, int aAccuracy ) const
{
    int max_dist = aAccuracy + ( m_Width / 2 );

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
    {
        return arect.Contains( box );
    }
    else
    {
        return arect.IntersectsCircle( GetStart(), GetWidth() / 2 );
    }
}


wxString TRACK::GetSelectMenuText( EDA_UNITS_T aUnits ) const
{
    return wxString::Format( _("Track %s %s on %s, length: %s" ),
                             MessageTextFromValue( aUnits, m_Width ),
                             GetNetnameMsg(),
                             GetLayerName(),
                             MessageTextFromValue( aUnits, GetLength() ) );
}


BITMAP_DEF TRACK::GetMenuImage() const
{
    return add_tracks_xpm;
}

void TRACK::SwapData( BOARD_ITEM* aImage )
{
    assert( aImage->Type() == PCB_TRACE_T );

    std::swap( *((TRACK*) this), *((TRACK*) aImage) );
}

void VIA::SwapData( BOARD_ITEM* aImage )
{
    assert( aImage->Type() == PCB_VIA_T );

    std::swap( *((VIA*) this), *((VIA*) aImage) );
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
