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

#include <pcb_base_frame.h>
#include <connectivity/connectivity_data.h>
#include <board.h>
#include <track.h>
#include <base_units.h>
#include <bitmaps.h>
#include <kicad_string.h>
#include <view/view.h>
#include <settings/color_settings.h>
#include <settings/settings_manager.h>
#include <i18n_utility.h>
#include <geometry/shape_segment.h>
#include <geometry/shape_circle.h>
#include <geometry/shape_arc.h>
#include <drc/drc_engine.h>
#include <pcb_painter.h>

using KIGFX::PCB_PAINTER;
using KIGFX::PCB_RENDER_SETTINGS;

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
    m_bottomLayer = B_Cu;
    SetDrillDefault();
    m_removeUnconnectedLayer = false;
    m_keepTopBottomLayer = true;
    m_isFree = false;
}


EDA_ITEM* VIA::Clone() const
{
    return new VIA( *this );
}


wxString VIA::GetSelectMenuText( EDA_UNITS aUnits ) const
{
    wxString formatStr;

    switch( GetViaType() )
    {
    case VIATYPE::BLIND_BURIED: formatStr = _( "Blind/Buried Via %s on %s" ); break;
    case VIATYPE::MICROVIA:     formatStr = _( "Micro Via %s on %s" );        break;
    default:                    formatStr = _( "Via %s on %s" );              break;
    }

    return wxString::Format( formatStr,
                             GetNetnameMsg(),
                             layerMaskDescribe() );
}


BITMAP_DEF VIA::GetMenuImage() const
{
    return via_xpm;
}


int TRACK::GetLocalClearance( wxString* aSource ) const
{
    // Not currently implemented
    return 0;
}


void TRACK::GetWidthConstraints( int* aMin, int* aMax, wxString* aSource ) const
{
    *aMin = 0;
    *aMax = INT_MAX;

    DRC_CONSTRAINT constraint;

    if( GetBoard() && GetBoard()->GetDesignSettings().m_DRCEngine )
    {
        BOARD_DESIGN_SETTINGS& bds = GetBoard()->GetDesignSettings();

        constraint = bds.m_DRCEngine->EvalRulesForItems( TRACK_WIDTH_CONSTRAINT, this, nullptr,
                                                         GetLayer() );
    }

    if( constraint.Value().HasMin() || constraint.Value().HasMax() )
    {
        if( constraint.Value().HasMin() )
            *aMin = constraint.Value().Min();

        if( constraint.Value().HasMax() )
            *aMax = constraint.Value().Max();

        if( aSource )
            *aSource = constraint.GetName();
    }
}


int VIA::GetMinAnnulus( PCB_LAYER_ID aLayer, wxString* aSource ) const
{
    if( !FlashLayer( aLayer ) )
    {
        if( aSource )
            *aSource = _( "removed annular ring" );

        return 0;
    }

    DRC_CONSTRAINT constraint;

    if( GetBoard() && GetBoard()->GetDesignSettings().m_DRCEngine )
    {
        BOARD_DESIGN_SETTINGS& bds = GetBoard()->GetDesignSettings();

        constraint = bds.m_DRCEngine->EvalRulesForItems( ANNULAR_WIDTH_CONSTRAINT, this, nullptr,
                                                         aLayer );
    }

    if( constraint.Value().HasMin() )
    {
        if( aSource )
            *aSource = constraint.GetName();

        return constraint.Value().Min();
    }

    return 0;
}


int VIA::GetDrillValue() const
{
    if( m_drill > 0 ) // Use the specific value.
        return m_drill;

    // Use the default value from the Netclass
    NETCLASS* netclass = GetNetClass();

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
    else if( Type() == PCB_ARC_T )
    {
        std::shared_ptr<SHAPE> arc = GetEffectiveShape();
        auto bbox = arc->BBox();

        xmin = bbox.GetLeft();
        xmax = bbox.GetRight();
        ymin = bbox.GetTop();
        ymax = bbox.GetBottom();
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

    wxASSERT( m_layer <= m_bottomLayer );

    // PCB_LAYER_IDs are numbered from front to back, this is top to bottom.
    for( LAYER_NUM id = m_layer; id <= m_bottomLayer; ++id )
    {
        layermask.set( id );
    }

    return layermask;
}


void VIA::SetLayerSet( LSET aLayerSet )
{
    bool first = true;

    for( PCB_LAYER_ID layer : aLayerSet.Seq() )
    {
        if( first )
        {
            m_layer = layer;
            first = false;
        }

        m_bottomLayer = layer;
    }
}


void VIA::SetLayerPair( PCB_LAYER_ID aTopLayer, PCB_LAYER_ID aBottomLayer )
{

    m_layer = aTopLayer;
    m_bottomLayer = aBottomLayer;
    SanitizeLayers();
}


void VIA::SetTopLayer( PCB_LAYER_ID aLayer )
{
    m_layer = aLayer;
}


void VIA::SetBottomLayer( PCB_LAYER_ID aLayer )
{
    m_bottomLayer = aLayer;
}


void VIA::LayerPair( PCB_LAYER_ID* top_layer, PCB_LAYER_ID* bottom_layer ) const
{
    PCB_LAYER_ID t_layer = F_Cu;
    PCB_LAYER_ID b_layer = B_Cu;

    if( GetViaType() != VIATYPE::THROUGH )
    {
        b_layer = m_bottomLayer;
        t_layer = m_layer;

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
    return m_layer;
}


PCB_LAYER_ID VIA::BottomLayer() const
{
    return m_bottomLayer;
}


void VIA::SanitizeLayers()
{
    if( GetViaType() == VIATYPE::THROUGH )
    {
        m_layer       = F_Cu;
        m_bottomLayer = B_Cu;
    }

    if( m_bottomLayer < m_layer )
        std::swap( m_bottomLayer, m_layer );
}


bool VIA::FlashLayer( LSET aLayers ) const
{
    for( auto layer : aLayers.Seq() )
    {
        if( FlashLayer( layer ) )
            return true;
    }

    return false;
}


bool VIA::FlashLayer( int aLayer ) const
{
    std::vector<KICAD_T> types
    { PCB_TRACE_T, PCB_ARC_T, PCB_PAD_T, PCB_ZONE_T, PCB_FP_ZONE_T };

    // Return the "normal" shape if the caller doesn't specify a particular layer
    if( aLayer == UNDEFINED_LAYER )
        return true;

    BOARD* board = GetBoard();

    if( !board )
        return false;

    if( !IsOnLayer( static_cast<PCB_LAYER_ID>( aLayer ) ) )
        return false;

    if( !m_removeUnconnectedLayer )
        return true;

    if( m_keepTopBottomLayer && ( aLayer == m_layer || aLayer == m_bottomLayer ) )
        return true;

    return board->GetConnectivity()->IsConnectedOnLayer( this, static_cast<int>( aLayer ),
            types );
}


void TRACK::ViewGetLayers( int aLayers[], int& aCount ) const
{
    // Show the track and its netname on different layers
    aLayers[0] = GetLayer();
    aLayers[1] = GetNetnameLayer( aLayers[0] );
    aCount = 2;
}


double TRACK::ViewGetLOD( int aLayer, KIGFX::VIEW* aView ) const
{
    constexpr double HIDE = std::numeric_limits<double>::max();

    PCB_PAINTER*         painter = static_cast<PCB_PAINTER*>( aView->GetPainter() );
    PCB_RENDER_SETTINGS* renderSettings = painter->GetSettings();

    if( !aView->IsLayerVisible( LAYER_TRACKS ) )
        return HIDE;

    if( IsNetnameLayer( aLayer ) )
    {
        // Hide netnames on dimmed tracks
        if( renderSettings->GetHighContrast() )
        {
            if( m_layer != renderSettings->GetPrimaryHighContrastLayer() )
                return HIDE;
        }

        // Netnames will be shown only if zoom is appropriate
        return ( double ) Millimeter2iu( 4 ) / ( m_Width + 1 );
    }

    // Other layers are shown without any conditions
    return 0.0;
}


const BOX2I TRACK::ViewBBox() const
{
    BOX2I bbox = GetBoundingBox();

    BOARD* board = GetBoard();

    if( board )
        bbox.Inflate( 2 * board->GetDesignSettings().GetBiggestClearanceValue() );

    return bbox;
}


void VIA::ViewGetLayers( int aLayers[], int& aCount ) const
{
    aLayers[0] = LAYER_VIA_HOLES;
    aLayers[1] = LAYER_VIA_HOLEWALLS;
    aLayers[2] = LAYER_VIA_NETNAMES;

    // Just show it on common via & via holes layers
    switch( GetViaType() )
    {
    case VIATYPE::THROUGH:      aLayers[3] = LAYER_VIA_THROUGH;  break;
    case VIATYPE::BLIND_BURIED: aLayers[3] = LAYER_VIA_BBLIND;   break;
    case VIATYPE::MICROVIA:     aLayers[3] = LAYER_VIA_MICROVIA; break;
    default:                    aLayers[3] = LAYER_GP_OVERLAY;   break;
    }

    aCount = 4;
}


double VIA::ViewGetLOD( int aLayer, KIGFX::VIEW* aView ) const
{
    constexpr double HIDE = (double)std::numeric_limits<double>::max();

    PCB_PAINTER*         painter = static_cast<PCB_PAINTER*>( aView->GetPainter() );
    PCB_RENDER_SETTINGS* renderSettings = painter->GetSettings();
    BOARD*               board = GetBoard();
    LSET                 visible = LSET::AllLayersMask();

    // Meta control for hiding all vias
    if( !aView->IsLayerVisible( LAYER_VIAS ) )
        return HIDE;

    // Handle board visibility (unless printing)
    if( board && !aView->GetPrintMode() )
        visible = board->GetVisibleLayers() & board->GetEnabledLayers();

    if( IsViaPadLayer( aLayer ) )
    {
        if( !FlashLayer( visible ) )
            return HIDE;
    }
    else if( IsHoleLayer( aLayer ) )
    {
        if( !( visible & LSET::PhysicalLayersMask() ).any() )
            return HIDE;
    }
    else if( IsNetnameLayer( aLayer ) )
    {
        if( renderSettings->GetHighContrast() )
        {
            // Hide netnames unless via is flashed to a high-contrast layer
            if( !FlashLayer( renderSettings->GetPrimaryHighContrastLayer() ) )
                return HIDE;
        }
        else
        {
            // Hide netnames unless pad is flashed to a visible layer
            if( !FlashLayer( visible ) )
                return HIDE;
        }

        // Netnames will be shown only if zoom is appropriate
        return m_Width == 0 ? HIDE : ( (double)Millimeter2iu( 10 ) / m_Width );
    }

    // Passed all tests; show.
    return 0.0;
}


// see class_track.h
void TRACK::GetMsgPanelInfo( EDA_DRAW_FRAME* aFrame, std::vector<MSG_PANEL_ITEM>& aList )
{
    EDA_UNITS units = aFrame->GetUserUnits();
    wxString  msg;
    BOARD*    board = GetBoard();

    aList.emplace_back( _( "Type" ), _( "Track" ) );

    GetMsgPanelInfoBase_Common( aFrame, aList );

    aList.emplace_back( _( "Layer" ), layerMaskDescribe() );

    aList.emplace_back( _( "Width" ), MessageTextFromValue( units, m_Width ) );

    // Display full track length (in Pcbnew)
    if( board )
    {
        int    count;
        double trackLen;
        double lenPadToDie;

        std::tie( count, trackLen, lenPadToDie ) = board->GetTrackLength( *this );

        aList.emplace_back( _( "Length" ), MessageTextFromValue( units, trackLen ) );

        if( lenPadToDie != 0 )
        {
            msg = MessageTextFromValue( units, lenPadToDie );
            aList.emplace_back( _( "Pad To Die Length" ), msg );

            msg = MessageTextFromValue( units, trackLen + lenPadToDie );
            aList.emplace_back( _( "Full Length" ), msg );
        }
    }

    wxString source;
    int clearance = GetOwnClearance( GetLayer(), &source );

    aList.emplace_back( wxString::Format( _( "Min Clearance: %s" ),
                                          MessageTextFromValue( units, clearance ) ),
                        wxString::Format( _( "(from %s)" ), source ) );

    int minWidth, maxWidth;
    GetWidthConstraints( &minWidth, &maxWidth, &source );

    aList.emplace_back( wxString::Format( _( "Min Width: %s" ),
                                          MessageTextFromValue( units, minWidth ) ),
                        wxString::Format( _( "(from %s)" ), source ) );
}


void VIA::GetMsgPanelInfo( EDA_DRAW_FRAME* aFrame, std::vector<MSG_PANEL_ITEM>& aList )
{
    EDA_UNITS units = aFrame->GetUserUnits();
    wxString  msg;

    switch( GetViaType() )
    {
    case VIATYPE::MICROVIA:     msg = _( "Micro Via" );        break;
    case VIATYPE::BLIND_BURIED: msg = _( "Blind/Buried Via" ); break;
    case VIATYPE::THROUGH:      msg = _( "Through Via" );      break;
    default:                    msg = _( "Via" );              break;
    }

    aList.emplace_back( _( "Type" ), msg );

    GetMsgPanelInfoBase_Common( aFrame, aList );

    aList.emplace_back( _( "Layer" ), layerMaskDescribe() );

    msg = MessageTextFromValue( aFrame->GetUserUnits(), m_Width );

    aList.emplace_back( _( "Diameter" ), msg );

    msg = MessageTextFromValue( aFrame->GetUserUnits(), GetDrillValue() );

    aList.emplace_back( _( "Drill" ), msg );

    wxString  source;
    int clearance = GetOwnClearance( GetLayer(), &source );

    aList.emplace_back( wxString::Format( _( "Min Clearance: %s" ),
                                          MessageTextFromValue( units, clearance ) ),
                        wxString::Format( _( "(from %s)" ), source ) );

    int minAnnulus = GetMinAnnulus( GetLayer(), &source );

    aList.emplace_back( wxString::Format( _( "Min Annular Width: %s" ),
                                          MessageTextFromValue( units, minAnnulus ) ),
                        wxString::Format( _( "(from %s)" ), source ) );
}


void TRACK::GetMsgPanelInfoBase_Common( EDA_DRAW_FRAME* aFrame, std::vector<MSG_PANEL_ITEM>& aList ) const
{
    wxString msg;

    aList.emplace_back( _( "Net" ), UnescapeString( GetNetname() ) );

    aList.emplace_back( _( "NetClass" ), UnescapeString( GetNetClass()->GetName() ) );

    #if 0   // Enable for debugging
    if( GetBoard() )
    {
        // Display net code:
        msg.Printf( wxT( "%d" ), GetNetCode() );
        aList.emplace_back( _( "NetCode" ), msg );
    }

    // Display the flags:
    msg.Printf( wxT( "0x%08X" ), m_flags );
    aList.emplace_back( wxT( "Flags" ), msg );

    // Display start and end positions:
    msg.Printf( wxT( "%d %d" ), m_Start.x, m_Start.y );
    aList.push_back( MSG_PANEL_ITEM( wxT( "Start pos" ), msg ) );
    msg.Printf( wxT( "%d %d" ), m_End.x, m_End.y );
    aList.push_back( MSG_PANEL_ITEM( wxT( "End pos" ), msg ) );
#endif

    // Display the State member
    aList.emplace_back( _( "Status" ), IsLocked() ? _( "Locked" ) : wxT( "" ) );
}


wxString VIA::layerMaskDescribe() const
{
    BOARD*       board = GetBoard();
    PCB_LAYER_ID top_layer;
    PCB_LAYER_ID bottom_layer;

    LayerPair( &top_layer, &bottom_layer );

    return board->GetLayerName( top_layer ) + wxT( " - " ) + board->GetLayerName( bottom_layer );
}


bool TRACK::HitTest( const wxPoint& aPosition, int aAccuracy ) const
{
    return TestSegmentHit( aPosition, m_Start, m_End, aAccuracy + ( m_Width / 2 ) );
}


bool ARC::HitTest( const wxPoint& aPosition, int aAccuracy ) const
{
    int max_dist = aAccuracy + ( m_Width / 2 );

    // Short-circuit common cases where the arc is connected to a track or via at an endpoint
    if( EuclideanNorm( GetStart() - aPosition ) <= max_dist ||
            EuclideanNorm( GetEnd() - aPosition ) <= max_dist )
    {
        return true;
    }

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

    return  arc_hittest <= arc_angle;
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
        return arect.Contains( GetStart() ) && arect.Contains( GetEnd() );
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
        return arect.Contains( box );
    else
        return arect.IntersectsCircle( GetStart(), GetWidth() / 2 );
}


wxString TRACK::GetSelectMenuText( EDA_UNITS aUnits ) const
{
    return wxString::Format( _("Track %s on %s, length %s" ),
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


wxPoint ARC::GetPosition() const
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


bool TRACK::cmp_tracks::operator() ( const TRACK* a, const TRACK* b ) const
{
    if( a->GetNetCode() != b->GetNetCode() )
        return a->GetNetCode() < b->GetNetCode();

    if( a->GetLayer() != b->GetLayer() )
        return a->GetLayer() < b->GetLayer();

    if( a->Type() != b->Type() )
        return a->Type() < b->Type();

    if( a->m_Uuid != b->m_Uuid )
        return a->m_Uuid < b->m_Uuid;

    return a < b;
}


std::shared_ptr<SHAPE> TRACK::GetEffectiveShape( PCB_LAYER_ID aLayer ) const
{
    return std::make_shared<SHAPE_SEGMENT>( m_Start, m_End, m_Width );
}


std::shared_ptr<SHAPE> VIA::GetEffectiveShape( PCB_LAYER_ID aLayer ) const
{
    if( FlashLayer( aLayer ) )
        return std::make_shared<SHAPE_CIRCLE>( m_Start, m_Width / 2 );
    else
        return std::make_shared<SHAPE_CIRCLE>( m_Start, GetDrillValue() / 2 );
}


std::shared_ptr<SHAPE> ARC::GetEffectiveShape( PCB_LAYER_ID aLayer ) const
{
    return std::make_shared<SHAPE_ARC>( GetStart(), GetMid(), GetEnd(), GetWidth() );
}



#if defined(DEBUG)

wxString TRACK::ShowState( int stateBits )
{
    wxString ret;

    if( stateBits & IS_LINKED )
        ret << wxT( " | IS_LINKED" );

    if( stateBits & LOCKED )
        ret << wxT( " | LOCKED" );

    if( stateBits & IN_EDIT )
        ret << wxT( " | IN_EDIT" );

    if( stateBits & IS_DRAGGED )
        ret << wxT( " | IS_DRAGGED" );

    if( stateBits & DO_NOT_DRAW )
        ret << wxT( " | DO_NOT_DRAW" );

    if( stateBits & IS_DELETED )
        ret << wxT( " | IS_DELETED" );

    if( stateBits & END_ONPAD )
        ret << wxT( " | END_ONPAD" );

    if( stateBits & BEGIN_ONPAD )
        ret << wxT( " | BEGIN_ONPAD" );

    return ret;
}

#endif


static struct TRACK_VIA_DESC
{
    TRACK_VIA_DESC()
    {
        ENUM_MAP<VIATYPE>::Instance()
            .Undefined( VIATYPE::NOT_DEFINED )
            .Map( VIATYPE::THROUGH,      _HKI( "Through" ) )
            .Map( VIATYPE::BLIND_BURIED, _HKI( "Blind/buried" ) )
            .Map( VIATYPE::MICROVIA,     _HKI( "Micro" ) );

        ENUM_MAP<PCB_LAYER_ID>& layerEnum = ENUM_MAP<PCB_LAYER_ID>::Instance();

        if( layerEnum.Choices().GetCount() == 0 )
        {
            layerEnum.Undefined( UNDEFINED_LAYER );

            for( LSEQ seq = LSET::AllLayersMask().Seq(); seq; ++seq )
                layerEnum.Map( *seq, LSET::Name( *seq ) );
        }

        PROPERTY_MANAGER& propMgr = PROPERTY_MANAGER::Instance();

        // Track
        REGISTER_TYPE( TRACK );
        propMgr.InheritsAfter( TYPE_HASH( TRACK ), TYPE_HASH( BOARD_CONNECTED_ITEM ) );

        propMgr.AddProperty( new PROPERTY<TRACK, int>( _HKI( "Width" ),
            &TRACK::SetWidth, &TRACK::GetWidth, PROPERTY_DISPLAY::DISTANCE ) );
        propMgr.ReplaceProperty( TYPE_HASH( BOARD_ITEM ), _HKI( "Position X" ),
            new PROPERTY<TRACK, int, BOARD_ITEM>( _HKI( "Origin X" ),
            &TRACK::SetX, &TRACK::GetX, PROPERTY_DISPLAY::DISTANCE ) );
        propMgr.ReplaceProperty( TYPE_HASH( BOARD_ITEM ), _HKI( "Position Y" ),
            new PROPERTY<TRACK, int, BOARD_ITEM>( _HKI( "Origin Y" ),
            &TRACK::SetY, &TRACK::GetY, PROPERTY_DISPLAY::DISTANCE ) );
        propMgr.AddProperty( new PROPERTY<TRACK, int>( _HKI( "End X" ),
            &TRACK::SetEndX, &TRACK::GetEndX, PROPERTY_DISPLAY::DISTANCE ) );
        propMgr.AddProperty( new PROPERTY<TRACK, int>( _HKI( "End Y" ),
            &TRACK::SetEndY, &TRACK::GetEndY, PROPERTY_DISPLAY::DISTANCE ) );

        // Arc
        REGISTER_TYPE( ARC );
        propMgr.InheritsAfter( TYPE_HASH( ARC ), TYPE_HASH( BOARD_CONNECTED_ITEM ) );

        propMgr.AddProperty( new PROPERTY<TRACK, int>( _HKI( "Width" ),
            &ARC::SetWidth, &ARC::GetWidth, PROPERTY_DISPLAY::DISTANCE ) );
        propMgr.ReplaceProperty( TYPE_HASH( BOARD_ITEM ), _HKI( "Position X" ),
            new PROPERTY<ARC, int, BOARD_ITEM>( _HKI( "Origin X" ),
            &TRACK::SetX, &ARC::GetX, PROPERTY_DISPLAY::DISTANCE ) );
        propMgr.ReplaceProperty( TYPE_HASH( BOARD_ITEM ), _HKI( "Position Y" ),
            new PROPERTY<ARC, int, BOARD_ITEM>( _HKI( "Origin Y" ),
            &TRACK::SetY, &ARC::GetY, PROPERTY_DISPLAY::DISTANCE ) );
        propMgr.AddProperty( new PROPERTY<TRACK, int>( _HKI( "End X" ),
            &TRACK::SetEndX, &ARC::GetEndX, PROPERTY_DISPLAY::DISTANCE ) );
        propMgr.AddProperty( new PROPERTY<TRACK, int>( _HKI( "End Y" ),
            &TRACK::SetEndY, &ARC::GetEndY, PROPERTY_DISPLAY::DISTANCE ) );

        // Via
        REGISTER_TYPE( VIA );
        propMgr.InheritsAfter( TYPE_HASH( VIA ), TYPE_HASH( BOARD_CONNECTED_ITEM ) );

        // TODO layerset for vias?
        // TODO test drill, use getdrillvalue?
        propMgr.ReplaceProperty( TYPE_HASH( TRACK ), _HKI( "Width" ),
            new PROPERTY<VIA, int, TRACK>( _HKI( "Diameter" ),
            &VIA::SetWidth, &VIA::GetWidth, PROPERTY_DISPLAY::DISTANCE ) );
        propMgr.AddProperty( new PROPERTY<VIA, int>( _HKI( "Drill" ),
            &VIA::SetDrill, &VIA::GetDrillValue, PROPERTY_DISPLAY::DISTANCE ) );
        propMgr.ReplaceProperty( TYPE_HASH( BOARD_ITEM ), _HKI( "Layer" ),
            new PROPERTY_ENUM<VIA, PCB_LAYER_ID, BOARD_ITEM>( _HKI( "Layer Top" ),
            &VIA::SetLayer, &VIA::GetLayer ) );
        propMgr.AddProperty( new PROPERTY_ENUM<VIA, PCB_LAYER_ID>( _HKI( "Layer Bottom" ),
            &VIA::SetBottomLayer, &VIA::BottomLayer ) );
        propMgr.AddProperty( new PROPERTY_ENUM<VIA, VIATYPE>( _HKI( "Via Type" ),
            &VIA::SetViaType, &VIA::GetViaType ) );
    }
} _TRACK_VIA_DESC;

ENUM_TO_WXANY( VIATYPE );
