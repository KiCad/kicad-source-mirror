/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 Jean-Pierre Charras, jean-pierre.charras@ujf-grenoble.fr
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2012 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 1992-2019 KiCad Developers, see AUTHORS.txt for contributors.
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
//#include <common.h>
#include <pcb_screen.h>
#include <pcb_base_frame.h>
#include <class_board.h>
#include <class_track.h>
#include <pcbnew.h>
#include <base_units.h>
#include <msgpanel.h>
#include <bitmaps.h>
#include <view/view.h>
#include <math/util.h>      // for KiROUND
#include <pgm_base.h>
#include <settings/color_settings.h>
#include <settings/settings_manager.h>


TRACK::TRACK( BOARD_ITEM* aParent, KICAD_T idtype ) :
    BOARD_CONNECTED_ITEM( aParent, idtype )
{
    m_Width = Millimeter2iu( 0.2 );     // Gives a reasonable default width
}


EDA_ITEM* TRACK::Clone() const
{
    return new TRACK( *this );
}


EDA_ITEM* ARC::Clone() const
{
    return new ARC( *this );
}


VIA::VIA( BOARD_ITEM* aParent ) :
    TRACK( aParent, PCB_VIA_T )
{
    SetViaType( VIATYPE::THROUGH );
    m_BottomLayer = B_Cu;
    SetDrillDefault();
}


EDA_ITEM* VIA::Clone() const
{
    return new VIA( *this );
}


wxString VIA::GetSelectMenuText( EDA_UNITS aUnits ) const
{
    wxString format;
    BOARD*   board = GetBoard();

    switch( GetViaType() )
    {
    case VIATYPE::BLIND_BURIED:
        format = _( "Blind/Buried Via %s %s on %s - %s" );
        break;
    case VIATYPE::MICROVIA:
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
        return wxString::Format( format.GetData(), MessageTextFromValue( aUnits, m_Width ),
                GetNetnameMsg(), board->GetLayerName( topLayer ), board->GetLayerName( botLayer ) );
    }
    else
    {
        return wxString::Format( format.GetData(), MessageTextFromValue( aUnits, m_Width ),
                GetNetnameMsg(), wxT( "??" ), wxT( "??" ) );
    }
}


BITMAP_DEF VIA::GetMenuImage() const
{
    return via_xpm;
}


int TRACK::GetClearance( BOARD_CONNECTED_ITEM* aItem, wxString* aSource ) const
{
    // Currently tracks have no specific clearance parameter on a per track or per
    // segment basis.  The NETCLASS clearance is used.
    return BOARD_CONNECTED_ITEM::GetClearance( aItem, aSource );
}


int VIA::GetDrillValue() const
{
    if( m_Drill > 0 ) // Use the specific value.
        return m_Drill;

    // Use the default value from the Netclass
    NETCLASSPTR netclass = GetNetClass();

    if( GetViaType() == VIATYPE::MICROVIA )
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


void ARC::Rotate( const wxPoint& aRotCentre, double aAngle )
{
    RotatePoint( &m_Start, aRotCentre, aAngle );
    RotatePoint( &m_End, aRotCentre, aAngle );
    RotatePoint( &m_Mid, aRotCentre, aAngle );
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


void ARC::Flip( const wxPoint& aCentre, bool aFlipLeftRight )
{
    if( aFlipLeftRight )
    {
        m_Start.x = aCentre.x - ( m_Start.x - aCentre.x );
        m_End.x   = aCentre.x - ( m_End.x - aCentre.x );
        m_Mid.x = aCentre.x - ( m_Mid.x - aCentre.x );
    }
    else
    {
        m_Start.y = aCentre.y - ( m_Start.y - aCentre.y );
        m_End.y   = aCentre.y - ( m_End.y - aCentre.y );
        m_Mid.y = aCentre.y - ( m_Mid.y - aCentre.y );
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

    if( GetViaType() != VIATYPE::THROUGH )
    {
        int          copperLayerCount = GetBoard()->GetCopperLayerCount();
        PCB_LAYER_ID top_layer;
        PCB_LAYER_ID bottom_layer;
        LayerPair( &top_layer, &bottom_layer );
        top_layer    = FlipLayer( top_layer, copperLayerCount );
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
        if( SEARCH_RESULT::QUIT == inspector( this, testData ) )
            return SEARCH_RESULT::QUIT;
    }

    return SEARCH_RESULT::CONTINUE;
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
    if( GetViaType() == VIATYPE::THROUGH )
        return LSET::AllCuMask();

    // VIA_BLIND_BURIED or VIA_MICRVIA:

    LSET layermask;

    wxASSERT( m_Layer <= m_BottomLayer );

    // PCB_LAYER_IDs are numbered from front to back, this is top to bottom.
    for( LAYER_NUM id = m_Layer; id <= m_BottomLayer; ++id )
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

    if( GetViaType() != VIATYPE::THROUGH )
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
    if( GetViaType() == VIATYPE::THROUGH )
    {
        m_Layer       = F_Cu;
        m_BottomLayer = B_Cu;
    }

    if( m_BottomLayer < m_Layer )
        std::swap( m_BottomLayer, m_Layer );
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


void VIA::ViewGetLayers( int aLayers[], int& aCount ) const
{
    aLayers[0] = LAYER_VIAS_HOLES;
    aLayers[1] = LAYER_VIAS_NETNAMES;
    aCount = 3;

    // Just show it on common via & via holes layers
    switch( GetViaType() )
    {
    case VIATYPE::THROUGH:
        aLayers[2] = LAYER_VIA_THROUGH;
        break;

    case VIATYPE::BLIND_BURIED:
        aLayers[2] = LAYER_VIA_BBLIND;
        aLayers[3] = m_Layer;
        aLayers[4] = m_BottomLayer;
        aCount += 2;
        break;

    case VIATYPE::MICROVIA:
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

    LSET visibleLayers;

    for( int i = 0; i < PCB_LAYER_ID_COUNT; ++i )
    {
        if( aView->IsLayerVisible( i ) )
            visibleLayers.set( i );
    }

    // Only draw the via if at least one of the layers it crosses is being displayed
    if( ( visibleLayers & GetLayerSet() ).any() && aView->IsLayerVisible( LAYER_VIAS ) )
    {
        switch( m_ViaType )
        {
        case VIATYPE::THROUGH:      return aView->IsLayerVisible( LAYER_VIA_THROUGH )  ? 0 : HIDE;
        case VIATYPE::BLIND_BURIED: return aView->IsLayerVisible( LAYER_VIA_BBLIND )   ? 0 : HIDE;
        case VIATYPE::MICROVIA:     return aView->IsLayerVisible( LAYER_VIA_MICROVIA ) ? 0 : HIDE;
        default:                    return 0;
        }
    }

    return HIDE;
}


// see class_track.h
void TRACK::GetMsgPanelInfo( EDA_DRAW_FRAME* aFrame, std::vector<MSG_PANEL_ITEM>& aList )
{
    wxString msg;
    BOARD*   board = GetBoard();

    // Display basic infos
    GetMsgPanelInfoBase( aFrame, aList );

    // Display full track length (in Pcbnew)
    if( board )
    {
        int    count;
        double trackLen;
        double lenPadToDie;

        std::tie( count, trackLen, lenPadToDie ) = board->GetTrackLength( *this );

        msg = MessageTextFromValue( aFrame->GetUserUnits(), trackLen );
        aList.emplace_back( _( "Length" ), msg, DARKCYAN );

        if( lenPadToDie != 0 )
        {
            msg = MessageTextFromValue( aFrame->GetUserUnits(), trackLen + lenPadToDie );
            aList.emplace_back( _( "Full Length" ), msg, DARKCYAN );

            msg = MessageTextFromValue( aFrame->GetUserUnits(), lenPadToDie, true );
            aList.emplace_back( _( "Pad To Die Length" ), msg, DARKCYAN );
        }
    }

    NETCLASSPTR netclass = GetNetClass();

    if( netclass )
    {
        aList.emplace_back( _( "NC Name" ), netclass->GetName(), DARKMAGENTA );

        msg = MessageTextFromValue( aFrame->GetUserUnits(), netclass->GetClearance(), true );
        aList.emplace_back( _( "NC Clearance" ), msg, DARKMAGENTA );

        msg = MessageTextFromValue( aFrame->GetUserUnits(), netclass->GetTrackWidth(), true );
        aList.emplace_back( _( "NC Width" ), msg, DARKMAGENTA );

        msg = MessageTextFromValue( aFrame->GetUserUnits(), netclass->GetViaDiameter(), true );
        aList.emplace_back( _( "NC Via Size" ), msg, DARKMAGENTA );

        msg = MessageTextFromValue( aFrame->GetUserUnits(), netclass->GetViaDrill(), true );
        aList.emplace_back( _( "NC Via Drill"), msg, DARKMAGENTA );
    }
}

void TRACK::GetMsgPanelInfoBase_Common( EDA_DRAW_FRAME* aFrame, std::vector<MSG_PANEL_ITEM>& aList )
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

        aList.emplace_back( _( "NetName" ), msg, RED );

        // Display net code : (useful in test or debug)
        msg.Printf( wxT( "%d" ), GetNetCode() );
        aList.emplace_back( _( "NetCode" ), msg, RED );
    }

#if defined(DEBUG)

    // Display the flags
    msg.Printf( wxT( "0x%08X" ), m_Flags );
    aList.emplace_back( wxT( "Flags" ), msg, BLUE );

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

    aList.emplace_back( _( "Status" ), msg, MAGENTA );
}


void TRACK::GetMsgPanelInfoBase( EDA_DRAW_FRAME* aFrame, std::vector<MSG_PANEL_ITEM>& aList )
{
    wxString msg;
    BOARD* board = GetBoard();

    aList.emplace_back( _( "Type" ), _( "Track" ), DARKCYAN );

    GetMsgPanelInfoBase_Common( aFrame, aList );

    // Display layer
    if( board )
        msg = board->GetLayerName( m_Layer );
    else
        msg.Printf(wxT("%d"), m_Layer );

    aList.emplace_back( _( "Layer" ), msg, BROWN );

    // Display width
    msg = MessageTextFromValue( aFrame->GetUserUnits(), m_Width, true );

    aList.emplace_back( _( "Width" ), msg, DARKCYAN );

    // Display segment length
    msg = ::MessageTextFromValue( aFrame->GetUserUnits(), GetLength() );
    aList.emplace_back( _( "Segment Length" ), msg, DARKCYAN );
}


void VIA::GetMsgPanelInfoBase( EDA_DRAW_FRAME* aFrame, std::vector<MSG_PANEL_ITEM>& aList )
{
    wxString msg;
    BOARD*   board = GetBoard();

    switch( GetViaType() )
    {
    default:
    case VIATYPE::NOT_DEFINED:
        msg = wxT( "???" ); // Not used yet, does not exist currently
        break;

    case VIATYPE::MICROVIA:
        msg = _( "Micro Via" ); // from external layer (TOP or BOTTOM) from
                                // the near neighbor inner layer only
        break;

    case VIATYPE::BLIND_BURIED:
        msg = _( "Blind/Buried Via" ); // from inner or external to inner
                                       // or external layer (no restriction)
        break;

    case VIATYPE::THROUGH:
        msg = _( "Through Via" ); // Usual via (from TOP to BOTTOM layer only )
        break;
    }

    aList.emplace_back( _( "Type" ), msg, DARKCYAN );

    GetMsgPanelInfoBase_Common( aFrame, aList );


    // Display layer pair
    PCB_LAYER_ID top_layer, bottom_layer;

    LayerPair( &top_layer, &bottom_layer );

    if( board )
        msg = board->GetLayerName( top_layer ) + wxT( "/" )
            + board->GetLayerName( bottom_layer );
    else
        msg.Printf( wxT( "%d/%d" ), top_layer, bottom_layer );

    aList.emplace_back( _( "Layers" ), msg, BROWN );

    // Display width
    msg = MessageTextFromValue( aFrame->GetUserUnits(), m_Width, true );

    // Display diameter value:
    aList.emplace_back( _( "Diameter" ), msg, DARKCYAN );

    // Display drill value
    msg = MessageTextFromValue( aFrame->GetUserUnits(), GetDrillValue() );

    wxString title = _( "Drill" );
    title += wxT( " " );

    bool drl_specific = true;

    if( GetBoard() )
    {
        NETINFO_ITEM* net = GetNet();
        int drill_class_value = 0;

        if( net )
        {
            if( GetViaType() == VIATYPE::MICROVIA )
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

    aList.emplace_back( title, msg, RED );
}


bool TRACK::HitTest( const wxPoint& aPosition, int aAccuracy ) const
{
    return TestSegmentHit( aPosition, m_Start, m_End, aAccuracy + ( m_Width / 2 ) );
}


bool ARC::HitTest( const wxPoint& aPosition, int aAccuracy ) const
{
    int max_dist = aAccuracy + ( m_Width / 2 );
    wxPoint center = GetPosition();
    wxPoint relpos = aPosition - center;
    double dist = EuclideanNorm( relpos );
    double radius = GetRadius();

    if( std::abs( dist - radius ) > max_dist )
        return false;

    double arc_angle_start = GetArcAngleStart();    // Always 0.0 ... 360 deg, in 0.1 deg
    double arc_hittest = ArcTangente( relpos.y, relpos.x );

    // Calculate relative angle between the starting point of the arc, and the test point
    arc_hittest -= arc_angle_start;

    // Normalise arc_hittest between 0 ... 360 deg
    NORMALIZE_ANGLE_POS( arc_hittest );
    double arc_angle = GetAngle();

    if( arc_angle < 0 )
        return arc_hittest >= 3600 + arc_angle;

    return  arc_hittest <= GetAngle();
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


bool ARC::HitTest( const EDA_RECT& aRect, bool aContained, int aAccuracy ) const
{
    EDA_RECT box;
    EDA_RECT arect = aRect;
    arect.Inflate( aAccuracy );

    box.SetOrigin( GetStart() );
    box.Merge( GetMid() );
    box.Merge( GetEnd() );

    box.Inflate( GetWidth() / 2 );

    if( aContained )
        return arect.Contains( box );
    else
        return arect.Intersects( box );
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


wxString TRACK::GetSelectMenuText( EDA_UNITS aUnits ) const
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

void ARC::SwapData( BOARD_ITEM* aImage )
{
    assert( aImage->Type() == PCB_ARC_T );

    std::swap( *this, *static_cast<ARC*>( aImage ) );
}

void VIA::SwapData( BOARD_ITEM* aImage )
{
    assert( aImage->Type() == PCB_VIA_T );

    std::swap( *((VIA*) this), *((VIA*) aImage) );
}


const wxPoint ARC::GetPosition() const
{
    auto center = GetArcCenter( VECTOR2I( m_Start ), VECTOR2I( m_Mid ), VECTOR2I( m_End ) );
    return wxPoint( center.x, center.y );
}

double ARC::GetRadius() const
{
    auto center = GetArcCenter( VECTOR2I( m_Start ), VECTOR2I( m_Mid ), VECTOR2I( m_End ) );
    return GetLineLength( wxPoint( center ), m_Start );
}

double ARC::GetAngle() const
{
    wxPoint center = GetPosition();
    wxPoint p0 = m_Start - center;
    wxPoint p1 = m_Mid - center;
    wxPoint p2 = m_End - center;
    double angle1 = ArcTangente( p1.y, p1.x ) - ArcTangente( p0.y, p0.x );
    double angle2 = ArcTangente( p2.y, p2.x ) - ArcTangente( p1.y, p1.x );

    return NormalizeAngle180( angle1 ) + NormalizeAngle180( angle2 );
}

double ARC::GetArcAngleStart() const
{
    wxPoint center = GetPosition();

    double angleStart = ArcTangente( m_Start.y - center.y,
                                     m_Start.x - center.x );
    return NormalizeAnglePos( angleStart );
}

double ARC::GetArcAngleEnd() const
{
    wxPoint center = GetPosition();

    double angleEnd = ArcTangente( m_End.y - center.y,
                                   m_End.x - center.x );
    return NormalizeAnglePos( angleEnd );
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
